import fs from "fs";

const ENV = {
  HS: process.env.MATRIX_HOMESERVER_URL || "http://localhost:8008",
  ACCESS_TOKEN: process.env.MATRIX_ACCESS_TOKEN,
  LOCALPART: process.env.MATRIX_LOCALPART,
  MATRIX_PASS: process.env.MATRIX_PASSWORD,
  ROOM_ALIAS:
    process.env.MATRIX_ROOM_ALIAS || "#ayuda-en-cali:matrix.sismoinfo.co",
  API_BASE: process.env.API_BASE || "http://127.0.0.1:3000",
  RASA_URL: process.env.RASA_URL || "http://127.0.0.1:5005/model/parse",
  THRESHOLD: parseFloat(process.env.CONFIDENCE_THRESHOLD || "0.8"),
  WHITELIST: (process.env.INTENT_WHITELIST || "")
    .split(",")
    .map((s) => s.trim())
    .filter(Boolean),
  BOT_EMAIL: process.env.BOT_EMAIL,
  BOT_PASS: process.env.BOT_PASSWORD,
};

const STORAGE_FILE = "/opt/sismo-api/.bot-store/sync.json";
let syncToken = null;
let myUserId = null;
let targetRoomId = null;

async function matrixFetch(path, opts = {}) {
  const url = `${ENV.HS}${path}`;
  const headers = {
    "Content-Type": "application/json",
    ...(ENV.ACCESS_TOKEN
      ? { Authorization: `Bearer ${ENV.ACCESS_TOKEN}` }
      : {}),
    ...(opts.headers || {}),
  };
  const res = await fetch(url, { ...opts, headers });
  if (!res.ok) {
    const text = await res.text();
    throw new Error(`Matrix ${res.status}: ${text}`);
  }
  return res.json();
}

// 1. Login if we don't have a token
async function login() {
  if (ENV.ACCESS_TOKEN) {
    // Verify token works
    try {
      const whoami = await matrixFetch("/_matrix/client/v3/account/whoami");
      myUserId = whoami.user_id;
      console.log("[bot] Using existing token for", myUserId);
      return;
    } catch (e) {
      console.log("[bot] Token invalid, trying password login...");
    }
  }
  if (!ENV.LOCALPART || !ENV.MATRIX_PASS) {
    throw new Error(
      "Falta MATRIX_ACCESS_TOKEN o MATRIX_PASSWORD + MATRIX_LOCALPART",
    );
  }
  const data = await matrixFetch("/_matrix/client/v3/login", {
    method: "POST",
    body: JSON.stringify({
      type: "m.login.password",
      identifier: { type: "m.id.user", user: ENV.LOCALPART },
      password: ENV.MATRIX_PASS,
    }),
  });
  ENV.ACCESS_TOKEN = data.access_token;
  myUserId = data.user_id;
  console.log("[bot] Logged in as", myUserId);
  console.log("[bot] ACCESS_TOKEN:", data.access_token);
  console.log("[bot] → Guardá ese token en .env.bot como MATRIX_ACCESS_TOKEN");
}

// 2. Resolve and join room
async function joinRoom() {
  // Resolve alias
  const aliasRes = await matrixFetch(
    `/_matrix/client/v3/directory/room/${encodeURIComponent(ENV.ROOM_ALIAS)}`,
  );
  targetRoomId = aliasRes.room_id;
  console.log("[bot] Resolved room", ENV.ROOM_ALIAS, "→", targetRoomId);

  // Join
  try {
    await matrixFetch(
      `/_matrix/client/v3/rooms/${encodeURIComponent(targetRoomId)}/join`,
      { method: "POST", body: "{}" },
    );
    console.log("[bot] Joined room");
  } catch (e) {
    if (
      e.message.includes("M_UNKNOWN") ||
      e.message.includes("already in the room")
    ) {
      console.log("[bot] Already in room");
    } else {
      throw e;
    }
  }
}

// 3. Process a single message event
async function processMessage(roomId, event) {
  if (event.sender === myUserId) return;
  if (event.content?.msgtype !== "m.text") return;

  const text = event.content.body;
  console.log(`[bot] 📩 ${event.sender}: ${text.substring(0, 120)}`);

  // Rasa NLU
  let rasaRes;
  try {
    rasaRes = await fetch(ENV.RASA_URL, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ text }),
    }).then((r) => r.json());
  } catch (e) {
    console.error("[bot] ❌ Rasa unreachable:", e.message);
    return;
  }

  const intent = rasaRes?.intent;
  if (!intent) {
    console.log("[bot] ⚠️ No intent from Rasa");
    return;
  }
  console.log(`[bot] 🧠 intent=${intent.name} confidence=${intent.confidence}`);

  if (intent.confidence < ENV.THRESHOLD) {
    console.log("[bot] ⏭️ Below threshold");
    return;
  }
  if (ENV.WHITELIST.length && !ENV.WHITELIST.includes(intent.name)) {
    console.log("[bot] ⏭️ Not in whitelist");
    return;
  }

  // Login to sismo-api
  let apiToken;
  try {
    const loginRes = await fetch(`${ENV.API_BASE}/auth/login`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({
        identifier: ENV.BOT_EMAIL,
        password: ENV.BOT_PASS,
      }),
    }).then((r) => r.json());
    apiToken = loginRes.token;
    if (!apiToken)
      throw new Error("No token in response: " + JSON.stringify(loginRes));
  } catch (e) {
    console.error("[bot] ❌ sismo-api login failed:", e.message);
    return;
  }

  // POST backlog
  try {
    const backlogRes = await fetch(`${ENV.API_BASE}/backlog`, {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
        Authorization: `Bearer ${apiToken}`,
      },
      body: JSON.stringify({
        source: "matrix",
        room_id: roomId,
        event_id: event.event_id,
        sender: event.sender,
        raw_text: text,
        parsed_intent: intent.name,
        confidence: intent.confidence,
        entities: rasaRes.entities || [],
        timestamp: Date.now(),
      }),
    });
    if (backlogRes.ok) {
      console.log("[bot] ✅ Backlog created");
    } else {
      const errText = await backlogRes.text();
      console.error("[bot] ❌ Backlog POST", backlogRes.status, errText);
    }
  } catch (e) {
    console.error("[bot] ❌ Backlog error:", e.message);
  }
}

// 4. Main sync loop
async function syncLoop() {
  // Load previous sync token
  if (fs.existsSync(STORAGE_FILE)) {
    try {
      const saved = JSON.parse(fs.readFileSync(STORAGE_FILE, "utf8"));
      syncToken = saved.next_batch;
      console.log("[bot] Resumed from sync token");
    } catch { }
  }

  while (true) {
    let url = `/_matrix/client/v3/sync?timeout=30000`;
    if (syncToken) url += `&since=${encodeURIComponent(syncToken)}`;

    let data;
    try {
      data = await matrixFetch(url);
    } catch (e) {
      console.error("[bot] Sync error:", e.message);
      await new Promise((r) => setTimeout(r, 5000));
      continue;
    }

    syncToken = data.next_batch;
    fs.writeFileSync(STORAGE_FILE, JSON.stringify({ next_batch: syncToken }));

    const rooms = data.rooms?.join || {};
    for (const [roomId, roomData] of Object.entries(rooms)) {
      if (roomId !== targetRoomId) continue;
      const events = roomData.timeline?.events || [];
      for (const event of events) {
        if (event.type === "m.room.message") {
          await processMessage(roomId, event);
        }
      }
    }
  }
}

// Start
await login();
await joinRoom();
console.log("[bot] Starting sync loop...");
await syncLoop();

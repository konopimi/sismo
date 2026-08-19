import {
  MatrixClient,
  SimpleFsStorageProvider,
  AutojoinRoomsMixin,
} from "matrix-bot-sdk";
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

const STORAGE_DIR = "/opt/sismo-api/.bot-store";
if (!fs.existsSync(STORAGE_DIR)) fs.mkdirSync(STORAGE_DIR, { recursive: true });

const storage = new SimpleFsStorageProvider(`${STORAGE_DIR}/storage.json`);
let client;

if (ENV.ACCESS_TOKEN) {
  client = new MatrixClient(ENV.HS, ENV.ACCESS_TOKEN, storage);
  console.log("[bot] Using existing access token");
} else if (ENV.LOCALPART && ENV.MATRIX_PASS) {
  client = new MatrixClient(ENV.HS, "", storage);
  const loginRes = await client.loginWithPassword(
    ENV.LOCALPART,
    ENV.MATRIX_PASS,
  );
  console.log("[bot] Logged in. ACCESS_TOKEN:", loginRes.accessToken);
  console.log("[bot] USER_ID:", loginRes.userId);
  console.log("[bot] → Save that token to .env.bot as MATRIX_ACCESS_TOKEN");
  // Recreate client with the obtained token to ensure it's used for sync
  if (loginRes.accessToken) {
    client = new MatrixClient(ENV.HS, loginRes.accessToken, storage);
  }
} else {
  console.error(
    "[bot] Faltan credenciales: MATRIX_ACCESS_TOKEN o MATRIX_PASSWORD + MATRIX_LOCALPART",
  );
  process.exit(1);
}

AutojoinRoomsMixin.setupOnClient(client);

client.start().then(async () => {
  const userId = await client.getUserId();
  console.log("[bot] Syncing as", userId);

  try {
    const roomId = await client.resolveRoom(ENV.ROOM_ALIAS);
    await client.joinRoom(roomId);
    console.log("[bot] Joined room:", ENV.ROOM_ALIAS, "→", roomId);
  } catch (e) {
    console.error("[bot] Could not join room:", e.message);
  }
});

client.on("room.message", async (roomId, event) => {
  const myUserId = await client.getUserId();
  if (event.sender === myUserId) return;
  if (event.content?.msgtype !== "m.text") return;

  const text = event.content.body;
  console.log(`[bot] 📩 ${event.sender}: ${text}`);

  // --- 1. Rasa NLU ---
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
    console.log("[bot] ⚠️ No intent returned by Rasa");
    return;
  }

  console.log(`[bot] 🧠 intent=${intent.name} confidence=${intent.confidence}`);

  if (intent.confidence < ENV.THRESHOLD) {
    console.log("[bot] ⏭️ Confidence below threshold, ignoring");
    return;
  }
  if (ENV.WHITELIST.length && !ENV.WHITELIST.includes(intent.name)) {
    console.log("[bot] ⏭️ Intent not in whitelist, ignoring");
    return;
  }

  // --- 2. Login to sismo-api ---
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
    if (!apiToken) throw new Error("No token in response");
  } catch (e) {
    console.error("[bot] ❌ sismo-api login failed:", e.message);
    return;
  }

  // --- 3. POST backlog ---
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
      console.log("[bot] ✅ Backlog entry created");
    } else {
      const errText = await backlogRes.text();
      console.error("[bot] ❌ Backlog POST", backlogRes.status, errText);
    }
  } catch (e) {
    console.error("[bot] ❌ Backlog error:", e.message);
  }
});

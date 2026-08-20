import fs from "fs";

const envPath = "/opt/sismo-api/.env.bot";
if (fs.existsSync(envPath)) {
  const content = fs.readFileSync(envPath, "utf8");
  for (const line of content.split(/\r?\n/)) {
    const match = line.match(/^\s*([A-Za-z0-9_]+)\s*=\s*(.*?)\s*$/);
    if (match && !process.env[match[1]]) {
      process.env[match[1]] = match[2];
    }
  }
}

const ENV = {
  HS: process.env.MATRIX_HOMESERVER_URL || "http://localhost:8008",
  ACCESS_TOKEN: process.env.MATRIX_ACCESS_TOKEN,
  LOCALPART: process.env.MATRIX_LOCALPART,
  MATRIX_PASS: process.env.MATRIX_PASSWORD,
  ROOM_ALIAS:
    process.env.MATRIX_ROOM_ALIAS || "#ayuda-en-cali:matrix.sismoinfo.co",
  API_BASE: process.env.API_BASE || "http://127.0.0.1:3000",
  RASA_URL: process.env.RASA_URL || "http://127.0.0.1:5005/model/parse",
  THRESHOLD: parseFloat(process.env.CONFIDENCE_THRESHOLD || "0.5"),
  BLACKLIST: (process.env.INTENT_BLACKLIST || "")
    .split(",")
    .map((s) => s.trim())
    .filter(Boolean),
  BOT_EMAIL: process.env.BOT_EMAIL,
  BOT_PASS: process.env.BOT_PASSWORD,
  NVIDIA_API_KEY: process.env.NVIDIA_API_KEY || "",
};

import pLimit from "p-limit";

const STORAGE_DIR = "/opt/sismo-api/.bot-store";
if (!fs.existsSync(STORAGE_DIR)) fs.mkdirSync(STORAGE_DIR, { recursive: true });
const STORAGE_FILE = `${STORAGE_DIR}/sync.json`;

let apiToken = null;
let myUserId = null;
let targetRoomId = null;
let nextBatch = null;
let jobErrors = 0;
const MAX_JOB_ERRORS = 50;
const BUFFER_TIMEOUT_MS = 8000;
const userBuffers = new Map();
const processLimit = pLimit(3);

// ================================================================
//  API AUTHENTICATION
// ================================================================
async function authenticateWithApi() {
  console.log("[bot] Authenticating with API...");
  const res = await fetch(`${ENV.API_BASE}/api/auth/login`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ identifier: ENV.BOT_EMAIL, password: ENV.BOT_PASS }),
  });
  if (!res.ok) throw new Error(`API Login failed: ${res.status}`);
  apiToken = (await res.json()).token;
  console.log("[bot] API Auth successful.");
}

async function safeApiCall(url, options) {
  if (!apiToken) {
    try { await authenticateWithApi(); }
    catch (e) { throw new Error(`Cannot make API call: auth failed - ${e.message}`); }
  }
  const opts = {
    ...options,
    headers: { ...(options.headers || {}), Authorization: `Bearer ${apiToken}` }
  };
  let res = await fetch(url, opts);
  if (res.status === 401) {
    console.warn("[bot] API 401. Re-authenticating...");
    try { await authenticateWithApi(); }
    catch (e) { throw new Error(`Re-auth failed: ${e.message}`); }
    opts.headers.Authorization = `Bearer ${apiToken}`;
    res = await fetch(url, opts);
  }
  return res;
}

// ================================================================
//  MATRIX HELPERS (Raw Fetch — no SDK)
// ================================================================
async function matrixGet(path) {
  const res = await fetch(`${ENV.HS}${path}`, {
    headers: { Authorization: `Bearer ${ENV.ACCESS_TOKEN}` }
  });
  if (!res.ok) throw new Error(`Matrix GET ${path}: ${res.status}`);
  return res.json();
}

async function matrixPost(path, body) {
  const res = await fetch(`${ENV.HS}${path}`, {
    method: "POST",
    headers: {
      Authorization: `Bearer ${ENV.ACCESS_TOKEN}`,
      "Content-Type": "application/json"
    },
    body: JSON.stringify(body)
  });
  if (!res.ok) throw new Error(`Matrix POST ${path}: ${res.status}`);
  return res.json();
}

async function downloadMatrixMedia(mxcUrl) {
  const match = mxcUrl.match(/^mxc:\/\/([^/]+)\/(.+)$/);
  if (!match) throw new Error(`Invalid mxc URL: ${mxcUrl}`);
  const [, serverName, mediaId] = match;
  const res = await fetch(`${ENV.HS}/_matrix/media/v3/download/${serverName}/${mediaId}`, {
    headers: { Authorization: `Bearer ${ENV.ACCESS_TOKEN}` }
  });
  if (!res.ok) throw new Error(`Media download failed: ${res.status}`);
  const contentType = res.headers.get("content-type") || "application/octet-stream";
  const buffer = Buffer.from(await res.arrayBuffer());
  return { buffer, contentType };
}

// ================================================================
//  CONVERSATION BUFFER SYSTEM
// ================================================================
class UserBuffer {
  constructor(userId) {
    this.userId = userId;
    this.messages = [];
    this.timer = null;
  }
  add(msg) {
    this.messages.push(msg);
    this.resetTimer();
  }
  resetTimer() {
    if (this.timer) clearTimeout(this.timer);
    this.timer = setTimeout(() => this.flush(), BUFFER_TIMEOUT_MS);
  }
  async flush() {
    if (this.messages.length === 0) return;
    const batch = [...this.messages];
    this.messages = [];
    this.timer = null;
    userBuffers.delete(this.userId);
    processLimit(() => processMessageBatch(this.userId, batch)).catch(err => {
      jobErrors++;
      console.error(`[bot] Batch error #${jobErrors}:`, err);
      if (jobErrors > MAX_JOB_ERRORS) process.exit(1);
    });
  }
}

function getBuffer(userId) {
  if (!userBuffers.has(userId)) userBuffers.set(userId, new UserBuffer(userId));
  return userBuffers.get(userId);
}

const SHOUT_PATTERN = /(AUXILIO|URGENTE|SOCORRO|EMERGENCIA|AYUDA|PELIGRO)/i;
const LOSS_PATTERN = /(se\s+va(n)?\s+(a\s+)?perder|ayudaa+|ya\s+no\s+hay|sin\s+nada)/i;

function shouldFlushImmediately(msg, buffer) {
  const text = msg.text || "";
  if (text.includes("@room")) return true;

  const isShortShout = text.length < 30 && text === text.toUpperCase() && SHOUT_PATTERN.test(text);
  const isLossPhrase = LOSS_PATTERN.test(text);
  if (isShortShout || isLossPhrase) return true;

  if (msg.replyTo && buffer.messages.length > 0) {
    const age = msg.timestamp - buffer.messages[0].timestamp;
    if (age > 5 * 60 * 1000) return true;
  }
  return false;
}

// ================================================================
//  PAYMENT-INFO DETECTION (fraud vector flagging)
// ================================================================
const PAYMENT_CONTEXT = /(cuenta|nequi|bancolombia|llave|transfier|consignar|ahorros|corriente)/i;
const ACCOUNT_NUMBER = /\b\d{8,14}\b/;
const NEQUI_HANDLE = /@\w{4,}(?!\.(jpg|png|jpeg|webp|mp4|opus))/i;

function detectPaymentInfo(text) {
  if (!text || !PAYMENT_CONTEXT.test(text)) return false;
  return ACCOUNT_NUMBER.test(text) || NEQUI_HANDLE.test(text);
}

// ================================================================
//  REPLY CONTEXT RESOLUTION
// ================================================================
async function resolveReplyContext(roomId, eventId) {
  if (!eventId) return null;
  try {
    const event = await matrixGet(
      `/_matrix/client/v3/rooms/${encodeURIComponent(roomId)}/event/${encodeURIComponent(eventId)}`
    );
    return {
      eventId: event.event_id,
      sender: event.sender,
      content: event.content || {},
      isImage: event.content?.msgtype === "m.image",
      isAudio: event.content?.msgtype === "m.audio",
      isText: event.content?.msgtype === "m.text",
    };
  } catch (e) {
    console.error(`[bot] Failed to fetch parent ${eventId}:`, e.message);
    return null;
  }
}

// ================================================================
//  OCR (NVIDIA VISION)
// ================================================================
async function extractTextFromImage(mxcUrl) {
  if (!ENV.NVIDIA_API_KEY) { console.log("[bot] ⚠️ No NVIDIA_API_KEY"); return ""; }
  try {
    const { buffer, contentType } = await downloadMatrixMedia(mxcUrl);
    if (buffer.length > 10 * 1024 * 1024) { console.warn("[bot] ⚠️ Image >10MB"); return ""; }

    const base64Image = buffer.toString("base64");
    const mimeType = contentType || "image/jpeg";

    const nvidiaRes = await fetch("https://integrate.api.nvidia.com/v1/chat/completions", {
      method: "POST",
      headers: {
        "Authorization": `Bearer ${ENV.NVIDIA_API_KEY}`,
        "Content-Type": "application/json"
      },
      body: JSON.stringify({
        model: "meta/llama-3.2-11b-vision-instruct",
        messages: [
          {
            role: "system",
            content: "You are a precise OCR engine. Output only raw extracted text, never markdown or explanations."
          },
          {
            role: "user",
            content: [
              { type: "text", text: "Extract all text from this image. Output only the raw extracted text, nothing else." },
              { type: "image_url", image_url: { url: `data:${mimeType};base64,${base64Image}` } }
            ]
          }
        ],
        max_tokens: 4096,
        temperature: 0
      })
    });

    if (!nvidiaRes.ok) { console.error(`[bot] ❌ NVIDIA ${nvidiaRes.status}`); return ""; }
    const data = await nvidiaRes.json();
    return data.choices?.[0]?.message?.content?.trim() || "";
  } catch (e) {
    console.error("[bot] ❌ OCR failed:", e.message);
    return "";
  }
}

// ================================================================
//  NIM STRUCTURED EXTRACTION (SLOW PATH)
// ================================================================
async function structureWithNIM(rawText) {
  const res = await fetch("https://integrate.api.nvidia.com/v1/chat/completions", {
    method: "POST",
    headers: {
      "Authorization": `Bearer ${ENV.NVIDIA_API_KEY}`,
      "Content-Type": "application/json"
    },
    body: JSON.stringify({
      model: "meta/llama-3.1-8b-instruct",
      messages: [
        {
          role: "system",
          content: `You are a precise data extraction engine for disaster response.
Respond with ONLY a valid JSON object. Do not include markdown formatting (like \`\`\`json), prose, or explanations.
The JSON must strictly follow this schema:
{
  "intent": "donate_offer" | "request_help" | "report_need" | "report_emergency" | "find_supplier" | "volunteer_recruitment" | "request_item" | "mental_health" | "unknown",
  "items": [{"name": "string", "quantity": number, "unit": "string"}],
  "location": "string",
  "urgency": "low" | "medium" | "high" | "critical",
  "contact": "string",
  "notes": "string"
}
CRITICAL RULES — extract ONLY what is explicitly stated in the text:
- NEVER invent quantities, units, locations, or contacts that are not present.
- If a quantity is not stated, set "quantity" to null (do NOT guess a number).
- If a unit is not stated, set "unit" to null (do NOT use articles like "unas"/"unos" as units).
- If a location is not stated, set "location" to null (do NOT use the first word of the sentence).
- If no items are mentioned, use [].
- Use "unknown" for intent if unclear.
- Extract quantities as numbers, not strings.`
        },
        { role: "user", content: rawText }
      ],
      temperature: 0,
      max_tokens: 1024
    })
  });

  if (!res.ok) {
    const errText = await res.text();
    throw new Error(`NIM API failed: ${res.status} ${errText.substring(0, 200)}`);
  }

  const data = await res.json();
  const raw = data.choices?.[0]?.message?.content?.trim() || "";

  if (!raw) throw new Error("NIM returned empty content");

  // Robust JSON extraction: find the first { ... } block to ignore markdown or prose
  const jsonMatch = raw.match(/\{[\s\S]*\}/);
  if (!jsonMatch) {
    console.error("[bot] ❌ NIM raw output (no JSON found):", raw);
    throw new Error("No JSON object found in NIM output");
  }

  let parsed;
  try {
    parsed = JSON.parse(jsonMatch[0]);
  } catch (e) {
    console.error("[bot] ❌ NIM JSON parse error:", e.message, "| Raw snippet:", jsonMatch[0].substring(0, 150));
    throw new Error("Malformed JSON in NIM output");
  }

  // Basic schema validation
  if (!parsed.intent || !Array.isArray(parsed.items)) {
    console.error("[bot] ❌ NIM schema validation failed:", parsed);
    throw new Error("Malformed NIM output schema: missing intent or items array.");
  }

  return parsed;
}

// Flatten NIM structured output into { entity, value } entries the frontend renders.
// Each item becomes an "item" entity; quantity/unit/location/urgency become their own entries.
function nimEntitiesToFlat(structured) {
  const out = [];
  for (const i of structured.items || []) {
    if (i.name) out.push({ entity: "item", value: i.name });
    if (i.quantity != null) out.push({ entity: "quantity", value: String(i.quantity) });
    if (i.unit) out.push({ entity: "unit", value: i.unit });
  }
  if (structured.location && structured.location !== "desconocido" && structured.location !== "desconocida") {
    out.push({ entity: "location", value: structured.location });
  }
  if (structured.urgency) out.push({ entity: "urgency", value: structured.urgency });
  return out;
}

// ================================================================
//  BATCH PROCESSOR
// ================================================================
async function processMessageBatch(userId, messages) {
  if (userId === myUserId) return;

  const parentMap = new Map();
  for (const msg of messages) {
    if (msg.replyTo && (!msg.text || msg.text.length < 15)) {
      const parent = await resolveReplyContext(targetRoomId, msg.replyTo);
      if (parent) parentMap.set(msg.replyTo, parent);
    }
  }

  let primaryEventId = messages[0].eventId;
  let primaryIsImage = false;
  for (const msg of messages) {
    if (msg.imageUrl) { primaryEventId = msg.eventId; primaryIsImage = true; break; }
  }
  if (!primaryIsImage) {
    for (const msg of messages) {
      if (msg.replyTo && parentMap.get(msg.replyTo)?.isImage) {
        primaryEventId = msg.replyTo; primaryIsImage = true; break;
      }
    }
  }

  const textParts = [];
  const imageUrls = [];
  const audioUrls = [];
  const seenUrls = new Set();

  for (const [eventId, parent] of parentMap) {
    if (parent.isImage && parent.content?.url && !seenUrls.has(parent.content.url)) {
      seenUrls.add(parent.content.url);
      imageUrls.push({ url: parent.content.url, source: "parent", eventId });
    } else if (parent.isAudio && parent.content?.url) {
      audioUrls.push({ url: parent.content.url, source: "parent", eventId });
    } else if (parent.isText && parent.content?.body) {
      textParts.push(`[Contexto anterior: ${parent.content.body}]`);
    }
  }

  for (const msg of messages) {
    if (msg.text) textParts.push(msg.text);
    if (msg.imageUrl && !seenUrls.has(msg.imageUrl)) {
      seenUrls.add(msg.imageUrl);
      imageUrls.push({ url: msg.imageUrl, source: "direct", eventId: msg.eventId });
    }
    if (msg.audioUrl) {
      audioUrls.push({ url: msg.audioUrl, source: "direct", eventId: msg.eventId });
    }
  }

  // Log audio (transcription TBD)
  for (const { url, source, eventId } of audioUrls) {
    console.log(`[bot] 🎤 Audio ${source} (${eventId}): ${url}`);
  }

  const ocrParts = [];
  for (const { url, source, eventId } of imageUrls) {
    console.log(`[bot] 🔍 OCR ${source} image (${eventId})...`);
    const ocrText = await extractTextFromImage(url);
    if (ocrText) {
      ocrParts.push(ocrText);

      // Persist immediately, keyed by this image's own event ID — independent
      // of whatever Rasa/NIM/whitelist decide about the batch downstream.
      try {
        await safeApiCall(`${ENV.API_BASE}/api/media-text`, {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({ source_event_id: eventId, raw_text: ocrText })
        });
        console.log(`[bot] ✅ Persisted OCR for ${eventId}`);
      } catch (e) {
        console.error(`[bot] ⚠️ Failed to persist OCR for ${eventId}:`, e.message);
      }
    }
  }

  let combinedText = textParts.join('\n');
  if (ocrParts.length > 0) {
    const ocrBlock = ocrParts.join('\n');
    combinedText = combinedText ? `${combinedText}\n${ocrBlock}` : ocrBlock;
  }

  if (!combinedText?.trim() || combinedText.trim().length < 3) {
    console.log("[bot] ⏭️ Empty combined text after buffer flush");
    return;
  }

  console.log(`[bot] 📦 Batch from ${userId}: "${combinedText.substring(0, 150)}..."`);

  const paymentFlag = detectPaymentInfo(combinedText);

  let rasaRes;
  try {
    const res = await fetch(ENV.RASA_URL, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ text: combinedText.trim() })
    });
    if (!res.ok) throw new Error(`Rasa HTTP ${res.status}`);
    rasaRes = await res.json();
  } catch (e) {
    console.error("[bot] ❌ Rasa unreachable:", e.message);
    return;
  }

  const intent = rasaRes?.intent;
  const entities = rasaRes.entities || [];
  if (!intent) { console.log("[bot] ⚠️ No intent from Rasa"); return; }

  console.log(`[bot] 🧠 Rasa: ${intent.name} (${intent.confidence})`);

  // Blacklist: block known-noise intents (unless payment info is present)
  if (ENV.BLACKLIST.includes(intent.name) && !paymentFlag) {
    console.log(`[bot] ⏭️ '${intent.name}' blacklisted`);
    return;
  }

  let finalIntent = intent.name;
  // Normalize Rasa entities to { entity, value } shape the frontend renders
  let finalEntities = entities.map((e) => ({ entity: e.entity, value: e.value }));
  let enrichedData = null;

  // Low confidence OR explicit fallback: always route through NIM for enrichment before saving
  if (intent.name === "nlu_fallback" || intent.confidence < ENV.THRESHOLD) {
    console.log("[bot] 🐢 Low confidence / Fallback, NIM structuring");
    try {
      const structured = await structureWithNIM(combinedText);
      if (structured.intent !== "unknown") finalIntent = structured.intent;
      finalEntities = nimEntitiesToFlat(structured);
      enrichedData = structured;
      console.log(`[bot] ✅ NIM: ${finalIntent}`);
    } catch (e) {
      console.error("[bot] ⚠️ NIM failed, saving with Rasa raw:", e.message);
      // fall through — save with Rasa's intent even if low confidence
    }
  } else if (intent.confidence > 0.90 && entities.length >= 2) {
    console.log("[bot] ⚡ Fast path");
  } else {
    console.log("[bot] 🐢 Slow path: NIM structuring");
    try {
      const structured = await structureWithNIM(combinedText);
      if (structured.intent !== "unknown") finalIntent = structured.intent;
      finalEntities = nimEntitiesToFlat(structured);
      enrichedData = structured;
      console.log(`[bot] ✅ NIM: ${finalIntent}`);
    } catch (e) {
      console.error("[bot] ⚠️ NIM failed, using Rasa raw:", e.message);
    }
  }

  if (finalIntent === "nlu_fallback" && !paymentFlag) {
    console.log("[bot] ⏭️ Unresolvable fallback, skipping save");
    return;
  }

  if (paymentFlag) {
    finalEntities.push({ entity: "payment_info_detected", value: "true" });
    console.log(`[bot] 🚩 Payment info detected in batch from ${userId}`);
  }

  try {
    const res = await safeApiCall(`${ENV.API_BASE}/api/backlog`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({
        source_event_id: primaryEventId,
        creator_matrix_id: userId,
        intent: finalIntent,
        extracted_data: finalEntities,
        raw_text: combinedText.trim(),
        raw_json: JSON.stringify({ matrix_events: messages, rasa_response: rasaRes }, null, 2),
        enriched_data: enrichedData ? JSON.stringify(enrichedData) : null,
        message_count: messages.length
      })
    });

    if (res.status === 409) console.log(`[bot] ⏩ Duplicate: ${primaryEventId}`);
    else if (res.ok) console.log(`[bot] ✅ Backlog: ${primaryEventId} (${messages.length} msg(s))`);
    else {
      const errText = await res.text().catch(() => "No body");
      console.error(`[bot] ❌ Backlog ${res.status}: ${errText.substring(0, 200)}`);
    }
  } catch (e) {
    console.error(`[bot] ❌ Backlog error:`, e.message);
    jobErrors++;
    if (jobErrors > MAX_JOB_ERRORS) { console.error("[bot] 🚨 Max errors. Exiting."); process.exit(1); }
  }
}

// ================================================================
//  SYNC LOOP (Raw Fetch)
// ================================================================
async function doSync() {
  const params = new URLSearchParams({ timeout: "30000" });
  if (nextBatch) params.set("since", nextBatch);
  params.set("filter", JSON.stringify({
    room: {
      rooms: [targetRoomId],
      timeline: { limit: 100, types: ["m.room.message"] }
    }
  }));

  const data = await matrixGet(`/_matrix/client/v3/sync?${params.toString()}`);

  const roomData = data.rooms?.join?.[targetRoomId];
  if (!roomData) {
    // No events, but still advance the token so we don't re-read the same batch.
    nextBatch = data.next_batch;
    fs.writeFileSync(STORAGE_FILE, JSON.stringify({ next_batch: nextBatch }));
    return;
  }

  const events = roomData.timeline?.events || [];
  for (const event of events) {
    if (event.sender === myUserId) continue;
    if (event.type !== "m.room.message") continue;

    const content = event.content || {};
    if (content.msgtype === "m.reaction") continue;
    if (content["m.relates_to"]?.rel_type === "m.replace") continue;

    let text = content.body || "";
    const isReply = content["m.relates_to"]?.["m.in_reply_to"]?.event_id;
    if (isReply) {
      text = text.replace(/^>(?:>|\s).*?\n\n/, '').trim() || text;
    }

    const msg = {
      eventId: event.event_id,
      text: content.msgtype === "m.text" ? text : "",
      imageUrl: content.msgtype === "m.image" ? content.url : null,
      audioUrl: content.msgtype === "m.audio" ? content.url : null,
      replyTo: isReply || null,
      timestamp: event.origin_server_ts,
    };

    const buffer = getBuffer(event.sender);
    if (shouldFlushImmediately(msg, buffer)) {
      buffer.add(msg);
      await buffer.flush();
    } else {
      buffer.add(msg);
    }
  }

  // Persist the sync token ONLY after events are safely handed to buffers.
  // This narrows the data-loss window from the full 8s buffer timeout to the
  // synchronous event loop above (milliseconds).
  nextBatch = data.next_batch;
  fs.writeFileSync(STORAGE_FILE, JSON.stringify({ next_batch: nextBatch }));
}

// ================================================================
//  BOOT
// ================================================================
async function boot() {
  console.log("[bot] Booting...");

  await authenticateWithApi();

  const whoami = await matrixGet("/_matrix/client/v3/account/whoami");
  myUserId = whoami.user_id;
  console.log(`[bot] Logged in as: ${myUserId}`);

  const roomResolve = await matrixGet(
    `/_matrix/client/v3/directory/room/${encodeURIComponent(ENV.ROOM_ALIAS)}`
  );
  targetRoomId = roomResolve.room_id;
  console.log(`[bot] Target room: ${targetRoomId}`);

  try {
    await matrixPost(`/_matrix/client/v3/rooms/${encodeURIComponent(targetRoomId)}/join`, {});
    console.log("[bot] Joined room");
  } catch (e) {
    console.log("[bot] Room join:", e.message);
  }

  if (fs.existsSync(STORAGE_FILE)) {
    try {
      const saved = JSON.parse(fs.readFileSync(STORAGE_FILE, "utf8"));
      nextBatch = saved.next_batch;
      console.log("[bot] Resumed sync");
    } catch { }
  }

  console.log("[bot] 🚀 Operational.");
  setInterval(() => console.log("[bot] ❤️ Heartbeat"), 10 * 60 * 1000);

  process.on("SIGTERM", async () => {
    console.log("[bot] 🛑 SIGTERM. Draining buffers...");
    const flushes = Array.from(userBuffers.values()).map(b => b.flush());
    await Promise.allSettled(flushes);
    process.exit(0);
  });

  while (true) {
    try {
      await doSync();
    } catch (e) {
      console.error("[bot] Sync error:", e.message);
      await new Promise(r => setTimeout(r, 5000));
    }
  }
}

boot().catch(e => {
  console.error("[bot] Fatal boot error:", e);
  process.exit(1);
});

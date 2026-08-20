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

// Widened from 8s -> 30s so multi-image + text bursts land in the same
// window and can be semantically grouped instead of split by a timer.
const BUFFER_TIMEOUT_MS = 30000;

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
    try {
      await authenticateWithApi();
    } catch (e) {
      throw new Error(`Cannot make API call: auth failed - ${e.message}`);
    }
  }
  const opts = {
    ...options,
    headers: {
      ...(options.headers || {}),
      Authorization: `Bearer ${apiToken}`,
    },
  };
  let res = await fetch(url, opts);
  if (res.status === 401) {
    console.warn("[bot] API 401. Re-authenticating...");
    try {
      await authenticateWithApi();
    } catch (e) {
      throw new Error(`Re-auth failed: ${e.message}`);
    }
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
    headers: { Authorization: `Bearer ${ENV.ACCESS_TOKEN}` },
  });
  if (!res.ok) throw new Error(`Matrix GET ${path}: ${res.status}`);
  return res.json();
}

async function matrixPost(path, body) {
  const res = await fetch(`${ENV.HS}${path}`, {
    method: "POST",
    headers: {
      Authorization: `Bearer ${ENV.ACCESS_TOKEN}`,
      "Content-Type": "application/json",
    },
    body: JSON.stringify(body),
  });
  if (!res.ok) throw new Error(`Matrix POST ${path}: ${res.status}`);
  return res.json();
}

async function downloadMatrixMedia(mxcUrl) {
  const match = mxcUrl.match(/^mxc:\/\/([^/]+)\/(.+)$/);
  if (!match) throw new Error(`Invalid mxc URL: ${mxcUrl}`);
  const [, serverName, mediaId] = match;
  const res = await fetch(
    `${ENV.HS}/_matrix/media/v3/download/${serverName}/${mediaId}`,
    {
      headers: { Authorization: `Bearer ${ENV.ACCESS_TOKEN}` },
    },
  );
  if (!res.ok) throw new Error(`Media download failed: ${res.status}`);
  const contentType =
    res.headers.get("content-type") || "application/octet-stream";
  const buffer = Buffer.from(await res.arrayBuffer());
  return { buffer, contentType };
}

// ================================================================
//  CONVERSATION BUFFER SYSTEM
//  Collects a 30s window per user. Media no longer flushes instantly —
//  it joins the same window as surrounding text so the grouping step
//  (see groupMessagesWithNIM) can decide what belongs together.
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
    if (this.timer) {
      clearTimeout(this.timer);
      this.timer = null;
    }
    userBuffers.delete(this.userId);
    processLimit(() => processMessageBatch(this.userId, batch)).catch((err) => {
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
const LOSS_PATTERN =
  /(se\s+va(n)?\s+(a\s+)?perder|ayudaa+|ya\s+no\s+hay|sin\s+nada)/i;

// Media no longer triggers an immediate flush — it now waits in the window
// with everything else so grouping can link it to nearby text. Only
// genuinely urgent signals (shouts, @room, stale replies) still cut the
// window short.
function shouldFlushImmediately(msg, buffer) {
  const text = msg.text || "";
  if (text.includes("@room")) return true;
  const isShortShout =
    text.length < 30 && text === text.toUpperCase() && SHOUT_PATTERN.test(text);
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
const PAYMENT_CONTEXT =
  /(cuenta|nequi|bancolombia|llave|transfier|consignar|ahorros|corriente)/i;
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
      `/_matrix/client/v3/rooms/${encodeURIComponent(roomId)}/event/${encodeURIComponent(eventId)}`,
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
  if (!ENV.NVIDIA_API_KEY) {
    console.log("[bot] ⚠️ No NVIDIA_API_KEY");
    return "";
  }
  try {
    const { buffer, contentType } = await downloadMatrixMedia(mxcUrl);
    if (buffer.length > 10 * 1024 * 1024) {
      console.warn("[bot] ⚠️ Image >10MB");
      return "";
    }
    const base64Image = buffer.toString("base64");
    const mimeType = contentType || "image/jpeg";

    const nvidiaRes = await fetch(
      "https://integrate.api.nvidia.com/v1/chat/completions",
      {
        method: "POST",
        headers: {
          Authorization: `Bearer ${ENV.NVIDIA_API_KEY}`,
          "Content-Type": "application/json",
        },
        body: JSON.stringify({
          model: "meta/llama-3.2-11b-vision-instruct",
          messages: [
            {
              role: "system",
              content:
                "You are a precise OCR engine. Output only raw extracted text, never markdown or explanations.",
            },
            {
              role: "user",
              content: [
                {
                  type: "text",
                  text: "Extract all text from this image. Output only the raw extracted text, nothing else.",
                },
                {
                  type: "image_url",
                  image_url: { url: `data:${mimeType};base64,${base64Image}` },
                },
              ],
            },
          ],
          max_tokens: 4096,
          temperature: 0,
        }),
      },
    );

    if (!nvidiaRes.ok) {
      console.error(`[bot] ❌ NVIDIA ${nvidiaRes.status}`);
      return "";
    }
    const data = await nvidiaRes.json();
    return data.choices?.[0]?.message?.content?.trim() || "";
  } catch (e) {
    console.error("[bot] ❌ OCR failed:", e.message);
    return "";
  }
}

// ================================================================
//  NIM MESSAGE GROUPING (hybrid: reply-hints + semantic segmentation)
//
//  Takes the flat list of items in a user's 30s window (each already
//  carrying its own OCR'd text if it was an image) and asks a cheap
//  8B model to segment them into logical items. Reply metadata is
//  passed in as a hint — the model treats it as a prior, not a rule,
//  so it can override a reply that points at the wrong message and
//  can also resolve bare deictic references ("esto", "el de la foto")
//  that have no reply attached at all.
//
//  Returns an array of groups, each group a list of 0-based indices
//  into `events`. On any failure, returns null so the caller can fall
//  back to "each event is its own item" (the safe default — see
//  decision #3: never silently merge unrelated items).
// ================================================================
async function groupMessagesWithNIM(events) {
  if (!ENV.NVIDIA_API_KEY) return null;
  if (events.length <= 1) return events.map((_, i) => [i]);

  const numbered = events
    .map((e, i) => {
      const kind =
        e.type === "image" ? "IMAGEN" : e.type === "audio" ? "AUDIO" : "TEXTO";
      const body = (e.text || "(sin texto)").replace(/\s+/g, " ").trim();
      const replyHint =
        e.replyToIndex != null
          ? ` (responde al mensaje #${e.replyToIndex + 1})`
          : "";
      return `${i + 1}. [${kind}]${replyHint} ${body}`;
    })
    .join("\n");

  const body = {
    model: "meta/llama-3.1-8b-instruct",
    messages: [
      {
        role: "system",
        content: `Agrupas mensajes de un chat de respuesta a emergencias en Colombia (donaciones, necesidades, personas desaparecidas, edificios). Cada grupo debe representar UN solo ítem autocontenido (una oferta, una necesidad, un reporte).

Reglas:
- Un mensaje de TEXTO que responde a una IMAGEN (indicado como "responde al mensaje #N") casi siempre pertenece a ese mismo grupo que la imagen.
- Un texto SIN respuesta explícita puede referirse a un mensaje anterior mediante expresiones como "esto", "eso", "el de la foto", "la de arriba", "lo que envié", "uno de estos" — usa el significado del texto y el orden de los mensajes para decidir si pertenece con una imagen anterior.
- Las IMÁGENES son casi siempre ítems separados entre sí, a menos que el texto las conecte explícitamente (por ejemplo "las dos fotos son de lo mismo").
- Si tienes dudas razonables, prefiere separar en grupos distintos antes que fusionar cosas que podrían ser diferentes.
- No inventes mensajes ni ignores ninguno: cada índice del 1 al ${events.length} debe aparecer en exactamente un grupo.

Responde ÚNICAMENTE con un objeto JSON, sin markdown ni explicación, con esta forma exacta:
{"groups": [[1], [2, 3]]}
donde cada array interno contiene los números (1-based) de los mensajes que pertenecen juntos.`,
      },
      { role: "user", content: numbered },
    ],
    temperature: 0,
    max_tokens: 512,
  };

  try {
    const res = await fetch(
      "https://integrate.api.nvidia.com/v1/chat/completions",
      {
        method: "POST",
        headers: {
          Authorization: `Bearer ${ENV.NVIDIA_API_KEY}`,
          "Content-Type": "application/json",
        },
        body: JSON.stringify(body),
      },
    );
    if (!res.ok) {
      console.error(`[bot] ⚠️ Grouping NIM call failed: ${res.status}`);
      return null;
    }
    const data = await res.json();
    const raw = data.choices?.[0]?.message?.content?.trim() || "";
    const jsonMatch = raw.match(/\{[\s\S]*\}/);
    if (!jsonMatch) {
      console.error(
        "[bot] ⚠️ Grouping: no JSON in NIM output:",
        raw.slice(0, 200),
      );
      return null;
    }
    const parsed = JSON.parse(jsonMatch[0]);
    if (!Array.isArray(parsed.groups)) {
      console.error("[bot] ⚠️ Grouping: malformed groups field:", parsed);
      return null;
    }

    // Validate: every index 1..N appears exactly once across all groups.
    // Any deviation (missing index, duplicate, out-of-range) is treated
    // as an untrustworthy grouping — fall back rather than silently drop
    // or double-count a message.
    const seen = new Set();
    const groups = [];
    for (const g of parsed.groups) {
      if (!Array.isArray(g) || !g.length) continue;
      const idxGroup = [];
      for (const n of g) {
        const idx = Number(n) - 1;
        if (!Number.isInteger(idx) || idx < 0 || idx >= events.length) {
          console.error(`[bot] ⚠️ Grouping: out-of-range index ${n}`);
          return null;
        }
        if (seen.has(idx)) {
          console.error(`[bot] ⚠️ Grouping: duplicate index ${n}`);
          return null;
        }
        seen.add(idx);
        idxGroup.push(idx);
      }
      if (idxGroup.length) groups.push(idxGroup);
    }
    if (seen.size !== events.length) {
      console.error(
        `[bot] ⚠️ Grouping: covered ${seen.size}/${events.length} indices, missing some`,
      );
      return null;
    }
    return groups;
  } catch (e) {
    console.error("[bot] ⚠️ Grouping NIM error:", e.message);
    return null;
  }
}

// ================================================================
//  NIM STRUCTURED EXTRACTION (SLOW PATH)
// ================================================================
async function structureWithNIM(rawText) {
  const res = await fetch(
    "https://integrate.api.nvidia.com/v1/chat/completions",
    {
      method: "POST",
      headers: {
        Authorization: `Bearer ${ENV.NVIDIA_API_KEY}`,
        "Content-Type": "application/json",
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
  "intent": "donate_offer" | "request_help" | "report_need" | "report_emergency" | "find_supplier" | "volunteer_recruitment" | "request_item" | "mental_health" | "fraud_warning" | "unknown",
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
- Extract quantities as numbers, not strings.`,
          },
          { role: "user", content: rawText },
        ],
        temperature: 0,
        max_tokens: 1024,
      }),
    },
  );

  if (!res.ok) {
    const errText = await res.text();
    throw new Error(
      `NIM API failed: ${res.status} ${errText.substring(0, 200)}`,
    );
  }
  const data = await res.json();
  const raw = data.choices?.[0]?.message?.content?.trim() || "";
  if (!raw) throw new Error("NIM returned empty content");

  const jsonMatch = raw.match(/\{[\s\S]*\}/);
  if (!jsonMatch) {
    console.error("[bot] ❌ NIM raw output (no JSON found):", raw);
    throw new Error("No JSON object found in NIM output");
  }
  let parsed;
  try {
    parsed = JSON.parse(jsonMatch[0]);
  } catch (e) {
    console.error(
      "[bot] ❌ NIM JSON parse error:",
      e.message,
      "| Raw snippet:",
      jsonMatch[0].substring(0, 150),
    );
    throw new Error("Malformed JSON in NIM output");
  }
  if (!parsed.intent || !Array.isArray(parsed.items)) {
    console.error("[bot] ❌ NIM schema validation failed:", parsed);
    throw new Error(
      "Malformed NIM output schema: missing intent or items array.",
    );
  }
  return parsed;
}

function nimEntitiesToFlat(structured) {
  const out = [];
  for (const i of structured.items || []) {
    if (i.name) out.push({ entity: "item", value: i.name });
    if (i.quantity != null)
      out.push({ entity: "quantity", value: String(i.quantity) });
    if (i.unit) out.push({ entity: "unit", value: i.unit });
  }
  if (
    structured.location &&
    structured.location !== "desconocido" &&
    structured.location !== "desconocida"
  ) {
    out.push({ entity: "location", value: structured.location });
  }
  if (structured.urgency)
    out.push({ entity: "urgency", value: structured.urgency });
  return out;
}

// ================================================================
//  PER-GROUP EXTRACTION + SAVE
//  This is the old processMessageBatch body, now scoped to a single
//  logical group (one item) instead of the whole raw buffer. Runs the
//  existing Rasa fast-path / NIM slow-path pipeline unchanged.
// ================================================================
async function processGroup(userId, groupEvents) {
  const textParts = [];
  for (const e of groupEvents) {
    if (e.contextNote) textParts.push(e.contextNote);
    if (e.text) textParts.push(e.text);
    if (e.ocrText) textParts.push(e.ocrText);
  }
  const combinedText = textParts.join("\n").trim();
  if (!combinedText || combinedText.length < 3) {
    console.log("[bot] ⏭️ Empty group text, skipping");
    return;
  }

  // Representative event id for this group: prefer an image, else the
  // first event in the group.
  const primaryEvent =
    groupEvents.find((e) => e.type === "image") || groupEvents[0];
  const primaryEventId = primaryEvent.eventId;

  console.log(
    `[bot] 📦 Group from ${userId} (${groupEvents.length} item(s)): "${combinedText.substring(0, 150)}..."`,
  );

  const paymentFlag = detectPaymentInfo(combinedText);

  let rasaRes;
  try {
    const res = await fetch(ENV.RASA_URL, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ text: combinedText }),
    });
    if (!res.ok) throw new Error(`Rasa HTTP ${res.status}`);
    rasaRes = await res.json();
  } catch (e) {
    console.error("[bot] ❌ Rasa unreachable:", e.message);
    return;
  }

  const intent = rasaRes?.intent;
  const entities = rasaRes.entities || [];
  if (!intent) {
    console.log("[bot] ⚠️ No intent from Rasa");
    return;
  }
  console.log(`[bot] 🧠 Rasa: ${intent.name} (${intent.confidence})`);

  if (ENV.BLACKLIST.includes(intent.name) && !paymentFlag) {
    console.log(`[bot] ⏭️ '${intent.name}' blacklisted`);
    return;
  }

  let finalIntent = intent.name;
  let finalEntities = entities.map((e) => ({
    entity: e.entity,
    value: e.value,
  }));
  let enrichedData = null;

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
    }
  } else if (intent.confidence > 0.9 && entities.length >= 2) {
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
    console.log(`[bot] 🚩 Payment info detected in group from ${userId}`);
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
        raw_text: combinedText,
        raw_json: JSON.stringify(
          { group_events: groupEvents, rasa_response: rasaRes },
          null,
          2,
        ),
        enriched_data: enrichedData ? JSON.stringify(enrichedData) : null,
        message_count: groupEvents.length,
      }),
    });
    if (res.status === 409)
      console.log(`[bot] ⏩ Duplicate: ${primaryEventId}`);
    else if (res.ok)
      console.log(
        `[bot] ✅ Backlog: ${primaryEventId} (${groupEvents.length} msg(s))`,
      );
    else {
      const errText = await res.text().catch(() => "No body");
      console.error(
        `[bot] ❌ Backlog ${res.status}: ${errText.substring(0, 200)}`,
      );
    }
  } catch (e) {
    console.error(`[bot] ❌ Backlog error:`, e.message);
    jobErrors++;
    if (jobErrors > MAX_JOB_ERRORS) {
      console.error("[bot] 🚨 Max errors. Exiting.");
      process.exit(1);
    }
  }
}

// ================================================================
//  BATCH PROCESSOR
//  Now a two-phase pipeline:
//    1. Build a flat `events` list for the whole window (resolving reply
//       parents, OCR'ing every image, persisting OCR to media_text as
//       before — independent of grouping/triage outcomes).
//    2. Ask groupMessagesWithNIM to segment `events` into logical items;
//       fall back to "each event = own item" on any failure.
//    3. Run the existing extraction pipeline (processGroup) per item.
// ================================================================
async function processMessageBatch(userId, messages) {
  if (userId === myUserId) return;

  // Resolve reply parents for EVERY reply in the window, not just short
  // ones — the grouping step needs the reply hint regardless of the
  // child's text length (the old `< 15` gate was tuned for the
  // now-separate context-note feature below, not for grouping).
  const parentMap = new Map();
  for (const msg of messages) {
    if (msg.replyTo && !parentMap.has(msg.replyTo)) {
      const parent = await resolveReplyContext(targetRoomId, msg.replyTo);
      if (parent) parentMap.set(msg.replyTo, parent);
    }
  }

  // Build the flat, ordered event list for this window. Each event keeps
  // enough metadata for both grouping (type, replyToIndex) and extraction
  // (text, ocrText, contextNote).
  const events = [];
  const eventIndexByMatrixId = new Map();
  const seenImageUrls = new Set();

  // Image/audio parents become real standalone events (inserted first if
  // not already part of this window) so grouping can link a reply to the
  // actual media it's referring to, and so that media gets OCR'd/logged.
  //
  // Text parents are handled differently: they're NOT added as standalone
  // events. Instead their body is attached to the replying child as a
  // labeled context note (restored below) — this mirrors the pre-grouping
  // behavior, where a reply to a text message always got the parent's
  // text injected regardless of any grouping outcome, and avoids the
  // parent's text appearing twice if grouping happens to also place it
  // in the same item as the child.
  const textParentBodyByChildReplyTo = new Map();
  for (const [parentEventId, parent] of parentMap) {
    if (parent.isText) {
      if (parent.content?.body) {
        textParentBodyByChildReplyTo.set(parentEventId, parent.content.body);
      }
      continue;
    }
    if (eventIndexByMatrixId.has(parentEventId)) continue;
    let type = "text";
    let imageUrl = null;
    if (parent.isImage) {
      type = "image";
      imageUrl = parent.content?.url || null;
    } else if (parent.isAudio) {
      type = "audio";
    }
    events.push({
      eventId: parentEventId,
      type,
      text: null,
      imageUrl,
      ocrText: null,
      replyToIndex: null,
      contextNote: null,
    });
    eventIndexByMatrixId.set(parentEventId, events.length - 1);
  }

  for (const msg of messages) {
    let type = "text";
    if (msg.imageUrl) type = "image";
    else if (msg.audioUrl) type = "audio";
    const replyToIndex =
      msg.replyTo && eventIndexByMatrixId.has(msg.replyTo)
        ? eventIndexByMatrixId.get(msg.replyTo)
        : null;
    const textParentBody = msg.replyTo
      ? textParentBodyByChildReplyTo.get(msg.replyTo)
      : null;
    const contextNote = textParentBody
      ? `[Contexto anterior: ${textParentBody}]`
      : null;
    events.push({
      eventId: msg.eventId,
      type,
      text: msg.text || null,
      imageUrl: msg.imageUrl || null,
      ocrText: null,
      replyToIndex,
      contextNote,
    });
    eventIndexByMatrixId.set(msg.eventId, events.length - 1);
  }

  // OCR every image event exactly once, persist immediately to media_text
  // (independent of what grouping/triage later decide — same as before).
  for (const ev of events) {
    if (ev.type !== "image" || !ev.imageUrl) continue;
    if (seenImageUrls.has(ev.imageUrl)) continue;
    seenImageUrls.add(ev.imageUrl);
    console.log(`[bot] 🔍 OCR image (${ev.eventId})...`);
    const ocrText = await extractTextFromImage(ev.imageUrl);
    if (ocrText) {
      ev.ocrText = ocrText;
      try {
        await safeApiCall(`${ENV.API_BASE}/api/media-text`, {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({
            source_event_id: ev.eventId,
            raw_text: ocrText,
          }),
        });
        console.log(`[bot] ✅ Persisted OCR for ${ev.eventId}`);
      } catch (e) {
        console.error(
          `[bot] ⚠️ Failed to persist OCR for ${ev.eventId}:`,
          e.message,
        );
      }
    }
  }

  // Audio: logged only for now (transcription TBD), same as before.
  for (const ev of events) {
    if (ev.type === "audio") console.log(`[bot] 🎤 Audio (${ev.eventId})`);
  }

  // Grouping input text: for images, use OCR text (the model needs to
  // "read" the image to decide what it's about); for text events, the
  // message body. A text event that replies to a TEXT parent has no
  // replyToIndex (that parent was never inserted as a standalone event —
  // see contextNote above), so without help the grouping model can't
  // tell it's a reply at all. Prepend the contextNote here so the model
  // still sees "[Contexto anterior: ...]" ahead of the child's own text
  // when deciding groups. This is grouping-input only — ev.text itself
  // stays untouched, so processGroup's own contextNote + text handling
  // (used for the actual extraction pass) is unaffected and doesn't
  // double up.
  const groupingInput = events.map((ev) => {
    const baseText = ev.type === "image" ? ev.ocrText : ev.text;
    const text = ev.contextNote
      ? [ev.contextNote, baseText].filter(Boolean).join(" ")
      : baseText;
    return {
      type: ev.type,
      text,
      replyToIndex: ev.replyToIndex,
    };
  });

  let groups = await groupMessagesWithNIM(groupingInput);
  if (!groups) {
    console.log(
      "[bot] ⚠️ Grouping failed/unavailable — falling back to one item per message",
    );
    groups = events.map((_, i) => [i]);
  } else {
    console.log(
      `[bot] 🧩 Grouped ${events.length} message(s) into ${groups.length} item(s)`,
    );
  }

  for (const idxGroup of groups) {
    const groupEvents = idxGroup.sort((a, b) => a - b).map((i) => events[i]);
    await processGroup(userId, groupEvents);
  }
}

// ================================================================
//  SYNC LOOP (Raw Fetch)
// ================================================================
async function doSync() {
  const params = new URLSearchParams({ timeout: "30000" });
  if (nextBatch) params.set("since", nextBatch);
  params.set(
    "filter",
    JSON.stringify({
      room: {
        rooms: [targetRoomId],
        timeline: { limit: 100, types: ["m.room.message"] },
      },
    }),
  );

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
      text = text.replace(/^>(?:>|\s).*?\n\n/, "").trim() || text;
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
    // Media now joins the window like any other message — grouping
    // decides what belongs together, not arrival order.
    buffer.add(msg);
    if (shouldFlushImmediately(msg, buffer)) {
      await buffer.flush();
    }
  }

  // Persist the sync token ONLY after events are safely handed to buffers.
  // Note this doesn't make the buffer window itself crash-safe (a message
  // can still sit in a 30s UserBuffer timer when the process dies) — we
  // decided that risk is acceptable for this project: those messages
  // aren't lost, they just don't get auto-triaged, since Matrix still
  // holds them and a human can act on them from the chat directly.
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
    `/_matrix/client/v3/directory/room/${encodeURIComponent(ENV.ROOM_ALIAS)}`,
  );
  targetRoomId = roomResolve.room_id;
  console.log(`[bot] Target room: ${targetRoomId}`);

  try {
    await matrixPost(
      `/_matrix/client/v3/rooms/${encodeURIComponent(targetRoomId)}/join`,
      {},
    );
    console.log("[bot] Joined room");
  } catch (e) {
    console.log("[bot] Room join:", e.message);
  }

  if (fs.existsSync(STORAGE_FILE)) {
    try {
      const saved = JSON.parse(fs.readFileSync(STORAGE_FILE, "utf8"));
      nextBatch = saved.next_batch;
      console.log("[bot] Resumed sync");
    } catch {}
  }

  console.log("[bot] 🚀 Operational.");
  setInterval(() => console.log("[bot] ❤️ Heartbeat"), 10 * 60 * 1000);

  process.on("SIGTERM", async () => {
    console.log("[bot] 🛑 SIGTERM. Draining buffers...");
    const flushes = Array.from(userBuffers.values()).map((b) => b.flush());
    await Promise.allSettled(flushes);
    process.exit(0);
  });

  while (true) {
    try {
      await doSync();
    } catch (e) {
      console.error("[bot] Sync error:", e.message);
      await new Promise((r) => setTimeout(r, 5000));
    }
  }
}

boot().catch((e) => {
  console.error("[bot] Fatal boot error:", e);
  process.exit(1);
});

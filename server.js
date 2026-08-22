// Tiny emergency backend: Express + SQLite
// Run with systemd. Reports of people, pets, buildings, and announcements.
import express from "express";
import cors from "cors";
import Database from "better-sqlite3";
import crypto from "crypto";
const app = express();
const PORT = process.env.PORT || 3000;
const MATRIX_HOMESERVER_URL = process.env.MATRIX_HOMESERVER_URL || "http://localhost:8008";
const MATRIX_SHARED_SECRET = process.env.MATRIX_SHARED_SECRET || "";
const MATRIX_DOMAIN = process.env.MATRIX_DOMAIN || "matrix.sismoinfo.co";
// Public base URL the browser uses to reach Matrix (reverse proxy).
const MATRIX_PUBLIC_URL = process.env.MATRIX_PUBLIC_URL || `https://${MATRIX_DOMAIN}`;
const DB_PATH = process.env.DB_PATH || "/opt/sismo-api/data.db";
app.use(cors());
app.use(express.json());

// Admin auth middleware
function requireAdmin(req, res, next) {
  if (!process.env.ADMIN_KEY || req.headers["x-admin-key"] !== process.env.ADMIN_KEY) {
    return res.status(403).json({ error: "forbidden" });
  }
  next();
}

// ========== Auth (zero deps: Node built-in crypto) ==========
const AUTH_SECRET = process.env.AUTH_SECRET || crypto.randomBytes(32).toString("hex");
const SCRYPT_PARAMS = { N: 16384, r: 8, p: 1 };
const SCRYPT_KEYLEN = 32;
const TOKEN_TTL_MS = 15 * 24 * 60 * 60 * 1000; // 15 days

function hashPassword(password) {
  const salt = crypto.randomBytes(16);
  const hash = crypto.scryptSync(password, salt, SCRYPT_KEYLEN, SCRYPT_PARAMS);
  return `scrypt$${salt.toString("hex")}$${hash.toString("hex")}`;
}

function verifyPassword(password, stored) {
  if (!stored || typeof stored !== "string") return false;
  const parts = stored.split("$");
  if (parts.length !== 3 || parts[0] !== "scrypt") return false;
  const salt = Buffer.from(parts[1], "hex");
  const hash = Buffer.from(parts[2], "hex");
  const test = crypto.scryptSync(password, salt, SCRYPT_KEYLEN, SCRYPT_PARAMS);
  return crypto.timingSafeEqual(hash, test);
}

function createToken(userId) {
  const ts = Date.now();
  const payload = `${userId}.${ts}`;
  const sig = crypto.createHmac("sha256", AUTH_SECRET).update(payload).digest("hex");
  return `${payload}.${sig}`;
}

function verifyToken(token) {
  if (!token || typeof token !== "string") return null;
  const parts = token.split(".");
  if (parts.length !== 3) return null;
  const [userId, tsStr, sig] = parts;
  const ts = parseInt(tsStr, 10);
  if (!Number.isFinite(ts)) return null;
  if (Date.now() - ts > TOKEN_TTL_MS) return null; // expired
  const expected = crypto.createHmac("sha256", AUTH_SECRET).update(`${userId}.${ts}`).digest("hex");
  if (sig.length !== expected.length) return null;
  if (!crypto.timingSafeEqual(Buffer.from(sig, "hex"), Buffer.from(expected, "hex"))) return null;
  return userId;
}

function requireAuth(req, res, next) {
  const token = (req.headers.authorization || "").replace(/^Bearer\s+/i, "");
  const userId = verifyToken(token);
  if (!userId) return res.status(401).json({ error: "unauthorized" });
  req.userId = userId;
  next();
}
// Resolve how a collaborator appears in chat: display_name → name → contact (WhatsApp) → "Anónimo"
function resolveDisplayName(user) {
  if (!user) return "Anónimo";
  return (
    (user.display_name && user.display_name.trim()) ||
    (user.name && user.name.trim()) ||
    (user.contact && user.contact.trim()) ||
    "Anónimo"
  );
}
// Map a Matrix user id (@localpart:domain) back to a collaborator display name.
// The localpart is the collaborator id (see provisionMatrixAccount).
function resolveMatrixDisplayName(matrixId) {
  if (!matrixId || typeof matrixId !== "string") return "Anónimo";
  const localpart = matrixId.replace(/^@/, "").split(":")[0];
  const user = db.prepare("SELECT id, name, display_name, contact FROM collaborators WHERE id = ?").get(localpart);
  return resolveDisplayName(user);
}

// ========== Matrix (Dendrite) account provisioning ==========
// Dendrite supports the Synapse-compatible shared-secret registration endpoint.
// We create the Matrix account server-side (username + password) so the user
// never needs a second password. The client then logs in with matrix-js-sdk
// and generates its own E2E device keys in the browser.
function matrixMac(nonce, username, password, admin = false) {
  const parts = [nonce, username, password, admin ? "admin" : "notadmin"];
  return crypto
    .createHmac("sha1", MATRIX_SHARED_SECRET)
    .update(parts.join("\0"))
    .digest("hex");
}

async function provisionMatrixAccount(userId, displayName) {
  if (!MATRIX_SHARED_SECRET) {
    throw new Error("MATRIX_SHARED_SECRET not configured");
  }
  // Username is the collaborator id (stable, unique, no email/phone ambiguity).
  const username = userId;
  const password = crypto.randomBytes(24).toString("base64url");

  // 1. Fetch a one-time nonce.
  const nonceRes = await fetch(`${MATRIX_HOMESERVER_URL}/_synapse/admin/v1/register`);
  if (!nonceRes.ok) throw new Error(`nonce fetch failed: ${nonceRes.status}`);
  const { nonce } = await nonceRes.json();

  // 2. Register the account with the HMAC-SHA1 MAC.
  const mac = matrixMac(nonce, username, password, false);
  const regRes = await fetch(`${MATRIX_HOMESERVER_URL}/_synapse/admin/v1/register`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({
      nonce,
      username,
      password,
      displayname: displayName,
      admin: false,
      mac,
    }),
  });
  if (!regRes.ok) {
    const body = await regRes.text().catch(() => "");
    throw new Error(`register failed: ${regRes.status} ${body}`);
  }
  const data = await regRes.json();
  return {
    user_id: data.user_id || `@${username}:${MATRIX_DOMAIN}`,
    password,
  };
}

// Helper for PATCH /:id/photo endpoints
function handlePhotoPatch(tableName) {
  return (req, res) => {
    const { photo_url } = req.body || {};
    if (
      !photo_url ||
      typeof photo_url !== "string" ||
      !/^https:\/\/res\.cloudinary\.com\//.test(photo_url)
    ) {
      return res.status(400).json({ error: "invalid photo_url" });
    }
    const result = db
      .prepare(`UPDATE ${tableName} SET photo_url = ? WHERE id = ?`)
      .run(photo_url, req.params.id);
    if (result.changes === 0) return res.status(404).json({ error: "not found" });
    res.json({ id: req.params.id, photo_url });
  };
}
const db = new Database(DB_PATH);
db.pragma("journal_mode = WAL");
// --- Create tables ---
db.exec(`
  CREATE TABLE IF NOT EXISTS buildings (
    id TEXT PRIMARY KEY,
    name TEXT NOT NULL,
    status TEXT NOT NULL DEFAULT 'seguro',
    location TEXT,
    city TEXT,
    image TEXT,
    created_at TEXT NOT NULL
  )
`);
db.exec(`
  CREATE TABLE IF NOT EXISTS disappeared (
    id TEXT PRIMARY KEY,
    name TEXT NOT NULL,
    status TEXT NOT NULL DEFAULT 'desaparecido',
    location TEXT,
    city TEXT,
    image TEXT,
    created_at TEXT NOT NULL
  )
`);
db.exec(`
  CREATE TABLE IF NOT EXISTS pets (
    id TEXT PRIMARY KEY,
    name TEXT NOT NULL,
    status TEXT NOT NULL DEFAULT 'desaparecido',
    location TEXT,
    city TEXT,
    image TEXT,
    created_at TEXT NOT NULL
  )
`);
db.exec(`
  CREATE TABLE IF NOT EXISTS collaborators (
    id TEXT PRIMARY KEY,
    name TEXT NOT NULL,
    skill TEXT,
    contact TEXT,
    city TEXT,
    created_at TEXT NOT NULL
  )
`);
db.exec(`
  CREATE TABLE IF NOT EXISTS anuncios (
    id TEXT PRIMARY KEY,
    title TEXT,
    text TEXT NOT NULL,
    image TEXT,
    photo_url TEXT,
    images TEXT,
    created_at TEXT NOT NULL
  )
`);
db.exec(`
  CREATE TABLE IF NOT EXISTS comments (
    id TEXT PRIMARY KEY,
    item_id TEXT NOT NULL,
    item_type TEXT NOT NULL,  -- 'disappeared', 'pets', 'buildings'
    text TEXT NOT NULL,
    created_at TEXT NOT NULL
  )
`);
// Ubicaciones adicionales posibles (además de lat/lng principal) para
// personas y mascotas: un item puede haber sido visto en más de un lugar.
db.exec(`
  CREATE TABLE IF NOT EXISTS item_locations (
    id TEXT PRIMARY KEY,
    item_id TEXT NOT NULL,
    item_type TEXT NOT NULL,  -- 'disappeared' o 'pets'
    lat REAL NOT NULL,
    lng REAL NOT NULL,
    label TEXT,
    created_at TEXT NOT NULL
  )
`);
// --- Tablas privadas de la pestaña Colaboradores (requieren login) ---
// Donaciones ofrecidas por colaboradores (comida, ropa, insumos, etc.)
db.exec(`
  CREATE TABLE IF NOT EXISTS donaciones (
    id TEXT PRIMARY KEY,
    item_type TEXT NOT NULL,      -- 'comida' | 'ropa' | 'insumos' | 'medicinas' | 'transporte' | 'otro'
    quantity TEXT,
    description TEXT NOT NULL,
    status TEXT DEFAULT 'disponible',  -- 'disponible' | 'reservado' | 'entregado' | 'vencido'
    location TEXT,
    lat REAL,
    lng REAL,
    contact TEXT,
    donor_id TEXT,                -- FK a collaborators (quién la ofrece)
    created_at TEXT NOT NULL
  )
`);
// Necesidades reportadas por punto de rescate
db.exec(`
  CREATE TABLE IF NOT EXISTS necesidades (
    id TEXT PRIMARY KEY,
    item_type TEXT NOT NULL,      -- 'comida' | 'ropa' | 'carpas' | 'medicinas' | 'aseo' | 'otro'
    quantity TEXT,
    description TEXT NOT NULL,
    urgency TEXT DEFAULT 'media', -- 'alta' | 'media' | 'baja'
    status TEXT DEFAULT 'abierta',-- 'abierta' | 'en_proceso' | 'cubierta'
    point_name TEXT,              -- 'Cantabria', 'Cámbulos', 'Guadalupe', etc.
    location TEXT,
    lat REAL,
    lng REAL,
    contact TEXT,
    reporter_id TEXT,             -- FK a collaborators (quién la reporta)
    created_at TEXT NOT NULL
  )
`);
// Tareas de logística: entregas, recogidas, transporte
db.exec(`
  CREATE TABLE IF NOT EXISTS logistica (
    id TEXT PRIMARY KEY,
    task_type TEXT NOT NULL,      -- 'entrega' | 'recogida' | 'transporte' | 'apoyo'
    item_ref TEXT,                -- referencia a donación/necesidad (opcional)
    description TEXT NOT NULL,
    status TEXT DEFAULT 'pendiente', -- 'pendiente' | 'en_ruta' | 'completado' | 'cancelado'
    origin TEXT,
    destination TEXT,
    lat REAL,
    lng REAL,
    contact TEXT,
    assignee_id TEXT,             -- FK a collaborators (quién lo ejecuta)
    creator_id TEXT,              -- FK a collaborators (quién lo crea)
    created_at TEXT NOT NULL
  )
`);
// ========== Backlog (AI Triage) ==========
db.exec(`
  CREATE TABLE IF NOT EXISTS backlog (
    id TEXT PRIMARY KEY,
    source_event_id TEXT UNIQUE,
    creator_matrix_id TEXT,
    intent TEXT NOT NULL,
    extracted_data TEXT,
    raw_text TEXT,
    raw_json TEXT,
    enriched_data TEXT,
    message_count INTEGER DEFAULT 1,
    status TEXT DEFAULT 'pending',
    created_at TEXT NOT NULL,
    audio_url TEXT
  )
`);
// ========== Media Text (Standalone OCR Storage) ==========
// Decoupled from the backlog/triage pipeline: every image's extracted text
// is persisted here keyed by its own Matrix event id, so the Lightbox can
// show OCR text even when the message never triggered a triage item.
db.exec(`
  CREATE TABLE IF NOT EXISTS media_text (
    source_event_id TEXT PRIMARY KEY,
    raw_text TEXT,
    created_at TEXT NOT NULL,
    audio_url TEXT
  )
`);
// Migrations for existing backlog tables (raw message + raw payload for debugging)
try { db.exec("ALTER TABLE backlog ADD COLUMN raw_text TEXT"); } catch (e) {}
try { db.exec("ALTER TABLE backlog ADD COLUMN raw_json TEXT"); } catch (e) {}
try { db.exec("ALTER TABLE backlog ADD COLUMN enriched_data TEXT"); } catch (e) {}
try { db.exec("ALTER TABLE backlog ADD COLUMN message_count INTEGER DEFAULT 1"); } catch (e) {}
try { db.exec("ALTER TABLE backlog ADD COLUMN audio_url TEXT"); } catch (e) {}
try { db.exec("ALTER TABLE media_text ADD COLUMN audio_url TEXT"); } catch (e) {}
// --- Migrations for existing columns (city and image) ---
try {
  db.exec("ALTER TABLE disappeared ADD COLUMN city TEXT");
} catch (e) { }
try {
  db.exec("ALTER TABLE buildings ADD COLUMN city TEXT");
} catch (e) { }
try {
  db.exec("ALTER TABLE pets ADD COLUMN city TEXT");
} catch (e) { }
try {
  db.exec("ALTER TABLE disappeared ADD COLUMN image TEXT");
} catch (e) { }
try {
  db.exec("ALTER TABLE buildings ADD COLUMN image TEXT");
} catch (e) { }
try {
  db.exec("ALTER TABLE pets ADD COLUMN image TEXT");
} catch (e) { }
// --- Migration: add items JSON column to donaciones and necesidades ---
try {
  db.exec("ALTER TABLE donaciones ADD COLUMN items TEXT");
} catch (e) { }
try {
  db.exec("ALTER TABLE necesidades ADD COLUMN items TEXT");
} catch (e) { }
try {
  db.exec("ALTER TABLE disappeared ADD COLUMN photo_url TEXT");
} catch (e) { }
// Add photo_url to pets (for file uploads)
try {
  db.exec("ALTER TABLE pets ADD COLUMN photo_url TEXT");
} catch (e) { }
// Add photo_url to buildings (for file uploads)
try {
  db.exec("ALTER TABLE buildings ADD COLUMN photo_url TEXT");
} catch (e) { }
// Add private flag to buildings (only logged-in collaborators can see them)
try {
  db.exec("ALTER TABLE buildings ADD COLUMN private INTEGER DEFAULT 0");
} catch (e) { }
// --- lat/lng for map picker (separate from free-text location) ---
for (const t of ["disappeared", "pets", "buildings"]) {
  try {
    db.exec(`ALTER TABLE ${t} ADD COLUMN lat REAL`);
  } catch (e) { }
  try {
    db.exec(`ALTER TABLE ${t} ADD COLUMN lng REAL`);
  } catch (e) { }
}

try {
  db.exec("ALTER TABLE disappeared ADD COLUMN source_url TEXT");
  db.exec("ALTER TABLE disappeared ADD COLUMN age INTEGER");
  db.exec("ALTER TABLE disappeared ADD COLUMN physical_description TEXT");
  db.exec("ALTER TABLE disappeared ADD COLUMN department TEXT");
  db.exec("ALTER TABLE disappeared ADD COLUMN category TEXT");
  db.exec("ALTER TABLE disappeared ADD COLUMN report_count INTEGER");
  db.exec("ALTER TABLE disappeared ADD COLUMN time_elapsed TEXT");
  db.exec("ALTER TABLE disappeared ADD COLUMN source_type TEXT");
  db.exec("ALTER TABLE disappeared ADD COLUMN reporter TEXT");
} catch (e) { }

// Add new deep-scrape columns to pets table
const newPetCols = [
  "description",
  "breed",
  "color",
  "sex",
  "size",
  "contact_name",
  "contact_phone",
  "contact_email",
  "contact_whatsapp",
  "meta",
  "source_url",
];
for (const col of newPetCols) {
  try {
    db.exec(`ALTER TABLE pets ADD COLUMN ${col} TEXT`);
  } catch (e) { }
}
// Add meta column for tags (e.g., "angel")
try {
  db.exec("ALTER TABLE disappeared ADD COLUMN meta TEXT");
} catch (e) { }
try {
  db.exec("ALTER TABLE disappeared ADD COLUMN description TEXT");
} catch (e) { }
// Add image/photo_url to anuncios (for image support)
try {
  db.exec("ALTER TABLE anuncios ADD COLUMN image TEXT");
} catch (e) { }
try {
  db.exec("ALTER TABLE anuncios ADD COLUMN photo_url TEXT");
} catch (e) { }
try {
  db.exec("ALTER TABLE anuncios ADD COLUMN images TEXT");
} catch (e) { }
try {
  db.exec("ALTER TABLE anuncios ADD COLUMN title TEXT");
} catch (e) { }
// Auth columns for collaborators (email + password hash)
try {
  db.exec("ALTER TABLE collaborators ADD COLUMN email TEXT");
} catch (e) { }
try {
  db.exec("ALTER TABLE collaborators ADD COLUMN password_hash TEXT");
} catch (e) { }
try {
  db.exec("ALTER TABLE collaborators ADD COLUMN display_name TEXT");
} catch (e) { }
try {
  db.exec("ALTER TABLE collaborators ADD COLUMN matrix_user_id TEXT");
} catch (e) { }
try {
  db.exec("ALTER TABLE collaborators ADD COLUMN matrix_password TEXT");
} catch (e) { }
// ========== Comentarios ==========
app.get("/api/comments", (req, res) => {
  const { itemId, itemType } = req.query;
  if (!itemId || !itemType) {
    return res.status(400).json({ error: "itemId and itemType are required" });
  }
  const rows = db
    .prepare(
      "SELECT id, text, created_at FROM comments WHERE item_id = ? AND item_type = ? ORDER BY created_at DESC",
    )
    .all(itemId, itemType);
  res.json(rows);
});
app.post("/api/comments", (req, res) => {
  const { itemId, itemType, text } = req.body || {};
  if (!itemId || !itemType || !text || !text.trim()) {
    return res
      .status(400)
      .json({ error: "itemId, itemType and text are required" });
  }
  const finalId = crypto.randomUUID();
  const createdAt = new Date().toISOString();
  db.prepare(
    "INSERT INTO comments (id, item_id, item_type, text, created_at) VALUES (?, ?, ?, ?, ?)",
  ).run(finalId, itemId, itemType, text.trim(), createdAt);
  res
    .status(201)
    .json({ id: finalId, text: text.trim(), created_at: createdAt });
});
app.delete("/api/comments/:id", requireAdmin, (req, res) => {
  const result = db
    .prepare("DELETE FROM comments WHERE id = ?")
    .run(req.params.id);
  if (result.changes === 0) return res.status(404).json({ error: "not found" });
  res.status(204).end();
});
// ========== Ubicaciones adicionales (personas y mascotas) ==========
app.get("/api/item-locations", (req, res) => {
  const { itemId, itemType } = req.query;
  if (!itemId || !itemType) {
    return res.status(400).json({ error: "itemId and itemType are required" });
  }
  if (!["disappeared", "pets"].includes(itemType)) {
    return res.status(400).json({ error: "invalid itemType" });
  }
  const rows = db
    .prepare(
      "SELECT id, lat, lng, label, created_at FROM item_locations WHERE item_id = ? AND item_type = ? ORDER BY created_at ASC",
    )
    .all(itemId, itemType);
  res.json(rows);
});
app.post("/api/item-locations", (req, res) => {
  const { itemId, itemType, lat, lng, label } = req.body || {};
  if (!itemId || !itemType || !["disappeared", "pets"].includes(itemType)) {
    return res
      .status(400)
      .json({ error: "itemId and a valid itemType are required" });
  }
  if (!Number.isFinite(lat) || !Number.isFinite(lng)) {
    return res.status(400).json({ error: "lat and lng are required" });
  }
  const finalId = crypto.randomUUID();
  const createdAt = new Date().toISOString();
  const lbl = label && label.trim() ? label.trim() : null;
  db.prepare(
    "INSERT INTO item_locations (id, item_id, item_type, lat, lng, label, created_at) VALUES (?, ?, ?, ?, ?, ?, ?)",
  ).run(finalId, itemId, itemType, lat, lng, lbl, createdAt);
  res
    .status(201)
    .json({ id: finalId, lat, lng, label: lbl, created_at: createdAt });
});
app.delete("/api/item-locations/:id", requireAdmin, (req, res) => {
  const result = db
    .prepare("DELETE FROM item_locations WHERE id = ?")
    .run(req.params.id);
  if (result.changes === 0) return res.status(404).json({ error: "not found" });
  res.status(204).end();
});
// ========== Personas ==========
app.get("/api/disappeared", (req, res) => {
  const rows = db
    .prepare(
      `
      SELECT id, name, status, location, city, image, photo_url, created_at, lat, lng, meta, description,
             (SELECT text FROM comments WHERE item_id = disappeared.id AND item_type = 'disappeared' ORDER BY created_at DESC LIMIT 1) AS last_comment,
             (SELECT COUNT(*) FROM comments WHERE item_id = disappeared.id AND item_type = 'disappeared') AS comment_count
      FROM disappeared ORDER BY RANDOM()
    `,
    )
    .all();
  res.json(rows);
});
// The photo itself is uploaded straight from the browser to Cloudinary
// (unsigned preset); this just records the resulting URL against the report.
app.patch("/api/disappeared/:id/photo", handlePhotoPatch("disappeared"));
app.post("/api/disappeared", (req, res) => {
  const {
    id,
    name,
    status,
    location,
    city,
    image,
    lat,
    lng,
    meta,
    description,
    source_url,
    age,
    physical_description,
    department,
    category,
    report_count,
    time_elapsed,
    source_type,
    reporter,
  } = req.body || {};

  if (!name || !name.trim()) {
    return res.status(400).json({ error: "name is required" });
  }

  // 🛡️ Prevent duplicates: If we already scraped this exact URL, skip it!
  if (source_url) {
    const existing = db
      .prepare("SELECT id FROM disappeared WHERE source_url = ?")
      .get(source_url);
    if (existing) {
      return res
        .status(200)
        .json({ id: existing.id, status: "already_exists" });
    }
  }

  const finalId = id || crypto.randomUUID();
  const createdAt = new Date().toISOString();
  const loc = location && location.trim() ? location.trim() : null;
  const cty = city && city.trim() ? city.trim() : null;
  const img = image && image.trim() ? image.trim() : null;
  const latVal = Number.isFinite(lat) ? lat : null;
  const lngVal = Number.isFinite(lng) ? lng : null;
  const metaVal = meta && typeof meta === "string" ? meta.trim() : null;
  const descVal =
    description && typeof description === "string" ? description.trim() : null;
  const ageVal = Number.isFinite(age) ? age : null;
  const physDescVal =
    physical_description && typeof physical_description === "string"
      ? physical_description.trim()
      : null;
  const deptVal = department && typeof department === "string" ? department.trim() : null;
  const catVal = category && typeof category === "string" ? category.trim() : null;
  const reportCountVal = Number.isFinite(report_count) ? report_count : null;
  const timeElapsedVal = time_elapsed && typeof time_elapsed === "string" ? time_elapsed.trim() : null;
  const sourceTypeVal = source_type && typeof source_type === "string" ? source_type.trim() : null;
  const reporterVal = reporter && typeof reporter === "string" ? reporter.trim() : null;
  const sourceUrlVal = source_url && typeof source_url === "string" ? source_url.trim() : null;

  // Map and validate status
  const validStatuses = ["desaparecido", "encontrado", "angel"];
  const finalStatus = validStatuses.includes(status) ? status : "desaparecido";

  db.prepare(
    `INSERT INTO disappeared (
      id, name, status, location, city, image, created_at, lat, lng, meta, description,
      source_url, age, physical_description, department, category, report_count,
      time_elapsed, source_type, reporter
    ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)`
  ).run(
    finalId,
    name.trim(),
    finalStatus,
    loc,
    cty,
    img,
    createdAt,
    latVal,
    lngVal,
    metaVal,
    descVal,
    sourceUrlVal,
    ageVal,
    physDescVal,
    deptVal,
    catVal,
    reportCountVal,
    timeElapsedVal,
    sourceTypeVal,
    reporterVal,
  );

  res.status(201).json({
    id: finalId,
    name: name.trim(),
    status: finalStatus,
    location: loc,
    city: cty,
    image: img,
    created_at: createdAt,
    lat: latVal,
    lng: lngVal,
    meta: metaVal,
    description: descVal,
    source_url: sourceUrlVal,
    age: ageVal,
    physical_description: physDescVal,
    department: deptVal,
    category: catVal,
    report_count: reportCountVal,
    time_elapsed: timeElapsedVal,
    source_type: sourceTypeVal,
    reporter: reporterVal,
  });
});
app.patch("/api/disappeared/:id", (req, res) => {
  const { status, image, lat, lng } = req.body || {};
  if (status && !["desaparecido", "encontrado", "angel"].includes(status)) {
    return res.status(400).json({ error: "invalid status" });
  }
  const fields = [];
  const values = [];
  if (status !== undefined) {
    fields.push("status = ?");
    values.push(status);
  }
  if (image !== undefined) {
    fields.push("image = ?");
    values.push(image && image.trim() ? image.trim() : null);
  }
  if (lat !== undefined) {
    fields.push("lat = ?");
    values.push(Number.isFinite(lat) ? lat : null);
  }
  if (lng !== undefined) {
    fields.push("lng = ?");
    values.push(Number.isFinite(lng) ? lng : null);
  }
  if (fields.length === 0) {
    return res.status(400).json({ error: "no fields to update" });
  }
  values.push(req.params.id);
  const result = db
    .prepare(`UPDATE disappeared SET ${fields.join(", ")} WHERE id = ?`)
    .run(...values);
  if (result.changes === 0) return res.status(404).json({ error: "not found" });
  res.json({ id: req.params.id, status, image, lat, lng });
});
app.delete("/api/disappeared/:id", requireAdmin, (req, res) => {
  const result = db
    .prepare("DELETE FROM disappeared WHERE id = ?")
    .run(req.params.id);
  if (result.changes === 0) return res.status(404).json({ error: "not found" });
  res.status(204).end();
});
// ========== Mascotas (Pets) ==========
app.get("/api/pets", (req, res) => {
  const rows = db
    .prepare(
      `
      SELECT id, name, status, location, city, image, photo_url, created_at, lat, lng,
             description, breed, color, sex, size, 
             contact_name, contact_phone, contact_email, contact_whatsapp, 
             meta, source_url,
             (SELECT text FROM comments WHERE item_id = pets.id AND item_type = 'pets' ORDER BY created_at DESC LIMIT 1) AS last_comment,
             (SELECT COUNT(*) FROM comments WHERE item_id = pets.id AND item_type = 'pets') AS comment_count
      FROM pets ORDER BY RANDOM()
    `,
    )
    .all();
  res.json(rows);
});
// Endpoint para actualizar photo_url de mascotas (similar a personas)
app.patch("/api/pets/:id/photo", handlePhotoPatch("pets"));
app.post("/api/pets", (req, res) => {
  const {
    id,
    name,
    location,
    city,
    image,
    lat,
    lng,
    status,
    description,
    breed,
    color,
    sex,
    size,
    contact_name,
    contact_phone,
    contact_email,
    contact_whatsapp,
    meta,
    source_url,
  } = req.body || {};
  if (!name || !name.trim()) {
    return res.status(400).json({ error: "name is required" });
  }
  // 🛡️ Prevent duplicates: If we already scraped this exact URL, skip it!
  if (source_url) {
    const existing = db
      .prepare("SELECT id FROM pets WHERE source_url = ?")
      .get(source_url);
    if (existing) {
      return res
        .status(200)
        .json({ id: existing.id, status: "already_exists" });
    }
  }
  const finalId = id || crypto.randomUUID();
  const createdAt = new Date().toISOString();
  // Map and validate status
  const validStatuses = ["desaparecido", "encontrado"];
  const finalStatus = validStatuses.includes(status) ? status : "desaparecido";
  db.prepare(
    `
    INSERT INTO pets (
      id, name, status, location, city, image, created_at, lat, lng,
      description, breed, color, sex, size, contact_name, contact_phone, 
      contact_email, contact_whatsapp, meta, source_url
    ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
  `,
  ).run(
    finalId,
    name.trim(),
    finalStatus,
    location?.trim() || null,
    city?.trim() || null,
    image?.trim() || null,
    createdAt,
    Number.isFinite(lat) ? lat : null,
    Number.isFinite(lng) ? lng : null,
    description?.trim() || null,
    breed?.trim() || null,
    color?.trim() || null,
    sex?.trim() || null,
    size?.trim() || null,
    contact_name?.trim() || null,
    contact_phone?.trim() || null,
    contact_email?.trim() || null,
    contact_whatsapp?.trim() || null,
    meta?.trim() || null,
    source_url?.trim() || null,
  );
  res.status(201).json({ id: finalId, name: name.trim(), status: finalStatus });
});
app.patch("/api/pets/:id", (req, res) => {
  const { status, image, lat, lng } = req.body || {};
  if (status && !["desaparecido", "encontrado", "angel"].includes(status)) {
    return res.status(400).json({ error: "invalid status" });
  }
  const fields = [];
  const values = [];
  if (status !== undefined) {
    fields.push("status = ?");
    values.push(status);
  }
  if (image !== undefined) {
    fields.push("image = ?");
    values.push(image && image.trim() ? image.trim() : null);
  }
  if (lat !== undefined) {
    fields.push("lat = ?");
    values.push(Number.isFinite(lat) ? lat : null);
  }
  if (lng !== undefined) {
    fields.push("lng = ?");
    values.push(Number.isFinite(lng) ? lng : null);
  }
  if (fields.length === 0) {
    return res.status(400).json({ error: "no fields to update" });
  }
  values.push(req.params.id);
  const result = db
    .prepare(`UPDATE pets SET ${fields.join(", ")} WHERE id = ?`)
    .run(...values);
  if (result.changes === 0) return res.status(404).json({ error: "not found" });
  res.json({ id: req.params.id, status, image, lat, lng });
});
app.delete("/api/pets/:id", requireAdmin, (req, res) => {
  const result = db.prepare("DELETE FROM pets WHERE id = ?").run(req.params.id);
  if (result.changes === 0) return res.status(404).json({ error: "not found" });
  res.status(204).end();
});
// ========== Edificios ==========
app.get("/api/buildings", (req, res) => {
  // Private buildings are only visible to logged-in collaborators.
  const token = (req.headers.authorization || "").replace(/^Bearer\s+/i, "");
  const userId = verifyToken(token);
  const rows = db
    .prepare(
      `
      SELECT id, name, status, location, city, image, photo_url, created_at, lat, lng, private,
        (SELECT text FROM comments WHERE item_id = buildings.id AND item_type = 'buildings' ORDER BY created_at DESC LIMIT 1) AS last_comment,
        (SELECT COUNT(*) FROM comments WHERE item_id = buildings.id AND item_type = 'buildings') AS comment_count
      FROM buildings
      ${userId ? "" : "WHERE private = 0"}
      ORDER BY RANDOM()
    `,
    )
    .all();
  res.json(rows);
});
app.post("/api/buildings", (req, res) => {
  const { id, name, location, city, image, lat, lng, private: isPrivate } = req.body || {};
  if (!name || !name.trim()) {
    return res.status(400).json({ error: "name is required" });
  }
  const finalId = id || crypto.randomUUID();
  const createdAt = new Date().toISOString();
  const loc = location && location.trim() ? location.trim() : null;
  const cty = city && city.trim() ? city.trim() : null;
  const img = image && image.trim() ? image.trim() : null;
  const latVal = Number.isFinite(lat) ? lat : null;
  const lngVal = Number.isFinite(lng) ? lng : null;
  const privateVal = isPrivate ? 1 : 0;
  db.prepare(
    "INSERT INTO buildings (id, name, status, location, city, image, created_at, lat, lng, private) VALUES (?, ?, 'seguro', ?, ?, ?, ?, ?, ?, ?)",
  ).run(finalId, name.trim(), loc, cty, img, createdAt, latVal, lngVal, privateVal);
  res.status(201).json({
    id: finalId,
    name: name.trim(),
    status: "seguro",
    location: loc,
    city: cty,
    image: img,
    created_at: createdAt,
    lat: latVal,
    lng: lngVal,
    private: privateVal,
  });
});
// Endpoint para actualizar photo_url de edificios (similar a personas y mascotas)
app.patch("/api/buildings/:id/photo", handlePhotoPatch("buildings"));

app.patch("/api/buildings/:id", (req, res) => {
  const { status, image, lat, lng, private: isPrivate } = req.body || {};
  if (status && !["seguro", "danado", "colapsado", "acopio"].includes(status)) {
    return res.status(400).json({ error: "invalid status" });
  }
  const fields = [];
  const values = [];
  if (status !== undefined) {
    fields.push("status = ?");
    values.push(status);
  }
  if (image !== undefined) {
    fields.push("image = ?");
    values.push(image && image.trim() ? image.trim() : null);
  }
  if (lat !== undefined) {
    fields.push("lat = ?");
    values.push(Number.isFinite(lat) ? lat : null);
  }
  if (lng !== undefined) {
    fields.push("lng = ?");
    values.push(Number.isFinite(lng) ? lng : null);
  }
  if (isPrivate !== undefined) {
    fields.push("private = ?");
    values.push(isPrivate ? 1 : 0);
  }
  if (fields.length === 0) {
    return res.status(400).json({ error: "no fields to update" });
  }
  values.push(req.params.id);
  const result = db
    .prepare(`UPDATE buildings SET ${fields.join(", ")} WHERE id = ?`)
    .run(...values);
  if (result.changes === 0) return res.status(404).json({ error: "not found" });
  res.json({ id: req.params.id, status, image, lat, lng, private: isPrivate });
});
app.delete("/api/buildings/:id", requireAdmin, (req, res) => {
  const result = db
    .prepare("DELETE FROM buildings WHERE id = ?")
    .run(req.params.id);
  if (result.changes === 0) return res.status(404).json({ error: "not found" });
  res.status(204).end();
});
// ========== Colaboradores (voluntarios) ==========
app.get("/api/collaborators", (req, res) => {
  const rows = db
    .prepare(
      "SELECT id, name, skill, contact, city, created_at FROM collaborators ORDER BY created_at DESC",
    )
    .all();
  res.json(rows);
});
// Colaboradores con cuenta Matrix (para invitar al room privado).
// Requiere login: solo miembros del chat pueden invitar a otros.
app.get("/api/collaborators/matrix", requireAuth, (req, res) => {
  const rows = db
    .prepare(
      "SELECT id, name, matrix_user_id FROM collaborators WHERE matrix_user_id IS NOT NULL ORDER BY name ASC",
    )
    .all();
  res.json(rows);
});
app.post("/api/collaborators", (req, res) => {
  const { name, skill, contact, city, email, password } = req.body || {};
  if (!name || !name.trim()) {
    return res.status(400).json({ error: "name is required" });
  }
  const finalId = crypto.randomUUID();
  const createdAt = new Date().toISOString();
  const sk = skill && skill.trim() ? skill.trim() : null;
  const ct = contact && contact.trim() ? contact.trim() : null;
  const cty = city && city.trim() ? city.trim() : null;

  // Email optional; if provided, normalize + check uniqueness
  let emailVal = null;
  if (email && email.trim()) {
    emailVal = email.trim().toLowerCase();
    const exists = db.prepare("SELECT id FROM collaborators WHERE email = ?").get(emailVal);
    if (exists) return res.status(409).json({ error: "email already registered" });
  }

  // Password optional; if provided, hash it
  let passwordHash = null;
  if (password) {
    if (password.length < 6) return res.status(400).json({ error: "password must be at least 6 chars" });
    passwordHash = hashPassword(password);
  }

  db.prepare(
    "INSERT INTO collaborators (id, name, skill, contact, city, email, password_hash, created_at) VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
  ).run(finalId, name.trim(), sk, ct, cty, emailVal, passwordHash, createdAt);
  res.status(201).json({
    id: finalId,
    name: name.trim(),
    skill: sk,
    contact: ct,
    city: cty,
    email: emailVal,
    created_at: createdAt,
  });
});
// Authorize a volunteer: set email + provisional password (admin only)
app.post("/api/collaborators/:id/authorize", requireAdmin, (req, res) => {
  const { email, password } = req.body || {};
  if (!email || !email.trim()) return res.status(400).json({ error: "email is required to authorize" });
  if (!password || password.length < 6) return res.status(400).json({ error: "password must be at least 6 chars" });

  const emailVal = email.trim().toLowerCase();
  const dup = db.prepare("SELECT id FROM collaborators WHERE email = ? AND id != ?").get(emailVal, req.params.id);
  if (dup) return res.status(409).json({ error: "email already in use" });

  const result = db.prepare("UPDATE collaborators SET email = ?, password_hash = ? WHERE id = ?")
    .run(emailVal, hashPassword(password), req.params.id);
  if (result.changes === 0) return res.status(404).json({ error: "not found" });
  res.json({ ok: true });
});
// Admin password reset
app.post("/api/collaborators/:id/reset-password", requireAdmin, (req, res) => {
  const { password } = req.body || {};
  if (!password || password.length < 6) return res.status(400).json({ error: "password must be at least 6 chars" });
  const result = db.prepare("UPDATE collaborators SET password_hash = ? WHERE id = ?")
    .run(hashPassword(password), req.params.id);
  if (result.changes === 0) return res.status(404).json({ error: "not found" });
  res.json({ ok: true });
});
// ========== Auth endpoints ==========
app.post("/api/auth/login", (req, res) => {
  const { identifier, password } = req.body || {};
  if (!identifier || !password) return res.status(400).json({ error: "identifier and password are required" });

  const id = identifier.trim().toLowerCase();
  const user = db.prepare(
    "SELECT id, name, email, password_hash, contact, display_name FROM collaborators WHERE LOWER(email) = ? OR LOWER(name) = ?",
  ).get(id, id);

  if (!user || !verifyPassword(password, user.password_hash)) {
    return res.status(401).json({ error: "invalid credentials" });
  }
  const token = createToken(user.id);
  res.json({
    token,
    user: {
      id: user.id,
      name: user.name,
      email: user.email,
      display_name: user.display_name || null,
      chat_name: resolveDisplayName(user),
    },
  });
});
app.get("/api/auth/me", requireAuth, (req, res) => {
  const user = db.prepare("SELECT id, name, email, skill, contact, city, display_name FROM collaborators WHERE id = ?").get(req.userId);
  if (!user) return res.status(404).json({ error: "not found" });
  res.json({ ...user, chat_name: resolveDisplayName(user) });
});
// Provision (or return) Matrix credentials for the logged-in collaborator.
// The account is created on first call; subsequent calls return the stored
// credentials so the client can re-login without a second password.
app.post("/api/auth/matrix", requireAuth, async (req, res) => {
  try {
    const user = db.prepare("SELECT id, name, contact, display_name, matrix_user_id, matrix_password FROM collaborators WHERE id = ?").get(req.userId);
    if (!user) return res.status(404).json({ error: "not found" });

    let matrixUserId = user.matrix_user_id;
    let matrixPassword = user.matrix_password;

    if (!matrixUserId || !matrixPassword) {
      const displayName = resolveDisplayName(user);
      const provisioned = await provisionMatrixAccount(user.id, displayName);
      matrixUserId = provisioned.user_id;
      matrixPassword = provisioned.password;
      db.prepare("UPDATE collaborators SET matrix_user_id = ?, matrix_password = ? WHERE id = ?")
        .run(matrixUserId, matrixPassword, user.id);
    }

    res.json({
      base_url: MATRIX_PUBLIC_URL,
      user_id: matrixUserId,
      password: matrixPassword,
    });
  } catch (e) {
    console.error("Matrix provisioning error:", e);
    res.status(502).json({ error: "matrix provisioning failed" });
  }
});
app.patch("/api/auth/me", requireAuth, (req, res) => {
  const { display_name } = req.body || {};
  if (display_name !== undefined && display_name !== null && typeof display_name !== "string") {
    return res.status(400).json({ error: "display_name must be a string" });
  }
  const trimmed = typeof display_name === "string" ? display_name.trim() : null;
  if (trimmed && trimmed.length > 40) {
    return res.status(400).json({ error: "display_name too long (max 40 chars)" });
  }
  const result = db.prepare("UPDATE collaborators SET display_name = ? WHERE id = ?").run(trimmed, req.userId);
  if (result.changes === 0) return res.status(404).json({ error: "not found" });
  const user = db.prepare("SELECT id, name, email, skill, contact, city, display_name FROM collaborators WHERE id = ?").get(req.userId);
  res.json({ ...user, chat_name: resolveDisplayName(user) });
});
app.delete("/api/collaborators/:id", requireAdmin, (req, res) => {
  const result = db
    .prepare("DELETE FROM collaborators WHERE id = ?")
    .run(req.params.id);
  if (result.changes === 0) return res.status(404).json({ error: "not found" });
  res.status(204).end();
});
// ========== Anuncios ==========
// Helper: parse the JSON `images` column into an array (or [] if null/invalid).
function parseImages(raw) {
  if (!raw) return [];
  try {
    const arr = JSON.parse(raw);
    return Array.isArray(arr) ? arr.filter((u) => typeof u === "string" && u.trim()) : [];
  } catch {
    return [];
  }
}

app.get("/api/anuncios", (req, res) => {
  const rows = db
    .prepare("SELECT id, title, text, image, photo_url, images, created_at FROM anuncios ORDER BY RANDOM()")
    .all();
  res.json(
    rows.map((r) => ({ ...r, images: parseImages(r.images) })),
  );
});
app.post("/api/anuncios", (req, res) => {
  const { title, text, image, images } = req.body || {};
  if (!text || !text.trim()) {
    return res.status(400).json({ error: "text is required" });
  }
  const finalId = crypto.randomUUID();
  const createdAt = new Date().toISOString();
  const ttl = title && title.trim() ? title.trim() : null;
  const img = image && image.trim() ? image.trim() : null;
  const imgs = Array.isArray(images)
    ? images.filter((u) => typeof u === "string" && u.trim()).map((u) => u.trim())
    : [];
  // Ensure the main image is part of the gallery.
  if (img && !imgs.includes(img)) imgs.unshift(img);
  db.prepare(
    "INSERT INTO anuncios (id, title, text, image, images, created_at) VALUES (?, ?, ?, ?, ?, ?)",
  ).run(finalId, ttl, text.trim(), img, JSON.stringify(imgs), createdAt);
  res.status(201).json({
    id: finalId,
    title: ttl,
    text: text.trim(),
    image: img,
    images: imgs,
    created_at: createdAt,
  });
});
// Update main image and/or gallery for an anuncio.
app.patch("/api/anuncios/:id", (req, res) => {
  const { image, images } = req.body || {};
  const existing = db
    .prepare("SELECT id, image, images FROM anuncios WHERE id = ?")
    .get(req.params.id);
  if (!existing) return res.status(404).json({ error: "not found" });

  const fields = [];
  const values = [];

  if (image !== undefined) {
    const img = image && image.trim() ? image.trim() : null;
    fields.push("image = ?");
    values.push(img);
  }
  if (images !== undefined) {
    const imgs = Array.isArray(images)
      ? images.filter((u) => typeof u === "string" && u.trim()).map((u) => u.trim())
      : [];
    fields.push("images = ?");
    values.push(JSON.stringify(imgs));
  }
  if (fields.length === 0) {
    return res.status(400).json({ error: "no fields to update" });
  }
  values.push(req.params.id);
  db.prepare(`UPDATE anuncios SET ${fields.join(", ")} WHERE id = ?`).run(...values);

  const updated = db
    .prepare("SELECT id, title, text, image, photo_url, images, created_at FROM anuncios WHERE id = ?")
    .get(req.params.id);
  res.json({ ...updated, images: parseImages(updated.images) });
});
// Endpoint para actualizar photo_url de anuncios (similar a personas/mascotas/edificios)
app.patch("/api/anuncios/:id/photo", handlePhotoPatch("anuncios"));
app.delete("/api/anuncios/:id", requireAdmin, (req, res) => {
  const result = db
    .prepare("DELETE FROM anuncios WHERE id = ?")
    .run(req.params.id);
  if (result.changes === 0) return res.status(404).json({ error: "not found" });
  res.status(204).end();
});
// ========== Donaciones (privado: requiere login) ==========
app.get("/api/donaciones", requireAuth, (req, res) => {
  const rows = db
    .prepare(`
      SELECT d.*,
        (SELECT text FROM comments WHERE item_id = d.id AND item_type = 'donaciones' ORDER BY created_at DESC LIMIT 1) AS last_comment,
        (SELECT COUNT(*) FROM comments WHERE item_id = d.id AND item_type = 'donaciones') AS comment_count
      FROM donaciones d ORDER BY d.created_at DESC
    `)
    .all();
  res.json(rows);
});
app.post("/api/donaciones", requireAuth, (req, res) => {
  const { item_type, quantity, description, status, location, lat, lng, contact } = req.body || {};
  if (!item_type || !item_type.trim()) return res.status(400).json({ error: "item_type is required" });
  if (!description || !description.trim()) return res.status(400).json({ error: "description is required" });
  const finalId = crypto.randomUUID();
  const createdAt = new Date().toISOString();
  db.prepare(
    "INSERT INTO donaciones (id, item_type, quantity, description, status, location, lat, lng, contact, donor_id, created_at) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
  ).run(
    finalId,
    item_type.trim(),
    quantity && quantity.trim() ? quantity.trim() : null,
    description.trim(),
    status || "disponible",
    location && location.trim() ? location.trim() : null,
    lat ?? null,
    lng ?? null,
    contact && contact.trim() ? contact.trim() : null,
    req.userId,
    createdAt,
  );
  res.status(201).json({ id: finalId, created_at: createdAt });
});
app.patch("/api/donaciones/:id", requireAuth, (req, res) => {
  const { status, quantity, description, location } = req.body || {};
  const existing = db.prepare("SELECT id FROM donaciones WHERE id = ?").get(req.params.id);
  if (!existing) return res.status(404).json({ error: "not found" });
  const fields = [];
  const values = [];
  if (status !== undefined) { fields.push("status = ?"); values.push(status); }
  if (quantity !== undefined) { fields.push("quantity = ?"); values.push(quantity); }
  if (description !== undefined) { fields.push("description = ?"); values.push(description); }
  if (location !== undefined) { fields.push("location = ?"); values.push(location); }
  if (fields.length === 0) return res.status(400).json({ error: "no fields to update" });
  values.push(req.params.id);
  db.prepare(`UPDATE donaciones SET ${fields.join(", ")} WHERE id = ?`).run(...values);
  res.json({ ok: true });
});
app.delete("/api/donaciones/:id", requireAdmin, (req, res) => {
  const result = db.prepare("DELETE FROM donaciones WHERE id = ?").run(req.params.id);
  if (result.changes === 0) return res.status(404).json({ error: "not found" });
  res.status(204).end();
});

// ========== Necesidades (privado: requiere login) ==========
app.get("/api/necesidades", requireAuth, (req, res) => {
  const rows = db
    .prepare(`
      SELECT n.*,
        (SELECT text FROM comments WHERE item_id = n.id AND item_type = 'necesidades' ORDER BY created_at DESC LIMIT 1) AS last_comment,
        (SELECT COUNT(*) FROM comments WHERE item_id = n.id AND item_type = 'necesidades') AS comment_count
      FROM necesidades n ORDER BY n.created_at DESC
    `)
    .all();
  res.json(rows);
});
app.post("/api/necesidades", requireAuth, (req, res) => {
  const { item_type, quantity, description, urgency, status, point_name, location, lat, lng, contact } = req.body || {};
  if (!item_type || !item_type.trim()) return res.status(400).json({ error: "item_type is required" });
  if (!description || !description.trim()) return res.status(400).json({ error: "description is required" });
  const finalId = crypto.randomUUID();
  const createdAt = new Date().toISOString();
  db.prepare(
    "INSERT INTO necesidades (id, item_type, quantity, description, urgency, status, point_name, location, lat, lng, contact, reporter_id, created_at) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
  ).run(
    finalId,
    item_type.trim(),
    quantity && quantity.trim() ? quantity.trim() : null,
    description.trim(),
    urgency || "media",
    status || "abierta",
    point_name && point_name.trim() ? point_name.trim() : null,
    location && location.trim() ? location.trim() : null,
    lat ?? null,
    lng ?? null,
    contact && contact.trim() ? contact.trim() : null,
    req.userId,
    createdAt,
  );
  res.status(201).json({ id: finalId, created_at: createdAt });
});
app.patch("/api/necesidades/:id", requireAuth, (req, res) => {
  const { status, urgency, quantity, description } = req.body || {};
  const existing = db.prepare("SELECT id FROM necesidades WHERE id = ?").get(req.params.id);
  if (!existing) return res.status(404).json({ error: "not found" });
  const fields = [];
  const values = [];
  if (status !== undefined) { fields.push("status = ?"); values.push(status); }
  if (urgency !== undefined) { fields.push("urgency = ?"); values.push(urgency); }
  if (quantity !== undefined) { fields.push("quantity = ?"); values.push(quantity); }
  if (description !== undefined) { fields.push("description = ?"); values.push(description); }
  if (fields.length === 0) return res.status(400).json({ error: "no fields to update" });
  values.push(req.params.id);
  db.prepare(`UPDATE necesidades SET ${fields.join(", ")} WHERE id = ?`).run(...values);
  res.json({ ok: true });
});
app.delete("/api/necesidades/:id", requireAdmin, (req, res) => {
  const result = db.prepare("DELETE FROM necesidades WHERE id = ?").run(req.params.id);
  if (result.changes === 0) return res.status(404).json({ error: "not found" });
  res.status(204).end();
});

// ========== Logística (privado: requiere login) ==========
app.get("/api/logistica", requireAuth, (req, res) => {
  const rows = db
    .prepare(`
      SELECT l.*,
        (SELECT text FROM comments WHERE item_id = l.id AND item_type = 'logistica' ORDER BY created_at DESC LIMIT 1) AS last_comment,
        (SELECT COUNT(*) FROM comments WHERE item_id = l.id AND item_type = 'logistica') AS comment_count
      FROM logistica l ORDER BY l.created_at DESC
    `)
    .all();
  res.json(rows);
});
app.post("/api/logistica", requireAuth, (req, res) => {
  const { task_type, item_ref, description, status, origin, destination, lat, lng, contact, assignee_id } = req.body || {};
  if (!task_type || !task_type.trim()) return res.status(400).json({ error: "task_type is required" });
  if (!description || !description.trim()) return res.status(400).json({ error: "description is required" });
  const finalId = crypto.randomUUID();
  const createdAt = new Date().toISOString();
  db.prepare(
    "INSERT INTO logistica (id, task_type, item_ref, description, status, origin, destination, lat, lng, contact, assignee_id, creator_id, created_at) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
  ).run(
    finalId,
    task_type.trim(),
    item_ref && item_ref.trim() ? item_ref.trim() : null,
    description.trim(),
    status || "pendiente",
    origin && origin.trim() ? origin.trim() : null,
    destination && destination.trim() ? destination.trim() : null,
    lat ?? null,
    lng ?? null,
    contact && contact.trim() ? contact.trim() : null,
    assignee_id || null,
    req.userId,
    createdAt,
  );
  res.status(201).json({ id: finalId, created_at: createdAt });
});
app.patch("/api/logistica/:id", requireAuth, (req, res) => {
  const { status, assignee_id, description, origin, destination } = req.body || {};
  const existing = db.prepare("SELECT id FROM logistica WHERE id = ?").get(req.params.id);
  if (!existing) return res.status(404).json({ error: "not found" });
  const fields = [];
  const values = [];
  if (status !== undefined) { fields.push("status = ?"); values.push(status); }
  if (assignee_id !== undefined) { fields.push("assignee_id = ?"); values.push(assignee_id); }
  if (description !== undefined) { fields.push("description = ?"); values.push(description); }
  if (origin !== undefined) { fields.push("origin = ?"); values.push(origin); }
  if (destination !== undefined) { fields.push("destination = ?"); values.push(destination); }
  if (fields.length === 0) return res.status(400).json({ error: "no fields to update" });
  values.push(req.params.id);
  db.prepare(`UPDATE logistica SET ${fields.join(", ")} WHERE id = ?`).run(...values);
  res.json({ ok: true });
});
app.delete("/api/logistica/:id", requireAdmin, (req, res) => {
  const result = db.prepare("DELETE FROM logistica WHERE id = ?").run(req.params.id);
  if (result.changes === 0) return res.status(404).json({ error: "not found" });
  res.status(204).end();
});
// ========== Health ==========
app.get("/api/health", (req, res) => res.json({ ok: true }));
// ============================================================
// NUEVO: Endpoint para obtener sismos en tiempo real (USGS)
// ============================================================
app.get("/api/earthquakes", async (req, res) => {
  try {
    // Parámetros: magnitud mínima (default 0) y límite (default 500).
    // USGS permite hasta 20000 resultados por petición.
    const minmag = parseFloat(req.query.minmagnitude) || 0;
    const limit = Math.min(parseInt(req.query.limit, 10) || 500, 20000);

    // Ventana temporal: USGS por defecto solo devuelve los últimos 30 días
    // si no se pasa starttime/endtime. Ampliamos a 365 días por defecto.
    // `hours` permite ventanas cortas (p. ej. ?hours=24) para el feed "en vivo".
    const hours = parseFloat(req.query.hours);
    const days = Number.isFinite(hours) && hours > 0
      ? hours / 24
      : (parseInt(req.query.days, 10) || 365);
    const end = new Date();
    const start = new Date(end.getTime() - days * 86400000);

    // Bounding box aproximado de Colombia (puedes ajustarlo)
    const url = `https://earthquake.usgs.gov/fdsnws/event/1/query?format=geojson&minlatitude=-4&maxlatitude=12&minlongitude=-80&maxlongitude=-66&minmagnitude=${minmag}&limit=${limit}&orderby=time&starttime=${start.toISOString()}&endtime=${end.toISOString()}`;

    const response = await fetch(url);
    if (!response.ok) throw new Error("Error al consultar USGS");
    const data = await response.json();

    // Transformamos los datos a un formato liviano para el frontend
    const earthquakes = data.features.map((f) => ({
      id: f.id,
      mag: f.properties.mag,
      place: f.properties.place,
      time: new Date(f.properties.time).toLocaleString("es-CO", {
        timeZone: "America/Bogota",
      }),
      timestamp: f.properties.time, // Unix en milisegundos
      lat: f.geometry.coordinates[1],
      lng: f.geometry.coordinates[0],
      depth: f.geometry.coordinates[2],
      url: f.properties.url,
    }));

    // Extraemos el último sismo para mostrarlo como alerta
    const lastQuake = earthquakes.length > 0 ? earthquakes[0] : null;

    res.json({
      count: earthquakes.length,
      lastQuake,
      earthquakes,
    });
  } catch (error) {
    console.error("Error en /api/earthquakes:", error);
    res.status(500).json({ error: "No se pudo obtener la data sísmica" });
  }
});

// ========== Backlog Routes ==========
app.get("/api/backlog", requireAuth, (req, res) => {
  const rows = db.prepare("SELECT * FROM backlog ORDER BY created_at DESC").all();
  res.json(rows.map(r => ({
    ...r,
    creator_name: resolveMatrixDisplayName(r.creator_matrix_id),
    extracted_data: r.extracted_data ? JSON.parse(r.extracted_data) : []
  })));
});

app.post("/api/backlog", requireAuth, (req, res) => {
  const { source_event_id, creator_matrix_id, intent, extracted_data, raw_text, raw_json, enriched_data, message_count } = req.body;

  if (!source_event_id || !intent) {
    return res.status(400).json({ error: "source_event_id and intent are required" });
  }

  const finalId = crypto.randomUUID();
  const createdAt = new Date().toISOString();
  const dataJson = extracted_data ? JSON.stringify(extracted_data) : "[]";
  const rawJson = typeof raw_json === "string" ? raw_json : (raw_json ? JSON.stringify(raw_json) : null);
  const enrichedJson = typeof enriched_data === "string" ? enriched_data : (enriched_data ? JSON.stringify(enriched_data) : null);
  const msgCount = Number.isInteger(message_count) && message_count > 0 ? message_count : 1;

  try {
    db.prepare(
      "INSERT INTO backlog (id, source_event_id, creator_matrix_id, intent, extracted_data, raw_text, raw_json, enriched_data, message_count, created_at) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
    ).run(finalId, source_event_id, creator_matrix_id, intent, dataJson, raw_text || null, rawJson, enrichedJson, msgCount, createdAt);

    res.status(201).json({ id: finalId, status: "pending" });
  } catch (e) {
    // The UNIQUE constraint on source_event_id prevents duplicate backlog items
    if (e.message.includes("UNIQUE constraint failed")) {
      return res.status(409).json({ error: "already_exists" });
    }
    console.error("Backlog insert error:", e);
    res.status(500).json({ error: "Failed to create backlog item" });
  }
});

app.patch("/api/backlog/:id", requireAuth, (req, res) => {
  const { status } = req.body;
  if (!['pending', 'accepted', 'rejected'].includes(status)) {
    return res.status(400).json({ error: "Invalid status" });
  }

  const result = db.prepare("UPDATE backlog SET status = ? WHERE id = ?").run(status, req.params.id);
  if (result.changes === 0) return res.status(404).json({ error: "not found" });

  res.json({ ok: true });
});

// Standalone OCR storage — independent of triage decisions.
app.post("/api/media-text", requireAuth, (req, res) => {
  const { source_event_id, raw_text } = req.body || {};
  if (!source_event_id || !raw_text) {
    return res.status(400).json({ error: "source_event_id and raw_text are required" });
  }
  const createdAt = new Date().toISOString();
  db.prepare(
    `INSERT INTO media_text (source_event_id, raw_text, created_at)
     VALUES (?, ?, ?)
     ON CONFLICT(source_event_id) DO UPDATE SET raw_text = excluded.raw_text`
  ).run(source_event_id, raw_text, createdAt);
  res.status(201).json({ ok: true });
});

// Fetch OCR text + intent for the Lightbox, keyed by Matrix event id.
app.get("/api/backlog/by-event/:eventId", requireAuth, (req, res) => {
  // 1. Check backlog first (triaged messages)
  const row = db.prepare(
    "SELECT raw_text, intent, enriched_data, extracted_data FROM backlog WHERE source_event_id = ?"
  ).get(req.params.eventId);

  if (row) {
    return res.json({
      raw_text: row.raw_text,
      intent: row.intent,
      enriched_data: row.enriched_data ? JSON.parse(row.enriched_data) : null,
      extracted_data: row.extracted_data ? JSON.parse(row.extracted_data) : []
    });
  }

  // 2. Fallback to media_text (non-triaged images)
  const mediaRow = db.prepare(
    "SELECT raw_text FROM media_text WHERE source_event_id = ?"
  ).get(req.params.eventId);

  if (mediaRow) {
    return res.json({ raw_text: mediaRow.raw_text, intent: null, enriched_data: null, extracted_data: [] });
  }

  // 3. Nothing found
  res.json({});
});
// Static files AFTER all API routes so /api/* is never intercepted
app.use(express.static("public"));
app.listen(PORT, () => {
  console.log(`sismo-api listening on :${PORT}`);
});

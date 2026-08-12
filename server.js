// Tiny emergency backend: Express + SQLite
// Run with systemd. Reports of people, pets, buildings, and announcements.
import express from "express";
import cors from "cors";
import Database from "better-sqlite3";

const app = express();
const PORT = process.env.PORT || 3000;
const DB_PATH = process.env.DB_PATH || "/opt/sismo-api/data.db";

app.use(cors());
app.use(express.json());
app.use(express.static("public"));

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
    text TEXT NOT NULL,
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

// --- Migrations for existing columns (city and image) ---
try {
  db.exec("ALTER TABLE disappeared ADD COLUMN city TEXT");
} catch (e) {}
try {
  db.exec("ALTER TABLE buildings ADD COLUMN city TEXT");
} catch (e) {}
try {
  db.exec("ALTER TABLE pets ADD COLUMN city TEXT");
} catch (e) {}
try {
  db.exec("ALTER TABLE disappeared ADD COLUMN image TEXT");
} catch (e) {}
try {
  db.exec("ALTER TABLE buildings ADD COLUMN image TEXT");
} catch (e) {}
try {
  db.exec("ALTER TABLE pets ADD COLUMN image TEXT");
} catch (e) {}
try {
  db.exec("ALTER TABLE disappeared ADD COLUMN photo_url TEXT");
} catch (e) {}
// Add photo_url to pets (for file uploads)
try {
  db.exec("ALTER TABLE pets ADD COLUMN photo_url TEXT");
} catch (e) {}
// --- lat/lng for map picker (separate from free-text location) ---
for (const t of ["disappeared", "pets", "buildings"]) {
  try { db.exec(`ALTER TABLE ${t} ADD COLUMN lat REAL`); } catch (e) {}
  try { db.exec(`ALTER TABLE ${t} ADD COLUMN lng REAL`); } catch (e) {}
}
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

app.delete("/api/comments/:id", (req, res) => {
  if (!process.env.ADMIN_KEY || req.headers["x-admin-key"] !== process.env.ADMIN_KEY) {
    return res.status(403).json({ error: "forbidden" });
  }
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
    return res.status(400).json({ error: "itemId and a valid itemType are required" });
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
  res.status(201).json({ id: finalId, lat, lng, label: lbl, created_at: createdAt });
});

app.delete("/api/item-locations/:id", (req, res) => {
  if (!process.env.ADMIN_KEY || req.headers["x-admin-key"] !== process.env.ADMIN_KEY) {
    return res.status(403).json({ error: "forbidden" });
  }
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
      SELECT id, name, status, location, city, created_at, photo_url, lat, lng,
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
app.patch("/api/disappeared/:id/photo", (req, res) => {
  const { photo_url } = req.body || {};
  if (
    !photo_url ||
    typeof photo_url !== "string" ||
    !/^https:\/\/res\.cloudinary\.com\//.test(photo_url)
  ) {
    return res.status(400).json({ error: "invalid photo_url" });
  }
  const result = db
    .prepare("UPDATE disappeared SET photo_url = ? WHERE id = ?")
    .run(photo_url, req.params.id);
  if (result.changes === 0) return res.status(404).json({ error: "not found" });
  res.json({ id: req.params.id, photo_url });
});

app.post("/api/disappeared", (req, res) => {
  const { id, name, location, city, image, lat, lng } = req.body || {};
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
  db.prepare(
    "INSERT INTO disappeared (id, name, status, location, city, image, created_at, lat, lng) VALUES (?, ?, 'desaparecido', ?, ?, ?, ?, ?, ?)",
  ).run(finalId, name.trim(), loc, cty, img, createdAt, latVal, lngVal);
  res.status(201).json({
    id: finalId,
    name: name.trim(),
    status: "desaparecido",
    location: loc,
    city: cty,
    image: img,
    created_at: createdAt,
    lat: latVal,
    lng: lngVal,
  });
});

app.patch("/api/disappeared/:id", (req, res) => {
  const { status, image, lat, lng } = req.body || {};
  if (status && !["desaparecido", "encontrado"].includes(status)) {
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

app.delete("/api/disappeared/:id", (req, res) => {
  if (!process.env.ADMIN_KEY || req.headers["x-admin-key"] !== process.env.ADMIN_KEY) {
    return res.status(403).json({ error: "forbidden" });
  }
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
        (SELECT text FROM comments WHERE item_id = pets.id AND item_type = 'pets' ORDER BY created_at DESC LIMIT 1) AS last_comment,
        (SELECT COUNT(*) FROM comments WHERE item_id = pets.id AND item_type = 'pets') AS comment_count
      FROM pets ORDER BY RANDOM()
    `,
    )
    .all();
  res.json(rows);
});
// Endpoint para actualizar photo_url de mascotas (similar a personas)
app.patch("/api/pets/:id/photo", (req, res) => {
  const { photo_url } = req.body || {};
  if (
    !photo_url ||
    typeof photo_url !== "string" ||
    !/^https:\/\/res\.cloudinary\.com\//.test(photo_url)
  ) {
    return res.status(400).json({ error: "invalid photo_url" });
  }
  const result = db
    .prepare("UPDATE pets SET photo_url = ? WHERE id = ?")
    .run(photo_url, req.params.id);
  if (result.changes === 0) return res.status(404).json({ error: "not found" });
  res.json({ id: req.params.id, photo_url });
});

app.post("/api/pets", (req, res) => {
  const { id, name, location, city, image, lat, lng } = req.body || {};
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
  db.prepare(
    "INSERT INTO pets (id, name, status, location, city, image, created_at, lat, lng) VALUES (?, ?, 'desaparecido', ?, ?, ?, ?, ?, ?)",
  ).run(finalId, name.trim(), loc, cty, img, createdAt, latVal, lngVal);
  res.status(201).json({
    id: finalId,
    name: name.trim(),
    status: "desaparecido",
    location: loc,
    city: cty,
    image: img,
    created_at: createdAt,
    lat: latVal,
    lng: lngVal,
  });
});

app.patch("/api/pets/:id", (req, res) => {
  const { status, image, lat, lng } = req.body || {};
  if (status && !["desaparecido", "encontrado"].includes(status)) {
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

app.delete("/api/pets/:id", (req, res) => {
  if (!process.env.ADMIN_KEY || req.headers["x-admin-key"] !== process.env.ADMIN_KEY) {
    return res.status(403).json({ error: "forbidden" });
  }
  const result = db.prepare("DELETE FROM pets WHERE id = ?").run(req.params.id);
  if (result.changes === 0) return res.status(404).json({ error: "not found" });
  res.status(204).end();
});

// ========== Edificios ==========
app.get("/api/buildings", (req, res) => {
  const rows = db
    .prepare(
      `
      SELECT id, name, status, location, city, image, created_at, lat, lng,
        (SELECT text FROM comments WHERE item_id = buildings.id AND item_type = 'buildings' ORDER BY created_at DESC LIMIT 1) AS last_comment,
        (SELECT COUNT(*) FROM comments WHERE item_id = buildings.id AND item_type = 'buildings') AS comment_count
      FROM buildings ORDER BY RANDOM()
    `,
    )
    .all();
  res.json(rows);
});

app.post("/api/buildings", (req, res) => {
  const { id, name, location, city, image, lat, lng } = req.body || {};
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
  db.prepare(
    "INSERT INTO buildings (id, name, status, location, city, image, created_at, lat, lng) VALUES (?, ?, 'seguro', ?, ?, ?, ?, ?, ?)",
  ).run(finalId, name.trim(), loc, cty, img, createdAt, latVal, lngVal);
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
  });
});

app.patch("/api/buildings/:id", (req, res) => {
  const { status, image, lat, lng } = req.body || {};
  if (status && !["seguro", "danado", "colapsado"].includes(status)) {
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
    .prepare(`UPDATE buildings SET ${fields.join(", ")} WHERE id = ?`)
    .run(...values);
  if (result.changes === 0) return res.status(404).json({ error: "not found" });
  res.json({ id: req.params.id, status, image, lat, lng });
});

app.delete("/api/buildings/:id", (req, res) => {
  if (!process.env.ADMIN_KEY || req.headers["x-admin-key"] !== process.env.ADMIN_KEY) {
    return res.status(403).json({ error: "forbidden" });
  }
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

app.post("/api/collaborators", (req, res) => {
  const { name, skill, contact, city } = req.body || {};
  if (!name || !name.trim()) {
    return res.status(400).json({ error: "name is required" });
  }
  const finalId = crypto.randomUUID();
  const createdAt = new Date().toISOString();
  const sk = skill && skill.trim() ? skill.trim() : null;
  const ct = contact && contact.trim() ? contact.trim() : null;
  const cty = city && city.trim() ? city.trim() : null;
  db.prepare(
    "INSERT INTO collaborators (id, name, skill, contact, city, created_at) VALUES (?, ?, ?, ?, ?, ?)",
  ).run(finalId, name.trim(), sk, ct, cty, createdAt);
  res.status(201).json({
    id: finalId,
    name: name.trim(),
    skill: sk,
    contact: ct,
    city: cty,
    created_at: createdAt,
  });
});

app.delete("/api/collaborators/:id", (req, res) => {
  if (!process.env.ADMIN_KEY || req.headers["x-admin-key"] !== process.env.ADMIN_KEY) {
    return res.status(403).json({ error: "forbidden" });
  }
  const result = db
    .prepare("DELETE FROM collaborators WHERE id = ?")
    .run(req.params.id);
  if (result.changes === 0) return res.status(404).json({ error: "not found" });
  res.status(204).end();
});

// ========== Anuncios ==========
app.get("/api/anuncios", (req, res) => {
  const rows = db
    .prepare("SELECT id, text, created_at FROM anuncios ORDER BY RANDOM()")
    .all();
  res.json(rows);
});

app.post("/api/anuncios", (req, res) => {
  const { text } = req.body || {};
  if (!text || !text.trim()) {
    return res.status(400).json({ error: "text is required" });
  }
  const finalId = crypto.randomUUID();
  const createdAt = new Date().toISOString();
  db.prepare(
    "INSERT INTO anuncios (id, text, created_at) VALUES (?, ?, ?)",
  ).run(finalId, text.trim(), createdAt);
  res
    .status(201)
    .json({ id: finalId, text: text.trim(), created_at: createdAt });
});

app.delete("/api/anuncios/:id", (req, res) => {
  if (!process.env.ADMIN_KEY || req.headers["x-admin-key"] !== process.env.ADMIN_KEY) {
    return res.status(403).json({ error: "forbidden" });
  }
  const result = db
    .prepare("DELETE FROM anuncios WHERE id = ?")
    .run(req.params.id);
  if (result.changes === 0) return res.status(404).json({ error: "not found" });
  res.status(204).end();
});

// ========== Health ==========
app.get("/api/health", (req, res) => res.json({ ok: true }));

app.listen(PORT, () => {
  console.log(`sismo-api listening on :${PORT}`);
});

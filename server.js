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

// Admin auth middleware
function requireAdmin(req, res, next) {
  if (!process.env.ADMIN_KEY || req.headers["x-admin-key"] !== process.env.ADMIN_KEY) {
    return res.status(403).json({ error: "forbidden" });
  }
  next();
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
  const rows = db
    .prepare(
      `
      SELECT id, name, status, location, city, image, photo_url, created_at, lat, lng,
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
// Endpoint para actualizar photo_url de edificios (similar a personas y mascotas)
app.patch("/api/buildings/:id/photo", handlePhotoPatch("buildings"));

app.patch("/api/buildings/:id", (req, res) => {
  const { status, image, lat, lng } = req.body || {};
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
    const days = parseInt(req.query.days, 10) || 365;
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
// Static files AFTER all API routes so /api/* is never intercepted
app.use(express.static("public"));
app.listen(PORT, () => {
  console.log(`sismo-api listening on :${PORT}`);
});

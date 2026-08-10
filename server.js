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

// --- Migrate existing tables (add city column if missing) ---
try {
  db.exec("ALTER TABLE disappeared ADD COLUMN city TEXT");
} catch (e) {}
try {
  db.exec("ALTER TABLE buildings ADD COLUMN city TEXT");
} catch (e) {}
try {
  db.exec("ALTER TABLE pets ADD COLUMN city TEXT");
} catch (e) {}

// ========== Personas ==========
app.get("/api/disappeared", (req, res) => {
  const rows = db
    .prepare(
      "SELECT id, name, status, location, city, created_at FROM disappeared ORDER BY created_at DESC",
    )
    .all();
  res.json(rows);
});

app.post("/api/disappeared", (req, res) => {
  const { id, name, location, city } = req.body || {};
  if (!name || !name.trim()) {
    return res.status(400).json({ error: "name is required" });
  }
  const finalId = id || crypto.randomUUID();
  const createdAt = new Date().toISOString();
  const loc = location && location.trim() ? location.trim() : null;
  const cty = city && city.trim() ? city.trim() : null;
  db.prepare(
    "INSERT INTO disappeared (id, name, status, location, city, created_at) VALUES (?, ?, 'desaparecido', ?, ?, ?)",
  ).run(finalId, name.trim(), loc, cty, createdAt);
  res.status(201).json({
    id: finalId,
    name: name.trim(),
    status: "desaparecido",
    location: loc,
    city: cty,
    created_at: createdAt,
  });
});

app.patch("/api/disappeared/:id", (req, res) => {
  const { status } = req.body || {};
  if (!status || !["desaparecido", "encontrado"].includes(status)) {
    return res.status(400).json({ error: "invalid status" });
  }
  const result = db
    .prepare("UPDATE disappeared SET status = ? WHERE id = ?")
    .run(status, req.params.id);
  if (result.changes === 0) return res.status(404).json({ error: "not found" });
  res.json({ id: req.params.id, status });
});

app.delete("/api/disappeared/:id", (req, res) => {
  if (req.headers["x-admin-key"] !== process.env.ADMIN_KEY) {
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
      "SELECT id, name, status, location, city, created_at FROM pets ORDER BY created_at DESC",
    )
    .all();
  res.json(rows);
});

app.post("/api/pets", (req, res) => {
  const { id, name, location, city } = req.body || {};
  if (!name || !name.trim()) {
    return res.status(400).json({ error: "name is required" });
  }
  const finalId = id || crypto.randomUUID();
  const createdAt = new Date().toISOString();
  const loc = location && location.trim() ? location.trim() : null;
  const cty = city && city.trim() ? city.trim() : null;
  db.prepare(
    "INSERT INTO pets (id, name, status, location, city, created_at) VALUES (?, ?, 'desaparecido', ?, ?, ?)",
  ).run(finalId, name.trim(), loc, cty, createdAt);
  res.status(201).json({
    id: finalId,
    name: name.trim(),
    status: "desaparecido",
    location: loc,
    city: cty,
    created_at: createdAt,
  });
});

app.patch("/api/pets/:id", (req, res) => {
  const { status } = req.body || {};
  if (!status || !["desaparecido", "encontrado"].includes(status)) {
    return res.status(400).json({ error: "invalid status" });
  }
  const result = db
    .prepare("UPDATE pets SET status = ? WHERE id = ?")
    .run(status, req.params.id);
  if (result.changes === 0) return res.status(404).json({ error: "not found" });
  res.json({ id: req.params.id, status });
});

app.delete("/api/pets/:id", (req, res) => {
  if (req.headers["x-admin-key"] !== process.env.ADMIN_KEY) {
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
      "SELECT id, name, status, location, city, created_at FROM buildings ORDER BY created_at DESC",
    )
    .all();
  res.json(rows);
});

app.post("/api/buildings", (req, res) => {
  const { id, name, location, city } = req.body || {};
  if (!name || !name.trim()) {
    return res.status(400).json({ error: "name is required" });
  }
  const finalId = id || crypto.randomUUID();
  const createdAt = new Date().toISOString();
  const loc = location && location.trim() ? location.trim() : null;
  const cty = city && city.trim() ? city.trim() : null;
  db.prepare(
    "INSERT INTO buildings (id, name, status, location, city, created_at) VALUES (?, ?, 'seguro', ?, ?, ?)",
  ).run(finalId, name.trim(), loc, cty, createdAt);
  res.status(201).json({
    id: finalId,
    name: name.trim(),
    status: "seguro",
    location: loc,
    city: cty,
    created_at: createdAt,
  });
});

app.patch("/api/buildings/:id", (req, res) => {
  const { status } = req.body || {};
  if (!status || !["seguro", "danado", "colapsado"].includes(status)) {
    return res.status(400).json({ error: "invalid status" });
  }
  const result = db
    .prepare("UPDATE buildings SET status = ? WHERE id = ?")
    .run(status, req.params.id);
  if (result.changes === 0) return res.status(404).json({ error: "not found" });
  res.json({ id: req.params.id, status });
});

app.delete("/api/buildings/:id", (req, res) => {
  if (req.headers["x-admin-key"] !== process.env.ADMIN_KEY) {
    return res.status(403).json({ error: "forbidden" });
  }
  const result = db
    .prepare("DELETE FROM buildings WHERE id = ?")
    .run(req.params.id);
  if (result.changes === 0) return res.status(404).json({ error: "not found" });
  res.status(204).end();
});

// ========== Anuncios ==========
app.get("/api/anuncios", (req, res) => {
  const rows = db
    .prepare(
      "SELECT id, text, created_at FROM anuncios ORDER BY created_at DESC",
    )
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
  if (req.headers["x-admin-key"] !== process.env.ADMIN_KEY) {
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

// Tiny emergency backend: Express + SQLite, no Docker, no pm2.
// Run with systemd. Reports of disappeared people, shared across devices.
import express from "express";
import cors from "cors";
import Database from "better-sqlite3";

const app = express();
const PORT = process.env.PORT || 3000;
const DB_PATH = process.env.DB_PATH || "/opt/sismo-api/data.db";

// Middleware
app.use(cors());
app.use(express.json());

// SQLite setup
const db = new Database(DB_PATH);
db.pragma("journal_mode = WAL"); // crash-safe, better concurrency

db.exec(`
  CREATE TABLE IF NOT EXISTS disappeared (
    id TEXT PRIMARY KEY,
    name TEXT NOT NULL,
    status TEXT NOT NULL DEFAULT 'desaparecido',
    created_at TEXT NOT NULL
  )
`);

// List all reports (newest first)
app.get("/api/disappeared", (req, res) => {
  const rows = db.prepare(
    "SELECT id, name, status, created_at FROM disappeared ORDER BY created_at DESC"
  ).all();
  res.json(rows);
});

// Create a report
app.post("/api/disappeared", (req, res) => {
  const { id, name } = req.body || {};
  if (!name || !name.trim()) {
    return res.status(400).json({ error: "name is required" });
  }
  const finalId = id || crypto.randomUUID();
  const createdAt = new Date().toISOString();
  db.prepare(
    "INSERT INTO disappeared (id, name, status, created_at) VALUES (?, ?, 'desaparecido', ?)"
  ).run(finalId, name.trim(), createdAt);
  res.status(201).json({ id: finalId, name: name.trim(), status: "desaparecido", created_at: createdAt });
});

// Update status (mark found)
app.patch("/api/disappeared/:id", (req, res) => {
  const { status } = req.body || {};
  if (!status || !["desaparecido", "encontrado"].includes(status)) {
    return res.status(400).json({ error: "invalid status" });
  }
  const result = db.prepare("UPDATE disappeared SET status = ? WHERE id = ?").run(status, req.params.id);
  if (result.changes === 0) return res.status(404).json({ error: "not found" });
  res.json({ id: req.params.id, status });
});

// Delete a report
app.delete("/api/disappeared/:id", (req, res) => {
  const result = db.prepare("DELETE FROM disappeared WHERE id = ?").run(req.params.id);
  if (result.changes === 0) return res.status(404).json({ error: "not found" });
  res.status(204).end();
});

// Health check
app.get("/api/health", (req, res) => res.json({ ok: true }));

app.listen(PORT, () => {
  console.log(`sismo-api listening on :${PORT}`);
});
// normalize_db_cities.js
// Usage: node normalize_db_cities.js
// Reads existing 'disappeared' table, normalizes city/department, backfills lat/lng, writes back.

const fs = require("fs");
const path = require("path");
const Database = require("better-sqlite3");

const DB_PATH = process.env.DB_PATH || path.join(__dirname, "data.db");
const CITIES = path.join(__dirname, "public", "cities.json");

const db = new Database(DB_PATH);

// --- helpers ---
const normalize = (s) =>
  (s || "")
    .toLowerCase()
    .normalize("NFD")
    .replace(/[\u0300-\u036f]/g, "")
    .replace(/\s+/g, " ")
    .trim();

// --- load cities.json ---
const citiesRaw = JSON.parse(fs.readFileSync(CITIES, "utf8"));
const lookup = new Map();
for (const c of citiesRaw) {
  const key = normalize(c.name);
  if (key) {
    lookup.set(key, { city: c.name, lat: c.lat, lng: c.lng });
  }
}

// --- department canonicalization ---
const deptFix = (d) => {
  if (!d) return d;
  const n = normalize(d);
  if (n === "choco") return "Chocó";
  if (n === "valle de cauca") return "Valle del Cauca";
  return d;
};

// --- city fixes ---
const cityFix = (city) => {
  if (!city) return city;
  const trimmed = city.trim();
  if (/^["\s]*choco["\s]*$/i.test(trimmed)) return "Quibdó";
  if (normalize(trimmed) === "timbio") return "Timbío";
  if (normalize(trimmed) === "el dovio") return "El Dovío";
  if (normalize(trimmed) === "anserma nuevo") return "Ansermanuevo";
  return trimmed;
};

// --- process ---
const rows = db.prepare("SELECT id, city, department, lat, lng FROM disappeared").all();

let cityFixes = 0;
let deptFixes = 0;
let geocoded = 0;
let unmatched = [];

const update = db.prepare(`
  UPDATE disappeared
  SET city = ?, department = ?, lat = ?, lng = ?
  WHERE id = ?
`);

for (const row of rows) {
  let city = row.city;
  let department = row.department;
  let lat = row.lat;
  let lng = row.lng;

  // Fix city
  if (city) {
    const fixed = cityFix(city);
    if (fixed !== city) {
      cityFixes++;
      city = fixed;
    }
  }
  const cityNorm = normalize(city);

  // Backfill lat/lng
  if ((lat == null || lng == null) && cityNorm) {
    const hit = lookup.get(cityNorm);
    if (hit) {
      lat = hit.lat;
      lng = hit.lng;
      geocoded++;
    } else {
      unmatched.push(city);
    }
  }

  // Fix department
  if (department) {
    const fixed = deptFix(department);
    if (fixed !== department) {
      deptFixes++;
      department = fixed;
    }
  }

  // Write back
  update.run(city, department, lat, lng, row.id);
}

console.log(`Done. ${rows.length} rows updated.`);
console.log(`  City spelling fixes:     ${cityFixes}`);
console.log(`  Department spelling fixes: ${deptFixes}`);
console.log(`  Lat/lng backfilled:      ${geocoded}`);
if (unmatched.length > 0) {
  console.log(`  Cities with no match in cities.json (${unmatched.length}):`);
  console.log("    " + [...new Set(unmatched)].join(", "));
}
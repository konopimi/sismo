// normalize_cities.js
// Usage: node normalize_cities.js
// Input:  parsed_encontrados.json  (place in project root first)
// Output: parsed_encontrados_normalized.json
//
// What it does:
//   1. Loads public/cities.json -> builds a normalized lookup map
//   2. Loads parsed_encontrados.json (101 records)
//   3. Fixes corrupted city values (e.g. '   "Choco"' -> 'Quibdó')
//   4. Fixes missing accents (Timbio -> Timbío, El Dovio -> El Dovío)
//   5. Backfills lat/lng from cities.json via fuzzy (accent-insensitive) match
//   6. Fixes department spelling (Choco -> Chocó, Valle de Cauca -> Valle del Cauca)
//   7. Adds city_normalized and department_normalized fields
//   8. Writes parsed_encontrados_normalized.json

import fs from "fs";

const INPUT = "/home/kpm/.local/src/sismo_spider/parsed_encontrados.json";
const CITIES = "/home/kpm/.local/src/sismo/public/cities.json";
const OUTPUT =
  "/home/kpm/.local/src/sismo_spider/parsed_encontrados_normalized.json";

// --- helpers ---
// Lowercase + strip tildes + trim, so "Quibdó" and "quibdo" match.
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

// --- load records ---
const records = JSON.parse(fs.readFileSync(INPUT, "utf8"));

// --- department canonicalization ---
const deptFix = (d) => {
  if (!d) return d;
  const n = normalize(d);
  if (n === "choco") return "Chocó";
  if (n === "valle de cauca") return "Valle del Cauca";
  return d;
};

// --- city fixes for known bad values ---
const cityFix = (city) => {
  if (!city) return city;
  const trimmed = city.trim();
  // Corrupted line 1478: '   "Choco"' — Chocó is a department, not a city.
  // The only Chocó city in our data is Quibdó, so map it there.
  if (/^["\s]*choco["\s]*$/i.test(trimmed)) return "Quibdó";
  if (normalize(trimmed) === "timbio") return "Timbío";
  if (normalize(trimmed) === "el dovio") return "El Dovío";
  if (normalize(trimmed) === "anserma nuevo") return "Ansermanuevo";
  return trimmed;
};

// --- process ---
let cityFixes = 0;
let deptFixes = 0;
let geocoded = 0;
let unmatched = [];

for (const rec of records) {
  // Fix corrupted / accent-missing city
  if (rec.city) {
    const fixed = cityFix(rec.city);
    if (fixed !== rec.city) cityFixes++;
    rec.city = fixed;
  }
  rec.city_normalized = normalize(rec.city);

  // Backfill lat/lng from cities.json
  if (rec.lat == null && rec.lng == null && rec.city_normalized) {
    const hit = lookup.get(rec.city_normalized);
    if (hit) {
      rec.lat = hit.lat;
      rec.lng = hit.lng;
      geocoded++;
    } else {
      unmatched.push(rec.city);
    }
  }

  // Fix department spelling
  if (rec.department) {
    const fixed = deptFix(rec.department);
    if (fixed !== rec.department) deptFixes++;
    rec.department = fixed;
  }
  rec.department_normalized = normalize(rec.department);
}

fs.writeFileSync(OUTPUT, JSON.stringify(records, null, 2) + "\n");

console.log(`Done. ${records.length} records -> ${OUTPUT}`);
console.log(`  City spelling fixes:     ${cityFixes}`);
console.log(`  Department spelling fixes: ${deptFixes}`);
console.log(`  Lat/lng backfilled:      ${geocoded}`);
if (unmatched.length > 0) {
  console.log(`  Cities with no match in cities.json (${unmatched.length}):`);
  console.log("    " + [...new Set(unmatched)].join(", "));
}

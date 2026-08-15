// ================================================================
//  MAPA - Leaflet + Marcadores + Heatmap
// ================================================================

let cityMarkersLayer = null;
let mapMarkersLayer = null;
let heatmapLayer = null;

const MAP_MARKER_META = {
  person: {
    emoji: "🫂",
    data: () => personsData,
    tabArgs: () => [tabPersonasBtn, tabPersonas, "person"],
  },
  pet: {
    emoji: "🐕",
    data: () => petsData,
    tabArgs: () => [tabPetsBtn, tabPets, "pet"],
  },
  building: {
    emoji: "🏢",
    data: () => buildingsData,
    tabArgs: () => [tabEdificiosBtn, tabEdificios, "building"],
  },
};

const MAP_SIDEBAR_EMOJI = { person: "🫂", pet: "🐕" };

// Which marker types are currently visible on the map. Empty set = all.
const mapTypeFilter = new Set(["person", "pet", "building"]);

function isTypeVisible(type) {
  return mapTypeFilter.has(type);
}

// Flatten all visible map item types into a single [{ item, type }] list.
// Used by the sidebar and off-screen arrows (persons + pets).
function visibleMapItems() {
  return Object.entries(MAP_MARKER_META)
    .filter(([type]) => isTypeVisible(type))
    .flatMap(([type, meta]) =>
      meta.data().map((item) => ({ item, type })),
    );
}

// Marker circle color is driven by the item's status, not its type.
// Single source of truth for status semantics (color, CSS class, label).
// Exposed on window so index.js can also use it.
const STATUS_META = {
  desaparecido: { color: "#d64545", cssClass: "desaparecido", label: "desaparecido", icon: "❓" },
  encontrado:   { color: "#3fa34d", cssClass: "encontrado", label: "encontrado", icon: "✅" },
  seguro:       { color: "#3fa34d", cssClass: "seguro", label: "seguro", icon: "🫶" },
  danado:       { color: "#e0a63c", cssClass: "danado", label: "dañado", icon: "⚠️" },
  colapsado:    { color: "#b53838", cssClass: "colapsado", label: "colapsado", icon: "💥" },
  acopio:       { color: "#ffffff", cssClass: "acopio", label: "📦 acopio", icon: "📦" },
  angel:        { color: "#add8e6", cssClass: "angel", label: "👼", icon: "👼" },
};
window.STATUS_META = STATUS_META;

function statusColor(status) {
  return STATUS_META[status]?.color || "#d64545";
}

function statusIcon(status) {
  return STATUS_META[status]?.icon || "❓";
}

// Resolve an item's coordinates: prefer its own lat/lng, otherwise fall
// back to the city's coordinates (if we know them). Returns [lat, lng].
function resolveItemCoords(item) {
  if (item.lat != null && item.lng != null) return [item.lat, item.lng];
  if (item.city && cityCoordinates[item.city])
    return cityCoordinates[item.city];
  return [null, null];
}

function renderCityMarkers() {
  if (!window.sismoMap) return;
  if (cityMarkersLayer) {
    cityMarkersLayer.clearLayers();
  } else {
    cityMarkersLayer = L.layerGroup().addTo(window.sismoMap);
  }

  Object.entries(cityCoordinates).forEach(([name, coords]) => {
    L.marker(coords, {
      icon: L.divIcon({
        className: "city-pin",
        html: `<div class="city-pin-dot"></div>`,
        iconSize: [14, 14],
        iconAnchor: [7, 7],
      }),
      zIndexOffset: -1000,
    })
      .addTo(cityMarkersLayer)
      .bindTooltip(name, { permanent: false, direction: "top" });
  });
}

function mapSidebarCardHtml(item, type) {
  const found = item.status === "encontrado";
  const isAngel = item.status === "angel";
  const photo = item.photo_url || item.image;
  const placeholder = type === "pet" ? PET_PLACEHOLDER : PERSON_PLACEHOLDER;
  const imgHtml = isAngel
    ? `<img class="map-sidebar-photo" style="opacity:0.62;" src="angel.png" alt="" />`
    : photo
      ? `<img class="map-sidebar-photo" src="${escapeHtml(photo)}" alt="" />`
      : `<img class="map-sidebar-photo" style="opacity:0.62;" src="${placeholder}" alt="" />`;
  const location = item.city || item.location || "Sin ubicación";
  const emoji = isAngel ? "👼" : MAP_SIDEBAR_EMOJI[type];
  return `
    <div class="map-sidebar-card ${found ? "encontrado" : ""}" data-id="${item.id}" data-type="${type}" style="${isAngel ? "background:lightblue;opacity:0.62;" : ""}">
      ${imgHtml}
      <div class="map-sidebar-info">
        <span class="map-sidebar-name">${emoji} ${escapeHtml(item.name)}</span>
        <span class="map-sidebar-meta">${escapeHtml(location)}</span>
      </div>
      <span style="display:none"class="status-tag ${found ? "encontrado" : ""}">${item.status}</span>
    </div>
  `;
}

// Number of off-screen items to point at with border arrows.
const OFFSCREEN_ARROW_COUNT = 5;

// Compute where a line from the viewport center to an off-screen point
// crosses the viewport rectangle border. Returns {x, y, angle} in pixels.
// `inset` pulls the point inward from the border so the arrow circle sits
// fully inside the map instead of straddling the edge.
// `insets` (top/right/bottom/left) shrink the border rectangle so arrows
// don't land behind the sidebar (left) or the driver bar (bottom).
function borderIntersection(
  center,
  point,
  width,
  height,
  inset = 20,
  insets = {},
) {
  const dx = point.x - center.x;
  const dy = point.y - center.y;
  if (dx === 0 && dy === 0) return { x: center.x, y: center.y, angle: 0 };

  const top = (insets.top || 0) + inset;
  const right = (insets.right || 0) + inset;
  const bottom = (insets.bottom || 0) + inset;
  const left = (insets.left || 0) + inset;

  // Distance from center to each border, shrunk by the corresponding inset.
  const distX = dx > 0 ? width / 2 - right : width / 2 - left;
  const distY = dy > 0 ? height / 2 - bottom : height / 2 - top;
  const scaleX = dx !== 0 ? distX / Math.abs(dx) : Infinity;
  const scaleY = dy !== 0 ? distY / Math.abs(dy) : Infinity;
  const scale = Math.min(scaleX, scaleY);

  const x = center.x + dx * scale;
  const y = center.y + dy * scale;
  const angle = (Math.atan2(dy, dx) * 180) / Math.PI;
  return { x, y, angle };
}

function renderOffscreenArrows() {
  const container = document.getElementById("offscreenArrows");
  if (!container || !window.sismoMap) return;

  const map = window.sismoMap;
  const size = map.getSize();
  const center = map.latLngToContainerPoint(map.getCenter());
  const bounds = map.getBounds();

  // Measure the overlays that sit on top of the map so arrows don't get
  // hidden behind them. The sidebar is fixed to the left (desktop) or the
  // bottom (mobile); the driver bar is always fixed to the bottom; the top
  // bar is sticky at the top.
  const sidebar = document.getElementById("mapSidebar");
  const driver = document.getElementById("driver");
  const topBar = document.querySelector(".top-bar");
  const insets = { top: 0, right: 0, bottom: 0, left: 0 };
  const mapRect = map.getContainer().getBoundingClientRect();
  if (sidebar && sidebar.style.display !== "none") {
    const rect = sidebar.getBoundingClientRect();
    // Only account for the sidebar on the side where it actually overlaps
    // the map (left on desktop, bottom on mobile).
    if (rect.width < rect.height) {
      insets.left = Math.max(0, rect.right - mapRect.left);
    } else {
      insets.bottom = Math.max(0, mapRect.bottom - rect.top);
    }
  }
  if (driver) {
    const rect = driver.getBoundingClientRect();
    insets.bottom = Math.max(insets.bottom, mapRect.bottom - rect.top);
  }
  if (topBar) {
    const rect = topBar.getBoundingClientRect();
    insets.top = Math.max(0, rect.bottom - mapRect.top);
  }

  const all = visibleMapItems();

  // Off-screen items with coordinates, sorted by distance from center.
  const offscreen = all
    .map(({ item, type }) => ({ item, type, coords: resolveItemCoords(item) }))
    .filter(
      ({ coords }) =>
        coords[0] != null && coords[1] != null && !bounds.contains(coords),
    )
    .map(({ item, type, coords }) => {
      const p = map.latLngToContainerPoint(coords);
      const dist = Math.hypot(p.x - center.x, p.y - center.y);
      return { item, type, point: p, dist };
    })
    .sort((a, b) => a.dist - b.dist)
    .slice(0, OFFSCREEN_ARROW_COUNT);

  // Compute each arrow's border position, then drop arrows that land too
  // close to an already-kept arrow at the border. Items are sorted by
  // distance from center, so the closest one wins when they overlap.
  const MIN_ARROW_GAP = 24; // px between arrow centers at the border
  const kept = [];
  for (const { item, type, point } of offscreen) {
    const { x, y, angle } = borderIntersection(
      center,
      point,
      size.x,
      size.y,
      20,
      insets,
    );
    const overlaps = kept.some(
      (k) => Math.hypot(k.x - x, k.y - y) < MIN_ARROW_GAP,
    );
    if (overlaps) continue;
    kept.push({ item, type, x, y, angle });
  }

  container.innerHTML = kept
    .map(({ item, type, x, y, angle }) => {
      const emoji = MAP_SIDEBAR_EMOJI[type] || "📍";
      return `
        <div class="offscreen-arrow"
             style="left:${x}px; top:${y}px;"
             data-id="${item.id}" data-type="${type}"
             title="${escapeHtml(item.name)}">
          <span class="arrow-glyph" style="transform:rotate(${angle}deg);">➤</span>
        </div>`;
    })
    .join("");
}

function renderMapSidebar() {
  const listEl = document.getElementById("mapSidebarList");
  if (!listEl) return;
  const all = visibleMapItems()
    .map((entry) => ({
      ...entry,
      ts: new Date(entry.item.created_at).getTime(),
    }))
    .sort((a, b) => b.ts - a.ts);

  // Filter to items inside the current map viewport. Items without
  // lat/lng are excluded (they can't be placed on the map). Pad the
  // bounds slightly so markers whose icon overhangs the edge (center just
  // outside) are still included.
  const bounds = window.sismoMap ? window.sismoMap.getBounds().pad(0.1) : null;
  const items = bounds
    ? all.filter(({ item }) => {
        const [lat, lng] = resolveItemCoords(item);
        return lat != null && lng != null && bounds.contains([lat, lng]);
      })
    : all;

  if (!items.length) {
    listEl.innerHTML = `<div style="color:#777; font-size:0.78rem; padding:8px;">Sin registros en esta zona.</div>`;
    return;
  }
  listEl.innerHTML = items
    .map(({ item, type }) => mapSidebarCardHtml(item, type))
    .join("");
}

function renderMapMarkers() {
  if (!window.sismoMap) return;
  if (mapMarkersLayer) window.sismoMap.removeLayer(mapMarkersLayer);

  // Single mixed cluster group: persons, pets and buildings cluster
  // together (no per-type separation).
  const clusterGroup = L.markerClusterGroup({
    showCoverageOnHover: false,
    maxClusterRadius: 50,
    spiderfyOnMaxZoom: true,
    iconCreateFunction: (cluster) => {
      const total = cluster.getChildCount();
      const size = total < 10 ? "small" : total < 100 ? "medium" : "large";
      return L.divIcon({
        html: `<div class="cluster-counts">
                 <span class="cluster-emoji">📍</span>
                 <span class="cluster-total">${total}</span>
               </div>`,
        className: `marker-cluster marker-cluster-${size}`,
        iconSize: [40, 40],
        iconAnchor: [20, 20],
      });
    },
  });
  window.sismoMap.addLayer(clusterGroup);

  // Keep a single layer reference for removal on re-render.
  mapMarkersLayer = clusterGroup;

  if (heatmapLayer && window.sismoMap.hasLayer(heatmapLayer)) {
    const points = [];
    Object.entries(MAP_MARKER_META).forEach(([type, { data }]) => {
      if (!isTypeVisible(type)) return;
      data().forEach((item) => {
        const [lat, lng] = resolveItemCoords(item);
        if (lat != null && lng != null) points.push([lat, lng, 1.0]);
      });
    });
    heatmapLayer.setLatLngs(points);
  }

  Object.keys(MAP_MARKER_META).forEach((type) => {
    if (!isTypeVisible(type)) return;
    const { emoji, data } = MAP_MARKER_META[type];
    data().forEach((item) => {
      const [lat, lng] = resolveItemCoords(item);
      if (lat == null || lng == null) return;
      const isAngel = item.status === "angel";
      const markerEmoji = isAngel ? "👼" : emoji;
      const color = statusColor(item.status);
      const sIcon = statusIcon(item.status);
      // Show the status icon alongside the type emoji (e.g. 📦 + 🏢).
      const markerContent = isAngel ? "👼" : `${sIcon}${emoji}`;
      L.marker([lat, lng], {
        type,
        icon: L.divIcon({
          className: "",
          html: `<div style="
                background:${color};
                width:34px;
                height:28px;
                border-radius:14px;
                display:flex;
                align-items:center;
                justify-content:center;
                gap:1px;
                font-size:13px;
                border:2px solid rgba(255,255,255,0.6);
                box-shadow:0 2px 6px rgba(0,0,0,0.4);
              ">${markerContent}</div>`,
          iconSize: [34, 28],
          iconAnchor: [17, 14],
        }),
      }).addTo(clusterGroup).bindPopup(`
              ${markerEmoji} <strong>${escapeHtml(item.name)}</strong><br>
              ${item.status ? `<span class="status-tag ${item.status}">${item.status}</span><br>` : ""}
              <a href="#" onclick="window.__mapPopupOpen('${item.id}','${type}'); return false;">Ver detalle</a>
            `);
    });
  });
}

function initMap() {
  const colombiaCenter = [4.5709, -74.2973];
  const map = L.map("map", {
    center: colombiaCenter,
    zoomControl: false,
    zoom: 6,
    minZoom: 6,
    maxBounds: [
      [-5.0, -82.0],
      [13.0, -66.0],
    ],
    maxBoundsViscosity: 1.0,
  });

  L.control.zoom({ position: "bottomright" }).addTo(map);
  L.tileLayer(
    "https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}",
    {
      ext: "jpg",

      //   attribution:
      //     "Tiles &copy; Esri &mdash; Source: Esri, i-cubed, USDA, USGS, AEX, GeoEye, Getmapping, Aerogrid, IGN, IGP, UPR-EGP, and the GIS User Community",
      //   // attribution:
      //   //   '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors',
    },
  ).addTo(map);

  window.sismoMap = map;

  // Prevent Leaflet from hijacking touch scroll on the sidebar list.
  const sidebarList = document.getElementById("mapSidebarList");
  if (sidebarList) {
    sidebarList.addEventListener("touchstart", (e) => e.stopPropagation());
    sidebarList.addEventListener("touchmove", (e) => e.stopPropagation());
    sidebarList.addEventListener("wheel", (e) => e.stopPropagation());
  }

  renderCityMarkers();
  renderMapMarkers();
  renderMapSidebar();
  renderOffscreenArrows();

  // Re-render the sidebar and off-screen arrows whenever the viewport
  // changes. Arrows are rAF-throttled so the burst of move/zoom events
  // coalesces into one render per frame (max 60fps) instead of running
  // O(n) work on every event. The sidebar (DOM-heavy) only refreshes on
  // moveend/zoomend.
  let offscreenRaf = null;
  const scheduleOffscreenArrows = () => {
    if (offscreenRaf) return;
    offscreenRaf = requestAnimationFrame(() => {
      offscreenRaf = null;
      renderOffscreenArrows();
    });
  };
  map.on("move zoom", scheduleOffscreenArrows);
  map.on("moveend zoomend", () => {
    renderMapSidebar();
    renderOffscreenArrows();
  });

  // Clicking an off-screen arrow flies to that item.
  const arrowsContainer = document.getElementById("offscreenArrows");
  if (arrowsContainer) {
    arrowsContainer.addEventListener("click", (e) => {
      const arrow = e.target.closest(".offscreen-arrow");
      if (!arrow) return;
      const type = arrow.dataset.type;
      const source = type === "pet" ? petsData : personsData;
      const item = source.find((p) => p.id === arrow.dataset.id);
      if (!item) return;
      const [lat, lng] = resolveItemCoords(item);
      if (lat != null && lng != null) {
        map.flyTo([lat, lng], 17);
      }
    });
  }
}

function hideMap() {
  // The map panel is a fixed full-viewport layer; hide it so it doesn't
  // paint behind the other tabs' content.
  document.getElementById("tabMapPanel").style.display = "none";
  document.getElementById("mapSidebar").style.display = "none";
}

function showMap() {
  document.getElementById("tabMapPanel").style.display = "block";
  document.getElementById("mapSidebar").style.display = "flex";
}

// Re-render markers + sidebar after data loads. No-op if the map
// hasn't been initialized yet (initMap runs on first map-tab visit).
function refreshMap() {
  if (!window.sismoMap) return;
  renderMapMarkers();
  renderMapSidebar();
}

function toggleHeatmap(enabled) {
  if (enabled) {
    if (!heatmapLayer) {
      const points = [];
      Object.entries(MAP_MARKER_META).forEach(([type, { data }]) => {
        if (!isTypeVisible(type)) return;
        data().forEach((item) => {
          const [lat, lng] = resolveItemCoords(item);
          if (lat != null && lng != null) points.push([lat, lng, 0.4]);
        });
      });
      heatmapLayer = L.heatLayer(points, {
        radius: 25,
        blur: 15,
        maxZoom: 5,
        minOpacity: 0.05,
        maxOpacity: 0.5,
        gradient: {
          0.2: "rgba(63, 163, 77, 0.15)",
          0.5: "rgba(224, 166, 60, 0.2)",
          0.8: "rgba(214, 100, 69, 0.25)",
          1.0: "rgba(214, 69, 69, 0.3)",
        },
      });
    }
    heatmapLayer.addTo(window.sismoMap);
    // leaflet.heat only redraws on 'zoomend'/'moveend', so during zoom and
    // the post-zoom lerp the canvas lags. A single 'move' handler (which
    // fires during both) with _reset() keeps it in sync. rAF throttling
    // coalesces the burst of events so we don't double-reposition and cause
    // the visible jump.
    if (!window.__heatmapMoveHandler) {
      let raf = null;
      window.__heatmapMoveHandler = () => {
        if (raf) return;
        raf = requestAnimationFrame(() => {
          raf = null;
          if (
            heatmapLayer &&
            window.sismoMap &&
            window.sismoMap.hasLayer(heatmapLayer)
          ) {
            heatmapLayer._reset();
          }
        });
      };
      window.sismoMap.on("move", window.__heatmapMoveHandler);
    }
  } else if (heatmapLayer) {
    window.sismoMap.removeLayer(heatmapLayer);
  }
}

window.__mapPopupOpen = function (id, type) {
  const meta = MAP_MARKER_META[type];
  if (!meta) return;
  const item = meta.data().find((i) => i.id === id);
  if (!item) return;
  // Open the detail modal without switching tabs, so the user stays on
  // the map view.
  openModalForItem(item, type);
};

// Toggle map marker types (people / pets / places).
function wireMapTypeFilter() {
  const container = document.getElementById("mapTypeFilter");
  if (!container) return;
  container.querySelectorAll(".filter-pill[data-map-type]").forEach((pill) => {
    pill.addEventListener("click", () => {
      const type = pill.dataset.mapType;
      if (mapTypeFilter.has(type)) mapTypeFilter.delete(type);
      else mapTypeFilter.add(type);
      pill.classList.toggle("active", mapTypeFilter.has(type));
      // Re-render markers, sidebar, and off-screen arrows.
      if (window.sismoMap) {
        renderMapMarkers();
        renderMapSidebar();
        renderOffscreenArrows();
      }
    });
  });
}

// Event listener para el sidebar del mapa
document.addEventListener("DOMContentLoaded", function () {
  wireMapTypeFilter();
  const mapSidebarList = document.getElementById("mapSidebarList");
  if (mapSidebarList) {
    mapSidebarList.addEventListener("click", (e) => {
      const card = e.target.closest(".map-sidebar-card");
      if (!card) return;
      const type = card.dataset.type;
      const source = type === "pet" ? petsData : personsData;
      const item = source.find((p) => p.id === card.dataset.id);
      if (!item) return;
      const [lat, lng] = resolveItemCoords(item);
      if (lat != null && lng != null && window.sismoMap) {
        window.sismoMap.flyTo([lat, lng], 17);
      }
    });
  }
});

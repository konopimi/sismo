// DIAGNÓSTICO TEMPORAL: verifica si ECharts cargó.
console.log("[sismos] typeof echarts =", typeof echarts);
// Keep CSS variables --top-bar-height and --driver-height in sync with
// the real rendered heights of the top bar and the bottom driver bar.
// These bars are content-driven (they wrap on narrow screens), so we
// measure them live instead of hardcoding a fixed value.
function trackLayoutHeights() {
  const root = document.documentElement;
  const topBar = document.querySelector(".top-bar");
  const driver = document.getElementById("driver");
  const measure = () => {
    if (topBar)
      root.style.setProperty("--top-bar-height", topBar.offsetHeight + "px");
    if (driver)
      root.style.setProperty("--driver-height", driver.offsetHeight + "px");
  };
  measure();
  if (window.ResizeObserver) {
    const ro = new ResizeObserver(measure);
    if (topBar) ro.observe(topBar);
    if (driver) ro.observe(driver);
  } else {
    window.addEventListener("resize", measure);
  }
}
// Mapeo de tipos de pestaña a elementos DOM
const tabMap = {
  person: {
    btn: document.getElementById("tabPersonasBtn"),
    panel: document.getElementById("tabPersonas"),
  },
  pet: {
    btn: document.getElementById("tabPetsBtn"),
    panel: document.getElementById("tabPets"),
  },
  building: {
    btn: document.getElementById("tabEdificiosBtn"),
    panel: document.getElementById("tabEdificios"),
  },
  anuncio: {
    btn: document.getElementById("tabAnunciosBtn"),
    panel: document.getElementById("tabAnuncios"),
  },
  colaborador: {
    btn: document.getElementById("tabColabBtn"),
    panel: document.getElementById("tabColab"),
  },
  map: {
    btn: document.getElementById("tabMapBtn"),
    panel: document.getElementById("tabMapPanel"),
  },
  wiki: {
    btn: document.getElementById("wikiBtn"),
    panel: document.getElementById("tabWiki"),
  },
  sismos: {
    btn: document.getElementById("tabSismosBtn"),
    panel: document.getElementById("tabSismos"),
  },
};
// ================================================================
//  TYPE REGISTRY — single source of truth for all entity type metadata
//  (data array, emoji, DOM refs, API type, create title, render fn)
//  Exposed on window so map.js can also use it.
// ================================================================
const TYPE_REGISTRY = {
  person: {
    data: () => personsData,
    emoji: "🫂",
    apiType: "disappeared",
    createTitle: "Crear persona",
    renderFn: render,
  },
  pet: {
    data: () => petsData,
    emoji: "🐕",
    apiType: "pets",
    createTitle: "Crear mascota",
    renderFn: renderP,
  },
  building: {
    data: () => buildingsData,
    emoji: "🏢",
    apiType: "buildings",
    createTitle: "Crear ubicación",
    renderFn: renderB,
  },
  anuncio: {
    data: () => anunciosData,
    emoji: "📢",
    apiType: "anuncios",
    createTitle: "Crear anuncio",
    renderFn: renderA,
  },
  colaborador: {
    data: () => colabData,
    emoji: "🤝",
    apiType: "collaborators",
    createTitle: "Colaborar",
    renderFn: renderColab,
  },
  donacion: {
    data: () => donacionesData,
    emoji: "🎁",
    apiType: "donaciones",
    createTitle: "Ofrecer donación",
    renderFn: renderDonaciones,
  },
  necesidad: {
    data: () => necesidadesData,
    emoji: "🆘",
    apiType: "necesidades",
    createTitle: "Reportar necesidad",
    renderFn: renderNecesidades,
  },
  logistica: {
    data: () => logisticaData,
    emoji: "🚚",
    apiType: "logistica",
    createTitle: "Crear tarea",
    renderFn: renderLogistica,
  },
  map: {
    renderFn: () => {},
  },
  wiki: {
    renderFn: () => {},
  },
  sismos: {
    renderFn: () => {},
  },
};
window.TYPE_REGISTRY = TYPE_REGISTRY;
// Coordenadas de ciudades principales de Colombia
// (Agrega más ciudades según necesites)
let cityCoordinates = {};
// ================================================================
//  CONFIGURACIÓN
// ================================================================
const isLocalhost =
  location.hostname === "localhost" || location.hostname === "127.0.0.1";
function resolveApiBase() {
  if (!isLocalhost) return "/api";
  const target = localStorage.getItem("sismo_api_target") || "remote";
  return target === "local"
    ? "http://localhost:3000/api"
    : "https://sismoinfo.co/api";
}
const API_BASE = resolveApiBase();
window.API_BASE = API_BASE;
// ================================================================
//  AUTH (login de colaboradores)
//  Token HMAC firmado por el server, guardado en localStorage.
// ================================================================
const AUTH_TOKEN_KEY = "sismo_auth_token";
function getAuthToken() {
  return localStorage.getItem(AUTH_TOKEN_KEY);
}
function setAuthToken(t) {
  localStorage.setItem(AUTH_TOKEN_KEY, t);
}
function clearAuthToken() {
  localStorage.removeItem(AUTH_TOKEN_KEY);
}
async function authFetch(url, opts = {}) {
  const token = getAuthToken();
  if (token) {
    opts.headers = { ...(opts.headers || {}), Authorization: `Bearer ${token}` };
  }
  const res = await fetch(url, opts);
  if (res.status === 401) {
    clearAuthToken();
    showLoginForm();
  }
  return res;
}
function showLoginForm() {
  const loginForm = document.getElementById("loginForm");
  const chatContainer = document.getElementById("chatContainer");
  const privateWrapper = document.getElementById("colabPrivateWrapper");
  if (loginForm) loginForm.style.display = "flex";
  if (chatContainer) chatContainer.style.display = "none";
  if (privateWrapper) privateWrapper.style.display = "none";
  if (colabLoginPrompt) colabLoginPrompt.style.display = "block";
  window.currentUser = null;
  // Hide the collaborators count badge now that we're logged out.
  if (typeof updateTabCounts === "function") updateTabCounts();
}
function showChat() {
  const loginForm = document.getElementById("loginForm");
  const chatContainer = document.getElementById("chatContainer");
  const privateWrapper = document.getElementById("colabPrivateWrapper");
  const connContainer = document.getElementById("connContainer");
  if (loginForm) loginForm.style.display = "none";
  if (chatContainer) {
    chatContainer.style.display = "flex";
  }
  if (connContainer) {
    connContainer.innerHTML = `<div style="text-align:right;font-size:60%;">Conectado como ${escapeHtml(window.currentUser?.chat_name || window.currentUser?.name || "")}</div>`;
}
  if (privateWrapper) privateWrapper.style.display = "flex";
  if (colabLoginPrompt) colabLoginPrompt.style.display = "none";
  if (!colabData.length) loadListColab();
  if (typeof loadListDonaciones === "function" && !donacionesData.length) loadListDonaciones();
  if (typeof loadListNecesidades === "function" && !necesidadesData.length) loadListNecesidades();
  if (typeof loadListLogistica === "function" && !logisticaData.length) loadListLogistica();
  // Show the collaborators count badge now that we're logged in.
  if (typeof updateTabCounts === "function") updateTabCounts();
  // Conectar el chat Matrix (E2E) una vez autenticado.
  startMatrixChat();
}
async function attemptLogin() {
  const identifier = document.getElementById("loginIdentifier")?.value.trim();
  const password = document.getElementById("loginPassword")?.value;
  const errEl = document.getElementById("loginError");
  if (errEl) errEl.textContent = "";
  if (!identifier || !password) {
    if (errEl) errEl.textContent = "Ingresa usuario y contraseña";
    return;
  }
  try {
    const res = await fetch(`${API_BASE}/auth/login`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ identifier, password }),
    });
    const data = await res.json().catch(() => ({}));
    if (!res.ok) {
      if (errEl) errEl.textContent = data.error || "Error al entrar";
      return;
    }
    setAuthToken(data.token);
    window.currentUser = data.user;
    showChat();
  } catch (e) {
    if (errEl) errEl.textContent = "Error de red";
  }
}
(async function initAuth() {
  const loginBtn = document.getElementById("loginBtn");
  if (loginBtn) loginBtn.addEventListener("click", attemptLogin);
  const token = getAuthToken();
  if (!token) return showLoginForm();
  try {
    const res = await authFetch(`${API_BASE}/auth/me`);
    if (res.ok) {
      window.currentUser = await res.json();
      showChat();
    } else {
      showLoginForm();
    }
  } catch {
    showLoginForm();
  }
})();
// ================================================================
//  CHAT MATRIX (E2E)
//  Un solo canal grupal. El backend provisiona la cuenta Matrix en el
//  primer login; el cliente hace login con matrix-js-sdk y genera sus
//  claves E2E en el navegador. El room se crea/une automáticamente.
// ================================================================
const MATRIX_ROOM_ALIAS = "#sismo-general:matrix.sismoinfo.co";
let matrixClient = null;
let matrixRoom = null;
let matrixStarted = false;

function matrixSdk() {
  return window.matrixcs || window.matrix;
}

async function startMatrixChat() {
  if (matrixStarted) return;
  matrixStarted = true;
  const sdk = matrixSdk();
  if (!sdk) {
    renderChatError("Matrix SDK no cargó. Recarga la página.");
    return;
  }
  try {
    // 1. Obtener credenciales Matrix del backend (provisiona si hace falta).
    const res = await authFetch(`${API_BASE}/auth/matrix`, { method: "POST" });
    if (!res.ok) {
      const data = await res.json().catch(() => ({}));
      renderChatError(data.error || "No se pudo conectar al chat.");
      return;
    }
    const { base_url, user_id, password } = await res.json();

    // 2. Login en Matrix (genera claves E2E en el navegador).
    // v28.2.0 usa Olm (cargado por CDN antes del SDK).
    if (window.Olm && typeof window.Olm.init === "function") {
      await window.Olm.init();
    }
    // Login temporal para obtener device_id (necesario para initCrypto).
    // Reutilizamos un device_id persistente para no perder las claves E2E.
    const storedDeviceId = localStorage.getItem("sismo_matrix_device_id");
    const tmpClient = sdk.createClient({ baseUrl: base_url });
    const loginResp = await tmpClient.login("m.login.password", {
      identifier: { type: "m.id.user", user: user_id.split(":")[0].replace("@", "") },
      password,
      ...(storedDeviceId ? { device_id: storedDeviceId } : {}),
    });
    localStorage.setItem("sismo_matrix_device_id", loginResp.device_id);
    // Re-crear el cliente con deviceId + accessToken + userId.
    matrixClient = sdk.createClient({
      baseUrl: base_url,
      accessToken: loginResp.access_token,
      userId: loginResp.user_id,
      deviceId: loginResp.device_id,
    });
    await matrixClient.initCrypto();
    matrixClient.setGlobalErrorOnUnknownDevices(false);
    await matrixClient.startClient({ initialSyncLimit: 20 });

    // 3. Unirse/crear el room grupal.
    matrixRoom = await joinOrCreateRoom();

    // 4. Renderizar la UI del chat.
    renderChatUI();
    bindChatEvents();
  } catch (e) {
    console.error("Matrix chat error:", e);
    renderChatError("Error al conectar el chat: " + (e.message || e));
  }
}

async function joinOrCreateRoom() {
  try {
    const aliasRes = await matrixClient.getRoomIdForAlias(MATRIX_ROOM_ALIAS);
    const roomId = aliasRes.room_id;
    await matrixClient.joinRoom(roomId);
    return roomId;
  } catch (e) {
    // Room no existe: crearlo.
    const { room_id } = await matrixClient.createRoom({
      name: "Sismo General",
      topic: "Canal general de colaboradores de Sismo",
      visibility: "private",
      room_alias_name: "sismo-general",
    });
    return room_id;
  }
}

function renderChatUI() {
  const container = document.getElementById("chatContainer");
  if (!container) return;
  container.style.display = "flex";
  container.innerHTML = `
    <div style="display:flex;flex-direction:column;flex:1;min-height:0;">
      <div id="chatMessages" style="flex:1;overflow-y:auto;padding:10px;display:flex;flex-direction:column;gap:8px;"></div>
      <div style="display:flex;gap:6px;padding:8px;border-top:1px solid rgba(120,120,120,0.3);">
        <input id="chatInput" type="text" placeholder="Escribe un mensaje..." style="flex:1;border-radius:6px;padding:8px;" />
        <button id="chatSendBtn" style="border-radius:6px;padding:8px 14px;">Enviar</button>
      </div>
    </div>
  `;
}

function renderChatError(msg) {
  const container = document.getElementById("chatContainer");
  if (!container) return;
  container.style.display = "flex";
  container.innerHTML = `<div style="padding:20px;color:#f66;">${escapeHtml(msg)}</div>`;
}

function bindChatEvents() {
  const input = document.getElementById("chatInput");
  const sendBtn = document.getElementById("chatSendBtn");
  const send = async () => {
    const text = input.value.trim();
    if (!text || !matrixRoom) return;
    try {
      await matrixClient.sendTextMessage(matrixRoom, text);
      input.value = "";
    } catch (e) {
      console.error("send error:", e);
    }
  };
  sendBtn.addEventListener("click", send);
  input.addEventListener("keydown", (e) => {
    if (e.key === "Enter") send();
  });

  // Escuchar mensajes nuevos.
  matrixClient.on("Room.timeline", (event, room) => {
    if (!room || room.roomId !== matrixRoom) return;
    if (event.getType() !== "m.room.message") return;
    appendMessage(event);
  });

  // Cargar historial inicial (si el room ya está en el store).
  const room = matrixClient.getRoom(matrixRoom);
  if (room) {
    const timeline = room.getLiveTimeline().getEvents();
    timeline.forEach((event) => {
      if (event.getType() === "m.room.message") appendMessage(event);
    });
  }
}

function appendMessage(event) {
  const list = document.getElementById("chatMessages");
  if (!list) return;
  const content = event.getContent();
  const body = content.body || "";
  const sender = event.getSender();
  const isSelf = sender === matrixClient.getUserId();
  const name = isSelf
    ? (window.currentUser?.chat_name || "Tú")
    : sender.split(":")[0].replace("@", "");
  const div = document.createElement("div");
  div.style.cssText = `
    align-self:${isSelf ? "flex-end" : "flex-start"};
    max-width:75%;
    padding:8px 12px;
    border-radius:12px;
    background:${isSelf ? "rgba(63,163,77,0.35)" : "rgba(120,120,120,0.25)"};
  `;
  div.innerHTML = `
    <div style="font-size:70%;opacity:0.7;">${escapeHtml(name)}</div>
    <div style="white-space:pre-wrap;word-break:break-word;">${escapeHtml(body)}</div>
  `;
  list.appendChild(div);
  list.scrollTop = list.scrollHeight;
}

// ================================================================
//  PRIVACY MODAL
// ================================================================
const privacyModal = document.getElementById("privacyModal");
const privacyInfoBtn = document.getElementById("privacyInfoBtn");
if (privacyInfoBtn && privacyModal) {
  const privacyModalShell = Modal({ id: "privacyModal" });
  privacyInfoBtn.addEventListener("click", () => privacyModalShell.open());
}
const CLOUDINARY_CLOUD_NAME = "snaspwdz";
const CLOUDINARY_UPLOAD_PRESET = "sismopinto";
if (isLocalhost) {
  const toggle = document.getElementById("apiTargetToggle");
  const btnRemote = document.getElementById("apiTargetRemote");
  const btnLocal = document.getElementById("apiTargetLocal");
  // The toggle HTML may be commented out; bail if missing.
  if (!toggle || !btnRemote || !btnLocal) {
    // no-op: API target selector is disabled in this build
  } else {
    const activeTarget = localStorage.getItem("sismo_api_target") || "remote";
    toggle.style.display = "flex";
    btnRemote.style.background = activeTarget === "remote" ? "#d64545" : "#333";
    btnLocal.style.background = activeTarget === "local" ? "#d64545" : "#333";
    btnRemote.addEventListener("click", () => {
      localStorage.setItem("sismo_api_target", "remote");
      location.reload();
    });
    btnLocal.addEventListener("click", () => {
      localStorage.setItem("sismo_api_target", "local");
      location.reload();
    });
  }
}
// ================================================================
//  STATUS META — single source of truth for status semantics
//  (color for map markers, CSS class for pills, display label)
//  Defined in map.js and exposed on window.STATUS_META.
//  Helpers here for convenient access.
// ================================================================
function statusMeta(status) {
  return (window.STATUS_META || {})[status] || (window.STATUS_META || {}).desaparecido;
}
function statusColor(status) {
  return statusMeta(status).color;
}
function statusCssClass(status) {
  return statusMeta(status).cssClass;
}
function statusLabel(status) {
  return statusMeta(status).label;
}
// Convert a hex color to its hue angle (0-360) using standard RGB→HSL.
function hexToHue(hex) {
  const m = /^#?([0-9a-f]{6})$/i.exec((hex || "").trim());
  if (!m) return 0;
  const n = parseInt(m[1], 16);
  const r = ((n >> 16) & 255) / 255;
  const g = ((n >> 8) & 255) / 255;
  const b = (n & 255) / 255;
  const max = Math.max(r, g, b);
  const min = Math.min(r, g, b);
  const delta = max - min;
  if (delta === 0) return 0; // achromatic
  let hue;
  if (max === r) hue = ((g - b) / delta) % 6;
  else if (max === g) hue = (b - r) / delta + 2;
  else hue = (r - g) / delta + 4;
  hue *= 60;
  return hue < 0 ? hue + 360 : hue;
}
// Hue of `sepia(100%) saturate(10000%)` applied to white — the base the
// hue-rotate starts from. Derived from the W3C sepia matrix.
const SEPIA_BASE_HUE = 60;
// Build a CSS filter that recolors a grayscale silhouette to the hue of the
// given status color. Standard "black → any color" technique: black → white
// → sepia → saturate to a pure hue → rotate to the target hue.
function placeholderFilter(status) {
  const hue = hexToHue(statusMeta(status).color);
  const rotate = hue - SEPIA_BASE_HUE;
  return `brightness(0) invert(100%) sepia(100%) saturate(10000%) hue-rotate(${rotate.toFixed(1)}deg)`;
}
// ================================================================
//  UTILIDADES
// ================================================================
function escapeHtml(str) {
  const div = document.createElement("div");
  div.textContent = str;
  return div.innerHTML;
}
// Helper para copiar al portapapeles con feedback visual
function copyWithFeedback(btn, text) {
  navigator.clipboard.writeText(text).then(() => {
    const original = btn.textContent;
    btn.textContent = "✅ ¡Copiado!";
    setTimeout(() => { btn.textContent = original; }, 2000);
  }).catch(() => {
    alert("No se pudo copiar. Intenta seleccionar el texto manualmente.");
  });
}
// Factory para previsualización de foto (archivo + URL)
function setupPhotoPreview({ fileInput, urlInput, previewEl, labelEl, placeholderText, urlPreviewEl }) {
  fileInput.addEventListener("change", function() {
    if (previewEl.src) {
      URL.revokeObjectURL(previewEl.src);
      previewEl.src = "";
    }
    if (this.files.length > 0) {
      labelEl.textContent = this.files[0].name;
      previewEl.src = URL.createObjectURL(this.files[0]);
      previewEl.style.display = "inline-block";
    } else {
      labelEl.textContent = placeholderText;
      previewEl.style.display = "none";
      previewEl.src = "";
    }
  });
  urlInput.addEventListener("input", function() {
    const url = this.value.trim();
    if (url && (url.startsWith("http://") || url.startsWith("https://"))) {
      urlPreviewEl.src = url;
      urlPreviewEl.style.display = "inline-block";
      urlPreviewEl.onerror = function() {
        urlPreviewEl.style.display = "none";
        urlPreviewEl.src = "";
      };
    } else {
      urlPreviewEl.style.display = "none";
      urlPreviewEl.src = "";
      urlPreviewEl.onerror = null;
    }
  });
}
function locationLinksHtml(lat, lng, coordLabel) {
  const label = coordLabel || `${lat.toFixed(5)}, ${lng.toFixed(5)}`;
  return `<span class="loc-links">🗺️ ${label}</br> 
        <a href="#" onclick="viewOnAppMap(${lat},${lng});return false;" style="color:#4ea1ff;">Ver aquí</a> ·
        <a href="https://www.google.com/maps?q=${lat},${lng}" target="_blank" rel="noopener" style="color:#4ea1ff;">Google Maps</a> ·
        <a href="https://waze.com/ul?ll=${lat},${lng}&navigate=yes" target="_blank" rel="noopener" style="color:#4ea1ff;">Waze</a></span>`;
}
function haversineDistance(lat1, lng1, lat2, lng2) {
  const R = 6371000;
  const toRad = (deg) => (deg * Math.PI) / 180;
  const dLat = toRad(lat2 - lat1);
  const dLng = toRad(lng2 - lng1);
  const a =
    Math.sin(dLat / 2) ** 2 +
    Math.cos(toRad(lat1)) * Math.cos(toRad(lat2)) * Math.sin(dLng / 2) ** 2;
  return R * 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));
}
function nearbyItemsHtml(lat, lng, excludeId, excludeType) {
  const RADIUS_M = 1000;
  const candidates = Object.entries(TYPE_REGISTRY)
    .filter(([, meta]) => meta.data)
    .flatMap(([type, meta]) =>
      meta.data().map((item) => ({ item, type })),
    );
  const nearby = candidates
    .filter(
      ({ item, type }) =>
        item.lat != null &&
        item.lng != null &&
        !(item.id === excludeId && type === excludeType),
    )
    .map(({ item, type }) => ({
      item,
      type,
      distance: haversineDistance(lat, lng, item.lat, item.lng),
    }))
    .filter(({ distance }) => distance <= RADIUS_M)
    .sort((a, b) => a.distance - b.distance)
    .slice(0, 12);
  if (!nearby.length) {
    return `<div style="color:#666; font-size:0.85rem;">Sin items cercanos (menos de ${RADIUS_M / 1000}km).</div>`;
  }
  return nearby
    .map(({ item, type, distance }) => {
      const emoji = MAP_MARKER_META[type] ? MAP_MARKER_META[type].emoji : "📍";
      const distLabel =
        distance < 1000
          ? `${Math.round(distance)}m`
          : `${(distance / 1000).toFixed(1)}km`;
      return `<div class="nearby-item" data-nearby-id="${item.id}" data-nearby-type="${type}">
          <span>${emoji} ${escapeHtml(item.name)}</span>
          <span class="nearby-dist">${distLabel}</span>
        </div>`;
    })
    .join("");
}
// ================================================================
//  VIRTUALIZACIÓN LIGERA DE LISTAS
//  Renderiza en lotes y agrega más al acercarse al final, en vez de
//  volcar cientos de tarjetas al DOM de una sola vez.
// ================================================================
const VIRTUAL_BATCH_SIZE = 30;
const virtualObservers = new WeakMap();
function renderVirtualList(containerEl, items, cardHtmlFn) {
  const prevObserver = virtualObservers.get(containerEl);
  if (prevObserver) prevObserver.disconnect();
  let rendered = Math.min(VIRTUAL_BATCH_SIZE, items.length);
  containerEl.innerHTML =
    items.slice(0, rendered).map(cardHtmlFn).join("") +
    `<div class="virtual-sentinel"></div>`;
  const sentinel = containerEl.querySelector(".virtual-sentinel");
  if (rendered >= items.length) {
    sentinel.remove();
    return;
  }
  const observer = new IntersectionObserver(
    (entries) => {
      if (!entries[0].isIntersecting) return;
      const next = Math.min(rendered + VIRTUAL_BATCH_SIZE, items.length);
      sentinel.insertAdjacentHTML(
        "beforebegin",
        items.slice(rendered, next).map(cardHtmlFn).join(""),
      );
      rendered = next;
      if (rendered >= items.length) {
        observer.disconnect();
        sentinel.remove();
      }
    },
    { rootMargin: "600px" },
  );
  observer.observe(sentinel);
  virtualObservers.set(containerEl, observer);
}
async function loadExtraLocations(itemId, type) {
  const container = document.getElementById("modalExtraLocationsContainer");
  if (!container) return;
  const itemType = type === "person" ? "disappeared" : "pets";
  try {
    const res = await fetch(
      `${API_BASE}/item-locations?itemId=${itemId}&itemType=${itemType}`,
    );
    const locations = await res.json();
    if (!locations.length) {
      container.innerHTML = `<div style="color:#666; font-size:0.85rem;">Ninguna todavía.</div>`;
      return;
    }
    container.innerHTML = locations
      .map(
        (loc) => `
          <div class="extra-location-item">
            <span>${locationLinksHtml(loc.lat, loc.lng, loc.label)}</span>
            <button type="button" class="btn-small btn-delete" onclick="deleteExtraLocation('${loc.id}','${itemId}','${type}')">✕</button>
          </div>
        `,
      )
      .join("");
  } catch {
    container.innerHTML = `<div style="color:#666; font-size:0.85rem;">No se pudieron cargar.</div>`;
  }
}
async function deleteExtraLocation(locationId, itemId, type) {
  if (!confirm("¿Eliminar esta ubicación adicional?")) return;
  const ok = await adminDelete(`${API_BASE}/item-locations/${locationId}`);
  if (ok) loadExtraLocations(itemId, type);
}
function viewOnAppMap(lat, lng) {
  closeModal();
  switchTab(tabMapBtn, tabMapPanel, "map");
  setTimeout(() => {
    if (window.sismoMap)
      window.sismoMap.flyTo([lat, lng], 16, { duration: 0.3 });
  }, 250);
}
function commentBadgeHtml(item) {
  if (!item.last_comment) return "";
  const preview =
    item.last_comment.length > 60
      ? item.last_comment.slice(0, 60) + "…"
      : item.last_comment;
  const count = item.comment_count > 1 ? ` (${item.comment_count})` : "";
  return `<div class="comment-badge" title="${escapeHtml(item.last_comment)}">💬${count} ${escapeHtml(preview)}</div>`;
}
function normalize(str) {
  return (str || "")
    .toLowerCase()
    .normalize("NFD")
    .replace(/[\u0300-\u036f]/g, "");
}
function fuzzyMatch(query, target) {
  query = normalize(query);
  target = normalize(target);
  if (!query) return true;
  let qi = 0;
  for (let ti = 0; ti < target.length && qi < query.length; ti++) {
    if (target[ti] === query[qi]) qi++;
  }
  return qi === query.length;
}
function fuzzyFilter(query, items) {
  if (!query.trim()) return items;
  return items.filter(
    (item) =>
      fuzzyMatch(query, item.name) ||
      fuzzyMatch(query, item.location || "") ||
      fuzzyMatch(query, item.city || ""),
  );
}
async function adminDelete(url) {
  const key = prompt("Clave de administrador:");
  if (!key) return false;
  const r = await fetch(url, {
    method: "DELETE",
    headers: { "x-admin-key": key },
  });
  if (r.status === 403) {
    alert("Clave incorrecta");
    return false;
  }
  return true;
}
// ================================================================
//  CARGA DE CIUDADES
// ================================================================
fetch("/cities.json")
  .then((r) => r.json())
  .then((cities) => {
    // Build the coordinate map from fetched data.
    cities.forEach((c) => {
      cityCoordinates[c.name] = [c.lat, c.lng];
    });
    // Populate all city <select> elements with the same options.
    const citySelectIds = [
      "cityInput",
      "cityInputP",
      "cityInputB",
      "cityInputColab",
      "citySelectorMap",
    ];
    const opts = cities
      .map((c) => `<option value="${c.name}">${c.name}</option>`)
      .join("");
    citySelectIds.forEach((id) => {
      const el = document.getElementById(id);
      if (el) el.insertAdjacentHTML("beforeend", opts);
    });
    // Reflect any already-restored sharedCity into the map selector (and
    // the other selectors) now that their options exist.
    if (typeof syncCitySelects === "function") syncCitySelects();
    // El selector del mapa vuela a la ciudad elegida Y actualiza el filtro
    // de ciudad compartido (mantiene todos los selectores en sincronía).
    const citySelectorMap = document.getElementById("citySelectorMap");
    if (citySelectorMap) {
      citySelectorMap.addEventListener("change", function() {
        const city = this.value;
        // Update the shared city filter (and sync every other selector).
        sharedCity = city || "";
        syncCitySelects();
        if (currentRenderFn) currentRenderFn();
        saveFilters();
        // Fly the map to the chosen city.
        if (city && window.sismoMap && cityCoordinates[city]) {
          const coords = cityCoordinates[city];
          window.sismoMap.flyTo(coords, 13, { duration: 0.3 });
        }
      });
    }
    // Re-run map rendering now that coordinates are loaded (handles the
    // timing issue where initMap runs before the async fetch resolves).
    if (window.sismoMap) {
      if (typeof renderCityMarkers === "function") renderCityMarkers();
      if (typeof renderMapMarkers === "function") renderMapMarkers();
      if (typeof renderMapSidebar === "function") renderMapSidebar();
      if (typeof renderOffscreenArrows === "function") renderOffscreenArrows();
    }
  })
  .catch(() => console.error("No se pudo cargar cities.json"));
// ================================================================
//  SUBIDA A CLOUDINARY
// ================================================================
async function uploadPersonPhoto(personId, file) {
  const formData = new FormData();
  formData.append("file", file);
  formData.append("upload_preset", CLOUDINARY_UPLOAD_PRESET);
  try {
    const res = await fetch(
      `https://api.cloudinary.com/v1_1/${CLOUDINARY_CLOUD_NAME}/image/upload`,
      {
        method: "POST",
        body: formData,
      },
    );
    const data = await res.json();
    if (!data.secure_url) throw new Error("No se obtuvo secure_url");
    await fetch(`${API_BASE}/disappeared/${personId}/photo`, {
      method: "PATCH",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ photo_url: data.secure_url }),
    });
    return data.secure_url;
  } catch (err) {
    alert(
      "El reporte se guardó, pero la foto no se pudo subir. Intenta agregarla de nuevo abriendo el reporte.",
    );
    console.error(err);
    return null;
  }
}
// ================================================================
//  COPIAR LINK
// ================================================================
document.getElementById("copyLinkBtn").addEventListener("click", () => {
  copyWithFeedback(document.getElementById("copyLinkBtn"), "https://sismoinfo.co");
});
function copyItemLink(id, type, btn) {
  const url = new URL(window.location);
  url.searchParams.set("id", id);
  url.searchParams.set("type", type);
  copyWithFeedback(btn, url.toString());
}
// ================================================================
//  COMPARTIR (Web Share API + fallback WhatsApp)
// ================================================================
async function shareContent(title, text, url) {
  if (navigator.share) {
    try {
      await navigator.share({ title, text, url });
      return;
    } catch (err) {
      if (err && err.name === "AbortError") return;
    }
  }
  const waText = encodeURIComponent(url ? `${text}\n${url}` : text);
  window.open(`https://wa.me/?text=${waText}`, "_blank");
}
document.getElementById("shareAppBtn").addEventListener("click", () => {
  shareContent(
    "Sismo",
    "Reporta o busca personas, mascotas y edificios afectados, y revisa los anuncios de la comunidad.",
    "https://sismoinfo.co",
  );
});
// ================================================================
//  TABS
// ================================================================
const tabPersonasBtn = document.getElementById("tabPersonasBtn");
const tabPetsBtn = document.getElementById("tabPetsBtn");
const tabEdificiosBtn = document.getElementById("tabEdificiosBtn");
const tabAnunciosBtn = document.getElementById("tabAnunciosBtn");
const tabColabBtn = document.getElementById("tabColabBtn");
const tabMapBtn = document.getElementById("tabMapBtn");
const wikiBtn = document.getElementById("wikiBtn");
const tabPersonas = document.getElementById("tabPersonas");
const tabPets = document.getElementById("tabPets");
const tabEdificios = document.getElementById("tabEdificios");
const tabAnuncios = document.getElementById("tabAnuncios");
const tabColab = document.getElementById("tabColab");
const tabMapPanel = document.getElementById("tabMapPanel");
const tabWiki = document.getElementById("tabWiki");
const tabSismosBtn = document.getElementById("tabSismosBtn");
const tabSismos = document.getElementById("tabSismos");
let currentTabType = "person"; // track active tab for map cleanup
window.currentTabType = currentTabType;
function switchTab(activeBtn, activePanel, tabType) {
  // If leaving the map tab, hide the map container
  if (currentTabType === "map" && tabType !== "map") {
    hideMap();
  }
  currentTabType = tabType;
  window.currentTabType = currentTabType;
  // Desactivar todos los botones
  [
    tabPersonasBtn,
    tabPetsBtn,
    tabEdificiosBtn,
    tabAnunciosBtn,
    tabColabBtn,
    tabMapBtn,
    wikiBtn,
    tabSismosBtn,
  ].forEach((b) => b && b.classList.remove("active"));
  // Activar el botón seleccionado
  activeBtn.classList.add("active");
  // Ocultar todos los paneles
  [
    tabPersonas,
    tabPets,
    tabEdificios,
    tabAnuncios,
    tabColab,
    tabMapPanel,
    tabWiki,
    tabSismos,
  ].forEach((p) => p && p.classList.remove("active"));
  // Mostrar el panel seleccionado
  activePanel.classList.add("active");
  // Render function from TYPE_REGISTRY (single source of truth).
  currentRenderFn = TYPE_REGISTRY[tabType]?.renderFn || (() => {});
  // Side effects specific to certain tabs.
  if (tabType === "map") {
    // Show the panel immediately (optimistic), then init/render the map on
    // the next frame so the tab highlight + panel switch paint first.
    showMap();
    // Reset the map search query (the search bar was cleared above).
    if (typeof window.setMapSearchQuery === "function") {
      window.setMapSearchQuery("");
    }
    requestAnimationFrame(() => {
      // Crear el mapa una sola vez (heavy: tiles + markers + sidebar).
      // initMap() already renders markers/sidebar, so only re-render when
      // the map already existed (subsequent visits).
      const alreadyInit = !!window.sismoMap;
      if (!alreadyInit) {
        initMap();
      }
      if (window.sismoMap) {
        window.sismoMap.invalidateSize();
        if (alreadyInit) renderMapMarkers();
        // Fly to the shared city filter if one is selected (keeps the map
        // in sync with a city chosen in another tab).
        if (sharedCity && cityCoordinates[sharedCity]) {
          window.sismoMap.flyTo(cityCoordinates[sharedCity], 13, { duration: 0.3 });
        }
      }
    });
  } else if (tabType === "sismos") {
    window.initSismos();
    // Reset sismos location filter on tab switch
    if (typeof window.setSismosLocationFilter === "function") {
      window.setSismosLocationFilter("");
    }
  }
  // Re-render the newly active list so shared filters (city/search) apply.
  // map/sismos/wiki manage their own rendering; list tabs use currentRenderFn.
  // Defer to the next frame so the tab highlight + panel switch paint first
  // (optimistic UI: instant feedback, list fills in a frame later).
  // Capture the render fn now so rapid tab switches don't run the wrong one.
  const renderFn = currentRenderFn;
  if (renderFn && tabType !== "map" && tabType !== "sismos" && tabType !== "wiki") {
    requestAnimationFrame(() => renderFn());
  }
  // Actualizar la URL con el parámetro "tab" (sin recargar)
  const url = new URL(window.location);
  if (tabType) {
    url.searchParams.set("tab", tabType);
  } else {
    url.searchParams.delete("tab");
  }
  window.history.replaceState({}, "", url);
  // Persist the active tab (and current search/filters) for reload.
  saveFilters();
}
tabPersonasBtn.addEventListener("click", () =>
  switchTab(tabPersonasBtn, tabPersonas, "person"),
);
tabPetsBtn.addEventListener("click", () =>
  switchTab(tabPetsBtn, tabPets, "pet"),
);
tabEdificiosBtn.addEventListener("click", () =>
  switchTab(tabEdificiosBtn, tabEdificios, "building"),
);
tabAnunciosBtn.addEventListener("click", () =>
  switchTab(tabAnunciosBtn, tabAnuncios, "anuncio"),
);
tabColabBtn.addEventListener("click", () =>
  switchTab(tabColabBtn, tabColab, "colaborador"),
);
tabMapBtn.addEventListener("click", () =>
  switchTab(tabMapBtn, tabMapPanel, "map"),
);
wikiBtn.addEventListener("click", () => switchTab(wikiBtn, tabWiki, "wiki"));
tabSismosBtn.addEventListener("click", () =>
  switchTab(tabSismosBtn, tabSismos, "sismos"),
);
// ================================================================
//  CREAR MODAL
//  Unified modal hosting all creation forms. Only the form matching
//  the requested type is shown; the others stay hidden.
// ================================================================
const crearModal = document.getElementById("crearModal");
const crearModalTitle = document.getElementById("crearModalTitle");
const crearFormWraps = crearModal.querySelectorAll(".crear-form-wrap");
// CREAR_META derived from TYPE_REGISTRY
const CREAR_META = {};
for (const [type, meta] of Object.entries(TYPE_REGISTRY)) {
  if (meta.createTitle) CREAR_META[type] = { title: meta.createTitle };
}
const crearModalShell = Modal({ id: "crearModal" });
function openCrearModal(type) {
  if (!CREAR_META[type]) type = "person";
  crearModalTitle.textContent = CREAR_META[type].title;
  crearFormWraps.forEach((w) => {
    w.classList.toggle("active", w.dataset.type === type);
  });
  crearModalShell.open();
}
function closeCrearModal() {
  crearModalShell.close();
}
function openCrearModalForActiveTab() {
  const activeBtn = document.querySelector(".tab-btn.active");
  let type = "person";
  if (activeBtn) {
    const id = activeBtn.id;
    if (id === "tabPetsBtn") type = "pet";
    else if (id === "tabEdificiosBtn") type = "building";
    else if (id === "tabAnunciosBtn") type = "anuncio";
    else if (id === "tabColabBtn") type = "colaborador";
    // wiki tab has no creation form -> default to person
  }
  openCrearModal(type);
}
// Top (+) CREAR!!! button: open modal for the currently active tab.
document
  .getElementById("crear")
  .addEventListener("click", () => openCrearModalForActiveTab());
// ================================================================
//  MODAL (detail)
//  Backed by the reusable Modal shell (modal.js). openModalForItem and
//  closeModal stay as globals because map.js and generated onclick
//  strings call them directly.
// ================================================================
const modal = document.getElementById("modal");
const modalTitle = document.getElementById("modalTitle");
const modalStatus = document.getElementById("modalStatus");
const modalMeta = document.getElementById("modalMeta");
const modalBody = document.getElementById("modalBody");
const modalActions = document.getElementById("modalActions");
let currentItemId = null;
let currentItemType = null; // modal type: "person" | "pet" | "building" | "colaborador" | "anuncio"
let currentCommentType = null; // API comment type: "disappeared" | "pets" | "buildings" | "collaborators" | "anuncios"
// Cleanup performed every time the detail modal closes.
function resetDetailModal() {
  modalTitle.textContent = "";
  modalStatus.innerHTML = "";
  modalMeta.innerHTML = "";
  modalBody.innerHTML = "";
  modalActions.innerHTML = "";
  document.getElementById("commentsList").innerHTML = "";
  document.getElementById("commentInput").value = "";
  currentItemId = null;
  currentItemType = null;
  currentCommentType = null;
  if (modalMiniMap) {
    modalMiniMap.remove();
    modalMiniMap = null;
  }
  const url = new URL(window.location);
  url.searchParams.delete("id");
  url.searchParams.delete("type");
  window.history.replaceState({}, "", url);
}
const detailModal = Modal({ id: "modal", onClose: resetDetailModal });
function closeModal() {
  detailModal.close();
}
const modalRandom = document.getElementById("modalRandom");
modalRandom.addEventListener("click", () => {
  if (currentItemType && currentItemId)
    showRandomItem(currentItemType, currentItemId);
});
document.addEventListener("keydown", (e) => {
  if (e.key !== "Escape") return;
  // Escape handling is centralized in modal.js (wireEscape). All three
  // modals (crear, detail, map) are now backed by Modal shells.
});
// ----- Selector de ubicación en mapa -----
const CALI_CENTER = [3.4516, -76.532];
let mapPickerContext = null; // "person" | "pet" | "building" | "modalItem"
const mapPickerLat = { person: null, pet: null, building: null };
const mapPickerLng = { person: null, pet: null, building: null };
let modalLocationTarget = null; // {id, type} cuando se edita ubicación desde el modal
let modalExtraLocationTarget = null; // {id, type} cuando se agrega una ubicación adicional
let leafletMap = null;
let leafletMarker = null;
let modalMiniMap = null;
const mapModal = document.getElementById("mapModal");
const mapPickerConfirm = document.getElementById("mapPickerConfirm");
const mapPickerLatInput = document.getElementById("mapPickerLatInput");
const mapPickerLngInput = document.getElementById("mapPickerLngInput");
const mapPickerGoBtn = document.getElementById("mapPickerGoBtn");
const mapModalShell = Modal({ id: "mapModal" });
// Build the context-info block (photo + emoji + name + subtitle) for the
// map picker when editing an item's location. Returns { target, contextHtml }.
function mapPickerContextInfo(targetRef, subtitle) {
  const dataArr =
    targetRef.type === "building" ? buildingsData
    : targetRef.type === "person" ? personsData
    : petsData;
  const target = dataArr.find((i) => i.id === targetRef.id);
  if (!target) return { target: null, contextHtml: "" };
  const emoji = MAP_MARKER_META[targetRef.type]?.emoji || "📍";
  const photo = target.photo_url || target.image;
  // Derive a useful subtitle from the item's own data when none is given.
  const derivedSubtitle =
    subtitle ||
    (target.location
      ? escapeHtml(target.location)
      : target.city
        ? escapeHtml(target.city)
        : target.lat != null
          ? "Ubicación actual en el mapa"
          : "Sin ubicación todavía");
  const contextHtml = `
        ${photo ? `<img src="${escapeHtml(photo)}" alt="" style="width:36px;height:36px;border-radius:8px;object-fit:cover;flex:0 0 auto;" />` : ""}
        <span style="font-weight:600;">${emoji} ${escapeHtml(target.name)}</span>
        <span style="color:#999;">${derivedSubtitle}</span>
      `;
  return { target, contextHtml };
}
function openMapPicker(context) {
  mapPickerContext = context;
  mapModalShell.open();
  if (!leafletMap) {
    leafletMap = L.map("mapPickerContainer").setView(CALI_CENTER, 13);
    L.tileLayer("https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png", {
      attribution: "&copy; OpenStreetMap",
    }).addTo(leafletMap);
    leafletMarker = L.marker(CALI_CENTER, { draggable: true }).addTo(
      leafletMap,
    );
    leafletMap.on("click", (e) => {
      leafletMarker.setLatLng(e.latlng);
      syncPickerInputs(e.latlng.lat, e.latlng.lng);
    });
    leafletMarker.on("dragend", () => {
      const ll = leafletMarker.getLatLng();
      syncPickerInputs(ll.lat, ll.lng);
    });
  }
  let existingLat = mapPickerLat[context];
  let existingLng = mapPickerLng[context];
  const mapPickerContextEl = document.getElementById("mapPickerContext");
  mapPickerContextEl.style.display = "none";
  mapPickerContextEl.innerHTML = "";
  // Show a context hint for the create-form contexts too, so the user
  // knows which item type they are placing on the map.
  const createContextLabels = {
    person: "🫂 Ubicación de la persona",
    pet: "🐕 Ubicación de la mascota",
    building: "🏢 Ubicación del edificio",
  };
  if (createContextLabels[context]) {
    mapPickerContextEl.innerHTML = `<span style="font-weight:600;">${createContextLabels[context]}</span>`;
    mapPickerContextEl.style.display = "flex";
  }
  if (context === "modalItem" && modalLocationTarget) {
    const { target, contextHtml } = mapPickerContextInfo(
      modalLocationTarget,
      null,
    );
    if (target && target.lat != null && target.lng != null) {
      existingLat = target.lat;
      existingLng = target.lng;
    }
    if (target) {
      mapPickerContextEl.innerHTML = contextHtml;
      mapPickerContextEl.style.display = "flex";
    }
  } else if (context === "modalItemExtra" && modalExtraLocationTarget) {
    const { target, contextHtml } = mapPickerContextInfo(
      modalExtraLocationTarget,
      "Marca otra ubicación donde pudo haber sido visto/a",
    );
    existingLat = null;
    existingLng = null;
    if (target) {
      mapPickerContextEl.innerHTML = contextHtml;
      mapPickerContextEl.style.display = "flex";
    }
  }
  if (existingLat != null && existingLng != null) {
    leafletMarker.setLatLng([existingLat, existingLng]);
    leafletMap.setView([existingLat, existingLng], 15);
    syncPickerInputs(existingLat, existingLng);
  } else if (navigator.geolocation) {
    navigator.geolocation.getCurrentPosition(
      (pos) => {
        const ll = [pos.coords.latitude, pos.coords.longitude];
        leafletMarker.setLatLng(ll);
        leafletMap.setView(ll, 15);
        syncPickerInputs(ll[0], ll[1]);
      },
      () => {
        syncPickerInputs(CALI_CENTER[0], CALI_CENTER[1]);
      },
    );
  } else {
    syncPickerInputs(CALI_CENTER[0], CALI_CENTER[1]);
  }
  // Leaflet necesita recalcular el tamaño una vez el modal ya es visible.
  setTimeout(() => leafletMap.invalidateSize(), 50);
}
function closeMapPicker() {
  mapModalShell.close();
  mapPickerContext = null;
  mapSearchInput.value = "";
  mapSearchResults.style.display = "none";
  mapSearchResults.innerHTML = "";
}
function syncPickerInputs(lat, lng) {
  mapPickerLatInput.value = lat.toFixed(6);
  mapPickerLngInput.value = lng.toFixed(6);
}
mapPickerGoBtn.addEventListener("click", () => {
  const lat = parseFloat(mapPickerLatInput.value);
  const lng = parseFloat(mapPickerLngInput.value);
  if (
    !Number.isFinite(lat) ||
    !Number.isFinite(lng) ||
    lat < -90 ||
    lat > 90 ||
    lng < -180 ||
    lng > 180
  ) {
    alert(
      "Coordenadas inválidas. Latitud entre -90 y 90, longitud entre -180 y 180.",
    );
    return;
  }
  leafletMarker.setLatLng([lat, lng]);
  leafletMap.setView([lat, lng], 15);
});
const mapSearchInput = document.getElementById("mapSearchInput");
const mapSearchResults = document.getElementById("mapSearchResults");
let mapSearchDebounce = null;
let mapSearchAbort = null;
mapSearchInput.addEventListener("input", () => {
  clearTimeout(mapSearchDebounce);
  const q = mapSearchInput.value.trim();
  if (q.length < 3) {
    mapSearchResults.style.display = "none";
    mapSearchResults.innerHTML = "";
    return;
  }
  mapSearchDebounce = setTimeout(() => runMapSearch(q), 500);
});
async function runMapSearch(query) {
  if (mapSearchAbort) mapSearchAbort.abort();
  mapSearchAbort = new AbortController();
  try {
    const url = `https://nominatim.openstreetmap.org/search?format=json&addressdetails=0&limit=6&countrycodes=co&q=${encodeURIComponent(query)}`;
    const res = await fetch(url, {
      signal: mapSearchAbort.signal,
      headers: { "Accept-Language": "es" },
    });
    const results = await res.json();
    renderMapSearchResults(results);
  } catch (err) {
    if (err.name !== "AbortError") console.error(err);
  }
}
function renderMapSearchResults(results) {
  if (!results.length) {
    mapSearchResults.innerHTML = `<div class="map-search-result" style="color:#666; cursor:default;">Sin resultados</div>`;
    mapSearchResults.style.display = "block";
    return;
  }
  mapSearchResults.innerHTML = results
    .map(
      (r, i) =>
        `<div class="map-search-result" data-index="${i}">${escapeHtml(r.display_name)}</div>`,
    )
    .join("");
  mapSearchResults.style.display = "block";
  mapSearchResults
    .querySelectorAll(".map-search-result[data-index]")
    .forEach((el) => {
      el.addEventListener("click", () => {
        const r = results[el.dataset.index];
        const lat = parseFloat(r.lat);
        const lng = parseFloat(r.lon);
        leafletMarker.setLatLng([lat, lng]);
        leafletMap.setView([lat, lng], 16);
        syncPickerInputs(lat, lng);
        mapSearchResults.style.display = "none";
        mapSearchInput.value = r.display_name;
      });
    });
}
document.addEventListener("click", (e) => {
  if (!mapSearchResults.contains(e.target) && e.target !== mapSearchInput) {
    mapSearchResults.style.display = "none";
  }
});
mapPickerConfirm.addEventListener("click", async () => {
  if (!mapPickerContext) return;
  const typedLat = parseFloat(mapPickerLatInput.value);
  const typedLng = parseFloat(mapPickerLngInput.value);
  const lat = Number.isFinite(typedLat)
    ? typedLat
    : leafletMarker
      ? leafletMarker.getLatLng().lat
      : null;
  const lng = Number.isFinite(typedLng)
    ? typedLng
    : leafletMarker
      ? leafletMarker.getLatLng().lng
      : null;
  if (lat == null || lng == null) return;
  if (mapPickerContext === "modalItem" && modalLocationTarget) {
    const { id, type } = modalLocationTarget;
    const endpoint =
      type === "building"
        ? "buildings"
        : type === "person"
          ? "disappeared"
          : "pets";
    await fetch(`${API_BASE}/${endpoint}/${id}`, {
      method: "PATCH",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ lat, lng }),
    });
    modalLocationTarget = null;
    closeMapPicker();
    if (type === "building") await loadListB();
    else if (type === "person") await loadList();
    else await loadListP();
    const dataArr =
      type === "building"
        ? buildingsData
        : type === "person"
          ? personsData
          : petsData;
    const updated = dataArr.find((i) => i.id === id);
    if (updated) openModalForItem(updated, type);
    return;
  }
  if (mapPickerContext === "modalItemExtra" && modalExtraLocationTarget) {
    const { id, type } = modalExtraLocationTarget;
    const itemType = type === "person" ? "disappeared" : "pets";
    await fetch(`${API_BASE}/item-locations`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ itemId: id, itemType, lat, lng }),
    });
    modalExtraLocationTarget = null;
    closeMapPicker();
    loadExtraLocations(id, type);
    return;
  }
  mapPickerLat[mapPickerContext] = lat;
  mapPickerLng[mapPickerContext] = lng;
  const btnByContext = { person: gpsBtn, pet: gpsBtnP, building: gpsBtnB };
  const btn = btnByContext[mapPickerContext];
  if (btn) {
    btn.classList.add("has-location");
    btn.title = `Ubicación guardada: ${lat.toFixed(5)}, ${lng.toFixed(5)}`;
  }
  // Update location display in the corresponding form
  const displayByContext = {
    person: document.getElementById("locationDisplay"),
    pet: document.getElementById("locationDisplayP"),
    building: document.getElementById("locationDisplayB"),
  };
  const display = displayByContext[mapPickerContext];
  if (display)
    display.innerHTML = `<a href="https://www.google.com/maps?q=${lat},${lng}" target="_blank" style="color:#4ea1ff;">🗺️ ${lat.toFixed(5)}, ${lng.toFixed(5)}</a>`;
  closeMapPicker();
});
function initModalMiniMap(lat, lng) {
  if (modalMiniMap) {
    modalMiniMap.remove();
    modalMiniMap = null;
  }
  setTimeout(() => {
    const container = document.getElementById("modalMiniMapContainer");
    if (!container) return; // el modal pudo haberse cerrado ya
    modalMiniMap = L.map("modalMiniMapContainer", {
      zoomControl: false,
      dragging: false,
      scrollWheelZoom: false,
      doubleClickZoom: false,
    }).setView([lat, lng], 16);
    L.tileLayer("https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png", {
      attribution: "&copy; OpenStreetMap",
    }).addTo(modalMiniMap);
    L.marker([lat, lng]).addTo(modalMiniMap);
    modalMiniMap.invalidateSize();
  }, 50);
}
// Data array for a given item type (used by the random-jump button).
function dataForType(type) {
  return TYPE_REGISTRY[type]?.data?.() || [];
}
// Open the modal for a random item of the same type (excludes current id).
function showRandomItem(type, currentId) {
  const data = dataForType(type);
  const pool = data.filter((it) => it.id !== currentId);
  if (!pool.length) return; // only one item (or none) — nothing to switch to
  const next = pool[Math.floor(Math.random() * pool.length)];
  openModalForItem(next, type);
}
function openModalForItem(item, type) {
  const commentsContainer = document.getElementById("commentsContainer");
  // Track the open item up front so the 🎲 random button works for every
  // type (anuncios return early below, before the old assignment ran).
  currentItemId = item.id;
  currentItemType = type;
  // --- Soporte para anuncios en el modal ---
  if (type === "anuncio") {
    modalStatus.innerHTML = "";
    modalTitle.textContent = item.title || "Anuncio";
    modalMeta.innerHTML = `
          <span>🕒 ${new Date(item.created_at).toLocaleString("es-CO", {
      day: "2-digit",
      month: "2-digit",
      year: "numeric",
      hour: "2-digit",
      minute: "2-digit",
    })}</span>
        `;
    // Galería: todas las imágenes (images array) + main (image/photo_url).
    const gallery = Array.isArray(item.images) ? item.images.slice() : [];
    const main = item.photo_url || item.image || gallery[0] || null;
    if (main && !gallery.includes(main)) gallery.unshift(main);
    const mainHtml = main
      ? `<img class="modal-photo" src="${escapeHtml(main)}" alt="Imagen principal" />`
      : `<div style="color:#666; padding:12px;">Sin imagen todavía.</div>`;
    const galleryHtml = gallery.length
      ? `<div class="anuncio-gallery">${gallery
          .map(
            (url) => `
            <div class="anuncio-gallery-item ${url === main ? "is-main" : ""}">
              <img src="${escapeHtml(url)}" alt="" loading="lazy" />
              <div class="anuncio-gallery-actions">
                ${url === main
                  ? `<span class="anuncio-main-badge">⭐ Principal</span>`
                  : `<button type="button" class="btn-small" onclick="setAnuncioMain('${item.id}','${escapeHtml(url)}')">⭐ Principal</button>`}
                <button type="button" class="btn-small btn-delete" onclick="removeAnuncioImage('${item.id}','${escapeHtml(url)}')">✕</button>
              </div>
            </div>
          `,
          )
          .join("")}</div>`
      : "";
    modalBody.innerHTML = `
          ${mainHtml}
          <div style="white-space:pre-wrap;">${escapeHtml(item.text)}</div>
          <label style="display:block; color:#999; margin:14px 0 4px;">Agregar fotos</label>
          <input type="file" accept="image/*" id="modalPhotoInput" multiple
            style="width:100%; padding:8px; border-radius:8px; border:1px solid #333; background:#1a1d24; color:#eaeaea;" />
          ${galleryHtml}
        `;
    modalActions.innerHTML = isLocalhost
      ? `
            <button class="btn-small btn-share" onclick="shareAnuncio('${item.id}'); closeModal();">📤 Compartir</button>
            <button class="btn-small btn-delete" onclick="removeA('${item.id}'); closeModal();">✕ Eliminar</button>
          `
      : `
            <button class="btn-small btn-share" onclick="shareAnuncio('${item.id}'); closeModal();">📤 Compartir</button>
          `;
    commentsContainer.style.display = "";
    detailModal.open();
    loadComments(item.id, "anuncios");
    // Subir fotos desde el modal (clonar input para limpiar listeners previos)
    const oldInput = document.getElementById("modalPhotoInput");
    const newInput = oldInput.cloneNode(true);
    oldInput.parentNode.replaceChild(newInput, oldInput);
    newInput.addEventListener("change", async (e) => {
      const files = Array.from(e.target.files || []);
      if (!files.length) return;
      const urls = await uploadAnuncioPhotos(files);
      if (!urls.length) return;
      const merged = Array.from(new Set([...gallery, ...urls]));
      await fetch(`${API_BASE}/anuncios/${item.id}`, {
        method: "PATCH",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ images: merged }),
      });
      await loadListA();
      const updated = anunciosData.find((a) => a.id === item.id);
      if (updated) openModalForItem(updated, "anuncio");
    });
    return;
  } else {
    commentsContainer.style.display = "";
  }
  // --- Resto del código existente para personas, mascotas y edificios ---
  const statusClass = item.status || "desaparecido";
  const statusTagLabel = statusLabel(statusClass);
  const statusTagClass = statusCssClass(statusClass);
  modalStatus.innerHTML = `<span class="status-tag ${statusTagClass}">${statusTagLabel}</span>`;
  modalTitle.textContent = item.name || "Sin nombre";
  const dateStr = item.created_at
    ? new Date(item.created_at).toLocaleString("es-CO", {
      day: "2-digit",
      month: "2-digit",
      year: "numeric",
      hour: "2-digit",
      minute: "2-digit",
    })
    : "";
  const metaParts = [];
  if (item.city) metaParts.push(`📍 ${escapeHtml(item.city)}`);
  if (item.location) metaParts.push(`🗺️ ${escapeHtml(item.location)}`);
  if (item.lat != null && item.lng != null)
    metaParts.push(locationLinksHtml(item.lat, item.lng));
  metaParts.push(`🕒 ${dateStr}`);
  modalMeta.innerHTML = metaParts.map((p) => `<span>${p}</span>`).join("");
  // --- Cuerpo del modal: imágenes y subida de foto (para personas y mascotas) ---
  if (type === "person" || type === "pet") {
    const hasLoc = item.lat != null && item.lng != null;
    let imagesHtml = "";
    if (item.photo_url) {
      imagesHtml = `<img class="modal-photo" src="${escapeHtml(item.photo_url)}" alt="Foto subida" style=${item.name.toLowerCase() == "sharon alvear guzmán" && ""}  />`;
    } else if (item.image) {
      imagesHtml = `<img class="modal-photo" src="${escapeHtml(item.image)}" alt="Imagen URL" style=${item.name.toLowerCase() == "sharon alvear guzmán" && ""}/>`;
    }
    // 🐕 NEW: Deep scraped details for Pets
    let deepDetailsHtml = "";
    if (type === "pet") {
      const details = [];
      if (item.breed)
        details.push(`<span>🐕 <b>Raza:</b> ${escapeHtml(item.breed)}</span>`);
      if (item.color)
        details.push(`<span>🎨 <b>Color:</b> ${escapeHtml(item.color)}</span>`);
      if (item.sex)
        details.push(`<span>⚧ <b>Sexo:</b> ${escapeHtml(item.sex)}</span>`);
      if (item.size)
        details.push(`<span>📏 <b>Tamaño:</b> ${escapeHtml(item.size)}</span>`);
      if (item.description) {
        details.push(`<div style="width:100%;margin-bottom:12px;white-space:pre-wrap;color:#ccc;font-size:0.95rem;line-height:1.4;">${escapeHtml(item.description)}</div>`);
      }
      let contactHtml = "";
      if (
        item.contact_name ||
        item.contact_phone ||
        item.contact_whatsapp ||
        item.contact_email
      ) {
        contactHtml = `<div class="pet-contact" style="width:100%;padding:12px;background:rgba(63,163,77,0.1);border:1px solid rgba(63,163,77,0.3);border-radius:8px;margin-bottom:12px;">
                    <div style="font-weight:bold;margin-bottom:8px;">📞 Contacto${item.contact_name ? ": " + escapeHtml(item.contact_name) : ""}</div>
                    <div style="display:flex;flex-wrap:wrap;gap:8px;">`;
        if (item.contact_whatsapp)
          contactHtml += `<a href="https://wa.me/${item.contact_whatsapp}" target="_blank"><button style="background:#25D366;color:white;">💬 WhatsApp </button> </a>`;
        if (item.contact_phone)
          contactHtml += `<a href="tel:${item.contact_phone}"><button>📞 Llamar</button></a>`;
        if (item.contact_email)
          contactHtml += `<a href="mailto:${item.contact_email}"><button>✉️ Email</button></a>`;
        contactHtml += `</div></div>`;
        details.push(contactHtml);
      }
      let sourceHtml = "";
      if (item.source_url) {
        sourceHtml += `<div style="margin-bottom:12px;font-size:0.85rem;"><a href="${item.source_url}" target="_blank" style="color:#4ea1ff;text-decoration:underline;">🔗 Ver publicación original (${item.meta || "Fuente externa"})</a></div>`;
      }
      details.push(sourceHtml);
      if (details.length) {
        deepDetailsHtml = `<div class="pet-details" style="display:flex;flex-wrap:wrap;gap:12px;margin-bottom:12px;padding:10px;background:rgba(255,255,255,0.05);border-radius:8px;font-size:0.9rem;">${details.join("")}</div>`;
      }
    }
    modalBody.innerHTML = `
          ${deepDetailsHtml}
          <label style="display:block; color:#999; margin-bottom:4px;">${item.photo_url ? "Cambiar foto" : "Agregar foto"}</label>
          <input type="file" accept="image/*" id="modalPhotoInput"
            style="width:100%; padding:8px; border-radius:8px; border:1px solid #333; background:#1a1d24; color:#eaeaea;" />
          ${imagesHtml}
        <div style="display:flex;">
            <div style="color:#999;margin-bottom:6px;">📍 Ubicación principal</div>
          <button type="button" class="btn-small" id="modalSetLocationBtn" style="margin-left:auto;margin-bottom:10px;">${hasLoc ? "Cambiar ubicación" : "Agregar ubicación"}</button>
        </div>
          <div style="display:flex;">
            <div style="color:#999;margin-bottom:6px;">📍 Otras ubicaciones posibles</div>
            <button type="button" class="btn-small" id="modalAddExtraLocationBtn" style="margin-left:auto;">Otra ubicación +</button>
          </div>
          ${hasLoc ? `<div id="modalMiniMapContainer" style="height:min(60vh,600px);margin-bottom:10px; overflow:hidden;"></div>` : ""}
            <div id="modalExtraLocationsContainer" style="margin-bottom:6px;"></div>
          ${hasLoc
        ? `
            <div style="margin-bottom:14px;">
              <div style="color:#999; margin-bottom:6px;">📍 Items cercanos (menos de 1km)</div>
              <div id="modalNearbyContainer">${nearbyItemsHtml(item.lat, item.lng, item.id, type)}</div>
            </div>
          `
        : ""
      }
        `;
    document
      .getElementById("modalSetLocationBtn")
      .addEventListener("click", () => {
        modalLocationTarget = { id: item.id, type };
        openMapPicker("modalItem");
      });
    document
      .getElementById("modalAddExtraLocationBtn")
      .addEventListener("click", () => {
        modalExtraLocationTarget = { id: item.id, type };
        openMapPicker("modalItemExtra");
      });
    loadExtraLocations(item.id, type);
    const nearbyContainer = document.getElementById("modalNearbyContainer");
    if (nearbyContainer) {
      nearbyContainer.addEventListener("click", (e) => {
        const row = e.target.closest("[data-nearby-id]");
        if (!row) return;
        const nId = row.dataset.nearbyId;
        const nType = row.dataset.nearbyType;
        const arr =
          nType === "person"
            ? personsData
            : nType === "pet"
              ? petsData
              : buildingsData;
        const nItem = arr.find((x) => x.id === nId);
        if (nItem) openModalForItem(nItem, nType);
      });
    }
    // Remover listeners previos clonando el input
    const oldInput = document.getElementById("modalPhotoInput");
    const newInput = oldInput.cloneNode(true);
    oldInput.parentNode.replaceChild(newInput, oldInput);
    newInput.addEventListener("change", async (e) => {
      const file = e.target.files[0];
      if (!file) return;
      if (type === "person") {
        await uploadPersonPhoto(item.id, file);
        loadList();
      } else if (type === "pet") {
        await uploadPetPhoto(item.id, file);
        loadListP();
      }
      closeModal();
    });
    if (hasLoc) initModalMiniMap(item.lat, item.lng);
  } else if (type === "building") {
    const hasLoc = item.lat != null && item.lng != null;
    let imagesHtml = "";
    if (item.photo_url) {
      imagesHtml = `<img class="modal-photo" src="${escapeHtml(item.photo_url)}" alt="Foto subida" />`;
    } else if (item.image) {
      imagesHtml = `<img class="modal-photo" src="${escapeHtml(item.image)}" alt="Imagen URL" />`;
    }
    modalBody.innerHTML = `
          ${imagesHtml}
          ${hasLoc ? `<div id="modalMiniMapContainer" style="height:min(60vh,600px);margin-bottom:10px; overflow:hidden;"></div>` : ""}
          <div style="color:#888; margin-bottom:8px;">
            ${hasLoc
        ? `📐 ${locationLinksHtml(item.lat, item.lng)}`
        : "Este lugar no tiene ubicación en el mapa todavía."
      }
          </div>
          <button type="button" class="btn-small" id="modalSetLocationBtn">📍 ${hasLoc ? "Cambiar ubicación" : "Agregar ubicación"}</button>
          <label style="display:block; color:#999; margin:14px 0 4px;">${item.photo_url ? "Cambiar foto" : "Agregar foto"}</label>
          <input type="file" accept="image/*" id="modalPhotoInput"
            style="width:100%; padding:8px; border-radius:8px; border:1px solid #333; background:#1a1d24; color:#eaeaea;" />
          ${hasLoc
        ? `
            <div style="margin-top:14px;">
              <div style="color:#999;  margin-bottom:6px;">📍 Items cercanos (menos de 1km)</div>
              <div id="modalNearbyContainer">${nearbyItemsHtml(item.lat, item.lng, item.id, "building")}</div>
            </div>
          `
        : ""
      }
        `;
    document
      .getElementById("modalSetLocationBtn")
      .addEventListener("click", () => {
        modalLocationTarget = { id: item.id, type: "building" };
        openMapPicker("modalItem");
      });
    // Subir foto desde el modal (clonar input para limpiar listeners previos)
    const oldInput = document.getElementById("modalPhotoInput");
    const newInput = oldInput.cloneNode(true);
    oldInput.parentNode.replaceChild(newInput, oldInput);
    newInput.addEventListener("change", async (e) => {
      const file = e.target.files[0];
      if (!file) return;
      await uploadBuildingPhoto(item.id, file);
      loadListB();
      closeModal();
    });
    const nearbyContainer = document.getElementById("modalNearbyContainer");
    if (nearbyContainer) {
      nearbyContainer.addEventListener("click", (e) => {
        const row = e.target.closest("[data-nearby-id]");
        if (!row) return;
        const nId = row.dataset.nearbyId;
        const nType = row.dataset.nearbyType;
        const arr =
          nType === "person"
            ? personsData
            : nType === "pet"
              ? petsData
              : buildingsData;
        const nItem = arr.find((x) => x.id === nId);
        if (nItem) openModalForItem(nItem, nType);
      });
    }
    if (hasLoc) initModalMiniMap(item.lat, item.lng);
  } else if (type === "colaborador") {
    modalStatus.innerHTML = `<span class="status-tag colaborador">🤝 Voluntario</span>`;
    modalTitle.textContent = item.name || "Colaborador";
    const colabMetaParts = [];
    if (item.city) colabMetaParts.push(`📍 ${escapeHtml(item.city)}`);
    if (item.created_at)
      colabMetaParts.push(
        `🕒 ${new Date(item.created_at).toLocaleString("es-CO", {
          day: "2-digit",
          month: "2-digit",
          year: "numeric",
          hour: "2-digit",
          minute: "2-digit",
        })}`,
      );
    modalMeta.innerHTML = colabMetaParts
      .map((p) => `<span>${p}</span>`)
      .join("");
    const details = [];
    if (item.skill)
      details.push(
        `<span>🛠️ <b>Cómo ayuda:</b> ${escapeHtml(item.skill)}</span>`,
      );
    if (item.contact)
      details.push(
        `<span>📞 <b>Contacto:</b> ${escapeHtml(item.contact)}</span>`,
      );
    if (item.city)
      details.push(`<span>📍 <b>Ciudad:</b> ${escapeHtml(item.city)}</span>`);
    modalBody.innerHTML = `
          <div style="display:flex;flex-direction:column;gap:10px;padding:10px;background:rgba(255,255,255,0.05);border-radius:8px;font-size:0.95rem;">
            ${details.join("")}
          </div>
        `;
  } else if (type === "donacion") {
    modalStatus.innerHTML = `<span class="status-tag donacion-${escapeHtml(item.status || "disponible")}">${DONACION_STATUS_LABEL[item.status] || item.status}</span>`;
    modalTitle.textContent = `${item.item_type || "Donación"}${item.quantity ? ` · ${item.quantity}` : ""}`;
    const metaParts = [];
    if (item.location) metaParts.push(`📍 ${escapeHtml(item.location)}`);
    if (item.created_at) metaParts.push(`🕒 ${new Date(item.created_at).toLocaleString("es-CO", { day: "2-digit", month: "2-digit", year: "numeric", hour: "2-digit", minute: "2-digit" })}`);
    modalMeta.innerHTML = metaParts.map((p) => `<span>${p}</span>`).join("");
    const details = [];
    if (item.description) details.push(`<div style="white-space:pre-wrap;">${escapeHtml(item.description)}</div>`);
    if (item.contact) details.push(`<span>📞 <b>Contacto:</b> ${escapeHtml(item.contact)}</span>`);
    modalBody.innerHTML = `<div style="display:flex;flex-direction:column;gap:10px;padding:10px;background:rgba(255,255,255,0.05);border-radius:8px;font-size:0.95rem;">${details.join("")}</div>`;
  } else if (type === "necesidad") {
    modalStatus.innerHTML = `<span class="status-tag necesidad-${escapeHtml(item.status || "abierta")}">${NECESIDAD_STATUS_LABEL[item.status] || item.status}</span>`;
    modalTitle.textContent = `${item.item_type || "Necesidad"}${item.quantity ? ` · ${item.quantity}` : ""}`;
    const metaParts = [];
    if (item.point_name) metaParts.push(`📍 ${escapeHtml(item.point_name)}`);
    if (item.created_at) metaParts.push(`🕒 ${new Date(item.created_at).toLocaleString("es-CO", { day: "2-digit", month: "2-digit", year: "numeric", hour: "2-digit", minute: "2-digit" })}`);
    modalMeta.innerHTML = metaParts.map((p) => `<span>${p}</span>`).join("");
    const details = [];
    if (item.urgency === "alta") details.push(`<span class="status-tag necesidad-urgente">⚠️ URGENTE</span>`);
    if (item.description) details.push(`<div style="white-space:pre-wrap;">${escapeHtml(item.description)}</div>`);
    if (item.contact) details.push(`<span>📞 <b>Contacto:</b> ${escapeHtml(item.contact)}</span>`);
    modalBody.innerHTML = `<div style="display:flex;flex-direction:column;gap:10px;padding:10px;background:rgba(255,255,255,0.05);border-radius:8px;font-size:0.95rem;">${details.join("")}</div>`;
  } else if (type === "logistica") {
    modalStatus.innerHTML = `<span class="status-tag logistica-${escapeHtml(item.status || "pendiente")}">${LOGISTICA_STATUS_LABEL[item.status] || item.status}</span>`;
    modalTitle.textContent = item.task_type || "Tarea";
    const metaParts = [];
    if (item.origin && item.destination) metaParts.push(`📍 ${escapeHtml(item.origin)} → ${escapeHtml(item.destination)}`);
    else if (item.origin) metaParts.push(`📍 ${escapeHtml(item.origin)}`);
    else if (item.destination) metaParts.push(`📍 → ${escapeHtml(item.destination)}`);
    if (item.created_at) metaParts.push(`🕒 ${new Date(item.created_at).toLocaleString("es-CO", { day: "2-digit", month: "2-digit", year: "numeric", hour: "2-digit", minute: "2-digit" })}`);
    modalMeta.innerHTML = metaParts.map((p) => `<span>${p}</span>`).join("");
    const details = [];
    if (item.description) details.push(`<div style="white-space:pre-wrap;">${escapeHtml(item.description)}</div>`);
    if (item.contact) details.push(`<span>📞 <b>Contacto:</b> ${escapeHtml(item.contact)}</span>`);
    modalBody.innerHTML = `<div style="display:flex;flex-direction:column;gap:10px;padding:10px;background:rgba(255,255,255,0.05);border-radius:8px;font-size:0.95rem;">${details.join("")}</div>`;
  } else {
    modalBody.innerHTML = `<div style="color:#888;">Detalles del reporte</div>`;
  }
  // --- Acciones del modal (botones) ---
  let actionsHtml = "";
  if (
    type === "person" ||
    type === "pet" ||
    type === "building" ||
    type === "colaborador"
  ) {
    actionsHtml += `<button type="button" class="btn-small" onclick="copyItemLink('${item.id}','${type}', this)">🔗</button>`;
  }
  if (isLocalhost) {
    actionsHtml += `<button  onclick="removeItem('${item.id}','${type}'); closeModal();">🗑️</button>`;
  }
  if (type === "person" || type === "pet") {
    if (item.status === "encontrado") {
      actionsHtml += `<button  onclick="setStatus('${item.id}','desaparecido','${type}'); closeModal();">Marcar desaparecido</button>`;
    } else {
      actionsHtml += `<button style="margin-left:auto;" onclick="setStatus('${item.id}','encontrado','${type}'); closeModal();">Marcar encontrado</button>`;
      actionsHtml += `<button  onclick="setStatus('${item.id}','angel','${type}'); closeModal();">👼</button>`;
    }
  } else if (type === "building") {
    const s = item.status || "seguro";
    actionsHtml += `<button class="btn-small btn-seguro${s === "seguro" ? " btn-active" : ""}" ${s === "seguro" ? "disabled" : ""} onclick="setStatusB('${item.id}','seguro'); closeModal();">Seguro</button>`;
    actionsHtml += `<button class="btn-small btn-danado${s === "danado" ? " btn-active" : ""}" ${s === "danado" ? "disabled" : ""} onclick="setStatusB('${item.id}','danado'); closeModal();">Dañado</button>`;
    actionsHtml += `<button class="btn-small btn-colapsado${s === "colapsado" ? " btn-active" : ""}" ${s === "colapsado" ? "disabled" : ""} onclick="setStatusB('${item.id}','colapsado'); closeModal();">Colapsado</button>`;
    actionsHtml += `<button class="btn-small btn-acopio${s === "acopio" ? " btn-active" : ""}" ${s === "acopio" ? "disabled" : ""} onclick="setStatusB('${item.id}','acopio'); closeModal();">📦 Acopio</button>`;
    if (window.currentUser) {
      const isPriv = item.private ? 1 : 0;
      actionsHtml += `<button class="btn-small" onclick="togglePrivateB('${item.id}', ${isPriv ? 0 : 1}); closeModal();">${isPriv ? "🔓 Hacer público" : "🔒 Hacer privado"}</button>`;
    }
    if (isLocalhost) {
      actionsHtml += `<button class="btn-small btn-delete" onclick="removeB('${item.id}'); closeModal();">✕ Eliminar</button>`;
    }
  } else if (type === "donacion") {
    const s = item.status || "disponible";
    actionsHtml += `<button class="btn-small${s === "disponible" ? " btn-active" : ""}" ${s === "disponible" ? "disabled" : ""} onclick="setDonacionStatus('${item.id}','disponible'); closeModal();">Disponible</button>`;
    actionsHtml += `<button class="btn-small${s === "reservado" ? " btn-active" : ""}" ${s === "reservado" ? "disabled" : ""} onclick="setDonacionStatus('${item.id}','reservado'); closeModal();">Reservado</button>`;
    actionsHtml += `<button class="btn-small${s === "entregado" ? " btn-active" : ""}" ${s === "entregado" ? "disabled" : ""} onclick="setDonacionStatus('${item.id}','entregado'); closeModal();">✅ Entregado</button>`;
    if (isLocalhost) actionsHtml += `<button class="btn-small btn-delete" onclick="removePrivateItem('${item.id}','donacion'); closeModal();">✕ Eliminar</button>`;
  } else if (type === "necesidad") {
    const s = item.status || "abierta";
    actionsHtml += `<button class="btn-small${s === "abierta" ? " btn-active" : ""}" ${s === "abierta" ? "disabled" : ""} onclick="setNecesidadStatus('${item.id}','abierta'); closeModal();">Abierta</button>`;
    actionsHtml += `<button class="btn-small${s === "en_proceso" ? " btn-active" : ""}" ${s === "en_proceso" ? "disabled" : ""} onclick="setNecesidadStatus('${item.id}','en_proceso'); closeModal();">En proceso</button>`;
    actionsHtml += `<button class="btn-small${s === "cubierta" ? " btn-active" : ""}" ${s === "cubierta" ? "disabled" : ""} onclick="setNecesidadStatus('${item.id}','cubierta'); closeModal();">✅ Cubierta</button>`;
    if (isLocalhost) actionsHtml += `<button class="btn-small btn-delete" onclick="removePrivateItem('${item.id}','necesidad'); closeModal();">✕ Eliminar</button>`;
  } else if (type === "logistica") {
    const s = item.status || "pendiente";
    actionsHtml += `<button class="btn-small${s === "pendiente" ? " btn-active" : ""}" ${s === "pendiente" ? "disabled" : ""} onclick="setLogisticaStatus('${item.id}','pendiente'); closeModal();">Pendiente</button>`;
    actionsHtml += `<button class="btn-small${s === "en_ruta" ? " btn-active" : ""}" ${s === "en_ruta" ? "disabled" : ""} onclick="setLogisticaStatus('${item.id}','en_ruta'); closeModal();">En ruta</button>`;
    actionsHtml += `<button class="btn-small${s === "completado" ? " btn-active" : ""}" ${s === "completado" ? "disabled" : ""} onclick="setLogisticaStatus('${item.id}','completado'); closeModal();">✅ Completado</button>`;
    if (isLocalhost) actionsHtml += `<button class="btn-small btn-delete" onclick="removePrivateItem('${item.id}','logistica'); closeModal();">✕ Eliminar</button>`;
  }
  modalActions.innerHTML = actionsHtml;
  // --- Abrir modal y actualizar URL ---
  detailModal.open();
  const url = new URL(window.location);
  url.searchParams.set("id", item.id);
  url.searchParams.set("type", type);
  window.history.replaceState({}, "", url);
  // --- Cargar comentarios (solo si no es anuncio) ---
  if (type !== "anuncio") {
    const commentType =
      type === "person"
        ? "disappeared"
        : type === "pet"
          ? "pets"
          : type === "colaborador"
            ? "collaborators"
            : type === "donacion"
              ? "donaciones"
              : type === "necesidad"
                ? "necesidades"
                : type === "logistica"
                  ? "logistica"
                  : "buildings";
    loadComments(item.id, commentType);
  }
}
// ================================================================
//  COMENTARIOS
// ================================================================
let currentComments = [];
async function loadComments(itemId, itemType) {
  if (!itemId || !itemType) return;
  currentItemId = itemId;
  currentCommentType = itemType;
  try {
    const res = await fetch(
      `${API_BASE}/comments?itemId=${itemId}&itemType=${itemType}`,
    );
    currentComments = await res.json();
    renderComments();
  } catch {
    document.getElementById("commentsList").innerHTML =
      `<div style="color:#666;">Error al cargar comentarios.</div>`;
  }
}
function renderComments() {
  const list = document.getElementById("commentsList");
  if (!currentComments.length) {
    list.innerHTML = `<div style="color:#666; text-align:center; padding:12px;">Sin comentarios aún.</div>`;
    return;
  }
  list.innerHTML = currentComments
    .map((c) => {
      const date = new Date(c.created_at).toLocaleString("es-CO", {
        day: "2-digit",
        month: "2-digit",
        hour: "2-digit",
        minute: "2-digit",
      });
      return `
          <div class="comment-item">
            <div>
              <pre class="comment-text">${escapeHtml(c.text)}</pre>
              <div class="comment-meta">${date}</div>
            </div>
            <button class="btn-small btn-delete" onclick="deleteComment('${c.id}')">✕</button>
          </div>
        `;
    })
    .join("");
}
async function deleteComment(id) {
  if (!confirm("¿Eliminar este comentario?")) return;
  const ok = await adminDelete(`${API_BASE}/comments/${id}`);
  if (ok) loadComments(currentItemId, currentCommentType);
}
document.getElementById("commentForm").addEventListener("submit", async (e) => {
  e.preventDefault();
  const input = document.getElementById("commentInput");
  const text = input.value.trim();
  if (!text || !currentItemId || !currentCommentType) return;
  await fetch(`${API_BASE}/comments`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({
      itemId: currentItemId,
      itemType: currentCommentType,
      text,
    }),
  });
  input.value = "";
  loadComments(currentItemId, currentCommentType);
});
// ================================================================
//  PERSONS (Personas)
// ================================================================
let personsData = [];
const listEl = document.getElementById("list");
const form = document.getElementById("addForm");
const nameInput = document.getElementById("nameInput");
const locationInput = document.getElementById("locationInput");
const cityInput = document.getElementById("cityInput");
const imageInput = document.getElementById("imageInput");
const gpsBtn = document.getElementById("gpsBtn");
const searchInput = document.getElementById("searchInput");
// Single shared search bar: drives whichever section is active.
// currentRenderFn is set by switchTab() to the active section's render fn.
let currentRenderFn = render;
searchInput.addEventListener("input", () => {
  // On the map tab, the search bar filters map markers/sidebar instead.
  if (currentTabType === "map") {
    if (typeof window.setMapSearchQuery === "function") {
      window.setMapSearchQuery(searchInput.value);
    }
    saveFilters();
    return;
  }
  // On the sismos tab, the search bar filters by location via sismos.js filters
  if (currentTabType === "sismos") {
    if (typeof window.setSismosLocationFilter === "function") {
      window.setSismosLocationFilter(searchInput.value);
    }
    saveFilters();
    return;
  }
  if (currentRenderFn) currentRenderFn();
  saveFilters();
});
// ================================================================
//  FILTROS POR TAB
//  Cada tab con lista tiene su propio estado de filtro {status, city}.
//  Los <select> de ciudad se llenan desde los datos reales de cada lista
//  (no desde cities.json) para mostrar solo ciudades presentes.
//  Los render fns aplican applyFilters() después de fuzzyFilter().
// ================================================================
// status is a Set of selected statuses (inclusive OR).
// Empty set = "Todos" (show all). city is a single SHARED string across tabs.
const filterState = {
  person: { status: new Set() },
  pet: { status: new Set() },
  building: { status: new Set() },
  colaborador: { status: new Set() },
};
// Shared city filter: changing it in any tab applies to every tab.
let sharedCity = "";
// ================================================================
//  PERSISTENCIA DE FILTROS + TAB + BÚSQUEDA (localStorage)
//  Al recargar, se restaura el tab activo, la búsqueda y los filtros.
// ================================================================
const FILTER_STORAGE_KEY = "sismo_filters_v1";
function saveFilters() {
  const data = {
    tab: currentTabType,
    search: searchInput ? searchInput.value : "",
    city: sharedCity,
    status: {
      person: [...filterState.person.status],
      pet: [...filterState.pet.status],
      building: [...filterState.building.status],
      colaborador: [...filterState.colaborador.status],
    },
    // Map filters (type pills + heatmap) — read from map.js.
    map: typeof window.getMapFilterState === "function"
      ? window.getMapFilterState()
      : undefined,
  };
  try {
    localStorage.setItem(FILTER_STORAGE_KEY, JSON.stringify(data));
  } catch (e) { /* storage full or unavailable */ }
}
function loadFilters() {
  try {
    const raw = localStorage.getItem(FILTER_STORAGE_KEY);
    if (!raw) return null;
    return JSON.parse(raw);
  } catch (e) {
    return null;
  }
}
// Restore saved filters (city, search, status) into live state on startup.
// Must run BEFORE the loadList*() calls so the first render applies them.
function restoreFilters() {
  const saved = loadFilters();
  if (!saved) return;
  // Shared city filter.
  if (typeof saved.city === "string") sharedCity = saved.city;
  // Search bar.
  if (searchInput && typeof saved.search === "string") {
    searchInput.value = saved.search;
  }
  // Per-tab status filters.
  if (saved.status) {
    for (const tab of Object.keys(filterState)) {
      const arr = saved.status[tab];
      if (Array.isArray(arr)) {
        filterState[tab].status = new Set(arr);
      }
    }
  }
  // Map filters (type pills + heatmap) — restore into map.js state.
  if (saved.map && typeof window.setMapFilterState === "function") {
    window.setMapFilterState(saved.map);
  }
  // Reflect restored status into the pill UI (visual state).
  document.querySelectorAll(".filter-row").forEach((row) => {
    const tab = row.dataset.tab;
    const f = filterState[tab];
    if (!f) return;
    const allStatuses = [...row.querySelectorAll(".filter-pill")]
      .map((p) => p.dataset.status)
      .filter(Boolean);
    const isAll =
      f.status.size === 0 ||
      (allStatuses.length > 0 && allStatuses.every((s) => f.status.has(s)));
    row.querySelectorAll(".filter-pill").forEach((p) => {
      const s = p.dataset.status || "";
      if (s === "") p.classList.toggle("active", isAll);
      else p.classList.toggle("active", f.status.has(s));
    });
  });
}
// Apply the active tab's filter state to an already-search-filtered array.
// status: empty set = all; otherwise item.status must be in the set (OR).
function applyFilters(items, tab) {
  const f = filterState[tab];
  if (!f) return items;
  return items.filter((item) => {
    if (f.status.size && !f.status.has(item.status || "")) return false;
    if (sharedCity && (item.city || "") !== sharedCity) return false;
    return true;
  });
}
// Rebuild a tab's city <select> from its data so only present cities show.
// The city filter is SHARED across tabs: every select reflects sharedCity.
function refreshCityFilter(tab, data) {
  const row = document.querySelector(`.filter-row[data-tab="${tab}"]`);
  if (!row) return;
  const sel = row.querySelector('.filter-select[data-filter="city"]');
  if (!sel) return;
  const cities = [...new Set(data.map((d) => d.city).filter(Boolean))].sort(
    (a, b) => a.localeCompare(b, "es"),
  );
  sel.innerHTML =
    '<option value="">📍 TODAS</option>' +
    cities
      .map((c) => `<option value="${escapeHtml(c)}">${escapeHtml(c)}</option>`)
      .join("");
  // Reflect the shared city selection (if this tab's data has it).
  if (sharedCity && cities.includes(sharedCity)) sel.value = sharedCity;
  else sel.value = "";
}
// Sync the shared city value into every tab's city <select>.
function syncCitySelects() {
  document.querySelectorAll('.filter-select[data-filter="city"]').forEach((sel) => {
    const row = sel.closest(".filter-row");
    if (!row) return;
    const tab = row.dataset.tab;
    const data = TYPE_REGISTRY[tab]?.data?.() || [];
    const cities = [...new Set(data.map((d) => d.city).filter(Boolean))];
    if (sharedCity && cities.includes(sharedCity)) sel.value = sharedCity;
    else sel.value = "";
  });
  // The map's city selector is populated with ALL cities (from cities.json),
  // so it can always reflect sharedCity directly.
  const mapSel = document.getElementById("citySelectorMap");
  if (mapSel) mapSel.value = sharedCity || "";
}
// Reset a tab's STATUS filters to default (city is shared and NOT reset here).
function resetFilters(tab) {
  const f = filterState[tab];
  if (!f) return;
  f.status.clear();
  const row = document.querySelector(`.filter-row[data-tab="${tab}"]`);
  if (!row) return;
  // Empty set = "Todos" (show all): highlight every pill.
  row
    .querySelectorAll(".filter-pill")
    .forEach((p) => p.classList.add("active"));
}
// Wire up all filter controls once.
document.querySelectorAll(".filter-row").forEach((row) => {
  const tab = row.dataset.tab;
  const f = filterState[tab];
  // Sismos manages its own filters (magnitude/depth/date/location) in sismos.js.
  if (!f) return;
  row.querySelectorAll(".filter-pill").forEach((pill) => {
    pill.addEventListener("click", () => {
      const status = pill.dataset.status || "";
      if (status === "") {
        // "Todos" = show all: empty set (applyFilters treats empty as all),
        // but visually highlight every pill.
        f.status.clear();
        row
          .querySelectorAll(".filter-pill")
          .forEach((p) => p.classList.add("active"));
      } else {
        // Toggle this status in the set (inclusive OR).
        if (f.status.has(status)) f.status.delete(status);
        else f.status.add(status);
        // All available statuses = list of non-empty data-status pills.
        const allStatuses = [...row.querySelectorAll(".filter-pill")]
          .map((p) => p.dataset.status)
          .filter(Boolean);
        // "Todos" is active when set is empty OR fully complete.
        const isAll =
          f.status.size === 0 ||
          (allStatuses.length > 0 && allStatuses.every((s) => f.status.has(s)));
        // Reconcile visual state from the set.
        row.querySelectorAll(".filter-pill").forEach((p) => {
          const s = p.dataset.status || "";
          if (s === "") {
            p.classList.toggle("active", isAll);
          } else {
            p.classList.toggle("active", f.status.has(s));
          }
        });
      }
      if (currentRenderFn) currentRenderFn();
      saveFilters();
    });
  });
  const sel = row.querySelector('.filter-select[data-filter="city"]');
  if (sel) {
    sel.addEventListener("change", () => {
      sharedCity = sel.value;
      syncCitySelects();
      if (currentRenderFn) currentRenderFn();
      saveFilters();
    });
  }
});
const photoInput = document.getElementById("photoInput");
const fileLabel = document.getElementById("fileLabel");
const photoPreview = document.getElementById("photoPreview");
// Referencia para la previsualización de URL
const urlPreview = document.getElementById("urlPreview");
// Previsualización de archivo y URL — usa factory compartida
setupPhotoPreview({
  fileInput: photoInput,
  urlInput: imageInput,
  previewEl: photoPreview,
  labelEl: fileLabel,
  placeholderText: "Ningún archivo seleccionado",
  urlPreviewEl: urlPreview,
});
async function loadList() {
  try {
    const res = await fetch(`${API_BASE}/disappeared`);
    personsData = await res.json();
    refreshCityFilter("person", personsData);
    render();
    updateTabCounts();
    refreshMap();
    tryOpenModalFromUrl();
  } catch {
    listEl.innerHTML = `<div class="empty">No se pudo conectar al servidor.</div>`;
  }
}
// Shared card renderer for persons and pets (same layout, same fields).
// Only the entity type and fallback image differ between the two.
const PERSON_PLACEHOLDER =
  "person.png";
const PET_PLACEHOLDER =
  "paws.png";
function entityCardHtml(item, type, placeholderImg) {
  const found = item.status === "encontrado";
  const date = new Date(item.created_at).toLocaleString("es-CO", {
    day: "2-digit",
    month: "2-digit",
    hour: "2-digit",
    minute: "2-digit",
  });
  const metaParts = [];
  if (item.city) metaParts.push(escapeHtml(item.city));
  if (item.location) metaParts.push(escapeHtml(item.location));
  if (item.lat != null && item.lng != null)
    metaParts.push(locationLinksHtml(item.lat, item.lng));
  metaParts.push(date);
  let imgHtml = "";
  if (item.status === "angel") {
    imgHtml = `<img style="opacity:0.62;" class="card-photo" src="angel.png" alt="Imagen" />`;
  }
  else if (item.name.toLowerCase().includes("#paciente")) {
    imgHtml = `<img style="opacity:0.62;" class="card-photo" src="hospital.png" alt="Imagen" />`;
  }
  else if (item.photo_url) {
    imgHtml = `<img class="card-photo" src="${escapeHtml(item.photo_url)}" alt="Foto" style=${item.name.toLowerCase() == "sharon alvear guzmán" && ""}/>`;
  } else if (item.image) {
    imgHtml = `<img class="card-photo" src="${escapeHtml(item.image)}" alt="Imagen URL" style=${item.name.toLowerCase() == "sharon alvear guzmán" && ""}/>`;
  } else {
    imgHtml = `<img style="opacity:0.62;filter:${placeholderFilter(item.status)};" class="card-photo" src="${placeholderImg}" alt="Imagen URL" />`;
  }
  const isAngel =
    item.name.toLowerCase() == "juan david cano" || item.status == "angel";
  const cardStatusClass = found ? "encontrado" : statusCssClass(item.status);
  const statusTagLabel = isAngel ? "👼" : statusLabel(item.status);
  const statusTagClass = found ? "encontrado" : statusCssClass(item.status);
  return `
        <div class="card ${cardStatusClass}" data-id="${item.id}" data-type="${type}" style = "${isAngel && "background:#6fa8dc; color: white; opacity: 0.62; border-left: 4px solid white; "}" >
    <div style="padding:5px;background:rgba(var(--surface),0.38)">
      <span class="name"
        style="${isAngel && " color:white;"}"
>${escapeHtml(item.name)}</span>
</div >
    <div class="card-main">
      ${imgHtml}
      <div class="card-inner">
        <div style="display:flex;flex-direction:column;">
          <span class="status-tag ${statusTagClass}" style="margin-left:auto;${isAngel && "background:lightblue;color:white;"}">${statusTagLabel}</span >
      </div >
      <div class="info" style="${isAngel & " color:white;"}">
      <span class="meta" style="${isAngel & " color:white;"}">${metaParts.join(" · ")}</span>
                ${commentBadgeHtml(item)}
              </div >
    <div class="actions" style="${isAngel && " display:none"};" >
      ${found
      ? (item.name.toLowerCase().includes("#paciente") ? "" :
        `<button class="" onclick="setStatus('${item.id}','desaparecido','${type}')">Marcar desaparecido</button>`)
      : `<button class="" ${isAngel ? "disabled" : ""} onclick="setStatus('${item.id}','encontrado','${type}')">Marcar encontrado</button>`
    }
              ${isLocalhost
      ? `<button class="" onclick="removeItem('${item.id}','${type}')">✕</button>`
      : ""
    }
            </div >
            </div >
          </div >
        </div >
    `;
}
function personCardHtml(item) {
  return entityCardHtml(item, "person", PERSON_PLACEHOLDER);
}
function render() {
  const items = applyFilters(
    fuzzyFilter(searchInput.value, personsData),
    "person",
  );
  if (!items.length) {
    listEl.innerHTML = `< div class="empty" > ${personsData.length ? "Sin resultados." : "No hay reportes todavía."}</div > `;
    return;
  }
  renderVirtualList(listEl, items, personCardHtml);
}
listEl.addEventListener("click", (e) => {
  if (e.target.closest("button")) return;
  const card = e.target.closest(".card");
  if (!card) return;
  const id = card.dataset.id;
  const item = personsData.find((p) => p.id === id);
  if (item) openModalForItem(item, "person");
});
form.addEventListener("submit", async (e) => {
  e.preventDefault();
  const name = nameInput.value.trim();
  const location = locationInput.value.trim();
  const city = cityInput.value.trim();
  const image = imageInput.value.trim();
  const lat = mapPickerLat.person;
  const lng = mapPickerLng.person;
  if (!name) return;
  const payload = { name, location, city, lat, lng };
  if (image) payload.image = image;
  const res = await fetch(`${API_BASE}/disappeared`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(payload),
  });
  const created = await res.json();
  nameInput.value = "";
  locationInput.value = "";
  cityInput.value = "";
  imageInput.value = "";
  // Limpiar previsualización de URL
  if (urlPreview) {
    urlPreview.style.display = "none";
    urlPreview.src = "";
  }
  const file = photoInput.files[0];
  // Limpiar previsualización
  if (photoPreview.src) {
    URL.revokeObjectURL(photoPreview.src);
    photoPreview.src = "";
    photoPreview.style.display = "none";
  }
  photoInput.value = "";
  fileLabel.textContent = "Ningún archivo seleccionado";
  if (file && created.id) {
    await uploadPersonPhoto(created.id, file);
  }
  mapPickerLat.person = null;
  mapPickerLng.person = null;
  gpsBtn.classList.remove("has-location");
  gpsBtn.title = "Usar mi ubicación";
  document.getElementById("locationDisplay").textContent = "";
  loadList();
  closeCrearModal();
});
gpsBtn.addEventListener("click", () => openMapPicker("person"));
// ================================================================
//  PETS (Mascotas) - sin cambios
// ================================================================
let petsData = [];
const listElP = document.getElementById("listP");
const formP = document.getElementById("addFormP");
const nameInputP = document.getElementById("nameInputP");
const locationInputP = document.getElementById("locationInputP");
const cityInputP = document.getElementById("cityInputP");
const gpsBtnP = document.getElementById("gpsBtnP");
const imageInputP = document.getElementById("imageInputP");
const photoInputP = document.getElementById("photoInputP");
const fileLabelP = document.getElementById("fileLabelP");
const photoPreviewP = document.getElementById("photoPreviewP");
const urlPreviewP = document.getElementById("urlPreviewP");
// Función para subir foto de mascota a Cloudinary
async function uploadPetPhoto(petId, file) {
  const formData = new FormData();
  formData.append("file", file);
  formData.append("upload_preset", CLOUDINARY_UPLOAD_PRESET);
  try {
    const res = await fetch(
      `https://api.cloudinary.com/v1_1/${CLOUDINARY_CLOUD_NAME}/image/upload`,
      {
        method: "POST",
        body: formData,
      },
    );
    const data = await res.json();
    if (!data.secure_url) throw new Error("No se obtuvo secure_url");
    await fetch(`${API_BASE}/pets/${petId}/photo`, {
      method: "PATCH",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ photo_url: data.secure_url }),
    });
    return data.secure_url;
  } catch (err) {
    alert(
      "El reporte se guardó, pero la foto no se pudo subir. Intenta agregarla de nuevo abriendo el reporte.",
    );
    console.error(err);
    return null;
  }
}
// Previsualización de archivo y URL — usa factory compartida
setupPhotoPreview({
  fileInput: photoInputP,
  urlInput: imageInputP,
  previewEl: photoPreviewP,
  labelEl: fileLabelP,
  placeholderText: "Sube una foto",
  urlPreviewEl: urlPreviewP,
});
async function loadListP() {
  try {
    const res = await fetch(`${API_BASE}/pets`);
    petsData = await res.json();
    petsData.forEach((pet) => {
      if (
        (pet.photo_url && pet.photo_url.includes("https://buscandoa.com/assets/img/placeholder.svg")) ||
        (pet.image && pet.image.includes("https://buscandoa.com/assets/img/placeholder.svg"))
      ) {
        pet.photo_url = null;
        pet.image = null;
      }
    });
    refreshCityFilter("pet", petsData);
    renderP();
    updateTabCounts();
    refreshMap();
    tryOpenModalFromUrl();
  } catch {
    listElP.innerHTML = `<div class="empty">No se pudo conectar al servidor.</div>`;
  }
}
function petCardHtml(item) {
  return entityCardHtml(item, "pet", PET_PLACEHOLDER);
}
function renderP() {
  const items = applyFilters(fuzzyFilter(searchInput.value, petsData), "pet");
  if (!items.length) {
    listElP.innerHTML = `<div class="empty">${petsData.length ? "Sin resultados." : "No hay mascotas reportadas."}</div>`;
    return;
  }
  renderVirtualList(listElP, items, petCardHtml);
}
listElP.addEventListener("click", (e) => {
  if (e.target.closest("button")) return;
  const card = e.target.closest(".card");
  if (!card) return;
  const id = card.dataset.id;
  const item = petsData.find((p) => p.id === id);
  if (item) openModalForItem(item, "pet");
});
formP.addEventListener("submit", async (e) => {
  e.preventDefault();
  const name = nameInputP.value.trim();
  const location = locationInputP.value.trim();
  const city = cityInputP.value.trim();
  const image = imageInputP.value.trim();
  const lat = mapPickerLat.pet;
  const lng = mapPickerLng.pet;
  if (!name) return;
  const payload = { name, location, city, lat, lng };
  if (image) payload.image = image;
  const res = await fetch(`${API_BASE}/pets`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(payload),
  });
  const created = await res.json();
  nameInputP.value = "";
  locationInputP.value = "";
  cityInputP.value = "";
  imageInputP.value = "";
  // Limpiar previsualizaciones
  if (urlPreviewP) {
    urlPreviewP.style.display = "none";
    urlPreviewP.src = "";
  }
  const file = photoInputP.files[0];
  if (photoPreviewP.src) {
    URL.revokeObjectURL(photoPreviewP.src);
    photoPreviewP.src = "";
    photoPreviewP.style.display = "none";
  }
  photoInputP.value = "";
  fileLabelP.textContent = "Sube una foto";
  if (file && created.id) {
    await uploadPetPhoto(created.id, file);
  }
  mapPickerLat.pet = null;
  mapPickerLng.pet = null;
  gpsBtnP.classList.remove("has-location");
  gpsBtnP.title = "Usar mi ubicación";
  document.getElementById("locationDisplayP").textContent = "";
  loadListP();
  closeCrearModal();
});
gpsBtnP.addEventListener("click", () => openMapPicker("pet"));
// ================================================================
//  BUILDINGS (Edificios) - sin cambios
// ================================================================
let buildingsData = [];
const listElB = document.getElementById("listB");
const formB = document.getElementById("addFormB");
const nameInputB = document.getElementById("nameInputB");
const locationInputB = document.getElementById("locationInputB");
const cityInputB = document.getElementById("cityInputB");
const gpsBtnB = document.getElementById("gpsBtnB");
const privateInputB = document.getElementById("privateInputB");
const imageInputB = document.getElementById("imageInputB");
const photoInputB = document.getElementById("photoInputB");
const fileLabelB = document.getElementById("fileLabelB");
const photoPreviewB = document.getElementById("photoPreviewB");
const urlPreviewB = document.getElementById("urlPreviewB");
// Función para subir foto de edificio a Cloudinary
async function uploadBuildingPhoto(buildingId, file) {
  const formData = new FormData();
  formData.append("file", file);
  formData.append("upload_preset", CLOUDINARY_UPLOAD_PRESET);
  try {
    const res = await fetch(
      `https://api.cloudinary.com/v1_1/${CLOUDINARY_CLOUD_NAME}/image/upload`,
      {
        method: "POST",
        body: formData,
      },
    );
    const data = await res.json();
    if (!data.secure_url) throw new Error("No se obtuvo secure_url");
    await fetch(`${API_BASE}/buildings/${buildingId}/photo`, {
      method: "PATCH",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ photo_url: data.secure_url }),
    });
    return data.secure_url;
  } catch (err) {
    alert(
      "El reporte se guardó, pero la foto no se pudo subir. Intenta agregarla de nuevo abriendo el reporte.",
    );
    console.error(err);
    return null;
  }
}
// Previsualización de archivo y URL — usa factory compartida
setupPhotoPreview({
  fileInput: photoInputB,
  urlInput: imageInputB,
  previewEl: photoPreviewB,
  labelEl: fileLabelB,
  placeholderText: "Sube una foto",
  urlPreviewEl: urlPreviewB,
});
async function loadListB() {
  try {
    const res = await authFetch(`${API_BASE}/buildings`);
    buildingsData = await res.json();
    refreshCityFilter("building", buildingsData);
    renderB();
    updateTabCounts();
    tryOpenModalFromUrl();
  } catch {
    listElB.innerHTML = `<div class="empty">No se pudo conectar al servidor.</div>`;
  }
}
const BUILDING_PLACEHOLDER =
  "https://external-content.duckduckgo.com/iu/?u=https%3A%2F%2Fstatic.vecteezy.com%2Fsystem%2Fresources%2Fpreviews%2F077%2F676%2F103%2Fnon_2x%2Fsimple-rounded-image-placeholder-with-mountain-landscape-inside-icon-vector.jpg&f=1&nofb=1&ipt=90c0361e0c9755a3d22b949bed9e2588c3e34ea83dc3b27b43f316c5723c07cf";
function buildingCardHtml(item) {
  const date = new Date(item.created_at).toLocaleString("es-CO", {
    day: "2-digit",
    month: "2-digit",
    hour: "2-digit",
    minute: "2-digit",
  });
  const metaParts = [];
  if (item.city) metaParts.push(escapeHtml(item.city));
  if (item.location) metaParts.push(escapeHtml(item.location));
  if (item.lat != null && item.lng != null)
    metaParts.push(locationLinksHtml(item.lat, item.lng));
  metaParts.push(date);
  const statusClass = item.status || "seguro";
  const statusTagLabel = statusLabel(statusClass);
  const statusTagClass = statusCssClass(statusClass);
  let imgHtml = "";
  if (item.photo_url) {
    imgHtml = `<img class="card-photo" src="${escapeHtml(item.photo_url)}" alt="Foto" />`;
  } else if (item.image) {
    imgHtml = `<img class="card-photo" src="${escapeHtml(item.image)}" alt="Imagen" />`;
  } else {
    imgHtml = `<img style="opacity:0.62;" class="card-photo" src="${BUILDING_PLACEHOLDER}" alt="Imagen" />`;
  }
  return `
        <div class="card ${statusClass}" data-id="${item.id}" data-type="building">
<div style="padding:5px;background:rgba(var(--surface),0.38);border-bottom:2px solid rgba(var(--surface),0.62);">
              <span class="name">${escapeHtml(item.name)}${item.private ? " 🔒" : ""}</span>
</div>
          <div class="card-main">
          ${imgHtml}
            <div class="card-inner">
            <div style="display:flex;flex-direction:column;">
<span class="status-tag ${statusTagClass}">${statusTagLabel}</span>
            </div >
              <div class="info">
                <span class="meta">${metaParts.join(" · ")}</span>
                ${commentBadgeHtml(item)}
              </div>
<div class="actions">
              <button class="btn-seguro${statusClass === "seguro" ? " btn-active" : ""}" ${statusClass === "seguro" ? "disabled" : ""} onclick="setStatusB('${item.id}','seguro')">🫶</button>
              <button style="display:none;"class="btn-danado${statusClass === "danado" ? " btn-active" : ""}" ${statusClass === "danado" ? "disabled" : ""} onclick="setStatusB('${item.id}','danado')">Dañado</button>
              <button class=" btn-colapsado${statusClass === "colapsado" ? " btn-active" : ""}" ${statusClass === "colapsado" ? "disabled" : ""} onclick="setStatusB('${item.id}','colapsado')">💥</button>
              <button class=" btn-acopio${statusClass === "acopio" ? " btn-active" : ""}" ${statusClass === "acopio" ? "disabled" : ""} onclick="setStatusB('${item.id}','acopio')">📦</button>
              ${isLocalhost
      ? `<button class="" onclick="removeB('${item.id}')">✕</button>`
      : ""
    }
            </div>
            </div>
          </div >
        </div >
      `;
}
function renderB() {
  const items = applyFilters(
    fuzzyFilter(searchInput.value, buildingsData),
    "building",
  );
  if (!items.length) {
    listElB.innerHTML = `<div class="empty">${buildingsData.length ? "Sin resultados." : "No hay edificios reportados."}</div>`;
    return;
  }
  renderVirtualList(listElB, items, buildingCardHtml);
}
listElB.addEventListener("click", (e) => {
  if (e.target.closest("button")) return;
  const card = e.target.closest(".card");
  if (!card) return;
  const id = card.dataset.id;
  const item = buildingsData.find((b) => b.id === id);
  if (item) openModalForItem(item, "building");
});
formB.addEventListener("submit", async (e) => {
  e.preventDefault();
  const name = nameInputB.value.trim();
  const location = locationInputB.value.trim();
  const city = cityInputB.value.trim();
  const image = imageInputB.value.trim();
  const lat = mapPickerLat.building;
  const lng = mapPickerLng.building;
  const isPrivate = privateInputB ? privateInputB.checked : false;
  if (!name) return;
  const payload = { name, location, city, lat, lng, private: isPrivate };
  if (image) payload.image = image;
  const res = await fetch(`${API_BASE}/buildings`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(payload),
  });
  const created = await res.json();
  nameInputB.value = "";
  locationInputB.value = "";
  cityInputB.value = "";
  imageInputB.value = "";
  if (privateInputB) privateInputB.checked = false;
  if (urlPreviewB) {
    urlPreviewB.style.display = "none";
    urlPreviewB.src = "";
  }
  const file = photoInputB.files[0];
  if (photoPreviewB.src) {
    URL.revokeObjectURL(photoPreviewB.src);
    photoPreviewB.src = "";
    photoPreviewB.style.display = "none";
  }
  photoInputB.value = "";
  fileLabelB.textContent = "Sube una foto";
  if (file && created.id) {
    await uploadBuildingPhoto(created.id, file);
  }
  mapPickerLat.building = null;
  mapPickerLng.building = null;
  gpsBtnB.classList.remove("has-location");
  gpsBtnB.title = "Usar mi ubicación";
  document.getElementById("locationDisplayB").textContent = "";
  loadListB();
  closeCrearModal();
});
gpsBtnB.addEventListener("click", () => openMapPicker("building"));
// ================================================================
//  MODAL: ya muestra imágenes para cualquier tipo
//  La función openModalForItem() ya maneja person, pet y building
//  con el mismo código. Para pets también mostrará image y photo_url.
// ================================================================
// (No se requiere cambio, pero se deja claro que el modal ya es compatible)
// ================================================================
//  ANUNCIOS
// ================================================================
let anunciosData = [];
const listElA = document.getElementById("listA");
const formA = document.getElementById("addFormA");
const anuncioInput = document.getElementById("anuncioInput");
const anuncioTitleInput = document.getElementById("anuncioTitleInput");
const imageInputA = document.getElementById("imageInputA");
const photoInputA = document.getElementById("photoInputA");
const fileLabelA = document.getElementById("fileLabelA");
const photoPreviewA = document.getElementById("photoPreviewA");
const urlPreviewA = document.getElementById("urlPreviewA");
// Sube un único archivo a Cloudinary y devuelve su secure_url (o null).
async function uploadToCloudinary(file) {
  const formData = new FormData();
  formData.append("file", file);
  formData.append("upload_preset", CLOUDINARY_UPLOAD_PRESET);
  const res = await fetch(
    `https://api.cloudinary.com/v1_1/${CLOUDINARY_CLOUD_NAME}/image/upload`,
    { method: "POST", body: formData },
  );
  const data = await res.json();
  if (!data.secure_url) throw new Error("No se obtuvo secure_url");
  return data.secure_url;
}
// Sube varios archivos a Cloudinary y devuelve un array de URLs (ignora fallos).
async function uploadAnuncioPhotos(files) {
  const urls = [];
  for (const file of files) {
    try {
      const url = await uploadToCloudinary(file);
      if (url) urls.push(url);
    } catch (err) {
      console.error(err);
    }
  }
  return urls;
}
// Previsualización de archivo para anuncios (soporta múltiples)
photoInputA.addEventListener("change", function() {
  if (photoPreviewA.src) {
    URL.revokeObjectURL(photoPreviewA.src);
    photoPreviewA.src = "";
  }
  if (this.files.length > 0) {
    fileLabelA.textContent =
      this.files.length === 1
        ? this.files[0].name
        : `${this.files.length} fotos seleccionadas`;
    const objectUrl = URL.createObjectURL(this.files[0]);
    photoPreviewA.src = objectUrl;
    photoPreviewA.style.display = "inline-block";
  } else {
    fileLabelA.textContent = "Sube una o más fotos";
    photoPreviewA.style.display = "none";
    photoPreviewA.src = "";
  }
});
// Previsualización de URL para anuncios
imageInputA.addEventListener("input", function() {
  const url = this.value.trim();
  if (url && (url.startsWith("http://") || url.startsWith("https://"))) {
    urlPreviewA.src = url;
    urlPreviewA.style.display = "inline-block";
    urlPreviewA.onerror = function() {
      urlPreviewA.style.display = "none";
      urlPreviewA.src = "";
    };
  } else {
    urlPreviewA.style.display = "none";
    urlPreviewA.src = "";
    urlPreviewA.onerror = null;
  }
});
async function loadListA() {
  try {
    const res = await fetch(`${API_BASE}/anuncios`);
    anunciosData = await res.json();
    renderA();
    updateTabCounts();
  } catch {
    listElA.innerHTML = `<div class="empty">No se pudo conectar al servidor.</div>`;
  }
}
function renderA() {
  const q = searchInput.value;
  const items = q.trim()
    ? anunciosData.filter(
        (a) => fuzzyMatch(q, a.title || "") || fuzzyMatch(q, a.text || ""),
      )
    : anunciosData;
  if (!items.length) {
    listElA.innerHTML = `<div class="empty">${anunciosData.length ? "Sin resultados." : "No hay anuncios todavía."}</div>`;
    return;
  }
  listElA.innerHTML = items
    .map((item) => {
      const date = new Date(item.created_at).toLocaleString("es-CO", {
        day: "2-digit",
        month: "2-digit",
        hour: "2-digit",
        minute: "2-digit",
      });
      const mainImg =
        item.photo_url ||
        item.image ||
        (Array.isArray(item.images) && item.images[0]) ||
        null;
      const imgHtml = mainImg
        ? `<img class="card-photo" src="${escapeHtml(mainImg)}" alt="Imagen" />`
        : "";
      const titleHtml = item.title
        ? `<div style="padding:5px;background:rgba(var(--surface),0.38);border-bottom:2px solid rgba(var(--surface),0.62);"><span class="name">${escapeHtml(item.title)}</span></div>`
        : "";
      return `
      <div class="card" data-id="${item.id}" style="display:flex;flex-direction:column;">
        ${titleHtml}
        ${imgHtml}
        <div class="info" style="padding:5px;">
          <pre class="anuncio-text">${escapeHtml(item.text)}</pre>
          <span class="meta">${date}</span>
        </div>
        <div class="actions">
          ${isLocalhost
          ? `<button class="" onclick="removeA('${item.id}')">✕</button>`
          : ""
        }
          <button class="" onclick="shareAnuncio('${item.id}')">📤</button>
        </div>
      </div>
    `;
    })
    .join("");
}
// --- Abrir modal al hacer clic en un anuncio ---
listElA.addEventListener("click", (e) => {
  if (e.target.closest("button")) return;
  const card = e.target.closest(".card");
  if (!card) return;
  const id = card.dataset.id;
  if (!id) return;
  const item = anunciosData.find((a) => a.id === id);
  if (item) openModalForItem(item, "anuncio");
});
formA.addEventListener("submit", async (e) => {
  e.preventDefault();
  const title = anuncioTitleInput.value.trim();
  const text = anuncioInput.value.trim();
  const image = imageInputA.value.trim();
  if (!text) return;
  // Subir todas las fotos seleccionadas a Cloudinary primero.
  const files = Array.from(photoInputA.files || []);
  const uploadedUrls = files.length ? await uploadAnuncioPhotos(files) : [];
  // Construir la galería: URL principal (si hay) + fotos subidas.
  const images = [];
  if (image) images.push(image);
  images.push(...uploadedUrls);
  const payload = { text };
  if (title) payload.title = title;
  if (image) payload.image = image;
  if (images.length) payload.images = images;
  const res = await fetch(`${API_BASE}/anuncios`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(payload),
  });
  await res.json();
  anuncioTitleInput.value = "";
  anuncioInput.value = "";
  imageInputA.value = "";
  if (urlPreviewA) {
    urlPreviewA.style.display = "none";
    urlPreviewA.src = "";
  }
  if (photoPreviewA.src) {
    URL.revokeObjectURL(photoPreviewA.src);
    photoPreviewA.src = "";
    photoPreviewA.style.display = "none";
  }
  photoInputA.value = "";
  fileLabelA.textContent = "Sube una o más fotos";
  loadListA();
  closeCrearModal();
});
async function removeA(id) {
  if (!confirm("¿Eliminar este anuncio?")) return;
  const ok = await adminDelete(`${API_BASE}/anuncios/${id}`);
  if (ok) loadListA();
}
// Establece una imagen de la galería como principal.
async function setAnuncioMain(id, url) {
  const item = anunciosData.find((a) => a.id === id);
  if (!item) return;
  await fetch(`${API_BASE}/anuncios/${id}`, {
    method: "PATCH",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ image: url }),
  });
  await loadListA();
  const updated = anunciosData.find((a) => a.id === id);
  if (updated) openModalForItem(updated, "anuncio");
}
// Elimina una imagen de la galería (y ajusta la principal si era esa).
async function removeAnuncioImage(id, url) {
  const item = anunciosData.find((a) => a.id === id);
  if (!item) return;
  const gallery = Array.isArray(item.images) ? item.images.slice() : [];
  const next = gallery.filter((u) => u !== url);
  const main = item.photo_url || item.image;
  const body = { images: next };
  // Si se elimina la imagen principal, promover la primera restante.
  if (main === url) {
    body.image = next[0] || null;
  }
  await fetch(`${API_BASE}/anuncios/${id}`, {
    method: "PATCH",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(body),
  });
  await loadListA();
  const updated = anunciosData.find((a) => a.id === id);
  if (updated) openModalForItem(updated, "anuncio");
}
function shareAnuncio(id) {
  const item = anunciosData.find((a) => a.id === id);
  if (!item) return;
  const shareText = item.title ? `${item.title}\n${item.text}` : item.text;
  shareContent(
    item.title || "Anuncio - Sismo",
    shareText,
    `https://sismoinfo.co/#anuncio/${item.id}`,
  );
}
// ================================================================
//  COLABORADORES (Voluntarios)
// ================================================================
let colabData = [];
const listElColab = document.getElementById("listColab");
const colabListWrapper = document.getElementById("colabListWrapper");
const colabLoginPrompt = document.getElementById("colabLoginPrompt");
const formColab = document.getElementById("addFormColab");
const nameInputColab = document.getElementById("nameInputColab");
const skillInputColab = document.getElementById("skillInputColab");
const contactInputColab = document.getElementById("contactInputColab");
const cityInputColab = document.getElementById("cityInputColab");
async function loadListColab() {
  if (!getAuthToken()) {
    colabData = [];
    renderColab();
    if (colabListWrapper) colabListWrapper.style.display = "none";
    if (colabLoginPrompt) colabLoginPrompt.style.display = "block";
    return;
  }
  try {
    const res = await fetch(`${API_BASE}/collaborators`);
    colabData = await res.json();
    refreshCityFilter("colaborador", colabData);
    renderColab();
    updateTabCounts();
    if (colabListWrapper) colabListWrapper.style.display = "block";
    if (colabLoginPrompt) colabLoginPrompt.style.display = "none";
  } catch {
    listElColab.innerHTML = `<div class="empty">No se pudo conectar al servidor.</div>`;
  }
}
// ================================================================
//  CONTADORES EN LAS PESTAÑAS
//  Mantiene un badge con el total de items por categoría, vivo.
// ================================================================
function updateTabCounts() {
  const set = (id, n) => {
    const badge = document.querySelector(`#${id} .n`);
    if (badge) badge.textContent = n;
  };
  set("tabPersonasBtn", personsData.length);
  set("tabPetsBtn", petsData.length);
  set("tabEdificiosBtn", buildingsData.length);
  set("tabAnunciosBtn", anunciosData.length);
  // Collaborators count is only shown to logged-in users.
  const colabBadge = document.querySelector("#tabColabBtn .n");
  if (colabBadge) {
    if (window.currentUser) {
      colabBadge.textContent = colabData.length;
      colabBadge.style.display = "";
    } else {
      colabBadge.style.display = "none";
    }
  }
}
const COLAB_PLACEHOLDER =
  "person.png";
function colabCardHtml(item) {
  const date = new Date(item.created_at).toLocaleString("es-CO", {
    day: "2-digit",
    month: "2-digit",
    hour: "2-digit",
    minute: "2-digit",
  });
  const metaParts = [];
  if (item.city) metaParts.push(escapeHtml(item.city));
  if (item.contact) metaParts.push(escapeHtml(item.contact));
  metaParts.push(date);
  const imgHtml = `<img style="opacity:0.62;filter:${placeholderFilter("colaborador")};" class="card-photo" src="${COLAB_PLACEHOLDER}" alt="Colaborador" />`;
  return `
        <div class="card colaborador" data-id="${item.id}" data-type="colaborador">
<div style="padding:5px;background:rgba(var(--surface),0.38);border-bottom:2px solid rgba(var(--surface),0.62);">
              <span class="name">${escapeHtml(item.name)}</span>
</div>
          <div class="card-main">
          ${imgHtml}
            <div class="card-inner">
            <div style="display:flex;flex-direction:column;">
<span class="status-tag colaborador">🤝</span>
            </div >
              <div class="info">
                ${item.skill ? `<span class="meta">${escapeHtml(item.skill)}</span>` : ""}
                <span class="meta">${metaParts.join(" · ")}</span>
                ${commentBadgeHtml(item)}
              </div>
<div class="actions">
              ${isLocalhost
      ? `<button class="btn-small btn-delete" onclick="removeColab('${item.id}')">✕</button>`
      : ""
    }
            </div>
            </div>
          </div >
        </div >
      `;
}
function renderColab() {
  const q = searchInput.value;
  const searched = q.trim()
    ? colabData.filter(
      (c) =>
        fuzzyMatch(q, c.name || "") ||
        fuzzyMatch(q, c.skill || "") ||
        fuzzyMatch(q, c.city || "") ||
        fuzzyMatch(q, c.contact || ""),
    )
    : colabData;
  const items = applyFilters(searched, "colaborador");
  if (!items.length) {
    listElColab.innerHTML = `<div class="empty">${colabData.length ? "Sin resultados." : "Todavía nadie se ha unido como colaborador."}</div>`;
    return;
  }
  renderVirtualList(listElColab, items, colabCardHtml);
}
// --- Abrir modal al hacer clic en un colaborador ---
listElColab.addEventListener("click", (e) => {
  if (e.target.closest("button")) return;
  const card = e.target.closest(".card");
  if (!card) return;
  const id = card.dataset.id;
  if (!id) return;
  const item = colabData.find((c) => c.id === id);
  if (item) openModalForItem(item, "colaborador");
});
formColab.addEventListener("submit", async (e) => {
  e.preventDefault();
  const name = nameInputColab.value.trim();
  if (!name) return;
  const email = document.getElementById("emailInputColab")?.value.trim() || null;
  const password = document.getElementById("passwordInputColab")?.value || null;
  const body = {
    name,
    skill: skillInputColab.value.trim(),
    contact: contactInputColab.value.trim(),
    city: cityInputColab.value.trim(),
  };
  if (email) body.email = email;
  if (password) body.password = password;
  const res = await fetch(`${API_BASE}/collaborators`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(body),
  });
  if (!res.ok) {
    const data = await res.json().catch(() => ({}));
    alert(data.error || "Error al registrarse");
    return;
  }
  nameInputColab.value = "";
  skillInputColab.value = "";
  contactInputColab.value = "";
  cityInputColab.value = "";
  const emailEl = document.getElementById("emailInputColab");
  const passEl = document.getElementById("passwordInputColab");
  if (emailEl) emailEl.value = "";
  if (passEl) passEl.value = "";
  loadListColab();
});
async function removeColab(id) {
  if (!confirm("¿Quitar este colaborador?")) return;
  const ok = await adminDelete(`${API_BASE}/collaborators/${id}`);
  if (ok) loadListColab();
}
// ================================================================
//  SUB-TABS de la pestaña Colaboradores
//  Cambia entre Chat / Donaciones / Necesidades / Logística / Voluntarios
// ================================================================
const subTabBtns = document.querySelectorAll(".sub-tab-btn");
const subTabPanels = document.querySelectorAll(".sub-tab-panel");
const subActionBtns = document.querySelectorAll(".sub-action");
function setActiveSubTab(target) {
  subTabBtns.forEach((b) => b.classList.toggle("active", b.dataset.subtab === target));
  subTabPanels.forEach((p) => {
    p.classList.toggle("active", p.dataset.subtabPanel === target);
  });
  // Mostrar solo el botón de acción de la sub-tab activa.
  subActionBtns.forEach((a) => {
    a.style.display = a.dataset.actionFor === target ? "" : "none";
  });
  // Render the target list if it's a data sub-tab and empty.
  if (target === "donaciones" && !donacionesData.length) loadListDonaciones();
  else if (target === "necesidades" && !necesidadesData.length) loadListNecesidades();
  else if (target === "logistica" && !logisticaData.length) loadListLogistica();
  else if (target === "voluntarios" && !colabData.length) loadListColab();
}
subTabBtns.forEach((btn) => {
  btn.addEventListener("click", () => setActiveSubTab(btn.dataset.subtab));
});
// Estado inicial: sub-tab "chat" activa, sin botón de acción visible.
setActiveSubTab("chat");

// ================================================================
//  DONACIONES (privado)
// ================================================================
let donacionesData = [];
const listElDonaciones = document.getElementById("listDonaciones");
const formDonacion = document.getElementById("addFormDonacion");
const DONACION_STATUS_LABEL = {
  disponible: "Disponible",
  reservado: "Reservado",
  entregado: "Entregado",
  vencido: "Vencido",
};
const DONACION_EMOJI = {
  comida: "🍽️", ropa: "👕", insumos: "🧰", medicinas: "💊", transporte: "🚗", otro: "📦",
};
async function loadListDonaciones() {
  if (!getAuthToken()) { donacionesData = []; renderDonaciones(); return; }
  try {
    const res = await fetch(`${API_BASE}/donaciones`, {
      headers: { Authorization: `Bearer ${getAuthToken()}` },
    });
    if (res.status === 401) { clearAuthToken(); showLoginForm(); return; }
    donacionesData = await res.json();
    renderDonaciones();
  } catch {
    listElDonaciones.innerHTML = `<div class="empty">No se pudo conectar al servidor.</div>`;
  }
}
function donacionCardHtml(item) {
  const date = new Date(item.created_at).toLocaleString("es-CO", {
    day: "2-digit", month: "2-digit", hour: "2-digit", minute: "2-digit",
  });
  const emoji = DONACION_EMOJI[item.item_type] || "🎁";
  const metaParts = [];
  if (item.location) metaParts.push(`📍 ${escapeHtml(item.location)}`);
  metaParts.push(`🕒 ${date}`);
  return `
    <div class="card donacion" data-id="${item.id}" data-type="donacion">
      <div style="padding:5px;background:rgba(var(--surface),0.38);border-bottom:2px solid rgba(var(--surface),0.62);">
        <span class="name">${emoji} ${escapeHtml(item.item_type)}${item.quantity ? ` · ${escapeHtml(item.quantity)}` : ""}</span>
        <span class="status-tag donacion-${escapeHtml(item.status || "disponible")}">${DONACION_STATUS_LABEL[item.status] || item.status}</span>
      </div>
      <div class="card-main">
        <div class="card-inner">
          <div class="info">
            <span class="meta">${escapeHtml(item.description)}</span>
            <span class="meta">${metaParts.join(" · ")}</span>
            ${commentBadgeHtml(item)}
          </div>
        </div>
      </div>
    </div>
  `;
}
function renderDonaciones() {
  const q = searchInput.value;
  const searched = q.trim()
    ? donacionesData.filter((d) =>
        fuzzyMatch(q, d.item_type || "") || fuzzyMatch(q, d.description || "") || fuzzyMatch(q, d.location || ""))
    : donacionesData;
  if (!searched.length) {
    listElDonaciones.innerHTML = `<div class="empty">${donacionesData.length ? "Sin resultados." : "Aún no hay donaciones ofrecidas."}</div>`;
    return;
  }
  renderVirtualList(listElDonaciones, searched, donacionCardHtml);
}
listElDonaciones.addEventListener("click", (e) => {
  if (e.target.closest("button")) return;
  const card = e.target.closest(".card");
  if (!card) return;
  const item = donacionesData.find((d) => d.id === card.dataset.id);
  if (item) openModalForItem(item, "donacion");
});
formDonacion.addEventListener("submit", async (e) => {
  e.preventDefault();
  const body = {
    item_type: document.getElementById("donacionTypeInput").value,
    quantity: document.getElementById("donacionQtyInput").value.trim(),
    description: document.getElementById("donacionDescInput").value.trim(),
    location: document.getElementById("donacionLocInput").value.trim(),
    contact: document.getElementById("donacionContactInput").value.trim(),
  };
  const res = await fetch(`${API_BASE}/donaciones`, {
    method: "POST",
    headers: { "Content-Type": "application/json", Authorization: `Bearer ${getAuthToken()}` },
    body: JSON.stringify(body),
  });
  if (!res.ok) { const d = await res.json().catch(() => ({})); alert(d.error || "Error"); return; }
  e.target.reset();
  closeCrearModal();
  loadListDonaciones();
});
async function setDonacionStatus(id, status) {
  await fetch(`${API_BASE}/donaciones/${id}`, {
    method: "PATCH",
    headers: { "Content-Type": "application/json", Authorization: `Bearer ${getAuthToken()}` },
    body: JSON.stringify({ status }),
  });
  loadListDonaciones();
}

// ================================================================
//  NECESIDADES (privado)
// ================================================================
let necesidadesData = [];
const listElNecesidades = document.getElementById("listNecesidades");
const formNecesidad = document.getElementById("addFormNecesidad");
const NECESIDAD_STATUS_LABEL = { abierta: "Abierta", en_proceso: "En proceso", cubierta: "Cubierta" };
const NECESIDAD_EMOJI = {
  comida: "🍽️", ropa: "👕", carpas: "⛺", medicinas: "💊", aseo: "🧼", otro: "📦",
};
async function loadListNecesidades() {
  if (!getAuthToken()) { necesidadesData = []; renderNecesidades(); return; }
  try {
    const res = await fetch(`${API_BASE}/necesidades`, {
      headers: { Authorization: `Bearer ${getAuthToken()}` },
    });
    if (res.status === 401) { clearAuthToken(); showLoginForm(); return; }
    necesidadesData = await res.json();
    renderNecesidades();
  } catch {
    listElNecesidades.innerHTML = `<div class="empty">No se pudo conectar al servidor.</div>`;
  }
}
function necesidadCardHtml(item) {
  const date = new Date(item.created_at).toLocaleString("es-CO", {
    day: "2-digit", month: "2-digit", hour: "2-digit", minute: "2-digit",
  });
  const emoji = NECESIDAD_EMOJI[item.item_type] || "🆘";
  const metaParts = [];
  if (item.point_name) metaParts.push(`📍 ${escapeHtml(item.point_name)}`);
  metaParts.push(`🕒 ${date}`);
  const urgent = item.urgency === "alta" ? `<span class="status-tag necesidad-urgente">⚠️ URGENTE</span>` : "";
  return `
    <div class="card necesidad" data-id="${item.id}" data-type="necesidad">
      <div style="padding:5px;background:rgba(var(--surface),0.38);border-bottom:2px solid rgba(var(--surface),0.62);">
        <span class="name">${emoji} ${escapeHtml(item.item_type)}${item.quantity ? ` · ${escapeHtml(item.quantity)}` : ""}</span>
        <span class="status-tag necesidad-${escapeHtml(item.status || "abierta")}">${NECESIDAD_STATUS_LABEL[item.status] || item.status}</span>
        ${urgent}
      </div>
      <div class="card-main">
        <div class="card-inner">
          <div class="info">
            <span class="meta">${escapeHtml(item.description)}</span>
            <span class="meta">${metaParts.join(" · ")}</span>
            ${commentBadgeHtml(item)}
          </div>
        </div>
      </div>
    </div>
  `;
}
function renderNecesidades() {
  const q = searchInput.value;
  const searched = q.trim()
    ? necesidadesData.filter((n) =>
        fuzzyMatch(q, n.item_type || "") || fuzzyMatch(q, n.description || "") || fuzzyMatch(q, n.point_name || ""))
    : necesidadesData;
  if (!searched.length) {
    listElNecesidades.innerHTML = `<div class="empty">${necesidadesData.length ? "Sin resultados." : "Aún no hay necesidades reportadas."}</div>`;
    return;
  }
  renderVirtualList(listElNecesidades, searched, necesidadCardHtml);
}
listElNecesidades.addEventListener("click", (e) => {
  if (e.target.closest("button")) return;
  const card = e.target.closest(".card");
  if (!card) return;
  const item = necesidadesData.find((n) => n.id === card.dataset.id);
  if (item) openModalForItem(item, "necesidad");
});
formNecesidad.addEventListener("submit", async (e) => {
  e.preventDefault();
  const body = {
    item_type: document.getElementById("necesidadTypeInput").value,
    quantity: document.getElementById("necesidadQtyInput").value.trim(),
    description: document.getElementById("necesidadDescInput").value.trim(),
    urgency: document.getElementById("necesidadUrgencyInput").value,
    point_name: document.getElementById("necesidadPointInput").value.trim(),
    contact: document.getElementById("necesidadContactInput").value.trim(),
  };
  const res = await fetch(`${API_BASE}/necesidades`, {
    method: "POST",
    headers: { "Content-Type": "application/json", Authorization: `Bearer ${getAuthToken()}` },
    body: JSON.stringify(body),
  });
  if (!res.ok) { const d = await res.json().catch(() => ({})); alert(d.error || "Error"); return; }
  e.target.reset();
  closeCrearModal();
  loadListNecesidades();
});
async function setNecesidadStatus(id, status) {
  await fetch(`${API_BASE}/necesidades/${id}`, {
    method: "PATCH",
    headers: { "Content-Type": "application/json", Authorization: `Bearer ${getAuthToken()}` },
    body: JSON.stringify({ status }),
  });
  loadListNecesidades();
}

// ================================================================
//  LOGÍSTICA (privado)
// ================================================================
let logisticaData = [];
const listElLogistica = document.getElementById("listLogistica");
const formLogistica = document.getElementById("addFormLogistica");
const LOGISTICA_STATUS_LABEL = { pendiente: "Pendiente", en_ruta: "En ruta", completado: "Completado", cancelado: "Cancelado" };
const LOGISTICA_EMOJI = { entrega: "📦", recogida: "📥", transporte: "🚚", apoyo: "🙋" };
async function loadListLogistica() {
  if (!getAuthToken()) { logisticaData = []; renderLogistica(); return; }
  try {
    const res = await fetch(`${API_BASE}/logistica`, {
      headers: { Authorization: `Bearer ${getAuthToken()}` },
    });
    if (res.status === 401) { clearAuthToken(); showLoginForm(); return; }
    logisticaData = await res.json();
    renderLogistica();
  } catch {
    listElLogistica.innerHTML = `<div class="empty">No se pudo conectar al servidor.</div>`;
  }
}
function logisticaCardHtml(item) {
  const date = new Date(item.created_at).toLocaleString("es-CO", {
    day: "2-digit", month: "2-digit", hour: "2-digit", minute: "2-digit",
  });
  const emoji = LOGISTICA_EMOJI[item.task_type] || "🚚";
  const metaParts = [];
  if (item.origin && item.destination) metaParts.push(`📍 ${escapeHtml(item.origin)} → ${escapeHtml(item.destination)}`);
  else if (item.origin) metaParts.push(`📍 ${escapeHtml(item.origin)}`);
  else if (item.destination) metaParts.push(`📍 → ${escapeHtml(item.destination)}`);
  metaParts.push(`🕒 ${date}`);
  return `
    <div class="card logistica" data-id="${item.id}" data-type="logistica">
      <div style="padding:5px;background:rgba(var(--surface),0.38);border-bottom:2px solid rgba(var(--surface),0.62);">
        <span class="name">${emoji} ${escapeHtml(item.task_type)}</span>
        <span class="status-tag logistica-${escapeHtml(item.status || "pendiente")}">${LOGISTICA_STATUS_LABEL[item.status] || item.status}</span>
      </div>
      <div class="card-main">
        <div class="card-inner">
          <div class="info">
            <span class="meta">${escapeHtml(item.description)}</span>
            <span class="meta">${metaParts.join(" · ")}</span>
            ${commentBadgeHtml(item)}
          </div>
        </div>
      </div>
    </div>
  `;
}
function renderLogistica() {
  const q = searchInput.value;
  const searched = q.trim()
    ? logisticaData.filter((l) =>
        fuzzyMatch(q, l.task_type || "") || fuzzyMatch(q, l.description || "") || fuzzyMatch(q, l.origin || "") || fuzzyMatch(q, l.destination || ""))
    : logisticaData;
  if (!searched.length) {
    listElLogistica.innerHTML = `<div class="empty">${logisticaData.length ? "Sin resultados." : "Aún no hay tareas de logística."}</div>`;
    return;
  }
  renderVirtualList(listElLogistica, searched, logisticaCardHtml);
}
listElLogistica.addEventListener("click", (e) => {
  if (e.target.closest("button")) return;
  const card = e.target.closest(".card");
  if (!card) return;
  const item = logisticaData.find((l) => l.id === card.dataset.id);
  if (item) openModalForItem(item, "logistica");
});
formLogistica.addEventListener("submit", async (e) => {
  e.preventDefault();
  const body = {
    task_type: document.getElementById("logisticaTypeInput").value,
    description: document.getElementById("logisticaDescInput").value.trim(),
    origin: document.getElementById("logisticaOriginInput").value.trim(),
    destination: document.getElementById("logisticaDestInput").value.trim(),
    contact: document.getElementById("logisticaContactInput").value.trim(),
  };
  const res = await fetch(`${API_BASE}/logistica`, {
    method: "POST",
    headers: { "Content-Type": "application/json", Authorization: `Bearer ${getAuthToken()}` },
    body: JSON.stringify(body),
  });
  if (!res.ok) { const d = await res.json().catch(() => ({})); alert(d.error || "Error"); return; }
  e.target.reset();
  closeCrearModal();
  loadListLogistica();
});
async function setLogisticaStatus(id, status) {
  await fetch(`${API_BASE}/logistica/${id}`, {
    method: "PATCH",
    headers: { "Content-Type": "application/json", Authorization: `Bearer ${getAuthToken()}` },
    body: JSON.stringify({ status }),
  });
  loadListLogistica();
}
// Deep link: #anuncio/ abre directamente ese anuncio en su modal.
function handleAnuncioDeepLink() {
  const match = location.hash.match(/^#anuncio\/(.+)$/);
  if (!match) return;
  const id = decodeURIComponent(match[1]);
  const item = anunciosData.find((a) => a.id === id);
  if (!item) return;
  switchTab(tabAnunciosBtn, tabAnuncios, "anuncio");
  openModalForItem(item, "anuncio");
}
// ================================================================
//  ACCIONES COMPARTIDAS
// ================================================================
async function setStatus(id, status, type) {
  const endpoint =
    type === "person" ? "disappeared" : type === "pet" ? "pets" : "";
  if (!endpoint) return;
  await fetch(`${API_BASE}/${endpoint}/${id}`, {
    method: "PATCH",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ status }),
  });
  if (type === "person") loadList();
  else if (type === "pet") loadListP();
}
async function removeItem(id, type) {
  if (!confirm("¿Eliminar este reporte?")) return;
  const endpoint =
    type === "person" ? "disappeared" : type === "pet" ? "pets" : "";
  if (!endpoint) return;
  const ok = await adminDelete(`${API_BASE}/${endpoint}/${id}`);
  if (ok) {
    if (type === "person") loadList();
    else if (type === "pet") loadListP();
  }
}
// Eliminar items privados de la pestaña Colaboradores (admin).
async function removePrivateItem(id, type) {
  if (!confirm("¿Eliminar este elemento?")) return;
  const endpoint =
    type === "donacion" ? "donaciones" : type === "necesidad" ? "necesidades" : "logistica";
  const ok = await adminDelete(`${API_BASE}/${endpoint}/${id}`);
  if (ok) {
    if (type === "donacion") loadListDonaciones();
    else if (type === "necesidad") loadListNecesidades();
    else loadListLogistica();
  }
}
async function setStatusB(id, status) {
  await fetch(`${API_BASE}/buildings/${id}`, {
    method: "PATCH",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ status }),
  });
  loadListB();
}
async function removeB(id) {
  if (!confirm("¿Eliminar este edificio?")) return;
  const ok = await adminDelete(`${API_BASE}/buildings/${id}`);
  if (ok) loadListB();
}
async function togglePrivateB(id, isPrivate) {
  await authFetch(`${API_BASE}/buildings/${id}`, {
    method: "PATCH",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ private: isPrivate ? 1 : 0 }),
  });
  loadListB();
}
// ================================================================
//  ABRIR MODAL DESDE URL
// ================================================================
let modalOpenedFromUrl = false;
let urlModalAttempts = 0;
const MAX_URL_ATTEMPTS = 6;
function getUrlParams() {
  const params = new URLSearchParams(window.location.search);
  return { id: params.get("id"), type: params.get("type") };
}
function tryOpenModalFromUrl() {
  if (modalOpenedFromUrl) return true;
  // If the user is on the map tab, the modal was opened from the map (not a
  // URL deep-link). Re-opening it here would switch tabs away from the map,
  // so bail out and let the map's own modal flow continue.
  if (currentTabType === "map") return true;
  const { id, type } = getUrlParams();
  if (!id || !type || type === "anuncio") return true;
  let dataArray = null;
  if (type === "person") dataArray = personsData;
  else if (type === "pet") dataArray = petsData;
  else if (type === "building") dataArray = buildingsData;
  else if (type === "colaborador") dataArray = colabData;
  else return true;
  if (!dataArray || dataArray.length === 0) return false;
  const item = dataArray.find((i) => i.id === id);
  if (item) {
    if (type === "person") switchTab(tabPersonasBtn, tabPersonas, "person");
    else if (type === "pet") switchTab(tabPetsBtn, tabPets, "pet");
    else if (type === "building")
      switchTab(tabEdificiosBtn, tabEdificios, "building");
    else if (type === "colaborador")
      switchTab(tabColabBtn, tabColab, "colaborador");
    setTimeout(() => {
      openModalForItem(item, type);
      modalOpenedFromUrl = true;
    });
  }
  // Si no hay parámetro o es inválido, ya está activa "person" por defecto
  return true;
}
// ================================================================
//  INICIALIZACIÓN
//  Lee el tab de la URL (si existe) y carga todos los datasets.
//  Sin esto, personsData/petsData/buildingsData/anunciosData/colabData
//  nunca se llenan y ni las listas ni el mapa muestran nada.
// ================================================================
function applyTabFromUrl() {
  const params = new URLSearchParams(window.location.search);
  const tab = params.get("tab");
  const tabTargets = {
    person: [tabPersonasBtn, tabPersonas, "person"],
    pet: [tabPetsBtn, tabPets, "pet"],
    building: [tabEdificiosBtn, tabEdificios, "building"],
    anuncio: [tabAnunciosBtn, tabAnuncios, "anuncio"],
    colaborador: [tabColabBtn, tabColab, "colaborador"],
    map: [tabMapBtn, tabMapPanel, "map"],
    wiki: [wikiBtn, tabWiki, "wiki"],
    sismos: [tabSismosBtn, tabSismos, "sismos"],
  };
  // URL ?tab= takes priority; otherwise fall back to the saved tab.
  const saved = loadFilters();
  const targetTab = tab || (saved && saved.tab);
  if (targetTab && tabTargets[targetTab]) {
    switchTab(...tabTargets[targetTab]);
  }
}
restoreFilters();
loadList();
loadListP();
loadListB();
loadListA();
loadListColab();
applyTabFromUrl();
handleAnuncioDeepLink();
trackLayoutHeights();
// Ejecutar después de que todo esté cargado
setTimeout(tryOpenModalFromUrl, 100);

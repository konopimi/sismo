//===== public/index.js =====
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
  const privateWrapper = document.getElementById("colabPrivateWrapper");
  if (loginForm) loginForm.style.display = "flex";
  if (privateWrapper) privateWrapper.style.display = "none";
  if (colabLoginPrompt) colabLoginPrompt.style.display = "block";
  window.currentUser = null;
  // Hide the collaborators count badge now that we're logged out.
  if (typeof updateTabCounts === "function") updateTabCounts();
}
function showChat() {
  const loginForm = document.getElementById("loginForm");
  const privateWrapper = document.getElementById("colabPrivateWrapper");
  const connContainer = document.getElementById("connContainer");
  if (loginForm) loginForm.style.display = "none";
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
const MATRIX_ROOM_ALIAS = "#ayuda-en-cali:matrix.sismoinfo.co";
let matrixClient = null;
let matrixRoom = null;
let matrixStarted = false;
// Directorio de matrix_user_id -> nombre, para resolver remitentes en el
// chat. /api/collaborators NO incluye matrix_user_id (por diseño, es
// público); solo /api/collaborators/matrix lo trae, y requiere sesión.
let matrixDirectory = null; // Map<matrix_user_id, name>
async function loadMatrixDirectory() {
  try {
    const res = await authFetch(`${API_BASE}/collaborators/matrix`);
    if (!res.ok) return new Map();
    const data = await res.json();
    return new Map(data.map((c) => [c.matrix_user_id, c.name]));
  } catch {
    return new Map();
  }
}
// Control de carga de mensajes antiguos (infinite scroll).
let loadingOlder = false;
// Estado de respuesta (cita de mensaje).
// Estado de respuesta (cita de mensaje).
let replyingTo = null; // { eventId, sender, body, msgtype, url }
function setReplyingTo(event) {
  const content = event.getContent();
  replyingTo = {
    eventId: event.getId(),
    sender: event.getSender(),
    body: content.body || "",
    msgtype: content.msgtype,
    url: content.url,
  };
  updateReplyBar();
}
function clearReplyingTo() {
  replyingTo = null;
  updateReplyBar();
}
function updateReplyBar() {
  const bar = document.getElementById("chatReplyBar");
  const preview = document.getElementById("chatReplyPreview");
  if (!bar || !preview) return;
  if (replyingTo) {
    // Synchronous lookup from cached directory.
    let name = replyingTo.sender === matrixClient.getUserId()
      ? "Tú"
      : (matrixDirectory?.get(replyingTo.sender) || replyingTo.sender.split(":")[0].replace("@", ""));
    let previewText = replyingTo.body || "";
    if (replyingTo.msgtype === "m.image") previewText = "📷 Imagen";
    else if (replyingTo.msgtype === "m.video") previewText = "🎬 Video";
    previewText = previewText.length > 50 ? previewText.slice(0, 50) + "…" : previewText;
    preview.innerHTML = `<div style="font-size:0.75rem;color:#ccc;white-space:nowrap;overflow:hidden;text-overflow:ellipsis;"><strong>${escapeHtml(name)}</strong> ${escapeHtml(previewText)}</div>`;
    bar.style.display = "flex";
    // If name is still a UUID, try to refresh directory and update.
    if (name.includes("-") && name.length === 36) {
      loadMatrixDirectory().then((dir) => {
        matrixDirectory = dir;
        const resolved = dir.get(replyingTo.sender);
        if (resolved) {
          const nameEl = preview.querySelector("strong");
          if (nameEl) nameEl.textContent = resolved;
        }
      });
    }
  } else {
    bar.style.display = "none";
    preview.innerHTML = "";
  }
}
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
  // Resolve the alias. If it doesn't exist yet, create the room (private).
  let roomId = null;
  try {
    const aliasRes = await matrixClient.getRoomIdForAlias(MATRIX_ROOM_ALIAS);
    roomId = aliasRes && aliasRes.room_id;
  } catch (e) {
    // M_NOT_FOUND: alias no existe todavía → crear el room.
    if (e.errcode !== "M_NOT_FOUND") throw e;
  }
  if (!roomId) {
    const { room_id } = await matrixClient.createRoom({
      name: "Ayuda en Cali 🆘",
      topic: "Canal general de colaboradores de Sismo",
      visibility: "private",
      room_alias_name: "ayuda-en-cali",
    });
    return room_id;
  }
  // El room ya existe (privado). Intentamos unirnos: funciona si ya somos
  // miembros o si tenemos una invitación pendiente. Si no, no lanzamos
  // error — el usuario verá el chat vacío hasta que un miembro lo invite.
  try {
    await matrixClient.joinRoom(roomId);
  } catch (e) {
    // M_FORBIDDEN: sin invitación. No es fatal; el room se mostrará igual
    // y el usuario podrá ser invitado después.
    if (e.errcode !== "M_FORBIDDEN") throw e;
  }
  return roomId;
}
// Renderiza la lista de conversaciones en el sub-tab "Chat".
function renderChatUI() {
  const container = document.getElementById("chatListContainer");
  if (!container) return;
  container.innerHTML = `
    <div style="display:flex;flex-direction:column;gap:8px;">
      <button class="chat-list-item" data-room-id="${escapeHtml(matrixRoom)}" style="
        display:flex;align-items:center;gap:10px;padding:12px;border-radius:10px;
        background:rgba(120,120,120,0.15);border:1px solid rgba(120,120,120,0.3);
        cursor:pointer;text-align:left;width:100%;
      ">
        <span style="font-size:1.4em;">💬</span>
        <div style="flex:1;">
          <div style="font-weight:600;">Ayuda en Cali 🆘</div>
          <div style="font-size:0.8em;opacity:0.7;">Canal general de colaboradores</div>
        </div>
      </button>
      <button id="chatInviteBtn" style="
        padding:8px 12px;border-radius:8px;border:1px solid rgba(120,120,120,0.3);
        background:rgba(120,120,120,0.1);cursor:pointer;font-size:0.85em;
      ">➕ Invitar colaboradores</button>
    </div>
  `;
  container.querySelector(".chat-list-item").addEventListener("click", openChatModal);
  container.querySelector("#chatInviteBtn").addEventListener("click", openInviteModal);
}
// Abre el modal de invitación y carga la lista de colaboradores con cuenta Matrix.
async function openInviteModal() {
  const modal = Modal({ id: "inviteModal" });
  if (!modal) return;
  const list = document.getElementById("inviteList");
  if (list) list.innerHTML = `<div style="padding:12px;color:#999;">Cargando…</div>`;
  modal.open();
  try {
    const res = await authFetch(`${API_BASE}/collaborators/matrix`);
    if (!res.ok) throw new Error("no se pudo cargar la lista");
    const collabs = await res.json();
    renderInviteList(collabs);
  } catch (e) {
    if (list) list.innerHTML = `<div style="padding:12px;color:#f66;">Error al cargar colaboradores.</div>`;
  }
}
function renderInviteList(collabs) {
  const list = document.getElementById("inviteList");
  if (!list) return;
  if (!collabs.length) {
    list.innerHTML = `<div style="padding:12px;color:#999;">No hay colaboradores con cuenta de chat todavía.</div>`;
    return;
  }
  list.innerHTML = collabs
    .map((c) => `
      <div style="display:flex;align-items:center;gap:8px;padding:8px;border-bottom:1px solid rgba(120,120,120,0.2);">
        <span style="flex:1;">${escapeHtml(c.name)}</span>
        <button class="invite-collab-btn" data-user-id="${escapeHtml(c.matrix_user_id)}" style="
          padding:4px 10px;border-radius:6px;border:1px solid rgba(120,120,120,0.3);
          background:rgba(63,163,77,0.25);cursor:pointer;font-size:0.8em;
        ">Invitar</button>
      </div>
    `)
    .join("");
  list.querySelectorAll(".invite-collab-btn").forEach((btn) => {
    btn.addEventListener("click", () => inviteCollaborator(btn));
  });
}
async function inviteCollaborator(btn) {
  const userId = btn.dataset.userId;
  if (!userId || !matrixRoom) return;
  btn.disabled = true;
  btn.textContent = "…";
  try {
    await matrixClient.invite(matrixRoom, userId);
    btn.textContent = "✅";
    btn.style.background = "rgba(63,163,77,0.5)";
  } catch (e) {
    console.error("invite error:", e);
    btn.textContent = "Error";
    btn.style.background = "rgba(214,69,69,0.4)";
    setTimeout(() => {
      btn.textContent = "Invitar";
      btn.style.background = "rgba(63,163,77,0.25)";
      btn.disabled = false;
    }, 2000);
  }
}
function renderChatError(msg) {
  const container = document.getElementById("chatListContainer");
  if (!container) return;
  container.innerHTML = `<div style="padding:20px;color:#f66;">${escapeHtml(msg)}</div>`;
}
// Abre el modal del chat (a pantalla completa).
function openChatModal() {
  const modal = Modal({ id: "chatModal" });
  if (modal) modal.open();
}
function bindChatEvents() {
  const input = document.getElementById("chatInput");
  const sendBtn = document.getElementById("chatSendBtn");
  const send = async () => {
    const text = input.value.trim();
    if (!text || !matrixRoom) return;
    try {
      const content = { msgtype: "m.text", body: text };
      if (replyingTo) {
        content["m.relates_to"] = {
          "m.in_reply_to": { event_id: replyingTo.eventId },
        };
        content["m.reply_preview"] = {
          sender: replyingTo.sender,
          body: replyingTo.body,
          msgtype: replyingTo.msgtype,
        };
      }
      await matrixClient.sendMessage(matrixRoom, content);
      input.value = "";
      autoResizeChatInput();
      clearReplyingTo();
    } catch (e) {
      console.error("send error:", e);
    }
  };
  sendBtn.addEventListener("click", send);
  // Cancelar respuesta.
  const replyCancel = document.getElementById("chatReplyCancel");
  if (replyCancel) replyCancel.addEventListener("click", clearReplyingTo);
  // Adjuntar imagen/video.
  const attachBtn = document.getElementById("chatAttachBtn");
  const fileInput = document.getElementById("chatFileInput");
  if (attachBtn && fileInput) {
    attachBtn.addEventListener("click", () => fileInput.click());
    fileInput.addEventListener("change", async () => {
      for (const file of Array.from(fileInput.files || [])) {
        await sendChatFile(file);
      }
      fileInput.value = "";
    });
  }
  // Enter envía; Shift+Enter hace salto de línea.
  input.addEventListener("keydown", (e) => {
    if (e.key === "Enter" && !e.shiftKey) {
      e.preventDefault();
      send();
    }
  });
  // Auto-resize del textarea.
  input.addEventListener("input", autoResizeChatInput);
  function autoResizeChatInput() {
    input.style.height = "auto";
    input.style.height = Math.min(input.scrollHeight, 120) + "px";
  }
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
  // ===== Infinite scroll =====
  const messagesList = document.getElementById("chatMessages");
let scrollbackToken = room?.getLiveTimeline()?.getPaginationToken?.("b");
  messagesList.addEventListener("scroll", () => {
    if (messagesList.scrollTop === 0 && !loadingOlder) loadOlderMessages();
  });
}
// Set global para no duplicar mensajes ya pintados
const renderedEventIds = new Set();
async function loadOlderMessages() {
  const messagesList = document.getElementById("chatMessages");
  const room = matrixClient.getRoom(matrixRoom);
  if (!room) return;
  const timeline = room.getLiveTimeline();
  const token = timeline?.getPaginationToken?.("b");
  if (!token) return;
  loadingOlder = true;
  // Guardamos la posición de scroll para restaurarla después de insertar
  // arriba (si no, el navegador reajusta el scroll y parece que "salta").
  const prevScrollHeight = messagesList.scrollHeight;
  const prevScrollTop = messagesList.scrollTop;
  try {
    await matrixClient.scrollback(room, 30);
    // No confiamos en result.events — leemos la timeline en vivo, que el
    // SDK mantiene siempre en orden cronológico ascendente.
    const olderEvents = room.getLiveTimeline().getEvents()
      .filter(e => e.getType() === "m.room.message" && !renderedEventIds.has(e.getId()));
    if (olderEvents.length > 0) {
      const firstChild = messagesList.firstChild;
      for (const event of olderEvents) {
        const div = document.createElement("div");
        prependMessage(event, div);
        renderedEventIds.add(event.getId());
        messagesList.insertBefore(div, firstChild);
      }
      // Restaurar posición relativa de scroll.
      messagesList.scrollTop = messagesList.scrollHeight - prevScrollHeight + prevScrollTop;
    }
  } catch (e) {
    console.error("load older messages error:", e);
  } finally {
    loadingOlder = false;
  }
}
  // Renderiza un mensaje en un div para prepend (versión simplificada de appendMessage).
  function prependMessage(event, div) {
    const content = event.getContent();
    const msgtype = content.msgtype;
    const sender = event.getSender();
    const isSelf = sender === matrixClient.getUserId();
    const isMedia = msgtype === "m.image" || msgtype === "m.video";
    const name = isSelf
      ? (window.currentUser?.chat_name || "Tú")
      : sender.split(":")[0].replace("@", "");
    
    div.style.cssText = `
      align-self:${isSelf ? "flex-end" : "flex-start"};
      max-width:75%;
      padding:${isMedia ? "3px" : "8px 12px"};
      border-radius:12px;
      background:${isSelf ? "rgba(63,163,77,0.35)" : "rgba(120,120,120,0.25)"};
      margin-top:8px;
    `;
    let bodyHtml = "";
    if (isMedia) {
      const src = content.url ? matrixClient.mxcUrlToHttp(content.url) : null;
      const gridEl = document.createElement("div");
      gridEl.className = "chat-media-grid";
      const group = { sender, ts: event.getTs(), gridEl, items: [{ msgtype, src: content.url ? matrixClient.mxcUrlToHttp(content.url) : null, body: content.body }] };
      renderMediaGrid(group);
      bodyHtml = "";
      div.innerHTML = `<div class="msg-sender-name" style="font-size:70%;opacity:0.7;">${escapeHtml(name)}</div>`;
      if (isMedia) div.appendChild(group.gridEl);
    } else {
      bodyHtml = `<div style="white-space:pre-wrap;word-break:break-word;">${escapeHtml(content.body || "")}</div>`;
      div.innerHTML = `<div class="msg-sender-name" style="font-size:70%;opacity:0.7;">${escapeHtml(name)}</div>${bodyHtml}`;
    }
  }
// Sube un archivo (imagen/video) y lo envía como mensaje al room.
async function sendChatFile(file) {
  if (!matrixRoom || !file) return;
  const isImage = file.type.startsWith("image/");
  const isVideo = file.type.startsWith("video/");
  if (!isImage && !isVideo) return;
  try {
    const uploadRes = await matrixClient.uploadContent(file, {
      includeFilename: true,
      type: file.type,
    });
    const msgtype = isVideo ? "m.video" : "m.image";
    const content = {
      msgtype,
      body: file.name,
      url: uploadRes.content_uri,
      info: { mimetype: file.type, size: file.size },
    };
    if (replyingTo) {
      content["m.relates_to"] = {
        "m.in_reply_to": { event_id: replyingTo.eventId },
      };
      content["m.reply_preview"] = {
        sender: replyingTo.sender,
        body: replyingTo.body,
        msgtype: replyingTo.msgtype,
      };
    }
    await matrixClient.sendMessage(matrixRoom, content);
    clearReplyingTo();
  } catch (e) {
    console.error("send file error:", e);
    alert("No se pudo enviar el archivo.");
  }
}
// Estado para agrupar mensajes consecutivos del mismo remitente (como WhatsApp).
let lastChatSender = null;
let lastChatTs = 0;
const CHAT_GROUP_WINDOW_MS = 5 * 60 * 1000; // 5 minutos
// Grupo de medios en curso (para armar la cuadrícula estilo WhatsApp).
let lastMediaGroup = null; // { sender, ts, gridEl, items: [] }
const MEDIA_GRID_CAP = 6; // máx. de miniaturas visibles antes de "+N"
function mediaThumbHtml(item, index, isLast, extraCount) {
  const src = item.src;
  let inner;
  if (item.msgtype === "m.video") {
    inner = src
      ? `<video src="${escapeHtml(src)}" muted style="width:100%;height:100%;object-fit:cover;display:block;"></video>`
      : `<div style="color:#999;display:flex;align-items:center;justify-content:center;height:100%;">🎬</div>`;
  } else {
    inner = src
      ? `<img src="${escapeHtml(src)}" alt="${escapeHtml(item.body || "imagen")}" style="width:100%;height:100%;object-fit:cover;display:block;cursor:pointer;" />`
      : `<div style="color:#999;display:flex;align-items:center;justify-content:center;height:100%;">📷</div>`;
  }
  const overlay =
    isLast && extraCount > 0
      ? `<div class="chat-media-more">+${extraCount}</div>`
      : "";
  return `<div class="chat-media-item" data-index="${index}">${inner}${overlay}</div>`;
}
// Reconstruye la cuadrícula completa a partir de group.items. Se llama
// cada vez que se agrega un archivo nuevo al grupo.
function renderMediaGrid(group) {
  const items = group.items;
  const total = items.length;
  let visible = items;
  let extraCount = 0;
  if (total > MEDIA_GRID_CAP) {
    visible = items.slice(0, MEDIA_GRID_CAP - 1);
    extraCount = total - (MEDIA_GRID_CAP - 1);
  }
  const cols = visible.length === 1 ? 1 : visible.length === 2 ? 2 : 3;
  group.gridEl.dataset.count = String(visible.length);
  group.gridEl.style.gridTemplateColumns = `repeat(${cols}, 1fr)`;
  group.gridEl.innerHTML = visible
    .map((item, i) =>
      mediaThumbHtml(item, i, i === visible.length - 1, extraCount),
    )
    .join("");
  // Tocar cualquier miniatura abre el lightbox con TODOS los items del
// ================================================================
//  LIGHTBOX DE MEDIOS — visor a pantalla completa con navegación
// ================================================================
const mediaLightboxShell = Modal({ id: "mediaLightbox" });
let lightboxItems = [];
let lightboxIndex = 0;
function renderLightboxStage() {
  const stage = document.getElementById("lightboxStage");
  const counter = document.getElementById("lightboxCounter");
  const prevBtn = document.getElementById("lightboxPrev");
  const nextBtn = document.getElementById("lightboxNext");
  const item = lightboxItems[lightboxIndex];
  if (!item) return;
  stage.innerHTML =
    item.msgtype === "m.video"
      ? `<video src="${escapeHtml(item.src)}" controls autoplay></video>`
      : `<img src="${escapeHtml(item.src)}" alt="" />`;
  counter.textContent =
    lightboxItems.length > 1
      ? `${lightboxIndex + 1} / ${lightboxItems.length}`
      : "";
  prevBtn.disabled = lightboxIndex === 0;
  nextBtn.disabled = lightboxIndex === lightboxItems.length - 1;
  prevBtn.style.display = lightboxItems.length > 1 ? "" : "none";
  nextBtn.style.display = lightboxItems.length > 1 ? "" : "none";
}
function openLightbox(items, startIndex) {
  lightboxItems = items.filter((it) => it.src);
  const startSrc = items[startIndex]?.src;
  lightboxIndex = Math.max(
    0,
    lightboxItems.findIndex((it) => it.src === startSrc),
  );
  renderLightboxStage();
  mediaLightboxShell.open();
}
document.getElementById("lightboxPrev").addEventListener("click", () => {
  if (lightboxIndex > 0) {
    lightboxIndex--;
    renderLightboxStage();
  }
});
document.getElementById("lightboxNext").addEventListener("click", () => {
  if (lightboxIndex < lightboxItems.length - 1) {
    lightboxIndex++;
    renderLightboxStage();
  }
});
document.addEventListener("keydown", (e) => {
  if (!mediaLightboxShell.isOpen()) return;
  if (e.key === "ArrowLeft") document.getElementById("lightboxPrev").click();
  if (e.key === "ArrowRight") document.getElementById("lightboxNext").click();
});
  // grupo (incluso los ocultos detrás del "+N"), como el álbum de WhatsApp.
  group.gridEl.onclick = (e) => {
    const tile = e.target.closest(".chat-media-item");
    if (!tile) return;
    openLightbox(group.items, parseInt(tile.dataset.index, 10));
  };
}
function appendMessage(event) {
  const eventId = event.getId();
  if (eventId) {
    if (renderedEventIds.has(eventId)) return; // evita duplicados
    renderedEventIds.add(eventId);
  }
  const list = document.getElementById("chatMessages");
  if (!list) return;
  const content = event.getContent();
  const msgtype = content.msgtype;
  const sender = event.getSender();
  const isSelf = sender === matrixClient.getUserId();
  const ts = event.getTs();
  const isMedia = msgtype === "m.image" || msgtype === "m.video";
  // ---- Agrupación de medios estilo WhatsApp: varias fotos/videos
  // consecutivos del mismo remitente se apilan en una sola burbuja con
  // cuadrícula, en vez de una burbuja por archivo.
  if (
    isMedia &&
    lastMediaGroup &&
    sender === lastMediaGroup.sender &&
    ts - lastMediaGroup.ts < CHAT_GROUP_WINDOW_MS
  ) {
    const src = content.url ? matrixClient.mxcUrlToHttp(content.url) : null;
    lastMediaGroup.items.push({ msgtype, src, body: content.body });
    lastMediaGroup.ts = ts;
    renderMediaGrid(lastMediaGroup);
    lastChatSender = sender;
    lastChatTs = ts;
    list.scrollTop = list.scrollHeight;
    return;
  }
  // Agrupar si es el mismo remitente y llegó dentro de la ventana.
  const sameGroup =
    sender === lastChatSender && ts - lastChatTs < CHAT_GROUP_WINDOW_MS;
  lastChatSender = sender;
  lastChatTs = ts;
  // Resuelve el nombre a partir del Matrix user_id, usando un directorio
  // cacheado de /api/collaborators/matrix (el único endpoint que expone
  // matrix_user_id; /api/collaborators no lo incluye).
  const resolveMatrixName = async (userId) => {
    if (!matrixDirectory) matrixDirectory = await loadMatrixDirectory();
    let resolved = matrixDirectory.get(userId);
    if (!resolved) {
      // Puede ser un colaborador nuevo que aún no estaba en el directorio
      // cacheado; refrescamos una vez antes de rendirnos.
      matrixDirectory = await loadMatrixDirectory();
      resolved = matrixDirectory.get(userId);
    }
    return resolved || userId.split(":")[0].replace("@", "");
  };
  // Usamos una promesa para el nombre; si no está listo, mostramos
  // el UUID temporalmente y actualizamos cuando resuelva.
  let name = isSelf
    ? window.currentUser?.chat_name || "Tú"
    : sender.split(":")[0].replace("@", "");
  if (!isSelf) {
    resolveMatrixName(sender).then((resolved) => {
      if (resolved !== name) {
        name = resolved;
        // Actualizar el nombre en el DOM si el mensaje ya se renderizó.
        const nameEl = div.querySelector(".msg-sender-name");
        if (nameEl) nameEl.textContent = resolved;
      }
    });
  }
  const div = document.createElement("div");
  div.style.cssText = `
    align-self:${isSelf ? "flex-end" : "flex-start"};
    max-width:75%;
    padding:${isMedia ? "3px" : sameGroup ? "2px 12px" : "8px 12px"};
    border-radius:12px;
    background:${isSelf ? "rgba(63,163,77,0.35)" : "rgba(120,120,120,0.25)"};
    margin-top:${sameGroup ? "2px" : "8px"};
  `;
  // Renderizar mensaje citado (respuesta) si existe m.reply_preview.
  let replyHtml = "";
  const replyPreview = content["m.reply_preview"];
  if (replyPreview) {
    const replySender = replyPreview.sender === matrixClient.getUserId()
      ? "Tú"
      : replyPreview.sender.split(":")[0].replace("@", "");
    let previewText = replyPreview.body || "";
    if (replyPreview.msgtype === "m.image") previewText = "📷 Imagen";
    else if (replyPreview.msgtype === "m.video") previewText = "🎬 Video";
    previewText = previewText.length > 50 ? previewText.slice(0, 50) + "…" : previewText;
    replyHtml = `<div class="msg-reply" style="margin-bottom:4px;padding:6px 8px;background:rgba(0,0,0,0.15);border-radius:8px;border-left:3px solid #3fa34d;font-size:0.8rem;">
      <div style="font-weight:600;font-size:0.7rem;color:#3fa34d;">${escapeHtml(replySender)}</div>
      <div style="color:#ccc;white-space:pre-wrap;overflow:hidden;text-overflow:ellipsis;">${escapeHtml(previewText)}</div>
    </div>`;
  }
  let bodyHtml = "";
  if (isMedia) {
    const src = content.url ? matrixClient.mxcUrlToHttp(content.url) : null;
    const gridEl = document.createElement("div");
    gridEl.className = "chat-media-grid";
    const group = { sender, ts, gridEl, items: [{ msgtype, src, body: content.body }] };
    renderMediaGrid(group);
    lastMediaGroup = group;
    bodyHtml = ""; // el grid se agrega directamente abajo, no vía innerHTML
  } else {
    lastMediaGroup = null; // un mensaje de texto rompe la cadena de medios
    bodyHtml = `<div style="white-space:pre-wrap;word-break:break-word;">${escapeHtml(content.body || "")}</div>`;
  }
  div.innerHTML = `
    ${sameGroup ? "" : `<div class="msg-sender-name" style="font-size:70%;opacity:0.7;">${escapeHtml(name)}</div>`}
    ${replyHtml}
    ${bodyHtml}
  `;
  if (isMedia) div.appendChild(lastMediaGroup.gridEl);
  // Context menu / long-press para responder (estilo WhatsApp).
  const showReplyMenu = (e) => {
    e.preventDefault();
    const eventId = event.getId();
    if (!eventId) return;
    setReplyingTo(event);
    // Feedback visual breve.
    div.style.outline = "2px solid #3fa34d";
    setTimeout(() => { div.style.outline = ""; }, 300);
  };
  div.addEventListener("contextmenu", showReplyMenu);
  let pressTimer = null;
  div.addEventListener("mousedown", () => {
    pressTimer = setTimeout(showReplyMenu, 500);
  });
  div.addEventListener("mouseup", () => clearTimeout(pressTimer));
  div.addEventListener("mouseleave", () => clearTimeout(pressTimer));
  div.addEventListener("touchstart", () => {
    pressTimer = setTimeout(showReplyMenu, 500);
  }, { passive: true });
  div.addEventListener("touchend", () => clearTimeout(pressTimer));
  div.addEventListener("touchmove", () => clearTimeout(pressTimer));
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
//===== public/paws.svg =====
<svg xmlns="http://www.w3.org/2000/svg" width="400" height="300" viewBox="0 0 400 300">
    <rect width="400" height="300" fill="#e9ecef"/>
    <text x="200" y="150" font-size="90" text-anchor="middle" dominant-baseline="central">🐾</text>
    <text x="200" y="220" font-size="18" fill="#868e96" text-anchor="middle" font-family="sans-serif">Sin foto</text>
</svg>
//===== public/sismoinfo-ui.html =====
<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<meta name="theme-color" content="#0E1B30">
<title>SismoInfo Colombia — Datos que salvan vidas</title>
<meta name="description" content="Red ciudadana de alertas en Colombia. Reporta personas y mascotas desaparecidas, lugares en riesgo y avisos de tu ciudad.">
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=Archivo:wdth,wght@100..125,400..900&family=Public+Sans:wght@400;500;600;700&family=IBM+Plex+Mono:wght@400;500;600&display=swap" rel="stylesheet">
<style>
/* ============ TOKENS ============ */
:root{
  --navy-950:#0A1424;   /* fondo profundo */
  --navy-900:#0E1B30;   /* hero */
  --navy-800:#142540;   /* azul de marca (muestreado del logo) */
  --navy-600:#24406B;
  --slate-500:#5A6879;
  --slate-300:#AEB8C4;
  --paper:#F4F6F9;
  --white:#FFFFFF;
  --red-700:#9D2220;    /* rojo de marca (muestreado del logo) */
  --red-600:#C0201E;
  --red-500:#E03A32;    /* alerta / pulso */
  --amber:#E5A320;      /* estado: en revisión */
  --green:#1F8A5B;      /* estado: resuelto */
  --line:#DCE2EA;
  --r-sm:8px; --r-md:14px; --r-lg:22px;
  --shadow:0 1px 2px rgba(14,27,48,.06), 0 12px 28px -14px rgba(14,27,48,.28);
  --header-h:60px;
  --display:'Archivo', system-ui, sans-serif;
  --body:'Public Sans', system-ui, sans-serif;
  --mono:'IBM Plex Mono', ui-monospace, monospace;
}
*,*::before,*::after{box-sizing:border-box}
html{-webkit-text-size-adjust:100%}
body{
  margin:0; background:var(--paper); color:var(--navy-900);
  font-family:var(--body); font-size:16px; line-height:1.55;
  -webkit-font-smoothing:antialiased; padding-bottom:env(safe-area-inset-bottom);
}
h1,h2,h3{font-family:var(--display); font-weight:800; letter-spacing:-.02em; line-height:1.05; margin:0}
p{margin:0}
a{color:inherit}
button{font:inherit; cursor:pointer}
img{max-width:100%; display:block}
:focus-visible{outline:3px solid var(--red-500); outline-offset:2px; border-radius:4px}
.wrap{width:min(1180px,100% - 40px); margin-inline:auto}
@media(max-width:640px){.wrap{width:min(1180px,100% - 28px)}}
.skip{position:absolute;left:-9999px}
.skip:focus{left:12px;top:12px;z-index:200;background:var(--white);padding:10px 16px;border-radius:var(--r-sm)}
.mono{font-family:var(--mono); font-variant-numeric:tabular-nums}
.eyebrow{
  font-family:var(--mono); font-size:11px; letter-spacing:.18em; text-transform:uppercase;
  color:var(--slate-500);
}
/* ============ HEADER ============ */
.header{
  position:sticky; top:0; z-index:60; height:var(--header-h);
  background:rgba(14,27,48,.94); backdrop-filter:blur(10px);
  border-bottom:1px solid rgba(255,255,255,.08); color:var(--white);
}
.header .wrap{height:100%; display:flex; align-items:center; gap:14px}
.brand{display:flex; align-items:center; gap:10px; text-decoration:none; margin-right:auto}
.brand svg{width:32px; height:32px; flex:none}
.brand b{font-family:var(--display); font-weight:900; font-size:18px; letter-spacing:-.02em}
.brand b i{font-style:normal; color:var(--red-500)}
.brand small{display:block; font-family:var(--mono); font-size:9px; letter-spacing:.16em; color:var(--slate-300); margin-top:-2px}
.city-pick{
  display:flex; align-items:center; gap:6px; background:rgba(255,255,255,.09);
  border:1px solid rgba(255,255,255,.14); color:var(--white);
  border-radius:999px; padding:7px 12px; font-size:13px; font-weight:600;
}
.city-pick select{all:unset; color:var(--white); font-weight:600; font-size:13px; cursor:pointer}
.city-pick select option{color:#000}
.btn-share{
  background:transparent; border:1px solid rgba(255,255,255,.2); color:var(--white);
  border-radius:999px; width:36px; height:36px; display:grid; place-items:center;
}
.btn-share:hover{background:rgba(255,255,255,.1)}
.btn-report{
  background:var(--red-600); color:var(--white); border:0; border-radius:999px;
  padding:9px 18px; font-weight:700; font-size:14px; box-shadow:0 6px 18px -8px var(--red-600);
}
.btn-report:hover{background:var(--red-500)}
@media(max-width:760px){.btn-report{display:none} .city-pick{margin-left:auto}}
/* ============ HERO ============ */
.hero{
  background:var(--navy-900); color:var(--white); position:relative; overflow:hidden;
  padding:52px 0 0;
}
.hero::after{ /* anillos de epicentro, del logo */
  content:""; position:absolute; right:-160px; top:-120px; width:520px; height:520px;
  border-radius:50%; pointer-events:none; opacity:.5;
  background:
    radial-gradient(circle, transparent 0 28%, rgba(224,58,50,.5) 28% 28.6%, transparent 28.6%),
    radial-gradient(circle, transparent 0 42%, rgba(255,255,255,.12) 42% 42.4%, transparent 42.4%),
    radial-gradient(circle, transparent 0 58%, rgba(255,255,255,.08) 58% 58.3%, transparent 58.3%),
    radial-gradient(circle, transparent 0 74%, rgba(255,255,255,.05) 74% 74.2%, transparent 74.2%);
}
.hero-grid{display:grid; gap:34px; grid-template-columns:1.15fr .85fr; align-items:center; position:relative; z-index:2}
@media(max-width:900px){.hero-grid{grid-template-columns:1fr; gap:26px}}
.hero .eyebrow{color:var(--slate-300)}
.hero h1{font-size:clamp(36px,6.4vw,62px); margin:12px 0 14px; font-stretch:112%}
.hero h1 em{font-style:normal; color:var(--red-500)}
.hero p.lede{color:#C6D0DC; font-size:17px; max-width:46ch}
.hero-actions{display:flex; gap:12px; flex-wrap:wrap; margin-top:26px}
.btn{
  border:0; border-radius:var(--r-md); padding:14px 22px; font-weight:700; font-size:15px;
  display:inline-flex; align-items:center; gap:9px; text-decoration:none;
}
.btn-primary{background:var(--red-600); color:#fff; box-shadow:0 10px 26px -12px var(--red-500)}
.btn-primary:hover{background:var(--red-500)}
.btn-ghost{background:rgba(255,255,255,.08); color:#fff; border:1px solid rgba(255,255,255,.18)}
.btn-ghost:hover{background:rgba(255,255,255,.15)}
/* tarjeta de última alerta */
.last-alert{
  background:rgba(255,255,255,.06); border:1px solid rgba(255,255,255,.14);
  border-radius:var(--r-lg); padding:18px; backdrop-filter:blur(6px);
}
.last-alert .row{display:flex; align-items:center; gap:10px; margin-bottom:14px}
.live-dot{width:8px;height:8px;border-radius:50%;background:var(--red-500);box-shadow:0 0 0 0 rgba(224,58,50,.6);animation:ping 2s infinite}
@keyframes ping{70%{box-shadow:0 0 0 10px rgba(224,58,50,0)}100%{box-shadow:0 0 0 0 rgba(224,58,50,0)}}
.last-alert h3{font-size:17px; color:#fff; margin-bottom:4px}
.last-alert .meta{font-family:var(--mono); font-size:12px; color:var(--slate-300)}
.last-alert .thumb{width:100%; height:132px; border-radius:var(--r-md); background:linear-gradient(135deg,#1B3054,#0F1F38); border:1px solid rgba(255,255,255,.1); display:grid; place-items:center; color:var(--slate-500); font-size:12px; margin-bottom:14px; font-family:var(--mono)}
/* ---- FIRMA: línea sismográfica ---- */
.seismo{display:block; width:100%; height:78px; margin-top:34px}
.seismo path{fill:none; stroke:var(--red-500); stroke-width:2.4; stroke-linejoin:round; stroke-linecap:round}
.seismo .base{stroke:rgba(255,255,255,.16); stroke-width:1}
.seismo .trace{stroke-dasharray:2400; stroke-dashoffset:2400; animation:draw 3.4s cubic-bezier(.6,0,.2,1) forwards}
@keyframes draw{to{stroke-dashoffset:0}}
/* ============ CONTADORES ============ */
.stats{background:var(--navy-800); color:#fff; border-top:1px solid rgba(255,255,255,.08)}
.stats .wrap{display:grid; grid-template-columns:repeat(4,1fr); gap:1px; background:rgba(255,255,255,.08)}
.stat{background:var(--navy-800); padding:18px 4px; text-align:center}
.stat b{display:block; font-family:var(--mono); font-size:clamp(20px,3.4vw,28px); font-weight:600; letter-spacing:-.03em}
.stat span{font-size:11px; letter-spacing:.1em; text-transform:uppercase; color:var(--slate-300)}
.stat.hi b{color:var(--red-500)}
@media(max-width:560px){.stats .wrap{grid-template-columns:repeat(2,1fr)}}
/* ============ TABS ============ */
.tabbar{position:sticky; top:var(--header-h); z-index:50; background:var(--white); border-bottom:1px solid var(--line)}
.tabs{display:flex; gap:6px; overflow-x:auto; scrollbar-width:none; padding:10px 0}
.tabs::-webkit-scrollbar{display:none}
.tab{
  flex:none; background:transparent; border:1px solid var(--line); border-radius:999px;
  padding:9px 16px; font-weight:600; font-size:14px; color:var(--slate-500);
  display:inline-flex; align-items:center; gap:7px; white-space:nowrap;
}
.tab:hover{border-color:var(--navy-600); color:var(--navy-800)}
.tab[aria-selected="true"]{background:var(--navy-800); border-color:var(--navy-800); color:#fff}
.tab .n{font-family:var(--mono); font-size:11px; padding:1px 6px; border-radius:999px; background:rgba(0,0,0,.08)}
.tab[aria-selected="true"] .n{background:var(--red-600); color:#fff}
/* ============ CONTENIDO ============ */
main{padding:28px 0 90px}
.toolbar{display:flex; gap:10px; flex-wrap:wrap; align-items:center; margin-bottom:22px}
.search{
  flex:1 1 260px; display:flex; align-items:center; gap:9px; background:var(--white);
  border:1px solid var(--line); border-radius:var(--r-md); padding:11px 14px;
}
.search input{all:unset; flex:1; font-size:15px}
.chips{display:flex; gap:8px; flex-wrap:wrap}
.chip{background:var(--white); border:1px solid var(--line); border-radius:999px; padding:8px 14px; font-size:13px; font-weight:600; color:var(--slate-500)}
.chip[aria-pressed="true"]{background:var(--navy-800); color:#fff; border-color:var(--navy-800)}
.section-head{display:flex; align-items:baseline; justify-content:space-between; gap:16px; margin:6px 0 16px}
.section-head h2{font-size:clamp(22px,3vw,30px)}
.grid{display:grid; grid-template-columns:repeat(auto-fill,minmax(255px,1fr)); gap:16px}
.card{
  background:var(--white); border:1px solid var(--line); border-radius:var(--r-lg);
  overflow:hidden; box-shadow:var(--shadow); display:flex; flex-direction:column;
  transition:transform .18s ease, box-shadow .18s ease;
}
.card:hover{transform:translateY(-3px); box-shadow:0 1px 2px rgba(14,27,48,.06),0 22px 40px -20px rgba(14,27,48,.4)}
.card .photo{aspect-ratio:4/3; background:linear-gradient(150deg,#E7ECF2,#D7DEE8); display:grid; place-items:center; position:relative; color:var(--slate-500); font-family:var(--mono); font-size:12px}
.status{
  position:absolute; top:10px; left:10px; font-size:11px; font-weight:700; letter-spacing:.06em;
  text-transform:uppercase; padding:5px 10px; border-radius:999px; color:#fff; font-family:var(--body);
}
.s-urgente{background:var(--red-600)}
.s-revision{background:var(--amber); color:#3B2A00}
.s-resuelto{background:var(--green)}
.card .body{padding:14px 16px 16px; display:flex; flex-direction:column; gap:8px; flex:1}
.card h3{font-size:17px}
.card .meta{font-family:var(--mono); font-size:12px; color:var(--slate-500); display:flex; gap:10px; flex-wrap:wrap}
.card .desc{font-size:14px; color:var(--slate-500); flex:1}
.card .act{display:flex; gap:8px; margin-top:4px}
.btn-sm{border-radius:var(--r-sm); padding:9px 12px; font-size:13px; font-weight:700; border:1px solid var(--line); background:var(--white); color:var(--navy-800); flex:1}
.btn-sm.fill{background:var(--navy-800); color:#fff; border-color:var(--navy-800)}
.btn-sm.fill:hover{background:var(--navy-600)}
/* mapa */
.map-panel{border-radius:var(--r-lg); overflow:hidden; border:1px solid var(--line); background:var(--navy-900); position:relative; min-height:440px; display:grid; place-items:center; color:var(--slate-300)}
.radar{width:220px; height:220px; border-radius:50%; border:1px solid rgba(255,255,255,.18); position:relative; display:grid; place-items:center}
.radar::before,.radar::after{content:""; position:absolute; inset:0; border-radius:50%; border:1.5px solid var(--red-500); opacity:0; animation:radar 3s ease-out infinite}
.radar::after{animation-delay:1.5s}
@keyframes radar{0%{transform:scale(.25);opacity:.8}100%{transform:scale(1);opacity:0}}
.map-panel .pin{width:14px;height:14px;border-radius:50%;background:var(--red-500);box-shadow:0 0 20px var(--red-500)}
.map-note{position:absolute; bottom:16px; left:16px; font-family:var(--mono); font-size:12px}
/* pasos (secuencia real) */
.steps{display:grid; grid-template-columns:repeat(3,1fr); gap:18px; margin-top:14px}
@media(max-width:820px){.steps{grid-template-columns:1fr}}
.step{background:var(--white); border:1px solid var(--line); border-radius:var(--r-lg); padding:20px}
.step .num{font-family:var(--mono); font-size:12px; color:var(--red-600); font-weight:600; letter-spacing:.1em}
.step h3{font-size:18px; margin:8px 0 6px}
.step p{font-size:14px; color:var(--slate-500)}
/* línea de emergencia */
.emergency{background:var(--red-700); color:#fff; border-radius:var(--r-lg); padding:20px 22px; margin-top:34px; display:flex; gap:18px; align-items:center; flex-wrap:wrap}
.emergency h3{font-size:18px}
.emergency p{font-size:14px; color:#F6D6D5; max-width:52ch}
.dials{display:flex; gap:10px; margin-left:auto; flex-wrap:wrap}
.dial{background:rgba(255,255,255,.14); border:1px solid rgba(255,255,255,.25); border-radius:var(--r-md); padding:9px 14px; text-align:center; text-decoration:none}
.dial b{display:block; font-family:var(--mono); font-size:19px}
.dial span{font-size:11px; letter-spacing:.08em; text-transform:uppercase; color:#F6D6D5}
/* footer */
footer{background:var(--navy-950); color:var(--slate-300); padding:44px 0 100px; font-size:14px}
footer .cols{display:grid; grid-template-columns:1.4fr 1fr 1fr; gap:28px}
@media(max-width:760px){footer .cols{grid-template-columns:1fr 1fr} footer .cols>:first-child{grid-column:1/-1}}
footer h4{font-family:var(--display); color:#fff; font-size:13px; letter-spacing:.12em; text-transform:uppercase; margin:0 0 12px}
footer a{display:block; padding:4px 0; text-decoration:none}
footer a:hover{color:#fff}
.legal{border-top:1px solid rgba(255,255,255,.1); margin-top:30px; padding-top:18px; font-family:var(--mono); font-size:11px; color:var(--slate-500)}
/* nav inferior móvil */
.bottomnav{
  position:fixed; bottom:0; left:0; right:0; z-index:70; display:none;
  background:rgba(255,255,255,.97); backdrop-filter:blur(10px); border-top:1px solid var(--line);
  padding:6px 4px calc(6px + env(safe-area-inset-bottom));
}
.bottomnav .inner{display:grid; grid-template-columns:repeat(5,1fr)}
.bnav{background:none; border:0; padding:7px 2px; display:grid; justify-items:center; gap:3px; font-size:10px; font-weight:600; color:var(--slate-500)}
.bnav[aria-selected="true"]{color:var(--red-600)}
.bnav .fab{background:var(--red-600); color:#fff; width:40px; height:40px; border-radius:50%; display:grid; place-items:center; margin-top:-16px; box-shadow:0 8px 18px -6px var(--red-600)}
@media(max-width:760px){.bottomnav{display:block} main{padding-bottom:120px}}
/* modal */
.modal[hidden]{display:none}
.modal{position:fixed; inset:0; z-index:120; display:grid; place-items:end center}
.modal .backdrop{position:absolute; inset:0; background:rgba(10,20,36,.6); backdrop-filter:blur(3px)}
.sheet{
  position:relative; background:var(--white); width:min(560px,100%); max-height:92vh; overflow:auto;
  border-radius:var(--r-lg) var(--r-lg) 0 0; padding:22px 22px calc(22px + env(safe-area-inset-bottom));
}
@media(min-width:620px){.modal{place-items:center} .sheet{border-radius:var(--r-lg)}}
.sheet header{display:flex; align-items:center; justify-content:space-between; margin-bottom:16px}
.sheet h2{font-size:22px}
.x{background:var(--paper); border:0; border-radius:50%; width:34px; height:34px; font-size:18px; color:var(--slate-500)}
label{display:block; font-size:13px; font-weight:700; margin:14px 0 6px}
input[type=text],input[type=url],textarea,select{
  width:100%; border:1px solid var(--line); border-radius:var(--r-md); padding:12px 14px;
  font:inherit; font-size:15px; background:var(--white); color:var(--navy-900);
}
textarea{min-height:92px; resize:vertical}
.img-choice{display:grid; grid-template-columns:1fr 1fr; gap:10px; margin-top:6px}
.drop{border:1.5px dashed var(--line); border-radius:var(--r-md); padding:16px; text-align:center; font-size:13px; color:var(--slate-500)}
.geo{display:flex; gap:8px}
.geo button{flex:none; border:1px solid var(--line); background:var(--paper); border-radius:var(--r-md); padding:0 14px; font-weight:700; font-size:13px}
.sheet .submit{width:100%; margin-top:20px; background:var(--red-600); color:#fff; border:0; border-radius:var(--r-md); padding:15px; font-weight:800; font-size:16px}
.hint{font-size:12px; color:var(--slate-500); margin-top:8px}
/* toast */
.toast{
  position:fixed; left:50%; bottom:96px; transform:translate(-50%,20px); z-index:200;
  background:var(--navy-800); color:#fff; padding:13px 20px; border-radius:999px;
  font-size:14px; font-weight:600; opacity:0; pointer-events:none; transition:.28s;
}
.toast.show{opacity:1; transform:translate(-50%,0)}
@media (prefers-reduced-motion:reduce){
  *{animation:none !important; transition:none !important}
  .seismo .trace{stroke-dashoffset:0}
}
</style>
</head>
<body>
<a class="skip" href="#contenido">Ir al contenido</a>
<!-- ============ HEADER ============ -->
<header class="header">
  <div class="wrap">
    <a class="brand" href="/">
      <svg viewBox="0 0 40 40" aria-hidden="true">
        <circle cx="20" cy="20" r="17" fill="none" stroke="#E03A32" stroke-width="2.6" stroke-dasharray="62 45" transform="rotate(35 20 20)"/>
        <circle cx="20" cy="20" r="17" fill="none" stroke="#4E6C9B" stroke-width="2.6" stroke-dasharray="42 65" transform="rotate(200 20 20)"/>
        <path d="M8 20h4.5l2-6 3 12 2.5-8 2 4H28" fill="none" stroke="#E03A32" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"/>
        <circle cx="30" cy="11" r="4.2" fill="#E03A32"/>
      </svg>
      <span>
        <b>SISMO<i>INFO</i></b>
        <small>DATOS QUE SALVAN VIDAS</small>
      </span>
    </a>
    <div class="city-pick">
      <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2"><path d="M12 21s7-6.4 7-11a7 7 0 10-14 0c0 4.6 7 11 7 11z"/><circle cx="12" cy="10" r="2.4"/></svg>
      <select id="ciudad" aria-label="Elegir ciudad">
        <option>Cali</option><option>Yumbo</option><option>Bogotá</option>
        <option>Medellín</option><option>Barranquilla</option><option>Palmira</option>
        <option>Buenaventura</option><option>Todo el país</option>
      </select>
    </div>
    <button class="btn-share" id="share" aria-label="Compartir la app">
      <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="18" cy="5" r="3"/><circle cx="6" cy="12" r="3"/><circle cx="18" cy="19" r="3"/><path d="M8.6 13.5l6.8 4M15.4 6.5l-6.8 4"/></svg>
    </button>
    <button class="btn-report" data-open-modal>Crear reporte</button>
  </div>
</header>
<!-- ============ HERO ============ -->
<section class="hero">
  <div class="wrap">
    <div class="hero-grid">
      <div>
        <p class="eyebrow">Colombia · Red ciudadana de alertas</p>
        <h1>Cuando alguien se pierde, <em>los primeros minutos</em> deciden.</h1>
        <p class="lede">Publica un reporte en menos de un minuto y llega a la gente que está cerca ahora mismo. Personas, mascotas, lugares en riesgo y avisos de tu barrio, en un solo mapa.</p>
        <div class="hero-actions">
          <button class="btn btn-primary" data-open-modal>
            <svg width="17" height="17" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.4"><path d="M12 5v14M5 12h14"/></svg>
            Crear reporte
          </button>
          <a class="btn btn-ghost" href="#mapa">
            <svg width="17" height="17" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M9 4L3 7v13l6-3 6 3 6-3V4l-6 3-6-3z"/></svg>
            Ver el mapa
          </a>
        </div>
      </div>
      <aside class="last-alert" aria-label="Último reporte">
        <div class="row"><span class="live-dot"></span><span class="eyebrow" style="color:#fff">Último reporte</span></div>
        <div class="thumb">FOTO DEL REPORTE</div>
        <h3>Mujer, 68 años — sector Meléndez</h3>
        <p class="meta">CALI · HACE 14 MIN · 3.3906 N, 76.5322 W</p>
      </aside>
    </div>
    <!-- FIRMA: trazo sismográfico -->
    <svg class="seismo" viewBox="0 0 1200 78" preserveAspectRatio="none" aria-hidden="true">
      <path class="base" d="M0 39H1200"/>
      <path class="trace" d="M0 39h120l14-9 12 18 14-30 16 46 15-58 14 70 16-40 13 20 12-8h130l18-14 14 26 16-38 15 50 14-24 13 10h150l16-12 14 22 15-34 16 44 14-20 13 8h180l16-10 15 20 14-30 16 40 15-22 14 12H1200"/>
    </svg>
  </div>
</section>
<!-- ============ CONTADORES ============ -->
<section class="stats" aria-label="Actividad de la red">
  <div class="wrap">
    <div class="stat hi"><b>27</b><span>Activos hoy</span></div>
    <div class="stat"><b>1 482</b><span>Reportes totales</span></div>
    <div class="stat"><b>316</b><span>Casos resueltos</span></div>
    <div class="stat"><b>64</b><span>Ciudades</span></div>
  </div>
</section>
<!-- ============ TABS ============ -->
<nav class="tabbar" aria-label="Tipo de reporte">
  <div class="wrap">
    <div class="tabs" role="tablist">
      <button class="tab" role="tab" aria-selected="true" data-tab="personas">🫂 Personas <span class="n">12</span></button>
      <button class="tab" role="tab" aria-selected="false" data-tab="mascotas">🐕 Mascotas <span class="n">9</span></button>
      <button class="tab" role="tab" aria-selected="false" data-tab="lugares">🏢 Lugares <span class="n">4</span></button>
      <button class="tab" role="tab" aria-selected="false" data-tab="anuncios">📣 Anuncios <span class="n">2</span></button>
      <button class="tab" role="tab" aria-selected="false" data-tab="mapa">🗺️ Mapa</button>
    </div>
  </div>
</nav>
<main id="contenido">
  <div class="wrap">
    <div class="toolbar">
      <div class="search">
        <svg width="17" height="17" viewBox="0 0 24 24" fill="none" stroke="#5A6879" stroke-width="2"><circle cx="11" cy="11" r="7"/><path d="M20 20l-3.5-3.5"/></svg>
        <input type="text" placeholder="Buscar por nombre, barrio o seña particular" aria-label="Buscar reportes">
      </div>
      <div class="chips">
        <button class="chip" aria-pressed="true">Todos</button>
        <button class="chip" aria-pressed="false">Urgentes</button>
        <button class="chip" aria-pressed="false">Últimas 24 h</button>
        <button class="chip" aria-pressed="false">Resueltos</button>
      </div>
    </div>
    <!-- PANEL: PERSONAS -->
    <section id="panel-personas" data-panel>
      <div class="section-head">
        <h2>Personas reportadas</h2>
        <span class="eyebrow" id="ciudad-eco">Cali · 12 activos</span>
      </div>
      <div class="grid">
        <article class="card">
          <div class="photo"><span class="status s-urgente">Urgente</span>FOTO</div>
          <div class="body">
            <h3>Ana Lucía R., 68 años</h3>
            <p class="meta"><span>CALI</span><span>HACE 14 MIN</span></p>
            <p class="desc">Salió de casa en Meléndez sin celular. Viste blusa azul y sandalias. Tiene pérdida de memoria.</p>
            <div class="act"><button class="btn-sm fill">Tengo información</button><button class="btn-sm" aria-label="Compartir">↗</button></div>
          </div>
        </article>
        <article class="card">
          <div class="photo"><span class="status s-revision">En revisión</span>FOTO</div>
          <div class="body">
            <h3>Jhon Steven M., 17 años</h3>
            <p class="meta"><span>YUMBO</span><span>HACE 2 H</span></p>
            <p class="desc">No regresó del colegio en Ciudad Guabinas. Uniforme gris, morral negro.</p>
            <div class="act"><button class="btn-sm fill">Tengo información</button><button class="btn-sm" aria-label="Compartir">↗</button></div>
          </div>
        </article>
        <article class="card">
          <div class="photo"><span class="status s-resuelto">Encontrado</span>FOTO</div>
          <div class="body">
            <h3>Marta E. G., 41 años</h3>
            <p class="meta"><span>PALMIRA</span><span>AYER 19:40</span></p>
            <p class="desc">Reunida con su familia. Gracias a quienes compartieron el reporte.</p>
            <div class="act"><button class="btn-sm">Ver el caso</button></div>
          </div>
        </article>
        <article class="card">
          <div class="photo"><span class="status s-urgente">Urgente</span>FOTO</div>
          <div class="body">
            <h3>Hombre sin identificar, ~55 años</h3>
            <p class="meta"><span>CALI</span><span>HACE 5 H</span></p>
            <p class="desc">Desorientado cerca de la Terminal. Buscamos a su familia.</p>
            <div class="act"><button class="btn-sm fill">Tengo información</button><button class="btn-sm" aria-label="Compartir">↗</button></div>
          </div>
        </article>
      </div>
    </section>
    <!-- PANEL: MASCOTAS -->
    <section id="panel-mascotas" data-panel hidden>
      <div class="section-head"><h2>Mascotas perdidas y encontradas</h2><span class="eyebrow">Cali · 9 activos</span></div>
      <div class="grid">
        <article class="card">
          <div class="photo"><span class="status s-urgente">Perdido</span>FOTO</div>
          <div class="body">
            <h3>Kira — criolla café</h3>
            <p class="meta"><span>CALI · SAN FERNANDO</span><span>HACE 40 MIN</span></p>
            <p class="desc">Collar rojo sin placa. Responde al nombre. Le teme a la moto.</p>
            <div class="act"><button class="btn-sm fill">La vi</button><button class="btn-sm" aria-label="Compartir">↗</button></div>
          </div>
        </article>
        <article class="card">
          <div class="photo"><span class="status s-revision">Encontrado en la calle</span>FOTO</div>
          <div class="body">
            <h3>Gato gris, adulto</h3>
            <p class="meta"><span>YUMBO</span><span>HACE 3 H</span></p>
            <p class="desc">Está resguardado en una casa del barrio. Busca a su dueño.</p>
            <div class="act"><button class="btn-sm fill">Es mío</button><button class="btn-sm" aria-label="Compartir">↗</button></div>
          </div>
        </article>
      </div>
    </section>
    <!-- PANEL: LUGARES -->
    <section id="panel-lugares" data-panel hidden>
      <div class="section-head"><h2>Lugares en riesgo</h2><span class="eyebrow">Reportados por la comunidad</span></div>
      <div class="grid">
        <article class="card">
          <div class="photo"><span class="status s-urgente">Riesgo activo</span>FOTO</div>
          <div class="body">
            <h3>Talud inestable — vía Cali–Yumbo</h3>
            <p class="meta"><span>YUMBO</span><span>HACE 1 H</span></p>
            <p class="desc">Desprendimiento de tierra sobre el carril derecho tras la lluvia de anoche.</p>
            <div class="act"><button class="btn-sm fill">Confirmar</button><button class="btn-sm" aria-label="Compartir">↗</button></div>
          </div>
        </article>
        <article class="card">
          <div class="photo"><span class="status s-revision">Sin verificar</span>FOTO</div>
          <div class="body">
            <h3>Poste con cables sueltos</h3>
            <p class="meta"><span>CALI · EL VALLADO</span><span>HACE 6 H</span></p>
            <p class="desc">Cables a la altura de la acera frente al parque.</p>
            <div class="act"><button class="btn-sm fill">Confirmar</button><button class="btn-sm" aria-label="Compartir">↗</button></div>
          </div>
        </article>
      </div>
    </section>
    <!-- PANEL: ANUNCIOS -->
    <section id="panel-anuncios" data-panel hidden>
      <div class="section-head"><h2>Avisos de la comunidad</h2><span class="eyebrow">Cali · 2 activos</span></div>
      <div class="grid">
        <article class="card">
          <div class="photo">FOTO</div>
          <div class="body">
            <h3>Jornada de vacunación de mascotas</h3>
            <p class="meta"><span>YUMBO</span><span>SÁB 09:00</span></p>
            <p class="desc">Gratuita, en el parque principal. Lleva a tu perro con correa.</p>
            <div class="act"><button class="btn-sm fill">Ver detalles</button></div>
          </div>
        </article>
      </div>
    </section>
    <!-- PANEL: MAPA -->
    <section id="panel-mapa" data-panel hidden>
      <div class="section-head"><h2 id="mapa">Mapa en vivo</h2><span class="eyebrow">Actualizado hace 1 min</span></div>
      <div class="map-panel">
        <div class="radar"><span class="pin"></span></div>
        <p class="map-note">CONECTA AQUÍ TU MAPA · 27 MARCADORES ACTIVOS</p>
      </div>
    </section>
    <!-- CÓMO FUNCIONA (secuencia real) -->
    <section aria-labelledby="pasos" style="margin-top:52px">
      <p class="eyebrow">Cómo funciona</p>
      <h2 id="pasos" style="font-size:clamp(22px,3vw,30px); margin-top:8px">Tres pasos, un minuto</h2>
      <div class="steps">
        <div class="step"><span class="num">PASO 01</span><h3>Publica</h3><p>Foto, ciudad y una descripción corta. Marca el punto en el mapa o usa tu ubicación.</p></div>
        <div class="step"><span class="num">PASO 02</span><h3>Difunde</h3><p>Comparte el enlace por WhatsApp. Quien esté cerca lo ve primero.</p></div>
        <div class="step"><span class="num">PASO 03</span><h3>Recibe pistas</h3><p>Las personas responden en el reporte. Ciérralo cuando el caso se resuelva.</p></div>
      </div>
    </section>
    <!-- EMERGENCIA -->
    <section class="emergency">
      <div>
        <h3>SismoInfo no reemplaza a las autoridades</h3>
        <p>Si hay riesgo para la vida, llama primero. Publicar aquí ayuda a difundir, no a atender la emergencia.</p>
      </div>
      <div class="dials">
        <a class="dial" href="tel:123"><b>123</b><span>Emergencias</span></a>
        <a class="dial" href="tel:119"><b>119</b><span>Bomberos</span></a>
        <a class="dial" href="tel:132"><b>132</b><span>Cruz Roja</span></a>
      </div>
    </section>
  </div>
</main>
<!-- ============ FOOTER ============ -->
<footer>
  <div class="wrap">
    <div class="cols">
      <div>
        <a class="brand" href="/" style="margin-bottom:12px">
          <svg viewBox="0 0 40 40" aria-hidden="true">
            <circle cx="20" cy="20" r="17" fill="none" stroke="#E03A32" stroke-width="2.6" stroke-dasharray="62 45" transform="rotate(35 20 20)"/>
            <circle cx="20" cy="20" r="17" fill="none" stroke="#4E6C9B" stroke-width="2.6" stroke-dasharray="42 65" transform="rotate(200 20 20)"/>
            <path d="M8 20h4.5l2-6 3 12 2.5-8 2 4H28" fill="none" stroke="#E03A32" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"/>
            <circle cx="30" cy="11" r="4.2" fill="#E03A32"/>
          </svg>
          <span><b style="color:#fff">SISMO<i>INFO</i></b><small>DATOS QUE SALVAN VIDAS</small></span>
        </a>
        <p style="max-width:38ch">Red ciudadana de reportes en Colombia. Hecha para difundir rápido lo que pasa cerca de ti.</p>
      </div>
      <div>
        <h4>Reportar</h4>
        <a href="#">Personas</a><a href="#">Mascotas</a><a href="#">Lugares</a><a href="#">Anuncios</a>
      </div>
      <div>
        <h4>Contacto</h4>
        <a href="https://instagram.com/neokpm">Instagram @neokpm</a>
        <a href="https://wa.me/573153410282">WhatsApp +57 315 341 0282</a>
        <a href="https://colombiatebusca.com">colombiatebusca.com</a>
      </div>
    </div>
    <p class="legal">SISMOINFO.CO · LOS REPORTES SON PUBLICADOS POR USUARIOS Y NO SON VERIFICADOS OFICIALMENTE.</p>
  </div>
</footer>
<!-- ============ NAV INFERIOR MÓVIL ============ -->
<nav class="bottomnav" aria-label="Navegación principal">
  <div class="inner">
    <button class="bnav" data-tab="personas" aria-selected="true"><span>🫂</span>Personas</button>
    <button class="bnav" data-tab="mascotas" aria-selected="false"><span>🐕</span>Mascotas</button>
    <button class="bnav" data-open-modal aria-label="Crear reporte"><span class="fab"><svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.6"><path d="M12 5v14M5 12h14"/></svg></span></button>
    <button class="bnav" data-tab="lugares" aria-selected="false"><span>🏢</span>Lugares</button>
    <button class="bnav" data-tab="mapa" aria-selected="false"><span>🗺️</span>Mapa</button>
  </div>
</nav>
<!-- ============ MODAL ============ -->
<div class="modal" id="modal" hidden role="dialog" aria-modal="true" aria-labelledby="modal-title">
  <div class="backdrop" data-close></div>
  <div class="sheet">
    <header>
      <h2 id="modal-title">Crear reporte</h2>
      <button class="x" data-close aria-label="Cerrar">✕</button>
    </header>
    <label for="tipo">Tipo de reporte</label>
    <select id="tipo"><option>Persona</option><option>Mascota</option><option>Lugar en riesgo</option><option>Anuncio</option></select>
    <label for="nombre">Nombre o título</label>
    <input id="nombre" type="text" placeholder="Ej.: Ana Lucía, 68 años">
    <label for="ciudad2">Ciudad</label>
    <select id="ciudad2"><option>Cali</option><option>Yumbo</option><option>Bogotá</option><option>Medellín</option><option>Palmira</option></select>
    <label for="ubi">Ubicación</label>
    <div class="geo">
      <input id="ubi" type="text" placeholder="Barrio, dirección o punto de referencia">
      <button type="button" id="geo">📍 Usar la mía</button>
    </div>
    <label>Foto</label>
    <div class="img-choice">
      <input type="url" placeholder="Pegar URL de imagen" aria-label="URL de la imagen">
      <label class="drop" for="file" style="margin:0; font-weight:500">📷 Subir foto<input id="file" type="file" accept="image/*" hidden></label>
    </div>
    <p class="hint">Una foto clara del rostro o de una seña particular multiplica las respuestas.</p>
    <label for="desc">Descripción</label>
    <textarea id="desc" placeholder="Ropa, estatura, hora y lugar donde se vio por última vez, condiciones de salud…"></textarea>
    <label for="tel">Teléfono de contacto</label>
    <input id="tel" type="text" placeholder="+57 300 000 0000">
    <button class="submit" id="publicar">Publicar reporte</button>
    <p class="hint">Al publicar aceptas que la información sea visible para cualquiera. No incluyas datos que no quieras hacer públicos.</p>
  </div>
</div>
<div class="toast" id="toast"></div>
<script>
(function(){
  const $ = s => document.querySelector(s);
  const $$ = s => Array.from(document.querySelectorAll(s));
  /* --- toast --- */
  let t;
  function toast(msg){
    const el = $('#toast'); el.textContent = msg; el.classList.add('show');
    clearTimeout(t); t = setTimeout(()=>el.classList.remove('show'), 2600);
  }
  /* --- tabs --- */
  function activar(name){
    $$('[data-panel]').forEach(p => p.hidden = (p.id !== 'panel-' + name));
    $$('.tab').forEach(b => b.setAttribute('aria-selected', String(b.dataset.tab === name)));
    $$('.bnav[data-tab]').forEach(b => b.setAttribute('aria-selected', String(b.dataset.tab === name)));
    window.scrollTo({top: Math.min(window.scrollY, $('main').offsetTop - 120), behavior:'smooth'});
  }
  $$('.tab, .bnav[data-tab]').forEach(b => b.addEventListener('click', () => activar(b.dataset.tab)));
  /* --- chips --- */
  $$('.chip').forEach(c => c.addEventListener('click', () => {
    $$('.chip').forEach(o => o.setAttribute('aria-pressed','false'));
    c.setAttribute('aria-pressed','true');
  }));
  /* --- ciudad --- */
  $('#ciudad').addEventListener('change', e => {
    $('#ciudad-eco').textContent = e.target.value + ' · 12 activos';
    toast('Mostrando reportes de ' + e.target.value);
  });
  /* --- modal --- */
  const modal = $('#modal');
  let ultimoFoco = null;
  function abrir(){ ultimoFoco = document.activeElement; modal.hidden = false; document.body.style.overflow='hidden'; $('#nombre').focus(); }
  function cerrar(){ modal.hidden = true; document.body.style.overflow=''; if(ultimoFoco) ultimoFoco.focus(); }
  $$('[data-open-modal]').forEach(b => b.addEventListener('click', abrir));
  $$('[data-close]').forEach(b => b.addEventListener('click', cerrar));
  document.addEventListener('keydown', e => { if(e.key === 'Escape' && !modal.hidden) cerrar(); });
  $('#publicar').addEventListener('click', () => {
    if(!$('#nombre').value.trim()){ toast('Escribe un nombre o título para el reporte'); $('#nombre').focus(); return; }
    cerrar(); toast('Reporte publicado');
  });
  /* --- geolocalización --- */
  $('#geo').addEventListener('click', () => {
    if(!navigator.geolocation){ toast('Tu navegador no comparte la ubicación'); return; }
    navigator.geolocation.getCurrentPosition(
      p => { $('#ubi').value = p.coords.latitude.toFixed(5) + ', ' + p.coords.longitude.toFixed(5); toast('Ubicación agregada'); },
      () => toast('No pudimos leer tu ubicación. Escríbela a mano.')
    );
  });
  /* --- compartir --- */
  $('#share').addEventListener('click', async () => {
    const data = { title:'SismoInfo Colombia', text:'Reportes de personas, mascotas y lugares cerca de ti.', url: location.href };
    if(navigator.share){ try{ await navigator.share(data); }catch(e){} }
    else { try{ await navigator.clipboard.writeText(location.href); toast('Enlace copiado'); }catch(e){ toast('Copia el enlace desde la barra del navegador'); } }
  });
})();
</script>
</body>
</html>
//===== public/iframe_test.html =====
<!doctype html>
<html lang="es">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>SISMOINFO.CO</title>
  <link rel="stylesheet" href="style.css" />
</head>
<body>
  <iframe href="http://colombiatebusca.com/" />
</body>
</html>
//===== public/run.sh =====
npx browser-sync start --server --files "index.html, index.js, map.js, modal.js, virtue.js, style.css, modal.css"
 # npx browser-sync start --server public --files "public/**/*"
//===== public/modal.css =====
/* ===== MODAL (shell + content) — extracted from style.css ===== */
.modal {
  position: fixed;
  min-height: 100vh;
  min-height: 100dvh;
  max-height: 100vh;
  max-height: 100dvh;
  min-width: 100vw;
  max-width: 100vw;
  inset: 0;
  background: rgba(0, 0, 0, 0.75);
  backdrop-filter: blur(10px);
  display: none;
  align-items: center;
  justify-content: center;
  z-index: 10000;
  padding: 5px;
  animation: modalFade 0.2s ease-out;
}
.modal-header {
  position: sticky;
  top: 0px;
  display: flex;
  gap: 5px;
  background: rgba(var(--surface), 0.8);
  align-items: center;
  padding: 4px 6px;
  backdrop-filter: blur(10px);
  border-bottom: 2px solid rgba(var(--surface), 0.82);
  border-radius: 5px 5px 0px 0px;
  z-index: 999999;
}
.modal.open {
  display: flex;
}
@keyframes modalFade {
  from {
    opacity: 0;
    transform: scale(0.96);
  }
  to {
    opacity: 1;
    transform: scale(1);
  }
}
#mapPickerContainer {
  height: min(100%, 600px);
}
.modal-content {
  display: flex;
  flex-direction: column;
  background: #1a1d24;
  border-radius: 10px;
  max-width: min(1200px, 100%);
  width: 100%;
  height: 100%;
  max-height: 100%;
  overflow-y: auto;
  position: relative;
  border: 1px solid #2a2e36;
  box-shadow: 0 20px 60px rgba(0, 0, 0, 0.6);
}
.modal-content::-webkit-scrollbar {
  width: 4px;
}
.modal-content::-webkit-scrollbar-track {
  background: transparent;
}
.modal-content::-webkit-scrollbar-thumb {
  background: #444;
  border-radius: 4px;
}
.modal-close {
  /* position: absolute; */
  /* top: 12px; */
  /* right: 12px; */
  line-height: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  transition: background 0.15s;
}
.modal-random {
  /* position: absolute; */
  /* top: 12px; */
  /* left: 12px; */
  line-height: 1;
  /* display: flex; */
  /* display: none; */
  align-items: center;
  justify-content: center;
  transition: background 0.15s;
  cursor: pointer;
}
.modal-random:hover {
  background: #3a3e46;
  color: #eee;
}
.modal-close:hover {
  background: #3a3e46;
  color: #eee;
}
.modal-content h2 {
  margin: 0 0 6px 0;
  /* font-size: 1.4rem; */
  padding-right: 30px;
  word-break: break-word;
}
.modal-status {
  display: inline-block;
  /* margin-bottom: 12px; */
}
.modal-meta {
  /* font-size: 0.9rem; */
  color: #aaa;
  padding: 5px;
  margin-bottom: 4px;
  line-height: 1.6;
}
.modal-meta span {
  display: inline-block;
  margin-right: 12px;
}
.modal-divider {
  height: 1px;
  background: #2a2e36;
  margin: 16px 0;
}
.modal-photo {
  max-height: 90vh;
  width: 100%;
  /* max-height: 280px; */
  object-fit: contain;
  border-radius: 10px;
  margin-bottom: 12px;
}
/* ===== Galería de imágenes de anuncios ===== */
.anuncio-gallery {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(90px, 1fr));
  gap: 8px;
  margin-top: 12px;
}
.anuncio-gallery-item {
  position: relative;
  border-radius: 8px;
  overflow: hidden;
  border: 2px solid transparent;
  background: #1a1d24;
}
.anuncio-gallery-item.is-main {
  border-color: #fcd116;
}
.anuncio-gallery-item img {
  width: 100%;
  height: 90px;
  object-fit: cover;
  display: block;
}
.anuncio-gallery-actions {
  display: flex;
  flex-direction: column;
  gap: 4px;
  padding: 4px;
}
.anuncio-main-badge {
  font-size: 0.7rem;
  color: #fcd116;
  text-align: center;
  padding: 2px 4px;
}
.modal-actions {
  display: flex;
  gap: 5px;
  flex-wrap: wrap;
  position: sticky;
  bottom: 0px;
  margin-top: 16px;
  background: rgba(var(--surface), 0.8);
  align-items: center;
  padding: 3px;
  backdrop-filter: blur(10px);
  border-top: 2px solid rgba(var(--surface), 0.82);
  border-radius: 5px 5px 0px 0px;
  z-index: 999999;
}
#commentsList {
  flex: 1;
  margin-bottom: 12px;
}
//===== public/index.html =====
<!doctype html>
<html lang="es">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>SISMOINFO.CO</title>
  <link rel="stylesheet" href="style.css" />
  <link rel="stylesheet" href="modal.css" />
  <!-- Leaflet CSS y JS para el mapa -->
  <link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css" />
  <script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"></script>
  <!-- Leaflet.markercluster plugin -->
  <link rel="stylesheet" href="https://unpkg.com/leaflet.markercluster@1.5.3/dist/MarkerCluster.css" />
  <link rel="stylesheet" href="https://unpkg.com/leaflet.markercluster@1.5.3/dist/MarkerCluster.Default.css" />
  <script src="https://unpkg.com/leaflet.markercluster@1.5.3/dist/leaflet.markercluster.js"></script>
  <!-- Leaflet.heat plugin -->
  <script src="https://unpkg.com/leaflet.heat@0.2.0/dist/leaflet-heat.js"></script>
  <!-- ECharts (CDN) -->
  <script src="https://cdn.jsdelivr.net/npm/echarts@5.5.0/dist/echarts.min.js"></script>
  <!-- Olm (motor criptográfico E2E, WASM inline) — debe cargar antes que matrix-js-sdk -->
  <script src="https://cdn.jsdelivr.net/npm/@matrix-org/olm@3.2.15/olm.js"></script>
  <!-- Matrix JS SDK (chat E2E) — v28.2.0 es la última con bundle UMD -->
  <script src="https://cdn.jsdelivr.net/npm/matrix-js-sdk@28.2.0/dist/browser-matrix.js"></script>
</head>
<body>
  <!-- ===== HEADER ===== -->
  <div class="top-bar" style="display: flex; align-items: center">
    <img src="logo.png" style="
          max-height: 30px;
          margin-right: 13px;
          transform: scale(162%) translate(-5px, 3px);
          z-index: 999;
        " />
    <b style="display:flex;flex: 1; font-size: 110%">
      <div style="color: #ffff46">SISMO</div>
      <div style="color: #7a9df5">INFO</div>
      <div style="color: #fe5166">.CO</div>
    </b>
    <button id="copyLinkBtn">🔗 <span class="tab-label">LINK</span></button>
    <button id="shareAppBtn">📤 <span class="tab-label">Compartir</span></button>
    <!-- <div id="apiTargetToggle" style="display:none;margin-left:auto;"> -->
    <!-- <button type="button" id="apiTargetRemote" class="btn-small" style="border-radius: 6px 0 0 6px;">🌏 </button> -->
    <!-- <button type="button" id="apiTargetLocal" class="btn-small" style="border-radius: 0 6px 6px 0;">🖥️ </button> -->
    <!-- </div> -->
  </div>
  <div style="display: none; padding: 5px">
    <div>+57 3153410282 @neokpm</div>
    <a style="
          margin-left: auto;
          border-radius: 5px;
          padding: 3px;
          background: rgb(200, 200, 200);
        " href="https://colombiatebusca.com/">
      https://colombiatebusca.com/</a>
  </div>
  <!-- ===== DRIVER (TABS + SEARCH) ===== -->
  <!-- ===== PANEL PERSONAS ===== -->
  <div id="tabPersonas" class="tab-panel active">
    <div class="tool-bar">
      <div class="filter-row" data-tab="person">
        <button id="crear-top" class="crear-btn" onclick="openCrearModal('person')">
          🫂 NUEVO
        </button>
        <div class="filter-pills">
          <button class="filter-pill active" data-status="">Todo</button>
          <button class="filter-pill" data-status="desaparecido">🔍 <span class="pill-label">Desap.</span></button>
          <button class="filter-pill" data-status="encontrado">✅ <span class="pill-label">Encont.</span></button>
          <button class="filter-pill" data-status="angel">👼</button>
        </div>
        <select class="filter-select" data-filter="city">
          <option value="">📍</option>
        </select>
      </div>
    </div>
    <div id="list" class="list"></div>
  </div>
  <!-- ===== PANEL MASCOTAS ===== -->
  <div id="tabPets" class="tab-panel">
    <div class="tool-bar">
      <div class="filter-row" data-tab="pet">
        <button id="crear-top" class="crear-btn" onclick="openCrearModal('pet')">
          🐾 NUEVO
        </button>
        <div class="filter-pills">
          <button class="filter-pill active" data-status="">Todos</button>
          <button class="filter-pill" data-status="desaparecido">🔍 <span
              class="pill-label">Desaparecido</span></button>
          <button class="filter-pill" data-status="encontrado">✅ <span class="pill-label">Encontrado</span></button>
        </div>
        <select class="filter-select" data-filter="city">
          <option value="">📍 TODAS</opton>
        </select>
      </div>
    </div>
    <div id="listP" class="list"></div>
  </div>
  <!-- ===== PANEL EDIFICIOS ===== -->
  <div id="tabEdificios" class="tab-panel">
    <div class="tool-bar">
      <div class="filter-row" data-tab="building">
        <button id="crear-top" class="crear-btn" onclick="openCrearModal('building')">
          📍 CREAR UBICACIÓN
        </button>
        <div class="filter-pills">
          <button class="filter-pill active" data-status="">Todos</button>
          <button class="filter-pill" data-status="seguro">🫶 <span class="pill-label">Seguro</span></button>
          <button class="filter-pill" data-status="danado">⚠️ <span class="pill-label">Dañado</span></button>
          <button class="filter-pill" data-status="colapsado">💥 <span class="pill-label">Colapsado</span></button>
          <button class="filter-pill" data-status="acopio">📦 <span class="pill-label">Acopio</span></button>
        </div>
        <select class="filter-select" data-filter="city">
          <option value="">📍 TODAS</option>
        </select>
      </div>
    </div>
    <div id="listB" class="list"></div>
  </div>
  <!-- ===== PANEL ANUNCIOS ===== -->
  <div id="tabAnuncios" class="tab-panel">
    <div class="tool-bar">
      <!-- Anuncios are free-text only; no filters beyond search. -->
      <button id="crear-top" class="crear-btn" onclick="openCrearModal('anuncio')">
        📣 CREAR ANUNCIO
      </button>
    </div>
    <div id="listA" class="list"></div>
  </div>
  <!-- ===== PANEL COLABORADORES (100% privado: requiere login) ===== -->
  <div id="tabColab" class="tab-panel">
    <!-- Login (sin sesión) -->
    <div id="chatAuth">
      <div id="loginForm"
        style="
          margin:auto;margin-top:20px;max-width:600px;display:flex;flex-direction:column;gap:10px;padding:20px;background:rgba(var(--surface),0.38);border-radius:10px;">
        <b style="color:#ccc;">💬 Chat de colaboradores</b>
        <input type="text" id="loginIdentifier" placeholder="Email o usuario" />
        <input type="password" id="loginPassword" placeholder="Contraseña" />
        <button id="loginBtn">Entrar</button>
        <button id="privacyInfoBtn"
          style="background:none;margin-top:20px;border:none;cursor:pointer;font-size:0.75rem;padding:10px;">
          <div style="display:flex;align-items:center;gap:8px;margin-top:4px;">
            <small style="color:#999;font-size:100%;">
              🔒 Cifrado E2E con Matrix · Sin federación · Las conversaciones son encriptadas, no se guardan en base de
              datos, solo son legibles desde el dispositivo de quien haga parte de la conversación. Tu contraseña está
              encriptada con "scrypt".
            </small>
          </div>
          <div style="color:#3a5dc5;text-decoration:underline;">
            Click para más
            información
          </div>
        </button>
        <div id="loginError" style="color:#f66;font-size:0.85rem;"></div>
      </div>
    </div>
    <!-- Contenido privado (con sesión) -->
    <div id="colabPrivateWrapper" style="display:none;flex-direction:column;flex:1;min-height:0;">
      <div id="connContainer"></div>
      <!-- Sub-tabs -->
      <div class="sub-tabs"
        style="position:sticky;top:calc(var(--top-bar-height) + 5px);z-index:9999;backdrop-filter:blur(10px);">
        <button class="sub-tab-btn active" data-subtab="chat" style="margin-right:auto;">💬<span
            class="tab-label">Chat</span></button>
        <button class="crear-btn sub-action" data-action-for="donaciones" onclick="openCrearModal('donacion')">🎁
          DONAR</button>
        <button class="crear-btn sub-action" data-action-for="necesidades" onclick="openCrearModal('necesidad')">🆘
          REPORTAR</button>
        <button class="crear-btn sub-action" data-action-for="logistica" onclick="openCrearModal('logistica')">🚚
          CREAR</button>
        <button class="crear-btn sub-action" data-action-for="voluntarios" onclick="openCrearModal('colaborador')">🤝
          COLABORAR</button>
        <button class="sub-tab-btn" data-subtab="donaciones">🎁<span class="tab-label">Donaciones</span></button>
        <button class="sub-tab-btn" data-subtab="necesidades">🆘<span class="tab-label">Necesidades</span></button>
        <button class="sub-tab-btn" data-subtab="logistica">🚚<span class="tab-label">Logística</span></button>
        <button class="sub-tab-btn" data-subtab="voluntarios">🤝<span class="tab-label">Voluntarios</span></button>
      </div>
      <!-- Panel Chat: lista de conversaciones -->
      <div class="sub-tab-panel active" data-subtab-panel="chat">
        <div id="chatListContainer"
          style="flex:1;overflow-y:auto;padding:0px;display:flex;flex-direction:column;gap:8px;"></div>
      </div>
      <!-- Panel Donaciones -->
      <div class="sub-tab-panel" data-subtab-panel="donaciones">
        <div id="listDonaciones" class="list"></div>
      </div>
      <!-- Panel Necesidades -->
      <div class="sub-tab-panel" data-subtab-panel="necesidades">
        <div id="listNecesidades" class="list"></div>
      </div>
      <!-- Panel Logística -->
      <div class="sub-tab-panel" data-subtab-panel="logistica">
        <div id="listLogistica" class="list"></div>
      </div>
      <!-- Panel Voluntarios (lista de colaboradores) -->
      <div class="sub-tab-panel" data-subtab-panel="voluntarios">
        <div id="colabListWrapper">
          <div id="listColab" class="list"></div>
        </div>
      </div>
    </div>
    <div id="colabLoginPrompt" style="padding:16px;text-align:center;color:#999;font-size:0.9rem;">
      🔒 Inicia sesión para ver la zona de colaboradores
    </div>
  </div>
  <!-- ===== PANEL MAPA ===== -->
  <div id="tabMapPanel" class="tab-panel">
    <div id="map">
      <!-- <label for="citySelectorMap" style="color:#ccc; margin-right:8px;">Ir a ciudad:</label> -->
      <div id="mapTypeFilter" style="
              display: flex;
              /* flex-direction: column; */
              position:absolute;
              /* top:calc(var(--top-bar-height) + 3px); */
              top:3px;
              right:3px;
              justify-content:right;
              gap: 4px;
              background: rgba(10, 10, 10, 0.38);
              border-radius: 10px 10px 10px 10px ;
              padding:3px;
              border: 1px solid rgba(var(--surface), 0.6);
              z-index:99999999999999999999999999;
            ">
        <button class="filter-pill active" data-map-type="person">🫂</button>
        <button class="filter-pill active" data-map-type="pet">🐕</button>
        <button class="filter-pill active" data-map-type="building">🏢</button>
        <button class="filter-pill active" data-map-type="sismo">🌋</button>
        <button class="filter-pill active" id="heatmapToggle" title="Mapa de calor">🔥</button>
        <select id="citySelectorMap" style="
              padding: 3px 5px;
              border-radius: 8px;
              border: 1px solid #333;
              background: rgba(10, 10, 10, 0.8);
              color: #eaeaea;
              min-width: 150px;
            ">
          <option value="">-- 📍 IR A --</option>
        </select>
      </div>
      <div id="mapSidebar" class="map-sidebar">
        <div class="map-sidebar-title">🫂 Personas · 🐕 Mascotas</div>
        <div id="mapSidebarList" class="map-sidebar-list"></div>
      </div>
      <div id="offscreenArrows"></div>
    </div>
  </div>
  <!-- ===== PANEL SISMOS ===== -->
  <div id="tabSismos" class="tab-panel">
    <!-- ===== ÚLTIMO SISMO (card, consistente con el resto) ===== -->
    <div class="sismo-head">
      <div id="sismoLive" class="sismo-live" style="margin-top:10px;">
        <div class="sismo-live-head">
          <span class="sismo-live-dot" aria-hidden="true"></span>
          <span class="sismo-live-title">En vivo · últimos 72 h</span>
          <span id="sismoLiveUpdated" class="sismo-live-updated">—</span>
        </div>
        <div id="sismoLiveList" class="sismo-live-list"></div>
      </div>
      <div id="sismoAlert" class="alerta">
        <div class="alerta-head">
          <span class="alerta-title">Último sismo</span>
          <span id="ultimoRel" class="alerta-rel">—</span>
        </div>
        <div class="alerta-body">
          <span id="ultimoMag" class="alerta-mag">--</span>
          <span class="alerta-info">
            <span id="ultimoLugar" class="alerta-lugar"></span>
            <span id="ultimoHora" class="alerta-hora"></span>
          </span>
        </div>
      </div>
      <!-- ===== FILTROS (misma familia visual: pills + selects) ===== -->
      <div class="tool-bar"
        style="background:rgba(var(--surface),0.38);position:static;top:auto;backdrop-filter: blur(10px);border-radius:10px;padding:5px 10px;">
        <!-- ===== HISTORIAL ===== -->
        <div style="font-weight:600;color:#ccc;font-size=60%;">📜</div>
        <div class="filter-row" data-tab="sismos">
          <select class="filter-select" id="sismoMagFilter">
            <option value="0">Magn.: todas</option>
            <option value="3">Magn.: M3+</option>
            <option value="4">Magn.: M4+</option>
            <option value="5">Magn.: M5+</option>
            <option value="6">Magn.: M6+</option>
          </select>
          <select class="filter-select" id="sismoDepthFilter">
            <option value="">Prof.: todas</option>
            <option value="shallow">Superf. (&lt;30 km)</option>
            <option value="mid">Media (30–70 km)</option>
            <option value="deep">Alta (70–150 km)</option>
            <option value="vdeep">Muy (&gt;150 km)</option>
          </select>
          <select class="filter-select" id="sismoDateFilter">
            <option value="0">Hace:--</option>
            <option value="1">24 h</option>
            <option value="7">7 días</option>
            <option value="30">30 días</option>
            <option value="365">Un año</option>
          </select>
        </div>
      </div>
    </div>
    <!-- ===== FEED EN VIVO (ventana corta, poll 60s) ===== -->
    <div id="sismoChart"
      style="display:none;height:400px;width:100%;background:#222;border-radius:0px 0px 10px 10px;border-bottom:1px solid #e9ecef;">
    </div>
    <div id="sismosList" class="sismos-list" style="margin-top:10px;"></div>
    <div style="display:flex;justify-content:space-between;margin-top:10px;font-size:14px;color:#999;">
      <span>Fuente: USGS | En vivo cada 60s</span>
      <span id="totalSismos">0 sismos</span>
    </div>
  </div>
  <!-- ===== PANEL WIKI ===== -->
  <div id="tabWiki" class="tab-panel">
    <div class="wiki">
      <details open>
        <summary>📞 Líneas de emergencia</summary>
        <div class="wiki-body">
          <p><b>123</b> — Emergencias / Policía</p>
          <p><b>119</b> — Cruz Roja / Ambulancia</p>
          <p><b>132</b> — Bomberos</p>
          <p><b>165</b> — Línea de atención sismos (Colombia)</p>
          <p><b>141</b> — Fijación telefónica de personas desaparecidas</p>
        </div>
      </details>
      <details>
        <summary>🫂 Qué hacer si una persona desaparece</summary>
        <div class="wiki-body">
          <ol>
            <li>
              Reporta inmediatamente en la pestaña <b>Personas</b> de esta
              app.
            </li>
            <li>
              Llama al <b>123</b> y a la línea <b>141</b> para fijar el
              reporte.
            </li>
            <li>
              Reúne una foto reciente y datos: nombre, edad, vestimenta,
              última ubicación.
            </li>
            <li>Comparte el enlace del reporte por WhatsApp y redes.</li>
            <li>
              Acude a la fiscalía o comisaría más cercana con el reporte.
            </li>
          </ol>
        </div>
      </details>
      <details>
        <summary>🐕 Qué hacer si pierdes tu mascota</summary>
        <div class="wiki-body">
          <ol>
            <li>
              Reporta en la pestaña <b>Mascotas</b> con foto y última
              ubicación.
            </li>
            <li>
              Busca en un radio de 1 km; deja prendas con tu olor en el lugar.
            </li>
            <li>
              Pega carteles en veterinarias, tiendas y parques cercanos.
            </li>
            <li>Publica en grupos locales de Facebook y WhatsApp.</li>
          </ol>
        </div>
      </details>
      <details>
        <summary>🏢 Cómo reportar el estado de un edificio</summary>
        <div class="wiki-body">
          <p>Usa la pestaña <b>Lugar</b> y marca el estado:</p>
          <ul>
            <li><b>Seguro</b> — sin daños visibles, habitable.</li>
            <li>
              <b>Dañado</b> — grietas o daños parciales, requiere revisión.
            </li>
            <li><b>Colapso</b> — estructura comprometida, NO ingresar.</li>
          </ul>
        </div>
      </details>
      <details>
        <summary>🛡️ Durante un sismo</summary>
        <div class="wiki-body">
          <ul>
            <li>
              <b>Agáchate, cubrete, agárrate.</b> Mantente bajo una mesa
              resistente.
            </li>
            <li>Aleja de ventanas, espejos y objetos que puedan caer.</li>
            <li>
              Si estás en la calle, busca un espacio abierto lejos de
              edificios.
            </li>
            <li>
              No uses ascensores. Si estás en uno, sal en el primer piso
              seguro.
            </li>
          </ul>
        </div>
      </details>
      <details>
        <summary>📦 Kit de emergencia recomendado</summary>
        <div class="wiki-body">
          <ul>
            <li>
              Agua (4 L por persona por día) y alimentos no perecederos.
            </li>
            <li>Botiquín de primeros auxilios y medicamentos básicos.</li>
            <li>Linterna, pilas, radio a batería.</li>
            <li>Copias de documentos en bolsa impermeable.</li>
            <li>Cargador portátil para el celular.</li>
          </ul>
        </div>
      </details>
      <details>
        <summary>🤝 Cómo colaborar</summary>
        <div class="wiki-body">
          <p>
            Únete en la pestaña <b>Voluntad</b> indicando cómo puedes ayudar:
            transporte, primeros auxilios, albergue, búsqueda, etc.
          </p>
        </div>
      </details>
    </div>
  </div>
  <!-- ===== CREAR MODAL (all creation forms) ===== -->
  <div id="crearModal" class="modal" role="dialog" aria-modal="true" aria-labelledby="crearModalTitle">
    <div class="modal-content" style="max-width: 560px; max-height: 90vh">
      <div class="modal-header" data-modal-header>
        <h2 id="crearModalTitle" style="flex:1;">Crear</h2>
        <button class="modal-close" id="crearModalClose" data-modal-close aria-label="Cerrar">
          ✕
        </button>
      </div>
      <div data-modal-body style="padding:5px;flex:1;display:flex;flex-direction:column;width:100%;">
        <!-- PERSONA -->
        <div class="crear-form-wrap" data-type="person">
          <form id="addForm">
            <input type="text" id="nameInput" placeholder="Nombre de la persona" required />
            <input type="text" id="locationInput" placeholder="Ubicación (opcional)" />
            <select id="cityInput">
              <option value="">Ciudad</option>
            </select>
            <button type="button" id="gpsBtn" class="gps-btn" title="Usar mi ubicación">
              📍
            </button>
            <span id="locationDisplay" style="font-size: 62%; color: #666; min-width: 120px"></span>
            <div style="min-width: 100%">
              🖼️ Dos opciones (URL o Subir imagen)
              <br />
              <input type="text" id="imageInput" placeholder="URL de imagen (opcional)" />
              <img id="urlPreview" class="photo-preview" style="display: none" alt="Vista previa URL" />
              <label class="custom-file-upload">
                📷 Subir foto
                <input type="file" id="photoInput" accept="image/*" />
              </label>
              <span class="file-name" id="fileLabel">Sube una foto</span>
              <img id="photoPreview" class="photo-preview" style="display: none" alt="Vista previa" />
            </div>
            <button type="submit">Reportar</button>
          </form>
        </div>
        <!-- MASCOTA -->
        <div class="crear-form-wrap" data-type="pet">
          <form id="addFormP">
            <input type="text" id="nameInputP" placeholder="Nombre de la mascota" required />
            <div style="display: flex; gap: 5px; width: 100%">
              <button type="button" id="gpsBtnP" class="gps-btn" title="Usar mi ubicación">
                📍
              </button>
              <input type="text" id="locationInputP" placeholder="Ubicación (opcional)" style="flex: 1" />
              <select id="cityInputP">
                <option value="">Ciudad</option>
              </select>
            </div>
            <div id="locationDisplayP" style="font-size: 0.8rem; color: #666; min-width: 120px"></div>
            <div style="min-width: 100%">
              🖼️ Dos opciones (URL o Subir imagen)
              <br />
              <input type="text" id="imageInputP" placeholder="URL de imagen (opcional)" />
              <img id="urlPreviewP" class="photo-preview" style="display: none" alt="Vista previa URL" />
              <label class="custom-file-upload">
                📷 Subir foto
                <input type="file" id="photoInputP" accept="image/*" />
              </label>
              <span class="file-name" id="fileLabelP">Sube una foto</span>
              <img id="photoPreviewP" class="photo-preview" style="display: none" alt="Vista previa" />
            </div>
            <button type="submit">Reportar</button>
          </form>
        </div>
        <!-- UBICACIÓN / EDIFICIO -->
        <div class="crear-form-wrap" data-type="building">
          <form id="addFormB">
            <input type="text" id="nameInputB" placeholder="Nombre o dirección del edificio" required />
            <input type="text" id="locationInputB" placeholder="Ubicación (opcional)" />
            <select id="cityInputB">
              <option value="">Ciudad</option>
            </select>
            <button type="button" id="gpsBtnB" class="gps-btn" title="Usar mi ubicación">
              📍
            </button>
            <span id="locationDisplayB" style="font-size: 62%; color: #666; min-width: 120px"></span>
            <label style="display:flex; align-items:center; gap:8px; margin-top:8px; font-size:0.85rem; color:#ccc;">
              <input type="checkbox" id="privateInputB" />
              🔒 Privado (solo colaboradores registrados)
            </label>
            <div style="min-width: 100%">
              🖼️ Dos opciones (URL o Subir imagen)
              <br />
              <input type="text" id="imageInputB" placeholder="URL de imagen (opcional)" />
              <img id="urlPreviewB" class="photo-preview" style="display: none" alt="Vista previa URL" />
              <label class="custom-file-upload">
                📷 Subir foto
                <input type="file" id="photoInputB" accept="image/*" />
              </label>
              <span class="file-name" id="fileLabelB">Sube una foto</span>
              <img id="photoPreviewB" class="photo-preview" style="display: none" alt="Vista previa" />
            </div>
            <button type="submit">Reportar</button>
          </form>
        </div>
        <!-- ANUNCIO -->
        <div class="crear-form-wrap" data-type="anuncio">
          <form id="addFormA" style="flex-direction: column; align-items: stretch">
            <input type="text" id="anuncioTitleInput" placeholder="Título del anuncio" style="
                padding: 12px;
                border-radius: 8px;
                border: 1px solid #333;
                background: #1a1d24;
                color: #eaeaea;
                font-family: inherit;
                width: 100%;
                margin-bottom: 8px;
              " />
            <textarea id="anuncioInput" rows="3" placeholder="Escribe un anuncio..." required style="
                padding: 12px;
                border-radius: 8px;
                border: 1px solid #333;
                background: #1a1d24;
                color: #eaeaea;
                resize: vertical;
                min-height: 80px;
                font-family: inherit;
                width: 100%;
                margin-bottom: 8px;
              "></textarea>
            <div style="min-width: 100%">
              🖼️ Dos opciones (URL o Subir imagen)
              <br />
              <input type="text" id="imageInputA" placeholder="URL de imagen (opcional)" />
              <img id="urlPreviewA" class="photo-preview" style="display: none" alt="Vista previa URL" />
              <label class="custom-file-upload">
                📷 Subir fotos
                <input type="file" id="photoInputA" accept="image/*" multiple />
              </label>
              <span class="file-name" id="fileLabelA">Sube una o más fotos</span>
              <img id="photoPreviewA" class="photo-preview" style="display: none" alt="Vista previa" />
            </div>
            <button type="submit" style="width: 100%">Publicar</button>
          </form>
        </div>
        <!-- COLABORADOR -->
        <div class="crear-form-wrap" data-type="colaborador">
          <form id="addFormColab">
            <input type="text" id="nameInputColab" placeholder="Nombre" required />
            <input type="text" id="skillInputColab"
              placeholder="¿Cómo puedes ayudar? (ej: transporte, primeros auxilios)" />
            <input type="text" id="contactInputColab" placeholder="Contacto (teléfono/WhatsApp, opcional)" />
            <select id="cityInputColab">
              <option value="">Ciudad</option>
            </select>
            <hr style="width:100%;border-color:rgba(60,60,60,0.4);margin:8px 0;" />
            <small style="color:#999;width:100%;">Opcional: crea una cuenta para acceder al chat de
              colaboradores</small>
            <input type="email" id="emailInputColab" placeholder="Email (opcional)" />
            <input type="password" id="passwordInputColab" placeholder="Contraseña (opcional, mín 6 chars)" />
            <button type="submit">Unirme</button>
          </form>
        </div>
        <!-- DONACIÓN -->
        <div class="crear-form-wrap" data-type="donacion">
          <form id="addFormDonacion">
            <select id="donacionTypeInput" required>
              <option value="">Tipo de donación</option>
              <option value="comida">🍽️ Comida</option>
              <option value="ropa">👕 Ropa</option>
              <option value="insumos">🧰 Insumos</option>
              <option value="medicinas">💊 Medicinas</option>
              <option value="transporte">🚗 Transporte</option>
              <option value="otro">📦 Otro</option>
            </select>
            <input type="text" id="donacionQtyInput" placeholder="Cantidad (ej: 200 almuerzos)" />
            <textarea id="donacionDescInput" placeholder="Descripción" required></textarea>
            <input type="text" id="donacionLocInput" placeholder="Punto de rescate / dirección" />
            <input type="text" id="donacionContactInput" placeholder="Tu contacto (opcional)" />
            <button type="submit">🎁 Ofrecer</button>
          </form>
        </div>
        <!-- NECESIDAD -->
        <div class="crear-form-wrap" data-type="necesidad">
          <form id="addFormNecesidad">
            <select id="necesidadTypeInput" required>
              <option value="">Tipo de necesidad</option>
              <option value="comida">🍽️ Comida</option>
              <option value="ropa">👕 Ropa</option>
              <option value="carpas">⛺ Carpas</option>
              <option value="medicinas">💊 Medicinas</option>
              <option value="aseo">🧼 Aseo</option>
              <option value="otro">📦 Otro</option>
            </select>
            <input type="text" id="necesidadQtyInput" placeholder="Cantidad" />
            <textarea id="necesidadDescInput" placeholder="¿Qué se necesita?" required></textarea>
            <select id="necesidadUrgencyInput">
              <option value="media">Urgencia media</option>
              <option value="alta">⚠️ URGENTE</option>
              <option value="baja">Baja</option>
            </select>
            <input type="text" id="necesidadPointInput" placeholder="Punto de rescate (ej: Cantabria)" />
            <input type="text" id="necesidadContactInput" placeholder="Contacto en el punto (opcional)" />
            <button type="submit">🆘 Reportar</button>
          </form>
        </div>
        <!-- LOGÍSTICA -->
        <div class="crear-form-wrap" data-type="logistica">
          <form id="addFormLogistica">
            <select id="logisticaTypeInput" required>
              <option value="">Tipo de tarea</option>
              <option value="entrega">📦 Entrega</option>
              <option value="recogida">📥 Recogida</option>
              <option value="transporte">🚚 Transporte</option>
              <option value="apoyo">🙋 Apoyo en punto</option>
            </select>
            <textarea id="logisticaDescInput" placeholder="¿Qué hay que hacer?" required></textarea>
            <input type="text" id="logisticaOriginInput" placeholder="Origen" />
            <input type="text" id="logisticaDestInput" placeholder="Destino" />
            <input type="text" id="logisticaContactInput" placeholder="Contacto (opcional)" />
            <button type="submit">🚚 Crear tarea</button>
          </form>
        </div>
      </div>
      <div class="modal-actions" data-modal-footer>
        <button type="button" class="btn-small" onclick="closeCrearModal()">Cancelar</button>
      </div>
    </div>
  </div>
  <!-- ===== MODAL (detail) ===== -->
  <div id="modal" class="modal" role="dialog" aria-modal="true" aria-labelledby="modalTitle">
    <div class="modal-content">
      <div class="modal-header" data-modal-header>
        <button class="modal-random" id="modalRandom" type="button" aria-label="Ver otro al azar"
          title="Ver otro al azar">🎲</button>
        <div id="modalTitle" style="flex:1;text-transform:uppercase;"></div>
        <button class="modal-close" id="modalClose" data-modal-close aria-label="Cerrar">
          ✕
        </button>
      </div>
      <div data-modal-body style="padding:5px;flex:1;display:flex;flex-direction:column;width:100%;">
        <div class="modal-status" id="modalStatus" style="margin-left:auto;">
        </div>
        <div id="modalMeta" class="modal-meta">
        </div>
        <div id="modalBody"></div>
        <div id="commentsContainer">
          <div id="commentsList"></div>
          <form id="commentForm" style="display:flex;flex-direction:column;">
            <textarea id="commentInput" rows="2" placeholder="Escribe un comentario..." required></textarea>
            <button type="submit">Comentar</button>
          </form>
        </div>
      </div>
      <div id="modalActions" class="modal-actions" data-modal-footer></div>
    </div>
  </div>
  <div id="mapModal" class="modal" role="dialog" aria-modal="true" aria-labelledby="mapModalTitle">
    <div class="modal-content" data-modal-body>
      <div class="modal-header" data-modal-header>
        <h2 id="mapModalTitle" style="flex:1;">Marca la ubicación</h2>
        <button class="modal-close" id="mapModalClose" data-modal-close aria-label="Cerrar">
          ✕
        </button>
      </div>
      <div style="padding:5px;">
        <div id="mapPickerContext" style="display: none"></div>
        <div style="position: relative; margin-bottom: 10px">
          <input type="text" id="mapSearchInput" placeholder="🔍 Buscar dirección o lugar..." style="
              width: 100%;
              padding: 10px;
              border-radius: 8px;
              border: 1px solid #333;
              background: #1a1d24;
              color: #eaeaea;
            " />
          <div id="mapSearchResults" style="
              display: none;
              position: absolute;
              top: 100%;
              left: 0;
              right: 0;
              background: #1a1d24;
              border: 1px solid #333;
              border-radius: 8px;
              margin-top: 4px;
              z-index: 2000;
              max-height: 220px;
              overflow-y: auto;
            "></div>
        </div>
        <div style="display: flex; gap: 8px; margin-bottom: 10px;">
          <input type="number" id="mapPickerLatInput" step="any" placeholder="Latitud" style="
              flex: 1 1 120px;
              padding: 10px;
              border-radius: 8px;
              border: 1px solid #333;
              background: #1a1d24;
              color: #eaeaea;
            " />
          <input type="number" id="mapPickerLngInput" step="any" placeholder="Longitud" style="
              flex: 1 1 120px;
              padding: 10px;
              border-radius: 8px;
              border: 1px solid #333;
              background: #1a1d24;
              color: #eaeaea;
            " />
          <button type="button" id="mapPickerGoBtn">
            Ir
          </button>
        </div>
      </div>
      <div id="mapPickerContainer"></div>
      <div class="modal-actions" data-modal-footer>
        <button type="button" id="mapPickerConfirm">
          📍 Usar esta ubicación
        </button>
      </div>
    </div>
  </div>
  <div id="driver" style="">
    <div class="driver-inner">
      <div id="searchBar" class="search-row">
        <input type="search" id="searchInput" placeholder="🔍 Buscar por nombre, ubicación o ciudad..." />
      </div>
      <div class="tabs">
        <button id="crear" style="
            border: 2px solid palegoldenrod;
            background: teal;
            color: palegoldenrod;
          ">
          <b>NUEVO</b>
        </button>
        <button class="tab-btn active" id="tabPersonasBtn">
          🫂 <span class="tab-label">Personas</span><span class="n">0</span>
        </button>
        <button class="tab-btn" id="tabPetsBtn">
          🐕 <span class="tab-label">Mascotas</span><span class="n">0</span>
        </button>
        <button class="tab-btn" id="tabEdificiosBtn">
          🏢 <span class="tab-label">Lugar</span><span class="n">0</span>
        </button>
        <button class="tab-btn" id="tabAnunciosBtn">
          📣 <span class="tab-label">Anuncios</span><span class="n">0</span>
        </button>
        <button class="tab-btn" id="tabColabBtn">
          🤝 <span class="tab-label">Voluntad</span><span class="n">0</span>
        </button>
        <button class="tab-btn" id="tabMapBtn">🗺️ <span class="tab-label">Mapa</span></button>
        <button class="tab-btn" id="tabSismosBtn">📊 <span class="tab-label">Sismos</span></button>
        <button class="tab-btn" id="wikiBtn">📚 <span class="tab-label">WIKI</span></button>
      </div>
    </div>
  </div>
  <!-- ===== MODAL PRIVACIDAD ===== -->
  <div id="privacyModal" class="modal" role="dialog" aria-modal="true" aria-labelledby="privacyModalTitle">
    <div class="modal-content" style="max-width: 520px; max-height: 85vh">
      <div class="modal-header" data-modal-header>
        <h2 id="privacyModalTitle">🔒 Privacidad del chat</h2>
        <button class="modal-close" data-modal-close aria-label="Cerrar">✕</button>
      </div>
      <div data-modal-body style="padding: 16px; display: flex; flex-direction: column; gap: 12px; overflow-y: auto;">
        <p style="margin: 0; color: #ccc; line-height: 1.6;">
          Este chat usa <b>Matrix</b> con cifrado de extremo a extremo (E2E) mediante el protocolo <b>Olm/Megolm</b>.
        </p>
        <ul style="margin: 0; padding-left: 18px; color: #ccc; line-height: 1.8;">
          <li>Las conversaciones son <b>encriptadas</b> en tu dispositivo antes de enviarse.</li>
          <li>No se guardan en texto plano en ninguna base de datos.</li>
          <li>Solo son legibles por los participantes de la conversación, en sus propios dispositivos.</li>
          <li>El servidor <b>Dendrite</b> solo almacena y retransmite mensajes cifrados; no puede leer su contenido.
          </li>
          <li>Sin federación: no se comparte información con otros servidores Matrix.</li>
        </ul>
        <p style="margin: 0; color: #999; font-size: 0.85rem; line-height: 1.5;">
          Si perdés tu dispositivo o tus claves de sesión, no podrás recuperar el historial de conversaciones.
        </p>
      </div>
    </div>
  </div>
  <!-- Modal del chat (a pantalla completa, sin topbar ni driver) -->
  <div id="chatModal" class="modal" role="dialog" aria-modal="true" aria-labelledby="chatModalTitle"
    style="padding:0px;">
    <div class="modal-content" style="max-width: 100%; max-height: 100vh; width: 100%; height: 100%; border-radius: 0;">
      <div class="modal-header" data-modal-header style="flex:0 0 auto;">
        <div id="chatModalTitle" style="flex:1;">💬 Ayuda en Cali 🆘</div>
        <button class="modal-close" data-modal-close aria-label="Cerrar">✕</button>
      </div>
      <div data-modal-body style="padding:0;flex:1;display:flex;flex-direction:column;overflow:hidden;">
        <div id="chatMessages" style="flex:1;overflow-y:auto;padding:10px;display:flex;flex-direction:column;gap:8px;">
        </div>
        <!-- Barra de respuesta (se muestra cuando se responde a un mensaje) -->
        <div id="chatReplyBar" style="display:none;padding:6px 8px;background:rgba(120,120,120,0.15);border-top:1px solid rgba(120,120,120,0.2);flex:0 0 auto;">
          <div style="display:flex;align-items:center;gap:8px;">
            <span style="font-size:0.7rem;color:#888;">Respondiendo a</span>
            <div id="chatReplyPreview" style="flex:1;min-width:0;"></div>
            <button id="chatReplyCancel" type="button" style="flex:0 0 auto;padding:2px 8px;font-size:0.75rem;">✕</button>
          </div>
        </div>
        <div
          style="display:flex;gap:6px;padding:5px;border-top:1px solid rgba(120,120,120,0.3);flex:0 0 auto;align-items:flex-end;">
          <button id="chatAttachBtn" type="button" title="Adjuntar imagen o video" style="flex:0 0 auto;">📎</button>
          <input type="file" id="chatFileInput" accept="image/*,video/*" multiple style="display:none;" />
          <textarea id="chatInput" rows="1" placeholder="Escribe un mensaje..."
            style="flex:1;resize:none;min-height:38px;max-height:120px;font-family:inherit;"></textarea>
          <button id="chatSendBtn" style="flex:0 0 auto;">Enviar</button>
        </div>
      </div>
    </div>
  </div>
  <!-- Modal de invitación al chat (lista de colaboradores con cuenta Matrix) -->
  <div id="inviteModal" class="modal" role="dialog" aria-modal="true" aria-labelledby="inviteModalTitle">
    <div class="modal-content" style="max-width:420px;">
      <div class="modal-header" data-modal-header>
        <div id="inviteModalTitle" style="flex:1;">➕ Invitar colaboradores</div>
        <button class="modal-close" data-modal-close aria-label="Cerrar">✕</button>
      </div>
      <div data-modal-body style="max-height:60vh;overflow-y:auto;">
        <div id="inviteList"></div>
      </div>
    </div>
  </div>
  <!-- Lightbox de medios del chat (visor a pantalla completa, con navegación) -->
  <div id="mediaLightbox" class="modal" role="dialog" aria-modal="true" style="padding:0;">
    <div class="modal-content" style="max-width:100%;max-height:100vh;width:100%;height:100%;border-radius:0;background:#000;">
      <div class="modal-header" data-modal-header style="background:rgba(0,0,0,0.6);">
        <div id="lightboxCounter" style="flex:1;color:#ccc;font-size:0.85rem;"></div>
        <button class="modal-close" data-modal-close aria-label="Cerrar">✕</button>
      </div>
      <div data-modal-body style="flex:1;display:flex;align-items:center;justify-content:center;overflow:hidden;position:relative;">
        <button id="lightboxPrev" class="lightbox-nav lightbox-prev" aria-label="Anterior">‹</button>
        <div id="lightboxStage" style="max-width:100%;max-height:100%;display:flex;align-items:center;justify-content:center;"></div>
        <button id="lightboxNext" class="lightbox-nav lightbox-next" aria-label="Siguiente">›</button>
      </div>
    </div>
  </div>
  <script src="modal.js"></script>
  <script src="map.js"></script>
  <script src="sismos.js"></script>
  <script src="index.js"></script>
</body>
</html>
//===== public/matrix-chat.js =====
// matrix-chat.js — módulo ESM que carga matrix-js-sdk y expone el chat al
// window. Se carga con <script type="module">. El SDK es ESM-only (ya no
// publica bundle UMD), por eso usamos import en vez de <script src>.
import * as matrixcs from "https://cdn.jsdelivr.net/npm/matrix-js-sdk@34.11.1/+esm";
const MATRIX_ROOM_ALIAS = "#sismo-general:matrix.sismoinfo.co";
let matrixClient = null;
let matrixRoom = null;
let matrixStarted = false;
function escapeHtml(str) {
  const div = document.createElement("div");
  div.textContent = str == null ? "" : String(str);
  return div.innerHTML;
}
async function startMatrixChat() {
  if (matrixStarted) return;
  matrixStarted = true;
  try {
    // 1. Obtener credenciales Matrix del backend (provisiona si hace falta).
    const token = localStorage.getItem("sismo_auth_token");
    const res = await fetch(`${window.API_BASE}/auth/matrix`, {
      method: "POST",
      headers: { Authorization: `Bearer ${token}` },
    });
    if (!res.ok) {
      const data = await res.json().catch(() => ({}));
      renderChatError(data.error || "No se pudo conectar al chat.");
      return;
    }
    const { base_url, user_id, password } = await res.json();
    // 2. Login en Matrix (genera claves E2E en el navegador).
    matrixClient = matrixcs.createClient({ baseUrl: base_url });
    await matrixClient.login("m.login.password", {
      identifier: {
        type: "m.id.user",
        user: user_id.split(":")[0].replace("@", ""),
      },
      password,
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
    return matrixClient.getRoom(roomId);
  } catch (e) {
    const { room_id } = await matrixClient.createRoom({
      name: "Sismo General",
      topic: "Canal general de colaboradores de Sismo",
      visibility: "private",
      room_alias_name: "sismo-general",
    });
    return matrixClient.getRoom(room_id);
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
      await matrixClient.sendTextMessage(matrixRoom.roomId, text);
      input.value = "";
    } catch (e) {
      console.error("send error:", e);
    }
  };
  sendBtn.addEventListener("click", send);
  input.addEventListener("keydown", (e) => {
    if (e.key === "Enter") send();
  });
  matrixClient.on("Room.timeline", (event, room) => {
    if (!room || room.roomId !== matrixRoom.roomId) return;
    if (event.getType() !== "m.room.message") return;
    appendMessage(event);
  });
  const timeline = matrixRoom.getLiveTimeline().getEvents();
  timeline.forEach((event) => {
    if (event.getType() === "m.room.message") appendMessage(event);
  });
}
function appendMessage(event) {
  const list = document.getElementById("chatMessages");
  if (!list) return;
  const content = event.getContent();
  const body = content.body || "";
  const sender = event.getSender();
  const isSelf = sender === matrixClient.getUserId();
  const name = isSelf
    ? window.currentUser?.chat_name || "Tú"): sender.split(":")[0].replace("@", "");
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
// Exponer al window para que index.js (script clásico) pueda llamarlo.
window.startMatrixChat = startMatrixChat;
//===== public/cities.json =====
[
  { "name": "Choco", "lat": 5.2528, "lng": -76.8259 },
  { "name": "Bogotá", "lat": 4.7110, "lng": -74.0721 },
  { "name": "Bolivar", "lat": 10.3910, "lng": -75.4794 },
  { "name": "Medellín", "lat": 6.2442, "lng": -75.5812 },
  { "name": "Cali", "lat": 3.4516, "lng": -76.5320 },
  { "name": "Jamundi", "lat": 3.2618, "lng": -76.5400 },
  { "name": "Barranquilla", "lat": 10.9639, "lng": -74.7968 },
  { "name": "Cartagena", "lat": 10.3910, "lng": -75.4794 },
  { "name": "Cúcuta", "lat": 7.8939, "lng": -72.5078 },
  { "name": "Soacha", "lat": 4.5793, "lng": -74.2168 },
  { "name": "Ibagué", "lat": 4.4389, "lng": -75.2322 },
  { "name": "Bucaramanga", "lat": 7.1193, "lng": -73.1227 },
  { "name": "Soledad", "lat": 10.9184, "lng": -74.7646 },
  { "name": "Villavicencio", "lat": 4.1531, "lng": -73.6350 },
  { "name": "Santa Marta", "lat": 11.2408, "lng": -74.1990 },
  { "name": "Valledupar", "lat": 10.4631, "lng": -73.2532 },
  { "name": "Pereira", "lat": 4.8133, "lng": -75.6961 },
  { "name": "Montería", "lat": 8.7499, "lng": -75.8759 },
  { "name": "Manizales", "lat": 5.0673, "lng": -75.5204 },
  { "name": "Pasto", "lat": 1.2136, "lng": -77.2811 },
  { "name": "Neiva", "lat": 2.9274, "lng": -75.2819 },
  { "name": "Palmira", "lat": 3.5394, "lng": -76.3036 },
  { "name": "Armenia", "lat": 4.5343, "lng": -75.6813 },
  { "name": "Popayán", "lat": 2.4448, "lng": -76.6147 },
  { "name": "Buenaventura", "lat": 3.8831, "lng": -77.0197 },
  { "name": "Sincelejo", "lat": 9.3047, "lng": -75.3978 },
  { "name": "Tuluá", "lat": 4.0847, "lng": -76.1954 },
  { "name": "Florencia", "lat": 1.6144, "lng": -75.6062 },
  { "name": "Barrancabermeja", "lat": 7.0653, "lng": -73.8547 },
  { "name": "Girardot", "lat": 4.3008, "lng": -74.8075 },
  { "name": "Tunja", "lat": 5.5353, "lng": -73.3678 },
  { "name": "Riohacha", "lat": 11.5444, "lng": -72.9072 },
  { "name": "Quibdó", "lat": 5.6947, "lng": -76.6610 },
  { "name": "Yopal", "lat": 5.3378, "lng": -72.3959 },
  { "name": "Apartadó", "lat": 7.8830, "lng": -76.6259 },
  { "name": "Ipiales", "lat": 0.8300, "lng": -77.6444 },
  { "name": "Magangué", "lat": 9.2414, "lng": -74.7547 },
  { "name": "Turbo", "lat": 8.0930, "lng": -76.7284 },
  { "name": "Duitama", "lat": 5.8265, "lng": -73.0201 },
  { "name": "Sogamoso", "lat": 5.7143, "lng": -72.9339 },
  { "name": "Facatativá", "lat": 4.8137, "lng": -74.3548 },
  { "name": "Chía", "lat": 4.8616, "lng": -74.0605 },
  { "name": "Zipaquirá", "lat": 5.0247, "lng": -74.0048 },
  { "name": "Rionegro", "lat": 6.1551, "lng": -75.3737 },
  { "name": "Envigado", "lat": 6.1759, "lng": -75.5917 },
  { "name": "Itagüí", "lat": 6.1719, "lng": -75.6114 },
  { "name": "Bello", "lat": 6.3373, "lng": -75.5580 },
  { "name": "Dosquebradas", "lat": 4.8392, "lng": -75.6673 },
  { "name": "Cartago", "lat": 4.7464, "lng": -75.9117 },
  { "name": "San Andrés", "lat": 12.5847, "lng": -81.7006 },
  { "name": "Leticia", "lat": -4.2153, "lng": -69.9406 },
  { "name": "Mocoa", "lat": 1.1520, "lng": -76.6520 },
  { "name": "Arauca", "lat": 7.0847, "lng": -70.7591 },
  { "name": "Puerto Carreño", "lat": 6.1890, "lng": -67.4859 },
  { "name": "Inírida", "lat": 3.8654, "lng": -67.9239 },
  { "name": "Mitú", "lat": 1.1983, "lng": -70.1736 },
  { "name": "Floridablanca", "lat": 7.0622, "lng": -73.0864 },
  { "name": "Tumaco", "lat": 1.7986, "lng": -78.8156 },
  { "name": "Piedecuesta", "lat": 6.9878, "lng": -73.0495 },
  { "name": "Maicao", "lat": 11.3784, "lng": -72.2390 },
  { "name": "Uribia", "lat": 11.7149, "lng": -72.2659 },
  { "name": "San Juan de Girón", "lat": 7.0686, "lng": -73.1698 },
  { "name": "Fusagasugá", "lat": 4.3438, "lng": -74.3678 },
  { "name": "Mosquera", "lat": 4.7079, "lng": -74.2326 },
  { "name": "Malambo", "lat": 10.8595, "lng": -74.7739 },
  { "name": "Buga", "lat": 3.9009, "lng": -76.3021 },
  { "name": "Pitalito", "lat": 1.8537, "lng": -76.0507 },
  { "name": "Ciénaga", "lat": 11.0071, "lng": -74.2477 },
  { "name": "Ocaña", "lat": 8.2377, "lng": -73.3560 },
  { "name": "Lorica", "lat": 9.2365, "lng": -75.8135 },
  { "name": "Madrid", "lat": 4.7324, "lng": -74.2642 },
  { "name": "Santander", "lat": 7.1193, "lng": -73.1227 },
  { "name": "Aguachica", "lat": 8.3088, "lng": -73.6166 },
  { "name": "Sahagún", "lat": 8.9460, "lng": -75.4428 },
  { "name": "Yumbo", "lat": 3.5824, "lng": -76.4915 },
  { "name": "Cereté", "lat": 8.8848, "lng": -75.7907 },
  { "name": "Turbaco", "lat": 10.3293, "lng": -75.4114 },
  { "name": "Villa del Rosario", "lat": 7.8339, "lng": -72.4742 },
  { "name": "Tierralta", "lat": 8.1736, "lng": -76.0592 },
  { "name": "Los Patios", "lat": 7.8380, "lng": -72.5033 }
]
//===== public/virtue.js =====
/**
 * GIT:konopimi IG:neokpm
 * virte.js — minimal windowed virtualization for long HTML lists.
 *
 * Keeps only the DOM nodes near the viewport rendered. Two spacer
 * elements (top/bottom) simulate the height of everything that's
 * currently scrolled out of view. Item heights are measured per node
 * (so variable-height cards, e.g. with/without a photo, work fine) and
 * refined as they render or as their images load.
 *
 * Usage:
 *   import { VirtualList } from "./virtual-list.js";
 *
 *   const vlist = new VirtualList(containerEl, {
 *     overscan: 6,          // extra items rendered beyond the viewport edge
 *     estimatedHeight: 160, // initial guess before anything is measured
 *   });
 *
 *   vlist.setItems(items, (item) => `<div class="card">...</div>`);
 *
 *   // later, e.g. after a search filter changes the array:
 *   vlist.setItems(filteredItems, cardHtmlFn);
 *
 *   // if the container becomes visible again after being display:none
 *   // (e.g. a tab switch), force a recalculation:
 *   vlist.refresh();
 *
 *   // cleanup if the container is ever removed from the DOM:
 *   vlist.destroy();
 */
export class VirtualList {
  /**
   * @param {HTMLElement} containerEl - element that will hold the list.
   *   Its content is fully managed by VirtualList once constructed.
   * @param {Object} [options]
   * @param {number} [options.overscan=6] - extra items rendered past each viewport edge.
   * @param {number} [options.estimatedHeight=160] - initial per-item height guess (px).
   */
  constructor(containerEl, options = {}) {
    this.containerEl = containerEl;
    this.overscan = options.overscan ?? 6;
    this.avgHeight = options.estimatedHeight ?? 160;
    this.items = [];
    this.cardHtmlFn = null;
    this.heights = [];
    this.start = 0;
    this.end = 0;
    this.nodes = new Map(); // index -> element
    this.scrollParent = null;
    this._onScroll = null;
    this._rafScheduled = false;
    this._destroyed = false;
    this.containerEl.style.position = "relative";
    this.containerEl.innerHTML = "";
    this.topSpacer = document.createElement("div");
    this.bottomSpacer = document.createElement("div");
    this.containerEl.appendChild(this.topSpacer);
    this.containerEl.appendChild(this.bottomSpacer);
  }
  /**
   * Replace the items and/or the card renderer. Safe to call repeatedly
   * (e.g. every time a search filter changes the array) — if the array
   * reference differs from the last call, previously rendered nodes are
   * discarded and heights are reset to the last known estimates.
   *
   * @param {Array} items
   * @param {(item: any, index: number) => string} cardHtmlFn - returns
   *   HTML for a single item; must produce exactly one root element.
   */
  setItems(items, cardHtmlFn) {
    if (this._destroyed) return;
    if (this.items !== items) {
      this.nodes.forEach((node) => node.remove());
      this.nodes.clear();
      const oldHeights = this.heights;
      this.heights = items.map((_, i) => oldHeights[i] || this.avgHeight);
      this.start = 0;
      this.end = 0;
    }
    this.items = items;
    this.cardHtmlFn = cardHtmlFn;
    if (!this.scrollParent) {
      this.scrollParent = VirtualList._getScrollParent(this.containerEl);
      this._onScroll = () => this._scheduleUpdate();
      this.scrollParent.addEventListener("scroll", this._onScroll, { passive: true });
      window.addEventListener("resize", this._onScroll, { passive: true });
    }
    this._update(true);
  }
  /** Force a full recalculation — call after the container becomes
   * visible again (e.g. a tab switch away from display:none). */
  refresh() {
    if (this._destroyed) return;
    this._update(true);
  }
  /** Detach listeners and clear rendered nodes. */
  destroy() {
    if (this._destroyed) return;
    this._destroyed = true;
    if (this.scrollParent && this._onScroll) {
      this.scrollParent.removeEventListener("scroll", this._onScroll);
      window.removeEventListener("resize", this._onScroll);
    }
    this.nodes.forEach((node) => node.remove());
    this.nodes.clear();
  }
  // ---- internals ----
  static _getScrollParent(el) {
    let node = el.parentElement;
    while (node) {
      const style = getComputedStyle(node);
      if (/(auto|scroll)/.test(style.overflowY) && node.scrollHeight > node.clientHeight) {
        return node;
      }
      node = node.parentElement;
    }
    return document.scrollingElement || document.documentElement;
  }
  _getContainerOffsetTop(isWindowScroll) {
    if (isWindowScroll) {
      return this.containerEl.getBoundingClientRect().top + window.scrollY;
    }
    let top = 0;
    let node = this.containerEl;
    while (node && node !== this.scrollParent) {
      top += node.offsetTop;
      node = node.offsetParent;
    }
    return top;
  }
  _scheduleUpdate() {
    if (this._rafScheduled) return;
    this._rafScheduled = true;
    requestAnimationFrame(() => {
      this._rafScheduled = false;
      this._update(false);
    });
  }
  _update(force) {
    if (this._destroyed) return;
    const items = this.items;
    if (!items.length) {
      this.topSpacer.style.height = "0px";
      this.bottomSpacer.style.height = "0px";
      this.nodes.forEach((node) => node.remove());
      this.nodes.clear();
      this.start = 0;
      this.end = 0;
      return;
    }
    const scrollParent = this.scrollParent;
    const isWindowScroll = scrollParent === document.scrollingElement || scrollParent === document.documentElement;
    const viewportTop = isWindowScroll ? window.scrollY : scrollParent.scrollTop;
    const viewportHeight = isWindowScroll ? window.innerHeight : scrollParent.clientHeight;
    const containerTop = this._getContainerOffsetTop(isWindowScroll);
    const relativeScroll = Math.max(0, viewportTop - containerTop);
    const relativeBottom = relativeScroll + viewportHeight;
    const heights = this.heights;
    let acc = 0;
    let start = 0;
    for (; start < heights.length; start++) {
      if (acc + heights[start] > relativeScroll) break;
      acc += heights[start];
    }
    let end = start;
    let visAcc = acc;
    for (; end < heights.length; end++) {
      if (visAcc > relativeBottom) break;
      visAcc += heights[end];
    }
    start = Math.max(0, start - this.overscan);
    end = Math.min(heights.length, end + this.overscan);
    if (!force && start === this.start && end === this.end) return;
    let topSpacerHeight = 0;
    for (let i = 0; i < start; i++) topSpacerHeight += heights[i];
    let bottomSpacerHeight = 0;
    for (let i = end; i < heights.length; i++) bottomSpacerHeight += heights[i];
    this.topSpacer.style.height = topSpacerHeight + "px";
    this.bottomSpacer.style.height = bottomSpacerHeight + "px";
    this.nodes.forEach((node, idx) => {
      if (idx < start || idx >= end) {
        node.remove();
        this.nodes.delete(idx);
      }
    });
    for (let i = start; i < end; i++) {
      if (!this.nodes.has(i) && items[i] !== undefined) {
        const wrapper = document.createElement("div");
        wrapper.innerHTML = this.cardHtmlFn(items[i], i).trim();
        const node = wrapper.firstElementChild;
        if (node) {
          node.dataset.vIndex = i;
          this.nodes.set(i, node);
          node.querySelectorAll("img").forEach((img) => {
            if (!img.complete) {
              const bump = () => this._scheduleUpdate();
              img.addEventListener("load", bump, { once: true });
              img.addEventListener("error", bump, { once: true });
            }
          });
        }
      }
    }
    const orderedIndices = Array.from(this.nodes.keys()).sort((a, b) => a - b);
    orderedIndices.forEach((idx) => {
      this.containerEl.insertBefore(this.nodes.get(idx), this.bottomSpacer);
    });
    this.start = start;
    this.end = end;
    requestAnimationFrame(() => {
      if (this._destroyed) return;
      let changed = false;
      let total = 0;
      let count = 0;
      this.nodes.forEach((node, idx) => {
        const h = node.offsetHeight;
        if (h > 0) {
          if (Math.abs((this.heights[idx] || 0) - h) > 1) {
            this.heights[idx] = h;
            changed = true;
          }
          total += h;
          count++;
        }
      });
      if (count) this.avgHeight = total / count;
      if (changed) this._update(true);
    });
  }
}
//===== public/style.css =====
/* ===== RESET Y BASE ===== */
/* Leaflet.markercluster custom colors matching app palette */
.marker-cluster-small {
  background-color: rgba(63, 163, 77, 0.6);
}
.marker-cluster-medium {
  background-color: rgba(224, 166, 60, 0.6);
}
.marker-cluster-large {
  background-color: rgba(214, 69, 69, 0.6);
}
.marker-cluster-small div,
.marker-cluster-medium div,
.marker-cluster-large div {
  background-color: inherit;
}
/* Custom cluster icon: circular badge with emoji + count */
.marker-cluster {
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 50% !important;
  width: 40px !important;
  height: 40px !important;
}
.marker-cluster .cluster-counts {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 0;
  line-height: 1;
  white-space: nowrap;
}
.marker-cluster .cluster-emoji {
  font-size: 14px;
  line-height: 1;
}
.marker-cluster .cluster-total {
  font-size: 12px;
  font-weight: 700;
  color: #fff;
  line-height: 1;
}
* {
  box-sizing: border-box;
}
/* Remove the default (often white/blue) focus outline. Keyboard users
   still get a subtle, theme-consistent indicator via :focus-visible. */
:focus {
  outline: none;
}
:focus-visible {
  outline: 2px solid rgba(69, 214, 214, 0.6);
  outline-offset: 0px;
}
:root {
  /* Live-tracked heights (updated by JS via ResizeObserver).
     Defaults are fallbacks until the first measurement runs. */
  --top-bar-height: 47px;
  --driver-height: 0px;
  --surface: 42, 42, 42;
}
html {
  min-width: 100vw;
  max-width: 100vw;
  min-height: 100vh;
  min-height: 100dvh;
  max-height: 100vh;
  max-height: 100dvh;
  overflow: hidden;
}
body {
  display: flex;
  flex-direction: column;
  overflow-x: hidden;
  overflow-y: scroll;
  /* scrollbar-gutter: stable ; */
  margin: 0;
  font-family:
    -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
  font-size: 1rem;
  background: #0f1115;
  color: #eaeaea;
  min-width: 100vw;
  max-width: 100vw;
  min-height: 100vh;
  min-height: 100dvh;
  max-height: 100vh;
  max-height: 100dvh;
}
*::-webkit-scrollbar {
  display: none;
}
body {
  scrollbar-width: none;
  /* Firefox */
  -ms-overflow-style: none;
  /* IE/Edge */
}
body::-webkit-scrollbar {
  display: none;
}
body {
  scrollbar-width: none;
  /* Firefox */
  -ms-overflow-style: none;
  /* IE/Edge */
}
.map-sidebar-list::-webkit-scrollbar {
  display: none;
}
.map-sidebar-list {
  scrollbar-width: none;
  /* Firefox */
  -ms-overflow-style: none;
  /* IE/Edge */
}
h1 {
  /* font-size: 1rem; */
  margin: 0 0 5px 0;
}
.name {
  text-transform: uppercase;
  font-size: 110%;
  margin-bottom: 5px;
}
#commentsContainer {
  display: flex;
  flex-direction: column;
  flex: 1;
}
/* The map panel is a fixed full-viewport layer that sits BEHIND the
   sticky top bar and driver (which float over it with translucent blur).
   It is taken out of the body's flex/scroll flow entirely, so the other
   tab panels scroll normally between the sticky bars without the map's
   100dvh forcing overflow. Visibility is toggled from JS (showMap/
   hideMap) so it only paints on the map tab. */
#tabMapPanel {
  position: relative;
  inset: 0;
  padding: 0 !important;
  z-index: 1;
  /* below the sticky bars (9999/1000) and content */
}
#map {
  position: absolute;
  inset: 0;
  width: 100%;
  height: 100%;
}
.leaflet-control-zoom {
  bottom: 25px;
  /* bottom: calc(var(--driver-height) + 25px); */
  /* Move to bottom-right */
}
/* ===== OFFSCREEN ARROWS (items outside the viewport) ===== */
#offscreenArrows {
  position: absolute;
  inset: 0;
  pointer-events: none;
  z-index: 999999000000;
}
.offscreen-arrow {
  position: absolute;
  pointer-events: auto;
  width: 22px;
  height: 22px;
  border-radius: 50%;
  background: rgba(20, 20, 20, 0.7);
  color: #fff;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 12px;
  cursor: pointer;
  transform: translate(-50%, -50%);
  box-shadow: 0 1px 4px rgba(0, 0, 0, 0.5);
  transition: background 0.15s;
}
.offscreen-arrow:hover {
  background: rgba(60, 60, 60, 0.9);
}
.offscreen-arrow .arrow-glyph {
  display: block;
  line-height: 1;
}
.map-sidebar {
  display: none;
  position: fixed;
  /* Start below the sticky top bar so the first card isn't occluded. */
  top: var(--top-bar-height);
  left: 0px;
  bottom: var(--driver-height);
  width: 220px;
  max-width: calc(100% - 20px);
  background: rgba(40, 40, 40, 0.55);
  backdrop-filter: blur(8px);
  border: 1px solid rgba(255, 255, 255, 0.08);
  z-index: 9999;
  flex-direction: column;
  overflow: hidden;
}
.map-sidebar-title {
  display: none;
  flex: 0 0 auto;
  padding: 10px 12px 6px;
  font-size: 0.8rem;
  font-weight: 700;
  color: #ccc;
  text-transform: uppercase;
  letter-spacing: 0.4px;
}
.map-sidebar-list {
  flex: 1;
  overflow-y: auto;
  padding: 2px;
  display: flex;
  flex-direction: column;
  gap: 3px;
}
.map-sidebar-card {
  display: flex;
  overflow: hidden;
  align-items: center;
  gap: 8px;
  /* Keep natural height: without this, flex-shrink:1 (default) squeezes
     every card to fit the list height when there are many items, instead
     of letting the list scroll. */
  flex-shrink: 0;
  /* Background and left border are derived from the status color (set
     inline as --status-color) via color-mix, matching the main cards. */
  background: color-mix(in srgb,
      var(--status-color, #e57373) 12%,
      rgba(26, 29, 36, 0.85));
  border-left: 3px solid var(--status-color, #e57373);
  border-radius: 8px;
  cursor: pointer;
  transition: background 0.15s;
}
.map-sidebar-card:hover {
  background: color-mix(in srgb,
      var(--status-color, #e57373) 20%,
      rgba(34, 38, 46, 0.85));
}
/* Angel is a special light-blue case (deceased), not a dark status tint. */
.map-sidebar-card.angel {
  background: rgba(127, 184, 236, 0.62);
  border-left-color: #add8e6;
}
.map-sidebar-photo {
  width: 40px;
  height: 40px;
  object-fit: cover;
  flex-shrink: 0;
  /* background: #12151c; */
}
.map-sidebar-info {
  flex: 1;
  min-width: 0;
  display: flex;
  flex-direction: column;
  gap: 1px;
}
.map-sidebar-name {
  font-weight: 600;
  font-size: 0.82rem;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}
.map-sidebar-meta {
  font-size: 0.72rem;
  color: #999;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}
.map-sidebar-card .status-tag {
  margin-left: 0;
  flex-shrink: 0;
  font-size: 62%;
  padding: 3px 5px;
}
@media (max-width: 768px) {
  .leaflet-control-zoom {
    bottom: 25px;
    /* bottom: calc(var(--driver-height) + 40px); */
    /* Move to bottom-right */
  }
  .map-sidebar {
    top: auto;
    left: 0;
    right: 0;
    bottom: var(--driver-height);
    width: auto;
    max-width: 100%;
    /* Fixed height so the horizontal strip doesn't collapse (flex:1 child
       in an auto-height column flex container shrinks to 0). */
    height: 56px;
  }
  .map-sidebar-list {
    flex: 0 0 auto;
    flex-direction: row;
    overflow-x: auto;
    overflow-y: hidden;
  }
  .map-sidebar-card {
    flex: 0 0 auto;
    width: 190px;
    height: 100%;
  }
}
.top-bar {
  position: sticky;
  top: 0px;
  gap: 5px;
  background: rgba(var(--surface), 0.38);
  backdrop-filter: blur(10px);
  border-bottom: 2px solid rgb(var(--surface), 0.62);
  padding: 5px;
  z-index: 9999;
}
.crear {
  border: 1px solid gray;
  padding: 8px 10px;
  border-radius: 10px;
  margin-bottom: 8px;
  background: teal;
}
.tool-bar {
  display: flex;
  position: sticky;
  top: 47px;
  flex: 1;
  width: 100%;
  /* max-width: calc(100%-50px); */
  padding: 5px;
  z-index: 999;
  margin-bottom: 5px;
}
/* Button that opens the creation modal (replaces inline forms) */
.crear-btn {
  text-align: left;
  border: 1px solid palegoldenrod;
  padding: 5px 10px;
  border-radius: 5px;
  margin-right: auto;
  background: teal;
  color: palegoldenrod;
  font-size: 70% !important;
  font-weight: 700;
  cursor: pointer;
}
.crear-btn:hover {
  filter: brightness(1.1);
}
/* ===== FILTER ROW (inside .tool-bar) ===== */
.filter-row {
  display: flex;
  justify-content: right;
  gap: 5px;
  flex-wrap: wrap;
  flex: 1;
  min-width: 0;
}
.filter-pills {
  display: flex;
  gap: 4px;
  flex-wrap: wrap;
}
.filter-pill {
  border: 1px solid #333;
  background: #1a1d24;
  color: #ccc;
  padding: 4px 9px;
  border-radius: 14px;
  font-size: 70%;
  font-weight: 600;
  cursor: pointer;
  white-space: nowrap;
  transition:
    background 0.12s,
    color 0.12s,
    border-color 0.12s;
}
.filter-pill:hover {
  filter: brightness(1.25);
}
.filter-pill.active {
  background: teal;
  color: palegoldenrod;
  border-color: palegoldenrod;
}
/* Status pills: active color follows the status color. */
.filter-pill[data-status="desaparecido"].active {
  background: #e57373;
  color: #fff;
  border-color: #e57373;
}
.filter-pill[data-status="encontrado"].active {
  background: #66bb6a;
  color: #fff;
  border-color: #66bb6a;
}
.filter-pill[data-status="angel"].active {
  background: #add8e6;
  color: #222;
  border-color: #add8e6;
}
.filter-pill[data-status="seguro"].active {
  background: #66bb6a;
  color: #fff;
  border-color: #66bb6a;
}
.filter-pill[data-status="danado"].active {
  background: #f0c060;
  color: #222;
  border-color: #f0c060;
}
.filter-pill[data-status="colapsado"].active {
  background: #d06060;
  color: #fff;
  border-color: #d06060;
}
.filter-pill[data-status="acopio"].active {
  background: #ffffff;
  color: #222;
  border-color: #ffffff;
}
.filter-select {
  padding: 4px 8px;
  border-radius: 14px;
  border: 1px solid #333;
  background: #1a1d24;
  color: #eaeaea;
  font-size: 70%;
  cursor: pointer;
  max-width: 160px;
}
.filter-select:focus {
  outline: none;
  border-color: palegoldenrod;
}
.filter-input {
  padding: 4px 8px;
  border-radius: 14px;
  border: 1px solid #333;
  background: #1a1d24;
  color: #eaeaea;
  font-size: 70%;
  max-width: 120px;
}
.filter-input::placeholder {
  color: #777;
}
.filter-input:focus {
  outline: none;
  border-color: palegoldenrod;
}
/* ===== WIKI ===== */
.wiki {
  display: flex;
  flex-direction: column;
  gap: 8px;
  padding: 4px 0;
}
.wiki details {
  background: #1a1d24;
  border: 1px solid #2a2e36;
  border-radius: 10px;
  overflow: hidden;
}
.wiki summary {
  cursor: pointer;
  padding: 12px 14px;
  font-weight: 600;
  list-style: none;
  user-select: none;
}
.wiki summary::-webkit-details-marker {
  display: none;
}
.wiki summary::before {
  content: "▸ ";
  color: #d64545;
}
.wiki details[open] summary::before {
  content: "▾ ";
}
.wiki-body {
  padding: 0 14px 12px 14px;
  color: #c9ccd2;
  line-height: 1.6;
}
.wiki-body p,
.wiki-body ul,
.wiki-body ol {
  margin: 6px 0;
}
.wiki-body li {
  margin-bottom: 4px;
}
.wiki-body b {
  color: #eaeaea;
}
/* Each form inside the crear modal; only the active one is shown */
.crear-form-wrap {
  display: none;
}
.crear-form-wrap.active {
  display: block;
}
/* Forms inside the modal reuse .crear form styling */
#crearModal .crear-form-wrap form {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  align-items: center;
}
#crearModal .crear-form-wrap form select {
  max-width: 100px;
}
#crearModal .crear-form-wrap form input,
#crearModal .crear-form-wrap form select {
  flex: 1 1 150px;
  padding: 10px 12px;
  border-radius: 8px;
  border: 1px solid #333;
  background: #1a1d24;
  color: #eaeaea;
}
#crearModal .crear-form-wrap form .gps-btn {
  flex: 0 0 auto;
  background: #333;
  padding: 10px 14px;
}
#crearModal .crear-form-wrap form .gps-btn:hover {
  background: #444;
}
.crear form {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  margin-top: 6px;
  align-items: center;
}
.crear form input,
.crear form select {
  flex: 1 1 150px;
  min-width: 120px;
  padding: 10px 12px;
  /* font-size: 0.95rem; */
  border-radius: 8px;
  border: 1px solid #333;
  background: #1a1d24;
  color: #eaeaea;
}
.crear form .gps-btn {
  flex: 0 0 auto;
  background: #333;
  padding: 10px 14px;
}
.crear form .gps-btn:hover {
  background: #444;
}
.crear form button[type="submit"] {
  flex: 1 1 100px;
  background: #d64545;
  color: white;
  border: none;
  border-radius: 8px;
  padding: 10px 16px;
  cursor: pointer;
  /* font-size: 0.95rem; */
  transition: background 0.15s;
}
.crear form button[type="submit"]:hover {
  background: #b53838;
}
/* ===== BOTÓN DE SUBIDA DE ARCHIVO PERSONALIZADO ===== */
.custom-file-upload {
  position: relative;
  display: inline-block;
  padding: 10px 16px;
  background: #2a2e36;
  color: #ddd;
  border-radius: 8px;
  cursor: pointer;
  /* font-size: 0.95rem; */
  transition: background 0.15s;
  border: 1px solid #444;
  flex: 0 0 auto;
  white-space: nowrap;
}
.custom-file-upload:hover {
  background: #3a3e46;
}
.custom-file-upload input[type="file"] {
  position: absolute;
  width: 1px;
  height: 1px;
  padding: 0;
  margin: -1px;
  overflow: hidden;
  clip: rect(0, 0, 0, 0);
  white-space: nowrap;
  border: 0;
}
.file-name {
  display: none;
  color: #999;
  /* font-size: 0.85rem; */
  margin-left: 6px;
  flex: 1 1 120px;
  min-width: 80px;
  padding: 6px 0;
}
/* ===== TABS ===== */
.tabs {
  display: flex;
  padding: 8px;
  padding-top: 12px;
  gap: 5px;
  flex-wrap: wrap;
  align-items: center;
  justify-content: center;
}
#driver {
  position: sticky;
  bottom: 0px;
  left: 0px;
  z-index: 1000;
}
.driver-inner {
  overflow: hidden;
  background: rgba(20, 20, 20, 0.85);
  backdrop-filter: blur(5px);
  border-top: 2px solid rgba(var(--surface), 0.62);
  display: flex;
  flex-direction: column;
  border-radius: 10px 10px 0px 0px;
  /* overflow: hidden; */
  /* gap: 10px; */
  /* padding-left: 5px; */
  /* padding-right: 5px; */
  /* padding-top: 11px; */
  min-width: 100%;
  max-width: 100%;
}
button {
  min-width: 25px;
  padding: 5px;
  border-radius: 8px;
  border: 1px solid rgba(var(--surface), 0.38);
  background: rgba(20, 20, 20, 0.8);
  color: #888;
  cursor: pointer;
  box-shadow:
    rgba(0, 0, 0, 0.4) 0px 2px 4px,
    rgba(0, 0, 0, 0.3) 0px 7px 13px -3px,
    rgba(60, 60, 60, 0.8) 0px 3px 0px;
  /* outside, below */
  transition: background 0.15s;
}
input,
textarea {
  padding: 5px;
  padding-left: 20px;
  border-radius: 8px;
  border: 2px solid rgba(var(--surface), 0.62);
  background: rgba(10, 10, 10, 0.8);
  color: #888;
  /* font-size: 0.95rem; */
}
.tab-btn {
  position: relative;
}
.tab-btn.active {
  background: rgba(0, 128, 128, 0.62);
  color: white;
}
/* live count badge on category tabs */
.tab-btn .n,
.sub-tab-btn .n {
  position: absolute;
  top: -10px;
  right: 2px;
  min-width: 16px;
  height: 16px;
  font-family: ui-monospace, monospace;
  font-size: 62%;
  font-weight: 1000;
  padding: 0px 3px;
  line-height: 16px;
  text-align: center;
  border-radius: 6px;
  background: rgba(20, 20, 20, 0.38);
  opacity: 0.85;
  color: white;
  vertical-align: middle;
  box-shadow: 0 0 0 1px rgba(100, 100, 100, 0.8);
}
.tab-btn.active .n {
  background: rgba(0, 128, 128, 0.38);
  /* color: blue; */
  box-shadow: 0 0 0 1px rgba(0, 160, 160, 0.8);
}
.tab-panel {
  display: none;
  padding: 5px;
  flex: 1;
}
.tab-panel.active {
  display: block;
}
.search-row>input {
  border-radius: 0px !important;
  border: none !important;
  border-bottom: 2px solid rgba(var(--surface), 0.62) !important;
  padding: 10px 20px !important;
  text-align: center;
}
.search-row input[type="search"] {
  width: 100%;
}
/* ===== LISTA ===== */
.list {
  display: grid;
  padding-bottom: 200px;
  grid-template-columns: repeat(auto-fill, minmax(250px, 1fr));
  gap: 10px;
}
.wiki {
  padding-bottom: 200px;
}
.card {
  display: flex;
  flex-direction: column;
  width: 100%;
  min-width: 200px;
  /* max-width: 300px; */
  min-height: 170px;
  max-height: 170px;
  font-size: 62%;
  /* Background and left border are derived from the status color (set
     inline as --status-color by the card renderers) via color-mix, so
     there's no per-status hardcoding to keep in sync. */
  background: color-mix(in srgb, var(--status-color, #e57373) 12%, #1a1d24);
  border-radius: 10px;
  border-left: 4px solid var(--status-color, #e57373);
  cursor: default;
  overflow: hidden;
  transition: background 0.15s;
  box-shadow:
    rgba(0, 0, 0, 0.4) 0px 2px 4px,
    rgba(0, 0, 0, 0.3) 0px 7px 13px -3px,
    rgba(60, 60, 60, 0.8) 0px 3px 0px;
}
.card:hover {
  background: color-mix(in srgb, var(--status-color, #e57373) 20%, #22262e);
}
.card.colaborador {
  min-height: 120px;
  max-height: 120px;
}
.card.colaborador .card-photo {
  width: 70px;
  min-height: 70px;
  max-height: 70px;
}
.card.colaborador .card-inner {
  gap: 4px;
}
.card.colaborador .card-main {
  padding: 6px 0;
}
.card .card-inner {
  display: flex;
  flex: 1;
  flex-direction: column;
  gap: 12px;
  margin-left: 5px;
  overflow: hidden;
}
.card-photo {
  width: 130px;
  min-height: 100%;
  max-height: 100%;
  object-fit: cover;
  flex-shrink: 0;
  /* background: #12151c; */
}
.card-main {
  display: flex;
  overflow: hidden;
  /* flex-direction: column; */
  flex: 1;
  /* padding: 5px; */
}
.card .info {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 2px;
}
.card .name {
  font-weight: 600;
  /* font-size: 1.05rem; */
}
.card .meta {
  /* font-size: 0.8rem; */
  /* color: #999; */
}
.card .actions {
  /* display: flex; */
  padding: 5px;
  gap: 6px;
  flex-wrap: wrap;
  margin-top: auto;
  margin-left: auto;
  pointer-events: auto;
}
.card .actions button {
  font-size: 85% !important;
}
.anuncio-text {
  padding: 5px;
  white-space: pre-wrap;
  word-wrap: break-word;
  overflow-wrap: break-word;
  font-family: inherit;
  margin: 0;
  /* font-size: 1.05rem; */
  font-weight: 600;
  color: #eaeaea;
}
.map-search-result {
  padding: 8px 10px;
  cursor: pointer;
  /* font-size: 0.85rem; */
  border-bottom: 1px solid #2a2e36;
}
.map-search-result:hover {
  background: #22262e;
}
.map-search-result:last-child {
  border-bottom: none;
}
#mapPickerContainer {
  height: 320px;
  /* border-radius: 10px; */
  overflow: hidden;
  /* margin-bottom: 12px; */
}
.extra-location-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: 8px;
  background: #12151c;
  border-radius: 8px;
  padding: 6px 10px;
  margin-bottom: 6px;
  /* font-size: 0.85rem; */
  flex-wrap: wrap;
}
.nearby-item {
  cursor: pointer;
  padding: 7px 10px;
  border-radius: 6px;
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: 8px;
  /* font-size: 0.88rem; */
}
.nearby-item:hover {
  background: #1a1d24;
}
.nearby-item .nearby-dist {
  color: #888;
  /* font-size: 0.78rem; */
  flex: 0 0 auto;
}
#mapPickerContext {
  align-items: center;
  gap: 8px;
  background: #12151c;
  border-radius: 8px;
  padding: 8px 10px;
  margin-bottom: 10px;
  /* font-size: 0.9rem; */
}
.btn-share {
  background: #1e6b52;
  color: #ccc;
}
.btn-share:hover {
  background: #24805f;
}
/* ===== STATUS TAGS ===== */
.status-tag {
  margin: 5px;
  margin-left: auto;
  text-transform: uppercase;
  padding: 5px;
  border-radius: 4px;
  background: #e57373;
  display: inline-block;
  letter-spacing: 0.3px;
  color: white;
  font-size: 80%;
}
.status-tag.encontrado,
.status-tag.seguro {
  background: #66bb6a;
}
.status-tag.danado {
  background: #f0c060;
  color: #222;
}
.status-tag.colapsado {
  background: #d06060;
}
.status-tag.acopio {
  background: #ffffff;
  color: #222;
}
.status-tag.colaborador {
  background: #9b59b6;
}
.empty {
  display: none;
  text-align: center;
  color: #666;
  padding: 40px 0;
}
.photo-preview {
  width: 50px;
  height: 50px;
  object-fit: cover;
  border-radius: 8px;
}
.comment-badge {
  /* font-size: 0.78rem; */
  color: #9aa0a6;
  margin-top: 4px;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  max-width: 100%;
}
.comment-item {
  background: #12151c;
  border-radius: 8px;
  padding: 8px 12px;
  margin-bottom: 6px;
  display: flex;
  justify-content: space-between;
  align-items: center;
  border-left: 2px solid #d64545;
}
.comment-item .comment-text {
  color: #eaeaea;
  word-break: break-word;
  white-space: pre-wrap;
  word-wrap: break-word;
  overflow-wrap: break-word;
  font-family: inherit;
  margin: 0;
}
.comment-item .comment-meta {
  /* font-size: 0.7rem; */
  color: #888;
}
#commentForm {
  display: flex;
  gap: 8px;
  flex-wrap: wrap;
}
#commentForm textarea {
  flex: 1;
  min-height: 100px;
}
#commentForm button {
  margin-left: auto;
}
.sub-tabs .tab-label {
  display: none;
}
@media (max-width: 900px) {
  .tab-label {
    display: none;
  }
  /* Building status pills: icon only on small screens. */
  .filter-pill .pill-label {
    display: none;
  }
}
@media (max-width: 480px) {
  .crear form input,
  .crear form select {
    flex: 1 1 100%;
  }
  .custom-file-upload {
    flex: 1 1 100%;
    text-align: center;
  }
}
.sismo-head {
  position: sticky;
  top: calc(var(--top-bar-height) + 5px);
  z-index: 998;
  display: flex;
  flex-direction: column;
  gap: 6px;
}
/* ===== ALERTA SISMO (card, consistente con .card) ===== */
.alerta {
  display: flex;
  flex-direction: column;
  gap: 6px;
  padding: 10px 14px;
  background: #1a1d24;
  border: 1px solid #333;
  border-left: 4px solid #74b9ff;
  border-radius: 10px;
  box-shadow:
    rgba(0, 0, 0, 0.4) 0px 2px 4px,
    rgba(0, 0, 0, 0.3) 0px 7px 13px -3px;
  transition:
    background 0.3s,
    border-color 0.3s;
}
.alerta-head {
  display: flex;
  justify-content: space-between;
  align-items: center;
  font-size: 70%;
  color: #999;
}
.alerta-title {
  font-weight: 700;
  letter-spacing: 0.03em;
  text-transform: uppercase;
}
.alerta-rel {
  color: #ccc;
  font-weight: 600;
}
.alerta-body {
  display: flex;
  align-items: center;
  gap: 12px;
}
.alerta-mag {
  min-width: 56px;
  text-align: center;
  font-size: 150%;
  font-weight: 800;
  border-radius: 8px;
  padding: 4px 0;
  background: #74b9ff;
  color: #0f1115;
}
.alerta-info {
  display: flex;
  flex-direction: column;
  gap: 2px;
  min-width: 0;
}
.alerta-lugar {
  font-weight: 600;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}
.alerta-hora {
  font-size: 80%;
  color: #999;
}
.alerta.peligro {
  border-left-color: #d63031;
  background: #2a1518;
  animation: pulse 1.5s infinite;
}
.alerta.peligro .alerta-mag {
  background: #d63031;
  color: #fff;
}
@keyframes pulse {
  0% {
    transform: scale(1);
  }
  50% {
    transform: scale(1.02);
  }
  100% {
    transform: scale(1);
  }
}
/* ===== LISTA TEXTO DE SISMOS (fallback sin ECharts) ===== */
.sismos-list {
  display: flex;
  flex-direction: column;
  gap: 6px;
  padding: 0 4px;
}
.sismo-row {
  display: flex;
  flex-direction: column;
  gap: 6px;
  padding: 10px 12px;
  border-radius: 10px;
  background: #1a1d24;
  border: 1px solid #333;
  border-left: 4px solid #74b9ff;
  font-size: 14px;
  box-shadow:
    rgba(0, 0, 0, 0.4) 0px 2px 4px,
    rgba(0, 0, 0, 0.3) 0px 7px 13px -3px;
}
/* Borde izquierdo según magnitud (agrupa visualmente) */
.sismo-row.sev-0 {
  border-left-color: #74b9ff;
}
.sismo-row.sev-4 {
  border-left-color: #fdcb6e;
}
.sismo-row.sev-5 {
  border-left-color: #e17055;
}
.sismo-row.sev-6 {
  border-left-color: #d63031;
}
.sismo-row .sismo-top {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: 8px;
}
.sismo-row .sismo-bottom {
  display: flex;
  align-items: center;
  gap: 10px;
  flex-wrap: wrap;
}
.sismo-row .mag {
  min-width: 42px;
  text-align: center;
  font-weight: bold;
  font-size: 16px;
  border-radius: 6px;
  padding: 2px 0;
}
.sismo-row .mag.m0 {
  background: #74b9ff;
  color: #0f1115;
}
.sismo-row .mag.m4 {
  background: #fdcb6e;
  color: #0f1115;
}
.sismo-row .mag.m5 {
  background: #e17055;
  color: #fff;
}
.sismo-row .mag.m6 {
  background: #d63031;
  color: #fff;
}
.sismo-row .place {
  font-weight: 600;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}
.sismo-row .rel {
  font-size: 12px;
  color: #999;
  white-space: nowrap;
}
.sismo-row .meta {
  font-size: 12px;
  color: #999;
}
.sismo-row a {
  color: #74b9ff;
  text-decoration: none;
  font-size: 13px;
  white-space: nowrap;
}
.sismo-row a:hover {
  text-decoration: underline;
}
.sismo-row .depth-pill {
  padding: 2px 8px;
  border-radius: 12px;
  border: 1px solid #333;
  background: #1a1d24;
  color: #ccc;
  font-size: 12px;
  white-space: nowrap;
}
/* ===== FEED EN VIVO (ventana corta, poll 60s) ===== */
.sismo-live {
  border: 1px solid #2f3b4d;
  border-radius: 10px;
  background: rgba(var(--surface), 0.38);
  backdrop-filter: blur(10px);
  overflow: hidden;
  box-shadow:
    rgba(0, 0, 0, 0.4) 0px 2px 4px,
    rgba(0, 0, 0, 0.3) 0px 7px 13px -3px;
}
.sismo-live-head {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 12px;
  border-bottom: 1px solid #2a2f3a;
  background: rgba(var(--surface), 0.23);
}
.sismo-live-dot {
  width: 9px;
  height: 9px;
  border-radius: 50%;
  background: #3ddc84;
  box-shadow: 0 0 0 0 rgba(61, 220, 132, 0.6);
  animation: live-pulse 2s infinite;
}
@keyframes live-pulse {
  0% {
    box-shadow: 0 0 0 0 rgba(61, 220, 132, 0.6);
  }
  70% {
    box-shadow: 0 0 0 8px rgba(61, 220, 132, 0);
  }
  100% {
    box-shadow: 0 0 0 0 rgba(61, 220, 132, 0);
  }
}
.sismo-live-title {
  font-size: 75%;
  font-weight: 700;
  letter-spacing: 0.04em;
  text-transform: uppercase;
  color: #3ddc84;
}
.sismo-live-updated {
  margin-left: auto;
  font-size: 70%;
  color: #888;
}
.sismo-live-list {
  display: flex;
  flex-direction: column;
  gap: 0;
  max-height: 170px;
  overflow-y: auto;
}
.sismo-live-item {
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 8px 12px;
  border-left: 4px solid #74b9ff;
  font-size: 13px;
}
.sismo-live-item+.sismo-live-item {
  border-top: 1px solid #23272f;
}
.sismo-live-item.sev-0 {
  border-left-color: #74b9ff;
}
.sismo-live-item.sev-4 {
  border-left-color: #fdcb6e;
}
.sismo-live-item.sev-5 {
  border-left-color: #e17055;
}
.sismo-live-item.sev-6 {
  border-left-color: #d63031;
}
.sismo-live-item .mag {
  min-width: 38px;
  text-align: center;
  font-weight: 700;
  font-size: 14px;
  border-radius: 6px;
  padding: 2px 0;
}
.sismo-live-item .mag.m0 {
  background: #74b9ff;
  color: #0f1115;
}
.sismo-live-item .mag.m4 {
  background: #fdcb6e;
  color: #0f1115;
}
.sismo-live-item .mag.m5 {
  background: #e17055;
  color: #fff;
}
.sismo-live-item .mag.m6 {
  background: #d63031;
  color: #fff;
}
.sismo-live-item .place {
  flex: 1;
  font-weight: 600;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  min-width: 0;
}
.sismo-live-item .rel {
  font-size: 12px;
  color: #999;
  white-space: nowrap;
}
.sismo-live-item .depth {
  font-size: 11px;
  color: #aaa;
  white-space: nowrap;
}
.sismo-live-empty {
  padding: 12px;
  font-size: 13px;
  color: #999;
}
/* ============================================================
   SUB-TABS de la pestaña Colaboradores (privada)
   ============================================================ */
.sub-tabs {
  display: flex;
  /* flex-direction: column; */
  gap: 4px;
  padding: 5px;
  overflow-x: auto;
  border-bottom: 1px solid rgba(var(--surface), 0.62);
  /* background: rgba(var(--surface), 0.23); */
  flex-shrink: 0;
  height: 100%;
  border-radius: 10px;
}
.sub-tab-btn {}
.sub-tab-btn:hover {
  background: rgba(255, 255, 255, 0.06);
  color: #ccc;
}
.sub-tab-btn.active {
  background: rgba(0, 128, 128, 0.62);
  color: white;
  /* background: rgba(69, 214, 214, 0.15); */
  /* color: #45d6d6; */
}
/* Botones de acción (DONAR/REPORTAR/CREAR/COLABORAR) dentro de la barra de sub-tabs */
.sub-tabs .crear-btn {
  margin-right: 0;
  text-align: center;
  white-space: nowrap;
  flex-shrink: 0;
}
.sub-tab-panel {
  display: none;
  flex: 1;
  overflow: hidden;
  min-height: 0;
}
.sub-tab-panel.active {
  display: flex;
  flex-direction: column;
}
/* ============================================================
   Status tags por tipo (donaciones / necesidades / logística)
   ============================================================ */
/* Donaciones */
.status-tag.donacion-disponible {
  background: rgba(63, 163, 77, 0.2);
  color: #3fa34d;
}
.status-tag.donacion-reservado {
  background: rgba(240, 173, 78, 0.2);
  color: #f0ad4e;
}
.status-tag.donacion-entregado {
  background: rgba(128, 128, 128, 0.2);
  color: #888;
}
.status-tag.donacion-vencido {
  background: rgba(214, 48, 49, 0.2);
  color: #d63031;
}
/* Necesidades */
.status-tag.necesidad-abierta {
  background: rgba(214, 48, 49, 0.2);
  color: #d63031;
}
.status-tag.necesidad-en_proceso {
  background: rgba(240, 173, 78, 0.2);
  color: #f0ad4e;
}
.status-tag.necesidad-cubierta {
  background: rgba(63, 163, 77, 0.2);
  color: #3fa34d;
}
.status-tag.necesidad-urgente {
  background: rgba(214, 48, 49, 0.25);
  color: #ff5050;
}
/* Logística */
.status-tag.logistica-pendiente {
  background: rgba(128, 128, 128, 0.2);
  color: #888;
}
.status-tag.logistica-en_ruta {
  background: rgba(69, 214, 214, 0.2);
  color: #45d6d6;
}
.status-tag.logistica-completado {
  background: rgba(63, 163, 77, 0.2);
  color: #3fa34d;
}
.status-tag.logistica-cancelado {
  background: rgba(214, 48, 49, 0.2);
  color: #d63031;
}
/* ===== Media grid (WhatsApp-style album) inside chat bubbles ===== */
.chat-media-grid {
  display: grid;
  gap: 2px;
  border-radius: 8px;
  overflow: hidden;
}
.chat-media-item {
  position: relative;
  aspect-ratio: 1 / 1;
  overflow: hidden;
  background: #12151c;
}
/* Single-image groups shouldn't be forced square — let it breathe like
   a normal photo message. */
.chat-media-grid[data-count="1"] .chat-media-item {
  aspect-ratio: auto;
  max-height: 320px;
}
.chat-media-grid[data-count="1"] .chat-media-item img,
.chat-media-grid[data-count="1"] .chat-media-item video {
  object-fit: contain !important;
}
.chat-media-more {
  position: absolute;
  inset: 0;
  background: rgba(0, 0, 0, 0.55);
  color: #fff;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 1.2rem;
  font-weight: 700;
  pointer-events: none;
}
/* ===== Lightbox ===== */
#lightboxStage img,
#lightboxStage video {
  max-width: 100%;
  max-height: calc(100vh - 60px);
  object-fit: contain;
  display: block;
}
.lightbox-nav {
  position: absolute;
  top: 50%;
  transform: translateY(-50%);
  background: rgba(20, 20, 20, 0.6);
  color: #fff;
  border: none;
  font-size: 28px;
  width: 44px;
  height: 44px;
  border-radius: 50%;
  cursor: pointer;
  z-index: 5;
}
.lightbox-prev { left: 10px; }
.lightbox-next { right: 10px; }
.lightbox-nav[disabled] { opacity: 0.25; cursor: default; }
//===== public/sismos.js =====
// ================================================================
//  SISMOS EN TIEMPO REAL (ECharts + lista texto)
//  Self-contained module. Exposes window.initSismos() which index.js
//  calls when the Sismos tab is activated.
// ================================================================
(function() {
  "use strict";
  console.log("[sismos.js] cargado, definiendo window.initSismos");
  let sismosChart = null;
  let sismosData = [];
  let liveData = [];
  let liveWired = false;
  let liveFetchAttempted = false;
  // Estado de filtros (client-side; el backend ya trae hasta 500).
  const filters = {
    minMag: 0, // 0 = todas
    depth: "", // shallow | mid | deep | vdeep
    days: 0, // 0 = toda
    location: "", // texto libre sobre el lugar
  };
  // ================================================================
  //  HELPERS
  // ================================================================
  function esc(str) {
    const div = document.createElement("div");
    div.textContent = str == null ? "" : String(str);
    return div.innerHTML;
  }
  function magClass(mag) {
    if (mag >= 6) return "m6";
    if (mag >= 5) return "m5";
    if (mag >= 4) return "m4";
    return "m0";
  }
  // Clase para el borde izquierdo según magnitud (agrupa visualmente).
  function sevClass(mag) {
    if (mag >= 6) return "sev-6";
    if (mag >= 5) return "sev-5";
    if (mag >= 4) return "sev-4";
    return "sev-0";
  }
  // Color hex según magnitud (consistente con los bordes y badges).
  function sevColor(mag) {
    if (mag >= 6) return "#d63031";
    if (mag >= 5) return "#e17055";
    if (mag >= 4) return "#fdcb6e";
    return "#74b9ff";
  }
  function depthLabel(d) {
    if (d == null) return "—";
    if (d < 30) return "Superficial";
    if (d < 70) return "Intermedia";
    if (d < 150) return "Profunda";
    return "Muy profunda";
  }
  function relTime(ts) {
    const diff = Date.now() - ts;
    const s = Math.floor(diff / 1000);
    if (s < 60) return "hace unos segundos";
    const m = Math.floor(s / 60);
    if (m < 60) return `hace ${m} min`;
    const h = Math.floor(m / 60);
    if (h < 24) return `hace ${h} h`;
    const d = Math.floor(h / 24);
    if (d < 30) return `hace ${d} d`;
    const mo = Math.floor(d / 30);
    if (mo < 12) return `hace ${mo} mes${mo > 1 ? "es" : ""}`;
    const y = Math.floor(d / 365);
    return `hace ${y} año${y > 1 ? "s" : ""}`;
  }
  function fullTime(ts) {
    return new Date(ts).toLocaleString("es-CO", {
      day: "2-digit",
      month: "2-digit",
      year: "numeric",
      hour: "2-digit",
      minute: "2-digit",
      timeZone: "America/Bogota",
    });
  }
  // ================================================================
  //  FILTROS
  // ================================================================
  function applyFilters() {
    const now = Date.now();
    const loc = filters.location.trim().toLowerCase();
    return sismosData
      .filter((q) => {
        const mag = q.mag || 0;
        if (mag < filters.minMag) return false;
        const d = q.depth == null ? 0 : q.depth;
        if (filters.depth === "shallow" && !(d < 30)) return false;
        if (filters.depth === "mid" && !(d >= 30 && d < 70)) return false;
        if (filters.depth === "deep" && !(d >= 70 && d < 150)) return false;
        if (filters.depth === "vdeep" && !(d >= 150)) return false;
        if (filters.days > 0 && now - q.timestamp > filters.days * 86400000) {
          return false;
        }
        if (loc && !(q.place || "").toLowerCase().includes(loc)) return false;
        return true;
      })
      .sort((a, b) => b.timestamp - a.timestamp);
  }
  // ================================================================
  //  ECharts
  // ================================================================
  function initSismoChart() {
    const dom = document.getElementById("sismoChart");
    if (!dom) return;
    if (typeof echarts === "undefined") {
      console.error("ECharts no está cargado. Verifica el CDN en <head>.");
      return;
    }
    sismosChart = echarts.init(dom);
    window.sismoChart = sismosChart;
    setTimeout(() => sismosChart && sismosChart.resize(), 50);
    const option = {
      tooltip: {
        trigger: "axis",
        axisPointer: { type: "shadow" },
        formatter: function(params) {
          const p = params[0];
          if (!p) return "";
          const data = p.data;
          return `<strong>${esc(data.place)}</strong><br/>Magn.: <strong>${data.mag}</strong><br/>Prof.: ${data.depth} km<br/>Hora: ${data.time}`;
        },
      },
      grid: {
        left: "5%",
        right: "5%",
        bottom: "15%",
        top: "15%",
        containLabel: true,
      },
      xAxis: {
        type: "category",
        data: [],
        axisLabel: { rotate: 30, fontSize: 10, interval: 0 },
        name: "Hora (UTC-5)",
        nameLocation: "middle",
        nameGap: 35,
      },
      yAxis: {
        type: "value",
        name: "Magnitud (Mw)",
        min: 0,
        max: 8,
        splitLine: { lineStyle: { type: "dashed", color: "#ccc" } },
      },
      series: [
        {
          name: "Sismos",
          type: "bar",
          data: [],
          itemStyle: {
            color: function(params) {
              const val = params.value;
              if (val >= 6) return "#d63031";
              if (val >= 5) return "#e17055";
              if (val >= 4) return "#fdcb6e";
              return "#74b9ff";
            },
          },
          barWidth: "50%",
          label: {
            show: true,
            position: "top",
            formatter: (p) => p.value.toFixed(1),
            fontSize: 10,
          },
        },
      ],
    };
    sismosChart.setOption(option);
    window.addEventListener(
      "resize",
      () => sismosChart && sismosChart.resize(),
    );
  }
  function updateChart() {
    if (!sismosChart) return;
    const filtered = applyFilters();
    const xData = filtered.map((q) =>
      new Date(q.timestamp).toLocaleTimeString("es-CO", {
        hour: "2-digit",
        minute: "2-digit",
        timeZone: "America/Bogota",
      }),
    );
    const seriesData = filtered.map((q) => ({
      value: q.mag,
      place: q.place || "N/A",
      time: fullTime(q.timestamp),
      depth: q.depth,
      mag: q.mag,
    }));
    sismosChart.setOption({
      xAxis: { data: xData },
      series: [{ data: seriesData }],
    });
  }
  // ================================================================
  //  RENDER: alerta, lista, contador
  // ================================================================
  function renderAlert() {
    const last = sismosData.length
      ? sismosData.reduce((a, b) => (b.timestamp > a.timestamp ? b : a))
      : null;
    const alerta = document.getElementById("sismoAlert");
    if (!alerta) return;
    // Check if this latest sismo is already shown in "En vivo" (liveData)
    const liveLatest = liveData.length > 0 ? liveData[0] : null;
    if (!last || (liveLatest && last.timestamp === liveLatest.timestamp && last.place === liveLatest.place)) {
      // Hide the alert card: no data, or already shown in En vivo
      alerta.style.display = "none";
      return;
    }
    // Show the alert card - unique latest sismo not in En vivo
    alerta.style.display = "";
    const mag = last.mag || 0;
    document.getElementById("ultimoLugar").textContent =
      last.place || "Lugar desconocido";
    document.getElementById("ultimoMag").textContent = mag.toFixed(1);
    document.getElementById("ultimoHora").textContent = fullTime(
      last.timestamp,
    );
    document.getElementById("ultimoRel").textContent = relTime(last.timestamp);
    const color = sevColor(mag);
    alerta.className = "alerta" + (mag >= 5 ? " peligro" : "");
    alerta.style.borderLeftColor = color;
    document.getElementById("ultimoMag").style.background = color;
    document.getElementById("ultimoMag").style.color =
      mag >= 5 ? "#fff" : "#0f1115";
  }
  function renderSismosList() {
    const listEl = document.getElementById("sismosList");
    if (!listEl) return;
    const filtered = applyFilters();
    const total = document.getElementById("totalSismos");
    if (total) total.textContent = `${filtered.length} sismos`;
    if (!filtered.length) {
      listEl.innerHTML =
        '<div style="color:#999;padding:10px;">Sin sismos que coincidan con los filtros.</div>';
      return;
    }
    // Get the latest earthquake from live data (last 72h) to exclude from history
    const liveLatest = liveData.length > 0 ? liveData[0] : null;
    
    listEl.innerHTML = filtered
      .filter((q) => {
        // Exclude the latest sismo if it's already shown in "En vivo" (last 72h)
        if (liveLatest && q.timestamp === liveLatest.timestamp && q.place === liveLatest.place) {
          return false;
        }
        return true;
      })
      .map((q) => {
        const mag = q.mag || 0;
        const depth = q.depth != null ? q.depth.toFixed(1) : null;
        const url = q.url || "";
        return `
          <div class="sismo-row ${sevClass(mag)}">
            <div class="sismo-top">
              <span class="place">${esc(q.place) || "Lugar desconocido"}</span>
              <span class="rel">${esc(relTime(q.timestamp))}</span>
            </div>
            <div class="sismo-bottom">
              <span class="mag ${magClass(mag)}">${mag.toFixed(1)}</span>
              <span class="meta">${esc(fullTime(q.timestamp))}</span>
              <span class="depth-pill">${esc(depthLabel(q.depth))}${depth ? ` · ${esc(depth)} km` : ""}</span>
              ${url ? `<a style="margin-left:auto;"href="${esc(url)}" target="_blank" rel="noopener">Detalle ↗</a>` : ""}
            </div>
          </div>`;
      })
      .join("");
  }
  function renderLiveStrip() {
    const listEl = document.getElementById("sismoLiveList");
    const updEl = document.getElementById("sismoLiveUpdated");
    if (!listEl) return;
    if (updEl) {
      updEl.textContent = new Date().toLocaleTimeString("es-CO", {
        hour: "2-digit",
        minute: "2-digit",
        second: "2-digit",
        timeZone: "America/Bogota",
      });
    }
    if (!liveData.length) {
      if (!liveFetchAttempted) {
        listEl.innerHTML =
          '<div class="sismo-live-empty" style="color:#999;">Cargando sismos en vivo...</div>';
      } else {
        listEl.innerHTML =
          '<div class="sismo-live-empty">Sin sismos en las últimas 72 h.</div>';
      }
      return;
    }
    listEl.innerHTML = liveData
      .map((q) => {
        const mag = q.mag || 0;
        const depth = q.depth != null ? `${q.depth.toFixed(0)} km` : "";
        return `
          <div class="sismo-live-item ${sevClass(mag)}">
            <span class="mag ${magClass(mag)}">${mag.toFixed(1)}</span>
            <span class="place">${esc(q.place) || "Lugar desconocido"}</span>
            ${depth ? `<span class="depth">${esc(depth)}</span>` : ""}
            <span class="rel">${esc(relTime(q.timestamp))}</span>
          </div>`;
      })
      .join("");
  }
  function updateSismoUI(data) {
    sismosData = data.earthquakes || [];
    renderAlert();
    renderSismosList();
    updateChart();
    // Notify map.js so sismo markers refresh on the map.
    if (typeof window.onSismosUpdate === "function") {
      window.onSismosUpdate();
    }
  }
  // ================================================================
  //  FETCH
  // ================================================================
  async function fetchEarthquakes() {
    try {
      const apiBase = window.API_BASE || "/api";
      const res = await fetch(
        `${apiBase}/earthquakes?minmagnitude=3&limit=500&days=365`,
      );
      if (!res.ok) throw new Error("Error en API");
      const data = await res.json();
      updateSismoUI(data);
    } catch (err) {
      console.error("Error fetching sismos:", err);
      document.getElementById("ultimoLugar").textContent = "⚠️ Error al cargar";
      document.getElementById("sismoAlert").style.borderColor = "#d63031";
      const listEl = document.getElementById("sismosList");
      if (listEl) {
        listEl.innerHTML =
          '<div style="color:#ff6b6b;padding:10px;">⚠️ No se pudieron cargar los sismos. Revisa la conexión o el endpoint /api/earthquakes.</div>';
      }
    }
  }
  // Feed "en vivo": ventana corta (24 h), solo para la franja superior.
  // No toca sismosData (el historial) para no inundar la lista principal.
  async function fetchLive() {
    liveFetchAttempted = true;
    try {
      const apiBase = window.API_BASE || "/api";
      const res = await fetch(
        `${apiBase}/earthquakes?minmagnitude=0&limit=50&hours=72`,
      );
      if (!res.ok) throw new Error("Error en API live");
      const data = await res.json();
      liveData = (data.earthquakes || []).sort(
        (a, b) => b.timestamp - a.timestamp,
      );
      renderLiveStrip();
      // Re-render alert in case the latest sismo is now in live data
      renderAlert();
    } catch (err) {
      console.error("Error fetching sismos en vivo:", err);
      const listEl = document.getElementById("sismoLiveList");
      if (listEl) {
        listEl.innerHTML =
          '<div class="sismo-live-empty" style="color:#ff6b6b;">⚠️ No se pudo actualizar el feed en vivo.</div>';
      }
    }
  }
  // ================================================================
  //  WIRE FILTERS
  // ================================================================
  function wireFilters() {
    const magSel = document.getElementById("sismoMagFilter");
    if (magSel && !magSel.dataset.wired) {
      magSel.dataset.wired = "1";
      magSel.addEventListener("change", () => {
        filters.minMag = parseInt(magSel.value, 10) || 0;
        renderSismosList();
        updateChart();
      });
    }
    const depthSel = document.getElementById("sismoDepthFilter");
    if (depthSel && !depthSel.dataset.wired) {
      depthSel.dataset.wired = "1";
      depthSel.addEventListener("change", () => {
        filters.depth = depthSel.value;
        renderSismosList();
        updateChart();
      });
    }
    const dateSel = document.getElementById("sismoDateFilter");
    if (dateSel && !dateSel.dataset.wired) {
      dateSel.dataset.wired = "1";
      dateSel.addEventListener("change", () => {
        filters.days = parseInt(dateSel.value, 10) || 0;
        renderSismosList();
        updateChart();
      });
    }
  }
  // ================================================================
  //  PUBLIC API
  // ================================================================
  window.initSismos = function() {
    if (!window.sismoChart) {
      initSismoChart();
    }
    wireFilters();
    // Show initial loading state for En vivo
    renderLiveStrip();
    // Historial: carga completa una sola vez (no se refetchea cada 10s).
    if (!sismosData.length) fetchEarthquakes();
    // Feed en vivo: primera carga + poll cada 60s, sin tocar el historial.
    if (!liveWired) {
      liveWired = true;
      fetchLive();
      window.sismoLiveInterval = setInterval(fetchLive, 60000);
    }
  };
  // Expose the latest earthquake data so map.js can plot sismos on the map.
  window.getSismosData = function() {
    return sismosData;
  };
  // Let map.js trigger a fetch when it needs sismo data (e.g. first map visit).
  window.fetchSismos = function() {
    if (!sismosData.length) fetchEarthquakes();
  };
  // Allow the main search bar to set the location filter for sismos
  window.setSismosLocationFilter = function(value) {
    filters.location = value;
    renderSismosList();
    updateChart();
  };
})();
//===== public/map.js =====
// ================================================================
//  MAPA - Leaflet + Marcadores + Heatmap
// ================================================================
let cityMarkersLayer = null;
let mapMarkersLayer = null;
let heatmapLayer = null;
let sismoMarkersLayer = null;
// Search query applied to map markers/sidebar (driven by the shared search bar).
let mapSearchQuery = "";
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
const MAP_SIDEBAR_EMOJI = { person: "🫂", pet: "🐕", building: "🏢" };
// Which marker types are currently visible on the map. Empty set = all.
const mapTypeFilter = new Set(["person", "pet", "building", "sismo"]);
function isTypeVisible(type) {
  return mapTypeFilter.has(type);
}
// Flatten all visible map item types into a single [{ item, type }] list.
// Used by the sidebar and off-screen arrows (persons + pets).
function visibleMapItems() {
  return Object.entries(MAP_MARKER_META)
    .filter(([type]) => isTypeVisible(type))
    .flatMap(([type, meta]) => meta.data().map((item) => ({ item, type })))
    .filter(({ item, type }) => mapItemMatchesSearch(item, type));
}
// Apply the shared search-bar query to a map item. Matches name/location/city
// for persons/pets/buildings, and place for sismos.
function mapItemMatchesSearch(item, type) {
  if (!mapSearchQuery.trim()) return true;
  const q = mapSearchQuery;
  if (type === "sismo") {
    return fuzzyMatch(q, item.place || "");
  }
  return (
    fuzzyMatch(q, item.name || "") ||
    fuzzyMatch(q, item.location || "") ||
    fuzzyMatch(q, item.city || "")
  );
}
// Marker circle color is driven by the item's status, not its type.
// Single source of truth for status semantics (color, CSS class, label).
// Exposed on window so index.js can also use it.
const STATUS_META = {
  desaparecido: {
    color: "#e57373",
    cssClass: "desaparecido",
    label: "desaparecido",
    icon: "🔍",
  },
  encontrado: {
    color: "#66bb6a",
    cssClass: "encontrado",
    label: "encontrado",
    icon: "✅",
  },
  seguro: { color: "#66bb6a", cssClass: "seguro", label: "seguro", icon: "🫶" },
  danado: { color: "#f0c060", cssClass: "danado", label: "dañado", icon: "⚠️" },
  colapsado: {
    color: "#d06060",
    cssClass: "colapsado",
    label: "colapsado",
    icon: "💥",
  },
  acopio: {
    color: "#ffffff",
    cssClass: "acopio",
    label: "📦 acopio",
    icon: "📦",
  },
  angel: { color: "#add8e6", cssClass: "angel", label: "👼", icon: "👼" },
  colaborador: {
    color: "#9b59b6",
    cssClass: "colaborador",
    label: "colaborador",
    icon: "🤝",
  },
};
window.STATUS_META = STATUS_META;
function statusColor(status) {
  return STATUS_META[status]?.color || "#e57373";
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
  // Buildings use their own placeholder and default status ("seguro"),
  // not the person placeholder / "desaparecido" fallback.
  const placeholder =
    type === "pet"
      ? PET_PLACEHOLDER
      : type === "building"
        ? BUILDING_PLACEHOLDER
        : PERSON_PLACEHOLDER;
  const imgHtml = isAngel
    ? `<img class="map-sidebar-photo" style="opacity:0.62;" src="angel.png" alt="" />`
    : photo
      ? `<img class="map-sidebar-photo" src="${escapeHtml(photo)}" alt="" />`
      : `<img class="map-sidebar-photo" style="opacity:0.62;" src="${placeholder}" alt="" />`;
  const location = item.city || item.location || "Sin ubicación";
  const emoji = isAngel ? "👼" : MAP_SIDEBAR_EMOJI[type];
  const defaultStatus = type === "building" ? "seguro" : "desaparecido";
  const statusClass = isAngel ? "angel" : item.status || defaultStatus;
  // Expose the status color so the card background/border are derived
  // automatically (color-mix), matching the main list cards.
  const statusColorHex = statusColor(statusClass);
  return `
    <div class="map-sidebar-card ${statusClass}" data-id="${item.id}" data-type="${type}" style="--status-color:${statusColorHex};">
      ${imgHtml}
      <div class="map-sidebar-info">
        <span class="map-sidebar-name">${emoji} ${escapeHtml(item.name)}</span>
        <span class="map-sidebar-meta">${escapeHtml(location)}</span>
      </div>
      <span style="display:none"class="status-tag ${statusClass}">${item.status}</span>
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
  // Reuse the cluster group across re-renders: constructing a new
  // L.markerClusterGroup is expensive (internal event wiring + cluster
  // recomputation), so we build it once and just clearLayers() + re-add.
  if (!mapMarkersLayer) {
    mapMarkersLayer = L.markerClusterGroup({
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
    window.sismoMap.addLayer(mapMarkersLayer);
  }
  const clusterGroup = mapMarkersLayer;
  clusterGroup.clearLayers();
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
      if (!mapItemMatchesSearch(item, type)) return;
      const [lat, lng] = resolveItemCoords(item);
      if (lat == null || lng == null) return;
      const isAngel = item.status === "angel";
      const markerEmoji = isAngel ? "👼" : emoji;
      const color = statusColor(item.status);
      const sIcon = statusIcon(item.status);
      // Show the status icon alongside the type emoji (e.g. 📦 + 🏢).
      const markerContent = isAngel
        ? `<span>👼</span>`
        : `<span>${sIcon}</span><span>${emoji}</span>`;
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
                flex-wrap:nowrap;
                align-items:center;
                justify-content:center;
                gap:1px;
                font-size:13px;
                line-height:1;
                white-space:nowrap;
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
  renderSismoMarkers();
}
// Plot earthquakes (sismos) on the map as magnitude-colored circles.
function renderSismoMarkers() {
  if (!window.sismoMap) return;
  if (!sismoMarkersLayer) {
    sismoMarkersLayer = L.layerGroup().addTo(window.sismoMap);
  }
  sismoMarkersLayer.clearLayers();
  if (!isTypeVisible("sismo")) return;
  // Ensure sismo data is loaded (fetch on first map visit if empty).
  if (typeof window.fetchSismos === "function") window.fetchSismos();
  const sismos =
    (typeof window.getSismosData === "function"
      ? window.getSismosData()
      : []) || [];
  sismos.forEach((q) => {
    if (q.lat == null || q.lng == null) return;
    if (!mapItemMatchesSearch(q, "sismo")) return;
    const mag = q.mag || 0;
    const color =
      mag >= 6
        ? "#d63031"
        : mag >= 5
          ? "#e17055"
          : mag >= 4
            ? "#fdcb6e"
            : "#74b9ff";
    const size = Math.max(16, Math.min(34, 12 + mag * 3));
    L.circleMarker([q.lat, q.lng], {
      radius: size / 2,
      color: "#fff",
      weight: 1,
      fillColor: color,
      fillOpacity: 0.85,
    }).addTo(sismoMarkersLayer).bindPopup(`
        🌋 <strong>${escapeHtml(q.place || "Sismo")}</strong><br>
        Magnitud: <strong>${mag.toFixed(1)}</strong><br>
        Profundidad: ${q.depth != null ? q.depth.toFixed(1) + " km" : "—"}<br>
        ${q.url ? `<a href="${escapeHtml(q.url)}" target="_blank" rel="noopener">Detalle ↗</a>` : ""}
      `);
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
  // Apply the heatmap toggle on first init (the pill is active by default,
  // but toggleHeatmap was never called on load, so the heatmap never
  // actually rendered until the user toggled it). Respects the restored
  // state from setMapFilterState().
  const heatmapEl = document.getElementById("heatmapToggle");
  if (heatmapEl && heatmapEl.classList.contains("active")) toggleHeatmap(true);
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
        map.flyTo([lat, lng], 17, { duration: 0.3 });
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
// Set the shared search-bar query for the map and re-render. Called by
// index.js when the user types in the search bar while on the map tab.
// Debounced so a burst of keystrokes coalesces into one re-render.
window.setMapSearchQuery = function (q) {
  mapSearchQuery = q || "";
  if (!window.sismoMap) return;
  if (mapSearchDebounce) clearTimeout(mapSearchDebounce);
  mapSearchDebounce = setTimeout(() => {
    mapSearchDebounce = null;
    renderMapMarkers();
    renderMapSidebar();
    renderOffscreenArrows();
  }, 150);
};
// Re-render sismo markers when new earthquake data arrives.
window.onSismosUpdate = function () {
  if (!window.sismoMap) return;
  renderSismoMarkers();
};
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
    // leaflet.heat only redraws on 'zoomend'/'moveend', so during the zoom
    // animation the canvas lags. A 'zoom' handler (fires only during zoom,
    // not pan) with _reset() keeps it in sync. rAF throttling coalesces the
    // burst of events so we don't double-reposition and cause a visible jump.
    // Listening to 'zoom' instead of 'move' avoids a full heatmap redraw on
    // every pan frame, which was the main source of sluggishness.
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
      window.sismoMap.on("zoom", window.__heatmapMoveHandler);
    }
  } else if (heatmapLayer) {
    window.sismoMap.removeLayer(heatmapLayer);
  }
  // Persist the heatmap toggle.
  if (typeof saveFilters === "function") saveFilters();
}
// Read the current map filter state (type filter + heatmap) for persistence.
window.getMapFilterState = function () {
  const heatmapEl = document.getElementById("heatmapToggle");
  return {
    types: [...mapTypeFilter],
    heatmap: heatmapEl ? heatmapEl.classList.contains("active") : true,
  };
};
// Restore map filter state (type filter + heatmap) from persisted data.
// Called by index.js on startup, before the map is first initialized.
window.setMapFilterState = function (state) {
  if (!state) return;
  // Type filter: rebuild the Set from the saved array.
  if (Array.isArray(state.types)) {
    mapTypeFilter.clear();
    state.types.forEach((t) => mapTypeFilter.add(t));
    // Reflect into the pill UI.
    const container = document.getElementById("mapTypeFilter");
    if (container) {
      container
        .querySelectorAll(".filter-pill[data-map-type]")
        .forEach((pill) => {
          pill.classList.toggle(
            "active",
            mapTypeFilter.has(pill.dataset.mapType),
          );
        });
    }
  }
  // Heatmap toggle.
  if (typeof state.heatmap === "boolean") {
    const heatmapEl = document.getElementById("heatmapToggle");
    if (heatmapEl) heatmapEl.classList.toggle("active", state.heatmap);
  }
};
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
      // Persist the map type filter.
      if (typeof saveFilters === "function") saveFilters();
    });
  });
  // Heatmap toggle: same pill styling, toggles the heatmap layer.
  const heatmapBtn = document.getElementById("heatmapToggle");
  if (heatmapBtn) {
    heatmapBtn.addEventListener("click", () => {
      const enabled = !heatmapBtn.classList.contains("active");
      heatmapBtn.classList.toggle("active", enabled);
      toggleHeatmap(enabled);
    });
  }
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
        window.sismoMap.flyTo([lat, lng], 17, { duration: 0.3 });
      }
    });
  }
});
//===== public/vStore.js =====
//===== public/modal.js =====
// modal.js — reusable modal shell.
// Loaded before map.js and index.js. Defines window.Modal, a factory
// that wraps a .modal element and provides open/close with focus
// management, scroll-lock, backdrop-click, Escape, and a basic Tab trap.
// Slots (header/body/footer) are opt-in via data-modal-* attributes;
// they are null until the HTML is refactored in later stages.
(function () {
  "use strict";
  // Stack of all created instances; used for Escape dispatch and
  // scroll-lock refcounting.
  const instances = [];
  let escapeWired = false;
  function wireEscape() {
    if (escapeWired) return;
    escapeWired = true;
    document.addEventListener("keydown", function (e) {
      if (e.key !== "Escape") return;
      // Close the topmost open modal (most recently pushed).
      for (let i = instances.length - 1; i >= 0; i--) {
        if (instances[i].isOpen()) {
          instances[i].close();
          break;
        }
      }
    });
  }
  function lockScroll() {
    document.body.style.overflow = "hidden";
  }
  function unlockScroll() {
    // Only release the scroll lock when no modal remains open.
    if (!instances.some(function (m) { return m.isOpen(); })) {
      document.body.style.overflow = "";
    }
  }
  function focusableEls(root) {
    return root.querySelectorAll(
      'button:not([disabled]), [href], input, select, textarea, ' +
      '[tabindex]:not([tabindex="-1"])'
    );
  }
  /**
   * @param {Object} opts
   * @param {string} opts.id            — element id of the .modal wrapper
   * @param {Function} [opts.onOpen]   — called after opening
   * @param {Function} [opts.onClose]   — called after closing
   * @returns {{el, header, body, footer, open, close, isOpen}|null}
   */
  function Modal(opts) {
    var el = document.getElementById(opts.id);
    if (!el) {
      console.warn("Modal: #" + opts.id + " not found");
      return null;
    }
    var header = el.querySelector("[data-modal-header]");
    var body = el.querySelector("[data-modal-body]");
    var footer = el.querySelector("[data-modal-footer]");
    var closeBtn = el.querySelector("[data-modal-close]");
    var onOpen = opts.onOpen || null;
    var onClose = opts.onClose || null;
    var previousFocus = null;
    wireEscape();
    var api = {
      el: el,
      header: header,
      body: body,
      footer: footer,
      previousFocus: null,
      isOpen: isOpen,
      open: open,
      close: close
    };
    instances.push(api);
    function isOpen() {
      return el.classList.contains("open");
    }
    function open() {
      previousFocus = document.activeElement;
      el.classList.add("open");
      lockScroll();
      // Move focus into the modal for keyboard users.
      var f = focusableEls(el);
      if (f.length) f[0].focus();
      if (onOpen) onOpen();
    }
    function close() {
      if (!isOpen()) return;
      el.classList.remove("open");
      unlockScroll();
      if (previousFocus && typeof previousFocus.focus === "function") {
        previousFocus.focus();
        previousFocus = null;
      }
      if (onClose) onClose();
    }
    // Backdrop click: close only when the click lands on the wrapper
    // itself (not an inner element).
    el.addEventListener("click", function (e) {
      if (e.target === el) close();
    });
    // Explicit close button.
    if (closeBtn) closeBtn.addEventListener("click", close);
    // Basic focus trap: keep Tab cycling inside the modal while open.
    el.addEventListener("keydown", function (e) {
      if (e.key !== "Tab" || !isOpen()) return;
      var f = focusableEls(el);
      if (!f.length) return;
      var first = f[0];
      var last = f[f.length - 1];
      if (e.shiftKey && document.activeElement === first) {
        e.preventDefault();
        last.focus();
      } else if (!e.shiftKey && document.activeElement === last) {
        e.preventDefault();
        first.focus();
      }
    });
    return api;
  }
  window.Modal = Modal;
})();
//===== public/colombia_earthquake_missing_platforms.json =====
{
  "event": "Colombia earthquake - August 2026",
  "epicenter": "San José del Palmar, Chocó",
  "magnitude": 7.4,
  "date": "2026-08-10",
  "last_updated": "2026-08-12",
  "platforms": [
    {
      "name": "Colombia Te Busca",
      "url": "https://colombiatebusca.com/",
      "type": "citizen-run",
      "description": "Main citizen-run hub (~3,300-3,900 reports). Search by name, ID, code, location; includes 'Terremoto' category. Voluntary, non-profit.",
      "notes": "Does not replace authorities, judicial bodies, rescue agencies, or official emergency lines."
    },
    {
      "name": "SismoInfo.co",
      "url": "http://sismoinfo.co",
      "type": "citizen-run",
      "description": "Citizen app with map; sections for people, pets, places, announcements, collaborators.",
      "notes": "Links to colombiatebusca.com. Admin contact: +57 3153410282, IG: @neokpm"
    },
    {
      "name": "Asocapitales tool",
      "url": "https://www.asocapitales.co/",
      "type": "official",
      "description": "Emergency digital tool by Asociación Colombiana de Ciudades Capitales. Integrated with Colombia Te Busca; indicators for registered/found/to-be-located.",
      "notes": "Launched 2026-08-11."
    },
    {
      "name": "Cruz Roja Colombiana",
      "url": "https://www.cruzrojacolombiana.org/",
      "type": "official",
      "description": "Official humanitarian channel with WhatsApp lines for missing persons reports.",
      "phone": "132"
    },
    {
      "name": "Registro Nacional de Desaparecidos (Medicina Legal)",
      "url": "https://www.medicinalegal.gov.co/",
      "type": "official",
      "description": "Official national registry of missing persons maintained by Instituto Nacional de Medicina Legal y Ciencias Forenses."
    },
    {
      "name": "Desaparecidos Terremoto Venezuela",
      "url": "https://desaparecidosterremotovenezuela.com/",
      "type": "citizen-run",
      "description": "Venezuelan citizen platform (same model as Colombia Te Busca). Relevant for cross-border missing persons.",
      "notes": "Created by volunteers; donations fund the initiative's operations."
    }
  ],
  "emergency_phone_lines": [
    {
      "service": "Policía Nacional",
      "number": "112"
    },
    {
      "service": "Cruz Roja Colombiana",
      "number": "132"
    },
    {
      "service": "Defensa Civil",
      "number": "144"
    },
    {
      "service": "Bomberos",
      "number": "119"
    },
    {
      "service": "General emergencies",
      "number": "123"
    }
  ],
  "authoritative_sources": [
    "UNGRD (Unidad Nacional para la Gestión del Riesgo de Desastres)",
    "Cruz Roja Colombiana"
  ],
  "disclaimer": "Situation is evolving rapidly; figures change hourly. Citizen sites are the most complete lists; official sources (UNGRD, Cruz Roja) are authoritative for confirmed data."
}
//===== server.js =====
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
// Static files AFTER all API routes so /api/* is never intercepted
app.use(express.static("public"));
app.listen(PORT, () => {
  console.log(`sismo-api listening on :${PORT}`);
});

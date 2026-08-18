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

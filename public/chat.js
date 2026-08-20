//===== public/chat.js =====
// ================================================================
//  CHAT MATRIX (E2E) & MEDIA LIGHTBOX
//  Extracted from index.js to keep the main file clean.
//  Assumes global access to: escapeHtml, Modal, API_BASE, authFetch,
//  window.currentUser, updateTabCounts, loadListColab, etc.
// ================================================================

const MATRIX_ROOM_ALIAS = "#ayuda-en-cali:matrix.sismoinfo.co";

// Deterministic color from user ID
function getUserColor(userId) {
  let hash = 0;
  for (let i = 0; i < userId.length; i++) {
    hash = (hash << 5) - hash + userId.charCodeAt(i);
    hash |= 0;
  }
  const hue = Math.abs(hash) % 360;
  return `hsl(${hue}, 70%, 75%)`;
}

// Format seconds into mm:ss
function formatDuration(seconds) {
  if (!seconds || isNaN(seconds)) return "";
  const m = Math.floor(seconds / 60);
  const s = Math.floor(seconds % 60);
  return `${m}:${String(s).padStart(2, "0")}`;
}

// ================================================================
//  CHAT VIRTUALIZER (Bidirectional DOM Windowing)
// ================================================================
class ChatVirtualList {
  constructor() {
    this.container = document.getElementById("chatMessages");
    this.wrapper = document.getElementById("chatContentWrapper");
    this.topSpacer = document.getElementById("chatTopSpacer");
    this.bottomSpacer = document.getElementById("chatBottomSpacer");

    this.events = [];
    this.heights = new Map();
    this.nodes = new Map();

    this.DEFAULT_HEIGHT = 80;
    this.OVERSCAN = 8;
    this.isAtBottom = true;
    this._rafId = null;
    this._rafAnchor = null;
    this._stickRaf = null;
    this._preserveAnchorId = null;
    this._preserveAnchorIndex = null;
    this._pendingScrollTo = null;
    this._isDestroyed = false;

    // ResizeObserver: sticky bottom when pinned, compensate when scrolled up
    this._resizeObserver = new ResizeObserver((entries) => {
      if (this._isDestroyed) return;
      let changed = false;
      let scrollAdjustment = 0;
      const containerTop = this.container.getBoundingClientRect().top;

      for (let entry of entries) {
        const id = entry.target.dataset.eventId;
        const newH = entry.target.offsetHeight;
        const oldH = this.heights.get(id) || this.DEFAULT_HEIGHT;

        if (oldH !== newH) {
          this.heights.set(id, newH);
          changed = true;

          if (!this.isAtBottom) {
            const nodeTop = entry.target.getBoundingClientRect().top;
            if (nodeTop <= containerTop + 5) {
              scrollAdjustment += newH - oldH;
            }
          }
        }
      }

      if (changed) {
        if (this.isAtBottom) {
          this._stickToBottom();
        } else if (scrollAdjustment !== 0) {
          this.container.scrollTop += scrollAdjustment;
        }
        this._scheduleRender();
      }
    });

    this._onScroll = this._onScroll.bind(this);
    this.container.addEventListener("scroll", this._onScroll, {
      passive: true,
    });
  }

  destroy() {
    this._isDestroyed = true;
    this.container.removeEventListener("scroll", this._onScroll);
    this._resizeObserver.disconnect();
    this.nodes.forEach((node) => node.remove());
    this.nodes.clear();
    this.heights.clear();
    this.events = [];
    if (this._rafAnchor) cancelAnimationFrame(this._rafAnchor);
    if (this._rafId) cancelAnimationFrame(this._rafId);
    if (this._stickRaf) cancelAnimationFrame(this._stickRaf);
  }

  _isEventAlreadyPresent(eventId) {
    return this.events.some((ev) => {
      if (ev.getId() === eventId) return true;
      if (ev._isMediaGroup) {
        return ev.items.some((i) => i.getId() === eventId);
      }
      return false;
    });
  }

  setEvents(events) {
    if (this._isDestroyed) return;
    this.events = this._groupEvents(events);
    this._render(true);
    this.scrollToBottom();
  }

  prependEvents(newEvents) {
    if (this._isDestroyed || !newEvents.length) return;

    const uniqueNewEvents = newEvents.filter(
      (ev) => !this._isEventAlreadyPresent(ev.getId()),
    );
    if (!uniqueNewEvents.length) return;

    const groupedNew = this._groupEvents(uniqueNewEvents);
    if (!groupedNew.length) return;

    if (this._rafAnchor) cancelAnimationFrame(this._rafAnchor);

    const anchorNode = this.wrapper.firstElementChild;
    const anchorId = anchorNode ? anchorNode.dataset.eventId : null;
    const anchorTop = anchorNode ? anchorNode.offsetTop : 0;
    const prevScrollTop = this.container.scrollTop;

    let anchorIndexInOld = -1;
    if (anchorId) {
      anchorIndexInOld = this.events.findIndex((e) => e.getId() === anchorId);
    }

    this._preserveAnchorId = anchorId;
    this._preserveAnchorIndex =
      anchorIndexInOld >= 0 ? anchorIndexInOld + groupedNew.length : null;

    this.events = [...groupedNew, ...this.events];
    this._render(true);

    if (anchorId && anchorIndexInOld >= 0) {
      const newAnchorNode = this.nodes.get(anchorId);
      if (newAnchorNode) {
        const shift = newAnchorNode.offsetTop - anchorTop;
        this.container.scrollTop = prevScrollTop + shift;
      }
    }

    this._preserveAnchorId = null;
    this._preserveAnchorIndex = null;
    this._scheduleRender();

    // Check if a pending scroll target arrived
    if (this._pendingScrollTo) {
      const idx = this.events.findIndex(
        (e) => e.getId() === this._pendingScrollTo,
      );
      if (idx >= 0) {
        clearTimeout(this._pendingScrollTimeout);
        this._pendingScrollTo = null;
        this._doScrollToIndex(idx);
      } else {
        // Check inside media groups
        for (let i = 0; i < this.events.length; i++) {
          const ev = this.events[i];
          if (ev._isMediaGroup) {
            const itemIdx = ev.items.findIndex(
              (item) => item.getId() === this._pendingScrollTo,
            );
            if (itemIdx >= 0) {
              clearTimeout(this._pendingScrollTimeout);
              this._pendingScrollTo = null;
              this._doScrollToIndex(i);
              break;
            }
          }
        }
      }
    }
  }

  appendEvent(event) {
    if (this._isDestroyed) return;
    if (this._isEventAlreadyPresent(event.getId())) return;

    const content = event.getContent();
    const msgtype = content.msgtype;
    const isMedia = msgtype === "m.image" || msgtype === "m.video";
    const sender = event.getSender();
    const ts = event.getTs();

    if (isMedia && this.events.length > 0) {
      const last = this.events[this.events.length - 1];
      if (
        last._isMediaGroup &&
        last.sender === sender &&
        ts - last.getTs() < 5 * 60 * 1000
      ) {
        if (!last.items.some((i) => i.getId() === event.getId())) {
          last.items.push(event);
          const groupId = last.getId();
          this.nodes.delete(groupId);
          this.heights.delete(groupId);
          this._scheduleRender();
          if (this.isAtBottom) this._stickToBottom();
        }
        return;
      }
    }

    if (isMedia) {
      const newGroup = {
        _isMediaGroup: true,
        sender,
        ts,
        items: [event],
        getId: () => event.getId(),
        getType: () => "m.room.message",
        getContent: () => content,
        getSender: () => sender,
        getTs: () => ts,
      };
      this.events.push(newGroup);
    } else {
      this.events.push(event);
    }

    this._scheduleRender();
    if (this.isAtBottom) this._stickToBottom();
  }

  scrollToBottom() {
    this.isAtBottom = true;
    this._stickToBottom();
  }

  _stickToBottom() {
    if (this._isDestroyed || !this.isAtBottom) return;
    if (this._stickRaf) cancelAnimationFrame(this._stickRaf);
    this._stickRaf = requestAnimationFrame(() => {
      if (this._isDestroyed) return;
      this.container.scrollTop = this.container.scrollHeight;
      this._stickRaf = null;
    });
  }

  scrollToEvent(eventId) {
    if (this._isDestroyed) return;

    // Check top-level events
    let index = this.events.findIndex((e) => e.getId() === eventId);
    if (index >= 0) {
      this._doScrollToIndex(index);
      return;
    }

    // Check inside media groups
    for (let i = 0; i < this.events.length; i++) {
      const ev = this.events[i];
      if (ev._isMediaGroup) {
        const itemIdx = ev.items.findIndex((item) => item.getId() === eventId);
        if (itemIdx >= 0) {
          this._doScrollToIndex(i);
          return;
        }
      }
    }

    // Event not loaded yet — store as pending and trigger loading
    this._pendingScrollTo = eventId;
    clearTimeout(this._pendingScrollTimeout);
    this._pendingScrollTimeout = setTimeout(() => {
      this._pendingScrollTo = null;
    }, 10000);

    // Scroll to top to trigger loadOlderMessages
    this.container.scrollTop = 0;
  }

  _doScrollToIndex(index) {
    let top = 0;
    for (let i = 0; i < index; i++) {
      top += this.heights.get(this.events[i].getId()) || this.DEFAULT_HEIGHT;
    }
    this.container.scrollTop = top;

    const node = this.nodes.get(this.events[index].getId());
    if (node) {
      node.style.outline = "2px solid #3fa34d";
      setTimeout(() => {
        node.style.outline = "";
      }, 1500);
    }
  }

  _onScroll() {
    if (this._isDestroyed) return;
    if (this._stickRaf) {
      cancelAnimationFrame(this._stickRaf);
      this._stickRaf = null;
    }
    const threshold = 50;
    this.isAtBottom =
      this.container.scrollHeight -
      this.container.scrollTop -
      this.container.clientHeight <
      threshold;
    if (this.container.scrollTop === 0 && !window.loadingOlder) {
      window.loadOlderMessages();
    }
    this._scheduleRender();
  }

  _scheduleRender() {
    if (this._rafId || this._isDestroyed) return;
    this._rafId = requestAnimationFrame(() => {
      this._rafId = null;
      this._render(false);
    });
  }

  _groupEvents(events) {
    const grouped = [];
    let currentGroup = null;

    const flush = () => {
      if (currentGroup) {
        grouped.push(currentGroup);
        currentGroup = null;
      }
    };

    for (const ev of events) {
      if (ev.getType() !== "m.room.message") {
        flush();
        grouped.push(ev);
        continue;
      }
      const content = ev.getContent();
      const msgtype = content.msgtype;
      if (msgtype !== "m.image" && msgtype !== "m.video") {
        flush();
        grouped.push(ev);
        continue;
      }

      const sender = ev.getSender();
      const ts = ev.getTs();

      if (currentGroup) {
        const lastTs =
          currentGroup.items[currentGroup.items.length - 1].getTs();
        if (sender === currentGroup.sender && ts - lastTs < 5 * 60 * 1000) {
          currentGroup.items.push(ev);
          continue;
        } else {
          flush();
        }
      }

      currentGroup = {
        _isMediaGroup: true,
        sender,
        ts,
        items: [ev],
        getId: () => ev.getId(),
        getType: () => "m.room.message",
        getContent: () => content,
        getSender: () => sender,
        getTs: () => ts,
      };
    }
    flush();
    return grouped;
  }

  _render(force) {
    if (this._isDestroyed || !this.container || !this.wrapper) return;

    // 1. Calculate visible range
    let totalHeight = 0;
    const eventHeights = this.events.map((ev) => {
      const h = this.heights.get(ev.getId()) || this.DEFAULT_HEIGHT;
      totalHeight += h;
      return h;
    });

    const scrollTop = this.container.scrollTop;
    const viewportHeight = this.container.clientHeight;

    let acc = 0,
      startIndex = 0;
    for (let i = 0; i < eventHeights.length; i++) {
      if (acc + eventHeights[i] > scrollTop) break;
      acc += eventHeights[i];
      startIndex = i + 1;
    }

    let endAcc = acc,
      endIndex = startIndex;
    for (let i = startIndex; i < eventHeights.length; i++) {
      if (endAcc > scrollTop + viewportHeight) break;
      endAcc += eventHeights[i];
      endIndex = i + 1;
    }

    startIndex = Math.max(0, startIndex - this.OVERSCAN);
    endIndex = Math.min(this.events.length, endIndex + this.OVERSCAN);

    // Expand range to include the preserved anchor index
    if (this._preserveAnchorIndex !== null) {
      startIndex = Math.min(startIndex, this._preserveAnchorIndex);
      endIndex = Math.max(endIndex, this._preserveAnchorIndex + 1);
    }

    // 2. Calculate Spacer Heights
    let topHeight = 0;
    for (let i = 0; i < startIndex; i++) topHeight += eventHeights[i];
    let bottomHeight = 0;
    for (let i = endIndex; i < eventHeights.length; i++)
      bottomHeight += eventHeights[i];

    this.topSpacer.style.height = `${topHeight}px`;
    this.bottomSpacer.style.height = `${bottomHeight}px`;

    // 3. Determine visible IDs
    const visibleIds = new Set();
    for (let i = startIndex; i < endIndex; i++) {
      visibleIds.add(this.events[i].getId());
    }

    // 4. DOM RECONCILIATION
    // Remove nodes that scrolled out of bounds
    for (const [id, node] of this.nodes.entries()) {
      if (!visibleIds.has(id)) {
        this._resizeObserver.unobserve(node);
        node.remove();
        this.nodes.delete(id);
      }
    }

    // Build a fragment and replace children atomically (single reflow)
    const fragment = document.createDocumentFragment();
    for (let i = startIndex; i < endIndex; i++) {
      const ev = this.events[i];
      const id = ev.getId();
      let node = this.nodes.get(id);
      if (!node) {
        node = createMessageNode(ev);
        node.dataset.eventId = id;
        this.nodes.set(id, node);
        this._resizeObserver.observe(node);
      }
      fragment.appendChild(node);
    }
    this.wrapper.replaceChildren(fragment);

    // Keep bottom sticky if user was at bottom
    if (this.isAtBottom) {
      this._stickToBottom();
    }
  }
}

let chatVirtualizer = null;
let chatTimelineListenerBound = false;
const MEDIA_GRID_CAP = 6;
window.loadingOlder = false;
let matrixClient = null;
let matrixRoom = null;
let matrixStarted = false;

// Directorio de matrix_user_id -> nombre
let matrixDirectory = null;
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

// Estado de respuesta (cita de mensaje).
let replyingTo = null;
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
    let name =
      replyingTo.sender === matrixClient.getUserId()
        ? "Tú"
        : matrixDirectory?.get(replyingTo.sender) ||
        replyingTo.sender.split(":")[0].replace("@", "");
    const replyColor = getUserColor(replyingTo.sender);
    let previewText = replyingTo.body || "";
    if (replyingTo.msgtype === "m.image") previewText = "📷 Imagen";
    else if (replyingTo.msgtype === "m.video") previewText = "🎬 Video";
    else if (replyingTo.msgtype === "m.audio") previewText = "🎤 Audio";
    previewText =
      previewText.length > 50 ? previewText.slice(0, 50) + "…" : previewText;
    preview.innerHTML = `<div style="font-size:0.75rem;color:#ccc;white-space:nowrap;overflow:hidden;text-overflow:ellipsis;"><strong style="color:${replyColor};">${escapeHtml(name)}</strong> ${escapeHtml(previewText)}</div>`;
    bar.style.display = "flex";
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
    const res = await authFetch(`${API_BASE}/auth/matrix`, { method: "POST" });
    if (!res.ok) {
      const data = await res.json().catch(() => ({}));
      renderChatError(data.error || "No se pudo conectar al chat.");
      return;
    }
    const { base_url, user_id, password } = await res.json();
    if (window.Olm && typeof window.Olm.init === "function")
      await window.Olm.init();

    const storedDeviceId = localStorage.getItem("sismo_matrix_device_id");
    const tmpClient = sdk.createClient({ baseUrl: base_url });
    const loginResp = await tmpClient.login("m.login.password", {
      identifier: {
        type: "m.id.user",
        user: user_id.split(":")[0].replace("@", ""),
      },
      password,
      ...(storedDeviceId ? { device_id: storedDeviceId } : {}),
    });
    localStorage.setItem("sismo_matrix_device_id", loginResp.device_id);

    matrixClient = sdk.createClient({
      baseUrl: base_url,
      accessToken: loginResp.access_token,
      userId: loginResp.user_id,
      deviceId: loginResp.device_id,
    });
    await matrixClient.initCrypto();
    matrixClient.setGlobalErrorOnUnknownDevices(false);
    await matrixClient.startClient({ initialSyncLimit: 20 });

    matrixRoom = await joinOrCreateRoom();
    matrixDirectory = await loadMatrixDirectory();
    renderChatUI();
    bindChatEvents();
  } catch (e) {
    console.error("Matrix chat error:", e);
    renderChatError("Error al conectar el chat: " + (e.message || e));
  }
}

async function joinOrCreateRoom() {
  let roomId = null;
  try {
    const aliasRes = await matrixClient.getRoomIdForAlias(MATRIX_ROOM_ALIAS);
    roomId = aliasRes && aliasRes.room_id;
  } catch (e) {
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
  try {
    await matrixClient.joinRoom(roomId);
  } catch (e) {
    if (e.errcode !== "M_FORBIDDEN") throw e;
  }
  return roomId;
}

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
  container
    .querySelector(".chat-list-item")
    .addEventListener("click", openChatModal);
  container
    .querySelector("#chatInviteBtn")
    .addEventListener("click", openInviteModal);
}

async function openInviteModal() {
  const modal = Modal({ id: "inviteModal" });
  if (!modal) return;
  const list = document.getElementById("inviteList");
  if (list)
    list.innerHTML = `<div style="padding:12px;color:#999;">Cargando…</div>`;
  modal.open();
  try {
    const res = await authFetch(`${API_BASE}/collaborators/matrix`);
    if (!res.ok) throw new Error("no se pudo cargar la lista");
    const collabs = await res.json();
    renderInviteList(collabs);
  } catch (e) {
    if (list)
      list.innerHTML = `<div style="padding:12px;color:#f66;">Error al cargar colaboradores.</div>`;
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
    .map(
      (c) => `
    <div style="display:flex;align-items:center;gap:8px;padding:8px;border-bottom:1px solid rgba(120,120,120,0.2);">
        <span style="flex:1;">${escapeHtml(c.name)}</span>
        <button class="invite-collab-btn" data-user-id="${escapeHtml(c.matrix_user_id)}" style="
        padding:4px 10px;border-radius:6px;border:1px solid rgba(120,120,120,0.3);
        background:rgba(63,163,77,0.25);cursor:pointer;font-size:0.8em;
        ">Invitar</button>
    </div>
    `,
    )
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

async function openChatModal() {
  if (!matrixDirectory) {
    matrixDirectory = await loadMatrixDirectory();
  }
  const modal = Modal({
    id: "chatModal",
    onClose: cleanupChatVirtualizer,
    onOpen: () => {
      if (!chatVirtualizer) {
        chatVirtualizer = new ChatVirtualList();
      }
      const room = matrixClient.getRoom(matrixRoom);
      if (room && chatVirtualizer) {
        const initialEvents = room
          .getLiveTimeline()
          .getEvents()
          .filter((e) => e.getType() === "m.room.message");
        chatVirtualizer.setEvents(initialEvents);
      }
      // Reset the modal sub-tabs to the chat view on open.
      if (typeof setChatModalSubTab === "function") setChatModalSubTab("chat");
    },
  });
  if (modal) modal.open();
}

function bindChatEvents() {
  const input = document.getElementById("chatInput");
  const sendBtn = document.getElementById("chatSendBtn");

  // Guard against duplicate Matrix timeline listeners
  if (!chatTimelineListenerBound && matrixClient) {
    matrixClient.on("Room.timeline", (event, room, toStartOfTimeline) => {
      if (!room || room.roomId !== matrixRoom) return;
      if (event.getType() !== "m.room.message") return;
      if (toStartOfTimeline) return; // Ignore scrollback pagination

      if (chatVirtualizer) chatVirtualizer.appendEvent(event);
    });
    chatTimelineListenerBound = true;
  }

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
      input.style.height = "auto";
      clearReplyingTo();
    } catch (e) {
      console.error("send error:", e);
    }
  };

  sendBtn.addEventListener("click", send);
  const replyCancel = document.getElementById("chatReplyCancel");
  if (replyCancel) replyCancel.addEventListener("click", clearReplyingTo);

  const attachBtn = document.getElementById("chatAttachBtn");
  const fileInput = document.getElementById("chatFileInput");
  if (attachBtn && fileInput) {
    attachBtn.addEventListener("click", () => fileInput.click());
    fileInput.addEventListener("change", async () => {
      for (const file of Array.from(fileInput.files || []))
        await sendChatFile(file);
      fileInput.value = "";
    });
  }

  // Camera button: opens the device camera (mobile) or a file picker
  // filtered to images (desktop) via the capture attribute.
  const cameraBtn = document.getElementById("chatCameraBtn");
  const cameraInput = document.getElementById("chatCameraInput");
  if (cameraBtn && cameraInput) {
    cameraBtn.addEventListener("click", () => cameraInput.click());
    cameraInput.addEventListener("change", async () => {
      for (const file of Array.from(cameraInput.files || []))
        await sendChatFile(file);
      cameraInput.value = "";
    });
  }

  input.addEventListener("keydown", (e) => {
    if (e.key === "Enter" && !e.shiftKey) {
      e.preventDefault();
      send();
    }
  });
  input.addEventListener("input", () => {
    input.style.height = "auto";
    input.style.height = Math.min(input.scrollHeight, 120) + "px";
  });

  // --- Audio recording ---
  const micBtn = document.getElementById("chatMicBtn");
  let mediaRecorder = null;
  let audioChunks = [];
  let isRecording = false;

  function updateMicBtn(recording) {
    if (!micBtn) return;
    if (recording) {
      micBtn.textContent = "⏹";
      micBtn.title = "Detener grabación";
      micBtn.classList.add("recording");
    } else {
      micBtn.textContent = "🎤";
      micBtn.title = "Grabar mensaje de voz";
      micBtn.classList.remove("recording");
    }
  }

  if (micBtn) {
    micBtn.addEventListener("click", async () => {
      if (isRecording) {
        if (mediaRecorder && mediaRecorder.state !== "inactive") {
          mediaRecorder.stop();
        }
        return;
      }
      try {
        const stream = await navigator.mediaDevices.getUserMedia({
          audio: true,
        });
        audioChunks = [];
        mediaRecorder = new MediaRecorder(stream, {
          mimeType: "audio/webm;codecs=opus",
        });
        mediaRecorder.ondataavailable = (e) => {
          if (e.data.size > 0) audioChunks.push(e.data);
        };
        mediaRecorder.onstop = async () => {
          stream.getTracks().forEach((t) => t.stop());
          if (audioChunks.length === 0) {
            isRecording = false;
            updateMicBtn(false);
            return;
          }
          const blob = new Blob(audioChunks, { type: "audio/webm" });
          const file = new File([blob], "voice_message.webm", {
            type: "audio/webm",
          });
          await sendChatAudio(file);
          isRecording = false;
          updateMicBtn(false);
        };
        mediaRecorder.start();
        isRecording = true;
        updateMicBtn(true);
      } catch (e) {
        console.error("mic error:", e);
        alert("No se pudo acceder al micrófono.");
      }
    });
  }
}

function cleanupChatVirtualizer() {
  if (chatVirtualizer) {
    chatVirtualizer.destroy();
    chatVirtualizer = null;
  }
}

async function loadOlderMessages() {
  const room = matrixClient.getRoom(matrixRoom);
  if (!room || !chatVirtualizer) return;

  const timeline = room.getLiveTimeline();
  const token = timeline?.getPaginationToken?.("b");
  if (!token) return;

  window.loadingOlder = true;
  const currentEventCount = chatVirtualizer.events.length;

  try {
    await matrixClient.scrollback(room, 30);
    const allEvents = room
      .getLiveTimeline()
      .getEvents()
      .filter((e) => e.getType() === "m.room.message");

    const newOlderEvents = allEvents.slice(
      0,
      allEvents.length - currentEventCount,
    );
    if (newOlderEvents.length > 0) {
      chatVirtualizer.prependEvents(newOlderEvents);
    }
  } catch (e) {
    console.error("load older messages error:", e);
  } finally {
    window.loadingOlder = false;
  }
}

function createMessageNode(item) {
  // Handle media group (multiple images/videos from same sender)
  if (item._isMediaGroup) {
    const sender = item.getSender();
    const isSelf = sender === matrixClient.getUserId();
    const name = isSelf
      ? window.currentUser?.chat_name || "Tú"
      : matrixDirectory?.get(sender) || sender.split(":")[0].replace("@", "");
    const ts = item.getTs();
    const date = new Date(ts);
    const timeStr = date.toLocaleTimeString([], {
      hour: "2-digit",
      minute: "2-digit",
    });
    const senderColor = getUserColor(sender);

    const div = document.createElement("div");
    div.style.cssText = `align-self:${isSelf ? "flex-end" : "flex-start"};max-width:75%;padding:3px;border-radius:12px;background:${isSelf ? "rgba(63,163,77,0.35)" : "rgba(120,120,120,0.25)"};margin-top:8px;word-break:break-word;contain:layout;`;

    const items = item.items.map((ev) => {
      const c = ev.getContent();
      const fullSrc = c.url ? matrixClient.mxcUrlToHttp(c.url) : null;
      const thumbSrc = c.info?.thumbnail_url
        ? matrixClient.mxcUrlToHttp(c.info.thumbnail_url)
        : fullSrc;
      return {
        msgtype: c.msgtype,
        src: thumbSrc,
        fullSrc,
        body: c.body,
        eventId: ev.getId(),
      };
    });

    const gridEl = document.createElement("div");
    gridEl.className = "chat-media-grid";
    const firstContent = item.items[0].getContent();
    if (firstContent.info && firstContent.info.w && firstContent.info.h) {
      gridEl.style.aspectRatio = `${firstContent.info.w} / ${firstContent.info.h}`;
      gridEl.style.minHeight = "80px";
    } else {
      gridEl.style.aspectRatio = "4 / 3";
      gridEl.style.minHeight = "120px";
    }

    const group = { sender, ts: item.getTs(), gridEl, items };
    renderMediaGrid(group);

    let replyHtml = "";
    const replyPreview = firstContent["m.reply_preview"];
    const replyEventId =
      firstContent["m.relates_to"]?.["m.in_reply_to"]?.["event_id"];
    if (replyPreview) {
      const replySender =
        replyPreview.sender === matrixClient.getUserId()
          ? "Tú"
          : matrixDirectory?.get(replyPreview.sender) ||
          replyPreview.sender.split(":")[0].replace("@", "");
      const replyColor = getUserColor(replyPreview.sender);
      let previewText = replyPreview.body || "";
      if (replyPreview.msgtype === "m.image") previewText = "📷 Imagen";
      else if (replyPreview.msgtype === "m.video") previewText = "🎬 Video";
      else if (replyPreview.msgtype === "m.audio") previewText = "🎤 Audio";
      previewText =
        previewText.length > 50 ? previewText.slice(0, 50) + "…" : previewText;
      const clickAttr = replyEventId
        ? ` onclick="window.scrollToChatEvent('${replyEventId}')" style="cursor:pointer;"`
        : "";
      replyHtml = `<div class="msg-reply"${clickAttr} style="margin-bottom:4px;padding:6px 8px;background:rgba(0,0,0,0.15);border-radius:8px;border-left:3px solid ${replyColor};font-size:0.8rem;"><div style="font-weight:600;font-size:0.7rem;color:${replyColor};">${escapeHtml(replySender)}</div><div style="color:#ccc;white-space:pre-wrap;overflow:hidden;text-overflow:ellipsis;">${escapeHtml(previewText)}</div></div>`;
    }

    div.innerHTML = `<div class="msg-sender-name" style="font-size:70%;opacity:0.7;display:flex;align-items:center;gap:6px;"><span style="color:${senderColor};font-weight:600;">${escapeHtml(name)}</span><span style="font-size:60%;opacity:0.6;">${escapeHtml(timeStr)}</span></div>${replyHtml}`;
    div.appendChild(group.gridEl);

    const firstEvent = item.items[0];
    const showReplyMenu = (e) => {
      e.preventDefault();
      setReplyingTo(firstEvent);
      div.style.outline = "2px solid #3fa34d";
      setTimeout(() => {
        div.style.outline = "";
      }, 300);
    };
    div.addEventListener("contextmenu", showReplyMenu);
    let pressTimer = null;
    div.addEventListener("mousedown", () => {
      pressTimer = setTimeout(showReplyMenu, 500);
    });
    div.addEventListener("mouseup", () => clearTimeout(pressTimer));
    div.addEventListener("mouseleave", () => clearTimeout(pressTimer));
    div.addEventListener(
      "touchstart",
      () => {
        pressTimer = setTimeout(showReplyMenu, 500);
      },
      { passive: true },
    );
    div.addEventListener("touchend", () => clearTimeout(pressTimer));
    div.addEventListener("touchmove", () => clearTimeout(pressTimer));

    return div;
  }

  // Handle single event (original logic)
  const content = item.getContent();
  const msgtype = content.msgtype;
  const sender = item.getSender();
  const isSelf = sender === matrixClient.getUserId();
  const isMedia = msgtype === "m.image" || msgtype === "m.video";

  const name = isSelf
    ? window.currentUser?.chat_name || "Tú"
    : matrixDirectory?.get(sender) || sender.split(":")[0].replace("@", "");

  const ts = item.getTs();
  const date = new Date(ts);
  const timeStr = date.toLocaleTimeString([], {
    hour: "2-digit",
    minute: "2-digit",
  });
  const senderColor = getUserColor(sender);

  const div = document.createElement("div");
  div.style.cssText = `align-self:${isSelf ? "flex-end" : "flex-start"};max-width:75%;padding:${isMedia ? "3px" : "8px 12px"};border-radius:12px;background:${isSelf ? "rgba(63,163,77,0.35)" : "rgba(120,120,120,0.25)"};margin-top:8px;word-break:break-word;contain:layout;`;

  let replyHtml = "";
  const replyPreview = content["m.reply_preview"];
  const replyEventId = content["m.relates_to"]?.["m.in_reply_to"]?.["event_id"];
  if (replyPreview) {
    const replySender =
      replyPreview.sender === matrixClient.getUserId()
        ? "Tú"
        : matrixDirectory?.get(replyPreview.sender) ||
        replyPreview.sender.split(":")[0].replace("@", "");
    const replyColor = getUserColor(replyPreview.sender);
    let previewText = replyPreview.body || "";
    if (replyPreview.msgtype === "m.image") previewText = "📷 Imagen";
    else if (replyPreview.msgtype === "m.video") previewText = "🎬 Video";
    else if (replyPreview.msgtype === "m.audio") previewText = "🎤 Audio";
    previewText =
      previewText.length > 50 ? previewText.slice(0, 50) + "…" : previewText;
    const clickAttr = replyEventId
      ? ` onclick="window.scrollToChatEvent('${replyEventId}')" style="cursor:pointer;"`
      : "";
    replyHtml = `<div class="msg-reply"${clickAttr} style="margin-bottom:4px;padding:6px 8px;background:rgba(0,0,0,0.15);border-radius:8px;border-left:3px solid ${replyColor};font-size:0.8rem;"><div style="font-weight:600;font-size:0.7rem;color:${replyColor};">${escapeHtml(replySender)}</div><div style="color:#ccc;white-space:pre-wrap;overflow:hidden;text-overflow:ellipsis;">${escapeHtml(previewText)}</div></div>`;
  }

  if (isMedia) {
    const fullSrc = content.url ? matrixClient.mxcUrlToHttp(content.url) : null;
    const thumbSrc = content.info?.thumbnail_url
      ? matrixClient.mxcUrlToHttp(content.info.thumbnail_url)
      : fullSrc;
    const gridEl = document.createElement("div");
    gridEl.className = "chat-media-grid";
    if (content.info && content.info.w && content.info.h) {
      gridEl.style.aspectRatio = `${content.info.w} / ${content.info.h}`;
      gridEl.style.minHeight = "80px";
    } else {
      gridEl.style.aspectRatio = "4 / 3";
      gridEl.style.minHeight = "120px";
    }
    const group = {
      sender,
      ts: item.getTs(),
      gridEl,
      items: [
        {
          msgtype,
          src: thumbSrc,
          fullSrc,
          body: content.body,
          eventId: item.getId(),
        },
      ],
    };
    renderMediaGrid(group);
    div.innerHTML = `<div class="msg-sender-name" style="font-size:70%;opacity:0.7;display:flex;align-items:center;gap:6px;"><span style="color:${senderColor};font-weight:600;">${escapeHtml(name)}</span><span style="font-size:60%;opacity:0.6;">${escapeHtml(timeStr)}</span></div>${replyHtml}`;
    div.appendChild(group.gridEl);
  } else if (msgtype === "m.audio") {
    const audioSrc = content.url
      ? matrixClient.mxcUrlToHttp(content.url)
      : null;
    const duration = content.info?.duration
      ? formatDuration(content.info.duration)
      : "";
    div.innerHTML = `<div class="msg-sender-name" style="font-size:70%;opacity:0.7;display:flex;align-items:center;gap:6px;"><span style="color:${senderColor};font-weight:600;">${escapeHtml(name)}</span><span style="font-size:60%;opacity:0.6;">${escapeHtml(timeStr)}</span></div>${replyHtml}<div style="display:flex;align-items:center;gap:8px;padding:4px 0;"><audio controls style="height:28px;flex:1;" src="${escapeHtml(audioSrc || "")}"></audio>${duration ? `<span style="font-size:0.75em;opacity:0.7;">${duration}</span>` : ""}</div>`;
  } else {
    div.innerHTML = `<div class="msg-sender-name" style="font-size:70%;opacity:0.7;display:flex;align-items:center;gap:6px;"><span style="color:${senderColor};font-weight:600;">${escapeHtml(name)}</span><span style="font-size:60%;opacity:0.6;">${escapeHtml(timeStr)}</span></div>${replyHtml}<div style="white-space:pre-wrap;">${escapeHtml(content.body || "")}</div>`;
  }

  const showReplyMenu = (e) => {
    e.preventDefault();
    setReplyingTo(item);
    div.style.outline = "2px solid #3fa34d";
    setTimeout(() => {
      div.style.outline = "";
    }, 300);
  };
  div.addEventListener("contextmenu", showReplyMenu);
  let pressTimer = null;
  div.addEventListener("mousedown", () => {
    pressTimer = setTimeout(showReplyMenu, 500);
  });
  div.addEventListener("mouseup", () => clearTimeout(pressTimer));
  div.addEventListener("mouseleave", () => clearTimeout(pressTimer));
  div.addEventListener(
    "touchstart",
    () => {
      pressTimer = setTimeout(showReplyMenu, 500);
    },
    { passive: true },
  );
  div.addEventListener("touchend", () => clearTimeout(pressTimer));
  div.addEventListener("touchmove", () => clearTimeout(pressTimer));

  return div;
}

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

async function sendChatAudio(file) {
  if (!matrixRoom || !file) return;
  try {
    const uploadRes = await matrixClient.uploadContent(file, {
      includeFilename: true,
      type: file.type,
    });
    const content = {
      msgtype: "m.audio",
      body: file.name || "voice_message.webm",
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
    console.error("send audio error:", e);
    alert("No se pudo enviar el audio.");
  }
}

function mediaThumbHtml(item, index, isLast, extraCount) {
  const src = item.src;
  const fullSrc = item.fullSrc || src;
  const emoji = item.msgtype === "m.video" ? "🎬" : "📷";
  let inner;
  if (item.msgtype === "m.video") {
    inner = src
      ? `<video src="${escapeHtml(src)}" muted style="width:100%;height:100%;object-fit:cover;display:block;"></video>`
      : `<div style="color:#999;display:flex;align-items:center;justify-content:center;height:100%;">${emoji}</div>`;
  } else {
    inner = src
      ? `<img src="${escapeHtml(src)}" alt="${escapeHtml(item.body || "imagen")}" loading="lazy" decoding="async" onerror="if(!this.dataset.retry){this.dataset.retry='1';this.src='${escapeHtml(fullSrc)}';}" style="width:100%;height:100%;object-fit:cover;display:block;cursor:pointer;" />`
      : `<div style="color:#999;display:flex;align-items:center;justify-content:center;height:100%;">${emoji}</div>`;
  }
  const overlay =
    isLast && extraCount > 0
      ? `<div class="chat-media-more">+${extraCount}</div>`
      : "";
  const emojiOverlay = `<span style="position:absolute;top:4px;left:4px;font-size:0.9em;z-index:1;pointer-events:none;">${emoji}</span>`;
  return `<div class="chat-media-item" data-index="${index}" style="position:relative;">${inner}${emojiOverlay}${overlay}</div>`;
}

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
  group.gridEl.onclick = (e) => {
    const tile = e.target.closest(".chat-media-item");
    if (!tile) return;
    openLightbox(group.items, parseInt(tile.dataset.index, 10));
  };
}

// Expose to global scope so index.js can trigger it after login
window.startMatrixChat = startMatrixChat;
window.scrollToChatEvent = (id) => chatVirtualizer?.scrollToEvent(id);

// ===== LIGHTBOX =====
let _lightboxItems = [];
let _lightboxIndex = 0;
let _lightboxOcrData = null; // Store current OCR data for overlay

function openLightbox(items, index) {
  _lightboxItems = items;
  _lightboxIndex = index;
  updateLightboxStage();
  const modal = Modal({ id: "mediaLightbox" });
  if (modal) modal.open();
}

function updateLightboxStage() {
  const stage = document.getElementById("lightboxStage");
  const counter = document.getElementById("lightboxCounter");
  const prevBtn = document.getElementById("lightboxPrev");
  const nextBtn = document.getElementById("lightboxNext");
  const ocrToggleBtn = document.getElementById("lightboxOcrToggle");
  const ocrOverlay = document.getElementById("lightboxOcrOverlay");
  if (!stage) return;

  const item = _lightboxItems[_lightboxIndex];
  const src = item.fullSrc || item.src;

  const emoji = item.msgtype === "m.video" ? "🎬" : "📷";
  if (item.msgtype === "m.video") {
    stage.innerHTML = `<span style="position:absolute;top:8px;left:8px;font-size:1.5em;z-index:1;">${emoji}</span><video src="${escapeHtml(src)}" controls autoplay style="max-width:100%;max-height:100%;width:100%;height:100%;object-fit:contain;"></video>`;
  } else {
    stage.innerHTML = `<span style="position:absolute;top:8px;left:8px;font-size:1.5em;z-index:1;">${emoji}</span><img src="${escapeHtml(src)}" alt="${escapeHtml(item.body || "")}" style="max-width:100%;max-height:100%;width:100%;height:100%;object-fit:contain;"/>`;
  }

  // OCR / extracted-text panel below the media (images only)
  const ocrContainer = document.getElementById("lightboxOcr");
  if (ocrContainer) {
    if (item.msgtype === "m.image" && item.eventId) {
      ocrContainer.innerHTML = `<div style="text-align:center;color:#666;font-size:0.8rem;">Cargando texto extraído…</div>`;
      fetchOcrText(item.eventId, ocrContainer);
    } else {
      ocrContainer.innerHTML = "";
    }
  }

  // Show/hide OCR toggle button for images with eventId
  if (ocrToggleBtn) {
    if (item.msgtype === "m.image" && item.eventId) {
      ocrToggleBtn.style.display = "flex";
    } else {
      ocrToggleBtn.style.display = "none";
    }
  }
  // Hide overlay when changing image
  if (ocrOverlay) ocrOverlay.style.display = "none";

  if (counter)
    counter.textContent = `${_lightboxIndex + 1} / ${_lightboxItems.length}`;
  if (prevBtn) prevBtn.disabled = _lightboxIndex === 0;
  if (nextBtn) nextBtn.disabled = _lightboxIndex === _lightboxItems.length - 1;
}

async function fetchOcrText(eventId, container) {
  try {
    const res = await authFetch(
      `${API_BASE}/backlog/by-event/${encodeURIComponent(eventId)}`,
    );
    if (!res.ok) {
      container.innerHTML = "";
      _lightboxOcrData = null;
      return;
    }
    const data = await res.json();
    if (!data.raw_text) {
      container.innerHTML = "";
      _lightboxOcrData = null;
      return;
    }
    // Cache the OCR text so the overlay can render it on toggle.
    _lightboxOcrData = { raw_text: data.raw_text, intent: data.intent || null };
    container.innerHTML = `
            <div style="padding:12px;background:rgba(255,255,255,0.05);border:1px solid rgba(255,255,255,0.1);border-radius:8px;font-size:0.9rem;color:#ccc;max-height:25vh;overflow-y:auto;text-align:left;">
                <div style="font-size:0.75rem;color:#45d6d6;margin-bottom:8px;font-weight:bold;">
                    🧠 Texto extraído
                    ${data.intent ? `<span style="background:rgba(69,214,214,0.2);padding:2px 6px;border-radius:4px;margin-left:6px;">${escapeHtml(data.intent)}</span>` : ""}
                </div>
                <pre style="white-space:pre-wrap;word-break:break-word;margin:0;font-family:inherit;line-height:1.5;">${escapeHtml(data.raw_text)}</pre>
            </div>`;
    // Populate the overlay text so the toggle can show it over the image.
    const overlayText = document.getElementById("lightboxOcrText");
    if (overlayText) overlayText.textContent = data.raw_text;
  } catch (e) {
    console.error("OCR fetch failed:", e);
    container.innerHTML = "";
    _lightboxOcrData = null;
  }
}

// Wire lightbox nav buttons once
let _lightboxWired = false;
function wireLightboxNav() {
  if (_lightboxWired) return;
  _lightboxWired = true;
  const prevBtn = document.getElementById("lightboxPrev");
  const nextBtn = document.getElementById("lightboxNext");
  if (prevBtn)
    prevBtn.addEventListener("click", () => {
      if (_lightboxIndex > 0) {
        _lightboxIndex--;
        updateLightboxStage();
      }
    });
  if (nextBtn)
    nextBtn.addEventListener("click", () => {
      if (_lightboxIndex < _lightboxItems.length - 1) {
        _lightboxIndex++;
        updateLightboxStage();
      }
    });

  // OCR toggle: show/hide the semi-transparent overlay over the image.
  const ocrToggleBtn = document.getElementById("lightboxOcrToggle");
  const ocrOverlay = document.getElementById("lightboxOcrOverlay");
  if (ocrToggleBtn && ocrOverlay) {
    ocrToggleBtn.addEventListener("click", () => {
      const isHidden =
        ocrOverlay.style.display === "none" || ocrOverlay.style.display === "";
      ocrOverlay.style.display = isHidden ? "block" : "none";
      ocrToggleBtn.textContent = isHidden ? "✕ OCR" : "📝 OCR";
    });
  }

  // Copy OCR text to clipboard.
  const ocrCopyBtn = document.getElementById("lightboxOcrCopy");
  if (ocrCopyBtn) {
    ocrCopyBtn.addEventListener("click", async () => {
      const text =
        document.getElementById("lightboxOcrText")?.textContent || "";
      if (!text) return;
      try {
        await navigator.clipboard.writeText(text);
        ocrCopyBtn.textContent = "✅ Copiado";
        setTimeout(() => {
          ocrCopyBtn.textContent = "📋 Copiar texto";
        }, 1500);
      } catch (e) {
        console.error("copy failed:", e);
      }
    });
  }
}
if (document.readyState === "loading") {
  document.addEventListener("DOMContentLoaded", wireLightboxNav);
} else {
  wireLightboxNav();
}

// ================================================================
//  SUB-TABS DENTRO DEL MODAL DE CHAT
//  Replica las sub-tabs de la pestaña Colaboradores dentro del modal
//  de chat. Al hacer clic, cambia el contenido del modal entre el chat
//  y las listas (donaciones/necesidades/logística/voluntarios/triage).
// ================================================================
let _chatModalSubTabWired = false;
function setChatModalSubTab(target) {
  const tabs = document.getElementById("chatModalSubTabs");
  const panels = document.getElementById("chatModalSubPanels");
  const messages = document.getElementById("chatMessages");
  const inputBar = document.getElementById("chatInput")?.closest("div");
  const replyBar = document.getElementById("chatReplyBar");
  if (!tabs || !panels) return;

  const btns = tabs.querySelectorAll(".sub-tab-btn");
  const panelEls = panels.querySelectorAll(".sub-tab-panel");

  btns.forEach((b) =>
    b.classList.toggle("active", b.dataset.subtab === target),
  );
  panelEls.forEach((p) =>
    p.classList.toggle("active", p.dataset.subtabPanel === target),
  );

  const isChat = target === "chat";
  // Chat view: show messages + input bar, hide the list panels.
  if (messages) messages.style.display = isChat ? "" : "none";
  // Explicitly set "flex" (not "") so the flex layout is never dropped
  // when toggling visibility — an empty string removes the inline style
  // and the div falls back to display:block, breaking the row.
  if (inputBar) inputBar.style.display = isChat ? "flex" : "none";
  if (replyBar) replyBar.style.display = isChat ? "flex" : "none";
  panels.style.display = isChat ? "none" : "flex";

  // Lazy-load the target list into the modal's own container.
  if (!isChat) {
    if (target === "donaciones")
      renderModalList(
        "chatModalListDonaciones",
        donacionesData,
        donacionCardHtml,
        "Aún no hay donaciones ofrecidas.",
      );
    else if (target === "necesidades")
      renderModalList(
        "chatModalListNecesidades",
        necesidadesData,
        necesidadCardHtml,
        "Aún no hay necesidades reportadas.",
      );
    else if (target === "logistica")
      renderModalList(
        "chatModalListLogistica",
        logisticaData,
        logisticaCardHtml,
        "Aún no hay tareas de logística.",
      );
    else if (target === "voluntarios")
      renderModalList(
        "chatModalListColab",
        colabData,
        colabCardHtml,
        "Todavía nadie se ha unido como colaborador.",
      );
    else if (target === "backlog")
      renderModalList(
        "chatModalListBacklog",
        backlogData,
        backlogCardHtml,
        "Aún no hay mensajes para revisar.",
      );
  }
}

function wireChatModalSubTabs() {
  if (_chatModalSubTabWired) return;
  _chatModalSubTabWired = true;

  const tabs = document.getElementById("chatModalSubTabs");
  if (!tabs) return;

  const btns = tabs.querySelectorAll(".sub-tab-btn");
  btns.forEach((btn) =>
    btn.addEventListener("click", () => setChatModalSubTab(btn.dataset.subtab)),
  );
}

// Render a data list into a modal container using the shared card renderers
// (defined in index.js). Falls back to a plain message if renderers are missing.
function renderModalList(containerId, data, cardHtmlFn, emptyMsg) {
  const el = document.getElementById(containerId);
  if (!el) return;
  if (!data || !data.length) {
    el.innerHTML = `<div class="empty">${emptyMsg}</div>`;
    return;
  }
  if (typeof cardHtmlFn !== "function") {
    el.innerHTML = `<div class="empty">No se pudo renderizar.</div>`;
    return;
  }
  if (typeof renderVirtualList === "function") {
    renderVirtualList(el, data, cardHtmlFn);
  } else {
    el.innerHTML = data.map(cardHtmlFn).join("");
  }
}

if (document.readyState === "loading") {
  document.addEventListener("DOMContentLoaded", wireChatModalSubTabs);
} else {
  wireChatModalSubTabs();
}

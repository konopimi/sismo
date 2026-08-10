import { html } from "https://esm.sh/uhtml@4.0";
import { store, addDisappeared, markFound, removeDisappeared } from "../vStore/assets.js";

const API_BASE = "http://YOUR_SERVER_IP:3000/api"; // <-- REPLACE with your Vultr IP

// Sync local reports to server when online
async function syncToServer() {
  try {
    const res = await fetch(`${API_BASE}/disappeared`);
    if (res.ok) {
      const serverReports = await res.json();
      const localById = new Map(store.disappeared.map(d => [d.id, d]));
      const merged = serverReports.map(sr => {
        const local = localById.get(sr.id);
        if (local && local.updatedAt > sr.created_at) return local;
        return { ...sr, createdAt: sr.created_at };
      });
      for (const local of store.disappeared) {
        if (!serverReports.some(sr => sr.id === local.id)) {
          merged.push(local);
        }
      }
      store.disappeared = merged;
    }
  } catch (e) {
    console.log("Offline, using local data");
  }
}

// Push a new report to server
async function pushToServer(report) {
  try {
    await fetch(`${API_BASE}/disappeared`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(report),
    });
  } catch (e) {
    console.log("Will sync later when online");
  }
}

export default function Desaparecidos() {
  const list = store.disappeared;

  syncToServer();

  function onSubmit(e) {
    e.preventDefault();
    const input = e.target.elements.name;
    const name = input.value.trim();
    if (!name) return;
    const report = {
      id: crypto.randomUUID(),
      name,
      status: "desaparecido",
      createdAt: new Date().toISOString(),
    };
    addDisappeared(name);
    pushToServer(report);
    input.value = "";
    input.focus();
  }

  function onMarkFound(id) {
    markFound(id);
  }

  function onRemove(id) {
    removeDisappeared(id);
  }

  return html`
    <section style="max-width:640px;margin:0 auto;padding:16px;">
      <h1 style="margin:0 0 4px;">Personas Desaparecidas</h1>
      <p style="color:#666;margin:0 0 16px;">
        Reporta y consulta personas reportadas desaparecidas.
      </p>

      <form onsubmit=${onSubmit} style="display:flex;gap:8px;margin-bottom:20px;">
        <input
          name="name"
          type="text"
          placeholder="Nombre y apellido"
          required
          style="flex:1;padding:10px;font-size:16px;"
        />
        <button type="submit" style="padding:10px 16px;font-size:16px;">
          Reportar
        </button>
      </form>

      <h2 style="font-size:18px;margin:0 0 8px;">
        Lista (${list.length})
      </h2>

      ${list.length === 0
        ? html`<p style="color:#999;">Aún no hay personas reportadas.</p>`
        : html`<ul style="list-style:none;padding:0;margin:0;">
            ${list.map(
              (p) => html`<li
                style="display:flex;align-items:center;justify-content:space-between;
                       padding:12px;border:1px solid #eee;border-radius:6px;margin-bottom:8px;
                       ${p.status === "encontrado"
                         ? "opacity:0.6;text-decoration:line-through;"
                         : ""}"
              >
                <span>
                  <strong>${p.name}</strong>
                  <br />
                  <small style="color:#888;">
                    ${new Date(p.createdAt).toLocaleString("es-CO")} —
                    ${p.status}
                  </small>
                </span>
                <span style="display:flex;gap:6px;">
                  ${p.status === "desaparecido"
                    ? html`<button
                        onclick=${() => onMarkFound(p.id)}
                        style="padding:6px 10px;"
                      >Encontrado</button>`
                    : null}
                  <button
                    onclick=${() => onRemove(p.id)}
                    style="padding:6px 10px;color:#c00;"
                  >Eliminar</button>
                </span>
              </li>`
            )}
          </ul>`}
    </section>
  `;
}
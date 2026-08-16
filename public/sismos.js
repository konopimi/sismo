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
    if (!last) return;

    const mag = last.mag || 0;
    document.getElementById("ultimoLugar").textContent =
      last.place || "Lugar desconocido";
    document.getElementById("ultimoMag").textContent = mag.toFixed(1);
    document.getElementById("ultimoHora").textContent = fullTime(
      last.timestamp,
    );
    document.getElementById("ultimoRel").textContent = relTime(last.timestamp);

    const alerta = document.getElementById("sismoAlert");
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
      listEl.innerHTML =
        '<div class="sismo-live-empty">Sin sismos en las últimas 72 h.</div>';
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

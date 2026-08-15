// ================================================================
//  SISMOS EN TIEMPO REAL (ECharts)
//  Self-contained module. Exposes window.initSismos() which index.js
//  calls when the Sismos tab is activated.
// ================================================================
(function() {
  "use strict";
  console.log("[sismos.js] cargado, definiendo window.initSismos");

  let sismosChart = null;
  let sismosData = [];

  function initSismoChart() {
    const dom = document.getElementById("sismoChart");
    if (!dom) return;
    if (typeof echarts === "undefined") {
      console.error("ECharts no está cargado. Verifica el CDN en <head>.");
      return;
    }
    sismosChart = echarts.init(dom);
    window.sismoChart = sismosChart;
    // Asegura que el contenedor tenga tamaño antes de dibujar.
    setTimeout(() => sismosChart && sismosChart.resize(), 50);
    const option = {
      tooltip: {
        trigger: "axis",
        axisPointer: { type: "shadow" },
        formatter: function(params) {
          const p = params[0];
          if (!p) return "";
          const data = p.data;
          return `<strong>${data.place}</strong><br/>Magnitud: <strong>${data.mag}</strong><br/>Profundidad: ${data.depth} km<br/>Hora: ${data.time}`;
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

  async function fetchEarthquakes() {
    try {
      const apiBase = window.API_BASE || "/api";
      const res = await fetch(
        `${apiBase}/earthquakes?minmagnitude=0&limit=500&days=365`,
      );
      if (!res.ok) throw new Error("Error en API");
      const data = await res.json();
      sismosData = data.earthquakes || [];
      updateSismoUI(data);
    } catch (err) {
      console.error("Error fetching sismos:", err);
      document.getElementById("ultimoLugar").textContent = "⚠️ Error al cargar";
      document.getElementById("sismoAlert").style.borderColor = "#d63031";
      const listEl = document.getElementById("sismosList");
      if (listEl) {
        listEl.innerHTML = '<div style="color:#ff6b6b;padding:10px;">⚠️ No se pudieron cargar los sismos. Revisa la conexión o el endpoint /api/earthquakes.</div>';
      }
    }
  }

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

  function renderSismosList() {
    const listEl = document.getElementById("sismosList");
    if (!listEl) return;
    if (!sismosData.length) {
      listEl.innerHTML = '<div style="color:#999;padding:10px;">Sin datos sísmicos todavía.</div>';
      return;
    }
    listEl.innerHTML = sismosData
      .map((q) => {
        const depth = q.depth != null ? `${q.depth.toFixed(1)} km` : "—";
        const url = q.url || "";
        return `
          <div class="sismo-row">
            <span class="mag ${magClass(q.mag)}">${q.mag.toFixed(1)}</span>
            <span class="info">
              <span class="place">${esc(q.place) || "Lugar desconocido"}</span>
              <span class="meta">${esc(q.time)} · Prof. ${esc(depth)}</span>
            </span>
            ${url ? `<a href="${esc(url)}" target="_blank" rel="noopener">Detalle ↗</a>` : ""}
          </div>`;
      })
      .join("");
  }

  function updateSismoUI(data) {
    const last =
      data.earthquakes && data.earthquakes.length ? data.earthquakes[0] : null;
    if (last) {
      document.getElementById("ultimoLugar").textContent =
        last.place || "Lugar desconocido";
      document.getElementById("ultimoMag").textContent = last.mag
        ? last.mag.toFixed(1)
        : "--";
      document.getElementById("ultimoHora").textContent = last.time || "";
      const alerta = document.getElementById("sismoAlert");
      alerta.className = "alerta" + (last.mag >= 5 ? " peligro" : "");
    }
    document.getElementById("totalSismos").textContent =
      `${data.count} sismos mostrados`;
    const badge = document.querySelector("#tabSismosBtn .n");
    if (badge) badge.textContent = data.count;

    // Text fallback: always render, independent of ECharts.
    renderSismosList();

    if (!sismosChart) return;
    const sorted = sismosData.slice().sort((a, b) => a.timestamp - b.timestamp);
    const xData = sorted.map((q) =>
      new Date(q.timestamp).toLocaleTimeString("es-CO", {
        hour: "2-digit",
        minute: "2-digit",
        timeZone: "America/Bogota",
      }),
    );
    const seriesData = sorted.map((q) => ({
      value: q.mag,
      place: q.place || "N/A",
      time: q.time,
      depth: q.depth,
      mag: q.mag,
    }));
    sismosChart.setOption({
      xAxis: { data: xData },
      series: [{ data: seriesData }],
    });
  }

  // Public API: called by index.js when the Sismos tab is activated.
  window.initSismos = function() {
    if (!window.sismoChart) {
      initSismoChart();
    }
    fetchEarthquakes();
    if (window.sismoInterval) clearInterval(window.sismoInterval);
    window.sismoInterval = setInterval(fetchEarthquakes, 10000);
  };
})();

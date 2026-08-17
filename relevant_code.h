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
  <!-- ===== PANEL COLABORADORES ===== -->
  <div id="tabColab" class="tab-panel">
    <!-- Chat de colaboradores (requiere login) -->
    <div id="chatAuth" style="margin-bottom:10px;">
      <div id="loginForm"
        style="margin:auto;margin-top:20px;max-width:600px;display:flex;flex-direction:column;gap:10px;padding:20px;background:rgba(var(--surface),0.38);border-radius:10px;">
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
      <div id="chatContainer" style="display:none;flex:1;overflow:hidden;min-height:0;"></div>
    </div>
    <div id="colabListWrapper" style="display:none;">
      <div id="listColab" class="list"></div>
    </div>
    <div id="colabLoginPrompt" style="padding:16px;text-align:center;color:#999;font-size:0.9rem;">
      🔒 Inicia sesión para ver la lista de colaboradores
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
          <small style="color:#999;width:100%;">Opcional: crea una cuenta para acceder al chat de colaboradores</small>
          <input type="email" id="emailInputColab" placeholder="Email (opcional)" />
          <input type="password" id="passwordInputColab" placeholder="Contraseña (opcional, mín 6 chars)" />
          <button type="submit">Unirme</button>
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
  <script src="modal.js"></script>
  <script src="map.js"></script>
  <script src="sismos.js"></script>
  <script src="index.js"></script>
</body>
</html>
<!-- <div class="tool-bar"> -->
<!--   <div class="filter-row" data-tab="colaborador"> -->
<!--     <select class="filter-select" data-filter="city" disabled> -->
<!--       <option value="">📍 TODAS</option> -->
<!--     </select> -->
<!--   </div> -->
<!--   <button id="crear-top" class="crear-btn" onclick="openCrearModal('colaborador')" disabled> -->
<!--     🤝 COLABORAR -->
<!--   </button> -->
<!-- </div> -->
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
  if (loginForm) loginForm.style.display = "flex";
  if (chatContainer) chatContainer.style.display = "none";
  if (colabListWrapper) colabListWrapper.style.display = "none";
  if (colabLoginPrompt) colabLoginPrompt.style.display = "block";
  window.currentUser = null;
  // Hide the collaborators count badge now that we're logged out.
  if (typeof updateTabCounts === "function") updateTabCounts();
}
function showChat() {
  const loginForm = document.getElementById("loginForm");
  const chatContainer = document.getElementById("chatContainer");
  if (loginForm) loginForm.style.display = "none";
  if (chatContainer) {
    chatContainer.style.display = "flex";
    chatContainer.innerHTML = `<div style="padding:20px;">Conectado como ${escapeHtml(window.currentUser?.name || "")}</div>`;
  }
  if (colabListWrapper) colabListWrapper.style.display = "block";
  if (colabLoginPrompt) colabLoginPrompt.style.display = "none";
  if (!colabData.length) loadListColab();
  // Show the collaborators count badge now that we're logged in.
  if (typeof updateTabCounts === "function") updateTabCounts();
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
  // Expose the status color as a CSS var so the card background/border are
  // derived automatically (color-mix) instead of hardcoded per status.
  const statusColorHex = statusColor(item.status);
  return `
        <div class="card ${cardStatusClass}" data-id="${item.id}" data-type="${type}" style="--status-color:${statusColorHex};${isAngel ? "background:#7fb8ec; color: white; opacity: 0.62; border-left: 4px solid white;" : ""}" >
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
    listEl.innerHTML = `<div class="empty">${personsData.length ? "Sin resultados." : "No hay reportes todavía."}</div>`;
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
  const statusColorHex = statusColor(statusClass);
  let imgHtml = "";
  if (item.photo_url) {
    imgHtml = `<img class="card-photo" src="${escapeHtml(item.photo_url)}" alt="Foto" />`;
  } else if (item.image) {
    imgHtml = `<img class="card-photo" src="${escapeHtml(item.image)}" alt="Imagen" />`;
  } else {
    imgHtml = `<img style="opacity:0.62;" class="card-photo" src="${BUILDING_PLACEHOLDER}" alt="Imagen" />`;
  }
  return `
        <div class="card ${statusClass}" data-id="${item.id}" data-type="building" style="--status-color:${statusColorHex};">
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
      <div class="card" data-id="${item.id}" style="display:flex;flex-direction:column;--status-color:#888;">
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
        <div class="card colaborador" data-id="${item.id}" data-type="colaborador" style="--status-color:${statusColor("colaborador")};">
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
//===== server.js =====
// Tiny emergency backend: Express + SQLite
// Run with systemd. Reports of people, pets, buildings, and announcements.
import express from "express";
import cors from "cors";
import Database from "better-sqlite3";
import crypto from "crypto";
const app = express();
const PORT = process.env.PORT || 3000;
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
    "SELECT id, name, email, password_hash FROM collaborators WHERE LOWER(email) = ? OR LOWER(name) = ?",
  ).get(id, id);
  if (!user || !verifyPassword(password, user.password_hash)) {
    return res.status(401).json({ error: "invalid credentials" });
  }
  const token = createToken(user.id);
  res.json({ token, user: { id: user.id, name: user.name, email: user.email } });
});
app.get("/api/auth/me", requireAuth, (req, res) => {
  const user = db.prepare("SELECT id, name, email, skill, contact, city FROM collaborators WHERE id = ?").get(req.userId);
  if (!user) return res.status(404).json({ error: "not found" });
  res.json(user);
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

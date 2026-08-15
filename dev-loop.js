const puppeteer = require("puppeteer");
const express = require("express");
const chokidar = require("chokidar");
const fs = require("fs");
const path = require("path");
const { execSync } = require("child_process");

const LOG_FILE = path.join(__dirname, "console-errors.log");
const HTML_FILE = path.join(__dirname, "index.html");
const PORT = process.env.PORT || 3001;

let page;
let errorBuffer = [];
let lastReloadTime = 0;
const RELOAD_DEBOUNCE_MS = 800;
let isReloading = false;

async function start() {
  // ---- Kill orphaned Chrome instances from previous runs ----
  try {
    execSync('pkill -f "puppeteer" 2>/dev/null || true');
    execSync('pkill -f "chrome" 2>/dev/null || true');
    await new Promise((r) => setTimeout(r, 600));
  } catch (e) {
    /* ignore */
  }

  // ---- Express server ----
  const app = express();
  app.use(express.json());
  app.use(express.static(__dirname));

  app.post("/report-errors", (req, res) => {
    const { errors } = req.body;
    if (errors && Array.isArray(errors)) {
      errors.forEach((err) => {
        const entry = {
          timestamp: err.time || new Date().toISOString(),
          type: err.type || "error",
          text: err.msg || err.args?.join(" ") || "Unknown error",
          details: err,
        };
        errorBuffer.push(entry);
        logToFile(entry);
        console.log(`[browser] [${entry.type}] ${entry.text}`);
      });
    }
    res.json({ status: "ok", received: errors?.length || 0 });
  });

  app.post("/reload", async (req, res) => {
    const result = await reloadPage({
      resetBuffer: true,
      timeout: req.body.timeout,
    });
    res.json({
      status: result.skipped ? "skipped" : "done",
      errors: result.errors ?? 0,
      hasErrors: (result.errors ?? 0) > 0,
      ...(result.error ? { error: result.error } : {}),
    });
  });

  app.post("/click", async (req, res) => {
    const { selector, type = "click" } = req.body;
    if (!selector) {
      return res.status(400).json({ error: "Missing selector parameter" });
    }
    try {
      await page.waitForSelector(selector, { visible: true, timeout: 5000 });
      if (type === "click") {
        await page.click(selector);
      } else if (type === "dblclick") {
        await page.evaluate((sel) => {
          const el = document.querySelector(sel);
          if (el)
            el.dispatchEvent(new MouseEvent("dblclick", { bubbles: true }));
        }, selector);
      } else if (type === "hover") {
        await page.hover(selector);
      }
      await new Promise((r) => setTimeout(r, 500));
      res.json({
        status: "success",
        action: type,
        selector,
        message: `Successfully performed ${type} on "${selector}"`,
      });
    } catch (err) {
      console.error(`Click failed: ${err.message}`);
      res.status(500).json({
        status: "error",
        error: err.message,
        selector,
      });
    }
  });

  app.post("/type", async (req, res) => {
    const { selector, text, clear = true } = req.body;
    if (!selector || text === undefined) {
      return res
        .status(400)
        .json({ error: "Missing selector or text parameter" });
    }
    try {
      await page.waitForSelector(selector, { visible: true, timeout: 5000 });
      if (clear) {
        await page.$eval(selector, (el) => (el.value = ""));
      }
      await page.type(selector, text);
      await page.evaluate((sel) => {
        const el = document.querySelector(sel);
        if (el) el.dispatchEvent(new Event("input", { bubbles: true }));
      }, selector);
      await new Promise((r) => setTimeout(r, 300));
      res.json({
        status: "success",
        selector,
        text,
        message: `Typed "${text}" into "${selector}"`,
      });
    } catch (err) {
      console.error(`Type failed: ${err.message}`);
      res.status(500).json({
        status: "error",
        error: err.message,
        selector,
      });
    }
  });

  app.get("/element", async (req, res) => {
    const { selector } = req.query;
    if (!selector) {
      return res.status(400).json({ error: "Missing selector parameter" });
    }
    try {
      const info = await page.evaluate((sel) => {
        const el = document.querySelector(sel);
        if (!el) return { found: false };
        const rect = el.getBoundingClientRect();
        return {
          found: true,
          visible: rect.width > 0 && rect.height > 0,
          text: el.textContent?.trim() || "",
          tagName: el.tagName,
          className: el.className,
          id: el.id,
          attributes: Array.from(el.attributes).map((a) => ({
            name: a.name,
            value: a.value,
          })),
          rect: {
            x: rect.left,
            y: rect.top,
            width: rect.width,
            height: rect.height,
          },
        };
      }, selector);
      res.json(info);
    } catch (err) {
      res.status(500).json({
        status: "error",
        error: err.message,
      });
    }
  });

  app.get("/health", (req, res) => {
    res.json({ status: "ok", errors: errorBuffer.length });
  });
  // ---- Autokill port before binding ----
  try {
    execSync(`fuser -k ${PORT}/tcp 2>/dev/null || true`);
    await new Promise((r) => setTimeout(r, 200));
  } catch (e) { /* ignore */ }

  app.listen(PORT, () => {
    console.log(`📡 Dev loop server running on http://localhost:${PORT}`);
    console.log(`📊 Errors logged to: ${LOG_FILE}`);
  });

  // ---- Launch Puppeteer ----
  // Use a persistent profile so DevTools "undocked" preference survives restarts
  const userDataDir = path.join(__dirname, ".chrome-profile");
  const localStatePath = path.join(userDataDir, "Local State");
  try {
    fs.mkdirSync(userDataDir, { recursive: true });
    // Force DevTools to open in a separate (undocked) window
    fs.writeFileSync(
      localStatePath,
      JSON.stringify({
        devtools: { preferences: { currentDockState: '"undocked"' } },
      })
    );
  } catch (e) {
    /* ignore if it already exists or fails */
  }

  console.log("🚀 Launching browser...");
  const browser = await puppeteer.launch({
    headless: false,
    devtools: true, // Open DevTools
    userDataDir, // Persistent profile for DevTools preferences
    defaultViewport: null, // viewport follows actual window size
    args: [
      "--no-sandbox",
      "--disable-setuid-sandbox",
      "--disable-dev-shm-usage",
      "--disable-gpu",
      "--disable-software-rasterizer",
      "--disable-background-timer-throttling",
      "--disable-renderer-backgrounding",
      "--auto-open-devtools-for-tabs", // Force DevTools open on launch
      "--window-size=1280,720",
      "--start-minimized",
    ],
  });
  // Reuse the default blank tab Chrome opens on launch (avoids a leftover empty tab)
  const pages = await browser.pages();
  page = pages.length > 0 ? pages[0] : await browser.newPage();

  page.on("console", (msg) => {
    const text = msg.text();
    if (text.includes("[BS]") || text.includes("browser-sync")) return;
    const entry = {
      timestamp: new Date().toISOString(),
      type: msg.type(),
      text: text,
      location: msg.location(),
    };
    if (msg.type() === "error" || msg.type() === "warn") {
      errorBuffer.push(entry);
      logToFile(entry);
      console.log(`[${msg.type()}] ${text}`);
    }
  });

  page.on("pageerror", (error) => {
    const entry = {
      timestamp: new Date().toISOString(),
      type: "pageerror",
      message: error.message,
      stack: error.stack,
    };
    errorBuffer.push(entry);
    logToFile(entry);
    console.log(`[pageerror] ${error.message}`);
  });

  page.on("requestfailed", (request) => {
    const entry = {
      timestamp: new Date().toISOString(),
      type: "requestfailed",
      url: request.url(),
      errorText: request.failure()?.errorText,
    };
    errorBuffer.push(entry);
    logToFile(entry);
  });

  console.log(`📄 Loading ${HTML_FILE}...`);
  await page.goto(`http://localhost:${PORT}/index.html`, {
    waitUntil: "domcontentloaded", // <-- FIX: no esperar networkidle0
    timeout: 10000,
  });
  console.log("✅ Dev loop ready!");

  // ---- File watcher: solo archivos de TU proyecto ----
  const watcher = chokidar.watch(
    [
      path.join(__dirname, "index.html"),
      path.join(__dirname, "style.css"),
      path.join(__dirname, "vComponents.js"),
      path.join(__dirname, "vStore", "*.js"),
    ],
    {
      ignoreInitial: true,
      awaitWriteFinish: { stabilityThreshold: 400, pollInterval: 100 },
      ignored: /node_modules|dev-loop\.js|console-errors\.log/,
    }
  );

  watcher.on("change", async (filePath) => {
    if (path.basename(filePath) === "dev-loop.js") return;
    console.log(
      `\n📝 File changed: ${path.basename(filePath)} (auto-reloading)`
    );
    const result = await reloadPage({ resetBuffer: true });
    if (result.skipped) {
      console.log("   (debounced — skipped)");
    } else if (result.errors > 0) {
      console.log(`   ⚠️  ${result.errors} error(s) captured`);
    } else {
      console.log("   ✅ No errors detected");
    }
  });

  watcher.on("error", (err) => console.error("Watcher error:", err));

  // ---- Graceful shutdown ----
  async function shutdown() {
    console.log("\n👋 Shutting down...");
    try {
      await browser.close();
    } catch (e) { }
    process.exit(0);
  }

  process.on("SIGINT", shutdown);
  process.on("SIGTERM", shutdown);
  process.on("uncaughtException", (err) => {
    console.error("Uncaught:", err);
    shutdown();
  });
}

function logToFile(entry) {
  const line = `[${entry.timestamp}] [${entry.type}] ${entry.text || entry.message || entry.url
    }\n`;
  fs.appendFileSync(LOG_FILE, line);
}

async function reloadPage({ resetBuffer = true, timeout = 2000 } = {}) {
  if (isReloading) return { skipped: true, reason: "already-reloading" };

  const now = Date.now();
  if (now - lastReloadTime < RELOAD_DEBOUNCE_MS) {
    return { skipped: true };
  }

  isReloading = true;
  lastReloadTime = now;

  if (resetBuffer) {
    errorBuffer = [];
    fs.writeFileSync(LOG_FILE, "");
  }

  try {
    await page.reload({
      waitUntil: "domcontentloaded", // <-- FIX: no bloquear por esm.sh
      timeout: 8000,
    });

    // Wait for errors to surface — event-driven, no polling, no leaks
    const initialErrorCount = errorBuffer.length;
    let handler;
    const errorDetected = new Promise((resolve) => {
      handler = () => {
        if (errorBuffer.length > initialErrorCount) resolve();
      };
      page.on("console", handler);
      page.on("pageerror", handler);
      page.on("requestfailed", handler);
    });
    const timeoutPromise = new Promise((resolve) => setTimeout(resolve, timeout));
    await Promise.race([errorDetected, timeoutPromise]);
    // Cleanup listeners regardless of which promise won
    page.off("console", handler);
    page.off("pageerror", handler);
      page.off("requestfailed", handler);

    return { skipped: false, errors: errorBuffer.length };
  } catch (err) {
    console.error("Reload failed:", err.message);
    logToFile({
      timestamp: new Date().toISOString(),
      type: "reload_error",
      message: err.message,
    });
    return { skipped: false, error: err.message };
  } finally {
    isReloading = false;
  }
}

start().catch((err) => {
  console.error("❌ Failed to start dev loop:", err);
  process.exit(1);
});

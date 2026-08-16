const { Client, LocalAuth } = require("whatsapp-web.js");
const fs = require("fs");
const path = require("path");

const DOWNLOAD_DIR = path.resolve(__dirname, "downloads");
if (!fs.existsSync(DOWNLOAD_DIR)) {
  fs.mkdirSync(DOWNLOAD_DIR, { recursive: true });
}

const client = new Client({
  authStrategy: new LocalAuth(), // saves session, no need to scan again
  puppeteer: { headless: true }, // set to false to see the browser
});

client.on("qr", (qr) => {
  const qrcode = require("qrcode-terminal");
  qrcode.generate(qr, { small: true });
  console.log("Scan the QR code with your WhatsApp mobile app.");
});

client.on("ready", () => {
  console.log("WhatsApp client is ready!");
  console.log(`Media will be saved to: ${DOWNLOAD_DIR}`);
});

client.on("message", async (message) => {
  if (message.hasMedia) {
    try {
      console.log(
        `Downloading media from ${message.author || message.from}...`,
      );
      const media = await message.downloadMedia();
      if (media) {
        const ext = media.mimetype.split("/")[1] || "bin";
        const filename = `${message.id.id}.${ext}`;
        const filepath = path.join(DOWNLOAD_DIR, filename);
        fs.writeFileSync(filepath, media.data, "base64");
        console.log(`Downloaded: ${filename}`);
      }
    } catch (error) {
      console.error(`Failed to download media: ${error.message}`);
    }
  }
});

client.initialize();

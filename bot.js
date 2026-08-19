import {
  MatrixClient,
  SimpleFsStorageProvider,
  RustSdkCryptoStorageProvider,
} from "matrix-bot-sdk";
import fs from "fs";

const MATRIX_HOMESERVER_URL =
  process.env.MATRIX_HOMESERVER_URL || "http://localhost:8008";
const MATRIX_LOCALPART = process.env.MATRIX_LOCALPART || "sismo-bot";
const BOT_PASSWORD = process.env.BOT_PASSWORD;

const STORAGE_DIR = "/opt/sismo-api/.bot-store";
if (!fs.existsSync(STORAGE_DIR)) fs.mkdirSync(STORAGE_DIR, { recursive: true });

const storage = new SimpleFsStorageProvider(`${STORAGE_DIR}/storage.json`);
const crypto = new RustSdkCryptoStorageProvider(`${STORAGE_DIR}/crypto`);

const client = new MatrixClient(MATRIX_HOMESERVER_URL, "", storage, crypto);
const loginRes = await client.loginWithPassword(MATRIX_LOCALPART, BOT_PASSWORD);

console.log("ACCESS_TOKEN:", loginRes.accessToken);
console.log("USER_ID:", loginRes.userId);

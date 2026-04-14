import { createServer } from "node:http";
import { readFile } from "node:fs/promises";
import { createReadStream, existsSync } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const repoRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const siteRoot = path.join(repoRoot, "website");
const port = Number(process.env.AIFRED_SITE_PORT || 4173);

const mime = new Map([
  [".css", "text/css; charset=utf-8"],
  [".html", "text/html; charset=utf-8"],
  [".js", "text/javascript; charset=utf-8"],
  [".json", "application/json; charset=utf-8"],
  [".jpg", "image/jpeg"],
  [".jpeg", "image/jpeg"],
  [".png", "image/png"],
  [".mp3", "audio/mpeg"],
  [".wav", "audio/wav"],
  [".txt", "text/plain; charset=utf-8"]
]);

function sendJson(response, status, body) {
  response.writeHead(status, {
    "content-type": "application/json; charset=utf-8",
    "cache-control": "no-store"
  });
  response.end(JSON.stringify(body));
}

async function readRequestJson(request) {
  const chunks = [];
  for await (const chunk of request) chunks.push(chunk);
  const raw = Buffer.concat(chunks).toString("utf8");
  if (!raw.trim()) return {};
  return JSON.parse(raw);
}

async function routeApi(request, response, url) {
  if (url.pathname === "/api/v1/health") {
    return sendJson(response, 200, { ok: true, service: "AIFRED local website backend" });
  }

  if (url.pathname === "/api/v1/catalog/list") {
    const raw = await readFile(path.join(siteRoot, "assets", "data", "beat_catalog.json"), "utf8");
    return sendJson(response, 200, { ok: true, tracks: JSON.parse(raw) });
  }

  if (url.pathname === "/api/v1/models/list") {
    return sendJson(response, 200, {
      ok: true,
      providers: {
        openai: { configured: Boolean(process.env.OPENAI_API_KEY), model: process.env.OPENAI_MODEL || "" },
        ollama: { configured: Boolean(process.env.OLLAMA_BASE_URL), model: process.env.OLLAMA_MODEL || "" }
      }
    });
  }

  if (url.pathname === "/api/v1/inquiries/submit" && request.method === "POST") {
    const body = await readRequestJson(request);
    if (!body.name || !body.email || !body.message) {
      return sendJson(response, 400, { ok: false, error: "name, email, and message are required" });
    }
    return sendJson(response, 200, { ok: true, inquiry_id: crypto.randomUUID(), target_email: process.env.AIFRED_CONTACT_EMAIL || "north3rnlight3rofficial@outlook.com" });
  }

  if (url.pathname === "/api/v1/chat/ask") {
    return sendJson(response, 503, { ok: false, error: "Use Cloudflare Pages or configure provider calls in the deployed backend for chat." });
  }

  return sendJson(response, 404, { ok: false, error: `unknown route: ${url.pathname}` });
}

function staticPath(urlPath) {
  const decoded = decodeURIComponent(urlPath === "/" ? "/index.html" : urlPath);
  const resolved = path.resolve(siteRoot, `.${decoded}`);
  if (!resolved.startsWith(siteRoot)) return "";
  return resolved;
}

createServer(async (request, response) => {
  try {
    const url = new URL(request.url || "/", `http://${request.headers.host || "localhost"}`);
    if (url.pathname.startsWith("/api/")) return routeApi(request, response, url);

    const filePath = staticPath(url.pathname);
    if (!filePath || !existsSync(filePath)) {
      response.writeHead(404, { "content-type": "text/plain; charset=utf-8" });
      response.end("Not found");
      return;
    }

    response.writeHead(200, { "content-type": mime.get(path.extname(filePath).toLowerCase()) || "application/octet-stream" });
    createReadStream(filePath).pipe(response);
  } catch (error) {
    sendJson(response, 500, { ok: false, error: error.message || "server error" });
  }
}).listen(port, () => {
  console.log(`AIFRED website preview: http://localhost:${port}`);
});

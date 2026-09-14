import http from "node:http";
import { timingSafeEqual } from "node:crypto";

const host = "127.0.0.1";
const port = Number(process.env.AIFRED_OLLAMA_PROXY_PORT || 11435);
const upstream = new URL(process.env.AIFRED_OLLAMA_UPSTREAM || "http://127.0.0.1:11434");
const token = String(process.env.OLLAMA_API_TOKEN || "");
const maxBodyBytes = 2 * 1024 * 1024;
const allowed = new Map([
  ["GET /api/tags", 5_000],
  ["POST /api/show", 10_000],
  ["POST /api/chat", 90_000],
  ["GET /v1/models", 5_000],
  ["POST /v1/chat/completions", 90_000]
]);

if (!Number.isSafeInteger(port) || port < 1024 || port > 65535) throw new Error("AIFRED_OLLAMA_PROXY_PORT must be a valid unprivileged port");
if (upstream.protocol !== "http:" || upstream.hostname !== "127.0.0.1" || upstream.port !== "11434") throw new Error("AIFRED_OLLAMA_UPSTREAM must remain http://127.0.0.1:11434");
if (token.length < 32) throw new Error("OLLAMA_API_TOKEN must be configured with at least 32 characters");

function equalToken(actual, expected) {
  const left = Buffer.from(actual);
  const right = Buffer.from(expected);
  return left.length === right.length && timingSafeEqual(left, right);
}

function send(res, status, payload, extraHeaders = {}) {
  const body = JSON.stringify(payload);
  res.writeHead(status, {
    "content-type": "application/json; charset=utf-8",
    "content-length": Buffer.byteLength(body),
    "cache-control": "no-store",
    "x-content-type-options": "nosniff",
    ...extraHeaders
  });
  res.end(body);
}

const server = http.createServer(async (req, res) => {
  const requestUrl = new URL(req.url || "/", `http://${host}:${port}`);
  if (requestUrl.pathname === "/health" && req.method === "GET") {
    send(res, 200, { ok: true, service: "aifred-ollama-auth-proxy" });
    return;
  }

  const route = `${req.method} ${requestUrl.pathname}`;
  const timeoutMs = allowed.get(route);
  if (!timeoutMs) {
    send(res, 404, { ok: false, error: "route_not_found" });
    return;
  }

  const authorization = String(req.headers.authorization || "");
  const supplied = authorization.toLowerCase().startsWith("bearer ") ? authorization.slice(7).trim() : "";
  if (!equalToken(supplied, token)) {
    send(res, 401, { ok: false, error: "authentication_required" }, { "www-authenticate": "Bearer" });
    return;
  }

  const declaredLength = Number(req.headers["content-length"] || 0);
  if (declaredLength > maxBodyBytes) {
    send(res, 413, { ok: false, error: "body_too_large" });
    req.resume();
    return;
  }

  const controller = new AbortController();
  const timeout = setTimeout(() => controller.abort(), timeoutMs);
  const chunks = [];
  let size = 0;
  try {
    for await (const chunk of req) {
      size += chunk.length;
      if (size > maxBodyBytes) throw Object.assign(new Error("body_too_large"), { status: 413 });
      chunks.push(chunk);
    }
    const headers = new Headers({ accept: req.headers.accept || "application/json" });
    if (req.headers["content-type"]) headers.set("content-type", String(req.headers["content-type"]));
    const response = await fetch(new URL(requestUrl.pathname, upstream), {
      method: req.method,
      headers,
      body: req.method === "GET" ? undefined : Buffer.concat(chunks),
      signal: controller.signal
    });
    const outputHeaders = {
      "content-type": response.headers.get("content-type") || "application/octet-stream",
      "cache-control": "no-store",
      "x-content-type-options": "nosniff"
    };
    res.writeHead(response.status, outputHeaders);
    if (!response.body) {
      res.end();
      return;
    }
    const reader = response.body.getReader();
    while (true) {
      const { done, value } = await reader.read();
      if (done) break;
      if (!res.write(value)) await new Promise((resolve) => res.once("drain", resolve));
    }
    res.end();
  } catch (error) {
    if (!res.headersSent) {
      const status = error?.status === 413 ? 413 : error?.name === "AbortError" ? 504 : 502;
      const code = status === 413 ? "body_too_large" : status === 504 ? "upstream_timeout" : "upstream_unavailable";
      send(res, status, { ok: false, error: code });
    } else {
      res.destroy();
    }
  } finally {
    clearTimeout(timeout);
  }
});

server.requestTimeout = 95_000;
server.headersTimeout = 10_000;
server.keepAliveTimeout = 5_000;
server.listen(port, host, () => {
  console.log(JSON.stringify({ event: "ollama_proxy_started", host, port, upstream: upstream.origin }));
});

for (const signal of ["SIGINT", "SIGTERM"]) {
  process.on(signal, () => server.close(() => process.exit(0)));
}

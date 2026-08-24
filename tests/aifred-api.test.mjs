import assert from "node:assert/strict";
import { createHash } from "node:crypto";
import test from "node:test";
import { onRequest } from "../apps/website/functions/api/v1/[[path]].js";

const request = (url, init = {}) => new Request(`https://aifred.test${url}`, init);
const env = { AIFRED_ADMIN_USERNAME: "operator", AIFRED_ADMIN_PASSWORD_SHA256: "", AIFRED_ADMIN_SESSION_SECRET: "test-secret" };

test("health is outside the v1 contract", async () => {
  const response = await onRequest({ request: request("/health"), env, params: { path: ["health"] } });
  assert.equal(response.status, 200);
  assert.equal((await response.json()).api_version, "v1");
});

test("models uses OpenAI list shape", async () => {
  const response = await onRequest({ request: request("/v1/models"), env: { ...env, OPENAI_API_KEY: "configured", OPENAI_MODEL: "aifred-test" }, params: { path: ["models"] } });
  const payload = await response.json();
  assert.equal(payload.object, "list");
  assert.equal(payload.data[0].id, "aifred-test");
});

test("malformed chat is a structured 400", async () => {
  const response = await onRequest({ request: request("/v1/chat/completions", { method: "POST", body: "{}" }), env, params: { path: ["chat", "completions"] } });
  assert.equal(response.status, 400);
  assert.equal((await response.json()).error.type, "invalid_request_error");
});

test("streaming returns OpenAI-compatible SSE", async () => {
  const originalFetch = globalThis.fetch;
  globalThis.fetch = async () => new Response('{"message":{"content":"hello"}}\n{"done":true}\n', { headers: { "content-type": "application/x-ndjson" } });
  try {
    const response = await onRequest({ request: request("/v1/chat/completions", { method: "POST", body: JSON.stringify({ model: "aifred:latest", messages: [{ role: "user", content: "hi" }], stream: true }) }), env: { ...env, OLLAMA_BASE_URL: "https://ollama.test", OLLAMA_MODEL: "aifred:latest" }, params: { path: ["chat", "completions"] } });
    assert.equal(response.status, 200);
    const body = await response.text();
    assert.match(body, /chat\.completion\.chunk/);
    assert.match(body, /data: \[DONE\]/);
  } finally { globalThis.fetch = originalFetch; }
});

test("admin operations require server-side authorization", async () => {
  const response = await onRequest({ request: request("/api/v1/admin/ops/status"), env, params: { path: ["admin", "ops", "status"] } });
  assert.equal(response.status, 401);
});

test("authorized admin can read operations status", async () => {
  const password = "test-password";
  const configuredEnv = { AIFRED_ADMIN_USERNAME: "operator", AIFRED_ADMIN_PASSWORD_SHA256: createHash("sha256").update(password).digest("hex"), AIFRED_ADMIN_SESSION_SECRET: "test-secret" };
  const login = await onRequest({ request: request("/api/v1/admin/login", { method: "POST", body: JSON.stringify({ username: "operator", password }) }), env: configuredEnv, params: { path: ["admin", "login"] } });
  const session = (await login.json()).session_token;
  const response = await onRequest({ request: request("/api/v1/admin/ops/status", { headers: { authorization: `Bearer ${session}` } }), env: configuredEnv, params: { path: ["admin", "ops", "status"] } });
  assert.equal(response.status, 200);
  assert.equal((await response.json()).service, "AIFRED operations");
});

test("embeddings and future responses are explicit", async () => {
  const embeddings = await onRequest({ request: request("/v1/embeddings", { method: "POST" }), env, params: { path: ["embeddings"] } });
  const responses = await onRequest({ request: request("/v1/responses", { method: "POST" }), env, params: { path: ["responses"] } });
  assert.equal(embeddings.status, 501);
  assert.equal(responses.status, 501);
});

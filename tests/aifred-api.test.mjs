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

test("all admin data routes require server-side authorization", async () => {
  for (const path of ["admin/dashboard/state", "admin/catalog/list", "admin/inquiries/list", "admin/logs/list", "admin/sales/list", "admin/reference/list"]) {
    const response = await onRequest({
      request: request(`/api/v1/${path}`),
      env,
      params: { path: path.split("/") }
    });
    assert.equal(response.status, 401, path);
  }
});

test("active content contract advertises free distribution", async () => {
  const response = await onRequest({ request: request("/api/v1/content/get"), env, params: { path: ["content", "get"] } });
  const payload = await response.json();
  assert.equal(response.status, 200);
  assert.equal(payload.content.products[0].price, "Free beta download");
  assert.match(payload.content.services[1].price, /Free MP3/);
  assert.doesNotMatch(JSON.stringify(payload), /\$5|\$100|\$200/);
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

test("plugin downloads are public and served from the downloads bucket", async () => {
  const stored = [];
  const downloadEnv = {
    ...env,
    AIFRED_RELEASE_VERSION: "v0.3.6-installer-ai-alias",
    AIFRED_SALES_LOG: {
      async put(key, value) { stored.push([key, value]); }
    },
    AIFRED_DOWNLOADS: {
      async get(key) {
        assert.equal(key, "releases/v0.3.6-installer-ai-alias/AIFRED-VST3-windows.zip");
        return {
          body: new TextEncoder().encode("plugin-zip"),
          size: 10,
          httpEtag: '"test-etag"',
          httpMetadata: { contentType: "application/zip" }
        };
      }
    }
  };
  const response = await onRequest({
    request: request("/api/v1/downloads/plugin?asset=zip"),
    env: downloadEnv,
    params: { path: ["downloads", "plugin"] }
  });
  assert.equal(response.status, 200);
  assert.equal(response.headers.get("x-aifred-download-source"), "r2");
  assert.equal(response.headers.get("content-disposition"), 'attachment; filename="AIFRED-VST3-windows.zip"');
  assert.equal(await response.text(), "plugin-zip");
  assert.equal(stored.length, 1);
});

test("setup downloads resolve the installer object without a token", async () => {
  const response = await onRequest({
    request: request("/api/v1/downloads/plugin?asset=setup"),
    env: {
      ...env,
      AIFRED_RELEASE_VERSION: "v0.3.6-installer-ai-alias",
      AIFRED_DOWNLOADS: {
        async get(key) {
          assert.equal(key, "releases/v0.3.6-installer-ai-alias/AIFRED-VST3-Setup.exe");
          return { body: new Uint8Array([77, 90]), size: 2, httpMetadata: { contentType: "application/vnd.microsoft.portable-executable" } };
        }
      }
    },
    params: { path: ["downloads", "plugin"] }
  });
  assert.equal(response.status, 200);
  assert.equal(response.headers.get("content-type"), "application/vnd.microsoft.portable-executable");
  assert.equal(response.headers.get("content-length"), "2");
});

test("macOS downloads resolve the published plugin ZIP", async () => {
  const response = await onRequest({
    request: request("/api/v1/downloads/plugin?asset=macos"),
    env: {
      ...env,
      AIFRED_RELEASE_VERSION: "v0.3.6-installer-ai-alias",
      AIFRED_DOWNLOADS: {
        async get(key) {
          assert.equal(key, "releases/v0.3.6-installer-ai-alias/AIFRED-VST3-macos.zip");
          return { body: new Uint8Array([80, 75]), size: 2, httpMetadata: { contentType: "application/zip" } };
        }
      }
    },
    params: { path: ["downloads", "plugin"] }
  });
  assert.equal(response.status, 200);
  assert.equal(response.headers.get("content-disposition"), 'attachment; filename="AIFRED-VST3-macos.zip"');
});

test("plugin download HEAD returns metadata without a response body", async () => {
  const response = await onRequest({
    request: request("/api/v1/downloads/plugin?asset=zip", { method: "HEAD" }),
    env: {
      ...env,
      AIFRED_RELEASE_VERSION: "v0.3.6-installer-ai-alias",
      AIFRED_DOWNLOADS: {
        async get() { throw new Error("GET should not be used when R2 HEAD is available"); },
        async head(key) {
          assert.equal(key, "releases/v0.3.6-installer-ai-alias/AIFRED-VST3-windows.zip");
          return { size: 34925966, httpMetadata: { contentType: "application/zip" } };
        }
      }
    },
    params: { path: ["downloads", "plugin"] }
  });
  assert.equal(response.status, 200);
  assert.equal(response.headers.get("content-length"), "34925966");
  assert.equal(await response.text(), "");
});

test("plugin download distinguishes missing objects and missing R2 binding", async () => {
  const missing = await onRequest({
    request: request("/api/v1/downloads/plugin?asset=zip"),
    env: { ...env, AIFRED_DOWNLOADS: { async get() { return null; } } },
    params: { path: ["downloads", "plugin"] }
  });
  const unbound = await onRequest({
    request: request("/api/v1/downloads/plugin?asset=zip"),
    env,
    params: { path: ["downloads", "plugin"] }
  });
  assert.equal(missing.status, 404);
  assert.equal(unbound.status, 503);
});

test("beat download requests use attachment headers", async () => {
  const response = await onRequest({
    request: request("/api/v1/assets/audio/catalog/Test%20Beat.mp3?download=1"),
    env: {
      ...env,
      AIFRED_DOWNLOADS: {
        async get(key) {
          assert.equal(key, "assets/audio/catalog/Test Beat.mp3");
          return {
            body: new TextEncoder().encode("beat"),
            size: 4,
            httpMetadata: { contentType: "audio/mpeg" }
          };
        }
      }
    },
    params: { path: ["assets", "audio", "catalog", "Test Beat.mp3"] }
  });
  assert.equal(response.status, 200);
  assert.equal(response.headers.get("content-disposition"), 'attachment; filename="Test Beat.mp3"');
  assert.equal(response.headers.get("content-type"), "audio/mpeg");
});

test("beat streaming supports byte ranges", async () => {
  const response = await onRequest({
    request: request("/api/v1/assets/audio/catalog/Test.mp3", { headers: { range: "bytes=0-3" } }),
    env: {
      ...env,
      AIFRED_DOWNLOADS: {
        async head(key) {
          assert.equal(key, "assets/audio/catalog/Test.mp3");
          return { size: 10, httpMetadata: { contentType: "audio/mpeg" } };
        },
        async get(key, options) {
          assert.equal(key, "assets/audio/catalog/Test.mp3");
          assert.deepEqual(options.range, { offset: 0, length: 4 });
          return {
            body: new TextEncoder().encode("beat"),
            size: 10,
            httpMetadata: { contentType: "audio/mpeg" }
          };
        }
      }
    },
    params: { path: ["assets", "audio", "catalog", "Test.mp3"] }
  });
  assert.equal(response.status, 206);
  assert.equal(response.headers.get("content-range"), "bytes 0-3/10");
  assert.equal(response.headers.get("accept-ranges"), "bytes");
  assert.equal(await response.text(), "beat");
});

test("invalid byte ranges return 416 without reading the object", async () => {
  const response = await onRequest({
    request: request("/api/v1/assets/audio/catalog/Test.mp3", { headers: { range: "bytes=20-30" } }),
    env: {
      ...env,
      AIFRED_DOWNLOADS: {
        async head() { return { size: 10 }; },
        async get() { throw new Error("invalid ranges must not read R2"); }
      }
    },
    params: { path: ["assets", "audio", "catalog", "Test.mp3"] }
  });
  assert.equal(response.status, 416);
  assert.equal(response.headers.get("content-range"), "bytes */10");
});

test("catalog asset paths reject traversal", async () => {
  const response = await onRequest({
    request: request("/api/v1/assets/blocked"),
    env,
    params: { path: ["assets", "..", "private"] }
  });
  assert.equal(response.status, 400);
});

test("public activity never falls back to GitHub writes", async () => {
  const originalFetch = globalThis.fetch;
  globalThis.fetch = async () => { throw new Error("GitHub must not be called"); };
  try {
    const response = await onRequest({
      request: request("/api/v1/activity/record", { method: "POST", body: JSON.stringify({ event_type: "plugin.download.clicked" }) }),
      env: { ...env, GITHUB_TOKEN: "configured-but-read-only-for-activity" },
      params: { path: ["activity", "record"] }
    });
    const payload = await response.json();
    assert.equal(response.status, 200);
    assert.equal(payload.configured, false);
    assert.equal(payload.storage, "unconfigured");
  } finally {
    globalThis.fetch = originalFetch;
  }
});

test("contact inquiries prefer KV and do not write to GitHub", async () => {
  const stored = [];
  const response = await onRequest({
    request: request("/api/v1/inquiries/submit", {
      method: "POST",
      body: JSON.stringify({ name: "Tester", email: "test@example.com", message: "Hello" })
    }),
    env: {
      ...env,
      GITHUB_TOKEN: "configured-but-not-used",
      AIFRED_SALES_LOG: { async put(key, value) { stored.push([key, value]); } }
    },
    params: { path: ["inquiries", "submit"] }
  });
  const payload = await response.json();
  assert.equal(response.status, 200);
  assert.equal(payload.storage, "kv");
  assert.equal(stored.length, 1);
  assert.match(stored[0][0], /^inquiry:/);
});

test("removed PayPal routes are not exposed", async () => {
  const response = await onRequest({
    request: request("/api/v1/paypal/config"),
    env,
    params: { path: ["paypal", "config"] }
  });
  assert.equal(response.status, 404);
});

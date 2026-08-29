import assert from "node:assert/strict";
import { createHash } from "node:crypto";
import { existsSync, readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import test from "node:test";
import { onRequest } from "../apps/website/functions/api/v1/[[path]].js";
import { activityKey, normalizeActivityEvent } from "../apps/website/lib/activity-log.js";

const request = (url, init = {}) => new Request(`https://aifred.test${url}`, init);
const env = { AIFRED_ADMIN_USERNAME: "operator", AIFRED_ADMIN_PASSWORD_SHA256: "", AIFRED_ADMIN_SESSION_SECRET: "test-secret" };

test("activity events use the compact v1 envelope and strip secret fields", () => {
  const event = normalizeActivityEvent({
    event_type: "Download Completed",
    session_id: "anonymous-session",
    request_id: "request-123",
    actor: { type: "anonymous", id: "anonymous-session" },
    source: { surface: "website", route: "/downloads/windows" },
    subject: { type: "download", id: "setup", name: "Windows installer" },
    operation: { action: "download", status: "success", result: "response_resolved" },
    metadata: { artifact: "setup", authorization: "Bearer must-not-survive", nested: { api_key: "must-not-survive", safe: true } }
  }, {
    now: new Date("2026-08-27T12:00:00.000Z"),
    randomUUID: () => "event-123"
  });
  assert.equal(event.event_id, "event-123");
  assert.equal(event.event_type, "download.completed");
  assert.equal(event.timestamp, "2026-08-27T12:00:00.000Z");
  assert.equal(event.metadata.artifact, "setup");
  assert.equal(event.metadata.authorization, undefined);
  assert.deepEqual(event.metadata.nested, { safe: true });
  assert.equal(activityKey(event), "activity:v1:2026-08-27T12:00:00.000Z:download.completed:event-123");
});

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
  for (const path of ["admin/dashboard/state", "admin/catalog/list", "admin/inquiries/list", "admin/logs/list", "admin/sales/list", "admin/reference/list", "admin/api/config", "admin/api/test"]) {
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

test("every catalog track resolves to a checked-in distribution asset", () => {
  const catalog = JSON.parse(readFileSync(new URL("../apps/website/assets/data/beat_catalog.json", import.meta.url), "utf8"));
  assert.equal(catalog.length, 53);
  assert.equal(new Set(catalog.map((track) => track.asset_file_name)).size, catalog.length);
  for (const track of catalog) {
    const fileName = decodeURIComponent(new URL(track.stream_url, "https://aifred.test").pathname.split("/").pop());
    const asset = fileURLToPath(new URL(`../apps/website/assets/audio/catalog/${encodeURIComponent(fileName)}`, import.meta.url));
    assert.equal(existsSync(asset), true, `${track.title}: ${fileName}`);
    assert.match(track.price, /Free MP3 download/);
  }
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

test("admin login remains available when optional KV throttling is quota-limited", async () => {
  const password = "test-password";
  const quotaEnv = {
    AIFRED_ADMIN_USERNAME: "operator",
    AIFRED_ADMIN_PASSWORD_SHA256: createHash("sha256").update(password).digest("hex"),
    AIFRED_ADMIN_SESSION_SECRET: "test-secret",
    AIFRED_SALES_LOG: {
      async get() { throw new Error("KV get() limit exceeded for the day."); },
      async put() { throw new Error("KV put() limit exceeded for the day."); },
      async delete() { throw new Error("KV delete() limit exceeded for the day."); }
    }
  };
  const response = await onRequest({
    request: request("/api/v1/admin/login", { method: "POST", body: JSON.stringify({ username: "operator", password }) }),
    env: quotaEnv,
    params: { path: ["admin", "login"] }
  });
  const body = await response.json();
  assert.equal(response.status, 200);
  assert.equal(body.ok, true);
  assert.ok(body.session_token);
});

test("authorized API configuration is KV-backed, secret-safe, and testable", async () => {
  const password = "test-password";
  const records = new Map();
  const configuredEnv = {
    AIFRED_ADMIN_USERNAME: "operator",
    AIFRED_ADMIN_PASSWORD_SHA256: createHash("sha256").update(password).digest("hex"),
    AIFRED_ADMIN_SESSION_SECRET: "test-secret",
    OLLAMA_ACCESS_CLIENT_ID: "access-client-id",
    OLLAMA_ACCESS_CLIENT_SECRET: "access-client-secret",
    AIFRED_SALES_LOG: {
      async get(key) { return records.get(key) || null; },
      async put(key, value) { records.set(key, value); }
    }
  };
  const login = await onRequest({ request: request("/api/v1/admin/login", { method: "POST", body: JSON.stringify({ username: "operator", password }) }), env: configuredEnv, params: { path: ["admin", "login"] } });
  const session = (await login.json()).session_token;
  const headers = { authorization: `Bearer ${session}`, "content-type": "application/json" };
  const save = await onRequest({
    request: request("/api/v1/admin/api/config", { method: "POST", headers, body: JSON.stringify({ provider: "ollama", ollama_base_url: "https://ollama.example.test", ollama_model: "aifred:latest", openai_model: "gpt-test" }) }),
    env: configuredEnv,
    params: { path: ["admin", "api", "config"] }
  });
  const saved = await save.json();
  assert.equal(save.status, 200);
  assert.equal(saved.config.ollama.base_url, "https://ollama.example.test");
  assert.equal(saved.config.secret_values_exposed, false);
  assert.doesNotMatch(JSON.stringify(saved), /api_key|client_secret/i);

  const rejected = await onRequest({
    request: request("/api/v1/admin/api/config", { method: "POST", headers, body: JSON.stringify({ provider: "ollama", ollama_base_url: "http://127.0.0.1:11434", ollama_model: "aifred:latest", openai_model: "gpt-test" }) }),
    env: configuredEnv,
    params: { path: ["admin", "api", "config"] }
  });
  assert.equal(rejected.status, 400);

  const originalFetch = globalThis.fetch;
  globalThis.fetch = async (url, init = {}) => {
    assert.equal(String(url), "https://ollama.example.test/api/tags");
    assert.equal(init.headers["CF-Access-Client-Id"], "access-client-id");
    assert.equal(init.headers["CF-Access-Client-Secret"], "access-client-secret");
    return new Response(JSON.stringify({ models: [{ name: "aifred:latest" }] }), { headers: { "content-type": "application/json" } });
  };
  try {
    const tested = await onRequest({
      request: request("/api/v1/admin/api/test", { method: "POST", headers, body: JSON.stringify({ provider: "ollama" }) }),
      env: configuredEnv,
      params: { path: ["admin", "api", "test"] }
    });
    assert.equal(tested.status, 200);
    assert.deepEqual((await tested.json()).models, ["aifred:latest"]);

    globalThis.fetch = async (url, init = {}) => {
      assert.equal(String(url), "https://ollama.example.test/api/chat");
      assert.equal(init.headers["CF-Access-Client-Id"], "access-client-id");
      assert.equal(init.headers["CF-Access-Client-Secret"], "access-client-secret");
      return new Response(JSON.stringify({ message: { content: "runtime ollama ready" } }), { headers: { "content-type": "application/json" } });
    };
    const chat = await onRequest({
      request: request("/api/v1/chat/ask", { method: "POST", body: JSON.stringify({ model: "aifred:latest", messages: [{ role: "user", content: "status" }] }) }),
      env: configuredEnv,
      params: { path: ["chat", "ask"] }
    });
    assert.equal(chat.status, 200);
    assert.equal((await chat.json()).choices[0].message.content, "runtime ollama ready");
  } finally {
    globalThis.fetch = originalFetch;
  }
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
    request: request("/api/v1/downloads/plugin?asset=zip&sid=session-123&rid=request-123&surface=website.downloads"),
    env: downloadEnv,
    params: { path: ["downloads", "plugin"] }
  });
  assert.equal(response.status, 200);
  assert.equal(response.headers.get("x-aifred-download-source"), "r2");
  assert.equal(response.headers.get("content-disposition"), 'attachment; filename="AIFRED-VST3-windows.zip"');
  assert.equal(await response.text(), "plugin-zip");
  assert.equal(response.headers.get("x-aifred-request-id"), "request-123");
  assert.equal(stored.length, 2);
  assert.match(stored[0][0], /^activity:v1:/);
  const events = stored.map(([, value]) => JSON.parse(value));
  assert.deepEqual(events.map((event) => event.event_type), ["download.requested", "download.completed"]);
  assert.deepEqual(events.map((event) => event.request_id), ["request-123", "request-123"]);
  assert.deepEqual(events.map((event) => event.session_id), ["session-123", "session-123"]);
  assert.equal(events[1].operation.result, "response_resolved");
  assert.equal(events[1].metadata.object_key, "releases/v0.3.6-installer-ai-alias/AIFRED-VST3-windows.zip");
});

test("activity logging failure never breaks a successful plugin download", async () => {
  const response = await onRequest({
    request: request("/api/v1/downloads/plugin?asset=zip"),
    env: {
      ...env,
      AIFRED_RELEASE_VERSION: "v0.3.6-installer-ai-alias",
      AIFRED_SALES_LOG: { async put() { throw new Error("KV unavailable"); } },
      AIFRED_DOWNLOADS: {
        async get() {
          return { body: new TextEncoder().encode("plugin-zip"), size: 10, httpMetadata: { contentType: "application/zip" } };
        }
      }
    },
    params: { path: ["downloads", "plugin"] }
  });
  assert.equal(response.status, 200);
  assert.equal(await response.text(), "plugin-zip");
});

test("failed plugin resolution emits requested and failed events with one correlation id", async () => {
  const stored = [];
  const response = await onRequest({
    request: request("/api/v1/downloads/plugin?asset=macos&rid=failed-request&sid=failed-session"),
    env: {
      ...env,
      AIFRED_SALES_LOG: { async put(_key, value) { stored.push(JSON.parse(value)); } },
      AIFRED_DOWNLOADS: { async get() { return null; } }
    },
    params: { path: ["downloads", "plugin"] }
  });
  assert.equal(response.status, 404);
  assert.deepEqual(stored.map((event) => event.event_type), ["download.requested", "download.failed"]);
  assert.deepEqual(stored.map((event) => event.request_id), ["failed-request", "failed-request"]);
  assert.equal(stored[1].operation.result, "http_404");
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
  const activity = [];
  const response = await onRequest({
    request: request("/api/v1/assets/audio/catalog/Test%20Beat.mp3?download=1&sid=catalog-session&rid=catalog-request&surface=catalog"),
    env: {
      ...env,
      AIFRED_SALES_LOG: { async put(key, value) { activity.push([key, JSON.parse(value)]); } },
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
  assert.equal(response.headers.get("x-aifred-request-id"), "catalog-request");
  assert.deepEqual(activity.map(([, event]) => event.event_type), ["download.requested", "download.completed"]);
  assert.equal(activity[1][1].subject.type, "track");
  assert.equal(activity[1][1].metadata.response_source, "r2");
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
      body: JSON.stringify({ name: "Tester", email: "test@example.com", message: "Hello", session_id: "inquiry-session", request_id: "inquiry-request" })
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
  assert.equal(stored.length, 2);
  assert.match(stored[0][0], /^inquiry:/);
  assert.match(stored[1][0], /^activity:v1:/);
  const inquiryEvent = JSON.parse(stored[1][1]);
  assert.equal(inquiryEvent.event_type, "inquiry.submitted");
  assert.equal(inquiryEvent.request_id, "inquiry-request");
  assert.equal(inquiryEvent.subject.id, payload.inquiry_id);
  assert.doesNotMatch(JSON.stringify(inquiryEvent), /test@example\.com|Hello/);
});

test("public activity cannot forge server-confirmed or admin events", async () => {
  for (const event_type of ["download.completed", "inquiry.submitted", "admin.catalog.updated"]) {
    const response = await onRequest({
      request: request("/api/v1/activity/record", { method: "POST", body: JSON.stringify({ event_type }) }),
      env,
      params: { path: ["activity", "record"] }
    });
    assert.equal(response.status, 400, event_type);
  }
});

test("admin commands log resolved allowlist operations without raw command bodies", async () => {
  const password = "test-password";
  const stored = [];
  const configuredEnv = {
    AIFRED_ADMIN_USERNAME: "operator",
    AIFRED_ADMIN_PASSWORD_SHA256: createHash("sha256").update(password).digest("hex"),
    AIFRED_ADMIN_SESSION_SECRET: "test-secret",
    AIFRED_SALES_LOG: { async put(_key, value) { stored.push(JSON.parse(value)); } }
  };
  const login = await onRequest({
    request: request("/api/v1/admin/login", { method: "POST", body: JSON.stringify({ username: "operator", password }) }),
    env: configuredEnv,
    params: { path: ["admin", "login"] }
  });
  const session = (await login.json()).session_token;
  const response = await onRequest({
    request: request("/api/v1/command/run", {
      method: "POST",
      headers: { authorization: `Bearer ${session}`, "content-type": "application/json" },
      body: JSON.stringify({ command: "action:health" })
    }),
    env: configuredEnv,
    params: { path: ["command", "run"] }
  });
  assert.equal(response.status, 200);
  const event = stored.find((entry) => entry.event_type === "admin.operation.completed");
  assert.equal(event.actor.id, "operator");
  assert.equal(event.operation.result, "allowlist_action_completed");
  assert.doesNotMatch(JSON.stringify(event), /command_line|action:health|authorization|password/i);
});

test("removed PayPal routes are not exposed", async () => {
  const response = await onRequest({
    request: request("/api/v1/paypal/config"),
    env,
    params: { path: ["paypal", "config"] }
  });
  assert.equal(response.status, 404);
});

test("admin exports are authenticated, versioned, UTC, secret-safe, and registered", async () => {
  const password = "test-password";
  const records = new Map();
  const exportEnv = {
    AIFRED_ADMIN_USERNAME: "operator",
    AIFRED_ADMIN_PASSWORD_SHA256: createHash("sha256").update(password).digest("hex"),
    AIFRED_ADMIN_SESSION_SECRET: "test-secret",
    AIFRED_SALES_LOG: {
      async get(key) { return records.get(key) || null; },
      async put(key, value) { records.set(key, value); },
      async delete(key) { records.delete(key); },
      async list() { return { keys: [], list_complete: true }; }
    },
    AIFRED_REFERENCE_POOL: { async list() { return { keys: [] }; }, async get() { return null; } }
  };
  const unauthorized = await onRequest({ request: request("/api/v1/admin/export/site"), env: exportEnv, params: { path: ["admin", "export", "site"] } });
  assert.equal(unauthorized.status, 401);
  const login = await onRequest({ request: request("/api/v1/admin/login", { method: "POST", body: JSON.stringify({ username: "operator", password }) }), env: exportEnv, params: { path: ["admin", "login"] } });
  const token = (await login.json()).session_token;
  const headers = { authorization: `Bearer ${token}` };
  const originalFetch = globalThis.fetch;
  globalThis.fetch = async () => new Response("[]", { headers: { "content-type": "application/json" } });
  try {
    for (const [route, type] of [["site", "aifred.site-data"], ["tracks", "aifred.track-analysis"]]) {
      const response = await onRequest({ request: request(`/api/v1/admin/export/${route}`, { headers }), env: exportEnv, params: { path: ["admin", "export", route] } });
      assert.equal(response.status, 200);
      assert.equal(response.headers.get("cache-control"), "no-store");
      assert.match(response.headers.get("content-disposition"), /^attachment; filename="aifred-/);
      const payload = await response.json();
      assert.equal(payload.schemaVersion, "1.0.0");
      assert.equal(payload.exportType, type);
      assert.equal(payload.timeZone, "UTC");
      assert.doesNotMatch(JSON.stringify(payload), /test-secret|authorization/i);
    }
    const registry = await onRequest({ request: request("/api/v1/registry/actions"), env: exportEnv, params: { path: ["registry", "actions"] } });
    const registryActions = (await registry.json()).actions;
    const actionIds = registryActions.map((action) => action.id);
    assert.ok(actionIds.includes("export:site"));
    assert.ok(actionIds.includes("export:tracks"));
    assert.ok(actionIds.includes("help"));
    const helpResponse = await onRequest({ request: request("/api/v1/command/run", { method: "POST", headers: { ...headers, "content-type": "application/json" }, body: JSON.stringify({ command_line: "help" }) }), env: exportEnv, params: { path: ["command", "run"] } });
    assert.equal(helpResponse.status, 200);
    assert.deepEqual(JSON.parse((await helpResponse.json()).stdout), registryActions);
  } finally { globalThis.fetch = originalFetch; }
});

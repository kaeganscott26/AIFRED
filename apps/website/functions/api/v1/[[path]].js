import {
  activityEventId,
  activityTimestamp,
  normalizeActivityType,
  recordActivity
} from "../../../lib/activity-log.js";
import { BackendAdminActions } from "../../../lib/admin-command-registry.js";

const json = (body, init = {}) =>
  new Response(JSON.stringify(body), {
    ...init,
    headers: {
      "content-type": "application/json; charset=utf-8",
      "cache-control": "no-store",
      ...(init.headers || {})
    }
  });

async function readJson(request) {
  try {
    return await request.json();
  } catch (_) {
    return {};
  }
}

function base64Url(bytes) {
  return btoa(String.fromCharCode(...bytes))
    .replace(/\+/g, "-")
    .replace(/\//g, "_")
    .replace(/=+$/, "");
}

async function sha256Hex(input) {
  const bytes = new TextEncoder().encode(input);
  const hash = await crypto.subtle.digest("SHA-256", bytes);
  return [...new Uint8Array(hash)].map((byte) => byte.toString(16).padStart(2, "0")).join("");
}

function constantTimeEqual(left, right) {
  const a = new TextEncoder().encode(String(left));
  const b = new TextEncoder().encode(String(right));
  let difference = a.length ^ b.length;
  const length = Math.max(a.length, b.length);
  for (let index = 0; index < length; index += 1) difference |= (a[index] || 0) ^ (b[index] || 0);
  return difference === 0;
}

function getExpectedAdmin(env) {
  return {
    username: String(env.AIFRED_ADMIN_USERNAME || "").trim(),
    passwordHash: String(env.AIFRED_ADMIN_PASSWORD_SHA256 || "").trim().toLowerCase()
  };
}

async function createAdminSession(username, env) {
  const secret = String(env.AIFRED_ADMIN_SESSION_SECRET || "").trim();
  if (!secret) throw new Error("AIFRED_ADMIN_SESSION_SECRET is not configured");
  const issuedAt = Date.now();
  const nonce = crypto.randomUUID();
  const payload = `${username}|${issuedAt}|${nonce}`;
  const sig = await sha256Hex(`${payload}|${secret}`);
  return base64Url(new TextEncoder().encode(`${payload}|${sig}`));
}

async function verifyAdmin(request, env) {
  const auth = request.headers.get("authorization") || "";
  const token = auth.toLowerCase().startsWith("bearer ") ? auth.slice(7).trim() : "";
  if (!token) return false;
  try {
    const decoded = atob(token.replace(/-/g, "+").replace(/_/g, "/"));
    const parts = decoded.split("|");
    if (parts.length !== 4) return false;
    const [username, issuedAt, nonce, sig] = parts;
    if (!username || !issuedAt || !nonce || !sig) return false;
    const ageMs = Date.now() - Number(issuedAt);
    if (!Number.isFinite(ageMs) || ageMs < 0 || ageMs > 24 * 60 * 60 * 1000) return false;
    const secret = String(env.AIFRED_ADMIN_SESSION_SECRET || "").trim();
    if (!secret) return false;
    const expected = await sha256Hex(`${username}|${issuedAt}|${nonce}|${secret}`);
    return constantTimeEqual(expected, sig);
  } catch (_) {
    return false;
  }
}

async function loadCatalog(request) {
  const response = await fetch(new URL("/assets/data/beat_catalog.json", request.url), { cache: "no-store" });
  const tracks = await response.json();
  return Array.isArray(tracks) ? tracks : [];
}

function websiteAssetUrl(request, relPath) {
  const safePath = safeRepoPath(relPath);
  const url = new URL(`/api/v1/assets/${safePath}`, request.url);
  return url.pathname + url.search;
}

function withR2CatalogUrls(request, tracks) {
  return tracks.map((track) => {
    const fileName = String(track.asset_file_name || track.file_name || "")
      .replace(/\\/g, "/")
      .split("/")
      .pop()
      .trim();
    if (!fileName) return track;
    const encodedName = encodeURIComponent(fileName).replace(/%2F/gi, "/");
    const streamUrl = websiteAssetUrl(request, `audio/catalog/${encodedName}`);
    return {
      ...track,
      public_url: streamUrl,
      full_song_url: streamUrl,
      stream_url: streamUrl
    };
  });
}

function bandScore(value, idealMin, idealMax, acceptMin, acceptMax) {
  if (value >= idealMin && value <= idealMax) return 100;
  if (value < acceptMin || value > acceptMax) return 0;
  if (value < idealMin) return Math.round(((value - acceptMin) / Math.max(0.0001, idealMin - acceptMin)) * 100);
  return Math.round(((acceptMax - value) / Math.max(0.0001, acceptMax - idealMax)) * 100);
}

function floorScore(value, idealMin, acceptMin) {
  if (value >= idealMin) return 100;
  if (value <= acceptMin) return 0;
  return Math.round(((value - acceptMin) / Math.max(0.0001, idealMin - acceptMin)) * 100);
}

function ceilingScore(value, idealMax, acceptMax) {
  if (value <= idealMax) return 100;
  if (value > acceptMax) return 0;
  return Math.round(((acceptMax - value) / Math.max(0.0001, acceptMax - idealMax)) * 100);
}

function proGate(metrics) {
  const checks = [
    {
      id: "integrated_lufs",
      score: bandScore(metrics.integrated_lufs, -16.0, -7.0, -24.0, -3.0),
      target: "Loudness review lane: -24 to -3 LUFS"
    },
    {
      id: "peak_dbfs",
      score: ceilingScore(metrics.peak_dbfs, -0.7, 0.05),
      target: "Peak ceiling: around -1 dBFS target, accepted to 0 dBFS when not clipping"
    },
    {
      id: "tone_balance",
      score: bandScore(metrics.tone_balance, 30, 100, 8, 100),
      target: "Tone balance: rejects only broken ranges"
    },
    {
      id: "crest_factor_db",
      score: bandScore(metrics.crest_factor_db, 2.0, 22.0, 0.5, 28.0),
      target: "Dynamics: wide crest range"
    },
    {
      id: "stereo_width",
      score: bandScore(metrics.stereo_width, 0.12, 1.0, 0.0, 1.0),
      target: "Stereo width: broad acceptance; mono-safe and wide records can both pass"
    },
    {
      id: "low_end_control",
      score: floorScore(metrics.low_end_control, 18, 3),
      target: "Low-end control: reject only severe mud"
    },
    {
      id: "harshness_control",
      score: floorScore(metrics.harshness_control, 16, 3),
      target: "Harshness control: reject only severe upper-mid failure"
    }
  ];
  const weights = {
    integrated_lufs: 0.24,
    peak_dbfs: 0.18,
    tone_balance: 0.18,
    crest_factor_db: 0.14,
    stereo_width: 0.12,
    low_end_control: 0.07,
    harshness_control: 0.07
  };
  const score = Math.round(checks.reduce((sum, check) => sum + check.score * (weights[check.id] || 0), 0));
  const clipping = metrics.peak_dbfs > 0.05;
  const invalid = !Number.isFinite(metrics.integrated_lufs) || !Number.isFinite(metrics.peak_dbfs);
  const severeToneFailure = metrics.tone_balance < 8 || metrics.low_end_control < 3 || metrics.harshness_control < 3;
  const proLoudnessLane = metrics.integrated_lufs >= -24.0 && metrics.integrated_lufs <= -3.0;
  const proPeakLane = metrics.peak_dbfs <= 0.0;
  const noSevereToneFailure = metrics.tone_balance >= 8 && metrics.low_end_control >= 3 && metrics.harshness_control >= 3;
  const essentialPass = proLoudnessLane && proPeakLane && noSevereToneFailure;
  let classification = "Poor Reference";
  let referenceUtility = Math.max(0, Math.min(100, score));
  let technicalCaution = 100 - Math.min(checks.find((check) => check.id === "peak_dbfs")?.score || 0, checks.find((check) => check.id === "harshness_control")?.score || 0);
  let styleTag = metrics.integrated_lufs > -8.0 ? "modern-hot" : metrics.stereo_width > 0.75 ? "wide" : metrics.crest_factor_db < 8.0 ? "dense-limited" : "balanced";
  let bestUse = "Use only as a cautionary comparison.";
  let caution = "Several measured values sit outside the useful reference lane.";

  if (invalid || (clipping && severeToneFailure)) {
    classification = "Reject";
    referenceUtility = 0;
    technicalCaution = 100;
    bestUse = "Do not use this material as a reference.";
    caution = "The file is analytically invalid, clipped with severe balance failure, or otherwise unusable.";
  } else if (clipping || metrics.peak_dbfs > -0.3 || metrics.integrated_lufs > -7.0) {
    classification = score >= 30 && noSevereToneFailure ? "Technically Hot Reference" : "Poor Reference";
    bestUse = "Useful for modern loudness, density, and competitive ceiling behavior.";
    caution = "Treat peak and limiter behavior as a caution, not a default rejection.";
  } else if (score >= 78) {
    classification = "Strong Reference";
    bestUse = "Useful for broad tone, loudness, dynamics, and stereo alignment.";
    caution = "No major technical caution detected.";
  } else if (score >= 56 || essentialPass) {
    classification = "Usable Reference";
    bestUse = "Useful for comparison after checking style and section context.";
    caution = "Some dimensions are outside the center lane but remain analytically useful.";
  } else if (score >= 36 && noSevereToneFailure) {
    classification = "Style-Specific Reference";
    bestUse = "Useful when the target intentionally matches this style tag.";
    caution = "Do not average this against unrelated genres without tagging it.";
  }

  return {
    accepted: classification !== "Reject" && classification !== "Poor Reference",
    score,
    classification,
    reference_utility: referenceUtility,
    technical_caution: technicalCaution,
    style_tag: styleTag,
    best_use: bestUse,
    caution,
    why: `${classification}: score ${score}/100, loudness ${metrics.integrated_lufs} LUFS, peak ${metrics.peak_dbfs} dBFS, crest ${metrics.crest_factor_db} dB, width ${metrics.stereo_width}.`,
    checks: checks.map((check) => ({ id: check.id, ok: check.score >= 20, score: check.score, target: check.target }))
  };
}

async function handleAnalysisSubmit(request, env) {
  const body = await readJson(request);
  const metrics = {
    tone_balance: Number(body.metrics?.tone_balance || 0),
    integrated_lufs: Number(body.metrics?.integrated_lufs || -99),
    peak_dbfs: Number(body.metrics?.peak_dbfs || 99),
    crest_factor_db: Number(body.metrics?.crest_factor_db || 0),
    stereo_width: Number(body.metrics?.stereo_width || 0),
    low_end_control: Number(body.metrics?.low_end_control || 0),
    harshness_control: Number(body.metrics?.harshness_control || 0),
    spectral_centroid_hz: Number(body.metrics?.spectral_centroid_hz || 0)
  };
  const gate = proGate(metrics);
  const analysisId = crypto.randomUUID();
  const metadata = {
    id: analysisId,
    created_at: new Date().toISOString(),
    file_name: String(body.file_name || "browser-analysis").slice(0, 180),
    duration_seconds: Number(body.duration_seconds || 0),
    metrics,
    gate
  };

  let persistence = "disposed";
  if (gate.accepted && env.AIFRED_REFERENCE_POOL) {
    await persistReferenceRecord(env, metadata);
    persistence = "stored";
  } else if (gate.accepted) {
    persistence = "accepted-no-binding";
  }
  await recordActivity(env, {
    event_type: "analysis.submitted",
    session_id: String(body.session_id || "").trim(),
    request_id: String(body.request_id || "").trim(),
    actor: { type: "anonymous", id: String(body.session_id || "").trim() },
    source: { surface: "website.analysis", route: "/api/v1/analysis/submit" },
    subject: { type: "analysis", id: analysisId, name: String(body.file_name || "browser-analysis").trim() },
    operation: { action: "analyze", status: "success", result: gate.accepted ? "accepted" : "rejected" },
    metadata: {
      accepted: gate.accepted,
      score: gate.score,
      classification: gate.classification,
      persistence
    }
  }, { request });

  return json({
    ok: true,
    accepted: gate.accepted,
    score: gate.score,
    classification: gate.classification,
    reference_utility: gate.reference_utility,
    technical_caution: gate.technical_caution,
    style_tag: gate.style_tag,
    best_use: gate.best_use,
    caution: gate.caution,
    why: gate.why,
    action: gate.accepted ? "metadata eligible for the AIFRED reference pool" : "metadata rejected or kept out of the pool",
    persistence,
    checks: gate.checks,
    analysis_id: gate.accepted ? analysisId : null
  });
}

async function handleActivityRecord(request, env) {
  const body = await readJson(request);
  const requestedType = normalizeActivityType(body.event_type || body.type || body.kind || "site.event");
  const publicTypes = new Set([
    "website.page.view",
    "website.resource.clicked",
    "catalog.loaded",
    "catalog.playback.started",
    "catalog.playback.failed",
    "download.clicked",
    "analysis.failed",
    "inquiry.fallback.opened",
    "website.catalog.loaded",
    "catalog.play.clicked",
    "catalog.download.clicked",
    "plugin.download.clicked",
    "website.download.clicked",
    "website.analysis.submitted",
    "website.analysis.failed",
    "website.inquiry.submit",
    "website.inquiry.completed",
    "website.inquiry.fallback"
  ]);
  if (!publicTypes.has(requestedType)) {
    return json({ ok: false, error: "event type is not accepted from the public activity endpoint" }, { status: 400 });
  }
  const suppliedSurface = String(body.source?.surface || body.source || "website").trim();
  const record = {
    event_type: requestedType,
    session_id: String(body.session_id || body.client_session_id || "").trim(),
    request_id: String(body.request_id || "").trim(),
    actor: { type: "anonymous", id: String(body.session_id || body.client_session_id || "").trim() },
    source: {
      surface: suppliedSurface.toLowerCase().startsWith("admin") ? "website" : suppliedSurface,
      route: String(body.source?.route || body.path || "").trim(),
      referrer: String(body.source?.referrer || body.referrer || "").trim()
    },
    subject: body.subject || {},
    operation: body.operation || {},
    metadata: body.metadata || body.details || {}
  };
  const stored = await recordActivity(env, record, { request });
  return json({
    ok: true,
    event_type: normalizeActivityType(record.event_type),
    event_id: stored.event.event_id,
    request_id: stored.event.request_id,
    storage: stored.storage,
    configured: Boolean(env.AIFRED_SALES_LOG)
  });
}

async function persistReferenceRecord(env, metadata) {
  if (env.AIFRED_REFERENCE_POOL && typeof env.AIFRED_REFERENCE_POOL.put === "function") {
    await env.AIFRED_REFERENCE_POOL.put(`reference:${metadata.id}`, JSON.stringify(metadata));
  }
  if (env.AIFRED_REFERENCE_BUCKET && typeof env.AIFRED_REFERENCE_BUCKET.put === "function") {
    await env.AIFRED_REFERENCE_BUCKET.put(
      `reference-pool/metadata/${metadata.id}.json`,
      JSON.stringify(metadata, null, 2),
      { httpMetadata: { contentType: "application/json; charset=utf-8" } }
    );
  }
}

async function listReferenceRecords(env, limit = 100) {
  if (!env.AIFRED_REFERENCE_POOL || typeof env.AIFRED_REFERENCE_POOL.list !== "function") return [];
  const listed = await env.AIFRED_REFERENCE_POOL.list({ prefix: "reference:", limit });
  const records = [];
  for (const key of listed.keys || []) {
    const raw = await env.AIFRED_REFERENCE_POOL.get(key.name);
    if (!raw) continue;
    try {
      records.push(JSON.parse(raw));
    } catch (_) {}
  }
  return records.sort((a, b) => String(b.created_at || "").localeCompare(String(a.created_at || "")));
}

function contentPayload() {
  return {
    products: [
      {
        sku: "aifred_vst3_windows",
        title: "AIFRED VST3 for Windows",
        description: "A visual mix tool built to help you understand what your mix is actually doing.",
        price: "Free beta download",
        availability_label: "Free beta distribution.",
        future_price_label: "No checkout, subscription, or payment is required."
      }
    ],
    services: [
      {
        title: "Mixing and Mastering",
        description: "Pay for quality, not for time.",
        price: "Project pricing by inquiry"
      },
      {
        title: "Beat Licensing",
        description: "Catalog MP3 downloads are free. Contact North3rnLight3r for commercial licensing or custom production.",
        price: "Free MP3 download; licensing by inquiry"
      },
      {
        title: "AIFRED VST",
        description: "Visual feedback for tone, width, loudness, punch, reference alignment, and compare workflow.",
        price: "Free beta download"
      }
    ]
  };
}

const API_RUNTIME_CONFIG_KEY = "admin:config:api-runtime";

function defaultRuntimeApiConfig(env) {
  return {
    provider: String(env.AIFRED_CHAT_PROVIDER || (env.OPENAI_API_KEY ? "openai" : env.OLLAMA_BASE_URL ? "ollama" : "website")).toLowerCase(),
    ollama_base_url: String(env.OLLAMA_BASE_URL || "").replace(/\/+$/, ""),
    ollama_model: String(env.OLLAMA_MODEL || "aifred:latest"),
    openai_model: String(env.OPENAI_MODEL || "gpt-5.6-luna")
  };
}

async function loadRuntimeApiConfig(env) {
  const defaults = defaultRuntimeApiConfig(env);
  if (!env.AIFRED_SALES_LOG || typeof env.AIFRED_SALES_LOG.get !== "function") return defaults;
  try {
    const raw = await env.AIFRED_SALES_LOG.get(API_RUNTIME_CONFIG_KEY);
    if (!raw) return defaults;
    const stored = JSON.parse(raw);
    return {
      provider: ["website", "ollama", "openai"].includes(String(stored.provider || "").toLowerCase()) ? String(stored.provider).toLowerCase() : defaults.provider,
      ollama_base_url: String(stored.ollama_base_url || defaults.ollama_base_url).replace(/\/+$/, ""),
      ollama_model: String(stored.ollama_model || defaults.ollama_model),
      openai_model: String(stored.openai_model || defaults.openai_model)
    };
  } catch (_) {
    return defaults;
  }
}

async function runtimeChatEnv(env) {
  const config = await loadRuntimeApiConfig(env);
  return {
    ...env,
    AIFRED_CHAT_PROVIDER: config.provider,
    OLLAMA_BASE_URL: config.ollama_base_url,
    OLLAMA_MODEL: config.ollama_model,
    OPENAI_MODEL: config.openai_model
  };
}

function validateCloudflareOllamaUrl(value) {
  if (!value) return "";
  try {
    const url = new URL(value);
    if (url.protocol !== "https:") return "Cloudflare Ollama endpoints must use HTTPS";
    const host = url.hostname.toLowerCase();
    if (host === "localhost" || host === "127.0.0.1" || host === "::1" || host.endsWith(".local")) {
      return "Cloudflare cannot reach a device-local Ollama endpoint";
    }
    return "";
  } catch (_) {
    return "Ollama endpoint must be a valid HTTPS URL";
  }
}

function safeRuntimeApiPayload(config, env) {
  return {
    provider: config.provider,
    ollama: {
      base_url: config.ollama_base_url,
      model: config.ollama_model,
      configured: Boolean(config.ollama_base_url),
      token_configured: Boolean(env.OLLAMA_API_TOKEN),
      access_service_token_configured: Boolean(env.OLLAMA_ACCESS_CLIENT_ID && env.OLLAMA_ACCESS_CLIENT_SECRET)
    },
    openai: {
      base_url: "https://api.openai.com/v1",
      model: config.openai_model,
      configured: Boolean(env.OPENAI_API_KEY)
    },
    persistence: env.AIFRED_SALES_LOG ? "Cloudflare KV" : "environment defaults only",
    secret_values_exposed: false
  };
}

async function handleAdminApiConfig(request, env) {
  const config = await loadRuntimeApiConfig(env);
  if (request.method === "GET") return json({ ok: true, config: safeRuntimeApiPayload(config, env) });
  if (request.method !== "POST") return json({ ok: false, error: "method not allowed" }, { status: 405 });
  if (!env.AIFRED_SALES_LOG || typeof env.AIFRED_SALES_LOG.put !== "function") {
    return json({ ok: false, error: "API configuration persistence requires AIFRED_SALES_LOG KV" }, { status: 503 });
  }
  const body = await readJson(request);
  const has = (key) => Object.prototype.hasOwnProperty.call(body, key);
  const next = {
    provider: String(body.provider || config.provider).trim().toLowerCase(),
    ollama_base_url: String(has("ollama_base_url") ? body.ollama_base_url : config.ollama_base_url).trim().replace(/\/+$/, ""),
    ollama_model: String(has("ollama_model") ? body.ollama_model : config.ollama_model).trim(),
    openai_model: String(has("openai_model") ? body.openai_model : config.openai_model).trim()
  };
  if (!["website", "ollama", "openai"].includes(next.provider)) {
    return json({ ok: false, error: "provider must be website, ollama, or openai" }, { status: 400 });
  }
  const ollamaUrlError = validateCloudflareOllamaUrl(next.ollama_base_url);
  if (ollamaUrlError) return json({ ok: false, error: ollamaUrlError }, { status: 400 });
  if (next.provider === "ollama" && !next.ollama_base_url) {
    return json({ ok: false, error: "A reachable HTTPS Ollama endpoint is required" }, { status: 400 });
  }
  if (!next.ollama_model || !next.openai_model) {
    return json({ ok: false, error: "Ollama and OpenAI model names are required" }, { status: 400 });
  }
  await env.AIFRED_SALES_LOG.put(API_RUNTIME_CONFIG_KEY, JSON.stringify(next));
  await recordActivity(env, {
    event_type: "admin.api_configuration.updated",
    actor: { type: "admin", id: adminActorId(request) },
    source: { surface: "admin", route: "/api/v1/admin/api/config" },
    subject: { type: "admin_operation", id: "api-runtime", name: "Runtime API configuration" },
    operation: { action: "update", status: "success", result: "configuration_saved" },
    metadata: { provider: next.provider, ollama_model: next.ollama_model, openai_model: next.openai_model }
  }, { request });
  return json({ ok: true, config: safeRuntimeApiPayload(next, env), message: "API configuration saved to KV; provider secrets remain Cloudflare-managed" });
}

async function handleAdminApiTest(request, env) {
  const configured = await runtimeChatEnv(env);
  const body = request.method === "POST" ? await readJson(request) : {};
  const provider = String(body.provider || configured.AIFRED_CHAT_PROVIDER || "").toLowerCase();
  try {
    if (provider === "ollama") {
      const base = String(configured.OLLAMA_BASE_URL || "").replace(/\/+$/, "");
      if (!base) return json({ ok: false, error: "OLLAMA_BASE_URL is not configured" }, { status: 503 });
      const response = await fetch(`${base}/api/tags`, {
        headers: {
          ...(configured.OLLAMA_API_TOKEN ? { authorization: `Bearer ${configured.OLLAMA_API_TOKEN}` } : {}),
          ...(configured.OLLAMA_ACCESS_CLIENT_ID && configured.OLLAMA_ACCESS_CLIENT_SECRET ? {
            "CF-Access-Client-Id": configured.OLLAMA_ACCESS_CLIENT_ID,
            "CF-Access-Client-Secret": configured.OLLAMA_ACCESS_CLIENT_SECRET
          } : {})
        }
      });
      if (!response.ok) return json({ ok: false, error: `Ollama test failed with HTTP ${response.status}` }, { status: 502 });
      const payload = await response.json();
      const models = Array.isArray(payload.models) ? payload.models.map((item) => String(item.name || "")).filter(Boolean) : [];
      return json({ ok: true, provider, models, message: `Ollama connected; ${models.length} model(s) discovered` });
    }
    if (provider === "openai") {
      if (!configured.OPENAI_API_KEY) return json({ ok: false, error: "OPENAI_API_KEY is not configured" }, { status: 503 });
      const response = await fetch("https://api.openai.com/v1/models", { headers: { authorization: `Bearer ${configured.OPENAI_API_KEY}` } });
      if (!response.ok) return json({ ok: false, error: `OpenAI test failed with HTTP ${response.status}` }, { status: 502 });
      const payload = await response.json();
      const models = Array.isArray(payload.data) ? payload.data.map((item) => String(item.id || "")).filter((id) => id.startsWith("gpt-")).slice(0, 50) : [];
      return json({ ok: true, provider, models, message: `OpenAI connected; ${models.length} GPT model(s) discovered` });
    }
    return json({ ok: true, provider: "website", models: canonicalModelList(configured), message: "Website API profile is available" });
  } catch (error) {
    return json({ ok: false, error: error.message || "provider test failed" }, { status: 503 });
  }
}

async function askOpenAI(env, message) {
  const apiKey = env.OPENAI_API_KEY;
  if (!apiKey) throw new Error("OPENAI_API_KEY is not configured");
  const model = env.OPENAI_MODEL || "gpt-5.6-luna";
  const response = await fetch("https://api.openai.com/v1/responses", {
    method: "POST",
    headers: {
      "authorization": `Bearer ${apiKey}`,
      "content-type": "application/json"
    },
    body: JSON.stringify({
      model,
      input: [
        {
          role: "system",
          content: "You are Aifred for North3rnLight3r. Be direct, technical, practical, and brand-safe. Focus on mix decisions, beat catalog questions, and AIFRED VST workflow."
        },
        { role: "user", content: message }
      ]
    })
  });
  const payload = await response.json();
  if (!response.ok) throw new Error(payload?.error?.message || "OpenAI request failed");
  const answer = payload.output_text
    || payload.output?.flatMap((item) => item.content || []).map((part) => part.text || "").join("").trim()
    || "Aifred received the request, but the model returned no text.";
  return { provider: "openai", model, answer };
}

function ollamaRequestHeaders(env, includeJson = false) {
  return {
    ...(includeJson ? { "content-type": "application/json" } : {}),
    ...(env.OLLAMA_API_TOKEN ? { authorization: `Bearer ${env.OLLAMA_API_TOKEN}` } : {}),
    ...(env.OLLAMA_ACCESS_CLIENT_ID && env.OLLAMA_ACCESS_CLIENT_SECRET ? {
      "CF-Access-Client-Id": env.OLLAMA_ACCESS_CLIENT_ID,
      "CF-Access-Client-Secret": env.OLLAMA_ACCESS_CLIENT_SECRET
    } : {})
  };
}

async function askOllama(env, message) {
  const base = String(env.OLLAMA_BASE_URL || "").replace(/\/+$/, "");
  if (!base) throw new Error("OLLAMA_BASE_URL is not configured");
  const model = env.OLLAMA_MODEL || "aifred:latest";
  const response = await fetch(`${base}/api/chat`, {
    method: "POST",
    headers: ollamaRequestHeaders(env, true),
    body: JSON.stringify({
      model,
      stream: false,
      messages: [
        {
          role: "system",
          content: "You are Aifred for North3rnLight3r. Be direct, technical, practical, and brand-safe. Focus on mix decisions, beat catalog questions, and AIFRED VST workflow."
        },
        { role: "user", content: message }
      ]
    })
  });
  const payload = await response.json();
  if (!response.ok) throw new Error(payload?.error || "Ollama request failed");
  return { provider: "ollama", model, answer: payload?.message?.content || payload?.response || "" };
}

async function handleChat(request, env) {
  env = await runtimeChatEnv(env);
  const body = await readJson(request);
  const message = String(body.message || body.prompt || "").trim();
  if (!message) return json({ ok: false, error: "message is required" }, { status: 400 });
  const provider = String(body.provider || env.AIFRED_CHAT_PROVIDER || "").toLowerCase();
  try {
    const result = provider === "ollama" ? await askOllama(env, message)
      : provider === "openai" ? await askOpenAI(env, message)
      : env.OPENAI_API_KEY ? await askOpenAI(env, message)
      : await askOllama(env, message);
    return json({ ok: true, ...result });
  } catch (error) {
    return json({ ok: false, error: error.message || "chat provider failed" }, { status: 503 });
  }
}

function openAiError(message, type = "invalid_request_error", code = null, param = null) {
  return { error: { message: String(message), type, code, param } };
}

function canonicalModelList(env) {
  const models = [];
  if (env.OPENAI_API_KEY) models.push(String(env.OPENAI_MODEL || "gpt-5.6-luna"));
  if (env.OLLAMA_BASE_URL) models.push(String(env.OLLAMA_MODEL || "aifred:latest"));
  return [...new Set(models.filter(Boolean))];
}

function canonicalModelsResponse(env) {
  const created = Math.floor(Date.now() / 1000);
  return {
    object: "list",
    data: canonicalModelList(env).map((id) => ({ id, object: "model", created, owned_by: id.startsWith("gpt-") ? "openai" : "aifred" }))
  };
}

function messagesFromBody(body) {
  if (!Array.isArray(body.messages) || body.messages.length === 0) throw new Error("messages must be a non-empty array");
  return body.messages.map((message) => {
    const role = String(message?.role || "").trim();
    if (!["system", "user", "assistant", "tool"].includes(role)) throw new Error("messages contains an unsupported role");
    const content = Array.isArray(message.content)
      ? message.content.map((part) => part?.text || "").join("")
      : String(message.content ?? "");
    return { role, content };
  });
}

function completionResponse(model, content, usage = {}) {
  return {
    id: `chatcmpl-${crypto.randomUUID()}`,
    object: "chat.completion",
    created: Math.floor(Date.now() / 1000),
    model,
    choices: [{ index: 0, message: { role: "assistant", content }, finish_reason: "stop" }],
    usage: { prompt_tokens: Number(usage.prompt_tokens || 0), completion_tokens: Number(usage.completion_tokens || 0), total_tokens: Number(usage.total_tokens || 0) }
  };
}

async function canonicalChat(request, env) {
  env = await runtimeChatEnv(env);
  let body;
  try { body = await request.json(); } catch (_) { return json(openAiError("request body must be valid JSON"), { status: 400 }); }
  let messages;
  try { messages = messagesFromBody(body); } catch (error) { return json(openAiError(error.message), { status: 400 }); }
  const model = String(body.model || env.OPENAI_MODEL || env.OLLAMA_MODEL || "").trim();
  if (!model) return json(openAiError("model is required"), { status: 400 });
  const stream = body.stream === true;
  try {
    let result;
    if (env.OLLAMA_BASE_URL && (!env.OPENAI_API_KEY || model === env.OLLAMA_MODEL)) {
      const response = await fetch(`${String(env.OLLAMA_BASE_URL).replace(/\/+$/, "")}/api/chat`, {
        method: "POST", headers: ollamaRequestHeaders(env, true),
        body: JSON.stringify({ model, messages, stream, options: { temperature: body.temperature, top_p: body.top_p, max_tokens: body.max_tokens } })
      });
      if (!response.ok) return json(openAiError("chat provider failed", "server_error"), { status: 502 });
      if (stream) {
        const reader = response.body?.getReader();
        if (!reader) return json(openAiError("stream unavailable", "server_error"), { status: 502 });
        const decoder = new TextDecoder();
        const encoder = new TextEncoder();
        const streamBody = new ReadableStream({ async pull(controller) { const { value, done } = await reader.read(); if (done) { controller.enqueue(encoder.encode("data: [DONE]\n\n")); controller.close(); return; } for (const line of decoder.decode(value, { stream: true }).split("\n").filter(Boolean)) { try { const item = JSON.parse(line); const delta = item.message?.content || ""; if (delta) controller.enqueue(encoder.encode(`data: ${JSON.stringify({ id: "chatcmpl-stream", object: "chat.completion.chunk", created: Math.floor(Date.now()/1000), model, choices: [{ index: 0, delta: { content: delta }, finish_reason: null }] })}\n\n`)); } catch (_) {} } } });
        return new Response(streamBody, { headers: { "content-type": "text/event-stream; charset=utf-8", "cache-control": "no-cache", connection: "keep-alive" } });
      }
      const payload = await response.json();
      result = completionResponse(model, payload.message?.content || payload.response || "", payload.prompt_eval_count ? { prompt_tokens: payload.prompt_eval_count, completion_tokens: payload.eval_count, total_tokens: payload.prompt_eval_count + payload.eval_count } : {});
    } else if (env.OPENAI_API_KEY) {
      const response = await fetch("https://api.openai.com/v1/responses", { method: "POST", headers: { authorization: `Bearer ${env.OPENAI_API_KEY}`, "content-type": "application/json" }, body: JSON.stringify({ model, input: messages, temperature: body.temperature, max_output_tokens: body.max_tokens }) });
      const payload = await response.json();
      if (!response.ok) return json(openAiError(payload?.error?.message || "chat provider failed", "server_error"), { status: 502 });
      const content = payload.output_text || payload.output?.flatMap((item) => item.content || []).map((part) => part.text || "").join("").trim() || "";
      result = completionResponse(model, content, { prompt_tokens: payload.usage?.input_tokens, completion_tokens: payload.usage?.output_tokens, total_tokens: payload.usage?.total_tokens });
    } else return json(openAiError("no chat provider is configured", "server_error"), { status: 503 });
    if (!stream) return json(result);
    return new Response(`data: ${JSON.stringify({ ...result, object: "chat.completion.chunk", choices: [{ index: 0, delta: result.choices[0].message, finish_reason: "stop" }] })}\n\ndata: [DONE]\n\n`, { headers: { "content-type": "text/event-stream; charset=utf-8", "cache-control": "no-cache" } });
  } catch (error) { return json(openAiError(error.message || "chat provider failed", "server_error"), { status: 503 }); }
}

async function handleOpsStatus(request, env) {
  if (!(await verifyAdmin(request, env))) return json(openAiError("admin session required", "authentication_error"), { status: 401 });
  const runtimeEnv = await runtimeChatEnv(env);
  const runtimeConfig = await loadRuntimeApiConfig(env);
  return json({
    ok: true,
    service: "AIFRED operations",
    snapshot_at: new Date().toISOString(),
    health: {
      api: "ok",
      providers: canonicalModelList(runtimeEnv).length,
      r2: { downloads_and_assets: Boolean(env.AIFRED_DOWNLOADS), reference: Boolean(env.AIFRED_REFERENCE_BUCKET) },
      kv: { reference_pool: Boolean(env.AIFRED_REFERENCE_POOL), activity_log: Boolean(env.AIFRED_SALES_LOG) }
    },
    deploy: repoConfig(env),
    distribution: { mode: "free", payment_pipeline: "disabled" },
    api_configuration: safeRuntimeApiPayload(runtimeConfig, env)
  });
}

function chatSettingsPayload(request, env) {
  const url = new URL(request.url);
  const wsProtocol = url.protocol === "https:" ? "wss:" : "ws:";
  const ollamaModel = env.OLLAMA_MODEL || "aifred:latest";
  const openAiModel = env.OPENAI_MODEL || "gpt-5.6-luna";
  return {
    ok: true,
    websocket_url: `${wsProtocol}//${url.host}/ws/chat`,
    persistence: env.AIFRED_CHAT_SESSIONS ? "bound" : "stateless",
    active_model: ollamaModel,
    models: [ollamaModel, openAiModel].filter(Boolean),
    settings: {
      transport_mode: "http",
      webhook: { enabled: false, url: "", secret: "", events: ["chat.completed", "chat.failed"] },
      context: { use_previous_response_id: true, memory_window_items: 40, summary_items: 6, max_prompt_chars: 4000, compact_threshold: 12 },
      prompt: { tone: "direct", personality_mode: "professional_mentor", system_prefix: "", system_suffix: "" },
      reasoning: { enabled: true, effort: "low" },
      response: { verbosity: "low", max_output_tokens: 900 }
    }
  };
}

function commandCatalog() {
  return BackendAdminActions.map(({ id, description, command }) => ({ id, description, command }));
}

function repoConfig(env) {
  const repo = String(env.AIFRED_GITHUB_REPO || "kaeganscott26/AIFRED").trim();
  const branch = String(env.AIFRED_GITHUB_BRANCH || "main").trim();
  return { repo, branch };
}

const EXPORT_SECRET_FIELD = /authorization|cookie|password|secret|token|api[_-]?key|private[_-]?key|client[_-]?secret/i;

function sanitizeExport(value, depth = 0) {
  if (depth > 8 || value === undefined) return undefined;
  if (value === null || ["string", "number", "boolean"].includes(typeof value)) return value;
  if (Array.isArray(value)) return value.map((entry) => sanitizeExport(entry, depth + 1)).filter((entry) => entry !== undefined);
  if (typeof value !== "object") return String(value);
  return Object.fromEntries(Object.entries(value)
    .filter(([key]) => !EXPORT_SECRET_FIELD.test(key))
    .map(([key, entry]) => [key, sanitizeExport(entry, depth + 1)])
    .filter(([, entry]) => entry !== undefined));
}

function exportFilename(prefix, generatedAt) {
  return `${prefix}-${generatedAt.replace(/[:.]/g, "-")}.json`;
}

async function buildSiteExport(request, env) {
  const generatedAt = new Date().toISOString();
  const [activity, inquiries] = await Promise.all([listActivityRecords(env, 5000), listInquiryRecords(env)]);
  const downloads = activity.filter((event) => String(event.event_type || "").startsWith("download."));
  const errors = activity.filter((event) => /failed|error/.test(String(event.event_type || "")) || event.operation?.status === "failure");
  const apiActivity = activity.filter((event) => /api|chat|admin\./.test(String(event.event_type || "")));
  const sessions = [...new Set(activity.map((event) => String(event.session_id || "")).filter(Boolean))];
  const pageViews = activity.filter((event) => String(event.event_type || "") === "website.page.view");
  return sanitizeExport({
    schemaVersion: "1.0.0", exportType: "aifred.site-data", generatedAt, timeZone: "UTC",
    summary: {
      activityEvents: activity.length, pageViews: pageViews.length,
      downloads: new Set(downloads.filter((event) => event.event_type === "download.completed").map((event) => event.request_id || event.event_id)).size,
      sessions: sessions.length, inquiries: inquiries.length, errors: errors.length
    },
    siteAnalytics: { pageViews, referrers: pageViews.map((event) => event.source?.referrer).filter(Boolean) },
    downloads, sessions, inquiries, errors, apiActivity,
    deployment: { ...repoConfig(env), target: "Cloudflare Pages project aifred-site", domains: ["north3rnlight3r.com", "www.north3rnlight3r.com", "aifred-site.pages.dev"] },
    operations: { healthEndpoint: new URL("/health", request.url).toString(), modelsEndpoint: new URL("/v1/models", request.url).toString() }
  });
}

async function buildTrackAnalysisExport(request, env) {
  const generatedAt = new Date().toISOString();
  const [tracks, references, activity] = await Promise.all([
    loadCatalog(request), listReferenceRecords(env, 1000), listActivityRecords(env, 5000)
  ]);
  const analysis = activity.filter((event) => /analysis|analyzer|reference/.test(String(event.event_type || "")));
  const errors = analysis.filter((event) => /failed|error|reject/.test(String(event.event_type || "")) || event.operation?.status === "failure");
  return sanitizeExport({
    schemaVersion: "1.0.0", exportType: "aifred.track-analysis", generatedAt, timeZone: "UTC",
    summary: { catalogTracks: tracks.length, referenceRecords: references.length, analysisEvents: analysis.length, errors: errors.length },
    tracks: withR2CatalogUrls(request, tracks), analysis: { references, events: analysis }, errors,
    engine: { localFirst: true, localApiBase: "http://127.0.0.1:8787", ollamaBase: "http://127.0.0.1:11434", cloudApiContract: ["/health", "/v1/models", "/v1/chat/completions"] }
  });
}

function exportResponse(payload, prefix) {
  return json(payload, { headers: {
    "cache-control": "no-store",
    "content-disposition": `attachment; filename="${exportFilename(prefix, payload.generatedAt)}"`,
    "x-content-type-options": "nosniff"
  } });
}

function pluginReleaseConfig(env) {
  return {
    repo: String(env.AIFRED_PLUGIN_REPO || "kaeganscott26/AIFRED").trim(),
    tag: String(env.AIFRED_PLUGIN_RELEASE_TAG || "v0.3.6-installer-ai-alias").trim()
  };
}

function contactEmail(env) {
  return String(env.AIFRED_CONTACT_EMAIL || "north3rnlight3rofficial@outlook.com").trim();
}

function emailFrom(env) {
  return String(env.AIFRED_EMAIL_FROM || "sales@north3rnlight3r.com").trim();
}

function escapeHtml(value) {
  return String(value || "")
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/"/g, "&quot;")
    .replace(/'/g, "&#39;");
}

function safeRepoPath(path) {
  const normalized = String(path || "").replace(/\\/g, "/").replace(/^\/+/, "");
  if (!normalized || normalized.includes("..") || normalized.startsWith(".git/")) {
    throw new Error("unsafe repo path");
  }
  return normalized;
}

function githubHeaders(env) {
  if (!env.GITHUB_TOKEN) throw new Error("GITHUB_TOKEN is not configured in Cloudflare Pages");
  return {
    "accept": "application/vnd.github+json",
    "authorization": `Bearer ${env.GITHUB_TOKEN}`,
    "content-type": "application/json",
    "user-agent": "aifred-admin"
  };
}

function utf8ToBase64(value) {
  const bytes = new TextEncoder().encode(value);
  let binary = "";
  bytes.forEach((byte) => { binary += String.fromCharCode(byte); });
  return btoa(binary);
}

function base64ToUtf8(value) {
  const binary = atob(value.replace(/\n/g, ""));
  const bytes = Uint8Array.from(binary, (char) => char.charCodeAt(0));
  return new TextDecoder().decode(bytes);
}

async function githubRequest(env, path, init = {}) {
  const response = await fetch(`https://api.github.com${path}`, {
    ...init,
    headers: { ...githubHeaders(env), ...(init.headers || {}) }
  });
  const raw = await response.text();
  const payload = raw ? JSON.parse(raw) : {};
  if (!response.ok) throw new Error(payload?.message || `GitHub request failed (${response.status})`);
  return payload;
}

async function readRepoJsonArray(env, relPath) {
  if (!env.GITHUB_TOKEN) return [];
  const { repo, branch } = repoConfig(env);
  const encodedPath = encodeURIComponent(safeRepoPath(relPath)).replace(/%2F/g, "/");
  try {
    const payload = await githubRequest(env, `/repos/${repo}/contents/${encodedPath}?ref=${branch}`);
    const parsed = JSON.parse(base64ToUtf8(payload.content || "[]"));
    return Array.isArray(parsed) ? parsed : [];
  } catch (_) {
    return [];
  }
}

async function writeRepoJsonArray(env, relPath, records, message) {
  if (!env.GITHUB_TOKEN) return "";
  const safePath = safeRepoPath(relPath);
  const { repo, branch } = repoConfig(env);
  const encodedPath = encodeURIComponent(safePath).replace(/%2F/g, "/");
  let sha = "";
  try {
    const existing = await githubRequest(env, `/repos/${repo}/contents/${encodedPath}?ref=${branch}`);
    sha = existing.sha || "";
  } catch (_) {}
  const payload = await githubRequest(env, `/repos/${repo}/contents/${encodedPath}`, {
    method: "PUT",
    body: JSON.stringify({
      message,
      content: utf8ToBase64(JSON.stringify(records, null, 2)),
      branch,
      ...(sha ? { sha } : {})
    })
  });
  return payload.commit?.sha || "";
}

async function appendRepoJsonRecord(env, relPath, record, message) {
  const records = await readRepoJsonArray(env, relPath);
  records.unshift(record);
  return {
    commit: await writeRepoJsonArray(env, relPath, records.slice(0, 400), message),
    records
  };
}

function activityRepoPath() {
  return "ops/activity/site-activity.json";
}

function requestCorrelation(request, defaultSurface) {
  const url = new URL(request.url);
  return {
    session_id: String(url.searchParams.get("sid") || "").trim(),
    request_id: String(url.searchParams.get("rid") || "").trim(),
    surface: String(url.searchParams.get("surface") || defaultSurface || "website").trim()
  };
}

function adminActorId(request) {
  const auth = request.headers.get("authorization") || "";
  const token = auth.toLowerCase().startsWith("bearer ") ? auth.slice(7).trim() : "";
  if (!token) return "";
  try {
    return String(atob(token.replace(/-/g, "+").replace(/_/g, "/")).split("|")[0] || "").trim();
  } catch (_) {
    return "";
  }
}

async function listActivityRecords(env, limit = 300) {
  const records = [];
  if (env.AIFRED_SALES_LOG && typeof env.AIFRED_SALES_LOG.list === "function") {
    const keys = [];
    let cursor;
    do {
      const listed = await env.AIFRED_SALES_LOG.list({ prefix: "activity:", limit: 1000, ...(cursor ? { cursor } : {}) });
      keys.push(...(listed.keys || []));
      cursor = listed.list_complete === false ? listed.cursor : undefined;
    } while (cursor && keys.length < 5000);
    for (const key of keys) {
      const raw = await env.AIFRED_SALES_LOG.get(key.name);
      if (!raw) continue;
      try {
        records.push(JSON.parse(raw));
      } catch (_) {}
    }
  }
  records.push(...await readRepoJsonArray(env, activityRepoPath()));
  const seen = new Set();
  const merged = [];
  for (const record of records) {
    const id = activityEventId(record);
    const key = id || JSON.stringify(record);
    if (seen.has(key)) continue;
    seen.add(key);
    merged.push(record);
  }
  return merged.sort((a, b) => activityTimestamp(b).localeCompare(activityTimestamp(a))).slice(0, limit);
}

async function listSaleRecords(env) {
  const repoSales = await readRepoJsonArray(env, salesRepoPath());
  if (!env.AIFRED_SALES_LOG || typeof env.AIFRED_SALES_LOG.list !== "function") return repoSales;
  const listed = await env.AIFRED_SALES_LOG.list({ prefix: "sale:", limit: 200 });
  const kvSales = [];
  for (const key of listed.keys || []) {
    const raw = await env.AIFRED_SALES_LOG.get(key.name);
    if (!raw) continue;
    try {
      kvSales.push(JSON.parse(raw));
    } catch (_) {}
  }
  const byTxn = new Map();
  [...kvSales, ...repoSales].forEach((sale) => {
    const id = String(sale.txn_id || sale.id || "");
    if (id && !byTxn.has(id)) byTxn.set(id, sale);
  });
  return [...byTxn.values()].sort((a, b) => String(b.created_at || "").localeCompare(String(a.created_at || "")));
}

async function persistInquiryRecord(env, inquiry) {
  if (env.AIFRED_SALES_LOG && typeof env.AIFRED_SALES_LOG.put === "function") {
    await env.AIFRED_SALES_LOG.put(`inquiry:${inquiry.id}`, JSON.stringify(inquiry));
    return { storage: "kv" };
  }
  return { storage: "unconfigured" };
}

async function listInquiryRecords(env, limit = 200) {
  const records = [];
  if (env.AIFRED_SALES_LOG && typeof env.AIFRED_SALES_LOG.list === "function") {
    const listed = await env.AIFRED_SALES_LOG.list({ prefix: "inquiry:", limit });
    for (const key of listed.keys || []) {
      const raw = await env.AIFRED_SALES_LOG.get(key.name);
      if (!raw) continue;
      try {
        records.push(JSON.parse(raw));
      } catch (_) {}
    }
  }
  records.push(...await readRepoJsonArray(env, inquiriesRepoPath()));
  const byId = new Map();
  for (const record of records) {
    const id = String(record?.id || record?.created_at || JSON.stringify(record));
    if (!byId.has(id)) byId.set(id, record);
  }
  return [...byId.values()]
    .sort((a, b) => String(b.created_at || "").localeCompare(String(a.created_at || "")))
    .slice(0, limit);
}

async function sendNotificationEmail(env, payload) {
  if (!env.EMAIL || typeof env.EMAIL.send !== "function") {
    return { ok: false, error: "EMAIL binding is not configured" };
  }

  try {
    const response = await env.EMAIL.send(payload);
    return { ok: true, id: response?.messageId || "" };
  } catch (error) {
    return { ok: false, error: error.message || "email send failed" };
  }
}

function salesRepoPath() {
  return "ops/payments/sales.json";
}

function inquiriesRepoPath() {
  return "ops/support/inquiries.json";
}

function assetNameForKey(key) {
  if (key === "setup") return "AIFRED-VST3-Setup.exe";
  if (key === "zip") return "AIFRED-VST3-windows.zip";
  if (key === "macos") return "AIFRED-VST3-macos.zip";
  return "";
}

function releaseAssetObjectKey(env, assetName) {
  const version = String(env.AIFRED_RELEASE_VERSION || pluginReleaseConfig(env).tag).trim();
  return `releases/${version}/${assetName}`;
}

function parseByteRange(value, size) {
  const match = String(value || "").trim().match(/^bytes=(\d*)-(\d*)$/i);
  if (!match || (!match[1] && !match[2]) || !Number.isSafeInteger(size) || size < 1) return null;
  let start;
  let end;
  if (!match[1]) {
    const suffixLength = Number(match[2]);
    if (!Number.isSafeInteger(suffixLength) || suffixLength < 1) return null;
    const length = Math.min(suffixLength, size);
    start = size - length;
    end = size - 1;
  } else {
    start = Number(match[1]);
    end = match[2] ? Number(match[2]) : size - 1;
    if (!Number.isSafeInteger(start) || !Number.isSafeInteger(end) || start < 0 || start >= size || end < start) return null;
    end = Math.min(end, size - 1);
  }
  return { offset: start, length: end - start + 1 };
}

function rangeNotSatisfiable(size) {
  return new Response(null, {
    status: 416,
    headers: {
      "accept-ranges": "bytes",
      "cache-control": "no-store",
      "content-range": `bytes */${size}`
    }
  });
}

function r2Response(object, { contentType, fileName = "", cacheControl = "public, max-age=3600", includeBody = true, responseRange = null, totalSize = object.size } = {}) {
  const headers = new Headers();
  if (typeof object.writeHttpMetadata === "function") object.writeHttpMetadata(headers);
  headers.set("accept-ranges", "bytes");
  headers.set("cache-control", cacheControl);
  headers.set("content-type", headers.get("content-type") || contentType || "application/octet-stream");
  const etag = object.httpEtag || object.etag || "";
  if (etag) headers.set("etag", etag);
  headers.set("x-aifred-download-source", "r2");
  if (fileName) headers.set("content-disposition", `attachment; filename="${fileName.replace(/["\\]/g, "_")}"`);

  let status = 200;
  if (responseRange) {
    const start = responseRange.offset;
    const end = start + responseRange.length - 1;
    headers.set("content-range", `bytes ${start}-${end}/${totalSize}`);
    headers.set("content-length", String(responseRange.length));
    status = 206;
  } else {
    headers.set("content-length", String(totalSize));
  }
  return new Response(includeBody ? object.body : null, { status, headers });
}

async function fetchReleaseAssetResponse(request, env, assetName) {
  if (!env.AIFRED_DOWNLOADS || typeof env.AIFRED_DOWNLOADS.get !== "function") {
    return json({ ok: false, error: "AIFRED_DOWNLOADS R2 binding is not configured" }, { status: 503 });
  }

  const objectKey = releaseAssetObjectKey(env, assetName);
  const isHead = request.method === "HEAD";
  let object;
  let responseRange = null;
  let totalSize;
  if (isHead && typeof env.AIFRED_DOWNLOADS.head === "function") {
    object = await env.AIFRED_DOWNLOADS.head(objectKey);
  } else if (request.headers.has("range") && typeof env.AIFRED_DOWNLOADS.head === "function") {
    const metadata = await env.AIFRED_DOWNLOADS.head(objectKey);
    if (metadata) {
      totalSize = metadata.size;
      responseRange = parseByteRange(request.headers.get("range"), totalSize);
      if (!responseRange) return rangeNotSatisfiable(totalSize);
      object = await env.AIFRED_DOWNLOADS.get(objectKey, { range: responseRange });
    }
  } else {
    object = await env.AIFRED_DOWNLOADS.get(objectKey);
  }
  if (object) {
    return r2Response(object, {
      contentType: object.httpMetadata?.contentType || "application/octet-stream",
      fileName: assetName,
      includeBody: !isHead,
      responseRange,
      totalSize: totalSize ?? object.size
    });
  }

  if (!env.GITHUB_TOKEN) {
    return json({ ok: false, error: `release object is not available in AIFRED_DOWNLOADS: ${assetName}` }, { status: 404 });
  }
  const { repo, tag } = pluginReleaseConfig(env);
  const release = await githubRequest(env, `/repos/${repo}/releases/tags/${encodeURIComponent(tag)}`);
  const asset = Array.isArray(release.assets) ? release.assets.find((entry) => entry.name === assetName) : null;
  if (!asset?.url) throw new Error(`release asset not found: ${assetName}`);

  const response = await fetch(asset.url, {
    method: request.method,
    headers: {
      accept: "application/octet-stream",
      authorization: `Bearer ${env.GITHUB_TOKEN}`,
      "user-agent": "aifred-site",
      ...(request.headers.has("range") ? { range: request.headers.get("range") } : {})
    }
  });

  if (!response.ok) {
    throw new Error(`asset download failed (${response.status})`);
  }

  const headers = new Headers();
  headers.set("accept-ranges", response.headers.get("accept-ranges") || "bytes");
  headers.set("cache-control", "public, max-age=3600");
  headers.set("content-disposition", `attachment; filename="${assetName}"`);
  headers.set("content-type", response.headers.get("content-type") || "application/octet-stream");
  const contentLength = response.headers.get("content-length");
  if (contentLength) headers.set("content-length", contentLength);
  const contentRange = response.headers.get("content-range");
  if (contentRange) headers.set("content-range", contentRange);
  headers.set("x-aifred-download-source", "github-release");
  return new Response(isHead ? null : response.body, { status: response.status, headers });
}

async function fetchWebsiteAssetResponse(request, env, relPath) {
  let safePath = "";
  try {
    safePath = safeRepoPath(decodeURIComponent(String(relPath || "")));
  } catch (_) {
    return json({ ok: false, error: "invalid asset path" }, { status: 400 });
  }
  const objectKey = `assets/${safePath}`;
  const bucket = env.AIFRED_DOWNLOADS;
  const isHead = request.method === "HEAD";
  const wantsDownload = new URL(request.url).searchParams.get("download") === "1";
  if (bucket && typeof bucket.get === "function") {
    let object;
    let responseRange = null;
    let totalSize;
    if (isHead && typeof bucket.head === "function") {
      object = await bucket.head(objectKey);
    } else if (request.headers.has("range") && typeof bucket.head === "function") {
      const metadata = await bucket.head(objectKey);
      if (metadata) {
        totalSize = metadata.size;
        responseRange = parseByteRange(request.headers.get("range"), totalSize);
        if (!responseRange) return rangeNotSatisfiable(totalSize);
        object = await bucket.get(objectKey, { range: responseRange });
      }
    } else {
      object = await bucket.get(objectKey);
    }
    if (object) {
      const response = r2Response(object, {
        contentType: object.httpMetadata?.contentType || contentTypeForPath(safePath),
        fileName: wantsDownload ? safePath.split("/").pop() : "",
        includeBody: !isHead,
        responseRange,
        totalSize: totalSize ?? object.size
      });
      response.headers.set("x-aifred-asset-source", "r2");
      return response;
    }
  }

  const fallback = await fetch(new URL(`/assets/${safePath}`, request.url), {
    method: request.method,
    cache: "no-store",
    headers: request.headers.has("range") ? { range: request.headers.get("range") } : undefined
  });
  if (fallback.ok) {
    const headers = new Headers(fallback.headers);
    headers.set("x-aifred-asset-source", "static-fallback");
    if (wantsDownload) headers.set("content-disposition", `attachment; filename="${safePath.split("/").pop().replace(/["\\]/g, "_")}"`);
    return new Response(isHead ? null : fallback.body, { status: fallback.status, headers });
  }
  return json({ ok: false, error: `asset not found: ${safePath}` }, { status: 404 });
}

function contentTypeForPath(path) {
  const lower = String(path || "").toLowerCase();
  if (lower.endsWith(".mp3")) return "audio/mpeg";
  if (lower.endsWith(".wav")) return "audio/wav";
  if (lower.endsWith(".flac")) return "audio/flac";
  if (lower.endsWith(".m4a")) return "audio/mp4";
  if (lower.endsWith(".jpg") || lower.endsWith(".jpeg")) return "image/jpeg";
  if (lower.endsWith(".png")) return "image/png";
  if (lower.endsWith(".webp")) return "image/webp";
  if (lower.endsWith(".json")) return "application/json; charset=utf-8";
  return "application/octet-stream";
}

async function handlePluginDownload(request, env) {
  const url = new URL(request.url);
  const assetKey = String(url.searchParams.get("asset") || "").trim();
  const assetName = assetNameForKey(assetKey);
  if (!assetName) return json({ ok: false, error: "asset must be setup, zip, or macos" }, { status: 400 });
  if (request.method !== "GET") return fetchReleaseAssetResponse(request, env, assetName);

  const correlation = requestCorrelation(request, "website.downloads");
  const release = pluginReleaseConfig(env).tag;
  const objectKey = releaseAssetObjectKey(env, assetName);
  const baseEvent = {
    session_id: correlation.session_id,
    request_id: correlation.request_id,
    actor: { type: "anonymous", id: correlation.session_id },
    source: { surface: correlation.surface, route: "/api/v1/downloads/plugin" },
    subject: { type: "download", id: assetKey, name: assetName }
  };
  const requested = await recordActivity(env, {
    ...baseEvent,
    event_type: "download.requested",
    operation: { action: "download", status: "started", result: "worker_received" },
    metadata: { artifact: assetKey, release, object_key: objectKey }
  }, { request });
  baseEvent.request_id = requested.event.request_id;

  let response;
  try {
    response = await fetchReleaseAssetResponse(request, env, assetName);
  } catch (error) {
    await recordActivity(env, {
      ...baseEvent,
      event_type: "download.failed",
      operation: { action: "download", status: "failure", result: "resolution_exception" },
      metadata: { artifact: assetKey, release, object_key: objectKey, error_type: error?.name || "Error" }
    }, { request });
    throw error;
  }

  const responseSource = response.headers.get("x-aifred-download-source") || "unresolved";
  await recordActivity(env, {
    ...baseEvent,
    event_type: response.ok ? "download.completed" : "download.failed",
    operation: {
      action: "download",
      status: response.ok ? "success" : "failure",
      result: response.ok ? "response_resolved" : `http_${response.status}`
    },
    metadata: {
      artifact: assetKey,
      release,
      object_key: objectKey,
      response_source: responseSource,
      http_status: response.status,
      content_length: response.headers.get("content-length") || ""
    }
  }, { request });
  response.headers.set("x-aifred-request-id", requested.event.request_id);
  return response;
}

async function handleWebsiteAssetRequest(request, env, relPath) {
  const url = new URL(request.url);
  if (request.method !== "GET" || url.searchParams.get("download") !== "1") {
    return fetchWebsiteAssetResponse(request, env, relPath);
  }

  const correlation = requestCorrelation(request, "catalog");
  let decodedPath = String(relPath || "");
  try { decodedPath = safeRepoPath(decodeURIComponent(decodedPath)); } catch (_) {}
  const fileName = decodedPath.replace(/\\/g, "/").split("/").pop() || decodedPath;
  const baseEvent = {
    session_id: correlation.session_id,
    request_id: correlation.request_id,
    actor: { type: "anonymous", id: correlation.session_id },
    source: { surface: correlation.surface, route: `/api/v1/assets/${decodedPath}` },
    subject: { type: decodedPath.startsWith("audio/catalog/") ? "track" : "download", id: decodedPath, name: fileName }
  };
  const requested = await recordActivity(env, {
    ...baseEvent,
    event_type: "download.requested",
    operation: { action: "download", status: "started", result: "worker_received" },
    metadata: { object_key: `assets/${decodedPath}`, artifact: "catalog_mp3" }
  }, { request });
  baseEvent.request_id = requested.event.request_id;

  let response;
  try {
    response = await fetchWebsiteAssetResponse(request, env, relPath);
  } catch (error) {
    await recordActivity(env, {
      ...baseEvent,
      event_type: "download.failed",
      operation: { action: "download", status: "failure", result: "resolution_exception" },
      metadata: { object_key: `assets/${decodedPath}`, artifact: "catalog_mp3", error_type: error?.name || "Error" }
    }, { request });
    throw error;
  }

  await recordActivity(env, {
    ...baseEvent,
    event_type: response.ok ? "download.completed" : "download.failed",
    operation: {
      action: "download",
      status: response.ok ? "success" : "failure",
      result: response.ok ? "response_resolved" : `http_${response.status}`
    },
    metadata: {
      object_key: `assets/${decodedPath}`,
      artifact: "catalog_mp3",
      response_source: response.headers.get("x-aifred-asset-source") || "unresolved",
      http_status: response.status,
      content_length: response.headers.get("content-length") || ""
    }
  }, { request });
  response.headers.set("x-aifred-request-id", requested.event.request_id);
  return response;
}

async function handleAdminFileRead(request, env) {
  if (!(await verifyAdmin(request, env))) return json({ ok: false, error: "admin session required" }, { status: 401 });
  const body = await readJson(request);
  const relPath = safeRepoPath(body.path);
  if (!env.GITHUB_TOKEN) {
    const response = await fetch(new URL(`/${relPath}`, request.url), { cache: "no-store" });
    if (!response.ok) return json({ ok: false, error: "GITHUB_TOKEN required for repo reads outside deployed assets" }, { status: 501 });
    return json({ ok: true, path: relPath, content: await response.text(), source: "deployed asset" });
  }
  const { repo, branch } = repoConfig(env);
  try {
    const payload = await githubRequest(env, `/repos/${repo}/contents/${encodeURIComponent(relPath).replace(/%2F/g, "/")}?ref=${branch}`);
    return json({ ok: true, path: relPath, sha: payload.sha, content: base64ToUtf8(payload.content || ""), source: "github" });
  } catch (error) {
    const response = await fetch(new URL(`/${relPath}`, request.url), { cache: "no-store" });
    if (response.ok) return json({ ok: true, path: relPath, content: await response.text(), source: "deployed asset", warning: `GitHub read failed: ${error.message || "unknown error"}` });
    return json({ ok: false, error: `GitHub read failed: ${error.message || "unknown error"}` }, { status: 502 });
  }
}

async function handleAdminFileWrite(request, env) {
  if (!(await verifyAdmin(request, env))) return json({ ok: false, error: "admin session required" }, { status: 401 });
  const body = await readJson(request);
  const relPath = safeRepoPath(body.path);
  const content = String(body.content || "");
  const shouldDeploy = body.deploy !== false;
  if (!env.GITHUB_TOKEN) return json({ ok: false, error: "GITHUB_TOKEN is not configured, so mobile website writes cannot be committed" }, { status: 501 });
  const { repo, branch } = repoConfig(env);
  const encodedPath = encodeURIComponent(relPath).replace(/%2F/g, "/");
  let sha = "";
  try {
    const existing = await githubRequest(env, `/repos/${repo}/contents/${encodedPath}?ref=${branch}`);
    sha = existing.sha || "";
  } catch (_) {}
  let payload;
  try {
    payload = await githubRequest(env, `/repos/${repo}/contents/${encodedPath}`, {
      method: "PUT",
      body: JSON.stringify({
        message: `Update ${relPath} from AIFRED admin`,
        content: utf8ToBase64(content),
        branch,
        ...(sha ? { sha } : {})
      })
    });
  } catch (error) {
    return json({ ok: false, error: `GitHub write failed: ${error.message || "unknown error"}` }, { status: 502 });
  }
    await recordActivity(env, {
      event_type: "admin.file.updated",
      actor: { type: "admin", id: adminActorId(request) },
      source: { surface: "admin", route: "/api/v1/admin/files/write" },
      subject: { type: "admin_operation", id: relPath, name: relPath },
      operation: { action: "update", status: "success", result: "github_committed" },
      metadata: { deploy_requested: Boolean(shouldDeploy), length: content.length, commit: payload.commit?.sha || "" }
    }, { request });
  return json({
    ok: true,
    path: relPath,
    commit: payload.commit?.sha || "",
    deploy_dispatched: false,
    deploy_error: "",
    message: shouldDeploy
      ? "website file committed; run the repository deployment command or manually dispatch the deployment workflow after validation"
      : "website file committed without requesting a deployment"
  });
}

async function handleAdminFileList(request, env) {
  if (!(await verifyAdmin(request, env))) return json({ ok: false, error: "admin session required" }, { status: 401 });
  if (!env.GITHUB_TOKEN) return json({ ok: false, error: "GITHUB_TOKEN is not configured in Cloudflare Pages" }, { status: 501 });
  const url = new URL(request.url);
  const relPath = safeRepoPath(url.searchParams.get("path") || "apps/website");
  const { repo, branch } = repoConfig(env);
  const encodedPath = encodeURIComponent(relPath).replace(/%2F/g, "/");
  const payload = await githubRequest(env, `/repos/${repo}/contents/${encodedPath}?ref=${branch}`);
  const entries = (Array.isArray(payload) ? payload : [payload]).map((item) => ({
    name: item.name,
    path: item.path,
    type: item.type,
    size: item.size || 0,
    sha: item.sha || ""
  }));
  return json({ ok: true, path: relPath, entries });
}

async function handleAdminFileDelete(request, env) {
  if (!(await verifyAdmin(request, env))) return json({ ok: false, error: "admin session required" }, { status: 401 });
  const body = await readJson(request);
  const relPath = safeRepoPath(body.path);
  if (!relPath.startsWith("apps/website/")) return json({ ok: false, error: "mobile delete is restricted to apps/website/" }, { status: 403 });
  const { repo, branch } = repoConfig(env);
  const encodedPath = encodeURIComponent(relPath).replace(/%2F/g, "/");
  const existing = await githubRequest(env, `/repos/${repo}/contents/${encodedPath}?ref=${branch}`);
  const payload = await githubRequest(env, `/repos/${repo}/contents/${encodedPath}`, {
    method: "DELETE",
    body: JSON.stringify({
      message: `Delete ${relPath} from AIFRED admin`,
      branch,
      sha: existing.sha
    })
  });
  await recordActivity(env, {
    event_type: "admin.file.deleted",
    actor: { type: "admin", id: adminActorId(request) },
    source: { surface: "admin", route: "/api/v1/admin/files/delete" },
    subject: { type: "admin_operation", id: relPath, name: relPath },
    operation: { action: "delete", status: "success", result: "github_committed" },
    metadata: { source_sha: existing.sha || "", commit: payload.commit?.sha || "" }
  }, { request });
  return json({ ok: true, path: relPath, commit: payload.commit?.sha || "" });
}

function safeUploadName(name) {
  return String(name || `upload-${crypto.randomUUID()}`)
    .replace(/\\/g, "/")
    .split("/")
    .pop()
    .replace(/[^A-Za-z0-9._ -]/g, "-")
    .replace(/\s+/g, "-")
    .slice(0, 120);
}

async function fileToBase64(file) {
  const bytes = new Uint8Array(await file.arrayBuffer());
  let binary = "";
  bytes.forEach((byte) => { binary += String.fromCharCode(byte); });
  return btoa(binary);
}

async function writeBinaryRepoFile(env, relPath, file, message) {
  const safePath = safeRepoPath(relPath);
  const { repo, branch } = repoConfig(env);
  const encodedPath = encodeURIComponent(safePath).replace(/%2F/g, "/");
  let sha = "";
  try {
    const existing = await githubRequest(env, `/repos/${repo}/contents/${encodedPath}?ref=${branch}`);
    sha = existing.sha || "";
  } catch (_) {}
  const payload = await githubRequest(env, `/repos/${repo}/contents/${encodedPath}`, {
    method: "PUT",
    body: JSON.stringify({
      message,
      content: await fileToBase64(file),
      branch,
      ...(sha ? { sha } : {})
    })
  });
  return { path: safePath, commit: payload.commit?.sha || "" };
}

async function handleAdminFileUpload(request, env) {
  if (!(await verifyAdmin(request, env))) return json({ ok: false, error: "admin session required" }, { status: 401 });
  const form = await request.formData();
  const file = form.get("file");
  if (!file || typeof file === "string") return json({ ok: false, error: "file is required" }, { status: 400 });
  const targetPath = safeRepoPath(form.get("path") || `apps/website/assets/uploads/${safeUploadName(file.name)}`);
  const written = await writeBinaryRepoFile(env, targetPath, file, `Upload ${targetPath} from AIFRED admin`);
  await recordActivity(env, {
    event_type: "admin.file.uploaded",
    actor: { type: "admin", id: adminActorId(request) },
    source: { surface: "admin", route: "/api/v1/admin/files/upload" },
    subject: { type: "admin_operation", id: targetPath, name: file.name },
    operation: { action: "upload", status: "success", result: "github_committed" },
    metadata: { size: file.size || 0, content_type: file.type || "", commit: written.commit }
  }, { request });
  return json({ ok: true, stored_path: written.path, commit: written.commit });
}

async function handleAdminReferenceUpload(request, env) {
  if (!(await verifyAdmin(request, env))) return json({ ok: false, error: "admin session required" }, { status: 401 });
  const form = await request.formData();
  const file = form.get("file");
  if (!file || typeof file === "string") return json({ ok: false, error: "file is required" }, { status: 400 });
  const genre = String(form.get("genre") || "reference").replace(/[^A-Za-z0-9._-]/g, "-").toLowerCase();
  const targetPath = `apps/website/assets/reference_pool/${genre}/${safeUploadName(file.name)}`;
  const written = await writeBinaryRepoFile(env, targetPath, file, `Upload reference ${targetPath} from AIFRED admin`);
  await recordActivity(env, {
    event_type: "admin.reference.updated",
    actor: { type: "admin", id: adminActorId(request) },
    source: { surface: "admin", route: "/api/v1/admin/reference/upload" },
    subject: { type: "admin_operation", id: targetPath, name: file.name },
    operation: { action: "upload", status: "success", result: "reference_uploaded" },
    metadata: { genre, size: file.size || 0, commit: written.commit }
  }, { request });
  return json({ ok: true, stored_path: written.path, commit: written.commit });
}

async function handleAdminCatalogUpload(request, env) {
  if (!(await verifyAdmin(request, env))) return json({ ok: false, error: "admin session required" }, { status: 401 });
  const form = await request.formData();
  const file = form.get("file");
  if (!file || typeof file === "string") return json({ ok: false, error: "file is required" }, { status: 400 });
  const title = String(form.get("title") || file.name || "North3rnLight3r catalog upload").trim();
  const fileName = safeUploadName(file.name);
  const audioPath = `apps/website/assets/audio/catalog/${fileName}`;
  const audioWrite = await writeBinaryRepoFile(env, audioPath, file, `Upload catalog audio ${fileName} from AIFRED admin`);
  const tracks = await loadCatalog(request);
  const track = {
    key: crypto.randomUUID(),
    title,
    bpm: String(form.get("bpm") || "").trim(),
    genre: String(form.get("pack_type") || "North3rnLight3r").trim(),
    duration_label: "",
    price: "Free MP3 download; licensing by inquiry",
    asset_file_name: fileName,
    stream_url: `/api/v1/assets/audio/catalog/${fileName}`,
    artwork_url: "assets/brand/aifred-mascot.jpg",
    description: String(form.get("description") || "").trim(),
    key_signature: String(form.get("key") || "").trim(),
    tempo: String(form.get("tempo") || "").trim()
  };
  const { repo, branch } = repoConfig(env);
  const catalogPath = "apps/website/assets/data/beat_catalog.json";
  const encodedPath = encodeURIComponent(catalogPath).replace(/%2F/g, "/");
  let sha = "";
  try {
    const existing = await githubRequest(env, `/repos/${repo}/contents/${encodedPath}?ref=${branch}`);
    sha = existing.sha || "";
  } catch (_) {}
  await githubRequest(env, `/repos/${repo}/contents/${encodedPath}`, {
    method: "PUT",
    body: JSON.stringify({
      message: `Add catalog item ${title} from AIFRED admin`,
      content: utf8ToBase64(JSON.stringify([...tracks, track], null, 2)),
      branch,
      ...(sha ? { sha } : {})
    })
  });
  await recordActivity(env, {
    event_type: "admin.catalog.updated",
    actor: { type: "admin", id: adminActorId(request) },
    source: { surface: "admin", route: "/api/v1/admin/catalog/upload" },
    subject: { type: "admin_operation", id: track.key, name: title },
    operation: { action: "upload", status: "success", result: "catalog_item_added" },
    metadata: { asset_file_name: fileName, genre: track.genre, bpm: track.bpm, commit: audioWrite.commit }
  }, { request });
  return json({ ok: true, stored_path: audioWrite.path, track, commit: audioWrite.commit });
}

async function handleCommand(request, env) {
  if (!(await verifyAdmin(request, env))) return json({ ok: false, error: "admin session required" }, { status: 401 });
  const body = await readJson(request);
  const command = String(body.command_line || body.command || "").trim();
  const normalized = command.startsWith("action:") ? command.slice(7).trim() : command;
  const resolvedActions = Object.fromEntries(BackendAdminActions.map((action) => [action.command, { event_type: action.eventType, subject: action.subject }]));
  const resolved = resolvedActions[normalized];
  if (!resolved) {
    await recordActivity(env, {
      event_type: "admin.operation.failed",
      actor: { type: "admin", id: adminActorId(request) },
      source: { surface: "admin", route: "/api/v1/command/run" },
      subject: { type: "admin_operation", id: "unsupported", name: "Unsupported allowlist action" },
      operation: { action: "run", status: "failure", result: "unsupported_action" }
    }, { request });
    return json({ ok: false, exit_code: 2, stderr: "Unsupported command. Use /api/v1/registry/actions for the allowlist." }, { status: 400 });
  }
  let stdout = "";
  if (normalized === "help") stdout = JSON.stringify(commandCatalog(), null, 2);
  else if (normalized === "health") stdout = JSON.stringify({ ok: true, service: "AIFRED website backend" }, null, 2);
  else if (normalized === "catalog:list") stdout = `tracks=${(await loadCatalog(request)).length}`;
  else if (normalized === "models:list") stdout = JSON.stringify({
    openai: Boolean(env.OPENAI_API_KEY),
    openai_model: env.OPENAI_MODEL || "gpt-5.6-luna",
    ollama: Boolean(env.OLLAMA_BASE_URL),
    ollama_model: env.OLLAMA_MODEL || "aifred:latest"
  }, null, 2);
  else if (normalized === "reference:stats") stdout = JSON.stringify({
    reference_pool_binding: Boolean(env.AIFRED_REFERENCE_POOL),
    reference_bucket_binding: Boolean(env.AIFRED_REFERENCE_BUCKET),
    website_assets_binding: Boolean(env.AIFRED_DOWNLOADS),
    accepted_uploads: env.AIFRED_REFERENCE_POOL ? "stored in KV and mirrored to R2 when bound" : "accepted metadata is reported but not persisted until KV is bound"
  }, null, 2);
  else if (normalized === "deploy:status") stdout = "Cloudflare Pages project: aifred-site. Production domains: north3rnlight3r.com and aifred-site.pages.dev.";
  else if (normalized === "sales:list") stdout = JSON.stringify(await listSaleRecords(env), null, 2);
  else if (normalized === "inquiries:list") stdout = JSON.stringify(await listInquiryRecords(env), null, 2);
  else if (normalized === "export:site") stdout = JSON.stringify(await buildSiteExport(request, env), null, 2);
  else if (normalized === "export:tracks") stdout = JSON.stringify(await buildTrackAnalysisExport(request, env), null, 2);
  await recordActivity(env, {
    event_type: resolved.event_type,
    actor: { type: "admin", id: adminActorId(request) },
    source: { surface: "admin", route: "/api/v1/command/run" },
    subject: { type: "admin_operation", id: resolved.subject, name: resolved.subject },
    operation: { action: "run", status: "success", result: "allowlist_action_completed" }
  }, { request });
  return json({ ok: true, exit_code: 0, stdout, stderr: "" });
}

async function handleAdminLogin(request, env) {
  const body = await readJson(request);
  const username = String(body.username || "").trim();
  const password = String(body.password || "");
  const expected = getExpectedAdmin(env);
  if (!expected.username || !expected.passwordHash || !String(env.AIFRED_ADMIN_SESSION_SECRET || "").trim()) {
    return json({ ok: false, error: "admin authentication is not configured" }, { status: 503 });
  }
  const clientAddress = String(request.headers.get("cf-connecting-ip") || "unknown").trim();
  const throttleKey = `security:admin-login:${await sha256Hex(clientAddress)}`;
  const throttleStore = env.AIFRED_SALES_LOG &&
    typeof env.AIFRED_SALES_LOG.get === "function" &&
    typeof env.AIFRED_SALES_LOG.put === "function"
    ? env.AIFRED_SALES_LOG
    : null;
  let attempts = 0;
  if (throttleStore) {
    try {
      attempts = Number(await throttleStore.get(throttleKey) || 0);
    } catch (_) {
      // Authentication must remain available when the optional free-tier KV
      // throttle store has exhausted its operation quota.
      attempts = 0;
    }
  }
  if (attempts >= 5) {
    return json({ ok: false, error: "too many login attempts; try again later" }, { status: 429, headers: { "retry-after": "600" } });
  }
  const passwordHash = await sha256Hex(password);
  if (!constantTimeEqual(username, expected.username) || !constantTimeEqual(passwordHash, expected.passwordHash)) {
    if (throttleStore) {
      try { await throttleStore.put(throttleKey, String(attempts + 1), { expirationTtl: 600 }); } catch (_) {}
    }
    return json({ ok: false, error: "invalid admin credentials" }, { status: 401 });
  }
  if (throttleStore && typeof throttleStore.delete === "function") {
    try { await throttleStore.delete(throttleKey); } catch (_) {}
  }
  await recordActivity(env, {
    event_type: "admin.login.succeeded",
    actor: { type: "admin", id: username },
    source: { surface: "admin", route: "/api/v1/admin/login" },
    subject: { type: "admin_operation", id: "admin-session", name: "Admin session" },
    operation: { action: "login", status: "success", result: "authenticated" }
  }, { request });
  return json({ ok: true, username, session_token: await createAdminSession(username, env) });
}

export async function onRequest({ request, env, params }) {
  const path = Array.isArray(params.path) ? params.path.join("/") : String(params.path || "");

  if (request.method === "OPTIONS") return new Response(null, { status: 204 });
  try {
  if (path === "health") return json({ ok: true, service: "AIFRED API", api_version: "v1" });
  if (path === "models") return json(canonicalModelsResponse(await runtimeChatEnv(env)));
  if (path === "chat/completions" && request.method === "POST") return canonicalChat(request, env);
  if (path === "embeddings" && request.method === "POST") return json(openAiError("embeddings provider is not configured", "server_error"), { status: 501 });
  if (path === "responses") return json(openAiError("responses is reserved for a future compatible implementation", "not_implemented"), { status: 501 });
  if (path === "catalog/list") return json({ ok: true, tracks: withR2CatalogUrls(request, await loadCatalog(request)) });
  if (path === "soundpacks/list") return json({ ok: true, soundpacks: [] });
  if (path === "content/get") return json({ ok: true, content: contentPayload() });
  if (path === "activity/record" && request.method === "POST") return handleActivityRecord(request, env);
  if (path === "analysis/submit" && request.method === "POST") return handleAnalysisSubmit(request, env);
  if (path === "analyzer/submit" && request.method === "POST") return handleAnalysisSubmit(request, env);
  if (path === "chat/settings") return json(chatSettingsPayload(request, await runtimeChatEnv(env)));
  if (path === "downloads/plugin" && (request.method === "GET" || request.method === "HEAD")) return handlePluginDownload(request, env);
  if (path.startsWith("assets/") && (request.method === "GET" || request.method === "HEAD")) return handleWebsiteAssetRequest(request, env, path.slice("assets/".length));
  if (path === "admin/login" && request.method === "POST") return handleAdminLogin(request, env);
  if (path.startsWith("admin/") && !(await verifyAdmin(request, env))) {
    return json({ ok: false, error: "admin session required" }, { status: 401 });
  }
  if (path === "admin/api/config") return handleAdminApiConfig(request, env);
  if (path === "admin/api/test" && (request.method === "GET" || request.method === "POST")) return handleAdminApiTest(request, env);
  if (path === "admin/chat/settings/save" && request.method === "POST") return json(chatSettingsPayload(request, await runtimeChatEnv(env)));
  if (path === "command/run" && request.method === "POST") return handleCommand(request, env);
  if (path === "registry/actions") return json({ ok: true, actions: commandCatalog() });
  if (path === "admin/dashboard/state") {
  const catalog = await loadCatalog(request);
  const allActivity = await listActivityRecords(env, 5000);
  const activity = allActivity.slice(0, 300);
  const inquiries = await listInquiryRecords(env);
  const sales = await listSaleRecords(env);

  const eventRecords = activity.filter((entry) =>
    !String(entry.event_type || "").startsWith("admin.")
  );

  const adminRecords = activity.filter((entry) =>
    String(entry.event_type || "").startsWith("admin.")
  );

  const trafficEvents = eventRecords.filter((entry) => {
    const type = String(entry.event_type || "");
    return (
      type.includes("page_view") ||
      type.includes("buy") ||
      type.includes("play") ||
      type.includes("analysis") ||
      type.includes("download")
    );
  });

  const completedDownloadIds = new Set(
    allActivity
      .filter((entry) => String(entry.event_type || "") === "download.completed")
      .map((entry) => String(entry.request_id || activityEventId(entry) || ""))
      .filter(Boolean)
  );

  return json({
    ok: true,
    snapshot_at: new Date().toISOString(),
    traffic: {
      status: "live",
      source: env.AIFRED_SALES_LOG ? "Cloudflare KV activity log" : "read-only historical GitHub activity records",
      page_views: eventRecords.filter((entry) => String(entry.event_type || "").includes("page_view")).length,
      api_hits: activity.length,
      media_streams: eventRecords.filter((entry) => String(entry.event_type || "").includes("play")).length,
      downloads: completedDownloadIds.size,
      recent: trafficEvents.slice(0, 12)
    },
      catalog: { tracks: catalog.length, source: "apps/website/assets/data/beat_catalog.json" },
      inquiries: {
        count: inquiries.length,
        latest: inquiries.slice(0, 1)
      },
      sales: {
        count: sales.length,
        latest: sales.slice(0, 1)
      },
      logs: {
        configured: Boolean(env.AIFRED_SALES_LOG),
        events: eventRecords.slice(0, 100),
        adminlog: adminRecords.slice(0, 100)
      },
      analytics: { configured: Boolean(env.AIFRED_ANALYTICS_API_TOKEN), message: env.AIFRED_ANALYTICS_API_TOKEN ? "analytics provider configured" : "live analytics are not configured" },
      deploy: { source: repoConfig(env).repo, branch: repoConfig(env).branch, target: "Cloudflare Pages project aifred-site" }
    });
  }
  if (path === "admin/ops/status" && request.method === "GET") return handleOpsStatus(request, env);
  if (path === "admin/export/site" && request.method === "GET") return exportResponse(await buildSiteExport(request, env), "aifred-site-data-export");
  if (path === "admin/export/tracks" && request.method === "GET") return exportResponse(await buildTrackAnalysisExport(request, env), "aifred-track-analysis-export");
  if (path === "admin/catalog/list") return json({ ok: true, tracks: withR2CatalogUrls(request, await loadCatalog(request)) });
  if (path === "admin/files/read" && request.method === "POST") return handleAdminFileRead(request, env);
  if (path === "admin/files/write" && request.method === "POST") return handleAdminFileWrite(request, env);
  if (path === "admin/files/list") return handleAdminFileList(request, env);
  if (path === "admin/files/delete" && request.method === "POST") return handleAdminFileDelete(request, env);
  if (path === "admin/files/upload" && request.method === "POST") return handleAdminFileUpload(request, env);
  if (path === "admin/reference/upload" && request.method === "POST") return handleAdminReferenceUpload(request, env);
  if (path === "admin/catalog/upload" && request.method === "POST") return handleAdminCatalogUpload(request, env);
  if (path === "admin/inquiries/list") {
    const inquiries = await listInquiryRecords(env);
    return json({
      ok: true,
      configured: Boolean(env.AIFRED_SALES_LOG),
      inquiries,
      message: env.AIFRED_SALES_LOG ? "Inquiries loaded from KV with read-only historical repository records." : "Inquiry persistence requires AIFRED_SALES_LOG KV."
    });
  }
  if (path === "admin/logs/list") {
    const activity = await listActivityRecords(env, 300);
    const adminlog = activity.filter((entry) => String(entry.event_type || "").startsWith("admin."));
    const events = activity.filter((entry) => !String(entry.event_type || "").startsWith("admin."));
    return json({
      ok: true,
      configured: Boolean(env.AIFRED_SALES_LOG),
      logs: activity,
      events,
      adminlog,
      message: env.AIFRED_SALES_LOG ? "Activity loaded from KV with read-only historical repository records." : "New activity persistence requires AIFRED_SALES_LOG KV; historical repository records remain read-only."
    });
  }
  if (path === "admin/sales/list") {
    const sales = await listSaleRecords(env);
    return json({
      ok: true,
      configured: Boolean(env.GITHUB_TOKEN || env.AIFRED_SALES_LOG),
      sales,
      message: env.AIFRED_SALES_LOG ? "Sales records loaded from KV with repository fallback." : env.GITHUB_TOKEN ? "Sales records loaded from repository storage." : "Sales storage requires AIFRED_SALES_LOG KV or GITHUB_TOKEN."
    });
  }
  if (path === "admin/reference/list") {
    const references = await listReferenceRecords(env, 200);
    return json({
      ok: true,
      configured: Boolean(env.AIFRED_REFERENCE_POOL),
      references,
      message: env.AIFRED_REFERENCE_POOL ? "Accepted analyzer references loaded from KV." : "Reference persistence requires AIFRED_REFERENCE_POOL KV."
    });
  }
  if (path === "models/list") {
    const runtimeEnv = await runtimeChatEnv(env);
    const ollamaModel = runtimeEnv.OLLAMA_MODEL || "aifred:latest";
    const openAiModel = runtimeEnv.OPENAI_MODEL || "gpt-5.6-luna";
    return json({
      ok: true,
      models: [ollamaModel, openAiModel].filter(Boolean),
      active_model: ollamaModel,
      providers: {
        openai: { configured: Boolean(runtimeEnv.OPENAI_API_KEY), model: openAiModel },
        ollama: { configured: Boolean(runtimeEnv.OLLAMA_BASE_URL), model: ollamaModel }
      }
    });
  }
  if (path === "chat/ask" && request.method === "POST") return canonicalChat(request, env);
  if (path === "inquiries/submit" && request.method === "POST") {
    const body = await readJson(request);
    const sessionId = String(body.session_id || "").trim();
    const requestId = String(body.request_id || "").trim();
    const inquiry = {
      id: crypto.randomUUID(),
      created_at: new Date().toISOString(),
      name: String(body.name || "").trim(),
      email: String(body.email || "").trim(),
      message: String(body.message || "").trim()
    };
    if (!inquiry.name || !inquiry.email || !inquiry.message) {
      return json({ ok: false, error: "name, email, and message are required" }, { status: 400 });
    }
    const stored = await persistInquiryRecord(env, inquiry);
    const text = [
      "AIFRED inquiry received.",
      `name: ${inquiry.name}`,
      `email: ${inquiry.email}`,
      "",
      inquiry.message
    ].join("\n");
    const html = `
      <h1>AIFRED inquiry received</h1>
      <p><strong>Name:</strong> ${escapeHtml(inquiry.name)}</p>
      <p><strong>Email:</strong> ${escapeHtml(inquiry.email)}</p>
      <pre>${escapeHtml(inquiry.message)}</pre>
    `;
    const emailResult = await sendNotificationEmail(env, {
      to: contactEmail(env),
      from: emailFrom(env),
      subject: `AIFRED inquiry: ${inquiry.name}`,
      text,
      html
    });
    const activity = await recordActivity(env, {
      event_type: "inquiry.submitted",
      session_id: sessionId,
      request_id: requestId,
      actor: { type: "anonymous", id: sessionId },
      source: { surface: "website.inquiry", route: "/api/v1/inquiries/submit" },
      subject: { type: "inquiry", id: inquiry.id },
      operation: { action: "submit", status: "success", result: stored.storage === "kv" ? "stored" : "accepted_unconfigured" },
      metadata: { storage: stored.storage, email_sent: emailResult.ok }
    }, { request });
    const responseBody = {
      ok: true,
      inquiry_id: inquiry.id,
      request_id: activity.event.request_id,
      target_email: contactEmail(env),
      stored: stored.storage === "kv",
      storage: stored.storage,
      email_sent: emailResult.ok
    };
    return json(responseBody);
  }

  return json({ ok: false, error: `unknown route: ${path}` }, { status: 404 });
  } catch (error) {
    if (path.startsWith("admin/") || path === "command/run") {
      await recordActivity(env, {
        event_type: "admin.operation.failed",
        actor: { type: "admin", id: adminActorId(request) },
        source: { surface: "admin", route: `/api/v1/${path}` },
        subject: { type: "admin_operation", id: path, name: path },
        operation: { action: "execute", status: "failure", result: "operation_exception" },
        metadata: { error_type: error?.name || "Error" }
      }, { request });
    }
    return json({ ok: false, error: error.message || "backend route failed", route: path }, { status: 500 });
  }
}

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

function getExpectedAdmin(env) {
  return {
    username: String(env.AIFRED_ADMIN_USERNAME || "North3rnLight3r").trim(),
    passwordHash: String(env.AIFRED_ADMIN_PASSWORD_SHA256 || "c5c5188f8c698dfa5f956f4883f878a212d882fef0c8aed7c49a12c41d9ad8c5").trim().toLowerCase()
  };
}

async function createAdminSession(username, env) {
  const secret = String(env.AIFRED_ADMIN_SESSION_SECRET || env.AIFRED_API_TOKEN || "aifred-local-session").trim();
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
    const secret = String(env.AIFRED_ADMIN_SESSION_SECRET || env.AIFRED_API_TOKEN || "aifred-local-session").trim();
    const expected = await sha256Hex(`${username}|${issuedAt}|${nonce}|${secret}`);
    return expected === sig;
  } catch (_) {
    return false;
  }
}

async function loadCatalog(request) {
  const response = await fetch(new URL("/assets/data/beat_catalog.json", request.url), { cache: "no-store" });
  const tracks = await response.json();
  return Array.isArray(tracks) ? tracks : [];
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
      score: bandScore(metrics.integrated_lufs, -14.0, -9.0, -18.0, -6.5),
      target: "Loudness pro center: -14 to -9 LUFS; acceptable: -18 to -6.5 LUFS"
    },
    {
      id: "peak_dbfs",
      score: ceilingScore(metrics.peak_dbfs, -1.0, 0.0),
      target: "Peak ceiling: -1 dBFS target, accepted up to 0 dBFS when not clipping"
    },
    {
      id: "tone_balance",
      score: bandScore(metrics.tone_balance, 58, 96, 38, 100),
      target: "Tone balance: filled frequency range, broad musical tolerance"
    },
    {
      id: "crest_factor_db",
      score: bandScore(metrics.crest_factor_db, 6.0, 14.5, 3.5, 20.0),
      target: "Dynamics: 6-14.5 dB crest center; small crest differences are not hard rejects"
    },
    {
      id: "stereo_width",
      score: bandScore(metrics.stereo_width, 0.22, 0.92, 0.08, 1.0),
      target: "Stereo width: wide acceptance with mono-safety protection"
    },
    {
      id: "low_end_control",
      score: floorScore(metrics.low_end_control, 48, 22),
      target: "Low-end control: reject only severe mud, not normal genre weight"
    },
    {
      id: "harshness_control",
      score: floorScore(metrics.harshness_control, 46, 18),
      target: "Harshness control: reject only clearly painful upper-mid balance"
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
  const clipping = metrics.peak_dbfs > 0.0;
  return {
    accepted: !clipping && score >= 58,
    score,
    checks: checks.map((check) => ({ id: check.id, ok: check.score >= 35, score: check.score, target: check.target }))
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
    await env.AIFRED_REFERENCE_POOL.put(`reference:${analysisId}`, JSON.stringify(metadata));
    persistence = "stored";
  } else if (gate.accepted) {
    persistence = "accepted-no-binding";
  }

  return json({
    ok: true,
    accepted: gate.accepted,
    score: gate.score,
    action: gate.accepted ? "metadata eligible for the AIFRED reference pool" : "metadata disposed",
    persistence,
    checks: gate.checks,
    analysis_id: gate.accepted ? analysisId : null
  });
}

function contentPayload() {
  return {
    products: [
      {
        sku: "aifred_vst3_windows",
        title: "AIFRED VST3 for Windows",
        description: "FL Studio-first mix alignment plugin with Halo scoring, reference-aware diagnosis, and ranked fix guidance.",
        price: "$149.99",
        availability_label: "Windows build first",
        future_price_label: "Cross-platform builds follow CI validation"
      }
    ],
    services: [
      {
        title: "Mix Diagnostic Pass",
        description: "Tone, width, headroom, punch, and next-step notes in the AIFRED diagnostic language.",
        price: "$49"
      },
      {
        title: "Reference Match Notes",
        description: "A target-aware comparison for producers who need a clear adjustment path.",
        price: "$79"
      }
    ]
  };
}

async function askOpenAI(env, message) {
  const apiKey = env.OPENAI_API_KEY;
  if (!apiKey) throw new Error("OPENAI_API_KEY is not configured");
  const model = env.OPENAI_MODEL || "gpt-5.4-mini";
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

async function askOllama(env, message) {
  const base = String(env.OLLAMA_BASE_URL || "").replace(/\/+$/, "");
  if (!base) throw new Error("OLLAMA_BASE_URL is not configured");
  const model = env.OLLAMA_MODEL || "llama3.1";
  const response = await fetch(`${base}/api/chat`, {
    method: "POST",
    headers: {
      "content-type": "application/json",
      ...(env.OLLAMA_API_TOKEN ? { "authorization": `Bearer ${env.OLLAMA_API_TOKEN}` } : {})
    },
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

function chatSettingsPayload(request, env) {
  const url = new URL(request.url);
  const wsProtocol = url.protocol === "https:" ? "wss:" : "ws:";
  return {
    ok: true,
    websocket_url: `${wsProtocol}//${url.host}/ws/chat`,
    persistence: env.AIFRED_CHAT_SESSIONS ? "bound" : "stateless",
    active_model: env.OPENAI_MODEL || env.OLLAMA_MODEL || "gpt-5.4-mini",
    models: [env.OPENAI_MODEL || "gpt-5.4-mini", env.OLLAMA_MODEL || "llama3.1"].filter(Boolean),
    settings: {
      transport_mode: "websocket",
      webhook: { enabled: false, url: "", secret: "", events: ["chat.completed", "chat.failed"] },
      context: { use_previous_response_id: true, memory_window_items: 40, summary_items: 6, max_prompt_chars: 4000, compact_threshold: 12 },
      prompt: { tone: "direct", personality_mode: "professional_mentor", system_prefix: "", system_suffix: "" },
      reasoning: { enabled: true, effort: "low" },
      response: { verbosity: "low", max_output_tokens: 900 }
    }
  };
}

function commandCatalog() {
  return [
    { id: "health", description: "Check live website API health", command: "health" },
    { id: "catalog:list", description: "Count beat catalog tracks", command: "catalog:list" },
    { id: "models:list", description: "Show configured OpenAI/Ollama model routes", command: "models:list" },
    { id: "reference:stats", description: "Show analyzer reference-pool status", command: "reference:stats" },
    { id: "deploy:status", description: "Show Cloudflare Pages deployment status", command: "deploy:status" },
    { id: "deploy:site", description: "Dispatch GitHub Actions website deployment when GITHUB_TOKEN is configured", command: "deploy:site" }
  ];
}

function repoConfig(env) {
  const repo = String(env.AIFRED_GITHUB_REPO || "kaeganscott26/AIFRED").trim();
  const branch = String(env.AIFRED_GITHUB_BRANCH || "main").trim();
  return { repo, branch };
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
  let deploy = "";
  let deployError = "";
  if (shouldDeploy) {
    try {
      deploy = await dispatchDeploy(env);
    } catch (error) {
      deployError = `Deploy dispatch failed: ${error.message || "unknown error"}`;
    }
  }
  return json({
    ok: true,
    path: relPath,
    commit: payload.commit?.sha || "",
    deploy_dispatched: Boolean(deploy),
    deploy_error: deployError,
    message: deploy || deployError || "website file committed; run deploy:site to publish it"
  });
}

async function handleAdminFileList(request, env) {
  if (!(await verifyAdmin(request, env))) return json({ ok: false, error: "admin session required" }, { status: 401 });
  if (!env.GITHUB_TOKEN) return json({ ok: false, error: "GITHUB_TOKEN is not configured in Cloudflare Pages" }, { status: 501 });
  const url = new URL(request.url);
  const relPath = safeRepoPath(url.searchParams.get("path") || "website");
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
  if (!relPath.startsWith("website/")) return json({ ok: false, error: "mobile delete is restricted to website/" }, { status: 403 });
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
  const targetPath = safeRepoPath(form.get("path") || `website/assets/uploads/${safeUploadName(file.name)}`);
  const written = await writeBinaryRepoFile(env, targetPath, file, `Upload ${targetPath} from AIFRED admin`);
  return json({ ok: true, stored_path: written.path, commit: written.commit });
}

async function handleAdminReferenceUpload(request, env) {
  if (!(await verifyAdmin(request, env))) return json({ ok: false, error: "admin session required" }, { status: 401 });
  const form = await request.formData();
  const file = form.get("file");
  if (!file || typeof file === "string") return json({ ok: false, error: "file is required" }, { status: 400 });
  const genre = String(form.get("genre") || "reference").replace(/[^A-Za-z0-9._-]/g, "-").toLowerCase();
  const targetPath = `website/assets/reference_pool/${genre}/${safeUploadName(file.name)}`;
  const written = await writeBinaryRepoFile(env, targetPath, file, `Upload reference ${targetPath} from AIFRED admin`);
  return json({ ok: true, stored_path: written.path, commit: written.commit });
}

async function handleAdminCatalogUpload(request, env) {
  if (!(await verifyAdmin(request, env))) return json({ ok: false, error: "admin session required" }, { status: 401 });
  const form = await request.formData();
  const file = form.get("file");
  if (!file || typeof file === "string") return json({ ok: false, error: "file is required" }, { status: 400 });
  const title = String(form.get("title") || file.name || "North3rnLight3r catalog upload").trim();
  const fileName = safeUploadName(file.name);
  const audioPath = `website/assets/audio/catalog/${fileName}`;
  const audioWrite = await writeBinaryRepoFile(env, audioPath, file, `Upload catalog audio ${fileName} from AIFRED admin`);
  const tracks = await loadCatalog(request);
  const track = {
    key: crypto.randomUUID(),
    title,
    bpm: String(form.get("bpm") || "").trim(),
    genre: String(form.get("pack_type") || "North3rnLight3r").trim(),
    duration_label: "",
    price: String(form.get("price") || "$19.99").trim(),
    asset_file_name: fileName,
    stream_url: `assets/audio/catalog/${fileName}`,
    artwork_url: "assets/brand/aifred-mascot.jpg",
    description: String(form.get("description") || "").trim(),
    key_signature: String(form.get("key") || "").trim(),
    tempo: String(form.get("tempo") || "").trim()
  };
  const { repo, branch } = repoConfig(env);
  const catalogPath = "website/assets/data/beat_catalog.json";
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
  return json({ ok: true, stored_path: audioWrite.path, track, commit: audioWrite.commit });
}

async function dispatchDeploy(env) {
  const { repo, branch } = repoConfig(env);
  await githubRequest(env, `/repos/${repo}/actions/workflows/build.yml/dispatches`, {
    method: "POST",
    body: JSON.stringify({ ref: branch })
  });
  return "GitHub Actions workflow dispatched for website deployment.";
}

async function handleCommand(request, env) {
  if (!(await verifyAdmin(request, env))) return json({ ok: false, error: "admin session required" }, { status: 401 });
  const body = await readJson(request);
  const command = String(body.command_line || body.command || "").trim();
  const normalized = command.startsWith("action:") ? command.slice(7).trim() : command;
  let stdout = "";
  if (normalized === "health") stdout = JSON.stringify({ ok: true, service: "AIFRED website backend" }, null, 2);
  else if (normalized === "catalog:list") stdout = `tracks=${(await loadCatalog(request)).length}`;
  else if (normalized === "models:list") stdout = JSON.stringify({
    openai: Boolean(env.OPENAI_API_KEY),
    openai_model: env.OPENAI_MODEL || "gpt-5.4-mini",
    ollama: Boolean(env.OLLAMA_BASE_URL),
    ollama_model: env.OLLAMA_MODEL || "llama3.1"
  }, null, 2);
  else if (normalized === "reference:stats") stdout = JSON.stringify({
    reference_pool_binding: Boolean(env.AIFRED_REFERENCE_POOL),
    accepted_uploads: env.AIFRED_REFERENCE_POOL ? "stored in KV" : "accepted metadata is reported but not persisted until KV is bound"
  }, null, 2);
  else if (normalized === "deploy:status") stdout = "Cloudflare Pages production domain: north3rnlight3r.com / www.north3rnlight3r.com";
  else if (normalized === "deploy:site") stdout = await dispatchDeploy(env);
  else return json({ ok: false, exit_code: 2, stderr: "Unsupported command. Use /api/v1/registry/actions for the allowlist." }, { status: 400 });
  return json({ ok: true, exit_code: 0, stdout, stderr: "" });
}

async function handleAdminLogin(request, env) {
  const body = await readJson(request);
  const username = String(body.username || "").trim();
  const password = String(body.password || "");
  const expected = getExpectedAdmin(env);
  const passwordHash = await sha256Hex(password);
  if (username !== expected.username || passwordHash !== expected.passwordHash) {
    return json({ ok: false, error: "invalid admin credentials" }, { status: 401 });
  }
  return json({ ok: true, username, session_token: await createAdminSession(username, env) });
}

export async function onRequest({ request, env, params }) {
  const path = Array.isArray(params.path) ? params.path.join("/") : String(params.path || "");

  if (request.method === "OPTIONS") return new Response(null, { status: 204 });
  try {
  if (path === "health") return json({ ok: true, service: "AIFRED website backend" });
  if (path === "catalog/list") return json({ ok: true, tracks: await loadCatalog(request) });
  if (path === "soundpacks/list") return json({ ok: true, soundpacks: [] });
  if (path === "content/get") return json({ ok: true, content: contentPayload() });
  if (path === "analysis/submit" && request.method === "POST") return handleAnalysisSubmit(request, env);
  if (path === "analyzer/submit" && request.method === "POST") return handleAnalysisSubmit(request, env);
  if (path === "chat/settings") return json(chatSettingsPayload(request, env));
  if (path === "admin/chat/settings/save" && request.method === "POST") return json(chatSettingsPayload(request, env));
  if (path === "admin/login" && request.method === "POST") return handleAdminLogin(request, env);
  if (path === "command/run" && request.method === "POST") return handleCommand(request, env);
  if (path === "registry/actions") return json({ ok: true, actions: commandCatalog() });
  if (path === "admin/dashboard/state") return json({ ok: true, traffic: { status: "live" }, catalog: { tracks: (await loadCatalog(request)).length } });
  if (path === "admin/catalog/list") return json({ ok: true, tracks: await loadCatalog(request) });
  if (path === "admin/files/read" && request.method === "POST") return handleAdminFileRead(request, env);
  if (path === "admin/files/write" && request.method === "POST") return handleAdminFileWrite(request, env);
  if (path === "admin/files/list") return handleAdminFileList(request, env);
  if (path === "admin/files/delete" && request.method === "POST") return handleAdminFileDelete(request, env);
  if (path === "admin/files/upload" && request.method === "POST") return handleAdminFileUpload(request, env);
  if (path === "admin/reference/upload" && request.method === "POST") return handleAdminReferenceUpload(request, env);
  if (path === "admin/catalog/upload" && request.method === "POST") return handleAdminCatalogUpload(request, env);
  if (path === "admin/inquiries/list") return json({ ok: true, inquiries: [] });
  if (path === "admin/logs/list") return json({ ok: true, logs: [] });
  if (path === "admin/sales/list") return json({ ok: true, sales: [] });
  if (path === "admin/sales/record" && request.method === "POST") return json({ ok: true, sale_id: crypto.randomUUID() });
  if (path === "models/list") {
    return json({
      ok: true,
      models: [env.OPENAI_MODEL || "gpt-5.4-mini", env.OLLAMA_MODEL || "llama3.1"].filter(Boolean),
      active_model: env.OPENAI_MODEL || env.OLLAMA_MODEL || "gpt-5.4-mini",
      providers: {
        openai: { configured: Boolean(env.OPENAI_API_KEY), model: env.OPENAI_MODEL || "gpt-5.4-mini" },
        ollama: { configured: Boolean(env.OLLAMA_BASE_URL), model: env.OLLAMA_MODEL || "llama3.1" }
      }
    });
  }
  if (path === "chat/ask" && request.method === "POST") return handleChat(request, env);
  if (path === "inquiries/submit" && request.method === "POST") {
    const body = await readJson(request);
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
    return json({ ok: true, inquiry_id: inquiry.id, target_email: env.AIFRED_CONTACT_EMAIL || "north3rnlight3rofficial@outlook.com" });
  }

  return json({ ok: false, error: `unknown route: ${path}` }, { status: 404 });
  } catch (error) {
    return json({ ok: false, error: error.message || "backend route failed", route: path }, { status: 500 });
  }
}

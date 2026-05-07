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

async function readText(request) {
  try {
    return await request.text();
  } catch (_) {
    return "";
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

async function loadCatalog(request, env) {
  if (env && env.AIFRED_DB) {
    try {
      const { results } = await env.AIFRED_DB.prepare("SELECT * FROM catalog ORDER BY created_at DESC").all();
      if (Array.isArray(results) && results.length > 0) return results;
    } catch (_) {}
  }
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
  await persistActivityRecord(env, {
    event_type: "website.analysis.submit",
    source: "website",
    path: "/api/v1/analysis/submit",
    page: String(body.page || body.path || "").trim(),
    title: String(body.file_name || "browser-analysis").trim(),
    status: gate.accepted ? "accepted" : "rejected",
    message: gate.summary || gate.classification || "analysis submitted",
    details: {
      accepted: gate.accepted,
      score: gate.score,
      classification: gate.classification,
      persistence
    }
  }, `Record analysis ${analysisId}`);

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
  const record = {
    event_type: body.event_type || body.type || body.kind || "site.event",
    source: String(body.source || "website").trim(),
    path: String(body.path || "").trim(),
    page: String(body.page || "").trim(),
    title: String(body.title || "").trim(),
    message: String(body.message || "").trim(),
    session_id: String(body.session_id || body.client_session_id || "").trim(),
    referrer: String(body.referrer || "").trim(),
    status: String(body.status || "").trim(),
    item_name: String(body.item_name || "").trim(),
    amount: String(body.amount || "").trim(),
    currency: String(body.currency || "").trim(),
    order_id: String(body.order_id || "").trim(),
    txn_id: String(body.txn_id || "").trim(),
    custom_id: String(body.custom_id || "").trim(),
    download_token: String(body.download_token || "").trim(),
    actor: String(body.actor || "").trim(),
    details: body.details || {}
  };
  const stored = await persistActivityRecord(env, record, `Record ${normalizeActivityType(record.event_type)}`);
  return json({
    ok: true,
    event_type: normalizeActivityType(record.event_type),
    stored_path: stored.commit?.sha || "",
    configured: Boolean(env.GITHUB_TOKEN || env.AIFRED_SALES_LOG)
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
        price: "$5 one-time beta access",
        availability_label: "2-week launch beta.",
        future_price_label: "$5 launch beta access. Later pricing will be set after the final launch window."
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
        description: "Non-exclusive licenses are $100. Exclusive licenses are handled through inquiry.",
        price: "$100 non-exclusive / $200 exclusive"
      },
      {
        title: "AIFRED VST",
        description: "Visual feedback for tone, width, loudness, punch, reference alignment, and compare workflow.",
        price: "$5 one-time beta access"
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
  const model = env.OLLAMA_MODEL || "aifred:latest";
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

function paypalApiBase(env) {
  return String(env.PAYPAL_ENVIRONMENT || env.AIFRED_PAYPAL_ENVIRONMENT || "").toLowerCase() === "sandbox"
    ? "https://api-m.sandbox.paypal.com"
    : "https://api-m.paypal.com";
}

async function paypalAccessToken(env) {
  const clientId = String(env.PAYPAL_CLIENT_ID || "").trim();
  const secret = String(env.PAYPAL_CLIENT_SECRET || "").trim();
  if (!clientId || !secret) throw new Error("PAYPAL_CLIENT_ID and PAYPAL_CLIENT_SECRET are required");
  const response = await fetch(`${paypalApiBase(env)}/v1/oauth2/token`, {
    method: "POST",
    headers: {
      "authorization": `Basic ${btoa(`${clientId}:${secret}`)}`,
      "content-type": "application/x-www-form-urlencoded"
    },
    body: "grant_type=client_credentials"
  });
  const payload = await response.json().catch(() => ({}));
  if (!response.ok) throw new Error(payload?.error_description || payload?.error || "PayPal token request failed");
  return payload.access_token;
}

function paypalCheckoutConfig(env) {
  return {
    client_id: String(env.PAYPAL_CLIENT_ID || "").trim(),
    configured: Boolean(env.PAYPAL_CLIENT_ID && env.PAYPAL_CLIENT_SECRET),
    environment: String(env.PAYPAL_ENVIRONMENT || env.AIFRED_PAYPAL_ENVIRONMENT || "live").toLowerCase() === "sandbox" ? "sandbox" : "live",
    amount: String(env.AIFRED_PAYPAL_AMOUNT || "5.00").trim(),
    currency: String(env.AIFRED_PAYPAL_CURRENCY || "USD").trim().toUpperCase(),
    item_name: String(env.AIFRED_PAYPAL_ITEM_NAME || "AIFRED Plugin (download)").trim()
  };
}

async function handlePayPalConfig(request, env) {
  return json({ ok: true, paypal: paypalCheckoutConfig(env) });
}

async function handlePayPalCreateOrder(request, env) {
  const cfg = paypalCheckoutConfig(env);
  if (!cfg.configured) return json({ ok: false, error: "PayPal checkout is not configured" }, { status: 503 });
  const body = await readJson(request);
  const customId = String(body.custom_id || body.custom || crypto.randomUUID()).trim().slice(0, 127);
  await persistActivityRecord(env, {
    event_type: "paypal.order.create.requested",
    source: "website",
    path: "/api/v1/paypal/create-order",
    title: cfg.item_name,
    item_name: cfg.item_name,
    amount: cfg.amount,
    currency: cfg.currency,
    custom_id: customId,
    status: "requested",
    details: { custom_id: customId, environment: paypalEnvironment(env) }
  }, `Request PayPal order for ${customId}`);
  const token = await paypalAccessToken(env);
  const response = await fetch(`${paypalApiBase(env)}/v2/checkout/orders`, {
    method: "POST",
    headers: {
      "authorization": `Bearer ${token}`,
      "content-type": "application/json",
      "prefer": "return=representation"
    },
    body: JSON.stringify({
      intent: "CAPTURE",
      purchase_units: [{
        description: cfg.item_name,
        custom_id: customId,
        amount: {
          currency_code: cfg.currency,
          value: cfg.amount
        },
        items: [{
          name: cfg.item_name,
          quantity: "1",
          unit_amount: {
            currency_code: cfg.currency,
            value: cfg.amount
          }
        }]
      }],
      application_context: {
        brand_name: "AIFRED",
        shipping_preference: "NO_SHIPPING",
        user_action: "PAY_NOW",
        return_url: new URL("/?purchase=success#vst", request.url).toString(),
        cancel_url: new URL("/?purchase=cancelled#vst", request.url).toString()
      }
    })
  });
  const payload = await response.json().catch(() => ({}));
  if (!response.ok) {
    await persistActivityRecord(env, {
      event_type: "paypal.order.create.failed",
      source: "website",
      path: "/api/v1/paypal/create-order",
      title: cfg.item_name,
      item_name: cfg.item_name,
      amount: cfg.amount,
      currency: cfg.currency,
      custom_id: customId,
      status: "failed",
      message: payload?.message || payload?.details?.[0]?.description || "PayPal order create failed",
      details: payload
    }, `Fail PayPal order for ${customId}`);
    return json({ ok: false, error: payload?.message || payload?.details?.[0]?.description || "PayPal order create failed" }, { status: 502 });
  }
  await persistActivityRecord(env, {
    event_type: "paypal.order.created",
    source: "website",
    path: "/api/v1/paypal/create-order",
    title: cfg.item_name,
    item_name: cfg.item_name,
    amount: cfg.amount,
    currency: cfg.currency,
    custom_id: customId,
    order_id: payload.id || "",
    status: String(payload.status || "CREATED").toLowerCase(),
    details: payload
  }, `Created PayPal order ${payload.id || customId}`);
  return json({ ok: true, id: payload.id, status: payload.status });
}

function captureSummary(capturePayload) {
  const unit = capturePayload?.purchase_units?.[0] || {};
  const capture = unit?.payments?.captures?.[0] || {};
  const payer = capturePayload?.payer || {};
  const payerName = [payer.name?.given_name, payer.name?.surname].filter(Boolean).join(" ").trim();
  return {
    order_id: String(capturePayload?.id || "").trim(),
    capture_id: String(capture.id || "").trim(),
    status: String(capture.status || capturePayload?.status || "").trim(),
    payer_email: String(payer.email_address || "").trim(),
    payer_name: payerName,
    amount: String(capture.amount?.value || "").trim(),
    currency: String(capture.amount?.currency_code || "").trim().toUpperCase(),
    custom: String(unit.custom_id || "").trim()
  };
}

async function fulfillCapturedSale(request, env, summary, source) {
  const cfg = paypalCheckoutConfig(env);
  if (summary.status !== "COMPLETED" && summary.status !== "Completed") {
    return json({ ok: false, error: `PayPal capture is ${summary.status || "not completed"}` }, { status: 402 });
  }
  if (summary.currency !== cfg.currency || Number.parseFloat(summary.amount).toFixed(2) !== Number.parseFloat(cfg.amount).toFixed(2)) {
    return json({ ok: false, error: "PayPal capture amount did not match AIFRED price" }, { status: 409 });
  }
  const existing = await listSaleRecords(env);
  const txnId = summary.capture_id || summary.order_id;
  const duplicate = existing.find((sale) => sale.txn_id === txnId || sale.order_id === summary.order_id);
  if (duplicate?.download_token) {
    return json({
      ok: true,
      sale: duplicate,
      downloads: {
        setup: buildDownloadUrl(request, duplicate.download_token, "setup"),
        zip: buildDownloadUrl(request, duplicate.download_token, "zip")
      }
    });
  }

  const downloadToken = base64Url(crypto.getRandomValues(new Uint8Array(24)));
  const sale = {
    id: crypto.randomUUID(),
    created_at: new Date().toISOString(),
    source,
    order_id: summary.order_id,
    txn_id: txnId,
    custom: summary.custom,
    payer_email: summary.payer_email,
    payer_name: summary.payer_name,
    payment_status: "Completed",
    amount: Number.parseFloat(summary.amount).toFixed(2),
    currency: summary.currency,
    item_name: cfg.item_name,
    receiver_email: paypalBusiness(env).toLowerCase(),
    download_token: downloadToken,
    release_tag: pluginReleaseConfig(env).tag
  };
  await persistSaleRecord(env, sale, `Record PayPal sale ${txnId}`);
  await persistActivityRecord(env, {
    event_type: "paypal.sale.completed",
    source: "website",
    path: "/api/v1/paypal/capture-order",
    title: cfg.item_name,
    item_name: cfg.item_name,
    amount: sale.amount,
    currency: sale.currency,
    order_id: sale.order_id,
    txn_id: sale.txn_id,
    custom_id: sale.custom,
    download_token: sale.download_token,
    status: sale.payment_status,
    details: sale
  }, `Complete PayPal sale ${txnId}`);
  const setupUrl = buildDownloadUrl(request, downloadToken, "setup");
  const zipUrl = buildDownloadUrl(request, downloadToken, "zip");
  await sendSaleEmails(env, sale, setupUrl, zipUrl);
  return json({ ok: true, sale, downloads: { setup: setupUrl, zip: zipUrl } });
}

async function handlePayPalCaptureOrder(request, env) {
  const body = await readJson(request);
  const orderId = String(body.order_id || body.orderID || "").trim();
  if (!orderId) return json({ ok: false, error: "order_id is required" }, { status: 400 });
  await persistActivityRecord(env, {
    event_type: "paypal.order.capture.requested",
    source: "website",
    path: "/api/v1/paypal/capture-order",
    order_id: orderId,
    status: "requested",
    details: { order_id: orderId }
  }, `Request PayPal capture ${orderId}`);
  const token = await paypalAccessToken(env);
  const response = await fetch(`${paypalApiBase(env)}/v2/checkout/orders/${encodeURIComponent(orderId)}/capture`, {
    method: "POST",
    headers: {
      "authorization": `Bearer ${token}`,
      "content-type": "application/json",
      "prefer": "return=representation"
    }
  });
  const payload = await response.json().catch(() => ({}));
  if (!response.ok) {
    await persistActivityRecord(env, {
      event_type: "paypal.order.capture.failed",
      source: "website",
      path: "/api/v1/paypal/capture-order",
      order_id: orderId,
      status: "failed",
      message: payload?.message || payload?.details?.[0]?.description || "PayPal order capture failed",
      details: payload
    }, `Fail PayPal capture ${orderId}`);
    return json({ ok: false, error: payload?.message || payload?.details?.[0]?.description || "PayPal order capture failed" }, { status: 502 });
  }
  await persistActivityRecord(env, {
    event_type: "paypal.order.captured",
    source: "website",
    path: "/api/v1/paypal/capture-order",
    order_id: orderId,
    status: "completed",
    details: payload
  }, `Captured PayPal order ${orderId}`);
  return fulfillCapturedSale(request, env, captureSummary(payload), "paypal-orders-capture");
}

function chatSettingsPayload(request, env) {
  const url = new URL(request.url);
  const wsProtocol = url.protocol === "https:" ? "wss:" : "ws:";
  const ollamaModel = env.OLLAMA_MODEL || "aifred:latest";
  const openAiModel = env.OPENAI_MODEL || "gpt-5.4-mini";
  return {
    ok: true,
    websocket_url: `${wsProtocol}//${url.host}/ws/chat`,
    persistence: env.AIFRED_CHAT_SESSIONS ? "bound" : "stateless",
    active_model: ollamaModel,
    models: [ollamaModel, openAiModel].filter(Boolean),
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
    { id: "sales:list", description: "Show recorded PayPal beta sales", command: "sales:list" },
    { id: "inquiries:list", description: "Show recorded contact inquiries", command: "inquiries:list" }
  ];
}

function repoConfig(env) {
  const repo = String(env.AIFRED_GITHUB_REPO || "kaeganscott26/aifred-site").trim();
  const branch = String(env.AIFRED_GITHUB_BRANCH || "main").trim();
  return { repo, branch };
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

function paypalBusiness(env) {
  return String(env.AIFRED_PAYPAL_BUSINESS || contactEmail(env)).trim();
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

function normalizeActivityType(value) {
  const text = String(value || "site.event").trim().toLowerCase();
  const normalized = text
    .replace(/[^a-z0-9._-]+/g, ".")
    .replace(/\.{2,}/g, ".")
    .replace(/^\.+|\.+$/g, "");
  return normalized || "site.event";
}

function activityCategory(type) {
  const head = String(type || "").split(".")[0];
  switch (head) {
    case "admin":
      return "admin";
    case "paypal":
      return "paypal";
    case "website":
      return "website";
    case "analysis":
      return "analysis";
    case "inquiry":
      return "inquiry";
    case "catalog":
      return "catalog";
    case "sale":
      return "sale";
    case "download":
      return "download";
    default:
      return "site";
  }
}

async function persistActivityRecord(env, record, message) {
  const normalizedType = normalizeActivityType(record.event_type || record.type || record.kind);
  const normalized = {
    id: String(record.id || crypto.randomUUID()).trim(),
    created_at: String(record.created_at || new Date().toISOString()).trim(),
    event_type: normalizedType,
    category: String(record.category || activityCategory(normalizedType)).trim(),
    source: String(record.source || "website").trim(),
    path: String(record.path || "").trim(),
    page: String(record.page || "").trim(),
    title: String(record.title || "").trim(),
    message: String(record.message || "").trim(),
    session_id: String(record.session_id || record.client_session_id || "").trim(),
    referrer: String(record.referrer || "").trim(),
    user_agent: String(record.user_agent || "").trim(),
    item_name: String(record.item_name || "").trim(),
    amount: String(record.amount || "").trim(),
    currency: String(record.currency || "").trim(),
    order_id: String(record.order_id || "").trim(),
    txn_id: String(record.txn_id || "").trim(),
    custom_id: String(record.custom_id || "").trim(),
    download_token: String(record.download_token || "").trim(),
    status: String(record.status || "").trim(),
    actor: String(record.actor || "").trim(),
    details: record.details || {}
  };

  if (env.AIFRED_SALES_LOG && typeof env.AIFRED_SALES_LOG.put === "function") {
    await env.AIFRED_SALES_LOG.put(`activity:${normalized.id}`, JSON.stringify(normalized));
  }
  return appendRepoJsonRecord(env, activityRepoPath(), normalized, message);
}

async function listActivityRecords(env, limit = 300) {
  const records = [];
  if (env.AIFRED_SALES_LOG && typeof env.AIFRED_SALES_LOG.list === "function") {
    const listed = await env.AIFRED_SALES_LOG.list({ prefix: "activity:", limit });
    for (const key of listed.keys || []) {
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
    const id = String(record?.id || record?.txn_id || record?.order_id || record?.created_at || "").trim();
    const key = id || JSON.stringify(record);
    if (seen.has(key)) continue;
    seen.add(key);
    merged.push(record);
  }
  return merged.sort((a, b) => String(b.created_at || "").localeCompare(String(a.created_at || ""))).slice(0, limit);
}

async function persistSaleRecord(env, sale, message) {
  if (env.AIFRED_SALES_LOG && typeof env.AIFRED_SALES_LOG.put === "function") {
    await env.AIFRED_SALES_LOG.put(`sale:${sale.txn_id}`, JSON.stringify(sale));
    await env.AIFRED_SALES_LOG.put(`token:${sale.download_token}`, JSON.stringify(sale));
  }
  return appendRepoJsonRecord(env, salesRepoPath(), sale, message);
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

async function findSaleByDownloadToken(env, token) {
  if (env.AIFRED_SALES_LOG && typeof env.AIFRED_SALES_LOG.get === "function") {
    const raw = await env.AIFRED_SALES_LOG.get(`token:${token}`);
    if (raw) {
      try {
        return JSON.parse(raw);
      } catch (_) {}
    }
  }
  const sales = await readRepoJsonArray(env, salesRepoPath());
  return sales.find((entry) => entry.download_token === token) || null;
}

async function sendNotificationEmail(env, payload) {
  if (env.MAILER && typeof env.MAILER.fetch === "function") {
    try {
      const response = await env.MAILER.fetch("https://mailer.internal/send", {
        method: "POST",
        headers: {
          "content-type": "application/json",
          ...(env.MAILER_SHARED_TOKEN ? { "x-aifred-mailer-token": env.MAILER_SHARED_TOKEN } : {})
        },
        body: JSON.stringify(payload)
      });
      const result = await response.json().catch(() => ({}));
      if (!response.ok) {
        return { ok: false, error: result?.error || `mailer returned ${response.status}` };
      }
      return { ok: true, id: result?.id || "" };
    } catch (error) {
      return { ok: false, error: error.message || "mailer request failed" };
    }
  }

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
  return "";
}

function releaseAssetObjectKey(env, assetName) {
  const version = String(env.AIFRED_RELEASE_VERSION || pluginReleaseConfig(env).tag).trim();
  return `releases/${version}/${assetName}`;
}

function buildDownloadUrl(request, token, assetKey) {
  const url = new URL("/api/v1/sales/download", request.url);
  url.searchParams.set("token", token);
  url.searchParams.set("asset", assetKey);
  return url.toString();
}

async function fetchReleaseAssetResponse(env, assetName) {
  if (env.AIFRED_DOWNLOADS && typeof env.AIFRED_DOWNLOADS.get === "function") {
    const objectKey = releaseAssetObjectKey(env, assetName);
    const object = await env.AIFRED_DOWNLOADS.get(objectKey);
    if (object) {
      const headers = new Headers();
      headers.set("cache-control", "no-store");
      headers.set("content-disposition", `attachment; filename="${assetName}"`);
      headers.set("content-type", object.httpMetadata?.contentType || "application/octet-stream");
      headers.set("content-length", String(object.size));
      headers.set("x-aifred-download-source", "r2");
      return new Response(object.body, { status: 200, headers });
    }
  }

  if (!env.GITHUB_TOKEN) throw new Error("GITHUB_TOKEN is not configured");
  const { repo, tag } = pluginReleaseConfig(env);
  const release = await githubRequest(env, `/repos/${repo}/releases/tags/${encodeURIComponent(tag)}`);
  const asset = Array.isArray(release.assets) ? release.assets.find((entry) => entry.name === assetName) : null;
  if (!asset?.url) throw new Error(`release asset not found: ${assetName}`);

  const response = await fetch(asset.url, {
    headers: {
      accept: "application/octet-stream",
      authorization: `Bearer ${env.GITHUB_TOKEN}`,
      "user-agent": "aifred-site"
    }
  });

  if (!response.ok) {
    throw new Error(`asset download failed (${response.status})`);
  }

  const headers = new Headers();
  headers.set("cache-control", "no-store");
  headers.set("content-disposition", `attachment; filename="${assetName}"`);
  headers.set("content-type", response.headers.get("content-type") || "application/octet-stream");
  const contentLength = response.headers.get("content-length");
  if (contentLength) headers.set("content-length", contentLength);
  return new Response(response.body, { status: 200, headers });
}

async function handleSalesDownload(request, env) {
  const url = new URL(request.url);
  const token = String(url.searchParams.get("token") || "").trim();
  const assetKey = String(url.searchParams.get("asset") || "").trim();
  const assetName = assetNameForKey(assetKey);
  if (!token || !assetName) return json({ ok: false, error: "valid token and asset are required" }, { status: 400 });

  const sale = await findSaleByDownloadToken(env, token);
  if (!sale || sale.payment_status !== "Completed") {
    return json({ ok: false, error: "download token is invalid" }, { status: 403 });
  }

  await persistActivityRecord(env, {
    event_type: "website.download.requested",
    source: "website",
    path: "/api/v1/sales/download",
    item_name: sale.item_name || "AIFRED Plugin (download)",
    amount: sale.amount,
    currency: sale.currency,
    order_id: sale.order_id,
    txn_id: sale.txn_id,
    download_token: token,
    status: "served",
    details: { asset: assetKey }
  }, `Serve download ${assetKey} for ${sale.txn_id || sale.order_id || token}`);

  return fetchReleaseAssetResponse(env, assetName);
}

async function handleManualSaleRecord(request, env) {
  if (!(await verifyAdmin(request, env))) return json({ ok: false, error: "admin session required" }, { status: 401 });
  const body = await readJson(request);
  const record = {
    id: crypto.randomUUID(),
    created_at: new Date().toISOString(),
    source: "admin-manual",
    payer_email: String(body.payer_email || "").trim(),
    payer_name: String(body.payer_name || "").trim(),
    amount: String(body.amount || "5.00").trim(),
    currency: String(body.currency || "USD").trim(),
    payment_status: String(body.payment_status || "Completed").trim(),
    txn_id: String(body.txn_id || crypto.randomUUID()).trim(),
    custom: String(body.custom || "").trim(),
    download_token: base64Url(crypto.getRandomValues(new Uint8Array(24)))
  };
  const written = await persistSaleRecord(env, record, `Record sale ${record.txn_id} from AIFRED admin`);
  await persistActivityRecord(env, {
    event_type: "admin.sale.record",
    source: "admin",
    path: "/api/v1/admin/sales/record",
    item_name: "AIFRED Plugin (download)",
    amount: record.amount,
    currency: record.currency,
    txn_id: record.txn_id,
    status: record.payment_status,
    details: record
  }, `Record admin sale ${record.txn_id}`);
  return json({ ok: true, sale: record, commit: written.commit });
}

async function sendSaleEmails(env, sale, setupUrl, zipUrl) {
  const ownerRecipient = contactEmail(env);
  const buyerLabel = sale.payer_email || sale.txn_id || sale.order_id || "AIFRED sale";
  const ownerSubject = `AIFRED sale received: ${buyerLabel}`;
  const ownerText = [
    "AIFRED sale received.",
    `payer: ${sale.payer_name || "Unknown payer"} <${sale.payer_email || "no-email"}>`,
    `txn_id: ${sale.txn_id || "n/a"}`,
    `order_id: ${sale.order_id || "n/a"}`,
    `amount: ${sale.amount} ${sale.currency}`,
    `setup link: ${setupUrl}`,
    `zip link: ${zipUrl}`
  ].join("\n");
  const ownerHtml = `
    <h1>AIFRED sale received</h1>
    <p><strong>Payer:</strong> ${escapeHtml(sale.payer_name || "Unknown payer")} &lt;${escapeHtml(sale.payer_email || "no-email")}&gt;</p>
    <p><strong>Transaction:</strong> ${escapeHtml(sale.txn_id || "n/a")}</p>
    <p><strong>Order:</strong> ${escapeHtml(sale.order_id || "n/a")}</p>
    <p><strong>Amount:</strong> ${escapeHtml(sale.amount)} ${escapeHtml(sale.currency)}</p>
    <p><a href="${escapeHtml(setupUrl)}">Windows installer</a></p>
    <p><a href="${escapeHtml(zipUrl)}">Windows zip</a></p>
  `;

  await sendNotificationEmail(env, {
    to: ownerRecipient,
    from: emailFrom(env),
    subject: ownerSubject,
    text: ownerText,
    html: ownerHtml
  });

  if (!sale.payer_email) return;

  const buyerSubject = "AIFRED beta access download links";
  const buyerText = [
    "Thanks for purchasing AIFRED beta access.",
    "",
    `Windows installer: ${setupUrl}`,
    `Windows zip: ${zipUrl}`,
    "",
    "Lifetime beta updates are included in this beta window."
  ].join("\n");
  const buyerHtml = `
    <h1>AIFRED beta access</h1>
    <p>Thanks for purchasing AIFRED beta access.</p>
    <p><a href="${escapeHtml(setupUrl)}">Download the Windows installer</a></p>
    <p><a href="${escapeHtml(zipUrl)}">Download the Windows zip</a></p>
    <p>Lifetime beta updates are included in this beta window.</p>
  `;

  await sendNotificationEmail(env, {
    to: sale.payer_email,
    from: emailFrom(env),
    subject: buyerSubject,
    text: buyerText,
    html: buyerHtml
  });
}

async function handlePayPalIpn(request, env) {
  const rawBody = await readText(request);
  if (!rawBody) return new Response("", { status: 200 });

  const params = new URLSearchParams(rawBody);
  const verificationUrl = params.get("test_ipn") === "1"
    ? "https://ipnpb.sandbox.paypal.com/cgi-bin/webscr"
    : "https://ipnpb.paypal.com/cgi-bin/webscr";

  const verification = await fetch(verificationUrl, {
    method: "POST",
    headers: {
      "content-type": "application/x-www-form-urlencoded",
      "user-agent": "AIFRED-PayPal-IPN"
    },
    body: `cmd=_notify-validate&${rawBody}`
  });

  const verificationText = (await verification.text()).trim();
  if (verificationText !== "VERIFIED") {
    return new Response("", { status: 200 });
  }

  const txnId = String(params.get("txn_id") || "").trim();
  const payerEmail = String(params.get("payer_email") || "").trim();
  const payerName = [params.get("first_name"), params.get("last_name")].filter(Boolean).join(" ").trim();
  const paymentStatus = String(params.get("payment_status") || "").trim();
  const receiverEmail = String(params.get("receiver_email") || "").trim().toLowerCase();
  const receiverExpected = paypalBusiness(env).toLowerCase();
  const amount = Number.parseFloat(String(params.get("mc_gross") || params.get("payment_gross") || "0"));
  const currency = String(params.get("mc_currency") || "").trim().toUpperCase();

  if (!txnId || paymentStatus !== "Completed" || receiverEmail !== receiverExpected || currency !== "USD" || Number.isNaN(amount) || amount.toFixed(2) !== "5.00") {
    return new Response("", { status: 200 });
  }

  const sales = await readRepoJsonArray(env, salesRepoPath());
  if (sales.some((sale) => sale.txn_id === txnId)) {
    return new Response("", { status: 200 });
  }

  await persistActivityRecord(env, {
    event_type: "paypal.ipn.completed",
    source: "website",
    path: "/api/v1/paypal/ipn",
    txn_id: txnId,
    item_name: "AIFRED Plugin (download)",
    amount: amount.toFixed(2),
    currency,
    status: paymentStatus.toLowerCase(),
    details: {
      payment_status: paymentStatus,
      receiver_email: receiverEmail
    }
  }, `Confirm PayPal IPN ${txnId}`);
  const downloadToken = base64Url(crypto.getRandomValues(new Uint8Array(24)));
  const sale = {
    id: crypto.randomUUID(),
    created_at: new Date().toISOString(),
    source: "paypal-ipn",
    txn_id: txnId,
    custom: String(params.get("custom") || "").trim(),
    payer_email: payerEmail,
    payer_name: payerName,
    payment_status: paymentStatus,
    amount: amount.toFixed(2),
    currency,
    item_name: String(params.get("item_name") || "AIFRED Plugin (download)").trim(),
    receiver_email: receiverEmail,
    download_token: downloadToken,
    release_tag: pluginReleaseConfig(env).tag
  };

  await persistSaleRecord(env, sale, `Record PayPal sale ${txnId}`);

  const setupUrl = buildDownloadUrl(request, downloadToken, "setup");
  const zipUrl = buildDownloadUrl(request, downloadToken, "zip");
  await sendSaleEmails(env, sale, setupUrl, zipUrl);

  return new Response("", { status: 200 });
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
  await persistActivityRecord(env, {
    event_type: "admin.file.write",
    source: "admin",
    path: relPath,
    title: relPath,
    status: "saved",
    details: { deploy: Boolean(shouldDeploy), length: content.length }
  }, `Write ${relPath}`);
  return json({
    ok: true,
    path: relPath,
    commit: payload.commit?.sha || "",
    deploy_dispatched: false,
    deploy_error: "",
    message: shouldDeploy
      ? "website file committed; Cloudflare Pages deploys from the repository branch configuration"
      : "website file committed"
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
  await persistActivityRecord(env, {
    event_type: "admin.file.delete",
    source: "admin",
    path: relPath,
    title: relPath,
    status: "deleted",
    details: { sha: existing.sha || "" }
  }, `Delete ${relPath}`);
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
  await persistActivityRecord(env, {
    event_type: "admin.file.upload",
    source: "admin",
    path: targetPath,
    title: file.name,
    status: "uploaded",
    details: { size: file.size || 0, content_type: file.type || "" }
  }, `Upload ${targetPath}`);
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
  await persistActivityRecord(env, {
    event_type: "admin.reference.upload",
    source: "admin",
    path: targetPath,
    title: file.name,
    status: "uploaded",
    details: { genre, size: file.size || 0 }
  }, `Upload reference ${targetPath}`);
  return json({ ok: true, stored_path: written.path, commit: written.commit });
}

async function handleAdminCatalogUpload(request, env) {
  if (!(await verifyAdmin(request, env))) return json({ ok: false, error: "admin session required" }, { status: 401 });
  const form = await request.formData();
  const file = form.get("file");
  if (!file || typeof file === "string") return json({ ok: false, error: "file is required" }, { status: 400 });
  const title = String(form.get("title") || file.name || "North3rnLight3r catalog upload").trim();
  const fileName = safeUploadName(file.name);

  // Save to R2 if available
  if (env.AIFRED_CATALOG) {
    try {
      await env.AIFRED_CATALOG.put(`audio/${fileName}`, file.stream(), {
        httpMetadata: { contentType: file.type || "audio/mpeg" }
      });
    } catch (_) {}
  }

  const audioPath = `website/assets/audio/catalog/${fileName}`;
  const audioWrite = await writeBinaryRepoFile(env, audioPath, file, `Upload catalog audio ${fileName} from AIFRED admin`);
  const tracks = await loadCatalog(request, env);
  const track = {
    key: crypto.randomUUID(),
    title,
    bpm: String(form.get("bpm") || "").trim(),
    genre: String(form.get("pack_type") || "North3rnLight3r").trim(),
    duration_label: "",
    price: String(form.get("price") || "$100").trim(),
    asset_file_name: fileName,
    stream_url: `assets/audio/catalog/${fileName}`,
    artwork_url: "assets/brand/aifred-mascot.jpg",
    description: String(form.get("description") || "").trim(),
    key_signature: String(form.get("key") || "").trim(),
    tempo: String(form.get("tempo") || "").trim()
  };

  // Save to D1 if available
  if (env.AIFRED_DB) {
    try {
      await env.AIFRED_DB.prepare(`
        INSERT INTO catalog (id, title, bpm, genre, price, asset_file_name, stream_url, artwork_url, description, key_signature, tempo, created_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
      `).bind(
        track.key, track.title, track.bpm, track.genre, track.price, track.asset_file_name,
        track.stream_url, track.artwork_url, track.description, track.key_signature, track.tempo, Date.now()
      ).run();
    } catch (_) {}
  }

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
  await persistActivityRecord(env, {
    event_type: "admin.catalog.upload",
    source: "admin",
    path: catalogPath,
    title,
    item_name: title,
    amount: track.price,
    status: "uploaded",
    details: track
  }, `Upload catalog item ${title}`);
  return json({ ok: true, stored_path: audioWrite.path, track, commit: audioWrite.commit });
}

async function handleCommand(request, env) {
  if (!(await verifyAdmin(request, env))) return json({ ok: false, error: "admin session required" }, { status: 401 });
  const body = await readJson(request);
  const command = String(body.command_line || body.command || "").trim();
  const normalized = command.startsWith("action:") ? command.slice(7).trim() : command;
  await persistActivityRecord(env, {
    event_type: "admin.command.run",
    source: "admin",
    path: "/api/v1/command/run",
    title: normalized || command,
    status: "executed",
    details: { command: normalized || command }
  }, `Run command ${normalized || command}`);
  let stdout = "";
  if (normalized === "health") stdout = JSON.stringify({ ok: true, service: "AIFRED website backend" }, null, 2);
  else if (normalized === "catalog:list") stdout = `tracks=${(await loadCatalog(request, env)).length}`;
  else if (normalized === "models:list") stdout = JSON.stringify({
    openai: Boolean(env.OPENAI_API_KEY),
    openai_model: env.OPENAI_MODEL || "gpt-5.4-mini",
    ollama: Boolean(env.OLLAMA_BASE_URL),
    ollama_model: env.OLLAMA_MODEL || "aifred:latest"
  }, null, 2);
  else if (normalized === "reference:stats") stdout = JSON.stringify({
    reference_pool_binding: Boolean(env.AIFRED_REFERENCE_POOL),
    reference_bucket_binding: Boolean(env.AIFRED_REFERENCE_BUCKET),
    accepted_uploads: env.AIFRED_REFERENCE_POOL ? "stored in KV and mirrored to R2 when bound" : "accepted metadata is reported but not persisted until KV is bound"
  }, null, 2);
  else if (normalized === "deploy:status") stdout = "Cloudflare Pages project: aifred-site. Production domains: north3rnlight3r.com and aifred-site.pages.dev.";
  else if (normalized === "sales:list") stdout = JSON.stringify(await listSaleRecords(env), null, 2);
  else if (normalized === "inquiries:list") stdout = JSON.stringify(await readRepoJsonArray(env, inquiriesRepoPath()), null, 2);
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
  await persistActivityRecord(env, {
    event_type: "admin.login.success",
    source: "admin",
    path: "/api/v1/admin/login",
    title: username,
    actor: username,
    status: "authenticated"
  }, `Admin login ${username}`);
  return json({ ok: true, username, session_token: await createAdminSession(username, env) });
}

export async function onRequest({ request, env, params }) {
  const path = Array.isArray(params.path) ? params.path.join("/") : String(params.path || "");

  if (request.method === "OPTIONS") return new Response(null, { status: 204 });
  try {
  if (path === "health") return json({ ok: true, service: "AIFRED website backend" });
  if (path === "catalog/list") return json({ ok: true, tracks: await loadCatalog(request, env) });
  if (path === "soundpacks/list") return json({ ok: true, soundpacks: [] });
  if (path === "content/get") return json({ ok: true, content: contentPayload() });
  if (path === "activity/record" && request.method === "POST") return handleActivityRecord(request, env);
  if (path === "analysis/submit" && request.method === "POST") return handleAnalysisSubmit(request, env);
  if (path === "analyzer/submit" && request.method === "POST") return handleAnalysisSubmit(request, env);
  if (path === "chat/settings") return json(chatSettingsPayload(request, env));
  if (path === "admin/chat/settings/save" && request.method === "POST") return json(chatSettingsPayload(request, env));
  if (path === "admin/login" && request.method === "POST") return handleAdminLogin(request, env);
  if (path === "command/run" && request.method === "POST") return handleCommand(request, env);
  if (path === "registry/actions") return json({ ok: true, actions: commandCatalog() });
  if (path === "admin/dashboard/state") {
    const catalog = await loadCatalog(request, env);
    const activity = await listActivityRecords(env, 300);
    const inquiries = await readRepoJsonArray(env, inquiriesRepoPath());
    const sales = await listSaleRecords(env);
    const eventRecords = activity.filter((entry) => !String(entry.event_type || "").startsWith("admin."));
    const adminRecords = activity.filter((entry) => String(entry.event_type || "").startsWith("admin."));
    const trafficEvents = eventRecords.filter((entry) => {
      const type = String(entry.event_type || "");
      return type.includes("page_view") || type.includes("buy") || type.includes("play") || type.includes("analysis") || type.includes("download");
    });
    return json({
      ok: true,
      snapshot_at: new Date().toISOString(),
      traffic: {
        status: "live",
        source: env.AIFRED_SALES_LOG ? "Cloudflare KV and GitHub activity log" : "GitHub activity log",
        page_views: eventRecords.filter((entry) => String(entry.event_type || "").includes("page_view")).length,
        api_hits: activity.length,
        media_streams: eventRecords.filter((entry) => String(entry.event_type || "").includes("play")).length,
        downloads: eventRecords.filter((entry) => String(entry.event_type || "").includes("download")).length,
        recent: trafficEvents.slice(0, 12)
      },
      catalog: { tracks: catalog.length, source: "website/assets/data/beat_catalog.json" },
      inquiries: {
        count: inquiries.length,
        latest: inquiries.slice(0, 1)
      },
      sales: {
        count: sales.length,
        latest: sales.slice(0, 1)
      },
      logs: {
        configured: Boolean(env.GITHUB_TOKEN || env.AIFRED_SALES_LOG),
        events: eventRecords.slice(0, 100),
        adminlog: adminRecords.slice(0, 100)
      },
      analytics: { configured: Boolean(env.AIFRED_ANALYTICS_API_TOKEN), message: env.AIFRED_ANALYTICS_API_TOKEN ? "analytics provider configured" : "live analytics are not configured" },
      deploy: { source: repoConfig(env).repo, branch: repoConfig(env).branch, target: "Cloudflare Pages project aifred-site" }
    });
  }
  if (path === "admin/catalog/list") return json({ ok: true, tracks: await loadCatalog(request, env) });
  if (path === "admin/files/read" && request.method === "POST") return handleAdminFileRead(request, env);
  if (path === "admin/files/write" && request.method === "POST") return handleAdminFileWrite(request, env);
  if (path === "admin/files/list") return handleAdminFileList(request, env);
  if (path === "admin/files/delete" && request.method === "POST") return handleAdminFileDelete(request, env);
  if (path === "admin/files/upload" && request.method === "POST") return handleAdminFileUpload(request, env);
  if (path === "admin/reference/upload" && request.method === "POST") return handleAdminReferenceUpload(request, env);
  if (path === "admin/catalog/upload" && request.method === "POST") return handleAdminCatalogUpload(request, env);
  if (path === "admin/inquiries/list") {
    const inquiries = await readRepoJsonArray(env, inquiriesRepoPath());
    return json({
      ok: true,
      configured: Boolean(env.GITHUB_TOKEN),
      inquiries,
      message: env.GITHUB_TOKEN ? "Inquiry records loaded from repository storage." : "Inquiry persistence requires GITHUB_TOKEN in Cloudflare Pages."
    });
  }
  if (path === "admin/logs/list") {
    const activity = await listActivityRecords(env, 300);
    const adminlog = activity.filter((entry) => String(entry.event_type || "").startsWith("admin."));
    const events = activity.filter((entry) => !String(entry.event_type || "").startsWith("admin."));
    return json({
      ok: true,
      configured: Boolean(env.GITHUB_TOKEN || env.AIFRED_SALES_LOG),
      logs: activity,
      events,
      adminlog,
      message: env.GITHUB_TOKEN || env.AIFRED_SALES_LOG ? "Activity log loaded from repo or KV." : "Activity storage requires GITHUB_TOKEN or AIFRED_SALES_LOG."
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
  if (path === "admin/sales/record" && request.method === "POST") return handleManualSaleRecord(request, env);
  if (path === "models/list") {
    const ollamaModel = env.OLLAMA_MODEL || "aifred:latest";
    const openAiModel = env.OPENAI_MODEL || "gpt-5.4-mini";
    return json({
      ok: true,
      models: [ollamaModel, openAiModel].filter(Boolean),
      active_model: ollamaModel,
      providers: {
        openai: { configured: Boolean(env.OPENAI_API_KEY), model: openAiModel },
        ollama: { configured: Boolean(env.OLLAMA_BASE_URL), model: ollamaModel }
      }
    });
  }
  if (path === "chat/ask" && request.method === "POST") return handleChat(request, env);
  if (path === "paypal/config" && request.method === "GET") return handlePayPalConfig(request, env);
  if (path === "paypal/create-order" && request.method === "POST") return handlePayPalCreateOrder(request, env);
  if (path === "paypal/capture-order" && request.method === "POST") return handlePayPalCaptureOrder(request, env);
  if (path === "paypal/ipn" && request.method === "POST") return handlePayPalIpn(request, env);
  if (path === "sales/download" && request.method === "GET") return handleSalesDownload(request, env);
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
    const stored = await appendRepoJsonRecord(env, inquiriesRepoPath(), inquiry, `Record inquiry ${inquiry.id}`);
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
    const responseBody = {
      ok: true,
      inquiry_id: inquiry.id,
      target_email: contactEmail(env),
      stored: Boolean(stored.commit),
      email_sent: emailResult.ok
    };
    return json(responseBody);
  }

  return json({ ok: false, error: `unknown route: ${path}` }, { status: 404 });
  } catch (error) {
    return json({ ok: false, error: error.message || "backend route failed", route: path }, { status: 500 });
  }
}


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

async function loadCatalog(request) {
  const response = await fetch(new URL("/assets/data/beat_catalog.json", request.url), { cache: "no-store" });
  const tracks = await response.json();
  return Array.isArray(tracks) ? tracks : [];
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
    headers: { "content-type": "application/json" },
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

export async function onRequest({ request, env, params }) {
  const path = Array.isArray(params.path) ? params.path.join("/") : String(params.path || "");

  if (request.method === "OPTIONS") return new Response(null, { status: 204 });
  if (path === "health") return json({ ok: true, service: "AIFRED website backend" });
  if (path === "catalog/list") return json({ ok: true, tracks: await loadCatalog(request) });
  if (path === "soundpacks/list") return json({ ok: true, soundpacks: [] });
  if (path === "content/get") return json({ ok: true, content: contentPayload() });
  if (path === "models/list") {
    return json({
      ok: true,
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
}

export async function onRequest({ request, env }) {
  const upgrade = request.headers.get("Upgrade") || "";
  if (upgrade.toLowerCase() !== "websocket") {
    return new Response("Expected WebSocket", { status: 426 });
  }

  const pair = new WebSocketPair();
  const [client, server] = Object.values(pair);
  server.accept();
  server.send(JSON.stringify({
    type: "chat.ready",
    provider: env.OPENAI_API_KEY ? "openai" : env.OLLAMA_BASE_URL ? "ollama" : "http-fallback"
  }));

  server.addEventListener("message", async (event) => {
    let payload = {};
    try {
      payload = JSON.parse(event.data);
    } catch (_) {
      server.send(JSON.stringify({ type: "chat.error", message: "invalid websocket payload" }));
      return;
    }

    if (payload.type === "chat.clear") {
      server.send(JSON.stringify({ type: "chat.ready", provider: "memory-cleared" }));
      return;
    }

    if (payload.type !== "chat.send") return;
    const text = String(payload.text || "").trim();
    if (!text) {
      server.send(JSON.stringify({ type: "chat.error", message: "empty chat message" }));
      return;
    }

    const answer = await askModel(env, text);
    const chunks = answer.match(/.{1,160}/gs) || [answer];
    for (const chunk of chunks) {
      server.send(JSON.stringify({ type: "chat.token", text: chunk }));
    }
  });

  return new Response(null, { status: 101, webSocket: client });
}

async function askModel(env, message) {
  if (env.OPENAI_API_KEY) {
    const model = env.OPENAI_MODEL || "gpt-5.4-mini";
    const response = await fetch("https://api.openai.com/v1/responses", {
      method: "POST",
      headers: {
        "authorization": `Bearer ${env.OPENAI_API_KEY}`,
        "content-type": "application/json"
      },
      body: JSON.stringify({
        model,
        input: [
          { role: "system", content: "You are Aifred for North3rnLight3r. Keep answers direct and useful for admin control, catalog, and mix decisions." },
          { role: "user", content: message }
        ]
      })
    });
    const payload = await response.json();
    if (!response.ok) return payload?.error?.message || "OpenAI request failed.";
    return payload.output_text
      || payload.output?.flatMap((item) => item.content || []).map((part) => part.text || "").join("").trim()
      || "Aifred received the request, but the model returned no text.";
  }

  if (env.OLLAMA_BASE_URL) {
    const base = String(env.OLLAMA_BASE_URL).replace(/\/+$/, "");
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
          { role: "system", content: "You are Aifred for North3rnLight3r. Keep answers direct and useful for admin control, catalog, and mix decisions." },
          { role: "user", content: message }
        ]
      })
    });
    const payload = await response.json();
    if (!response.ok) return payload?.error || "Ollama request failed.";
    return payload?.message?.content || payload?.response || "Ollama returned no text.";
  }

  return "Chat backend is live. Add OPENAI_API_KEY or OLLAMA_BASE_URL in Cloudflare Pages settings to enable model answers.";
}

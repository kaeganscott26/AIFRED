import http from "node:http";

const listenHost = process.env.AIFRED_OLLAMA_PROXY_HOST || "127.0.0.1";
const listenPort = Number(process.env.AIFRED_OLLAMA_PROXY_PORT || 11435);
const ollamaBase = (process.env.OLLAMA_UPSTREAM || "http://127.0.0.1:11434").replace(/\/+$/, "");
const requiredToken = process.env.OLLAMA_API_TOKEN || "";

if (!requiredToken) {
  console.error("OLLAMA_API_TOKEN is required.");
  process.exit(1);
}

const server = http.createServer(async (req, res) => {
  const auth = req.headers.authorization || "";
  if (auth !== `Bearer ${requiredToken}`) {
    res.writeHead(401, { "content-type": "application/json" });
    res.end(JSON.stringify({ error: "unauthorized" }));
    return;
  }

  const chunks = [];
  req.on("data", (chunk) => chunks.push(chunk));
  req.on("end", async () => {
    try {
      const upstreamUrl = new URL(req.url || "/", ollamaBase);
      const upstream = await fetch(upstreamUrl, {
        method: req.method,
        headers: {
          "content-type": req.headers["content-type"] || "application/json"
        },
        body: ["GET", "HEAD"].includes(req.method || "GET") ? undefined : Buffer.concat(chunks)
      });
      res.writeHead(upstream.status, Object.fromEntries(upstream.headers));
      if (upstream.body) {
        const reader = upstream.body.getReader();
        while (true) {
          const { value, done } = await reader.read();
          if (done) break;
          res.write(Buffer.from(value));
        }
      }
      res.end();
    } catch (error) {
      res.writeHead(502, { "content-type": "application/json" });
      res.end(JSON.stringify({ error: error instanceof Error ? error.message : "proxy failed" }));
    }
  });
});

server.listen(listenPort, listenHost, () => {
  console.log(`AIFRED Ollama auth proxy listening on http://${listenHost}:${listenPort}`);
});

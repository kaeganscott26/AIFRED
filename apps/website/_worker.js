import { onRequest as apiV1 } from "./functions/api/v1/[[path]].js";
import { onRequest as apiLegacy } from "./functions/api/[[path]].js";
import { onRequest as wsChat } from "./functions/ws/chat.js";

const ALLOWED_ORIGINS = new Set([
  "https://north3rnlight3r.com",
  "https://www.north3rnlight3r.com",
  "https://aifred-site.pages.dev",
  "http://localhost:8787",
  "http://127.0.0.1:8787"
]);

function withCors(request, response) {
  const origin = request.headers.get("origin");
  if (!origin || !ALLOWED_ORIGINS.has(origin)) return response;
  const headers = new Headers(response.headers);
  headers.set("access-control-allow-origin", origin);
  headers.set("access-control-allow-methods", "GET, HEAD, POST, OPTIONS");
  headers.set("access-control-allow-headers", "Authorization, Content-Type, X-Api-Token, X-AIFRED-Request-ID");
  headers.set("access-control-max-age", "86400");
  headers.append("vary", "Origin");
  return new Response(response.body, { status: response.status, statusText: response.statusText, headers });
}

async function apiResponse(handler, context) {
  return withCors(context.request, await handler(context));
}

export default {
  async fetch(request, env, ctx) {
    const url = new URL(request.url);
    const path = url.pathname.replace(/^\/+/, "");

    if (path === "ws/chat") {
      return wsChat({ request, env, ctx, params: {} });
    }

    if (request.method === "OPTIONS" && (path === "health" || path.startsWith("v1/") || path.startsWith("api/"))) {
      return withCors(request, new Response(null, { status: 204, headers: { "cache-control": "no-store" } }));
    }

    if (path === "health" || path.startsWith("v1/")) {
      return apiResponse(apiV1, { request, env, ctx, params: { path: path === "health" ? ["health"] : path.slice(3).split("/").filter(Boolean) } });
    }

    if (path.startsWith("api/v1/")) {
      return apiResponse(apiV1, {
        request,
        env,
        ctx,
        params: { path: path.slice("api/v1/".length).split("/").filter(Boolean) }
      });
    }

    if (path.startsWith("api/")) {
      return apiResponse(apiLegacy, {
        request,
        env,
        ctx,
        params: { path: path.slice("api/".length).split("/").filter(Boolean) }
      });
    }

    return env.ASSETS.fetch(request);
  }
};

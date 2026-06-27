import { onRequest as apiV1 } from "./functions/api/v1/[[path]].js";
import { onRequest as apiLegacy } from "./functions/api/[[path]].js";
import { onRequest as wsChat } from "./functions/ws/chat.js";

export default {
  async fetch(request, env, ctx) {
    const url = new URL(request.url);
    const path = url.pathname.replace(/^\/+/, "");

    if (path === "ws/chat") {
      return wsChat({ request, env, ctx, params: {} });
    }

    if (path.startsWith("api/v1/")) {
      return apiV1({
        request,
        env,
        ctx,
        params: { path: path.slice("api/v1/".length).split("/").filter(Boolean) }
      });
    }

    if (path.startsWith("api/")) {
      return apiLegacy({
        request,
        env,
        ctx,
        params: { path: path.slice("api/".length).split("/").filter(Boolean) }
      });
    }

    return env.ASSETS.fetch(request);
  }
};

const json = (body, init = {}) =>
  new Response(JSON.stringify(body), {
    ...init,
    headers: { "content-type": "application/json; charset=utf-8", "cache-control": "no-store" }
  });

export async function onRequest({ params }) {
  const path = Array.isArray(params.path) ? params.path.join("/") : String(params.path || "");
  if (path === "create-download-session") {
    return json({ ok: false, error: "Distribution downloads are published through GitHub Releases or configured Cloudflare storage after the Windows VST3 build is signed." }, { status: 501 });
  }
  return json({ ok: false, error: `unknown route: ${path}` }, { status: 404 });
}

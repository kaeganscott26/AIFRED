(() => {
  "use strict";
  const key = "aifred.ops.session";
  const names = ["Overview", "Analytics", "Downloads", "Track Analysis", "API", "Logs", "Inquiries", "Exports", "FORGE", "Archive"];
  const state = { logs: [], inquiries: [], tracks: [], references: [] };
  const $ = (s) => document.querySelector(s);
  const escape = (v) => String(v ?? "").replace(/[&<>"']/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" }[c]));
  const parsedTime = (v) => { const d = new Date(v); return !v || Number.isNaN(d.valueOf()) ? null : d; };
  const time = (v) => { const d = parsedTime(v); return d ? d.toLocaleString(undefined, { dateStyle: "medium", timeStyle: "medium" }) : "—"; };
  const timeCell = (v) => { const d = parsedTime(v); return d ? `${escape(time(v))}<br><small class="muted" title="Raw UTC timestamp">UTC ${escape(d.toISOString())}</small>` : "—"; };
  const auth = () => sessionStorage.getItem(key) || "";

  async function request(path, init = {}) {
    const response = await fetch(path, { ...init, cache: "no-store", headers: { accept: "application/json", authorization: `Bearer ${auth()}`, ...(init.body ? { "content-type": "application/json" } : {}), ...(init.headers || {}) } });
    if (response.status === 401) { signOut(); throw new Error("Your admin session expired."); }
    return response;
  }
  async function get(path) { const r = await request(path); const body = await r.json(); if (!r.ok) throw new Error(body.error || `Request failed (${r.status})`); return body; }
  const card = (label, value, detail = "") => `<article class="ops-card metric"><span>${escape(label)}</span><strong>${escape(value)}</strong><small class="muted">${escape(detail)}</small></article>`;
  function table(items, columns) {
    if (!items.length) return '<div class="empty">No records available.</div>';
    return `<div class="table-wrap"><table><thead><tr>${columns.map((c) => `<th>${escape(c.label)}</th>`).join("")}</tr></thead><tbody>${items.map((item) => `<tr>${columns.map((c) => `<td>${c.render ? c.render(item) : escape(item[c.key] ?? "—")}</td>`).join("")}</tr>`).join("")}</tbody></table></div>`;
  }
  const eventColumns = [
    { label: "Time", render: (r) => timeCell(r.timestamp || r.created_at || r.createdAt) },
    { label: "Event", render: (r) => `<span class="event-type">${escape(r.event_type || r.type || "event")}</span>` },
    { label: "Source", render: (r) => escape(r.source || r.metadata?.source || r.referrer || "—") },
    { label: "Details", render: (r) => escape(r.artifact || r.platform || r.subject || r.message || r.request_id || "—") }
  ];
  function render() {
    const d = state.dashboard || {}, traffic = d.traffic || {};
    const events = state.logs.filter((x) => !String(x.event_type || "").startsWith("admin."));
    const downloads = events.filter((x) => String(x.event_type || "").includes("download"));
    const errors = events.filter((x) => /error|fail/i.test(`${x.event_type || ""}${x.message || ""}`));
    $("#overview").innerHTML = `<h2>Overview</h2><div class="metric-grid">${card("API", state.status?.ok ? "Healthy" : "Unavailable", state.status?.api_version || "v1")}${card("Page views", traffic.page_views || 0, d.activity_snapshot?.source_available === false ? "Activity source temporarily unavailable" : "Lifetime activity aggregate")}${card("Downloads", traffic.downloads ?? "Unavailable", traffic.canonical?.available === false ? "Canonical KV total temporarily unavailable" : "Canonical download.counted only")}${card("Tracks", d.catalog?.tracks || state.tracks.length, "Published catalog")}${card("Errors", errors.length, "Recent bounded log")}${card("Inquiries", d.inquiries?.count || state.inquiries.length, "Recorded")}</div><h3>Recent activity</h3>${table((traffic.recent || events).slice(0, 20), eventColumns)}<p class="muted">Snapshot: ${time(d.snapshot_at)} local · UTC ${escape(parsedTime(d.snapshot_at)?.toISOString() || "—")} · Production: ${escape(d.deploy?.target || "aifred-site")}</p>`;
    const counts = Object.entries(events.reduce((a, x) => { const name = x.event_type || "unknown"; a[name] = (a[name] || 0) + 1; return a; }, {})).sort((a, b) => b[1] - a[1]);
    $("#analytics").innerHTML = `<h2>Analytics</h2><div class="metric-grid">${card("Events", traffic.api_hits || events.length, "Compact lifetime aggregate")}${card("Sessions", new Set(events.map((x) => x.session_id).filter(Boolean)).size, "Recent bounded identifiers")}${card("Downloads", traffic.downloads ?? "Unavailable", "Canonical download.counted only")}${card("Lifecycle events", downloads.length, "Recent diagnostics, not summed")}${card("Media streams", traffic.media_streams || 0, "Compact lifetime aggregate")}</div>${table(counts.map(([type, count]) => ({ type, count })), [{ label: "Event type", key: "type" }, { label: "Count", key: "count" }])}<p class="muted">Historical requested/completed/clicked events remain available but are excluded from the canonical download total because unique human delivery cannot be reconstructed reliably.</p>`;
    $("#downloads").innerHTML = `<h2>Downloads</h2>${table(downloads, eventColumns)}`;
    $("#track-analysis").innerHTML = `<h2>Track Analysis</h2><div class="metric-grid">${card("Catalog tracks", state.tracks.length, "Public catalog")}${card("Accepted analyses", state.references.length, "Reference pool")}</div><h3>Analysis records</h3>${table(state.references.slice(0, 100), eventColumns)}<h3>Catalog</h3>${table(state.tracks.slice(0, 100), [{ label: "Track", render: (r) => escape(r.title || r.name || r.id) }, { label: "Artist", render: (r) => escape(r.artist || "—") }, { label: "Version", render: (r) => escape(r.version || "—") }])}`;
    $("#api").innerHTML = `<h2>API</h2><div class="metric-grid">${card("Production base", location.origin, "Canonical custom domain")}${card("Version", state.status?.api_version || "v1", "Normalized contract")}${card("Models", state.models?.models?.length || 0, state.models?.active_model || "")}</div><pre>${escape(JSON.stringify({ health: "/health", models: "/v1/models", chat: "/v1/chat/completions", admin: "/api/v1/admin/*", providers: state.models?.providers || {} }, null, 2))}</pre>`;
    $("#logs").innerHTML = `<h2>Logs / Errors</h2><div class="toolbar"><input id="log-filter" class="ops-control" placeholder="Filter current bounded log"></div><div id="log-table">${table(state.logs.slice(0, 300), eventColumns)}</div>`;
    $("#inquiries").innerHTML = `<h2>Inquiries</h2>${table(state.inquiries.slice(0, 200), [{ label: "Time", render: (r) => timeCell(r.timestamp || r.created_at) }, { label: "Name", render: (r) => escape(r.name || "—") }, { label: "Contact", render: (r) => escape(r.email || r.contact || "—") }, { label: "Status", render: (r) => escape(r.status || "received") }])}`;
    $("#exports").innerHTML = '<h2>Exports</h2><div class="card-grid"><article class="ops-card"><h3>Site Data</h3><p>Sanitized analytics, downloads, sessions, inquiries, errors, activity and deployment state.</p><button data-export="site">Export Site Data</button></article><article class="ops-card"><h3>Track Analysis</h3><p>Catalog and authoritative accepted analysis/reference records.</p><button data-export="tracks">Export Track Analysis</button></article></div><p class="muted">Authenticated, UTC ISO-8601, and Cache-Control: no-store.</p>';
    $("#forge").innerHTML = `<h2>FORGE</h2><div class="metric-grid">${card("Bridge", "Configured", "integrations/forge/manifest.json")}${card("Active policy", "Bounded", "25 MB default, configurable")}${card("Latest exports", "Mirrored", "Site + track analysis")}</div><article class="ops-card archive-note"><p>FORGE retains current summaries, latest exports and a lightweight archive pointer. Completed history rotates only after verified local archival. This web console cannot access local files.</p></article>`;
    $("#archive").innerHTML = '<h2>Archive</h2><article class="ops-card archive-note"><h3>Desktop-owned cold storage</h3><p>Permanent archives live under gitignored <code>runtime/aifred-archive</code>. Use Desktop Admin to view, verify, search, restore or manually prune them.</p><p>Rotation never removes Cloudflare production data.</p></article>';
    $("#log-filter")?.addEventListener("input", (e) => { $("#log-table").innerHTML = table(state.logs.filter((x) => JSON.stringify(x).toLowerCase().includes(e.target.value.toLowerCase())).slice(0, 300), eventColumns); });
    document.querySelectorAll("[data-export]").forEach((button) => button.addEventListener("click", () => downloadExport(button.dataset.export, button)));
  }
  async function refresh() {
    busy(true, "Refreshing…");
    try {
      const dashboard = await get("/api/v1/admin/dashboard/state");
      Object.assign(state, {
        dashboard,
        status: dashboard.status || {},
        logs: dashboard.logs?.logs || [],
        inquiries: dashboard.inquiries?.items || [],
        tracks: dashboard.catalog?.items || [],
        references: dashboard.references?.items || [],
        models: dashboard.models || {}
      });
      render(); note("Data refreshed.");
    } catch (error) { note(error.message, true); } finally { busy(false); }
  }
  async function downloadExport(kind, button) {
    button.disabled = true; note(`Preparing ${kind} export…`);
    try { const r = await request(`/api/v1/admin/export/${kind}`); if (!r.ok) throw new Error(`Export failed (${r.status})`); const blob = await r.blob(), disposition = r.headers.get("content-disposition") || "", filename = /filename="?([^";]+)"?/i.exec(disposition)?.[1] || `aifred-${kind}-export.json`, url = URL.createObjectURL(blob), a = document.createElement("a"); a.href = url; a.download = filename; a.click(); setTimeout(() => URL.revokeObjectURL(url), 1000); note(`Saved ${filename}`); } catch (error) { note(error.message, true); } finally { button.disabled = false; }
  }
  function note(message, error = false) { const element = $("#notice"); element.hidden = false; element.textContent = message; element.classList.toggle("error", error); }
  function busy(active, label) { $("#refresh").disabled = active; $("#connection").textContent = label || (auth() ? "Production connected" : "Signed out"); }
  function signOut() { sessionStorage.removeItem(key); $("#console").hidden = true; $("#login").hidden = false; $("#refresh").hidden = true; $("#logout").hidden = true; $("#connection").classList.remove("online"); $("#connection").textContent = "Signed out"; }
  function signedIn() { $("#login").hidden = true; $("#console").hidden = false; $("#refresh").hidden = false; $("#logout").hidden = false; $("#connection").classList.add("online"); if (!$("#tabs").children.length) names.forEach((name, index) => { const id = name.toLowerCase().replaceAll(" ", "-"), button = document.createElement("button"); button.textContent = name; button.classList.toggle("active", index === 0); button.onclick = () => { document.querySelectorAll(".tab-panel").forEach((panel) => { panel.hidden = panel.id !== id; }); document.querySelectorAll("#tabs button").forEach((item) => item.classList.toggle("active", item === button)); }; $("#tabs").append(button); }); refresh(); }
  $("#login-form").addEventListener("submit", async (event) => { event.preventDefault(); $("#login-error").textContent = ""; try { const r = await fetch("/api/v1/admin/login", { method: "POST", headers: { "content-type": "application/json" }, body: JSON.stringify({ username: $("#username").value, password: $("#password").value }) }), body = await r.json(); if (!r.ok) throw new Error(body.error || "Sign-in failed"); sessionStorage.setItem(key, body.session_token); $("#password").value = ""; signedIn(); } catch (error) { $("#login-error").textContent = error.message; } });
  $("#refresh").addEventListener("click", refresh); $("#logout").addEventListener("click", signOut); if (auth()) signedIn();
})();

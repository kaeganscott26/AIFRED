import { EmailMessage } from "cloudflare:email";

function safeHeader(value, fallback = "") {
  return String(value || fallback).replace(/[\r\n]+/g, " ").trim();
}

function buildRawMessage({ from, to, subject, text, html, replyTo }) {
  const boundary = `aifred-${crypto.randomUUID()}`;
  const lines = [
    `From: ${safeHeader(from)}`,
    `To: ${safeHeader(to)}`,
    `Subject: ${safeHeader(subject, "AIFRED notification")}`,
    "MIME-Version: 1.0"
  ];

  if (replyTo) {
    lines.push(`Reply-To: ${safeHeader(replyTo)}`);
  }

  lines.push(`Content-Type: multipart/alternative; boundary="${boundary}"`, "");
  lines.push(`--${boundary}`);
  lines.push("Content-Type: text/plain; charset=UTF-8", "", String(text || ""));

  if (html) {
    lines.push(`--${boundary}`);
    lines.push("Content-Type: text/html; charset=UTF-8", "", String(html));
  }

  lines.push(`--${boundary}--`, "");
  return lines.join("\r\n");
}

export default {
  async fetch(request, env) {
    if (request.method !== "POST") {
      return new Response("Method Not Allowed", { status: 405 });
    }

    const expectedToken = safeHeader(env.MAILER_SHARED_TOKEN);
    const providedToken = safeHeader(request.headers.get("x-aifred-mailer-token"));
    if (expectedToken && providedToken !== expectedToken) {
      return Response.json({ ok: false, error: "unauthorized" }, { status: 401 });
    }

    const payload = await request.json().catch(() => null);
    if (!payload || typeof payload !== "object") {
      return Response.json({ ok: false, error: "invalid payload" }, { status: 400 });
    }

    const to = safeHeader(env.MAILER_DESTINATION || "north3rnlight3rofficial@outlook.com");
    const from = safeHeader(payload.from || env.MAILER_SENDER || to);
    const raw = buildRawMessage({
      from,
      to,
      subject: payload.subject || "AIFRED notification",
      text: payload.text || "",
      html: payload.html || "",
      replyTo: payload.replyTo || ""
    });

    await env.EMAIL.send(new EmailMessage(from, to, raw));
    return Response.json({ ok: true, id: crypto.randomUUID() });
  }
};

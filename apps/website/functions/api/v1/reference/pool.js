const json = (body, init = {}) =>
  new Response(JSON.stringify(body), {
    ...init,
    headers: {
      "content-type": "application/json; charset=utf-8",
      "cache-control": "public, max-age=60, s-maxage=300",
      ...(init.headers || {})
    }
  });

function sanitizeReference(record) {
  const metrics = record && typeof record.metrics === "object" && record.metrics ? record.metrics : {};
  const gate = record && typeof record.gate === "object" && record.gate ? record.gate : {};
  return {
    id: String(record?.id || ""),
    created_at: String(record?.created_at || ""),
    duration_seconds: Number(record?.duration_seconds || 0),
    metrics,
    classification: String(gate.classification || record?.classification || ""),
    reference_utility: Number(gate.reference_utility ?? record?.reference_utility ?? 0),
    technical_caution: Number(gate.technical_caution ?? record?.technical_caution ?? 0),
    style_tag: String(gate.style_tag || record?.style_tag || ""),
    best_use: String(gate.best_use || record?.best_use || ""),
    caution: String(gate.caution || record?.caution || "")
  };
}

export async function onRequest({ env }) {
  if (!env.AIFRED_REFERENCE_POOL || typeof env.AIFRED_REFERENCE_POOL.list !== "function") {
    return json({ ok: false, error: "reference pool binding unavailable", records: [] }, { status: 503 });
  }

  try {
    const records = [];
    let cursor;
    do {
      const listed = await env.AIFRED_REFERENCE_POOL.list({
        prefix: "reference:",
        limit: Math.min(1000, Math.max(1, 500 - records.length)),
        ...(cursor ? { cursor } : {})
      });

      for (const key of listed.keys || []) {
        if (records.length >= 500) break;
        const raw = await env.AIFRED_REFERENCE_POOL.get(key.name);
        if (!raw) continue;
        try {
          const parsed = JSON.parse(raw);
          const sanitized = sanitizeReference(parsed);
          if (sanitized.id) records.push(sanitized);
        } catch (_) {
          // A malformed legacy record must not break the shared public pool.
        }
      }

      cursor = listed.list_complete || records.length >= 500 ? undefined : listed.cursor;
    } while (cursor);

    records.sort((a, b) => String(b.created_at).localeCompare(String(a.created_at)));
    return json({
      ok: true,
      schema: "aifred.reference-pool.public.v1",
      source: "AIFRED_REFERENCE_POOL",
      count: records.length,
      records
    });
  } catch (_) {
    return json({ ok: false, error: "reference pool temporarily unavailable", records: [] }, { status: 503 });
  }
}

const SECRET_FIELD = /^(?:authorization|cookie|password|passwd|secret|token|access_token|refresh_token|api[_-]?key|private[_-]?key|client[_-]?secret|download_token)$/i;
const ACTOR_TYPES = new Set(["anonymous", "admin", "system"]);
const OPERATION_STATUSES = new Set(["success", "failure", "started"]);
const ACTIVITY_RETENTION_SECONDS = 90 * 24 * 60 * 60;

function cleanText(value, maxLength = 500) {
  return String(value ?? "").trim().slice(0, maxLength);
}

function cleanId(value, maxLength = 120) {
  return cleanText(value, maxLength).replace(/[^A-Za-z0-9._:@-]/g, "-");
}

function compactObject(value) {
  if (!value || typeof value !== "object" || Array.isArray(value)) return value;
  return Object.fromEntries(Object.entries(value).filter(([, entry]) => {
    if (entry === undefined || entry === null || entry === "") return false;
    if (Array.isArray(entry)) return entry.length > 0;
    if (typeof entry === "object") return Object.keys(entry).length > 0;
    return true;
  }));
}

function sanitizeValue(value, depth = 0) {
  if (depth > 4 || value === undefined || value === null) return undefined;
  if (typeof value === "string") return cleanText(value, 1000);
  if (typeof value === "number" || typeof value === "boolean") return value;
  if (Array.isArray(value)) {
    return value.slice(0, 25).map((entry) => sanitizeValue(entry, depth + 1)).filter((entry) => entry !== undefined);
  }
  if (typeof value !== "object") return cleanText(value, 1000);
  const sanitized = {};
  for (const [key, entry] of Object.entries(value).slice(0, 50)) {
    if (SECRET_FIELD.test(key)) continue;
    const next = sanitizeValue(entry, depth + 1);
    if (next !== undefined) sanitized[cleanText(key, 80)] = next;
  }
  return compactObject(sanitized);
}

function requestRoute(request) {
  try {
    return new URL(request.url).pathname;
  } catch (_) {
    return "";
  }
}

function requestHeader(request, name) {
  try {
    return request?.headers?.get(name) || "";
  } catch (_) {
    return "";
  }
}

function normalizeOperationStatus(value, eventType) {
  const status = cleanText(value, 30).toLowerCase();
  if (OPERATION_STATUSES.has(status)) return status;
  if (/fail|error|reject|denied|invalid/.test(status) || /\.failed$/.test(eventType)) return "failure";
  if (/success|complete|completed|served|saved|stored|updated|uploaded|deleted|authenticated|accepted|resolved/.test(status)) return "success";
  return "started";
}

export function normalizeActivityType(value) {
  const text = cleanText(value || "site.event", 100).toLowerCase();
  return text
    .replace(/[^a-z0-9._-]+/g, ".")
    .replace(/\.{2,}/g, ".")
    .replace(/^\.+|\.+$/g, "") || "site.event";
}

export function activityEventId(record) {
  return cleanId(record?.event_id || record?.id || record?.txn_id || record?.order_id || record?.timestamp || record?.created_at, 160);
}

export function activityTimestamp(record) {
  return cleanText(record?.timestamp || record?.created_at, 40);
}

export function normalizeActivityEvent(record = {}, { request, now, randomUUID } = {}) {
  const createId = () => typeof randomUUID === "function" ? randomUUID() : crypto.randomUUID();
  const eventType = normalizeActivityType(record.event_type || record.type || record.kind);
  const timestamp = cleanText(record.timestamp || record.created_at || (now || new Date()).toISOString(), 40);
  const eventId = cleanId(record.event_id || record.id || createId(), 160);
  const suppliedSource = record.source && typeof record.source === "object" ? record.source : {};
  const surface = cleanText(suppliedSource.surface || (typeof record.source === "string" ? record.source : "") || "worker", 40).toLowerCase();
  const sessionId = cleanId(record.session_id || record.client_session_id, 160);
  const requestId = cleanId(
    record.request_id || suppliedSource.request_id || requestHeader(request, "x-aifred-request-id") || requestHeader(request, "cf-ray") || createId(),
    160
  );

  const suppliedActor = record.actor && typeof record.actor === "object" ? record.actor : {};
  const actorTypeCandidate = cleanText(suppliedActor.type || (surface === "admin" ? "admin" : sessionId ? "anonymous" : "system"), 20).toLowerCase();
  const actor = compactObject({
    type: ACTOR_TYPES.has(actorTypeCandidate) ? actorTypeCandidate : "system",
    id: cleanId(suppliedActor.id || (typeof record.actor === "string" ? record.actor : "") || (actorTypeCandidate === "anonymous" ? sessionId : ""), 160)
  });

  const source = compactObject({
    surface,
    route: cleanText(suppliedSource.route || record.path || requestRoute(request), 500),
    referrer: cleanText(suppliedSource.referrer || record.referrer || requestHeader(request, "referer"), 500),
    user_agent: cleanText(suppliedSource.user_agent || record.user_agent || requestHeader(request, "user-agent"), 500),
    country: cleanText(suppliedSource.country || request?.cf?.country, 8),
    colo: cleanText(suppliedSource.colo || request?.cf?.colo, 16)
  });

  const suppliedSubject = record.subject && typeof record.subject === "object" ? record.subject : {};
  const fallbackSubjectName = record.item_name || record.title || "";
  const subject = compactObject({
    type: cleanText(suppliedSubject.type || (fallbackSubjectName ? "activity" : ""), 50).toLowerCase(),
    id: cleanId(suppliedSubject.id, 200),
    name: cleanText(suppliedSubject.name || fallbackSubjectName, 300)
  });

  const suppliedOperation = record.operation && typeof record.operation === "object" ? record.operation : {};
  const legacyResult = record.status || record.message || "";
  const operation = compactObject({
    action: cleanText(suppliedOperation.action || eventType.split(".").pop(), 80).toLowerCase(),
    status: normalizeOperationStatus(suppliedOperation.status || record.status, eventType),
    result: cleanText(suppliedOperation.result || legacyResult, 300)
  });

  return compactObject({
    event_id: eventId,
    event_type: eventType,
    timestamp,
    session_id: sessionId,
    request_id: requestId,
    actor,
    source,
    subject,
    operation,
    metadata: sanitizeValue(record.metadata ?? record.details ?? {})
  });
}

export function activityKey(event) {
  const timestamp = activityTimestamp(event) || new Date().toISOString();
  const eventType = normalizeActivityType(event?.event_type);
  const eventId = activityEventId(event) || crypto.randomUUID();
  return `activity:v1:${timestamp}:${eventType}:${eventId}`;
}

export async function recordActivity(env, record, options = {}) {
  const event = normalizeActivityEvent(record, options);
  const key = activityKey(event);
  const binding = env?.AIFRED_SALES_LOG;
  if (!binding || typeof binding.put !== "function") return { event, key, storage: "unconfigured", stored: false };

  try {
    await binding.put(key, JSON.stringify(event), {
      expirationTtl: ACTIVITY_RETENTION_SECONDS,
      metadata: compactObject({ schema: 1, type: event.event_type, ts: event.timestamp, request_id: event.request_id })
    });
    return { event, key, storage: "kv", stored: true };
  } catch (_) {
    return { event, key, storage: "error", stored: false, error: "activity_log_write_failed" };
  }
}

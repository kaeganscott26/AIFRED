const config = window.AIFRED_CONFIG || {};
const API_BASE = String(config.apiBase || window.location.origin || "").replace(/\/+$/, "");
const CONTACT_EMAIL = String(config.contactEmail || "north3rnlight3rofficial@outlook.com").trim();
const DEFAULT_ART = "assets/brand/aifred-mascot.jpg";
const RELEASE_URL = String(config.releaseUrl || "https://github.com/kaeganscott26/AIFRED/releases/latest").trim();
const PRODUCT_PRICE = String(config.productPrice || "$149.99").trim();

const catalogList = document.getElementById("catalog-list");
const catalogRefresh = document.getElementById("catalog-refresh");
const audioPlayer = document.getElementById("audio-player");
const nowTitle = document.getElementById("now-title");
const nowMeta = document.getElementById("now-meta");
const serviceList = document.getElementById("service-list");
const chatForm = document.getElementById("chat-form");
const chatInput = document.getElementById("chat-input");
const chatOutput = document.getElementById("chat-output");
const contactForm = document.getElementById("contact-form");
const contactStatus = document.getElementById("contact-status");
const year = document.getElementById("year");
const aifredBuyButton = document.getElementById("aifred-buy-button");
const aifredReleaseButton = document.getElementById("aifred-release-button");

let tracks = [];

function apiUrl(path) {
  const safePath = path.startsWith("/") ? path : `/${path}`;
  return `${API_BASE}${safePath}`;
}

function localCatalogUrl(fileName = "") {
  return `assets/audio/catalog/${encodeURIComponent(fileName).replace(/%2F/gi, "/")}`;
}

function trackUrl(track) {
  return String(track.stream_url || track.full_song_url || track.public_url || localCatalogUrl(track.asset_file_name || track.file_name || "")).trim();
}

function trackTitle(track) {
  return String(track.title || track.file_name || "North3rnLight3r beat").replace(/\.(wav|mp3)$/i, "").trim();
}

function paypalUrl(itemName, price = "$19.99", returnUrl = "") {
  const amount = String(price).replace(/[^0-9.]/g, "") || "19.99";
  const params = new URLSearchParams({
    cmd: "_xclick",
    business: CONTACT_EMAIL,
    item_name: itemName,
    amount,
    currency_code: "USD"
  });
  if (returnUrl) {
    params.set("return", returnUrl);
    params.set("cancel_return", window.location.href.split("#")[0]);
  }
  return `https://www.paypal.com/cgi-bin/webscr?${params}`;
}

async function getJson(path, fallback) {
  try {
    const response = await fetch(apiUrl(path), { cache: "no-store" });
    const payload = await response.json();
    if (response.ok) return payload;
  } catch (_) {}
  return fallback;
}

async function loadCatalog() {
  const fallbackResponse = await fetch("assets/data/beat_catalog.json", { cache: "no-store" });
  const fallback = await fallbackResponse.json();
  const payload = await getJson("/api/v1/catalog/list", { ok: true, tracks: fallback });
  tracks = Array.isArray(payload.tracks) ? payload.tracks : fallback;
  renderCatalog();
}

function renderCatalog() {
  catalogList.innerHTML = "";
  tracks.forEach((track, index) => {
    const title = trackTitle(track);
    const bpm = String(track.bpm || track.tempo || "BPM N/A");
    const genre = String(track.genre || "North3rnLight3r");
    const price = String(track.price || "$19.99");
    const card = document.createElement("article");
    card.className = "catalog-card";
    card.innerHTML = `
      <img src="${track.artwork_url || DEFAULT_ART}" alt="${title} artwork" loading="lazy" />
      <h3>${title}</h3>
      <p>${bpm} · ${genre} · ${price}</p>
      <div class="card-actions">
        <button class="btn" type="button">Play</button>
        <a class="btn ghost" target="_blank" rel="noreferrer">License</a>
      </div>
    `;
    card.querySelector("img").addEventListener("error", (event) => {
      event.currentTarget.src = DEFAULT_ART;
    });
    card.querySelector("button").addEventListener("click", () => {
      audioPlayer.src = trackUrl(track);
      audioPlayer.play().catch(() => {});
      nowTitle.textContent = title;
      nowMeta.textContent = `${bpm} · ${genre}`;
    });
    card.querySelector("a").href = paypalUrl(title, price);
    catalogList.appendChild(card);
  });
}

function renderServices() {
  const services = [
    ["Mix Diagnostic Pass", "A focused read on tone, width, headroom, punch, and the next move before mastering.", "$49"],
    ["Reference Match Notes", "AIFRED-style notes against a target track so you know what to adjust and what to leave alone.", "$79"],
    ["Release Readiness Check", "A final translation pass for artists who need a clean decision before upload.", "$99"]
  ];
  serviceList.innerHTML = services.map(([name, description, price]) => `
    <article>
      <strong>${name}</strong>
      <span>${description}</span>
      <p>${price}</p>
      <a class="btn ghost" href="${paypalUrl(name, price)}" target="_blank" rel="noreferrer">Book</a>
    </article>
  `).join("");
}

async function askAifred(message) {
  chatOutput.textContent = "Aifred is checking the configured model route...";
  try {
    const response = await fetch(apiUrl("/api/v1/chat/ask"), {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ message })
    });
    const payload = await response.json();
    if (!response.ok || !payload.ok) throw new Error(payload.error || "chat unavailable");
    chatOutput.textContent = `[${payload.provider || "model"}]\n${payload.answer}`;
  } catch (error) {
    chatOutput.textContent = `Chat route is not configured yet. ${error.message || "Set OPENAI_API_KEY or OLLAMA_BASE_URL in the backend."}`;
  }
}

function setupForms() {
  chatForm.addEventListener("submit", (event) => {
    event.preventDefault();
    const message = chatInput.value.trim();
    if (message) askAifred(message);
  });

  contactForm.addEventListener("submit", async (event) => {
    event.preventDefault();
    const name = document.getElementById("contact-name").value.trim();
    const email = document.getElementById("contact-email").value.trim();
    const message = document.getElementById("contact-message").value.trim();
    contactStatus.textContent = "Sending...";
    try {
      const response = await fetch(apiUrl("/api/v1/inquiries/submit"), {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ name, email, message })
      });
      const payload = await response.json();
      if (!response.ok || !payload.ok) throw new Error(payload.error || "backend unavailable");
      contactStatus.textContent = "Inquiry received.";
      contactForm.reset();
    } catch (_) {
      const subject = encodeURIComponent(`AIFRED inquiry from ${name}`);
      const body = encodeURIComponent(`Name: ${name}\nEmail: ${email}\n\n${message}`);
      window.location.href = `mailto:${CONTACT_EMAIL}?subject=${subject}&body=${body}`;
      contactStatus.textContent = `Opening email fallback for ${CONTACT_EMAIL}.`;
    }
  });
}

function renderReleaseActions() {
  aifredBuyButton.href = paypalUrl("AIFRED VST3 Plugin", PRODUCT_PRICE, RELEASE_URL);
  aifredReleaseButton.href = RELEASE_URL;
}

year.textContent = new Date().getFullYear();
catalogRefresh.addEventListener("click", loadCatalog);
renderReleaseActions();
renderServices();
setupForms();
loadCatalog();

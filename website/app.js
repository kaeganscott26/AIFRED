const config = window.AIFRED_CONFIG || {};
const API_BASE = String(config.apiBase || window.location.origin || "").replace(/\/+$/, "");
const CONTACT_EMAIL = String(config.contactEmail || "north3rnlight3rofficial@outlook.com").trim();
const DEFAULT_ART = "assets/showcase/album-art-02.jpg";
const CATALOG_ART = ["assets/showcase/album-art-02.jpg", "assets/brand/aifred-mascot.jpg"];
const DOWNLOAD_URLS = config.downloadUrls || {};
const PAYPAL_CONFIG = config.paypal || {};

const catalogList = document.getElementById("catalog-list");
const catalogRefresh = document.getElementById("catalog-refresh");
const catalogToggle = document.getElementById("catalog-toggle");
const audioPlayer = document.getElementById("audio-player");
const nowTitle = document.getElementById("now-title");
const nowMeta = document.getElementById("now-meta");
const contactForm = document.getElementById("contact-form");
const contactStatus = document.getElementById("contact-status");
const year = document.getElementById("year");
const aifredDownloads = document.getElementById("aifred-downloads");
const aifredPayPalButtons = document.getElementById("aifred-paypal-buttons");
const aifredPurchaseStatus = document.getElementById("aifred-purchase-status");
const analysisUpload = document.getElementById("analysis-upload");
const spectralCanvas = document.getElementById("spectral-canvas");
const analysisTitle = document.getElementById("analysis-title");
const analysisStatus = document.getElementById("analysis-status");
const analysisMetrics = document.getElementById("analysis-metrics");
const analysisSubmit = document.getElementById("analysis-submit");
const analysisResult = document.getElementById("analysis-result");

let tracks = [];
let currentAnalysis = null;
let catalogOpen = false;
let audioContext = null;
let audioAnalyser = null;
let audioSource = null;
let visualizerFrame = 0;
const ACTIVITY_SESSION_KEY = "aifred-site-session";

function activitySessionId() {
  try {
    const existing = sessionStorage.getItem(ACTIVITY_SESSION_KEY);
    if (existing) return existing;
    const created = crypto.randomUUID();
    sessionStorage.setItem(ACTIVITY_SESSION_KEY, created);
    return created;
  } catch (_) {
    return `session-${Date.now()}-${Math.random().toString(36).slice(2, 10)}`;
  }
}

function recordActivity(eventType, details = {}) {
  const payload = {
    event_type: eventType,
    source: "website",
    path: `${window.location.pathname}${window.location.hash || ""}`,
    page: document.title || "AIFRED",
    title: document.title || "AIFRED",
    session_id: activitySessionId(),
    referrer: document.referrer || "",
    details
  };
  const body = JSON.stringify(payload);
  try {
    if (navigator.sendBeacon) {
      const blob = new Blob([body], { type: "application/json" });
      if (navigator.sendBeacon(apiUrl("/api/v1/activity/record"), blob)) return Promise.resolve(true);
    }
  } catch (_) {}
  return fetch(apiUrl("/api/v1/activity/record"), {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body,
    keepalive: true
  }).then(() => true).catch(() => false);
}

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
  void recordActivity("website.catalog.loaded", { track_count: tracks.length });
}

function renderCatalog() {
  catalogList.innerHTML = "";
  tracks.forEach((track, index) => {
    const title = trackTitle(track);
    const bpm = String(track.bpm || track.tempo || "BPM N/A");
    const genre = String(track.genre || "North3rnLight3r");
    const price = String(track.price || "$100");
    const card = document.createElement("article");
    card.className = "catalog-card";
    card.innerHTML = `
      <img src="${CATALOG_ART[index % CATALOG_ART.length] || DEFAULT_ART}" alt="${title} artwork" loading="lazy" />
      <h3>${title}</h3>
      <p>${bpm} · ${genre} · ${price}</p>
      <div class="card-actions">
        <button class="btn" type="button">Play</button>
      </div>
    `;
    card.querySelector("img").addEventListener("error", (event) => {
      event.currentTarget.src = DEFAULT_ART;
    });
    card.querySelector("button").addEventListener("click", () => {
      audioPlayer.crossOrigin = "anonymous";
      audioPlayer.src = trackUrl(track);
      audioPlayer.play().then(startCatalogVisualizer).catch(() => {});
      nowTitle.textContent = title;
      nowMeta.textContent = `${bpm} · ${genre}`;
      void recordActivity("catalog.play.clicked", {
        title,
        bpm,
        genre,
        price,
        stream_url: trackUrl(track)
      });
    });
    catalogList.appendChild(card);
  });
}

function setCatalogOpen(open) {
  catalogOpen = open;
  catalogList.hidden = !catalogOpen;
  catalogToggle.textContent = catalogOpen ? "Hide catalog" : "Show catalog";
  catalogToggle.setAttribute("aria-expanded", String(catalogOpen));
}

function setPurchaseStatus(message) {
  if (aifredPurchaseStatus) aifredPurchaseStatus.textContent = message;
}

function loadScript(src) {
  return new Promise((resolve, reject) => {
    const existing = document.querySelector(`script[src="${src}"]`);
    if (existing) {
      existing.addEventListener("load", resolve, { once: true });
      existing.addEventListener("error", reject, { once: true });
      if (window.paypal) resolve();
      return;
    }
    const script = document.createElement("script");
    script.src = src;
    script.async = true;
    script.addEventListener("load", resolve, { once: true });
    script.addEventListener("error", reject, { once: true });
    document.head.appendChild(script);
  });
}

function renderUnlockedDownloads(downloads) {
  if (!aifredDownloads || !downloads) return;
  const releaseNotes = String(DOWNLOAD_URLS.releaseNotes || "assets/docs/aifred-release-notes.txt").trim();
  const items = [
    ["Windows installer", downloads.setup],
    ["Windows zip", downloads.zip],
    ["Release notes", releaseNotes]
  ].filter(([, url]) => url);
  aifredDownloads.innerHTML = "";
  items.forEach(([label, url]) => {
    const link = document.createElement("a");
    link.href = url;
    link.target = "_blank";
    link.rel = "noreferrer";
    link.textContent = label;
    link.addEventListener("click", () => {
      void recordActivity("website.download.clicked", { label, url });
    });
    aifredDownloads.appendChild(link);
  });
}

async function renderPayPalButtons() {
  if (!aifredPayPalButtons) return false;
  const payload = await getJson("/api/v1/paypal/config", { ok: false });
  const paypalConfig = payload.paypal || {};
  if (!payload.ok || !paypalConfig.configured || !paypalConfig.client_id) {
    setPurchaseStatus("PayPal checkout is being configured. Use the contact form if you already paid.");
    return false;
  }

  const params = new URLSearchParams({
    "client-id": paypalConfig.client_id,
    currency: paypalConfig.currency || "USD",
    intent: "capture",
    components: "buttons"
  });
  await loadScript(`https://www.paypal.com/sdk/js?${params.toString()}`);
  if (!window.paypal?.Buttons) throw new Error("PayPal SDK did not load");

  window.paypal.Buttons({
    style: { layout: "vertical", label: "pay", height: 44 },
    createOrder: async () => {
      setPurchaseStatus("Opening PayPal checkout...");
      void recordActivity("paypal.buy.clicked", {
        item_name: paypalConfig.item_name || "AIFRED Plugin (download)",
        amount: paypalConfig.amount || "5.00",
        currency: paypalConfig.currency || "USD"
      });
      const response = await fetch(apiUrl("/api/v1/paypal/create-order"), {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ custom_id: `aifred-${Date.now()}-${Math.random().toString(36).slice(2, 10)}` })
      });
      const result = await response.json();
      if (!response.ok || !result.ok) throw new Error(result.error || "PayPal order create failed");
      void recordActivity("paypal.order.created", {
        order_id: result.id || "",
        item_name: paypalConfig.item_name || "AIFRED Plugin (download)",
        amount: paypalConfig.amount || "5.00",
        currency: paypalConfig.currency || "USD"
      });
      return result.id;
    },
    onApprove: async (data) => {
      setPurchaseStatus("Capturing payment and preparing download...");
      void recordActivity("paypal.order.capture.started", {
        order_id: data.orderID || ""
      });
      const response = await fetch(apiUrl("/api/v1/paypal/capture-order"), {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ order_id: data.orderID })
      });
      const result = await response.json();
      if (!response.ok || !result.ok) throw new Error(result.error || "PayPal capture failed");
      renderUnlockedDownloads(result.downloads);
      void recordActivity("paypal.order.captured", {
        order_id: data.orderID || "",
        download_setup: Boolean(result.downloads?.setup),
        download_zip: Boolean(result.downloads?.zip)
      });
      setPurchaseStatus("Payment complete. Download links are ready and have been emailed when PayPal provided an email.");
    },
    onCancel: () => {
      void recordActivity("paypal.checkout.cancelled", { item_name: paypalConfig.item_name || "AIFRED Plugin (download)" });
      setPurchaseStatus("PayPal checkout was cancelled.");
    },
    onError: (error) => {
      void recordActivity("paypal.checkout.error", { message: error.message || "try again shortly" });
      setPurchaseStatus(`PayPal checkout unavailable: ${error.message || "try again shortly"}`);
    }
  }).render("#aifred-paypal-buttons");
  return true;
}

async function renderReleaseActions() {
  const releaseNotes = String(DOWNLOAD_URLS.releaseNotes || "assets/docs/aifred-release-notes.txt").trim();
  const releasePage = String(DOWNLOAD_URLS.betaRelease || "https://github.com/kaeganscott26/AIFRED/releases/tag/beta-0.1.0").trim();

  if (!aifredDownloads) {
    return;
  }

  const downloads = [
    ["Free beta release", releasePage],
    ["Release notes", releaseNotes]
  ];

  aifredDownloads.innerHTML = downloads
    .map(([label, url]) => `<a href="${url}" target="_blank" rel="noreferrer">${label}</a>`)
    .join("");
  setPurchaseStatus("Free beta downloads are published through the GitHub release when artifacts are attached.");
}

function clamp(value, min, max) {
  return Math.min(max, Math.max(min, value));
}

function db(value) {
  return 20 * Math.log10(Math.max(value, 0.000001));
}

function drawSpectrum(buffer) {
  const canvas = spectralCanvas;
  const ctx = canvas.getContext("2d");
  const width = canvas.width;
  const height = canvas.height;
  const left = buffer.getChannelData(0);
  const step = Math.max(1, Math.floor(left.length / width));
  ctx.clearRect(0, 0, width, height);
  ctx.fillStyle = "#02060b";
  ctx.fillRect(0, 0, width, height);

  for (let x = 0; x < width; x += 1) {
    let sum = 0;
    let peak = 0;
    const start = x * step;
    for (let i = 0; i < step && start + i < left.length; i += 1) {
      const value = Math.abs(left[start + i]);
      sum += value;
      peak = Math.max(peak, value);
    }
    const energy = clamp(sum / step, 0, 1);
    const bar = Math.max(2, Math.round((energy * 0.55 + peak * 0.45) * height));
    const hue = 172 - Math.round(energy * 85);
    ctx.fillStyle = `hsl(${hue} 95% ${48 + Math.round(energy * 25)}%)`;
    ctx.fillRect(x, height - bar, 1, bar);
  }

  ctx.strokeStyle = "rgba(140,255,69,0.86)";
  ctx.lineWidth = 2;
  ctx.beginPath();
  const mid = height / 2;
  for (let x = 0; x < width; x += 1) {
    const value = left[Math.min(left.length - 1, x * step)] || 0;
    const y = mid + value * mid * 0.82;
    if (x === 0) ctx.moveTo(x, y);
    else ctx.lineTo(x, y);
  }
  ctx.stroke();
}

function setupCatalogAudio() {
  if (audioAnalyser) return true;
  const AudioContextClass = window.AudioContext || window.webkitAudioContext;
  if (!AudioContextClass) return false;
  audioContext = audioContext || new AudioContextClass();
  audioSource = audioSource || audioContext.createMediaElementSource(audioPlayer);
  audioAnalyser = audioContext.createAnalyser();
  audioAnalyser.fftSize = 2048;
  audioAnalyser.smoothingTimeConstant = 0.74;
  audioSource.connect(audioAnalyser);
  audioAnalyser.connect(audioContext.destination);
  return true;
}

function drawCatalogVisualizer() {
  if (!audioAnalyser || audioPlayer.paused || audioPlayer.ended) {
    visualizerFrame = 0;
    return;
  }

  const canvas = spectralCanvas;
  const ctx = canvas.getContext("2d");
  const width = canvas.width;
  const height = canvas.height;
  const bins = new Uint8Array(audioAnalyser.frequencyBinCount);
  const wave = new Uint8Array(audioAnalyser.fftSize);
  audioAnalyser.getByteFrequencyData(bins);
  audioAnalyser.getByteTimeDomainData(wave);

  ctx.clearRect(0, 0, width, height);
  ctx.fillStyle = "#02060b";
  ctx.fillRect(0, 0, width, height);

  const columns = 96;
  const step = Math.max(1, Math.floor(bins.length / columns));
  const columnWidth = width / columns;
  for (let x = 0; x < columns; x += 1) {
    let sum = 0;
    for (let i = 0; i < step; i += 1) sum += bins[x * step + i] || 0;
    const energy = sum / (step * 255);
    const bar = Math.max(3, energy * height * 0.92);
    const hue = 184 - Math.round(energy * 92);
    ctx.fillStyle = `hsl(${hue} 96% ${44 + Math.round(energy * 28)}%)`;
    ctx.fillRect(x * columnWidth + 1, height - bar, Math.max(1, columnWidth - 2), bar);
  }

  ctx.strokeStyle = "rgba(140,255,69,0.9)";
  ctx.lineWidth = 2;
  ctx.beginPath();
  for (let i = 0; i < wave.length; i += 1) {
    const x = (i / (wave.length - 1)) * width;
    const y = height * 0.5 + ((wave[i] - 128) / 128) * height * 0.32;
    if (i === 0) ctx.moveTo(x, y);
    else ctx.lineTo(x, y);
  }
  ctx.stroke();

  ctx.fillStyle = "rgba(232,251,255,0.82)";
  ctx.font = "700 14px Space Grotesk, sans-serif";
  ctx.fillText("BEAT CATALOG", 18, 28);
  visualizerFrame = requestAnimationFrame(drawCatalogVisualizer);
}

function startCatalogVisualizer() {
  if (!setupCatalogAudio()) return;
  audioContext.resume().catch(() => {});
  if (!visualizerFrame) visualizerFrame = requestAnimationFrame(drawCatalogVisualizer);
}

function analyzeAudioBuffer(buffer, fileName) {
  const left = buffer.getChannelData(0);
  const right = buffer.numberOfChannels > 1 ? buffer.getChannelData(1) : left;
  const sampleRate = buffer.sampleRate;
  const stride = Math.max(1, Math.floor(left.length / 240000));
  let sumSquares = 0;
  let peak = 0;
  let sideSum = 0;
  let midSum = 0;
  let lowEnergy = 0;
  let highEnergy = 0;
  let diffEnergy = 0;
  let previous = 0;
  let count = 0;

  for (let i = 0; i < left.length; i += stride) {
    const l = left[i] || 0;
    const r = right[i] || 0;
    const mono = (l + r) * 0.5;
    const side = (l - r) * 0.5;
    sumSquares += mono * mono;
    peak = Math.max(peak, Math.abs(l), Math.abs(r));
    midSum += Math.abs(mono);
    sideSum += Math.abs(side);
    const diff = Math.abs(mono - previous);
    diffEnergy += diff;
    if (diff < 0.012) lowEnergy += Math.abs(mono);
    else highEnergy += diff;
    previous = mono;
    count += 1;
  }

  const rms = Math.sqrt(sumSquares / Math.max(1, count));
  const rmsDb = db(rms);
  const peakDb = db(peak);
  const crest = peakDb - rmsDb;
  const width = clamp(sideSum / Math.max(0.000001, midSum + sideSum), 0, 1);
  const transient = diffEnergy / Math.max(1, count);
  const brightness = clamp(highEnergy / Math.max(0.000001, lowEnergy + highEnergy), 0, 1);
  const integratedLufs = rmsDb - 0.7;
  const lowEndControl = clamp(100 - Math.abs(brightness - 0.34) * 115 - Math.max(0, width - 0.92) * 34, 0, 100);
  const harshnessControl = clamp(100 - Math.max(0, brightness - 0.70) * 125 - Math.max(0, peakDb + 0.1) * 18, 0, 100);
  const toneBalance = clamp(100 - Math.abs(brightness - 0.46) * 82, 0, 100);

  return {
    file_name: fileName,
    duration_seconds: buffer.duration,
    metrics: {
      tone_balance: Math.round(toneBalance),
      integrated_lufs: Number(integratedLufs.toFixed(1)),
      peak_dbfs: Number(peakDb.toFixed(1)),
      crest_factor_db: Number(crest.toFixed(1)),
      stereo_width: Number(width.toFixed(2)),
      low_end_control: Math.round(lowEndControl),
      harshness_control: Math.round(harshnessControl),
      transient_density: Number(transient.toFixed(4)),
      spectral_centroid_hz: Math.round(brightness * sampleRate * 0.45)
    }
  };
}

function renderAnalysis(analysis) {
  const rows = [
    ["loudness", `${analysis.metrics.integrated_lufs} LUFS`],
    ["tonal balance", `${analysis.metrics.tone_balance}/100`],
    ["stereo behavior", analysis.metrics.stereo_width],
    ["general mix translation", `${analysis.metrics.crest_factor_db} dB crest`],
    ["peak", `${analysis.metrics.peak_dbfs} dBFS`],
    ["low-end control", `${analysis.metrics.low_end_control}/100`],
    ["harshness control", `${analysis.metrics.harshness_control}/100`]
  ];

  analysisMetrics.innerHTML = rows
    .map(([label, value]) => `
      <article>
        <span>${label}</span>
        <strong>${value}</strong>
      </article>
    `)
    .join("");
}

async function handleAnalysisFile(file) {
  if (!file) return;
  analysisTitle.textContent = file.name;
  analysisStatus.textContent = "Running the free limited analysis...";
  analysisSubmit.disabled = true;
  analysisResult.textContent = "The online analyzer is running locally in your browser.";
  try {
    const context = new AudioContext();
    const arrayBuffer = await file.arrayBuffer();
    const buffer = await context.decodeAudioData(arrayBuffer);
    drawSpectrum(buffer);
    currentAnalysis = analyzeAudioBuffer(buffer, file.name);
    renderAnalysis(currentAnalysis);
    analysisStatus.textContent = "Local analysis complete.";
    analysisSubmit.disabled = false;
    context.close().catch(() => {});
  } catch (error) {
    currentAnalysis = null;
    analysisStatus.textContent = "This browser could not decode that audio file.";
    analysisResult.textContent = error.message || "Audio decode failed.";
  }
}

async function submitAnalysisGate() {
  if (!currentAnalysis) return;
  analysisSubmit.disabled = true;
  analysisResult.textContent = "Submitting the reference candidate to the quality gate...";
  try {
    const response = await fetch(apiUrl("/api/v1/analysis/submit"), {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(currentAnalysis)
    });
    const payload = await response.json();
    if (!response.ok || !payload.ok) throw new Error(payload.error || "analysis unavailable");
    const lines = [
      `${payload.classification || "Analysis result"}`,
      `Reference utility: ${payload.reference_utility ?? payload.score}/100`,
      `Technical caution: ${payload.technical_caution ?? "N/A"}/100`,
      `Style tag: ${payload.style_tag || "unclassified"}`,
      `Best use: ${payload.best_use || "No note returned."}`,
      `Caution: ${payload.caution || "No caution returned."}`,
      `Why: ${payload.why || "No explanation returned."}`,
      `Persistence: ${payload.persistence || "none"}`
    ];
    analysisResult.textContent = lines.join("\n");
    void recordActivity("website.analysis.submitted", {
      file_name: currentAnalysis.file_name || analysisTitle.textContent || "analysis",
      accepted: Boolean(payload.accepted),
      score: payload.score,
      classification: payload.classification
    });
  } catch (error) {
    analysisResult.textContent = `Analysis unavailable: ${error.message || "request failed"}`;
    void recordActivity("website.analysis.failed", {
      file_name: currentAnalysis.file_name || analysisTitle.textContent || "analysis",
      error: error.message || "request failed"
    });
  } finally {
    analysisSubmit.disabled = false;
  }
}

function setupForms() {
  contactForm.addEventListener("submit", async (event) => {
    event.preventDefault();
    const name = document.getElementById("contact-name").value.trim();
    const email = document.getElementById("contact-email").value.trim();
    const message = document.getElementById("contact-message").value.trim();
    contactStatus.textContent = "Sending...";
    void recordActivity("website.inquiry.submit", { name, email });
    try {
      const response = await fetch(apiUrl("/api/v1/inquiries/submit"), {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ name, email, message })
      });
      const payload = await response.json();
      if (!response.ok || !payload.ok) throw new Error(payload.error || "backend unavailable");
      contactStatus.textContent = "Inquiry received.";
      void recordActivity("website.inquiry.completed", { name, email });
      contactForm.reset();
    } catch (_) {
      const subject = encodeURIComponent(`North3rnLight3r inquiry from ${name}`);
      const body = encodeURIComponent(`Name: ${name}\nEmail: ${email}\n\n${message}`);
      window.location.href = `mailto:${CONTACT_EMAIL}?subject=${subject}&body=${body}`;
      contactStatus.textContent = `Opening email fallback for ${CONTACT_EMAIL}.`;
      void recordActivity("website.inquiry.fallback", { name, email });
    }
  });
}

year.textContent = new Date().getFullYear();
audioPlayer.setAttribute("controlsList", "nodownload noplaybackrate");
audioPlayer.addEventListener("contextmenu", (event) => event.preventDefault());
audioPlayer.addEventListener("play", startCatalogVisualizer);
catalogRefresh.addEventListener("click", loadCatalog);
catalogToggle.addEventListener("click", () => setCatalogOpen(!catalogOpen));
analysisUpload.addEventListener("change", (event) => handleAnalysisFile(event.target.files?.[0]));
analysisSubmit.addEventListener("click", submitAnalysisGate);

renderReleaseActions();
setupForms();
loadCatalog();
void recordActivity("website.page.view", {
  title: document.title || "AIFRED",
  path: window.location.pathname,
  purchase: new URLSearchParams(window.location.search).get("purchase") || ""
});

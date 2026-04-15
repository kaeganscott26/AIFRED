const config = window.AIFRED_CONFIG || {};
const API_BASE = String(config.apiBase || window.location.origin || "").replace(/\/+$/, "");
const CONTACT_EMAIL = String(config.contactEmail || "north3rnlight3rofficial@outlook.com").trim();
const DEFAULT_ART = "assets/brand/aifred-mascot.jpg";
const RELEASE_URL = String(config.releaseUrl || "https://github.com/kaeganscott26/AIFRED/releases/latest").trim();
const DOWNLOAD_URLS = config.downloadUrls || {};
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
const chatProvider = document.getElementById("chat-provider");
const chatEndpoint = document.getElementById("chat-endpoint");
const chatApiKey = document.getElementById("chat-api-key");
const chatModel = document.getElementById("chat-model");
const contactForm = document.getElementById("contact-form");
const contactStatus = document.getElementById("contact-status");
const year = document.getElementById("year");
const aifredBuyButton = document.getElementById("aifred-buy-button");
const aifredReleaseButton = document.getElementById("aifred-release-button");
const aifredDownloads = document.getElementById("aifred-downloads");
const analysisUpload = document.getElementById("analysis-upload");
const spectralCanvas = document.getElementById("spectral-canvas");
const analysisTitle = document.getElementById("analysis-title");
const analysisStatus = document.getElementById("analysis-status");
const analysisMetrics = document.getElementById("analysis-metrics");
const analysisSubmit = document.getElementById("analysis-submit");
const analysisResult = document.getElementById("analysis-result");

let tracks = [];
let currentAnalysis = null;

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
  const provider = chatProvider?.value || "backend";
  const endpoint = String(chatEndpoint?.value || "").replace(/\/+$/, "");
  const apiKey = String(chatApiKey?.value || "").trim();
  const model = String(chatModel?.value || (provider === "openai" ? "gpt-5.4-mini" : "llama3.1")).trim();
  chatOutput.textContent = "Aifred is checking the configured model route...";
  try {
    if (provider === "ollama") {
      if (!endpoint) throw new Error("Enter an Ollama endpoint.");
      const response = await fetch(`${endpoint}/api/chat`, {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
          ...(apiKey ? { Authorization: `Bearer ${apiKey}` } : {})
        },
        body: JSON.stringify({
          model,
          stream: false,
          messages: [
            { role: "system", content: "You are AIFRED for North3rnLight3r. Return concise mix decisions and a fix list." },
            { role: "user", content: message }
          ]
        })
      });
      const payload = await response.json();
      if (!response.ok) throw new Error(payload.error || "Ollama request failed");
      chatOutput.textContent = `[ollama:${model}]\n${payload.message?.content || payload.response || "No text returned."}`;
      return;
    }
    if (provider === "openai") {
      if (!endpoint || !apiKey) throw new Error("Enter an OpenAI endpoint and API key.");
      const response = await fetch(`${endpoint}/responses`, {
        method: "POST",
        headers: { "Content-Type": "application/json", Authorization: `Bearer ${apiKey}` },
        body: JSON.stringify({
          model,
          input: [
            { role: "system", content: "You are AIFRED for North3rnLight3r. Return concise mix decisions and a fix list." },
            { role: "user", content: message }
          ]
        })
      });
      const payload = await response.json();
      if (!response.ok) throw new Error(payload.error?.message || "OpenAI request failed");
      chatOutput.textContent = `[openai:${model}]\n${payload.output_text || "No text returned."}`;
      return;
    }
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
  if (!aifredDownloads) return;
  const downloads = [
    ["Windows .exe", DOWNLOAD_URLS.windows || `${RELEASE_URL}/download/AIFRED-VST3-Setup.exe`],
    ["macOS VST3", DOWNLOAD_URLS.macos || `${RELEASE_URL}/download/AIFRED-VST3-macOS.zip`],
    ["Arch Linux", DOWNLOAD_URLS.arch || `${RELEASE_URL}/download/AIFRED-VST3-Arch.tar.gz`]
  ];
  aifredDownloads.innerHTML = downloads.map(([label, url]) => (
    `<a href="${url}" target="_blank" rel="noreferrer">${label}</a>`
  )).join("");
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
  const lowEndControl = clamp(100 - Math.abs(brightness - 0.34) * 170 - Math.max(0, width - 0.85) * 60, 0, 100);
  const harshnessControl = clamp(100 - Math.max(0, brightness - 0.62) * 180 - Math.max(0, peakDb + 0.8) * 12, 0, 100);
  const toneBalance = clamp(100 - Math.abs(brightness - 0.46) * 125, 0, 100);

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
    ["Tone balance", `${analysis.metrics.tone_balance}/100`],
    ["Loudness", `${analysis.metrics.integrated_lufs} LUFS`],
    ["Peak", `${analysis.metrics.peak_dbfs} dBFS`],
    ["Dynamics", `${analysis.metrics.crest_factor_db} dB crest`],
    ["Stereo width", analysis.metrics.stereo_width],
    ["Low-end control", `${analysis.metrics.low_end_control}/100`],
    ["Harshness control", `${analysis.metrics.harshness_control}/100`]
  ];
  analysisMetrics.innerHTML = rows.map(([label, value]) => `
    <article>
      <span>${label}</span>
      <strong>${value}</strong>
    </article>
  `).join("");
}

async function handleAnalysisFile(file) {
  if (!file) return;
  analysisTitle.textContent = file.name;
  analysisStatus.textContent = "Decoding and reading mix behavior...";
  analysisSubmit.disabled = true;
  analysisResult.textContent = "Analysis running locally in your browser.";
  try {
    const context = new AudioContext();
    const arrayBuffer = await file.arrayBuffer();
    const buffer = await context.decodeAudioData(arrayBuffer);
    drawSpectrum(buffer);
    currentAnalysis = analyzeAudioBuffer(buffer, file.name);
    renderAnalysis(currentAnalysis);
    analysisStatus.textContent = "Local analysis complete. Submit the metadata gate to test reference-pool eligibility.";
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
  analysisResult.textContent = "Checking pro target gate...";
  try {
    const response = await fetch(apiUrl("/api/v1/analysis/submit"), {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(currentAnalysis)
    });
    const payload = await response.json();
    if (!response.ok || !payload.ok) throw new Error(payload.error || "gate failed");
    const lines = [
      payload.accepted ? "PASSED: metadata accepted for the AIFRED reference pool." : "NOT PASSED: metadata disposed.",
      `Score: ${payload.score}/100`,
      `Persistence: ${payload.persistence}`,
      ...payload.checks.map((check) => `${check.ok ? "PASS" : "MISS"} ${check.target}`)
    ];
    analysisResult.textContent = lines.join("\n");
  } catch (error) {
    analysisResult.textContent = `Gate unavailable: ${error.message || "request failed"}`;
  } finally {
    analysisSubmit.disabled = false;
  }
}

year.textContent = new Date().getFullYear();
catalogRefresh.addEventListener("click", loadCatalog);
analysisUpload.addEventListener("change", (event) => handleAnalysisFile(event.target.files?.[0]));
analysisSubmit.addEventListener("click", submitAnalysisGate);
renderReleaseActions();
renderServices();
setupForms();
loadCatalog();

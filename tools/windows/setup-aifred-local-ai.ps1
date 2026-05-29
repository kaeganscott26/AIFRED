param(
  [string] $InstallRoot = "$env:ProgramFiles\Aifred",
  [string] $GatewayUrl = "http://127.0.0.1:8787",
  [string] $OllamaUrl = "http://127.0.0.1:11434",
  [string] $ModelName = "aifred:latest"
)

$ErrorActionPreference = "Stop"

function Write-Step($message) { Write-Host "[AIFRED] $message" }
function Command-Exists($name) { [bool](Get-Command $name -ErrorAction SilentlyContinue) }

$enginePath = Join-Path $InstallRoot "bin\AifredEngine.exe"
$configDir = Join-Path $InstallRoot "config"
$modelFile = Join-Path $InstallRoot "models\aifred\Modelfile"
$userSettingsDir = Join-Path $env:APPDATA "Aifred"
$userSettingsPath = Join-Path $userSettingsDir "user_settings.json"

New-Item -ItemType Directory -Force -Path $configDir, $userSettingsDir | Out-Null
@{
  mode = "local"
  port = 8787
  gateway_url = $GatewayUrl
  provider = "ollama"
  model_path = "models/aifred-assistant-q4.gguf"
  model_name = $ModelName
  openai_api_key = ""
  ollama_url = $OllamaUrl
  custom_endpoint = $OllamaUrl
  timeout_ms = 420000
} | ConvertTo-Json | Set-Content -Encoding UTF8 (Join-Path $configDir "config.json")
@{
  provider_override_enabled = $false
  provider_mode = "ollama"
  api_key = ""
  ollama_url = $OllamaUrl
  custom_endpoint = $OllamaUrl
  model_name = $ModelName
} | ConvertTo-Json | Set-Content -Encoding UTF8 $userSettingsPath

if (-not (Command-Exists "ollama")) {
  if (Command-Exists "winget") {
    Write-Step "Installing Ollama with winget."
    winget install --id Ollama.Ollama -e --accept-package-agreements --accept-source-agreements
  } else {
    throw "Ollama is not installed and winget is unavailable. Install Ollama from https://ollama.com/download/windows, then rerun this script."
  }
}

Write-Step "Starting Ollama if needed."
Start-Process -FilePath "ollama" -ArgumentList "serve" -WindowStyle Hidden -ErrorAction SilentlyContinue | Out-Null
Start-Sleep -Seconds 2

Write-Step "Ensuring base model llama3.2:3b exists."
if (-not ((ollama list) -match '^llama3\.2:3b\s')) {
  ollama pull llama3.2:3b
}

if (-not (Test-Path -LiteralPath $modelFile)) {
  throw "AIFRED Modelfile is missing at $modelFile."
}

Write-Step "Creating $ModelName from $modelFile."
ollama create aifred -f $modelFile

Write-Step "Verifying Ollama model response at $OllamaUrl."
$modelReply = ollama run $ModelName "Confirm AIFRED local AI is ready."
if (-not $modelReply) {
  throw "AIFRED model did not return a response through Ollama."
}

if (-not (Test-Path -LiteralPath $enginePath)) {
  throw "AifredEngine.exe is missing at $enginePath."
}

Write-Step "Registering AifredEngine startup."
$action = New-ScheduledTaskAction -Execute $enginePath
$trigger = New-ScheduledTaskTrigger -AtLogOn
$settings = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries -ExecutionTimeLimit (New-TimeSpan -Hours 0)
Register-ScheduledTask -TaskName "AIFRED Engine" -Action $action -Trigger $trigger -Settings $settings -Description "Starts AIFRED local AI engine at user login." -Force | Out-Null

Write-Step "Starting AifredEngine."
Start-Process -FilePath $enginePath -WindowStyle Hidden
Start-Sleep -Seconds 2

$health = Invoke-RestMethod -Uri "$GatewayUrl/health" -TimeoutSec 5
if (-not $health.local_ai_ready) {
  $message = if ($health.last_error) { $health.last_error } else { "local_ai_ready=false" }
  throw "AIFRED local AI verification failed: $message"
}

Write-Step "Local AI ready: engine, Ollama, and $ModelName are reachable."

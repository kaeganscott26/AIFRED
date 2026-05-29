param(
  [string] $BuildRoot = "build",
  [string] $OutputDir = "dist",
  [string] $Platform = $([System.Runtime.InteropServices.RuntimeInformation]::OSDescription -replace '[^A-Za-z0-9._-]', '-')
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$buildPath = Join-Path $repoRoot $BuildRoot
$distPath = Join-Path $repoRoot $OutputDir
New-Item -ItemType Directory -Force -Path $distPath | Out-Null

$plugin = Get-ChildItem -LiteralPath $buildPath -Recurse -Filter "Aifred.vst3" -ErrorAction SilentlyContinue | Where-Object { $_.PSIsContainer } | Select-Object -First 1
if (-not $plugin) {
  throw "Aifred.vst3 was not found under $buildPath"
}

$safePlatform = $Platform -replace '[^A-Za-z0-9._-]', '-'
$zipPath = Join-Path $distPath "AIFRED-VST3-$safePlatform.zip"
$packagePath = Join-Path $distPath "AIFRED-VST3-$safePlatform"
if (Test-Path -LiteralPath $zipPath) {
  Remove-Item -LiteralPath $zipPath -Force
}
if (Test-Path -LiteralPath $packagePath) {
  Remove-Item -LiteralPath $packagePath -Recurse -Force
}

New-Item -ItemType Directory -Force -Path $packagePath | Out-Null
Copy-Item -LiteralPath $plugin.FullName -Destination (Join-Path $packagePath "Aifred.vst3") -Recurse -Force

$productRoot = Join-Path $packagePath "Aifred"
New-Item -ItemType Directory -Force -Path (Join-Path $productRoot "bin") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $productRoot "config") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $productRoot "logs") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $productRoot "models") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $productRoot "models\aifred") | Out-Null
if ($IsWindows -or $env:OS -eq "Windows_NT") {
  $engineOut = Join-Path $distPath "engine\windows"
  & dotnet publish (Join-Path $repoRoot "tools\AifredEngine\AifredEngine.csproj") -c Release -o $engineOut
  if ($LASTEXITCODE -ne 0) {
    throw "AifredEngine publish failed."
  }
  Copy-Item -LiteralPath (Join-Path $engineOut "AifredEngine.exe") -Destination (Join-Path $productRoot "bin\AifredEngine.exe") -Force
}
@'
{
  "mode": "local",
  "port": 8787,
  "gateway_url": "http://127.0.0.1:8787",
  "provider": "ollama",
  "model_path": "models/aifred-assistant-q4.gguf",
  "model_name": "aifred:latest",
  "openai_api_key": "",
  "ollama_url": "http://127.0.0.1:11434",
  "custom_endpoint": "http://127.0.0.1:11434",
  "timeout_ms": 420000
}
'@ | Set-Content -Encoding UTF8 (Join-Path $productRoot "config\config.json")
@'
Optional offline model slot: place a licensed GGUF model here as aifred-assistant-q4.gguf.
AIFRED works out of the box with local Ollama chat. The plugin sends measured DSP snapshots to the local engine only when the user asks a question.
'@ | Set-Content -Encoding UTF8 (Join-Path $productRoot "models\README.txt")
Copy-Item -LiteralPath (Join-Path $repoRoot "models\aifred\Modelfile") -Destination (Join-Path $productRoot "models\aifred\Modelfile") -Force
New-Item -ItemType Directory -Force -Path (Join-Path $packagePath "tools\windows") | Out-Null
Copy-Item -LiteralPath (Join-Path $repoRoot "tools\windows\setup-aifred-local-ai.ps1") -Destination (Join-Path $packagePath "tools\windows\setup-aifred-local-ai.ps1") -Force
Copy-Item -LiteralPath (Join-Path $repoRoot "website\assets\docs\aifred-beta-release-notes.txt") -Destination (Join-Path $packagePath "AIFRED-BETA-NOTES.txt") -Force
@'
AIFRED VST3 Free Beta

Install:
1. Copy Aifred.vst3 to your VST3 folder or run the Windows installer when provided.
2. Run tools/windows/setup-aifred-local-ai.ps1 from an elevated PowerShell if local AI is not ready after install.
3. The local AI provider is Ollama at http://127.0.0.1:11434 using aifred:latest.
4. AifredEngine is the plugin gateway at http://127.0.0.1:8787 and should start at login after installer/setup registration.

OpenAI-compatible routing is optional and not required for local AI.
'@ | Set-Content -Encoding UTF8 (Join-Path $packagePath "README-BETA.txt")

Compress-Archive -Path (Join-Path $packagePath "*") -DestinationPath $zipPath
Write-Host "Packaged $packagePath -> $zipPath"

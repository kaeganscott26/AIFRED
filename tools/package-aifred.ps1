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
  "provider": "ollama",
  "model_path": "models/aifred-assistant-q4.gguf",
  "model_name": "aifred:latest",
  "openai_api_key": "",
  "custom_endpoint": "http://127.0.0.1:11434",
  "timeout_ms": 420000
}
'@ | Set-Content -Encoding UTF8 (Join-Path $productRoot "config\config.json")
@'
Optional offline model slot: place a licensed GGUF model here as aifred-assistant-q4.gguf.
AIFRED works out of the box with local Ollama chat. The plugin sends measured DSP snapshots to the local engine only when the user asks a question.
'@ | Set-Content -Encoding UTF8 (Join-Path $productRoot "models\README.txt")

Compress-Archive -Path (Join-Path $packagePath "*") -DestinationPath $zipPath
Write-Host "Packaged $packagePath -> $zipPath"

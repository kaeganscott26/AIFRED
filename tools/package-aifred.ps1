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

$engineOut = Join-Path $distPath "engine\windows"
& dotnet publish (Join-Path $repoRoot "tools\AifredEngine\AifredEngine.csproj") -c Release -o $engineOut
if ($LASTEXITCODE -ne 0) {
  throw "AifredEngine publish failed."
}
$productRoot = Join-Path $packagePath "Aifred"
New-Item -ItemType Directory -Force -Path (Join-Path $productRoot "bin") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $productRoot "config") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $productRoot "logs") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $productRoot "models") | Out-Null
Copy-Item -LiteralPath (Join-Path $engineOut "AifredEngine.exe") -Destination (Join-Path $productRoot "bin\AifredEngine.exe") -Force
@'
{
  "mode": "local",
  "port": 8787,
  "provider": "bundled-local",
  "model_path": "C:\\Program Files\\Aifred\\models\\aifred-assistant-q4.gguf",
  "openai_api_key": "",
  "custom_endpoint": "",
  "timeout_ms": 8000
}
'@ | Set-Content -Encoding UTF8 (Join-Path $productRoot "config\config.json")
@'
Place the licensed bundled AIFRED GGUF model here as aifred-assistant-q4.gguf before producing a public installer.
The engine falls back to deterministic rule-based coaching when the model is unavailable.
'@ | Set-Content -Encoding UTF8 (Join-Path $productRoot "models\README.txt")

Compress-Archive -Path (Join-Path $packagePath "*") -DestinationPath $zipPath
Write-Host "Packaged $packagePath -> $zipPath"

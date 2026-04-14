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

@(
  '$ErrorActionPreference = "Stop"',
  '$source = Join-Path $PSScriptRoot "Aifred.vst3"',
  '$target = Join-Path $env:COMMONPROGRAMFILES "VST3\Aifred.vst3"',
  'New-Item -ItemType Directory -Force -Path (Split-Path $target) | Out-Null',
  'Copy-Item -Recurse -Force -LiteralPath $source -Destination $target',
  'Write-Host "AIFRED installed to $target. Rescan plugins in FL Studio."'
) | Set-Content (Join-Path $packagePath "install-aifred.ps1")

Compress-Archive -Path (Join-Path $packagePath "*") -DestinationPath $zipPath
Write-Host "Packaged $packagePath -> $zipPath"

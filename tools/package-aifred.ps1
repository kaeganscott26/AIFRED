[CmdletBinding()]
param([string] $BuildRoot='out/windows-x64/build', [string] $OutputDir='out/windows-x64/stage', [ValidateSet('windows')] [string] $Platform='windows')
$ErrorActionPreference='Stop'
$repoRoot=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$stage=Join-Path $repoRoot $OutputDir
$package=Join-Path $stage 'AIFRED-VST3-windows'
if (Test-Path -LiteralPath $package) { throw 'Candidate must be prepared by the canonical release pipeline.' }
$plugin=Join-Path $repoRoot "$BuildRoot/plugin-aifred/Aifred_artefacts/Release/VST3/Aifred.vst3"
if (!(Test-Path -LiteralPath (Join-Path $plugin 'Contents/x86_64-win/Aifred.vst3'))) { throw 'Exact VST3 target missing.' }
New-Item -ItemType Directory -Path $package | Out-Null
Copy-Item -LiteralPath $plugin -Destination (Join-Path $package 'Aifred.vst3') -Recurse
& dotnet publish (Join-Path $repoRoot 'tools/AifredIntelligenceHost/AifredIntelligenceHost.csproj') -c Release -r win-x64 --self-contained false -o (Join-Path $package 'AifredIntelligenceHost')
if ($LASTEXITCODE -ne 0) { throw 'Intelligence Host publish failed.' }
'{"channel":"beta"}' | Set-Content -Encoding utf8 (Join-Path $package 'AifredIntelligenceHost/channel.json')
Copy-Item -LiteralPath (Join-Path $repoRoot 'README.md') -Destination (Join-Path $package 'README.md')
Compress-Archive -Path (Join-Path $package '*') -DestinationPath (Join-Path $stage 'AIFRED-VST3-windows.zip')

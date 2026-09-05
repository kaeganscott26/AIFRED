[CmdletBinding()]
param([switch] $ReplaceSharedSlot)
$ErrorActionPreference = 'Stop'
if (!$ReplaceSharedSlot) { throw 'Explicit -ReplaceSharedSlot is required: Beta and Official currently share the VST3 install slot.' }
$root = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../..')).Path
& python -B (Join-Path $root 'scripts/common/release.py') verify --platform windows-x64
if ($LASTEXITCODE -ne 0) { throw 'Artifact verification failed.' }
Start-Process -FilePath (Join-Path $root 'out/windows-x64/current/installer/AIFRED-VST3-Setup.exe') -Verb RunAs -Wait

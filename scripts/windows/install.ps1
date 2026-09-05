[CmdletBinding()]
param()
. (Join-Path $PSScriptRoot '../common/install-ownership.ps1')
& python -B (Join-Path $repositoryRoot 'scripts/common/release.py') verify --platform windows-x64
if ($LASTEXITCODE -ne 0) {throw 'Current artifact verification failed.'}
$current=Join-Path $repositoryRoot 'out/windows-x64/current'
$manifest=Get-Content -Raw -LiteralPath (Join-Path $current 'manifest.json') | ConvertFrom-Json
Stop-OwnedHost
Install-OwnedTree (Join-Path $current $manifest.plugin) $pluginParent 'Aifred.vst3'
Install-OwnedTree (Join-Path $current $manifest.engine) $hostParent 'IntelligenceHost'
New-Item -Path $runKey -Force | Out-Null
Set-ItemProperty -LiteralPath $runKey -Name $startupName -Value "`"$hostExe`" --channel $channel"
& (Join-Path $PSScriptRoot 'start-host.ps1')
Write-Host "$displayChannel installed. User settings retained; DAW rescan/load validation required."

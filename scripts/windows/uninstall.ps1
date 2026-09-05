[CmdletBinding()]
param()
. (Join-Path $PSScriptRoot '../common/install-ownership.ps1')
Stop-OwnedHost
if (Get-ItemProperty -LiteralPath $runKey -Name $startupName -ErrorAction SilentlyContinue) {Remove-ItemProperty -LiteralPath $runKey -Name $startupName}
Remove-OwnedTree (Join-Path $pluginParent 'Aifred.vst3') $pluginParent
Remove-OwnedTree $hostTarget $hostParent
Write-Host "$displayChannel binaries removed. User settings and other channels retained."

[CmdletBinding()]
param([Parameter(Mandatory)] [ValidateSet('uninstall','update')] [string] $Action)
$ErrorActionPreference='Stop'
if ($Action -eq 'uninstall') { & (Join-Path $PSScriptRoot 'uninstall.ps1'); return }
& (Join-Path $PSScriptRoot 'build.ps1') -Action release
& (Join-Path $PSScriptRoot 'install.ps1')

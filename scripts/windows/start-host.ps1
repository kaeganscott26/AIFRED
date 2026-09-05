[CmdletBinding()]
param()
. (Join-Path $PSScriptRoot '../common/install-ownership.ps1')
if (!(Test-Path -LiteralPath $hostExe)) {throw 'Install the canonical current artifact first.'}
if (Get-Process -Name AifredIntelligenceHost -ErrorAction SilentlyContinue | Where-Object {$_.Path -eq $hostExe}) {return}
$logs=Join-Path $hostParent 'logs';New-Item -ItemType Directory -Force -Path $logs | Out-Null
Start-Process -FilePath $hostExe -ArgumentList @('--channel',$channel) -WindowStyle Hidden -RedirectStandardOutput (Join-Path $logs 'host.log') -RedirectStandardError (Join-Path $logs 'host-error.log')

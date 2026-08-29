param([Parameter(Mandatory=$true)][string]$Destination)
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$SourcePath = Join-Path $RepoRoot "integrations/forge"
$Parent = Split-Path -Parent $Destination
New-Item -ItemType Directory -Force -Path $Parent | Out-Null
if (Test-Path -LiteralPath $Destination) { throw "Refusing to overwrite existing destination: $Destination" }
New-Item -ItemType SymbolicLink -Path $Destination -Target $SourcePath | Out-Null
Write-Output "$Destination -> $SourcePath"

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
function Invoke-Checked {
    param([string] $Program, [string[]] $Arguments)
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed with exit code $LASTEXITCODE" }
}
function Initialize-AifredMsvc {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    if (!(Test-Path -LiteralPath $vswhere)) { throw 'Install Visual Studio C++ Build Tools and Windows SDK.' }
    $installation = (& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath).Trim()
    if (!$installation) { throw 'Visual Studio x64 C++ tools are unavailable.' }
    $developerCommand = Join-Path $installation 'Common7/Tools/VsDevCmd.bat'
    $lines = & cmd.exe /d /s /c "`"$developerCommand`" -arch=x64 -host_arch=x64 >nul && set"
    if ($LASTEXITCODE -ne 0) { throw 'MSVC environment discovery failed.' }
    foreach ($line in $lines) {
        $parts = $line -split '=', 2
        if ($parts.Count -eq 2 -and $parts[0]) { [Environment]::SetEnvironmentVariable($parts[0], $parts[1], 'Process') }
    }
}

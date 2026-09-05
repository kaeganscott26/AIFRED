[CmdletBinding()]
param([ValidateSet('configure','build','test','stage','package','release')] [string] $Action = 'release')
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '../common/windows.ps1')
$repositoryRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../..')).Path
$platformRoot = Join-Path $repositoryRoot 'out/windows-x64'
$buildRoot = Join-Path $platformRoot 'build'
$stageRoot = Join-Path $platformRoot 'stage'
$layout = Get-Content -Raw -LiteralPath (Join-Path $repositoryRoot 'scripts/release-layout.json') | ConvertFrom-Json
$official = $layout.product -eq 'AIFRED 4'
New-Item -ItemType Directory -Force -Path $platformRoot | Out-Null
$buildLock = [IO.File]::Open((Join-Path $platformRoot 'pipeline.lock'), [IO.FileMode]::OpenOrCreate, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None)
Push-Location $repositoryRoot
try {
    Initialize-AifredMsvc
    Invoke-Checked cmake @('--preset','windows-release')
    if ($Action -eq 'configure') { return }
    $targets = @('Aifred_VST3')
    if ($official) { $targets += @('aifred_dsp_smoke','aifred_comparison_tests','aifred_integration_contract_tests') }
    Invoke-Checked cmake (@('--build','--preset','windows-release','--target') + $targets)
    if ($Action -eq 'build') { return }
    Invoke-Checked python @('-B','scripts/common/check_repository.py')
    Invoke-Checked python @('-B','-m','unittest','discover','-s','scripts/tests')
    if ($official) {
        Invoke-Checked ctest @('--preset','windows-release')
        Invoke-Checked dotnet @('run','--project','tools/AifredEngine.Tests/AifredEngine.ContractTests.csproj','-c','Release')
        foreach ($suite in @('python_brain/tests','ai_engine/tests','bridge/tests')) {
            Invoke-Checked python @('-B','-m','unittest','discover','-s',$suite)
        }
    } else {
        Invoke-Checked pwsh @('-NoProfile','-File','tools/check-aifred-analysis-regressions.ps1')
        Invoke-Checked node @('--test','tests/aifred-api.test.mjs','tests/aifred-archive.test.mjs')
        Invoke-Checked npm @('--prefix','apps','run','website:check')
    }
    if ($Action -eq 'test') { return }
    Invoke-Checked python @('-B','scripts/common/release.py','prepare','--platform','windows-x64')
    $sourceBundle = Join-Path $buildRoot $layout.platforms.'windows-x64'.plugin
    if (!(Test-Path -LiteralPath (Join-Path $sourceBundle 'Contents/x86_64-win/Aifred.vst3'))) { throw 'Exact expected VST3 binary is missing.' }
    if ($official) {
        Copy-Item -LiteralPath $sourceBundle -Destination (Join-Path $stageRoot 'Aifred.vst3') -Recurse
        Invoke-Checked dotnet @('publish','tools/AifredEngine/AifredEngine.csproj','-c','Release','-r','win-x64','--self-contained','false','-o',(Join-Path $stageRoot 'AifredEngine'))
    } else {
        Invoke-Checked pwsh @('-NoProfile','-File','tools/package-aifred.ps1','-BuildRoot','out/windows-x64/build','-OutputDir','out/windows-x64/stage','-Platform','windows')
        Invoke-Checked dotnet @('publish','tools/AifredWindowsInstaller/AifredWindowsInstaller.csproj','-c','Release','-o',(Join-Path $stageRoot 'installer'))
        Invoke-Checked dotnet @('publish','tools/AifredWindowsUninstaller/AifredWindowsUninstaller.csproj','-c','Release','-o',(Join-Path $stageRoot 'uninstaller'))
    }
    Invoke-Checked python @('-B','scripts/common/release.py','manifest','--platform','windows-x64')
    Invoke-Checked python @('-B','scripts/common/release.py','verify','--platform','windows-x64','--location','stage')
    if ($Action -eq 'release') {
        Invoke-Checked python @('-B','scripts/common/release.py','promote','--platform','windows-x64')
    }
} finally {
    Pop-Location
    $buildLock.Dispose()
}

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
    $targets = @('Aifred_VST3','aifred_frontend_contract_tests','aifred_fixture_meter','aifred_state_contract_tests','aifred_core_tests')
    Invoke-Checked cmake (@('--build','--preset','windows-release','--target') + $targets)
    if ($Action -eq 'build') { return }
    Invoke-Checked python @('-B','scripts/common/check_repository.py')
    Invoke-Checked python @('-B','-m','unittest','discover','-s','scripts/tests')
    Invoke-Checked dotnet @('run','--project','tools/AifredIntelligenceHost.Tests/AifredIntelligenceHost.ContractTests.csproj','-c','Release')
    if ($official) {
        Invoke-Checked ctest @('--preset','windows-release')

    } else {
        Invoke-Checked ctest @('--preset','windows-release')
        Invoke-Checked node @('--test','tests/aifred-api.test.mjs','tests/aifred-archive.test.mjs')
        Invoke-Checked npm @('--prefix','apps','run','website:check')
    }
    Invoke-Checked python @('-B','scripts/common/check_shared_core.py')
    if ($Action -eq 'test') { return }
    Invoke-Checked python @('-B','scripts/common/release.py','prepare','--platform','windows-x64')
    $sourceBundle = Join-Path $buildRoot $layout.platforms.'windows-x64'.plugin
    if (!(Test-Path -LiteralPath (Join-Path $sourceBundle 'Contents/x86_64-win/Aifred.vst3'))) { throw 'Exact expected VST3 binary is missing.' }
    if ($official) {
        Copy-Item -LiteralPath $sourceBundle -Destination (Join-Path $stageRoot 'Aifred.vst3') -Recurse
        Invoke-Checked dotnet @('publish','tools/AifredIntelligenceHost/AifredIntelligenceHost.csproj','-c','Release','-r','win-x64','--self-contained','false','-o',(Join-Path $stageRoot 'AifredIntelligenceHost'))
    } else {
        Invoke-Checked pwsh @('-NoProfile','-File','tools/package-aifred.ps1','-BuildRoot','out/windows-x64/build','-OutputDir','out/windows-x64/stage','-Platform','windows')
        Invoke-Checked dotnet @('publish','tools/AifredWindowsInstaller/AifredWindowsInstaller.csproj','-c','Release','-o',(Join-Path $stageRoot 'installer'))
        Invoke-Checked dotnet @('publish','tools/AifredWindowsUninstaller/AifredWindowsUninstaller.csproj','-c','Release','-o',(Join-Path $stageRoot 'uninstaller'))
    }
    if ($official) {
        '{"channel":"official"}' | Set-Content -Encoding utf8 (Join-Path $stageRoot 'AifredIntelligenceHost/channel.json')
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

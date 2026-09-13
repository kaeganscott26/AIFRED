[CmdletBinding()]
param(
    [string] $BuildRoot = 'out/windows-x64/build',
    [string] $OutputDir = 'out/windows-x64/stage',
    [ValidateSet('windows')] [string] $Platform = 'windows'
)
$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$stage = Join-Path $repoRoot $OutputDir
$build = Join-Path $repoRoot $BuildRoot
$scratch = Join-Path $build 'packaging'
$payload = Join-Path $scratch 'payload'
$payloadArchive = Join-Path $scratch 'payload.zip'
$lifecycleProject = Join-Path $repoRoot 'tools/AifredWindowsLifecycle/AifredWindowsLifecycle.csproj'
$version = '0.4.0'

function Invoke-Dotnet([string[]] $Arguments) {
    & dotnet @Arguments
    if ($LASTEXITCODE -ne 0) { throw "dotnet failed with exit code $LASTEXITCODE" }
}

function Publish-Lifecycle([string] $Mode, [string] $Name) {
    $destination = Join-Path $scratch ("publish-" + $Mode.ToLowerInvariant())
    New-Item -ItemType Directory -Force -Path $destination | Out-Null
    $arguments = @(
        'publish', $lifecycleProject, '-c', 'Release', '-r', 'win-x64', '--self-contained', 'true',
        "-p:LifecycleMode=$Mode", "-p:AssemblyName=$Name", "-p:Version=$version", '-o', $destination
    )
    if ($Mode -eq 'Setup') { $arguments += "-p:AifredPayloadPath=$payloadArchive" }
    Invoke-Dotnet $arguments
    $executable = Join-Path $destination "$Name.exe"
    if (!(Test-Path -LiteralPath $executable)) { throw "Missing published lifecycle executable: $Name.exe" }
    return $executable
}

& python -B (Join-Path $repoRoot 'scripts/common/release.py') prepare_scratch --platform windows-x64
if ($LASTEXITCODE -ne 0) { throw 'Could not prepare owned packaging scratch.' }
New-Item -ItemType Directory -Force -Path $payload | Out-Null

$plugin = Join-Path $build 'plugin-aifred/Aifred_artefacts/Release/VST3/Aifred.vst3'
if (!(Test-Path -LiteralPath (Join-Path $plugin 'Contents/x86_64-win/Aifred.vst3'))) { throw 'Exact VST3 target missing.' }
Copy-Item -LiteralPath $plugin -Destination (Join-Path $payload 'Aifred.vst3') -Recurse

$host = Join-Path $payload 'IntelligenceHost'
Invoke-Dotnet @(
    'publish', (Join-Path $repoRoot 'tools/AifredIntelligenceHost/AifredIntelligenceHost.csproj'),
    '-c', 'Release', '-r', 'win-x64', '--self-contained', 'true', '-p:PublishSingleFile=true',
    '-p:IncludeNativeLibrariesForSelfExtract=true', '-p:DebugType=none', '-p:DebugSymbols=false', '-o', $host
)
'{"channel":"beta"}' | Set-Content -Encoding utf8 -LiteralPath (Join-Path $host 'channel.json')

$updater = Publish-Lifecycle 'Update' 'AIFRED-Beta-Updater'
$uninstaller = Publish-Lifecycle 'Uninstall' 'AIFRED-Beta-Uninstall'
$lifecyclePayload = Join-Path $payload 'Lifecycle'
New-Item -ItemType Directory -Force -Path $lifecyclePayload | Out-Null
Copy-Item -LiteralPath $updater -Destination (Join-Path $lifecyclePayload 'AIFRED-Beta-Updater.exe')
Copy-Item -LiteralPath $uninstaller -Destination (Join-Path $lifecyclePayload 'AIFRED-Beta-Uninstall.exe')

$configuration = Join-Path $payload 'Configuration'
New-Item -ItemType Directory -Force -Path $configuration | Out-Null
Copy-Item -LiteralPath (Join-Path $repoRoot 'config/distribution/aifred-settings.example.json') -Destination $configuration
Copy-Item -LiteralPath (Join-Path $repoRoot 'config/distribution/README.md') -Destination $configuration
Copy-Item -LiteralPath (Join-Path $repoRoot 'README.md') -Destination (Join-Path $payload 'README.md')

$hashes = [ordered]@{}
Get-ChildItem -LiteralPath $payload -File -Recurse | Sort-Object FullName | ForEach-Object {
    $relative = [IO.Path]::GetRelativePath($payload, $_.FullName).Replace('\', '/')
    $hashes[$relative] = (Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash.ToLowerInvariant()
}
[ordered]@{
    schema = 'aifred.windows-payload.v1'
    product = 'AIFRED Beta'
    channel = 'beta'
    version = $version
    git_sha = (& git -C $repoRoot rev-parse HEAD).Trim()
    files = $hashes
} | ConvertTo-Json -Depth 5 | Set-Content -Encoding utf8 -LiteralPath (Join-Path $payload 'payload-manifest.json')

Compress-Archive -Path (Join-Path $payload '*') -DestinationPath $payloadArchive -CompressionLevel Optimal
$setup = Publish-Lifecycle 'Setup' 'AIFRED-Beta-Setup'
New-Item -ItemType Directory -Force -Path $stage | Out-Null
Copy-Item -LiteralPath $setup -Destination (Join-Path $stage 'AIFRED-Beta-Setup.exe')
Copy-Item -LiteralPath $updater -Destination (Join-Path $stage 'AIFRED-Beta-Updater.exe')
Copy-Item -LiteralPath $uninstaller -Destination (Join-Path $stage 'AIFRED-Beta-Uninstall.exe')

[ordered]@{
    schema = 'aifred.windows-package-receipt.v1'
    product = 'AIFRED Beta'
    channel = 'beta'
    version = $version
    git_sha = (& git -C $repoRoot rev-parse HEAD).Trim()
    plugin_sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $plugin 'Contents/x86_64-win/Aifred.vst3')).Hash.ToLowerInvariant()
    payload_sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $payloadArchive).Hash.ToLowerInvariant()
} | ConvertTo-Json | Set-Content -Encoding utf8 -LiteralPath (Join-Path $stage 'packaging-receipt.json')

Write-Host 'AIFRED Beta executable lifecycle package assembled and hash-verified.'

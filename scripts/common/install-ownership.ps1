$ErrorActionPreference='Stop'
$repositoryRoot=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../..')).Path
$layout=Get-Content -Raw -LiteralPath (Join-Path $repositoryRoot 'scripts/release-layout.json') | ConvertFrom-Json
$channel=$layout.runtimeChannel
$displayChannel=(Get-Culture).TextInfo.ToTitleCase($channel)
$pluginParent=Join-Path $env:COMMONPROGRAMFILES "VST3/AIFRED $displayChannel"
$hostParent=Join-Path $env:LOCALAPPDATA "Aifred/$channel"
$hostTarget=Join-Path $hostParent 'IntelligenceHost'
$hostExe=Join-Path $hostTarget 'AifredIntelligenceHost.exe'
$startupName="AIFRED $displayChannel Intelligence Host"
$runKey='HKCU:\Software\Microsoft\Windows\CurrentVersion\Run'
function Assert-OwnedTree([string]$Target,[string]$Parent) {
    $targetPath=[IO.Path]::GetFullPath($Target);$parentPath=[IO.Path]::GetFullPath($Parent)
    if (!$targetPath.StartsWith($parentPath.TrimEnd('\')+'\',[StringComparison]::OrdinalIgnoreCase)) {throw 'Install target escaped owner.'}
    $ancestor=$targetPath
    while ($ancestor) {
        if ((Test-Path -LiteralPath $ancestor) -and ((Get-Item -LiteralPath $ancestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {throw 'Reparse point in install path.'}
        $ancestor=[IO.Path]::GetDirectoryName($ancestor)
    }
    if (Test-Path -LiteralPath $targetPath) {
        if (Get-ChildItem -LiteralPath $targetPath -Recurse -Force | Where-Object {$_.Attributes -band [IO.FileAttributes]::ReparsePoint}) {throw 'Reparse point in installed tree.'}
    }
}
function Remove-OwnedTree([string]$Target,[string]$Parent) {
    Assert-OwnedTree $Target $Parent
    if (Test-Path -LiteralPath $Target) {
        Add-Type -AssemblyName Microsoft.VisualBasic
        [Microsoft.VisualBasic.FileIO.FileSystem]::DeleteDirectory([IO.Path]::GetFullPath($Target),'OnlyErrorDialogs','SendToRecycleBin','ThrowException')
    }
}
function Stop-OwnedHost {
    Get-Process -Name AifredIntelligenceHost -ErrorAction SilentlyContinue | ForEach-Object {
        if (!$_.Path) {throw 'Cannot establish host process ownership.'}
        if ([string]::Equals($_.Path,$hostExe,[StringComparison]::OrdinalIgnoreCase)) {Stop-Process -Id $_.Id;Wait-Process -Id $_.Id -Timeout 10 -ErrorAction SilentlyContinue}
    }
}
function Install-OwnedTree([string]$Source,[string]$Parent,[string]$Name) {
    $target=Join-Path $Parent $Name;$candidate="$target.candidate";$previous="$target.previous"
    foreach($path in @($target,$candidate,$previous)) {Assert-OwnedTree $path $Parent}
    if ((Test-Path -LiteralPath $candidate) -or (Test-Path -LiteralPath $previous)) {throw 'Retained installation recovery requires inspection.'}
    New-Item -ItemType Directory -Force -Path $Parent | Out-Null
    Copy-Item -LiteralPath $Source -Destination $candidate -Recurse
    foreach($file in Get-ChildItem -LiteralPath $Source -File -Recurse) {
        $relative=[IO.Path]::GetRelativePath($Source,$file.FullName)
        if ((Get-FileHash -LiteralPath $file.FullName).Hash -ne (Get-FileHash -LiteralPath (Join-Path $candidate $relative)).Hash) {throw 'Installed hash mismatch.'}
    }
    if (Test-Path -LiteralPath $target) {Move-Item -LiteralPath $target -Destination $previous}
    try {Move-Item -LiteralPath $candidate -Destination $target}
    catch {if(Test-Path -LiteralPath $previous){Move-Item -LiteralPath $previous -Destination $target};throw}
    Remove-OwnedTree $previous $Parent
}

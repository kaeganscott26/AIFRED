$ErrorActionPreference = "Stop"

$source = Join-Path $PSScriptRoot "AIFRED-Admin-Desktop.ps1"
$installRoot = Join-Path $env:LOCALAPPDATA "Programs\AIFRED Admin"
$target = Join-Path $installRoot "AIFRED-Admin-Desktop.ps1"
$desktopShortcut = Join-Path ([Environment]::GetFolderPath("Desktop")) "AIFRED Admin Desktop.lnk"

New-Item -ItemType Directory -Force -Path $installRoot | Out-Null
Copy-Item -LiteralPath $source -Destination $target -Force

$shell = New-Object -ComObject WScript.Shell
$shortcut = $shell.CreateShortcut($desktopShortcut)
$shortcut.TargetPath = "powershell.exe"
$shortcut.Arguments = "-NoProfile -ExecutionPolicy Bypass -File `"$target`""
$shortcut.WorkingDirectory = $installRoot
$shortcut.IconLocation = "powershell.exe,0"
$shortcut.Description = "AIFRED Admin Desktop"
$shortcut.Save()

[Environment]::SetEnvironmentVariable("AIFRED_REPO_ROOT", (Resolve-Path (Join-Path $PSScriptRoot "..\..\..\..")).Path, "User")

Write-Host "Installed AIFRED Admin Desktop to $target"
Write-Host "Shortcut created at $desktopShortcut"

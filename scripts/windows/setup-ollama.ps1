[CmdletBinding()]
param([string]$Model = 'aifred:latest')
$ErrorActionPreference = 'Stop'

$endpoint = 'http://127.0.0.1:11434'
$ollama = @(
    (Join-Path $env:LOCALAPPDATA 'Programs/Ollama/ollama.exe'),
    (Join-Path $env:LOCALAPPDATA 'Ollama/ollama.exe'),
    (Join-Path $env:ProgramFiles 'Ollama/ollama.exe')
) | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1

if (!$ollama) {
    $installer = Join-Path $env:TEMP 'AIFRED-OllamaSetup.exe'
    Invoke-WebRequest -Uri 'https://ollama.com/download/OllamaSetup.exe' -OutFile $installer
    $setup = Start-Process -FilePath $installer -Wait -PassThru
    if ($setup.ExitCode -ne 0) { throw "Ollama setup exited with code $($setup.ExitCode)." }
    $ollama = 1..30 | ForEach-Object {
        Start-Sleep -Seconds 1
        @(
            (Join-Path $env:LOCALAPPDATA 'Programs/Ollama/ollama.exe'),
            (Join-Path $env:LOCALAPPDATA 'Ollama/ollama.exe'),
            (Join-Path $env:ProgramFiles 'Ollama/ollama.exe')
        ) | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
        if ($ollama) { return $ollama }
    } | Select-Object -Last 1
}
if (!$ollama) { throw 'Ollama was not found after setup.' }

function Test-OllamaReady {
    try { Invoke-RestMethod -Uri "$endpoint/api/tags" -TimeoutSec 2 | Out-Null; return $true }
    catch { return $false }
}

if (!(Test-OllamaReady)) { Start-Process -FilePath $ollama -ArgumentList 'serve' -WindowStyle Hidden | Out-Null }
for ($attempt = 0; $attempt -lt 60 -and !(Test-OllamaReady); $attempt++) { Start-Sleep -Seconds 1 }
if (!(Test-OllamaReady)) { throw 'Ollama did not become ready on port 11434.' }

$tags = Invoke-RestMethod -Uri "$endpoint/api/tags"
$installed = @($tags.models | ForEach-Object { $_.name }) -contains $Model
if (!$installed) {
    $pull = Start-Process -FilePath $ollama -ArgumentList @('pull', $Model) -Wait -PassThru -NoNewWindow
    if ($pull.ExitCode -ne 0) { throw "Ollama could not download $Model." }
}
Write-Host "Ollama is ready with $Model."
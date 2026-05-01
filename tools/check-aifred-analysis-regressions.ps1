Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Clamp01([double]$Value) {
  return [Math]::Min(1.0, [Math]::Max(0.0, $Value))
}

function DistanceScore([double]$Current, [double]$Target, [double]$Tolerance, [double]$PerfectDelta) {
  if ([double]::IsNaN($Current) -or [double]::IsInfinity($Current) -or
      [double]::IsNaN($Target) -or [double]::IsInfinity($Target) -or
      $Tolerance -le 0.0) {
    return 0.0
  }

  $score = 1.0 - (Clamp01 ([Math]::Abs($Current - $Target) / $Tolerance))
  if ($score -gt 0.99 -and [Math]::Abs($Current - $Target) -gt $PerfectDelta) {
    return 0.99
  }
  return Clamp01 $score
}

function Percent([double]$Score, [bool]$Valid) {
  if (-not $Valid) { return 0 }
  return [Math]::Round((Clamp01 $Score) * 100.0)
}

function Assert-NotPerfect([string]$Name, [int]$Percent) {
  if ($Percent -ge 100) {
    throw "$Name incorrectly displays $Percent percent."
  }
}

$valid = $true
$widthDistance = DistanceScore 0.206 0.520 0.54 0.005
$phaseRisk = 0.70
$widthScore = 1.0 - (Clamp01 ((0.70 * (1.0 - $widthDistance)) + (0.30 * $phaseRisk)))
Assert-NotPerfect "Width 0.206 vs reference 0.520 with correlation 0.001" (Percent $widthScore $valid)

$punchCrestScore = DistanceScore 7.2 10.4 10.0 0.05
$punchTransientScore = DistanceScore 0.073 0.580 0.62 0.005
$punchScore = 1.0 - (Clamp01 ((0.62 * (1.0 - $punchCrestScore)) + (0.38 * (1.0 - $punchTransientScore))))
Assert-NotPerfect "Punch 0.073 vs reference 0.580" (Percent $punchScore $valid)

foreach ($case in @(
  @{ Name = "missing reference"; Valid = $false },
  @{ Name = "no signal"; Valid = $false },
  @{ Name = "invalid metrics"; Valid = $false }
)) {
  Assert-NotPerfect $case.Name (Percent 1.0 $case.Valid)
}

$badChat = "doesn\u2019t {`"x`":NaN} undefined"
$cleanChat = $badChat.Replace("\u2019", "'").Replace("u2019", "'").Replace("NaN", "unavailable").Replace("undefined", "unavailable")
foreach ($token in @("u2019", "NaN", "undefined")) {
  if ($cleanChat.Contains($token)) {
    throw "Chat sanitizer regression: visible text still contains $token."
  }
}

$source = Get-Content "$PSScriptRoot\..\plugin-aifred\Source\DiagnosticInterpreter.cpp" -Raw
foreach ($required in @("analysis_snapshot", "displayed_width_percent", "mix_health_5s", "current_analysis_window")) {
  if (-not $source.Contains($required)) {
    throw "Canonical chat context is missing $required."
  }
}

Write-Host "AIFRED analysis regression checks passed."

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$scanRoots = @(
  "plugin-aifred",
  "website",
  "android_admin",
  ".github",
  "tools",
  "CMakeLists.txt",
  "CMakePresets.json"
)

$patterns = @(
  "DAWAI",
  "DawAI",
  "dawai",
  "Pooh",
  "/home/",
  "C:\\Users\\",
  "apps/website",
  "AIFR3D"
)

$textExtensions = @(
  ".cmake",
  ".css",
  ".gradle",
  ".html",
  ".js",
  ".json",
  ".kts",
  ".kt",
  ".md",
  ".ps1",
  ".toml",
  ".txt",
  ".xml",
  ".yml"
)

$findings = @()
foreach ($root in $scanRoots) {
  $path = Join-Path $repoRoot $root
  if (-not (Test-Path -LiteralPath $path)) {
    continue
  }
  $items = if ((Get-Item -LiteralPath $path).PSIsContainer) {
    Get-ChildItem -LiteralPath $path -Recurse -File
  } else {
    Get-Item -LiteralPath $path
  }
  foreach ($item in $items) {
    if ($item.Extension -and ($textExtensions -notcontains $item.Extension.ToLowerInvariant())) {
      continue
    }
    if ($item.FullName -like "*\build\*" -or $item.FullName -like "*\.gradle\*") {
      continue
    }
    if ($item.FullName -eq $PSCommandPath) {
      continue
    }
    foreach ($pattern in $patterns) {
      $hit = Select-String -LiteralPath $item.FullName -Pattern $pattern -SimpleMatch -ErrorAction SilentlyContinue
      if ($hit) {
        $findings += $hit
      }
    }
  }
}

if ($findings.Count -gt 0) {
  $findings | ForEach-Object {
    $line = if ($null -ne $_.Line) { $_.Line.Trim() } else { "" }
    Write-Error "$($_.Path):$($_.LineNumber): $line"
  }
  exit 1
}

Write-Host "No blocked hardcoded paths or old active product names found."

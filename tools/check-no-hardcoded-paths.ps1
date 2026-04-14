$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$patterns = "DAWAI|DawAI|dawai|Pooh|/home/|C:\\Users\\|apps/website|AIFR3D"
$scanArgs = @(
  "-n",
  "--glob", "!**/build/**",
  "--glob", "!**/.gradle/**",
  "--glob", "!tools/check-no-hardcoded-paths.ps1",
  "--glob", "*.{cmake,css,gradle,html,js,json,kts,kt,md,ps1,toml,txt,xml,yml}",
  $patterns,
  "plugin-aifred",
  "website",
  "android_admin",
  ".github",
  "CMakeLists.txt",
  "CMakePresets.json",
  "README.md"
)

$rg = Get-Command rg -ErrorAction SilentlyContinue
if ($rg) {
  Push-Location $repoRoot
  try {
    & rg @scanArgs
    $exit = $LASTEXITCODE
  } finally {
    Pop-Location
  }
  if ($exit -eq 0) {
    throw "Blocked hardcoded path or old active product name found."
  }
  if ($exit -eq 1) {
    Write-Host "No blocked hardcoded paths or old active product names found."
    exit 0
  }
  exit $exit
}

Write-Host "ripgrep not found; skipping active path/name guard."

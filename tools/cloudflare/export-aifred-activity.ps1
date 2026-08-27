[CmdletBinding()]
param(
  [string]$AccountId = "b5bd4e29593c5e9ebb17ce26f2ae8f8d",
  [string]$NamespaceId = "2c66da7795b54135a4d67e514b97491f",
  [string]$OutputPath = "",
  [switch]$Force
)

$ErrorActionPreference = "Stop"
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot "..\.."))
if ([string]::IsNullOrWhiteSpace($OutputPath)) {
  $stamp = [DateTime]::UtcNow.ToString("yyyyMMddTHHmmssZ")
  $OutputPath = Join-Path $repoRoot "smart-env\Aifred_Site_Activity_$stamp.jsonl"
} elseif (-not [IO.Path]::IsPathRooted($OutputPath)) {
  $OutputPath = Join-Path $repoRoot $OutputPath
}
$OutputPath = [IO.Path]::GetFullPath($OutputPath)
if ((Test-Path -LiteralPath $OutputPath) -and -not $Force) {
  throw "Output already exists. Choose a new path or pass -Force: $OutputPath"
}

$authRaw = (& npx --yes wrangler@4.125.0 auth token --json 2>&1 | Out-String)
if ($LASTEXITCODE -ne 0) { throw "Wrangler authentication failed. Run: npx --yes wrangler@4.125.0 login" }
$auth = $authRaw | ConvertFrom-Json -ErrorAction Stop
if ($auth -is [string]) { $token = [string]$auth }
elseif ($null -ne $auth.token) { $token = [string]$auth.token }
elseif ($null -ne $auth.access_token) { $token = [string]$auth.access_token }
else { throw "Wrangler auth output did not contain a token." }
if ([string]::IsNullOrWhiteSpace($token)) { throw "Wrangler returned an empty token." }

$apiBase = "https://api.cloudflare.com/client/v4/accounts/$AccountId/storage/kv/namespaces/$NamespaceId"
$headers = @{ Authorization = "Bearer $token" }
$keys = [Collections.Generic.List[object]]::new()
$cursor = $null
$pageCount = 0
do {
  $uri = "${apiBase}/keys?limit=1000"
  if (-not [string]::IsNullOrWhiteSpace($cursor)) {
    $uri += "&cursor=$([Uri]::EscapeDataString($cursor))"
  }
  $page = Invoke-RestMethod -Method Get -Uri $uri -Headers $headers
  if (-not $page.success) { throw "Cloudflare returned success=false while listing KV keys." }
  foreach ($key in @($page.result)) { $keys.Add($key) }
  $pageCount++
  $cursor = [string]$page.result_info.cursor
} while (-not [string]::IsNullOrWhiteSpace($cursor))

$outputDirectory = Split-Path -Parent $OutputPath
[IO.Directory]::CreateDirectory($outputDirectory) | Out-Null
$tempPath = Join-Path $outputDirectory (".{0}.{1}.tmp" -f [IO.Path]::GetFileName($OutputPath), $PID)
$utf8 = [Text.UTF8Encoding]::new($false)
$writer = [IO.StreamWriter]::new($tempPath, $false, $utf8)
$client = [Net.Http.HttpClient]::new()
$client.DefaultRequestHeaders.Authorization = [Net.Http.Headers.AuthenticationHeaderValue]::new("Bearer", $token)
$retrieved = 0
$failed = 0

try {
  $manifest = [ordered]@{
    record_type = "export_manifest"
    format_version = 1
    namespace_binding = "AIFRED_SALES_LOG"
    conceptual_name = "AIFRED_ACTIVITY_LOG"
    namespace_id = $NamespaceId
    account_id = $AccountId
    snapshot_utc = [DateTime]::UtcNow.ToString("o")
    pagination = [ordered]@{ method = "cloudflare_cursor"; page_limit = 1000; page_count = $pageCount }
    kv_entry_count = $keys.Count
  }
  $writer.WriteLine(($manifest | ConvertTo-Json -Depth 20 -Compress))

  foreach ($key in $keys) {
    $encodedKey = [Uri]::EscapeDataString([string]$key.name)
    $response = $client.GetAsync("$apiBase/values/$encodedKey").GetAwaiter().GetResult()
    $bytes = $response.Content.ReadAsByteArrayAsync().GetAwaiter().GetResult()
    $valueBase64 = if ($response.IsSuccessStatusCode) { [Convert]::ToBase64String($bytes) } else { $null }
    $valueText = $null
    $encoding = "unretrievable"
    if ($response.IsSuccessStatusCode) {
      $candidate = [Text.Encoding]::UTF8.GetString($bytes)
      $roundTrip = [Text.Encoding]::UTF8.GetBytes($candidate)
      if ([Convert]::ToBase64String($roundTrip) -ceq $valueBase64) {
        $valueText = $candidate
        $encoding = "utf-8"
      } else {
        $encoding = "base64-only"
      }
      $retrieved++
    } else {
      $failed++
    }
    $entry = [ordered]@{
      record_type = "kv_entry"
      key = [string]$key.name
      expiration = $key.expiration
      metadata = $key.metadata
      http_status = [int]$response.StatusCode
      byte_length = if ($response.IsSuccessStatusCode) { $bytes.Length } else { $null }
      value_encoding = $encoding
      value_text = $valueText
      value_base64 = $valueBase64
    }
    $writer.WriteLine(($entry | ConvertTo-Json -Depth 50 -Compress))
  }
} finally {
  $client.Dispose()
  $writer.Dispose()
}

[IO.File]::Move($tempPath, $OutputPath, $true)
Write-Output "output=$OutputPath"
Write-Output "pages=$pageCount"
Write-Output "keys=$($keys.Count)"
Write-Output "retrieved=$retrieved"
Write-Output "failed=$failed"

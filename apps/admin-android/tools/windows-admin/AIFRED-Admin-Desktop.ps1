Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
$ErrorActionPreference = "Stop"
$script:BaseUrl = if ($env:AIFRED_API_BASE_URL) { $env:AIFRED_API_BASE_URL.TrimEnd('/') } else { "https://www.north3rnlight3r.com" }
$script:SessionToken = ""
$script:RepoRoot = if ($env:AIFRED_REPO_ROOT) { $env:AIFRED_REPO_ROOT } else { (Resolve-Path (Join-Path $PSScriptRoot "..\..\..\..")).Path }
$script:ArchiveTool = Join-Path $script:RepoRoot "tools\aifred-archive.mjs"

function Invoke-AifredApi([string]$Path, [string]$Method = "GET", [object]$Body = $null) {
    $headers = @{ Accept = "application/json" }; if ($script:SessionToken) { $headers.Authorization = "Bearer $script:SessionToken" }
    $parameters = @{ Uri = "$script:BaseUrl$Path"; Method = $Method; Headers = $headers; TimeoutSec = 45 }
    if ($null -ne $Body) { $parameters.ContentType = "application/json"; $parameters.Body = $Body | ConvertTo-Json -Depth 10 }
    Invoke-RestMethod @parameters
}
function Invoke-Archive([string[]]$Arguments) {
    if (-not (Test-Path $script:ArchiveTool)) { throw "Archive tool not found. Set AIFRED_REPO_ROOT to the AIFRED repository." }
    $result = & node $script:ArchiveTool @Arguments 2>&1 | Out-String
    if ($LASTEXITCODE -ne 0) { throw $result }; $result
}
function Save-AdminExport([string]$Kind) {
    if (-not $script:SessionToken) { throw "Online admin login is required." }
    $dialog = New-Object System.Windows.Forms.SaveFileDialog
    $dialog.Filter = "JSON files (*.json)|*.json"; $dialog.FileName = "aifred-$Kind-export-$((Get-Date).ToUniversalTime().ToString('yyyy-MM-ddTHH-mm-ssZ')).json"
    if ($dialog.ShowDialog() -eq "OK") { Invoke-WebRequest -Uri "$script:BaseUrl/api/v1/admin/export/$Kind" -Headers @{ Authorization = "Bearer $script:SessionToken" } -OutFile $dialog.FileName -UseBasicParsing; "Saved $($dialog.FileName)" }
}
function Format-Result($Value) { if ($Value -is [string]) { $Value } else { $Value | ConvertTo-Json -Depth 15 } }
function Run([scriptblock]$Action) { try { $output.Text = Format-Result (& $Action); $status.Text = "Operation completed at $((Get-Date).ToString('g'))." } catch { $output.Text = "ERROR: $($_.Exception.Message)"; $status.Text = "Operation failed." } }

$form = New-Object System.Windows.Forms.Form
$form.Text = "AIFRED Admin Desktop"; $form.Size = New-Object System.Drawing.Size(1120, 780); $form.StartPosition = "CenterScreen"; $form.BackColor = [Drawing.Color]::FromArgb(7,16,23)
$title = New-Object Windows.Forms.Label; $title.Text = "AIFRED Admin Desktop"; $title.ForeColor = [Drawing.Color]::FromArgb(232,243,255); $title.Font = New-Object Drawing.Font("Segoe UI",18,[Drawing.FontStyle]::Bold); $title.SetBounds(18,12,500,36); $form.Controls.Add($title)
$status = New-Object Windows.Forms.Label; $status.Text = "Live administration requires login. Local archives remain available offline."; $status.ForeColor = [Drawing.Color]::FromArgb(141,176,200); $status.SetBounds(22,50,900,22); $form.Controls.Add($status)
$user = New-Object Windows.Forms.TextBox; $user.Text = "North3rnLight3r"; $user.SetBounds(22,78,210,27); $form.Controls.Add($user)
$pass = New-Object Windows.Forms.TextBox; $pass.UseSystemPasswordChar = $true; $pass.SetBounds(242,78,220,27); $form.Controls.Add($pass)
$login = New-Object Windows.Forms.Button; $login.Text = "Online Login"; $login.SetBounds(475,76,125,31); $login.Add_Click({ Run { $result = Invoke-AifredApi "/api/v1/admin/login" "POST" @{ username=$user.Text; password=$pass.Text }; $script:SessionToken=$result.session_token; $pass.Text=""; $status.Text="Production session active."; $result | Select-Object ok,username,expires_at } }); $form.Controls.Add($login)
$openOps = New-Object Windows.Forms.Button; $openOps.Text = "Open /ops"; $openOps.SetBounds(610,76,110,31); $openOps.Add_Click({ Start-Process "$script:BaseUrl/ops" }); $form.Controls.Add($openOps)

$tabs = New-Object Windows.Forms.TabControl; $tabs.SetBounds(18,120,1065,455); $form.Controls.Add($tabs)
$definitions = [ordered]@{
    "Overview"=@(@("Dashboard",{Invoke-AifredApi "/api/v1/admin/dashboard/state"}),@("API Health",{Invoke-AifredApi "/api/v1/admin/ops/status"}))
    "Analytics"=@(@("Site Analytics",{Invoke-AifredApi "/api/v1/admin/dashboard/state"}),@("Session / Events",{Invoke-AifredApi "/api/v1/admin/logs/list?limit=300"}))
    "Downloads"=@(@("Download Activity",{ $r=Invoke-AifredApi "/api/v1/admin/logs/list?limit=300"; $r.logs | Where-Object { $_.event_type -match 'download' } }))
    "Track Analysis"=@(@("Analysis Records",{Invoke-AifredApi "/api/v1/admin/reference/list"}),@("Catalog",{Invoke-AifredApi "/api/v1/admin/catalog/list"}))
    "API"=@(@("Models",{Invoke-AifredApi "/v1/models"}),@("Route Status",{Invoke-AifredApi "/api/v1/admin/ops/status"}))
    "Logs"=@(@("Operational Logs",{Invoke-AifredApi "/api/v1/admin/logs/list?limit=300"}),@("Errors",{$r=Invoke-AifredApi "/api/v1/admin/logs/list?limit=300"; $r.logs | Where-Object { "$($_.event_type) $($_.message)" -match 'error|fail' }}))
    "Inquiries"=@(@("Load Inquiries",{Invoke-AifredApi "/api/v1/admin/inquiries/list"}))
    "Exports"=@(@("Export Site Data",{Save-AdminExport "site"}),@("Export Track Analysis",{Save-AdminExport "tracks"}))
    "FORGE"=@(@("Bridge Manifest",{Get-Content (Join-Path $script:RepoRoot "integrations\forge\manifest.json") -Raw}),@("Active Data Status",{Invoke-Archive @("status")} ),@("Archive Eligible FORGE Data",{Invoke-Archive @("rotate")}))
    "Archive"=@(@("Archive Status",{Invoke-Archive @("status")}),@("Archive Current Logs",{Invoke-Archive @("archive","--force")}),@("View Archives",{Invoke-Archive @("list")}),@("Verify Archives",{Invoke-Archive @("verify")}),@("Rebuild Archive Index",{Invoke-Archive @("rebuild-index")}),@("Search Archives",{Invoke-Archive @("search","--query",$search.Text,"--limit","100","--byte-limit","1048576")}),@("Restore to FORGE Workspace",{Invoke-Archive @("restore","--query",$search.Text,"--limit","100","--byte-limit","1048576")}),@("Prune Archive ID",{if([Windows.Forms.MessageBox]::Show("Permanently delete archive '$($search.Text)'?","Confirm archive deletion",[Windows.Forms.MessageBoxButtons]::YesNo,[Windows.Forms.MessageBoxIcon]::Warning)-eq "Yes"){Invoke-Archive @("prune","--id",$search.Text,"--confirm")}else{"Cancelled"}}))
}
foreach($entry in $definitions.GetEnumerator()) { $tab=New-Object Windows.Forms.TabPage; $tab.Text=$entry.Key; $tab.BackColor=[Drawing.Color]::FromArgb(13,27,37); $x=15;$y=18; foreach($item in $entry.Value){$button=New-Object Windows.Forms.Button;$button.Text=$item[0];$button.SetBounds($x,$y,195,34);$action=$item[1];$button.Add_Click({Run $action}.GetNewClosure());$tab.Controls.Add($button);$x+=205;if($x -gt 820){$x=15;$y+=45}};$tabs.TabPages.Add($tab)}
$searchLabel = New-Object Windows.Forms.Label; $searchLabel.Text = "Bounded archive search:"; $searchLabel.ForeColor = [Drawing.Color]::FromArgb(141,176,200); $searchLabel.SetBounds(22,584,165,24); $form.Controls.Add($searchLabel)
$search = New-Object Windows.Forms.TextBox; $search.SetBounds(190,582,360,27); $form.Controls.Add($search)
$output = New-Object Windows.Forms.TextBox; $output.Multiline=$true;$output.ScrollBars="Both";$output.WordWrap=$false;$output.ReadOnly=$true;$output.BackColor=[Drawing.Color]::FromArgb(3,8,12);$output.ForeColor=[Drawing.Color]::FromArgb(156,208,239);$output.Font=New-Object Drawing.Font("Consolas",10);$output.SetBounds(22,620,1060,105);$form.Controls.Add($output)
[void]$form.ShowDialog()

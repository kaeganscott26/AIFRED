Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

$ErrorActionPreference = "Stop"

$script:BaseUrl = "https://www.north3rnlight3r.com"
$script:SessionToken = ""
$script:OfflineMode = $false
$script:AdminUser = "North3rnLight3r"

function Invoke-AifredApi {
    param(
        [Parameter(Mandatory=$true)][string]$Path,
        [string]$Method = "GET",
        [object]$Body = $null
    )
    if ($script:OfflineMode -and $Path -notmatch "/admin/login$") {
        return @{
            ok = $true
            mode = "offline"
            message = "Desktop admin is unlocked offline. Live website data requires internet and online login."
            route = $Path
            items = @()
        } | ConvertTo-Json -Depth 8
    }

    $headers = @{}
    if ($script:SessionToken) {
        $headers.Authorization = "Bearer $script:SessionToken"
    }
    $uri = "$($script:BaseUrl.TrimEnd('/'))$Path"
    if ($Body -ne $null) {
        return Invoke-RestMethod -Uri $uri -Method $Method -Headers $headers -ContentType "application/json" -Body ($Body | ConvertTo-Json -Depth 8) | ConvertTo-Json -Depth 12
    }
    return Invoke-RestMethod -Uri $uri -Method $Method -Headers $headers | ConvertTo-Json -Depth 12
}

function Write-OutputBox {
    param([string]$Text)
    $output.Text = $Text
}

function Run-Action {
    param([scriptblock]$Action)
    try {
        Write-OutputBox (& $Action)
    } catch {
        Write-OutputBox ("ERROR: " + $_.Exception.Message)
    }
}

$form = New-Object System.Windows.Forms.Form
$form.Text = "AIFRED Admin Desktop"
$form.Size = New-Object System.Drawing.Size(980, 720)
$form.StartPosition = "CenterScreen"
$form.BackColor = [System.Drawing.Color]::FromArgb(7, 16, 23)

$title = New-Object System.Windows.Forms.Label
$title.Text = "AIFRED Admin Desktop"
$title.ForeColor = [System.Drawing.Color]::FromArgb(232, 243, 255)
$title.Font = New-Object System.Drawing.Font("Segoe UI", 18, [System.Drawing.FontStyle]::Bold)
$title.SetBounds(18, 14, 420, 34)
$form.Controls.Add($title)

$status = New-Object System.Windows.Forms.Label
$status.Text = "Offline-capable admin console. Online routes use www.north3rnlight3r.com."
$status.ForeColor = [System.Drawing.Color]::FromArgb(156, 208, 239)
$status.SetBounds(22, 52, 820, 22)
$form.Controls.Add($status)

$userLabel = New-Object System.Windows.Forms.Label
$userLabel.Text = "Username"
$userLabel.ForeColor = [System.Drawing.Color]::FromArgb(141, 176, 200)
$userLabel.SetBounds(22, 88, 80, 22)
$form.Controls.Add($userLabel)

$user = New-Object System.Windows.Forms.TextBox
$user.Text = $script:AdminUser
$user.SetBounds(110, 86, 210, 26)
$form.Controls.Add($user)

$passLabel = New-Object System.Windows.Forms.Label
$passLabel.Text = "Password"
$passLabel.ForeColor = [System.Drawing.Color]::FromArgb(141, 176, 200)
$passLabel.SetBounds(340, 88, 80, 22)
$form.Controls.Add($passLabel)

$pass = New-Object System.Windows.Forms.TextBox
$pass.UseSystemPasswordChar = $true
$pass.SetBounds(430, 86, 220, 26)
$form.Controls.Add($pass)

$login = New-Object System.Windows.Forms.Button
$login.Text = "Online Login"
$login.SetBounds(670, 84, 120, 30)
$login.Add_Click({
    Run-Action {
        $script:OfflineMode = $false
        $payload = @{ username = $user.Text; password = $pass.Text }
        $raw = Invoke-AifredApi -Path "/api/v1/admin/login" -Method "POST" -Body $payload
        $json = $raw | ConvertFrom-Json
        if ($json.ok) {
            $script:SessionToken = $json.session_token
            $status.Text = "Online admin session active as $($json.username)."
        }
        $raw
    }
})
$form.Controls.Add($login)

$offline = New-Object System.Windows.Forms.Button
$offline.Text = "Offline Login"
$offline.SetBounds(805, 84, 120, 30)
$offline.Add_Click({
    $script:OfflineMode = $true
    $script:SessionToken = "local-admin-desktop"
    $status.Text = "Offline desktop admin session active."
    Write-OutputBox "{`"ok`":true,`"mode`":`"offline`",`"message`":`"Desktop admin unlocked offline.`"}"
})
$form.Controls.Add($offline)

$buttons = @(
    @("Dashboard", { Invoke-AifredApi -Path "/api/v1/admin/dashboard/state" }),
    @("Sales Log", { Invoke-AifredApi -Path "/api/v1/admin/sales/list" }),
    @("Reference Pool", { Invoke-AifredApi -Path "/api/v1/admin/reference/list" }),
    @("Admin Log", { Invoke-AifredApi -Path "/api/v1/admin/logs/list?limit=300" }),
    @("Inquiries", { Invoke-AifredApi -Path "/api/v1/admin/inquiries/list" }),
    @("Catalog", { Invoke-AifredApi -Path "/api/v1/admin/catalog/list" }),
    @("Health", { Invoke-AifredApi -Path "/api/v1/health" }),
    @("Local Git Status", { git -C "C:\Users\North\Documents\Projects\aifred-site" status --short | Out-String })
)

$x = 22
$y = 128
for ($i = 0; $i -lt $buttons.Count; $i++) {
    $button = New-Object System.Windows.Forms.Button
    $button.Text = $buttons[$i][0]
    $button.SetBounds($x, $y, 140, 32)
    $action = $buttons[$i][1]
    $button.Add_Click({ Run-Action $action }.GetNewClosure())
    $form.Controls.Add($button)
    $x += 150
    if ($x -gt 780) { $x = 22; $y += 42 }
}

$cmdLabel = New-Object System.Windows.Forms.Label
$cmdLabel.Text = "Local command"
$cmdLabel.ForeColor = [System.Drawing.Color]::FromArgb(141, 176, 200)
$cmdLabel.SetBounds(22, 222, 120, 22)
$form.Controls.Add($cmdLabel)

$cmd = New-Object System.Windows.Forms.TextBox
$cmd.Text = "git -C C:\Users\North\Documents\Projects\aifred-site status --short"
$cmd.SetBounds(140, 220, 640, 26)
$form.Controls.Add($cmd)

$runCmd = New-Object System.Windows.Forms.Button
$runCmd.Text = "Run Local"
$runCmd.SetBounds(800, 218, 125, 30)
$runCmd.Add_Click({
    Run-Action {
        powershell -NoProfile -ExecutionPolicy Bypass -Command $cmd.Text | Out-String
    }
})
$form.Controls.Add($runCmd)

$output = New-Object System.Windows.Forms.TextBox
$output.Multiline = $true
$output.ScrollBars = "Both"
$output.WordWrap = $false
$output.BackColor = [System.Drawing.Color]::FromArgb(3, 8, 12)
$output.ForeColor = [System.Drawing.Color]::FromArgb(156, 208, 239)
$output.Font = New-Object System.Drawing.Font("Consolas", 10)
$output.SetBounds(22, 262, 905, 380)
$form.Controls.Add($output)

[void]$form.ShowDialog()

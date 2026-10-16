Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

$ErrorActionPreference = 'Stop'

function New-Shortcut {
    param(
        [Parameter(Mandatory = $true)][string]$ShortcutPath,
        [Parameter(Mandatory = $true)][string]$TargetPath,
        [string]$Arguments = '',
        [string]$WorkingDirectory = ''
    )

    $shell = New-Object -ComObject WScript.Shell
    $shortcut = $shell.CreateShortcut($ShortcutPath)
    $shortcut.TargetPath = $TargetPath
    $shortcut.Arguments = $Arguments
    if ($WorkingDirectory) {
        $shortcut.WorkingDirectory = $WorkingDirectory
    }
    $shortcut.IconLocation = "$TargetPath,0"
    $shortcut.Save()
}

$form = New-Object System.Windows.Forms.Form
$form.Text = 'Install Expense Tracker'
$form.StartPosition = 'CenterScreen'
$form.FormBorderStyle = 'FixedDialog'
$form.MaximizeBox = $false
$form.MinimizeBox = $false
$form.ClientSize = New-Object System.Drawing.Size(510, 205)
$form.Font = New-Object System.Drawing.Font('Segoe UI', 9)

$title = New-Object System.Windows.Forms.Label
$title.Text = 'Install Expense Tracker'
$title.Font = New-Object System.Drawing.Font('Segoe UI', 15, [System.Drawing.FontStyle]::Bold)
$title.Location = New-Object System.Drawing.Point(20, 15)
$title.Size = New-Object System.Drawing.Size(450, 32)
$form.Controls.Add($title)

$description = New-Object System.Windows.Forms.Label
$description.Text = 'Choose where to install the app. Your accounts and transactions are kept separately.'
$description.Location = New-Object System.Drawing.Point(22, 54)
$description.Size = New-Object System.Drawing.Size(465, 34)
$form.Controls.Add($description)

$pathBox = New-Object System.Windows.Forms.TextBox
$pathBox.Location = New-Object System.Drawing.Point(23, 94)
$pathBox.Size = New-Object System.Drawing.Size(365, 25)
$pathBox.Text = Join-Path $env:LOCALAPPDATA 'Programs\Expense Tracker'
$form.Controls.Add($pathBox)

$browseButton = New-Object System.Windows.Forms.Button
$browseButton.Text = 'Browse...'
$browseButton.Location = New-Object System.Drawing.Point(398, 92)
$browseButton.Size = New-Object System.Drawing.Size(88, 28)
$browseButton.Add_Click({
    $picker = New-Object System.Windows.Forms.FolderBrowserDialog
    $picker.Description = 'Choose the installation folder'
    $picker.SelectedPath = $pathBox.Text
    if ($picker.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK) {
        $pathBox.Text = $picker.SelectedPath
    }
})
$form.Controls.Add($browseButton)

$desktopCheck = New-Object System.Windows.Forms.CheckBox
$desktopCheck.Text = 'Create a desktop shortcut'
$desktopCheck.Location = New-Object System.Drawing.Point(23, 132)
$desktopCheck.Size = New-Object System.Drawing.Size(240, 24)
$desktopCheck.Checked = $true
$form.Controls.Add($desktopCheck)

$installButton = New-Object System.Windows.Forms.Button
$installButton.Text = 'Install'
$installButton.Location = New-Object System.Drawing.Point(316, 163)
$installButton.Size = New-Object System.Drawing.Size(82, 29)
$installButton.DialogResult = [System.Windows.Forms.DialogResult]::OK
$form.Controls.Add($installButton)

$cancelButton = New-Object System.Windows.Forms.Button
$cancelButton.Text = 'Cancel'
$cancelButton.Location = New-Object System.Drawing.Point(405, 163)
$cancelButton.Size = New-Object System.Drawing.Size(82, 29)
$cancelButton.DialogResult = [System.Windows.Forms.DialogResult]::Cancel
$form.Controls.Add($cancelButton)
$form.AcceptButton = $installButton
$form.CancelButton = $cancelButton

if ($form.ShowDialog() -ne [System.Windows.Forms.DialogResult]::OK) {
    exit 0
}

$installPath = [Environment]::ExpandEnvironmentVariables($pathBox.Text.Trim())
if ([string]::IsNullOrWhiteSpace($installPath)) {
    [System.Windows.Forms.MessageBox]::Show(
        'Choose an installation folder.',
        'Expense Tracker',
        [System.Windows.Forms.MessageBoxButtons]::OK,
        [System.Windows.Forms.MessageBoxIcon]::Warning) | Out-Null
    exit 1
}
$installPath = [IO.Path]::GetFullPath($installPath)
if ($installPath.TrimEnd('\') -eq [IO.Path]::GetPathRoot($installPath).TrimEnd('\')) {
    [System.Windows.Forms.MessageBox]::Show(
        'Choose a folder below a drive root.',
        'Expense Tracker',
        [System.Windows.Forms.MessageBoxButtons]::OK,
        [System.Windows.Forms.MessageBoxIcon]::Warning) | Out-Null
    exit 1
}

$payloadZip = Join-Path $PSScriptRoot 'ExpenseTracker-Payload.zip'
$temporaryPath = Join-Path $env:TEMP ('ExpenseTracker-Install-' + [guid]::NewGuid().ToString('N'))
$manifestPath = Join-Path $installPath 'ExpenseTracker.install-manifest'
$progress = New-Object System.Windows.Forms.Form
$progress.Text = 'Installing Expense Tracker'
$progress.StartPosition = 'CenterScreen'
$progress.FormBorderStyle = 'FixedDialog'
$progress.ControlBox = $false
$progress.ClientSize = New-Object System.Drawing.Size(380, 85)
$progressLabel = New-Object System.Windows.Forms.Label
$progressLabel.Text = 'Installing files...'
$progressLabel.Location = New-Object System.Drawing.Point(18, 15)
$progressLabel.Size = New-Object System.Drawing.Size(340, 22)
$progress.Controls.Add($progressLabel)
$progressBar = New-Object System.Windows.Forms.ProgressBar
$progressBar.Style = 'Marquee'
$progressBar.Location = New-Object System.Drawing.Point(20, 45)
$progressBar.Size = New-Object System.Drawing.Size(340, 20)
$progress.Controls.Add($progressBar)
$progress.Show()
$progress.Refresh()

try {
    New-Item -ItemType Directory -Path $temporaryPath -Force | Out-Null
    Expand-Archive -LiteralPath $payloadZip -DestinationPath $temporaryPath -Force
    New-Item -ItemType Directory -Path $installPath -Force | Out-Null

    $ownedFiles = @{}
    if (Test-Path -LiteralPath $manifestPath -PathType Leaf) {
        foreach ($relativePath in Get-Content -LiteralPath $manifestPath) {
            if (-not [string]::IsNullOrWhiteSpace($relativePath)) {
                $ownedFiles[$relativePath] = $true
            }
        }
    }
    Get-ChildItem -LiteralPath $temporaryPath -Recurse -File | ForEach-Object {
        $relativePath = $_.FullName.Substring($temporaryPath.Length).TrimStart('\')
        $ownedFiles[$relativePath] = $true
    }
    Copy-Item -Path (Join-Path $temporaryPath '*') -Destination $installPath -Recurse -Force
    $ownedFiles.Keys | Set-Content -LiteralPath $manifestPath -Encoding UTF8

    $startMenu = Join-Path $env:APPDATA 'Microsoft\Windows\Start Menu\Programs\Expense Tracker'
    New-Item -ItemType Directory -Path $startMenu -Force | Out-Null
    $executable = Join-Path $installPath 'ExpenseTracker.exe'
    New-Shortcut -ShortcutPath (Join-Path $startMenu 'Expense Tracker.lnk') `
        -TargetPath $executable -WorkingDirectory $installPath
    New-Shortcut -ShortcutPath (Join-Path $startMenu 'Uninstall Expense Tracker.lnk') `
        -TargetPath (Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe') `
        -Arguments "-NoProfile -File `"$(Join-Path $installPath 'Uninstall-ExpenseTracker.ps1')`"" `
        -WorkingDirectory $installPath

    if ($desktopCheck.Checked) {
        $desktop = [Environment]::GetFolderPath([Environment+SpecialFolder]::DesktopDirectory)
        New-Shortcut -ShortcutPath (Join-Path $desktop 'Expense Tracker.lnk') `
            -TargetPath $executable -WorkingDirectory $installPath
    }
}
catch {
    $progress.Close()
    [System.Windows.Forms.MessageBox]::Show(
        "Installation failed:`r`n$($_.Exception.Message)",
        'Expense Tracker',
        [System.Windows.Forms.MessageBoxButtons]::OK,
        [System.Windows.Forms.MessageBoxIcon]::Error) | Out-Null
    Remove-Item -LiteralPath $temporaryPath -Recurse -Force -ErrorAction SilentlyContinue
    exit 1
}
finally {
    if (-not $progress.IsDisposed) {
        $progress.Close()
    }
}

Remove-Item -LiteralPath $temporaryPath -Recurse -Force -ErrorAction SilentlyContinue
$launch = [System.Windows.Forms.MessageBox]::Show(
    'Expense Tracker was installed successfully. Start it now?',
    'Expense Tracker',
    [System.Windows.Forms.MessageBoxButtons]::YesNo,
    [System.Windows.Forms.MessageBoxIcon]::Information)
if ($launch -eq [System.Windows.Forms.DialogResult]::Yes) {
    Start-Process -FilePath (Join-Path $installPath 'ExpenseTracker.exe') `
        -WorkingDirectory $installPath
}

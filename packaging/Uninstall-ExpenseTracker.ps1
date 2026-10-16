Add-Type -AssemblyName System.Windows.Forms

$ErrorActionPreference = 'Stop'

$installPath = [IO.Path]::GetFullPath((Split-Path -Parent $MyInvocation.MyCommand.Path))
$installRoot = $installPath.TrimEnd([char[]]@('\', '/')) + '\'
$manifestPath = Join-Path $installPath 'ExpenseTracker.install-manifest'
$startMenu = Join-Path $env:APPDATA 'Microsoft\Windows\Start Menu\Programs\Expense Tracker'
$desktopShortcut = Join-Path `
    ([Environment]::GetFolderPath([Environment+SpecialFolder]::DesktopDirectory)) `
    'Expense Tracker.lnk'
$answer = [System.Windows.Forms.MessageBox]::Show(
    'Remove Expense Tracker? Your accounts and transaction data will be kept.',
    'Uninstall Expense Tracker',
    [System.Windows.Forms.MessageBoxButtons]::YesNo,
    [System.Windows.Forms.MessageBoxIcon]::Question)
if ($answer -ne [System.Windows.Forms.DialogResult]::Yes) {
    exit 0
}

if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
    [System.Windows.Forms.MessageBox]::Show(
        'The installation manifest is missing. No files were removed.',
        'Uninstall Expense Tracker',
        [System.Windows.Forms.MessageBoxButtons]::OK,
        [System.Windows.Forms.MessageBoxIcon]::Error) | Out-Null
    exit 1
}

try {
    $installItem = Get-Item -LiteralPath $installPath -Force
    $manifestItem = Get-Item -LiteralPath $manifestPath -Force
    if (($installItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0 -or
        ($manifestItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw 'The installation folder or manifest is a reparse point. No files were removed.'
    }
    if (Test-Path -LiteralPath $startMenu) {
        $startMenuItem = Get-Item -LiteralPath $startMenu -Force
        if (($startMenuItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw 'The Expense Tracker Start Menu folder is a reparse point. No files were removed.'
        }
    }

    $ownedFiles = @(Get-Content -LiteralPath $manifestPath)
    $filesToRemove = @()
    foreach ($relativePath in $ownedFiles) {
        if ([string]::IsNullOrWhiteSpace($relativePath)) {
            continue
        }

        $segments = $relativePath -split '[\\/]'
        $unsafeSegments = @($segments | Where-Object {
            $_ -in @('', '.', '..') -or
            $_ -match '[<>:"|?*\x00-\x1f]' -or
            $_ -match '[. ]$'
        })
        if ([IO.Path]::IsPathRooted($relativePath) -or
            $segments.Count -eq 0 -or
            $unsafeSegments.Count -gt 0) {
            throw "The installation manifest contains an unsafe path: $relativePath"
        }

        if ($relativePath -match '(?i)\.db(?:-(?:journal|wal|shm))?$') {
            continue
        }

        $filePath = [IO.Path]::GetFullPath((Join-Path $installPath $relativePath))
        if (-not $filePath.StartsWith($installRoot, [StringComparison]::OrdinalIgnoreCase)) {
            throw "The installation manifest contains a path outside the installation folder: $relativePath"
        }

        $currentPath = $installPath
        for ($index = 0; $index -lt $segments.Count; $index++) {
            $currentPath = Join-Path $currentPath $segments[$index]
            if (Test-Path -LiteralPath $currentPath) {
                $pathItem = Get-Item -LiteralPath $currentPath -Force
                if (($pathItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                    throw "The installation contains a reparse point: $currentPath"
                }
                if ($index -lt ($segments.Count - 1) -and -not $pathItem.PSIsContainer) {
                    throw "The installation manifest path is not a file path: $relativePath"
                }
            }
        }

        if (Test-Path -LiteralPath $filePath -PathType Leaf) {
            $filesToRemove += $filePath
        }
    }

    foreach ($filePath in $filesToRemove) {
        Remove-Item -LiteralPath $filePath -Force
    }
    Remove-Item -LiteralPath $manifestPath -Force

    if (Test-Path -LiteralPath $startMenu) {
        foreach ($shortcutName in @(
                'Expense Tracker.lnk',
                'Uninstall Expense Tracker.lnk'
            )) {
            $shortcutPath = Join-Path $startMenu $shortcutName
            if (Test-Path -LiteralPath $shortcutPath -PathType Leaf) {
                Remove-Item -LiteralPath $shortcutPath -Force
            }
        }
        if (-not (Get-ChildItem -LiteralPath $startMenu -Force | Select-Object -First 1)) {
            Remove-Item -LiteralPath $startMenu -Force
        }
    }
    if (Test-Path -LiteralPath $desktopShortcut) {
        Remove-Item -LiteralPath $desktopShortcut -Force
    }
}
catch {
    [System.Windows.Forms.MessageBox]::Show(
        "Uninstallation failed:`r`n$($_.Exception.Message)",
        'Uninstall Expense Tracker',
        [System.Windows.Forms.MessageBoxButtons]::OK,
        [System.Windows.Forms.MessageBoxIcon]::Error) | Out-Null
    exit 1
}

[System.Windows.Forms.MessageBox]::Show(
    'Expense Tracker was removed. Your accounts and transaction data were kept.',
    'Uninstall Expense Tracker',
    [System.Windows.Forms.MessageBoxButtons]::OK,
    [System.Windows.Forms.MessageBoxIcon]::Information) | Out-Null

param(
    [string]$QtBin = 'C:\Qt\6.12.0\mingw_64\bin',
    [string]$MinGWBin = 'C:\Qt\Tools\mingw1310_64\bin'
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$buildDirectory = Join-Path $root 'build-installer'
$stageDirectory = Join-Path $root 'installer-stage'
$payloadDirectory = Join-Path $stageDirectory 'payload'
$outputDirectory = Join-Path $root 'dist'
$payloadZip = Join-Path $stageDirectory 'ExpenseTracker-Payload.zip'
$installerScript = Join-Path $PSScriptRoot 'Install-ExpenseTracker.ps1'
$uninstallerScript = Join-Path $PSScriptRoot 'Uninstall-ExpenseTracker.ps1'
$qtDeploy = Join-Path $QtBin 'windeployqt.exe'
$qtPrefix = Split-Path -Parent $QtBin
$makeProgram = Join-Path $MinGWBin 'mingw32-make.exe'
$iexpress = Join-Path $env:SystemRoot 'System32\iexpress.exe'

foreach ($requiredPath in @(
        $qtDeploy,
        (Join-Path $qtPrefix 'lib\cmake\Qt6\Qt6Config.cmake'),
        $makeProgram,
        (Join-Path $MinGWBin 'g++.exe'),
        $iexpress,
        $installerScript,
        $uninstallerScript
    )) {
    if (-not (Test-Path -LiteralPath $requiredPath -PathType Leaf)) {
        throw "Required packaging file or tool was not found: $requiredPath"
    }
}

New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
if (Test-Path -LiteralPath $stageDirectory) {
    Remove-Item -LiteralPath $stageDirectory -Recurse -Force
}
if (Test-Path -LiteralPath $buildDirectory) {
    Remove-Item -LiteralPath $buildDirectory -Recurse -Force
}
New-Item -ItemType Directory -Path $payloadDirectory -Force | Out-Null

$env:Path = "$MinGWBin;$QtBin;$env:Path"
cmake -G 'MinGW Makefiles' -S $root -B $buildDirectory `
    "-DCMAKE_MAKE_PROGRAM=$makeProgram" `
    "-DCMAKE_PREFIX_PATH=$qtPrefix" `
    -DCMAKE_BUILD_TYPE=Release `
    -DBUILD_TESTING=OFF
if ($LASTEXITCODE -ne 0) {
    throw 'CMake configuration failed.'
}
cmake --build $buildDirectory --config Release --target ExpenseTracker --parallel
if ($LASTEXITCODE -ne 0) {
    throw 'Expense Tracker release build failed.'
}

Copy-Item -LiteralPath (Join-Path $buildDirectory 'ExpenseTracker.exe') `
    -Destination $payloadDirectory
$translationDirectory = Join-Path $payloadDirectory 'translations'
New-Item -ItemType Directory -Path $translationDirectory -Force | Out-Null
foreach ($language in @('en', 'it')) {
    $catalog = Join-Path $buildDirectory "translations\ExpenseTracker_$language.qm"
    if (-not (Test-Path -LiteralPath $catalog -PathType Leaf)) {
        throw "Required translation catalog was not built: $catalog"
    }
    Copy-Item -LiteralPath $catalog -Destination $translationDirectory
}

& $qtDeploy --release --no-translations --compiler-runtime `
    (Join-Path $payloadDirectory 'ExpenseTracker.exe')
if ($LASTEXITCODE -ne 0) {
    throw 'Qt runtime deployment failed.'
}

foreach ($runtimeDll in @('libgcc_s_seh-1.dll', 'libstdc++-6.dll', 'libwinpthread-1.dll')) {
    $runtimePath = Join-Path $MinGWBin $runtimeDll
    if (-not (Test-Path -LiteralPath $runtimePath -PathType Leaf)) {
        throw "Required MinGW runtime was not found: $runtimePath"
    }
    Copy-Item -LiteralPath $runtimePath -Destination $payloadDirectory -Force
}

Copy-Item -LiteralPath $uninstallerScript -Destination $payloadDirectory
Compress-Archive -Path (Join-Path $payloadDirectory '*') `
    -DestinationPath $payloadZip -CompressionLevel Optimal

$setupScript = Join-Path $stageDirectory 'Install-ExpenseTracker.ps1'
Copy-Item -LiteralPath $installerScript -Destination $setupScript
$sedPath = Join-Path $stageDirectory 'ExpenseTracker-Setup.sed'
$stagedInstaller = Join-Path $stageDirectory 'ExpenseTracker-Setup.exe'
$installerPath = Join-Path $outputDirectory 'ExpenseTracker-Setup.exe'
$sedLines = @(
    '[Version]',
    'Class=IEXPRESS',
    'SEDVersion=3',
    '[Options]',
    'PackagePurpose=InstallApp',
    'ShowInstallProgramWindow=1',
    'HideExtractAnimation=1',
    'UseLongFileName=1',
    'InsideCompressed=0',
    'CAB_FixedSize=0',
    'CAB_ResvCodeSigning=0',
    'RebootMode=N',
    'InstallPrompt=%InstallPrompt%',
    'DisplayLicense=%DisplayLicense%',
    'FinishMessage=%FinishMessage%',
    'TargetName=%TargetName%',
    'FriendlyName=%FriendlyName%',
    'AppLaunched=%AppLaunched%',
    'PostInstallCmd=%PostInstallCmd%',
    'AdminQuietInstCmd=%AdminQuietInstCmd%',
    'UserQuietInstCmd=%UserQuietInstCmd%',
    'SourceFiles=SourceFiles',
    '[Strings]',
    "InstallPrompt=Install Expense Tracker for your Windows account?",
    'DisplayLicense=',
    'FinishMessage=Expense Tracker setup has finished.',
    'TargetName=ExpenseTracker-Setup.exe',
    'FriendlyName=Expense Tracker Setup',
    'AppLaunched=powershell.exe -NoProfile -File Install-ExpenseTracker.ps1',
    'PostInstallCmd=<None>',
    'AdminQuietInstCmd=',
    'UserQuietInstCmd=',
    'FILE0="ExpenseTracker-Payload.zip"',
    'FILE1="Install-ExpenseTracker.ps1"',
    '[SourceFiles]',
    'SourceFiles0=.',
    '[SourceFiles0]',
    '%FILE0%=',
    '%FILE1%='
)
Set-Content -LiteralPath $sedPath -Value $sedLines -Encoding ASCII
Push-Location $stageDirectory
try {
    & $iexpress /N /Q 'ExpenseTracker-Setup.sed'
    $iexpressExitCode = $LASTEXITCODE
}
finally {
    Pop-Location
}
if ($iexpressExitCode -ne 0 -or
    -not (Test-Path -LiteralPath $stagedInstaller -PathType Leaf)) {
    throw 'IExpress failed to create the installer executable.'
}
Copy-Item -LiteralPath $stagedInstaller -Destination $installerPath -Force

Write-Host "Installer created: $installerPath"
Write-Host ("Size: {0:N1} MB" -f ((Get-Item -LiteralPath $installerPath).Length / 1MB))

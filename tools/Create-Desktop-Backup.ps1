param(
    [string]$SourceDir = "",
    [string]$BackupRoot = ""
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($SourceDir)) {
    $SourceDir = Split-Path -Parent $PSScriptRoot
}

if ([string]::IsNullOrWhiteSpace($BackupRoot)) {
    $desktop = Join-Path $env:USERPROFILE "Desktop"
    $BackupRoot = Join-Path $desktop "FluffyFiveM_Permanent_Kit"
}

$packageDir = Join-Path $BackupRoot "CurrentPackage"
$scriptsDir = Join-Path $BackupRoot "Scripts"
$archiveDir = Join-Path $BackupRoot "Archives"
$stamp = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"
$zipOut = Join-Path $archiveDir ("snapshot_" + $stamp + ".zip")

New-Item -ItemType Directory -Force -Path $BackupRoot, $packageDir, $scriptsDir, $archiveDir | Out-Null

$copyIfExists = @(
    "Nova.exe",
    "Fluffy-FiveM.exe",
    "fluffyFiveM.exe",
    "D3DCompiler_43.dll",
    "d3dx9_43.dll",
    "d3dx11_43.dll",
    "d3dx10_43.dll",
    "APP_VERSION",
    "README.md",
    ".github\workflows\publish-release.yml",
    "tools\Update-FluffyFiveM.ps1",
    "tools\Ensure-LatestFluffy.ps1"
)

foreach ($item in $copyIfExists) {
    $src = Join-Path $SourceDir $item
    if (Test-Path $src) {
        $dest = Join-Path $packageDir $item
        $destParent = Split-Path -Parent $dest
        if (-not (Test-Path $destParent)) {
            New-Item -ItemType Directory -Force -Path $destParent | Out-Null
        }
        Copy-Item -Path $src -Destination $dest -Force
    }
}

Copy-Item -Path (Join-Path $SourceDir "tools\Update-FluffyFiveM.ps1") -Destination (Join-Path $scriptsDir "Update-FluffyFiveM.ps1") -Force
Copy-Item -Path (Join-Path $SourceDir "tools\Ensure-LatestFluffy.ps1") -Destination (Join-Path $scriptsDir "Ensure-LatestFluffy.ps1") -Force
Copy-Item -Path (Join-Path $SourceDir "tools\Create-Desktop-Backup.ps1") -Destination (Join-Path $scriptsDir "Create-Desktop-Backup.ps1") -Force

if (Test-Path $zipOut) {
    Remove-Item $zipOut -Force
}
Compress-Archive -Path (Join-Path $packageDir "*") -DestinationPath $zipOut -Force

Write-Host "Backup complete."
Write-Host "Folder: $BackupRoot"
Write-Host "Archive: $zipOut"

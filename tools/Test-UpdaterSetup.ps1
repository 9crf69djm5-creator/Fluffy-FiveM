$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$required = @(
    ".github\workflows\publish-release.yml",
    "tools\Update-FluffyFiveM.ps1",
    "tools\Ensure-LatestFluffy.ps1",
    "tools\Create-Desktop-Backup.ps1",
    "APP_VERSION"
)

$missing = @()
foreach ($rel in $required) {
    $p = Join-Path $root $rel
    if (-not (Test-Path $p)) {
        $missing += $rel
    }
}

if ($missing.Count -gt 0) {
    Write-Error ("Missing required files: " + ($missing -join ", "))
    exit 1
}

$version = (Get-Content (Join-Path $root "APP_VERSION") -Raw).Trim()
if ([string]::IsNullOrWhiteSpace($version)) {
    Write-Error "APP_VERSION is empty."
    exit 1
}

Write-Host "Updater setup looks good."
Write-Host "APP_VERSION: $version"
Write-Host "Workflow + scripts present."

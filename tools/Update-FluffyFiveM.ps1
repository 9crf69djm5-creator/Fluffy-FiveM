param(
    [string]$Repo = "9crf69djm5-creator/Fluffy-FiveM",
    [string]$InstallDir = "",
    [switch]$IncludePrerelease
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($InstallDir)) {
    $InstallDir = Split-Path -Parent $PSScriptRoot
}

if (-not (Test-Path $InstallDir)) {
    New-Item -ItemType Directory -Path $InstallDir | Out-Null
}

$apiUrl = if ($IncludePrerelease) {
    "https://api.github.com/repos/$Repo/releases"
} else {
    "https://api.github.com/repos/$Repo/releases/latest"
}

Write-Host "Checking releases from $Repo ..."
$headers = @{ "User-Agent" = "FluffyFiveM-Updater" }
$releaseInfo = Invoke-RestMethod -Uri $apiUrl -Headers $headers
if ($IncludePrerelease) {
    $releaseInfo = $releaseInfo | Select-Object -First 1
}

if (-not $releaseInfo) {
    throw "No release information found."
}

$tag = $releaseInfo.tag_name
if ([string]::IsNullOrWhiteSpace($tag)) {
    throw "Release tag name missing."
}

Write-Host "Latest tag: $tag"

$wanted = @(
    "Fluffy-FiveM.exe",
    "fluffyFiveM.exe",
    "D3DCompiler_43.dll",
    "d3dx9_43.dll",
    "d3dx11_43.dll",
    "d3dx10_43.dll"
)

$assets = @($releaseInfo.assets)
if ($assets.Count -eq 0) {
    throw "Release has no assets."
}

$downloaded = @()
foreach ($name in $wanted) {
    $asset = $assets | Where-Object { $_.name -eq $name } | Select-Object -First 1
    if (-not $asset) {
        continue
    }
    $targetPath = Join-Path $InstallDir $asset.name
    Write-Host "Downloading $($asset.name) ..."
    Invoke-WebRequest -Uri $asset.browser_download_url -Headers $headers -OutFile $targetPath
    $downloaded += $asset.name
}

if ($downloaded.Count -eq 0) {
    throw "No matching assets found in release $tag."
}

$versionPath = Join-Path $InstallDir "APP_VERSION"
$plainTag = $tag.TrimStart("v")
Set-Content -Path $versionPath -Value $plainTag -Encoding ASCII

Write-Host ""
Write-Host "Update complete."
Write-Host "Installed to: $InstallDir"
Write-Host "Version: $plainTag"
Write-Host "Files: $($downloaded -join ', ')"

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

$exeOrder = @("Nova.exe", "Fluffy-FiveM.exe", "fluffyFiveM.exe")
$dllOrder = @("D3DCompiler_43.dll", "d3dx9_43.dll", "d3dx11_43.dll", "d3dx10_43.dll")

$assets = @($releaseInfo.assets)
if ($assets.Count -eq 0) {
    throw "Release has no assets."
}

function Download-OneAsset {
    param([string]$name)
    $asset = $assets | Where-Object { $_.name -eq $name } | Select-Object -First 1
    if (-not $asset) { return $false }
    $targetPath = Join-Path $InstallDir $asset.name
    Write-Host "Downloading $($asset.name) ..."
    Invoke-WebRequest -Uri $asset.browser_download_url -Headers $headers -OutFile $targetPath
    return $true
}

$downloaded = @()
$gotExe = $false
foreach ($name in $exeOrder) {
    if (Download-OneAsset $name) {
        $downloaded += $name
        $gotExe = $true
        break
    }
}

foreach ($name in $dllOrder) {
    if (Download-OneAsset $name) {
        $downloaded += $name
    }
}

if (-not $gotExe) {
    throw "No executable asset found in release $tag (expected Nova.exe or Fluffy-FiveM.exe)."
}

$versionPath = Join-Path $InstallDir "APP_VERSION"
$plainTag = $tag.TrimStart("v")
Set-Content -Path $versionPath -Value $plainTag -Encoding ASCII

Write-Host ""
Write-Host "Update complete."
Write-Host "Installed to: $InstallDir"
Write-Host "Version: $plainTag"
Write-Host "Files: $($downloaded -join ', ')"

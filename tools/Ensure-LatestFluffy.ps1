# Compares local APP_VERSION to GitHub stable "latest". Downloads only when remote semver is greater.
# Align KeyAuth application version with APP_VERSION when shipping to avoid auth/log spam.
#
param(
    [string]$Repo = "9crf69djm5-creator/Fluffy-FiveM",
    [Parameter(Mandatory = $false)]
    [string]$RunDir = "",
    [switch]$ForceDownload
)

$ErrorActionPreference = "Stop"

function Get-SemVerParts([string]$v) {
    $s = $v.Trim().TrimStart("v")
    $parts = $s -split "\."
    $a = 0; $b = 0; $c = 0
    if ($parts.Length -ge 1) { [void][int]::TryParse(($parts[0] -replace "[^\d].*$", ""), [ref]$a) }
    if ($parts.Length -ge 2) { [void][int]::TryParse(($parts[1] -replace "[^\d].*$", ""), [ref]$b) }
    if ($parts.Length -ge 3) { [void][int]::TryParse(($parts[2] -replace "[^\d].*$", ""), [ref]$c) }
    return @( $a, $b, $c )
}

function Test-RemoteVersionNewer([string]$remote, [string]$local) {
    $r = Get-SemVerParts $remote
    $l = Get-SemVerParts $local
    for ($i = 0; $i -lt 3; $i++) {
        if ($r[$i] -gt $l[$i]) { return $true }
        if ($r[$i] -lt $l[$i]) { return $false }
    }
    return $false
}

if ([string]::IsNullOrWhiteSpace($RunDir)) {
    $RunDir = Split-Path -Parent $PSScriptRoot
}

function Get-InstalledExePath([string]$dir) {
    foreach ($n in @("Fluffy-FiveM.exe", "Nova.exe", "fluffyFiveM.exe")) {
        $p = Join-Path $dir $n
        if (Test-Path $p) { return $p }
    }
    return $null
}

$exePath = Get-InstalledExePath $RunDir
$verPath = Join-Path $RunDir "APP_VERSION"
$updater = Join-Path $PSScriptRoot "Update-FluffyFiveM.ps1"

if (-not (Test-Path $RunDir)) {
    New-Item -ItemType Directory -Force -Path $RunDir | Out-Null
}

$localVer = "0.0.0"
if (Test-Path $verPath) {
    $localVer = (Get-Content -Raw -Path $verPath).Trim()
    if ([string]::IsNullOrWhiteSpace($localVer)) { $localVer = "0.0.0" }
}

$needExe = [string]::IsNullOrWhiteSpace($exePath)
$needUpdate = $ForceDownload.IsPresent

if (-not $needExe -and -not $needUpdate) {
    $apiUrl = "https://api.github.com/repos/$Repo/releases/latest"
    $headers = @{ "User-Agent" = "FluffyFiveM-EnsureLatest" }
    try {
        $release = Invoke-RestMethod -Uri $apiUrl -Headers $headers -TimeoutSec 25
    }
    catch {
        Write-Warning "Could not reach GitHub ($($_.Exception.Message)). Using installed build without update check."
        return
    }

    if ($release.prerelease -eq $true) {
        Write-Warning "GitHub 'latest' is prerelease; skipping auto-download."
        return
    }

    $tag = $release.tag_name
    if ([string]::IsNullOrWhiteSpace($tag)) {
        Write-Warning "Release has no tag_name; skipping compare."
        return
    }

    $remotePlain = $tag.TrimStart("v")
    if (Test-RemoteVersionNewer $remotePlain $localVer) {
        Write-Host "Newer stable release: v$remotePlain (installed $localVer). Downloading..."
        $needUpdate = $true
    }
    else {
        Write-Host "Already on latest stable ($localVer)."
    }
}

if ($needExe) {
    Write-Host "Executable missing. Downloading latest stable..."
    & $updater -InstallDir $RunDir
}
elseif ($needUpdate) {
    & $updater -InstallDir $RunDir
}

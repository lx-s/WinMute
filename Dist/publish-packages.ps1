<#
.SYNOPSIS
    Packages the already-published, already-signed GitHub Release assets for
    Chocolatey and winget, and walks you through pushing/submitting them.

.DESCRIPTION
    Run this after .github/workflows/release.yml has finished and published a
    GitHub Release for the given tag. It does not build or sign anything
    itself - it downloads the signed WinMute.exe from that release, stages
    Dist\bin, packs the Chocolatey nupkg, and prints/prompts for the
    `choco push` and `wingetcreate update --submit` commands.

    Requires: gh CLI (authenticated), choco CLI (with an API key already
    configured via `choco apikey add`), wingetcreate.exe on PATH (with a
    token already stored via `wingetcreate token --store`).

.PARAMETER Version
    The release tag to package, e.g. "2.6.1.0".

.EXAMPLE
    .\Dist\publish-packages.ps1 -Version 2.6.1.0
#>
param(
    [Parameter(Mandatory = $true)]
    [string]$Version
)

$ErrorActionPreference = 'Stop'

function Invoke-Checked {
    param([string]$Description, [scriptblock]$Command)
    & $Command
    if ($LASTEXITCODE -ne 0) {
        throw "$Description failed with exit code $LASTEXITCODE"
    }
}

$repoRoot = Split-Path $PSScriptRoot -Parent
$distDir = Join-Path $repoRoot 'Dist'
$binDir = Join-Path $distDir 'bin'
$chocoDir = Join-Path $distDir 'chocolatey'

if ($Version.Split('.').Count -ne 4) {
    throw "Version '$Version' is not a 4-part version (expected e.g. 2.6.1.0)"
}

$remoteUrl = git -C $repoRoot remote get-url origin
if ($remoteUrl -notmatch 'github\.com[:/](?<repo>[^/]+/[^/.]+)') {
    throw "Could not determine the GitHub repo (owner/name) from origin remote '$remoteUrl'"
}
$repo = $Matches.repo
$setupAssetName = "WinMute-$Version-Setup.exe"
$setupUrl = "https://github.com/$repo/releases/download/$Version/$setupAssetName"

Write-Host "Repo:          $repo"
Write-Host "Version:       $Version"
Write-Host "Setup asset:   $setupUrl"
Write-Host ''

# --- Stage Dist\bin from the signed release asset -------------------------

New-Item -ItemType Directory -Force -Path (Join-Path $binDir 'lang') | Out-Null

Write-Host 'Downloading signed WinMute.exe from the GitHub Release...'
Invoke-Checked 'gh release download' {
    gh release download $Version --repo $repo --pattern 'WinMute.exe' --dir $binDir --clobber
}

Copy-Item (Join-Path $repoRoot 'Translations\*.json') (Join-Path $binDir 'lang') -Force

$guiMarker = Join-Path $binDir 'WinMute.exe.gui'
if (-not (Test-Path $guiMarker)) {
    New-Item -ItemType File -Path $guiMarker | Out-Null
}

# --- Chocolatey pack --------------------------------------------------------

$nuspecSource = Join-Path $chocoDir 'WinMute.nuspec'
$nuspecGenerated = Join-Path $chocoDir 'WinMute.generated.nuspec'

try {
    $content = Get-Content $nuspecSource -Raw
    $content = $content.Replace('{{VERSION}}', $Version).Replace('{{YEAR}}', (Get-Date).Year.ToString())
    Set-Content -Path $nuspecGenerated -Value $content -NoNewline

    Write-Host ''
    Write-Host 'Packing Chocolatey package...'
    Invoke-Checked 'choco pack' {
        choco pack $nuspecGenerated --outputdirectory $chocoDir
    }
} finally {
    Remove-Item $nuspecGenerated -ErrorAction SilentlyContinue
}

$nupkgPath = Join-Path $chocoDir "winmute.$Version.nupkg"
if (-not (Test-Path $nupkgPath)) {
    throw "Expected package '$nupkgPath' was not created by choco pack"
}

# --- Publish steps (confirm before each) ------------------------------------

Write-Host ''
Write-Host "Chocolatey package ready: $nupkgPath"
$pushCommand = "choco push `"$nupkgPath`" --source https://push.chocolatey.org/"
Write-Host "  $pushCommand"
if ((Read-Host 'Push to Chocolatey now? [y/N]') -eq 'y') {
    Invoke-Checked 'choco push' { choco push $nupkgPath --source 'https://push.chocolatey.org/' }
}

Write-Host ''
$wingetArgs = @('update', '--submit', '--urls', "$setupUrl|x64", '--version', $Version, 'LX-Systems.WinMute')
Write-Host '  wingetcreate ' ($wingetArgs -join ' ')
if ((Read-Host 'Submit the winget manifest update now? [y/N]') -eq 'y') {
    Invoke-Checked 'wingetcreate update' { wingetcreate @wingetArgs }
}

Write-Host ''
Write-Host 'Done.'

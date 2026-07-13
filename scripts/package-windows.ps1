[CmdletBinding()]
param(
    [string]$ReleaseBuild = "build/release",
    [string]$PortableBuild = "build/release-portable",
    [string]$DistDir = "dist/windows"
)

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$releaseBuildPath = Join-Path $repoRoot $ReleaseBuild
$portableBuildPath = Join-Path $repoRoot $PortableBuild
$distPath = Join-Path $repoRoot $DistDir
$stageRoot = Join-Path $repoRoot "build/windows-package"
$installerStage = Join-Path $stageRoot "installer"
$portableStage = Join-Path $stageRoot "portable"
$zipPath = Join-Path $distPath "ZealRN-0.1.0-alpha-win64-portable.zip"
$setupPath = Join-Path $distPath "ZealRN-0.1.0-alpha-win64-setup.exe"
$checksumsPath = Join-Path $distPath "SHA256SUMS"

function Invoke-Checked {
    param([scriptblock]$Command, [string]$Description)
    & $Command
    if ($LASTEXITCODE -ne 0) {
        throw "$Description failed with exit code $LASTEXITCODE."
    }
}

Remove-Item $stageRoot -Recurse -Force -ErrorAction SilentlyContinue
New-Item $installerStage, $portableStage, $distPath -ItemType Directory -Force | Out-Null
Remove-Item $zipPath, $setupPath, $checksumsPath -Force -ErrorAction SilentlyContinue

Invoke-Checked { cmake --install $releaseBuildPath --prefix $installerStage } "Installer staging"
Invoke-Checked { cmake --install $portableBuildPath --prefix $portableStage } "Portable staging"

& (Join-Path $PSScriptRoot "validate-windows-package.ps1") -Root $installerStage
& (Join-Path $PSScriptRoot "validate-windows-package.ps1") -Root $portableStage

Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::CreateFromDirectory(
    $portableStage,
    $zipPath,
    [System.IO.Compression.CompressionLevel]::Optimal,
    $false
)

$makensis = Get-Command makensis.exe -ErrorAction Stop
Invoke-Checked {
    & $makensis.Source "/DAPP_SOURCE=$installerStage" "/DOUTPUT_FILE=$setupPath" "/DAPP_ICON=$(Join-Path $repoRoot "src/app/resources/zeal.ico")" (Join-Path $repoRoot "pkg/windows/zealrn.nsi")
} "NSIS packaging"

$checksumLines = foreach ($artifact in @($zipPath, $setupPath)) {
    $hash = (Get-FileHash $artifact -Algorithm SHA256).Hash.ToLowerInvariant()
    "$hash *$([System.IO.Path]::GetFileName($artifact))"
}
Set-Content -Path $checksumsPath -Value $checksumLines -Encoding ascii

Write-Output "Windows artifacts:"
Get-Item $zipPath, $setupPath, $checksumsPath | Select-Object FullName, Length

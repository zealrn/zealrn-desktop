[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Root
)

$ErrorActionPreference = "Stop"
$rootPath = (Resolve-Path $Root).Path

$required = @(
    "zealrn.exe",
    "Qt6Core.dll",
    "Qt6Gui.dll",
    "Qt6Widgets.dll",
    "Qt6Network.dll",
    "Qt6Sql.dll",
    "Qt6PrintSupport.dll",
    "Qt6WebEngineCore.dll",
    "Qt6WebEngineWidgets.dll",
    "QtWebEngineProcess.exe",
    "plugins/platforms/qwindows.dll",
    "plugins/sqldrivers/qsqlite.dll",
    "resources/qtwebengine_resources.pak",
    "COPYING",
    "THIRD_PARTY_LICENSES.md"
)

foreach ($relativePath in $required) {
    $path = Join-Path $rootPath $relativePath
    if (-not (Test-Path $path -PathType Leaf)) {
        throw "Missing Windows deployment file: $relativePath"
    }
}

$locale = Get-ChildItem (Join-Path $rootPath "translations/qtwebengine_locales") -Filter "*.pak" -File -ErrorAction Stop |
    Select-Object -First 1
if (-not $locale) {
    throw "No Qt WebEngine locale was deployed."
}

$forbidden = Get-ChildItem $rootPath -Recurse -File | Where-Object {
    $_.Extension -in @(".cpp", ".h", ".obj", ".pdb", ".lib")
}
if ($forbidden) {
    throw "Build/source files were included in deployment: $($forbidden[0].FullName)"
}

$versionOutput = & (Join-Path $rootPath "zealrn.exe") --attach-console --version 2>&1 | Out-String
if ($LASTEXITCODE -ne 0 -or $versionOutput -notmatch "0\.1\.0-alpha") {
    throw "Deployed zealrn.exe version check failed: $versionOutput"
}

Write-Output "Validated Windows deployment: $rootPath"

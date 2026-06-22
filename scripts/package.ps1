<#
.SYNOPSIS
    Packages interactiveIO into a self-contained ZIP that runs on another
    Windows PC without Qt or a compiler installed.

.DESCRIPTION
    Stages the built executables, gathers every runtime dependency the GUI
    needs (Qt DLLs + plugins via windeployqt, plus the MinGW runtime DLLs),
    adds the standalone console / REST-API executables and the key docs, then
    compresses the whole folder into dist\interactiveIO-v<version>-Windows-x64.zip.

    The console (interactiveIO.exe) and API (interactiveIO-api.exe) builds are
    statically linked and need no DLLs; the GUI (interactiveIO-gui.exe) is the
    one that pulls in Qt.

.PARAMETER QtPath
    Root of the Qt (MinGW) install that holds bin\windeployqt.exe.

.PARAMETER MingwPath
    bin folder of the MinGW toolchain (source of libgcc / libstdc++ / winpthread).

.PARAMETER BuildDir
    Folder containing the built *.exe (default: build\bin\Release).

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File scripts\package.ps1
#>

[CmdletBinding()]
param(
    [string]$QtPath    = "C:\Qt\6.10.2\mingw_64",
    [string]$MingwPath = "C:\Qt\Tools\mingw1310_64\bin",
    [string]$BuildDir  = "build\bin\Release"
)

$ErrorActionPreference = "Stop"

# Always operate from the repository root (this script lives in scripts\).
$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $repoRoot

function Write-Step($msg) { Write-Host "==> $msg" -ForegroundColor Cyan }
function Write-Ok($msg)   { Write-Host "    $msg" -ForegroundColor Green }
function Write-Warn($msg) { Write-Host "    $msg" -ForegroundColor Yellow }

Write-Host "=============================================" -ForegroundColor White
Write-Host "  Packaging interactiveIO for distribution"   -ForegroundColor White
Write-Host "=============================================" -ForegroundColor White

# --- Resolve version from the single source of truth (CMakeLists.txt) --------
$version = "1.0.0"
$cmake = Get-Content "CMakeLists.txt" -Raw
if ($cmake -match 'project\s*\(\s*interactiveIO\s+VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)') {
    $version = $Matches[1]
}
$packageName = "interactiveIO-v$version-Windows-x64"
$stageDir    = Join-Path "dist" $packageName
$zipPath     = Join-Path "dist" "$packageName.zip"
Write-Ok "Version: $version"

# --- Sanity checks -----------------------------------------------------------
$guiExe = Join-Path $BuildDir "interactiveIO-gui.exe"
if (-not (Test-Path $guiExe)) {
    throw "GUI executable not found: $guiExe`nBuild first (e.g. cmake --build --preset mingw-qt)."
}
$windeployqt = Join-Path $QtPath "bin\windeployqt.exe"
if (-not (Test-Path $windeployqt)) {
    throw "windeployqt not found: $windeployqt`nPass -QtPath <your Qt mingw folder>."
}

# --- Fresh staging directory -------------------------------------------------
Write-Step "Preparing staging folder"
if (Test-Path $stageDir) { Remove-Item $stageDir -Recurse -Force }
New-Item -ItemType Directory -Path $stageDir | Out-Null
New-Item -ItemType Directory -Path (Join-Path $stageDir "docs") | Out-Null

# --- Copy executables --------------------------------------------------------
Write-Step "Copying executables"
$exes = @("interactiveIO-gui.exe", "interactiveIO.exe", "interactiveIO-api.exe")
foreach ($exe in $exes) {
    $src = Join-Path $BuildDir $exe
    if (Test-Path $src) {
        Copy-Item $src $stageDir -Force
        Write-Ok $exe
    } else {
        Write-Warn "skipped (not built): $exe"
    }
}

# --- Gather Qt runtime + plugins for the GUI ---------------------------------
Write-Step "Running windeployqt (Qt DLLs + plugins)"
# windeployqt prints harmless warnings (e.g. missing DirectX dxcompiler.dll) to
# stderr. Under $ErrorActionPreference='Stop' the 2>&1 merge would turn those
# into terminating errors, so relax the preference just for this call and rely
# on the exit code to detect real failures.
$prevEap = $ErrorActionPreference
$ErrorActionPreference = "Continue"
$deployLog = & $windeployqt --release --no-translations --compiler-runtime `
    (Join-Path $stageDir "interactiveIO-gui.exe") 2>&1
$deployExit = $LASTEXITCODE
$ErrorActionPreference = $prevEap
if ($deployExit -ne 0) {
    $deployLog | ForEach-Object { Write-Host $_ }
    throw "windeployqt failed with exit code $deployExit"
}
Write-Ok "Qt dependencies deployed"

# --- Ensure the MinGW runtime DLLs are present (belt-and-braces) -------------
Write-Step "Verifying MinGW runtime DLLs"
$runtimeDlls = @("libgcc_s_seh-1.dll", "libstdc++-6.dll", "libwinpthread-1.dll")
foreach ($dll in $runtimeDlls) {
    $dest = Join-Path $stageDir $dll
    if (-not (Test-Path $dest)) {
        $src = Join-Path $MingwPath $dll
        if (Test-Path $src) {
            Copy-Item $src $stageDir -Force
            Write-Ok "added $dll"
        } else {
            Write-Warn "missing $dll (not found in $MingwPath)"
        }
    } else {
        Write-Ok "$dll present"
    }
}

# --- Documentation -----------------------------------------------------------
Write-Step "Copying documentation"
$docs = @("README.md", "docs\VISA_GUIDE.md", "docs\USB_GUIDE.md", "docs\DEPLOYMENT_GUIDE.md")
foreach ($doc in $docs) {
    if (Test-Path $doc) {
        Copy-Item $doc (Join-Path $stageDir "docs") -Force
        Write-Ok (Split-Path $doc -Leaf)
    }
}

# --- Readme for the recipient ------------------------------------------------
$readme = @"
interactiveIO v$version - Windows x64
=====================================

HOW TO RUN
  - GUI:      double-click interactiveIO-gui.exe
  - Console:  run interactiveIO.exe
  - REST API: run interactiveIO-api.exe

REQUIREMENTS
  - Windows 10 or later. No Qt or compiler install needed - all required
    DLLs are bundled in this folder.
  - Keep the folder structure intact: the platforms\, imageformats\,
    iconengines\ and styles\ subfolders must stay next to the GUI exe.

CONNECTIONS
  - TCP/IP works out of the box, nothing extra to install.
  - VISA (GPIB / USB / VXI-11 / Serial) needs a VISA runtime on this PC,
    e.g. NI-VISA: https://www.ni.com/visa

Built: $(Get-Date -Format 'yyyy-MM-dd HH:mm')
"@
$readme | Out-File -FilePath (Join-Path $stageDir "READ_ME_FIRST.txt") -Encoding utf8

# --- Compress ----------------------------------------------------------------
Write-Step "Creating ZIP archive"
if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
Compress-Archive -Path (Join-Path $stageDir "*") -DestinationPath $zipPath -Force
$zipSize = "{0:N1} MB" -f ((Get-Item $zipPath).Length / 1MB)
Write-Ok "$zipPath ($zipSize)"

Write-Host ""
Write-Host "=============================================" -ForegroundColor White
Write-Host "  Done" -ForegroundColor Green
Write-Host "=============================================" -ForegroundColor White
Write-Host "  Folder: $stageDir"
Write-Host "  ZIP:    $zipPath"
Write-Host ""
Write-Host "  Copy the ZIP to the other PC, extract it, and run interactiveIO-gui.exe."

# Force a clean success exit code (native tools like windeployqt may write
# harmless warnings to stderr that would otherwise taint the host exit code).
exit 0

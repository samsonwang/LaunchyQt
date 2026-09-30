# ============================================================================
# LaunchyQt Windows Release deployment script (PowerShell)
# ----------------------------------------------------------------------------
# Deploys everything the CMake build does NOT copy, so the Release output in
# build/bin/Release can actually run:
#   1. Qt runtime              (windeployqt -> qtlibs/, core Qt5*.dll back to root,
#                               duplicate Qt5*.dll dropped from qtlibs/,
#                               qt.conf honoured via misc/qt.conf, Prefix=./qtlibs)
#   2. MSVC 2017 CRT           (vcruntime140.dll / msvcp140.dll / concrt140.dll)
#   3. Embedded Python 3.6.6   (python36.dll + Lib + DLLs)
#   4. Skins                   (repo skins/)
#   5. OpenSSL runtime         (safety copy from deps/openssl if missing)
# See docs/HOW_TO_BUILD.org -> "Run the Release build (deploy on Windows)".
#
# Run from the repo root, e.g.:
#   powershell -ExecutionPolicy Bypass -File misc\deploy_win.ps1
# (or: Set-ExecutionPolicy -Scope CurrentUser RemoteSigned)
# ============================================================================

# ---- configurable paths (adjust to your machine) ---------------------------
$QT_PREFIX        = "C:\Qt\Qt5.12.10\5.12.10\msvc2017_64"
$PYTHON_EMBED_ZIP = "E:\Package\python-3.6.6-embed-amd64.zip" # empty -> defaults to deps/python-embed/python-3.6.6-embed-amd64.zip
$PYTHON366        = "C:\Program Files\Python36" # only used by $INSTALL_PYSIDE2 (needs pip)
$VS_REDIST        = ""          # auto-detected when empty
$INSTALL_PYSIDE2  = $false      # set $true to bundle PySide2 for Qt-based python plugins
$QT_CONF_SRC      = ""          # empty -> defaults to misc/qt.conf next to this script

# ---- helper functions -------------------------------------------------------
function Copy-Tree($src, $dst) {
    if (-not (Test-Path $src)) { return }
    Copy-Item -Path $src -Destination $dst -Recurse -Force
}

function Find-Redist {
    $roots = @(
        "C:\Program Files (x86)\Microsoft Visual Studio",
        "C:\Program Files\Microsoft Visual Studio"
    )
    foreach ($r in $roots) {
        if (-not (Test-Path $r)) { continue }
        foreach ($yearDir in Get-ChildItem $r -Directory) {
            $msvc = Join-Path $yearDir.FullName "VC\Redist\MSVC"
            if (-not (Test-Path $msvc)) { continue }
            foreach ($ver in Get-ChildItem $msvc -Directory) {
                foreach ($crtDir in Get-ChildItem (Join-Path $ver.FullName "x64\Microsoft.VC14*.CRT") -Directory -ErrorAction SilentlyContinue) {
                    if (Test-Path (Join-Path $crtDir.FullName "vcruntime140.dll")) {
                        return $crtDir.FullName
                    }
                }
            }
        }
    }
    if (Test-Path "C:\Windows\System32\vcruntime140.dll") {
        return "C:\Windows\System32"
    }
    return $null
}

# ---- derive project / release directories ----------------------------------
$scriptDir = $PSScriptRoot
$repoRoot  = Split-Path $scriptDir -Parent        # misc/ -> repo root
if (-not (Test-Path (Join-Path $repoRoot "build\bin\Release\Launchy.exe"))) {
    $repoRoot = $scriptDir
}
$RELEASE   = Join-Path $repoRoot "build\bin\Release"
$SKINS_SRC = Join-Path $repoRoot "skins"

Write-Host "[deploy] PROJECT_ROOT = $repoRoot"
Write-Host "[deploy] RELEASE      = $RELEASE"

# ---- sanity check -----------------------------------------------------------
if (-not (Test-Path (Join-Path $RELEASE "Launchy.exe"))) {
    Write-Host "[deploy][ERROR] Launchy.exe not found in $RELEASE" -ForegroundColor Red
    Write-Host "[deploy][ERROR] Build the Release config first: cmake --build build --config Release" -ForegroundColor Red
    exit 1
}

# ---- 1. windeployqt (Qt runtime) -------------------------------------------
# The bundled qt.conf (misc/qt.conf, Prefix = ./qtlibs) relocates Qt's plugins,
# qml and translations into the "qtlibs" subfolder. So we deploy the whole Qt
# tree there with --dir, then copy the top-level Qt5*.dll back to the Release
# root: Launchy.exe's Qt imports are resolved by the Windows loader, which only
# looks next to the executable (qt.conf cannot redirect those). Qt's plugins
# under qtlibs/ resolve their own Qt imports against the already-loaded modules.
$wdeploy = Join-Path $QT_PREFIX "bin\windeployqt.exe"
if (-not (Test-Path $wdeploy)) {
    Write-Host "[deploy][ERROR] windeployqt not found at $wdeploy" -ForegroundColor Red
    Write-Host "[deploy][ERROR] Set `$QT_PREFIX to your Qt5.12.10 msvc2017_64 install." -ForegroundColor Red
    exit 1
}
$qtlibs = Join-Path $RELEASE "qtlibs"
Write-Host "[deploy] Running windeployqt --dir qtlibs ..."
& $wdeploy --release --no-angle --no-opengl-sw --dir $qtlibs (Join-Path $RELEASE "Launchy.exe")
if ($LASTEXITCODE -ne 0) {
    Write-Host "[deploy][WARN] windeployqt reported an error, check output above." -ForegroundColor Yellow
}

# Copy the top-level Qt DLLs imported directly by Launchy.exe back to the root.
$QT_CORE_DLLS = @("Qt5Core.dll", "Qt5Gui.dll", "Qt5Widgets.dll", "Qt5Network.dll",
                  "Qt5Svg.dll", "Qt5WinExtras.dll",
                  "D3Dcompiler_47.dll", "libEGL.dll", "libGLESV2.dll", "opengl32sw.dll")
foreach ($d in $QT_CORE_DLLS) {
    $srcf = Join-Path $qtlibs $d
    if (Test-Path $srcf) { Copy-Item -Path $srcf -Destination $RELEASE -Force }
}

# Extra Qt module required by PySide2 at runtime. windeployqt only walks
# Launchy.exe's import table, so it never sees what python plugins pull in:
# PySide2/pyside2.abi3.dll statically imports Qt5Core and Qt5Qml, hence
# Qt5Qml.dll must be present or Qt-based python plugins fail to import.
# Qt5Qml.dll itself only needs Qt5Core.dll + Qt5Network.dll (both already in
# the root), so this single DLL is the whole extra dependency. Take it from the
# Qt install so the version always matches the rest of the Qt runtime.
$QT_EXTRA_DLLS = @("Qt5Qml.dll")
foreach ($d in $QT_EXTRA_DLLS) {
    $srcf = Join-Path $QT_PREFIX ("bin\" + $d)
    if (-not (Test-Path $srcf)) { continue }
    # Root copy only: the Windows loader resolves Qt5Qml's own imports from the
    # exe directory, a duplicate under qtlibs/ would just be dropped later on.
    Copy-Item -Path $srcf -Destination $RELEASE -Force
}

# Drop the Qt DLL duplicates windeployqt leaves next to the plugins in qtlibs/.
# The plugins import Qt5Core/Qt5Gui/... and the Windows loader resolves those
# from the directory the application image was loaded from (the Release root) -
# it never searches a dependent DLL's own folder - so the copies sitting in
# qtlibs/ are dead weight (~22 MB) as long as the root has them. Any Qt5*.dll
# that is NOT in the root is kept under qtlibs/.
foreach ($f in (Get-ChildItem -Path $qtlibs -Filter "Qt5*.dll" -File -ErrorAction SilentlyContinue)) {
    if (Test-Path (Join-Path $RELEASE $f.Name)) {
        Write-Host "[deploy] Dropping duplicate $($f.Name) from qtlibs (already in root)"
        try { Remove-Item -Path $f.FullName -Force }
        catch { Write-Host "[deploy][WARN] could not remove $($f.FullName) : $_" -ForegroundColor Yellow }
    }
}

# Remove stale Qt plugin folders left in the root by older deploy runs so the
# layout matches qt.conf (everything Qt lives under qtlibs/). App translations in
# root/translations are NOT touched.
$QT_PLUGIN_DIRS = @("platforms", "imageformats", "iconengines", "bearer", "styles", "qml")
foreach ($p in $QT_PLUGIN_DIRS) {
    $pd = Join-Path $RELEASE $p
    if (Test-Path $pd) {
        Write-Host "[deploy] Removing stale Qt folder in root: $p"
        Remove-Item -Path $pd -Recurse -Force
    }
}

# Restore the bundled qt.conf (windeployqt may have written its own Prefix).
if (-not $QT_CONF_SRC) { $QT_CONF_SRC = Join-Path $scriptDir "qt.conf" }
if (Test-Path $QT_CONF_SRC) {
    Write-Host "[deploy] Installing qt.conf (Prefix = ./qtlibs)"
    Copy-Item -Path $QT_CONF_SRC -Destination (Join-Path $RELEASE "qt.conf") -Force
} else {
    Write-Host "[deploy][WARN] misc/qt.conf not found, leaving existing qt.conf in place." -ForegroundColor Yellow
}

# ---- 2. MSVC 2017 CRT -------------------------------------------------------
if (-not $VS_REDIST) { $VS_REDIST = Find-Redist }
if ($VS_REDIST) {
    Write-Host "[deploy] Copying MSVC CRT from $VS_REDIST"
    foreach ($f in "vcruntime140.dll", "msvcp140.dll", "concrt140.dll") {
        $srcf = Join-Path $VS_REDIST $f
        if (Test-Path $srcf) { Copy-Item -Path $srcf -Destination $RELEASE -Force }
    }
} else {
    Write-Host "[deploy][WARN] Could not locate VC++ 2017 Redistributable." -ForegroundColor Yellow
    Write-Host "[deploy][WARN] Install 'Visual C++ 2017 Redistributable (x64)' on the target machine." -ForegroundColor Yellow
}

# ---- 3. embedded Python 3.6.6 (official embeddable package) -----------------
# We use the compact embeddable package from deps/ instead of a full CPython
# install. The OS loader needs python36.dll next to Launchy.exe, so it stays in
# the Release root (this also defines sys.prefix = root). Everything else of the
# embeddable package (python36.zip = the zipped stdlib, the C extension .pyd
# modules, python3.dll) goes into Release/python/ so the whole Python footprint
# is one small directory. python36._pth (beside python36.dll) points Python at
# that folder; it also adds python/Lib/site-packages for the optional PySide2.
if (-not $PYTHON_EMBED_ZIP) {
    $PYTHON_EMBED_ZIP = Join-Path $repoRoot "deps\python-embed\python-3.6.6-embed-amd64.zip"
}
if (-not (Test-Path $PYTHON_EMBED_ZIP)) {
    Write-Host "[deploy][ERROR] python embeddable zip not found at $PYTHON_EMBED_ZIP" -ForegroundColor Red
    Write-Host "[deploy][ERROR] Set `$PYTHON_EMBED_ZIP to the python-3.6.6-embed-amd64.zip path." -ForegroundColor Red
    exit 1
}
Add-Type -AssemblyName System.IO.Compression.FileSystem
# Extract into a temp dir UNDER the Release folder (a normal Windows path) so
# cleanup never depends on $env:TEMP, which may resolve to a non-Windows path
# when the script is launched from a POSIX shell.
$tmp = Join-Path $RELEASE (".embed_extract_" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Force -Path $tmp | Out-Null
[System.IO.Compression.ZipFile]::ExtractToDirectory($PYTHON_EMBED_ZIP, $tmp)

Write-Host "[deploy] Deploying embedded Python from $PYTHON_EMBED_ZIP ..."
# python36.dll must sit in the Release root (loaded by the OS loader).
Copy-Item -Path (Join-Path $tmp "python36.dll") -Destination $RELEASE -Force
# python3.dll is the stable-ABI forwarder that extension modules (e.g.
# PySide2/pyside2.abi3.dll, shiboken2) statically import. Keep it next to
# python36.dll in the root so the Windows loader can resolve "python3.dll"
# without relying on python/ being on PATH.
Copy-Item -Path (Join-Path $tmp "python3.dll") -Destination $RELEASE -Force
# The rest of the embeddable package lives in Release/python/ (compact dir).
$pyDir = Join-Path $RELEASE "python"
New-Item -ItemType Directory -Force -Path $pyDir | Out-Null
foreach ($f in (Get-ChildItem $tmp -File)) {
    if ($f.Name -eq "python36.dll") { continue }
    Copy-Item -Path $f.FullName -Destination $pyDir -Force
}
# python36._pth next to python36.dll tells Python where its files are.
# Paths are relative to this file's directory (the Release root).
$pthContent = @("python/python36.zip", "python", "python/Lib/site-packages")
$pthContent | Out-File -FilePath (Join-Path $RELEASE "python36._pth") -Encoding ascii

# Remove the old full-install layout (Lib/ + DLLs/) and any stray root python36.zip
# so the Release root stays compact and consistent with the embeddable package.
# The stdlib paths exceed MAX_PATH, so delete via the \\?\ long-path prefix and
# never let a failure abort the deploy.
function Remove-LongPathDir($p) {
    if (-not (Test-Path $p)) { return }
    try {
        $lp = "\\?\" + (Resolve-Path $p).ProviderPath
        [System.IO.Directory]::Delete($lp, $true)
        Write-Host "[deploy] Removed stale $p (now using embeddable package)"
    } catch {
        Write-Host "[deploy][WARN] could not remove $p : $_" -ForegroundColor Yellow
    }
}
Remove-LongPathDir (Join-Path $RELEASE "Lib")
Remove-LongPathDir (Join-Path $RELEASE "DLLs")
$rz = Join-Path $RELEASE "python36.zip"
if (Test-Path $rz) {
    try { Remove-Item -Path $rz -Force } catch { Write-Host "[deploy][WARN] could not remove $rz : $_" -ForegroundColor Yellow }
}
# Clean the temp extract dir (flat, no long-path issue).
try { Remove-Item -Path $tmp -Recurse -Force } catch { Write-Host "[deploy][WARN] could not remove temp $tmp" -ForegroundColor Yellow }

# ---- 3b. optional PySide2 (needed by python plugins that use Qt) -----------
if ($INSTALL_PYSIDE2) {
    $sitePkg = Join-Path $RELEASE "python\Lib\site-packages"
    if (-not (Test-Path (Join-Path $sitePkg "PySide2"))) {
        Write-Host "[deploy] Installing PySide2==5.12.6 into python\Lib\site-packages ..."
        & "$PYTHON366\python.exe" -m pip install --target $sitePkg PySide2==5.12.6
    } else {
        Write-Host "[deploy] PySide2 already present, skip."
    }
}

# ---- 4. skins ---------------------------------------------------------------
if (Test-Path $SKINS_SRC) {
    Write-Host "[deploy] Copying skins ..."
    Copy-Tree $SKINS_SRC (Join-Path $RELEASE "skins")
} else {
    Write-Host "[deploy][WARN] skins/ not found at $SKINS_SRC, UI will have no skin." -ForegroundColor Yellow
}

# ---- 5. safety: OpenSSL runtime --------------------------------------------
$sslDst = Join-Path $RELEASE "libssl-1_1-x64.dll"
if (-not (Test-Path $sslDst)) {
    $sslSrc = Join-Path $repoRoot "deps\openssl\libssl-1_1-x64.dll"
    if (Test-Path $sslSrc) {
        Write-Host "[deploy] Copying OpenSSL runtime from deps/openssl ..."
        Copy-Item -Path $sslSrc -Destination $RELEASE -Force
        Copy-Item -Path (Join-Path $repoRoot "deps\openssl\libcrypto-1_1-x64.dll") -Destination $RELEASE -Force
    }
}

Write-Host "[deploy] Done. Launchy should now run from $(Join-Path $RELEASE 'Launchy.exe')"

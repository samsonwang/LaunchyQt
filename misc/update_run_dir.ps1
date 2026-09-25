<#
.SYNOPSIS
    Update the artifacts built by this project (LaunchyQt) into the production run directory
    (D:\Program Files\Launchy by default).

.DESCRIPTION
    Steps:
      1. Terminate every running Launchy process and wait until they are fully gone
         (so that the files are no longer locked);
      2. Copy only the files produced by this project's build into the run directory;
      3. Verify every copied file with SHA256 and report missing / mismatching files;
      4. Re-run this script elevated (UAC) when the run directory is not writable.

    Artifacts of this project (copied):
      Launchy.exe / Launchy.dll / Launchy.pdb
      PluginPy.dll / PluginPy.pdb
      plugins\<plugin name>\*.dll, *.pdb, *.png
      python\launchy.pyd, python\launchy_util.py

    Third-party files (never copied, left untouched in the run directory):
      Qt5*.dll, qt.conf, qtlibs\, styles\, translations\
      python\PySide2\, python\shiboken2\
      python3.dll, python36.dll, python36.zip, vcruntime140.dll
      libEGL.dll, libGLESv2.dll, opengl32sw.dll, D3Dcompiler_47.dll
      libssl-1_1-x64.dll, libcrypto-1_1-x64.dll
      config\ (user configuration data, this script never reads or writes it)

.PARAMETER Config
    Build configuration: Release (default) or Debug.

.PARAMETER SourceDir
    Build output directory, <repo root>\build\bin\<Config> by default.

.PARAMETER TargetDir
    Production run directory, 'D:\Program Files\Launchy' by default.

.PARAMETER NoPdb
    Do not copy the .pdb debug symbol files.

.PARAMETER DryRun
    Preview mode: only list the processes to terminate and the files to copy,
    nothing is terminated and nothing is written.

.PARAMETER Restart
    Start Launchy again after the update.

.PARAMETER SkipVerify
    Skip the SHA256 verification after the update.

.PARAMETER Force
    Copy even when the target file has identical content (identical files are skipped by default).

.PARAMETER NoElevate
    Fail immediately instead of re-running elevated when the run directory is not writable.

.EXAMPLE
    powershell -NoProfile -ExecutionPolicy Bypass -File .\misc\update_run_dir.ps1
    Update the Release artifacts into D:\Program Files\Launchy with the default parameters.

.EXAMPLE
    .\misc\update_run_dir.ps1 -DryRun
    Preview the files that would be copied, without modifying anything.

.EXAMPLE
    .\misc\update_run_dir.ps1 -Restart -NoPdb -Config Debug
    Update the Debug artifacts (without pdb files) and start Launchy afterwards.
#>

[CmdletBinding()]
param (
    [ValidateSet('Release', 'Debug')]
    [string] $Config = 'Release',

    [string] $SourceDir,

    [string] $TargetDir = 'D:\Program Files\Launchy',

    [switch] $NoPdb,

    [switch] $DryRun,

    [switch] $Restart,

    [switch] $SkipVerify,

    [switch] $Force,

    [switch] $NoElevate
)

$ErrorActionPreference = 'Stop'

# Repository root = parent of this script's folder (misc)
$RepoRoot = Split-Path -Parent $PSScriptRoot
if (-not $SourceDir) {
    $SourceDir = Join-Path $RepoRoot ('build\bin\' + $Config)
}
$SourceDir = [System.IO.Path]::GetFullPath($SourceDir)
$TargetDir = [System.IO.Path]::GetFullPath($TargetDir)

##################################################
# Console output helpers
##################################################
function Write-Section([string] $Text) {
    Write-Host ''
    Write-Host ('== ' + $Text) -ForegroundColor Cyan
}

function Write-Info([string] $Text) {
    Write-Host ('   ' + $Text)
}

function Write-Ok([string] $Text) {
    Write-Host ('   [OK] ' + $Text) -ForegroundColor Green
}

function Write-Skip([string] $Text) {
    Write-Host ('   [--] ' + $Text) -ForegroundColor DarkGray
}

function Write-Warn([string] $Text) {
    Write-Host ('   [!!] ' + $Text) -ForegroundColor Yellow
}

function Write-Fail([string] $Text) {
    Write-Host ('   [XX] ' + $Text) -ForegroundColor Red
}

##################################################
# Artifacts produced by this project (paths relative to SourceDir)
# Anything not listed here is treated as a third-party file and is never copied
##################################################
function Get-ProjectArtifactList {
    param (
        [Parameter(Mandatory)] [string] $SourceDir,
        [switch] $NoPdb
    )

    $relativePaths = New-Object System.Collections.Generic.List[string]

    # Main executable, core library (LaunchyLib builds Launchy.dll),
    # Python binding module and the project Python scripts
    $topLevelFiles = @(
        'Launchy.exe',
        'Launchy.dll',
        'Launchy.pdb',
        'PluginPy.dll',
        'PluginPy.pdb',
        'python\launchy.pyd',
        'python\launchy_util.py'
    )

    foreach ($relativePath in $topLevelFiles) {
        if ($NoPdb -and $relativePath.EndsWith('.pdb')) {
            continue
        }
        $relativePaths.Add($relativePath)
    }

    # Native plugins: plugin binary, icon resources and debug symbols under plugins\<plugin name>\
    $pluginRoot = Join-Path $SourceDir 'plugins'
    if (Test-Path -LiteralPath $pluginRoot -PathType Container) {
        $allowedExtensions = @('.dll', '.png')
        if (-not $NoPdb) {
            $allowedExtensions += '.pdb'
        }

        foreach ($pluginDir in Get-ChildItem -LiteralPath $pluginRoot -Directory) {
            foreach ($file in Get-ChildItem -LiteralPath $pluginDir.FullName -File) {
                if ($allowedExtensions -contains $file.Extension.ToLowerInvariant()) {
                    $relativePaths.Add(('plugins\{0}\{1}' -f $pluginDir.Name, $file.Name))
                }
            }
        }
    }

    return @($relativePaths | Sort-Object)
}

##################################################
# Shared helpers
##################################################
function Test-IsAdmin {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($identity)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Test-DirectoryWritable {
    param ([Parameter(Mandatory)] [string] $Directory)

    $probeFile = Join-Path $Directory ('.~writetest-' + [Guid]::NewGuid().ToString('N') + '.tmp')
    try {
        New-Item -ItemType File -Path $probeFile -ErrorAction Stop | Out-Null
        Remove-Item -LiteralPath $probeFile -Force -ErrorAction SilentlyContinue
        return $true
    } catch {
        return $false
    }
}

function Get-FileSha256 {
    param ([Parameter(Mandatory)] [string] $Path)
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
}

function Invoke-ScriptElevated {
    $windowsPowerShell = Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe'

    $arguments = @(
        '-NoProfile'
        '-ExecutionPolicy'
        'Bypass'
        '-File'
        ('"{0}"' -f $PSCommandPath)
        '-Config'
        $Config
        '-SourceDir'
        ('"{0}"' -f $SourceDir)
        '-TargetDir'
        ('"{0}"' -f $TargetDir)
    )
    if ($NoPdb) { $arguments += '-NoPdb' }
    if ($Restart) { $arguments += '-Restart' }
    if ($SkipVerify) { $arguments += '-SkipVerify' }
    if ($Force) { $arguments += '-Force' }

    Write-Warn 'The production run directory is not writable, re-running this script elevated (UAC prompt)...'
    Start-Process -FilePath $windowsPowerShell -Verb RunAs -ArgumentList $arguments
}

##################################################
# Terminate all Launchy processes
##################################################
function Stop-LaunchyProcesses {
    param (
        [int] $TimeoutSeconds = 15,
        [switch] $PreviewOnly
    )

    $processes = @(Get-Process -Name 'Launchy' -ErrorAction SilentlyContinue)
    if ($processes.Count -eq 0) {
        Write-Skip 'No running Launchy process found'
        return
    }

    Write-Info ('Found {0} Launchy process(es), terminating:' -f $processes.Count)

    foreach ($process in $processes) {
        $description = 'PID {0}' -f $process.Id
        try {
            $description = $description + ' (' + $process.Path + ')'
        } catch {
            # Process information may be unreadable for some processes, ignore it
        }

        if ($PreviewOnly) {
            Write-Skip ('[preview] would terminate ' + $description)
            continue
        }

        try {
            Stop-Process -Id $process.Id -Force -ErrorAction Stop
            Write-Ok ('Terminated ' + $description)
        } catch {
            Write-Fail ('Failed to terminate ' + $description + ': ' + $_.Exception.Message)
        }
    }

    if ($PreviewOnly) {
        return
    }

    # Wait until the processes are fully gone so that the files are not locked during the copy
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        if (@(Get-Process -Name 'Launchy' -ErrorAction SilentlyContinue).Count -eq 0) {
            Write-Ok 'All Launchy processes have exited'
            return
        }
        Start-Sleep -Milliseconds 250
    }

    $survivors = @(Get-Process -Name 'Launchy' -ErrorAction SilentlyContinue)
    $survivorIds = ($survivors | ForEach-Object { $_.Id }) -join ', '
    throw ('Launchy processes are still alive after {0} seconds (PID: {1}), terminate them manually and retry' -f $TimeoutSeconds, $survivorIds)
}

##################################################
# Copy the artifacts of this project
# (third-party files are not part of the list, therefore they are never copied)
##################################################
function Copy-ProjectArtifacts {
    param (
        [Parameter(Mandatory)] [string] $SourceDir,
        [Parameter(Mandatory)] [string] $TargetDir,
        [Parameter(Mandatory)] [string[]] $RelativePaths,
        [switch] $ForceCopy,
        [switch] $PreviewOnly
    )

    $copied = New-Object System.Collections.Generic.List[string]
    $unchanged = New-Object System.Collections.Generic.List[string]
    $failed = New-Object System.Collections.Generic.List[string]

    foreach ($relativePath in $RelativePaths) {
        $sourceFile = Join-Path $SourceDir $relativePath
        $targetFile = Join-Path $TargetDir $relativePath

        if (-not (Test-Path -LiteralPath $sourceFile -PathType Leaf)) {
            Write-Fail ('Source file is missing (build the project first): ' + $relativePath)
            $failed.Add($relativePath)
            continue
        }

        if ($PreviewOnly) {
            Write-Skip ('[preview] would update ' + $relativePath)
            continue
        }

        if (-not $ForceCopy -and (Test-Path -LiteralPath $targetFile -PathType Leaf)) {
            $sameContent = $false
            try {
                $sameContent = ((Get-FileSha256 -Path $sourceFile) -eq (Get-FileSha256 -Path $targetFile))
            } catch {
                # Reading the target failed (e.g. locked by another process), treat it as "needs update"
                $sameContent = $false
            }

            if ($sameContent) {
                Write-Skip ('Identical content, skipped: ' + $relativePath)
                $unchanged.Add($relativePath)
                continue
            }
        }

        try {
            $targetParent = Split-Path -Parent $targetFile
            if (-not (Test-Path -LiteralPath $targetParent -PathType Container)) {
                New-Item -ItemType Directory -Path $targetParent -Force -ErrorAction Stop | Out-Null
            }

            Copy-Item -LiteralPath $sourceFile -Destination $targetFile -Force -ErrorAction Stop
            Write-Ok ('Updated ' + $relativePath)
            $copied.Add($relativePath)
        } catch {
            Write-Fail ('Copy failed ' + $relativePath + ': ' + $_.Exception.Message)
            $failed.Add($relativePath)
        }
    }

    return [pscustomobject]@{
        Copied    = $copied
        Unchanged = $unchanged
        Failed    = $failed
    }
}

##################################################
# Verify the deployed files
##################################################
function Test-ProjectArtifacts {
    param (
        [Parameter(Mandatory)] [string] $SourceDir,
        [Parameter(Mandatory)] [string] $TargetDir,
        [Parameter(Mandatory)] [string[]] $RelativePaths
    )

    $problems = New-Object System.Collections.Generic.List[string]

    foreach ($relativePath in $RelativePaths) {
        $sourceFile = Join-Path $SourceDir $relativePath
        $targetFile = Join-Path $TargetDir $relativePath

        if (-not (Test-Path -LiteralPath $sourceFile -PathType Leaf)) {
            # A missing source file was already reported by the copy step, do not report it twice
            continue
        }

        if (-not (Test-Path -LiteralPath $targetFile -PathType Leaf)) {
            $problems.Add($relativePath + ' (target file does not exist)')
            continue
        }

        try {
            if ((Get-FileSha256 -Path $sourceFile) -ne (Get-FileSha256 -Path $targetFile)) {
                $problems.Add($relativePath + ' (SHA256 mismatch)')
            }
        } catch {
            $problems.Add($relativePath + ' (verification failed: ' + $_.Exception.Message + ')')
        }
    }

    return @($problems)
}

##################################################
# Main flow
##################################################
Write-Host ''
Write-Host '===================================================================' -ForegroundColor Cyan
Write-Host ' LaunchyQt: update build artifacts to the production run directory' -ForegroundColor Cyan
Write-Host '===================================================================' -ForegroundColor Cyan
Write-Info ('Config   : ' + $Config)
Write-Info ('Source   : ' + $SourceDir)
Write-Info ('Target   : ' + $TargetDir)
if ($NoPdb) {
    Write-Info 'Symbols  : skip (-NoPdb)'
} else {
    Write-Info 'Symbols  : include (*.pdb)'
}
if ($DryRun) {
    Write-Warn 'Preview mode: no process is terminated and nothing is written'
}

# 1. Build the list of artifacts to copy
Write-Section '1/5 Build the artifact list'
if (-not (Test-Path -LiteralPath $SourceDir -PathType Container)) {
    Write-Fail ('Build output directory not found: ' + $SourceDir)
    Write-Info 'Build the project first, for example: cmake --build build --config Release'
    exit 1
}

$artifacts = @(Get-ProjectArtifactList -SourceDir $SourceDir -NoPdb:$NoPdb)
if ($artifacts.Count -eq 0) {
    Write-Fail 'No artifact of this project was found, make sure the build is finished.'
    exit 1
}
Write-Ok ('{0} file(s) in total (artifacts of this project only, third-party files are not copied)' -f $artifacts.Count)

# 2. Check the production run directory
Write-Section '2/5 Check the production run directory'
if (-not (Test-Path -LiteralPath $TargetDir -PathType Container)) {
    Write-Fail ('Production run directory not found: ' + $TargetDir)
    exit 1
}

if (-not (Test-DirectoryWritable -Directory $TargetDir)) {
    if ($NoElevate -or $DryRun) {
        Write-Fail 'The production run directory is not writable, run PowerShell as administrator and retry.'
        exit 1
    }
    if (Test-IsAdmin) {
        Write-Fail 'The production run directory is not writable (already elevated: the folder may be locked or its permissions are restricted).'
        exit 1
    }
    Invoke-ScriptElevated
    exit 0
}
Write-Ok 'The production run directory exists and is writable'

# 3. Terminate all Launchy processes
Write-Section '3/5 Terminate Launchy processes'
Stop-LaunchyProcesses -PreviewOnly:$DryRun

# 4. Copy the artifacts
Write-Section '4/5 Copy the artifacts'
$result = Copy-ProjectArtifacts -SourceDir $SourceDir -TargetDir $TargetDir `
    -RelativePaths $artifacts -ForceCopy:$Force -PreviewOnly:$DryRun

if ($DryRun) {
    Write-Host ''
    Write-Ok ('Preview finished: {0} file(s) would be updated in {1}' -f $artifacts.Count, $TargetDir)
    exit 0
}

# 5. Verify the deployment (files that already failed during the copy step are skipped)
$problems = @()
$verifyTargets = @($artifacts | Where-Object { $result.Failed -notcontains $_ })

if ($SkipVerify) {
    Write-Section '5/5 Verify the deployment (skipped)'
    Write-Skip 'SHA256 verification skipped as requested'
} else {
    Write-Section '5/5 Verify the deployment (SHA256)'
    if ($verifyTargets.Count -eq 0) {
        Write-Skip 'Nothing to verify (every file failed during the copy step)'
    } else {
        $problems = @(Test-ProjectArtifacts -SourceDir $SourceDir -TargetDir $TargetDir -RelativePaths $verifyTargets)
        foreach ($problem in $problems) {
            Write-Fail $problem
        }
        if ($problems.Count -eq 0) {
            Write-Ok ('All {0} file(s) verified successfully' -f $verifyTargets.Count)
        }
    }
}

# Summary
Write-Section 'Summary'
Write-Info ('Updated    : {0}' -f $result.Copied.Count)
Write-Info ('Unchanged  : {0}' -f $result.Unchanged.Count)
Write-Info ('Failed     : {0}' -f $result.Failed.Count)
Write-Info ('Verify err : {0}' -f $problems.Count)

$exitCode = 0
if ($result.Failed.Count -gt 0 -or $problems.Count -gt 0) {
    $exitCode = 1
}

Write-Host ''
if ($exitCode -eq 0) {
    Write-Host ('  Deployment finished: ' + $TargetDir) -ForegroundColor Green

    if ($Restart) {
        $launchyExe = Join-Path $TargetDir 'Launchy.exe'
        try {
            Start-Process -FilePath $launchyExe -WorkingDirectory $TargetDir -ErrorAction Stop
            Write-Ok 'Launchy restarted'
        } catch {
            Write-Fail ('Failed to start Launchy: ' + $_.Exception.Message)
            $exitCode = 1
        }
    }
} else {
    Write-Host '  Deployment did not fully succeed, see the errors above' -ForegroundColor Red
}
Write-Host ''

exit $exitCode

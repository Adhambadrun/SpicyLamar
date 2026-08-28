# =============================================================================
#  SPICY LAMAR — build.ps1
#  Full C++ build in pure PowerShell. Locates Visual Studio via vswhere.exe,
#  imports the vcvars64 environment, compiles the .rc with rc.exe and builds
#  the monolith with cl.exe.
#
#  Usage:
#      powershell -NoProfile -ExecutionPolicy Bypass -File build\build.ps1
#
#  Output:  dist\SpicyLamar.exe
# =============================================================================
$ErrorActionPreference = 'Stop'

# Repo root = parent of build\ (this script lives in build\).
$repoRoot = Split-Path -Parent $PSScriptRoot
$objDir   = Join-Path $repoRoot 'build\obj'
$distDir  = Join-Path $repoRoot 'dist'
$res      = Join-Path $repoRoot 'build\obj\app.res'
$outExe   = Join-Path $distDir 'SpicyLamar.exe'

Push-Location $repoRoot
try {
    Write-Host "=========================================================="
    Write-Host " SPICY LAMAR v4.1 - C++ PORTABLE BUILD (PS)"
    Write-Host "=========================================================="

    # --- 1. Locate Visual Studio -------------------------------------------
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path -LiteralPath $vswhere)) {
        throw "vswhere.exe not found at $vswhere - install Visual Studio 2022 Build Tools (C++ workload)."
    }
    $vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ([string]::IsNullOrWhiteSpace($vsPath)) {
        throw "No Visual Studio installation with the C++ (MSVC) workload found."
    }
    $vcvars = Join-Path $vsPath 'VC\Auxiliary\Build\vcvars64.bat'
    if (-not (Test-Path -LiteralPath $vcvars)) {
        throw "vcvars64.bat not found at $vcvars"
    }

    # --- 2. Import the vcvars64 environment into this PowerShell process ----
    Write-Host "[1/3] Initializing MSVC environment ($vcvars)..."
    $envDump = & cmd /c "call `"$vcvars`" >nul 2>&1 && set"
    if ($LASTEXITCODE -ne 0 -or -not $envDump) {
        throw "Failed to import the MSVC environment (vcvars64.bat)."
    }
    foreach ($line in $envDump) {
        if ($line -match '^([^=]+)=(.*)$') {
            [System.Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process')
        }
    }
    $clPath = (Get-Command cl.exe -ErrorAction SilentlyContinue)
    if (-not $clPath) { throw "cl.exe not available after vcvars import." }

    # --- 3. Compile resources ------------------------------------------------
    New-Item -ItemType Directory -Path $objDir -Force | Out-Null
    New-Item -ItemType Directory -Path $distDir -Force | Out-Null

    Write-Host "[2/3] Compiling resources (rc.exe)..."
    Push-Location (Join-Path $repoRoot 'resources')
    try {
        & rc.exe /nologo /fo $res app.rc
        if ($LASTEXITCODE -ne 0) { throw "rc.exe failed with exit code $LASTEXITCODE" }
    } finally {
        Pop-Location
    }

    # --- 4. Compile + link the monolith ---------------------------------------
    Write-Host "[3/3] Compiling and linking the monolith (cl.exe)..."
    $cxxFlags = @(
        '/nologo','/std:c++20','/O2','/Oi','/GL','/Gy','/MT','/utf-8',
        '/DUNICODE','/D_UNICODE','/DSPICY_LAMAR_QUANTUM','/DNDEBUG','/EHsc',
        '/c', (Join-Path $repoRoot 'src\main.cpp'), '/Fo:' + (Join-Path $objDir 'main.obj')
    )
    & cl.exe @cxxFlags
    if ($LASTEXITCODE -ne 0) { throw "cl.exe (compile) failed with exit code $LASTEXITCODE" }

    $linkFlags = @(
        '/nologo','/LTCG','/OPT:REF','/OPT:ICF','/SUBSYSTEM:WINDOWS,10.0','/MACHINE:X64',
        (Join-Path $objDir 'main.obj'), $res,
        'comctl32.lib','shell32.lib','ole32.lib','oleaut32.lib','advapi32.lib','uxtheme.lib',
        'winmm.lib','avrt.lib','dwmapi.lib','uiautomationcore.lib','oleacc.lib','tdh.lib','psapi.lib',
        '/OUT:' + '"' + $outExe + '"'
    )
    & link.exe @linkFlags
    if ($LASTEXITCODE -ne 0) { throw "link.exe failed with exit code $LASTEXITCODE" }

    if (-not (Test-Path -LiteralPath $outExe)) { throw "Output exe was not produced: $outExe" }

    Write-Host ""
    Write-Host "=========================================================="
    Write-Host " SUCCESS - $outExe"
    Write-Host (" Size: {0:N0} bytes" -f (Get-Item -LiteralPath $outExe).Length)
    Write-Host "=========================================================="
    exit 0
} finally {
    Pop-Location
}

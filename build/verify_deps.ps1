# =============================================================================
#  SPICY LAMAR — verify_deps.ps1
#  Checks that every tool needed to build is present and prints a report.
#  Exit code 0 = everything OK, 1 = something missing.
# =============================================================================
$ErrorActionPreference = 'Continue'
$ok = $true

Write-Host "== SpicyLamar build dependency check =="

function Check([string]$name, [bool]$present, [string]$hint = "") {
    if ($present) { Write-Host "  [OK]   $name" }
    else { Write-Host "  [MISS] $name  $hint" -ForegroundColor Red; $script:ok = $false }
}

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
Check "vswhere.exe (Visual Studio installer)" (Test-Path -LiteralPath $vswhere) "(install VS 2022 Build Tools with C++ workload)"

$vsPath = $null
if (Test-Path -LiteralPath $vswhere) {
    $vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null
}
Check "MSVC C++ tools (VC.Tools.x86.x64)" (-not [string]::IsNullOrWhiteSpace($vsPath)) "(add the 'Desktop development with C++' workload)"

if ($vsPath) {
    $vcvars = Join-Path $vsPath 'VC\Auxiliary\Build\vcvars64.bat'
    Check "vcvars64.bat" (Test-Path -LiteralPath $vcvars)
}

$csc64 = Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
$csc32 = Join-Path $env:WINDIR 'Microsoft.NET\Framework\v4.0.30319\csc.exe'
Check ".NET Framework csc.exe (C# build)" ((Test-Path -LiteralPath $csc64) -or (Test-Path -LiteralPath $csc32)) "(install .NET Framework 4.x)"

Check "PowerShell 5.1+ (build helpers)" ($PSVersionTable.PSVersion.Major -ge 5)

$repoRoot = Split-Path -Parent $PSScriptRoot
Check "resources\app.rc" (Test-Path -LiteralPath (Join-Path $repoRoot 'resources\app.rc'))
Check "resources\app.manifest" (Test-Path -LiteralPath (Join-Path $repoRoot 'resources\app.manifest'))
Check "resources\icon.ico (bundled fallback)" (Test-Path -LiteralPath (Join-Path $repoRoot 'resources\icon.ico'))
Check "src\main.cpp" (Test-Path -LiteralPath (Join-Path $repoRoot 'src\main.cpp'))

Write-Host ""
if ($ok) {
    Write-Host "All build dependencies present." -ForegroundColor Green
    exit 0
} else {
    Write-Host "Some dependencies are missing - see [MISS] entries above." -ForegroundColor Red
    exit 1
}

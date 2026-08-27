# =============================================================================
#  SPICY LAMAR — verify_artifact.ps1
#  Validates the built artifact: exists, is a PE32+ executable, has an
#  embedded icon and version resource. Exit 0 = all good.
# =============================================================================
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$exe = Join-Path $repoRoot 'dist\SpicyLamar RingCentral Auto-Answer.exe'

Write-Host "== SpicyLamar artifact verification =="

if (-not (Test-Path -LiteralPath $exe)) {
    Write-Host "  [FAIL] $exe not found - run build_portable.bat or build\build.ps1 first." -ForegroundColor Red
    exit 1
}

$file = Get-Item -LiteralPath $exe
Write-Host "  [OK]   exists ($($file.Length) bytes)"

# PE header check: MZ + PE\0\0 at 0x3C
$fs = [System.IO.File]::OpenRead($exe)
try {
    $br = New-Object System.IO.BinaryReader($fs)
    $mz = $br.ReadUInt16()
    $fs.Position = 0x3C
    $peOff = $br.ReadInt32()
    $fs.Position = $peOff
    $sig = $br.ReadUInt32()
    if ($mz -ne 0x5A4D -or $sig -ne 0x00004550) {
        Write-Host "  [FAIL] not a valid PE executable." -ForegroundColor Red
        exit 1
    }
    $machine = $br.ReadUInt16()
    $machineName = switch ($machine) { 0x8664 { "x64" } 0x14C { "x86" } 0xAA64 { "ARM64" } default { "0x{0:X}" -f $machine } }
    Write-Host "  [OK]   PE32+ $machineName executable"
    $fs.Position = $peOff + 0x5C
    $subsystem = $br.ReadUInt16()
    $subName = switch ($subsystem) { 2 { "Windows GUI" } 3 { "Console" } default { $subsystem } }
    Write-Host "  [OK]   Subsystem: $subName"
} finally {
    $fs.Close()
    $fs.Dispose()
}

# Icon resource check via GDI+
Add-Type -AssemblyName System.Drawing
try {
    $icon = [System.Drawing.Icon]::ExtractAssociatedIcon($exe)
    if ($icon) {
        Write-Host "  [OK]   Embedded icon: $($icon.Width)x$($icon.Height)"
        $icon.Dispose()
    } else {
        Write-Host "  [WARN] No icon resource detected." -ForegroundColor Yellow
    }
} catch {
    Write-Host "  [WARN] Could not read icon resource: $($_.Exception.Message)" -ForegroundColor Yellow
}

# Version resource check
try {
    $vi = [System.Diagnostics.FileVersionInfo]::GetVersionInfo($exe)
    if ($vi.FileDescription) {
        Write-Host "  [OK]   Version info: '$($vi.FileDescription)' v$($vi.FileVersion)"
    } else {
        Write-Host "  [WARN] No version resource detected." -ForegroundColor Yellow
    }
} catch {
    Write-Host "  [WARN] Could not read version info: $($_.Exception.Message)" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Verification complete." -ForegroundColor Green
exit 0

# =============================================================================
#  SPICY LAMAR — extract_icon.ps1
#  Extracts the genuine Windows Bluetooth icon from a system file and saves
#  it as an .ico so the resource compiler (rc.exe / llvm-rc) can embed it.
#
#  Usage:
#      powershell -NoProfile -ExecutionPolicy Bypass -File build\extract_icon.ps1
#      powershell -NoProfile -ExecutionPolicy Bypass -File build\extract_icon.ps1 -OutFile "resources\icon.ico"
#
#  Notes:
#   - Paths are resolved relative to this script, so it works no matter what
#     the current working directory is.
#   - If the icon already exists it is left untouched (exit 0).
#   - Exit code 1 means "could not extract" — callers may fall back to the
#     committed resources\icon.ico and continue.
# =============================================================================
param(
    [string]$OutFile = ""
)

$ErrorActionPreference = 'Stop'

# Repo root = parent of the build\ folder this script lives in.
$repoRoot = Split-Path -Parent $PSScriptRoot

if ([string]::IsNullOrWhiteSpace($OutFile)) {
    $OutFile = Join-Path $repoRoot 'resources\icon.ico'
}
# Allow relative -OutFile values to resolve against the repo root.
if (-not [System.IO.Path]::IsPathRooted($OutFile)) {
    $OutFile = Join-Path $repoRoot $OutFile
}

if (Test-Path -LiteralPath $OutFile) {
    $existing = Get-Item -LiteralPath $OutFile
    if ($existing.Length -gt 0) {
        Write-Host "[icon] $OutFile already exists ($($existing.Length) bytes) - skipping extraction."
        exit 0
    }
}

$candidates = @(
    (Join-Path $env:WINDIR 'System32\bthprops.cpl'),   # Bluetooth control panel (genuine BT glyph)
    (Join-Path $env:WINDIR 'System32\deviceflow.dll'),
    (Join-Path $env:WINDIR 'System32\shell32.dll')     # fallback: generic shell icon
)

Add-Type -AssemblyName System.Drawing

foreach ($candidate in $candidates) {
    if (-not (Test-Path -LiteralPath $candidate)) { continue }
    try {
        $icon = [System.Drawing.Icon]::ExtractAssociatedIcon($candidate)
        if ($null -eq $icon) { continue }

        $outDir = Split-Path -Parent $OutFile
        if (-not [string]::IsNullOrWhiteSpace($outDir) -and -not (Test-Path -LiteralPath $outDir)) {
            New-Item -ItemType Directory -Path $outDir -Force | Out-Null
        }

        $fs = [System.IO.File]::OpenWrite($OutFile)
        try {
            $icon.Save($fs)
        } finally {
            $fs.Close()
            $fs.Dispose()
            $icon.Dispose()
        }
        $size = (Get-Item -LiteralPath $OutFile).Length
        Write-Host "[icon] Extracted $($size) bytes from $candidate -> $OutFile"
        exit 0
    } catch {
        Write-Warning "[icon] Failed to extract from $candidate : $($_.Exception.Message)"
    }
}

Write-Warning "[icon] No usable system Bluetooth icon found; the build will use the bundled resources\icon.ico instead."
exit 1

# =============================================================================
#  SPICY LAMAR — package_portable.ps1
#  Packages the built single-file portable executable into a zip:
#      dist\SpicyLamar RingCentral Auto-Answer.exe
#      dist\README.txt  -> packaged as README.txt
#  Output: dist\SpicyLamar-Portable.zip
# =============================================================================
$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$exe      = Join-Path $repoRoot 'dist\SpicyLamar RingCentral Auto-Answer.exe'
$readme   = Join-Path $repoRoot 'dist\README.txt'
$zipPath  = Join-Path $repoRoot 'dist\SpicyLamar-Portable.zip'

if (-not (Test-Path -LiteralPath $exe)) {
    throw "Build output missing: $exe"
}
if (-not (Test-Path -LiteralPath $readme)) {
    throw "README missing: $readme"
}

# Download the current repo name so the zip has a stable name.
$staging = Join-Path $env:TEMP 'SpicyLamarPortableStaging'
if (Test-Path -LiteralPath $staging) { Remove-Item -LiteralPath $staging -Recurse -Force }
New-Item -ItemType Directory -Path $staging -Force | Out-Null

Copy-Item -LiteralPath $exe    -Destination $staging
Copy-Item -LiteralPath $readme -Destination $staging

if (Test-Path -LiteralPath $zipPath) { Remove-Item -LiteralPath $zipPath -Force }
Compress-Archive -Path (Join-Path $staging '*') -DestinationPath $zipPath -CompressionLevel Optimal

Write-Host "Packaged portable build:"
Write-Host ("  Exe: {0:N0} bytes" -f (Get-Item -LiteralPath $exe).Length)
Write-Host ("  Zip: {0:N0} bytes" -f (Get-Item -LiteralPath $zipPath).Length)
Write-Host "  Ready artifact: $zipPath"

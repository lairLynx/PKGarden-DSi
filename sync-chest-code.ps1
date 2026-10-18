# The chest/journal code is shared with PKGarden-Knulli (single source of truth: Knulli's src/).
# Run without arguments to copy it into arm9/source/pkgarden; with -Check to verify the copies are identical.
param([switch]$Check)
$ErrorActionPreference = 'Stop'
$src = Join-Path $PSScriptRoot '..\PKGarden-Knulli\src'
$dst = Join-Path $PSScriptRoot 'arm9\source\pkgarden'
$files = @(
  'chest\Envelope.hpp','chest\Envelope.cpp','chest\ChestStore.hpp',
  'chest\LocalChestStore.hpp','chest\LocalChestStore.cpp',
  'chest\Journal.hpp','chest\Journal.cpp',
  'gba\Convert.hpp','gba\Convert.cpp',
  'util\Crc32.hpp','util\Crc32.cpp','util\Files.hpp','util\Files.cpp','util\MiniFs.hpp'
)
$bad = 0
foreach ($f in $files) {
  $s = Join-Path $src $f; $d = Join-Path $dst $f
  if ($Check) {
    if (-not (Test-Path $d) -or (Get-FileHash $s).Hash -ne (Get-FileHash $d).Hash) { Write-Host "DIFFERENT: $f"; $bad++ }
  } else {
    New-Item -ItemType Directory -Force (Split-Path $d) | Out-Null
    Copy-Item $s $d -Force
  }
}
if ($Check) { if ($bad) { exit 1 } else { Write-Host 'chest code is in sync' } } else { Write-Host "copied $($files.Count) files to $dst" }

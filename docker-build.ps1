# Builds the .nds with the same blocksds image the upstream CI uses. Requires Docker Desktop (running).
$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot
$image = 'skylyrac/blocksds:slim-v1.18.0'

# bmp2ds converts the graphics at build time; the upstream CI downloads the same release.
if (-not (Test-Path .tools/bmp2ds)) {
    New-Item -ItemType Directory -Force .tools | Out-Null
    Invoke-WebRequest https://github.com/Universal-Team/bmp2ds/releases/download/v1.0.0/bmp2ds-linux -OutFile .tools/bmp2ds
}

docker run --rm -v "${PSScriptRoot}:/work" -w /work $image bash -c 'install -m755 .tools/bmp2ds /usr/local/bin/bmp2ds && make -j4'
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
Get-ChildItem $PSScriptRoot -Filter *.nds | ForEach-Object { Write-Host "Built $($_.FullName) ($($_.Length) bytes)" }

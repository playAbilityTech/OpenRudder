param(
    [string]$DockerImage = "nordicplayground/nrfconnect-sdk:v2.2-branch"
)

$ErrorActionPreference = "Stop"

$firmwareDir = $PSScriptRoot
$repoRoot = Split-Path -Parent $firmwareDir
$buildDir = "build-xiao-sense"
$overlay = "boards/arm/seeed_xiao_nrf52840/seeed_xiao_nrf52840_sense.overlay"
$config = "boards/arm/seeed_xiao_nrf52840/seeed_xiao_nrf52840_sense.conf"

$dockerArgs = @(
    "run",
    "--rm",
    "-v", "${repoRoot}:/workdir/project",
    "-w", "/workdir/project/firmware-bluetooth",
    $DockerImage,
    "west", "build",
    "-b", "seeed_xiao_nrf52840",
    "-d", $buildDir,
    "-p", "always",
    "--",
    "-DOVERLAY_CONFIG=$config",
    "-DDTC_OVERLAY_FILE=$overlay"
)

& docker @dockerArgs
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

$uf2Path = Join-Path $firmwareDir "$buildDir\zephyr\remapper.uf2"
if (Test-Path $uf2Path) {
    Write-Host "Built $uf2Path"
} else {
    throw "Build finished, but $uf2Path was not found."
}

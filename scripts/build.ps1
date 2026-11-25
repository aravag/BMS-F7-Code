# Usage: .\scripts\build.ps1 [Debug|Release]
param([string]$Config = "Debug")
. "$PSScriptRoot\env.ps1"
Set-Location "$PSScriptRoot\..\bms_h5"
cmake --preset $Config
if ($LASTEXITCODE) { exit $LASTEXITCODE }
cmake --build --preset $Config
if ($LASTEXITCODE) { exit $LASTEXITCODE }
arm-none-eabi-size "build\$Config\bms_h5.elf"
# Raw binary for drag-and-drop flashing (the Nucleo's ST-LINK USB drive), used by deploy_jetson.sh
arm-none-eabi-objcopy -O binary "build\$Config\bms_h5.elf" "build\$Config\bms_h5.bin"

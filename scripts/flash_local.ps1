# Flash the Nucleo plugged into THIS laptop. Usage: .\scripts\flash_local.ps1 [Debug|Release]
param([string]$Config = "Debug")
. "$PSScriptRoot\env.ps1"
$elf = Resolve-Path "$PSScriptRoot\..\bms_h5\build\$Config\bms_h5.elf"
STM32_Programmer_CLI -c port=SWD mode=UR -w $elf -v -rst

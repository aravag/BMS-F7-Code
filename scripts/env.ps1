# Dot-source this (". .\scripts\env.ps1") to put the build tools on PATH.
# Uses the toolchain bundled with STM32CubeIDE's install folder (the IDE itself is not used).
$ide = "C:\ST\STM32CubeIDE_1.18.0\STM32CubeIDE\plugins"
$dirs = @(
  (Get-ChildItem "$ide\com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32*\tools\bin" -Directory -ErrorAction Stop | Select-Object -First 1),
  (Get-ChildItem "$ide\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer*\tools\bin" -Directory -ErrorAction Stop | Select-Object -First 1),
  (Get-ChildItem "$ide\com.st.stm32cube.ide.mcu.externaltools.openocd*\tools\bin" -Directory -ErrorAction Stop | Select-Object -First 1)
) | ForEach-Object FullName
$ninja = Get-ChildItem "$env:LOCALAPPDATA\Microsoft\WinGet\Packages" -Recurse -Filter ninja.exe -ErrorAction SilentlyContinue | Select-Object -First 1
if ($ninja) { $dirs += $ninja.DirectoryName }
$dirs += "C:\Program Files\CMake\bin"
$env:PATH = ($dirs -join ";") + ";" + $env:PATH

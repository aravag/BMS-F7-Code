# Only used by the micro-ROS library builder (scripts/build_microros.ps1), which runs
# `make print_cflags` to learn the MCU flags. The real build is CMake.
print_cflags:
	@echo -mcpu=cortex-m33 -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=hard -O2 -ffunction-sections -fdata-sections

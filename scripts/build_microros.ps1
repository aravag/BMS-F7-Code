# Builds micro_ros_stm32cubemx_utils/microros_static_library/libmicroros (libmicroros.a + headers)
# for the H5, using the official Docker builder. Needs Docker Desktop running. Takes a few minutes.
# Only needs re-running if the micro-ROS version or the MCU flags in bms_h5/Makefile change.
$proj = Resolve-Path "$PSScriptRoot\..\bms_h5"
"y" | docker run -i --rm -v "${proj}:/project" `
  --env MICROROS_LIBRARY_FOLDER=micro_ros_stm32cubemx_utils/microros_static_library `
  microros/micro_ros_static_library_builder:humble

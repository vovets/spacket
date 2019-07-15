cmake_minimum_required(VERSION 3.5)

enable_testing()

get_filename_component(SPACKET_ROOT .. ABSOLUTE)

list(APPEND CMAKE_MODULE_PATH "${SPACKET_ROOT}/cmake")

include(util)

set(JLINK_EXE
  /opt/SEGGER/JLink/JLinkExe
  CACHE
  FILEPATH
  "Path to JLinkExe binary on host system"
  )

set(JLINK_DEVICE_NAME
  STM32F103C8
  CACHE
  STRING
  "Device name that is recognized by j-link for -device option"
  )

set(JLINK_CONNECT_OPTIONS
  -autoconnect 1
  -device ${JLINK_DEVICE_NAME}
  -if SWD
  -speed 9000
  )

set(JLINK_FLASH_OPTIONS
  -exitonerror 1
  )

set(JLINK_RTT_OPTIONS
  -exitonerror 0
  )

util_join_cl(JLINK_RTT_COMMAND
  ${JLINK_EXE}
  ${JLINK_CONNECT_OPTIONS}
  ${JLINK_RTT_OPTIONS}
  )

set(SPACKET_FW_TOOLCHAIN_FILE
  "${SPACKET_ROOT}/cmake/toolchain_gcc_arm-none-eabi.cmake"
  CACHE
  FILEPATH
  "Toolchain file for building firmware projects"
  )

add_subdirectory(host_spacket)
add_subdirectory(host_utils)
add_subdirectory(test_host_various)
add_subdirectory(test_host_serial_loopback)
add_subdirectory(test_target_chibios)
add_subdirectory(test_target_loopback)
add_subdirectory(test_host_target_packet_echo)
add_subdirectory(test_host_target_packetizer_echo)
add_subdirectory(test_host_target_cobs_echo)
add_subdirectory(test_target_crc_device)

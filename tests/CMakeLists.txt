cmake_minimum_required(VERSION 3.5)

enable_testing()

get_filename_component(SPACKET_ROOT .. ABSOLUTE)

list(APPEND CMAKE_MODULE_PATH "${SPACKET_ROOT}/cmake")

include(util)

set(JLINK_EXE /opt/SEGGER/JLink/JLinkExe)

set(JLINK_CONNECT_OPTIONS
  -autoconnect 1
  -device STM32F103C8
  -if SWD
  -speed 4000
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

add_subdirectory(host_spacket)
add_subdirectory(host_utils)
add_subdirectory(test_host_various)
add_subdirectory(test_host_serial_loopback)
add_subdirectory(test_target_chibios)
add_subdirectory(test_target_packet_loopback)

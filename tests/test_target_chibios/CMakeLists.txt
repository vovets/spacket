cmake_minimum_required(VERSION 3.5)

get_filename_component(SPACKET_ROOT ../.. ABSOLUTE)

list(APPEND CMAKE_MODULE_PATH "${SPACKET_ROOT}/cmake")

include(fw_tests)
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

fw_tests_add_fw_subdirectory(test_target_chibios "${CMAKE_CURRENT_SOURCE_DIR}/fw")
fw_tests_add_jlink_test(chibios test_target_chibios "${CMAKE_CURRENT_SOURCE_DIR}/test.sh")

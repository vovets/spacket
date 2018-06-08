cmake_minimum_required(VERSION 3.5)
project(test_target_crc_device_fw C CXX ASM)

get_filename_component(SPACKET_ROOT ../../.. ABSOLUTE)

list(APPEND CMAKE_MODULE_PATH "${SPACKET_ROOT}/cmake")

include(gather)
include(chibios)
include(fw)
include(fw_vars)

list(APPEND SPACKET_EXCLUDED_FEATURES serial_device)

set(debug_optimization_flags -Os)

fw_add_executable(fw)

fw_compile_definitions(fw
  -DBOOST_STRICT_CONFIG
  -DSHELL_CONFIG_FILE
  -DBUFFER_ENABLE_DEBUG_PRINT_BUFFER
  -DBUFFER_ENABLE_DEBUG_PRINT_BUFFER_SIZE_ONLY
  )

ch_include(fw
  /os/common/startup/ARMCMx/compilers/GCC/mk/startup_stm32f1xx.cmake
  /os/common/ports/ARMCMx/compilers/GCC/mk/port_v7m.cmake
  /os/hal/hal.cmake
  /os/hal/ports/STM32/STM32F1xx/platform.cmake
  /os/hal/osal/rt/osal.cmake
  /os/hal/lib/streams/streams.cmake
  /os/rt/rt.cmake
  /os/various/shell/shell.cmake
  )

gt_include(fw
  "${SPACKET_ROOT}/tests/target_boards/STM32F103C8_MINIMAL/sources.cmake"
  "${SPACKET_ROOT}/lib/sources.cmake"
  "${SPACKET_ROOT}/lib/chibios/sources.cmake"
  "${SPACKET_ROOT}/lib/stm32/sources.cmake"
  "${SPACKET_ROOT}/lib/stm32/crc_v1/sources.cmake"
  )

fw_include_directories(fw
  conf
  .
  ${CH_ROOT_DIR}/os/license
  "${SPACKET_EXT}/SEGGER_RTT_V614c/RTT"
  "${SPACKET_EXT}/boost_1_66_0"
  )

fw_sources(fw
  "${SPACKET_EXT}/SEGGER_RTT_V614c/RTT/SEGGER_RTT.c"
  main.cpp
  system_halt_hook.cpp
  rtt_stream.cpp
  buffer.cpp
  shell_commands.cpp
  )

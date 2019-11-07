cmake_minimum_required(VERSION 3.5)
project(test_host_target_packet_device_fw C CXX ASM)

get_filename_component(SPACKET_ROOT ../../.. ABSOLUTE)
set(EXTERNAL ${SPACKET_ROOT}/external)
list(APPEND CMAKE_MODULE_PATH "${SPACKET_ROOT}/cmake")

include(gather)
include(chibios)
include(fw)
include(fw_vars)

set(CH_CPU cortex-m3)
set(CH_MCU_NAME STM32F103x8)
set(CH_ROOT_DIR [[${EXTERNAL}/chibios]])
set(debug_optimization_flags -Os)

fw_vars_set()
fw_add_executable(fw)

fw_compile_definitions(fw
  -DUART_USE_WAIT=FALSE
  -DBOOST_STRICT_CONFIG
  # -DENABLE_DEBUG_PRINT
  # -DDEBUG_PRINT_BUFFER_FULL
  # -DDEBUG_PRINT_BUFFER_FIRST_ONLY
  # -DSERIAL_DEVICE_ENABLE_DEBUG_PRINT
  # -DPACKET_DEVICE_ENABLE_DEBUG_PRINT
  )

ch_include(fw
  /os/common/startup/ARMCMx/compilers/GCC/mk/startup_stm32f1xx.cmake
  /os/common/ports/ARMCMx/compilers/GCC/mk/port_v7m.cmake
  /os/hal/hal.cmake
  /os/hal/ports/STM32/STM32F1xx/platform.cmake
  /os/hal/osal/rt/osal.cmake
  /os/hal/lib/streams/streams.cmake
  /os/rt/rt.cmake
  )

gt_include(fw
  "${SPACKET_ROOT}/tests/target_boards/STM32F103C8_MINIMAL/sources.cmake"
  "${SPACKET_ROOT}/lib/sources.cmake"
  "${SPACKET_ROOT}/lib/chibios/sources.cmake"
  "${SPACKET_ROOT}/lib/stm32/crc_v1/sources.cmake"
  )

fw_include_directories(fw
  conf
  .
  ${CH_ROOT_DIR}/os/license
  "${EXTERNAL}/SEGGER_RTT_V614c/RTT"
  "${EXTERNAL}/boost_1_66_0"
  )

fw_sources(fw
  "${EXTERNAL}/SEGGER_RTT_V614c/RTT/SEGGER_RTT.c"
  main.cpp
  system_halt_hook.cpp
  rtt_stream.cpp
  )

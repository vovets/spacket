cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(sbus2pwm_fw C CXX ASM)

if(NOT DEFINED SPACKET_ROOT)
  message(FATAL_ERROR "SPACKET_ROOT must be defined and evaluate to the root dir of spacket lib")
endif(NOT DEFINED SPACKET_ROOT)

get_filename_component(SPACKET_ROOT ${SPACKET_ROOT} ABSOLUTE)
set(EXTERNAL ${SPACKET_ROOT}/external)
list(APPEND CMAKE_MODULE_PATH "${SPACKET_ROOT}/cmake")

message("${CMAKE_MOUDLE_PATH}")

include(gather)
include(chibios)
include(fw)
include(fw_vars)

set(CH_CPU cortex-m3)
set(CH_MCU_NAME STM32F103x8)
set(CH_ROOT_DIR [[${EXTERNAL}/chibios]])
set(debug_optimization_flags -Os)
set(CH_PROCESS_STACK_SIZE 0x500)
# set(CH_MAIN_STACK_SIZE 0x1000)

fw_vars_set()
fw_add_executable(sbus2pwm_fw)

fw_compile_definitions(sbus2pwm_fw
  -DUART_USE_WAIT=FALSE
  -DBOOST_STRICT_CONFIG
  -DBOOST_DISABLE_ASSERTS
  -DNDEBUG
  -DSPACKET_ENABLE_THREAD_CHECK_STACK
  -DENABLE_DEBUG_PRINT
#  -DCPH_ENABLE_DEBUG_PRINT
#  -DDEBUG_PRINT_BUFFER_FIRST_ONLY
  -DDEBUG_PRINT_BUFFER_FULL
#  -DDEBUG_PRINT_BUFFER_ASCII
  )

ch_include(sbus2pwm_fw
  /os/common/startup/ARMCMx/compilers/GCC/mk/startup_stm32f1xx.cmake
  /os/common/ports/ARMCMx/compilers/GCC/mk/port_v7m.cmake
  /os/hal/hal.cmake
  /os/hal/ports/STM32/STM32F1xx/platform.cmake
  /os/hal/osal/rt/osal.cmake
  /os/hal/lib/streams/streams.cmake
  /os/rt/rt.cmake
  )

ch_set_source_file_properties(/os/hal/ports/STM32/LLD/TIMv1/hal_pwm_lld.c COMPILE_FLAGS -Wno-implicit-fallthrough)
ch_set_source_file_properties(/os/rt/src/chschd.c COMPILE_FLAGS -Wno-implicit-fallthrough)

gt_include(sbus2pwm_fw
  "${SPACKET_ROOT}/lib/sources.cmake"
  "${SPACKET_ROOT}/lib/chibios/sources.cmake"
  "${SPACKET_ROOT}/lib/stm32/crc_v1/sources.cmake"
  )

fw_include_directories(sbus2pwm_fw
  conf
  .
  ${CH_ROOT_DIR}/os/license
  "${EXTERNAL}/SEGGER_RTT_V614c/RTT"
  "${EXTERNAL}/boost_1_66_0"
  )

fw_sources(sbus2pwm_fw
  "${EXTERNAL}/SEGGER_RTT_V614c/RTT/SEGGER_RTT.c"
  board.c
  main.cpp
  system_halt_hook.cpp
  rtt_stream.cpp
  )

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
  -device ${JLINK_DEVICE_NAME}
  -if SWD
  -speed 9000
  )

include(util)

util_join_cl(JLINK_CONNECT_OPTIONS_STR "" ${JLINK_CONNECT_OPTIONS})

fw_add_flash_target_to_exe(sbus2pwm_fw)
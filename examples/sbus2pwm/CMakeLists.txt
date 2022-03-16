cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(sbus2pwm_fw C CXX ASM)

if(NOT DEFINED SPACKET_ROOT)
  message(FATAL_ERROR "SPACKET_ROOT must be defined and evaluate to the root dir of spacket lib")
endif(NOT DEFINED SPACKET_ROOT)

set(EXTERNAL ${SPACKET_ROOT}/external)
list(APPEND CMAKE_MODULE_PATH "${SPACKET_ROOT}/cmake")

include(build_type)
include(gather)
include(chibios)
include(fw)
include(fw_vars)

#set(board "blue_pill")
set(board "black_pill")

set(CH_CPU-blue_pill cortex-m3)
set(CH_CPU-black_pill cortex-m4)
  
set(CH_MCU_NAME-blue_pill STM32F103x8)
set(CH_MCU_NAME-black_pill STM32F411xE)

set(JLINK_DEVICE_NAME-blue_pill STM32F103C8)
set(JLINK_DEVICE_NAME-black_pill STM32F411CE)

set(CH_CPU ${CH_CPU-${board}})
set(CH_MCU_NAME ${CH_MCU_NAME-${board}})
set(CH_ROOT_DIR [[${EXTERNAL}/chibios]])

set(spacket_debug_print_enabled 1)

set(spacket_debug_print_buffer_size 512)

set(debug_optimization_flags -Os -finline-small-functions -findirect-inlining)
set(CH_PROCESS_STACK_SIZE 0x580)
set(CH_MAIN_STACK_SIZE    0x80)
set(spacket_debug_print_excess_stack_size 384)

#set(debug_optimization_flags -O3)
#set(CH_PROCESS_STACK_SIZE 0x600)
#set(CH_MAIN_STACK_SIZE 0x80)
#set(spacket_debug_print_excess_stack_size 384)

#set(debug_optimization_flags -O0)
#set(CH_PROCESS_STACK_SIZE 0x1000)
#set(CH_MAIN_STACK_SIZE 0x180)
#set(spacket_debug_print_excess_stack_size 384)

if(${spacket_debug_print_enabled})
math(EXPR CH_PROCESS_STACK_SIZE "${CH_PROCESS_STACK_SIZE} + ${spacket_debug_print_buffer_size} + ${spacket_debug_print_excess_stack_size}")
math(EXPR CH_MAIN_STACK_SIZE "${CH_MAIN_STACK_SIZE} + ${spacket_debug_print_buffer_size} + ${spacket_debug_print_excess_stack_size}")
endif()

fw_vars_set()
fw_add_executable(sbus2pwm_fw)

fw_compile_definitions(sbus2pwm_fw
  -DUART_USE_WAIT=FALSE
  -DBOOST_STRICT_CONFIG
  -DBOOST_DISABLE_ASSERTS
  -DNDEBUG
  -DPRINTF_INCLUDE_CONFIG_H
  -DSPACKET_ENABLE_CHECK_STACK
  -DSPACKET_DEBUG_PRINT_BUFFER_SIZE=${spacket_debug_print_buffer_size}
#  -DENABLE_DEBUG_PRINT
#  -DDEBUG_PRINT_BUFFER_FIRST_ONLY
  -DDEBUG_PRINT_BUFFER_FULL
#  -DDEBUG_PRINT_BUFFER_ASCII
  -DSPACKET_STATIC_POOL_ALLOCATOR_ALL_ERRORS_FATAL
  )

if(${spacket_debug_print_enabled})
fw_compile_definitions(sbus2pwm_fw -DCPH_ENABLE_DEBUG_PRINT)
endif()

set(ch_include_board-blue_pill
  /os/common/startup/ARMCMx/compilers/GCC/mk/startup_stm32f1xx.cmake
  /os/common/ports/ARMCMx/compilers/GCC/mk/port_v7m.cmake
  /os/hal/ports/STM32/STM32F1xx/platform.cmake
  )
set(ch_include_board-black_pill
  /os/common/startup/ARMCMx/compilers/GCC/mk/startup_stm32f4xx.cmake
  /os/common/ports/ARMCMx/compilers/GCC/mk/port_v7m.cmake
  /os/hal/ports/STM32/STM32F4xx/platform.cmake
  )

ch_include(sbus2pwm_fw
  ${ch_include_board-${board}}
  /os/hal/hal.cmake
  /os/hal/osal/rt/osal.cmake
  /os/hal/lib/streams/streams.cmake
  /os/rt/rt.cmake
  )

fw_include_directories(sbus2pwm_fw
  ${SPACKET_ROOT}/src/common/include
  ${SPACKET_ROOT}/src/chibios/include
  conf-${board}
  conf-common
  .
  ${CH_ROOT_DIR}/os/license
  ${EXTERNAL}/SEGGER_RTT_V614c/RTT
  ${EXTERNAL}/boost_1_66_0
  ${EXTERNAL}/printf
  )

fw_sources(sbus2pwm_fw
  "${EXTERNAL}/SEGGER_RTT_V614c/RTT/SEGGER_RTT.c"
  "${EXTERNAL}/printf/printf.c"
  board-${board}.c
  main.cpp
  system_halt_hook.cpp
  )

set(JLINK_EXE
  /opt/SEGGER/JLink/JLinkExe
  CACHE
  FILEPATH
  "Path to JLinkExe binary on host system"
  )

set(JLINK_DEVICE_NAME ${JLINK_DEVICE_NAME-${board}})

set(JLINK_CONNECT_OPTIONS
  -device ${JLINK_DEVICE_NAME}
  -if SWD
  -speed 9000
  )

include(util)

util_join_cl(JLINK_CONNECT_OPTIONS_STR "" ${JLINK_CONNECT_OPTIONS})

fw_add_flash_target_to_exe(sbus2pwm_fw)
fw_add_jlink_connect()

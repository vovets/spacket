cmake_minimum_required(VERSION 3.5)
project(packet_loopback C CXX ASM)

get_filename_component(SPACKET_ROOT ../../.. ABSOLUTE)
set(SPACKET_EXT "${SPACKET_ROOT}/external")

list(APPEND CMAKE_MODULE_PATH "${SPACKET_ROOT}/cmake")

include(gather)
include(chibios)

set(GATHER_DEBUG FALSE)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CH_ROOT_DIR "${SPACKET_EXT}/chibios")
set(CH_MCU_NAME STM32F103x8)
set(CH_LINKER_LIBRARY_PATH "${CH_ROOT_DIR}/os/common/startup/ARMCMx/compilers/GCC/ld")
set(CH_LINKER_SCRIPT "${CH_LINKER_LIBRARY_PATH}/${CH_MCU_NAME}.ld")
set(CH_CPU cortex-m3)

set(mc_flags -mcpu=${CH_CPU})
set(thumb_flags -mthumb -mno-thumb-interwork)
set(thumb_defs -DTHUMB -DTHUMB_NO_INTERWORKING)
set(debug_flags -ggdb)
set(c_warn_flags -Wall -Wextra -Wundef -Wstrict-prototypes)
set(cxx_warn_flags -Wall -Wextra -Wundef -Wno-literal-suffix)
set(cxx_features_flags -fno-exceptions -fno-rtti)
set(debug_optimization_flags -Og)
set(release_optimization_flags -O0 -s)
set(linker_flags
  "${mc_flags}"
  "${thumb_flags}"
  -nostartfiles
  -ffunction-sections -fdata-sections -fno-common
  -flto
  "-Wl,--library-path,${CH_LINKER_LIBRARY_PATH}"
  "-Wl,--script,${CH_LINKER_SCRIPT}"
  -Wl,--defsym,__main_stack_size__=0x400
  -Wl,--defsym,__process_stack_size__=0x400
  -Wl,--gc-sections
  )

add_compile_options(
  "${mc_flags}"
  "${thumb_flags}"
  "${debug_flags}"
  "$<$<COMPILE_LANGUAGE:C>:${c_warn_flags}>"
  "$<$<COMPILE_LANGUAGE:CXX>:${cxx_warn_flags}>"
  "$<$<COMPILE_LANGUAGE:CXX>:${cxx_features_flags}>"
  "$<$<CONFIG:Debug>:${debug_optimization_flags}>"
  "$<$<CONFIG:Release>:${release_optimization_flags}>"
  )

add_definitions(
  ${thumb_defs}
  -DSHELL_CONFIG_FILE
  -DBOOST_STRICT_CONFIG
  )

add_executable(fw "")

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
  "${SPACKET_ROOT}/tests/target_utils/sources.cmake"
  )

target_include_directories(fw PUBLIC
  conf
  .
  ${CH_ROOT_DIR}/os/license
  "${SPACKET_EXT}/SEGGER_RTT_V614c/RTT"
  "${SPACKET_EXT}/boost_1_66_0"
  )

target_sources(fw PRIVATE
  "${SPACKET_EXT}/SEGGER_RTT_V614c/RTT/SEGGER_RTT.c"
  main.cpp
  system_halt_hook.cpp
  rtt_stream.cpp
  shell_commands.cpp
  )

ch_add_linker_flags(fw "${linker_flags}")

ch_add_hex_bin_targets(fw)

ch_add_size_output(fw)

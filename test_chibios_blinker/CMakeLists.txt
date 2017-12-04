cmake_minimum_required(VERSION 3.5)
project(blinker C CXX ASM)

list(APPEND CMAKE_MODULE_PATH ../cmake)

include(gather)
include(chibios)

set(GATHER_DEBUG FALSE)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")

set(mc_flags -mcpu=cortex-m3)
set(thumb_flags -mthumb -DTHUMB)
set(c_warn_flags -Wall -Wextra -Wundef -Wstrict-prototypes)
set(cxx_warn_flags -Wall -Wextra -Wundef -Wno-literal-suffix)

add_compile_options(
  "${mc_flags}"
  "${thumb_flags}"
  "$<$<COMPILE_LANGUAGE:C>:${c_warn_flags}>"
  "$<$<COMPILE_LANGUAGE:CXX>:${cxx_warn_flags}>")

set(CH_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../chibios")
set(CH_USE_LINK_GC TRUE)
set(CH_USE_LTO TRUE)

add_executable(blinker "")

ch_include(blinker
  /os/common/startup/ARMCMx/compilers/GCC/mk/startup_stm32f1xx.cmake
  /os/common/ports/ARMCMx/compilers/GCC/mk/port_v7m.cmake
  /os/hal/hal.cmake
  /os/hal/ports/STM32/STM32F1xx/platform.cmake
  /os/hal/osal/rt/osal.cmake
  /os/hal/lib/streams/streams.cmake
  /os/rt/rt.cmake
  /os/various/shell/shell.cmake
  /test/rt/test.cmake
  board/sources.cmake
  )

target_include_directories(blinker PUBLIC
  conf
  ${CH_ROOT_DIR}/os/license
  ../SEGGER_RTT_V614c/RTT
  )

target_sources(blinker PRIVATE
  main.cpp
  ../SEGGER_RTT_V614c/RTT/SEGGER_RTT.c
  )

set(LDSCRIPT "${CHIBIOS_ROOT}/os/common/startup/ARMCMx/compilers/GCC/ld/STM32f103x8.ld")

# if(CH_USE_LINK_GC)
#   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections -fno-common")
#   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections -fno-common")
# endif()

# if(CH_USE_LTO)
#   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto")
#   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
# endif()

cmake_minimum_required(VERSION 3.5)
project(blinker C CXX ASM)

# enable_language(ASM)

list(APPEND CMAKE_MODULE_PATH ../cmake)

include(gather)
include(chibios)

set(GATHER_DEBUG TRUE)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")

set(MCFLAGS -mcpu=cortex-m3)
set(TFLAGS "-mthumb -DTHUMB")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MCFLAGS} ${TFLAGS}")

set(CHIBIOS_ROOT "${CMAKE_CURRENT_LIST_DIR}/../chibios")

add_executable(blinker "")

ch_include(blinker /os/common/startup/ARMCMx/compilers/GCC/mk/startup_stm32f1xx.cmake)
ch_include(blinker board/sources.cmake)
ch_include(blinker /os/hal/hal.cmake)
ch_include(blinker /os/hal/ports/STM32/STM32F1xx/platform.cmake)
ch_include(blinker /os/hal/osal/rt/osal.cmake)
ch_include(blinker /os/rt/rt.cmake)
ch_include(blinker /os/common/ports/ARMCMx/compilers/GCC/mk/port_v7m.cmake)

target_include_directories(blinker PUBLIC
  conf
  ${CHIBIOS_ROOT}/os/license
  )

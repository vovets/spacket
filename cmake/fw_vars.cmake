set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED)

set(SPACKET_EXT "${SPACKET_ROOT}/external")
set(CH_ROOT_DIR "${SPACKET_EXT}/chibios")
set(CH_MCU_NAME STM32F103x8 CACHE STRING "MCU name for purposes of finding specific linker script, etc")
set(CH_LINKER_LIBRARY_PATH "${CH_ROOT_DIR}/os/common/startup/ARMCMx/compilers/GCC/ld")
set(CH_LINKER_SCRIPT "${CH_LINKER_LIBRARY_PATH}/${CH_MCU_NAME}.ld")
set(CH_CPU cortex-m3)

set(mc_flags -mcpu=${CH_CPU})
set(thumb_flags -mthumb -mno-thumb-interwork)
set(thumb_defs -DTHUMB -DTHUMB_NO_INTERWORKING)
set(debug_flags -ggdb)
set(c_warn_flags -Wall -Wextra -Wundef -Wstrict-prototypes)
set(cxx_warn_flags -Wall -Wextra -Wundef -Wno-literal-suffix)
set(cxx_features_flags -fno-exceptions -fno-rtti -fno-unwind-tables)
set(debug_optimization_flags -Og)
set(release_optimization_flags -Os)
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
  --specs=nano.specs
  )

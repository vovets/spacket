macro(fw_vars_require var)
    message("require: ${var}=${${var}}")
    if (NOT DEFINED ${var})
        message(FATAL_ERROR "var ${var} is required to be set")
    endif()
    string(CONFIGURE ${${var}} ${var})
    message("require: ${var}=${${var}}")
endmacro()

macro(fw_vars_default var)
    message("default: ${var}=${${var}} ${ARGN}")
    if(NOT DEFINED ${var})
        string(CONFIGURE "${ARGN}" ${var})
    endif()
    message("default: ${var}=${${var}}")
endmacro()

macro(fw_vars_set)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_C_STANDARD 99)
    set(CMAKE_C_STANDARD_REQUIRED)

    fw_vars_require(CH_CPU)
    fw_vars_default(mc_flags [[-mcpu=${CH_CPU}]])
    fw_vars_default(SPACKET_EXT [[${SPACKET_ROOT}/external]])
    fw_vars_require(CH_ROOT_DIR)
    fw_vars_default(CH_LINKER_LIBRARY_PATH "${CH_ROOT_DIR}/os/common/startup/ARMCMx/compilers/GCC/ld")
    fw_vars_require(CH_MCU_NAME)
    fw_vars_default(CH_LINKER_SCRIPT "${CH_LINKER_LIBRARY_PATH}/${CH_MCU_NAME}.ld")
    fw_vars_default(CH_MAIN_STACK_SIZE 0x400) # "Stack size (in hex) allocated for exceptions
    fw_vars_default(CH_PROCESS_STACK_SIZE 0x400) # "Stack size (in hex) allocated for main thread"
    fw_vars_default(CH_USE_FPU NO)
    fw_vars_default(CH_FLOAT_ABI hard) # "hard|softfp|soft"
    fw_vars_default(thumb_flags -mthumb -mno-thumb-interwork)
    fw_vars_default(thumb_defs -DTHUMB -DTHUMB_NO_INTERWORKING)
    fw_vars_default(fpu_flags -mfloat-abi=${CH_FLOAT_ABI} -mfpu=fpv4-sp-d16 -fsingle-precision-constant)
    fw_vars_default(debug_flags -ggdb)
    fw_vars_default(c_warn_flags -Wall -Wextra -Wundef -Wstrict-prototypes)
    fw_vars_default(cxx_warn_flags -Wall -Wextra -Wundef -Wno-literal-suffix -Wno-register)
    fw_vars_default(cxx_features_flags -fno-exceptions -fno-rtti -fno-unwind-tables -fno-use-cxa-atexit) # "list of cxx features flags like -fno-exceptions"
    fw_vars_default(debug_optimization_flags -Og) # "Optimization flags for debug build"
    fw_vars_default(release_optimization_flags -Os) # CACHE STRING "Optimization flags for release build"
    set(linker_flags
            "${mc_flags}"
            "${thumb_flags}"
            -nostartfiles
            -ffunction-sections -fdata-sections -fno-common
            -flto
            "-Wl,--library-path,${CH_LINKER_LIBRARY_PATH}"
            "-Wl,--script,${CH_LINKER_SCRIPT}"
            -Wl,--defsym,__main_stack_size__=${CH_MAIN_STACK_SIZE}
            -Wl,--defsym,__process_stack_size__=${CH_PROCESS_STACK_SIZE}
            -Wl,--gc-sections
            --specs=nano.specs
            )
endmacro()



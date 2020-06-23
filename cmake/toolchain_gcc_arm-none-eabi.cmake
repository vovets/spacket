set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

set(TRIPLE arm-none-eabi)
#set(TOOLCHAIN_PREFIX /usr)
set(TOOLCHAIN_PREFIX /home/vovka/opt/gcc-arm-none-eabi-9-2020-q2-update)
set(TOOLCHAIN_BIN ${TOOLCHAIN_PREFIX}/bin)

# Without that flag CMake is not able to pass test compilation check
# Actual fw may be built with other spect like --specs=nano.specs
set(CMAKE_EXE_LINKER_FLAGS_INIT "--specs=nosys.specs")

set(CMAKE_C_COMPILER "${TOOLCHAIN_BIN}/${TRIPLE}-gcc")
set(CMAKE_ASM_COMPILER "${TOOLCHAIN_BIN}/${TRIPLE}-gcc")
set(CMAKE_ASM_FLAGS "-x assembler-with-cpp")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_BIN}/${TRIPLE}-g++")

set(CMAKE_OBJCOPY ${TOOLCHAIN_BIN}/${TRIPLE}-objcopy CACHE INTERNAL "objcopy tool")
set(CMAKE_SIZE ${TOOLCHAIN_BIN}/${TRIPLE}-size CACHE INTERNAL "size tool")

# set(CMAKE_FIND_ROOT_PATH ${BINUTILS_PATH})
# set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
# set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

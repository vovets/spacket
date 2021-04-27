cmake_minimum_required(VERSION 3.5)

enable_testing()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")

if(NOT DEFINED SPACKET_ROOT)
  message(FATAL_ERROR "SPACKET_ROOT is not defined")
endif()

set(EXTERNAL ${SPACKET_ROOT}/external)

list(APPEND CMAKE_MODULE_PATH ${SPACKET_ROOT}/cmake)

include(build_type)
include(cpp_standard)
include(util)

add_compile_options(-Wno-unused-comparison)

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

util_join_cl(JLINK_CONNECT_OPTIONS_STR "" ${JLINK_CONNECT_OPTIONS})

set(SPACKET_FW_TOOLCHAIN_FILE
  "${SPACKET_ROOT}/cmake/toolchain_gcc_arm-none-eabi.cmake"
  CACHE
  FILEPATH
  "Toolchain file for building firmware projects"
  )

add_subdirectory(libhost_spacket)
add_subdirectory(libhost_utils)
add_subdirectory(libhost_crc)
add_subdirectory(test_host_various)
add_subdirectory(test_host_serial_loopback)
add_subdirectory(test_target_chibios)
add_subdirectory(test_target_loopback)
add_subdirectory(test_host_target_packet_echo)
add_subdirectory(test_host_target_packetizer_echo)
add_subdirectory(test_host_target_cobs_echo)
add_subdirectory(test_target_crc_device)
add_subdirectory(test_host_target_packet_device)
add_subdirectory(test_host_target_packet_crc)
add_subdirectory(test_host_target_multiplexer)
add_subdirectory(test_host_target_cph_uart)

cmake_minimum_required(VERSION 3.5)

include(ExternalProject)

enable_testing()

get_filename_component(SPACKET_ROOT .. ABSOLUTE)

function(util_join_cl var command)
  set(result "${command}")
  if(${ARGC} GREATER 1)
    foreach(x ${ARGN})
      set(result "${result} ${x}")
    endforeach()
  endif()
  set(${var} "${result}" PARENT_SCOPE)
endfunction()

add_subdirectory(host_spacket)
add_subdirectory(host_utils)
add_subdirectory(test_host_various)
add_subdirectory(test_host_serial_loopback)

ExternalProject_Add(test_chibios_blinker
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/test_chibios_blinker"
  BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/test_chibios_blinker"
  CMAKE_CACHE_ARGS "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${SPACKET_ROOT}/cmake/toolchain_gcc_arm-none-eabi.cmake"
  INSTALL_COMMAND ""
  USES_TERMINAL_BUILD 1
  BUILD_ALWAYS 1
  )


set(JLINK_EXE /opt/SEGGER/JLink/JLinkExe)
set(JLINK_CONNECT_OPTIONS
  -autoconnect 1
  -device STM32F103C8
  -if SWD
  -speed 4000
  )
set(JLINK_FLASH_OPTIONS
  -exitonerror 1
  )
set(JLINK_RTT_OPTIONS
  -exitonerror 0
  )
util_join_cl(JLINK_RTT_COMMAND
  ${JLINK_EXE}
  ${JLINK_CONNECT_OPTIONS}
  ${JLINK_RTT_OPTIONS}
  )

function(add_fw_flash_target fw_project)
  ExternalProject_Get_Property(${fw_project} BINARY_DIR)

  set(jlink_flash_hex "${BINARY_DIR}/fw.hex")
  set(jlink_flash_commandfile "${BINARY_DIR}/flash.jlink")
  configure_file(flash.jlink.in "${jlink_flash_commandfile}")
  set(target "${fw_project}_flash")
  add_custom_target(${target}
    ${JLINK_EXE} ${JLINK_CONNECT_OPTIONS} ${JLINK_FLASH_OPTIONS} -commandfile ${jlink_flash_commandfile}
    VERBATIM
    )
  add_dependencies(${target} ${fw_project})
endfunction()

function(add_jlink_test test_name)
  set(jlink_test_wrapper ${CMAKE_CURRENT_BINARY_DIR}/jlink_test_wrapper.sh)
  configure_file(jlink_test_wrapper.sh.in ${jlink_test_wrapper})
  add_test(NAME ${test_name}
    COMMAND sh -e ${jlink_test_wrapper}
    )
endfunction()

add_fw_flash_target(test_chibios_blinker)
add_jlink_test(chibios_blinker test_chibios_blinker)

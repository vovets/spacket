include(ExternalProject)

function(fw_tests_add_flash_target fw_project)
  ExternalProject_Get_Property(${fw_project} BINARY_DIR)

  set(jlink_flash_hex "${BINARY_DIR}/fw.hex")
  set(jlink_flash_commandfile "${BINARY_DIR}/flash.jlink")
  configure_file("${SPACKET_ROOT}/tests/misc/flash.jlink.in" "${jlink_flash_commandfile}")
  set(target "${fw_project}-flash")
  add_custom_target(${target}
    ${JLINK_EXE} ${JLINK_CONNECT_OPTIONS} ${JLINK_FLASH_OPTIONS} -commandfile ${jlink_flash_commandfile}
    VERBATIM
    )
  add_dependencies(${target} ${fw_project})
endfunction()

function(fw_tests_add_fw_subdirectory name source_dir)
  ExternalProject_Add(${name}
    SOURCE_DIR "${source_dir}"
    BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/${name}"
    CMAKE_ARGS "-DCMAKE_VERBOSE_MAKEFILE=${CMAKE_VERBOSE_MAKEFILE}" "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}" "-G${CMAKE_GENERATOR}"
    CMAKE_CACHE_ARGS "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${SPACKET_FW_TOOLCHAIN_FILE}"
    INSTALL_COMMAND ""
    USES_TERMINAL_BUILD 1
    BUILD_ALWAYS 1
    )
  ExternalProject_Add_StepTargets(${name} configure build)
  fw_tests_add_flash_target("${name}")
endfunction()

function(fw_tests_add_jlink_connect)
  set(jlink_connect ${CMAKE_CURRENT_BINARY_DIR}/jlink_connect.sh)
  configure_file("${SPACKET_ROOT}/tests/misc/jlink_connect.sh.in" ${jlink_connect})
endfunction()

function(fw_tests_add_jlink_test test_name fw_project host_test_executable)
  set(jlink_test_wrapper ${CMAKE_CURRENT_BINARY_DIR}/${test_name}.sh)
  configure_file("${SPACKET_ROOT}/tests/misc/jlink_test_wrapper.sh.in" ${jlink_test_wrapper})
  fw_tests_add_jlink_connect()
  add_test(NAME ${test_name}
    COMMAND sh -e ${jlink_test_wrapper}
    )
endfunction()

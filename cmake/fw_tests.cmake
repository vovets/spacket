include(ExternalProject)

function(fw_tests_add_jlink_connect)
  set(jlink_connect ${CMAKE_CURRENT_BINARY_DIR}/jlink_connect.sh)
  # message("3 SPACKET_ROOT=${SPACKET_ROOT}")
  configure_file("${SPACKET_ROOT}/tests/misc/jlink_connect.sh.in" ${jlink_connect})
endfunction()

function(fw_tests_add_jlink_test test_name fw_project host_test_executable)
  set(jlink_test_wrapper ${CMAKE_CURRENT_BINARY_DIR}/${test_name}.sh)
  # message("4 SPACKET_ROOT=${SPACKET_ROOT}")
  configure_file("${SPACKET_ROOT}/tests/misc/jlink_test_wrapper.sh.in" ${jlink_test_wrapper})
  fw_tests_add_jlink_connect()
  add_test(NAME ${test_name}
    COMMAND sh -e ${jlink_test_wrapper}
    )
endfunction()

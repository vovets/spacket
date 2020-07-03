cmake_minimum_required(VERSION 3.5)

if(NOT DEFINED SPACKET_ROOT)
  message(FATAL_ERROR "SPACKET_ROOT is not defined")
endif()

list(APPEND CMAKE_MODULE_PATH "${SPACKET_ROOT}/cmake")

include(fw_tests)

fw_add_fw_targets(test_target_loopback_fw "${CMAKE_CURRENT_SOURCE_DIR}/fw")

fw_tests_add_jlink_test(target_loopback
  test_target_loopback_fw
  "py.test-3 -vs ${CMAKE_CURRENT_SOURCE_DIR}/tests.py"
  )

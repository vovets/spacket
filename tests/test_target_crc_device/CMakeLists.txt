cmake_minimum_required(VERSION 3.5)

get_filename_component(SPACKET_ROOT ../.. ABSOLUTE)

list(APPEND CMAKE_MODULE_PATH "${SPACKET_ROOT}/cmake")

include(fw_tests)

fw_add_fw_targets(test_target_crc_device_fw "${CMAKE_CURRENT_SOURCE_DIR}/fw")

fw_tests_add_jlink_test(target_crc_device
  test_target_crc_device_fw
  "py.test-3 -vs ${CMAKE_CURRENT_SOURCE_DIR}/tests.py"
  )

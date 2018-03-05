cmake_minimum_required(VERSION 3.5)

get_filename_component(SPACKET_ROOT ../.. ABSOLUTE)

list(APPEND CMAKE_MODULE_PATH "${SPACKET_ROOT}/cmake")

include(fw_tests)

fw_tests_add_fw_subdirectory(test_target_loopback_fw "${CMAKE_CURRENT_SOURCE_DIR}/fw")

fw_tests_add_jlink_test(target_loopback_serial_ctor_dtor
  test_target_loopback_fw
  "python3 ${CMAKE_CURRENT_SOURCE_DIR}/test_ctor_dtor.py"
  )

fw_tests_add_jlink_test(target_loopback_rx_timeout_no_data
  test_target_loopback_fw
  "python3 ${CMAKE_CURRENT_SOURCE_DIR}/test_rx_timeout_no_data.py"
  )

fw_tests_add_jlink_test(target_loopback_packet_size_limit
  test_target_loopback_fw
  "python3 ${CMAKE_CURRENT_SOURCE_DIR}/test_packet_size_limit.py"
  )

fw_tests_add_jlink_test(target_loopback_rx_timeout
  test_target_loopback_fw
  "python3 ${CMAKE_CURRENT_SOURCE_DIR}/test_rx_timeout.py"
  )

fw_tests_add_jlink_test(target_loopback_idle_line
  test_target_loopback_fw
  "python3 ${CMAKE_CURRENT_SOURCE_DIR}/test_idle_line.py"
  )

fw_tests_add_jlink_test(target_loopback_info
  test_target_loopback_fw
  "python3 ${CMAKE_CURRENT_SOURCE_DIR}/test_info.py"
  )

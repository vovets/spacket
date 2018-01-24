cmake_minimum_required(VERSION 3.5)

get_filename_component(SPACKET_ROOT ../.. ABSOLUTE)

list(APPEND CMAKE_MODULE_PATH "${SPACKET_ROOT}/cmake")

include(fw_tests)

fw_tests_add_fw_subdirectory(test_target_packet_loopback "${CMAKE_CURRENT_SOURCE_DIR}/fw")
fw_tests_add_jlink_test(target_packet_loopback test_target_packet_loopback "python3 ${CMAKE_CURRENT_SOURCE_DIR}/test.py")

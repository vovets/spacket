cmake_minimum_required(VERSION 3.5)

get_filename_component(SPACKET_ROOT ../.. ABSOLUTE)

list(APPEND CMAKE_MODULE_PATH "${SPACKET_ROOT}/cmake")

include(fw_tests)

set(DEVICE_CONFIG_PATH "${CMAKE_CURRENT_SOURCE_DIR}/device_config.json")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/build_config.cpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/build_config.cpp"
)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(test_host_target_packet_crc tests.cpp "${CMAKE_CURRENT_BINARY_DIR}/build_config.cpp")

target_link_libraries(test_host_target_packet_crc spacket utils pthread)
#target_link_libraries(test_host_target_packet_crc asan spacket utils pthread)

target_compile_definitions(test_host_target_packet_crc PUBLIC
#  -DPACKET_DEVICE_ENABLE_DEBUG_PRINT
  -DDEBUG_PRINT_BUFFER_FULL
  )
#target_compile_options(test_host_target_packet_crc PUBLIC -fsanitize=address)

fw_tests_add_fw_subdirectory(test_host_target_packet_crc_fw "${CMAKE_CURRENT_SOURCE_DIR}/fw")

fw_tests_add_jlink_test(host_target_packet_crc
  test_host_target_packet_crc_fw
  "${CMAKE_CURRENT_BINARY_DIR}/test_host_target_packet_crc")

cmake_minimum_required(VERSION 3.5)

get_filename_component(SPACKET_ROOT ../.. ABSOLUTE)

list(APPEND CMAKE_MODULE_PATH "${SPACKET_ROOT}/cmake")

include(fw_tests)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")

set(DEVICE_CONFIG_PATH "${CMAKE_CURRENT_SOURCE_DIR}/device_config.json")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/build_config.cpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/build_config.cpp"
)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(test_host_target_packet_device tests.cpp "${CMAKE_CURRENT_BINARY_DIR}/build_config.cpp")

target_link_libraries(test_host_target_packet_device spacket utils pthread)

target_compile_definitions(test_host_target_packet_device PUBLIC
  # -DPACKET_DEVICE_ENABLE_DEBUG_PRINT
  -DDEBUG_PRINT_BUFFER_FULL
  )

fw_add_fw_targets(test_host_target_packet_device_fw "${CMAKE_CURRENT_SOURCE_DIR}/fw")

fw_tests_add_jlink_test(host_target_packet_device
  test_host_target_packet_device_fw
  "${CMAKE_CURRENT_BINARY_DIR}/test_host_target_packet_device")

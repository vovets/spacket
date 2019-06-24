cmake_minimum_required(VERSION 3.5)

get_filename_component(SPACKET_ROOT ../.. ABSOLUTE)
set(SPACKET_EXT "${SPACKET_ROOT}/external")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")

#set(Boost_USE_STATIC_LIBS ON)

#find_package(Boost REQUIRED program_options)

set(DEVICE_CONFIG_PATH "${CMAKE_CURRENT_SOURCE_DIR}/device_config.json")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/build_config.cpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/build_config.cpp"
)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(test_host_serial_loopback tests.cpp "${CMAKE_CURRENT_BINARY_DIR}/build_config.cpp")
#target_link_libraries(test1 spacket utils ${Boost_LIBRARIES})
target_link_libraries(test_host_serial_loopback spacket utils)

add_test(host_serial_loopback test_host_serial_loopback)

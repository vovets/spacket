cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")

#set(Boost_USE_STATIC_LIBS ON)

find_package(Boost REQUIRED program_options)

set(DEVICE_CONFIG_PATH "${CMAKE_CURRENT_SOURCE_DIR}/device_config.json")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/build_config.cpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/build_config.cpp"
)

include_directories(../json/src)
include_directories(../catch/include)
include_directories(../lib/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(utils utils.cpp catch_main.cpp "${CMAKE_CURRENT_BINARY_DIR}/build_config.cpp")
target_link_libraries(utils spacket)

add_executable(test1 test1.cpp)
#target_link_libraries(test1 spacket utils ${Boost_LIBRARIES})
target_link_libraries(test1 spacket utils)

add_executable(test_packetizer packetizer.cpp)
target_link_libraries(test_packetizer spacket utils)

add_executable(test_cobs cobs.cpp)
target_link_libraries(test_cobs spacket utils)

add_test(1 test1)

add_test(packetizer test_packetizer)

add_test(cobs test_cobs)

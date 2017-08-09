cmake_minimum_required(VERSION 3.5)

#set(Boost_USE_STATIC_LIBS ON)

find_package(Boost REQUIRED program_options)

set(DEVICE_CONFIG_PATH "${CMAKE_CURRENT_SOURCE_DIR}/device_config.json")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/build_config.cpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/build_config.cpp"
)

add_library(utils utils.cpp "${CMAKE_CURRENT_BINARY_DIR}/build_config.cpp")

add_executable(test1 test1.cpp test1_main.cpp)

#target_link_libraries(test1 spacket utils ${Boost_LIBRARIES})
target_link_libraries(test1 spacket utils)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

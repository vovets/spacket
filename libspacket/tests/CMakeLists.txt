cmake_minimum_required(VERSION 3.5)

set(Boost_USE_STATIC_LIBS ON)

find_package(Boost REQUIRED program_options)

add_executable(test1 test1.cpp)

target_link_libraries(test1 spacket ${Boost_LIBRARIES})

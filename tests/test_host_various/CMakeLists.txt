cmake_minimum_required(VERSION 3.5)

get_filename_component(SPACKET_ROOT ../.. ABSOLUTE)
set(SPACKET_EXT "${SPACKET_ROOT}/external")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")

add_executable(test_packetizer packetizer.cpp)
target_link_libraries(test_packetizer spacket utils)

add_executable(test_cobs cobs.cpp)
target_link_libraries(test_cobs spacket utils)

add_test(packetizer test_packetizer)

add_test(cobs test_cobs)

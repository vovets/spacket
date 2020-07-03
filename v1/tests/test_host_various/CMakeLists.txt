cmake_minimum_required(VERSION 3.5)

if(NOT DEFINED SPACKET_ROOT)
  message(FATAL_ERROR "SPACKET_ROOT is not defined")
endif()

set(SPACKET_EXT "${SPACKET_ROOT}/external")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")

add_executable(test_packetizer packetizer.cpp)
target_link_libraries(test_packetizer spacket utils)

add_executable(test_cobs cobs.cpp)
target_link_libraries(test_cobs spacket utils)

add_executable(test_packet_decode_fsm packet_decode_fsm.cpp)
target_link_libraries(test_packet_decode_fsm spacket utils)

add_executable(test_crc crc.cpp)
target_link_libraries(test_crc spacket utils crc)
target_compile_definitions(test_crc PUBLIC
  -DENABLE_DEBUG_PRINT
  -DDEBUG_PRINT_BUFFER_FULL
  )

add_test(host_packetizer test_packetizer)

add_test(host_cobs test_cobs)

add_test(host_packet_decode_fsm test_packet_decode_fsm)

add_test(host_crc test_crc)

cmake_minimum_required(VERSION 3.5)

enable_testing()

add_subdirectory(host_spacket)
add_subdirectory(host_utils)
add_subdirectory(test_host_various)
add_subdirectory(test_host_serial_loopback)
add_subdirectory(test_target_chibios)

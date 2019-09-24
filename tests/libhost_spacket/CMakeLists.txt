cmake_minimum_required(VERSION 3.5)

get_filename_component(SPACKET_ROOT ../.. ABSOLUTE)

list(APPEND CMAKE_MODULE_PATH "${SPACKET_ROOT}/cmake")

include(gather)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(spacket "")

target_include_directories(spacket PUBLIC ${SPACKET_ROOT}/external/boost_1_66_0)
#target_compile_definitions(spacket PUBLIC -DSPACKET_ENABLE_TRACE)
#target_compile_options(spacket PUBLIC -fsanitize=address)
target_link_libraries(spacket crc)

gt_include(spacket ${SPACKET_ROOT}/lib/sources.cmake)
gt_include(spacket ${SPACKET_ROOT}/lib/linux/sources.cmake)

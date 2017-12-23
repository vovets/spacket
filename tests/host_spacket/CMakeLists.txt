cmake_minimum_required(VERSION 3.5)

get_filename_component(SPACKET_ROOT ../.. ABSOLUTE)

list(APPEND CMAKE_MODULE_PATH "${SPACKET_ROOT}/cmake")

include(gather)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(spacket "")

gt_include(spacket ${SPACKET_ROOT}/lib/sources.cmake)
gt_include(spacket ${SPACKET_ROOT}/lib/linux/sources.cmake)

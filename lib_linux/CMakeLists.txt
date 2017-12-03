cmake_minimum_required(VERSION 3.5)

list(APPEND CMAKE_MODULE_PATH ../cmake)

include(gather)

set(GATHER_DEBUG TRUE)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")

add_library(spacket "")

gt_include(spacket ../lib/sources.cmake)
gt_include(spacket ../lib/linux/sources.cmake)


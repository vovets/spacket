cmake_minimum_required(VERSION 3.5)

get_filename_component(SPACKET_ROOT ../.. ABSOLUTE)
set(SPACKET_EXT "${SPACKET_ROOT}/external")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${SPACKET_EXT}/json/src)

add_library(utils utils.cpp catch_main.cpp)
target_link_libraries(utils spacket)
target_include_directories(utils
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${SPACKET_EXT}/catch/include
  )

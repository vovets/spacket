cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${EXTERNAL}/json/src)

add_library(utils utils.cpp catch_main.cpp)
target_link_libraries(utils spacket)
target_include_directories(utils
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${EXTERNAL}/catch/include
  )
#target_compile_options(utils PUBLIC -fsanitize=address)

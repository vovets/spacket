cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")

string(TOLOWER ${CMAKE_SYSTEM_NAME} PLATFORM_ID)

add_subdirectory(${PLATFORM_ID})

set(COMMON_SRCS errors.cpp config.cpp config_utils.cpp cobs/cobs.c)

include_directories(include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(spacket ${COMMON_SRCS} ${PLATFORM_SRCS})

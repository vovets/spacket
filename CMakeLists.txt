cmake_minimum_required(VERSION 3.5)

project(test1)

set(Boost_USE_STATIC_LIBS ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")

find_package(Boost REQUIRED program_options)

string(TOLOWER ${CMAKE_SYSTEM_NAME} PLATFORM_ID)

add_subdirectory(${PLATFORM_ID})

set(COMMON_SRCS test1.cpp errors.cpp config_utils.cpp)

add_executable(test1 ${COMMON_SRCS} ${PLATFORM_SRCS})

include_directories(${PROJECT_SOURCE_DIR})

target_link_libraries(test1 ${Boost_LIBRARIES})

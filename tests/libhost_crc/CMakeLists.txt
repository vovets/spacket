cmake_minimum_required(VERSION 3.5)

get_filename_component(SPACKET_ROOT ../.. ABSOLUTE)
set(SPACKET_EXTERNAL ${SPACKET_ROOT}/external)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/crc.cpp ${CMAKE_CURRENT_BINARY_DIR}/crc.cpp.h
  COMMAND python3 ${SPACKET_EXTERNAL}/pycrc/pycrc.py --model crc-32-mpeg --algorithm table-driven --table-idx-width 8 --generate h -o ${CMAKE_CURRENT_BINARY_DIR}/crc.cpp.h
  COMMAND python3 ${SPACKET_EXTERNAL}/pycrc/pycrc.py --model crc-32-mpeg --algorithm table-driven --table-idx-width 8 --generate c -o ${CMAKE_CURRENT_BINARY_DIR}/crc.cpp
  )

add_custom_target(crc_sources
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/crc.cpp.h ${CMAKE_CURRENT_BINARY_DIR}/crc.cpp
  )

add_library(crc
  ${CMAKE_CURRENT_BINARY_DIR}/crc.cpp.h
  ${CMAKE_CURRENT_BINARY_DIR}/crc.cpp
  )

add_dependencies(crc crc_sources)

target_include_directories(crc PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

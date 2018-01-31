gt_add_sources(
  fatal_error.cpp
  )

if(NOT serial_device IN_LIST SPACKET_EXCLUDED_FEATURES)
  gt_add_sources(
    serial_device_impl.cpp
    )
endif()
  
gt_add_include_directories(
  include
  )

function(gt_debug)
  foreach(message_ ${ARGV})
    if(GATHER_DEBUG)
      message(STATUS "[GATHER_DEBUG] ${message_}")
    endif()
  endforeach()
endfunction()

function(gt_absolutize)
  unset(result)
  foreach(path ${ARGV})
    if(IS_ABSOLUTE "${path}")
      list(APPEND result "${path}")
    else()
      list(APPEND result "${CMAKE_CURRENT_LIST_DIR}/${path}")
    endif()
  endforeach()
  set(result ${result} PARENT_SCOPE)
endfunction()

function(gt_add_sources)
  unset(result)
  gt_absolutize("${ARGV}")
  target_sources(${target} PRIVATE ${result})
endfunction()

function(gt_add_include_directories)
  unset(result)
  gt_absolutize("${ARGV}")
  target_include_directories(${target} PUBLIC ${result})
endfunction()

function(gt_include target)
  unset(result)
  foreach(path ${ARGN})
    gt_absolutize("${path}")
    gt_debug("including ${result}")
    include("${result}")
  endforeach()
endfunction()

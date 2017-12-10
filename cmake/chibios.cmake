function(ch_absolutize)
  unset(result)
  foreach(path ${ARGV})
    if(IS_ABSOLUTE "${path}")
      list(APPEND result "${CH_ROOT_DIR}/${path}")
    else()
      list(APPEND result "${path}")
    endif()
  endforeach()
  gt_absolutize("${result}")
  set(result "${result}" PARENT_SCOPE)
endfunction()

function(ch_add_sources)
  unset(result)
  ch_absolutize("${ARGV}")
  gt_add_sources(${result})
endfunction()

function(ch_add_include_directories)
  unset(result)
  ch_absolutize("${ARGV}")
  gt_add_include_directories(${result})
endfunction()

function(ch_include target)
  unset(result)
  ch_absolutize("${ARGN}")
  gt_include(${target} "${result}")
endfunction()

function(ch_add_linker_flags target linker_flags)
  get_target_property(dir ${target} BINARY_DIR)
  target_link_libraries(${target}
    "${linker_flags}"
    "-Wl,-Map,${dir}/${target}.map"
    -Wl,--cref
    )
endfunction()

function(ch_add_hex_bin_targets target)
  get_target_property(dir ${target} BINARY_DIR)
  add_custom_target(${target}.hex ALL DEPENDS ${target} COMMAND ${CMAKE_OBJCOPY} -Oihex "${dir}/${target}" "${dir}/${target}.hex")
  add_custom_target(${target}.bin ALL DEPENDS ${target} COMMAND ${CMAKE_OBJCOPY} -Obinary "${dir}/${target}" "${dir}/${target}.bin")
endfunction()

function(ch_add_size_output target)
  get_target_property(dir ${target} BINARY_DIR)
  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_SIZE} "${dir}/${target}"
    )
endfunction()

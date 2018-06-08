function(fw_add_executable name)
  add_executable(${name} "")
  target_compile_options(${name} PUBLIC
    "${mc_flags}"
    "${thumb_flags}"
    "${debug_flags}"
    "$<$<COMPILE_LANGUAGE:C>:${c_warn_flags}>"
    "$<$<COMPILE_LANGUAGE:CXX>:${cxx_warn_flags}>"
    "$<$<COMPILE_LANGUAGE:CXX>:${cxx_features_flags}>"
    "$<$<CONFIG:Debug>:${debug_optimization_flags}>"
    "$<$<CONFIG:Release>:${release_optimization_flags}>"
    )
  target_compile_definitions(${name} PUBLIC
    ${thumb_defs}
    )
  ch_add_linker_flags(${name} "${linker_flags}")
  ch_add_hex_bin_targets(${name})
  ch_add_size_output(${name})
endfunction()

function(fw_include_directories name)
  target_include_directories(${name} PUBLIC ${ARGN})
endfunction()

function(fw_compile_definitions name)
  target_compile_definitions(${name} PUBLIC ${ARGN})
endfunction()

function(fw_sources name)
  target_sources(${name} PUBLIC ${ARGN})
endfunction()

include(ExternalProject)

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
     $<$<BOOL:${CH_USE_FPU}>:${fpu_flags}>
    )
  target_compile_definitions(${name} PUBLIC
    ${thumb_defs}
    $<$<BOOL:${CH_USE_FPU}>:CORTEX_USE_FPU=TRUE>
    $<$<NOT:$<BOOL:${CH_USE_FPU}>>:CORTEX_USE_FPU=FALSE>
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
  target_sources(${name} PRIVATE ${ARGN})
endfunction()

function(fw_add_flash_target fw_project)
  ExternalProject_Get_Property(${fw_project} BINARY_DIR)

  set(jlink_flash_hex "${BINARY_DIR}/fw.hex")
  set(jlink_flash_commandfile "${BINARY_DIR}/flash.jlink")
  configure_file("${SPACKET_ROOT}/misc/flash.jlink.in" "${jlink_flash_commandfile}")
  set(target "${fw_project}-flash")
  add_custom_target(${target}
    ${JLINK_EXE} ${JLINK_CONNECT_OPTIONS} ${JLINK_FLASH_OPTIONS} -commandfile ${jlink_flash_commandfile}
    VERBATIM
    )
  add_dependencies(${target} ${fw_project})
endfunction()

function(fw_add_flash_target_to_exe fw_executable)
  get_target_property(dir ${fw_executable} BINARY_DIR)
  set(jlink_flash_hex "${dir}/${fw_executable}.hex")
  set(jlink_flash_commandfile "${dir}/flash.jlink")
  configure_file("${SPACKET_ROOT}/misc/flash.jlink.in" "${jlink_flash_commandfile}")
  set(target "${fw_executable}-flash")
  add_custom_target(${target}
    ${JLINK_EXE} ${JLINK_CONNECT_OPTIONS} ${JLINK_FLASH_OPTIONS} -commandfile ${jlink_flash_commandfile}
    VERBATIM
    )
  add_dependencies(${target} ${fw_executable}.hex)
endfunction()

# As one cannot have two different compilers in one cmake tree,
# this is used to add fw project via ExternalProject machinery to host project.
#
function(fw_add_fw_targets name source_dir)
  ExternalProject_Add(${name}
    SOURCE_DIR "${source_dir}"
    BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/${name}"
    CMAKE_ARGS "-DCMAKE_VERBOSE_MAKEFILE=${CMAKE_VERBOSE_MAKEFILE}" "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}" "-G${CMAKE_GENERATOR}"
    CMAKE_CACHE_ARGS "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${SPACKET_FW_TOOLCHAIN_FILE}"
    INSTALL_COMMAND ""
    USES_TERMINAL_BUILD 1
    BUILD_ALWAYS 1
    )
  ExternalProject_Add_StepTargets(${name} configure build)
  fw_add_flash_target("${name}")
endfunction()

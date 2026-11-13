# Tests.  tests/unit, tests/golden and tests/lint belong to the foundation (K0).  Other test
# directories (litmus, mutation, property, recall, cfs, ...) bring their own CMakeLists.txt
# and are added when present.
add_subdirectory(unit)
add_subdirectory(golden)
add_subdirectory(lint)

file(GLOB kz_test_files RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" CONFIGURE_DEPENDS "*/CMakeLists.txt")
list(SORT kz_test_files)
foreach(kz_file IN LISTS kz_test_files)
  get_filename_component(kz_dir "${kz_file}" DIRECTORY)
  if(NOT kz_dir MATCHES "^(unit|golden|lint)$")
    add_subdirectory("${kz_dir}")
  endif()
endforeach()

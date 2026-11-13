# Unit tests of libkz-core.  Convention: tests/unit/test_core_<name>.cpp is built and
# registered automatically.  Tests of other components are registered by those components
# (kz_add_unit_test in their own CMakeLists.txt) and are named test_<component>_<name>.cpp.

add_library(kz-testsupport STATIC support/fixtures.cpp support/fixtures_data.cpp support/source_identity.cpp)
target_include_directories(kz-testsupport PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(kz-testsupport PUBLIC kz-core)
kz_apply_settings(kz-testsupport)

# The schema header as plain C: C99 with -pedantic, and C11.  The two objects export flat
# layout tables that test_core_schema compares with the C++ view.
foreach(kz_std 99 11)
  add_library(kz_schema_c${kz_std} OBJECT schema_c.c)
  set_target_properties(kz_schema_c${kz_std} PROPERTIES C_STANDARD ${kz_std} C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
  target_compile_definitions(kz_schema_c${kz_std} PRIVATE KZ_C_PFX=kz_c${kz_std}_)
  target_include_directories(kz_schema_c${kz_std} PRIVATE "${CMAKE_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}")
  kz_apply_settings(kz_schema_c${kz_std})
endforeach()

kz_add_unit_test(test_core_schema
  SOURCES test_core_schema.cpp $<TARGET_OBJECTS:kz_schema_c99> $<TARGET_OBJECTS:kz_schema_c11>
  LIBS kz-testsupport)

# The tool hash and the source identity: the test recomputes the identity of libkz-core from the
# files of the tree and compares it with the one CMake compiled into the library.
list(JOIN KZ_CORE_SOURCE_PATHS ":" kz_core_paths_joined)
kz_add_unit_test(test_core_identity SOURCES test_core_identity.cpp LIBS kz-testsupport)
target_compile_definitions(test_core_identity PRIVATE
  "KZ_SOURCE_ROOT_DIR=\"${KZ_SOURCE_ROOT}\"" "KZ_CORE_SOURCE_PATHS_TEXT=\"${kz_core_paths_joined}\"")

# The stand-alone dump tool, run as a program.  It prints its own tool hash and source
# identities (--identity); the test recomputes the identity of the tool from the paths that
# kz_tool_identity() recorded on the target.
kz_add_unit_test(test_core_kzdump SOURCES test_core_kzdump.cpp LIBS kz-testsupport ARGS $<TARGET_FILE:kz-dump>)
target_compile_definitions(test_core_kzdump PRIVATE
  "KZ_SOURCE_ROOT_DIR=\"${KZ_SOURCE_ROOT}\"" "KZ_KZDUMP_IDENTITY_PATHS=\"$<TARGET_PROPERTY:kz-dump,KZ_TOOL_IDENTITY_PATHS>\"")
add_dependencies(test_core_kzdump kz-dump)

# cmake/KzSourceIdentity.cmake: the identity depends on the content, the names and the set of
# files, and on nothing else (not on the place of the tree, hidden files or backup files).
add_test(NAME source_identity_cmake
  COMMAND ${CMAKE_COMMAND} -DKZ_MODULE=${KZ_SOURCE_ROOT}/cmake/KzSourceIdentity.cmake -DWORK=${CMAKE_CURRENT_BINARY_DIR}/source_identity_work
          -P ${CMAKE_CURRENT_SOURCE_DIR}/check_source_identity.cmake)
set_tests_properties(source_identity_cmake PROPERTIES LABELS unit)

file(GLOB kz_core_tests RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" CONFIGURE_DEPENDS "test_core_*.cpp")
list(SORT kz_core_tests)
foreach(kz_file IN LISTS kz_core_tests)
  get_filename_component(kz_name "${kz_file}" NAME_WE)
  if(NOT TARGET ${kz_name})
    kz_add_unit_test(${kz_name} SOURCES ${kz_file} LIBS kz-testsupport)
  endif()
endforeach()

# The schema header from a BPF program (the observer's kernel side includes it).  Needs a
# clang with the bpf target; the test is skipped from the build when there is none.
find_program(KZ_BPF_CLANG clang)
if(KZ_BPF_CLANG)
  execute_process(COMMAND ${KZ_BPF_CLANG} -print-targets OUTPUT_VARIABLE kz_clang_targets ERROR_QUIET)
  if(kz_clang_targets MATCHES "bpf")
    add_test(NAME schema_bpf_compile
      COMMAND ${KZ_BPF_CLANG} -target bpf -ffreestanding -O2 -Wall -Wextra -Werror -std=c11
              -I${CMAKE_SOURCE_DIR}/include -c ${CMAKE_CURRENT_SOURCE_DIR}/schema_bpf.c -o schema_bpf.o)
    set_tests_properties(schema_bpf_compile PROPERTIES LABELS unit)
  else()
    message(STATUS "clang has no bpf target: schema_bpf_compile not registered")
  endif()
else()
  message(STATUS "no clang found: schema_bpf_compile not registered")
endif()

# The layout assertions must fire: a record with padding is rejected by the C++ and by the
# C99 compiler (and, as a control, a record without padding compiles).
set(kz_probe_regex "kz_probe_record_(size_is_twin_size|size_is_field_sum|off_[a-z]+|align_[a-z]+)")
foreach(kz_case 0 1 2)
  if(kz_case EQUAL 0)
    set(kz_expect PASS)
  else()
    set(kz_expect FAIL)
  endif()
  add_test(NAME schema_probe_${kz_case}_cxx
    COMMAND ${CMAKE_COMMAND} -DCOMPILER=${CMAKE_CXX_COMPILER} "-DFLAGS=-x;c++;-std=c++17;-DKZ_PROBE_CASE=${kz_case}"
            -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/schema_probe.c -DINCLUDE=${CMAKE_SOURCE_DIR}/include
            -DEXPECT=${kz_expect} "-DREGEX=${kz_probe_regex}" -P ${CMAKE_CURRENT_SOURCE_DIR}/check_compile.cmake)
  add_test(NAME schema_probe_${kz_case}_c99
    COMMAND ${CMAKE_COMMAND} -DCOMPILER=${CMAKE_C_COMPILER} "-DFLAGS=-std=c99;-pedantic;-Wall;-Werror;-DKZ_PROBE_CASE=${kz_case}"
            -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/schema_probe.c -DINCLUDE=${CMAKE_SOURCE_DIR}/include
            -DEXPECT=${kz_expect} "-DREGEX=${kz_probe_regex}" -P ${CMAKE_CURRENT_SOURCE_DIR}/check_compile.cmake)
  set_tests_properties(schema_probe_${kz_case}_cxx schema_probe_${kz_case}_c99 PROPERTIES LABELS unit)
endforeach()

# Tests of the observer that need no framework: the decoder, the identity join, the ELF/DWARF
# reader, the profile reader, the loader against a small purpose-built program.  Everything
# here is a unit-test value, not a research condition (CONDITIONS_POLICY).
include_directories("${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/tests/unit")

file(GLOB kz_trace_tests RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" CONFIGURE_DEPENDS "test_trace_*.cpp")
list(SORT kz_trace_tests)
foreach(kz_file IN LISTS kz_trace_tests)
  get_filename_component(kz_name "${kz_file}" NAME_WE)
  kz_add_unit_test(${kz_name} SOURCES ${kz_file} LIBS kz-trace kz-testsupport)
endforeach()

# The purpose-built target program.  Every function starts `endbr64; push %rbp; mov %rsp,%rbp`
# (frame pointers, control-flow protection) like the framework's functions at -O0, whatever the
# sanitizer flags of the build.
find_package(Threads REQUIRED)
add_executable(target_prog target/target_prog.c)
target_compile_options(target_prog PRIVATE -g -O0 -fno-omit-frame-pointer -fcf-protection=full)
set_target_properties(target_prog PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(target_prog PRIVATE Threads::Threads)

# The same program built by the other compiler and with DWARF 4, as inputs for the reader tests.
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  find_program(KZ_ALT_CC NAMES clang clang-18)
else()
  find_program(KZ_ALT_CC NAMES gcc gcc-13)
endif()
set(kz_alt_binaries "")
if(KZ_ALT_CC)
  add_custom_command(OUTPUT target_prog_alt
    COMMAND ${KZ_ALT_CC} -g -O0 -fno-omit-frame-pointer -fcf-protection=full -fPIE -pie -pthread -o target_prog_alt ${CMAKE_CURRENT_SOURCE_DIR}/target/target_prog.c
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/target/target_prog.c VERBATIM)
  list(APPEND kz_alt_binaries target_prog_alt)
endif()
add_custom_command(OUTPUT target_prog_dwarf4
  COMMAND ${CMAKE_C_COMPILER} -g -gdwarf-4 -O0 -fno-omit-frame-pointer -fcf-protection=full -fPIE -pie -pthread -o target_prog_dwarf4 ${CMAKE_CURRENT_SOURCE_DIR}/target/target_prog.c
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/target/target_prog.c VERBATIM)
list(APPEND kz_alt_binaries target_prog_dwarf4)
add_custom_target(target_prog_variants DEPENDS ${kz_alt_binaries})

set(kz_probe_libs kz-loader kz-plan kz-anchor kz-trace kz-testsupport)
file(GLOB kz_probe_tests RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" CONFIGURE_DEPENDS "test_probe_*.cpp")
list(SORT kz_probe_tests)
foreach(kz_file IN LISTS kz_probe_tests)
  get_filename_component(kz_name "${kz_file}" NAME_WE)
  kz_add_unit_test(${kz_name} SOURCES ${kz_file} LIBS ${kz_probe_libs}
    ARGS $<TARGET_FILE:target_prog> ${CMAKE_CURRENT_BINARY_DIR}/target_prog_dwarf4 ${CMAKE_CURRENT_BINARY_DIR}/target_prog_alt
         ${CMAKE_CURRENT_SOURCE_DIR}/target/target.kzp ${CMAKE_CURRENT_SOURCE_DIR}/target/target_prog.c ${CMAKE_SOURCE_DIR}
         $<TARGET_FILE:kz> ${CMAKE_CURRENT_BINARY_DIR})
  add_dependencies(${kz_name} target_prog target_prog_variants kz)
  target_include_directories(${kz_name} PRIVATE "${CMAKE_SOURCE_DIR}/probe/anchor" "${CMAKE_SOURCE_DIR}/probe/plan" "${CMAKE_SOURCE_DIR}/probe/loader")
endforeach()

# test_probe_golden compares canonical dumps with tests/cfs-free/golden (accept a deliberate change
# with KZ_GOLDEN_UPDATE=1 ctest -L golden).  test_probe_observe attaches real uprobes: it needs
# root and exits 77 (reported as skipped, never as passed) where the kernel refuses; each
# observation spends about 0.1 s per probe event on removing the probes, so it takes tens of seconds.
set_tests_properties(test_probe_golden PROPERTIES LABELS "unit;golden")
set_tests_properties(test_probe_observe PROPERTIES LABELS "unit;integration" SKIP_RETURN_CODE 77 TIMEOUT 900)

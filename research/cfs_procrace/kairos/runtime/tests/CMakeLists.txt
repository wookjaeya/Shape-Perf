# Runtime unit tests (label "runtime").  Small programs: each runs in its own process because the
# runtime is a static singleton.  Values in them are unit-test values, not research conditions.
function(kzrt_test name)
  cmake_parse_arguments(T "" "" "SOURCES;LIBS;ARGS" ${ARGN})
  if(NOT TARGET ${name})
    add_executable(${name} ${T_SOURCES})
    target_link_libraries(${name} PRIVATE ${T_LIBS})
    target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    set_target_properties(${name} PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
    kzrt_settings(${name})
  endif()
endfunction()

function(kzrt_case test exe)
  add_test(NAME ${test} COMMAND ${exe} ${ARGN})
  set_tests_properties(${test} PROPERTIES LABELS runtime TIMEOUT 60)
endfunction()

set(kzrt_test_libs kzrt-posix kzrt kzrt-decode)
kzrt_test(test_rt_bootstrap SOURCES test_rt_bootstrap.c LIBS ${kzrt_test_libs})
kzrt_test(test_rt_order SOURCES test_rt_order.c LIBS ${kzrt_test_libs})
kzrt_test(test_rt_loss SOURCES test_rt_loss.c LIBS ${kzrt_test_libs})
kzrt_test(test_rt_serial SOURCES test_rt_serial.c LIBS ${kzrt_test_libs})
kzrt_test(test_rt_gate SOURCES test_rt_gate.c LIBS ${kzrt_test_libs})
kzrt_test(test_rt_holdsem SOURCES test_rt_holdsem.c LIBS ${kzrt_test_libs})
kzrt_test(test_rt_manifest SOURCES test_rt_manifest.cpp LIBS kzrt-manifest)
kzrt_test(test_rt_auto SOURCES test_rt_auto.c LIBS kzrt-posix-auto kzrt-posix kzrt kzrt-decode)

kzrt_case(rt_bootstrap test_rt_bootstrap)
kzrt_case(rt_order test_rt_order)
kzrt_case(rt_loss test_rt_loss)
kzrt_case(rt_serial test_rt_serial ${CMAKE_CURRENT_BINARY_DIR}/rt_serial.kzrt)
set_tests_properties(rt_serial PROPERTIES FIXTURES_SETUP rt_serial_dump)
foreach(s separation record context isr nowait timeout preattach)
  kzrt_case(rt_gate_${s} test_rt_gate ${s})
endforeach()
foreach(s rearm nth defer busy)
  kzrt_case(rt_holdsem_${s} test_rt_holdsem ${s})
endforeach()
kzrt_case(rt_manifest test_rt_manifest ${CMAKE_CURRENT_SOURCE_DIR}/data)

# kz-rt-dump on the round-trip dump: the checks pass with the example manifest, and the string
# copies are reported when the manifest marks that slot pointer-only.
kzrt_case(rt_dump_tool kz-rt-dump --manifest ${CMAKE_CURRENT_SOURCE_DIR}/data/sites_example.kzs ${CMAKE_CURRENT_BINARY_DIR}/rt_serial.kzrt)
set_tests_properties(rt_dump_tool PROPERTIES FIXTURES_REQUIRED rt_serial_dump
  PASS_REGULAR_EXPRESSION "check actor 1 aseq 1\\.\\.6 missing=0 overflow_counted=0.*module 6 \\(never_loaded\\.so\\) never registered.*check ok")
kzrt_case(rt_dump_tool_strict kz-rt-dump --manifest ${CMAKE_CURRENT_SOURCE_DIR}/data/sites_pointer_only.kzs ${CMAKE_CURRENT_BINARY_DIR}/rt_serial.kzrt)
set_tests_properties(rt_dump_tool_strict PROPERTIES FIXTURES_REQUIRED rt_serial_dump
  PASS_REGULAR_EXPRESSION "string copied from slot 0, which the manifest does not mark string=validated.*check FAILED \\(2 failures\\)")

# The start-up object: a program that never calls the runtime's setup gets a dump at exit.
kzrt_case(rt_auto test_rt_auto)
set_tests_properties(rt_auto PROPERTIES FIXTURES_SETUP rt_auto_dump
  ENVIRONMENT "KZ_RT_DUMP=${CMAKE_CURRENT_BINARY_DIR}/rt_auto.kzrt;KZ_RT_MODE=record;KZ_RT_RUN_ID=42;KZ_RT_INSTANCE=1")
kzrt_case(rt_auto_dump kz-rt-dump ${CMAKE_CURRENT_BINARY_DIR}/rt_auto.kzrt)
set_tests_properties(rt_auto_dump PROPERTIES FIXTURES_REQUIRED rt_auto_dump
  PASS_REGULAR_EXPRESSION "run abi=1 mode=stopped run_id=42 instance=1 .*ev actor=1 aseq=3 .*check ok")

# The core references no library symbol (freestanding claim of kz_rt.h).
if(NOT KZ_SANITIZE AND CMAKE_NM)
  add_test(NAME rt_no_library_symbols COMMAND ${CMAKE_COMMAND} -DNM=${CMAKE_NM} -DLIB=$<TARGET_FILE:kzrt> -P ${CMAKE_CURRENT_SOURCE_DIR}/check_undefined.cmake)
  set_tests_properties(rt_no_library_symbols PROPERTIES LABELS runtime)
endif()

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
kzrt_test(test_rt_pool SOURCES test_rt_pool.c LIBS ${kzrt_test_libs})
kzrt_test(test_rt_mode SOURCES test_rt_mode.c LIBS kzrt-posix-auto kzrt-posix kzrt kzrt-decode)

# The sha256 of the example manifest goes into the serial dump, so that kz-rt-dump --manifest finds it.
file(SHA256 ${CMAKE_CURRENT_SOURCE_DIR}/data/sites_example.kzs kzrt_example_sha)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/data/sites_example.kzs)
target_compile_definitions(test_rt_serial PRIVATE KZRT_TEST_MANIFEST_SHA="${kzrt_example_sha}")

# A small pool (4 chunks) for the pool-exhaustion case: the core built again with other size macros
# (a build parameter; the test sees the same macros).  Unit-test sizes, not research conditions.
add_library(kzrt-pool4 STATIC ${CMAKE_CURRENT_SOURCE_DIR}/../src/kz_rt.c ${CMAKE_CURRENT_SOURCE_DIR}/../src/kz_rt_plan.c)
target_include_directories(kzrt-pool4 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_compile_definitions(kzrt-pool4 PUBLIC KZ_RT_POOL_BYTES=16384u KZ_RT_CHUNK_BYTES=4096u)
target_compile_options(kzrt-pool4 PRIVATE -ffreestanding -fno-stack-protector "$<$<C_COMPILER_ID:GNU>:-fno-tree-loop-distribute-patterns>")
set_target_properties(kzrt-pool4 PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF POSITION_INDEPENDENT_CODE ON)
kzrt_settings(kzrt-pool4)
kzrt_test(test_rt_pool4 SOURCES test_rt_pool.c LIBS kzrt-pool4 kzrt-decode Threads::Threads)

# The build identity of the example manifest, given as an instrumented build would (unit-test values).
kzrt_build_info(kzrt-build-info-test BACKEND "unit-test backend" COMPILER "${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}"
                TARGET "unit-test target" MANIFEST ${CMAKE_CURRENT_SOURCE_DIR}/data/sites_example.kzs)
# ... and one with the defaults (no COMPILER: the compiler's __VERSION__; no TARGET) and a sha256 that is not hex.
add_library(kzrt-build-info-bad OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/../src/kz_rt_build_info.c)
target_include_directories(kzrt-build-info-bad PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_compile_definitions(kzrt-build-info-bad PRIVATE "KZ_RT_BUILD_BACKEND=\"unit-test backend\""
  "KZ_RT_BUILD_MANIFEST_SHA256=\"zz00000000000000000000000000000000000000000000000000000000000000\"")
set_target_properties(kzrt-build-info-bad PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF POSITION_INDEPENDENT_CODE ON)
kzrt_test(test_rt_auto_bad SOURCES test_rt_auto.c LIBS kzrt-build-info-bad kzrt-posix-auto kzrt-posix kzrt kzrt-decode)
kzrt_test(test_rt_mode_api SOURCES test_rt_mode_api.c LIBS kzrt-build-info-test ${kzrt_test_libs})
kzrt_test(test_rt_auto_bi SOURCES test_rt_auto.c LIBS kzrt-build-info-test kzrt-posix-auto kzrt-posix kzrt kzrt-decode)

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
  PASS_REGULAR_EXPRESSION "ev actor=1 aseq=1 site=10\\[obj\\.use\\.before\\] bind=1001 link=0 t=[0-9]+ pc=0x[0-9a-f]+ key=tid:[0-9]+ .*check actor 1 aseq 1\\.\\.6 missing=0 overflow_counted=0.*check manifest [0-9a-f]+ = [^\n]*sites_example\\.kzs.*module 6 \\(never_loaded\\.so\\) never registered.*check ok")
kzrt_case(rt_dump_tool_strict kz-rt-dump --manifest ${CMAKE_CURRENT_SOURCE_DIR}/data/sites_pointer_only.kzs ${CMAKE_CURRENT_BINARY_DIR}/rt_serial.kzrt)
set_tests_properties(rt_dump_tool_strict PROPERTIES FIXTURES_REQUIRED rt_serial_dump
  PASS_REGULAR_EXPRESSION "string copied from slot 0, which the manifest does not mark string=validated.*the run names manifest [0-9a-f]+, which is none of the given files.*check FAILED \\(3 failures\\)")

# The start-up object: a program that never calls the runtime's setup gets a dump at exit.
kzrt_case(rt_auto test_rt_auto)
set_tests_properties(rt_auto PROPERTIES FIXTURES_SETUP rt_auto_dump
  ENVIRONMENT "KZ_RT_DUMP=${CMAKE_CURRENT_BINARY_DIR}/rt_auto.kzrt;KZ_RT_MODE=record;KZ_RT_RUN_ID=42;KZ_RT_INSTANCE=1")
kzrt_case(rt_auto_dump kz-rt-dump ${CMAKE_CURRENT_BINARY_DIR}/rt_auto.kzrt)
set_tests_properties(rt_auto_dump PROPERTIES FIXTURES_REQUIRED rt_auto_dump
  PASS_REGULAR_EXPRESSION "run abi=1 mode=stopped run_id=42 instance=1 .*runx window_mode=record mode_source=early flags=-.*runx process=[0-9]+ initial=tid:[0-9]+ evidence=constructor,tid-is-pid\nrunx end=in-process end_flags=- end_value=0 store=memory.*ev actor=1 aseq=3 .*check end in-process dump \\(store memory\\).*check ok")
# ... and with the build-identity object linked: the RUN record names the binary's backend, compiler,
# target and manifest (whether kz-rt-dump finds that manifest among given files: rt_dump_tool, rt_mode_api_latch_dump).
kzrt_case(rt_auto_bi test_rt_auto_bi)
set_tests_properties(rt_auto_bi PROPERTIES FIXTURES_SETUP rt_auto_bi_dump
  ENVIRONMENT "KZ_RT_DUMP=${CMAKE_CURRENT_BINARY_DIR}/rt_auto_bi.kzrt;KZ_RT_MODE=record")
kzrt_case(rt_auto_bi_dump kz-rt-dump ${CMAKE_CURRENT_BINARY_DIR}/rt_auto_bi.kzrt)
set_tests_properties(rt_auto_bi_dump PROPERTIES FIXTURES_REQUIRED rt_auto_bi_dump
  PASS_REGULAR_EXPRESSION "run backend=\"unit-test backend\" compiler=\"[^\"]+\" target=\"unit-test target\" manifest_sha256=${kzrt_example_sha}.*runx window_mode=record mode_source=early flags=build-info.*check ok")

kzrt_case(rt_auto_bad test_rt_auto_bad)
set_tests_properties(rt_auto_bad PROPERTIES FIXTURES_SETUP rt_auto_bad_dump ENVIRONMENT "KZ_RT_DUMP=${CMAKE_CURRENT_BINARY_DIR}/rt_auto_bad.kzrt")
kzrt_case(rt_auto_bad_dump kz-rt-dump ${CMAKE_CURRENT_BINARY_DIR}/rt_auto_bad.kzrt)
set_tests_properties(rt_auto_bad_dump PROPERTIES FIXTURES_REQUIRED rt_auto_bad_dump
  PASS_REGULAR_EXPRESSION "run backend=\"unit-test backend\" compiler=\"__VERSION__ [^\"]+\" target=\"\" manifest_sha256=none.*flags=build-info,mode-defaulted,build-info-bad.*check FAIL RUNX: the binary's build info has a manifest sha256 that is not 64 hex digits.*check FAILED \\(1 failure\\)")

# The chunk pool: a large actor, a per-actor quota, an empty (4-chunk) pool, dumps during appends.
foreach(s big quota dumps)
  kzrt_case(rt_pool_${s} test_rt_pool ${s})
endforeach()
kzrt_case(rt_pool_empty test_rt_pool4 pool ${CMAKE_CURRENT_BINARY_DIR}/rt_pool_empty.kzrt)
set_tests_properties(rt_pool_empty PROPERTIES FIXTURES_SETUP rt_pool_empty_dump)
kzrt_case(rt_pool_empty_dump kz-rt-dump ${CMAKE_CURRENT_BINARY_DIR}/rt_pool_empty.kzrt)
set_tests_properties(rt_pool_empty_dump PROPERTIES FIXTURES_REQUIRED rt_pool_empty_dump
  PASS_REGULAR_EXPRESSION "dropped_bytes=1232 chunks=1 overflow_quota=0 overflow_pool=14.*check actor 2 aseq 1\\.\\.60 missing=14 overflow_counted=14.*check capacity pool_bytes=16384 chunk_bytes=4096 chunks_used=4 actors=2 committed=16192 dropped=1232 max_actor_demand=12144 \\(actor 1\\) pool_dropped=14.*check ok")

# One binary, three groups (G-I0 / G-I / G-I-H): the mode comes from KZ_RT_MODE before any constructor.
foreach(m off record control unset bogus)
  kzrt_case(rt_mode_${m} test_rt_mode)
  set_tests_properties(rt_mode_${m} PROPERTIES FIXTURES_SETUP rt_mode_${m}_dump)
  if(m STREQUAL "unset")
    set_tests_properties(rt_mode_${m} PROPERTIES ENVIRONMENT "KZ_RT_DUMP=${CMAKE_CURRENT_BINARY_DIR}/rt_mode_${m}.kzrt")
  elseif(m STREQUAL "bogus")
    set_tests_properties(rt_mode_${m} PROPERTIES ENVIRONMENT "KZ_RT_DUMP=${CMAKE_CURRENT_BINARY_DIR}/rt_mode_${m}.kzrt;KZ_RT_MODE=of")
  else()
    set_tests_properties(rt_mode_${m} PROPERTIES ENVIRONMENT "KZ_RT_DUMP=${CMAKE_CURRENT_BINARY_DIR}/rt_mode_${m}.kzrt;KZ_RT_MODE=${m}")
  endif()
  kzrt_case(rt_mode_${m}_dump kz-rt-dump ${CMAKE_CURRENT_BINARY_DIR}/rt_mode_${m}.kzrt)
  set_tests_properties(rt_mode_${m}_dump PROPERTIES FIXTURES_REQUIRED rt_mode_${m}_dump)
endforeach()
set_tests_properties(rt_mode_off_dump PROPERTIES FAIL_REGULAR_EXPRESSION "\nev "
  PASS_REGULAR_EXPRESSION "run abi=1 mode=off .*runx window_mode=off mode_source=early flags=-.*buf owner=shared bytes=0 .*check ok")
set_tests_properties(rt_mode_record_dump PROPERTIES
  PASS_REGULAR_EXPRESSION "runx window_mode=record mode_source=early flags=-.*ev actor=P1 aseq=1 site=1 .*ev actor=1 aseq=4 site=3 .*check ok")
set_tests_properties(rt_mode_control_dump PROPERTIES
  PASS_REGULAR_EXPRESSION "runx window_mode=control mode_source=early flags=-.*ev actor=P1 aseq=1 site=1 .*ev actor=1 aseq=4 site=3 .*check ok")
set_tests_properties(rt_mode_unset_dump PROPERTIES
  PASS_REGULAR_EXPRESSION "runx window_mode=record mode_source=early flags=mode-defaulted.*ev actor=1 aseq=4 site=3 .*check ok")
set_tests_properties(rt_mode_bogus_dump PROPERTIES
  PASS_REGULAR_EXPRESSION "runx window_mode=record mode_source=early flags=mode-invalid.*check FAIL RUNX: the run named a mode that is not off, record or control.*check FAILED \\(1 failure\\)")

# The setup calls around the mode and the build identity (no start-up object).
foreach(s latch late)
  kzrt_case(rt_mode_api_${s} test_rt_mode_api ${s} ${CMAKE_CURRENT_BINARY_DIR}/rt_mode_api_${s}.kzrt)
  set_tests_properties(rt_mode_api_${s} PROPERTIES FIXTURES_SETUP rt_mode_api_${s}_dump)
endforeach()
kzrt_case(rt_mode_api_latch_dump kz-rt-dump --manifest ${CMAKE_CURRENT_SOURCE_DIR}/data/sites_example.kzs ${CMAKE_CURRENT_BINARY_DIR}/rt_mode_api_latch.kzrt)
set_tests_properties(rt_mode_api_latch_dump PROPERTIES FIXTURES_REQUIRED rt_mode_api_latch_dump
  PASS_REGULAR_EXPRESSION "module 5 actor=[?0-9]+ t=[0-9]+ registrations=3 .*check FAIL RUNX: kz_rt_init named another mode.*check FAIL RUNX: the harness gave a backend.*check FAIL module 5 registered with 2 different manifests.*check FAIL module 5 names manifest 5a5a.*check FAIL the run names manifest 5a5a.*check FAILED \\(5 failures\\)")
kzrt_case(rt_mode_api_late_dump kz-rt-dump ${CMAKE_CURRENT_BINARY_DIR}/rt_mode_api_late.kzrt)
set_tests_properties(rt_mode_api_late_dump PROPERTIES FIXTURES_REQUIRED rt_mode_api_late_dump
  PASS_REGULAR_EXPRESSION "check mode window=record source=early \\(hooks ran under the static default before the early mode\\).*check ok")

# The serialized plan (kzrt 0.3.0): kz_rt_plan_load's checks with and without a build identity, and the start-up
# object's KZ_RT_PLAN in control mode (loaded, checked, kept in the dump) and in record mode (ignored: a check failure).
kzrt_test(test_rt_plan SOURCES test_rt_plan.c LIBS kzrt-build-info-test ${kzrt_test_libs})
target_compile_definitions(test_rt_plan PRIVATE KZRT_TEST_MANIFEST_SHA="${kzrt_example_sha}")
kzrt_test(test_rt_plan_nb SOURCES test_rt_plan.c LIBS ${kzrt_test_libs})
target_compile_definitions(test_rt_plan_nb PRIVATE KZRT_TEST_MANIFEST_SHA="${kzrt_example_sha}")
foreach(s ok mismatch crc format size points twice)
  kzrt_case(rt_plan_${s} test_rt_plan ${s})
endforeach()
kzrt_case(rt_plan_nobuild test_rt_plan_nb nobuild)
kzrt_case(rt_plan_write test_rt_plan write ${CMAKE_CURRENT_BINARY_DIR}/rt_plan.kzrtp)
set_tests_properties(rt_plan_write PROPERTIES FIXTURES_SETUP rt_plan_file)
foreach(m control record none)
  kzrt_case(rt_auto_plan_${m} test_rt_auto_bi)
  set_tests_properties(rt_auto_plan_${m} PROPERTIES FIXTURES_REQUIRED rt_plan_file FIXTURES_SETUP rt_auto_plan_${m}_dump)
endforeach()
set_tests_properties(rt_auto_plan_control PROPERTIES ENVIRONMENT
  "KZ_RT_DUMP=${CMAKE_CURRENT_BINARY_DIR}/rt_auto_plan_control.kzrt;KZ_RT_MODE=control;KZ_RT_PLAN=${CMAKE_CURRENT_BINARY_DIR}/rt_plan.kzrtp")
set_tests_properties(rt_auto_plan_record PROPERTIES ENVIRONMENT
  "KZ_RT_DUMP=${CMAKE_CURRENT_BINARY_DIR}/rt_auto_plan_record.kzrt;KZ_RT_MODE=record;KZ_RT_PLAN=${CMAKE_CURRENT_BINARY_DIR}/rt_plan.kzrtp")
set_tests_properties(rt_auto_plan_none PROPERTIES ENVIRONMENT "KZ_RT_DUMP=${CMAKE_CURRENT_BINARY_DIR}/rt_auto_plan_none.kzrt;KZ_RT_MODE=control")
kzrt_case(rt_auto_plan_control_dump kz-rt-dump --plan ${CMAKE_CURRENT_BINARY_DIR}/rt_plan.kzrtp ${CMAKE_CURRENT_BINARY_DIR}/rt_auto_plan_control.kzrt)
set_tests_properties(rt_auto_plan_control_dump PROPERTIES FIXTURES_REQUIRED "rt_auto_plan_control_dump;rt_plan_file"
  PASS_REGULAR_EXPRESSION "point 0 gate label=1 site=10 releases=- occurrence=first flags=0x1 hits=0 .*plan status=loaded flags=manifest-checked bytes=468 given=468 sha256=[0-9a-f]+\nplan file major=1 minor=0 points=2 manifest=${kzrt_example_sha} .*plan point 0 gate name=P0 label=1 site=10 releases=- occurrence=first flags=unless-released binds=any str=- val=- timeout_ns=0\nplan point 1 release name=P1 label=2 site=20 releases=0 .*check plan status=loaded\ncheck plan [0-9a-f]+ = [^\n]*rt_plan\\.kzrtp.*check ok")
kzrt_case(rt_auto_plan_record_dump kz-rt-dump ${CMAKE_CURRENT_BINARY_DIR}/rt_auto_plan_record.kzrt)
set_tests_properties(rt_auto_plan_record_dump PROPERTIES FIXTURES_REQUIRED rt_auto_plan_record_dump
  PASS_REGULAR_EXPRESSION "plan status=ignored flags=- bytes=0 given=0 sha256=-.*check FAIL PLAN: ignored \\(a plan was given and not loaded\\).*check FAILED \\(1 failure\\)")
kzrt_case(rt_auto_plan_none_dump kz-rt-dump ${CMAKE_CURRENT_BINARY_DIR}/rt_auto_plan_none.kzrt)
set_tests_properties(rt_auto_plan_none_dump PROPERTIES FIXTURES_REQUIRED rt_auto_plan_none_dump
  PASS_REGULAR_EXPRESSION "runx window_mode=control .*plan status=none flags=- bytes=0 .*check plan status=none.*check ok")

# Records that outlive the process (kzrt 0.4.0, event_semantics.md section 12): the target (the test program run again
# as a child, with the start-up object and KZ_RT_STORE) ends by SIGKILL, by SIGSEGV inside a hook, by abort, by SIGTERM
# after a stall, by SIGKILL after a stall whose SIGTERM it ignores, or normally; kz-rt-dump --recover makes the dump of
# its store file and the program checks it (for the normal exit also that the exit handler's dump has the same records,
# and that a cut file and a store of another build are refused).
kzrt_test(test_rt_end SOURCES test_rt_end.c LIBS kzrt-posix-auto kzrt-posix kzrt kzrt-decode)
foreach(c kill segv abort term term-kill normal)
  kzrt_case(rt_end_${c} test_rt_end ${c} ${CMAKE_CURRENT_BINARY_DIR} $<TARGET_FILE:kz-rt-dump>)
  # the dumps stay (rt_end_<case>.rec.kzrt, and rt_end_normal.kzrt of the exit handler): the host converter's test
  # reads them when this tree is built inside kairos (tests/cfs-free test_kzrt_recovered)
  set_tests_properties(rt_end_${c} PROPERTIES FIXTURES_SETUP rt_end_${c}_dump)
endforeach()
set_tests_properties(rt_end_segv PROPERTIES PASS_REGULAR_EXPRESSION
  "runx end=recovered end_flags=signal end_value=11 store=file.*loss partial actor=[0-9]+ key=0 count=1\nloss abnormal-end actor=- key=0 count=1.*check end recovered from the store: a prefix.*check ok.*segv: recovered dump checked")
set_tests_properties(rt_end_term-kill PROPERTIES PASS_REGULAR_EXPRESSION
  "runx end=recovered end_flags=signal end_value=9 store=file.*loss abnormal-end actor=- key=0 count=1.*check ok.*term-kill: the target ended with signal 9.*term-kill: recovered dump checked")
set_tests_properties(rt_end_normal PROPERTIES PASS_REGULAR_EXPRESSION
  "runx end=recovered end_flags=dumped,exit end_value=0 store=file.*check end recovered from the store: the process had completed its own dump.*check ok.*recover refused: the file has [0-9]+ bytes.*recover: the store has pool_bytes=.*normal: recovered dump checked")
kzrt_case(rt_end_notstore kz-rt-dump --recover ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt --out ${CMAKE_CURRENT_BINARY_DIR}/rt_end_notstore.kzrt)
set_tests_properties(rt_end_notstore PROPERTIES PASS_REGULAR_EXPRESSION "not a kzrt store")

# The OSAL adapter where OSAL is initialized (stage F): only with a target OSAL build (KZ_RT_OSAL_INCLUDE_DIRS and
# KZ_RT_OSAL_LIBS, e.g. the C1 reference build's osal/libosal.a and libosal_bsp.a; its BSP main calls the test's
# OS_Application_Startup).  Needs the OSAL's POSIX implementation (pthread, rt).
if(KZ_RT_OSAL_INCLUDE_DIRS AND KZ_RT_OSAL_LIBS)
  add_executable(test_rt_osal test_rt_osal.c)
  target_include_directories(test_rt_osal PRIVATE ${KZ_RT_OSAL_INCLUDE_DIRS})
  target_link_libraries(test_rt_osal PRIVATE kzrt-osal kzrt kzrt-decode -Wl,--start-group ${KZ_RT_OSAL_LIBS} -Wl,--end-group Threads::Threads rt dl)
  set_target_properties(test_rt_osal PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
  kzrt_case(rt_osal_gate test_rt_osal)
endif()

# The core references no library symbol (freestanding claim of kz_rt.h).
if(NOT KZ_SANITIZE AND CMAKE_NM)
  add_test(NAME rt_no_library_symbols COMMAND ${CMAKE_COMMAND} -DNM=${CMAKE_NM} -DLIB=$<TARGET_FILE:kzrt> -P ${CMAKE_CURRENT_SOURCE_DIR}/check_undefined.cmake)
  set_tests_properties(rt_no_library_symbols PROPERTIES LABELS runtime)
endif()

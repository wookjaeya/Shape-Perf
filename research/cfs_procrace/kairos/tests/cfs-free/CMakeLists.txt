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
# kz occ diff as a command: the exit status of the process (test_trace_occ_basis with the kz binary and a directory).
add_test(NAME test_trace_occ_basis_cli COMMAND test_trace_occ_basis $<TARGET_FILE:kz> ${CMAKE_CURRENT_BINARY_DIR})
set_tests_properties(test_trace_occ_basis_cli PROPERTIES LABELS unit)
add_dependencies(test_trace_occ_basis kz)

# Derived records of the S1-R2 development run container (read in place from env/logs, not copied):
# the real-record replay must not become more decisive when information is removed.  Exits 77
# (skipped) when the container is not present.
kz_add_unit_test(test_realrecord_derived SOURCES test_realrecord_derived.cpp LIBS kz-trace kz-testsupport
  ARGS ${CMAKE_SOURCE_DIR}/profiles/cfs-v7.0.1 ${CMAKE_SOURCE_DIR}/../env/logs/s1r2_p1.kzt ${CMAKE_SOURCE_DIR}/tests/static/data/core_cpu1.golden)
set_tests_properties(test_realrecord_derived PROPERTIES SKIP_RETURN_CODE 77)

# The neutral event layer (schema revision 3) on the same real record: the replay of its converted container (neutral
# sections) equals the replay of the record as written (F0 sections converted in memory).  Exits 77 when absent.
add_test(NAME test_trace_neutral_real COMMAND test_trace_neutral ${CMAKE_SOURCE_DIR}/profiles/cfs-v7.0.1 ${CMAKE_SOURCE_DIR}/../env/logs/s1r2_p1.kzt)
set_tests_properties(test_trace_neutral_real PROPERTIES SKIP_RETURN_CODE 77 LABELS unit)
# An instrumented run: a kzrt-dump written by the runtime in this test (POSIX adapter, two threads), converted to the
# neutral layer with a unit-test manifest and replayed on the mini profile.
kz_add_unit_test(test_neutral_kzrt SOURCES test_neutral_kzrt.cpp LIBS kz-trace kz-testsupport kzrt-posix kzrt)
# The identity join of an instrumented run (kzs-sites 2 identity reads): a dump written here with real threads, tasks
# tied to their creating calls by the id alone, owners by the rule F0 uses.
kz_add_unit_test(test_kzrt_identity SOURCES test_kzrt_identity.cpp LIBS kz-trace kz-testsupport kzrt-posix kzrt)
# A recovered run (kzrt 0.4.0, event_semantics.md section 12): the dumps the runtime's rt_end tests leave (SIGKILL, SIGSEGV
# inside a hook, a normal exit and its recovery), converted and read by the replay.
if(TARGET test_rt_end)
  kz_add_unit_test(test_kzrt_recovered SOURCES test_kzrt_recovered.cpp LIBS kz-trace kz-testsupport ARGS ${CMAKE_BINARY_DIR}/runtime/tests)
  set_tests_properties(test_kzrt_recovered PROPERTIES FIXTURES_REQUIRED "rt_end_kill_dump;rt_end_segv_dump;rt_end_normal_dump")
endif()

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
# Without -g: no line table (a release build such as the C1 builds); function and function-scoped
# call targets only.
add_custom_command(OUTPUT target_prog_nodebug
  COMMAND ${CMAKE_C_COMPILER} -O0 -fno-omit-frame-pointer -fcf-protection=full -fPIE -pie -pthread -o target_prog_nodebug ${CMAKE_CURRENT_SOURCE_DIR}/target/target_prog.c
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/target/target_prog.c VERBATIM)
list(APPEND kz_alt_binaries target_prog_nodebug)
# The entry-reached tests: a program whose lookup reaches its search by a tail jump, at -O2 without -g (a
# release-like build), by the build's compiler and by the other one.
add_custom_command(OUTPUT tailjump_prog_cc
  COMMAND ${CMAKE_C_COMPILER} -O2 -fcf-protection=full -fPIE -pie -o tailjump_prog_cc ${CMAKE_CURRENT_SOURCE_DIR}/target/tailjump_prog.c
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/target/tailjump_prog.c VERBATIM)
list(APPEND kz_alt_binaries tailjump_prog_cc)
if(KZ_ALT_CC)
  add_custom_command(OUTPUT tailjump_prog_alt
    COMMAND ${KZ_ALT_CC} -O2 -fcf-protection=full -fPIE -pie -o tailjump_prog_alt ${CMAKE_CURRENT_SOURCE_DIR}/target/tailjump_prog.c
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/target/tailjump_prog.c VERBATIM)
  list(APPEND kz_alt_binaries tailjump_prog_alt)
endif()
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
# test_probe_anchor_compilers exits 77 when addr2line or the other compiler's build is missing: the
# cross-check did not run, so the test is reported as skipped, not as passed.
set_tests_properties(test_probe_anchor_compilers PROPERTIES SKIP_RETURN_CODE 77)
# test_probe_anchor_reached exits 77 when a part could not run: the other compiler's build, or the run
# under kz observe (root and a tracefs: KZ_TRACEFS or /sys/kernel/tracing).
set_tests_properties(test_probe_anchor_reached PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 300)
set_tests_properties(test_probe_observe PROPERTIES LABELS "unit;integration" SKIP_RETURN_CODE 77 TIMEOUT 900)

# The anchor cross-check step of probe/scripts/observe_cfs_smoke.sh (--xcheck), on the target program:
# every site of target.kzp is found by addr2line and in objdump's disassembly at the address kz anchor
# resolved, and the output is the same twice (it holds no time).  Exits 77 (skipped) without binutils.
add_test(NAME probe_anchor_xcheck_script
  COMMAND bash -c [=[
    set -eu
    kz=$1; prog=$2; profile=$3; root=$4; script=$5; d=$6
    "$kz" anchor "$prog" "$profile" -o "$d/xcheck_basis.kz" --source-root "$root" > /dev/null
    rc=0; bash "$script" --xcheck "$kz" "$d/xcheck_basis.kz" "$profile" "$d/xcheck_1.txt" || rc=$?
    [ "$rc" -eq 0 ] || exit "$rc"
    bash "$script" --xcheck "$kz" "$d/xcheck_basis.kz" "$profile" "$d/xcheck_2.txt"
    cmp -s "$d/xcheck_1.txt" "$d/xcheck_2.txt" || { echo "the cross-check output differs between two runs"; exit 1; }
    n_sites=$(grep -c '^site ' "$profile")
    [ "$(grep -c '^== site ' "$d/xcheck_1.txt")" -eq "$n_sites" ] || { echo "not every site has a block"; exit 1; }
    [ "$(grep -c '^>' "$d/xcheck_1.txt")" -eq "$n_sites" ] || { echo "a probed address is not an instruction boundary of objdump"; exit 1; }
    ! grep -q 'address not found' "$d/xcheck_1.txt" || exit 1
    grep -q '^tp_send$' "$d/xcheck_1.txt" || { echo "addr2line did not name tp_send"; exit 1; }
    grep -q 'target_prog.c:' "$d/xcheck_1.txt" || { echo "addr2line gave no line of target_prog.c"; exit 1; }
    grep -A3 'publish.after_get' "$d/xcheck_1.txt" | grep -q '^tp_publish$' || { echo "the after-call site is not in tp_publish"; exit 1; }
    echo "xcheck: $n_sites sites cross-checked"
  ]=] _ $<TARGET_FILE:kz> $<TARGET_FILE:target_prog> ${CMAKE_CURRENT_SOURCE_DIR}/target/target.kzp ${CMAKE_SOURCE_DIR}
      ${CMAKE_SOURCE_DIR}/probe/scripts/observe_cfs_smoke.sh ${CMAKE_CURRENT_BINARY_DIR}
  VERBATIM)
set_tests_properties(probe_anchor_xcheck_script PROPERTIES LABELS "unit" SKIP_RETURN_CODE 77)

# The hold program and the kz hold integration test (SPEC 4.13 F0): a two-thread program, built like
# the framework's functions, that the test gates and releases with real ptrace + debug registers.
add_executable(hold_prog target/hold_prog.c)
target_compile_options(hold_prog PRIVATE -g -O0 -fno-omit-frame-pointer -fcf-protection=full)
set_target_properties(hold_prog PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(hold_prog PRIVATE Threads::Threads)

kz_add_unit_test(test_hold_ptrace SOURCES test_hold_ptrace.cpp LIBS kz-testsupport
  ARGS $<TARGET_FILE:hold_prog> ${CMAKE_CURRENT_SOURCE_DIR}/target/hold.kzp ${CMAKE_CURRENT_SOURCE_DIR}/target/hold.kzh
       $<TARGET_FILE:kz> ${CMAKE_CURRENT_BINARY_DIR})
add_dependencies(test_hold_ptrace hold_prog kz)
# Needs the right to PTRACE_SEIZE and to write the debug registers; where the kernel refuses, the
# test exits 77 (reported as skipped, never as passed).
set_tests_properties(test_hold_ptrace PROPERTIES LABELS "unit;integration" SKIP_RETURN_CODE 77 TIMEOUT 120)

# The sequence program and its kz hold test: a thread resumed from a gate fires a later release (its debug
# registers stay armed), `occurrence <n>` acts on the n-th qualifying hit, two gates share one address, a release on
# a held hit acts on the resume, and an unless-released gate is satisfied by a release that comes first.
add_executable(hold_seq_prog target/hold_seq_prog.c)
target_compile_options(hold_seq_prog PRIVATE -g -O0 -fno-omit-frame-pointer -fcf-protection=full)
set_target_properties(hold_seq_prog PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(hold_seq_prog PRIVATE Threads::Threads)
kz_add_unit_test(test_hold_sequence SOURCES test_hold_sequence.cpp LIBS kz-testsupport
  ARGS $<TARGET_FILE:hold_seq_prog> ${CMAKE_CURRENT_SOURCE_DIR}/target/hold_seq.kzp ${CMAKE_CURRENT_SOURCE_DIR}/target/hold_rearm.kzh
       ${CMAKE_CURRENT_SOURCE_DIR}/target/hold_nth.kzh $<TARGET_FILE:kz> ${CMAKE_CURRENT_BINARY_DIR}
       ${CMAKE_CURRENT_SOURCE_DIR}/target/hold_defer.kzh)
add_dependencies(test_hold_sequence hold_seq_prog kz)
set_tests_properties(test_hold_sequence PROPERTIES LABELS "unit;integration" SKIP_RETURN_CODE 77 TIMEOUT 120)
# The kzp-hold 1 parser: occurrence first|each|<n>, unless-released.
kz_add_unit_test(test_hold_plan SOURCES test_hold_plan.cpp LIBS kz-hold kz-testsupport)
# The cooperative gate's host converter (kz rtplan): kzp-hold 1 -> kzrt-plan 1 for one build's manifest and bindings.
kz_add_unit_test(test_rt_plan_convert SOURCES test_rt_plan_convert.cpp LIBS kz-hold kz-testsupport)

# Tests of the static analysis layer (lib/llvm and tools/kz-static).  They exist only in a tree
# configured with -DKZ_BUILD_LLVM_TOOLS=ON, like the code they test.
#
#   fixtures/        small self-contained C programs, one for each kind of fact; each is compiled by the
#                    C compiler of the build with -std=c99 -pedantic -Wall -Wextra -Werror, and read by
#                    kz-static with -std=c99 -pedantic (static_fixture_compile_*, static_golden_*);
#                    clobber.c is read only (see below); wrappers.c is read in the profile-free mode
#                    (extract --no-profile, no fixture profile); flowfacts.c is read twice for the extended
#                    facts F1-F3, with the fixture profile and --extended-facts (flowfacts) and in the
#                    profile-free mode (flowfacts_np); argflow.c is read the same two ways for the extended
#                    facts F4 and F6 (argflow, argflow_np), lockset.c for F5 and the target-independent rules
#                    (lockset, lockset_np), relations.c for `kz-static relations` (relations: the fixture profile
#                    and --extended-facts; relations_np: --no-profile --targets-rule R-T1,R-T2), relpaths.c for the
#                    order and entry-path facts LOCK_PATHS and the relations rules that read them (lockpaths_np,
#                    relpaths_np: --no-profile --targets-rule R-T1,R-T2); relsound.c for the soundness rules of
#                    relations path C (relsound_np: the read run as the spawned entry, indirect callers, record-wide
#                    writes, a lock taken again around a loop, a lock that is no static instance, an unresolved call,
#                    the creator's caller's write, and the control; revisions 4 and 5 add their cases: the creator run
#                    again, hooks whose targets are not complete, a callback of a function without a body, unresolved
#                    calls of the child, an entry from a union member of a static table, hooks set through a cast lvalue
#                    of the variable, a copy of the pointer to it or the pointer an array decays to); deepcond.c (a condition more than 200
#                    levels deep) is read by test_static_cli only
#   data/*.golden    the text view of the facts of each fixture and of the core executable
#   test_static_*    unit tests of the parts that need no Clang (expression pool, flow graph, table
#                    walks, recipe, commit reader, link set, command line, basis -> extract on a
#                    synthetic program, the P0 table and the wrapper derivation of the profile-free mode)
#   static_golden    the golden driver; `core` runs on the pinned bundle and is skipped (exit 77) where
#                    the bundle or its build is not there
#
# Accept a deliberate change of a golden with KZ_GOLDEN_UPDATE=1 ctest -R static_golden, then review
# the diff of data/.  The values in the goldens are those of the programs under analysis; they are
# not experiment results.
if(NOT KZ_BUILD_LLVM_TOOLS)
  return()
endif()

include_directories("${CMAKE_SOURCE_DIR}/tests/unit" "${CMAKE_SOURCE_DIR}/lib/llvm")

set(KZ_PROFILE_DIR "${CMAKE_SOURCE_DIR}/profiles/cfs-v7.0.1" CACHE PATH "The profile directory the tests use")
set(KZ_BUNDLE_ROOT "/home/user/work/procrace/cfs_ref/cFS" CACHE PATH "The pinned bundle the real run reads")
set(kz_fixture_dir "${CMAKE_CURRENT_SOURCE_DIR}/fixtures")
set(kz_fixture_profile "${CMAKE_CURRENT_SOURCE_DIR}/profile")

# ---- the fixtures are valid C99 for the compiler of the build -----------------------------------------
# clobber.c writes past the end of an object on purpose (a fact the extractor must see); both
# compilers reject that at -Werror, so it is read by kz-static but not compiled here.
set(kz_fixtures writers outptr derives loops spawn atype wrappers flowfacts argflow lockset relations relpaths relsound deepcond)
set(kz_golden_fixtures writers outptr derives loops spawn atype wrappers flowfacts argflow lockset relations clobber flowfacts_np argflow_np lockset_np
    relations_np lockpaths_np relpaths_np relsound_np)
foreach(kz_name IN LISTS kz_fixtures)
  add_test(NAME static_fixture_compile_${kz_name}
    COMMAND ${CMAKE_C_COMPILER} -std=c99 -pedantic -Wall -Wextra -Werror -c ${kz_fixture_dir}/${kz_name}.c -o ${CMAKE_CURRENT_BINARY_DIR}/${kz_name}.fixture.o)
  set_tests_properties(static_fixture_compile_${kz_name} PROPERTIES LABELS static)
endforeach()

# ---- golden files ---------------------------------------------------------------------------------------
add_executable(static_golden static_golden.cpp)
target_link_libraries(static_golden PRIVATE kz-core)
kz_apply_settings(static_golden)
add_dependencies(static_golden kz-static)
foreach(kz_name IN LISTS kz_golden_fixtures)
  add_test(NAME static_golden_${kz_name}
    COMMAND static_golden fixture ${kz_name} $<TARGET_FILE:kz-static> ${kz_fixture_dir} ${kz_fixture_profile}
            ${CMAKE_CURRENT_SOURCE_DIR}/data ${CMAKE_CURRENT_BINARY_DIR}/work_${kz_name})
  set_tests_properties(static_golden_${kz_name} PROPERTIES LABELS "static;golden")
endforeach()

# The core executable of the pinned bundle: basis, extract, numbers.
# Three lines of data/core_cpu1.golden come from the bundle build the golden was made from, not from the
# tool: binary.sha256, binary.build_id and generated.build_env_table.sha256.  The generated table holds the
# build date (BUILDDATE), and the date also changes the executable and its GNU ld build id, so a rebuild of
# the bundle from the same pinned sources changes these three lines and no other.  Regenerated on 2026-10-29
# from the rebuilt bundle (env/REBUILD_20261029.md sections 2.2 and 5.2); only these three lines changed:
#   binary.sha256                     0edb1ae28780160ebee95adf8861fc76fe7119a30434e9cd8698cb505bc97799 (BUILDDATE 202610080115)
#                                  -> c10d7dd1ad0b83106d3e0ebdb90a13a617c34ab2ea073f3bd360f0358b04be44 (BUILDDATE 202610290151)
#   binary.build_id                   be6d04f1f80d421efea50cc9f47db848b2a52941 -> 7b87b2a417ca18b8cf874ed4bb1a6972e851585e
#   generated.build_env_table.sha256  ebac47dd2849b67c... -> 126f83723941c3eb...
# The rebuilt build with only the date string set back to 202610080115 gives the old table hash and, relinked,
# the old executable (REBUILD section 2.2); the other 563 lines are the same for both builds.
add_test(NAME static_golden_core_cpu1
  COMMAND static_golden core $<TARGET_FILE:kz-static> ${KZ_PROFILE_DIR} ${KZ_PROFILE_DIR}/basis/core-cpu1.kzb ${KZ_BUNDLE_ROOT}
          ${CMAKE_CURRENT_SOURCE_DIR}/data ${CMAKE_CURRENT_BINARY_DIR}/work_core)
set_tests_properties(static_golden_core_cpu1 PROPERTIES LABELS "static;golden;integration" SKIP_RETURN_CODE 77 TIMEOUT 600)

# ---- unit tests ---------------------------------------------------------------------------------------------
foreach(kz_name test_static_expr test_static_flow test_static_recipe test_static_gitrev test_static_unroll test_static_wrappers)
  kz_add_unit_test(${kz_name} SOURCES ${kz_name}.cpp LIBS kz-llvm kz-testsupport LABEL static)
endforeach()

# Object files for the link-set test, built with the C compiler of the build.
# They are data for the link-set reader, not code under test: a sanitizer build must not add its
# runtime symbols to them (the test checks their symbol tables).
foreach(kz_obj main_obj whole_member lazy_used lazy_unused)
  add_library(kz_static_${kz_obj} OBJECT objects/${kz_obj}.c)
  target_compile_options(kz_static_${kz_obj} PRIVATE -std=c99 -fno-sanitize=all)
endforeach()
kz_add_unit_test(test_static_link SOURCES test_static_link.cpp LIBS kz-llvm kz-testsupport LABEL static
  ARGS $<TARGET_OBJECTS:kz_static_main_obj> $<TARGET_OBJECTS:kz_static_whole_member> $<TARGET_OBJECTS:kz_static_lazy_used> $<TARGET_OBJECTS:kz_static_lazy_unused>)
# the objects are arguments, not sources: building the test must build them too
add_dependencies(test_static_link kz_static_main_obj kz_static_whole_member kz_static_lazy_used kz_static_lazy_unused)

# ELF32 little-endian i386 (E-ST, the reading part): objects, a shared object and an executable built at test time with
# -m32, damaged headers, one ELF identity per program, and the basis of a small 32-bit closure.  Skipped (77) where the
# compiler cannot build -m32 programs.
kz_add_unit_test(test_static_elf32 SOURCES test_static_elf32.cpp LIBS kz-llvm kz-testsupport LABEL static
  ARGS $<TARGET_FILE:kz-static> ${CMAKE_C_COMPILER} ${CMAKE_CURRENT_SOURCE_DIR}/objects ${kz_fixture_profile} ${CMAKE_CURRENT_BINARY_DIR}/work_elf32)
set_tests_properties(test_static_elf32 PROPERTIES SKIP_RETURN_CODE 77)
add_dependencies(test_static_elf32 kz-static)

kz_add_unit_test(test_static_cli SOURCES test_static_cli.cpp LIBS kz-core kz-testsupport LABEL static
  ARGS $<TARGET_FILE:kz-static> ${kz_fixture_dir} ${CMAKE_CURRENT_SOURCE_DIR}/objects ${kz_fixture_profile} ${CMAKE_C_COMPILER} ${CMAKE_CURRENT_BINARY_DIR})
add_dependencies(test_static_cli kz-static)

# Checks kz_source_identity() from cmake/KzSourceIdentity.cmake on small trees written to a
# scratch directory:  cmake -DKZ_MODULE=<KzSourceIdentity.cmake> -DWORK=<scratch dir> -P this file
include("${KZ_MODULE}")

function(check_eq name a b)
  if(NOT "${a}" STREQUAL "${b}")
    message(FATAL_ERROR "${name}: expected equal identities\n  ${a}\n  ${b}")
  endif()
endfunction()

function(check_ne name a b)
  if("${a}" STREQUAL "${b}")
    message(FATAL_ERROR "${name}: identities should differ but both are ${a}")
  endif()
endfunction()

function(make_tree root)
  file(REMOVE_RECURSE "${root}")
  file(WRITE "${root}/lib/a.txt" "int a;\n")
  file(WRITE "${root}/lib/sub/b.txt" "int b;\n")
  file(WRITE "${root}/top.txt" "top\n")
endfunction()

make_tree("${WORK}/t1")
kz_source_identity(base ROOT "${WORK}/t1" PATHS lib top.txt)
string(LENGTH "${base}" base_length)
if(NOT base MATCHES "^[0-9a-f]+$" OR NOT base_length EQUAL 64)
  message(FATAL_ERROR "identity is not 64 lower-case hex digits: ${base}")
endif()

# the same tree in another place, listed in another order, with a path twice
make_tree("${WORK}/elsewhere/t2")
kz_source_identity(moved ROOT "${WORK}/elsewhere/t2" PATHS top.txt lib lib)
check_eq("a moved tree" "${base}" "${moved}")

# content, name, set of files
file(WRITE "${WORK}/t1/lib/a.txt" "int a2;\n")
kz_source_identity(changed ROOT "${WORK}/t1" PATHS lib top.txt)
check_ne("changed content" "${base}" "${changed}")
make_tree("${WORK}/t1")
file(RENAME "${WORK}/t1/lib/sub/b.txt" "${WORK}/t1/lib/sub/c.txt")
kz_source_identity(renamed ROOT "${WORK}/t1" PATHS lib top.txt)
check_ne("renamed file" "${base}" "${renamed}")
make_tree("${WORK}/t1")
file(WRITE "${WORK}/t1/lib/sub/extra.txt" "int x;\n")
kz_source_identity(added ROOT "${WORK}/t1" PATHS lib top.txt)
check_ne("added file" "${base}" "${added}")
make_tree("${WORK}/t1")
file(REMOVE "${WORK}/t1/lib/a.txt")
kz_source_identity(removed ROOT "${WORK}/t1" PATHS lib top.txt)
check_ne("removed file" "${base}" "${removed}")

# what is covered: a path that is not listed does not matter, a listed one does
make_tree("${WORK}/t1")
file(WRITE "${WORK}/t1/top.txt" "top changed\n")
kz_source_identity(only_lib ROOT "${WORK}/t1" PATHS lib)
make_tree("${WORK}/t1")
kz_source_identity(only_lib_before ROOT "${WORK}/t1" PATHS lib)
check_eq("an uncovered file" "${only_lib}" "${only_lib_before}")

# hidden files and backup files are left out
make_tree("${WORK}/t1")
file(WRITE "${WORK}/t1/lib/.hidden" "x\n")
file(WRITE "${WORK}/t1/lib/a.txt~" "x\n")
file(WRITE "${WORK}/t1/lib/.cache/z.txt" "x\n")
kz_source_identity(noisy ROOT "${WORK}/t1" PATHS lib top.txt)
check_eq("hidden and backup files" "${base}" "${noisy}")

# a missing path is an error, not an empty identity
execute_process(COMMAND ${CMAKE_COMMAND} -DKZ_MODULE=${KZ_MODULE} -DROOT=${WORK}/t1 -P ${CMAKE_CURRENT_LIST_DIR}/check_source_identity_missing.cmake
                RESULT_VARIABLE rc OUTPUT_QUIET ERROR_QUIET)
if(rc EQUAL 0)
  message(FATAL_ERROR "a missing path should be an error")
endif()

file(REMOVE_RECURSE "${WORK}")
message(STATUS "source identity: all checks passed")

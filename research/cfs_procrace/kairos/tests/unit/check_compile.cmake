# check_compile.cmake -- run a compiler on a source file and check that it succeeds or fails.
# Arguments (-D): COMPILER, SOURCE, INCLUDE, FLAGS (a ;-list), EXPECT (PASS|FAIL), REGEX (for FAIL).
execute_process(
  COMMAND ${COMPILER} ${FLAGS} -I${INCLUDE} -fsyntax-only ${SOURCE}
  RESULT_VARIABLE result
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err)
if(EXPECT STREQUAL "PASS")
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "expected the compile to pass, but it failed:\n${out}${err}")
  endif()
else()
  if(result EQUAL 0)
    message(FATAL_ERROR "expected the compile to fail, but it passed")
  endif()
  if(NOT "${out}${err}" MATCHES "${REGEX}")
    message(FATAL_ERROR "the compile failed, but not on the layout assertion (${REGEX}):\n${out}${err}")
  endif()
endif()

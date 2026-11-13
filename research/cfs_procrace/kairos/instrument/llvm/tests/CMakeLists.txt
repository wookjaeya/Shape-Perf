# Unit tests of the kzllvm plugin (unit-test values, not research conditions).  The target program and its
# manifests are those of the GCC candidate (instrument/gcc/tests).
set(KZLLVM_RT ${CMAKE_BINARY_DIR}/kzrt)
set(KZLLVM_T ${CMAKE_CURRENT_SOURCE_DIR}/../../gcc/tests)
foreach(mode O0 O2 O3 O0-g lines untouched refuse)
  add_test(NAME kzllvm_${mode}
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/run_target_test.sh ${mode} ${KZLLVM_CLANG} $<TARGET_FILE:kzllvm> ${KZLLVM_RT}
            $<TARGET_FILE:kz-events> ${KZLLVM_T} ${CMAKE_CURRENT_BINARY_DIR}/${mode})
endforeach()

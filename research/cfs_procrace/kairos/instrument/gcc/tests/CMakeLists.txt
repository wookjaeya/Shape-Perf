# Unit tests of the kzgcc plugin (unit-test values, not research conditions).  Each test builds the
# small target with the plugin, runs it with the POSIX runtime and checks the dump and binding file.
set(KZGCC_RT ${CMAKE_BINARY_DIR}/kzrt)
foreach(opt O0 O2 O3 O2-lto)
  add_test(NAME kzgcc_target_${opt}
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/run_target_test.sh ${opt} ${KZGCC_GCC} $<TARGET_FILE:kzgcc> ${KZGCC_RT}
            $<TARGET_FILE:kzgcc-events> ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/${opt})
endforeach()
add_test(NAME kzgcc_untouched_unit
  COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/run_target_test.sh untouched ${KZGCC_GCC} $<TARGET_FILE:kzgcc> ${KZGCC_RT}
          $<TARGET_FILE:kzgcc-events> ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/untouched)
add_test(NAME kzgcc_refuses_runtime
  COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/run_target_test.sh refuse ${KZGCC_GCC} $<TARGET_FILE:kzgcc> ${KZGCC_RT}
          $<TARGET_FILE:kzgcc-events> ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/refuse)
foreach(opt O0 O2 O3)
  add_test(NAME kzgcc_shapes_${opt}
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/run_target_test.sh shapes-${opt} ${KZGCC_GCC} $<TARGET_FILE:kzgcc> ${KZGCC_RT}
            $<TARGET_FILE:kzgcc-events> ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/shapes_${opt})
endforeach()
# The cooperative gate (stage F) end to end: hold_seq_prog.c with kzgcc_hold_seq.kzs, plans converted by kz rtplan and
# loaded by the start-up object in control mode, one kzp-hold 1 rule per plan (run_hold_test.sh).  Needs the kz
# executable of the Kairos build (KZGCC_KZ); skipped (77) without it.
set(KZGCC_KZ "" CACHE FILEPATH "the kz executable of the Kairos build (kz rtplan), for the cooperative-gate tests")
foreach(plan rearm nth defer caller refuse)
  add_test(NAME kzgcc_hold_${plan}
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/run_hold_test.sh ${plan} ${KZGCC_GCC} $<TARGET_FILE:kzgcc> ${KZGCC_RT} "${KZGCC_KZ}" ${KZ_KAIROS}
            ${CMAKE_CURRENT_BINARY_DIR}/hold_${plan})
  set_tests_properties(kzgcc_hold_${plan} PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
endforeach()

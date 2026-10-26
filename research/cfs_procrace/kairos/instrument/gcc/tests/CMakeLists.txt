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

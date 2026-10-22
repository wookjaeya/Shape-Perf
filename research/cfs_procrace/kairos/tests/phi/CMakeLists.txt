# Tests of the profile language against the real profile of the V1 subset and synthetic traces:
#   - golden files: the canonical text and hash of the profile, the replay of every trace;
#   - the profile checks without error (kz-phi check);
#   - the citations of the profile re-read in the pinned bundle (skipped if the bundle is absent).
# Golden update: KZ_GOLDEN_UPDATE=1 ctest -R phi_golden, then review the diff of data/.

set(KZ_PROFILE_DIR "${CMAKE_SOURCE_DIR}/profiles/cfs-v7.0.1" CACHE PATH "The profile directory the tests use")
set(KZ_BUNDLE_ROOT "/home/user/work/procrace/cfs_ref/cFS" CACHE PATH "The pinned bundle the citations of the profile are checked against")

add_executable(kz_phi_golden golden_phi.cpp)
target_link_libraries(kz_phi_golden PRIVATE kz-core)
kz_apply_settings(kz_phi_golden)

set(kz_phi_cases
  profile_canonical profile_summary
  replay_startup_ok replay_startup_old_version replay_startup_no_version replay_startup_no_predicate
  replay_kid_illegal replay_kid_unregistered replay_kid_ordinals replay_kid_disagree
  replay_shutdown_scope replay_poll_timeout replay_poll_timeout_wrong
  replay_kid_outcomes replay_effects_core replay_effects_external replay_cleanup_window)
foreach(kz_case IN LISTS kz_phi_cases)
  add_test(NAME phi_golden_${kz_case}
    COMMAND kz_phi_golden ${kz_case} ${KZ_PROFILE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/data ${CMAKE_CURRENT_SOURCE_DIR}/traces)
  set_tests_properties(phi_golden_${kz_case} PROPERTIES LABELS golden)
endforeach()

add_test(NAME phi_profile_check COMMAND kz-phi check ${KZ_PROFILE_DIR})
set_tests_properties(phi_profile_check PROPERTIES LABELS unit)

# The citations: every cited line range is re-read under the bundle and its SHA-256 compared.
# Exit status 77 (skip) when the bundle is not there, so that a machine without it says so.
add_test(NAME phi_profile_provenance
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/run_verify_prov.sh $<TARGET_FILE:kz-phi> ${KZ_BUNDLE_ROOT} ${KZ_PROFILE_DIR})
set_tests_properties(phi_profile_provenance PROPERTIES LABELS unit SKIP_RETURN_CODE 77)
add_dependencies(kz_phi_golden kz-phi)

# The command line tool on the real profile.
kz_add_unit_test(test_phi_cli SOURCES test_phi_cli.cpp LIBS kz-testsupport
  ARGS $<TARGET_FILE:kz-phi> ${KZ_PROFILE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/data ${CMAKE_CURRENT_SOURCE_DIR}/traces)
add_dependencies(test_phi_cli kz-phi)

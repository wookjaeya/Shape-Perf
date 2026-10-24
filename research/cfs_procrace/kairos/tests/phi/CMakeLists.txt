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

# ---- profile c1-bp-b051eb0 (case C1 on the W3 builds): one profile directory plus variant directories
# (CF revision, cFS bundle candidate, readiness specification as it stands or the P3 alternative).
#   - every combination checks (kz-phi check);
#   - every combination's citations are re-read in the C1 source view of its build (skipped, 77, where
#     the views are absent; docs/c1_smoke_observe.md says how they are made);
#   - golden files: the canonical text of the two CF revisions (bundle 1df262a), the hash, statistics and
#     caller-level Allowed of six combinations, and replays of synthetic C1 traces (roles, Allowed).
set(KZ_C1_PROFILE_DIR "${CMAKE_SOURCE_DIR}/profiles/c1-bp-b051eb0")
set(KZ_C1_PROV_ROOTS "/home/user/work/procrace/c1_runs/prov_roots" CACHE PATH "The C1 source views (one per W3 build) the C1 citations are checked against")
foreach(kz_cf 281a941 833fdbb)
  if(kz_cf STREQUAL "281a941")
    set(kz_cfg pre)
  else()
    set(kz_cfg post)
  endif()
  foreach(kz_cand 1df262a 1b0b338)
    foreach(kz_rs "" "-altP3")
      set(kz_v "${kz_cf}_${kz_cand}${kz_rs}")
      set(kz_dirs ${KZ_C1_PROFILE_DIR} ${KZ_C1_PROFILE_DIR}/cf-${kz_cf} ${KZ_C1_PROFILE_DIR}/bundle-${kz_cand} ${KZ_C1_PROFILE_DIR}/rspec-${kz_cf}${kz_rs})
      add_test(NAME phi_c1_check_${kz_v} COMMAND kz-phi check ${kz_dirs})
      set_tests_properties(phi_c1_check_${kz_v} PROPERTIES LABELS unit)
      add_test(NAME phi_c1_provenance_${kz_v}
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/run_verify_prov.sh $<TARGET_FILE:kz-phi> ${KZ_C1_PROV_ROOTS}/${kz_cfg}_${kz_cand} ${kz_dirs})
      set_tests_properties(phi_c1_provenance_${kz_v} PROPERTIES LABELS unit SKIP_RETURN_CODE 77)
      if(kz_cand STREQUAL "1df262a" OR kz_rs STREQUAL "")
        string(REPLACE ";" ":" kz_dirlist "${kz_dirs}")
        add_test(NAME phi_c1_golden_summary_${kz_v}
          COMMAND kz_phi_golden profile_summary ${kz_dirlist} ${CMAKE_CURRENT_SOURCE_DIR}/data ${CMAKE_CURRENT_SOURCE_DIR}/traces c1_summary_${kz_v})
        set_tests_properties(phi_c1_golden_summary_${kz_v} PROPERTIES LABELS golden)
        if(kz_cand STREQUAL "1df262a" AND kz_rs STREQUAL "")
          add_test(NAME phi_c1_golden_canonical_${kz_v}
            COMMAND kz_phi_golden profile_canonical ${kz_dirlist} ${CMAKE_CURRENT_SOURCE_DIR}/data ${CMAKE_CURRENT_SOURCE_DIR}/traces c1_canonical_${kz_v})
          set_tests_properties(phi_c1_golden_canonical_${kz_v} PROPERTIES LABELS golden)
        endif()
      endif()
    endforeach()
  endforeach()
endforeach()
# Replays of synthetic C1 traces: case -> CF revision and readiness specification directory.
foreach(kz_pair "c1_pre_fail;281a941;rspec-281a941" "c1_post_retry;833fdbb;rspec-833fdbb" "c1_post_retry_altP3;833fdbb;rspec-833fdbb-altP3"
                "c1_role_unbound;281a941;rspec-281a941")
  list(GET kz_pair 0 kz_case)
  list(GET kz_pair 1 kz_cf)
  list(GET kz_pair 2 kz_rsdir)
  add_test(NAME phi_golden_replay_${kz_case}
    COMMAND kz_phi_golden replay_${kz_case} "${KZ_C1_PROFILE_DIR}:${KZ_C1_PROFILE_DIR}/cf-${kz_cf}:${KZ_C1_PROFILE_DIR}/bundle-1df262a:${KZ_C1_PROFILE_DIR}/${kz_rsdir}"
            ${CMAKE_CURRENT_SOURCE_DIR}/data ${CMAKE_CURRENT_SOURCE_DIR}/traces)
  set_tests_properties(phi_golden_replay_${kz_case} PROPERTIES LABELS golden)
endforeach()

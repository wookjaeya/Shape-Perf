# Golden tests: canonical dumps of the layouts, of pattern-filled records and of three
# fixture containers (with their artifact hashes), the rendering rules and the definition of the
# tool hash (on made-up inputs).  To accept a
# deliberate change: KZ_GOLDEN_UPDATE=1 ctest -R golden, then review the diff of data/.
add_executable(kz_golden golden_main.cpp)
target_link_libraries(kz_golden PRIVATE kz-testsupport)
kz_apply_settings(kz_golden)
foreach(kz_case layout records container_facts container_run container_verdicts render tool_hash)
  add_test(NAME golden_${kz_case} COMMAND kz_golden ${kz_case} ${CMAKE_CURRENT_SOURCE_DIR}/data)
  set_tests_properties(golden_${kz_case} PROPERTIES LABELS golden)
endforeach()

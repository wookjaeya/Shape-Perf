# Lints of the repository (see kz_lint.cpp).  They run on the source tree at ctest time:
#   ctest -L lint
# The selftest checks that every rule rejects what it must and accepts what it must.
add_executable(kz_lint kz_lint.cpp)
kz_apply_settings(kz_lint)

add_test(NAME lint_selftest COMMAND kz_lint --selftest --data ${CMAKE_CURRENT_SOURCE_DIR}/data)
foreach(kz_rule case-ids retired-term banned-files no-llvm no-generated)
  add_test(NAME lint_${kz_rule} COMMAND kz_lint ${kz_rule} ${CMAKE_SOURCE_DIR} --data ${CMAKE_CURRENT_SOURCE_DIR}/data)
endforeach()
set_tests_properties(lint_selftest lint_case-ids lint_retired-term lint_banned-files lint_no-llvm lint_no-generated PROPERTIES LABELS lint)

# `cmake --build <dir> --target kz-lint` runs every rule at once (not part of the default build,
# so that one component's work in progress cannot stop another's build).
add_custom_target(kz-lint
  COMMAND ctest -L lint --output-on-failure
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  DEPENDS kz_lint)

# kz-instr-clang: the Clang AST source-transformation connector of the instrumented family (stage C candidate;
# SPEC rev 1.4 sections 4.1 and 5.12, docs/instr_candidate_clang.md).  Built only with -DKZ_BUILD_LLVM_TOOLS=ON
# (the top-level CMakeLists lists this directory with the components that need LLVM).  It reads the site
# manifest with the runtime's host parser (kzrt-manifest) and writes edited copies of target sources into a
# separate tree; it never writes Kairos sources and is not linked into kz.
add_executable(kz-instr-clang kz_instr_clang.cpp)
target_link_libraries(kz-instr-clang PRIVATE kzrt-manifest clangTooling clangRewrite clangFrontend clangLex clangAST clangBasic)
kz_use_llvm(kz-instr-clang)
kz_apply_settings(kz-instr-clang)
target_compile_options(kz-instr-clang PRIVATE "$<$<CXX_COMPILER_ID:GNU>:-Wno-nonnull>")
set(kz_instr_resource_dir "${KZ_LLVM_BUILD_DIR}/lib/clang/${LLVM_VERSION_MAJOR}")
target_compile_definitions(kz-instr-clang PRIVATE "KZI_DEFAULT_RESOURCE_DIR=\"${kz_instr_resource_dir}\"")

# The transform on a small C program (tests/: side effects in arguments, a variadic callee, a macro call site,
# return paths in macros, a skipped region), then gcc with the C1 reference warning flags, the runtime, one run
# and kz-rt-dump against the manifests.
add_test(NAME instr_clang_transform
  COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_transform_test.sh
          $<TARGET_FILE:kz-instr-clang> ${CMAKE_C_COMPILER} ${CMAKE_CURRENT_SOURCE_DIR}/tests
          ${CMAKE_SOURCE_DIR}/runtime/include $<TARGET_FILE:kzrt-posix> $<TARGET_FILE:kzrt>
          "$<TARGET_OBJECTS:kzrt-posix-auto>" $<TARGET_FILE:kz-rt-dump> ${CMAKE_CURRENT_BINARY_DIR}/transform_test)
set_tests_properties(instr_clang_transform PROPERTIES LABELS "instr")

# kz-llvm: the analysis layer that links LLVM and Clang (SPEC 4.1, 4.2).  Built only with
# -DKZ_BUILD_LLVM_TOOLS=ON (the top-level CMakeLists skips this directory otherwise).
#
#   model, spec, finalize, serialize, facts_text    no Clang types: the facts model, what to look
#                                                   for, whole-program resolution, the KZ
#                                                   container, the text view
#   p0, wrappers                                    no Clang types: the built-in table of library
#                                                   primitives and the operations the profile-free
#                                                   mode derives from it through wrappers
#   ast_*                                           the Clang front end of the extraction
#   basis_*                                         the analysis basis B (SPEC 1.7)
add_library(kz-llvm STATIC
  model.cpp
  spec.cpp
  p0.cpp
  wrappers.cpp
  finalize.cpp
  serialize.cpp
  facts_text.cpp
  facts_query.cpp
  unroll.cpp
  elf_info.cpp
  linkset.cpp
  gitrev.cpp
  recipe.cpp
  profile_load.cpp
  basis.cpp
  ast_tu.cpp
  ast_fn.cpp
  ast_run.cpp
  ast_basis.cpp)
target_include_directories(kz-llvm PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(kz-llvm PUBLIC kz-core clangTooling clangFrontend clangAnalysis clangAST clangLex clangBasic)
kz_use_llvm(kz-llvm)
kz_apply_settings(kz-llvm)

# gcc 13 reports a false 'this pointer is null' (-Wnonnull) from Clang's RecursiveASTVisitor
# (LazyOffsetPtr::get, inlined into TraverseCXXRecordHelper) in code that instantiates the visitor.
# The warning is in the LLVM headers, not in Kairos code; it is switched off for gcc only.
target_compile_options(kz-llvm PRIVATE "$<$<CXX_COMPILER_ID:GNU>:-Wno-nonnull>")

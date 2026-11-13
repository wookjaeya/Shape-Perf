# kz-static: the static analysis front end (SPEC 4.1, 4.2).  Built only with
# -DKZ_BUILD_LLVM_TOOLS=ON.
#
#   kz-static basis    record the analysis basis B of a closed program (a BASIS container)
#   kz-static extract  read the program's translation units and write the facts (a FACTS container)
#   kz-static show     print the text view of a BASIS or FACTS container
#   kz-static reach    which call sites that pass a value derived from a location are reachable from roots
#   kz-static relations  the relations of paths A/B/C and the outcome-flip signal, without a case input
#
# The tool names no Clang type: the Clang side is in lib/llvm.  The default location of Clang's
# builtin headers (-resource-dir) comes from the pinned build and is a compile definition.
add_executable(kz-static
  main.cpp
  cmd_basis.cpp
  cmd_extract.cpp
  cmd_show.cpp)
target_link_libraries(kz-static PRIVATE kz-llvm)
set(kz_resource_dir "${KZ_LLVM_BUILD_DIR}/lib/clang/${LLVM_VERSION_MAJOR}")
if(NOT IS_DIRECTORY "${kz_resource_dir}/include")
  message(WARNING "kz-static: no Clang builtin headers at ${kz_resource_dir}/include; pass --resource-dir")
  set(kz_resource_dir "")
endif()
target_compile_definitions(kz-static PRIVATE "KZS_DEFAULT_RESOURCE_DIR=\"${kz_resource_dir}\"")
kz_apply_settings(kz-static)
kz_tool_identity(kz-static)  # after target_link_libraries: defines KZ_TOOL_SOURCE_SHA256

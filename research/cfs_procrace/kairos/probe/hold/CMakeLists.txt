# The hold: the F0 control mechanism (kz hold).  ptrace with hardware debug-register breakpoints,
# thread auto-attach, event-based release.  Linux only; no library beyond libc (P-LIBBPF: the
# existing environment, as the loader).  It links the plan and anchor libraries to turn an
# observation profile's sites into the file offsets it gates on.  rt_plan.cpp converts the same plans for the
# instrumented family's cooperative gate (kz rtplan: kzrt-plan 1 from a build's site manifest and binding list).
add_library(kz-hold STATIC
  hold_plan.cpp
  rt_plan.cpp
  supervisor.cpp)
target_include_directories(kz-hold PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/include")
target_compile_features(kz-hold PUBLIC cxx_std_17)
target_compile_options(kz-hold PRIVATE -fno-exceptions)
target_link_libraries(kz-hold PUBLIC kz-plan kz-anchor kz-trace kz-core kzrt-manifest)
kz_apply_settings(kz-hold)

# libkz-trace: offline decoding of observation streams (decode_*), the identity join
# (identity_*), the neutral event layer and its converters (neutral, kzrt_convert), the real-record trace of a run
# container and the replay output container.  Standard library, libkz-core and the kzrt host reader only; no LLVM; no exceptions.  The HB and
# classification parts of lib/trace belong to later stages and add their own files here.
add_library(kz-trace STATIC
  decode_merge.cpp
  decode_match.cpp
  decode_perf.cpp
  decode_sites.cpp
  hold_nctl.cpp
  identity_join.cpp
  identity_rule.cpp
  identity_symbols.cpp
  kzrt_convert.cpp
  neutral.cpp
  occ_table.cpp
  real_trace.cpp
  replay_container.cpp
  static_closure.cpp)
target_include_directories(kz-trace PUBLIC "${CMAKE_SOURCE_DIR}/include")
target_compile_features(kz-trace PUBLIC cxx_std_17)
target_compile_options(kz-trace PRIVATE -fno-exceptions)
target_link_libraries(kz-trace PUBLIC kz-core)
# The host reader of the instrumented family's dump and the site manifest parser (runtime/tools; host code, not the
# target runtime, which kz never links): the kzrt-dump -> neutral layer conversion (kzrt_convert.cpp).
target_link_libraries(kz-trace PRIVATE kzrt-decode kzrt-manifest)
kz_apply_settings(kz-trace)
set_target_properties(kz-trace PROPERTIES POSITION_INDEPENDENT_CODE ON)
kz_declare_sources(kz-trace PATHS include/kz/trace)

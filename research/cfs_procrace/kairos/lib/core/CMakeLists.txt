# libkz-core: KZ container I/O, hashing, canonical dump, status/verdict records.
# Standard library only; no LLVM; no exceptions (it is also linked into LLVM tools).
add_library(kz-core STATIC
  container_reader.cpp
  container_writer.cpp
  crc32.cpp
  dump.cpp
  identity.cpp
  registry.cpp
  schema_tables.cpp
  sha256.cpp
  status.cpp
  verdict.cpp)
# The profile language (phi_*) and its evaluator (eval_*): parser, checker, canonical printer,
# provenance, the one evaluator with its concrete and abstract domains, and the rule engine.
target_sources(kz-core PRIVATE
  eval_abstract.cpp
  eval_concrete.cpp
  eval_trace.cpp
  phi_allowed.cpp
  phi_check.cpp
  phi_expr.cpp
  phi_lexer.cpp
  phi_parser.cpp
  phi_print.cpp
  phi_profile.cpp
  phi_provenance.cpp
  phi_stats.cpp)
target_include_directories(kz-core PUBLIC "${CMAKE_SOURCE_DIR}/include")
target_compile_features(kz-core PUBLIC cxx_std_17)
target_compile_options(kz-core PRIVATE -fno-exceptions)
kz_apply_settings(kz-core)
set_target_properties(kz-core PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_definitions(kz-core PUBLIC KZ_TOOL_VERSION="${KZ_TOOL_VERSION}")
kz_declare_sources(kz-core PATHS ${KZ_CORE_SOURCE_PATHS})

# The source identity of libkz-core (SHA-256 over the sources, headers and build recipe it is
# made from, see cmake/KzSourceIdentity.cmake) goes into the tool hash.  The definition
# applies to identity.cpp only, so that a source change recompiles one file.
kz_source_identity(kz_core_source_sha256 PATHS ${KZ_CORE_SOURCE_PATHS})
set_source_files_properties(identity.cpp PROPERTIES COMPILE_DEFINITIONS "KZ_CORE_SOURCE_SHA256=\"${kz_core_source_sha256}\"")
message(STATUS "libkz-core source identity ${kz_core_source_sha256}")

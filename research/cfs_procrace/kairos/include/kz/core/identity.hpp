// identity.hpp -- what a tool build is, for the tool hash in every container header.
//
// The tool hash identifies the SOURCE of the tool that wrote (or accepted) an artifact, not
// the compiler or the flags: the same sources give the same hash with gcc and clang
// (compiler and language level are a separate BASIS item, kz::build_identity()).  It is the
// SHA-256 of these length-framed fields:
//
//   domain tag "kz-tool/2", tool name, tool version (KZ_TOOL_VERSION, a human label),
//   ABI version, schema fingerprint (hash of every record layout, enum and format),
//   core source identity (the sources of libkz-core), tool source identity (the sources of
//   everything compiled into the tool).
//
// A "source identity" is computed by CMake at configure time (cmake/KzSourceIdentity.cmake):
// the SHA-256 of the text lines "<path relative to the source root> <sha256 of the file>\n",
// sorted by path, over the source and header files of the paths it covers.  It does not
// depend on where the tree is checked out.  A build that differs in any covered file has a
// different tool hash, so two builds with different writer, verdict or checker logic never
// share a hash even when the version label is unchanged.
//
// Limit: the identity covers files, not the toolchain.  Two compilers can produce binaries
// that behave differently from the same sources; that is what build_identity() records.
#pragma once

#include <string>
#include <string_view>

#include "kz/core/sha256.hpp"

namespace kz {

// Hex SHA-256 (64 lower-case digits) of the sources of libkz-core, fixed when the library was
// configured.
std::string_view core_source_identity();

// True for exactly 64 lower-case hex digits.
bool is_hex_digest(std::string_view text);

// The inputs of a tool hash.  Exposed so that tests can vary each one on its own.
struct ToolHashParts {
  std::string_view tool_name;
  std::string_view version;
  uint32_t abi_version = 0;
  Digest schema_fingerprint = {};
  std::string_view core_source;  // core_source_identity()
  std::string_view tool_source;  // KZ_TOOL_SOURCE_SHA256 of the tool; empty for a tool with no code of its own
};
Digest tool_hash_from_parts(const ToolHashParts& parts);

// The tool hash of the running build: `tool_source_sha256` is the identity of the tool's own
// sources and of every library it links (CMake: kz_tool_identity(<target>) defines
// KZ_TOOL_SOURCE_SHA256 for the target; use KZ_THIS_TOOL_HASH below).
Digest tool_hash(std::string_view tool_name, std::string_view version, std::string_view tool_source_sha256);

// Core sources only.  This covers libkz-core but not the tool's own code: use it for the
// test tools of the foundation, and use the three-argument form in every real tool.
Digest tool_hash(std::string_view tool_name, std::string_view version);

}  // namespace kz

// The tool hash of the tool that this translation unit belongs to.  KZ_TOOL_VERSION comes
// from libkz-core's usage requirements and KZ_TOOL_SOURCE_SHA256 from kz_tool_identity(); a
// target that forgot the latter does not compile.
#define KZ_THIS_TOOL_HASH(tool_name) ::kz::tool_hash((tool_name), KZ_TOOL_VERSION, KZ_TOOL_SOURCE_SHA256)

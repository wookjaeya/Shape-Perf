// identity.cpp -- tool hash and the source identity of libkz-core.
//
// KZ_CORE_SOURCE_SHA256 is defined by lib/core/CMakeLists.txt for this file only (a compile
// definition computed from the sources by cmake/KzSourceIdentity.cmake, not generated code).

#include "kz/core/identity.hpp"

#include <string>

#include "kz/core/dump.hpp"
#include "kz/schema/kz_schema.h"

#ifndef KZ_CORE_SOURCE_SHA256
#error "KZ_CORE_SOURCE_SHA256 must be defined for identity.cpp (see lib/core/CMakeLists.txt)"
#endif

namespace kz {

namespace {

// Length-framed field: "<decimal length>:<bytes>\n", so that no concatenation of two
// fields can be read as another pair.
void put_field(Sha256* h, std::string_view value) {
  h->update(std::to_string(value.size()));
  h->update(std::string_view(":"));
  h->update(value);
  h->update(std::string_view("\n"));
}

}  // namespace

std::string_view core_source_identity() { return KZ_CORE_SOURCE_SHA256; }

bool is_hex_digest(std::string_view text) {
  if (text.size() != 64) return false;
  for (char c : text)
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
  return true;
}

Digest tool_hash_from_parts(const ToolHashParts& p) {
  Sha256 h;
  put_field(&h, "kz-tool/2");
  put_field(&h, p.tool_name);
  put_field(&h, p.version);
  put_field(&h, std::to_string(p.abi_version));
  put_field(&h, to_hex(p.schema_fingerprint));
  put_field(&h, p.core_source);
  put_field(&h, p.tool_source);
  return h.finish();
}

Digest tool_hash(std::string_view tool_name, std::string_view version, std::string_view tool_source_sha256) {
  ToolHashParts parts;
  parts.tool_name = tool_name;
  parts.version = version;
  parts.abi_version = KZ_ABI_VERSION;
  parts.schema_fingerprint = schema_fingerprint();
  parts.core_source = core_source_identity();
  parts.tool_source = tool_source_sha256;
  return tool_hash_from_parts(parts);
}

Digest tool_hash(std::string_view tool_name, std::string_view version) { return tool_hash(tool_name, version, std::string_view()); }

}  // namespace kz

// The tool hash: what it is made of, that each input changes it, and that the source identity
// that CMake compiled into libkz-core is the identity of the sources in the tree now (so a
// build that is stale, or an identity algorithm that diverges from the documented one, fails).

#include <string>

#include "kz/core/dump.hpp"
#include "kz/core/identity.hpp"
#include "support/kz_test.hpp"
#include "support/source_identity.hpp"

#ifndef KZ_SOURCE_ROOT_DIR
#error "KZ_SOURCE_ROOT_DIR is defined by tests/unit/CMakeLists.txt"
#endif

namespace {

const std::string kA(64, 'a');
const std::string kB(64, 'b');

kz::ToolHashParts base() {
  kz::ToolHashParts p;
  p.tool_name = "kz-unit-test";
  p.version = "0";
  p.abi_version = 1;
  p.schema_fingerprint = kz::sha256("layout");
  p.core_source = kA;
  p.tool_source = kB;
  return p;
}

void test_every_input_changes_the_hash() {
  const kz::Digest h = kz::tool_hash_from_parts(base());
  KZ_CHECK(h == kz::tool_hash_from_parts(base()));
  kz::ToolHashParts p = base();
  p.tool_name = "kz-unit-test2";
  KZ_CHECK(h != kz::tool_hash_from_parts(p));
  p = base();
  p.version = "1";
  KZ_CHECK(h != kz::tool_hash_from_parts(p));
  p = base();
  p.abi_version = 2;
  KZ_CHECK(h != kz::tool_hash_from_parts(p));
  p = base();
  p.schema_fingerprint = kz::sha256("another layout");  // any record layout, enum or format changed
  KZ_CHECK(h != kz::tool_hash_from_parts(p));
  p = base();
  p.core_source = kB;  // the sources of libkz-core changed
  KZ_CHECK(h != kz::tool_hash_from_parts(p));
  p = base();
  p.tool_source = kA;  // the sources of the tool or of a library it links changed
  KZ_CHECK(h != kz::tool_hash_from_parts(p));
  p = base();
  p.tool_source = "";  // a tool with no source identity is not the same as one with
  KZ_CHECK(h != kz::tool_hash_from_parts(p));
  // The fields are framed: moving a character from one field to the next changes the hash.
  kz::ToolHashParts x = base(), y = base();
  x.tool_name = "ab";
  x.version = "c";
  y.tool_name = "a";
  y.version = "bc";
  KZ_CHECK(kz::tool_hash_from_parts(x) != kz::tool_hash_from_parts(y));
}

void test_running_build() {
  const std::string_view core = kz::core_source_identity();
  KZ_CHECK(kz::is_hex_digest(core));
  KZ_CHECK(!kz::is_hex_digest(""));
  KZ_CHECK(!kz::is_hex_digest(std::string(63, 'a')));
  KZ_CHECK(!kz::is_hex_digest(std::string(64, 'A')));  // lower case only
  KZ_CHECK(!kz::is_hex_digest(std::string(63, 'a') + "g"));

  // tool_hash() is the parts-based hash of this build.
  kz::ToolHashParts p;
  p.tool_name = "kz-unit-test";
  p.version = "0";
  p.abi_version = KZ_ABI_VERSION;
  p.schema_fingerprint = kz::schema_fingerprint();
  p.core_source = core;
  p.tool_source = kB;
  KZ_CHECK(kz::tool_hash("kz-unit-test", "0", kB) == kz::tool_hash_from_parts(p));
  p.tool_source = "";
  KZ_CHECK(kz::tool_hash("kz-unit-test", "0") == kz::tool_hash_from_parts(p));
  KZ_CHECK(kz::tool_hash("kz-unit-test", "0") == kz::tool_hash("kz-unit-test", "0", ""));
  // The tool's own sources and the version label both matter; the two forms differ.
  KZ_CHECK(kz::tool_hash("kz-unit-test", "0", kA) != kz::tool_hash("kz-unit-test", "0", kB));
  KZ_CHECK(kz::tool_hash("kz-unit-test", "0", kA) != kz::tool_hash("kz-unit-test", "0"));
  KZ_CHECK(kz::tool_hash("kz-unit-test", "0", kA) != kz::tool_hash("kz-unit-test", "1", kA));
  // The provenance text carries the source identity next to the compiler.
  KZ_CHECK(kz::build_identity().find(std::string("source=") + std::string(core)) != std::string::npos);
}

// The identity that CMake computed at configure time equals an independent computation over
// the files of the tree now.
void test_core_identity_matches_the_tree() {
  const std::vector<std::string> paths = kzfix::split_paths(KZ_CORE_SOURCE_PATHS_TEXT);
  KZ_CHECK(paths.size() >= 4);
  const std::string again = kzfix::recompute_source_identity(KZ_SOURCE_ROOT_DIR, paths);
  KZ_CHECK(!again.empty());
  KZ_CHECK_EQ(again, std::string(kz::core_source_identity()));
  // The paths cover what libkz-core is made of.
  bool has_sources = false, has_core_headers = false, has_schema = false;
  for (const std::string& q : paths) {
    has_sources = has_sources || q == "lib/core";
    has_core_headers = has_core_headers || q == "include/kz/core";
    has_schema = has_schema || q == "include/kz/schema";
  }
  KZ_CHECK(has_sources && has_core_headers && has_schema);
  // A path that does not exist gives no identity instead of a hash of nothing.
  KZ_CHECK(kzfix::recompute_source_identity(KZ_SOURCE_ROOT_DIR, {"lib/does-not-exist"}).empty());
}

}  // namespace

int main() {
  test_every_input_changes_the_hash();
  test_running_build();
  test_core_identity_matches_the_tree();
  return kztest::finish("test_core_identity");
}

// fixtures.hpp -- deterministic fixtures shared by the unit and golden tests.
//
// Everything here is synthetic: names such as "example_publish" stand for nothing, and
// numbers are unit-test values, not research conditions.
#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "kz/core/container.hpp"

namespace kzfix {

// Small deterministic generator (no <random>: distributions differ between standard libraries).
struct Rng {
  explicit Rng(uint64_t seed) : s(seed * 6364136223846793005ull + 1442695040888963407ull) {}
  uint32_t next() {
    s = s * 6364136223846793005ull + 1442695040888963407ull;
    return static_cast<uint32_t>(s >> 33);
  }
  uint64_t s;
};

// One record of `rec`, every non-reserved field filled with a deterministic value that is
// valid for its format (listed enum values, existing section ids in references).  Strings
// are interned into `w` when given; otherwise they are the empty string.  References are
// null or point into the artifact `foreign`.
kz::Bytes make_record(const kz_record_desc& rec, uint32_t seed, kz::ContainerWriter* w, const kz::Digest& foreign);

struct SectionBytes {
  uint32_t type;
  kz::Bytes data;  // the records as given to the writer
};

// A container with `n` pattern records in every registered section except STRINGS, and
// `foreign` as its only parent.  `given` (optional) receives the record bytes of each section.
kz::Bytes make_all_sections_container(size_t n, const kz::Digest& foreign, kz::Digest* tool, std::vector<SectionBytes>* given = nullptr);

template <class T>
T zero() {
  T t;
  std::memset(&t, 0, sizeof(t));
  return t;
}

// Meaningful fixtures (kinds FACTS, RUN and VERDICTS; the last has the first two as parents).
kz::Bytes build_facts_fixture();
kz::Bytes build_run_fixture();
kz::Bytes build_verdicts_fixture(const kz::Digest& facts, const kz::Digest& run);

// The tool hash written into every fixture container.  A fixed constant, not kz::tool_hash():
// the real tool hash changes with the source identity and with every record layout, and the
// container goldens must change only when the container format or the fixture content does.
// The dependence of the tool hash on its inputs is tested by test_core_identity and by the
// golden case `tool_hash`.
kz::Digest fixture_tool_hash();

// Unwraps a Status in a fixture builder (a failure is a bug in the fixture).
void must(const kz::Status& s, const char* what);

}  // namespace kzfix

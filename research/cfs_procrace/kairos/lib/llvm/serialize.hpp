// serialize.hpp -- the extracted facts as a KZ container (artifact kind FACTS).
//
// One call writes every section of the facts layer (kz_facts.h) from a finalized Model:
// FUNCS, FACTS and FACT_ARGS, DERIVES, FACT_EXPRS, FACT_LOOPS, FACT_INIT, FACT_CONSTS, WRITERS,
// FACT_ATYPE, CALL_TARGETS, STATIC_IMAGE, FACT_LOCS, TERMINALS, OUTCALLS, plus FILE_HASHES (the
// SHA-256 of every file a fact points into), BASIS (the extraction parameters and counts) and
// PROVENANCE (one record per hand-entered input).  The header carries the hash of the basis
// container the facts were read under (also listed as a parent) and the profile hash.
#pragma once

#include <string>
#include <vector>

#include "kz/core/container.hpp"
#include "model.hpp"
#include "spec.hpp"

namespace kzs {

constexpr uint32_t kParentRoleBasis = 1;

struct SerializeOptions {
  std::string root;        // where the recorded (relative) file paths are read from, to hash them
  bool has_basis = false;
  kz::Digest basis_hash = {};    // the artifact hash of the basis container
  kz::Digest profile_hash = {};
  kz::Digest tool_hash = {};
  std::string build_identity;    // kz::build_identity() of the running tool
  std::vector<std::pair<std::string, std::string>> notes;  // further BASIS items (name, value), policy unset
};

kz::Status serialize_facts(const Model& model, const ExtractSpec& spec, const SerializeOptions& options, kz::Bytes* out);

// SHA-256 of a file, and its size.
kz::Status hash_file(const std::string& path, kz::Digest* digest, uint64_t* size);

}  // namespace kzs

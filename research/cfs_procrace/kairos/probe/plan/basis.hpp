// basis.hpp -- the anchor table as a basis container, and the plan the observer derives from it.
//
// `build_basis` reads the traced binary and a profile and writes a KZ container of kind BASIS:
//   BASIS        items: binary.path, binary.sha256, binary.build_id, binary.size, binary.type, ...
//   FILE_HASHES  the binary (path, size, SHA-256)
//   SYMBOLS      every function symbol (for naming code by address, offline)
//   TARGETS      one resolved probe target per profile site (function/line -> file offset)
//   PROVENANCE   the profile's provenance entries, with the hash of the cited text where the
//                source tree was given
// The header carries the profile's SHA-256, so a basis belongs to exactly one profile text.
//
// `make_plan` joins profile and basis into the SiteSpec list the observer attaches, refusing any
// disagreement; `verify_binary` is the check the observer makes before it attaches anything:
// the file it will probe is byte for byte the file the anchors were resolved in.
#pragma once

#include <string>
#include <utility>
#include <vector>

#include "kz/core/container.hpp"
#include "kz/trace/decode_sites.hpp"
#include "profile.hpp"

namespace kz::plan {

struct BasisOptions {
  std::string binary_path;   // recorded as binary.path
  std::string source_root;   // optional: directory that the profile's cited files are relative to (root "")
  std::vector<std::pair<std::string, std::string>> roots;  // further named roots: (name, directory)
  Digest tool_hash = {};
  std::string build_identity;  // recorded as tool.build_identity; empty: the identity of this build of the tool (compiler, source hash)
};

struct BasisReport {
  size_t targets = 0, symbols = 0;
  size_t tier_count[6] = {0, 0, 0, 0, 0, 0};  // provenance entries per tier, in KZ_TIER order
  size_t prov_verified = 0;    // the declared hash equals the hash of the cited lines read from the source root
  size_t prov_computed = 0;    // no hash declared; the hash of the cited lines was computed and recorded
  size_t prov_declared = 0;    // a hash is declared but the source root was not given: recorded, not verified
  size_t prov_unhashed = 0;    // neither: the cited text is not hashed
  size_t sites_by_tier[6] = {0, 0, 0, 0, 0, 0};
};

Status build_basis(const Bytes& binary, const Profile& profile, const BasisOptions& options, Bytes* out, BasisReport* report);

struct LoadedBasis {
  Container container;
  std::string binary_path;
  std::string build_id;
  Digest binary_sha256 = {};
  uint64_t binary_size = 0;
  std::vector<kz_target> targets;
  std::vector<std::string> target_labels, target_funcs, target_files;
};

Status load_basis(const std::string& path, LoadedBasis* out);
Status load_basis_bytes(Bytes bytes, LoadedBasis* out);

// The sites to attach, one per profile site, in profile order.
Status make_plan(const Profile& profile, const LoadedBasis& basis, std::vector<kz::trace::SiteSpec>* sites);

// Compares a binary (its full contents) with the basis: size, SHA-256, build-id, and the bytes at
// every target.  All differences are listed.
Status verify_binary(const Bytes& binary, const LoadedBasis& basis);

// The SHA-256 of the plan: the canonical bytes of the site and slot records without the fields
// that are only known at attach time (inode, device).
Digest plan_hash(const std::vector<kz::trace::SiteSpec>& sites);

// Site and slot records for a run container (inode/device/path filled by the caller).
void site_records(const std::vector<kz::trace::SiteSpec>& sites, ContainerWriter* w, std::vector<kz_site>* out_sites,
                  std::vector<kz_site_slot>* out_slots);

}  // namespace kz::plan

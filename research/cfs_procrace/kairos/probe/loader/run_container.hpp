// run_container.hpp -- the `.kzt` run container of an observation (SPEC 5.5).
//
//   SITES, SITE_SLOTS   the plan that was attached, with the identity of the probed file
//   RAW_EVENTS          the probe hits, in the merged order, with per-thread sequence numbers
//   SIDE_EVENTS, MAPS   FORK/EXIT/COMM of the traced processes, and the mappings of the traced file
//   THREADS             the identity join
//   LOSS                the kernel's loss reports (empty means none were reported, not that the
//                       decoder proved there was none: see kz_raw.h)
//   RUNMETA, PERTURBATION, BASIS items, DEVIATIONS, FILE_HASHES
// The parent is the basis container the anchors came from.  Content is a function of the run's
// records and the caller's record: nothing about wall-clock time is written.
#pragma once

#include <string>
#include <vector>

#include "kz/trace/decode_merge.hpp"
#include "kz/trace/identity_join.hpp"
#include "perf_session.hpp"
#include "run_record.hpp"

namespace kz::probe {

struct RunInputs {
  Digest basis_artifact = {};
  Digest profile_sha256 = {};
  Digest tool_hash = {};
  std::vector<kz::trace::SiteSpec> sites;
  std::string exe_path;
  uint64_t inode = 0, device = 0;
  uint8_t build_id[20] = {0};
  uint64_t exe_size = 0;
  Digest exe_sha256 = {};
  kz::trace::DecodedRun decoded;
  std::vector<kz::trace::ThreadRecord> threads;
  SessionStats session;
  RunRecord record;
  uint32_t run_ordinal = 0;
  uint8_t stratum = KZ_STRATUM_NONE;
  std::string kernel, harness, placement;
  uint64_t observer_cpu_ns = 0, boot_ns = 0;
  uint16_t run_flags = 0;
};

Status write_run_container(const RunInputs& in, Bytes* out);

// Hex text (40 digits for a 20-byte build id) to bytes; false if the text is not that long.
bool build_id_from_hex(const std::string& hex, uint8_t out[20]);

}  // namespace kz::probe

// run_container.hpp -- the `.kzt` run container of an observation (SPEC 5.5).
//
//   SITES, SITE_SLOTS   the plan that was attached, with the identity of the probed file
//   RAW_EVENTS          the probe hits, in the merged order, with per-thread sequence numbers
//   SIDE_EVENTS, MAPS   FORK/EXIT/COMM of the traced processes, and the mappings of the traced file
//   THREADS             the identity join
//   LOSS                the kernel's loss reports (empty means none were reported, not that the
//                       decoder proved there was none: see kz_raw.h)
//   RUNMETA, PERTURBATION, BASIS items, DEVIATIONS, FILE_HASHES
//                       (PERTURBATION: one probe-hits row per site, a return site with cost class
//                       ret-trampoline; then observer-cpu, boot-time and proc-capture; where each
//                       site's trap is counted follows the rule of kz_raw.h and is stated in the
//                       record, items perturbation.site<N>.trap)
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

// A further file of a plan over several files (a shared object the process loads): its identity as
// the observer verified it, written with the sites in it and into FILE_HASHES.
struct FileIdentity {
  std::string path;
  uint64_t inode = 0, device = 0;
  uint8_t build_id[20] = {0};
  uint64_t size = 0;
  Digest sha256 = {};
};

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
  std::vector<FileIdentity> also;  // SiteSpec::binary 1, 2, ...
  kz::trace::DecodedRun decoded;
  std::vector<kz::trace::ThreadRecord> threads;
  std::vector<kz::trace::OwnerRecord> owners;  // app_owner per thread (ACTOR_OWNERS); empty: the section is not written
  SessionStats session;
  RunRecord record;
  uint32_t run_ordinal = 0;
  uint8_t stratum = KZ_STRATUM_NONE;
  std::string kernel, harness, placement;
  uint64_t observer_cpu_ns = 0, boot_ns = 0;
  uint64_t proc_reads = 0, proc_ns = 0;  // the observer's /proc reads while the target ran (proc_capture.hpp)
  uint16_t run_flags = 0;
};

Status write_run_container(const RunInputs& in, Bytes* out);

// For every site, where the trap at its file offset is counted (the accounting rule of kz_raw.h,
// PERTURBATION): CountedHere for the first ENTRY or POINT site at that offset, InOtherRow for every
// other site at an offset that has one, Uncounted for a site at an offset with only RET sites (their
// entry trap is in no row).
enum class TrapAccount { CountedHere, InOtherRow, Uncounted };
std::vector<TrapAccount> trap_accounts(const std::vector<kz::trace::SiteSpec>& sites);
// The same as a sentence for the run record (item perturbation.site<N>.trap).
std::string trap_account_text(const std::vector<kz::trace::SiteSpec>& sites, size_t i);

// Hex text (40 digits for a 20-byte build id) to bytes; false if the text is not that long.
bool build_id_from_hex(const std::string& hex, uint8_t out[20]);

}  // namespace kz::probe

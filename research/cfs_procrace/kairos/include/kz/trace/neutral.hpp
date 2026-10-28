// neutral.hpp -- the neutral event layer of a run (schema revision 3, kz/schema/kz_neutral.h) in memory,
// and its converters.
//
//   neutral_from_f0     F0's run container sections (kz_raw.h) -> the neutral layer.  This is where F0's
//                       kernel-thread identity becomes an actor: the incarnation of a tid alive at an event
//                       (the identity join's THREADS, by FORK time and generation), the creating executor,
//                       the owner record, the creating call's return bound by the id, the initial thread,
//                       and the loss intervals.  The rules are the ones the replay applied to the F0 sections
//                       directly before revision 3 (real_trace.hpp), moved here unchanged.
//   neutral_from_kzrt   a kzrt-dump stream (runtime/include/kz_rt_format.h) with its site manifests
//                       ("kzs-sites 1" or "2") -> the neutral layer (kzrt_convert.cpp; its identity join:
//                       identity_rule.hpp).
//   read_neutral        the neutral sections of a container; neutral_of_run reads them when present and
//                       otherwise converts the F0 sections in memory (a container written before revision 3
//                       is read as it was written).
//   add_neutral_sections  writes the layer into a container writer (the explicit little-endian codec).
//   convert_f0_container  a converted run container: every section of the input unchanged (the backend raw
//                       evidence), the neutral layer beside it, the input as a parent.
//
// Strings are kept as std::string beside each record; the records' kz_str fields mean nothing in memory and
// are set when the layer is written.  A kz_ref in memory is final: a reference into the container the layer
// is written to (F0: the raw sections copied with it).
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "kz/core/container.hpp"
#include "kz/core/sha256.hpp"
#include "kz/core/status.hpp"

namespace kz::trace {

struct NRun {
  kz_nrun r{};
  std::string backend_name, runtime, adapter, compiler, target;
};
struct NSite {
  kz_nsite r{};
  std::string label, op, symbol, source, at;
};
struct NSlot {
  kz_nslot r{};
  std::string role, from;
};
struct NValue {
  kz_nvalue r{};
  std::string text;
};
struct NActor {
  kz_nactor r{};
  std::string name, owner_app, request_func;
};

struct NeutralRun {
  NRun run;
  std::vector<NSite> sites;
  std::vector<NSlot> slots;
  std::vector<kz_nevent> events;
  std::vector<NValue> values;
  std::vector<NActor> actors;
  std::vector<kz_nloss> losses;
  std::vector<kz_nctl> ctl;
};

// A zeroed record with every index field that means "none" set to KZ_NONE32 (the neutral layer's defaults).
kz_nevent nevent_init();
kz_nactor nactor_init();
kz_nloss nloss_init();
kz_nctl nctl_init();
kz_ref self_ref(uint32_t section, uint32_t index);

// Structural checks of a layer: every index in range, values belong to their event, enums in range.
Status check_neutral(const NeutralRun& n);

Status neutral_from_f0(const Container& run, NeutralRun* out);
Status read_neutral(const Container& run, NeutralRun* out);
// The neutral layer of a run container: its neutral sections if it has them (*from_sections = true),
// else the in-memory conversion of its F0 sections.
Status neutral_of_run(const Container& run, NeutralRun* out, bool* from_sections);
bool has_neutral_sections(const Container& run);

Status add_neutral_sections(const NeutralRun& n, ContainerWriter* w);

// Copies every section of `src` into `w` (strings re-interned), unchanged otherwise; the sections in `except` are left
// out (the caller writes them).
Status copy_sections(const Container& src, ContainerWriter* w, const std::vector<uint32_t>& except = {});

// The converted container of an F0 run container (refused if it already has the neutral layer).
Status convert_f0_container(const Container& run, const Digest& tool_sha256, Bytes* out, NeutralRun* layer);

// The real-record view of the actors (report and replay): the actor's name in the trace.  F0's rule, made per
// binding: the backend's name, or <prefix><native key> without one (prefix "tid" for a Linux thread, "a" for a
// runtime actor, "P" for a provisional one, "?" unresolved); a name used by several join-record actors gets
// ".<prefix><key>"; a generation other than 0 gets ".g<generation>".
std::vector<std::string> actor_names(const NeutralRun& n);

// One-line text of a loss record (reports).
std::string loss_text(const kz_nloss& l);

}  // namespace kz::trace

// kzrt_convert.hpp -- a kzrt-dump stream (the instrumented family's raw evidence, runtime/include/kz_rt_format.h)
// with its site manifests ("kzs-sites 1", event_semantics.md section 3) -> the neutral event layer
// (neutral.hpp), so that `kz replay` reads an instrumented run as it reads an F0 run.
//
// What the conversion states (docs/schema.md section 12.4):
//   sites     one neutral site per manifest site (by id).  phase before/after is a call/return of `callee`
//             (flag call-site) or, without a callee, of `func`; effect/exit/point is a point.  `attached` iff the
//             site has a binding and every binding is bound, inlined or duplicated.  A site id the manifests
//             do not declare becomes an unmapped placeholder (a note).
//   slots     per `value`: role, `from`, kind (ptr string=validated: text; ptr pointer-only: pointer; else int).
//   actors    one per runtime actor (ACTOR records), with its native key and generation (binding kzrt-actor);
//             a provisional actor (before attach) whose cell a run actor took (ACTOR boot_link, evidence
//             same-cell) is joined to it: its events are the run actor's (the aseq continues); any other
//             provisional actor stays one (kzrt-provisional); every event without a cell (actor 0) is an actor
//             of its own (unresolved), so no program order is invented.  The runtime records no creator, owner
//             or name: they stay undetermined (the identity join over manifest roles is not done here).
//   events    per actor in aseq order; the container order is the events without a clock (bootstrap) in buffer
//             order, then a merge by time that keeps every actor's aseq order (NOT an order between actors).
//             Values: a slot whose valid bit is set; a validated string where the runtime copied one.  `link`
//             is the opening event of the same call instance (the before record's aseq).
//   losses    the runtime's LOSS records: overflow and partial of an actor's buffer from the actor's first aseq
//             gap (or its last event) to the end; preinit, partial bootstrap, excluded, unloaded, reentry,
//             note-table, an unexplained aseq gap and a truncated stream (no END, bad CRC): the whole record;
//             control losses (ctl-skipped, ctl-nocond), actor-table, unresolved, module-table and after-stop are
//             reported but are no interval (the events are recorded, or lie outside the observation window).
//   start     bootstrap-buffer evidence iff the runtime was initialized in record or control mode and no
//             pre-init or bootstrap partial loss was counted, in a complete stream.
//   control   CTL records -> NCTL (actors, the hit's observation event, site); POINT held_actor -> held-at-end.
#pragma once

#include <string>
#include <utility>
#include <vector>

#include "kz/core/container.hpp"
#include "kz/trace/neutral.hpp"

namespace kz::trace {

struct KzrtConvertInput {
  Bytes dump;
  std::string dump_name;                                    // recorded in FILE_HASHES
  std::vector<std::pair<std::string, std::string>> manifests;  // (name, text), read into one manifest in order
  const Container* basis = nullptr;  // optional basis container of the instrumented build (becomes the parent)
  Digest tool_sha256{};
};

struct KzrtConvertResult {
  NeutralRun layer;
  std::vector<std::string> notes;  // what the conversion could not state or found inconsistent
  Bytes container;                 // the run container (convert_kzrt_container only)
};

// The neutral layer of a dump.  Fails on a stream that is not a kzrt-dump of major 1 or is malformed, and on
// a manifest that does not parse; a truncated stream is converted as far as it goes, with its loss.
Status neutral_from_kzrt(const KzrtConvertInput& in, KzrtConvertResult* out);
// The same, written as a run container: BASIS (the conversion inputs), FILE_HASHES (dump, manifests), the layer.
Status convert_kzrt_container(const KzrtConvertInput& in, KzrtConvertResult* out);

}  // namespace kz::trace

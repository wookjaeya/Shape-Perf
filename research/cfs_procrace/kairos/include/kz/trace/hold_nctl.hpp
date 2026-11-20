// hold_nctl.hpp -- the realized control of an F0 hold run (`kz hold`, record hold.kzr) as the neutral control records
// NCTL of its run container (schema revision 4; docs/replay_output.md section 6, event_semantics.md sections 4-5).
//
// The F0 holder (probe/hold/supervisor.cpp) stops a thread with a debug-register trap at a control point's address,
// before the instruction there executes, and records what it did: per hold the gate, the thread, the gate's qualifying
// hit, the time it held it (`held_ns`) and the release that resumed it (`release_ns`); per release that acted the
// thread whose hit it was, the hit, the time (`t_ns`) and the thread it resumed, or that the gate held none
// (`satisfied` / `consumed`), and whether it acted on a held thread's own hit when that thread resumed (`deferred`).
// Its times are the supervisor's CLOCK_MONOTONIC, the clock of the observer's events (perf use_clockid).
//
// What this conversion adds to the F0 neutral layer (neutral_from_f0), per hold.kzr fact:
//   hold      NCTL gate-hold (actor = the held thread's actor) and, if released, resume (other = the releasing actor);
//   fire      NCTL fire, reason resumed / satisfied / consumed, flag deferred as recorded;
//   points    `point` = the control point's index, gates first then releases, each in the record's order (the order
//             kz rtplan gives the runtime's points); `gate` = the gate's index; `hit` as recorded; `site` = the NSITES
//             site the observer had at the point's binary and file offset (an entry or a point probe), or none;
//   ctx       undeclared (F0 checks no context class, event_semantics.md section 4);
//   ctl_seq   the supervisor's order: its records by time (gate-hold, then fire, then resume at the same time);
//   t, raw_off  the supervisor's time and the byte offset of the record's line in hold.kzr.
// The observation event of a hit (`event`): the trap stops the thread before the instruction its probe observes, so
// the thread's probe event of that hit is its first event after the trap.  For a hold: the held actor's first event
// with time after held_ns; the actor has no event between held_ns and release_ns (it was stopped); that event is at the
// gate's site.  For a fire: the hitting actor's first event with time after t_ns, at the release's site.  The time
// only names which of the actor's events is the hit's; the order the control makes is the release's, not the clock's.
// The actor of a thread id (a reused id has several incarnations, which do not overlap): the incarnation whose events
// span the fact's time; else, for a thread that ran on after it, the one whose first event is the earliest after it,
// and for a hold never released the one whose first event is the latest at or before it.
// Where any of this does not hold (no site at the address, no such event, another site, an event inside the hold, a
// thread id with no actor or with no fitting incarnation), the record keeps `event` none with flag no-observation-event and the
// reason is a note: no event is invented.  Every record whose event was found has flag observed-after-control: the
// replay then starts a control edge at the releasing actor's last event before the release hit's observation, and ends
// it at the held actor's gate observation itself (real_trace.cpp).
//
// Semantics (P11, PI_DECISIONS_RESOLVED_20261026.md section 3; kzp-hold 1): the control order is a basis kind of this
// controlled run only; a hold alone does not show that a value was absent; the replay still requires the observed
// start state, the writers, the resource generation and the observed control location.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "kz/core/container.hpp"
#include "kz/core/sha256.hpp"
#include "kz/core/status.hpp"
#include "kz/trace/neutral.hpp"

namespace kz::trace {

struct HoldPoint {
  std::string label, binary;
  bool release = false;
  uint64_t file_offset = 0;
  std::string releases;  // a release: the gate it releases
};
struct HoldFact {
  std::string gate;  // the gate's label
  uint32_t tid = 0;
  uint64_t hit = 0, held_ns = 0, release_ns = 0;
  bool released = false;
  std::string released_by;
  uint64_t offset = 0;  // the line's byte offset in the record
};
struct FireFact {
  std::string release, gate;
  uint32_t tid = 0, resumed = 0;  // resumed 0: none
  uint64_t hit = 0, t_ns = 0;
  bool satisfied = false, consumed = false, deferred = false;
  uint64_t offset = 0;
};
struct HoldRecord {
  bool debugregs_ok = false;
  std::string debugregs_error;  // the writer's message where debugregs_ok=no
  std::vector<HoldPoint> points;  // as listed
  std::vector<HoldFact> holds;
  std::vector<FireFact> fires;
};

// Parses a hold.kzr text (tools/kz/cmd_hold.cpp writes it).  The writer's lines of one key (primary_binary, child_pid,
// debugregs_ok, debugregs_error, wall_ns, child_exit, child_signal) are read as text to the end of the line.  Invalid
// for a line it does not understand.
Status parse_hold_record(const std::string& text, HoldRecord* out);

// Adds the NCTL records of `h` to the F0 layer `n` (neutral_from_f0 of the hold run's container) and sets NRUN
// control-recorded.  `notes` says, per fact, which event it was mapped to or why none.
Status add_hold_control(const HoldRecord& h, NeutralRun* n, std::vector<std::string>* notes);

// The converted container of an F0 run container with its hold record: convert_f0_container's container (every section
// unchanged, the neutral layer, the input as a parent) with NCTL from the record, BASIS items convert.hold.* (the
// record's sha256 and the notes) and the record in FILE_HASHES.
Status convert_f0_container_with_hold(const Container& run, const std::string& hold_name, const std::string& hold_text, const Digest& tool_sha256, Bytes* out,
                                      NeutralRun* layer, std::vector<std::string>* notes);

}  // namespace kz::trace

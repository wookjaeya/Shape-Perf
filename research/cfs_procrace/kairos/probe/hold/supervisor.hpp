// supervisor.hpp -- the F0 `kz hold` control mechanism (SPEC 4.13 "Mechanisms", "Attach strategy",
// "Release").
//
// The supervisor runs the target command as its child and seizes it with PTRACE_SEIZE, with
// PTRACE_O_TRACECLONE|FORK|VFORK|EXEC so that it follows the whole process tree down to the cFS
// application threads (the provider and the consumer are threads the core creates after the run
// starts).  On every traced thread of the cFS process it arms the gate and release addresses as
// *hardware execution breakpoints* in the debug registers (DR0..DR3, set through PTRACE_POKEUSER):
// the text on disk and in memory is never modified.  Only the thread that hits a gate is left
// stopped (held); the others run.  A gate that carries a condition (the semaphore name at an
// argument register, or the module the return address points into) holds only the call that meets
// it.  When a release site is hit (by any thread), the held thread of the gate it releases is
// resumed.  Every stop, every hold and its duration, and the attach of each new thread are recorded
// as the perturbation of the run (the holder cost and the ptrace interaction that SPEC 4.13 lists
// as unmeasured).
//
// The addresses are given per declared binary as a file offset; the supervisor turns each into a
// run-time address from /proc/<pid>/maps when the binary is mapped (the shared objects cf.so and
// bp.so are mapped by the loader before the app task runs).  A process is relevant only if it maps
// the primary binary (the cFS executable); other processes of the tree (the shell, setpriv, the
// observer) are followed but never gated.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "kz/core/status.hpp"

namespace kz::hold {

// A gate or release address, resolved from the profile/basis to a binary + file offset, with its
// condition turned into a register and offset (for a name string) or a binary (for the caller).
struct ControlPoint {
  std::string label;
  std::string binary;       // the declared binary basename this address is in (maps path basename)
  uint64_t file_offset = 0; // the probe target's file offset in that binary
  uint64_t vaddr = 0;       // link-time address (report only)
  bool is_release = false;
  std::string releases;     // (release only) the gate label it resumes
  bool occurrence_each = false;
  // name condition: the NUL-terminated string at [reg + off] must equal name_text
  bool match_name = false;
  int name_reg = -1;        // KZ_REG index 0..15
  int32_t name_off = 0;
  std::string name_text;
  // caller condition: the return address at the top of the stack must be in caller_binary's maps
  bool match_caller = false;
  std::string caller_binary;
};

struct HoldConfig {
  std::string primary_binary;          // the executable basename (relevance test)
  std::vector<ControlPoint> points;    // gates (is_release=false) and releases
  std::vector<std::string> argv;       // the command and its arguments
  bool verbose = false;                // print the timeline to stderr as it happens
};

// One recorded hold.
struct HoldEvent {
  std::string gate_label;
  int tid = 0;
  std::string name;         // the name argument read at the gate (if any)
  uint64_t t_hold_ns = 0;   // CLOCK_MONOTONIC when the thread was held
  uint64_t t_release_ns = 0;// when it was resumed (0 if never released)
  std::string release_label;// which release resumed it
  bool released = false;
};

struct HoldReport {
  bool debugregs_ok = true;        // false if the kernel refused a debug-register write
  std::string debugregs_error;     // the exact failure, when debugregs_ok is false
  int child_pid = 0;
  bool child_exited = false;
  int child_exit_code = -1;
  int child_term_signal = 0;
  uint64_t t_start_ns = 0, t_end_ns = 0;

  uint64_t total_stops = 0;        // ptrace stops handled across the whole tree
  uint64_t gate_traps = 0;         // hits at a gate address (held or not)
  uint64_t release_traps = 0;      // hits at a release address
  uint64_t threads_attached = 0;   // distinct traced threads seen
  uint64_t clone_attaches = 0;     // threads born under TRACECLONE (new-thread stops)
  uint64_t relevant_procs = 0;     // processes that mapped the primary binary (and were armed)
  uint64_t armings = 0;            // debug-register (re)arm operations
  std::vector<HoldEvent> holds;
  std::vector<std::string> notes;  // per-run notes (e.g. a binary never mapped)
};

// Runs the command under the hold supervisor.  Returns an error only for a setup failure (bad
// argv, fork/seize failure); a run in which the debug registers could not be used returns success
// with report->debugregs_ok == false and the exact failure in debugregs_error (SPEC: report what
// fails, do not silently fall back to int3 text patching).
Status run_hold(const HoldConfig& config, HoldReport* report);

// The index into x86-64 user_regs_struct for a KZ_REG register number (0..15); -1 if out of range.
// Exposed for the unit test.
long read_reg(const void* user_regs, int kz_reg);

}  // namespace kz::hold

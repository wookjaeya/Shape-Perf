// supervisor.cpp -- see supervisor.hpp.
#include "supervisor.hpp"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stddef.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace kz::hold {

namespace {

uint64_t mono_ns() {
  timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return static_cast<uint64_t>(ts.tv_sec) * 1000000000ull + static_cast<uint64_t>(ts.tv_nsec);
}

// One mapping line of /proc/<pid>/maps that we keep.
struct Mapping {
  uint64_t start = 0, end = 0, pgoff = 0;
  bool exec = false;
  std::string base;  // basename of the mapped file
};

std::string basename_of(const std::string& p) {
  size_t s = p.find_last_of('/');
  return s == std::string::npos ? p : p.substr(s + 1);
}

bool read_maps(pid_t pid, std::vector<Mapping>* out) {
  out->clear();
  char path[64];
  std::snprintf(path, sizeof path, "/proc/%d/maps", (int)pid);
  FILE* f = std::fopen(path, "re");
  if (!f) return false;
  char* line = nullptr;
  size_t n = 0;
  ssize_t len;
  while ((len = getline(&line, &n, f)) >= 0) {
    uint64_t a, b, off;
    char perms[8];
    int name_pos = 0;
    // start-end perms offset dev inode path
    if (std::sscanf(line, "%" SCNx64 "-%" SCNx64 " %7s %" SCNx64 " %*s %*s %n", &a, &b, perms, &off, &name_pos) < 4) continue;
    std::string name;
    const char* p = line + name_pos;
    while (*p == ' ') ++p;
    name = p;
    while (!name.empty() && (name.back() == '\n' || name.back() == ' ')) name.pop_back();
    if (name.empty() || name[0] == '[') continue;  // anonymous or special
    Mapping m;
    m.start = a;
    m.end = b;
    m.pgoff = off;
    m.exec = perms[2] == 'x';
    m.base = basename_of(name);
    out->push_back(std::move(m));
  }
  std::free(line);
  std::fclose(f);
  return true;
}

// Per-process state: whether it is relevant (maps the primary binary), the resolved run-time
// addresses of each control point, and the mappings (for the caller test).
struct ProcState {
  bool known = false;
  bool relevant = false;
  bool all_resolved = false;   // every point's binary is mapped
  std::vector<uint64_t> addr;  // run-time address per config point (0 = not yet mapped)
  std::vector<Mapping> maps;
};

// user_regs_struct field for a KZ_REG register number 0..15.
long reg_field(const user_regs_struct& r, int kz) {
  switch (kz) {
    case 0: return (long)r.rax;
    case 1: return (long)r.rbx;
    case 2: return (long)r.rcx;
    case 3: return (long)r.rdx;
    case 4: return (long)r.rsi;
    case 5: return (long)r.rdi;
    case 6: return (long)r.rbp;
    case 7: return (long)r.rsp;
    case 8: return (long)r.r8;
    case 9: return (long)r.r9;
    case 10: return (long)r.r10;
    case 11: return (long)r.r11;
    case 12: return (long)r.r12;
    case 13: return (long)r.r13;
    case 14: return (long)r.r14;
    case 15: return (long)r.r15;
    default: return 0;
  }
}

// Read a NUL-terminated string (bounded) from the tracee's memory.
std::string peek_string(pid_t tid, uint64_t addr, size_t maxlen = 128) {
  std::string out;
  for (size_t i = 0; i < maxlen; i += sizeof(long)) {
    errno = 0;
    long w = ptrace(PTRACE_PEEKDATA, tid, (void*)(addr + i), nullptr);
    if (w == -1 && errno != 0) break;
    char b[sizeof(long)];
    std::memcpy(b, &w, sizeof(long));
    for (size_t k = 0; k < sizeof(long); ++k) {
      if (b[k] == '\0') return out;
      out.push_back(b[k]);
    }
  }
  return out;
}

uint64_t peek_word(pid_t tid, uint64_t addr) {
  errno = 0;
  long w = ptrace(PTRACE_PEEKDATA, tid, (void*)addr, nullptr);
  return (uint64_t)w;
}

const size_t kDr0Off = offsetof(struct user, u_debugreg[0]);
const size_t kDr7Off = offsetof(struct user, u_debugreg[7]);

}  // namespace

long read_reg(const void* user_regs, int kz_reg) {
  if (kz_reg < 0 || kz_reg > 15) return 0;
  return reg_field(*reinterpret_cast<const user_regs_struct*>(user_regs), kz_reg);
}

Status run_hold(const HoldConfig& config, HoldReport* report) {
  *report = HoldReport();
  if (config.argv.empty()) return Status::error(Err::Invalid, "kz hold: no command to run");
  const size_t np = config.points.size();
  if (np == 0) return Status::error(Err::Invalid, "kz hold: the hold plan has no gate");
  {
    // one debug-register slot per distinct address (binary + file offset); points may share one
    std::vector<std::pair<std::string, uint64_t>> distinct;
    for (const ControlPoint& cp : config.points) {
      bool seen = false;
      for (const auto& d : distinct) seen = seen || (d.first == cp.binary && d.second == cp.file_offset);
      if (!seen) distinct.push_back({cp.binary, cp.file_offset});
    }
    if (distinct.size() > 4) return Status::error(Err::Invalid, "kz hold: more than four distinct control addresses do not fit in DR0..DR3");
  }
  report->point_hits.assign(np, 0);

  std::vector<char*> argv;
  for (const std::string& a : config.argv) argv.push_back(const_cast<char*>(a.c_str()));
  argv.push_back(nullptr);

  report->t_start_ns = mono_ns();
  const pid_t child = fork();
  if (child < 0) return Status::error(Err::Io, std::string("fork: ") + std::strerror(errno));
  if (child == 0) {
    if (ptrace(PTRACE_TRACEME, 0, 0, 0) < 0) _exit(126);
    raise(SIGSTOP);  // let the tracer set options before we exec
    execvp(argv[0], argv.data());
    _exit(127);
  }
  report->child_pid = child;

  int st = 0;
  if (waitpid(child, &st, 0) < 0) return Status::error(Err::Io, std::string("waitpid(initial): ") + std::strerror(errno));
  long opts = PTRACE_O_TRACECLONE | PTRACE_O_TRACEFORK | PTRACE_O_TRACEVFORK | PTRACE_O_TRACEEXEC | PTRACE_O_EXITKILL;
  if (ptrace(PTRACE_SETOPTIONS, child, 0, (void*)opts) < 0)
    return Status::error(Err::Io, std::string("PTRACE_SETOPTIONS: ") + std::strerror(errno));

  std::unordered_map<pid_t, pid_t> tid_pid;   // thread -> its process (thread-group leader)
  std::unordered_map<pid_t, ProcState> proc;  // per process
  std::unordered_set<pid_t> seen_tid;
  // Per thread: the DR7 value currently written and the address in each DR slot.
  struct DrState { uint64_t slot[4] = {0, 0, 0, 0}; uint64_t dr7 = 0; bool set = false; };
  std::unordered_map<pid_t, DrState> drs;
  // The thread currently held for each gate label (0 = none).
  std::unordered_map<std::string, pid_t> held_for_gate;
  // Per point: whether it already acted (first / n-th: it acts once).
  std::vector<bool> acted(np, false);
  // Per held thread: the release points of the hit at which it was held (they act when it is resumed).
  std::unordered_map<pid_t, std::vector<size_t>> deferred;
  size_t total_holds = 0;

  auto proc_of = [&](pid_t tid) -> pid_t {
    auto it = tid_pid.find(tid);
    if (it != tid_pid.end()) return it->second;
    // Find the thread-group id from /proc/<tid>/status (Tgid:).
    char path[64];
    std::snprintf(path, sizeof path, "/proc/%d/status", (int)tid);
    FILE* f = std::fopen(path, "re");
    pid_t tg = tid;
    if (f) {
      char* line = nullptr; size_t n = 0;
      while (getline(&line, &n, f) >= 0) {
        if (std::sscanf(line, "Tgid:\t%d", &tg) == 1) break;
      }
      std::free(line);
      std::fclose(f);
    }
    tid_pid[tid] = tg;
    return tg;
  };

  // (Re)resolve a process's control addresses from its maps.
  auto resolve_proc = [&](pid_t pid) {
    ProcState& ps = proc[pid];
    if (ps.all_resolved) return;
    if (!read_maps(pid, &ps.maps)) { ps.known = true; return; }
    ps.known = true;
    bool has_primary = false;
    for (const Mapping& m : ps.maps) if (m.base == config.primary_binary) has_primary = true;
    ps.relevant = has_primary;
    if (!ps.relevant) return;
    ps.addr.assign(np, 0);
    bool all = true;
    for (size_t i = 0; i < np; ++i) {
      const ControlPoint& cp = config.points[i];
      uint64_t a = 0;
      for (const Mapping& m : ps.maps) {
        if (m.base != cp.binary || !m.exec) continue;
        if (cp.file_offset >= m.pgoff && cp.file_offset < m.pgoff + (m.end - m.start)) { a = m.start + (cp.file_offset - m.pgoff); break; }
      }
      ps.addr[i] = a;
      if (a == 0) all = false;
    }
    ps.all_resolved = all;
  };

  // The config points (in plan order) that sit at run-time address `rip` in process `pid`.
  auto points_at = [&](pid_t pid, uint64_t rip) -> std::vector<size_t> {
    std::vector<size_t> out;
    const ProcState& ps = proc[pid];
    for (size_t i = 0; i < ps.addr.size(); ++i) if (ps.addr[i] != 0 && ps.addr[i] == rip) out.push_back(i);
    return out;
  };

  // Is `ra` inside a mapping of `binary` in process `pid`?
  auto caller_in = [&](pid_t pid, uint64_t ra, const std::string& binary) -> bool {
    for (const Mapping& m : proc[pid].maps) if (m.base == binary && ra >= m.start && ra < m.end) return true;
    return false;
  };

  // Arm the debug registers of thread `tid` for process `pid` (idempotent): one slot per distinct
  // resolved address.
  auto arm_thread = [&](pid_t tid, pid_t pid) {
    ProcState& ps = proc[pid];
    if (!ps.relevant) return;
    DrState desired;
    int n = 0;
    for (size_t i = 0; i < np && n < 4; ++i) {
      if (ps.addr.size() <= i || ps.addr[i] == 0) continue;
      bool dup = false;
      for (int s2 = 0; s2 < n; ++s2) dup = dup || desired.slot[s2] == ps.addr[i];
      if (!dup) desired.slot[n++] = ps.addr[i];
    }
    uint64_t dr7 = 0;
    for (int s = 0; s < n; ++s) dr7 |= (1ull << (s * 2));  // Ls enable, RW=00 (exec), LEN=00
    desired.dr7 = dr7;
    desired.set = true;
    DrState& cur = drs[tid];
    bool same = cur.set && cur.dr7 == desired.dr7;
    for (int s = 0; s < 4 && same; ++s) same = cur.slot[s] == desired.slot[s];
    if (same) return;
    for (int s = 0; s < n; ++s) {
      if (ptrace(PTRACE_POKEUSER, tid, (void*)(kDr0Off + s * sizeof(long)), (void*)desired.slot[s]) < 0) {
        if (report->debugregs_ok) { report->debugregs_ok = false; report->debugregs_error = std::string("PTRACE_POKEUSER DR") + std::to_string(s) + ": " + std::strerror(errno); }
        return;
      }
    }
    if (ptrace(PTRACE_POKEUSER, tid, (void*)kDr7Off, (void*)desired.dr7) < 0) {
      if (report->debugregs_ok) { report->debugregs_ok = false; report->debugregs_error = std::string("PTRACE_POKEUSER DR7: ") + std::strerror(errno); }
      return;
    }
    cur = desired;
    report->armings++;
  };

  auto cont = [&](pid_t tid, int sig, bool step_over) {
    if (step_over) {
      user_regs_struct r;
      if (ptrace(PTRACE_GETREGS, tid, 0, &r) == 0) { r.eflags |= (1u << 16); /* RF */ ptrace(PTRACE_SETREGS, tid, 0, &r); }
    }
    ptrace(PTRACE_CONT, tid, 0, (void*)(long)sig);
  };

  // Count a qualifying hit of point i; true if the point acts on it (first / n-th once, each always).
  auto count_hit = [&](size_t i) -> bool {
    const ControlPoint& cp = config.points[i];
    const uint64_t h = ++report->point_hits[i];
    if (cp.occurrence_each) return true;
    if (acted[i] || h != (uint64_t)(cp.occurrence_nth == 0 ? 1 : cp.occurrence_nth)) return false;
    acted[i] = true;
    return true;
  };

  // A release point acts (its hit by thread `by`): resume the thread its gate holds, if any.  The
  // resumed threads are queued; their deferred releases act before they continue.
  std::vector<pid_t> to_resume;
  auto fire = [&](size_t i, pid_t by, bool was_deferred) {
    const ControlPoint& cp = config.points[i];
    FireEvent fe;
    fe.release_label = cp.label;
    fe.gate_label = cp.releases;
    fe.tid = (int)by;
    fe.hit = report->point_hits[i];
    fe.t_ns = mono_ns();
    fe.deferred = was_deferred;
    auto it = held_for_gate.find(cp.releases);
    if (it != held_for_gate.end() && it->second != 0) {
      const pid_t held = it->second;
      it->second = 0;
      fe.resumed_tid = (int)held;
      for (HoldEvent& he : report->holds) if (he.tid == (int)held && he.gate_label == cp.releases && !he.released) { he.released = true; he.t_release_ns = fe.t_ns; he.release_label = cp.label; break; }
      if (config.verbose) std::fprintf(stderr, "[hold] RELEASE %s (hit %" PRIu64 ") resumes gate %s tid=%d\n", cp.label.c_str(), fe.hit, cp.releases.c_str(), (int)held);
      to_resume.push_back(held);
    } else {
      // the gate holds no thread now: an unless-released gate that has not acted is satisfied (the order it
      // enforces already holds); any other gate is left as it is (the release is consumed)
      for (size_t g = 0; g < np; ++g) {
        const ControlPoint& gp = config.points[g];
        if (gp.is_release || gp.label != cp.releases || !gp.unless_released || acted[g]) continue;
        acted[g] = true;
        fe.satisfied = true;
      }
      if (config.verbose)
        std::fprintf(stderr, "[hold] RELEASE %s (hit %" PRIu64 ") acts while gate %s holds no thread (%s)\n", cp.label.c_str(), fe.hit, cp.releases.c_str(),
                     fe.satisfied ? "the gate is satisfied" : "consumed");
    }
    report->fires.push_back(fe);
  };

  // Resume the queued threads: the releases of the hit at which each was held act first (it passes
  // that address now), then it continues past the gate instruction with its debug registers armed.
  auto drain_resumes = [&]() {
    while (!to_resume.empty()) {
      const pid_t h = to_resume.front();
      to_resume.erase(to_resume.begin());
      auto d = deferred.find(h);
      if (d != deferred.end()) {
        std::vector<size_t> rel = std::move(d->second);
        deferred.erase(d);
        for (size_t i : rel) if (count_hit(i)) fire(i, h, true);
      }
      const pid_t hp = proc_of(h);
      if (proc[hp].relevant) arm_thread(h, hp);  // (they stayed armed; idempotent)
      cont(h, 0, true);  // the resume flag steps over the gate instruction once
    }
  };

  if (config.verbose) std::fprintf(stderr, "[hold] seized pid=%d, options set; letting it exec\n", (int)child);
  // Resume the initial SIGSTOP (the child will exec next).
  ptrace(PTRACE_CONT, child, 0, 0);

  while (true) {
    int status = 0;
    pid_t w = waitpid(-1, &status, __WALL);
    if (w < 0) {
      if (errno == EINTR) continue;
      if (errno == ECHILD) break;
      return Status::error(Err::Io, std::string("waitpid: ") + std::strerror(errno));
    }
    if (WIFEXITED(status) || WIFSIGNALED(status)) {
      if (w == child) { report->child_exited = WIFEXITED(status); report->child_exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1; report->child_term_signal = WIFSIGNALED(status) ? WTERMSIG(status) : 0; }
      drs.erase(w);
      continue;
    }
    if (!WIFSTOPPED(status)) continue;
    report->total_stops++;
    const int sig = WSTOPSIG(status);
    const unsigned event = (unsigned)status >> 16;
    const pid_t pid = proc_of(w);
    if (seen_tid.insert(w).second) report->threads_attached++;

    if (sig == SIGTRAP && (event == PTRACE_EVENT_CLONE || event == PTRACE_EVENT_FORK || event == PTRACE_EVENT_VFORK)) {
      if (event == PTRACE_EVENT_CLONE) report->clone_attaches++;
      unsigned long newpid = 0;
      ptrace(PTRACE_GETEVENTMSG, w, 0, &newpid);
      cont(w, 0, false);
      continue;
    }
    if (sig == SIGTRAP && event == PTRACE_EVENT_EXEC) {
      // New image: its maps are fresh.
      proc[pid] = ProcState();
      resolve_proc(pid);
      if (proc[pid].relevant) arm_thread(w, pid);
      cont(w, 0, false);
      continue;
    }
    if (sig == SIGTRAP && event != 0) {  // other event stop (EXIT etc.)
      cont(w, 0, false);
      continue;
    }

    // Group stop / new-thread stop (PTRACE_EVENT_STOP) or a hardware breakpoint SIGTRAP.
    resolve_proc(pid);
    ProcState& ps = proc[pid];
    if (ps.relevant) arm_thread(w, pid);

    if (sig == SIGTRAP && event == 0) {
      // A trap: either our hardware breakpoint, or a spurious one.
      user_regs_struct r;
      if (ptrace(PTRACE_GETREGS, w, 0, &r) != 0) { cont(w, 0, false); continue; }
      const std::vector<size_t> pts = ps.relevant ? points_at(pid, (uint64_t)r.rip) : std::vector<size_t>();
      if (pts.empty()) { cont(w, 0, false); continue; }
      // Evaluate the conditions of every point at this address.
      std::vector<bool> ok(pts.size(), true);
      std::vector<std::string> names(pts.size());
      bool maps_fresh = false;
      bool any_gate = false, any_release = false;
      for (size_t k = 0; k < pts.size(); ++k) {
        const ControlPoint& cp = config.points[pts[k]];
        (cp.is_release ? any_release : any_gate) = true;
        if (cp.match_name && cp.name_reg >= 0) {
          uint64_t base = (uint64_t)reg_field(r, cp.name_reg);
          names[k] = peek_string(w, base + cp.name_off);
          if (names[k] != cp.name_text) ok[k] = false;
        }
        if (ok[k] && cp.match_caller) {
          uint64_t ra = peek_word(w, (uint64_t)r.rsp);  // entry: top of stack is the return address
          // The caller's module (e.g. cf.so) may be mapped after the control-point binaries, so the
          // resolved-addresses snapshot can predate it; read the maps fresh for this test.
          if (!maps_fresh) { read_maps(pid, &ps.maps); maps_fresh = true; }
          if (!caller_in(pid, ra, cp.caller_binary)) ok[k] = false;
        }
      }
      if (any_gate) report->gate_traps++;
      if (any_release) report->release_traps++;
      // Gates first: the first one that acts holds the thread.
      bool held = false;
      std::string held_by;
      for (size_t k = 0; k < pts.size(); ++k) {
        const ControlPoint& cp = config.points[pts[k]];
        if (cp.is_release || !ok[k]) continue;
        if (!count_hit(pts[k])) continue;
        if (held) {
          report->notes.push_back("gate " + cp.label + " acted on hit " + std::to_string(report->point_hits[pts[k]]) + " of tid " + std::to_string((int)w) +
                                  ", which gate " + held_by + " already holds at the same address: consumed without a hold");
          continue;
        }
        held = true;
        held_by = cp.label;
        // Hold this thread: leave it stopped (its debug registers stay armed).
        held_for_gate[cp.label] = w;
        HoldEvent he;
        he.gate_label = cp.label;
        he.tid = (int)w;
        he.name = names[k];
        he.hit = report->point_hits[pts[k]];
        he.t_hold_ns = mono_ns();
        report->holds.push_back(he);
        total_holds++;
        if (config.verbose) std::fprintf(stderr, "[hold] GATE %s (hit %" PRIu64 ") held tid=%d name=%s t=%" PRIu64 "\n", cp.label.c_str(), he.hit, (int)w, names[k].c_str(), he.t_hold_ns);
      }
      // Then the releases: those of a held thread's hit act when it is resumed.
      for (size_t k = 0; k < pts.size(); ++k) {
        const ControlPoint& cp = config.points[pts[k]];
        if (!cp.is_release || !ok[k]) continue;
        if (held) { deferred[w].push_back(pts[k]); continue; }
        if (count_hit(pts[k])) fire(pts[k], w, false);
      }
      drain_resumes();
      // A thread that is not held steps over its own breakpoint and runs on.
      if (!held) cont(w, 0, true);
      continue;
    }

    // A plain group stop or signal-delivery stop.
    if (sig == SIGSTOP || sig == SIGTRAP || (event == PTRACE_EVENT_STOP)) {
      cont(w, 0, false);
    } else {
      cont(w, sig, false);  // forward the signal
    }
  }

  report->t_end_ns = mono_ns();
  report->relevant_procs = 0;
  for (auto& kv : proc) if (kv.second.relevant) report->relevant_procs++;
  (void)total_holds;
  return Status::success();
}

}  // namespace kz::hold

// proc_capture.hpp -- what the observer reads about the traced process from /proc, from outside
// it, for the observation-effect record of PI decision D1 (user id, command, scheduling policy,
// environment).  Reading /proc/<pid>/... is not an in-process lookup: the traced process runs no
// code of the observer (SPEC 5.7).  Everything is best effort at the time of the call; a thread
// that is gone by then is reported as missing, never filled in.
//
// These reads are actions of the observer on the running target (PI decision D1: separate the
// observer's actions in the observation-effect record).  Every capture counts the /proc entries
// it read (`reads`); the caller times them and reports both as PERTURBATION kind proc-capture.
// Reading `environ` and `cmdline` copies the target's memory (access_remote_vm), which takes the
// target's address-space lock; mode `status` reads only /proc/<pid>/status of the process (and
// stat/status of each thread), mode `full` also cmdline, environ, the exe and cwd links, cgroup
// and limits.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "kz/core/status.hpp"

namespace kz::probe {

enum class ProcCaptureMode { Full, Status };
const char* proc_capture_mode_name(ProcCaptureMode mode);
bool parse_proc_capture_mode(const std::string& text, ProcCaptureMode* out);  // "full" | "status"

struct ThreadCapture {
  uint32_t pid = 0, tid = 0;
  bool ok = false;
  uint32_t reads = 0;         // /proc entries read (attempted)
  std::string comm;
  char state = '?';
  int policy = -1;            // 0 other, 1 fifo, 2 rr, 3 batch, 5 idle, 6 deadline
  int rt_priority = -1;
  int nice = 0;
  int kernel_priority = 0;    // the "priority" field of /proc/<pid>/task/<tid>/stat
  int last_cpu = -1;
  std::string cpus_allowed;   // Cpus_allowed_list
};

struct ProcessCapture {
  uint32_t pid = 0;
  bool ok = false;
  ProcCaptureMode mode = ProcCaptureMode::Full;
  uint32_t reads = 0;                     // /proc entries read (attempted)
  std::string uid, gid, groups;           // the Uid:/Gid:/Groups: lines of /proc/<pid>/status
  std::string cap_eff, cap_prm, cap_bnd, no_new_privs, seccomp, cpus_allowed;
  std::string cmdline;                    // arguments joined by single spaces
  std::string exe, cwd;                   // symbolic links
  std::vector<std::string> env_names;     // sorted; the values are not recorded (they may hold credentials)
  size_t env_bytes = 0;
  std::string env_sha256;                 // of the raw environment block
  std::string cgroup;
  std::vector<std::string> limits;        // the "Max ..." lines of /proc/<pid>/limits
};

Status capture_thread(uint32_t pid, uint32_t tid, ThreadCapture* out);
Status capture_process(uint32_t pid, ProcessCapture* out, ProcCaptureMode mode = ProcCaptureMode::Full);
// The tids that exist now in /proc/<pid>/task, sorted (one read of the directory).
std::vector<uint32_t> list_threads(uint32_t pid);
const char* policy_name(int policy);

// Parses the text of a stat file (exposed for tests).
Status parse_stat_line(const std::string& text, ThreadCapture* out);

}  // namespace kz::probe

// run_target.hpp -- start the target command and wait for it, without changing what it runs in.
//
// The observer starts the command by fork and exec of the argument vector as given (a search
// of PATH only if the name has no slash, as a shell does).  It changes none of the child's
// conditions: environment, user, scheduling class, priority, working directory and file
// descriptors 0-2 are inherited as they are; every other descriptor of the observer is
// close-on-exec.  The observer's own scheduling is not touched before the fork, so that the
// child inherits what a shell would give it.  SIGINT, SIGTERM and SIGHUP received by the
// observer are forwarded to the child, and the observation then ends normally.
#pragma once

#include <sys/resource.h>
#include <sys/types.h>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "kz/core/status.hpp"

namespace kz::probe {

struct TargetRun {
  pid_t pid = 0;
  bool exited = false;
  int exit_code = -1;
  int term_signal = 0;
  uint64_t t_start_ns = 0;   // CLOCK_MONOTONIC just before the fork
  uint64_t t_end_ns = 0;     // CLOCK_MONOTONIC when the wait returned
  int forwarded_signal = 0;  // the signal the observer forwarded, if any
  rusage usage = {};
};

// `after_start` runs in the observer right after the fork, with the child's pid.
Status run_target(const std::vector<std::string>& argv, TargetRun* out, const std::function<void(pid_t)>& after_start = nullptr);

uint64_t monotonic_ns();

}  // namespace kz::probe

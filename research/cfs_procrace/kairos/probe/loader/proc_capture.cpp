// proc_capture.cpp -- see proc_capture.hpp.
#include "proc_capture.hpp"

#include <dirent.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>

#include "kz/core/sha256.hpp"

namespace kz::probe {

namespace {

bool read_file(const std::string& path, std::string* out) {
  std::ifstream f(path, std::ios::binary);
  if (!f) return false;
  std::ostringstream ss;
  ss << f.rdbuf();
  *out = ss.str();
  return true;
}

std::string link_target(const std::string& path) {
  char buf[4096];
  const ssize_t n = readlink(path.c_str(), buf, sizeof buf - 1);
  if (n < 0) return std::string();
  return std::string(buf, static_cast<size_t>(n));
}

// The value of a "Key:\tvalue" line of a status file.
std::string status_value(const std::string& text, const std::string& key) {
  size_t pos = 0;
  while (pos < text.size()) {
    const size_t nl = text.find('\n', pos);
    const std::string line = text.substr(pos, nl == std::string::npos ? std::string::npos : nl - pos);
    pos = nl == std::string::npos ? text.size() : nl + 1;
    if (line.compare(0, key.size() + 1, key + ":") == 0) {
      size_t v = key.size() + 1;
      while (v < line.size() && (line[v] == ' ' || line[v] == '\t')) ++v;
      return line.substr(v);
    }
  }
  return std::string();
}

}  // namespace

const char* proc_capture_mode_name(ProcCaptureMode mode) { return mode == ProcCaptureMode::Status ? "status" : "full"; }

bool parse_proc_capture_mode(const std::string& text, ProcCaptureMode* out) {
  if (text == "full") *out = ProcCaptureMode::Full;
  else if (text == "status") *out = ProcCaptureMode::Status;
  else return false;
  return true;
}

const char* policy_name(int policy) {
  switch (policy) {
    case 0: return "SCHED_OTHER";
    case 1: return "SCHED_FIFO";
    case 2: return "SCHED_RR";
    case 3: return "SCHED_BATCH";
    case 5: return "SCHED_IDLE";
    case 6: return "SCHED_DEADLINE";
    default: return "unknown";
  }
}

Status parse_stat_line(const std::string& text, ThreadCapture* out) {
  // "pid (comm) S ppid ...": comm may hold spaces and parentheses, so cut at the LAST ')'.
  const size_t open = text.find('(');
  const size_t close = text.rfind(')');
  if (open == std::string::npos || close == std::string::npos || close < open) return Status::error(Err::Invalid, "stat: no (comm) field");
  out->comm = text.substr(open + 1, close - open - 1);
  std::istringstream rest(text.substr(close + 1));
  std::vector<std::string> f;
  std::string tok;
  while (rest >> tok) f.push_back(tok);
  // f[0] is field 3 (state); field N is f[N-3].
  if (f.size() < 39) return Status::error(Err::Invalid, "stat: fewer fields than a scheduling policy needs");
  out->state = f[0].empty() ? '?' : f[0][0];
  out->kernel_priority = std::atoi(f[15].c_str());   // field 18
  out->nice = std::atoi(f[16].c_str());              // field 19
  out->last_cpu = std::atoi(f[36].c_str());          // field 39
  out->rt_priority = std::atoi(f[37].c_str());       // field 40
  out->policy = std::atoi(f[38].c_str());            // field 41
  return Status::success();
}

Status capture_thread(uint32_t pid, uint32_t tid, ThreadCapture* out) {
  *out = ThreadCapture();
  out->pid = pid;
  out->tid = tid;
  const std::string base = "/proc/" + std::to_string(pid) + "/task/" + std::to_string(tid);
  std::string stat;
  ++out->reads;
  if (!read_file(base + "/stat", &stat)) return Status::error(Err::Io, "thread " + std::to_string(tid) + " is gone or unreadable");
  Status s = parse_stat_line(stat, out);
  if (!s.ok()) return s;
  std::string status;
  ++out->reads;
  if (read_file(base + "/status", &status)) out->cpus_allowed = status_value(status, "Cpus_allowed_list");
  out->ok = true;
  return Status::success();
}

Status capture_process(uint32_t pid, ProcessCapture* out, ProcCaptureMode mode) {
  *out = ProcessCapture();
  out->pid = pid;
  out->mode = mode;
  const std::string base = "/proc/" + std::to_string(pid);
  std::string status;
  ++out->reads;
  if (!read_file(base + "/status", &status)) return Status::error(Err::Io, "process " + std::to_string(pid) + " is gone or unreadable");
  out->uid = status_value(status, "Uid");
  out->gid = status_value(status, "Gid");
  out->groups = status_value(status, "Groups");
  out->cap_eff = status_value(status, "CapEff");
  out->cap_prm = status_value(status, "CapPrm");
  out->cap_bnd = status_value(status, "CapBnd");
  out->no_new_privs = status_value(status, "NoNewPrivs");
  out->seccomp = status_value(status, "Seccomp");
  out->cpus_allowed = status_value(status, "Cpus_allowed_list");
  if (mode == ProcCaptureMode::Status) {
    out->ok = true;
    return Status::success();
  }
  std::string cmd;
  ++out->reads;
  if (read_file(base + "/cmdline", &cmd)) {
    for (char& c : cmd)
      if (c == '\0') c = ' ';
    while (!cmd.empty() && cmd.back() == ' ') cmd.pop_back();
    out->cmdline = cmd;
  }
  out->exe = link_target(base + "/exe");
  out->cwd = link_target(base + "/cwd");
  out->reads += 2;
  std::string env;
  ++out->reads;
  if (read_file(base + "/environ", &env)) {
    out->env_bytes = env.size();
    out->env_sha256 = kz::to_hex(kz::sha256(env.data(), env.size()));
    size_t pos = 0;
    while (pos < env.size()) {
      const size_t nul = env.find('\0', pos);
      const std::string entry = env.substr(pos, nul == std::string::npos ? std::string::npos : nul - pos);
      pos = nul == std::string::npos ? env.size() : nul + 1;
      const size_t eq = entry.find('=');
      out->env_names.push_back(eq == std::string::npos ? entry : entry.substr(0, eq));
    }
    std::sort(out->env_names.begin(), out->env_names.end());
  }
  std::string cg;
  ++out->reads;
  if (read_file(base + "/cgroup", &cg)) {
    while (!cg.empty() && cg.back() == '\n') cg.pop_back();
    std::replace(cg.begin(), cg.end(), '\n', ';');
    out->cgroup = cg;
  }
  std::string limits;
  ++out->reads;
  if (read_file(base + "/limits", &limits)) {
    size_t pos = 0;
    while (pos < limits.size()) {
      const size_t nl = limits.find('\n', pos);
      std::string line = limits.substr(pos, nl == std::string::npos ? std::string::npos : nl - pos);
      pos = nl == std::string::npos ? limits.size() : nl + 1;
      if (line.compare(0, 4, "Max ") == 0) out->limits.push_back(line);
    }
  }
  out->ok = true;
  return Status::success();
}

std::vector<uint32_t> list_threads(uint32_t pid) {
  std::vector<uint32_t> tids;
  DIR* d = opendir(("/proc/" + std::to_string(pid) + "/task").c_str());
  if (d == nullptr) return tids;
  while (dirent* e = readdir(d)) {
    if (e->d_name[0] < '0' || e->d_name[0] > '9') continue;
    tids.push_back(static_cast<uint32_t>(std::strtoul(e->d_name, nullptr, 10)));
  }
  closedir(d);
  std::sort(tids.begin(), tids.end());
  return tids;
}

}  // namespace kz::probe

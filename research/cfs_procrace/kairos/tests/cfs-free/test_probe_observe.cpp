// Integration test of `kz anchor` + `kz observe` on the purpose-built target program: real
// perf_event_open uprobes, real per-CPU rings, no framework.
//
//   argv: 1 target_prog, 2 dwarf4 build, 3 other-compiler build, 4 target.kzp, 5 target_prog.c,
//         6 kairos source root, 7 the kz executable, 8 build directory (scratch files go below it)
//
// The scenarios (each states what the observer must establish and what it must refuse):
//   basic       every event, in program order per thread, with the arguments, the return-address
//               attribution through the mapping's load bias, the identity of the two tasks
//               (announced id, entry symbol, priority, kernel name, creator), call pairing, no loss
//   threads     concurrent threads: exact per-thread counts
//   burst       a one-page ring that must overflow: lost + seen == made, exactly
//   copy        the same bytes at another path are accepted, through --binary
//   refusals    a changed byte anywhere, a changed byte at a probe target, a truncated file, a
//               profile that is not the basis' profile: exit 2 and no container
//   exit codes  a command that fails, a command that does not exist
//   check       --check attaches nothing
// The test needs the right to attach uprobes (root).  Without it the test is skipped (exit 77),
// never passed.
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>

#include "basis.hpp"
#include "kz/core/container.hpp"
#include "kz/core/sha256.hpp"
#include "kz/trace/decode_match.hpp"
#include "kz/trace/identity_symbols.hpp"
#include "support/kz_test.hpp"

namespace {

std::string g_target, g_profile, g_source_root, g_kz, g_dir;

constexpr uint32_t kSend = 0, kSendRet = 1, kAfterGet = 2, kCreate = 3, kCreateRet = 4, kThreadEntry = 5;
constexpr uint64_t kStatusFail = 0xc2000005ull;

// ---- subprocesses ----------------------------------------------------------------------------

struct Run {
  int code = -1;       // exit status, or 128 + signal
  std::string output;  // stdout and stderr
};

std::string slurp(const std::string& path) {
  std::ifstream f(path, std::ios::binary);
  std::ostringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

bool exists(const std::string& path) { return access(path.c_str(), F_OK) == 0; }

Run run_command(const std::vector<std::string>& args) {
  Run r;
  const std::string log = g_dir + "/last_output.txt";
  const pid_t pid = fork();
  if (pid == 0) {
    const int fd = open(log.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) {
      dup2(fd, 1);
      dup2(fd, 2);
      close(fd);
    }
    std::vector<char*> argv;
    for (const std::string& a : args) argv.push_back(const_cast<char*>(a.c_str()));
    argv.push_back(nullptr);
    execv(argv[0], argv.data());
    _exit(127);
  }
  int status = 0;
  if (pid < 0 || waitpid(pid, &status, 0) != pid) return r;
  r.code = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
  r.output = slurp(log);
  return r;
}

bool contains(const std::string& s, const std::string& what) { return s.find(what) != std::string::npos; }

// The number printed at the end of the first line that starts (after blanks) with `name`; -1 if none.
long count_line(const std::string& text, const std::string& name) {
  std::istringstream in(text);
  std::string line;
  while (std::getline(in, line)) {
    const size_t b = line.find_first_not_of(' ');
    if (b == std::string::npos || line.compare(b, name.size(), name) != 0) continue;
    const size_t e = line.find_last_of(' ');
    if (e == std::string::npos || e + 1 >= line.size()) continue;
    return std::strtol(line.c_str() + e + 1, nullptr, 10);
  }
  return -1;
}

// ---- what an observation leaves --------------------------------------------------------------

struct Observation {
  Run run;
  std::string container_path, record_path;
  kz::Container container;
  std::map<std::string, std::string> record;  // "name = value" lines of the human record
  std::vector<kz_raw_ev> events;
  std::vector<kz_thread> threads;
  std::vector<kz_loss> loss;
  std::vector<kz_map_ev> maps;
  std::vector<kz_site> sites;
  std::vector<kz_runmeta> runmeta;
  bool loaded = false;

  std::string text(const kz_str& s) const {
    std::string out;
    container.string(s, &out);
    return out;
  }
  std::string rec(const std::string& name) const {
    auto it = record.find(name);
    return it == record.end() ? std::string("<absent>") : it->second;
  }
  uint64_t num(const std::string& name) const { return std::strtoull(rec(name).c_str(), nullptr, 10); }
  const kz_thread* thread_named(const std::string& name) const {
    for (const kz_thread& t : threads)
      if (text(t.name) == name) return &t;
    return nullptr;
  }
  std::vector<kz_raw_ev> of_tid(uint32_t tid) const {
    std::vector<kz_raw_ev> v;
    for (const kz_raw_ev& e : events)
      if (e.tid == tid) v.push_back(e);
    return v;
  }
};

void parse_record(const std::string& text, std::map<std::string, std::string>* out) {
  std::istringstream in(text);
  std::string line;
  while (std::getline(in, line)) {
    const size_t eq = line.find(" = ");
    if (eq == std::string::npos) continue;
    std::string value = line.substr(eq + 3);
    const size_t tag = value.find("   [");
    if (tag != std::string::npos) value.resize(tag);
    (*out)[line.substr(0, eq)] = value;
  }
}

std::string make_basis(const std::string& name, const std::string& binary, const std::string& profile) {
  const std::string path = g_dir + "/" + name;
  const Run r = run_command({g_kz, "anchor", binary, profile, "-o", path, "--source-root", g_source_root});
  KZ_CHECK_EQ(r.code, 0);
  if (r.code != 0) std::fprintf(stderr, "kz anchor: %s\n", r.output.c_str());
  return path;
}

// Runs `kz observe` and loads what it wrote.  `extra` are options before the `--`.
Observation observe(const std::string& tag, const std::string& basis, const std::string& profile, const std::vector<std::string>& extra,
                    const std::vector<std::string>& command) {
  Observation o;
  o.container_path = g_dir + "/" + tag + ".kzt";
  o.record_path = o.container_path + ".record.txt";
  unlink(o.container_path.c_str());
  unlink(o.record_path.c_str());
  std::vector<std::string> args = {g_kz, "observe", basis, profile, "-o", o.container_path};
  args.insert(args.end(), extra.begin(), extra.end());
  args.push_back("--");
  args.insert(args.end(), command.begin(), command.end());
  o.run = run_command(args);
  if (!exists(o.container_path)) return o;
  KZ_CHECK_OK(kz::Container::read_file(o.container_path, &o.container));
  KZ_CHECK_OK(o.container.verify_content());
  KZ_CHECK_EQ(o.container.header().artifact_kind, uint32_t(KZ_ARTIFACT_RUN));
  KZ_CHECK_OK(o.container.read_section(KZ_SEC_RAW_EVENTS, &o.events));
  KZ_CHECK_OK(o.container.read_section(KZ_SEC_THREADS, &o.threads));
  KZ_CHECK_OK(o.container.read_section(KZ_SEC_LOSS, &o.loss));
  KZ_CHECK_OK(o.container.read_section(KZ_SEC_MAPS, &o.maps));
  KZ_CHECK_OK(o.container.read_section(KZ_SEC_SITES, &o.sites));
  KZ_CHECK_OK(o.container.read_section(KZ_SEC_RUNMETA, &o.runmeta));
  parse_record(slurp(o.record_path), &o.record);
  o.loaded = true;
  return o;
}

bool no_permission(const Run& r) {
  return r.code == 3 && (contains(r.output, "Operation not permitted") || contains(r.output, "Permission denied") ||
                         contains(r.output, "EPERM") || contains(r.output, "EACCES"));
}

// The load bias of the process, from the mapping record.
bool load_bias(const Observation& o, uint64_t* bias) {
  for (const kz_map_ev& m : o.maps)
    if (m.bias_known != 0) {
      *bias = m.bias;
      return true;
    }
  return false;
}

std::string caller_of(const kz::trace::SymbolIndex& syms, uint64_t bias, uint64_t return_address) {
  const kz::trace::SymbolEntry* s = syms.find(return_address - bias);
  return s == nullptr ? std::string("<none>") : s->name;
}

// ---- scenarios -------------------------------------------------------------------------------

struct Step {
  uint32_t site;
  uint64_t a0, a1, a2;
  bool operator==(const Step& o) const { return site == o.site && a0 == o.a0 && a1 == o.a1 && a2 == o.a2; }
};

std::string show_steps(const std::vector<kz_raw_ev>& v) {
  std::ostringstream s;
  for (const kz_raw_ev& e : v) s << "(" << e.site << ":" << std::hex << e.a[0] << "," << e.a[1] << "," << e.a[2] << std::dec << ") ";
  return s.str();
}

bool same_steps(const std::vector<kz_raw_ev>& got, const std::vector<Step>& want) {
  if (got.size() != want.size()) return false;
  for (size_t i = 0; i < got.size(); ++i)
    if (!(Step{got[i].site, got[i].a[0], got[i].a[1], got[i].a[2]} == want[i])) return false;
  return true;
}

void test_basic(const std::string& basis_path, kz::plan::LoadedBasis& basis, const kz::trace::SymbolIndex& syms) {
  Observation o = observe("basic", basis_path, g_profile, {"--stratum", "cold"}, {g_target, "basic"});
  KZ_CHECK_EQ(o.run.code, 0);
  if (!o.loaded) return;

  // The container is a child of the basis and its content is sound.
  KZ_CHECK_OK(o.container.verify_parent(basis.container));
  KZ_CHECK(o.container.profile_hash() == basis.container.profile_hash());

  KZ_CHECK_EQ(o.events.size(), size_t(21));
  KZ_CHECK_EQ(o.rec("decode.raw_events"), std::string("21"));
  KZ_CHECK_EQ(o.rec("loss.perf_record_lost.dropped_records"), std::string("0"));
  KZ_CHECK_EQ(o.rec("loss.perf_throttle.intervals"), std::string("0"));
  KZ_CHECK_EQ(o.rec("loss.stream_errors"), std::string("0"));
  KZ_CHECK(o.loss.empty());
  KZ_CHECK_EQ(o.rec("decode.slot_failures"), std::string("0"));
  KZ_CHECK_EQ(o.rec("match.call_pairs"), std::string("9"));
  KZ_CHECK_EQ(o.rec("match.entries_without_return"), std::string("0"));
  KZ_CHECK_EQ(o.rec("match.returns_without_entry"), std::string("0"));
  KZ_CHECK_EQ(o.rec("hits.site0.send.entry"), std::string("7"));
  KZ_CHECK_EQ(o.rec("hits.site2.publish.after_get"), std::string("1"));
  KZ_CHECK_EQ(o.rec("hits.site5.thread.entry"), std::string("2"));
  // The conditions of the observation (D1).
  KZ_CHECK(contains(o.rec("observer.backend"), "perf-sample"));
  KZ_CHECK(contains(o.rec("observer.backend"), "the binary is not modified"));
  KZ_CHECK(contains(o.rec("tool.clock"), "CLOCK_MONOTONIC"));
  KZ_CHECK(contains(o.rec("binary.verified_against_basis"), "SHA-256"));
  KZ_CHECK_EQ(o.rec("observer.uid"), std::string("0 (euid 0)"));
  KZ_CHECK(contains(o.rec("target.command_line"), "basic"));
  KZ_CHECK_EQ(o.rec("target.exit"), std::string("exited 0"));
  KZ_CHECK_EQ(o.runmeta.size(), size_t(1));
  if (o.runmeta.size() == 1) {
    KZ_CHECK_EQ(o.runmeta[0].stratum, uint32_t(KZ_STRATUM_COLD));  // declared by the caller, not detected
    KZ_CHECK_EQ(o.runmeta[0].run_ordinal, 0u);
    KZ_CHECK(std::memcmp(o.runmeta[0].basis_sha256, basis.container.artifact_hash().data(), 32) == 0);
  }
  KZ_CHECK(o.rec("observer.stop_ns") != "<absent>");

  // Sites: labels, probed symbols, the identity of the file.
  KZ_CHECK_EQ(o.sites.size(), size_t(6));
  if (o.sites.size() == 6) {
    KZ_CHECK_EQ(o.text(o.sites[kSend].label), std::string("send.entry"));
    KZ_CHECK_EQ(o.text(o.sites[kSend].symbol), std::string("tp_send"));
    KZ_CHECK_EQ(o.text(o.sites[kAfterGet].symbol), std::string("tp_publish"));
    KZ_CHECK_EQ(o.sites[kAfterGet].line, 64u);
    KZ_CHECK(o.sites[kSend].inode != 0);
  }

  // The mapping of the traced file and its load bias.
  KZ_CHECK_EQ(o.maps.size(), size_t(1));
  uint64_t bias = 0;
  KZ_CHECK(load_bias(o, &bias));
  KZ_CHECK(bias != 0);  // a position-independent executable is mapped away from 0

  // Threads: the process' main thread and the two tasks.
  KZ_CHECK_EQ(o.threads.size(), size_t(3));
  const kz_thread* w1 = o.thread_named("WORKER_1");
  const kz_thread* w2 = o.thread_named("WORKER_2");
  const kz_thread* main_thread = o.thread_named("target_prog");
  KZ_CHECK(w1 != nullptr && w2 != nullptr && main_thread != nullptr);
  if (w1 == nullptr || w2 == nullptr || main_thread == nullptr) return;
  KZ_CHECK_EQ(main_thread->tid, main_thread->pid);
  KZ_CHECK_EQ(w1->pid, main_thread->pid);
  KZ_CHECK_EQ(w1->ptid, main_thread->tid);
  KZ_CHECK_EQ(w2->ptid, main_thread->tid);
  KZ_CHECK_EQ(w1->announced_id, 0x100u);
  KZ_CHECK_EQ(w2->announced_id, 0x101u);
  KZ_CHECK_EQ(o.text(w1->entry_symbol), std::string("worker1"));
  KZ_CHECK_EQ(o.text(w2->entry_symbol), std::string("worker2"));
  KZ_CHECK_EQ(w1->priority, 11u);
  KZ_CHECK_EQ(w2->priority, 22u);
  const uint32_t want = KZ_THREAD_FLAG_FORK_SEEN | KZ_THREAD_FLAG_COMM_SEEN | KZ_THREAD_FLAG_ENTRY_SEEN | KZ_THREAD_FLAG_SPAWN_MATCHED;
  KZ_CHECK_EQ(w1->flags & want, want);
  KZ_CHECK_EQ(w2->flags & want, want);
  KZ_CHECK_EQ(w1->flags & KZ_THREAD_FLAG_SPAWN_AMBIGUOUS, 0u);
  KZ_CHECK(w1->creator_event < o.events.size() && w2->creator_event < o.events.size());
  if (w1->creator_event < o.events.size()) {
    KZ_CHECK_EQ(o.events[w1->creator_event].tid, main_thread->tid);
    KZ_CHECK_EQ(o.events[w1->creator_event].site, kCreate);
  }
  KZ_CHECK_EQ(w1->n_events, 7u);
  KZ_CHECK_EQ(w2->n_events, 5u);
  KZ_CHECK_EQ(main_thread->n_events, 9u);
  KZ_CHECK(o.rec("identity.spawn_matched") == "2" && o.rec("identity.spawn_ambiguous") == "0");
  // The creator link rests on FORK-time containment only, and nothing on the creator side was
  // compared with the announced ids: the records say so.
  const uint32_t honest = KZ_THREAD_FLAG_MATCH_BY_TIME | KZ_THREAD_FLAG_ID_UNVERIFIED;
  KZ_CHECK_EQ(w1->flags & honest, honest);
  KZ_CHECK_EQ(w2->flags & honest, honest);
  KZ_CHECK_EQ(main_thread->flags & honest, 0u);
  KZ_CHECK(o.rec("identity.matched_by_time").compare(0, 2, "2 ") == 0);
  KZ_CHECK(o.rec("identity.ids_unverified").compare(0, 2, "2 ") == 0);
  KZ_CHECK_EQ(o.rec("match.events_with_unreadable_key"), std::string("0"));

  // The observer's own /proc reads while the command ran are counted, and are a PERTURBATION row.
  KZ_CHECK(o.rec("observer.proc_capture.mode").compare(0, 5, "full:") == 0);
  const long proc_reads = std::strtol(o.rec("observer.proc_capture.reads").c_str(), nullptr, 10);
  // At least: the process (1 attempt; 7 reads if it was still there), its thread list (1), and one
  // attempt for each of the three threads (2 reads each if it was still there).  This short program
  // may be gone before the drain thread looks: then the attempts alone are counted.
  KZ_CHECK(proc_reads >= 1 + 1 + 3);
  KZ_CHECK(contains(o.rec("observer.proc_capture.window"), "after the command started"));
  std::vector<kz_perturbation> pert;
  KZ_CHECK_OK(o.container.read_section(KZ_SEC_PERTURBATION, &pert));
  bool proc_row = false;
  for (const kz_perturbation& p : pert) {
    if (p.kind == KZ_PERTURBATION_KIND_PROC_CAPTURE) {
      proc_row = true;
      KZ_CHECK_EQ(p.count, static_cast<uint64_t>(proc_reads));
      KZ_CHECK(p.ns > 0);
    }
    // the return site shares the entry's breakpoint: its own cost is the return trampoline
    if (p.kind == KZ_PERTURBATION_KIND_PROBE_HITS && p.site == 1) KZ_CHECK_EQ(p.cost_class, uint32_t(KZ_INSN_CLASS_RET_TRAMPOLINE));
    if (p.kind == KZ_PERTURBATION_KIND_PROBE_HITS && p.site == 0) KZ_CHECK_EQ(p.cost_class, uint32_t(KZ_INSN_CLASS_EMULATED));
  }
  KZ_CHECK(proc_row);
  KZ_CHECK(o.rec("perturbation.site0.trap").compare(0, 12, "counted-here") == 0);
  KZ_CHECK(contains(o.rec("perturbation.site1.trap"), "counted in the row of site 0 (send.entry)"));

  // Events of each thread, in program order: site and the first three arguments.
  const std::vector<kz_raw_ev> e1 = o.of_tid(w1->tid), e2 = o.of_tid(w2->tid), em = o.of_tid(main_thread->tid);
  KZ_CHECK_EQ(e1.size(), size_t(7));
  KZ_CHECK_EQ(e2.size(), size_t(5));
  KZ_CHECK_EQ(em.size(), size_t(9));
  const std::vector<Step> want1 = {{kThreadEntry, 0x100, 0, 0}, {kSend, 5, 2, 0},  {kSendRet, kStatusFail, 0, 0}, {kSend, 10, 3, 0},
                                   {kSendRet, kStatusFail, 0, 0}, {kSend, 5, 2, 7}, {kSendRet, 0, 0, 0}};
  const std::vector<Step> want2 = {{kThreadEntry, 0x101, 0, 0}, {kSend, 10, 3, 0}, {kSendRet, kStatusFail, 0, 0}, {kSend, 5, 2, 0}, {kSendRet, kStatusFail, 0, 0}};
  KZ_CHECK(same_steps(e1, want1));
  KZ_CHECK(same_steps(e2, want2));
  if (!same_steps(e1, want1)) std::fprintf(stderr, "WORKER_1 events: %s\n", show_steps(e1).c_str());
  if (!same_steps(e2, want2)) std::fprintf(stderr, "WORKER_2 events: %s\n", show_steps(e2).c_str());
  // The main thread: two spawn pairs, the after-call point, two sends.  (entry-point arguments are
  // pointers: only the status, priority and the pair structure are asserted.)
  if (em.size() == 9) {
    const uint32_t sites_main[9] = {kCreate, kCreateRet, kCreate, kCreateRet, kAfterGet, kSend, kSendRet, kSend, kSendRet};
    for (size_t i = 0; i < 9; ++i) KZ_CHECK_EQ(em[i].site, sites_main[i]);
    KZ_CHECK_EQ(em[0].a[2], uint64_t(11));
    KZ_CHECK_EQ(em[2].a[2], uint64_t(22));
    KZ_CHECK_EQ(em[1].a[0], uint64_t(0));
    KZ_CHECK_EQ(em[4].a[0], uint64_t(0));        // the status of the call before the probed line
    KZ_CHECK_EQ(em[5].a[2], uint64_t(0x110003)); // the identity that the program stored and then used
    KZ_CHECK_EQ(em[7].a[2], uint64_t(3));
    // Entry-point values of the creator site are runtime addresses of worker1 and worker2.
    KZ_CHECK_EQ(caller_of(syms, bias, em[0].a[1]), std::string("worker1"));
    KZ_CHECK_EQ(caller_of(syms, bias, em[2].a[1]), std::string("worker2"));
    // Return-address attribution: the word at the top of the stack names the caller of tp_send.
    KZ_CHECK_EQ(caller_of(syms, bias, em[5].a[3]), std::string("tp_wrap_a"));  // tp_publish -> tp_wrap_a -> tp_send
    KZ_CHECK_EQ(caller_of(syms, bias, em[7].a[3]), std::string("tp_wrap_b"));
    KZ_CHECK_EQ(caller_of(syms, bias, em[0].a[3]), std::string("main"));  // the creator of the task
  }
  const std::string callers1[3] = {"tp_wrap_a", "tp_wrap_b", "tp_wrap_a"};
  const size_t entries1[3] = {1, 3, 5};
  if (e1.size() == 7)
    for (int i = 0; i < 3; ++i) {
      KZ_CHECK_EQ(caller_of(syms, bias, e1[entries1[i]].a[3]), callers1[i]);
      // The return probe's ip is the return address that the entry probe saw.
      KZ_CHECK_EQ(e1[entries1[i] + 1].a[3], e1[entries1[i]].a[3]);
    }
  KZ_CHECK_EQ(caller_of(syms, bias, e2.size() == 5 ? e2[1].a[3] : 0), std::string("tp_wrap_b"));

  // `kz hits`: counts only, from the container.
  {
    const Run h = run_command({g_kz, "hits", o.container_path});
    KZ_CHECK_EQ(h.code, 0);
    KZ_CHECK(contains(h.output, "events 21"));
    KZ_CHECK(contains(h.output, "send.entry"));
    KZ_CHECK(contains(h.output, "WORKER_1 (tid " + std::to_string(w1->tid) + ")"));
    const Run b = run_command({g_kz, "hits", o.container_path, "--site", "send.entry", "--before", "publish.after_get", "--group-slot", "app_id",
                               "--group-return-slot", "status"});
    KZ_CHECK_EQ(b.code, 0);
    KZ_CHECK(contains(b.output, "events of site send.entry before the first event of site publish.after_get: 5"));
    KZ_CHECK(contains(b.output, "WORKER_1"));
    KZ_CHECK(contains(b.output, "0xc2000005"));
    KZ_CHECK(contains(b.output, "(no return event paired)"));
    // the main thread's two sends come after the point, so they are not counted
    KZ_CHECK(!contains(b.output, "target_prog   "));
    // the callers, from the return addresses of the entry events and the symbols of the basis
    const Run callers = run_command({g_kz, "hits", o.container_path, "--basis", basis_path, "--site", "send.entry", "--group-caller"});
    KZ_CHECK_EQ(callers.code, 0);
    KZ_CHECK_EQ(count_line(callers.output, "tp_wrap_a"), 4L);
    KZ_CHECK_EQ(count_line(callers.output, "tp_wrap_b"), 3L);
    KZ_CHECK_EQ(run_command({g_kz, "hits", o.container_path, "--site", "send.entry", "--group-caller"}).code, 1);                      // needs --basis
    KZ_CHECK_EQ(run_command({g_kz, "hits", o.container_path, "--basis", o.container_path, "--site", "send.entry", "--group-caller"}).code, 2);  // not a basis
    KZ_CHECK_EQ(run_command({g_kz, "hits", o.container_path, "--basis", basis_path, "--site", "thread.entry", "--group-caller"}).code, 0);
    KZ_CHECK_EQ(run_command({g_kz, "hits", o.container_path, "--site", "no.such.site"}).code, 2);
    KZ_CHECK_EQ(run_command({g_kz, "hits", o.container_path, "--before", "send.entry"}).code, 1);
    KZ_CHECK_EQ(run_command({g_kz, "hits", basis_path}).code, 2);  // a basis is not a run
  }

  // Timestamps are ordered per thread (evidence only: nothing here is a happens-before edge).
  for (const std::vector<kz_raw_ev>* v : {&e1, &e2, &em})
    for (size_t i = 1; i < v->size(); ++i) {
      KZ_CHECK((*v)[i].t_ns >= (*v)[i - 1].t_ns);
      KZ_CHECK_EQ((*v)[i].tseq, (*v)[i - 1].tseq + 1);
    }
}

void test_threads(const std::string& basis_path) {
  const unsigned kThreads = 3, kCalls = 200;
  Observation o = observe("threads", basis_path, g_profile, {}, {g_target, "threads", std::to_string(kThreads), std::to_string(kCalls)});
  KZ_CHECK_EQ(o.run.code, 0);
  if (!o.loaded) return;
  KZ_CHECK(o.loss.empty());
  KZ_CHECK_EQ(o.rec("loss.perf_record_lost.dropped_records"), std::string("0"));
  KZ_CHECK_EQ(o.threads.size(), size_t(kThreads + 1));
  KZ_CHECK_EQ(o.events.size(), size_t(kThreads * (2 * kCalls + 1) + 2 * kThreads));
  KZ_CHECK_EQ(o.rec("match.call_pairs"), std::to_string(kThreads * kCalls + kThreads));  // the sends, and the spawn calls of the main thread
  KZ_CHECK_EQ(o.rec("match.entries_without_return"), std::string("0"));
  KZ_CHECK_EQ(o.rec("identity.spawn_matched"), std::to_string(kThreads));
  KZ_CHECK_EQ(o.rec("identity.spawn_ambiguous"), std::string("0"));
  for (unsigned i = 0; i < kThreads; ++i) {
    char name[8];
    std::snprintf(name, sizeof name, "T%02u", i);
    const kz_thread* t = o.thread_named(name);
    KZ_CHECK(t != nullptr);
    if (t == nullptr) continue;
    KZ_CHECK_EQ(t->announced_id, 0x100u + i);
    KZ_CHECK_EQ(t->n_events, 2 * kCalls + 1);
    KZ_CHECK_EQ(o.text(t->entry_symbol), std::string("looper"));
    const std::vector<kz_raw_ev> ev = o.of_tid(t->tid);
    KZ_CHECK_EQ(ev.size(), size_t(2 * kCalls + 1));
    if (ev.size() != 2 * kCalls + 1) continue;
    KZ_CHECK_EQ(ev[0].site, kThreadEntry);
    for (unsigned c = 0; c < kCalls; ++c) {
      const kz_raw_ev& in = ev[1 + 2 * c];
      const kz_raw_ev& out = ev[2 + 2 * c];
      KZ_CHECK_EQ(in.site, kSend);
      KZ_CHECK_EQ(out.site, kSendRet);
      KZ_CHECK_EQ(in.a[2], uint64_t(c & 1));                          // the argument alternates 0, 1, 0, 1 ...
      KZ_CHECK_EQ(out.a[0], (c & 1) == 0 ? kStatusFail : uint64_t(0));  // and so does the status
      KZ_CHECK_EQ(out.a[3], in.a[3]);
    }
  }
}

void test_burst(const std::string& basis_path) {
  const unsigned kCalls = 20000;
  Observation o = observe("burst", basis_path, g_profile, {"--ring-pages", "1"}, {g_target, "burst", std::to_string(kCalls)});
  KZ_CHECK_EQ(o.run.code, 0);
  if (!o.loaded) return;
  uint64_t dropped = 0;
  for (const kz_loss& l : o.loss)
    if (l.source == KZ_LOSS_SOURCE_PERF_RECORD_LOST) dropped += l.count;
  KZ_CHECK(dropped > 0);  // the ring is one page: it must have overflowed
  KZ_CHECK(!o.events.empty());
  // The kernel counts every record it drops from a ring, and the side-band records of all processes
  // share the ring with the probe samples: the count is an upper bound of the lost probe hits, not
  // an exact number.  What must hold: no probe hit disappears without being counted, and the
  // overcount is small (a few side-band records, not thousands of probe hits).
  const size_t made = 2 * kCalls, slack = 256;
  KZ_CHECK(o.events.size() + dropped >= made);
  KZ_CHECK(o.events.size() + dropped <= made + slack);
  if (o.events.size() + dropped < made || o.events.size() + dropped > made + slack)
    std::fprintf(stderr, "burst: %zu events + %llu dropped for %zu hits\n", o.events.size(), static_cast<unsigned long long>(dropped), made);
  KZ_CHECK_EQ(o.num("loss.perf_record_lost.dropped_records"), dropped);
  KZ_CHECK_EQ(o.num("decode.raw_events"), o.events.size());
  // The matching accounts for every surviving event.  Whether a drop leaves an entry without its
  // return (or the reverse) depends on where the ring happened to fill, so only the bookkeeping is
  // asserted, not that a failure occurred.
  KZ_CHECK_EQ(o.num("match.call_pairs") + o.num("match.entries_without_return"), o.num("hits.site0.send.entry"));
  KZ_CHECK_EQ(o.num("match.call_pairs") + o.num("match.returns_without_entry"), o.num("hits.site1.send.ret"));
  KZ_CHECK_EQ(o.num("hits.site0.send.entry") + o.num("hits.site1.send.ret"), o.events.size());
  for (const kz_loss& l : o.loss) {
    KZ_CHECK(l.t_ns != 0);
    KZ_CHECK_EQ(l.source, uint8_t(KZ_LOSS_SOURCE_PERF_RECORD_LOST));
  }
}

void test_copy_and_refusals(const std::string& basis_path, kz::plan::LoadedBasis& basis) {
  const std::string bytes = slurp(g_target);
  KZ_CHECK(!bytes.empty());

  // The same bytes at another path: accepted.  The probes follow the file that was verified.
  const std::string copy = g_dir + "/target_copy";
  {
    std::ofstream f(copy, std::ios::binary | std::ios::trunc);
    f.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  }
  chmod(copy.c_str(), 0755);
  Observation c = observe("copy", basis_path, g_profile, {"--binary", copy}, {copy, "basic"});
  KZ_CHECK_EQ(c.run.code, 0);
  KZ_CHECK_EQ(c.events.size(), size_t(21));
  KZ_CHECK_EQ(c.rec("binary.path"), copy);
  KZ_CHECK_EQ(c.rec("binary.sha256"), kz::to_hex(basis.binary_sha256));
  struct stat copy_stat, orig_stat;
  KZ_CHECK(stat(copy.c_str(), &copy_stat) == 0 && stat(g_target.c_str(), &orig_stat) == 0);
  KZ_CHECK(copy_stat.st_ino != orig_stat.st_ino);
  if (c.loaded && !c.sites.empty()) KZ_CHECK_EQ(c.sites[0].inode, uint64_t(copy_stat.st_ino));  // the file that was probed, not the basis' file

  // A command that runs a DIFFERENT file than the one that was probed produces no events, and the
  // record says which file was probed.
  Observation other = observe("other_file", basis_path, g_profile, {"--binary", copy}, {g_target, "basic"});
  KZ_CHECK_EQ(other.run.code, 0);
  KZ_CHECK_EQ(other.events.size(), size_t(0));
  KZ_CHECK_EQ(other.rec("binary.path"), copy);

  auto write_variant = [&](const std::string& name, const std::string& content) {
    const std::string path = g_dir + "/" + name;
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    f.write(content.data(), static_cast<std::streamsize>(content.size()));
    f.close();
    chmod(path.c_str(), 0755);
    return path;
  };
  auto refused = [&](const std::string& tag, const std::string& basis_arg, const std::string& profile_arg, const std::string& binary,
                     const std::string& needle) {
    std::vector<std::string> extra;
    if (!binary.empty()) extra = {"--binary", binary};
    Observation o = observe(tag, basis_arg, profile_arg, extra, {binary.empty() ? g_target : binary, "basic"});
    KZ_CHECK_EQ(o.run.code, 2);
    KZ_CHECK(!exists(o.container_path));  // a refused observation writes nothing
    KZ_CHECK(!needle.empty() && contains(o.run.output, needle));
    if (o.run.code != 2 || !contains(o.run.output, needle)) std::fprintf(stderr, "[%s] output: %s\n", tag.c_str(), o.run.output.c_str());
  };

  // One byte changed far from any probe target (the last byte of the file).
  std::string tampered = bytes;
  tampered.back() = static_cast<char>(tampered.back() ^ 0x01);
  refused("refuse_last_byte", basis_path, g_profile, write_variant("tampered_last", tampered), "SHA-256");

  // One byte changed at a probe target: the check of the bytes at the target is not what stops it
  // (the hash already differs), but the binary must not be probed either way.
  tampered = bytes;
  const uint64_t off = basis.targets.at(0).file_offset;
  tampered[off] = static_cast<char>(tampered[off] ^ 0x01);
  refused("refuse_target_byte", basis_path, g_profile, write_variant("tampered_target", tampered), "SHA-256");

  // A truncated file.
  refused("refuse_truncated", basis_path, g_profile, write_variant("truncated", bytes.substr(0, bytes.size() / 2)), "size");

  // A profile that is not the profile the basis was built for (a comment line is enough).
  const std::string altered_profile = g_dir + "/altered.kzp";
  {
    std::ofstream f(altered_profile, std::ios::binary | std::ios::trunc);
    f << slurp(g_profile) << "# changed after the basis was built\n";
  }
  refused("refuse_profile", basis_path, altered_profile, std::string(), "profile");

  // A basis that is not a basis.
  refused("refuse_basis", g_target, g_profile, std::string(), "basis");
}

void test_exit_codes_and_check(const std::string& basis_path) {
  // A command that exits with another status: the observation is complete, and the exit status says so.
  Observation failing = observe("exit7", basis_path, g_profile, {}, {g_target, "exit", "7"});
  KZ_CHECK_EQ(failing.run.code, 10);
  KZ_CHECK_EQ(failing.rec("target.exit"), std::string("exited 7"));
  KZ_CHECK(failing.loaded);

  // A command that does not exist.
  Observation missing = observe("missing", basis_path, g_profile, {}, {"/nonexistent/kz-test-command"});
  KZ_CHECK_EQ(missing.run.code, 10);
  KZ_CHECK_EQ(missing.rec("target.exit"), std::string("exited 127"));
  KZ_CHECK(contains(missing.run.output, "cannot execute the command"));

  // --check attaches nothing and runs nothing.
  const Run check = run_command({g_kz, "observe", basis_path, g_profile, "--check"});
  KZ_CHECK_EQ(check.code, 0);
  KZ_CHECK(contains(check.output, "hash, size, build-id and the bytes at 6 targets match"));
  KZ_CHECK(contains(check.output, "site 0  send.entry"));
  KZ_CHECK(contains(check.output, "slot 3: return_address"));

  // Usage errors.
  KZ_CHECK_EQ(run_command({g_kz, "observe"}).code, 1);
  KZ_CHECK_EQ(run_command({g_kz, "observe", basis_path, g_profile, "-o", g_dir + "/u.kzt"}).code, 1);  // no command after --
  KZ_CHECK_EQ(run_command({g_kz, "observe", basis_path, g_profile, "-o", g_dir + "/u.kzt", "--stratum", "tepid", "--", "/bin/true"}).code, 1);
  KZ_CHECK_EQ(run_command({g_kz, "observe", basis_path, g_profile, "-o", g_dir + "/u.kzt", "--proc-capture", "environ", "--", "/bin/true"}).code, 1);
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 9) {
    std::fprintf(stderr, "usage: %s target_prog dwarf4 alt target.kzp target_prog.c source_root kz build_dir\n", argv[0]);
    return 64;
  }
  g_target = argv[1];
  g_profile = argv[4];
  g_source_root = argv[6];
  g_kz = argv[7];
  g_dir = std::string(argv[8]) + "/observe_it";
  mkdir(g_dir.c_str(), 0755);

  const std::string basis_path = make_basis("target.basis.kz", g_target, g_profile);
  kz::plan::LoadedBasis basis;
  KZ_CHECK_OK(kz::plan::load_basis(basis_path, &basis));
  kz::trace::SymbolIndex syms;
  KZ_CHECK_OK(kz::trace::load_symbols(basis.container, &syms));

  // First an observation that tells whether the machine lets this process attach probes at all.
  {
    Observation probe = observe("capability", basis_path, g_profile, {}, {g_target, "burst", "1"});
    if (no_permission(probe.run)) {
      std::fprintf(stderr, "SKIPPED: the probes cannot be attached here: %s\n", probe.run.output.c_str());
      return 77;
    }
    KZ_CHECK_EQ(probe.run.code, 0);
    if (probe.run.code != 0) std::fprintf(stderr, "kz observe: %s\n", probe.run.output.c_str());
  }

  test_basic(basis_path, basis, syms);
  test_threads(basis_path);
  test_burst(basis_path);
  test_copy_and_refusals(basis_path, basis);
  test_exit_codes_and_check(basis_path);
  return kztest::finish("test_probe_observe");
}

// A recovered instrumented run read by the replay (kz/trace/kzrt_convert.hpp "end", docs/schema.md section 14,
// event_semantics.md section 12): the dumps the runtime's rt_end tests leave (runtime/tests/test_rt_end.c: three threads
// record 300 records each, then the process ends by SIGKILL, by SIGSEGV inside a hook, or normally; kz-rt-dump --recover
// makes the dump of its store file), converted with a unit-test site manifest and read on the mini profile.
// Tool tests: every value is a unit-test value, not a research condition and not a case (CONDITIONS_POLICY).
//
//   - a recovered dump of a process that did not complete its own dump: run flags recovered and loss-present, never
//     complete; end signal and its number; LOSS abnormal-end with no interval inside the events (begin = end = the event
//     count); the start evidence is that of the process's own dump; the replay keeps every observed event before any
//     loss interval (the prefix is not lost) and states how the run ended;
//   - the actor that was inside a hook (SIGSEGV) has its partial loss from its last event on, and that is the replay's
//     only loss interval;
//   - recovered after the process had completed its own dump: recovered and dumped, no abnormal-end, the same events as
//     the process's own dump; that dump (in-process) is complete and says so.
//
//   test_kzrt_recovered <dir of the rt_end dumps>

#include <cstdio>
#include <string>
#include <vector>

#include "kz/core/container.hpp"
#include "kz/core/eval_trace.hpp"
#include "kz/core/phi.hpp"
#include "kz/trace/kzrt_convert.hpp"
#include "kz/trace/neutral.hpp"
#include "kz/trace/real_trace.hpp"
#include "support/kz_test.hpp"
#include "support/phi_mini.hpp"

namespace {

// The rt_end target's sites: 10 + tag per thread (main 0, workers 1 and 2), the string hook 20 of the segv case.
const char* const kManifest =
    "kzs-sites 1\n"
    "source unit rev=0000000000000000000000000000000000000000 ref=\"tests/cfs-free/test_kzrt_recovered.cpp\"\n"
    "module 1 name=test_rt_end source=unit\n"
    "site 10 label=main.wait op=wait_poll phase=before tu=t.c func=record callee=wait_poll line=10\n"
    "value 10 0 role=Level from=arg0 type=u32\n"
    "value 10 1 role=Timeout from=arg1 type=u32\n"
    "site 11 label=w1.wait op=wait_poll phase=before tu=t.c func=record callee=wait_poll line=11\n"
    "value 11 0 role=Level from=arg0 type=u32\n"
    "value 11 1 role=Timeout from=arg1 type=u32\n"
    "site 12 label=w2.wait op=wait_poll phase=before tu=t.c func=record callee=wait_poll line=12\n"
    "value 12 0 role=Level from=arg0 type=u32\n"
    "value 12 1 role=Timeout from=arg1 type=u32\n"
    "site 20 label=str.copy op=other phase=point tu=t.c func=child line=20\n"
    "value 20 0 role=Name from=arg0 type=ptr string=validated bound=16 ref=\"unit test: the copy the hook makes\"\n"
    "bind 100 site=10 backend=hand module=1 at=\"t.c:10\" status=bound\n"
    "bind 101 site=11 backend=hand module=1 at=\"t.c:11\" status=bound\n"
    "bind 102 site=12 backend=hand module=1 at=\"t.c:12\" status=bound\n"
    "bind 200 site=20 backend=hand module=1 at=\"t.c:20\" status=bound\n";

constexpr size_t kEvents = 900;  // 3 threads x 300 records (test_rt_end.c)

struct Read {
  kz::trace::KzrtConvertResult r;
  kz::eval::Trace t;
  kz::trace::RealTraceReport rep;
  bool ok = false;
};

bool has_note(const std::vector<std::string>& notes, const std::string& fragment) {
  for (const std::string& n : notes)
    if (n.find(fragment) != std::string::npos) return true;
  return false;
}

Read read(const std::string& path, const kz::phi::Profile& p) {
  Read x;
  kz::trace::KzrtConvertInput in;
  if (!kz::read_whole_file(path, &in.dump).ok()) {
    std::fprintf(stderr, "cannot read %s\n", path.c_str());
    KZ_CHECK(false);
    return x;
  }
  in.dump_name = path;
  in.manifests.push_back({"unit.kzs", kManifest});
  KZ_CHECK_OK(kz::trace::convert_kzrt_container(in, &x.r));
  for (const std::string& n : x.r.notes) std::printf("%s: note %s\n", path.c_str(), n.c_str());
  // the replay reads the written container, as kz replay does
  kz::Container c;
  KZ_CHECK_OK(kz::Container::parse(x.r.container, &c));
  kz::trace::RealTraceOptions opt;
  opt.consts["TABLE"] = 4;
  KZ_CHECK_OK(kz::trace::real_trace_from_run(c, p, opt, &x.t, &x.rep));
  x.ok = x.r.layer.events.size() == kEvents && x.t.events.size() == kEvents;
  KZ_CHECK(x.ok);
  return x;
}

size_t count_kind(const kz::trace::NeutralRun& n, uint8_t kind) {
  size_t k = 0;
  for (const kz_nloss& l : n.losses) k += l.kind == kind ? 1 : 0;
  return k;
}

// The checks every recovered dump of an unfinished process passes.
void check_prefix(const Read& x, uint32_t signal, uint8_t start) {
  const kz::trace::NeutralRun& n = x.r.layer;
  const uint32_t f = n.run.r.flags;
  KZ_CHECK((f & KZ_NRUN_FLAG_RECOVERED) != 0 && (f & KZ_NRUN_FLAG_DUMPED) == 0 && (f & KZ_NRUN_FLAG_COMPLETE) == 0);
  KZ_CHECK((f & KZ_NRUN_FLAG_LOSS_PRESENT) != 0 && (f & KZ_NRUN_FLAG_LOSS_EVIDENCE) != 0);
  KZ_CHECK_EQ(n.run.r.end_kind, KZ_NEND_SIGNAL);
  KZ_CHECK_EQ(n.run.r.end_value, signal);
  KZ_CHECK_EQ(n.run.r.start_evidence, start);  // the end does not change the start
  KZ_CHECK_EQ(count_kind(n, KZ_NLOSS_ABNORMAL_END), static_cast<size_t>(1));
  KZ_CHECK_EQ(count_kind(n, KZ_NLOSS_UNKNOWN) + count_kind(n, KZ_NLOSS_TRUNCATED_STREAM), static_cast<size_t>(0));  // read as whole before
  for (const kz_nloss& l : n.losses)
    if (l.kind == KZ_NLOSS_ABNORMAL_END) KZ_CHECK(l.count == 1 && l.begin == kEvents && l.end == kEvents && l.actor == KZ_NONE32);
  KZ_CHECK(has_note(x.r.notes, "end: recovered from the store after the process ended (signal " + std::to_string(signal) +
                                   "), store file: the records are the prefix the store held (abnormal-end)"));
  KZ_CHECK(x.rep.loss_present);
  KZ_CHECK(x.rep.end.find("(signal " + std::to_string(signal) + "): every actor's records are the gap-free prefix") != std::string::npos);
  KZ_CHECK(kz::trace::report_text(x.rep).find("record end: recovered from the runtime's store") != std::string::npos);
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::fprintf(stderr, "usage: test_kzrt_recovered <dir of the rt_end dumps>\n");
    return 2;
  }
  const std::string dir = argv[1];
  kz::phi::Profile p;
  std::vector<kz::phi::Diag> diags;
  KZ_CHECK_OK(kz::phi::load_profile(kzmini::files(), &p, &diags));

  // the process's own dump of the normal exit (the exit handler; the store was a file)
  const Read own = read(dir + "/rt_end_normal.kzrt", p);
  if (own.ok) {
    const uint32_t f = own.r.layer.run.r.flags;
    KZ_CHECK((f & KZ_NRUN_FLAG_COMPLETE) != 0 && (f & KZ_NRUN_FLAG_RECOVERED) == 0 && (f & KZ_NRUN_FLAG_LOSS_PRESENT) == 0);
    KZ_CHECK_EQ(own.r.layer.run.r.end_kind, KZ_NEND_IN_PROCESS);
    KZ_CHECK(has_note(own.r.notes, "end: the process's own dump, store file"));
    KZ_CHECK_EQ(own.rep.end, std::string("the process's own dump (in-process)"));
    KZ_CHECK(own.t.losses.empty());
  }
  const uint8_t start = own.r.layer.run.r.start_evidence;

  // SIGKILL: every actor's prefix whole, no loss interval at all
  const Read kill = read(dir + "/rt_end_kill.rec.kzrt", p);
  if (kill.ok) {
    check_prefix(kill, 9, start);
    KZ_CHECK_EQ(count_kind(kill.r.layer, KZ_NLOSS_PARTIAL), static_cast<size_t>(0));
    KZ_CHECK(kill.t.losses.empty());  // the observed prefix is not lost
    KZ_CHECK_EQ(kill.rep.loss_intervals, 0u);
  }

  // SIGSEGV inside a hook of main: main's record being written is its partial loss, from its last event on
  const Read segv = read(dir + "/rt_end_segv.rec.kzrt", p);
  if (segv.ok) {
    check_prefix(segv, 11, start);
    const kz::trace::NeutralRun& n = segv.r.layer;
    const kz_nloss* partial = nullptr;
    for (const kz_nloss& l : n.losses)
      if (l.kind == KZ_NLOSS_PARTIAL) {
        KZ_CHECK(partial == nullptr);
        partial = &l;
      }
    KZ_CHECK(partial != nullptr);
    if (partial != nullptr) {
      KZ_CHECK(partial->actor != KZ_NONE32 && partial->unit_kind == KZ_NLOSS_UNIT_ACTOR_BUFFER && partial->end == KZ_NONE32 && partial->count == 1);
      uint32_t last = KZ_NONE32;
      size_t of_actor = 0;
      for (size_t i = 0; i < n.events.size(); ++i)
        if (n.events[i].actor == partial->actor) {
          last = static_cast<uint32_t>(i);
          ++of_actor;
        }
      KZ_CHECK_EQ(of_actor, static_cast<size_t>(300));  // main's whole prefix of records is there
      KZ_CHECK_EQ(partial->begin, last + 1);
      // the replay's only interval is that one: every event up to main's last is before it
      KZ_CHECK_EQ(segv.t.losses.size(), static_cast<size_t>(1));
      if (segv.t.losses.size() == 1) {
        KZ_CHECK_EQ(segv.t.losses[0].begin, static_cast<uint64_t>(last) + 1);
        KZ_CHECK(segv.t.losses[0].end == kz::eval::kNoEvent);
        KZ_CHECK(segv.t.losses[0].begin > 0);
      }
    }
  }

  // recovered after the process had completed its own dump: the same records, not a prefix
  const Read after = read(dir + "/rt_end_normal.rec.kzrt", p);
  if (after.ok && own.ok) {
    const kz::trace::NeutralRun& n = after.r.layer;
    const uint32_t f = n.run.r.flags;
    KZ_CHECK((f & KZ_NRUN_FLAG_RECOVERED) != 0 && (f & KZ_NRUN_FLAG_DUMPED) != 0 && (f & KZ_NRUN_FLAG_COMPLETE) == 0 && (f & KZ_NRUN_FLAG_LOSS_PRESENT) == 0);
    KZ_CHECK_EQ(n.run.r.end_kind, KZ_NEND_EXIT);
    KZ_CHECK_EQ(n.run.r.end_value, 0u);
    KZ_CHECK_EQ(count_kind(n, KZ_NLOSS_ABNORMAL_END), static_cast<size_t>(0));
    KZ_CHECK(after.t.losses.empty());
    KZ_CHECK(after.rep.end.find("(exit status 0); the process had completed its own dump first") != std::string::npos);
    KZ_CHECK(kz::eval::trace_text(after.t) == kz::eval::trace_text(own.t));
  }
  return kztest::finish("test_kzrt_recovered");
}

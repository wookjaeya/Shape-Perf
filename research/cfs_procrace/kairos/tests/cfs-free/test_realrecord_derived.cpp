// Derived records of a real record (masterplan 2026-10-24 section 9.1, priority 4; SPEC 4.8 "Validation of
// the observation judgement"): the real-record replay must not become more decisive when information is
// removed.  A definite summary of a derived record (OBSERVED_VIOLATION, NO_VIOLATION_IN_CHECKED_PREFIX)
// must be the summary of the same occurrence in the original record; an INCONCLUSIVE occurrence of the
// original stays INCONCLUSIVE; no NO_VIOLATION_IN_CHECKED_PREFIX appears from missing writes.
//
// Input: the run container of the S1-R2 development run (env/logs/s1r2_p1.kzt, docs/observer_smoke_run.md;
// a development run, not an experiment result), read in place and not copied, with the profile
// cfs-v7.0.1.  The one value the container does not carry, the size of the application table, is taken
// from the extraction golden of the pinned source (tests/static/data/core_cpu1.golden, the line
// `const.CFE_PLATFORM_ES_MAX_APPLICATIONS = ...`), not chosen here.  The derived records are tests of the
// analyser's handling of incomplete information (deleted events, a dropped site, a late start); they are
// not evidence about the program and their counts are not results.  Seeds and cut points are unit-test
// values.  Exits 77 (skipped) when the container is not present.
//
// argv: <profile dir> <run.kzt> <core_cpu1.golden>

#include <cstdio>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include "kz/core/container.hpp"
#include "kz/core/eval_trace.hpp"
#include "kz/core/phi.hpp"
#include "kz/core/verdict.hpp"
#include "kz/trace/real_trace.hpp"
#include "support/kz_test.hpp"

namespace {

namespace fs = std::filesystem;
using kz::eval::Occurrence;
using kz::eval::Result;
using kz::eval::Trace;

struct Rng {
  uint64_t s;
  explicit Rng(uint64_t seed) : s(seed * 0x9E3779B97F4A7C15ull + 1) {}
  uint64_t next() {
    s ^= s << 13;
    s ^= s >> 7;
    s ^= s << 17;
    return s;
  }
  uint64_t below(uint64_t n) { return n == 0 ? 0 : next() % n; }
};

// A derived record: the events kept (by position in the original), the losses declared over them.
struct Derived {
  Trace trace;
  std::vector<uint64_t> orig;  // position in the original trace of each kept event
};

Derived derive(const Trace& t, const std::vector<bool>& keep) {
  Derived d;
  d.trace = t;
  d.trace.events.clear();
  d.trace.losses.clear();
  for (size_t i = 0; i < t.events.size(); ++i) {
    if (!keep[i]) continue;
    d.trace.add(t.events[i]);
    d.orig.push_back(i);
  }
  return d;
}

// Declares one loss over the positions of the derived record where original events [first, last] were.
void declare_loss(Derived* d, uint64_t first, uint64_t last) {
  kz::eval::LossInterval l;
  l.begin = d->orig.size();
  l.end = kz::eval::kNoEvent;
  for (size_t i = 0; i < d->orig.size(); ++i)
    if (d->orig[i] >= first) {
      l.begin = i;
      break;
    }
  for (size_t i = 0; i < d->orig.size(); ++i)
    if (d->orig[i] > last) {
      l.end = i;
      break;
    }
  d->trace.losses.push_back(l);
}

bool definite(uint8_t obs) { return obs == KZ_OBS_OBSERVED_VIOLATION || obs == KZ_OBS_NO_VIOLATION_IN_CHECKED_PREFIX; }

struct Tally {
  long derived_records = 0, occurrences = 0, definite = 0, weakened = 0, violations = 0;
};

// The monotonicity check of one derived record against the original.
void compare(const kz::phi::Profile& p, const Result& base, const Derived& d, const char* what, Tally* tally) {
  Result r;
  const kz::Status s = kz::eval::replay(p, d.trace, kz::eval::Options(), &r);
  KZ_CHECK(s.ok());
  ++tally->derived_records;
  std::map<uint64_t, const Occurrence*> by_call;
  for (const Occurrence& o : base.occurrences) by_call[o.call] = &o;
  for (const Occurrence& o : r.occurrences) {
    ++tally->occurrences;
    const auto it = by_call.find(d.orig[o.call]);
    KZ_CHECK(it != by_call.end());
    if (it == by_call.end()) continue;
    const Occurrence& b = *it->second;
    const uint8_t got = o.summary.observation, had = b.summary.observation;
    if (definite(got)) ++tally->definite;
    if (definite(had) && !definite(got)) ++tally->weakened;
    const bool ok = !definite(got) || got == had;
    if (!ok) {
      ++tally->violations;
      if (tally->violations <= 5)
        std::fprintf(stderr, "%s: occurrence at original #%llu is %s in the derived record and %s in the original\n", what, static_cast<unsigned long long>(b.call),
                     kz::render_observation(o.summary).c_str(), kz::render_observation(b.summary).c_str());
    }
    KZ_CHECK(ok);
    KZ_CHECK(kz::validate_summary(o.summary).empty());
  }
}

int64_t table_size(const std::string& golden) {
  kz::Bytes b;
  if (!kz::read_whole_file(golden, &b).ok()) return -1;
  const std::string text(b.begin(), b.end());
  const std::string key = "const.CFE_PLATFORM_ES_MAX_APPLICATIONS = ";
  const size_t at = text.find(key);
  if (at == std::string::npos) return -1;
  return std::strtoll(text.c_str() + at + key.size(), nullptr, 10);
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 4) {
    std::fprintf(stderr, "usage: test_realrecord_derived <profile dir> <run.kzt> <core_cpu1.golden>\n");
    return 2;
  }
  if (!fs::exists(argv[2])) {
    std::printf("test_realrecord_derived: %s is not present: skipped\n", argv[2]);
    return 77;
  }
  // the profile
  std::vector<kz::phi::SourceText> files;
  std::vector<std::string> paths;
  for (const fs::directory_entry& e : fs::directory_iterator(argv[1]))
    if (e.is_regular_file() && e.path().extension() == ".kzp") paths.push_back(e.path().string());
  std::sort(paths.begin(), paths.end());
  for (const std::string& p : paths) {
    kz::Bytes b;
    KZ_CHECK(kz::read_whole_file(p, &b).ok());
    files.push_back(kz::phi::SourceText{p, std::string(b.begin(), b.end())});
  }
  kz::phi::Profile p;
  std::vector<kz::phi::Diag> diags;
  KZ_CHECK(kz::phi::load_profile(files, &p, &diags).ok());
  // the record
  kz::Container c;
  kz::Status s = kz::Container::read_file(argv[2], &c);
  if (s.ok()) s = c.verify_content();
  KZ_CHECK(s.ok());
  Trace base;
  kz::trace::RealTraceReport report;
  KZ_CHECK(kz::trace::real_trace_from_run(c, p, &base, &report).ok());
  const int64_t n_apps = table_size(argv[3]);
  KZ_CHECK(n_apps > 0);
  base.ref_consts["CFE_PLATFORM_ES_MAX_APPLICATIONS"] = n_apps;
  // an earlier container (schema revision 1): no owner section, every owner undetermined
  KZ_CHECK(!report.owner_section);
  KZ_CHECK(base.owner_levels.empty());
  KZ_CHECK(base.record == kz::eval::RecordKind::Real && !base.order_established);

  Result r0;
  KZ_CHECK(kz::eval::replay(p, base, kz::eval::Options(), &r0).ok());
  // The original record (SPEC App. A.1 expected result on the 8-site plan): the uses whose identity
  // argument is 0 are OBSERVED_VIOLATION from the argument alone; every other use is INCONCLUSIVE (the
  // registry's writers and the registry conjunct's point are not in the plan); nothing is
  // NO_VIOLATION_IN_CHECKED_PREFIX and nothing is an input error.
  long violations = 0, inconclusive = 0, no_violation = 0, other = 0;
  for (const Occurrence& o : r0.occurrences) {
    switch (o.summary.observation) {
      case KZ_OBS_OBSERVED_VIOLATION:
        ++violations;
        KZ_CHECK(o.has_passed && o.passed == 0);
        KZ_CHECK(o.owner_level == KZ_OWNER_LEVEL_UNDETERMINED);
        break;
      case KZ_OBS_INCONCLUSIVE: ++inconclusive; break;
      case KZ_OBS_NO_VIOLATION_IN_CHECKED_PREFIX: ++no_violation; break;
      default: ++other; break;
    }
    KZ_CHECK(!o.input_error);
  }
  std::printf("original record: %zu occurrences, %ld OBSERVED_VIOLATION, %ld INCONCLUSIVE, %ld NO_VIOLATION_IN_CHECKED_PREFIX, %ld other\n", r0.occurrences.size(), violations,
              inconclusive, no_violation, other);
  KZ_CHECK_EQ(violations, 24L);
  KZ_CHECK_EQ(no_violation, 0L);
  KZ_CHECK_EQ(other, 0L);
  KZ_CHECK(!r0.partial);

  Tally tally;
  const size_t n = base.events.size();
  // 1. a site dropped from the plan: its events and its observed point are gone
  std::map<std::pair<std::string, int>, bool> sites;
  for (const kz::eval::TraceEvent& e : base.events) sites[{e.site, static_cast<int>(e.phase)}] = true;
  for (const auto& kv : sites) {
    std::vector<bool> keep(n, true);
    for (size_t i = 0; i < n; ++i)
      if (base.events[i].site == kv.first.first && static_cast<int>(base.events[i].phase) == kv.first.second) keep[i] = false;
    Derived d = derive(base, keep);
    d.trace.observed_points.erase(kv.first.first);
    compare(p, r0, d, ("site dropped: " + kv.first.first).c_str(), &tally);
  }
  // 2. blocks of events lost, the loss declared (a lossy run)
  for (uint64_t seed = 1; seed <= 24; ++seed) {
    Rng rng(seed);
    const uint64_t len = 1 + rng.below(60), first = rng.below(n);
    const uint64_t last = std::min<uint64_t>(n - 1, first + len - 1);
    std::vector<bool> keep(n, true);
    for (uint64_t i = first; i <= last; ++i) keep[i] = false;
    Derived d = derive(base, keep);
    declare_loss(&d, first, last);
    compare(p, r0, d, "block lost, declared", &tally);
  }
  // 3. scattered events deleted, with and without a declared loss (nothing in this record is fixed by
  //    recorded writes, so even a silent deletion must not make anything more decisive)
  for (uint64_t seed = 101; seed <= 116; ++seed) {
    Rng rng(seed);
    std::vector<bool> keep(n, true);
    uint64_t first = n, last = 0;
    for (size_t i = 0; i < n; ++i)
      if (rng.below(100) < 5) {
        keep[i] = false;
        first = std::min<uint64_t>(first, i);
        last = std::max<uint64_t>(last, i);
      }
    Derived silent = derive(base, keep);
    compare(p, r0, silent, "scattered deletion, silent", &tally);
    Derived declared = derive(base, keep);
    if (first <= last) declare_loss(&declared, first, last);
    compare(p, r0, declared, "scattered deletion, declared", &tally);
  }
  // 4. a late start: the observer attached after the first N events; the start state is not evidenced
  for (uint64_t cut : {1ull, 10ull, 50ull, 100ull, 200ull, 300ull, 450ull, 600ull}) {
    if (cut >= n) continue;
    std::vector<bool> keep(n, true);
    for (uint64_t i = 0; i < cut; ++i) keep[i] = false;
    Derived d = derive(base, keep);
    d.trace.start_evidence = KZ_START_EVIDENCE_NONE;
    for (auto& kv : d.trace.kind_adequacy) kv.second.start_known = false;
    for (auto& kv : d.trace.loc_adequacy) kv.second.start_known = false;
    compare(p, r0, d, "late start", &tally);
  }
  std::printf("derived records: %ld, occurrences compared %ld, definite %ld, weakened %ld, more decisive than the original %ld\n", tally.derived_records, tally.occurrences,
              tally.definite, tally.weakened, tally.violations);
  KZ_CHECK_EQ(tally.violations, 0L);
  KZ_CHECK(tally.derived_records >= 50);
  return kztest::finish("test_realrecord_derived");
}

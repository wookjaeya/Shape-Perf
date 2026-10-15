// Unit tests of the identity join: creation relation, creator-side intervals, announced ids and
// kernel names, on hand-made event streams.  Every scenario below states what the join must
// establish and what it must refuse to guess.
#include <algorithm>
#include <cstring>

#include "kz/trace/identity_join.hpp"
#include "support/kz_test.hpp"

using namespace kz::trace;

namespace {

enum : uint32_t { kSiteCreate = 0, kSiteReturn = 1, kSiteEntry = 2, kSiteOther = 3 };

std::vector<SiteSpec> sites() {
  std::vector<SiteSpec> v(4);
  for (uint32_t i = 0; i < 4; ++i) {
    v[i].site = i;
    v[i].label = "site" + std::to_string(i);
    v[i].kind = KZ_SITE_KIND_ENTRY_PROBE;
    v[i].phases = KZ_SITE_PHASE_ENTRY;
    v[i].decoder = KZ_DECODER_GENERIC;
  }
  v[kSiteCreate].decoder = KZ_DECODER_SPAWN_CREATE;
  v[kSiteReturn].kind = KZ_SITE_KIND_RET_PROBE;
  v[kSiteReturn].phases = KZ_SITE_PHASE_RET;
  v[kSiteReturn].decoder = KZ_DECODER_SPAWN_RETURN;
  v[kSiteEntry].decoder = KZ_DECODER_THREAD_ENTRY;
  return v;
}

struct Trace {
  std::vector<kz_raw_ev> events;
  std::vector<SideEvent> side;
  std::vector<SiteSpec> sites = ::sites();
  std::unordered_map<uint32_t, uint32_t> tseq;

  uint32_t raw(uint32_t tid, uint64_t t, uint32_t site, uint64_t a0 = 0, uint64_t a1 = 0, uint64_t a2 = 0, uint64_t a3 = 0) {
    kz_raw_ev e;
    std::memset(&e, 0, sizeof e);
    e.t_ns = t; e.tid = tid; e.tseq = tseq[tid]++; e.site = site;
    e.a[0] = a0; e.a[1] = a1; e.a[2] = a2; e.a[3] = a3;
    events.push_back(e);
    return static_cast<uint32_t>(events.size() - 1);
  }
  uint32_t create(uint32_t tid, uint64_t t, uint64_t entry_fn, uint64_t prio, uint64_t retaddr) { return raw(tid, t, kSiteCreate, 0x1234, entry_fn, prio, retaddr); }
  uint32_t create_ret(uint32_t tid, uint64_t t, uint64_t status, uint64_t retaddr) { return raw(tid, t, kSiteReturn, status, 0, 0, retaddr); }
  uint32_t entry(uint32_t tid, uint64_t t, uint64_t id) { return raw(tid, t, kSiteEntry, id); }
  void fork(uint32_t pid, uint32_t tid, uint32_t ptid, uint64_t t) { side_ev(KZ_SIDE_KIND_FORK, pid, tid, ptid, t, ""); }
  void exit_(uint32_t pid, uint32_t tid, uint64_t t) { side_ev(KZ_SIDE_KIND_EXIT, pid, tid, 0, t, ""); }
  void comm(uint32_t pid, uint32_t tid, uint64_t t, const char* name) { side_ev(KZ_SIDE_KIND_COMM, pid, tid, 0, t, name); }
  void side_ev(uint8_t kind, uint32_t pid, uint32_t tid, uint32_t ptid, uint64_t t, const char* name) {
    SideEvent s;
    std::memset(&s.rec, 0, sizeof s.rec);
    s.rec.kind = kind; s.rec.pid = pid; s.rec.tid = tid; s.rec.ptid = ptid; s.rec.ppid = pid; s.rec.t_ns = t;
    s.comm = name;
    side.push_back(s);
  }
  std::vector<ThreadRecord> join(JoinStats* st = nullptr, const SymbolIndex* sym = nullptr, uint64_t bias = 0) {
    JoinInput in;
    in.events = &events; in.sites = &sites; in.side = &side;
    in.symbols = sym; in.bias_known = sym != nullptr; in.load_bias = bias;
    std::vector<ThreadRecord> out;
    KZ_CHECK_OK(join_identities(in, &out, st));
    return out;
  }
};

const ThreadRecord* find(const std::vector<ThreadRecord>& v, uint32_t tid, uint32_t gen = 0) {
  for (const ThreadRecord& t : v)
    if (t.rec.tid == tid && t.rec.generation == gen) return &t;
  return nullptr;
}
bool has(const ThreadRecord& t, uint32_t flag) { return (t.rec.flags & flag) != 0; }

void test_normal_spawns() {
  Trace tr;
  tr.fork(10, 10, 9, 50);
  tr.comm(10, 10, 51, "main");
  // The creator (tid 10) makes two threads; each child announces its id and names itself.
  const uint32_t c1 = tr.create(10, 100, 0x4000, 7, 0x900);
  tr.fork(10, 11, 10, 105);
  const uint32_t e1 = tr.entry(11, 110, 0x10001);
  tr.comm(10, 11, 111, "CHILD_A");
  tr.create_ret(10, 120, 0, 0x900);
  const uint32_t c2 = tr.create(10, 130, 0x5000, 9, 0x900);
  tr.fork(10, 12, 10, 135);
  tr.entry(12, 140, 0x10002);
  tr.comm(10, 12, 141, "CHILD_B");
  tr.create_ret(10, 150, 0, 0x900);
  JoinStats st;
  auto v = tr.join(&st);
  KZ_CHECK_EQ(v.size(), size_t(3));
  KZ_CHECK_EQ(st.threads, uint64_t(3));
  KZ_CHECK_EQ(st.spawn_matched, uint64_t(2));
  KZ_CHECK_EQ(st.spawn_ambiguous, uint64_t(0));
  KZ_CHECK_EQ(st.creates_without_fork, uint64_t(0));
  KZ_CHECK_EQ(st.creates_without_return, uint64_t(0));
  KZ_CHECK_EQ(st.entries_unmatched, uint64_t(0));
  const ThreadRecord* a = find(v, 11);
  const ThreadRecord* b = find(v, 12);
  const ThreadRecord* m = find(v, 10);
  KZ_CHECK(a != nullptr && b != nullptr && m != nullptr);
  KZ_CHECK_EQ(a->name, std::string("CHILD_A"));
  KZ_CHECK_EQ(a->rec.pid, 10u);
  KZ_CHECK_EQ(a->rec.ptid, 10u);
  KZ_CHECK_EQ(a->rec.announced_id, 0x10001u);
  KZ_CHECK_EQ(a->rec.creator_event, c1);
  KZ_CHECK_EQ(a->rec.entry_fn, uint64_t(0x4000));
  KZ_CHECK_EQ(a->rec.priority, 7u);
  KZ_CHECK_EQ(a->rec.first_event, e1);
  KZ_CHECK_EQ(a->rec.n_events, 1u);
  KZ_CHECK_EQ(a->rec.t_fork, uint64_t(105));
  KZ_CHECK_EQ(a->rec.t_name, uint64_t(111));
  const uint32_t all = KZ_THREAD_FLAG_FORK_SEEN | KZ_THREAD_FLAG_COMM_SEEN | KZ_THREAD_FLAG_ENTRY_SEEN | KZ_THREAD_FLAG_SPAWN_MATCHED |
                       KZ_THREAD_FLAG_NAME_AFTER_FIRST_EVENT;  // the child's first event (its entry) precedes its rename
  KZ_CHECK_EQ(a->rec.flags, all);
  KZ_CHECK_EQ(b->name, std::string("CHILD_B"));
  KZ_CHECK_EQ(b->rec.creator_event, c2);
  KZ_CHECK_EQ(b->rec.entry_fn, uint64_t(0x5000));
  KZ_CHECK_EQ(b->rec.priority, 9u);
  // the creator itself: named, forked by someone outside the trace, no creator-side event
  KZ_CHECK_EQ(m->name, std::string("main"));
  KZ_CHECK_EQ(m->rec.ptid, 9u);
  KZ_CHECK_EQ(m->rec.creator_event, KZ_NONE32);
  KZ_CHECK_EQ(m->rec.announced_id, KZ_NONE32);
  KZ_CHECK(!has(*m, KZ_THREAD_FLAG_SPAWN_MATCHED));
  KZ_CHECK_EQ(m->rec.n_events, 4u);
  KZ_CHECK_EQ(m->rec.first_event, c1);
  // ordered by (pid, tid, generation)
  KZ_CHECK_EQ(v[0].rec.tid, 10u);
  KZ_CHECK_EQ(v[1].rec.tid, 11u);
  KZ_CHECK_EQ(v[2].rec.tid, 12u);
}

void test_symbolization() {
  Trace tr;
  tr.fork(10, 11, 10, 105);
  tr.create(10, 100, 0x7f00004000, 7, 0x900);
  tr.create_ret(10, 120, 0, 0x900);
  tr.entry(11, 110, 5);
  SymbolIndex idx;
  SymbolEntry e;
  e.vaddr = 0x4000; e.size = 0x80; e.bind = KZ_SYMBOL_BIND_GLOBAL; e.name = "task_trampoline";
  idx.add(e);
  auto v = tr.join(nullptr, &idx, 0x7f00000000ull);
  const ThreadRecord* a = find(v, 11);
  KZ_CHECK(a != nullptr);
  KZ_CHECK_EQ(a->entry_symbol, std::string("task_trampoline"));
  auto w = tr.join(nullptr, nullptr, 0);  // no symbols: no name, no error
  KZ_CHECK_EQ(find(w, 11)->entry_symbol, std::string(""));
}

void test_child_runs_before_creator_returns() {
  Trace tr;
  tr.create(10, 100, 0x4000, 1, 0x900);
  tr.fork(10, 11, 10, 101);
  tr.entry(11, 102, 1);
  tr.comm(10, 11, 103, "EARLY");
  tr.raw(11, 104, kSiteOther);
  tr.raw(11, 105, kSiteOther);
  tr.create_ret(10, 500, 0, 0x900);  // the creator is delayed; the child already ran
  auto v = tr.join();
  const ThreadRecord* a = find(v, 11);
  KZ_CHECK(a != nullptr);
  KZ_CHECK(has(*a, KZ_THREAD_FLAG_SPAWN_MATCHED));
  KZ_CHECK_EQ(a->rec.n_events, 3u);
}

void test_ambiguity_is_reported_not_guessed() {
  // Two threads forked inside one create interval: the join cannot say which the call made.
  Trace tr;
  tr.create(10, 100, 0x4000, 1, 0x900);
  tr.fork(10, 11, 10, 110);
  tr.fork(10, 12, 10, 120);  // e.g. a helper thread created by the library inside the same call
  tr.create_ret(10, 200, 0, 0x900);
  JoinStats st;
  auto v = tr.join(&st);
  for (uint32_t tid : {11u, 12u}) {
    const ThreadRecord* t = find(v, tid);
    KZ_CHECK(t != nullptr);
    KZ_CHECK(has(*t, KZ_THREAD_FLAG_SPAWN_AMBIGUOUS));
    KZ_CHECK(!has(*t, KZ_THREAD_FLAG_SPAWN_MATCHED));
    KZ_CHECK_EQ(t->rec.creator_event, KZ_NONE32);
    KZ_CHECK_EQ(t->rec.entry_fn, uint64_t(0));
  }
  KZ_CHECK_EQ(st.spawn_ambiguous, uint64_t(2));
  KZ_CHECK_EQ(st.spawn_matched, uint64_t(0));
}

void test_nested_intervals_innermost_first() {
  Trace tr;
  tr.create(10, 100, 0x4000, 1, 0x900);    // outer call
  tr.create(10, 120, 0x5000, 2, 0x910);    // a nested call from a callback inside it
  tr.fork(10, 11, 10, 130);                // belongs to the inner call
  tr.create_ret(10, 140, 0, 0x910);
  tr.fork(10, 12, 10, 160);                // belongs to the outer call only
  tr.create_ret(10, 200, 0, 0x900);
  auto v = tr.join();
  KZ_CHECK_EQ(find(v, 11)->rec.entry_fn, uint64_t(0x5000));
  KZ_CHECK_EQ(find(v, 12)->rec.entry_fn, uint64_t(0x4000));
  KZ_CHECK(has(*find(v, 11), KZ_THREAD_FLAG_SPAWN_MATCHED));
  KZ_CHECK(has(*find(v, 12), KZ_THREAD_FLAG_SPAWN_MATCHED));
}

void test_missing_events() {
  // No return event (lost, or the call never returned): the interval is open, so any later fork could be its own.
  {
    Trace tr;
    tr.create(10, 100, 0x4000, 1, 0x900);
    tr.fork(10, 11, 10, 110);
    JoinStats st;
    auto v = tr.join(&st);
    KZ_CHECK_EQ(st.creates_without_return, uint64_t(1));
    KZ_CHECK(has(*find(v, 11), KZ_THREAD_FLAG_SPAWN_AMBIGUOUS));
    KZ_CHECK(!has(*find(v, 11), KZ_THREAD_FLAG_SPAWN_MATCHED));
  }
  // A return with no entry
  {
    Trace tr;
    tr.create_ret(10, 100, 0, 0x900);
    JoinStats st;
    tr.join(&st);
    KZ_CHECK_EQ(st.returns_without_create, uint64_t(1));
  }
  // A call that created nothing (failed)
  {
    Trace tr;
    tr.create(10, 100, 0x4000, 1, 0x900);
    tr.create_ret(10, 110, 0xffffffff, 0x900);
    JoinStats st;
    tr.join(&st);
    KZ_CHECK_EQ(st.creates_without_fork, uint64_t(1));
  }
  // An entry on an inner call that returned, outer call lost its return: the outer is reported open.
  {
    Trace tr;
    tr.create(10, 100, 0x4000, 1, 0x900);
    tr.create(10, 120, 0x5000, 2, 0x910);
    tr.fork(10, 11, 10, 130);
    tr.create_ret(10, 140, 0, 0x910);
    JoinStats st;
    auto v = tr.join(&st);
    KZ_CHECK_EQ(st.creates_without_return, uint64_t(1));
    KZ_CHECK(has(*find(v, 11), KZ_THREAD_FLAG_SPAWN_MATCHED));
    KZ_CHECK_EQ(find(v, 11)->rec.entry_fn, uint64_t(0x5000));
  }
  // A return whose address matches an outer call abandons the entries above it
  {
    Trace tr;
    tr.create(10, 100, 0x4000, 1, 0x900);
    tr.create(10, 120, 0x5000, 2, 0x910);   // never returns (lost)
    tr.fork(10, 11, 10, 130);
    tr.create_ret(10, 200, 0, 0x900);       // the outer returns
    JoinStats st;
    auto v = tr.join(&st);
    KZ_CHECK_EQ(st.creates_without_return, uint64_t(1));
    KZ_CHECK(has(*find(v, 11), KZ_THREAD_FLAG_SPAWN_AMBIGUOUS));
  }
}

void test_names_and_tid_reuse() {
  Trace tr;
  tr.fork(10, 11, 10, 100);     // generation 0
  tr.comm(10, 11, 101, "FIRST");
  tr.raw(11, 150, kSiteOther);
  tr.exit_(10, 11, 200);
  tr.fork(10, 11, 10, 300);     // the kernel reuses the tid: generation 1
  tr.raw(11, 350, kSiteOther);
  tr.raw(11, 360, kSiteOther);
  tr.fork(10, 12, 10, 310);     // a thread that never names itself
  tr.raw(12, 320, kSiteOther);
  JoinStats st;
  auto v = tr.join(&st);
  KZ_CHECK_EQ(v.size(), size_t(3));
  const ThreadRecord* g0 = find(v, 11, 0);
  const ThreadRecord* g1 = find(v, 11, 1);
  KZ_CHECK(g0 != nullptr && g1 != nullptr);
  KZ_CHECK_EQ(g0->name, std::string("FIRST"));
  KZ_CHECK_EQ(g0->rec.n_events, 1u);
  KZ_CHECK_EQ(g0->rec.t_exit, uint64_t(200));
  KZ_CHECK(has(*g0, KZ_THREAD_FLAG_EXIT_SEEN));
  KZ_CHECK(!has(*g0, KZ_THREAD_FLAG_TID_REUSED));
  KZ_CHECK_EQ(g1->rec.n_events, 2u);
  KZ_CHECK(has(*g1, KZ_THREAD_FLAG_TID_REUSED));
  KZ_CHECK(has(*g1, KZ_THREAD_FLAG_NAME_INHERITED));
  KZ_CHECK_EQ(g1->name, std::string(""));
  KZ_CHECK(has(*find(v, 12), KZ_THREAD_FLAG_NAME_INHERITED));
  KZ_CHECK_EQ(st.tid_reuses, uint64_t(1));
  KZ_CHECK_EQ(st.threads_without_name, uint64_t(2));
  // the name set after the first event is flagged
  KZ_CHECK(!has(*g0, KZ_THREAD_FLAG_NAME_AFTER_FIRST_EVENT));  // renamed at 101, first event at 150
}

void test_threads_seen_without_fork() {
  // Existing before the observer started (no FORK): a COMM alone makes it known, and says so.
  Trace tr;
  tr.comm(10, 20, 100, "OLD_THREAD");
  tr.raw(20, 110, kSiteOther);
  // A thread no side event ever named: it is still a thread of the trace, with no evidence at all.
  tr.raw(30, 120, kSiteOther);
  JoinStats st;
  auto v = tr.join(&st);
  const ThreadRecord* old = find(v, 20);
  KZ_CHECK(old != nullptr);
  KZ_CHECK(!has(*old, KZ_THREAD_FLAG_FORK_SEEN));
  KZ_CHECK(has(*old, KZ_THREAD_FLAG_COMM_SEEN));
  KZ_CHECK_EQ(old->rec.ptid, KZ_NONE32);
  KZ_CHECK_EQ(old->rec.t_fork, uint64_t(0));
  const ThreadRecord* ghost = find(v, 30);
  KZ_CHECK(ghost != nullptr);
  KZ_CHECK_EQ(ghost->rec.pid, 0u);
  KZ_CHECK_EQ(ghost->rec.flags & ~uint32_t(KZ_THREAD_FLAG_NAME_INHERITED), 0u);
  KZ_CHECK_EQ(st.events_without_thread, uint64_t(1));
}

void test_entry_announcements() {
  Trace tr;
  tr.fork(10, 11, 10, 100);
  tr.entry(11, 110, 0xaa);
  tr.entry(11, 120, 0xbb);  // a second announcement by the same thread: the first stands
  JoinStats st;
  auto v = tr.join(&st);
  KZ_CHECK_EQ(find(v, 11)->rec.announced_id, 0xaau);
  KZ_CHECK_EQ(st.duplicate_entries, uint64_t(1));
  KZ_CHECK(has(*find(v, 11), KZ_THREAD_FLAG_ENTRY_UNMATCHED));  // announced, but no creator-side event matched
  KZ_CHECK_EQ(st.entries_unmatched, uint64_t(1));
  // an announced id wider than 32 bits keeps its low word
  Trace wide;
  wide.fork(10, 11, 10, 100);
  wide.entry(11, 110, 0x1234500000042ull);
  KZ_CHECK_EQ(find(wide.join(), 11)->rec.announced_id, 0x42u);
}

void test_input_order_does_not_matter() {
  Trace a;
  a.fork(10, 11, 10, 105);
  a.comm(10, 11, 111, "CHILD");
  a.fork(10, 12, 10, 135);
  a.comm(10, 12, 141, "CHILD2");
  a.create(10, 100, 0x4000, 1, 0x900);
  a.create_ret(10, 120, 0, 0x900);
  a.create(10, 130, 0x4000, 1, 0x900);
  a.create_ret(10, 150, 0, 0x900);
  Trace b = a;
  std::reverse(b.side.begin(), b.side.end());  // the side-band events arrive in another order
  auto va = a.join();
  auto vb = b.join();
  KZ_CHECK_EQ(va.size(), vb.size());
  for (size_t i = 0; i < va.size(); ++i) {
    KZ_CHECK(std::memcmp(&va[i].rec, &vb[i].rec, sizeof(kz_thread)) == 0);
    KZ_CHECK_EQ(va[i].name, vb[i].name);
  }
}

void test_required_inputs() {
  JoinInput in;
  std::vector<ThreadRecord> out;
  KZ_CHECK_ERR(join_identities(in, &out, nullptr), kz::Err::Invalid);
}

}  // namespace

int main() {
  test_normal_spawns();
  test_symbolization();
  test_child_runs_before_creator_returns();
  test_ambiguity_is_reported_not_guessed();
  test_nested_intervals_innermost_first();
  test_missing_events();
  test_names_and_tid_reuse();
  test_threads_seen_without_fork();
  test_entry_announcements();
  test_input_order_does_not_matter();
  test_required_inputs();
  return kztest::finish("test_trace_identity");
}

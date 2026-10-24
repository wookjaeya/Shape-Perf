// Unit tests of the identity join: creation relation, creator-side intervals, announced ids and
// kernel names, on hand-made event streams.  Every scenario below states what the join must
// establish and what it must refuse to guess.
#include <algorithm>
#include <cstring>

#include "kz/trace/identity_join.hpp"
#include "support/kz_test.hpp"

using namespace kz::trace;

namespace {

enum : uint32_t { kSiteCreate = 0, kSiteReturn = 1, kSiteEntry = 2, kSiteOther = 3, kSiteRequest = 4, kSiteChildRequest = 5, kSiteRequestReturn = 6, kSiteSpawnId = 7 };

std::vector<SiteSpec> sites() {
  std::vector<SiteSpec> v(8);
  for (uint32_t i = 0; i < 8; ++i) {
    v[i].site = i;
    v[i].label = "site" + std::to_string(i);
    v[i].kind = KZ_SITE_KIND_ENTRY_PROBE;
    v[i].phases = KZ_SITE_PHASE_ENTRY;
    v[i].decoder = KZ_DECODER_GENERIC;
  }
  auto slot = [](uint8_t n, uint8_t src, uint32_t index, uint8_t width, const char* role) {
    SlotSpec sl;
    sl.slot = n; sl.src = src; sl.index = index; sl.width = width; sl.role = role;
    return sl;
  };
  // The slots the decoders read (kz_raw.h, KZ_DECODER); check_site refuses a spawn site without them.
  v[kSiteCreate].decoder = KZ_DECODER_SPAWN_CREATE;
  v[kSiteCreate].slots = {slot(0, KZ_SLOT_SRC_REG, KZ_REG_RSI, 8, "name_ptr"), slot(1, KZ_SLOT_SRC_REG, KZ_REG_RDX, 8, "entry_fn"),
                          slot(2, KZ_SLOT_SRC_REG, KZ_REG_R9, 4, "priority"), slot(3, KZ_SLOT_SRC_STACK, 0, 8, "return_address")};
  v[kSiteReturn].kind = KZ_SITE_KIND_RET_PROBE;
  v[kSiteReturn].phases = KZ_SITE_PHASE_RET;
  v[kSiteReturn].decoder = KZ_DECODER_SPAWN_RETURN;
  v[kSiteReturn].slots = {slot(0, KZ_SLOT_SRC_REG, KZ_REG_RAX, 4, "status"), slot(3, KZ_SLOT_SRC_IP, 0, 8, "return_address")};
  v[kSiteEntry].decoder = KZ_DECODER_THREAD_ENTRY;
  v[kSiteEntry].slots = {slot(0, KZ_SLOT_SRC_REG, KZ_REG_RDI, 4, "task_id")};
  // Creation-side evidence of app_owner (W2): the request naming an application, the child-task
  // request, their returns, and the id the spawn call assigns.
  v[kSiteRequest].decoder = KZ_DECODER_OWNER_REQUEST;
  v[kSiteRequest].slots = {slot(2, KZ_SLOT_SRC_REG, KZ_REG_R8, 4, "owner_id"), slot(3, KZ_SLOT_SRC_STACK, 0, 8, "return_address")};
  v[kSiteChildRequest].decoder = KZ_DECODER_CHILD_REQUEST;
  v[kSiteChildRequest].slots = {slot(3, KZ_SLOT_SRC_STACK, 0, 8, "return_address")};
  v[kSiteRequestReturn].kind = KZ_SITE_KIND_RET_PROBE;
  v[kSiteRequestReturn].phases = KZ_SITE_PHASE_RET;
  v[kSiteRequestReturn].decoder = KZ_DECODER_REQUEST_RETURN;
  v[kSiteRequestReturn].slots = {slot(0, KZ_SLOT_SRC_REG, KZ_REG_RAX, 4, "status"), slot(3, KZ_SLOT_SRC_IP, 0, 8, "return_address")};
  v[kSiteSpawnId].decoder = KZ_DECODER_SPAWN_ID;
  v[kSiteSpawnId].slots = {slot(0, KZ_SLOT_SRC_REG, KZ_REG_R9, 4, "task_id"), slot(3, KZ_SLOT_SRC_STACK, 0, 8, "return_address")};
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
  // The last event had a slot the kernel did not copy: TRUNC, and that slot 0 (apply_slots).
  void truncate_last(int slot) {
    events.back().flags |= KZ_RAW_FLAG_TRUNC;
    events.back().a[slot] = 0;
  }
  uint32_t create(uint32_t tid, uint64_t t, uint64_t entry_fn, uint64_t prio, uint64_t retaddr) { return raw(tid, t, kSiteCreate, 0x1234, entry_fn, prio, retaddr); }
  uint32_t create_ret(uint32_t tid, uint64_t t, uint64_t status, uint64_t retaddr) { return raw(tid, t, kSiteReturn, status, 0, 0, retaddr); }
  uint32_t entry(uint32_t tid, uint64_t t, uint64_t id) { return raw(tid, t, kSiteEntry, id); }
  uint32_t request(uint32_t tid, uint64_t t, uint64_t owner, uint64_t retaddr) { return raw(tid, t, kSiteRequest, 0, 0, owner, retaddr); }
  uint32_t child_request(uint32_t tid, uint64_t t, uint64_t retaddr) { return raw(tid, t, kSiteChildRequest, 0, 0, 0, retaddr); }
  uint32_t request_ret(uint32_t tid, uint64_t t, uint64_t retaddr) { return raw(tid, t, kSiteRequestReturn, 0, 0, 0, retaddr); }
  uint32_t spawn_id(uint32_t tid, uint64_t t, uint64_t id) { return raw(tid, t, kSiteSpawnId, id, 0, 0, 0xabc); }
  // One task started by `creator` for a request: [request [create [spawn-id] fork] ...] on one thread,
  // the child announcing `announced` (KZ_NONE32: no announcement).  `id` 0: no spawn-id event.
  void start_task(uint32_t creator, uint32_t child, uint64_t t, uint64_t owner, bool by_child, uint32_t id, uint32_t announced, const char* name) {
    if (by_child) child_request(creator, t, 0x700); else request(creator, t, owner, 0x800);
    create(creator, t + 10, 0x4000, 5, 0x900);
    if (id != 0) spawn_id(creator, t + 15, id);
    fork(10, child, creator, t + 20);
    if (announced != KZ_NONE32) entry(child, t + 25, announced);
    comm(10, child, t + 26, name);
    create_ret(creator, t + 30, 0, 0x900);
    request_ret(creator, t + 40, by_child ? 0x700 : 0x800);
  }
  std::vector<OwnerRecord> owners(std::vector<ThreadRecord>* threads = nullptr, JoinStats* st = nullptr) {
    JoinInput in;
    in.events = &events; in.sites = &sites; in.side = &side;
    std::vector<ThreadRecord> out;
    std::vector<OwnerRecord> own;
    KZ_CHECK_OK(join_identities(in, &out, st, &own));
    KZ_CHECK_EQ(own.size(), out.size());
    for (size_t i = 0; i < own.size(); ++i) KZ_CHECK_EQ(own[i].rec.thread, static_cast<uint32_t>(i));
    if (threads != nullptr) *threads = out;
    return own;
  }
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
  KZ_CHECK_EQ(st.matched_by_time, uint64_t(2));
  KZ_CHECK_EQ(st.ids_unverified, uint64_t(2));
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
  // The creator link rests on FORK-time containment only (MATCH_BY_TIME), and nothing on the creator
  // side was compared with the announced id (ID_UNVERIFIED).
  const uint32_t all = KZ_THREAD_FLAG_FORK_SEEN | KZ_THREAD_FLAG_COMM_SEEN | KZ_THREAD_FLAG_ENTRY_SEEN | KZ_THREAD_FLAG_SPAWN_MATCHED |
                       KZ_THREAD_FLAG_MATCH_BY_TIME | KZ_THREAD_FLAG_ID_UNVERIFIED |
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
  KZ_CHECK(!has(*m, KZ_THREAD_FLAG_MATCH_BY_TIME));
  KZ_CHECK(!has(*m, KZ_THREAD_FLAG_ID_UNVERIFIED));
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
    KZ_CHECK(has(*t, KZ_THREAD_FLAG_MATCH_BY_TIME));  // the candidates too were found by time containment
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

void test_sites_must_declare_decoder_slots() {
  // A spawn site whose slots do not say what the decoder reads would be read as zeros: refused.
  for (int which = 0; which < 4; ++which) {
    Trace tr;
    tr.create(10, 100, 0x4000, 1, 0x900);
    tr.create_ret(10, 120, 0, 0x900);
    switch (which) {
      case 0: tr.sites[kSiteCreate].slots[1].role = "something_else"; break;   // entry_fn not at slot 1
      case 1: tr.sites[kSiteCreate].slots.pop_back(); break;                   // no return address
      case 2: tr.sites[kSiteReturn].slots.pop_back(); break;                   // the return has no key
      case 3: tr.sites[kSiteEntry].slots.clear(); break;                       // no announced id
    }
    JoinInput in;
    in.events = &tr.events; in.sites = &tr.sites; in.side = &tr.side;
    std::vector<ThreadRecord> out;
    KZ_CHECK_ERR(join_identities(in, &out, nullptr), kz::Err::Invalid);
  }
}

void test_truncated_slots_are_not_keys() {
  // A create whose return address was not copied (TRUNC, slot 0): it opens no interval, so a return
  // with return address 0 cannot close it, and the fork in it is not assigned to it.
  {
    Trace tr;
    tr.create(10, 100, 0x4000, 1, 0);
    tr.truncate_last(3);
    tr.fork(10, 11, 10, 105);
    tr.create_ret(10, 120, 0, 0);
    JoinStats st;
    auto v = tr.join(&st);
    KZ_CHECK_EQ(st.creates_unreadable, uint64_t(1));
    KZ_CHECK_EQ(st.returns_without_create, uint64_t(1));
    KZ_CHECK_EQ(st.spawn_matched, uint64_t(0));
    KZ_CHECK(!has(*find(v, 11), KZ_THREAD_FLAG_SPAWN_MATCHED));
    KZ_CHECK_EQ(find(v, 11)->rec.entry_fn, uint64_t(0));
  }
  // Without the TRUNC flag the same values would match: the flag, not the value, decides.
  {
    Trace tr;
    tr.create(10, 100, 0x4000, 1, 0);
    tr.fork(10, 11, 10, 105);
    tr.create_ret(10, 120, 0, 0);
    JoinStats st;
    auto v = tr.join(&st);
    KZ_CHECK_EQ(st.spawn_matched, uint64_t(1));
  }
  // A return whose key is unreadable leaves the call open: its fork is ambiguous, not matched.
  {
    Trace tr;
    tr.create(10, 100, 0x4000, 1, 0x900);
    tr.fork(10, 11, 10, 105);
    tr.create_ret(10, 120, 0, 0);
    tr.truncate_last(3);
    JoinStats st;
    auto v = tr.join(&st);
    KZ_CHECK_EQ(st.returns_unreadable, uint64_t(1));
    KZ_CHECK_EQ(st.creates_without_return, uint64_t(1));
    KZ_CHECK(has(*find(v, 11), KZ_THREAD_FLAG_SPAWN_AMBIGUOUS));
  }
  // An entry whose id was not copied announces nothing.
  {
    Trace tr;
    tr.fork(10, 11, 10, 100);
    tr.entry(11, 110, 0);
    tr.truncate_last(0);
    JoinStats st;
    auto v = tr.join(&st);
    KZ_CHECK_EQ(st.entries_unreadable, uint64_t(1));
    KZ_CHECK_EQ(find(v, 11)->rec.announced_id, KZ_NONE32);
    KZ_CHECK(!has(*find(v, 11), KZ_THREAD_FLAG_ENTRY_SEEN));
    KZ_CHECK(!has(*find(v, 11), KZ_THREAD_FLAG_ID_UNVERIFIED));
  }
}

const OwnerRecord* owner_of(const std::vector<ThreadRecord>& threads, const std::vector<OwnerRecord>& owners, uint32_t tid) {
  for (size_t i = 0; i < threads.size(); ++i)
    if (threads[i].rec.tid == tid && i < owners.size()) return &owners[i];
  return nullptr;
}

// app_owner (W2; SPEC 1.2 "Creating application", 5.7, ADR-056): the three levels and what each rests on.
void test_owner_confirmed_by_id() {
  // The request names application 0x110003; inside it the spawn call assigns id 0x40010005, and the
  // child announces the same id: the link rests on the id, the owner is confirmed.
  Trace tr;
  tr.fork(10, 10, 9, 50);
  tr.start_task(10, 11, 100, 0x110003, false, 0x40010005, 0x40010005, "APP_A");
  std::vector<ThreadRecord> th;
  JoinStats st;
  auto own = tr.owners(&th, &st);
  const OwnerRecord* o = owner_of(th, own, 11);
  KZ_CHECK(o != nullptr);
  KZ_CHECK_EQ(o->rec.level, uint8_t(KZ_OWNER_LEVEL_CONFIRMED));
  KZ_CHECK_EQ(o->rec.owner_key, 0x110003u);
  KZ_CHECK_EQ(o->rec.evidence, uint32_t(KZ_OWNER_EVIDENCE_CREATION_REQUEST | KZ_OWNER_EVIDENCE_FORK_IN_CALL | KZ_OWNER_EVIDENCE_ANNOUNCED_ID));
  KZ_CHECK_EQ(o->app_name, std::string("APP_A"));
  KZ_CHECK_EQ(o->rec.creation_event, 0u);  // the request event
  KZ_CHECK_EQ(th[o->rec.creator_thread].rec.tid, 10u);  // the creating executor, kept apart from the owner
  const ThreadRecord* t = find(th, 11);
  KZ_CHECK(has(*t, KZ_THREAD_FLAG_ID_MATCHED));
  KZ_CHECK(has(*t, KZ_THREAD_FLAG_SPAWN_MATCHED));
  KZ_CHECK(!has(*t, KZ_THREAD_FLAG_MATCH_BY_TIME));
  KZ_CHECK(!has(*t, KZ_THREAD_FLAG_ID_UNVERIFIED));
  KZ_CHECK_EQ(st.ids_matched, uint64_t(1));
  KZ_CHECK_EQ(st.matched_by_time, uint64_t(0));
  KZ_CHECK_EQ(st.owners_confirmed, uint64_t(1));
  KZ_CHECK_EQ(st.owners_undetermined, uint64_t(1));  // the creator itself: no request made it
  KZ_CHECK_EQ(owner_of(th, own, 10)->rec.level, uint8_t(KZ_OWNER_LEVEL_UNDETERMINED));
}

void test_owner_inferred_by_time() {
  // The same without the assigned id: only the FORK time ties the child to the call.
  Trace tr;
  tr.fork(10, 10, 9, 50);
  tr.start_task(10, 11, 100, 0x110003, false, 0, 0x40010005, "APP_A");
  std::vector<ThreadRecord> th;
  JoinStats st;
  auto own = tr.owners(&th, &st);
  const OwnerRecord* o = owner_of(th, own, 11);
  KZ_CHECK_EQ(o->rec.level, uint8_t(KZ_OWNER_LEVEL_INFERRED));
  KZ_CHECK_EQ(o->rec.owner_key, 0x110003u);
  KZ_CHECK_EQ(o->rec.evidence, uint32_t(KZ_OWNER_EVIDENCE_CREATION_REQUEST | KZ_OWNER_EVIDENCE_FORK_IN_CALL | KZ_OWNER_EVIDENCE_MATCH_BY_TIME |
                                         KZ_OWNER_EVIDENCE_ID_UNVERIFIED));
  KZ_CHECK(has(*find(th, 11), KZ_THREAD_FLAG_MATCH_BY_TIME));
  KZ_CHECK(has(*find(th, 11), KZ_THREAD_FLAG_ID_UNVERIFIED));
  KZ_CHECK_EQ(st.owners_inferred, uint64_t(1));
}

void test_owner_undetermined() {
  // (a) A spawn call that no request encloses: the creator link holds, the application is not named.
  {
    Trace tr;
    tr.fork(10, 10, 9, 50);
    tr.create(10, 100, 0x4000, 5, 0x900);
    tr.spawn_id(10, 105, 0x40010001);
    tr.fork(10, 11, 10, 110);
    tr.entry(11, 115, 0x40010001);
    tr.create_ret(10, 120, 0, 0x900);
    std::vector<ThreadRecord> th;
    auto own = tr.owners(&th);
    const OwnerRecord* o = owner_of(th, own, 11);
    KZ_CHECK_EQ(o->rec.level, uint8_t(KZ_OWNER_LEVEL_UNDETERMINED));
    KZ_CHECK_EQ(o->rec.owner_key, KZ_NONE32);
    KZ_CHECK_EQ(o->rec.evidence, uint32_t(KZ_OWNER_EVIDENCE_FORK_IN_CALL | KZ_OWNER_EVIDENCE_ANNOUNCED_ID));
    KZ_CHECK_EQ(o->rec.creation_event, 0u);  // the creating call
  }
  // (b) The assigned id contradicts the thread the FORK time gives: no link, no owner.
  {
    Trace tr;
    tr.fork(10, 10, 9, 50);
    tr.start_task(10, 11, 100, 0x110003, false, 0x40010005, 0x40010006, "APP_A");
    std::vector<ThreadRecord> th;
    JoinStats st;
    auto own = tr.owners(&th, &st);
    KZ_CHECK_EQ(owner_of(th, own, 11)->rec.level, uint8_t(KZ_OWNER_LEVEL_UNDETERMINED));
    KZ_CHECK_EQ(owner_of(th, own, 11)->rec.owner_key, KZ_NONE32);
    KZ_CHECK(has(*find(th, 11), KZ_THREAD_FLAG_ID_MISMATCH));
    KZ_CHECK(!has(*find(th, 11), KZ_THREAD_FLAG_SPAWN_MATCHED));
    KZ_CHECK_EQ(st.ids_mismatched, uint64_t(1));
  }
  // (c) A thread created without a probed spawn call (a raw thread), and a thread seen without FORK.
  {
    Trace tr;
    tr.fork(10, 11, 10, 100);
    tr.comm(10, 20, 100, "OLD");
    std::vector<ThreadRecord> th;
    auto own = tr.owners(&th);
    KZ_CHECK_EQ(owner_of(th, own, 11)->rec.evidence, uint32_t(KZ_OWNER_EVIDENCE_RAW_THREAD));
    KZ_CHECK_EQ(owner_of(th, own, 20)->rec.evidence, 0u);
    KZ_CHECK_EQ(owner_of(th, own, 11)->rec.level, uint8_t(KZ_OWNER_LEVEL_UNDETERMINED));
  }
  // (d) A request whose slots were not copied (TRUNC): it names nothing and closes nothing.
  {
    Trace tr;
    tr.fork(10, 10, 9, 50);
    tr.request(10, 100, 0, 0);
    tr.truncate_last(2);
    tr.create(10, 110, 0x4000, 5, 0x900);
    tr.spawn_id(10, 115, 0x40010005);
    tr.fork(10, 11, 10, 120);
    tr.entry(11, 125, 0x40010005);
    tr.create_ret(10, 130, 0, 0x900);
    std::vector<ThreadRecord> th;
    JoinStats st;
    auto own = tr.owners(&th, &st);
    KZ_CHECK_EQ(owner_of(th, own, 11)->rec.level, uint8_t(KZ_OWNER_LEVEL_UNDETERMINED));
    KZ_CHECK_EQ(st.requests_unreadable, uint64_t(1));
  }
}

void test_owner_of_child_task() {
  // Application A's main task (confirmed) asks for a child task.  The start-task request nested in the
  // child request names B (its argument is the creator's lookup in the App table): not used.  The child
  // belongs to A, at most at the creator's level.
  Trace tr;
  tr.fork(10, 10, 9, 50);
  tr.start_task(10, 11, 100, 0x110003, false, 0x40010005, 0x40010005, "APP_A");
  tr.child_request(11, 300, 0x700);
  tr.request(11, 305, 0x110009, 0x800);
  tr.create(11, 310, 0x4100, 5, 0x900);
  tr.spawn_id(11, 315, 0x40010006);
  tr.fork(10, 12, 11, 320);
  tr.entry(12, 325, 0x40010006);
  tr.comm(10, 12, 326, "APP_A_CHILD");
  tr.create_ret(11, 330, 0, 0x900);
  tr.request_ret(11, 335, 0x800);
  tr.request_ret(11, 340, 0x700);
  std::vector<ThreadRecord> th;
  auto own = tr.owners(&th);
  const OwnerRecord* c = owner_of(th, own, 12);
  KZ_CHECK_EQ(c->rec.level, uint8_t(KZ_OWNER_LEVEL_CONFIRMED));
  KZ_CHECK_EQ(c->rec.owner_key, 0x110003u);
  KZ_CHECK_EQ(c->app_name, std::string("APP_A"));
  KZ_CHECK((c->rec.evidence & KZ_OWNER_EVIDENCE_CREATOR_OWNER) != 0);
  KZ_CHECK((c->rec.evidence & KZ_OWNER_EVIDENCE_ANALYSED_TABLE_EXCLUDED) != 0);
  KZ_CHECK_EQ(th[c->rec.creator_thread].rec.tid, 11u);
  // A child of a creator whose owner is only inferred is at most inferred; of an undetermined one, undetermined.
  Trace weak;
  weak.fork(10, 10, 9, 50);
  weak.start_task(10, 11, 100, 0x110003, false, 0, 0x40010005, "APP_A");
  weak.start_task(11, 12, 300, 0, true, 0x40010006, 0x40010006, "APP_A_CHILD");
  weak.start_task(10, 13, 500, 0, true, 0x40010007, 0x40010007, "ORPHAN");
  std::vector<ThreadRecord> wt;
  auto wo = weak.owners(&wt);
  KZ_CHECK_EQ(owner_of(wt, wo, 12)->rec.level, uint8_t(KZ_OWNER_LEVEL_INFERRED));
  KZ_CHECK_EQ(owner_of(wt, wo, 12)->rec.owner_key, 0x110003u);
  KZ_CHECK_EQ(owner_of(wt, wo, 13)->rec.level, uint8_t(KZ_OWNER_LEVEL_UNDETERMINED));  // tid 10 has no owner
  KZ_CHECK_EQ(owner_of(wt, wo, 13)->rec.owner_key, KZ_NONE32);
}

void test_id_resolves_time_ambiguity() {
  // Two threads forked inside one call: time alone is ambiguous; the assigned id picks one, the other
  // stays ambiguous.
  Trace tr;
  tr.fork(10, 10, 9, 50);
  tr.request(10, 100, 0x110003, 0x800);
  tr.create(10, 110, 0x4000, 5, 0x900);
  tr.spawn_id(10, 112, 0x40010005);
  tr.fork(10, 11, 10, 115);
  tr.fork(10, 12, 10, 116);
  tr.entry(11, 120, 0x40010005);
  tr.entry(12, 121, 0x40010099);
  tr.create_ret(10, 130, 0, 0x900);
  tr.request_ret(10, 140, 0x800);
  std::vector<ThreadRecord> th;
  auto own = tr.owners(&th);
  KZ_CHECK(has(*find(th, 11), KZ_THREAD_FLAG_ID_MATCHED));
  KZ_CHECK(!has(*find(th, 11), KZ_THREAD_FLAG_SPAWN_AMBIGUOUS));
  KZ_CHECK(has(*find(th, 12), KZ_THREAD_FLAG_SPAWN_AMBIGUOUS));
  KZ_CHECK_EQ(owner_of(th, own, 11)->rec.level, uint8_t(KZ_OWNER_LEVEL_CONFIRMED));
  KZ_CHECK_EQ(owner_of(th, own, 12)->rec.level, uint8_t(KZ_OWNER_LEVEL_UNDETERMINED));
  // A spawn id outside any spawn call (a thread the framework starts otherwise) is counted, not used.
  Trace other;
  other.spawn_id(10, 100, 0x10001);
  JoinStats st;
  other.owners(nullptr, &st);
  KZ_CHECK_EQ(st.spawn_ids_outside_create, uint64_t(1));
}

void test_request_sites_must_declare_slots() {
  for (int which = 0; which < 3; ++which) {
    Trace tr;
    switch (which) {
      case 0: tr.sites[kSiteRequest].slots.erase(tr.sites[kSiteRequest].slots.begin()); break;  // no owner_id
      case 1: tr.sites[kSiteRequestReturn].slots.pop_back(); break;                            // no return address
      case 2: tr.sites[kSiteSpawnId].slots.erase(tr.sites[kSiteSpawnId].slots.begin()); break;  // no id
    }
    JoinInput in;
    in.events = &tr.events; in.sites = &tr.sites; in.side = &tr.side;
    std::vector<ThreadRecord> out;
    KZ_CHECK_ERR(join_identities(in, &out, nullptr), kz::Err::Invalid);
  }
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
  test_sites_must_declare_decoder_slots();
  test_truncated_slots_are_not_keys();
  test_owner_confirmed_by_id();
  test_owner_inferred_by_time();
  test_owner_undetermined();
  test_owner_of_child_task();
  test_id_resolves_time_ambiguity();
  test_request_sites_must_declare_slots();
  return kztest::finish("test_trace_identity");
}

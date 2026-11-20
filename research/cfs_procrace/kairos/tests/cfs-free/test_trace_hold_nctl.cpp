// The realized control of an F0 hold run as NCTL (kz/trace/hold_nctl.hpp; `kz convert <run.kzt> --hold <hold.kzr>`):
// a small synthetic F0 run container and hold record on the state-order mini profile (support/state_mini.hpp).
// Tool tests: every value is a unit-test value, not a research condition and not a case (CONDITIONS_POLICY).
//
//   - the hold record parses (control points, holds, fires; a record without its header or with an unknown line is
//     refused);
//   - each hold and each release maps to its hit's observation event: the actor's first event after the trap at the
//     point's site (entry or point probe at the binary and file offset), flag observed-after-control; a hold also gives a
//     resume; point indices are gates first, then releases; ctl_seq is the supervisor's order; NRUN control-recorded;
//   - no event is invented: no probe at the address, no event after the trap, an event of the held thread inside its
//     hold, another site first, an unknown thread -- each keeps no event (flag no-observation-event) or is not converted,
//     with the reason as a note;
//   - the converted container keeps every F0 section, names the record in BASIS and FILE_HASHES, and the replay makes the
//     control edge from the releasing actor's last mapped event before the release hit's observation to the held actor's
//     gate observation itself;
//   - the writer's free-text lines (debugregs_error=<message>, child_signal=, a path with a space) parse, and a record
//     with debugregs_ok=no converts with its note;
//   - end to end through the replay: a satisfied and a consumed release make no control edge; a deferred release (the
//     held thread's own hit of a release at its gate's address, acted on when it was resumed) makes the chain's second
//     edge from that thread's last event before its trap to the second held thread's gate observation; a reused thread
//     id maps each fact to the incarnation that was trapped, also one whose first event is its gate observation; two
//     holds of one thread at an `each` gate each get their own observation, resume (with its own releaser) and edge.

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "kz/core/container.hpp"
#include "kz/core/eval_trace.hpp"
#include "kz/core/sha256.hpp"
#include "kz/trace/hold_nctl.hpp"
#include "kz/trace/neutral.hpp"
#include "kz/trace/real_trace.hpp"
#include "support/kz_test.hpp"
#include "support/state_mini.hpp"

namespace {

using kz::Container;

template <class T>
T zero() {
  T v;
  std::memset(&v, 0, sizeof v);
  return v;
}

bool has(const std::string& text, const std::string& fragment) { return text.find(fragment) != std::string::npos; }

// sites at /w/prog: 0 lookup entry (0x1000), 1 lookup return (0x1000), 2 create entry (0x1100), 3 create return (0x1100),
// 4 point create.publish (0x1180), 5 point after_lookup (0x1040, no point of the profile), 6 point lookup.exists (0x1020)
enum { kLookup = 0, kLookupRet = 1, kCreate = 2, kCreateRet = 3, kPublish = 4, kAfterLookup = 5, kExists = 6 };
struct Ev {
  uint64_t t;
  uint32_t tid, site;
};
struct Thr {
  uint32_t tid, generation;
  uint64_t t_fork;
  const char* name;
};
// An F0 run container of these events (in this order, the merged order; tseq per thread id) and thread incarnations.
Container build_run(const std::vector<Ev>& evs, const std::vector<Thr>& thr) {
  kz::ContainerWriter w(KZ_ARTIFACT_RUN);
  w.set_basis_hash(kz::sha256(std::string("unit-test basis")));
  std::vector<kz_site> sites;
  auto site = [&](uint16_t kind, const char* symbol, const char* label, uint64_t off) {
    kz_site s = zero<kz_site>();
    s.site = static_cast<uint32_t>(sites.size());
    s.kind = kind;
    s.decoder = KZ_DECODER_GENERIC;
    s.symbol = w.intern(symbol);
    s.path = w.intern("/w/prog");
    s.label = w.intern(label);
    s.source = w.intern("");
    s.file_offset = off;
    sites.push_back(s);
  };
  site(KZ_SITE_KIND_ENTRY_PROBE, "lookup", "lookup.entry", 0x1000);
  site(KZ_SITE_KIND_RET_PROBE, "lookup", "lookup.ret", 0x1000);
  site(KZ_SITE_KIND_ENTRY_PROBE, "create", "create.entry", 0x1100);
  site(KZ_SITE_KIND_RET_PROBE, "create", "create.ret", 0x1100);
  site(KZ_SITE_KIND_POINT_PROBE, "create", "create.publish", 0x1180);
  site(KZ_SITE_KIND_POINT_PROBE, "consumer", "after_lookup", 0x1040);
  site(KZ_SITE_KIND_POINT_PROBE, "lookup", "lookup.exists", 0x1020);
  std::vector<kz_raw_ev> ev;
  std::vector<uint32_t> tseq(16, 0);
  for (const Ev& x : evs) {
    kz_raw_ev e = zero<kz_raw_ev>();
    e.t_ns = x.t;
    e.tid = x.tid;
    e.tseq = tseq[x.tid - 10]++;
    e.site = x.site;
    e.phase = x.site == kLookup || x.site == kCreate ? KZ_PHASE_ENTRY : x.site == kLookupRet || x.site == kCreateRet ? KZ_PHASE_RET : KZ_PHASE_POINT;
    ev.push_back(e);
  }
  std::vector<kz_thread> threads;
  for (const Thr& x : thr) {
    kz_thread t = zero<kz_thread>();
    t.pid = 10;
    t.tid = x.tid;
    t.generation = x.generation;
    t.t_fork = x.t_fork;
    t.ptid = KZ_NONE32;
    t.creator_event = KZ_NONE32;
    t.first_event = KZ_NONE32;
    t.announced_id = KZ_NONE32;
    t.name = w.intern(x.name);
    threads.push_back(t);
  }
  std::vector<kz_runmeta> meta(1, zero<kz_runmeta>());
  meta[0].backend = KZ_BACKEND_PERF_SAMPLE;
  meta[0].kernel = w.intern("");
  meta[0].harness = w.intern("");
  meta[0].placement = w.intern("");
  std::vector<kz_basis_item> basis(1, zero<kz_basis_item>());
  basis[0].name = w.intern("observe.unit");
  basis[0].value = w.intern("test");
  basis[0].prov = KZ_NONE32;
  basis[0].deviation = KZ_NONE32;
  KZ_CHECK_OK(w.add_section(KZ_SEC_BASIS, basis));
  KZ_CHECK_OK(w.add_section(KZ_SEC_RAW_EVENTS, ev));
  KZ_CHECK_OK(w.add_section(KZ_SEC_SITES, sites));
  KZ_CHECK_OK(w.add_section(KZ_SEC_SITE_SLOTS, std::vector<kz_site_slot>()));
  KZ_CHECK_OK(w.add_section(KZ_SEC_THREADS, threads));
  KZ_CHECK_OK(w.add_section(KZ_SEC_MAPS, std::vector<kz_map_ev>()));
  KZ_CHECK_OK(w.add_section(KZ_SEC_RUNMETA, meta));
  KZ_CHECK_OK(w.add_section(KZ_SEC_LOSS, std::vector<kz_loss>()));
  KZ_CHECK_OK(w.add_section(KZ_SEC_ACTOR_OWNERS, std::vector<kz_app_owner>()));
  kz::Bytes b;
  KZ_CHECK_OK(w.finish(&b));
  Container c;
  KZ_CHECK_OK(Container::parse(b, &c));
  KZ_CHECK_OK(c.verify_content());
  return c;
}

const std::vector<Thr> kThreads{{10, 0, 0, "M"}, {11, 0, 0, "A"}, {12, 0, 0, "B"}};

// threads: 10 M (pid 10), 11 A (the consumer), 12 B (the provider).  `extra_b_at`: an event of B at that time (0: none).
Container make_run(uint64_t extra_b_at = 0) {
  std::vector<Ev> e{{100, 11, kLookup},       // 0 A lookup
                    {110, 11, kExists},       // 1 A lookup.exists
                    {120, 11, kLookupRet},    // 2 A lookup returns
                    {130, 11, kAfterLookup}}; // 3 A after_lookup (the release's first qualifying hit; it acts on the second)
  if (extra_b_at != 0) e.push_back({extra_b_at, 12, kPublish});  // B, inside its hold (a disagreeing record)
  for (const Ev& x : std::vector<Ev>{{200, 11, kLookup},         // A lookup again
                                     {210, 11, kExists},
                                     {220, 11, kLookupRet},      // A's last mapped event before the release
                                     {226, 12, kCreate},         // B's create entry: its gate hit, observed after the resume
                                     {227, 11, kAfterLookup},    // A's after_lookup: the release hit, observed after the release acted
                                     {230, 12, kPublish},        // B create.publish
                                     {240, 12, kCreateRet}})     // B create returns
    e.push_back(x);
  return build_run(e, kThreads);
}

const char* const kRecordHead =
    "# kz hold run record (label: controlled; SPEC 4.13 F0)\n"
    "command: kz observe unit test\n";

std::string record(const std::string& gate_where, uint64_t fire_t, uint32_t fire_tid = 11) {
  return "# kz hold run record (label: controlled; SPEC 4.13 F0)\n"
         "command: kz observe unit test\n"
         "primary_binary=prog\nchild_pid=10\ndebugregs_ok=yes\nwall_ns=1000\nchild_exit=0\n"
         "# control points (gate/release -> binary:file_offset, link-time vaddr; qualifying_hits = the hits that met the\n"
         "# conditions, by any thread; the point acts on the one its occurrence names):\n"
         "gate    G1 " + gate_where + " vaddr=0x1100 name[reg4+0]==\"S0\" caller-in=prog occurrence=first unless-released qualifying_hits=1\n"
         "release R1 prog:0x1040 vaddr=0x1040 releases=G1 occurrence=2 qualifying_hits=2\n"
         "# perturbation (holder cost and the ptrace interaction, SPEC 4.13):\n"
         "total_stops=10 threads_attached=3 clone_attaches=2 relevant_procs=1 gate_traps=1 release_traps=2 armings=3\n"
         "# holds (1):\n"
         "hold gate=G1 tid=12 name=\"S0\" hit=1 held_ns=150 released_by=R1 release_ns=225 hold_duration_ns=75\n"
         "# releases that acted (1; resumed=none: the gate held no thread then -- satisfied=yes: an unless-released gate\n"
         "# that will not hold; consumed=yes: the gate may still hold its hit later):\n"
         "fire release=R1 gate=G1 tid=" + std::to_string(fire_tid) + " hit=2 t_ns=" + std::to_string(fire_t) + " resumed=12\n";
}

void test_parse() {
  kz::trace::HoldRecord h;
  KZ_CHECK_OK(kz::trace::parse_hold_record(record("prog:0x1100", 225), &h));
  KZ_CHECK(h.debugregs_ok && h.points.size() == 2 && h.holds.size() == 1 && h.fires.size() == 1);
  KZ_CHECK(h.points[0].label == "G1" && h.points[0].binary == "prog" && h.points[0].file_offset == 0x1100 && !h.points[0].release);
  KZ_CHECK(h.points[1].release && h.points[1].releases == "G1" && h.points[1].file_offset == 0x1040);
  KZ_CHECK(h.holds[0].tid == 12 && h.holds[0].hit == 1 && h.holds[0].held_ns == 150 && h.holds[0].released && h.holds[0].release_ns == 225);
  KZ_CHECK(h.fires[0].tid == 11 && h.fires[0].hit == 2 && h.fires[0].resumed == 12 && !h.fires[0].satisfied && !h.fires[0].deferred);
  KZ_CHECK(!kz::trace::parse_hold_record("hold gate=G1 tid=1 held_ns=2\n", &h).ok());  // no header
  KZ_CHECK(!kz::trace::parse_hold_record(record("prog:0x1100", 225) + "something else here\n", &h).ok());
  KZ_CHECK_OK(kz::trace::parse_hold_record("# kz hold run record (label: controlled; SPEC 4.13 F0)\nfire release=R tid=1 gate=G hit=3 t_ns=9 resumed=none satisfied=yes deferred=yes\n", &h));
  KZ_CHECK(h.fires.size() == 1 && h.fires[0].satisfied && h.fires[0].deferred && h.fires[0].resumed == 0);
}

void test_mapping() {
  const Container run = make_run();
  kz::trace::NeutralRun n;
  KZ_CHECK_OK(kz::trace::neutral_from_f0(run, &n));
  kz::trace::HoldRecord h;
  KZ_CHECK_OK(kz::trace::parse_hold_record(record("prog:0x1100", 225), &h));
  std::vector<std::string> notes;
  KZ_CHECK_OK(kz::trace::add_hold_control(h, &n, &notes));
  KZ_CHECK((n.run.r.flags & KZ_NRUN_FLAG_CONTROL_RECORDED) != 0);
  KZ_CHECK_EQ(n.ctl.size(), static_cast<size_t>(3));
  // the supervisor's order: hold (150), fire (225), resume (225)
  const kz_nctl& g = n.ctl[0];
  const kz_nctl& f = n.ctl[1];
  const kz_nctl& r = n.ctl[2];
  KZ_CHECK(g.what == KZ_NCTL_WHAT_GATE_HOLD && f.what == KZ_NCTL_WHAT_FIRE && r.what == KZ_NCTL_WHAT_RESUME);
  KZ_CHECK(g.ctl_seq == 1 && f.ctl_seq == 2 && r.ctl_seq == 3);
  KZ_CHECK(g.actor == 2 && g.event == 7 && g.point == 0 && g.gate == 0 && g.hit == 1 && g.site == 2 && (g.cflags & KZ_NCTL_FLAG_OBS_AFTER) != 0);
  KZ_CHECK(f.actor == 1 && f.subject == 1 && f.other == 2 && f.event == 8 && f.point == 1 && f.gate == 0 && f.hit == 2 && f.reason == KZ_NCTL_REASON_RESUMED);
  KZ_CHECK((f.cflags & KZ_NCTL_FLAG_OBS_AFTER) != 0 && f.site == 5);
  KZ_CHECK(r.actor == 2 && r.other == 1 && r.t == 225);
  KZ_CHECK(g.raw_off != 0 && g.raw_off != KZ_NONE64 && g.ctx == KZ_NCTX_UNDECLARED);
  KZ_CHECK(notes.size() == 2 && has(notes[0], "observation #7 create.entry") && has(notes[1], "observation #8 after_lookup"));
}

// No event is invented.
void test_no_observation() {
  auto convert = [](const Container& run, const std::string& text, kz::trace::NeutralRun* n, std::vector<std::string>* notes) {
    KZ_CHECK_OK(kz::trace::neutral_from_f0(run, n));
    kz::trace::HoldRecord h;
    KZ_CHECK_OK(kz::trace::parse_hold_record(text, &h));
    KZ_CHECK_OK(kz::trace::add_hold_control(h, n, notes));
  };
  {  // no probe at the gate's address
    kz::trace::NeutralRun n;
    std::vector<std::string> notes;
    convert(make_run(), record("prog:0x9999", 225), &n, &notes);
    KZ_CHECK(n.ctl[0].event == KZ_NONE32 && (n.ctl[0].cflags & KZ_NCTL_FLAG_NO_OBS) != 0 && n.ctl[0].site == KZ_NONE32);
    KZ_CHECK(has(notes[0], "no entry or point probe at the control point's address"));
  }
  {  // the releasing thread has no event after the trap
    kz::trace::NeutralRun n;
    std::vector<std::string> notes;
    convert(make_run(), record("prog:0x1100", 300), &n, &notes);
    const kz_nctl* fire = nullptr;
    for (const kz_nctl& c : n.ctl)
      if (c.what == KZ_NCTL_WHAT_FIRE) fire = &c;
    KZ_CHECK(fire != nullptr && fire->event == KZ_NONE32 && (fire->cflags & KZ_NCTL_FLAG_NO_OBS) != 0);
    KZ_CHECK(has(notes[1], "no event after the control acted"));
  }
  {  // the release's first event after the trap is not at its site
    kz::trace::NeutralRun n;
    std::vector<std::string> notes;
    convert(make_run(), record("prog:0x1100", 215), &n, &notes);
    KZ_CHECK(has(notes[1], "is at lookup.ret, not at the control point's site"));
  }
  {  // an event of the held thread inside its hold: the record and the hold disagree
    kz::trace::NeutralRun n;
    std::vector<std::string> notes;
    convert(make_run(180), record("prog:0x1100", 225), &n, &notes);
    KZ_CHECK(n.ctl[0].what == KZ_NCTL_WHAT_GATE_HOLD && n.ctl[0].event == KZ_NONE32);
    KZ_CHECK(has(notes[0], "between its hold and its release"));
  }
  {  // a thread id with no actor: not converted
    kz::trace::NeutralRun n;
    std::vector<std::string> notes;
    convert(make_run(), record("prog:0x1100", 225, 77), &n, &notes);
    for (const kz_nctl& c : n.ctl) KZ_CHECK(c.what != KZ_NCTL_WHAT_FIRE);
    KZ_CHECK(has(notes[1], "thread 77 has no actor in the record: not converted"));
  }
}

void test_container_and_replay() {
  const Container run = make_run();
  const std::string text = record("prog:0x1100", 225);
  kz::Bytes bytes;
  kz::trace::NeutralRun layer;
  std::vector<std::string> notes;
  KZ_CHECK_OK(kz::trace::convert_f0_container_with_hold(run, "hold.kzr", text, kz::sha256(std::string("unit-test tool")), &bytes, &layer, &notes));
  Container c;
  KZ_CHECK_OK(Container::parse(bytes, &c));
  KZ_CHECK_OK(c.verify_content());
  KZ_CHECK_OK(c.verify_parent(run));
  // every F0 section is kept (the same records; strings re-interned), BASIS and FILE_HASHES get the record's items
  for (const kz_section_entry& e : run.section_table()) {
    if (e.type == KZ_SEC_STRINGS || e.type == KZ_SEC_BASIS || e.type == KZ_SEC_FILE_HASHES) continue;
    const kz_section_entry* f = c.find_section(e.type);
    KZ_CHECK(f != nullptr && f->count == e.count && f->schema_version == e.schema_version);
  }
  std::vector<kz_raw_ev> ev_a, ev_b;
  KZ_CHECK_OK(run.read_section(KZ_SEC_RAW_EVENTS, &ev_a));
  KZ_CHECK_OK(c.read_section(KZ_SEC_RAW_EVENTS, &ev_b));
  KZ_CHECK(ev_a.size() == ev_b.size() && std::memcmp(ev_a.data(), ev_b.data(), ev_a.size() * sizeof(kz_raw_ev)) == 0);
  std::vector<kz_basis_item> items;
  KZ_CHECK_OK(c.read_section(KZ_SEC_BASIS, &items));
  bool kept = false, sha = false;
  for (const kz_basis_item& b : items) {
    std::string n, v;
    c.string(b.name, &n);
    c.string(b.value, &v);
    kept = kept || (n == "observe.unit" && v == "test");
    sha = sha || (n == "convert.hold.sha256" && v == kz::to_hex(kz::sha256(text)));
  }
  KZ_CHECK(kept && sha);
  std::vector<kz_file_hash> files;
  KZ_CHECK_OK(c.read_section(KZ_SEC_FILE_HASHES, &files));
  KZ_CHECK(files.size() == 1 && files[0].size == text.size());
  // a second conversion is refused (the layer is there)
  kz::Bytes again;
  KZ_CHECK(!kz::trace::convert_f0_container_with_hold(c, "hold.kzr", text, kz::Digest(), &again, nullptr, &notes).ok());
  // the replay's control edge: A's lookup return (its last mapped event before the release hit) -> B's create call
  const kz::phi::Profile p = kzstate::load();
  kz::eval::Trace t;
  kz::trace::RealTraceReport rep;
  KZ_CHECK_OK(kz::trace::real_trace_from_run(c, p, &t, &rep));
  KZ_CHECK_EQ(t.control_edges.size(), static_cast<size_t>(1));
  if (t.control_edges.size() == 1) {
    const kz::eval::TraceEvent& from = t.events[static_cast<size_t>(t.control_edges[0].from)];
    const kz::eval::TraceEvent& to = t.events[static_cast<size_t>(t.control_edges[0].to)];
    KZ_CHECK(from.actor == "A" && from.site == "lookup" && from.phase == kz::eval::Phase::Return);
    KZ_CHECK(to.actor == "B" && to.site == "create" && to.phase == kz::eval::Phase::Call);
    KZ_CHECK_EQ(t.control_edges[0].rule, std::string("control:gate0/release1"));
    // the second lookup's return, not the first one's
    uint64_t returns = 0;
    for (uint64_t i = 0; i < t.control_edges[0].from; ++i) returns += t.events[static_cast<size_t>(i)].actor == "A" && t.events[static_cast<size_t>(i)].phase == kz::eval::Phase::Return;
    KZ_CHECK_EQ(returns, 1u);
  }
  KZ_CHECK(rep.control_records == 3 && rep.control_resumes == 1 && rep.control_edges == 1);
}


// The writer's lines of one key are text to the end of the line (tools/kz/cmd_hold.cpp): a debug-register error message, a
// child's signal, a path with a space.  A record whose control did not run as planned converts with its note.
void test_parse_free_text() {
  const std::string text = std::string(kRecordHead) +
                           "primary_binary=/w/a dir/prog\nchild_pid=10\ndebugregs_ok=no\ndebugregs_error=PTRACE_POKEUSER dr7: Operation not permitted (errno 1)\n"
                           "wall_ns=1000\nchild_signal=9\n"
                           "gate    G1 prog:0x1100 vaddr=0x1100 occurrence=first qualifying_hits=0\n"
                           "release R1 prog:0x1040 vaddr=0x1040 releases=G1 occurrence=2 qualifying_hits=0\n"
                           "total_stops=0 threads_attached=1 clone_attaches=0 relevant_procs=1 gate_traps=0 release_traps=0 armings=0\n";
  kz::trace::HoldRecord h;
  KZ_CHECK_OK(kz::trace::parse_hold_record(text, &h));
  KZ_CHECK(!h.debugregs_ok && h.debugregs_error == "PTRACE_POKEUSER dr7: Operation not permitted (errno 1)" && h.points.size() == 2);
  kz::trace::NeutralRun n;
  KZ_CHECK_OK(kz::trace::neutral_from_f0(make_run(), &n));
  std::vector<std::string> notes;
  KZ_CHECK_OK(kz::trace::add_hold_control(h, &n, &notes));
  KZ_CHECK(n.ctl.empty() && notes.size() == 1 &&
           has(notes[0], "debugregs_ok=no (PTRACE_POKEUSER dr7: Operation not permitted (errno 1)): its control did not run as planned"));
  // an unknown line of one key with a space is still refused
  KZ_CHECK(!kz::trace::parse_hold_record(text + "other_key=a b\n", &h).ok());
}

struct Converted {
  kz::trace::NeutralRun n;
  std::vector<std::string> notes;
  kz::eval::Trace t;
  kz::trace::RealTraceReport rep;
};

// The run converted with the record, written, and read by the replay (as kz convert --hold and kz replay do).
Converted end_to_end(const Container& run, const std::string& text) {
  Converted x;
  kz::Bytes bytes;
  KZ_CHECK_OK(kz::trace::convert_f0_container_with_hold(run, "hold.kzr", text, kz::sha256(std::string("unit-test tool")), &bytes, &x.n, &x.notes));
  Container c;
  KZ_CHECK_OK(Container::parse(bytes, &c));
  const kz::phi::Profile p = kzstate::load();
  KZ_CHECK_OK(kz::trace::real_trace_from_run(c, p, &x.t, &x.rep));
  return x;
}

const kz_nctl* ctl_of(const kz::trace::NeutralRun& n, uint8_t what, int k = 0) {
  for (const kz_nctl& c : n.ctl)
    if (c.what == what && k-- == 0) return &c;
  return nullptr;
}

std::string edge_text(const kz::eval::Trace& t, size_t i) {
  if (i >= t.control_edges.size()) return "none";
  const kz::eval::TraceEvent& f = t.events[static_cast<size_t>(t.control_edges[i].from)];
  const kz::eval::TraceEvent& e = t.events[static_cast<size_t>(t.control_edges[i].to)];
  return f.actor + " " + f.site + " " + kz::eval::phase_text(f.phase) + " -> " + e.actor + " " + e.site + " " + kz::eval::phase_text(e.phase) + " " + t.control_edges[i].rule;
}

// A release that acts while its gate holds no thread: satisfied (an unless-released gate) or consumed; no control edge.
void test_satisfied_consumed() {
  const std::string text = std::string(kRecordHead) + "debugregs_ok=yes\n" +
                           "gate    G1 prog:0x1100 vaddr=0x1100 occurrence=first unless-released qualifying_hits=0\n"
                           "release R1 prog:0x1040 vaddr=0x1040 releases=G1 occurrence=each qualifying_hits=2\n"
                           "fire release=R1 gate=G1 tid=11 hit=1 t_ns=125 resumed=none consumed=yes\n"
                           "fire release=R1 gate=G1 tid=11 hit=2 t_ns=225 resumed=none satisfied=yes\n";
  const Converted x = end_to_end(make_run(), text);
  KZ_CHECK_EQ(x.n.ctl.size(), static_cast<size_t>(2));
  const kz_nctl* consumed = ctl_of(x.n, KZ_NCTL_WHAT_FIRE, 0);
  const kz_nctl* satisfied = ctl_of(x.n, KZ_NCTL_WHAT_FIRE, 1);
  KZ_CHECK(consumed != nullptr && satisfied != nullptr);
  if (consumed == nullptr || satisfied == nullptr) return;
  KZ_CHECK(consumed->reason == KZ_NCTL_REASON_CONSUMED && consumed->event == 3 && consumed->other == KZ_NONE32 && (consumed->cflags & KZ_NCTL_FLAG_OBS_AFTER) != 0);
  KZ_CHECK(satisfied->reason == KZ_NCTL_REASON_SATISFIED && satisfied->event == 8 && satisfied->other == KZ_NONE32);
  KZ_CHECK(ctl_of(x.n, KZ_NCTL_WHAT_GATE_HOLD) == nullptr && ctl_of(x.n, KZ_NCTL_WHAT_RESUME) == nullptr);
  KZ_CHECK(x.t.control_edges.empty());
  KZ_CHECK(x.rep.control_records == 2 && x.rep.control_resumes == 0 && x.rep.control_edges == 0);
}

// A deferred release chain: C is held at G2; B traps at G1, whose address is also the release R2 of G2, so B's hit of R2 is
// deferred; A's R1 resumes B, and B's deferred R2 then resumes C.
void test_deferred_chain() {
  const std::vector<Thr> thr{{10, 0, 0, "M"}, {11, 0, 0, "A"}, {12, 0, 0, "B"}, {13, 0, 0, "C"}};
  const Container run = build_run({{100, 11, kLookup},
                                   {110, 11, kExists},
                                   {120, 11, kLookupRet},
                                   {130, 11, kAfterLookup},
                                   {140, 13, kLookup},      // C's last event before its trap at G2 (lookup.exists)
                                   {147, 12, kLookup},      // B's last event before its trap at G1 (create entry)
                                   {200, 11, kLookup},
                                   {210, 11, kExists},
                                   {220, 11, kLookupRet},   // A's last event before its release hit
                                   {226, 12, kCreate},      // B's gate observation and its (deferred) R2 hit
                                   {227, 11, kAfterLookup}, // A's R1 hit
                                   {230, 12, kPublish},
                                   {240, 12, kCreateRet},
                                   {260, 13, kExists},      // C's gate observation
                                   {270, 13, kLookupRet}},
                                  thr);
  const std::string text = std::string(kRecordHead) + "debugregs_ok=yes\n" +
                           "gate    G1 prog:0x1100 vaddr=0x1100 occurrence=first qualifying_hits=1\n"
                           "gate    G2 prog:0x1020 vaddr=0x1020 occurrence=first qualifying_hits=1\n"
                           "release R1 prog:0x1040 vaddr=0x1040 releases=G1 occurrence=2 qualifying_hits=2\n"
                           "release R2 prog:0x1100 vaddr=0x1100 releases=G2 occurrence=first qualifying_hits=1\n"
                           "hold gate=G2 tid=13 name=\"\" hit=1 held_ns=145 released_by=R2 release_ns=225 hold_duration_ns=80\n"
                           "hold gate=G1 tid=12 name=\"\" hit=1 held_ns=150 released_by=R1 release_ns=225 hold_duration_ns=75\n"
                           "fire release=R1 gate=G1 tid=11 hit=2 t_ns=225 resumed=12\n"
                           "fire release=R2 gate=G2 tid=12 hit=1 t_ns=225 resumed=13 deferred=yes\n";
  const Converted x = end_to_end(run, text);
  KZ_CHECK_EQ(x.n.ctl.size(), static_cast<size_t>(6));
  const kz_nctl* deferred = ctl_of(x.n, KZ_NCTL_WHAT_FIRE, 1);
  KZ_CHECK(deferred != nullptr);
  if (deferred != nullptr) {
    KZ_CHECK((deferred->cflags & KZ_NCTL_FLAG_DEFERRED) != 0 && (deferred->cflags & KZ_NCTL_FLAG_OBS_AFTER) != 0);
    KZ_CHECK(deferred->actor == 2 && deferred->other == 3 && deferred->event == 9 && deferred->point == 3 && deferred->gate == 1);
  }
  const kz_nctl* resume_c = ctl_of(x.n, KZ_NCTL_WHAT_RESUME, 0);
  KZ_CHECK(resume_c != nullptr && resume_c->actor == 3 && resume_c->other == 2);  // C, resumed by B
  KZ_CHECK_EQ(x.t.control_edges.size(), static_cast<size_t>(2));
  // the first edge: A's last event before its hit -> B's gate observation; the second: B's last event before its trap ->
  // C's gate observation
  KZ_CHECK_EQ(edge_text(x.t, 0), std::string("A lookup ret -> B create call control:gate0/release2"));
  KZ_CHECK_EQ(edge_text(x.t, 1), std::string("B lookup call -> C lookup.exists pt control:gate1/release3/deferred"));
  KZ_CHECK(x.rep.control_resumes == 2 && x.rep.control_edges == 2);
}

// A reused thread id: B (tid 12) ran and ended; a later thread with the same id is held at the gate, and its first event
// is its gate observation.  Every fact maps to the later incarnation.
void test_reused_tid() {
  const std::vector<Thr> thr{{10, 0, 0, "M"}, {11, 0, 0, "A"}, {12, 0, 0, "B"}, {12, 1, 140, "B"}};
  const Container run = build_run({{50, 12, kLookup},
                                   {60, 12, kLookupRet},   // the first incarnation's last event
                                   {100, 11, kLookup},
                                   {110, 11, kExists},
                                   {120, 11, kLookupRet},
                                   {130, 11, kAfterLookup},
                                   {200, 11, kLookup},
                                   {210, 11, kExists},
                                   {220, 11, kLookupRet},
                                   {226, 12, kCreate},     // the second incarnation's first event: its gate observation
                                   {227, 11, kAfterLookup},
                                   {230, 12, kPublish},
                                   {240, 12, kCreateRet}},
                                  thr);
  const Converted x = end_to_end(run, record("prog:0x1100", 225));
  KZ_CHECK_EQ(x.n.actors.size(), static_cast<size_t>(4));
  KZ_CHECK(x.n.actors[3].r.native_key == 12 && x.n.actors[3].r.generation == 1);
  const kz_nctl* hold = ctl_of(x.n, KZ_NCTL_WHAT_GATE_HOLD);
  const kz_nctl* fire = ctl_of(x.n, KZ_NCTL_WHAT_FIRE);
  const kz_nctl* resume = ctl_of(x.n, KZ_NCTL_WHAT_RESUME);
  KZ_CHECK(hold != nullptr && fire != nullptr && resume != nullptr);
  if (hold == nullptr || fire == nullptr || resume == nullptr) return;
  KZ_CHECK(hold->actor == 3 && hold->event == 9 && (hold->cflags & KZ_NCTL_FLAG_OBS_AFTER) != 0);
  KZ_CHECK(fire->actor == 1 && fire->other == 3 && resume->actor == 3 && resume->other == 1);
  KZ_CHECK((x.n.actors[2].r.flags & KZ_NACTOR_FLAG_HELD_AT_END) == 0);
  KZ_CHECK_EQ(x.t.control_edges.size(), static_cast<size_t>(1));
  if (x.t.control_edges.size() == 1) {
    const kz::eval::TraceEvent& to = x.t.events[static_cast<size_t>(x.t.control_edges[0].to)];
    KZ_CHECK(to.site == "create" && to.phase == kz::eval::Phase::Call && to.actor != x.t.events[0].actor);  // not the first incarnation
  }
  // an unreleased hold of the later incarnation, before any of its events: the latest incarnation that existed then
  const std::string unreleased = std::string(kRecordHead) + "debugregs_ok=yes\n" +
                                 "gate    G1 prog:0x1100 vaddr=0x1100 occurrence=first qualifying_hits=1\n"
                                 "release R1 prog:0x1040 vaddr=0x1040 releases=G1 occurrence=2 qualifying_hits=0\n"
                                 "hold gate=G1 tid=12 name=\"\" hit=1 held_ns=70 released=no\n";
  const Converted y = end_to_end(run, unreleased);
  const kz_nctl* h2 = ctl_of(y.n, KZ_NCTL_WHAT_GATE_HOLD);
  KZ_CHECK(h2 != nullptr && h2->actor == 2 && (y.n.actors[2].r.flags & KZ_NACTOR_FLAG_HELD_AT_END) != 0);
}

// Two holds of one thread at an `each` gate, released by two threads: each hold its own observation, resume and edge.
void test_each_gate_twice() {
  const Container run = build_run({{100, 11, kLookup},
                                   {120, 11, kLookupRet},
                                   {200, 11, kLookup},
                                   {210, 11, kExists},
                                   {220, 11, kLookupRet},   // A's last event before its release hit
                                   {226, 12, kCreate},      // B's first gate observation
                                   {227, 11, kAfterLookup}, // A's release hit
                                   {230, 12, kPublish},
                                   {240, 12, kCreateRet},
                                   {300, 10, kLookup},
                                   {310, 10, kExists},
                                   {320, 10, kLookupRet},   // M's last event before its release hit
                                   {326, 12, kCreate},      // B's second gate observation
                                   {327, 10, kAfterLookup}, // M's release hit
                                   {330, 12, kPublish},
                                   {340, 12, kCreateRet}},
                                  kThreads);
  const std::string text = std::string(kRecordHead) + "debugregs_ok=yes\n" +
                           "gate    G1 prog:0x1100 vaddr=0x1100 occurrence=each qualifying_hits=2\n"
                           "release R1 prog:0x1040 vaddr=0x1040 releases=G1 occurrence=each qualifying_hits=2\n"
                           "hold gate=G1 tid=12 name=\"\" hit=1 held_ns=150 released_by=R1 release_ns=225 hold_duration_ns=75\n"
                           "hold gate=G1 tid=12 name=\"\" hit=2 held_ns=250 released_by=R1 release_ns=325 hold_duration_ns=75\n"
                           "fire release=R1 gate=G1 tid=11 hit=1 t_ns=225 resumed=12\n"
                           "fire release=R1 gate=G1 tid=10 hit=2 t_ns=325 resumed=12\n";
  const Converted x = end_to_end(run, text);
  KZ_CHECK_EQ(x.n.ctl.size(), static_cast<size_t>(6));
  const kz_nctl* h1 = ctl_of(x.n, KZ_NCTL_WHAT_GATE_HOLD, 0);
  const kz_nctl* h2 = ctl_of(x.n, KZ_NCTL_WHAT_GATE_HOLD, 1);
  const kz_nctl* r1 = ctl_of(x.n, KZ_NCTL_WHAT_RESUME, 0);
  const kz_nctl* r2 = ctl_of(x.n, KZ_NCTL_WHAT_RESUME, 1);
  KZ_CHECK(h1 != nullptr && h2 != nullptr && r1 != nullptr && r2 != nullptr);
  if (h1 == nullptr || h2 == nullptr || r1 == nullptr || r2 == nullptr) return;
  KZ_CHECK(h1->hit == 1 && h1->event == 5 && h2->hit == 2 && h2->event == 12);
  KZ_CHECK(r1->other == 1 && r2->other == 0);  // A resumed the first hold, M the second
  KZ_CHECK(h1->ctl_seq < r1->ctl_seq && r1->ctl_seq < h2->ctl_seq && h2->ctl_seq < r2->ctl_seq);
  KZ_CHECK_EQ(x.t.control_edges.size(), static_cast<size_t>(2));
  KZ_CHECK_EQ(edge_text(x.t, 0), std::string("A lookup ret -> B create call control:gate0/release1"));
  KZ_CHECK_EQ(edge_text(x.t, 1), std::string("M lookup ret -> B create call control:gate0/release1"));
  if (x.t.control_edges.size() == 2) KZ_CHECK(x.t.control_edges[0].to < x.t.control_edges[1].to);  // B's first and second create calls
}

}  // namespace

int main() {
  test_parse();
  test_mapping();
  test_no_observation();
  test_container_and_replay();
  test_parse_free_text();
  test_satisfied_consumed();
  test_deferred_chain();
  test_reused_tid();
  test_each_gate_twice();
  return kztest::finish("test_trace_hold_nctl");
}

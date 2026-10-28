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
//     gate observation itself.

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
// threads: 10 M (pid 10), 11 A (the consumer), 12 B (the provider).  `extra_b_at`: an event of B at that time (0: none).
Container make_run(uint64_t extra_b_at = 0) {
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
  auto add = [&](uint64_t t, uint32_t tid, uint32_t s, uint16_t phase) {
    kz_raw_ev e = zero<kz_raw_ev>();
    e.t_ns = t;
    e.tid = tid;
    e.tseq = tseq[tid - 10]++;
    e.site = s;
    e.phase = phase;
    ev.push_back(e);
  };
  add(100, 11, 0, KZ_PHASE_ENTRY);  // 0 A lookup
  add(110, 11, 6, KZ_PHASE_POINT);  // 1 A lookup.exists
  add(120, 11, 1, KZ_PHASE_RET);    // 2 A lookup returns
  add(130, 11, 5, KZ_PHASE_POINT);  // 3 A after_lookup (the release's first qualifying hit; it acts on the second)
  if (extra_b_at != 0) add(extra_b_at, 12, 4, KZ_PHASE_POINT);  // B, inside its hold (a disagreeing record)
  add(200, 11, 0, KZ_PHASE_ENTRY);  // A lookup again
  add(210, 11, 6, KZ_PHASE_POINT);
  add(220, 11, 1, KZ_PHASE_RET);    // A's last mapped event before the release
  add(226, 12, 2, KZ_PHASE_ENTRY);  // B's create entry: its gate hit, observed after the resume
  add(227, 11, 5, KZ_PHASE_POINT);  // A's after_lookup: the release hit, observed after the release acted
  add(230, 12, 4, KZ_PHASE_POINT);  // B create.publish
  add(240, 12, 3, KZ_PHASE_RET);    // B create returns
  std::vector<kz_thread> threads;
  for (uint32_t tid : {10u, 11u, 12u}) {
    kz_thread t = zero<kz_thread>();
    t.pid = 10;
    t.tid = tid;
    t.ptid = KZ_NONE32;
    t.creator_event = KZ_NONE32;
    t.first_event = KZ_NONE32;
    t.announced_id = KZ_NONE32;
    t.name = w.intern(tid == 10 ? "M" : tid == 11 ? "A" : "B");
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

}  // namespace

int main() {
  test_parse();
  test_mapping();
  test_no_observation();
  test_container_and_replay();
  return kztest::finish("test_trace_hold_nctl");
}

// The neutral event layer (schema revision 3; kz/trace/neutral.hpp, kz/core/record_codec.hpp, docs/schema.md section 12).
// Everything here is a unit-test value, not a research condition (CONDITIONS_POLICY).
//
//   codec        every registered record: the explicit little-endian encoding equals the struct bytes on this
//                (little-endian) host and decodes back; a neutral event's stored bytes, field by field, as hand-written
//                little-endian bytes;
//   F0 -> neutral  a synthetic F0 run container (mini profile): actors are thread incarnations with their generation
//                (a tid reused after a fork), a tid without a THREADS record is an actor of its own, the creating
//                executor, the initial thread, the call instance link, the creating call's return bound by the id,
//                observed values only (a string slot without its string has no value, a TRUNC event none), the
//                loss intervals of a throttle and of a lost-records report;
//   round trip   the converted container keeps every F0 section byte for byte, parses, verifies, reads back the same
//                layer, refuses a second conversion, and is dumped by the canonical dump;
//   equivalence  the replay's trace and report of the original container (in-memory conversion) and of the converted
//                one (neutral sections) are the same; with arguments <profile dir> <run.kzt> the same is checked on a
//                real record (exit 77 if it is not present).

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#include "kz/core/container.hpp"
#include "kz/core/dump.hpp"
#include "kz/core/eval_trace.hpp"
#include "kz/core/phi.hpp"
#include "kz/core/record_codec.hpp"
#include "kz/core/registry.hpp"
#include "kz/core/sha256.hpp"
#include "kz/trace/neutral.hpp"
#include "kz/trace/real_trace.hpp"
#include "support/kz_test.hpp"
#include "support/phi_mini.hpp"

namespace {

using kz::Container;

template <class T>
T zero() {
  T v;
  std::memset(&v, 0, sizeof v);
  return v;
}

// ---- codec ----------------------------------------------------------------------------------------------
void test_codec() {
  uint32_t seed = 12345;
  auto next = [&]() {
    seed = seed * 1103515245u + 12345u;
    return static_cast<uint8_t>(seed >> 16);
  };
  for (const kz_record_desc* rec : kz::records()) {
    std::vector<uint8_t> host(rec->size * 3), stored(rec->size * 3), back(rec->size * 3);
    for (uint8_t& b : host) b = next();
    kz::encode_records(*rec, host.data(), 3, stored.data());
    KZ_CHECK(host == stored);  // little-endian host, no padding: the stored form is the struct's bytes
    kz::decode_records(*rec, stored.data(), 3, back.data());
    KZ_CHECK(back == host);
  }
  kz_nevent e = zero<kz_nevent>();
  e.t = 0x0102030405060708ull;
  e.actor = 0x11223344u;
  e.aseq = 7;
  e.site = 0xAABBCCDDu;
  e.bind = 0x00010002u;
  e.link = KZ_NONE32;
  e.first_value = 0x01000000u;
  e.nvals = 0x0203;
  e.flags = KZ_NEV_FLAG_CLOCK | KZ_NEV_FLAG_ASEQ_TARGET;
  e.raw_off = 0x1122334455667788ull;
  e.raw.section = KZ_SEC_RAW_EVENTS;
  e.raw.index = 0x80000001u;
  e.raw.artifact_sha256[0] = 0x5A;
  uint8_t stored[sizeof(kz_nevent)];
  kz::encode_records(*kz::find_record("kz_nevent"), &e, 1, stored);
  const uint8_t expect[] = {
      0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01,  // t
      0x44, 0x33, 0x22, 0x11, 0x07, 0x00, 0x00, 0x00,  // actor, aseq
      0xDD, 0xCC, 0xBB, 0xAA, 0x02, 0x00, 0x01, 0x00,  // site, bind
      0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x01,  // link, first_value
      0x03, 0x02, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00,  // nvals, flags, rsv0
      0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11,  // raw_off
  };
  KZ_CHECK(std::memcmp(stored, expect, sizeof expect) == 0);
  KZ_CHECK_EQ(stored[48], 0x5A);                                                 // raw.artifact_sha256[0]
  KZ_CHECK(stored[80] == 41 && stored[81] == 0 && stored[84] == 1 && stored[87] == 0x80);  // raw.section, raw.index
  KZ_CHECK_EQ(sizeof(kz_nevent), static_cast<size_t>(88));
  KZ_CHECK(kz::section_is_serialized(KZ_SEC_NEVENTS) && !kz::section_is_serialized(KZ_SEC_RAW_EVENTS));
}

// ---- a synthetic F0 run container on the mini profile ---------------------------------------------------
//   sites: 0 ENTRY send, 1 RET send, 2 POINT send.who_valid (string slot "name"), 3 POINT send.who_reg,
//          4 ENTRY spawn_it (spawn-create), 5 RET spawn_it (spawn-return)
//   threads: 0 X tid 10 = pid (initial), 1 W tid 11 generation 0, 2 W tid 11 generation 1 (forked by X at 500,
//            created by the spawn_it call, its id matched); events of tid 99 have no THREADS record
Container make_run() {
  kz::ContainerWriter w(KZ_ARTIFACT_RUN);
  w.set_basis_hash(kz::sha256(std::string("unit-test basis")));
  KZ_CHECK_OK(w.add_parent(KZ_ARTIFACT_BASIS, 0, kz::sha256(std::string("unit-test basis"))));
  std::vector<kz_site> sites;
  auto site = [&](uint16_t kind, uint16_t decoder, const char* symbol, const char* label) {
    kz_site s = zero<kz_site>();
    s.site = static_cast<uint32_t>(sites.size());
    s.kind = kind;
    s.decoder = decoder;
    s.symbol = w.intern(symbol);
    s.path = w.intern("/w/prog");
    s.label = w.intern(label);
    s.source = w.intern(kind == KZ_SITE_KIND_POINT_PROBE ? "src/send.c" : "");
    s.line = kind == KZ_SITE_KIND_POINT_PROBE ? 40 + s.site : 0;
    s.file_offset = 0x1000 + 0x10 * s.site;
    sites.push_back(s);
  };
  site(KZ_SITE_KIND_ENTRY_PROBE, KZ_DECODER_GENERIC, "send", "send.entry");
  site(KZ_SITE_KIND_RET_PROBE, KZ_DECODER_GENERIC, "send", "send.ret");
  site(KZ_SITE_KIND_POINT_PROBE, KZ_DECODER_GENERIC, "send", "send.who_valid");
  site(KZ_SITE_KIND_POINT_PROBE, KZ_DECODER_GENERIC, "send", "send.who_reg");
  site(KZ_SITE_KIND_ENTRY_PROBE, KZ_DECODER_SPAWN_CREATE, "spawn_it", "spawn.entry");
  site(KZ_SITE_KIND_RET_PROBE, KZ_DECODER_SPAWN_RETURN, "spawn_it", "spawn.ret");
  std::vector<kz_site_slot> slots;
  auto slot = [&](uint32_t s, uint8_t k, const char* role, uint8_t src) {
    kz_site_slot x = zero<kz_site_slot>();
    x.site = s;
    x.slot = k;
    x.src = src;
    x.width = 8;
    x.role = w.intern(role);
    slots.push_back(x);
  };
  slot(0, 1, "who", KZ_SLOT_SRC_REG);
  slot(0, 3, "return_address", KZ_SLOT_SRC_STACK);
  slot(1, 0, "status", KZ_SLOT_SRC_REG);
  slot(1, 3, "return_address", KZ_SLOT_SRC_IP);
  slot(2, 0, "who", KZ_SLOT_SRC_REG);
  slot(2, 1, "name", KZ_SLOT_SRC_STR);
  slot(4, 1, "entry_fn", KZ_SLOT_SRC_REG);
  slot(4, 3, "return_address", KZ_SLOT_SRC_STACK);
  slot(5, 0, "status", KZ_SLOT_SRC_REG);
  slot(5, 3, "return_address", KZ_SLOT_SRC_IP);
  std::vector<kz_raw_ev> ev;
  auto add = [&](uint64_t t, uint32_t tid, uint32_t s, uint16_t phase, uint64_t a0, uint64_t a1, uint64_t a3, uint16_t flags = 0) {
    kz_raw_ev e = zero<kz_raw_ev>();
    e.t_ns = t;
    e.tid = tid;
    e.tseq = static_cast<uint32_t>(ev.size());
    e.site = s;
    e.phase = phase;
    e.flags = flags;
    e.a[0] = a0;
    e.a[1] = a1;
    e.a[3] = a3;
    ev.push_back(e);
  };
  add(100, 11, 2, KZ_PHASE_POINT, 0x1002, 4, 0);              // 0 W.g0 capture, string read (EVENT_STRINGS)
  add(110, 11, 0, KZ_PHASE_ENTRY, 0, 0x1002, 0x5000);         // 1 W.g0 send(Who)
  add(120, 11, 1, KZ_PHASE_RET, 0, 0, 0x5000);                // 2 W.g0 OK, linked to 1
  add(400, 10, 4, KZ_PHASE_ENTRY, 0, 0x7777, 0x6000);         // 3 X spawn_it
  add(600, 10, 5, KZ_PHASE_RET, 0, 0, 0x6000);                // 4 X spawn_it returns: bound to W.g1 by the id
  add(700, 11, 2, KZ_PHASE_POINT, 0x1003, 4, 0);              // 5 W.g1 capture, the kernel gave no string
  add(710, 11, 0, KZ_PHASE_ENTRY, 0, 0x1003, 0x5100, KZ_RAW_FLAG_TRUNC);  // 6 W.g1 send, no slot values
  add(720, 99, 3, KZ_PHASE_POINT, 0, 0, 0);                   // 7 tid 99 (no THREADS record)
  add(730, 11, 7, KZ_PHASE_POINT, 0, 0, 0);                   // 8 a site the table does not have
  std::vector<kz_ev_str> strs(1, zero<kz_ev_str>());
  strs[0].event = 0;
  strs[0].slot = 1;
  strs[0].text = w.intern("APP_W");
  std::vector<kz_thread> threads;
  auto thread = [&](uint32_t tid, uint32_t gen, const char* name, uint64_t t_fork, uint32_t ptid, uint32_t flags, uint32_t creator_event, uint32_t announced) {
    kz_thread t = zero<kz_thread>();
    t.pid = 10;
    t.tid = tid;
    t.generation = gen;
    t.ptid = ptid;
    t.flags = flags;
    t.creator_event = creator_event;
    t.first_event = KZ_NONE32;
    t.announced_id = announced;
    t.t_fork = t_fork;
    t.name = w.intern(name);
    threads.push_back(t);
  };
  thread(10, 0, "X", 0, KZ_NONE32, 0, KZ_NONE32, KZ_NONE32);
  thread(11, 0, "W", 0, KZ_NONE32, KZ_THREAD_FLAG_EXIT_SEEN, KZ_NONE32, KZ_NONE32);
  thread(11, 1, "W", 500, 10, KZ_THREAD_FLAG_FORK_SEEN | KZ_THREAD_FLAG_SPAWN_MATCHED | KZ_THREAD_FLAG_ID_MATCHED | KZ_THREAD_FLAG_TID_REUSED, 3, 0x1003);
  std::vector<kz_map_ev> maps(1, zero<kz_map_ev>());
  maps[0].t_ns = 50;
  maps[0].pid = 10;
  maps[0].path = w.intern("/w/prog");
  std::vector<kz_runmeta> meta(1, zero<kz_runmeta>());
  meta[0].backend = KZ_BACKEND_PERF_SAMPLE;
  meta[0].flags = KZ_RUN_FLAG_LOSS_PRESENT;
  meta[0].kernel = w.intern("");
  meta[0].harness = w.intern("");
  meta[0].placement = w.intern("");
  std::vector<kz_loss> losses(2, zero<kz_loss>());
  losses[0].source = KZ_LOSS_SOURCE_PERF_THROTTLE;
  losses[0].cpu = 1;
  losses[0].t_ns = 300;
  losses[0].count = 350;  // 300 .. 650
  losses[1].source = KZ_LOSS_SOURCE_PERF_RECORD_LOST;
  losses[1].cpu = 2;
  losses[1].count = 3;
  losses[1].t_ns = 705;
  std::vector<kz_app_owner> owners(1, zero<kz_app_owner>());
  owners[0].thread = 2;
  owners[0].creator_thread = 0;
  owners[0].creation_event = 3;
  owners[0].owner_key = 0x1003;
  owners[0].level = KZ_OWNER_LEVEL_CONFIRMED;
  owners[0].evidence = KZ_OWNER_EVIDENCE_CREATION_REQUEST | KZ_OWNER_EVIDENCE_ANNOUNCED_ID;
  owners[0].app_name = w.intern("APP_W");
  KZ_CHECK_OK(w.add_section(KZ_SEC_RAW_EVENTS, ev));
  KZ_CHECK_OK(w.add_section(KZ_SEC_SITES, sites));
  KZ_CHECK_OK(w.add_section(KZ_SEC_SITE_SLOTS, slots));
  KZ_CHECK_OK(w.add_section(KZ_SEC_THREADS, threads));
  KZ_CHECK_OK(w.add_section(KZ_SEC_MAPS, maps));
  KZ_CHECK_OK(w.add_section(KZ_SEC_RUNMETA, meta));
  KZ_CHECK_OK(w.add_section(KZ_SEC_LOSS, losses));
  KZ_CHECK_OK(w.add_section(KZ_SEC_ACTOR_OWNERS, owners));
  KZ_CHECK_OK(w.add_section(KZ_SEC_EVENT_STRINGS, strs));
  kz::Bytes b;
  KZ_CHECK_OK(w.finish(&b));
  Container c;
  KZ_CHECK_OK(Container::parse(b, &c));
  KZ_CHECK_OK(c.verify_content());
  return c;
}

kz::phi::Profile load() {
  kz::phi::Profile p;
  std::vector<kz::phi::Diag> diags;
  KZ_CHECK_OK(kz::phi::load_profile(kzmini::files(), &p, &diags));
  return p;
}

void test_f0_conversion(const Container& run) {
  kz::trace::NeutralRun n;
  KZ_CHECK_OK(kz::trace::neutral_from_f0(run, &n));
  KZ_CHECK_EQ(n.run.r.backend, KZ_NBACKEND_F0);
  KZ_CHECK_EQ(n.run.backend_name, std::string("perf-sample"));
  KZ_CHECK_EQ(n.run.r.start_evidence, KZ_NSTART_EXEC_MAPPING);
  KZ_CHECK_EQ(n.run.r.flags, static_cast<uint32_t>(KZ_NRUN_FLAG_LOSS_EVIDENCE | KZ_NRUN_FLAG_LOSS_PRESENT | KZ_NRUN_FLAG_OWNERS_RECORDED | KZ_NRUN_FLAG_COMPLETE));
  KZ_CHECK_EQ(n.sites.size(), static_cast<size_t>(6));
  KZ_CHECK_EQ(n.sites[0].r.kind, KZ_NSITE_KIND_CALL);
  KZ_CHECK_EQ(n.sites[1].r.kind, KZ_NSITE_KIND_RETURN);
  KZ_CHECK_EQ(n.sites[2].r.kind, KZ_NSITE_KIND_POINT);
  KZ_CHECK_EQ(n.sites[2].source, std::string("src/send.c"));
  KZ_CHECK_EQ(n.sites[2].r.raw.section, static_cast<uint32_t>(KZ_SEC_SITES));
  KZ_CHECK_EQ(n.sites[0].at, std::string("/w/prog+0x1000"));
  // actors: the three incarnations in THREADS order, then tid 99
  KZ_CHECK_EQ(n.actors.size(), static_cast<size_t>(4));
  KZ_CHECK_EQ(n.actors[2].r.generation, 1u);
  KZ_CHECK_EQ(n.actors[2].r.binding, KZ_NBIND_LINUX_TID);
  KZ_CHECK_EQ(n.actors[2].r.native_key, 11u);
  KZ_CHECK_EQ(n.actors[2].r.creator, 0u);
  KZ_CHECK_EQ(n.actors[3].r.native_key, 99u);
  KZ_CHECK((n.actors[3].r.flags & KZ_NACTOR_FLAG_JOIN_RECORD) == 0);
  KZ_CHECK((n.actors[0].r.flags & KZ_NACTOR_FLAG_INITIAL) != 0);
  KZ_CHECK((n.actors[1].r.flags & KZ_NACTOR_FLAG_INITIAL) == 0);
  KZ_CHECK((n.actors[2].r.flags & KZ_NACTOR_FLAG_OWNER_RECORDED) != 0);
  KZ_CHECK_EQ(n.actors[2].r.owner_id, 0x1003u);
  KZ_CHECK_EQ(n.actors[2].owner_app, std::string("APP_W"));
  KZ_CHECK_EQ(n.actors[2].r.spawn_return, 4u);
  KZ_CHECK_EQ(n.actors[1].r.spawn_return, KZ_NONE32);
  const std::vector<std::string> names = kz::trace::actor_names(n);
  KZ_CHECK_EQ(names[0], std::string("X"));
  KZ_CHECK_EQ(names[1], std::string("W.tid11"));
  KZ_CHECK_EQ(names[2], std::string("W.tid11.g1"));
  KZ_CHECK_EQ(names[3], std::string("tid99"));
  // events: the incarnation alive at each, the link, the values
  KZ_CHECK_EQ(n.events.size(), static_cast<size_t>(9));
  KZ_CHECK_EQ(n.events[0].actor, 1u);
  KZ_CHECK_EQ(n.events[5].actor, 2u);
  KZ_CHECK_EQ(n.events[7].actor, 3u);
  KZ_CHECK_EQ(n.events[2].link, 1u);
  KZ_CHECK_EQ(n.events[4].link, 3u);
  KZ_CHECK_EQ(n.events[8].site, KZ_NONE32);
  KZ_CHECK_EQ(n.events[0].nvals, 2);  // who and the string
  KZ_CHECK_EQ(n.values[n.events[0].first_value + 1].r.kind, KZ_NVAL_TEXT);
  KZ_CHECK_EQ(n.values[n.events[0].first_value + 1].text, std::string("APP_W"));
  KZ_CHECK_EQ(n.events[5].nvals, 1);  // who only: the string was not read, so it is not observed
  KZ_CHECK_EQ(n.events[6].nvals, 0);  // TRUNC
  KZ_CHECK((n.events[6].flags & KZ_NEV_FLAG_NO_VALUES) != 0);
  KZ_CHECK((n.events[0].flags & KZ_NEV_FLAG_ASEQ_TARGET) == 0);  // F0's tseq is the decoder's
  // losses: the throttle 300..650 covers events 3..4, the lost report up to 705 covers 0..5
  KZ_CHECK_EQ(n.losses.size(), static_cast<size_t>(2));
  KZ_CHECK_EQ(n.losses[0].kind, KZ_NLOSS_THROTTLE);
  KZ_CHECK_EQ(n.losses[0].begin, 3u);
  KZ_CHECK_EQ(n.losses[0].end, 5u);
  KZ_CHECK_EQ(n.losses[1].kind, KZ_NLOSS_RECORDS_LOST);
  KZ_CHECK_EQ(n.losses[1].begin, 0u);
  KZ_CHECK_EQ(n.losses[1].end, 6u);
  KZ_CHECK_EQ(n.losses[1].unit, 2u);
  KZ_CHECK_OK(kz::trace::check_neutral(n));
  // a broken layer is refused
  kz::trace::NeutralRun bad = n;
  bad.events[3].actor = 9;
  KZ_CHECK(!kz::trace::check_neutral(bad).ok());
  bad = n;
  bad.events[1].link = 2;
  KZ_CHECK(!kz::trace::check_neutral(bad).ok());
}

// A record with its kz_str fields zeroed (they mean nothing in memory), for comparison.
template <class T>
std::vector<uint8_t> bytes_of(const T& r) {
  T c = r;
  const kz_record_desc* rec = kz::find_record(kz::RecordName<T>::value);
  for (uint32_t f = 0; f < rec->nfields; ++f)
    if (rec->fields[f].kind == KZ_FK_STR) std::memset(reinterpret_cast<uint8_t*>(&c) + rec->fields[f].offset, 0, rec->fields[f].elem_size * rec->fields[f].count);
  const uint8_t* p = reinterpret_cast<const uint8_t*>(&c);
  return std::vector<uint8_t>(p, p + sizeof c);
}

void check_same_layer(const kz::trace::NeutralRun& a, const kz::trace::NeutralRun& b) {
  KZ_CHECK(bytes_of(a.run.r) == bytes_of(b.run.r));
  KZ_CHECK(a.run.backend_name == b.run.backend_name && a.run.runtime == b.run.runtime && a.run.adapter == b.run.adapter && a.run.compiler == b.run.compiler &&
           a.run.target == b.run.target);
  KZ_CHECK_EQ(a.sites.size(), b.sites.size());
  for (size_t i = 0; i < a.sites.size() && i < b.sites.size(); ++i)
    KZ_CHECK(bytes_of(a.sites[i].r) == bytes_of(b.sites[i].r) && a.sites[i].label == b.sites[i].label && a.sites[i].op == b.sites[i].op &&
             a.sites[i].symbol == b.sites[i].symbol && a.sites[i].source == b.sites[i].source && a.sites[i].at == b.sites[i].at);
  KZ_CHECK_EQ(a.slots.size(), b.slots.size());
  for (size_t i = 0; i < a.slots.size() && i < b.slots.size(); ++i)
    KZ_CHECK(bytes_of(a.slots[i].r) == bytes_of(b.slots[i].r) && a.slots[i].role == b.slots[i].role && a.slots[i].from == b.slots[i].from);
  KZ_CHECK_EQ(a.events.size(), b.events.size());
  for (size_t i = 0; i < a.events.size() && i < b.events.size(); ++i) KZ_CHECK(bytes_of(a.events[i]) == bytes_of(b.events[i]));
  KZ_CHECK_EQ(a.values.size(), b.values.size());
  for (size_t i = 0; i < a.values.size() && i < b.values.size(); ++i) KZ_CHECK(bytes_of(a.values[i].r) == bytes_of(b.values[i].r) && a.values[i].text == b.values[i].text);
  KZ_CHECK_EQ(a.actors.size(), b.actors.size());
  for (size_t i = 0; i < a.actors.size() && i < b.actors.size(); ++i)
    KZ_CHECK(bytes_of(a.actors[i].r) == bytes_of(b.actors[i].r) && a.actors[i].name == b.actors[i].name && a.actors[i].owner_app == b.actors[i].owner_app &&
             a.actors[i].request_func == b.actors[i].request_func);
  KZ_CHECK_EQ(a.losses.size(), b.losses.size());
  for (size_t i = 0; i < a.losses.size() && i < b.losses.size(); ++i) KZ_CHECK(bytes_of(a.losses[i]) == bytes_of(b.losses[i]));
  KZ_CHECK_EQ(a.ctl.size(), b.ctl.size());
  for (size_t i = 0; i < a.ctl.size() && i < b.ctl.size(); ++i) KZ_CHECK(bytes_of(a.ctl[i]) == bytes_of(b.ctl[i]));
}

// The converted container: F0 sections unchanged, the layer read back equal, a second conversion refused.
void test_round_trip(const Container& run, Container* converted) {
  kz::Bytes bytes;
  kz::trace::NeutralRun layer;
  KZ_CHECK_OK(kz::trace::convert_f0_container(run, kz::sha256(std::string("unit-test tool")), &bytes, &layer));
  Container c;
  KZ_CHECK_OK(Container::parse(bytes, &c));
  KZ_CHECK_OK(c.verify_content());
  KZ_CHECK(kz::trace::has_neutral_sections(c) && !kz::trace::has_neutral_sections(run));
  KZ_CHECK_OK(c.verify_parent(run));
  KZ_CHECK(c.basis_hash() == run.basis_hash());
  // every F0 section: the same records (strings compared as text)
  for (const kz_section_entry& e : run.section_table()) {
    if (e.type == KZ_SEC_STRINGS) continue;
    const kz_section_entry* f = c.find_section(e.type);
    KZ_CHECK(f != nullptr && f->count == e.count && f->schema_version == e.schema_version);
  }
  KZ_CHECK_EQ(kz::dump_container(run).find("NEVENTS"), std::string::npos);
  std::vector<kz_raw_ev> ev_a, ev_b;
  KZ_CHECK_OK(run.read_section(KZ_SEC_RAW_EVENTS, &ev_a));
  KZ_CHECK_OK(c.read_section(KZ_SEC_RAW_EVENTS, &ev_b));
  KZ_CHECK(ev_a.size() == ev_b.size() && std::memcmp(ev_a.data(), ev_b.data(), ev_a.size() * sizeof(kz_raw_ev)) == 0);
  kz::trace::NeutralRun back;
  KZ_CHECK_OK(kz::trace::read_neutral(c, &back));
  check_same_layer(layer, back);
  bool from_sections = false;
  KZ_CHECK_OK(kz::trace::neutral_of_run(c, &back, &from_sections));
  KZ_CHECK(from_sections);
  kz::Bytes again;
  KZ_CHECK(!kz::trace::convert_f0_container(c, kz::sha256(std::string("unit-test tool")), &again, nullptr).ok());
  // the same input gives the same bytes
  kz::Bytes twice;
  KZ_CHECK_OK(kz::trace::convert_f0_container(run, kz::sha256(std::string("unit-test tool")), &twice, nullptr));
  KZ_CHECK(twice == bytes);
  // the canonical dump shows the layer
  const std::string d = kz::dump_container(c);
  KZ_CHECK(d.find("section NEVENTS") != std::string::npos);
  KZ_CHECK(d.find("binding=linux-tid") != std::string::npos);
  *converted = c;
}

void check_same_replay(const Container& original, const Container& converted, const kz::phi::Profile& p, const kz::trace::RealTraceOptions& opt) {
  kz::eval::Trace ta, tb;
  kz::trace::RealTraceReport ra, rb;
  KZ_CHECK_OK(kz::trace::real_trace_from_run(original, p, opt, &ta, &ra));
  KZ_CHECK_OK(kz::trace::real_trace_from_run(converted, p, opt, &tb, &rb));
  KZ_CHECK(!ra.neutral_sections && rb.neutral_sections);
  KZ_CHECK(kz::eval::trace_text(ta) == kz::eval::trace_text(tb));
  KZ_CHECK(kz::trace::report_text(ra) == kz::trace::report_text(rb));
  KZ_CHECK_EQ(ta.events.size(), tb.events.size());
  for (const auto& kv : opt.consts) ta.ref_consts[kv.first] = tb.ref_consts[kv.first] = kv.second;  // as kz replay does
  kz::eval::Result xa, xb;
  KZ_CHECK_OK(kz::eval::replay(p, ta, kz::eval::Options(), &xa));
  KZ_CHECK_OK(kz::eval::replay(p, tb, kz::eval::Options(), &xb));
  KZ_CHECK(kz::eval::dump_result(p, ta, xa) == kz::eval::dump_result(p, tb, xb));
}

void test_equivalence(const Container& original, const Container& converted) {
  const kz::phi::Profile p = load();
  kz::trace::RealTraceOptions opt;
  opt.consts["TABLE"] = 4;
  check_same_replay(original, converted, p, opt);
  kz::eval::Trace t;
  kz::trace::RealTraceReport r;
  KZ_CHECK_OK(kz::trace::real_trace_from_run(converted, p, opt, &t, &r));
  // the spawn_it return carries the child's id, bound by the identity join
  bool bound = false;
  for (const kz::eval::TraceEvent& e : t.events)
    if (e.site == "spawn_it" && e.phase == kz::eval::Phase::Return && e.values.count("out.Handle") != 0 && e.values.at("out.Handle") == 0x1003) bound = true;
  KZ_CHECK(bound);
  KZ_CHECK_EQ(r.spawn_outs_bound, 1u);
  KZ_CHECK_EQ(r.threads, 3u);  // the actors of the identity records; tid 99 is an actor of the trace, not a thread record
  KZ_CHECK_EQ(r.identities.size(), static_cast<size_t>(3));
  KZ_CHECK_EQ(r.identities[2].actor_index, 2u);
  KZ_CHECK_EQ(r.identities[2].binding, KZ_NBIND_LINUX_TID);
  KZ_CHECK_EQ(r.identities[2].generation, 1u);
  KZ_CHECK_EQ(t.creators.at("W.tid11.g1"), std::string("X"));
  KZ_CHECK_EQ(r.string_values, 1u);
  KZ_CHECK_EQ(r.unreadable_events, 1u);
  KZ_CHECK_EQ(r.loss_intervals, 1u);  // 0..6 and 3..5 merge
}

int real_record(const std::string& profile_dir, const std::string& path) {
  std::error_code ec;
  if (!std::filesystem::exists(path, ec)) {
    std::printf("%s not present: skipped\n", path.c_str());
    return 77;
  }
  std::vector<kz::phi::SourceText> files;
  std::vector<std::string> paths;
  for (const auto& e : std::filesystem::directory_iterator(profile_dir, ec))
    if (e.is_regular_file() && e.path().extension() == ".kzp") paths.push_back(e.path().string());
  std::sort(paths.begin(), paths.end());
  for (const std::string& f : paths) {
    kz::Bytes b;
    KZ_CHECK_OK(kz::read_whole_file(f, &b));
    files.push_back(kz::phi::SourceText{f, std::string(b.begin(), b.end())});
  }
  kz::phi::Profile p;
  std::vector<kz::phi::Diag> diags;
  KZ_CHECK_OK(kz::phi::load_profile(files, &p, &diags));
  Container run;
  KZ_CHECK_OK(Container::read_file(path, &run));
  kz::Bytes bytes;
  KZ_CHECK_OK(kz::trace::convert_f0_container(run, kz::sha256(std::string("unit-test tool")), &bytes, nullptr));
  Container c;
  KZ_CHECK_OK(Container::parse(bytes, &c));
  KZ_CHECK_OK(c.verify_content());
  kz::trace::RealTraceOptions opt;
  opt.consts["CFE_PLATFORM_ES_MAX_APPLICATIONS"] = 32;  // unit-test value for the table size the profile names
  check_same_replay(run, c, p, opt);
  return kztest::finish("test_trace_neutral (real record)");
}

}  // namespace

int main(int argc, char** argv) {
  if (argc == 3) return real_record(argv[1], argv[2]);
  test_codec();
  const Container run = make_run();
  test_f0_conversion(run);
  Container converted;
  test_round_trip(run, &converted);
  test_equivalence(run, converted);
  return kztest::finish("test_trace_neutral");
}

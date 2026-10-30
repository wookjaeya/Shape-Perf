// Unit tests of the memory reads (fetch slots: the probe event's fetch arguments, PERF_SAMPLE_RAW) and of the
// attribution of a point reached from a named function (decode_merge.hpp), on synthetic streams.  The raw
// records follow the layout of the kernel's probe-event records as the format file states it (a common
// header, then the fields at the offsets the format gives; a string as a __data_loc word with its text
// after the fields).  Values are unit-test values.
#include <cstring>

#include "kz/trace/decode_merge.hpp"
#include "support/kz_test.hpp"
#include "support/perf_synth.hpp"

using namespace kz::trace;
using kzsyn::Stream;

namespace {

constexpr uint64_t kSide = 100, kFEntry = 10, kFRet = 11, kVia = 12, kStatic = 13, kFetch = 14;
constexpr uint64_t kBias = 0x5000;

SiteSpec site(uint32_t id, uint16_t kind, uint64_t vaddr, const std::string& symbol, std::vector<SlotSpec> slots) {
  SiteSpec s;
  s.site = id;
  s.kind = kind;
  s.phases = kind == KZ_SITE_KIND_ENTRY_PROBE ? KZ_SITE_PHASE_ENTRY : kind == KZ_SITE_KIND_RET_PROBE ? KZ_SITE_PHASE_RET : KZ_SITE_PHASE_POINT;
  s.decoder = KZ_DECODER_GENERIC;
  s.vaddr = vaddr;
  s.symbol = symbol;
  s.label = "site" + std::to_string(id);
  s.slots = std::move(slots);
  return s;
}

// The raw record of a probe event with two fields: f0 u32 at offset 16 and f1 a string (__data_loc at 20).
std::vector<uint8_t> raw_record(uint32_t f0, const char* f1) {
  std::vector<uint8_t> r(24, 0);
  r[0] = 0x16;  // common_type (any value), flags, preempt count, pid: not read
  std::memcpy(&r[16], &f0, 4);
  uint32_t loc = 0;
  if (f1 != nullptr) {
    const uint32_t len = static_cast<uint32_t>(std::strlen(f1) + 1);
    loc = static_cast<uint32_t>(r.size()) | (len << 16);
    r.insert(r.end(), f1, f1 + len);
  } else {
    loc = static_cast<uint32_t>(r.size());  // length 0: the kernel could not read the string
  }
  std::memcpy(&r[20], &loc, 4);
  return r;
}

struct Fixture {
  DecodeInput in;
  EventLayout side = kzsyn::side_layout(kSide);
  EventLayout fentry = kzsyn::probe_layout(kFEntry, 0, (1u << 7), 8);
  EventLayout fret = kzsyn::probe_layout(kFRet, 1, 1, 0);
  EventLayout via = kzsyn::probe_layout(kVia, 2, (1u << 7), 8);
  EventLayout via_static = kzsyn::probe_layout(kStatic, 3, (1u << 7), 8);
  EventLayout fetch = kzsyn::probe_layout(kFetch, 4, 1, 0);
  Stream cpu0;

  Fixture() {
    const SlotSpec ra_stack{3, KZ_SLOT_SRC_STACK, 8, 0, "return_address"};
    in.sites.push_back(site(0, KZ_SITE_KIND_ENTRY_PROBE, 0x1100, "F", {ra_stack}));
    in.sites.push_back(site(1, KZ_SITE_KIND_RET_PROBE, 0x1100, "F", {{0, KZ_SLOT_SRC_REG, 4, KZ_REG_RAX, "status"}, {3, KZ_SLOT_SRC_IP, 8, 0, "return_address"}}));
    // G reached from F by a tail jump (the return address is F's own) ...
    SiteSpec g = site(2, KZ_SITE_KIND_POINT_PROBE, 0x1200, "G", {ra_stack});
    g.via_symbol = "F";
    g.via_chain = "F>G";
    g.via_hops = 1;
    g.via_ret_dynamic = true;
    in.sites.push_back(g);
    // ... and H reached from F through a call that returns to 0x1150 (link time)
    SiteSpec h = site(3, KZ_SITE_KIND_POINT_PROBE, 0x1300, "H", {ra_stack});
    h.via_symbol = "F";
    h.via_chain = "F>H";
    h.via_hops = 1;
    h.via_expect_ret = 0x1150;
    in.sites.push_back(h);
    // a site with fetch slots: an integer and a string read by the kernel
    SlotSpec m{0, KZ_SLOT_SRC_MEM, 4, KZ_REG_RDI, "obj_id"};
    m.offset = 16;
    SlotSpec t{1, KZ_SLOT_SRC_STR, 4, KZ_REG_RSI, "name"};
    in.sites.push_back(site(4, KZ_SITE_KIND_POINT_PROBE, 0x1400, "K", {m, t, {2, KZ_SLOT_SRC_REG, 8, KZ_REG_RAX, "rax"}}));
    fetch.sample_type |= kzsyn::pa::kSampleRaw;
    fetch.fields = {RawField{0, 16, 4, false}, RawField{1, 20, 4, true}};
    for (const EventLayout& l : {side, fentry, fret, via, via_static, fetch}) KZ_CHECK_OK(in.layouts.add(l));
    in.traced.known = true;
    in.traced.ino = 4242;
  }
  void at_entry(const EventLayout& l, uint64_t vaddr, uint32_t tid, uint64_t t, uint64_t ra, uint64_t sp = 0x7ff0) {
    Stream::SampleArgs a;
    a.ip = kBias + vaddr; a.pid = 7; a.tid = tid; a.time = t;
    a.regs = {sp};
    for (int i = 0; i < 8; ++i) a.stack.push_back(static_cast<uint8_t>(ra >> (8 * i)));
    cpu0.sample(l, a);
  }
  void f_entry(uint32_t tid, uint64_t t, uint64_t ra, uint64_t sp) { at_entry(fentry, 0x1100, tid, t, ra, sp); }
  void f_ret(uint32_t tid, uint64_t t, uint64_t ra) {
    Stream::SampleArgs a;
    a.ip = ra; a.pid = 7; a.tid = tid; a.time = t;
    a.regs = {0};
    cpu0.sample(fret, a);
  }
  DecodedRun run() {
    in.streams.clear();
    in.streams.push_back({0, cpu0.bytes});
    DecodedRun out;
    KZ_CHECK_OK(decode_run(in, &out));
    return out;
  }
};

size_t count_site(const DecodedRun& r, uint32_t site) {
  size_t n = 0;
  for (const kz_raw_ev& e : r.events) n += e.site == site ? 1 : 0;
  return n;
}

void test_fetch_slots() {
  Fixture f;
  Stream::SampleArgs a;
  a.ip = kBias + 0x1400; a.pid = 7; a.tid = 20; a.time = 10;
  a.regs = {0x99};
  a.raw_data = raw_record(0x30005, "BP0");
  f.cpu0.sample(f.fetch, a);
  a.time = 20;
  a.raw_data = raw_record(0x30006, nullptr);  // the string could not be read
  f.cpu0.sample(f.fetch, a);
  a.time = 30;
  a.raw_data = raw_record(7, "");  // an empty string is a string
  f.cpu0.sample(f.fetch, a);
  const DecodedRun r = f.run();
  KZ_CHECK_EQ(r.events.size(), 3u);
  KZ_CHECK_EQ(r.events[0].a[0], 0x30005u);
  KZ_CHECK_EQ(r.events[0].a[1], 3u);     // the string's byte length
  KZ_CHECK_EQ(r.events[0].a[2], 0x99u);  // a register slot of the same site, from the sample
  KZ_CHECK_EQ(r.events[0].flags & KZ_RAW_FLAG_TRUNC, 0);
  KZ_CHECK((r.events[1].flags & KZ_RAW_FLAG_TRUNC) != 0);
  KZ_CHECK_EQ(r.events[1].a[0], 0x30006u);  // the readable field is kept
  KZ_CHECK_EQ(r.strings.size(), 2u);
  KZ_CHECK_EQ(r.strings[0].event, 0u);
  KZ_CHECK_EQ(r.strings[0].slot, 1u);
  KZ_CHECK_EQ(r.strings[0].text, std::string("BP0"));
  KZ_CHECK_EQ(r.strings[1].event, 2u);
  KZ_CHECK_EQ(r.strings[1].text, std::string(""));
  // the raw record is taken from the sample (the decoder parses RAW between CPU and the registers)
  std::vector<PerfRecord> recs;
  KZ_CHECK_OK(parse_records(f.cpu0.bytes.data(), f.cpu0.bytes.size(), 0, f.in.layouts, &recs));
  KZ_CHECK(!recs.empty() && recs[0].raw.size() >= 28 && recs[0].regs[0] == 0x99);
  // a field beyond the raw record: unreadable, never a value
  Fixture g;
  g.fetch.fields = {RawField{0, 60, 4, false}, RawField{1, 20, 4, true}};
  DecodeInput in2 = g.in;
  in2.layouts = LayoutTable();
  for (const EventLayout& l : {g.side, g.fentry, g.fret, g.via, g.via_static, g.fetch}) KZ_CHECK_OK(in2.layouts.add(l));
  Stream s2;
  a.raw_data = raw_record(5, "X");
  s2.sample(g.fetch, a);
  in2.streams = {{0, s2.bytes}};
  DecodedRun r2;
  KZ_CHECK_OK(decode_run(in2, &r2));
  KZ_CHECK(r2.events.size() == 1 && (r2.events[0].flags & KZ_RAW_FLAG_TRUNC) != 0 && r2.events[0].a[0] == 0);
}

void test_reached_attribution() {
  Fixture f;
  // thread 20: F called from 0xaaa with sp 0x7f00, G entered by F's tail jump: the same sp (the top of the
  // stack may hold the return probe's trampoline instead of 0xaaa: not compared): kept
  f.f_entry(20, 10, 0xaaa, 0x7f00);
  f.at_entry(f.via, 0x1200, 20, 11, 0x7ffff000, 0x7f00);
  // H entered through F's call: the return address is 0x1150 + bias: kept; another return address: not
  f.at_entry(f.via_static, 0x1300, 20, 12, kBias + 0x1150, 0x7ee0);
  f.at_entry(f.via_static, 0x1300, 20, 13, kBias + 0x1160, 0x7ee0);
  // G entered with another stack pointer while F is open (a call from inside F, or from elsewhere): not kept
  f.at_entry(f.via, 0x1200, 20, 14, 0xbbb, 0x7ee8);
  f.f_ret(20, 15, 0xaaa);
  // after F returned, and on a thread with no open call of F: not kept
  f.at_entry(f.via, 0x1200, 20, 16, 0xaaa, 0x7f00);
  f.at_entry(f.via, 0x1200, 21, 17, 0xaaa, 0x7f00);
  // nested: F open on thread 22, then again (recursion through another path): the innermost decides
  f.f_entry(22, 18, 0xccc, 0x6f00);
  f.f_entry(22, 19, 0xddd, 0x6e00);
  f.at_entry(f.via, 0x1200, 22, 20, 0xccc, 0x6f00);  // the outer call's frame: not the innermost -> not kept
  f.at_entry(f.via, 0x1200, 22, 21, 0xddd, 0x6e00);  // kept
  f.f_ret(22, 22, 0xddd);
  f.at_entry(f.via, 0x1200, 22, 23, 0xccc, 0x6f00);  // now the outer call is the innermost: kept
  const DecodedRun r = f.run();
  KZ_CHECK_EQ(count_site(r, 2), 3u);
  KZ_CHECK_EQ(count_site(r, 3), 1u);
  KZ_CHECK_EQ(r.stats.site_hits[2], 7u);  // every sample is a trap, kept or not
  KZ_CHECK_EQ(r.stats.via_unattributed[2], 4u);
  KZ_CHECK_EQ(r.stats.via_unattributed[3], 1u);
  KZ_CHECK_EQ(r.stats.via_unattributed[0], 0u);
}

void test_plan_check() {
  Fixture f;
  KZ_CHECK_OK(check_plan(f.in.sites));
  // the function a point is reached from must have its entry and return sites
  std::vector<SiteSpec> no_ret = f.in.sites;
  no_ret.erase(no_ret.begin() + 1);
  for (size_t i = 0; i < no_ret.size(); ++i) no_ret[i].site = static_cast<uint32_t>(i);
  KZ_CHECK_ERR(check_plan(no_ret), kz::Err::Invalid);
  // the attribution reads slot 3
  std::vector<SiteSpec> no_key = f.in.sites;
  no_key[2].slots.clear();
  KZ_CHECK_ERR(check_plan(no_key), kz::Err::Invalid);
  // a string is never read where a decoder reads a number
  SiteSpec spawn = site(0, KZ_SITE_KIND_ENTRY_PROBE, 0x1, "S", {{0, KZ_SLOT_SRC_STR, 4, KZ_REG_RDI, "id"}});
  spawn.decoder = KZ_DECODER_THREAD_ENTRY;
  KZ_CHECK_ERR(check_site(spawn), kz::Err::Invalid);
  // a string slot keeps 4 bytes (its length); a memory slot needs a known register
  SiteSpec bad = site(0, KZ_SITE_KIND_POINT_PROBE, 0x1, "S", {{0, KZ_SLOT_SRC_STR, 8, KZ_REG_RDI, "name"}});
  KZ_CHECK_ERR(check_site(bad), kz::Err::Invalid);
  bad.slots = {{0, KZ_SLOT_SRC_MEM, 4, 99, "v"}};
  KZ_CHECK_ERR(check_site(bad), kz::Err::Invalid);
  // a fetch slot asks nothing of the sample
  const PerfNeeds n = needs_of(f.in.sites[4]);
  KZ_CHECK_EQ(n.regs_user, 1u);  // rax only, for the register slot
  KZ_CHECK_EQ(n.stack_bytes, 0u);
  KZ_CHECK(has_fetch_slots(f.in.sites[4]));
  KZ_CHECK(!has_fetch_slots(f.in.sites[0]));
}

}  // namespace

int main() {
  test_fetch_slots();
  test_reached_attribution();
  test_plan_check();
  return kztest::finish("test_trace_decode_fetch");
}

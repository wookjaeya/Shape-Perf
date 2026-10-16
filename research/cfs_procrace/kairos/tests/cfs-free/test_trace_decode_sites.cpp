// Unit tests of the site descriptors: what a site asks of the kernel, and how a sample's
// slots become the four raw values.
#include <cstring>

#include "kz/trace/decode_match.hpp"
#include "kz/trace/decode_sites.hpp"
#include "support/kz_test.hpp"
#include "support/perf_synth.hpp"

using namespace kz::trace;

namespace {

SiteSpec entry_site() {
  SiteSpec s;
  s.site = 0;
  s.kind = KZ_SITE_KIND_ENTRY_PROBE;
  s.phases = KZ_SITE_PHASE_ENTRY;
  s.decoder = KZ_DECODER_GENERIC;
  s.label = "unit.entry";
  s.slots = {{0, KZ_SLOT_SRC_REG, 2, KZ_REG_RDI, "id"},
             {1, KZ_SLOT_SRC_REG, 4, KZ_REG_RDX, "app"},
             {2, KZ_SLOT_SRC_REG, 8, KZ_REG_R9, "wide"},
             {3, KZ_SLOT_SRC_STACK, 8, 0, "return_address"}};
  return s;
}

void test_register_numbers() {
  // KZ_REG order is rax rbx rcx rdx rsi rdi rbp rsp r8..r15; perf's x86 numbering puts ip, flags and the
  // segment registers between rsp and r8.
  KZ_CHECK_EQ(perf_reg_of(KZ_REG_RAX), 0u);
  KZ_CHECK_EQ(perf_reg_of(KZ_REG_RDI), 5u);
  KZ_CHECK_EQ(perf_reg_of(KZ_REG_RSP), 7u);
  KZ_CHECK_EQ(perf_reg_of(KZ_REG_R8), 16u);
  KZ_CHECK_EQ(perf_reg_of(KZ_REG_R9), 17u);
  KZ_CHECK_EQ(perf_reg_of(KZ_REG_R15), 23u);
  KZ_CHECK_EQ(perf_reg_of(16), KZ_NONE32);
  KZ_CHECK_EQ(perf_reg_of(1000), KZ_NONE32);
}

void test_needs() {
  const SiteSpec s = entry_site();
  KZ_CHECK_OK(check_site(s));
  const PerfNeeds n = needs_of(s);
  KZ_CHECK_EQ(n.stack_bytes, 8u);
  KZ_CHECK_EQ(n.regs_user, uint64_t((1u << 5) | (1u << 3) | (1ull << 17) | (1u << 7)));  // rdi rdx r9 + rsp for the dump
  SiteSpec ret;
  ret.kind = KZ_SITE_KIND_RET_PROBE;
  ret.phases = KZ_SITE_PHASE_RET;
  ret.decoder = KZ_DECODER_GENERIC;
  ret.label = "unit.ret";
  ret.slots = {{0, KZ_SLOT_SRC_REG, 4, KZ_REG_RAX, "status"}, {3, KZ_SLOT_SRC_IP, 8, 0, "return_address"}};
  KZ_CHECK_OK(check_site(ret));
  const PerfNeeds r = needs_of(ret);
  KZ_CHECK_EQ(r.stack_bytes, 0u);
  KZ_CHECK_EQ(r.regs_user, uint64_t(1));
  SiteSpec odd = entry_site();
  odd.slots = {{0, KZ_SLOT_SRC_STACK, 8, 12, "w"}};  // offset 12, 8 bytes: 20 -> rounded to 24
  KZ_CHECK_EQ(needs_of(odd).stack_bytes, 24u);
  KZ_CHECK_EQ(phase_of(s), uint16_t(KZ_PHASE_ENTRY));
  KZ_CHECK_EQ(phase_of(ret), uint16_t(KZ_PHASE_RET));
}

void test_check_site_rejects() {
  auto bad = [](SiteSpec s) { return check_site(s).code == kz::Err::Invalid; };
  SiteSpec s = entry_site();
  KZ_CHECK(!bad(s));
  { SiteSpec t = s; t.kind = KZ_SITE_KIND_WATCHPOINT; KZ_CHECK(bad(t)); }
  { SiteSpec t = s; t.phases = KZ_SITE_PHASE_RET; KZ_CHECK(bad(t)); }
  { SiteSpec t = s; t.decoder = 99; KZ_CHECK(bad(t)); }
  { SiteSpec t = s; t.slots[1].slot = 0; KZ_CHECK(bad(t)); }          // slot 0 twice
  { SiteSpec t = s; t.slots[0].slot = 4; KZ_CHECK(bad(t)); }
  { SiteSpec t = s; t.slots[0].width = 3; KZ_CHECK(bad(t)); }
  { SiteSpec t = s; t.slots[0].index = 16; KZ_CHECK(bad(t)); }        // no such register
  { SiteSpec t = s; t.slots[0].src = 0; KZ_CHECK(bad(t)); }
  { SiteSpec t = s; t.slots[0].role.clear(); KZ_CHECK(bad(t)); }
  { SiteSpec t = s; t.slots[3].index = 100000; KZ_CHECK(bad(t)); }
  { SiteSpec t = s; t.slots.resize(5, t.slots[0]); KZ_CHECK(bad(t)); }
}

// The decoders of the identity join read fixed positions (kz_raw.h, KZ_DECODER): check_site says
// which, and refuses a site that does not declare them.
void test_check_site_decoders() {
  auto bad = [](const SiteSpec& s) { return check_site(s).code == kz::Err::Invalid; };
  SiteSpec c = entry_site();
  c.decoder = KZ_DECODER_SPAWN_CREATE;
  c.slots = {{0, KZ_SLOT_SRC_REG, 8, KZ_REG_RSI, "name_ptr"}, {1, KZ_SLOT_SRC_REG, 8, KZ_REG_RDX, "entry_fn"},
             {2, KZ_SLOT_SRC_REG, 4, KZ_REG_R9, "priority"}, {3, KZ_SLOT_SRC_STACK, 8, 0, "return_address"}};
  KZ_CHECK_OK(check_site(c));
  { SiteSpec t = c; t.slots.erase(t.slots.begin()); KZ_CHECK_OK(check_site(t)); }   // the name pointer is not read
  { SiteSpec t = c; t.slots.erase(t.slots.begin() + 1); KZ_CHECK(bad(t)); }         // entry_fn missing
  { SiteSpec t = c; t.slots[1].role = "fn"; KZ_CHECK(bad(t)); }                     // another role at slot 1
  { SiteSpec t = c; t.slots[2].width = 8; KZ_CHECK(bad(t)); }                       // priority kept in 32 bits
  { SiteSpec t = c; t.slots[3].role = "ra"; KZ_CHECK(bad(t)); }
  { SiteSpec t = c; t.slots.pop_back(); KZ_CHECK(bad(t)); }
  { SiteSpec t = c; t.kind = KZ_SITE_KIND_RET_PROBE; t.phases = KZ_SITE_PHASE_RET; KZ_CHECK(bad(t)); }
  SiteSpec r;
  r.kind = KZ_SITE_KIND_RET_PROBE;
  r.phases = KZ_SITE_PHASE_RET;
  r.decoder = KZ_DECODER_SPAWN_RETURN;
  r.slots = {{0, KZ_SLOT_SRC_REG, 4, KZ_REG_RAX, "status"}, {3, KZ_SLOT_SRC_IP, 8, 0, "return_address"}};
  KZ_CHECK_OK(check_site(r));
  { SiteSpec t = r; t.slots.pop_back(); KZ_CHECK(bad(t)); }
  { SiteSpec t = r; t.kind = KZ_SITE_KIND_ENTRY_PROBE; t.phases = KZ_SITE_PHASE_ENTRY; KZ_CHECK(bad(t)); }
  SiteSpec e = entry_site();
  e.decoder = KZ_DECODER_THREAD_ENTRY;
  e.slots = {{0, KZ_SLOT_SRC_REG, 4, KZ_REG_RDI, "task_id"}};
  KZ_CHECK_OK(check_site(e));
  { SiteSpec t = e; t.slots[0].width = 8; KZ_CHECK(bad(t)); }                       // the id is kept in 32 bits
  { SiteSpec t = e; t.slots[0].slot = 1; KZ_CHECK(bad(t)); }
  // a generic site reads by role: no position is required
  { SiteSpec t = entry_site(); t.slots = {{2, KZ_SLOT_SRC_REG, 4, KZ_REG_RDI, "anything"}}; KZ_CHECK_OK(check_site(t)); }
}

SiteSpec pair_site(uint32_t id, uint16_t kind, const char* symbol) {
  SiteSpec s;
  s.site = id;
  s.kind = kind;
  s.phases = kind == KZ_SITE_KIND_ENTRY_PROBE ? KZ_SITE_PHASE_ENTRY : KZ_SITE_PHASE_RET;
  s.decoder = KZ_DECODER_GENERIC;
  s.label = std::string(symbol) + (kind == KZ_SITE_KIND_ENTRY_PROBE ? ".entry" : ".ret");
  s.symbol = symbol;
  if (kind == KZ_SITE_KIND_ENTRY_PROBE)
    s.slots = {{0, KZ_SLOT_SRC_REG, 4, KZ_REG_RDI, "x"}, {3, KZ_SLOT_SRC_STACK, 8, 0, "return_address"}};
  else
    s.slots = {{0, KZ_SLOT_SRC_REG, 4, KZ_REG_RAX, "status"}, {3, KZ_SLOT_SRC_IP, 8, 0, "return_address"}};
  return s;
}

void test_check_plan_pairs() {
  std::vector<SiteSpec> plan = {pair_site(0, KZ_SITE_KIND_ENTRY_PROBE, "f"), pair_site(1, KZ_SITE_KIND_RET_PROBE, "f")};
  KZ_CHECK_OK(check_plan(plan));
  KZ_CHECK(has_return_address_key(plan[0]) && has_return_address_key(plan[1]));
  { auto t = plan; t[1].slots.pop_back(); KZ_CHECK_ERR(check_plan(t), kz::Err::Invalid); }               // the return has no key
  { auto t = plan; t[0].slots[1].role = "caller"; KZ_CHECK_ERR(check_plan(t), kz::Err::Invalid); }       // the entry's slot 3 is something else
  { auto t = plan; t[0].slots[1].width = 4; KZ_CHECK_ERR(check_plan(t), kz::Err::Invalid); }             // a 4-byte key
  { auto t = plan; t[1].slots.pop_back(); t[1].symbol = "g"; KZ_CHECK_OK(check_plan(t)); }               // not a pair: no key needed
  { auto t = plan; t[0].kind = 77; KZ_CHECK_ERR(check_plan(t), kz::Err::Invalid); }                       // every site is checked
}

kz_raw_ev ev(uint32_t tid, uint32_t site, uint64_t a3, uint16_t flags = 0) {
  kz_raw_ev e;
  std::memset(&e, 0, sizeof e);
  e.tid = tid;
  e.site = site;
  e.a[3] = a3;
  e.flags = flags;
  return e;
}

void test_match_calls() {
  const std::vector<SiteSpec> plan = {pair_site(0, KZ_SITE_KIND_ENTRY_PROBE, "f"), pair_site(1, KZ_SITE_KIND_RET_PROBE, "f")};
  // Nested calls on one thread pair by return address, innermost first; another thread is separate.
  {
    const std::vector<kz_raw_ev> evs = {ev(1, 0, 0x100), ev(1, 0, 0x200), ev(2, 0, 0x100), ev(1, 1, 0x200), ev(2, 1, 0x100), ev(1, 1, 0x100)};
    MatchStats st;
    const std::vector<CallPair> p = match_calls(evs, plan, &st);
    KZ_CHECK_EQ(st.sites_paired, uint64_t(1));
    KZ_CHECK_EQ(st.pairs, uint64_t(3));
    KZ_CHECK_EQ(st.entries_without_return, uint64_t(0));
    KZ_CHECK_EQ(st.returns_without_entry, uint64_t(0));
    KZ_CHECK_EQ(p.size(), size_t(3));
    KZ_CHECK_EQ(p[0].return_event, 5u);
    KZ_CHECK_EQ(p[1].return_event, 3u);
    KZ_CHECK_EQ(p[2].return_event, 4u);
  }
  // An event whose slot was not copied (TRUNC: the value there is 0) is never matched on that 0: an
  // entry and a return that would both read 0 stay apart, and are counted separately.
  {
    const std::vector<kz_raw_ev> evs = {ev(1, 0, 0, KZ_RAW_FLAG_TRUNC), ev(1, 1, 0)};
    MatchStats st;
    const std::vector<CallPair> p = match_calls(evs, plan, &st);
    KZ_CHECK_EQ(st.pairs, uint64_t(0));
    KZ_CHECK_EQ(st.events_with_unreadable_key, uint64_t(1));
    KZ_CHECK_EQ(st.entries_without_return, uint64_t(0));
    KZ_CHECK_EQ(st.returns_without_entry, uint64_t(1));
    KZ_CHECK_EQ(p.size(), size_t(2));
    KZ_CHECK(p[0].unreadable_key);
    KZ_CHECK_EQ(p[0].return_event, KZ_NONE32);
  }
  {
    const std::vector<kz_raw_ev> evs = {ev(1, 0, 0x100), ev(1, 1, 0, KZ_RAW_FLAG_TRUNC)};
    MatchStats st;
    const std::vector<CallPair> p = match_calls(evs, plan, &st);
    KZ_CHECK_EQ(st.pairs, uint64_t(0));
    KZ_CHECK_EQ(st.events_with_unreadable_key, uint64_t(1));
    KZ_CHECK_EQ(st.entries_without_return, uint64_t(1));
    KZ_CHECK(p[1].unreadable_key);
  }
  // Sites of one symbol without the key are not paired at all (and the sites without slots, as a
  // reader that forgot them would pass, pair nothing).
  {
    std::vector<SiteSpec> nokey = plan;
    nokey[1].slots.pop_back();
    MatchStats st;
    const std::vector<CallPair> p = match_calls({ev(1, 0, 0x100), ev(1, 1, 0x100)}, nokey, &st);
    KZ_CHECK_EQ(st.sites_paired, uint64_t(0));
    KZ_CHECK_EQ(st.sites_without_key, uint64_t(1));
    KZ_CHECK_EQ(st.pairs, uint64_t(0));
    KZ_CHECK(p.empty());
  }
}

void test_apply() {
  const SiteSpec s = entry_site();
  PerfRecord rec;
  rec.kind = RecKind::Sample;
  rec.ip = 0x1111;
  rec.regs_valid = (1u << 5) | (1u << 3) | (1ull << 17) | (1u << 7);
  rec.regs[5] = 0xffffffffffff1234ull;  // edi garbage above 16 bits: width 2 keeps 0x1234
  rec.regs[3] = 0x1122334455667788ull;  // width 4 keeps 0x55667788
  rec.regs[17] = 0xfedcba9876543210ull;
  rec.stack = {0x25, 0xc3, 0x17, 0x60, 0x8d, 0x55, 0x00, 0x00};
  uint64_t out[4];
  uint16_t flags = 0xffff;
  KZ_CHECK_OK(apply_slots(s, rec, out, &flags));
  KZ_CHECK_EQ(out[0], uint64_t(0x1234));
  KZ_CHECK_EQ(out[1], uint64_t(0x55667788));
  KZ_CHECK_EQ(out[2], uint64_t(0xfedcba9876543210ull));
  KZ_CHECK_EQ(out[3], uint64_t(0x558d6017c325));
  KZ_CHECK_EQ(flags, uint16_t(0));

  // A register the kernel did not copy, and a stack word beyond the dumped bytes, are zero and flagged.
  PerfRecord missing = rec;
  missing.regs_valid &= ~(1u << 3);
  missing.stack.resize(4);
  KZ_CHECK_OK(apply_slots(s, missing, out, &flags));
  KZ_CHECK_EQ(out[1], uint64_t(0));
  KZ_CHECK_EQ(out[3], uint64_t(0));
  KZ_CHECK_EQ(out[0], uint64_t(0x1234));
  KZ_CHECK((flags & KZ_RAW_FLAG_TRUNC) != 0);

  // An unused slot stays zero; the ip source.
  SiteSpec ret;
  ret.kind = KZ_SITE_KIND_RET_PROBE;
  ret.phases = KZ_SITE_PHASE_RET;
  ret.decoder = KZ_DECODER_GENERIC;
  ret.slots = {{0, KZ_SLOT_SRC_REG, 4, KZ_REG_RAX, "status"}, {3, KZ_SLOT_SRC_IP, 8, 0, "return_address"}};
  PerfRecord rr;
  rr.kind = RecKind::Sample;
  rr.ip = 0x558d6017c325;
  rr.regs_valid = 1;
  rr.regs[0] = 0xffffffffc2000005ull;
  KZ_CHECK_OK(apply_slots(ret, rr, out, &flags));
  KZ_CHECK_EQ(out[0], uint64_t(0xc2000005));  // zero-extended 32 bits: the status as the callee's int32 pattern
  KZ_CHECK_EQ(out[1], uint64_t(0));
  KZ_CHECK_EQ(out[2], uint64_t(0));
  KZ_CHECK_EQ(out[3], uint64_t(0x558d6017c325));
  KZ_CHECK_EQ(flags, uint16_t(0));

  PerfRecord not_a_sample;
  not_a_sample.kind = RecKind::Fork;
  KZ_CHECK_ERR(apply_slots(ret, not_a_sample, out, &flags), kz::Err::Invalid);
}

}  // namespace

int main() {
  test_register_numbers();
  test_needs();
  test_check_site_rejects();
  test_check_site_decoders();
  test_check_plan_pairs();
  test_match_calls();
  test_apply();
  return kztest::finish("test_trace_decode_sites");
}

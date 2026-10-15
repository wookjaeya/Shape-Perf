// Unit tests of the site descriptors: what a site asks of the kernel, and how a sample's
// slots become the four raw values.
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
  test_apply();
  return kztest::finish("test_trace_decode_sites");
}

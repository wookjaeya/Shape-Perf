// Unit tests of the merge: per-CPU streams to one ordered, filtered, typed run.
#include <cstring>

#include "kz/trace/decode_merge.hpp"
#include "support/kz_test.hpp"
#include "support/perf_synth.hpp"

using namespace kz::trace;
using kzsyn::Stream;

namespace {

constexpr uint64_t kSide0 = 100, kSide1 = 101;  // the side-band events of CPU 0 and CPU 1
constexpr uint64_t kEntryId = 10, kRetId = 11, kPointId = 12;

SiteSpec site(uint32_t id, uint16_t kind, uint64_t vaddr, std::vector<SlotSpec> slots) {
  SiteSpec s;
  s.site = id;
  s.kind = kind;
  s.phases = kind == KZ_SITE_KIND_ENTRY_PROBE ? KZ_SITE_PHASE_ENTRY : kind == KZ_SITE_KIND_RET_PROBE ? KZ_SITE_PHASE_RET : KZ_SITE_PHASE_POINT;
  s.decoder = KZ_DECODER_GENERIC;
  s.vaddr = vaddr;
  s.label = "site" + std::to_string(id);
  s.slots = std::move(slots);
  return s;
}

struct Fixture {
  DecodeInput in;
  EventLayout side0 = kzsyn::side_layout(kSide0);
  EventLayout side1 = kzsyn::side_layout(kSide1);
  EventLayout entry = kzsyn::probe_layout(kEntryId, 0, (1u << 5) | (1u << 7), 8);
  EventLayout ret = kzsyn::probe_layout(kRetId, 1, 1, 0);
  EventLayout point = kzsyn::probe_layout(kPointId, 2, 1, 0);
  Stream cpu0, cpu1;

  Fixture() {
    in.sites.push_back(site(0, KZ_SITE_KIND_ENTRY_PROBE, 0x122a, {{0, KZ_SLOT_SRC_REG, 4, KZ_REG_RDI, "arg"}, {3, KZ_SLOT_SRC_STACK, 8, 0, "return_address"}}));
    in.sites.push_back(site(1, KZ_SITE_KIND_RET_PROBE, 0, {{0, KZ_SLOT_SRC_REG, 4, KZ_REG_RAX, "status"}, {3, KZ_SLOT_SRC_IP, 8, 0, "return_address"}}));
    in.sites.push_back(site(2, KZ_SITE_KIND_POINT_PROBE, 0x1372, {{0, KZ_SLOT_SRC_REG, 4, KZ_REG_RAX, "value"}}));
    for (const EventLayout& l : {side0, side1, entry, ret, point}) KZ_CHECK_OK(in.layouts.add(l));
    in.traced.known = true;
    in.traced.ino = 4242;
    in.traced.dev_major = 254;
    in.traced.dev_minor = 0;
  }
  void entry_sample(Stream& s, uint32_t pid, uint32_t tid, uint64_t t, uint64_t arg, uint64_t retaddr, uint64_t bias = 0x5000) {
    Stream::SampleArgs a;
    a.ip = bias + 0x122a; a.pid = pid; a.tid = tid; a.time = t;
    a.regs = {arg, 0x7ff0};
    for (int i = 0; i < 8; ++i) a.stack.push_back(static_cast<uint8_t>(retaddr >> (8 * i)));
    s.sample(entry, a);
  }
  void ret_sample(Stream& s, uint32_t pid, uint32_t tid, uint64_t t, uint64_t status, uint64_t retaddr) {
    Stream::SampleArgs a;
    a.ip = retaddr; a.pid = pid; a.tid = tid; a.time = t;
    a.regs = {status};
    s.sample(ret, a);
  }
  DecodedRun run(kz::Status* st = nullptr) {
    in.streams.clear();
    in.streams.push_back({0, cpu0.bytes});
    in.streams.push_back({1, cpu1.bytes});
    DecodedRun out;
    kz::Status s = decode_run(in, &out);
    if (st != nullptr) *st = s;
    return out;
  }
};

void test_order_and_tseq() {
  Fixture f;
  // Thread 20 runs on CPU 0 then migrates to CPU 1; thread 21 interleaves.  The merged order is by
  // time, and each thread's sequence number follows it.
  f.entry_sample(f.cpu0, 7, 20, 100, 1, 0xaaa);
  f.ret_sample(f.cpu0, 7, 20, 110, 0, 0xaaa);
  f.entry_sample(f.cpu1, 7, 21, 105, 2, 0xbbb);
  f.entry_sample(f.cpu1, 7, 20, 120, 3, 0xccc);   // thread 20 after migration
  f.ret_sample(f.cpu1, 7, 21, 125, 0, 0xbbb);
  DecodedRun r = f.run();
  KZ_CHECK_EQ(r.events.size(), size_t(5));
  const uint64_t times[5] = {100, 105, 110, 120, 125};
  const uint32_t tids[5] = {20, 21, 20, 20, 21};
  const uint32_t tseq[5] = {0, 0, 1, 2, 1};
  for (int i = 0; i < 5; ++i) {
    KZ_CHECK_EQ(r.events[i].t_ns, times[i]);
    KZ_CHECK_EQ(r.events[i].tid, tids[i]);
    KZ_CHECK_EQ(r.events[i].tseq, tseq[i]);
  }
  KZ_CHECK_EQ(r.events[0].site, 0u);
  KZ_CHECK_EQ(r.events[0].phase, uint16_t(KZ_PHASE_ENTRY));
  KZ_CHECK_EQ(r.events[0].a[0], uint64_t(1));
  KZ_CHECK_EQ(r.events[0].a[3], uint64_t(0xaaa));
  KZ_CHECK_EQ(r.events[2].phase, uint16_t(KZ_PHASE_RET));
  KZ_CHECK_EQ(r.events[2].a[3], uint64_t(0xaaa));
  KZ_CHECK_EQ(r.events[0].objseq, uint64_t(0));
  KZ_CHECK_EQ(r.events[0].flags, uint16_t(0));
  KZ_CHECK_EQ(r.stats.site_hits[0], uint64_t(3));
  KZ_CHECK_EQ(r.stats.site_hits[1], uint64_t(2));
  KZ_CHECK_EQ(r.stats.site_hits[2], uint64_t(0));
  KZ_CHECK(r.bias_known);
  KZ_CHECK_EQ(r.load_bias, uint64_t(0x5000));
  KZ_CHECK_EQ(r.stats.bias_mismatches, uint64_t(0));
  KZ_CHECK(r.traced_pids == std::vector<uint32_t>({7}));

  // Equal timestamps on two CPUs: the CPU number decides, deterministically.
  Fixture g;
  g.entry_sample(g.cpu1, 7, 30, 500, 1, 1);
  g.entry_sample(g.cpu0, 7, 31, 500, 2, 2);
  DecodedRun h = g.run();
  KZ_CHECK_EQ(h.events[0].tid, 31u);
  KZ_CHECK_EQ(h.events[1].tid, 30u);

  // Deterministic: the same input twice gives the same events.
  DecodedRun r2 = f.run();
  KZ_CHECK_EQ(r2.events.size(), r.events.size());
  KZ_CHECK(std::memcmp(r2.events.data(), r.events.data(), r.events.size() * sizeof(kz_raw_ev)) == 0);
}

void test_load_bias_mismatch() {
  Fixture f;
  f.entry_sample(f.cpu0, 7, 20, 100, 1, 1, 0x5000);
  f.entry_sample(f.cpu0, 7, 20, 110, 1, 1, 0x6000);  // a different load address: a second mapping, or a wrong site
  DecodedRun r = f.run();
  KZ_CHECK_EQ(r.load_bias, uint64_t(0x5000));
  KZ_CHECK_EQ(r.stats.bias_mismatches, uint64_t(1));
}

void test_filtering_and_side() {
  Fixture f;
  // The side-band stream is system-wide: process 7 maps the traced file and runs; process 8 does not.
  f.cpu0.mmap2(f.side0, 7, 7, 0x5000, 0x1000, 0x1000, 254, 0, 4242, "/bin/traced", 10);
  f.cpu0.mmap2(f.side0, 7, 7, 0x7000, 0x9000, 0, 254, 0, 999, "/lib/libc.so", 11);
  f.cpu0.mmap2(f.side0, 8, 8, 0x5000, 0x1000, 0x1000, 254, 0, 5555, "/bin/other", 12);
  f.cpu0.fork(f.side0, 7, 6, 7, 6, 9);
  f.cpu0.comm(f.side0, 7, 7, "traced", 10, true);
  f.cpu0.fork(f.side0, 8, 6, 8, 6, 12);
  f.cpu0.comm(f.side0, 8, 8, "other", 13, true);
  f.cpu1.fork(f.side1, 7, 7, 9, 7, 20);
  f.cpu1.comm(f.side1, 7, 9, "WORKER", 21);
  f.cpu1.exit_(f.side1, 7, 7, 9, 7, 30);
  f.cpu1.exit_(f.side1, 8, 6, 8, 6, 31);
  f.entry_sample(f.cpu1, 7, 9, 25, 5, 1);
  DecodedRun r = f.run();
  KZ_CHECK_EQ(r.maps.size(), size_t(1));
  KZ_CHECK_EQ(r.maps[0].path, std::string("/bin/traced"));
  KZ_CHECK_EQ(r.maps[0].rec.pid, 7u);
  KZ_CHECK_EQ(r.maps[0].rec.addr, uint64_t(0x5000));
  KZ_CHECK_EQ(r.maps[0].rec.pgoff, uint64_t(0x1000));
  KZ_CHECK_EQ(r.stats.maps_dropped, uint64_t(2));
  KZ_CHECK_EQ(r.side.size(), size_t(5));  // fork 7, comm 7, fork 9, comm 9, exit 9 -- nothing about process 8
  KZ_CHECK_EQ(r.stats.side_dropped, uint64_t(3));
  for (const SideEvent& s : r.side) KZ_CHECK_EQ(s.rec.pid, 7u);
  KZ_CHECK_EQ(r.side[0].rec.kind, uint8_t(KZ_SIDE_KIND_FORK));
  KZ_CHECK_EQ(r.side[1].rec.kind, uint8_t(KZ_SIDE_KIND_COMM));
  KZ_CHECK_EQ(r.side[1].comm, std::string("traced"));
  KZ_CHECK((r.side[1].rec.flags & KZ_SIDE_FLAG_COMM_EXEC) != 0);
  KZ_CHECK_EQ(r.side[2].rec.tid, 9u);
  KZ_CHECK_EQ(r.side[2].rec.ptid, 7u);
  KZ_CHECK_EQ(r.side[3].comm, std::string("WORKER"));
  KZ_CHECK_EQ(r.side[4].rec.kind, uint8_t(KZ_SIDE_KIND_EXIT));
  KZ_CHECK_EQ(r.side[0].rec.t_ns, uint64_t(9));

  // Without the traced file known, a process is traced only if it produced a sample.
  Fixture g;
  g.in.traced.known = false;
  g.cpu0.fork(g.side0, 7, 6, 7, 6, 9);
  g.cpu0.fork(g.side0, 8, 6, 8, 6, 9);
  g.entry_sample(g.cpu0, 7, 7, 25, 5, 1);
  DecodedRun h = g.run();
  KZ_CHECK_EQ(h.side.size(), size_t(1));
  KZ_CHECK_EQ(h.side[0].rec.pid, 7u);
}

void test_map_bias() {
  // The text segment at file offset 0x1000 is mapped at 0x500000001000: every site in it says the bias is 0x500000000000.
  Fixture f;
  f.in.sites[0].file_offset = 0x122a; f.in.sites[0].vaddr = 0x122a;
  f.in.sites[2].file_offset = 0x1372; f.in.sites[2].vaddr = 0x1372;
  f.cpu0.mmap2(f.side0, 7, 7, 0x500000001000, 0x1000, 0x1000, 254, 0, 4242, "/bin/traced", 10);
  f.cpu0.mmap2(f.side0, 7, 7, 0x500000003000, 0x1000, 0x3000, 254, 0, 4242, "/bin/traced", 11);  // a mapping with no site in it
  f.entry_sample(f.cpu0, 7, 7, 20, 1, 1, 0x500000000000);
  DecodedRun r = f.run();
  KZ_CHECK_EQ(r.maps.size(), size_t(2));
  KZ_CHECK_EQ(r.maps[0].rec.bias_known, 1u);
  KZ_CHECK_EQ(r.maps[0].rec.bias, uint64_t(0x500000000000));
  KZ_CHECK_EQ(r.maps[1].rec.bias_known, 0u);
  KZ_CHECK_EQ(r.maps[1].rec.bias, uint64_t(0));
  KZ_CHECK(r.bias_known);
  KZ_CHECK_EQ(r.load_bias, uint64_t(0x500000000000));  // from the sample's ip
  KZ_CHECK_EQ(r.stats.bias_mismatches, uint64_t(0));
  // A sample that disagrees with the mapping is a mismatch, not a silent choice.
  Fixture g;
  g.in.sites[0].file_offset = 0x122a; g.in.sites[0].vaddr = 0x122a;
  g.cpu0.mmap2(g.side0, 7, 7, 0x500000001000, 0x1000, 0x1000, 254, 0, 4242, "/bin/traced", 10);
  g.entry_sample(g.cpu0, 7, 7, 20, 1, 1, 0x600000000000);
  DecodedRun h = g.run();
  KZ_CHECK_EQ(h.stats.bias_mismatches, uint64_t(1));
  // Two sites in one mapping that disagree about the bias (a wrong vaddr in the plan).
  Fixture k;
  k.in.sites[0].file_offset = 0x122a; k.in.sites[0].vaddr = 0x122a;
  k.in.sites[2].file_offset = 0x1372; k.in.sites[2].vaddr = 0x1370;
  k.cpu0.mmap2(k.side0, 7, 7, 0x500000001000, 0x1000, 0x1000, 254, 0, 4242, "/bin/traced", 10);
  DecodedRun m = k.run();
  KZ_CHECK_EQ(m.maps[0].rec.bias_known, 0u);
  KZ_CHECK_EQ(m.stats.bias_mismatches, uint64_t(1));
}

void test_loss_records() {
  Fixture f;
  f.entry_sample(f.cpu0, 7, 20, 100, 1, 1);
  f.cpu0.lost(f.entry, 12, 7, 20, 150, 0);
  f.cpu1.lost(f.ret, 3, 7, 21, 140, 1);
  // a closed throttle interval of the entry event, an open one of the return event
  f.cpu0.throttle(f.entry, true, 200, 7, 20);
  f.cpu0.throttle(f.entry, false, 260, 7, 20);
  f.cpu1.throttle(f.ret, true, 300, 7, 20);
  DecodedRun r = f.run();
  KZ_CHECK_EQ(r.loss.size(), size_t(4));
  KZ_CHECK_EQ(r.stats.lost_reports, uint64_t(2));
  KZ_CHECK_EQ(r.stats.dropped_records, uint64_t(15));
  KZ_CHECK_EQ(r.stats.throttle_intervals, uint64_t(2));
  // ordered by time: lost@140 (cpu1), lost@150 (cpu0), throttle@200, throttle@300
  KZ_CHECK_EQ(r.loss[0].source, uint8_t(KZ_LOSS_SOURCE_PERF_RECORD_LOST));
  KZ_CHECK_EQ(r.loss[0].t_ns, uint64_t(140));
  KZ_CHECK_EQ(r.loss[0].cpu, uint16_t(1));
  KZ_CHECK_EQ(r.loss[0].count, uint64_t(3));
  KZ_CHECK_EQ(r.loss[0].tid, 21u);
  KZ_CHECK_EQ(r.loss[0].site, 1u);
  KZ_CHECK_EQ(r.loss[1].count, uint64_t(12));
  KZ_CHECK_EQ(r.loss[1].site, 0u);
  KZ_CHECK_EQ(r.loss[2].source, uint8_t(KZ_LOSS_SOURCE_PERF_THROTTLE));
  KZ_CHECK_EQ(r.loss[2].t_ns, uint64_t(200));
  KZ_CHECK_EQ(r.loss[2].count, uint64_t(60));   // the interval's length
  KZ_CHECK_EQ(r.loss[2].tseq_first, 0u);
  KZ_CHECK_EQ(r.loss[3].source, uint8_t(KZ_LOSS_SOURCE_PERF_THROTTLE));
  KZ_CHECK_EQ(r.loss[3].t_ns, uint64_t(300));
  KZ_CHECK_EQ(r.loss[3].tseq_first, 1u);        // still open at the end of the trace
  KZ_CHECK_EQ(r.loss[3].count, uint64_t(0));
}

void test_bad_samples_and_streams() {
  Fixture f;
  // A sample of an event that feeds no site (a layout whose site is out of range)
  EventLayout stray = kzsyn::probe_layout(50, 99, 1, 0);
  KZ_CHECK_OK(f.in.layouts.add(stray));
  Stream::SampleArgs a;
  a.ip = 1; a.pid = 7; a.tid = 7; a.time = 5; a.regs = {1};
  f.cpu0.sample(stray, a);
  // an entry sample missing the stack the site wants (layout asks for it, the record has dyn_size 4)
  Stream::SampleArgs b;
  b.ip = 0x5000 + 0x122a; b.pid = 7; b.tid = 7; b.time = 6; b.regs = {9, 9};
  b.stack = {1, 2, 3, 4};
  b.stack_size = 8;
  b.stack_dyn = 4;
  f.cpu0.sample(f.entry, b);
  DecodedRun r = f.run();
  KZ_CHECK_EQ(r.stats.samples_without_site, uint64_t(1));
  KZ_CHECK_EQ(r.events.size(), size_t(1));
  KZ_CHECK(r.events[0].flags & KZ_RAW_FLAG_TRUNC);
  KZ_CHECK_EQ(r.events[0].a[3], uint64_t(0));
  KZ_CHECK_EQ(r.stats.slots_truncated, uint64_t(1));

  // A malformed stream: the records before the damage are kept, the error is reported and counted.
  Fixture g;
  g.entry_sample(g.cpu0, 7, 20, 100, 1, 1);
  g.entry_sample(g.cpu0, 7, 20, 110, 2, 2);
  g.cpu0.bytes.push_back(0xff);  // trailing garbage shorter than a header
  g.entry_sample(g.cpu1, 7, 21, 105, 3, 3);
  kz::Status st;
  DecodedRun h = g.run(&st);
  KZ_CHECK_ERR(st, kz::Err::Truncated);
  KZ_CHECK_EQ(h.stats.stream_errors, uint64_t(1));
  KZ_CHECK_EQ(h.events.size(), size_t(3));
  KZ_CHECK_EQ(h.stream_status.size(), size_t(2));
  KZ_CHECK(!h.stream_status[0].ok());
  KZ_CHECK(h.stream_status[1].ok());
}

}  // namespace

int main() {
  test_order_and_tseq();
  test_load_bias_mismatch();
  test_filtering_and_side();
  test_map_bias();
  test_loss_records();
  test_bad_samples_and_streams();
  return kztest::finish("test_trace_decode_merge");
}

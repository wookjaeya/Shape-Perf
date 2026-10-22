// Known-answer test of the perf record decoder on bytes the kernel wrote (support/real_stream.hpp).
//
// The expected values below were computed from those bytes by a separate parse (python struct,
// outside the repository) and are written down as constants.  Facts about the captured program
// that the numbers must agree with: the probed function takes (event id, event type, app id)
// in edi, esi, edx; the worker thread called it with ids 5, 10, 5; the main thread called it
// once with id 10; at the entry probe (before the prologue's push) the word on top of the stack
// is the return address, and the next return sample's ip is that address.
#include <cstring>

#include "kz/trace/decode_perf.hpp"
#include "support/kz_test.hpp"
#include "support/real_stream.hpp"

using namespace kz::trace;

namespace {

LayoutTable real_layouts() {
  LayoutTable t;
  EventLayout side;
  side.id = kzreal::kIdSide;
  side.sample_type = perfabi::kSampleIdentifier | perfabi::kSampleTid | perfabi::kSampleTime | perfabi::kSampleCpu;
  KZ_CHECK_OK(t.add(side));
  const uint64_t base = perfabi::kSampleIdentifier | perfabi::kSampleIp | perfabi::kSampleTid | perfabi::kSampleTime |
                        perfabi::kSampleCpu | perfabi::kSampleRegsUser;
  EventLayout entry;
  entry.id = kzreal::kIdEntry;
  entry.sample_type = base | perfabi::kSampleStackUser;
  // edi esi edx ecx = perf registers 5, 4, 3, 2; rsp = 7
  entry.regs_user = (1u << 5) | (1u << 4) | (1u << 3) | (1u << 2) | (1u << 7);
  entry.stack_user = 8;
  entry.site = 0;
  KZ_CHECK_OK(t.add(entry));
  EventLayout ret;
  ret.id = kzreal::kIdRet;
  ret.sample_type = base;
  ret.regs_user = (1u << 0) | (1u << 7);
  ret.site = 1;
  KZ_CHECK_OK(t.add(ret));
  EventLayout point = ret;
  point.id = kzreal::kIdPoint;
  point.site = 2;
  KZ_CHECK_OK(t.add(point));
  return t;
}

void test_real_stream() {
  const LayoutTable layouts = real_layouts();
  std::vector<PerfRecord> recs;
  ParseStats st;
  size_t consumed = 0;
  KZ_CHECK_OK(parse_records(kzreal::stream(), kzreal::kStreamSize, 1, layouts, &recs, &st, &consumed));
  KZ_CHECK_EQ(consumed, kzreal::kStreamSize);
  KZ_CHECK_EQ(recs.size(), size_t(19));
  KZ_CHECK_EQ(st.records, uint64_t(19));
  KZ_CHECK_EQ(st.samples, uint64_t(9));
  KZ_CHECK_EQ(st.other, uint64_t(0));
  KZ_CHECK_EQ(st.unknown_id, uint64_t(0));

  int n_comm = 0, n_mmap = 0, n_fork = 0, n_exit = 0, n_sample = 0;
  for (const PerfRecord& r : recs) {
    KZ_CHECK_EQ(r.buffer_cpu, uint16_t(1));
    switch (r.kind) {
      case RecKind::Comm: ++n_comm; break;
      case RecKind::Mmap2: ++n_mmap; break;
      case RecKind::Fork: ++n_fork; break;
      case RecKind::Exit: ++n_exit; break;
      case RecKind::Sample: ++n_sample; break;
      default: KZ_CHECK(false);
    }
  }
  KZ_CHECK_EQ(n_comm, 2);
  KZ_CHECK_EQ(n_mmap, 5);  // four at exec, and the [uprobes] page at the first trap
  KZ_CHECK_EQ(n_fork, 1);
  KZ_CHECK_EQ(n_exit, 2);
  KZ_CHECK_EQ(n_sample, 9);

  // COMM by exec: the process renamed itself to the program's name.
  KZ_CHECK_EQ(recs[0].kind, RecKind::Comm);
  KZ_CHECK_EQ(recs[0].pid, uint32_t(28066));
  KZ_CHECK_EQ(recs[0].tid, uint32_t(28066));
  KZ_CHECK_EQ(recs[0].text, std::string("tgt"));
  KZ_CHECK((recs[0].misc & perfabi::kMiscCommExec) != 0);

  // The executable mapping of the program: dev 254:0, inode 1966900, offset 0x1000, 0x1000 long.
  KZ_CHECK_EQ(recs[1].kind, RecKind::Mmap2);
  KZ_CHECK_EQ(recs[1].addr, uint64_t(0x558d6017c000));
  KZ_CHECK_EQ(recs[1].len, uint64_t(0x1000));
  KZ_CHECK_EQ(recs[1].pgoff, uint64_t(0x1000));
  KZ_CHECK_EQ(recs[1].dev_major, uint32_t(254));
  KZ_CHECK_EQ(recs[1].dev_minor, uint32_t(0));
  KZ_CHECK_EQ(recs[1].ino, uint64_t(1966900));
  KZ_CHECK_EQ(recs[1].prot, uint32_t(5));  // r-x
  KZ_CHECK(recs[1].text.size() > 3 && recs[1].text.compare(recs[1].text.size() - 3, 3, "tgt") == 0);
  KZ_CHECK_EQ(recs[3].text, std::string("[vdso]"));
  KZ_CHECK_EQ(recs[3].ino, uint64_t(0));
  // The kernel maps its single-step page into the process at the first probe hit; it is an
  // executable mapping of no file.
  KZ_CHECK_EQ(recs[8].kind, RecKind::Mmap2);
  KZ_CHECK_EQ(recs[8].text, std::string("[uprobes]"));
  KZ_CHECK_EQ(recs[8].tid, uint32_t(28067));

  // FORK of the worker thread, then its rename.
  KZ_CHECK_EQ(recs[5].kind, RecKind::Fork);
  KZ_CHECK_EQ(recs[5].pid, uint32_t(28066));
  KZ_CHECK_EQ(recs[5].ppid, uint32_t(28066));
  KZ_CHECK_EQ(recs[5].tid, uint32_t(28067));
  KZ_CHECK_EQ(recs[5].ptid, uint32_t(28066));
  KZ_CHECK_EQ(recs[5].time, uint64_t(8217710120288));
  KZ_CHECK_EQ(recs[6].kind, RecKind::Comm);
  KZ_CHECK_EQ(recs[6].tid, uint32_t(28067));
  KZ_CHECK_EQ(recs[6].text, std::string("WORKER"));
  KZ_CHECK_EQ(recs[6].misc & perfabi::kMiscCommExec, 0);

  // The first sample: entry of the probed function in the worker (event id 5, type 2, app 0).
  const PerfRecord& s0 = recs[7];
  KZ_CHECK_EQ(s0.kind, RecKind::Sample);
  KZ_CHECK_EQ(s0.id, kzreal::kIdEntry);
  KZ_CHECK_EQ(s0.ip, uint64_t(0x558d6017c22a));
  KZ_CHECK_EQ(s0.pid, uint32_t(28066));
  KZ_CHECK_EQ(s0.tid, uint32_t(28067));
  KZ_CHECK_EQ(s0.time, uint64_t(8217710191248));
  KZ_CHECK_EQ(s0.cpu, uint32_t(1));
  KZ_CHECK_EQ(s0.regs_valid, uint64_t((1u << 5) | (1u << 4) | (1u << 3) | (1u << 2) | (1u << 7)));
  KZ_CHECK_EQ(s0.regs[2], uint64_t(0x558d6017d004));  // ecx: the format string
  KZ_CHECK_EQ(s0.regs[3], uint64_t(0));               // edx: the app id
  KZ_CHECK_EQ(s0.regs[4], uint64_t(2));               // esi: the event type
  KZ_CHECK_EQ(s0.regs[5], uint64_t(5));               // edi: the event id
  KZ_CHECK_EQ(s0.regs[7], uint64_t(0x7f1e04ffee88));  // rsp
  KZ_CHECK_EQ(s0.stack.size(), size_t(8));
  KZ_CHECK_EQ(s0.stack_dyn, uint64_t(8));
  const uint8_t ra[8] = {0x25, 0xc3, 0x17, 0x60, 0x8d, 0x55, 0x00, 0x00};  // 0x558d6017c325 little-endian
  KZ_CHECK(std::memcmp(s0.stack.data(), ra, 8) == 0);

  // The next sample is the return of that call: rax = 0xc2000005, ip = the return address read above.
  const PerfRecord& r0 = recs[9];
  KZ_CHECK_EQ(r0.id, kzreal::kIdRet);
  KZ_CHECK_EQ(r0.ip, uint64_t(0x558d6017c325));
  KZ_CHECK_EQ(r0.regs_valid, uint64_t((1u << 0) | (1u << 7)));
  KZ_CHECK_EQ(r0.regs[0], uint64_t(0xc2000005));
  KZ_CHECK_EQ(r0.regs[7], uint64_t(0x7f1e04ffee90));  // rsp after the return: the entry rsp plus 8
  KZ_CHECK_EQ(r0.stack.size(), size_t(0));

  // The point probe, the third call (app id 0x110003), the main thread's call.
  KZ_CHECK_EQ(recs[12].id, kzreal::kIdPoint);
  KZ_CHECK_EQ(recs[12].ip, uint64_t(0x558d6017c372));
  KZ_CHECK_EQ(recs[12].regs[0], uint64_t(0));
  KZ_CHECK_EQ(recs[13].id, kzreal::kIdEntry);
  KZ_CHECK_EQ(recs[13].regs[3], uint64_t(0x110003));
  KZ_CHECK_EQ(recs[15].kind, RecKind::Exit);
  KZ_CHECK_EQ(recs[15].tid, uint32_t(28067));
  KZ_CHECK_EQ(recs[15].ptid, uint32_t(28065));
  KZ_CHECK_EQ(recs[16].id, kzreal::kIdEntry);
  KZ_CHECK_EQ(recs[16].tid, uint32_t(28066));
  KZ_CHECK_EQ(recs[16].regs[5], uint64_t(10));
  KZ_CHECK_EQ(recs[16].regs[3], uint64_t(7));
  KZ_CHECK_EQ(recs[18].kind, RecKind::Exit);
  KZ_CHECK_EQ(recs[18].tid, uint32_t(28066));

  // The load bias from the mapping and from the entry sample agree: the entry probe is the
  // function at file offset 0x122a (text segment at file offset 0x1000).
  const uint64_t bias_from_map = recs[1].addr - recs[1].pgoff;
  KZ_CHECK_EQ(s0.ip - 0x122a, bias_from_map);

  // Every prefix of the stream either parses fully or stops at a record boundary with a
  // Truncated status, and never reads outside its buffer (the sanitizers watch).
  for (size_t cut = 0; cut <= kzreal::kStreamSize; ++cut) {
    std::vector<uint8_t> copy(kzreal::stream(), kzreal::stream() + cut);
    std::vector<PerfRecord> part;
    size_t used = 0;
    kz::Status s = parse_records(copy.data(), copy.size(), 0, layouts, &part, nullptr, &used);
    if (s.ok())
      KZ_CHECK_EQ(used, cut);
    else
      KZ_CHECK_EQ(s.code, kz::Err::Truncated);
    KZ_CHECK(used <= cut);
  }
}

}  // namespace

int main() {
  test_real_stream();
  return kztest::finish("test_trace_decode_real");
}

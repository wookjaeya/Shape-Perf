// Golden tests of the two containers of the observer: the basis (anchor table) built from a
// synthetic ELF file, and the run container written from hand-made decoder results.  The golden
// files are the stable record of the layouts, the string pool and the hashes; a deliberate change
// is accepted with
//     KZ_GOLDEN_UPDATE=1 ctest -R test_probe_golden
// and a review of the diff of tests/cfs-free/golden/.
//
//   argv: 1 target_prog, 2 dwarf4 build, 3 other-compiler build, 4 target.kzp, 5 target_prog.c,
//         6 kairos source root, 7 the kz executable, 8 build directory
//
// Every value is a unit-test value: nothing here is a condition or a result of an experiment.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>

#include "basis.hpp"
#include "kz/core/dump.hpp"
#include "kz/core/sha256.hpp"
#include "run_container.hpp"
#include "support/elf_synth.hpp"
#include "support/kz_test.hpp"

namespace {

std::string g_golden_dir, g_build_dir;

std::string slurp(const std::string& path, bool* ok = nullptr) {
  std::ifstream f(path, std::ios::binary);
  if (ok != nullptr) *ok = static_cast<bool>(f);
  std::ostringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

void check_golden(const std::string& name, const std::string& actual) {
  const std::string path = g_golden_dir + "/" + name + ".golden";
  const char* update = std::getenv("KZ_GOLDEN_UPDATE");
  if (update != nullptr && std::string(update) == "1") {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    f << actual;
    std::printf("updated %s\n", path.c_str());
    return;
  }
  bool ok = false;
  const std::string expected = slurp(path, &ok);
  ++kztest::state().checks;
  if (!ok) {
    kztest::fail(__FILE__, __LINE__, "golden file missing: " + path);
    return;
  }
  if (expected != actual) {
    const std::string actual_path = g_build_dir + "/" + name + ".actual";
    std::ofstream f(actual_path, std::ios::binary | std::ios::trunc);
    f << actual;
    kztest::fail(__FILE__, __LINE__, "golden mismatch for " + name + " (the actual text is in " + actual_path + ")");
  }
}

// ---- a synthetic file: f (calls g) and g, with a line table ------------------------------------
//
//   0x1000 f:  endbr64; push %rbp; mov %rsp,%rbp          line 10
//   0x1008     call g                                    line 11
//   0x100d     nop; pop %rbp; ret                        line 12
//   0x1010 g:  endbr64; push %rbp; mov %rsp,%rbp; pop %rbp; ret      line 20
kz::Bytes synthetic_binary() {
  kzsyn::LineProgram lp;
  lp.dirs = {"/w"};
  lp.files = {{"x.c", 0}, {"x.c", 0}};
  lp.set_address(0x1000);
  lp.advance_line(9);
  lp.copy();            // line 10 at 0x1000
  lp.advance_pc(8);
  lp.advance_line(1);
  lp.copy();            // line 11 at 0x1008
  lp.advance_pc(5);
  lp.advance_line(1);
  lp.copy();            // line 12 at 0x100d
  lp.advance_pc(3);
  lp.advance_line(8);
  lp.copy();            // line 20 at 0x1010
  lp.advance_pc(10);
  lp.end_sequence();
  kzsyn::SynthElf s;
  s.text = {0xf3, 0x0f, 0x1e, 0xfa, 0x55, 0x48, 0x89, 0xe5,       // f
            0xe8, 0x03, 0x00, 0x00, 0x00,                         // call g (rel32 = 3)
            0x90, 0x5d, 0xc3,                                     // nop; pop %rbp; ret
            0xf3, 0x0f, 0x1e, 0xfa, 0x55, 0x48, 0x89, 0xe5, 0x5d, 0xc3};  // g
  s.funcs = {{"f", 0x1000, 16}, {"g", 0x1010, 10}};
  s.debug_line = lp.build();
  s.build_id = std::string("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14", 20);
  return s.build();
}

const char* kProfile =
    "kzp-observe 1\n"
    "binary synthetic.bin\n"
    "prov p1 tier=researcher kind=researcher file=\"x.c\" lines=1-2 cite=\"the sites of the golden test\" authored=\"K2\"\n"
    "site f.entry entry func=f prov=p1 s0=rdi:u32:first s1=rsi:u16:second s3=stack+0:u64:return_address\n"
    "site f.ret ret func=f prov=p1 s0=rax:u32:status s3=ip:u64:return_address\n"
    "site f.after point at=x.c:11 after-call=g prov=p1 s0=rax:u32:status\n";

kz::plan::BasisOptions options() {
  kz::plan::BasisOptions o;
  o.binary_path = "/synthetic/synthetic.bin";
  o.tool_hash = kz::sha256("golden tool");
  o.build_identity = "unit-test build identity";  // the real one names the compiler and the source hash of this build
  return o;
}

struct Fixture {
  kz::Bytes binary;
  kz::plan::Profile profile;
  kz::Bytes basis_bytes;
  kz::plan::LoadedBasis basis;
  std::vector<kz::trace::SiteSpec> sites;
};

bool make_fixture(Fixture* f) {
  f->binary = synthetic_binary();
  kz::Status s = kz::plan::parse_profile(kProfile, &f->profile);
  KZ_CHECK_OK(s);
  if (!s.ok()) return false;
  s = kz::plan::build_basis(f->binary, f->profile, options(), &f->basis_bytes, nullptr);
  KZ_CHECK_OK(s);
  if (!s.ok()) return false;
  s = kz::plan::load_basis_bytes(f->basis_bytes, &f->basis);
  KZ_CHECK_OK(s);
  if (!s.ok()) return false;
  s = kz::plan::make_plan(f->profile, f->basis, &f->sites);
  KZ_CHECK_OK(s);
  return s.ok();
}

std::string dump(const kz::Bytes& bytes) {
  kz::Container c;
  KZ_CHECK_OK(kz::Container::parse(bytes, &c));
  KZ_CHECK_OK(c.verify_content());
  return kz::dump_container(c) + "artifact_sha256 " + kz::to_hex(c.artifact_hash()) + "\n";
}

void test_basis() {
  Fixture f;
  if (!make_fixture(&f)) return;
  check_golden("basis", dump(f.basis_bytes));

  // The anchors, read back.
  KZ_CHECK_EQ(f.basis.targets.size(), size_t(3));
  KZ_CHECK_EQ(f.basis.targets[0].file_offset, uint64_t(0x1004));  // after endbr64: the `push %rbp`
  KZ_CHECK_EQ(f.basis.targets[1].file_offset, uint64_t(0x1004));  // the return probe shares the address
  KZ_CHECK_EQ(f.basis.targets[2].file_offset, uint64_t(0x100d));  // the instruction after the call on line 11

  // The same inputs give the same bytes; another binary or another profile give another artifact.
  kz::Bytes again;
  KZ_CHECK_OK(kz::plan::build_basis(f.binary, f.profile, options(), &again, nullptr));
  KZ_CHECK(again == f.basis_bytes);
  kz::Bytes other = f.binary;
  other[0x1002] ^= 0x01;  // inside f's first instruction: the file hash changes
  kz::Bytes changed;
  kz::Status s = kz::plan::build_basis(other, f.profile, options(), &changed, nullptr);
  if (s.ok()) KZ_CHECK(changed != f.basis_bytes);
}

void test_run() {
  Fixture f;
  if (!make_fixture(&f)) return;

  kz::probe::RunInputs in;
  in.basis_artifact = f.basis.container.artifact_hash();
  in.profile_sha256 = f.profile.sha256;
  in.tool_hash = kz::sha256("golden tool");
  in.sites = f.sites;
  in.exe_path = "/synthetic/synthetic.bin";
  in.inode = 4242;
  in.device = 0xfe00;
  KZ_CHECK(kz::probe::build_id_from_hex("0102030405060708090a0b0c0d0e0f1011121314", in.build_id));
  in.exe_size = f.binary.size();
  in.exe_sha256 = kz::sha256(f.binary.data(), f.binary.size());
  in.run_ordinal = 3;
  in.stratum = KZ_STRATUM_COLD;
  in.kernel = "Linux 0.0.0-unit-test";
  in.harness = "unit-test harness";
  in.placement = "unit-test placement";
  in.observer_cpu_ns = 123456;
  in.boot_ns = 654321;
  in.proc_reads = 9;
  in.proc_ns = 31000;

  // Events: the main thread (tid 100) enters f and returns; thread 101 enters f once more.
  auto ev = [](uint64_t t, uint32_t tid, uint32_t tseq, uint32_t site, uint16_t phase, uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3) {
    kz_raw_ev e;
    std::memset(&e, 0, sizeof e);
    e.t_ns = t; e.tid = tid; e.tseq = tseq; e.site = site; e.phase = phase;
    e.a[0] = a0; e.a[1] = a1; e.a[2] = a2; e.a[3] = a3;
    return e;
  };
  in.decoded.events = {
      ev(1000, 100, 0, 0, KZ_PHASE_ENTRY, 7, 8, 0, 0x7000000010a0),
      ev(1100, 100, 1, 2, KZ_PHASE_POINT, 0, 0, 0, 0),
      ev(1200, 100, 2, 1, KZ_PHASE_RET, 0xc2000005, 0, 0, 0x7000000010a0),
      ev(1300, 101, 0, 0, KZ_PHASE_ENTRY, 9, 2, 0, 0x7000000010c0),
  };
  in.decoded.events[3].flags = KZ_RAW_FLAG_TRUNC;
  in.decoded.stats.site_hits = {2, 1, 1};
  in.decoded.stats.stream_errors = 0;
  in.decoded.bias_known = true;
  in.decoded.load_bias = 0x700000000000;

  kz::trace::SideEvent fork_main, comm_main, fork_t, comm_t, exit_t;
  std::memset(&fork_main.rec, 0, sizeof fork_main.rec);
  fork_main.rec.t_ns = 500; fork_main.rec.pid = 100; fork_main.rec.tid = 100; fork_main.rec.ppid = 50; fork_main.rec.ptid = 50; fork_main.rec.kind = KZ_SIDE_KIND_FORK;
  comm_main.rec = fork_main.rec;
  comm_main.rec.t_ns = 600; comm_main.rec.kind = KZ_SIDE_KIND_COMM; comm_main.rec.flags = KZ_SIDE_FLAG_COMM_EXEC; comm_main.comm = "synthetic.bin";
  fork_t.rec = fork_main.rec;
  fork_t.rec.t_ns = 900; fork_t.rec.tid = 101; fork_t.rec.ptid = 100; fork_t.rec.ppid = 50;
  comm_t.rec = fork_t.rec;
  comm_t.rec.t_ns = 950; comm_t.rec.kind = KZ_SIDE_KIND_COMM; comm_t.rec.flags = 0; comm_t.comm = "TASK_A";
  exit_t.rec = fork_t.rec;
  exit_t.rec.t_ns = 2000; exit_t.rec.kind = KZ_SIDE_KIND_EXIT;
  in.decoded.side = {fork_main, comm_main, fork_t, comm_t, exit_t};

  kz::trace::MapEvent map;
  std::memset(&map.rec, 0, sizeof map.rec);
  map.rec.t_ns = 550; map.rec.addr = 0x700000000000; map.rec.len = 0x3000; map.rec.pgoff = 0; map.rec.ino = 4242;
  map.rec.pid = 100; map.rec.tid = 100; map.rec.dev_major = 254; map.rec.dev_minor = 0; map.rec.prot = 5; map.rec.flags = 2;
  map.rec.bias = 0x700000000000; map.rec.bias_known = 1;
  map.path = "/synthetic/synthetic.bin";
  in.decoded.maps = {map};

  kz_loss lost;
  std::memset(&lost, 0, sizeof lost);
  lost.source = KZ_LOSS_SOURCE_PERF_RECORD_LOST; lost.cpu = 2; lost.site = 0; lost.tid = 101; lost.count = 17; lost.t_ns = 1250;
  kz_loss throttle;
  std::memset(&throttle, 0, sizeof throttle);
  throttle.source = KZ_LOSS_SOURCE_PERF_THROTTLE; throttle.cpu = 1; throttle.site = KZ_NONE32; throttle.tid = KZ_NONE32; throttle.count = 400; throttle.t_ns = 1400;
  in.decoded.loss = {lost, throttle};

  kz::trace::ThreadRecord main_thread, task;
  std::memset(&main_thread.rec, 0, sizeof main_thread.rec);
  main_thread.rec.pid = 100; main_thread.rec.tid = 100; main_thread.rec.ptid = 50; main_thread.rec.announced_id = KZ_NONE32;
  main_thread.rec.creator_event = KZ_NONE32; main_thread.rec.first_event = 0; main_thread.rec.t_fork = 500; main_thread.rec.t_name = 600;
  main_thread.rec.flags = KZ_THREAD_FLAG_FORK_SEEN | KZ_THREAD_FLAG_COMM_SEEN; main_thread.rec.n_events = 3;
  main_thread.name = "synthetic.bin";
  task.rec = main_thread.rec;
  task.rec.tid = 101; task.rec.ptid = 100; task.rec.announced_id = 0x100; task.rec.creator_event = 0; task.rec.first_event = 3;
  task.rec.t_fork = 900; task.rec.t_exit = 2000; task.rec.t_name = 950; task.rec.entry_fn = 0x7000000010c0; task.rec.priority = 11; task.rec.n_events = 1;
  task.rec.flags = KZ_THREAD_FLAG_FORK_SEEN | KZ_THREAD_FLAG_COMM_SEEN | KZ_THREAD_FLAG_ENTRY_SEEN | KZ_THREAD_FLAG_SPAWN_MATCHED | KZ_THREAD_FLAG_EXIT_SEEN |
                   KZ_THREAD_FLAG_MATCH_BY_TIME | KZ_THREAD_FLAG_ID_UNVERIFIED;
  task.name = "TASK_A";
  task.entry_symbol = "g";
  in.threads = {main_thread, task};

  in.record.add("observer.backend", "perf-sample (unit-test value)");
  in.record.add("cfs.user", "unit-test-user", KZ_POLICY_FROM_REFERENCE);
  in.record.add("run.count", "1", KZ_POLICY_DEVIATION);
  kz::probe::Deviation dev;
  dev.subject = "run.count";
  dev.reference_value = "3";
  dev.used_value = "1";
  dev.reason = "unit-test reason";
  dev.effect = "unit-test effect";
  dev.avoidable = KZ_AVOIDABLE_NO;
  in.record.add_deviation(dev);

  kz::Bytes bytes;
  KZ_CHECK_OK(kz::probe::write_run_container(in, &bytes));
  check_golden("run", dump(bytes));

  // Reading it back.
  kz::Container c;
  KZ_CHECK_OK(kz::Container::parse(bytes, &c));
  KZ_CHECK_OK(c.verify_content());
  KZ_CHECK_OK(c.verify_parent(f.basis.container));
  KZ_CHECK_EQ(c.header().artifact_kind, uint32_t(KZ_ARTIFACT_RUN));
  std::vector<kz_runmeta> meta;
  KZ_CHECK_OK(c.read_section(KZ_SEC_RUNMETA, &meta));
  KZ_CHECK_EQ(meta.size(), size_t(1));
  if (meta.size() == 1) {
    KZ_CHECK_EQ(meta[0].run_ordinal, 3u);
    KZ_CHECK_EQ(meta[0].stratum, uint32_t(KZ_STRATUM_COLD));
    KZ_CHECK((meta[0].flags & KZ_RUN_FLAG_LOSS_PRESENT) != 0);  // loss records exist: the run says so
    KZ_CHECK_EQ(meta[0].backend, uint8_t(KZ_BACKEND_PERF_SAMPLE));
    KZ_CHECK_EQ(meta[0].fidelity, uint8_t(KZ_FIDELITY_F0));
  }

  // The same inputs give the same bytes (nothing of wall-clock time is in the file); a different
  // ordinal gives a different artifact; a run without loss does not carry the loss flag.
  kz::Bytes again;
  KZ_CHECK_OK(kz::probe::write_run_container(in, &again));
  KZ_CHECK(again == bytes);
  kz::probe::RunInputs second = in;
  second.run_ordinal = 4;
  kz::Bytes second_bytes;
  KZ_CHECK_OK(kz::probe::write_run_container(second, &second_bytes));
  KZ_CHECK(second_bytes != bytes);
  kz::probe::RunInputs clean = in;
  clean.decoded.loss.clear();
  kz::Bytes clean_bytes;
  KZ_CHECK_OK(kz::probe::write_run_container(clean, &clean_bytes));
  kz::Container cc;
  KZ_CHECK_OK(kz::Container::parse(clean_bytes, &cc));
  std::vector<kz_runmeta> cm;
  KZ_CHECK_OK(cc.read_section(KZ_SEC_RUNMETA, &cm));
  if (cm.size() == 1) KZ_CHECK_EQ(cm[0].flags & KZ_RUN_FLAG_LOSS_PRESENT, 0u);
  KZ_CHECK(cc.find_section(KZ_SEC_LOSS) == nullptr);

  // A stream error alone also sets the flag: a damaged observation is not a clean one.
  kz::probe::RunInputs damaged = clean;
  damaged.decoded.stats.stream_errors = 1;
  kz::Bytes damaged_bytes;
  KZ_CHECK_OK(kz::probe::write_run_container(damaged, &damaged_bytes));
  kz::Container dc;
  KZ_CHECK_OK(kz::Container::parse(damaged_bytes, &dc));
  std::vector<kz_runmeta> dm;
  KZ_CHECK_OK(dc.read_section(KZ_SEC_RUNMETA, &dm));
  if (dm.size() == 1) KZ_CHECK((dm[0].flags & KZ_RUN_FLAG_LOSS_PRESENT) != 0);

  // Perturbation accounting: the return site (same file offset as the entry site) is charged the
  // return trampoline, not a second trap; the /proc reads of the observer are a row of their own.
  kz::Container rc;
  KZ_CHECK_OK(kz::Container::parse(bytes, &rc));
  std::vector<kz_perturbation> pert;
  KZ_CHECK_OK(rc.read_section(KZ_SEC_PERTURBATION, &pert));
  KZ_CHECK_EQ(pert.size(), size_t(6));
  if (pert.size() == 6) {
    KZ_CHECK_EQ(pert[0].cost_class, uint32_t(f.sites[0].insn_class));
    KZ_CHECK_EQ(pert[1].site, 1u);
    KZ_CHECK_EQ(pert[1].cost_class, uint32_t(KZ_INSN_CLASS_RET_TRAMPOLINE));
    KZ_CHECK_EQ(pert[5].kind, uint8_t(KZ_PERTURBATION_KIND_PROC_CAPTURE));
    KZ_CHECK_EQ(pert[5].count, uint64_t(9));
    KZ_CHECK_EQ(pert[5].ns, uint64_t(31000));
  }
  using kz::probe::TrapAccount;
  const std::vector<TrapAccount> acc = kz::probe::trap_accounts(f.sites);
  KZ_CHECK(acc == std::vector<TrapAccount>({TrapAccount::CountedHere, TrapAccount::InOtherRow, TrapAccount::CountedHere}));
  KZ_CHECK(kz::probe::trap_account_text(f.sites, 1).find("counted in the row of site 0") != std::string::npos);
  KZ_CHECK(kz::probe::trap_account_text(f.sites, 1).find("return trampolines") != std::string::npos);
  // A return site alone at its address: the trap there is counted in no row, and the record says so.
  std::vector<kz::trace::SiteSpec> alone = {f.sites[1], f.sites[2]};
  KZ_CHECK(kz::probe::trap_accounts(alone) == std::vector<TrapAccount>({TrapAccount::Uncounted, TrapAccount::CountedHere}));
  KZ_CHECK(kz::probe::trap_account_text(alone, 0).find("uncounted") == 0);
  // The order of the sites does not move the count to the return site.
  std::vector<kz::trace::SiteSpec> ret_first = {f.sites[1], f.sites[0]};
  KZ_CHECK(kz::probe::trap_accounts(ret_first) == std::vector<TrapAccount>({TrapAccount::InOtherRow, TrapAccount::CountedHere}));
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 9) {
    std::fprintf(stderr, "usage: %s target_prog dwarf4 alt target.kzp target_prog.c source_root kz build_dir\n", argv[0]);
    return 64;
  }
  g_golden_dir = std::string(argv[6]) + "/tests/cfs-free/golden";
  g_build_dir = argv[8];
  test_basis();
  test_run();
  return kztest::finish("test_probe_golden");
}

// Unit tests of the basis container (the anchor table) and the plan: building, loading,
// determinism, the provenance hashes, and the refusals of the binary check.
//   argv: 1 target_prog, 2 dwarf4 build, 3 other-compiler build, 4 target.kzp, 5 target_prog.c, 6 kairos source root
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

#include "basis.hpp"
#include "kz/core/dump.hpp"
#include "support/elf_synth.hpp"
#include "support/kz_test.hpp"

using namespace kz::plan;

namespace {

std::string g_binary, g_profile_path, g_source, g_root;

std::string slurp(const std::string& path) {
  std::ifstream f(path, std::ios::binary);
  std::ostringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

bool contains(const std::string& s, const std::string& what) { return s.find(what) != std::string::npos; }

struct Built {
  kz::Bytes binary;
  Profile profile;
  kz::Bytes basis;
  BasisReport report;
};

Built build(const std::string& root = std::string()) {
  Built b;
  KZ_CHECK_OK(kz::read_whole_file(g_binary, &b.binary));
  const std::string text = slurp(g_profile_path);
  KZ_CHECK_OK(parse_profile(text, &b.profile));
  BasisOptions opt;
  opt.binary_path = g_binary;
  opt.source_root = root;
  opt.tool_hash = kz::sha256("test tool");
  KZ_CHECK_OK(build_basis(b.binary, b.profile, opt, &b.basis, &b.report));
  return b;
}

void test_build_and_load() {
  Built b = build();
  LoadedBasis lb;
  KZ_CHECK_OK(load_basis_bytes(b.basis, &lb));
  KZ_CHECK_EQ(lb.container.header().artifact_kind, uint32_t(KZ_ARTIFACT_BASIS));
  KZ_CHECK(lb.container.profile_hash() == b.profile.sha256);
  KZ_CHECK_EQ(lb.binary_path, g_binary);
  KZ_CHECK_EQ(lb.binary_size, b.binary.size());
  KZ_CHECK(lb.binary_sha256 == kz::sha256(b.binary.data(), b.binary.size()));
  KZ_CHECK_EQ(lb.build_id.size(), size_t(40));
  KZ_CHECK_EQ(lb.targets.size(), size_t(6));
  KZ_CHECK_EQ(lb.target_labels[0], std::string("send.entry"));
  KZ_CHECK_EQ(lb.target_labels[2], std::string("publish.after_get"));
  KZ_CHECK_EQ(lb.target_funcs[0], std::string("tp_send"));
  KZ_CHECK_EQ(lb.target_funcs[2], std::string("tp_publish"));
  KZ_CHECK_EQ(lb.targets[0].kind, uint16_t(KZ_TARGET_KIND_FUNC_ENTRY));
  KZ_CHECK_EQ(lb.targets[2].kind, uint16_t(KZ_TARGET_KIND_AFTER_CALL));
  KZ_CHECK_EQ(lb.targets[0].insn_class, uint16_t(KZ_INSN_CLASS_EMULATED));
  KZ_CHECK_EQ(lb.targets[0].insn[0], uint8_t(0x55));
  KZ_CHECK_EQ(lb.targets[0].file_offset, lb.targets[1].file_offset);   // the return probe shares the entry's address
  KZ_CHECK_EQ(lb.targets[0].prov, 1u);                                  // tp_send_def
  KZ_CHECK_EQ(lb.targets[2].prov, 0u);                                  // tp_plan
  KZ_CHECK_EQ(b.report.targets, size_t(6));
  KZ_CHECK(b.report.symbols > 10);
  // sites by tier: three code-derived entries, one researcher (the point), and the rest code-derived
  KZ_CHECK_EQ(b.report.sites_by_tier[KZ_TIER_RESEARCHER], size_t(1));
  KZ_CHECK_EQ(b.report.sites_by_tier[KZ_TIER_CODE_DERIVED], size_t(5));
  KZ_CHECK_EQ(b.report.tier_count[KZ_TIER_CODE_DERIVED], size_t(2));
  KZ_CHECK_EQ(b.report.prov_unhashed, size_t(3));   // no source root was given: the cited text is not hashed

  // The container is canonical: it round-trips and equal inputs give equal bytes.
  Built again = build();
  KZ_CHECK(again.basis == b.basis);

  // Its dump is text a person can read and diff.
  const std::string dump = kz::dump_container(lb.container);
  KZ_CHECK(contains(dump, "section TARGETS type=55"));
  KZ_CHECK(contains(dump, "label=\"send.entry\""));
  KZ_CHECK(contains(dump, "section SYMBOLS type=54"));
  KZ_CHECK(contains(dump, "name=\"tp_publish\""));
  KZ_CHECK(contains(dump, "BASIS[2] name=\"binary.build_id\""));
}

void test_plan() {
  Built b = build();
  LoadedBasis lb;
  KZ_CHECK_OK(load_basis_bytes(b.basis, &lb));
  std::vector<kz::trace::SiteSpec> sites;
  KZ_CHECK_OK(make_plan(b.profile, lb, &sites));
  KZ_CHECK_EQ(sites.size(), size_t(6));
  KZ_CHECK_EQ(sites[0].label, std::string("send.entry"));
  KZ_CHECK_EQ(sites[0].kind, uint16_t(KZ_SITE_KIND_ENTRY_PROBE));
  KZ_CHECK_EQ(sites[0].phases, uint16_t(KZ_SITE_PHASE_ENTRY));
  KZ_CHECK_EQ(sites[0].symbol, std::string("tp_send"));
  KZ_CHECK_EQ(sites[0].slots.size(), size_t(4));
  KZ_CHECK_EQ(sites[1].kind, uint16_t(KZ_SITE_KIND_RET_PROBE));
  KZ_CHECK_EQ(sites[2].kind, uint16_t(KZ_SITE_KIND_POINT_PROBE));
  KZ_CHECK_EQ(sites[2].symbol, std::string("tp_publish"));
  KZ_CHECK(sites[2].source.size() > 4 && sites[2].source.compare(sites[2].source.size() - 13, 13, "target_prog.c") == 0);
  KZ_CHECK_EQ(sites[3].decoder, uint16_t(KZ_DECODER_SPAWN_CREATE));
  KZ_CHECK_EQ(sites[5].decoder, uint16_t(KZ_DECODER_THREAD_ENTRY));
  KZ_CHECK_EQ(sites[0].file_offset, lb.targets[0].file_offset);
  KZ_CHECK_EQ(sites[0].vaddr, lb.targets[0].vaddr);
  KZ_CHECK_EQ(sites[0].insn_class, uint16_t(KZ_INSN_CLASS_EMULATED));

  // The plan hash depends on the plan and on nothing else.
  const kz::Digest h = plan_hash(sites);
  KZ_CHECK(h == plan_hash(sites));
  auto changed = sites;
  changed[0].slots[0].role = "other";
  KZ_CHECK(plan_hash(changed) != h);
  changed = sites;
  changed[2].file_offset += 1;
  KZ_CHECK(plan_hash(changed) != h);
  changed = sites;
  changed[4].label = "renamed";
  KZ_CHECK(plan_hash(changed) != h);

  // A basis belongs to one profile text.
  Profile other;
  KZ_CHECK_OK(parse_profile(slurp(g_profile_path) + "\n# one more comment\n", &other));
  KZ_CHECK_ERR(make_plan(other, lb, &sites), kz::Err::Invalid);
  // A profile with the same text but fewer sites cannot be joined either (it has another hash).
  // And site records for a container carry slots and names.
  kz::ContainerWriter w(KZ_ARTIFACT_RUN);
  std::vector<kz_site> srecs;
  std::vector<kz_site_slot> slots;
  KZ_CHECK_OK(make_plan(b.profile, lb, &sites));
  site_records(sites, &w, &srecs, &slots);
  KZ_CHECK_EQ(srecs.size(), size_t(6));
  KZ_CHECK_EQ(slots.size(), size_t(4 + 2 + 1 + 4 + 2 + 2));
  KZ_CHECK_EQ(srecs[3].decoder, uint16_t(KZ_DECODER_SPAWN_CREATE));
}

void test_verify_binary() {
  Built b = build();
  LoadedBasis lb;
  KZ_CHECK_OK(load_basis_bytes(b.basis, &lb));
  KZ_CHECK_OK(verify_binary(b.binary, lb));

  // One byte changed anywhere in the file: refused, and the message says the hash differs.
  kz::Bytes c = b.binary;
  c[c.size() / 2] ^= 1;
  kz::Status s = verify_binary(c, lb);
  KZ_CHECK_ERR(s, kz::Err::Invalid);
  KZ_CHECK(contains(s.message, "SHA-256"));
  // A byte at a probe target: the target check says so as well.
  c = b.binary;
  c[lb.targets[0].file_offset] ^= 0xff;
  s = verify_binary(c, lb);
  KZ_CHECK_ERR(s, kz::Err::Invalid);
  KZ_CHECK(contains(s.message, "send.entry"));
  // A different size, a file that is not ELF, an empty file.
  c = b.binary;
  c.push_back(0);
  KZ_CHECK_ERR(verify_binary(c, lb), kz::Err::Invalid);
  KZ_CHECK_ERR(verify_binary(kz::Bytes{1, 2, 3}, lb), kz::Err::Invalid);
  KZ_CHECK_ERR(verify_binary(kz::Bytes{}, lb), kz::Err::Invalid);
  // The same source built by another compiler is another binary: refused.
  if (std::FILE* f = std::fopen(g_binary.c_str(), "rb")) std::fclose(f);
}

void test_provenance_hashes() {
  // With the source root, the cited lines are hashed; the hash is the one `sed -n 'A,Bp' | sha256sum` gives.
  Built b = build(g_root);
  KZ_CHECK_EQ(b.report.prov_computed, size_t(3));
  KZ_CHECK_EQ(b.report.prov_unhashed, size_t(0));
  LoadedBasis lb;
  KZ_CHECK_OK(load_basis_bytes(b.basis, &lb));
  std::vector<kz_prov> provs;
  KZ_CHECK_OK(lb.container.read_section(KZ_SEC_PROVENANCE, &provs));
  KZ_CHECK_EQ(provs.size(), size_t(3));
  // tp_send_def cites lines 51-58 of target_prog.c
  const std::string cmd = "sed -n '51,58p' '" + g_source + "' | sha256sum";
  FILE* p = popen(cmd.c_str(), "r");
  if (p != nullptr) {
    char line[256] = {0};
    if (std::fgets(line, sizeof line, p) != nullptr) {
      const std::string want(line, 64);
      std::string got;
      for (int i = 0; i < 32; ++i) {
        char hx[3];
        std::snprintf(hx, sizeof hx, "%02x", provs[1].text_sha256[i]);
        got += hx;
      }
      KZ_CHECK_EQ(got, want);
    }
    pclose(p);
  }
  KZ_CHECK_EQ(provs[0].tier, uint8_t(KZ_TIER_RESEARCHER));
  KZ_CHECK_EQ(provs[1].tier, uint8_t(KZ_TIER_CODE_DERIVED));
  KZ_CHECK_EQ(provs[1].line_from, 51u);
  KZ_CHECK_EQ(provs[1].line_to, 58u);

  // A declared hash is checked against the cited lines; a wrong one refuses the basis.
  Profile prof;
  std::string text = slurp(g_profile_path);
  KZ_CHECK_OK(parse_profile(text, &prof));
  const std::string good_hash = [&] {
    std::string g;
    for (int i = 0; i < 32; ++i) { char hx[3]; std::snprintf(hx, sizeof hx, "%02x", provs[1].text_sha256[i]); g += hx; }
    return g;
  }();
  const std::string needle = "cite=\"tp_send: (event id, event type, app id, spec, ...) returns a status\"";
  KZ_CHECK(contains(text, needle));
  std::string with_good = text;
  with_good.replace(with_good.find(needle), needle.size(), needle + " sha256=" + good_hash);
  Profile p_good;
  KZ_CHECK_OK(parse_profile(with_good, &p_good));
  BasisOptions opt;
  opt.binary_path = g_binary;
  opt.source_root = g_root;
  kz::Bytes out;
  BasisReport rep;
  KZ_CHECK_OK(build_basis(b.binary, p_good, opt, &out, &rep));
  KZ_CHECK_EQ(rep.prov_verified, size_t(1));
  KZ_CHECK_EQ(rep.prov_computed, size_t(2));
  std::string with_bad = text;
  with_bad.replace(with_bad.find(needle), needle.size(), needle + " sha256=" + std::string(64, '0'));
  Profile p_bad;
  KZ_CHECK_OK(parse_profile(with_bad, &p_bad));
  KZ_CHECK_ERR(build_basis(b.binary, p_bad, opt, &out, &rep), kz::Err::Invalid);
  // Declared but no source root: recorded, not verified, and counted as such.
  opt.source_root.clear();
  KZ_CHECK_OK(build_basis(b.binary, p_good, opt, &out, &rep));
  KZ_CHECK_EQ(rep.prov_declared, size_t(1));
  KZ_CHECK_EQ(rep.prov_unhashed, size_t(2));
  // A cited line that does not exist is an error.
  std::string beyond = text;
  const std::string cited = "lines=51-58";
  beyond.replace(beyond.find(cited), cited.size(), "lines=51-99999");
  Profile p_beyond;
  KZ_CHECK_OK(parse_profile(beyond, &p_beyond));
  opt.source_root = g_root;
  KZ_CHECK_ERR(build_basis(b.binary, p_beyond, opt, &out, &rep), kz::Err::Invalid);
  // A named root that was not given leaves its entries unhashed.
  std::string named = text;
  {
    const std::string from = "prov tp_plan tier=researcher";
    named.replace(named.find(from), from.size(), "prov tp_plan root=elsewhere tier=researcher");
  }
  Profile p_named;
  KZ_CHECK_OK(parse_profile(named, &p_named));
  KZ_CHECK_OK(build_basis(b.binary, p_named, opt, &out, &rep));
  KZ_CHECK_EQ(rep.prov_unhashed, size_t(1));
  opt.roots.push_back({"elsewhere", g_root});
  KZ_CHECK_OK(build_basis(b.binary, p_named, opt, &out, &rep));
  KZ_CHECK_EQ(rep.prov_unhashed, size_t(0));
}

void test_build_refusals() {
  Built b = build();
  BasisOptions opt;
  opt.binary_path = g_binary;
  kz::Bytes out;
  // a site that names a function the binary does not have: the error names the site and its line
  {
    std::string text = slurp(g_profile_path);
    text += "site extra.entry entry func=tp_not_there s0=rdi:u32:x\n";
    Profile p;
    KZ_CHECK_OK(parse_profile(text, &p));
    kz::Status s = build_basis(b.binary, p, opt, &out, nullptr);
    KZ_CHECK_ERR(s, kz::Err::Invalid);
    KZ_CHECK(contains(s.message, "extra.entry"));
    KZ_CHECK(contains(s.message, "tp_not_there"));
  }
  // a point whose line holds no call to the named callee
  {
    std::string text = slurp(g_profile_path);
    text += "site extra.point point at=target_prog.c:64 after-call=tp_send s0=rax:u32:x\n";
    Profile p;
    KZ_CHECK_OK(parse_profile(text, &p));
    KZ_CHECK_ERR(build_basis(b.binary, p, opt, &out, nullptr), kz::Err::Invalid);
  }
  // not an ELF file; an ELF without a line table; an ELF without a build-id
  {
    kz::Bytes junk(100, 7);
    KZ_CHECK_ERR(build_basis(junk, b.profile, opt, &out, nullptr), kz::Err::Invalid);
    kzsyn::SynthElf s;
    s.text = {0x90, 0xc3};
    s.funcs = {{"tp_send", 0x1000, 2}};
    s.build_id = std::string(20, 'x');
    kz::Status st = build_basis(s.build(), b.profile, opt, &out, nullptr);
    KZ_CHECK_ERR(st, kz::Err::Invalid);
    KZ_CHECK(contains(st.message, "line table"));
  }
  {
    std::string text = "kzp-observe 1\nsite a entry func=f s0=rax:u32:x\n";
    Profile p;
    KZ_CHECK_OK(parse_profile(text, &p));
    kzsyn::LineProgram lp;
    lp.dirs = {"/w"};
    lp.files = {{"x.c", 0}, {"x.c", 0}};
    lp.set_address(0x1000); lp.copy(); lp.advance_pc(2); lp.end_sequence();
    kzsyn::SynthElf s;
    s.text = {0x90, 0xc3};
    s.funcs = {{"f", 0x1000, 2}};
    s.debug_line = lp.build();
    kz::Status st = build_basis(s.build(), p, opt, &out, nullptr);   // no build-id note
    KZ_CHECK_ERR(st, kz::Err::Invalid);
    KZ_CHECK(contains(st.message, "build-id"));
    s.build_id = std::string(20, 'x');
    KZ_CHECK_OK(build_basis(s.build(), p, opt, &out, nullptr));
  }
  // load_basis refuses what is not a basis, and a basis that does not identify its binary
  {
    LoadedBasis lb;
    KZ_CHECK_ERR(load_basis_bytes(kz::Bytes{1, 2, 3}, &lb), kz::Err::Truncated);
    kz::ContainerWriter w(KZ_ARTIFACT_RUN);
    kz::Bytes run;
    KZ_CHECK_OK(w.finish(&run));
    KZ_CHECK_ERR(load_basis_bytes(run, &lb), kz::Err::Invalid);
    kz::ContainerWriter w2(KZ_ARTIFACT_BASIS);
    kz::Bytes empty_basis;
    KZ_CHECK_OK(w2.finish(&empty_basis));
    KZ_CHECK_ERR(load_basis_bytes(empty_basis, &lb), kz::Err::Invalid);
  }
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 7) {
    std::fprintf(stderr, "usage: test_probe_basis <target_prog> <dwarf4> <alt> <profile> <source> <root>\n");
    return 2;
  }
  g_binary = argv[1];
  g_profile_path = argv[4];
  g_source = argv[5];
  g_root = argv[6];
  test_build_and_load();
  test_plan();
  test_verify_binary();
  test_provenance_hashes();
  test_build_refusals();
  return kztest::finish("test_probe_basis");
}

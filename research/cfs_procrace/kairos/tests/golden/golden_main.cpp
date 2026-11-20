// Golden tests: canonical dumps compared with the files in tests/golden/data.
//
//   kz_golden <case> <golden-dir>
//
// With KZ_GOLDEN_UPDATE=1 in the environment the golden file is rewritten instead of
// compared; review the diff before accepting it, because the golden files are the stable
// record of the layouts, the dump format, the hashes and the rendering rules.
//
// All inputs are the synthetic fixtures of tests/unit/support: nothing here is a research
// condition or an experiment result.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "kz/core/dump.hpp"
#include "kz/core/identity.hpp"
#include "kz/core/verdict.hpp"
#include "support/fixtures.hpp"

namespace {

std::string case_layout() {
  std::string out = kz::layout_text();
  out += "fingerprint=" + kz::to_hex(kz::schema_fingerprint()) + "\n";
  return out;
}

// One pattern-filled record of every registered type, for two seeds: exercises the dump of
// every field of every record, including all enumerations that have a format.
std::string case_records() {
  const kz::Digest foreign = kz::sha256("golden foreign parent");
  std::string out;
  for (uint32_t seed = 1; seed <= 2; ++seed)
    for (const kz_record_desc* rec : kz::records()) {
      kz::Bytes r = kzfix::make_record(*rec, seed, nullptr, foreign);
      out += kz::dump_record(*rec, r.data()) + "\n";
    }
  return out;
}

std::string dump_of(const kz::Bytes& bytes) {
  kz::Container c;
  kz::Status s = kz::Container::parse(bytes, &c);
  if (!s.ok()) {
    std::fprintf(stderr, "fixture does not parse: %s\n", s.describe().c_str());
    std::exit(2);
  }
  kz::Status v = c.verify_content();
  if (!v.ok()) {
    std::fprintf(stderr, "fixture content check failed: %s\n", v.describe().c_str());
    std::exit(2);
  }
  return kz::dump_container(c);
}

kz::Digest hash_of(const kz::Bytes& b) { return kz::sha256(b.data(), b.size()); }

std::string case_facts() { return dump_of(kzfix::build_facts_fixture()); }
std::string case_run() { return dump_of(kzfix::build_run_fixture()); }
std::string case_verdicts() {
  return dump_of(kzfix::build_verdicts_fixture(hash_of(kzfix::build_facts_fixture()), hash_of(kzfix::build_run_fixture())));
}

// The tool hash is a fixed function of its inputs.  The inputs here are made up, so this file
// changes only when the definition of the hash changes (the framing, the order, the domain tag),
// never because a record layout or a source file of the tree changed.
std::string case_tool_hash() {
  const std::string core(64, 'a'), tool(64, 'b'), other(64, 'c');
  kz::ToolHashParts base;
  base.tool_name = "kz-unit-test";
  base.version = "0";
  base.abi_version = 1;
  base.schema_fingerprint = kz::sha256("layout");
  base.core_source = core;
  base.tool_source = tool;
  std::string out;
  auto line = [&](const std::string& name, const kz::ToolHashParts& p) { out += name + ": " + kz::to_hex(kz::tool_hash_from_parts(p)) + "\n"; };
  line("all inputs", base);
  kz::ToolHashParts p = base;
  p.tool_name = "kz-unit-test2";
  line("another tool name", p);
  p = base;
  p.version = "1";
  line("another version label", p);
  p = base;
  p.abi_version = 2;
  line("another ABI version", p);
  p = base;
  p.schema_fingerprint = kz::sha256("another layout");
  line("another schema fingerprint", p);
  p = base;
  p.core_source = other;
  line("another core source identity", p);
  p = base;
  p.tool_source = other;
  line("another tool source identity", p);
  p = base;
  p.tool_source = "";
  line("no tool source identity", p);
  return out;
}

kz_ref some_ref(uint32_t section) {
  kz_ref r = kzfix::zero<kz_ref>();
  r.section = section;
  r.index = 1;
  return r;
}

// How cells and ledgers are shown to a reader (SPEC 1.10 rendering rule, D20).
std::string case_render() {
  std::string out;
  auto line = [&](const std::string& name, const std::string& text) { out += name + ": " + text + "\n"; };
  kz_bound inf = kzfix::zero<kz_bound>();
  inf.h = KZ_BOUND_INFINITE;
  kz_bound h12 = inf;
  h12.h = 12;

  kz_verdict_cell c = kz::make_cell(KZ_SIGMA_U, KZ_MLEVEL_API);
  line("default", kz::render_cell(c));

  c.cls = KZ_CLS_PROTECTED;
  c.unknown_reason = KZ_UNKNOWN_REASON_NONE;
  c.safety_routes = kz::route_set(kz::route_set(0, KZ_ROUTE_NOT_BAD_SHIFT, KZ_ROUTE_CERT), KZ_ROUTE_NOT_NR_SHIFT, KZ_ROUTE_CERT);
  c.reach = KZ_REACH_PROVED;
  c.runs = some_ref(KZ_SEC_RUNMETA);
  c.certificate = some_ref(KZ_SEC_CERTIFICATES);
  c.proof_status = KZ_PROOF_PROVED;
  line("protected-by-certificate", kz::render_cell(c, nullptr, &inf));

  kz_verdict_cell m = kz::make_cell(KZ_SIGMA_U_INF, KZ_MLEVEL_IMPL);
  m.cls = KZ_CLS_PROTECTED;
  m.unknown_reason = KZ_UNKNOWN_REASON_NONE;
  m.safety_routes = kz::route_set(0, KZ_ROUTE_NOT_BAD_SHIFT, KZ_ROUTE_MODEL);
  m.reach = KZ_REACH_MODEL_B;
  line("protected-within-bound", kz::render_cell(m, nullptr, &h12));

  kz_verdict_cell f = kz::make_cell(KZ_SIGMA_U, KZ_MLEVEL_API);
  f.cls = KZ_CLS_FIXED;
  f.unknown_reason = KZ_UNKNOWN_REASON_NONE;
  f.flags = KZ_CELL_FLAG_DECIDES;
  f.safety_routes = kz::route_set(0, KZ_ROUTE_NOT_OK_SHIFT, KZ_ROUTE_CERT);
  f.proof_status = KZ_PROOF_UNPROVED;
  line("fixed-order-with-decision-route", kz::render_cell(f, nullptr, &inf));
  f.flags = 0;
  line("fixed-order-without-decision-route", kz::render_cell(f, nullptr, &inf));

  kz_verdict_cell s = kz::make_cell(KZ_SIGMA_E, KZ_MLEVEL_IMPL);
  s.cls = KZ_CLS_PROTECTED;
  s.unknown_reason = KZ_UNKNOWN_REASON_NONE;
  s.safety_routes = kz::route_set(0, KZ_ROUTE_NOT_BAD_SHIFT, KZ_ROUTE_CERT);
  s.proof_status = KZ_PROOF_UNPROVED;
  s.progress = KZ_PROGRESS_STALL_WITNESS;
  s.progress_kind = KZ_PROGRESS_KIND_PRE_USE;
  s.progress_realization = KZ_PROGRESS_REALIZATION_MODEL;
  s.witness_stall = some_ref(KZ_SEC_WITNESSES);
  line("protected-with-a-stall-witness", kz::render_cell(s));  // a consistent record: the progress result never changes the class

  kz_verdict_cell t = kz::make_cell(KZ_SIGMA_U, KZ_MLEVEL_API);
  t.cls = KZ_CLS_CONTESTED;
  t.unknown_reason = KZ_UNKNOWN_REASON_MODEL_FAULT;
  line("contested", kz::render_cell(t));

  // SPEC 1.10 rendering rule: bounds, retained statements and witnesses, stall witnesses.
  line("protected-without-a-bound", kz::render_cell(c));
  kz_verdict_cell r = kz::make_cell(KZ_SIGMA_U_INF, KZ_MLEVEL_IMPL);
  r.reach = KZ_REACH_UNREACHED_B;
  r.progress = KZ_PROGRESS_NO_STALL_B;
  r.progress_bounds = some_ref(KZ_SEC_BOUNDS);
  kz_bound h7 = inf;
  h7.h = 7;
  line("bounded-reach-and-progress", kz::render_cell(r, nullptr, &h12, &h7));
  line("bounded-reach-and-progress-without-bounds", kz::render_cell(r));
  kz_verdict_cell n = kz::make_cell(KZ_SIGMA_U, KZ_MLEVEL_API);
  n.unknown_reason = KZ_UNKNOWN_REASON_NO_DECISION_ROUTE;
  n.safety_routes = kz::route_set(0, KZ_ROUTE_NOT_OK_SHIFT, KZ_ROUTE_CERT);  // not-OK holds, but nothing decides
  n.certificate = some_ref(KZ_SEC_CERTIFICATES);
  n.realization = KZ_REALIZATION_CERT;
  n.proof_status = KZ_PROOF_UNPROVED;
  line("unknown-no-decision-route", kz::render_cell(n));
  kz_verdict_cell o = kz::make_cell(KZ_SIGMA_U, KZ_MLEVEL_API);
  o.unknown_reason = KZ_UNKNOWN_REASON_ONE_SIDED;
  o.witness_bad = some_ref(KZ_SEC_WITNESSES);
  line("unknown-one-sided", kz::render_cell(o));
  kz_verdict_cell d = kz::make_cell(KZ_SIGMA_U, KZ_MLEVEL_API);
  d.cls = KZ_CLS_SCHED_DEP;
  d.unknown_reason = KZ_UNKNOWN_REASON_NONE;
  d.witness_bad = some_ref(KZ_SEC_WITNESSES);
  d.witness_good = some_ref(KZ_SEC_WITNESSES);
  line("schedule-dependent", kz::render_cell(d));
  kz_verdict_cell k = kz::make_cell(KZ_SIGMA_E, KZ_MLEVEL_IMPL);
  k.progress = KZ_PROGRESS_STALL_WITNESS;
  k.progress_kind = KZ_PROGRESS_KIND_PRE_USE | KZ_PROGRESS_KIND_IN_USE;
  k.progress_flags = KZ_PROGRESS_FLAG_DEADLOCK_CYCLE | KZ_PROGRESS_FLAG_IDLE | KZ_PROGRESS_FLAG_TERMINAL_PENDING;
  k.progress_harm = KZ_PROGRESS_HARM_TERMINAL_PENDING_ABORT;
  k.progress_realization = KZ_PROGRESS_REALIZATION_MODEL;
  k.witness_stall = some_ref(KZ_SEC_WITNESSES);
  line("stall-witness-in-full", kz::render_cell(k, nullptr, nullptr, &h12));

  // Proof status of the lemmas a result rests on (PS2, PS3): the cell and the certificate carry
  // the weakest status of their ledger, and the text says so.
  auto lemma = [](uint8_t proof, bool with_source) {
    kz_trust x = kz::make_trust_entry(KZ_TRUST_PART_TRUSTED, KZ_TRUST_KIND_LEMMA);
    x.proof = proof;
    if (with_source) x.source = some_ref(KZ_SEC_PROVENANCE);
    return x;
  };
  std::vector<kz_trust> only_listed = {lemma(KZ_PROOF_UNPROVED, false), lemma(KZ_PROOF_UNPROVED, false)};
  only_listed[0].ident = 3;
  only_listed[1].ident = 1;
  std::vector<kz_trust> mixed = {lemma(KZ_PROOF_PROVED, true), lemma(KZ_PROOF_DEPENDS, false)};
  mixed[0].ident = 4;
  mixed[1].ident = 2;
  std::vector<kz_trust> all_proved = {lemma(KZ_PROOF_PROVED, true)};
  all_proved[0].ident = 5;
  const struct {
    const char* name;
    const std::vector<kz_trust>* entries;
  } ledgers[] = {{"lemmas-only-listed", &only_listed}, {"lemmas-mixed", &mixed}, {"lemmas-all-proved", &all_proved}};
  for (const auto& l : ledgers) {
    const kz::LedgerCounts counts = kz::count_ledger(*l.entries);
    kz_verdict_cell x = c;
    x.proof_status = kz::weakest_proof(*l.entries);
    line(l.name, kz::render_cell(x, &counts, &inf));
    kz_cert cert = kzfix::zero<kz_cert>();
    cert.proof_status = x.proof_status;
    line(std::string(l.name) + " acceptance", kz::render_acceptance(cert, &counts));
    line(std::string(l.name) + " labels", kz::ledger_label(kz::make_ledger(*l.entries, 0)) + " " + kz::trusted_label(counts) + " " + kz::lemmas_label(counts));
  }
  // A stored status is checked against the ledger it comes from, and a `proved` that was not
  // checked is not printed as plain proved.
  {
    const kz::LedgerCounts weak = kz::count_ledger(only_listed);
    kz_verdict_cell x = c;
    x.proof_status = KZ_PROOF_PROVED;  // claims more than its ledger supports
    line("proved-but-the-ledger-says-unproved", kz::render_cell(x, &weak, &inf));
    kz_cert cert = kzfix::zero<kz_cert>();
    cert.proof_status = KZ_PROOF_PROVED;
    line("proved-but-the-ledger-says-unproved acceptance", kz::render_acceptance(cert, &weak));
    line("proved-without-a-ledger", kz::render_cell(x, nullptr, &inf));
    line("proved-without-a-ledger acceptance", kz::render_acceptance(cert));
  }
  return out;
}

struct Case {
  const char* name;
  std::string (*run)();
};

const Case kCases[] = {
    {"layout", case_layout},       {"records", case_records},   {"container_facts", case_facts},
    {"container_run", case_run},   {"container_verdicts", case_verdicts}, {"render", case_render},
    {"tool_hash", case_tool_hash},
};

bool read_text(const std::string& path, std::string* out) {
  FILE* f = std::fopen(path.c_str(), "rb");
  if (f == nullptr) return false;
  char buf[4096];
  size_t n;
  out->clear();
  while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0) out->append(buf, n);
  std::fclose(f);
  return true;
}

bool write_text(const std::string& path, const std::string& text) {
  FILE* f = std::fopen(path.c_str(), "wb");
  if (f == nullptr) return false;
  bool ok = std::fwrite(text.data(), 1, text.size(), f) == text.size();
  return (std::fclose(f) == 0) && ok;
}

std::vector<std::string> lines_of(const std::string& s) {
  std::vector<std::string> v;
  size_t pos = 0;
  while (pos < s.size()) {
    size_t nl = s.find('\n', pos);
    if (nl == std::string::npos) nl = s.size();
    v.push_back(s.substr(pos, nl - pos));
    pos = nl + 1;
  }
  return v;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 3) {
    std::fprintf(stderr, "usage: kz_golden <case> <golden-dir>\n");
    return 2;
  }
  const Case* chosen = nullptr;
  for (const Case& c : kCases)
    if (std::strcmp(c.name, argv[1]) == 0) chosen = &c;
  if (chosen == nullptr) {
    std::fprintf(stderr, "unknown case %s\n", argv[1]);
    return 2;
  }
  const std::string actual = chosen->run();
  // Determinism: a second run gives the same text.
  if (chosen->run() != actual) {
    std::fprintf(stderr, "%s: output is not deterministic\n", chosen->name);
    return 1;
  }
  const std::string path = std::string(argv[2]) + "/" + chosen->name + ".golden";
  const char* update = std::getenv("KZ_GOLDEN_UPDATE");
  if (update != nullptr && std::strcmp(update, "1") == 0) {
    if (!write_text(path, actual)) {
      std::fprintf(stderr, "cannot write %s\n", path.c_str());
      return 2;
    }
    std::printf("%s: golden file written (%zu lines)\n", chosen->name, lines_of(actual).size());
    return 0;
  }
  std::string expected;
  if (!read_text(path, &expected)) {
    std::fprintf(stderr, "%s: golden file %s is missing (run with KZ_GOLDEN_UPDATE=1 and review it)\n", chosen->name, path.c_str());
    return 1;
  }
  if (expected == actual) {
    std::printf("%s: matches %s\n", chosen->name, path.c_str());
    return 0;
  }
  const std::vector<std::string> e = lines_of(expected), a = lines_of(actual);
  size_t i = 0;
  while (i < e.size() && i < a.size() && e[i] == a[i]) ++i;
  std::fprintf(stderr, "%s: differs from %s at line %zu\n  expected: %s\n  actual:   %s\n", chosen->name, path.c_str(), i + 1,
               i < e.size() ? e[i].c_str() : "<end>", i < a.size() ? a[i].c_str() : "<end>");
  const std::string actual_path = std::string(chosen->name) + ".actual";
  if (write_text(actual_path, actual)) std::fprintf(stderr, "  full actual output written to %s\n", actual_path.c_str());
  return 1;
}

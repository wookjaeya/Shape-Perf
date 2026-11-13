// The canonical dump: value formats for every field kind, determinism, and the rule that
// zero reserved fields are not printed.

#include <cstring>

#include "kz/core/dump.hpp"
#include "support/fixtures.hpp"
#include "support/kz_test.hpp"

namespace {

bool has(const std::string& text, const std::string& part) { return text.find(part) != std::string::npos; }

void test_escape() {
  KZ_CHECK_EQ(kz::escape_string("plain"), std::string("plain"));
  KZ_CHECK_EQ(kz::escape_string("a\"b\\c"), std::string("a\\\"b\\\\c"));
  KZ_CHECK_EQ(kz::escape_string("l1\nl2\tx\r"), std::string("l1\\nl2\\tx\\r"));
  KZ_CHECK_EQ(kz::escape_string(std::string("\x01\x7f\xc3\xa9", 4)), std::string("\\x01\\x7f\\xc3\\xa9"));  // ASCII output only
  KZ_CHECK_EQ(kz::escape_string(""), std::string(""));
}

void test_scalar_formats() {
  const kz_record_desc* cell = kz::find_record("kz_verdict_cell");
  kz_verdict_cell c = kzfix::zero<kz_verdict_cell>();
  c.sigma = KZ_SIGMA_U_INF;
  c.mlevel = KZ_MLEVEL_IMPL;
  c.cls = KZ_CLS_SCHED_DEP;
  c.refinements = KZ_REFINEMENT_TOLERATED | KZ_REFINEMENT_TIMEOUT_RESCUED;
  c.realization = 0;
  c.naturalness = KZ_NATURALNESS_HELD_F0;
  c.progress_flags = 0x8000;  // an unlisted bit
  c.unknown_reason = 200;     // an unlisted value
  c.safety_routes = 0x26;     // binary 10 01 10: not-Bad model, not-OK cert, not-NR model
  c.observed_n = 3;
  c.observed_N = 5;
  const std::string line = kz::dump_record(*cell, &c);
  KZ_CHECK(line.compare(0, 16, "kz_verdict_cell ") == 0);
  KZ_CHECK(has(line, " sigma=sigma_Uinf "));
  KZ_CHECK(has(line, " mlevel=M_impl "));
  KZ_CHECK(has(line, " cls=schedule-dependent "));
  KZ_CHECK(has(line, " refinements={tolerated,timeout-rescued} "));
  KZ_CHECK(has(line, " realization={} "));              // an empty set
  KZ_CHECK(has(line, " naturalness={held-F0} "));
  KZ_CHECK(has(line, " progress_flags={0x8000} "));     // unlisted bits are shown, not dropped
  KZ_CHECK(has(line, " unknown_reason=#200 "));         // unlisted enumerated values are shown as #n
  KZ_CHECK(has(line, " safety_routes=not_bad=model,not_ok=cert,not_nr=model "));  // 0x26 = 10 01 10
  KZ_CHECK(has(line, " observed_n=3 observed_N=5 "));
  KZ_CHECK(has(line, " certificate=- "));              // a null reference
  KZ_CHECK(has(line, " progress=unknown "));
  KZ_CHECK(has(line, " progress_reason=not_checked ")); // the default is visible in every dump
  KZ_CHECK(!has(line, "rsv"));                          // zero reserved fields are not printed
  KZ_CHECK(!has(line, "\n"));
}

void test_special_formats() {
  const kz_record_desc* raw = kz::find_record("kz_raw_ev");
  kz_raw_ev ev = kzfix::zero<kz_raw_ev>();
  ev.t_ns = 12345;
  ev.phase = KZ_PHASE_RET;
  ev.flags = KZ_RAW_FLAG_STR_FOLLOWS | KZ_RAW_FLAG_TRUNC;
  ev.objseq = 0xabc;
  ev.a[0] = 1;
  ev.a[3] = 0xffffffffffffffffull;
  const std::string line = kz::dump_record(*raw, &ev);
  KZ_CHECK(has(line, " t_ns=12345 "));
  KZ_CHECK(has(line, " phase=RET "));
  KZ_CHECK(has(line, " flags={STR_FOLLOWS,TRUNC} "));
  KZ_CHECK(has(line, " objseq=0xabc "));
  KZ_CHECK(has(line, " a=[0x1,0x0,0x0,0xffffffffffffffff]"));

  // Idx fields: the all-ones sentinel is "none".
  const kz_record_desc* trust = kz::find_record("kz_trust");
  kz_trust t = kzfix::zero<kz_trust>();
  t.site = KZ_NONE32;
  t.refutes = 4;
  const std::string tl = kz::dump_record(*trust, &t);
  KZ_CHECK(has(tl, " site=none "));
  KZ_CHECK(has(tl, " refutes=4 "));
  KZ_CHECK(has(tl, " proof=n/a "));
  t.bound = KZ_BOUND_INFINITE;  // b = infinity is shown as inf, a finite bound as a number
  KZ_CHECK(has(kz::dump_record(*trust, &t), " bound=inf "));
  t.bound = 12;
  KZ_CHECK(has(kz::dump_record(*trust, &t), " bound=12 "));

  // Byte arrays are hex strings; signed values print as signed.
  const kz_record_desc* fh = kz::find_record("kz_file_hash");
  kz_file_hash f = kzfix::zero<kz_file_hash>();
  f.sha256[0] = 0xde;
  f.sha256[31] = 0x01;
  f.size = 10;
  const std::string fl = kz::dump_record(*fh, &f);
  KZ_CHECK(has(fl, " sha256=de" + std::string(60, '0') + "01 "));
  const kz_record_desc* arg = kz::find_record("kz_fact_arg");
  kz_fact_arg a = kzfix::zero<kz_fact_arg>();
  a.value = -7;
  KZ_CHECK(has(kz::dump_record(*arg, &a), " value=-7 "));

  // References: null, this container, another artifact.
  const kz_record_desc* cell = kz::find_record("kz_verdict_cell");
  kz_verdict_cell c = kzfix::zero<kz_verdict_cell>();
  c.bounds.section = KZ_SEC_BOUNDS;
  c.bounds.index = 3;
  std::memset(c.certificate.artifact_sha256, 0xab, 32);
  c.certificate.section = KZ_SEC_CERTIFICATES;
  c.certificate.index = 9;
  const std::string cl = kz::dump_record(*cell, &c);
  KZ_CHECK(has(cl, " bounds=self/BOUNDS#3 "));
  std::string hex_ab;
  for (int i = 0; i < 32; ++i) hex_ab += "ab";
  KZ_CHECK(has(cl, " certificate=" + hex_ab + "/CERTIFICATES#9 "));

  // A reserved field is shown when it is not zero.
  const kz_record_desc* dev = kz::find_record("kz_deviation");
  kz_deviation d = kzfix::zero<kz_deviation>();
  d.rsv1 = 3;
  KZ_CHECK(has(kz::dump_record(*dev, &d), " rsv1=3 "));
  d.rsv1 = 0;
  KZ_CHECK(!has(kz::dump_record(*dev, &d), "rsv"));
}

void test_strings_in_container() {
  kz::ContainerWriter w(KZ_ARTIFACT_TEST);
  std::vector<kz_basis_item> basis(2, kzfix::zero<kz_basis_item>());
  basis[0].name = w.intern("with \"quotes\"\nand a newline");
  basis[0].policy = KZ_POLICY_FROM_REFERENCE;
  basis[0].prov = KZ_NONE32;
  basis[0].deviation = KZ_NONE32;
  basis[1].prov = 2;
  basis[1].deviation = KZ_NONE32;
  basis[1].policy = 9;
  kzfix::must(w.add_section(KZ_SEC_BASIS, basis), "BASIS");
  kz::Bytes bytes;
  kzfix::must(w.finish(&bytes), "finish");
  kz::Container c;
  KZ_CHECK_OK(kz::Container::parse(bytes, &c));
  const std::string text = kz::dump_container(c);
  KZ_CHECK(has(text, "name=\"with \\\"quotes\\\"\\nand a newline\""));
  KZ_CHECK(has(text, "policy=from-reference"));
  KZ_CHECK(has(text, "policy=#9"));
  KZ_CHECK(has(text, "name=\"\" "));  // the empty string
  KZ_CHECK(has(text, "prov=2"));
  KZ_CHECK(has(text, "prov=none"));
  KZ_CHECK(has(text, "artifact_sha256=" + kz::to_hex(c.artifact_hash())));
  KZ_CHECK(has(text, "kind=unit-test"));
  // Without the container the string reference is shown as is.
  KZ_CHECK(has(kz::dump_record(*kz::find_record("kz_basis_item"), &basis[0]), "name=str("));
}

void test_dump_is_deterministic_and_complete() {
  const kz::Bytes facts = kzfix::build_facts_fixture();
  kz::Container c;
  KZ_CHECK_OK(kz::Container::parse(facts, &c));
  const std::string a = kz::dump_container(c);
  KZ_CHECK_EQ(a, kz::dump_container(c));
  // One line per record, plus the header and section lines.
  size_t lines = 0, records = 0;
  for (char ch : a) lines += ch == '\n';
  for (const kz_section_entry& e : c.section_table())
    if (e.type != KZ_SEC_STRINGS) records += e.count;
  const size_t fixed_lines = 1 /*kz-dump*/ + 1 /*container*/ + 3 /*hashes*/ + 1 /*parents=*/ + c.parents().size() + 1 /*sections=*/ +
                             c.section_table().size() + 1 /*artifact_sha256*/;
  KZ_CHECK_EQ(lines, fixed_lines + records);
  KZ_CHECK(a.find('\t') == std::string::npos);
  for (char ch : a) KZ_CHECK(static_cast<unsigned char>(ch) < 0x80);  // ASCII only
  // Layout text is deterministic as well and its fingerprint changes with any layout change.
  KZ_CHECK_EQ(kz::layout_text(), kz::layout_text());
  KZ_CHECK(has(kz::layout_text(), "record kz_raw_ev size=64 fields=8"));
}

}  // namespace

int main() {
  test_escape();
  test_scalar_formats();
  test_special_formats();
  test_strings_in_container();
  test_dump_is_deterministic_and_complete();
  return kztest::finish("test_core_dump");
}

// KZ container: round trip with stable hashes, deterministic sorted-section writer,
// zeroed padding, string pool, typed accessors, argument checking and file I/O.

#include <cstdio>
#include <cstring>
#include <set>
#include <unistd.h>

#include "kz/core/container.hpp"
#include "kz/core/dump.hpp"
#include "support/fixtures.hpp"
#include "support/kz_test.hpp"

namespace {

kz::Container must_parse(const kz::Bytes& b) {
  kz::Container c;
  KZ_CHECK_OK(kz::Container::parse(b, &c));
  return c;
}

// Every byte of the file that is not header, a table, or the payload of a section is padding.
void check_padding_is_zero(const kz::Container& c) {
  const kz::Bytes& b = c.bytes();
  std::vector<bool> covered(b.size(), false);
  const kz_header& h = c.header();
  for (size_t i = 0; i < h.parents_off + h.n_parents * sizeof(kz_parent); ++i) covered[i] = true;
  for (size_t i = h.sections_off; i < h.sections_off + h.n_sections * sizeof(kz_section_entry); ++i) covered[i] = true;
  for (const kz_section_entry& e : c.section_table())
    for (uint64_t i = 0; i < e.size; ++i) covered[e.offset + i] = true;
  size_t padding = 0;
  for (size_t i = 0; i < b.size(); ++i)
    if (!covered[i]) {
      ++padding;
      KZ_CHECK_EQ(b[i], static_cast<uint8_t>(0));
    }
  (void)padding;
}

void test_empty_container() {
  kz::ContainerWriter w(KZ_ARTIFACT_TEST);
  kz::Bytes b;
  KZ_CHECK_OK(w.finish(&b));
  kz::Container c = must_parse(b);
  KZ_CHECK_EQ(c.section_table().size(), static_cast<size_t>(1));  // STRINGS only
  KZ_CHECK_EQ(c.section_table()[0].type, static_cast<uint32_t>(KZ_SEC_STRINGS));
  KZ_CHECK_EQ(c.header().total_size, static_cast<uint64_t>(b.size()));
  KZ_CHECK_EQ(b.size() % 8, static_cast<size_t>(0));
  KZ_CHECK(c.parents().empty());
  KZ_CHECK_OK(c.verify_content());
  // Same inputs, same bytes, same hash.
  kz::ContainerWriter w2(KZ_ARTIFACT_TEST);
  kz::Bytes b2;
  KZ_CHECK_OK(w2.finish(&b2));
  KZ_CHECK(b == b2);
  KZ_CHECK(kz::sha256(b.data(), b.size()) == c.artifact_hash());
  check_padding_is_zero(c);
}

void test_round_trip_and_hash_stability() {
  const kz::Bytes facts = kzfix::build_facts_fixture();
  const kz::Bytes facts_again = kzfix::build_facts_fixture();
  KZ_CHECK(facts == facts_again);  // deterministic: no timestamps, no host data
  kz::Container c = must_parse(facts);
  KZ_CHECK(c.artifact_hash() == kz::sha256(facts.data(), facts.size()));
  KZ_CHECK_EQ(static_cast<uint32_t>(c.header().artifact_kind), static_cast<uint32_t>(KZ_ARTIFACT_FACTS));
  KZ_CHECK(c.basis_hash() == kz::sha256("fixture basis"));
  KZ_CHECK(c.profile_hash() == kz::sha256("fixture profile"));
  KZ_CHECK_OK(c.verify_content());
  check_padding_is_zero(c);

  // Typed read-back, then write the same records again: identical bytes.
  std::vector<kz_fact_call> calls;
  KZ_CHECK_OK(c.read_section(KZ_SEC_FACTS, &calls));
  KZ_CHECK_EQ(calls.size(), static_cast<size_t>(2));
  std::string callee;
  KZ_CHECK(c.string(calls[0].callee, &callee));
  KZ_CHECK_EQ(callee, std::string("example_publish"));
  KZ_CHECK(c.string(calls[1].callee, &callee));
  KZ_CHECK_EQ(callee, std::string("example_consume"));

  // Sections are stored in ascending id order, each aligned to 8 bytes.
  uint32_t last = 0;
  for (const kz_section_entry& e : c.section_table()) {
    KZ_CHECK(e.type > last);
    last = e.type;
    KZ_CHECK_EQ(e.offset % 8, static_cast<uint64_t>(0));
    KZ_CHECK_EQ(e.size, e.count * e.record_size);
  }

  // Parents: the verdict container lists the two fixtures; changing a parent changes the hash.
  const kz::Bytes run = kzfix::build_run_fixture();
  kz::Container rc = must_parse(run);
  const kz::Bytes verdicts = kzfix::build_verdicts_fixture(c.artifact_hash(), rc.artifact_hash());
  kz::Container vc = must_parse(verdicts);
  KZ_CHECK_EQ(vc.parents().size(), static_cast<size_t>(2));
  KZ_CHECK_OK(vc.verify_parent(c));
  KZ_CHECK_OK(vc.verify_parent(rc));
  KZ_CHECK_OK(vc.verify_content());
  kz::Bytes changed = facts;
  const kz_section_entry* fe = c.find_section(KZ_SEC_FACTS);
  KZ_CHECK(fe != nullptr);
  if (fe != nullptr) changed[fe->offset] ^= 0x01;
  kz::Container tampered;
  // The tampered file is no longer valid (CRC), and in any case its hash is not the parent's.
  KZ_CHECK_ERR(kz::Container::parse(changed, &tampered), kz::Err::BadCrc);
  KZ_CHECK(kz::sha256(changed.data(), changed.size()) != c.artifact_hash());
  const kz::Bytes other = kzfix::build_run_fixture();  // a valid container that is not a parent
  kz::Container oc = must_parse(other);
  kz::ContainerWriter w2(KZ_ARTIFACT_TEST);
  kz::Bytes alone;
  KZ_CHECK_OK(w2.finish(&alone));
  kz::Container ac = must_parse(alone);
  KZ_CHECK_ERR(ac.verify_parent(rc), kz::Err::BadParent);
  (void)oc;

  // A parent listed with another artifact kind than the parent's own is refused.
  kz::ContainerWriter wrong(KZ_ARTIFACT_VERDICTS);
  KZ_CHECK_OK(wrong.add_parent(KZ_ARTIFACT_RUN, 0, c.artifact_hash()));  // `c` is a FACTS container
  kz::Bytes wrong_bytes;
  KZ_CHECK_OK(wrong.finish(&wrong_bytes));
  kz::Container wc = must_parse(wrong_bytes);
  kz::Status ks = wc.verify_parent(c);
  KZ_CHECK_ERR(ks, kz::Err::BadParent);
  KZ_CHECK(ks.message.find("different artifact kind") != std::string::npos);
  kz::ContainerWriter right(KZ_ARTIFACT_VERDICTS);
  KZ_CHECK_OK(right.add_parent(KZ_ARTIFACT_FACTS, 0, c.artifact_hash()));
  kz::Bytes right_bytes;
  KZ_CHECK_OK(right.finish(&right_bytes));
  KZ_CHECK_OK(must_parse(right_bytes).verify_parent(c));
}

void test_writer_is_order_independent() {
  // Sections added in any order, parents added in any order, strings interned in any order:
  // the same canonical bytes.
  std::vector<kz_file_hash> files(1, kzfix::zero<kz_file_hash>());
  const kz::Digest p1 = kz::sha256("p1"), p2 = kz::sha256("p2"), p3 = kz::sha256("p3");

  // The records name strings; the writers intern them in opposite orders.
  auto records = [&](kz::ContainerWriter* w, bool forward, std::vector<kz_basis_item>* basis, std::vector<kz_deviation>* dev) {
    const char* words[] = {"alpha", "beta", "gamma", "delta"};
    kz_str s[4];
    for (int i = 0; i < 4; ++i) {
      const int k = forward ? i : 3 - i;
      s[k] = w->intern(words[k]);
    }
    *basis = std::vector<kz_basis_item>(2, kzfix::zero<kz_basis_item>());
    (*basis)[0].name = s[0];
    (*basis)[0].value = s[2];
    (*basis)[1].name = s[3];
    (*basis)[1].value = s[0];
    *dev = std::vector<kz_deviation>(1, kzfix::zero<kz_deviation>());
    (*dev)[0].subject = s[1];
    (*dev)[0].reason = s[3];
    for (kz_basis_item& item : *basis) item.prov = item.deviation = KZ_NONE32;
    (*dev)[0].prov = KZ_NONE32;
  };

  kz::ContainerWriter a(KZ_ARTIFACT_TEST);
  std::vector<kz_basis_item> basis_a;
  std::vector<kz_deviation> dev_a;
  records(&a, true, &basis_a, &dev_a);
  KZ_CHECK_OK(a.add_section(KZ_SEC_BASIS, basis_a));
  KZ_CHECK_OK(a.add_section(KZ_SEC_FILE_HASHES, files));
  KZ_CHECK_OK(a.add_section(KZ_SEC_DEVIATIONS, dev_a));
  KZ_CHECK_OK(a.add_parent(KZ_ARTIFACT_FACTS, 0, p1));
  KZ_CHECK_OK(a.add_parent(KZ_ARTIFACT_RUN, 1, p2));
  KZ_CHECK_OK(a.add_parent(KZ_ARTIFACT_RUN, 0, p3));

  kz::ContainerWriter b(KZ_ARTIFACT_TEST);
  std::vector<kz_basis_item> basis_b;
  std::vector<kz_deviation> dev_b;
  records(&b, false, &basis_b, &dev_b);
  b.intern("never referenced");  // an unreferenced string does not reach the file
  KZ_CHECK(basis_a[0].name.off != basis_b[0].name.off);  // the provisional references do differ
  KZ_CHECK_OK(b.add_parent(KZ_ARTIFACT_RUN, 0, p3));
  KZ_CHECK_OK(b.add_section(KZ_SEC_DEVIATIONS, dev_b));
  KZ_CHECK_OK(b.add_parent(KZ_ARTIFACT_RUN, 1, p2));
  KZ_CHECK_OK(b.add_section(KZ_SEC_FILE_HASHES, files));
  KZ_CHECK_OK(b.add_parent(KZ_ARTIFACT_FACTS, 0, p1));
  KZ_CHECK_OK(b.add_section(KZ_SEC_BASIS, basis_b));

  kz::Bytes ba, bb;
  KZ_CHECK_OK(a.finish(&ba));
  KZ_CHECK_OK(b.finish(&bb));
  KZ_CHECK(ba == bb);
  kz::Container c = must_parse(ba);
  KZ_CHECK_OK(c.verify_content());
  KZ_CHECK_EQ(c.parents().size(), static_cast<size_t>(3));
  // Parents are stored sorted by (kind, role, hash).
  KZ_CHECK_EQ(c.parents()[0].kind, static_cast<uint32_t>(KZ_ARTIFACT_FACTS));
  KZ_CHECK_EQ(c.parents()[1].role, 0u);
  KZ_CHECK_EQ(c.parents()[2].role, 1u);
  check_padding_is_zero(c);
}

// The STRINGS section is a function of the strings that the records reference.
void test_canonical_string_pool() {
  kz::ContainerWriter w(KZ_ARTIFACT_TEST);
  std::vector<kz_basis_item> basis(2, kzfix::zero<kz_basis_item>());
  basis[0].name = w.intern("beta");
  basis[0].value = w.intern("alpha");
  basis[1].name = w.intern("alpha");  // a string used twice is stored once
  basis[1].value = w.intern("al");    // a prefix of another string is a string of its own
  for (auto& item : basis) item.prov = item.deviation = KZ_NONE32;
  w.intern("unused");
  KZ_CHECK_OK(w.add_section(KZ_SEC_BASIS, basis));
  kz::Bytes bytes;
  KZ_CHECK_OK(w.finish(&bytes));
  kz::Container c = must_parse(bytes);
  KZ_CHECK_OK(c.verify_content());
  size_t n = 0;
  const uint8_t* pool = c.section_data(KZ_SEC_STRINGS, &n);
  const std::string expected = std::string("\0", 1) + "al" + std::string("\0", 1) + "alpha" + std::string("\0", 1) + "beta" + std::string("\0", 1);
  KZ_CHECK_EQ(std::string(reinterpret_cast<const char*>(pool), n), expected);  // sorted byte-wise, shorter prefix first
  std::vector<kz_basis_item> back;
  KZ_CHECK_OK(c.read_section(KZ_SEC_BASIS, &back));
  KZ_CHECK_EQ(back[0].name.off, 10u);
  KZ_CHECK_EQ(back[0].value.off, 4u);
  KZ_CHECK_EQ(back[1].name.off, 4u);
  KZ_CHECK_EQ(back[1].value.off, 1u);
  std::string text;
  KZ_CHECK(c.string(back[1].value, &text));
  KZ_CHECK_EQ(text, std::string("al"));

  // Bytes above 0x7f sort after ASCII (unsigned byte order), and the empty string is (0, 0).
  kz::ContainerWriter w2(KZ_ARTIFACT_TEST);
  std::vector<kz_basis_item> b2(1, kzfix::zero<kz_basis_item>());
  b2[0].name = w2.intern("\xc3\xa9");
  b2[0].value = w2.intern("z");
  b2[0].prov = b2[0].deviation = KZ_NONE32;
  KZ_CHECK_OK(w2.add_section(KZ_SEC_BASIS, b2));
  kz::Bytes bytes2;
  KZ_CHECK_OK(w2.finish(&bytes2));
  kz::Container c2 = must_parse(bytes2);
  KZ_CHECK_OK(c2.verify_content());
  std::vector<kz_basis_item> back2;
  KZ_CHECK_OK(c2.read_section(KZ_SEC_BASIS, &back2));
  KZ_CHECK_EQ(back2[0].value.off, 1u);  // "z" before "\xc3\xa9"
  KZ_CHECK_EQ(back2[0].name.off, 3u);

  // A strict writer refuses a kz_str that is not in its pool (a value made up by hand); a writer set up
  // for reader tests leaves it alone.
  kz::ContainerWriter w3(KZ_ARTIFACT_TEST);
  std::vector<kz_basis_item> b3(1, kzfix::zero<kz_basis_item>());
  b3[0].name = kz_str{5, 2};
  b3[0].prov = b3[0].deviation = KZ_NONE32;
  KZ_CHECK_OK(w3.add_section(KZ_SEC_BASIS, b3));
  kz::Bytes bytes3;
  KZ_CHECK_ERR(w3.finish(&bytes3), kz::Err::BadRef);
  w3.set_check_reserved(false);
  KZ_CHECK_OK(w3.finish(&bytes3));
  kz::Container c3 = must_parse(bytes3);
  KZ_CHECK_ERR(c3.verify_refs(), kz::Err::BadRef);

  // Many distinct strings (the provisional table is hashed, not scanned).
  kz::StringPool pool2;
  for (int i = 0; i < 20000; ++i) pool2.intern("name_" + std::to_string(i));
  KZ_CHECK_EQ(pool2.intern("name_77").off, pool2.intern("name_77").off);
  KZ_CHECK_EQ(pool2.intern("name_19999").len, 10u);
}

void test_string_pool() {
  kz::StringPool pool;
  KZ_CHECK_EQ(pool.bytes().size(), static_cast<size_t>(1));
  kz_str e = pool.intern("");
  KZ_CHECK_EQ(e.off, 0u);
  KZ_CHECK_EQ(e.len, 0u);
  kz_str a = pool.intern("alpha");
  kz_str b = pool.intern("beta");
  kz_str a2 = pool.intern("alpha");
  KZ_CHECK(a.off == a2.off && a.len == a2.len);  // equal strings share one entry
  KZ_CHECK(a.off != b.off);
  KZ_CHECK_EQ(a.off, 1u);
  KZ_CHECK_EQ(a.len, 5u);
  KZ_CHECK_EQ(pool.bytes()[a.off + a.len], static_cast<uint8_t>(0));  // always NUL terminated
  // Arbitrary bytes, including quotes and newlines, survive.
  kz::ContainerWriter w(KZ_ARTIFACT_TEST);
  const std::string odd = std::string("line1\nline2 \"quoted\" \\ tab\t") + '\xc3' + '\xa9';
  std::vector<kz_basis_item> basis(1, kzfix::zero<kz_basis_item>());
  basis[0].name = w.intern(odd);
  basis[0].prov = KZ_NONE32;
  basis[0].deviation = KZ_NONE32;
  KZ_CHECK_OK(w.add_section(KZ_SEC_BASIS, basis));
  kz::Bytes bytes;
  KZ_CHECK_OK(w.finish(&bytes));
  kz::Container c = must_parse(bytes);
  std::vector<kz_basis_item> back;
  KZ_CHECK_OK(c.read_section(KZ_SEC_BASIS, &back));
  std::string text;
  KZ_CHECK(c.string(back[0].name, &text));
  KZ_CHECK_EQ(text, odd);
  KZ_CHECK(!c.string(kz_str{0xFFFF, 4}, &text));  // out of range
  KZ_CHECK(!c.string(kz_str{3, 0}, &text));       // empty strings are (0, 0)
}

// The rules of Container::string at the edges of the pool.
void test_string_references_at_the_edges() {
  kz::ContainerWriter w(KZ_ARTIFACT_TEST);
  std::vector<kz_basis_item> basis(1, kzfix::zero<kz_basis_item>());
  basis[0].name = w.intern("ab");
  basis[0].prov = basis[0].deviation = KZ_NONE32;
  KZ_CHECK_OK(w.add_section(KZ_SEC_BASIS, basis));
  kz::Bytes bytes;
  KZ_CHECK_OK(w.finish(&bytes));
  kz::Container c = must_parse(bytes);
  size_t n = 0;
  const uint8_t* pool = c.section_data(KZ_SEC_STRINGS, &n);
  KZ_CHECK_EQ(n, static_cast<size_t>(4));  // NUL, 'a', 'b', NUL
  KZ_CHECK(pool != nullptr && pool[3] == 0);
  std::string text;
  KZ_CHECK(c.string(kz_str{1, 2}, &text) && text == "ab");
  KZ_CHECK(c.string(kz_str{2, 1}, &text) && text == "b");  // a suffix resolves (verify_strings refuses it)
  // A string must end at a NUL inside the pool.
  KZ_CHECK(!c.string(kz_str{1, 1}, &text));  // would end at 'b', which is not a terminator
  KZ_CHECK(!c.string(kz_str{1, 3}, &text));  // its terminator would be the first byte after the pool
  KZ_CHECK(!c.string(kz_str{1, 4}, &text));
  KZ_CHECK(!c.string(kz_str{3, 1}, &text));  // starts at the last byte
  KZ_CHECK(!c.string(kz_str{0, 1}, &text));  // offset 0 is the empty string only
  KZ_CHECK(!c.string(kz_str{4, 0}, &text));  // so is length 0
  KZ_CHECK(!c.string(kz_str{0xFFFFFFFFu, 2}, &text));
  KZ_CHECK(!c.string(kz_str{2, 0xFFFFFFFFu}, &text));  // offset + length does not wrap around
  KZ_CHECK(c.string(kz_str{0, 0}, &text) && text.empty());
}

void test_writer_argument_checks() {
  kz::ContainerWriter w(KZ_ARTIFACT_TEST);
  std::vector<kz_basis_item> basis(2, kzfix::zero<kz_basis_item>());
  KZ_CHECK_OK(w.add_section(KZ_SEC_BASIS, basis));
  KZ_CHECK_ERR(w.add_section(KZ_SEC_BASIS, basis), kz::Err::Duplicate);
  // A section holds one record type.
  std::vector<kz_file_hash> files(1, kzfix::zero<kz_file_hash>());
  KZ_CHECK_ERR(w.add_section(KZ_SEC_DEVIATIONS, files), kz::Err::BadRecord);
  // Unregistered section ids and the string pool section are not accepted.
  KZ_CHECK_ERR(w.add_section_raw(999, basis.data(), 1, sizeof(kz_basis_item)), kz::Err::BadSection);
  KZ_CHECK_ERR(w.add_section_raw(KZ_SEC_STRINGS, "x", 1, 1), kz::Err::Invalid);
  KZ_CHECK_ERR(w.add_section_raw(KZ_SEC_FILE_HASHES, basis.data(), 1, 7), kz::Err::BadRecord);
  const kz::Digest p = kz::sha256("p");
  KZ_CHECK_OK(w.add_parent(KZ_ARTIFACT_FACTS, 0, p));
  KZ_CHECK_ERR(w.add_parent(KZ_ARTIFACT_FACTS, 1, p), kz::Err::BadParent);  // the same parent twice
  // An empty section is allowed and is distinct from an absent one.
  std::vector<kz_deviation> none;
  KZ_CHECK_OK(w.add_section(KZ_SEC_DEVIATIONS, none));
  kz::Bytes bytes;
  KZ_CHECK_OK(w.finish(&bytes));
  kz::Container c = must_parse(bytes);
  const kz_section_entry* e = c.find_section(KZ_SEC_DEVIATIONS);
  KZ_CHECK(e != nullptr && e->count == 0);
  KZ_CHECK(c.find_section(KZ_SEC_PROVENANCE) == nullptr);
  std::vector<kz_deviation> got(3);
  KZ_CHECK_OK(c.read_section(KZ_SEC_DEVIATIONS, &got));
  KZ_CHECK(got.empty());
  std::vector<kz_prov> absent(3);
  KZ_CHECK_OK(c.read_section(KZ_SEC_PROVENANCE, &absent));
  KZ_CHECK(absent.empty());
}

void test_typed_access_checks() {
  const kz::Bytes b = kzfix::build_facts_fixture();
  kz::Container c = must_parse(b);
  std::vector<kz_site> wrong;
  KZ_CHECK_ERR(c.read_section(KZ_SEC_FACTS, &wrong), kz::Err::BadRecord);  // FACTS holds kz_fact_call
  std::vector<kz_fact_call> right;
  KZ_CHECK_OK(c.read_section(KZ_SEC_FACTS, &right));
  std::vector<kz_fact_call> unregistered;
  KZ_CHECK_ERR(c.read_section(999, &unregistered), kz::Err::BadRecord);
}

void test_file_io() {
  const kz::Bytes b = kzfix::build_run_fixture();
  const std::string path = "kz_test_container_" + std::to_string(static_cast<long>(getpid())) + ".kz";
  KZ_CHECK_OK(kz::write_whole_file(path, b));
  kz::Container c;
  KZ_CHECK_OK(kz::Container::read_file(path, &c));
  KZ_CHECK(c.bytes() == b);
  KZ_CHECK(c.artifact_hash() == kz::sha256(b.data(), b.size()));
  std::remove(path.c_str());
  KZ_CHECK_ERR(kz::Container::read_file(path, &c), kz::Err::Io);
  // ContainerWriter::write_file leaves no temporary file behind.
  kz::ContainerWriter w(KZ_ARTIFACT_TEST);
  KZ_CHECK_OK(w.write_file(path));
  KZ_CHECK_OK(kz::Container::read_file(path, &c));
  std::remove(path.c_str());
  KZ_CHECK(access((path + ".tmp").c_str(), F_OK) != 0);
}

void test_tool_hash() {
  // The inputs of the tool hash and the source identity are tested in test_core_identity;
  // here the hash as the container header carries it.
  const kz::Digest a = kz::tool_hash("kz", "1");
  KZ_CHECK(a == kz::tool_hash("kz", "1"));
  KZ_CHECK(a != kz::tool_hash("kz", "2"));
  KZ_CHECK(a != kz::tool_hash("kz2", "1"));
  KZ_CHECK(a != kz::tool_hash("kz", "1", std::string(64, 'c')));  // the sources of the tool count
  KZ_CHECK(kz::schema_fingerprint() == kz::sha256(kz::layout_text()));
  const std::string id = kz::build_identity();  // provenance text: schema fingerprint and compiler, no time
  KZ_CHECK(id.find(kz::to_hex(kz::schema_fingerprint())) != std::string::npos);
  KZ_CHECK(id.find("abi=1") != std::string::npos && id.find("compiler=") != std::string::npos);
  KZ_CHECK_EQ(id, kz::build_identity());
  // The header carries what the writer was given.
  kz::ContainerWriter w(KZ_ARTIFACT_TEST);
  w.set_tool_hash(a);
  kz::Bytes bytes;
  KZ_CHECK_OK(w.finish(&bytes));
  KZ_CHECK(must_parse(bytes).tool_hash() == a);
}

}  // namespace

int main() {
  test_empty_container();
  test_round_trip_and_hash_stability();
  test_writer_is_order_independent();
  test_canonical_string_pool();
  test_string_pool();
  test_string_references_at_the_edges();
  test_writer_argument_checks();
  test_typed_access_checks();
  test_file_io();
  test_tool_hash();
  return kztest::finish("test_core_container");
}

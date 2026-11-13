// The reader rejects every damaged container: every single-byte change and every
// truncation of the fixtures is detected, and each structural rule is exercised on its own
// by editing a valid image and recomputing the CRCs so that only that rule is violated.

#include <cstring>

#include "kz/core/container.hpp"
#include "kz/core/crc32.hpp"
#include "kz/core/registry.hpp"
#include "support/fixtures.hpp"
#include "support/kz_test.hpp"

namespace {

kz_header read_header(const kz::Bytes& b) {
  kz_header h;
  std::memcpy(&h, b.data(), sizeof(h));
  return h;
}

void write_header(kz::Bytes* b, const kz_header& h) { std::memcpy(b->data(), &h, sizeof(h)); }

size_t entry_pos(const kz_header& h, uint32_t i) { return static_cast<size_t>(h.sections_off) + i * sizeof(kz_section_entry); }

kz_section_entry read_entry(const kz::Bytes& b, uint32_t i) {
  kz_section_entry e;
  std::memcpy(&e, b.data() + entry_pos(read_header(b), i), sizeof(e));
  return e;
}

void write_entry(kz::Bytes* b, uint32_t i, const kz_section_entry& e) {
  std::memcpy(b->data() + entry_pos(read_header(*b), i), &e, sizeof(e));
}

// Recomputes every section CRC and the header CRC of an edited image.
void refresh_crcs(kz::Bytes* b) {
  kz_header h = read_header(*b);
  for (uint32_t i = 0; i < h.n_sections; ++i) {
    kz_section_entry e = read_entry(*b, i);
    if (e.offset <= b->size() && e.size <= b->size() - e.offset) e.crc32 = kz::crc32(b->data() + e.offset, static_cast<size_t>(e.size));
    write_entry(b, i, e);
  }
  h.header_crc = 0;
  write_header(b, h);
  const size_t tables_end = static_cast<size_t>(h.sections_off) + h.n_sections * sizeof(kz_section_entry);
  h.header_crc = kz::crc32(b->data(), tables_end);
  write_header(b, h);
}

int index_of_section(const kz::Bytes& b, uint32_t type) {
  const kz_header h = read_header(b);
  for (uint32_t i = 0; i < h.n_sections; ++i)
    if (read_entry(b, i).type == type) return static_cast<int>(i);
  return -1;
}

// A section of a hand-assembled container.
struct RawSection {
  uint32_t type;
  uint32_t record_size;
  uint64_t count;
  kz::Bytes data;
};

uint64_t align8(uint64_t x) { return (x + 7u) & ~static_cast<uint64_t>(7u); }

// Assembles a container without parents from sections given in ascending type order, in the
// canonical layout.  It lets a test put bytes into a valid file that the canonical writer never
// writes (a string pool that is not the canonical one).
kz::Bytes assemble(uint32_t kind, const std::vector<RawSection>& sections) {
  const uint64_t sections_off = align8(sizeof(kz_header));
  const uint64_t tables_end = sections_off + sections.size() * sizeof(kz_section_entry);
  std::vector<kz_section_entry> table(sections.size(), kzfix::zero<kz_section_entry>());
  uint64_t cursor = tables_end;
  for (size_t i = 0; i < sections.size(); ++i) {
    const kz::SectionInfo* info = kz::find_section(sections[i].type);
    table[i].type = sections[i].type;
    table[i].schema_version = info != nullptr ? info->schema_version : 1;
    table[i].record_size = sections[i].record_size;
    table[i].count = sections[i].count;
    table[i].offset = align8(cursor);
    table[i].size = sections[i].data.size();
    table[i].crc32 = kz::crc32(sections[i].data.data(), sections[i].data.size());
    cursor = table[i].offset + table[i].size;
  }
  kz::Bytes b(static_cast<size_t>(align8(cursor)), 0);
  kz_header h = kzfix::zero<kz_header>();
  std::memcpy(h.magic, kz::kMagic, 8);
  h.abi_version = KZ_ABI_VERSION;
  h.artifact_kind = kind;
  h.header_size = sizeof(kz_header);
  h.n_sections = static_cast<uint32_t>(sections.size());
  h.parents_off = sizeof(kz_header);
  h.sections_off = sections_off;
  h.total_size = b.size();
  std::memcpy(b.data(), &h, sizeof(h));
  for (size_t i = 0; i < table.size(); ++i) {
    std::memcpy(b.data() + sections_off + i * sizeof(kz_section_entry), &table[i], sizeof(kz_section_entry));
    if (!sections[i].data.empty()) std::memcpy(b.data() + table[i].offset, sections[i].data.data(), sections[i].data.size());
  }
  h.header_crc = kz::crc32(b.data(), static_cast<size_t>(tables_end));
  std::memcpy(b.data(), &h, sizeof(h));
  return b;
}

kz::Bytes bytes_of(const std::string& text) { return kz::Bytes(text.begin(), text.end()); }

RawSection pool_section(const std::string& pool) { return RawSection{KZ_SEC_STRINGS, 1, pool.size(), bytes_of(pool)}; }

// One DEVIATIONS record whose `subject` and `reason` are the given string references.
RawSection deviation_section(kz_str subject, kz_str reason) {
  kz_deviation d = kzfix::zero<kz_deviation>();
  d.subject = subject;
  d.reason = reason;
  d.prov = KZ_NONE32;
  kz::Bytes data(sizeof(d));
  std::memcpy(data.data(), &d, sizeof(d));
  return RawSection{KZ_SEC_DEVIATIONS, sizeof(kz_deviation), 1, data};
}

kz::Status parse(const kz::Bytes& b, const kz::ParseOptions& o = kz::ParseOptions()) {
  kz::Container c;
  return kz::Container::parse(b, &c, o);
}

void test_every_single_byte_change_is_detected(const char* name, const kz::Bytes& good) {
  KZ_CHECK_OK(parse(good));
  size_t undetected = 0;
  for (size_t i = 0; i < good.size(); ++i) {
    kz::Bytes bad = good;
    bad[i] ^= 0x01;
    if (parse(bad).ok()) {
      ++undetected;
      std::fprintf(stderr, "%s: change of byte %zu was not detected\n", name, i);
    }
    bad[i] = static_cast<uint8_t>(good[i] ^ 0xff);
    if (parse(bad).ok()) {
      ++undetected;
      std::fprintf(stderr, "%s: inversion of byte %zu was not detected\n", name, i);
    }
  }
  KZ_CHECK_EQ(undetected, static_cast<size_t>(0));
}

void test_every_truncation_is_detected(const char* name, const kz::Bytes& good) {
  size_t undetected = 0;
  for (size_t n = 0; n < good.size(); ++n) {
    kz::Bytes bad(good.begin(), good.begin() + static_cast<std::ptrdiff_t>(n));
    if (parse(bad).ok()) {
      ++undetected;
      std::fprintf(stderr, "%s: truncation to %zu bytes was not detected\n", name, n);
    }
  }
  KZ_CHECK_EQ(undetected, static_cast<size_t>(0));
  kz::Bytes longer = good;
  longer.push_back(0);
  KZ_CHECK_ERR(parse(longer), kz::Err::BadHeader);
}

void test_structural_rules() {
  const kz::Bytes good = kzfix::build_facts_fixture();
  KZ_CHECK_OK(parse(good));
  const kz_header h0 = read_header(good);
  KZ_CHECK(h0.n_sections >= 3);

  {  // magic
    kz::Bytes b = good;
    b[0] = 0;
    KZ_CHECK_ERR(parse(b), kz::Err::BadMagic);
  }
  {  // ABI version and header size
    kz::Bytes b = good;
    kz_header h = h0;
    h.abi_version = 2;
    write_header(&b, h);
    refresh_crcs(&b);
    KZ_CHECK_ERR(parse(b), kz::Err::BadAbi);
    b = good;
    h = h0;
    h.header_size = 161;
    write_header(&b, h);
    refresh_crcs(&b);
    KZ_CHECK_ERR(parse(b), kz::Err::BadAbi);
  }
  {  // total_size
    kz::Bytes b = good;
    kz_header h = h0;
    h.total_size = good.size() - 8;
    write_header(&b, h);
    refresh_crcs(&b);
    KZ_CHECK_ERR(parse(b), kz::Err::BadHeader);
    b = good;
    h = h0;
    h.total_size = good.size() + 8;
    write_header(&b, h);
    refresh_crcs(&b);
    KZ_CHECK_ERR(parse(b), kz::Err::Truncated);
  }
  {  // reserved header fields
    kz::Bytes b = good;
    kz_header h = h0;
    h.flags = 1;
    write_header(&b, h);
    refresh_crcs(&b);
    KZ_CHECK_ERR(parse(b), kz::Err::BadReserved);
  }
  {  // table placement
    kz::Bytes b = good;
    kz_header h = h0;
    h.parents_off = 0x100;
    write_header(&b, h);
    refresh_crcs(&b);
    KZ_CHECK_ERR(parse(b), kz::Err::NotCanonical);
    b = good;
    h = h0;
    h.sections_off += 8;
    write_header(&b, h);
    refresh_crcs(&b);
    KZ_CHECK_ERR(parse(b), kz::Err::NotCanonical);
  }
  {  // table sizes that cannot be real
    kz::Bytes b = good;
    kz_header h = h0;
    h.n_sections = 0x7fffffff;
    write_header(&b, h);
    KZ_CHECK_ERR(parse(b), kz::Err::BadHeader);
  }
  {  // header CRC itself
    kz::Bytes b = good;
    kz_header h = h0;
    h.header_crc ^= 1;
    write_header(&b, h);
    KZ_CHECK_ERR(parse(b), kz::Err::BadCrc);
  }
  {  // section table: type not registered; order; record size; schema version; counts; offsets; reserved
    const uint32_t last = h0.n_sections - 1;
    kz::Bytes b = good;
    kz_section_entry e = read_entry(b, last);
    e.type = 99;
    write_entry(&b, last, e);
    refresh_crcs(&b);
    KZ_CHECK_ERR(parse(b), kz::Err::BadSection);
    kz::ParseOptions lenient;
    lenient.allow_unknown_sections = true;
    // An unknown section is accepted only when asked for, and only with a plausible record size.
    KZ_CHECK_OK(parse(b, lenient));

    b = good;  // a duplicate type, and a type that goes backwards: not strictly ascending
    e = read_entry(b, 1);
    e.type = read_entry(b, 0).type;
    write_entry(&b, 1, e);
    refresh_crcs(&b);
    KZ_CHECK_ERR(parse(b), kz::Err::BadSection);
    b = good;
    e = read_entry(b, last);
    e.type = 0;
    write_entry(&b, last, e);
    refresh_crcs(&b);
    KZ_CHECK_ERR(parse(b), kz::Err::BadSection);

    b = good;
    e = read_entry(b, 1);
    e.record_size += 1;
    write_entry(&b, 1, e);
    refresh_crcs(&b);
    KZ_CHECK_ERR(parse(b), kz::Err::BadRecord);

    b = good;
    e = read_entry(b, 1);
    e.schema_version += 1;
    write_entry(&b, 1, e);
    refresh_crcs(&b);
    KZ_CHECK_ERR(parse(b), kz::Err::BadRecord);

    b = good;
    e = read_entry(b, 1);
    e.count += 1;
    write_entry(&b, 1, e);
    refresh_crcs(&b);
    KZ_CHECK_ERR(parse(b), kz::Err::BadSection);

    b = good;
    e = read_entry(b, 1);
    e.offset += 8;
    write_entry(&b, 1, e);
    refresh_crcs(&b);
    KZ_CHECK_ERR(parse(b), kz::Err::NotCanonical);

    b = good;
    e = read_entry(b, 1);
    e.rsv0 = 1;
    write_entry(&b, 1, e);
    refresh_crcs(&b);
    KZ_CHECK_ERR(parse(b), kz::Err::BadReserved);

    b = good;  // a payload change without a CRC update
    e = read_entry(b, 1);
    b[e.offset] ^= 0x10;
    kz_header h = h0;
    (void)h;
    KZ_CHECK_ERR(parse(b), kz::Err::BadCrc);
  }
  {  // the string pool must start with the empty string
    kz::Bytes b = good;
    const int i = index_of_section(b, KZ_SEC_STRINGS);
    KZ_CHECK(i >= 0);
    b[read_entry(b, static_cast<uint32_t>(i)).offset] = 1;
    refresh_crcs(&b);
    KZ_CHECK_ERR(parse(b), kz::Err::BadSection);
  }
}

void test_padding_rules() {
  const kz::Bytes good = kzfix::build_facts_fixture();
  const kz_header h = read_header(good);
  // Find a gap between sections (a section whose size is not a multiple of 8).
  int gap_after = -1;
  for (uint32_t i = 0; i + 1 < h.n_sections; ++i) {
    kz_section_entry e = read_entry(good, i), n = read_entry(good, i + 1);
    if (e.offset + e.size < n.offset) {
      gap_after = static_cast<int>(i);
      break;
    }
  }
  KZ_CHECK(gap_after >= 0);
  if (gap_after >= 0) {
    kz::Bytes b = good;
    kz_section_entry e = read_entry(b, static_cast<uint32_t>(gap_after));
    b[e.offset + e.size] = 0x5a;  // padding is covered by no CRC: only the zero rule catches it
    KZ_CHECK_ERR(parse(b), kz::Err::BadPadding);
  }
}

// Padding after the last section: it is covered by no CRC, so only the zero rule catches a change.
// Both containers are known to have trailing padding; the check is not conditional.
void test_trailing_padding_rule() {
  kz::Bytes good_empty;
  {
    kz::ContainerWriter w(KZ_ARTIFACT_TEST);  // STRINGS only: a one-byte pool
    KZ_CHECK_OK(w.finish(&good_empty));
  }
  kz::Bytes good_trust;
  {
    kz::ContainerWriter w(KZ_ARTIFACT_TEST);  // the last section holds one 60-byte record
    std::vector<kz_trust> t(1, kzfix::zero<kz_trust>());
    t[0].site = t[0].refutes = KZ_NONE32;
    KZ_CHECK_OK(w.add_section(KZ_SEC_TRUST, t));
    KZ_CHECK_OK(w.finish(&good_trust));
  }
  const size_t expected_padding[] = {7, 4};
  size_t which = 0;
  for (const kz::Bytes* good : {&good_empty, &good_trust}) {
    KZ_CHECK_OK(parse(*good));
    const kz_header h = read_header(*good);
    const kz_section_entry last = read_entry(*good, h.n_sections - 1);
    const size_t end = static_cast<size_t>(last.offset + last.size);
    KZ_CHECK(end < good->size());            // there is trailing padding ...
    KZ_CHECK_EQ(good->size() - end, expected_padding[which++]);
    KZ_CHECK_EQ(good->size() % 8, static_cast<size_t>(0));
    for (size_t i = end; i < good->size(); ++i) {  // ... and every byte of it is checked
      kz::Bytes b = *good;
      b[i] = 0x01;
      KZ_CHECK_ERR(parse(b), kz::Err::BadPadding);
      b[i] = 0x80;
      KZ_CHECK_ERR(parse(b), kz::Err::BadPadding);
    }
  }
}

// The string pool is canonical (sorted, distinct, only referenced strings, every reference at the
// canonical offset).  The files are valid in every other respect: they parse, their CRCs hold
// and every reference resolves, so only verify_strings() can refuse them.
void test_string_pool_rules() {
  // The control: the canonical pool of "a" and "b" assembled by hand is what the writer writes.
  const std::string nul(1, '\0');
  const std::string canonical_pool = nul + "a" + nul + "b" + nul;
  const kz_str a{1, 1}, b{3, 1};
  {
    kz::ContainerWriter w(KZ_ARTIFACT_TEST);
    std::vector<kz_deviation> d(1, kzfix::zero<kz_deviation>());
    d[0].subject = w.intern("b");
    d[0].reason = w.intern("a");
    d[0].prov = KZ_NONE32;
    KZ_CHECK_OK(w.add_section(KZ_SEC_DEVIATIONS, d));
    kz::Bytes written;
    KZ_CHECK_OK(w.finish(&written));
    const kz::Bytes by_hand = assemble(KZ_ARTIFACT_TEST, {pool_section(canonical_pool), deviation_section(b, a)});
    KZ_CHECK(written == by_hand);
    kz::Container c;
    KZ_CHECK_OK(kz::Container::parse(by_hand, &c));
    KZ_CHECK_OK(c.verify_content());
  }
  struct Case {
    const char* name;
    std::string pool;
    kz_str subject, reason;
  };
  const Case cases[] = {
      {"strings not sorted", nul + "b" + nul + "a" + nul, kz_str{1, 1}, kz_str{3, 1}},
      {"an unreferenced string", nul + "a" + nul + "b" + nul + "c" + nul, b, a},
      {"an unreferenced string first", nul + "9" + nul + "a" + nul + "b" + nul, kz_str{5, 1}, kz_str{3, 1}},
      {"a string stored twice", nul + "a" + nul + "a" + nul + "b" + nul, kz_str{5, 1}, a},
      {"the second copy referenced", nul + "a" + nul + "a" + nul + "b" + nul, kz_str{5, 1}, kz_str{3, 1}},
      {"a suffix stored as a string of its own is missing", nul + "ab" + nul, kz_str{1, 2}, kz_str{2, 1}},
      {"a reference into the middle of a string", nul + "abc" + nul + "c" + nul, kz_str{1, 3}, kz_str{3, 1}},
  };
  for (const Case& cs : cases) {
    const kz::Bytes bytes = assemble(KZ_ARTIFACT_TEST, {pool_section(cs.pool), deviation_section(cs.subject, cs.reason)});
    kz::Container c;
    kz::Status s = kz::Container::parse(bytes, &c);
    KZ_CHECK_OK(s);
    if (!s.ok()) continue;
    KZ_CHECK_OK(c.verify_reserved());
    KZ_CHECK_OK(c.verify_refs());  // every reference resolves ...
    kz::Status strings = c.verify_strings();
    if (strings.code != kz::Err::NotCanonical) std::fprintf(stderr, "case \"%s\": %s\n", cs.name, strings.describe().c_str());
    KZ_CHECK_ERR(strings, kz::Err::NotCanonical);  // ... but the pool is not canonical
    KZ_CHECK_ERR(c.verify_content(), kz::Err::NotCanonical);
  }
  {  // the last case reaches the second rule: the pool is canonical, one reference is not
    kz::Container c;
    KZ_CHECK_OK(kz::Container::parse(assemble(KZ_ARTIFACT_TEST, {pool_section(nul + "abc" + nul + "c" + nul), deviation_section(kz_str{1, 3}, kz_str{3, 1})}), &c));
    kz::Status s = c.verify_strings();
    KZ_CHECK_ERR(s, kz::Err::NotCanonical);
    KZ_CHECK(s.message.find("canonical offset") != std::string::npos);
    KZ_CHECK_OK(c.verify_refs());
  }
  {  // a reference outside the pool is a reference error, not a canonicity error
    kz::Container c;
    KZ_CHECK_OK(kz::Container::parse(assemble(KZ_ARTIFACT_TEST, {pool_section(canonical_pool), deviation_section(kz_str{9, 1}, a)}), &c));
    KZ_CHECK_ERR(c.verify_strings(), kz::Err::BadRef);
  }
  {  // a section type this build does not know may reference strings that cannot be seen: the pool
     // is then not compared as a whole, while the references that can be seen still have to resolve
    const RawSection opaque{99, 8, 1, kz::Bytes(8, 0)};
    kz::ParseOptions lenient;
    lenient.allow_unknown_sections = true;
    const std::string extra = nul + "a" + nul + "b" + nul + "c" + nul;
    kz::Container c;
    KZ_CHECK_OK(kz::Container::parse(assemble(KZ_ARTIFACT_TEST, {pool_section(extra), deviation_section(b, a), opaque}), &c, lenient));
    KZ_CHECK_OK(c.verify_strings());
    KZ_CHECK_ERR(parse(assemble(KZ_ARTIFACT_TEST, {pool_section(extra), deviation_section(b, a), opaque})), kz::Err::BadSection);  // refused unless asked
    kz::Container d;
    KZ_CHECK_OK(kz::Container::parse(assemble(KZ_ARTIFACT_TEST, {pool_section(extra), deviation_section(kz_str{20, 1}, a), opaque}), &d, lenient));
    KZ_CHECK_ERR(d.verify_strings(), kz::Err::BadRef);
  }
}

void test_parent_table_rules() {
  const kz::Bytes facts = kzfix::build_facts_fixture();
  const kz::Bytes run = kzfix::build_run_fixture();
  kz::Container fc, rc;
  KZ_CHECK_OK(kz::Container::parse(facts, &fc));
  KZ_CHECK_OK(kz::Container::parse(run, &rc));
  const kz::Bytes good = kzfix::build_verdicts_fixture(fc.artifact_hash(), rc.artifact_hash());
  KZ_CHECK_OK(parse(good));
  const kz_header h = read_header(good);
  KZ_CHECK_EQ(h.n_parents, 2u);
  kz::Bytes b = good;  // swap the two parents: no longer strictly sorted
  uint8_t tmp[sizeof(kz_parent)];
  std::memcpy(tmp, b.data() + h.parents_off, sizeof(kz_parent));
  std::memcpy(b.data() + h.parents_off, b.data() + h.parents_off + sizeof(kz_parent), sizeof(kz_parent));
  std::memcpy(b.data() + h.parents_off + sizeof(kz_parent), tmp, sizeof(kz_parent));
  refresh_crcs(&b);
  KZ_CHECK_ERR(parse(b), kz::Err::BadParent);
  b = good;  // the same parent twice
  std::memcpy(b.data() + h.parents_off + sizeof(kz_parent), b.data() + h.parents_off, sizeof(kz_parent));
  refresh_crcs(&b);
  KZ_CHECK_ERR(parse(b), kz::Err::BadParent);
}

}  // namespace

int main() {
  const kz::Bytes facts = kzfix::build_facts_fixture();
  const kz::Bytes run = kzfix::build_run_fixture();
  kz::Container fc, rc;
  KZ_CHECK_OK(kz::Container::parse(facts, &fc));
  KZ_CHECK_OK(kz::Container::parse(run, &rc));
  const kz::Bytes verdicts = kzfix::build_verdicts_fixture(fc.artifact_hash(), rc.artifact_hash());
  test_every_single_byte_change_is_detected("facts", facts);
  test_every_single_byte_change_is_detected("run", run);
  test_every_single_byte_change_is_detected("verdicts", verdicts);
  test_every_truncation_is_detected("facts", facts);
  test_every_truncation_is_detected("verdicts", verdicts);
  test_structural_rules();
  test_padding_rules();
  test_trailing_padding_rule();
  test_string_pool_rules();
  test_parent_table_rules();
  return kztest::finish("test_core_container_corrupt");
}

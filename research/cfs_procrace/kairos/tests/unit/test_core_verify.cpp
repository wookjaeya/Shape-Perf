// Content checks on a parsed container: reserved fields are zero, typed references resolve
// (null, a record of this container, or a parent), string references lie in the pool, and
// every registered record type passes a pattern round trip.

#include <cstring>

#include "kz/core/container.hpp"
#include "kz/core/dump.hpp"
#include "support/fixtures.hpp"
#include "support/kz_test.hpp"

namespace {

kz::Container build(const std::vector<kz_verdict_cell>& cells, const std::vector<kz_ref>& refs, const kz::Digest* parent,
                    const std::vector<kz_deviation>* devs = nullptr) {
  kz::ContainerWriter w(KZ_ARTIFACT_TEST);
  w.set_check_reserved(false);  // these tests build containers that the reader must catch
  if (parent != nullptr) kzfix::must(w.add_parent(KZ_ARTIFACT_FACTS, 0, *parent), "parent");
  if (!cells.empty()) kzfix::must(w.add_section(KZ_SEC_VERDICT_CELLS, cells), "cells");
  if (!refs.empty()) kzfix::must(w.add_section(KZ_SEC_REF_LISTS, refs), "refs");
  if (devs != nullptr) kzfix::must(w.add_section(KZ_SEC_DEVIATIONS, *devs), "devs");
  kz::Bytes b;
  kzfix::must(w.finish(&b), "finish");
  kz::Container c;
  kzfix::must(kz::Container::parse(b, &c), "parse");
  return c;
}

void test_reserved_fields() {
  std::vector<kz_verdict_cell> cells(1, kzfix::zero<kz_verdict_cell>());
  KZ_CHECK_OK(build(cells, {}, nullptr).verify_reserved());
  // kz_verdict_cell has no rsv field; use records that do.
  std::vector<kz_deviation> devs(1, kzfix::zero<kz_deviation>());
  devs[0].prov = KZ_NONE32;
  KZ_CHECK_OK(build({}, {}, nullptr, &devs).verify_reserved());
  // The writer refuses such a record in the first place.
  {
    kz::ContainerWriter w(KZ_ARTIFACT_TEST);
    std::vector<kz_deviation> bad = devs;
    bad[0].rsv0 = 1;
    kz::Status s = w.add_section(KZ_SEC_DEVIATIONS, bad);
    KZ_CHECK_ERR(s, kz::Err::BadReserved);
    KZ_CHECK(s.message.find("DEVIATIONS[0].rsv0") != std::string::npos);
  }
  devs[0].rsv1 = 1;
  kz::Status s = build({}, {}, nullptr, &devs).verify_reserved();
  KZ_CHECK_ERR(s, kz::Err::BadReserved);
  KZ_CHECK(s.message.find("DEVIATIONS[0].rsv1") != std::string::npos);
  devs[0].rsv1 = 0;
  devs[0].rsv0 = 7;
  KZ_CHECK_ERR(build({}, {}, nullptr, &devs).verify_reserved(), kz::Err::BadReserved);
}

void test_references() {
  const kz::Digest parent = kz::sha256("parent");
  const kz::Digest stranger = kz::sha256("not a parent");
  auto ref = [](const kz::Digest* d, uint32_t section, uint32_t index) {
    kz_ref r = kzfix::zero<kz_ref>();
    if (d != nullptr) std::memcpy(r.artifact_sha256, d->data(), 32);
    r.section = section;
    r.index = index;
    return r;
  };
  std::vector<kz_verdict_cell> cells(2, kzfix::zero<kz_verdict_cell>());

  // Null references (all zero), a reference to a parent, and a reference to a record of this container.
  cells[0].certificate = ref(&parent, KZ_SEC_CERTIFICATES, 5);
  cells[0].bounds = ref(nullptr, KZ_SEC_VERDICT_CELLS, 1);
  KZ_CHECK_OK(build(cells, {}, &parent).verify_refs());

  // A reference to an artifact that is not a parent.
  cells[0].certificate = ref(&stranger, KZ_SEC_CERTIFICATES, 5);
  kz::Status s = build(cells, {}, &parent).verify_refs();
  KZ_CHECK_ERR(s, kz::Err::BadRef);
  KZ_CHECK(s.message.find("VERDICT_CELLS[0].certificate") != std::string::npos);
  // No parents at all: any foreign reference dangles.
  cells[0].certificate = ref(&parent, KZ_SEC_CERTIFICATES, 5);
  KZ_CHECK_ERR(build(cells, {}, nullptr).verify_refs(), kz::Err::BadRef);

  // Self references: index in range, section present.
  cells[0].certificate = kzfix::zero<kz_ref>();
  cells[0].bounds = ref(nullptr, KZ_SEC_VERDICT_CELLS, 2);  // only 2 cells: index 2 is out of range
  KZ_CHECK_ERR(build(cells, {}, nullptr).verify_refs(), kz::Err::BadRef);
  cells[0].bounds = ref(nullptr, KZ_SEC_BOUNDS, 0);  // this container has no BOUNDS section
  KZ_CHECK_ERR(build(cells, {}, nullptr).verify_refs(), kz::Err::BadRef);
  cells[0].bounds = ref(nullptr, KZ_SEC_VERDICT_CELLS, 1);
  KZ_CHECK_OK(build(cells, {}, nullptr).verify_refs());

  // REF_LISTS entries are references too.
  std::vector<kz_ref> list = {ref(&stranger, KZ_SEC_FACTS, 0)};
  KZ_CHECK_ERR(build({}, list, &parent).verify_refs(), kz::Err::BadRef);
  list[0] = ref(&parent, KZ_SEC_FACTS, 0);
  KZ_CHECK_OK(build({}, list, &parent).verify_refs());
}

void test_string_references() {
  std::vector<kz_deviation> devs(1, kzfix::zero<kz_deviation>());
  devs[0].prov = KZ_NONE32;
  devs[0].subject.off = 0x7fff;  // outside the pool
  devs[0].subject.len = 3;
  KZ_CHECK_ERR(build({}, {}, nullptr, &devs).verify_refs(), kz::Err::BadRef);
  devs[0].subject.off = 1;  // inside the pool of an empty writer? it has only the leading zero byte
  devs[0].subject.len = 1;
  KZ_CHECK_ERR(build({}, {}, nullptr, &devs).verify_refs(), kz::Err::BadRef);
  devs[0].subject = kz_str{0, 0};
  KZ_CHECK_OK(build({}, {}, nullptr, &devs).verify_refs());
}

void test_pattern_round_trip_of_every_record() {
  const kz::Digest foreign = kz::sha256("foreign parent");
  kz::Digest tool;
  std::vector<kzfix::SectionBytes> given;
  const kz::Bytes b = kzfix::make_all_sections_container(3, foreign, &tool, &given);
  kz::Container c;
  KZ_CHECK_OK(kz::Container::parse(b, &c));
  KZ_CHECK(c.tool_hash() == tool);
  KZ_CHECK_OK(c.verify_content());
  // Every registered section is present with 3 records (the string pool has its own size).
  for (const kz::SectionInfo& s : kz::sections()) {
    const kz_section_entry* e = c.find_section(s.type);
    KZ_CHECK(e != nullptr);
    if (e == nullptr || s.type == KZ_SEC_STRINGS) continue;
    KZ_CHECK_EQ(e->count, static_cast<uint64_t>(3));
    KZ_CHECK_EQ(e->record_size, s.record->size);
    // Rebuild the same record bytes and compare with what was stored.
    size_t size = 0;
    const uint8_t* data = c.section_data(s.type, &size);
    KZ_CHECK_EQ(size, static_cast<size_t>(3) * s.record->size);
    for (size_t i = 0; i < 3 && data != nullptr; ++i) {
      std::string line = kz::dump_record(*s.record, data + i * s.record->size, &c);
      KZ_CHECK(line.compare(0, std::strlen(s.record->name), s.record->name) == 0);
      KZ_CHECK(line.find('\n') == std::string::npos);  // one record per line
    }
  }
  // Round trip: the bytes read back are the bytes written, for every record type -- except
  // the kz_str fields, whose offsets the writer rewrites into the canonical string pool: they
  // must read back as the same text (or the empty string).
  KZ_CHECK_EQ(given.size() + 1, c.section_table().size());  // plus the string pool
  for (const kzfix::SectionBytes& s : given) {
    size_t size = 0;
    const uint8_t* data = c.section_data(s.type, &size);
    KZ_CHECK_EQ(size, s.data.size());
    const kz::SectionInfo* info = kz::find_section(s.type);
    KZ_CHECK(data != nullptr && info != nullptr && info->record != nullptr);
    if (data == nullptr || info == nullptr || info->record == nullptr) continue;
    const kz_record_desc& rec = *info->record;
    for (size_t i = 0; i < 3; ++i)
      for (uint32_t f = 0; f < rec.nfields; ++f) {
        const kz_field_desc& fd = rec.fields[f];
        const size_t at = i * rec.size + fd.offset;
        const size_t n = static_cast<size_t>(fd.elem_size) * fd.count;
        if (fd.kind != KZ_FK_STR) {
          KZ_CHECK(std::memcmp(data + at, s.data.data() + at, n) == 0);
          continue;
        }
        kz_str stored;
        std::memcpy(&stored, data + at, sizeof(stored));
        std::string text;
        KZ_CHECK(c.string(stored, &text));
        const std::string expected = std::string(rec.name) + "." + fd.name + "#" + std::to_string(s.type * 100 + i);
        KZ_CHECK(text.empty() || text == expected);
      }
  }
  // The dump is a pure function of the bytes.
  KZ_CHECK_EQ(kz::dump_container(c), kz::dump_container(c));
  // The same inputs give the same bytes (hash stability for every record type).
  kz::Digest tool2;
  KZ_CHECK(b == kzfix::make_all_sections_container(3, foreign, &tool2));
}

}  // namespace

int main() {
  test_reserved_fields();
  test_references();
  test_string_references();
  test_pattern_round_trip_of_every_record();
  return kztest::finish("test_core_verify");
}

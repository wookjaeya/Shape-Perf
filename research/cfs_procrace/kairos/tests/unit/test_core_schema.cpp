// Properties of the schema: no implicit padding, naturally aligned fields, the sizes the
// SPEC states (and the documented S0 refinements), agreement of the C99, C11 and C++ views,
// consistency of the registry (sections, enumerations, field formats), conservative zero
// values, and the SPEC's statement that no field means "safe" or "live".

#include <cstring>
#include <set>
#include <string>

#include "kz/core/registry.hpp"
#include "schema_c.h"
#include "support/kz_test.hpp"

namespace {

uint32_t natural_alignment(uint32_t kind) {
  switch (kind) {
    case KZ_FK_U8: return 1;
    case KZ_FK_U16: return 2;
    case KZ_FK_U32:
    case KZ_FK_I32:
    case KZ_FK_REF:
    case KZ_FK_STR: return 4;
    case KZ_FK_U64:
    case KZ_FK_I64: return 8;
  }
  return 0;
}

uint32_t expected_elem_size(uint32_t kind) {
  switch (kind) {
    case KZ_FK_U8: return 1;
    case KZ_FK_U16: return 2;
    case KZ_FK_U32:
    case KZ_FK_I32: return 4;
    case KZ_FK_U64:
    case KZ_FK_I64: return 8;
    case KZ_FK_REF: return sizeof(kz_ref);
    case KZ_FK_STR: return sizeof(kz_str);
  }
  return 0;
}

bool is_integer_kind(uint32_t k) { return k >= KZ_FK_U8 && k <= KZ_FK_I64; }

void check_layout_properties() {
  for (const kz_record_desc* rec : kz::records()) {
    std::set<std::string> names;
    uint32_t expect = 0;
    uint32_t max_align = 1;
    for (uint32_t i = 0; i < rec->nfields; ++i) {
      const kz_field_desc& f = rec->fields[i];
      KZ_CHECK(names.insert(f.name).second);              // field names are unique within a record
      KZ_CHECK_EQ(f.offset, expect);                      // offset = sum of the preceding sizes
      KZ_CHECK_EQ(f.elem_size, expected_elem_size(f.kind));
      KZ_CHECK(f.count >= 1);
      KZ_CHECK_EQ(f.offset % natural_alignment(f.kind), 0u);
      if (f.kind == KZ_FK_REF || f.kind == KZ_FK_STR) KZ_CHECK_EQ(f.count, 1u);
      if (natural_alignment(f.kind) > max_align) max_align = natural_alignment(f.kind);
      expect += f.elem_size * f.count;
      // Reserved fields are plain integers, named rsv<digits>.
      if (kz::is_reserved_field(f)) {
        KZ_CHECK(is_integer_kind(f.kind));
        for (const char* c = f.name + 3; *c != 0; ++c) KZ_CHECK(*c >= '0' && *c <= '9');
      }
    }
    KZ_CHECK_EQ(rec->size, expect);  // sizeof = sum of the field sizes: no padding at all
    KZ_CHECK_EQ(rec->size % max_align, 0u);
    KZ_CHECK(rec->nfields > 0);
  }
}

// SPEC 2.3 states these sizes (kz_verdict, kz_cert and kz_cert_step differ by the S0 refinements
// documented in docs/schema.md; the others are as stated).
void check_known_sizes() {
  struct Known {
    const char* name;
    uint32_t size;
  } known[] = {
      {"kz_ref", 40},       {"kz_str", 8},          {"kz_header", 160},  {"kz_parent", 40},       {"kz_section_entry", 48},
      {"kz_raw_ev", 64},    // SPEC 2.3: 64 bytes
      {"kz_bound", 192},    // SPEC 2.3: 32 bytes of fields + 4 kz_ref
      {"kz_verdict_cell", 472},  // SPEC 2.3 (rev 1.2): 24 + 8 + 11 kz_ref, with proof_status
      {"kz_verdict", 112},  // SPEC 2.3: 104, plus first_cell and rsv2
      {"kz_cert", 136},     // SPEC 2.3: 128, plus first_step and rsv1
      {"kz_cert_step", 36}, // SPEC 2.3: 32, plus flags and rsv0
      {"kz_trust", 60},     // SPEC 2.3: 60 bytes, with proof
      {"kz_trust_ledger", 224},  // SPEC 2.3: 224 bytes
  };
  for (const Known& k : known) {
    const kz_record_desc* rec = kz::find_record(k.name);
    KZ_CHECK(rec != nullptr);
    if (rec != nullptr) KZ_CHECK_EQ(rec->size, k.size);
  }
}

void check_c_views() {
  // The same layout from C99, C11 and C++, field by field.
  struct View {
    const char* name;
    const kz_c_field* fields;
    unsigned nfields;
    const kz_c_record* records;
    unsigned nrecords;
    long language;
  } views[] = {
      {"c99", kz_c99_fields, kz_c99_nfields, kz_c99_records, kz_c99_nrecords, kz_c99_language},
      {"c11", kz_c11_fields, kz_c11_nfields, kz_c11_records, kz_c11_nrecords, kz_c11_language},
  };
  KZ_CHECK_EQ(views[0].language, 199901L);
  KZ_CHECK_EQ(views[1].language, 201112L);
  size_t nfields = 0;
  for (const kz_record_desc* rec : kz::records()) nfields += rec->nfields;
  for (const View& v : views) {
    KZ_CHECK_EQ(static_cast<size_t>(v.nrecords), kz::records().size());
    KZ_CHECK_EQ(static_cast<size_t>(v.nfields), nfields);
    size_t r = 0, fi = 0;
    for (const kz_record_desc* rec : kz::records()) {
      if (r >= v.nrecords) break;
      KZ_CHECK_EQ(std::string(v.records[r].name), std::string(rec->name));
      KZ_CHECK_EQ(v.records[r].size, rec->size);
      for (uint32_t i = 0; i < rec->nfields; ++i, ++fi) {
        if (fi >= v.nfields) break;
        KZ_CHECK_EQ(std::string(v.fields[fi].record), std::string(rec->name));
        KZ_CHECK_EQ(std::string(v.fields[fi].field), std::string(rec->fields[i].name));
        KZ_CHECK_EQ(v.fields[fi].offset, rec->fields[i].offset);
        KZ_CHECK_EQ(v.fields[fi].size, rec->fields[i].elem_size);
        KZ_CHECK_EQ(v.fields[fi].count, rec->fields[i].count);
      }
      ++r;
    }
  }
}

void check_registry_sections() {
  std::set<uint32_t> ids;
  std::set<std::string> names;
  uint32_t last = 0;
  for (const kz::SectionInfo& s : kz::sections()) {
    KZ_CHECK(ids.insert(s.type).second);
    KZ_CHECK(names.insert(s.name).second);
    KZ_CHECK(s.type > last);  // ascending: the canonical section order
    last = s.type;
    KZ_CHECK(s.record != nullptr);  // every section names a record that exists
    KZ_CHECK(s.schema_version >= 1);
    KZ_CHECK(kz::find_section(s.type) == &s);
    KZ_CHECK(kz::find_section(std::string_view(s.name)) == &s);
  }
  KZ_CHECK(kz::find_section(static_cast<uint32_t>(0)) == nullptr);
  KZ_CHECK(!kz::sections().empty() && kz::sections().front().type == KZ_SEC_STRINGS);
  // Records that are not the payload of a section are the container's own structure.
  std::set<std::string> bound;
  for (const kz::SectionInfo& s : kz::sections()) bound.insert(s.record->name);
  const std::set<std::string> structural = {"kz_str", "kz_header", "kz_parent", "kz_section_entry"};
  for (const kz_record_desc* rec : kz::records())
    KZ_CHECK(bound.count(rec->name) == 1 || structural.count(rec->name) == 1);
}

void check_enums() {
  std::set<std::string> enum_names;
  for (const kz::EnumDesc& en : kz::enums()) {
    KZ_CHECK(enum_names.insert(en.name).second);
    KZ_CHECK(en.nitems > 0);
    std::set<uint64_t> values;
    std::set<std::string> ids, displays;
    for (size_t i = 0; i < en.nitems; ++i) {
      const kz::EnumItem& it = en.items[i];
      KZ_CHECK(values.insert(it.value).second);
      KZ_CHECK(ids.insert(it.id).second);
      KZ_CHECK(displays.insert(it.display).second);
      if (en.is_set) KZ_CHECK(it.value != 0 && (it.value & (it.value - 1)) == 0);  // one bit each
    }
    if (!en.is_set) {
      // Value 0 is the conservative or "unset" value: every single-valued enumeration lists it.
      bool zero = false;
      for (size_t i = 0; i < en.nitems; ++i) zero = zero || en.items[i].value == 0;
      KZ_CHECK(zero);
    }
  }
}

void check_formats() {
  std::set<std::string> used_enums;
  std::set<std::string> seen;
  for (const kz::FieldFormat& f : kz::formats()) {
    KZ_CHECK(seen.insert(std::string(f.record) + "." + f.field).second);  // one format per field
    const kz_record_desc* rec = kz::find_record(f.record);
    KZ_CHECK(rec != nullptr);
    if (rec == nullptr) continue;
    const kz_field_desc* fd = kz::find_field(*rec, f.field);
    KZ_CHECK(fd != nullptr);
    if (fd == nullptr) continue;
    KZ_CHECK(!kz::is_reserved_field(*fd));
    switch (f.fmt) {
      case kz::Fmt::Enum:
      case kz::Fmt::Flags: {
        const kz::EnumDesc* en = kz::find_enum(f.enum_name);
        KZ_CHECK(en != nullptr);
        if (en == nullptr) break;
        used_enums.insert(en->name);
        KZ_CHECK(is_integer_kind(fd->kind));
        KZ_CHECK_EQ(en->is_set, f.fmt == kz::Fmt::Flags);
        uint64_t all = 0, limit = fd->elem_size >= 8 ? ~static_cast<uint64_t>(0) : ((static_cast<uint64_t>(1) << (8 * fd->elem_size)) - 1);
        for (size_t i = 0; i < en->nitems; ++i) all |= en->items[i].value;
        KZ_CHECK(all <= limit);  // every listed value fits in the field
        break;
      }
      case kz::Fmt::Routes:
        KZ_CHECK_EQ(fd->kind, static_cast<uint32_t>(KZ_FK_U8));
        KZ_CHECK_EQ(fd->count, 1u);
        break;
      case kz::Fmt::Idx:
      case kz::Fmt::Bound:
      case kz::Fmt::Hex:
        KZ_CHECK(is_integer_kind(fd->kind));
        break;
      case kz::Fmt::Default:
        break;
    }
  }
  // Every enumeration is used by some field (KZ_ROUTE is used by the routes format).
  for (const kz::EnumDesc& en : kz::enums())
    if (std::string(en.name) != "KZ_ROUTE") KZ_CHECK(used_enums.count(en.name) == 1);
}

void check_conservative_defaults() {
  // The all-zero record must say the least: nothing established, nothing assumed good.
  auto zero_name = [](const char* enum_name) {
    const kz::EnumDesc* en = kz::find_enum(enum_name);
    const char* d = en != nullptr ? kz::enum_display(*en, 0) : nullptr;
    return std::string(d != nullptr ? d : "<missing>");
  };
  KZ_CHECK_EQ(zero_name("KZ_CLS"), std::string("unknown"));
  KZ_CHECK_EQ(zero_name("KZ_REACH"), std::string("assumed"));
  KZ_CHECK_EQ(zero_name("KZ_PROGRESS"), std::string("unknown"));
  KZ_CHECK_EQ(zero_name("KZ_PROGRESS_REASON"), std::string("not_checked"));  // D20: progress is not checked by default
  KZ_CHECK_EQ(zero_name("KZ_FW_CONF"), std::string("unknown"));
  KZ_CHECK_EQ(zero_name("KZ_PROVIDER"), std::string("unknown"));
  KZ_CHECK_EQ(zero_name("KZ_HARM"), std::string("unspecified_by_reference"));
  KZ_CHECK_EQ(zero_name("KZ_TIER"), std::string("unreferenced"));
  KZ_CHECK_EQ(zero_name("KZ_PROOF"), std::string("n/a"));
  KZ_CHECK_EQ(zero_name("KZ_OCC"), std::string("NOT_EVALUATED"));
  KZ_CHECK_EQ(zero_name("KZ_PROGRESS_HARM"), std::string("none"));
  KZ_CHECK_EQ(zero_name("KZ_ROUTE"), std::string("none"));
  KZ_CHECK_EQ(zero_name("KZ_OUTCOME_LABEL"), std::string("none"));
  // Tier names are the ones the PI fixed (T1).
  const kz::EnumDesc* tier = kz::find_enum("KZ_TIER");
  KZ_CHECK(tier != nullptr);
  std::set<std::string> t;
  for (size_t i = 0; tier != nullptr && i < tier->nitems; ++i) t.insert(tier->items[i].display);
  const std::set<std::string> want = {"contract-backed", "admitted", "code-derived", "issue-derived", "researcher", "unreferenced"};
  KZ_CHECK(t == want);
}

void check_no_safe_or_live_field() {
  // SPEC 1.10: "There is never a 'safe' field, and no field means 'live' or 'deadlock-free'."
  for (const kz_record_desc* rec : kz::records())
    for (uint32_t i = 0; i < rec->nfields; ++i) {
      const std::string n = rec->fields[i].name;
      KZ_CHECK(n != "safe" && n != "is_safe" && n != "live" && n != "is_live" && n != "alive");
      KZ_CHECK(n.find("deadlock_free") == std::string::npos && n.find("safe_") != 0);
    }
}

void check_verdict_has_proof_and_progress_fields() {
  // D20: progress and proof status are present in the records.
  const kz_record_desc* cell = kz::find_record("kz_verdict_cell");
  KZ_CHECK(cell != nullptr && kz::find_field(*cell, "progress") != nullptr && kz::find_field(*cell, "progress_reason") != nullptr);
  const kz_record_desc* trust = kz::find_record("kz_trust");
  KZ_CHECK(trust != nullptr && kz::find_field(*trust, "proof") != nullptr);
  KZ_CHECK(cell != nullptr && kz::find_field(*cell, "proof_status") != nullptr);
  const kz_record_desc* cert = kz::find_record("kz_cert");
  KZ_CHECK(cert != nullptr && kz::find_field(*cert, "proof_status") != nullptr);
}

}  // namespace

int main() {
  check_layout_properties();
  check_known_sizes();
  check_c_views();
  check_registry_sections();
  check_enums();
  check_formats();
  check_conservative_defaults();
  check_no_safe_or_live_field();
  check_verdict_has_proof_and_progress_fields();
  return kztest::finish("test_core_schema");
}

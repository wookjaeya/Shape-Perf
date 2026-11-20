// registry.cpp -- serves the tables of schema_tables.cpp.

#include "kz/core/registry.hpp"
#include "schema_tables.hpp"

#include <algorithm>
#include <cstring>

namespace kz {

namespace {

struct Registry {
  std::vector<const kz_record_desc*> records;
  std::vector<SectionInfo> sections;
  std::vector<EnumDesc> enums;
  std::vector<FieldFormat> formats;
  // resolved[i][j] is the format of field j of record i.
  std::vector<std::vector<ResolvedFormat>> resolved;
};

const kz_record_desc* lookup_record(const Registry& r, std::string_view name) {
  for (const kz_record_desc* rec : r.records)
    if (name == rec->name) return rec;
  return nullptr;
}

const EnumDesc* lookup_enum(const Registry& r, std::string_view name) {
  for (const EnumDesc& e : r.enums)
    if (name == e.name) return &e;
  return nullptr;
}

Registry build() {
  Registry r;
  for (size_t i = 0; i < tables::record_count; ++i) r.records.push_back(&tables::record_table[i]);
  for (size_t i = 0; i < tables::enum_count; ++i) r.enums.push_back(tables::enum_table[i]);
  for (size_t i = 0; i < tables::format_count; ++i) r.formats.push_back(tables::format_table[i]);
  for (size_t i = 0; i < tables::section_count; ++i) {
    const tables::RawSection& s = tables::section_table[i];
    r.sections.push_back(SectionInfo{s.type, s.name, lookup_record(r, s.record), s.schema_version});
  }
  std::sort(r.sections.begin(), r.sections.end(),
            [](const SectionInfo& a, const SectionInfo& b) { return a.type < b.type; });
  r.resolved.resize(r.records.size());
  for (size_t i = 0; i < r.records.size(); ++i) r.resolved[i].resize(r.records[i]->nfields);
  for (const FieldFormat& f : r.formats) {
    const kz_record_desc* rec = lookup_record(r, f.record);
    if (rec == nullptr) continue;  // reported by the registry unit test
    const kz_field_desc* field = nullptr;
    size_t field_index = 0;
    for (uint32_t j = 0; j < rec->nfields; ++j)
      if (std::strcmp(rec->fields[j].name, f.field) == 0) {
        field = &rec->fields[j];
        field_index = j;
      }
    if (field == nullptr) continue;
    ResolvedFormat rf;
    rf.fmt = f.fmt;
    rf.en = (f.fmt == Fmt::Enum || f.fmt == Fmt::Flags) ? lookup_enum(r, f.enum_name) : nullptr;
    size_t ri = static_cast<size_t>(std::find(r.records.begin(), r.records.end(), rec) - r.records.begin());
    r.resolved[ri][field_index] = rf;
  }
  return r;
}

const Registry& registry() {
  static const Registry r = build();
  return r;
}

}  // namespace

const std::vector<const kz_record_desc*>& records() { return registry().records; }

const kz_record_desc* find_record(std::string_view name) { return lookup_record(registry(), name); }

size_t record_index(const kz_record_desc* rec) {
  const auto& v = registry().records;
  for (size_t i = 0; i < v.size(); ++i)
    if (v[i] == rec) return i;
  return static_cast<size_t>(-1);
}

const kz_field_desc* find_field(const kz_record_desc& rec, std::string_view name) {
  for (uint32_t i = 0; i < rec.nfields; ++i)
    if (name == rec.fields[i].name) return &rec.fields[i];
  return nullptr;
}

bool is_reserved_field(const kz_field_desc& f) { return std::strncmp(f.name, "rsv", 3) == 0; }

const std::vector<SectionInfo>& sections() { return registry().sections; }

const SectionInfo* find_section(uint32_t type) {
  for (const SectionInfo& s : registry().sections)
    if (s.type == type) return &s;
  return nullptr;
}

const SectionInfo* find_section(std::string_view name) {
  for (const SectionInfo& s : registry().sections)
    if (name == s.name) return &s;
  return nullptr;
}

const std::vector<EnumDesc>& enums() { return registry().enums; }
const EnumDesc* find_enum(std::string_view name) { return lookup_enum(registry(), name); }
const std::vector<FieldFormat>& formats() { return registry().formats; }

ResolvedFormat format_of(const kz_record_desc& rec, const kz_field_desc& field) {
  const Registry& r = registry();
  size_t ri = record_index(&rec);
  if (ri == static_cast<size_t>(-1)) return ResolvedFormat{};
  size_t fi = static_cast<size_t>(&field - rec.fields);
  if (fi >= r.resolved[ri].size()) return ResolvedFormat{};
  return r.resolved[ri][fi];
}

const char* enum_display(const EnumDesc& en, uint64_t value) {
  for (size_t i = 0; i < en.nitems; ++i)
    if (en.items[i].value == value) return en.items[i].display;
  return nullptr;
}

}  // namespace kz

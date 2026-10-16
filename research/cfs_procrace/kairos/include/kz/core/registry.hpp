// registry.hpp -- run-time view of the schema: records, fields, enums, sections, formats.
//
// All tables are expanded from the X-macro lists of include/kz/schema (see
// lib/core/schema_tables.cpp); nothing here is written by hand per record.
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "kz/schema/kz_schema.h"

namespace kz {

struct EnumItem {
  uint64_t value;
  const char* display;  // text printed by the canonical dump
  const char* id;       // identifier in the header, for diagnostics
};

struct EnumDesc {
  const char* name;  // the prefix in the header, e.g. "KZ_CLS"
  bool is_set;       // a set of bit flags rather than a single value
  size_t nitems;
  const EnumItem* items;
};

enum class Fmt { Default, Enum, Flags, Hex, Idx, Bound, Routes };

struct FieldFormat {
  const char* record;
  const char* field;
  Fmt fmt;
  const char* enum_name;  // "0" when the format takes no enum
};

struct SectionInfo {
  uint32_t type;
  const char* name;
  const kz_record_desc* record;  // null only if the table names an unknown record
  uint32_t schema_version;
};

struct ResolvedFormat {
  Fmt fmt = Fmt::Default;
  const EnumDesc* en = nullptr;
};

// Records, in definition order.
const std::vector<const kz_record_desc*>& records();
const kz_record_desc* find_record(std::string_view name);
size_t record_index(const kz_record_desc* rec);  // SIZE_MAX if rec is not in the registry
const kz_field_desc* find_field(const kz_record_desc& rec, std::string_view name);
// Reserved fields are named rsv<N> and must be zero.
bool is_reserved_field(const kz_field_desc& f);

// Sections, ascending by type id (the canonical container order).
const std::vector<SectionInfo>& sections();
const SectionInfo* find_section(uint32_t type);
const SectionInfo* find_section(std::string_view name);

// Enumerations and field formats.
const std::vector<EnumDesc>& enums();
const EnumDesc* find_enum(std::string_view name);
const std::vector<FieldFormat>& formats();
ResolvedFormat format_of(const kz_record_desc& rec, const kz_field_desc& field);
// Display text of an enumerated value, or nullptr if the value is not listed.
const char* enum_display(const EnumDesc& en, uint64_t value);

}  // namespace kz

// schema_tables.cpp -- expands the X-macro lists of include/kz/schema into the tables
// that registry.cpp serves.  There is no per-record code here: adding a record or an
// enumeration to a schema header adds it to these tables.

#include <cstddef>
#include <cstdint>

#include "schema_tables.hpp"
#include "kz/core/registry.hpp"
#include "kz/schema/kz_schema.h"

namespace kz {
namespace tables {

// ---- field and record descriptors ----------------------------------------------------
#define KZ_TBL_FIELD(T, type, name, arr)                                                  \
  {#name, static_cast<uint32_t>(offsetof(T, name)), static_cast<uint32_t>(sizeof(type)), \
   static_cast<uint32_t>(sizeof(T::name) / sizeof(type)), KZ_KIND_##type},
#define KZ_TBL_FIELDS(T, LIST) static const kz_field_desc fields_##T[] = {LIST(KZ_TBL_FIELD, T)};
KZ_ALL_RECORDS(KZ_TBL_FIELDS)

#define KZ_TBL_RECORD(T, LIST) \
  {#T, static_cast<uint32_t>(sizeof(T)), static_cast<uint32_t>(sizeof(fields_##T) / sizeof(fields_##T[0])), fields_##T},
const kz_record_desc record_table[] = {KZ_ALL_RECORDS(KZ_TBL_RECORD)};
const size_t record_count = sizeof(record_table) / sizeof(record_table[0]);

// ---- enumerations ---------------------------------------------------------------------
#define KZ_TBL_ENUM_ITEM(P, id, value, display) {static_cast<uint64_t>(value), display, #id},
#define KZ_TBL_ENUM_ITEMS(P, LIST, is_set) static const EnumItem items_##P[] = {LIST(KZ_TBL_ENUM_ITEM, P)};
KZ_ALL_ENUMS(KZ_TBL_ENUM_ITEMS)

#define KZ_TBL_ENUM(P, LIST, is_set) \
  {#P, (is_set) != 0, sizeof(items_##P) / sizeof(items_##P[0]), items_##P},
const EnumDesc enum_table[] = {KZ_ALL_ENUMS(KZ_TBL_ENUM)};
const size_t enum_count = sizeof(enum_table) / sizeof(enum_table[0]);

// ---- sections -------------------------------------------------------------------------
#define KZ_TBL_SECTION(NAME, id, record, version) {id, #NAME, #record, version},
const RawSection section_table[] = {KZ_ALL_SECTIONS(KZ_TBL_SECTION)};
const size_t section_count = sizeof(section_table) / sizeof(section_table[0]);

// ---- field formats --------------------------------------------------------------------
#define KZ_FMTK_ENUM Fmt::Enum
#define KZ_FMTK_FLAGS Fmt::Flags
#define KZ_FMTK_HEX Fmt::Hex
#define KZ_FMTK_IDX Fmt::Idx
#define KZ_FMTK_BOUND Fmt::Bound
#define KZ_FMTK_ROUTES Fmt::Routes
#define KZ_TBL_FORMAT(rec, field, kind, arg) {#rec, #field, KZ_FMTK_##kind, #arg},
const FieldFormat format_table[] = {KZ_ALL_FORMATS(KZ_TBL_FORMAT)};
const size_t format_count = sizeof(format_table) / sizeof(format_table[0]);

}  // namespace tables
}  // namespace kz

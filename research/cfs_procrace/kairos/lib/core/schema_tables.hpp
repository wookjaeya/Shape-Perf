// schema_tables.hpp -- internal: the raw tables expanded in schema_tables.cpp.
#pragma once

#include <cstddef>
#include <cstdint>

#include "kz/core/registry.hpp"

namespace kz {
namespace tables {

struct RawSection {
  uint32_t type;
  const char* name;
  const char* record;  // record type name; resolved by registry.cpp
  uint32_t schema_version;
};

extern const kz_record_desc record_table[];
extern const size_t record_count;
extern const EnumDesc enum_table[];
extern const size_t enum_count;
extern const RawSection section_table[];
extern const size_t section_count;
extern const FieldFormat format_table[];
extern const size_t format_count;

}  // namespace tables
}  // namespace kz

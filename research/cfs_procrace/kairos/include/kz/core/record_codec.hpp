// record_codec.hpp -- explicit little-endian serialization of schema records (schema revision 3).
//
// The stored form of a record is defined by its field list (kz/schema): field after field at the
// field-list offset, every integer element little-endian, a kz_ref as its 32 hash bytes then section
// and index, a kz_str as offset then length.  encode_records() writes that form from host structs and
// decode_records() reads it back, element by element with shifts, so the stored bytes do not depend on
// the host's byte order or struct layout (SPEC rev 1.4 2.3 "Byte order").  On a little-endian host
// without padding (every record: kz_schema.h asserts it) the stored form equals the struct bytes; the
// container layer still uses this codec for the sections it serializes (section_is_serialized: the
// neutral event layer, kz_neutral.h), and keeps copying the bytes of the earlier sections as before.
#pragma once

#include <cstddef>
#include <cstdint>

#include "kz/schema/kz_schema.h"

namespace kz {

// True for the section ids whose records the container writer and reader pass through this codec.
bool section_is_serialized(uint32_t section_type);

// `count` records of `rec` from host structs at `src` to stored bytes at `dst` (count * rec.size bytes).
void encode_records(const kz_record_desc& rec, const void* src, size_t count, uint8_t* dst);
// Stored bytes at `src` to host structs at `dst`.
void decode_records(const kz_record_desc& rec, const uint8_t* src, size_t count, void* dst);

// Little-endian helpers for stored integers.
inline uint32_t load_le32(const uint8_t* p) {
  return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) | (static_cast<uint32_t>(p[2]) << 16) |
         (static_cast<uint32_t>(p[3]) << 24);
}
inline void store_le32(uint8_t* p, uint32_t v) {
  p[0] = static_cast<uint8_t>(v);
  p[1] = static_cast<uint8_t>(v >> 8);
  p[2] = static_cast<uint8_t>(v >> 16);
  p[3] = static_cast<uint8_t>(v >> 24);
}

}  // namespace kz

// record_codec.cpp -- explicit little-endian serialization of schema records (see record_codec.hpp).

#include "kz/core/record_codec.hpp"

#include <cstring>

namespace kz {

namespace {

// One integer element of `size` bytes: host value (read through its own type) to stored bytes.
void put_int(uint8_t* dst, const uint8_t* src, uint32_t size) {
  uint64_t v = 0;
  switch (size) {
    case 1: {
      uint8_t x;
      std::memcpy(&x, src, 1);
      v = x;
      break;
    }
    case 2: {
      uint16_t x;
      std::memcpy(&x, src, 2);
      v = x;
      break;
    }
    case 4: {
      uint32_t x;
      std::memcpy(&x, src, 4);
      v = x;
      break;
    }
    default: {
      uint64_t x;
      std::memcpy(&x, src, 8);
      v = x;
      break;
    }
  }
  for (uint32_t i = 0; i < size; ++i) dst[i] = static_cast<uint8_t>(v >> (8u * i));
}

void get_int(uint8_t* dst, const uint8_t* src, uint32_t size) {
  uint64_t v = 0;
  for (uint32_t i = 0; i < size; ++i) v |= static_cast<uint64_t>(src[i]) << (8u * i);
  switch (size) {
    case 1: {
      const uint8_t x = static_cast<uint8_t>(v);
      std::memcpy(dst, &x, 1);
      break;
    }
    case 2: {
      const uint16_t x = static_cast<uint16_t>(v);
      std::memcpy(dst, &x, 2);
      break;
    }
    case 4: {
      const uint32_t x = static_cast<uint32_t>(v);
      std::memcpy(dst, &x, 4);
      break;
    }
    default:
      std::memcpy(dst, &v, 8);
      break;
  }
}

// One element of a field: the host struct offset of the element and of its parts equals the stored
// offset (kz_schema.h asserts that every field sits at the sum of the preceding field sizes).
void code_element(const kz_field_desc& fd, uint8_t* out, const uint8_t* in, bool encode) {
  auto one = encode ? put_int : get_int;
  switch (fd.kind) {
    case KZ_FK_REF:  // 32 hash bytes, then section and index
      std::memcpy(out, in, 32);
      one(out + offsetof(kz_ref, section), in + offsetof(kz_ref, section), 4);
      one(out + offsetof(kz_ref, index), in + offsetof(kz_ref, index), 4);
      break;
    case KZ_FK_STR:
      one(out + offsetof(kz_str, off), in + offsetof(kz_str, off), 4);
      one(out + offsetof(kz_str, len), in + offsetof(kz_str, len), 4);
      break;
    default:
      one(out, in, fd.elem_size);
      break;
  }
}

void code(const kz_record_desc& rec, const uint8_t* in, size_t count, uint8_t* out, bool encode) {
  for (size_t r = 0; r < count; ++r) {
    const size_t base = r * rec.size;
    for (uint32_t f = 0; f < rec.nfields; ++f) {
      const kz_field_desc& fd = rec.fields[f];
      for (uint32_t k = 0; k < fd.count; ++k) {
        const size_t at = base + fd.offset + static_cast<size_t>(k) * fd.elem_size;
        code_element(fd, out + at, in + at, encode);
      }
    }
  }
}

}  // namespace

bool section_is_serialized(uint32_t section_type) {
  return section_type >= KZ_SEC_SERIALIZED_FIRST && section_type <= KZ_SEC_SERIALIZED_LAST;
}

void encode_records(const kz_record_desc& rec, const void* src, size_t count, uint8_t* dst) {
  code(rec, static_cast<const uint8_t*>(src), count, dst, true);
}

void decode_records(const kz_record_desc& rec, const uint8_t* src, size_t count, void* dst) {
  code(rec, src, count, static_cast<uint8_t*>(dst), false);
}

}  // namespace kz

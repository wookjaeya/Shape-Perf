// crc32.cpp -- CRC-32/IEEE.  The table is computed at compile time from the reflected
// polynomial 0xEDB88320 (the standard CRC-32 used by zip, PNG and Ethernet).

#include "kz/core/crc32.hpp"

#include <array>

namespace kz {

namespace {

constexpr uint32_t kPolynomial = 0xEDB88320u;

constexpr std::array<uint32_t, 256> make_table() {
  std::array<uint32_t, 256> t{};
  for (uint32_t i = 0; i < 256; ++i) {
    uint32_t c = i;
    for (int k = 0; k < 8; ++k) c = (c & 1u) ? (kPolynomial ^ (c >> 1)) : (c >> 1);
    t[i] = c;
  }
  return t;
}

constexpr std::array<uint32_t, 256> kTable = make_table();

}  // namespace

uint32_t crc32(const void* data, size_t size, uint32_t previous) {
  const uint8_t* p = static_cast<const uint8_t*>(data);
  uint32_t c = ~previous;
  for (size_t i = 0; i < size; ++i) c = kTable[(c ^ p[i]) & 0xffu] ^ (c >> 8);
  return ~c;
}

}  // namespace kz

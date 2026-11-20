// crc32.hpp -- CRC-32 (IEEE 802.3, reflected, polynomial 0xEDB88320), in-tree.
#pragma once

#include <cstddef>
#include <cstdint>

namespace kz {

// Incremental form: crc32(b, nb, crc32(a, na)) == crc32(a || b).
uint32_t crc32(const void* data, size_t size, uint32_t previous = 0);

}  // namespace kz

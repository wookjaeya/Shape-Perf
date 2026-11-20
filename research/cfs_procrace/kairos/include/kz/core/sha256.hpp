// sha256.hpp -- SHA-256 (FIPS 180-4) implemented in-tree; no external dependency.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace kz {

using Digest = std::array<uint8_t, 32>;

class Sha256 {
 public:
  Sha256();
  void update(const void* data, size_t size);
  void update(std::string_view text) { update(text.data(), text.size()); }
  // Finishes the hash; the object must not be updated afterwards.
  Digest finish();

 private:
  void compress(const uint8_t* block);
  uint32_t h_[8];
  uint8_t buf_[64];
  size_t buf_len_;
  uint64_t total_;
};

Digest sha256(const void* data, size_t size);
inline Digest sha256(std::string_view text) { return sha256(text.data(), text.size()); }

// Lower-case hexadecimal text.
std::string to_hex(const void* data, size_t size);
inline std::string to_hex(const Digest& d) { return to_hex(d.data(), d.size()); }
// Parses exactly 64 hex digits (either case).
bool digest_from_hex(std::string_view hex, Digest* out);

constexpr Digest kZeroDigest = {};

}  // namespace kz

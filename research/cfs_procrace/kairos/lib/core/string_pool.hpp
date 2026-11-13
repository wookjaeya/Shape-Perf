// string_pool.hpp -- internal: the canonical STRINGS payload shared by the writer and the reader.
//
// The pool of a container is a pure function of the strings its records reference:
//   byte 0 is NUL (the empty string, kz_str {0, 0}); then every distinct non-empty string
//   that some record references, sorted byte-wise (unsigned bytes, a shorter prefix first),
//   each followed by one NUL byte.  No unreferenced string, no duplicate, no shared suffix.
// So two producers that intern the same strings in a different order write the same bytes.
#pragma once

#include <cstdint>
#include <map>
#include <string>

#include "kz/core/container.hpp"

namespace kz {

// Resolves `s` against a pool of `n` bytes.  (0, 0) is the empty string; any other reference
// needs a non-zero offset, and the NUL terminator at offset + length inside the pool (so a
// string that ends exactly at the end of the pool, whose terminator would lie outside it, is
// rejected).
inline bool resolve_string(const uint8_t* pool, size_t n, const kz_str& s, std::string* out) {
  if (s.len == 0) {
    out->clear();
    return s.off == 0;
  }
  if (pool == nullptr || s.off == 0 || static_cast<uint64_t>(s.off) + s.len >= n) return false;
  if (pool[s.off + s.len] != 0) return false;
  out->assign(reinterpret_cast<const char*>(pool) + s.off, s.len);
  return true;
}

// `strings` maps each non-empty string to its offset in the canonical pool (filled in by
// canonical_pool); the returned bytes are the STRINGS payload.
inline Bytes canonical_pool(std::map<std::string, uint32_t>* strings) {
  Bytes pool;
  pool.push_back(0);
  for (auto& entry : *strings) {  // std::map orders std::string by unsigned bytes
    entry.second = static_cast<uint32_t>(pool.size());
    pool.insert(pool.end(), entry.first.begin(), entry.first.end());
    pool.push_back(0);
  }
  return pool;
}

}  // namespace kz

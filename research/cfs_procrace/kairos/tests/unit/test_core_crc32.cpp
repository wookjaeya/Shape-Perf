// CRC-32/IEEE: standard check values and incremental use.

#include <string>

#include "kz/core/crc32.hpp"
#include "support/kz_test.hpp"

int main() {
  auto crc = [](const std::string& s) { return kz::crc32(s.data(), s.size()); };
  KZ_CHECK_EQ(crc(""), 0u);
  KZ_CHECK_EQ(crc("a"), 0xe8b7be43u);
  KZ_CHECK_EQ(crc("abc"), 0x352441c2u);
  KZ_CHECK_EQ(crc("123456789"), 0xcbf43926u);  // the CRC-32 check value
  KZ_CHECK_EQ(crc("The quick brown fox jumps over the lazy dog"), 0x414fa339u);
  const std::string s = "123456789";
  for (size_t cut = 0; cut <= s.size(); ++cut) {
    uint32_t part = kz::crc32(s.data(), cut);
    KZ_CHECK_EQ(kz::crc32(s.data() + cut, s.size() - cut, part), 0xcbf43926u);
  }
  return kztest::finish("test_core_crc32");
}

// SHA-256: published vectors, block-boundary lengths against the system's sha256sum,
// streaming equivalence and hex helpers.

#include <string>

#include "kz/core/sha256.hpp"
#include "support/kz_test.hpp"

namespace {

std::string hex_of(const std::string& s) { return kz::to_hex(kz::sha256(s)); }

}  // namespace

int main() {
  // FIPS 180-4 / NIST examples.
  KZ_CHECK_EQ(hex_of(""), std::string("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));
  KZ_CHECK_EQ(hex_of("abc"), std::string("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
  KZ_CHECK_EQ(hex_of("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"),
              std::string("248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"));
  KZ_CHECK_EQ(hex_of(std::string(1000000, 'a')), std::string("cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0"));

  // Lengths around the padding boundaries; expected values from the system's sha256sum.
  struct Case {
    size_t n;
    const char* hex;
  } cases[] = {
      {55, "9f4390f8d30c2dd92ec9f095b65e2b9ae9b0a925a5258e241c9f1e910f734318"},
      {56, "b35439a4ac6f0948b6d6f9e3c6af0f5f590ce20f1bde7090ef7970686ec6738a"},
      {57, "f13b2d724659eb3bf47f2dd6af1accc87b81f09f59f2b75e5c0bed6589dfe8c6"},
      {63, "7d3e74a05d7db15bce4ad9ec0658ea98e3f06eeecf16b4c6fff2da457ddc2f34"},
      {64, "ffe054fe7ae0cb6dc65c3af9b61d5209f439851db43d0ba5997337df154668eb"},
      {65, "635361c48bb9eab14198e76ea8ab7f1a41685d6ad62aa9146d301d4f17eb0ae0"},
      {119, "31eba51c313a5c08226adf18d4a359cfdfd8d2e816b13f4af952f7ea6584dcfb"},
      {120, "2f3d335432c70b580af0e8e1b3674a7c020d683aa5f73aaaedfdc55af904c21c"},
      {128, "6836cf13bac400e9105071cd6af47084dfacad4e5e302c94bfed24e013afb73e"},
  };
  for (const Case& c : cases) KZ_CHECK_EQ(hex_of(std::string(c.n, 'a')), std::string(c.hex));

  // Streaming: every split of a message gives the one-shot digest.
  std::string msg;
  for (int i = 0; i < 200; ++i) msg.push_back(static_cast<char>('A' + (i * 7) % 26));
  const kz::Digest whole = kz::sha256(msg);
  for (size_t cut = 0; cut <= msg.size(); ++cut) {
    kz::Sha256 h;
    h.update(msg.data(), cut);
    h.update(msg.data() + cut, msg.size() - cut);
    KZ_CHECK(h.finish() == whole);
  }
  {  // byte at a time
    kz::Sha256 h;
    for (char ch : msg) h.update(&ch, 1);
    KZ_CHECK(h.finish() == whole);
  }

  {  // an empty update with a null pointer (an empty string_view or vector) changes nothing, also in the
     // middle of a block; passing null to memcpy would be undefined even for zero bytes
    kz::Sha256 h;
    h.update(nullptr, 0);
    h.update(msg.data(), 10);
    h.update(std::string_view());
    h.update(nullptr, 0);
    h.update(msg.data() + 10, msg.size() - 10);
    h.update(std::string_view());
    KZ_CHECK(h.finish() == whole);
    KZ_CHECK_EQ(kz::to_hex(kz::sha256(nullptr, 0)), std::string("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));
  }

  // Hex helpers.
  kz::Digest back;
  KZ_CHECK(kz::digest_from_hex(kz::to_hex(whole), &back));
  KZ_CHECK(back == whole);
  KZ_CHECK(kz::digest_from_hex("E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855", &back));
  KZ_CHECK_EQ(kz::to_hex(back), std::string("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));
  KZ_CHECK(!kz::digest_from_hex("abc", &back));
  KZ_CHECK(!kz::digest_from_hex(std::string(63, 'a') + "g", &back));
  KZ_CHECK(kz::kZeroDigest == kz::Digest{});
  return kztest::finish("test_core_sha256");
}

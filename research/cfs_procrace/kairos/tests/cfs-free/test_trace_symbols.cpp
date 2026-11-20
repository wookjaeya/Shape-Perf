// Unit tests of the symbol index used to name code by address.
#include <cstring>

#include "kz/core/container.hpp"
#include "kz/trace/identity_symbols.hpp"
#include "support/kz_test.hpp"

using namespace kz::trace;

namespace {

SymbolEntry sym(uint64_t v, uint64_t size, uint8_t bind, const char* name) {
  SymbolEntry e;
  e.vaddr = v;
  e.size = size;
  e.bind = bind;
  e.name = name;
  return e;
}

void test_lookup() {
  SymbolIndex idx;
  idx.add(sym(0x2000, 0x40, KZ_SYMBOL_BIND_GLOBAL, "second"));
  idx.add(sym(0x1000, 0x100, KZ_SYMBOL_BIND_GLOBAL, "first"));
  idx.add(sym(0x3000, 0, KZ_SYMBOL_BIND_GLOBAL, "marker"));  // size 0: contains only its own address
  KZ_CHECK_EQ(idx.size(), size_t(3));
  KZ_CHECK(idx.find(0xfff) == nullptr);
  KZ_CHECK_EQ(idx.find(0x1000)->name, std::string("first"));
  KZ_CHECK_EQ(idx.find(0x10ff)->name, std::string("first"));
  KZ_CHECK(idx.find(0x1100) == nullptr);  // the gap after "first"
  KZ_CHECK_EQ(idx.find(0x2000)->name, std::string("second"));
  KZ_CHECK_EQ(idx.find(0x203f)->name, std::string("second"));
  KZ_CHECK(idx.find(0x2040) == nullptr);
  KZ_CHECK_EQ(idx.find(0x3000)->name, std::string("marker"));
  KZ_CHECK(idx.find(0x3001) == nullptr);
  KZ_CHECK(idx.find(0xffffffffffffffffull) == nullptr);
}

void test_aliases_are_chosen_the_same_way_every_time() {
  // Three names for the same code: global before weak before local, then the larger size, then the smaller name.
  for (int order = 0; order < 6; ++order) {
    SymbolIndex idx;
    SymbolEntry a = sym(0x1000, 0x20, KZ_SYMBOL_BIND_LOCAL, "aaa_local");
    SymbolEntry b = sym(0x1000, 0x20, KZ_SYMBOL_BIND_WEAK, "bbb_weak");
    SymbolEntry c = sym(0x1000, 0x20, KZ_SYMBOL_BIND_GLOBAL, "zzz_global");
    SymbolEntry d = sym(0x1000, 0x20, KZ_SYMBOL_BIND_GLOBAL, "yyy_global");
    const SymbolEntry* seq[4] = {&a, &b, &c, &d};
    // a fixed set of permutations
    static const int perms[6][4] = {{0, 1, 2, 3}, {3, 2, 1, 0}, {1, 3, 0, 2}, {2, 0, 3, 1}, {0, 3, 1, 2}, {2, 1, 3, 0}};
    for (int k = 0; k < 4; ++k) idx.add(*seq[perms[order][k]]);
    KZ_CHECK_EQ(idx.find(0x1010)->name, std::string("yyy_global"));
  }
  SymbolIndex sizes;
  sizes.add(sym(0x1000, 0x10, KZ_SYMBOL_BIND_GLOBAL, "short"));
  sizes.add(sym(0x1000, 0x30, KZ_SYMBOL_BIND_GLOBAL, "long"));
  KZ_CHECK_EQ(sizes.find(0x1020)->name, std::string("long"));  // the larger one covers the address
}

void test_from_container() {
  kz::ContainerWriter w(KZ_ARTIFACT_BASIS);
  std::vector<kz_symbol> syms(2);
  std::memset(syms.data(), 0, syms.size() * sizeof(kz_symbol));
  syms[0].vaddr = 0x1000; syms[0].size = 0x20; syms[0].bind = KZ_SYMBOL_BIND_GLOBAL; syms[0].name = w.intern("alpha");
  syms[1].vaddr = 0x2000; syms[1].size = 0x20; syms[1].bind = KZ_SYMBOL_BIND_LOCAL; syms[1].name = w.intern("beta");
  KZ_CHECK_OK(w.add_section(KZ_SEC_SYMBOLS, syms));
  kz::Bytes bytes;
  KZ_CHECK_OK(w.finish(&bytes));
  kz::Container c;
  KZ_CHECK_OK(kz::Container::parse(bytes, &c));
  KZ_CHECK_OK(c.verify_content());
  SymbolIndex idx;
  KZ_CHECK_OK(load_symbols(c, &idx));
  KZ_CHECK_EQ(idx.size(), size_t(2));
  KZ_CHECK_EQ(idx.find(0x2010)->name, std::string("beta"));
  KZ_CHECK_EQ(idx.find(0x2010)->bind, uint8_t(KZ_SYMBOL_BIND_LOCAL));
  // a container without the section: no symbols, not an error
  kz::ContainerWriter w2(KZ_ARTIFACT_BASIS);
  kz::Bytes b2;
  KZ_CHECK_OK(w2.finish(&b2));
  kz::Container c2;
  KZ_CHECK_OK(kz::Container::parse(b2, &c2));
  SymbolIndex empty;
  KZ_CHECK_OK(load_symbols(c2, &empty));
  KZ_CHECK_EQ(empty.size(), size_t(0));
}

void test_link_address() {
  std::vector<kz_map_ev> maps(3);
  std::memset(maps.data(), 0, maps.size() * sizeof(kz_map_ev));
  maps[0].pid = 7; maps[0].addr = 0x500000001000; maps[0].len = 0x1000; maps[0].bias = 0x500000000000; maps[0].bias_known = 1;
  maps[1].pid = 7; maps[1].addr = 0x500000003000; maps[1].len = 0x1000; maps[1].bias_known = 0;  // no site in this one: the bias of the other applies
  maps[2].pid = 8; maps[2].addr = 0x700000001000; maps[2].len = 0x1000; maps[2].bias_known = 0;  // another process, no bias at all
  uint64_t out = 0;
  KZ_CHECK(link_address(maps, 7, 0x500000001234, &out));
  KZ_CHECK_EQ(out, uint64_t(0x1234));
  KZ_CHECK(link_address(maps, 7, 0x500000003010, &out));
  KZ_CHECK_EQ(out, uint64_t(0x3010));
  KZ_CHECK(!link_address(maps, 7, 0x500000000fff, &out));  // outside every mapping
  KZ_CHECK(!link_address(maps, 7, 0x500000002000, &out));  // between the two mappings
  KZ_CHECK(!link_address(maps, 8, 0x700000001010, &out));  // the process has no known bias
  KZ_CHECK(!link_address(maps, 9, 0x500000001234, &out));  // another process
}

}  // namespace

int main() {
  test_lookup();
  test_aliases_are_chosen_the_same_way_every_time();
  test_from_container();
  test_link_address();
  return kztest::finish("test_trace_symbols");
}

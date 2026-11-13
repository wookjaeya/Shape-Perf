// identity_symbols.cpp -- see include/kz/trace/identity_symbols.hpp.
#include "kz/trace/identity_symbols.hpp"

#include <algorithm>

namespace kz::trace {

namespace {

int binding_rank(uint8_t bind) {
  switch (bind) {
    case KZ_SYMBOL_BIND_GLOBAL: return 0;
    case KZ_SYMBOL_BIND_WEAK: return 1;
    default: return 2;
  }
}

}  // namespace

void SymbolIndex::finish() {
  // Order: address ascending; among equal addresses the preferred alias first, so that the
  // first entry of an address group is the one a lookup returns.
  std::sort(entries_.begin(), entries_.end(), [](const SymbolEntry& a, const SymbolEntry& b) {
    if (a.vaddr != b.vaddr) return a.vaddr < b.vaddr;
    const int ra = binding_rank(a.bind), rb = binding_rank(b.bind);
    if (ra != rb) return ra < rb;
    if (a.size != b.size) return a.size > b.size;
    return a.name < b.name;
  });
  sorted_ = true;
}

const SymbolEntry* SymbolIndex::find(uint64_t addr) const {
  if (!sorted_) const_cast<SymbolIndex*>(this)->finish();
  // The last entry group whose address is <= addr.
  auto it = std::upper_bound(entries_.begin(), entries_.end(), addr, [](uint64_t v, const SymbolEntry& e) { return v < e.vaddr; });
  while (it != entries_.begin()) {
    --it;
    // `it` is the last entry at its address in sorted order; the preferred alias is the first.
    auto first = it;
    while (first != entries_.begin() && (first - 1)->vaddr == it->vaddr) --first;
    const bool inside = addr == first->vaddr || addr < first->vaddr + first->size;
    if (inside) return &*first;
    // A zero-sized or short symbol that does not reach `addr`: an enclosing larger symbol at a
    // lower address might still contain it, but function symbols do not nest, so stop.
    return nullptr;
  }
  return nullptr;
}

bool link_address(const std::vector<kz_map_ev>& maps, uint32_t pid, uint64_t runtime_addr, uint64_t* link_addr) {
  // The bias is a property of the file's load in the process: take it from any mapping of the
  // process that has one, and accept an address that lies in a mapping of the traced file.
  bool have_bias = false, inside = false;
  uint64_t bias = 0;
  for (const kz_map_ev& m : maps) {
    if (m.pid != pid) continue;
    if (m.bias_known != 0 && !have_bias) {
      have_bias = true;
      bias = m.bias;
    }
    if (runtime_addr >= m.addr && runtime_addr - m.addr < m.len) inside = true;
  }
  if (!have_bias || !inside) return false;
  *link_addr = runtime_addr - bias;
  return true;
}

Status load_symbols(const Container& basis, SymbolIndex* out) {
  std::vector<kz_symbol> syms;
  Status s = basis.read_section(KZ_SEC_SYMBOLS, &syms);
  if (!s.ok()) return s;
  for (const kz_symbol& k : syms) {
    SymbolEntry e;
    e.vaddr = k.vaddr;
    e.size = k.size;
    e.bind = k.bind;
    if (!basis.string(k.name, &e.name)) return Status::error(Err::BadRef, "SYMBOLS: a name does not resolve");
    out->add(std::move(e));
  }
  out->finish();
  return Status::success();
}

}  // namespace kz::trace

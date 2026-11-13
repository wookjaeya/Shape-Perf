// identity_symbols.hpp -- function symbols of the traced binary, for naming the code that an
// address belongs to (the creating function of a thread, the caller named by a return address).
//
// Addresses are link-time addresses of the file; a runtime address is first reduced by the
// load bias (decode_merge.hpp).  Two symbols may name the same code (aliases): the choice is
// fixed -- the stronger binding first, then the smaller name -- so that equal inputs give
// equal output.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "kz/core/container.hpp"
#include "kz/core/status.hpp"
#include "kz/schema/kz_schema.h"

namespace kz::trace {

struct SymbolEntry {
  uint64_t vaddr = 0;
  uint64_t size = 0;
  uint8_t bind = KZ_SYMBOL_BIND_LOCAL;
  std::string name;
};

class SymbolIndex {
 public:
  void add(SymbolEntry e) { entries_.push_back(std::move(e)); sorted_ = false; }
  // Sorts and fixes the choice among aliases.  Called by the lookups on demand.
  void finish();
  // The symbol whose [vaddr, vaddr + size) contains `addr`; null if there is none.  A symbol of
  // size 0 contains only its own address.
  const SymbolEntry* find(uint64_t addr) const;
  size_t size() const { return entries_.size(); }

 private:
  mutable std::vector<SymbolEntry> entries_;
  mutable bool sorted_ = true;
};

// Reads the SYMBOLS section of a basis container.
Status load_symbols(const Container& basis, SymbolIndex* out);

// The link-time address of a runtime address of process `pid`, through the load bias of the
// mapping (kz_map_ev) of the traced file that contains it.  False if no mapping of that process
// with a known bias contains the address.
bool link_address(const std::vector<kz_map_ev>& maps, uint32_t pid, uint64_t runtime_addr, uint64_t* link_addr);

}  // namespace kz::trace

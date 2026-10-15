// elf_image.cpp -- see elf_image.hpp.
#include "elf_image.hpp"

#include <elf.h>

#include <algorithm>
#include <cstring>

#include "kz/schema/kz_schema.h"

namespace kz::anchor {

namespace {

bool in_file(const Bytes& b, uint64_t off, uint64_t n) { return off <= b.size() && n <= b.size() - off; }

template <class T>
bool read_struct(const Bytes& b, uint64_t off, T* out) {
  if (!in_file(b, off, sizeof(T))) return false;
  std::memcpy(out, b.data() + off, sizeof(T));
  return true;
}

// A NUL-terminated string at `off` inside [base, base + size).
bool c_string_in(const Bytes& b, uint64_t base, uint64_t size, uint64_t off, std::string* out) {
  if (off >= size || !in_file(b, base, size)) return false;
  const uint8_t* start = b.data() + base + off;
  const uint8_t* end = b.data() + base + size;
  const uint8_t* nul = static_cast<const uint8_t*>(std::memchr(start, 0, static_cast<size_t>(end - start)));
  if (nul == nullptr) return false;
  out->assign(reinterpret_cast<const char*>(start), static_cast<size_t>(nul - start));
  return true;
}

Status bad(const std::string& what) { return Status::error(Err::Invalid, "ELF: " + what); }

}  // namespace

std::string to_hex_string(const uint8_t* p, size_t n) {
  static const char* d = "0123456789abcdef";
  std::string s;
  s.reserve(n * 2);
  for (size_t i = 0; i < n; ++i) {
    s.push_back(d[p[i] >> 4]);
    s.push_back(d[p[i] & 15]);
  }
  return s;
}

Status ElfImage::parse(Bytes bytes, ElfImage* out) {
  *out = ElfImage();
  out->bytes_ = std::move(bytes);
  const Bytes& b = out->bytes_;
  Elf64_Ehdr eh;
  if (!read_struct(b, 0, &eh)) return bad("file shorter than an ELF header");
  if (std::memcmp(eh.e_ident, ELFMAG, SELFMAG) != 0) return bad("not an ELF file");
  if (eh.e_ident[EI_CLASS] != ELFCLASS64 || eh.e_ident[EI_DATA] != ELFDATA2LSB) return bad("only little-endian ELF64 is supported");
  if (eh.e_machine != EM_X86_64) return bad("only x86-64 is supported (the probe sites and instruction classes are x86-64)");
  if (eh.e_type != ET_EXEC && eh.e_type != ET_DYN) return bad("neither an executable nor a position-independent executable");
  out->type_ = eh.e_type;

  // Program headers.
  if (eh.e_phnum != 0) {
    if (eh.e_phentsize != sizeof(Elf64_Phdr)) return bad("unexpected program header size");
    for (uint32_t i = 0; i < eh.e_phnum; ++i) {
      Elf64_Phdr ph;
      if (!read_struct(b, eh.e_phoff + static_cast<uint64_t>(i) * sizeof ph, &ph)) return bad("program header outside the file");
      if (ph.p_type != PT_LOAD) continue;
      if (!in_file(b, ph.p_offset, ph.p_filesz)) return bad("a loadable segment lies outside the file");
      if (ph.p_filesz > ph.p_memsz) return bad("a loadable segment has more file bytes than memory bytes");
      ElfSegment s;
      s.vaddr = ph.p_vaddr; s.offset = ph.p_offset; s.filesz = ph.p_filesz; s.memsz = ph.p_memsz; s.flags = ph.p_flags;
      out->segments_.push_back(s);
    }
  }

  // Section headers.
  std::vector<Elf64_Shdr> shdr;
  if (eh.e_shnum != 0) {
    if (eh.e_shentsize != sizeof(Elf64_Shdr)) return bad("unexpected section header size");
    shdr.resize(eh.e_shnum);
    for (uint32_t i = 0; i < eh.e_shnum; ++i)
      if (!read_struct(b, eh.e_shoff + static_cast<uint64_t>(i) * sizeof(Elf64_Shdr), &shdr[i])) return bad("section header outside the file");
    if (eh.e_shstrndx >= shdr.size()) return bad("section name table index out of range");
    const Elf64_Shdr& names = shdr[eh.e_shstrndx];
    if (names.sh_type != SHT_NOBITS && !in_file(b, names.sh_offset, names.sh_size)) return bad("section name table outside the file");
    for (const Elf64_Shdr& sh : shdr) {
      Sec s;
      if (!c_string_in(b, names.sh_offset, names.sh_size, sh.sh_name, &s.name)) return bad("section name outside the name table");
      s.offset = sh.sh_offset;
      s.size = sh.sh_type == SHT_NOBITS ? 0 : sh.sh_size;
      s.type = sh.sh_type;
      if (sh.sh_type != SHT_NOBITS && !in_file(b, s.offset, s.size)) return bad("section " + s.name + " lies outside the file");
      out->sections_.push_back(std::move(s));
    }
  }

  // Build id: the GNU note.
  for (const Sec& s : out->sections_) {
    if (s.type != SHT_NOTE) continue;
    uint64_t pos = 0;
    while (pos + sizeof(Elf64_Nhdr) <= s.size) {
      Elf64_Nhdr nh;
      std::memcpy(&nh, b.data() + s.offset + pos, sizeof nh);
      const uint64_t name_off = pos + sizeof nh;
      const uint64_t desc_off = name_off + ((static_cast<uint64_t>(nh.n_namesz) + 3) & ~3ull);
      const uint64_t next = desc_off + ((static_cast<uint64_t>(nh.n_descsz) + 3) & ~3ull);
      if (desc_off > s.size || nh.n_descsz > s.size - desc_off || next > s.size + 3) break;
      if (nh.n_type == NT_GNU_BUILD_ID && nh.n_namesz == 4 && std::memcmp(b.data() + s.offset + name_off, "GNU", 4) == 0) {
        out->build_id_ = to_hex_string(b.data() + s.offset + desc_off, nh.n_descsz);
        break;
      }
      pos = next;
    }
    if (!out->build_id_.empty()) break;
  }

  // Function symbols from .symtab.
  for (uint32_t i = 0; i < shdr.size(); ++i) {
    if (shdr[i].sh_type != SHT_SYMTAB) continue;
    const Elf64_Shdr& st = shdr[i];
    if (st.sh_entsize != sizeof(Elf64_Sym) || st.sh_link >= shdr.size()) return bad("symbol table is malformed");
    const Elf64_Shdr& strtab = shdr[st.sh_link];
    if (!in_file(b, strtab.sh_offset, strtab.sh_size)) return bad("string table outside the file");
    const uint64_t n = st.sh_size / sizeof(Elf64_Sym);
    for (uint64_t k = 0; k < n; ++k) {
      Elf64_Sym sym;
      if (!read_struct(b, st.sh_offset + k * sizeof sym, &sym)) return bad("symbol outside the file");
      if (ELF64_ST_TYPE(sym.st_info) != STT_FUNC || sym.st_shndx == SHN_UNDEF) continue;
      ElfFunction f;
      if (!c_string_in(b, strtab.sh_offset, strtab.sh_size, sym.st_name, &f.name)) return bad("symbol name outside the string table");
      if (f.name.empty()) continue;
      f.vaddr = sym.st_value;
      f.size = sym.st_size;
      f.shndx = sym.st_shndx;
      const unsigned bind = ELF64_ST_BIND(sym.st_info);
      f.bind = bind == STB_GLOBAL ? KZ_SYMBOL_BIND_GLOBAL : bind == STB_WEAK ? KZ_SYMBOL_BIND_WEAK : KZ_SYMBOL_BIND_LOCAL;
      out->functions_.push_back(std::move(f));
    }
  }
  std::sort(out->functions_.begin(), out->functions_.end(), [](const ElfFunction& a, const ElfFunction& c) {
    if (a.vaddr != c.vaddr) return a.vaddr < c.vaddr;
    return a.name < c.name;
  });
  return Status::success();
}

bool ElfImage::vaddr_to_offset(uint64_t vaddr, uint64_t* offset) const {
  for (const ElfSegment& s : segments_)
    if (vaddr >= s.vaddr && vaddr - s.vaddr < s.filesz) {
      *offset = s.offset + (vaddr - s.vaddr);
      return true;
    }
  return false;
}

bool ElfImage::read_offset(uint64_t offset, size_t n, uint8_t* out) const {
  if (!in_file(bytes_, offset, n)) return false;
  std::memcpy(out, bytes_.data() + offset, n);
  return true;
}

bool ElfImage::read_vaddr(uint64_t vaddr, size_t n, uint8_t* out) const {
  for (const ElfSegment& s : segments_)
    if (vaddr >= s.vaddr && vaddr - s.vaddr < s.filesz) {
      if (n > s.filesz - (vaddr - s.vaddr)) return false;  // would run past the file-backed bytes of the segment
      return read_offset(s.offset + (vaddr - s.vaddr), n, out);
    }
  return false;
}

Slice ElfImage::section(std::string_view name) const {
  for (const Sec& s : sections_)
    if (s.name == name) return Slice{bytes_.data() + s.offset, static_cast<size_t>(s.size)};
  return Slice();
}

std::vector<const ElfFunction*> ElfImage::find_functions(std::string_view name) const {
  std::vector<const ElfFunction*> v;
  for (const ElfFunction& f : functions_)
    if (f.name == name) v.push_back(&f);
  return v;
}

const ElfFunction* ElfImage::function_at(uint64_t vaddr) const {
  const ElfFunction* best = nullptr;
  for (const ElfFunction& f : functions_) {
    if (f.vaddr > vaddr) break;
    if (vaddr - f.vaddr < f.size || (f.size == 0 && f.vaddr == vaddr)) {
      // Among aliases at one address prefer the stronger binding (global, weak, local), then the
      // earlier name: the list is sorted by (address, name), so the first of equals is kept.
      auto rank = [](uint8_t bind) { return bind == KZ_SYMBOL_BIND_GLOBAL ? 0 : bind == KZ_SYMBOL_BIND_WEAK ? 1 : 2; };
      if (best == nullptr || f.vaddr > best->vaddr || (f.vaddr == best->vaddr && rank(f.bind) < rank(best->bind))) best = &f;
    }
  }
  return best;
}

}  // namespace kz::anchor

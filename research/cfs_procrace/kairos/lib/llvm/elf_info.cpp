// elf_info.cpp -- see elf_info.hpp.
#include "elf_info.hpp"

#include <elf.h>

#include <cstring>

#include "kz/core/container.hpp"

namespace kzs {

namespace {

struct View {
  const uint8_t* p;
  size_t n;
  bool in(uint64_t off, uint64_t len) const { return off <= n && len <= n - off; }
};

kz::Status bad(const std::string& path, const std::string& what) { return kz::Status::error(kz::Err::Invalid, path + ": " + what); }

// Section header `index`, bounds checked.
bool section_at(const View& v, const Elf64_Ehdr& eh, uint32_t index, Elf64_Shdr* out) {
  if (index >= eh.e_shnum) return false;
  const uint64_t off = eh.e_shoff + static_cast<uint64_t>(index) * eh.e_shentsize;
  if (eh.e_shentsize < sizeof(Elf64_Shdr) || !v.in(off, sizeof(Elf64_Shdr))) return false;
  std::memcpy(out, v.p + off, sizeof(Elf64_Shdr));
  return true;
}

kz::Status load(const std::string& path, kz::Bytes* bytes, Elf64_Ehdr* eh) {
  kz::Status s = kz::read_whole_file(path, bytes);
  if (!s.ok()) return s;
  if (bytes->size() < sizeof(Elf64_Ehdr)) return bad(path, "too short for an ELF header");
  std::memcpy(eh, bytes->data(), sizeof *eh);
  if (std::memcmp(eh->e_ident, ELFMAG, SELFMAG) != 0) return bad(path, "not an ELF file");
  if (eh->e_ident[EI_CLASS] != ELFCLASS64 || eh->e_ident[EI_DATA] != ELFDATA2LSB) return bad(path, "not a 64-bit little-endian ELF file");
  return kz::Status::success();
}

std::string cstr_at(const View& v, uint64_t off, uint64_t limit_end) {
  std::string s;
  while (off < limit_end && v.in(off, 1) && v.p[off] != 0) s.push_back(static_cast<char>(v.p[off++]));
  return s;
}

}  // namespace

kz::Status read_object_symbols(const std::string& path, ObjectSymbols* out) {
  kz::Bytes bytes;
  Elf64_Ehdr eh;
  kz::Status st = load(path, &bytes, &eh);
  if (!st.ok()) return st;
  if (eh.e_type != ET_REL) return bad(path, "not a relocatable object");
  const View v{bytes.data(), bytes.size()};
  for (uint32_t i = 0; i < eh.e_shnum; ++i) {
    Elf64_Shdr sh;
    if (!section_at(v, eh, i, &sh)) return bad(path, "section header out of range");
    if (sh.sh_type != SHT_SYMTAB) continue;
    Elf64_Shdr strsh;
    if (!section_at(v, eh, sh.sh_link, &strsh) || !v.in(strsh.sh_offset, strsh.sh_size)) return bad(path, "string table out of range");
    if (!v.in(sh.sh_offset, sh.sh_size) || sh.sh_entsize < sizeof(Elf64_Sym)) return bad(path, "symbol table out of range");
    const uint64_t count = sh.sh_size / sh.sh_entsize;
    for (uint64_t k = 1; k < count; ++k) {
      Elf64_Sym sym;
      std::memcpy(&sym, v.p + sh.sh_offset + k * sh.sh_entsize, sizeof sym);
      const unsigned bind = ELF64_ST_BIND(sym.st_info);
      const unsigned type = ELF64_ST_TYPE(sym.st_info);
      if (bind != STB_GLOBAL && bind != STB_WEAK && bind != STB_GNU_UNIQUE) continue;
      if (type == STT_SECTION || type == STT_FILE) continue;
      if (sym.st_name >= strsh.sh_size) return bad(path, "symbol name out of range");
      const std::string name = cstr_at(v, strsh.sh_offset + sym.st_name, strsh.sh_offset + strsh.sh_size);
      if (name.empty()) continue;
      if (sym.st_shndx == SHN_UNDEF) {
        if (bind != STB_WEAK) out->undefined.insert(name);
      } else {
        out->defined.insert(name);
      }
    }
  }
  return kz::Status::success();
}

kz::Status read_elf_notes(const std::string& path, ElfNotes* out) {
  kz::Bytes bytes;
  Elf64_Ehdr eh;
  kz::Status st = load(path, &bytes, &eh);
  if (!st.ok()) return st;
  out->type = eh.e_type == ET_REL ? "relocatable" : eh.e_type == ET_EXEC ? "executable" : eh.e_type == ET_DYN ? "ET_DYN (a position-independent executable or a shared object)" : "other";
  const View v{bytes.data(), bytes.size()};
  Elf64_Shdr shstr;
  if (!section_at(v, eh, eh.e_shstrndx, &shstr) || !v.in(shstr.sh_offset, shstr.sh_size)) return bad(path, "section name table out of range");
  for (uint32_t i = 0; i < eh.e_shnum; ++i) {
    Elf64_Shdr sh;
    if (!section_at(v, eh, i, &sh)) return bad(path, "section header out of range");
    if (sh.sh_name >= shstr.sh_size) continue;
    const std::string name = cstr_at(v, shstr.sh_offset + sh.sh_name, shstr.sh_offset + shstr.sh_size);
    if (sh.sh_type == SHT_NOBITS || !v.in(sh.sh_offset, sh.sh_size)) continue;
    if (name == ".comment") {
      uint64_t off = sh.sh_offset;
      const uint64_t end = sh.sh_offset + sh.sh_size;
      while (off < end) {
        std::string s = cstr_at(v, off, end);
        off += s.size() + 1;
        if (s.empty()) continue;
        bool dup = false;
        for (const std::string& c : out->comments) dup |= (c == s);
        if (!dup) out->comments.push_back(s);
      }
    } else if (name == ".note.gnu.build-id") {
      // Elf64_Nhdr {namesz, descsz, type}, name padded to 4, then the descriptor
      uint64_t off = sh.sh_offset;
      const uint64_t end = sh.sh_offset + sh.sh_size;
      while (off + sizeof(Elf64_Nhdr) <= end) {
        Elf64_Nhdr nh;
        std::memcpy(&nh, v.p + off, sizeof nh);
        const uint64_t name_off = off + sizeof nh;
        const uint64_t desc_off = name_off + ((nh.n_namesz + 3u) & ~3u);
        const uint64_t next = desc_off + ((nh.n_descsz + 3u) & ~3u);
        if (next > end || !v.in(desc_off, nh.n_descsz)) break;
        if (nh.n_type == NT_GNU_BUILD_ID) {
          out->build_id_hex = kz::to_hex(v.p + desc_off, nh.n_descsz);
          break;
        }
        off = next;
      }
    }
  }
  return kz::Status::success();
}

}  // namespace kzs

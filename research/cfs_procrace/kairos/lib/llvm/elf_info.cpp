// elf_info.cpp -- see elf_info.hpp.
#include "elf_info.hpp"

#include <elf.h>

#include <cstring>

#include "kz/core/container.hpp"

namespace kzs {

namespace {

// The headers of one ELF class (the readers below are the same for both; <elf.h> names the fields alike).
struct Elf64Class {
  using Ehdr = Elf64_Ehdr;
  using Shdr = Elf64_Shdr;
  using Sym = Elf64_Sym;
  using Nhdr = Elf64_Nhdr;
  static unsigned bind(unsigned char info) { return ELF64_ST_BIND(info); }
  static unsigned type(unsigned char info) { return ELF64_ST_TYPE(info); }
  static unsigned visibility(unsigned char other) { return ELF64_ST_VISIBILITY(other); }
};
struct Elf32Class {
  using Ehdr = Elf32_Ehdr;
  using Shdr = Elf32_Shdr;
  using Sym = Elf32_Sym;
  using Nhdr = Elf32_Nhdr;
  static unsigned bind(unsigned char info) { return ELF32_ST_BIND(info); }
  static unsigned type(unsigned char info) { return ELF32_ST_TYPE(info); }
  static unsigned visibility(unsigned char other) { return ELF32_ST_VISIBILITY(other); }
};

struct View {
  const uint8_t* p;
  size_t n;
  bool in(uint64_t off, uint64_t len) const { return off <= n && len <= n - off; }
};

kz::Status bad(const std::string& path, const std::string& what) { return kz::Status::error(kz::Err::Invalid, path + ": " + what); }

// Section header `index`, bounds checked.
template <typename C>
bool section_at(const View& v, const typename C::Ehdr& eh, uint32_t index, typename C::Shdr* out) {
  if (index >= eh.e_shnum) return false;
  const uint64_t off = static_cast<uint64_t>(eh.e_shoff) + static_cast<uint64_t>(index) * eh.e_shentsize;
  if (eh.e_shentsize < sizeof(typename C::Shdr) || !v.in(off, sizeof(typename C::Shdr))) return false;
  std::memcpy(out, v.p + off, sizeof(typename C::Shdr));
  return true;
}

// Reads the file and its identity: ELF32 or ELF64, little-endian.
kz::Status load(const std::string& path, kz::Bytes* bytes, ElfId* id) {
  kz::Status s = kz::read_whole_file(path, bytes);
  if (!s.ok()) return s;
  if (bytes->size() < EI_NIDENT) return bad(path, "too short for an ELF header");
  const uint8_t* e = bytes->data();
  if (std::memcmp(e, ELFMAG, SELFMAG) != 0) return bad(path, "not an ELF file");
  if ((e[EI_CLASS] != ELFCLASS64 && e[EI_CLASS] != ELFCLASS32) || e[EI_DATA] != ELFDATA2LSB) return bad(path, "not a 32-bit or 64-bit little-endian ELF file");
  const size_t need = e[EI_CLASS] == ELFCLASS64 ? sizeof(Elf64_Ehdr) : sizeof(Elf32_Ehdr);
  if (bytes->size() < need) return bad(path, "too short for an ELF header");
  id->elf_class = e[EI_CLASS];
  id->data = e[EI_DATA];
  uint16_t machine = 0;
  std::memcpy(&machine, e + EI_NIDENT + 2, sizeof machine);  // e_machine follows e_type in both classes
  id->machine = machine;
  return kz::Status::success();
}

std::string cstr_at(const View& v, uint64_t off, uint64_t limit_end) {
  std::string s;
  while (off < limit_end && v.in(off, 1) && v.p[off] != 0) s.push_back(static_cast<char>(v.p[off++]));
  return s;
}

template <typename C>
kz::Status object_symbols(const std::string& path, const kz::Bytes& bytes, ObjectSymbols* out) {
  typename C::Ehdr eh;
  std::memcpy(&eh, bytes.data(), sizeof eh);
  if (eh.e_type != ET_REL) return bad(path, "not a relocatable object");
  const View v{bytes.data(), bytes.size()};
  for (uint32_t i = 0; i < eh.e_shnum; ++i) {
    typename C::Shdr sh;
    if (!section_at<C>(v, eh, i, &sh)) return bad(path, "section header out of range");
    if (sh.sh_type != SHT_SYMTAB) continue;
    typename C::Shdr strsh;
    if (!section_at<C>(v, eh, sh.sh_link, &strsh) || !v.in(strsh.sh_offset, strsh.sh_size)) return bad(path, "string table out of range");
    if (!v.in(sh.sh_offset, sh.sh_size) || sh.sh_entsize < sizeof(typename C::Sym)) return bad(path, "symbol table out of range");
    const uint64_t count = sh.sh_size / sh.sh_entsize;
    for (uint64_t k = 1; k < count; ++k) {
      typename C::Sym sym;
      std::memcpy(&sym, v.p + sh.sh_offset + k * sh.sh_entsize, sizeof sym);
      const unsigned bind = C::bind(sym.st_info);
      const unsigned type = C::type(sym.st_info);
      if (bind != STB_GLOBAL && bind != STB_WEAK && bind != STB_GNU_UNIQUE) continue;
      if (type == STT_SECTION || type == STT_FILE) continue;
      if (sym.st_name >= strsh.sh_size) return bad(path, "symbol name out of range");
      const std::string name = cstr_at(v, static_cast<uint64_t>(strsh.sh_offset) + sym.st_name, static_cast<uint64_t>(strsh.sh_offset) + strsh.sh_size);
      if (name.empty()) continue;
      if (sym.st_shndx == SHN_UNDEF) {
        if (bind != STB_WEAK) out->undefined.insert(name);
      } else {
        out->defined.insert(name);
        const unsigned vis = C::visibility(sym.st_other);
        if (vis == STV_HIDDEN || vis == STV_INTERNAL) out->hidden.insert(name);
      }
    }
  }
  return kz::Status::success();
}

template <typename C>
kz::Status dynamic_symbols(const std::string& path, const kz::Bytes& bytes, ObjectSymbols* out) {
  typename C::Ehdr eh;
  std::memcpy(&eh, bytes.data(), sizeof eh);
  if (eh.e_type != ET_DYN && eh.e_type != ET_EXEC) return bad(path, "not a linked executable or shared object");
  const View v{bytes.data(), bytes.size()};
  bool found = false;
  for (uint32_t i = 0; i < eh.e_shnum; ++i) {
    typename C::Shdr sh;
    if (!section_at<C>(v, eh, i, &sh)) return bad(path, "section header out of range");
    if (sh.sh_type != SHT_DYNSYM) continue;
    found = true;
    typename C::Shdr strsh;
    if (!section_at<C>(v, eh, sh.sh_link, &strsh) || !v.in(strsh.sh_offset, strsh.sh_size)) return bad(path, "dynamic string table out of range");
    if (!v.in(sh.sh_offset, sh.sh_size) || sh.sh_entsize < sizeof(typename C::Sym)) return bad(path, "dynamic symbol table out of range");
    const uint64_t count = sh.sh_size / sh.sh_entsize;
    for (uint64_t k = 1; k < count; ++k) {
      typename C::Sym sym;
      std::memcpy(&sym, v.p + sh.sh_offset + k * sh.sh_entsize, sizeof sym);
      const unsigned bind = C::bind(sym.st_info);
      const unsigned type = C::type(sym.st_info);
      if (bind != STB_GLOBAL && bind != STB_WEAK && bind != STB_GNU_UNIQUE) continue;
      if (type == STT_SECTION || type == STT_FILE) continue;
      if (sym.st_name >= strsh.sh_size) return bad(path, "symbol name out of range");
      const std::string name = cstr_at(v, static_cast<uint64_t>(strsh.sh_offset) + sym.st_name, static_cast<uint64_t>(strsh.sh_offset) + strsh.sh_size);
      if (name.empty()) continue;
      if (sym.st_shndx == SHN_UNDEF) {
        if (bind != STB_WEAK) out->undefined.insert(name);
        continue;
      }
      const unsigned vis = C::visibility(sym.st_other);
      if (vis == STV_HIDDEN || vis == STV_INTERNAL) continue;
      out->defined.insert(name);
    }
  }
  if (!found) return bad(path, "no dynamic symbol table");
  return kz::Status::success();
}

template <typename C>
kz::Status elf_notes(const std::string& path, const kz::Bytes& bytes, ElfNotes* out) {
  typename C::Ehdr eh;
  std::memcpy(&eh, bytes.data(), sizeof eh);
  out->type = eh.e_type == ET_REL ? "relocatable" : eh.e_type == ET_EXEC ? "executable" : eh.e_type == ET_DYN ? "ET_DYN (a position-independent executable or a shared object)" : "other";
  const View v{bytes.data(), bytes.size()};
  typename C::Shdr shstr;
  if (!section_at<C>(v, eh, eh.e_shstrndx, &shstr) || !v.in(shstr.sh_offset, shstr.sh_size)) return bad(path, "section name table out of range");
  for (uint32_t i = 0; i < eh.e_shnum; ++i) {
    typename C::Shdr sh;
    if (!section_at<C>(v, eh, i, &sh)) return bad(path, "section header out of range");
    if (sh.sh_name >= shstr.sh_size) continue;
    const std::string name = cstr_at(v, static_cast<uint64_t>(shstr.sh_offset) + sh.sh_name, static_cast<uint64_t>(shstr.sh_offset) + shstr.sh_size);
    if (sh.sh_type == SHT_NOBITS || !v.in(sh.sh_offset, sh.sh_size)) continue;
    if (name == ".comment") {
      uint64_t off = sh.sh_offset;
      const uint64_t end = static_cast<uint64_t>(sh.sh_offset) + sh.sh_size;
      while (off < end) {
        std::string s = cstr_at(v, off, end);
        off += s.size() + 1;
        if (s.empty()) continue;
        bool dup = false;
        for (const std::string& c : out->comments) dup |= (c == s);
        if (!dup) out->comments.push_back(s);
      }
    } else if (name == ".note.gnu.build-id") {
      // Nhdr {namesz, descsz, type} (three 4-byte words in both classes), name padded to 4, then the descriptor
      uint64_t off = sh.sh_offset;
      const uint64_t end = static_cast<uint64_t>(sh.sh_offset) + sh.sh_size;
      while (off + sizeof(typename C::Nhdr) <= end) {
        typename C::Nhdr nh;
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

}  // namespace

uint32_t ElfId::pointer_bytes() const { return elf_class == ELFCLASS64 ? 8u : elf_class == ELFCLASS32 ? 4u : 0u; }

std::string ElfId::text() const {
  if (elf_class == 0) return "not read";
  std::string m;
  switch (machine) {
    case EM_386: m = "EM_386"; break;
    case EM_X86_64: m = "EM_X86_64"; break;
    default: m = "EM"; break;
  }
  return std::string(elf_class == ELFCLASS64 ? "ELF64" : "ELF32") + (data == ELFDATA2LSB ? " little-endian " : " big-endian ") + m + " (" + std::to_string(machine) + ")";
}

kz::Status ElfTarget::add(const std::string& path, const ElfId& id) {
  if (!known()) {
    id_ = id;
    first_ = path;
    return kz::Status::success();
  }
  if (id == id_) return kz::Status::success();
  return kz::Status::error(kz::Err::Invalid, path + " is " + id.text() + ", but " + first_ + " (the first file read for this program) is " + id_.text() +
                                                 ": one program's files must have one ELF class, byte order and machine");
}

kz::Status read_object_symbols(const std::string& path, ObjectSymbols* out) {
  kz::Bytes bytes;
  kz::Status st = load(path, &bytes, &out->elf);
  if (!st.ok()) return st;
  return out->elf.elf_class == ELFCLASS64 ? object_symbols<Elf64Class>(path, bytes, out) : object_symbols<Elf32Class>(path, bytes, out);
}

kz::Status read_dynamic_symbols(const std::string& path, ObjectSymbols* out) {
  kz::Bytes bytes;
  kz::Status st = load(path, &bytes, &out->elf);
  if (!st.ok()) return st;
  return out->elf.elf_class == ELFCLASS64 ? dynamic_symbols<Elf64Class>(path, bytes, out) : dynamic_symbols<Elf32Class>(path, bytes, out);
}

kz::Status read_elf_notes(const std::string& path, ElfNotes* out) {
  kz::Bytes bytes;
  kz::Status st = load(path, &bytes, &out->elf);
  if (!st.ok()) return st;
  return out->elf.elf_class == ELFCLASS64 ? elf_notes<Elf64Class>(path, bytes, out) : elf_notes<Elf32Class>(path, bytes, out);
}

}  // namespace kzs

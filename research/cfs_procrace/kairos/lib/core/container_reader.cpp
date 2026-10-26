// container_reader.cpp -- strict reader: accepts only the canonical layout.

#include <cstdio>
#include <cstring>
#include <map>

#include "kz/core/container.hpp"
#include "kz/core/crc32.hpp"
#include "kz/core/record_codec.hpp"
#include "string_pool.hpp"

namespace kz {

namespace {

constexpr uint64_t align8(uint64_t x) { return (x + 7u) & ~static_cast<uint64_t>(7u); }
constexpr uint32_t kMaxTableEntries = 1u << 20;

bool all_zero(const uint8_t* p, size_t n) {
  for (size_t i = 0; i < n; ++i)
    if (p[i] != 0) return false;
  return true;
}

std::string at(const char* what, uint64_t value) { return std::string(what) + " " + std::to_string(value); }

}  // namespace

Status read_whole_file(const std::string& path, Bytes* out) {
  FILE* f = std::fopen(path.c_str(), "rb");
  if (f == nullptr) return Status::error(Err::Io, "cannot open " + path);
  Bytes data;
  uint8_t chunk[65536];
  for (;;) {
    size_t n = std::fread(chunk, 1, sizeof(chunk), f);
    data.insert(data.end(), chunk, chunk + n);
    if (n < sizeof(chunk)) break;
  }
  bool bad = std::ferror(f) != 0;
  std::fclose(f);
  if (bad) return Status::error(Err::Io, "read error on " + path);
  *out = std::move(data);
  return Status::success();
}

Status Container::read_file(const std::string& path, Container* out, const ParseOptions& options) {
  Bytes data;
  Status s = read_whole_file(path, &data);
  if (!s.ok()) return s;
  return parse(std::move(data), out, options);
}

Status Container::parse(Bytes bytes, Container* out, const ParseOptions& options) {
  Container c;
  c.bytes_ = std::move(bytes);
  const Bytes& b = c.bytes_;
  if (b.size() < sizeof(kz_header)) return Status::error(Err::Truncated, "file shorter than the header");
  std::memcpy(&c.header_, b.data(), sizeof(kz_header));
  const kz_header& h = c.header_;
  if (std::memcmp(h.magic, kMagic, 8) != 0) return Status::error(Err::BadMagic, "not a KZ container");
  if (h.abi_version != KZ_ABI_VERSION)
    return Status::error(Err::BadAbi, at("ABI version", h.abi_version) + " is not " + std::to_string(KZ_ABI_VERSION));
  if (h.header_size != sizeof(kz_header)) return Status::error(Err::BadAbi, at("header size", h.header_size));
  if (h.total_size > b.size()) return Status::error(Err::Truncated, "file is shorter than total_size");
  if (h.total_size < b.size()) return Status::error(Err::BadHeader, "bytes after total_size");
  if (h.flags != 0 || h.rsv0 != 0) return Status::error(Err::BadReserved, "reserved header field is not zero");
  if (h.n_parents > kMaxTableEntries || h.n_sections > kMaxTableEntries)
    return Status::error(Err::BadHeader, "table size is implausible");
  if (h.parents_off != sizeof(kz_header)) return Status::error(Err::NotCanonical, "parent table is not at its canonical offset");
  const uint64_t sections_off = align8(h.parents_off + static_cast<uint64_t>(h.n_parents) * sizeof(kz_parent));
  if (h.sections_off != sections_off) return Status::error(Err::NotCanonical, "section table is not at its canonical offset");
  const uint64_t tables_end = sections_off + static_cast<uint64_t>(h.n_sections) * sizeof(kz_section_entry);
  if (tables_end > b.size()) return Status::error(Err::Truncated, "tables extend past the end of the file");

  {  // header CRC: header + tables with header_crc taken as zero
    Bytes copy(b.begin(), b.begin() + static_cast<std::ptrdiff_t>(tables_end));
    kz_header z = h;
    z.header_crc = 0;
    std::memcpy(copy.data(), &z, sizeof(z));
    if (crc32(copy.data(), copy.size()) != h.header_crc) return Status::error(Err::BadCrc, "header/table CRC mismatch");
  }
  if (!all_zero(b.data() + h.parents_off + static_cast<uint64_t>(h.n_parents) * sizeof(kz_parent),
                static_cast<size_t>(sections_off - (h.parents_off + static_cast<uint64_t>(h.n_parents) * sizeof(kz_parent)))))
    return Status::error(Err::BadPadding, "padding after the parent table is not zero");

  c.parents_.resize(h.n_parents);
  for (uint32_t i = 0; i < h.n_parents; ++i) {
    std::memcpy(&c.parents_[i], b.data() + h.parents_off + static_cast<uint64_t>(i) * sizeof(kz_parent), sizeof(kz_parent));
    if (i > 0) {
      const kz_parent& p = c.parents_[i - 1];
      const kz_parent& q = c.parents_[i];
      bool less = (p.kind != q.kind) ? p.kind < q.kind
                  : (p.role != q.role) ? p.role < q.role
                                       : std::memcmp(p.sha256, q.sha256, 32) < 0;
      if (!less) return Status::error(Err::BadParent, "parent table is not strictly sorted");
    }
  }

  c.table_.resize(h.n_sections);
  uint64_t cursor = tables_end;
  for (uint32_t i = 0; i < h.n_sections; ++i) {
    kz_section_entry& e = c.table_[i];
    std::memcpy(&e, b.data() + sections_off + static_cast<uint64_t>(i) * sizeof(kz_section_entry), sizeof(e));
    const std::string name = "section " + std::to_string(e.type);
    if (e.rsv0 != 0 || e.rsv1 != 0) return Status::error(Err::BadReserved, name + ": reserved field is not zero");
    if (i > 0 && c.table_[i - 1].type >= e.type) return Status::error(Err::BadSection, name + ": not in strictly ascending order");
    const SectionInfo* info = kz::find_section(e.type);
    if (info == nullptr || info->record == nullptr) {
      if (!options.allow_unknown_sections) return Status::error(Err::BadSection, name + ": type is not registered");
      if (e.record_size == 0) return Status::error(Err::BadRecord, name + ": zero record size");
    } else {
      if (e.record_size != info->record->size)
        return Status::error(Err::BadRecord, std::string("section ") + info->name + ": record size " +
                                                 std::to_string(e.record_size) + " != " + std::to_string(info->record->size));
      if (e.schema_version != info->schema_version)
        return Status::error(Err::BadRecord, std::string("section ") + info->name + ": schema version " +
                                                 std::to_string(e.schema_version) + " != " + std::to_string(info->schema_version));
    }
    if (e.record_size != 0 && e.count > UINT64_MAX / e.record_size) return Status::error(Err::BadSection, name + ": size overflows");
    if (e.size != e.count * e.record_size) return Status::error(Err::BadSection, name + ": size != count * record_size");
    if (e.offset != align8(cursor)) return Status::error(Err::NotCanonical, name + ": not at its canonical offset");
    if (e.offset > b.size() || e.size > b.size() - e.offset) return Status::error(Err::Truncated, name + ": extends past the end of the file");
    if (!all_zero(b.data() + cursor, static_cast<size_t>(e.offset - cursor))) return Status::error(Err::BadPadding, name + ": padding before the section is not zero");
    if (crc32(b.data() + e.offset, static_cast<size_t>(e.size)) != e.crc32) return Status::error(Err::BadCrc, name + ": payload CRC mismatch");
    cursor = e.offset + e.size;
  }
  if (align8(cursor) != h.total_size) return Status::error(Err::NotCanonical, "total_size is not the end of the last section, aligned");
  if (!all_zero(b.data() + cursor, static_cast<size_t>(h.total_size - cursor))) return Status::error(Err::BadPadding, "trailing padding is not zero");

  const kz_section_entry* strings = nullptr;
  for (const kz_section_entry& e : c.table_)
    if (e.type == KZ_SEC_STRINGS) strings = &e;
  if (strings == nullptr || strings->count == 0 || b[strings->offset] != 0)
    return Status::error(Err::BadSection, "STRINGS section missing or does not start with the empty string");

  c.hash_ = sha256(b.data(), b.size());
  *out = std::move(c);
  return Status::success();
}

const kz_section_entry* Container::find_section(uint32_t type) const {
  for (const kz_section_entry& e : table_)
    if (e.type == type) return &e;
  return nullptr;
}

const uint8_t* Container::section_data(uint32_t type, size_t* size) const {
  const kz_section_entry* e = find_section(type);
  if (e == nullptr) {
    *size = 0;
    return nullptr;
  }
  *size = static_cast<size_t>(e->size);
  return bytes_.data() + e->offset;
}

void Container::copy_section(const kz_section_entry& e, const kz_record_desc& rec, void* dst) const {
  if (section_is_serialized(e.type)) decode_records(rec, bytes_.data() + e.offset, static_cast<size_t>(e.count), dst);
  else std::memcpy(dst, bytes_.data() + e.offset, static_cast<size_t>(e.size));
}

bool Container::string(const kz_str& s, std::string* out) const {
  size_t n = 0;
  const uint8_t* pool = section_data(KZ_SEC_STRINGS, &n);
  return resolve_string(pool, n, s, out);
}

Status Container::verify_reserved() const {
  for (const kz_section_entry& e : table_) {
    const SectionInfo* info = kz::find_section(e.type);
    if (info == nullptr || info->record == nullptr) continue;
    const kz_record_desc& rec = *info->record;
    for (uint64_t i = 0; i < e.count; ++i) {
      const uint8_t* r = bytes_.data() + e.offset + i * rec.size;
      for (uint32_t f = 0; f < rec.nfields; ++f) {
        const kz_field_desc& fd = rec.fields[f];
        if (!is_reserved_field(fd)) continue;
        if (!all_zero(r + fd.offset, static_cast<size_t>(fd.elem_size) * fd.count))
          return Status::error(Err::BadReserved, std::string(info->name) + "[" + std::to_string(i) + "]." + fd.name + " is not zero");
      }
    }
  }
  return Status::success();
}

Status Container::verify_refs() const {
  size_t pool_size = 0;
  section_data(KZ_SEC_STRINGS, &pool_size);
  auto check_ref = [&](const uint8_t* p, const std::string& where) -> Status {
    kz_ref ref;
    std::memcpy(&ref, p, sizeof(ref));
    if (all_zero(p, sizeof(ref))) return Status::success();  // null reference
    if (all_zero(ref.artifact_sha256, 32)) {                  // a record of this container
      const kz_section_entry* t = find_section(ref.section);
      if (t == nullptr) return Status::error(Err::BadRef, where + ": reference to a section this container does not have");
      if (ref.index >= t->count) return Status::error(Err::BadRef, where + ": reference index out of range");
      return Status::success();
    }
    for (const kz_parent& par : parents_)
      if (std::memcmp(par.sha256, ref.artifact_sha256, 32) == 0) return Status::success();
    return Status::error(Err::BadRef, where + ": reference names an artifact that is not a parent");
  };
  for (const kz_section_entry& e : table_) {
    const SectionInfo* info = kz::find_section(e.type);
    if (info == nullptr || info->record == nullptr) continue;
    const kz_record_desc& rec = *info->record;
    const bool is_ref_list = std::strcmp(rec.name, "kz_ref") == 0;
    for (uint64_t i = 0; i < e.count; ++i) {
      const uint8_t* r = bytes_.data() + e.offset + i * rec.size;
      const std::string base = std::string(info->name) + "[" + std::to_string(i) + "]";
      if (is_ref_list) {
        Status s = check_ref(r, base);
        if (!s.ok()) return s;
        continue;
      }
      for (uint32_t f = 0; f < rec.nfields; ++f) {
        const kz_field_desc& fd = rec.fields[f];
        if (fd.kind == KZ_FK_REF) {
          Status s = check_ref(r + fd.offset, base + "." + fd.name);
          if (!s.ok()) return s;
        } else if (fd.kind == KZ_FK_STR) {
          kz_str s;
          std::memcpy(&s, r + fd.offset, sizeof(s));
          std::string tmp;
          if (!string(s, &tmp)) return Status::error(Err::BadRef, base + "." + fd.name + ": string reference outside the STRINGS section");
        }
      }
    }
  }
  return Status::success();
}

Status Container::verify_strings() const {
  size_t pool_size = 0;
  const uint8_t* pool = section_data(KZ_SEC_STRINGS, &pool_size);
  bool opaque = false;  // a section whose strings this build cannot see
  struct Use {
    const SectionInfo* section;
    uint64_t record;
    const kz_field_desc* field;
    uint32_t off;
    const std::string* text;  // key of `canonical` (map nodes do not move)
  };
  std::vector<Use> uses;
  std::map<std::string, uint32_t> canonical;  // referenced string -> canonical offset (filled below)
  auto where = [](const Use& u) { return std::string(u.section->name) + "[" + std::to_string(u.record) + "]." + u.field->name; };
  for (const kz_section_entry& e : table_) {
    const SectionInfo* info = kz::find_section(e.type);
    if (info == nullptr || info->record == nullptr) {
      opaque = true;
      continue;
    }
    if (e.type == KZ_SEC_STRINGS) continue;
    const kz_record_desc& rec = *info->record;
    for (uint32_t f = 0; f < rec.nfields; ++f) {
      const kz_field_desc& fd = rec.fields[f];
      if (fd.kind != KZ_FK_STR) continue;
      for (uint64_t i = 0; i < e.count; ++i)
        for (uint32_t k = 0; k < fd.count; ++k) {
          kz_str s;
          std::memcpy(&s, bytes_.data() + e.offset + i * rec.size + fd.offset + static_cast<size_t>(k) * fd.elem_size, sizeof(s));
          std::string text;
          if (!resolve_string(pool, pool_size, s, &text))
            return Status::error(Err::BadRef, where(Use{info, i, &fd, s.off, nullptr}) + ": string reference outside the STRINGS section");
          if (text.empty()) continue;  // (0, 0)
          auto it = canonical.emplace(std::move(text), 0u).first;
          uses.push_back(Use{info, i, &fd, s.off, &it->first});
        }
    }
  }
  if (opaque) return Status::success();  // the pool cannot be compared as a whole (see container.hpp)
  const Bytes want = canonical_pool(&canonical);
  if (want.size() != pool_size || std::memcmp(want.data(), pool, pool_size) != 0)
    return Status::error(Err::NotCanonical,
                         "the STRINGS section is not the canonical pool of the strings that the records reference (sorted, no duplicate, no unreferenced string)");
  for (const Use& u : uses)
    if (canonical.find(*u.text)->second != u.off)
      return Status::error(Err::NotCanonical, where(u) + ": string reference is not at the canonical offset of its string");
  return Status::success();
}

Status Container::verify_content() const {
  Status s = verify_reserved();
  if (!s.ok()) return s;
  s = verify_refs();
  if (!s.ok()) return s;
  return verify_strings();
}

Status Container::verify_parent(const Container& parent) const {
  for (const kz_parent& p : parents_) {
    if (std::memcmp(p.sha256, parent.artifact_hash().data(), 32) != 0) continue;
    if (p.kind != parent.header().artifact_kind) return Status::error(Err::BadParent, "parent is listed with a different artifact kind");
    return Status::success();
  }
  return Status::error(Err::BadParent, "artifact " + to_hex(parent.artifact_hash()) + " is not a parent of this container");
}

}  // namespace kz

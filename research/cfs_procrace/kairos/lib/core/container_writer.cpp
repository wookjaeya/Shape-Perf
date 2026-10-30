// container_writer.cpp -- the deterministic, sorted-section KZ writer.

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <map>

#include "kz/core/container.hpp"
#include "kz/core/crc32.hpp"
#include "kz/core/dump.hpp"
#include "kz/core/record_codec.hpp"
#include "string_pool.hpp"

namespace kz {

namespace {

constexpr uint64_t align8(uint64_t x) { return (x + 7u) & ~static_cast<uint64_t>(7u); }

template <class T>
void put(Bytes& out, uint64_t offset, const T& record) {
  std::memcpy(out.data() + offset, &record, sizeof(T));
}

bool parent_less(const kz_parent& a, const kz_parent& b) {
  if (a.kind != b.kind) return a.kind < b.kind;
  if (a.role != b.role) return a.role < b.role;
  return std::memcmp(a.sha256, b.sha256, 32) < 0;
}

}  // namespace

StringPool::StringPool() {
  bytes_.push_back(0);  // offset 0 is the empty string
}

kz_str StringPool::intern(std::string_view s) {
  kz_str ref;
  std::memset(&ref, 0, sizeof(ref));
  if (s.empty()) return ref;
  auto found = index_.find(std::string(s));
  if (found != index_.end()) return found->second;
  ref.off = static_cast<uint32_t>(bytes_.size());
  ref.len = static_cast<uint32_t>(s.size());
  bytes_.insert(bytes_.end(), s.begin(), s.end());
  bytes_.push_back(0);  // NUL terminator: a reference always ends at a zero byte
  index_.emplace(std::string(s), ref);
  return ref;
}

ContainerWriter::ContainerWriter(uint32_t artifact_kind) : artifact_kind_(artifact_kind) {}

Status ContainerWriter::add_parent(uint32_t parent_kind, uint32_t role, const Digest& artifact_hash) {
  kz_parent p;
  std::memset(&p, 0, sizeof(p));
  std::memcpy(p.sha256, artifact_hash.data(), 32);
  p.kind = parent_kind;
  p.role = role;
  for (const kz_parent& q : parents_)
    if (std::memcmp(q.sha256, p.sha256, 32) == 0)
      return Status::error(Err::BadParent, "parent " + to_hex(artifact_hash) + " added twice");
  parents_.push_back(p);
  return Status::success();
}

Status ContainerWriter::add_section_raw(uint32_t section_type, const void* records, size_t count, size_t record_size) {
  const SectionInfo* info = find_section(section_type);
  if (info == nullptr || info->record == nullptr)
    return Status::error(Err::BadSection, "section type " + std::to_string(section_type) + " is not registered");
  if (section_type == KZ_SEC_STRINGS)
    return Status::error(Err::Invalid, "STRINGS is written from the string pool; use intern()");
  if (record_size != info->record->size)
    return Status::error(Err::BadRecord, std::string("section ") + info->name + ": record size " +
                                             std::to_string(record_size) + " != " + std::to_string(info->record->size));
  for (const Section& s : sections_)
    if (s.type == section_type) return Status::error(Err::Duplicate, std::string("section ") + info->name + " added twice");
  if (check_reserved_ && count > 0) {
    const kz_record_desc& rec = *info->record;
    const uint8_t* base = static_cast<const uint8_t*>(records);
    for (size_t i = 0; i < count; ++i)
      for (uint32_t f = 0; f < rec.nfields; ++f) {
        const kz_field_desc& fd = rec.fields[f];
        if (!is_reserved_field(fd)) continue;
        for (size_t k = 0; k < static_cast<size_t>(fd.elem_size) * fd.count; ++k)
          if (base[i * rec.size + fd.offset + k] != 0)
            return Status::error(Err::BadReserved, std::string("section ") + info->name + "[" + std::to_string(i) + "]." + fd.name + " is not zero");
      }
  }
  Section s;
  s.type = section_type;
  s.count = count;
  s.record_size = record_size;
  s.data.resize(count * record_size);
  // The neutral layer's sections are stored by the explicit little-endian codec; the earlier sections
  // keep the byte copy of their (little-endian, padding-free) structs, as before (record_codec.hpp).
  if (count > 0 && section_is_serialized(section_type)) encode_records(*info->record, records, count, s.data.data());
  else if (count > 0) std::memcpy(s.data.data(), records, count * record_size);
  sections_.push_back(std::move(s));
  return Status::success();
}

Status ContainerWriter::finish(Bytes* out) const {
  // Work on copies of the payloads: the kz_str offsets in them are provisional (positions in
  // the intern-order table) and are rewritten to the canonical pool below.
  std::vector<Section> work = sections_;
  const Bytes& provisional = pool_.bytes();
  struct Site {
    size_t section;           // index into `work`
    size_t byte;              // offset of the kz_str inside that payload
    const std::string* text;  // key of `canonical` (map nodes do not move)
  };
  std::vector<Site> sites;
  std::map<std::string, uint32_t> canonical;  // referenced string -> offset in the canonical pool
  for (size_t si = 0; si < work.size(); ++si) {
    const SectionInfo* info = find_section(work[si].type);
    const kz_record_desc& rec = *info->record;
    for (size_t i = 0; i < work[si].count; ++i)
      for (uint32_t f = 0; f < rec.nfields; ++f) {
        const kz_field_desc& fd = rec.fields[f];
        if (fd.kind != KZ_FK_STR) continue;
        for (uint32_t k = 0; k < fd.count; ++k) {
          const size_t at = i * rec.size + fd.offset + static_cast<size_t>(k) * fd.elem_size;
          kz_str ref;  // stored form: offset, then length, little-endian
          ref.off = load_le32(work[si].data.data() + at);
          ref.len = load_le32(work[si].data.data() + at + 4);
          std::string text;
          if (!resolve_string(provisional.data(), provisional.size(), ref, &text)) {
            if (check_reserved_)
              return Status::error(Err::BadRef, std::string("section ") + info->name + "[" + std::to_string(i) + "]." + fd.name +
                                                    ": string reference does not resolve in the string pool (use intern())");
            continue;  // reader tests build damaged containers on purpose
          }
          if (text.empty()) continue;  // (0, 0) stays
          auto it = canonical.emplace(std::move(text), 0u).first;
          sites.push_back(Site{si, at, &it->first});
        }
      }
  }
  Section strings;
  strings.type = KZ_SEC_STRINGS;
  strings.data = canonical_pool(&canonical);  // assigns the offsets
  if (strings.data.size() >= 0xFFFFFFFFu) return Status::error(Err::Invalid, "the string pool does not fit in 32-bit offsets");
  strings.count = strings.data.size();
  strings.record_size = 1;
  for (const Site& site : sites) {
    store_le32(work[site.section].data.data() + site.byte, canonical.find(*site.text)->second);
    store_le32(work[site.section].data.data() + site.byte + 4, static_cast<uint32_t>(site.text->size()));
  }
  // The string pool is a section like the others, written first (lowest id).
  std::vector<const Section*> order;
  order.push_back(&strings);
  for (const Section& s : work) order.push_back(&s);
  std::sort(order.begin(), order.end(), [](const Section* a, const Section* b) { return a->type < b->type; });

  std::vector<kz_parent> parents = parents_;
  std::sort(parents.begin(), parents.end(), parent_less);

  const uint64_t n_parents = parents.size();
  const uint64_t n_sections = order.size();
  const uint64_t parents_off = sizeof(kz_header);
  const uint64_t sections_off = align8(parents_off + n_parents * sizeof(kz_parent));
  const uint64_t tables_end = sections_off + n_sections * sizeof(kz_section_entry);

  std::vector<kz_section_entry> table(order.size());
  uint64_t cursor = tables_end;
  for (size_t i = 0; i < order.size(); ++i) {
    kz_section_entry& e = table[i];
    std::memset(&e, 0, sizeof(e));
    const SectionInfo* info = find_section(order[i]->type);
    e.type = order[i]->type;
    e.schema_version = info->schema_version;
    e.record_size = static_cast<uint32_t>(order[i]->record_size);
    e.count = order[i]->count;
    e.offset = align8(cursor);
    e.size = order[i]->data.size();
    e.crc32 = crc32(order[i]->data.data(), order[i]->data.size());
    cursor = e.offset + e.size;
  }
  const uint64_t total = align8(cursor);

  Bytes buf(static_cast<size_t>(total), 0);  // zero fill: all padding is zero
  kz_header h;
  std::memset(&h, 0, sizeof(h));
  std::memcpy(h.magic, kMagic, 8);
  h.abi_version = KZ_ABI_VERSION;
  h.artifact_kind = artifact_kind_;
  h.header_size = sizeof(kz_header);
  h.n_parents = static_cast<uint32_t>(n_parents);
  h.n_sections = static_cast<uint32_t>(n_sections);
  h.parents_off = parents_off;
  h.sections_off = sections_off;
  h.total_size = total;
  std::memcpy(h.basis_sha256, basis_.data(), 32);
  std::memcpy(h.profile_sha256, profile_.data(), 32);
  std::memcpy(h.tool_sha256, tool_.data(), 32);
  put(buf, 0, h);
  for (size_t i = 0; i < parents.size(); ++i) put(buf, parents_off + i * sizeof(kz_parent), parents[i]);
  for (size_t i = 0; i < table.size(); ++i) put(buf, sections_off + i * sizeof(kz_section_entry), table[i]);
  for (size_t i = 0; i < order.size(); ++i)
    if (!order[i]->data.empty()) std::memcpy(buf.data() + table[i].offset, order[i]->data.data(), order[i]->data.size());
  // The header CRC covers header, parent table and section table, with the field itself zero.
  h.header_crc = crc32(buf.data(), static_cast<size_t>(tables_end));
  put(buf, 0, h);
  *out = std::move(buf);
  return Status::success();
}

Status ContainerWriter::write_file(const std::string& path) const {
  Bytes bytes;
  Status s = finish(&bytes);
  if (!s.ok()) return s;
  return write_whole_file(path, bytes);
}

Status write_whole_file(const std::string& path, const Bytes& data) {
  const std::string tmp = path + ".tmp";
  FILE* f = std::fopen(tmp.c_str(), "wb");
  if (f == nullptr) return Status::error(Err::Io, "cannot open " + tmp + " for writing");
  size_t n = data.empty() ? 0 : std::fwrite(data.data(), 1, data.size(), f);
  bool ok = (n == data.size());
  ok = (std::fclose(f) == 0) && ok;
  if (!ok) {
    std::remove(tmp.c_str());
    return Status::error(Err::Io, "short write to " + tmp);
  }
  if (std::rename(tmp.c_str(), path.c_str()) != 0) {
    std::remove(tmp.c_str());
    return Status::error(Err::Io, "cannot rename " + tmp + " to " + path);
  }
  return Status::success();
}

}  // namespace kz

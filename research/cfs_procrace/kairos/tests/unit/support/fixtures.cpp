#include "fixtures.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "kz/core/registry.hpp"
#include "kz/core/sha256.hpp"

namespace kzfix {

kz::Digest fixture_tool_hash() { return kz::sha256("kz-unit-test fixture tool"); }

void must(const kz::Status& s, const char* what) {
  if (!s.ok()) {
    std::fprintf(stderr, "fixture error: %s: %s\n", what, s.describe().c_str());
    std::abort();
  }
}

namespace {

uint64_t mask_for(uint32_t size) { return size >= 8 ? ~static_cast<uint64_t>(0) : ((static_cast<uint64_t>(1) << (8 * size)) - 1); }

uint64_t element_value(const kz_field_desc& f, const kz::ResolvedFormat& rf, Rng* rng) {
  const uint64_t m = mask_for(f.elem_size);
  switch (rf.fmt) {
    case kz::Fmt::Enum:
      if (rf.en != nullptr && rf.en->nitems > 0) return rf.en->items[rng->next() % rf.en->nitems].value & m;
      break;
    case kz::Fmt::Flags: {
      uint64_t v = 0;
      if (rf.en != nullptr)
        for (size_t i = 0; i < rf.en->nitems; ++i)
          if (rng->next() % 2 == 0) v |= rf.en->items[i].value;
      return v & m;
    }
    case kz::Fmt::Routes:
      return rng->next() & KZ_ROUTES_VALID_MASK;
    case kz::Fmt::Idx:
      if (rng->next() % 5 == 0) return m;
      return rng->next() % 1000;
    default:
      break;
  }
  uint64_t v = (static_cast<uint64_t>(rng->next()) << 32) | rng->next();
  if (f.elem_size < 8) v &= m;
  return v;
}

// A reference that is null or points into `foreign`.
void fill_ref(uint8_t* dst, Rng* rng, const kz::Digest& foreign) {
  uint32_t r = rng->next();
  if (r % 3 == 0) return;  // null reference: all zero
  kz_ref ref = zero<kz_ref>();
  std::memcpy(ref.artifact_sha256, foreign.data(), 32);
  ref.section = kz::sections()[(r >> 4) % kz::sections().size()].type;
  ref.index = (r >> 12) % 100;
  std::memcpy(dst, &ref, sizeof(ref));
}

kz_str intern_for(kz::ContainerWriter* w, const kz_record_desc& rec, const kz_field_desc& f, uint32_t seed, Rng* rng) {
  if (w == nullptr || rng->next() % 4 == 0) return zero<kz_str>();
  return w->intern(std::string(rec.name) + "." + f.name + "#" + std::to_string(seed));
}

}  // namespace

kz::Bytes make_record(const kz_record_desc& rec, uint32_t seed, kz::ContainerWriter* w, const kz::Digest& foreign) {
  kz::Bytes out(rec.size, 0);
  Rng rng(static_cast<uint64_t>(seed) + 1);
  if (std::strcmp(rec.name, "kz_ref") == 0) {  // the record is itself a reference (REF_LISTS)
    fill_ref(out.data(), &rng, foreign);
    return out;
  }
  for (uint32_t i = 0; i < rec.nfields; ++i) {
    const kz_field_desc& f = rec.fields[i];
    if (kz::is_reserved_field(f)) continue;
    uint8_t* p = out.data() + f.offset;
    if (f.kind == KZ_FK_REF) {
      fill_ref(p, &rng, foreign);
      continue;
    }
    if (f.kind == KZ_FK_STR) {
      kz_str s = intern_for(w, rec, f, seed, &rng);
      std::memcpy(p, &s, sizeof(s));
      continue;
    }
    const kz::ResolvedFormat rf = kz::format_of(rec, f);
    for (uint32_t k = 0; k < f.count; ++k) {
      uint64_t v = element_value(f, rf, &rng);
      std::memcpy(p + static_cast<size_t>(k) * f.elem_size, &v, f.elem_size);  // little-endian host
    }
  }
  return out;
}

kz::Bytes make_all_sections_container(size_t n, const kz::Digest& foreign, kz::Digest* tool, std::vector<SectionBytes>* given) {
  kz::ContainerWriter w(KZ_ARTIFACT_TEST);
  *tool = fixture_tool_hash();
  w.set_tool_hash(*tool);
  must(w.add_parent(KZ_ARTIFACT_FACTS, 0, foreign), "add_parent");
  for (const kz::SectionInfo& s : kz::sections()) {
    if (s.type == KZ_SEC_STRINGS || s.record == nullptr) continue;
    kz::Bytes data;
    for (size_t i = 0; i < n; ++i) {
      kz::Bytes r = make_record(*s.record, static_cast<uint32_t>(s.type * 100 + i), &w, foreign);
      data.insert(data.end(), r.begin(), r.end());
    }
    must(w.add_section_raw(s.type, data.data(), n, s.record->size), s.name);
    if (given != nullptr) given->push_back(SectionBytes{s.type, data});
  }
  kz::Bytes out;
  must(w.finish(&out), "finish");
  return out;
}

}  // namespace kzfix

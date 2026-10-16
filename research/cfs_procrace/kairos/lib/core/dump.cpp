// dump.cpp -- canonical text dump.

#include "kz/core/dump.hpp"

#include <cstdio>
#include <cstring>

namespace kz {

namespace {

const char* kind_name(uint32_t kind) {
  switch (kind) {
    case KZ_FK_U8: return "u8";
    case KZ_FK_U16: return "u16";
    case KZ_FK_U32: return "u32";
    case KZ_FK_U64: return "u64";
    case KZ_FK_I32: return "i32";
    case KZ_FK_I64: return "i64";
    case KZ_FK_REF: return "ref";
    case KZ_FK_STR: return "str";
  }
  return "?";
}

uint64_t load_unsigned(const uint8_t* p, uint32_t size) {
  uint64_t v = 0;
  std::memcpy(&v, p, size);  // little-endian host (checked by kz_schema.h)
  return v;
}

int64_t load_signed(const uint8_t* p, uint32_t size) {
  uint64_t u = load_unsigned(p, size);
  if (size == 4) return static_cast<int32_t>(static_cast<uint32_t>(u));
  return static_cast<int64_t>(u);
}

std::string hex_value(uint64_t v) {
  char buf[24];
  std::snprintf(buf, sizeof(buf), "0x%llx", static_cast<unsigned long long>(v));
  return buf;
}

std::string decimal(uint64_t v) { return std::to_string(v); }

std::string flags_text(const EnumDesc& en, uint64_t v) {
  std::string s = "{";
  uint64_t rest = v;
  bool first = true;
  for (size_t i = 0; i < en.nitems; ++i) {
    if ((v & en.items[i].value) != 0 && en.items[i].value != 0) {
      if (!first) s += ",";
      s += en.items[i].display;
      first = false;
      rest &= ~en.items[i].value;
    }
  }
  if (rest != 0) {
    if (!first) s += ",";
    s += hex_value(rest);
  }
  return s + "}";
}

std::string routes_text(uint64_t v) {
  const EnumDesc* en = find_enum("KZ_ROUTE");
  auto name = [&](uint64_t r) -> std::string {
    const char* d = en != nullptr ? enum_display(*en, r) : nullptr;
    return d != nullptr ? d : "#" + decimal(r);
  };
  std::string s = "not_bad=" + name((v >> KZ_ROUTE_NOT_BAD_SHIFT) & KZ_ROUTE_MASK);
  s += ",not_ok=" + name((v >> KZ_ROUTE_NOT_OK_SHIFT) & KZ_ROUTE_MASK);
  s += ",not_nr=" + name((v >> KZ_ROUTE_NOT_NR_SHIFT) & KZ_ROUTE_MASK);
  if ((v & ~static_cast<uint64_t>(KZ_ROUTES_VALID_MASK)) != 0) s += ",unused=" + hex_value(v & ~static_cast<uint64_t>(KZ_ROUTES_VALID_MASK));
  return s;
}

std::string scalar_text(const kz_field_desc& f, const ResolvedFormat& rf, const uint8_t* p) {
  if (f.kind == KZ_FK_I32 || f.kind == KZ_FK_I64) return std::to_string(load_signed(p, f.elem_size));
  uint64_t v = load_unsigned(p, f.elem_size);
  switch (rf.fmt) {
    case Fmt::Enum: {
      const char* d = rf.en != nullptr ? enum_display(*rf.en, v) : nullptr;
      return d != nullptr ? std::string(d) : "#" + decimal(v);
    }
    case Fmt::Flags:
      return rf.en != nullptr ? flags_text(*rf.en, v) : hex_value(v);
    case Fmt::Hex:
      return hex_value(v);
    case Fmt::Idx: {
      const uint64_t none = (f.elem_size == 8) ? ~static_cast<uint64_t>(0) : ((static_cast<uint64_t>(1) << (8 * f.elem_size)) - 1);
      return v == none ? "none" : decimal(v);
    }
    case Fmt::Bound: {
      const uint64_t all_ones = (f.elem_size == 8) ? ~static_cast<uint64_t>(0) : ((static_cast<uint64_t>(1) << (8 * f.elem_size)) - 1);
      return v == all_ones ? "inf" : decimal(v);  // the bound b = infinity is the all-ones value
    }
    case Fmt::Routes:
      return routes_text(v);
    case Fmt::Default:
      break;
  }
  return decimal(v);
}

std::string ref_text(const uint8_t* p) {
  kz_ref r;
  std::memcpy(&r, p, sizeof(r));
  bool zero_hash = true;
  for (int i = 0; i < 32; ++i)
    if (r.artifact_sha256[i] != 0) zero_hash = false;
  if (zero_hash && r.section == 0 && r.index == 0) return "-";
  const SectionInfo* s = find_section(r.section);
  std::string sec = s != nullptr ? s->name : "#" + decimal(r.section);
  std::string prefix = zero_hash ? "self" : to_hex(r.artifact_sha256, 32);
  return prefix + "/" + sec + "#" + decimal(r.index);
}

std::string str_text(const uint8_t* p, const Container* c) {
  kz_str s;
  std::memcpy(&s, p, sizeof(s));
  std::string text;
  if (c != nullptr && c->string(s, &text)) return "\"" + escape_string(text) + "\"";
  return "str(" + decimal(s.off) + "," + decimal(s.len) + ")";
}

bool bytes_all_zero(const uint8_t* p, size_t n) {
  for (size_t i = 0; i < n; ++i)
    if (p[i] != 0) return false;
  return true;
}

std::string field_text(const kz_field_desc& f, const ResolvedFormat& rf, const uint8_t* p, const Container* c) {
  if (f.kind == KZ_FK_REF) return ref_text(p);
  if (f.kind == KZ_FK_STR) return str_text(p, c);
  if (f.count == 1) return scalar_text(f, rf, p);
  if (f.kind == KZ_FK_U8 && rf.fmt == Fmt::Default) return to_hex(p, f.count);
  std::string s = "[";
  for (uint32_t i = 0; i < f.count; ++i) {
    if (i > 0) s += ",";
    s += scalar_text(f, rf, p + static_cast<size_t>(i) * f.elem_size);
  }
  return s + "]";
}

}  // namespace

std::string ref_to_text(const kz_ref& ref) { return ref_text(reinterpret_cast<const uint8_t*>(&ref)); }

std::string escape_string(std::string_view s) {
  std::string out;
  for (unsigned char ch : s) {
    switch (ch) {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n"; break;
      case '\t': out += "\\t"; break;
      case '\r': out += "\\r"; break;
      default:
        if (ch < 0x20 || ch >= 0x7f) {
          char buf[8];
          std::snprintf(buf, sizeof(buf), "\\x%02x", ch);
          out += buf;
        } else {
          out.push_back(static_cast<char>(ch));
        }
    }
  }
  return out;
}

namespace {

// " field=value" for every field that is printed (zero reserved fields are not).
std::string fields_text(const kz_record_desc& rec, const void* record, const Container* c) {
  const uint8_t* r = static_cast<const uint8_t*>(record);
  std::string line;
  for (uint32_t i = 0; i < rec.nfields; ++i) {
    const kz_field_desc& f = rec.fields[i];
    const uint8_t* p = r + f.offset;
    if (is_reserved_field(f) && bytes_all_zero(p, static_cast<size_t>(f.elem_size) * f.count)) continue;
    ResolvedFormat rf = format_of(rec, f);
    line += " ";
    line += f.name;
    line += "=";
    line += field_text(f, rf, p, c);
  }
  return line;
}

}  // namespace

// Drops the single leading space of fields_text.
std::string trim_lead(const std::string& s) { return s.empty() ? s : s.substr(1); }

std::string dump_record(const kz_record_desc& rec, const void* record, const Container* c) {
  return std::string(rec.name) + fields_text(rec, record, c);
}

std::string dump_container(const Container& c) {
  const kz_header& h = c.header();
  std::string out = "kz-dump 1\n";
  const EnumDesc* kinds = find_enum("KZ_ARTIFACT");
  auto kind_text = [&](uint32_t k) -> std::string {
    const char* d = kinds != nullptr ? enum_display(*kinds, k) : nullptr;
    return d != nullptr ? std::string(d) : "#" + decimal(k);
  };
  out += "container abi=" + decimal(h.abi_version) + " kind=" + kind_text(h.artifact_kind) + " size=" + decimal(h.total_size) + "\n";
  out += "basis_sha256=" + to_hex(h.basis_sha256, 32) + "\n";
  out += "profile_sha256=" + to_hex(h.profile_sha256, 32) + "\n";
  out += "tool_sha256=" + to_hex(h.tool_sha256, 32) + "\n";
  out += "parents=" + decimal(c.parents().size()) + "\n";
  for (size_t i = 0; i < c.parents().size(); ++i) {
    const kz_parent& p = c.parents()[i];
    out += "  parent[" + decimal(i) + "] kind=" + kind_text(p.kind) + " role=" + decimal(p.role) + " sha256=" + to_hex(p.sha256, 32) + "\n";
  }
  out += "sections=" + decimal(c.section_table().size()) + "\n";
  for (const kz_section_entry& e : c.section_table()) {
    const SectionInfo* info = find_section(e.type);
    const std::string name = info != nullptr ? info->name : "#" + decimal(e.type);
    out += "section " + name + " type=" + decimal(e.type) + " version=" + decimal(e.schema_version) +
           " record=" + (info != nullptr && info->record != nullptr ? info->record->name : "opaque") +
           " record_size=" + decimal(e.record_size) + " count=" + decimal(e.count) + " crc32=" + hex_value(e.crc32) + "\n";
    if (info == nullptr || info->record == nullptr || e.type == KZ_SEC_STRINGS) continue;
    size_t size = 0;
    const uint8_t* data = c.section_data(e.type, &size);
    for (uint64_t i = 0; i < e.count; ++i)
      out += "  " + name + "[" + decimal(i) + "] " + trim_lead(fields_text(*info->record, data + i * e.record_size, &c)) + "\n";
  }
  out += "artifact_sha256=" + to_hex(c.artifact_hash()) + "\n";
  return out;
}

std::string layout_text() {
  std::string out = "kz-layout abi=" + decimal(KZ_ABI_VERSION) + "\n";
  for (const kz_record_desc* rec : records()) {
    out += "record " + std::string(rec->name) + " size=" + decimal(rec->size) + " fields=" + decimal(rec->nfields) + "\n";
    for (uint32_t i = 0; i < rec->nfields; ++i) {
      const kz_field_desc& f = rec->fields[i];
      out += "  " + std::string(f.name) + " offset=" + decimal(f.offset) + " size=" + decimal(f.elem_size) + " count=" + decimal(f.count) +
             " kind=" + kind_name(f.kind) + "\n";
    }
  }
  for (const EnumDesc& en : enums()) {
    out += std::string(en.is_set ? "flags " : "enum ") + en.name + "\n";
    for (size_t i = 0; i < en.nitems; ++i)
      out += "  " + hex_value(en.items[i].value) + " " + en.items[i].id + " \"" + en.items[i].display + "\"\n";
  }
  for (const SectionInfo& s : sections())
    out += "section " + std::string(s.name) + " type=" + decimal(s.type) + " record=" + (s.record != nullptr ? s.record->name : "?") +
           " version=" + decimal(s.schema_version) + "\n";
  for (const FieldFormat& f : formats()) {
    const char* fmt = "default";
    switch (f.fmt) {
      case Fmt::Enum: fmt = "enum"; break;
      case Fmt::Flags: fmt = "flags"; break;
      case Fmt::Hex: fmt = "hex"; break;
      case Fmt::Idx: fmt = "idx"; break;
      case Fmt::Bound: fmt = "bound"; break;
      case Fmt::Routes: fmt = "routes"; break;
      case Fmt::Default: break;
    }
    out += "format " + std::string(f.record) + "." + f.field + " " + fmt + " " + f.enum_name + "\n";
  }
  return out;
}

Digest schema_fingerprint() { return sha256(layout_text()); }

std::string build_identity() {
  std::string s = "kz-core abi=" + decimal(KZ_ABI_VERSION) + " schema=" + to_hex(schema_fingerprint()) + " source=" + std::string(core_source_identity()) +
                  " compiler=";
#if defined(__clang__)
  s += "clang " __clang_version__;
#elif defined(__GNUC__)
  s += "gcc " __VERSION__;
#else
  s += "unknown";
#endif
  s += " cxx=" + decimal(static_cast<uint64_t>(__cplusplus));
  return s;
}

}  // namespace kz

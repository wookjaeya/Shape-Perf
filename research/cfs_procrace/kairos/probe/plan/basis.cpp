// basis.cpp -- see basis.hpp.
#include "basis.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>

#include "anchor_resolve.hpp"
#include "kz/core/dump.hpp"

namespace kz::plan {

namespace {

std::string site_error(const ProfileSite& s, const Status& st) {
  return "site '" + s.label + "' (profile line " + std::to_string(s.source_line) + "): " + st.message;
}

kz_basis_item item(ContainerWriter& w, const std::string& name, const std::string& value, uint8_t policy = KZ_POLICY_UNSET) {
  kz_basis_item b;
  std::memset(&b, 0, sizeof b);
  b.name = w.intern(name);
  b.value = w.intern(value);
  b.policy = policy;
  b.prov = KZ_NONE32;
  b.deviation = KZ_NONE32;
  return b;
}

// SHA-256 of the lines [from, to] of a file (1-based, inclusive), newlines included.
Status hash_cited_lines(const std::string& path, uint32_t from, uint32_t to, Digest* out) {
  std::ifstream f(path, std::ios::binary);
  if (!f) return Status::error(Err::Io, "cannot read the cited file " + path);
  std::ostringstream ss;
  ss << f.rdbuf();
  const std::string text = ss.str();
  size_t pos = 0;
  uint32_t line = 1;
  size_t begin = std::string::npos, end = std::string::npos;
  while (pos <= text.size() && line <= to) {
    if (line == from) begin = pos;
    const size_t nl = text.find('\n', pos);
    const size_t next = nl == std::string::npos ? text.size() + 1 : nl + 1;
    if (line == to) {
      end = nl == std::string::npos ? text.size() : nl + 1;
      break;
    }
    pos = next;
    ++line;
  }
  if (begin == std::string::npos || end == std::string::npos || begin >= text.size())
    return Status::error(Err::Invalid, "the cited lines " + std::to_string(from) + "-" + std::to_string(to) + " do not exist in " + path);
  *out = sha256(text.data() + begin, end - begin);
  return Status::success();
}

std::string join_root(const std::string& root, const std::string& rel) {
  if (!rel.empty() && rel[0] == '/') return rel;
  if (root.empty()) return rel;
  return root.back() == '/' ? root + rel : root + "/" + rel;
}

}  // namespace

Status build_basis(const Bytes& binary, const Profile& profile, const BasisOptions& opt, Bytes* out, BasisReport* report) {
  BasisReport local;
  BasisReport& rep = report != nullptr ? *report : local;
  rep = BasisReport();
  anchor::ElfImage elf;
  Status s = anchor::ElfImage::parse(binary, &elf);
  if (!s.ok()) return s;
  anchor::LineTable lines;
  s = anchor::LineTable::parse(elf, &lines);
  if (!s.ok()) return s;
  if (lines.rows().empty()) return Status::error(Err::Invalid, "the binary has no DWARF line table (build it with -g): after-call targets cannot be resolved");
  if (elf.build_id_hex().empty()) return Status::error(Err::Invalid, "the binary has no build-id note: its identity would rest on the file hash alone; refusing to write a basis");

  ContainerWriter w(KZ_ARTIFACT_BASIS);
  w.set_profile_hash(profile.sha256);
  w.set_tool_hash(opt.tool_hash);

  // Provenance entries of the profile.
  std::vector<kz_prov> provs;
  for (const ProvDecl& p : profile.provs) {
    kz_prov k;
    std::memset(&k, 0, sizeof k);
    k.tier = p.tier;
    k.kind = p.kind;
    k.side = p.side;
    k.line_from = p.line_from;
    k.line_to = p.line_to;
    k.file = w.intern(p.file);
    k.cite = w.intern(p.cite);
    k.authored = w.intern(p.authored);
    std::string root_dir;
    bool root_known = false;
    if (p.root.empty()) {
      root_dir = opt.source_root;
      root_known = !opt.source_root.empty();
    } else {
      for (const auto& r : opt.roots)
        if (r.first == p.root) { root_dir = r.second; root_known = true; }
    }
    if (root_known && p.tier != KZ_TIER_UNREFERENCED && !p.file.empty()) {
      Digest d;
      Status hs = hash_cited_lines(join_root(root_dir, p.file), p.line_from, p.line_to, &d);
      if (!hs.ok()) return Status::error(hs.code, "provenance '" + p.id + "' (profile line " + std::to_string(p.source_line) + "): " + hs.message);
      if (p.has_sha256 && d != p.sha256)
        return Status::error(Err::Invalid, "provenance '" + p.id + "' (profile line " + std::to_string(p.source_line) + "): the cited lines of " + p.file +
                                               " hash to " + to_hex(d) + ", the profile declares " + to_hex(p.sha256) + ": the source is not the one the profile was written against");
      std::memcpy(k.text_sha256, d.data(), 32);
      if (p.has_sha256) ++rep.prov_verified; else ++rep.prov_computed;
    } else if (p.has_sha256) {
      std::memcpy(k.text_sha256, p.sha256.data(), 32);
      ++rep.prov_declared;
    } else {
      ++rep.prov_unhashed;
    }
    if (p.tier < 6) ++rep.tier_count[p.tier];
    provs.push_back(k);
  }

  // Targets.
  std::vector<kz_target> targets;
  for (uint32_t i = 0; i < profile.sites.size(); ++i) {
    const ProfileSite& ps = profile.sites[i];
    anchor::ResolvedTarget t;
    Status r = ps.kind != KZ_SITE_KIND_POINT_PROBE ? anchor::resolve_entry(elf, lines, ps.func, ps.in_suffix, &t)
               : ps.line_start                       ? anchor::resolve_line_start(elf, lines, ps.at_file, ps.at_line, &t)
                                                     : anchor::resolve_after_call(elf, lines, ps.at_file, ps.at_line, ps.after_call, &t);
    if (!r.ok()) return Status::error(r.code, site_error(ps, r));
    kz_target k;
    std::memset(&k, 0, sizeof k);
    k.id = i;
    k.kind = t.kind;
    k.insn_class = t.insn_class;
    k.vaddr = t.vaddr;
    k.file_offset = t.file_offset;
    k.func_vaddr = t.func_vaddr;
    k.delta = t.delta;
    k.line = t.line;
    k.col = t.col;
    k.prov = ps.prov >= 0 ? static_cast<uint32_t>(ps.prov) : KZ_NONE32;
    std::memcpy(k.insn, t.insn, 8);
    k.func = w.intern(t.func);
    k.file = w.intern(t.file);
    k.label = w.intern(ps.label);
    targets.push_back(k);
    const uint8_t tier = ps.prov >= 0 ? profile.provs[ps.prov].tier : static_cast<uint8_t>(KZ_TIER_UNREFERENCED);
    if (tier < 6) ++rep.sites_by_tier[tier];
  }
  rep.targets = targets.size();

  // Symbols.
  std::vector<kz_symbol> syms;
  syms.reserve(elf.functions().size());
  for (const anchor::ElfFunction& f : elf.functions()) {
    kz_symbol k;
    std::memset(&k, 0, sizeof k);
    k.vaddr = f.vaddr;
    k.size = f.size;
    uint64_t off = 0;
    k.file_offset = elf.vaddr_to_offset(f.vaddr, &off) ? off : 0;
    k.name = w.intern(f.name);
    k.bind = f.bind;
    syms.push_back(k);
  }
  rep.symbols = syms.size();

  // Items and file hashes.
  const Digest bin_hash = sha256(binary.data(), binary.size());
  std::vector<kz_basis_item> items;
  items.push_back(item(w, "binary.path", opt.binary_path));
  items.push_back(item(w, "binary.sha256", to_hex(bin_hash)));
  items.push_back(item(w, "binary.build_id", elf.build_id_hex()));
  items.push_back(item(w, "binary.size", std::to_string(binary.size())));
  items.push_back(item(w, "binary.type", elf.position_independent() ? "position-independent (ET_DYN)" : "fixed-address (ET_EXEC)"));
  items.push_back(item(w, "binary.arch", "x86-64"));
  items.push_back(item(w, "profile.binary_name", profile.binary_name));
  items.push_back(item(w, "profile.sites", std::to_string(profile.sites.size())));
  items.push_back(item(w, "anchor.line_table", "DWARF .debug_line, " + std::to_string(lines.files().size()) + " files, " + std::to_string(lines.rows().size()) + " rows"));
  items.push_back(item(w, "anchor.reader", "kz probe/anchor: ELF (elf.h) and DWARF line-table reader, no LLVM"));
  items.push_back(item(w, "anchor.source_root", opt.source_root.empty() ? "(not given: cited text not hashed)" : opt.source_root));
  items.push_back(item(w, "tool.build_identity", opt.build_identity.empty() ? build_identity() : opt.build_identity));
  std::vector<kz_file_hash> files(1);
  std::memset(&files[0], 0, sizeof(kz_file_hash));
  files[0].path = w.intern(opt.binary_path);
  std::memcpy(files[0].sha256, bin_hash.data(), 32);
  files[0].size = binary.size();
  files[0].prov = KZ_NONE32;

  if (!(s = w.add_section(KZ_SEC_BASIS, items)).ok()) return s;
  if (!(s = w.add_section(KZ_SEC_FILE_HASHES, files)).ok()) return s;
  if (!provs.empty() && !(s = w.add_section(KZ_SEC_PROVENANCE, provs)).ok()) return s;
  if (!(s = w.add_section(KZ_SEC_SYMBOLS, syms)).ok()) return s;
  if (!(s = w.add_section(KZ_SEC_TARGETS, targets)).ok()) return s;
  return w.finish(out);
}

Status load_basis_bytes(Bytes bytes, LoadedBasis* out) {
  *out = LoadedBasis();
  Status s = Container::parse(std::move(bytes), &out->container);
  if (!s.ok()) return s;
  s = out->container.verify_content();
  if (!s.ok()) return s;
  if (out->container.header().artifact_kind != KZ_ARTIFACT_BASIS)
    return Status::error(Err::Invalid, "the file is not a basis container (artifact kind is not 'basis')");
  std::vector<kz_basis_item> items;
  s = out->container.read_section(KZ_SEC_BASIS, &items);
  if (!s.ok()) return s;
  bool have_path = false, have_hash = false, have_id = false, have_size = false;
  for (const kz_basis_item& b : items) {
    std::string name, value;
    if (!out->container.string(b.name, &name) || !out->container.string(b.value, &value)) return Status::error(Err::BadRef, "BASIS: a string does not resolve");
    if (name == "binary.path") { out->binary_path = value; have_path = true; }
    else if (name == "binary.sha256") { have_hash = digest_from_hex(value, &out->binary_sha256); }
    else if (name == "binary.build_id") { out->build_id = value; have_id = !value.empty(); }
    else if (name == "binary.size") { out->binary_size = std::strtoull(value.c_str(), nullptr, 10); have_size = out->binary_size != 0; }
  }
  if (!have_path || !have_hash || !have_id || !have_size) return Status::error(Err::Invalid, "the basis does not identify its binary (binary.path/sha256/build_id/size are required)");
  s = out->container.read_section(KZ_SEC_TARGETS, &out->targets);
  if (!s.ok()) return s;
  for (const kz_target& t : out->targets) {
    std::string label, func, file;
    if (!out->container.string(t.label, &label) || !out->container.string(t.func, &func) || !out->container.string(t.file, &file))
      return Status::error(Err::BadRef, "TARGETS: a string does not resolve");
    out->target_labels.push_back(label);
    out->target_funcs.push_back(func);
    out->target_files.push_back(file);
  }
  return Status::success();
}

Status load_basis(const std::string& path, LoadedBasis* out) {
  Bytes b;
  Status s = read_whole_file(path, &b);
  if (!s.ok()) return s;
  return load_basis_bytes(std::move(b), out);
}

Status make_plan(const Profile& profile, const LoadedBasis& basis, std::vector<kz::trace::SiteSpec>* sites) {
  sites->clear();
  Digest want = basis.container.profile_hash();
  if (want != profile.sha256)
    return Status::error(Err::Invalid, "the basis was built for another profile text (profile hash " + to_hex(want) + " in the basis, " +
                                           to_hex(profile.sha256) + " for the profile given): rebuild the basis with this profile");
  if (basis.targets.size() != profile.sites.size())
    return Status::error(Err::Invalid, "the basis has " + std::to_string(basis.targets.size()) + " targets, the profile " + std::to_string(profile.sites.size()) + " sites");
  for (uint32_t i = 0; i < profile.sites.size(); ++i) {
    const ProfileSite& ps = profile.sites[i];
    const kz_target& t = basis.targets[i];
    if (t.id != i || basis.target_labels[i] != ps.label)
      return Status::error(Err::Invalid, "target " + std::to_string(i) + " is '" + basis.target_labels[i] + "' but the profile's site " + std::to_string(i) + " is '" + ps.label + "'");
    const uint16_t want_kind = ps.kind != KZ_SITE_KIND_POINT_PROBE ? KZ_TARGET_KIND_FUNC_ENTRY : ps.line_start ? KZ_TARGET_KIND_LINE_START : KZ_TARGET_KIND_AFTER_CALL;
    if (t.kind != want_kind) return Status::error(Err::Invalid, "target '" + ps.label + "' has the wrong kind for the site");
    kz::trace::SiteSpec spec;
    spec.site = i;
    spec.kind = ps.kind;
    spec.phases = ps.kind == KZ_SITE_KIND_ENTRY_PROBE ? KZ_SITE_PHASE_ENTRY : ps.kind == KZ_SITE_KIND_RET_PROBE ? KZ_SITE_PHASE_RET : KZ_SITE_PHASE_POINT;
    spec.decoder = ps.decoder;
    spec.insn_class = t.insn_class;
    spec.file_offset = t.file_offset;
    spec.vaddr = t.vaddr;
    spec.line = t.line;
    spec.label = ps.label;
    spec.symbol = basis.target_funcs[i];
    spec.source = ps.kind == KZ_SITE_KIND_POINT_PROBE ? basis.target_files[i] : std::string();
    spec.slots = ps.slots;
    sites->push_back(std::move(spec));
  }
  // Every site, and the slot-3 key of every entry/return pair (kz::trace::check_plan).
  Status st = kz::trace::check_plan(*sites);
  if (!st.ok()) {
    sites->clear();
    return st;
  }
  return Status::success();
}

Status verify_binary(const Bytes& binary, const LoadedBasis& basis) {
  std::string problems;
  if (binary.size() != basis.binary_size)
    problems += "size " + std::to_string(binary.size()) + " differs from the basis (" + std::to_string(basis.binary_size) + "); ";
  const Digest h = sha256(binary.data(), binary.size());
  if (h != basis.binary_sha256) problems += "SHA-256 " + to_hex(h) + " differs from the basis (" + to_hex(basis.binary_sha256) + "); ";
  anchor::ElfImage elf;
  Status s = anchor::ElfImage::parse(binary, &elf);
  if (!s.ok()) {
    problems += "not a readable ELF: " + s.message + "; ";
  } else {
    if (elf.build_id_hex() != basis.build_id) problems += "build-id " + elf.build_id_hex() + " differs from the basis (" + basis.build_id + "); ";
    for (size_t i = 0; i < basis.targets.size(); ++i) {
      uint8_t cur[8] = {0};
      size_t n = 8;
      while (n > 0 && !elf.read_offset(basis.targets[i].file_offset, n, cur)) --n;
      uint8_t want[8];
      std::memcpy(want, basis.targets[i].insn, 8);
      if (n < 8) std::memset(cur + n, 0, 8 - n);
      if (std::memcmp(cur, want, 8) != 0) problems += "the bytes at target '" + basis.target_labels[i] + "' differ; ";
    }
  }
  if (problems.empty()) return Status::success();
  return Status::error(Err::Invalid, "the binary is not the one the basis was built for: " + problems);
}

void site_records(const std::vector<kz::trace::SiteSpec>& sites, ContainerWriter* w, std::vector<kz_site>* out_sites,
                  std::vector<kz_site_slot>* out_slots) {
  for (const kz::trace::SiteSpec& s : sites) {
    kz_site k;
    std::memset(&k, 0, sizeof k);
    k.site = s.site;
    k.kind = s.kind;
    k.insn_class = s.insn_class;
    k.decoder = s.decoder;
    k.phases = s.phases;
    k.line = s.line;
    k.file_offset = s.file_offset;
    k.symbol = w->intern(s.symbol);
    k.label = w->intern(s.label);
    k.source = w->intern(s.source);
    out_sites->push_back(k);
    for (const kz::trace::SlotSpec& sl : s.slots) {
      kz_site_slot ks;
      std::memset(&ks, 0, sizeof ks);
      ks.site = s.site;
      ks.slot = sl.slot;
      ks.src = sl.src;
      ks.width = sl.width;
      ks.index = sl.index;
      ks.role = w->intern(sl.role);
      out_slots->push_back(ks);
    }
  }
}

Digest plan_hash(const std::vector<kz::trace::SiteSpec>& sites) {
  // A canonical text of what the plan is: nothing that depends on the run or the host.
  Sha256 h;
  for (const kz::trace::SiteSpec& s : sites) {
    std::string t = "site " + std::to_string(s.site) + " kind=" + std::to_string(s.kind) + " decoder=" + std::to_string(s.decoder) +
                    " offset=" + std::to_string(s.file_offset) + " class=" + std::to_string(s.insn_class) + " line=" + std::to_string(s.line) +
                    " label=" + s.label + " symbol=" + s.symbol + " source=" + s.source + "\n";
    h.update(t);
    for (const kz::trace::SlotSpec& sl : s.slots) {
      std::string u = " slot " + std::to_string(sl.slot) + " src=" + std::to_string(sl.src) + " width=" + std::to_string(sl.width) +
                      " index=" + std::to_string(sl.index) + " role=" + sl.role + "\n";
      h.update(u);
    }
  }
  return h.finish();
}

}  // namespace kz::plan

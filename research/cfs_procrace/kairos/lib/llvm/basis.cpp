// basis.cpp -- see basis.hpp.
#include "basis.hpp"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>

#include "elf_info.hpp"
#include "gitrev.hpp"
#include "kz/core/dump.hpp"
#include "linkset.hpp"
#include "recipe.hpp"
#include "serialize.hpp"

namespace kzs {

namespace fs = std::filesystem;

namespace {

template <class T>
T zero() {
  T t;
  std::memset(&t, 0, sizeof t);
  return t;
}

std::string rel_to(const std::string& path, const std::string& root) {
  std::string s = normalize_path(path);
  if (!root.empty()) {
    std::string r = normalize_path(root);
    if (r.back() != '/') r += '/';
    if (s.compare(0, r.size(), r) == 0) s = s.substr(r.size());
  }
  return s;
}

std::string abs_in(const std::string& dir, const std::string& p) {
  if (!p.empty() && p[0] == '/') return normalize_path(p);
  return normalize_path(dir + "/" + p);
}

bool read_lines(const std::string& path, std::vector<std::string>* lines) {
  std::ifstream f(path);
  if (!f) return false;
  std::string l;
  while (std::getline(f, l)) {
    if (!l.empty() && l.back() == '\r') l.pop_back();
    lines->push_back(l);
  }
  return true;
}

// SHA-256 of the lines [from, to] (1-based, inclusive), each followed by LF.
bool hash_lines(const std::string& path, uint32_t from, uint32_t to, kz::Digest* out) {
  std::vector<std::string> lines;
  if (!read_lines(path, &lines) || from == 0 || to < from || to > lines.size()) return false;
  kz::Sha256 h;
  for (uint32_t i = from; i <= to; ++i) {
    h.update(lines[i - 1]);
    h.update("\n", 1);
  }
  *out = h.finish();
  return true;
}

std::string trim(std::string s) {
  const char* ws = " \t\r\n";
  const size_t b = s.find_first_not_of(ws);
  if (b == std::string::npos) return "";
  return s.substr(b, s.find_last_not_of(ws) - b + 1);
}

// The leaves of one element of a flattened table, as text.
std::string leaf_text(const ImageFact& f) {
  switch (f.kind) {
    case KZ_IMAGE_KIND_INT: return std::to_string(f.value);
    case KZ_IMAGE_KIND_STRING: return "\"" + f.text + "\"";
    case KZ_IMAGE_KIND_ADDR: return "&" + f.text;
    case KZ_IMAGE_KIND_FUNC: return f.text;
    case KZ_IMAGE_KIND_NULLPTR: return "NULL";
    default: return "?" + f.text;
  }
}

class Builder {
 public:
  Builder(const BasisInputs& in, BasisReport* rep) : in_(in), rep_(rep), w_(KZ_ARTIFACT_BASIS) {}
  kz::Status run(const kz::phi::Profile& profile, kz::Bytes* out);

 private:
  uint32_t add_prov(uint8_t tier, uint8_t kind, uint8_t side, const std::string& file, uint32_t from, uint32_t to, const std::string& cite,
                    const std::string& authored, const kz::Digest* hash) {
    kz_prov p = zero<kz_prov>();
    p.tier = tier;
    p.kind = kind;
    p.side = side;
    p.file = w_.intern(file);
    p.line_from = from;
    p.line_to = to;
    p.cite = w_.intern(cite);
    p.authored = w_.intern(authored);
    if (hash != nullptr) std::memcpy(p.text_sha256, hash->data(), 32);
    provs_.push_back(p);
    return static_cast<uint32_t>(provs_.size() - 1);
  }
  uint32_t add_deviation(const std::string& subject, const std::string& reference, const std::string& used, const std::string& reason,
                         const std::string& effect, uint8_t avoidable) {
    kz_deviation d = zero<kz_deviation>();
    d.subject = w_.intern(subject);
    d.reference_value = w_.intern(reference);
    d.used_value = w_.intern(used);
    d.reason = w_.intern(reason);
    d.effect = w_.intern(effect);
    d.avoidable = avoidable;
    d.prov = KZ_NONE32;
    devs_.push_back(d);
    return static_cast<uint32_t>(devs_.size() - 1);
  }
  void item(const std::string& name, const std::string& value, uint8_t policy = KZ_POLICY_UNSET, uint32_t prov = KZ_NONE32, uint32_t dev = KZ_NONE32) {
    kz_basis_item b = zero<kz_basis_item>();
    b.name = w_.intern(name);
    b.value = w_.intern(value);
    b.policy = policy;
    b.prov = prov;
    b.deviation = dev;
    items_.push_back(b);
  }
  uint32_t add_file(const std::string& abs_path) {
    const std::string key = normalize_path(abs_path);
    auto it = file_idx_.find(key);
    if (it != file_idx_.end()) return it->second;
    kz::Digest d;
    uint64_t size = 0;
    kz::Status s = hash_file(key, &d, &size);
    if (!s.ok()) {
      rep_->warnings.push_back(s.describe());
      return KZ_NONE32;
    }
    kz_file_hash f = zero<kz_file_hash>();
    f.path = w_.intern(rel_to(key, in_.bundle_root));
    std::memcpy(f.sha256, d.data(), 32);
    f.size = size;
    f.prov = KZ_NONE32;
    files_.push_back(f);
    return file_idx_[key] = static_cast<uint32_t>(files_.size() - 1);
  }
  std::string file_sha(const std::string& abs_path) {
    kz::Digest d;
    uint64_t size = 0;
    if (!hash_file(normalize_path(abs_path), &d, &size).ok()) return "unreadable";
    return kz::to_hex(d);
  }

  kz::Status revisions(const kz::phi::Profile& profile);
  kz::Status program(const Recipe& recipe);
  kz::Status front_end(const Recipe& recipe);
  kz::Status constants(const kz::phi::Profile& profile, const Recipe& recipe);
  kz::Status generated_and_tables(const Recipe& recipe);
  kz::Status startup(const Recipe& recipe);
  kz::Status modules(const Recipe& recipe);
  kz::Status instrumentation();
  kz::Status unit_diagnostics(const Recipe& recipe, size_t from);
  kz::Status relocatables(const Recipe& recipe);
  size_t record_tus(size_t from);
  std::string map_virtual(const Recipe::Startup& st, const std::string& name) const;

  const UnitCommand* unit_of(const std::string& source_rel) const {
    for (const UnitCommand& u : units_)
      if (rel_to(u.source, in_.bundle_root) == source_rel) return &u;
    return nullptr;
  }

  const BasisInputs& in_;
  BasisReport* rep_;
  kz::ContainerWriter w_;
  std::vector<kz_basis_item> items_;
  std::vector<kz_file_hash> files_;
  std::vector<kz_prov> provs_;
  std::vector<kz_deviation> devs_;
  std::vector<kz_tu> tus_;
  std::map<std::string, uint32_t> file_idx_;
  std::vector<UnitCommand> units_;
  std::map<std::string, std::string> string_consts_;  // recipe string constants by name
  std::map<std::string, std::string> unit_member_;    // object -> how the link command takes it in
  std::string compiler_;
  std::vector<CdbEntry> cdb_;
  std::map<std::string, size_t> by_object_;           // object -> compile database entry
  std::vector<std::string> core_objects_;             // the members the executable's link takes in
  ElfTarget elf_target_;                              // the ELF identity of every file read (one program: one class and machine)
  std::vector<bool> diag_matched_;                    // recipe front_end_diagnostic -> it named a unit of the program
  RelocatableLinks relocatable_;                      // recipe relocatable: the object -> the plan of the link that wrote it
  std::set<std::string> relocatable_used_;            // the relocatable objects a link command took in
  std::vector<std::string> without_command_;          // link members no compile command produces (recorded)
  struct ScriptEntry {
    size_t index = 0;
    std::string kind;                 // "library", "app" or "other"
    std::vector<std::string> fields;  // fields[0] is the entry word
  };
  std::vector<ScriptEntry> script_entries_;
};

kz::Status Builder::revisions(const kz::phi::Profile& profile) {
  for (const kz::phi::SourceDecl& s : profile.sources) {
    const std::string dir = s.root.empty() ? in_.bundle_root : abs_in(in_.bundle_root, s.root);
    std::string head;
    kz::Status st = read_git_head(dir, &head);
    ++rep_->revisions_checked;
    const std::string base = "source." + s.name;
    item(base + ".root", s.root.empty() ? "." : s.root);
    item(base + ".revision_declared", s.rev, KZ_POLICY_FROM_REFERENCE);
    if (!st.ok()) {
      item(base + ".revision_actual", "unreadable: " + st.message, KZ_POLICY_UNSPECIFIED);
      rep_->warnings.push_back("revision of " + s.name + ": " + st.message);
      continue;
    }
    if (head == s.rev) {
      ++rep_->revisions_matching;
      item(base + ".revision_actual", head, KZ_POLICY_FROM_REFERENCE);
    } else {
      const uint32_t dev = add_deviation("revision of source tree " + s.name, s.rev, head, "the tree is not at the revision the profile pins",
                                         "facts read from this tree are not facts of the pinned revision", KZ_AVOIDABLE_UNKNOWN);
      item(base + ".revision_actual", head, KZ_POLICY_DEVIATION, KZ_NONE32, dev);
      rep_->warnings.push_back("source " + s.name + " is at " + head + ", the profile pins " + s.rev);
    }
  }
  item("source.worktree_state", "not checked: the commit is HEAD of the repository; every source file read is hashed (FILE_HASHES)", KZ_POLICY_UNSPECIFIED);
  return kz::Status::success();
}

kz::Status Builder::program(const Recipe& recipe) {
  std::vector<CdbEntry>& cdb = cdb_;
  kz::Status s = load_compile_db(in_.compile_db, &cdb);
  if (!s.ok()) return s;
  rep_->cdb_entries = cdb.size();
  add_file(in_.compile_db);
  item("compile_db.path", rel_to(in_.compile_db, in_.bundle_root));
  item("compile_db.entries", std::to_string(cdb.size()));

  std::map<std::string, size_t>& by_object = by_object_;
  for (size_t i = 0; i < cdb.size(); ++i) {
    const CdbEntry& e = cdb[i];
    std::string out;
    for (size_t k = 0; k + 1 < e.command_line.size(); ++k)
      if (e.command_line[k] == "-o") out = e.command_line[k + 1];
    if (out.empty()) out = e.output;
    if (!by_object.emplace(abs_in(e.directory, out), i).second) rep_->warnings.push_back("two compile commands produce " + abs_in(e.directory, out) + ": the first is used");
  }

  std::vector<std::pair<std::string, std::string>> chosen;  // object path, how the link command takes it in
  const std::string link_file = !in_.link_override.empty() ? in_.link_override : (recipe.link.empty() ? "" : abs_in(in_.build_dir, recipe.link));
  if (link_file.empty()) {
    for (const auto& kv : by_object) chosen.push_back({kv.first, "every entry of the compile database (no link command given)"});
    item("link.command", "none: the program is every entry of the compile database", KZ_POLICY_UNSPECIFIED);
  } else {
    std::ifstream f(link_file);
    if (!f) return kz::Status::error(kz::Err::Io, "cannot read the link command file " + link_file);
    std::ostringstream ss;
    ss << f.rdbuf();
    add_file(link_file);
    // the command ran in the directory above CMakeFiles
    std::string cwd = fs::path(link_file).parent_path().string();
    const size_t cm = link_file.find("/CMakeFiles/");
    if (cm != std::string::npos) cwd = link_file.substr(0, cm);
    LinkPlan plan;
    s = parse_link_command(ss.str(), cwd, &plan);
    if (!s.ok()) return s;
    if (!(s = relocatables(recipe)).ok()) return s;
    LinkSet set;
    s = resolve_link_set(plan, &set, relocatable_.empty() ? nullptr : &relocatable_);
    if (s.ok() && set.elf.known()) s = elf_target_.add(set.elf.first_path(), set.elf.id());
    if (!s.ok()) return s;
    rep_->link_inputs = plan.inputs.size();
    rep_->link_members = set.members.size();
    rep_->link_included = set.included();
    rep_->archives = set.archives;
    item("link.command", rel_to(link_file, in_.bundle_root));
    item("link.cwd", rel_to(cwd, in_.bundle_root));
    item("link.inputs", std::to_string(plan.inputs.size()) + " (" + std::to_string(set.archives) + " archives)");
    item("link.members_total", std::to_string(set.members.size()));
    item("link.members_in_program", std::to_string(set.included()));
    std::map<std::string, std::vector<std::string>> not_pulled;
    for (const LinkMember& m : set.members) {
      std::string how;
      switch (m.mode) {
        case LinkMember::Named: how = "named on the link command"; break;
        case LinkMember::Relocatable:
          how = "member of " + fs::path(m.archive).filename().string() + " (relocatable link)";
          relocatable_used_.insert(m.archive);
          break;
        case LinkMember::Whole: how = "member of " + fs::path(m.archive).filename().string() + " (whole archive)"; break;
        case LinkMember::Pulled: how = "member of " + fs::path(m.archive).filename().string() + " (pulled in for the symbol " + m.pulled_for + ")"; break;
        case LinkMember::NotPulled:
          not_pulled[fs::path(m.archive).filename().string()].push_back(fs::path(m.object).filename().string());
          ++rep_->lazy_not_pulled;
          continue;
      }
      chosen.push_back({m.object, how});
      core_objects_.push_back(m.object);
    }
    for (const auto& kv : not_pulled) {
      std::string v;
      for (const std::string& n : kv.second) v += (v.empty() ? "" : ", ") + n;
      item("link.not_in_program[" + kv.first + "]", v);
    }
    if (!plan.ignored.empty()) {
      std::string v;
      for (const std::string& n : plan.ignored) v += (v.empty() ? "" : " ") + n;
      item("link.options_ignored", v);
    }
  }

  // units
  std::set<std::string> seen;
  for (const auto& c : chosen) {
    auto it = by_object.find(c.first);
    if (it == by_object.end()) {
      ++rep_->unmatched_objects;
      rep_->warnings.push_back("no compile command produces " + c.first + " (" + c.second + ")");
      without_command_.push_back(rel_to(c.first, in_.bundle_root) + " (" + c.second + ")");
      continue;
    }
    if (!seen.insert(c.first).second) continue;
    const CdbEntry& e = cdb[it->second];
    UnitCommand u;
    u.directory = e.directory;
    u.source = abs_in(e.directory, e.file);
    u.object = c.first;
    u.command_line = e.command_line;
    units_.push_back(u);
    unit_member_[u.object] = c.second;
  }
  std::sort(units_.begin(), units_.end(), [&](const UnitCommand& a, const UnitCommand& b) {
    const std::string sa = rel_to(a.source, in_.bundle_root), sb = rel_to(b.source, in_.bundle_root);
    if (sa != sb) return sa < sb;
    return a.object < b.object;
  });
  rep_->units = units_.size();
  item("units.count", std::to_string(units_.size()));
  if (!units_.empty() && !units_.front().command_line.empty()) compiler_ = units_.front().command_line.front();

  item("units.distinct_flag_sets", std::to_string(record_tus(0)));
  return kz::Status::success();
}

// The rows of the TU table for units_[from..]; returns the number of distinct flag sets among them.
size_t Builder::record_tus(size_t from) {
  std::map<std::string, size_t> distinct;
  for (size_t ui = from; ui < units_.size(); ++ui) {
    const UnitCommand& u = units_[ui];
    const std::vector<std::string> fl = flag_list(u.command_line);
    kz_tu t = zero<kz_tu>();
    t.tu = static_cast<uint32_t>(tus_.size());
    t.fh = add_file(u.source);
    t.source = w_.intern(rel_to(u.source, in_.bundle_root));
    t.object = w_.intern(rel_to(u.object, in_.bundle_root));
    t.flags = w_.intern(join_command(fl));
    t.member = w_.intern(unit_member_[u.object]);
    t.directory = w_.intern(u.directory);
    t.compiler = w_.intern(u.command_line.empty() ? "" : u.command_line.front());
    kz::Sha256 h;
    h.update(u.command_line.empty() ? std::string() : u.command_line.front());
    h.update("\0", 1);
    for (const std::string& a : fl) {
      h.update(a);
      h.update("\0", 1);
    }
    h.update(rel_to(u.source, in_.bundle_root));
    h.update("\0", 1);
    const kz::Digest d = h.finish();
    std::memcpy(t.command_sha256, d.data(), 32);
    tus_.push_back(t);
    ++distinct[join_command(fl)];
  }
  return distinct.size();
}

kz::Status Builder::front_end(const Recipe& recipe) {
  item("compiler.recorded", compiler_.empty() ? "none" : compiler_);
  std::string comment = "unknown (no built binary given)";
  ElfNotes notes;
  if (!recipe.binary.empty()) {
    const std::string bin = abs_in(in_.build_dir, recipe.binary);
    kz::Status s = read_elf_notes(bin, &notes);
    if (s.ok()) {
      const kz::Status e = elf_target_.add(bin, notes.elf);
      if (!e.ok()) return e;
      add_file(bin);
      comment.clear();
      for (const std::string& c : notes.comments) comment += (comment.empty() ? "" : "; ") + c;
      item("binary.path", rel_to(bin, in_.bundle_root));
      item("binary.sha256", file_sha(bin));
      item("binary.build_id", notes.build_id_hex.empty() ? "none" : notes.build_id_hex);
      item("binary.type", notes.type);
    } else {
      rep_->warnings.push_back("binary: " + s.message);
      item("binary.path", rel_to(bin, in_.bundle_root) + " (unreadable)", KZ_POLICY_UNSPECIFIED);
    }
  }
  item("compiler.identification", comment, notes.comments.empty() ? KZ_POLICY_UNSPECIFIED : KZ_POLICY_UNSET);
  std::string adapt = "-Wno-unknown-warning-option -Wno-error -w";
  if (!in_.read.resource_dir.empty()) adapt += " -resource-dir=" + in_.read.resource_dir;
  for (const std::string& a : in_.read.extra_args) adapt += " " + a;
  const std::string version = clang_version_text();
  const uint32_t dev = add_deviation(
      "front end that reads the program", "the recorded commands (compiler " + (compiler_.empty() ? std::string("unknown") : compiler_) + ", identified as: " + comment + ")",
      "Clang LibTooling, " + version + "; arguments added to every command: " + adapt,
      "the analysis needs the syntax tree with explicit field identity (SPEC 4.2), which the build compiler does not offer",
      "diagnostics are changed (-w, -Wno-error, -Wno-unknown-warning-option); differences between the two compilers in builtin functions and predefined macros are possible and not excluded; "
      "the program described is the recorded command's program as another front end reads it",
      KZ_AVOIDABLE_NO);
  item("front_end", "Clang LibTooling, " + version, KZ_POLICY_DEVIATION, KZ_NONE32, dev);
  item("front_end.arguments_added", adapt, KZ_POLICY_DEVIATION, KZ_NONE32, dev);
  item("front_end.build_identity", in_.build_identity);
  return kz::Status::success();
}

kz::Status Builder::constants(const kz::phi::Profile& profile, const Recipe& recipe) {
  // group the queries by the unit that reads them
  struct Q {
    std::string source;
    std::vector<ConstQuery> queries;
    std::vector<std::string> names;  // item names
  };
  std::map<std::string, Q> groups;
  for (const kz::phi::ConstDecl& c : profile.consts) {
    if (c.value != kz::phi::ConstDecl::Value::RefEval) continue;
    Q& g = groups[c.ref_file];
    g.source = c.ref_file;
    g.queries.push_back(ConstQuery{c.ref_symbol, false});
    g.names.push_back("const." + c.name);
  }
  for (const Recipe::Startup::File& f : recipe.startup.files) {
    Q& g = groups[f.source];
    g.source = f.source;
    g.queries.push_back(ConstQuery{f.constant, true});
    g.names.push_back("const." + f.constant);
  }
  const Recipe::Startup::ModuleFile& mf = recipe.startup.module_file;
  if (mf.present && !mf.full_path && !recipe.modules.empty()) {
    Q& g = groups[mf.source];
    g.source = mf.source;
    for (const std::string& c : {mf.path_const, mf.ext_const}) {
      g.queries.push_back(ConstQuery{c, true});
      g.names.push_back("const." + c);
    }
  }
  for (const auto& kv : groups) {
    const Q& g = kv.second;
    const UnitCommand* unit = unit_of(g.source);
    if (unit == nullptr) {
      for (const std::string& n : g.names) {
        item(n, "not evaluated: " + g.source + " is not a translation unit of the program", KZ_POLICY_UNSPECIFIED);
        ++rep_->constants_failed;
      }
      continue;
    }
    std::vector<ConstResult> res;
    kz::Status s = evaluate_constants(*unit, in_.read, g.queries, &res);
    if (!s.ok()) {
      for (const std::string& n : g.names) {
        item(n, "not evaluated: " + s.message, KZ_POLICY_UNSPECIFIED);
        ++rep_->constants_failed;
      }
      continue;
    }
    for (size_t i = 0; i < res.size(); ++i) {
      const ConstResult& r = res[i];
      if (!r.ok) {
        item(g.names[i], "not evaluated: " + r.error, KZ_POLICY_UNSPECIFIED);
        ++rep_->constants_failed;
        continue;
      }
      ++rep_->constants_ok;
      uint32_t prov = KZ_NONE32;
      if (!r.macro_file.empty() && r.macro_line != 0) {
        kz::Digest d;
        const std::string abs = abs_in(in_.bundle_root, r.macro_file);
        const bool hashed = hash_lines(abs, r.macro_line, r.macro_line, &d);
        prov = add_prov(KZ_TIER_CODE_DERIVED, KZ_PROV_KIND_CODE_PATTERN, KZ_PROV_SIDE_RULE, r.macro_file, r.macro_line, r.macro_line,
                        "constant-evaluated in " + g.source + "; macro " + r.name + " defined as: " + r.macro_body, "", hashed ? &d : nullptr);
        add_file(abs);
      }
      const std::string value = r.is_string ? "\"" + r.text + "\"" : std::to_string(r.value);
      item(g.names[i], value + "   [evaluated in " + g.source + "]", KZ_POLICY_UNSET, prov);
      if (r.is_string) string_consts_[r.name] = r.text;
    }
  }
  return kz::Status::success();
}

kz::Status Builder::generated_and_tables(const Recipe& recipe) {
  for (const Recipe::Generated& g : recipe.generated) {
    const std::string abs = abs_in(in_.build_dir, g.path);
    add_file(abs);
    item("generated." + g.role + ".path", rel_to(abs, in_.bundle_root));
    item("generated." + g.role + ".sha256", file_sha(abs));
    ++rep_->generated;
  }
  std::map<std::string, std::map<std::string, std::vector<ImageFact>>> images_of_file;  // source -> object -> rows
  for (const Recipe::Table& t : recipe.tables) {
    const std::string abs = abs_in(in_.build_dir, t.file);
    const std::string rel = rel_to(abs, in_.bundle_root);
    auto cached = images_of_file.find(rel);
    if (cached == images_of_file.end()) {
      const UnitCommand* unit = unit_of(rel);
      std::map<std::string, std::vector<ImageFact>> images;
      if (unit == nullptr) {
        item("table." + t.symbol, "not read: " + rel + " is not a translation unit of the program", KZ_POLICY_UNSPECIFIED);
        continue;
      }
      kz::Status s = read_unit_images(*unit, in_.read, &images);
      if (!s.ok()) {
        item("table." + t.symbol, "not read: " + s.message, KZ_POLICY_UNSPECIFIED);
        continue;
      }
      cached = images_of_file.emplace(rel, std::move(images)).first;
    }
    auto it = cached->second.find(t.symbol);
    if (it == cached->second.end()) {
      item("table." + t.symbol, "no initializer for the symbol in " + rel, KZ_POLICY_UNSPECIFIED);
      continue;
    }
    ++rep_->tables;
    // elements in order; the first element whose leaves are all null or zero ends the table
    std::map<uint32_t, std::vector<const ImageFact*>> elems;
    for (const ImageFact& f : it->second) {
      if (f.path.empty() || f.path[0] != '[') continue;
      const size_t close = f.path.find(']');
      if (close == std::string::npos) continue;
      const std::string num = f.path.substr(1, close - 1);
      if (num.empty() || num.find_first_not_of("0123456789") != std::string::npos) continue;
      if (f.kind == KZ_IMAGE_KIND_ZERO_FILL) continue;
      elems[static_cast<uint32_t>(std::stoul(num))].push_back(&f);
    }
    uint32_t n = 0;
    std::vector<std::string> texts;
    bool terminated = false;
    for (const auto& kv : elems) {
      bool all_null = true;
      std::string text;
      for (const ImageFact* f : kv.second) {
        if (!(f->kind == KZ_IMAGE_KIND_NULLPTR || (f->kind == KZ_IMAGE_KIND_INT && f->value == 0))) all_null = false;
        text += (text.empty() ? "" : ", ") + leaf_text(*f);
      }
      if (all_null) {
        terminated = true;
        break;
      }
      texts.push_back(text);
      ++n;
    }
    item("table." + t.symbol + ".entries", std::to_string(n) + (terminated ? " (null-terminated)" : " (no terminator found)") + "   [initializer in " + rel + "]");
    for (size_t i = 0; i < texts.size(); ++i) item("table." + t.symbol + "[" + std::to_string(i) + "]", texts[i]);
  }
  return kz::Status::success();
}

kz::Status Builder::startup(const Recipe& recipe) {
  const Recipe::Startup& st = recipe.startup;
  if (!st.present) return kz::Status::success();
  item("startup.reset_type", st.reset, KZ_POLICY_DECLARED, add_prov(KZ_TIER_RESEARCHER, KZ_PROV_KIND_RESEARCHER, KZ_PROV_SIDE_NA, "", 0, 0, "recipe " + in_.recipe_path, "the reset type the basis is evaluated for is a basis input (SPEC 1.7 Input), entered in the recipe", nullptr));
  for (const Recipe::Startup::Cite& c : st.cites) {
    kz::Digest d;
    const std::string abs = abs_in(in_.bundle_root, c.file);
    const bool hashed = hash_lines(abs, c.from, c.to, &d);
    add_file(abs);
    const uint32_t prov = add_prov(KZ_TIER_CODE_DERIVED, KZ_PROV_KIND_CODE_PATTERN, KZ_PROV_SIDE_RULE, c.file, c.from, c.to, c.note, "", hashed ? &d : nullptr);
    item("startup.rule_source[" + c.file + ":" + std::to_string(c.from) + "-" + std::to_string(c.to) + "]", c.note.empty() ? "cited" : c.note, KZ_POLICY_UNSET, prov);
  }
  // the rule for this reset type
  const Recipe::Startup::Rule* rule = nullptr;
  for (const Recipe::Startup::Rule& r : st.rules)
    if (r.reset == st.reset) rule = &r;
  if (rule == nullptr) {
    item("startup.script", "no rule for the reset type " + st.reset, KZ_POLICY_UNSPECIFIED);
    return kz::Status::success();
  }
  std::string order;
  for (const std::string& r : rule->order) order += (order.empty() ? "" : " then ") + r;
  item("startup.files_tried", order);
  std::string chosen_abs;
  for (const std::string& role : rule->order) {
    const Recipe::Startup::File* file = nullptr;
    for (const Recipe::Startup::File& f : st.files)
      if (f.role == role) file = &f;
    if (file == nullptr) continue;
    auto cit = string_consts_.find(file->constant);
    if (cit == string_consts_.end()) {
      item("startup.file[" + role + "]", "constant " + file->constant + " not evaluated", KZ_POLICY_UNSPECIFIED);
      continue;
    }
    const std::string name = cit->second;
    const std::string mapped = map_virtual(st, name);
    if (mapped.empty()) {
      item("startup.file[" + role + "]", name + " (no mount point maps it into the build: a run-time file)", KZ_POLICY_UNSPECIFIED);
      continue;
    }
    std::error_code ec;
    if (!fs::is_regular_file(mapped, ec)) {
      item("startup.file[" + role + "]", name + " -> " + rel_to(mapped, in_.bundle_root) + " (absent)", KZ_POLICY_UNSPECIFIED);
      continue;
    }
    item("startup.file[" + role + "]", name + " -> " + rel_to(mapped, in_.bundle_root));
    chosen_abs = mapped;
    break;  // the first file that exists is opened
  }
  if (chosen_abs.empty()) {
    item("startup.script", "no file of the rule exists in the build", KZ_POLICY_UNSPECIFIED);
    return kz::Status::success();
  }
  add_file(chosen_abs);
  rep_->startup_script = rel_to(chosen_abs, in_.bundle_root);
  item("startup.script", rep_->startup_script);
  item("startup.script.sha256", file_sha(chosen_abs));
  std::vector<std::string> lines;
  read_lines(chosen_abs, &lines);
  size_t libs = 0, apps = 0, other = 0, idx = 0;
  for (const std::string& raw : lines) {
    const std::string line = trim(raw);
    if (line.empty() || line[0] == '!' || line[0] == '#') continue;
    std::vector<std::string> fields;
    std::string cur;
    for (char c : line) {
      if (c == ',' || c == ';') {
        fields.push_back(trim(cur));
        cur.clear();
      } else {
        cur.push_back(c);
      }
    }
    if (!trim(cur).empty()) fields.push_back(trim(cur));
    if (fields.empty()) continue;
    std::string kind = "other";
    if (fields[0] == st.library_word) {
      kind = "library";
      ++libs;
    } else if (fields[0] == st.app_word) {
      kind = "app";
      ++apps;
    } else {
      ++other;
    }
    std::string text = kind;
    for (size_t i = 1; i < fields.size(); ++i) text += (i == 1 ? " " : ", ") + fields[i];
    script_entries_.push_back(ScriptEntry{idx, kind, fields});
    item("startup.entry[" + std::to_string(idx++) + "]", text);
  }
  rep_->startup_libraries = libs;
  rep_->startup_apps = apps;
  item("startup.entries", std::to_string(libs) + " libraries, " + std::to_string(apps) + " applications" + (other != 0 ? ", " + std::to_string(other) + " of another kind" : ""));
  return kz::Status::success();
}

// A virtual path of the target (e.g. /mnt/x) mapped onto the build's directory by the longest
// mount point that is a prefix of it; empty when no mount point maps it.
std::string Builder::map_virtual(const Recipe::Startup& st, const std::string& name) const {
  std::string mapped;
  size_t best = 0;
  for (const Recipe::Startup::Mount& m : st.mounts)
    if (name.compare(0, m.point.size(), m.point) == 0 && m.point.size() >= best) {
      best = m.point.size();
      mapped = abs_in(in_.build_dir, m.dir) + name.substr(m.point.size());
    }
  return mapped;
}

std::string join_set(const std::set<std::string>& names, const char* sep = ", ") {
  std::string v;
  for (const std::string& n : names) v += (v.empty() ? "" : sep) + n;
  return v;
}

// The units the selected startup script loads at run time, linked with the executable (SPEC 4.1
// step 6, the app-unit probe).  A script line names a file (recipe `module_file`, constants
// evaluated in the unit of their use); the recipe's `module` names the link command that builds
// it, and the bytes that link wrote must be the bytes the script loads, or the unit is not linked.
// The link members of each module join the program.  The loader's binding is then recorded: a
// module's undefined symbols are looked up in the executable's exported symbols (its dynamic
// symbol table), then in the libraries with global scope loaded before it, in script order; an
// application's own symbols stay local (the scope words of the recipe).  The extraction of the
// merged program binds external names by name; that is the loader's binding only when no name is
// defined twice in the closure and every reference across units goes to the unit that defines
// it.  Both are checked here, and a difference is a deviation, not hidden.
kz::Status Builder::modules(const Recipe& recipe) {
  if (recipe.modules.empty()) return kz::Status::success();
  rep_->modules_requested = true;
  const Recipe::Startup& st = recipe.startup;
  const Recipe::Startup::ModuleFile& mf = st.module_file;
  auto pit = string_consts_.find(mf.path_const), eit = string_consts_.find(mf.ext_const);
  std::string why_not;
  if (script_entries_.empty()) why_not = "no startup script was selected";
  else if (!mf.full_path && (pit == string_consts_.end() || eit == string_consts_.end())) why_not = "the constants that name a module's file were not evaluated";
  const std::string exe = abs_in(in_.build_dir, recipe.binary);
  const std::string exe_label = fs::path(exe).filename().string();
  ObjectSymbols exports;
  if (why_not.empty()) {
    kz::Status s = read_dynamic_symbols(exe, &exports);
    if (!s.ok()) why_not = "the executable's exported symbols are not readable: " + s.message;
    else if (!(s = elf_target_.add(exe, exports.elf)).ok()) return s;
  }
  if (!why_not.empty()) {
    item("closure", "the executable only: " + why_not, KZ_POLICY_UNSPECIFIED);
    rep_->warnings.push_back("modules not linked: " + why_not);
    return kz::Status::success();
  }
  const size_t core_units = units_.size();
  item("closure.executable_exports", std::to_string(exports.defined.size()) + " symbols in the dynamic symbol table of " + rel_to(exe, in_.bundle_root));
  item("closure.binding_model",
       "a module's undefined symbol is bound to the executable's export, else to the first library with global scope loaded before it (script order); "
       "an application's symbols stay local; the system libraries in the loader's search list (the executable's and the modules' DT_NEEDED) are not read: "
       "a symbol none of the loaded units exports is listed as unbound here");

  // which unit of the closure defines each name (by name, as the merged extraction binds)
  std::map<std::string, std::set<std::string>> defined_by;
  for (const std::string& o : core_objects_) {
    ObjectSymbols os;
    if (read_object_symbols(o, &os).ok())
      for (const std::string& d : os.defined) defined_by[d].insert(exe_label);
  }
  struct Loaded {
    std::string name;
    std::set<std::string> exports;
  };
  std::vector<Loaded> global_scope;  // libraries with global scope, in load order
  struct Probe {
    std::string name;
    std::map<std::string, std::string> bound;  // undefined symbol -> provider ("" = none of the loaded units)
  };
  std::vector<Probe> probes;
  std::set<std::string> used, in_names, out_names;
  for (const ScriptEntry& e : script_entries_) {
    if (e.kind != "library" && e.kind != "app") continue;
    const std::string scope = e.kind == "library" ? st.library_scope : st.app_scope;
    if (e.fields.size() <= mf.field) {
      item("module[startup.entry[" + std::to_string(e.index) + "]]", "the line has no field " + std::to_string(mf.field) + ": no file named", KZ_POLICY_UNSPECIFIED);
      continue;
    }
    const std::string fname = e.fields[mf.field];
    std::string vpath = fname;
    if (!mf.full_path) {  // a name: under the mount constant, with the extension constant where it has none
      if (fname.find('/') == std::string::npos) vpath = pit->second + "/" + fname;
      if (fs::path(vpath).filename().string().find('.') == std::string::npos) vpath += eit->second;
    }
    const std::string mapped = map_virtual(st, vpath);
    std::error_code ec;
    const bool exists = !mapped.empty() && fs::is_regular_file(mapped, ec);
    const std::string key = "module[" + fname + "]";
    const std::string head = e.kind + " (startup.entry[" + std::to_string(e.index) + "], symbols " + scope + "): " + vpath +
                             (mapped.empty() ? " (no mount point maps it into the build)" : " -> " + rel_to(mapped, in_.bundle_root)) + (exists || mapped.empty() ? "" : " (absent)");
    const Recipe::Module* rm = nullptr;
    for (const Recipe::Module& m : recipe.modules)
      if (m.name == fname) rm = &m;
    if (rm == nullptr) {
      ++rep_->modules_outside;
      out_names.insert(fname);
      Loaded l{fname, {}};
      std::string why = "outside the closure: the recipe names no link command for it";
      ObjectSymbols ds;
      if (exists && read_dynamic_symbols(mapped, &ds).ok()) {
        const kz::Status es = elf_target_.add(mapped, ds.elf);  // loaded into the same process
        if (!es.ok()) return es;
        l.exports = ds.defined;
        why += "; the " + std::to_string(ds.defined.size()) + " symbols it exports are read from the loaded file, for the binding only";
      }
      item(key, head + "; " + why, KZ_POLICY_UNSPECIFIED);
      if (scope == "global") global_scope.push_back(l);
      continue;
    }
    used.insert(fname);
    item(key, head);
    auto not_linked = [&](const std::string& why) {
      item(key + ".closure", "not linked: " + why, KZ_POLICY_UNSPECIFIED);
      rep_->warnings.push_back("module " + fname + " not linked: " + why);
    };
    if (!exists) {
      not_linked("the file the script loads is not in the build");
      continue;
    }
    const std::string link_file = abs_in(in_.build_dir, rm->link);
    std::ifstream f(link_file);
    if (!f) {
      not_linked("cannot read the link command file " + rel_to(link_file, in_.bundle_root));
      continue;
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    std::string cwd = fs::path(link_file).parent_path().string();
    const size_t cm = link_file.find("/CMakeFiles/");
    if (cm != std::string::npos) cwd = link_file.substr(0, cm);
    LinkPlan plan;
    LinkSet set;
    kz::Status s = parse_link_command(ss.str(), cwd, &plan);
    if (s.ok()) s = resolve_link_set(plan, &set, relocatable_.empty() ? nullptr : &relocatable_);
    for (const LinkMember& m : set.members)
      if (m.mode == LinkMember::Relocatable) relocatable_used_.insert(m.archive);
    if (!s.ok()) {
      not_linked(s.message);
      continue;
    }
    if (set.elf.known() && !(s = elf_target_.add(set.elf.first_path(), set.elf.id())).ok()) return s;
    add_file(link_file);
    item(key + ".link.command", rel_to(link_file, in_.bundle_root));
    const std::string out_sha = plan.output.empty() ? "unreadable" : file_sha(plan.output), loaded_sha = file_sha(mapped);
    if (out_sha == "unreadable" || out_sha != loaded_sha) {
      item(key + ".link.output_is_the_loaded_file",
           "no: the link writes " + (plan.output.empty() ? std::string("no named file") : rel_to(plan.output, in_.bundle_root) + " (sha256 " + out_sha + ")") + ", the script loads sha256 " + loaded_sha,
           KZ_POLICY_UNSPECIFIED);
      not_linked("the bytes the link wrote are not the bytes the script loads");
      continue;
    }
    add_file(mapped);
    item(key + ".link.output_is_the_loaded_file", "yes: " + rel_to(plan.output, in_.bundle_root) + ", sha256 " + out_sha);
    // the members join the program
    ObjectSymbols syms;
    size_t nunits = 0;
    for (const LinkMember& m : set.members) {
      if (m.mode == LinkMember::NotPulled) continue;
      ObjectSymbols os;
      if (read_object_symbols(m.object, &os).ok()) {
        syms.defined.insert(os.defined.begin(), os.defined.end());
        syms.hidden.insert(os.hidden.begin(), os.hidden.end());
        syms.undefined.insert(os.undefined.begin(), os.undefined.end());
      }
      auto it = by_object_.find(m.object);
      if (it == by_object_.end()) {
        ++rep_->unmatched_objects;
        rep_->warnings.push_back("no compile command produces " + m.object + " (module " + fname + ")");
        without_command_.push_back(rel_to(m.object, in_.bundle_root) + " (module " + fname + ")");
        continue;
      }
      if (unit_member_.count(m.object) != 0) continue;
      const CdbEntry& ce = cdb_[it->second];
      UnitCommand u;
      u.directory = ce.directory;
      u.source = abs_in(ce.directory, ce.file);
      u.object = m.object;
      u.command_line = ce.command_line;
      units_.push_back(u);
      std::string how = m.mode == LinkMember::Named ? "named on its link command" : "member of " + fs::path(m.archive).filename().string();
      if (m.mode == LinkMember::Relocatable) how += " (relocatable link)";
      unit_member_[m.object] = "module " + fname + " (" + e.kind + ", symbols " + scope + "), " + how;
      ++nunits;
    }
    for (const std::string& d : syms.defined) syms.undefined.erase(d);
    for (const std::string& d : syms.defined) defined_by[d].insert(fname);
    ++rep_->modules_in;
    rep_->module_units += nunits;
    in_names.insert(fname);
    item(key + ".units", std::to_string(nunits));
    // the loader's binding of its undefined symbols: those the loaded file still asks the loader
    // for (its dynamic symbol table); the others the module's own link resolved (e.g. symbols the
    // static linker defines in its output)
    ObjectSymbols loaded;
    kz::Status ls = read_dynamic_symbols(mapped, &loaded);
    if (!ls.ok()) rep_->warnings.push_back("module " + fname + ": " + ls.message);
    else if (!(s = elf_target_.add(mapped, loaded.elf)).ok()) return s;
    Probe p;
    p.name = fname;
    std::map<std::string, std::set<std::string>> by_provider;
    std::set<std::string> by_link;
    for (const std::string& u : syms.undefined) {
      if (ls.ok() && loaded.undefined.count(u) == 0) {
        by_link.insert(u);
        p.bound[u] = "the link of " + fname;
        continue;
      }
      std::string provider;
      if (exports.defined.count(u) != 0) provider = exe_label;
      else
        for (const Loaded& l : global_scope)
          if (l.exports.count(u) != 0) {
            provider = l.name;
            break;
          }
      p.bound[u] = provider;
      by_provider[provider].insert(u);
    }
    std::string counts;
    for (const auto& kv : by_provider) {
      if (kv.first.empty()) continue;
      counts += (counts.empty() ? "" : ", ") + kv.first + " " + std::to_string(kv.second.size());
      (kv.first == exe_label ? rep_->bound_executable : rep_->bound_library) += kv.second.size();
    }
    const size_t nun = by_provider.count("") != 0 ? by_provider[""].size() : 0;
    rep_->unbound += nun;
    item(key + ".undefined", std::to_string(syms.undefined.size()) + " (bound to " + (counts.empty() ? std::string("none") : counts) + "; unbound " + std::to_string(nun) +
                                 "; resolved by the module's own link " + std::to_string(by_link.size()) + ")");
    for (const auto& kv : by_provider)
      item(key + (kv.first.empty() ? std::string(".unbound") : ".bound[" + kv.first + "]"), join_set(kv.second));
    if (!by_link.empty()) item(key + ".resolved_by_its_link", join_set(by_link));
    if (scope == "global") {
      Loaded l{fname, {}};
      for (const std::string& d : syms.defined)
        if (syms.hidden.count(d) == 0) l.exports.insert(d);
      global_scope.push_back(l);
    }
    probes.push_back(p);
  }
  for (const Recipe::Module& m : recipe.modules)
    if (used.count(m.name) == 0) item("module[" + m.name + "]", "not loaded by the selected startup script: not in the closure", KZ_POLICY_UNSPECIFIED);

  // the merged program binds by name: compare with the loader
  std::set<std::string> dups, differs;
  for (const auto& kv : defined_by)
    if (kv.second.size() > 1) dups.insert(kv.first + " (" + join_set(kv.second, " and ") + ")");
  size_t across = 0;
  for (const Probe& p : probes)
    for (const auto& kv : p.bound) {
      auto it = defined_by.find(kv.first);
      if (it == defined_by.end()) continue;  // no unit of the closure defines it: no body either way
      ++across;
      if (it->second.size() != 1 || it->second.count(kv.second) == 0)
        differs.insert(p.name + ": " + kv.first + " is defined in the closure by " + join_set(it->second, " and ") + ", the loader binds it to " +
                       (kv.second.empty() ? std::string("nothing") : kv.second));
    }
  rep_->across_units = across;
  rep_->duplicate_definitions = dups.size();
  rep_->binding_differs = differs.size();
  std::vector<std::string> mods(in_names.begin(), in_names.end());
  item("closure.modules", exe_label + (mods.empty() ? "" : " + " + join_set(in_names)));
  item("closure.loaded_outside", out_names.empty() ? "none" : join_set(out_names), out_names.empty() ? KZ_POLICY_UNSET : KZ_POLICY_UNSPECIFIED);
  if (dups.empty()) {
    item("closure.duplicate_definitions", "none");
  } else {
    const uint32_t dev = add_deviation("names defined by more than one unit of the closure", "one definition per name", join_set(dups, "; "),
                                       "the loader binds each reference to one definition; the units define the same name",
                                       "the extraction keys external functions and objects by name and reads the first definition: the facts of the others are missing",
                                       KZ_AVOIDABLE_UNKNOWN);
    item("closure.duplicate_definitions", join_set(dups, "; "), KZ_POLICY_DEVIATION, KZ_NONE32, dev);
    rep_->warnings.push_back(std::to_string(dups.size()) + " names are defined by more than one unit of the closure");
  }
  if (differs.empty()) {
    item("closure.name_binding", "the loader's binding: each of the " + std::to_string(across) + " references across units goes to the one unit that defines the name");
  } else {
    const uint32_t dev = add_deviation("binding of references across units", "the dynamic loader's binding (closure.binding_model)", join_set(differs, "; "),
                                       "the merged extraction binds by name", "these references reach, in the facts, a body the loader does not bind them to", KZ_AVOIDABLE_UNKNOWN);
    item("closure.name_binding", std::to_string(differs.size()) + " of " + std::to_string(across) + " references across units differ from the loader: " + join_set(differs, "; "),
         KZ_POLICY_DEVIATION, KZ_NONE32, dev);
    rep_->warnings.push_back(std::to_string(differs.size()) + " references across units are bound differently by the loader");
  }
  // the rows of the new units, in source order after those of the executable
  std::sort(units_.begin() + static_cast<std::ptrdiff_t>(core_units), units_.end(), [&](const UnitCommand& a, const UnitCommand& b) {
    const std::string sa = rel_to(a.source, in_.bundle_root), sb = rel_to(b.source, in_.bundle_root);
    if (sa != sb) return sa < sb;
    return a.object < b.object;
  });
  const size_t flag_sets = record_tus(core_units);
  item("closure.units", std::to_string(units_.size()) + " (" + std::to_string(core_units) + " of " + exe_label + ", " + std::to_string(units_.size() - core_units) + " of " +
                            std::to_string(in_names.size()) + " modules; distinct flag sets among the module units " + std::to_string(flag_sets) + ")");
  rep_->units = units_.size();
  return kz::Status::success();
}

// The recipe's relocatable statements (recipe.hpp): the plan of each relocatable link, checked, by the object it writes.
kz::Status Builder::relocatables(const Recipe& recipe) {
  for (const Recipe::Relocatable& x : recipe.relocatables) {
    const std::string object = normalize_path(abs_in(in_.build_dir, x.object)), file = abs_in(in_.build_dir, x.link);
    std::ifstream f(file);
    if (!f) return kz::Status::error(kz::Err::Io, "cannot read the relocatable link command file " + file);
    std::ostringstream ss;
    ss << f.rdbuf();
    std::string cwd = fs::path(file).parent_path().string();  // as for the link command (program())
    const size_t cm = file.find("/CMakeFiles/");
    if (cm != std::string::npos) cwd = file.substr(0, cm);
    LinkPlan plan;
    kz::Status s = parse_link_command(ss.str(), cwd, &plan);
    if (s.ok()) s = relocatable_plan_check(plan, object);
    if (!s.ok()) return kz::Status::error(s.code, "recipe relocatable " + x.object + ": " + s.message);
    add_file(file);
    item("link.relocatable[" + rel_to(object, in_.bundle_root) + "]", rel_to(file, in_.bundle_root) + ": " + std::to_string(plan.inputs.size()) + " objects");
    relocatable_[object] = plan;
  }
  return kz::Status::success();
}

// The recipe's front_end_diagnostic statements for the units from `from` on (recipe.hpp): -Wno-error=<name> for that unit
// only, each recorded as an analysis-only deviation with the build compiler's reference.  What the arguments lower is
// established, not assumed: the unit is parsed without and with them, warnings reported (read_unit_diagnostics), and the
// lowered occurrences (an error of the first parse that the second reports as a warning, same diagnostic and place) must
// be exactly the cited ones: each of the named diagnostic (not another member of a group), each on a cited line, each
// cited line with one, and no error left.  Each occurrence is recorded (the extraction's -w then suppresses it).
kz::Status Builder::unit_diagnostics(const Recipe& recipe, size_t from) {
  diag_matched_.resize(recipe.front_end_diagnostics.size(), false);
  auto where = [](const UnitDiagnostic& d) { return d.file + ":" + std::to_string(d.line) + ":" + std::to_string(d.column); };
  for (size_t ui = from; ui < units_.size(); ++ui) {
    UnitCommand& u = units_[ui];
    const std::string rel = rel_to(u.source, in_.bundle_root);
    std::vector<size_t> ks;  // the statements of this unit
    std::vector<std::string> args;
    for (size_t k = 0; k < recipe.front_end_diagnostics.size(); ++k)
      if (recipe.front_end_diagnostics[k].unit == rel) {
        ks.push_back(k);
        args.push_back("-Wno-error=" + recipe.front_end_diagnostics[k].name);
      }
    if (ks.empty()) continue;
    const std::string what = "recipe front_end_diagnostic for " + rel + ": ";
    std::vector<UnitDiagnostic> before, after;
    std::string err;
    if (!read_unit_diagnostics(u, in_.read, {}, &before, &err) || !read_unit_diagnostics(u, in_.read, args, &after, &err))
      return kz::Status::error(kz::Err::Invalid, what + err);
    std::vector<const UnitDiagnostic*> lowered;
    for (const UnitDiagnostic& a : after) {
      if (a.error) return kz::Status::error(kz::Err::Invalid, what + "the front end still rejects the unit: " + where(a) + ": " + a.message);
      for (const UnitDiagnostic& b : before)
        if (b.error && b.id == a.id && b.file == a.file && b.line == a.line && b.column == a.column) {
          lowered.push_back(&a);
          break;
        }
    }
    for (const UnitDiagnostic* l : lowered) {
      bool named = false;
      for (size_t k : ks) named = named || recipe.front_end_diagnostics[k].name == l->option;
      if (!named)
        return kz::Status::error(kz::Err::Invalid, what + "it lowers " + (l->option.empty() ? std::string("a diagnostic without a warning option") : l->option) + " at " + where(*l) +
                                                       ", which no statement of the unit names (a group name lowers its members; name the diagnostic itself)");
    }
    for (size_t k : ks) {
      const Recipe::FrontEndDiagnostic& d = recipe.front_end_diagnostics[k];
      diag_matched_[k] = true;
      std::string occ;
      size_t n = 0;
      for (const UnitDiagnostic* l : lowered) {
        if (l->option != d.name) continue;
        bool cited = false;
        for (const Recipe::FrontEndDiagnostic::At& a : d.at) cited = cited || (a.file == l->file && a.line == l->line);
        if (!cited) return kz::Status::error(kz::Err::Invalid, what + d.name + " at " + where(*l) + " is on a line the statement does not cite: " + l->message);
        occ += (occ.empty() ? "" : "; ") + where(*l) + " " + l->message;
        ++n;
      }
      for (const Recipe::FrontEndDiagnostic::At& a : d.at) {
        bool found = false;
        for (const UnitDiagnostic* l : lowered) found = found || (l->option == d.name && a.file == l->file && a.line == l->line);
        if (!found) return kz::Status::error(kz::Err::Invalid, what + "the statement cites " + a.file + ":" + std::to_string(a.line) + ", where the front end raises no " + d.name + " as an error");
      }
      const std::string arg = "-Wno-error=" + d.name;
      u.analysis_args.push_back(arg);
      const uint32_t dev = add_deviation(
          "front-end diagnostic " + d.name + " of " + rel, "the recorded command, which the build compiler compiles without an error (" + d.reference + ")",
          arg + " added to this unit's command for the front end only",
          "the front end raises " + d.name + " as an error by default and would not read the unit; the build compiler accepts it (the reference)",
          "analysis only: the front end reports this one diagnostic of this one unit as a warning (and -w then suppresses it) instead of rejecting the unit; "
          "the recorded command, the executable the build compiler builds, the other units and the other diagnostics are unchanged",
          KZ_AVOIDABLE_NO);
      item("front_end.unit_diagnostic[" + rel + "][" + d.name + "]", arg, KZ_POLICY_DEVIATION, KZ_NONE32, dev);
      item("front_end.lowered[" + rel + "][" + d.name + "]", std::to_string(n) + " occurrence(s), each on a cited line: " + occ, KZ_POLICY_DEVIATION, KZ_NONE32, dev);
    }
  }
  return kz::Status::success();
}

// A derived build (SPEC 1.7 B'): the build compiler's plugin options in the recorded commands, and the link members no
// compile command produces (e.g. an instrumentation runtime linked from outside the build).  Nothing is written for a
// build without them, so the basis of a reference build is unchanged.
kz::Status Builder::instrumentation() {
  if (!without_command_.empty()) {
    std::string v;
    for (const std::string& m : without_command_) v += (v.empty() ? "" : "; ") + m;
    item("link.members_without_command", std::to_string(without_command_.size()) + ": " + v + " (in the program; no facts are read from them)");
  }
  std::map<std::string, size_t> sets;  // a unit's plugin options, as recorded -> units
  std::set<std::string> plugin_files;
  size_t with = 0;
  for (const UnitCommand& u : units_) {
    std::string opts;
    for (size_t k = 1; k < u.command_line.size(); ++k) {
      const std::string& a = u.command_line[k];
      if (!is_compiler_plugin_option(a)) continue;
      opts += (opts.empty() ? "" : " ") + a;
      if (a.rfind("-fplugin=", 0) == 0) plugin_files.insert(abs_in(u.directory, a.substr(9)));
    }
    if (opts.empty()) continue;
    ++with;
    ++sets[opts];
  }
  if (with == 0) return kz::Status::success();
  const std::string used = std::to_string(with) + " of " + std::to_string(units_.size()) + " units, " + std::to_string(sets.size()) + " distinct option set(s)";
  const uint32_t dev = add_deviation(
      "build-compiler plugin in the recorded commands", "commands without a compiler plugin (the reference configuration)", used,
      "a derived configuration: an instrumented build of the same sources (SPEC 1.7 B'; every difference from the reference build is recorded)",
      "the basis records the commands as they ran; the front end reads every unit without the plugin options (it cannot load a plugin of the build compiler). "
      "That this is the program the build compiler parsed holds for a plugin that changes neither preprocessing nor parsing (kzgcc registers passes and "
      "observing callbacks only, no pragma, attribute or macro: instrument/gcc/kzgcc.cpp plugin_init); for another plugin it is not established. The built objects contain what the plugin inserted, which the facts do not describe",
      KZ_AVOIDABLE_NO);
  item("instrumentation.units_with_compiler_plugin", used, KZ_POLICY_DEVIATION, KZ_NONE32, dev);
  size_t k = 0;
  for (const auto& kv : sets) item("instrumentation.compiler_plugin_options[" + std::to_string(k++) + "]", kv.first + "  (" + std::to_string(kv.second) + " units)", KZ_POLICY_DEVIATION, KZ_NONE32, dev);
  for (const std::string& f : plugin_files) {
    std::error_code ec;
    if (fs::is_regular_file(f, ec)) add_file(f);
    item("instrumentation.plugin_file", rel_to(f, in_.bundle_root) + " sha256 " + file_sha(f), KZ_POLICY_DEVIATION, KZ_NONE32, dev);
  }
  item("front_end.arguments_removed", "-fplugin=... and -fplugin-arg-... of the recorded commands", KZ_POLICY_DEVIATION, KZ_NONE32, dev);
  return kz::Status::success();
}

kz::Status Builder::run(const kz::phi::Profile& profile, kz::Bytes* out) {
  w_.set_profile_hash(in_.profile_hash);
  w_.set_tool_hash(in_.tool_hash);
  Recipe recipe;
  if (!in_.recipe_text.empty()) {
    kz::Status s = parse_recipe(in_.recipe_text, &recipe);
    if (!s.ok()) return s;
    item("recipe.name", recipe.name);
    item("recipe.path", in_.recipe_path);
    kz::Digest d = kz::sha256(in_.recipe_text);
    item("recipe.sha256", kz::to_hex(d));
  }
  item("bundle.root", in_.bundle_root);
  item("build.dir", in_.build_dir);
  item("profile.name", profile.name + " / " + profile.revision);
  kz::Status s = revisions(profile);
  if (!s.ok()) return s;
  if (!(s = program(recipe)).ok()) return s;
  if (!(s = unit_diagnostics(recipe, 0)).ok()) return s;  // the executable's units (before the constants are evaluated in them)
  if (!(s = front_end(recipe)).ok()) return s;
  if (!(s = constants(profile, recipe)).ok()) return s;
  if (!(s = generated_and_tables(recipe)).ok()) return s;
  if (!(s = startup(recipe)).ok()) return s;
  const size_t core_units = units_.size();
  if (!(s = modules(recipe)).ok()) return s;
  if (!(s = unit_diagnostics(recipe, core_units)).ok()) return s;  // the modules' units
  for (const auto& kv : relocatable_)
    if (relocatable_used_.count(kv.first) == 0)
      return kz::Status::error(kz::Err::Invalid, "recipe relocatable " + rel_to(kv.first, in_.bundle_root) + ": no link command takes it in");
  for (size_t k = 0; k < recipe.front_end_diagnostics.size(); ++k)
    if (k >= diag_matched_.size() || !diag_matched_[k])
      return kz::Status::error(kz::Err::Invalid, "recipe front_end_diagnostic `" + recipe.front_end_diagnostics[k].name + "` names " + recipe.front_end_diagnostics[k].unit +
                                                     ", which is no unit of the program");
  // the target's ELF identity, where it is not the ELF64 one every basis before 2026-10-29 had (so such a basis is written
  // as before; a reader takes a basis without the item as ELF64 little-endian): for the correspondence of the build with
  // the runtime and its recovery tool (pointer size and byte order, PI decision D5 of 2026-10-29)
  if (elf_target_.known() && elf_target_.id().pointer_bytes() != 8)
    item("target.elf", elf_target_.id().text() + ", pointer " + std::to_string(elf_target_.id().pointer_bytes()) +
                           " bytes: every object, archive member, executable and module this basis read (the first: " + rel_to(elf_target_.first_path(), in_.bundle_root) + ")");
  if (!(s = instrumentation()).ok()) return s;
  for (const Recipe::Declared& d : recipe.declared) {
    const uint32_t prov = add_prov(KZ_TIER_RESEARCHER, KZ_PROV_KIND_RESEARCHER, KZ_PROV_SIDE_NA, "", 0, 0, "recipe " + in_.recipe_path, d.authored, nullptr);
    item(d.name, d.value, KZ_POLICY_DECLARED, prov);
    ++rep_->hand_entered;
  }
  // the hand-entered reset type counts as hand-entered too
  if (recipe.startup.present) ++rep_->hand_entered;

  // fix the TU file-hash indices now that every file is listed: they were taken from the same table
  kz::Status st = w_.add_section(KZ_SEC_BASIS, items_);
  if (!st.ok()) return st;
  if (!(st = w_.add_section(KZ_SEC_FILE_HASHES, files_)).ok()) return st;
  if (!(st = w_.add_section(KZ_SEC_PROVENANCE, provs_)).ok()) return st;
  if (!(st = w_.add_section(KZ_SEC_DEVIATIONS, devs_)).ok()) return st;
  if (!(st = w_.add_section(KZ_SEC_TUS, tus_)).ok()) return st;
  return w_.finish(out);
}

}  // namespace

kz::Status build_basis(const BasisInputs& in, const kz::phi::Profile& profile, kz::Bytes* out, BasisReport* report) {
  BasisReport local;
  BasisReport* rep = report != nullptr ? report : &local;
  *rep = BasisReport();
  Builder b(in, rep);
  return b.run(profile, out);
}

kz::Status units_from_basis(const kz::Container& basis, std::vector<UnitCommand>* units, std::string* bundle_root) {
  units->clear();
  std::vector<kz_tu> tus;
  kz::Status s = basis.read_section(KZ_SEC_TUS, &tus);
  if (!s.ok()) return s;
  std::vector<kz_basis_item> items;
  if (!(s = basis.read_section(KZ_SEC_BASIS, &items)).ok()) return s;
  std::string root;
  std::map<std::string, std::vector<std::string>> unit_args;  // front_end.unit_diagnostic[<unit>][<name>] = -Wno-error=<name>
  for (const kz_basis_item& b : items) {
    std::string name;
    basis.string(b.name, &name);
    if (name == "bundle.root") basis.string(b.value, &root);
    static const std::string kDiag = "front_end.unit_diagnostic[";
    if (name.rfind(kDiag, 0) == 0) {
      const size_t end = name.find("][", kDiag.size());
      std::string value;
      basis.string(b.value, &value);
      if (end == std::string::npos || value.rfind("-Wno-error=", 0) != 0) return kz::Status::error(kz::Err::Invalid, "basis item " + name + " is not a unit diagnostic");
      unit_args[name.substr(kDiag.size(), end - kDiag.size())].push_back(value);
    }
  }
  if (bundle_root != nullptr) *bundle_root = root;
  for (const kz_tu& t : tus) {
    UnitCommand u;
    std::string source, object, flags, directory, compiler;
    basis.string(t.source, &source);
    basis.string(t.object, &object);
    basis.string(t.flags, &flags);
    basis.string(t.directory, &directory);
    basis.string(t.compiler, &compiler);
    u.directory = directory;
    u.source = abs_in(root, source);
    u.object = abs_in(root, object);
    u.command_line.push_back(compiler);
    for (const std::string& a : split_command(flags)) u.command_line.push_back(a);
    u.command_line.push_back("-o");
    u.command_line.push_back(u.object);
    u.command_line.push_back("-c");
    u.command_line.push_back(u.source);
    // the recorded command must be the one rebuilt
    kz::Sha256 h;
    h.update(compiler);
    h.update("\0", 1);
    for (const std::string& a : split_command(flags)) {
      h.update(a);
      h.update("\0", 1);
    }
    h.update(source);
    h.update("\0", 1);
    const kz::Digest d = h.finish();
    if (std::memcmp(d.data(), t.command_sha256, 32) != 0) return kz::Status::error(kz::Err::Invalid, "the command of unit " + source + " does not match its recorded hash");
    auto ua = unit_args.find(source);
    if (ua != unit_args.end()) {
      u.analysis_args = ua->second;
      unit_args.erase(ua);
    }
    units->push_back(u);
  }
  if (!unit_args.empty()) return kz::Status::error(kz::Err::Invalid, "basis item front_end.unit_diagnostic names " + unit_args.begin()->first + ", which is no unit of the basis");
  return kz::Status::success();
}

std::string basis_items_text(const kz::Container& basis) {
  std::vector<kz_basis_item> items;
  std::ostringstream os;
  if (!basis.read_section(KZ_SEC_BASIS, &items).ok()) return "";
  const kz::EnumDesc* policy = kz::find_enum("KZ_POLICY");
  for (const kz_basis_item& b : items) {
    std::string name, value;
    basis.string(b.name, &name);
    basis.string(b.value, &value);
    const char* p = policy != nullptr ? kz::enum_display(*policy, b.policy) : nullptr;
    os << name << " = " << value << "  (" << (p != nullptr ? p : "?") << ")\n";
  }
  return os.str();
}

}  // namespace kzs

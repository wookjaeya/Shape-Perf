#include "kz/core/phi_provenance.hpp"

#include <algorithm>
#include <filesystem>
#include <map>
#include <set>

#include "kz/core/container.hpp"
#include "kz/core/phi.hpp"
#include "kz/core/registry.hpp"

namespace kz {
namespace phi {

namespace {

// Walks the lines [from, to] of `bytes`; `sink(begin, end)` receives each line without its LF.
// False if the range is empty or the file ends before `to`.
template <class Sink>
bool walk_lines(const std::string& bytes, uint32_t from, uint32_t to, Sink sink) {
  if (from < 1 || to < from) return false;
  uint32_t line = 1;
  size_t i = 0;
  bool any = false;
  while (i < bytes.size() && line <= to) {
    const size_t j = bytes.find('\n', i);
    const bool has_eol = j != std::string::npos;
    const size_t end = has_eol ? j : bytes.size();
    if (line >= from) {
      sink(bytes.data() + i, end - i);
      any = true;
    }
    i = has_eol ? j + 1 : bytes.size();
    ++line;
  }
  return any && line > to;  // false if the file ended before line `to`
}

}  // namespace

bool cited_text_hash(const std::string& bytes, uint32_t from, uint32_t to, Digest* out) {
  Sha256 h;
  const bool ok = walk_lines(bytes, from, to, [&](const char* b, size_t n) {
    h.update(b, n);
    h.update("\n", 1);
  });
  if (!ok) return false;
  *out = h.finish();
  return true;
}

bool cited_text(const std::string& bytes, uint32_t from, uint32_t to, std::string* out) {
  std::string text;
  const bool ok = walk_lines(bytes, from, to, [&](const char* b, size_t n) {
    text.append(b, n);
    text += '\n';
  });
  if (!ok) return false;
  *out = std::move(text);
  return true;
}

bool text_has_identifier(const std::string& text, const std::string& id) {
  if (id.empty()) return false;
  auto is_word = [](char c) { return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_'; };
  size_t pos = 0;
  while ((pos = text.find(id, pos)) != std::string::npos) {
    const bool left = pos == 0 || !is_word(text[pos - 1]);
    const size_t after = pos + id.size();
    const bool right = after >= text.size() || !is_word(text[after]);
    if (left && right) return true;
    ++pos;
  }
  return false;
}

int32_t resolve_source(const Profile& p, const std::string& path) {
  int32_t best = -1;
  size_t best_len = 0;
  for (size_t i = 0; i < p.sources.size(); ++i) {
    const std::string& r = p.sources[i].root;
    const bool match = r.empty() || path == r || (path.size() > r.size() && path.compare(0, r.size(), r) == 0 && path[r.size()] == '/');
    if (!match) continue;
    if (best < 0 || r.size() > best_len) {
      best = static_cast<int32_t>(i);
      best_len = r.size();
    }
  }
  return best;
}

bool tier_kind_compatible(const std::string& tier, const std::string& kind) {
  if (tier == "contract-backed") return kind == "api_doc";
  if (tier == "requirement-derived") return kind == "requirement";
  if (tier == "admitted") return kind == "code_admission";
  if (tier == "issue-derived") return kind == "issue";
  if (tier == "code-derived") return kind == "code_pattern";
  if (tier == "researcher") return kind == "researcher";
  if (tier == "unreferenced") return kind == "none";
  return false;
}

int tier_rank(const std::string& tier) {
  if (tier == "contract-backed") return 0;
  if (tier == "requirement-derived") return 1;
  if (tier == "admitted") return 2;
  if (tier == "issue-derived") return 3;
  if (tier == "code-derived") return 4;
  if (tier == "researcher") return 5;
  if (tier == "unreferenced") return 6;
  return -1;
}

int tier_index(const std::string& tier) {
  const EnumDesc* en = find_enum("KZ_TIER");
  if (en == nullptr) return -1;
  for (size_t i = 0; i < en->nitems; ++i)
    if (tier == en->items[i].display) return static_cast<int>(en->items[i].value);
  return -1;
}

namespace {

// Reads files under the bundle root once.
class Bundle {
 public:
  explicit Bundle(const std::string& root) : root_(root) {
    while (root_.size() > 1 && root_.back() == '/') root_.pop_back();
  }
  // The bytes of `file`; null if it cannot be read.
  const std::string* get(const std::string& file) {
    const auto it = cache_.find(file);
    if (it != cache_.end()) return it->second.ok ? &it->second.bytes : nullptr;
    Bytes b;
    Entry e;
    e.ok = read_whole_file(root_ + "/" + file, &b).ok();
    if (e.ok) e.bytes.assign(b.begin(), b.end());
    return cache_.emplace(file, std::move(e)).first->second.ok ? &cache_[file].bytes : nullptr;
  }
  const std::string& root() const { return root_; }

 private:
  struct Entry {
    bool ok = false;
    std::string bytes;
  };
  std::string root_;
  std::map<std::string, Entry> cache_;
};

Status check_root(const std::string& bundle_root) {
  if (bundle_root.empty()) return Status::error(Err::Invalid, "no bundle root given");
  std::error_code ec;
  if (!std::filesystem::is_directory(bundle_root, ec))
    return Status::error(Err::Io, "the bundle root '" + bundle_root + "' is not a readable directory");
  return Status::success();
}

}  // namespace

Status verify_provenance(const Profile& p, const std::string& bundle_root, std::vector<ProvProblem>* problems) {
  problems->clear();
  const Status root_ok = check_root(bundle_root);
  if (!root_ok.ok()) return root_ok;
  Bundle bundle(bundle_root);
  for (const ProvDecl& d : p.provs) {
    if (d.file.empty()) continue;
    const std::string* bytes = bundle.get(d.file);
    if (bytes == nullptr) {
      problems->push_back(ProvProblem{d.name, "cannot read " + bundle.root() + "/" + d.file});
      continue;
    }
    if (d.line_from == 0) {
      problems->push_back(ProvProblem{d.name, "the record names a file but no lines"});
      continue;
    }
    Digest h;
    if (!cited_text_hash(*bytes, d.line_from, d.line_to, &h)) {
      problems->push_back(ProvProblem{d.name, "lines " + std::to_string(d.line_from) + "-" + std::to_string(d.line_to) + " are outside " + d.file});
      continue;
    }
    if (to_hex(h) != d.sha256)
      problems->push_back(ProvProblem{d.name, "the text of " + d.file + ":" + std::to_string(d.line_from) + "-" + std::to_string(d.line_to) +
                                                  " has sha256 " + to_hex(h) + ", the profile says " + d.sha256});
  }
  std::sort(problems->begin(), problems->end(), [](const ProvProblem& a, const ProvProblem& b) {
    return a.prov != b.prov ? a.prov < b.prov : a.message < b.message;
  });
  return Status::success();
}

// ---- identifiers named by entries ----------------------------------------------------------------

namespace {

void add_all(std::vector<int32_t>* dst, const std::vector<int32_t>& src) {
  for (int32_t i : src)
    if (std::find(dst->begin(), dst->end(), i) == dst->end()) dst->push_back(i);
}

struct Claims {
  std::vector<SymbolClaim>* out;
  void add(const std::string& entry, const char* role, const std::string& id, const std::vector<int32_t>& prov) {
    if (id.empty()) return;
    out->push_back(SymbolClaim{entry, role, id, prov});
  }
  // The identifiers a pattern names.  `fn(F)` is the enclosing function and is not claimed.  A
  // comparison `arg[i] != C` is a discriminator: the matched call does not spell C, so it is not
  // claimed.  `memset(T)` names the type of the cleared object, which the matched text may spell
  // through the variable instead (`sizeof(*p)`), so only the call `memset` is claimed.
  void pattern(const Pattern& pat, const std::string& entry, const std::vector<int32_t>& prov) {
    if (!pat.present) return;
    switch (pat.op) {
      case PatOp::Call:
        add(entry, "callee", pat.callee, prov);
        for (const ArgPred& a : pat.args) {
          if (a.cmp == ArgCmp::Eq && a.is_name) add(entry, "argument constant", a.name, prov);
          if (a.cmp == ArgCmp::AddrOf) add(entry, "address-of field", a.name.substr(a.name.find('.') == std::string::npos ? 0 : a.name.find('.') + 1), prov);
        }
        break;
      case PatOp::Store:
      case PatOp::Load: add(entry, "field", pat.field, prov); break;
      case PatOp::Memset: add(entry, "callee", "memset", prov); break;
      case PatOp::LoopPoll:
      case PatOp::Return: break;
    }
  }
};

}  // namespace

std::vector<SymbolClaim> symbol_claims(const Profile& p) {
  std::vector<SymbolClaim> out;
  Claims c{&out};
  for (const ConstDecl& d : p.consts)
    if (d.value == ConstDecl::Value::RefEval) c.add("const " + d.name, "ref eval symbol", d.ref_symbol, d.prov_idx);
  for (const LocDecl& d : p.locs) {
    c.add("loc " + d.name, "struct", d.struct_name, d.prov_idx);
    c.add("loc " + d.name, "field", d.field, d.prov_idx);
  }
  for (const BindDecl& d : p.binds) {
    c.add("bind " + d.name, "struct", d.struct_name, d.prov_idx);
    c.add("bind " + d.name, "field", d.field, d.prov_idx);
  }
  for (const SiteDecl& d : p.sites) c.pattern(d.pattern, "site " + d.name, d.prov_idx);
  for (const OpDecl& op : p.ops) {
    // The operation is its function: its symbol must show in a text the operation (or one of its
    // parts) cites.
    std::vector<int32_t> whole = op.prov_idx;
    for (const ConjunctDecl& cj : op.conjuncts) add_all(&whole, cj.prov_idx);
    for (const EffectDecl& e : op.effects) add_all(&whole, e.prov_idx);
    for (const OutcomeDecl& oc : op.outcomes) add_all(&whole, oc.doc_prov_idx);
    c.add("op " + op.name, "bind symbol", op.symbol, whole);
    c.pattern(op.terminal.after, "op " + op.name + " terminal", op.prov_idx);
    for (const ConjunctDecl& cj : op.conjuncts)
      if (cj.lp.kind == LpKind::At) c.pattern(cj.lp.pattern, "conjunct " + op.name + "." + cj.label, cj.prov_idx.empty() ? op.prov_idx : cj.prov_idx);
    for (const EffectDecl& e : op.effects)
      if (e.lp.kind == LpKind::At) c.pattern(e.lp.pattern, "effect " + op.name + "." + e.label, e.prov_idx.empty() ? op.prov_idx : e.prov_idx);
  }
  for (const HbRuleDecl& r : p.rules) {
    if (r.tmpl != RuleTemplate::Observe) continue;
    c.add("hb " + r.name, "poll function", r.poll_fn, r.prov_idx);
    c.pattern(r.called_at, "hb " + r.name, r.prov_idx);
    c.add("hb " + r.name, "else-terminal function", r.else_terminal, r.prov_idx);
  }
  return out;
}

Status verify_symbols(const Profile& p, const std::string& bundle_root, std::vector<SymbolProblem>* problems, SymbolReport* report) {
  problems->clear();
  *report = SymbolReport();
  const Status root_ok = check_root(bundle_root);
  if (!root_ok.ok()) return root_ok;
  Bundle bundle(bundle_root);
  std::map<int32_t, std::string> texts;       // record -> its cited text
  std::map<int32_t, std::string> unreadable;  // record -> why its text cannot be had
  auto text_of = [&](int32_t pi) -> const std::string* {
    const auto it = texts.find(pi);
    if (it != texts.end()) return &it->second;
    if (unreadable.count(pi) != 0) return nullptr;
    const ProvDecl& d = p.provs[static_cast<size_t>(pi)];
    const std::string* bytes = bundle.get(d.file);
    if (bytes == nullptr) {
      unreadable[pi] = "cannot read " + bundle.root() + "/" + d.file;
      return nullptr;
    }
    std::string t;
    if (!cited_text(*bytes, d.line_from, d.line_to, &t)) {
      unreadable[pi] = "lines " + std::to_string(d.line_from) + "-" + std::to_string(d.line_to) + " are outside " + d.file;
      return nullptr;
    }
    return &(texts[pi] = std::move(t));
  };
  const std::vector<SymbolClaim> claims = symbol_claims(p);
  // record -> (has a claim-bearing entry cited it, did any claim of such an entry occur in it)
  std::map<int32_t, std::pair<bool, bool>> relevance;
  for (const SymbolClaim& cl : claims) {
    for (int32_t pi : cl.prov) {
      if (p.provs[static_cast<size_t>(pi)].file.empty()) continue;
      const std::string* t = text_of(pi);
      std::pair<bool, bool>& rel = relevance[pi];
      rel.first = true;
      if (t != nullptr && text_has_identifier(*t, cl.identifier)) rel.second = true;
    }
  }
  for (const auto& kv : relevance) {
    if (kv.second.second || unreadable.count(kv.first) != 0) continue;
    const ProvDecl& d = p.provs[static_cast<size_t>(kv.first)];
    report->idle_records.push_back(d.name + " (" + d.file + ":" + std::to_string(d.line_from) + "-" + std::to_string(d.line_to) + ")");
  }
  std::sort(report->idle_records.begin(), report->idle_records.end());
  for (const SymbolClaim& cl : claims) {
    ++report->claims;
    bool has_text_record = false, found = false;
    std::string why;
    for (int32_t pi : cl.prov) {
      const ProvDecl& d = p.provs[static_cast<size_t>(pi)];
      if (d.file.empty()) continue;  // a hand-entered record: no text
      has_text_record = true;
      const std::string* t = text_of(pi);
      if (t == nullptr) {
        why = unreadable[pi];
        continue;
      }
      if (text_has_identifier(*t, cl.identifier)) {
        found = true;
        break;
      }
    }
    if (!has_text_record) {
      ++report->unverifiable;
      report->unverifiable_claims.push_back(cl.entry + ": " + cl.role + " '" + cl.identifier + "'");
      continue;
    }
    ++report->checked;
    if (found) continue;
    std::string cited;
    for (int32_t pi : cl.prov) {
      const ProvDecl& d = p.provs[static_cast<size_t>(pi)];
      if (d.file.empty()) continue;
      if (!cited.empty()) cited += ", ";
      cited += d.name + " (" + d.file + ":" + std::to_string(d.line_from) + "-" + std::to_string(d.line_to) + ")";
    }
    problems->push_back(SymbolProblem{cl.entry, cl.role, cl.identifier,
                                      "the " + cl.role + " '" + cl.identifier + "' occurs in none of the cited texts: " + cited + (why.empty() ? "" : "; " + why)});
  }
  std::sort(problems->begin(), problems->end(), [](const SymbolProblem& a, const SymbolProblem& b) {
    if (a.entry != b.entry) return a.entry < b.entry;
    if (a.role != b.role) return a.role < b.role;
    return a.identifier < b.identifier;
  });
  return Status::success();
}

}  // namespace phi
}  // namespace kz

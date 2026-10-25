// phi_print.cpp -- the canonical text of a profile.
//
// The canonical text is itself a valid profile (parse(canonical_text(p)) prints the same text
// again, which the tests check) and is what the profile hash covers.  Declarations come in a
// fixed order of categories and, inside a category, in the byte order of their names; every
// set-like list is sorted and free of duplicates; nothing of the layout of the source (comments,
// white space, file split, declaration order, spelling of integers) survives.  The order of
// the states of a kind and of the tag values is kept: it is part of the content.

#include <algorithm>
#include <cstdio>

#include "kz/core/phi.hpp"

namespace kz {
namespace phi {

namespace {

struct Out {
  std::string s;
  void line(int indent, const std::string& text) {
    s.append(static_cast<size_t>(indent) * 2, ' ');
    s += text;
    s.push_back('\n');
  }
};

std::string int_text(int64_t v) {
  if (v > 1000000) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "0x%llx", static_cast<unsigned long long>(v));
    return buf;
  }
  return std::to_string(v);
}

std::string join(const std::vector<std::string>& v, const char* sep) {
  std::string out;
  for (size_t i = 0; i < v.size(); ++i) {
    if (i != 0) out += sep;
    out += v[i];
  }
  return out;
}

std::string sorted_join(std::vector<std::string> v, const char* sep) {
  std::sort(v.begin(), v.end());
  v.erase(std::unique(v.begin(), v.end()), v.end());
  return join(v, sep);
}

std::string sort_text(const SortSpec& s) {
  switch (s.kind) {
    case SortKind::Bool: return "Bool";
    case SortKind::Int: return "Int";
    case SortKind::Str: return "Str";
    case SortKind::Actor: return "Actor";
    case SortKind::Loc: return "Loc";
    case SortKind::Id: return "Id<" + s.ref + ">";
    case SortKind::Res: return "Res<" + s.ref + ">";
    default: return "?";
  }
}

std::string guard_text(const Guard& g) { return "guard " + g.component + " " + cmp_text(g.op) + " " + quote_string(g.version); }

// " prov A, B guard ... note ..." for the compact declarations.
std::string clauses_inline(const EntryBase& e) {
  std::string out;
  if (!e.prov.empty()) out += " prov " + sorted_join(e.prov, ", ");
  for (const Guard& g : e.guards) out += " " + guard_text(g);
  if (!e.note.empty()) out += " note " + quote_string(e.note);
  return out;
}

// The same as separate items of a block.
void clauses_block(Out* o, int indent, const EntryBase& e) {
  for (const Guard& g : e.guards) o->line(indent, guard_text(g) + ";");
  if (!e.note.empty()) o->line(indent, "note " + quote_string(e.note) + ";");
  if (!e.prov.empty()) o->line(indent, "prov " + sorted_join(e.prov, ", ") + ";");
}

std::string lp_text(const LpSpec& lp) {
  std::string out;
  switch (lp.kind) {
    case LpKind::Capture: out = "lp capture"; break;
    case LpKind::At: out = "lp at " + pattern_text(lp.pattern); break;
    case LpKind::Site: out = "lp site " + lp.site; break;
    case LpKind::None: out = "lp ?"; break;
  }
  if (!lp.values.empty()) {
    out += " values ";
    for (size_t i = 0; i < lp.values.size(); ++i) {
      if (i != 0) out += ", ";
      out += lp.values[i].name + ": " + sort_text(lp.values[i].sort);
    }
  }
  return out;
}

std::string effect_body(const EffectDecl& e) {
  switch (e.kind) {
    case EffectKind::Set:
      return "lifecycle " + (e.res ? expr_text(*e.res) : "all " + e.all_kind) + " -> " + e.to_state + (e.with_id ? " id " + expr_text(*e.with_id) : std::string());
    case EffectKind::Raise: return "lifecycle " + expr_text(*e.res) + " raise " + expr_text(*e.value);
    case EffectKind::Use: return "use " + expr_text(*e.res);
    case EffectKind::StoreOut: return "store *" + e.target + " := " + expr_text(*e.value);
    case EffectKind::StoreLoc: return "store " + e.target + " := " + expr_text(*e.value);
    case EffectKind::SetTag: return "settag " + e.target + " of " + expr_text(*e.res) + " := " + e.tag_value;
    case EffectKind::Mark: return "mark " + e.target;
    case EffectKind::Sync: return "sync " + e.target + " " + quote_string(e.sync_object);
    case EffectKind::Env: return "env " + quote_string(e.target);
  }
  return "?";
}

std::string allowed_entry_text(const AllowedEntry& a) {
  std::string out = allowed_text(a.kind);
  switch (a.kind) {
    case AllowedKind::DocumentedDrop:
    case AllowedKind::DeferByDesign: out += " " + quote_string(a.src); break;
    case AllowedKind::ReturnedAndTested:
      out += " codes {" + sorted_join(a.codes, ", ") + "}";
      if (!a.continuation.empty()) out += " continuation " + quote_string(a.continuation);
      break;
    case AllowedKind::RetryUntilReady: out += " codes {" + sorted_join(a.codes, ", ") + "} bound " + a.bound; break;
    case AllowedKind::Fallback: out += " " + a.op; break;
    case AllowedKind::Exit: out += " " + a.exit_code + " " + quote_string(a.src); break;
    default: break;
  }
  if (a.conflicts_appreq) out += " conflicts_appreq";
  if (a.conflicts_caller) out += " conflicts_caller";
  if (!a.prov.empty()) out += " prov " + sorted_join(a.prov, ", ");
  return out;
}

void print_prov(Out* o, const ProvDecl& d) {
  o->line(0, "prov " + d.name + " {");
  o->line(1, "tier " + d.tier + ";");
  o->line(1, "kind " + d.kind + ";");
  if (!d.side.empty()) o->line(1, "side " + d.side + ";");
  if (!d.file.empty()) o->line(1, "file " + quote_string(d.file) + ";");
  if (d.line_from != 0) o->line(1, "lines " + std::to_string(d.line_from) + " " + std::to_string(d.line_to) + ";");
  if (!d.sha256.empty()) o->line(1, "sha256 " + quote_string(d.sha256) + ";");
  if (!d.cite.empty()) o->line(1, "cite " + quote_string(d.cite) + ";");
  if (!d.authored.empty()) o->line(1, "authored " + quote_string(d.authored) + ";");
  if (!d.note.empty()) o->line(1, "note " + quote_string(d.note) + ";");
  o->line(0, "}");
}

void print_kind(Out* o, const KindDecl& k) {
  std::string head = "kind " + k.name + "(";
  for (size_t i = 0; i < k.params.size(); ++i) {
    if (i != 0) head += ", ";
    head += k.params[i].name + ": " + sort_text(k.params[i].sort);
  }
  o->line(0, head + ") {");
  o->line(1, "states " + join(k.states, " < ") + ";");
  o->line(1, "initial " + k.initial + ";");
  if (!k.scope.empty()) o->line(1, "scope " + k.scope + ";");
  if (!k.table.empty()) o->line(1, "table " + k.table + ";");
  if (k.ids.present) {
    o->line(1, "ids {");
    o->line(2, "undefined " + k.ids.undefined_c + ";");
    o->line(2, "reserved " + k.ids.reserved_c + ";");
    o->line(2, "base " + k.ids.base_c + ";");
    o->line(2, "serial_max " + k.ids.serial_max_c + ";");
    o->line(1, "}");
  }
  for (const TagDecl& t : k.tags) o->line(1, "tag " + t.name + " { " + join(t.values, ", ") + " };");
  for (const TransitionDecl& t : k.transitions) o->line(1, "transition " + t.from + " -> " + t.to + " on " + t.action + ";");
  for (const DerivedDecl& d : k.derived) o->line(1, "derived " + d.state + " when " + expr_text(*d.when) + ";");
  clauses_block(o, 1, k);
  o->line(0, "}");
}

void print_op(Out* o, const OpDecl& d) {
  std::string head = "op " + d.name + "(";
  for (size_t i = 0; i < d.params.size(); ++i) {
    if (i != 0) head += ", ";
    if (!d.params[i].role.empty()) head += d.params[i].role + " ";
    head += d.params[i].name;
    if (d.params[i].has_sort) head += ": " + sort_text(d.params[i].sort);
  }
  o->line(0, head + ") {");
  if (!d.symbol.empty()) o->line(1, "bind symbol " + quote_string(d.symbol) + ";");
  for (const ConjunctDecl& c : d.conjuncts) {
    std::string t = "conjunct " + c.label + ": " + expr_text(*c.expr) + " " + lp_text(c.lp);
    if (!c.prov.empty()) t += " prov " + sorted_join(c.prov, ", ");
    o->line(1, t + ";");
  }
  for (const EffectDecl& e : d.effects) {
    std::string t = "effect " + e.label + ": " + effect_body(e);
    if (e.when) t += std::string(e.filter ? " filter " : " when ") + expr_text(*e.when);
    t += " " + lp_text(e.lp);
    if (!e.status.empty()) t += " status " + e.status;
    if (!e.refutable_by.empty()) t += " refutable_by " + sorted_join(e.refutable_by, ", ");
    if (!e.prov.empty()) t += " prov " + sorted_join(e.prov, ", ");
    for (const Guard& g : e.guards) t += " " + guard_text(g);
    o->line(1, t + ";");
  }
  for (const OutcomeDecl& oc : d.outcomes) {
    std::string t = "outcome " + oc.code + ": " + outcome_class_text(oc.cls);
    if (oc.cls == OutcomeClass::NotReady) t += "(" + oc.conjunct + ")";
    if (!oc.documented.empty()) {
      t += " documented " + quote_string(oc.documented);
      if (!oc.doc_prov.empty()) t += " prov " + sorted_join(oc.doc_prov, ", ");
    }
    if (!oc.narrow.present) {
      o->line(1, t + ";");
      continue;
    }
    const NarrowDecl& n = oc.narrow;
    o->line(1, t + " narrow {");
    o->line(2, "api " + quote_string(n.api) + ";");
    std::string revs;
    for (const std::string& r : n.revs) revs += (revs.empty() ? "" : ", ") + quote_string(r);
    o->line(2, "source " + n.source + " rev " + revs + ";");
    o->line(2, "branch " + quote_string(n.branch) + ";");
    o->line(2, "context " + (n.context_any ? std::string("any") : sorted_join(n.contexts, ", ")) + ";");
    for (const NarrowCaptured& c : n.captured)
      o->line(2, "captured " + c.param + (c.text ? " text" : "") + (c.length_below.empty() ? std::string() : " length < " + c.length_below) + ";");
    if (n.requires_) o->line(2, "requires " + expr_text(*n.requires_) + ";");
    o->line(2, "holds_at " + n.holds_at + ";");
    if (!n.prov.empty()) o->line(2, "prov " + sorted_join(n.prov, ", ") + ";");
    if (!n.note.empty()) o->line(2, "note " + quote_string(n.note) + ";");
    o->line(1, "};");
  }
  if (d.terminal.present) {
    std::string t = "terminal";
    if (d.terminal.process_terminal) t += " process_terminal";
    if (d.terminal.after.present) t += " after " + pattern_text(d.terminal.after);
    o->line(1, t + ";");
  }
  clauses_block(o, 1, d);
  o->line(0, "}");
}

void print_rule(Out* o, const HbRuleDecl& r) {
  o->line(0, "hb " + r.name + " {");
  if (!r.title.empty()) o->line(1, "title " + quote_string(r.title) + ";");
  o->line(1, std::string("stratum ") + stratum_text(r.stratum) + ";");
  o->line(1, std::string("robust ") + robust_text(r.robust) + ";");
  o->line(1, std::string("template ") + template_text(r.tmpl) + " {");
  switch (r.tmpl) {
    case RuleTemplate::ProgramOrder: o->line(2, std::string("looper ") + (r.looper ? "yes" : "no") + ";"); break;
    case RuleTemplate::Spawn:
      o->line(2, "create " + r.create_op + " out " + r.create_out + ";");
      o->line(2, "entry " + r.entry_op + ";");
      break;
    case RuleTemplate::Phase: o->line(2, "from " + r.from_op + " phase " + r.from_phase + ";"); break;
    case RuleTemplate::Observe:
      o->line(2, "poll " + quote_string(r.poll_fn) + " called_at " + pattern_text(r.called_at) + ";");
      o->line(2, "exit_outcome " + r.exit_outcome + ";");
      o->line(2, r.predicate_from_code || !r.predicate ? std::string("predicate from_code;") : "predicate " + expr_text(*r.predicate) + ";");
      if (!r.else_terminal.empty()) o->line(2, "else_terminal " + quote_string(r.else_terminal) + ";");
      if (!r.watch_kinds.empty()) o->line(2, "watch " + sorted_join(r.watch_kinds, ", ") + ";");
      break;
    case RuleTemplate::ReadsFrom:
      o->line(2, "use " + r.use_op + "." + r.use_conjunct + ";");
      o->line(2, "outcome " + r.use_outcome + ";");
      o->line(2, "publish " + r.pub_op + "." + r.pub_effect + ";");
      if (!r.match.empty()) o->line(2, "match " + r.match + ";");
      break;
  }
  o->line(1, "}");
  clauses_block(o, 1, r);
  o->line(0, "}");
}

void print_rspec(Out* o, const RspecDecl& r) {
  o->line(0, "rspec " + r.name + " {");
  o->line(1, "use op " + r.use_op + ";");
  if (r.key) o->line(1, "key " + expr_text(*r.key) + ";");
  if (r.requires_) o->line(1, "requires " + expr_text(*r.requires_) + ";");
  if (!r.oracle_fns.empty()) {
    std::vector<std::string> q;
    for (const std::string& f : r.oracle_fns) q.push_back(quote_string(f));
    std::sort(q.begin(), q.end());
    q.erase(std::unique(q.begin(), q.end()), q.end());
    o->line(1, "oracle fns " + join(q, ", ") + ";");
  }
  o->line(1, "appreq " + (r.appreq_unspecified ? std::string("unspecified_by_reference") : r.appreq) + ";");
  if (r.allowed.empty()) {
    o->line(1, std::string("allowed ") + allowed_text(r.allowed_head.kind) + ";");
  } else {
    o->line(1, "allowed {");
    std::vector<std::string> es;
    for (const AllowedEntry& a : r.allowed) es.push_back(allowed_entry_text(a) + ";");
    std::sort(es.begin(), es.end());
    for (const std::string& e : es) o->line(2, e);
    o->line(1, "}");
  }
  if (!r.harm.empty()) o->line(1, "harm " + r.harm + ";");
  if (!r.authored.empty()) o->line(1, "authored " + quote_string(r.authored) + ";");
  clauses_block(o, 1, r);
  o->line(0, "}");
}

}  // namespace

std::string canonical_text(const Profile& p) {
  Out o;
  o.line(0, "phi " + std::to_string(kLanguageVersion) + ";");
  if (p.has_header) {
    std::string t = "profile " + quote_string(p.name);
    if (!p.revision.empty()) t += " revision " + quote_string(p.revision);
    o.line(0, t + ";");
  }
  for (const ProvDecl& d : p.provs) print_prov(&o, d);
  for (const SourceDecl& d : p.sources)
    o.line(0, "source " + d.name + " root " + quote_string(d.root) + " rev " + quote_string(d.rev) + clauses_inline(d) + ";");
  for (const ConstDecl& d : p.consts) {
    std::string v;
    switch (d.value) {
      case ConstDecl::Value::Int: v = int_text(d.ival); break;
      case ConstDecl::Value::Str: v = quote_string(d.sval); break;
      case ConstDecl::Value::Bool: v = d.bval ? "true" : "false"; break;
      case ConstDecl::Value::RefEval: v = "ref eval " + quote_string(d.ref_symbol) + " in " + quote_string(d.ref_file); break;
    }
    o.line(0, "const " + d.name + " = " + v + clauses_inline(d) + ";");
  }
  if (p.context.present) {
    o.line(0, "context {");
    o.line(1, "owner_kind " + p.context.owner_kind + ";");
    if (p.context.initial_thread_not_app) o.line(1, "initial_thread not_app prov " + sorted_join(p.context.not_app_prov, ", ") + ";");
    if (!p.context.prov.empty()) o.line(1, "prov " + sorted_join(p.context.prov, ", ") + ";");
    o.line(0, "}");
  }
  if (p.scope.present) {
    o.line(0, "scope {");
    o.line(1, "kind " + p.scope.kind + ";");
    o.line(1, "request " + p.scope.request + ";");
    o.line(1, "teardown " + p.scope.teardown + ";");
    if (!p.scope.prov.empty()) o.line(1, "prov " + sorted_join(p.scope.prov, ", ") + ";");
    o.line(0, "}");
  }
  for (const KindDecl& k : p.kinds) print_kind(&o, k);
  for (const LocDecl& d : p.locs) {
    o.line(0, "loc " + d.name + " {");
    o.line(1, "struct " + quote_string(d.struct_name) + ";");
    o.line(1, "field " + quote_string(d.field) + ";");
    o.line(1, "holds " + sort_text(d.holds) + ";");
    clauses_block(&o, 1, d);
    o.line(0, "}");
  }
  for (const BindDecl& d : p.binds) {
    std::string via;
    if (!d.via.empty()) {
      std::vector<std::string> v = d.via;  // a set of functions: the normal form sorts it
      std::sort(v.begin(), v.end());
      v.erase(std::unique(v.begin(), v.end()), v.end());
      for (const std::string& f : v) via += (via.empty() ? " via " : ", ") + quote_string(f);
    }
    o.line(0, "bind field " + quote_string(d.struct_name) + " " + quote_string(d.field) + " as " + expr_text(*d.target) + via + clauses_inline(d) + ";");
  }
  for (const SiteDecl& d : p.sites) o.line(0, "site " + d.name + " = " + pattern_text(d.pattern) + clauses_inline(d) + ";");
  for (const RoleDecl& d : p.roles) {
    o.line(0, "role " + d.name + " {");
    o.line(1, "op " + d.op + ";");
    o.line(1, "site " + d.site + ";");
    o.line(1, "purpose " + d.purpose + ";");
    clauses_block(&o, 1, d);
    o.line(0, "}");
  }
  for (const OpDecl& d : p.ops) print_op(&o, d);
  for (const InitDecl& d : p.inits) {
    if (d.is_loc)
      o.line(0, "init " + d.name + ": loc " + d.loc_name + " = " + expr_text(*d.value) + clauses_inline(d) + ";");
    else
      o.line(0, "init " + d.name + ": state " + expr_text(*d.res) + " = " + d.state + clauses_inline(d) + ";");
  }
  for (const HbRuleDecl& r : p.rules) print_rule(&o, r);
  for (const RspecDecl& r : p.rspecs) print_rspec(&o, r);
  for (const TrustedDecl& d : p.trusted) {
    o.line(0, "trusted " + d.name + " {");
    o.line(1, "class " + d.cls + ";");
    o.line(1, "statement " + quote_string(d.statement) + ";");
    if (!d.refutable_by.empty()) o.line(1, "refutable_by " + sorted_join(d.refutable_by, ", ") + ";");
    clauses_block(&o, 1, d);
    o.line(0, "}");
  }
  return o.s;
}

}  // namespace phi
}  // namespace kz

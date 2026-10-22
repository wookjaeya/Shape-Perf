// spec.cpp -- see spec.hpp.
#include "spec.hpp"

#include <algorithm>

namespace kzs {

bool parse_field_ref(const std::string& text, FieldRef* out) {
  const size_t dot = text.find('.');
  if (dot == std::string::npos || dot == 0 || dot + 1 >= text.size()) return false;
  if (text.find('.', dot + 1) != std::string::npos) return false;
  out->type = text.substr(0, dot);
  out->field = text.substr(dot + 1);
  return true;
}

bool ExtractSpec::has_field(const FieldRef& f) const {
  for (const TargetField& t : fields)
    if (t.field == f) return true;
  return false;
}

std::set<std::string> ExtractSpec::field_types() const {
  std::set<std::string> out;
  for (const TargetField& t : fields) out.insert(t.field.type);
  return out;
}

namespace {

void add_field(ExtractSpec* spec, const std::string& type, const std::string& field, const std::string& origin) {
  if (type.empty() || field.empty()) return;
  FieldRef f{type, field};
  if (spec->has_field(f)) return;
  spec->fields.push_back(TargetField{f, origin});
}

uint8_t role_of(const std::string& r) {
  if (r == "stamp") return KZ_ARG_ROLE_STAMP;
  if (r == "key") return KZ_ARG_ROLE_KEY;
  if (r == "id") return KZ_ARG_ROLE_ID;
  return KZ_ARG_ROLE_NONE;
}

void scan_pattern(ExtractSpec* spec, const kz::phi::Pattern& p, const std::string& origin) {
  if (!p.present) return;
  if (p.op == kz::phi::PatOp::Store) add_field(spec, p.type_name, p.field, origin + " (store pattern)");
}

}  // namespace

void spec_from_profile(const kz::phi::Profile& profile, ExtractSpec* spec) {
  spec->profile_name = profile.name;
  spec->profile_revision = profile.revision;
  for (const kz::phi::LocDecl& l : profile.locs) add_field(spec, l.struct_name, l.field, "profile loc " + l.name);
  for (const kz::phi::BindDecl& b : profile.binds) add_field(spec, b.struct_name, b.field, "profile bind " + b.name);
  for (const kz::phi::SiteDecl& s : profile.sites) scan_pattern(spec, s.pattern, "profile site " + s.name);
  for (const kz::phi::OpDecl& op : profile.ops) {
    OpInfo info;
    info.symbol = op.symbol.empty() ? op.name : op.symbol;
    info.origin = "profile op " + op.name;
    for (const kz::phi::OpParam& p : op.params) {
      info.roles.push_back(role_of(p.role));
      info.out.push_back(p.role == "out");
    }
    for (const kz::phi::ConjunctDecl& c : op.conjuncts) scan_pattern(spec, c.lp.pattern, info.origin);
    for (const kz::phi::EffectDecl& e : op.effects) scan_pattern(spec, e.lp.pattern, info.origin);
    if (op.terminal.present) {
      info.terminal = true;
      info.process_terminal = op.terminal.process_terminal;
      spec->terminal_ops.insert(info.symbol);
    }
    spec->ops[info.symbol] = info;
  }
  for (const kz::phi::HbRuleDecl& r : profile.rules) {
    if (r.tmpl == kz::phi::RuleTemplate::Spawn && !r.create_op.empty()) spec->spawn_ops.insert(r.create_op);
    if (r.tmpl == kz::phi::RuleTemplate::Observe && !r.poll_fn.empty()) spec->poll_fns.insert(r.poll_fn);
  }
}

void spec_add_builtin_data_fns(ExtractSpec* spec) {
  // Summaries of the C library functions that write through a pointer argument.  They are
  // hand-entered (SPEC 4.2 step 2: "external functions with summaries"), not derived from the
  // program under analysis; the report counts them.
  struct Row {
    const char* name;
    int dest, src, fill;
    bool zero;
    uint8_t kind;
  };
  static const Row rows[] = {
      {"memset", 0, -1, 1, false, KZ_WRITER_KIND_MEMSET},  {"bzero", 0, -1, -1, true, KZ_WRITER_KIND_MEMSET},
      {"memcpy", 0, 1, -1, false, KZ_WRITER_KIND_MEMCPY},  {"memmove", 0, 1, -1, false, KZ_WRITER_KIND_MEMCPY},
      {"strcpy", 0, 1, -1, false, KZ_WRITER_KIND_MEMCPY},  {"strncpy", 0, 1, -1, false, KZ_WRITER_KIND_MEMCPY},
      {"strcat", 0, 1, -1, false, KZ_WRITER_KIND_MEMCPY},  {"strncat", 0, 1, -1, false, KZ_WRITER_KIND_MEMCPY},
      {"sprintf", 0, -1, -1, false, KZ_WRITER_KIND_MEMCPY}, {"snprintf", 0, -1, -1, false, KZ_WRITER_KIND_MEMCPY},
      {"vsprintf", 0, -1, -1, false, KZ_WRITER_KIND_MEMCPY}, {"vsnprintf", 0, -1, -1, false, KZ_WRITER_KIND_MEMCPY},
  };
  for (const Row& r : rows) {
    if (spec->data_fns.count(r.name) != 0) continue;
    DataFn d;
    d.name = r.name;
    d.dest = r.dest;
    d.src = r.src;
    d.fill = r.fill;
    d.zero_fill = r.zero;
    d.writer_kind = r.kind;
    d.origin = "built-in table (hand-entered, tier researcher)";
    spec->data_fns[d.name] = d;
    spec->declared.push_back(DeclaredItem{"data_fn." + d.name, "dest=" + std::to_string(d.dest), d.origin});
  }
}

}  // namespace kzs

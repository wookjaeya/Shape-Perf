// target_rules.cpp -- see target_rules.hpp.
#include "target_rules.hpp"

#include <algorithm>
#include <cctype>
#include <functional>

namespace kzs {

std::string TargetRules::text() const {
  std::string t;
  if (t1) t += "R-T1";
  if (t2) t += std::string(t.empty() ? "" : ",") + "R-T2";
  return t;
}

bool parse_target_rules(const std::string& text, TargetRules* out, std::string* error) {
  TargetRules r;
  size_t start = 0;
  while (start <= text.size()) {
    const size_t comma = text.find(',', start);
    const std::string part = text.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
    if (part == "R-T1") r.t1 = true;
    else if (part == "R-T2") r.t2 = true;
    else {
      *error = "--targets-rule expects R-T1, R-T2 or R-T1,R-T2, got " + text;
      return false;
    }
    if (comma == std::string::npos) break;
    start = comma + 1;
  }
  *out = r;
  return true;
}

const char* target_rules_revision() { return "target rules revision 1"; }

namespace {

std::string base_name(const std::string& key) {
  const size_t at = key.find('@');
  return at == std::string::npos ? key : key.substr(0, at);
}

// The functions whose every return value is the address of a global object (or of a part of one) or a
// null pointer, and at least one is an address: a call result of one of them is a pointer whose source
// is global.  An address counts when the innermost location of its tree is a global or an element of one,
// or when the address as written starts with `&` and the name of a global object the reads name (a member
// of a global record: the tree names the member's owner type, not the object).  To a fixed point over
// the calls such a function returns (finite: a function joins the set at most once).
std::set<std::string> global_address_functions(const Model& m) {
  std::set<std::string> globals;
  for (const ReadFact& r : m.reads)
    if (r.root_kind == KZ_READ_ROOT_GLOBAL && !r.root.empty()) globals.insert(r.root);
  for (const ArgFlowFact& a : m.argflows)
    if (!a.object.empty()) globals.insert(base_name(a.object));
  auto innermost = [](ExprRef e) {
    while (e != nullptr && e->op != KZ_EXPR_OP_LOAD) e = e->a;
    return e;
  };
  auto written_global = [&](const std::string& text) {
    if (text.size() < 2 || text[0] != '&') return false;
    size_t i = 1;
    while (i < text.size() && (std::isalnum(static_cast<unsigned char>(text[i])) || text[i] == '_')) ++i;
    if (i < text.size() && text[i] != '.' && text[i] != '[') return false;
    return globals.count(text.substr(1, i - 1)) != 0;
  };
  std::map<std::string, std::vector<const ReturnFact*>> by_fn;
  for (const ReturnFact& r : m.returns) by_fn[base_name(r.func_key)].push_back(&r);
  std::set<std::string> out;
  for (bool changed = true; changed;) {
    changed = false;
    for (const auto& [fn, rows] : by_fn) {
      if (out.count(fn) != 0) continue;
      bool all = true, address = false;
      for (const ReturnFact* r : rows) {
        if (r->vkind == KZ_RET_VALUE_CONST && r->constant == 0) continue;
        if (r->vkind == KZ_RET_VALUE_ADDR) {
          const ExprRef in = innermost(r->value);
          if ((in != nullptr && (in->ref == KZ_EXPR_REF_GLOBAL || in->ref == KZ_EXPR_REF_ELEM)) || written_global(r->text)) {
            address = true;
            continue;
          }
        }
        if (r->vkind == KZ_RET_VALUE_CALL && r->value != nullptr && r->value->op == KZ_EXPR_OP_CALL_RESULT && out.count(r->value->name) != 0) {
          address = true;
          continue;
        }
        all = false;
        break;
      }
      if (all && address) {
        out.insert(fn);
        changed = true;
      }
    }
  }
  return out;
}

// The base of an argument's access path is shared: a global object, an element of a global array, the
// pointee of a global pointer, the pointee of a pointer field reached from a global (ARGFLOW names that
// global in `object`), or the pointee of a call result whose function returns global addresses only.
bool shared_base(const ArgFlowFact& a, const std::set<std::string>& global_address_fns) {
  switch (a.base) {
    case KZ_ARGBASE_GLOBAL:
    case KZ_ARGBASE_GLOBAL_ELEMENT:
    case KZ_ARGBASE_GLOBAL_POINTER:
      return true;
    case KZ_ARGBASE_FIELD_POINTER:
      return !a.object.empty();
    case KZ_ARGBASE_CALL_RESULT:
      return global_address_fns.count(a.root) != 0;
    default:
      return false;
  }
}

bool record_field(const std::string& type, const std::string& field) { return !type.empty() && !field.empty() && type != "<global>"; }

// The field loads of a tree, whatever its depth.  The pool shares structurally equal subtrees and builds a node
// after its children, so the nodes form an acyclic graph: each node is visited once, and the walk ends.
void field_loads(ExprRef root, std::set<std::string>* out) {
  std::set<ExprRef> seen;
  std::vector<ExprRef> stack{root};
  while (!stack.empty()) {
    const ExprRef e = stack.back();
    stack.pop_back();
    if (e == nullptr || !seen.insert(e).second) continue;
    if (e->op == KZ_EXPR_OP_LOAD && e->ref == KZ_EXPR_REF_FIELD) out->insert(e->name);
    stack.push_back(e->a);
    stack.push_back(e->b);
  }
}

}  // namespace

std::vector<TargetField> rule_targets(const Model& first, const TargetRules& rules, RuleTargetStats* stats) {
  RuleTargetStats st;
  std::map<FieldRef, uint8_t> chosen;  // bit 1: R-T1, bit 2: R-T2
  std::set<FieldRef> t1_unshared, t2_unshared;
  const std::set<std::string> gaf = global_address_functions(first);
  if (rules.t1) {
    for (const ArgFlowFact& a : first.argflows) {
      const bool field_source = a.src == KZ_ARGSRC_FIELD || a.src == KZ_ARGSRC_ADDRESS;
      if (!field_source || !record_field(a.type, a.field)) continue;
      const FieldRef f{a.type, a.field};
      if (shared_base(a, gaf)) {
        ++st.argflow_rows_shared;
        chosen[f] |= 1;
      } else {
        t1_unshared.insert(f);
      }
    }
  }
  if (rules.t2) {
    // per function, the fields in the conditions under which it returns a constant
    std::map<std::string, std::set<std::string>> const_return_fields;
    for (const ReturnFact& r : first.returns) {
      if (r.vkind != KZ_RET_VALUE_CONST) continue;
      std::set<std::string>& s = const_return_fields[r.func_key];
      field_loads(r.cond, &s);
      field_loads(r.guard, &s);
    }
    for (const ReadFact& r : first.reads) {
      if (r.kind != KZ_READ_KIND_FIELD || !record_field(r.type, r.field)) continue;
      const FieldRef f{r.type, r.field};
      bool decides = (r.use & (KZ_READ_USE_RETURN | KZ_READ_USE_OUT_STORE)) != 0;
      if (!decides && (r.use & KZ_READ_USE_BRANCH) != 0) {
        auto it = const_return_fields.find(r.func_key);
        decides = it != const_return_fields.end() && it->second.count(r.type + "." + r.field) != 0;
      }
      if (!decides) continue;
      if ((r.root_kind == KZ_READ_ROOT_GLOBAL && !r.root_object.empty()) || (r.root_kind == KZ_READ_ROOT_CALL && gaf.count(r.root) != 0)) {
        ++st.read_rows_shared;
        chosen[f] |= 2;
      } else {
        t2_unshared.insert(f);
      }
    }
  }
  std::vector<TargetField> out;
  for (const auto& [f, bits] : chosen) {
    if (bits & 1) ++st.t1;
    if (bits & 2) ++st.t2;
    if (bits == 3) ++st.both;
    out.push_back(TargetField{f, bits == 3 ? "rule R-T1,R-T2" : bits == 1 ? "rule R-T1" : "rule R-T2"});
  }
  for (const FieldRef& f : t1_unshared)
    if (!(chosen.count(f) != 0 && (chosen[f] & 1))) ++st.t1_unshared_only;
  for (const FieldRef& f : t2_unshared)
    if (!(chosen.count(f) != 0 && (chosen[f] & 2))) ++st.t2_unshared_only;
  if (stats != nullptr) *stats = st;
  return out;
}

}  // namespace kzs

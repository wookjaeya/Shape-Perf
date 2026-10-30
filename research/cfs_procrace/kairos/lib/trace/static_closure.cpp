// static_closure.cpp -- the static closure matched with the probe plan of a real record (see static_closure.hpp).

#include "kz/trace/static_closure.hpp"

#include <algorithm>
#include <functional>
#include <map>
#include <set>

#include "kz/core/registry.hpp"

namespace kz::trace {

namespace {

// A record component a field is bound to: the state, a tag or the id of a kind, or the value of a location.
struct Component {
  enum What { State, Tag, Id, Value } what = State;
  int32_t kind = -1;
  int32_t tag = -1;
  int32_t loc = -1;
  bool operator<(const Component& o) const {
    if (what != o.what) return what < o.what;
    if (kind != o.kind) return kind < o.kind;
    if (tag != o.tag) return tag < o.tag;
    return loc < o.loc;
  }
};

bool bound_component(const phi::BindDecl& b, Component* out) {
  const phi::Expr* t = b.target.get();
  if (t == nullptr || t->kind != phi::ExprKind::Call) return false;
  if (t->fn == phi::CallFn::Value) {
    if (t->ref_aux < 0) return false;
    out->what = Component::Value;
    out->loc = t->ref_aux;
    return true;
  }
  if (t->args.empty() || !t->args[0] || t->args[0]->kind != phi::ExprKind::Call || t->args[0]->fn != phi::CallFn::Res || t->args[0]->ref_index < 0) return false;
  out->kind = t->args[0]->ref_index;
  switch (t->fn) {
    case phi::CallFn::State: out->what = Component::State; return true;
    case phi::CallFn::Id: out->what = Component::Id; return true;
    case phi::CallFn::Tag:
      out->what = Component::Tag;
      out->tag = t->ref_aux;
      return out->tag >= 0;
    default: return false;
  }
}

bool ends_with_path(const std::string& path, const std::string& rel) {
  if (path == rel) return true;
  return path.size() > rel.size() && path.compare(path.size() - rel.size(), rel.size(), rel) == 0 && path[path.size() - rel.size() - 1] == '/';
}

}  // namespace

Status apply_static_closure(const Container& facts, const phi::Profile& p, const std::vector<ObservedSite>& observed, eval::Trace* trace, ClosureReport* report) {
  *report = ClosureReport();
  if (facts.header().artifact_kind != KZ_ARTIFACT_FACTS) return Status::error(Err::Invalid, "not a facts container");
  std::vector<kz_fact_loc> locs;
  std::vector<kz_fact_writer> writers;
  std::vector<kz_fact_call> calls;
  std::vector<kz_fact_atype> atypes;
  std::vector<kz_fact_target> targets;
  std::vector<kz_fact_func> funcs;
  Status s = facts.read_section(KZ_SEC_FACT_LOCS, &locs);
  if (s.ok()) s = facts.read_section(KZ_SEC_WRITERS, &writers);
  if (s.ok()) s = facts.read_section(KZ_SEC_FACTS, &calls);
  if (s.ok() && facts.find_section(KZ_SEC_FACT_ATYPE) != nullptr) s = facts.read_section(KZ_SEC_FACT_ATYPE, &atypes);
  if (s.ok() && facts.find_section(KZ_SEC_CALL_TARGETS) != nullptr) s = facts.read_section(KZ_SEC_CALL_TARGETS, &targets);
  if (s.ok()) s = facts.read_section(KZ_SEC_FUNCS, &funcs);
  if (!s.ok()) return s;
  auto str = [&](const kz_str& x) {
    std::string v;
    facts.string(x, &v);
    return v;
  };
  report->targets = locs.size();

  // ---- what each observed point writes, by the profile ----
  const std::map<std::string, int32_t> pubs = eval::publication_sites(p);
  std::map<std::string, std::set<Component>> writes;  // point name -> components
  for (const phi::OpDecl& op : p.ops) {
    for (const phi::EffectDecl& e : op.effects) {
      if (e.lp.kind == phi::LpKind::None) continue;
      const std::string name = eval::lp_name(op, e.label, e.lp);
      std::set<Component>& w = writes[name];
      Component c;
      switch (e.kind) {
        case phi::EffectKind::Set:
        case phi::EffectKind::Raise: {
          c.kind = !e.all_kind.empty() ? p.find_kind(e.all_kind) : (e.res ? e.res->sort.kind_index : -1);
          if (c.kind < 0) break;
          c.what = Component::State;
          w.insert(c);
          if (e.kind == phi::EffectKind::Set && e.with_id) {
            c.what = Component::Id;
            w.insert(c);
          }
          break;
        }
        case phi::EffectKind::SetTag: {
          c.kind = e.res ? e.res->sort.kind_index : -1;
          if (c.kind < 0) break;
          const phi::KindDecl& k = p.kinds[static_cast<size_t>(c.kind)];
          for (size_t ti = 0; ti < k.tags.size(); ++ti)
            if (k.tags[ti].name == e.target) c.tag = static_cast<int32_t>(ti);
          if (c.tag < 0) break;
          c.what = Component::Tag;
          w.insert(c);
          break;
        }
        case phi::EffectKind::StoreLoc:
          c.what = Component::Value;
          c.loc = p.find_loc(e.target);
          if (c.loc >= 0) w.insert(c);
          break;
        default: break;  // a store through a pointer names no location at its point
      }
    }
  }
  for (const auto& kv : pubs) {
    Component c;
    c.what = Component::Value;
    c.loc = kv.second;
    writes[kv.first].insert(c);
  }
  std::string at_line;  // a point observed at the line of the last direct() check that does not write the component
  auto direct = [&](const std::string& file, uint32_t line, const Component& c) {
    at_line.clear();
    for (const ObservedSite& o : observed) {
      if (o.line != line || !ends_with_path(o.file, file)) continue;
      const auto w = writes.find(o.point);
      if (w != writes.end() && w->second.count(c) != 0) return true;
      at_line = o.point;
    }
    return false;
  };
  auto component_text = [&](const Component& c) {
    switch (c.what) {
      case Component::State: return "the state of " + p.kinds[static_cast<size_t>(c.kind)].name;
      case Component::Id: return "the id of " + p.kinds[static_cast<size_t>(c.kind)].name;
      case Component::Tag: return "tag " + p.kinds[static_cast<size_t>(c.kind)].tags[static_cast<size_t>(c.tag)].name + " of " + p.kinds[static_cast<size_t>(c.kind)].name;
      case Component::Value: return "the value of " + p.locs[static_cast<size_t>(c.loc)].name;
    }
    return std::string("?");
  };
  std::set<std::string> indirect_targets;
  for (const kz_fact_target& t : targets)
    if (t.kind == KZ_CALL_TARGET_KIND_INDIRECT && t.fn < funcs.size()) indirect_targets.insert(str(funcs[t.fn].name));
  // Every direct call of `fn` is at an observed point that writes `c`, or (a few levels up) every call of its caller is.
  std::string why;
  std::function<bool(const std::string&, const Component&, int)> calls_covered = [&](const std::string& fn, const Component& c, int depth) -> bool {
    if (depth > 4) {
      why = "the calls of " + fn + " are nested too deep to follow";
      return false;
    }
    if (indirect_targets.count(fn) != 0) {
      why = fn + " is the target of an indirect call";
      return false;
    }
    bool any = false;
    for (const kz_fact_call& k : calls) {
      if (str(k.callee) != fn) continue;
      any = true;
      const std::string file = str(k.file), caller = str(k.caller);
      if (direct(file, k.line, c)) continue;
      if (!calls_covered(caller, c, depth + 1)) {
        if (why.empty() || depth == 0) why = "the call at " + file + ":" + std::to_string(k.line) + " (" + caller + ") is not observed";
        return false;
      }
    }
    if (!any) why = fn + " has no direct call in the closure";
    return any;
  };

  // ---- key separation (`bind ... via`; SPEC 4.2 step 8) ----
  // A field the records of several kinds share (a table of all object types): the profile names the functions inside
  // whose executions a write of the field changes the bound component.  A static writer whose function is one of them,
  // or is reached from one by direct calls, is covered where that function's operation has an observed effect point
  // that writes the component (the operation's effects stand for the writes it makes); a writer reached from none of
  // them is key-separated (its executions address records of another key) unless its function is the target of an
  // indirect call (then it is not covered: separation is not shown).
  std::map<std::string, std::set<std::string>> callees;  // caller -> direct callees
  for (const kz_fact_call& k : calls) callees[str(k.caller)].insert(str(k.callee));
  std::set<std::string> observed_points;
  for (const ObservedSite& o : observed) observed_points.insert(o.point);
  auto via_covers = [&](const std::string& fn, const Component& c, std::string* lp) {
    for (const phi::OpDecl& op : p.ops) {
      if (op.symbol != fn) continue;
      for (const phi::EffectDecl& e : op.effects) {
        if (e.lp.kind == phi::LpKind::None) continue;
        const std::string name = eval::lp_name(op, e.label, e.lp);
        const auto w = writes.find(name);
        if (w == writes.end() || w->second.count(c) == 0 || observed_points.count(name) == 0) continue;
        *lp = name;
        return true;
      }
    }
    return false;
  };
  std::map<std::string, std::set<std::string>> reach_cache;
  auto reach_of = [&](const std::string& root) -> const std::set<std::string>& {
    auto it = reach_cache.find(root);
    if (it != reach_cache.end()) return it->second;
    std::set<std::string>& seen = reach_cache[root];
    std::vector<std::string> work{root};
    seen.insert(root);
    while (!work.empty()) {
      const std::string f = work.back();
      work.pop_back();
      const auto cs = callees.find(f);
      if (cs == callees.end()) continue;
      for (const std::string& g : cs->second)
        if (seen.insert(g).second) work.push_back(g);
    }
    return seen;
  };

  // ---- per kind and location ----
  struct Acc {
    std::vector<std::string> status, unobserved;
    uint32_t writers = 0, covered = 0, separated = 0;
    bool all_complete = true, any_bound = false, via = false;
  };
  std::map<int32_t, Acc> by_kind, by_loc;
  for (const phi::BindDecl& b : p.binds) {
    Component c;
    if (!bound_component(b, &c)) continue;
    ++report->bound_fields;
    Acc& a = c.what == Component::Value ? by_loc[c.loc] : by_kind[c.kind];
    a.any_bound = true;
    const std::string field = b.struct_name + "." + b.field;
    const kz_fact_loc* row = nullptr;
    for (const kz_fact_loc& l : locs)
      if (str(l.loc_type) == b.struct_name && str(l.loc_field) == b.field) row = &l;
    if (row == nullptr) {
      a.all_complete = false;
      a.status.push_back(field + " not in the closure");
      a.unobserved.push_back(field + ": not a target of the static closure");
      continue;
    }
    const EnumDesc* sd = find_enum("KZ_LOC_STATUS");
    const char* st = sd != nullptr ? enum_display(*sd, row->status) : nullptr;
    a.status.push_back(field + " " + (st != nullptr ? st : "?"));
    if (row->status != KZ_LOC_STATUS_COMPLETE) {
      a.all_complete = false;
      for (const kz_fact_atype& t : atypes) {
        if (str(t.loc_type) != b.struct_name || str(t.loc_field) != b.field) continue;
        const EnumDesc* ad = find_enum("KZ_ATYPE_KIND");
        const char* an = ad != nullptr ? enum_display(*ad, t.kind) : nullptr;
        a.unobserved.push_back(field + " alias " + (an != nullptr ? an : "?") + " at " + str(t.file) + ":" + std::to_string(t.line));
      }
    }
    if (!b.via.empty()) a.via = true;
    for (const kz_fact_writer& w : writers) {
      if (str(w.loc_type) != b.struct_name || str(w.loc_field) != b.field) continue;
      ++a.writers;
      ++report->writers;
      const std::string file = str(w.file), fn = str(w.func);
      why.clear();
      const bool at_point = direct(file, w.line, c);
      const std::string here = at_line;
      if (!at_point && !b.via.empty()) {
        std::vector<std::string> roots;
        for (const std::string& v : b.via)
          if (reach_of(v).count(fn) != 0) roots.push_back(v);
        bool ok = true;
        std::string missing;
        if (roots.empty()) {
          if (indirect_targets.count(fn) != 0) {
            ok = false;
            missing = fn + " is the target of an indirect call and none of the 'via' functions reaches it by direct calls: key separation not shown";
          } else {
            ++a.separated;
          }
        } else {
          for (const std::string& v : roots) {
            std::string lp;
            if (!via_covers(v, c, &lp)) {
              ok = false;
              missing = "reached from " + v + ", whose operation has no observed effect point that writes " + component_text(c);
            }
          }
        }
        if (ok) {
          ++a.covered;
          ++report->writers_observed;
          continue;
        }
        const EnumDesc* wd = find_enum("KZ_WRITER_KIND");
        const char* wn = wd != nullptr ? enum_display(*wd, w.kind) : nullptr;
        a.unobserved.push_back(field + " writer " + (wn != nullptr ? wn : "?") + " at " + file + ":" + std::to_string(w.line) + " in " + fn + " (" + missing + ")");
        continue;
      }
      const bool ok = at_point || (w.kind != KZ_WRITER_KIND_OUTPTR && calls_covered(fn, c, 0));
      if (ok) {
        ++a.covered;
        ++report->writers_observed;
        continue;
      }
      const EnumDesc* wd = find_enum("KZ_WRITER_KIND");
      const char* wn = wd != nullptr ? enum_display(*wd, w.kind) : nullptr;
      std::string u = field + " writer " + (wn != nullptr ? wn : "?") + " at " + file + ":" + std::to_string(w.line) + " in " + fn;
      if (!here.empty()) u += " (the point " + here + " observed at that line does not write " + component_text(c) + ")";
      else if (w.kind == KZ_WRITER_KIND_OUTPTR) u += " (the call is not observed as a publication site)";
      else if (!why.empty()) u += " (" + why + ")";
      a.unobserved.push_back(u);
    }
  }
  auto finish = [&](Acc& a, const std::string& name, eval::Adequacy* adq, eval::StaticClosure* out, const char* what) {
    std::sort(a.unobserved.begin(), a.unobserved.end());
    a.unobserved.erase(std::unique(a.unobserved.begin(), a.unobserved.end()), a.unobserved.end());
    for (const std::string& x : a.status) out->status += (out->status.empty() ? "" : "; ") + x;
    out->writers = a.writers;
    out->covered = a.covered;
    out->separated = a.separated;
    out->unobserved = a.unobserved;
    adq->aliases_covered = a.all_complete;
    adq->writers_covered = a.all_complete && a.covered == a.writers;
    report->lines.push_back(std::string("static closure ") + what + " " + name + ": " + out->status + "; writers " + std::to_string(a.writers) + ", at observed points " +
                            std::to_string(a.covered - a.separated) + (a.via ? ", key-separated by 'via' " + std::to_string(a.separated) : std::string()) + "; aliases " +
                            (adq->aliases_covered ? "covered" : "not covered") + ", writers " + (adq->writers_covered ? "covered" : "not covered"));
  };
  for (auto& kv : by_kind) {
    const phi::KindDecl& k = p.kinds[static_cast<size_t>(kv.first)];
    if (!k.derived.empty()) continue;
    finish(kv.second, k.name, &trace->kind_adequacy[k.name], &trace->kind_closure[k.name], "kind");
  }
  for (auto& kv : by_loc) {
    const phi::LocDecl& l = p.locs[static_cast<size_t>(kv.first)];
    finish(kv.second, l.name, &trace->loc_adequacy[l.name], &trace->loc_closure[l.name], "loc");
  }
  return Status::success();
}

}  // namespace kz::trace

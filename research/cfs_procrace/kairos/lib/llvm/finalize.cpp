// finalize.cpp -- see finalize.hpp.
#include "finalize.hpp"

#include <algorithm>
#include <functional>
#include <queue>
#include <tuple>

#include "argflow.hpp"
#include "unroll.hpp"

namespace kzs {

// ---- function-pointer flow --------------------------------------------------------------------------

FlowSolution solve_flow(const std::vector<FlowEdge>& edges) {
  FlowSolution sol;
  std::set<std::pair<std::string, std::string>> uniq;
  std::map<std::string, std::vector<std::string>> succ;
  std::set<std::string> nodes;
  for (const FlowEdge& e : edges) {
    if (!uniq.insert({e.src, e.dst}).second) continue;
    succ[e.src].push_back(e.dst);
    nodes.insert(e.src);
    nodes.insert(e.dst);
  }
  for (const std::string& n : nodes) {
    if (n.compare(0, 2, "F:") == 0) sol.funcs[n].insert(n.substr(2));
    if (n.compare(0, 2, "?:") == 0) sol.unknown[n] = true;
  }
  std::queue<std::string> work;
  for (const std::string& n : nodes) work.push(n);
  while (!work.empty()) {
    const std::string n = work.front();
    work.pop();
    auto sit = succ.find(n);
    if (sit == succ.end()) continue;
    const std::set<std::string> from = sol.funcs[n];
    const bool unk = sol.unknown[n];
    for (const std::string& d : sit->second) {
      bool changed = false;
      std::set<std::string>& to = sol.funcs[d];
      for (const std::string& f : from)
        if (to.insert(f).second) changed = true;
      if (unk && !sol.unknown[d]) {
        sol.unknown[d] = true;
        changed = true;
      }
      if (changed) work.push(d);
    }
  }
  return sol;
}

namespace {

// ---- sorting helpers ---------------------------------------------------------------------------------


// ---- out-pointer summaries ---------------------------------------------------------------------------

struct OutResult {
  bool store_only = false;
  bool body_known = false;
  std::vector<OutStore> stores;
  std::string reason;
};

class OutPtr {
 public:
  explicit OutPtr(const Model& m) : m_(m) {}
  const OutResult& get(const std::string& fkey, uint32_t k) {
    const std::string id = fkey + "#" + std::to_string(k);
    auto it = memo_.find(id);
    if (it != memo_.end()) return it->second;
    if (visiting_.count(id) != 0) {
      static const OutResult cyc = [] {
        OutResult r;
        r.body_known = true;
        r.reason = "recursion";
        return r;
      }();
      return cyc;
    }
    visiting_.insert(id);
    OutResult r = compute(fkey, k);
    visiting_.erase(id);
    return memo_[id] = r;
  }

 private:
  OutResult compute(const std::string& fkey, uint32_t k) {
    OutResult r;
    auto it = m_.summaries.find(fkey);
    if (it == m_.summaries.end() || k >= it->second.params.size()) {
      r.reason = "no body in the closed program";
      return r;
    }
    r.body_known = true;
    const ParamSummary& ps = it->second.params[k];
    if (!ps.is_pointer) {
      r.reason = "not a pointer";
      return r;
    }
    if (ps.escapes) {
      r.reason = ps.escape_detail;
      return r;
    }
    r.stores = ps.stores;
    for (const ParamPass& pp : ps.passes) {
      if (pp.data_fn) {
        OutStore st;
        st.pos = pp.pos;
        st.value_kind = KZ_ARG_KIND_TOP;
        st.value_text = "written by " + pp.callee_name;
        st.via_func_key = pp.callee_key;
        st.via_pos = pp.pos;
        r.stores.push_back(st);
        continue;
      }
      const OutResult& inner = get(pp.callee_key, pp.arg_index);
      if (!inner.store_only) {
        r.reason = "passed to " + pp.callee_name + " (" + (inner.reason.empty() ? "not store-only" : inner.reason) + ")";
        r.stores.clear();
        return r;
      }
      for (OutStore st : inner.stores) {
        if (st.via_func_key.empty()) {
          st.via_func_key = pp.callee_key;
          st.via_pos = st.pos;
        }
        st.pos = pp.pos;
        r.stores.push_back(st);
      }
    }
    r.store_only = true;
    return r;
  }

  const Model& m_;
  std::map<std::string, OutResult> memo_;
  std::set<std::string> visiting_;
};

// ---- table walks -------------------------------------------------------------------------------------

const ImageFact* find_entry(const std::vector<ImageFact>& img, const std::string& path) {
  for (const ImageFact& f : img)
    if (f.path == path) return &f;
  return nullptr;
}

bool has_prefix_entries(const std::vector<ImageFact>& img, const std::string& prefix) {
  for (const ImageFact& f : img)
    if (f.path.compare(0, prefix.size() + 1, prefix + ".") == 0 || f.path.compare(0, prefix.size() + 1, prefix + "[") == 0) return true;
  return false;
}

}  // namespace

bool resolve_table_walk(const Model& model, const Model::WalkSpec& walk, std::vector<ImageFact>* elements, int64_t* trip, std::string* why,
                        std::vector<ImageFact>* entries) {
  elements->clear();
  if (entries != nullptr) entries->clear();
  auto find_image = [&](const std::string& obj) -> const std::vector<ImageFact>* {
    auto it = model.object_images.find(obj);
    return it == model.object_images.end() ? nullptr : &it->second;
  };
  std::string cur = walk.root_object, prefix;
  const std::vector<ImageFact>* img = find_image(cur);
  for (const std::string& f : walk.fields) {
    if (img == nullptr) {
      *why = "the initializer of " + cur + " is not in the closed program";
      return false;
    }
    const std::string want = prefix + "." + f;
    const ImageFact* e = find_entry(*img, want);
    if (e != nullptr) {
      elements->push_back(*e);
      if (e->kind == KZ_IMAGE_KIND_ADDR) {
        cur = e->text;
        prefix.clear();
        img = find_image(cur);
        continue;
      }
      *why = "the field " + want + " of " + cur + " is not the address of a table";
      return false;
    }
    if (has_prefix_entries(*img, want)) {
      prefix = want;
      continue;
    }
    *why = "the field " + want + " has no initializer in " + cur + " (it is zero or not constant)";
    return false;
  }
  if (img == nullptr) {
    *why = "the initializer of " + cur + " is not in the closed program";
    return false;
  }
  int64_t n = 0;
  bool terminated = false;
  for (int64_t i = 0;; ++i) {
    const std::string path = prefix + "[" + std::to_string(i) + "]";
    const ImageFact* e = find_entry(*img, path);
    if (e == nullptr) {
      // the explicit elements ended: a zero-filled rest terminates, otherwise there is no end
      for (const ImageFact& f : *img)
        if (f.kind == KZ_IMAGE_KIND_ZERO_FILL && f.path.compare(0, prefix.size() + 1, prefix + "[") == 0) terminated = true;
      break;
    }
    elements->push_back(*e);
    if (e->kind == KZ_IMAGE_KIND_NULLPTR || (e->kind == KZ_IMAGE_KIND_INT && e->value == 0)) {
      terminated = true;
      break;
    }
    if (entries != nullptr) entries->push_back(*e);
    ++n;
    if (e->kind == KZ_IMAGE_KIND_ADDR) {
      const std::vector<ImageFact>* eo = find_image(e->text);
      if (eo != nullptr) elements->insert(elements->end(), eo->begin(), eo->end());
    }
  }
  if (!terminated) {
    *why = "no null element ends the table " + cur + prefix;
    return false;
  }
  *trip = n;
  return true;
}

// ---- finalize ------------------------------------------------------------------------------------------

void finalize(Model* mp, const ExtractSpec& spec) {
  Model& m = *mp;
  Model::Resolved& R = m.resolved;

  // functions
  for (const auto& kv : m.funcs) R.func_keys.push_back(kv.first);
  for (size_t i = 0; i < R.func_keys.size(); ++i) R.fn_index[R.func_keys[i]] = static_cast<uint32_t>(i);

  // sort the located tables
  std::stable_sort(m.calls.begin(), m.calls.end(), [](const CallFact& a, const CallFact& b) {
    if (!(a.pos == b.pos)) return a.pos < b.pos;
    if (a.caller_key != b.caller_key) return a.caller_key < b.caller_key;
    return a.ordinal < b.ordinal;
  });
  std::stable_sort(m.loops.begin(), m.loops.end(), [](const LoopFact& a, const LoopFact& b) { return a.pos < b.pos; });
  for (size_t i = 0; i < m.calls.size(); ++i) {
    CallFact& c = m.calls[i];
    // The call key was written as "<pos>#<ordinal>" by the reader; rebuild it the same way.
    const std::string key = c.pos.str() + "#" + std::to_string(c.ordinal);
    if (!R.call_index.emplace(key, i).second) ++R.key_collisions;
  }
  for (size_t i = 0; i < m.loops.size(); ++i) R.loop_index[m.loops[i].key] = i;

  // ---- indirect calls and spawn entries
  const FlowSolution flow = solve_flow(m.flow);
  {
    std::set<std::string> nodes;
    for (const FlowEdge& e : m.flow) {
      nodes.insert(e.src);
      nodes.insert(e.dst);
    }
    R.flow_nodes = static_cast<uint32_t>(nodes.size());
    std::set<std::pair<std::string, std::string>> uniq;
    for (const FlowEdge& e : m.flow) uniq.insert({e.src, e.dst});
    R.flow_edges = static_cast<uint32_t>(uniq.size());
  }
  auto candidates_of = [&](const std::string& node, std::set<std::string>* fns, bool* unknown) {
    *unknown = false;
    if (node.compare(0, 2, "F:") == 0) {
      fns->insert(node.substr(2));
      return;
    }
    if (node.compare(0, 2, "?:") == 0) {
      *unknown = true;
      return;
    }
    auto fit = flow.funcs.find(node);
    if (fit != flow.funcs.end()) *fns = fit->second;
    auto uit = flow.unknown.find(node);
    if (uit != flow.unknown.end() && uit->second) *unknown = true;
  };
  std::map<std::string, std::set<std::string>> indirect_targets;  // call key -> function keys
  std::set<std::string> unresolved_calls;
  for (const IndirectCall& ic : m.indirect_calls) {
    ++R.indirect_total;
    std::set<std::string> fns;
    bool unknown = false;
    candidates_of(ic.node, &fns, &unknown);
    TargetFact t;
    t.call_key = ic.call_key;
    t.kind = KZ_CALL_TARGET_KIND_INDIRECT;
    t.via = ic.node;
    t.n = static_cast<uint32_t>(fns.size());
    for (const std::string& f : fns) {
      t.fn_key = f;
      t.reason = KZ_CALL_TARGET_REASON_RESOLVED;
      m.targets.push_back(t);
    }
    indirect_targets[ic.call_key] = fns;
    if (unknown || fns.empty()) {
      t.fn_key.clear();
      t.reason = unknown ? KZ_CALL_TARGET_REASON_UNKNOWN_SOURCE : KZ_CALL_TARGET_REASON_NO_FLOW;
      m.targets.push_back(t);
      ++R.indirect_unresolved;
      unresolved_calls.insert(ic.call_key);
    } else {
      ++R.indirect_resolved;
    }
  }
  for (const std::string& ck : m.spawn_calls) {
    ++R.spawn_total;
    std::set<std::string> fns;
    bool unknown = false;
    std::string via;
    auto it = m.spawn_entry_nodes.find(ck);
    if (it != m.spawn_entry_nodes.end()) {
      for (const std::string& node : it->second) {
        std::set<std::string> f2;
        bool u2 = false;
        candidates_of(node, &f2, &u2);
        fns.insert(f2.begin(), f2.end());
        unknown |= u2;
        if (!via.empty()) via += ",";
        via += node;
      }
      if (it->second.empty()) unknown = true;
    } else {
      unknown = true;
    }
    // the profile-free mode: entries the startup script names for an entry that is data (wrappers.hpp)
    auto sit = spec.spawn_script_entries.find(ck);
    if (sit != spec.spawn_script_entries.end() && !sit->second.empty()) {
      fns.insert(sit->second.begin(), sit->second.end());
      unknown = false;
      via += (via.empty() ? "" : ",") + std::string("startup-script entries through the symbol lookup");
    }
    TargetFact t;
    t.call_key = ck;
    t.kind = KZ_CALL_TARGET_KIND_SPAWN_ENTRY;
    t.via = via;
    t.n = static_cast<uint32_t>(fns.size());
    for (const std::string& f : fns) {
      t.fn_key = f;
      m.targets.push_back(t);
    }
    if (unknown || fns.empty()) {
      t.fn_key.clear();
      t.reason = unknown ? KZ_CALL_TARGET_REASON_UNKNOWN_SOURCE : KZ_CALL_TARGET_REASON_NO_FLOW;
      m.targets.push_back(t);
    } else {
      ++R.spawn_resolved;
    }
  }

  // ---- the program graph: calls, resolved indirect targets, unresolved sites
  ProgramGraph graph;
  graph.indirect_targets = indirect_targets;
  graph.unresolved_calls = unresolved_calls;
  for (const CallFact& c : m.calls) {
    const std::string key = c.pos.str() + "#" + std::to_string(c.ordinal);
    graph.call_caller[key] = c.caller_key;
    if (!c.callee_key.empty()) graph.cg[c.caller_key].insert(c.callee_key);
    auto it = indirect_targets.find(key);
    if (it != indirect_targets.end()) graph.cg[c.caller_key].insert(it->second.begin(), it->second.end());
  }
  find_spawn_params(&m, graph);

  // ---- table walks
  std::map<std::string, std::vector<ImageFact>> walk_entries;  // loop key -> one entry per iteration
  for (const Model::WalkSpec& w : m.walks) {
    auto lit = R.loop_index.find(w.loop_key);
    if (lit == R.loop_index.end()) continue;
    LoopFact& lf = m.loops[lit->second];
    std::vector<ImageFact> elems, entries;
    int64_t trip = 0;
    std::string why;
    WalkTrust trust;
    const bool resolved = resolve_table_walk(m, w, &elems, &trip, &why, &entries);
    if (resolved) trust = check_walk_links(m, w, entries);
    if (resolved && !trust.written.empty()) {
      lf.trip_status = KZ_TRIP_STATUS_UNKNOWN;
      lf.trip_note = "the static image may not be the value at run time: " + trust.written;
      ++R.walks_failed;
    } else if (resolved) {
      if (!trust.assumed.empty()) {
        std::string links;
        for (const std::string& a : trust.assumed) links += (links.empty() ? "" : ", ") + a;
        lf.image_assumed = links;
        lf.lflags |= KZ_LOOP_FLAG_IMAGE_ASSUMED;
        ++R.walks_assumed;
        m.assumptions.push_back(Model::Assumption{
            "assume.static_image[" + lf.key + "]",
            "the walk over " + lf.table + " reads links that are not const (" + links +
                "); their static-initializer images are taken as their values when the walk runs. No typed write of them was found; "
                "writes through pointers, casts and code without a body are not excluded."});
      }
      lf.trip = trip;
      lf.trip_status = KZ_TRIP_STATUS_EXACT;
      walk_entries[w.loop_key] = entries;
      ++R.walks_resolved;
      std::set<std::pair<std::string, std::string>> seen;
      for (const ImageFact& e : m.images) seen.insert({e.object, e.path});
      for (const ImageFact& e : elems)
        if (seen.insert({e.object, e.path}).second) m.images.push_back(e);
    } else {
      lf.trip_status = KZ_TRIP_STATUS_UNKNOWN;
      lf.trip_note = why;
      ++R.walks_failed;
    }
  }
  unroll_table_walks(&m, walk_entries);

  // ---- call graph (for the early-initialization flags)
  const std::map<std::string, std::set<std::string>>& cg = graph.cg;
  std::set<std::string> walk_called;
  for (const LoopFact& lf : m.loops) {
    if (lf.kind != KZ_LOOP_KIND_TABLE_WALK) continue;
    for (const CallFact& c : m.calls) {
      if (c.loop_key != lf.key || c.kind != KZ_CALL_KIND_INDIRECT) continue;
      auto it = indirect_targets.find(c.pos.str() + "#" + std::to_string(c.ordinal));
      if (it != indirect_targets.end()) walk_called.insert(it->second.begin(), it->second.end());
    }
  }
  std::set<std::string> init_tree = walk_called;
  {
    std::queue<std::string> q;
    for (const std::string& f : walk_called) q.push(f);
    while (!q.empty()) {
      const std::string f = q.front();
      q.pop();
      auto it = cg.find(f);
      if (it == cg.end()) continue;
      for (const std::string& g : it->second)
        if (init_tree.insert(g).second) q.push(g);
    }
  }

  // ---- out-pointer calls: summaries, instantiated writers, escapes
  OutPtr outptr(m);
  for (OutCallFact& oc : m.outcalls) {
    if (oc.summary == KZ_OUTPTR_SUMMARY_DATA_FN) {
      ++R.outptr_data_fn;
      continue;
    }
    bool escape = false;
    std::string why;
    if (oc.callee_key.empty()) {
      oc.summary = KZ_OUTPTR_SUMMARY_UNSUMMARIZED;
      why = "indirect call";
      escape = true;
    } else {
      const OutResult& r = outptr.get(oc.callee_key, oc.arg_index);
      if (!r.body_known) {
        oc.summary = KZ_OUTPTR_SUMMARY_UNSUMMARIZED;
        why = "the callee has no body in the closed program";
        escape = true;
      } else if (!r.store_only) {
        oc.summary = KZ_OUTPTR_SUMMARY_ESCAPES;
        why = r.reason;
        escape = true;
      } else {
        oc.summary = KZ_OUTPTR_SUMMARY_STORE_ONLY;
        oc.n_stores = static_cast<uint32_t>(r.stores.size());
        ++R.outptr_store_only;
        for (const OutStore& st : r.stores) {
          WriterFact w;
          w.loc_type = oc.loc_type;
          w.loc_field = oc.loc_field;
          w.pos = oc.pos;
          w.ordinal = oc.ordinal;
          w.tu = oc.tu;
          w.func_key = oc.func_key;
          auto fit = m.funcs.find(oc.func_key);
          w.func_name = fit != m.funcs.end() ? fit->second.name : oc.func_key;
          w.kind = KZ_WRITER_KIND_OUTPTR;
          w.value_kind = st.value_kind;
          w.value = st.value;
          w.value_text = st.value_text;
          w.key = "*" + oc.callee_name + " argument " + std::to_string(oc.arg_index);
          w.guard = st.guard;
          w.via = st.via_func_key.empty() ? st.pos : st.via_pos;
          w.via_func_key = st.via_func_key.empty() ? oc.callee_key : st.via_func_key;
          w.call_key = oc.call_key;
          m.writers.push_back(w);
        }
      }
    }
    if (escape) {
      if (oc.summary == KZ_OUTPTR_SUMMARY_ESCAPES) ++R.outptr_escapes;
      else ++R.outptr_unsummarized;
      ATypeFact a;
      a.kind = KZ_ATYPE_KIND_ADDRESS_ESCAPE;
      a.type_name = oc.loc_type;
      a.loc_field = oc.loc_field;
      a.loc_type = oc.loc_type;
      a.pos = oc.pos;
      a.ordinal = oc.ordinal;
      a.func_key = oc.func_key;
      a.tu = oc.tu;
      a.call_key = oc.call_key;
      a.detail = "the address is passed to " + (oc.callee_name.empty() ? std::string("an indirect call") : oc.callee_name) + ": " + why;
      m.atypes.push_back(a);
    }
  }

  // ---- pointers that reach a target and go to code whose effect is unknown (SPEC 4.2 steps 2-3)
  // The writer collection reads every body of the closed program, so a callee with a body is
  // covered by its own facts, whether it is called directly or through a resolved pointer.  What
  // is not covered: a callee without a body, an indirect call whose targets are not all known,
  // and a variadic position (the callee's va_arg reads are not followed).  A pointer that reaches
  // the owner type of a target and goes there may be used to write any target of that type.
  for (const Model::ReachPass& rp : m.reach_passes) {
    std::string why;
    auto no_body = [&](const std::string& fk) { return m.summaries.count(fk) == 0; };
    auto nparams_of = [&](const std::string& fk) -> uint32_t {
      auto it = m.funcs.find(fk);
      return it == m.funcs.end() ? 0u : it->second.nparams;
    };
    if (rp.callee_key.empty()) {
      auto it = indirect_targets.find(rp.call_key);
      if (unresolved_calls.count(rp.call_key) != 0 || it == indirect_targets.end() || it->second.empty()) {
        why = "an indirect call whose targets are not all known";
      } else {
        for (const std::string& f : it->second) {
          if (no_body(f)) why = "an indirect call that may reach " + f + ", which has no body in the closed program";
          else if (rp.arg_index >= nparams_of(f)) why = "an indirect call that may reach " + f + ", in a variadic position";
          if (!why.empty()) break;
        }
      }
    } else if (spec.data_fns.count(rp.callee_name) != 0) {
      continue;  // a data function: its summary says what it writes
    } else if (rp.variadic_position) {
      why = rp.callee_name + ", in a variadic position (its va_arg reads are not followed)";
    } else if (no_body(rp.callee_key)) {
      why = rp.callee_name + ", which has no body in the closed program";
    }
    if (why.empty()) continue;
    ++R.reach_passes_unknown;
    for (const std::string& type : rp.types) {
      ATypeFact a;
      a.kind = KZ_ATYPE_KIND_UNKNOWN_EFFECT;
      a.type_name = type;
      a.pos = rp.pos;
      a.ordinal = rp.ordinal;
      a.func_key = rp.func_key;
      a.tu = rp.tu;
      a.call_key = rp.call_key;
      a.detail = "a pointer that reaches the type (argument " + std::to_string(rp.arg_index) + ": " + rp.text + ") is passed to " + why;
      m.atypes.push_back(a);
    }
  }

  // ---- A-TYPE listing
  std::set<std::string> exposed_types;
  for (const ATypeFact& a : m.atypes)
    if (a.kind == KZ_ATYPE_KIND_CAST || a.kind == KZ_ATYPE_KIND_UNION || a.kind == KZ_ATYPE_KIND_ADDRESS_ESCAPE || a.kind == KZ_ATYPE_KIND_UNKNOWN_EFFECT)
      exposed_types.insert(a.type_name);
  R.atype_untyped_sites_total = static_cast<uint32_t>(m.untyped_writes.size());
  if (!exposed_types.empty()) {
    R.atype_untyped_sites_relevant = static_cast<uint32_t>(m.untyped_writes.size());
    for (const std::string& t : exposed_types)
      for (const Model::UntypedWrite& u : m.untyped_writes) {
        ATypeFact a;
        a.kind = KZ_ATYPE_KIND_UNTYPED_WRITE;
        a.type_name = t;
        a.pos = u.pos;
        a.ordinal = u.ordinal;
        a.func_key = u.func_key;
        a.tu = u.tu;
        a.detail = u.detail;
        a.call_key = u.call_key;
        m.atypes.push_back(a);
      }
  }
  {  // expand to the target fields of the type, deduplicate, sort
    std::vector<ATypeFact> out;
    for (const ATypeFact& a : m.atypes) {
      for (const TargetField& tf : spec.fields) {
        if (tf.field.type != a.type_name) continue;
        if (!a.loc_field.empty() && a.loc_field != tf.field.field) continue;
        ATypeFact b = a;
        b.loc_type = tf.field.type;
        b.loc_field = tf.field.field;
        out.push_back(b);
      }
    }
    std::sort(out.begin(), out.end(), [](const ATypeFact& a, const ATypeFact& b) {
      return std::tie(a.loc_type, a.loc_field, a.kind) < std::tie(b.loc_type, b.loc_field, b.kind) ||
             (std::tie(a.loc_type, a.loc_field, a.kind) == std::tie(b.loc_type, b.loc_field, b.kind) &&
              (a.pos < b.pos || (a.pos == b.pos && (a.ordinal < b.ordinal || (a.ordinal == b.ordinal && a.detail < b.detail)))));
    });
    out.erase(std::unique(out.begin(), out.end(), [](const ATypeFact& a, const ATypeFact& b) {
                return a.loc_type == b.loc_type && a.loc_field == b.loc_field && a.kind == b.kind && a.pos == b.pos && a.detail == b.detail;
              }),
              out.end());
    m.atypes = out;
  }

  // ---- writers: flags and order
  for (WriterFact& w : m.writers) {
    if (walk_called.count(w.func_key) != 0) w.flags |= KZ_WRITER_FLAG_EARLY_INIT;
    if (init_tree.count(w.func_key) != 0) w.flags |= KZ_WRITER_FLAG_INIT_TREE;
  }
  std::stable_sort(m.writers.begin(), m.writers.end(), [](const WriterFact& a, const WriterFact& b) {
    if (a.loc_type != b.loc_type) return a.loc_type < b.loc_type;
    if (a.loc_field != b.loc_field) return a.loc_field < b.loc_field;
    if (!(a.pos == b.pos)) return a.pos < b.pos;
    if (a.ordinal != b.ordinal) return a.ordinal < b.ordinal;
    if (a.kind != b.kind) return a.kind < b.kind;
    if (!(a.via == b.via)) return a.via < b.via;
    return a.value_text < b.value_text;
  });

  // ---- per-target summary rows
  for (const TargetField& tf : spec.fields) {
    LocFact l;
    l.loc_type = tf.field.type;
    l.loc_field = tf.field.field;
    for (const WriterFact& w : m.writers) {
      if (w.loc_type != l.loc_type || w.loc_field != l.loc_field) continue;
      ++l.n_writers;
      if (w.kind == KZ_WRITER_KIND_ASSIGN || w.kind == KZ_WRITER_KIND_INCDEC || w.kind == KZ_WRITER_KIND_AGGREGATE) ++l.n_direct;
      else if (w.kind == KZ_WRITER_KIND_MEMSET || w.kind == KZ_WRITER_KIND_MEMCPY) ++l.n_data_fn;
      else if (w.kind == KZ_WRITER_KIND_OUTPTR) ++l.n_outptr;
    }
    for (const ATypeFact& a : m.atypes) {
      if (a.loc_type != l.loc_type || a.loc_field != l.loc_field) continue;
      if (a.kind == KZ_ATYPE_KIND_UNTYPED_WRITE) ++l.n_untyped;
      else if (a.kind == KZ_ATYPE_KIND_ADDRESS_ESCAPE) ++l.n_escape;
      else if (a.kind == KZ_ATYPE_KIND_CAST) ++l.n_cast;
      else if (a.kind == KZ_ATYPE_KIND_UNION) ++l.n_union;
      else if (a.kind == KZ_ATYPE_KIND_UNKNOWN_EFFECT) ++l.n_unknown;
    }
    for (const InitFact& i : m.inits)
      if (i.loc_type == l.loc_type && i.loc_field == l.loc_field) ++l.n_init;
    // Informational: an unresolved indirect call decides nothing by itself.  Its possible targets
    // with a body are read like every other body; what it may do beyond them is an unknown-effect
    // hit when it receives a pointer that reaches the type (above), or an address-escape hit when
    // it receives the address of the field.
    l.n_unresolved_indirect = R.indirect_unresolved;
    l.status = (l.n_untyped + l.n_escape + l.n_cast + l.n_union + l.n_unknown) > 0 ? KZ_LOC_STATUS_DISABLED : KZ_LOC_STATUS_COMPLETE;
    m.locs.push_back(l);
  }

  // ---- the certificate-view rule of every loop (needs the writer-set status of the targets)
  decide_loop_rules(&m, spec, graph);

  // ---- sites: one number for every located fact, in the order of (file, line, col, category)
  struct SiteRow {
    Pos pos;
    int cat;
    uint32_t ord;
    std::string tie;
    int kind;  // 0 call, 1 loop, 2 writer, 3 atype
    size_t index;
  };
  std::vector<SiteRow> rows;
  for (size_t i = 0; i < m.calls.size(); ++i) rows.push_back({m.calls[i].pos, 0, m.calls[i].ordinal, m.calls[i].caller_key, 0, i});
  for (size_t i = 0; i < m.loops.size(); ++i) rows.push_back({m.loops[i].pos, 1, 0, m.loops[i].func_key, 1, i});
  for (size_t i = 0; i < m.writers.size(); ++i)
    if (m.writers[i].kind != KZ_WRITER_KIND_OUTPTR) rows.push_back({m.writers[i].pos, 2, m.writers[i].ordinal, m.writers[i].func_key + "|" + m.writers[i].loc_field, 2, i});
  for (size_t i = 0; i < m.atypes.size(); ++i) rows.push_back({m.atypes[i].pos, 3, m.atypes[i].ordinal, m.atypes[i].func_key + "|" + m.atypes[i].loc_field + "|" + m.atypes[i].detail, 3, i});
  std::stable_sort(rows.begin(), rows.end(), [](const SiteRow& a, const SiteRow& b) {
    if (!(a.pos == b.pos)) return a.pos < b.pos;
    if (a.cat != b.cat) return a.cat < b.cat;
    if (a.ord != b.ord) return a.ord < b.ord;
    return a.tie < b.tie;
  });
  R.writer_site.assign(m.writers.size(), kNone);
  R.atype_site.assign(m.atypes.size(), kNone);
  uint32_t next = 0;
  for (const SiteRow& r : rows) {
    const uint32_t site = next++;
    switch (r.kind) {
      case 0: {
        const CallFact& c = m.calls[r.index];
        R.call_site[c.pos.str() + "#" + std::to_string(c.ordinal)] = site;
        break;
      }
      case 1: R.loop_site[m.loops[r.index].key] = site; break;
      case 2: R.writer_site[r.index] = site; break;
      default: R.atype_site[r.index] = site; break;
    }
  }
  R.n_sites = next;
  for (size_t i = 0; i < m.writers.size(); ++i)
    if (m.writers[i].kind == KZ_WRITER_KIND_OUTPTR) {
      auto it = R.call_site.find(m.writers[i].call_key);
      R.writer_site[i] = it != R.call_site.end() ? it->second : kNone;
    }

  // order the remaining tables
  std::stable_sort(m.derives.begin(), m.derives.end(), [&](const DerivesFact& a, const DerivesFact& b) {
    const uint32_t sa = R.call_site.count(a.call_key) ? R.call_site[a.call_key] : kNone;
    const uint32_t sb = R.call_site.count(b.call_key) ? R.call_site[b.call_key] : kNone;
    if (sa != sb) return sa < sb;
    return a.arg_index < b.arg_index;
  });
  std::stable_sort(m.terminals.begin(), m.terminals.end(), [&](const TerminalFact& a, const TerminalFact& b) {
    const uint32_t sa = R.call_site.count(a.call_key) ? R.call_site[a.call_key] : kNone;
    const uint32_t sb = R.call_site.count(b.call_key) ? R.call_site[b.call_key] : kNone;
    return sa < sb;
  });
  std::stable_sort(m.outcalls.begin(), m.outcalls.end(), [&](const OutCallFact& a, const OutCallFact& b) {
    const uint32_t sa = R.call_site.count(a.call_key) ? R.call_site[a.call_key] : kNone;
    const uint32_t sb = R.call_site.count(b.call_key) ? R.call_site[b.call_key] : kNone;
    if (sa != sb) return sa < sb;
    return a.arg_index < b.arg_index;
  });
  std::stable_sort(m.targets.begin(), m.targets.end(), [&](const TargetFact& a, const TargetFact& b) {
    const uint32_t sa = R.call_site.count(a.call_key) ? R.call_site[a.call_key] : kNone;
    const uint32_t sb = R.call_site.count(b.call_key) ? R.call_site[b.call_key] : kNone;
    if (sa != sb) return sa < sb;
    if (a.kind != b.kind) return a.kind < b.kind;
    return a.fn_key < b.fn_key;
  });
  std::stable_sort(m.inits.begin(), m.inits.end(), [](const InitFact& a, const InitFact& b) {
    if (std::tie(a.loc_type, a.loc_field, a.object) != std::tie(b.loc_type, b.loc_field, b.object))
      return std::tie(a.loc_type, a.loc_field, a.object) < std::tie(b.loc_type, b.loc_field, b.object);
    if (!(a.pos == b.pos)) return a.pos < b.pos;
    return std::tie(a.source, a.value) < std::tie(b.source, b.value);
  });
  std::stable_sort(m.consts.begin(), m.consts.end(), [](const ConstFact& a, const ConstFact& b) {
    if (!(a.pos == b.pos)) return a.pos < b.pos;
    return a.name < b.name;
  });
  std::stable_sort(m.images.begin(), m.images.end(), [](const ImageFact& a, const ImageFact& b) {
    if (a.object != b.object) return a.object < b.object;
    return a.path < b.path;
  });
  std::stable_sort(m.locs.begin(), m.locs.end(), [](const LocFact& a, const LocFact& b) { return std::tie(a.loc_type, a.loc_field) < std::tie(b.loc_type, b.loc_field); });

  // ---- the extended facts F1-F3 (spec.extended_facts; empty otherwise): order, and the constants each
  // function returns (RET_CONSTS: per function and constant, the disjunction of the conditions of its rows)
  std::stable_sort(m.reads.begin(), m.reads.end(), [](const ReadFact& a, const ReadFact& b) {
    if (!(a.pos == b.pos)) return a.pos < b.pos;
    if (a.func_key != b.func_key) return a.func_key < b.func_key;
    return a.ordinal < b.ordinal;
  });
  std::stable_sort(m.returns.begin(), m.returns.end(), [](const ReturnFact& a, const ReturnFact& b) {
    if (!(a.pos == b.pos)) return a.pos < b.pos;
    if (a.func_key != b.func_key) return a.func_key < b.func_key;
    if (a.ordinal != b.ordinal) return a.ordinal < b.ordinal;
    return a.alt < b.alt;
  });
  {
    std::map<std::string, bool> only_constants;
    for (const ReturnFact& r : m.returns) {
      auto it = only_constants.emplace(r.func_key, true).first;
      if (r.vkind != KZ_RET_VALUE_CONST) it->second = false;
    }
    struct Acc {
      RetConstFact f;
      std::vector<ExprRef> conds;
      std::set<ExprRef> seen;  // lookup only
      bool always = false;
    };
    std::map<std::pair<std::string, int64_t>, Acc> acc;  // ordered by (function key, value)
    for (const ReturnFact& r : m.returns) {
      if (r.vkind != KZ_RET_VALUE_CONST) continue;
      Acc& a = acc[{r.func_key, r.constant}];
      if (a.f.n_rows == 0) {
        a.f.func_key = r.func_key;
        a.f.value = r.constant;
        a.f.text = r.text;
      }
      ++a.f.n_rows;
      a.f.flags |= r.flags;
      if (r.cond == nullptr) a.always = true;
      else if (a.seen.insert(r.cond).second) a.conds.push_back(r.cond);
    }
    for (auto& kv : acc) {
      Acc& a = kv.second;
      a.f.cond = a.always ? nullptr : m.exprs.conj(KZ_EXPR_OP_OR, a.conds);
      if (only_constants[a.f.func_key]) a.f.flags |= KZ_RET_FLAG_ONLY_CONSTANTS;
      m.ret_consts.push_back(a.f);
    }
  }

  // ---- the extended facts F4 and F6 (spec.extended_facts; empty otherwise): ARGFLOW, PARAM_FLOW, PTR_FIELDS,
  // GLOBAL_IMAGES (argflow.hpp)
  if (spec.extended_facts) finalize_flow_facts(&m, spec);
}

}  // namespace kzs

// argflow.cpp -- see argflow.hpp.
#include "argflow.hpp"

#include <algorithm>
#include <tuple>

namespace kzs {

namespace {

std::string key_of(const CallFact& c) { return c.pos.str() + "#" + std::to_string(c.ordinal); }

// The program as the composition reads it.
struct Prog {
  explicit Prog(const Model& model) : m(model) {}
  const Model& m;
  std::map<std::string, size_t> call_index;                  // call key -> index in m.calls
  std::map<std::string, std::vector<std::string>> itargets;  // indirect call key -> resolved targets, sorted
  std::set<std::string> iunresolved;                         // indirect calls with targets that are not all known
  std::set<std::string> spawn_entries;                       // functions a spawned thread starts at
  std::map<std::string, std::vector<std::pair<std::string, std::string>>> callers;  // callee -> (caller, call key)

  void build() {
    for (size_t i = 0; i < m.calls.size(); ++i) call_index[key_of(m.calls[i])] = i;
    for (const TargetFact& t : m.targets) {
      if (t.kind == KZ_CALL_TARGET_KIND_SPAWN_ENTRY) {
        if (!t.fn_key.empty()) spawn_entries.insert(t.fn_key);
        continue;
      }
      if (t.kind != KZ_CALL_TARGET_KIND_INDIRECT) continue;
      if (t.fn_key.empty()) iunresolved.insert(t.call_key);
      else itargets[t.call_key].push_back(t.fn_key);
    }
    for (auto& kv : itargets) {
      std::sort(kv.second.begin(), kv.second.end());
      kv.second.erase(std::unique(kv.second.begin(), kv.second.end()), kv.second.end());
    }
    for (const CallFact& c : m.calls) {
      const std::string ck = key_of(c);
      if (!c.callee_key.empty()) {
        callers[c.callee_key].push_back({c.caller_key, ck});
        continue;
      }
      auto it = itargets.find(ck);
      if (it != itargets.end())
        for (const std::string& t : it->second) callers[t].push_back({c.caller_key, ck});
    }
    for (auto& kv : callers) std::sort(kv.second.begin(), kv.second.end());
  }
  bool body(const std::string& k) const { return m.summaries.count(k) != 0; }
  uint32_t nparams(const std::string& k) const {
    auto it = m.funcs.find(k);
    return it == m.funcs.end() ? 0u : it->second.nparams;
  }
  std::string name(const std::string& k) const {
    auto it = m.funcs.find(k);
    return it == m.funcs.end() ? k : it->second.name;
  }
};

// ---- PARAM_FLOW ----------------------------------------------------------------------------------------

bool is_sink(const ParamFlowFact& r) {
  if (r.kind != KZ_PFLOW_KIND_CALL_ARG) return true;
  return (r.flags & (KZ_PFLOW_FLAG_NO_BODY | KZ_PFLOW_FLAG_UNRESOLVED | KZ_PFLOW_FLAG_VARIADIC)) != 0;
}

class Composer {
 public:
  Composer(const Prog& p, const ExtractSpec& spec, Model* m) : p_(p), spec_(spec), m_(*m) {}

  void run() {
    collect_direct();
    nodes_and_edges();
    compose();
    emit();
  }

 private:
  struct Edge {
    uint32_t to = 0;
    bool may = false;
    std::string call_key;
  };
  struct Entry {
    uint32_t depth = 0;
    int32_t edge = -1;  // -1: a use of depth 0
    bool may = false;
  };
  static bool same(const std::map<uint32_t, Entry>& a, const std::map<uint32_t, Entry>& b) {
    if (a.size() != b.size()) return false;
    for (auto ia = a.begin(), ib = b.begin(); ia != a.end(); ++ia, ++ib)
      if (ia->first != ib->first || ia->second.depth != ib->second.depth || ia->second.edge != ib->second.edge || ia->second.may != ib->second.may) return false;
    return true;
  }
  static bool better(const Entry& a, const Entry& b) {
    if (a.depth != b.depth) return a.depth < b.depth;
    if (a.may != b.may) return !a.may;
    return a.edge < b.edge;
  }

  // the uses of depth 0, sorted
  void collect_direct() {
    for (const ArgFlowFact& a : m_.argflows) {
      if (a.src != KZ_ARGSRC_PARAM) continue;
      auto cit = p_.call_index.find(a.call_key);
      if (cit == p_.call_index.end()) continue;
      const CallFact& c = m_.calls[cit->second];
      ParamFlowFact r;
      r.kind = KZ_PFLOW_KIND_CALL_ARG;
      r.func_key = a.func_key;
      r.param = a.param;
      r.call_key = a.call_key;
      r.sink_key = a.func_key;
      r.pos = c.pos;
      r.guard = a.guard;
      r.text = a.text;
      if (c.flags & KZ_CALL_FLAG_RESULT_TESTED) r.flags |= KZ_PFLOW_FLAG_RESULT_TESTED;
      if (c.flags & KZ_CALL_FLAG_IN_LOOP) r.flags |= KZ_PFLOW_FLAG_IN_LOOP;
      const uint32_t j = a.arg_index;
      auto add_target = [&](const std::string& t, uint16_t extra) {
        ParamFlowFact x = r;
        x.flags |= extra;
        x.next_key = t;
        x.next_param = j;
        if ((a.flags & KZ_ARGFLOW_FLAG_VARIADIC) || (p_.body(t) && j >= p_.nparams(t))) x.flags |= KZ_PFLOW_FLAG_VARIADIC;
        else if (!p_.body(t)) x.flags |= KZ_PFLOW_FLAG_NO_BODY;
        direct_.push_back(x);
      };
      if (!c.callee_key.empty()) {
        add_target(c.callee_key, 0);
        continue;
      }
      auto tit = p_.itargets.find(a.call_key);
      if (tit != p_.itargets.end())
        for (const std::string& t : tit->second) add_target(t, KZ_PFLOW_FLAG_INDIRECT | KZ_PFLOW_FLAG_MAY);
      if (tit == p_.itargets.end() || p_.iunresolved.count(a.call_key) != 0) {
        ParamFlowFact x = r;
        x.flags |= KZ_PFLOW_FLAG_INDIRECT | KZ_PFLOW_FLAG_UNRESOLVED;
        x.next_param = j;
        direct_.push_back(x);
      }
    }
    for (const ParamUse& u : m_.param_uses) {
      ParamFlowFact r;
      r.kind = u.kind;
      r.func_key = u.func_key;
      r.sink_key = u.func_key;
      r.param = u.param;
      r.flags = u.flags;
      r.pos = u.pos;
      r.n = u.ordinal;  // the order among the uses at one position (cleared before the rows are written)
      r.expr = u.expr;
      r.guard = u.guard;
      r.text = u.text;
      direct_.push_back(r);
    }
    for (const auto& kv : m_.summaries) {
      for (size_t k = 0; k < kv.second.params.size(); ++k) {
        const ParamSummary& ps = kv.second.params[k];
        if (!ps.is_pointer) continue;
        for (const OutStore& st : ps.stores) {
          if (!st.via_func_key.empty()) continue;
          ParamFlowFact r;
          r.kind = KZ_PFLOW_KIND_OUT_STORE;
          r.func_key = kv.first;
          r.sink_key = kv.first;
          r.param = static_cast<uint32_t>(k);
          r.pos = st.pos;
          r.n = st.ordinal;
          r.guard = st.guard;
          r.text = st.value_text;
          if (st.value_kind == KZ_ARG_KIND_CONST) r.expr = m_.exprs.constant(st.value);
          direct_.push_back(r);
        }
      }
    }
    std::stable_sort(direct_.begin(), direct_.end(), [](const ParamFlowFact& a, const ParamFlowFact& b) {
      if (a.func_key != b.func_key) return a.func_key < b.func_key;
      if (a.param != b.param) return a.param < b.param;
      if (!(a.pos == b.pos)) return a.pos < b.pos;
      return std::tie(a.kind, a.n, a.call_key, a.next_key, a.flags) < std::tie(b.kind, b.n, b.call_key, b.next_key, b.flags);
    });
  }

  uint32_t node(const std::string& fn, uint32_t k) {
    auto it = node_id_.find({fn, k});
    if (it != node_id_.end()) return it->second;
    const uint32_t id = static_cast<uint32_t>(nodes_.size());
    nodes_.push_back({fn, k});
    node_id_[{fn, k}] = id;
    return id;
  }

  void nodes_and_edges() {
    // every (function, parameter) with a use, and the callee parameters the calls reach
    for (const ParamFlowFact& r : direct_) {
      node(r.func_key, r.param);
      if (!is_sink(r)) node(r.next_key, r.next_param);
    }
    // ids in sorted order, so that the composition does not depend on the order of the rows
    std::vector<std::pair<std::string, uint32_t>> sorted = nodes_;
    std::sort(sorted.begin(), sorted.end());
    nodes_ = sorted;
    node_id_.clear();
    for (uint32_t i = 0; i < nodes_.size(); ++i) node_id_[nodes_[i]] = i;
    sinks_.assign(nodes_.size(), {});
    edges_.assign(nodes_.size(), {});
    for (size_t i = 0; i < direct_.size(); ++i) {
      const ParamFlowFact& r = direct_[i];
      const uint32_t n = node_id_.at({r.func_key, r.param});
      if (is_sink(r)) {
        sinks_[n].push_back(static_cast<uint32_t>(i));
      } else {
        Edge e;
        e.to = node_id_.at({r.next_key, r.next_param});
        e.may = (r.flags & KZ_PFLOW_FLAG_MAY) != 0;
        e.call_key = r.call_key;
        edges_[n].push_back(e);
      }
    }
    m_.flow_stats.nodes = static_cast<uint32_t>(nodes_.size());
  }

  // Tarjan over the functions that have nodes; completion order is callees first.
  std::vector<std::vector<std::string>> function_sccs() const {
    std::map<std::string, std::set<std::string>> cg;
    std::set<std::string> fns;
    for (uint32_t n = 0; n < nodes_.size(); ++n) {
      fns.insert(nodes_[n].first);
      for (const Edge& e : edges_[n]) cg[nodes_[n].first].insert(nodes_[e.to].first);
    }
    std::vector<std::vector<std::string>> out;
    std::map<std::string, int> index, low;
    std::set<std::string> on;
    std::vector<std::string> stack;
    int counter = 0;
    struct Frame {
      std::string v;
      std::vector<std::string> succ;
      size_t i = 0;
    };
    for (const std::string& root : fns) {
      if (index.count(root) != 0) continue;
      std::vector<Frame> frames;
      auto push = [&](const std::string& v) {
        index[v] = low[v] = counter++;
        stack.push_back(v);
        on.insert(v);
        Frame f;
        f.v = v;
        auto it = cg.find(v);
        if (it != cg.end()) f.succ.assign(it->second.begin(), it->second.end());
        frames.push_back(f);
      };
      push(root);
      while (!frames.empty()) {
        Frame& fr = frames.back();
        if (fr.i < fr.succ.size()) {
          const std::string w = fr.succ[fr.i++];
          if (index.count(w) == 0) push(w);
          else if (on.count(w) != 0) low[fr.v] = std::min(low[fr.v], index[w]);
          continue;
        }
        const std::string v = fr.v;
        frames.pop_back();
        if (!frames.empty()) low[frames.back().v] = std::min(low[frames.back().v], low[v]);
        if (low[v] == index[v]) {
          std::vector<std::string> scc;
          for (;;) {
            const std::string w = stack.back();
            stack.pop_back();
            on.erase(w);
            scc.push_back(w);
            if (w == v) break;
          }
          std::sort(scc.begin(), scc.end());
          out.push_back(scc);
        }
      }
    }
    return out;
  }

  void compose() {
    sum_.assign(nodes_.size(), {});
    rejected_.assign(nodes_.size(), {});
    for (uint32_t n = 0; n < nodes_.size(); ++n)
      for (uint32_t s : sinks_[n]) sum_[n][s] = Entry{0, -1, false};
    std::map<std::string, std::vector<uint32_t>> nodes_of;
    for (uint32_t n = 0; n < nodes_.size(); ++n) nodes_of[nodes_[n].first].push_back(n);
    const std::vector<std::vector<std::string>> sccs = function_sccs();
    m_.flow_stats.sccs = static_cast<uint32_t>(sccs.size());
    const uint32_t D = spec_.argflow_depth, S = spec_.argflow_size;
    for (const std::vector<std::string>& scc : sccs) {
      std::vector<uint32_t> members;
      std::set<std::string> in_scc(scc.begin(), scc.end());
      for (const std::string& f : scc) {
        auto it = nodes_of.find(f);
        if (it != nodes_of.end()) members.insert(members.end(), it->second.begin(), it->second.end());
      }
      std::sort(members.begin(), members.end());
      bool recursive = scc.size() > 1;
      for (uint32_t n : members)
        for (const Edge& e : edges_[n])
          if (in_scc.count(nodes_[e.to].first) != 0) recursive = true;
      if (recursive) ++m_.flow_stats.sccs_recursive;
      for (bool changed = true; changed;) {
        changed = false;
        for (uint32_t n : members) {
          // within a recursive component a pass changes something when a node's entries differ after it
          std::map<uint32_t, Entry> before;
          if (recursive) before = sum_[n];
          for (size_t ei = 0; ei < edges_[n].size(); ++ei) {
            const Edge& e = edges_[n][ei];
            // a snapshot: within the component the target may be this very node
            const std::vector<std::pair<uint32_t, Entry>> from(sum_[e.to].begin(), sum_[e.to].end());
            for (const auto& se : from) {
              Entry cand{se.second.depth + 1, static_cast<int32_t>(ei), e.may || se.second.may};
              if (D != kNone && cand.depth > D) {
                rejected_[n].insert(se.first);
                continue;
              }
              auto it = sum_[n].find(se.first);
              if (it == sum_[n].end()) sum_[n][se.first] = cand;
              else if (better(cand, it->second)) it->second = cand;
            }
          }
          if (S != kNone) truncate(n, S);
          if (recursive && !same(before, sum_[n])) changed = true;
        }
        if (!recursive) break;  // no edge back into the component: one pass is the fixed point
      }
    }
  }
  // Keeps the S best composed entries of a node (by depth, then must before may, then use); the others
  // are left out and counted.
  void truncate(uint32_t n, uint32_t S) {
    std::vector<std::pair<Entry, uint32_t>> composed;
    for (const auto& kv : sum_[n])
      if (kv.second.depth > 0) composed.push_back({kv.second, kv.first});
    if (composed.size() <= S) return;
    std::stable_sort(composed.begin(), composed.end(), [](const std::pair<Entry, uint32_t>& a, const std::pair<Entry, uint32_t>& b) {
      if (a.first.depth != b.first.depth) return a.first.depth < b.first.depth;
      if (a.first.may != b.first.may) return !a.first.may;
      return a.second < b.second;
    });
    for (size_t i = S; i < composed.size(); ++i) {
      dropped_[n].insert(composed[i].second);
      sum_[n].erase(composed[i].second);
    }
  }

  std::string chain(uint32_t n, uint32_t s) const {
    std::string out = p_.name(nodes_[n].first) + "#" + std::to_string(nodes_[n].second);
    uint32_t cur = n;
    std::set<uint32_t> seen{n};  // a summary chain does not come back to a node; if it did, the text stops there
    for (;;) {
      auto it = sum_[cur].find(s);
      if (it == sum_[cur].end()) {
        out += " -> ...";
        break;
      }
      if (it->second.edge < 0) break;
      const uint32_t next = edges_[cur][static_cast<size_t>(it->second.edge)].to;
      out += " -> " + p_.name(nodes_[next].first) + "#" + std::to_string(nodes_[next].second);
      if (!seen.insert(next).second) {
        out += " (again)";
        break;
      }
      cur = next;
    }
    return out;
  }

  void emit() {
    std::vector<ParamFlowFact>& out = m_.param_flows;
    for (ParamFlowFact r : direct_) {
      if (r.kind != KZ_PFLOW_KIND_CALL_ARG) r.n = 0;
      out.push_back(r);
      ++m_.flow_stats.direct;
    }
    for (uint32_t n = 0; n < nodes_.size(); ++n) {
      for (const auto& kv : sum_[n]) {
        const Entry& en = kv.second;
        if (en.depth == 0) continue;
        const ParamFlowFact& sk = direct_[kv.first];
        const Edge& e = edges_[n][static_cast<size_t>(en.edge)];
        ParamFlowFact r = sk;
        r.func_key = nodes_[n].first;
        r.param = nodes_[n].second;
        r.depth = en.depth;
        r.call_key = e.call_key;
        r.next_key = nodes_[e.to].first;
        r.next_param = nodes_[e.to].second;
        r.sink_key = sk.func_key;
        r.sink_call_key = sk.kind == KZ_PFLOW_KIND_CALL_ARG ? sk.call_key : std::string();
        r.n = sk.kind == KZ_PFLOW_KIND_CALL_ARG ? sk.next_param : 0;
        if (en.may) r.flags |= KZ_PFLOW_FLAG_MAY;
        r.chain = chain(n, kv.first);
        out.push_back(r);
        ++m_.flow_stats.composed;
      }
      uint64_t cut_d = 0, cut_s = 0;
      for (uint32_t s : rejected_[n])
        if (sum_[n].count(s) == 0) ++cut_d;
      auto dit = dropped_.find(n);
      if (dit != dropped_.end())
        for (uint32_t s : dit->second)
          if (sum_[n].count(s) == 0 && rejected_[n].count(s) == 0) ++cut_s;
      for (int which = 0; which < 2; ++which) {
        const uint64_t cnt = which == 0 ? cut_d : cut_s;
        if (cnt == 0) continue;
        ParamFlowFact r;
        r.kind = KZ_PFLOW_KIND_OPEN;
        r.func_key = nodes_[n].first;
        r.param = nodes_[n].second;
        r.sink_key = r.func_key;
        r.depth = which == 0 ? spec_.argflow_depth : 0;
        r.flags = which == 0 ? KZ_PFLOW_FLAG_DEPTH_BOUND : KZ_PFLOW_FLAG_SIZE_BOUND;
        r.n = static_cast<uint32_t>(std::min<uint64_t>(cnt, 0xFFFFFFFEu));
        r.text = which == 0 ? "uses beyond the depth bound" : "uses beyond the size bound";
        out.push_back(r);
        (which == 0 ? m_.flow_stats.cut_depth : m_.flow_stats.cut_size) += 1;
        m_.flow_stats.left_out += cnt;
      }
    }
    std::stable_sort(out.begin(), out.end(), [](const ParamFlowFact& a, const ParamFlowFact& b) {
      if (a.func_key != b.func_key) return a.func_key < b.func_key;
      if (a.param != b.param) return a.param < b.param;
      if (a.kind == KZ_PFLOW_KIND_OPEN || b.kind == KZ_PFLOW_KIND_OPEN) {
        if ((a.kind == KZ_PFLOW_KIND_OPEN) != (b.kind == KZ_PFLOW_KIND_OPEN)) return b.kind == KZ_PFLOW_KIND_OPEN;
        return a.flags < b.flags;
      }
      if (a.depth != b.depth) return a.depth < b.depth;
      if (a.sink_key != b.sink_key) return a.sink_key < b.sink_key;
      if (!(a.pos == b.pos)) return a.pos < b.pos;
      return std::tie(a.kind, a.call_key, a.next_key, a.next_param, a.sink_call_key, a.n, a.text) < std::tie(b.kind, b.call_key, b.next_key, b.next_param, b.sink_call_key, b.n, b.text);
    });
  }

  const Prog& p_;
  const ExtractSpec& spec_;
  Model& m_;
  std::vector<ParamFlowFact> direct_;
  std::vector<std::pair<std::string, uint32_t>> nodes_;
  std::map<std::pair<std::string, uint32_t>, uint32_t> node_id_;
  std::vector<std::vector<uint32_t>> sinks_;  // node -> direct_ rows that are uses
  std::vector<std::vector<Edge>> edges_;      // node -> calls of functions with a body
  std::vector<std::map<uint32_t, Entry>> sum_;  // node -> use (direct_ row) -> how it is reached
  std::vector<std::set<uint32_t>> rejected_;    // node -> uses offered beyond the depth bound
  std::map<uint32_t, std::set<uint32_t>> dropped_;  // node -> uses left out by the size bound
};

// ---- PTR_FIELDS ----------------------------------------------------------------------------------------

class PtrFields {
 public:
  PtrFields(const Prog& p, Model* m) : p_(p), m_(*m) {}

  void run() {
    writes();
    others();
    reads();
  }

 private:
  void writes() {
    for (const PtrWriteFact& w : m_.ptr_writes) all_.push_back(w);
    // the static initializers: a leaf of a global's image that is a record field holding the address of a global
    for (const auto& kv : m_.object_images) {
      for (const ImageFact& f : kv.second) {
        if (f.kind != KZ_IMAGE_KIND_ADDR || f.owner.empty() || f.field.empty()) continue;
        if (f.text.find('+') != std::string::npos) continue;  // an address inside an object: not the start of an element
        PtrWriteFact w;
        w.kind = KZ_PTRF_KIND_STATIC_INIT;
        w.type = f.owner;
        w.field = f.field;
        w.root = kv.first;
        w.root_global = true;
        w.object = f.text;
        w.text = f.text;
        w.pos = f.pos;
        all_.push_back(w);
      }
    }
    std::stable_sort(all_.begin(), all_.end(), [](const PtrWriteFact& a, const PtrWriteFact& b) {
      return std::tie(a.kind, a.type, a.field, a.func_key, a.pos, a.ordinal, a.root) < std::tie(b.kind, b.type, b.field, b.func_key, b.pos, b.ordinal, b.root);
    });
  }
  // the other typed writes of each write's field that remain
  void others() {
    remaining_.assign(all_.size(), 0);
    for (size_t i = 0; i < all_.size(); ++i) {
      const PtrWriteFact& w = all_[i];
      uint32_t n = 0;
      std::set<std::tuple<std::string, std::string, uint8_t>> seen;  // one write per (function, position, kind)
      for (const WriteEvent& e : m_.write_events) {
        const bool same = ((e.kind == KZ_FWRITE_KIND_FIELD || e.kind == KZ_FWRITE_KIND_OUTARG) && e.type == w.type && e.field == w.field) ||
                          (e.kind == KZ_FWRITE_KIND_RECORD && e.type == w.type && e.field == "*");
        if (!same) continue;
        if (w.kind == KZ_PTRF_KIND_ADDRESS_WRITE && e.func_key == w.func_key) {
          if (e.kind == KZ_FWRITE_KIND_FIELD && e.pos == w.pos) continue;  // the write itself
          if (w.overwritten.count(e.pos.str()) != 0) continue;
        }
        if (seen.insert({e.func_key, e.pos.str(), e.kind}).second) ++n;
      }
      remaining_[i] = n;
    }
  }
  bool ordered(const PtrWriteFact& w, const ReadFact& r) const {
    if (w.kind == KZ_PTRF_KIND_STATIC_INIT) return true;
    if (r.func_key == w.func_key) return w.after_reads.count(r.key()) != 0;
    std::set<std::string> visited{r.func_key};
    std::vector<std::string> work{r.func_key};
    while (!work.empty()) {
      const std::string x = work.back();
      work.pop_back();
      if (p_.spawn_entries.count(x) != 0) return false;  // a thread starts there
      auto fit = m_.funcs.find(x);
      if (fit != m_.funcs.end() && (fit->second.flags & KZ_FUNC_FLAG_ADDRESS_TAKEN) && !p_.iunresolved.empty()) return false;
      auto it = p_.callers.find(x);
      if (it == p_.callers.end() || it->second.empty()) return false;  // reached from a root without the writer
      for (const auto& yc : it->second) {
        if (yc.first == w.func_key) {
          if (w.after_calls.count(yc.second) == 0) return false;
          continue;
        }
        if (visited.insert(yc.first).second) work.push_back(yc.first);
      }
    }
    return true;
  }
  void reads() {
    std::vector<PtrFieldFact>& out = m_.ptr_fields;
    for (size_t i = 0; i < all_.size(); ++i) {
      const PtrWriteFact& w = all_[i];
      PtrFieldFact f;
      f.kind = w.kind;
      f.func_key = w.func_key;
      f.type = w.type;
      f.field = w.field;
      f.root = w.root;
      f.object = w.object;
      f.path = w.text;
      f.elem = w.elem;
      f.n = remaining_[i];
      f.guard = w.guard;
      f.pos = w.pos;
      if (w.in_loop) f.flags |= KZ_PTRF_FLAG_IN_LOOP;
      if (!w.root_global) f.flags |= KZ_PTRF_FLAG_FIELD_BASED;
      if (!w.overwritten.empty()) f.flags |= KZ_PTRF_FLAG_EARLIER_OVERWRITTEN;
      if (remaining_[i] > 0) f.flags |= KZ_PTRF_FLAG_OTHER_WRITES;
      out.push_back(f);
    }
    for (const ReadFact& r : m_.reads) {
      if (r.ptr_field.empty()) continue;
      PtrFieldFact f;
      f.kind = KZ_PTRF_KIND_READ;
      f.func_key = r.func_key;
      f.read_key = r.key();
      f.type = r.ptr_type;
      f.field = r.ptr_field;
      f.root = r.root_object;
      f.path = r.ptr_rest;
      f.index = r.ptr_index;
      f.pos = r.pos;
      if (r.ptr_steps > 1) f.flags |= KZ_PTRF_FLAG_MULTI_LEVEL;
      std::vector<size_t> cands;
      bool same_root = false;
      for (size_t i = 0; i < all_.size(); ++i) {
        const PtrWriteFact& w = all_[i];
        if (w.type != r.ptr_type || w.field != r.ptr_field) continue;
        if (w.root_global && !r.root_object.empty() && r.root_object != w.root) continue;  // another object of the type
        cands.push_back(i);
      }
      f.n = static_cast<uint32_t>(cands.size());
      if (cands.empty()) {
        f.status = KZ_PTRF_STATUS_UNRESOLVED;
        out.push_back(f);
        continue;
      }
      const size_t wi = cands.front();
      const PtrWriteFact& w = all_[wi];
      f.write = static_cast<int64_t>(wi);
      f.object = w.object;
      same_root = w.root_global && !r.root_object.empty() && r.root_object == w.root;
      if (same_root) f.flags |= KZ_PTRF_FLAG_SAME_ROOT;
      else f.flags |= KZ_PTRF_FLAG_FIELD_BASED;
      const bool ord = ordered(w, r);
      if (ord) f.flags |= KZ_PTRF_FLAG_WRITE_BEFORE_READ;
      if (remaining_[wi] > 0) f.flags |= KZ_PTRF_FLAG_OTHER_WRITES;
      f.status = (cands.size() == 1 && same_root && ord && remaining_[wi] == 0) ? KZ_PTRF_STATUS_RESOLVED : KZ_PTRF_STATUS_MAY;
      // the elements read: the subscript of the step
      if (r.ptr_index == nullptr || r.ptr_index_is_const) {
        f.elem = w.elem + (r.ptr_index_is_const ? r.ptr_index_const : 0);
        f.idx_n = 1;
        f.flags |= KZ_PTRF_FLAG_CONST_INDEX;
      } else if (!r.ptr_index_local.empty()) {
        for (std::string cur = r.loop_key; !cur.empty();) {
          auto lit = m_.resolved.loop_index.find(cur);
          if (lit == m_.resolved.loop_index.end()) break;
          const LoopFact& lf = m_.loops[lit->second];
          if (lf.induction == r.ptr_index_local && lf.step != 0 && (lf.trip_status == KZ_TRIP_STATUS_EXACT || lf.trip_status == KZ_TRIP_STATUS_UPPER_BOUND)) {
            f.elem = w.elem + lf.c0;
            f.idx_step = lf.step;
            f.idx_n = static_cast<uint32_t>(std::max<int64_t>(0, lf.trip));
            f.flags |= KZ_PTRF_FLAG_INDEX_RANGE;
            if (lf.trip_status == KZ_TRIP_STATUS_UPPER_BOUND) f.flags |= KZ_PTRF_FLAG_INDEX_UPPER_BOUND;
            break;
          }
          cur = lf.parent_key;
        }
      }
      out.push_back(f);
    }
  }

  const Prog& p_;
  Model& m_;
  std::vector<PtrWriteFact> all_;
  std::vector<uint32_t> remaining_;
};

// ---- GLOBAL_IMAGES -------------------------------------------------------------------------------------

void global_images(Model* mp) {
  Model& m = *mp;
  std::map<std::string, uint8_t> reach;  // object key -> KZ_GIMAGE_REACH
  for (const ArgFlowFact& a : m.argflows)
    if (!a.object.empty()) reach[a.object] |= KZ_GIMAGE_REACH_ARGUMENT;
  for (const PtrFieldFact& f : m.ptr_fields) {
    if (!f.object.empty()) reach[f.object] |= KZ_GIMAGE_REACH_PTR_FIELD;
    if (f.kind != KZ_PTRF_KIND_READ && !(f.flags & KZ_PTRF_FLAG_FIELD_BASED) && !f.root.empty()) reach[f.root] |= KZ_GIMAGE_REACH_PTR_FIELD;
  }
  std::set<std::string> global_written, record_written;
  std::set<std::pair<std::string, std::string>> field_written;
  for (const WriteEvent& e : m.write_events) {
    if (e.kind == KZ_FWRITE_KIND_RECORD) record_written.insert(e.type);
    if (e.kind != KZ_FWRITE_KIND_FIELD && e.kind != KZ_FWRITE_KIND_OUTARG) continue;
    if (e.type == "<global>") global_written.insert(e.field);
    else field_written.insert({e.type, e.field});
  }
  for (const auto& kv : reach) {
    const std::string& key = kv.first;
    const std::string name = key.substr(0, key.find('@'));
    std::vector<ImageFact> leaves;
    uint16_t obj_flags = 0;
    auto img = m.object_images.find(key);
    auto sc = m.object_scalar_images.find(key);
    if (img != m.object_images.end()) {
      leaves = img->second;
    } else if (sc != m.object_scalar_images.end()) {
      leaves.push_back(sc->second);
    } else {
      auto pit = m.object_pos.find(key);
      if (pit == m.object_pos.end()) {
        ++m.flow_stats.objects_outside;  // not defined in the closed program
        continue;
      }
      ImageFact z;
      z.object = key;
      z.kind = KZ_IMAGE_KIND_INT;
      z.value = 0;
      z.text = "zero (no initializer)";
      z.pos = pit->second;
      leaves.push_back(z);
      obj_flags |= KZ_GIMAGE_FLAG_NO_INITIALIZER;
    }
    ++m.flow_stats.objects_reached;
    auto cit = m.object_const.find(key);
    if (cit != m.object_const.end() && cit->second) obj_flags |= KZ_GIMAGE_FLAG_CONST_OBJECT;
    if (global_written.count(name) != 0) obj_flags |= KZ_GIMAGE_FLAG_OBJECT_WRITTEN;
    for (const ImageFact& f : leaves)
      if (f.text == "<image truncated>") obj_flags |= KZ_GIMAGE_FLAG_TRUNCATED;
    for (const ImageFact& f : leaves) {
      GlobalImageFact g;
      g.leaf = f;
      g.leaf.object = key;
      g.reach = kv.second;
      g.flags = obj_flags;
      if (!f.owner.empty() && field_written.count({f.owner, f.field}) != 0) g.flags |= KZ_GIMAGE_FLAG_FIELD_WRITTEN;
      if (!f.owner.empty() && record_written.count(f.owner) != 0) g.flags |= KZ_GIMAGE_FLAG_RECORD_WRITTEN;
      if (!f.macro.empty()) g.flags |= KZ_GIMAGE_FLAG_FROM_MACRO;
      m.global_images.push_back(g);
    }
  }
}

}  // namespace

void finalize_flow_facts(Model* m, const ExtractSpec& spec) {
  Prog p(*m);
  p.build();
  // ARGFLOW in the order of the calls
  const Model::Resolved& R = m->resolved;
  std::stable_sort(m->argflows.begin(), m->argflows.end(), [&](const ArgFlowFact& a, const ArgFlowFact& b) {
    auto ia = R.call_site.find(a.call_key), ib = R.call_site.find(b.call_key);
    const uint32_t sa = ia == R.call_site.end() ? kNone : ia->second, sb = ib == R.call_site.end() ? kNone : ib->second;
    if (sa != sb) return sa < sb;
    return a.arg_index < b.arg_index;
  });
  Composer c(p, spec, m);
  c.run();
  PtrFields pf(p, m);
  pf.run();
  global_images(m);
}

}  // namespace kzs

// wrappers.cpp -- see wrappers.hpp.
#include "wrappers.hpp"

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <queue>
#include <tuple>

#include "finalize.hpp"
#include "kz/core/registry.hpp"
#include "p0.hpp"
#include "target_rules.hpp"

namespace kzs {

const char* builtin_profile_text() { return "phi 1;\nprofile \"kz-static-builtin-minimal\";\n"; }

kz::Status load_builtin_profile(kz::phi::Profile* out) {
  std::vector<kz::phi::Diag> diags;
  return kz::phi::load_profile(std::vector<kz::phi::SourceText>{{"<built-in profile of --no-profile>", builtin_profile_text()}}, out, &diags);
}

const char* wrapper_rules_revision() { return "wrapper derivation rules F7/F8 revision 1 (2026-10-30)"; }

void spec_add_p0_entries(ExtractSpec* spec) {
  const kz::EnumDesc* kinds = kz::find_enum("KZ_P0_KIND");
  for (const P0Entry& e : p0_table()) {
    if (e.kind == KZ_P0_KIND_DATA_FN) continue;
    const char* k = kinds != nullptr ? kz::enum_display(*kinds, e.kind) : nullptr;
    spec->declared.push_back(DeclaredItem{std::string("p0.") + e.name, std::string(k != nullptr ? k : "?") + "; " + e.version + "; source " + e.source,
                                          "built-in P0 table (hand-entered, tier researcher)"});
  }
}

std::vector<StartupEntry> startup_entries_from_basis(const kz::Container& basis) {
  std::vector<StartupEntry> out;
  std::vector<kz_basis_item> items;
  if (!basis.read_section(KZ_SEC_BASIS, &items).ok()) return out;
  const std::string prefix = "startup.entry[";
  for (const kz_basis_item& b : items) {
    std::string name, value;
    basis.string(b.name, &name);
    basis.string(b.value, &value);
    if (name.compare(0, prefix.size(), prefix) != 0 || name.back() != ']') continue;
    StartupEntry e;
    e.index = static_cast<uint32_t>(std::strtoul(name.c_str() + prefix.size(), nullptr, 10));
    e.text = value;
    const size_t sp = value.find(' ');
    e.kind = value.substr(0, sp);
    // "<kind> <file>, <symbol>, <name>, ...": the second comma-separated field
    const size_t first = value.find(',');
    if (first != std::string::npos) {
      const size_t second = value.find(',', first + 1);
      std::string sym = value.substr(first + 1, second == std::string::npos ? std::string::npos : second - first - 1);
      while (!sym.empty() && sym.front() == ' ') sym.erase(sym.begin());
      while (!sym.empty() && sym.back() == ' ') sym.pop_back();
      e.symbol = sym;
    }
    out.push_back(e);
  }
  std::sort(out.begin(), out.end(), [](const StartupEntry& a, const StartupEntry& b) { return a.index < b.index; });
  return out;
}

namespace {

constexpr uint32_t kInf = 0xFFFFFFFFu;

std::string key_of(const CallFact& c) { return c.pos.str() + "#" + std::to_string(c.ordinal); }

// The program as the derivation reads it.
struct Prog {
  explicit Prog(const Model& model) : m(model) {}
  const Model& m;
  std::map<std::string, size_t> p0_by_name;
  std::set<std::string> body;                                // keys with a body read
  std::vector<std::string> keys;                             // body keys, sorted
  std::map<std::string, std::vector<size_t>> calls_of;       // function key -> indices of m.calls, by call key
  std::vector<std::string> ckey;                             // parallel to m.calls
  std::map<std::string, size_t> call_index;                  // call key -> index
  std::map<std::string, std::vector<std::string>> itargets;  // indirect call key -> resolved targets
  std::set<std::string> iunresolved;
  std::map<std::string, std::set<std::string>> cg;           // function -> callees and targets with a body

  void build() {
    for (size_t i = 0; i < p0_table().size(); ++i) p0_by_name[p0_table()[i].name] = i;
    body = m.processed_functions;
    keys.assign(body.begin(), body.end());
    ckey.resize(m.calls.size());
    for (size_t i = 0; i < m.calls.size(); ++i) {
      ckey[i] = key_of(m.calls[i]);
      call_index[ckey[i]] = i;
      calls_of[m.calls[i].caller_key].push_back(i);
    }
    for (auto& kv : calls_of) std::sort(kv.second.begin(), kv.second.end(), [&](size_t a, size_t b) { return ckey[a] < ckey[b]; });
    for (const TargetFact& t : m.targets) {
      if (t.kind != KZ_CALL_TARGET_KIND_INDIRECT) continue;
      if (t.fn_key.empty()) iunresolved.insert(t.call_key);
      else itargets[t.call_key].push_back(t.fn_key);
    }
    for (auto& kv : itargets) {
      std::sort(kv.second.begin(), kv.second.end());
      kv.second.erase(std::unique(kv.second.begin(), kv.second.end()), kv.second.end());
    }
    for (size_t i = 0; i < m.calls.size(); ++i) {
      const CallFact& c = m.calls[i];
      if (body.count(c.caller_key) == 0) continue;
      for (const std::string& t : targets(i))
        if (body.count(t) != 0) cg[c.caller_key].insert(t);
    }
  }
  // The P0 entry a callee key is: a function of that name without a body in the program.
  const P0Entry* prim(const std::string& key) const {
    auto it = p0_by_name.find(key);
    if (it == p0_by_name.end() || body.count(key) != 0) return nullptr;
    return &p0_table()[it->second];
  }
  std::vector<std::string> targets(size_t ci) const {
    const CallFact& c = m.calls[ci];
    if (!c.callee_key.empty()) return {c.callee_key};
    auto it = itargets.find(ckey[ci]);
    return it == itargets.end() ? std::vector<std::string>{} : it->second;
  }
  bool unresolved(size_t ci) const { return m.calls[ci].callee_key.empty() && (iunresolved.count(ckey[ci]) != 0 || itargets.count(ckey[ci]) == 0); }
  std::string name(const std::string& key) const {
    auto it = m.funcs.find(key);
    return it != m.funcs.end() ? it->second.name : key;
  }
};

struct FnState {
  bool cfg = false;
  bool returns = true;
  bool term_must = false, term_may = false;
  bool delay_must = false, delay_may = false;
  uint8_t scope = 0;  // 1 thread exit, 2 process exit
};

class F8 {
 public:
  F8(const Prog& p) : p_(p) {}

  void run(Derivation* d) {
    for (const std::string& k : p_.keys) {
      FnState s;
      s.cfg = p_.m.cfgs.count(k) != 0;
      st_[k] = s;
    }
    std::vector<std::vector<std::string>> sccs = tarjan();
    d->sccs = static_cast<uint32_t>(sccs.size());
    for (const auto& scc : sccs) {
      const bool recursive = scc.size() > 1 || (p_.cg.count(scc[0]) != 0 && p_.cg.at(scc[0]).count(scc[0]) != 0);
      if (recursive) ++d->sccs_recursive;
      solve(scc);
    }
  }

  const FnState& state(const std::string& k) const { return st_.at(k); }
  bool has(const std::string& k) const { return st_.count(k) != 0; }

  // ---- per-target and per-call verdicts with the current states
  bool t_returns(const std::string& t) const {
    if (const P0Entry* e = p_.prim(t)) return !p0_is_terminal(e->kind);
    auto it = st_.find(t);
    return it == st_.end() ? true : it->second.returns;
  }
  bool t_term_must(const std::string& t) const {
    if (const P0Entry* e = p_.prim(t)) return p0_is_terminal(e->kind);
    auto it = st_.find(t);
    return it != st_.end() && it->second.term_must;
  }
  bool t_term_any(const std::string& t) const {
    if (const P0Entry* e = p_.prim(t)) return p0_is_terminal(e->kind);
    auto it = st_.find(t);
    return it != st_.end() && it->second.term_may;
  }
  bool t_delay_must(const std::string& t) const {
    if (const P0Entry* e = p_.prim(t)) return p0_is_delay(e->kind);
    auto it = st_.find(t);
    return it != st_.end() && it->second.delay_must;
  }
  bool t_delay_any(const std::string& t) const {
    if (const P0Entry* e = p_.prim(t)) return p0_is_delay(e->kind);
    auto it = st_.find(t);
    return it != st_.end() && it->second.delay_may;
  }
  uint8_t t_scope(const std::string& t) const {
    if (const P0Entry* e = p_.prim(t)) return e->kind == KZ_P0_KIND_THREAD_EXIT ? 1 : e->kind == KZ_P0_KIND_PROCESS_EXIT ? 2 : 0;
    auto it = st_.find(t);
    return it == st_.end() ? 0 : it->second.scope;
  }
  // A call ends the path when its callee does not return (an indirect call: none of its resolved targets does).
  bool c_stop(size_t ci) const {
    const std::vector<std::string> ts = p_.targets(ci);
    if (ts.empty() || p_.unresolved(ci)) return false;
    for (const std::string& t : ts)
      if (t_returns(t)) return false;
    return true;
  }
  template <class Pred>
  bool c_all(size_t ci, Pred pred) const {
    const std::vector<std::string> ts = p_.targets(ci);
    if (ts.empty() || p_.unresolved(ci)) return false;
    for (const std::string& t : ts)
      if (!pred(t)) return false;
    return true;
  }
  template <class Pred>
  bool c_any(size_t ci, Pred pred) const {
    for (const std::string& t : p_.targets(ci))
      if (pred(t)) return true;
    return false;
  }
  bool c_term_must(size_t ci) const { return c_all(ci, [&](const std::string& t) { return t_term_must(t); }); }
  bool c_term_any(size_t ci) const { return c_any(ci, [&](const std::string& t) { return t_term_any(t); }); }
  bool c_delay_must(size_t ci) const { return c_all(ci, [&](const std::string& t) { return t_delay_must(t); }); }
  bool c_delay_any(size_t ci) const { return c_any(ci, [&](const std::string& t) { return t_delay_any(t); }); }

  // The blocks reachable from the entry when a block's continuation ends at the first call for
  // which `ends` holds, or at a call of a function declared not to return.  end_at: -1 the block
  // continues, -2 it ends without such a call (declared not to return), k the call index in it.
  struct Walk {
    std::vector<char> seen;
    std::vector<int> end_at;
  };
  template <class Ends>
  Walk walk(const FuncCfg& g, Ends ends) const {
    Walk w;
    w.seen.assign(g.blocks.size(), 0);
    w.end_at.assign(g.blocks.size(), -1);
    if (g.entry >= g.blocks.size()) return w;
    std::vector<uint32_t> stack{g.entry};
    w.seen[g.entry] = 1;
    while (!stack.empty()) {
      const uint32_t b = stack.back();
      stack.pop_back();
      const CfgBlock& blk = g.blocks[b];
      int end = -1;
      for (size_t i = 0; i < blk.calls.size() && end < 0; ++i) {
        auto it = p_.call_index.find(blk.calls[i]);
        if (it != p_.call_index.end() && ends(it->second)) end = static_cast<int>(i);
      }
      if (end < 0 && blk.noreturn) end = -2;
      w.end_at[b] = end;
      if (end != -1) continue;
      for (uint32_t s : blk.succ)
        if (s < g.blocks.size() && !w.seen[s]) {
          w.seen[s] = 1;
          stack.push_back(s);
        }
    }
    return w;
  }
  // The calls of the visited blocks up to the one their continuation ends at.
  template <class Fn>
  void each_reached_call(const FuncCfg& g, const Walk& w, Fn fn) const {
    for (size_t b = 0; b < g.blocks.size(); ++b) {
      if (!w.seen[b]) continue;
      const CfgBlock& blk = g.blocks[b];
      const size_t n = w.end_at[b] >= 0 ? static_cast<size_t>(w.end_at[b]) + 1 : blk.calls.size();
      for (size_t i = 0; i < n && i < blk.calls.size(); ++i) {
        auto it = p_.call_index.find(blk.calls[i]);
        if (it != p_.call_index.end()) fn(it->second);
      }
    }
  }
  // The calls of a function: those its control-flow graph reaches, or every call when it has none.
  template <class Fn>
  void each_step_candidate(const std::string& f, Fn fn) const {
    auto git = p_.m.cfgs.find(f);
    if (git == p_.m.cfgs.end()) {
      auto cit = p_.calls_of.find(f);
      if (cit != p_.calls_of.end())
        for (size_t ci : cit->second) fn(ci);
      return;
    }
    const Walk w = walk(git->second, [&](size_t ci) { return c_stop(ci); });
    each_reached_call(git->second, w, fn);
  }

 private:
  std::vector<std::vector<std::string>> tarjan() const {
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
    for (const std::string& root : p_.keys) {
      if (index.count(root) != 0) continue;
      std::vector<Frame> frames;
      auto push = [&](const std::string& v) {
        index[v] = low[v] = counter++;
        stack.push_back(v);
        on.insert(v);
        Frame f;
        f.v = v;
        auto it = p_.cg.find(v);
        if (it != p_.cg.end()) f.succ.assign(it->second.begin(), it->second.end());
        frames.push_back(f);
      };
      push(root);
      while (!frames.empty()) {
        Frame& fr = frames.back();
        if (fr.i < fr.succ.size()) {
          const std::string w = fr.succ[fr.i++];
          if (index.count(w) == 0) {
            push(w);
          } else if (on.count(w) != 0) {
            low[fr.v] = std::min(low[fr.v], index[w]);
          }
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
    return out;  // completion order: callees before callers
  }

  bool has_cycle(const FuncCfg& g, const Walk& w) const {
    // a cycle among the visited blocks whose continuation does not end
    std::vector<int> color(g.blocks.size(), 0);
    for (size_t s = 0; s < g.blocks.size(); ++s) {
      if (!w.seen[s] || w.end_at[s] != -1 || color[s] != 0) continue;
      std::vector<std::pair<uint32_t, size_t>> st{{static_cast<uint32_t>(s), 0}};
      color[s] = 1;
      while (!st.empty()) {
        auto& top = st.back();
        const CfgBlock& blk = g.blocks[top.first];
        if (top.second < blk.succ.size()) {
          const uint32_t n = blk.succ[top.second++];
          if (n >= g.blocks.size() || !w.seen[n] || w.end_at[n] != -1) continue;
          if (color[n] == 1) return true;
          if (color[n] == 0) {
            color[n] = 1;
            st.push_back({n, 0});
          }
          continue;
        }
        color[top.first] = 2;
        st.pop_back();
      }
    }
    return false;
  }

  bool calc_returns(const std::string& f) const {
    auto git = p_.m.cfgs.find(f);
    if (git == p_.m.cfgs.end()) return true;
    const Walk w = walk(git->second, [&](size_t ci) { return c_stop(ci); });
    return git->second.exit < w.seen.size() && w.seen[git->second.exit];
  }
  bool calc_term_must(const std::string& f) const {
    auto git = p_.m.cfgs.find(f);
    if (git == p_.m.cfgs.end()) return false;
    const FuncCfg& g = git->second;
    const Walk w = walk(g, [&](size_t ci) { return c_term_must(ci) || c_stop(ci); });
    if (g.exit < w.seen.size() && w.seen[g.exit]) return false;
    bool covered = false;
    for (size_t b = 0; b < g.blocks.size(); ++b) {
      if (!w.seen[b]) continue;
      if (w.end_at[b] == -2) return false;
      if (w.end_at[b] >= 0) {
        auto it = p_.call_index.find(g.blocks[b].calls[static_cast<size_t>(w.end_at[b])]);
        if (it == p_.call_index.end() || !c_term_must(it->second)) return false;
        covered = true;
      } else if (g.blocks[b].succ.empty()) {
        return false;  // a path that ends here without a terminal call
      }
    }
    return covered && !has_cycle(g, w);
  }
  bool calc_term_may(const std::string& f) const {
    bool any = false;
    each_step_candidate(f, [&](size_t ci) { any = any || c_term_any(ci); });
    return any;
  }
  bool calc_delay_must(const std::string& f) const {
    auto git = p_.m.cfgs.find(f);
    if (git == p_.m.cfgs.end() || !st_.at(f).returns) return false;
    const Walk w = walk(git->second, [&](size_t ci) { return c_delay_must(ci) || c_stop(ci); });
    return !(git->second.exit < w.seen.size() && w.seen[git->second.exit]);
  }
  bool calc_delay_may(const std::string& f) const {
    bool any = false;
    each_step_candidate(f, [&](size_t ci) { any = any || c_delay_any(ci); });
    return any;
  }
  uint8_t calc_scope(const std::string& f) const {
    uint8_t s = 0;
    each_step_candidate(f, [&](size_t ci) {
      if (!c_term_any(ci)) return;
      for (const std::string& t : p_.targets(ci))
        if (t_term_any(t)) s |= t_scope(t);
    });
    return s;
  }

  // One component, callees done: each property to its fixed point within the component.  `returns`
  // starts true and only falls; the others start false and only rise.
  void solve(const std::vector<std::string>& scc) {
    for (bool changed = true; changed;) {
      changed = false;
      for (const std::string& f : scc) {
        const bool v = calc_returns(f);
        if (v != st_[f].returns) {
          st_[f].returns = v;
          changed = true;
        }
      }
    }
    for (bool changed = true; changed;) {
      changed = false;
      for (const std::string& f : scc) {
        const bool v = calc_term_must(f);
        if (v && !st_[f].term_must) {
          st_[f].term_must = true;
          st_[f].term_may = true;
          changed = true;
        }
      }
    }
    for (bool changed = true; changed;) {
      changed = false;
      for (const std::string& f : scc) {
        const bool v = calc_term_may(f);
        if (v && !st_[f].term_may) {
          st_[f].term_may = true;
          changed = true;
        }
      }
    }
    for (bool changed = true; changed;) {
      changed = false;
      for (const std::string& f : scc) {
        const uint8_t v = static_cast<uint8_t>(calc_scope(f) | st_[f].scope);
        if (v != st_[f].scope) {
          st_[f].scope = v;
          changed = true;
        }
      }
    }
    for (bool changed = true; changed;) {
      changed = false;
      for (const std::string& f : scc) {
        const bool v = calc_delay_must(f);
        if (v && !st_[f].delay_must) {
          st_[f].delay_must = true;
          st_[f].delay_may = true;
          changed = true;
        }
      }
    }
    for (bool changed = true; changed;) {
      changed = false;
      for (const std::string& f : scc) {
        const bool v = calc_delay_may(f);
        if (v && !st_[f].delay_may) {
          st_[f].delay_may = true;
          changed = true;
        }
      }
    }
  }

  const Prog& p_;
  std::map<std::string, FnState> st_;
};

uint8_t term_mode(const FnState& s) {
  if (s.term_must) return KZ_OPDERIV_MODE_MUST;
  if (s.term_may && !s.returns) return KZ_OPDERIV_MODE_NORETURN;
  if (s.term_may) return KZ_OPDERIV_MODE_MAY;
  return KZ_OPDERIV_MODE_NONE;
}
uint8_t delay_mode(const FnState& s) {
  if (s.delay_must) return KZ_OPDERIV_MODE_MUST;
  if (s.delay_may) return KZ_OPDERIV_MODE_MAY;
  return KZ_OPDERIV_MODE_NONE;
}

// The rows of one kind: per function with a mode, per call of it that leads to the primitive.
void op_rows(const Prog& p, const F8& f8, uint8_t kind, Derivation* d) {
  const bool term = kind == KZ_OPDERIV_KIND_TERMINAL;
  auto any = [&](const std::string& t) { return term ? f8.t_term_any(t) : f8.t_delay_any(t); };
  auto mode_of = [&](const std::string& f) { return term ? term_mode(f8.state(f)) : delay_mode(f8.state(f)); };
  // steps: function -> (call, targets that lead on)
  std::map<std::string, std::vector<std::pair<size_t, std::vector<std::string>>>> steps;
  std::map<std::string, std::set<std::string>> rev;  // target -> functions with a step to it
  std::set<std::string> prims;
  for (const std::string& f : p.keys) {
    if (mode_of(f) == KZ_OPDERIV_MODE_NONE) continue;
    std::set<size_t> seen;
    std::vector<std::pair<size_t, std::vector<std::string>>> st;
    f8.each_step_candidate(f, [&](size_t ci) {
      if (!seen.insert(ci).second) return;
      std::vector<std::string> on;
      for (const std::string& t : p.targets(ci))
        if (any(t)) on.push_back(t);
      if (on.empty()) return;
      for (const std::string& t : on) {
        rev[t].insert(f);
        if (p.prim(t) != nullptr) prims.insert(t);
      }
      st.push_back({ci, on});
    });
    std::sort(st.begin(), st.end(), [&](const auto& a, const auto& b) { return p.ckey[a.first] < p.ckey[b.first]; });
    steps[f] = st;
  }
  // shortest chains to a primitive: breadth first from the primitives, ties by key
  std::map<std::string, uint32_t> dist;
  std::map<std::string, std::string> parent;
  std::queue<std::string> q;
  for (const std::string& t : prims) {
    dist[t] = 0;
    q.push(t);
  }
  while (!q.empty()) {
    const std::string t = q.front();
    q.pop();
    auto it = rev.find(t);
    if (it == rev.end()) continue;
    for (const std::string& f : it->second) {
      if (dist.count(f) != 0) continue;
      dist[f] = dist[t] + 1;
      parent[f] = t;
      q.push(f);
    }
  }
  std::map<std::string, std::string> chain_memo;
  std::function<std::string(const std::string&)> chain = [&](const std::string& n) -> std::string {
    auto it = chain_memo.find(n);
    if (it != chain_memo.end()) return it->second;
    std::string c = p.name(n);
    auto pit = parent.find(n);
    if (pit != parent.end()) c += " -> " + chain(pit->second);
    return chain_memo[n] = c;
  };
  // the breadth-first parents form a tree rooted at the primitives
  std::function<std::string(const std::string&)> prim_of = [&](const std::string& n) -> std::string {
    std::string cur = n;
    std::set<std::string> seen{cur};
    for (auto pit = parent.find(cur); pit != parent.end() && seen.insert(pit->second).second; pit = parent.find(cur)) cur = pit->second;
    return cur;
  };
  for (const auto& kv : steps) {
    const std::string& f = kv.first;
    const FnState& s = f8.state(f);
    const uint8_t mode = mode_of(f);
    if (term) (mode == KZ_OPDERIV_MODE_MUST ? d->term_must : mode == KZ_OPDERIV_MODE_NORETURN ? d->term_noreturn : d->term_may)++;
    else (mode == KZ_OPDERIV_MODE_MUST ? d->delay_must : d->delay_may)++;
    for (const auto& step : kv.second) {
      std::string next;
      uint32_t best = kInf;
      for (const std::string& t : step.second) {
        auto dit = dist.find(t);
        const uint32_t dt = dit == dist.end() ? kInf : dit->second;
        if (dt < best || (dt == best && (next.empty() || t < next))) {
          best = dt;
          next = t;
        }
      }
      OpDerivFact r;
      r.func_key = f;
      r.call_key = p.ckey[step.first];
      r.next_key = next;
      r.kind = kind;
      r.mode = mode;
      r.depth = best == kInf ? 0 : best + 1;
      r.chain = p.name(f) + " -> " + chain(next);
      r.primitive = p.name(prim_of(next));
      if (p.m.calls[step.first].callee_key.empty()) r.flags |= KZ_OPDERIV_FLAG_INDIRECT;
      if (!s.cfg) r.flags |= KZ_OPDERIV_FLAG_NO_CFG;
      if (term) {
        r.flags |= KZ_OPDERIV_FLAG_VOCABULARY;
        if (s.scope & 1) r.flags |= KZ_OPDERIV_FLAG_THREAD_TERMINAL;
        if (s.scope & 2) r.flags |= KZ_OPDERIV_FLAG_PROCESS_TERMINAL;
      } else if (mode == KZ_OPDERIV_MODE_MUST) {
        r.flags |= KZ_OPDERIV_FLAG_VOCABULARY;
      }
      d->ops.push_back(r);
    }
  }
}

// ---- F7 ----------------------------------------------------------------------------------------------

struct FixedCall {
  std::string call, next, node;
  std::set<std::string> entries;
};

struct SpawnOp {
  bool primitive = false;
  int entry_param = -1;           // -1: a fixed entry
  std::set<std::string> fixed;    // FIXED_ENTRY: the entries of all its fixed-entry spawn calls
  std::vector<FixedCall> fixed_calls;
  uint8_t kind = KZ_SPAWNW_KIND_NONE;
  std::string via_call, next, entry_fn, icall, via_node;
  uint32_t depth = 0;
};

class F7 {
 public:
  explicit F7(const Prog& p) : p_(p), sol_(solve_flow(p.m.flow)) {
    std::set<std::pair<std::string, std::string>> uniq;
    for (const FlowEdge& e : p_.m.flow)
      if (uniq.insert({e.src, e.dst}).second) succ_[e.src].push_back(e.dst);
    for (auto& kv : succ_) std::sort(kv.second.begin(), kv.second.end());
    for (const IndirectCall& ic : p_.m.indirect_calls) {
      auto it = p_.call_index.find(ic.call_key);
      if (it == p_.call_index.end()) continue;
      icalls_of_[p_.m.calls[it->second].caller_key].push_back(ic);
    }
    for (auto& kv : icalls_of_)
      std::sort(kv.second.begin(), kv.second.end(), [](const IndirectCall& a, const IndirectCall& b) { return a.call_key < b.call_key; });
  }

  void run(Derivation* d) {
    for (const P0Entry& e : p0_table()) {
      if (e.kind != KZ_P0_KIND_THREAD_CREATE || p_.body.count(e.name) != 0) continue;
      SpawnOp op;
      op.primitive = true;
      op.entry_param = e.entry;
      ops_[e.name] = op;
    }
    for (bool changed = true; changed;) {
      changed = false;
      for (const std::string& f : p_.keys) {
        auto existing = ops_.find(f);
        if (existing != ops_.end() && existing->second.entry_param >= 0) continue;
        SpawnOp got;
        if (!derive(f, &got)) continue;
        // a parameter found, or the fixed entries grew: the operations of this round are new
        if (existing == ops_.end() || got.entry_param >= 0 || got.fixed != existing->second.fixed || got.fixed_calls.size() != existing->second.fixed_calls.size()) {
          ops_[f] = got;
          changed = true;
        }
      }
    }
    // depth and chains
    for (const auto& kv : ops_) chain(kv.first);
    // rows
    for (const auto& kv : ops_) {
      const SpawnOp& op = kv.second;
      if (op.primitive) continue;
      SpawnWrapFact r;
      r.func_key = kv.first;
      r.param = op.entry_param >= 0 ? static_cast<uint32_t>(op.entry_param) : kNone;
      r.call_key = op.via_call;
      r.next_key = op.next;
      r.kind = op.kind;
      r.depth = depth_[kv.first];
      r.chain = chain(kv.first);
      r.primitive = p_.name(primitive_of(kv.first));
      r.via = op.via_node;
      if (op.entry_param >= 0) r.flags = KZ_SPAWNW_FLAG_VOCABULARY;
      if (op.kind == KZ_SPAWNW_KIND_FIXED_ENTRY) {
        ++d->spawn_fixed;
        for (const FixedCall& fc : op.fixed_calls)
          for (const std::string& e : fc.entries) {
            SpawnWrapFact x = r;
            x.call_key = fc.call;
            x.next_key = fc.next;
            x.via = fc.node;
            x.entry_key = e;
            x.depth = depth_of(fc.next) + 1;
            x.chain = p_.name(kv.first) + " -> " + chain(fc.next);
            d->spawns.push_back(x);
          }
        continue;
      }
      if (op.kind == KZ_SPAWNW_KIND_ENTRY_ARG) ++d->spawn_entry_arg;
      if (op.kind == KZ_SPAWNW_KIND_CALLED_BY_ENTRY) {
        ++d->spawn_called_by_entry;
        r.entry_key = op.entry_fn;
        r.icall_key = op.icall;
      }
      d->spawns.push_back(r);
    }
    // callers of fixed-entry operations that are no operation themselves
    std::set<std::string> callers;
    for (const auto& kv : ops_) {
      if (kv.second.kind != KZ_SPAWNW_KIND_FIXED_ENTRY) continue;
      for (size_t i = 0; i < p_.m.calls.size(); ++i)
        if (p_.m.calls[i].callee_key == kv.first && ops_.count(p_.m.calls[i].caller_key) == 0) callers.insert(p_.m.calls[i].caller_key);
    }
    d->fixed_callers_not_followed = static_cast<uint32_t>(callers.size());
    // vocabulary: the primitives and the operations with an entry parameter.  A fixed-entry
    // function starts a thread at an entry of its own; it serves the derivation of its callers
    // (one level) and is reported, but its calls are no spawn calls.
    for (const auto& kv : ops_) {
      if (kv.second.entry_param < 0) continue;
      d->spawn_keys.insert(kv.first);
      d->spawn_entry_param[kv.first] = static_cast<uint32_t>(kv.second.entry_param);
    }
  }

  // Startup-script entries for spawns whose entry is data, in functions that reach the symbol lookup.
  void script(const std::vector<StartupEntry>& script, Derivation* d) {
    std::set<std::string> lookups;
    for (const P0Entry& e : p0_table())
      if (e.kind == KZ_P0_KIND_DL_SYM && p_.body.count(e.name) == 0) lookups.insert(e.name);
    for (size_t ci = 0; ci < p_.m.calls.size(); ++ci) {
      const CallFact& c = p_.m.calls[ci];
      auto oit = ops_.find(c.callee_key);
      if (c.callee_key.empty() || oit == ops_.end() || oit->second.entry_param < 0) continue;
      const FpArg* fa = fp_arg(p_.ckey[ci], static_cast<uint32_t>(oit->second.entry_param));
      if (fa == nullptr || !fa->from_data) continue;
      bool unknown = false;
      if (!resolve(fa->nodes, &unknown).empty()) continue;
      std::string lookup_call;
      const std::string path = lookup_path(c.caller_key, lookups, &lookup_call);
      if (path.empty()) continue;
      ++d->script_calls;
      std::set<std::string> entries;
      for (const StartupEntry& se : script) {
        if (se.kind != "app" || se.symbol.empty()) continue;
        SpawnWrapFact r;
        r.func_key = c.caller_key;
        r.param = static_cast<uint32_t>(oit->second.entry_param);
        r.call_key = p_.ckey[ci];
        r.next_key = c.callee_key;
        r.icall_key = lookup_call;
        r.kind = KZ_SPAWNW_KIND_SCRIPT_ENTRY;
        r.depth = depth_[c.callee_key] + 1;
        r.chain = path;
        r.primitive = p_.name(primitive_of(c.callee_key));
        r.via = "startup.entry[" + std::to_string(se.index) + "] = " + se.text;
        auto fit = p_.m.funcs.find(se.symbol);
        if (fit != p_.m.funcs.end() && p_.body.count(se.symbol) != 0) {
          r.entry_key = se.symbol;
          entries.insert(se.symbol);
          ++d->script_in_closure;
        } else {
          r.flags |= KZ_SPAWNW_FLAG_NOT_IN_CLOSURE;
        }
        ++d->script_rows;
        d->spawns.push_back(r);
      }
      if (!entries.empty()) d->spawn_script_entries[p_.ckey[ci]] = entries;
    }
  }

 private:
  const FpArg* fp_arg(const std::string& ck, uint32_t index) const {
    auto it = p_.m.call_fp_args.find(ck);
    if (it == p_.m.call_fp_args.end()) return nullptr;
    for (const FpArg& a : it->second)
      if (a.index == index) return &a;
    return nullptr;
  }
  std::set<std::string> resolve(const std::vector<std::string>& nodes, bool* unknown) const {
    std::set<std::string> fns;
    *unknown = false;
    for (const std::string& n : nodes) {
      if (n.compare(0, 2, "F:") == 0) {
        fns.insert(n.substr(2));
        continue;
      }
      if (n.compare(0, 2, "?:") == 0) {
        *unknown = true;
        continue;
      }
      auto fit = sol_.funcs.find(n);
      if (fit != sol_.funcs.end()) fns.insert(fit->second.begin(), fit->second.end());
      auto uit = sol_.unknown.find(n);
      if (uit != sol_.unknown.end() && uit->second) *unknown = true;
    }
    return fns;
  }
  const std::set<std::string>& reach(const std::string& node) {
    auto it = reach_memo_.find(node);
    if (it != reach_memo_.end()) return it->second;
    std::set<std::string> seen{node};
    std::queue<std::string> q;
    q.push(node);
    while (!q.empty()) {
      const std::string n = q.front();
      q.pop();
      auto sit = succ_.find(n);
      if (sit == succ_.end()) continue;
      for (const std::string& x : sit->second)
        if (seen.insert(x).second) q.push(x);
    }
    return reach_memo_[node] = seen;
  }
  const std::set<std::string>& funcs_from(const std::string& root) {
    auto it = funcs_memo_.find(root);
    if (it != funcs_memo_.end()) return it->second;
    std::set<std::string> seen{root};
    std::queue<std::string> q;
    q.push(root);
    while (!q.empty()) {
      const std::string f = q.front();
      q.pop();
      auto cit = p_.cg.find(f);
      if (cit == p_.cg.end()) continue;
      for (const std::string& h : cit->second)
        if (seen.insert(h).second) q.push(h);
    }
    return funcs_memo_[root] = seen;
  }

  // One function: does a spawn call in it make it a spawn operation?
  // One function: is it a spawn operation through the spawn calls it makes (an entry parameter
  // first; else the fixed entries of all of them)?
  bool derive(const std::string& f, SpawnOp* out) {
    auto cit = p_.calls_of.find(f);
    if (cit == p_.calls_of.end()) return false;
    auto fit = p_.m.funcs.find(f);
    const uint32_t nparams = fit == p_.m.funcs.end() ? 0 : fit->second.nparams;
    SpawnOp fixed;
    bool have_fixed = false;
    for (size_t ci : cit->second) {
      const CallFact& c = p_.m.calls[ci];
      if (c.callee_key.empty() || c.callee_key == f) continue;
      auto oit = ops_.find(c.callee_key);
      if (oit == ops_.end()) continue;
      const SpawnOp& op = oit->second;
      std::vector<std::string> nodes;
      std::set<std::string> entries;
      if (op.entry_param >= 0) {
        if (const FpArg* fa = fp_arg(p_.ckey[ci], static_cast<uint32_t>(op.entry_param))) nodes = fa->nodes;
        bool unknown = false;
        entries = resolve(nodes, &unknown);
      } else {
        entries = op.fixed;
      }
      for (uint32_t k = 0; k < nparams; ++k) {
        const std::string pnode = "P:" + f + "#" + std::to_string(k);
        const std::set<std::string>& r = reach(pnode);
        for (const std::string& n : nodes)
          if (r.count(n) != 0) {
            SpawnOp w;
            w.entry_param = static_cast<int>(k);
            w.kind = KZ_SPAWNW_KIND_ENTRY_ARG;
            w.via_call = p_.ckey[ci];
            w.next = c.callee_key;
            w.via_node = n;
            *out = w;
            return true;
          }
        if (r.size() <= 1 && succ_.count(pnode) == 0) continue;
        for (const std::string& e : entries)
          for (const std::string& g : funcs_from(e)) {
            auto iit = icalls_of_.find(g);
            if (iit == icalls_of_.end()) continue;
            for (const IndirectCall& ic : iit->second)
              if (r.count(ic.node) != 0) {
                SpawnOp w;
                w.entry_param = static_cast<int>(k);
                w.kind = KZ_SPAWNW_KIND_CALLED_BY_ENTRY;
                w.via_call = p_.ckey[ci];
                w.next = c.callee_key;
                w.entry_fn = e;
                w.icall = ic.call_key;
                w.via_node = ic.node;
                *out = w;
                return true;
              }
          }
      }
      // a fixed entry: resolved, not from a parameter, through an operation that is not itself fixed
      if (op.entry_param >= 0 && !entries.empty()) {
        if (!have_fixed) {
          fixed.kind = KZ_SPAWNW_KIND_FIXED_ENTRY;
          fixed.via_call = p_.ckey[ci];
          fixed.next = c.callee_key;
          fixed.via_node = nodes.empty() ? std::string() : nodes.front();
          have_fixed = true;
        }
        fixed.fixed.insert(entries.begin(), entries.end());
        std::string via;
        for (const std::string& n : nodes) via += (via.empty() ? "" : ",") + n;
        fixed.fixed_calls.push_back(FixedCall{p_.ckey[ci], c.callee_key, via, entries});
      }
    }
    if (have_fixed) *out = fixed;
    return have_fixed;
  }

  // The spawn operations from `k` down to the primitive along `next`.  A function whose fixed entry
  // was later replaced by an entry parameter may point back at a caller; the walk stops at a repeat.
  uint32_t depth_of(const std::string& k) {
    auto it = depth_.find(k);
    if (it != depth_.end()) return it->second;
    std::vector<std::string> path;
    std::set<std::string> on_path;
    std::string cur = k;
    uint32_t below = 0;
    for (;;) {
      auto dit = depth_.find(cur);
      if (dit != depth_.end()) {
        below = dit->second;
        break;
      }
      auto oit = ops_.find(cur);
      if (oit == ops_.end() || oit->second.primitive || !on_path.insert(cur).second) {
        if (on_path.count(cur) == 0) depth_[cur] = 0;
        below = depth_.count(cur) != 0 ? depth_[cur] : 0;
        break;
      }
      path.push_back(cur);
      cur = oit->second.next;
    }
    for (auto p = path.rbegin(); p != path.rend(); ++p) depth_[*p] = ++below;
    return depth_[k];
  }
  std::string chain(const std::string& k) {
    auto it = chain_memo_.find(k);
    if (it != chain_memo_.end()) return it->second;
    depth_of(k);
    auto oit = ops_.find(k);
    std::string c = p_.name(k);
    if (oit != ops_.end() && !oit->second.primitive && chain_guard_.insert(k).second) c += " -> " + chain(oit->second.next);
    return chain_memo_[k] = c;
  }
  std::string primitive_of(const std::string& k) const {
    std::string cur = k;
    std::set<std::string> seen{cur};
    for (auto oit = ops_.find(cur); oit != ops_.end() && !oit->second.primitive && seen.insert(oit->second.next).second; oit = ops_.find(cur)) cur = oit->second.next;
    return cur;
  }
  // The shortest path of direct calls from `f` to a call of a lookup primitive; empty when there is
  // none.  Only direct calls: the address must come back to `f` as a value (a result, a store through
  // an argument), and a call through a function pointer that the field-based flow resolves would
  // attribute every lookup made in some callback to its caller.
  std::string lookup_path(const std::string& f, const std::set<std::string>& lookups, std::string* call) const {
    std::map<std::string, std::string> parent;
    std::queue<std::string> q;
    std::set<std::string> seen{f};
    q.push(f);
    while (!q.empty()) {
      const std::string g = q.front();
      q.pop();
      auto cit = p_.calls_of.find(g);
      if (cit == p_.calls_of.end()) continue;
      for (size_t ci : cit->second) {
        const std::string& t = p_.m.calls[ci].callee_key;
        if (t.empty()) continue;
        if (lookups.count(t) != 0 && p_.prim(t) != nullptr) {
          std::string path = p_.name(t);
          for (std::string cur = g;; cur = parent[cur]) {
            path = p_.name(cur) + " -> " + path;
            if (cur == f) break;
          }
          *call = p_.ckey[ci];
          return path;
        }
        if (p_.body.count(t) != 0 && seen.insert(t).second) {
          parent[t] = g;
          q.push(t);
        }
      }
    }
    return "";
  }

  const Prog& p_;
  FlowSolution sol_;
  std::map<std::string, std::vector<std::string>> succ_;
  std::map<std::string, std::vector<IndirectCall>> icalls_of_;
  std::map<std::string, SpawnOp> ops_;
  std::map<std::string, std::set<std::string>> reach_memo_, funcs_memo_;
  std::map<std::string, uint32_t> depth_;
  std::map<std::string, std::string> chain_memo_;
  std::set<std::string> chain_guard_;
};

}  // namespace

void derive_wrappers(const Model& first, const std::vector<StartupEntry>& script, Derivation* out) {
  Derivation& d = *out;
  d = Derivation();
  Prog p(first);
  p.build();
  d.functions_with_body = static_cast<uint32_t>(p.keys.size());
  d.cfg_built = static_cast<uint32_t>(first.cfgs.size());

  // P0 rows and the primitives in the vocabulary
  for (size_t i = 0; i < p0_table().size(); ++i) {
    const P0Entry& e = p0_table()[i];
    P0Fact r;
    r.entry = i;
    if (first.funcs.count(e.name) != 0) r.fn_key = e.name;
    for (const CallFact& c : first.calls)
      if (c.callee_key == e.name) ++r.n_calls;
    if (e.conditional) r.flags |= KZ_P0_FLAG_CONDITIONAL;
    const bool shadowed = p.body.count(e.name) != 0;
    if (shadowed) {
      r.flags |= KZ_P0_FLAG_SHADOWED;
    } else {
      bool vocab = true;
      switch (e.kind) {
        case KZ_P0_KIND_THREAD_CREATE: d.spawn_keys.insert(e.name); break;
        case KZ_P0_KIND_SLEEP: d.delay_keys.insert(e.name); break;
        case KZ_P0_KIND_THREAD_EXIT: d.terminal_keys.insert(e.name); break;
        case KZ_P0_KIND_PROCESS_EXIT:
          d.terminal_keys.insert(e.name);
          d.process_terminal_keys.insert(e.name);
          break;
        case KZ_P0_KIND_MUTEX_ACQUIRE:
          if (e.conditional) vocab = false;
          else d.lock_keys.insert(e.name);
          break;
        case KZ_P0_KIND_MUTEX_RELEASE: d.unlock_keys.insert(e.name); break;
        default: vocab = false; break;
      }
      if (vocab) r.flags |= KZ_P0_FLAG_VOCABULARY;
    }
    d.p0.push_back(r);
  }

  // F8
  F8 f8(p);
  f8.run(&d);
  op_rows(p, f8, KZ_OPDERIV_KIND_DELAY, &d);
  op_rows(p, f8, KZ_OPDERIV_KIND_TERMINAL, &d);
  for (const std::string& f : p.keys) {
    const FnState& s = f8.state(f);
    if (s.delay_must) d.delay_keys.insert(f);
    const uint8_t tm = term_mode(s);
    if (tm == KZ_OPDERIV_MODE_NONE) continue;
    d.terminal_keys.insert(f);
    if (tm != KZ_OPDERIV_MODE_MUST) d.conditional_terminal_keys.insert(f);
    if (s.scope == 2) d.process_terminal_keys.insert(f);
  }
  // loops whose own body calls a `may` delay operation (not in the vocabulary)
  {
    std::set<std::string> may;
    for (const std::string& f : p.keys)
      if (delay_mode(f8.state(f)) == KZ_OPDERIV_MODE_MAY) may.insert(f);
    std::set<std::string> loops;
    for (const CallFact& c : first.calls)
      if (!c.loop_key.empty() && !c.callee_key.empty() && may.count(c.callee_key) != 0) loops.insert(c.loop_key);
    d.loops_calling_may_delay = static_cast<uint32_t>(loops.size());
  }

  // F7
  F7 f7(p);
  f7.run(&d);
  f7.script(script, &d);
}

void apply_derivation(const Derivation& d, ExtractSpec* spec) {
  spec->delay_keys.insert(d.delay_keys.begin(), d.delay_keys.end());
  spec->terminal_keys.insert(d.terminal_keys.begin(), d.terminal_keys.end());
  spec->process_terminal_keys.insert(d.process_terminal_keys.begin(), d.process_terminal_keys.end());
  spec->conditional_terminal_keys.insert(d.conditional_terminal_keys.begin(), d.conditional_terminal_keys.end());
  spec->spawn_keys.insert(d.spawn_keys.begin(), d.spawn_keys.end());
  spec->lock_keys.insert(d.lock_keys.begin(), d.lock_keys.end());
  spec->unlock_keys.insert(d.unlock_keys.begin(), d.unlock_keys.end());
  for (const auto& kv : d.spawn_entry_param) spec->spawn_entry_param[kv.first] = kv.second;
  for (const auto& kv : d.spawn_script_entries) spec->spawn_script_entries[kv.first] = kv.second;
  for (const OpDerivFact& r : d.ops) spec->guard_calls.insert(r.call_key);
}

bool builtin_base_spec(ExtractSpec* out, std::string* error) {
  kz::phi::Profile profile;
  const kz::Status ps = load_builtin_profile(&profile);
  if (!ps.ok()) {
    *error = "the built-in profile: " + ps.describe();
    return false;
  }
  *out = ExtractSpec();
  spec_from_profile(profile, out);
  spec_add_builtin_data_fns(out);
  return true;
}

bool completeness_pass(const std::vector<UnitCommand>& units, const ExtractSpec& builtin, const ReadOptions& options, const Model& m, FieldStatusTable* out,
                       std::string* error) {
  ExtractSpec spec;
  spec.data_fns = builtin.data_fns;
  spec.profile_name = builtin.profile_name;
  spec.profile_revision = builtin.profile_revision;
  for (const FieldRef& f : completeness_fields(m)) spec.fields.push_back(TargetField{f, "completeness pass"});
  spec.keep_raw_atypes = true;
  Model pass;
  if (!extract_units(units, spec, options, &pass, error)) return false;
  finalize(&pass, spec);
  field_status_from(pass, m, out);
  return true;
}

bool add_lock_facts(const std::vector<UnitCommand>& units, const ExtractSpec& builtin, const ReadOptions& options, Model* m, std::string* error) {
  FieldStatusTable fields;
  if (!completeness_pass(units, builtin, options, *m, &fields, error)) return false;
  mark_incomplete_targets(m, fields);
  LockDerivation d;
  derive_locks(*m, &fields, &d);
  finalize_lock_facts(m, d, fields, {});
  return true;
}

bool extract_profile_free(const std::vector<UnitCommand>& units, const ExtractSpec& base, const ReadOptions& options, const std::vector<StartupEntry>& script,
                          Model* model, ExtractSpec* spec, Derivation* derivation, std::string* error) {
  FieldStatusTable fields;
  std::vector<FieldRef> first_fields;
  std::vector<TargetField> rule_fields;
  {
    ExtractSpec first_spec = base;
    first_spec.collect_wrapper_inputs = true;
    first_spec.extended_facts = true;  // F5 reads them (the lock-set skeletons, the argument identities, the reads)
    Model first;
    if (!extract_units(units, first_spec, options, &first, error)) return false;
    finalize(&first, first_spec);
    derive_wrappers(first, script, derivation);
    if (!completeness_pass(units, base, options, first, &fields, error)) return false;
    first_fields = completeness_fields(first);
    mark_incomplete_targets(&first, fields);
    LockDerivation ld;
    derive_locks(first, &fields, &ld);
    // the derived lock and unlock operations join the P0 primitives in the vocabulary
    derivation->lock_keys.insert(ld.lock_keys.begin(), ld.lock_keys.end());
    derivation->unlock_keys.insert(ld.unlock_keys.begin(), ld.unlock_keys.end());
    derivation->lock_vocabulary.insert(ld.lock_keys.begin(), ld.lock_keys.end());
    derivation->lock_vocabulary.insert(ld.unlock_keys.begin(), ld.unlock_keys.end());
    derivation->completeness_targets = fields.targets;
    if (!base.targets_rule.empty()) {
      TargetRules rules;
      if (!parse_target_rules(base.targets_rule, &rules, error)) return false;
      RuleTargetStats rs;
      rule_fields = rule_targets(first, rules, &rs);
      derivation->rule_t1 = rs.t1;
      derivation->rule_t2 = rs.t2;
      derivation->rule_both = rs.both;
      derivation->rule_targets = static_cast<uint32_t>(rule_fields.size());
      derivation->rule_t1_unshared_only = rs.t1_unshared_only;
      derivation->rule_t2_unshared_only = rs.t2_unshared_only;
    }
  }
  *spec = base;
  apply_derivation(*derivation, spec);
  // the rule targets (none without --targets-rule): the second pass extracts their writer sets
  for (const TargetField& t : rule_fields)
    if (!spec->has_field(t.field)) spec->fields.push_back(t);
  if (!extract_units(units, *spec, options, model, error)) return false;
  finalize(model, *spec);
  model->p0_facts = derivation->p0;
  model->op_derivs = derivation->ops;
  model->spawn_wraps = derivation->spawns;
  for (OpDerivFact& r : model->op_derivs) {
    auto it = model->call_guards.find(r.call_key);
    if (it != model->call_guards.end()) r.guard = it->second;
  }
  derivation->completeness_same_fields = completeness_fields(*model) == first_fields;
  mark_incomplete_targets(model, fields);
  LockDerivation ld;
  derive_locks(*model, &fields, &ld);
  std::set<std::string> final_vocabulary(ld.lock_keys.begin(), ld.lock_keys.end());
  final_vocabulary.insert(ld.unlock_keys.begin(), ld.unlock_keys.end());
  derivation->lock_vocabulary_same = final_vocabulary == derivation->lock_vocabulary;
  finalize_lock_facts(model, ld, fields, derivation->lock_vocabulary);
  return true;
}

}  // namespace kzs

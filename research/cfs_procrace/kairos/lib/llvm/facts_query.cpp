// facts_query.cpp -- see facts_query.hpp.
#include "facts_query.hpp"

#include <algorithm>
#include <map>
#include <queue>
#include <set>
#include <sstream>

namespace kzs {

kz::Status reach_report(const kz::Container& c, const ReachQuery& q, std::string* out) {
  std::vector<kz_fact_func> funcs;
  std::vector<kz_fact_call> calls;
  std::vector<kz_fact_derives> derives;
  std::vector<kz_fact_target> targets;
  kz::Status s;
  if (!(s = c.read_section(KZ_SEC_FUNCS, &funcs)).ok() || !(s = c.read_section(KZ_SEC_FACTS, &calls)).ok() ||
      !(s = c.read_section(KZ_SEC_DERIVES, &derives)).ok() || !(s = c.read_section(KZ_SEC_CALL_TARGETS, &targets)).ok())
    return s;
  auto str = [&](const kz_str& k) {
    std::string t;
    c.string(k, &t);
    return t;
  };
  auto fname = [&](uint32_t fn) {
    if (fn >= funcs.size()) return std::string("?");
    return str(funcs[fn].name);
  };
  std::map<uint32_t, size_t> call_by_site;
  for (size_t i = 0; i < calls.size(); ++i) call_by_site[calls[i].site] = i;

  std::vector<std::set<uint32_t>> adj(funcs.size());
  for (const kz_fact_call& k : calls)
    if (k.callee_fn != KZ_NONE32 && k.caller_fn < adj.size()) adj[k.caller_fn].insert(k.callee_fn);
  size_t unresolved = 0;
  for (const kz_fact_target& t : targets) {
    auto it = call_by_site.find(t.site);
    if (it == call_by_site.end()) continue;
    const uint32_t from = calls[it->second].caller_fn;
    if (from >= adj.size()) continue;
    if (t.fn == KZ_NONE32) {
      if (t.kind == KZ_CALL_TARGET_KIND_INDIRECT) ++unresolved;
      continue;
    }
    if (t.kind == KZ_CALL_TARGET_KIND_SPAWN_ENTRY && !q.follow_spawn) continue;
    adj[from].insert(t.fn);
  }

  struct Root {
    std::string name;
    std::vector<uint32_t> fns;
    std::vector<bool> reach;
  };
  std::vector<Root> roots;
  for (const std::string& name : q.roots) {
    Root r;
    r.name = name;
    for (uint32_t i = 0; i < funcs.size(); ++i)
      if (str(funcs[i].name) == name) r.fns.push_back(i);
    r.reach.assign(funcs.size(), false);
    std::queue<uint32_t> work;
    for (uint32_t f : r.fns) {
      r.reach[f] = true;
      work.push(f);
    }
    while (!work.empty()) {
      const uint32_t f = work.front();
      work.pop();
      for (uint32_t g : adj[f])
        if (!r.reach[g]) {
          r.reach[g] = true;
          work.push(g);
        }
    }
    roots.push_back(std::move(r));
  }

  struct Sink {
    uint32_t site;
    uint32_t arg;
    uint32_t fn;
  };
  std::vector<Sink> sinks;
  for (const kz_fact_derives& d : derives) {
    if (d.status != KZ_DERIVES_STATUS_DERIVED) continue;
    if (str(d.loc_type) + "." + str(d.loc_field) != q.loc) continue;
    if (!q.callee.empty() && str(d.callee) != q.callee) continue;
    auto it = call_by_site.find(d.site);
    if (it == call_by_site.end()) continue;
    sinks.push_back({d.site, d.arg_index, calls[it->second].caller_fn});
  }
  std::ostringstream os;
  os << "reach-report 1\n";
  os << "location: " << q.loc << (q.callee.empty() ? "" : "  callee: " + q.callee) << "\n";
  os << "sink call sites (an argument derives from the location): " << sinks.size() << "\n";
  os << "indirect call sites without a resolved target (they add no edge): " << unresolved << "\n";
  os << "spawn entries followed: " << (q.follow_spawn ? "yes" : "no") << "\n";
  for (const Root& r : roots) {
    size_t n = 0;
    for (const Sink& k : sinks)
      if (k.fn < r.reach.size() && r.reach[k.fn]) ++n;
    os << "root " << r.name << " (" << r.fns.size() << " function" << (r.fns.size() == 1 ? "" : "s") << "): " << n << " of " << sinks.size() << " sink sites in reachable functions\n";
  }
  for (const Sink& k : sinks) {
    const kz_fact_call& call = calls[call_by_site[k.site]];
    os << "  site " << k.site << " " << str(call.file) << ":" << call.line << " in " << fname(k.fn) << " arg " << k.arg << " reachable from:";
    bool any = false;
    for (const Root& r : roots)
      if (k.fn < r.reach.size() && r.reach[k.fn]) {
        os << " " << r.name;
        any = true;
      }
    if (!any) os << " -";
    os << "\n";
  }
  *out = os.str();
  return kz::Status::success();
}

}  // namespace kzs

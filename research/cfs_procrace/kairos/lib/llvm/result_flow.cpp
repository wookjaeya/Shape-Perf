// result_flow.cpp -- the values each function may return, for the outcome classes of --result-flow (result_flow.hpp).
#include "result_flow.hpp"

#include <algorithm>
#include <functional>
#include <map>
#include <set>
#include <string>

#include "p0.hpp"

namespace kzs {

const char* result_flow_revision() { return "result flow rules revision 1 (2026-11-16)"; }

std::shared_ptr<const ResultValues> build_result_values(const Model& first) {
  auto out = std::make_shared<ResultValues>();
  std::map<std::string, std::vector<const ReturnFact*>> rets;  // function key -> its rows
  for (const ReturnFact& r : first.returns) rets[r.func_key].push_back(&r);
  std::map<std::string, const CallFact*> call_by_key;
  for (const CallFact& c : first.calls) call_by_key[c.pos.str() + "#" + std::to_string(c.ordinal)] = &c;
  std::map<std::string, RfVals> memo;
  std::set<std::string> active;
  auto merge = [](RfVals* to, const RfVals& in, const std::string& via, bool present_only) {
    for (const RfVal& v : in.vals) {
      bool dup = false;
      for (RfVal& o : to->vals)
        if (o.v == v.v) {
          dup = true;
          o.present_only = o.present_only && (v.present_only || present_only);
        }
      if (!dup) {
        RfVal x = v;
        x.present_only = x.present_only || present_only;
        if (!via.empty()) x.via = via + (x.via.empty() ? "" : "; " + x.via);
        to->vals.push_back(x);
      }
    }
    for (const std::string& o : in.open) to->open.push_back(o);
    if (in.complement != 0 && to->complement == 0) {
      to->complement = in.complement;
      to->complement_why = in.complement_why;
    }
  };
  // a bodiless function: a P0 thread creation returns zero when it succeeds and an error number otherwise
  // (POSIX.1-2017 pthread_create, RETURN VALUE; the P0 table's thread-creation family); any other is not followed
  auto bodiless = [&](const std::string& key, const std::string& name) {
    RfVals v;
    for (const P0Entry& e : p0_table())
      if (e.kind == KZ_P0_KIND_THREAD_CREATE && name == e.name) {
        v.vals.push_back(RfVal{0, "0 (" + name + ": zero on success, P0 thread creation)", false, ""});
        v.complement = KZ_REFF_OUTCOME_FAILURE;
        v.complement_why = "an error number: " + name + " creates no thread (P0 thread creation)";
        return v;
      }
    v.open.push_back(name + " has no body in these facts");
    (void)key;
    return v;
  };
  std::function<RfVals(const std::string&)> of_fn;
  // the values of one call: the callee's, its parameters returned unchanged resolved by this call's arguments
  std::function<RfVals(const std::string&, int)> of_call = [&](const std::string& ck, int depth) {
    RfVals r;
    auto cit = call_by_key.find(ck);
    if (cit == call_by_key.end() || cit->second->callee_key.empty()) {
      r.open.push_back("an indirect call or a call not in these facts");
      return r;
    }
    const CallFact& c = *cit->second;
    const RfVals f = of_fn(c.callee_key);
    r.vals = f.vals;
    r.open = f.open;
    r.complement = f.complement;
    r.complement_why = f.complement_why;
    for (uint32_t k : f.params) {
      const std::string via = c.callee_name + " returns its parameter " + std::to_string(k) + " unchanged";
      auto ac = first.rf_arg_consts.find(ck);
      if (ac != first.rf_arg_consts.end() && ac->second.count(k) != 0) {
        merge(&r, RfVals{{RfVal{ac->second.at(k), std::to_string(ac->second.at(k)), false, via}}, {}, {}, 0, ""}, "", false);
        continue;
      }
      auto am = first.rf_arg_calls.find(ck);
      if (am != first.rf_arg_calls.end() && am->second.count(k) != 0 && depth < 16) {
        merge(&r, of_call(am->second.at(k), depth + 1), via + " (its argument is the result of the call at " + am->second.at(k) + ")", false);
        continue;
      }
      r.open.push_back(via + " and the argument at " + c.pos.str() + " is not followed");
    }
    return r;
  };
  of_fn = [&](const std::string& fk) -> RfVals {
    auto mit = memo.find(fk);
    if (mit != memo.end()) return mit->second;
    if (!active.insert(fk).second) {
      RfVals v;
      v.open.push_back("the returns of " + fk + " lead back to it (recursion)");
      return v;
    }
    RfVals v;
    auto rit = rets.find(fk);
    if (first.processed_functions.count(fk) == 0) {
      auto fit = first.funcs.find(fk);
      v = bodiless(fk, fit != first.funcs.end() ? fit->second.name : fk);
    } else if (rit != rets.end()) {
      auto pr = first.rf_param_returns.find(fk);
      for (const ReturnFact* r : rit->second) {
        const bool redefined = (r->flags & KZ_RET_FLAG_REDEFINED) != 0;
        if (r->vkind == KZ_RET_VALUE_CONST) {
          merge(&v, RfVals{{RfVal{r->constant, r->text, redefined, ""}}, {}, {}, 0, ""}, "", false);
        } else if (r->vkind == KZ_RET_VALUE_CALL && !r->call_key.empty()) {
          merge(&v, of_call(r->call_key, 0), "", redefined);
        } else if (r->vkind == KZ_RET_VALUE_PARAM && r->value != nullptr && pr != first.rf_param_returns.end() &&
                   pr->second.count({r->pos.str(), static_cast<uint32_t>(r->value->value)}) != 0) {
          const uint32_t k = static_cast<uint32_t>(r->value->value);
          if (std::find(v.params.begin(), v.params.end(), k) == v.params.end()) v.params.push_back(k);
        } else {
          v.open.push_back("returns " + r->text + " at " + r->pos.str());
        }
      }
    }
    active.erase(fk);
    memo[fk] = v;
    return v;
  };
  for (const auto& [k, f] : first.funcs) {
    (void)f;
    out->by_fn[k] = of_fn(k);
  }
  return out;
}

}  // namespace kzs

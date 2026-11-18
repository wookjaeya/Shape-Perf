// outcome.hpp -- the outcome classes of a status value: one definition for the SEMANTICS rows (semantics.cpp), the
// RESULT_EFFECT rows (serialize.cpp) and the readers of the facts (relations.cpp).  docs/static_facts.md 5w.
//
// outcome class rule revision 1 (2026-11-18).  A value v a function returns is classed in the status space of the
// function:
//   success   v is the success constant of the space (a `status-space success` line of the semantics data);
//   failure   v is not the success constant, and v is negative, or the space has severity constants and every bit of
//             their mask (the bitwise or of the `status-space severity` constants) is set in v as a 32-bit value;
//   other     the rest.
// The space of a function: the status space of the component whose semantics entry applies to it (SEMANTICS rows); an
// entry whose return kind is not `status` gives every value `other`.  A function without an entry is classed in the
// space the facts' components share: the success constant when all of them have the same one, the masks together.
// Facts without any status-space row (no --semantics) are classed by the fallback of the rule: success constant 0 and
// the mask 0xC0000000 (the earlier method rule of the failure range, plan_gen.cpp fail_class).  No API name is here:
// the constants come from the data rows.
#pragma once

#include <cstdint>
#include <map>
#include <string>

#include "kz/schema/kz_schema.h"

namespace kzs {

inline const char* outcome_rule_revision() { return "outcome class rule revision 1 (2026-11-18)"; }
// the item texts the rule's rows carry (written by semantics.cpp, read back from the facts)
inline const char* outcome_severity_item() { return "severity constant "; }
inline const char* outcome_return_kind_item() { return "return kind "; }

struct OutcomeSpace {
  bool has_success = false;
  int64_t success = 0;
  uint64_t mask = 0;
};

inline uint8_t outcome_class(const OutcomeSpace& sp, int64_t v) {
  if (sp.has_success && v == sp.success) return KZ_REFF_OUTCOME_SUCCESS;
  if (v < 0) return KZ_REFF_OUTCOME_FAILURE;
  if (sp.mask != 0 && v <= 0xFFFFFFFFLL && (static_cast<uint64_t>(v) & sp.mask) == sp.mask) return KZ_REFF_OUTCOME_FAILURE;
  return KZ_REFF_OUTCOME_OTHER;
}

struct OutcomeSpaces {
  struct Entry {
    std::string component;
    bool status = false;
  };
  std::map<std::string, OutcomeSpace> by_component;
  std::map<uint32_t, Entry> entry_of_fn;
  // one SEMANTICS row
  void add(uint8_t layer, uint8_t column, const std::string& component, uint32_t fn, const std::string& item, int64_t value) {
    if (layer == KZ_SEM_LAYER_STATUS) {
      OutcomeSpace& s = by_component[component];
      if (column == KZ_SEM_COLUMN_SUCCESS) {
        s.has_success = true;
        s.success = value;
      } else if (item.compare(0, std::string(outcome_severity_item()).size(), outcome_severity_item()) == 0 ||
                 (column == KZ_SEM_COLUMN_FAILURE && item.compare(0, 14, "severity mask ") == 0)) {
        // (the second form: facts written before 2026-11-18 carry the severity constants in the failure column)
        s.mask |= static_cast<uint64_t>(value) & 0xFFFFFFFFULL;
      }
      return;
    }
    const std::string rk = outcome_return_kind_item();
    if (layer == KZ_SEM_LAYER_FRAMEWORK && fn != KZ_NONE32 && column == KZ_SEM_COLUMN_NONE && item.compare(0, rk.size(), rk) == 0) {
      const std::string kind = item.substr(rk.size(), item.find(' ', rk.size()) == std::string::npos ? std::string::npos : item.find(' ', rk.size()) - rk.size());
      entry_of_fn[fn] = Entry{component, kind == "status"};
    }
  }
  // the space a function without an entry is classed in
  OutcomeSpace shared() const {
    OutcomeSpace s;
    if (by_component.empty()) {
      s.has_success = true;
      s.success = 0;
      s.mask = 0xC0000000ULL;
      return s;
    }
    bool first = true, agree = true;
    for (const auto& [c, x] : by_component) {
      s.mask |= x.mask;
      if (!x.has_success) continue;
      if (first) {
        s.success = x.success;
        first = false;
      } else if (s.success != x.success) {
        agree = false;
      }
    }
    s.has_success = !first && agree;
    return s;
  }
  uint8_t cls(uint32_t fn, int64_t v) const {
    auto e = entry_of_fn.find(fn);
    if (e != entry_of_fn.end()) {
      if (!e->second.status) return KZ_REFF_OUTCOME_OTHER;
      auto s = by_component.find(e->second.component);
      if (s != by_component.end()) return outcome_class(s->second, v);
    }
    return outcome_class(shared(), v);
  }
  std::string basis_text() const {
    std::string t = std::string(outcome_rule_revision()) + ": success = the success constant of the callee's status space; failure = not that constant and negative, or every bit of the "
                    "space's severity mask set; other = the rest; ";
    if (by_component.empty()) return t + "no status-space row in these facts: the fallback (success constant 0, mask 3221225472)";
    t += "status spaces of these facts " + std::to_string(by_component.size()) + ":";
    for (const auto& [c, x] : by_component)
      t += " [" + c + ": success " + (x.has_success ? std::to_string(x.success) : std::string("none")) + ", severity mask " + (x.mask != 0 ? std::to_string(x.mask) : std::string("none")) + "]";
    const OutcomeSpace sh = shared();
    t += "; a callee without a semantics entry: success " + (sh.has_success ? std::to_string(sh.success) : std::string("none (the spaces differ)")) + ", mask " +
         (sh.mask != 0 ? std::to_string(sh.mask) : std::string("none")) + "; entries with a status return kind " + std::to_string(n_status()) + " of " + std::to_string(entry_of_fn.size());
    return t;
  }
  size_t n_status() const {
    size_t n = 0;
    for (const auto& [f, e] : entry_of_fn) n += e.status ? 1 : 0;
    return n;
  }
};

}  // namespace kzs

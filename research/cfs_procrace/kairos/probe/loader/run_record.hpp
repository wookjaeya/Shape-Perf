// run_record.hpp -- the conditions of one observed run, as BASIS items and DEVIATIONS of the run
// container (CONDITIONS_POLICY: a value, its reference, its status; a deviation has a reason and
// an effect).  The record holds what the observer measured (user, command, scheduling, ...)
// and what the caller declares about the run (a conditions file: the harness's own settings,
// the run count and the reason for it, the stratum).  Nothing is chosen by the observer: a
// condition that nobody gave stays absent, and the report says so.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "kz/core/status.hpp"
#include "kz/schema/kz_schema.h"

namespace kz::probe {

struct Item {
  std::string name, value;
  uint8_t policy = KZ_POLICY_UNSET;
  int deviation = -1;  // index into RunRecord::deviations, or -1
};

struct Deviation {
  std::string subject, reference_value, used_value, reason, effect;
  uint8_t avoidable = KZ_AVOIDABLE_UNKNOWN;
};

class RunRecord {
 public:
  std::vector<Item> items;
  std::vector<Deviation> deviations;

  void add(const std::string& name, const std::string& value, uint8_t policy = KZ_POLICY_UNSET) { items.push_back({name, value, policy, -1}); }
  // Adds a deviation and links the item of the same name (if there is one, else adds none).
  void add_deviation(const Deviation& d);
  // "name = value  [policy]" lines, in insertion order, for the human-readable record.
  std::string text() const;
  const Item* find(const std::string& name) const;
};

// Reads a conditions file.  Tab-separated lines; '#' lines and blank lines are ignored:
//   item <TAB> name <TAB> value <TAB> policy
//   deviation <TAB> subject <TAB> reference_value <TAB> used_value <TAB> reason <TAB> effect <TAB> avoidable
// policy: from-reference | deviation | unspecified_by_reference | declared-assumption | unit-test-value | unset
// avoidable: yes | no | unknown.  A deviation line attaches to the item whose name equals its subject.
Status load_conditions(const std::string& text, RunRecord* out);

bool policy_from_name(const std::string& name, uint8_t* out);
const char* policy_to_name(uint8_t policy);

}  // namespace kz::probe

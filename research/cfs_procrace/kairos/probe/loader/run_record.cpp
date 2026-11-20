// run_record.cpp -- see run_record.hpp.
#include "run_record.hpp"

#include <sstream>

namespace kz::probe {

namespace {

std::vector<std::string> split_tabs(const std::string& line) {
  std::vector<std::string> f;
  size_t pos = 0;
  for (;;) {
    const size_t t = line.find('\t', pos);
    if (t == std::string::npos) { f.push_back(line.substr(pos)); break; }
    f.push_back(line.substr(pos, t - pos));
    pos = t + 1;
  }
  return f;
}

}  // namespace

bool policy_from_name(const std::string& name, uint8_t* out) {
  static const struct { const char* n; uint8_t v; } table[] = {
      {"unset", KZ_POLICY_UNSET},         {"from-reference", KZ_POLICY_FROM_REFERENCE},
      {"deviation", KZ_POLICY_DEVIATION}, {"unspecified_by_reference", KZ_POLICY_UNSPECIFIED},
      {"unit-test-value", KZ_POLICY_UNIT_TEST}, {"declared-assumption", KZ_POLICY_DECLARED}};
  for (const auto& e : table)
    if (name == e.n) { *out = e.v; return true; }
  return false;
}

const char* policy_to_name(uint8_t policy) {
  switch (policy) {
    case KZ_POLICY_FROM_REFERENCE: return "from-reference";
    case KZ_POLICY_DEVIATION: return "deviation";
    case KZ_POLICY_UNSPECIFIED: return "unspecified_by_reference";
    case KZ_POLICY_UNIT_TEST: return "unit-test-value";
    case KZ_POLICY_DECLARED: return "declared-assumption";
    default: return "unset";
  }
}

void RunRecord::add_deviation(const Deviation& d) {
  const int idx = static_cast<int>(deviations.size());
  deviations.push_back(d);
  for (Item& it : items)
    if (it.name == d.subject && it.deviation < 0) { it.deviation = idx; return; }
}

const Item* RunRecord::find(const std::string& name) const {
  for (const Item& it : items)
    if (it.name == name) return &it;
  return nullptr;
}

std::string RunRecord::text() const {
  std::ostringstream o;
  for (const Item& it : items) {
    o << it.name << " = " << it.value;
    if (it.policy != KZ_POLICY_UNSET) o << "   [" << policy_to_name(it.policy) << "]";
    if (it.deviation >= 0) {
      const Deviation& d = deviations[it.deviation];
      o << "\n    deviation: reference: " << (d.reference_value.empty() ? "(silent)" : d.reference_value) << "; used: " << d.used_value
        << "; reason: " << d.reason << "; effect: " << d.effect;
    }
    o << "\n";
  }
  for (size_t i = 0; i < deviations.size(); ++i) {
    bool linked = false;
    for (const Item& it : items) linked = linked || it.deviation == static_cast<int>(i);
    if (!linked)
      o << "deviation (no item of that name): " << deviations[i].subject << ": reference: " << deviations[i].reference_value
        << "; used: " << deviations[i].used_value << "; reason: " << deviations[i].reason << "; effect: " << deviations[i].effect << "\n";
  }
  return o.str();
}

Status load_conditions(const std::string& text, RunRecord* out) {
  size_t pos = 0;
  uint32_t lineno = 0;
  while (pos <= text.size()) {
    const size_t nl = text.find('\n', pos);
    std::string line = text.substr(pos, nl == std::string::npos ? std::string::npos : nl - pos);
    pos = nl == std::string::npos ? text.size() + 1 : nl + 1;
    ++lineno;
    while (!line.empty() && (line.back() == '\r')) line.pop_back();
    if (line.empty() || line[0] == '#') continue;
    const std::vector<std::string> f = split_tabs(line);
    auto bad = [&](const std::string& what) { return Status::error(Err::Invalid, "conditions line " + std::to_string(lineno) + ": " + what); };
    if (f[0] == "item") {
      if (f.size() != 4) return bad("item <TAB> name <TAB> value <TAB> policy");
      uint8_t pol = 0;
      if (!policy_from_name(f[3], &pol)) return bad("unknown policy '" + f[3] + "'");
      if (f[1].empty()) return bad("an item needs a name");
      out->add(f[1], f[2], pol);
    } else if (f[0] == "deviation") {
      if (f.size() != 7) return bad("deviation <TAB> subject <TAB> reference_value <TAB> used_value <TAB> reason <TAB> effect <TAB> avoidable");
      Deviation d;
      d.subject = f[1]; d.reference_value = f[2]; d.used_value = f[3]; d.reason = f[4]; d.effect = f[5];
      if (f[6] == "yes") d.avoidable = KZ_AVOIDABLE_YES;
      else if (f[6] == "no") d.avoidable = KZ_AVOIDABLE_NO;
      else if (f[6] == "unknown") d.avoidable = KZ_AVOIDABLE_UNKNOWN;
      else return bad("avoidable must be yes, no or unknown");
      if (d.subject.empty() || d.reason.empty() || d.effect.empty()) return bad("a deviation needs a subject, a reason and an effect");
      out->add_deviation(d);
    } else {
      return bad("unknown line type '" + f[0] + "'");
    }
  }
  return Status::success();
}

}  // namespace kz::probe

#include <algorithm>

#include "kz/core/phi.hpp"
#include "kz/core/phi_provenance.hpp"

namespace kz {
namespace phi {

namespace {

// The strongest (`weakest` false) or weakest tier among the records an entry cites;
// "unreferenced" if it cites none.
std::string extreme_tier(const Profile& p, const std::vector<std::string>& names, bool weakest) {
  std::string pick = "unreferenced";
  int pick_rank = -1;
  for (const std::string& n : names) {
    const int32_t i = p.find_prov(n);
    if (i < 0) continue;
    const std::string& t = p.provs[static_cast<size_t>(i)].tier;
    const int r = tier_rank(t);
    if (r < 0) continue;
    if (pick_rank < 0 || (weakest ? r > pick_rank : r < pick_rank)) {
      pick = t;
      pick_rank = r;
    }
  }
  return pick;
}

void count(const Profile& p, ProfileStats* st, const char* what, const std::string& name, const std::vector<std::string>& prov) {
  const std::string strongest = extreme_tier(p, prov, false);
  const std::string weakest = extreme_tier(p, prov, true);
  ++st->entries;
  const int si = tier_index(strongest);
  if (si >= 0 && si < 6) ++st->by_strongest_tier[si];
  const int wi = tier_index(weakest);
  if (wi >= 0 && wi < 6) ++st->by_weakest_tier[wi];
  if (strongest == "researcher" || strongest == "unreferenced") {
    ++st->hand_entered;
    st->hand_entered_names.push_back(std::string(what) + " " + name);
  }
  for (const std::string& n : prov) {
    const int32_t i = p.find_prov(n);
    if (i >= 0 && p.provs[static_cast<size_t>(i)].tier == "researcher") {
      ++st->cites_researcher;
      st->cites_researcher_names.push_back(std::string(what) + " " + name);
      break;
    }
  }
}

}  // namespace

ProfileStats profile_stats(const Profile& p) {
  ProfileStats st;
  st.prov_records = p.provs.size();
  for (const ConstDecl& d : p.consts) count(p, &st, "const", d.name, d.prov);
  for (const KindDecl& d : p.kinds) count(p, &st, "kind", d.name, d.prov);
  for (const LocDecl& d : p.locs) count(p, &st, "loc", d.name, d.prov);
  for (const BindDecl& d : p.binds) count(p, &st, "bind", d.name, d.prov);
  for (const SiteDecl& d : p.sites) count(p, &st, "site", d.name, d.prov);
  for (const OpDecl& d : p.ops) {
    count(p, &st, "op", d.name, d.prov);
    for (const EffectDecl& e : d.effects) {
      count(p, &st, "effect", d.name + "." + e.label, e.prov);
      if (e.status == "declared_trusted") {
        ++st.declared_trusted;
        st.declared_trusted_names.push_back("effect " + d.name + "." + e.label);
      }
    }
  }
  for (const InitDecl& d : p.inits) count(p, &st, "init", d.name, d.prov);
  for (const HbRuleDecl& d : p.rules) count(p, &st, "hb", d.name, d.prov);
  for (const RspecDecl& d : p.rspecs) {
    count(p, &st, "rspec", d.name, d.prov);
    // The caller level: appreq and the Allowed entries are entries of their own.
    auto describe = [&](const std::vector<std::string>& names, bool* hand) {
      std::string t;
      for (const std::string& n : names) {
        const int32_t i = p.find_prov(n);
        if (!t.empty()) t += ", ";
        if (i < 0) {
          t += n + " (unknown record)";
          *hand = true;
          continue;
        }
        const ProvDecl& r = p.provs[static_cast<size_t>(i)];
        t += n + " (" + r.tier + ", side " + (r.side.empty() ? std::string("n/a") : r.side) + ")";
        if (r.tier == "researcher" || r.tier == "unreferenced") *hand = true;
      }
      if (names.empty()) {
        t = "no record";
        *hand = true;
      }
      return t;
    };
    if (d.appreq_unspecified) {
      st.caller_level_lines.push_back("rspec " + d.name + " appreq unspecified_by_reference");
    } else {
      ++st.appreq_specified;
      const std::vector<std::string> one{d.appreq};
      count(p, &st, "appreq", d.name, one);
      bool hand = false;
      st.caller_level_lines.push_back("rspec " + d.name + " appreq " + describe(one, &hand));
      if (hand) ++st.caller_level_hand_entered;
    }
    if (d.allowed.empty()) st.caller_level_lines.push_back("rspec " + d.name + " allowed " + allowed_text(d.allowed_head.kind));
    for (size_t i = 0; i < d.allowed.size(); ++i) {
      const AllowedEntry& a = d.allowed[i];
      const std::string label = d.name + "." + allowed_text(a.kind) + "#" + std::to_string(i + 1);
      ++st.allowed_entries;
      count(p, &st, "allowed", label, a.prov);
      bool hand = false;
      st.caller_level_lines.push_back("rspec " + d.name + " allowed " + allowed_text(a.kind) + "#" + std::to_string(i + 1) + ": " + describe(a.prov, &hand));
      if (hand) ++st.caller_level_hand_entered;
    }
  }
  for (const TrustedDecl& d : p.trusted) {
    count(p, &st, "trusted", d.name, d.prov);
    if (d.cls == "declared_trusted") {
      ++st.declared_trusted;
      st.declared_trusted_names.push_back("trusted " + d.name);
    }
    if (d.cls == "assumption") ++st.assumptions;
  }
  if (p.context.present) count(p, &st, "context", "context", p.context.prov);
  if (p.scope.present) count(p, &st, "scope", "scope", p.scope.prov);
  std::sort(st.hand_entered_names.begin(), st.hand_entered_names.end());
  std::sort(st.cites_researcher_names.begin(), st.cites_researcher_names.end());
  std::sort(st.declared_trusted_names.begin(), st.declared_trusted_names.end());
  return st;
}

std::string stats_text(const ProfileStats& st) {
  static const char* const tier_names[6] = {"unreferenced", "contract-backed", "admitted", "code-derived", "issue-derived", "researcher"};
  auto tiers = [&](const size_t (&counts)[6]) {
    std::string out;
    for (int i = 1; i < 6; ++i) out += std::string("  ") + tier_names[i] + ": " + std::to_string(counts[i]) + "\n";
    out += std::string("  ") + tier_names[0] + ": " + std::to_string(counts[0]) + "\n";
    return out;
  };
  std::string out = "provenance records: " + std::to_string(st.prov_records) + "\nentries with provenance: " + std::to_string(st.entries) + "\n";
  out += "by strongest cited tier (the best part of the entry):\n" + tiers(st.by_strongest_tier);
  out += "by weakest cited tier (SPEC 1.6: a mixed entry takes the weakest tier of its parts):\n" + tiers(st.by_weakest_tier);
  out += "hand-entered (strongest cited tier researcher or unreferenced): " + std::to_string(st.hand_entered) + "\n";
  for (const std::string& n : st.hand_entered_names) out += "  " + n + "\n";
  out += "entries citing at least one researcher record: " + std::to_string(st.cites_researcher) + "\n";
  for (const std::string& n : st.cites_researcher_names) out += "  " + n + "\n";
  out += "declared-trusted: " + std::to_string(st.declared_trusted) + "\n";
  for (const std::string& n : st.declared_trusted_names) out += "  " + n + "\n";
  out += "assumptions: " + std::to_string(st.assumptions) + "\n";
  out += "caller level (SPEC 1.6): " + std::to_string(st.appreq_specified) + " specified appreq, " + std::to_string(st.allowed_entries) + " Allowed entries, " +
         std::to_string(st.caller_level_hand_entered) + " citing a hand-entered record or none\n";
  for (const std::string& l : st.caller_level_lines) out += "  " + l + "\n";
  return out;
}

}  // namespace phi
}  // namespace kz

// decode_match.cpp -- see include/kz/trace/decode_match.hpp.
#include "kz/trace/decode_match.hpp"

#include <algorithm>
#include <map>
#include <unordered_map>

namespace kz::trace {

std::vector<CallPair> match_calls(const std::vector<kz_raw_ev>& events, const std::vector<SiteSpec>& sites, MatchStats* stats) {
  MatchStats local;
  MatchStats& st = stats != nullptr ? *stats : local;
  st = MatchStats();
  // site -> the site it pairs with (and whether it is the entry side)
  std::vector<int64_t> partner(sites.size(), -1);
  for (size_t i = 0; i < sites.size(); ++i) {
    if (sites[i].kind != KZ_SITE_KIND_ENTRY_PROBE || sites[i].symbol.empty()) continue;
    for (size_t j = 0; j < sites.size(); ++j)
      if (sites[j].kind == KZ_SITE_KIND_RET_PROBE && sites[j].symbol == sites[i].symbol) {
        // The key is slot 3 with role return_address on both sides; a pair without it is not
        // matched at all (check_plan refuses such a plan before an observation).
        if (!has_return_address_key(sites[i]) || !has_return_address_key(sites[j])) {
          ++st.sites_without_key;
          break;
        }
        partner[i] = static_cast<int64_t>(j);
        partner[j] = static_cast<int64_t>(i);
        ++st.sites_paired;
        break;
      }
  }
  std::vector<CallPair> out;
  // per (entry site, thread): the stack of open calls (index into `out`, return address)
  struct Open { size_t pair; uint64_t retaddr; };
  std::map<std::pair<uint32_t, uint32_t>, std::vector<Open>> stacks;
  for (uint32_t i = 0; i < events.size(); ++i) {
    const kz_raw_ev& ev = events[i];
    if (ev.site >= sites.size() || partner[ev.site] < 0) continue;
    const SiteSpec& s = sites[ev.site];
    const bool unreadable = (ev.flags & KZ_RAW_FLAG_TRUNC) != 0;  // a slot of the event holds no value: never match on it
    if (s.kind == KZ_SITE_KIND_ENTRY_PROBE) {
      CallPair p;
      p.entry_event = i;
      out.push_back(p);
      if (unreadable) {
        out.back().unreadable_key = true;
        ++st.events_with_unreadable_key;
        continue;
      }
      stacks[{ev.site, ev.tid}].push_back({out.size() - 1, ev.a[kSlotReturnAddress]});
    } else {
      if (unreadable) {
        CallPair p;
        p.return_event = i;
        p.unreadable_key = true;
        out.push_back(p);
        ++st.events_with_unreadable_key;
        continue;
      }
      auto& stack = stacks[{static_cast<uint32_t>(partner[ev.site]), ev.tid}];
      size_t k = stack.size();
      while (k > 0 && stack[k - 1].retaddr != ev.a[kSlotReturnAddress]) --k;
      if (k == 0) {
        CallPair p;
        p.return_event = i;
        out.push_back(p);
        ++st.returns_without_entry;
        continue;
      }
      out[stack[k - 1].pair].return_event = i;
      ++st.pairs;
      // calls opened after the matching one and still open have lost their return
      stack.resize(k - 1);
    }
  }
  for (const CallPair& p : out)
    if (p.entry_event != KZ_NONE32 && p.return_event == KZ_NONE32 && !p.unreadable_key) ++st.entries_without_return;
  return out;
}

}  // namespace kz::trace

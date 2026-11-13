// decode_match.hpp -- pairing of call-entry and return events, per thread.
//
// A function's entry site and its return site see the same call: on one thread, calls nest like
// a stack, and the return address read at the entry (the top of the stack) is the address the
// return site's sample reports as its instruction pointer.  The pairing is therefore exact where
// the events are complete, and the unmatched ones are the "event-matching failures" of the
// observation-effect record (PI decision D1): an entry with no return (the call was still running
// at the end of the trace, or the return event was lost) and a return with no entry.
//
// Which sites form a pair is data: the same `symbol` in the site table, one ENTRY site and one
// RET site, both declaring slot 3 with role "return_address" (8 bytes; kz_raw.h, check_plan).  A
// pair whose sites do not declare it is not matched (sites_without_key).  An event with TRUNC set
// is never matched on its slot value (an unreadable slot is 0): it is returned alone with
// `unreadable_key` and counted in events_with_unreadable_key, not in the two failure counts.
#pragma once

#include <cstdint>
#include <vector>

#include "kz/trace/decode_sites.hpp"

namespace kz::trace {

struct CallPair {
  uint32_t entry_event = KZ_NONE32;  // index into the events
  uint32_t return_event = KZ_NONE32; // KZ_NONE32 when the return was not seen
  bool unreadable_key = false;       // the one event of this record had TRUNC set: it was not matched
};

struct MatchStats {
  uint64_t pairs = 0;
  uint64_t entries_without_return = 0;
  uint64_t returns_without_entry = 0;
  uint64_t sites_paired = 0;         // (entry site, return site) pairs found in the site table
  uint64_t sites_without_key = 0;    // pairs of the same symbol not matched: slot 3 is not a return_address
  uint64_t events_with_unreadable_key = 0;  // events of paired sites with TRUNC set, left unmatched
};

// Pairs the events of every (entry site, return site) of the same symbol.  `events` are in the
// merged order.  Pairs are returned in the order of their entry events; calls without a return
// and returns without an entry are included with KZ_NONE32 on the missing side.
std::vector<CallPair> match_calls(const std::vector<kz_raw_ev>& events, const std::vector<SiteSpec>& sites, MatchStats* stats);

}  // namespace kz::trace

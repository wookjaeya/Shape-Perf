// identity_join.cpp -- see include/kz/trace/identity_join.hpp.
#include "kz/trace/identity_join.hpp"

#include <algorithm>
#include <cstring>
#include <map>
#include <unordered_map>

namespace kz::trace {

namespace {

struct Incarnation {
  uint32_t pid = 0, tid = 0, generation = 0;
  uint32_t ptid = KZ_NONE32;
  uint32_t announced = KZ_NONE32;
  uint32_t flags = 0;
  uint32_t creator_event = KZ_NONE32;
  uint32_t first_event = KZ_NONE32;
  uint64_t t_fork = 0, t_exit = 0, t_name = 0, entry_fn = 0;
  uint32_t priority = 0, n_events = 0;
  std::string name;
  bool has_fork = false;
};

struct Interval {
  uint32_t creator_tid = 0;
  uint32_t create_event = 0;
  bool closed = false;
  uint64_t t_begin = 0, t_end = 0;
  uint64_t entry_fn = 0;
  uint32_t priority = 0;
};

}  // namespace

Status join_identities(const JoinInput& in, std::vector<ThreadRecord>* out, JoinStats* stats) {
  JoinStats local;
  JoinStats& st = stats != nullptr ? *stats : local;
  st = JoinStats();
  out->clear();
  if (in.events == nullptr || in.sites == nullptr || in.side == nullptr)
    return Status::error(Err::Invalid, "join_identities: events, sites and side events are required");
  const std::vector<kz_raw_ev>& events = *in.events;
  const std::vector<SiteSpec>& sites = *in.sites;
  // The decoders read fixed slot positions (kz_raw.h): a site that does not declare them would be
  // read as zeros.  Refuse it instead.
  for (const SiteSpec& site : sites)
    if (site.decoder != KZ_DECODER_GENERIC && site.decoder != KZ_DECODER_NONE) {
      Status cs = check_site(site);
      if (!cs.ok()) return Status::error(Err::Invalid, "join_identities: " + cs.message);
    }

  // 1. Thread incarnations from the side-band stream, in time order.
  std::vector<Incarnation> inc;
  std::unordered_map<uint32_t, std::vector<uint32_t>> by_tid;  // tid -> incarnation indices, by creation order
  auto latest = [&](uint32_t tid) -> Incarnation* {
    auto it = by_tid.find(tid);
    return it == by_tid.end() ? nullptr : &inc[it->second.back()];
  };
  auto make = [&](uint32_t pid, uint32_t tid) -> Incarnation& {
    Incarnation n;
    n.pid = pid;
    n.tid = tid;
    std::vector<uint32_t>& v = by_tid[tid];
    n.generation = static_cast<uint32_t>(v.size());
    if (n.generation > 0) {
      n.flags |= KZ_THREAD_FLAG_TID_REUSED;
      ++st.tid_reuses;
    }
    v.push_back(static_cast<uint32_t>(inc.size()));
    inc.push_back(n);
    return inc.back();
  };
  std::vector<const SideEvent*> sorted_side;
  sorted_side.reserve(in.side->size());
  for (const SideEvent& s : *in.side) sorted_side.push_back(&s);
  std::stable_sort(sorted_side.begin(), sorted_side.end(), [](const SideEvent* a, const SideEvent* b) { return a->rec.t_ns < b->rec.t_ns; });

  for (const SideEvent* se : sorted_side) {
    const kz_side_ev& r = se->rec;
    switch (r.kind) {
      case KZ_SIDE_KIND_FORK: {
        Incarnation& n = make(r.pid, r.tid);
        n.has_fork = true;
        n.ptid = r.ptid;
        n.t_fork = r.t_ns;
        n.flags |= KZ_THREAD_FLAG_FORK_SEEN;
        break;
      }
      case KZ_SIDE_KIND_COMM: {
        Incarnation* n = latest(r.tid);
        if (n == nullptr || (n->flags & KZ_THREAD_FLAG_EXIT_SEEN) != 0) {
          // A thread that existed before the observer saw it (or after its exit record): it has no FORK.
          n = &make(r.pid, r.tid);
          n->t_fork = r.t_ns;
        }
        n->name = se->comm;
        n->t_name = r.t_ns;
        n->flags |= KZ_THREAD_FLAG_COMM_SEEN;
        break;
      }
      case KZ_SIDE_KIND_EXIT: {
        Incarnation* n = latest(r.tid);
        if (n == nullptr || (n->flags & KZ_THREAD_FLAG_EXIT_SEEN) != 0) break;
        n->t_exit = r.t_ns;
        n->flags |= KZ_THREAD_FLAG_EXIT_SEEN;
        break;
      }
      default:
        break;
    }
  }

  // The incarnation of a tid that a raw event at time t belongs to: the latest one forked at or
  // before t; failing that the earliest one (a thread seen before its FORK was recorded).  A
  // thread no side event named gets a record of its own, without any evidence flag.
  auto incarnation_at = [&](uint32_t tid, uint64_t t) -> Incarnation* {
    auto it = by_tid.find(tid);
    if (it == by_tid.end()) return nullptr;
    Incarnation* pick = &inc[it->second.front()];
    for (uint32_t idx : it->second)
      if (inc[idx].t_fork <= t) pick = &inc[idx];
    return pick;
  };

  // 2. Raw events: first event, counts, child-side announcements, creator-side intervals.
  std::unordered_map<uint32_t, std::vector<Interval>> open_creates;  // creator tid -> stack of open creates
  std::vector<Interval> intervals;
  std::vector<uint32_t> synthetic_pid_tids;
  for (uint32_t i = 0; i < events.size(); ++i) {
    const kz_raw_ev& ev = events[i];
    Incarnation* n = incarnation_at(ev.tid, ev.t_ns);
    if (n == nullptr) {
      ++st.events_without_thread;
      n = &make(0, ev.tid);  // pid unknown: no side event named the thread
      n->t_fork = 0;
    }
    if (n->first_event == KZ_NONE32) n->first_event = i;
    ++n->n_events;
    if (ev.site >= sites.size()) continue;
    const SiteSpec& site = sites[ev.site];
    // An event with TRUNC set may hold 0 in the very slot a decoder reads: it is counted, never
    // used as a key or as an announced id.
    const bool unreadable = (ev.flags & KZ_RAW_FLAG_TRUNC) != 0;
    switch (site.decoder) {
      case KZ_DECODER_THREAD_ENTRY:
        if (unreadable) {
          ++st.entries_unreadable;
        } else if ((n->flags & KZ_THREAD_FLAG_ENTRY_SEEN) != 0) {
          ++st.duplicate_entries;
        } else {
          n->announced = static_cast<uint32_t>(ev.a[0] & 0xffffffffu);
          // Nothing on the creator side can be compared with it: the id the spawn call assigns is
          // an out-parameter, which this backend cannot read (kz_raw.h, KZ_THREAD_FLAG).
          n->flags |= KZ_THREAD_FLAG_ENTRY_SEEN | KZ_THREAD_FLAG_ID_UNVERIFIED;
        }
        break;
      case KZ_DECODER_SPAWN_CREATE: {
        if (unreadable) {
          // Without its return address the call cannot be closed by its return, and without its
          // entry function it says nothing about the child: it opens no interval.
          ++st.creates_unreadable;
          break;
        }
        Interval iv;
        iv.creator_tid = ev.tid;
        iv.create_event = i;
        iv.t_begin = ev.t_ns;
        iv.entry_fn = ev.a[1];
        iv.priority = static_cast<uint32_t>(ev.a[2] & 0xffffffffu);
        // the return address of this call identifies its return event
        open_creates[ev.tid].push_back(iv);
        // stash the return address in t_end until the return is seen
        open_creates[ev.tid].back().t_end = ev.a[kSlotReturnAddress];
        break;
      }
      case KZ_DECODER_SPAWN_RETURN: {
        if (unreadable) {
          // The open call it would close stays open: forks after it become ambiguous, not assigned.
          ++st.returns_unreadable;
          break;
        }
        std::vector<Interval>& stack = open_creates[ev.tid];
        // The matching entry is the innermost open one with this return address.
        size_t k = stack.size();
        while (k > 0 && stack[k - 1].t_end != ev.a[kSlotReturnAddress]) --k;
        if (k == 0) {
          ++st.returns_without_create;
          break;
        }
        Interval iv = stack[k - 1];
        // Entries opened after it and still open have no return: their call did not come back
        // through a probed return (loss, or a thread that never returned).
        for (size_t j = k; j < stack.size(); ++j) {
          Interval abandoned = stack[j];
          abandoned.closed = false;
          abandoned.t_end = ev.t_ns;  // known to end no later than this
          ++st.creates_without_return;
          intervals.push_back(abandoned);
        }
        stack.resize(k - 1);
        iv.closed = true;
        iv.t_end = ev.t_ns;
        intervals.push_back(iv);
        break;
      }
      default:
        break;
    }
  }
  // Creates still open at the end of the trace.
  {
    std::vector<uint32_t> tids;
    for (const auto& kv : open_creates) tids.push_back(kv.first);
    std::sort(tids.begin(), tids.end());
    for (uint32_t tid : tids)
      for (Interval iv : open_creates[tid]) {
        iv.closed = false;
        iv.t_end = ~0ull;
        ++st.creates_without_return;
        intervals.push_back(iv);
      }
  }

  // 3. Match FORK records to creator-side intervals: innermost interval first.
  std::stable_sort(intervals.begin(), intervals.end(), [](const Interval& a, const Interval& b) {
    const uint64_t la = a.t_end - a.t_begin, lb = b.t_end - b.t_begin;
    if (la != lb) return la < lb;
    return a.create_event < b.create_event;
  });
  std::vector<bool> claimed(inc.size(), false);
  for (const Interval& iv : intervals) {
    std::vector<uint32_t> cands;
    for (uint32_t idx = 0; idx < inc.size(); ++idx) {
      const Incarnation& c = inc[idx];
      if (!c.has_fork || c.ptid != iv.creator_tid || claimed[idx]) continue;
      if (c.t_fork >= iv.t_begin && c.t_fork <= iv.t_end) cands.push_back(idx);
    }
    if (cands.empty()) {
      ++st.creates_without_fork;
      continue;
    }
    if (cands.size() > 1 || !iv.closed) {
      // More than one thread could belong to this call, or the call has no recorded end so
      // that later forks could also be its own: say so and assign nothing.
      for (uint32_t idx : cands) {
        inc[idx].flags |= KZ_THREAD_FLAG_SPAWN_AMBIGUOUS | KZ_THREAD_FLAG_MATCH_BY_TIME;
        claimed[idx] = true;
      }
      continue;
    }
    Incarnation& c = inc[cands[0]];
    claimed[cands[0]] = true;
    // The only key is containment of the FORK time in the creator's interval: timestamps of one
    // thread (the creator's probe events and the kernel's FORK record of that thread), so the
    // match holds under A-CLK-PO (SPEC 5.4).
    c.flags |= KZ_THREAD_FLAG_SPAWN_MATCHED | KZ_THREAD_FLAG_MATCH_BY_TIME;
    c.creator_event = iv.create_event;
    c.entry_fn = iv.entry_fn;
    c.priority = iv.priority;
  }

  // 4. Finish the records.
  std::vector<uint32_t> order(inc.size());
  for (uint32_t i = 0; i < order.size(); ++i) order[i] = i;
  std::sort(order.begin(), order.end(), [&](uint32_t a, uint32_t b) {
    if (inc[a].pid != inc[b].pid) return inc[a].pid < inc[b].pid;
    if (inc[a].tid != inc[b].tid) return inc[a].tid < inc[b].tid;
    return inc[a].generation < inc[b].generation;
  });
  for (uint32_t idx : order) {
    Incarnation& n = inc[idx];
    if ((n.flags & KZ_THREAD_FLAG_COMM_SEEN) == 0) {
      n.flags |= KZ_THREAD_FLAG_NAME_INHERITED;
      ++st.threads_without_name;
    }
    if (n.first_event != KZ_NONE32 && (n.flags & KZ_THREAD_FLAG_COMM_SEEN) != 0 && n.t_name > events[n.first_event].t_ns) {
      n.flags |= KZ_THREAD_FLAG_NAME_AFTER_FIRST_EVENT;
      ++st.names_after_first_event;
    }
    if ((n.flags & KZ_THREAD_FLAG_SPAWN_MATCHED) != 0) ++st.spawn_matched;
    if ((n.flags & KZ_THREAD_FLAG_SPAWN_AMBIGUOUS) != 0) ++st.spawn_ambiguous;
    if ((n.flags & KZ_THREAD_FLAG_MATCH_BY_TIME) != 0) ++st.matched_by_time;
    if ((n.flags & KZ_THREAD_FLAG_ID_UNVERIFIED) != 0) ++st.ids_unverified;
    if ((n.flags & KZ_THREAD_FLAG_ENTRY_SEEN) != 0 && (n.flags & KZ_THREAD_FLAG_SPAWN_MATCHED) == 0) {
      n.flags |= KZ_THREAD_FLAG_ENTRY_UNMATCHED;
      ++st.entries_unmatched;
    }
    ThreadRecord tr;
    std::memset(&tr.rec, 0, sizeof tr.rec);
    tr.rec.pid = n.pid;
    tr.rec.tid = n.tid;
    tr.rec.generation = n.generation;
    tr.rec.ptid = n.ptid;
    tr.rec.announced_id = n.announced;
    tr.rec.flags = n.flags;
    tr.rec.creator_event = n.creator_event;
    tr.rec.first_event = n.first_event;
    tr.rec.t_fork = n.has_fork ? n.t_fork : 0;
    tr.rec.t_exit = n.t_exit;
    tr.rec.t_name = n.t_name;
    tr.rec.entry_fn = n.entry_fn;
    tr.rec.priority = n.priority;
    tr.rec.n_events = n.n_events;
    tr.name = n.name;
    if (in.symbols != nullptr && in.bias_known && n.entry_fn != 0) {
      const SymbolEntry* sym = in.symbols->find(n.entry_fn - in.load_bias);
      if (sym != nullptr) tr.entry_symbol = sym->name;
    }
    out->push_back(std::move(tr));
    ++st.threads;
  }
  return Status::success();
}

}  // namespace kz::trace

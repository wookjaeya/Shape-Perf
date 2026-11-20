// identity_join.cpp -- see include/kz/trace/identity_join.hpp.
#include "kz/trace/identity_join.hpp"

#include "kz/trace/identity_rule.hpp"

#include <algorithm>
#include <cstring>
#include <functional>
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
  bool has_id = false;      // a SPAWN_ID event inside the call gave the id the spawn assigned
  bool id_unreadable = false;
  uint32_t assigned_id = 0;
  int32_t request = -1;     // the outermost creation request open on the creator thread at the call, or -1
};

// A creation request (OWNER_REQUEST or CHILD_REQUEST) on one thread, from its entry to its return.
struct Request {
  uint32_t tid = 0;
  uint32_t event = 0;
  bool child = false;
  bool owner_readable = false;
  uint32_t owner_id = 0;    // OWNER_REQUEST a[2]
  uint64_t ret_addr = 0;
  bool nested_owner_request = false;  // an OWNER_REQUEST ran inside this CHILD_REQUEST (its argument is not read)
};

}  // namespace

Status join_identities(const JoinInput& in, std::vector<ThreadRecord>* out, JoinStats* stats, std::vector<OwnerRecord>* owners) {
  JoinStats local;
  JoinStats& st = stats != nullptr ? *stats : local;
  st = JoinStats();
  out->clear();
  if (owners != nullptr) owners->clear();
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

  // 2. Raw events: first event, counts, child-side announcements, creator-side intervals, creation
  //    requests and the ids the spawn calls assign.
  std::unordered_map<uint32_t, std::vector<Interval>> open_creates;  // creator tid -> stack of open creates
  std::vector<Interval> intervals;
  std::vector<Request> requests;
  std::unordered_map<uint32_t, std::vector<uint32_t>> open_requests;  // tid -> stack of indices into requests
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
        {
          const auto rq = open_requests.find(ev.tid);
          if (rq != open_requests.end() && !rq->second.empty()) iv.request = static_cast<int32_t>(rq->second.front());
        }
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
      case KZ_DECODER_SPAWN_ID: {
        // The id the spawn call assigns, read where the call passes it to the new thread: it belongs
        // to the innermost spawn call open on this thread.  Outside one (a thread the framework starts
        // by other means) it says nothing about a spawn call.
        std::vector<Interval>& stack = open_creates[ev.tid];
        if (stack.empty()) {
          ++st.spawn_ids_outside_create;
          break;
        }
        Interval& iv = stack.back();
        if (iv.has_id || iv.id_unreadable) {
          ++st.spawn_ids_repeated;  // a second id in one call: neither is used
          iv.has_id = false;
          iv.id_unreadable = true;
          break;
        }
        if (unreadable) {
          iv.id_unreadable = true;
          ++st.spawn_ids_unreadable;
          break;
        }
        iv.has_id = true;
        iv.assigned_id = static_cast<uint32_t>(ev.a[0] & 0xffffffffu);
        break;
      }
      case KZ_DECODER_OWNER_REQUEST:
      case KZ_DECODER_CHILD_REQUEST: {
        if (unreadable) {
          // Its return address (and owner) may be a zero of a slot the kernel did not copy: it could
          // not be closed by its return, so it opens nothing (a spawn call inside it names no owner).
          ++st.requests_unreadable;
          break;
        }
        Request rq;
        rq.tid = ev.tid;
        rq.event = i;
        rq.child = site.decoder == KZ_DECODER_CHILD_REQUEST;
        rq.ret_addr = ev.a[kSlotReturnAddress];
        if (!rq.child) {
          rq.owner_readable = true;
          rq.owner_id = static_cast<uint32_t>(ev.a[2] & 0xffffffffu);
        }
        std::vector<uint32_t>& rs = open_requests[ev.tid];
        if (!rq.child)
          for (uint32_t r : rs)
            if (requests[r].child) requests[r].nested_owner_request = true;
        rs.push_back(static_cast<uint32_t>(requests.size()));
        requests.push_back(rq);
        ++st.requests;
        break;
      }
      case KZ_DECODER_REQUEST_RETURN: {
        if (unreadable) {
          ++st.requests_unreadable;
          break;
        }
        std::vector<uint32_t>& rs = open_requests[ev.tid];
        size_t k = rs.size();
        while (k > 0 && requests[rs[k - 1]].ret_addr != ev.a[kSlotReturnAddress]) --k;
        if (k == 0) {
          ++st.request_returns_without_request;
          break;
        }
        st.requests_without_return += rs.size() - k;  // requests opened after it never returned through a probe
        rs.resize(k - 1);
        break;
      }
      default:
        break;
    }
  }
  for (const auto& kv : open_requests) st.requests_without_return += kv.second.size();
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
  std::unordered_map<uint32_t, uint32_t> time_match;  // create_event -> incarnation matched by FORK time
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
    time_match[iv.create_event] = cands[0];
  }

  // 3b. The id the spawn call assigned against the id the child announced (SPEC 5.7 items 2-3).  A
  //     thread of the creator whose announced id equals it, forked inside the call, is the call's
  //     child by id: the link no longer rests on time alone.  An id that contradicts the time match
  //     leaves the link unestablished.  Nothing is decided by the id alone when two threads announce it.
  std::unordered_map<uint32_t, const Interval*> by_event;
  for (const Interval& iv : intervals) by_event[iv.create_event] = &iv;
  for (const Interval& iv : intervals) {
    if (!iv.has_id) continue;
    std::vector<uint32_t> cands;
    for (uint32_t idx = 0; idx < inc.size(); ++idx) {
      const Incarnation& c = inc[idx];
      if (!c.has_fork || c.ptid != iv.creator_tid || (c.flags & KZ_THREAD_FLAG_ENTRY_SEEN) == 0) continue;
      if (c.announced != iv.assigned_id || c.t_fork < iv.t_begin || c.t_fork > iv.t_end) continue;
      cands.push_back(idx);
    }
    const auto tm = time_match.find(iv.create_event);
    if (tm != time_match.end()) {
      Incarnation& c = inc[tm->second];
      if ((c.flags & KZ_THREAD_FLAG_ENTRY_SEEN) != 0 && c.announced != iv.assigned_id) {
        // The thread the FORK time gave this call announced another id.
        c.flags |= KZ_THREAD_FLAG_ID_MISMATCH;
        c.flags &= ~static_cast<uint32_t>(KZ_THREAD_FLAG_SPAWN_MATCHED);
        c.creator_event = KZ_NONE32;
        ++st.ids_mismatched;
        continue;
      }
    }
    if (cands.size() != 1) continue;
    Incarnation& c = inc[cands[0]];
    if ((c.flags & KZ_THREAD_FLAG_SPAWN_MATCHED) != 0 && c.creator_event != iv.create_event) {
      c.flags |= KZ_THREAD_FLAG_ID_MISMATCH;  // matched by time to another call
      c.flags &= ~static_cast<uint32_t>(KZ_THREAD_FLAG_SPAWN_MATCHED);
      c.creator_event = KZ_NONE32;
      ++st.ids_mismatched;
      continue;
    }
    c.flags |= KZ_THREAD_FLAG_SPAWN_MATCHED | KZ_THREAD_FLAG_ID_MATCHED;
    c.flags &= ~static_cast<uint32_t>(KZ_THREAD_FLAG_SPAWN_AMBIGUOUS | KZ_THREAD_FLAG_MATCH_BY_TIME | KZ_THREAD_FLAG_ID_UNVERIFIED);
    c.creator_event = iv.create_event;
    c.entry_fn = iv.entry_fn;
    c.priority = iv.priority;
    ++st.ids_matched;
  }

  // 4. Finish the records.
  std::vector<uint32_t> order(inc.size());
  for (uint32_t i = 0; i < order.size(); ++i) order[i] = i;
  std::sort(order.begin(), order.end(), [&](uint32_t a, uint32_t b) {
    if (inc[a].pid != inc[b].pid) return inc[a].pid < inc[b].pid;
    if (inc[a].tid != inc[b].tid) return inc[a].tid < inc[b].tid;
    return inc[a].generation < inc[b].generation;
  });
  std::vector<uint32_t> out_index(inc.size(), KZ_NONE32);
  for (uint32_t k = 0; k < order.size(); ++k) out_index[order[k]] = k;
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

  // 5. app_owner (SPEC 1.2 "Creating application", 5.7, ADR-056): from the creation side only.  The
  //    outermost creation request open on the creator thread when the spawn call began says which
  //    application the new task belongs to: an OWNER_REQUEST by its argument, a CHILD_REQUEST by the
  //    creating task's own app_owner.  The level is `confirmed` only when the child's announced id
  //    equals the id the spawn call assigned (ID_MATCHED); a link by FORK time alone is `inferred`
  //    (match-by-time, id-unverified).  Nothing is looked up in the process or in a framework table.
  std::vector<OwnerRecord> own(inc.size());
  std::vector<bool> done(inc.size(), false);
  std::vector<uint32_t> by_fork(inc.size());
  for (uint32_t i = 0; i < by_fork.size(); ++i) by_fork[i] = i;
  std::stable_sort(by_fork.begin(), by_fork.end(), [&](uint32_t a, uint32_t b) { return inc[a].t_fork < inc[b].t_fork; });
  // Recursive: a child's owner is read from its creator's (a cycle cannot occur in kernel records; a
  // record already being computed is returned as it stands, i.e. undetermined).
  std::function<OwnerRecord&(uint32_t)> owner_of = [&](uint32_t idx) -> OwnerRecord& {
    OwnerRecord& o = own[idx];
    if (done[idx]) return o;
    done[idx] = true;
    const Incarnation& n = inc[idx];
    std::memset(&o.rec, 0, sizeof o.rec);
    o.rec.thread = out_index[idx];
    o.rec.creator_thread = KZ_NONE32;
    o.rec.creation_event = KZ_NONE32;
    o.rec.owner_key = KZ_NONE32;
    o.rec.level = KZ_OWNER_LEVEL_UNDETERMINED;
    o.rec.evidence = 0;
    if (n.has_fork && n.ptid != KZ_NONE32) {
      Incarnation* cr = incarnation_at(n.ptid, n.t_fork);
      if (cr != nullptr) o.rec.creator_thread = out_index[static_cast<size_t>(cr - inc.data())];
    }
    if ((n.flags & KZ_THREAD_FLAG_SPAWN_MATCHED) == 0) {
      if ((n.flags & KZ_THREAD_FLAG_SPAWN_AMBIGUOUS) != 0) o.rec.evidence |= KZ_OWNER_EVIDENCE_MATCH_BY_TIME;
      else if (n.has_fork && (n.flags & KZ_THREAD_FLAG_ID_MISMATCH) == 0) o.rec.evidence |= KZ_OWNER_EVIDENCE_RAW_THREAD;  // created without a probed spawn call
      return o;
    }
    const auto ivp = by_event.find(n.creator_event);
    const Interval* iv = ivp != by_event.end() ? ivp->second : nullptr;
    const bool by_id = (n.flags & KZ_THREAD_FLAG_ID_MATCHED) != 0;
    if (iv != nullptr && n.t_fork >= iv->t_begin && n.t_fork <= iv->t_end) o.rec.evidence |= KZ_OWNER_EVIDENCE_FORK_IN_CALL;
    o.rec.evidence |= by_id ? KZ_OWNER_EVIDENCE_ANNOUNCED_ID : (KZ_OWNER_EVIDENCE_MATCH_BY_TIME | KZ_OWNER_EVIDENCE_ID_UNVERIFIED);
    o.rec.creation_event = n.creator_event;
    if (iv == nullptr || iv->request < 0) return o;  // a spawn call outside any creation request: the application is not named
    const Request& rq = requests[static_cast<size_t>(iv->request)];
    o.rec.creation_event = rq.event;
    // The rule both families use (identity_rule.hpp): the request names the application, a child request through
    // the creating task's own owner.
    CreationRequest cq;
    cq.child = rq.child;
    cq.owner_readable = rq.owner_readable;
    cq.owner_id = rq.owner_id;
    cq.nested_owner_request = rq.nested_owner_request;
    const OwnerRecord* co = nullptr;
    if (rq.child) {
      Incarnation* cr = n.has_fork ? incarnation_at(n.ptid, n.t_fork) : nullptr;
      if (cr != nullptr && cr != &n) co = &owner_of(static_cast<uint32_t>(cr - inc.data()));
    }
    OwnerDecision od;
    od.evidence = o.rec.evidence;
    OwnerDecision cd;
    if (co != nullptr) cd.owner_key = co->rec.owner_key, cd.level = co->rec.level, cd.evidence = co->rec.evidence;
    const OwnerSource src = owner_from_request(cq, by_id ? KZ_OWNER_LEVEL_CONFIRMED : KZ_OWNER_LEVEL_INFERRED, co != nullptr ? &cd : nullptr, &od);
    o.rec.owner_key = od.owner_key;
    o.rec.level = od.level;
    o.rec.evidence = od.evidence;
    if (src == OwnerSource::Request) o.app_name = n.name;
    else if (src == OwnerSource::Creator) o.app_name = co->app_name;
    return o;
  };
  for (uint32_t idx : by_fork) owner_of(idx);
  for (uint32_t idx : order) {
    const OwnerRecord& o = own[idx];
    if (o.rec.level == KZ_OWNER_LEVEL_CONFIRMED) ++st.owners_confirmed;
    else if (o.rec.level == KZ_OWNER_LEVEL_INFERRED) ++st.owners_inferred;
    else ++st.owners_undetermined;
    if (owners != nullptr) owners->push_back(o);
  }
  return Status::success();
}

}  // namespace kz::trace

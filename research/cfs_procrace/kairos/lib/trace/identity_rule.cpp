// identity_rule.cpp -- see include/kz/trace/identity_rule.hpp.
#include "kz/trace/identity_rule.hpp"

#include <algorithm>
#include <functional>
#include <map>
#include <set>

namespace kz::trace {

OwnerSource owner_from_request(const CreationRequest& rq, uint8_t tie_level, const OwnerDecision* creator, OwnerDecision* o) {
  o->evidence |= KZ_OWNER_EVIDENCE_CREATION_REQUEST;
  if (!rq.child) {
    if (!rq.owner_readable) return OwnerSource::None;
    o->owner_key = rq.owner_id;
    o->level = tie_level;
    return OwnerSource::Request;
  }
  // A child task: the application of the task that asked for it.
  o->evidence |= KZ_OWNER_EVIDENCE_CREATOR_OWNER;
  if (rq.nested_owner_request) o->evidence |= KZ_OWNER_EVIDENCE_ANALYSED_TABLE_EXCLUDED;
  if (creator == nullptr || creator->level == KZ_OWNER_LEVEL_UNDETERMINED || creator->owner_key == KZ_NONE32) return OwnerSource::None;
  o->owner_key = creator->owner_key;
  o->level = std::min<uint8_t>(tie_level, creator->level);
  return OwnerSource::Creator;
}

namespace {

struct Call {
  uint32_t actor = 0, open_event = 0;
  uint32_t return_event = KZ_NONE32;
  bool has_id = false, id_unusable = false;
  uint32_t id = 0;
  int32_t request = -1;  // index into requests: the outermost creation request open on the actor at the call
};

struct Request {
  uint32_t actor = 0, event = 0;
  CreationRequest rq;
  std::string func;
};

// The value of `slot` of event i, if it was observed.
bool value_of(const NeutralRun& n, uint32_t i, int slot, uint64_t* v) {
  if (slot < 0) return false;
  const kz_nevent& e = n.events[i];
  for (uint32_t k = e.first_value; k < e.first_value + e.nvals && k < n.values.size(); ++k)
    if (n.values[k].r.slot == static_cast<uint8_t>(slot)) {
      *v = n.values[k].r.v;
      return true;
    }
  return false;
}

uint64_t width_mask(uint8_t w) { return w >= 8 ? ~0ull : ((1ull << (8u * w)) - 1u); }

}  // namespace

Status join_actor_identities(const std::vector<IdentityRead>& reads, NeutralRun* n, ActorJoinStats* stats) {
  ActorJoinStats local;
  ActorJoinStats& st = stats != nullptr ? *stats : local;
  st = ActorJoinStats();
  if (reads.size() != n->sites.size()) return Status::error(Err::Invalid, "join_actor_identities: one identity read per site is required");
  const size_t na = n->actors.size();
  auto takes_part = [&](uint32_t a) { return a < na && (n->actors[a].r.binding == KZ_NBIND_KZRT_ACTOR || n->actors[a].r.binding == KZ_NBIND_KZRT_PROVISIONAL); };

  // 1. Per actor in aseq order (the container order keeps every actor's aseq order).
  std::vector<Call> calls;
  std::vector<Request> requests;
  std::map<uint32_t, std::vector<uint32_t>> open_calls, open_requests;  // actor -> stack of indices
  std::vector<bool> entry_seen(na, false);
  std::vector<uint32_t> announced(na, KZ_NONE32);
  for (uint32_t i = 0; i < n->events.size(); ++i) {
    const kz_nevent& e = n->events[i];
    if (e.site >= reads.size() || reads[e.site].decoder == KZ_DECODER_NONE) continue;
    const IdentityRead& r = reads[e.site];
    ++st.identity_events;
    if (!takes_part(e.actor)) {
      ++st.events_without_actor;
      continue;
    }
    const uint32_t a = e.actor;
    uint64_t v = 0;
    switch (r.decoder) {
      case KZ_DECODER_SPAWN_CREATE: {
        Call c;
        c.actor = a;
        c.open_event = i;
        const auto rq = open_requests.find(a);
        if (rq != open_requests.end() && !rq->second.empty()) c.request = static_cast<int32_t>(rq->second.front());
        open_calls[a].push_back(static_cast<uint32_t>(calls.size()));
        calls.push_back(c);
        ++st.creates;
        break;
      }
      case KZ_DECODER_SPAWN_ID: {
        std::vector<uint32_t>& stack = open_calls[a];
        if (stack.empty()) {
          ++st.spawn_ids_outside_create;
          break;
        }
        Call& c = calls[stack.back()];
        if (c.has_id || c.id_unusable) {
          ++st.spawn_ids_repeated;  // a second id in one call: neither is used
          c.has_id = false;
          c.id_unusable = true;
          break;
        }
        if (!value_of(*n, i, r.id_slot, &v)) {
          ++st.spawn_ids_unreadable;
          c.id_unusable = true;
          break;
        }
        c.has_id = true;
        c.id = static_cast<uint32_t>(v & 0xffffffffu);
        break;
      }
      case KZ_DECODER_SPAWN_RETURN: {
        std::vector<uint32_t>& stack = open_calls[a];
        size_t k = stack.size();
        while (k > 0 && (e.link == KZ_NONE32 || calls[stack[k - 1]].open_event != e.link)) --k;
        if (k == 0) {
          ++st.returns_without_create;
          break;
        }
        st.creates_without_return += stack.size() - k;  // opened after it, never returned through a hook
        Call& c = calls[stack[k - 1]];
        stack.resize(k - 1);
        c.return_event = i;
        if (r.id_slot < 0) break;
        uint64_t status = 0;
        if (!value_of(*n, i, r.status_slot, &status) || (status & width_mask(r.status_width)) != (r.ok & width_mask(r.status_width))) {
          ++st.out_ids_not_ok;  // the call did not report success: its out parameter is not the id it assigned
          break;
        }
        if (!value_of(*n, i, r.id_slot, &v)) break;
        const uint32_t out = static_cast<uint32_t>(v & 0xffffffffu);
        if (c.has_id && c.id != out) {
          ++st.ids_conflicting;  // the id passed to the new task and the id the call returned disagree: neither is used
          c.has_id = false;
          c.id_unusable = true;
        } else if (!c.has_id && !c.id_unusable) {
          c.has_id = true;
          c.id = out;
          ++st.out_ids_used;
        }
        break;
      }
      case KZ_DECODER_OWNER_REQUEST:
      case KZ_DECODER_CHILD_REQUEST: {
        Request q;
        q.actor = a;
        q.event = i;
        q.rq.child = r.decoder == KZ_DECODER_CHILD_REQUEST;
        q.func = r.request_func;
        if (!q.rq.child && value_of(*n, i, r.owner_slot, &v)) {
          q.rq.owner_readable = true;
          q.rq.owner_id = static_cast<uint32_t>(v & 0xffffffffu);
        }
        std::vector<uint32_t>& rs = open_requests[a];
        if (!q.rq.child)
          for (uint32_t x : rs)
            if (requests[x].rq.child) requests[x].rq.nested_owner_request = true;
        rs.push_back(static_cast<uint32_t>(requests.size()));
        requests.push_back(q);
        ++st.requests;
        break;
      }
      case KZ_DECODER_REQUEST_RETURN: {
        std::vector<uint32_t>& rs = open_requests[a];
        size_t k = rs.size();
        while (k > 0 && (e.link == KZ_NONE32 || requests[rs[k - 1]].event != e.link)) --k;
        if (k == 0) {
          ++st.request_returns_without_request;
          break;
        }
        st.requests_without_return += rs.size() - k;
        rs.resize(k - 1);
        break;
      }
      case KZ_DECODER_THREAD_ENTRY: {
        if (entry_seen[a]) {
          ++st.duplicate_entries;
          break;
        }
        if (!value_of(*n, i, r.id_slot, &v)) {
          ++st.entries_unreadable;
          break;
        }
        entry_seen[a] = true;
        announced[a] = static_cast<uint32_t>(v & 0xffffffffu);
        ++st.entries;
        break;
      }
      default:
        break;
    }
  }
  for (const auto& kv : open_calls) st.creates_without_return += kv.second.size();
  for (const auto& kv : open_requests) st.requests_without_return += kv.second.size();

  // 2. The tie by the id: one call assigned it, one actor announced it.
  std::map<uint32_t, std::vector<uint32_t>> assigned_by, announced_by;  // id -> calls / actors
  for (uint32_t c = 0; c < calls.size(); ++c)
    if (calls[c].has_id) assigned_by[calls[c].id].push_back(c);
  for (uint32_t a = 0; a < na; ++a)
    if (entry_seen[a]) announced_by[announced[a]].push_back(a);
  std::vector<int64_t> tie(na, -1);  // actor -> call
  for (uint32_t a = 0; a < na; ++a) {
    if (!takes_part(a)) continue;
    NActor& x = n->actors[a];
    if (!entry_seen[a]) continue;
    x.r.announced_id = announced[a];
    x.r.join_flags |= KZ_THREAD_FLAG_ENTRY_SEEN | KZ_THREAD_FLAG_ID_UNVERIFIED;
    const auto as = assigned_by.find(announced[a]);
    const size_t n_calls = as == assigned_by.end() ? 0 : as->second.size();
    const size_t n_actors = announced_by[announced[a]].size();
    if (n_calls == 0) continue;
    if (n_calls > 1 || n_actors > 1) {
      x.r.join_flags |= KZ_THREAD_FLAG_SPAWN_AMBIGUOUS;  // the id alone cannot tell which call or which task
      ++st.ambiguous_ids;
      continue;
    }
    const Call& c = calls[as->second.front()];
    if (c.actor == a) continue;  // a task does not create itself
    tie[a] = as->second.front();
    x.r.join_flags |= KZ_THREAD_FLAG_SPAWN_MATCHED | KZ_THREAD_FLAG_ID_MATCHED;
    x.r.join_flags &= ~static_cast<uint32_t>(KZ_THREAD_FLAG_ID_UNVERIFIED);
    x.r.creator = c.actor;
    x.r.creator_event = c.open_event;
    x.r.spawn_return = c.return_event;
    ++st.ids_matched;
  }
  for (uint32_t a = 0; a < na; ++a)
    if (takes_part(a) && entry_seen[a] && tie[a] < 0) {
      n->actors[a].r.join_flags |= KZ_THREAD_FLAG_ENTRY_UNMATCHED;
      ++st.entries_unmatched;
    }

  // 3. The owners (owner_from_request), the creator's first where a child request needs it.
  std::vector<OwnerDecision> own(na);
  std::vector<int> state(na, 0);  // 0 not computed, 1 in progress, 2 done
  std::function<const OwnerDecision&(uint32_t)> owner_of = [&](uint32_t a) -> const OwnerDecision& {
    OwnerDecision& o = own[a];
    if (state[a] != 0) return o;  // done, or in progress (a cycle cannot be a creation: undetermined as it stands)
    state[a] = 1;
    NActor& x = n->actors[a];
    x.r.owner_creator = KZ_NONE32;
    x.r.owner_request = KZ_NONE32;
    if (tie[a] >= 0) {
      const Call& c = calls[static_cast<size_t>(tie[a])];
      o.evidence |= KZ_OWNER_EVIDENCE_ANNOUNCED_ID;
      x.r.owner_creator = c.actor;
      x.r.owner_request = c.open_event;
      if (c.request >= 0) {
        const Request& q = requests[static_cast<size_t>(c.request)];
        x.r.owner_request = q.event;
        x.request_func = q.func;
        const OwnerDecision* co = nullptr;
        if (q.rq.child && c.actor != a) co = &owner_of(c.actor);
        const OwnerSource src = owner_from_request(q.rq, KZ_OWNER_LEVEL_CONFIRMED, co, &o);
        if (src == OwnerSource::Creator) x.owner_app = n->actors[c.actor].owner_app;
      }
    }
    state[a] = 2;
    return o;
  };
  for (uint32_t a = 0; a < na; ++a) {
    if (!takes_part(a)) continue;
    const OwnerDecision& o = owner_of(a);
    NActor& x = n->actors[a];
    x.r.flags |= KZ_NACTOR_FLAG_OWNER_RECORDED;
    x.r.owner_level = o.level;
    x.r.owner_evidence = o.evidence;
    x.r.owner_id = o.owner_key;
    if (o.level == KZ_OWNER_LEVEL_CONFIRMED) ++st.owners_confirmed;
    else ++st.owners_undetermined;
  }
  n->run.r.flags |= KZ_NRUN_FLAG_OWNERS_RECORDED;
  return Status::success();
}

std::string actor_join_text(const ActorJoinStats& s) {
  auto f = [](const char* k, uint64_t v) { return std::string(k) + " " + std::to_string(v); };
  std::string t = "identity join (by the id; no creation record): ";
  t += f("identity events", s.identity_events) + ", " + f("of unresolved actors (not used)", s.events_without_actor) + "; ";
  t += f("announced ids", s.entries) + ", " + f("unreadable", s.entries_unreadable) + ", " + f("repeated", s.duplicate_entries) + "; ";
  t += f("spawn calls", s.creates) + ", " + f("without return", s.creates_without_return) + ", " + f("returns without call", s.returns_without_create) + "; ";
  t += f("spawn ids outside a call", s.spawn_ids_outside_create) + ", " + f("unreadable", s.spawn_ids_unreadable) + ", " + f("repeated", s.spawn_ids_repeated) + "; ";
  t += f("out ids used", s.out_ids_used) + ", " + f("not success", s.out_ids_not_ok) + ", " + f("conflicting", s.ids_conflicting) + "; ";
  t += f("requests", s.requests) + ", " + f("without return", s.requests_without_return) + ", " + f("returns without request", s.request_returns_without_request) + "; ";
  t += f("tied by the id", s.ids_matched) + ", " + f("ambiguous", s.ambiguous_ids) + ", " + f("announced but not tied", s.entries_unmatched) + "; ";
  t += f("owners confirmed", s.owners_confirmed) + ", " + f("undetermined", s.owners_undetermined);
  return t;
}

}  // namespace kz::trace

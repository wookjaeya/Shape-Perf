// neutral.cpp -- the neutral event layer in memory: F0 conversion, container reading and writing
// (see neutral.hpp).

#include "kz/trace/neutral.hpp"

#include <algorithm>
#include <cstring>
#include <map>
#include <set>

#include "kz/core/record_codec.hpp"
#include "kz/core/registry.hpp"
#include "kz/trace/decode_sites.hpp"
#include "kz/trace/identity_symbols.hpp"

namespace kz::trace {

namespace {

template <class T>
T zeroed() {
  T v;
  std::memset(&v, 0, sizeof v);
  return v;
}

std::string enum_text(const char* en, uint64_t v) {
  const EnumDesc* d = find_enum(en);
  const char* t = d != nullptr ? enum_display(*d, v) : nullptr;
  return t != nullptr ? t : "#" + std::to_string(v);
}

std::string hex64(uint64_t v) {
  static const char* const kDigits = "0123456789abcdef";
  std::string s;
  do {
    s.insert(s.begin(), kDigits[v & 0xF]);
    v >>= 4;
  } while (v != 0);
  return "0x" + s;
}

uint32_t first_at_or_after(const std::vector<kz_raw_ev>& ev, uint64_t t) {
  for (size_t i = 0; i < ev.size(); ++i)
    if (ev[i].t_ns >= t) return static_cast<uint32_t>(i);
  return static_cast<uint32_t>(ev.size());
}

uint32_t first_after(const std::vector<kz_raw_ev>& ev, uint64_t t) {
  for (size_t i = 0; i < ev.size(); ++i)
    if (ev[i].t_ns > t) return static_cast<uint32_t>(i);
  return static_cast<uint32_t>(ev.size());
}

uint8_t nloss_of_source(uint8_t source) {
  switch (source) {
    case KZ_LOSS_SOURCE_PERF_RECORD_LOST: return KZ_NLOSS_RECORDS_LOST;
    case KZ_LOSS_SOURCE_PERF_THROTTLE: return KZ_NLOSS_THROTTLE;
    case KZ_LOSS_SOURCE_RING_RESERVE_FAIL: return KZ_NLOSS_RING_RESERVE_FAIL;
    case KZ_LOSS_SOURCE_TSEQ_GAP: return KZ_NLOSS_TSEQ_GAP;
    default: return KZ_NLOSS_UNKNOWN;
  }
}

}  // namespace

kz_ref self_ref(uint32_t section, uint32_t index) {
  kz_ref r = zeroed<kz_ref>();
  r.section = section;
  r.index = index;
  return r;
}

kz_nevent nevent_init() {
  kz_nevent e = zeroed<kz_nevent>();
  e.link = KZ_NONE32;
  e.site = KZ_NONE32;
  e.raw_off = KZ_NONE64;
  return e;
}

kz_nactor nactor_init() {
  kz_nactor a = zeroed<kz_nactor>();
  a.process = KZ_NONE32;
  a.creator = KZ_NONE32;
  a.announced_id = KZ_NONE32;
  a.creator_event = KZ_NONE32;
  a.first_event = KZ_NONE32;
  a.spawn_return = KZ_NONE32;
  a.boot_link = KZ_NONE32;
  a.owner_id = KZ_NONE32;
  a.owner_creator = KZ_NONE32;
  a.owner_request = KZ_NONE32;
  return a;
}

kz_nloss nloss_init() {
  kz_nloss l = zeroed<kz_nloss>();
  l.end = KZ_NONE32;
  l.actor = KZ_NONE32;
  l.unit = KZ_NONE32;
  return l;
}

kz_nctl nctl_init() {
  kz_nctl c = zeroed<kz_nctl>();
  c.subject = KZ_NONE32;
  c.other = KZ_NONE32;
  c.event = KZ_NONE32;
  c.site = KZ_NONE32;
  c.raw_off = KZ_NONE64;
  return c;
}

// ---------------------------------------------------------------------------------------------------------
// F0 -> neutral.  The identity and loss rules are those the replay applied to the F0 sections before
// revision 3 (real_trace.cpp at 78ff6ee), unchanged.
// ---------------------------------------------------------------------------------------------------------
Status neutral_from_f0(const Container& run, NeutralRun* out) {
  *out = NeutralRun();
  if (run.header().artifact_kind != KZ_ARTIFACT_RUN) return Status::error(Err::Invalid, "not a run container");
  std::vector<kz_raw_ev> events;
  std::vector<kz_site> sites;
  std::vector<kz_site_slot> slots;
  std::vector<kz_thread> threads;
  std::vector<kz_map_ev> maps;
  std::vector<kz_runmeta> meta;
  std::vector<kz_loss> losses;
  std::vector<kz_app_owner> owners;
  Status s = run.read_section(KZ_SEC_RAW_EVENTS, &events);
  if (s.ok()) s = run.read_section(KZ_SEC_SITES, &sites);
  if (s.ok()) s = run.read_section(KZ_SEC_SITE_SLOTS, &slots);
  if (s.ok()) s = run.read_section(KZ_SEC_THREADS, &threads);
  if (s.ok()) s = run.read_section(KZ_SEC_MAPS, &maps);
  if (s.ok()) s = run.read_section(KZ_SEC_RUNMETA, &meta);
  if (s.ok()) s = run.read_section(KZ_SEC_LOSS, &losses);
  if (s.ok()) s = run.read_section(KZ_SEC_ACTOR_OWNERS, &owners);
  std::vector<kz_ev_str> strings;  // the texts of string slots (containers with fetch slots only)
  if (s.ok() && run.find_section(KZ_SEC_EVENT_STRINGS) != nullptr) s = run.read_section(KZ_SEC_EVENT_STRINGS, &strings);
  if (!s.ok()) return s;
  if (events.size() >= KZ_NONE32 || threads.size() >= KZ_NONE32 / 2) return Status::error(Err::Invalid, "the run container is too large for the neutral layer");
  std::map<std::pair<uint32_t, uint8_t>, std::string> text_of;
  for (const kz_ev_str& e : strings) {
    std::string t;
    if (!run.string(e.text, &t)) return Status::error(Err::BadRef, "EVENT_STRINGS: a string does not resolve");
    text_of[{e.event, e.slot}] = t;
  }

  // ---- the run ----
  NRun& nr = out->run;
  nr.r.backend = KZ_NBACKEND_F0;
  nr.r.clock = KZ_NCLOCK_MONOTONIC;  // kz_raw_ev.t_ns: CLOCK_MONOTONIC (docs/schema.md section 9)
  nr.r.flags = KZ_NRUN_FLAG_COMPLETE;
  const bool flagged = !meta.empty() && (meta.front().flags & KZ_RUN_FLAG_LOSS_PRESENT) != 0;
  if (!meta.empty()) {
    nr.r.flags |= KZ_NRUN_FLAG_LOSS_EVIDENCE;
    nr.r.raw_meta = self_ref(KZ_SEC_RUNMETA, 0);
    nr.backend_name = enum_text("KZ_BACKEND", meta.front().backend);
  }
  if (flagged || !losses.empty() || meta.empty()) nr.r.flags |= KZ_NRUN_FLAG_LOSS_PRESENT;
  if (run.find_section(KZ_SEC_ACTOR_OWNERS) != nullptr) nr.r.flags |= KZ_NRUN_FLAG_OWNERS_RECORDED;
  if (!events.empty())
    for (const kz_map_ev& mp : maps)
      if (mp.t_ns <= events.front().t_ns) nr.r.start_evidence = KZ_NSTART_EXEC_MAPPING;

  // ---- the sites and their slots ----
  for (size_t i = 0; i < sites.size(); ++i) {
    if (sites[i].site != i) return Status::error(Err::Invalid, "the site table is not indexed by site id");
    NSite ns;
    ns.r.id = sites[i].site;
    ns.r.kind = sites[i].kind == KZ_SITE_KIND_ENTRY_PROBE ? KZ_NSITE_KIND_CALL
                : sites[i].kind == KZ_SITE_KIND_RET_PROBE ? KZ_NSITE_KIND_RETURN
                : sites[i].kind == KZ_SITE_KIND_POINT_PROBE ? KZ_NSITE_KIND_POINT
                                                             : KZ_NSITE_KIND_UNKNOWN;
    ns.r.phase = sites[i].kind == KZ_SITE_KIND_ENTRY_PROBE ? KZ_NPHASE_BEFORE
                 : sites[i].kind == KZ_SITE_KIND_RET_PROBE ? KZ_NPHASE_AFTER
                 : sites[i].kind == KZ_SITE_KIND_POINT_PROBE ? KZ_NPHASE_POINT
                                                              : KZ_NPHASE_UNKNOWN;
    ns.r.decoder = sites[i].decoder;
    ns.r.line = sites[i].line;
    ns.r.bind = sites[i].site;
    ns.r.flags = KZ_NSITE_FLAG_ATTACHED;  // F0: a site is attached for the whole record or the observer fails the run
    ns.r.raw = self_ref(KZ_SEC_SITES, static_cast<uint32_t>(i));
    run.string(sites[i].label, &ns.label);
    run.string(sites[i].symbol, &ns.symbol);
    run.string(sites[i].source, &ns.source);
    std::string path;
    run.string(sites[i].path, &path);
    ns.at = path + "+" + hex64(sites[i].file_offset);
    out->sites.push_back(std::move(ns));
  }
  std::map<std::pair<uint32_t, uint8_t>, std::string> role_of;  // as the replay read it: the last slot record wins
  std::set<std::pair<uint32_t, uint8_t>> string_slot;
  for (size_t i = 0; i < slots.size(); ++i) {
    const kz_site_slot& sl = slots[i];
    NSlot n;
    n.r.site = sl.site;
    n.r.slot = sl.slot;
    n.r.kind = sl.src == KZ_SLOT_SRC_STR ? KZ_NVAL_TEXT : KZ_NVAL_INT;
    n.r.width = sl.width;
    n.r.raw = self_ref(KZ_SEC_SITE_SLOTS, static_cast<uint32_t>(i));
    run.string(sl.role, &n.role);
    n.from = enum_text("KZ_SLOT_SRC", sl.src);
    role_of[{sl.site, sl.slot}] = n.role;
    if (sl.src == KZ_SLOT_SRC_STR) string_slot.insert({sl.site, sl.slot});
    out->slots.push_back(std::move(n));
  }
  auto return_slot = [&](uint32_t site) {
    for (uint8_t k = 0; k < 4; ++k) {
      const auto r = role_of.find({site, k});
      if (r != role_of.end() && r->second == kRoleReturnAddress) return static_cast<int>(k);
    }
    return -1;
  };

  // ---- the actors: one per thread incarnation (THREADS order), then the tids the record names without one ----
  for (size_t i = 0; i < threads.size(); ++i) {
    const kz_thread& t = threads[i];
    NActor a;
    a.r = nactor_init();
    a.r.binding = KZ_NBIND_LINUX_TID;
    a.r.native_kind = KZ_NKEY_LINUX_TID;
    a.r.native_key = t.tid;
    a.r.backend_id = t.tid;
    a.r.generation = t.generation;
    a.r.process = t.pid;
    a.r.flags = KZ_NACTOR_FLAG_JOIN_RECORD;
    // the thread-group leader (gettid(2) = getpid(2)), as the first incarnation of that id
    if (t.tid == t.pid && t.generation == 0) a.r.flags |= KZ_NACTOR_FLAG_INITIAL;
    if ((t.flags & KZ_THREAD_FLAG_EXIT_SEEN) != 0) a.r.flags |= KZ_NACTOR_FLAG_EXITED;
    a.r.join_flags = t.flags;
    a.r.announced_id = t.announced_id;
    a.r.creator_event = t.creator_event < events.size() ? t.creator_event : KZ_NONE32;
    a.r.first_event = t.first_event < events.size() ? t.first_event : KZ_NONE32;
    a.r.raw = self_ref(KZ_SEC_THREADS, static_cast<uint32_t>(i));
    run.string(t.name, &a.name);
    out->actors.push_back(std::move(a));
  }
  // the incarnation of `tid` alive at time t: the latest generation forked no later than t
  auto incarnation = [&](uint32_t tid, uint64_t t) -> int64_t {
    int64_t best = -1;
    for (size_t i = 0; i < threads.size(); ++i) {
      if (threads[i].tid != tid) continue;
      if (best < 0 || (threads[i].t_fork <= t && threads[i].generation >= threads[static_cast<size_t>(best)].generation)) best = static_cast<int64_t>(i);
    }
    return best;
  };
  std::map<uint32_t, uint32_t> without_record;  // tid -> actor of a tid that has no THREADS record
  auto actor_of = [&](uint32_t tid, uint64_t t) -> uint32_t {
    const int64_t i = incarnation(tid, t);
    if (i >= 0) return static_cast<uint32_t>(i);
    const auto it = without_record.find(tid);
    if (it != without_record.end()) return it->second;
    NActor a;
    a.r = nactor_init();
    a.r.binding = KZ_NBIND_LINUX_TID;
    a.r.native_kind = KZ_NKEY_LINUX_TID;
    a.r.native_key = tid;
    a.r.backend_id = tid;
    const uint32_t idx = static_cast<uint32_t>(out->actors.size());
    out->actors.push_back(std::move(a));
    without_record[tid] = idx;
    return idx;
  };
  for (size_t i = 0; i < threads.size(); ++i) {
    if ((threads[i].flags & KZ_THREAD_FLAG_FORK_SEEN) == 0 || threads[i].ptid == KZ_NONE32) continue;
    const uint32_t c = actor_of(threads[i].ptid, threads[i].t_fork);
    out->actors[i].r.creator = c;
  }
  // owner records (ACTOR_OWNERS, schema revision 2): one per thread incarnation
  std::vector<bool> owner_seen(threads.size(), false);
  for (size_t oi = 0; oi < owners.size(); ++oi) {
    const kz_app_owner& o = owners[oi];
    if (o.thread >= threads.size()) continue;
    if (owner_seen[o.thread]) return Status::error(Err::Invalid, "ACTOR_OWNERS: two records for thread " + std::to_string(o.thread));
    owner_seen[o.thread] = true;
    NActor& a = out->actors[o.thread];
    a.r.flags |= KZ_NACTOR_FLAG_OWNER_RECORDED;
    a.r.owner_level = o.level;
    a.r.owner_evidence = o.evidence;
    a.r.owner_id = o.owner_key;
    a.r.owner_creator = o.creator_thread < threads.size() ? o.creator_thread : KZ_NONE32;
    a.r.owner_request = o.creation_event < events.size() ? o.creation_event : KZ_NONE32;
    a.r.raw_owner = self_ref(KZ_SEC_ACTOR_OWNERS, static_cast<uint32_t>(oi));
    run.string(o.app_name, &a.owner_app);
    // the calling code of the creation request: the return address at its OWNER_REQUEST site, as a link-time address
    if (o.level == KZ_OWNER_LEVEL_UNDETERMINED || o.owner_key == KZ_NONE32 || o.creation_event >= events.size()) continue;
    const kz_raw_ev& rq = events[o.creation_event];
    if (rq.site >= sites.size() || sites[rq.site].decoder != KZ_DECODER_OWNER_REQUEST || (rq.flags & KZ_RAW_FLAG_TRUNC) != 0) continue;
    const int slot = return_slot(rq.site);
    uint64_t link = 0;
    if (slot < 0 || !link_address(maps, threads[o.thread].pid, rq.a[slot], &link)) continue;
    a.r.request_addr = link;
    a.r.flags |= KZ_NACTOR_FLAG_REQUEST_ADDR;
  }

  // ---- the creating call's return, bound by the id (the identity join's id match) ----
  std::map<uint32_t, size_t> created_by;  // RAW_EVENTS index of a spawn-create event -> the thread it created
  for (size_t i = 0; i < threads.size(); ++i) {
    const kz_thread& t = threads[i];
    if ((t.flags & KZ_THREAD_FLAG_ID_MATCHED) != 0 && (t.flags & KZ_THREAD_FLAG_SPAWN_MATCHED) != 0 && t.creator_event != KZ_NONE32) created_by[t.creator_event] = i;
  }
  {
    std::map<uint32_t, std::vector<std::pair<uint64_t, uint64_t>>> open;  // tid -> (create index, return address)
    for (size_t i = 0; i < events.size(); ++i) {
      const kz_raw_ev& e = events[i];
      if (e.site >= sites.size() || (e.flags & KZ_RAW_FLAG_TRUNC) != 0) continue;
      if (sites[e.site].decoder == KZ_DECODER_SPAWN_CREATE) {
        open[e.tid].push_back({i, e.a[kSlotReturnAddress]});
      } else if (sites[e.site].decoder == KZ_DECODER_SPAWN_RETURN) {
        std::vector<std::pair<uint64_t, uint64_t>>& st = open[e.tid];
        size_t k = st.size();
        while (k > 0 && st[k - 1].second != e.a[kSlotReturnAddress]) --k;
        if (k == 0) continue;
        const auto cb = created_by.find(static_cast<uint32_t>(st[k - 1].first));
        if (cb != created_by.end()) out->actors[cb->second].r.spawn_return = static_cast<uint32_t>(i);
        st.resize(k - 1);
      }
    }
  }

  // ---- the events ----
  // call instances: a return is linked to the open call of the same actor on the same function whose return
  // address it carries (the entry/return pairing of check_plan: both sites declare a return_address slot)
  std::map<std::pair<uint32_t, std::string>, std::vector<std::pair<uint32_t, uint64_t>>> open_calls;
  for (size_t ei = 0; ei < events.size(); ++ei) {
    const kz_raw_ev& e = events[ei];
    kz_nevent ne = nevent_init();
    ne.t = e.t_ns;
    ne.actor = actor_of(e.tid, e.t_ns);
    ne.aseq = e.tseq;  // the decoder's per-thread order (not ASEQ_TARGET: a gap proves nothing)
    ne.site = e.site < sites.size() ? e.site : KZ_NONE32;
    ne.bind = e.site;
    ne.flags = KZ_NEV_FLAG_CLOCK;
    ne.raw = self_ref(KZ_SEC_RAW_EVENTS, static_cast<uint32_t>(ei));
    ne.first_value = static_cast<uint32_t>(out->values.size());
    const bool unreadable = (e.flags & KZ_RAW_FLAG_TRUNC) != 0;
    if (unreadable) ne.flags |= KZ_NEV_FLAG_NO_VALUES;
    for (uint8_t k = 0; k < 4 && !unreadable && ne.site != KZ_NONE32; ++k) {
      if (role_of.count({e.site, k}) == 0) continue;
      NValue v;
      v.r.event = static_cast<uint32_t>(ei);
      v.r.slot = k;
      v.r.v = e.a[k];
      if (string_slot.count({e.site, k}) != 0) {
        const auto tx = text_of.find({static_cast<uint32_t>(ei), k});
        if (tx == text_of.end()) continue;  // the kernel gave no string: the value is not observed
        v.r.kind = KZ_NVAL_TEXT;
        v.text = tx->second;
      } else {
        v.r.kind = KZ_NVAL_INT;
      }
      out->values.push_back(std::move(v));
    }
    ne.nvals = static_cast<uint16_t>(out->values.size() - ne.first_value);
    if (ne.site != KZ_NONE32 && !unreadable) {
      const int rs = return_slot(e.site);
      const NSite& site = out->sites[e.site];
      if (rs >= 0 && (site.r.kind == KZ_NSITE_KIND_CALL || site.r.kind == KZ_NSITE_KIND_RETURN)) {
        std::vector<std::pair<uint32_t, uint64_t>>& st = open_calls[{ne.actor, site.symbol}];
        if (site.r.kind == KZ_NSITE_KIND_CALL) {
          st.push_back({static_cast<uint32_t>(ei), e.a[rs]});
        } else {
          size_t k = st.size();
          while (k > 0 && st[k - 1].second != e.a[rs]) --k;
          if (k > 0) {
            ne.link = st[k - 1].first;
            st.resize(k - 1);
          }
        }
      }
    }
    out->events.push_back(ne);
  }

  // ---- losses: the event interval each report may affect ----
  for (size_t i = 0; i < losses.size(); ++i) {
    const kz_loss& l = losses[i];
    kz_nloss nl = nloss_init();
    nl.kind = nloss_of_source(l.source);
    nl.count = l.count;
    nl.unit_kind = KZ_NLOSS_UNIT_CPU_RING;
    nl.unit = l.cpu;
    nl.raw = self_ref(KZ_SEC_LOSS, static_cast<uint32_t>(i));
    if (l.source == KZ_LOSS_SOURCE_PERF_THROTTLE) {
      nl.begin = first_at_or_after(events, l.t_ns);
      nl.end = l.tseq_first == 1 ? KZ_NONE32 : first_after(events, l.t_ns + l.count);
    } else {
      // the kernel does not say when the lost records were written: from the start up to the report
      nl.begin = 0;
      nl.end = first_after(events, l.t_ns);
    }
    out->losses.push_back(nl);
  }
  if (flagged && losses.empty()) {
    kz_nloss nl = nloss_init();
    nl.kind = KZ_NLOSS_FLAGGED;
    out->losses.push_back(nl);
  }
  if (meta.empty()) {  // no RUNMETA: no loss evidence at all, the whole record is lossy
    kz_nloss nl = nloss_init();
    nl.kind = KZ_NLOSS_NO_EVIDENCE;
    out->losses.push_back(nl);
  }
  return check_neutral(*out);
}

// ---------------------------------------------------------------------------------------------------------
// Checks, names
// ---------------------------------------------------------------------------------------------------------
Status check_neutral(const NeutralRun& n) {
  const size_t ne = n.events.size(), na = n.actors.size(), ns = n.sites.size();
  auto bad = [](const std::string& what) { return Status::error(Err::Invalid, "neutral layer: " + what); };
  auto idx_ok = [](uint32_t v, size_t size) { return v == KZ_NONE32 || v < size; };
  for (size_t i = 0; i < n.slots.size(); ++i)
    if (n.slots[i].r.site >= ns) return bad("NSLOTS[" + std::to_string(i) + "].site out of range");
  for (size_t i = 0; i < ne; ++i) {
    const kz_nevent& e = n.events[i];
    if (e.actor >= na) return bad("NEVENTS[" + std::to_string(i) + "].actor out of range");
    if (!idx_ok(e.site, ns)) return bad("NEVENTS[" + std::to_string(i) + "].site out of range");
    if (!idx_ok(e.link, ne) || (e.link != KZ_NONE32 && e.link >= i)) return bad("NEVENTS[" + std::to_string(i) + "].link is not an earlier event");
    if (static_cast<uint64_t>(e.first_value) + e.nvals > n.values.size()) return bad("NEVENTS[" + std::to_string(i) + "] values out of range");
    for (uint32_t k = 0; k < e.nvals; ++k)
      if (n.values[e.first_value + k].r.event != i) return bad("NVALUES[" + std::to_string(e.first_value + k) + "] does not belong to its event");
  }
  for (size_t i = 0; i < na; ++i) {
    const kz_nactor& a = n.actors[i].r;
    if (!idx_ok(a.creator, na) || !idx_ok(a.owner_creator, na) || !idx_ok(a.boot_link, na)) return bad("NACTORS[" + std::to_string(i) + "] actor index out of range");
    if (!idx_ok(a.creator_event, ne) || !idx_ok(a.first_event, ne) || !idx_ok(a.spawn_return, ne) || !idx_ok(a.owner_request, ne))
      return bad("NACTORS[" + std::to_string(i) + "] event index out of range");
    if (a.binding > KZ_NBIND_KZRT_PROVISIONAL || a.owner_level > KZ_OWNER_LEVEL_CONFIRMED) return bad("NACTORS[" + std::to_string(i) + "] enum out of range");
  }
  for (size_t i = 0; i < n.losses.size(); ++i) {
    const kz_nloss& l = n.losses[i];
    if (l.begin > ne || (l.end != KZ_NONE32 && (l.end > ne || l.end < l.begin)) || !idx_ok(l.actor, na)) return bad("NLOSS[" + std::to_string(i) + "] interval out of range");
  }
  for (size_t i = 0; i < n.ctl.size(); ++i) {
    const kz_nctl& c = n.ctl[i];
    if (c.actor >= na || !idx_ok(c.subject, na) || !idx_ok(c.other, na) || !idx_ok(c.event, ne) || !idx_ok(c.site, ns)) return bad("NCTL[" + std::to_string(i) + "] index out of range");
  }
  return Status::success();
}

std::vector<std::string> actor_names(const NeutralRun& n) {
  auto prefix = [](uint8_t b) -> std::string {
    switch (b) {
      case KZ_NBIND_LINUX_TID: return "tid";
      case KZ_NBIND_KZRT_ACTOR: return "a";
      case KZ_NBIND_KZRT_PROVISIONAL: return "P";
      default: return "?";
    }
  };
  auto key = [](const kz_nactor& a) -> std::string {
    if (a.binding == KZ_NBIND_UNRESOLVED) return std::string();
    if (a.binding == KZ_NBIND_LINUX_TID) return std::to_string(a.native_key);
    return std::to_string(a.backend_id & ~0x80000000u);  // the runtime's id, without its provisional bit
  };
  auto base = [&](const NActor& a) { return a.name.empty() ? prefix(a.r.binding) + key(a.r) : a.name; };
  std::map<std::string, int> count;
  for (const NActor& a : n.actors)
    if ((a.r.flags & KZ_NACTOR_FLAG_JOIN_RECORD) != 0) ++count[base(a)];
  std::vector<std::string> out(n.actors.size());
  for (size_t i = 0; i < n.actors.size(); ++i) {
    const NActor& a = n.actors[i];
    std::string s = base(a);
    if ((a.r.flags & KZ_NACTOR_FLAG_JOIN_RECORD) != 0) {
      const std::string p = prefix(a.r.binding);
      if (count[s] > 1 && s.rfind(p, 0) != 0) s += "." + p + key(a.r);
      if (a.r.generation != 0) s += ".g" + std::to_string(a.r.generation);
    }
    out[i] = s;
  }
  return out;
}

std::string loss_text(const kz_nloss& l) {
  std::string t = enum_text("KZ_NLOSS", l.kind) + " count " + std::to_string(l.count);
  if (l.unit_kind != KZ_NLOSS_UNIT_NONE) t += " at " + enum_text("KZ_NLOSS_UNIT", l.unit_kind) + (l.unit == KZ_NONE32 ? std::string() : " " + std::to_string(l.unit));
  t += ", events " + std::to_string(l.begin) + ".." + (l.end == KZ_NONE32 ? std::string("end") : std::to_string(l.end));
  if (l.actor != KZ_NONE32) t += ", actor " + std::to_string(l.actor);
  return t;
}

// ---------------------------------------------------------------------------------------------------------
// Container I/O
// ---------------------------------------------------------------------------------------------------------
bool has_neutral_sections(const Container& run) { return run.find_section(KZ_SEC_NRUN) != nullptr; }

Status read_neutral(const Container& run, NeutralRun* out) {
  *out = NeutralRun();
  if (run.header().artifact_kind != KZ_ARTIFACT_RUN) return Status::error(Err::Invalid, "not a run container");
  std::vector<kz_nrun> nrun;
  std::vector<kz_nsite> sites;
  std::vector<kz_nslot> slots;
  std::vector<kz_nvalue> values;
  std::vector<kz_nactor> actors;
  Status s = run.read_section(KZ_SEC_NRUN, &nrun);
  if (s.ok()) s = run.read_section(KZ_SEC_NSITES, &sites);
  if (s.ok()) s = run.read_section(KZ_SEC_NSLOTS, &slots);
  if (s.ok()) s = run.read_section(KZ_SEC_NEVENTS, &out->events);
  if (s.ok()) s = run.read_section(KZ_SEC_NVALUES, &values);
  if (s.ok()) s = run.read_section(KZ_SEC_NACTORS, &actors);
  if (s.ok()) s = run.read_section(KZ_SEC_NLOSS, &out->losses);
  if (s.ok()) s = run.read_section(KZ_SEC_NCTL, &out->ctl);
  if (!s.ok()) return s;
  if (nrun.size() != 1) return Status::error(Err::Invalid, "neutral layer: NRUN must hold one record");
  bool strings_ok = true;
  auto str = [&](const kz_str& k, std::string* t) { strings_ok = run.string(k, t) && strings_ok; };
  out->run.r = nrun.front();
  str(nrun.front().backend_name, &out->run.backend_name);
  str(nrun.front().runtime, &out->run.runtime);
  str(nrun.front().adapter, &out->run.adapter);
  str(nrun.front().compiler, &out->run.compiler);
  str(nrun.front().target, &out->run.target);
  for (const kz_nsite& r : sites) {
    NSite n;
    n.r = r;
    str(r.label, &n.label);
    str(r.op, &n.op);
    str(r.symbol, &n.symbol);
    str(r.source, &n.source);
    str(r.at, &n.at);
    out->sites.push_back(std::move(n));
  }
  for (const kz_nslot& r : slots) {
    NSlot n;
    n.r = r;
    str(r.role, &n.role);
    str(r.from, &n.from);
    out->slots.push_back(std::move(n));
  }
  for (const kz_nvalue& r : values) {
    NValue n;
    n.r = r;
    str(r.text, &n.text);
    out->values.push_back(std::move(n));
  }
  for (const kz_nactor& r : actors) {
    NActor n;
    n.r = r;
    str(r.name, &n.name);
    str(r.owner_app, &n.owner_app);
    str(r.request_func, &n.request_func);
    out->actors.push_back(std::move(n));
  }
  if (!strings_ok) return Status::error(Err::BadRef, "neutral layer: a string does not resolve");
  return check_neutral(*out);
}

Status neutral_of_run(const Container& run, NeutralRun* out, bool* from_sections) {
  const bool has = has_neutral_sections(run);
  if (from_sections != nullptr) *from_sections = has;
  return has ? read_neutral(run, out) : neutral_from_f0(run, out);
}

Status add_neutral_sections(const NeutralRun& n, ContainerWriter* w) {
  Status s = check_neutral(n);
  if (!s.ok()) return s;
  kz_nrun run = n.run.r;
  run.backend_name = w->intern(n.run.backend_name);
  run.runtime = w->intern(n.run.runtime);
  run.adapter = w->intern(n.run.adapter);
  run.compiler = w->intern(n.run.compiler);
  run.target = w->intern(n.run.target);
  std::vector<kz_nrun> runs{run};
  std::vector<kz_nsite> sites;
  for (const NSite& x : n.sites) {
    kz_nsite r = x.r;
    r.label = w->intern(x.label);
    r.op = w->intern(x.op);
    r.symbol = w->intern(x.symbol);
    r.source = w->intern(x.source);
    r.at = w->intern(x.at);
    sites.push_back(r);
  }
  std::vector<kz_nslot> slots;
  for (const NSlot& x : n.slots) {
    kz_nslot r = x.r;
    r.role = w->intern(x.role);
    r.from = w->intern(x.from);
    slots.push_back(r);
  }
  std::vector<kz_nvalue> values;
  for (const NValue& x : n.values) {
    kz_nvalue r = x.r;
    r.text = w->intern(x.text);
    values.push_back(r);
  }
  std::vector<kz_nactor> actors;
  for (const NActor& x : n.actors) {
    kz_nactor r = x.r;
    r.name = w->intern(x.name);
    r.owner_app = w->intern(x.owner_app);
    r.request_func = w->intern(x.request_func);
    actors.push_back(r);
  }
  s = w->add_section(KZ_SEC_NRUN, runs);
  if (s.ok()) s = w->add_section(KZ_SEC_NSITES, sites);
  if (s.ok()) s = w->add_section(KZ_SEC_NSLOTS, slots);
  if (s.ok()) s = w->add_section(KZ_SEC_NEVENTS, n.events);
  if (s.ok()) s = w->add_section(KZ_SEC_NVALUES, values);
  if (s.ok()) s = w->add_section(KZ_SEC_NACTORS, actors);
  if (s.ok()) s = w->add_section(KZ_SEC_NLOSS, n.losses);
  if (s.ok()) s = w->add_section(KZ_SEC_NCTL, n.ctl);
  return s;
}

Status copy_sections(const Container& src, ContainerWriter* w) {
  for (const kz_section_entry& e : src.section_table()) {
    if (e.type == KZ_SEC_STRINGS) continue;
    const SectionInfo* info = find_section(e.type);
    if (info == nullptr || info->record == nullptr) return Status::error(Err::BadSection, "section " + std::to_string(e.type) + " is not registered");
    const kz_record_desc& rec = *info->record;
    size_t size = 0;
    const uint8_t* data = src.section_data(e.type, &size);
    std::vector<uint8_t> host(size);
    if (size != 0) {
      if (section_is_serialized(e.type)) decode_records(rec, data, static_cast<size_t>(e.count), host.data());
      else std::memcpy(host.data(), data, size);
    }
    for (size_t i = 0; i < e.count; ++i)
      for (uint32_t f = 0; f < rec.nfields; ++f) {
        const kz_field_desc& fd = rec.fields[f];
        if (fd.kind != KZ_FK_STR) continue;
        for (uint32_t k = 0; k < fd.count; ++k) {
          uint8_t* at = host.data() + i * rec.size + fd.offset + static_cast<size_t>(k) * fd.elem_size;
          kz_str old;
          std::memcpy(&old, at, sizeof old);
          std::string text;
          if (!src.string(old, &text)) return Status::error(Err::BadRef, std::string("section ") + info->name + ": a string does not resolve");
          const kz_str fresh = w->intern(text);
          std::memcpy(at, &fresh, sizeof fresh);
        }
      }
    Status s = w->add_section_raw(e.type, host.data(), static_cast<size_t>(e.count), rec.size);
    if (!s.ok()) return s;
  }
  return Status::success();
}

Status convert_f0_container(const Container& run, const Digest& tool_sha256, Bytes* out, NeutralRun* layer) {
  if (run.header().artifact_kind != KZ_ARTIFACT_RUN) return Status::error(Err::Invalid, "not a run container");
  if (has_neutral_sections(run)) return Status::error(Err::Invalid, "the container already has the neutral layer");
  NeutralRun n;
  Status s = neutral_from_f0(run, &n);
  if (!s.ok()) return s;
  ContainerWriter w(KZ_ARTIFACT_RUN);
  w.set_basis_hash(run.basis_hash());
  w.set_profile_hash(run.profile_hash());
  w.set_tool_hash(tool_sha256);
  for (const kz_parent& p : run.parents()) {
    Digest d;
    std::memcpy(d.data(), p.sha256, d.size());
    s = w.add_parent(p.kind, p.role, d);
    if (!s.ok()) return s;
  }
  // the record it was converted from (role 1 = "source record of a conversion")
  s = w.add_parent(KZ_ARTIFACT_RUN, 1, run.artifact_hash());
  if (s.ok()) s = copy_sections(run, &w);
  if (s.ok()) s = add_neutral_sections(n, &w);
  if (s.ok()) s = w.finish(out);
  if (s.ok() && layer != nullptr) *layer = std::move(n);
  return s;
}

}  // namespace kz::trace

// kzrt_convert.cpp -- a kzrt-dump stream with its site manifests -> the neutral event layer (kzrt_convert.hpp).

#include "kz/trace/kzrt_convert.hpp"

#include <algorithm>
#include <cstring>
#include <map>
#include <queue>
#include <set>
#include <tuple>

#include "kz/trace/identity_rule.hpp"
#include "kz_rt_decode.h"
#include "kz_rt_manifest.hpp"

namespace kz::trace {

// The neutral key kinds, context classes and control records have the runtime's values (kz_neutral.h).
constexpr bool same(unsigned a, unsigned b) { return a == b; }
static_assert(same(KZ_NKEY_NONE, KZ_RT_KEY_NONE) && same(KZ_NKEY_LINUX_TID, KZ_RT_KEY_LINUX_TID) && same(KZ_NKEY_PTHREAD, KZ_RT_KEY_PTHREAD) &&
                  same(KZ_NKEY_OTHER, KZ_RT_KEY_OTHER),
              "the neutral key kinds are the runtime's");
static_assert(same(KZ_NCTX_TASK, KZ_RT_CTX_TASK) && same(KZ_NCTX_LOCK_HELD, KZ_RT_CTX_LOCK_HELD) && same(KZ_NCTX_ISR, KZ_RT_CTX_ISR) &&
                  same(KZ_NCTX_TERMINATING, KZ_RT_CTX_TERMINATING) && same(KZ_NCTX_UNSUPPORTED, KZ_RT_CTX_UNSUPPORTED),
              "the neutral context classes are the runtime's");
static_assert(same(KZ_NCTL_WHAT_TIMEOUT, KZ_RT_CW_TIMEOUT) && same(KZ_NCTL_WHAT_FIRE, KZ_RT_CW_FIRE) && same(KZ_NCTL_REASON_NO_SLOT, KZ_RT_CR_NO_SLOT),
              "the neutral control records are the runtime's");

namespace {

template <class T>
T zeroed() {
  T v;
  std::memset(&v, 0, sizeof v);
  return v;
}

struct Ev {
  kz_rt_dec_event e;
  uint64_t off = 0;
  uint64_t order = 0;     // decode order
  uint32_t actor = 0;     // neutral actor
  uint32_t index = KZ_NONE32;  // neutral event index
};

struct Collected {
  bool have_run = false, have_runx = false;
  kz_rt_dec_run run{};
  kz_rt_dec_runx runx{};
  std::vector<kz_rt_dec_module> modules;
  std::vector<kz_rt_dec_actor> actors;
  std::vector<kz_rt_dec_loss> losses;
  std::vector<kz_rt_dec_point> points;
  std::vector<Ev> events;
  std::vector<std::pair<kz_rt_dec_ctl, uint64_t>> ctls;
  std::vector<std::pair<uint32_t, uint32_t>> other_aseq;  // (runtime actor, aseq) of the actors' CTL and AEXIT records
  bool end_seen = false, crc_ok = false;
};

int collect(void* ctx, const kz_rt_dec_item* it) {
  Collected& c = *static_cast<Collected*>(ctx);
  switch (it->type) {
    case KZ_RT_REC_RUN:
      c.have_run = true;
      c.run = it->u.run;
      break;
    case KZ_RT_REC_RUNX:
      c.have_runx = true;
      c.runx = it->u.runx;
      break;
    case KZ_RT_REC_MODULE: c.modules.push_back(it->u.module); break;
    case KZ_RT_REC_ACTOR: c.actors.push_back(it->u.actor); break;
    case KZ_RT_REC_LOSS: c.losses.push_back(it->u.loss); break;
    case KZ_RT_REC_POINT: c.points.push_back(it->u.point); break;
    case KZ_RT_REC_EVENT: {
      Ev e;
      e.e = it->u.event;
      e.off = it->offset;
      e.order = c.events.size();
      c.events.push_back(e);
      break;
    }
    case KZ_RT_REC_CTL:
      c.ctls.push_back({it->u.ctl, it->offset});
      c.other_aseq.push_back({it->u.ctl.actor, it->u.ctl.aseq});
      break;
    case KZ_RT_REC_AEXIT: c.other_aseq.push_back({it->u.aexit.actor, it->u.aexit.aseq}); break;
    case KZ_RT_REC_END:
      c.end_seen = true;
      c.crc_ok = it->u.end.crc_ok != 0;
      break;
    default: break;
  }
  return 0;
}

std::string hex(const unsigned char* p, size_t n) {
  static const char* const d = "0123456789abcdef";
  std::string s;
  for (size_t i = 0; i < n; ++i) {
    s += d[p[i] >> 4];
    s += d[p[i] & 15];
  }
  return s;
}

bool all_zero(const unsigned char* p, size_t n) {
  for (size_t i = 0; i < n; ++i)
    if (p[i] != 0) return false;
  return true;
}

uint8_t type_width(const std::string& t) {
  if (t == "u8" || t == "s8") return 1;
  if (t == "u16" || t == "s16") return 2;
  if (t == "u32" || t == "s32") return 4;
  return 8;  // u64, s64, ptr
}

uint8_t nloss_kind(uint32_t k) {
  switch (k) {
    case KZ_RT_LOSS_OVERFLOW: return KZ_NLOSS_OVERFLOW;
    case KZ_RT_LOSS_PARTIAL: return KZ_NLOSS_PARTIAL;
    case KZ_RT_LOSS_PREINIT: return KZ_NLOSS_PREINIT;
    case KZ_RT_LOSS_EXCLUDED: return KZ_NLOSS_EXCLUDED;
    case KZ_RT_LOSS_UNLOADED: return KZ_NLOSS_UNLOADED;
    case KZ_RT_LOSS_REENTRY: return KZ_NLOSS_REENTRY;
    case KZ_RT_LOSS_ACTOR_TABLE: return KZ_NLOSS_ACTOR_TABLE;
    case KZ_RT_LOSS_UNRESOLVED: return KZ_NLOSS_UNRESOLVED;
    case KZ_RT_LOSS_CTL_SKIPPED: return KZ_NLOSS_CTL_SKIPPED;
    case KZ_RT_LOSS_CTL_NOCOND: return KZ_NLOSS_CTL_NOCOND;
    case KZ_RT_LOSS_MODULE_TABLE: return KZ_NLOSS_MODULE_TABLE;
    case KZ_RT_LOSS_AFTER_STOP: return KZ_NLOSS_AFTER_STOP;
    case KZ_RT_LOSS_NOTE_TABLE: return KZ_NLOSS_NOTE_TABLE;
    case KZ_RT_LOSS_ABNORMAL_END: return KZ_NLOSS_ABNORMAL_END;
    default: return KZ_NLOSS_UNKNOWN;
  }
}

// KZ_DECODER of a manifest identity read (kzs-sites 2; the same reader names as F0's plan).
uint16_t decoder_code(const std::string& d) {
  if (d == "spawn-create") return KZ_DECODER_SPAWN_CREATE;
  if (d == "spawn-return") return KZ_DECODER_SPAWN_RETURN;
  if (d == "thread-entry") return KZ_DECODER_THREAD_ENTRY;
  if (d == "owner-request") return KZ_DECODER_OWNER_REQUEST;
  if (d == "child-request") return KZ_DECODER_CHILD_REQUEST;
  if (d == "request-return") return KZ_DECODER_REQUEST_RETURN;
  if (d == "spawn-id") return KZ_DECODER_SPAWN_ID;
  return KZ_DECODER_NONE;
}

// Losses that leave the recorded events whole: no interval (kzrt_convert.hpp, "losses").  An abnormal end loses what
// came after every actor's gap-free prefix (event_semantics.md 12.3), not a record inside it.
bool no_interval(uint8_t kind) {
  return kind == KZ_NLOSS_CTL_SKIPPED || kind == KZ_NLOSS_CTL_NOCOND || kind == KZ_NLOSS_ACTOR_TABLE || kind == KZ_NLOSS_UNRESOLVED ||
         kind == KZ_NLOSS_MODULE_TABLE || kind == KZ_NLOSS_AFTER_STOP || kind == KZ_NLOSS_ABNORMAL_END;
}

// RUNX store_flags as text (kz_rt.h KZ_RT_STOREF_*, the refusal reason in bits 8..15).
std::string store_text(uint32_t f) {
  if ((f & KZ_RT_STOREF_REFUSED) != 0) {
    static const char* const why[] = {"?", "align", "open", "size", "write", "map"};
    const uint32_t r = KZ_RT_STOREF_REASON(f);
    return std::string("store refused (") + (r < 6 ? why[r] : "?") + ")";
  }
  return (f & KZ_RT_STOREF_FILE) != 0 ? "store file" : "store memory";
}

}  // namespace

Status neutral_from_kzrt(const KzrtConvertInput& in, KzrtConvertResult* out) {
  out->layer = NeutralRun();
  out->notes.clear();
  NeutralRun& n = out->layer;
  std::vector<std::string>& notes = out->notes;

  // ---- manifests ----
  kzrt::Manifest m;
  for (const auto& f : in.manifests) {
    std::string err;
    if (!kzrt::parse_manifest(f.second, f.first, &m, &err)) return Status::error(Err::Invalid, "manifest " + err);
  }
  std::set<std::string> manifest_shas;
  for (const auto& f : in.manifests) manifest_shas.insert(to_hex(sha256(f.second)));

  // ---- the stream ----
  Collected c;
  kz_rt_dec_stats st;
  const int rc = kz_rt_decode(in.dump.data(), in.dump.size(), collect, &c, &st);
  if (rc != KZ_RT_DEC_OK && rc != KZ_RT_DEC_ETRUNC) return Status::error(Err::Invalid, std::string("kzrt-dump: ") + kz_rt_dec_error(rc));
  if (!c.have_run) return Status::error(Err::Invalid, "kzrt-dump: no RUN record");
  const bool complete = rc == KZ_RT_DEC_OK && c.end_seen && c.crc_ok && st.partial == 0;
  if (!complete) notes.push_back(std::string("the stream is not complete (") + (rc != KZ_RT_DEC_OK ? kz_rt_dec_error(rc) : !c.crc_ok ? "END CRC mismatch" : "partial records") + "): the whole record is lossy");
  if (c.events.size() >= KZ_NONE32 / 2) return Status::error(Err::Invalid, "kzrt-dump: too many events for the neutral layer");

  // ---- the run ----
  const kz_rt_dec_run& rr = c.run;
  NRun& nr = n.run;
  nr.r.backend = KZ_NBACKEND_KZRT;
  // CLOCK_MONOTONIC, none, a clock the runtime does not name (other), or the adapter's own clock
  nr.r.clock = rr.clock_id == KZ_RT_CLOCK_POSIX_MONOTONIC ? KZ_NCLOCK_MONOTONIC
               : rr.clock_id == KZ_RT_CLOCK_NONE          ? KZ_NCLOCK_NONE
               : rr.clock_id == KZ_RT_CLOCK_OTHER         ? KZ_NCLOCK_UNKNOWN
                                                          : KZ_NCLOCK_ADAPTER;
  nr.r.run_id = rr.run_id;
  nr.r.instance_id = rr.instance_id;
  std::memcpy(nr.r.manifest_sha256, rr.manifest_sha256, 32);
  const Digest dump_sha = sha256(in.dump.data(), in.dump.size());
  std::memcpy(nr.r.raw_sha256, dump_sha.data(), 32);
  nr.r.flags = KZ_NRUN_FLAG_LOSS_EVIDENCE;  // the runtime counts its own losses (event_semantics.md 8.4)
  // how the run ended (dump 1.4, event_semantics.md 12.2): the dump of the running process, or a dump recovered from its
  // store after it ended (the file header's flag and RUNX end_kind say it; either is taken).  A recovered record is
  // never `complete`, even when the stream is whole: it holds what the store held when the process ended.
  const bool hdr_recovered = (st.dump_flags & KZ_RT_DUMP_RECOVERED) != 0;
  const bool runx_recovered = c.have_runx && c.runx.end_kind == KZ_RT_END_RECOVERED;
  const bool recovered = hdr_recovered || runx_recovered;
  const bool dumped = recovered && c.have_runx && (c.runx.end_flags & KZ_RT_ENDF_DUMPED) != 0;
  if (hdr_recovered != runx_recovered)
    notes.push_back(std::string("the dump header ") + (hdr_recovered ? "says" : "does not say") + " recovered, RUNX end_kind " + std::to_string(c.have_runx ? c.runx.end_kind : 0) +
                    ": taken as recovered");
  if (recovered) {
    nr.r.flags |= KZ_NRUN_FLAG_RECOVERED;
    if (dumped) nr.r.flags |= KZ_NRUN_FLAG_DUMPED;
    const uint32_t ef = c.have_runx ? c.runx.end_flags : 0;
    nr.r.end_kind = (ef & KZ_RT_ENDF_SIGNAL) != 0 ? KZ_NEND_SIGNAL : (ef & KZ_RT_ENDF_EXIT) != 0 ? KZ_NEND_EXIT : KZ_NEND_UNSTATED;
    nr.r.end_value = nr.r.end_kind != KZ_NEND_UNSTATED ? c.runx.end_value : 0;
    notes.push_back("end: recovered from the store after the process ended" +
                    (nr.r.end_kind == KZ_NEND_SIGNAL ? " (signal " + std::to_string(nr.r.end_value) + ")"
                     : nr.r.end_kind == KZ_NEND_EXIT ? " (exit status " + std::to_string(nr.r.end_value) + ")"
                                                     : std::string(" (the harness gave no signal or exit status)")) +
                    ", " + store_text(c.have_runx ? c.runx.store_flags : 0) +
                    (dumped ? ": the process had completed its own dump, which holds the same records" : ": the records are the prefix the store held (abnormal-end)"));
  } else if (c.have_runx && c.runx.end_kind == KZ_RT_END_IN_PROCESS) {
    nr.r.end_kind = KZ_NEND_IN_PROCESS;
    notes.push_back("end: the process's own dump, " + store_text(c.runx.store_flags));
  }
  if (complete && !recovered) nr.r.flags |= KZ_NRUN_FLAG_COMPLETE;
  if ((st.dump_flags & KZ_RT_DUMP_STOPPED) != 0) nr.r.flags |= KZ_NRUN_FLAG_STOPPED;
  if (!c.ctls.empty()) nr.r.flags |= KZ_NRUN_FLAG_CONTROL_RECORDED;
  nr.r.raw_meta = zeroed<kz_ref>();
  nr.backend_name = std::string("kzrt") + (rr.backend[0] != 0 ? std::string(" ") + rr.backend : std::string());
  nr.runtime = rr.runtime;
  nr.adapter = rr.adapter;
  nr.compiler = rr.compiler;
  nr.target = rr.target;
  if (!all_zero(rr.manifest_sha256, 32) && manifest_shas.count(hex(rr.manifest_sha256, 32)) == 0)
    notes.push_back("RUN names manifest " + hex(rr.manifest_sha256, 32) + ", which is not one of the given manifests");
  for (const kz_rt_dec_module& md : c.modules) {
    if (m.module(md.module) == nullptr) notes.push_back("module " + std::to_string(md.module) + " is registered by the run but not declared by the manifests");
    if (!all_zero(md.manifest_sha256, 32) && manifest_shas.count(hex(md.manifest_sha256, 32)) == 0)
      notes.push_back("module " + std::to_string(md.module) + " names manifest " + hex(md.manifest_sha256, 32) + ", which is not one of the given manifests");
  }

  // ---- sites and slots ----
  std::vector<const kzrt::Site*> msites;
  for (const kzrt::Site& s : m.sites) msites.push_back(&s);
  std::sort(msites.begin(), msites.end(), [](const kzrt::Site* a, const kzrt::Site* b) { return a->id < b->id; });
  std::map<uint32_t, uint32_t> site_index;  // manifest site id -> NSITES index
  for (const kzrt::Site* s : msites) {
    NSite ns;
    ns.r.id = s->id;
    const bool callee = !s->callee.empty();
    if (s->phase == "before") ns.r.kind = KZ_NSITE_KIND_CALL, ns.r.phase = KZ_NPHASE_BEFORE;
    else if (s->phase == "after") ns.r.kind = KZ_NSITE_KIND_RETURN, ns.r.phase = KZ_NPHASE_AFTER;
    else {
      ns.r.kind = KZ_NSITE_KIND_POINT;
      ns.r.phase = s->phase == "effect" ? KZ_NPHASE_EFFECT : s->phase == "exit" ? KZ_NPHASE_EXIT : KZ_NPHASE_POINT;
    }
    if (callee && ns.r.kind != KZ_NSITE_KIND_POINT) ns.r.flags |= KZ_NSITE_FLAG_CALL_SITE;
    ns.symbol = ns.r.kind == KZ_NSITE_KIND_POINT ? s->func : callee ? s->callee : s->func;
    ns.label = s->label;
    ns.op = s->op;
    ns.source = s->tu;
    ns.r.line = s->src_line;
    ns.r.decoder = s->has_identity ? decoder_code(s->identity.decoder) : static_cast<uint16_t>(KZ_DECODER_NONE);
    ns.r.bind = KZ_NONE32;
    ns.r.raw = zeroed<kz_ref>();
    uint32_t nb = 0, observed = 0;
    for (const kzrt::Bind& b : m.binds) {
      if (b.site != s->id) continue;
      ++nb;
      if (b.status == "bound" || b.status == "inlined" || b.status == "duplicated") ++observed;
      ns.r.bind = nb == 1 ? b.id : KZ_NONE32;
      ns.at = nb == 1 ? b.at : std::string();
    }
    if (nb > 0 && observed == nb) ns.r.flags |= KZ_NSITE_FLAG_ATTACHED;
    if (observed > 0 && observed < nb) ns.r.flags |= KZ_NSITE_FLAG_PARTIAL_BINDING;
    site_index[s->id] = static_cast<uint32_t>(n.sites.size());
    n.sites.push_back(std::move(ns));
    for (const kzrt::Value& v : s->values) {
      NSlot sl;
      sl.r.site = site_index[s->id];
      sl.r.slot = static_cast<uint8_t>(v.slot);
      sl.r.kind = v.type == "ptr" ? (v.string_mode == "validated" ? KZ_NVAL_TEXT : KZ_NVAL_POINTER) : KZ_NVAL_INT;
      sl.r.width = type_width(v.type);
      if (!v.type.empty() && v.type[0] == 's') sl.r.flags |= KZ_NSLOT_FLAG_SIGNED;
      sl.r.raw = zeroed<kz_ref>();
      sl.role = v.role;
      sl.from = v.from;
      n.slots.push_back(std::move(sl));
    }
  }
  auto site_of = [&](uint32_t id) -> uint32_t {
    const auto it = site_index.find(id);
    if (it != site_index.end()) return it->second;
    notes.push_back("site " + std::to_string(id) + " is recorded but not declared by the manifests: an unmapped placeholder");
    NSite ns;
    ns.r.id = id;
    ns.r.bind = KZ_NONE32;
    ns.label = "site" + std::to_string(id);
    const uint32_t idx = static_cast<uint32_t>(n.sites.size());
    n.sites.push_back(std::move(ns));
    site_index[id] = idx;
    return idx;
  };

  // ---- actors ----
  std::sort(c.actors.begin(), c.actors.end(), [](const kz_rt_dec_actor& a, const kz_rt_dec_actor& b) { return a.actor < b.actor; });
  std::map<uint32_t, uint32_t> actor_of;  // runtime actor id (run or provisional) -> NACTORS index
  std::map<uint32_t, const kz_rt_dec_actor*> record_of;
  for (const kz_rt_dec_actor& a : c.actors) {
    NActor na;
    na.r = nactor_init();
    na.r.binding = KZ_NBIND_KZRT_ACTOR;
    na.r.backend_id = a.actor;
    na.r.native_key = a.native_key;
    // the neutral key kinds have the runtime's values (kz_neutral.h KZ_NKEY)
    na.r.native_kind = static_cast<uint8_t>(a.key_kind <= static_cast<uint32_t>(KZ_NKEY_TASK_ID) ? a.key_kind : static_cast<uint32_t>(KZ_NKEY_OTHER));
    na.r.generation = a.key_generation;
    na.r.flags = KZ_NACTOR_FLAG_JOIN_RECORD;
    if ((a.flags & KZ_RT_AF_EXITED) != 0) na.r.flags |= KZ_NACTOR_FLAG_EXITED;
    if ((a.flags & KZ_RT_AF_HELD) != 0) na.r.flags |= KZ_NACTOR_FLAG_HELD_AT_END;
    actor_of[a.actor] = static_cast<uint32_t>(n.actors.size());
    record_of[a.actor] = &a;
    n.actors.push_back(std::move(na));
  }
  // provisional actors: joined to the run actor that took their cell, or kept as provisional
  std::set<uint32_t> provisional;
  for (const Ev& e : c.events)
    if ((e.e.actor & KZ_RT_ACTOR_PROVISIONAL) != 0) provisional.insert(e.e.actor);
  std::map<uint32_t, uint32_t> joined_to;  // provisional id -> NACTORS index of the run actor
  for (const kz_rt_dec_actor& a : c.actors)
    if (a.boot_link != 0) {
      provisional.insert(a.boot_link);
      if (a.link_evidence == KZ_RT_LINK_SAME_CELL) joined_to[a.boot_link] = actor_of[a.actor];
    }
  for (uint32_t p : provisional) {
    NActor na;
    na.r = nactor_init();
    na.r.binding = KZ_NBIND_KZRT_PROVISIONAL;
    na.r.backend_id = p;
    const uint32_t idx = static_cast<uint32_t>(n.actors.size());
    n.actors.push_back(std::move(na));
    const auto j = joined_to.find(p);
    if (j != joined_to.end()) n.actors[j->second].r.boot_link = idx;
    else actor_of[p] = idx;
  }
  for (const auto& j : joined_to) actor_of[j.first] = j.second;  // the provisional actor's events are the run actor's
  // events of a run actor id without an ACTOR record, and events without a cell (each one its own actor)
  for (Ev& e : c.events) {
    const uint32_t id = e.e.actor;
    if (id == KZ_RT_ACTOR_UNRESOLVED) {
      NActor na;
      na.r = nactor_init();
      na.r.binding = KZ_NBIND_UNRESOLVED;
      na.name = "?" + std::to_string(e.e.aseq);
      e.actor = static_cast<uint32_t>(n.actors.size());
      n.actors.push_back(std::move(na));
      continue;
    }
    auto it = actor_of.find(id);
    if (it == actor_of.end()) {
      notes.push_back("actor " + std::to_string(id) + " has events but no ACTOR record");
      NActor na;
      na.r = nactor_init();
      na.r.binding = KZ_NBIND_KZRT_ACTOR;
      na.r.backend_id = id;
      it = actor_of.emplace(id, static_cast<uint32_t>(n.actors.size())).first;
      n.actors.push_back(std::move(na));
    }
    e.actor = it->second;
  }
  for (const kz_rt_dec_point& q : c.points)
    if (q.held_actor != 0 && actor_of.count(q.held_actor) != 0) n.actors[actor_of[q.held_actor]].r.flags |= KZ_NACTOR_FLAG_HELD_AT_END;
  // the process's initial thread (RUNX 1.2, kz_rt_process): the run actor whose native key is the key the start-up code
  // recorded for its own thread, where exactly one run actor has it (the backend's initial-thread record; F0 takes the
  // thread whose id is the process id, neutral.cpp).  Linux: the start-up object compares the thread id with the
  // process id, and the thread-group leader's id is not reused while the process lives, so one actor at most has it.
  if (c.have_runx && c.runx.process != 0) {
    std::vector<uint32_t> with_key;
    for (const kz_rt_dec_actor& a : c.actors)
      if (a.key_kind == c.runx.initial_kind && a.native_key == c.runx.initial_key && a.key_kind != KZ_RT_KEY_NONE) with_key.push_back(actor_of[a.actor]);
    const bool tid_pid = (c.runx.process_evidence & KZ_RT_PROC_TID_IS_PID) != 0;
    const bool ctor = (c.runx.process_evidence & KZ_RT_PROC_CONSTRUCTOR) != 0;
    const std::string ev = std::string(ctor ? "recorded by a constructor of the executable" : "recorded by the start-up code") +
                           (tid_pid ? ", thread id = process id" : "");
    if (with_key.size() == 1 && (ctor || tid_pid)) {
      NActor& a = n.actors[with_key[0]];
      a.r.flags |= KZ_NACTOR_FLAG_INITIAL;
      a.r.process = static_cast<uint32_t>(c.runx.process);
      notes.push_back("initial thread: run actor " + std::to_string(a.r.backend_id) + " of process " + std::to_string(c.runx.process) + " (" + ev + ")");
    } else {
      notes.push_back("initial thread of process " + std::to_string(c.runx.process) + " (" + ev + "): " + std::to_string(with_key.size()) +
                      " run actors have its key; not stated" + (with_key.size() == 1 ? " (the start-up code names no evidence)" : ""));
    }
    if (in.harness_process != 0 && in.harness_process != c.runx.process)
      notes.push_back("the harness names process " + std::to_string(in.harness_process) + ", the dump records " + std::to_string(c.runx.process) + ": the dump's record is used");
  } else if (in.harness_process != 0) {
    // the process id of the harness record (a 1.1 dump): the run actor whose Linux TID is the process id
    std::vector<uint32_t> with_key;
    for (const kz_rt_dec_actor& a : c.actors)
      if (a.key_kind == KZ_RT_KEY_LINUX_TID && a.native_key == in.harness_process && a.key_generation == 0) with_key.push_back(actor_of[a.actor]);
    if (with_key.size() == 1) {
      NActor& a = n.actors[with_key[0]];
      a.r.flags |= KZ_NACTOR_FLAG_INITIAL;
      a.r.process = static_cast<uint32_t>(in.harness_process);
      notes.push_back("initial thread: run actor " + std::to_string(a.r.backend_id) + " of process " + std::to_string(in.harness_process) +
                      " (the process id the harness recorded, thread id = process id; the dump does not record its process)");
    } else {
      notes.push_back("initial thread of process " + std::to_string(in.harness_process) + " (the harness record): " + std::to_string(with_key.size()) +
                      " run actors have that thread id; not stated");
    }
  } else {
    notes.push_back("initial thread: not recorded (no RUNX process, kzrt-dump 1.1 or kz_rt_process not called)");
  }

  // ---- the order: per actor by aseq; unclocked (bootstrap) heads first in buffer order, then by time ----
  std::vector<std::vector<size_t>> per_actor(n.actors.size());
  for (size_t i = 0; i < c.events.size(); ++i) per_actor[c.events[i].actor].push_back(i);
  for (std::vector<size_t>& v : per_actor)
    std::stable_sort(v.begin(), v.end(), [&](size_t a, size_t b) { return c.events[a].e.aseq < c.events[b].e.aseq; });
  using Key = std::tuple<int, uint64_t, uint64_t, uint32_t>;  // (clocked, t, decode order, actor)
  auto key_of = [&](size_t i) {
    const Ev& e = c.events[i];
    const bool clocked = (e.e.flags & KZ_RT_EF_CLOCK) != 0;
    return Key(clocked ? 1 : 0, clocked ? e.e.t : 0, e.order, e.actor);
  };
  std::priority_queue<std::pair<Key, size_t>, std::vector<std::pair<Key, size_t>>, std::greater<std::pair<Key, size_t>>> heads;
  std::vector<size_t> next(n.actors.size(), 0);
  for (size_t a = 0; a < per_actor.size(); ++a)
    if (!per_actor[a].empty()) heads.push({key_of(per_actor[a][0]), a});
  std::vector<size_t> order;
  while (!heads.empty()) {
    const size_t a = heads.top().second;
    heads.pop();
    order.push_back(per_actor[a][next[a]]);
    if (++next[a] < per_actor[a].size()) heads.push({key_of(per_actor[a][next[a]]), a});
  }
  std::map<std::pair<uint32_t, uint32_t>, uint32_t> at;  // (actor, aseq) -> NEVENTS index
  for (size_t k = 0; k < order.size(); ++k) {
    Ev& e = c.events[order[k]];
    e.index = static_cast<uint32_t>(k);
    at[{e.actor, e.e.aseq}] = e.index;
  }

  // ---- events and values ----
  std::set<uint32_t> noted_binds;
  for (size_t k = 0; k < order.size(); ++k) {
    const Ev& ev = c.events[order[k]];
    const kz_rt_dec_event& e = ev.e;
    kz_nevent ne = nevent_init();
    ne.t = e.t;
    ne.actor = ev.actor;
    ne.aseq = e.aseq;
    ne.site = site_of(e.site);
    ne.bind = e.bind;
    ne.raw_off = ev.off;
    ne.flags = KZ_NEV_FLAG_ASEQ_TARGET;
    if ((e.flags & KZ_RT_EF_CLOCK) != 0) ne.flags |= KZ_NEV_FLAG_CLOCK;
    if ((e.flags & KZ_RT_EF_BOOT) != 0) ne.flags |= KZ_NEV_FLAG_BOOT;
    if ((e.flags & KZ_RT_EF_PC) != 0) ne.flags |= KZ_NEV_FLAG_PC;
    if (e.link != 0) {
      const auto l = at.find({ev.actor, e.link});
      if (l != at.end() && l->second < k) ne.link = l->second;
    }
    const kzrt::Bind* b = m.bind(e.bind);
    if (noted_binds.insert(e.bind).second) {
      if (b == nullptr) notes.push_back("binding " + std::to_string(e.bind) + " is recorded but not declared by the manifests");
      else if (b->status == "removed" || b->status == "unsupported") notes.push_back("binding " + std::to_string(e.bind) + " is declared " + b->status + " but has events");
      else if (b->site != e.site) notes.push_back("binding " + std::to_string(e.bind) + " belongs to site " + std::to_string(b->site) + ", recorded at site " + std::to_string(e.site));
    }
    const kzrt::Site* ms = m.site(e.site);
    ne.first_value = static_cast<uint32_t>(n.values.size());
    const uint32_t nv = std::min<uint32_t>(e.nvals, KZ_RT_MAX_VALS);
    for (uint32_t s = 0; s < nv; ++s) {
      if (((e.valid >> s) & 1u) == 0) continue;  // not observed: no value (never 0)
      const kzrt::Value* decl = ms != nullptr ? ms->value(s) : nullptr;
      NValue v;
      v.r.event = static_cast<uint32_t>(k);
      v.r.slot = static_cast<uint8_t>(s);
      v.r.v = e.v[s];
      v.r.kind = decl == nullptr ? KZ_NVAL_INT : decl->type == "ptr" ? KZ_NVAL_POINTER : KZ_NVAL_INT;
      if (decl != nullptr && decl->type == "ptr" && decl->string_mode == "validated" && (e.flags & KZ_RT_EF_STR) != 0 && e.str_slot == s) {
        v.r.kind = KZ_NVAL_TEXT;
        v.text.assign(reinterpret_cast<const char*>(e.str), std::min<uint32_t>(e.str_len, KZ_RT_STR_MAX));
        v.r.v = v.text.size();
        if ((e.flags & KZ_RT_EF_STR_TRUNC) != 0) v.r.flags |= KZ_NVALUE_FLAG_TEXT_TRUNCATED;
      }
      n.values.push_back(std::move(v));
    }
    ne.nvals = static_cast<uint16_t>(n.values.size() - ne.first_value);
    n.events.push_back(ne);
  }
  for (size_t i = n.events.size(); i-- > 0;) n.actors[n.events[i].actor].r.first_event = static_cast<uint32_t>(i);

  // ---- identity: the join over the manifest's identity reads (identity_rule.hpp), when it declares any ----
  {
    std::vector<IdentityRead> reads(n.sites.size());
    bool any = false;
    for (const auto& si : site_index) {
      const kzrt::Site* s = m.site(si.first);
      if (s == nullptr || !s->has_identity) continue;
      IdentityRead& r = reads[si.second];
      r.decoder = decoder_code(s->identity.decoder);
      r.id_slot = s->identity.id_slot;
      r.owner_slot = s->identity.owner_slot;
      r.status_slot = s->identity.status_slot;
      r.ok = static_cast<uint64_t>(s->identity.ok);
      const kzrt::Value* sv = s->identity.status_slot >= 0 ? s->value(static_cast<uint32_t>(s->identity.status_slot)) : nullptr;
      if (sv != nullptr) r.status_width = type_width(sv->type);
      if (!s->callee.empty()) r.request_func = s->func;  // bound at the call: the calling function is the site's func
      any = any || r.decoder != KZ_DECODER_NONE;
    }
    if (any) {
      ActorJoinStats js;
      const Status j = join_actor_identities(reads, &n, &js);
      if (!j.ok()) return j;
      notes.push_back(actor_join_text(js));
    }
  }

  // ---- losses ----
  bool lossy = !complete;
  const uint32_t ne_total = static_cast<uint32_t>(n.events.size());
  auto after_last = [&](uint32_t actor, uint32_t below_aseq) -> uint32_t {  // one past the actor's last event with aseq < below
    uint32_t pos = 0;
    for (const kz_nevent& e : n.events)
      if (e.actor == actor && e.aseq < below_aseq) pos = static_cast<uint32_t>(&e - n.events.data()) + 1;
    return pos;
  };
  // the aseq values an actor's records used: its events, and its CTL and AEXIT records (they take an aseq too)
  auto used_aseq = [&](const kz_rt_dec_actor& a, uint32_t actor) {
    std::set<uint32_t> seen;
    for (const kz_nevent& e : n.events)
      if (e.actor == actor) seen.insert(e.aseq);
    for (const auto& o : c.other_aseq)
      if (o.first == a.actor) seen.insert(o.second);
    return seen;
  };
  auto first_gap = [&](const kz_rt_dec_actor& a, uint32_t actor) -> uint32_t {  // the first missing aseq in the actor's range, or 0
    const std::set<uint32_t> seen = used_aseq(a, actor);
    for (uint32_t q = a.first_aseq; q != 0 && q <= a.last_aseq; ++q)
      if (seen.count(q) == 0) return q;
    return 0;
  };
  for (const kz_rt_dec_loss& l : c.losses) {
    kz_nloss nl = nloss_init();
    nl.kind = nloss_kind(l.kind);
    nl.count = l.count;
    if (l.count != 0 && (!no_interval(nl.kind) || nl.kind == KZ_NLOSS_ABNORMAL_END)) lossy = true;  // an abnormal end: a prefix
    if (no_interval(nl.kind)) {
      nl.begin = nl.end = ne_total;  // reported; the recorded events are whole (or the loss lies after the window)
    } else if ((nl.kind == KZ_NLOSS_OVERFLOW || nl.kind == KZ_NLOSS_PARTIAL) && l.actor != 0 && actor_of.count(l.actor) != 0 && record_of.count(l.actor) != 0) {
      const uint32_t a = actor_of[l.actor];
      nl.actor = a;
      nl.unit_kind = KZ_NLOSS_UNIT_ACTOR_BUFFER;
      nl.unit = l.actor;
      const uint32_t gap = nl.kind == KZ_NLOSS_OVERFLOW ? first_gap(*record_of[l.actor], a) : 0;
      nl.begin = after_last(a, gap != 0 ? gap : 0xFFFFFFFFu);
    } else {
      nl.begin = 0;  // where the lost records were is not established: the whole record
      if (nl.kind == KZ_NLOSS_PREINIT || nl.kind == KZ_NLOSS_PARTIAL) nl.unit_kind = KZ_NLOSS_UNIT_BOOT_BUFFER;
      else if (l.key != 0) nl.unit_kind = KZ_NLOSS_UNIT_KEY, nl.unit = l.key;
    }
    n.losses.push_back(nl);
  }
  // an aseq gap that the counted overflow does not explain
  for (const kz_rt_dec_actor& a : c.actors) {
    const uint32_t idx = actor_of[a.actor];
    uint64_t in_range = 0;
    for (uint32_t q : used_aseq(a, idx))
      if (q >= a.first_aseq && q <= a.last_aseq) ++in_range;
    const uint64_t span = a.last_aseq >= a.first_aseq && a.first_aseq != 0 ? static_cast<uint64_t>(a.last_aseq) - a.first_aseq + 1 : 0;
    const uint64_t missing = span > in_range ? span - in_range : 0;
    if (missing > static_cast<uint64_t>(a.overflow) + a.partial) {
      notes.push_back("actor " + std::to_string(a.actor) + ": " + std::to_string(missing) + " aseq gap(s), " + std::to_string(a.overflow + a.partial) + " counted");
      kz_nloss nl = nloss_init();
      nl.kind = KZ_NLOSS_UNKNOWN;
      nl.count = missing - a.overflow - a.partial;
      nl.actor = idx;
      nl.unit_kind = KZ_NLOSS_UNIT_ACTOR_BUFFER;
      nl.unit = a.actor;
      const uint32_t gap = first_gap(a, idx);
      nl.begin = after_last(idx, gap != 0 ? gap : 0xFFFFFFFFu);
      n.losses.push_back(nl);
      lossy = true;
    }
  }
  // a recovered dump says whether it is a prefix: abnormal-end unless the process had completed its own dump
  {
    bool abnormal = false;
    for (const kz_nloss& l : n.losses) abnormal = abnormal || l.kind == KZ_NLOSS_ABNORMAL_END;
    if (abnormal && !recovered) notes.push_back("LOSS abnormal-end in a dump that does not say it was recovered");
    if (abnormal && dumped) notes.push_back("LOSS abnormal-end in a recovered dump whose process had completed its own dump: kept as a prefix");
    if (recovered && !dumped && !abnormal) {
      notes.push_back("a recovered dump without LOSS abnormal-end although its process had not completed its own dump: taken as a prefix");
      kz_nloss nl = nloss_init();
      nl.kind = KZ_NLOSS_ABNORMAL_END;
      nl.count = 1;
      nl.begin = nl.end = ne_total;
      n.losses.push_back(nl);
      lossy = true;
    }
  }
  if (!complete) {
    kz_nloss nl = nloss_init();
    nl.kind = KZ_NLOSS_TRUNCATED_STREAM;
    nl.count = st.partial;
    n.losses.push_back(nl);
  }
  if (lossy) nr.r.flags |= KZ_NRUN_FLAG_LOSS_PRESENT;

  // ---- the start: every hook from the first is recorded (static bootstrap buffer, no pre-init loss) ----
  bool boot_whole = complete && (rr.phase & KZ_RT_PHASE_INIT) != 0 && (rr.mode == KZ_RT_MODE_RECORD || rr.mode == KZ_RT_MODE_CONTROL || rr.mode == KZ_RT_MODE_STOPPED);
  for (const kz_rt_dec_loss& l : c.losses)
    if (l.count != 0 && (l.kind == KZ_RT_LOSS_PREINIT || (l.kind == KZ_RT_LOSS_PARTIAL && l.actor == 0) || l.kind == KZ_RT_LOSS_REENTRY)) boot_whole = false;
  nr.r.start_evidence = boot_whole ? KZ_NSTART_BOOTSTRAP : KZ_NSTART_NONE;

  // ---- control records ----
  for (const auto& cr : c.ctls) {
    const kz_rt_dec_ctl& x = cr.first;
    kz_nctl nc = nctl_init();
    auto actor = [&](uint32_t id) -> uint32_t {
      if (id == 0) return KZ_NONE32;
      const auto it = actor_of.find(id);
      return it != actor_of.end() ? it->second : KZ_NONE32;
    };
    nc.actor = actor(x.actor);
    if (nc.actor == KZ_NONE32) {
      notes.push_back("a CTL record of actor " + std::to_string(x.actor) + " that has no neutral actor: not converted");
      continue;
    }
    nc.subject = actor(x.subject);
    nc.other = actor(x.other);
    const uint32_t hitter = nc.subject != KZ_NONE32 ? nc.subject : nc.actor;
    if (x.ev != 0) {
      const auto it = at.find({hitter, x.ev});
      if (it != at.end()) nc.event = it->second;
    }
    nc.site = x.site != 0 ? site_of(x.site) : KZ_NONE32;
    nc.bind = x.bind;
    nc.ctl_seq = x.ctl_seq;
    nc.point = x.point;
    nc.hit = x.hit;
    nc.gate = x.gate;
    nc.t = x.t;
    nc.what = static_cast<uint8_t>(x.what <= KZ_RT_CW_TIMEOUT ? x.what : 0);
    nc.reason = static_cast<uint8_t>(x.reason & 0x0Fu);
    if ((x.reason & KZ_RT_CR_DEFERRED) != 0) nc.cflags |= KZ_NCTL_FLAG_DEFERRED;
    nc.ctx = static_cast<uint8_t>(x.ctx <= KZ_RT_CTX_UNSUPPORTED ? x.ctx : 0);
    nc.raw_off = cr.second;
    n.ctl.push_back(nc);
  }
  return check_neutral(n);
}

Status convert_kzrt_container(const KzrtConvertInput& in, KzrtConvertResult* out) {
  Status s = neutral_from_kzrt(in, out);
  if (!s.ok()) return s;
  ContainerWriter w(KZ_ARTIFACT_RUN);
  w.set_tool_hash(in.tool_sha256);
  if (in.basis != nullptr) {
    if (in.basis->header().artifact_kind != KZ_ARTIFACT_BASIS) return Status::error(Err::Invalid, "--basis: not a basis container");
    w.set_basis_hash(in.basis->artifact_hash());
    s = w.add_parent(KZ_ARTIFACT_BASIS, 0, in.basis->artifact_hash());
    if (!s.ok()) return s;
  }
  std::vector<kz_basis_item> items;
  auto item = [&](const std::string& name, const std::string& value) {
    kz_basis_item b = zeroed<kz_basis_item>();
    b.name = w.intern(name);
    b.value = w.intern(value);
    b.policy = KZ_POLICY_UNSET;
    b.prov = KZ_NONE32;
    b.deviation = KZ_NONE32;
    items.push_back(b);
  };
  std::vector<kz_file_hash> files;
  auto file = [&](const std::string& path, const void* data, size_t size) {
    kz_file_hash f = zeroed<kz_file_hash>();
    f.path = w.intern(path);
    const Digest d = sha256(data, size);
    std::memcpy(f.sha256, d.data(), 32);
    f.size = size;
    f.prov = KZ_NONE32;
    files.push_back(f);
  };
  item("convert.input", "kzrt-dump");
  item("convert.dump.sha256", to_hex(sha256(in.dump.data(), in.dump.size())));
  file(in.dump_name, in.dump.data(), in.dump.size());
  for (const auto& mf : in.manifests) {
    item("convert.manifest", mf.first + " " + to_hex(sha256(mf.second)));
    file(mf.first, mf.second.data(), mf.second.size());
  }
  if (in.harness_process != 0) item("convert.harness_process", std::to_string(in.harness_process));
  for (const std::string& note : out->notes) item("convert.note", note);
  s = w.add_section(KZ_SEC_BASIS, items);
  if (s.ok()) s = w.add_section(KZ_SEC_FILE_HASHES, files);
  if (s.ok()) s = add_neutral_sections(out->layer, &w);
  if (s.ok()) s = w.finish(&out->container);
  return s;
}

}  // namespace kz::trace

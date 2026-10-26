// kz-rt-dump -- print a "kzrt-dump" file (format major 1) and check it against site manifests.
//
//   kz-rt-dump [--manifest <kzs-sites file>]... [--events-only] <dump-file>
//
// Output: one line per record, in stream order (and buffer order inside each BUF), then a check
// section: per-actor sequence gaps (records the actor attempted but the dump lacks) next to the
// counted losses, and, with manifests, every event's site/binding, its valid mask against the
// binding's vals, string copies only at string=validated slots, and manifest modules that never
// registered.  Exit status: 0 = read and checks passed, 1 = a check failed, 2 = usage or format error.
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "kz_rt_decode.h"
#include "kz_rt_manifest.hpp"

namespace {

std::string actor_name(uint32_t a) {
  if (a == KZ_RT_ACTOR_UNRESOLVED) return "?";
  if ((a & KZ_RT_ACTOR_PROVISIONAL) != 0) return "P" + std::to_string(a & ~KZ_RT_ACTOR_PROVISIONAL);
  return std::to_string(a);
}

const char* mode_name(uint32_t m) {
  switch (m) {
    case KZ_RT_MODE_OFF: return "off";
    case KZ_RT_MODE_RECORD: return "record";
    case KZ_RT_MODE_CONTROL: return "control";
    case KZ_RT_MODE_STOPPED: return "stopped";
    default: return "?";
  }
}

const char* loss_name(uint32_t k) {
  switch (k) {
    case KZ_RT_LOSS_OVERFLOW: return "overflow";
    case KZ_RT_LOSS_PARTIAL: return "partial";
    case KZ_RT_LOSS_PREINIT: return "preinit";
    case KZ_RT_LOSS_EXCLUDED: return "excluded";
    case KZ_RT_LOSS_UNLOADED: return "unloaded";
    case KZ_RT_LOSS_REENTRY: return "reentry";
    case KZ_RT_LOSS_ACTOR_TABLE: return "actor-table";
    case KZ_RT_LOSS_UNRESOLVED: return "unresolved";
    case KZ_RT_LOSS_CTL_SKIPPED: return "ctl-skipped";
    case KZ_RT_LOSS_CTL_NOCOND: return "ctl-nocond";
    case KZ_RT_LOSS_MODULE_TABLE: return "module-table";
    case KZ_RT_LOSS_AFTER_STOP: return "after-stop";
    case KZ_RT_LOSS_NOTE_TABLE: return "note-table";
    default: return "?";
  }
}

const char* what_name(uint32_t w) {
  switch (w) {
    case KZ_RT_CW_GATE_HOLD: return "gate-hold";
    case KZ_RT_CW_GATE_REFUSED: return "gate-refused";
    case KZ_RT_CW_GATE_CONSUMED: return "gate-consumed";
    case KZ_RT_CW_GATE_BUSY: return "gate-busy";
    case KZ_RT_CW_FIRE: return "fire";
    case KZ_RT_CW_RESUME: return "resume";
    case KZ_RT_CW_TIMEOUT: return "timeout";
    default: return "?";
  }
}

std::string reason_name(uint32_t r) {
  std::string s;
  switch (r & ~KZ_RT_CR_DEFERRED) {
    case KZ_RT_CR_NONE: s = "-"; break;
    case KZ_RT_CR_RESUMED: s = "resumed"; break;
    case KZ_RT_CR_SATISFIED: s = "satisfied"; break;
    case KZ_RT_CR_CONSUMED: s = "consumed"; break;
    case KZ_RT_CR_NO_WAIT: s = "no-wait"; break;
    case KZ_RT_CR_PRE_ATTACH: s = "pre-attach"; break;
    case KZ_RT_CR_CONTEXT: s = "context"; break;
    case KZ_RT_CR_NO_SLOT: s = "no-slot"; break;
    default: s = "?"; break;
  }
  if ((r & KZ_RT_CR_DEFERRED) != 0) s += ",deferred";
  return s;
}

std::string event_flags(uint32_t f) {
  static const std::pair<uint32_t, const char*> names[] = {{KZ_RT_EF_CLOCK, "clock"}, {KZ_RT_EF_PC, "pc"},       {KZ_RT_EF_STR, "str"},
                                                           {KZ_RT_EF_STR_TRUNC, "str-trunc"}, {KZ_RT_EF_BOOT, "boot"}, {KZ_RT_EF_UNRESOLVED, "unresolved"},
                                                           {KZ_RT_EF_NOSLOT, "noslot"}};
  std::string s;
  for (const auto& n : names)
    if ((f & n.first) != 0) s += (s.empty() ? "" : ",") + std::string(n.second);
  return s.empty() ? "-" : s;
}

std::string ctx_mask_text(uint32_t m) {
  std::string s;
  for (uint32_t c = 1; c <= KZ_RT_CTX_UNSUPPORTED; ++c)
    if ((m & (1u << c)) != 0) s += (s.empty() ? "" : ",") + std::string(kzrt::ctx_name(c));
  return s.empty() ? "none" : s;
}

std::string hex(const unsigned char* p, size_t n) {
  static const char* d = "0123456789abcdef";
  std::string s;
  bool nonzero = false;
  for (size_t i = 0; i < n; ++i) {
    s.push_back(d[p[i] >> 4]);
    s.push_back(d[p[i] & 15]);
    nonzero = nonzero || p[i] != 0;
  }
  return nonzero ? s : "none";
}

std::string quoted(const unsigned char* p, size_t n) {
  std::string s = "\"";
  for (size_t i = 0; i < n; ++i) {
    const unsigned char c = p[i];
    if (c == '"' || c == '\\') {
      s.push_back('\\');
      s.push_back(static_cast<char>(c));
    } else if (c >= 0x20 && c < 0x7f) {
      s.push_back(static_cast<char>(c));
    } else {
      char b[8];
      std::snprintf(b, sizeof b, "\\x%02x", c);
      s += b;
    }
  }
  return s + "\"";
}

struct State {
  const kzrt::Manifest* m = nullptr;
  bool events_only = false;
  int failures = 0;
  std::map<uint32_t, std::vector<uint32_t>> aseqs;  // actor -> aseqs seen (events, ctl, aexit)
  std::map<uint32_t, kz_rt_dec_actor> actors;
  std::map<uint32_t, uint64_t> overflow;            // actor -> counted overflow
  std::set<uint32_t> modules_seen;

  void fail(const std::string& msg) {
    std::printf("check FAIL %s\n", msg.c_str());
    ++failures;
  }
  std::string site_text(uint32_t site) const {
    if (m == nullptr) return std::to_string(site);
    const kzrt::Site* s = m->site(site);
    return std::to_string(site) + (s != nullptr ? "[" + s->label + "]" : "[?]");
  }
};

void check_event(State* st, const kz_rt_dec_event& e) {
  if (st->m == nullptr) return;
  const kzrt::Site* s = st->m->site(e.site);
  const kzrt::Bind* b = st->m->bind(e.bind);
  const std::string where = "event actor=" + actor_name(e.actor) + " aseq=" + std::to_string(e.aseq);
  if (s == nullptr) {
    st->fail(where + ": site " + std::to_string(e.site) + " is not in the manifest");
    return;
  }
  if (b == nullptr) {
    st->fail(where + ": binding " + std::to_string(e.bind) + " is not in the manifest");
    return;
  }
  if (b->site != e.site) st->fail(where + ": binding " + std::to_string(e.bind) + " belongs to site " + std::to_string(b->site));
  if (b->status == "removed" || b->status == "unsupported") st->fail(where + ": binding " + std::to_string(e.bind) + " is declared " + b->status);
  if ((e.valid & ~b->vals) != 0) st->fail(where + ": valid mask has slots the binding does not bind");
  if ((e.flags & KZ_RT_EF_STR) != 0) {
    const kzrt::Value* v = s->value(e.str_slot);
    if (v == nullptr || v->string_mode != "validated")
      st->fail(where + ": string copied from slot " + std::to_string(e.str_slot) + ", which the manifest does not mark string=validated");
    else if (e.str_len > v->bound)
      st->fail(where + ": string copy longer than the manifest bound");
  }
}

int on_item(void* ctx, const kz_rt_dec_item* it) {
  State* st = static_cast<State*>(ctx);
  const bool meta = !st->events_only;
  switch (it->type) {
    case KZ_RT_REC_RUN: {
      const kz_rt_dec_run& r = it->u.run;
      if (meta) {
        std::printf("run abi=%u mode=%s run_id=%" PRIu64 " instance=%u phase=0x%x adapter=\"%s\" adapter_caps=0x%x clock=%u runtime=\"%s\"\n", r.abi,
                    mode_name(r.mode), r.run_id, r.instance_id, r.phase, r.adapter, r.adapter_caps, r.clock_id, r.runtime);
        std::printf("run backend=\"%s\" compiler=\"%s\" target=\"%s\" manifest_sha256=%s\n", r.backend, r.compiler, r.target,
                    hex(r.manifest_sha256, 32).c_str());
        std::printf("run t_init=%" PRIu64 " t_attach=%" PRIu64 " t_stop=%" PRIu64 " t_dump=%" PRIu64 "\n", r.t_init, r.t_attach, r.t_stop, r.t_dump);
        std::printf("caps max_actors=%u actor_buf_bytes=%u boot_buf_bytes=%u max_points=%u max_vals=%u str_max=%u holdable=%s\n", r.max_actors,
                    r.actor_buf_bytes, r.boot_buf_bytes, r.max_points, r.max_vals, r.str_max, ctx_mask_text(r.holdable_ctx_mask).c_str());
      }
      break;
    }
    case KZ_RT_REC_MODULE:
      st->modules_seen.insert(it->u.module.module);
      if (meta)
        std::printf("module %u actor=%s t=%" PRIu64 " manifest_sha256=%s\n", it->u.module.module, actor_name(it->u.module.actor).c_str(), it->u.module.t,
                    hex(it->u.module.manifest_sha256, 32).c_str());
      break;
    case KZ_RT_REC_RESOURCE:
      if (meta)
        std::printf("resource kind=%u origin=%s count=%" PRIu64 " what=\"%s\"\n", it->u.resource.kind,
                    it->u.resource.origin == KZ_RT_ORIGIN_TARGET_TABLE ? "target-table" : "private", it->u.resource.count, it->u.resource.what);
      break;
    case KZ_RT_REC_ACTOR: {
      const kz_rt_dec_actor& a = it->u.actor;
      st->actors[a.actor] = a;
      if (meta)
        std::printf("actor %s boot_link=%s evidence=%s key=%u:%" PRIu64 " gen=%u aseq=%u..%u recorded=%u overflow=%u partial=%u buf_bytes=%u flags=%s%s\n",
                    actor_name(a.actor).c_str(), a.boot_link != 0 ? actor_name(a.boot_link).c_str() : "-",
                    a.link_evidence == KZ_RT_LINK_SAME_CELL ? "same-cell" : "-", a.key_kind, a.native_key, a.key_generation, a.first_aseq, a.last_aseq,
                    a.recorded, a.overflow, a.partial, a.buf_bytes, (a.flags & KZ_RT_AF_EXITED) != 0 ? "exited" : "-",
                    (a.flags & KZ_RT_AF_HELD) != 0 ? ",held" : "");
      break;
    }
    case KZ_RT_REC_LOSS:
      if (it->u.loss.kind == KZ_RT_LOSS_OVERFLOW) st->overflow[it->u.loss.actor] += it->u.loss.count;
      if (meta)
        std::printf("loss %s actor=%s key=%u count=%" PRIu64 "\n", loss_name(it->u.loss.kind), it->u.loss.actor != 0 ? actor_name(it->u.loss.actor).c_str() : "-",
                    it->u.loss.key, it->u.loss.count);
      break;
    case KZ_RT_REC_POINT: {
      const kz_rt_dec_point& q = it->u.point;
      if (meta) {
        const std::string occ = q.occurrence == 0 ? "each" : q.occurrence == 1 ? "first" : std::to_string(q.occurrence);
        std::printf("point %u %s label=%u site=%s releases=%s occurrence=%s flags=0x%x hits=%u acted=%u satisfied=%u held=%s holds=%u\n", q.index,
                    q.kind == KZ_RT_POINT_GATE ? "gate" : "release", q.label, st->site_text(q.site).c_str(),
                    q.kind == KZ_RT_POINT_RELEASE ? std::to_string(q.releases).c_str() : "-", occ.c_str(), q.flags, q.hits, q.acted, q.satisfied,
                    q.held_actor != 0 ? (actor_name(q.held_actor) + " (unreleased at dump)").c_str() : "-", q.holds);
      }
      break;
    }
    case KZ_RT_REC_BUF:
      if (meta)
        std::printf("buf owner=%s bytes=%u cap=%u\n", it->u.buf.owner == 0 ? "shared" : actor_name(it->u.buf.owner).c_str(), it->u.buf.nbytes, it->u.buf.cap);
      break;
    case KZ_RT_REC_EVENT: {
      const kz_rt_dec_event& e = it->u.event;
      std::string vals;
      for (uint32_t i = 0; i < e.nvals; ++i) {
        char b[40];
        if ((e.valid & (1u << i)) != 0)
          std::snprintf(b, sizeof b, "%s0x%" PRIx64, i == 0 ? "" : ",", e.v[i]);
        else
          std::snprintf(b, sizeof b, "%s-", i == 0 ? "" : ",");
        vals += b;
      }
      std::printf("ev actor=%s aseq=%u site=%s bind=%u link=%u t=%" PRIu64 " v=[%s]%s%s flags=%s\n", actor_name(e.actor).c_str(), e.aseq,
                  st->site_text(e.site).c_str(), e.bind, e.link, e.t, vals.c_str(), (e.flags & KZ_RT_EF_STR) != 0 ? " str=" : "",
                  (e.flags & KZ_RT_EF_STR) != 0 ? quoted(e.str, e.str_len).c_str() : "", event_flags(e.flags).c_str());
      st->aseqs[e.actor].push_back(e.aseq);
      check_event(st, e);
      break;
    }
    case KZ_RT_REC_CTL: {
      const kz_rt_dec_ctl& c = it->u.ctl;
      std::printf("ctl actor=%s aseq=%u ctl_seq=%u what=%s site=%s bind=%u ev=%u point=%u hit=%u subject=%s other=%s gate=%u reason=%s ctx=%s t=%" PRIu64 "\n",
                  actor_name(c.actor).c_str(), c.aseq, c.ctl_seq, what_name(c.what), st->site_text(c.site).c_str(), c.bind, c.ev, c.point, c.hit,
                  actor_name(c.subject).c_str(), c.other != 0 ? actor_name(c.other).c_str() : "-", c.gate, reason_name(c.reason).c_str(),
                  kzrt::ctx_name(c.ctx), c.t);
      st->aseqs[c.actor].push_back(c.aseq);
      break;
    }
    case KZ_RT_REC_AEXIT:
      std::printf("aexit actor=%s aseq=%u t=%" PRIu64 "\n", actor_name(it->u.aexit.actor).c_str(), it->u.aexit.aseq, it->u.aexit.t);
      st->aseqs[it->u.aexit.actor].push_back(it->u.aexit.aseq);
      break;
    case KZ_RT_REC_END:
      if (meta) std::printf("end bytes=%" PRIu64 " crc=%s\n", it->u.end.bytes, it->u.end.crc_ok ? "ok" : "MISMATCH");
      if (!it->u.end.crc_ok) st->fail("END: byte count or CRC-32 does not match the stream");
      break;
    default:
      break;
  }
  return 0;
}

bool read_file(const std::string& path, std::string* out) {
  std::ifstream f(path, std::ios::binary);
  if (!f) return false;
  std::ostringstream ss;
  ss << f.rdbuf();
  *out = ss.str();
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  std::vector<std::string> manifests;
  std::string dump;
  bool events_only = false;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a == "--manifest" && i + 1 < argc)
      manifests.push_back(argv[++i]);
    else if (a == "--events-only")
      events_only = true;
    else if (!a.empty() && a[0] != '-' && dump.empty())
      dump = a;
    else {
      std::fprintf(stderr, "usage: kz-rt-dump [--manifest <kzs-sites file>]... [--events-only] <dump-file>\n");
      return 2;
    }
  }
  if (dump.empty()) {
    std::fprintf(stderr, "usage: kz-rt-dump [--manifest <kzs-sites file>]... [--events-only] <dump-file>\n");
    return 2;
  }
  kzrt::Manifest m;
  for (const std::string& path : manifests) {
    std::string text, err;
    if (!read_file(path, &text)) {
      std::fprintf(stderr, "kz-rt-dump: cannot read %s\n", path.c_str());
      return 2;
    }
    if (!kzrt::parse_manifest(text, path, &m, &err)) {
      std::fprintf(stderr, "kz-rt-dump: %s\n", err.c_str());
      return 2;
    }
  }
  std::string bytes;
  if (!read_file(dump, &bytes)) {
    std::fprintf(stderr, "kz-rt-dump: cannot read %s\n", dump.c_str());
    return 2;
  }
  State st;
  st.m = manifests.empty() ? nullptr : &m;
  st.events_only = events_only;
  kz_rt_dec_stats ds;
  const int r = kz_rt_decode(reinterpret_cast<const unsigned char*>(bytes.data()), bytes.size(), on_item, &st, &ds);
  std::printf("format kzrt-dump %u.%u dump_flags=0x%x records=%" PRIu64 " unknown=%" PRIu64 " partial=%" PRIu64 " pads=%" PRIu64 " end=%s\n", ds.major,
              ds.minor, ds.dump_flags, ds.records, ds.unknown, ds.partial, ds.pads, ds.end_seen ? "yes" : "no");
  if (r != KZ_RT_DEC_OK) {
    std::printf("decode error: %s\n", kz_rt_dec_error(r));
    if (r != KZ_RT_DEC_ETRUNC) return 2;
    ++st.failures;
  }
  // Sequence gaps per actor: each record took one aseq of its actor, so a missing number is a record
  // the actor attempted and the dump lacks (overflow, or after the dump read the buffer).
  for (const auto& e : st.aseqs) {
    const uint32_t a = e.first;
    if (a == KZ_RT_ACTOR_UNRESOLVED) continue;  // shared numbering, not program order
    // A run actor's own range starts at first_aseq (lower numbers are its provisional records in the
    // shared buffer, whose losses are counted globally as preinit); a provisional actor is reported only.
    std::set<uint32_t> seen(e.second.begin(), e.second.end());
    uint32_t lo = (a & KZ_RT_ACTOR_PROVISIONAL) != 0 ? 1u : *seen.begin(), hi = *seen.rbegin();
    auto ai = st.actors.find(a);
    if (ai != st.actors.end()) {
      lo = ai->second.first_aseq;
      hi = ai->second.last_aseq > hi ? ai->second.last_aseq : hi;
    }
    uint64_t missing = 0;
    for (uint32_t s = lo; s <= hi && s != 0; ++s) missing += seen.count(s) == 0 ? 1u : 0u;
    const uint64_t counted = st.overflow.count(a) != 0 ? st.overflow[a] : 0;
    std::printf("check actor %s aseq %u..%u missing=%" PRIu64 " overflow_counted=%" PRIu64 "%s\n", actor_name(a).c_str(), lo, hi, missing, counted,
                (a & KZ_RT_ACTOR_PROVISIONAL) != 0 ? " (provisional)" : "");
    if ((a & KZ_RT_ACTOR_PROVISIONAL) == 0 && missing != counted) st.fail("actor " + actor_name(a) + ": sequence gaps differ from the counted overflow");
  }
  if (st.m != nullptr)
    for (const kzrt::Module& md : st.m->modules)
      if (st.modules_seen.count(md.id) == 0)
        std::printf("check coverage module %u (%s) never registered: not loaded, unloaded or not run\n", md.id, md.name.c_str());
  std::printf("check %s (%d failure%s)\n", st.failures == 0 ? "ok" : "FAILED", st.failures, st.failures == 1 ? "" : "s");
  return st.failures == 0 ? 0 : 1;
}

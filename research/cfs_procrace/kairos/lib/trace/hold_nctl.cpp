// hold_nctl.cpp -- the realized control of an F0 hold run as NCTL records (see hold_nctl.hpp).

#include "kz/trace/hold_nctl.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <map>
#include <tuple>

namespace kz::trace {

namespace {

template <class T>
T zeroed() {
  T v;
  std::memset(&v, 0, sizeof v);
  return v;
}

std::string basename_of(const std::string& p) {
  const size_t s = p.find_last_of('/');
  return s == std::string::npos ? p : p.substr(s + 1);
}

// "key=value" tokens of a line (a value in double quotes may hold spaces).
std::map<std::string, std::string> fields(const std::string& line, size_t from) {
  std::map<std::string, std::string> out;
  size_t i = from;
  while (i < line.size()) {
    while (i < line.size() && line[i] == ' ') ++i;
    const size_t eq = line.find('=', i);
    const size_t sp = line.find(' ', i);
    if (eq == std::string::npos || (sp != std::string::npos && sp < eq)) {  // a bare word
      const size_t end = sp == std::string::npos ? line.size() : sp;
      if (end > i) out[line.substr(i, end - i)] = "";
      i = end;
      continue;
    }
    const std::string key = line.substr(i, eq - i);
    size_t v = eq + 1;
    std::string value;
    if (v < line.size() && line[v] == '"') {
      const size_t close = line.find('"', v + 1);
      value = line.substr(v + 1, (close == std::string::npos ? line.size() : close) - v - 1);
      i = close == std::string::npos ? line.size() : close + 1;
    } else {
      const size_t end = line.find(' ', v);
      value = line.substr(v, (end == std::string::npos ? line.size() : end) - v);
      i = end == std::string::npos ? line.size() : end;
    }
    out[key] = value;
  }
  return out;
}

bool number(const std::string& s, uint64_t* v) {
  if (s.empty()) return false;
  char* end = nullptr;
  *v = std::strtoull(s.c_str(), &end, 0);
  return end != nullptr && *end == '\0';
}

}  // namespace

Status parse_hold_record(const std::string& text, HoldRecord* out) {
  *out = HoldRecord();
  size_t pos = 0;
  bool header = false;
  while (pos < text.size()) {
    const size_t nl = text.find('\n', pos);
    const std::string line = text.substr(pos, (nl == std::string::npos ? text.size() : nl) - pos);
    const uint64_t offset = pos;
    pos = nl == std::string::npos ? text.size() : nl + 1;
    auto bad = [&](const std::string& why) { return Status::error(Err::Invalid, "hold record line at byte " + std::to_string(offset) + ": " + why); };
    if (line.rfind("# kz hold run record", 0) == 0) {
      header = true;
      continue;
    }
    if (line.empty() || line[0] == '#' || line.rfind("command:", 0) == 0 || line.rfind("note:", 0) == 0) continue;
    if (line.rfind("debugregs_ok=", 0) == 0) {
      out->debugregs_ok = line == "debugregs_ok=yes";
      continue;
    }
    if (line.rfind("gate ", 0) == 0 || line.rfind("release ", 0) == 0) {
      HoldPoint hp;
      hp.release = line[0] == 'r';
      std::string rest = line.substr(hp.release ? 8 : 5);
      size_t i = rest.find_first_not_of(' ');
      if (i == std::string::npos) return bad("a control point without a label");
      const size_t a = rest.find(' ', i);
      hp.label = rest.substr(i, a - i);
      const size_t b0 = a == std::string::npos ? std::string::npos : rest.find_first_not_of(' ', a);
      const size_t b1 = b0 == std::string::npos ? std::string::npos : rest.find(' ', b0);
      const std::string where = b0 == std::string::npos ? std::string() : rest.substr(b0, (b1 == std::string::npos ? rest.size() : b1) - b0);
      const size_t colon = where.rfind(':');
      if (colon == std::string::npos || !number(where.substr(colon + 1), &hp.file_offset)) return bad("a control point without binary:offset");
      hp.binary = where.substr(0, colon);
      const auto f = fields(rest, b1 == std::string::npos ? rest.size() : b1);
      if (hp.release) {
        const auto r = f.find("releases");
        if (r == f.end() || r->second.empty()) return bad("a release without releases=");
        hp.releases = r->second;
      }
      out->points.push_back(hp);
      continue;
    }
    if (line.rfind("hold ", 0) == 0) {
      const auto f = fields(line, 5);
      HoldFact h;
      h.offset = offset;
      uint64_t tid = 0;
      h.gate = f.count("gate") != 0 ? f.at("gate") : std::string();
      if (h.gate.empty() || f.count("tid") == 0 || !number(f.at("tid"), &tid) || f.count("held_ns") == 0 || !number(f.at("held_ns"), &h.held_ns))
        return bad("a hold without gate, tid or held_ns");
      h.tid = static_cast<uint32_t>(tid);
      if (f.count("hit") != 0 && !number(f.at("hit"), &h.hit)) return bad("a hold with a bad hit");
      if (f.count("hit") == 0) h.hit = 1;
      if (f.count("released_by") != 0) {
        h.released = true;
        h.released_by = f.at("released_by");
        if (f.count("release_ns") == 0 || !number(f.at("release_ns"), &h.release_ns)) return bad("a released hold without release_ns");
      }
      out->holds.push_back(h);
      continue;
    }
    if (line.rfind("fire ", 0) == 0) {
      const auto f = fields(line, 5);
      FireFact x;
      x.offset = offset;
      uint64_t tid = 0;
      if (f.count("release") == 0 || f.count("gate") == 0 || f.count("tid") == 0 || !number(f.at("tid"), &tid) || f.count("t_ns") == 0 || !number(f.at("t_ns"), &x.t_ns))
        return bad("a fire without release, gate, tid or t_ns");
      x.release = f.at("release");
      x.gate = f.at("gate");
      x.tid = static_cast<uint32_t>(tid);
      if (f.count("hit") != 0 && !number(f.at("hit"), &x.hit)) return bad("a fire with a bad hit");
      if (f.count("hit") == 0) x.hit = 1;
      const auto r = f.find("resumed");
      uint64_t resumed = 0;
      if (r != f.end() && r->second != "none") {
        if (!number(r->second, &resumed) || resumed == 0) return bad("a fire with a bad resumed=");
        x.resumed = static_cast<uint32_t>(resumed);
      }
      x.satisfied = f.count("satisfied") != 0 && f.at("satisfied") == "yes";
      x.consumed = f.count("consumed") != 0 && f.at("consumed") == "yes";
      x.deferred = f.count("deferred") != 0 && f.at("deferred") == "yes";
      out->fires.push_back(x);
      continue;
    }
    // the other key=value lines of the record (primary_binary, child_pid, wall_ns, perturbation counts, ...)
    if (line.find('=') != std::string::npos && line.find(' ') == std::string::npos) continue;
    if (line.rfind("total_stops=", 0) == 0) continue;
    return bad("not a line of a kz hold run record");
  }
  if (!header) return Status::error(Err::Invalid, "not a kz hold run record (no header line)");
  return Status::success();
}

Status add_hold_control(const HoldRecord& h, NeutralRun* n, std::vector<std::string>* notes) {
  // the point indices: gates first, then releases, each in the record's order
  std::map<std::string, uint32_t> index;
  std::map<std::string, const HoldPoint*> point;
  uint32_t k = 0;
  for (int pass = 0; pass < 2; ++pass)
    for (const HoldPoint& p : h.points)
      if (p.release == (pass == 1)) {
        index[p.label] = k++;
        point[p.label] = &p;
      }
  // the observer's sites at a point's binary and file offset (an entry or a point probe; a return probe shares the
  // entry's address but observes the return)
  auto sites_at = [&](const HoldPoint& p) {
    std::vector<uint32_t> out;
    for (size_t i = 0; i < n->sites.size(); ++i) {
      const NSite& s = n->sites[i];
      if (s.r.kind != KZ_NSITE_KIND_CALL && s.r.kind != KZ_NSITE_KIND_POINT) continue;
      const size_t plus = s.at.rfind('+');
      uint64_t off = 0;
      if (plus == std::string::npos || !number(s.at.substr(plus + 1), &off)) continue;
      if (off == p.file_offset && basename_of(s.at.substr(0, plus)) == p.binary) out.push_back(static_cast<uint32_t>(i));
    }
    return out;
  };
  // each actor's events in its order
  std::vector<std::vector<uint32_t>> by_actor(n->actors.size());
  for (size_t i = 0; i < n->events.size(); ++i)
    if (n->events[i].actor < n->actors.size()) by_actor[n->events[i].actor].push_back(static_cast<uint32_t>(i));
  for (std::vector<uint32_t>& v : by_actor)
    std::stable_sort(v.begin(), v.end(), [&](uint32_t a, uint32_t b) { return n->events[a].aseq < n->events[b].aseq; });
  // the actor of a thread id at a time: the actor bound to that Linux thread whose events span the time, else the one whose
  // first event is the latest before it; none if no actor has that id
  auto actor_of = [&](uint32_t tid, uint64_t t, std::string* why) -> uint32_t {
    std::vector<uint32_t> cand;
    for (size_t i = 0; i < n->actors.size(); ++i)
      if (n->actors[i].r.binding == KZ_NBIND_LINUX_TID && n->actors[i].r.native_key == tid) cand.push_back(static_cast<uint32_t>(i));
    if (cand.empty()) {
      *why = "thread " + std::to_string(tid) + " has no actor in the record";
      return KZ_NONE32;
    }
    if (cand.size() == 1) return cand.front();
    uint32_t best = KZ_NONE32;
    uint64_t best_first = 0;
    for (uint32_t a : cand) {
      if (by_actor[a].empty()) continue;
      const uint64_t first = n->events[by_actor[a].front()].t;
      if (first <= t && (best == KZ_NONE32 || first >= best_first)) {
        best = a;
        best_first = first;
      }
    }
    if (best == KZ_NONE32) *why = "thread " + std::to_string(tid) + " names " + std::to_string(cand.size()) + " actors, none with an event before the fact";
    return best;
  };
  // the hit's observation event: the actor's first event after `t` (and none before `until`, the end of a hold), at one of `sites`
  auto observation = [&](uint32_t actor, uint64_t t, uint64_t until, const std::vector<uint32_t>& sites, std::string* why) -> uint32_t {
    if (sites.empty()) {
      *why = "the observer has no entry or point probe at the control point's address";
      return KZ_NONE32;
    }
    uint32_t first = KZ_NONE32;
    for (uint32_t e : by_actor[actor])
      if (n->events[e].t > t) {
        first = e;
        break;
      }
    if (first == KZ_NONE32) {
      *why = "the actor has no event after the control acted";
      return KZ_NONE32;
    }
    if (until != 0 && n->events[first].t < until) {
      *why = "the actor has an event (#" + std::to_string(first) + ") between its hold and its release: the record and the hold disagree";
      return KZ_NONE32;
    }
    if (std::find(sites.begin(), sites.end(), n->events[first].site) == sites.end()) {
      const uint32_t s = n->events[first].site;
      *why = "the actor's first event after the control acted (#" + std::to_string(first) + ") is at " + (s < n->sites.size() ? n->sites[s].label : std::string("no site")) +
             ", not at the control point's site";
      return KZ_NONE32;
    }
    return first;
  };
  struct Pending {
    kz_nctl c;
    int order;  // gate-hold 0, fire 1, resume 2 at the same time
  };
  std::vector<Pending> recs;
  auto base = [&](uint64_t t, uint64_t offset) {
    kz_nctl c = nctl_init();
    c.t = t;
    c.raw_off = offset;
    c.ctx = KZ_NCTX_UNDECLARED;
    return c;
  };
  auto site_of = [&](const std::vector<uint32_t>& s, uint32_t event) {
    if (event != KZ_NONE32) return n->events[event].site;
    return s.size() == 1 ? s.front() : KZ_NONE32;
  };
  std::map<std::string, uint32_t> resumer;  // "<gate> <held tid>" -> the actor of the release that resumed it
  for (const FireFact& f : h.fires) {
    if (f.resumed == 0) continue;
    std::string why;
    const uint32_t a = actor_of(f.tid, f.t_ns, &why);
    if (a != KZ_NONE32) resumer[f.gate + " " + std::to_string(f.resumed)] = a;
  }
  for (const HoldFact& hf : h.holds) {
    const auto pi = point.find(hf.gate);
    const std::string what = "hold " + hf.gate + " hit " + std::to_string(hf.hit) + " tid " + std::to_string(hf.tid);
    if (pi == point.end() || pi->second->release) {
      notes->push_back(what + ": the record lists no such gate: not converted");
      continue;
    }
    std::string why;
    const uint32_t a = actor_of(hf.tid, hf.held_ns, &why);
    if (a == KZ_NONE32) {
      notes->push_back(what + ": " + why + ": not converted");
      continue;
    }
    const std::vector<uint32_t> s = sites_at(*pi->second);
    const uint32_t ev = observation(a, hf.held_ns, hf.released ? hf.release_ns : 0, s, &why);
    kz_nctl c = base(hf.held_ns, hf.offset);
    c.what = KZ_NCTL_WHAT_GATE_HOLD;
    c.actor = a;
    c.subject = a;
    c.event = ev;
    c.site = site_of(s, ev);
    c.bind = c.site != KZ_NONE32 ? n->sites[c.site].r.bind : 0;
    c.point = index[hf.gate];
    c.gate = index[hf.gate];
    c.hit = static_cast<uint32_t>(hf.hit);
    c.cflags = ev != KZ_NONE32 ? KZ_NCTL_FLAG_OBS_AFTER : KZ_NCTL_FLAG_NO_OBS;
    notes->push_back(what + (ev != KZ_NONE32 ? ": observation #" + std::to_string(ev) + " " + n->sites[n->events[ev].site].label : ": no observation event (" + why + ")"));
    recs.push_back({c, 0});
    if (!hf.released) {
      n->actors[a].r.flags |= KZ_NACTOR_FLAG_HELD_AT_END;
      continue;
    }
    kz_nctl r = c;
    r.what = KZ_NCTL_WHAT_RESUME;
    r.t = hf.release_ns;
    const auto rs = resumer.find(hf.gate + " " + std::to_string(hf.tid));
    r.other = rs != resumer.end() ? rs->second : KZ_NONE32;
    recs.push_back({r, 2});
  }
  for (const FireFact& f : h.fires) {
    const auto pi = point.find(f.release);
    const std::string what = "fire " + f.release + " hit " + std::to_string(f.hit) + " tid " + std::to_string(f.tid);
    if (pi == point.end() || !pi->second->release || index.count(f.gate) == 0) {
      notes->push_back(what + ": the record lists no such release or gate: not converted");
      continue;
    }
    std::string why;
    const uint32_t a = actor_of(f.tid, f.t_ns, &why);
    if (a == KZ_NONE32) {
      notes->push_back(what + ": " + why + ": not converted");
      continue;
    }
    const std::vector<uint32_t> s = sites_at(*pi->second);
    const uint32_t ev = observation(a, f.t_ns, 0, s, &why);
    kz_nctl c = base(f.t_ns, f.offset);
    c.what = KZ_NCTL_WHAT_FIRE;
    c.reason = f.resumed != 0 ? KZ_NCTL_REASON_RESUMED : f.satisfied ? KZ_NCTL_REASON_SATISFIED : KZ_NCTL_REASON_CONSUMED;
    c.actor = a;
    c.subject = a;
    if (f.resumed != 0) {
      std::string w2;
      c.other = actor_of(f.resumed, f.t_ns, &w2);
      if (c.other == KZ_NONE32) notes->push_back(what + ": the resumed thread: " + w2);
    }
    c.event = ev;
    c.site = site_of(s, ev);
    c.bind = c.site != KZ_NONE32 ? n->sites[c.site].r.bind : 0;
    c.point = index[f.release];
    c.gate = index[f.gate];
    c.hit = static_cast<uint32_t>(f.hit);
    c.cflags = (ev != KZ_NONE32 ? KZ_NCTL_FLAG_OBS_AFTER : KZ_NCTL_FLAG_NO_OBS) | (f.deferred ? KZ_NCTL_FLAG_DEFERRED : 0);
    notes->push_back(what + (ev != KZ_NONE32 ? ": observation #" + std::to_string(ev) + " " + n->sites[n->events[ev].site].label : ": no observation event (" + why + ")"));
    recs.push_back({c, 1});
  }
  // the controller's order: the supervisor's records by its time
  std::stable_sort(recs.begin(), recs.end(), [](const Pending& x, const Pending& y) { return std::tie(x.c.t, x.order) < std::tie(y.c.t, y.order); });
  for (size_t i = 0; i < recs.size(); ++i) {
    recs[i].c.ctl_seq = static_cast<uint32_t>(i + 1);
    n->ctl.push_back(recs[i].c);
  }
  if (!h.debugregs_ok) notes->push_back("the hold record says debugregs_ok=no: its control did not run as planned");
  if (!n->ctl.empty()) n->run.r.flags |= KZ_NRUN_FLAG_CONTROL_RECORDED;
  return check_neutral(*n);
}

Status convert_f0_container_with_hold(const Container& run, const std::string& hold_name, const std::string& hold_text, const Digest& tool_sha256, Bytes* out,
                                      NeutralRun* layer, std::vector<std::string>* notes) {
  if (run.header().artifact_kind != KZ_ARTIFACT_RUN) return Status::error(Err::Invalid, "not a run container");
  if (has_neutral_sections(run)) return Status::error(Err::Invalid, "the container already has the neutral layer");
  HoldRecord h;
  Status s = parse_hold_record(hold_text, &h);
  if (!s.ok()) return s;
  NeutralRun n;
  s = neutral_from_f0(run, &n);
  if (s.ok()) s = add_hold_control(h, &n, notes);
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
  s = w.add_parent(KZ_ARTIFACT_RUN, 1, run.artifact_hash());
  if (!s.ok()) return s;
  // every section of the input but BASIS and FILE_HASHES, which get the hold record's items (copied first, so that the
  // strings of the copied sections are interned in the order convert_f0_container interns them)
  s = copy_sections(run, &w, {KZ_SEC_BASIS, KZ_SEC_FILE_HASHES});
  if (!s.ok()) return s;
  std::vector<kz_basis_item> items;
  std::vector<kz_file_hash> files;
  if (s.ok() && run.find_section(KZ_SEC_BASIS) != nullptr) s = run.read_section(KZ_SEC_BASIS, &items);
  if (s.ok() && run.find_section(KZ_SEC_FILE_HASHES) != nullptr) s = run.read_section(KZ_SEC_FILE_HASHES, &files);
  if (!s.ok()) return s;
  for (kz_basis_item& b : items) {
    std::string name, value;
    run.string(b.name, &name);
    run.string(b.value, &value);
    b.name = w.intern(name);
    b.value = w.intern(value);
  }
  for (kz_file_hash& f : files) {
    std::string path;
    run.string(f.path, &path);
    f.path = w.intern(path);
  }
  auto item = [&](const std::string& name, const std::string& value) {
    kz_basis_item b = zeroed<kz_basis_item>();
    b.name = w.intern(name);
    b.value = w.intern(value);
    b.policy = KZ_POLICY_UNSET;
    b.prov = KZ_NONE32;
    b.deviation = KZ_NONE32;
    items.push_back(b);
  };
  const Digest hd = sha256(hold_text);
  item("convert.hold", hold_name);
  item("convert.hold.sha256", to_hex(hd));
  for (const std::string& note : *notes) item("convert.hold.note", note);
  kz_file_hash f = zeroed<kz_file_hash>();
  f.path = w.intern(hold_name);
  std::memcpy(f.sha256, hd.data(), 32);
  f.size = hold_text.size();
  f.prov = KZ_NONE32;
  files.push_back(f);
  s = w.add_section(KZ_SEC_BASIS, items);
  if (s.ok()) s = w.add_section(KZ_SEC_FILE_HASHES, files);
  if (s.ok()) s = add_neutral_sections(n, &w);
  if (s.ok()) s = w.finish(out);
  if (s.ok() && layer != nullptr) *layer = std::move(n);
  return s;
}

}  // namespace kz::trace

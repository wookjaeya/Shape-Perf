// explore.cpp -- the explicit-state search over the actor programs of a predict model (kz/predict/predict.hpp).
#include <algorithm>
#include <cstdlib>
#include <unordered_set>

#include "kz/predict/predict.hpp"

namespace kz::predict {

std::string tuple_text(const Tuple& t) {
  std::string s = t.op;
  if (!t.key.empty()) s += "(" + t.key + ")";
  if (t.has_result) s += " -> " + std::to_string(t.result);
  return s;
}

namespace {

enum : uint8_t { kUnspawned = 0, kActive = 1, kFinished = 2, kExited = 3, kHeld = 4 };

struct ARt {
  uint32_t pc = 0;
  uint8_t st = kUnspawned;
  bool has_last = false, entered = false;
  int64_t last = 0;
  std::vector<int64_t> ctr;
  int32_t vsteps = 0;
  uint32_t tid = 0;
};

struct RRt {
  int32_t state = 0;
  uint32_t gen = 0, id = 0, seq = 0;
  int32_t owner = -1;
};

struct Sys {
  std::vector<ARt> a;
  std::vector<RRt> r;
  std::vector<std::vector<std::pair<int64_t, uint32_t>>> runs;  // per actor * nres + res: (result, count)
  std::vector<uint32_t> before;                                 // per actor * nres + res: uses before the first creation
  std::vector<uint8_t> created;                                 // per res
  uint32_t seq = 0, tasks = 0;
  bool cut = false, torn = false, invalid = false;
  int32_t torn_actor = -1;
  std::vector<uint8_t> gate;  // 0 armed, 1 holding, 2 done
  std::vector<int32_t> holder;
  std::vector<uint8_t> fired;
  std::vector<uint8_t> disarmed;  // per actor: released from a gate; the supervisor cleared its debug registers
};

void put(std::string* k, int64_t v) {
  uint64_t u = static_cast<uint64_t>(v);
  do {
    k->push_back(static_cast<char>((u & 0x7f) | (u > 0x7f ? 0x80 : 0)));
    u >>= 7;
  } while (u != 0);
}

std::string key_of(const Sys& s) {
  std::string k;
  for (const ARt& a : s.a) {
    put(&k, a.pc);
    put(&k, a.st);
    put(&k, a.has_last ? 1 : 0);
    put(&k, a.entered ? 1 : 0);
    put(&k, a.last);
    for (int64_t c : a.ctr) put(&k, c);
    put(&k, a.vsteps);
  }
  for (const RRt& r : s.r) {
    put(&k, r.state);
    put(&k, r.gen);
    put(&k, r.owner);
  }
  for (const auto& rv : s.runs) {
    put(&k, static_cast<int64_t>(rv.size()));
    for (const auto& x : rv) {
      put(&k, x.first);
      put(&k, x.second);
    }
  }
  for (uint32_t b : s.before) put(&k, b);
  for (uint8_t c : s.created) put(&k, c);
  put(&k, (s.cut ? 1 : 0) | (s.torn ? 2 : 0) | (s.invalid ? 4 : 0));
  put(&k, s.torn_actor);
  for (uint8_t g : s.gate) put(&k, g);
  for (int32_t h : s.holder) put(&k, h);
  for (uint8_t f : s.fired) put(&k, f);
  for (uint8_t d : s.disarmed) put(&k, d);
  return k;
}

struct Ex {
  const phi::Profile& p;
  const Model& m;
  const ExploreOptions& o;
  ExploreResult* res;
  size_t nres = 0;
  std::unordered_set<std::string> visited;
  std::vector<Step> path;
  bool stop = false;
  std::vector<int32_t> gate_of_release;  // per plan point: the gate point index it releases (-1: a gate)

  int32_t state_index(int32_t kind, const std::string& st) const {
    const phi::KindDecl& kd = p.kinds[static_cast<size_t>(kind)];
    for (size_t i = 0; i < kd.states.size(); ++i)
      if (kd.states[i] == st) return static_cast<int32_t>(i);
    return -1;
  }
  const std::string& rec_name(int32_t ai) const { return m.actors[static_cast<size_t>(ai)].record; }
  uint32_t object_id(int32_t ri, uint32_t gen) const { return 0x7b000000u | (static_cast<uint32_t>(ri + 1) << 8) | (gen & 0xff); }
};

eval::TraceEvent ev(const std::string& actor, const std::string& site, eval::Phase ph) {
  eval::TraceEvent e;
  e.actor = actor;
  e.site = site;
  e.phase = ph;
  return e;
}

bool hit_matches(const ControlPoint& cp, const HitInst& h) {
  if (cp.site != h.site) return false;
  if (!cp.caller.empty() && cp.caller != h.caller) return false;
  if (!cp.name_text.empty() && (h.name_role != cp.name_role || h.name_text != cp.name_text)) return false;
  return true;
}

std::vector<HitInst> hits_of(const Instr& in, const std::string& key, bool before) {
  std::vector<HitInst> out;
  for (const SiteHit& h : in.hits) {
    if (h.before != before) continue;
    HitInst hi;
    hi.site = h.site;
    hi.before = h.before;
    hi.caller = h.caller;
    hi.name_role = h.name_role;
    if (!h.name_role.empty()) hi.name_text = key;
    out.push_back(hi);
  }
  return out;
}

void fire(Ex& x, Sys* s, const std::vector<HitInst>& hits, int32_t ai) {
  if (x.o.plan == nullptr || s->disarmed[static_cast<size_t>(ai)] != 0) return;
  const std::vector<ControlPoint>& pts = x.o.plan->points;
  for (size_t i = 0; i < pts.size(); ++i) {
    if (!pts[i].release || s->fired[i] != 0) continue;
    bool match = false;
    for (const HitInst& h : hits) match = match || hit_matches(pts[i], h);
    if (!match) continue;
    s->fired[i] = 1;  // the first qualifying hit acts (occurrence first), whether or not the gate holds yet
    const int32_t g = x.gate_of_release[i];
    if (g >= 0 && s->gate[static_cast<size_t>(g)] == 1) {
      const size_t h = static_cast<size_t>(s->holder[static_cast<size_t>(g)]);
      s->a[h].st = kActive;
      s->disarmed[h] = 1;
      s->gate[static_cast<size_t>(g)] = 2;
    }
  }
}

// One transition of actor `ai`: its local instructions, at most one visible instruction (or a hold at a gate),
// and the local instructions after it up to the next visible instruction, guard or end.  False: not enabled.
bool run_step(Ex& x, const Sys& s, int32_t ai, Sys* out, Step* st) {
  if (s.cut || s.invalid) return false;
  if (s.a[static_cast<size_t>(ai)].st != kActive) return false;
  if (s.torn && ai != s.torn_actor) return false;
  Sys ns = s;
  ARt& A = ns.a[static_cast<size_t>(ai)];
  const Actor& act = x.m.actors[static_cast<size_t>(ai)];
  const std::string& rn = x.rec_name(ai);
  st->actor = ai;
  if (!A.entered) {
    A.entered = true;
    if (!act.start) {
      // the child's first event: the entry operation of the spawn rule, with the id its spawn returned
      for (const Actor& other : x.m.actors)
        for (const Instr& in : other.prog)
          if (in.op == Ins::Spawn && in.actor_index == ai && !in.entry_op.empty()) {
            eval::TraceEvent e = ev(rn, in.entry_op, eval::Phase::Call);
            if (!in.entry_param.empty()) e.values["arg." + in.entry_param] = A.tid;
            st->events.push_back(e);
            goto entered;
          }
    entered:;
    }
  }
  {
    bool executed = false;
    uint32_t guard = 0;
    while (true) {
      if (++guard > 100000) {
        x.res->error = "actor " + act.name + ": a loop without a visible step";
        x.stop = true;
        return false;
      }
      if (A.pc >= act.prog.size()) {
        A.st = kFinished;
        break;
      }
      const Instr& in = act.prog[A.pc];
      if (ins_visible(in.op)) {
        if (executed) break;
        // a delete of each live resource: nothing live left is a local skip
        int32_t del = -1;
        if (in.op == Ins::DeleteEach) {
          const int32_t live = x.state_index(in.kind_index, x.p.kinds[static_cast<size_t>(in.kind_index)].initial);
          for (size_t ri = 0; ri < ns.r.size(); ++ri) {
            if (x.m.resources[ri].kind_index != in.kind_index || ns.r[ri].state == live) continue;
            if (del < 0 || ns.r[ri].seq < ns.r[static_cast<size_t>(del)].seq) del = static_cast<int32_t>(ri);
          }
          if (del < 0) {
            ++A.pc;
            continue;
          }
        }
        const int32_t k = A.vsteps;
        for (const OrderConstraint& c : x.o.constraints)
          if (c.to_actor == ai && c.to_index == k && ns.a[static_cast<size_t>(c.from_actor)].vsteps <= c.from_index) return false;
        const Tuple* expect = nullptr;
        if (x.o.follow != nullptr) {
          const std::vector<Tuple>& fl = (*x.o.follow)[static_cast<size_t>(ai)];
          if (static_cast<size_t>(k) >= fl.size()) return false;
          expect = &fl[static_cast<size_t>(k)];
        }
        const int32_t ri = in.res_index >= 0 ? in.res_index : del;
        const std::string key = ri >= 0 ? x.m.resources[static_cast<size_t>(ri)].key_text : std::string();
        // a gate of the plan holds the thread at the step's entry
        if (x.o.plan != nullptr && ns.disarmed[static_cast<size_t>(ai)] == 0) {
          const std::vector<HitInst> bh = hits_of(in, key, true);
          const std::vector<ControlPoint>& pts = x.o.plan->points;
          for (size_t gi = 0; gi < pts.size(); ++gi) {
            if (pts[gi].release || ns.gate[gi] != 0) continue;
            bool match = false;
            for (const HitInst& h : bh) match = match || hit_matches(pts[gi], h);
            if (!match) continue;
            ns.gate[gi] = 1;
            ns.holder[gi] = ai;
            A.st = kHeld;
            st->held = true;
            st->marks.push_back("held at " + pts[gi].label);
            *out = std::move(ns);
            return true;
          }
        }
        std::vector<HitInst> before = hits_of(in, key, true), after = hits_of(in, key, false);
        fire(x, &ns, before, ai);
        Tuple t;
        st->index = k;
        st->res = ri;
        st->pc = static_cast<int32_t>(A.pc);
        st->ins = in.op;
        switch (in.op) {
          case Ins::Clear: {
            const int32_t sti = x.state_index(in.kind_index, in.state);
            for (size_t r = 0; r < ns.r.size(); ++r)
              if (x.m.resources[r].kind_index == in.kind_index) ns.r[r].state = sti;
            st->events.push_back(ev(rn, in.site, eval::Phase::Point));
            t.op = in.site;
            break;
          }
          case Ins::Set: {
            const Resource& rr = x.m.resources[static_cast<size_t>(ri)];
            ns.r[static_cast<size_t>(ri)].state = x.state_index(rr.kind_index, in.state);
            st->events.push_back(ev(rn, in.site, eval::Phase::Point));
            t.op = in.site;
            if (rr.kind_index == x.m.scope_kind) {
              if (in.state == x.m.scope_request) {
                for (size_t aj = 0; aj < ns.a.size(); ++aj)
                  if (x.m.actors[aj].has_instance_use && (ns.a[aj].st == kUnspawned || ns.a[aj].st == kActive || ns.a[aj].st == kHeld)) ns.cut = true;
              } else if (in.state == x.m.scope_teardown) {
                ns.torn = true;
                ns.torn_actor = ai;
              }
            }
            break;
          }
          case Ins::Spawn: {
            ARt& child = ns.a[static_cast<size_t>(in.actor_index)];
            child.st = kActive;
            child.tid = 0x7a000000u | ++ns.tasks;
            st->events.push_back(ev(rn, in.opname, eval::Phase::Call));
            eval::TraceEvent r = ev(rn, in.opname, eval::Phase::Return);
            if (!in.out_param.empty()) r.values["out." + in.out_param] = child.tid;
            st->events.push_back(r);
            t.op = in.opname;
            t.key = x.m.actors[static_cast<size_t>(in.actor_index)].name;
            break;
          }
          case Ins::Use: {
            RRt& rr = ns.r[static_cast<size_t>(ri)];
            const int32_t live = x.state_index(x.m.resources[static_cast<size_t>(ri)].kind_index, in.live_state);
            int64_t result = rr.state == live ? in.ready_code : in.not_ready_code;
            if (x.o.keep_outcomes && expect != nullptr && expect->op == in.opname && expect->key == key && expect->has_result) result = expect->result;
            eval::TraceEvent c = ev(rn, in.opname, eval::Phase::Call);
            c.texts["arg." + in.key_param] = key;
            st->events.push_back(c);
            eval::TraceEvent pt = ev(rn, in.lp_point, eval::Phase::Point);
            pt.texts["arg." + in.key_param] = key;
            st->events.push_back(pt);
            eval::TraceEvent r = ev(rn, in.opname, eval::Phase::Return);
            r.has_result = true;
            r.result = result;
            if (result == in.ready_code && !in.out_param.empty()) r.values["out." + in.out_param] = rr.id != 0 ? rr.id : x.object_id(ri, rr.gen);
            st->events.push_back(r);
            st->events.push_back(ev(rn, in.site, eval::Phase::Point));
            t.op = in.opname;
            t.key = key;
            t.has_result = true;
            t.result = result;
            A.has_last = true;
            A.last = result;
            st->instance = in.instance;
            const size_t hi = static_cast<size_t>(ai) * x.nres + static_cast<size_t>(ri);
            auto& run = ns.runs[hi];
            if (!run.empty() && run.back().first == result) ++run.back().second;
            else run.push_back({result, 1});
            if (ns.created[static_cast<size_t>(ri)] == 0) ++ns.before[hi];
            st->gen = rr.gen;
            break;
          }
          case Ins::Create: {
            RRt& rr = ns.r[static_cast<size_t>(ri)];
            const int32_t live = x.state_index(x.m.resources[static_cast<size_t>(ri)].kind_index, in.live_state);
            int64_t result = in.value;
            eval::TraceEvent c = ev(rn, in.opname, eval::Phase::Call);
            c.texts["arg." + in.key_param] = key;
            st->events.push_back(c);
            if (rr.state != live) {
              rr.state = live;
              ++rr.gen;
              rr.id = x.object_id(ri, rr.gen);
              rr.owner = ai;
              rr.seq = ++ns.seq;
              ns.created[static_cast<size_t>(ri)] = 1;
              result = in.ready_code;
              st->created = true;
              eval::TraceEvent pt = ev(rn, in.lp_point, eval::Phase::Point);
              for (const auto& v : in.lp_values) {
                if (v.second) pt.texts["lp." + v.first] = key;
                else pt.values["lp." + v.first] = in.ready_code;
              }
              st->events.push_back(pt);
            }
            eval::TraceEvent r = ev(rn, in.opname, eval::Phase::Return);
            r.has_result = true;
            r.result = result;
            if (result == in.ready_code && !in.out_param.empty()) r.values["out." + in.out_param] = rr.id;
            st->events.push_back(r);
            st->events.push_back(ev(rn, in.site, eval::Phase::Point));
            t.op = in.opname;
            t.key = key;
            t.has_result = true;
            t.result = result;
            A.has_last = true;
            A.last = result;
            st->gen = rr.gen;
            break;
          }
          case Ins::DeleteEach: {
            RRt& rr = ns.r[static_cast<size_t>(del)];
            rr.state = x.state_index(in.kind_index, in.live_state);
            eval::TraceEvent c = ev(rn, in.opname, eval::Phase::Call);
            if (!in.key_param.empty()) c.values["arg." + in.key_param] = rr.id;
            st->events.push_back(c);
            eval::TraceEvent pt = ev(rn, in.lp_point, eval::Phase::Point);
            for (const auto& v : in.lp_values) {
              if (v.second) pt.texts["lp." + v.first] = key;
              else pt.values["lp." + v.first] = in.ready_code;
            }
            st->events.push_back(pt);
            eval::TraceEvent r = ev(rn, in.opname, eval::Phase::Return);
            r.has_result = true;
            r.result = in.ready_code;
            st->events.push_back(r);
            t.op = in.opname;
            t.key = key;
            t.has_result = true;
            t.result = in.ready_code;
            st->gen = rr.gen;
            break;
          }
          case Ins::Exit: {
            eval::TraceEvent c = ev(rn, in.opname, eval::Phase::Call);
            if (!in.exit_param.empty()) c.values["arg." + in.exit_param] = in.value;
            st->events.push_back(c);
            t.op = in.opname;
            A.st = kExited;
            // the cleanup of the exiting application deletes the objects its task created (the ownership of the
            // model); the model has no step for that cleanup, so an owned live resource cannot be continued
            for (size_t r = 0; r < ns.r.size(); ++r)
              if (ns.r[r].owner == ai && ns.r[r].state != x.state_index(x.m.resources[r].kind_index, x.p.kinds[static_cast<size_t>(x.m.resources[r].kind_index)].initial)) ns.invalid = true;
            break;
          }
          default:
            break;
        }
        if (expect != nullptr && !(t == *expect)) return false;
        st->tuple = t;
        st->n_pre = static_cast<uint32_t>(st->hits.size());
        st->hits.insert(st->hits.end(), before.begin(), before.end());
        st->n_before = static_cast<uint32_t>(st->hits.size());
        fire(x, &ns, after, ai);
        st->hits.insert(st->hits.end(), after.begin(), after.end());
        ++A.vsteps;
        executed = true;
        if (in.op == Ins::Exit) break;
        if (in.op != Ins::DeleteEach) ++A.pc;
        if (ns.cut) break;
        continue;
      }
      switch (in.op) {
        case Ins::Event: {
          eval::Phase ph = in.phase == "call" ? eval::Phase::Call : in.phase == "ret" ? eval::Phase::Return : eval::Phase::Point;
          eval::TraceEvent e = ev(rn, in.site, ph);
          for (const auto& kv : in.values) {
            const std::string& v = kv.second;
            if (!v.empty() && v[0] == '@') {
              e.texts[kv.first] = v.substr(1);
            } else if (!v.empty() && (v[0] == '-' || (v[0] >= '0' && v[0] <= '9'))) {
              const int64_t iv = std::strtoll(v.c_str(), nullptr, 0);
              if (kv.first == "result") {
                e.has_result = true;
                e.result = iv;
              } else {
                e.values[kv.first] = iv;
              }
            } else {
              auto it = x.m.consts.find(v);
              const int64_t iv = it != x.m.consts.end() ? it->second : 0;
              if (kv.first == "result") {
                e.has_result = true;
                e.result = iv;
              } else {
                e.values[kv.first] = iv;
              }
            }
          }
          st->events.push_back(e);
          {
            const std::vector<HitInst> eh = hits_of(in, std::string(), false);
            fire(x, &ns, eh, ai);
            st->hits.insert(st->hits.end(), eh.begin(), eh.end());
          }
          ++A.pc;
          break;
        }
        case Ins::Mark:
          st->marks.push_back(in.text);
          ++A.pc;
          break;
        case Ins::Label:
          ++A.pc;
          break;
        case Ins::Goto:
          A.pc = static_cast<uint32_t>(in.target);
          break;
        case Ins::IfResult:
          A.pc = (A.has_last && A.last == in.value) ? static_cast<uint32_t>(in.target) : A.pc + 1;
          break;
        case Ins::UnlessResult:
          A.pc = !(A.has_last && A.last == in.value) ? static_cast<uint32_t>(in.target) : A.pc + 1;
          break;
        case Ins::CounterReset:
          A.ctr[static_cast<size_t>(in.counter_index)] = 0;
          ++A.pc;
          break;
        case Ins::CounterInc:
          ++A.ctr[static_cast<size_t>(in.counter_index)];
          ++A.pc;
          break;
        case Ins::IfBelow:
          A.pc = A.ctr[static_cast<size_t>(in.counter_index)] < in.value ? static_cast<uint32_t>(in.target) : A.pc + 1;
          break;
        case Ins::Await: {
          if (executed) goto done;
          const Resource& rr = x.m.resources[static_cast<size_t>(in.res_index)];
          if (ns.r[static_cast<size_t>(in.res_index)].state != x.state_index(rr.kind_index, in.state)) return false;
          ++A.pc;
          break;
        }
        case Ins::Finish: {
          const std::vector<HitInst> h1 = hits_of(in, std::string(), true), h2 = hits_of(in, std::string(), false);
          fire(x, &ns, h1, ai);
          fire(x, &ns, h2, ai);
          if (!executed) {
            st->pc = static_cast<int32_t>(A.pc);
            st->ins = Ins::Finish;
          }
          st->hits.insert(st->hits.end(), h1.begin(), h1.end());
          st->hits.insert(st->hits.end(), h2.begin(), h2.end());
          A.st = kFinished;
          goto done;
        }
        default:
          ++A.pc;
          break;
      }
    }
  done:;
  }
  *out = std::move(ns);
  return true;
}

char end_char(uint8_t st) {
  switch (st) {
    case kUnspawned: return 'u';
    case kActive: return 'a';
    case kFinished: return 'f';
    case kExited: return 'x';
    case kHeld: return 'h';
  }
  return '?';
}

void terminal(Ex& x, const Sys& s) {
  ++x.res->terminals;
  if (x.o.follow != nullptr)
    for (size_t ai = 0; ai < s.a.size(); ++ai)
      if (static_cast<size_t>(s.a[ai].vsteps) != (*x.o.follow)[ai].size()) return;  // the followed record is not complete
  ClassInfo ci;
  std::vector<uint32_t> counts;
  for (size_t ai = 0; ai < s.a.size(); ++ai)
    for (size_t ri = 0; ri < x.nres; ++ri) {
      if (!x.m.actors[ai].has_instance_use) continue;  // the class is the history of the instances' actors
      const auto& run = s.runs[ai * x.nres + ri];
      if (run.empty()) continue;
      std::string seg = x.m.actors[ai].name + "/" + x.m.resources[ri].name + ":";
      for (const auto& r : run) {
        seg += " " + std::to_string(r.first) + "x" + (r.second == 1 ? "1" : "n");
        counts.push_back(r.second);
      }
      const uint32_t b = s.before[ai * x.nres + ri];
      seg += std::string(" | uses before creation ") + (b == 0 ? "0" : b == 1 ? "1" : "n");
      counts.push_back(b);
      ci.segments.push_back(seg);
    }
  std::string ends = "ends:";
  bool held = false;
  for (size_t ai = 0; ai < s.a.size(); ++ai) {
    if (x.m.actors[ai].has_instance_use) ends += " " + x.m.actors[ai].name + "=" + end_char(s.a[ai].st);
    held = held || s.a[ai].st == kHeld;
  }
  ci.segments.push_back(ends);
  if (x.o.plan != nullptr) {
    std::string ps = "plan:";
    for (size_t g = 0; g < x.o.plan->points.size(); ++g)
      if (!x.o.plan->points[g].release) ps += " " + x.o.plan->points[g].label + "=" + (s.gate[g] == 0 ? "unreached" : s.gate[g] == 1 ? "holding" : "released");
    ci.segments.push_back(ps);
  }
  if (s.cut) ci.segments.push_back("cut by the shutdown request");
  if (held) ci.segments.push_back("stall: held, never released");
  if (s.invalid) ci.segments.push_back("not modelled: an owned resource at exit");
  for (size_t i = 0; i < ci.segments.size(); ++i) ci.key += (i ? "; " : "") + ci.segments[i];
  ci.cut = s.cut;
  ci.stall = held;
  ci.invalid = s.invalid;
  for (ClassInfo& c : x.res->classes)
    if (c.key == ci.key) {
      ++c.terminals;
      for (size_t i = 0; i < counts.size() && i < c.min_counts.size(); ++i) {
        c.min_counts[i] = std::min(c.min_counts[i], counts[i]);
        c.max_counts[i] = std::max(c.max_counts[i], counts[i]);
      }
      if (counts < c.first_counts) {
        c.first = x.path;
        c.first_counts = counts;
      }
      if (counts > c.extreme_counts) {
        c.extreme = x.path;
        c.extreme_counts = counts;
      }
      return;
    }
  ci.terminals = 1;
  ci.first = x.path;
  ci.extreme = x.path;
  ci.min_counts = counts;
  ci.max_counts = counts;
  ci.extreme_counts = counts;
  ci.first_counts = counts;
  x.res->classes.push_back(std::move(ci));
}

void dfs(Ex& x, const Sys& s) {
  if (x.stop) return;
  if (!x.visited.insert(key_of(s)).second) return;
  ++x.res->states;
  if (x.o.max_states != 0 && x.visited.size() >= x.o.max_states) {
    x.res->cap_reached = true;
    x.stop = true;
  }
  bool any = false;
  for (int32_t ai = 0; ai < static_cast<int32_t>(s.a.size()) && !x.stop; ++ai) {
    Sys ns;
    Step st;
    if (!run_step(x, s, ai, &ns, &st)) continue;
    any = true;
    ++x.res->transitions;
    x.path.push_back(std::move(st));
    dfs(x, ns);
    x.path.pop_back();
  }
  if (!any && !x.stop) terminal(x, s);
}

}  // namespace

Status explore(const phi::Profile& p, const Model& m, const ExploreOptions& o, ExploreResult* out) {
  *out = ExploreResult();
  if (!m.resolved) return Status::error(Err::Invalid, "the model is not resolved");
  if (o.follow != nullptr && o.follow->size() != m.actors.size()) return Status::error(Err::Invalid, "follow: one tuple list per actor is needed");
  Ex x{p, m, o, out, m.resources.size(), {}, {}, false, {}};
  Sys s;
  s.a.resize(m.actors.size());
  for (size_t i = 0; i < m.actors.size(); ++i) {
    s.a[i].st = m.actors[i].start ? kActive : kUnspawned;
    s.a[i].ctr.assign(m.actors[i].counters.size(), 0);
  }
  s.r.resize(m.resources.size());
  for (size_t i = 0; i < m.resources.size(); ++i) {
    const phi::KindDecl& kd = p.kinds[static_cast<size_t>(m.resources[i].kind_index)];
    s.r[i].state = x.state_index(m.resources[i].kind_index, kd.initial);
  }
  s.runs.resize(m.actors.size() * m.resources.size());
  s.before.assign(m.actors.size() * m.resources.size(), 0);
  s.created.assign(m.resources.size(), 0);
  s.disarmed.assign(m.actors.size(), 0);
  if (o.plan != nullptr) {
    const size_t n = o.plan->points.size();
    s.gate.assign(n, 0);
    s.holder.assign(n, -1);
    s.fired.assign(n, 0);
    x.gate_of_release.assign(n, -1);
    for (size_t i = 0; i < n; ++i) {
      if (!o.plan->points[i].release) continue;
      for (size_t g = 0; g < n; ++g)
        if (!o.plan->points[g].release && o.plan->points[g].label == o.plan->points[i].releases) x.gate_of_release[i] = static_cast<int32_t>(g);
      if (x.gate_of_release[i] < 0) return Status::error(Err::Invalid, "plan: release " + o.plan->points[i].label + " names no gate");
    }
    for (size_t g = 0; g < n; ++g) s.gate[g] = o.plan->points[g].release ? 2 : 0;
  }
  dfs(x, s);
  if (!out->error.empty()) return Status::error(Err::Invalid, out->error);
  return Status::success();
}

eval::Trace execution_trace(const Model& m, const std::vector<Step>& steps, const std::map<std::string, int64_t>& ref_consts,
                            const std::map<std::string, std::vector<int64_t>>& owners) {
  eval::Trace t;
  t.record = eval::RecordKind::Model;
  t.ref_consts = ref_consts;
  for (const Actor& a : m.actors) {
    auto it = owners.find(a.record);
    if (it != owners.end()) t.owners[a.record] = it->second;
  }
  for (const Step& s : steps)
    for (const eval::TraceEvent& e : s.events) t.add(e);
  return t;
}

}  // namespace kz::predict

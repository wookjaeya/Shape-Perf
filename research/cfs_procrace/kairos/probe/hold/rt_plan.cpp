// rt_plan.cpp -- see rt_plan.hpp.
#include "rt_plan.hpp"

#include <cstring>
#include <map>
#include <set>

#include "kz/core/crc32.hpp"
#include "kz/core/sha256.hpp"
#include "kz_rt_format.h"

namespace kz::hold {

namespace {

bool usable(const kzrt::Bind& b) { return b.status == "bound" || b.status == "inlined" || b.status == "duplicated"; }

bool entry_site(const kzrt::Site& s) { return s.phase == "before" && s.callee.empty() && s.place.empty(); }

Status refuse(const std::string& point, const std::string& why) { return Status::error(Err::Invalid, "rtplan: point " + point + ": " + why); }

void put32(std::string* b, size_t off, uint32_t v) {
  for (int i = 0; i < 4; ++i) (*b)[off + i] = static_cast<char>((v >> (8 * i)) & 0xFF);
}
void put64(std::string* b, size_t off, uint64_t v) {
  put32(b, off, static_cast<uint32_t>(v));
  put32(b, off + 4, static_cast<uint32_t>(v >> 32));
}

std::string hex(const uint8_t* p) { return kz::to_hex(p, 32); }

std::string occ_text(uint32_t o) { return o == 0 ? "each" : o == 1 ? "first" : std::to_string(o); }

}  // namespace

Status convert_rt_plan(const HoldPlan& plan, std::string_view plan_text, const kzrt::Manifest& m, std::string_view manifest_text,
                       std::string_view binds_text, RtPlan* out) {
  *out = RtPlan();
  {
    const Digest a = sha256(manifest_text), b = sha256(plan_text), c = sha256(binds_text);
    std::memcpy(out->manifest_sha, a.data(), 32);
    std::memcpy(out->source_sha, b.data(), 32);
    std::memcpy(out->binds_sha, c.data(), 32);
  }
  // the modules this build has bindings in
  std::set<uint32_t> built;
  for (const kzrt::Bind& b : m.binds)
    if (usable(b)) built.insert(b.module);
  for (uint32_t id : built) {
    const kzrt::Module* md = m.module(id);
    out->modules.push_back(std::to_string(id) + "=" + (md != nullptr ? md->name : "?"));
  }
  if (built.empty()) return Status::error(Err::Invalid, "rtplan: the binding list has no usable binding (bound, inlined or duplicated)");

  // gates first, then releases, each in plan order: the runtime evaluates the gates of a hit in index order, then
  // its releases (kzp-hold 1 H3), so only the order among gates and among releases matters, as in F0
  struct Decl {
    bool gate;
    size_t index;
  };
  std::vector<Decl> order;
  for (size_t i = 0; i < plan.gates.size(); ++i) order.push_back(Decl{true, i});
  for (size_t i = 0; i < plan.releases.size(); ++i) order.push_back(Decl{false, i});
  if (order.size() > KZ_RT_MAX_POINTS) return Status::error(Err::Invalid, "rtplan: more than " + std::to_string(KZ_RT_MAX_POINTS) + " points");
  std::map<std::string, uint32_t> gate_index;
  for (size_t i = 0; i < order.size(); ++i)
    if (order[i].gate) gate_index[plan.gates[order[i].index].label] = static_cast<uint32_t>(i);

  bool any_caller = false, any_name = false, any_gate_ctx = false, any_each = false;
  for (size_t i = 0; i < order.size(); ++i) {
    const bool gate = order[i].gate;
    const std::string& label = gate ? plan.gates[order[i].index].label : plan.releases[order[i].index].label;
    const std::string& site_label = gate ? plan.gates[order[i].index].site_label : plan.releases[order[i].index].site_label;
    const HoldCond& cond = gate ? plan.gates[order[i].index].cond : plan.releases[order[i].index].cond;
    const bool each = gate ? plan.gates[order[i].index].occurrence_each : plan.releases[order[i].index].occurrence_each;
    const uint32_t nth = gate ? plan.gates[order[i].index].occurrence_nth : plan.releases[order[i].index].occurrence_nth;
    RtPoint rp;
    rp.name = label;
    rp.plan_site = site_label;
    if (label.size() >= KZ_RT_PLAN_NAME_BYTES) return refuse(label, "the label is longer than " + std::to_string(KZ_RT_PLAN_NAME_BYTES - 1) + " bytes");
    const kzrt::Site* s0 = m.site_by_label(site_label);
    if (s0 == nullptr) return refuse(label, "site " + site_label + " is not in the manifest");
    const kzrt::Site* t = s0;
    std::vector<uint32_t> binds;
    std::string map = site_label;
    if (cond.has_caller_in) {
      any_caller = true;
      uint32_t mod = 0;
      for (uint32_t id : built) {
        const kzrt::Module* md = m.module(id);
        if (md != nullptr && md->name == cond.caller_binary) mod = id;
      }
      if (mod == 0) return refuse(label, "caller-in " + cond.caller_binary + ": no module of that name has bindings in this build");
      if (!entry_site(*s0)) return refuse(label, "caller-in applies to a function entry site (F0 reads the return address there); " + site_label + " is not one");
      std::vector<const kzrt::Site*> cands;
      for (const kzrt::Site& s : m.sites) {
        if (s.callee != s0->func || s.place != "at-call") continue;
        bool in_mod = false;
        for (const kzrt::Bind& b : m.binds)
          if (b.site == s.id && b.module == mod && usable(b)) in_mod = true;
        if (in_mod) cands.push_back(&s);
      }
      if (cands.empty()) return refuse(label, "caller-in " + cond.caller_binary + ": no call-side site of " + s0->func + " (callee=" + s0->func + " place=at-call) bound in module " + std::to_string(mod));
      if (cands.size() > 1) return refuse(label, "caller-in " + cond.caller_binary + ": " + std::to_string(cands.size()) + " call-side sites of " + s0->func + " (one counter over several sites is not expressible in kzrt-plan 1)");
      t = cands[0];
      for (const kzrt::Bind& b : m.binds)
        if (b.site == t->id && b.module == mod && usable(b)) binds.push_back(b.id);
      if (binds.size() > KZ_RT_POINT_BINDS) return refuse(label, "more than " + std::to_string(KZ_RT_POINT_BINDS) + " call-side bindings");
      map += " caller-in " + cond.caller_binary + " -> site " + std::to_string(t->id) + " (" + t->label + ", right before the call of " + s0->func + ") in module " + std::to_string(mod);
    } else {
      bool any = false;
      for (const kzrt::Bind& b : m.binds) any = any || (b.site == s0->id && usable(b));
      if (!any) return refuse(label, "site " + site_label + " has no usable binding in this build");
      map += " -> site " + std::to_string(t->id) + " (every binding)";
    }
    if (!t->has_control) return refuse(label, "site " + std::to_string(t->id) + " (" + t->label + ") has no control line: kz_rt_ctl is not called there");
    rp.site = t;
    kz_rt_point& p = rp.p;
    std::memset(&p, 0, sizeof p);
    p.kind = gate ? KZ_RT_POINT_GATE : KZ_RT_POINT_RELEASE;
    p.label = static_cast<uint32_t>(i + 1);
    p.site = t->id;
    p.occurrence = each ? 0u : nth;
    p.bind_n = static_cast<uint32_t>(binds.size());
    for (size_t j = 0; j < binds.size(); ++j) p.bind[j] = binds[j];
    p.str_slot = -1;
    p.val_slot = -1;
    if (each) any_each = any_each || gate;
    if (gate && plan.gates[order[i].index].unless_released) p.flags |= KZ_RT_PT_UNLESS_RELEASED;
    if (!gate) {
      const std::string& g = plan.releases[order[i].index].releases;
      const auto gi = gate_index.find(g);
      if (gi == gate_index.end()) return refuse(label, "releases unknown gate " + g);
      p.releases = gi->second;
    }
    if (cond.has_name) {
      any_name = true;
      const kzrt::Value* v = nullptr;
      for (const kzrt::Value& x : t->values)
        if (x.role == cond.name_role) v = &x;
      if (v == nullptr) return refuse(label, "name " + cond.name_role + ": site " + std::to_string(t->id) + " has no value of that role");
      if (v->string_mode != "validated") return refuse(label, "name " + cond.name_role + ": the slot is not string=validated at site " + std::to_string(t->id) + " (no copy is made there)");
      if (cond.name_text.size() >= v->bound || cond.name_text.size() >= KZ_RT_STR_MAX)
        return refuse(label, "name " + cond.name_role + ": the text is not shorter than the slot's bound " + std::to_string(v->bound) + " (equality would not be decidable from the copy)");
      p.str_slot = static_cast<int32_t>(v->slot);
      p.str_len = static_cast<uint32_t>(cond.name_text.size());
      std::memcpy(p.str, cond.name_text.data(), cond.name_text.size());
      map += "; name " + cond.name_role + " \"" + cond.name_text + "\" -> slot " + std::to_string(v->slot) + " (validated, bound " + std::to_string(v->bound) + ")";
    }
    map += "; occurrence " + occ_text(p.occurrence);
    if ((p.flags & KZ_RT_PT_UNLESS_RELEASED) != 0) map += "; unless-released";
    map += "; ctx=" + t->ctx;
    if (gate && kzrt::ctx_code(t->ctx) != KZ_RT_CTX_TASK) {
      any_gate_ctx = true;
      map += " (not task: the runtime holds only where the point allows it and records gate-refused otherwise)";
    }
    rp.mapping = map;
    out->points.push_back(std::move(rp));
  }
  // the differences from F0 (kz hold) that apply (event_semantics.md section 4 and 11)
  if (any_caller)
    out->differences.push_back(
        "position: a caller-in point on a function entry is evaluated at the caller's call-side binding right before the call "
        "(F0: the callee's first instruction, the return address in the module); the call instruction itself is between them");
  if (any_name)
    out->differences.push_back(
        "name: compared with the runtime's bounded copy at a string=validated slot (F0: the string read from the stopped thread's memory)");
  out->differences.push_back(
      "release: evaluated by kz_rt_ctl after the release site's observation record in the releasing thread (F0: the debug trap before "
      "the instruction); a held thread's gate observation record is written before it waits");
  if (any_gate_ctx) out->differences.push_back("context: a gate at a context class other than task is refused and recorded (F0 does not check contexts)");
  if (any_each)
    out->differences.push_back("each gate: a hit while the gate holds another thread is recorded gate-busy and not held (F0 overwrote the held thread)");
  out->differences.push_back("timeout: none (timeout_ns 0, as F0's holder; the harness safeguard is the run's)");
  return Status::success();
}

std::string encode_rt_plan(const RtPlan& plan) {
  const size_t n = plan.points.size();
  const size_t total = KZ_RT_PLAN_HEADER_BYTES + n * KZ_RT_PLAN_POINT_BYTES + KZ_RT_PLAN_TRAILER_BYTES;
  std::string b(total, '\0');
  std::memcpy(&b[0], KZ_RT_PLAN_MAGIC, 8);
  b[8] = static_cast<char>(KZ_RT_PLAN_MAJOR);
  b[10] = static_cast<char>(KZ_RT_PLAN_MINOR);
  put32(&b, 12, static_cast<uint32_t>(n));
  put32(&b, 16, static_cast<uint32_t>(total));
  put32(&b, 20, 0);
  std::memcpy(&b[24], plan.manifest_sha, 32);
  std::memcpy(&b[56], plan.source_sha, 32);
  std::memcpy(&b[88], plan.binds_sha, 32);
  for (size_t i = 0; i < n; ++i) {
    const size_t q = KZ_RT_PLAN_HEADER_BYTES + i * KZ_RT_PLAN_POINT_BYTES;
    const kz_rt_point& p = plan.points[i].p;
    put32(&b, q, p.kind);
    put32(&b, q + 4, p.label);
    put32(&b, q + 8, p.site);
    put32(&b, q + 12, p.releases);
    put32(&b, q + 16, p.occurrence);
    put32(&b, q + 20, p.flags);
    put32(&b, q + 24, p.bind_n);
    for (uint32_t j = 0; j < KZ_RT_POINT_BINDS; ++j) put32(&b, q + 28 + 4 * j, p.bind[j]);
    put32(&b, q + 60, static_cast<uint32_t>(p.str_slot));
    put32(&b, q + 64, p.str_len);
    std::memcpy(&b[q + 68], p.str, KZ_RT_STR_MAX);
    put32(&b, q + 132, static_cast<uint32_t>(p.val_slot));
    put32(&b, q + 136, 0);
    put64(&b, q + 140, p.val);
    put64(&b, q + 148, p.timeout_ns);
    std::memcpy(&b[q + 156], plan.points[i].name.data(), std::min<size_t>(plan.points[i].name.size(), KZ_RT_PLAN_NAME_BYTES - 1));
  }
  put32(&b, total - 4, kz::crc32(b.data(), total - 4));
  return b;
}

std::string rt_plan_record(const RtPlan& plan) {
  std::string r;
  r += "format kzrt-plan " + std::to_string(KZ_RT_PLAN_MAJOR) + "." + std::to_string(KZ_RT_PLAN_MINOR) + " (semantics kzp-hold 1)\n";
  r += "manifest_sha256 " + hex(plan.manifest_sha) + "\n";
  r += "source_sha256 " + hex(plan.source_sha) + "\n";
  r += "binds_sha256 " + hex(plan.binds_sha) + "\n";
  r += "modules";
  for (const std::string& m : plan.modules) r += " " + m;
  r += "\n";
  for (size_t i = 0; i < plan.points.size(); ++i) {
    const RtPoint& p = plan.points[i];
    std::string binds;
    for (uint32_t j = 0; j < p.p.bind_n; ++j) binds += (j ? "," : "") + std::to_string(p.p.bind[j]);
    r += "point " + std::to_string(i) + " " + (p.p.kind == KZ_RT_POINT_GATE ? "gate " : "release ") + p.name +
         (p.p.kind == KZ_RT_POINT_RELEASE ? " releases=" + std::to_string(p.p.releases) : std::string()) + " site=" + std::to_string(p.p.site) +
         " binds=" + (binds.empty() ? "any" : binds) + ": " + p.mapping + "\n";
  }
  for (const std::string& d : plan.differences) r += "difference " + d + "\n";
  return r;
}

}  // namespace kz::hold

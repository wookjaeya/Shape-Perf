// The host converter of the cooperative gate (probe/hold/rt_plan.cpp, `kz rtplan`): a kzp-hold 1 plan with F0 site
// labels against a small manifest of the C1 shape (callee entries, the callers' call-side sites in two modules,
// control lines) and its binding list; the points, their mapping (caller-in -> call-side bindings, name -> validated
// slot), the bytes of kzrt-plan 1, and every refusal.  Unit-test values only.
#include <cstring>
#include <string>

#include "hold_plan.hpp"
#include "kz/core/crc32.hpp"
#include "kz_rt_format.h"
#include "rt_plan.hpp"
#include "support/kz_test.hpp"

namespace {

const char* kManifest =
    "kzs-sites 2\n"
    "source unit rev=0000000000000000000000000000000000000000\n"
    "module 1 name=core source=unit\n"
    "module 2 name=cf.so source=unit\n"
    "module 3 name=bp.so source=unit\n"
    "site 10 label=osal.lookup.entry op=lookup phase=before tu=core.c func=Lookup\n"
    "value 10 1 role=name_ptr from=arg1 type=ptr\n"
    "site 14 label=osal.create.entry op=create phase=before tu=core.c func=Create\n"
    "value 14 1 role=name_ptr from=arg1 type=ptr\n"
    "site 9 label=exit.entry op=exit phase=before tu=core.c func=ExitApp\n"
    "site 34 label=cf_lookup_call.before op=lookup phase=point tu=cf.c func=CfInit callee=Lookup place=at-call\n"
    "value 34 1 role=sem_name from=arg1 type=ptr string=validated bound=20 ref=\"char[20]\"\n"
    "control 34 ctx=task\n"
    "site 35 label=cf_lookup_call op=lookup phase=point tu=cf.c func=CfInit callee=Lookup place=after-call\n"
    "value 35 0 role=status from=ret type=s32\n"
    "control 35 ctx=task\n"
    "site 36 label=cf_retry_delay op=delay phase=point tu=cf.c func=CfInit callee=Delay place=after-call\n"
    "site 37 label=cf_exit_call.before op=exit phase=point tu=cf.c func=CfMain callee=ExitApp place=at-call\n"
    "control 37 ctx=terminating\n"
    "site 44 label=bp_create_call.before op=create phase=point tu=bp.c func=BpInit callee=Create place=at-call\n"
    "value 44 1 role=sem_name from=arg1 type=ptr string=validated bound=20 ref=\"char[20]\"\n"
    "value 44 2 role=initial from=arg2 type=u32\n"
    "control 44 ctx=task\n"
    "site 45 label=bp_create_call op=create phase=point tu=bp.c func=BpInit callee=Create place=after-call\n"
    "control 45 ctx=task\n"
    "site 46 label=bp_lookup_call.before op=lookup phase=point tu=bp.c func=BpInit callee=Lookup place=at-call\n"
    "value 46 1 role=sem_name from=arg1 type=ptr string=validated bound=20 ref=\"char[20]\"\n"
    "control 46 ctx=task\n"
    "site 47 label=bp_lookup2_call.before op=lookup phase=point tu=bp.c func=BpInit2 callee=Lookup place=at-call\n"
    "control 47 ctx=task\n";

const char* kBinds =
    "kzs-sites 1\n"
    "bind 65696 site=10 backend=gcc-plugin module=1 at=\"x\" status=bound\n"
    "bind 65760 site=14 backend=gcc-plugin module=1 at=\"x\" status=bound\n"
    "bind 65680 site=9 backend=gcc-plugin module=1 at=\"x\" status=bound\n"
    "bind 131616 site=34 backend=gcc-plugin module=2 at=\"x\" status=bound\n"
    "bind 131632 site=35 backend=gcc-plugin module=2 at=\"x\" status=bound\n"
    "bind 131648 site=36 backend=gcc-plugin module=2 at=\"x\" status=bound\n"
    "bind 131664 site=37 backend=gcc-plugin module=2 at=\"x\" status=bound\n"
    "bind 197312 site=44 backend=gcc-plugin module=3 at=\"x\" status=inlined\n"
    "bind 197313 site=44 backend=gcc-plugin module=3 at=\"x\" status=duplicated\n"
    "bind 197328 site=45 backend=gcc-plugin module=3 at=\"x\" status=inlined\n"
    "bind 197344 site=46 backend=gcc-plugin module=3 at=\"x\" status=bound\n"
    "bind 197360 site=47 backend=gcc-plugin module=3 at=\"x\" status=bound\n";

kz::Status convert(const std::string& body, kz::hold::RtPlan* out) {
  const std::string text = "kzp-hold 1\n" + body;
  kz::hold::HoldPlan plan;
  kz::Status s = kz::hold::parse_hold_plan(text, &plan);
  if (!s.ok()) return s;
  kzrt::Manifest m;
  std::string err;
  if (!kzrt::parse_manifest(kManifest, "m", &m, &err) || !kzrt::parse_manifest(kBinds, "b", &m, &err)) return kz::Status::error(kz::Err::Invalid, err);
  return kz::hold::convert_rt_plan(plan, text, m, kManifest, kBinds, out);
}

uint32_t le32(const std::string& b, size_t o) {
  return static_cast<uint32_t>(static_cast<uint8_t>(b[o])) | (static_cast<uint32_t>(static_cast<uint8_t>(b[o + 1])) << 8) |
         (static_cast<uint32_t>(static_cast<uint8_t>(b[o + 2])) << 16) | (static_cast<uint32_t>(static_cast<uint8_t>(b[o + 3])) << 24);
}

bool refused(const std::string& body, const std::string& why) {
  kz::hold::RtPlan p;
  const kz::Status s = convert(body, &p);
  if (s.ok()) return false;
  if (s.message.find(why) == std::string::npos) {
    std::fprintf(stderr, "refusal without '%s': %s\n", why.c_str(), s.message.c_str());
    return false;
  }
  return true;
}

// The W1/W5 shapes: caller-in entries, a name, the release at the callers' sites, occurrence n, unless-released.
void test_mapping() {
  kz::hold::RtPlan p;
  const kz::Status s = convert(
      "gate G1_BP site=osal.create.entry when name sem_name \"BP0_tsem\" when caller-in bp.so occurrence first unless-released\n"
      "release R1 site=exit.entry releases G1_BP when caller-in cf.so occurrence first\n"
      "gate G3_CF site=osal.lookup.entry when name sem_name \"BP0_tsem\" when caller-in cf.so occurrence 3 unless-released\n"
      "release R3 site=bp_create_call releases G3_CF occurrence 2\n",
      &p);
  KZ_CHECK(s.ok());
  if (!s.ok()) {
    std::fprintf(stderr, "%s\n", s.message.c_str());
    return;
  }
  KZ_CHECK_EQ(p.points.size(), size_t(4));
  // gates first, then releases, each in plan order
  KZ_CHECK_EQ(p.points[0].name, std::string("G1_BP"));
  KZ_CHECK_EQ(p.points[1].name, std::string("G3_CF"));
  KZ_CHECK_EQ(p.points[2].name, std::string("R1"));
  KZ_CHECK_EQ(p.points[3].name, std::string("R3"));
  const kz_rt_point& g1 = p.points[0].p;
  KZ_CHECK(g1.kind == KZ_RT_POINT_GATE && g1.site == 44 && g1.occurrence == 1 && (g1.flags & KZ_RT_PT_UNLESS_RELEASED) != 0);
  KZ_CHECK(g1.bind_n == 2 && g1.bind[0] == 197312 && g1.bind[1] == 197313);  // inlined and duplicated copies of bp.so
  KZ_CHECK(g1.str_slot == 1 && g1.str_len == 8 && std::memcmp(g1.str, "BP0_tsem", 8) == 0);
  const kz_rt_point& g3 = p.points[1].p;
  KZ_CHECK(g3.site == 34 && g3.bind_n == 1 && g3.bind[0] == 131616 && g3.occurrence == 3);
  const kz_rt_point& r1 = p.points[2].p;
  KZ_CHECK(r1.kind == KZ_RT_POINT_RELEASE && r1.site == 37 && r1.releases == 0 && r1.bind_n == 1 && r1.bind[0] == 131664 && r1.str_slot == -1);
  const kz_rt_point& r3 = p.points[3].p;
  KZ_CHECK(r3.site == 45 && r3.releases == 1 && r3.bind_n == 0 && r3.occurrence == 2);
  KZ_CHECK(p.points[0].mapping.find("caller-in bp.so -> site 44") != std::string::npos);
  KZ_CHECK(p.points[2].mapping.find("ctx=terminating") != std::string::npos);
  // the differences that apply are listed
  bool pos = false, name = false;
  for (const std::string& d : p.differences) {
    pos = pos || d.rfind("position:", 0) == 0;
    name = name || d.rfind("name:", 0) == 0;
  }
  KZ_CHECK(pos && name);
  // the bytes
  const std::string b = kz::hold::encode_rt_plan(p);
  KZ_CHECK_EQ(b.size(), size_t(KZ_RT_PLAN_HEADER_BYTES + 4 * KZ_RT_PLAN_POINT_BYTES + KZ_RT_PLAN_TRAILER_BYTES));
  KZ_CHECK(b.compare(0, 8, KZ_RT_PLAN_MAGIC) == 0);
  KZ_CHECK_EQ(le32(b, 12), 4u);
  KZ_CHECK_EQ(le32(b, 16), static_cast<uint32_t>(b.size()));
  KZ_CHECK_EQ(le32(b, b.size() - 4), kz::crc32(b.data(), b.size() - 4));
  const size_t q0 = KZ_RT_PLAN_HEADER_BYTES;
  KZ_CHECK_EQ(le32(b, q0 + 8), 44u);       // site
  KZ_CHECK_EQ(le32(b, q0 + 24), 2u);       // bind_n
  KZ_CHECK_EQ(le32(b, q0 + 28), 197312u);  // bind[0]
  KZ_CHECK_EQ(le32(b, q0 + 60), 1u);       // str_slot
  KZ_CHECK(b.compare(q0 + 68, 8, "BP0_tsem") == 0);
  KZ_CHECK(b.compare(q0 + 156, 5, "G1_BP") == 0);
  const size_t q2 = KZ_RT_PLAN_HEADER_BYTES + 2 * KZ_RT_PLAN_POINT_BYTES;
  KZ_CHECK_EQ(le32(b, q2), static_cast<uint32_t>(KZ_RT_POINT_RELEASE));
  KZ_CHECK_EQ(le32(b, q2 + 12), 0u);  // releases gate 0
  KZ_CHECK(kz::hold::rt_plan_record(p).find("point 0 gate G1_BP site=44 binds=197312,197313") != std::string::npos);
  // each -> occurrence 0
  KZ_CHECK(convert("gate G site=cf_lookup_call occurrence each\nrelease R site=bp_create_call releases G\n", &p).ok());
  KZ_CHECK(p.points.size() == 2 && p.points[0].p.occurrence == 0);
}

void test_refusals() {
  KZ_CHECK(refused("gate G site=nope\n", "site nope is not in the manifest"));
  KZ_CHECK(refused("gate G site=osal.create.entry when caller-in nothing.so\n", "no module of that name"));
  KZ_CHECK(refused("gate G site=bp_create_call when caller-in bp.so\n", "caller-in applies to a function entry site"));
  KZ_CHECK(refused("gate G site=exit.entry when caller-in bp.so\n", "no call-side site of ExitApp"));
  KZ_CHECK(refused("gate G site=osal.lookup.entry when caller-in bp.so\n", "2 call-side sites of Lookup"));
  KZ_CHECK(refused("gate G site=osal.create.entry\n", "has no control line"));
  KZ_CHECK(refused("gate G site=cf_retry_delay\n", "has no control line"));
  KZ_CHECK(refused("gate G site=cf_lookup_call when name sem_name \"X\"\n", "has no value of that role"));
  KZ_CHECK(refused("gate G site=bp_create_call.before when name initial \"X\"\n", "not string=validated"));
  KZ_CHECK(refused("gate G site=osal.create.entry when name sem_name \"ABCDEFGHIJKLMNOPQRST\" when caller-in bp.so\n", "not shorter than the slot's bound"));
  KZ_CHECK(refused("gate G_LABEL_LONGER_15 site=cf_lookup_call\n", "longer than 15 bytes"));
}

}  // namespace

int main() {
  test_mapping();
  test_refusals();
  return kztest::finish("test_rt_plan_convert");
}

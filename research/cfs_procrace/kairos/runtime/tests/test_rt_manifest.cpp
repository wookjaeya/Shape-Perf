// test_rt_manifest <data-dir> -- the "kzs-sites 1" / "kzs-sites 2" / "kzs-sites 3" parser accepts the example and
// rejects what the format forbids (each with its line); version 2 adds identity reads, the place of a point or effect
// site and parameter values, which a version 1 file cannot have; version 3 adds the readers of a request that
// publishes its application after the spawn (app-request, owner-publish, owner-check, request-return status).
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

#include "kz_rt.h"
#include "kz_rt_manifest.hpp"

#define CHECK(c)                                                              \
  do {                                                                        \
    if (!(c)) {                                                               \
      std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #c); \
      std::exit(1);                                                           \
    }                                                                         \
  } while (0)

namespace {

bool rejects(const std::string& body, const std::string& expect, const char* version = "1") {
  kzrt::Manifest m;
  std::string err;
  const std::string text = std::string("kzs-sites ") + version + "\nsource s rev=abc\nmodule 1 name=m source=s\nsite 1 label=a op=o phase=before tu=t.c func=f\n" + body;
  if (kzrt::parse_manifest(text, "t", &m, &err)) {
    std::fprintf(stderr, "accepted: %s\n", body.c_str());
    return false;
  }
  if (err.find(expect) == std::string::npos) {
    std::fprintf(stderr, "wrong error for '%s': %s\n", body.c_str(), err.c_str());
    return false;
  }
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  CHECK(argc > 1);
  std::ifstream f(std::string(argv[1]) + "/sites_example.kzs");
  CHECK(f.good());
  std::stringstream ss;
  ss << f.rdbuf();
  kzrt::Manifest m;
  std::string err;
  CHECK(kzrt::parse_manifest(ss.str(), "sites_example.kzs", &m, &err));
  CHECK(m.sites.size() == 4 && m.binds.size() == 4 && m.modules.size() == 2 && m.equivs.size() == 1);
  const kzrt::Site* s11 = m.site(11);
  CHECK(s11 != nullptr && s11->label == "obj.lookup.before" && s11->value(0) != nullptr && s11->value(0)->string_mode == "validated");
  CHECK(s11->value(0)->bound == 16 && s11->declared_mask() == 0x3u);
  CHECK(m.site(10)->has_control && kzrt::ctx_code(m.site(10)->ctx) == KZ_RT_CTX_TASK);
  CHECK(m.bind(1201)->vals == 0xFFu && !m.bind(1201)->vals_given && m.bind(1301)->status == "removed");
  CHECK(m.site_by_label("obj.use.before")->id == 10);
  // a second file (a backend's binding list) adds to the same manifest
  CHECK(kzrt::parse_manifest("kzs-sites 1\nbind 1002 site=10 backend=b module=5 at=\"x.c:1\" status=inlined vals=1\n", "binds", &m, &err));
  CHECK(m.bind(1002) != nullptr && m.bind(1002)->vals == 1u);

  CHECK(rejects("site 1 label=b op=o phase=before tu=t.c func=f\n", "declared twice"));
  CHECK(rejects("site 2 label=a op=o phase=before tu=t.c func=f\n", "label"));
  CHECK(rejects("site 2 label=b op=o phase=during tu=t.c func=f\n", "phase"));
  CHECK(rejects("value 1 8 role=r from=arg0 type=u32\n", "slot"));
  CHECK(rejects("value 1 0 role=r from=arg0 type=u32 string=validated\n", "type=ptr"));
  CHECK(rejects("value 1 0 role=r from=arg0 type=ptr string=validated bound=16\n", "ref="));
  CHECK(rejects("value 1 0 role=r from=arg0 type=ptr string=validated bound=65 ref=x\n", "bound"));
  CHECK(rejects("value 1 0 role=r from=reg3 type=u32\n", "from"));
  CHECK(rejects("value 9 0 role=r from=arg0 type=u32\n", "undeclared site"));
  CHECK(rejects("control 1 ctx=sometimes\n", "ctx"));
  CHECK(rejects("bind 5 site=1 backend=b module=1 at=x status=removed\n", "reason"));
  CHECK(rejects("bind 5 site=1 backend=b module=1 at=x status=bound vals=1\n", "does not declare"));
  CHECK(rejects("bind 5 site=1 backend=b module=2 at=x status=bound\n", "module"));
  CHECK(rejects("equiv 1 backends=a,b status=confirmed\n", "evidence"));
  CHECK(rejects("site 0 label=b op=o phase=before tu=t.c func=f\n", "site <id"));
  CHECK(rejects("bogus 1\n", "unknown statement"));
  CHECK(rejects("site 2 label=\"b op=o phase=before tu=t.c func=f\n", "quote"));
  {
    kzrt::Manifest m2;
    CHECK(!kzrt::parse_manifest("site 1 label=a\n", "nohdr", &m2, &err) && err.find("kzs-sites 1") != std::string::npos);
    CHECK(!kzrt::parse_manifest("kzs-sites 4\n", "v4", &m2, &err) && err.find("kzs-sites 3") != std::string::npos);
  }
  CHECK(m.version == 1u);

  // version 2: identity reads, in a file of their own over the sites of a version 1 file (unit-test names)
  {
    kzrt::Manifest mi;
    const char* sites =
        "kzs-sites 1\nsource s rev=abc\nmodule 1 name=m source=s\n"
        "site 1 label=req op=req phase=before tu=t.c func=start\nvalue 1 2 role=owner_id from=arg4 type=u32\n"
        "site 2 label=req.ret op=req phase=after tu=t.c func=start\nvalue 2 0 role=status from=ret type=s32\n"
        "site 3 label=mk op=mk phase=before tu=t.c func=caller callee=mk\n"
        "site 4 label=mk.ret op=mk phase=after tu=t.c func=caller callee=mk\nvalue 4 0 role=status from=ret type=s32\nvalue 4 1 role=task_id from=out0 type=u32\n"
        "site 5 label=mk.id op=mk phase=before tu=t.c func=impl\nvalue 5 0 role=task_id from=arg4 type=u32\n"
        "site 6 label=entry op=entry phase=before tu=t.c func=entry\nvalue 6 0 role=task_id from=arg0 type=u32\nvalue 6 1 role=p from=arg1 type=ptr\n"
        "site 7 label=child op=child phase=before tu=t.c func=child\nsite 8 label=child.ret op=child phase=after tu=t.c func=child\n";
    CHECK(kzrt::parse_manifest(sites, "sites", &mi, &err));
    const char* ident =
        "kzs-sites 2\n"
        "identity 1 decoder=owner-request owner=2 ref=\"unit test\"\nidentity 2 decoder=request-return\n"
        "identity 3 decoder=spawn-create\nidentity 4 decoder=spawn-return id=1 status=0 ok=0\nidentity 5 decoder=spawn-id id=0\n"
        "identity 6 decoder=thread-entry id=0\nidentity 7 decoder=child-request\nidentity 8 decoder=request-return\n";
    if (!kzrt::parse_manifest(ident, "ident", &mi, &err)) std::fprintf(stderr, "%s\n", err.c_str());
    CHECK(mi.version == 2u);
    CHECK(mi.site(1)->has_identity && mi.site(1)->identity.decoder == "owner-request" && mi.site(1)->identity.owner_slot == 2 && mi.site(1)->identity.id_slot == -1);
    CHECK(mi.site(4)->identity.status_slot == 0 && mi.site(4)->identity.ok == 0);
    CHECK(mi.site(4)->identity.id_slot == 1 && mi.site(6)->identity.id_slot == 0 && mi.site(3)->identity.id_slot == -1 && !mi.site(3)->has_control);
    // what version 2 forbids
    const std::string base = "value 1 0 role=status from=ret type=s32\nvalue 1 1 role=p from=arg0 type=ptr\nvalue 1 2 role=w from=arg1 type=u64\nvalue 1 3 role=id from=arg2 type=u32\n";
    CHECK(rejects(base + "identity 1 decoder=thread-entry id=3\n", "kzs-sites 2"));
    CHECK(rejects(base + "identity 1 decoder=guess\n", "decoder is", "2"));
    CHECK(rejects(base + "identity 9 decoder=spawn-create\n", "undeclared site", "2"));
    CHECK(rejects(base + "identity 1 decoder=spawn-return\n", "phase=after", "2"));
    CHECK(rejects(base + "identity 1 decoder=thread-entry\n", "needs id=", "2"));
    CHECK(rejects(base + "identity 1 decoder=owner-request\n", "needs owner=", "2"));
    CHECK(rejects(base + "identity 1 decoder=thread-entry id=5\n", "declares no value", "2"));
    CHECK(rejects(base + "identity 1 decoder=thread-entry id=1\n", "integer of at most 4 bytes", "2"));
    CHECK(rejects(base + "identity 1 decoder=thread-entry id=2\n", "integer of at most 4 bytes", "2"));
    CHECK(rejects(base + "identity 1 decoder=spawn-create id=3\n", "takes no id=", "2"));
    CHECK(rejects(base + "identity 1 decoder=thread-entry id=3 owner=3\n", "takes no owner=", "2"));
    CHECK(rejects(base + "identity 1 decoder=thread-entry id=3\nidentity 1 decoder=thread-entry id=3\n", "declared twice", "2"));
    CHECK(rejects(base + "identity 1 decoder=thread-entry id=3 slot=1\n", "unknown attribute", "2"));
    const std::string ret = "site 2 label=r op=o phase=after tu=t.c func=f\nvalue 2 0 role=status from=ret type=s32\nvalue 2 1 role=id from=out0 type=u32\nvalue 2 2 role=a from=arg0 type=u32\n";
    CHECK(rejects(ret + "identity 2 decoder=spawn-return id=2 status=0 ok=0\n", "from=out<k> or ret", "2"));
    CHECK(rejects(ret + "identity 2 decoder=spawn-return id=1\n", "needs status=", "2"));
    CHECK(rejects(ret + "identity 2 decoder=spawn-return id=1 status=0 ok=yes\n", "ok=<integer>", "2"));
    CHECK(rejects(ret + "identity 2 decoder=spawn-return status=0 ok=0\n", "belong to the id=", "2"));
    CHECK(rejects(base + "identity 1 decoder=thread-entry id=3 status=0 ok=0\n", "belong to the id=", "2"));
  }
  // version 2: the statement a point or effect site is bound to (place=), and the parameter values (from=param<k>)
  {
    kzrt::Manifest mp;
    const char* placed =
        "kzs-sites 2\nsource s rev=abc\nmodule 1 name=m source=s\n"
        "site 1 label=p.at op=o phase=point tu=t.c func=f callee=g line=12 place=at-call\nvalue 1 0 role=name from=param1 type=ptr string=validated bound=20 ref=\"checked at t.c:10\"\n"
        "site 2 label=p.after op=o phase=effect tu=t.c func=f callee=memset place=after-call\nvalue 2 0 role=dest from=arg0 type=ptr\n"
        "site 3 label=p.line op=o phase=point tu=t.c func=f line=14 place=line-start\n"
        "site 4 label=p.store op=o phase=effect tu=t.c func=f line=15 place=store\nvalue 4 0 role=required from=operand0 type=u32\nvalue 4 1 role=addr from=operand1 type=ptr\n"
        "site 5 label=p.id op=o phase=point tu=t.c func=impl callee=go place=at-call\nvalue 5 0 role=task_id from=arg4 type=u32\nidentity 5 decoder=spawn-id id=0\n";
    if (!kzrt::parse_manifest(placed, "placed", &mp, &err)) std::fprintf(stderr, "%s\n", err.c_str());
    CHECK(mp.site(1)->place == "at-call" && mp.site(1)->value(0)->from == "param1" && mp.site(2)->place == "after-call" && mp.site(2)->src_line == 0);
    CHECK(mp.site(3)->place == "line-start" && mp.site(4)->place == "store" && mp.site(5)->has_identity && mp.site(5)->place == "at-call");
    CHECK(rejects("site 2 label=b op=o phase=point tu=t.c func=f callee=g place=at-call\n", "kzs-sites 2"));
    CHECK(rejects("value 1 0 role=r from=param0 type=u32\n", "kzs-sites 2"));
    CHECK(rejects("site 2 label=b op=o phase=point tu=t.c func=f callee=g place=around\n", "place is", "2"));
    CHECK(rejects("site 2 label=b op=o phase=before tu=t.c func=f callee=g place=at-call\n", "phase=point|effect|exit", "2"));
    CHECK(rejects("site 2 label=b op=o phase=point tu=t.c func=f place=after-call\n", "needs callee=", "2"));
    CHECK(rejects("site 2 label=b op=o phase=effect tu=t.c func=f callee=g line=3 place=store\n", "takes no callee=", "2"));
    CHECK(rejects("site 2 label=b op=o phase=point tu=t.c func=f place=line-start\n", "needs line=", "2"));
  }
  // version 3: a request that creates an application and publishes its value after the spawn (app-request,
  // owner-publish, owner-check) and the status of a request-return; none of them in a version 2 file
  {
    kzrt::Manifest m3;
    const char* v3 =
        "kzs-sites 3\nsource s rev=abc\nmodule 1 name=m source=s\n"
        "site 1 label=app op=app phase=before tu=t.c func=create_app\n"
        "site 2 label=app.ret op=app phase=after tu=t.c func=create_app\nvalue 2 0 role=status from=ret type=s32\n"
        "site 3 label=app.out op=app phase=point tu=t.c func=create_app line=40 place=store\nvalue 3 0 role=app_id from=operand0 type=u32\n"
        "site 4 label=app.table op=pub phase=point tu=t.c func=create_app line=38 place=store\nvalue 4 0 role=app_id from=operand0 type=u32\n"
        "identity 1 decoder=app-request ref=\"unit test\"\nidentity 2 decoder=request-return status=0 ok=0\n"
        "identity 3 decoder=owner-publish owner=0\nidentity 4 decoder=owner-check owner=0\n";
    if (!kzrt::parse_manifest(v3, "v3", &m3, &err)) std::fprintf(stderr, "%s\n", err.c_str());
    CHECK(m3.version == 3u);
    CHECK(m3.site(1)->identity.decoder == "app-request" && m3.site(1)->identity.owner_slot == -1 && m3.site(1)->identity.status_slot == -1);
    CHECK(m3.site(2)->identity.decoder == "request-return" && m3.site(2)->identity.status_slot == 0 && m3.site(2)->identity.ok == 0);
    CHECK(m3.site(3)->identity.owner_slot == 0 && m3.site(4)->identity.decoder == "owner-check" && m3.site(4)->identity.owner_slot == 0);
    const std::string pt = "site 2 label=p op=o phase=point tu=t.c func=f line=9 place=store\nvalue 2 0 role=v from=operand0 type=u32\nvalue 2 1 role=a from=operand1 type=ptr\n"
                           "site 3 label=r op=o phase=after tu=t.c func=f\nvalue 3 0 role=status from=ret type=s32\n";
    CHECK(rejects("identity 1 decoder=app-request\n", "needs 'kzs-sites 3'", "2"));
    CHECK(rejects(pt + "identity 2 decoder=owner-publish owner=0\n", "needs 'kzs-sites 3'", "2"));
    CHECK(rejects(pt + "identity 2 decoder=owner-check owner=0\n", "needs 'kzs-sites 3'", "2"));
    CHECK(rejects(pt + "identity 3 decoder=request-return status=0 ok=0\n", "belong to the id= of a spawn-return", "2"));
    CHECK(rejects(pt + "identity 2 decoder=app-request\n", "phase=before", "3"));
    CHECK(rejects(pt + "identity 3 decoder=owner-publish owner=0\n", "phase=point|effect", "3"));
    CHECK(rejects(pt + "identity 2 decoder=owner-publish\n", "needs owner=", "3"));
    CHECK(rejects(pt + "identity 2 decoder=owner-check owner=1\n", "integer of at most 4 bytes", "3"));
    CHECK(rejects(pt + "identity 1 decoder=app-request owner=0\n", "takes no owner=", "3"));
    CHECK(rejects(pt + "identity 2 decoder=owner-publish owner=0 id=0\n", "takes no id=", "3"));
    CHECK(rejects(pt + "identity 3 decoder=request-return status=0\n", "together", "3"));
    CHECK(rejects(pt + "identity 3 decoder=request-return ok=0\n", "together", "3"));
    CHECK(rejects(pt + "identity 2 decoder=owner-publish owner=0 status=0 ok=0\n", "or to a request-return", "3"));
  }
  std::printf("manifest: example accepted (%zu sites, %zu bindings), 18 malformed inputs rejected; version 2: 8 identity reads accepted, 18 rejected; "
              "5 placed sites accepted, 7 rejected; version 3: 4 identity reads accepted, 13 rejected\n",
              m.sites.size(), m.binds.size());
  return 0;
}

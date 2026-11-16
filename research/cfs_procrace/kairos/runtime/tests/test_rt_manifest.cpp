// test_rt_manifest <data-dir> -- the "kzs-sites 1" / "kzs-sites 2" / "kzs-sites 3" / "kzs-sites 4" parser accepts the
// example and rejects what the format forbids (each with its line); version 2 adds identity reads, the place of a point
// or effect site and parameter values, which a version 1 file cannot have; version 3 adds the readers of a request that
// publishes its application after the spawn (app-request, owner-publish, owner-check, request-return status); version 4
// adds the plan header, site keys, rows, columns, ordinals and loads, need and gap statements, the spawn binding and
// the spawn-token reader, none of which a version 1-3 file can have.
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
    CHECK(!kzrt::parse_manifest("kzs-sites 5\n", "v5", &m2, &err) && err.find("kzs-sites 4") != std::string::npos);
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
  // version 4: the plan header, site keys and positions, loads, need and gap, the spawn binding and its token reader
  int v4_rejected = 0;
  {
    kzrt::Manifest m4;
    const std::string v4 =
        "kzs-sites 4\n"
        "plan 0123456789abcdef facts=" + std::string(64, 'a') + " relrules=\"relations rules revision 9\" obsrules=\"plan rules revision 3\" semantics=none "
        "select=observation-set budget=tier:D07 control=\"rows:3\"\n"
        "source build rev=abc\nmodule 1 name=m source=build\n"
        "site 1 label=s0001.use-call op=call.lookup phase=point tu=a.c func=init callee=lookup line=12 place=at-call skey=00000000000000a1 row=17975444d83f6589 col=19 nth=1\n"
        "value 1 0 role=key from=arg1 type=ptr\n"
        "site 2 label=s0002.use-call op=call.lookup phase=point tu=a.c func=init callee=lookup line=12 place=after-call skey=00000000000000a2 row=17975444d83f6589,35f07539e7606e2e col=19 nth=1\n"
        "value 2 0 role=status from=ret type=s64\n"
        "site 3 label=s0003.read op=load.reader phase=point tu=b.c func=reader line=861 place=load field=Rec_t.used skey=00000000000000a3\n"
        "value 3 0 role=value from=operand0 type=u64\nvalue 3 1 role=address from=operand1 type=ptr\n"
        "site 4 label=s0004.actor-create op=call.pthread_create phase=point tu=c.c func=create callee=pthread_create line=40 place=at-call skey=00000000000000a4\n"
        "value 4 0 role=start_routine from=arg2 type=u64\n"
        "spawn 4 wrapper=kz_rt_posix_create start=arg2 arg=arg3 out=arg0 ref=\"P0 thread creation\"\n"
        "identity 4 decoder=spawn-token\n"
        "need 1 subject=\"row 17975444d83f6589\" what=wait-result state=not-planned reason=\"plan_missing(need not planned)\"\n"
        "need 2 subject=\"row 17975444d83f6589\" what=use-result state=planned sites=2\n"
        "gap 1 subject=\"row 17975444d83f6589\" what=\"key text\" reason=\"record_insufficient(key text)\" rule=key-text\n";
    if (!kzrt::parse_manifest(v4, "v4", &m4, &err)) std::fprintf(stderr, "%s\n", err.c_str());
    CHECK(m4.version == 4u && m4.has_plan && m4.plan.id == "0123456789abcdef" && m4.plan.control == "rows:3" && m4.plan.semantics == "none");
    CHECK(m4.site(1)->skey == "00000000000000a1" && m4.site(1)->col == 19u && m4.site(1)->nth == 1u && m4.site(1)->rows.size() == 1u);
    CHECK(m4.site(2)->rows.size() == 2u && m4.site(2)->rows[1] == "35f07539e7606e2e");
    CHECK(m4.site(3)->place == "load" && m4.site(3)->field == "Rec_t.used" && m4.site(3)->src_line == 861u);
    CHECK(m4.site(4)->has_spawn && m4.site(4)->spawn.wrapper == "kz_rt_posix_create" && m4.site(4)->spawn.start_arg == 2 && m4.site(4)->spawn.arg_arg == 3 &&
          m4.site(4)->spawn.out_arg == 0);
    CHECK(m4.site(4)->has_identity && m4.site(4)->identity.decoder == "spawn-token" && m4.site(4)->identity.id_slot == -1);
    CHECK(m4.needs.size() == 2u && m4.needs[1].sites.size() == 1u && m4.needs[1].sites[0] == 2u && m4.gaps.size() == 1u && m4.gaps[0].rule == "key-text");
    CHECK(m4.skey_index.size() == 4u);
    // a version 4 file of a backend's bindings (no plan) adds to the same manifest
    CHECK(kzrt::parse_manifest("kzs-sites 4\nbind 7 site=1 backend=b module=1 at=\"a.c:12\" status=bound\n", "binds4", &m4, &err));
    // none of the version 4 words in a version 3 file
    const char* v3only[] = {
        "site 2 label=b op=o phase=point tu=t.c func=f callee=g line=3 place=at-call skey=00000000000000a1\n",
        "site 2 label=b op=o phase=point tu=t.c func=f callee=g line=3 place=at-call col=4\n",
        "site 2 label=b op=o phase=point tu=t.c func=f callee=g line=3 place=at-call nth=1\n",
        "site 2 label=b op=o phase=point tu=t.c func=f callee=g line=3 place=at-call row=17975444d83f6589\n",
        "site 2 label=b op=o phase=point tu=t.c func=f line=3 place=load\n",
        "plan 0123456789abcdef facts=x\n",
        "need 1 subject=s what=w state=planned sites=1\n",
        "gap 1 subject=s what=w reason=r\n",
        "spawn 1 wrapper=w start=arg0 arg=arg1\n",
    };
    for (const char* b : v3only) { CHECK(rejects(b, "needs 'kzs-sites 4'", "3")); ++v4_rejected; }
    const std::string at = "site 2 label=c op=o phase=point tu=t.c func=f callee=pthread_create line=3 place=at-call\nvalue 2 0 role=r from=arg2 type=u64\n";
    CHECK(rejects(at + "identity 2 decoder=spawn-token\n", "needs 'kzs-sites 4'", "3"));
    ++v4_rejected;
    struct Bad {
      std::string body;
      const char* expect;
    };
    const std::string pl = "plan 0123456789abcdef facts=" + std::string(64, 'a') + " relrules=r obsrules=o semantics=none select=s budget=b\n";
    const Bad bad[] = {
        {"site 2 label=b op=o phase=point tu=t.c func=f line=3 place=load\n", "needs field="},
        {"site 2 label=b op=o phase=point tu=t.c func=f line=3 place=store field=T.f\n", "only for place=load"},
        {"site 2 label=b op=o phase=point tu=t.c func=f line=3 place=load field=Tf\n", "field=<Type.field>"},
        {"site 2 label=b op=o phase=point tu=t.c func=f callee=g line=3 place=load field=T.f\n", "takes no callee="},
        {"site 2 label=b op=o phase=effect tu=t.c func=f line=3 place=load field=T.f\n", "needs phase=point"},
        {"site 2 label=b op=o phase=point tu=t.c func=f line=3 place=store col=4\n", "needs callee="},
        {"site 2 label=b op=o phase=point tu=t.c func=f callee=g line=3 place=at-call skey=XYZ\n", "skey=<16 hex digits>"},
        {"site 2 label=b op=o phase=point tu=t.c func=f callee=g line=3 place=at-call row=17975444d83f658\n", "row=<16 hex digits>"},
        {"site 2 label=b op=o phase=point tu=t.c func=f callee=g line=3 place=at-call col=0\n", "col=<n>"},
        {"site 2 label=b op=o phase=point tu=t.c func=f callee=g line=3 place=at-call skey=00000000000000a1\nsite 3 label=c op=o phase=point tu=t.c func=f callee=g line=4 place=at-call skey=00000000000000a1\n", "declared twice"},
        {pl, "before the first site"},
        {"need 1 subject=s what=w state=planned\n", "needs sites="},
        {"need 1 subject=s what=w state=not-planned\n", "needs reason="},
        {"need 1 subject=s what=w state=maybe reason=r\n", "state is"},
        {"need 1 subject=s what=w state=planned sites=9\n", "undeclared site"},
        {"need 1 subject=s what=w state=planned sites=1\nneed 1 subject=s what=w state=planned sites=1\n", "declared twice"},
        {"gap 1 subject=s what=w\n", "needs reason="},
        {"gap 1 what=w reason=r\n", "subject="},
        {"spawn 1 wrapper=w start=arg0 arg=arg1\n", "place=at-call"},
        {at + "spawn 2 wrapper=w start=arg2\n", "start=arg<k> and arg=arg<k>"},
        {at + "spawn 2 wrapper=\"w x\" start=arg2 arg=arg3\n", "wrapper=<function name>"},
        {at + "spawn 2 wrapper=w start=arg2 arg=arg2\n", "different arguments"},
        {at + "spawn 2 wrapper=w start=arg2 arg=arg3\nspawn 2 wrapper=w start=arg2 arg=arg3\n", "declared twice"},
        {at + "identity 2 decoder=spawn-token\n", "spawn statement first"},
        {at + "spawn 2 wrapper=w start=arg2 arg=arg3\nidentity 2 decoder=spawn-token id=0\n", "takes no slot"},
    };
    for (const Bad& b : bad) { CHECK(rejects(b.body, b.expect, "4")); ++v4_rejected; }
    // the plan statement: once per manifest, its fields
    {
      kzrt::Manifest mp;
      const std::string head = "kzs-sites 4\n" + pl;
      CHECK(kzrt::parse_manifest(head, "p1", &mp, &err));
      CHECK(!kzrt::parse_manifest(head, "p2", &mp, &err) && err.find("declared twice") != std::string::npos);
      ++v4_rejected;
      kzrt::Manifest mq;
      CHECK(!kzrt::parse_manifest("kzs-sites 4\nplan 0123456789abcdef facts=abc relrules=r obsrules=o semantics=none select=s budget=b\n", "p3", &mq, &err) &&
            err.find("64 hex digits") != std::string::npos);
      ++v4_rejected;
      kzrt::Manifest mr;
      CHECK(!kzrt::parse_manifest("kzs-sites 4\nplan 0123456789abcdef facts=" + std::string(64, 'a') + " relrules=r obsrules=o select=s budget=b\n", "p4", &mr, &err) &&
            err.find("semantics=") != std::string::npos);
      ++v4_rejected;
    }
  }
  std::printf("manifest: example accepted (%zu sites, %zu bindings), 18 malformed inputs rejected; version 2: 8 identity reads accepted, 18 rejected; "
              "5 placed sites accepted, 7 rejected; version 3: 4 identity reads accepted, 13 rejected; version 4: a plan with 4 sites, 2 needs, 1 gap "
              "and a spawn binding accepted, %d rejected\n",
              m.sites.size(), m.binds.size(), v4_rejected);
  return 0;
}

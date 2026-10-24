// Unit tests of the profile (kzp-observe) reader: the grammar, the errors, the slots.
#include "profile.hpp"
#include "support/kz_test.hpp"

using namespace kz::plan;

namespace {

const char* kGood = R"(# a comment
kzp-observe 1
binary prog   # trailing comment
prov p1 tier=code-derived kind=code_pattern file="src/a.c" lines=10-12 cite="the \"quoted\" cite" authored="me" sha256=0000000000000000000000000000000000000000000000000000000000000001
prov p2 tier=researcher kind=researcher file="doc.md" lines=3-3 cite="why" root=repo
prov p3 tier=unreferenced
site f.entry entry func=fn in=src/a.c prov=p1 s0=rdi:u16:id s1=rsi:u32:arg s2=r9:u64:wide s3=stack+8:u64:return_address
site f.ret ret func=fn decoder=spawn-return s0=rax:u32:status s3=ip:u64:return_address
site g.point point at=src/b.c:42 after-call=callee prov=p2 s0=rax:u8:b
)";

kz::Status parse(const char* text, Profile* p = nullptr) {
  Profile local;
  return parse_profile(text, p != nullptr ? p : &local);
}

void test_good() {
  Profile p;
  KZ_CHECK_OK(parse(kGood, &p));
  KZ_CHECK_EQ(p.binary_name, std::string("prog"));
  KZ_CHECK_EQ(p.provs.size(), size_t(3));
  KZ_CHECK_EQ(p.provs[0].id, std::string("p1"));
  KZ_CHECK_EQ(p.provs[0].tier, uint8_t(KZ_TIER_CODE_DERIVED));
  KZ_CHECK_EQ(p.provs[0].kind, uint8_t(KZ_PROV_KIND_CODE_PATTERN));
  KZ_CHECK_EQ(p.provs[0].file, std::string("src/a.c"));
  KZ_CHECK_EQ(p.provs[0].line_from, 10u);
  KZ_CHECK_EQ(p.provs[0].line_to, 12u);
  KZ_CHECK_EQ(p.provs[0].cite, std::string("the \"quoted\" cite"));
  KZ_CHECK(p.provs[0].has_sha256);
  KZ_CHECK_EQ(p.provs[0].sha256[31], uint8_t(1));
  KZ_CHECK_EQ(p.provs[1].root, std::string("repo"));
  KZ_CHECK(!p.provs[1].has_sha256);
  KZ_CHECK_EQ(p.provs[2].tier, uint8_t(KZ_TIER_UNREFERENCED));
  KZ_CHECK_EQ(p.sites.size(), size_t(3));
  const ProfileSite& s = p.sites[0];
  KZ_CHECK_EQ(s.label, std::string("f.entry"));
  KZ_CHECK_EQ(s.kind, uint16_t(KZ_SITE_KIND_ENTRY_PROBE));
  KZ_CHECK_EQ(s.func, std::string("fn"));
  KZ_CHECK_EQ(s.in_suffix, std::string("src/a.c"));
  KZ_CHECK_EQ(s.prov, 0);
  KZ_CHECK_EQ(s.decoder, uint16_t(KZ_DECODER_GENERIC));
  KZ_CHECK_EQ(s.slots.size(), size_t(4));
  KZ_CHECK_EQ(s.slots[0].slot, uint8_t(0));
  KZ_CHECK_EQ(s.slots[0].src, uint8_t(KZ_SLOT_SRC_REG));
  KZ_CHECK_EQ(s.slots[0].index, uint32_t(KZ_REG_RDI));
  KZ_CHECK_EQ(s.slots[0].width, uint8_t(2));
  KZ_CHECK_EQ(s.slots[0].role, std::string("id"));
  KZ_CHECK_EQ(s.slots[2].index, uint32_t(KZ_REG_R9));
  KZ_CHECK_EQ(s.slots[2].width, uint8_t(8));
  KZ_CHECK_EQ(s.slots[3].src, uint8_t(KZ_SLOT_SRC_STACK));
  KZ_CHECK_EQ(s.slots[3].index, 8u);
  KZ_CHECK_EQ(s.source_line, 7u);
  KZ_CHECK_EQ(p.sites[1].kind, uint16_t(KZ_SITE_KIND_RET_PROBE));
  KZ_CHECK_EQ(p.sites[1].decoder, uint16_t(KZ_DECODER_SPAWN_RETURN));
  KZ_CHECK_EQ(p.sites[1].prov, -1);
  KZ_CHECK_EQ(p.sites[1].slots[1].src, uint8_t(KZ_SLOT_SRC_IP));
  KZ_CHECK_EQ(p.sites[2].kind, uint16_t(KZ_SITE_KIND_POINT_PROBE));
  KZ_CHECK_EQ(p.sites[2].at_file, std::string("src/b.c"));
  KZ_CHECK_EQ(p.sites[2].at_line, 42u);
  KZ_CHECK_EQ(p.sites[2].after_call, std::string("callee"));
  KZ_CHECK_EQ(p.sites[2].prov, 1);
  // the hash is that of the text as read: any change in the text changes it
  Profile q;
  KZ_CHECK_OK(parse((std::string(kGood) + "\n").c_str(), &q));
  KZ_CHECK(q.sha256 != p.sha256);
  KZ_CHECK(p.sha256 == kz::sha256(std::string_view(kGood)));
  // the last statement may lack its newline, and CRLF line ends are not special
  KZ_CHECK_OK(parse("kzp-observe 1\nsite a entry func=f s0=rax:u8:r"));
}

void test_errors() {
  auto bad = [](const std::string& text) { return parse(text.c_str()).code == kz::Err::Invalid; };
  const std::string h = "kzp-observe 1\n";
  KZ_CHECK(bad(""));
  KZ_CHECK(bad("site a entry func=f\n"));                       // no header
  KZ_CHECK(bad("kzp-observe 2\nsite a entry func=f\n"));         // wrong version
  KZ_CHECK(bad(h));                                             // no site
  KZ_CHECK(bad(h + "bogus x\n"));
  KZ_CHECK(bad(h + "site a entry\n"));                          // too short
  KZ_CHECK(bad(h + "site a middle func=f\n"));                  // unknown kind
  KZ_CHECK(bad(h + "site a entry func=f\nsite a ret func=f\n")); // label twice
  KZ_CHECK(bad(h + "site a entry at=x.c:1 after-call=g\n"));    // a point target on an entry
  KZ_CHECK(bad(h + "site a point func=f\n"));                   // an entry target on a point
  KZ_CHECK(bad(h + "site a point at=x.c after-call=g\n"));      // no line
  KZ_CHECK(bad(h + "site a point at=x.c:0 after-call=g\n"));
  KZ_CHECK(bad(h + "site a entry func=f decoder=magic\n"));
  KZ_CHECK(bad(h + "site a entry func=f prov=nope\n"));         // undeclared provenance
  KZ_CHECK(bad(h + "site a entry func=f s0=rdi:u16\n"));        // no role
  KZ_CHECK(bad(h + "site a entry func=f s0=eax:u16:r\n"));      // not a register of the set
  KZ_CHECK(bad(h + "site a entry func=f s0=rdi:u24:r\n"));
  KZ_CHECK(bad(h + "site a entry func=f s0=rdi:u8:r s0=rsi:u8:q\n"));  // slot twice
  KZ_CHECK(bad(h + "site a entry func=f s4=rdi:u8:r\n"));       // unknown key
  KZ_CHECK(bad(h + "site a entry func=f s0=stack+x:u64:r\n"));
  KZ_CHECK(bad(h + "site a entry func=f nonsense=1\n"));
  KZ_CHECK(bad(h + "site a entry func=\"unterminated\n"));
  KZ_CHECK(bad(h + "prov p tier=made-up file=\"f\" lines=1-2\nsite a entry func=f\n"));
  KZ_CHECK(bad(h + "prov p tier=code-derived\nsite a entry func=f\n"));              // a tier above unreferenced needs file and lines
  KZ_CHECK(bad(h + "prov p tier=code-derived file=\"f\" lines=5-2\nsite a entry func=f\n"));
  KZ_CHECK(bad(h + "prov p tier=code-derived file=\"f\" lines=0-2\nsite a entry func=f\n"));
  KZ_CHECK(bad(h + "prov p tier=unreferenced\nprov p tier=unreferenced\nsite a entry func=f\n"));  // id twice
  KZ_CHECK(bad(h + "prov p tier=unreferenced sha256=zz\nsite a entry func=f\n"));
  // A plan over several files: binaries are declared once each, before the sites; bin= names one of them.
  KZ_CHECK(bad(h + "binary a\nbinary a\nsite a entry func=f\n"));
  KZ_CHECK(bad(h + "binary a\nsite s entry func=f\nbinary b\n"));
  KZ_CHECK(bad(h + "binary a\nsite s entry func=f bin=b\n"));
  {
    kz::plan::Profile mp;
    KZ_CHECK_OK(kz::plan::parse_profile(h + "binary a\nbinary b.so\nsite s entry func=f\nsite t entry func=g bin=b.so\n"
                                            "site u point func=g after-call=h bin=b.so\n", &mp));
    KZ_CHECK_EQ(mp.binaries.size(), size_t(2));
    KZ_CHECK_EQ(mp.binary_name, std::string("a"));
    KZ_CHECK_EQ(mp.sites[0].bin, uint16_t(0));
    KZ_CHECK_EQ(mp.sites[1].bin, uint16_t(1));
    KZ_CHECK(mp.sites[2].function_scoped());
    KZ_CHECK(!mp.sites[1].function_scoped());
  }
  // A function-scoped point names the function and the callee; it is not mixed with a line target.
  KZ_CHECK(bad(h + "site p point func=g\n"));                                  // no callee
  KZ_CHECK(bad(h + "site p point func=g line-start\n"));                       // line-start needs a line
  KZ_CHECK(bad(h + "site p point func=g at=x.c:3 after-call=h\n"));            // both forms
  KZ_CHECK(bad(h + "site p point func=g in=x.c after-call=h\n"));              // in= selects by line table
  // The decoders of the identity join read fixed slots, and an entry/return pair is matched on slot 3:
  // a profile that does not declare them is refused (never decoded as zeros).
  const std::string ra = " s3=stack+0:u64:return_address";
  const std::string create_ok = "site c entry func=mk decoder=spawn-create s0=rsi:u64:name_ptr s1=rdx:u64:entry_fn s2=r9:u32:priority" + ra + "\n";
  KZ_CHECK_OK(parse((h + create_ok).c_str()));
  KZ_CHECK(bad(h + "site c entry func=mk decoder=spawn-create s0=rsi:u64:name_ptr s2=r9:u32:priority" + ra + "\n"));            // no entry_fn
  KZ_CHECK(bad(h + "site c entry func=mk decoder=spawn-create s1=r9:u32:priority s2=rdx:u64:entry_fn" + ra + "\n"));             // roles swapped
  KZ_CHECK(bad(h + "site c entry func=mk decoder=spawn-create s1=rdx:u64:entry_fn s2=r9:u64:priority" + ra + "\n"));            // priority wider than kept
  KZ_CHECK(bad(h + "site c entry func=mk decoder=spawn-create s1=rdx:u64:entry_fn s2=r9:u32:priority\n"));                       // no return address
  KZ_CHECK(bad(h + "site c entry func=mk decoder=spawn-create s1=rdx:u64:entry_fn s2=r9:u32:priority s3=stack+0:u64:ra\n"));     // wrong role at 3
  KZ_CHECK(bad(h + "site c ret func=mk decoder=spawn-create s1=rdx:u64:entry_fn s2=r9:u32:priority s3=ip:u64:return_address\n"));  // not an entry
  KZ_CHECK_OK(parse((h + "site r ret func=mk decoder=spawn-return s0=rax:u32:status s3=ip:u64:return_address\n").c_str()));
  KZ_CHECK(bad(h + "site r ret func=mk decoder=spawn-return s0=rax:u32:status\n"));
  KZ_CHECK(bad(h + "site r entry func=mk decoder=spawn-return s3=stack+0:u64:return_address\n"));
  KZ_CHECK_OK(parse((h + "site t entry func=te decoder=thread-entry s0=rdi:u32:task_id\n").c_str()));
  KZ_CHECK(bad(h + "site t entry func=te decoder=thread-entry s1=rdi:u32:task_id\n"));                                           // slot 0 missing
  KZ_CHECK(bad(h + "site t entry func=te decoder=thread-entry s0=rdi:u64:task_id\n"));                                           // wider than the id kept
  // generic entry/return pairs of one function: both need the key, a lone entry or return does not
  KZ_CHECK_OK(parse((h + "site e entry func=f s0=rdi:u32:x" + ra + "\nsite r ret func=f s0=rax:u32:status s3=ip:u64:return_address\n").c_str()));
  KZ_CHECK(bad(h + "site e entry func=f s0=rdi:u32:x\nsite r ret func=f s0=rax:u32:status s3=ip:u64:return_address\n"));
  KZ_CHECK(bad(h + "site e entry func=f s0=rdi:u32:x" + ra + "\nsite r ret func=f s0=rax:u32:status\n"));
  KZ_CHECK(bad(h + "site e entry func=f s0=rdi:u32:x s3=stack+0:u32:return_address\nsite r ret func=f s3=ip:u64:return_address\n"));  // key not 8 bytes
  KZ_CHECK_OK(parse((h + "site e entry func=f s0=rdi:u32:x\nsite r ret func=g s0=rax:u32:status\n").c_str()));
  // line-start points (W2): at=<file>:<line> with line-start instead of after-call, never both
  {
    Profile p;
    KZ_CHECK_OK(parse((h + "site s point at=x.c:7 line-start\n").c_str(), &p));
    KZ_CHECK(p.sites[0].line_start);
    KZ_CHECK(p.sites[0].after_call.empty());
    KZ_CHECK_EQ(p.sites[0].at_line, 7u);
  }
  KZ_CHECK(bad(h + "site s point at=x.c:7 line-start after-call=g\n"));
  KZ_CHECK(bad(h + "site s point at=x.c:7\n"));
  KZ_CHECK(bad(h + "site s entry func=f line-start\n"));
  // creation-side decoders (W2): the request names the owner at slot 2, the returns and the spawn id carry their keys
  {
    Profile p;
    KZ_CHECK_OK(parse((h + "site q entry func=st decoder=owner-request s2=r8:u32:owner_id" + ra + "\n" +
                       "site qr ret func=st decoder=request-return s0=rax:u32:status s3=ip:u64:return_address\n" +
                       "site c entry func=ct decoder=child-request" + ra + "\n" +
                       "site cr ret func=ct decoder=request-return s3=ip:u64:return_address\n" +
                       "site i entry func=impl decoder=spawn-id s0=r9:u32:task_id" + ra + "\n").c_str(), &p));
    KZ_CHECK_EQ(p.sites[0].decoder, uint16_t(KZ_DECODER_OWNER_REQUEST));
    KZ_CHECK_EQ(p.sites[1].decoder, uint16_t(KZ_DECODER_REQUEST_RETURN));
    KZ_CHECK_EQ(p.sites[2].decoder, uint16_t(KZ_DECODER_CHILD_REQUEST));
    KZ_CHECK_EQ(p.sites[4].decoder, uint16_t(KZ_DECODER_SPAWN_ID));
  }
  KZ_CHECK(bad(h + "site q entry func=st decoder=owner-request s1=r8:u32:owner_id" + ra + "\n"));     // owner not at slot 2
  KZ_CHECK(bad(h + "site q entry func=st decoder=owner-request s2=r8:u64:owner_id" + ra + "\n"));     // wider than an id
  KZ_CHECK(bad(h + "site q entry func=st decoder=owner-request s2=r8:u32:app" + ra + "\n"));          // wrong role
  KZ_CHECK(bad(h + "site q ret func=st decoder=owner-request s2=r8:u32:owner_id s3=ip:u64:return_address\n"));  // not an entry
  KZ_CHECK(bad(h + "site c entry func=ct decoder=child-request\n"));                                  // no return address
  KZ_CHECK(bad(h + "site r entry func=ct decoder=request-return" + ra + "\n"));                       // not a return
  KZ_CHECK(bad(h + "site i entry func=impl decoder=spawn-id s1=r9:u32:task_id\n"));                  // slot 0 missing
  KZ_CHECK(bad(h + "site i entry func=impl decoder=spawn-id s0=r9:u64:task_id\n"));                  // wider than an id
  // an error names its line
  const kz::Status s = parse((h + "\n\nsite a entry func=f nonsense=1\n").c_str());
  KZ_CHECK(s.message.find("line 4") != std::string::npos);
}

}  // namespace

int main() {
  test_good();
  test_errors();
  return kztest::finish("test_probe_profile");
}

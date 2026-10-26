// test_rt_manifest <data-dir> -- the "kzs-sites 1" parser accepts the example and rejects what the
// format forbids (each with its line).
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

bool rejects(const std::string& body, const std::string& expect) {
  kzrt::Manifest m;
  std::string err;
  const std::string text = "kzs-sites 1\nsource s rev=abc\nmodule 1 name=m source=s\nsite 1 label=a op=o phase=before tu=t.c func=f\n" + body;
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
  }
  std::printf("manifest: example accepted (%zu sites, %zu bindings), 17 malformed inputs rejected\n", m.sites.size(), m.binds.size());
  return 0;
}

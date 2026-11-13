// The kzp-hold 1 parser (probe/hold/hold_plan.cpp): `occurrence first|each|<n>`, `unless-released` and their
// refusals.  Unit-test values only.
#include <string>

#include "hold_plan.hpp"
#include "support/kz_test.hpp"

namespace {

bool parses(const std::string& body, kz::hold::HoldPlan* p) { return kz::hold::parse_hold_plan("kzp-hold 1\n" + body, p).ok(); }

void test_occurrence() {
  kz::hold::HoldPlan p;
  KZ_CHECK(parses("gate G site=a when name name \"X\" occurrence 3\nrelease R site=b releases G occurrence 25\n", &p));
  KZ_CHECK(p.gates.size() == 1 && p.releases.size() == 1);
  if (p.gates.size() == 1 && p.releases.size() == 1) {
    KZ_CHECK(!p.gates[0].occurrence_each && p.gates[0].occurrence_nth == 3);
    KZ_CHECK(!p.releases[0].occurrence_each && p.releases[0].occurrence_nth == 25);
    KZ_CHECK(p.gates[0].cond.has_name && p.gates[0].cond.name_text == "X");
  }
  // first = 1 (the default), each
  KZ_CHECK(parses("gate G site=a\nrelease R site=b releases G occurrence first\n", &p));
  KZ_CHECK(p.gates[0].occurrence_nth == 1 && !p.gates[0].occurrence_each && p.releases[0].occurrence_nth == 1);
  KZ_CHECK(parses("gate G site=a occurrence each\nrelease R site=b releases G\n", &p));
  KZ_CHECK(p.gates[0].occurrence_each);
  // two points on one site are allowed (the supervisor evaluates every point of an address)
  KZ_CHECK(parses("gate G1 site=a when name name \"X\"\ngate G2 site=a occurrence 2\nrelease R site=b releases G1\nrelease R2 site=b releases G2 occurrence 2\n", &p));
  // refusals: zero, a leading zero, a sign, not a number, too large, missing
  for (const char* bad : {"0", "03", "-1", "+2", "x", "1000001", "2.5"})
    KZ_CHECK(!parses(std::string("gate G site=a occurrence ") + bad + "\n", &p));
  KZ_CHECK(!parses("gate G site=a occurrence\n", &p));
  // unless-released: a gate form only, not with each
  KZ_CHECK(parses("gate G site=a when name name \"X\" occurrence 2 unless-released\nrelease R site=b releases G\n", &p));
  KZ_CHECK(p.gates.size() == 1 && p.gates[0].unless_released && p.gates[0].occurrence_nth == 2);
  KZ_CHECK(parses("gate G site=a\nrelease R site=b releases G\n", &p));
  KZ_CHECK(!p.gates[0].unless_released);
  KZ_CHECK(!parses("gate G site=a\nrelease R site=b releases G unless-released\n", &p));
  KZ_CHECK(!parses("gate G site=a occurrence each unless-released\n", &p));
  KZ_CHECK(kz::hold::occurrence_text(false, 1) == "first");
  KZ_CHECK(kz::hold::occurrence_text(false, 7) == "7");
  KZ_CHECK(kz::hold::occurrence_text(true, 1) == "each");
}

}  // namespace

int main() {
  test_occurrence();
  return kztest::finish("test_hold_plan");
}

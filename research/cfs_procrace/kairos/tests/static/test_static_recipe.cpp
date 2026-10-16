// The basis recipe language.
#include <string>

#include "recipe.hpp"
#include "support/kz_test.hpp"

using namespace kzs;

namespace {

const char* kGood = R"(# a recipe
kzb 1;
name "demo";
link "a/link.txt";
binary "bin/prog";
generated "list" "gen/list.c";
table "TBL" in "gen/list.c";
declared "claim" "a value" authored "who and why";
startup_script {
  reset POWERON;
  rule PROCESSOR volatile nonvolatile;
  rule POWERON nonvolatile;
  file volatile const "VOL" in "src/a.c";
  file nonvolatile const "NONVOL" in "src/a.c";
  mount "/cf" "exe/cf";
  entry library "LIB";
  entry app "APP";
  cite "src/a.c" 10 20 note "where the rule is";
  cite "src/b.c" 5 5;
}
)";

}  // namespace

int main() {
  Recipe r;
  KZ_CHECK_OK(parse_recipe(kGood, &r));
  KZ_CHECK_EQ(r.name, std::string("demo"));
  KZ_CHECK_EQ(r.link, std::string("a/link.txt"));
  KZ_CHECK_EQ(r.binary, std::string("bin/prog"));
  KZ_CHECK_EQ(r.generated.size(), size_t(1));
  KZ_CHECK_EQ(r.generated[0].role, std::string("list"));
  KZ_CHECK_EQ(r.tables.size(), size_t(1));
  KZ_CHECK_EQ(r.tables[0].symbol, std::string("TBL"));
  KZ_CHECK_EQ(r.tables[0].file, std::string("gen/list.c"));
  KZ_CHECK_EQ(r.declared.size(), size_t(1));
  KZ_CHECK_EQ(r.declared[0].authored, std::string("who and why"));
  KZ_CHECK(r.startup.present);
  KZ_CHECK_EQ(r.startup.reset, std::string("POWERON"));
  KZ_CHECK_EQ(r.startup.rules.size(), size_t(2));
  KZ_CHECK_EQ(r.startup.rules[0].order.size(), size_t(2));
  KZ_CHECK_EQ(r.startup.rules[1].order.size(), size_t(1));
  KZ_CHECK_EQ(r.startup.files.size(), size_t(2));
  KZ_CHECK_EQ(r.startup.files[1].constant, std::string("NONVOL"));
  KZ_CHECK_EQ(r.startup.mounts[0].dir, std::string("exe/cf"));
  KZ_CHECK_EQ(r.startup.library_word, std::string("LIB"));
  KZ_CHECK_EQ(r.startup.app_word, std::string("APP"));
  KZ_CHECK_EQ(r.startup.cites.size(), size_t(2));
  KZ_CHECK_EQ(r.startup.cites[0].from, 10u);
  KZ_CHECK_EQ(r.startup.cites[0].note, std::string("where the rule is"));
  KZ_CHECK(r.startup.cites[1].note.empty());

  // errors say where
  struct Bad {
    const char* text;
    const char* expect;
  };
  const Bad bad[] = {
      {"name \"x\";\n", "starts with `kzb 1;`"},
      {"kzb 2;\n", "starts with `kzb 1;`"},
      {"kzb 1;\nfrobnicate \"x\";\n", "line 2: unknown statement `frobnicate`"},
      {"kzb 1;\nname \"x\";\n", "no `link` statement"},
      {"kzb 1;\nlink \"x\"\n", "expected `;`"},
      {"kzb 1;\nlink x;\n", "expected a quoted string"},
      {"kzb 1;\nlink \"x\";\ntable \"T\" \"f.c\";\n", "expected `in`"},
      {"kzb 1;\nlink \"x\";\nstartup_script {\n  entry thing \"w\";\n}\n", "entry role must be"},
      {"kzb 1;\nlink \"x\";\nstartup_script {\n  mystery;\n}\n", "unknown statement `mystery` in startup_script"},
      {"kzb 1;\nlink \"unterminated;\n", "expected a quoted string"},
  };
  for (const Bad& b : bad) {
    Recipe rr;
    const kz::Status s = parse_recipe(b.text, &rr);
    KZ_CHECK(!s.ok());
    KZ_CHECK(s.message.find(b.expect) != std::string::npos);
  }
  return kztest::finish("test_static_recipe");
}

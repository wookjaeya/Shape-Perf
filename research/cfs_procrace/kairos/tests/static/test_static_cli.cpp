// The kz-static command line: usage errors, a unit that does not parse, damaged inputs, and the
// whole basis -> extract path on a small synthetic program (two units, a link command, a table,
// a startup script, a source tree with a commit).
//
//   test_static_cli <kz-static> <fixtures-dir> <objects-dir> <profile-dir> <c-compiler> <work-dir>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "kz/core/container.hpp"
#include "support/kz_test.hpp"

namespace fs = std::filesystem;

namespace {

int run(const std::string& command, std::string* out) {
  out->clear();
  FILE* p = popen((command + " 2>&1").c_str(), "r");
  if (p == nullptr) return -1;
  char buf[4096];
  size_t n;
  while ((n = std::fread(buf, 1, sizeof buf, p)) > 0) out->append(buf, n);
  const int status = pclose(p);
  return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

std::string q(const std::string& s) { return "'" + s + "'"; }

void write(const fs::path& p, const std::string& text) {
  fs::create_directories(p.parent_path());
  std::ofstream(p, std::ios::binary) << text;
}

std::string read(const fs::path& p) {
  std::ifstream in(p, std::ios::binary);
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

bool has(const std::string& text, const std::string& part) { return text.find(part) != std::string::npos; }

}  // namespace

int main(int argc, char** argv) {
  if (argc != 7) {
    std::fprintf(stderr, "usage: test_static_cli <kz-static> <fixtures-dir> <objects-dir> <profile-dir> <c-compiler> <work-dir>\n");
    return 2;
  }
  const std::string tool = argv[1], fixtures = argv[2], objects = argv[3], profile = argv[4], cc = argv[5];
  const fs::path work = fs::path(argv[6]) / "cli";
  fs::remove_all(work);
  fs::create_directories(work);
  std::string out;

  // ---- usage ---------------------------------------------------------------------------------------
  KZ_CHECK_EQ(run(q(tool), &out), 2);
  KZ_CHECK(has(out, "usage:"));
  KZ_CHECK_EQ(run(q(tool) + " frobnicate", &out), 2);
  KZ_CHECK_EQ(run(q(tool) + " extract", &out), 2);
  KZ_CHECK_EQ(run(q(tool) + " extract --profile " + q(profile) + " -o x --basis a --source b", &out), 2);
  KZ_CHECK(has(out, "exclude each other"));
  KZ_CHECK_EQ(run(q(tool) + " extract --profile " + q(profile) + " -o x --source y --no-such-option", &out), 2);
  KZ_CHECK(has(out, "unknown option"));
  KZ_CHECK_EQ(run(q(tool) + " extract --profile " + q(profile) + " -o x --source y --target NotAField", &out), 2);
  KZ_CHECK(has(out, "Type.field"));
  KZ_CHECK_EQ(run(q(tool) + " basis --profile " + q(profile), &out), 2);
  KZ_CHECK_EQ(run(q(tool) + " reach " + q((work / "none").string()), &out), 2);
  KZ_CHECK_EQ(run(q(tool) + " show", &out), 2);

  // the identity: the tool hash is stable between runs
  {
    std::string a, b;
    KZ_CHECK_EQ(run(q(tool) + " --identity", &a), 0);
    KZ_CHECK_EQ(run(q(tool) + " --identity", &b), 0);
    KZ_CHECK_EQ(a, b);
    KZ_CHECK(has(a, "tool=kz-static\ntool_sha256="));
  }

  // ---- inputs that are wrong ---------------------------------------------------------------------
  // a unit that does not parse stops the run and names the file
  KZ_CHECK_EQ(run(q(tool) + " extract --profile " + q(profile) + " --source " + q(fixtures + "/broken/syntax_error.c") + " -o " + q((work / "broken.kz").string()) + " -- -std=c99", &out), 1);
  KZ_CHECK(has(out, "syntax_error.c"));
  KZ_CHECK(!fs::exists(work / "broken.kz"));
  // a profile that is not there
  KZ_CHECK_EQ(run(q(tool) + " extract --profile " + q((work / "no_profile").string()) + " --source " + q(fixtures + "/spawn.c") + " -o " + q((work / "x.kz").string()) + " -- -std=c99", &out), 1);
  // a damaged container
  write(work / "garbage.kz", "this is not a container");
  KZ_CHECK_EQ(run(q(tool) + " show " + q((work / "garbage.kz").string()), &out), 1);
  KZ_CHECK_EQ(run(q(tool) + " show " + q((work / "missing.kz").string()), &out), 1);

  // a facts container is not a basis
  const std::string facts = (work / "spawn.kz").string();
  KZ_CHECK_EQ(run(q(tool) + " extract --profile " + q(profile) + " --source " + q(fixtures + "/spawn.c") + " --root " + q(fixtures) + " -o " + q(facts) + " -- -std=c99", &out), 0);
  KZ_CHECK_EQ(run(q(tool) + " extract --profile " + q(profile) + " --basis " + q(facts) + " -o " + q((work / "y.kz").string()), &out), 1);
  KZ_CHECK(has(out, "not a basis"));

  // the filters of `show`: one section, one callee
  KZ_CHECK_EQ(run(q(tool) + " show " + q(facts) + " --section targets", &out), 0);
  KZ_CHECK(has(out, "[targets]") && !has(out, "[calls]"));
  KZ_CHECK_EQ(run(q(tool) + " show " + q(facts) + " --section calls --callee fx_spawn", &out), 0);
  KZ_CHECK(has(out, "spawn.c:13:3 start_direct -> fx_spawn"));
  KZ_CHECK_EQ(run(q(tool) + " show " + q(facts) + " --section calls --callee start_with", &out), 0);
  KZ_CHECK(!has(out, "-> fx_spawn"));

  // ---- a synthetic program: basis, then extract under it ------------------------------------------
  const fs::path D = work / "mini";
  const std::string rev_a = "0123456789abcdef0123456789abcdef01234567", rev_b = "fedcba9876543210fedcba9876543210fedcba98";
  fs::create_directories(D / "obj");
  fs::create_directories(D / "src");
  fs::create_directories(D / "profile");
  fs::copy_file(fs::path(objects) / "basis_unit_a.c", D / "src/basis_unit_a.c");
  fs::copy_file(fs::path(objects) / "basis_unit_b.c", D / "src/basis_unit_b.c");
  for (const char* n : {"a", "b"}) {
    const std::string src = (D / (std::string("src/basis_unit_") + n + ".c")).string();
    KZ_CHECK_EQ(run(q(cc) + " -std=c99 -c " + q(src) + " -o " + q((D / (std::string("obj/") + n + ".o")).string()), &out), 0);
  }
  const std::string a_c = (D / "src/basis_unit_a.c").string(), b_c = (D / "src/basis_unit_b.c").string();
  // the compile database: the second entry for b.o is a variant of the same object (a coverage build, say)
  write(D / "compile_commands.json",
        "[\n{\"directory\": \"" + D.string() + "\", \"command\": \"" + cc + " -std=c99 -c " + a_c + " -o obj/a.o\", \"file\": \"" + a_c + "\"},\n" +
            "{\"directory\": \"" + D.string() + "\", \"command\": \"" + cc + " -std=c99 -c " + b_c + " -o obj/b.o\", \"file\": \"" + b_c + "\"},\n" +
            "{\"directory\": \"" + D.string() + "\", \"command\": \"" + cc + " -std=c99 -DVARIANT=1 -c " + b_c + " -o obj/b.o\", \"file\": \"" + b_c + "\"}\n]\n");
  write(D / "link.txt", cc + " -o prog obj/a.o obj/b.o -lm\n");
  write(D / "cf/start.scr", "! the startup script\nLIB, libx, init_fn, 0\n\nAPP, appy, main_fn, 50, 4096\n");
  write(D / ".git/HEAD", rev_a + "\n");
  write(D / "recipe.kzb", R"(kzb 1;
name "mini";
link "link.txt";
table "TBL" in "src/basis_unit_b.c";
declared "claim.unchecked" "a statement to be checked" authored "the test, as an example of a hand-entered fact";
startup_script {
  reset POWERON;
  rule POWERON nonvolatile;
  file nonvolatile const "STARTUP_FILE" in "src/basis_unit_a.c";
  mount "/cf" "cf";
  entry library "LIB";
  entry app "APP";
}
)");
  for (const fs::directory_entry& e : fs::directory_iterator(profile))
    if (e.path().extension() == ".kzp") fs::copy_file(e.path(), D / "profile" / e.path().filename());
  write(D / "profile/10_source.kzp", "phi 1;\nsource fx root \"\" rev \"" + rev_a + "\";\nconst FX_LIMIT = ref eval \"ITEM_LIMIT\" in \"src/basis_unit_b.c\" prov fx_author;\n");

  const std::string basis_cmd = q(tool) + " basis --profile " + q((D / "profile").string()) + " --compile-db " + q((D / "compile_commands.json").string()) + " --build-dir " + q(D.string()) +
                                " --bundle-root " + q(D.string()) + " --recipe " + q((D / "recipe.kzb").string()) + " --text";
  const std::string basis1 = (D / "basis1.kz").string(), basis2 = (D / "basis2.kz").string();
  KZ_CHECK_EQ(run(basis_cmd + " -o " + q(basis1), &out), 0);
  KZ_CHECK(has(out, "translation units          2"));
  KZ_CHECK(has(out, "1 at the pinned revision"));
  KZ_CHECK(has(out, "two compile commands produce"));
  KZ_CHECK(has(out, "startup script             cf/start.scr (1 libraries, 1 applications)"));
  KZ_CHECK(has(out, "hand-entered items         2"));
  KZ_CHECK(has(out, "const.FX_LIMIT = 40   [evaluated in src/basis_unit_b.c]"));
  KZ_CHECK(has(out, "table.TBL.entries = 2 (null-terminated)"));
  KZ_CHECK(has(out, "table.TBL[1] = \"second\", 40"));
  KZ_CHECK(has(out, "link.members_in_program = 2"));
  KZ_CHECK(has(out, "source.fx.revision_actual = " + rev_a + "  (from-reference)"));
  KZ_CHECK(has(out, "startup.entry[0] = library libx, init_fn, 0"));
  KZ_CHECK(has(out, "startup.entry[1] = app appy, main_fn, 50, 4096"));
  KZ_CHECK(has(out, "claim.unchecked = a statement to be checked  (declared-assumption)"));
  KZ_CHECK(has(out, "recipe.path = recipe.kzb"));
  KZ_CHECK(has(out, "link.options_ignored = -lm"));
  // the same inputs give the same bytes
  KZ_CHECK_EQ(run(basis_cmd + " -o " + q(basis2), &out), 0);
  KZ_CHECK(read(basis1) == read(basis2));
  KZ_CHECK(!read(basis1).empty());

  // a tree at another commit is recorded as a deviation, not hidden
  write(D / ".git/HEAD", rev_b + "\n");
  KZ_CHECK_EQ(run(basis_cmd + " -o " + q((D / "basis3.kz").string()), &out), 0);
  KZ_CHECK(has(out, "0 at the pinned revision"));
  KZ_CHECK(has(out, "source.fx.revision_actual = " + rev_b + "  (deviation)"));
  write(D / ".git/HEAD", rev_a + "\n");

  // a missing recipe file is an input error
  KZ_CHECK_EQ(run(q(tool) + " basis --profile " + q((D / "profile").string()) + " --compile-db " + q((D / "compile_commands.json").string()) + " --build-dir " + q(D.string()) + " --bundle-root " +
                      q(D.string()) + " --recipe " + q((D / "nothing.kzb").string()) + " -o " + q((D / "b.kz").string()),
                  &out),
              2);

  // extract under the basis: the units come from the basis, the facts carry its hash
  const std::string facts_mini = (D / "facts.kz").string();
  KZ_CHECK_EQ(run(q(tool) + " extract --profile " + q((D / "profile").string()) + " --basis " + q(basis1) + " -o " + q(facts_mini) + " --target Item.stack", &out), 0);
  KZ_CHECK(has(out, "translation units          2"));
  kz::Container basis_c, facts_c;
  KZ_CHECK_OK(kz::Container::read_file(basis1, &basis_c));
  KZ_CHECK_OK(kz::Container::read_file(facts_mini, &facts_c));
  KZ_CHECK_OK(facts_c.verify_content());
  KZ_CHECK(facts_c.basis_hash() == basis_c.artifact_hash());
  bool parent_found = false;
  for (const kz_parent& p : facts_c.parents())
    if (std::memcmp(p.sha256, basis_c.artifact_hash().data(), 32) == 0) parent_found = true;
  KZ_CHECK(parent_found);
  // the image of the table reaches the facts: the item initialized with the constant
  KZ_CHECK_EQ(run(q(tool) + " show " + q(facts_mini) + " --section init", &out), 0);
  KZ_CHECK(has(out, "Item.stack in TBL static-initializer value=1"));
  KZ_CHECK(has(out, "Item.stack in TBL static-initializer value=40"));

  fs::remove_all(work);
  return kztest::finish("test_static_cli");
}

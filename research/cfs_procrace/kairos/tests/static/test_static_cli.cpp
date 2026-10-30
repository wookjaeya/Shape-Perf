// The kz-static command line: usage errors, a unit that does not parse, damaged inputs, and the
// whole basis -> extract path on a small synthetic program (two units, a link command, a table,
// a startup script, a source tree with a commit), a closure of an executable with the
// modules its startup script loads (recipe `module`), and the profile-free mode on such a closure.
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
#include <vector>

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

  // ---- a derived (instrumented) build of the same program (SPEC 1.7 B'): its recorded commands carry the build
  // compiler's plugin options.  The basis records them (TU flags, a deviation, the plugin file's hash); the front end
  // reads the units without them; the facts are those of the reference commands, under the derived basis.
  {
    write(D / "fake_plugin.so", "a file standing for a build-compiler plugin; never loaded\n");
    const std::string plug = " -fplugin=" + (D / "fake_plugin.so").string() + " -fplugin-arg-fake-manifest=" + (D / "recipe.kzb").string() + " -fplugin-arg-fake-quiet";
    write(D / "compile_commands_derived.json",
          "[\n{\"directory\": \"" + D.string() + "\", \"command\": \"" + cc + " -std=c99" + plug + " -c " + a_c + " -o obj/a.o\", \"file\": \"" + a_c + "\"},\n" +
              "{\"directory\": \"" + D.string() + "\", \"command\": \"" + cc + " -std=c99" + plug + " -c " + b_c + " -o obj/b.o\", \"file\": \"" + b_c + "\"},\n" +
              "{\"directory\": \"" + D.string() + "\", \"command\": \"" + cc + " -std=c99 -DVARIANT=1 -c " + b_c + " -o obj/b.o\", \"file\": \"" + b_c + "\"}\n]\n");
    const std::string basis_d = (D / "basis_derived.kz").string(), facts_d = (D / "facts_derived.kz").string();
    KZ_CHECK_EQ(run(q(tool) + " basis --profile " + q((D / "profile").string()) + " --compile-db " + q((D / "compile_commands_derived.json").string()) + " --build-dir " +
                        q(D.string()) + " --bundle-root " + q(D.string()) + " --recipe " + q((D / "recipe.kzb").string()) + " --text -o " + q(basis_d),
                    &out),
                0);
    KZ_CHECK(has(out, "instrumentation.units_with_compiler_plugin = 2 of 2 units, 1 distinct option set(s)  (deviation)"));
    KZ_CHECK(has(out, "-fplugin-arg-fake-quiet  (2 units)"));
    KZ_CHECK(has(out, "instrumentation.plugin_file = fake_plugin.so sha256 "));
    KZ_CHECK(has(out, "front_end.arguments_removed"));
    KZ_CHECK(has(out, "const.FX_LIMIT = 40   [evaluated in src/basis_unit_b.c]"));  // the constants are read without the plugin options
    KZ_CHECK(!has(out, "link.members_without_command"));
    KZ_CHECK(read(basis_d) != read(basis1));
    KZ_CHECK_EQ(run(q(tool) + " extract --profile " + q((D / "profile").string()) + " --basis " + q(basis_d) + " -o " + q(facts_d) + " --target Item.stack", &out), 0);
    kz::Container fd;
    KZ_CHECK_OK(kz::Container::read_file(facts_d, &fd));
    KZ_CHECK_OK(fd.verify_content());
    // the facts sections are those of the reference commands; only the header (basis hash, parents) and the note that
    // names the basis ("extract.units_from basis <first 16 hex digits>", in STRINGS) differ
    kz::Container bd;
    KZ_CHECK_OK(kz::Container::read_file(basis_d, &bd));
    const std::string href = kz::to_hex(basis_c.artifact_hash()).substr(0, 16), hder = kz::to_hex(bd.artifact_hash()).substr(0, 16);
    size_t compared = 0, differing = 0;
    for (const kz_section_entry& e : facts_c.section_table()) {
      size_t na = 0, nb = 0;
      const uint8_t* pa = facts_c.section_data(e.type, &na);
      const uint8_t* pb = fd.section_data(e.type, &nb);
      ++compared;
      std::string a(reinterpret_cast<const char*>(pa), na), b(pb == nullptr ? "" : std::string(reinterpret_cast<const char*>(pb), nb));
      if (e.type == KZ_SEC_STRINGS) {
        const size_t at = a.find(href);
        if (at != std::string::npos) a.replace(at, href.size(), hder);
      }
      if (pb == nullptr || a != b) {
        std::printf("derived facts: section %u differs\n", e.type);
        ++differing;
      }
    }
    KZ_CHECK(compared > 0);
    KZ_CHECK_EQ(differing, static_cast<size_t>(0));
    KZ_CHECK_EQ(fd.section_table().size(), facts_c.section_table().size());
    KZ_CHECK(fd.basis_hash() != facts_c.basis_hash());
    // a link member that no compile command produces (an instrumentation runtime linked from outside the build) is recorded
    KZ_CHECK_EQ(run(q(cc) + " -std=c99 -c " + q((fs::path(objects) / "basis_unit_a.c").string()) + " -DKZ_OUTSIDE=1 -o " + q((D / "obj/outside.o").string()), &out), 0);
    write(D / "link_outside.txt", cc + " -o prog obj/a.o obj/b.o obj/outside.o -lm\n");
    KZ_CHECK_EQ(run(q(tool) + " basis --profile " + q((D / "profile").string()) + " --compile-db " + q((D / "compile_commands_derived.json").string()) + " --build-dir " +
                        q(D.string()) + " --bundle-root " + q(D.string()) + " --recipe " + q((D / "recipe.kzb").string()) + " --link " + q((D / "link_outside.txt").string()) +
                        " --text -o " + q((D / "basis_outside.kz").string()),
                    &out),
                0);
    KZ_CHECK(has(out, "link.members_without_command = 1: obj/outside.o (named on the link command)"));
  }

  // ---- a closure: the executable linked with the modules its startup script loads ------------------
  // The application stores an id through the executable's API (an out pointer); without the
  // executable's body the field would be DISABLED (address escape), in the closure it has a writer.
  {
    const fs::path C = work / "clos";
    for (const char* d : {"src", "obj", "cf", "mods/mylib", "mods/myapp", "profile"}) fs::create_directories(C / d);
    write(C / "src/core_main.c",
          "#define NONVOL_MOUNT \"/cf\"\n#define MODULE_EXT \".so\"\n#define STARTUP_FILE \"/cf/start.scr\"\n"
          "const char *nonvol_mount(void) { return NONVOL_MOUNT; }\nconst char *module_ext(void) { return MODULE_EXT; }\n"
          "const char *startup_file(void) { return STARTUP_FILE; }\nint main(void) { return 0; }\n");
    write(C / "src/core_api.c", "typedef struct { int obj_id; } Rec;\nRec rec_table[2];\nint sem_create(int *sem_id) { rec_table[0].obj_id = 7; *sem_id = rec_table[0].obj_id; return 0; }\n");
    write(C / "src/lib.c", "int lib_helper(int x) { return x + 1; }\n");
    write(C / "src/app.c",
          "typedef struct { int sem; } Throttle;\nThrottle thr[2];\nint sem_create(int *sem_id);\nint lib_helper(int x);\nint ext_fn(void);\n"
          "int app_main(void) {\n  int i;\n  for (i = 0; i < 2; ++i)\n    if (sem_create(&thr[i].sem) != 0) return lib_helper(i);\n  return ext_fn();\n}\n");
    write(C / "src/ext.c", "int ext_fn(void) { return 3; }\n");
    std::string cdb = "[\n";
    for (const char* n : {"core_main", "core_api", "lib", "app"}) {
      const std::string src = (C / "src" / (std::string(n) + ".c")).string();
      const std::string cmd = cc + " -std=c99 -fPIC -c " + src + " -o obj/" + n + ".o";
      KZ_CHECK_EQ(run("cd " + q(C.string()) + " && " + cmd, &out), 0);
      cdb += std::string(cdb.size() > 2 ? ",\n" : "") + "{\"directory\": \"" + C.string() + "\", \"command\": \"" + cmd + "\", \"file\": \"" + src + "\"}";
    }
    write(C / "compile_commands.json", cdb + "\n]\n");
    // the executable exports its symbols (-rdynamic); the modules are shared objects
    write(C / "link_core.txt", cc + " -rdynamic -o core obj/core_main.o obj/core_api.o\n");
    write(C / "mods/mylib/link.txt", cc + " -shared -o mylib.so ../../obj/lib.o\n");
    write(C / "mods/myapp/link.txt", cc + " -shared -o myapp.so ../../obj/app.o\n");
    KZ_CHECK_EQ(run("cd " + q(C.string()) + " && " + read(C / "link_core.txt"), &out), 0);
    KZ_CHECK_EQ(run("cd " + q((C / "mods/mylib").string()) + " && " + read(C / "mods/mylib/link.txt"), &out), 0);
    KZ_CHECK_EQ(run("cd " + q((C / "mods/myapp").string()) + " && " + read(C / "mods/myapp/link.txt"), &out), 0);
    KZ_CHECK_EQ(run(q(cc) + " -shared -fPIC -o " + q((C / "cf/extlib.so").string()) + " " + q((C / "src/ext.c").string()), &out), 0);
    fs::copy_file(C / "mods/mylib/mylib.so", C / "cf/mylib.so");
    fs::copy_file(C / "mods/myapp/myapp.so", C / "cf/myapp.so");
    const std::string script_ok = "LIB, extlib, NULL, EXT_LIB, 0;\nLIB, mylib, NULL, MY_LIB, 0;\nAPP, myapp, app_main, MY_APP, 50;\n";
    write(C / "cf/start.scr", script_ok);
    write(C / "recipe.kzb", R"(kzb 1;
name "closure";
link "link_core.txt";
binary "core";
module "mylib" link "mods/mylib/link.txt";
module "myapp" link "mods/myapp/link.txt";
module "unused" link "mods/unused/link.txt";
startup_script {
  reset POWERON;
  rule POWERON nonvolatile;
  file nonvolatile const "STARTUP_FILE" in "src/core_main.c";
  mount "/cf" "cf";
  entry library "LIB" global;
  entry app "APP" local;
  module_file field 1 path const "NONVOL_MOUNT" extension const "MODULE_EXT" in "src/core_main.c";
}
)");
    for (const fs::directory_entry& e : fs::directory_iterator(profile))
      if (e.path().extension() == ".kzp") fs::copy_file(e.path(), C / "profile" / e.path().filename());
    const std::string cbasis = q(tool) + " basis --profile " + q((C / "profile").string()) + " --compile-db " + q((C / "compile_commands.json").string()) + " --build-dir " + q(C.string()) +
                               " --bundle-root " + q(C.string()) + " --recipe " + q((C / "recipe.kzb").string()) + " --text -o ";
    const std::string cb = (C / "basis.kz").string(), cf = (C / "facts.kz").string();
    KZ_CHECK_EQ(run(cbasis + q(cb), &out), 0);
    KZ_CHECK(has(out, "translation units          4"));
    KZ_CHECK(has(out, "modules in the closure     2 (units 2), loaded outside the closure 1"));
    KZ_CHECK(has(out, "(bound otherwise by the loader 0, names defined twice 0)"));
    KZ_CHECK(has(out, "module[extlib] = library (startup.entry[0], symbols global): /cf/extlib.so -> cf/extlib.so; outside the closure"));
    KZ_CHECK(has(out, "module[myapp] = app (startup.entry[2], symbols local): /cf/myapp.so -> cf/myapp.so  (unset)"));
    KZ_CHECK(has(out, "module[myapp].link.output_is_the_loaded_file = yes: mods/myapp/myapp.so"));
    KZ_CHECK(has(out, "module[myapp].bound[core] = sem_create"));
    KZ_CHECK(has(out, "module[myapp].bound[mylib] = lib_helper"));
    KZ_CHECK(has(out, "module[myapp].bound[extlib] = ext_fn"));
    KZ_CHECK(has(out, "module[unused] = not loaded by the selected startup script"));
    KZ_CHECK(has(out, "closure.name_binding = the loader's binding: each of the 2 references across units"));
    KZ_CHECK(has(out, "closure.units = 4 (2 of core, 2 of 2 modules"));
    // the units of a module carry where they come from
    kz::Container cbc;
    KZ_CHECK_OK(kz::Container::read_file(cb, &cbc));
    std::vector<kz_tu> tus;
    KZ_CHECK_OK(cbc.read_section(KZ_SEC_TUS, &tus));
    KZ_CHECK_EQ(tus.size(), size_t(4));
    bool app_member = false;
    for (const kz_tu& t : tus) {
      std::string m;
      cbc.string(t.member, &m);
      if (m == "module myapp (app, symbols local), named on its link command") app_member = true;
    }
    KZ_CHECK(app_member);
    // the extraction of the closure: the out-pointer store of the executable's body is a writer
    KZ_CHECK_EQ(run(q(tool) + " extract --profile " + q((C / "profile").string()) + " --basis " + q(cb) + " -o " + q(cf) + " --target Throttle.sem", &out), 0);
    KZ_CHECK(has(out, "translation units          4"));
    KZ_CHECK(has(out, "location Throttle.sem   writers 1 (direct 0, data function 0, out-pointer 1"));
    KZ_CHECK(has(out, "escape 0, cast 0, union 0, unknown effect 0  -> writer-set-complete"));
    // the library loaded after the application: the loader cannot bind lib_helper, the merged
    // program would; that is a deviation, not hidden
    write(C / "cf/start.scr", "LIB, extlib, NULL, EXT_LIB, 0;\nAPP, myapp, app_main, MY_APP, 50;\nLIB, mylib, NULL, MY_LIB, 0;\n");
    KZ_CHECK_EQ(run(cbasis + q((C / "basis_late.kz").string()), &out), 0);
    KZ_CHECK(has(out, "(bound otherwise by the loader 1, names defined twice 0)"));
    KZ_CHECK(has(out, "myapp: lib_helper is defined in the closure by mylib, the loader binds it to nothing  (deviation)"));
    // the file the script loads is not what the link wrote: the module is not linked
    write(C / "cf/start.scr", script_ok);
    {
      std::ofstream(C / "cf/myapp.so", std::ios::binary | std::ios::app) << '\0';
    }
    KZ_CHECK_EQ(run(cbasis + q((C / "basis_other.kz").string()), &out), 0);
    KZ_CHECK(has(out, "module[myapp].link.output_is_the_loaded_file = no:"));
    KZ_CHECK(has(out, "module myapp not linked: the bytes the link wrote are not the bytes the script loads"));
    KZ_CHECK(has(out, "translation units          3"));
  }

  // ---- the profile-free mode (basis/extract --no-profile) --------------------------------------------
  // No profile and no vocabulary: the built-in profile, the P0 table, the build and the startup-script
  // block of the recipe.  The executable starts a task through a wrapper of pthread_create whose entry
  // is stored in a global and called by a trampoline; the loader looks the application's entry up by
  // name (dlsym) and passes it on as an integer converted to a function pointer.
  {
    KZ_CHECK_EQ(run(q(tool) + " extract --no-profile --profile " + q(profile) + " --source x.c -o x", &out), 2);
    KZ_CHECK(has(out, "--profile and --no-profile exclude each other"));
    for (const char* o : {"--target T.f", "--delay-op f", "--lock-op f", "--unlock-op f", "--spawn-op f", "--terminal-op f", "--poll-fn f"}) {
      KZ_CHECK_EQ(run(q(tool) + " extract --no-profile --source x.c -o x " + o, &out), 2);
      KZ_CHECK(has(out, "--no-profile takes no vocabulary from the command line"));
    }
    KZ_CHECK_EQ(run(q(tool) + " basis --no-profile --profile " + q(profile) + " --compile-db a --build-dir b --bundle-root c -o x", &out), 2);
    KZ_CHECK_EQ(run(q(tool) + " basis --no-profile -o x", &out), 2);
    const fs::path N = work / "noprof";
    for (const char* d : {"src", "obj", "cf", "mods/myapp"}) fs::create_directories(N / d);
    write(N / "src/core_main.c",
          "#define NONVOL_MOUNT \"/cf\"\n#define MODULE_EXT \".so\"\n#define STARTUP_FILE \"/cf/start.scr\"\n"
          "const char *nonvol_mount(void) { return NONVOL_MOUNT; }\nconst char *module_ext(void) { return MODULE_EXT; }\n"
          "const char *startup_file(void) { return STARTUP_FILE; }\nint load_app(const char *symbol);\nint main(void) { return load_app(\"app_main\"); }\n");
    write(N / "src/core_ldr.c",
          "typedef unsigned long thr_t;\nint pthread_create(thr_t *t, const void *attr, void *(*start)(void *), void *arg);\n"
          "void *dlsym(void *handle, const char *name);\ntypedef void (*entry_t)(void);\nentry_t task_entry;\n"
          "static void *trampoline(void *arg) { (void)arg; task_entry(); return 0; }\n"
          "int task_create(entry_t e) { thr_t t; task_entry = e; return pthread_create(&t, 0, trampoline, 0); }\n"
          "unsigned long lookup(const char *name) { return (unsigned long)dlsym((void *)0, name); }\n"
          "int load_app(const char *symbol) { return task_create((entry_t)lookup(symbol)); }\n");
    write(N / "src/app.c", "void app_main(void) {}\n");
    std::string cdb = "[\n";
    for (const char* n : {"core_main", "core_ldr", "app"}) {
      const std::string src = (N / "src" / (std::string(n) + ".c")).string();
      const std::string cmd = cc + " -std=c99 -fPIC -c " + src + " -o obj/" + n + ".o";
      KZ_CHECK_EQ(run("cd " + q(N.string()) + " && " + cmd, &out), 0);
      cdb += std::string(cdb.size() > 2 ? ",\n" : "") + "{\"directory\": \"" + N.string() + "\", \"command\": \"" + cmd + "\", \"file\": \"" + src + "\"}";
    }
    write(N / "compile_commands.json", cdb + "\n]\n");
    write(N / "link_core.txt", cc + " -rdynamic -o core obj/core_main.o obj/core_ldr.o\n");
    write(N / "mods/myapp/link.txt", cc + " -shared -o myapp.so ../../obj/app.o\n");
    KZ_CHECK_EQ(run("cd " + q(N.string()) + " && " + read(N / "link_core.txt"), &out), 0);
    KZ_CHECK_EQ(run("cd " + q((N / "mods/myapp").string()) + " && " + read(N / "mods/myapp/link.txt"), &out), 0);
    fs::copy_file(N / "mods/myapp/myapp.so", N / "cf/myapp.so");
    write(N / "cf/start.scr", "APP, myapp, app_main, MY_APP, 50;\n");
    write(N / "recipe.kzb", R"(kzb 1;
name "profile-free closure";
link "link_core.txt";
binary "core";
module "myapp" link "mods/myapp/link.txt";
startup_script {
  reset POWERON;
  rule POWERON nonvolatile;
  file nonvolatile const "STARTUP_FILE" in "src/core_main.c";
  mount "/cf" "cf";
  entry library "LIB" global;
  entry app "APP" local;
  module_file field 1 path const "NONVOL_MOUNT" extension const "MODULE_EXT" in "src/core_main.c";
}
)");
    const std::string nb = (N / "basis.kz").string(), nf = (N / "facts.kz").string(), nf2 = (N / "facts2.kz").string();
    KZ_CHECK_EQ(run(q(tool) + " basis --no-profile --compile-db " + q((N / "compile_commands.json").string()) + " --build-dir " + q(N.string()) + " --bundle-root " + q(N.string()) +
                        " --recipe " + q((N / "recipe.kzb").string()) + " --text -o " + q(nb),
                    &out),
                0);
    KZ_CHECK(has(out, "profile.mode = profile-free (--no-profile): the built-in profile phi 1; profile \"kz-static-builtin-minimal\";"));
    KZ_CHECK(has(out, "startup.entry[0] = app myapp, app_main, MY_APP, 50"));
    KZ_CHECK_EQ(run(q(tool) + " extract --no-profile --basis " + q(nb) + " -o " + q(nf), &out), 0);
    KZ_CHECK(has(out, "profile-free mode            built-in profile, "));
    KZ_CHECK(has(out, "startup-script entries     spawn calls with a data entry that reach the symbol lookup 1, rows 1 (in the closure 1)"));
    KZ_CHECK(has(out, "spawn operations (F7)      entry-argument 0, called-by-entry 1, fixed-entry 0"));
    KZ_CHECK_EQ(run(q(tool) + " extract --no-profile --basis " + q(nb) + " -o " + q(nf2), &out), 0);
    KZ_CHECK_EQ(read(nf), read(nf2));
    // the facts carry the built-in profile's hash, as the basis does, and record the mode
    kz::Container nbc, nfc;
    KZ_CHECK_OK(kz::Container::read_file(nb, &nbc));
    KZ_CHECK_OK(kz::Container::read_file(nf, &nfc));
    KZ_CHECK(nfc.profile_hash() == nbc.profile_hash());
    KZ_CHECK_EQ(run(q(tool) + " show " + q(nf) + " --section basis --section spawnwrap --section targets --section p0", &out), 0);
    KZ_CHECK(has(out, "extract.mode = profile-free (--no-profile)"));
    KZ_CHECK(has(out, "extract.builtin_profile = phi 1; profile \"kz-static-builtin-minimal\"; (profile hash in the header)"));
    KZ_CHECK(has(out, "p0.pthread_create = thread-create; POSIX.1-2017 / glibc 2.39; source glibc 2.39 nptl/pthread_create.c:624"));
    KZ_CHECK(has(out, "task_create called-by-entry param 0 {vocabulary}"));
    KZ_CHECK(has(out, "load_app script-entry param 0 {}"));
    KZ_CHECK(has(out, "entry app_main"));
    KZ_CHECK(has(out, "chain load_app -> lookup -> dlsym"));
    KZ_CHECK(has(out, "via startup.entry[0] = app myapp, app_main, MY_APP, 50"));
    KZ_CHECK(has(out, "in load_app spawn-entry -> app_main via "));
    KZ_CHECK(has(out, "startup-script entries through the symbol lookup of 1"));
    KZ_CHECK(has(out, "dlsym symbol-lookup (dynamic loading) calls=1 {}"));
    // a facts container of the profile-driven mode has none of the three sections, and its view shows none
    KZ_CHECK_EQ(run(q(tool) + " extract --profile " + q(profile) + " --basis " + q(nb) + " -o " + q((N / "facts_prof.kz").string()), &out), 0);
    KZ_CHECK_EQ(run(q(tool) + " show " + q((N / "facts_prof.kz").string()), &out), 0);
    KZ_CHECK(!has(out, "[p0]") && !has(out, "[derived]") && !has(out, "[spawnwrap]") && !has(out, "extract.mode"));
    kz::Container pfc;
    KZ_CHECK_OK(kz::Container::read_file((N / "facts_prof.kz").string(), &pfc));
    KZ_CHECK(pfc.find_section(KZ_SEC_P0_TABLE) == nullptr && pfc.find_section(KZ_SEC_OP_DERIV) == nullptr && pfc.find_section(KZ_SEC_SPAWN_WRAP) == nullptr);
    KZ_CHECK(nfc.find_section(KZ_SEC_P0_TABLE) != nullptr && nfc.find_section(KZ_SEC_OP_DERIV) != nullptr && nfc.find_section(KZ_SEC_SPAWN_WRAP) != nullptr);
  }

  fs::remove_all(work);
  return kztest::finish("test_static_cli");
}

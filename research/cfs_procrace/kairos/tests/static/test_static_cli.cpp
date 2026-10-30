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

  // ---- the extended facts F1-F3 (READS, RETURNS, RET_CONSTS, LOOP_EXITS; kz_facts.h revision 6) ---------
  // Off in the profile-driven mode unless --extended-facts is given (its facts are then written as before), always
  // on in the profile-free mode.  With them on, the records of the earlier sections are the same: the string pool is
  // canonical, so string offsets move with the new strings, but the view and the decoded records do not change, and
  // the rows the earlier sections use are a prefix of FACT_EXPRS.
  {
    const fs::path X = work / "extended";
    fs::create_directories(X);
    const std::string src = fixtures + "/flowfacts.c";
    const std::string fa = (X / "plain.kz").string(), fb = (X / "ext.kz").string(), fc = (X / "np.kz").string(), fd = (X / "np2.kz").string();
    auto ex = [&](const std::string& mode, const std::string& dst) {
      return run(q(tool) + " extract " + mode + " --source " + q(src) + " --root " + q(fixtures) + " -o " + q(dst) + " -- -std=c99 -pedantic", &out);
    };
    KZ_CHECK_EQ(ex("--profile " + q(profile), fa), 0);
    KZ_CHECK_EQ(ex("--profile " + q(profile) + " --extended-facts", fb), 0);
    KZ_CHECK_EQ(ex("--no-profile", fc), 0);
    KZ_CHECK_EQ(ex("--no-profile --extended-facts", fd), 0);
    KZ_CHECK_EQ(read(fc), read(fd));  // implied by --no-profile
    kz::Container ca, cb, cn;
    KZ_CHECK_OK(kz::Container::read_file(fa, &ca));
    KZ_CHECK_OK(kz::Container::read_file(fb, &cb));
    KZ_CHECK_OK(kz::Container::read_file(fc, &cn));
    KZ_CHECK_OK(cb.verify_content());
    KZ_CHECK_OK(cn.verify_content());
    for (const uint32_t sec : {static_cast<uint32_t>(KZ_SEC_READS), static_cast<uint32_t>(KZ_SEC_RETURNS), static_cast<uint32_t>(KZ_SEC_RET_CONSTS),
                               static_cast<uint32_t>(KZ_SEC_LOOP_EXITS)}) {
      KZ_CHECK(ca.find_section(sec) == nullptr);
      KZ_CHECK(cb.find_section(sec) != nullptr);
      KZ_CHECK(cn.find_section(sec) != nullptr);
    }
    const std::string earlier =
        " --section files --section functions --section calls --section derives --section loops --section iters --section order --section init --section consts"
        " --section writers --section atype --section locs --section targets --section spawnparams --section image --section terminals --section outcalls --section fwrites";
    std::string va, vb;
    KZ_CHECK_EQ(run(q(tool) + " show " + q(fa) + earlier, &va), 0);
    KZ_CHECK_EQ(run(q(tool) + " show " + q(fb) + earlier, &vb), 0);
    KZ_CHECK_EQ(va, vb);
    KZ_CHECK(has(va, "[loops] ") && !has(va, "[reads]"));
    std::vector<kz_fact_expr> xa, xb;
    KZ_CHECK_OK(ca.read_section(KZ_SEC_FACT_EXPRS, &xa));
    KZ_CHECK_OK(cb.read_section(KZ_SEC_FACT_EXPRS, &xb));
    KZ_CHECK(xb.size() > xa.size());
    for (size_t i = 0; i < xa.size() && i < xb.size(); ++i) {
      std::string na, nb2;
      ca.string(xa[i].name, &na);
      cb.string(xb[i].name, &nb2);
      KZ_CHECK(xa[i].op == xb[i].op && xa[i].flags == xb[i].flags && xa[i].a == xb[i].a && xa[i].b == xb[i].b && xa[i].value == xb[i].value && na == nb2);
    }
    KZ_CHECK_EQ(run(q(tool) + " show " + q(fb) + " --section basis", &out), 0);
    KZ_CHECK(has(out, "extract.extended_facts = F1 READS, F2 RETURNS and RET_CONSTS, F3 LOOP_EXITS (kz_facts.h revision 6)"));
    KZ_CHECK_EQ(run(q(tool) + " show " + q(fa) + " --section basis", &out), 0);
    KZ_CHECK(!has(out, "extract.extended_facts") && !has(out, "count.reads"));
    // what the facts say about the fixture (both modes read the same bodies)
    std::string vc;
    KZ_CHECK_EQ(run(q(tool) + " show " + q(fc) + " --section reads --section returns --section retconsts --section loopexits", &vc), 0);
    KZ_CHECK_EQ(run(q(tool) + " show " + q(fb) + " --section reads --section returns --section retconsts --section loopexits", &out), 0);
    KZ_CHECK_EQ(vc, out);
    // a validity branch on an indexed record, and an out-store of another field of it
    KZ_CHECK(has(vc, "fx_owner_of reads field FxRec.used at flowfacts.c:38:65 root=global:fx_store use={branch} {} [fx_store.table[idx].used] index=[local:idx] ctx=(eq field:FxRec.used 1)"));
    KZ_CHECK(has(vc, "fx_owner_of reads field FxRec.owner at flowfacts.c:39:32 root=global:fx_store use={out-store} {} [fx_store.table[idx].owner] index=[local:idx] ctx=(deref param:out#1)"));
    // the subscript local with a single definition is written as that definition's value
    KZ_CHECK(has(vc, "fx_my_owner reads field FxRec.used at flowfacts.c:132:28 root=global:fx_store use={branch} {index-unfolded} [fx_store.table[slot].used] index=[(call-result fx_self_index)]"));
    // the contents of an array field passed to a call whose result decides a branch
    KZ_CHECK(has(vc, "use={branch,call-argument} {array-contents} [fx_store.table[i].name]"));
    // a read through a pointer parameter; a function pointer read as the callee
    KZ_CHECK(has(vc, "fx_is_used reads field FxRec.used at flowfacts.c:56:44 root=param:r#0 use={return} {through-pointer}"));
    KZ_CHECK(has(vc, "fx_run_hook reads field FxStore.hook at flowfacts.c:67:19 root=global:fx_store use={callee}"));
    // the returned local resolved to its reaching definitions; the constants and when each is returned
    KZ_CHECK(has(vc, "fx_owner_of returns at flowfacts.c:42:3 alt 0/2 const -3 [FX_ERR_ID] {from-definition,redefined-later}"));
    KZ_CHECK(has(vc, "fx_classify returns -3 [FX_ERR_ID] rows=1 {from-definition,only-constants} when=(lt param:id#0 0)"));
    KZ_CHECK(has(vc, "fx_classify returns -17 [FX_ERR_NOT_FOUND] rows=1 {from-definition,only-constants} when=(and (not (lt param:id#0 0)) (ge param:id#0 8))"));
    KZ_CHECK(has(vc, "fx_count_plus returns at flowfacts.c:75:3 alt 0/1 expression (add field:FxStore.count param:k#0) [c + k] {unfolded,from-definition}"));
    KZ_CHECK(has(vc, "fx_index_of returns at flowfacts.c:123:3 alt 0/1 local local:idx [idx] {}"));  // its address is taken
    // every loop has its exits, a loop without a delay call included
    KZ_CHECK(has(vc, "in fx_find_by_name exits=2 {} exit=(or (not (lt local:i 8))"));
    KZ_CHECK(has(vc, "exit 1 break at flowfacts.c:49:82 {}"));
    KZ_CHECK(has(vc, "in fx_spin exits=1 {no-exit} exit=0 mods=[global:fx_level]"));
    KZ_CHECK(has(vc, "exit 0 goto at flowfacts.c:108:35 {}"));
  }

  // ---- the extended facts F4 and F6 (ARGFLOW, PARAM_FLOW, PTR_FIELDS, GLOBAL_IMAGES; kz_facts.h revision 7) ----------
  // Written with the facts of revision 6 and after them: off in the profile-driven mode without --extended-facts, the
  // earlier sections unchanged with it (view and FACT_EXPRS prefix), the same rows in both modes.  The bounds of the
  // PARAM_FLOW composition and the image budget are tool parameters recorded as unspecified_by_reference.
  {
    const fs::path X = work / "argflow";
    fs::create_directories(X);
    const std::string src = fixtures + "/argflow.c";
    auto ex = [&](const std::string& mode, const std::string& dst) {
      return run(q(tool) + " extract " + mode + " --source " + q(src) + " --root " + q(fixtures) + " -o " + q(dst) + " -- -std=c99 -pedantic", &out);
    };
    const std::string fa = (X / "plain.kz").string(), fb = (X / "ext.kz").string(), fc = (X / "np.kz").string(), fd = (X / "np2.kz").string();
    KZ_CHECK_EQ(ex("--profile " + q(profile), fa), 0);
    KZ_CHECK_EQ(ex("--profile " + q(profile) + " --extended-facts", fb), 0);
    KZ_CHECK_EQ(ex("--no-profile", fc), 0);
    KZ_CHECK_EQ(ex("--no-profile", fd), 0);
    KZ_CHECK_EQ(read(fc), read(fd));
    kz::Container ca, cb, cn;
    KZ_CHECK_OK(kz::Container::read_file(fa, &ca));
    KZ_CHECK_OK(kz::Container::read_file(fb, &cb));
    KZ_CHECK_OK(kz::Container::read_file(fc, &cn));
    KZ_CHECK_OK(cb.verify_content());
    KZ_CHECK_OK(cn.verify_content());
    for (const uint32_t sec : {static_cast<uint32_t>(KZ_SEC_ARGFLOW), static_cast<uint32_t>(KZ_SEC_PARAM_FLOW), static_cast<uint32_t>(KZ_SEC_PTR_FIELDS),
                               static_cast<uint32_t>(KZ_SEC_GLOBAL_IMAGES)}) {
      KZ_CHECK(ca.find_section(sec) == nullptr);
      KZ_CHECK(cb.find_section(sec) != nullptr);
      KZ_CHECK(cn.find_section(sec) != nullptr);
    }
    const std::string earlier =
        " --section files --section functions --section calls --section derives --section loops --section iters --section order --section init --section consts"
        " --section writers --section atype --section locs --section targets --section spawnparams --section image --section terminals --section outcalls --section fwrites";
    std::string va, vb;
    KZ_CHECK_EQ(run(q(tool) + " show " + q(fa) + earlier, &va), 0);
    KZ_CHECK_EQ(run(q(tool) + " show " + q(fb) + earlier, &vb), 0);
    KZ_CHECK_EQ(va, vb);
    std::vector<kz_fact_expr> xa, xb;
    KZ_CHECK_OK(ca.read_section(KZ_SEC_FACT_EXPRS, &xa));
    KZ_CHECK_OK(cb.read_section(KZ_SEC_FACT_EXPRS, &xb));
    KZ_CHECK(xb.size() > xa.size());
    bool prefix = true;
    for (size_t i = 0; i < xa.size() && i < xb.size(); ++i)
      prefix = prefix && xa[i].op == xb[i].op && xa[i].flags == xb[i].flags && xa[i].a == xb[i].a && xa[i].b == xb[i].b && xa[i].value == xb[i].value;
    KZ_CHECK(prefix);
    const std::string newer = " --section argflow --section paramflow --section ptrfields --section gimages";
    std::string vn, vp;
    KZ_CHECK_EQ(run(q(tool) + " show " + q(fc) + newer, &vn), 0);
    KZ_CHECK_EQ(run(q(tool) + " show " + q(fb) + newer, &vp), 0);
    KZ_CHECK_EQ(vn, vp);
    // ARGFLOW: the copy through a local, the base of a pointer-field path, the address of a global array element
    KZ_CHECK(has(vn, "[(const void *)n] <- param root=name param=1 hops=1 {pointer-conversion} chain=(copy n param:name#1)"));
    KZ_CHECK(has(vn, "[fx_global.slots[index].name] <- field FxSlot.name root=FxGlobal.slots base=pointer-field object=fx_global hops=0 {through-pointer,array-contents} index=[local:index]"));
    KZ_CHECK(has(vn, "[&fx_table[i]] <- address <global>.fx_table root=fx_table base=global-array-element object=fx_table"));
    KZ_CHECK(has(vn, "[index] <- local root=index base=local hops=0 {} (multiple-definitions)"));
    // PARAM_FLOW: uses of depth 0, the composition through an indirect call to a comparison function without a body
    KZ_CHECK(has(vn, "fx_create#2 d0 comparison at argflow.c:71:7 {} [pos >= FX_NAMES] expr=(ge param:pos#2 4)"));
    KZ_CHECK(has(vn, "fx_create#1 d0 store at argflow.c:72:3 {} [fx_table[pos].name = name]"));
    KZ_CHECK(has(vn, "fx_lookup#1 d0 comparison at argflow.c:62:8 {truth-test} [n]"));
    KZ_CHECK(has(vn, "fx_lookup#0 d0 out-store at argflow.c:65:3"));
    KZ_CHECK(has(vn, "fx_find#1 d0 call-argument -> fx_name_match#0 at site 2 argflow.c:54:9 {may,indirect,result-tested,in-loop}"));
    KZ_CHECK(has(vn, "fx_lookup#1 d2 call-argument in fx_name_match at argflow.c:48:83"));
    KZ_CHECK(has(vn, "chain fx_lookup#1 -> fx_find#1 -> fx_name_match#0"));
    KZ_CHECK(has(vn, "fx_down#0 d1 comparison in fx_up"));
    KZ_CHECK(has(vn, "fx_dispatch#0 d0 comparison at argflow.c:120:11 {switch} [code]"));
    KZ_CHECK(has(vn, "fx_dispatch#0 d0 call-argument -> ?#0 at site 12 argflow.c:121:20 {indirect,unresolved} [code]"));
    // PTR_FIELDS: the address write after the record is cleared, the reads it resolves, the order, the elements and their values
    KZ_CHECK(has(vn, "0 address-write FxGlobal.slots at argflow.c:98:3 in fx_init root=fx_global -> fx_slots[0] [fx_slots] other-writes=0 {earlier-writes-overwritten}"));
    KZ_CHECK(has(vn, "in fx_setup_slots@argflow.c:87 read=3 root=fx_global resolved candidates=1 write=0 -> fx_slots path=.name index=local:index elements=0+1k (k<2) "
                     "{write-before-read,same-root,index-from-counted-loop}"));
    KZ_CHECK(has(vn, "fx_slots[0].name = string \"slot_zero\" at argflow.c:38:31 macro FX_SLOT_0_NAME spelled at argflow.c:13:24 {from-macro}"));
    KZ_CHECK(has(vn, "fx_slots[1].name = string \"slot_one\" at argflow.c:38:55 macro FX_SLOT_1_NAME spelled at argflow.c:15:24 {from-macro}"));
    KZ_CHECK(has(vn, "in fx_first_limit read=10 root=fx_global may candidates=1 write=0 -> fx_slots path=.limit index=0 elements=0+0k (k<1) {same-root,constant-index}"));
    KZ_CHECK(has(vn, "1 static-initializer FxGlobal.slots at argflow.c:41:22 root=fx_other -> fx_spares[0] [fx_spares] other-writes=2 {other-writes}"));
    KZ_CHECK(has(vn, "in fx_spare_limit read=12 root=fx_other may candidates=1 write=1 -> fx_spares path=.limit"));
    KZ_CHECK(has(vn, "read FxGlobal.cursor at argflow.c:114:28 in fx_cursor_limit read=14 root=fx_global needs-field-points-to candidates=0 {}"));
    // a string with a NUL byte: escaped in the view, which goes on to the end (the source text drops the backslash)
    KZ_CHECK(has(vn, "[\"0\"] <- string root=\"\\x00\" hops=0 {} chain=string:\\x00"));
    KZ_CHECK(has(vn, "[gimages] "));
    // GLOBAL_IMAGES: the objects reached, a zero-initialized one, the field written elsewhere
    KZ_CHECK(has(vn, "fx_global int 0 at argflow.c:40:10 {zero-initialized,object-written} reach={argument,pointer-field}"));
    KZ_CHECK(has(vn, "fx_slots[0].handle int 0 at argflow.c:38:50 {field-written} reach={argument,pointer-field} field=FxSlot.handle"));
    // the tool parameters: recorded, refused without the extended facts or with a value that is not a number
    KZ_CHECK_EQ(run(q(tool) + " show " + q(fc) + " --section basis", &out), 0);
    KZ_CHECK(has(out, "extract.extended_facts_f4_f6 = F4 ARGFLOW, PARAM_FLOW and PTR_FIELDS, F6 GLOBAL_IMAGES (kz_facts.h revision 7)"));
    KZ_CHECK(has(out, "param.argflow_depth_bound = none (calls a composed PARAM_FLOW row passes; a tool parameter no reference fixes)  (unspecified_by_reference)"));
    KZ_CHECK(has(out, "param.argflow_size_bound = none"));
    KZ_CHECK(has(out, "param.image_budget = 4096"));
    KZ_CHECK(has(out, "count.param_flow_cut = depth bound 0 parameters, size bound 0 parameters, uses left out 0"));
    KZ_CHECK_EQ(run(q(tool) + " show " + q(fa) + " --section basis", &out), 0);
    KZ_CHECK(!has(out, "extended_facts_f4_f6") && !has(out, "param.argflow"));
    KZ_CHECK_EQ(ex("--profile " + q(profile) + " --argflow-depth 1", (X / "bad.kz").string()), 2);
    KZ_CHECK(has(out, "needs the extended facts"));
    KZ_CHECK_EQ(ex("--no-profile --argflow-depth x", (X / "bad.kz").string()), 2);
    KZ_CHECK_EQ(ex("--no-profile --image-budget 0", (X / "bad.kz").string()), 2);
    // a depth bound of 1 cuts the composition of depth 2: an OPEN row, counted
    const std::string f1 = (X / "d1.kz").string(), f0 = (X / "s0.kz").string(), fi = (X / "img.kz").string();
    KZ_CHECK_EQ(ex("--no-profile --argflow-depth 1", f1), 0);
    KZ_CHECK_EQ(run(q(tool) + " show " + q(f1) + " --section paramflow --section basis", &out), 0);
    KZ_CHECK(has(out, "fx_lookup#1 d1 open {depth-bound} left out 1 [uses beyond the depth bound]"));
    KZ_CHECK(!has(out, "fx_lookup#1 d2 "));
    KZ_CHECK(has(out, "param.argflow_depth_bound = 1 "));
    KZ_CHECK(has(out, "count.param_flow_cut = depth bound 1 parameters, size bound 0 parameters, uses left out 1"));
    // a size bound of 0 keeps no composed row
    KZ_CHECK_EQ(ex("--no-profile --argflow-size 0", f0), 0);
    KZ_CHECK_EQ(run(q(tool) + " show " + q(f0) + " --section paramflow", &out), 0);
    KZ_CHECK(has(out, "fx_find#1 d0 open {size-bound} left out 1 [uses beyond the size bound]"));
    KZ_CHECK(!has(out, " d1 ") && !has(out, " d2 "));
    // an image budget of 1 leaf truncates the images of the tables
    KZ_CHECK_EQ(ex("--no-profile --image-budget 1", fi), 0);
    KZ_CHECK_EQ(run(q(tool) + " show " + q(fi) + " --section gimages", &out), 0);
    KZ_CHECK(has(out, "fx_slots[...] non-constant at argflow.c:38:8 {image-truncated} reach={argument,pointer-field}"));
  }

  // ---- F5 and the target-independent rules (LOCK_OPS, LOCK_IDS, LOCKSET, FIELD_STATUS, AWAIT_L1; kz_facts.h revision 8) -
  // Written with the extended facts: off in the profile-driven mode without --extended-facts, the earlier sections unchanged
  // with it, the same rows in both modes and whatever the targets.  The old rules of FACT_LOCS and FACT_LOOPS are unchanged.
  {
    const fs::path X = work / "lockset";
    fs::create_directories(X);
    const std::string src = fixtures + "/lockset.c";
    auto ex = [&](const std::string& mode, const std::string& dst) {
      return run(q(tool) + " extract " + mode + " --source " + q(src) + " --root " + q(fixtures) + " -o " + q(dst) + " -- -std=c99 -pedantic", &out);
    };
    const std::string fa = (X / "plain.kz").string(), fb = (X / "ext.kz").string(), fc = (X / "np.kz").string(), fd = (X / "np2.kz").string();
    const std::string ft = (X / "targets.kz").string(), fs1 = (X / "sibling.kz").string(), fs2 = (X / "alone.kz").string();
    KZ_CHECK_EQ(ex("--profile " + q(profile) + " --delay-op fx_delay", fa), 0);
    KZ_CHECK_EQ(ex("--profile " + q(profile) + " --delay-op fx_delay --extended-facts", fb), 0);
    KZ_CHECK_EQ(ex("--no-profile", fc), 0);
    KZ_CHECK_EQ(ex("--no-profile", fd), 0);
    KZ_CHECK_EQ(read(fc), read(fd));
    kz::Container ca, cb, cn;
    KZ_CHECK_OK(kz::Container::read_file(fa, &ca));
    KZ_CHECK_OK(kz::Container::read_file(fb, &cb));
    KZ_CHECK_OK(kz::Container::read_file(fc, &cn));
    KZ_CHECK_OK(cb.verify_content());
    KZ_CHECK_OK(cn.verify_content());
    for (const uint32_t sec : {static_cast<uint32_t>(KZ_SEC_LOCK_OPS), static_cast<uint32_t>(KZ_SEC_LOCK_IDS), static_cast<uint32_t>(KZ_SEC_LOCKSET),
                               static_cast<uint32_t>(KZ_SEC_FIELD_STATUS), static_cast<uint32_t>(KZ_SEC_AWAIT_L1)}) {
      KZ_CHECK(ca.find_section(sec) == nullptr);
      KZ_CHECK(cb.find_section(sec) != nullptr);
      KZ_CHECK(cn.find_section(sec) != nullptr);
    }
    const std::string earlier =
        " --section files --section functions --section calls --section derives --section loops --section iters --section order --section init --section consts"
        " --section writers --section atype --section locs --section targets --section spawnparams --section image --section terminals --section outcalls --section fwrites";
    std::string va, vb;
    KZ_CHECK_EQ(run(q(tool) + " show " + q(fa) + earlier, &va), 0);
    KZ_CHECK_EQ(run(q(tool) + " show " + q(fb) + earlier, &vb), 0);
    KZ_CHECK_EQ(va, vb);
    const std::string newer = " --section lockops --section lockids --section lockset --section fstatus --section awaits";
    std::string vn, vp;
    KZ_CHECK_EQ(run(q(tool) + " show " + q(fc) + newer, &vn), 0);
    KZ_CHECK_EQ(run(q(tool) + " show " + q(fb) + newer, &vp), 0);
    KZ_CHECK_EQ(vn, vp);
    // LOCK_OPS: an operation that holds the lock exactly when it returns 0, one that holds it on every return (under the
    // declared assumption that an acquisition succeeds), a try, a release; identities over the parameters
    KZ_CHECK(has(vn, "fx_take acquire result-conditional success=0 lock &fx_mtab[$0].id may=fx_mrec.id {vocabulary,assumes-success,parameter-identity}"));
    KZ_CHECK(has(vn, "fx_lock_shared acquire every-return lock &fx_mtab[fx_g.mutex].id may=fx_mrec.id {vocabulary,assumes-success}"));
    KZ_CHECK(has(vn, "chain fx_lock_shared -> fx_take -> pthread_mutex_lock"));
    KZ_CHECK(has(vn, "fx_try_take acquire try success=0 lock &fx_mtab[$0].id may=fx_mrec.id {parameter-identity}"));
    KZ_CHECK(has(vn, "fx_unlock_shared release every-return lock &fx_mtab[fx_g.mutex].id"));
    // LOCK_IDS: a handle written once by an out-pointer call in a function that runs once is a static instance; one
    // written by a function called twice is not
    KZ_CHECK(has(vn, "#0 &fx_mtab[fx_g.mutex].id closed static-instance {address-of-global,reads-value} may=fx_mrec.id value-of=fx_shared.mutex object=fx_g writer-instances=1"));
    KZ_CHECK(has(vn, "#1 &fx_mtab[fx_g.other].id closed not-static {reads-value} may=fx_mrec.id value-of=fx_shared.other object=fx_g writer-instances=1 (fx_shared.other: 1 writer "
                     "instance(s), not one that runs once)"));
    // LOCKSET: the spawn between two regions holds nothing, the writes in the second region hold the lock acquired there,
    // the failure branch of the acquisition holds nothing, a try holds on its success branch
    KZ_CHECK(has(vn, "call site 12 lockset.c:94:7 in fx_app_create held=0 {}"));
    KZ_CHECK(has(vn, "write fx_shared.count at lockset.c:97:3 in fx_app_create held=1 [#0 &fx_mtab[fx_g.mutex].id] acquired-at=[lockset.c:95:3] {}"));
    KZ_CHECK(has(vn, "call site 14 lockset.c:98:9 in fx_app_create held=1 [#0 &fx_mtab[fx_g.mutex].id] acquired-at=[lockset.c:95:3] {}"));
    KZ_CHECK(has(vn, "call site 4 lockset.c:71:11 in fx_lock_shared held=0 {failure-path}"));
    KZ_CHECK(has(vn, "read read 10 at lockset.c:116:14 in fx_try_reader held=1 [#0 &fx_mtab[fx_g.mutex].id] acquired-at=[lockset.c:115:7] {}"));
    KZ_CHECK(has(vn, "read read 7 at lockset.c:108:12 in fx_other_reader held=1 [#1 &fx_mtab[fx_g.other].id]"));
    // FIELD_STATUS: the sibling's escaped address exposes the type only under the type-level rule
    KZ_CHECK(has(vn, "fx_pair.a per-field=writer-set-complete type-level=DISABLED writers=1 (direct 1, data 0, out-pointer 0, init 1) instances=1 one-instance-once hits: "
                     "escape 0, cast 0, union 0, unknown 0, other fields 1; untyped sites 1 {sibling-exposed,untyped-in-program} by main"));
    KZ_CHECK(has(vn, "fx_pair.b per-field=DISABLED type-level=DISABLED"));
    KZ_CHECK(has(vn, "fx_shared.count per-field=writer-set-complete type-level=writer-set-complete writers=2 (direct 2, data 0, out-pointer 0, init 1) instances=2 not-once"));
    // AWAIT_L1: the predicate reads a field through the function it calls; the reads hold one lock acquired at one call
    KZ_CHECK(has(vn, "in fx_wait_state relevant=[field:fx_app.state, field:fx_app.used, field:fx_shared.apps] via-callee=[fx_app.used] L1(ii)=one-lock-region lock #0 "
                     "&fx_mtab[fx_g.mutex].id L1(i) per-field=holds type-level=holds {via-callee,lock-static}"));
    KZ_CHECK(has(vn, "in fx_spin_count relevant=[field:fx_shared.count] L1(ii)=one-lock-region lock #0 &fx_mtab[fx_g.mutex].id L1(i) per-field=fails type-level=fails"));
    KZ_CHECK(has(vn, "star-closed: L1 (i) fails: the body writes fx_shared.count by fx_spin_count at lockset.c:154:5"));
    // the BASIS records: the assumptions and the counts
    KZ_CHECK_EQ(run(q(tool) + " show " + q(fc) + " --section basis", &out), 0);
    KZ_CHECK(has(out, "extract.extended_facts_f5 = F5 LOCK_OPS, LOCK_IDS and LOCKSET, FIELD_STATUS, AWAIT_L1 (kz_facts.h revision 8)"));
    KZ_CHECK(has(out, "assume.lock_acquisition_succeeds = an acquisition that is not a try succeeds"));
    KZ_CHECK(has(out, "assume.no_cross_member_pointer_arithmetic = "));
    KZ_CHECK(has(out, "count.field_status = 12 fields (targets of the completeness pass 12)"));
    KZ_CHECK(has(out, "extract.lock_vocabulary = "));
    KZ_CHECK_EQ(run(q(tool) + " show " + q(fa) + " --section basis", &out), 0);
    KZ_CHECK(!has(out, "extended_facts_f5") && !has(out, "assume.lock_acquisition_succeeds"));
    // whatever the targets: with the sibling as a target the old rule of FACT_LOCS disables the field, FIELD_STATUS does not
    // change; with the field alone the old rule calls it complete
    KZ_CHECK_EQ(ex("--profile " + q(profile) + " --delay-op fx_delay --extended-facts --target fx_pair.a --target fx_pair.b --target fx_shared.count", ft), 0);
    std::string vt;
    KZ_CHECK_EQ(run(q(tool) + " show " + q(ft) + " --section fstatus --section lockids --section awaits", &vt), 0);
    std::string vf;
    KZ_CHECK_EQ(run(q(tool) + " show " + q(fc) + " --section fstatus --section lockids --section awaits", &vf), 0);
    auto nosite = [](std::string t) {
      std::string o;
      for (size_t i = 0; i < t.size();) {
        if (t.compare(i, 5, "site ") == 0) {
          size_t j = i + 5;
          while (j < t.size() && t[j] >= '0' && t[j] <= '9') ++j;
          if (j > i + 5) {
            i = j;
            continue;
          }
        }
        o += t[i++];
      }
      return o;
    };
    KZ_CHECK_EQ(nosite(vt), nosite(vf));
    KZ_CHECK_EQ(ex("--profile " + q(profile) + " --target fx_pair.a --target fx_pair.b", fs1), 0);
    KZ_CHECK(has(out, "location fx_pair.a   writers 1") && has(out, "-> DISABLED"));
    KZ_CHECK_EQ(ex("--profile " + q(profile) + " --target fx_pair.a", fs2), 0);
    KZ_CHECK(has(out, "location fx_pair.a   writers 1 (direct 1, data function 0, out-pointer 0, initial values 1), A-TYPE hits: untyped 0, escape 0, cast 0, union 0, unknown "
                      "effect 0  -> writer-set-complete"));
    // the profile-free second pass has the derived lock and unlock operations in its vocabulary
    KZ_CHECK_EQ(run(q(tool) + " show " + q(fc) + " --section calls", &out), 0);
    KZ_CHECK(has(out, "lockset.c:82:3 fx_get_id -> fx_lock_shared direct {intra-tu,lock-op}"));
    KZ_CHECK(has(out, "lockset.c:84:3 fx_get_id -> fx_unlock_shared direct {intra-tu,unlock-op}"));
  }

  // ---- the rule targets and `kz-static relations` (relations.c) -----------------------------------------
  {
    const fs::path X = work / "relations";
    fs::create_directories(X);
    const std::string src = fixtures + "/relations.c";
    auto ex = [&](const std::string& mode, const fs::path& o) {
      return run(q(tool) + " extract " + mode + " --source " + q(src) + " --root " + q(fixtures) + " -o " + q(o.string()) + " -- -std=c99 -pedantic", &out);
    };
    // --targets-rule: the profile-free mode only, and R-T1 / R-T2 only
    KZ_CHECK_EQ(ex("--profile " + q(profile) + " --targets-rule R-T1", X / "x.kz"), 2);
    KZ_CHECK(has(out, "--targets-rule chooses the targets of the profile-free mode"));
    KZ_CHECK_EQ(ex("--no-profile --targets-rule R-T3", X / "x.kz"), 2);
    KZ_CHECK(has(out, "--targets-rule expects R-T1, R-T2 or R-T1,R-T2"));
    KZ_CHECK_EQ(ex("--no-profile --targets-rule R-T2,R-T1", X / "both.kz"), 0);
    KZ_CHECK(has(out, "rule targets               7 (R-T1 5, R-T2 3, both 1;"));
    // the rule targets are recorded as rule targets, not as hand-entered inputs
    KZ_CHECK_EQ(run(q(tool) + " show " + q((X / "both.kz").string()) + " --section basis", &out), 0);
    KZ_CHECK(has(out, "target.Service.self_id = rule R-T1  (unset)"));
    KZ_CHECK(has(out, "target.TaskRec.registered = rule R-T2  (unset)"));
    KZ_CHECK(has(out, "extract.targets_rule = R-T1,R-T2 (target rules revision 1)"));
    KZ_CHECK(!has(out, "declared-assumption)\n  target."));
    // only R-T1: the field read in a branch only is no target
    KZ_CHECK_EQ(ex("--no-profile --targets-rule R-T1", X / "t1.kz"), 0);
    KZ_CHECK_EQ(run(q(tool) + " show " + q((X / "t1.kz").string()) + " --section basis", &out), 0);
    KZ_CHECK(has(out, "target.Service.self_id = rule R-T1"));
    KZ_CHECK(!has(out, "target.TaskRec.registered"));
    // without the option the profile-free facts have no targets, as before
    KZ_CHECK_EQ(ex("--no-profile", X / "none.kz"), 0);
    KZ_CHECK(!has(out, "rule targets"));
    KZ_CHECK_EQ(run(q(tool) + " show " + q((X / "none.kz").string()) + " --section writers", &out), 0);
    KZ_CHECK(has(out, "[writers] 0"));

    // relations: usage and inputs
    KZ_CHECK_EQ(run(q(tool) + " relations", &out), 2);
    KZ_CHECK_EQ(run(q(tool) + " relations a b", &out), 2);
    KZ_CHECK_EQ(run(q(tool) + " relations " + q((X / "both.kz").string()) + " --loc Slot.name", &out), 2);
    KZ_CHECK(has(out, "unknown option"));
    KZ_CHECK_EQ(run(q(tool) + " relations " + q((work / "garbage.kz").string()), &out), 1);
    // facts without the extended facts: refused with the reason
    KZ_CHECK_EQ(run(q(tool) + " relations " + q(facts), &out), 1);
    KZ_CHECK(has(out, "the facts have no extended facts"));
    // -o writes what stdout would show; two runs give the same bytes
    std::string a, b;
    KZ_CHECK_EQ(run(q(tool) + " relations " + q((X / "both.kz").string()), &a), 0);
    KZ_CHECK_EQ(run(q(tool) + " relations " + q((X / "both.kz").string()) + " -o " + q((X / "rel.txt").string()), &out), 0);
    KZ_CHECK_EQ(read(X / "rel.txt"), a);
    KZ_CHECK_EQ(run(q(tool) + " relations " + q((X / "both.kz").string()), &b), 0);
    KZ_CHECK_EQ(a, b);
    // the rows: every column, the layers, no case layer in the profile-free mode
    KZ_CHECK(has(a, "mode profile-free"));
    KZ_CHECK(has(a, "  namespace Slot.name (record type Slot; tables slots)"));
    KZ_CHECK(has(a, "      key-parameter functions: lookups slot_find#0; creators slot_create#0"));
    KZ_CHECK(has(a, "      active test: Slot.used == 1 is in use"));
    KZ_CHECK(has(a, "not a pair: probe-create"));
    KZ_CHECK(has(a, "creator: same key: relations.c:"));
    KZ_CHECK(has(a, "key: read through the pointer field Worker.cfg -> configs@relations.c:"));
    KZ_CHECK(has(a, "image configs[0].name = \"alpha\""));
    KZ_CHECK(has(a, "  B1 Service.self_id -> notify#0 sites=2 callee-check=yes"));
    KZ_CHECK(has(a, "      callee-check: notify returns -5 [ERR_ILLEGAL] when (eq param:id#0 0)"));
    KZ_CHECK(has(a, "      handling: discarded 2, used 0, tested 0"));
    KZ_CHECK(has(a, "      flip: static-possible: a written constant and the initial value give different results of the callee check"));
    KZ_CHECK(has(a, "in task_spawn -> pthread_create; field TaskRec.registered"));
    KZ_CHECK(has(a, "read subscript: the reader's own identity: task_self -> pthread_getspecific (P0 thread-specific-get)"));
    KZ_CHECK(has(a, "feasibility: possible: lock window: #0 &table_mutex"));
    KZ_CHECK(has(a, "feasibility: proven-impossible: lock #0 &table_mutex"));
    KZ_CHECK(has(a, "      run-order: no record given"));
    KZ_CHECK(has(a, "      violation: unconfirmed: RV2 §5.4"));
    KZ_CHECK(has(a, "      layers: B P0 R\n"));
    KZ_CHECK(!has(a, "case:profile"));
    // the profile-driven mode with the extended facts: the case layer is named in every row
    KZ_CHECK_EQ(ex("--profile " + q(profile) + " --extended-facts", X / "prof.kz"), 0);
    KZ_CHECK_EQ(run(q(tool) + " relations " + q((X / "prof.kz").string()), &out), 0);
    KZ_CHECK(has(out, "mode profile-driven"));
    KZ_CHECK(has(out, "case:profile("));
    // a command-line vocabulary is named as hand-entered (G0 or other names)
    KZ_CHECK_EQ(ex("--profile " + q(profile) + " --extended-facts --delay-op fx_none", X / "voc.kz"), 0);
    KZ_CHECK_EQ(run(q(tool) + " relations " + q((X / "voc.kz").string()), &out), 0);
    KZ_CHECK(has(out, "hand-entered:command-line(1 items; G0 or other names, not automatic)"));
    // the P0 tables of revision 2: their scope is recorded and printed
    KZ_CHECK_EQ(run(q(tool) + " show " + q((X / "both.kz").string()) + " --section basis --section p0", &out), 0);
    KZ_CHECK(has(out, "extract.p0_table = P0 table revision 2 (2026-10-30), 72 entries"));
    KZ_CHECK(has(out, "extract.p0_table_scope = families listed whole: "));
    KZ_CHECK(has(out, "[p0] 72"));
    KZ_CHECK(has(out, "  pthread_key_create thread-specific-key-create (thread-specific data) out=0 entry=1"));
    KZ_CHECK(has(out, "  thrd_create thread-create (thread creation) out=0 entry=1 arg=2"));
    KZ_CHECK(!has(out, "\n  pthread_rwlock_"));  // no row of the family (the scope names it)
    KZ_CHECK(has(a, "p0-compare P0 comparison table revision 2 (2026-10-30): "));
    KZ_CHECK(has(a, "p0-compare scope: families listed whole: byte-string comparison"));
  }

  // ---- the order and entry-path facts LOCK_PATHS and the relations rules that read them (relpaths.c) -----------
  {
    const fs::path X = work / "relpaths";
    fs::create_directories(X);
    const std::string src = fixtures + "/relpaths.c", rp = (X / "rp.kz").string();
    KZ_CHECK_EQ(run(q(tool) + " extract --no-profile --targets-rule R-T1,R-T2 --source " + q(src) + " --root " + q(fixtures) + " -o " + q(rp) + " -- -std=c99 -pedantic", &out), 0);
    KZ_CHECK_EQ(run(q(tool) + " show " + q(rp) + " --section lockpaths --section basis", &out), 0);
    // a write before the spawn in the loop body follows the spawn through the back edge; the write in the branch
    // without the spawn does not follow it
    KZ_CHECK(has(out, "after-spawn row ") && has(out, " write Rec.count at relpaths.c:83:5 in spawn_all after the spawn relpaths.c:84:5 pthread_create"));
    KZ_CHECK(has(out, " write Rec.ready at relpaths.c:94:5 in spawn_checked after the spawn relpaths.c:93:10 pthread_create"));
    KZ_CHECK(!has(out, "write Rec.ready at relpaths.c:98:"));
    // the read after the release of the caller's lock
    KZ_CHECK(has(out, " read at relpaths.c:53:15 in ready_unlocked through the call relpaths.c:52:3 pthread_mutex_unlock"));
    KZ_CHECK(has(out, "count.lock_paths = "));
    std::string a, b;
    KZ_CHECK_EQ(run(q(tool) + " relations " + q(rp), &a), 0);
    KZ_CHECK_EQ(run(q(tool) + " relations " + q(rp), &b), 0);
    KZ_CHECK_EQ(a, b);
    KZ_CHECK(has(a, "rules relations rules revision 2 (2026-10-30)"));
    // roots: the image entries of the table, the root that calls a root directly, the dispatching trampoline
    KZ_CHECK(has(a, "table-image-entry 2"));
    KZ_CHECK(has(a, "spawn calls resolved through the static image of the table their entry argument is read from: 1 [relpaths.c:154:10: alpha_main beta_main]"));
    KZ_CHECK(has(a, "trampolines 1 [tramp_main -> {helper_main (spawn-entry)}]"));
    KZ_CHECK(has(a, "through a direct call 1 [boss_main]"));
    // path C: the write in the loop body, the writes of the branch with the spawn only, the helper's write, the read
    // after the release
    KZ_CHECK(has(a, "field Rec.count; read relpaths.c:65:15 in rec_counted"));
    KZ_CHECK(has(a, "      write after the spawn in set_stage (through the call relpaths.c:113:3): relpaths.c:104:32"));
    KZ_CHECK(has(a, "      feasibility: possible: the writes after the spawn hold no lock of static identity"));
    KZ_CHECK(has(a, "(not composed: on a path from the entry of ready_unlocked the call relpaths.c:52:3 pthread_mutex_unlock"));
    KZ_CHECK(has(a, "      feasibility: proven-impossible: lock #0 &rec_mutex@relpaths.c:31 (static-instance) is held from the same acquisition"));
    KZ_CHECK(has(a, "rests-on(assume.lock_acquisition_succeeds; assume.no_cross_member_pointer_arithmetic; assume.static_instance_read_after_write; per-field completeness rule (PI decision pending))"));
    KZ_CHECK(!has(a, "write after the spawn: relpaths.c:98:"));
    // the ends: the destructor of a thread-specific key; the guard of the terminal call with the value passed
    KZ_CHECK(has(a, "after that entry returns the thread may still run functions of this program (thread-specific destructors: pthread_key_create at relpaths.c:"));
    KZ_CHECK(has(a, "noreturn: the terminal calls of app_exit are guarded: relpaths.c:224:20 -> exit under (eq param:status#0 9); with argument 0 = 3 every guard is false"));
    KZ_CHECK(has(a, "the loop relpaths.c:225:3 in app_exit has no exit"));
    // a derived spawn operation's store through its argument counts after the spawn, under no known lock
    KZ_CHECK(has(a, "      write after the spawn by the spawn operation: spawn_marked stores through arg 0 [&marks.started] inside the call at relpaths.c:"));
    // a condition more than 200 levels deep: R-T2 reads the whole tree
    const std::string dp = (X / "deep.kz").string();
    KZ_CHECK_EQ(run(q(tool) + " extract --no-profile --targets-rule R-T2 --source " + q(fixtures + "/deepcond.c") + " --root " + q(fixtures) + " -o " + q(dp) + " -- -std=c99 -pedantic", &out), 0);
    KZ_CHECK(has(out, "rule targets               1 (R-T1 0, R-T2 1, both 0;"));
    KZ_CHECK_EQ(run(q(tool) + " show " + q(dp) + " --section basis", &out), 0);
    KZ_CHECK(has(out, "target.Deep.flag = rule R-T2  (unset)"));
  }

  fs::remove_all(work);
  return kztest::finish("test_static_cli");
}

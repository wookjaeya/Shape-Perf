// ELF32 little-endian i386 in the analysis basis (E-ST of research/cfs_procrace/plan/c5_execution_basis_20261028.md
// section 10.5, the part that does not depend on the front end's handling of the target's diagnostics): the readers of
// elf_info.hpp on 32-bit objects, a shared object and an executable built here with -m32 (the same results as for the
// 64-bit ones, the identity ELF32 EM_386), damaged and refused headers, one ELF identity per program (a link set and a
// basis that mix classes are errors), and the basis of a small 32-bit closure (an executable and the modules its
// startup script loads) with its record target.elf; a 64-bit basis of the same program has no such item.  The programs
// are C99 without headers.  And the recipe's front_end_diagnostic (PI decision 2026-10-29 on section 10.5 item 6): one
// unit that the front end rejects by default (incompatible-function-pointer-types) and the build compiler compiles is
// read with -Wno-error=<that name> for that unit only, recorded as a deviation with the occurrence it lowers; without
// the statement it is not read; a second unit with the same diagnostic and no statement still stops the extraction; a
// statement for no unit, a name that is not one diagnostic, a group name, an unknown name, an occurrence on a line the
// statement does not cite and a cited line without one are errors.  And the recipe's relocatable (a link
// command that takes in the output of a relocatable link, `ld -r`, as a classic make build of the core does): the
// objects of that link are the program's units (one of them compiled with a relative -I, whose constants are evaluated
// as in the command's directory); without the statement the core has none; a command that is not relocatable, or a
// statement for an object no link takes in, is an error.  And a script line that names its module by the whole virtual
// path (recipe module_file ... full_path): the module is in the closure without the path and extension constants.
// Tool tests: every value is a unit-test value, not a research condition.
//
//   test_static_elf32 <kz-static> <c-compiler> <objects-dir> <profile-dir> <work-dir>
// Exits 77 when the compiler cannot build and link -m32 programs.
#include <elf.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "elf_info.hpp"
#include "kz/core/container.hpp"
#include "linkset.hpp"
#include "support/kz_test.hpp"

using namespace kzs;
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

// A copy of `from` with `edit` applied to its bytes.
fs::path edited(const fs::path& from, const fs::path& to, void (*edit)(std::string*)) {
  std::string b = read(from);
  edit(&b);
  write(to, b);
  return to;
}

// The section header table of an ELF32 file in b (for the edits).
Elf32_Shdr* shdr32(std::string* b, uint32_t i) {
  Elf32_Ehdr eh;
  std::memcpy(&eh, b->data(), sizeof eh);
  return reinterpret_cast<Elf32_Shdr*>(&(*b)[eh.e_shoff + i * eh.e_shentsize]);
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 6) {
    std::fprintf(stderr, "usage: test_static_elf32 <kz-static> <c-compiler> <objects-dir> <profile-dir> <work-dir>\n");
    return 2;
  }
  const std::string tool = argv[1], cc = argv[2], objects = argv[3], profile = argv[4];
  const fs::path work = fs::path(argv[5]);
  fs::remove_all(work);
  fs::create_directories(work);
  std::string out;
  write(work / "probe.c", "int main(void) { return 0; }\n");
  if (run(q(cc) + " -m32 -o " + q((work / "probe").string()) + " " + q((work / "probe.c").string()), &out) != 0) {
    std::printf("test_static_elf32: %s cannot build -m32 programs here: skipped\n%s", cc.c_str(), out.c_str());
    return 77;
  }

  // ---- the readers on real 32-bit files ----------------------------------------------------------------------------
  const fs::path O = work / "obj";
  fs::create_directories(O);
  for (const char* n : {"main_obj", "whole_member", "lazy_used", "lazy_unused"}) {
    const std::string src = (fs::path(objects) / (std::string(n) + ".c")).string();
    KZ_CHECK_EQ(run(q(cc) + " -m32 -std=c99 -fPIC -c " + q(src) + " -o " + q((O / (std::string(n) + "32.o")).string()), &out), 0);
    KZ_CHECK_EQ(run(q(cc) + " -std=c99 -fPIC -c " + q(src) + " -o " + q((O / (std::string(n) + "64.o")).string()), &out), 0);
  }
  KZ_CHECK_EQ(run(q(cc) + " -m32 -shared -o " + q((O / "whole32.so").string()) + " " + q((O / "whole_member32.o").string()), &out), 0);
  KZ_CHECK_EQ(run(q(cc) + " -m32 -Wl,-export-dynamic -o " + q((O / "prog32").string()) + " " + q((O / "main_obj32.o").string()) + " " + q((O / "lazy_used32.o").string()), &out), 0);
  {
    ObjectSymbols m, w, u, n, m64;
    KZ_CHECK_OK(read_object_symbols((O / "main_obj32.o").string(), &m));
    KZ_CHECK_OK(read_object_symbols((O / "whole_member32.o").string(), &w));
    KZ_CHECK_OK(read_object_symbols((O / "lazy_used32.o").string(), &u));
    KZ_CHECK_OK(read_object_symbols((O / "lazy_unused32.o").string(), &n));
    KZ_CHECK_OK(read_object_symbols((O / "main_obj64.o").string(), &m64));
    // position-independent code on i386 refers to _GLOBAL_OFFSET_TABLE_ and defines the hidden helpers __x86.get_pc_thunk.*;
    // apart from them the 32-bit objects have the 64-bit ones' symbols
    auto program_symbols = [](const std::set<std::string>& s) {
      std::set<std::string> o;
      for (const std::string& x : s)
        if (x != "_GLOBAL_OFFSET_TABLE_" && x.rfind("__x86.get_pc_thunk.", 0) != 0) o.insert(x);
      return o;
    };
    KZ_CHECK(m.defined.count("main") == 1 && m.undefined.count("used_sym") == 1);
    KZ_CHECK(w.defined.count("whole_sym") == 1 && program_symbols(w.undefined).empty());
    KZ_CHECK(u.defined.count("used_sym") == 1 && n.defined.count("unused_sym") == 1);
    KZ_CHECK(program_symbols(m.defined) == m64.defined && program_symbols(m.undefined) == m64.undefined && program_symbols(m.hidden) == m64.hidden);
    for (const std::string& x : m.hidden) KZ_CHECK(x.rfind("__x86.get_pc_thunk.", 0) == 0);  // hidden: the helpers only
    KZ_CHECK(m.elf.elf_class == ELFCLASS32 && m.elf.data == ELFDATA2LSB && m.elf.machine == EM_386 && m.elf.pointer_bytes() == 4u);
    KZ_CHECK_EQ(m.elf.text(), std::string("ELF32 little-endian EM_386 (3)"));
    KZ_CHECK(m64.elf.elf_class == ELFCLASS64 && m64.elf.machine == EM_X86_64 && m64.elf.pointer_bytes() == 8u);
    KZ_CHECK_EQ(m64.elf.text(), std::string("ELF64 little-endian EM_X86_64 (62)"));
    ObjectSymbols so, exe, none;
    KZ_CHECK_OK(read_dynamic_symbols((O / "whole32.so").string(), &so));
    KZ_CHECK(so.defined.count("whole_sym") == 1 && so.elf.elf_class == ELFCLASS32);
    KZ_CHECK_OK(read_dynamic_symbols((O / "prog32").string(), &exe));
    KZ_CHECK(exe.defined.count("main") == 1 && exe.defined.count("used_sym") == 1);
    KZ_CHECK(!read_dynamic_symbols((O / "main_obj32.o").string(), &none).ok());  // not linked
    ElfNotes on, xn;
    KZ_CHECK_OK(read_elf_notes((O / "main_obj32.o").string(), &on));
    KZ_CHECK(on.type == "relocatable" && !on.comments.empty() && on.elf.elf_class == ELFCLASS32);
    KZ_CHECK_OK(read_elf_notes((O / "prog32").string(), &xn));
    KZ_CHECK(xn.type.rfind("ET_DYN", 0) == 0 || xn.type == "executable");
    KZ_CHECK_EQ(xn.build_id_hex.size(), size_t(40));  // the linker's default build-id (sha1)
    // the build-id is the one readelf shows
    KZ_CHECK_EQ(run("readelf -n " + q((O / "prog32").string()), &out), 0);
    KZ_CHECK(has(out, "Build ID: " + xn.build_id_hex));
  }

  // ---- damaged and refused headers: a Status, never a crash -------------------------------------------------------
  {
    const fs::path main32 = O / "main_obj32.o";
    ObjectSymbols s;
    ElfNotes notes;
    const fs::path cut = edited(main32, O / "cut.o", [](std::string* b) { b->resize(40); });  // shorter than Elf32_Ehdr (52)
    KZ_CHECK(!read_object_symbols(cut.string(), &s).ok());
    const fs::path shoff = edited(main32, O / "shoff.o", [](std::string* b) {
      Elf32_Ehdr eh;
      std::memcpy(&eh, b->data(), sizeof eh);
      eh.e_shoff = static_cast<Elf32_Off>(b->size());
      std::memcpy(&(*b)[0], &eh, sizeof eh);
    });
    kz::Status st = read_object_symbols(shoff.string(), &s);
    KZ_CHECK(!st.ok() && has(st.message, "section header out of range"));
    KZ_CHECK(!read_elf_notes(shoff.string(), &notes).ok());
    const fs::path symoff = edited(main32, O / "symoff.o", [](std::string* b) {
      Elf32_Ehdr eh;
      std::memcpy(&eh, b->data(), sizeof eh);
      for (uint32_t i = 0; i < eh.e_shnum; ++i)
        if (shdr32(b, i)->sh_type == SHT_SYMTAB) shdr32(b, i)->sh_offset = static_cast<Elf32_Off>(b->size() - 4);
    });
    st = read_object_symbols(symoff.string(), &s);
    KZ_CHECK(!st.ok() && has(st.message, "symbol table out of range"));
    const fs::path msb = edited(main32, O / "msb.o", [](std::string* b) { (*b)[EI_DATA] = static_cast<char>(ELFDATA2MSB); });
    st = read_object_symbols(msb.string(), &s);
    KZ_CHECK(!st.ok() && has(st.message, "little-endian"));
    const fs::path cls = edited(main32, O / "class.o", [](std::string* b) { (*b)[EI_CLASS] = static_cast<char>(ELFCLASSNONE); });
    KZ_CHECK(!read_object_symbols(cls.string(), &s).ok());
  }

  // ---- one ELF identity per program -----------------------------------------------------------------------------------
  {
    ElfTarget t;
    ObjectSymbols a, b, c;
    KZ_CHECK_OK(read_object_symbols((O / "main_obj32.o").string(), &a));
    KZ_CHECK_OK(read_object_symbols((O / "lazy_used32.o").string(), &b));
    KZ_CHECK_OK(read_object_symbols((O / "main_obj64.o").string(), &c));
    KZ_CHECK_OK(t.add("a32", a.elf));
    KZ_CHECK_OK(t.add("b32", b.elf));
    kz::Status st = t.add("c64", c.elf);
    KZ_CHECK(!st.ok() && has(st.message, "c64 is ELF64 little-endian EM_X86_64 (62), but a32") && has(st.message, "is ELF32 little-endian EM_386 (3)"));
    const fs::path arm = edited(O / "lazy_used32.o", O / "arm.o", [](std::string* x) {
      const uint16_t m = EM_ARM;
      std::memcpy(&(*x)[EI_NIDENT + 2], &m, 2);
    });
    ObjectSymbols d;
    KZ_CHECK_OK(read_object_symbols(arm.string(), &d));
    KZ_CHECK(!t.add("arm", d.elf).ok());
    // a link that names a 32-bit and a 64-bit object
    LinkPlan plan;
    KZ_CHECK_OK(parse_link_command("cc " + (O / "main_obj32.o").string() + " " + (O / "lazy_used64.o").string() + "\n", O.string(), &plan));
    LinkSet set;
    st = resolve_link_set(plan, &set);
    KZ_CHECK(!st.ok() && has(st.message, "one program's files must have one ELF class"));
    // a 32-bit link with a whole archive and a lazy one: the 64-bit rules, the 32-bit identity
    const fs::path libdir = work / "lib";
    write(libdir / "libw.a", "placeholder\n");
    write(libdir / "libx.a", "placeholder\n");
    write(libdir / "CMakeFiles/w.dir/link.txt", "/usr/bin/ar qc libw.a " + (O / "whole_member32.o").string() + "\n");
    write(libdir / "CMakeFiles/x.dir/link.txt", "/usr/bin/ar qc libx.a " + (O / "lazy_unused32.o").string() + " " + (O / "lazy_used32.o").string() + "\n");
    LinkPlan plan32;
    KZ_CHECK_OK(parse_link_command("cc " + (O / "main_obj32.o").string() + " -Wl,--whole-archive " + (libdir / "libw.a").string() + " -Wl,--no-whole-archive " +
                                       (libdir / "libx.a").string() + "\n",
                                   O.string(), &plan32));
    KZ_CHECK_OK(resolve_link_set(plan32, &set));
    KZ_CHECK_EQ(set.members.size(), size_t(4));
    KZ_CHECK_EQ(set.included(), size_t(3));
    KZ_CHECK(set.elf.known() && set.elf.id().elf_class == ELFCLASS32 && set.elf.id().machine == EM_386);
    // the same archive with a 64-bit member among the 32-bit ones (not pulled, still read): an error
    write(libdir / "CMakeFiles/x.dir/link.txt", "/usr/bin/ar qc libx.a " + (O / "lazy_unused64.o").string() + " " + (O / "lazy_used32.o").string() + "\n");
    KZ_CHECK(!resolve_link_set(plan32, &set).ok());
  }

  // ---- the basis of a 32-bit closure, and of the same program as ELF64 --------------------------------------------------
  const std::string app_src =
      "typedef struct { int sem; } Throttle;\nThrottle thr[2];\nint sem_create(int *sem_id);\n"
      "int app_main(void) {\n  int i;\n  for (i = 0; i < 2; ++i)\n    if (sem_create(&thr[i].sem) != 0) return i;\n  return 0;\n}\n";
  auto closure = [&](const fs::path& C, const std::string& abi_core, const std::string& abi_app, const std::string& app = std::string(),
                     const std::string& recipe_extra = std::string(), bool partial = false, bool full_path = false, const std::string& core_api_extra = std::string()) {
    for (const char* d : {"src", "obj", "cf", "mods/myapp", "profile"}) fs::create_directories(C / d);
    const std::string defines = "#define NONVOL_MOUNT \"/cf\"\n#define MODULE_EXT \".so\"\n#define STARTUP_FILE \"/cf/start.scr\"\n";
    if (partial) write(C / "hdr/start.h", defines);  // and the core is compiled with a relative -Ihdr, as classic make does
    write(C / "src/core_main.c",
          (partial ? std::string("#include \"start.h\"\n") : defines) +
          "const char *nonvol_mount(void) { return NONVOL_MOUNT; }\nconst char *module_ext(void) { return MODULE_EXT; }\n"
          "const char *startup_file(void) { return STARTUP_FILE; }\nint main(void) { return 0; }\n");
    write(C / "src/core_api.c", "typedef struct { int obj_id; } Rec;\nRec rec_table[2];\nint sem_create(int *sem_id) { rec_table[0].obj_id = 7; *sem_id = rec_table[0].obj_id; return 0; }\n" + core_api_extra);
    write(C / "src/app.c", app.empty() ? app_src : app);
    std::string cdb = "[\n";
    for (const char* n : {"core_main", "core_api", "app"}) {
      const std::string src = (C / "src" / (std::string(n) + ".c")).string();
      const std::string abi = std::string(n) == "app" ? abi_app : abi_core;
      const std::string cmd = cc + abi + (partial && std::string(n) == "core_main" ? " -Ihdr" : "") + " -std=c99 -fPIC -c " + src + " -o obj/" + n + ".o";
      KZ_CHECK_EQ(run("cd " + q(C.string()) + " && " + cmd, &out), 0);
      cdb += std::string(cdb.size() > 2 ? ",\n" : "") + "{\"directory\": \"" + C.string() + "\", \"command\": \"" + cmd + "\", \"file\": \"" + src + "\"}";
    }
    write(C / "compile_commands.json", cdb + "\n]\n");
    if (partial) {  // the core's objects combined by a relocatable link first, as a classic make build does
      write(C / "obj/core.o.link.txt", "ld -melf_i386 -o core.o -r core_main.o core_api.o\n");
      KZ_CHECK_EQ(run("cd " + q((C / "obj").string()) + " && " + read(C / "obj/core.o.link.txt"), &out), 0);
      write(C / "link_core.txt", cc + abi_core + " -rdynamic -o core obj/core.o\n");
    } else {
      write(C / "link_core.txt", cc + abi_core + " -rdynamic -o core obj/core_main.o obj/core_api.o\n");
    }
    write(C / "mods/myapp/link.txt", cc + abi_app + " -shared -o myapp.so ../../obj/app.o\n");
    KZ_CHECK_EQ(run("cd " + q(C.string()) + " && " + read(C / "link_core.txt"), &out), 0);
    KZ_CHECK_EQ(run("cd " + q((C / "mods/myapp").string()) + " && " + read(C / "mods/myapp/link.txt"), &out), 0);
    fs::copy_file(C / "mods/myapp/myapp.so", C / "cf/myapp.so");
    // the script names the module by a name (the mount and extension constants complete it) or by its whole path
    write(C / "cf/start.scr", full_path ? "APP, /cf/myapp.so, app_main, MY_APP, 50;\n" : "APP, myapp, app_main, MY_APP, 50;\n");
    write(C / "recipe.kzb", std::string(R"(kzb 1;
name "closure32";
link "link_core.txt";
binary "core";
)") + (full_path ? "module \"/cf/myapp.so\"" : "module \"myapp\"") + R"( link "mods/myapp/link.txt";
startup_script {
  reset POWERON;
  rule POWERON nonvolatile;
  file nonvolatile const "STARTUP_FILE" in "src/core_main.c";
  mount "/cf" "cf";
  entry library "LIB" global;
  entry app "APP" local;
)" + (full_path ? "  module_file field 1 full_path;\n" : "  module_file field 1 path const \"NONVOL_MOUNT\" extension const \"MODULE_EXT\" in \"src/core_main.c\";\n") +
                                       "}\n" + recipe_extra);
    for (const fs::directory_entry& e : fs::directory_iterator(profile))
      if (e.path().extension() == ".kzp") fs::copy_file(e.path(), C / "profile" / e.path().filename());
    const int rc = run(q(tool) + " basis --profile " + q((C / "profile").string()) + " --compile-db " + q((C / "compile_commands.json").string()) + " --build-dir " +
                           q(C.string()) + " --bundle-root " + q(C.string()) + " --recipe " + q((C / "recipe.kzb").string()) + " --text -o " + q((C / "basis.kz").string()),
                       &out);
    if (rc != 0) std::printf("%s: kz-static basis exit %d:\n%s", C.filename().c_str(), rc, out.c_str());
    return rc;
  };
  KZ_CHECK_EQ(closure(work / "clos32", " -m32", " -m32"), 0);
  KZ_CHECK(has(out, "translation units          3"));
  KZ_CHECK(has(out, "modules in the closure     1 (units 1)"));
  KZ_CHECK(has(out, "module[myapp].bound[core] = sem_create"));
  KZ_CHECK(has(out, "target.elf = ELF32 little-endian EM_386 (3), pointer 4 bytes: every object, archive member, executable and module this basis read (the first: obj/core_main.o)"));
  // the extraction under it reads the units as the recorded -m32 commands give them
  KZ_CHECK_EQ(run(q(tool) + " extract --profile " + q((work / "clos32/profile").string()) + " --basis " + q((work / "clos32/basis.kz").string()) + " -o " +
                      q((work / "clos32/facts.kz").string()) + " --target Throttle.sem",
                  &out),
              0);
  KZ_CHECK(has(out, "location Throttle.sem   writers 1 (direct 0, data function 0, out-pointer 1"));
  // the 64-bit program: no target.elf item (as every basis before)
  KZ_CHECK_EQ(closure(work / "clos64", "", ""), 0);
  KZ_CHECK(has(out, "module[myapp].bound[core] = sem_create") && !has(out, "target.elf"));
  // a 32-bit executable that loads a 64-bit module: an error, not a closure
  const int rc = closure(work / "closmix", " -m32", "");
  KZ_CHECK(rc != 0);
  KZ_CHECK(has(out, "one program's files must have one ELF class, byte order and machine"));

  // ---- a unit the front end rejects by default and the build compiler compiles (recipe front_end_diagnostic) ----------
  // app.c passes a handler of another function type (gcc: a warning; clang: incompatible-function-pointer-types, an error
  // by default) on its line 12.  Without the statement the extraction stops at the unit; with it, for that unit only, the
  // unit is read, its call is a fact, and the basis records the deviation and the one occurrence it lowers.
  const std::string fp_app = app_src + "void on_tick(void) {}\nint add_handler(void (*h)(int));\nint app_init(void) { return add_handler(on_tick); }\n";
  const std::string fed_at = "front_end_diagnostic \"src/app.c\" \"incompatible-function-pointer-types\" at \"src/app.c\" 12";
  const std::string fed = fed_at + " reference \"unit test: the C compiler of the test compiles src/app.c\";\n";
  auto extract = [&](const fs::path& C) {
    const int re = run(q(tool) + " extract --profile " + q((C / "profile").string()) + " --basis " + q((C / "basis.kz").string()) + " -o " + q((C / "facts.kz").string()) +
                           " --target Throttle.sem",
                       &out);
    if (re != 0) return re;
    std::string calls;
    const int rs = run(q(tool) + " show " + q((C / "facts.kz").string()) + " --section calls --callee add_handler", &calls);
    out += calls;
    return rs;
  };
  KZ_CHECK_EQ(closure(work / "fp_without", " -m32", " -m32", fp_app), 0);
  KZ_CHECK(!has(out, "front_end.unit_diagnostic"));
  KZ_CHECK(extract(work / "fp_without") != 0);
  KZ_CHECK(has(out, "cannot parse") && has(out, "src/app.c") && has(out, "incompatible function pointer"));
  KZ_CHECK_EQ(closure(work / "fp_with", " -m32", " -m32", fp_app, fed), 0);
  KZ_CHECK(has(out, "front_end.unit_diagnostic[src/app.c][incompatible-function-pointer-types] = -Wno-error=incompatible-function-pointer-types  (deviation)"));
  KZ_CHECK(has(out, "front_end.lowered[src/app.c][incompatible-function-pointer-types] = 1 occurrence(s), each on a cited line: src/app.c:12:41 incompatible function pointer types"));
  KZ_CHECK_EQ(extract(work / "fp_with"), 0);
  KZ_CHECK(has(out, "app_init -> add_handler"));
  KZ_CHECK(has(out, "location Throttle.sem   writers 1 (direct 0, data function 0, out-pointer 1"));
  // the deviation is in the basis, with its reference
  {
    kz::Container b;
    KZ_CHECK_OK(kz::Container::read_file((work / "fp_with/basis.kz").string(), &b));
    std::vector<kz_deviation> devs;
    KZ_CHECK_OK(b.read_section(KZ_SEC_DEVIATIONS, &devs));
    bool found = false;
    for (const kz_deviation& d : devs) {
      std::string subject, ref;
      b.string(d.subject, &subject);
      b.string(d.reference_value, &ref);
      if (subject == "front-end diagnostic incompatible-function-pointer-types of src/app.c" && has(ref, "unit test: the C compiler of the test compiles src/app.c")) found = true;
    }
    KZ_CHECK(found);
  }
  // the statement is for its unit only: a second unit (core_api.c, line 6) with the same diagnostic and no statement stops
  // the extraction there; with a statement for each, both are read
  const std::string fp_core = "int core_handler(void (*h)(int)) { return h != 0; }\nvoid on_core(void) {}\nint core_init(void) { return core_handler(on_core); }\n";
  KZ_CHECK_EQ(closure(work / "fp_two", " -m32", " -m32", fp_app, fed, false, false, fp_core), 0);
  KZ_CHECK(has(out, "front_end.lowered[src/app.c]") && !has(out, "front_end.unit_diagnostic[src/core_api.c]"));
  KZ_CHECK(extract(work / "fp_two") != 0);
  KZ_CHECK(has(out, "cannot parse") && has(out, "src/core_api.c") && has(out, "incompatible function pointer"));
  const std::string fed_core = "front_end_diagnostic \"src/core_api.c\" \"incompatible-function-pointer-types\" at \"src/core_api.c\" 6 reference \"x\";\n";
  KZ_CHECK_EQ(closure(work / "fp_both", " -m32", " -m32", fp_app, fed + fed_core, false, false, fp_core), 0);
  KZ_CHECK(has(out, "front_end.lowered[src/core_api.c][incompatible-function-pointer-types] = 1 occurrence(s), each on a cited line: src/core_api.c:6:"));
  KZ_CHECK_EQ(extract(work / "fp_both"), 0);
  KZ_CHECK(has(out, "app_init -> add_handler"));
  // a statement for a unit the program does not have, and a name that is not one diagnostic: errors
  KZ_CHECK(closure(work / "fp_nounit", " -m32", " -m32", fp_app, "front_end_diagnostic \"src/none.c\" \"incompatible-function-pointer-types\" at \"src/none.c\" 12 reference \"x\";\n") != 0);
  KZ_CHECK(has(out, "which is no unit of the program"));
  KZ_CHECK(closure(work / "fp_blanket", " -m32", " -m32", fp_app, "front_end_diagnostic \"src/app.c\" \"-Wno-error\" at \"src/app.c\" 12 reference \"x\";\n") != 0);
  KZ_CHECK(has(out, "is not a diagnostic name"));
  KZ_CHECK(closure(work / "fp_noref", " -m32", " -m32", fp_app, fed_at + ";\n") != 0);
  KZ_CHECK(closure(work / "fp_noat", " -m32", " -m32", fp_app, "front_end_diagnostic \"src/app.c\" \"incompatible-function-pointer-types\" reference \"x\";\n") != 0);
  KZ_CHECK(has(out, "needs `at"));
  // a group that contains the diagnostic lowers it too: refused, the diagnostic itself must be named
  KZ_CHECK(closure(work / "fp_group", " -m32", " -m32", fp_app, "front_end_diagnostic \"src/app.c\" \"incompatible-pointer-types\" at \"src/app.c\" 12 reference \"x\";\n") != 0);
  KZ_CHECK(has(out, "it lowers incompatible-function-pointer-types at src/app.c:12:41, which no statement of the unit names"));
  // a name the front end does not know lowers nothing: the unit is still rejected
  KZ_CHECK(closure(work / "fp_unknown", " -m32", " -m32", fp_app, "front_end_diagnostic \"src/app.c\" \"incompatible-function-pointer-typo\" at \"src/app.c\" 12 reference \"x\";\n") != 0);
  KZ_CHECK(has(out, "the front end still rejects the unit: src/app.c:12:41"));
  // a second occurrence (line 13) the statement does not cite, and a cited line without one: errors
  KZ_CHECK(closure(work / "fp_uncited", " -m32", " -m32", fp_app + "int app_again(void) { return add_handler(on_tick); }\n", fed) != 0);
  KZ_CHECK(has(out, "incompatible-function-pointer-types at src/app.c:13:42 is on a line the statement does not cite"));
  KZ_CHECK(closure(work / "fp_noocc", " -m32", " -m32", fp_app, fed_at + " at \"src/app.c\" 5 reference \"x\";\n") != 0);
  KZ_CHECK(has(out, "the statement cites src/app.c:5, where the front end raises no incompatible-function-pointer-types as an error"));

  // ---- a link command that takes in the output of a relocatable link (recipe relocatable) -----------------------------
  const std::string rel = "relocatable \"obj/core.o\" link \"obj/core.o.link.txt\";\n";
  KZ_CHECK_EQ(closure(work / "rel_with", " -m32", " -m32", std::string(), rel, true), 0);
  KZ_CHECK(has(out, "translation units          3"));
  KZ_CHECK(has(out, "link.relocatable[obj/core.o] = obj/core.o.link.txt: 2 objects"));
  // the constants of a unit compiled with a relative -I are evaluated as in the command's directory, and the header that
  // defines them is named in the bundle (no unreadable file)
  KZ_CHECK(has(out, "const.STARTUP_FILE = \"/cf/start.scr\""));
  KZ_CHECK(!has(out, "cannot read"));
  {  // the core's units are the relocatable link's objects (TUS member)
    kz::Container b;
    KZ_CHECK_OK(kz::Container::read_file((work / "rel_with/basis.kz").string(), &b));
    std::vector<kz_tu> tus;
    KZ_CHECK_OK(b.read_section(KZ_SEC_TUS, &tus));
    size_t rel_members = 0;
    for (const kz_tu& t : tus) {
      std::string member;
      b.string(t.member, &member);
      if (member == "member of core.o (relocatable link)") ++rel_members;
    }
    KZ_CHECK_EQ(rel_members, size_t(2));
  }
  KZ_CHECK(has(out, "module[myapp].bound[core] = sem_create"));
  KZ_CHECK(has(out, "target.elf = ELF32 little-endian EM_386 (3), pointer 4 bytes"));
  // without the statement the relocatable object is a named object no compile command produces: the core has no unit
  KZ_CHECK_EQ(closure(work / "rel_without", " -m32", " -m32", std::string(), std::string(), true), 0);
  KZ_CHECK(has(out, "no compile command produces") && has(out, "obj/core.o"));
  KZ_CHECK(!has(out, "link.relocatable"));
  // a command that is not a relocatable link, and a relocatable object no link takes in: errors
  {
    const fs::path C = work / "rel_notr";
    KZ_CHECK_EQ(closure(C, " -m32", " -m32", std::string(), std::string(), true), 0);
    write(C / "obj/other.link.txt", "ld -melf_i386 -o core.o core_main.o core_api.o\n");
    write(C / "recipe.kzb", read(C / "recipe.kzb") + "relocatable \"obj/core.o\" link \"obj/other.link.txt\";\n");
    KZ_CHECK(run(q(tool) + " basis --profile " + q((C / "profile").string()) + " --compile-db " + q((C / "compile_commands.json").string()) + " --build-dir " + q(C.string()) +
                     " --bundle-root " + q(C.string()) + " --recipe " + q((C / "recipe.kzb").string()) + " -o " + q((C / "b2.kz").string()),
                 &out) != 0);
    KZ_CHECK(has(out, "is not a relocatable link"));
  }
  {  // the relocatable object exists, but the core's link names its objects directly
    const fs::path C = work / "rel_unused";
    KZ_CHECK_EQ(closure(C, " -m32", " -m32"), 0);
    write(C / "obj/core.o.link.txt", "ld -melf_i386 -o core.o -r core_main.o core_api.o\n");
    KZ_CHECK_EQ(run("cd " + q((C / "obj").string()) + " && " + read(C / "obj/core.o.link.txt"), &out), 0);
    write(C / "recipe.kzb", read(C / "recipe.kzb") + rel);
    KZ_CHECK(run(q(tool) + " basis --profile " + q((C / "profile").string()) + " --compile-db " + q((C / "compile_commands.json").string()) + " --build-dir " + q(C.string()) +
                     " --bundle-root " + q(C.string()) + " --recipe " + q((C / "recipe.kzb").string()) + " -o " + q((C / "b2.kz").string()),
                 &out) != 0);
    KZ_CHECK(has(out, "no link command takes it in"));
  }

  // ---- a script line that names its module by the whole virtual path (module_file field 1 full_path) --------------------
  KZ_CHECK_EQ(closure(work / "full_path", " -m32", " -m32", std::string(), rel, true, true), 0);
  KZ_CHECK(has(out, "modules in the closure     1 (units 1)"));
  KZ_CHECK(has(out, "module[/cf/myapp.so].bound[core] = sem_create"));
  KZ_CHECK(!has(out, "const.NONVOL_MOUNT") && !has(out, "const.MODULE_EXT"));

  fs::remove_all(work);
  return kztest::finish("test_static_elf32");
}

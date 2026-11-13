// The link command and the archive members it takes in, with real object files.
//
//   test_static_link <main_obj.o> <whole_member.o> <lazy_used.o> <lazy_unused.o>
#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "elf_info.hpp"
#include "linkset.hpp"
#include "support/kz_test.hpp"

using namespace kzs;
namespace fs = std::filesystem;

namespace {

void write(const fs::path& p, const std::string& text) {
  fs::create_directories(p.parent_path());
  std::ofstream(p) << text;
}

const LinkMember* find(const LinkSet& s, const std::string& object_name) {
  for (const LinkMember& m : s.members)
    if (fs::path(m.object).filename() == object_name) return &m;
  return nullptr;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 5) {
    std::fprintf(stderr, "usage: test_static_link <main_obj.o> <whole_member.o> <lazy_used.o> <lazy_unused.o>\n");
    return 2;
  }
  const std::string main_o = argv[1], whole_o = argv[2], used_o = argv[3], unused_o = argv[4];

  // ---- splitting and joining ------------------------------------------------------------------
  {
    const std::vector<std::string> t = split_command("cc -DNAME=\\\"xy\\\" 'it'\\''s' \"a b\" plain   -o out '-DQ=\"x y\"'");
    KZ_CHECK_EQ(t.size(), size_t(8));
    KZ_CHECK_EQ(t[1], std::string("-DNAME=\"xy\""));
    KZ_CHECK_EQ(t[7], std::string("-DQ=\"x y\""));
    KZ_CHECK_EQ(t[2], std::string("it's"));
    KZ_CHECK_EQ(t[3], std::string("a b"));
    KZ_CHECK_EQ(t[4], std::string("plain"));
    KZ_CHECK(split_command("").empty());
    KZ_CHECK(split_command("   \t ").empty());
    KZ_CHECK_EQ(split_command("''").size(), size_t(1));  // an empty argument is an argument
    const std::vector<std::string> tricky = {"plain", "with space", "quote'inside", "back\\slash", "dq\"x", "", "-DFLAG=\"a b\""};
    KZ_CHECK(split_command(join_command(tricky)) == tricky);
  }

  // ---- ELF symbols of real objects -----------------------------------------------------------
  {
    ObjectSymbols m, w, u, n;
    KZ_CHECK_OK(read_object_symbols(main_o, &m));
    KZ_CHECK_OK(read_object_symbols(whole_o, &w));
    KZ_CHECK_OK(read_object_symbols(used_o, &u));
    KZ_CHECK_OK(read_object_symbols(unused_o, &n));
    KZ_CHECK(m.defined.count("main") == 1 && m.undefined.count("used_sym") == 1);
    KZ_CHECK(w.defined.count("whole_sym") == 1 && w.undefined.empty());
    KZ_CHECK(u.defined.count("used_sym") == 1);
    KZ_CHECK(n.defined.count("unused_sym") == 1);
    ObjectSymbols none;
    KZ_CHECK(!read_object_symbols((fs::path(main_o).parent_path() / "no_such_file.o").string(), &none).ok());
    // a text file is not an object
    const fs::path junk = fs::temp_directory_path() / ("kz_static_junk_" + std::to_string(static_cast<long>(::getpid())));
    write(junk, "this is not an ELF file\n");
    KZ_CHECK(!read_object_symbols(junk.string(), &none).ok());
    ElfNotes notes;
    KZ_CHECK(!read_elf_notes(junk.string(), &notes).ok());
    fs::remove(junk);
    // an object file's notes: relocatable, with a compiler comment
    KZ_CHECK_OK(read_elf_notes(main_o, &notes));
    KZ_CHECK_EQ(notes.type, std::string("relocatable"));
    KZ_CHECK(!notes.comments.empty());
  }

  // ---- the link command ---------------------------------------------------------------------------
  const fs::path root = fs::temp_directory_path() / ("kz_static_link_" + std::to_string(static_cast<long>(::getpid())));
  fs::remove_all(root);
  const fs::path dir = root / "build/app";
  {
    LinkPlan plan;
    KZ_CHECK_OK(parse_link_command("/usr/bin/cc -g -rdynamic obj/a.o \"obj/b c.o\" -o prog -Wl,--whole-archive ../lib/libw.a -Wl,--no-whole-archive ../lib/libx.a -lm -lpthread\n", dir.string(), &plan));
    KZ_CHECK_EQ(plan.inputs.size(), size_t(4));
    KZ_CHECK(plan.inputs[0].kind == LinkInput::Object && !plan.inputs[0].whole);
    KZ_CHECK_EQ(plan.inputs[0].path, normalize_path(dir.string() + "/obj/a.o"));
    KZ_CHECK_EQ(fs::path(plan.inputs[1].path).filename().string(), std::string("b c.o"));
    KZ_CHECK(plan.inputs[2].kind == LinkInput::Archive && plan.inputs[2].whole);
    KZ_CHECK_EQ(plan.inputs[2].path, normalize_path(root.string() + "/build/lib/libw.a"));
    KZ_CHECK(plan.inputs[3].kind == LinkInput::Archive && !plan.inputs[3].whole);
    KZ_CHECK(!plan.ignored.empty());
    KZ_CHECK(!parse_link_command("\n\n", dir.string(), &plan).ok());
  }

  // archives built by `ar` commands in the link.txt files next to them
  const fs::path libdir = root / "build/lib";
  write(libdir / "libw.a", "placeholder\n");
  write(libdir / "libx.a", "placeholder\n");
  write(libdir / "CMakeFiles/w.dir/link.txt", "/usr/bin/ar qc libw.a " + whole_o + "\n/usr/bin/ranlib libw.a\n");
  write(libdir / "CMakeFiles/x.dir/link.txt", "/usr/bin/ar qc libx.a " + unused_o + " " + used_o + "\n/usr/bin/ranlib libx.a\n");
  {
    std::vector<std::string> members;
    KZ_CHECK_OK(archive_members((libdir / "libx.a").string(), &members));
    KZ_CHECK_EQ(members.size(), size_t(2));
    KZ_CHECK_EQ(members[0], normalize_path(unused_o));
    KZ_CHECK(!archive_members((libdir / "libnone.a").string(), &members).ok());
  }

  // main_obj is named; libw is linked whole; libx is lazy: only the member that defines used_sym is pulled
  {
    LinkPlan plan;
    KZ_CHECK_OK(parse_link_command("cc " + main_o + " -Wl,--whole-archive " + (libdir / "libw.a").string() + " -Wl,--no-whole-archive " + (libdir / "libx.a").string() + "\n", dir.string(), &plan));
    LinkSet set;
    KZ_CHECK_OK(resolve_link_set(plan, &set));
    KZ_CHECK_EQ(set.members.size(), size_t(4));
    KZ_CHECK_EQ(set.archives, size_t(2));
    KZ_CHECK_EQ(set.included(), size_t(3));
    const LinkMember* m = find(set, "main_obj.c.o");
    if (m == nullptr) m = find(set, fs::path(main_o).filename().string());
    KZ_CHECK(m != nullptr && m->mode == LinkMember::Named);
    const LinkMember* w = find(set, fs::path(whole_o).filename().string());
    KZ_CHECK(w != nullptr && w->mode == LinkMember::Whole);
    const LinkMember* u = find(set, fs::path(used_o).filename().string());
    KZ_CHECK(u != nullptr && u->mode == LinkMember::Pulled && u->pulled_for == "used_sym");
    const LinkMember* n = find(set, fs::path(unused_o).filename().string());
    KZ_CHECK(n != nullptr && n->mode == LinkMember::NotPulled);
  }

  // a lazy archive named before the object that needs it pulls nothing (GNU ld reads left to right)
  {
    LinkPlan plan;
    KZ_CHECK_OK(parse_link_command("cc " + (libdir / "libx.a").string() + " " + main_o + "\n", dir.string(), &plan));
    LinkSet set;
    KZ_CHECK_OK(resolve_link_set(plan, &set));
    KZ_CHECK_EQ(set.included(), size_t(1));
  }

  // an object that a relocatable link wrote: with that link's plan, the program takes in its objects (the object itself is
  // not read: here it does not even exist); the plan must be a relocatable link of objects that writes that object
  {
    const std::string sub = (dir / "sub.o").string();
    LinkPlan rp;
    KZ_CHECK_OK(parse_link_command("ld -o sub.o -r " + whole_o + " " + used_o + "\n", dir.string(), &rp));
    KZ_CHECK_OK(relocatable_plan_check(rp, sub));
    KZ_CHECK(!relocatable_plan_check(rp, (dir / "other.o").string()).ok());
    LinkPlan not_r, with_archive;
    KZ_CHECK_OK(parse_link_command("ld -o sub.o " + whole_o + "\n", dir.string(), &not_r));
    KZ_CHECK(!relocatable_plan_check(not_r, sub).ok());
    KZ_CHECK_OK(parse_link_command("ld -r -o sub.o " + (libdir / "libw.a").string() + "\n", dir.string(), &with_archive));
    KZ_CHECK(!relocatable_plan_check(with_archive, sub).ok());
    RelocatableLinks rl;
    rl[normalize_path(sub)] = rp;
    LinkPlan plan;
    KZ_CHECK_OK(parse_link_command("cc " + main_o + " sub.o\n", dir.string(), &plan));
    LinkSet set;
    KZ_CHECK_OK(resolve_link_set(plan, &set, &rl));
    KZ_CHECK_EQ(set.members.size(), size_t(3));
    KZ_CHECK_EQ(set.included(), size_t(3));
    const LinkMember* w = find(set, fs::path(whole_o).filename().string());
    KZ_CHECK(w != nullptr && w->mode == LinkMember::Relocatable && w->archive == normalize_path(sub));
    KZ_CHECK(!resolve_link_set(plan, &set).ok());  // without the plan, sub.o is a named object, and it does not exist
    // a relocatable link that takes in another relocatable object is not read
    LinkPlan nested;
    KZ_CHECK_OK(parse_link_command("ld -r -o top.o sub.o\n", dir.string(), &nested));
    rl[normalize_path((dir / "top.o").string())] = nested;
    KZ_CHECK_OK(parse_link_command("cc top.o\n", dir.string(), &plan));
    KZ_CHECK(!resolve_link_set(plan, &set, &rl).ok());
  }

  // an object that does not exist is an error, not a silent omission
  {
    LinkPlan plan;
    KZ_CHECK_OK(parse_link_command("cc " + (dir / "missing.o").string() + "\n", dir.string(), &plan));
    LinkSet set;
    KZ_CHECK(!resolve_link_set(plan, &set).ok());
  }
  fs::remove_all(root);
  return kztest::finish("test_static_link");
}

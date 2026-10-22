// kz-static basis -- record the analysis basis B (SPEC 1.7) of a closed program.
#include <cstdio>
#include <filesystem>

#include "basis.hpp"
#include "commands.hpp"
#include "kz/core/dump.hpp"
#include "kz/core/identity.hpp"
#include "facts_text.hpp"

namespace kzs_cli {

int cmd_basis(int argc, char** argv) {
  Args args;
  std::string err;
  if (!parse_args(argc, argv, 2,
                  {"--profile", "--compile-db", "--build-dir", "--bundle-root", "--recipe", "--link", "--resource-dir", "--extra-arg", "-o"},
                  {"--text", "--verbose"}, &args, &err)) {
    std::fprintf(stderr, "kz-static basis: %s\n", err.c_str());
    return 2;
  }
  for (const char* required : {"--profile", "--compile-db", "--build-dir", "--bundle-root", "-o"})
    if (!args.has(required)) {
      std::fprintf(stderr, "kz-static basis: %s is required\n", required);
      return 2;
    }
  kz::phi::Profile profile;
  if (!load_profile(args.one("--profile"), &profile)) return 1;

  kzs::BasisInputs in;
  in.compile_db = args.one("--compile-db");
  in.build_dir = args.one("--build-dir");
  in.bundle_root = args.one("--bundle-root");
  in.link_override = args.one("--link");
  if (args.has("--recipe")) {
    in.recipe_path = args.one("--recipe");
    if (!read_text_file(in.recipe_path, &in.recipe_text)) {
      std::fprintf(stderr, "kz-static basis: cannot read the recipe %s\n", in.recipe_path.c_str());
      return 2;
    }
    // only the file name is recorded: the recipe is identified by its hash, not by where it lies
    in.recipe_path = std::filesystem::path(in.recipe_path).filename().string();
  }
  in.read.root = in.bundle_root;
  in.read.resource_dir = resource_dir(args);
  in.read.extra_args = args.many("--extra-arg");
  in.read.verbose = args.has("--verbose");
  in.profile_hash = kz::phi::profile_hash(profile);
  in.tool_hash = KZ_THIS_TOOL_HASH("kz-static");
  in.build_identity = kz::build_identity();

  kz::Bytes out;
  kzs::BasisReport rep;
  const kz::Status s = kzs::build_basis(in, profile, &out, &rep);
  if (!s.ok()) return fail("basis", s.describe());
  const kz::Status w = kz::write_whole_file(args.one("-o"), out);
  if (!w.ok()) {
    std::fprintf(stderr, "kz-static basis: %s\n", w.describe().c_str());
    return 2;
  }
  kz::Container c;
  if (!kz::Container::parse(out, &c).ok()) return fail("basis", "the container just written does not parse");

  std::printf("basis %s\n", kz::to_hex(c.artifact_hash()).c_str());
  std::printf("  compile database entries   %zu\n", rep.cdb_entries);
  std::printf("  link inputs                %zu (%zu archives)\n", rep.link_inputs, rep.archives);
  std::printf("  link members               %zu, in the program %zu, left in archives %zu\n", rep.link_members, rep.link_included, rep.lazy_not_pulled);
  std::printf("  translation units          %zu (objects without a compile command: %zu)\n", rep.units, rep.unmatched_objects);
  std::printf("  source trees               %zu checked, %zu at the pinned revision\n", rep.revisions_checked, rep.revisions_matching);
  std::printf("  constants evaluated        %zu ok, %zu failed\n", rep.constants_ok, rep.constants_failed);
  std::printf("  generated files, tables    %zu, %zu\n", rep.generated, rep.tables);
  if (!rep.startup_script.empty())
    std::printf("  startup script             %s (%zu libraries, %zu applications)\n", rep.startup_script.c_str(), rep.startup_libraries, rep.startup_apps);
  std::printf("  hand-entered items         %zu\n", rep.hand_entered);
  for (const std::string& w2 : rep.warnings) std::printf("  warning: %s\n", w2.c_str());
  if (args.has("--text")) std::fputs(kzs::basis_items_text(c).c_str(), stdout);
  return 0;
}

}  // namespace kzs_cli

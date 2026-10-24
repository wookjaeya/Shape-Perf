// kz-static extract -- read the translation units of a closed program and write the facts.
#include <cstdio>
#include <filesystem>
#include <sstream>

#include "basis.hpp"
#include "commands.hpp"
#include "extract.hpp"
#include "facts_text.hpp"
#include "finalize.hpp"
#include "kz/core/dump.hpp"
#include "kz/core/identity.hpp"
#include "serialize.hpp"
#include "spec.hpp"

namespace kzs_cli {

namespace {

namespace fs = std::filesystem;

constexpr const char* kCommandLine = "command line";

void add_declared(kzs::ExtractSpec* spec, const std::string& name, const std::string& value) {
  spec->declared.push_back(kzs::DeclaredItem{name, value, kCommandLine});
}

// What the command line adds to the profile's vocabulary.  Each entry is hand-entered: it is
// recorded in the facts container with provenance tier `researcher` and counted in the report.
bool add_cli_vocabulary(const Args& args, kzs::ExtractSpec* spec, std::string* err) {
  for (const std::string& t : args.many("--target")) {
    kzs::FieldRef f;
    if (!kzs::parse_field_ref(t, &f)) {
      *err = "--target expects Type.field, got " + t;
      return false;
    }
    if (!spec->has_field(f)) {
      spec->fields.push_back(kzs::TargetField{f, kCommandLine});
      add_declared(spec, "target." + t, "extraction target");
    }
  }
  struct Set {
    const char* option;
    std::set<std::string> kzs::ExtractSpec::*members;
    const char* role;
  };
  const Set sets[] = {{"--delay-op", &kzs::ExtractSpec::delay_ops, "delay operation"},
                      {"--lock-op", &kzs::ExtractSpec::lock_ops, "lock operation"},
                      {"--unlock-op", &kzs::ExtractSpec::unlock_ops, "unlock operation"},
                      {"--spawn-op", &kzs::ExtractSpec::spawn_ops, "spawn operation"},
                      {"--terminal-op", &kzs::ExtractSpec::terminal_ops, "terminal operation"},
                      {"--poll-fn", &kzs::ExtractSpec::poll_fns, "poll function"}};
  for (const Set& s : sets)
    for (const std::string& name : args.many(s.option))
      if ((spec->*s.members).insert(name).second) add_declared(spec, std::string("op.") + s.role + "." + name, s.role);
  return true;
}

std::string summary_text(const kzs::Model& m, const kzs::ExtractSpec& spec, size_t units) {
  std::ostringstream os;
  const kzs::Model::Resolved& R = m.resolved;
  os << "extract\n";
  os << "  translation units          " << units << "\n";
  os << "  functions                  " << R.func_keys.size() << " (bodies read " << m.stats.functions_read << ", in system headers skipped " << m.stats.functions_skipped_system << ")\n";
  os << "  call sites                 " << m.calls.size() << "\n";
  os << "  indirect calls             " << R.indirect_total << " (resolved " << R.indirect_resolved << ", unresolved " << R.indirect_unresolved << ")\n";
  os << "  spawn calls                " << R.spawn_total << " (entry resolved " << R.spawn_resolved << ")\n";
  os << "  loops                      " << m.loops.size() << " (table walks resolved " << R.walks_resolved << ", failed " << R.walks_failed << ", under a declared assumption " << R.walks_assumed << ")\n";
  os << "  certificate-view rules     unroll " << R.loops_unroll << ", await " << R.loops_await << ", star-closed " << R.loops_star << "\n";
  os << "  spawn parameters           " << m.spawn_params.size() << ", unrolled iteration rows " << m.iters.size() << "\n";
  os << "  out-pointer summaries      store-only " << R.outptr_store_only << ", escaping " << R.outptr_escapes << ", unsummarized " << R.outptr_unsummarized << "\n";
  os << "  untyped write sites        " << R.atype_untyped_sites_total << " in the program, " << R.atype_untyped_sites_relevant << " of them count for a target\n";
  os << "  unknown-effect passes      " << R.reach_passes_unknown << " (pointers that reach a target, passed to code without a body, an unresolved call or a variadic position)\n";
  const kz::EnumDesc* status = kz::find_enum("KZ_LOC_STATUS");
  for (const kzs::LocFact& l : m.locs) {
    const char* st = status != nullptr ? kz::enum_display(*status, l.status) : nullptr;
    os << "  location " << l.loc_type << "." << l.loc_field << "   writers " << l.n_writers << " (direct " << l.n_direct << ", data function " << l.n_data_fn << ", out-pointer " << l.n_outptr
       << ", initial values " << l.n_init << "), A-TYPE hits: untyped " << l.n_untyped << ", escape " << l.n_escape << ", cast " << l.n_cast << ", union " << l.n_union << ", unknown effect " << l.n_unknown << "  -> "
       << (st != nullptr ? st : "?") << "\n";
  }
  size_t builtin = 0, cli = 0;
  for (const kzs::DeclaredItem& d : spec.declared) (d.origin == kCommandLine ? cli : builtin)++;
  os << "  hand-entered inputs        " << spec.declared.size() << " (built-in data-function table " << builtin << ", command line " << cli << ")\n";
  os << "  declared assumptions       " << m.assumptions.size() << " (static images of table-walk links that are not const)\n";
  return os.str();
}

}  // namespace

int cmd_extract(int argc, char** argv) {
  Args args;
  std::string err;
  if (!parse_args(argc, argv, 2,
                  {"--profile", "--basis", "--source", "--root", "--target", "--delay-op", "--lock-op", "--unlock-op", "--spawn-op", "--terminal-op", "--poll-fn",
                   "--resource-dir", "--extra-arg", "--section", "-o"},
                  {"--text", "--verbose"}, &args, &err)) {
    std::fprintf(stderr, "kz-static extract: %s\n", err.c_str());
    return 2;
  }
  if (!args.has("--profile") || !args.has("-o") || (!args.has("--basis") && !args.has("--source"))) {
    std::fprintf(stderr, "kz-static extract: --profile, -o and one of --basis or --source are required\n");
    return 2;
  }
  if (args.has("--basis") && args.has("--source")) {
    std::fprintf(stderr, "kz-static extract: --basis and --source exclude each other\n");
    return 2;
  }
  kz::phi::Profile profile;
  if (!load_profile(args.many("--profile"), &profile)) return 1;

  kzs::ExtractSpec spec;
  kzs::spec_from_profile(profile, &spec);
  kzs::spec_add_builtin_data_fns(&spec);
  if (!add_cli_vocabulary(args, &spec, &err)) {
    std::fprintf(stderr, "kz-static extract: %s\n", err.c_str());
    return 2;
  }

  kzs::SerializeOptions so;
  so.profile_hash = kz::phi::profile_hash(profile);
  so.tool_hash = KZ_THIS_TOOL_HASH("kz-static");
  so.build_identity = kz::build_identity();

  std::vector<kzs::UnitCommand> units;
  kzs::ReadOptions ro;
  ro.resource_dir = resource_dir(args);
  ro.extra_args = args.many("--extra-arg");
  ro.verbose = args.has("--verbose");
  std::string root = args.one("--root");
  if (args.has("--basis")) {
    kz::Container basis;
    kz::Status s = kz::Container::read_file(args.one("--basis"), &basis);
    if (s.ok()) s = basis.verify_content();
    if (!s.ok()) return fail("extract", args.one("--basis") + ": " + s.describe());
    if (basis.header().artifact_kind != KZ_ARTIFACT_BASIS) return fail("extract", args.one("--basis") + " is not a basis container");
    std::string basis_root;
    s = kzs::units_from_basis(basis, &units, &basis_root);
    if (!s.ok()) return fail("extract", s.describe());
    if (root.empty()) root = basis_root;
    so.has_basis = true;
    so.basis_hash = basis.artifact_hash();
    so.notes.push_back({"extract.units_from", "basis " + kz::to_hex(basis.artifact_hash()).substr(0, 16)});
  } else {
    std::string flags;
    for (const std::string& a : args.after_dashes) flags += (flags.empty() ? "" : " ") + a;
    for (const std::string& src : args.many("--source")) {
      kzs::UnitCommand u;
      u.directory = fs::current_path().string();
      u.source = fs::absolute(src).lexically_normal().string();
      u.command_line.push_back("cc");
      for (const std::string& a : args.after_dashes) u.command_line.push_back(a);
      u.command_line.push_back("-c");
      u.command_line.push_back(u.source);
      units.push_back(u);
    }
    so.notes.push_back({"extract.units_from", "command line: " + flags});
  }
  // recorded paths are made relative to the root, which is compared with absolute paths
  if (!root.empty()) {
    root = fs::absolute(root).lexically_normal().generic_string();
    while (root.size() > 1 && root.back() == '/') root.pop_back();
  }
  ro.root = root;
  so.root = root;

  kzs::Model model;
  std::string error;
  if (!kzs::extract_units(units, spec, ro, &model, &error)) return fail("extract", error);
  kzs::finalize(&model, spec);

  kz::Bytes out;
  kz::Status s = kzs::serialize_facts(model, spec, so, &out);
  if (!s.ok()) return fail("extract", s.describe());
  s = kz::write_whole_file(args.one("-o"), out);
  if (!s.ok()) {
    std::fprintf(stderr, "kz-static extract: %s\n", s.describe().c_str());
    return 2;
  }
  kz::Container c;
  s = kz::Container::parse(out, &c);
  if (s.ok()) s = c.verify_content();
  if (!s.ok()) return fail("extract", "the container just written does not verify: " + s.describe());

  std::printf("facts %s\n", kz::to_hex(c.artifact_hash()).c_str());
  std::fputs(summary_text(model, spec, units.size()).c_str(), stdout);
  if (args.has("--text")) {
    kzs::TextOptions to;
    for (const std::string& sec : args.many("--section")) to.sections.insert(sec);
    std::string text;
    s = kzs::facts_text(c, to, &text);
    if (!s.ok()) return fail("extract", s.describe());
    std::fputs(text.c_str(), stdout);
  }
  return 0;
}

}  // namespace kzs_cli

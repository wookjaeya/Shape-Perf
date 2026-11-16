// kz-static -- the static analysis front end.
//
//   kz-static basis   (--profile P [--profile P2 ...] | --no-profile) --compile-db F --build-dir D --bundle-root R [--recipe F] [--link F] -o OUT [--text]
//   kz-static extract (--profile P [--profile P2 ...] | --no-profile) (--basis B | --source S... [-- <compile flags>]) -o OUT [options] [--text]
//   kz-static show    <container> [--section S]... [--loc Type.field] [--callee NAME] [--full-hashes]
//   kz-static reach   <facts> --root NAME... --loc Type.field [--callee NAME] [--follow-spawn]
//   kz-static relations <facts> [-o OUT] [--keys KEYS]
//   kz-static order   <facts> [-o OUT] [--edges TSV] [--caller-dependent-report FILE]
//   kz-static plan    <facts> [--basis B] --framework F --select observation-set --order method-priority --budget SPEC
//                     --budget-reason TEXT [--budget-disclosure TEXT] -o MANIFEST --sidecar SIDECAR --modules-out TABLE --exes-out LIST
//   kz-static --identity
//
// Exit status: 0 = ok, 1 = the analysis failed (a unit does not parse, a container is damaged, the reading options differ
// from those of the basis without --accept-option-difference),
// 2 = usage or I/O error.  See docs/static_facts.md.

#include <cstdio>
#include <cstring>
#include <string>

#include "cli.hpp"
#include "commands.hpp"
#include "kz/core/container.hpp"
#include "kz/core/dump.hpp"
#include "kz/core/identity.hpp"

namespace kzs_cli {

bool parse_args(int argc, char** argv, int first, const std::set<std::string>& valued, const std::set<std::string>& boolean, Args* out, std::string* error) {
  for (int i = first; i < argc; ++i) {
    const std::string a = argv[i];
    if (a == "--") {
      for (int k = i + 1; k < argc; ++k) out->after_dashes.push_back(argv[k]);
      return true;
    }
    if (a.size() >= 2 && a[0] == '-' && !(a.size() > 1 && a[1] >= '0' && a[1] <= '9')) {
      if (valued.count(a) != 0) {
        if (i + 1 >= argc) {
          *error = "option " + a + " needs a value";
          return false;
        }
        out->values[a].push_back(argv[++i]);
      } else if (boolean.count(a) != 0) {
        out->flags.insert(a);
      } else {
        *error = "unknown option " + a;
        return false;
      }
      continue;
    }
    out->positional.push_back(a);
  }
  return true;
}

bool load_profile(const std::vector<std::string>& paths, kz::phi::Profile* profile) {
  std::string diag;
  const kz::Status s = kzs::load_profile_paths(paths, profile, &diag);
  if (!diag.empty()) std::fputs(diag.c_str(), stderr);
  if (!s.ok()) {
    std::string path;
    for (const std::string& x : paths) path += (path.empty() ? "" : " ") + x;
    std::fprintf(stderr, "kz-static: profile %s: %s\n", path.c_str(), s.describe().c_str());
    return false;
  }
  return true;
}

bool read_text_file(const std::string& path, std::string* out) {
  kz::Bytes b;
  if (!kz::read_whole_file(path, &b).ok()) return false;
  out->assign(b.begin(), b.end());
  return true;
}

std::string resource_dir(const Args& args) {
  if (args.has("--resource-dir")) return args.one("--resource-dir");
  return KZS_DEFAULT_RESOURCE_DIR;
}

int fail(const char* what, const std::string& message) {
  std::fprintf(stderr, "kz-static %s: %s\n", what, message.c_str());
  return 1;
}

}  // namespace kzs_cli

namespace {

int usage() {
  std::fputs(
      "usage: kz-static basis   (--profile P | --no-profile) --compile-db F --build-dir D --bundle-root R [--recipe F] [--link F]\n"
      "                         [--resource-dir D] [--extra-arg A]... -o OUT [--text]\n"
      "       kz-static extract (--profile P | --no-profile) (--basis B | --source S... -- <compile flags>) -o OUT\n"
      "                         [--root R] [--target Type.field]... [--delay-op N]... [--lock-op N]... [--unlock-op N]...\n"
      "                         [--spawn-op N]... [--terminal-op N]... [--poll-fn N]... [--resource-dir D] [--extra-arg A]...\n"
      "                         [--extended-facts] [--argflow-depth N|none] [--argflow-size N|none] [--image-budget N]\n"
      "                         [--targets-rule R-T1,R-T2] [--accept-option-difference] [--object-access]\n"
      "                         [--semantics FILE]... [--result-flow] [--loop-defs] [--lock-handles]\n"
      "                         [--text] [--section S]... [--verbose]\n"
      "                         (--no-profile: the built-in profile and the P0 table only, no vocabulary options;\n"
      "                         delay, terminal and spawn operations derived through wrappers; --extended-facts:\n"
      "                         the sections READS, RETURNS, RET_CONSTS, LOOP_EXITS, ARGFLOW, PARAM_FLOW, PTR_FIELDS\n"
      "                         and GLOBAL_IMAGES, always on with --no-profile; --argflow-depth, --argflow-size: the\n"
      "                         bounds of the PARAM_FLOW composition, none by default; --image-budget: the leaves of\n"
      "                         one static image, 4096 by default; --targets-rule, with --no-profile only: the targets\n"
      "                         chosen by rule from the first pass, shared bases only; --object-access, with the\n"
      "                         extended facts: the sections OBJECT_ACCESS, FIELD_TYPES and ARG_TYPES and the reading\n"
      "                         rules that come with them, docs/static_facts.md 5s; --semantics, with the extended\n"
      "                         facts: the semantics data files joined to the build as the section SEMANTICS, 5t;\n"
      "                         --result-flow, with --no-profile: the sections RESULT_FLOW and RESULT_EFFECT, 5u;\n"
      "                         --loop-defs, with --no-profile: the section LOOP_DEFS, the local definitions a wait's\n"
      "                         condition reads and the predicate through them; --lock-handles, with --no-profile: the\n"
      "                         section LOCK_HANDLES, lock identity by handle, 5v)\n"
      "       kz-static show    <container> [--section S]... [--loc Type.field] [--callee NAME] [--full-hashes]\n"
      "       kz-static reach   <facts> --root NAME... --loc Type.field [--callee NAME] [--follow-spawn]\n"
      "       kz-static relations <facts> [-o OUT] [--keys KEYS]   (paths A/B/C and the outcome-flip signal; KEYS: the use-site key of each row; no field, callee or case name)\n"
      "       kz-static order   <facts> [-o OUT] [--edges TSV] [--caller-dependent-report FILE]   (order edges with kinds and premises, waits, and the static order of each relations row; no field, callee or case name)\n"
      "       kz-static plan    <facts> [--basis B] --framework F --select observation-set --order method-priority\n"
      "                         --budget tier:D07|tier:directed|all|rows:<n> --budget-reason TEXT [--budget-disclosure TEXT]\n"
      "                         -o MANIFEST --sidecar SIDECAR --modules-out TABLE --exes-out LIST\n"
      "                         (an observation plan derived by rule: the method's observation set in the order of its\n"
      "                         priority under the budget, as a kzs-sites 3 manifest with a sidecar; docs/plan_manifest.md)\n"
      "       kz-static --identity\n",
      stderr);
  return 2;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) return usage();
  const std::string cmd = argv[1];
  if (cmd == "--identity" && argc == 2) {
    std::printf("tool=kz-static\ntool_sha256=%s\ntool_source=%s\ncore_source=%s\nschema_revision=%u\nschema_fingerprint=%s\n",
                kz::to_hex(KZ_THIS_TOOL_HASH("kz-static")).c_str(), KZ_TOOL_SOURCE_SHA256, std::string(kz::core_source_identity()).c_str(),
                static_cast<unsigned>(KZ_SCHEMA_REVISION), kz::to_hex(kz::schema_fingerprint()).c_str());
    return 0;
  }
  if (cmd == "basis") return kzs_cli::cmd_basis(argc, argv);
  if (cmd == "extract") return kzs_cli::cmd_extract(argc, argv);
  if (cmd == "show") return kzs_cli::cmd_show(argc, argv);
  if (cmd == "reach") return kzs_cli::cmd_reach(argc, argv);
  if (cmd == "relations") return kzs_cli::cmd_relations(argc, argv);
  if (cmd == "order") return kzs_cli::cmd_order(argc, argv);
  if (cmd == "plan") return kzs_cli::cmd_plan(argc, argv);
  return usage();
}

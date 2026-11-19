// kz-static plan -- an observation plan derived by rule from one facts container (plan_gen.hpp,
// docs/plan_manifest.md).
//
//   kz-static plan <facts> [--basis B] --framework F --select observation-set --order method-priority
//                  --budget tier:D07|tier:directed|all|rows:<n> --budget-reason TEXT [--budget-disclosure TEXT]
//                  -o MANIFEST --sidecar SIDECAR --modules-out TABLE --exes-out LIST
//                  [--manifest 3|4]  (4: plan rules revisions 3 to 5, kzs-sites 4; with it --control-budget all|rows:<n>
//                  --control-budget-reason TEXT --requests-out REQUESTS [--namespaces F]..., docs/plan_manifest.md 7-8)
//
// The command opens the files named on its command line and no other: the facts, the basis the facts were read under
// (required when the facts name one; refused when it is another), the framework-layer data file.  Each is listed in the
// sidecar with the sha256 of the bytes read.  It takes no field, callee or case name.
#include <cstdio>

#include "commands.hpp"
#include "plan_gen.hpp"
#include "kz/core/identity.hpp"
#include "kz/core/sha256.hpp"

namespace kzs_cli {

int cmd_plan(int argc, char** argv) {
  Args args;
  std::string err;
  const std::set<std::string> valued = {"--basis", "--framework", "--select", "--order", "--budget", "--budget-reason", "--budget-disclosure", "-o", "--sidecar", "--modules-out", "--exes-out",
                                        "--manifest", "--control-budget", "--control-budget-reason", "--namespaces", "--requests-out"};
  if (!parse_args(argc, argv, 2, valued, {}, &args, &err) || args.positional.size() != 1) {
    std::fprintf(stderr, "kz-static plan: %s\n", err.empty() ? "expected one facts file" : err.c_str());
    return 2;
  }
  for (const char* need : {"--framework", "--select", "--order", "--budget", "--budget-reason", "-o", "--sidecar", "--modules-out", "--exes-out"})
    if (!args.has(need)) {
      std::fprintf(stderr, "kz-static plan: %s is required\n", need);
      return 2;
    }
  for (const auto& [k, v] : args.values)
    if (v.size() > 1 && k != "--namespaces") {
      std::fprintf(stderr, "kz-static plan: %s given twice\n", k.c_str());
      return 2;
    }
  kzs::PlanOptions o;
  o.select = args.one("--select");
  o.order = args.one("--order");
  o.budget = args.one("--budget");
  o.budget_reason = args.one("--budget-reason");
  o.budget_disclosure = args.one("--budget-disclosure");
  o.sidecar_path = args.one("--sidecar");
  if (args.has("--manifest")) {
    const std::string mv = args.one("--manifest");
    if (mv != "3" && mv != "4") {
      std::fprintf(stderr, "kz-static plan: --manifest is 3 or 4\n");
      return 2;
    }
    o.manifest_revision = mv == "4" ? 4u : 3u;
  }
  if (args.has("--control-budget")) o.control_budget = args.one("--control-budget");
  if (args.has("--control-budget-reason")) o.control_budget_reason = args.one("--control-budget-reason");
  if (args.has("--requests-out")) o.requests_path = args.one("--requests-out");
  o.generator = std::string("tool=kz-static tool_sha256=") + kz::to_hex(KZ_THIS_TOOL_HASH("kz-static")) + " tool_source=" + KZ_TOOL_SOURCE_SHA256 +
                " core_source=" + std::string(kz::core_source_identity());
  // the facts
  kz::Container facts;
  kz::Status s = kz::Container::read_file(args.positional[0], &facts);
  if (s.ok()) s = facts.verify_content();
  if (!s.ok()) {
    std::fprintf(stderr, "kz-static plan: %s: %s\n", args.positional[0].c_str(), s.describe().c_str());
    return 1;
  }
  o.inputs.push_back({"facts", args.positional[0], kz::to_hex(facts.artifact_hash())});
  // the basis, only when it is named
  kz::Container basis;
  const bool have_basis = args.has("--basis");
  if (have_basis) {
    s = kz::Container::read_file(args.one("--basis"), &basis);
    if (s.ok()) s = basis.verify_content();
    if (!s.ok()) {
      std::fprintf(stderr, "kz-static plan: %s: %s\n", args.one("--basis").c_str(), s.describe().c_str());
      return 1;
    }
    if (basis.header().artifact_kind != KZ_ARTIFACT_BASIS) {
      std::fprintf(stderr, "kz-static plan: %s is not a basis container\n", args.one("--basis").c_str());
      return 2;
    }
    o.inputs.push_back({"basis", args.one("--basis"), kz::to_hex(basis.artifact_hash())});
  }
  // the framework-layer data file
  kz::Bytes fb;
  if (!kz::read_whole_file(args.one("--framework"), &fb).ok()) {
    std::fprintf(stderr, "kz-static plan: cannot read %s\n", args.one("--framework").c_str());
    return 2;
  }
  const std::string framework(fb.begin(), fb.end());
  o.inputs.push_back({"framework-layer-data", args.one("--framework"), kz::to_hex(kz::sha256(framework))});
  // the namespace data files (manifest revision 4), in the order given
  if (args.has("--namespaces"))
    for (const std::string& np : args.values.at("--namespaces")) {
      kz::Bytes nb;
      if (!kz::read_whole_file(np, &nb).ok()) {
        std::fprintf(stderr, "kz-static plan: cannot read %s\n", np.c_str());
        return 2;
      }
      const std::string nt(nb.begin(), nb.end());
      o.inputs.push_back({"namespace-data", np, kz::to_hex(kz::sha256(nt))});
      o.namespace_data.emplace_back(np, nt);
    }
  kzs::PlanOutput out;
  s = kzs::plan_generate(facts, have_basis ? &basis : nullptr, framework, o, &out);
  if (!s.ok()) {
    std::fprintf(stderr, "kz-static plan: %s\n", s.describe().c_str());
    return 2;
  }
  // the value requests first (the sidecar names their sha256), then the sidecar, then the files that name its sha256
  if (o.manifest_revision == 4) {
    const kz::Status w = kz::write_whole_file(args.one("--requests-out"), kz::Bytes(out.requests.begin(), out.requests.end()));
    if (!w.ok()) {
      std::fprintf(stderr, "kz-static plan: %s\n", w.describe().c_str());
      return 2;
    }
  }
  const std::pair<const char*, const std::string*> files[] = {{"--sidecar", &out.sidecar}, {"-o", &out.manifest}, {"--modules-out", &out.modules}, {"--exes-out", &out.exes}};
  for (const auto& f : files) {
    const kz::Status w = kz::write_whole_file(args.one(f.first), kz::Bytes(f.second->begin(), f.second->end()));
    if (!w.ok()) {
      std::fprintf(stderr, "kz-static plan: %s\n", w.describe().c_str());
      return 2;
    }
  }
  return 0;
}

}  // namespace kzs_cli

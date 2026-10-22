// kz-static show / reach -- reading a BASIS or FACTS container.
#include <cstdio>

#include "basis.hpp"
#include "commands.hpp"
#include "facts_query.hpp"
#include "facts_text.hpp"
#include "kz/core/dump.hpp"

namespace kzs_cli {

namespace {

bool open_container(const char* what, const std::string& path, kz::Container* c) {
  kz::Status s = kz::Container::read_file(path, c);
  if (s.ok()) s = c->verify_content();
  if (!s.ok()) {
    std::fprintf(stderr, "kz-static %s: %s: %s\n", what, path.c_str(), s.describe().c_str());
    return false;
  }
  return true;
}

}  // namespace

int cmd_show(int argc, char** argv) {
  Args args;
  std::string err;
  if (!parse_args(argc, argv, 2, {"--section", "--loc", "--callee"}, {"--full-hashes"}, &args, &err) || args.positional.size() != 1) {
    std::fprintf(stderr, "kz-static show: %s\n", err.empty() ? "expected one container file" : err.c_str());
    return 2;
  }
  kz::Container c;
  if (!open_container("show", args.positional[0], &c)) return 1;
  if (c.header().artifact_kind == KZ_ARTIFACT_BASIS) {
    std::fputs(kzs::basis_items_text(c).c_str(), stdout);
    return 0;
  }
  kzs::TextOptions to;
  for (const std::string& sec : args.many("--section")) to.sections.insert(sec);
  to.loc = args.one("--loc");
  to.callee = args.one("--callee");
  to.full_hashes = args.has("--full-hashes");
  std::string text;
  const kz::Status s = kzs::facts_text(c, to, &text);
  if (!s.ok()) return fail("show", s.describe());
  std::fputs(text.c_str(), stdout);
  return 0;
}

int cmd_reach(int argc, char** argv) {
  Args args;
  std::string err;
  if (!parse_args(argc, argv, 2, {"--root", "--loc", "--callee"}, {"--follow-spawn"}, &args, &err) || args.positional.size() != 1 || !args.has("--root") || !args.has("--loc")) {
    std::fprintf(stderr, "kz-static reach: %s\n", err.empty() ? "expected one facts file, --root and --loc" : err.c_str());
    return 2;
  }
  kz::Container c;
  if (!open_container("reach", args.positional[0], &c)) return 1;
  kzs::ReachQuery q;
  q.roots = args.many("--root");
  q.loc = args.one("--loc");
  q.callee = args.one("--callee");
  q.follow_spawn = args.has("--follow-spawn");
  std::string text;
  const kz::Status s = kzs::reach_report(c, q, &text);
  if (!s.ok()) return fail("reach", s.describe());
  std::fputs(text.c_str(), stdout);
  return 0;
}

}  // namespace kzs_cli

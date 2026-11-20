// kz-static show / reach / relations / order -- reading a BASIS or FACTS container.
#include <cstdio>

#include "basis.hpp"
#include "commands.hpp"
#include "facts_query.hpp"
#include "facts_text.hpp"
#include "order.hpp"
#include "relations.hpp"
#include "kz/core/dump.hpp"
#include "kz/core/identity.hpp"
#include "kz/core/sha256.hpp"

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

namespace {
// the tool line of the relations and order headers and of the pair file (static_facts.md 5x): the two values of the plan
// sidecar's generator line
std::string tool_line() {
  return std::string("tool kz-static tool_sha256=") + kz::to_hex(KZ_THIS_TOOL_HASH("kz-static")) + " tool_source=" + KZ_TOOL_SOURCE_SHA256;
}
}  // namespace

// The relations of a facts container (relations.hpp).  It takes no field, callee or case name: the facts
// file, the output file and the file of the use-site key listing only.
int cmd_relations(int argc, char** argv) {
  Args args;
  std::string err;
  if (!parse_args(argc, argv, 2, {"-o", "--keys"}, {}, &args, &err) || args.positional.size() != 1) {
    std::fprintf(stderr, "kz-static relations: %s\n", err.empty() ? "expected one facts file" : err.c_str());
    return 2;
  }
  kz::Container c;
  if (!open_container("relations", args.positional[0], &c)) return 1;
  if (c.header().artifact_kind != KZ_ARTIFACT_FACTS) {
    std::fprintf(stderr, "kz-static relations: %s is not a facts container\n", args.positional[0].c_str());
    return 2;
  }
  kzs::RelReport rep;
  kzs::RelationsOptions ro;
  ro.tool_line = tool_line();
  const kz::Status s = kzs::relations_rows(c, ro, &rep);
  if (!s.ok()) return fail("relations", s.describe());
  const std::string text = kzs::render_relations(rep);
  if (args.has("--keys")) {
    const std::string keys = kzs::relation_keys_text(rep);
    const kz::Status w = kz::write_whole_file(args.one("--keys"), kz::Bytes(keys.begin(), keys.end()));
    if (!w.ok()) {
      std::fprintf(stderr, "kz-static relations: %s\n", w.describe().c_str());
      return 2;
    }
  }
  if (args.has("-o")) {
    const kz::Status w = kz::write_whole_file(args.one("-o"), kz::Bytes(text.begin(), text.end()));
    if (!w.ok()) {
      std::fprintf(stderr, "kz-static relations: %s\n", w.describe().c_str());
      return 2;
    }
  } else {
    std::fputs(text.c_str(), stdout);
  }
  return 0;
}

// The order edges of a facts container and the static order of each relations row (order.hpp).  It takes no field, callee
// or case name: the facts file, the output file, the file of the edges, one per line, and the complete pair file.
int cmd_order(int argc, char** argv) {
  Args args;
  std::string err;
  if (!parse_args(argc, argv, 2, {"-o", "--edges", "--caller-dependent-report", "--pairs"}, {}, &args, &err) || args.positional.size() != 1) {
    std::fprintf(stderr, "kz-static order: %s\n", err.empty() ? "expected one facts file" : err.c_str());
    return 2;
  }
  kz::Container c;
  if (!open_container("order", args.positional[0], &c)) return 1;
  if (c.header().artifact_kind != KZ_ARTIFACT_FACTS) {
    std::fprintf(stderr, "kz-static order: %s is not a facts container\n", args.positional[0].c_str());
    return 2;
  }
  std::string text, tsv, cdr, pairs;
  kzs::OrderOptions oo;
  if (args.has("--caller-dependent-report")) oo.caller_dependent_report = &cdr;
  oo.tool_line = tool_line();
  if (args.has("--pairs")) oo.pairs = &pairs;
  const kz::Status s = kzs::order_report(c, oo, &text, &tsv);
  if (!s.ok()) return fail("order", s.describe());
  if (args.has("--pairs")) {
    const kz::Status w = kz::write_whole_file(args.one("--pairs"), kz::Bytes(pairs.begin(), pairs.end()));
    if (!w.ok()) {
      std::fprintf(stderr, "kz-static order: %s\n", w.describe().c_str());
      return 2;
    }
  }
  if (args.has("--caller-dependent-report")) {
    const kz::Status w = kz::write_whole_file(args.one("--caller-dependent-report"), kz::Bytes(cdr.begin(), cdr.end()));
    if (!w.ok()) {
      std::fprintf(stderr, "kz-static order: %s\n", w.describe().c_str());
      return 2;
    }
  }
  if (args.has("--edges")) {
    const kz::Status w = kz::write_whole_file(args.one("--edges"), kz::Bytes(tsv.begin(), tsv.end()));
    if (!w.ok()) {
      std::fprintf(stderr, "kz-static order: %s\n", w.describe().c_str());
      return 2;
    }
  }
  if (args.has("-o")) {
    const kz::Status w = kz::write_whole_file(args.one("-o"), kz::Bytes(text.begin(), text.end()));
    if (!w.ok()) {
      std::fprintf(stderr, "kz-static order: %s\n", w.describe().c_str());
      return 2;
    }
  } else {
    std::fputs(text.c_str(), stdout);
  }
  return 0;
}

}  // namespace kzs_cli

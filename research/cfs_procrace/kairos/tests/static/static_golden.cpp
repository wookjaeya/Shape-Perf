// static_golden -- golden files of the static extraction.
//
//   static_golden fixture <case> <kz-static> <fixtures-dir> <profile-dir> <data-dir> <work-dir>
//   static_golden core    <kz-static> <profile-dir> <recipe> <bundle> <data-dir> <work-dir>
//
// `fixture` extracts the facts of one self-contained C fixture (tests/static/fixtures) with the
// command line of the case, checks that two runs give the same bytes and that the container
// verifies, and compares the text of the run with data/<case>.golden.  `core` does the same for
// the real translation units of the core executable of the pinned bundle (basis first); it exits
// 77 (skipped) when the bundle or its build is not there.
//
// With KZ_GOLDEN_UPDATE=1 in the environment the golden file is rewritten instead of compared;
// review the diff.  The values are those of the program under analysis, not experiment results.

#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "kz/core/container.hpp"

namespace {

struct Case {
  const char* name;
  const char* source;
  std::vector<std::string> options;   // extra kz-static options
  std::vector<std::string> sections;  // the sections of the text view
};

// The sections that carry facts; `basis` (tool hash, counts) is left out, it changes with the tool.
const std::vector<std::string> kFactSections = {"files", "functions", "calls", "derives", "loops", "iters", "order", "init", "consts", "writers", "atype", "locs", "targets", "spawnparams", "image", "terminals", "outcalls", "fwrites"};

const std::vector<Case>& cases() {
  static const std::vector<Case> all = {
      {"writers", "writers.c", {}, kFactSections},
      {"outptr", "outptr.c", {"--target", "Slot.id"}, kFactSections},
      {"derives", "derives.c", {}, kFactSections},
      {"loops", "loops.c", {"--target", "Gate.state", "--target", "Rec.state", "--target", "Sys.ready", "--target", "Sys.phase", "--delay-op", "fx_delay", "--lock-op", "fx_lock", "--unlock-op", "fx_unlock"}, kFactSections},
      {"spawn", "spawn.c", {}, kFactSections},
      {"atype", "atype.c", {"--target", "Plain.id", "--target", "Cast.id", "--target", "Esc.id", "--target", "Mix.id", "--target", "Inner.id", "--target", "Safe.id"}, kFactSections},
  };
  return all;
}

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

bool read_file(const std::string& path, std::string* out) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return false;
  std::ostringstream ss;
  ss << in.rdbuf();
  *out = ss.str();
  return true;
}

std::string quote(const std::string& s) { return "'" + s + "'"; }

// The first line of the run output is "facts <hash>": the hash depends on the tool, so it is dropped.
std::string without_first_line(const std::string& s) {
  const size_t nl = s.find('\n');
  return nl == std::string::npos ? std::string() : s.substr(nl + 1);
}

int compare(const std::string& name, const std::string& got, const std::string& data_dir) {
  const std::string path = data_dir + "/" + name + ".golden";
  const char* upd = std::getenv("KZ_GOLDEN_UPDATE");
  if (upd != nullptr && std::strcmp(upd, "1") == 0) {
    std::ofstream out(path, std::ios::binary);
    out << got;
    std::printf("updated %s\n", path.c_str());
    return 0;
  }
  std::string want;
  if (!read_file(path, &want)) {
    std::fprintf(stderr, "missing golden file %s (KZ_GOLDEN_UPDATE=1 writes it)\n", path.c_str());
    return 1;
  }
  if (want == got) {
    std::printf("%s: matches\n", name.c_str());
    return 0;
  }
  std::fprintf(stderr, "%s: differs from %s\n", name.c_str(), path.c_str());
  std::istringstream a(want), b(got);
  std::string la, lb;
  for (int n = 1;; ++n) {
    const bool ga = static_cast<bool>(std::getline(a, la)), gb = static_cast<bool>(std::getline(b, lb));
    if (!ga && !gb) break;
    if (!ga || !gb || la != lb) {
      std::fprintf(stderr, "  line %d\n    golden: %s\n    got   : %s\n", n, ga ? la.c_str() : "(end)", gb ? lb.c_str() : "(end)");
      break;
    }
  }
  const std::string actual = name + ".actual";
  std::ofstream out(actual, std::ios::binary);
  out << got;
  std::fprintf(stderr, "  full output written to %s\n", actual.c_str());
  return 1;
}

bool verify(const std::string& path, kz::Container* c) {
  kz::Status s = kz::Container::read_file(path, c);
  if (s.ok()) s = c->verify_content();
  if (!s.ok()) {
    std::fprintf(stderr, "%s does not verify: %s\n", path.c_str(), s.describe().c_str());
    return false;
  }
  return true;
}

std::string sections_args(const std::vector<std::string>& sections) {
  std::string a;
  for (const std::string& s : sections) a += " --section " + s;
  return a;
}

int run_fixture(const Case& c, const std::string& tool, const std::string& fixtures, const std::string& profile, const std::string& data, const std::string& work) {
  ::mkdir(work.c_str(), 0777);
  const std::string out1 = work + "/" + c.name + ".kz", out2 = work + "/" + c.name + ".again.kz";
  std::string options;
  for (const std::string& o : c.options) options += " " + quote(o);
  auto extract = [&](const std::string& out, std::string* log) {
    return run(quote(tool) + " extract --profile " + quote(profile) + " --source " + quote(fixtures + "/" + c.source) + " --root " + quote(fixtures) + " -o " + quote(out) +
                   options + " -- -std=c99 -pedantic",
               log);
  };
  std::string log1, log2;
  if (extract(out1, &log1) != 0) {
    std::fprintf(stderr, "extract failed:\n%s\n", log1.c_str());
    return 1;
  }
  if (extract(out2, &log2) != 0) {
    std::fprintf(stderr, "second extract failed:\n%s\n", log2.c_str());
    return 1;
  }
  std::string b1, b2;
  if (!read_file(out1, &b1) || !read_file(out2, &b2) || b1 != b2) {
    std::fprintf(stderr, "%s: two runs give different containers\n", c.name);
    return 1;
  }
  kz::Container container;
  if (!verify(out1, &container)) return 1;
  std::string view;
  if (run(quote(tool) + " show " + quote(out1) + sections_args(c.sections), &view) != 0) {
    std::fprintf(stderr, "show failed:\n%s\n", view.c_str());
    return 1;
  }
  return compare(c.name, without_first_line(log1) + view, data);
}

int run_core(const std::string& tool, const std::string& profile, const std::string& recipe, const std::string& bundle, const std::string& data, const std::string& work) {
  struct stat st;
  const std::string build = bundle + "/build-native_std";
  if (::stat((build + "/native/default_cpu1/compile_commands.json").c_str(), &st) != 0) {
    std::printf("skipped: the build of the pinned bundle is not at %s\n", build.c_str());
    return 77;
  }
  ::mkdir(work.c_str(), 0777);
  const std::string basis = work + "/core_basis.kz", facts = work + "/core_facts.kz", facts2 = work + "/core_facts_again.kz";
  std::string log;
  if (run(quote(tool) + " basis --profile " + quote(profile) + " --compile-db " + quote(build + "/native/default_cpu1/compile_commands.json") + " --build-dir " + quote(build) +
              " --bundle-root " + quote(bundle) + " --recipe " + quote(recipe) + " -o " + quote(basis),
          &log) != 0) {
    std::fprintf(stderr, "basis failed:\n%s\n", log.c_str());
    return 1;
  }
  const std::string extract = quote(tool) + " extract --profile " + quote(profile) + " --basis " + quote(basis) +
                              " --delay-op OS_TaskDelay --lock-op CFE_ES_LockSharedData --unlock-op CFE_ES_UnlockSharedData --spawn-op CFE_ES_CreateChildTask -o ";
  std::string log1;
  if (run(extract + quote(facts), &log1) != 0) {
    std::fprintf(stderr, "extract failed:\n%s\n", log1.c_str());
    return 1;
  }
  if (run(extract + quote(facts2), &log) != 0) {
    std::fprintf(stderr, "second extract failed:\n%s\n", log.c_str());
    return 1;
  }
  std::string b1, b2;
  if (!read_file(facts, &b1) || !read_file(facts2, &b2) || b1 != b2) {
    std::fprintf(stderr, "core: two runs give different containers\n");
    return 1;
  }
  kz::Container cb, cf;
  if (!verify(basis, &cb) || !verify(facts, &cf)) return 1;
  // the facts container names the basis it was read under
  if (cf.basis_hash() != cb.artifact_hash()) {
    std::fprintf(stderr, "core: the facts container does not carry the artifact hash of the basis\n");
    return 1;
  }

  // The numbers: the analysis basis, the summary of the run, the writers of the identity field
  // with their INIT image, the loops of the core serialization and their certificate-view rules,
  // the order the core table walks establish, the spawn facts and the derives of the event calls.
  std::string text;
  {
    // the basis items; `front_end.build_identity` names the compiler that built this tool and
    // is left out, so that the golden is the same for every build of the tool
    std::string v;
    if (run(quote(tool) + " show " + quote(basis), &v) != 0) {
      std::fprintf(stderr, "show basis failed:\n%s\n", v.c_str());
      return 1;
    }
    text += "# analysis basis\n";
    std::istringstream in(v);
    std::string l;
    while (std::getline(in, l))
      if (l.rfind("front_end.build_identity ", 0) != 0) text += l + "\n";
  }
  text += "\n# extraction summary\n" + without_first_line(log1);
  auto show = [&](const std::string& args, const char* title) {
    std::string v;
    if (run(quote(tool) + " show " + quote(facts) + " " + args, &v) != 0) {
      std::fprintf(stderr, "show %s failed:\n%s\n", args.c_str(), v.c_str());
      return false;
    }
    text += std::string("\n# ") + title + "\n" + without_first_line(v);
    return true;
  };
  // Only the loops that wait or walk a table; the other loops of the core are counted in the summary.
  auto show_loops = [&]() {
    std::string v;
    if (run(quote(tool) + " show " + quote(facts) + " --section loops", &v) != 0) {
      std::fprintf(stderr, "show loops failed:\n%s\n", v.c_str());
      return false;
    }
    text += "\n# loops that walk a table or wait\n";
    std::istringstream in(v);
    std::string l;
    bool keep = false;
    while (std::getline(in, l)) {
      if (l.rfind("  loop ", 0) == 0) keep = l.find(" table-walk ") != std::string::npos || l.find(" poll ") != std::string::npos || l.find(" count-bounded ") != std::string::npos;
      if (keep && l.rfind("  loop ", 0) == 0) text += l + "\n";
      else if (keep && l.rfind("      ", 0) == 0) text += l + "\n";
    }
    return true;
  };
  if (!show("--section locs --section init --section writers --section outcalls --loc CFE_SB_Global_t.AppId", "the identity field of the SB task")) return 1;
  if (!show("--section fwrites --loc CFE_SB_Global_t.AppId", "functions whose own body writes the identity field (or the whole record)")) return 1;
  if (!show_loops()) return 1;
  if (!show("--section order", "the order the table walks establish (unrolled iterations)")) return 1;
  if (!show("--section spawnparams", "parameters that carry the function a spawned task runs")) return 1;
  if (!show("--section image", "the images of the tables the loops walk")) return 1;
  if (!show("--section targets", "indirect call and spawn targets")) return 1;
  std::string derives;
  if (run(quote(tool) + " show " + quote(facts) + " --section derives --callee CFE_EVS_SendEventWithAppID", &derives) != 0) return 1;
  size_t from_sb = 0, rows = 0;
  std::map<std::string, size_t> per_file;
  std::vector<std::string> sb_rows;
  std::set<std::string> capture_points;  // file:line of the read of the field
  std::istringstream ds(derives);
  std::string line;
  while (std::getline(ds, line)) {
    if (line.find(" arg ") == std::string::npos) continue;
    ++rows;
    if (line.find("derived from CFE_SB_Global_t.AppId") == std::string::npos) continue;
    ++from_sb;
    const size_t chain = line.find(" chain=");
    sb_rows.push_back(chain == std::string::npos ? line : line.substr(0, chain));
    const size_t at = line.find(" read at ");
    if (at != std::string::npos) {
      std::string where = line.substr(at + 9);
      where = where.substr(0, where.find(' '));
      const size_t c1 = where.find(':'), c2 = where.find(':', c1 + 1);
      ++per_file[where.substr(0, c1)];
      capture_points.insert(where.substr(0, c2));
    }
  }
  text += "\n# derives rows of the event calls: " + std::to_string(rows) + ", of them derived from CFE_SB_Global_t.AppId: " + std::to_string(from_sb) + "\n";
  for (const auto& kv : per_file) text += "  " + kv.first + ": " + std::to_string(kv.second) + "\n";
  for (const std::string& r : sb_rows) text += r + "\n";
  // Cross-check against the gdb record (results/R2_gdb_v701.md section 6, recount of 2026-10-12):
  // 39 static SB call sites pass the field (cfe_sb_api.c 34, cfe_sb_priv.c 1, cfe_sb_task.c 4), and
  // the three sites executed before the publication read the field at cfe_sb_api.c:277,
  // cfe_sb_api.c:1185 and cfe_sb_priv.c:786.  Test data from that document, not an output of the tool.
  text += "# the violating sites of the gdb record among the derived sites\n";
  for (const char* site : {"cfe/modules/sb/fsw/src/cfe_sb_api.c:277", "cfe/modules/sb/fsw/src/cfe_sb_api.c:1185", "cfe/modules/sb/fsw/src/cfe_sb_priv.c:786"})
    text += std::string("  ") + site + ": " + (capture_points.count(site) != 0 ? "derived" : "NOT FOUND") + "\n";
  // The reachability of the event calls from the startup thread and from the core task mains, without and with the
  // spawn edges (counts per root; the per-site lines are left out).
  for (const char* follow : {"", " --follow-spawn"}) {
    std::string reach;
    if (run(quote(tool) + " reach " + quote(facts) +
                " --root CFE_ES_Main --root CFE_ES_TaskMain --root CFE_EVS_TaskMain --root CFE_SB_TaskMain --root CFE_TBL_TaskMain --root CFE_TIME_TaskMain"
                " --loc CFE_SB_Global_t.AppId --callee CFE_EVS_SendEventWithAppID" + follow,
            &reach) != 0) {
      std::fprintf(stderr, "reach failed:\n%s\n", reach.c_str());
      return 1;
    }
    std::istringstream rs(reach);
    text += std::string("\n# reachability of the event calls (counts per root)") + (*follow != 0 ? ", spawn edges followed" : ", spawn edges not followed") + "\n";
    while (std::getline(rs, line)) {
      if (line.rfind("  site ", 0) == 0) continue;
      text += line + "\n";
    }
  }
  return compare("core_cpu1", text, data);
}

}  // namespace

int main(int argc, char** argv) {
  if (argc == 8 && std::strcmp(argv[1], "fixture") == 0) {
    for (const Case& c : cases())
      if (std::strcmp(c.name, argv[2]) == 0) return run_fixture(c, argv[3], argv[4], argv[5], argv[6], argv[7]);
    std::fprintf(stderr, "unknown case %s\n", argv[2]);
    return 2;
  }
  if (argc == 8 && std::strcmp(argv[1], "core") == 0) return run_core(argv[2], argv[3], argv[4], argv[5], argv[6], argv[7]);
  std::fprintf(stderr,
               "usage: static_golden fixture <case> <kz-static> <fixtures-dir> <profile-dir> <data-dir> <work-dir>\n"
               "       static_golden core <kz-static> <profile-dir> <recipe> <bundle> <data-dir> <work-dir>\n");
  return 2;
}

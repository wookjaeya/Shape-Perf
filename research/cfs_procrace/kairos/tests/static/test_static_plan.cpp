// test_static_plan -- `kz-static plan` (plan rules revision 2, docs/plan_manifest.md) on the fixture plangen.c:
//
//   test_static_plan <kz-static> <fixtures-dir> <data-dir> <work-dir>
//
//   - the facts of plangen.c (extract --no-profile --targets-rule R-T1,R-T2), the plan at the budget tier:D07 with the
//     fixture's framework-layer data (data/plangen_framework.txt);
//   - two generations give the same bytes (manifest, sidecar, module table, executable list); a copy with one byte
//     changed fails the regeneration comparison;
//   - the manifest parses with the runtime's host parser (kzrt-manifest, kzs-sites 3), has no identity, control or bind
//     statement, every site has a label with its rule names and a ref, and the manifest's header names the sidecar's
//     sha256, as does the module table's first line;
//   - the method priority port on the fixture's rows (labels R and J, order class RELOC, disposition D07) and on rows
//     written here for the other classes;
//   - the observation rules on the fixture: the use of the lookup (at-call with the key as a pointer, after-call with the
//     result and the out value), the key text under the non-null test in the lookup API (the runtime's bound) and in the
//     create API (the bound of its own memchr), the creator, the handling chain (the return in the init function, the
//     result in the app, the store of the run status, the terminal call), the two calls on one line reported, not
//     planned, the lookup in a loop planned, the actor rule (creation call, start routine with its parameter, root
//     entry, the owner store after the spawn), the stop events of the framework-layer data;
//   - refusals: a framework-layer data line that is no entry (the generator reads no file it is not given), a missing
//     budget reason, a budget it does not know, a --basis that is no basis container; no output file is written;
//   - a second caller added later (-DPLANGEN_SECOND_CALLER) is planned by the same rules: every site key of the first
//     plan is in the second, and the new lookup has its use-call sites;
//   - plankey.c (plangen.c with the lookup's key in a character array tested before the call): with the facts of
//     extract --object-access the key at the use is a validated string bounded by the array's extent (ARG_TYPES), without
//     them a pointer; the same sites otherwise;
//   - the text of the plan (manifest and sidecar, the sha256 values and the work path masked) against
//     data/plangen_np.plan.golden (KZ_GOLDEN_UPDATE=1 rewrites it);
//   - plan rules revision 3 (--manifest 4, docs/plan_manifest.md sections 7-8) on planv4.c (facts of --object-access,
//     --semantics with data/planv4_fixture/, --result-flow, --loop-defs): without --manifest the output equals that of
//     --manifest 3; the kzs-sites 4 manifest parses with the host parser (plan header with the facts' artifact hash,
//     unique site keys, call columns and ordinals, row keys, the load of the path C read, the spawn binding with the
//     spawn-token reader, need and gap statements whose sites are declared); the sidecar's control budget line comes
//     before the row lines and names the control rows (the probe-create row as a, the rows with creator sites as b,
//     planned with the tier budget rows:0 too); a control budget of rows:1 cuts the rest (complete=no, need lines); the
//     creators excluded by key constants are creator sites; the wait rule (the call result the retry tests, the break),
//     the object rules (the subscript call of the read, the lock calls, the delete function and its retire call of the
//     namespace data), the effect call with the argument the semantics data names; the value requests are written and
//     named with their sha256; refusals (no control budget, a revision-3 option with --manifest 3, a namespace data
//     line that is no statement); the text of the revision-3 plan against data/planv4_np.plan.golden.
#include <sys/wait.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "kz/core/sha256.hpp"
#include "kz_rt_manifest.hpp"
#include "plan_gen.hpp"
#include "relations.hpp"
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

std::string read(const fs::path& p) {
  std::ifstream in(p, std::ios::binary);
  std::ostringstream os;
  os << in.rdbuf();
  return os.str();
}

void write(const fs::path& p, const std::string& t) {
  std::ofstream out(p, std::ios::binary);
  out << t;
}

std::string q(const std::string& s) { return "'" + s + "'"; }

bool has(const std::string& text, const std::string& part) { return text.find(part) != std::string::npos; }

struct Plan {
  std::string manifest, sidecar, modules, exes;
};

std::string plan_cmd(const std::string& tool, const fs::path& facts, const fs::path& framework, const fs::path& out, const std::string& extra = "") {
  return q(tool) + " plan " + q(facts.string()) + " --framework " + q(framework.string()) +
         " --select observation-set --order method-priority --budget tier:D07 --budget-reason 'the priority tier whole (test)'" + extra + " -o " +
         q((out / "plan.kzs").string()) + " --sidecar " + q((out / "plan.side").string()) + " --modules-out " + q((out / "plan.mod").string()) + " --exes-out " +
         q((out / "plan.exe").string());
}

bool generate(const std::string& cmd, const fs::path& out, Plan* p) {
  fs::create_directories(out);
  std::string log;
  const int rc = run(cmd, &log);
  if (rc != 0) {
    std::fprintf(stderr, "plan failed (rc %d): %s\n", rc, log.c_str());
    return false;
  }
  p->manifest = read(out / "plan.kzs");
  p->sidecar = read(out / "plan.side");
  p->modules = read(out / "plan.mod");
  p->exes = read(out / "plan.exe");
  return true;
}

std::set<std::string> site_keys(const std::string& sidecar) {
  std::set<std::string> k;
  std::istringstream in(sidecar);
  std::string l;
  static const std::regex re(R"re(^site \d+ \S+ key="([^"]*)")re");
  while (std::getline(in, l)) {
    std::smatch m;
    if (std::regex_search(l, m, re)) k.insert(m[1].str());
  }
  return k;
}

// the values that change with the tool or the place of the run: sha256 values, the input paths, the sidecar path
std::string mask(std::string t, const std::string& work) {
  t = std::regex_replace(t, std::regex("[0-9a-f]{64}"), "<sha256>");
  t = std::regex_replace(t, std::regex("path=\\S+"), "path=<path>");
  size_t at;
  while ((at = t.find(work)) != std::string::npos) t.replace(at, work.size(), "<work>/");
  return t;
}

kzs::RelRow row(char path, const std::string& head, const std::vector<std::string>& lines, const std::string& dep, const std::string& feas, const std::string& flip) {
  kzs::RelRow r;
  r.path = path;
  r.number = 1;
  r.head = head;
  for (const std::string& l : lines) r.lines.push_back(kzs::RelLine{"", l, 6});
  r.dependency = dep;
  r.run_order = "no record given";
  r.feasibility = feas;
  r.violation = "unconfirmed";
  r.flip = flip;
  r.layers = "B R";
  r.key = "0000000000000000";
  return r;
}

}  // namespace

int main(int argc, char** argv) {
  KZ_CHECK_EQ(argc, 5);
  if (argc != 5) return kztest::finish("test_static_plan");
  const std::string tool = argv[1];
  const fs::path fixtures = argv[2], data = argv[3], work = argv[4];
  std::error_code ec;
  fs::remove_all(work, ec);
  fs::create_directories(work);
  const fs::path framework = data / "plangen_framework.txt";
  std::string log;
  // ---- the facts
  const std::string extract = q(tool) + " extract --no-profile --source " + q((fixtures / "plangen.c").string()) + " --root " + q(fixtures.string()) + " --targets-rule R-T1,R-T2 -o ";
  KZ_CHECK_EQ(run(extract + q((work / "plangen.kz").string()) + " -- -std=c99 -pedantic", &log), 0);
  KZ_CHECK_EQ(run(extract + q((work / "plangen2.kz").string()) + " -- -std=c99 -pedantic -DPLANGEN_SECOND_CALLER", &log), 0);

  // ---- two generations, the same bytes; a changed byte is found
  Plan a, b;
  KZ_CHECK(generate(plan_cmd(tool, work / "plangen.kz", framework, work / "p1"), work / "p1", &a));
  KZ_CHECK(generate(plan_cmd(tool, work / "plangen.kz", framework, work / "p1b"), work / "p1b", &b));
  KZ_CHECK(a.manifest == b.manifest);
  KZ_CHECK(a.exes.substr(a.exes.find('\n')) == b.exes.substr(b.exes.find('\n')));
  KZ_CHECK(a.modules.substr(a.modules.find('\n')) == b.modules.substr(b.modules.find('\n')));
  KZ_CHECK(a.sidecar == b.sidecar);
  {
    std::string edited = a.manifest;
    edited[edited.size() / 2] = edited[edited.size() / 2] == 'x' ? 'y' : 'x';
    KZ_CHECK(edited != b.manifest);  // the regeneration comparison fails on a one-byte edit
  }
  // the manifest names the sidecar's sha256, and so do the module table and the executable list
  const std::string side_sha = kz::to_hex(kz::sha256(a.sidecar));
  KZ_CHECK(has(a.manifest, "# sidecar sha256=" + side_sha + "\n"));
  KZ_CHECK(a.modules.compare(0, 31, "# kz module table; plan sidecar") == 0 && has(a.modules.substr(0, a.modules.find('\n')), "sha256=" + side_sha));
  KZ_CHECK(has(a.exes.substr(0, a.exes.find('\n')), "sha256=" + side_sha));

  // ---- the manifest parses with the runtime's host parser
  kzrt::Manifest m;
  std::string perr;
  KZ_CHECK(kzrt::parse_manifest(a.manifest, "plan.kzs", &m, &perr));
  if (!perr.empty()) std::fprintf(stderr, "%s\n", perr.c_str());
  KZ_CHECK_EQ(m.version, 3u);
  KZ_CHECK(!m.sites.empty());
  KZ_CHECK(m.binds.empty() && m.equivs.empty());
  for (const kzrt::Site& s : m.sites) {
    KZ_CHECK(!s.has_identity && !s.has_control);
    KZ_CHECK(!s.ref.empty());
    KZ_CHECK(std::regex_match(s.label, std::regex(R"(s\d{4}\.[a-z-]+(\+[a-z-]+)*)")));
    KZ_CHECK(s.values.size() <= 4);
  }
  KZ_CHECK(!has(a.manifest, "\nidentity ") && !has(a.manifest, "\ncontrol ") && !has(a.manifest, "\nbind "));
  auto site_line = [&](const std::string& func, const std::string& place, const std::string& callee) {
    for (const kzrt::Site& s : m.sites)
      if (s.func == func && s.place == place && s.callee == callee) return &s;
    return static_cast<const kzrt::Site*>(nullptr);
  };

  // ---- the observation rules on the fixture
  const kzrt::Site* use_at = site_line("one_init", "at-call", "reg_find");
  const kzrt::Site* use_after = site_line("one_init", "after-call", "reg_find");
  KZ_CHECK(use_at != nullptr && use_after != nullptr);
  if (use_at != nullptr) {
    const kzrt::Value* k = use_at->value(0);
    KZ_CHECK(k != nullptr && k->from == "arg1" && k->string_mode == "pointer-only");
  }
  if (use_after != nullptr) {
    KZ_CHECK(use_after->value(0) != nullptr && use_after->value(0)->from == "ret");
    KZ_CHECK(use_after->value(1) != nullptr && use_after->value(1)->from == "out0");
  }
  const kzrt::Site* kt = site_line("reg_find", "at-call", "reg_search");
  KZ_CHECK(kt != nullptr && kt->value(0) != nullptr && kt->value(0)->string_mode == "validated" && kt->value(0)->bound == 64 && kt->value(0)->from == "arg0");
  const kzrt::Site* ckt = site_line("reg_create", "at-call", "memchr");
  KZ_CHECK(ckt != nullptr && ckt->value(0) != nullptr && ckt->value(0)->string_mode == "validated" && ckt->value(0)->bound == 16);
  KZ_CHECK(site_line("two_main", "at-call", "reg_create") != nullptr && site_line("two_main", "after-call", "reg_create") != nullptr);
  KZ_CHECK(site_line("one_main", "after-call", "one_init") != nullptr);           // the result the guard tests
  KZ_CHECK(site_line("one_main", "at-call", "app_exit") != nullptr);              // the terminal call
  bool store = false, owner = false;
  for (const kzrt::Site& s : m.sites) {
    if (s.func == "one_main" && s.place == "store" && s.src_line == 104) store = true;  // one.run_status = RUN_ERROR
    if (s.func == "spawn_task" && s.place == "store" && s.src_line == 84) owner = true;   // tasks[id].owner = owner
  }
  KZ_CHECK(store && owner);
  KZ_CHECK(site_line("one_main", "at-call", "reg_find") != nullptr);  // the lookup in the loop (line 107)
  for (const kzrt::Site& s : m.sites) KZ_CHECK(!(s.func == "one_main" && s.callee == "reg_find" && s.src_line == 106));  // two calls on one line
  KZ_CHECK(has(a.sidecar, "reason=record_insufficient(two calls of one callee on one line)"));
  const kzrt::Site* cr_at = site_line("spawn_task", "at-call", "pthread_create");
  KZ_CHECK(cr_at != nullptr && cr_at->value(0) != nullptr && cr_at->value(0)->role == "start_routine" && cr_at->value(1) != nullptr && cr_at->value(1)->role == "creation_arg");
  KZ_CHECK(site_line("spawn_task", "after-call", "pthread_create") != nullptr);
  bool start_one = false, start_two = false, run_loop = false, app_exit = false;
  for (const kzrt::Site& s : m.sites) {
    if (s.phase != "before") continue;
    if (s.func == "one_main" && s.value(0) != nullptr && s.value(0)->role == "param0") start_one = true;
    if (s.func == "two_main" && s.value(0) != nullptr) start_two = true;
    if (s.func == "run_loop" && has(s.label, "stop-event")) run_loop = true;
    if (s.func == "app_exit" && has(s.label, "stop-event")) app_exit = true;
  }
  KZ_CHECK(start_one && start_two && run_loop && app_exit);
  KZ_CHECK(has(a.sidecar, "\ncreation plangen.c:"));
  KZ_CHECK(has(a.sidecar, "wait-result not-planned reason=plan_missing(need not planned)"));
  KZ_CHECK(has(a.sidecar, "\nbudget value=tier:D07 reason="));
  KZ_CHECK(has(a.sidecar, "\ninput facts sha256=") && has(a.sidecar, "\ninput framework-layer-data sha256="));
  // every gap and need reason is one of the generator's table
  {
    std::set<std::string> reasons;
    for (const kzs::RelReason& r : kzs::plan_reasons()) reasons.insert(kzs::reason_text(r));
    std::istringstream in(a.sidecar);
    std::string l;
    static const std::regex re(R"( reason=(\S+\([^)]*\)))");
    size_t n = 0;
    while (std::getline(in, l)) {
      if (l.compare(0, 5, "need ") != 0 && l.compare(0, 4, "gap ") != 0) continue;
      std::smatch mm;
      KZ_CHECK(std::regex_search(l, mm, re));
      if (!mm.empty()) KZ_CHECK(reasons.count(mm[1].str()) == 1);
      ++n;
    }
    KZ_CHECK(n != 0);
  }

  // ---- the priority port: the fixture's rows, then rows for the other classes
  {
    KZ_CHECK(has(a.sidecar, " A1 tier=1 position=1 label=R class=decided order=RELOC reach=REACH-FLIP disposition=D07 state=planned"));
    KZ_CHECK(has(a.sidecar, " A2 tier=1 position=2 label=J class=decided order=RELOC"));
    kzs::RelReport rep;
    kzs::RelGroup g;
    g.rows.push_back(row('A', "lookup x.c:1:1 in f -> g#0 namespace T.k partition {} key known", {"actors: {f (spawn-entry)}; result=tested"}, "path A: 1 creator(s) of this partition in other actors",
                         "undecidable: the facts give no static order between the lookup's actor and the creators' actors", "static-possible: x; search returns: g returns -17 [E]"));
    g.rows.push_back(row('A', "lookup x.c:2:1 in f -> g#0 namespace T.k partition {} key known", {"actors: {f (spawn-entry)}; result=discarded"}, "path A: no creator left after exclusions",
                         "undecidable: no pair", "none: x; search returns: g returns 0 [OK]"));
    g.rows.push_back(row('B', "T.f -> h#1 sites=2 callee-check=yes", {"callee-check: h returns -5 [E] when (eq param:a#1 0)", "handling: discarded 2, used 0, tested 0"}, "path B",
                         "undecidable: no static order between the reader and the writer actors", "none: the two values give the same result"));
    g.rows.push_back(row('B', "T.f -> h#1 sites=1 callee-check=yes", {"callee-check: h returns -5 [E] when (eq param:a#1 0)", "handling: discarded 0, used 0, tested 1"}, "path B",
                         "undecidable: fixed unconfirmed: reader actors created before it: a; after it: none.", "static-undetermined: x"));
    g.rows.push_back(row('C', "spawn x.c:3:1 in m -> pthread_create; field T.s; read x.c:4:1 in r", {}, "path C", "possible: the read holds no lock",
                         "static-possible: r returns -3 [E] with the earlier value"));
    g.rows.push_back(row('C', "spawn x.c:3:1 in m -> pthread_create; field T.s; read x.c:5:1 in r", {}, "path C", "proven-impossible: the lock", "none: x"));
    rep.groups.push_back(g);
    const std::vector<kzs::PlanPriority> p = kzs::plan_priorities(rep);
    KZ_CHECK_EQ(p.size(), static_cast<size_t>(6));
    if (p.size() == 6) {
      KZ_CHECK(p[0].label == "T?" && p[0].order_class == "RELOC" && p[0].reach == "REACH-FLIP" && p[0].disposition == "D11" && p[0].tier == 2);
      KZ_CHECK(p[1].label == "0" && p[1].order_class == "NOPAIR" && p[1].disposition == "D05" && p[1].tier == 3);
      KZ_CHECK(p[2].label == "J" && p[2].order_class == "RELOC" && p[2].reach == "INDEPENDENT" && p[2].disposition == "D08" && p[2].tier == 2);
      KZ_CHECK(p[3].label == "T?" && p[3].order_class == "FIXED" && p[3].disposition == "D02" && p[3].tier == 3);
      KZ_CHECK(p[4].label == "F" && p[4].order_class == "RELOC" && p[4].reach == "REACH-STATED" && p[4].disposition == "D10" && p[4].tier == 2);
      KZ_CHECK(p[5].order_class == "ENF" && p[5].disposition == "D01" && p[5].tier == 3);
    }
  }

  // ---- refusals: no output is written
  {
    const fs::path bad = work / "bad_framework.txt", out = work / "refused";
    write(bad, read(framework) + "include other_file.txt\n");
    fs::create_directories(out);
    KZ_CHECK_EQ(run(plan_cmd(tool, work / "plangen.kz", bad, out), &log), 2);
    KZ_CHECK(has(log, "is not an entry"));
    KZ_CHECK(!fs::exists(out / "plan.kzs") && !fs::exists(out / "plan.side"));
    const std::string no_reason = q(tool) + " plan " + q((work / "plangen.kz").string()) + " --framework " + q(framework.string()) +
                                  " --select observation-set --order method-priority --budget tier:D07 -o " + q((out / "plan.kzs").string()) + " --sidecar " +
                                  q((out / "plan.side").string()) + " --modules-out " + q((out / "plan.mod").string()) + " --exes-out " + q((out / "plan.exe").string());
    KZ_CHECK_EQ(run(no_reason, &log), 2);
    std::string bad_budget = plan_cmd(tool, work / "plangen.kz", framework, out);
    bad_budget.replace(bad_budget.find("tier:D07"), 8, "tier:9");
    KZ_CHECK_EQ(run(bad_budget, &log), 2);
    KZ_CHECK_EQ(run(plan_cmd(tool, work / "plangen.kz", framework, out, " --basis " + q((work / "plangen2.kz").string())), &log), 2);
    KZ_CHECK(!fs::exists(out / "plan.kzs") && !fs::exists(out / "plan.side"));
  }

  // ---- a second caller added later: planned by the same rules
  {
    Plan c;
    KZ_CHECK(generate(plan_cmd(tool, work / "plangen2.kz", framework, work / "p2"), work / "p2", &c));
    const std::set<std::string> k1 = site_keys(a.sidecar), k2 = site_keys(c.sidecar);
    for (const std::string& k : k1)
      if (k2.count(k) == 0) kztest::fail(__FILE__, __LINE__, "a site of the first plan is not in the second: " + k);
    ++kztest::state().checks;
    KZ_CHECK(k2.count("plangen.c|three_main|point|at-call|reg_find|136") == 1);
    KZ_CHECK(k2.count("plangen.c|three_main|point|after-call|reg_find|136") == 1);
    KZ_CHECK(k2.count("plangen.c|three_main|before|||0") == 1);
  }

  // ---- plan rules revision 2: a key argument that is a character array tested before the call (plankey.c).  With the facts
  // of --object-access (ARG_TYPES) the key at the use is a validated string bounded by the array's extent; without them a
  // pointer, as before; the rest of the plan does not depend on it (the same site keys)
  {
    const std::string kext = q(tool) + " extract --no-profile --source " + q((fixtures / "plankey.c").string()) + " --root " + q(fixtures.string()) + " --targets-rule R-T1,R-T2";
    KZ_CHECK_EQ(run(kext + " --object-access -o " + q((work / "plankey_on.kz").string()) + " -- -std=c99 -pedantic", &log), 0);
    KZ_CHECK_EQ(run(kext + " -o " + q((work / "plankey_off.kz").string()) + " -- -std=c99 -pedantic", &log), 0);
    const fs::path kfw = data / "plankey_framework.txt";
    Plan on, off;
    KZ_CHECK(generate(plan_cmd(tool, work / "plankey_on.kz", kfw, work / "pk_on"), work / "pk_on", &on));
    KZ_CHECK(generate(plan_cmd(tool, work / "plankey_off.kz", kfw, work / "pk_off"), work / "pk_off", &off));
    kzrt::Manifest mon, moff;
    std::string e1, e2;
    KZ_CHECK(kzrt::parse_manifest(on.manifest, "on.kzs", &mon, &e1));
    KZ_CHECK(kzrt::parse_manifest(off.manifest, "off.kzs", &moff, &e2));
    auto key_at = [](const kzrt::Manifest& mm) -> const kzrt::Value* {
      for (const kzrt::Site& st : mm.sites)
        if (st.func == "one_init" && st.place == "at-call" && st.callee == "reg_find") return st.value(0);
      return nullptr;
    };
    const kzrt::Value* kon = key_at(mon);
    const kzrt::Value* koff = key_at(moff);
    KZ_CHECK(kon != nullptr && kon->from == "arg1" && kon->string_mode == "validated" && kon->bound == 12);
    KZ_CHECK(koff != nullptr && koff->from == "arg1" && koff->string_mode == "pointer-only");
    KZ_CHECK(has(on.manifest, "ref=\"argument 1 is the character array OneCfg.name of extent 12 (declared at plankey.c:98), the read at plankey.c:104:15 dominates the call (ARG_TYPES)"));
    KZ_CHECK(has(on.sidecar, "\nbound string-extent source=ARG_TYPES reason=") && !has(off.sidecar, "string-extent"));
    KZ_CHECK(has(on.sidecar, "plan rules revision 2 (2026-11-16)") && has(off.sidecar, "plan rules revision 2 (2026-11-16)"));
    // the callee's key text keeps its own bound (the runtime's, under the lookup's non-null test)
    const kzrt::Site* ktx = nullptr;
    for (const kzrt::Site& st : mon.sites)
      if (st.func == "reg_find" && st.place == "at-call" && st.callee == "reg_search") ktx = &st;
    KZ_CHECK(ktx != nullptr && ktx->value(0) != nullptr && ktx->value(0)->bound == 64);
    std::set<std::string> son, soff;
    for (const kzrt::Site& st : mon.sites) son.insert(st.func + "|" + st.place + "|" + st.callee + "|" + std::to_string(st.src_line));
    for (const kzrt::Site& st : moff.sites) soff.insert(st.func + "|" + st.place + "|" + st.callee + "|" + std::to_string(st.src_line));
    KZ_CHECK(son == soff);
  }

  // ---- plan rules revision 3 (manifest revision 4) on planv4.c
  {
    const fs::path vd = data / "planv4_fixture";
    const std::string vext = q(tool) + " extract --no-profile --source " + q((fixtures / "planv4.c").string()) + " --root " + q(fixtures.string()) +
                             " --targets-rule R-T1,R-T2 --object-access --semantics " + q((vd / "framework_semantics_fixture-plan.txt").string()) +
                             " --result-flow --loop-defs -o " + q((work / "planv4.kz").string()) + " -- -std=c99 -pedantic";
    KZ_CHECK_EQ(run(vext, &log), 0);
    const fs::path vfw = data / "planv4_framework.txt";
    const fs::path vns = vd / "framework_namespaces_fixture-plan.txt";
    auto v4cmd = [&](const fs::path& out, const std::string& budget, const std::string& control) {
      return q(tool) + " plan " + q((work / "planv4.kz").string()) + " --framework " + q(vfw.string()) + " --select observation-set --order method-priority --budget " + budget +
             " --budget-reason 'the test budget' --manifest 4 --control-budget " + control + " --control-budget-reason 'the test control budget' --namespaces " +
             q(vns.string()) + " --requests-out " + q((out / "plan.req").string()) + " -o " + q((out / "plan.kzs").string()) + " --sidecar " +
             q((out / "plan.side").string()) + " --modules-out " + q((out / "plan.mod").string()) + " --exes-out " + q((out / "plan.exe").string());
    };
    // the default is revision 2's output: no --manifest equals --manifest 3
    Plan d3, e3;
    KZ_CHECK(generate(plan_cmd(tool, work / "planv4.kz", vfw, work / "v3a"), work / "v3a", &d3));
    KZ_CHECK(generate(plan_cmd(tool, work / "planv4.kz", vfw, work / "v3b", " --manifest 3"), work / "v3b", &e3));
    KZ_CHECK(d3.manifest == e3.manifest);
    KZ_CHECK(d3.sidecar == e3.sidecar);
    KZ_CHECK(has(d3.manifest, "\nkzs-sites 3\n") && !has(d3.manifest, " skey=") && !has(d3.sidecar, "control-budget"));
    // two generations of revision 3, the same bytes
    Plan v, w;
    fs::create_directories(work / "v4a");
    fs::create_directories(work / "v4b");
    KZ_CHECK(generate(v4cmd(work / "v4a", "all", "all"), work / "v4a", &v));
    KZ_CHECK(generate(v4cmd(work / "v4b", "all", "all"), work / "v4b", &w));
    KZ_CHECK(v.manifest == w.manifest);
    const std::string vreq = read(work / "v4a" / "plan.req");
    KZ_CHECK(vreq == read(work / "v4b" / "plan.req"));
    KZ_CHECK(has(v.sidecar, "\nrequests sha256=" + kz::to_hex(kz::sha256(vreq)) + " file=plan.req rows="));
    KZ_CHECK(has(v.sidecar, "kz-plan-sidecar 2\n") && has(v.sidecar, "\nmanifest-revision 4 (kzs-sites 4)\n") && has(v.sidecar, "plan rules revision 3 (2026-11-16)"));
    KZ_CHECK(vreq.compare(0, 19, "kz-plan-requests 1\n") == 0 && has(vreq, "\nend rows="));
    KZ_CHECK(has(v.sidecar, "\ninput namespace-data sha256=" + kz::to_hex(kz::sha256(read(vns))) + " path="));
    // the control budget line before the first row line
    KZ_CHECK(v.sidecar.find("\ncontrol-budget value=all ") != std::string::npos && v.sidecar.find("\ncontrol-budget ") < v.sidecar.find("\nrow "));
    KZ_CHECK(has(v.sidecar, " rows=3 (a 1, b 2) planned=3 cut=0 complete=yes "));
    KZ_CHECK(std::regex_search(v.sidecar, std::regex(R"(\nrow [0-9a-f]{16} A3 [^\n]* control=a\+b control-state=planned )")));
    KZ_CHECK(std::regex_search(v.sidecar, std::regex(R"(\nrow [0-9a-f]{16} A1 [^\n]* control=b control-state=planned )")));
    // the manifest of revision 4 parses with the host parser
    kzrt::Manifest mv;
    std::string ve;
    KZ_CHECK(kzrt::parse_manifest(v.manifest, "planv4.kzs", &mv, &ve));
    if (!ve.empty()) std::fprintf(stderr, "%s\n", ve.c_str());
    KZ_CHECK_EQ(mv.version, 4u);
    KZ_CHECK(mv.has_plan && mv.plan.obsrules.find("plan rules revision 3") == 0 && mv.plan.control == "all" && mv.plan.budget == "all");
    {
      kz::Container fc;
      KZ_CHECK(kz::Container::read_file((work / "planv4.kz").string(), &fc).ok());
      KZ_CHECK(mv.plan.facts == kz::to_hex(fc.artifact_hash()));
    }
    KZ_CHECK(mv.skey_index.size() == mv.sites.size());
    const kzrt::Site* vload = nullptr;
    const kzrt::Site* vcreate = nullptr;
    bool calls_have_col = true;
    for (const kzrt::Site& st : mv.sites) {
      if ((st.place == "at-call" || st.place == "after-call") && (st.col == 0 || st.nth == 0)) calls_have_col = false;
      if (st.place == "load") vload = &st;
      if (st.has_spawn) vcreate = &st;
    }
    KZ_CHECK(calls_have_col);
    // path C: the load of the read field, the subscript call, the lock calls of the creator and the reader
    KZ_CHECK(vload != nullptr && vload->field == "Task.owner" && vload->func == "own_owner" && vload->rows.size() == 1u);
    auto vsite = [&](const std::string& func, const std::string& place, const std::string& callee) {
      for (const kzrt::Site& st : mv.sites)
        if (st.func == func && st.place == place && st.callee == callee) return &st;
      return static_cast<const kzrt::Site*>(nullptr);
    };
    KZ_CHECK(vsite("own_owner", "after-call", "task_index") != nullptr && has(vsite("own_owner", "after-call", "task_index")->label, "object-index"));
    KZ_CHECK(vsite("own_owner", "after-call", "pthread_mutex_lock") != nullptr && vsite("own_owner", "at-call", "pthread_mutex_unlock") != nullptr);
    KZ_CHECK(vsite("start_app", "after-call", "pthread_mutex_lock") != nullptr && vsite("start_app", "at-call", "pthread_mutex_unlock") != nullptr);
    // the spawn binding at the thread-creation call, with the spawn-token reader
    KZ_CHECK(vcreate != nullptr && vcreate->callee == "pthread_create" && vcreate->spawn.wrapper == "kz_rt_posix_create" && vcreate->spawn.start_arg == 2 &&
             vcreate->spawn.arg_arg == 3 && vcreate->spawn.out_arg == 0 && vcreate->has_identity && vcreate->identity.decoder == "spawn-token");
    // the wait: the call result the retry tests and its break statement
    const kzrt::Site* wcall = vsite("one_main", "after-call", "reg_find");
    KZ_CHECK(wcall != nullptr && has(wcall->label, "wait-cond"));
    bool wexit = false;
    for (const kzrt::Site& st : mv.sites)
      if (st.func == "one_main" && st.place == "line-start" && has(st.label, "wait-exit")) wexit = true;
    KZ_CHECK(wexit);
    // the delete function of the namespace data and its retire call
    KZ_CHECK(vsite("reg_delete", "after-call", "reg_retire") != nullptr);
    bool del_entry = false;
    for (const kzrt::Site& st : mv.sites)
      if (st.func == "reg_delete" && st.phase == "before" && has(st.label, "object-delete")) del_entry = true;
    KZ_CHECK(del_entry);
    KZ_CHECK(has(v.sidecar, "\nlifetime FIXTURE_REG delete reg_delete planv4.c:83 data "));
    // the effect call with the argument the semantics data names (event-type, argument 1)
    const kzrt::Site* ev = vsite("one_init", "at-call", "send_event");
    KZ_CHECK(ev != nullptr && ev->value(0) != nullptr && ev->value(0)->role == "event-type" && ev->value(0)->from == "arg1");
    // the creates excluded by key constants are creator sites of the rows (8.12 (ㅁ)): the create of "plum" is planned
    bool plum = false;
    for (const kzrt::Site& st : mv.sites)
      if (st.func == "two_main" && st.callee == "reg_create" && st.src_line == 180 && has(st.label, "creator-call")) plum = true;
    KZ_CHECK(plum);
    // need and gap statements: a planned need names declared sites; the reasons are in the plan's table
    KZ_CHECK(!mv.needs.empty() && !mv.gaps.empty());
    for (const kzrt::Need& n : mv.needs) {
      if (n.state == "planned") KZ_CHECK(!n.sites.empty());
      else KZ_CHECK(!n.reason.empty());
    }
    bool need_wait = false, need_delete = false, need_lock = false;
    for (const kzrt::Need& n : mv.needs) {
      if (n.what.compare(0, 12, "wait-result ") == 0 && n.state == "planned") need_wait = true;
      if (n.what == "object-delete" && n.state == "planned") need_delete = true;
      if (n.what == "lock-result" && n.state == "planned") need_lock = true;
    }
    KZ_CHECK(need_wait && need_delete && need_lock);
    // the value requests of the T? row: the result at the after-call and the effect's event type
    KZ_CHECK(std::regex_search(vreq, std::regex(R"(\nrequest \d+ row=[0-9a-f]{16} id=A1 label=T\? value=result state=planned site=\d+ slot=ret )")));
    KZ_CHECK(std::regex_search(vreq, std::regex(R"(\nrequest \d+ row=[0-9a-f]{16} id=A1 label=T\? value=effect state=planned site=\d+ slot=arg1 )")));
    KZ_CHECK(std::regex_search(vreq, std::regex(R"(\nrequest \d+ row=[0-9a-f]{16} id=A1 label=T\? value=branch state=not-planned )")));
    // the control rows are planned apart from the tier order: with the tier budget rows:0 the control rows are still planned
    Plan z;
    fs::create_directories(work / "v4z");
    KZ_CHECK(generate(v4cmd(work / "v4z", "rows:0", "all"), work / "v4z", &z));
    KZ_CHECK(has(z.sidecar, " planned=0 truncation=4 ") && has(z.sidecar, " rows=3 (a 1, b 2) planned=3 cut=0 complete=yes "));
    KZ_CHECK(std::regex_search(z.sidecar, std::regex(R"(\nrow [0-9a-f]{16} A3 [^\n]* state=cut control=a\+b control-state=planned )")));
    // a row the tier budget cuts but the control selection plans carries no budget need line
    {
      std::smatch m3;
      KZ_CHECK(std::regex_search(z.sidecar, m3, std::regex(R"(\nrow ([0-9a-f]{16}) A3 )")));
      KZ_CHECK(!has(z.sidecar, "\nneed row " + m3[1].str() + " observation not-planned"));
    }
    {
      kzrt::Manifest mz;
      std::string ze;
      KZ_CHECK(kzrt::parse_manifest(z.manifest, "z.kzs", &mz, &ze));
      bool use3 = false;
      for (const kzrt::Site& st : mz.sites)
        if (st.func == "three_main" && st.callee == "reg_find" && st.place == "after-call") use3 = true;
      KZ_CHECK(use3);
    }
    // a control budget of rows:1 holds the probe-create row only
    Plan c1;
    fs::create_directories(work / "v4c");
    KZ_CHECK(generate(v4cmd(work / "v4c", "rows:0", "rows:1"), work / "v4c", &c1));
    KZ_CHECK(has(c1.sidecar, " rows=3 (a 1, b 2) planned=1 cut=2 complete=no "));
    KZ_CHECK(std::regex_search(c1.sidecar, std::regex(R"(\nneed row [0-9a-f]{16} control not-planned reason=budget_not_run\(control row outside the control budget\))")));
    // refusals: no output is written
    {
      const fs::path rf = work / "v4r";
      fs::create_directories(rf);
      std::string base = v4cmd(rf, "tier:D07", "all");
      KZ_CHECK(run(std::regex_replace(base, std::regex(" --control-budget all --control-budget-reason 'the test control budget'"), ""), &log) == 2);
      KZ_CHECK(has(log, "--control-budget"));
      KZ_CHECK(run(plan_cmd(tool, work / "planv4.kz", vfw, rf, " --control-budget all"), &log) == 2);
      KZ_CHECK(has(log, "need --manifest 4"));
      KZ_CHECK(run(std::regex_replace(base, std::regex(" --control-budget all "), " --control-budget some "), &log) == 2);
      write(rf / "bad_ns.txt", "semantics-data 1\nnot a statement\n");
      KZ_CHECK(run(std::regex_replace(base, std::regex(" --namespaces \\S+"), " --namespaces " + q((rf / "bad_ns.txt").string())), &log) == 2);
      KZ_CHECK(has(log, "is no statement of the namespace data"));
      KZ_CHECK(!fs::exists(rf / "plan.kzs") && !fs::exists(rf / "plan.side") && !fs::exists(rf / "plan.req"));
    }
    // the text of the revision-3 plan
    {
      std::string got = mask(v.manifest + "\n# sidecar\n" + v.sidecar + "\n# requests\n" + vreq, (work / "").string());
      // the data directory is named in the namespace data refs: masked as the work directory is
      const std::string dd = (data / "").string();
      for (size_t at = got.find(dd); at != std::string::npos; at = got.find(dd)) got.replace(at, dd.size(), "<data>/");
      const fs::path golden = data / "planv4_np.plan.golden";
      const char* upd = std::getenv("KZ_GOLDEN_UPDATE");
      if (upd != nullptr && std::strcmp(upd, "1") == 0) {
        write(golden, got);
        std::printf("updated %s\n", golden.string().c_str());
      } else {
        const std::string want = read(golden);
        if (want != got) {
          write(work / "planv4_np.plan.actual", got);
          kztest::fail(__FILE__, __LINE__, "the plan differs from " + golden.string() + " (actual in " + (work / "planv4_np.plan.actual").string() + ")");
        }
        ++kztest::state().checks;
      }
    }
  }

  // ---- the golden text
  {
    const std::string got = mask(a.manifest + "\n# sidecar\n" + a.sidecar + "\n# module table\n" + a.modules + "\n# executables\n" + a.exes, (work / "").string());
    const fs::path golden = data / "plangen_np.plan.golden";
    const char* upd = std::getenv("KZ_GOLDEN_UPDATE");
    if (upd != nullptr && std::strcmp(upd, "1") == 0) {
      write(golden, got);
      std::printf("updated %s\n", golden.string().c_str());
    } else {
      const std::string want = read(golden);
      if (want != got) {
        write(work / "plangen_np.plan.actual", got);
        kztest::fail(__FILE__, __LINE__, "the plan differs from " + golden.string() + " (actual in " + (work / "plangen_np.plan.actual").string() + ")");
      }
      ++kztest::state().checks;
    }
  }
  return kztest::finish("test_static_plan");
}

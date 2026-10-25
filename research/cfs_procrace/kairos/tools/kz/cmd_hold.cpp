// kz hold -- run a command under the F0 control mechanism and hold a thread at a gate until an
// observed event (SPEC 4.13).
//
//   kz hold <basis.kz> <profile.kzp> <holdplan.kzh> -o <record.txt> [--basis <basis2.kz>]...
//           [--binary NAME=PATH]... [--verbose] -- <command> [arguments]
//
// The observation profile and its basis (one per file, as `kz observe`) give the file offset and
// the slots of every site; the hold plan (kzp-hold 1) names which site is a gate and which is a
// release, and the condition (the semaphore name at an argument, or the module of the return
// address) that selects the call.  The command is run as the supervisor's child; it is seized with
// PTRACE_SEIZE and its whole tree is followed, so the provider/consumer threads that cFS creates
// after the run starts are gated too.  Hardware execution breakpoints in the debug registers hold
// the gated thread; the text is never modified.  Every hold, its duration and the attach of each
// thread are written to the record as the run's perturbation.
//
// Exit status: 0 the run completed and the command exited 0; 10 the run completed and the command
// did not exit 0; 1 usage; 2 a basis/profile/binary mismatch or a hold-plan error; 3 the debug
// registers could not be used (the exact failure is printed and written; no int3 fallback is made).
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "basis.hpp"
#include "command.hpp"
#include "hold_plan.hpp"
#include "kz/core/container.hpp"
#include "supervisor.hpp"

namespace {

using kz::Status;

void usage(FILE* f) {
  std::fprintf(f,
               "usage: kz hold <basis.kz> <profile.kzp> <holdplan.kzh> -o <record.txt> [options] -- <command> [args]\n"
               "  -o FILE             the run record to write (perturbation and the hold timeline)\n"
               "  --basis FILE        the basis of a further file of a plan over several files (repeat per file)\n"
               "  --binary NAME=PATH  the path of a declared binary, if not the one recorded in its basis\n"
               "  --verbose           print the timeline to stderr as it happens\n"
               "The command follows `--`; it is run as the supervisor's child.\n");
}

std::string hex(uint64_t v) { char b[24]; std::snprintf(b, sizeof b, "0x%llx", (unsigned long long)v); return b; }

// Find the site of the plan by label; returns its index or -1.
int find_site(const std::vector<kz::trace::SiteSpec>& sites, const std::string& label) {
  for (size_t i = 0; i < sites.size(); ++i) if (sites[i].label == label) return (int)i;
  return -1;
}

// Turn a hold-plan condition into the control-point fields, reading the named slot of the site.
bool set_cond(const kz::trace::SiteSpec& s, const kz::hold::HoldCond& c, kz::hold::ControlPoint* cp, std::string* err) {
  if (c.has_name) {
    const kz::trace::SlotSpec* found = nullptr;
    for (const kz::trace::SlotSpec& sl : s.slots) if (sl.role == c.name_role) found = &sl;
    if (!found) { *err = "site '" + s.label + "' has no slot with role '" + c.name_role + "'"; return false; }
    if (found->src != 4 /*MEM*/ && found->src != 5 /*STR*/) { *err = "slot '" + c.name_role + "' of site '" + s.label + "' is not a memory/string source (cannot read a name there)"; return false; }
    cp->match_name = true;
    cp->name_reg = (int)found->index;
    cp->name_off = found->offset;
    cp->name_text = c.name_text;
  }
  if (c.has_caller_in) { cp->match_caller = true; cp->caller_binary = c.caller_binary; }
  return true;
}

struct Options {
  std::string basis, profile, holdplan, out;
  std::vector<std::string> also_bases;
  std::map<std::string, std::string> bin_paths;  // NAME -> PATH overrides
  bool verbose = false;
  std::vector<std::string> command;
};

bool parse_args(int argc, char** argv, Options* o) {
  int i = 1;
  for (; i < argc; ++i) {
    const std::string a = argv[i];
    if (a == "--") { ++i; break; }
    auto value = [&](std::string* dst) { if (i + 1 >= argc) { std::fprintf(stderr, "kz hold: %s needs a value\n", a.c_str()); return false; } *dst = argv[++i]; return true; };
    if (a == "--help" || a == "-h") { usage(stdout); exit(0); }
    else if (a == "-o") { if (!value(&o->out)) return false; }
    else if (a == "--basis") { std::string v; if (!value(&v)) return false; o->also_bases.push_back(v); }
    else if (a == "--verbose") { o->verbose = true; }
    else if (a == "--binary") { std::string v; if (!value(&v)) return false; size_t eq = v.find('='); if (eq == std::string::npos) { std::fprintf(stderr, "kz hold: --binary NAME=PATH\n"); return false; } o->bin_paths[v.substr(0, eq)] = v.substr(eq + 1); }
    else if (!a.empty() && a[0] == '-') { std::fprintf(stderr, "kz hold: unknown option %s\n", a.c_str()); return false; }
    else if (o->basis.empty()) o->basis = a;
    else if (o->profile.empty()) o->profile = a;
    else if (o->holdplan.empty()) o->holdplan = a;
    else { std::fprintf(stderr, "kz hold: unexpected argument %s\n", a.c_str()); return false; }
  }
  for (; i < argc; ++i) o->command.push_back(argv[i]);
  return true;
}

int run(int argc, char** argv) {
  Options opt;
  if (!parse_args(argc, argv, &opt)) { usage(stderr); return 1; }
  if (opt.basis.empty() || opt.profile.empty() || opt.holdplan.empty() || opt.out.empty() || opt.command.empty()) { usage(stderr); return 1; }

  kz::Bytes prof;
  Status s = kz::read_whole_file(opt.profile, &prof);
  if (!s.ok()) { std::fprintf(stderr, "kz hold: %s\n", s.describe().c_str()); return 2; }
  kz::plan::Profile profile;
  s = kz::plan::parse_profile(std::string_view((const char*)prof.data(), prof.size()), &profile);
  if (!s.ok()) { std::fprintf(stderr, "kz hold: %s\n", s.describe().c_str()); return 2; }

  kz::plan::LoadedBasis basis;
  s = kz::plan::load_basis(opt.basis, &basis);
  if (!s.ok()) { std::fprintf(stderr, "kz hold: basis %s: %s\n", opt.basis.c_str(), s.describe().c_str()); return 2; }
  std::vector<kz::plan::LoadedBasis> also(opt.also_bases.size());
  for (size_t i = 0; i < opt.also_bases.size(); ++i) {
    s = kz::plan::load_basis(opt.also_bases[i], &also[i]);
    if (!s.ok()) { std::fprintf(stderr, "kz hold: basis %s: %s\n", opt.also_bases[i].c_str(), s.describe().c_str()); return 2; }
  }
  std::vector<const kz::plan::LoadedBasis*> all{&basis};
  for (const kz::plan::LoadedBasis& b : also) all.push_back(&b);
  std::vector<kz::trace::SiteSpec> sites;
  s = kz::plan::make_plan(profile, all, &sites);
  if (!s.ok()) { std::fprintf(stderr, "kz hold: %s\n", s.describe().c_str()); return 2; }

  // Verify each declared binary on disk against its basis (as `kz observe` does before it attaches),
  // and record the run-time basename (what /proc/<pid>/maps shows) for each declared binary: the
  // profile names the primary "core-tx" but the rx instance's file is "core-rx", so the supervisor
  // must match by the file basename, not the profile name.
  auto basis_for = [&](const std::string& name) -> const kz::plan::LoadedBasis* {
    if (basis.binary_name == name) return &basis;
    for (const kz::plan::LoadedBasis& b : also) if (b.binary_name == name) return &b;
    return nullptr;
  };
  auto basename_of = [](const std::string& p) { size_t s2 = p.find_last_of('/'); return s2 == std::string::npos ? p : p.substr(s2 + 1); };
  std::map<std::string, std::string> run_basename;  // profile binary name -> run-time basename
  for (const std::string& bn : profile.binaries) {
    const kz::plan::LoadedBasis* lb = basis_for(bn);
    if (!lb) { std::fprintf(stderr, "kz hold: no basis for declared binary '%s'\n", bn.c_str()); return 2; }
    std::string path = opt.bin_paths.count(bn) ? opt.bin_paths[bn] : lb->binary_path;
    kz::Bytes bytes;
    s = kz::read_whole_file(path, &bytes);
    if (!s.ok()) { std::fprintf(stderr, "kz hold: cannot read %s: %s\n", path.c_str(), s.describe().c_str()); return 2; }
    s = kz::plan::verify_binary(bytes, *lb);
    if (!s.ok()) { std::fprintf(stderr, "kz hold: REFUSED: %s (%s)\n", s.message.c_str(), path.c_str()); return 2; }
    run_basename[bn] = basename_of(path);
  }

  kz::Bytes hp;
  s = kz::read_whole_file(opt.holdplan, &hp);
  if (!s.ok()) { std::fprintf(stderr, "kz hold: %s\n", s.describe().c_str()); return 2; }
  kz::hold::HoldPlan plan;
  s = kz::hold::parse_hold_plan(std::string_view((const char*)hp.data(), hp.size()), &plan);
  if (!s.ok()) { std::fprintf(stderr, "kz hold: %s\n", s.describe().c_str()); return 2; }

  // Build the control points.
  kz::hold::HoldConfig cfg;
  cfg.verbose = opt.verbose;
  cfg.primary_binary = profile.binaries.empty() ? std::string() : run_basename[profile.binaries[0]];
  cfg.argv = opt.command;
  auto add_point = [&](const std::string& label, const std::string& site_label, bool is_release, const std::string& releases,
                       const kz::hold::HoldCond& cond, bool each) -> bool {
    int si = find_site(sites, site_label);
    if (si < 0) { std::fprintf(stderr, "kz hold: the profile has no site '%s' (named by %s)\n", site_label.c_str(), label.c_str()); return false; }
    const kz::trace::SiteSpec& ss = sites[si];
    kz::hold::ControlPoint cp;
    cp.label = label;
    cp.binary = ss.binary < profile.binaries.size() ? run_basename[profile.binaries[ss.binary]] : cfg.primary_binary;
    cp.file_offset = ss.file_offset;
    cp.vaddr = ss.vaddr;
    cp.is_release = is_release;
    cp.releases = releases;
    cp.occurrence_each = each;
    std::string err;
    if (!set_cond(ss, cond, &cp, &err)) { std::fprintf(stderr, "kz hold: %s\n", err.c_str()); return false; }
    cfg.points.push_back(std::move(cp));
    return true;
  };
  for (const kz::hold::GateDecl& g : plan.gates) if (!add_point(g.label, g.site_label, false, "", g.cond, g.occurrence_each)) return 2;
  for (const kz::hold::ReleaseDecl& r : plan.releases) if (!add_point(r.label, r.site_label, true, r.releases, r.cond, r.occurrence_each)) return 2;

  kz::hold::HoldReport rep;
  s = kz::hold::run_hold(cfg, &rep);
  if (!s.ok()) { std::fprintf(stderr, "kz hold: %s\n", s.describe().c_str()); return 2; }

  // Write the record.
  std::ostringstream o;
  o << "# kz hold run record (label: controlled; SPEC 4.13 F0)\n";
  o << "command:";
  for (const std::string& a : cfg.argv) o << ' ' << a;
  o << "\n";
  o << "primary_binary=" << cfg.primary_binary << "\n";
  o << "child_pid=" << rep.child_pid << "\n";
  o << "debugregs_ok=" << (rep.debugregs_ok ? "yes" : "no") << "\n";
  if (!rep.debugregs_ok) o << "debugregs_error=" << rep.debugregs_error << "\n";
  o << "wall_ns=" << (rep.t_end_ns - rep.t_start_ns) << "\n";
  if (rep.child_exited) o << "child_exit=" << rep.child_exit_code << "\n";
  else if (rep.child_term_signal) o << "child_signal=" << rep.child_term_signal << "\n";
  o << "# control points (gate/release -> binary:file_offset, link-time vaddr):\n";
  for (const kz::hold::ControlPoint& cp : cfg.points) {
    o << (cp.is_release ? "release " : "gate    ") << cp.label << " " << cp.binary << ":" << hex(cp.file_offset) << " vaddr=" << hex(cp.vaddr);
    if (cp.match_name) o << " name[reg" << cp.name_reg << "+" << cp.name_off << "]==\"" << cp.name_text << "\"";
    if (cp.match_caller) o << " caller-in=" << cp.caller_binary;
    if (cp.is_release) o << " releases=" << cp.releases;
    o << (cp.occurrence_each ? " occurrence=each" : " occurrence=first") << "\n";
  }
  o << "# perturbation (holder cost and the ptrace interaction, SPEC 4.13):\n";
  o << "total_stops=" << rep.total_stops << " threads_attached=" << rep.threads_attached
    << " clone_attaches=" << rep.clone_attaches << " relevant_procs=" << rep.relevant_procs
    << " gate_traps=" << rep.gate_traps << " release_traps=" << rep.release_traps << " armings=" << rep.armings << "\n";
  o << "# holds (" << rep.holds.size() << "):\n";
  for (const kz::hold::HoldEvent& he : rep.holds) {
    o << "hold gate=" << he.gate_label << " tid=" << he.tid << " name=\"" << he.name << "\""
      << " held_ns=" << he.t_hold_ns;
    if (he.released) o << " released_by=" << he.release_label << " release_ns=" << he.t_release_ns << " hold_duration_ns=" << (he.t_release_ns - he.t_hold_ns);
    else o << " released=no";
    o << "\n";
  }
  for (const std::string& n : rep.notes) o << "note: " << n << "\n";

  std::ofstream of(opt.out);
  of << o.str();
  of.close();
  std::fputs(o.str().c_str(), stdout);

  if (!rep.debugregs_ok) return 3;
  if (rep.child_exited && rep.child_exit_code == 0) return 0;
  return 10;
}

}  // namespace

KZ_COMMAND(hold, "run a command and hold a thread at a gate until an observed event (F0, SPEC 4.13)", run);

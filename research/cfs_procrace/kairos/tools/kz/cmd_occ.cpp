// kz occ -- the per-occurrence table of a replay output container (`kz replay --out`) and the comparison of two of them
// (kz/trace/occ_table.hpp, docs/replay_output.md).
//
//   kz occ show <verdicts.kz>
//       one line per occurrence: the occurrence key, the executing task, the verdict and its summary view, every basis
//       kind, the declared assumptions, the outcome check and the edges the result rests on.  A container written
//       before schema revision 4 has no OCC_BASIS: what SUMMARIES holds is shown and the rest is said not recorded.
//   kz occ diff <a.kz> <b.kz>
//       the two containers' occurrences matched by the path-independent occurrence key (rspec, operation, owning
//       application and the task's place in it, resource, ordinal); then without the owner, only where one side's owner
//       (or the task's place in it) is not established and the role, the via names and the text arguments agree -- two
//       established owners that differ are never paired; then by the text arguments where one side fixed the key value.
//       The reading differences, the verdict and basis differences of the full-key pairs, the pairs of the other rules,
//       the uncertain candidates (tied by the ordinal alone), the keys several occurrences of a side share (never paired
//       by order), the candidates whose owners differ and the occurrences of one side only are listed apart.  Exit 0
//       with no difference, 4 where the pairing of some occurrences is not established (an ambiguous key, an uncertain
//       candidate), else 3 with any difference (occ_table.hpp occ_diff_status; the head line "result:" says which).
//
// Neither form sets a research condition: it reads what the replay wrote.  Which records are compared, and under which
// reading, is the caller's statement.
#include <cstdio>
#include <string>

#include "command.hpp"
#include "kz/core/container.hpp"
#include "kz/trace/occ_table.hpp"

namespace {

void usage(FILE* f) {
  std::fprintf(f,
               "usage: kz occ show <verdicts.kz>\n"
               "       kz occ diff <a.kz> <b.kz>\n"
               "  show   the per-occurrence table: key, verdict, summary view, basis kinds, assumptions, outcome check, edges\n"
               "  diff   matched by the occurrence key; verdict and basis differences listed apart; without the owner only where\n"
               "         one side's owner is not established; a shared key or an uncertain candidate listed and not paired\n"
               "         (exit 4 if a pairing is not established, else 3 if any difference, else 0)\n");
}

bool load(const std::string& path, kz::Container* c, kz::trace::OccTable* t) {
  kz::Status s = kz::Container::read_file(path, c);
  if (s.ok()) s = c->verify_content();
  if (s.ok()) s = kz::trace::read_occ_table(*c, t);
  if (!s.ok()) {
    std::fprintf(stderr, "kz occ: %s: %s\n", path.c_str(), s.describe().c_str());
    return false;
  }
  return true;
}

int run(int argc, char** argv) {
  if (argc >= 2 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")) {
    usage(stdout);
    return 0;
  }
  if (argc == 3 && std::string(argv[1]) == "show") {
    kz::Container c;
    kz::trace::OccTable t;
    if (!load(argv[2], &c, &t)) return 2;
    std::printf("kz occ show %s\nartifact_sha256 %s\n", argv[2], kz::to_hex(c.artifact_hash()).c_str());
    std::fputs(kz::trace::occ_table_text(t).c_str(), stdout);
    return 0;
  }
  if (argc == 4 && std::string(argv[1]) == "diff") {
    kz::Container ca, cb;
    kz::trace::OccTable ta, tb;
    if (!load(argv[2], &ca, &ta) || !load(argv[3], &cb, &tb)) return 2;
    std::printf("kz occ diff\nA %s artifact_sha256 %s (per-occurrence basis %s)\nB %s artifact_sha256 %s (per-occurrence basis %s)\n", argv[2],
                kz::to_hex(ca.artifact_hash()).c_str(), ta.recorded ? "recorded" : "not recorded", argv[3], kz::to_hex(cb.artifact_hash()).c_str(),
                tb.recorded ? "recorded" : "not recorded");
    kz::trace::OccDiffCounts n;
    std::fputs(kz::trace::occ_diff_text(ta, tb, &n).c_str(), stdout);
    return kz::trace::occ_diff_status(n);
  }
  usage(stderr);
  return 1;
}

}  // namespace

KZ_COMMAND(occ, "the per-occurrence table of a replay output container, or the comparison of two (verdict and basis apart)", run);

// The incompleteness reasons of `kz-static relations` (relations rules revision 6, docs/static_facts.md §5m) against the
// reason vocabulary of the judgement rules (data/reason_vocabulary.txt, taken from JUDGEMENT_RULES_REV1.md 3.5):
//   - every reason the tool can write (relations_incomplete_reasons()) is a reason of the vocabulary, with a sub-reason
//     and a rule, and no reason and sub-reason twice;
//   - every `incomplete:` line of the relations goldens names only reasons of that table, and every row of a golden has
//     one such line; the summary line counts them.
// The tool writes a reason only through the table (an index into it), so the table is every reason string it can write.
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "relations.hpp"
#include "support/kz_test.hpp"

using namespace kzs;
namespace fs = std::filesystem;

namespace {

std::string read(const fs::path& p) {
  std::ifstream in(p, std::ios::binary);
  std::ostringstream os;
  os << in.rdbuf();
  return os.str();
}

std::vector<std::string> lines_of(const std::string& text) {
  std::vector<std::string> out;
  std::istringstream is(text);
  std::string l;
  while (std::getline(is, l)) out.push_back(l);
  return out;
}

bool is_row_head(const std::string& l) {
  if (l.size() < 5 || l.compare(0, 2, "  ") != 0) return false;
  if (l[2] != 'A' && l[2] != 'B' && l[2] != 'C') return false;
  size_t i = 3;
  while (i < l.size() && l[i] >= '0' && l[i] <= '9') ++i;
  return i > 3 && i < l.size() && l[i] == ' ';
}

}  // namespace

int main(int argc, char** argv) {
  KZ_CHECK_EQ(argc, 3);
  if (argc != 3) return kztest::finish("test_static_reasons");
  // the vocabulary
  std::set<std::string> vocab;
  size_t n_lines = 0;
  for (const std::string& l : lines_of(read(argv[1]))) {
    if (l.empty() || l[0] == '#') continue;
    ++n_lines;
    KZ_CHECK(l.find_first_of(" \t()") == std::string::npos);
    KZ_CHECK(vocab.insert(l).second);
  }
  KZ_CHECK_EQ(n_lines, static_cast<size_t>(22));
  for (const char* added : {"intervention_out_of_scope", "reading_options_differ", "build_mismatch"}) KZ_CHECK(vocab.count(added) == 1);

  // the table of the tool
  const std::vector<RelReason>& table = relations_incomplete_reasons();
  KZ_CHECK(!table.empty());
  std::set<std::string> texts;
  for (const RelReason& r : table) {
    const std::string reason = r.reason, sub = r.sub, when = r.when;
    if (vocab.count(reason) == 0) kztest::fail(__FILE__, __LINE__, "reason not in the vocabulary: " + reason);
    ++kztest::state().checks;
    KZ_CHECK(!sub.empty() && sub.find_first_of("();") == std::string::npos);
    KZ_CHECK(!when.empty());
    KZ_CHECK_EQ(reason_text(r), reason + "(" + sub + ")");
    KZ_CHECK(texts.insert(reason_text(r)).second);
  }
  // the reasons the table groups together come one after another (the summary groups by reason in the table's order)
  {
    std::set<std::string> closed;
    std::string last;
    for (const RelReason& r : table) {
      if (r.reason != last) {
        KZ_CHECK(closed.count(r.reason) == 0);
        if (!last.empty()) closed.insert(last);
        last = r.reason;
      }
    }
  }
  // the relations goldens: every row has one `incomplete:` line, of reasons of the table
  size_t n_goldens = 0, n_rows = 0;
  std::map<std::string, size_t> seen;
  for (const auto& e : fs::directory_iterator(argv[2])) {
    if (e.path().extension() != ".golden") continue;
    const std::string text = read(e.path());
    if (text.find("\nkz-relations 1\n") == std::string::npos && text.compare(0, 14, "kz-relations 1") != 0) continue;
    ++n_goldens;
    size_t heads = 0, incs = 0, summaries = 0;
    for (const std::string& l : lines_of(text)) {
      if (is_row_head(l)) ++heads;
      if (l.compare(0, 20, "summary incomplete (") == 0) ++summaries;
      const std::string tag = "      incomplete: ";
      if (l.compare(0, tag.size(), tag) != 0) continue;
      ++incs;
      std::string rest = l.substr(tag.size());
      KZ_CHECK(!rest.empty());
      size_t at = 0;
      while (at <= rest.size()) {
        size_t end = rest.find("; ", at);
        if (end == std::string::npos) end = rest.size();
        const std::string item = rest.substr(at, end - at);
        if (texts.count(item) == 0) kztest::fail(__FILE__, __LINE__, e.path().filename().string() + ": not a reason of the table: " + item);
        ++kztest::state().checks;
        ++seen[item];
        at = end + 2;
      }
    }
    KZ_CHECK(heads != 0);
    KZ_CHECK_EQ(incs, heads);
    KZ_CHECK(summaries != 0);
    n_rows += heads;
  }
  KZ_CHECK(n_goldens >= 6);  // relations, relations_np, relpaths_np, relsound_np, relincomplete_np, relincomplete_cut_np
  KZ_CHECK(n_rows != 0);
  // every row carries no_run in this revision
  KZ_CHECK_EQ(seen["no_run(no run record read)"], n_rows);
  return kztest::finish("test_static_reasons");
}

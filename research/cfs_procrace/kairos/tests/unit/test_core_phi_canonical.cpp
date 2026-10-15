// The canonical text and the hash of a profile: the mini profile loads without error, the canonical
// text is a fixed point (parse, check, print gives the same text), and the hash does not change
// with comments, white space, the order of declarations, the split into files or the spelling of
// integers, and does change with every semantic edit.

#include <algorithm>
#include <string>
#include <vector>

#include "kz/core/phi.hpp"
#include "support/kz_test.hpp"
#include "support/phi_mini.hpp"

namespace {

using kz::phi::Diag;
using kz::phi::Profile;
using kz::phi::SourceText;

bool load(const std::vector<SourceText>& files, Profile* p, std::vector<Diag>* diags) {
  return kz::phi::load_profile(files, p, diags).ok();
}

std::string show_diags(const Profile& p, const std::vector<Diag>& diags) {
  std::string out;
  for (const Diag& d : diags) out += kz::phi::format_diag(p.files, d) + "\n";
  return out;
}

std::string canon_of(const std::vector<SourceText>& files, std::string* hash = nullptr) {
  Profile p;
  std::vector<Diag> diags;
  if (!load(files, &p, &diags)) return "LOAD FAILED\n" + show_diags(p, diags);
  if (hash != nullptr) *hash = kz::to_hex(kz::phi::profile_hash(p));
  return kz::phi::canonical_text(p);
}

std::string replace_once(std::string text, const std::string& from, const std::string& to) {
  const size_t at = text.find(from);
  if (at == std::string::npos) return "MISSING: " + from;
  return text.replace(at, from.size(), to);
}

}  // namespace

int main() {
  {  // the mini profile loads; no error, and the diagnostics are only notes or warnings
    Profile p;
    std::vector<Diag> diags;
    const bool ok = load(kzmini::files(), &p, &diags);
    if (!ok) std::fprintf(stderr, "%s", show_diags(p, diags).c_str());
    KZ_CHECK(ok);
    KZ_CHECK(!kz::phi::has_errors(diags));
    KZ_CHECK(p.checked);
  }

  std::string base_hash;
  const std::string base = canon_of(kzmini::files(), &base_hash);
  KZ_CHECK(base.rfind("LOAD FAILED", 0) != 0);
  KZ_CHECK_EQ(base_hash.size(), static_cast<size_t>(64));

  {  // the canonical text is a fixed point: it parses, checks and prints to itself, with the same hash
    std::string hash2;
    const std::string again = canon_of({SourceText{"canon.kzp", base}}, &hash2);
    KZ_CHECK_EQ(again, base);
    KZ_CHECK_EQ(hash2, base_hash);
  }

  {  // the hash covers the canonical text under a domain tag
    Profile p;
    std::vector<Diag> diags;
    KZ_CHECK(load(kzmini::files(), &p, &diags));
    const kz::Digest d = kz::phi::profile_hash(p);
    KZ_CHECK(d == kz::sha256(std::string(kz::phi::kProfileHashDomain) + kz::phi::canonical_text(p)));
  }

  {  // comments and white space do not matter
    std::vector<SourceText> f = kzmini::files();
    f[0].text = "# extra comment\n\n\n" + f[0].text + "\n# trailing comment\n";
    f[1].text = replace_once(f[1].text, "states FREE < RESERVED < LIVE;", "states   FREE\n   <  RESERVED   <\tLIVE ;  # inline");
    f[2].text = replace_once(f[2].text, "title \"program order\";", "title    \"program order\" ;");
    std::string h;
    const std::string c = canon_of(f, &h);
    KZ_CHECK_EQ(c, base);
    KZ_CHECK_EQ(h, base_hash);
  }

  {  // the order of declarations and the split into files do not matter
    std::vector<SourceText> f = kzmini::files();
    std::reverse(f.begin(), f.end());
    std::string h;
    // The files are loaded in the order of their names; reversing the vector changes nothing.
    KZ_CHECK_EQ(canon_of(f, &h), base);
    KZ_CHECK_EQ(h, base_hash);

    // One file holding everything, with the second and third parts moved before the first part's
    // constants (declaration order is free; only the header must be somewhere).
    std::string all = "phi 1;\n";
    for (const SourceText& s : kzmini::files()) {
      std::string body = s.text;
      const size_t at = body.find("phi 1;");
      body.erase(at, 6);
      all += body;
    }
    KZ_CHECK_EQ(canon_of({SourceText{"all.kzp", all}}, &h), base);
    KZ_CHECK_EQ(h, base_hash);

    // The provenance records moved to a file of their own and the constants after the kinds.
    std::string head = kzmini::file_head();
    const size_t first_const = head.find("const OK");
    const std::string consts = head.substr(first_const);
    head.erase(first_const);
    KZ_CHECK_EQ(canon_of({SourceText{"1.kzp", head}, SourceText{"2.kzp", kzmini::file_kinds()}, SourceText{"3.kzp", kzmini::file_ops()},
                          SourceText{"4.kzp", "phi 1;\n" + consts}},
                         &h),
                base);
    KZ_CHECK_EQ(h, base_hash);
  }

  {  // the spelling of an integer does not matter: 0xc0000001 and 3221225473 are the same constant
    std::vector<SourceText> f = kzmini::files();
    f[0].text = replace_once(f[0].text, "0xc0000001", "3221225473");
    f[0].text = replace_once(f[0].text, "const BASE = 0x1000", "const BASE = 4096");
    std::string h;
    KZ_CHECK_EQ(canon_of(f, &h), base);
    KZ_CHECK_EQ(h, base_hash);
  }

  {  // the citations of a conjunct: sorted and unique in the canonical text, and the text is a fixed point
    std::vector<SourceText> f = kzmini::files();
    f[2].text = replace_once(f[2].text, "conjunct who_valid: valid(arg.Who) lp capture;", "conjunct who_valid: valid(arg.Who) lp capture prov doc_sup, code_a, doc_sup;");
    std::string h1, h2;
    const std::string c1 = canon_of(f, &h1);
    KZ_CHECK(c1.find("conjunct who_valid: valid(arg.Who) lp capture prov code_a, doc_sup;") != std::string::npos);
    KZ_CHECK_EQ(canon_of({SourceText{"canon.kzp", c1}}, &h2), c1);
    KZ_CHECK_EQ(h1, h2);
    std::vector<SourceText> g = kzmini::files();
    g[2].text = replace_once(g[2].text, "conjunct who_valid: valid(arg.Who) lp capture;", "conjunct who_valid: valid(arg.Who) lp capture prov code_a, doc_sup;");
    std::string h3;
    KZ_CHECK_EQ(canon_of(g, &h3), c1);
    KZ_CHECK_EQ(h3, h1);
  }

  {  // the order of a citation list, and a repeated citation, do not matter
    std::vector<SourceText> f = kzmini::files();
    f[2].text = replace_once(f[2].text, "prov doc_call, doc_sup, code_a;", "prov code_a, doc_sup, doc_call, code_a;");
    std::string h;
    KZ_CHECK_EQ(canon_of(f, &h), base);
    KZ_CHECK_EQ(h, base_hash);
  }

  // Semantic edits change the hash.  Each edit is a different one; all hashes differ from the base
  // and from each other.
  struct Edit {
    int file;
    const char* from;
    const char* to;
  };
  const Edit edits[] = {
      {0, "const LIMIT = 8", "const LIMIT = 9"},                            // a constant value
      {0, "lines 5;", "lines 6;"},                                           // a cited line
      {0, "authored \"unit test, nobody\"", "authored \"unit test, somebody\""},  // who entered a hand record
      {1, "initial FREE;", "initial RESERVED;"},                             // the initial state of a kind
      {1, "states ABSENT < PRESENT;", "states ABSENT < PRESENT < EXTRA;"},   // a state
      {1, "tag Type { UNSET, CORE, EXTERNAL };", "tag Type { UNSET, CORE, EXTERNAL, LIBRARY };"},  // a tag value
      {2, "outcome BAD_ARG: error;", "outcome BAD_ARG: ready;"},             // an outcome class
      {2, "allowed none;", "allowed unspecified_by_reference;"},             // the Allowed clause
      {2, "harm event_request_rejected;", "harm status_ignored;"},           // the harm
      {2, "stratum S;\n  robust unconditional;\n  template spawn", "stratum S;\n  robust obs_fatal;\n  template spawn"},  // robustness
      {2, "guard main >= \"2.0.1\";", "guard main >= \"2.0.2\";"},           // a version guard
      {2, "refutable_by O9;", "refutable_by O8;"},                           // a refutation
      {2, "conjunct who_valid: valid(arg.Who) lp capture;", "conjunct who_valid: valid(arg.Who) lp capture prov doc_sup;"},  // a citation of a conjunct
  };
  std::vector<std::string> seen{base_hash};
  for (const Edit& e : edits) {
    std::vector<SourceText> f = kzmini::files();
    f[static_cast<size_t>(e.file)].text = replace_once(f[static_cast<size_t>(e.file)].text, e.from, e.to);
    std::string h;
    const std::string c = canon_of(f, &h);
    if (c.rfind("LOAD FAILED", 0) == 0 || c.rfind("MISSING", 0) == 0) std::fprintf(stderr, "edit %s -> %s: %s\n", e.from, e.to, c.c_str());
    KZ_CHECK(c.rfind("LOAD FAILED", 0) != 0);
    KZ_CHECK(c != base);
    KZ_CHECK(std::find(seen.begin(), seen.end(), h) == seen.end());
    seen.push_back(h);
  }

  {  // a duplicate declaration is an error at load, not silently merged
    std::vector<SourceText> f = kzmini::files();
    f.push_back(SourceText{"dup.kzp", "phi 1;\nconst OK = 0 prov code_a;\n"});
    Profile p;
    std::vector<Diag> diags;
    KZ_CHECK(!load(f, &p, &diags));
    KZ_CHECK(show_diags(p, diags).find("duplicate const") != std::string::npos);
  }

  return kztest::finish("test_core_phi_canonical");
}

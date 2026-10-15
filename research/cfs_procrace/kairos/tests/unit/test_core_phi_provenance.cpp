// Provenance helpers of the profile language: the hash of a cited text, the source that owns a
// path, the pairing of tiers and kinds, and the verification of every citation of a profile
// against a source tree (here a made-up one in a temporary directory).

#include <unistd.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "kz/core/phi.hpp"
#include "kz/core/phi_provenance.hpp"
#include "support/kz_test.hpp"

namespace fs = std::filesystem;

namespace {

using kz::phi::Diag;
using kz::phi::Profile;
using kz::phi::ProvProblem;
using kz::phi::SourceText;

std::string hex_of(const std::string& bytes, uint32_t from, uint32_t to) {
  kz::Digest d;
  if (!kz::phi::cited_text_hash(bytes, from, to, &d)) return "NONE";
  return kz::to_hex(d);
}

void write_file(const fs::path& p, const std::string& bytes) {
  fs::create_directories(p.parent_path());
  std::ofstream out(p, std::ios::binary);
  out << bytes;
}

}  // namespace

int main() {
  // ---- cited_text_hash ------------------------------------------------------------------------
  const std::string four = "alpha\nbeta\ngamma\ndelta";  // the last line has no line break
  // Values from `sed -n 'A,Bp' file | sha256sum` on a file with these lines, all terminated.
  KZ_CHECK_EQ(hex_of(four, 1, 1), std::string("b6a98d9ce9a2d9149288fa3df42d377c3e42737afdcdaf714e33c0a100b51060"));
  KZ_CHECK_EQ(hex_of(four, 2, 3), std::string("aa5989aacb57830a365b63654addd2b3e7427ce3e8869f52e261ac98cc318734"));
  // The same texts, built independently: each cited line is followed by one LF.
  KZ_CHECK_EQ(hex_of(four, 1, 1), kz::to_hex(kz::sha256(std::string("alpha\n"))));
  KZ_CHECK_EQ(hex_of(four, 2, 3), kz::to_hex(kz::sha256(std::string("beta\ngamma\n"))));
  KZ_CHECK_EQ(hex_of(four, 1, 4), kz::to_hex(kz::sha256(std::string("alpha\nbeta\ngamma\ndelta\n"))));
  // A last line without a line break is treated as if it had one.
  KZ_CHECK_EQ(hex_of(four, 4, 4), kz::to_hex(kz::sha256(std::string("delta\n"))));
  KZ_CHECK_EQ(hex_of(four, 4, 4), hex_of(four + "\n", 4, 4));
  KZ_CHECK_EQ(hex_of(four, 3, 4), hex_of(four + "\n", 3, 4));
  // Out of range, empty range, line 0.
  KZ_CHECK_EQ(hex_of(four, 1, 5), std::string("NONE"));
  KZ_CHECK_EQ(hex_of(four, 5, 5), std::string("NONE"));
  KZ_CHECK_EQ(hex_of(four + "\n", 5, 5), std::string("NONE"));
  KZ_CHECK_EQ(hex_of(four, 3, 2), std::string("NONE"));
  KZ_CHECK_EQ(hex_of(four, 0, 2), std::string("NONE"));
  KZ_CHECK_EQ(hex_of("", 1, 1), std::string("NONE"));
  // Blank lines are lines; a CR stays part of its line (no line-end conversion).
  KZ_CHECK_EQ(hex_of("\n\nx\n", 1, 2), kz::to_hex(kz::sha256(std::string("\n\n"))));
  KZ_CHECK_EQ(hex_of("\n\nx\n", 1, 2), std::string("75a11da44c802486bc6f65640aa48a730f0f684c5c07a42ba3cd1735eb3fb070"));
  KZ_CHECK_EQ(hex_of("a\r\nb\r\n", 1, 2), std::string("58055bdcc73787eb88c78d36f0b4939e9c5dc1c3ad17e25cc85a6833cf1a0cab"));
  KZ_CHECK(hex_of("a\r\nb\r\n", 1, 2) != hex_of("a\nb\n", 1, 2));

  // ---- tiers and kinds (SPEC 1.6 "Naming and source tiers") ------------------------------------
  KZ_CHECK(kz::phi::tier_kind_compatible("contract-backed", "api_doc"));
  KZ_CHECK(kz::phi::tier_kind_compatible("requirement-derived", "requirement"));
  // A requirement record is not contract-backed: it has its own tier, which this build's schema
  // vocabulary (KZ_TIER) does not contain yet, so no profile can name it.
  KZ_CHECK(!kz::phi::tier_kind_compatible("contract-backed", "requirement"));
  KZ_CHECK_EQ(kz::phi::tier_index("requirement-derived"), -1);
  KZ_CHECK(kz::phi::tier_kind_compatible("admitted", "code_admission"));
  KZ_CHECK(kz::phi::tier_kind_compatible("code-derived", "code_pattern"));
  KZ_CHECK(kz::phi::tier_kind_compatible("issue-derived", "issue"));
  KZ_CHECK(kz::phi::tier_kind_compatible("researcher", "researcher"));
  KZ_CHECK(kz::phi::tier_kind_compatible("unreferenced", "none"));
  KZ_CHECK(!kz::phi::tier_kind_compatible("contract-backed", "code_pattern"));
  KZ_CHECK(!kz::phi::tier_kind_compatible("code-derived", "api_doc"));
  KZ_CHECK(!kz::phi::tier_kind_compatible("researcher", "api_doc"));
  KZ_CHECK(!kz::phi::tier_kind_compatible("nonsense", "api_doc"));
  // The rank follows the order SPEC 1.6 lists the tiers in, strongest first: contract-backed,
  // requirement-derived, admitted, issue-derived, code-derived, researcher, unreferenced.
  {
    const char* order[] = {"contract-backed", "requirement-derived", "admitted", "issue-derived", "code-derived", "researcher", "unreferenced"};
    for (int i = 0; i < 7; ++i) KZ_CHECK_EQ(kz::phi::tier_rank(order[i]), i);
  }
  KZ_CHECK_EQ(kz::phi::tier_rank("nonsense"), -1);
  // The index is the value in the schema's vocabulary: distinct for the six tiers.
  int seen_mask = 0;
  for (const char* t : {"unreferenced", "contract-backed", "admitted", "code-derived", "issue-derived", "researcher"}) {
    const int i = kz::phi::tier_index(t);
    KZ_CHECK(i >= 0 && i < 6);
    if (i >= 0 && i < 6) seen_mask |= 1 << i;
  }
  KZ_CHECK_EQ(seen_mask, 63);
  KZ_CHECK_EQ(kz::phi::tier_index("nonsense"), -1);

  // ---- cited_text and whole identifiers -----------------------------------------------------------
  {
    std::string t;
    KZ_CHECK(kz::phi::cited_text(four, 2, 3, &t));
    KZ_CHECK_EQ(t, std::string("beta\ngamma\n"));
    KZ_CHECK(kz::phi::cited_text(four, 4, 4, &t));  // the last line without a line break
    KZ_CHECK_EQ(t, std::string("delta\n"));
    KZ_CHECK(!kz::phi::cited_text(four, 1, 5, &t));
    KZ_CHECK(!kz::phi::cited_text(four, 3, 2, &t));
    KZ_CHECK(kz::phi::text_has_identifier("x = set_used(p);", "set_used"));
    KZ_CHECK(!kz::phi::text_has_identifier("x = set_used2(p);", "set_used"));   // inside a longer identifier
    KZ_CHECK(!kz::phi::text_has_identifier("x = reset_used(p);", "set_used"));
    KZ_CHECK(!kz::phi::text_has_identifier("x = set_used_(p);", "set_used"));
    KZ_CHECK(kz::phi::text_has_identifier("set_used", "set_used"));              // at both ends of the text
    KZ_CHECK(kz::phi::text_has_identifier("a->id = 1;", "id"));
    KZ_CHECK(!kz::phi::text_has_identifier("a->idx = 1;", "id"));
    KZ_CHECK(!kz::phi::text_has_identifier("anything", ""));
  }

  // ---- a tree on disk -----------------------------------------------------------------------------
  const fs::path root = fs::temp_directory_path() / ("kz_phi_prov_" + std::to_string(::getpid()));
  fs::remove_all(root);
  write_file(root / "inc" / "api.h", "line one\nline two\nline three\nline four\n");
  write_file(root / "libx" / "b.c", "int f(void) {\n  return 1;\n}\n");
  const std::string api_all = "line one\nline two\nline three\nline four\n";
  const std::string h_api_2_3 = kz::to_hex(kz::sha256(std::string("line two\nline three\n")));
  const std::string h_b_1_3 = kz::to_hex(kz::sha256(std::string("int f(void) {\n  return 1;\n}\n")));
  (void)api_all;

  auto profile_with = [&](const std::string& provs, Profile* p, std::vector<Diag>* diags) {
    std::string text = "phi 1;\nprofile \"t\";\nsource main root \"\" rev \"abcdef0\";\nsource libx root \"libx\" rev \"1234567\";\n" + provs;
    return kz::phi::load_profile({SourceText{"t.kzp", text}}, p, diags).ok();
  };
  auto prov = [](const std::string& name, const std::string& file, int a, int b, const std::string& hash) {
    return "prov " + name + " { tier contract-backed; kind api_doc; side supplier; file \"" + file + "\"; lines " + std::to_string(a) + " " + std::to_string(b) +
           "; sha256 \"" + hash + "\"; }\n";
  };

  {  // everything matches
    Profile p;
    std::vector<Diag> diags;
    KZ_CHECK(profile_with(prov("good_a", "inc/api.h", 2, 3, h_api_2_3) + prov("good_b", "libx/b.c", 1, 3, h_b_1_3) +
                              "prov hand { tier researcher; kind researcher; authored \"t\"; }\n",
                          &p, &diags));
    std::vector<ProvProblem> problems;
    KZ_CHECK_OK(kz::phi::verify_provenance(p, root.string(), &problems));
    KZ_CHECK_EQ(problems.size(), static_cast<size_t>(0));
    // a trailing slash on the root changes nothing
    KZ_CHECK_OK(kz::phi::verify_provenance(p, root.string() + "/", &problems));
    KZ_CHECK_EQ(problems.size(), static_cast<size_t>(0));
  }
  {  // the problems, sorted by record
    const std::string wrong(64, 'a');
    Profile p;
    std::vector<Diag> diags;
    KZ_CHECK(profile_with(prov("zz_wrong_hash", "inc/api.h", 2, 3, wrong) + prov("aa_beyond_end", "inc/api.h", 3, 9, wrong) +
                              prov("mm_missing_file", "inc/none.h", 1, 1, wrong) + prov("ok", "inc/api.h", 2, 3, h_api_2_3),
                          &p, &diags));
    std::vector<ProvProblem> problems;
    KZ_CHECK_OK(kz::phi::verify_provenance(p, root.string(), &problems));
    KZ_CHECK_EQ(problems.size(), static_cast<size_t>(3));
    if (problems.size() == 3) {
      KZ_CHECK_EQ(problems[0].prov, std::string("aa_beyond_end"));
      KZ_CHECK(problems[0].message.find("are outside inc/api.h") != std::string::npos);
      KZ_CHECK_EQ(problems[1].prov, std::string("mm_missing_file"));
      KZ_CHECK(problems[1].message.find("cannot read") != std::string::npos);
      KZ_CHECK_EQ(problems[2].prov, std::string("zz_wrong_hash"));
      KZ_CHECK(problems[2].message.find("has sha256 " + h_api_2_3) != std::string::npos);
      KZ_CHECK(problems[2].message.find("the profile says " + wrong) != std::string::npos);
    }
  }
  {  // a changed file is noticed
    Profile p;
    std::vector<Diag> diags;
    KZ_CHECK(profile_with(prov("good_a", "inc/api.h", 2, 3, h_api_2_3), &p, &diags));
    write_file(root / "inc" / "api.h", "line one\nline TWO\nline three\nline four\n");
    std::vector<ProvProblem> problems;
    KZ_CHECK_OK(kz::phi::verify_provenance(p, root.string(), &problems));
    KZ_CHECK_EQ(problems.size(), static_cast<size_t>(1));
  }
  {  // no root, no verification; a root that is not a directory is an I/O error, not a list of problems
    Profile p;
    std::vector<Diag> diags;
    KZ_CHECK(profile_with(prov("good_a", "inc/api.h", 2, 3, h_api_2_3), &p, &diags));
    std::vector<ProvProblem> problems;
    KZ_CHECK_ERR(kz::phi::verify_provenance(p, "", &problems), kz::Err::Invalid);
    KZ_CHECK_ERR(kz::phi::verify_provenance(p, (root / "does-not-exist").string(), &problems), kz::Err::Io);
    KZ_CHECK_ERR(kz::phi::verify_provenance(p, (root / "inc" / "api.h").string(), &problems), kz::Err::Io);  // a file
    std::vector<kz::phi::SymbolProblem> sp;
    kz::phi::SymbolReport rep;
    KZ_CHECK_ERR(kz::phi::verify_symbols(p, (root / "does-not-exist").string(), &sp, &rep), kz::Err::Io);
    KZ_CHECK_ERR(kz::phi::verify_symbols(p, "", &sp, &rep), kz::Err::Invalid);
  }

  // ---- the source that owns a path (the longest root wins; the empty root has the lowest priority)
  {
    Profile p;
    std::vector<Diag> diags;
    KZ_CHECK(profile_with("", &p, &diags));
    const int32_t main_i = p.find_source("main"), libx_i = p.find_source("libx");
    KZ_CHECK(main_i >= 0 && libx_i >= 0);
    KZ_CHECK_EQ(kz::phi::resolve_source(p, "inc/api.h"), main_i);
    KZ_CHECK_EQ(kz::phi::resolve_source(p, "libx/b.c"), libx_i);
    KZ_CHECK_EQ(kz::phi::resolve_source(p, "libx"), libx_i);
    KZ_CHECK_EQ(kz::phi::resolve_source(p, "libxtra/b.c"), main_i);  // a root matches whole path components
    KZ_CHECK_EQ(kz::phi::resolve_source(p, "a/libx/b.c"), main_i);
  }
  {  // without the empty root, a path outside every root has no source
    Profile p;
    std::vector<Diag> diags;
    const std::string text = "phi 1;\nprofile \"t\";\nsource libx root \"libx\" rev \"1234567\";\n";
    KZ_CHECK(kz::phi::load_profile({SourceText{"t.kzp", text}}, &p, &diags).ok());
    KZ_CHECK_EQ(kz::phi::resolve_source(p, "inc/api.h"), -1);
  }

  // ---- the identifiers that entries name (verify_symbols) ---------------------------------------------
  //
  // src/f.c, line by line:
  //   1 int f(void) {          2   set_used(p, 1);     3   p->id = 1;        4   memset(p, 0, sizeof(*p));
  //   5   lock(LIMIT);         6 }                     7 typedef struct { int id; } Box_t;
  //   8 int tbl[TABLE_SIZE];
  write_file(root / "src" / "f.c",
             "int f(void) {\n  set_used(p, 1);\n  p->id = 1;\n  memset(p, 0, sizeof(*p));\n  lock(LIMIT);\n}\ntypedef struct { int id; } Box_t;\nint tbl[TABLE_SIZE];\n");
  {
    std::string f_bytes = "int f(void) {\n  set_used(p, 1);\n  p->id = 1;\n  memset(p, 0, sizeof(*p));\n  lock(LIMIT);\n}\ntypedef struct { int id; } Box_t;\nint tbl[TABLE_SIZE];\n";
    auto rec = [&](const std::string& name, int a, int b) {
      return "prov " + name + " { tier code-derived; kind code_pattern; file \"src/f.c\"; lines " + std::to_string(a) + " " + std::to_string(b) + "; sha256 \"" + hex_of(f_bytes, a, b) + "\"; }\n";
    };
    const std::string records = rec("p_fn", 1, 1) + rec("p_set", 2, 2) + rec("p_store", 3, 3) + rec("p_mem", 4, 4) + rec("p_lock", 5, 5) + rec("p_close", 6, 6) +
                                rec("p_type", 7, 7) + rec("p_tbl", 8, 8) +
                                "prov p_hand { tier researcher; kind researcher; authored \"t\"; }\n"
                                "prov p_gone { tier code-derived; kind code_pattern; file \"src/gone.c\"; lines 1 1; sha256 \"" + std::string(64, 'b') + "\"; }\n";
    const std::string consts = "const LIMIT = 8 prov p_hand;\nconst SZ = ref eval \"TABLE_SIZE\" in \"src/f.c\" prov p_tbl;\n";
    const std::string kind = "kind box(rec: Int) { states A < B; initial A; prov p_hand; }\n";
    auto build = [&](const std::string& op_prov, const std::string& e_call_prov, const std::string& loc_prov, const std::string& extra = "") {
      return records + consts + kind +
             "loc Box.id { struct \"Box_t\"; field \"id\"; holds Int; prov " + loc_prov + "; }\n"
             "site s_lock = fn(\"f\").call(\"lock\", arg[0] == LIMIT) prov p_lock;\n"
             "op op1(Arg) {\n  bind symbol \"f\";\n"
             "  effect e_call: lifecycle box(1) -> B lp at fn(\"f\").call(\"set_used\") prov " + e_call_prov + ";\n"
             "  effect e_store: store Box.id := 1 lp at fn(\"f\").store(Box_t.id) prov p_store;\n"
             "  effect e_mem: store Box.id := 0 lp at fn(\"f\").memset(Box_t) prov p_mem;\n"
             "  prov " + op_prov + ";\n}\n" + extra;
    };
    auto run = [&](const std::string& text, std::vector<kz::phi::SymbolProblem>* problems, kz::phi::SymbolReport* report) {
      Profile p;
      std::vector<Diag> diags;
      const bool ok = profile_with(text, &p, &diags);
      if (!ok)
        for (const Diag& d : diags) std::fprintf(stderr, "%s\n", kz::phi::format_diag(p.files, d).c_str());
      KZ_CHECK(ok);
      if (!ok) return;
      KZ_CHECK_OK(kz::phi::verify_symbols(p, root.string(), problems, report));
    };

    {  // everything is spelled where it is cited
      std::vector<kz::phi::SymbolProblem> problems;
      kz::phi::SymbolReport report;
      run(build("p_fn", "p_set", "p_type"), &problems, &report);
      KZ_CHECK_EQ(problems.size(), static_cast<size_t>(0));
      // claims: bind symbol f; callee set_used; field id (store); callee memset; struct Box_t and field id of the
      // location; callee lock and constant LIMIT of the site; the ref eval symbol TABLE_SIZE
      KZ_CHECK_EQ(report.claims, static_cast<size_t>(9));
      KZ_CHECK_EQ(report.checked, static_cast<size_t>(9));
      KZ_CHECK_EQ(report.unverifiable, static_cast<size_t>(0));
      KZ_CHECK(report.idle_records.empty() || report.idle_records.size() == 1);  // p_close is cited by no entry here
    }
    {  // the case a hash cannot see: the effect cites a range that does not show its callee
      std::vector<kz::phi::SymbolProblem> problems;
      kz::phi::SymbolReport report;
      run(build("p_fn", "p_close", "p_type"), &problems, &report);
      KZ_CHECK_EQ(problems.size(), static_cast<size_t>(1));
      if (problems.size() == 1) {
        KZ_CHECK_EQ(problems[0].entry, std::string("effect op1.e_call"));
        KZ_CHECK_EQ(problems[0].identifier, std::string("set_used"));
        KZ_CHECK(problems[0].message.find("occurs in none of the cited texts: p_close (src/f.c:6-6)") != std::string::npos);
      }
    }
    {  // another cited record of the same entry shows the identifier: no problem, but the record that
       // shows nothing is listed, because that is how a copied-from-the-wrong-place range hides
      std::vector<kz::phi::SymbolProblem> problems;
      kz::phi::SymbolReport report;
      run(build("p_fn", "p_set, p_close", "p_type"), &problems, &report);
      KZ_CHECK_EQ(problems.size(), static_cast<size_t>(0));
      KZ_CHECK_EQ(report.idle_records.size(), static_cast<size_t>(1));
      if (report.idle_records.size() == 1) KZ_CHECK_EQ(report.idle_records[0], std::string("p_close (src/f.c:6-6)"));
    }
    {  // the op's own symbol may be shown by a record of one of its parts; a wrong symbol is reported
      std::vector<kz::phi::SymbolProblem> problems;
      kz::phi::SymbolReport report;
      run(build("p_set", "p_set", "p_type"), &problems, &report);  // p_fn is cited by nobody
      KZ_CHECK_EQ(problems.size(), static_cast<size_t>(1));
      if (problems.size() == 1) {
        KZ_CHECK_EQ(problems[0].entry, std::string("op op1"));
        KZ_CHECK_EQ(problems[0].role, std::string("bind symbol"));
      }
    }
    {  // a wrong struct name of a location; a record that cannot be read; a hand-entered op
      std::vector<kz::phi::SymbolProblem> problems;
      kz::phi::SymbolReport report;
      run(build("p_fn", "p_set", "p_close"), &problems, &report);
      KZ_CHECK_EQ(problems.size(), static_cast<size_t>(2));  // struct Box_t and field id are not on line 6
      run(build("p_fn", "p_gone", "p_type"), &problems, &report);
      KZ_CHECK_EQ(problems.size(), static_cast<size_t>(1));
      if (problems.size() == 1) KZ_CHECK(problems[0].message.find("cannot read") != std::string::npos);
      run(build("p_fn", "p_set", "p_type", "op op2() { bind symbol \"g\"; prov p_hand; }\n"), &problems, &report);
      KZ_CHECK_EQ(problems.size(), static_cast<size_t>(0));
      KZ_CHECK_EQ(report.unverifiable, static_cast<size_t>(1));
      KZ_CHECK_EQ(report.unverifiable_claims.size(), static_cast<size_t>(1));
    }
    {  // a pattern that compares with `!=` names no constant the cited text must spell
      std::vector<kz::phi::SymbolProblem> problems;
      kz::phi::SymbolReport report;
      const std::string text = build("p_fn", "p_set", "p_type") + "site s_ne = fn(\"f\").call(\"lock\", arg[0] != LIMIT) prov p_close, p_lock;\n";
      run(text, &problems, &report);
      KZ_CHECK_EQ(problems.size(), static_cast<size_t>(0));
    }
    {  // the claims are a pure function of the profile
      Profile p;
      std::vector<Diag> diags;
      KZ_CHECK(profile_with(build("p_fn", "p_set", "p_type"), &p, &diags));
      const std::vector<kz::phi::SymbolClaim> c1 = kz::phi::symbol_claims(p), c2 = kz::phi::symbol_claims(p);
      KZ_CHECK_EQ(c1.size(), c2.size());
      bool saw_fn = false;
      for (const kz::phi::SymbolClaim& c : c1)
        if (c.entry == "op op1" && c.role == "bind symbol" && c.identifier == "f") saw_fn = true;
      KZ_CHECK(saw_fn);
    }
  }

  fs::remove_all(root);
  return kztest::finish("test_core_phi_provenance");
}

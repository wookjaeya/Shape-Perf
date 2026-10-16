// phi.hpp -- the profile language: parse, check, canonical text and hash.
//
//   text (.kzp) --parse--> Profile --normalize--> --check--> resolved Profile
//                                                     |
//                                          canonical_text() --> profile_hash()
//
// Parsing stops at the first syntax error and reports it as `file:line:col: error: message`.
// Checking collects every semantic error.  The profile hash covers the canonical content only:
// comments, white space, the order of declarations, the split into files and the spelling of
// integers do not change it (docs/phi_reference.md section 9).
#pragma once

#include <string>
#include <vector>

#include "kz/core/phi_ast.hpp"
#include "kz/core/sha256.hpp"
#include "kz/core/status.hpp"

namespace kz {
namespace phi {

enum class Severity : uint8_t { Error, Warning, Note };

struct Diag {
  Severity severity = Severity::Error;
  SrcLoc loc;
  std::string message;
};

// "file:line:col: error: message".  `files` are the names of Profile::files.
std::string format_diag(const std::vector<std::string>& files, const Diag& d);
bool has_errors(const std::vector<Diag>& diags);

struct SourceText {
  std::string name;  // for diagnostics only
  std::string text;
};

// ---- parse -----------------------------------------------------------------------------

// Parses one file into `into` (appending its declarations).  `file_index` indexes into
// into->files, which the function extends.  On a syntax error the status is Invalid, the error
// is in `diags` and `into` may hold the declarations parsed so far.
Status parse_text(const SourceText& src, Profile* into, std::vector<Diag>* diags);

// Parses a single core expression (for tests and tools).  The expression is not resolved.
Status parse_expression(const std::string& text, ExprPtr* out, std::vector<Diag>* diags);

// Sorts every list of declarations by name (and detects duplicate names), so that indices do
// not depend on the order of files or declarations.  Returns false and reports on duplicates.
bool normalize_profile(Profile* p, std::vector<Diag>* diags);

// ---- check -----------------------------------------------------------------------------

struct CheckOptions {
  bool require_header = true;       // the profile has a name
  bool require_provenance = true;   // every claim (constant, kind, effect, rule, specification ...) cites a provenance record
  bool require_sources = true;      // a provenance file resolves to a declared source
};

// Resolves names, sorts and provenance references and checks the rules of the language
// (docs/phi_reference.md section 8).  Returns true iff there is no error.
bool check_profile(Profile* p, std::vector<Diag>* diags, const CheckOptions& options = CheckOptions());

// Loads a profile from several files: parse each (in the order of the file names), normalize,
// check.  Status Invalid if anything fails; all diagnostics are in `diags`.
Status load_profile(std::vector<SourceText> files, Profile* out, std::vector<Diag>* diags,
                    const CheckOptions& options = CheckOptions());

// An expression is resolved in a context: the parameters `arg.NAME`, the executing actor
// `exec`, and the call result `result`.
struct ArgSpec {
  std::string name;
  Sort sort;
};
struct ExprContext {
  std::vector<ArgSpec> args;
  bool exec = true;
  bool result = false;
};
// Resolves and sort-checks `e` against a checked profile.  `expect` is the sort the context
// wants (Bool for a condition; Unknown = any).
bool resolve_expression(const Profile& p, Expr* e, const ExprContext& ctx, Sort expect, std::vector<Diag>* diags);

// ---- print and hash --------------------------------------------------------------------

std::string canonical_text(const Profile& p);
std::string expr_text(const Expr& e);
std::string pattern_text(const Pattern& pat);
std::string quote_string(const std::string& s);

// SHA-256 of the domain tag followed by the canonical text.  This is the Phi hash of every
// artifact that uses the profile (kz_header.profile_sha256).
Digest profile_hash(const Profile& p);
constexpr const char* kProfileHashDomain = "kz-phi-profile\n";

// ---- statistics ------------------------------------------------------------------------

// Counts of the entries by provenance tier, and of the hand-entered entries.  An entry cites
// several records.  Its strongest cited tier says how good the best part is; its weakest cited
// tier is the tier SPEC 1.6 gives a mixed entry ("a mixed entry takes the weakest tier of its
// parts").  Both are reported, labelled as such; neither is the "source tier" of an instance,
// which `kz report` derives from the caller-level part only (SPEC 1.6).
struct ProfileStats {
  size_t entries = 0;           // declarations that carry provenance
  size_t by_strongest_tier[6] = {0, 0, 0, 0, 0, 0};  // indexed like KZ_TIER (0 unreferenced ... 5 researcher)
  size_t by_weakest_tier[6] = {0, 0, 0, 0, 0, 0};    // the same, by the weakest cited record
  size_t hand_entered = 0;      // entries whose strongest cited tier is `researcher` or `unreferenced`
  size_t declared_trusted = 0;  // trusted entries of class declared_trusted, and effects with that status
  size_t assumptions = 0;       // trusted entries of class assumption
  size_t prov_records = 0;
  std::vector<std::string> hand_entered_names;  // sorted, "kind NAME"
  // Entries that cite at least one record of tier researcher, whatever their strongest tier is (a
  // code-derived record beside a researcher record still leaves a hand-entered part).
  size_t cites_researcher = 0;
  std::vector<std::string> cites_researcher_names;  // sorted, "kind NAME"
  std::vector<std::string> declared_trusted_names;  // sorted, "kind NAME"
  // The caller-level citations of the readiness specifications (SPEC 1.6): the appreq record and the
  // records of every Allowed entry, with tier and side.  They are also counted as entries above
  // ("appreq RSPEC", "allowed RSPEC.KIND#N").  `caller_level_hand_entered` counts the appreqs and
  // entries that cite a researcher or unreferenced record (the checker never lets such a record be
  // caller-side, so they decide nothing; they are listed so that a reader sees them).
  size_t appreq_specified = 0;
  size_t allowed_entries = 0;
  size_t caller_level_hand_entered = 0;
  std::vector<std::string> caller_level_lines;      // in the order of the declarations
};
ProfileStats profile_stats(const Profile& p);
// The report of `kz-phi stats` (also the form of its golden file).
std::string stats_text(const ProfileStats& st);

}  // namespace phi
}  // namespace kz

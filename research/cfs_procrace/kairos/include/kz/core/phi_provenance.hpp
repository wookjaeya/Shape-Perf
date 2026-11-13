// phi_provenance.hpp -- provenance records of a profile: the text hash of a citation, the pairing
// of tiers and kinds, and verification of every citation against the pinned source tree.
//
// A citation is `file`, `lines a b` and `sha256`.  The hash is the SHA-256 of the cited lines
// as they are in the file, each followed by one LF (a last line without a line break is
// treated as if it had one), so `sed -n 'a,bp' file | sha256sum` gives the same value.  The
// file is named relative to the root of the pinned bundle; the source whose root is the longest
// prefix of the path says which tree the path belongs to.
//
// Two mechanical checks exist, and they say different things:
//   verify_provenance  the cited lines are still the lines the profile hashed (the text did not
//                      change, the range is inside the file);
//   verify_symbols     every identifier of the analysed program that an entry names (the symbol
//                      an operation is bound to, the callee, field or type of a pattern, the
//                      struct and field of a location or binding, a constant-evaluated symbol)
//                      occurs in at least one text the entry cites.
// Neither says that the text supports the claim; that is a reading by a person, recorded by the
// reviewer, not computed.  The second check catches the common error of the first kind: a range
// that moved or was copied from the wrong place while its hash was recomputed.
#pragma once

#include <string>
#include <vector>

#include "kz/core/phi_ast.hpp"
#include "kz/core/sha256.hpp"
#include "kz/core/status.hpp"

namespace kz {
namespace phi {

// SHA-256 of lines [from, to] (1-based, inclusive) of `file_bytes`.  False if the file has
// fewer than `to` lines or the range is empty.
bool cited_text_hash(const std::string& file_bytes, uint32_t from, uint32_t to, Digest* out);

// The text of lines [from, to] (1-based, inclusive), each followed by LF.  False as above.
bool cited_text(const std::string& file_bytes, uint32_t from, uint32_t to, std::string* out);

// Whether `id` occurs in `text` as a whole identifier (not inside a longer identifier).
bool text_has_identifier(const std::string& text, const std::string& id);

// Index of the source whose root is the longest prefix of `path` (a root matches the path
// itself or `root/...`; the empty root matches everything with the lowest priority); -1 if none.
int32_t resolve_source(const Profile& p, const std::string& path);

// The tier of a provenance kind (SPEC 1.6 "Naming and source tiers"): contract-backed (api_doc),
// requirement-derived (requirement), admitted (code_admission), issue-derived (issue),
// code-derived (code_pattern), researcher (researcher), unreferenced (none).  `tier` and `kind`
// are the display names of the schema's vocabularies.  The schema vocabulary KZ_TIER has no
// `requirement-derived` yet, so a profile cannot name that tier and a `requirement` record is
// rejected by the checker with a message that says so (docs/phi_reference.md section 10).
bool tier_kind_compatible(const std::string& tier, const std::string& kind);
// Whether a record of this tier may have side `caller` (SPEC 1.6 "Three judgements", QT-4, QT-9): a
// caller-side record is a cited text that states what the using code must achieve or how it handles
// an outcome.  Allowed: contract-backed (an API document addressed to the caller), requirement-derived
// (an application or mission requirement), admitted (a comment in the using code that admits the
// handling, SPEC 3.9 "side caller, code_admission") and issue-derived (a maintainer's statement in an
// issue or commit).  Not allowed: code-derived (a code pattern is a fixed rule of the tool, side
// `rule`), researcher and unreferenced (hand-entered: a researcher rule alone never accepts an
// outcome, QT-4, and no researcher-authored appreq turns an occurrence into a readiness violation,
// QT-9).  The checker rejects the other pairings; derive_caller_allowed() reads such a record as a
// rule if it meets one in a profile that was not checked.
bool caller_side_tier(const std::string& tier);
// Rank of a tier in the order of SPEC 1.6 (0 contract-backed, 1 requirement-derived, 2 admitted,
// 3 issue-derived, 4 code-derived, 5 researcher, 6 unreferenced); the lower, the stronger.  -1 for
// an unknown word.
int tier_rank(const std::string& tier);
// Index of the tier in the schema's vocabulary (KZ_TIER); -1 for an unknown word.
int tier_index(const std::string& tier);

struct ProvProblem {
  std::string prov;     // name of the provenance record
  std::string message;
};

// Re-reads every citation of the profile under `bundle_root` and compares the hash.  Records of
// kind researcher without a file are skipped.  Returns the problems; empty means every cited
// text is what the profile says it is.  Status is not Ok for an unusable `bundle_root` (not a
// directory): the problems are then not meaningful.
Status verify_provenance(const Profile& p, const std::string& bundle_root, std::vector<ProvProblem>* problems);

// The revision pin of a source (`source NAME root R rev X`) against the bundle on disk.  The commit
// that is checked out is read from the git metadata of the source tree as files (`.git` as a directory
// or a `gitdir:` file, then `HEAD`, a loose ref or `packed-refs`); no git command runs, so a read-only
// bundle stays untouched.  What this does NOT verify: that the working tree has no uncommitted edits
// (that needs the index and the objects).  verify_provenance covers the cited lines themselves.
enum class RevState : uint8_t { Match, Mismatch, Unverifiable };
const char* rev_state_text(RevState s);

struct RevisionCheck {
  std::string source;   // the source name
  std::string root;     // its root, relative to the bundle
  std::string pinned;   // the `rev` of the profile
  std::string found;    // the checked-out commit (empty if it cannot be read)
  RevState state = RevState::Unverifiable;
  std::string message;  // Unverifiable: why
};

// One check per declared source, in the order of Profile::sources (sorted by name after normalize).
// Status is not Ok for an unusable `bundle_root`.
Status verify_revisions(const Profile& p, const std::string& bundle_root, std::vector<RevisionCheck>* out);

// An identifier of the analysed program that an entry of the profile names, and the records whose
// cited lines may show it.
struct SymbolClaim {
  std::string entry;       // "op NAME", "effect OP.LABEL", "conjunct OP.LABEL", "loc NAME", ...
  std::string role;        // "bind symbol", "callee", "field", "type", ...
  std::string identifier;
  std::vector<int32_t> prov;  // indices into Profile::provs; the records of the entry (see symbol_claims)
};

// The claims of a checked profile, in the order of the declarations.  The function whose body a
// pattern searches (`fn(F)`) is not claimed: it is the enclosing function, which the citation of
// the operation shows, and a cited line inside it does not name it.  A pattern's callee, the
// field of a store or load, the type of a memset, the constant an argument is compared with, the
// struct and field of a location or binding, the symbol an operation is bound to and the symbol
// of a `ref eval` constant are claimed.
std::vector<SymbolClaim> symbol_claims(const Profile& p);

struct SymbolProblem {
  std::string entry, role, identifier;
  std::string message;
};

struct SymbolReport {
  size_t claims = 0;
  size_t checked = 0;        // claims with at least one cited text to look in
  size_t unverifiable = 0;   // claims whose entry cites no text (hand-entered records only)
  std::vector<std::string> unverifiable_claims;  // "ENTRY: ROLE 'IDENTIFIER'", in the order of the claims
  // Cited records that show none of the identifiers that any entry citing them names (only entries
  // that name identifiers count).  Not a failure: a text can be cited as context.  It is the list
  // a person reads first, because a range copied from the wrong place passes the lookup of a claim
  // as long as another record of the same entry shows the identifier.
  std::vector<std::string> idle_records;  // "NAME (file:a-b)", sorted
};

// Looks every claim up in the cited lines under `bundle_root`, and lists the cited records that show
// none of the identifiers of the entries that cite them (SymbolReport::idle_records).  A claim whose records cite no file
// is counted as unverifiable, not as a problem.  A cited text that cannot be read is a problem of
// the claims that need it.
Status verify_symbols(const Profile& p, const std::string& bundle_root, std::vector<SymbolProblem>* problems, SymbolReport* report);

}  // namespace phi
}  // namespace kz

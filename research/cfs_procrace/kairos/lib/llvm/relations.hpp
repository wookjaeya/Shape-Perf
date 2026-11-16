// relations.hpp -- `kz-static relations`: the relations of a facts container, without a case input.
//
// The query reads one facts container (the extended facts F1-F8 and the target-independent rules:
// `extract --no-profile`, or a profile-driven extraction with `--extended-facts`) and writes, for the
// whole program, the rows of three kinds of relation (plan/extraction_design_20261030.md §3.1-§3.4):
//
//   A  name / id / registration tables: a key comparison (a call of the byte-string comparison family of
//      P0, or `==` / `!=`) between a record field and a parameter, in a search loop or in a callback a
//      search loop calls; the functions whose parameter reaches it (PARAM_FLOW); the calls of them from
//      outside (uses), split into lookups and creators (a creator reaches a write of the key or of an
//      active field); per lookup, the creators of the same partition and key in other actors;
//   B  a shared field passed as a call argument (ARGFLOW, shared base) to a function with a body, when
//      the field has writers in another actor; with the callee's check of that parameter (RETURNS);
//   C  creation and registration: a spawn call, the writes of its creator after it, and the reads of the
//      same fields in the code the spawned entry reaches, with the lock sets (LOCKSET, LOCK_IDS).
//
// Every row has separate columns: dependency basis, run order (from a run record only; this revision
// reads none, so the column says so), feasibility of the other order (static, graded: proven impossible
// / possible / undecidable, with the reason), violation basis (RV2 §5.4: an observed failure with a
// feasible other order, or a realized forced order; without a record: unconfirmed), the counterfactual
// outcome-flip signal (static-possible / static-undetermined / none), and the input layers the row rests
// on (B build, P0 primitives, L0 loader semantics of the startup script, R rules; a case profile or a
// hand-entered vocabulary of the facts is named as such).  The query takes no field, callee or case
// name.  Deterministic: the same container gives the same bytes.  No Clang types.
//
// The rows are records (RelRow): the text is rendered from them (render_relations), so a tool that reads the rows
// reads what the text says, with references to the facts the row stands on and a use-site key that names where
// the row's use is in the program (docs/static_facts.md §5l).  The facts are loaded and indexed by RelFacts
// (rel_facts.hpp), which other readers of the same facts share.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "kz/core/container.hpp"

namespace kzs {

const char* relations_rules_revision();

struct RelationsOptions {
  // nothing yet: the query has no parameter that selects what it reports
};

// A line of a row between its first line and its columns: what it is (`name`: "site", "creator", "write after the
// spawn", ...), its text as written without the indentation, and the indentation.
struct RelLine {
  std::string name;
  std::string text;
  uint8_t indent = 6;
};

// One row of path A, B or C.  The facts references are row numbers of the container's sections (KZ_NONE32: none).
struct RelRow {
  char path = 0;          // 'A', 'B' or 'C'
  uint32_t number = 0;    // the row's number in its path (A1, A2, ...), as in the text
  std::string head;       // the first line after "A<number> "
  std::vector<RelLine> lines;
  // the columns, each without its label
  std::string dependency, run_order, feasibility, violation, flip, layers;
  std::string feasibility_grade, flip_grade;  // the grade the row was counted under in the summary line
  // what the row stands on
  std::string subject;                 // A: the namespace (the key field Type.field); B and C: the field
  uint32_t call = KZ_NONE32;         // FACTS: A the lookup call, C the spawn call
  uint32_t caller_fn = KZ_NONE32;    // FUNCS: A the function of the lookup call, C the creator
  uint32_t callee_fn = KZ_NONE32;    // FUNCS: A the key-parameter function, B the callee, C the spawn call's callee (none: a primitive)
  uint32_t param = KZ_NONE32;        // A the key parameter, B the argument
  std::vector<uint32_t> site_calls;    // FACTS: B the calls of the sites
  uint32_t read = KZ_NONE32;         // READS: C the read
  uint32_t read_fn = KZ_NONE32;      // FUNCS: C the reading function
  std::string context_kind;            // C: itself, entry, call, indirect-call, callback, unresolved
  uint32_t context_call = KZ_NONE32; // FACTS: C the call of the context
  // the use-site key: key_text and the first 16 hex digits of its sha256; B adds a key per site ("<key> <text>")
  std::string key_text, key;
  std::vector<std::string> site_keys;
};

// A part of the text: the lines that are no row (a path's heading line, a namespace of path A), then its rows.
struct RelGroup {
  std::string text;
  std::vector<RelRow> rows;
};

struct RelReport {
  std::string header;  // the lines before the first path
  std::vector<RelGroup> groups;
  std::string facts;   // the facts container's artifact hash (hex)
};

kz::Status relations_rows(const kz::Container& facts, const RelationsOptions& options, RelReport* out);
std::string render_row(const RelRow& row);
std::string render_relations(const RelReport& report);
// the use-site key listing: a header, the keys two or more rows share, then one line per row (and per path B site)
std::string relation_keys_text(const RelReport& report);

// relations_rows, rendered
kz::Status relations_report(const kz::Container& facts, const RelationsOptions& options, std::string* out);

}  // namespace kzs

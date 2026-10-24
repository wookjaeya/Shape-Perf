// verdict.hpp -- helpers for the status, verdict, certificate and trust-ledger records.
//
// These functions do not decide anything about a program.  They keep the records honest:
//   - defaults are conservative (D20: progress is unknown(not_checked) until a component
//     establishes it; a lemma is unproved until the lemma table says otherwise);
//   - the structural invariants of SPEC 1.10, 2.3, 4.5.1 and 6.2 (PS1-PS4) are checked;
//   - rendering follows the rules that a class is never shown without its reach and
//     progress, that a certificate-based statement is shown with the proof status of the
//     lemmas it rests on, and that nothing is called proved, verified or certified on the
//     strength of an unproved lemma.
// There is no "safe" or "live" output anywhere.
#pragma once

#include <string>
#include <vector>

#include "kz/schema/kz_schema.h"

namespace kz {

struct Finding {
  std::string where;  // e.g. "cell", "entry[3]"
  std::string what;
};

// ---- defaults -----------------------------------------------------------------------
// All-zero record with the sigma and M level set.  Because 0 is the conservative value of
// every axis, the result is: class unknown(not_reached), reach assumed, progress
// unknown(not_checked), fw_conf unknown, no routes, proof status n/a.
kz_verdict_cell make_cell(uint8_t sigma, uint8_t mlevel);
kz_verdict make_verdict(uint64_t instance);
// Zeroed entry; site, refutes = none; bound = infinity (SPEC 2.3); a LEMMA entry is unproved.
kz_trust make_trust_entry(uint8_t part, uint8_t kind);

// ---- safety routes ------------------------------------------------------------------
uint8_t route_get(uint8_t routes, unsigned shift);
uint8_t route_set(uint8_t routes, unsigned shift, uint8_t route);

// ---- proof status (D20, SPEC 6.2) ---------------------------------------------------
const char* proof_status_name(uint8_t proof);
// Weakest status among the LEMMA entries (unproved < depends-on-stated-assumption < proved);
// KZ_PROOF_NA if there is no LEMMA entry.
uint8_t weakest_proof(const std::vector<kz_trust>& entries);

// Counts derived from ledger entries.
struct LedgerCounts {
  uint32_t n_rechecked = 0, n_extracted = 0, n_trusted = 0, n_refutable = 0;  // sizes of parts (i)-(iv)
  uint32_t lemma_proved = 0, lemma_depends = 0, lemma_unproved = 0;
  uint32_t declared_trusted = 0;  // part (iii) entries of kind DECLARED_TRUSTED
  // `ident` of every LEMMA entry whose status is not `proved` (an invalid status included),
  // ascending, without repeats: PS3 names the lemmas that are not proved.  The ident is the
  // key into the lemma table of the checker; this layer has no names for them.
  std::vector<uint32_t> lemma_not_proved;
};
LedgerCounts count_ledger(const std::vector<kz_trust>& entries);
// Ledger record for `entries`, stored at TRUST[first, first + entries.size()); references left null.
kz_trust_ledger make_ledger(const std::vector<kz_trust>& entries, uint32_t first);

// "ledger:<i>/<ii>/<iii>/<iv>", "trusted:<n>", "lemmas:<p>/<d>/<u>" (SPEC 1.10).
std::string ledger_label(const kz_trust_ledger& ledger);
std::string trusted_label(const LedgerCounts& counts);
std::string lemmas_label(const LedgerCounts& counts);

// ---- validation ---------------------------------------------------------------------
std::vector<Finding> validate_cell(const kz_verdict_cell& cell);
// The cell's proof_status is the weakest status among the lemma entries of its ledger.
std::vector<Finding> validate_cell_proof(const kz_verdict_cell& cell, const std::vector<kz_trust>& ledger_entries);
std::vector<Finding> validate_verdict(const kz_verdict& verdict, const std::vector<kz_verdict_cell>& cells);
// Checks one entry on its own.  A LEMMA entry marked `proved` must name, in `source`, a
// PROVENANCE record (the record of the cited text that holds the proof); that the record
// exists and cites a text is checked by validate_proof_texts, and that the text really proves
// the lemma cannot be checked by code: the status is the author's classification (PS1).
std::vector<Finding> validate_trust_entry(const kz_trust& entry);
// For every `proved` LEMMA entry whose `source` is a record of this container: the PROVENANCE
// record exists, cites a path or requirement id (`file`) and carries the hash of the cited
// text (`text_sha256` is not zero).  A source in a parent artifact is not checked here.
std::vector<Finding> validate_proof_texts(const std::vector<kz_trust>& entries, const std::vector<kz_prov>& provenance);
// Entries are valid and the part sizes agree with the record.
std::vector<Finding> validate_ledger(const kz_trust_ledger& ledger, const std::vector<kz_trust>& entries);
// Certificate invariants; proof_status is the weakest status among the entries of its ledger.
std::vector<Finding> validate_cert(const kz_cert& cert, const std::vector<kz_trust>& ledger_entries);

// ---- rendering ----------------------------------------------------------------------
// One-line text of a cell (SPEC 1.10 rendering rule).
//   - A universal class (protected, fixed-order, no-final-status) is always rendered with its
//     bound: `primary`, the bound the class was computed at.  Without it the text says [b=?].
//     The bound also completes reach `model(b)` / `unreached(b)` and the stall realization
//     `model(b)`; `progress_bound` completes progress `no-stall-within(b)`; "?" if missing.
//   - An unknown cell shows the statements and witnesses it retains (`no_decision_route` shows
//     the established statement, `one_sided` the witnesses); schedule-dependent and
//     ordinal-split show their witnesses.
//   - A stall witness is rendered with the clauses it holds (pre-use, in-use, horizon), its kind
//     (deadlock-cycle, leaked-lock or other), its qualifiers, harm and realization, and with the
//     reference of the witness record that holds its wait-for root set.  The root set itself is
//     in that record and is not resolved here.
//   - A certificate-based statement carries "lemma status: <status>".  With `lemmas` (the counts
//     of the cell's own ledger) the status is checked against the ledger: a stored status that is
//     not the weakest status of the ledger is rendered as INCONSISTENT and never as proved.
//     Without `lemmas`, `proved` is rendered as "not checked against a ledger" (PS2).
std::string render_cell(const kz_verdict_cell& cell, const LedgerCounts* lemmas = nullptr, const kz_bound* primary = nullptr,
                        const kz_bound* progress_bound = nullptr);
// "accepted by kz proofcheck, relative to its trusted base; lemma status: <status>" (PS3), with
// the same ledger check as render_cell.
std::string render_acceptance(const kz_cert& cert, const LedgerCounts* lemmas = nullptr);

// ---- summary view (SPEC 1.10 "Summary view", ADR-054, D2; schema revision 2) --------
// The observation summary is a view computed last from the record; it never replaces the status.
// Each row of the D2 table applies only under its condition (never a mechanical substitution):
//   VIOLATING           OBSERVED_VIOLATION if the violation holds in every explanation of the record
//                       (SPEC 4.8), else INCONCLUSIVE(explanation_ambiguous)
//   READY               NO_VIOLATION_IN_CHECKED_PREFIX
//   TOLERATED           NO_VIOLATION_IN_CHECKED_PREFIX only with a caller record and the horizon
//                       monitor's ACCEPT (QT-4); else INCONCLUSIVE(caller_requirement_absent)
//   NOT_READY_UNJUDGED  INCONCLUSIVE(caller_requirement_absent)
//   PENDING, OPEN       INCONCLUSIVE(horizon-open)
//   NOT_EVALUATED       INCONCLUSIVE(not_evaluated)
//   undetermined        INCONCLUSIVE(the reasons of the record; explanation_ambiguous if none is known)
// No status maps to NOT_APPLICABLE (it needs its own evidence).  An empty explanation set (the record
// and the model contradict each other) is an analysis input error: observation `none` with the flag
// input-error, reported apart, never a violation and never no violation.
struct SummaryInput {
  uint8_t status = 0;             // KZ_OCC
  uint8_t status_flags = 0;       // set KZ_STATUS_FLAG: open (no decision in the trace), undetermined
  bool all_explanations = false;  // VIOLATING: the violation holds in every explanation of the record
  bool caller_record = false;     // TOLERATED: the Allowed entry rests on a caller record
  bool accepted = false;          // TOLERATED: the horizon monitor ACCEPTed
  bool input_error = false;       // the explanation set is empty (Qv and Qn both UNSAT)
  uint32_t reasons = 0;           // set KZ_WITHHOLD: what the record lacks (for INCONCLUSIVE)
};
// A summary record with observation, obs_reasons, status, status_flags and the flags input-error,
// caller-record and accept set; prediction and confirmation NOT_RUN, indices KZ_NONE32.
kz_summary summarize_observation(const SummaryInput& in);
// "OBSERVED_VIOLATION", "INCONCLUSIVE(horizon-open)", "analysis input error (reported apart)" ...
std::string render_observation(const kz_summary& s);
// The invariants of the D2 table on a stored summary.
std::vector<Finding> validate_summary(const kz_summary& s);

}  // namespace kz

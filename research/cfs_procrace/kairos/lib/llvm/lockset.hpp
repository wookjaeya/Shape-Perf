// lockset.hpp -- F5 LOCKSET and the writer-set and L1 (i) rules that do not depend on the targets
// (kz_facts.h revision 8; no Clang types).
//
// The walk (ast_fn.cpp) records, with spec.extended_facts, the lock-set skeleton of every body (its
// control-flow graph with the calls, reads, typed writes, definitions of locals and returns in
// evaluation order, and the test of each two-way branch against a constant), the identity of every
// call argument as an expression over the caller's parameters (model.hpp LockIdNode), every typed
// write site, and the fields of the record types the reads name.  This file then derives:
//
//   F5 LOCK_OPS   the lock operations, from the mutex primitives of P0 (acquire, try or timed
//                 acquire, release, condition wait) through the calls, bottom-up over the strongly
//                 connected components of the call graph, callees first.  A body is read by a
//                 forward analysis on its control-flow graph whose state is the set of locks held
//                 (one state per distinct set: the paths that hold a lock and those that do not stay
//                 apart, so that an operation which holds the lock exactly when it returns a certain
//                 value is seen as such).  An acquisition that is not a try is read under a declared
//                 assumption: it succeeds (the primitive returns 0, a derived operation the value it
//                 returns when it holds the lock), and a branch on its result that only a failure
//                 takes is a failure path.  A release of a lock the function did not acquire is a
//                 release of the caller's lock.  A pair the function acquires and releases itself
//                 leaves the caller's locks alone when its identity is a static instance or a fixed
//                 address; otherwise the release may be of another mutex, and it releases every
//                 held lock whose field-based location it may share.  A call that cannot be followed
//                 (an unresolved indirect call, a call into the function's own recursive component)
//                 releases every lock;
//   LOCK_IDS      the identities the rows hold and whether each is a static instance;
//   LOCKSET       the locks held at every call, read and write, relative to the function's entry;
//   LOCK_PATHS    per LOCKSET event, the spawn calls of its function it follows on the control-flow graph, and
//                 whether a call on some path from the function's entry to it releases a lock the function does
//                 not hold, may release one, or has a lock effect the analysis cannot follow;
//   FIELD_STATUS  the writer-set completeness of every field of the record types the reads name,
//                 per field and type-level, from a completeness pass (an extraction of its own whose
//                 targets are exactly those fields; ExtractSpec::keep_raw_atypes);
//   AWAIT_L1      per poll loop, L1 (i) with the locations its predicate reads, directly and through
//                 the functions it calls, under each completeness rule, and L1 (ii) from LOCKSET.
#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "model.hpp"
#include "spec.hpp"

namespace kzs {

const char* lock_rules_revision();

// The text of an identity ("$0", "&tbl[$0].id", "&tbl[G.f].id", "?(why)").
std::string lock_id_text(const LockId& id);
// The identity with each parameter $k replaced by args[k].
LockId lock_id_subst(const LockId& id, const std::vector<LockId>& args);
// The field-based location of the mutex object an identity addresses ("Type.field", "<global>.name"), or "".
std::string lock_id_may(const LockId& id);

// The targets of the completeness pass: every field of every record type a READS row names, sorted.
std::vector<FieldRef> completeness_fields(const Model& m);

// The FIELD_STATUS rows from a completeness pass (`pass`, finalized with keep_raw_atypes) and the
// program it was read from (`m`: calls, targets and functions, for the writer instances that run once).
struct FieldWriter {
  std::string func_key, call_key;
  Pos pos;
  uint8_t kind = KZ_WRITER_KIND_NONE;
};
struct FieldStatusTable {
  std::map<std::pair<std::string, std::string>, FieldStatusFact> rows;
  std::map<std::pair<std::string, std::string>, std::vector<FieldWriter>> writers;
  uint32_t targets = 0;
  const FieldStatusFact* find(const std::string& type, const std::string& field) const;
};
void field_status_from(const Model& pass, const Model& m, FieldStatusTable* out);

// With the extended facts: the indirect calls whose resolved candidates may not be all the functions they call (a node
// the function value comes from holds values the flow graph does not follow: a variable or field of a type that is no
// function pointer, the result of a function without a body, a parameter of an address-taken function, an address
// passed converted to another pointer type, a field whose FIELD_STATUS row is missing, not complete per field or has a
// writer that is no assignment) get a CALL_TARGETS row with no function and reason incomplete-source.  Returns their
// number (also Model::Resolved::indirect_incomplete).
uint32_t mark_incomplete_targets(Model* m, const FieldStatusTable& fields);

// F5 over a finalized model read with the extended facts.  `fields` gives the static-instance check;
// without it no value read by an identity is one.
struct LockDerivation {
  std::set<std::string> lock_keys, unlock_keys;  // the vocabulary of a profile-free second pass
  std::vector<LockOpFact> ops;
  std::vector<LockSetFact> rows;                 // `held` are indices into `ids`
  std::vector<LockIdFact> ids;
  std::vector<LockPathFact> paths;               // `row` are indices into `rows`
  Model::LockStats stats;
};
void derive_locks(const Model& m, const FieldStatusTable* fields, LockDerivation* out);

// Writes the rows of LOCK_OPS, LOCK_IDS, LOCKSET, LOCK_PATHS, FIELD_STATUS and AWAIT_L1 into the model (finalized,
// with the extended facts), from its derivation and the completeness pass.  `vocabulary` marks the
// operations a profile-free second pass had as lock and unlock operations (empty otherwise).
void finalize_lock_facts(Model* m, const LockDerivation& d, const FieldStatusTable& fields, const std::set<std::string>& vocabulary);

}  // namespace kzs

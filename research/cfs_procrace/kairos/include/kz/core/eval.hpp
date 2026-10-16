// eval.hpp -- ONE evaluator of the core expression language, and its domains.
//
// The evaluator (eval_evaluator.hpp) is a template over a domain: it walks a resolved
// expression (phi_ast.hpp) and asks the domain for every primitive: literals, the Boolean
// connectives, comparisons, the state of a resource, the value of a location, the tag of a
// resource, the owner of the executing actor, the instances of a finite kind.  Three domains
// are foreseen (SPEC 2.3); two exist:
//
//   ConcreteDomain  exact values over a concrete state (what a trace says at an event)
//   AbstractDomain  intervals, sets of states and three-valued Booleans {true, false, maybe}
//                   over an abstract state (what a static analysis knows)
//
// The operational domain of the explorer (S7) is another instance of the same template; it is
// a ConcreteDomain over the explorer's states and needs no new evaluator.
//
// Soundness of the abstract domain, tested by tests/property/test_eval_agreement.cpp: if the
// concrete state s is in the concretization of the abstract state a, and the concrete and the
// abstract arguments agree in the same way, then the concrete value of every expression is in
// the concretization of its abstract value.  The concretization is defined in the comments of
// AbstractState.
//
// Conventions the profile language fixes (docs/phi_reference.md section 6):
//   - a resource that was never given a state is in the initial state of its kind, so
//     state(K(key)) is total;
//   - the first value of a tag is the value of a resource that never set it;
//   - an Id is a 32-bit value.  Both domains read a value of sort Id modulo 2^32 where it enters
//     (an argument, the value of a location, the id of a resource, a key), so an Id is always in
//     0 .. 0xFFFFFFFF whatever the recorder delivered.  A comparison of an Id with an Int
//     compares the 64-bit values as they are: only the Id operand is reduced, the Int operand is
//     not, so `x = -1` and `x = 0x1FFFFFFFF` are false and `x < 0x100000000` is true for every Id
//     (the checker warns about an integer literal or constant outside 0 .. 0xFFFFFFFF compared
//     with an Id: its result is decided by the sorts alone).  valid(x) holds iff
//     (x - base) mod 2^32 <= serial_max in the id space of the kind, so the undefined and the
//     reserved markers are never valid;
//   - owner(a) is undefined if the actor has no owner, and if its owner is not a record of the
//     owner kind of the context (another kind or another key width: such a state is ill-formed
//     and both domains read the owner as undefined); the state of an undefined resource is
//     the initial state of its kind, its id is the undefined marker of the kind, its tags are
//     the first values.
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "kz/core/phi_ast.hpp"

namespace kz {
namespace eval {

// ---- resources ----------------------------------------------------------------------------

// The value of sort Id that a 64-bit number stands for: its residue modulo 2^32, in 0 .. 0xFFFFFFFF.
int64_t id_value(int64_t v);

// The handle of a string value: a 64-bit FNV-1a hash, so that domains need no shared table.
// Two different strings can in principle have the same handle; the tests do not hit one.
int64_t str_handle(const std::string& s);

// Number of elements of the key of a kind: one per parameter, a Res<K> parameter counts as the
// whole key of K.
size_t key_width(const phi::Profile& p, int32_t kind);

// A resource: a kind and its key.  The key has one element per parameter of the kind.  An Id
// parameter holds the id value, a Res parameter the (first element of the) key of the other
// resource, an Actor parameter the actor number, a Loc parameter the index of the location.
struct ResId {
  int32_t kind = -1;
  std::vector<int64_t> key;
  bool operator<(const ResId& o) const { return kind != o.kind ? kind < o.kind : key < o.key; }
  bool operator==(const ResId& o) const { return kind == o.kind && key == o.key; }
};

// ---- concrete domain ------------------------------------------------------------------------

struct CVal {
  phi::SortKind sort = phi::SortKind::Unknown;
  int64_t v = 0;      // Bool 0/1, Int, Id value, State/Tag ordinal, Actor number, Loc index, Str handle
  int32_t kind = -1;  // Id, State, Tag, Res: the kind (Id: -1 if not known)
  int32_t tag = -1;   // Tag
  bool defined = true;           // false: an undefined resource
  std::vector<int64_t> key;      // Res
  bool is_true() const { return sort == phi::SortKind::Bool && v != 0; }
};

// The concrete state a profile is evaluated in.
struct ConcreteState {
  std::map<ResId, int32_t> state;                       // resources that left their initial state
  std::map<std::pair<ResId, int32_t>, int32_t> tags;    // (resource, tag index) -> value ordinal
  std::map<ResId, int64_t> ids;                         // the id a resource carries
  std::map<int32_t, int64_t> loc_values;                // location index -> value
  std::map<int32_t, std::vector<std::vector<int64_t>>> instances;  // finite kind -> keys, ascending
  bool has_owner = false;
  ResId owner;
  int64_t exec = 0;
  std::vector<int64_t> args;     // parallel to the ArgSpec list of the context
  int64_t result = 0;
  std::map<int32_t, int64_t> consts;  // values of the constants that the extractor resolves (ref eval)
};

// A state with the INIT facts of the profile applied (location values), nothing else.
ConcreteState initial_state(const phi::Profile& p);

class ConcreteDomain {
 public:
  using Val = CVal;
  ConcreteDomain(const phi::Profile& p, const ConcreteState& s) : p_(p), s_(s) {}

  Val lit_bool(bool b) const;
  Val lit_int(int64_t v) const;
  Val lit_str(const std::string& s) const;
  Val lnot(const Val& a) const;
  Val land(const Val& a, const Val& b) const;
  Val lor(const Val& a, const Val& b) const;
  Val cmp(phi::CmpOp op, const Val& a, const Val& b) const;

  Val arg(int32_t index, const phi::Sort& sort) const;
  Val exec_actor() const;
  Val result_code() const;
  Val constant(int32_t index) const;
  Val state_constant(int32_t kind, int32_t ordinal) const;
  Val tag_constant(int32_t kind, int32_t tag, int32_t ordinal) const;
  Val loc_ref(int32_t index) const;

  Val make_res(int32_t kind, const std::vector<Val>& keys) const;
  Val state_of(const Val& res) const;           // stored state; the initial state if none (derived kinds: see the evaluator)
  Val key_of(const Val& res, size_t index, const phi::Sort& sort) const;
  Val gen_of(const Val& res) const;
  Val valid(const Val& id) const;
  Val value_of(const Val& loc) const;
  Val tag_of(const Val& res, int32_t tag) const;
  Val id_of(const Val& res) const;
  Val owner_of(const Val& actor) const;
  std::vector<Val> instances_of(int32_t kind) const;
  // The state of a derived kind: ordinals[i] if conds[i] is the first true one, else `fallback`.
  Val select_state(int32_t kind, const std::vector<Val>& conds, const std::vector<int32_t>& ordinals, int32_t fallback) const;

 private:
  const phi::Profile& p_;
  const ConcreteState& s_;
};

// ---- abstract domain ----------------------------------------------------------------------------

enum class Tri : uint8_t { False = 0, True = 1, Maybe = 2 };

struct AVal {
  phi::SortKind sort = phi::SortKind::Unknown;
  Tri tri = Tri::Maybe;       // Bool
  int64_t lo = 0, hi = 0;     // Int, Id, Actor, Loc, Str: an interval (a handle for Str is exact)
  uint64_t mask = 0;          // State, Tag: the set of possible ordinals (bit i = ordinal i)
  int32_t kind = -1;
  int32_t tag = -1;
  bool may_be_undefined = false;   // Res
  bool must_be_undefined = false;  // Res
  std::vector<std::pair<int64_t, int64_t>> key;  // Res: one interval per key element
};

// What a static analysis knows.  Its concretization: a concrete state s is described by it iff
//   - every resource listed in `res` has a state, tag values and an id inside the listed sets;
//   - every resource of a kind not listed there is in the initial state of its kind
//     (a listed resource is the way to say "unknown");
//   - every location listed in `locs` holds a value inside its interval, every location not
//     listed holds 0 (an interval of a location that holds an Id is read modulo 2^32, like every
//     Id; so is the interval of an Id-sorted argument and of the id of a resource);
//   - the executing actor and the result are inside their intervals; an argument that has no
//     interval (`args` is shorter than the parameter list) is unknown: any value of its sort (an
//     Id: 0 .. 0xFFFFFFFF); the owner is one of the listed owners, or undefined if `owner_none`
//     holds (a listed owner that is not a record of the context's owner kind, with its key width,
//     is read as undefined, as the concrete domain reads it);
//   - the instances of a finite kind are exactly the listed keys.
struct AbstractRes {
  uint64_t states = 0;                       // possible state ordinals
  std::map<int32_t, uint64_t> tags;          // tag index -> possible value ordinals (absent: the first value only)
  std::pair<int64_t, int64_t> id{0, 0};      // possible ids (kinds with ids)
  bool has_id = false;
};

struct AbstractState {
  std::map<ResId, AbstractRes> res;
  std::map<int32_t, std::pair<int64_t, int64_t>> locs;
  std::map<int32_t, std::vector<std::vector<int64_t>>> instances;
  bool owner_none = true;
  std::vector<ResId> owners;                 // possible owners of the executing actor
  std::pair<int64_t, int64_t> exec{0, 0};
  std::vector<std::pair<int64_t, int64_t>> args;
  std::pair<int64_t, int64_t> result{0, 0};
  std::map<int32_t, std::pair<int64_t, int64_t>> consts;  // ref eval constants (absent: unknown)
};

// Whether the concrete value is one of the values the abstract value stands for.
bool contains(const AVal& a, const CVal& c);
// The abstraction of a concrete value that stands for exactly it.
AVal exact(const CVal& c);

class AbstractDomain {
 public:
  using Val = AVal;
  AbstractDomain(const phi::Profile& p, const AbstractState& s) : p_(p), s_(s) {}

  Val lit_bool(bool b) const;
  Val lit_int(int64_t v) const;
  Val lit_str(const std::string& s) const;
  Val lnot(const Val& a) const;
  Val land(const Val& a, const Val& b) const;
  Val lor(const Val& a, const Val& b) const;
  Val cmp(phi::CmpOp op, const Val& a, const Val& b) const;

  Val arg(int32_t index, const phi::Sort& sort) const;
  Val exec_actor() const;
  Val result_code() const;
  Val constant(int32_t index) const;
  Val state_constant(int32_t kind, int32_t ordinal) const;
  Val tag_constant(int32_t kind, int32_t tag, int32_t ordinal) const;
  Val loc_ref(int32_t index) const;

  Val make_res(int32_t kind, const std::vector<Val>& keys) const;
  Val state_of(const Val& res) const;
  Val key_of(const Val& res, size_t index, const phi::Sort& sort) const;
  Val gen_of(const Val& res) const;
  Val valid(const Val& id) const;
  Val value_of(const Val& loc) const;
  Val tag_of(const Val& res, int32_t tag) const;
  Val id_of(const Val& res) const;
  Val owner_of(const Val& actor) const;
  std::vector<Val> instances_of(int32_t kind) const;
  Val select_state(int32_t kind, const std::vector<Val>& conds, const std::vector<int32_t>& ordinals, int32_t fallback) const;

 private:
  const phi::Profile& p_;
  const AbstractState& s_;
};

}  // namespace eval
}  // namespace kz

#include "kz/core/eval_evaluator.hpp"

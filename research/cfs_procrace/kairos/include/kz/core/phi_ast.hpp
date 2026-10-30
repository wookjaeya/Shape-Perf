// phi_ast.hpp -- the syntax tree of the profile language (Phi), version 1.
//
// A profile is plain text (.kzp) parsed by a hand-written parser (phi_parser.cpp) into the
// structures below.  Framework knowledge lives only in profiles; nothing in this header or in
// the code that uses it names a framework, a case or a bundle.  The grammar and the semantics
// are documented in docs/phi_reference.md.
//
// Layers:
//   parse   text -> Profile (names unresolved)         first syntax error stops the parse
//   check   Profile -> resolved Profile + diagnostics  all semantic errors are collected
//   print   Profile -> canonical text                  the profile hash is the hash of this text
//
// Terminology (T1): the judged object is a readiness specification (`rspec`), its Req is a set of
// readiness preconditions.  "contract" appears nowhere in identifiers.
#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace kz {
namespace phi {

constexpr int kLanguageVersion = 1;

struct SrcLoc {
  uint32_t file = 0;  // index into Profile::files (diagnostics only; never hashed)
  uint32_t line = 0;  // 1-based; 0 = unknown
  uint32_t col = 0;   // 1-based, counted in bytes
};

// ---- sorts -------------------------------------------------------------------------------

// Sorts of the core expression language (SPEC 2.3): Bool, Int, Id<K>, State<K>, resource
// references of a kind, tag values, actors, locations and strings.  `Gen` is an Int.
enum class SortKind : uint8_t { Unknown = 0, Bool, Int, Str, Actor, Loc, Id, State, Tag, Res };

struct Sort {
  SortKind kind = SortKind::Unknown;
  int32_t kind_index = -1;  // Id, State, Tag, Res: index into Profile::kinds
  int32_t tag_index = -1;   // Tag: index into KindDecl::tags
  bool operator==(const Sort& o) const { return kind == o.kind && kind_index == o.kind_index && tag_index == o.tag_index; }
  bool operator!=(const Sort& o) const { return !(*this == o); }
};

// A sort as spelled in the source: `Int`, `Id<es.app>`, `Res<es.app>`.  `ref` is the kind name
// of Id<...> or Res<...>.
struct SortSpec {
  SortKind kind = SortKind::Unknown;
  std::string ref;
  Sort resolved;
  SrcLoc loc;
};

// ---- expressions -------------------------------------------------------------------------

enum class ExprKind : uint8_t { BoolLit, IntLit, StrLit, Name, Not, And, Or, Imp, Cmp, In, Call, Quant };
enum class CmpOp : uint8_t { Eq, Ne, Lt, Le, Gt, Ge };
// Built-in functions.  `Res` is a resource reference `kind(key, ...)`; its name is Expr::text.
enum class CallFn : uint8_t { State, Key, Gen, Valid, Value, Tag, Id, Owner, Res };
enum class QuantKind : uint8_t { Forall, Exists };
// What a bare or dotted name denotes (set by the checker).
enum class NameRef : uint8_t { Unresolved = 0, BoundVar, Arg, Exec, Result, Const, StateConst, TagConst, LocRef };

struct Expr;
using ExprPtr = std::unique_ptr<Expr>;

struct Expr {
  ExprKind kind = ExprKind::BoolLit;
  SrcLoc loc;
  bool bval = false;
  int64_t ival = 0;
  std::string text;       // StrLit: the value; Name: the (dotted) path; Call: function or kind name;
                          // Quant: the bound variable; Call(Tag): the tag name is args[1]->text
  std::string kind_name;  // Quant: the name of the quantified kind
  CmpOp cmp = CmpOp::Eq;
  CallFn fn = CallFn::State;
  QuantKind quant = QuantKind::Forall;
  // Children.  Not: {x}.  And/Or: n >= 2 operands, flattened.  Imp: {a, b}.  Cmp: {a, b}.
  // In: {x, e1, ..., en}.  Call: the arguments.  Quant: {guard or null, body}.
  std::vector<ExprPtr> args;

  // Filled by the checker.
  Sort sort;
  NameRef ref = NameRef::Unresolved;
  int32_t ref_index = -1;  // BoundVar: binder depth; Arg: parameter index; Const: const index;
                           // StateConst/TagConst: ordinal; Call(Res) and Quant: kind index
  int32_t ref_aux = -1;    // Call(Tag): tag index; Call(Value): location index
};

ExprPtr clone_expr(const Expr& e);
// The first resource term R of `state(R)` in `e` (depth first) whose kind is `kind_index`, or null (a resolved expression).
const Expr* find_state_resource(const Expr& e, int32_t kind_index);
ExprPtr make_bool(bool v);
ExprPtr make_int(int64_t v);
ExprPtr make_name(std::string path);
ExprPtr make_not(ExprPtr x);
ExprPtr make_binary_list(ExprKind kind, std::vector<ExprPtr> operands);  // And / Or, flattened
ExprPtr make_cmp(CmpOp op, ExprPtr a, ExprPtr b);
ExprPtr make_call(CallFn fn, std::string name, std::vector<ExprPtr> args);

// ---- patterns (SPEC 2.3 AST-pattern language) ----------------------------------------------

enum class PatOp : uint8_t { Call, Store, Memset, Load, LoopPoll, Return };
enum class MacroFlag : uint8_t { Any, Yes, No };
enum class ArgCmp : uint8_t { Eq, Ne, AddrOf, Any };

struct ArgPred {
  uint32_t index = 0;
  ArgCmp cmp = ArgCmp::Any;
  bool is_name = false;      // Eq/Ne: the constant is a named constant, otherwise a literal
  int64_t ival = 0;
  std::string name;          // is_name: the constant; AddrOf: "Type.field"
};

struct Pattern {
  bool present = false;
  SrcLoc loc;
  std::string fn;  // the function whose body is searched
  PatOp op = PatOp::Call;
  std::string callee;                 // Call
  std::vector<ArgPred> args;          // Call
  std::string type_name, field;       // Store, Load: Type.field; Memset: Type
  uint32_t nth = 0;                   // 0 = the pattern must match exactly once; k = the k-th match in source order
  MacroFlag macro = MacroFlag::Any;
};

// ---- provenance, guards -------------------------------------------------------------------

// A cited text.  Tier and kind use the vocabulary of the schema (KZ_TIER, KZ_PROV_KIND,
// KZ_PROV_SIDE).  `file` is relative to the root of the pinned bundle; `sha256` is the SHA-256
// of the cited lines, each followed by LF (see phi_provenance.hpp).
struct ProvDecl {
  std::string name;
  SrcLoc loc;
  std::string tier, kind, side;  // display names; side empty = n/a
  std::string file;
  uint32_t line_from = 0, line_to = 0;
  std::string sha256;
  std::string cite;
  std::string authored;
  std::string note;
};

struct Guard {
  SrcLoc loc;
  std::string component;  // a declared source
  CmpOp op = CmpOp::Ge;
  std::string version;
};

// Common header of the declarations that make a claim.
struct EntryBase {
  std::string name;
  SrcLoc loc;
  std::vector<std::string> prov;  // names of ProvDecl; canonical form sorts and removes duplicates
  std::vector<int32_t> prov_idx;  // resolved
  std::vector<Guard> guards;
  std::string note;
};

// ---- declarations --------------------------------------------------------------------------

struct SourceDecl : EntryBase {
  std::string root;  // path of the source tree relative to the bundle root ("" = the root itself)
  std::string rev;
};

struct ConstDecl : EntryBase {
  enum class Value : uint8_t { Int, Str, Bool, RefEval } value = Value::Int;
  int64_t ival = 0;
  std::string sval;
  bool bval = false;
  std::string ref_symbol, ref_file;  // RefEval: constant-evaluated by the extractor in its translation unit
};

struct IdSpace {
  bool present = false;
  SrcLoc loc;
  std::string undefined_c, reserved_c, base_c, serial_max_c;  // names of integer constants
  int64_t undefined = 0, reserved = 0, base = 0, serial_max = 0;  // resolved
};

struct TagDecl {
  std::string name;
  std::vector<std::string> values;
  SrcLoc loc;
};

struct TransitionDecl {
  std::string from, to, action;
  SrcLoc loc;
};

struct DerivedDecl {
  std::string state;
  ExprPtr when;
  SrcLoc loc;
};

struct KindParam {
  std::string name;
  SortSpec sort;
};

struct KindDecl : EntryBase {
  std::vector<KindParam> params;
  std::vector<std::string> states;  // ordered: the declaration order is the order of the kind
  std::string initial;
  std::string scope;  // "process" (default) or "persistent"
  std::string table;  // name of the constant that gives the number of instances; empty = open
  IdSpace ids;
  std::vector<TagDecl> tags;
  std::vector<TransitionDecl> transitions;
  std::vector<DerivedDecl> derived;
  int32_t initial_ordinal = 0;  // resolved
};

struct LocDecl : EntryBase {
  std::string struct_name, field;
  SortSpec holds;
};

struct BindDecl : EntryBase {  // name = "Struct.field"
  std::string struct_name, field;
  ExprPtr target;  // state(kind(v)) | tag(kind(v), T) | id(kind(v)) | value(loc)
  // Key separation of a field the records of several kinds share (`via "F", ...`; 2026-10-24): a write of the
  // field changes the bound component only inside an execution of one of these functions (each the bound symbol of
  // an operation of the profile, whose effects stand for the writes it makes); every other execution of a writer of
  // the field addresses records of another key (SPEC 4.2 step 8, NO_KEY).  The claim is the profile's, with its
  // citations; the static-closure adapter reads it (kz/trace/static_closure.hpp).  Empty: no separation declared.
  std::vector<std::string> via;
};

struct SiteDecl : EntryBase {
  Pattern pattern;
};

// The role of a call context of an operation (SPEC rev 1.3 amendment 2026-10-24, 1.6 "Call
// contexts"): the purpose of the calls made at a declared site, justified by the cited sources.  A
// readiness specification of the operation applies to a call context through its role: a call at a
// site whose role is `existence_probe` (a lookup whose not-found outcome is the condition for the
// caller's own creation of the resource) carries no readiness precondition and is not an instance;
// its events are kept as facts.  `dependent_use` names a context that does carry it.  A call at no
// declared role's site stays an instance (an exclusion needs a stated, source-justified role).
struct RoleDecl : EntryBase {
  std::string op;       // the operation whose calls the role describes
  std::string site;     // a declared site (its pattern binds the role to the call site)
  std::string purpose;  // dependent_use | existence_probe
  int32_t op_index = -1, site_index = -1;
};

struct OpParam {
  std::string role;  // "", out, stamp, key, id, buffer, msg
  std::string name;
  bool has_sort = false;
  SortSpec sort;
};

enum class LpKind : uint8_t { None, Capture, At, Site };
struct LpSpec {
  LpKind kind = LpKind::None;
  Pattern pattern;   // At
  std::string site;  // Site
  SrcLoc loc;
  // Values that the observation point decodes besides the parameters of the operation; the
  // effect or conjunct names them `lp.NAME`.  This is the agreement between a profile and a
  // recorder about what an event at this point carries.
  std::vector<OpParam> values;
};

struct ConjunctDecl {
  std::string label;
  SrcLoc loc;
  ExprPtr expr;
  LpSpec lp;
  // Optional: the cited texts that stand behind this conjunct (its rule, its oracle).  A conjunct
  // without its own records is covered by those of its operation.
  std::vector<std::string> prov;
  std::vector<int32_t> prov_idx;
};

enum class EffectKind : uint8_t { Set, Raise, Use, StoreOut, StoreLoc, SetTag, Mark, Sync, Env };
struct EffectDecl {
  std::string label;
  SrcLoc loc;
  EffectKind kind = EffectKind::Use;
  ExprPtr res;            // Set, Raise, Use, SetTag: the resource (Res sorted)
  std::string all_kind;   // Set (instead of `res`): every instance of this finite kind (a memset of a table)
  std::string to_state;   // Set
  ExprPtr with_id;        // Set (optional): the identifier the resource carries from now on
  ExprPtr value;          // Raise: the level; StoreOut, StoreLoc: the stored value
  std::string target;     // StoreOut: out parameter; StoreLoc: location; Mark: handler_begin|handler_end;
                          // Sync: role; Env: input; SetTag: tag name
  std::string tag_value;  // SetTag
  std::string sync_object;  // Sync
  // The guard of the effect.  `when` (filter false): the code reaches the linearization point only
  // when the guard holds, so a point event whose guard is false in the replay is a finding (a
  // refutation for a declared_trusted effect, a trace/profile disagreement otherwise).  `filter`
  // (filter true): the point is shared by branches and the effect applies only where the guard
  // holds; a false guard is not a finding.  A declared_trusted effect cannot filter.
  ExprPtr when;
  bool filter = false;
  LpSpec lp;
  std::string status;  // "" | declared_trusted | declared_verified
  std::vector<std::string> refutable_by;
  std::vector<std::string> prov;
  std::vector<int32_t> prov_idx;
  std::vector<Guard> guards;
};

enum class OutcomeClass : uint8_t { Ready, NotReady, Error };

// The applicability conditions of an outcome mapping that a real-record replay may use to fix a condition the
// record leaves open (P1, conditionally adopted 2026-10-25: PI_DECISIONS_RESOLVED_20261025.md, review section 5.2).
// The mapping fixes a conjunct only where every condition holds in the record; otherwise the code fixes nothing.
//   narrow {
//     api "SYMBOL";                         the API whose code the implication was read for (= the bound symbol)
//     source COMPONENT rev "SHA", ...;      the revisions of that source at which it was read
//     branch "TEXT";                        the code path that returns this code (with the citations below)
//     context any | context ROLE, ...;      the call contexts (roles of the operation) in which it applies
//     captured PARAM [text [length < CONST]];  an argument the record must carry (a text: the string the call
//                                           received, shorter than CONST); one clause per argument
//     requires EXPR;                        optional: a condition on the arguments, true in every explanation
//     holds_at return;                      the implied state is that of this call's conjunct point, before
//                                           its return; it is never carried past the return
//     prov NAME, ...;                       the code or API texts the implication rests on
//     note "TEXT";
//   }
struct NarrowCaptured {
  std::string param;         // a parameter of the operation that is not `out`
  bool text = false;         // the record carries it as a text
  std::string length_below;  // text: its length is below this integer constant (or `ref eval` constant)
  int32_t length_const = -1;  // resolved
  SrcLoc loc;
};

struct NarrowDecl {
  bool present = false;
  SrcLoc loc;
  std::string api;
  std::string source;
  std::vector<std::string> revs;
  int32_t source_index = -1;  // resolved
  std::string branch;
  bool context_any = false;
  std::vector<std::string> contexts;  // role names
  std::vector<NarrowCaptured> captured;
  ExprPtr requires_;
  std::string holds_at;  // "return"
  std::vector<std::string> prov;
  std::vector<int32_t> prov_idx;
  std::string note;
};

struct OutcomeDecl {
  std::string code;  // a constant
  SrcLoc loc;
  OutcomeClass cls = OutcomeClass::Ready;
  std::string conjunct;    // NotReady: the label of the conjunct whose failure the code reports
  std::string documented;  // the documented result (supplier side), free text
  std::vector<std::string> doc_prov;
  std::vector<int32_t> doc_prov_idx;
  int32_t code_const = -1;  // resolved
  int32_t conjunct_index = -1;
  NarrowDecl narrow;  // absent: the code is checked against the replay but fixes nothing (P1 conditional)
};

struct TerminalDecl {
  bool present = false;
  bool process_terminal = false;
  Pattern after;  // optional: the terminal step happens after this pattern
  SrcLoc loc;
};

struct OpDecl : EntryBase {
  std::string symbol;
  std::vector<OpParam> params;
  std::vector<ConjunctDecl> conjuncts;
  std::vector<EffectDecl> effects;
  std::vector<OutcomeDecl> outcomes;
  TerminalDecl terminal;
};

struct InitDecl : EntryBase {
  bool is_loc = true;
  std::string loc_name;  // is_loc
  ExprPtr res;           // !is_loc: the resource
  ExprPtr value;         // is_loc: the value of the location at program entry
  std::string state;     // !is_loc: its state at program entry
};

// ---- happens-before rules -----------------------------------------------------------------

enum class Stratum : uint8_t { S, O, C };
enum class Robust : uint8_t { Unconditional, ObsFatal, ObsSoft, ScheduleChosen, Assumed };
enum class RuleTemplate : uint8_t { ProgramOrder, Spawn, Phase, Observe, ReadsFrom };

struct HbRuleDecl : EntryBase {
  std::string title;
  Stratum stratum = Stratum::S;
  Robust robust = Robust::Unconditional;
  RuleTemplate tmpl = RuleTemplate::ProgramOrder;
  // ProgramOrder
  bool looper = false;  // no edge between two handlers of one actor
  // Spawn: `create` op and the out-parameter that carries the child id, `entry` = op/site of the child's first event
  std::string create_op, create_out, entry_op;
  // Phase: events of `from_op` (phase `from_phase`) precede the first event of every actor spawned later by the same actor
  std::string from_op, from_phase;
  // Observe: the poll
  std::string poll_fn;
  Pattern called_at;
  std::string exit_outcome;   // constant: the successful exit of the poll
  bool predicate_from_code = true;
  ExprPtr predicate;          // optional explicit predicate (tests, tools)
  std::string else_terminal;  // a function whose call ends the other branch
  std::vector<std::string> watch_kinds;  // the kinds whose lifecycle effects are the writers
  // ReadsFrom (SPEC 3.16 row RF, 3.17 "Outcome-implied reads-from"; 2026-10-24): a use of `use_op` whose result is
  // `use_outcome` read, at the linearization point of its conjunct `use_conjunct`, the publication made by the effect
  // `pub_effect` of `pub_op` on the same resource and generation; with `match`, also the value of the out
  // parameter `match` of both operations is the same (the record carries both).
  std::string use_op, use_conjunct, use_outcome, pub_op, pub_effect, match;
  // ReadsFrom, optional (2026-10-26): `stored_out NAME prov ...;` -- the cited code (at least one code-derived record)
  // writes the use's out parameter NAME (= `match`) only on the path on which the use returns `use_outcome`.  Where
  // the record carries that out value as the operand of the store itself, captured inside the use's call (a direct
  // capture; TraceEvent texts "store.out.NAME"), the store stands for the outcome: the use qualifies without its
  // returned code.  A use with that store and another returned code contradicts the cited code (no edge, a finding).
  std::string stored_out;
  SrcLoc stored_out_loc;
  std::vector<std::string> stored_out_prov;
  std::vector<int32_t> stored_out_prov_idx;
};

// ---- readiness specifications -------------------------------------------------------------

enum class AllowedKind : uint8_t {
  None, Unspecified,  // the two whole-clause forms
  DocumentedDrop, ReturnedAndTested, RetryUntilReady, Fallback, DeferByDesign, Exit
};

struct AllowedEntry {
  AllowedKind kind = AllowedKind::DocumentedDrop;
  SrcLoc loc;
  std::string src;                  // documented_drop, defer_by_design, exit: citation text
  std::vector<std::string> codes;   // returned_and_tested, retry_until_ready: outcome constants
  std::string continuation;         // returned_and_tested: the normal continuation, free text
  std::string bound;                // retry_until_ready: constant
  std::string op;                   // fallback
  std::string exit_code;            // exit
  bool conflicts_appreq = false;    // the author recorded a conflict with appreq
  bool conflicts_caller = false;    // the author recorded a conflict with another caller entry (QT-5)
  std::vector<std::string> prov;
  std::vector<int32_t> prov_idx;
};

struct RspecDecl : EntryBase {
  std::string use_op;
  ExprPtr key;       // the resource selector (sel): a Res-sorted expression
  ExprPtr requires_; // the readiness preconditions, a conjunction
  std::vector<std::string> oracle_fns;
  std::string appreq;  // name of a ProvDecl (side caller), or empty = unspecified_by_reference
  bool appreq_unspecified = true;
  int32_t appreq_idx = -1;
  AllowedEntry allowed_head;                // None or Unspecified when no entries
  std::vector<AllowedEntry> allowed;        // entries (non-empty only for the block form)
  std::string harm;
  std::string authored;
  int32_t use_op_index = -1;
};

// ---- trusted entries ------------------------------------------------------------------------

struct TrustedDecl : EntryBase {
  std::string cls;  // declared_trusted | assumption
  std::string statement;
  std::vector<std::string> refutable_by;  // obligations O1..O10
};

// ---- profile-wide declarations ---------------------------------------------------------------

struct ContextDecl {
  bool present = false;
  SrcLoc loc;
  std::string owner_kind;
  int32_t owner_kind_index = -1;
  std::vector<std::string> prov;
  std::vector<int32_t> prov_idx;
  // `initial_thread not_app prov ...;` (P10, 2026-10-25): the profile's sources show that the initial thread of the
  // process (the one that runs main) is never the task of an application (SPEC 1.2 T_raw: a thread without a task
  // record of the framework).  A record adapter then states it for the actor whose thread id is the process id;
  // the execution relation names it "not an application" instead of "owner undetermined".
  bool initial_thread_not_app = false;
  SrcLoc not_app_loc;
  std::vector<std::string> not_app_prov;
  std::vector<int32_t> not_app_prov_idx;
};

struct ScopeDecl {
  bool present = false;
  SrcLoc loc;
  std::string kind, request, teardown;  // the shutdown kind and its two states
  int32_t kind_index = -1;
  std::vector<std::string> prov;
  std::vector<int32_t> prov_idx;
};

struct Profile {
  std::vector<std::string> files;  // file names, for diagnostics only
  std::string name;
  std::string revision;
  bool has_header = false;
  std::vector<ProvDecl> provs;
  std::vector<SourceDecl> sources;
  std::vector<ConstDecl> consts;
  std::vector<KindDecl> kinds;
  std::vector<LocDecl> locs;
  std::vector<BindDecl> binds;
  std::vector<SiteDecl> sites;
  std::vector<RoleDecl> roles;
  std::vector<OpDecl> ops;
  std::vector<InitDecl> inits;
  std::vector<HbRuleDecl> rules;
  std::vector<RspecDecl> rspecs;
  std::vector<TrustedDecl> trusted;
  ContextDecl context;
  ScopeDecl scope;
  bool checked = false;  // true after a successful check(): names are resolved

  Profile() = default;
  Profile(Profile&&) = default;
  Profile& operator=(Profile&&) = default;
  Profile(const Profile&) = delete;
  Profile& operator=(const Profile&) = delete;

  // Lookups (valid after normalize()); -1 if absent.
  int32_t find_kind(const std::string& n) const;
  int32_t find_const(const std::string& n) const;
  int32_t find_op(const std::string& n) const;
  int32_t find_loc(const std::string& n) const;
  int32_t find_site(const std::string& n) const;
  int32_t find_prov(const std::string& n) const;
  int32_t find_source(const std::string& n) const;
  int32_t find_rspec(const std::string& n) const;
  int32_t find_rule(const std::string& n) const;
  int32_t find_role(const std::string& n) const;
};

// Names of the closed vocabularies of the language (single source for the parser, the printer
// and the checker).
const char* cmp_text(CmpOp op);
const char* stratum_text(Stratum s);
const char* robust_text(Robust r);
const char* template_text(RuleTemplate t);
const char* allowed_text(AllowedKind k);
const char* effect_text(EffectKind k);
const char* outcome_class_text(OutcomeClass c);

}  // namespace phi
}  // namespace kz

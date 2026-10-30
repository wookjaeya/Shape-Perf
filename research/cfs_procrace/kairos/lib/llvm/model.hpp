// model.hpp -- the in-memory form of the extracted facts (SPEC 4.1, 4.2), before serialization.
//
// The extractor (extract_ast.cpp) fills a Model while it reads translation units one after the
// other; finalize() (finalize.cpp) resolves the references that need the whole program (callees,
// indirect calls, out-pointer summaries, static images, writer statuses); serialize.cpp writes
// the result as a KZ container.  Nothing in this file depends on LLVM or Clang, so the
// serialization, the text view and the tests of both can run without them.
//
// Determinism.  Facts are keyed by their position in the source ("file:line:col") and, where
// two facts share a position (macro expansions), by an ordinal that counts them inside their
// function.  A function body is read once, in the first translation unit that defines it (units
// are read in sorted order), so a function defined in a header gives one set of facts.  Before
// serialization every table is sorted by its key and indices are assigned, so the output does
// not depend on the order in which units were parsed or on any pointer value.
#pragma once

#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "kz/schema/kz_schema.h"

namespace kzs {

constexpr uint32_t kNone = 0xFFFFFFFFu;

// A source position.  `file` is the path as it will be recorded (relative to the bundle root
// when the file lies under it).
struct Pos {
  std::string file;
  uint32_t line = 0;
  uint32_t col = 0;
  bool operator<(const Pos& o) const {
    if (file != o.file) return file < o.file;
    if (line != o.line) return line < o.line;
    return col < o.col;
  }
  bool operator==(const Pos& o) const { return file == o.file && line == o.line && col == o.col; }
  std::string str() const { return file + ":" + std::to_string(line) + ":" + std::to_string(col); }
};

// ---- expression trees ------------------------------------------------------------------------

struct ExprNode {
  uint16_t op = KZ_EXPR_OP_NONE;
  uint16_t ref = KZ_EXPR_REF_NONE;  // LOAD: what it reads
  int64_t value = 0;
  std::string name;
  const ExprNode* a = nullptr;
  const ExprNode* b = nullptr;
};
using ExprRef = const ExprNode*;

// Hash-consing pool: structurally equal trees are one node.  Owns its nodes.
class ExprPool {
 public:
  ExprRef make(uint16_t op, uint16_t ref, int64_t value, const std::string& name, ExprRef a, ExprRef b);
  ExprRef constant(int64_t v) { return make(KZ_EXPR_OP_CONST, 0, v, "", nullptr, nullptr); }
  ExprRef load(uint16_t ref, const std::string& name, int64_t value = 0) { return make(KZ_EXPR_OP_LOAD, ref, value, name, nullptr, nullptr); }
  ExprRef top(const std::string& why) { return make(KZ_EXPR_OP_TOP, 0, 0, why, nullptr, nullptr); }
  // `not`, `and`, `or` and the comparisons fold the constants away, so that `while (1)` gives no
  // `(ne 1 0)` in an exit expression.  An operand of `and` / `or` is read as a condition.
  ExprRef unary(uint16_t op, ExprRef a);
  ExprRef binary(uint16_t op, ExprRef a, ExprRef b);
  ExprRef call_result(const std::string& callee, ExprRef args_seq) { return make(KZ_EXPR_OP_CALL_RESULT, 0, 0, callee, args_seq, nullptr); }
  // a = element, b = rest (a cons list); the list of `elems` in order.
  ExprRef list(uint16_t op, const std::vector<ExprRef>& elems);
  // Left-nested AND / OR of the operands (nullptr operands are skipped; an empty list gives nullptr).
  ExprRef conj(uint16_t op, const std::vector<ExprRef>& operands);
  size_t size() const { return nodes_.size(); }

 private:
  std::deque<ExprNode> nodes_;
  std::unordered_map<std::string, const ExprNode*> index_;
};

// Pre-order text of a tree, for logs and tests ("(and (eq (load field T.f) 0) ...)").
std::string expr_text(ExprRef e);

// ---- facts -----------------------------------------------------------------------------------

struct ArgFact {
  uint8_t kind = KZ_ARG_KIND_NONE;
  uint8_t role = KZ_ARG_ROLE_NONE;
  int64_t value = 0;
  std::string symbol, loc_type, loc_field;
  std::string fn_key;  // kind = FUNC
};

struct FuncFact {
  std::string key;   // unique: the name, or "name@file:line" for a function with internal linkage
  std::string name;
  Pos pos;           // the definition, or the first declaration
  uint32_t end_line = 0;
  uint32_t flags = 0;
  uint32_t nparams = 0;
  std::string tu;    // main file of the unit that defined it
};

struct CallFact {
  Pos pos;
  uint32_t ordinal = 0;  // among the facts at the same position, in the order of the walk
  std::string tu;
  std::string caller_key, caller_name;
  std::string callee_key, callee_name;  // empty for an indirect call
  uint8_t kind = KZ_CALL_KIND_NONE;
  uint32_t flags = 0;
  uint16_t loop_depth = 0;
  std::string loop_key;  // the innermost loop (LoopFact::key), or empty
  std::vector<ArgFact> args;
  std::string indirect_node;  // fp-flow node of the callee expression, for an indirect call
};

struct DerivesFact {
  std::string call_key;  // the call: pos.str()#ordinal
  uint32_t arg_index = 0;
  std::string loc_type, loc_field;
  ExprRef chain = nullptr;
  Pos capture;
  uint32_t hops = 0;
  std::string callee;
  uint8_t status = KZ_DERIVES_STATUS_NOT_DERIVED;
  uint8_t reason = KZ_DERIVES_REASON_NONE;
};

struct LoopFact {
  std::string key;  // pos.str()
  Pos pos;
  std::string tu;
  std::string func_key, func_name;
  std::string parent_key;  // the enclosing loop, or empty
  uint16_t depth = 0;
  uint8_t kind = KZ_LOOP_KIND_OTHER;
  uint8_t trip_status = KZ_TRIP_STATUS_UNKNOWN;
  int64_t trip = 0;
  uint32_t n_exits = 0, n_calls = 0;
  std::string delay_call_key;  // a call to a delay operation inside the loop
  std::string lock_call_key;
  uint8_t lock_kind = KZ_LOCK_KIND_NONE;
  ExprRef exit_expr = nullptr, modset = nullptr, budget = nullptr;
  std::string table;       // TABLE_WALK: the table as an access path ("Type.field" or a global name)
  std::string table_root;  // TABLE_WALK: the global object that holds it (resolved in finalize)
  std::string trip_note;   // TABLE_WALK that could not be resolved: why
  std::string image_assumed;  // TABLE_WALK unrolled through links that are not const: which (declared assumption)
  // SPEC 4.3, certificate view: what the reader found, and what finalize decides from it.
  bool delay_direct = false;   // the delay call is in this loop's own body, not in an inner loop
  ExprRef cond = nullptr;      // the condition exits (not the budget exits)
  ExprRef pred = nullptr;      // the predicate extracted from them
  uint8_t pred_form = KZ_PRED_FORM_NONE;
  std::string pred_note;       // why no predicate, or how it was rewritten
  uint8_t cert = KZ_LOOP_CERT_UNDECIDED;
  uint8_t lflags = 0;
  std::string cert_note;
  // F3 (LOOP_EXITS, spec.extended_facts): the exits of every loop, whatever its kind.
  struct Exit {
    uint8_t kind = KZ_LOOPEXIT_KIND_NONE;
    Pos pos;
    ExprRef when = nullptr;     // the guards from the loop down to the exit
    ExprRef trigger = nullptr;  // the condition that decides it, or null (unconditional)
    uint16_t flags = 0;
  };
  bool has_exits = false;  // filled (extended facts)
  bool exits_as_in_loops = false;  // exit_expr and modset above are the same trees
  ExprRef all_exit = nullptr, all_modset = nullptr;
  std::vector<Exit> exits;
  // F4 (PTR_FIELDS, spec.extended_facts): a counted loop of known trip count, its induction variable
  // (the local id of the expression trees) and the values it takes: c0 + k * step for k < trip.
  std::string induction;
  int64_t c0 = 0, step = 0;
};

// A call inside a table walk, in the form that finalize evaluates per iteration: its guards
// inside the loop and its arguments, with the fields of the walked element as ELEM loads.
struct WalkUse {
  std::string loop_key, call_key;
  ExprRef guard = nullptr;        // the guards from the loop body down to the call (element form)
  std::string callee_field;       // an indirect call through a field of the element: the field
  std::vector<std::pair<uint32_t, ExprRef>> args;  // (index, element-form expression) of the arguments that read the element
};

// A write of a field by a function's own body (FUNC_WRITES).
struct WriteEvent {
  std::string func_key;
  uint8_t kind = KZ_FWRITE_KIND_NONE;
  std::string type, field;  // field "*" for a whole record; both empty for an untyped write
  Pos pos;
  std::string loop_key;     // the innermost loop around the write, or empty
};

struct IterFact {
  std::string loop_key;
  uint32_t iter = 0;
  uint8_t kind = KZ_ITER_KIND_NONE;
  std::string call_key;   // CALL, ARG, CALLEE
  uint32_t arg_index = kNone;
  uint8_t status = KZ_ITER_STATUS_UNKNOWN;
  ExprRef guard = nullptr;
  uint8_t value_kind = KZ_IMAGE_KIND_NONE;
  int64_t value = 0;
  std::string text, fn_key;
  uint8_t spawn = KZ_ITER_SPAWN_NONE;
  Pos pos;  // where the value is written (the element's initializer)
};

struct SpawnParamFact {
  std::string func_key;
  uint32_t param = 0;
  std::string spawn_call_key;
  std::string entry_key;     // VIA_ENTRY
  std::string call_key;      // VIA_ENTRY: the indirect call
  uint8_t kind = KZ_SPAWN_PARAM_KIND_NONE;
  std::string via;
};

struct InitFact {
  std::string loc_type, loc_field, object;
  int64_t value = 0;
  uint8_t source = KZ_INIT_SOURCE_NONE;
  Pos pos;
  std::string tu;
};

struct ConstFact {
  std::string name;  // the source text of the expression
  Pos pos;
  std::string tu;
  int64_t value = 0;
  std::string text;
  uint8_t kind = KZ_CONST_KIND_NONE;
};

struct ATypeFact {
  uint8_t kind = KZ_ATYPE_KIND_NONE;
  std::string loc_type, loc_field;  // empty = decided later for every target of the type
  std::string type_name;            // the support type the hit is about (target owner type)
  Pos pos;
  uint32_t ordinal = 0;
  std::string func_key;
  std::string tu;
  std::string detail;
  std::string call_key;  // the call, if the hit is at one
  // The field of `type_name` whose own address or storage the hit is about (an address escape, a
  // store through a union member of that name); empty for a hit about the type as a whole (a cast,
  // a union that holds the type, an unknown effect).  Not written: the per-field completeness rule
  // of FIELD_STATUS (kz_facts.h revision 8) reads it from the raw hits.
  std::string cause_field;
};

struct WriterFact {
  std::string loc_type, loc_field;
  Pos pos;
  uint32_t ordinal = 0;
  std::string tu;
  std::string func_key, func_name;
  uint8_t kind = KZ_WRITER_KIND_NONE;
  uint8_t value_kind = KZ_ARG_KIND_NONE;
  int64_t value = 0;
  std::string value_text, key;
  ExprRef guard = nullptr;
  Pos via;  // out-pointer: the store inside the callee
  std::string via_func_key;
  uint32_t flags = 0;
  std::string call_key;  // out-pointer: the call that instantiates it
};

struct TerminalFact {
  std::string call_key;
  Pos pos;
  std::string tu;
  std::string func_key;
  std::string op;
  ExprRef guard = nullptr;
  uint32_t depth = 0;
  uint32_t flags = 0;
};

struct OutCallFact {
  std::string call_key;
  std::string func_key;  // the function that makes the call
  Pos pos;
  uint32_t ordinal = 0;
  std::string tu;
  uint32_t arg_index = 0;
  std::string loc_type, loc_field, callee_key, callee_name;
  uint32_t n_stores = 0;
  uint8_t summary = KZ_OUTPTR_SUMMARY_NONE;
};

struct TargetFact {
  std::string call_key;
  std::string fn_key;  // empty = unresolved
  uint8_t kind = KZ_CALL_TARGET_KIND_NONE;
  uint8_t reason = KZ_CALL_TARGET_REASON_RESOLVED;
  uint32_t n = 0;
  std::string via;
};

struct ImageFact {
  std::string object, path;
  uint8_t kind = KZ_IMAGE_KIND_NONE;
  int64_t value = 0;
  std::string text;
  std::string fn_key;
  Pos pos;
  std::string tu;
  // Filled with spec.extended_facts only (F6 GLOBAL_IMAGES; STATIC_IMAGE does not write them): the
  // record field the leaf is, and the macro its value is written with and where that text is spelled.
  std::string owner, field, macro;
  Pos spell;
};

struct LocFact {
  std::string loc_type, loc_field;
  uint32_t n_writers = 0, n_direct = 0, n_data_fn = 0, n_outptr = 0;
  uint32_t n_untyped = 0, n_escape = 0, n_cast = 0, n_union = 0, n_unknown = 0, n_init = 0, n_unresolved_indirect = 0;
  uint8_t status = KZ_LOC_STATUS_UNCHECKED;
};

// An out-pointer store inside a function: `*param = value` under a guard.
struct OutStore {
  Pos pos;
  uint32_t ordinal = 0;
  uint8_t value_kind = KZ_ARG_KIND_TOP;
  int64_t value = 0;
  std::string value_text;
  ExprRef guard = nullptr;
  std::string via_func_key;  // empty for a store in the function itself; else the nested callee
  Pos via_pos;               // nested: where the store is
};
struct ParamPass {
  std::string callee_key;  // empty = indirect or unknown callee
  uint32_t arg_index = 0;
  bool data_fn = false;    // a known data function writes through it (memset family)
  std::string callee_name;
  Pos pos;
};
struct ParamSummary {
  bool is_pointer = false;
  std::vector<OutStore> stores;
  std::vector<ParamPass> passes;
  bool escapes = false;
  std::string escape_detail;
};
struct FuncSummary {
  std::vector<ParamSummary> params;
};

// A function-valued flow edge (fp-flow graph).  Nodes are strings:
//   F:<function key>    a function constant
//   T.f                 a struct field of function-pointer type ("Type.field")
//   G:<name>            a global variable            L:<function key>/<name>  a local
//   P:<function key>#k  parameter k                  R:<function key>         the return value
//   ?:<why>             a source that cannot be followed
struct FlowEdge {
  std::string src, dst;
};
struct IndirectCall {
  std::string call_key;
  std::string node;
};

// ---- the profile-free mode (wrappers.hpp) -------------------------------------------------------------
// The control-flow graph of a function body as the wrapper derivation reads it: per block (Clang's
// block ids), the calls in it in evaluation order, its successors, and whether it ends in a call of a
// function declared not to return.
struct CfgBlock {
  std::vector<std::string> calls;  // call keys
  std::vector<uint32_t> succ;
  bool noreturn = false;
};
struct FuncCfg {
  uint32_t entry = 0, exit = 0;
  std::vector<CfgBlock> blocks;
};
// A function-pointer argument of a direct call (a parameter of function-pointer type).
struct FpArg {
  uint32_t index = 0;
  std::vector<std::string> nodes;  // fp-flow nodes of the argument
  bool from_data = false;          // an integer or a data pointer converted to a function pointer
};
// ---- the extended facts F1-F3 (spec.extended_facts; kz_facts.h revision 6) ------------------------------
// A read of a field or of a global (READS).
struct ReadFact {
  std::string func_key;
  Pos pos;
  uint32_t ordinal = 0;
  uint8_t kind = KZ_READ_KIND_NONE;
  uint8_t root_kind = KZ_READ_ROOT_NONE;
  uint16_t use = 0;
  uint32_t flags = 0;
  uint32_t root_param = kNone;
  std::string type, field, root, text;
  std::string loop_key;               // the innermost loop, or empty
  std::string call_key;               // call-argument: the innermost call
  uint32_t arg_index = kNone;
  ExprRef index = nullptr, guard = nullptr, ctx = nullptr;
  // F4 (PTR_FIELDS): the pointer field the access path dereferences nearest to its root, if it is one;
  // the subscript of that step (null for `->`), the rest of the path from the pointee to the location,
  // the number of pointer steps on the path, and the global at the root (its object key).
  std::string ptr_type, ptr_field, ptr_rest, root_object;
  ExprRef ptr_index = nullptr;
  std::string ptr_index_local;  // the subscript is this local (its tree id), for the counted-loop values
  int64_t ptr_index_const = 0;
  bool ptr_index_is_const = false;
  uint32_t ptr_steps = 0;
  std::string key() const { return func_key + "|" + pos.str() + "#" + std::to_string(ordinal); }
};
// One alternative of a return statement (RETURNS).
struct ReturnFact {
  std::string func_key;
  Pos pos;
  uint32_t ordinal = 0;
  uint32_t alt = 0, n_alts = 1;
  ExprRef value = nullptr, guard = nullptr, def_guard = nullptr, cond = nullptr;
  std::string call_key, loop_key;
  uint8_t vkind = KZ_RET_VALUE_NONE;
  uint16_t flags = 0;
  int64_t constant = 0;
  std::string local, text;
  Pos def;
};
// Per function and constant (RET_CONSTS); filled by finalize from the RETURNS rows.
struct RetConstFact {
  std::string func_key;
  int64_t value = 0;
  std::string text;
  ExprRef cond = nullptr;  // null: always
  uint32_t n_rows = 0;
  uint32_t flags = 0;
};

// ---- the extended facts F4 and F6 (spec.extended_facts; kz_facts.h revision 7) --------------------------------
// One argument of one call, traced to its source (ARGFLOW).
struct ArgFlowFact {
  std::string call_key, func_key;
  uint32_t arg_index = 0;
  uint8_t src = KZ_ARGSRC_NONE, base = KZ_ARGBASE_NONE, reason = KZ_DERIVES_REASON_NONE;
  uint32_t flags = 0;
  uint32_t param = kNone, hops = 0;
  int64_t value = 0;
  std::string type, field, root, object, text;
  ExprRef chain = nullptr, index = nullptr;
  ExprRef guard = nullptr;  // the conditions the call executes under (for the PARAM_FLOW rows of the call)
  std::string read_key;     // the READS row of the source read (ReadFact::key), or empty
};
// A use of a copy of a parameter in its function's body other than as a call argument (PARAM_FLOW,
// depth 0): a comparison, a return, a store, a subscript; and the stores through a pointer parameter.
struct ParamUse {
  std::string func_key;
  uint32_t param = 0;
  uint8_t kind = KZ_PFLOW_KIND_NONE;
  uint16_t flags = 0;
  Pos pos;
  uint32_t ordinal = 0;
  ExprRef expr = nullptr, guard = nullptr;
  std::string text;
};
// A row of PARAM_FLOW (filled by finalize_flow_facts).
struct ParamFlowFact {
  std::string func_key, call_key, next_key, sink_key, sink_call_key;
  uint32_t param = 0, next_param = kNone, depth = 0, n = 0;
  uint8_t kind = KZ_PFLOW_KIND_NONE;
  uint16_t flags = 0;
  ExprRef expr = nullptr, guard = nullptr;
  Pos pos;
  std::string text, chain;
};
// A store to a pointer field whose value is the address of a global object (PTR_FIELDS), with what the
// writer's control-flow graph says about its order: the calls and the reads of the writer that every
// path reaches after it, and the positions of the other writes of the field it overwrites.
struct PtrWriteFact {
  uint8_t kind = KZ_PTRF_KIND_ADDRESS_WRITE;
  std::string func_key;  // empty for a static initializer
  std::string type, field, root, object, text;
  bool root_global = false, in_loop = false;
  int64_t elem = 0;
  Pos pos;
  uint32_t ordinal = 0;
  ExprRef guard = nullptr;
  std::set<std::string> after_calls, after_reads;  // call keys, ReadFact keys
  std::set<std::string> overwritten;               // Pos::str() of earlier writes of the field in the writer
};
// A row of PTR_FIELDS (filled by finalize_flow_facts).
struct PtrFieldFact {
  uint8_t kind = KZ_PTRF_KIND_NONE, status = KZ_PTRF_STATUS_NONE;
  uint16_t flags = 0;
  std::string func_key, read_key, type, field, root, object, path;
  int64_t write = -1;  // index of the write row among the PTR_FIELDS rows
  uint32_t n = 0, idx_n = 0;
  int64_t elem = 0, idx_step = 0;
  ExprRef index = nullptr, guard = nullptr;
  Pos pos;
};
// A row of GLOBAL_IMAGES (filled by finalize_flow_facts).
struct GlobalImageFact {
  ImageFact leaf;
  uint8_t reach = 0;
  uint16_t flags = 0;
};

// A row of P0_TABLE: the table entry (index into p0_table()) and what this program has of it.
struct P0Fact {
  size_t entry = 0;
  std::string fn_key;  // empty when the program does not mention the name
  uint32_t n_calls = 0;
  uint16_t flags = 0;  // KZ_P0_FLAG
};
// A row of OP_DERIV (F8).
struct OpDerivFact {
  std::string func_key, call_key, next_key;  // next_key empty: the primitive itself is called
  std::string primitive, chain;
  uint8_t kind = KZ_OPDERIV_KIND_NONE;
  uint8_t mode = KZ_OPDERIV_MODE_NONE;
  uint16_t flags = 0;
  uint32_t depth = 0;
  ExprRef guard = nullptr;
};
// A row of SPAWN_WRAP (F7).
struct SpawnWrapFact {
  std::string func_key, call_key, next_key, entry_key, icall_key;
  std::string primitive, chain, via;
  uint32_t param = kNone, depth = 0;
  uint8_t kind = KZ_SPAWNW_KIND_NONE;
  uint16_t flags = 0;
};

// ---- F5 LOCKSET and the target-independent rules (spec.extended_facts; kz_facts.h revision 8) -------------
// The identity of a lock as an expression over the parameters of the function it is seen in: lvalues
// (a global object, a member, an element, a dereference) and values (a parameter, a constant, the address
// of an lvalue, the value read from an lvalue).  Two lock operations are the same lock only when their
// identities are the same closed expression and every value it reads is a static instance (lockset.hpp).
struct LockIdNode;
using LockId = std::shared_ptr<const LockIdNode>;
struct LockIdNode {
  enum Kind : uint8_t { UNKNOWN, PARAM, CONST, GLOBAL, FIELD, INDEX, DEREF, ADDR, LOAD };
  uint8_t k = UNKNOWN;
  int64_t v = 0;     // PARAM: the index; CONST: the value
  std::string name;  // GLOBAL: the object key; FIELD: "Type.field"; UNKNOWN: why
  LockId a, b;       // FIELD, DEREF, ADDR, LOAD: a; INDEX: a the base, b the subscript
  // For the overlap of two identities (lockset.cpp): GLOBAL and FIELD: the record type of the object the lvalue
  // designates (arrays stripped; "" when it is no record); FIELD: `owner` the record type that declares the field, and
  // `v` 1 when that record is a union (its members share their storage).  Not part of the text of an identity.
  std::string rtype, owner;
};
// The flag of a FIELD node whose owner is a union.
constexpr int64_t kLockFieldUnionMember = 1;
// The control-flow graph of a body as the lock-set analysis reads it: per block the events in evaluation
// order, the two successors of a two-way branch with its test, and the return statements.
struct LockEvent {
  enum Kind : uint8_t { CALL, READ, WRITE, DEF, RET };
  uint8_t kind = CALL;
  std::string key;    // CALL: call key; READ: ReadFact::key(); DEF: the local's tree id
  uint32_t index = 0; // WRITE: index in Model::write_sites
  // DEF and RET: the value: 0 unknown, 1 a constant, 2 the result of the call `call`, 3 a local `key`
  // (RET only), 4 none (RET of a void function)
  uint8_t vkind = 0;
  int64_t value = 0;
  std::string call;
};
struct LockTest {
  // the branch tests `subject` against `value`: 1 subject == value, 2 subject != value; a truth test
  // `if (x)` is x != 0.  The subject is a local (its tree id) or the result of a call of the block.
  uint8_t op = 0;  // 0: not a test the analysis reads
  bool on_call = false;
  std::string subject;
  int64_t value = 0;
};
struct LockBlock {
  std::vector<LockEvent> events;
  std::vector<uint32_t> succ;  // kNone for a successor that cannot be reached
  bool branch = false;         // two-way: succ[0] when the test holds, succ[1] when it does not
  bool noreturn = false;       // ends in a call of a function declared not to return
  LockTest test;
};
struct LockCfg {
  uint32_t entry = 0, exit = 0;
  std::vector<LockBlock> blocks;
};
// A typed write of a body (the FUNC_WRITES events, every site), for the WRITE rows of LOCKSET.
struct WriteSite {
  std::string func_key;
  uint8_t kind = KZ_FWRITE_KIND_NONE;
  std::string type, field;
  Pos pos;
};
// A row of LOCK_OPS: a function (or a P0 primitive) that is a lock operation.
struct LockOpFact {
  std::string func_key, call_key, next_key, chain, primitive;
  std::string ident;  // the identity of the lock, over the parameters of func_key (text)
  std::string may;    // the field-based location of the mutex object ("Type.field", "<global>.name"), or empty
  uint8_t kind = KZ_LOCKOP_KIND_NONE;
  uint8_t mode = KZ_LOCKOP_MODE_NONE;
  uint16_t flags = 0;
  uint32_t depth = 0;
  int64_t success = 0;  // RESULT_CONDITIONAL: the value returned when the lock is held
};
// A row of LOCK_IDS: one identity a LOCKSET row holds.
struct LockIdFact {
  std::string text, may, location, object, why;
  uint8_t kind = KZ_LOCKID_KIND_NONE;
  uint8_t status = KZ_LOCKID_STATUS_NONE;
  uint16_t flags = 0;
  uint32_t writers = 0;  // the writer instances of `location` (STATIC check)
};
// A row of LOCKSET: the locks held at one call, read or write.
struct LockSetFact {
  std::string func_key, ref;  // ref: call key, ReadFact::key() or the WriteSite index
  uint8_t kind = KZ_LOCKSET_KIND_NONE;
  uint16_t flags = 0;
  Pos pos;
  std::vector<uint32_t> held;     // LOCK_IDS indices, sorted
  std::vector<std::string> openers;  // call keys of the acquisitions that hold them (any path)
  uint32_t write = kNone;         // WRITE: Model::write_sites index
  ExprRef held_expr = nullptr, opener_expr = nullptr;  // SEQ of constants (filled by finalize_lock_facts)
};
// A row of FIELD_STATUS: the writer set of one field under the two target-independent completeness rules.
struct FieldStatusFact {
  std::string type, field;
  uint32_t n_writers = 0, n_direct = 0, n_data_fn = 0, n_outptr = 0, n_init = 0, n_instances = 0;
  uint32_t own_escape = 0, own_cast = 0, own_union = 0, own_unknown = 0;  // hits about the field or the type as a whole
  uint32_t sibling_hits = 0;   // hits about another field of the type
  uint32_t untyped = 0;        // untyped write sites in the program
  uint8_t status_field = KZ_LOC_STATUS_UNCHECKED, status_type = KZ_LOC_STATUS_UNCHECKED;
  uint8_t once = 0;            // KZ_FSTATUS_ONCE
  uint16_t flags = 0;
  std::string writer_fns;      // the functions of the writers (sorted, "; ")
};
// A row of AWAIT_L1: a poll loop under the L1 (i) rule whose relevant locations do not depend on the targets.
struct AwaitFact {
  std::string loop_key, func_key;
  std::vector<std::string> relevant;  // "Type.field" (or "<global>.name"), sorted
  std::vector<std::string> via_callee; // the relevant locations that come from a function the predicate calls
  uint8_t l1_read = 0;      // KZ_AWAIT_READ
  uint8_t l1_mod_field = 0, l1_mod_type = 0;  // KZ_AWAIT_MOD under the two completeness rules
  uint8_t cert_field = KZ_LOOP_CERT_UNDECIDED, cert_type = KZ_LOOP_CERT_UNDECIDED;
  uint16_t flags = 0;
  uint32_t lock = kNone;    // LOCK_IDS index of the lock that covers the test reads
  std::string note_field, note_type;
  ExprRef relevant_expr = nullptr;  // SEQ of loads (filled by finalize_lock_facts)
};
// A row of LOCK_PATHS: an F5 event (`row`, an index of Model::locksets) after a spawn call of its function on
// the control-flow graph (AFTER_SPAWN), or reached from its function's entry through a call that releases, may
// release or has a lock effect the analysis cannot follow (ENTRY_RELEASE).  `call_key` is that call.
struct LockPathFact {
  uint32_t row = kNone;
  std::string func_key, call_key;
  uint8_t kind = KZ_LOCKPATH_KIND_NONE;
};

struct Model {
  ExprPool exprs;

  std::map<std::string, FuncFact> funcs;  // by key
  std::vector<CallFact> calls;
  std::vector<DerivesFact> derives;
  std::vector<LoopFact> loops;
  std::vector<InitFact> inits;
  std::vector<ConstFact> consts;
  std::vector<ATypeFact> atypes;
  std::vector<WriterFact> writers;
  std::vector<TerminalFact> terminals;
  std::vector<OutCallFact> outcalls;
  std::vector<TargetFact> targets;
  std::vector<ImageFact> images;
  std::vector<LocFact> locs;
  std::vector<WalkUse> walk_uses;
  std::vector<WriteEvent> write_events;
  std::vector<IterFact> iters;              // filled by finalize
  std::vector<SpawnParamFact> spawn_params;  // filled by finalize

  std::map<std::string, FuncSummary> summaries;  // function key -> out-pointer summary
  std::vector<FlowEdge> flow;
  std::vector<IndirectCall> indirect_calls;
  std::vector<std::string> spawn_calls;           // call keys of spawn-operation calls
  std::map<std::string, std::vector<std::string>> spawn_entry_nodes;  // call key -> fp-flow nodes of the entry argument
  std::map<std::string, uint32_t> spawn_entry_arg;                     // call key -> index of the entry argument
  std::set<std::string> processed_functions;      // keys whose bodies were read
  std::vector<std::string> files;                 // every file a fact refers to (sorted by serialize)
  std::vector<std::string> main_files;            // the translation units read, in order

  // The profile-free mode: what its first pass reads (spec.collect_wrapper_inputs), the guards of
  // the calls its rows name (spec.guard_calls), and the derived rows it writes.
  std::map<std::string, FuncCfg> cfgs;                    // function key -> control-flow graph
  std::map<std::string, std::vector<FpArg>> call_fp_args;  // direct call key -> function-pointer arguments
  std::map<std::string, ExprRef> call_guards;              // call key -> the conditions it executes under
  std::vector<P0Fact> p0_facts;
  std::vector<OpDerivFact> op_derivs;
  std::vector<SpawnWrapFact> spawn_wraps;

  // The extended facts F1-F3 (spec.extended_facts); LOOP_EXITS is in LoopFact.
  std::vector<ReadFact> reads;
  std::vector<ReturnFact> returns;
  std::vector<RetConstFact> ret_consts;  // filled by finalize

  // The extended facts F4 and F6 (spec.extended_facts): what the walk collects, and the rows that
  // finalize_flow_facts composes from it.
  std::vector<ArgFlowFact> argflows;
  std::vector<ParamUse> param_uses;
  std::vector<PtrWriteFact> ptr_writes;
  std::map<std::string, ImageFact> object_scalar_images;  // object key -> the initializer of a scalar global
  std::vector<ParamFlowFact> param_flows;
  std::vector<PtrFieldFact> ptr_fields;
  std::vector<GlobalImageFact> global_images;
  struct FlowStats {
    uint32_t nodes = 0, sccs = 0, sccs_recursive = 0;
    uint32_t direct = 0, composed = 0, cut_depth = 0, cut_size = 0;  // rows; cut: OPEN rows
    uint64_t left_out = 0;                                            // uses the bounds left out
    uint32_t objects_reached = 0, objects_outside = 0;                // GLOBAL_IMAGES: in / outside the closed program
  } flow_stats;

  // F5 and the target-independent rules (spec.extended_facts): what the walk collects (the lock-set
  // skeleton of every body, the lock identity of every call argument, every typed write site, the fields
  // of the record types the reads name), the raw A-TYPE hits of a completeness pass
  // (spec.keep_raw_atypes), and the rows lockset.cpp writes.
  std::map<std::string, LockCfg> lock_cfgs;                        // function key -> skeleton
  std::map<std::string, std::vector<LockId>> call_arg_idents;      // call key -> identity per argument
  std::vector<WriteSite> write_sites;
  std::map<std::string, std::vector<std::string>> record_fields;   // owner type -> its fields, in order
  // "Type.field" of the fields of those record types whose type is an array of scalars that are not const: a store to
  // one element (x.f[i] = v) makes no writer row in these facts (FIELD_STATUS does not call such a field complete)
  std::set<std::string> array_fields;
  std::vector<ATypeFact> atypes_raw;                               // before the untyped expansion
  std::vector<LockOpFact> lock_ops;
  std::vector<LockIdFact> lock_ids;
  std::vector<LockSetFact> locksets;
  std::vector<FieldStatusFact> field_status;
  std::vector<AwaitFact> awaits;
  std::vector<LockPathFact> lock_paths;
  struct LockStats {
    uint32_t bodies = 0, cfg_built = 0, sccs = 0, sccs_recursive = 0;
    uint32_t acquire_ops = 0, release_ops = 0, try_ops = 0, wait_ops = 0;
    uint32_t rows_held = 0, rows_failure_path = 0, kills_unknown = 0;
    uint32_t spawn_calls = 0, after_spawn = 0, entry_release = 0;
    uint32_t fields = 0, completeness_targets = 0;
  } lock_stats;

  // Raw tables for the global phase.
  std::map<std::string, std::vector<ImageFact>> object_images;  // object key -> flattened initializer
  std::map<std::string, Pos> object_pos;
  std::map<std::string, std::string> object_tu;
  // Table walks to evaluate: loop key -> access path of the table in the loop.
  struct WalkSpec {
    std::string loop_key;
    std::string root_object;           // the global object the path starts at
    std::vector<std::string> fields;   // field names from the root to the array
    std::string text;
    // For the check that the image is the value at run time (SPEC 4.3 unrolling): per field, the
    // record type that declares it and whether the field itself is const; whether the root
    // object is const; the record type of the elements.
    std::vector<std::string> field_owner;
    std::vector<bool> field_const;
    bool root_const = false;
    std::string elem_type;
  };
  std::vector<WalkSpec> walks;
  // Untyped write sites (all of them, before the exposure filter).
  struct UntypedWrite {
    Pos pos;
    uint32_t ordinal = 0;
    std::string func_key, tu, call_key, detail;
  };
  std::vector<UntypedWrite> untyped_writes;
  // A pointer that reaches the owner type of a target (directly, inside records, or through
  // pointer fields) passed as a call argument.  finalize() decides whether the callee is code
  // whose effect is unknown (no body, unresolved indirect call, variadic position): then every
  // such target gets an A-TYPE unknown-effect hit.
  struct ReachPass {
    std::string call_key, func_key, tu;
    Pos pos;
    uint32_t ordinal = 0;
    uint32_t arg_index = 0;
    std::string callee_key, callee_name;  // empty for an indirect call
    bool variadic_position = false;
    std::vector<std::string> types;  // the owner types of the targets the pointer reaches (sorted)
    std::string text;                // the argument as written
  };
  std::vector<ReachPass> reach_passes;
  // Whether each global object is const (from its definition), for the table-walk check.
  std::map<std::string, bool> object_const;
  // Declared assumptions the extraction made (BASIS items with policy declared, provenance tier
  // researcher), counted in the report.
  struct Assumption {
    std::string name, value;
  };
  std::vector<Assumption> assumptions;
  // Calls whose callee expression is a known data function (memset family) with a typed writer:
  // already turned into writers; counted here for the report.
  uint32_t data_fn_calls = 0;

  struct Stats {
    uint32_t tus = 0, functions_read = 0, functions_skipped_system = 0, functions_dedup = 0;
    uint32_t calls = 0, parse_errors = 0;
    // the walks that stopped at a fixed bound (reported with the extended facts): the FACT_INIT walk of one object's
    // initializer (kInitBudget nodes; the objects, and those with no leaf read when it stopped), and the nesting of a
    // static initializer read for stored function values (kFpInitDepth levels)
    uint32_t init_budget_out = 0, init_budget_out_no_leaf = 0, fp_init_depth_cut = 0;
    // the expression trees read from the source (FACT_EXPRS): subexpressions deeper than kExprDepth levels, each a
    // node top "too deep" (every mode; reported with the extended facts)
    uint32_t expr_depth_cut = 0;
  } stats;

  // Filled by finalize(): indices and the numbers of the report.
  struct Resolved {
    std::vector<std::string> func_keys;             // sorted; the position is the FUNCS index
    std::map<std::string, uint32_t> fn_index;
    std::map<std::string, uint32_t> call_site, loop_site;  // by call key and loop key
    std::map<std::string, size_t> call_index, loop_index;  // position in the sorted vectors
    std::vector<uint32_t> writer_site, atype_site;         // parallel to writers and atypes
    uint32_t n_sites = 0;
    uint32_t key_collisions = 0;
    uint32_t indirect_total = 0, indirect_resolved = 0, indirect_unresolved = 0;
    uint32_t spawn_total = 0, spawn_resolved = 0;
    uint32_t walks_resolved = 0, walks_failed = 0, walks_assumed = 0;
    uint32_t outptr_store_only = 0, outptr_escapes = 0, outptr_unsummarized = 0, outptr_data_fn = 0;
    uint32_t reach_passes_unknown = 0;  // pointer arguments that reach a target and go to code of unknown effect
    uint32_t atype_untyped_sites_total = 0, atype_untyped_sites_relevant = 0;
    uint32_t flow_nodes = 0, flow_edges = 0;
    uint32_t loops_unroll = 0, loops_await = 0, loops_star = 0;
  } resolved;
};

// Resolves everything that needs the whole program.  `targets` are (type, field) pairs.
struct FieldRef {
  std::string type, field;
  bool operator<(const FieldRef& o) const { return type != o.type ? type < o.type : field < o.field; }
  bool operator==(const FieldRef& o) const { return type == o.type && field == o.field; }
};

}  // namespace kzs

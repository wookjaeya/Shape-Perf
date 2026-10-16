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

// kzgcc.cpp -- compiler binding candidate "GCC GIMPLE plugin" of the Kairos instrumentation family
// (stage C fitness prototype; kairos/docs/instr_candidate_gcc.md).
//
// The plugin reads a logical site manifest ("kzs-sites 1", kairos/docs/event_semantics.md section 3,
// parsed by the runtime's own host parser) and, in the translation units the manifest names, inserts
// calls of the runtime's observation hooks (kz_rt_obs, kz_rt_obs_str; hook ABI 1, kz_rt.h) and of the
// control hook kz_rt_ctl at
//   - a call of <callee> inside <func>        (phase=before: the call's argument operands; phase=after:
//                                              the call's result and out<k> = *arg<k> read right after it)
//   - the entry of <func> (phase=before, no callee: the parameters) and its returns (phase=after: ret)
//   - a store through a pointer parameter in <func> (phase=effect, no callee: operand0 = stored value,
//                                              operand1 = the address; line=<n> restricts it to that line)
// Values are the GIMPLE operands the original statement uses (SSA names or invariants), converted to
// 64 bits per the manifest type: nothing is evaluated twice, no shared variable is re-read (except the
// out<k> read the manifest declares), no store is changed.  A string is copied only at a slot the
// manifest marks string=validated; the plugin additionally checks the operand's type when it can.
//
// Pass positions: the insertion pass runs right after "ssa" (before every optimization, so the hooks
// follow inlining, cloning and unrolling of the code around them); a check pass after "optimized" counts,
// per binding, the hook calls left in each final function (bound / inlined / duplicated / removed).
// At the end of the unit it writes one "kzs-sites 1" binding file (bind lines + '#' notes) into out=.
// A unit no site names is not touched at all (no pass is registered for it).
//
// Arguments (-fplugin-arg-kzgcc-<key>=<value>):
//   manifest=<file>      the logical site manifest (required)
//   out=<dir>            where the per-unit binding file goes (required)
//   module=<id>:<prefix> units whose source path starts with <prefix> belong to module <id> (repeatable)
//   default-module=<id>  module of the other units (default 1)
//   skip-coverage        do not instrument units compiled with -fprofile-arcs (unit-test coverage builds)
// The runtime itself is never instrumented: a unit that defines a kz_rt_* function is refused.
//
// GCC internal API: only the exact GCC the plugin was built against is accepted
// (plugin_default_version_check); no compatibility across GCC versions is assumed.
// License note: GCC loads only plugins that declare plugin_is_GPL_compatible; the license under which
// this file is distributed is an open item of the PI (docs/instr_candidate_gcc.md).

#include "gcc-plugin.h"
#include "plugin-version.h"

#include "context.h"
#include "tree.h"
#include "tree-pass.h"
#include "basic-block.h"
#include "function.h"
#include "gimple.h"
#include "gimple-iterator.h"
#include "ssa.h"
#include "tree-dfa.h"
#include "tree-into-ssa.h"
#include "stringpool.h"
#include "attribs.h"
#include "cgraph.h"
#include "stor-layout.h"
#include "fold-const.h"
#include "diagnostic-core.h"
#include "tree-cfg.h"
#include "toplev.h"
#include "ggc.h"
#include "gtype-desc.h"
#include "version.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "kz/core/sha256.hpp"
#include "kz_rt_manifest.hpp"

int plugin_is_GPL_compatible;

namespace {

const char* const kPluginName = "kzgcc";
const char* const kVersion = "kzgcc 0.1";
const char* const kBackend = "gcc-plugin";

// ---- plugin state (one translation unit per cc1 process) -----------------------------------------
struct BindRec {
  uint32_t id = 0, site = 0, module = 0, vals = 0;
  std::string kind;          // call-before, call-after, entry, return, store
  std::string orig_func, at; // source function, location text
  std::string status = "bound", reason;
  std::string notes;         // string check, conversions
  std::map<std::string, int> copies;  // final function -> hook calls (check pass)
};

struct State {
  bool active = false;       // this unit has sites
  bool refused = false;
  std::string manifest_path, out_dir, tu;
  kz::Digest manifest_sha{};
  kzrt::Manifest m;
  uint32_t module = 1;
  std::vector<const kzrt::Site*> sites;  // sites whose tu is this unit
  std::vector<BindRec> binds;
  std::map<uint32_t, uint32_t> next_n;   // site -> next binding index
  std::set<uint32_t> matched;            // sites with at least one binding
  std::vector<std::string> notes;
  std::map<std::string, int> ctl_copies; // final function -> kz_rt_ctl calls
  std::vector<std::pair<uint32_t, std::string>> modules;
  bool skip_coverage = false;
};
State kzg;

// Hook declarations (GC roots: they live across passes).
tree fn_obs = NULL_TREE, fn_obs_str = NULL_TREE, fn_ctl = NULL_TREE, fn_module = NULL_TREE;
const struct ggc_root_tab kz_roots[] = {
    {&fn_obs, 1, sizeof(fn_obs), &gt_ggc_mx_tree_node, &gt_pch_nx_tree_node},
    {&fn_obs_str, 1, sizeof(fn_obs_str), &gt_ggc_mx_tree_node, &gt_pch_nx_tree_node},
    {&fn_ctl, 1, sizeof(fn_ctl), &gt_ggc_mx_tree_node, &gt_pch_nx_tree_node},
    {&fn_module, 1, sizeof(fn_module), &gt_ggc_mx_tree_node, &gt_pch_nx_tree_node},
    LAST_GGC_ROOT_TAB};

bool ends_with_path(const std::string& path, const std::string& suffix) {
  if (suffix.empty() || path.size() < suffix.size()) return false;
  if (path.compare(path.size() - suffix.size(), suffix.size(), suffix) != 0) return false;
  return path.size() == suffix.size() || path[path.size() - suffix.size() - 1] == '/';
}

std::string hex(const uint8_t* p, size_t n) {
  static const char* d = "0123456789abcdef";
  std::string s;
  for (size_t i = 0; i < n; ++i) {
    s += d[p[i] >> 4];
    s += d[p[i] & 15];
  }
  return s;
}

tree hook_decl(const char* name, tree ret, std::vector<tree> args, bool leaf) {
  tree type = build_function_type_array(ret, args.size(), args.data());
  tree d = build_fn_decl(name, type);
  TREE_NOTHROW(d) = 1;
  DECL_VISIBILITY(d) = VISIBILITY_DEFAULT;
  DECL_VISIBILITY_SPECIFIED(d) = 1;
  if (leaf) DECL_ATTRIBUTES(d) = tree_cons(get_identifier("leaf"), NULL_TREE, DECL_ATTRIBUTES(d));
  return d;
}

void make_decls() {
  if (fn_obs != NULL_TREE) return;
  tree u32 = uint32_type_node, u64 = uint64_type_node;
  // kz_rt_obs reads no memory of the target (only its operands): leaf, so the compiler need not assume
  // it changes this unit's static data.  kz_rt_obs_str reads the string through the pointer value it is
  // given and kz_rt_ctl may block: not leaf.
  fn_obs = hook_decl("kz_rt_obs", u32, {u32, u32, u32, u32, u64, u64, u64, u64}, true);
  fn_obs_str = hook_decl("kz_rt_obs_str", u32, {u32, u32, u32, u32, u64, u64, u64, u64, u32, u32}, false);
  fn_ctl = hook_decl("kz_rt_ctl", integer_type_node, {u32, u32, u32, u32}, false);
  fn_module = hook_decl("kz_rt_module", void_type_node, {u32, build_pointer_type(build_qualified_type(unsigned_char_type_node, TYPE_QUAL_CONST))},
                        false);
}

const char* fn_name(tree decl) { return IDENTIFIER_POINTER(DECL_NAME(decl)); }
const char* asm_name(tree decl) { return IDENTIFIER_POINTER(DECL_ASSEMBLER_NAME(decl)); }

int stmt_line(gimple* s) {
  location_t l = gimple_location(s);
  if (l == UNKNOWN_LOCATION) return 0;
  return LOCATION_LINE(l);
}

// ---- value conversion ---------------------------------------------------------------------------
// The operand as a GIMPLE value (a load into a new SSA name when it is a memory reference).
tree as_val(tree op, gimple_seq* seq) {
  if (is_gimple_val(op)) return op;
  tree t = make_ssa_name(TREE_TYPE(op));
  gimple_seq_add_stmt(seq, gimple_build_assign(t, op));
  return t;
}

tree conv(tree type, tree v, gimple_seq* seq) {
  if (TREE_TYPE(v) == type) return v;
  if (TREE_CODE(v) == INTEGER_CST) return fold_convert(type, v);
  tree t = make_ssa_name(type);
  gimple_seq_add_stmt(seq, gimple_build_assign(t, NOP_EXPR, v));
  return t;
}

// op (a call argument, parameter, result, stored value or loaded out value) -> uint64 per the manifest
// type: unsigned types zero-extend, signed sign-extend, ptr is the pointer value.  Returns NULL_TREE and
// sets *why when the type cannot be passed (aggregates larger than 8 bytes, floats).
tree to_u64(tree op, const kzrt::Value& v, gimple_seq* seq, std::string* why) {
  tree type = TREE_TYPE(op);
  tree val;
  if (AGGREGATE_TYPE_P(type)) {
    // a small aggregate (e.g. a strict-mode id struct): its bytes as an unsigned integer of its size
    if (!tree_fits_uhwi_p(TYPE_SIZE(type)) || tree_to_uhwi(TYPE_SIZE(type)) > 64 || tree_to_uhwi(TYPE_SIZE(type)) == 0) {
      *why = "aggregate operand larger than 8 bytes";
      return NULL_TREE;
    }
    tree it = build_nonstandard_integer_type(tree_to_uhwi(TYPE_SIZE(type)), 1);
    tree t = make_ssa_name(it);
    gimple_seq_add_stmt(seq, gimple_build_assign(t, build1(VIEW_CONVERT_EXPR, it, op)));
    val = t;
  } else if (POINTER_TYPE_P(type)) {
    val = conv(pointer_sized_int_node, as_val(op, seq), seq);
  } else if (INTEGRAL_TYPE_P(type)) {
    val = as_val(op, seq);
  } else {
    *why = "operand is neither integral, pointer nor a small aggregate";
    return NULL_TREE;
  }
  // the manifest's width and signedness
  unsigned bits = 64;
  bool sign = false;
  const std::string& t = v.type;
  if (t == "ptr") bits = 64;
  else if (t.size() >= 2 && (t[0] == 'u' || t[0] == 's')) {
    sign = t[0] == 's';
    bits = (unsigned)std::atoi(t.c_str() + 1);
  }
  tree nt = bits == 64 ? (sign ? build_nonstandard_integer_type(64, 0) : uint64_type_node)
                       : build_nonstandard_integer_type(bits, sign ? 0 : 1);
  if (TYPE_PRECISION(TREE_TYPE(val)) != bits || TYPE_UNSIGNED(TREE_TYPE(val)) != !sign) val = conv(nt, val, seq);
  return conv(uint64_type_node, val, seq);
}

// What the type of a string operand says about its bound: "array char[N]" when the pointer is the
// address of a char array of known size N (directly, or through one SSA copy/assignment), else "".
std::string string_evidence(tree op) {
  tree t = op;
  for (int hops = 0; hops < 4 && t != NULL_TREE; ++hops) {
    if (TREE_CODE(t) == SSA_NAME) {
      tree pt = TREE_TYPE(t);
      if (POINTER_TYPE_P(pt) && TREE_CODE(TREE_TYPE(pt)) == ARRAY_TYPE && TYPE_SIZE_UNIT(TREE_TYPE(pt)) &&
          tree_fits_uhwi_p(TYPE_SIZE_UNIT(TREE_TYPE(pt))))
        return "array char[" + std::to_string(tree_to_uhwi(TYPE_SIZE_UNIT(TREE_TYPE(pt)))) + "] (pointer type)";
      gimple* d = SSA_NAME_DEF_STMT(t);
      if (d == NULL || !is_gimple_assign(d)) return "";
      t = gimple_assign_rhs1(d);
      continue;
    }
    if (TREE_CODE(t) == ADDR_EXPR) {
      tree obj = TREE_OPERAND(t, 0);
      tree ot = TREE_TYPE(obj);
      if (TREE_CODE(ot) == ARRAY_TYPE && TYPE_SIZE_UNIT(ot) && tree_fits_uhwi_p(TYPE_SIZE_UNIT(ot)))
        return "array char[" + std::to_string(tree_to_uhwi(TYPE_SIZE_UNIT(ot))) + "] (address of array)";
      return "";
    }
    if (TREE_CODE(t) == NOP_EXPR) {
      t = TREE_OPERAND(t, 0);
      continue;
    }
    return "";
  }
  return "";
}

// ---- hook construction ------------------------------------------------------------------------------
struct Operands {      // where the slot values come from at one binding
  std::vector<tree> arg;  // call arguments or parameters (GIMPLE values or decls)
  tree ret = NULL_TREE;   // call result / return value
  tree operand0 = NULL_TREE, operand1 = NULL_TREE;  // store: value, address
  gcall* call = NULL;     // for out<k>
};

uint32_t bind_id(uint32_t module, uint32_t site, uint32_t n) { return (module << 16) | (site << 4) | n; }

// Builds the observation call for site s at one binding into *seq; returns its aseq SSA (uint32) or
// NULL_TREE when a value could not be bound (then *why is set and nothing is added).
tree build_obs(const kzrt::Site& s, uint32_t bind, tree link, const Operands& ops, gimple_seq* seq, uint32_t* vals, std::string* notes,
               std::string* why, location_t loc) {
  tree v[4] = {build_int_cst(uint64_type_node, 0), build_int_cst(uint64_type_node, 0), build_int_cst(uint64_type_node, 0),
               build_int_cst(uint64_type_node, 0)};
  uint32_t valid = 0;
  int str_slot = -1;
  uint32_t str_bound = 0;
  gimple_seq local = NULL;
  for (const kzrt::Value& val : s.values) {
    if (val.slot > 3) {
      *why = "slot > 3 (this prototype binds at most four values per event)";
      return NULL_TREE;
    }
    tree op = NULL_TREE;
    const std::string& f = val.from;
    if (f == "ret") {
      op = ops.ret;
      if (op == NULL_TREE) {
        *why = "no result value at this binding";
        return NULL_TREE;
      }
    } else if (f.compare(0, 3, "arg") == 0) {
      size_t k = (size_t)std::atoi(f.c_str() + 3);
      if (k >= ops.arg.size()) {
        *why = "argument " + std::to_string(k) + " does not exist here";
        return NULL_TREE;
      }
      op = ops.arg[k];
    } else if (f.compare(0, 3, "out") == 0) {
      size_t k = (size_t)std::atoi(f.c_str() + 3);
      if (ops.call == NULL || k >= gimple_call_num_args(ops.call)) {
        *why = "out" + std::to_string(k) + " needs a call argument " + std::to_string(k);
        return NULL_TREE;
      }
      tree p = gimple_call_arg(ops.call, k);
      if (!POINTER_TYPE_P(TREE_TYPE(p))) {
        *why = "out" + std::to_string(k) + ": the argument is not a pointer";
        return NULL_TREE;
      }
      // *arg<k>, read by the same thread right after the call (event_semantics.md 8.1)
      tree pointee = TREE_TYPE(TREE_TYPE(p));
      tree mem = build2(MEM_REF, pointee, p, build_int_cst(TREE_TYPE(p), 0));
      tree t = make_ssa_name(pointee);
      if (!is_gimple_reg_type(pointee)) {
        op = mem;  // small aggregate: to_u64 reads it with a VIEW_CONVERT_EXPR
      } else {
        gimple_seq_add_stmt(&local, gimple_build_assign(t, mem));
        op = t;
      }
      *notes += " out" + std::to_string(k) + "=load-after-call";
    } else if (f == "operand0") {
      op = ops.operand0;
    } else if (f == "operand1") {
      op = ops.operand1;
    }
    if (op == NULL_TREE) {
      *why = "value source " + f + " does not exist here";
      return NULL_TREE;
    }
    if (val.type == "ptr" && val.string_mode == "validated") {
      std::string ev = string_evidence(op);
      *notes += " slot" + std::to_string(val.slot) + ".string=" + (ev.empty() ? "type-unverified" : "type:" + ev);
      if (!ev.empty()) {
        size_t lb = ev.find('['), rb = ev.find(']');
        unsigned long n = std::strtoul(ev.c_str() + lb + 1, NULL, 10);
        if (rb != std::string::npos && n < val.bound) *notes += "(array smaller than bound " + std::to_string(val.bound) + ")";
      }
      str_slot = (int)val.slot;
      str_bound = val.bound;
    }
    tree c = to_u64(op, val, &local, why);
    if (c == NULL_TREE) return NULL_TREE;
    v[val.slot] = c;
    valid |= 1u << val.slot;
  }
  gcall* call;
  tree u32 = uint32_type_node;
  if (str_slot >= 0)
    call = gimple_build_call(fn_obs_str, 10, build_int_cst(u32, s.id), build_int_cst(u32, bind), link, build_int_cst(u32, valid), v[0], v[1],
                             v[2], v[3], build_int_cst(u32, str_slot), build_int_cst(u32, str_bound));
  else
    call = gimple_build_call(fn_obs, 8, build_int_cst(u32, s.id), build_int_cst(u32, bind), link, build_int_cst(u32, valid), v[0], v[1], v[2],
                             v[3]);
  tree aseq = make_ssa_name(u32);
  gimple_call_set_lhs(call, aseq);
  gimple_seq_add_stmt(&local, call);
  for (gimple_stmt_iterator i = gsi_start(local); !gsi_end_p(i); gsi_next(&i)) gimple_set_location(gsi_stmt(i), loc);
  gimple_seq_add_seq(seq, local);
  *vals = valid;
  return aseq;
}

void add_ctl(const kzrt::Site& s, uint32_t bind, tree aseq, gimple_seq* seq, location_t loc) {
  tree u32 = uint32_type_node;
  gcall* c = gimple_build_call(fn_ctl, 4, build_int_cst(u32, s.id), build_int_cst(u32, bind), aseq, build_int_cst(u32, kzrt::ctx_code(s.ctx)));
  gimple_set_location(c, loc);
  gimple_seq_add_stmt(seq, c);
}

BindRec& new_bind(const kzrt::Site& s, const std::string& kind, const std::string& at) {
  uint32_t n = kzg.next_n[s.id]++;
  BindRec b;
  b.id = bind_id(kzg.module, s.id, n);
  b.site = s.id;
  b.module = kzg.module;
  b.kind = kind;
  b.orig_func = s.func;
  b.at = at;
  if (n > 14) {
    b.status = "unsupported";
    b.reason = "more than 15 bindings of one site in one unit (bind id scheme)";
  }
  kzg.binds.push_back(b);
  kzg.matched.insert(s.id);
  return kzg.binds.back();
}

std::string where(gimple* st, const char* func) {
  location_t l = gimple_location(st);
  std::string file = l != UNKNOWN_LOCATION && LOCATION_FILE(l) ? LOCATION_FILE(l) : kzg.tu;
  size_t sl = file.rfind('/');
  return (sl == std::string::npos ? file : file.substr(sl + 1)) + ":" + std::to_string(stmt_line(st)) + " " + func;
}

// Inserts seq right after stmt (stmt does not end its block).
void insert_after(gimple_stmt_iterator* gsi, gimple_seq seq) { gsi_insert_seq_after(gsi, seq, GSI_CONTINUE_LINKING); }

// ---- the insertion pass -------------------------------------------------------------------------------
unsigned instrument_function(function* fun) {
  tree fndecl = fun->decl;
  const char* fname = fn_name(fndecl);
  if (std::strncmp(fname, "kz_rt_", 6) == 0) {
    kzg.refused = true;
    kzg.notes.push_back(std::string("refused: the unit defines runtime function ") + fname + " (the runtime is never instrumented)");
    return 0;
  }
  if (kzg.refused) return 0;
  std::vector<const kzrt::Site*> here;
  for (const kzrt::Site* s : kzg.sites)
    if (s->func == fname) here.push_back(s);
  if (here.empty()) return 0;
  make_decls();
  bool changed = false;
  location_t floc = DECL_SOURCE_LOCATION(fndecl);

  // parameters
  std::vector<tree> params;
  for (tree p = DECL_ARGUMENTS(fndecl); p; p = DECL_CHAIN(p)) {
    tree v = p;
    if (is_gimple_reg(p)) v = get_or_create_ssa_default_def(fun, p);
    params.push_back(v);
  }

  // 1. calls (before / after) and stores, collected first, then rewritten
  std::vector<gimple*> calls, stores, returns;
  basic_block bb;
  FOR_EACH_BB_FN(bb, fun) {
    for (gimple_stmt_iterator gsi = gsi_start_bb(bb); !gsi_end_p(gsi); gsi_next(&gsi)) {
      gimple* st = gsi_stmt(gsi);
      if (is_gimple_call(st)) calls.push_back(st);
      else if (gimple_code(st) == GIMPLE_RETURN) returns.push_back(st);
      else if (is_gimple_assign(st) && gimple_store_p(st)) stores.push_back(st);
    }
  }

  for (gimple* st : calls) {
    gcall* call = as_a<gcall*>(st);
    tree callee = gimple_call_fndecl(call);
    std::vector<const kzrt::Site*> before, after;
    for (const kzrt::Site* s : here) {
      if (s->callee.empty()) continue;
      if (callee == NULL_TREE || s->callee != fn_name(callee)) continue;
      if (s->src_line != 0 && (uint32_t)stmt_line(call) != s->src_line) continue;
      if (s->phase == "before") before.push_back(s);
      else if (s->phase == "after") after.push_back(s);
    }
    if (before.empty() && after.empty()) continue;
    const std::string at = where(call, fname) + " call " + fn_name(callee);
    location_t loc = gimple_location(call);
    Operands ops;
    for (unsigned i = 0; i < gimple_call_num_args(call); ++i) ops.arg.push_back(gimple_call_arg(call, i));
    ops.call = call;
    tree link = build_int_cst(uint32_type_node, 0);
    for (const kzrt::Site* s : before) {
      BindRec& b = new_bind(*s, "call-before", at);
      if (b.status != "bound") continue;
      gimple_seq seq = NULL;
      std::string why;
      tree aseq = build_obs(*s, b.id, build_int_cst(uint32_type_node, 0), ops, &seq, &b.vals, &b.notes, &why, loc);
      if (aseq == NULL_TREE) {
        b.status = "unsupported";
        b.reason = why;
        continue;
      }
      if (s->has_control) add_ctl(*s, b.id, aseq, &seq, loc);
      gimple_stmt_iterator gsi = gsi_for_stmt(call);
      gsi_insert_seq_before(&gsi, seq, GSI_SAME_STMT);
      if (integer_zerop(link)) link = aseq;
      changed = true;
    }
    if (after.empty()) continue;
    if (stmt_ends_bb_p(call)) {
      for (const kzrt::Site* s : after) {
        BindRec& b = new_bind(*s, "call-after", at);
        b.status = "unsupported";
        b.reason = "the call ends its basic block (noreturn, returns-twice or abnormal edge)";
      }
      continue;
    }
    // the result: an SSA name; give the call one when its result is unused or goes to memory
    tree rtype = TREE_TYPE(TREE_TYPE(callee));
    gimple_seq pre = NULL;
    if (!VOID_TYPE_P(rtype) && is_gimple_reg_type(rtype)) {
      tree lhs = gimple_call_lhs(call);
      if (lhs == NULL_TREE) {
        tree t = make_ssa_name(rtype);
        gimple_call_set_lhs(call, t);
        update_stmt(call);
        ops.ret = t;
      } else if (TREE_CODE(lhs) != SSA_NAME) {
        tree t = make_ssa_name(rtype);
        gimple_call_set_lhs(call, t);
        update_stmt(call);
        gimple* copy = gimple_build_assign(lhs, t);
        gimple_set_location(copy, loc);
        gimple_seq_add_stmt(&pre, copy);
        ops.ret = t;
      } else {
        ops.ret = lhs;
      }
    }
    gimple_seq seq = pre;
    for (const kzrt::Site* s : after) {
      BindRec& b = new_bind(*s, "call-after", at);
      if (b.status != "bound") continue;
      std::string why;
      tree aseq = build_obs(*s, b.id, link, ops, &seq, &b.vals, &b.notes, &why, loc);
      if (aseq == NULL_TREE) {
        b.status = "unsupported";
        b.reason = why;
        continue;
      }
      if (s->has_control) add_ctl(*s, b.id, aseq, &seq, loc);
      changed = true;
    }
    if (seq != NULL) {
      gimple_stmt_iterator gsi = gsi_for_stmt(call);
      insert_after(&gsi, seq);
      changed = true;
    }
  }

  // 2. stores through a pointer parameter (phase=effect, no callee)
  for (gimple* st : stores) {
    tree lhs = gimple_assign_lhs(st);
    if (TREE_CODE(lhs) != MEM_REF) continue;
    tree base = TREE_OPERAND(lhs, 0);
    tree parm = NULL_TREE;
    if (TREE_CODE(base) == SSA_NAME && SSA_NAME_VAR(base) && TREE_CODE(SSA_NAME_VAR(base)) == PARM_DECL && SSA_NAME_IS_DEFAULT_DEF(base))
      parm = SSA_NAME_VAR(base);
    if (parm == NULL_TREE) continue;
    for (const kzrt::Site* s : here) {
      if (s->phase != "effect" || !s->callee.empty()) continue;
      if (s->src_line != 0 && (uint32_t)stmt_line(st) != s->src_line) continue;
      BindRec& b = new_bind(*s, "store", where(st, fname) + " store *" + std::string(IDENTIFIER_POINTER(DECL_NAME(parm))));
      if (b.status != "bound") continue;
      Operands ops;
      ops.operand0 = gimple_assign_rhs1(st);
      ops.operand1 = build_fold_addr_expr(lhs);
      if (!is_gimple_val(ops.operand1)) ops.operand1 = base;
      gimple_seq seq = NULL;
      std::string why;
      tree aseq = build_obs(*s, b.id, build_int_cst(uint32_type_node, 0), ops, &seq, &b.vals, &b.notes, &why, gimple_location(st));
      if (aseq == NULL_TREE) {
        b.status = "unsupported";
        b.reason = why;
        continue;
      }
      if (s->has_control) add_ctl(*s, b.id, aseq, &seq, gimple_location(st));
      gimple_stmt_iterator gsi = gsi_for_stmt(st);
      insert_after(&gsi, seq);
      changed = true;
    }
  }

  // 3. entry (phase=before, no callee) and returns (phase=after, no callee)
  tree entry_aseq = build_int_cst(uint32_type_node, 0);
  for (const kzrt::Site* s : here) {
    if (s->phase != "before" || !s->callee.empty()) continue;
    BindRec& b = new_bind(*s, "entry", std::string("entry of ") + fname);
    if (b.status != "bound") continue;
    Operands ops;
    ops.arg = params;
    gimple_seq seq = NULL;
    std::string why;
    tree aseq = build_obs(*s, b.id, build_int_cst(uint32_type_node, 0), ops, &seq, &b.vals, &b.notes, &why, floc);
    if (aseq == NULL_TREE) {
      b.status = "unsupported";
      b.reason = why;
      continue;
    }
    if (s->has_control) add_ctl(*s, b.id, aseq, &seq, floc);
    gsi_insert_seq_on_edge_immediate(single_succ_edge(ENTRY_BLOCK_PTR_FOR_FN(fun)), seq);
    if (integer_zerop(entry_aseq)) entry_aseq = aseq;
    changed = true;
  }
  for (const kzrt::Site* s : here) {
    if (s->phase != "after" || !s->callee.empty()) continue;
    if (returns.empty()) {
      BindRec& b = new_bind(*s, "return", std::string("returns of ") + fname);
      b.status = "unsupported";
      b.reason = "the function has no return statement";
      continue;
    }
    for (gimple* st : returns) {
      greturn* r = as_a<greturn*>(st);
      BindRec& b = new_bind(*s, "return", where(st, fname) + " return");
      if (b.status != "bound") continue;
      Operands ops;
      ops.arg = params;
      ops.ret = gimple_return_retval(r);
      gimple_seq seq = NULL;
      std::string why;
      tree aseq = build_obs(*s, b.id, entry_aseq, ops, &seq, &b.vals, &b.notes, &why, gimple_location(st));
      if (aseq == NULL_TREE) {
        b.status = "unsupported";
        b.reason = why;
        continue;
      }
      if (s->has_control) add_ctl(*s, b.id, aseq, &seq, gimple_location(st));
      gimple_stmt_iterator gsi = gsi_for_stmt(st);
      gsi_insert_seq_before(&gsi, seq, GSI_SAME_STMT);
      changed = true;
    }
  }
  if (!changed) return 0;
  mark_virtual_operands_for_renaming(fun);
  return TODO_update_ssa;
}

const pass_data instr_pass_data = {GIMPLE_PASS, "kzgcc_instr", OPTGROUP_NONE, TV_NONE, PROP_cfg | PROP_ssa, 0, 0, 0, TODO_update_ssa};
struct instr_pass : gimple_opt_pass {
  instr_pass(gcc::context* ctx) : gimple_opt_pass(instr_pass_data, ctx) {}
  bool gate(function*) final override { return kzg.active && !kzg.refused; }
  unsigned int execute(function* fun) final override { return instrument_function(fun); }
};

// ---- the check pass (after "optimized"): hook calls per final function --------------------------------
unsigned check_function(function* fun) {
  const char* fname = asm_name(fun->decl);
  basic_block bb;
  FOR_EACH_BB_FN(bb, fun) {
    for (gimple_stmt_iterator gsi = gsi_start_bb(bb); !gsi_end_p(gsi); gsi_next(&gsi)) {
      gimple* st = gsi_stmt(gsi);
      if (!is_gimple_call(st)) continue;
      tree callee = gimple_call_fndecl(st);
      if (callee == NULL_TREE) continue;
      const char* n = fn_name(callee);
      if (std::strcmp(n, "kz_rt_obs") != 0 && std::strcmp(n, "kz_rt_obs_str") != 0 && std::strcmp(n, "kz_rt_ctl") != 0) continue;
      tree b = gimple_call_arg(st, 1);
      if (TREE_CODE(b) != INTEGER_CST) continue;
      uint32_t id = (uint32_t)tree_to_uhwi(b);
      if (std::strcmp(n, "kz_rt_ctl") == 0) {
        kzg.ctl_copies[std::string(fname) + "#" + std::to_string(id)]++;
        continue;
      }
      for (BindRec& r : kzg.binds)
        if (r.id == id) r.copies[fname]++;
    }
  }
  return 0;
}

const pass_data check_pass_data = {GIMPLE_PASS, "kzgcc_check", OPTGROUP_NONE, TV_NONE, PROP_cfg, 0, 0, 0, 0};
struct check_pass : gimple_opt_pass {
  check_pass(gcc::context* ctx) : gimple_opt_pass(check_pass_data, ctx) {}
  bool gate(function*) final override { return kzg.active; }
  unsigned int execute(function* fun) final override { return check_function(fun); }
};

// ---- module registration: a static constructor kz_rt_module(module, manifest sha256) ----------------
// Built by a small IPA pass right after "*free_lang_data" (front-end data is gone, the per-function
// passes have not run yet), only in a unit that has sites and was not refused.
unsigned build_module_ctor() {
  if (!kzg.active || kzg.refused) return 0;
  make_decls();
  tree lit = build_string_literal(32, reinterpret_cast<const char*>(kzg.manifest_sha.data()));
  tree arg = fold_convert(TREE_VALUE(TREE_CHAIN(TYPE_ARG_TYPES(TREE_TYPE(fn_module)))), lit);
  tree body = build_call_expr(fn_module, 2, build_int_cst(uint32_type_node, kzg.module), arg);
  cgraph_build_static_cdtor('I', body, DEFAULT_INIT_PRIORITY);
  kzg.notes.push_back("module constructor: kz_rt_module(" + std::to_string(kzg.module) + ", manifest sha256) at load");
  return 0;
}

const pass_data module_pass_data = {SIMPLE_IPA_PASS, "kzgcc_module", OPTGROUP_NONE, TV_NONE, 0, 0, 0, 0, 0};
struct module_pass : simple_ipa_opt_pass {
  module_pass(gcc::context* ctx) : simple_ipa_opt_pass(module_pass_data, ctx) {}
  bool gate(function*) final override { return kzg.active && !kzg.refused; }
  unsigned int execute(function*) final override { return build_module_ctor(); }
};

// A unit that defines a runtime function is refused before any pass runs (the runtime is never
// instrumented, kz_rt.h).
void finish_parse_function(void* gcc_data, void*) {
  tree fn = static_cast<tree>(gcc_data);
  if (fn == NULL_TREE || TREE_CODE(fn) != FUNCTION_DECL || DECL_NAME(fn) == NULL_TREE) return;
  const char* n = IDENTIFIER_POINTER(DECL_NAME(fn));
  if (std::strncmp(n, "kz_rt_", 6) == 0 && !kzg.refused) {
    kzg.refused = true;
    kzg.notes.push_back(std::string("refused: the unit defines runtime function ") + n + " (the runtime is never instrumented)");
  }
}

// ---- the binding file -----------------------------------------------------------------------------
void finish_unit(void*, void*) {
  if (!kzg.active) return;
  bool lto = flag_generate_lto || flag_lto != NULL;
  bool lto_only = lto && !flag_fat_lto_objects;
  for (BindRec& b : kzg.binds) {
    if (b.status != "bound") continue;
    int total = 0;
    bool other = false;
    for (const auto& c : b.copies) {
      total += c.second;
      if (c.first != b.orig_func) other = true;
    }
    if (lto_only) {
      b.notes += " check=deferred-to-lto-link";
      continue;
    }
    if (total == 0) {
      b.status = "removed";
      b.reason = "no hook call left after optimization (the code around it was removed)";
    } else if (total > 1) {
      b.status = "duplicated";
    } else if (other) {
      b.status = "inlined";
    }
  }
  std::string base = kzg.tu;
  for (char& c : base)
    if (c == '/') c = '_';
  std::string dumpbase = dump_base_name ? dump_base_name : "";
  kz::Digest dh = kz::sha256(dumpbase + "|" + kzg.tu);
  std::string path = kzg.out_dir + "/" + base + "." + hex(dh.data(), 4) + ".kzs";
  std::ofstream o(path);
  if (!o) {
    error("kzgcc: cannot write %s", path.c_str());
    return;
  }
  o << "kzs-sites 1\n";
  o << "# " << kVersion << " backend=" << kBackend << " gcc=" << version_string << " " << pkgversion_string << " unit=" << kzg.tu << " dumpbase=" << dumpbase << "\n";
  o << "# manifest=" << kzg.manifest_path << " sha256=" << hex(kzg.manifest_sha.data(), 32) << " module=" << kzg.module << "\n";
  o << "# optimize=-O" << (int)optimize << (optimize_size ? " (size)" : "") << " lto=" << (lto ? "on" : "off")
    << (flag_fat_lto_objects ? " fat" : "") << " pic=" << flag_pic << " pie=" << flag_pie << " profile_arcs=" << profile_arc_flag << "\n";
  o << "# passes: insertion after \"ssa\" (first instance), check after \"optimized\"\n";
  for (const std::string& n : kzg.notes) o << "# note " << n << "\n";
  for (const BindRec& b : kzg.binds) {
    std::string copies;
    for (const auto& c : b.copies) copies += (copies.empty() ? "" : ",") + c.first + ":" + std::to_string(c.second);
    o << "# bind " << b.id << " kind=" << b.kind << " source_func=" << b.orig_func << " final=" << (copies.empty() ? "-" : copies)
      << (b.notes.empty() ? "" : " notes=" + b.notes) << "\n";
    o << "bind " << b.id << " site=" << b.site << " backend=" << kBackend << " module=" << b.module << " at=\"" << b.at
      << (copies.empty() ? "" : " -> " + copies) << "\" status=" << b.status;
    if (b.status == "bound" || b.status == "inlined" || b.status == "duplicated") {
      char vb[16];
      std::snprintf(vb, sizeof vb, "0x%x", b.vals);
      o << " vals=" << vb;
    }
    if (!b.reason.empty()) o << " reason=\"" << b.reason << "\"";
    o << "\n";
  }
  for (const auto& c : kzg.ctl_copies) o << "# ctl " << c.first << ":" << c.second << "\n";
  // sites of this unit without any binding: declared gaps
  for (const kzrt::Site* s : kzg.sites) {
    if (kzg.matched.count(s->id)) continue;
    uint32_t id = bind_id(kzg.module, s->id, 15);
    o << "bind " << id << " site=" << s->id << " backend=" << kBackend << " module=" << kzg.module << " at=\"" << kzg.tu << " " << s->func
      << "\" status=unsupported reason=\"" << (kzg.refused ? "unit refused" : "no matching function, call or store in the unit") << "\"\n";
  }
}

bool read_file(const std::string& path, std::string* out) {
  std::ifstream f(path, std::ios::binary);
  if (!f) return false;
  std::ostringstream ss;
  ss << f.rdbuf();
  *out = ss.str();
  return true;
}

// The unit: which sites it has and which module it belongs to (main_input_filename is known here).
void start_unit(void*, void*) {
  kzg.tu = main_input_filename ? main_input_filename : "";
  for (const kzrt::Site& s : kzg.m.sites)
    if (ends_with_path(kzg.tu, s.tu)) kzg.sites.push_back(&s);
  if (kzg.sites.empty()) return;  // this unit is not touched
  for (const auto& md : kzg.modules)
    if (kzg.tu.compare(0, md.second.size(), md.second) == 0) {
      kzg.module = md.first;
      break;
    }
  if (kzg.m.module(kzg.module) == nullptr) {
    error("kzgcc: module %u is not declared in %s", kzg.module, kzg.manifest_path.c_str());
    return;
  }
  kzg.active = true;
  if (kzg.skip_coverage && profile_arc_flag) {
    kzg.refused = true;
    kzg.notes.push_back("not instrumented: unit compiled with -fprofile-arcs (skip-coverage)");
  }
}

}  // namespace

int plugin_init(struct plugin_name_args* info, struct plugin_gcc_version* version) {
  if (!plugin_default_version_check(version, &gcc_version)) {
    error("kzgcc: built for GCC %s, loaded into a different GCC", gcc_version.basever);
    return 1;
  }
  std::vector<std::pair<uint32_t, std::string>> modules;
  bool skip_coverage = false;
  for (int i = 0; i < info->argc; ++i) {
    const std::string k = info->argv[i].key, v = info->argv[i].value ? info->argv[i].value : "";
    if (k == "manifest") kzg.manifest_path = v;
    else if (k == "out") kzg.out_dir = v;
    else if (k == "default-module") kzg.module = (uint32_t)std::strtoul(v.c_str(), NULL, 10);
    else if (k == "module") {
      size_t c = v.find(':');
      if (c == std::string::npos) {
        error("kzgcc: module wants id:prefix, got %qs", v.c_str());
        return 1;
      }
      modules.push_back({(uint32_t)std::strtoul(v.substr(0, c).c_str(), NULL, 10), v.substr(c + 1)});
    } else if (k == "skip-coverage") skip_coverage = true;
    else {
      error("kzgcc: unknown argument %s", k.c_str());
      return 1;
    }
  }
  if (kzg.manifest_path.empty() || kzg.out_dir.empty()) {
    error("kzgcc: arguments %qs and %qs are required", "manifest", "out");
    return 1;
  }
  std::string text, err;
  if (!read_file(kzg.manifest_path, &text)) {
    error("kzgcc: cannot read %s", kzg.manifest_path.c_str());
    return 1;
  }
  if (!kzrt::parse_manifest(text, kzg.manifest_path, &kzg.m, &err)) {
    error("kzgcc: %s", err.c_str());
    return 1;
  }
  kzg.manifest_sha = kz::sha256(text);
  kzg.modules = modules;
  kzg.skip_coverage = skip_coverage;
  static struct plugin_info pinfo = {kVersion, "manifest=<kzs-sites file> out=<dir> [module=<id>:<prefix>]... [default-module=<id>] [skip-coverage]"};
  register_callback(kPluginName, PLUGIN_INFO, NULL, &pinfo);
  register_callback(kPluginName, PLUGIN_START_UNIT, start_unit, NULL);
  register_callback(kPluginName, PLUGIN_REGISTER_GGC_ROOTS, NULL, const_cast<ggc_root_tab*>(kz_roots));
  // The passes are registered for every unit; their gates are false in a unit no site names, so such a
  // unit is compiled exactly as without the plugin.
  struct register_pass_info ins;
  ins.pass = new instr_pass(g);
  ins.reference_pass_name = "ssa";
  ins.ref_pass_instance_number = 1;
  ins.pos_op = PASS_POS_INSERT_AFTER;
  register_callback(kPluginName, PLUGIN_PASS_MANAGER_SETUP, NULL, &ins);
  struct register_pass_info chk;
  chk.pass = new check_pass(g);
  chk.reference_pass_name = "optimized";
  chk.ref_pass_instance_number = 1;
  chk.pos_op = PASS_POS_INSERT_AFTER;
  register_callback(kPluginName, PLUGIN_PASS_MANAGER_SETUP, NULL, &chk);
  struct register_pass_info mod;
  mod.pass = new module_pass(g);
  mod.reference_pass_name = "*free_lang_data";
  mod.ref_pass_instance_number = 1;
  mod.pos_op = PASS_POS_INSERT_AFTER;
  register_callback(kPluginName, PLUGIN_PASS_MANAGER_SETUP, NULL, &mod);
  register_callback(kPluginName, PLUGIN_FINISH_PARSE_FUNCTION, finish_parse_function, NULL);
  register_callback(kPluginName, PLUGIN_FINISH_UNIT, finish_unit, NULL);
  return 0;
}

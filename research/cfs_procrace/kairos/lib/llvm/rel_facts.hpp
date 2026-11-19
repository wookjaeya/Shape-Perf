// rel_facts.hpp -- the facts the relation rules read, loaded once: the sections of a facts container, the
// indices over them and the static actors (the roots, their code, the trampolines).
//
// `kz-static relations` (relations.cpp) builds its rows on it, and so does every tool that reads the same facts the
// same way (the rows of the relations and the tools that start from them must not disagree on the loading, the
// indices or the actors).  It reads one facts container and nothing else: no field, callee or case name.  After
// prepare() the members are read only, except file_notes_ (see note_file).  No Clang types.
#pragma once

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "kz/core/container.hpp"
#include "kz/core/dump.hpp"
#include "kz/core/registry.hpp"

namespace kzs {

// The functions of the C library that return twice (ISO C 7.13 setjmp and the names it is spelled as in glibc 2.39,
// POSIX sigsetjmp, the ucontext getcontext, vfork): a non-local jump (longjmp, siglongjmp, setcontext) or the second
// return may come back to such a call in an order and under locks the control-flow graph does not have.
inline constexpr const char* kReturnsTwice[] = {"setjmp", "_setjmp", "__setjmp", "sigsetjmp", "__sigsetjmp", "savectx", "getcontext", "vfork"};

struct RelFacts {
  static constexpr uint32_t kN = KZ_NONE32;

  explicit RelFacts(const kz::Container& c) : c_(c) {}

  // load(), index() and actors(), in that order
  kz::Status prepare() {
    kz::Status s = load();
    if (!s.ok()) return s;
    index();
    actors();
    return kz::Status::success();
  }

  // ---------------------------------------------------------------- loading
  template <class T>
  kz::Status read(uint32_t id, std::vector<T>* v) {
    return c_.read_section(id, v);
  }
  std::string str(const kz_str& s) const {
    std::string out;
    c_.string(s, &out);
    return out;
  }
  static std::string base_name(const std::string& key) {
    const size_t at = key.find('@');
    return at == std::string::npos ? key : key.substr(0, at);
  }
  std::string fname(uint32_t fn) const {
    if (fn == kN || fn >= funcs_.size()) return "-";
    if (defined(fn)) note_file(str(funcs_[fn].file));
    if (fn_notes_ != nullptr) fn_notes_->insert(fn);
    std::string n = str(funcs_[fn].name);
    if (funcs_[fn].flags & KZ_FUNC_FLAG_STATIC) n += "@" + str(funcs_[fn].file) + ":" + std::to_string(funcs_[fn].line);
    return n;
  }
  static std::string join(const std::vector<std::string>& v, const char* sep) {
    std::string t;
    for (const std::string& x : v) t += (t.empty() ? "" : sep) + x;
    return t;
  }
  std::string short_fname(uint32_t fn) const { return fn == kN || fn >= funcs_.size() ? "-" : str(funcs_[fn].name); }
  bool defined(uint32_t fn) const { return fn != kN && fn < funcs_.size() && (funcs_[fn].flags & KZ_FUNC_FLAG_DEFINED) != 0; }
  // a position; the file is noted in file_notes_ when it is set (the relation rows note the files of the row being
  // written: the basis deviations of their units)
  void note_file(const std::string& f) const {
    if (!f.empty() && file_notes_ != nullptr) file_notes_->insert(f);
  }
  std::string pos(const std::string& file, uint32_t line, uint32_t col) const {
    note_file(file);
    std::string p = file + ":" + std::to_string(line);
    if (col != 0) p += ":" + std::to_string(col);
    return p;
  }
  std::string call_pos(uint32_t ci) const { return pos(str(calls_[ci].file), calls_[ci].line, calls_[ci].col); }
  static std::string written_flags(uint32_t f) {
    std::string t;
    if (f & KZ_GIMAGE_FLAG_OBJECT_WRITTEN) t += "object-written";
    if (f & KZ_GIMAGE_FLAG_FIELD_WRITTEN) t += std::string(t.empty() ? "" : ", ") + "field-written";
    if (f & KZ_GIMAGE_FLAG_RECORD_WRITTEN) t += std::string(t.empty() ? "" : ", ") + "record-written";
    return t;
  }
  std::string enum_text(const char* name, uint64_t v) const {
    const kz::EnumDesc* d = kz::find_enum(name);
    if (d == nullptr) return std::to_string(v);
    const char* t = kz::enum_display(*d, v);
    return t != nullptr ? t : "#" + std::to_string(v);
  }
  static bool before(const std::string& fa, uint32_t la, uint32_t ca, const std::string& fb, uint32_t lb, uint32_t cb) {
    return fa == fb && (la < lb || (la == lb && ca < cb));
  }


  kz::Status load();
  // ---------------------------------------------------------------- indices
  void index();
  // The code of the program a thread may run after its entry function has returned: the destructors of the
  // thread-specific keys (P0 key creation with a destructor argument that is not a null constant) and the cleanup
  // handlers (P0 cleanup push; conservatively every push of the program).
  void thread_end_code();
  // The entries of a spawn call whose entry argument is read from a global table with a static image: the function
  // leaves of that image at the field the argument reads (GLOBAL_IMAGES), for the spawn calls the facts leave without
  // a resolved entry.  The entry argument is the P0 entry parameter of a thread-creation primitive, or the parameter a
  // derived spawn operation passes on (SPAWN_WRAP).  No creation order is read from the image.  The image is the run-time
  // value only when nothing writes the member: the member's writer set must be complete with no writer instance under
  // both completeness rules of FIELD_STATUS (typed, data-function and out-pointer writes; an escape, a cast, a union or
  // an unknown effect makes it not complete), and no leaf used may be marked as written (object-, field- or
  // record-written: a store to the object, the field or a whole record of its type); otherwise the spawn stays
  // unresolved, with the reason.
  // Why the static image of a member is not taken as its run-time value ("" when it is): the image was cut at the image
  // budget (`cut`: its marker leaves, image_cut; relations rules revision 6), a leaf of the image is marked written
  // (object-, field- or record-written, GLOBAL_IMAGES), FIELD_STATUS has no row of the member, its writer set is not
  // complete under both rules, or it has writers at run time.  The rule of the static-image entries and of the image
  // keys of path A.
  std::string image_value_why(const std::string& type, const std::string& field, uint16_t written, const std::string& cut = "") const;
  // The marker leaves `<image truncated>` of the static image of an object (GLOBAL_IMAGES), as "object<path>, ...", or ""
  // when the image is whole: the extraction stopped there at the image budget (static_facts.md 5j), and the part from
  // there on was not read.  An image whose rows carry the flag image-truncated without a marker row is cut too (its text
  // is then "<object> (image-truncated)").  Relations rules revision 6: a value read from a cut image is not known to be
  // all the values of that image.
  std::string image_cut(const std::string& object) const;
  void image_entries();
  // the functions whose every return value is the address of (a part of) a global object or null
  // (target_rules.cpp, the same rule over the records)
  void global_address_functions();
  bool shared_base(const kz_fact_argflow& a) const;
  // ---------------------------------------------------------------- actors (static, may)
  // Roots: the process root (`main`), the resolved entries of spawn calls (CALL_TARGETS spawn-entry, the
  // unrolled table-walk iterations, the startup-script entries of SPAWN_WRAP, the function leaves of the static
  // image a spawn's entry argument is read from).  A root's code is what it reaches over the call graph (direct
  // calls and resolved indirect targets) without entering another root.  A root other than the process root is a
  // trampoline -- no actor of its own, its code belongs to the roots it calls (transitively through other
  // trampolines) -- when it dispatches only: it enters other roots, only through indirect calls, and its own body
  // reads no field or global through a shared root (READS: a global, or the result of a function that returns
  // global addresses only) and writes no global object (LOCKSET write rows of `<global>`).  The record-field writes
  // of its body are not read: a typed write carries no base in these facts.  The roots a trampoline stands for are
  // those its own code enters through resolved indirect calls (field-based flow graph), and the entries resolved at
  // the calls of every derived spawn operation that passes the trampoline as the entry of its inner spawn (SPAWN_WRAP
  // called-by-entry): a thread that operation starts runs the trampoline and, through it, the entry its caller passed.
  // An entry not resolved at such a call is missing (an under-approximation, named in the header).
  bool own_shared_access(uint32_t r) const;
  void actors();
  std::set<uint32_t> actor_set(uint32_t fn) const;
  // the actors a set of spawned entries stands for
  std::set<uint32_t> entry_actors(const std::set<uint32_t>& entries) const;
  // the functions a call may run: its defined callee, its resolved indirect targets, and the functions passed as
  // arguments to a callee without a body that is no P0 entry (it may call them back)
  std::set<uint32_t> call_runs(uint32_t ci) const;
  // the defined functions a call passes as arguments to a callee without a body that is no P0 entry (it may call them
  // back during the call)
  std::set<uint32_t> callback_args(uint32_t ci) const;
  // ---------------------------------------------------------------- state
  const kz::Container& c_;
  std::vector<kz_basis_item> basis_;
  std::vector<kz_fact_func> funcs_;
  std::vector<kz_fact_call> calls_;
  std::vector<kz_fact_expr> exprs_;
  std::vector<kz_fact_loop> loops_;
  std::vector<kz_fact_init> inits_;
  std::vector<kz_fact_writer> writers_;
  std::vector<kz_fact_target> targets_;
  std::vector<kz_fact_terminal> terminals_;
  std::vector<kz_fact_iter> iters_;
  std::vector<kz_fact_fwrite> fwrites_;
  std::vector<kz_fact_p0> p0_;
  std::vector<kz_fact_opderiv> opderiv_;
  std::vector<kz_fact_spawnw> spawnw_;
  std::vector<kz_fact_read> reads_;
  std::vector<kz_fact_return> returns_;
  std::vector<kz_fact_retconst> retconsts_;
  std::vector<kz_fact_loopexit> loopexits_;
  std::vector<kz_fact_argflow> argflow_;
  std::vector<kz_fact_pflow> pflow_;
  std::vector<kz_fact_ptrfield> ptrfields_;
  std::vector<kz_fact_gimage> gimages_;
  std::vector<kz_fact_lockid> lockids_;
  std::vector<kz_fact_lockset> lockset_;
  std::vector<kz_fact_fstatus> fstatus_;
  std::vector<kz_fact_atype> atypes_;  // ATYPE (the address-type hits; read by relations rules revision 14, the holder line)
  std::vector<kz_fact_await> awaits_;
  std::vector<kz_fact_lockpath> lockpaths_;
  std::vector<kz_fact_sem> sem_;  // SEMANTICS (extract --semantics, static_facts.md 5t); empty without the section
  std::vector<kz_fact_rflow> rflow_;      // RESULT_FLOW (extract --result-flow, static_facts.md 5u); empty without it
  std::vector<kz_fact_reffect> reffect_;  // RESULT_EFFECT; empty without it
  bool result_flow_ = false;              // the facts carry RESULT_FLOW and RESULT_EFFECT
  bool has_objacc_ = false;               // the facts carry OBJECT_ACCESS (extract --object-access)
  std::map<uint32_t, std::vector<uint32_t>> rflow_of_site_, reffect_of_site_;  // call site -> rows
  std::vector<kz_fact_loopdef> loopdefs_;  // LOOP_DEFS (extract --loop-defs, static_facts.md 5v); empty without it
  std::vector<kz_fact_lockh> lockh_;       // LOCK_HANDLES (extract --lock-handles); empty without it
  std::vector<kz_fact_lockop> lockops_;    // LOCK_OPS (read for LOCK_HANDLES)
  bool loop_defs_ = false, lock_handles_ = false;  // the facts carry LOOP_DEFS, LOCK_HANDLES
  std::map<uint32_t, uint32_t> loopdef_head_;      // loop site -> its own LOOP_DEFS row
  std::map<uint32_t, std::vector<uint32_t>> lockh_of_fn_;  // the operation's function -> LOCK_HANDLES rows

  struct BasisMap : std::map<std::string, std::string> {
    std::string at_or(const std::string& k) const {
      auto it = find(k);
      return it == end() ? std::string("?") : it->second;
    }
  } basis_map_;
  bool profile_free_ = false;
  uint32_t hand_entered_vocabulary_ = 0;
  std::map<std::string, std::vector<uint32_t>> fn_by_name_;
  std::vector<std::vector<std::pair<uint32_t, bool>>> succ_;
  std::vector<std::vector<uint32_t>> callers_;  // direct calls into a function
  std::map<uint32_t, std::vector<uint32_t>> indirect_callers_;
  std::map<uint32_t, uint32_t> call_by_site_;
  std::map<uint32_t, std::vector<uint32_t>> calls_of_;
  std::map<uint32_t, std::set<uint32_t>> spawn_entries_;  // spawn site -> entries (kN: unresolved)
  std::map<uint32_t, std::map<uint32_t, uint32_t>> iter_spawn_;  // spawn site -> entry -> iteration
  std::map<uint32_t, uint32_t> iter_loop_;
  std::set<uint32_t> script_entries_;
  std::map<std::pair<uint32_t, uint32_t>, uint32_t> argflow_at_;
  std::map<std::string, std::vector<uint32_t>> reads_of_field_;
  std::map<uint32_t, std::vector<uint32_t>> returns_of_, retconsts_of_, loopexits_of_, lockset_writes_of_, terminals_of_, opderiv_of_, awaits_of_;
  std::map<std::pair<uint32_t, uint32_t>, std::vector<uint32_t>> pflow_of_;
  std::map<uint32_t, uint32_t> lockset_call_, lockset_read_, ptrread_of_, loop_by_index_;
  std::map<std::string, uint32_t> fstatus_of_;
  std::map<std::string, std::vector<uint32_t>> atypes_of_;  // "Type.field" -> its ATYPE rows
  std::map<std::string, std::vector<uint32_t>> writers_of_, inits_of_, fwrites_of_, gimages_of_;
  std::map<std::string, uint8_t> p0_kind_;
  std::set<std::string> compare_names_;
  std::set<std::string> gaf_;
  std::map<uint32_t, std::string> root_kind_;
  std::map<uint32_t, std::set<uint32_t>> reach_, delegates_, actors_of_;
  std::set<uint32_t> trampolines_, kept_direct_, kept_shared_;
  std::map<uint32_t, std::set<uint32_t>> after_spawn_;  // spawn site -> LOCKSET rows after it (LOCK_PATHS)
  std::map<uint32_t, uint32_t> entry_release_;           // LOCKSET row -> the call site (LOCK_PATHS entry-release)
  std::map<uint32_t, std::set<uint32_t>> itargets_;      // indirect call site -> resolved targets
  std::map<uint32_t, std::set<uint32_t>> image_spawn_;   // spawn site -> entries from a static image
  std::map<uint32_t, std::string> image_member_;          // spawn site -> the member the entry argument reads (Type.field)
  std::map<uint32_t, std::string> image_premise_;         // spawn site -> what its entries from the image rest on (relations rules revision 5)
  std::map<uint32_t, std::string> image_cut_;             // spawn site -> its entries come from a cut image: why the set is not all (revision 6)
  std::map<uint32_t, std::string> unresolved_why_;        // indirect call site -> the reason of its unresolved row (CALL_TARGETS)
  std::map<uint32_t, std::map<uint32_t, std::string>> image_paths_;  // spawn site -> entry -> its image paths
  std::map<uint32_t, std::map<uint32_t, std::set<std::string>>> via_op_;  // trampoline -> root -> the derived spawn operations
  std::map<uint32_t, std::set<std::string>> via_op_missing_;  // trampoline -> calls of such an operation with no resolved entry
  std::set<uint32_t> unresolved_sites_;                  // indirect calls with a target the flow graph does not resolve
  std::set<std::string> p0_unverified_, lock_prims_;
  std::map<uint32_t, std::set<std::string>> script_prim_;  // script entry -> the P0 symbol lookup it is resolved through
  std::map<uint32_t, std::vector<uint32_t>> callback_callers_;  // function -> calls that pass it to a callee without a body
  std::map<uint32_t, std::vector<uint32_t>> returns_twice_;     // function -> its calls of a function that returns twice
  std::vector<std::string> deviations_;
  std::map<std::string, std::set<std::string>> deviation_files_;
  std::set<std::string> assumptions_;
  std::vector<std::string> keys_dtor_, keys_null_dtor_, cleanup_pushes_, unresolved_spawns_;
  // the files of the positions and functions named since the set was last cleared (set by the relation rows)
  std::set<std::string>* file_notes_ = nullptr;
  // the functions a row names (fname), when set: relations notes them only for facts with a SEMANTICS section (5t)
  std::set<uint32_t>* fn_notes_ = nullptr;
};

}  // namespace kzs

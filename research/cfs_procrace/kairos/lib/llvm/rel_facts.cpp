// rel_facts.cpp -- see rel_facts.hpp.  The rules are those of relations rules revision 5 (docs/static_facts.md
// §5e-§5i); they were part of relations.cpp before and are unchanged.
#include "rel_facts.hpp"

#include <algorithm>
#include <cctype>
#include <functional>

#include "p0.hpp"

namespace kzs {

kz::Status RelFacts::load() {
  for (uint32_t need : {KZ_SEC_READS, KZ_SEC_RETURNS, KZ_SEC_ARGFLOW, KZ_SEC_PARAM_FLOW, KZ_SEC_LOCKSET, KZ_SEC_LOCK_IDS, KZ_SEC_FIELD_STATUS})
    if (c_.find_section(need) == nullptr)
      return kz::Status::error(kz::Err::Invalid,
                               "the facts have no extended facts (section " + std::to_string(need) +
                                   "): extract them with --no-profile, or with --extended-facts in the profile-driven mode");
  if (c_.find_section(KZ_SEC_LOCK_PATHS) == nullptr)
    return kz::Status::error(kz::Err::Invalid,
                             "the facts have no LOCK_PATHS section (facts schema revision 11, written with the extended facts): extract them again with this kz-static");
  kz::Status s;
  if (!(s = read(KZ_SEC_BASIS, &basis_)).ok()) return s;
  if (!(s = read(KZ_SEC_FUNCS, &funcs_)).ok()) return s;
  if (!(s = read(KZ_SEC_FACTS, &calls_)).ok()) return s;
  if (!(s = read(KZ_SEC_FACT_EXPRS, &exprs_)).ok()) return s;
  if (!(s = read(KZ_SEC_FACT_LOOPS, &loops_)).ok()) return s;
  if (!(s = read(KZ_SEC_FACT_INIT, &inits_)).ok()) return s;
  if (!(s = read(KZ_SEC_WRITERS, &writers_)).ok()) return s;
  if (!(s = read(KZ_SEC_CALL_TARGETS, &targets_)).ok()) return s;
  if (!(s = read(KZ_SEC_TERMINALS, &terminals_)).ok()) return s;
  if (!(s = read(KZ_SEC_LOOP_ITERS, &iters_)).ok()) return s;
  if (!(s = read(KZ_SEC_FUNC_WRITES, &fwrites_)).ok()) return s;
  if (!(s = read(KZ_SEC_P0_TABLE, &p0_)).ok()) return s;
  if (!(s = read(KZ_SEC_OP_DERIV, &opderiv_)).ok()) return s;
  if (!(s = read(KZ_SEC_SPAWN_WRAP, &spawnw_)).ok()) return s;
  if (!(s = read(KZ_SEC_READS, &reads_)).ok()) return s;
  if (!(s = read(KZ_SEC_RETURNS, &returns_)).ok()) return s;
  if (!(s = read(KZ_SEC_RET_CONSTS, &retconsts_)).ok()) return s;
  if (!(s = read(KZ_SEC_LOOP_EXITS, &loopexits_)).ok()) return s;
  if (!(s = read(KZ_SEC_ARGFLOW, &argflow_)).ok()) return s;
  if (!(s = read(KZ_SEC_PARAM_FLOW, &pflow_)).ok()) return s;
  if (!(s = read(KZ_SEC_PTR_FIELDS, &ptrfields_)).ok()) return s;
  if (!(s = read(KZ_SEC_GLOBAL_IMAGES, &gimages_)).ok()) return s;
  if (!(s = read(KZ_SEC_LOCK_IDS, &lockids_)).ok()) return s;
  if (!(s = read(KZ_SEC_LOCKSET, &lockset_)).ok()) return s;
  if (!(s = read(KZ_SEC_FIELD_STATUS, &fstatus_)).ok()) return s;
  if (c_.find_section(KZ_SEC_FACT_ATYPE) != nullptr && !(s = read(KZ_SEC_FACT_ATYPE, &atypes_)).ok()) return s;
  if (!(s = read(KZ_SEC_AWAIT_L1, &awaits_)).ok()) return s;
  if (!(s = read(KZ_SEC_LOCK_PATHS, &lockpaths_)).ok()) return s;
  if (!(s = read(KZ_SEC_SEMANTICS, &sem_)).ok()) return s;
  if (!(s = read(KZ_SEC_RESULT_FLOW, &rflow_)).ok()) return s;
  if (!(s = read(KZ_SEC_RESULT_EFFECT, &reffect_)).ok()) return s;
  result_flow_ = c_.find_section(KZ_SEC_RESULT_FLOW) != nullptr && c_.find_section(KZ_SEC_RESULT_EFFECT) != nullptr;
  has_objacc_ = c_.find_section(KZ_SEC_OBJECT_ACCESS) != nullptr;
  for (size_t i = 0; i < rflow_.size(); ++i) rflow_of_site_[rflow_[i].site].push_back(static_cast<uint32_t>(i));
  for (size_t i = 0; i < reffect_.size(); ++i) reffect_of_site_[reffect_[i].site].push_back(static_cast<uint32_t>(i));
  if (!(s = read(KZ_SEC_LOOP_DEFS, &loopdefs_)).ok()) return s;
  if (!(s = read(KZ_SEC_LOCK_HANDLES, &lockh_)).ok()) return s;
  if (!(s = read(KZ_SEC_LOCK_OPS, &lockops_)).ok()) return s;
  loop_defs_ = c_.find_section(KZ_SEC_LOOP_DEFS) != nullptr;
  lock_handles_ = c_.find_section(KZ_SEC_LOCK_HANDLES) != nullptr;
  for (size_t i = 0; i < loopdefs_.size(); ++i) {
    std::string l;
    c_.string(loopdefs_[i].local, &l);
    if (l.empty()) loopdef_head_.emplace(loopdefs_[i].loop, static_cast<uint32_t>(i));
  }
  for (size_t i = 0; i < lockh_.size(); ++i) lockh_of_fn_[lockh_[i].fn].push_back(static_cast<uint32_t>(i));
  return kz::Status::success();
}


// ---------------------------------------------------------------- indices
void RelFacts::index() {
  for (const kz_basis_item& b : basis_) basis_map_[str(b.name)] = str(b.value);
  for (const kz_basis_item& b : basis_)
    if (b.policy == KZ_POLICY_DECLARED && b.prov != kN) {
      const std::string n = str(b.name);
      if (n.compare(0, 3, "op.") == 0 || n.compare(0, 7, "target.") == 0) ++hand_entered_vocabulary_;
    }
  profile_free_ = basis_map_.count("extract.mode") != 0 && basis_map_["extract.mode"].compare(0, 12, "profile-free") == 0;
  for (size_t i = 0; i < funcs_.size(); ++i) fn_by_name_[str(funcs_[i].name)].push_back(static_cast<uint32_t>(i));
  succ_.assign(funcs_.size(), {});
  callers_.assign(funcs_.size(), {});
  for (size_t i = 0; i < calls_.size(); ++i) {
    const kz_fact_call& c = calls_[i];
    call_by_site_[c.site] = static_cast<uint32_t>(i);
    if (c.caller_fn < funcs_.size()) calls_of_[c.caller_fn].push_back(static_cast<uint32_t>(i));
    if (c.caller_fn < funcs_.size() && defined(c.callee_fn)) {
      succ_[c.caller_fn].push_back({c.callee_fn, false});
      callers_[c.callee_fn].push_back(static_cast<uint32_t>(i));
    }
  }
  for (const kz_fact_target& t : targets_) {
    auto it = call_by_site_.find(t.site);
    if (it == call_by_site_.end()) continue;
    const uint32_t caller = calls_[it->second].caller_fn;
    if (t.kind == KZ_CALL_TARGET_KIND_INDIRECT && defined(t.fn) && caller < funcs_.size()) {
      succ_[caller].push_back({t.fn, true});
      indirect_callers_[t.fn].push_back(it->second);
      itargets_[t.site].insert(t.fn);
    }
    if (t.kind == KZ_CALL_TARGET_KIND_INDIRECT && !defined(t.fn)) {
      unresolved_sites_.insert(t.site);
      unresolved_why_[t.site] = enum_text("KZ_CALL_TARGET_REASON", t.reason);
    }
    if (t.kind == KZ_CALL_TARGET_KIND_SPAWN_ENTRY) spawn_entries_[t.site].insert(t.fn);
  }
  // an indirect call with no resolved target, or with a target the flow graph does not resolve, may call any function
  for (const kz_fact_call& c : calls_)
    if (c.kind == KZ_CALL_KIND_INDIRECT && itargets_.count(c.site) == 0 && unresolved_sites_.insert(c.site).second) unresolved_why_[c.site] = "no-function-flows-to-it";
  for (auto& v : succ_) {
    std::sort(v.begin(), v.end());
    v.erase(std::unique(v.begin(), v.end()), v.end());
  }
  for (const kz_fact_iter& it : iters_)
    if (it.spawn != KZ_ITER_SPAWN_NONE && it.site != kN && defined(it.fn)) {
      spawn_entries_[it.site].insert(it.fn);
      iter_spawn_[it.site][it.fn] = it.iter;
      iter_loop_[it.site] = it.loop;
    }
  for (const kz_fact_spawnw& w : spawnw_)
    if (w.kind == KZ_SPAWNW_KIND_SCRIPT_ENTRY) {
      spawn_entries_[w.site].insert(w.entry_fn);
      if (defined(w.entry_fn)) script_entries_.insert(w.entry_fn);
    }
  for (size_t i = 0; i < argflow_.size(); ++i) argflow_at_[{argflow_[i].site, argflow_[i].arg_index}] = static_cast<uint32_t>(i);
  for (size_t i = 0; i < reads_.size(); ++i) reads_of_field_[str(reads_[i].type) + "." + str(reads_[i].field)].push_back(static_cast<uint32_t>(i));
  for (size_t i = 0; i < returns_.size(); ++i) returns_of_[returns_[i].fn].push_back(static_cast<uint32_t>(i));
  for (size_t i = 0; i < retconsts_.size(); ++i) retconsts_of_[retconsts_[i].fn].push_back(static_cast<uint32_t>(i));
  for (size_t i = 0; i < loopexits_.size(); ++i) loopexits_of_[loopexits_[i].loop].push_back(static_cast<uint32_t>(i));
  for (size_t i = 0; i < pflow_.size(); ++i) pflow_of_[{pflow_[i].fn, pflow_[i].param}].push_back(static_cast<uint32_t>(i));
  for (size_t i = 0; i < lockset_.size(); ++i) {
    const kz_fact_lockset& r = lockset_[i];
    if (r.kind == KZ_LOCKSET_KIND_CALL) lockset_call_[r.site] = static_cast<uint32_t>(i);
    else if (r.kind == KZ_LOCKSET_KIND_READ) lockset_read_[r.read] = static_cast<uint32_t>(i);
    else if (r.kind == KZ_LOCKSET_KIND_WRITE) lockset_writes_of_[r.fn].push_back(static_cast<uint32_t>(i));
  }
  for (size_t i = 0; i < fstatus_.size(); ++i) fstatus_of_[str(fstatus_[i].type) + "." + str(fstatus_[i].field)] = static_cast<uint32_t>(i);
  for (size_t i = 0; i < atypes_.size(); ++i) atypes_of_[str(atypes_[i].loc_type) + "." + str(atypes_[i].loc_field)].push_back(static_cast<uint32_t>(i));
  for (size_t i = 0; i < writers_.size(); ++i) writers_of_[str(writers_[i].loc_type) + "." + str(writers_[i].loc_field)].push_back(static_cast<uint32_t>(i));
  for (size_t i = 0; i < inits_.size(); ++i) inits_of_[str(inits_[i].loc_type) + "." + str(inits_[i].loc_field)].push_back(static_cast<uint32_t>(i));
  for (size_t i = 0; i < fwrites_.size(); ++i)
    if (fwrites_[i].kind != KZ_FWRITE_KIND_UNTYPED) fwrites_of_[str(fwrites_[i].type) + "." + str(fwrites_[i].field)].push_back(static_cast<uint32_t>(i));
  for (size_t i = 0; i < terminals_.size(); ++i) terminals_of_[terminals_[i].fn].push_back(static_cast<uint32_t>(i));
  for (size_t i = 0; i < opderiv_.size(); ++i) opderiv_of_[opderiv_[i].fn].push_back(static_cast<uint32_t>(i));
  for (size_t i = 0; i < ptrfields_.size(); ++i)
    if (ptrfields_[i].kind == KZ_PTRF_KIND_READ) ptrread_of_[ptrfields_[i].read] = static_cast<uint32_t>(i);
  for (size_t i = 0; i < gimages_.size(); ++i) gimages_of_[base_name(str(gimages_[i].object))].push_back(static_cast<uint32_t>(i));
  for (size_t i = 0; i < loops_.size(); ++i) loop_by_index_[loops_[i].loop] = static_cast<uint32_t>(i);
  for (size_t i = 0; i < awaits_.size(); ++i) awaits_of_[awaits_[i].fn].push_back(static_cast<uint32_t>(i));
  for (const kz_fact_p0& p : p0_) {
    p0_kind_[str(p.name)] = p.kind;
    if (str(p.source) == "unverified") p0_unverified_.insert(str(p.name));
  }
  for (const P0CompareEntry& e : p0_compare_table()) {
    compare_names_.insert(e.name);
    if (std::string(e.source) == "unverified") p0_unverified_.insert(e.name);
  }
  // the lock primitives of P0 the program calls: the lock sets rest on their semantics
  for (const kz_fact_call& c : calls_) {
    if (defined(c.callee_fn)) continue;
    auto pk = p0_kind_.find(str(c.callee));
    if (pk != p0_kind_.end() && (pk->second == KZ_P0_KIND_MUTEX_ACQUIRE || pk->second == KZ_P0_KIND_MUTEX_RELEASE || pk->second == KZ_P0_KIND_COND_WAIT))
      lock_prims_.insert(pk->first);
  }
  // the P0 primitive a startup-script entry is resolved through (the symbol lookup the spawn operation reaches)
  for (const kz_fact_spawnw& w : spawnw_)
    if (w.kind == KZ_SPAWNW_KIND_SCRIPT_ENTRY && defined(w.entry_fn)) {
      auto cit = call_by_site_.find(w.call_site);
      if (cit != call_by_site_.end()) script_prim_[w.entry_fn].insert(str(calls_[cit->second].callee));
    }
  for (const kz_fact_lockpath& p : lockpaths_) {
    if (p.kind == KZ_LOCKPATH_KIND_AFTER_SPAWN) after_spawn_[p.site].insert(p.row);
    else if (p.kind == KZ_LOCKPATH_KIND_ENTRY_RELEASE && entry_release_.count(p.row) == 0) entry_release_[p.row] = p.site;
  }
  // the deviations of the basis the extraction copied (extract.basis_deviation[<item>]); a unit's own deviations
  // (an item whose name has the unit's file in its first [...]) mark the rows that name a position of that file
  const std::string dev = "extract.basis_deviation[";
  for (const auto& [k, v] : basis_map_) {
    if (k.compare(0, dev.size(), dev) != 0 || k.back() != ']') continue;
    const std::string item = k.substr(dev.size(), k.size() - dev.size() - 1);
    deviations_.push_back(item);
    const size_t a = item.find('['), b = a == std::string::npos ? std::string::npos : item.find(']', a);
    if (b != std::string::npos) deviation_files_[item.substr(a + 1, b - a - 1)].insert(item);
  }
  for (const kz_basis_item& b : basis_)
    if (b.policy == KZ_POLICY_DECLARED && str(b.name).compare(0, 7, "assume.") == 0) assumptions_.insert(str(b.name));
  // the calls that may call a function back: a function passed as an argument to a callee without a body that is no
  // P0 entry (the rule of call_runs), by the function passed
  for (size_t ci = 0; ci < calls_.size(); ++ci)
    for (uint32_t f : callback_args(static_cast<uint32_t>(ci))) callback_callers_[f].push_back(static_cast<uint32_t>(ci));
  // the calls of a function that returns twice (a non-local jump may come back to them), by caller
  for (size_t ci = 0; ci < calls_.size(); ++ci) {
    const kz_fact_call& c = calls_[ci];
    if (defined(c.callee_fn) || !defined(c.caller_fn)) continue;
    for (const char* n : kReturnsTwice)
      if (str(c.callee) == n) returns_twice_[c.caller_fn].push_back(static_cast<uint32_t>(ci));
  }
  thread_end_code();
  global_address_functions();
  image_entries();
}


// The code of the program a thread may run after its entry function has returned: the destructors of the
// thread-specific keys (P0 key creation with a destructor argument that is not a null constant) and the cleanup
// handlers (P0 cleanup push; conservatively every push of the program).
void RelFacts::thread_end_code() {
  for (size_t ci = 0; ci < calls_.size(); ++ci) {
    const kz_fact_call& c = calls_[ci];
    if (defined(c.callee_fn)) continue;
    const std::string callee = str(c.callee);
    for (const kz_fact_p0& p : p0_) {
      if (str(p.name) != callee) continue;
      if (p.kind == KZ_P0_KIND_TSD_KEY_CREATE && p.entry_param != kN) {
        auto af = argflow_at_.find({c.site, p.entry_param});
        const bool null = af != argflow_at_.end() && argflow_[af->second].src == KZ_ARGSRC_CONST && argflow_[af->second].value == 0;
        (null ? keys_null_dtor_ : keys_dtor_)
            .push_back(callee + " at " + call_pos(static_cast<uint32_t>(ci)) + (af != argflow_at_.end() ? " (arg " + std::to_string(p.entry_param) + " [" + str(argflow_[af->second].text) + "])" : ""));
      }
      if (p.kind == KZ_P0_KIND_CLEANUP_PUSH) cleanup_pushes_.push_back(callee + " at " + call_pos(static_cast<uint32_t>(ci)));
    }
  }
}


// The entries of a spawn call whose entry argument is read from a global table with a static image: the function
// leaves of that image at the field the argument reads (GLOBAL_IMAGES), for the spawn calls the facts leave without
// a resolved entry.  The entry argument is the P0 entry parameter of a thread-creation primitive, or the parameter a
// derived spawn operation passes on (SPAWN_WRAP).  No creation order is read from the image.  The image is the run-time
// value only when nothing writes the member: the member's writer set must be complete with no writer instance under
// both completeness rules of FIELD_STATUS (typed, data-function and out-pointer writes; an escape, a cast, a union or
// an unknown effect makes it not complete), and no leaf used may be marked as written (object-, field- or
// record-written: a store to the object, the field or a whole record of its type); otherwise the spawn stays
// unresolved, with the reason.
// Why the static image of a member is not taken as its run-time value ("" when it is): a leaf of the image is marked
// written (object-, field- or record-written, GLOBAL_IMAGES), FIELD_STATUS has no row of the member, its writer set
// is not complete under both rules, or it has writers at run time.  The rule of the static-image entries and of the
// image keys of path A.
std::string RelFacts::image_value_why(const std::string& type, const std::string& field, uint16_t written, const std::string& cut) const {
  if (!cut.empty()) return "the static image was cut at the image budget (marker leaves " + cut + "): the part not read is not known";
  if (written != 0) return "a leaf of the image is marked " + written_flags(written) + " (GLOBAL_IMAGES): the run-time value may differ from the image";
  auto fsit = fstatus_of_.find(type + "." + field);
  if (fsit == fstatus_of_.end()) return "FIELD_STATUS has no row of " + type + "." + field + ": that nothing writes it is not known";
  const kz_fact_fstatus& f = fstatus_[fsit->second];
  if (f.status_field != KZ_LOC_STATUS_COMPLETE || f.status_type != KZ_LOC_STATUS_COMPLETE)
    return "the writer set of " + type + "." + field + " is not complete (FIELD_STATUS per-field " + enum_text("KZ_LOC_STATUS", f.status_field) + ", type-level " +
           enum_text("KZ_LOC_STATUS", f.status_type) + ")";
  if (f.n_direct + f.n_data_fn + f.n_outptr != 0)
    return type + "." + field + " has writers at run time (FIELD_STATUS: direct " + std::to_string(f.n_direct) + ", data function " + std::to_string(f.n_data_fn) +
           ", out-pointer " + std::to_string(f.n_outptr) + ")";
  return "";
}

std::string RelFacts::image_cut(const std::string& object) const {
  auto git = gimages_of_.find(object);
  if (git == gimages_of_.end()) return "";
  std::vector<std::string> markers;
  bool flagged = false;
  for (uint32_t gi : git->second) {
    const kz_fact_gimage& g = gimages_[gi];
    if (g.flags & KZ_GIMAGE_FLAG_TRUNCATED) flagged = true;
    if (g.kind == KZ_IMAGE_KIND_NONCONST && str(g.text) == "<image truncated>") markers.push_back(object + str(g.path));
  }
  if (!markers.empty()) return join(markers, ", ");
  return flagged ? object + " (image-truncated)" : "";
}

void RelFacts::image_entries() {
  std::map<uint32_t, uint32_t> entry_param_of;  // spawn operation (function) -> its entry parameter
  for (const kz_fact_spawnw& w : spawnw_)
    if (w.param != kN && (w.kind == KZ_SPAWNW_KIND_ENTRY_ARG || w.kind == KZ_SPAWNW_KIND_CALLED_BY_ENTRY || w.kind == KZ_SPAWNW_KIND_SCRIPT_ENTRY) &&
        entry_param_of.count(w.fn) == 0)
      entry_param_of[w.fn] = w.param;
  for (size_t ci = 0; ci < calls_.size(); ++ci) {
    const kz_fact_call& c = calls_[ci];
    if ((c.flags & KZ_CALL_FLAG_SPAWN_OP) == 0 || !defined(c.caller_fn)) continue;
    auto se = spawn_entries_.find(c.site);
    bool resolved = false;
    if (se != spawn_entries_.end())
      for (uint32_t e : se->second)
        if (defined(e)) resolved = true;
    if (resolved) continue;
    uint32_t k = kN;
    if (defined(c.callee_fn)) {
      auto it = entry_param_of.find(c.callee_fn);
      if (it != entry_param_of.end()) k = it->second;
    } else {
      for (const kz_fact_p0& p : p0_)
        if (str(p.name) == str(c.callee) && p.kind == KZ_P0_KIND_THREAD_CREATE) k = p.entry_param;
    }
    const std::string where = call_pos(static_cast<uint32_t>(ci)) + " in " + fname(c.caller_fn) + " -> " + str(c.callee);
    if (k == kN) {
      unresolved_spawns_.push_back(where + ": no entry parameter of the spawn operation is known");
      continue;
    }
    auto af = argflow_at_.find({c.site, k});
    if (af == argflow_at_.end()) {
      unresolved_spawns_.push_back(where + ": no ARGFLOW row of the entry argument " + std::to_string(k));
      continue;
    }
    const kz_fact_argflow& a = argflow_[af->second];
    const std::string object = base_name(str(a.object)), field = str(a.field), type = str(a.type);
    if (a.src != KZ_ARGSRC_FIELD || object.empty() || field.empty() || (a.base != KZ_ARGBASE_GLOBAL && a.base != KZ_ARGBASE_GLOBAL_ELEMENT)) {
      unresolved_spawns_.push_back(where + ": the entry argument " + std::to_string(k) + " [" + str(a.text) + "] is no field of a global table");
      continue;
    }
    auto git = gimages_of_.find(object);
    std::set<uint32_t> found;
    std::set<std::string> others;  // the members of the image leaves of functions whose name is not the field read
    std::map<uint32_t, std::set<std::string>> paths;  // entry -> its image paths
    uint16_t written = 0;
    if (git != gimages_of_.end())
      for (uint32_t gi : git->second) {
        const kz_fact_gimage& g = gimages_[gi];
        if (g.kind != KZ_IMAGE_KIND_FUNC || !defined(g.fn)) continue;
        const std::string path = str(g.path), owner = str(g.owner), gfield = str(g.field);
        const size_t dot = path.rfind('.');
        if (dot == std::string::npos) continue;
        if (path.substr(dot + 1) != field) {
          others.insert(path.substr(dot + 1));
          continue;
        }
        if (!gfield.empty() && (gfield != field || owner != type)) continue;
        found.insert(g.fn);
        paths[g.fn].insert(path);
        written |= g.flags & (KZ_GIMAGE_FLAG_OBJECT_WRITTEN | KZ_GIMAGE_FLAG_FIELD_WRITTEN | KZ_GIMAGE_FLAG_RECORD_WRITTEN);
      }
    // relations rules revision 6: an image cut at the image budget gives the entries of the part read, not all of them
    const std::string cut = image_cut(object);
    if (found.empty()) {
      std::string o;
      for (const std::string& x : others) o += (o.empty() ? "" : ", ") + x;
      unresolved_spawns_.push_back(where + ": the entry argument " + std::to_string(k) + " [" + str(a.text) + "] reads " + type + "." + field + " of " + object +
                                   ", and the static image has no function leaf of that member" +
                                   (cut.empty() ? "" : " in the part read: the image was cut at the image budget (marker leaves " + cut + ")") +
                                   (o.empty() ? "" : " (its function leaves are of " + o + ": a read through another member of a union is not resolved in this revision)"));
      continue;
    }
    // the image is the run-time value: no write of the member (a cut is not passed here: it does not make the leaves read
    // wrong, it makes the set of entries not all, image_cut_)
    const std::string why = image_value_why(type, field, written);
    if (!why.empty()) {
      unresolved_spawns_.push_back(where + ": the entry argument " + std::to_string(k) + " [" + str(a.text) + "] reads " + type + "." + field + " of " + object +
                                   ", whose static image is not taken as its value: " + why);
      continue;
    }
    image_spawn_[c.site] = found;
    image_member_[c.site] = type + "." + field;
    if (!cut.empty())
      image_cut_[c.site] = "the static image of " + object + " was cut at the image budget (marker leaves " + cut +
                           "): the entries of the part not read are not found";
    // relations rules revision 5: the elements of the image whose member is not a function leaf of the member read
    // (a non-constant leaf covering it, or a function leaf of another member of its record: a union may hold that
    // value) give the member read another value; that the spawn reads only the elements found is the spawn's guard,
    // which this rule does not read, so the threads of the spawn rest on it
    {
      auto shape = [](const std::string& p) {
        std::string out;
        for (size_t i = 0; i < p.size(); ++i) {
          if (p[i] == '[') {
            const size_t j = p.find(']', i);
            if (j == std::string::npos) break;
            out += "[]";
            i = j;
            continue;
          }
          out += p[i];
        }
        return out;
      };
      std::set<std::string> containers;  // the shape of the path up to the member read ("[].FuncPtrUnion")
      for (const auto& [e, ps] : paths)
        for (const std::string& pth : ps) containers.insert(shape(pth.substr(0, pth.rfind('.'))));
      std::vector<std::string> nonconst, sibling;
      size_t n_nonconst = 0, n_sibling = 0;
      for (uint32_t gi : git->second) {
        const kz_fact_gimage& g = gimages_[gi];
        const std::string pth = str(g.path), sh = shape(pth);
        if (g.kind == KZ_IMAGE_KIND_NONCONST && str(g.text) == "<image truncated>") continue;  // a marker is no value (image_cut_)
        if (g.kind == KZ_IMAGE_KIND_NONCONST) {
          bool covers = false;
          for (const std::string& ct : containers)
            if (ct == sh || ct.compare(0, sh.size() + 1, sh + ".") == 0 || ct.compare(0, sh.size() + 1, sh + "[") == 0) covers = true;
          if (covers) {
            ++n_nonconst;
            if (nonconst.size() < 3) nonconst.push_back(object + pth);
          }
        } else if (g.kind == KZ_IMAGE_KIND_FUNC) {
          const size_t d2 = pth.rfind('.');
          if (d2 == std::string::npos || pth.substr(d2 + 1) == field) continue;
          if (containers.count(shape(pth.substr(0, d2))) == 0) continue;
          ++n_sibling;
          if (sibling.size() < 3) sibling.push_back(object + pth + " " + fname(g.fn));
        }
      }
      if (n_nonconst + n_sibling != 0) {
        std::string t = "besides the function leaves of " + type + "." + field + ", the image of " + object + " has";
        if (n_nonconst != 0) t += " " + std::to_string(n_nonconst) + " non-constant leaf/leaves covering that member (" + join(nonconst, ", ") + (n_nonconst > nonconst.size() ? ", ..." : "") + ")";
        if (n_nonconst != 0 && n_sibling != 0) t += " and";
        if (n_sibling != 0)
          t += " " + std::to_string(n_sibling) + " function leaf/leaves of another member of its record (" + join(sibling, ", ") + (n_sibling > sibling.size() ? ", ..." : "") +
               "; a union member read gives that value)";
        t += ": that the spawn reads only the elements whose " + field + " is a function leaf rests on its guard, which this revision does not read";
        image_premise_[c.site] = t;
      }
    }
    for (const auto& [e, ps] : paths) image_paths_[c.site][e] = join(std::vector<std::string>(ps.begin(), ps.end()), ", ");
    for (uint32_t e : found) spawn_entries_[c.site].insert(e);
  }
}


// the functions whose every return value is the address of (a part of) a global object or null
// (target_rules.cpp, the same rule over the records)
void RelFacts::global_address_functions() {
  std::set<std::string> globals;
  for (const kz_fact_read& r : reads_)
    if (r.root_kind == KZ_READ_ROOT_GLOBAL) globals.insert(str(r.root));
  for (const kz_fact_argflow& a : argflow_) {
    const std::string o = str(a.object);
    if (!o.empty()) globals.insert(base_name(o));
  }
  auto innermost = [&](uint32_t r) -> const kz_fact_expr* {
    while (r != kN && r < exprs_.size() && exprs_[r].op != KZ_EXPR_OP_LOAD) r = exprs_[r].a;
    return r != kN && r < exprs_.size() ? &exprs_[r] : nullptr;
  };
  auto written_global = [&](const std::string& text) {
    if (text.size() < 2 || text[0] != '&') return false;
    size_t i = 1;
    while (i < text.size() && (std::isalnum(static_cast<unsigned char>(text[i])) || text[i] == '_')) ++i;
    if (i < text.size() && text[i] != '.' && text[i] != '[') return false;
    return globals.count(text.substr(1, i - 1)) != 0;
  };
  std::map<std::string, std::vector<uint32_t>> by_fn;
  for (size_t i = 0; i < returns_.size(); ++i) by_fn[base_name(fname(returns_[i].fn))].push_back(static_cast<uint32_t>(i));
  for (bool changed = true; changed;) {
    changed = false;
    for (const auto& [fn, rows] : by_fn) {
      if (gaf_.count(fn) != 0) continue;
      bool all = true, address = false;
      for (uint32_t ri : rows) {
        const kz_fact_return& r = returns_[ri];
        if (r.vkind == KZ_RET_VALUE_CONST && r.value == 0) continue;
        if (r.vkind == KZ_RET_VALUE_ADDR) {
          const kz_fact_expr* in = innermost(r.value_root);
          if ((in != nullptr && (in->flags == KZ_EXPR_REF_GLOBAL || in->flags == KZ_EXPR_REF_ELEM)) || written_global(str(r.text))) {
            address = true;
            continue;
          }
        }
        if (r.vkind == KZ_RET_VALUE_CALL && r.value_root < exprs_.size() && exprs_[r.value_root].op == KZ_EXPR_OP_CALL_RESULT &&
            gaf_.count(str(exprs_[r.value_root].name)) != 0) {
          address = true;
          continue;
        }
        all = false;
        break;
      }
      if (all && address) {
        gaf_.insert(fn);
        changed = true;
      }
    }
  }
}

bool RelFacts::shared_base(const kz_fact_argflow& a) const {
  switch (a.base) {
    case KZ_ARGBASE_GLOBAL:
    case KZ_ARGBASE_GLOBAL_ELEMENT:
    case KZ_ARGBASE_GLOBAL_POINTER:
      return true;
    case KZ_ARGBASE_FIELD_POINTER:
      return !str(a.object).empty();
    case KZ_ARGBASE_CALL_RESULT:
      return gaf_.count(str(a.root)) != 0;
    default:
      return false;
  }
}


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
bool RelFacts::own_shared_access(uint32_t r) const {
  for (const kz_fact_read& rd : reads_)
    if (rd.fn == r && (rd.root_kind == KZ_READ_ROOT_GLOBAL || (rd.root_kind == KZ_READ_ROOT_CALL && gaf_.count(str(rd.root)) != 0))) return true;
  auto it = lockset_writes_of_.find(r);
  if (it != lockset_writes_of_.end())
    for (uint32_t li : it->second)
      if (str(lockset_[li].type) == "<global>") return true;
  return false;
}

void RelFacts::actors() {
  for (uint32_t fn : fn_by_name_["main"])
    if (defined(fn)) root_kind_[fn] = "process";
  for (const auto& [site, entries] : spawn_entries_)
    for (uint32_t e : entries)
      if (defined(e) && root_kind_.count(e) == 0)
        root_kind_[e] = script_entries_.count(e) != 0 ? "script-entry"
                        : iter_spawn_.count(site) != 0 ? "table-walk-entry"
                        : image_spawn_.count(site) != 0 ? "table-image-entry"
                                                        : "spawn-entry";
  std::map<uint32_t, std::set<uint32_t>> calls_roots, calls_roots_direct;
  for (const auto& [r, kind] : root_kind_) {
    std::set<uint32_t>& reach = reach_[r];
    std::vector<uint32_t> stack{r};
    reach.insert(r);
    while (!stack.empty()) {
      const uint32_t x = stack.back();
      stack.pop_back();
      for (const auto& [y, indirect] : succ_[x]) {
        if (y != r && root_kind_.count(y) != 0) {
          calls_roots[r].insert(y);
          if (!indirect) calls_roots_direct[r].insert(y);
          continue;
        }
        if (reach.insert(y).second) stack.push_back(y);
      }
    }
  }
  for (const auto& [r, kind] : root_kind_) {
    if (kind == "process" || calls_roots[r].empty()) continue;
    if (!calls_roots_direct[r].empty()) kept_direct_.insert(r);
    else if (own_shared_access(r)) kept_shared_.insert(r);
    else trampolines_.insert(r);
  }
  // the entries resolved at the calls of a derived spawn operation that passes a trampoline to its inner spawn
  for (const kz_fact_spawnw& w : spawnw_) {
    if (w.kind != KZ_SPAWNW_KIND_CALLED_BY_ENTRY || trampolines_.count(w.entry_fn) == 0 || !defined(w.fn)) continue;
    std::vector<uint32_t> in;
    for (uint32_t ci : callers_[w.fn]) in.push_back(ci);
    auto ic = indirect_callers_.find(w.fn);
    if (ic != indirect_callers_.end()) in.insert(in.end(), ic->second.begin(), ic->second.end());
    for (uint32_t ci : in) {
      auto se = spawn_entries_.find(calls_[ci].site);
      if (se == spawn_entries_.end()) {
        via_op_missing_[w.entry_fn].insert(call_pos(ci) + " in " + fname(calls_[ci].caller_fn) + " -> " + fname(w.fn));
        continue;
      }
      bool any = false;
      for (uint32_t e : se->second)
        if (defined(e) && root_kind_.count(e) != 0 && e != w.entry_fn) {
          via_op_[w.entry_fn][e].insert(fname(w.fn));
          any = true;
        }
      if (!any) via_op_missing_[w.entry_fn].insert(call_pos(ci) + " in " + fname(calls_[ci].caller_fn) + " -> " + fname(w.fn));
    }
  }
  // what a trampoline stands for, through other trampolines
  std::function<void(uint32_t, std::set<uint32_t>*, std::set<uint32_t>*)> expand = [&](uint32_t t, std::set<uint32_t>* out, std::set<uint32_t>* seen) {
    if (!seen->insert(t).second) return;
    std::set<uint32_t> ys = calls_roots[t];
    auto vo = via_op_.find(t);
    if (vo != via_op_.end())
      for (const auto& [e, ops] : vo->second) ys.insert(e);
    for (uint32_t y : ys) {
      if (y == t) continue;
      if (trampolines_.count(y) != 0) expand(y, out, seen);
      else out->insert(y);
    }
  };
  for (uint32_t t : trampolines_) {
    std::set<uint32_t> seen;
    expand(t, &delegates_[t], &seen);
  }
  for (const auto& [r, reach] : reach_) {
    std::set<uint32_t> as;
    if (trampolines_.count(r) != 0) as = delegates_[r];
    else as.insert(r);
    for (uint32_t f : reach) actors_of_[f].insert(as.begin(), as.end());
  }
}

std::set<uint32_t> RelFacts::actor_set(uint32_t fn) const {
  auto it = actors_of_.find(fn);
  return it == actors_of_.end() ? std::set<uint32_t>{} : it->second;
}

// the actors a set of spawned entries stands for
std::set<uint32_t> RelFacts::entry_actors(const std::set<uint32_t>& entries) const {
  std::set<uint32_t> out;
  for (uint32_t e : entries) {
    if (!defined(e)) continue;
    auto it = delegates_.find(e);
    if (trampolines_.count(e) != 0 && it != delegates_.end()) out.insert(it->second.begin(), it->second.end());
    else out.insert(e);
  }
  return out;
}

// the functions a call may run: its defined callee, its resolved indirect targets, and the functions passed as
// arguments to a callee without a body that is no P0 entry (it may call them back)
std::set<uint32_t> RelFacts::call_runs(uint32_t ci) const {
  std::set<uint32_t> t;
  const kz_fact_call& c = calls_[ci];
  if (defined(c.callee_fn)) t.insert(c.callee_fn);
  auto itg = itargets_.find(c.site);
  if (itg != itargets_.end()) t.insert(itg->second.begin(), itg->second.end());
  const std::set<uint32_t> cb = callback_args(ci);
  t.insert(cb.begin(), cb.end());
  return t;
}

// the defined functions a call passes as arguments to a callee without a body that is no P0 entry (it may call them
// back during the call)
std::set<uint32_t> RelFacts::callback_args(uint32_t ci) const {
  std::set<uint32_t> t;
  const kz_fact_call& c = calls_[ci];
  const std::string callee = str(c.callee);
  if (defined(c.callee_fn) || c.kind == KZ_CALL_KIND_INDIRECT || p0_kind_.count(callee) != 0 || compare_names_.count(callee) != 0) return t;
  for (uint32_t k = 0; k < c.nargs; ++k) {
    auto af = argflow_at_.find({c.site, k});
    if (af == argflow_at_.end() || argflow_[af->second].src != KZ_ARGSRC_FUNCTION) continue;
    auto fit = fn_by_name_.find(base_name(str(argflow_[af->second].root)));
    if (fit != fn_by_name_.end())
      for (uint32_t f : fit->second)
        if (defined(f)) t.insert(f);
  }
  return t;
}
}  // namespace kzs

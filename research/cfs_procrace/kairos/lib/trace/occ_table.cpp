// occ_table.cpp -- the per-occurrence table of a replay output container and its comparison (see occ_table.hpp).

#include "kz/trace/occ_table.hpp"

#include <algorithm>
#include <cstdio>
#include <map>
#include <set>

#include "kz/core/registry.hpp"

namespace kz::trace {

namespace {

std::string enum_text(const char* en, uint64_t v) {
  const EnumDesc* d = find_enum(en);
  const char* t = d != nullptr ? enum_display(*d, v) : nullptr;
  return t != nullptr ? t : "#" + std::to_string(v);
}

std::string flags_text(const char* en, uint64_t v) {
  const EnumDesc* d = find_enum(en);
  std::string out;
  uint64_t left = v;
  for (size_t i = 0; d != nullptr && i < d->nitems; ++i)
    if ((v & d->items[i].value) != 0) {
      out += (out.empty() ? "" : ",") + std::string(d->items[i].display);
      left &= ~d->items[i].value;
    }
  if (left != 0) {
    char buf[24];
    std::snprintf(buf, sizeof buf, "0x%llx", static_cast<unsigned long long>(left));
    out += (out.empty() ? "" : ",") + std::string(buf);
  }
  return "{" + out + "}";
}

// The basis kinds and assumptions of summary.flags, as KZ_OCC_BASIS / KZ_OCC_ASSUME bits (a container without OCC_BASIS).
uint32_t basis_of_flags(uint16_t f) {
  uint32_t b = 0;
  if ((f & KZ_SUMMARY_FLAG_BASIS_DIRECT_CAPTURE) != 0) b |= KZ_OCC_BASIS_DIRECT_CAPTURE;
  if ((f & KZ_SUMMARY_FLAG_BASIS_STATE_ORDER) != 0) b |= KZ_OCC_BASIS_STATE_ORDER;
  if ((f & KZ_SUMMARY_FLAG_BASIS_RETURN_DERIVED_ORDER) != 0) b |= KZ_OCC_BASIS_RETURN_DERIVED_ORDER;
  if ((f & KZ_SUMMARY_FLAG_OUTCOME_NARROWED) != 0) b |= KZ_OCC_BASIS_OUTCOME_NARROWED;
  return b;
}
uint32_t assumptions_of_flags(uint16_t f) {
  uint32_t a = 0;
  if ((f & KZ_SUMMARY_FLAG_ASSUMES_VIA_SEPARATION) != 0) a |= KZ_OCC_ASSUME_VIA_KEY_SEPARATION;
  if ((f & KZ_SUMMARY_FLAG_ASSUMES_OWNER_IDENTITY) != 0) a |= KZ_OCC_ASSUME_OWNER_IDENTITY_TARGETS;
  if ((f & KZ_SUMMARY_FLAG_ASSUMES_ROLE) != 0) a |= KZ_OCC_ASSUME_ROLE;
  if ((f & KZ_SUMMARY_FLAG_BASIS_RETURN_DERIVED_ORDER) != 0) a |= KZ_OCC_ASSUME_READS_FROM;
  return a;
}

std::string verdict_text(const OccRow& r) {
  std::string s = enum_text("KZ_OCC", r.status);
  if (r.status_flags != 0) s += flags_text("KZ_STATUS_FLAG", r.status_flags);
  s += " " + enum_text("KZ_OBS", r.observation);
  if (r.obs_reasons != 0) s += flags_text("KZ_WITHHOLD", r.obs_reasons);
  return s;
}

// The rules of an occurrence's edges, each with its kind, sorted (event positions left out: they depend on the record).
std::string rules_text(const OccRow& r) {
  std::map<std::string, int> n;
  for (const OccEdge& e : r.edges) ++n[e.rule + "[" + enum_text("KZ_OCC_EDGE_KIND", e.kind) + ((e.flags & KZ_OCC_EDGE_FLAG_BY_STORE) != 0 ? ",by-store" : "") + "]"];
  std::string out;
  for (const auto& kv : n) out += (out.empty() ? "" : ",") + kv.first + (kv.second > 1 ? "x" + std::to_string(kv.second) : "");
  return "{" + out + "}";
}

std::string basis_text(const OccRow& r) {
  if (!r.recorded) return "basis " + flags_text("KZ_OCC_BASIS", r.basis) + " (summary.flags only: store-derived-order, control-order, edges not recorded)";
  std::string s = "basis " + flags_text("KZ_OCC_BASIS", r.basis) + " assume " + flags_text("KZ_OCC_ASSUME", r.assumptions);
  if (!r.via.empty()) s += " via=" + r.via;
  if (!r.context.empty()) s += " role=" + r.context;
  s += " outcome " + enum_text("KZ_OUTCOME_CHECK", r.outcome);
  return s;
}


// The owning application of the executing task is established (inferred or confirmed) where the record gives its record
// key.  Not established: undetermined, or no record key -- the writer gives a key only where the level is inferred or
// confirmed and the owner's id maps to a record key (replay_container.cpp, real_trace.cpp owners_unresolved), so an
// owner the record names whose id did not map reads as undetermined here too.
bool owner_established(const OccRow& r) { return r.owner_key != KZ_NONE32 && r.owner_level != KZ_OWNER_LEVEL_UNDETERMINED; }

// The owner as the comparison saw it, with the name the record gives (display only: kz_raw.h ACTOR_OWNERS app_name).
std::string owner_text(const OccRow& r, bool with_task) {
  const std::string name = r.owner_app.empty() ? std::string() : " (" + r.owner_app + ")";
  if (!owner_established(r)) return "owner not established (" + enum_text("KZ_OWNER_LEVEL", r.owner_level) + (r.owner_key == KZ_NONE32 ? ", no record key)" : ")") + name;
  std::string s = "owner " + std::to_string(r.owner_key) + " " + enum_text("KZ_OWNER_LEVEL", r.owner_level);
  if (with_task) s += r.task != 0 ? " task " + std::to_string(r.task) : " task not established";
  return s + name;
}

// Two rows of one key without the owner (the second pass): how their owners stand.  same: both established, the same
// record key and task place (or the task not used); open: established on one side only; neither: established on
// neither side; task_open: the same record key, the task's place established on one side only; task_differ: the same
// record key, both task places established and different -- a place is a rank among the creation requests the record
// has (real_trace.cpp task_ordinal), so a request missing on one side renumbers the later tasks: not established as
// different tasks; differ: both established, different record keys.  Only differ is never the same occurrence.
enum class OwnerRel { same, open, neither, task_open, task_differ, differ };
OwnerRel owner_relation(const OccRow& x, const OccRow& y, bool with_task) {
  const bool ex = owner_established(x), ey = owner_established(y);
  if (!ex && !ey) return OwnerRel::neither;
  if (!ex || !ey) return OwnerRel::open;
  if (x.owner_key != y.owner_key) return OwnerRel::differ;
  if (!with_task || x.task == y.task) return OwnerRel::same;
  return x.task != 0 && y.task != 0 ? OwnerRel::task_differ : OwnerRel::task_open;
}

// The names the record gives the two owners, where both give one and they differ.  A name is display only, never a key
// (kz_raw.h ACTOR_OWNERS app_name): it does not establish two owners as different, but two names that differ do not let
// a key or an ordinal make the two one occurrence.
std::string names_differ(const OccRow& x, const OccRow& y) {
  if (x.owner_app.empty() || y.owner_app.empty() || x.owner_app == y.owner_app) return std::string();
  return " owner name A {" + x.owner_app + "} B {" + y.owner_app + "};";
}

// What the record says of the call beyond the key without the owner: the role, the via names, the text arguments, and
// the owner's name.  Where one of them differs, an equal ordinal (the k-th occurrence of its task) does not make the
// two one call.  (In a full-key pair the role and the via names are basis fields, compared after pairing.)
std::string descriptors_differ(const OccRow& x, const OccRow& y) {
  std::string d;
  if (x.context != y.context) d += " role A {" + x.context + "} B {" + y.context + "};";
  if (x.via != y.via) d += " via A {" + x.via + "} B {" + y.via + "};";
  if (x.name != y.name) d += " args A {" + x.name + "} B {" + y.name + "};";
  return d + names_differ(x, y);
}

bool reading_differs(const OccTable& a, const OccTable& b) {
  if (!a.has_reading || !b.has_reading) return false;
  const kz_replay_read& x = a.reading;
  const kz_replay_read& y = b.reading;
  return x.outcome_narrowing != y.outcome_narrowing || x.owner_identity_targets != y.owner_identity_targets || x.reads_from_edges != y.reads_from_edges ||
         x.reads_from_outcome != y.reads_from_outcome || x.control_order != y.control_order || x.relation_rule != y.relation_rule;
}

}  // namespace

Status read_occ_table(const Container& c, OccTable* out) {
  *out = OccTable();
  if (c.header().artifact_kind != KZ_ARTIFACT_VERDICTS) return Status::error(Err::Invalid, "not a verdicts container");
  std::vector<kz_summary> sums;
  std::vector<kz_identity> ids;
  std::vector<kz_basis_item> items;
  Status s = c.read_section(KZ_SEC_SUMMARIES, &sums);
  if (s.ok() && c.find_section(KZ_SEC_IDENTITIES) != nullptr) s = c.read_section(KZ_SEC_IDENTITIES, &ids);
  if (s.ok() && c.find_section(KZ_SEC_BASIS) != nullptr) s = c.read_section(KZ_SEC_BASIS, &items);
  if (!s.ok()) return s;
  for (const kz_basis_item& b : items) {
    std::string n, v;
    c.string(b.name, &n);
    c.string(b.value, &v);
    out->basis.push_back({n, v});
  }
  std::vector<kz_occ_basis> occ;
  std::vector<kz_occ_edge> edges;
  std::vector<kz_replay_read> reading;
  out->recorded = c.find_section(KZ_SEC_OCC_BASIS) != nullptr;
  if (out->recorded) {
    s = c.read_section(KZ_SEC_OCC_BASIS, &occ);
    if (s.ok() && c.find_section(KZ_SEC_OCC_EDGES) != nullptr) s = c.read_section(KZ_SEC_OCC_EDGES, &edges);
    if (s.ok() && c.find_section(KZ_SEC_REPLAY_READ) != nullptr) s = c.read_section(KZ_SEC_REPLAY_READ, &reading);
    if (!s.ok()) return s;
    if (occ.size() != sums.size()) return Status::error(Err::Invalid, "OCC_BASIS has " + std::to_string(occ.size()) + " records for " + std::to_string(sums.size()) + " summaries");
    if (!reading.empty()) {
      out->has_reading = true;
      out->reading = reading.front();
      out->task_recorded = out->reading.format >= 2;
    }
  }
  auto str = [&](const kz_str& v) {
    std::string t;
    c.string(v, &t);
    return t;
  };
  for (size_t i = 0; i < sums.size(); ++i) {
    const kz_summary& sm = sums[i];
    OccRow r;
    r.index = static_cast<uint32_t>(i);
    r.status = sm.status;
    r.status_flags = sm.status_flags;
    r.observation = sm.observation;
    r.obs_reasons = sm.obs_reasons;
    r.relation = sm.relation;
    r.summary_flags = sm.flags;
    r.basis = basis_of_flags(sm.flags);
    r.assumptions = assumptions_of_flags(sm.flags);
    if (sm.identity_first < ids.size()) {
      const kz_identity& id = ids[sm.identity_first];
      r.exec = str(id.exec_name);
      r.owner_level = id.owner_level;
      r.owner_key = id.owner_key;
      r.owner_app = str(id.owner_name);
    }
    if (out->recorded) {
      const kz_occ_basis& b = occ[i];
      if (b.summary != i) return Status::error(Err::Invalid, "OCC_BASIS record " + std::to_string(i) + " names summary " + std::to_string(b.summary));
      r.recorded = true;
      r.rspec = str(b.rspec);
      r.op = str(b.op);
      r.context = str(b.context);
      r.resource = str(b.resource);
      r.name = str(b.name);
      r.owner_app = str(b.owner_app);
      r.via = str(b.via);
      r.ordinal = b.ordinal;
      r.task = b.task;
      r.task_recorded = out->task_recorded;
      r.owner_key = b.owner_key;
      r.owner_level = b.owner_level;
      r.basis = b.basis;
      r.assumptions = b.assumptions;
      r.outcome = b.outcome;
      r.call = b.call;
      r.decision = b.decision;
      if (b.status != sm.status || b.observation != sm.observation || b.obs_reasons != sm.obs_reasons)
        return Status::error(Err::Invalid, "OCC_BASIS record " + std::to_string(i) + " disagrees with its summary");
      if (static_cast<uint64_t>(b.first_edge) + b.n_edges > edges.size()) return Status::error(Err::Invalid, "OCC_BASIS record " + std::to_string(i) + ": edges out of range");
      for (uint32_t k = 0; k < b.n_edges; ++k) {
        const kz_occ_edge& e = edges[b.first_edge + k];
        r.edges.push_back(OccEdge{e.from, e.to, e.kind, e.flags, str(e.rule)});
      }
    }
    out->rows.push_back(std::move(r));
  }
  return Status::success();
}

std::string occ_key(const OccRow& r, bool with_owner, bool with_task) {
  if (!r.recorded) return "#" + std::to_string(r.index);
  const bool owned = r.owner_key != KZ_NONE32 && r.owner_level != KZ_OWNER_LEVEL_UNDETERMINED;
  std::string owner = owned ? "app[" + std::to_string(r.owner_key) + "]" : "app:undetermined";
  if (owned && with_task && r.task_recorded && r.task != 0) owner += "#" + std::to_string(r.task);
  return r.rspec + " | " + r.op + (with_owner ? " | " + owner : std::string()) + " | " + (r.resource.empty() ? "name:" + r.name : r.resource) + " | " +
         std::to_string(r.ordinal);
}

std::string occ_table_text(const OccTable& t) {
  std::string out = "occurrences " + std::to_string(t.rows.size()) + "; per-occurrence basis " +
                    (t.recorded ? std::string("recorded (OCC_BASIS, schema revision 4)") : std::string("not recorded (a container written before schema revision 4: SUMMARIES only)")) +
                    (t.recorded ? (t.task_recorded ? "; task in the owner: recorded (app[<key>]#<task>)" : "; task in the owner: not recorded (REPLAY_READ format 1)") : "") + "\n";
  if (t.has_reading) {
    const kz_replay_read& r = t.reading;
    out += "reading: outcome-narrowing " + enum_text("KZ_READ_SWITCH", r.outcome_narrowing) + ", owner-identity-targets " + enum_text("KZ_READ_SWITCH", r.owner_identity_targets) +
           ", reads-from-edges " + enum_text("KZ_READ_SWITCH", r.reads_from_edges) + ", reads-from-outcome " + enum_text("KZ_READ_SWITCH", r.reads_from_outcome) +
           ", control-order " + enum_text("KZ_READ_SWITCH", r.control_order) + ", relation " + enum_text("KZ_RELATION_RULE", r.relation_rule) + "; control edges " +
           std::to_string(r.control_edges) + ", events moved by the linearization " + std::to_string(r.reordered) + "\n";
  } else {
    out += "reading: not recorded (the BASIS items below are what the writer was given)\n";
  }
  for (const auto& b : t.basis) out += "basis item " + b.first + " = " + b.second + "\n";
  for (const OccRow& r : t.rows) {
    out += "#" + std::to_string(r.index) + " " + occ_key(r) + " | exec " + (r.exec.empty() ? "?" : r.exec) + (r.owner_app.empty() ? "" : " app " + r.owner_app) + " | " +
           verdict_text(r) + " | " + basis_text(r);
    if (r.recorded) {
      if (!r.name.empty()) out += " | args " + r.name;
      out += " | edges " + std::to_string(r.edges.size());
      for (const OccEdge& e : r.edges)
        out += " " + e.rule + "[" + enum_text("KZ_OCC_EDGE_KIND", e.kind) + ((e.flags & KZ_OCC_EDGE_FLAG_BY_STORE) != 0 ? ",by-store" : "") + "]#" +
               std::to_string(e.from) + "->#" + std::to_string(e.to);
    }
    out += "\n";
  }
  return out;
}

int occ_diff_status(const OccDiffCounts& n) { return n.ambiguous != 0 || n.uncertain != 0 ? 4 : n.any() ? 3 : 0; }

std::string occ_diff_text(const OccTable& a, const OccTable& b, OccDiffCounts* counts) {
  OccDiffCounts n;
  std::string head, verdicts, bases, only, owners, neither, ambiguous, uncertain, conflicts_text, keys;
  const bool keyed = a.recorded && b.recorded;
  const bool with_task = a.task_recorded && b.task_recorded;  // the task's place, where both containers record it
  head += std::string("matching: ") +
          (keyed ? std::string("by the occurrence key (rspec | op | owner") + (with_task ? "#task" : "") +
                       " | resource | ordinal) where both owners are established; then without the owner, where the owner (or the task's place in it) "
                       "is not established on one side or on neither -- never two established owners that differ -- and only where the role, the via "
                       "names, the text arguments and the owners' names agree; then by the text arguments where one side fixed the key value; a key "
                       "shared by several occurrences of a side, or a row with more than one candidate, is not paired"
                 : std::string("by position (a side has no OCC_BASIS: no key; the basis of summary.flags only)")) +
          "\n";
  if (keyed && !with_task) head += "task in the key: not used (" + std::string(a.task_recorded ? "B" : b.task_recorded ? "A" : "neither container") + " does not record it)\n";
  if (a.has_reading && b.has_reading) {
    const kz_replay_read& x = a.reading;
    const kz_replay_read& y = b.reading;
    auto sw = [&](const char* what, uint8_t p, uint8_t q, const char* en) {
      if (p != q) head += std::string("reading differs: ") + what + " A " + enum_text(en, p) + ", B " + enum_text(en, q) + "\n";
    };
    sw("outcome-narrowing", x.outcome_narrowing, y.outcome_narrowing, "KZ_READ_SWITCH");
    sw("owner-identity-targets", x.owner_identity_targets, y.owner_identity_targets, "KZ_READ_SWITCH");
    sw("reads-from-edges", x.reads_from_edges, y.reads_from_edges, "KZ_READ_SWITCH");
    sw("reads-from-outcome", x.reads_from_outcome, y.reads_from_outcome, "KZ_READ_SWITCH");
    sw("control-order", x.control_order, y.control_order, "KZ_READ_SWITCH");
    sw("relation", x.relation_rule, y.relation_rule, "KZ_RELATION_RULE");
    head += "control edges A " + std::to_string(x.control_edges) + ", B " + std::to_string(y.control_edges) + "\n";
  }
  // pairs of rows, with the rule that made each: the key or the position (0), the key without the owner (1), the text
  // arguments (2)
  struct Pair {
    const OccRow* x;
    const OccRow* y;
    int pass;
    OwnerRel rel;
  };
  struct Conflict {  // a candidate of the second pass whose owners differ: never paired
    const OccRow* x;
    const OccRow* y;
    std::string key;
  };
  struct Held {  // a candidate (or a component of candidates) of the second pass not paired: final after the third pass
    std::string key;
    std::vector<const OccRow*> ra, rb;
    std::string why;
  };
  std::vector<Pair> pairs;
  std::vector<Conflict> conflicts;
  std::vector<Held> unsure, components;
  std::set<const OccRow*> held, amb_rows, unc_rows;  // rows listed as ambiguous or uncertain (not one side only)
  std::vector<const OccRow*> left_a, left_b;
  auto list_uncertain = [&](const std::string& key, const char* rule, const OccRow* x, const OccRow* y, const std::string& why) {
    ++n.uncertain;
    uncertain += "  " + key + " (" + rule + "): A#" + std::to_string(x->index) + " " + owner_text(*x, with_task) + ", B#" + std::to_string(y->index) + " " +
                 owner_text(*y, with_task) + ";" + why + " A " + verdict_text(*x) + " | B " + verdict_text(*y) + "\n";
    for (const OccRow* r : {x, y}) held.insert(r), unc_rows.insert(r);
  };
  auto list_ambiguous = [&](const std::string& key, const char* rule, const std::vector<const OccRow*>& ra, const std::vector<const OccRow*>& rb, bool show_owners,
                            const std::string& note) {
    ++n.ambiguous;
    ambiguous += "  " + key + (rule != nullptr ? std::string(" (") + rule + ")" : std::string()) + ":";
    for (const OccRow* r : ra) ambiguous += " A#" + std::to_string(r->index) + " " + verdict_text(*r) + (show_owners ? " " + owner_text(*r, with_task) : std::string()) + ";";
    for (const OccRow* r : rb) ambiguous += " B#" + std::to_string(r->index) + " " + verdict_text(*r) + (show_owners ? " " + owner_text(*r, with_task) : std::string()) + ";";
    ambiguous += note + "\n";
    for (const std::vector<const OccRow*>* v : {&ra, &rb})
      for (const OccRow* r : *v) held.insert(r), amb_rows.insert(r);
  };
  if (!keyed) {
    const size_t m = std::min(a.rows.size(), b.rows.size());
    for (size_t i = 0; i < m; ++i) pairs.push_back({&a.rows[i], &b.rows[i], 0, OwnerRel::same});
    for (size_t i = m; i < a.rows.size(); ++i) left_a.push_back(&a.rows[i]);
    for (size_t i = m; i < b.rows.size(); ++i) left_b.push_back(&b.rows[i]);
  } else {
    for (const OccRow& r : a.rows) left_a.push_back(&r);
    for (const OccRow& r : b.rows) left_b.push_back(&r);
    for (int pass = 0; pass < 3; ++pass) {
      auto key = [&](const OccRow& r) -> std::string {
        if (pass == 0) return owner_established(r) ? occ_key(r, true, with_task) : std::string();  // "app:undetermined" names no owner: the second pass
        if (pass == 1) return occ_key(r, false, with_task);
        if (r.name.empty()) return std::string();
        OccRow x = r;  // the key without its value and ordinal: those depend on whether the value was fixed
        x.resource.clear();
        x.ordinal = 0;
        return occ_key(x, true, with_task);
      };
      std::map<std::string, std::pair<std::vector<const OccRow*>, std::vector<const OccRow*>>> groups;
      for (const OccRow* r : left_a)
        if (!key(*r).empty()) groups[key(*r)].first.push_back(r);
      for (const OccRow* r : left_b)
        if (!key(*r).empty()) groups[key(*r)].second.push_back(r);
      std::set<const OccRow*> taken;
      for (const auto& kv : groups) {
        const std::vector<const OccRow*>& ga = kv.second.first;
        const std::vector<const OccRow*>& gb = kv.second.second;
        if (ga.empty() || gb.empty()) continue;  // nothing to pair here: left for the next pass
        if (pass == 2) {
          // the candidates: one row each, the key value fixed on exactly one side
          std::vector<const OccRow*> ca, cb;
          for (const OccRow* x : ga)
            for (const OccRow* y : gb)
              if (x->resource.empty() != y->resource.empty()) {
                if (std::find(ca.begin(), ca.end(), x) == ca.end()) ca.push_back(x);
                if (std::find(cb.begin(), cb.end(), y) == cb.end()) cb.push_back(y);
              }
          if (ca.empty()) continue;
          if (ga.size() == 1 && gb.size() == 1) {
            const std::string d = names_differ(*ga[0], *gb[0]);
            if (d.empty()) pairs.push_back({ga[0], gb[0], 2, owner_relation(*ga[0], *gb[0], with_task)});
            else list_uncertain(kv.first, "by the text arguments", ga[0], gb[0], d);
            taken.insert(ga[0]);
            taken.insert(gb[0]);
            continue;
          }
          // more than one occurrence of a side has these text arguments: which is which is not established
          list_ambiguous(kv.first, "by the text arguments", ca, cb, false, std::string());
          for (const OccRow* r : ca) taken.insert(r);
          for (const OccRow* r : cb) taken.insert(r);
          continue;
        }
        if (pass == 1) {
          // the candidates: a row of each side whose owners do not contradict each other.  The rows they connect form
          // components; a component of one row each is a pair where nothing else the record says of the call differs.
          // A candidate not paired here stays for the third pass and is listed only where it is still not paired then.
          const size_t na = ga.size(), nb = gb.size();
          std::vector<size_t> root(na + nb);
          for (size_t i = 0; i < root.size(); ++i) root[i] = i;
          auto find = [&](size_t v) {
            while (root[v] != v) v = root[v] = root[root[v]];
            return v;
          };
          std::vector<bool> linked(na + nb, false);
          std::map<std::pair<size_t, size_t>, OwnerRel> rel;
          for (size_t i = 0; i < na; ++i)
            for (size_t j = 0; j < nb; ++j) {
              const OwnerRel r = owner_relation(*ga[i], *gb[j], with_task);
              if (r == OwnerRel::differ) {
                conflicts.push_back({ga[i], gb[j], kv.first});  // said apart if neither row is paired or listed otherwise
                continue;
              }
              rel[{i, j}] = r;
              linked[i] = linked[na + j] = true;
              root[find(i)] = find(na + j);
            }
          std::map<size_t, std::pair<std::vector<const OccRow*>, std::vector<const OccRow*>>> comps;
          std::map<size_t, std::pair<size_t, size_t>> first;  // a component's rows (by group index) where it has one each
          for (size_t i = 0; i < na; ++i)
            if (linked[i]) comps[find(i)].first.push_back(ga[i]), first[find(i)].first = i;
          for (size_t j = 0; j < nb; ++j)
            if (linked[na + j]) comps[find(na + j)].second.push_back(gb[j]), first[find(na + j)].second = j;
          for (const auto& c : comps) {
            const std::vector<const OccRow*>& ca = c.second.first;
            const std::vector<const OccRow*>& cb = c.second.second;
            if (ca.size() != 1 || cb.size() != 1) {
              components.push_back({kv.first, ca, cb, std::string()});
              continue;
            }
            const OccRow* x = ca[0];
            const OccRow* y = cb[0];
            const OwnerRel r = rel[first[c.first]];
            std::string d = descriptors_differ(*x, *y);
            if (r == OwnerRel::task_differ) d += " task's place in application " + std::to_string(x->owner_key) + " A " + std::to_string(x->task) + " B " + std::to_string(y->task) + ";";
            if (!d.empty()) {  // only the ordinal ties them: kept uncertain, not paired
              unsure.push_back({kv.first, {x}, {y}, d});
              continue;
            }
            pairs.push_back({x, y, r == OwnerRel::same ? 0 : 1, r});
            taken.insert(x);
            taken.insert(y);
          }
          continue;
        }
        if (ga.size() == 1 && gb.size() == 1) {
          // the names the record gives differ: not paired here; the second pass lists the candidate as uncertain
          if (!names_differ(*ga[0], *gb[0]).empty()) continue;
          pairs.push_back({ga[0], gb[0], 0, OwnerRel::same});
          taken.insert(ga[0]);
          taken.insert(gb[0]);
          continue;
        }
        // a key several occurrences of a side share: which is which is not established, so none is paired
        list_ambiguous(kv.first, nullptr, ga, gb, false, std::string());
        for (const OccRow* r : ga) taken.insert(r);
        for (const OccRow* r : gb) taken.insert(r);
      }
      std::vector<const OccRow*> next_a, next_b;
      for (const OccRow* r : left_a)
        if (taken.count(r) == 0) next_a.push_back(r);
      for (const OccRow* r : left_b)
        if (taken.count(r) == 0) next_b.push_back(r);
      left_a = next_a;
      left_b = next_b;
    }
    // the candidates the second pass did not pair, where the third pass did not pair their rows either
    std::set<const OccRow*> paired;
    for (const Pair& pr : pairs) paired.insert(pr.x), paired.insert(pr.y);
    for (const Held& h : unsure)
      if (paired.count(h.ra[0]) == 0 && paired.count(h.rb[0]) == 0) list_uncertain(h.key, "without the owner", h.ra[0], h.rb[0], h.why);
    for (const Held& h : components) {
      std::vector<const OccRow*> ra, rb;
      for (const OccRow* r : h.ra)
        if (paired.count(r) == 0) ra.push_back(r);
      for (const OccRow* r : h.rb)
        if (paired.count(r) == 0) rb.push_back(r);
      if (ra.empty() || rb.empty()) continue;  // the rows left have no candidate: one side only
      list_ambiguous(h.key, "without the owner", ra, rb, true,
                     ra.size() + rb.size() < h.ra.size() + h.rb.size() ? " (other rows of this group paired by the text arguments)" : "");
    }
  }
  n.ambiguous_rows = amb_rows.size();
  n.uncertain_rows = unc_rows.size();
  uint64_t owner_verdict = 0, owner_basis = 0, neither_verdict = 0, neither_basis = 0, key_verdict = 0, key_basis = 0;
  for (const Pair& pr : pairs) {
    const OccRow& x = *pr.x;
    const OccRow& y = *pr.y;
    ++n.matched;
    const std::string key = occ_key(x, true, with_task);
    std::string v;
    if (x.status != y.status || x.status_flags != y.status_flags || x.observation != y.observation || x.obs_reasons != y.obs_reasons)
      v = "A " + verdict_text(x) + " | B " + verdict_text(y);
    std::string d;
    // by position (a side without OCC_BASIS): both sides read from summary.flags, so that a kind only OCC_BASIS can hold
    // is not taken for a difference
    const uint32_t xb = keyed ? x.basis : basis_of_flags(x.summary_flags), yb = keyed ? y.basis : basis_of_flags(y.summary_flags);
    const uint32_t xa = keyed ? x.assumptions : assumptions_of_flags(x.summary_flags), ya = keyed ? y.assumptions : assumptions_of_flags(y.summary_flags);
    if (xb != yb) d += " basis A " + flags_text("KZ_OCC_BASIS", xb) + " B " + flags_text("KZ_OCC_BASIS", yb) + ";";
    if (xa != ya) d += " assumptions A " + flags_text("KZ_OCC_ASSUME", xa) + " B " + flags_text("KZ_OCC_ASSUME", ya) + ";";
    if (keyed) {
      if (x.outcome != y.outcome) d += " outcome A " + enum_text("KZ_OUTCOME_CHECK", x.outcome) + " B " + enum_text("KZ_OUTCOME_CHECK", y.outcome) + ";";
      if (x.via != y.via) d += " via A {" + x.via + "} B {" + y.via + "};";
      if (x.context != y.context) d += " role A {" + x.context + "} B {" + y.context + "};";
      const std::string ra = rules_text(x), rb = rules_text(y);
      if (ra != rb) d += " edges A " + ra + " B " + rb + ";";
    }
    if (pr.pass == 0) {  // the full key (or the position): the verdict and basis differences
      ++n.by_key;
      if (!v.empty()) {
        ++n.verdict;
        verdicts += "  " + key + ": " + v + "\n";
      }
      if (!d.empty()) {
        ++n.basis;
        bases += "  " + key + ":" + d + "\n";
      }
      continue;
    }
    // a pair of the other rules: compared in its own section
    if (!v.empty()) ++n.verdict_other;
    if (!d.empty()) ++n.basis_other;
    const std::string cmp = std::string(v.empty() ? "verdict same" : "verdict " + v) + (d.empty() ? "; basis same" : ";" + d);
    const std::string sides = "A " + owner_text(x, with_task) + ", B " + owner_text(y, with_task);
    if (pr.pass == 1 && pr.rel == OwnerRel::neither) {
      ++n.owner_neither;
      neither_verdict += v.empty() ? 0 : 1;
      neither_basis += d.empty() ? 0 : 1;
      neither += "  " + key + " ~ B " + occ_key(y, true, with_task) + " (" + sides + "): " + cmp + "\n";
    } else if (pr.pass == 1) {
      ++n.owner_unestablished;
      owner_verdict += v.empty() ? 0 : 1;
      owner_basis += d.empty() ? 0 : 1;
      const std::string why = pr.rel == OwnerRel::task_open ? "the task's place in application " + std::to_string(x.owner_key) + " established in " + (x.task != 0 ? "A" : "B") + " only"
                                                            : "the owner established in " + std::string(owner_established(x) ? "A" : "B") + " only";
      owners += "  " + key + " ~ B " + occ_key(y, true, with_task) + " (" + why + "; " + sides + "): " + cmp + "\n";
    } else {
      ++n.key_differs;
      key_verdict += v.empty() ? 0 : 1;
      key_basis += d.empty() ? 0 : 1;
      keys += "  " + key + " ~ B " + occ_key(y, true, with_task) + " (key value fixed in " + (x.resource.empty() ? "B" : "A") + " only" +
              (pr.rel == OwnerRel::neither ? "; neither side's owner established: " + sides : std::string()) + "): " + cmp + "\n";
    }
  }
  // the rows of one side only: not paired and not listed as ambiguous or uncertain
  std::vector<const OccRow*> only_a, only_b;
  for (const OccRow* r : left_a)
    if (held.count(r) == 0) only_a.push_back(r);
  for (const OccRow* r : left_b)
    if (held.count(r) == 0) only_b.push_back(r);
  // the candidates of the second pass whose owners differ, where neither row was paired or listed otherwise
  const std::set<const OccRow*> alone_a(only_a.begin(), only_a.end()), alone_b(only_b.begin(), only_b.end());
  std::map<const OccRow*, std::string> refused;  // per row of one side only: the candidates refused for their owner
  for (const Conflict& c : conflicts) {
    if (alone_a.count(c.x) != 0) refused[c.x] += " B#" + std::to_string(c.y->index);
    if (alone_b.count(c.y) != 0) refused[c.y] += " A#" + std::to_string(c.x->index);
    if (alone_a.count(c.x) == 0 || alone_b.count(c.y) == 0) continue;
    ++n.owner_conflict;
    conflicts_text += "  " + c.key + " (without the owner): A#" + std::to_string(c.x->index) + " " + owner_text(*c.x, with_task) + " " + verdict_text(*c.x) + "; B#" +
                      std::to_string(c.y->index) + " " + owner_text(*c.y, with_task) + " " + verdict_text(*c.y) + "\n";
  }
  auto only_line = [&](const char* side, const OccRow* r) {
    const auto it = refused.find(r);
    return std::string("  only in ") + side + ": " + occ_key(*r, true, with_task) + " " + verdict_text(*r) +
           (it != refused.end() ? " (not paired with" + it->second + ": the owners differ)" : "") + "\n";
  };
  for (const OccRow* r : only_a) {
    ++n.only_a;
    only += only_line("A", r);
  }
  for (const OccRow* r : only_b) {
    ++n.only_b;
    only += only_line("B", r);
  }
  const int status = occ_diff_status(n);
  std::string out = head + "occurrences A " + std::to_string(a.rows.size()) + ", B " + std::to_string(b.rows.size()) + "; matched " + std::to_string(n.matched) +
                    ", only in A " + std::to_string(n.only_a) + ", only in B " + std::to_string(n.only_b) +
                    (n.ambiguous != 0 ? ", ambiguous keys " + std::to_string(n.ambiguous) + " (" + std::to_string(n.ambiguous_rows) + " occurrences, not paired)" : "") +
                    (n.uncertain != 0 ? ", uncertain " + std::to_string(n.uncertain) + " (" + std::to_string(n.uncertain_rows) + " occurrences, not paired)" : "") +
                    (n.owner_conflict != 0 ? ", owners differ " + std::to_string(n.owner_conflict) + " (not paired)" : "") +
                    (n.owner_unestablished != 0 ? ", matched with one side's owner not established " + std::to_string(n.owner_unestablished) : "") +
                    (n.owner_neither != 0 ? ", matched with neither side's owner established " + std::to_string(n.owner_neither) : "") +
                    (n.key_differs != 0 ? ", matched by the text arguments " + std::to_string(n.key_differs) : "") + "\n";
  if (keyed)
    out += "pairs: by the full key " + std::to_string(n.by_key) + ", with one side's owner not established " + std::to_string(n.owner_unestablished) +
           ", with neither side's owner established " + std::to_string(n.owner_neither) + ", by the text arguments " + std::to_string(n.key_differs) +
           "; the verdict and basis differences below count the pairs of the full key (both owners established), the other pairs are compared in "
           "their own sections; edge rules are compared per occurrence by rule name, kind, by-store and count, not by their endpoints\n";
  out += "result: exit " + std::to_string(status) +
         (status == 4   ? " (the pairing of some occurrences is not established: ambiguous keys " + std::to_string(n.ambiguous) + ", uncertain " + std::to_string(n.uncertain) + ")"
          : status == 3 ? std::string(" (differences)")
                        : std::string(" (no difference)")) +
         (reading_differs(a, b) ? "; the reading differs (above), which the exit status does not count" : "") + "\n";
  out += "verdict differences (status, summary view): " + std::to_string(n.verdict) + "\n" + verdicts;
  out += "basis differences (basis kinds, assumptions, outcome check, edge rules): " + std::to_string(n.basis) + "\n" + bases;
  if (!owners.empty())
    out += "matched with one side's owner not established (the key without the owner, the role, the via names, the text arguments and the owners' names "
           "equal; not a full-key match): " +
           std::to_string(n.owner_unestablished) + "; verdict differences " + std::to_string(owner_verdict) + ", basis differences " + std::to_string(owner_basis) + "\n" + owners;
  if (!neither.empty())
    out += "matched with neither side's owner established (the key without the owner, the role, the via names, the text arguments and the owners' names "
           "equal; no owner ties them): " +
           std::to_string(n.owner_neither) + "; verdict differences " + std::to_string(neither_verdict) + ", basis differences " + std::to_string(neither_basis) + "\n" + neither;
  if (!keys.empty())
    out += "matched by the text arguments (the key value is fixed on one side only): " + std::to_string(n.key_differs) + "; verdict differences " + std::to_string(key_verdict) +
           ", basis differences " + std::to_string(key_basis) + "\n" + keys;
  if (!uncertain.empty())
    out += "uncertain (a candidate the owners do not rule out, but the role, the via names, the text arguments, the owners' names or the task's places "
           "differ; not paired): " +
           std::to_string(n.uncertain) + "\n" + uncertain;
  if (!ambiguous.empty())
    out += "ambiguous keys (shared by more than one occurrence of a side, or more than one candidate of a row; which is which is not established, none "
           "paired): " +
           std::to_string(n.ambiguous) + "\n" + ambiguous;
  if (!conflicts_text.empty())
    out += "owners differ (the key without the owner equal, the owning applications established on both sides and different; not the same occurrence, "
           "not paired, listed again below as one side only): " +
           std::to_string(n.owner_conflict) + "\n" + conflicts_text;
  out += only;
  if (counts != nullptr) *counts = n;
  return out;
}

}  // namespace kz::trace

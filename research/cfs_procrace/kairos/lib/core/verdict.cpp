// verdict.cpp -- defaults, invariants, ledger summary and rendering of the verdict records.

#include "kz/core/verdict.hpp"

#include <algorithm>
#include <cstring>

#include "kz/core/dump.hpp"
#include "kz/core/registry.hpp"

namespace kz {

namespace {

bool is_zero(const void* p, size_t n) {
  const uint8_t* b = static_cast<const uint8_t*>(p);
  for (size_t i = 0; i < n; ++i)
    if (b[i] != 0) return false;
  return true;
}

bool ref_is_null(const kz_ref& r) { return is_zero(&r, sizeof(kz_ref)); }

void add(std::vector<Finding>* out, const std::string& where, const std::string& what) { out->push_back(Finding{where, what}); }

const char* enum_text(const char* enum_name, uint64_t v) {
  const EnumDesc* en = find_enum(enum_name);
  const char* d = en != nullptr ? enum_display(*en, v) : nullptr;
  return d != nullptr ? d : "?";
}

// Every reserved field of the record type `record_name` must be zero in the record at `p`.
void check_reserved(const char* record_name, const void* p, const std::string& where, std::vector<Finding>* out) {
  const kz_record_desc* rec = find_record(record_name);
  if (rec == nullptr) return;
  const uint8_t* b = static_cast<const uint8_t*>(p);
  for (uint32_t i = 0; i < rec->nfields; ++i) {
    const kz_field_desc& f = rec->fields[i];
    if (is_reserved_field(f) && !is_zero(b + f.offset, static_cast<size_t>(f.elem_size) * f.count))
      add(out, where, std::string("reserved field ") + f.name + " is not zero");
  }
}

// Proof-status order, weakest first.  NA (no lemma) is outside the order.
int proof_rank(uint8_t p) {
  switch (p) {
    case KZ_PROOF_UNPROVED: return 0;
    case KZ_PROOF_DEPENDS: return 1;
    case KZ_PROOF_PROVED: return 2;
  }
  return -1;
}

uint8_t status_of_rank(int rank) {
  return rank == 0 ? KZ_PROOF_UNPROVED : rank == 1 ? KZ_PROOF_DEPENDS : rank == 2 ? KZ_PROOF_PROVED : KZ_PROOF_NA;
}

// A statement rests on a certificate if one of its routes is a certificate or a transfer of
// one, or its realization says so.
bool certificate_based(const kz_verdict_cell& c) {
  const uint8_t not_bad = route_get(c.safety_routes, KZ_ROUTE_NOT_BAD_SHIFT);
  const uint8_t not_ok = route_get(c.safety_routes, KZ_ROUTE_NOT_OK_SHIFT);
  const uint8_t not_nr = route_get(c.safety_routes, KZ_ROUTE_NOT_NR_SHIFT);
  for (uint8_t r : {not_bad, not_ok, not_nr})
    if (r == KZ_ROUTE_CERT || r == KZ_ROUTE_TRANSFERRED) return true;
  return (c.realization & KZ_REALIZATION_CERT) != 0;
}

}  // namespace

// ---- defaults ------------------------------------------------------------------------

kz_verdict_cell make_cell(uint8_t sigma, uint8_t mlevel) {
  kz_verdict_cell c;
  std::memset(&c, 0, sizeof(c));
  c.sigma = sigma;
  c.mlevel = mlevel;
  c.cls = KZ_CLS_UNKNOWN;
  c.unknown_reason = KZ_UNKNOWN_REASON_NOT_REACHED;
  c.reach = KZ_REACH_ASSUMED;
  c.progress = KZ_PROGRESS_UNKNOWN;
  c.progress_reason = KZ_PROGRESS_REASON_NOT_CHECKED;
  c.fw_conf = KZ_FW_CONF_UNKNOWN;
  c.proof_status = KZ_PROOF_NA;
  return c;
}

kz_verdict make_verdict(uint64_t instance) {
  kz_verdict v;
  std::memset(&v, 0, sizeof(v));
  v.instance = instance;
  v.provider = KZ_PROVIDER_UNKNOWN;
  v.scope = KZ_SCOPE_IN_SCOPE;
  v.harm = KZ_HARM_UNSPECIFIED;
  return v;
}

kz_trust make_trust_entry(uint8_t part, uint8_t kind) {
  kz_trust t;
  std::memset(&t, 0, sizeof(t));
  t.part = part;
  t.kind = kind;
  t.site = KZ_NONE32;
  t.refutes = KZ_NONE32;
  t.bound = KZ_BOUND_INFINITE;  // SPEC 2.3: an entry supports the bound h, or b = infinity
  if (kind == KZ_TRUST_KIND_LEMMA) t.proof = KZ_PROOF_UNPROVED;  // a lemma is unproved until the lemma table says otherwise (PS2)
  return t;
}

uint8_t route_get(uint8_t routes, unsigned shift) { return static_cast<uint8_t>((routes >> shift) & KZ_ROUTE_MASK); }

uint8_t route_set(uint8_t routes, unsigned shift, uint8_t route) {
  unsigned cleared = static_cast<unsigned>(routes) & ~(KZ_ROUTE_MASK << shift);
  return static_cast<uint8_t>(cleared | ((static_cast<unsigned>(route) & KZ_ROUTE_MASK) << shift));
}

// ---- proof status and ledger ---------------------------------------------------------

const char* proof_status_name(uint8_t proof) { return enum_text("KZ_PROOF", proof); }

uint8_t weakest_proof(const std::vector<kz_trust>& entries) {
  int floor_rank = 3;
  for (const kz_trust& t : entries) {
    if (t.kind != KZ_TRUST_KIND_LEMMA) continue;
    int rank = proof_rank(t.proof);
    if (rank < 0) rank = 0;  // an invalid status counts as unproved
    if (rank < floor_rank) floor_rank = rank;
  }
  return floor_rank == 3 ? static_cast<uint8_t>(KZ_PROOF_NA) : status_of_rank(floor_rank);
}

LedgerCounts count_ledger(const std::vector<kz_trust>& entries) {
  LedgerCounts c;
  for (const kz_trust& t : entries) {
    switch (t.part) {
      case KZ_TRUST_PART_RECHECKED: ++c.n_rechecked; break;
      case KZ_TRUST_PART_EXTRACTED: ++c.n_extracted; break;
      case KZ_TRUST_PART_TRUSTED: ++c.n_trusted; break;
      case KZ_TRUST_PART_REFUTABLE: ++c.n_refutable; break;
      default: break;
    }
    if (t.kind == KZ_TRUST_KIND_DECLARED_TRUSTED) ++c.declared_trusted;
    if (t.kind != KZ_TRUST_KIND_LEMMA) continue;
    switch (proof_rank(t.proof)) {
      case 1: ++c.lemma_depends; break;
      case 2: ++c.lemma_proved; break;
      default: ++c.lemma_unproved; break;  // unproved, and anything that is not a valid status
    }
    if (proof_rank(t.proof) != 2) c.lemma_not_proved.push_back(t.ident);
  }
  std::sort(c.lemma_not_proved.begin(), c.lemma_not_proved.end());
  c.lemma_not_proved.erase(std::unique(c.lemma_not_proved.begin(), c.lemma_not_proved.end()), c.lemma_not_proved.end());
  return c;
}

kz_trust_ledger make_ledger(const std::vector<kz_trust>& entries, uint32_t first) {
  LedgerCounts c = count_ledger(entries);
  kz_trust_ledger l;
  std::memset(&l, 0, sizeof(l));
  l.n_rechecked = c.n_rechecked;
  l.n_extracted = c.n_extracted;
  l.n_trusted = c.n_trusted;
  l.n_refutable = c.n_refutable;
  l.first = first;
  l.count = static_cast<uint32_t>(entries.size());
  return l;
}

std::string ledger_label(const kz_trust_ledger& l) {
  return "ledger:" + std::to_string(l.n_rechecked) + "/" + std::to_string(l.n_extracted) + "/" + std::to_string(l.n_trusted) + "/" +
         std::to_string(l.n_refutable);
}

std::string trusted_label(const LedgerCounts& c) { return "trusted:" + std::to_string(c.declared_trusted); }

std::string lemmas_label(const LedgerCounts& c) {
  return "lemmas:" + std::to_string(c.lemma_proved) + "/" + std::to_string(c.lemma_depends) + "/" + std::to_string(c.lemma_unproved);
}

// ---- validation ----------------------------------------------------------------------

std::vector<Finding> validate_cell(const kz_verdict_cell& c) {
  std::vector<Finding> f;
  const std::string w = "cell";
  check_reserved("kz_verdict_cell", &c, w, &f);
  if (c.sigma < KZ_SIGMA_U || c.sigma > KZ_SIGMA_E) add(&f, w, "sigma is unset or out of range");
  if (c.mlevel < KZ_MLEVEL_API || c.mlevel > KZ_MLEVEL_IMPL) add(&f, w, "model level is unset or out of range");
  if (c.cls > KZ_CLS_CONTESTED) add(&f, w, "class value out of range");
  if (c.reach > KZ_REACH_CONTESTED) add(&f, w, "reach value out of range");
  if (c.progress > KZ_PROGRESS_CONTESTED) add(&f, w, "progress value out of range");
  if (c.unknown_reason > KZ_UNKNOWN_REASON_MODEL_LEVEL) add(&f, w, "unknown reason out of range");
  if (c.progress_reason > KZ_PROGRESS_REASON_MODEL_FAULT) add(&f, w, "progress reason out of range");
  if (c.progress_harm > KZ_PROGRESS_HARM_TERMINAL_PENDING_ABORT) add(&f, w, "progress harm out of range");
  if (c.progress_realization > KZ_PROGRESS_REALIZATION_REALIZED) add(&f, w, "progress realization out of range");
  if (c.fw_conf > KZ_FW_CONF_DEVIATES) add(&f, w, "framework conformance out of range");
  if (c.proof_status > KZ_PROOF_PROVED) add(&f, w, "proof status out of range");
  if ((c.safety_routes & ~KZ_ROUTES_VALID_MASK) != 0) add(&f, w, "safety_routes uses bits 6-7");

  const uint8_t not_bad = route_get(c.safety_routes, KZ_ROUTE_NOT_BAD_SHIFT);
  const uint8_t not_ok = route_get(c.safety_routes, KZ_ROUTE_NOT_OK_SHIFT);
  const uint8_t not_nr = route_get(c.safety_routes, KZ_ROUTE_NOT_NR_SHIFT);

  // Honest unknowns: a class without a statement carries its reason, and only then.
  if (c.cls == KZ_CLS_UNKNOWN && c.unknown_reason == KZ_UNKNOWN_REASON_NONE) add(&f, w, "class unknown without a reason");
  if (c.cls == KZ_CLS_CONTESTED && c.unknown_reason != KZ_UNKNOWN_REASON_MODEL_FAULT) add(&f, w, "class contested must carry reason model_fault");
  if (c.cls != KZ_CLS_UNKNOWN && c.cls != KZ_CLS_CONTESTED && c.unknown_reason != KZ_UNKNOWN_REASON_NONE)
    add(&f, w, "an unknown reason is set on a class that is not unknown");

  // The class is a function of the safety record (SPEC 1.10).
  switch (c.cls) {
    case KZ_CLS_PROTECTED:
      if (not_bad == KZ_ROUTE_NONE) add(&f, w, "protected without a not-Bad statement");
      if (not_ok != KZ_ROUTE_NONE) add(&f, w, "protected with a not-OK statement (that is no-final-status)");
      break;
    case KZ_CLS_FIXED:
      if (not_ok == KZ_ROUTE_NONE) add(&f, w, "fixed-order without a not-OK statement");
      if (not_bad != KZ_ROUTE_NONE) add(&f, w, "fixed-order with a not-Bad statement (that is no-final-status)");
      if ((c.flags & KZ_CELL_FLAG_DECIDES) == 0) add(&f, w, "fixed-order needs a decision route (flag decides)");
      break;
    case KZ_CLS_NO_FINAL:
      if (not_bad == KZ_ROUTE_NONE || not_ok == KZ_ROUTE_NONE) add(&f, w, "no-final-status needs both not-Bad and not-OK");
      break;
    case KZ_CLS_SCHED_DEP:
    case KZ_CLS_ORDINAL_SPLIT:
      if (ref_is_null(c.witness_bad) || ref_is_null(c.witness_good)) add(&f, w, "needs a bad and a good witness");
      break;
    default:
      break;
  }
  if (not_nr != KZ_ROUTE_NONE) {
    if (not_bad == KZ_ROUTE_NONE) add(&f, w, "not-NR without not-Bad (not-NR implies not-Bad)");
    if (not_nr == KZ_ROUTE_MODEL) add(&f, w, "not-NR comes only from a certificate or a transfer of one");
  }
  const bool by_cert = certificate_based(c);
  if (by_cert && ref_is_null(c.certificate)) add(&f, w, "a certificate route or realization names no certificate");
  // PS3: a certificate-based statement carries the proof status of the lemmas it uses.
  if (by_cert && c.proof_status == KZ_PROOF_NA) add(&f, w, "a certificate-based statement has no proof status (PS3)");

  // Reach is proved by an observed execution.
  if (c.reach == KZ_REACH_PROVED && ref_is_null(c.runs)) add(&f, w, "reach proved without a run");

  // Progress (D20): unknown(not_checked) unless something establishes more.
  switch (c.progress) {
    case KZ_PROGRESS_UNKNOWN:
      if (c.progress_kind != 0 || c.progress_harm != KZ_PROGRESS_HARM_NONE || c.progress_realization != KZ_PROGRESS_REALIZATION_NONE ||
          c.progress_flags != 0 || !ref_is_null(c.progress_chain) || !ref_is_null(c.witness_stall))
        add(&f, w, "progress unknown carries stall details");
      break;
    case KZ_PROGRESS_STALL_WITNESS:
      if (c.progress_kind == 0) add(&f, w, "stall witness without a kind set");
      if (c.progress_realization == KZ_PROGRESS_REALIZATION_NONE) add(&f, w, "stall witness without a realization");
      if (ref_is_null(c.witness_stall)) add(&f, w, "stall witness without a witness reference");
      if (c.progress_reason != KZ_PROGRESS_REASON_NOT_CHECKED) add(&f, w, "a progress reason is set on a progress value that is not unknown");
      break;
    case KZ_PROGRESS_NO_STALL_B:
      if (ref_is_null(c.progress_bounds)) add(&f, w, "no-stall-within(b) names no bound");
      if (c.progress_reason != KZ_PROGRESS_REASON_NOT_CHECKED) add(&f, w, "a progress reason is set on a progress value that is not unknown");
      break;
    case KZ_PROGRESS_CONTESTED:
      if (c.progress_reason != KZ_PROGRESS_REASON_MODEL_FAULT) add(&f, w, "progress contested must carry reason model_fault");
      break;
    default:
      break;
  }

  // Framework conformance comes from M_impl cells and runs; M_api assumes the API is as documented.
  if (c.mlevel == KZ_MLEVEL_API && c.fw_conf != KZ_FW_CONF_UNKNOWN) add(&f, w, "an M_api cell has fw_conf unknown");
  return f;
}

std::vector<Finding> validate_cell_proof(const kz_verdict_cell& c, const std::vector<kz_trust>& entries) {
  std::vector<Finding> f;
  if (c.proof_status != weakest_proof(entries))
    add(&f, "cell", std::string("proof_status ") + proof_status_name(c.proof_status) + " is not the weakest status among the lemmas of its ledger (" +
                        proof_status_name(weakest_proof(entries)) + ")");
  return f;
}

std::vector<Finding> validate_verdict(const kz_verdict& v, const std::vector<kz_verdict_cell>& cells) {
  std::vector<Finding> f;
  check_reserved("kz_verdict", &v, "verdict", &f);
  if (v.rsv1 != 0) add(&f, "verdict", "rsv1 (formerly reach) must be zero: reach is per cell");
  if (v.provider > KZ_PROVIDER_ENV_ABSENT) add(&f, "verdict", "provider out of range");
  if (v.scope > KZ_SCOPE_TEARDOWN) add(&f, "verdict", "scope out of range");
  if (v.outcome_label > KZ_OUTCOME_LABEL_NOT_READY_UNJUDGED) add(&f, "verdict", "outcome label out of range");
  if (v.shape > KZ_SHAPE_STALE) add(&f, "verdict", "shape out of range");
  if (v.consequence > KZ_CONSEQUENCE_EVENT_REJECTED) add(&f, "verdict", "consequence out of range");
  if (v.harm > KZ_HARM_MESSAGE_LOST) add(&f, "verdict", "harm out of range");
  if ((v.fw_doc & ~(KZ_FW_DOC_DOC_NOT_ERROR | KZ_FW_DOC_DOC_RESULT | KZ_FW_DOC_UNDOCUMENTED)) != 0) add(&f, "verdict", "fw_doc has unknown bits");
  if ((v.sublabels & ~(KZ_SUBLABEL_PORTABILITY_HAZARD | KZ_SUBLABEL_IMPL_DEFECT_CANDIDATE | KZ_SUBLABEL_PORTABILITY_HAZARD_EVIDENCE |
                       KZ_SUBLABEL_ASSUMED_VISIBILITY | KZ_SUBLABEL_A_ZT | KZ_SUBLABEL_TB_UNDECLARED)) != 0)
    add(&f, "verdict", "sublabels has unknown bits");
  if (v.ncells != cells.size()) add(&f, "verdict", "ncells does not match the cells given");
  bool shows_not_ready = false;
  for (size_t i = 0; i < cells.size(); ++i) {
    const std::string where = "cell[" + std::to_string(i) + "]";
    for (const Finding& x : validate_cell(cells[i])) add(&f, where, x.what);
    for (size_t j = 0; j < i; ++j)
      if (cells[j].sigma == cells[i].sigma && cells[j].mlevel == cells[i].mlevel) add(&f, where, "duplicate (sigma, M) cell");
    if (cells[i].cls == KZ_CLS_FIXED || cells[i].cls == KZ_CLS_SCHED_DEP || cells[i].cls == KZ_CLS_ORDINAL_SPLIT) shows_not_ready = true;
    if (v.scope == KZ_SCOPE_TEARDOWN &&
        (cells[i].progress != KZ_PROGRESS_UNKNOWN || cells[i].progress_reason != KZ_PROGRESS_REASON_TEARDOWN_SCOPE))
      add(&f, where, "a teardown-scope instance has progress unknown(teardown_scope)");
  }
  if (v.outcome_label != KZ_OUTCOME_LABEL_NONE && !shows_not_ready)
    add(&f, "verdict", "an outcome label is set but no cell has a class that shows a not-ready outcome");
  return f;
}

std::vector<Finding> validate_trust_entry(const kz_trust& t) {
  std::vector<Finding> f;
  const std::string w = "entry";
  check_reserved("kz_trust", &t, w, &f);
  switch (t.part) {
    case KZ_TRUST_PART_RECHECKED:
      if (t.kind != KZ_TRUST_KIND_CERT_STEP) add(&f, w, "part (i) holds certificate steps only");
      if (t.status != KZ_TRUST_STATUS_ACCEPTED && t.status != KZ_TRUST_STATUS_ACCEPTED_VIA_T5) add(&f, w, "part (i) status is ACCEPTED or ACCEPTED_VIA_T5");
      break;
    case KZ_TRUST_PART_EXTRACTED:
      if (t.kind < KZ_TRUST_KIND_FACT || t.kind > KZ_TRUST_KIND_PROBE) add(&f, w, "part (ii) kind is FACT, SKEL, CLOSURE, ANCHOR, RUN or PROBE");
      if (t.status != KZ_TRUST_STATUS_NA) add(&f, w, "part (ii) has no status");
      break;
    case KZ_TRUST_PART_TRUSTED:
      if (t.kind < KZ_TRUST_KIND_PROFILE_ENTRY || t.kind > KZ_TRUST_KIND_TOOL_BASE) add(&f, w, "part (iii) kind is PROFILE_ENTRY, DECLARED_TRUSTED, ASSUMPTION, LEMMA or TOOL_BASE");
      if (t.status != KZ_TRUST_STATUS_NA) add(&f, w, "part (iii) has no status");
      break;
    case KZ_TRUST_PART_REFUTABLE:
      if (t.kind != KZ_TRUST_KIND_OBLIGATION) add(&f, w, "part (iv) holds obligations only");
      if (t.status < KZ_TRUST_STATUS_NOT_RUN || t.status > KZ_TRUST_STATUS_REFUTED) add(&f, w, "part (iv) status is NOT_RUN, NOT_REFUTED_ON_EXERCISED or REFUTED");
      if (t.refutes == KZ_NONE32) add(&f, w, "part (iv) names the entry it can refute");
      if (ref_is_null(t.source)) add(&f, w, "part (iv) names its OBLIGATIONS record");
      break;
    default:
      add(&f, w, "part is unset or out of range");
  }
  // D20, PS1-PS2: only a lemma has a proof status, and 'proved' is a property of the text that
  // holds the proof, so the entry must point at that text.  A lemma that is only listed is unproved.
  if (t.kind == KZ_TRUST_KIND_LEMMA) {
    if (proof_rank(t.proof) < 0) add(&f, w, "a lemma has a proof status (unproved, depends-on-stated-assumption or proved)");
  } else if (t.proof != KZ_PROOF_NA) {
    add(&f, w, "only a lemma has a proof status");
  }
  if (t.proof == KZ_PROOF_PROVED && ref_is_null(t.source)) add(&f, w, "a lemma marked proved names no record of the text that holds its proof");
  else if (t.proof == KZ_PROOF_PROVED && t.source.section != KZ_SEC_PROVENANCE)
    add(&f, w, "a lemma marked proved must name a PROVENANCE record (the cited text that holds its proof), not another kind of record");
  return f;
}

std::vector<Finding> validate_proof_texts(const std::vector<kz_trust>& entries, const std::vector<kz_prov>& provenance) {
  std::vector<Finding> f;
  for (size_t i = 0; i < entries.size(); ++i) {
    const kz_trust& t = entries[i];
    if (t.kind != KZ_TRUST_KIND_LEMMA || t.proof != KZ_PROOF_PROVED || t.source.section != KZ_SEC_PROVENANCE) continue;
    if (!is_zero(t.source.artifact_sha256, sizeof(t.source.artifact_sha256))) continue;  // a record of a parent: the caller has to look there
    const std::string where = "entry[" + std::to_string(i) + "]";
    if (t.source.index >= provenance.size()) {
      add(&f, where, "a lemma marked proved names PROVENANCE[" + std::to_string(t.source.index) + "], which does not exist");
      continue;
    }
    const kz_prov& p = provenance[t.source.index];
    if (p.file.len == 0) add(&f, where, "the text record of a proved lemma names no path or requirement id (file)");
    if (is_zero(p.text_sha256, sizeof(p.text_sha256))) add(&f, where, "the text record of a proved lemma carries no hash of the text (text_sha256)");
  }
  return f;
}

std::vector<Finding> validate_ledger(const kz_trust_ledger& l, const std::vector<kz_trust>& entries) {
  std::vector<Finding> f;
  check_reserved("kz_trust_ledger", &l, "ledger", &f);
  for (size_t i = 0; i < entries.size(); ++i)
    for (const Finding& x : validate_trust_entry(entries[i])) add(&f, "entry[" + std::to_string(i) + "]", x.what);
  const LedgerCounts c = count_ledger(entries);
  if (l.count != entries.size()) add(&f, "ledger", "count does not match the entries");
  if (l.n_rechecked != c.n_rechecked || l.n_extracted != c.n_extracted || l.n_trusted != c.n_trusted || l.n_refutable != c.n_refutable)
    add(&f, "ledger", "part sizes do not match the entries");
  return f;
}

std::vector<Finding> validate_cert(const kz_cert& cert, const std::vector<kz_trust>& entries) {
  std::vector<Finding> f;
  check_reserved("kz_cert", &cert, "cert", &f);
  if (cert.kind < KZ_CERT_KIND_FIXED_ORDER || cert.kind > KZ_CERT_KIND_INACTIVE) add(&f, "cert", "kind is unset or out of range");
  if (cert.mlevel < KZ_MLEVEL_API || cert.mlevel > KZ_MLEVEL_IMPL) add(&f, "cert", "model level is unset or out of range");
  if ((cert.flags & ~(KZ_CERT_FLAG_DECIDES | KZ_CERT_FLAG_CONTESTED)) != 0) add(&f, "cert", "flags has unknown bits");
  const bool contested = (cert.flags & KZ_CERT_FLAG_CONTESTED) != 0;
  if (contested != !is_zero(cert.contested_profile, sizeof(cert.contested_profile)))
    add(&f, "cert", "contested mark and contested_profile disagree (the profile hash is set exactly when CONTESTED is)");
  if (contested && ref_is_null(cert.contested_by)) add(&f, "cert", "a contested mark names no witness (O10)");
  if (cert.proof_status > KZ_PROOF_PROVED) add(&f, "cert", "proof status out of range");
  if (cert.proof_status != weakest_proof(entries))
    add(&f, "cert", std::string("proof_status ") + proof_status_name(cert.proof_status) + " is not the weakest status among the lemmas of its ledger (" +
                        proof_status_name(weakest_proof(entries)) + ")");
  return f;
}

// ---- rendering -----------------------------------------------------------------------

namespace {

std::string bound_text(const kz_bound* b) { return b == nullptr ? "?" : b->h == KZ_BOUND_INFINITE ? "inf" : std::to_string(b->h); }

// "model(b)" -> "model(b=12)", "no-stall-within(b)" -> "no-stall-within(b=?)"; other names unchanged.
std::string with_bound(const char* name, const kz_bound* b) {
  std::string s = name;
  const std::string tail = "(b)";
  if (s.size() >= tail.size() && s.compare(s.size() - tail.size(), tail.size(), tail) == 0) s.replace(s.size() - tail.size(), tail.size(), "(b=" + bound_text(b) + ")");
  return s;
}

// Display names of the members of a set, comma separated (no braces).
std::string set_names(const char* enum_name, uint64_t value, uint64_t* rest) {
  std::string out;
  uint64_t left = value;
  const EnumDesc* en = find_enum(enum_name);
  if (en != nullptr)
    for (size_t i = 0; i < en->nitems; ++i)
      if (en->items[i].value != 0 && (value & en->items[i].value) != 0) {
        if (!out.empty()) out += ",";
        out += en->items[i].display;
        left &= ~en->items[i].value;
      }
  if (rest != nullptr) *rest = left;
  return out;
}

// The statements and witnesses that a cell keeps even when its class says less: they are what
// SPEC 1.10 asks to show for unknown(no_decision_route), unknown(one_sided) and for the classes
// that rest on witnesses.
std::string retained(const kz_verdict_cell& c, bool statements) {
  std::string out;
  auto item = [&](const std::string& text) { out += (out.empty() ? "" : "; ") + text; };
  if (statements) {
    const struct {
      const char* name;
      unsigned shift;
    } kinds[] = {{"not-Bad", KZ_ROUTE_NOT_BAD_SHIFT}, {"not-OK", KZ_ROUTE_NOT_OK_SHIFT}, {"not-NR", KZ_ROUTE_NOT_NR_SHIFT}};
    for (const auto& k : kinds) {
      const uint8_t r = route_get(c.safety_routes, k.shift);
      if (r != KZ_ROUTE_NONE) item(std::string(k.name) + " (" + enum_text("KZ_ROUTE", r) + ")");
    }
  }
  if (!ref_is_null(c.witness_bad)) item("bad witness " + ref_to_text(c.witness_bad));
  if (!ref_is_null(c.witness_good)) item("good witness " + ref_to_text(c.witness_good));
  return out;
}

std::string stall_text(const kz_verdict_cell& c, const kz_bound* progress_bound) {
  uint64_t rest = 0;
  std::string kind;
  if ((c.progress_flags & KZ_PROGRESS_FLAG_DEADLOCK_CYCLE) != 0) kind = "deadlock-cycle";
  if ((c.progress_flags & KZ_PROGRESS_FLAG_LEAKED_LOCK) != 0) kind += (kind.empty() ? "" : ",") + std::string("leaked-lock");
  if (kind.empty()) kind = "other";
  const uint64_t kind_bits = KZ_PROGRESS_FLAG_DEADLOCK_CYCLE | KZ_PROGRESS_FLAG_LEAKED_LOCK;
  std::string s = "stall-witness(S={" + set_names("KZ_PROGRESS_KIND", c.progress_kind, &rest) + "}; kind=" + kind + "; qualifiers={" +
                  set_names("KZ_PROGRESS_FLAG", c.progress_flags & ~kind_bits, &rest) + "}; harm=" + enum_text("KZ_PROGRESS_HARM", c.progress_harm) +
                  "; realization=" + with_bound(enum_text("KZ_PROGRESS_REALIZATION", c.progress_realization), progress_bound) +
                  "; wait-for root: in witness " + ref_to_text(c.witness_stall) + ")";
  if ((c.progress_flags & KZ_PROGRESS_FLAG_TERMINAL_PENDING) != 0) s += " relative to the closed cone";
  return s;
}

// The weakest status that a ledger's lemma counts imply (the rule of weakest_proof).
uint8_t status_from_counts(const LedgerCounts& c) {
  if (c.lemma_unproved > 0) return KZ_PROOF_UNPROVED;
  if (c.lemma_depends > 0) return KZ_PROOF_DEPENDS;
  if (c.lemma_proved > 0) return KZ_PROOF_PROVED;
  return KZ_PROOF_NA;
}

std::string lemma_counts_text(const LedgerCounts& c) {
  std::string s = lemmas_label(c);
  if (!c.lemma_not_proved.empty()) {
    s += "; not proved:";
    for (size_t i = 0; i < c.lemma_not_proved.size(); ++i) s += (i == 0 ? " #" : ", #") + std::to_string(c.lemma_not_proved[i]);
  }
  return s;
}

// "lemma status: ..." (PS2, PS3).  A status that was not checked against a ledger, or that
// disagrees with it, is never printed as a plain `proved`.
std::string lemma_status_text(uint8_t stored, const LedgerCounts* lemmas) {
  if (lemmas == nullptr) {
    std::string s = std::string("lemma status: ") + proof_status_name(stored);
    if (stored == KZ_PROOF_PROVED) s += " (not checked against a ledger)";
    return s;
  }
  const uint8_t derived = status_from_counts(*lemmas);
  if (derived != stored)
    return std::string("lemma status: INCONSISTENT (the record says ") + proof_status_name(stored) + ", its ledger says " + proof_status_name(derived) + "; " +
           lemma_counts_text(*lemmas) + ")";
  return std::string("lemma status: ") + proof_status_name(stored) + " (" + lemma_counts_text(*lemmas) + ")";
}

}  // namespace

std::string render_cell(const kz_verdict_cell& c, const LedgerCounts* lemmas, const kz_bound* primary, const kz_bound* progress_bound) {
  const uint8_t not_bad = route_get(c.safety_routes, KZ_ROUTE_NOT_BAD_SHIFT);
  const uint8_t not_ok = route_get(c.safety_routes, KZ_ROUTE_NOT_OK_SHIFT);
  const uint8_t not_nr = route_get(c.safety_routes, KZ_ROUTE_NOT_NR_SHIFT);
  const bool universal = c.cls == KZ_CLS_PROTECTED || c.cls == KZ_CLS_FIXED || c.cls == KZ_CLS_NO_FINAL;
  std::string s;
  if (c.cls == KZ_CLS_UNKNOWN || c.cls == KZ_CLS_CONTESTED) {
    s = std::string("unknown(") + (c.cls == KZ_CLS_CONTESTED ? "model_fault" : enum_text("KZ_UNKNOWN_REASON", c.unknown_reason)) + ")";
    const std::string kept = retained(c, true);
    if (!kept.empty()) s += " - retains " + kept;
  } else {
    s = enum_text("KZ_CLS", c.cls);
    switch (c.cls) {
      case KZ_CLS_PROTECTED:
        s += not_nr != KZ_ROUTE_NONE ? " - every reached occurrence READY" : " - no reached occurrence violates";
        s += std::string(" (") + enum_text("KZ_ROUTE", not_nr != KZ_ROUTE_NONE ? not_nr : not_bad) + ")";
        break;
      case KZ_CLS_FIXED:
        s += std::string(" - no reached occurrence READY (") + enum_text("KZ_ROUTE", not_ok) + "; " +
             ((c.flags & KZ_CELL_FLAG_DECIDES) != 0 ? "decision route" : "no decision route") + ")";
        break;
      case KZ_CLS_NO_FINAL:
        s += std::string(" (not-Bad ") + enum_text("KZ_ROUTE", not_bad) + ", not-OK " + enum_text("KZ_ROUTE", not_ok) + ")";
        break;
      case KZ_CLS_SCHED_DEP:
      case KZ_CLS_ORDINAL_SPLIT: {
        const std::string kept = retained(c, false);
        if (!kept.empty()) s += " - " + kept;
        break;
      }
      default:
        break;
    }
  }
  if (primary != nullptr) s += primary->h == KZ_BOUND_INFINITE ? " [b=inf]" : " [b=" + std::to_string(primary->h) + "]";
  else if (universal) s += " [b=?]";
  s += "; reach " + with_bound(enum_text("KZ_REACH", c.reach), primary);
  s += "; progress ";
  switch (c.progress) {
    case KZ_PROGRESS_UNKNOWN: s += std::string("unknown(") + enum_text("KZ_PROGRESS_REASON", c.progress_reason) + ")"; break;
    case KZ_PROGRESS_CONTESTED: s += "unknown(model_fault)"; break;
    case KZ_PROGRESS_STALL_WITNESS: s += stall_text(c, progress_bound); break;
    default: s += with_bound(enum_text("KZ_PROGRESS", c.progress), progress_bound); break;
  }
  // PS3: next to every certificate-based statement, the weakest proof status of its lemmas.
  if (certificate_based(c) || c.proof_status != KZ_PROOF_NA || (lemmas != nullptr && status_from_counts(*lemmas) != KZ_PROOF_NA))
    s += "; " + lemma_status_text(c.proof_status, lemmas);
  return s;
}

std::string render_acceptance(const kz_cert& cert, const LedgerCounts* lemmas) {
  return "accepted by kz proofcheck, relative to its trusted base; " + lemma_status_text(cert.proof_status, lemmas);
}

// ---- summary view ---------------------------------------------------------------------

kz_summary summarize_observation(const SummaryInput& in) {
  kz_summary s;
  std::memset(&s, 0, sizeof s);
  s.cell = KZ_NONE32;
  s.model = KZ_NONE32;
  s.adequacy = KZ_NONE32;
  s.identity_first = KZ_NONE32;
  s.prediction = KZ_PRED_NOT_RUN;
  s.confirmation = KZ_CONF_NOT_RUN;
  s.status = in.status;
  s.status_flags = in.status_flags;
  if (in.caller_record) s.flags |= KZ_SUMMARY_FLAG_CALLER_RECORD;
  if (in.accepted) s.flags |= KZ_SUMMARY_FLAG_ACCEPT;
  if (in.input_error) {
    s.flags |= KZ_SUMMARY_FLAG_INPUT_ERROR;
    s.observation = KZ_OBS_NONE;
    return s;
  }
  auto inconclusive = [&](uint32_t why) {
    s.observation = KZ_OBS_INCONCLUSIVE;
    s.obs_reasons = why;
  };
  if ((in.status_flags & KZ_STATUS_FLAG_OPEN) != 0) {
    inconclusive(KZ_WITHHOLD_HORIZON_OPEN | in.reasons);
    return s;
  }
  if ((in.status_flags & KZ_STATUS_FLAG_UNDETERMINED) != 0) {
    inconclusive(in.reasons != 0 ? in.reasons : static_cast<uint32_t>(KZ_WITHHOLD_EXPLANATION_AMBIGUOUS));
    return s;
  }
  switch (in.status) {
    case KZ_OCC_VIOLATING:
      if (in.all_explanations) s.observation = KZ_OBS_OBSERVED_VIOLATION;
      else inconclusive(KZ_WITHHOLD_EXPLANATION_AMBIGUOUS | in.reasons);
      break;
    case KZ_OCC_READY: s.observation = KZ_OBS_NO_VIOLATION_IN_CHECKED_PREFIX; break;
    case KZ_OCC_TOLERATED:
      if (in.caller_record && in.accepted) s.observation = KZ_OBS_NO_VIOLATION_IN_CHECKED_PREFIX;
      else inconclusive(KZ_WITHHOLD_CALLER_REQUIREMENT_ABSENT);
      break;
    case KZ_OCC_NOT_READY_UNJUDGED: inconclusive(KZ_WITHHOLD_CALLER_REQUIREMENT_ABSENT); break;
    case KZ_OCC_PENDING: inconclusive(KZ_WITHHOLD_HORIZON_OPEN); break;
    case KZ_OCC_NOT_EVALUATED: inconclusive(KZ_WITHHOLD_NOT_EVALUATED); break;
    default: inconclusive(KZ_WITHHOLD_EXPLANATION_AMBIGUOUS); break;  // a value outside KZ_OCC claims nothing
  }
  return s;
}

std::string render_observation(const kz_summary& s) {
  if ((s.flags & KZ_SUMMARY_FLAG_INPUT_ERROR) != 0) return "analysis input error (reported apart)";
  std::string out = enum_text("KZ_OBS", s.observation);
  if (s.observation == KZ_OBS_INCONCLUSIVE) {
    const EnumDesc* en = find_enum("KZ_WITHHOLD");
    std::string r;
    if (en != nullptr)
      for (size_t i = 0; i < en->nitems; ++i)
        if ((s.obs_reasons & en->items[i].value) != 0) r += (r.empty() ? "" : ",") + std::string(en->items[i].display);
    out += "(" + r + ")";
  }
  return out;
}

std::vector<Finding> validate_summary(const kz_summary& s) {
  std::vector<Finding> out;
  check_reserved("kz_summary", &s, "summary", &out);
  const bool input_error = (s.flags & KZ_SUMMARY_FLAG_INPUT_ERROR) != 0;
  if (input_error && s.observation != KZ_OBS_NONE) add(&out, "summary", "an analysis input error is reported apart: its observation must be none");
  if (s.observation == KZ_OBS_INCONCLUSIVE && s.obs_reasons == 0) add(&out, "summary", "INCONCLUSIVE names no reason");
  if (s.observation != KZ_OBS_INCONCLUSIVE && s.obs_reasons != 0) add(&out, "summary", "only INCONCLUSIVE carries reasons");
  if (s.observation > KZ_OBS_NOT_APPLICABLE) add(&out, "summary", "observation value out of range");
  const bool definite = s.status_flags == 0;
  if (s.observation == KZ_OBS_OBSERVED_VIOLATION && !(definite && s.status == KZ_OCC_VIOLATING))
    add(&out, "summary", "OBSERVED_VIOLATION needs the status VIOLATING");
  if (s.observation == KZ_OBS_NO_VIOLATION_IN_CHECKED_PREFIX) {
    const bool tolerated_ok = s.status == KZ_OCC_TOLERATED && (s.flags & KZ_SUMMARY_FLAG_CALLER_RECORD) != 0 && (s.flags & KZ_SUMMARY_FLAG_ACCEPT) != 0;
    if (!(definite && (s.status == KZ_OCC_READY || tolerated_ok)))
      add(&out, "summary", "NO_VIOLATION_IN_CHECKED_PREFIX needs READY, or TOLERATED with a caller record and ACCEPT (D2)");
  }
  if ((s.status_flags & KZ_STATUS_FLAG_OPEN) != 0 && s.observation != KZ_OBS_INCONCLUSIVE && !input_error)
    add(&out, "summary", "an OPEN occurrence is INCONCLUSIVE(horizon-open) (D2)");
  if (definite && s.status == KZ_OCC_PENDING && s.observation != KZ_OBS_INCONCLUSIVE && !input_error)
    add(&out, "summary", "a PENDING occurrence is INCONCLUSIVE(horizon-open) (D2)");
  if (definite && s.status == KZ_OCC_NOT_READY_UNJUDGED && s.observation != KZ_OBS_INCONCLUSIVE && !input_error)
    add(&out, "summary", "a NOT_READY_UNJUDGED occurrence is INCONCLUSIVE(caller_requirement_absent)");
  return out;
}

}  // namespace kz

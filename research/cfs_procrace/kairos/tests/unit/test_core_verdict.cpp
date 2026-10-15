// Verdict, cell, certificate, ledger and proof-status records: conservative defaults (D20),
// the invariants of SPEC 1.10, 2.3, 4.5.1 and 6.2 (PS1-PS4), labels, and the rendering rules.

#include <cstring>

#include "kz/core/verdict.hpp"
#include "support/fixtures.hpp"
#include "support/kz_test.hpp"

namespace {

bool has(const std::string& text, const std::string& part) { return text.find(part) != std::string::npos; }

bool mentions(const std::vector<kz::Finding>& f, const std::string& part) {
  for (const kz::Finding& x : f)
    if (has(x.what, part)) return true;
  return false;
}

kz_ref some_ref(uint32_t section = KZ_SEC_RUNMETA) {
  kz_ref r = kzfix::zero<kz_ref>();
  r.section = section;
  r.index = 1;
  return r;
}

kz_trust lemma(uint8_t proof, bool with_source = false) {
  kz_trust t = kz::make_trust_entry(KZ_TRUST_PART_TRUSTED, KZ_TRUST_KIND_LEMMA);
  t.proof = proof;
  if (with_source) t.source = some_ref(KZ_SEC_PROVENANCE);
  return t;
}

void test_defaults() {
  // The default cell establishes nothing and says so; the zero record has progress
  // unknown(not_checked) by construction.
  kz_verdict_cell c = kz::make_cell(KZ_SIGMA_U, KZ_MLEVEL_API);
  KZ_CHECK_EQ(c.cls, static_cast<uint8_t>(KZ_CLS_UNKNOWN));
  KZ_CHECK_EQ(c.unknown_reason, static_cast<uint8_t>(KZ_UNKNOWN_REASON_NOT_REACHED));
  KZ_CHECK_EQ(c.reach, static_cast<uint8_t>(KZ_REACH_ASSUMED));
  KZ_CHECK_EQ(c.progress, static_cast<uint8_t>(KZ_PROGRESS_UNKNOWN));
  KZ_CHECK_EQ(c.progress_reason, static_cast<uint8_t>(KZ_PROGRESS_REASON_NOT_CHECKED));
  KZ_CHECK_EQ(c.safety_routes, static_cast<uint8_t>(0));
  KZ_CHECK_EQ(c.fw_conf, static_cast<uint8_t>(KZ_FW_CONF_UNKNOWN));
  KZ_CHECK_EQ(c.proof_status, static_cast<uint8_t>(KZ_PROOF_NA));
  KZ_CHECK(kz::validate_cell(c).empty());
  kz_verdict_cell zero = kzfix::zero<kz_verdict_cell>();
  KZ_CHECK_EQ(zero.progress, static_cast<uint8_t>(KZ_PROGRESS_UNKNOWN));
  KZ_CHECK_EQ(zero.progress_reason, static_cast<uint8_t>(KZ_PROGRESS_REASON_NOT_CHECKED));
  kz_verdict v = kz::make_verdict(7);
  KZ_CHECK_EQ(v.harm, static_cast<uint8_t>(KZ_HARM_UNSPECIFIED));
  KZ_CHECK_EQ(v.provider, static_cast<uint8_t>(KZ_PROVIDER_UNKNOWN));
  KZ_CHECK_EQ(v.rsv1, static_cast<uint8_t>(0));
  KZ_CHECK(kz::validate_verdict(v, {}).empty());
  // A lemma entry starts unproved; the bound of an entry is infinity unless set.
  kz_trust t = kz::make_trust_entry(KZ_TRUST_PART_TRUSTED, KZ_TRUST_KIND_LEMMA);
  KZ_CHECK_EQ(t.proof, static_cast<uint8_t>(KZ_PROOF_UNPROVED));
  KZ_CHECK_EQ(t.bound, static_cast<uint32_t>(KZ_BOUND_INFINITE));
  KZ_CHECK_EQ(kz::make_trust_entry(KZ_TRUST_PART_EXTRACTED, KZ_TRUST_KIND_FACT).proof, static_cast<uint8_t>(KZ_PROOF_NA));
}

void test_routes() {
  uint8_t r = 0;
  r = kz::route_set(r, KZ_ROUTE_NOT_OK_SHIFT, KZ_ROUTE_CERT);
  KZ_CHECK_EQ(r, static_cast<uint8_t>(0x04));
  r = kz::route_set(r, KZ_ROUTE_NOT_BAD_SHIFT, KZ_ROUTE_TRANSFERRED);
  r = kz::route_set(r, KZ_ROUTE_NOT_NR_SHIFT, KZ_ROUTE_MODEL);
  KZ_CHECK_EQ(kz::route_get(r, KZ_ROUTE_NOT_BAD_SHIFT), static_cast<uint8_t>(KZ_ROUTE_TRANSFERRED));
  KZ_CHECK_EQ(kz::route_get(r, KZ_ROUTE_NOT_OK_SHIFT), static_cast<uint8_t>(KZ_ROUTE_CERT));
  KZ_CHECK_EQ(kz::route_get(r, KZ_ROUTE_NOT_NR_SHIFT), static_cast<uint8_t>(KZ_ROUTE_MODEL));
  r = kz::route_set(r, KZ_ROUTE_NOT_OK_SHIFT, KZ_ROUTE_NONE);
  KZ_CHECK_EQ(kz::route_get(r, KZ_ROUTE_NOT_OK_SHIFT), static_cast<uint8_t>(0));
  KZ_CHECK_EQ(kz::route_get(r, KZ_ROUTE_NOT_BAD_SHIFT), static_cast<uint8_t>(KZ_ROUTE_TRANSFERRED));  // others untouched
}

void test_cell_invariants() {
  auto base = [] { return kz::make_cell(KZ_SIGMA_U, KZ_MLEVEL_API); };
  {  // a valid fixed-order cell from a certificate with a decision route
    kz_verdict_cell c = base();
    c.cls = KZ_CLS_FIXED;
    c.unknown_reason = KZ_UNKNOWN_REASON_NONE;
    c.flags = KZ_CELL_FLAG_DECIDES;
    c.safety_routes = kz::route_set(0, KZ_ROUTE_NOT_OK_SHIFT, KZ_ROUTE_CERT);
    c.certificate = some_ref(KZ_SEC_CERTIFICATES);
    c.realization = KZ_REALIZATION_CERT;
    c.proof_status = KZ_PROOF_UNPROVED;
    c.reach = KZ_REACH_PROVED;
    c.runs = some_ref();
    KZ_CHECK(kz::validate_cell(c).empty());
    // ... without the certificate it cites
    kz_verdict_cell d = c;
    d.certificate = kzfix::zero<kz_ref>();
    KZ_CHECK(mentions(kz::validate_cell(d), "names no certificate"));
    // ... without a decision route it is not fixed-order
    d = c;
    d.flags = 0;
    KZ_CHECK(mentions(kz::validate_cell(d), "decision route"));
    // ... with both statements it is no-final-status
    d = c;
    d.safety_routes = kz::route_set(c.safety_routes, KZ_ROUTE_NOT_BAD_SHIFT, KZ_ROUTE_CERT);
    KZ_CHECK(mentions(kz::validate_cell(d), "no-final-status"));
    // reach proved needs a run
    d = c;
    d.runs = kzfix::zero<kz_ref>();
    KZ_CHECK(mentions(kz::validate_cell(d), "reach proved without a run"));
    // PS3: a certificate-based statement carries the proof status of its lemmas
    d = c;
    d.proof_status = KZ_PROOF_NA;
    KZ_CHECK(mentions(kz::validate_cell(d), "no proof status"));
    d.proof_status = 9;
    KZ_CHECK(mentions(kz::validate_cell(d), "proof status out of range"));
  }
  {  // protected needs not-Bad and no not-OK; not-NR implies not-Bad and never comes from the explorer
    kz_verdict_cell c = base();
    c.cls = KZ_CLS_PROTECTED;
    c.unknown_reason = KZ_UNKNOWN_REASON_NONE;
    c.certificate = some_ref(KZ_SEC_CERTIFICATES);
    c.proof_status = KZ_PROOF_DEPENDS;
    c.safety_routes = kz::route_set(0, KZ_ROUTE_NOT_BAD_SHIFT, KZ_ROUTE_CERT);
    KZ_CHECK(kz::validate_cell(c).empty());
    kz_verdict_cell d = c;
    d.safety_routes = 0;
    KZ_CHECK(mentions(kz::validate_cell(d), "without a not-Bad"));
    d = c;
    d.safety_routes = kz::route_set(c.safety_routes, KZ_ROUTE_NOT_OK_SHIFT, KZ_ROUTE_MODEL);
    KZ_CHECK(mentions(kz::validate_cell(d), "not-OK statement"));
    d = c;
    d.safety_routes = kz::route_set(c.safety_routes, KZ_ROUTE_NOT_NR_SHIFT, KZ_ROUTE_MODEL);
    KZ_CHECK(mentions(kz::validate_cell(d), "certificate or a transfer"));
    d = c;
    d.safety_routes = kz::route_set(0, KZ_ROUTE_NOT_NR_SHIFT, KZ_ROUTE_CERT);
    d.cls = KZ_CLS_UNKNOWN;
    d.unknown_reason = KZ_UNKNOWN_REASON_MODEL_GAP;
    KZ_CHECK(mentions(kz::validate_cell(d), "not-NR without not-Bad"));
  }
  {  // honest unknowns
    kz_verdict_cell c = base();
    c.unknown_reason = KZ_UNKNOWN_REASON_NONE;
    KZ_CHECK(mentions(kz::validate_cell(c), "unknown without a reason"));
    c = base();
    c.cls = KZ_CLS_INACTIVE;  // a reason on a class that is not unknown
    KZ_CHECK(mentions(kz::validate_cell(c), "unknown reason is set"));
    c = base();
    c.cls = KZ_CLS_CONTESTED;
    KZ_CHECK(mentions(kz::validate_cell(c), "model_fault"));
    c.unknown_reason = KZ_UNKNOWN_REASON_MODEL_FAULT;
    KZ_CHECK(kz::validate_cell(c).empty());
  }
  {  // schedule-dependent needs both witnesses
    kz_verdict_cell c = base();
    c.cls = KZ_CLS_SCHED_DEP;
    c.unknown_reason = KZ_UNKNOWN_REASON_NONE;
    KZ_CHECK(mentions(kz::validate_cell(c), "bad and a good witness"));
    c.witness_bad = some_ref(KZ_SEC_WITNESSES);
    c.witness_good = some_ref(KZ_SEC_WITNESSES);
    KZ_CHECK(kz::validate_cell(c).empty());
  }
  {  // progress: unknown(not_checked) by default; stall details only with a witness
    kz_verdict_cell c = base();
    c.progress_harm = KZ_PROGRESS_HARM_HANG_OR_DEADLOCK;
    KZ_CHECK(mentions(kz::validate_cell(c), "progress unknown carries stall details"));
    c = base();
    c.progress = KZ_PROGRESS_STALL_WITNESS;
    KZ_CHECK(mentions(kz::validate_cell(c), "stall witness without a kind"));
    c.progress_kind = KZ_PROGRESS_KIND_IN_USE;
    c.progress_realization = KZ_PROGRESS_REALIZATION_MODEL;
    c.witness_stall = some_ref(KZ_SEC_WITNESSES);
    KZ_CHECK(kz::validate_cell(c).empty());
    c = base();
    c.progress = KZ_PROGRESS_NO_STALL_B;
    KZ_CHECK(mentions(kz::validate_cell(c), "names no bound"));
    c.progress_bounds = some_ref(KZ_SEC_BOUNDS);
    KZ_CHECK(kz::validate_cell(c).empty());
    c = base();
    c.progress = KZ_PROGRESS_CONTESTED;
    KZ_CHECK(mentions(kz::validate_cell(c), "model_fault"));
    c.progress_reason = KZ_PROGRESS_REASON_MODEL_FAULT;
    KZ_CHECK(kz::validate_cell(c).empty());
  }
  {  // framework conformance is not claimed at M_api
    kz_verdict_cell c = base();
    c.fw_conf = KZ_FW_CONF_AS_DOCUMENTED;
    KZ_CHECK(mentions(kz::validate_cell(c), "M_api cell has fw_conf unknown"));
    c.mlevel = KZ_MLEVEL_IMPL;
    KZ_CHECK(kz::validate_cell(c).empty());
  }
  {  // out-of-range values and unset axes
    kz_verdict_cell c = base();
    c.sigma = 0;
    c.mlevel = 0;
    c.cls = 99;
    c.safety_routes = 0xC0;
    c.rsv2[1] = 1;
    std::vector<kz::Finding> f = kz::validate_cell(c);
    KZ_CHECK(mentions(f, "sigma is unset"));
    KZ_CHECK(mentions(f, "model level is unset"));
    KZ_CHECK(mentions(f, "class value out of range"));
    KZ_CHECK(mentions(f, "bits 6-7"));
    KZ_CHECK(mentions(f, "reserved field rsv2"));
  }
}

void test_verdict_invariants() {
  kz_verdict v = kz::make_verdict(1);
  std::vector<kz_verdict_cell> cells = {kz::make_cell(KZ_SIGMA_U, KZ_MLEVEL_API), kz::make_cell(KZ_SIGMA_U, KZ_MLEVEL_IMPL)};
  v.ncells = 2;
  KZ_CHECK(kz::validate_verdict(v, cells).empty());
  kz_verdict d = v;
  d.ncells = 3;
  KZ_CHECK(mentions(kz::validate_verdict(d, cells), "ncells"));
  d = v;
  d.rsv1 = 1;  // the field that was `reach`
  KZ_CHECK(mentions(kz::validate_verdict(d, cells), "reach is per cell"));
  d = v;
  d.outcome_label = KZ_OUTCOME_LABEL_VIOLATION;  // no cell shows a not-ready outcome
  KZ_CHECK(mentions(kz::validate_verdict(d, cells), "outcome label"));
  d = v;
  d.scope = KZ_SCOPE_TEARDOWN;  // teardown scope: progress is unknown(teardown_scope)
  KZ_CHECK(mentions(kz::validate_verdict(d, cells), "teardown"));
  std::vector<kz_verdict_cell> td = cells;
  for (auto& c : td) c.progress_reason = KZ_PROGRESS_REASON_TEARDOWN_SCOPE;
  KZ_CHECK(kz::validate_verdict(d, td).empty());
  std::vector<kz_verdict_cell> dup = cells;
  dup[1] = dup[0];
  KZ_CHECK(mentions(kz::validate_verdict(v, dup), "duplicate"));
}

void test_proof_status() {
  // Names (SPEC 6.2) and order of strength.
  KZ_CHECK_EQ(std::string(kz::proof_status_name(KZ_PROOF_PROVED)), std::string("proved"));
  KZ_CHECK_EQ(std::string(kz::proof_status_name(KZ_PROOF_UNPROVED)), std::string("unproved"));
  KZ_CHECK_EQ(std::string(kz::proof_status_name(KZ_PROOF_DEPENDS)), std::string("depends-on-stated-assumption"));
  KZ_CHECK_EQ(std::string(kz::proof_status_name(KZ_PROOF_NA)), std::string("n/a"));
  // A listed lemma is unproved (PS2) and passes validation as such.
  kz_trust listed = kz::make_trust_entry(KZ_TRUST_PART_TRUSTED, KZ_TRUST_KIND_LEMMA);
  KZ_CHECK(kz::validate_trust_entry(listed).empty());
  // 'proved' points at the text that holds the proof (PS1): an entry cannot claim it by itself.
  KZ_CHECK(mentions(kz::validate_trust_entry(lemma(KZ_PROOF_PROVED)), "names no record of the text"));
  KZ_CHECK(kz::validate_trust_entry(lemma(KZ_PROOF_PROVED, true)).empty());
  KZ_CHECK(kz::validate_trust_entry(lemma(KZ_PROOF_DEPENDS)).empty());
  // A lemma has a status; nothing else does.
  KZ_CHECK(mentions(kz::validate_trust_entry(lemma(KZ_PROOF_NA)), "lemma has a proof status"));
  kz_trust fact = kz::make_trust_entry(KZ_TRUST_PART_EXTRACTED, KZ_TRUST_KIND_FACT);
  KZ_CHECK(kz::validate_trust_entry(fact).empty());
  fact.proof = KZ_PROOF_PROVED;
  KZ_CHECK(mentions(kz::validate_trust_entry(fact), "only a lemma has a proof status"));
  // Part and kind agree.
  kz_trust bad = kz::make_trust_entry(KZ_TRUST_PART_RECHECKED, KZ_TRUST_KIND_FACT);
  KZ_CHECK(mentions(kz::validate_trust_entry(bad), "part (i)"));
  kz_trust ob = kz::make_trust_entry(KZ_TRUST_PART_REFUTABLE, KZ_TRUST_KIND_OBLIGATION);
  ob.status = KZ_TRUST_STATUS_NOT_RUN;
  KZ_CHECK(mentions(kz::validate_trust_entry(ob), "names the entry it can refute"));
  ob.refutes = 2;
  KZ_CHECK(mentions(kz::validate_trust_entry(ob), "OBLIGATIONS record"));
  ob.source = some_ref(KZ_SEC_OBLIGATIONS);
  KZ_CHECK(kz::validate_trust_entry(ob).empty());

  // The weakest link (PS3).
  KZ_CHECK_EQ(kz::weakest_proof({}), static_cast<uint8_t>(KZ_PROOF_NA));
  KZ_CHECK_EQ(kz::weakest_proof({fact}), static_cast<uint8_t>(KZ_PROOF_NA));  // only lemmas count
  KZ_CHECK_EQ(kz::weakest_proof({lemma(KZ_PROOF_PROVED, true)}), static_cast<uint8_t>(KZ_PROOF_PROVED));
  KZ_CHECK_EQ(kz::weakest_proof({lemma(KZ_PROOF_PROVED, true), lemma(KZ_PROOF_DEPENDS)}), static_cast<uint8_t>(KZ_PROOF_DEPENDS));
  KZ_CHECK_EQ(kz::weakest_proof({lemma(KZ_PROOF_PROVED, true), lemma(KZ_PROOF_DEPENDS), lemma(KZ_PROOF_UNPROVED)}), static_cast<uint8_t>(KZ_PROOF_UNPROVED));
}

void test_ledger_and_certificate() {
  std::vector<kz_trust> e;
  e.push_back(kz::make_trust_entry(KZ_TRUST_PART_RECHECKED, KZ_TRUST_KIND_CERT_STEP));
  e.back().status = KZ_TRUST_STATUS_ACCEPTED;
  e.push_back(kz::make_trust_entry(KZ_TRUST_PART_EXTRACTED, KZ_TRUST_KIND_FACT));
  e.push_back(kz::make_trust_entry(KZ_TRUST_PART_TRUSTED, KZ_TRUST_KIND_DECLARED_TRUSTED));
  e.push_back(kz::make_trust_entry(KZ_TRUST_PART_TRUSTED, KZ_TRUST_KIND_DECLARED_TRUSTED));
  e.push_back(lemma(KZ_PROOF_PROVED, true));
  kz::LedgerCounts c = kz::count_ledger(e);
  KZ_CHECK_EQ(c.n_rechecked, 1u);
  KZ_CHECK_EQ(c.n_extracted, 1u);
  KZ_CHECK_EQ(c.n_trusted, 3u);
  KZ_CHECK_EQ(c.declared_trusted, 2u);
  KZ_CHECK_EQ(c.lemma_proved, 1u);
  KZ_CHECK_EQ(kz::trusted_label(c), std::string("trusted:2"));
  KZ_CHECK_EQ(kz::lemmas_label(c), std::string("lemmas:1/0/0"));
  kz_trust_ledger l = kz::make_ledger(e, 10);
  KZ_CHECK_EQ(l.first, 10u);
  KZ_CHECK_EQ(l.count, 5u);
  KZ_CHECK_EQ(kz::ledger_label(l), std::string("ledger:1/1/3/0"));
  KZ_CHECK(kz::validate_ledger(l, e).empty());

  e.push_back(lemma(KZ_PROOF_UNPROVED));
  e.push_back(lemma(KZ_PROOF_DEPENDS));
  c = kz::count_ledger(e);
  KZ_CHECK_EQ(kz::lemmas_label(c), std::string("lemmas:1/1/1"));  // <proved>/<depends>/<unproved>
  kz_trust_ledger bad = kz::make_ledger(e, 0);
  KZ_CHECK(kz::validate_ledger(bad, e).empty());
  kz_trust_ledger d = bad;
  d.n_trusted += 1;
  KZ_CHECK(mentions(kz::validate_ledger(d, e), "part sizes"));
  d = bad;
  d.count += 1;
  KZ_CHECK(mentions(kz::validate_ledger(d, e), "count does not match"));
  d = bad;
  d.facts = some_ref();  // references are not checked here; reserved fields are
  KZ_CHECK(kz::validate_ledger(d, e).empty());

  // A certificate carries the weakest status of its ledger, and the contested mark is consistent.
  kz_cert cert = kzfix::zero<kz_cert>();
  cert.kind = KZ_CERT_KIND_FIXED_ORDER;
  cert.mlevel = KZ_MLEVEL_API;
  cert.flags = KZ_CERT_FLAG_DECIDES;
  cert.proof_status = KZ_PROOF_UNPROVED;
  KZ_CHECK(kz::validate_cert(cert, e).empty());
  kz_cert x = cert;
  x.proof_status = KZ_PROOF_PROVED;  // claims more than its lemmas support
  KZ_CHECK(mentions(kz::validate_cert(x, e), "not the weakest status"));
  x = cert;
  x.proof_status = KZ_PROOF_NA;
  KZ_CHECK(mentions(kz::validate_cert(x, e), "not the weakest status"));
  x = cert;
  x.flags |= KZ_CERT_FLAG_CONTESTED;  // contested without the profile hash
  KZ_CHECK(mentions(kz::validate_cert(x, e), "contested mark"));
  x.contested_profile[0] = 1;
  KZ_CHECK(mentions(kz::validate_cert(x, e), "names no witness"));
  x.contested_by = some_ref(KZ_SEC_WITNESSES);
  KZ_CHECK(kz::validate_cert(x, e).empty());
  x = cert;
  x.kind = 0;
  x.mlevel = 0;
  KZ_CHECK(mentions(kz::validate_cert(x, e), "kind is unset"));
  KZ_CHECK(mentions(kz::validate_cert(x, e), "model level is unset"));
  // A cell's status must also be the weakest of its ledger.
  kz_verdict_cell cell = kz::make_cell(KZ_SIGMA_U, KZ_MLEVEL_API);
  cell.proof_status = KZ_PROOF_UNPROVED;
  KZ_CHECK(kz::validate_cell_proof(cell, e).empty());
  cell.proof_status = KZ_PROOF_DEPENDS;
  KZ_CHECK(mentions(kz::validate_cell_proof(cell, e), "weakest status"));
}

void test_rendering() {
  // A universal class is always rendered with reach and progress, and a certificate-based
  // statement with the proof status of its lemmas (PS3).
  kz_verdict_cell c = kz::make_cell(KZ_SIGMA_U, KZ_MLEVEL_API);
  c.cls = KZ_CLS_PROTECTED;
  c.unknown_reason = KZ_UNKNOWN_REASON_NONE;
  c.safety_routes = kz::route_set(kz::route_set(0, KZ_ROUTE_NOT_BAD_SHIFT, KZ_ROUTE_CERT), KZ_ROUTE_NOT_NR_SHIFT, KZ_ROUTE_CERT);
  c.reach = KZ_REACH_PROVED;
  c.proof_status = KZ_PROOF_UNPROVED;
  kz_bound b = kzfix::zero<kz_bound>();
  b.h = KZ_BOUND_INFINITE;
  KZ_CHECK_EQ(kz::render_cell(c, nullptr, &b), std::string("protected - every reached occurrence READY (cert) [b=inf]; reach proved; progress unknown(not_checked); lemma status: unproved"));
  b.h = 12;
  KZ_CHECK(has(kz::render_cell(c, nullptr, &b), "[b=12]"));

  kz_verdict_cell f = kz::make_cell(KZ_SIGMA_U, KZ_MLEVEL_IMPL);
  f.cls = KZ_CLS_FIXED;
  f.unknown_reason = KZ_UNKNOWN_REASON_NONE;
  f.safety_routes = kz::route_set(0, KZ_ROUTE_NOT_OK_SHIFT, KZ_ROUTE_TRANSFERRED);
  f.proof_status = KZ_PROOF_DEPENDS;
  // a universal class is always rendered with its bound: without one the text says so
  KZ_CHECK_EQ(kz::render_cell(f), std::string("fixed-order - no reached occurrence READY (transferred; no decision route) [b=?]; reach assumed; progress unknown(not_checked); lemma status: depends-on-stated-assumption"));
  f.flags = KZ_CELL_FLAG_DECIDES;
  KZ_CHECK(has(kz::render_cell(f), "decision route)"));

  // A cell that rests on no certificate and no lemma shows no lemma status.
  kz_verdict_cell u = kz::make_cell(KZ_SIGMA_U, KZ_MLEVEL_API);
  KZ_CHECK_EQ(kz::render_cell(u), std::string("unknown(not_reached); reach assumed; progress unknown(not_checked)"));
  u.progress = KZ_PROGRESS_STALL_WITNESS;
  KZ_CHECK(has(kz::render_cell(u), "progress stall-witness"));
  // ... but a certificate-based cell without a status says so instead of staying silent.
  kz_verdict_cell silent = c;
  silent.proof_status = KZ_PROOF_NA;
  KZ_CHECK(has(kz::render_cell(silent), "lemma status: n/a"));

  // Lemma counts, when given.
  std::vector<kz_trust> e = {lemma(KZ_PROOF_UNPROVED), lemma(KZ_PROOF_DEPENDS), lemma(KZ_PROOF_PROVED, true)};
  e[0].ident = 11;
  e[1].ident = 12;
  e[2].ident = 13;
  kz::LedgerCounts counts = kz::count_ledger(e);
  // PS3 names the lemmas that are not proved (by their ident in the lemma table)
  KZ_CHECK(has(kz::render_cell(c, &counts, &b), "lemma status: unproved (lemmas:1/1/1; not proved: #11, #12)"));

  // PS2/PS3: acceptance is always "relative to its trusted base" with the status; no sentence
  // says proved, verified or certified for a statement that rests on an unproved lemma.
  kz_cert cert = kzfix::zero<kz_cert>();
  cert.proof_status = KZ_PROOF_UNPROVED;
  const std::string acc = kz::render_acceptance(cert, &counts);
  KZ_CHECK_EQ(acc, std::string("accepted by kz proofcheck, relative to its trusted base; lemma status: unproved (lemmas:1/1/1; not proved: #11, #12)"));
  cert.proof_status = KZ_PROOF_PROVED;
  KZ_CHECK(has(kz::render_acceptance(cert), "relative to its trusted base; lemma status: proved"));
  for (uint8_t status : {KZ_PROOF_NA, KZ_PROOF_UNPROVED, KZ_PROOF_DEPENDS, KZ_PROOF_PROVED}) {
    cert.proof_status = status;
    kz_verdict_cell x = c;
    x.proof_status = status;
    for (const std::string& t : {kz::render_acceptance(cert, &counts), kz::render_cell(x, &counts, &b)}) {
      KZ_CHECK(!has(t, "verified") && !has(t, "certified"));
      if (status != KZ_PROOF_PROVED) KZ_CHECK(!has(t, "lemma status: proved"));
    }
  }

  // No rendering ever claims safety or liveness.
  for (uint8_t cls = KZ_CLS_UNKNOWN; cls <= KZ_CLS_CONTESTED; ++cls) {
    kz_verdict_cell x = kz::make_cell(KZ_SIGMA_U, KZ_MLEVEL_API);
    x.cls = cls;
    const std::string t = kz::render_cell(x, &counts, &b);
    KZ_CHECK(t.find("safe") == std::string::npos && t.find("live") == std::string::npos && t.find("deadlock-free") == std::string::npos);
    KZ_CHECK(has(t, "reach ") && has(t, "progress "));
  }
}

void test_fixture_is_consistent() {
  // The verdicts fixture used by the golden tests satisfies every invariant.
  const kz::Bytes facts = kzfix::build_facts_fixture();
  const kz::Bytes run = kzfix::build_run_fixture();
  kz::Container fc, rc, vc;
  KZ_CHECK_OK(kz::Container::parse(facts, &fc));
  KZ_CHECK_OK(kz::Container::parse(run, &rc));
  KZ_CHECK_OK(kz::Container::parse(kzfix::build_verdicts_fixture(fc.artifact_hash(), rc.artifact_hash()), &vc));
  std::vector<kz_verdict> v;
  std::vector<kz_verdict_cell> cells;
  std::vector<kz_trust> trust;
  std::vector<kz_trust_ledger> ledgers;
  std::vector<kz_cert> certs;
  KZ_CHECK_OK(vc.read_section(KZ_SEC_VERDICTS, &v));
  KZ_CHECK_OK(vc.read_section(KZ_SEC_VERDICT_CELLS, &cells));
  KZ_CHECK_OK(vc.read_section(KZ_SEC_TRUST, &trust));
  KZ_CHECK_OK(vc.read_section(KZ_SEC_TRUST_LEDGERS, &ledgers));
  KZ_CHECK_OK(vc.read_section(KZ_SEC_CERTIFICATES, &certs));
  KZ_CHECK_EQ(v.size(), static_cast<size_t>(1));
  KZ_CHECK_EQ(ledgers.size(), static_cast<size_t>(1));
  KZ_CHECK_EQ(certs.size(), static_cast<size_t>(1));
  auto show = [](const std::vector<kz::Finding>& f) {
    for (const kz::Finding& x : f) std::fprintf(stderr, "finding: %s: %s\n", x.where.c_str(), x.what.c_str());
    return f.empty();
  };
  KZ_CHECK(show(kz::validate_verdict(v[0], cells)));
  KZ_CHECK(show(kz::validate_ledger(ledgers[0], trust)));
  KZ_CHECK(show(kz::validate_cert(certs[0], trust)));
  KZ_CHECK(show(kz::validate_cell_proof(cells[0], trust)));
  KZ_CHECK_EQ(kz::ledger_label(ledgers[0]), std::string("ledger:2/1/4/1"));
  KZ_CHECK_EQ(kz::trusted_label(kz::count_ledger(trust)), std::string("trusted:1"));
  KZ_CHECK_EQ(kz::lemmas_label(kz::count_ledger(trust)), std::string("lemmas:0/1/1"));
  KZ_CHECK_EQ(certs[0].proof_status, static_cast<uint8_t>(KZ_PROOF_UNPROVED));
}


// Rules that earlier tests only matched through a neighbouring message or did not reach.
void test_rules_that_were_untested() {
  // An invalid proof status counts as unproved, never as proved (weakest_proof, count_ledger).
  KZ_CHECK_EQ(kz::weakest_proof({lemma(9)}), static_cast<uint8_t>(KZ_PROOF_UNPROVED));
  KZ_CHECK_EQ(kz::weakest_proof({lemma(KZ_PROOF_PROVED, true), lemma(9)}), static_cast<uint8_t>(KZ_PROOF_UNPROVED));
  // A LEMMA entry whose status is n/a is a defect of the entry, and it weighs as unproved.
  KZ_CHECK_EQ(kz::weakest_proof({lemma(KZ_PROOF_NA)}), static_cast<uint8_t>(KZ_PROOF_UNPROVED));
  KZ_CHECK_EQ(kz::weakest_proof({lemma(KZ_PROOF_PROVED, true), lemma(KZ_PROOF_NA)}), static_cast<uint8_t>(KZ_PROOF_UNPROVED));
  {
    const kz::LedgerCounts c = kz::count_ledger({lemma(9), lemma(KZ_PROOF_NA), lemma(KZ_PROOF_PROVED, true)});
    KZ_CHECK_EQ(c.lemma_unproved, 2u);
    KZ_CHECK_EQ(c.lemma_proved, 1u);
  }

  // The contested mark and the profile hash it names are set together, in both directions.
  const std::vector<kz_trust> none;
  kz_cert cert = kzfix::zero<kz_cert>();
  cert.kind = KZ_CERT_KIND_FIXED_ORDER;
  cert.mlevel = KZ_MLEVEL_API;
  kz_cert x = cert;
  x.flags = KZ_CERT_FLAG_CONTESTED;
  x.contested_by = some_ref(KZ_SEC_WITNESSES);
  KZ_CHECK(mentions(kz::validate_cert(x, none), "contested_profile disagree"));  // mark without a profile hash
  x.contested_profile[3] = 7;
  KZ_CHECK(!mentions(kz::validate_cert(x, none), "contested_profile disagree"));
  x = cert;
  x.contested_profile[3] = 7;  // a profile hash without the mark
  KZ_CHECK(mentions(kz::validate_cert(x, none), "contested_profile disagree"));
  KZ_CHECK(!mentions(kz::validate_cert(cert, none), "contested_profile disagree"));
}

void test_proof_sources() {
  // A lemma marked proved names a PROVENANCE record, and not any other kind of record.
  kz_trust t = lemma(KZ_PROOF_PROVED, true);
  KZ_CHECK(kz::validate_trust_entry(t).empty());
  t.source = some_ref(KZ_SEC_RUNMETA);
  KZ_CHECK(mentions(kz::validate_trust_entry(t), "must name a PROVENANCE record"));
  t.source = some_ref(KZ_SEC_TRUST);
  KZ_CHECK(mentions(kz::validate_trust_entry(t), "must name a PROVENANCE record"));
  t.source = kzfix::zero<kz_ref>();
  KZ_CHECK(mentions(kz::validate_trust_entry(t), "names no record of the text"));

  // The record must exist in this container, cite a text and carry its hash.
  std::vector<kz_prov> prov(2, kzfix::zero<kz_prov>());
  prov[1].file = kz_str{1, 4};
  prov[1].text_sha256[0] = 1;
  auto proved_at = [](uint32_t index) {
    kz_trust x = lemma(KZ_PROOF_PROVED, true);
    x.source.index = index;
    return x;
  };
  KZ_CHECK(kz::validate_proof_texts({proved_at(1)}, prov).empty());
  KZ_CHECK(mentions(kz::validate_proof_texts({proved_at(2)}, prov), "does not exist"));
  KZ_CHECK(mentions(kz::validate_proof_texts({proved_at(0)}, prov), "names no path or requirement id"));
  KZ_CHECK(mentions(kz::validate_proof_texts({proved_at(0)}, prov), "carries no hash of the text"));
  prov[1].text_sha256[0] = 0;
  KZ_CHECK(mentions(kz::validate_proof_texts({proved_at(1)}, prov), "carries no hash of the text"));
  prov[1].text_sha256[0] = 1;
  prov[1].file = kz_str{0, 0};
  KZ_CHECK(mentions(kz::validate_proof_texts({proved_at(1)}, prov), "names no path or requirement id"));
  // Entries that are not proved lemmas, and records of a parent, are not examined here.
  kz_trust foreign = proved_at(5);
  foreign.source.artifact_sha256[0] = 9;
  KZ_CHECK(kz::validate_proof_texts({foreign, lemma(KZ_PROOF_DEPENDS), lemma(KZ_PROOF_UNPROVED)}, prov).empty());
}

void test_rendering_rules_of_the_spec() {
  kz_bound inf = kzfix::zero<kz_bound>();
  inf.h = KZ_BOUND_INFINITE;
  kz_bound h12 = inf;
  h12.h = 12;
  kz_bound h7 = inf;
  h7.h = 7;

  // A universal class is always rendered with its bound; a missing bound is said to be missing.
  for (uint8_t cls : {KZ_CLS_PROTECTED, KZ_CLS_FIXED, KZ_CLS_NO_FINAL}) {
    kz_verdict_cell c = kz::make_cell(KZ_SIGMA_U, KZ_MLEVEL_API);
    c.cls = cls;
    c.unknown_reason = KZ_UNKNOWN_REASON_NONE;
    KZ_CHECK(has(kz::render_cell(c), " [b=?]"));
    KZ_CHECK(has(kz::render_cell(c, nullptr, &inf), " [b=inf]"));
    KZ_CHECK(has(kz::render_cell(c, nullptr, &h12), " [b=12]"));
  }
  // The bound completes reach model(b) / unreached(b) and progress no-stall-within(b).
  {
    kz_verdict_cell c = kz::make_cell(KZ_SIGMA_U, KZ_MLEVEL_IMPL);
    c.reach = KZ_REACH_MODEL_B;
    c.progress = KZ_PROGRESS_NO_STALL_B;
    c.progress_bounds = some_ref(KZ_SEC_BOUNDS);
    KZ_CHECK(has(kz::render_cell(c, nullptr, &h12, &h7), "reach model(b=12); progress no-stall-within(b=7)"));
    KZ_CHECK(has(kz::render_cell(c), "reach model(b=?); progress no-stall-within(b=?)"));
    c.reach = KZ_REACH_UNREACHED_B;
    KZ_CHECK(has(kz::render_cell(c, nullptr, &h12), "reach unreached(b=12)"));
  }

  // A stall witness is rendered with its clauses, kind, qualifiers, harm, realization and the witness that holds the root.
  {
    kz_verdict_cell c = kz::make_cell(KZ_SIGMA_U, KZ_MLEVEL_API);
    c.progress = KZ_PROGRESS_STALL_WITNESS;
    c.progress_kind = KZ_PROGRESS_KIND_PRE_USE | KZ_PROGRESS_KIND_HORIZON;
    c.progress_flags = KZ_PROGRESS_FLAG_DEADLOCK_CYCLE | KZ_PROGRESS_FLAG_IDLE | KZ_PROGRESS_FLAG_TERMINAL_PENDING;
    c.progress_harm = KZ_PROGRESS_HARM_TERMINAL_PENDING_PANIC;
    c.progress_realization = KZ_PROGRESS_REALIZATION_MODEL;
    c.witness_stall = some_ref(KZ_SEC_WITNESSES);
    const std::string t = kz::render_cell(c, nullptr, nullptr, &h12);
    KZ_CHECK(has(t, "stall-witness(S={pre-use,horizon}; kind=deadlock-cycle; qualifiers={idle,terminal-pending}; harm=terminal_pending(panic); "
                    "realization=model(b=12); wait-for root: in witness self/WITNESSES#1) relative to the closed cone"));
    c.progress_flags = KZ_PROGRESS_FLAG_LEAKED_LOCK;
    c.progress_kind = KZ_PROGRESS_KIND_IN_USE;
    c.progress_harm = KZ_PROGRESS_HARM_NONE;
    c.progress_realization = KZ_PROGRESS_REALIZATION_REALIZED;
    const std::string u = kz::render_cell(c);
    KZ_CHECK(has(u, "S={in-use}; kind=leaked-lock; qualifiers={}; harm=none; realization=realized;"));
    KZ_CHECK(!has(u, "relative to the closed cone"));
    c.progress_flags = 0;  // neither a cycle nor a leaked lock: kind other
    KZ_CHECK(has(kz::render_cell(c), "kind=other;"));
  }

  // unknown(no_decision_route) still shows the statement it holds; unknown(one_sided) the witnesses.
  {
    kz_verdict_cell c = kz::make_cell(KZ_SIGMA_U, KZ_MLEVEL_API);
    c.unknown_reason = KZ_UNKNOWN_REASON_NO_DECISION_ROUTE;
    c.safety_routes = kz::route_set(0, KZ_ROUTE_NOT_OK_SHIFT, KZ_ROUTE_CERT);
    KZ_CHECK(has(kz::render_cell(c), "unknown(no_decision_route) - retains not-OK (cert);"));
    kz_verdict_cell d = kz::make_cell(KZ_SIGMA_U, KZ_MLEVEL_API);
    d.unknown_reason = KZ_UNKNOWN_REASON_ONE_SIDED;
    d.witness_bad = some_ref(KZ_SEC_WITNESSES);
    KZ_CHECK(has(kz::render_cell(d), "unknown(one_sided) - retains bad witness self/WITNESSES#1;"));
    d.witness_good = some_ref(KZ_SEC_WITNESSES);
    KZ_CHECK(has(kz::render_cell(d), "retains bad witness self/WITNESSES#1; good witness self/WITNESSES#1;"));
    // ... and an unknown cell with nothing retained shows nothing extra
    KZ_CHECK(!has(kz::render_cell(kz::make_cell(KZ_SIGMA_U, KZ_MLEVEL_API)), "retains"));
    // schedule-dependent and ordinal-split show their witnesses
    kz_verdict_cell e = kz::make_cell(KZ_SIGMA_U, KZ_MLEVEL_API);
    e.cls = KZ_CLS_SCHED_DEP;
    e.unknown_reason = KZ_UNKNOWN_REASON_NONE;
    e.witness_bad = some_ref(KZ_SEC_WITNESSES);
    e.witness_good = some_ref(KZ_SEC_WITNESSES);
    KZ_CHECK(has(kz::render_cell(e), "schedule-dependent - bad witness self/WITNESSES#1; good witness self/WITNESSES#1;"));
  }

  // PS2/PS3: the status is checked against the ledger it comes from whenever the ledger is given.
  {
    std::vector<kz_trust> weak = {lemma(KZ_PROOF_UNPROVED), lemma(KZ_PROOF_PROVED, true)};
    weak[0].ident = 5;
    const kz::LedgerCounts counts = kz::count_ledger(weak);
    kz_verdict_cell c = kz::make_cell(KZ_SIGMA_U, KZ_MLEVEL_API);
    c.cls = KZ_CLS_PROTECTED;
    c.unknown_reason = KZ_UNKNOWN_REASON_NONE;
    c.safety_routes = kz::route_set(0, KZ_ROUTE_NOT_BAD_SHIFT, KZ_ROUTE_CERT);
    c.certificate = some_ref(KZ_SEC_CERTIFICATES);
    c.proof_status = KZ_PROOF_PROVED;  // claims more than its ledger supports
    const std::string over = kz::render_cell(c, &counts, &inf);
    KZ_CHECK(has(over, "lemma status: INCONSISTENT (the record says proved, its ledger says unproved; lemmas:1/0/1; not proved: #5)"));
    KZ_CHECK(!has(over, "lemma status: proved"));
    c.proof_status = KZ_PROOF_NA;  // says nothing although its ledger has lemmas
    KZ_CHECK(has(kz::render_cell(c, &counts, &inf), "lemma status: INCONSISTENT (the record says n/a, its ledger says unproved"));
    c.proof_status = KZ_PROOF_UNPROVED;
    KZ_CHECK(has(kz::render_cell(c, &counts, &inf), "lemma status: unproved (lemmas:1/0/1; not proved: #5)"));
    // `proved` with no ledger to compare with is not printed as plain proved either
    c.proof_status = KZ_PROOF_PROVED;
    KZ_CHECK(has(kz::render_cell(c, nullptr, &inf), "lemma status: proved (not checked against a ledger)"));
    kz_cert cert = kzfix::zero<kz_cert>();
    cert.proof_status = KZ_PROOF_PROVED;
    KZ_CHECK(has(kz::render_acceptance(cert, &counts), "relative to its trusted base; lemma status: INCONSISTENT (the record says proved, its ledger says unproved"));
    KZ_CHECK(has(kz::render_acceptance(cert), "lemma status: proved (not checked against a ledger)"));
    std::vector<kz_trust> good = {lemma(KZ_PROOF_PROVED, true)};
    const kz::LedgerCounts gc = kz::count_ledger(good);
    KZ_CHECK(has(kz::render_acceptance(cert, &gc), "lemma status: proved (lemmas:1/0/0)"));
    // a cell that rests on no certificate but whose ledger has lemmas shows the status anyway
    kz_verdict_cell plain = kz::make_cell(KZ_SIGMA_U, KZ_MLEVEL_API);
    KZ_CHECK(has(kz::render_cell(plain, &counts), "lemma status: INCONSISTENT"));
  }
}

}  // namespace

int main() {
  test_defaults();
  test_routes();
  test_cell_invariants();
  test_verdict_invariants();
  test_proof_status();
  test_ledger_and_certificate();
  test_rendering();
  test_rules_that_were_untested();
  test_proof_sources();
  test_rendering_rules_of_the_spec();
  test_fixture_is_consistent();
  return kztest::finish("test_core_verdict");
}

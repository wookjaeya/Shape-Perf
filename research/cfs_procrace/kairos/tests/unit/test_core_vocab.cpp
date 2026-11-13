// The vocabularies of the verdict record against the lists of the SPEC.
//
// The enumerations in include/kz/schema are the stored form of words that SPEC_FINAL.md fixes:
// the harm values of 1.6, and the classes, reasons, shapes, outcome labels, consequences and
// axes of 1.10.  Their display names are hashed into the schema fingerprint and so into every
// tool hash, and a golden file records them, but a golden file is updated with one command.
// This test writes the SPEC lists down a second time, with the place they come from, so that a
// rename in the SPEC (or a stale word in the header) makes a test fail instead of a golden diff.
//
// When the SPEC changes a word: change the header, this table and the goldens in the same commit.

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "kz/core/registry.hpp"
#include "support/kz_test.hpp"

namespace {

struct Item {
  uint64_t value;
  const char* display;
};

// Compares the enumeration `name` with the SPEC list: same values, same display names, same
// order, nothing missing and nothing extra.
void check_vocabulary(const char* name, const char* spec_place, const std::vector<Item>& spec) {
  const kz::EnumDesc* en = kz::find_enum(name);
  KZ_CHECK(en != nullptr);
  if (en == nullptr) {
    std::fprintf(stderr, "enumeration %s (%s) is not in the registry\n", name, spec_place);
    return;
  }
  KZ_CHECK_EQ(en->nitems, spec.size());
  for (size_t i = 0; i < en->nitems && i < spec.size(); ++i) {
    const bool same = en->items[i].value == spec[i].value && std::strcmp(en->items[i].display, spec[i].display) == 0;
    KZ_CHECK(same);
    if (!same)
      std::fprintf(stderr, "%s item %zu: header has %llu \"%s\", %s has %llu \"%s\"\n", name, i, static_cast<unsigned long long>(en->items[i].value),
                   en->items[i].display, spec_place, static_cast<unsigned long long>(spec[i].value), spec[i].display);
  }
}

void test_harm() {
  // SPEC 1.6 `harm` (as renamed by 0.9 item 4): an event request that the event service refuses is
  // "rejected", never "lost".  Value 0 is the default: unspecified by reference.
  check_vocabulary("KZ_HARM", "SPEC 1.6 harm",
                   {{0, "unspecified_by_reference"},
                    {1, "event_request_rejected"},
                    {2, "status_ignored"},
                    {3, "wrong_identity_stamped"},
                    {4, "app_error_exit"},
                    {5, "app_zombie"},
                    {6, "service_task_absent"},
                    {7, "processor_reset_requested"},
                    {8, "process_abort"},
                    {9, "panic"},
                    {10, "task_never_runs"},
                    {11, "hang_or_deadlock"},
                    {12, "terminal_pending(panic)"},
                    {13, "terminal_pending(abort)"},
                    {14, "message_lost"}});
}

void test_consequence_and_labels() {
  // SPEC 1.10 "Consequence": crash or undefined behaviour; deadlock; process abort; panic; task
  // lost; app error exit; app zombie; core service task absent; status ignored and effect lost
  // silently; event then continue; delayed but recovered; message lost; event rejected.  Value 0
  // is "not observed".
  check_vocabulary("KZ_CONSEQUENCE", "SPEC 1.10 consequence",
                   {{0, "not-observed"},
                    {1, "crash-or-ub"},
                    {2, "deadlock"},
                    {3, "process-abort"},
                    {4, "panic"},
                    {5, "task-lost"},
                    {6, "app-error-exit"},
                    {7, "app-zombie"},
                    {8, "core-service-task-absent"},
                    {9, "status-ignored-effect-lost"},
                    {10, "event-then-continue"},
                    {11, "delayed-but-recovered"},
                    {12, "message-lost"},
                    {13, "event-rejected"}});
  // SPEC 1.10 "Outcome label": violation / LOSS-UNDER-ORDER / not-ready-unjudged; none while no
  // cell shows a not-ready outcome.  (`liveness` belongs to the progress result, not to this field.)
  check_vocabulary("KZ_OUTCOME_LABEL", "SPEC 1.10 outcome label",
                   {{0, "none"}, {1, "violation"}, {2, "LOSS-UNDER-ORDER"}, {3, "not-ready-unjudged"}});
  // SPEC 1.10 "Shape": WRONG-WAY, UNORDERED, ABSENT, NOT-OBSERVED, STALE.
  check_vocabulary("KZ_SHAPE", "SPEC 1.10 shape",
                   {{0, "none"}, {1, "WRONG-WAY"}, {2, "UNORDERED"}, {3, "ABSENT"}, {4, "NOT-OBSERVED"}, {5, "STALE"}});
  // SPEC 1.10 "Orthogonal axes": provider and scope.
  check_vocabulary("KZ_PROVIDER", "SPEC 1.10 provider", {{0, "unknown"}, {1, "present"}, {2, "none"}, {3, "env-absent"}});
  check_vocabulary("KZ_SCOPE", "SPEC 1.10 scope", {{0, "in-scope"}, {1, "teardown"}});
  // SPEC 1.10 "Framework-level judgement": fw_doc (a set) and fw_conf.
  check_vocabulary("KZ_FW_DOC", "SPEC 1.10 fw_doc", {{1, "documented-not-error"}, {2, "documented-result"}, {4, "undocumented"}});
  check_vocabulary("KZ_FW_CONF", "SPEC 1.10 fw_conf", {{0, "unknown"}, {1, "as-documented"}, {2, "deviates"}});
  // SPEC 1.10 "Sub-labels".
  check_vocabulary("KZ_SUBLABEL", "SPEC 1.10 sub-labels",
                   {{0x1, "portability-hazard"},
                    {0x2, "implementation-defect-candidate"},
                    {0x4, "portability-hazard-evidence"},
                    {0x8, "assumed-visibility"},
                    {0x10, "A-ZT"},
                    {0x20, "tb-undeclared"}});
}

void test_classes_and_axes() {
  // SPEC 1.10 class table; `contested` is the class of a bound refuted by evidence (rendered unknown(model_fault)).
  check_vocabulary("KZ_CLS", "SPEC 1.10 class table",
                   {{0, "unknown"},
                    {1, "inactive"},
                    {2, "protected"},
                    {3, "fixed-order"},
                    {4, "no-final-status"},
                    {5, "schedule-dependent"},
                    {6, "ordinal-split"},
                    {7, "contested"}});
  // SPEC 1.10 `unknown(reason)`; value 0 is "no reason" (the class is not unknown).
  check_vocabulary("KZ_UNKNOWN_REASON", "SPEC 1.10 unknown(reason)",
                   {{0, "none"},
                    {1, "not_reached"},
                    {2, "model_gap"},
                    {3, "bound"},
                    {4, "budget"},
                    {5, "key_unresolved"},
                    {6, "loss"},
                    {7, "unmodelled"},
                    {8, "writer_incomplete"},
                    {9, "horizon"},
                    {10, "horizon_static"},
                    {11, "no_decision_route"},
                    {12, "one_sided"},
                    {13, "unordered"},
                    {14, "O-ordered-only"},
                    {15, "conditional-observation"},
                    {16, "conformance"},
                    {17, "model_fault"},
                    {18, "model_level"}});
  // SPEC 1.10 "reach" and "progress".
  check_vocabulary("KZ_REACH", "SPEC 1.10 reach",
                   {{0, "assumed"}, {1, "proved"}, {2, "model(b)"}, {3, "unreached(b)"}, {4, "contested"}});
  check_vocabulary("KZ_PROGRESS", "SPEC 1.10 progress",
                   {{0, "unknown"}, {1, "stall-witness"}, {2, "no-stall-within(b)"}, {3, "contested"}});
  // SPEC 1.10 progress harm: hang_or_deadlock | task_never_runs | terminal_pending(panic | abort).
  check_vocabulary("KZ_PROGRESS_HARM", "SPEC 1.10 stall witness harm",
                   {{0, "none"}, {1, "hang_or_deadlock"}, {2, "task_never_runs"}, {3, "terminal_pending(panic)"}, {4, "terminal_pending(abort)"}});
  // SPEC 1.10 stall witness realization: model(b) | realized.
  check_vocabulary("KZ_PROGRESS_REALIZATION", "SPEC 1.10 stall witness realization", {{0, "none"}, {1, "model(b)"}, {2, "realized"}});
  // SPEC 1.10 stall clauses: a non-empty set of {pre-use, in-use, horizon}.
  check_vocabulary("KZ_PROGRESS_KIND", "SPEC 1.10 stall clauses", {{0x1, "pre-use"}, {0x2, "in-use"}, {0x4, "horizon"}});
}

void test_provenance_and_proof() {
  // T1 (PI decision): the provenance tiers keep the names the PI fixed; the identifier avoids the retired word.
  check_vocabulary("KZ_TIER", "PI decision T1",
                   {{0, "unreferenced"}, {1, "contract-backed"}, {2, "admitted"}, {3, "code-derived"}, {4, "issue-derived"}, {5, "researcher"}});
  // SPEC 1.6 `prov`: kind in {api_doc, requirement, code_admission, code_pattern, issue, researcher},
  // side in {supplier, caller, rule}.
  check_vocabulary("KZ_PROV_KIND", "SPEC 1.6 prov kind",
                   {{0, "none"}, {1, "api_doc"}, {2, "requirement"}, {3, "code_admission"}, {4, "code_pattern"}, {5, "issue"}, {6, "researcher"}});
  check_vocabulary("KZ_PROV_SIDE", "SPEC 1.6 prov side", {{0, "n/a"}, {1, "supplier"}, {2, "caller"}, {3, "rule"}});
  // SPEC 6.2 (D20): the proof status of a lemma; n/a is "no lemma".
  check_vocabulary("KZ_PROOF", "SPEC 6.2 proof status",
                   {{0, "n/a"}, {1, "unproved"}, {2, "depends-on-stated-assumption"}, {3, "proved"}});
}

void test_summary_view() {
  // SPEC 1.10 "Summary view" (rev 1.3, ADR-054, D2): four observation results, four prediction results,
  // three confirmation results.  The stored value 0 of the observation is "none" (no summary computed):
  // a zero record claims nothing.  INCONCLUSIVE carries its reasons in a separate set.
  check_vocabulary("KZ_OBS", "SPEC 1.10 summary view, observation",
                   {{0, "none"}, {1, "OBSERVED_VIOLATION"}, {2, "NO_VIOLATION_IN_CHECKED_PREFIX"}, {3, "INCONCLUSIVE"}, {4, "NOT_APPLICABLE"}});
  check_vocabulary("KZ_PRED", "SPEC 1.10 summary view, prediction",
                   {{0, "NOT_RUN"}, {1, "MODEL_CANDIDATE"}, {2, "NONE_IN_SCOPE"}, {3, "INCONCLUSIVE"}});
  check_vocabulary("KZ_CONF", "SPEC 1.10 summary view, confirmation", {{0, "NOT_RUN"}, {1, "CONFIRMED"}, {2, "NOT_CONFIRMED"}});
  // SPEC 1.1 "Execution relations" / 1.10 (the fifth row: ownership undetermined, the conservative value).
  check_vocabulary("KZ_EXEC_RELATION", "SPEC 1.1 execution relations",
                   {{0, "ownership-undetermined"},
                    {1, "inter-application"},
                    {2, "core-application"},
                    {3, "creation-framework-new-application"},
                    {4, "intra-application"}});
  // SPEC 4.8 "Reasons for withholding" (C-27), in the SPEC's order.
  check_vocabulary("KZ_WITHHOLD", "SPEC 4.8 reasons for withholding",
                   {{0x1, "start_state_unknown"},
                    {0x2, "loss"},
                    {0x4, "resource_or_lifetime_unmatched"},
                    {0x8, "ownership_undetermined"},
                    {0x10, "effect_point_ambiguous"},
                    {0x20, "caller_requirement_absent"},
                    {0x40, "effect_unsupported"},
                    {0x80, "horizon-open"},
                    {0x100, "not_evaluated"},
                    {0x200, "explanation_ambiguous"},
                    {0x400, "limit"}});
  // SPEC 1.2 "Creating application" (ADR-056): confirmed | inferred | undetermined; the zero value is undetermined.
  check_vocabulary("KZ_OWNER_LEVEL", "SPEC 1.2 app_owner level", {{0, "undetermined"}, {1, "inferred"}, {2, "confirmed"}});
  // SPEC 1.2 / 5.7: the marks of a mapping that rests on time containment alone (C-32).
  const kz::EnumDesc* ev = kz::find_enum("KZ_OWNER_EVIDENCE");
  KZ_CHECK(ev != nullptr);
  bool by_time = false, unverified = false;
  if (ev != nullptr)
    for (size_t i = 0; i < ev->nitems; ++i) {
      by_time = by_time || std::strcmp(ev->items[i].display, "match-by-time") == 0;
      unverified = unverified || std::strcmp(ev->items[i].display, "id-unverified") == 0;
    }
  KZ_CHECK(by_time);
  KZ_CHECK(unverified);
  // The summary keeps the occurrence status: the six statuses stay as they were (SPEC 1.6), and OPEN
  // and an undetermined status are flags of the summary, not statuses.
  check_vocabulary("KZ_OCC", "SPEC 1.6 occurrence status",
                   {{0, "NOT_EVALUATED"}, {1, "READY"}, {2, "TOLERATED"}, {3, "VIOLATING"}, {4, "PENDING"}, {5, "NOT_READY_UNJUDGED"}});
  check_vocabulary("KZ_STATUS_FLAG", "SPEC 1.10 summary view (OPEN, undetermined)", {{0x1, "open"}, {0x2, "undetermined"}});
}

// The words the SPEC retired must not reappear in any enumeration, whatever its list says.
void test_retired_words() {
  const char* retired[] = {"event_lost", "event-lost", "events_lost", "events-lost", "packet_lost", "packets_lost", "packet-lost", "packets-lost", "lost_silently"};
  for (const kz::EnumDesc& en : kz::enums())
    for (size_t i = 0; i < en.nitems; ++i)
      for (const char* word : retired) {
        const bool in_display = std::strstr(en.items[i].display, word) != nullptr;
        const bool in_id = std::strstr(en.items[i].id, word) != nullptr;
        KZ_CHECK(!in_display);
        KZ_CHECK(!in_id);
        if (in_display || in_id) std::fprintf(stderr, "%s has the retired word \"%s\" (SPEC 0.9 item 4: an event request is rejected, never lost)\n", en.name, word);
      }
  // R2's own harm is the renamed one, by identifier too.
  const kz::EnumDesc* harm = kz::find_enum("KZ_HARM");
  KZ_CHECK(harm != nullptr && harm->nitems > 1);
  if (harm != nullptr && harm->nitems > 1) KZ_CHECK_EQ(std::string(harm->items[1].id), std::string("EVENT_REQUEST_REJECTED"));
}

}  // namespace

int main() {
  test_harm();
  test_consequence_and_labels();
  test_classes_and_axes();
  test_provenance_and_proof();
  test_summary_view();
  test_retired_words();
  return kztest::finish("test_core_vocab");
}

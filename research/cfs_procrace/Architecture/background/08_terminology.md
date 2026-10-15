# 08 Terminology: is "contract" the right word for Kairos's readiness object?

- **Date:** 2026-10-12. **Revision 2** (same day) addresses the 32 issues raised by the citation pass and the adversarial pass on revision 1. Each issue and its handling is listed in **Review notes** at the end. **Revision 3** (2026-10-15) applies the terminology items of the external review EXT-20261012 §6.1–§6.4 (`history/review_EXT_20261012.md`) as `spec/SPEC_FINAL.md` revision 1.1 answers them (spec §0.7 item 4, §1.6 "Three judgements", §10.1 QT-2–QT-5, ADR-043): the reason for not calling κ an invariant (§0, §3, §5.7(b), §6.5), the scope of T4's census (§2.4, §3), and the separation of supplier-side results from caller-side acceptance (§1.1, §2.2–§2.4, §4.4–§4.5, §5). Review notes 33–37 list the edits. The line positions that the spec cites (L559–561, L754, L758) are unchanged.
- **Status:** recommendation for a PI decision (T1, pending; until it is decided every document keeps "contract" and 계약). Revisions 1 and 2 edited neither the spec nor the Architecture documents. Revision 3 edits only this note, errata sections in T1–T4 and `Architecture/09_prior_work.md`. Nothing was committed.
- **Question (PI):** "계약이라는 어휘가 타당한지 잘 모르겠네." Is *contract* (계약) a valid and appropriate name for κ, which `spec/SPEC_FINAL.md` §1.6 calls a "readiness contract"?
- **Inputs.** This note synthesizes four reports, cited here by their repository copies in `background/terminology/` (each now ends with errata of 2026-10-15):
  - T1 `T1_ai_paper.md`: the AI paper (FSE 2014), read in full;
  - T2 `T2_contract_lineage.md`: where "contract" comes from;
  - T3 `T3_alternatives.md`: alternative terms;
  - T4 `T4_term_census.md`: how 52 rows (37 distinct research papers, AI in two rows, and 14 platform, language or guideline documentation rows) name the required order and its violation, recorded at abstract level (T4 §1; scope in §2.4 below).

  I also re-read spec §0.1, §1.6 (`SPEC_FINAL.md:257–298` at commit b4ac55f; spec revision 1.1 moved these lines, so revision 3 cites spec sections), §1.10, §1.11, §3.15, §7.1–7.2 and the glossary, and counted the term's uses in `Architecture/*.md`, `diagrams/*.mmd` and `background/*.md`. For revision 2 I re-read in the pinned checkout `cfe_evs.h:150–199`, `cfe_evs.c:169–209`, `cfe_error.h:252–267`, `cfe/docs/cFE_FunctionalRequirements.csv`, all 138 `\par Assumptions` blocks of the cFE core API headers, and I re-ran the collision greps (§2.1, §3).

**How the citations were checked.** Three passes touched the bibliography. **None of them could run a web search after the synthesis's first six**, so most V tags rest on T1–T4.
- **Synthesis (revision 1).** Six searches before the run's search budget ran out. Three of them re-confirmed four records: AI (FSE 2014) and its TSE 2016 extension (one search), Lifestate (ECOOP 2019), and Strom & Yemini (TSE 1986). The re-search for Joshi & Sen (ASE 2008) returned results, but not this record. The searches for Meyer 1992 and Polikarpova et al. 2009 were refused by the budget limit.
- **Citation pass and adversarial pass (reviews of revision 1). No search budget.** The per-turn budget (200 searches, shared by all agents) was already used up at the first query. Crossref (`api.crossref.org`) was refused by the egress proxy, `dblp.org` did not resolve, and GitHub API access to nasa/cFE and nasa/CF is not enabled in this session. **No bibliographic V tag was re-verified on the web in either pass.** What those passes confirmed from primary documents on disk:
  - (a) the AI FSE 2014 record, from the PDF front matter: title, six authors, "FSE '14, November 16–21/22, 2014, Hong Kong", ACM 978-1-4503-3056-5/14/11, doi:10.1145/2635868.2635885, and pp. 330–340 from the printed page numbers;
  - (b) every AI quotation and locator used here (§4, §6, §7);
  - (c) AI's own reference list (p. 340), which corroborates authors, title, venue and year for Daikon SCP 69(1–3) 2007 [5], CCI OOPSLA'10 [7], Lu et al. ASPLOS'08 [12], AVIO ASPLOS'06 [13], Atom-Aid ISCA'08 [17], DefUse OOPSLA'10 [28], PSet ISCA'09 [35] and ConAir ASPLOS'13 [37]. This is secondary support: it gives no pages or DOIs;
  - (d) every platform-documentation quote, in T4's downloaded copies of the raw files (fetched from GitHub on 2026-10-12; T4 §1 lists repository, path and branch of each; the copies are kept in an unpublished working location, not in this repository);
  - (e) every cFS `path:line` quote, in the pinned checkout. The nasa/cFE #2663 and nasa/CF #184 issue bodies could not be re-checked; the case ledger records their URLs (`cases/R2.md:3`, `cases/C1.yaml:8`).
- **This revision (revision 2). Again no search budget**: WebSearch was refused at the first query. Direct fetches of primary pages (arxiv.org, link.springer.com, dl.acm.org, www.eiffel.org, www.cs.virginia.edu, api.openalex.org, api.semanticscholar.org, james0zan.github.io) were refused by the egress proxy (HTTP 403 on CONNECT). The GitHub connector is not configured for nasa/cFE, and the Scholar Gateway connector answered "access denied". **No bibliographic record was re-verified online.** The bibliographic corrections made here (Perracotta, Sousa et al., ICST 2017, Nienaltowski et al.) take their new initials or author order from the reviewers' recollection, so they are tagged **P**.
- **Labels in §8.** Every V that rests on a sibling report reads **"V per Tn (not re-verified in the citation or adversarial pass)"**. AI FSE 2014 stays **V** (primary PDF). The eight entries in (c) also carry "corroborated by AI ref [n]".
- **Before any bibliography is frozen**, run a follow-up with a fresh search budget for at least: Meyer 1992; Nienaltowski et al. 2009 (author order); Joshi & Sen 2008; Lifestate 2019; Lu et al. 2008 (definition wording); Polikarpova et al. 2009; the AI TSE 2016 extension; ICST 2017 (authors); Perracotta (initials, pages); Sousa et al. 2015 (initials); Benveniste et al. 2018; Hüttel et al. 2016; CCI (mechanism); the Monperrus et al. abstract; the eiffel.org DbC and SCOOP pages; a JML reference-manual page; Atom-Aid; Yu & Narayanasamy MICRO 2010; the two issue bodies; and the GPredict ICSE 2015 PDF (its Introduction's "a more generic contract", known only from the external review and a search snippet, and its §II.B property formalism; P-PAPERS).
- **AI quotations** were checked against a `pdftotext` extraction of the uploaded PDF (an unpublished working copy, not in this repository; T1 "How the paper was read"); page numbers are the printed ones (330–340). **cFS quotations** were checked in the pinned checkout `/home/user/work/procrace/cfs_ref/cFS`.

**Tags.**
- Bibliographic status:
  - **V**: authors, title, venue and year confirmed by a search result or a document page.
  - **V per Tn (not re-verified …)**: V as reported by sibling report Tn; nobody has re-checked it since.
  - **P**: partly confirmed, with the gap named.
  - **UNVERIFIED**: not confirmed.
  - **V-doc**: a primary documentation page that a sibling report fetched and read, and whose text is on disk.
  - **P-doc**: a documentation page seen only as a search snippet; the page itself was not opened.
- What was seen of the content:
  - **text**: the primary text was read;
  - **snippet**: a short excerpt shown in a search result;
  - **summary**: a search-engine or secondary summary.
- Other tags:
  - **[F:…]**: a fact with its source;
  - **[I]**: inference;
  - **[S]**: suggestion.

---

## 0. Answers in brief

| Question | Answer |
|---|---|
| Is "contract" valid for κ? | **Partly, and only in two different senses.** (1) The **Design-by-Contract (DbC) sense**, an obligation on the caller stated by the supplier, fits entries where a reference states the condition as the caller's obligation. R2 is the exemplar: "The Application ID must correspond to a registered application or core service" (`cfe_evs.h:169`). Even R2 is not textbook DbC: the same header documents the not-ready results as return codes (`\retval #CFE_EVS_APP_NOT_REGISTERED`, `#CFE_EVS_APP_ILLEGAL_APP_ID`, `cfe_evs.h:198–199`), so R2 is a documented caller assumption whose documented error return every caller discards (§2.3). Spec revision 1.1 keeps the two apart: the caller assumption is the caller-level judgement (`appreq`), the return code the framework-level one (spec §1.6 "Three judgements", App. A.1 step 8). (2) The **assume-guarantee (A/G) sense**, an assumption that the environment must meet, fits admitted assumptions such as C1. There CF assumes that a peer creates the semaphore and bounds its own retry. But A/G brings refinement and composition, which Kairos does not have. **Neither sense fits** two kinds of entry. The first is entries with no deciding reference (`unspecified_by_reference`), such as the SB subscribe-before-first-send loss: cFE requirement cSB4305.5 specifies only the supplier's no-subscriber handling and says the condition "is not considered an error". The second is `code_pattern` entries generated from the code under analysis. "Contract" has **no collision** in cFS flight code (§2.1). |
| What must the term convey? | Three things. (1) **κ is a specification object, not a single predicate.** It bundles a required state (`Req` with its `Oracle`), the not-ready outcomes that its references accept (Allowed, each with a reference horizon), what the use must achieve (`appreq`), the harm, the provenance of each part and the scope (spec §1.6). (2) **Written from a recorded source or by a fixed rule, never trained on runs.** (3) **No claim of a promise that nobody made.** "Invariant" names a predicate that should hold at given points; an invariant can be written, proved or inferred, and learning is not part of the definition (AI p. 331, text seen; Daikon's definition, T3 §2.1, snippet, P). So the word fits `Req` at most, never κ. Even `Req` need not hold at every use: an accepted not-ready occurrence is legitimate (`TOLERATED`), and where no reference decides, κ still states the required state and records that no reference says whether a not-ready outcome is acceptable (`unspecified_by_reference`). A secondary, reader-side risk [I]: the named interleaving invariants (AVIO, DefUse, PSet, AI) are learned from runs judged correct, so the word next to them may be read as learned. No noun does all of this alone (T3), so a qualifier and a source tier must carry the distinction. |
| Recommendation | Primary term: **readiness specification (준비 상태 명세)**. It names the *written artefact*: κ carries provenance, an author, an authoring time and a version guard. It asserts no agreement, and it fits documented, admitted, rule-generated and unreferenced entries alike. On first use, write **provenance-tracked readiness specification (근거 기록 준비 상태 명세)**. Reserve "reference-derived" for the tiers that have an external reference (§5.4). In Korean, always write 준비 상태 명세 in full and never bare 명세, which collides with "spec §x" (§5.1). Use **readiness property** for what an instance denotes, in the semantics section. **Keeping "readiness contract" is a genuine alternative** (§5.2). It is shorter, has no cFS collision, reads DbC-like for R2 and A/G-like for C1, but fails for unreferenced and rule-generated entries and brings blame and composition baggage. This is a trade-off for the PI. The name by itself guarantees neither novelty nor accuracy (EXT-20261012 §6.1); the definition, the source tier, the side of each reference (`supplier`, `caller`, `rule`; spec §1.6) and per-tier reporting carry the distinction. Either way: keep **"API contract" (API 계약)** for M_api and `contract-backed` as a tier name, and never call κ an invariant. |
| Why the specified/learned split matters | **R2 (cFE #2663)** is ordered the wrong way in every boot. A learner that accepts every behaviour seen in runs judged correct (AVIO, DefUse, PSet, AI; Daikon for values) contains that order by construction. A frequency-based deviance ranking could surface R2's sites, but only as a ranking, not as a verdict or a fixed-order certificate. Statistical isolation in the style of CCI has no failing boot to contrast. A provenance-tracked specification flags R2 in every boot. **C1 (CF #184).** A per-access learned invariant judges each access, not the outcome and its continuation within a bound. It either accepts every retry, which also hides exhaustion, or flags the first not-ready lookup. `retry_until_ready(codes, bound)` separates the two. **SB losses**, silent to the sender, pass any output-based training oracle. Kairos reports them as unjudged at the caller level (LOSS-UNDER-ORDER), not as violations, with the framework-level judgement (cSB4305.5: not an error) beside them (spec §3.9). |
| What the AI paper changes | Related work gains a row for learned interleaving invariants (AVIO, DefUse, PSet, AI), with CCI listed separately as statistical isolation. An AI-style comparator can be added to §8.3. Two design choices get a citable motivation: the gate/LP separation (vs AI's turning point), and the reference-derived watchdogs with `timeout-exercised` (vs AI's stall cap). The paper also brings two vocabulary constraints. Never use "invariant" for κ. And `TOLERATED` would sit next to "tolerating concurrency bugs" (AI; Yu & Narayanasamy 2010), so the paper should rename the enum to ALLOWED_NOT_READY / `allowed` (§6.5). |

---

## 1. The question, and why it matters for a paper

### 1.1 The object being named

SPEC_FINAL §1.6:

> κ = ⟨id, U, sel, Req, Oracle, Allowed, appreq, harm, prov, authored, guard, scope⟩ (spec revision 1.1; `appreq` was added by EXT-20261012 §6.4)

Its parts:
- **Req**: "a conjunction `P_1 ∧ … ∧ P_n` of required lifecycle or value predicates" (spec §1.6) on a key-selected framework resource. Each conjunct is evaluated at its own linearization point (LP); for a value conjunct, the LP is the **capture point** of the argument (§1.4).
- **Oracle**: the framework's own validity predicate for each conjunct, an independent observation of the required state (§7.2 "Two readiness oracles").
- **Allowed**: a finite vocabulary of acceptable not-ready outcomes, each with a reference horizon, or `unspecified_by_reference`. Since revision 1.1 only the **caller-level Allowed** decides statuses: entries with a `caller` record, or, under the default of PI item QT-4, with only a `rule` record (reported as rule-backed). An entry whose records are all `supplier` is framework-level information (spec §1.6 "Caller-level Allowed").
- **appreq** (revision 1.1): the reference that states what the use must achieve for the calling application or the mission, or `unspecified_by_reference`; never a supplier-side text or a rule (spec §1.6).
- **harm**: the effect class when the outcome is not allowed (e.g. `event_lost_silently`, `app_error_exit`).
- **prov**: provenance, one of `api_doc`, `requirement` (added in revision 1.1, pending PI item QT-2), `code_admission`, `code_pattern`, `issue` or `researcher`, with `side ∈ {supplier, caller, rule}`, `path:line` and a hash (spec §1.6).
- **scope**: "no SHUTDOWN_REQ LP of `es.shutdown` precedes the occurrence's decision event" (spec §1.6; revision 2 quoted the older "no shutdown request before the use").

Two governance rules apply:
- κ entries are **authored, or generated by a fixed rule, before evaluation, and never trained on runs** (§7.1–7.2). Run-derived sources (§7.1 sources 2, 10, 11) yield instances or candidates only, and later changes are labelled `oracle refinement`.
- The cross-thread **order** that makes Req true is **not part of κ**. Kairos derives it with happens-before (HB) and MustBefore analysis, and records who provides the state on the **provider** axis (`present`, `none`, `env-absent`, `unknown`; §1.10).

**A drift in the definition.** The term was first defined in R03 §4 as an **order** requirement: "a consumer step requires that a producer step … has already happened" [F: `background/03_cfs_app_patterns.md:383`]. The critique (R06) then re-framed it as "a typestate precondition on a shared resource, checked at a use event in another task" [F: `background/06_critique_v01.md:187`]. The spec adopted that **state** reading and widened it to value conjuncts. Any new name should follow §1.6 (state, allowed outcomes, harm and provenance), not R03's order reading.

### 1.2 Why the noun matters for a paper

1. **The noun selects the reviewer's comparison class and assumptions** (T2 §8, T4 §6.1) [I]:
   - DbC, JML, ACSL and C++26 readers expect per-function, executable pre- and postconditions with blame.
   - Assume-guarantee readers (AGREE, OCRA; the NASA formal-methods community) expect refinement, composition and compatibility checking. Kairos has none of these.
   - Concurrency-theory readers know "behavioural contracts" with compliance relations.
   - The Shan Lu line says "intention" for the ground truth and "invariant" for the learned check: AVIO, DefUse, PSet and AI, and CCI, which AI counts among the "interleaving invariants" (AI p. 331, text seen).
2. **Specified vs learned is one related-work argument.** R2 is the case that separates Kairos from DefUse, AI and every learner that accepts what it saw in runs judged correct (§4). The term should not undercut that argument. It is not the contribution: spec revision 1.1 places the candidate contribution in the cFS-specific extraction, the classification and the evidence, not in the use of a specification (spec §1.11).
3. **Overclaiming can be refuted, for R2 as well as C1.** If C1 is called a "contract violation", a reviewer can ask: "OSAL returned a documented result; which contract was violated?" (T2 §8 risk 1). The same question applies to R2: EVS documents `CFE_EVS_APP_ILLEGAL_APP_ID` as a `\retval` (`cfe_evs.h:199`) in the same way that OSAL documents `OS_ERR_NAME_NOT_FOUND` (`osapi-countsem.h:160`). R2 differs only by the one sentence stating a caller assumption (§2.3). Spec revision 1.1 answers the question by keeping the framework-level and the caller-level judgement apart (spec §1.6 "Three judgements").
4. **"The API already checks its contract."** EVS checks registration and returns a code. Framed as "contract checking", Kairos invites the reply that the framework already does it. The contribution is **order classification** (fixed-order vs schedule-dependent, with certificates) plus **outcome and harm judgement** (T2 §8 risk 2).
5. **Circularity hides behind the word.** For entries whose `Allowed` is read from the very code being judged (retry bounds; K-SELF from `code_pattern`; §7.1 sources 4–6), conformance holds by construction. "Contract" makes them sound like external promises (T2 §8 risk 3). Spec revision 1.1 records such sources with side `rule` and reports their acceptances as rule-backed (spec §1.6).
6. **The spec already uses "contract" in two senses.**
   - M_api is "a hypothetical OSAL backend that conforms atomically to the OSAL API contract" (§1.4). That is the supplier-semantics sense and correct DbC usage.
   - κ is a usage requirement.
   - K-LOCKBAL ("the API returns with the type lock released") is a **supplier postcondition** filed in the readiness catalogue (T2 §5). In revision 1.1 its `Allowed none` rests on supplier-side texts only, so its occurrences are NOT_READY_UNJUDGED until an `appreq` is recorded or the PI moves it to an implementation-defect family (PI item QT-9; spec §1.6 `harm`).
7. **Discoverability.** In T2's survey, a query mixing "contract" with locking and state terms returned mostly smart-contract and blockchain results (T2 §8 risk 8) [I for the impact on reviewer bidding].

---

## 2. What "contract" means in the literature, and in which sense κ is one

### 2.1 Senses of "contract"

| Tradition (status; what was seen) | What a contract is | Who is obligated | Can it express cross-thread order? | Specified or inferred |
|---|---|---|---|---|
| **Design by Contract**: Meyer, "Applying 'Design by Contract'", IEEE Computer 25(10):40–51, 1992 (**V per T1–T3**, bibliographic; article text **not seen**). DbC wording: **P-doc** (snippet; page not opened; eiffel.org or Wikipedia; T2 l.116) | Routine preconditions, postconditions and class invariants. Snippet (T2): "The client's obligation, which protects the supplier, is called a precondition." Blame (snippet, attributed by T2 to an Eiffel tutorial, possibly Wikipedia): "a precondition violation signals a bug in the client … a postcondition (or invariant) violation signals a bug in the supplier" | client: pre; supplier: post and invariant | no (state predicates at call and return) | specified |
| **JML**: Leavens, Baker, Ruby, ACM SIGSOFT SEN 31(3):1–38, 2006 (**V per T2**; summary) | Behavioural interface specification; `normal_behavior` and `exceptional_behavior` cases (**P-doc**: manual or tutorial snippets; page not opened; T2) | as DbC | not in core JML | specified |
| **SCOOP**: Nienaltowski et al., "Contracts for Concurrency", Formal Aspects of Computing 21(4):305–318, 2009 (**V per T2** for title, venue and DOI; **author order P**; abstract snippet) | DbC for concurrency. A precondition on a separate object becomes a **wait condition** (**P-doc**: eiffel.org "Design by Contract in SCOOP" or Wikipedia snippet; page not opened; T2 l.160; attribution to the FAC paper itself **P**) | client, then the runtime waits | **yes**: the routine proceeds only after a condition on another processor's object holds [I: T2's paraphrase, T2 l.167] | specified |
| **Rely-guarantee**: Jones, ACM TOPLAS 5(4):596–619, 1983 (**V per T2**; definitions **P**, secondary) | pre, rely, guarantee, post | each component, mutually | partly (two-state interference; stability) | specified |
| **Assume-guarantee contracts**: Benveniste et al., "Contracts for System Design", FnT EDA 12(2–3):124–400, 2018 (**V per T2**; definitions **P**). AGREE: Cofer et al., NFM 2012 (**V per T2**) | (assumption, guarantee) over traces; refinement, composition | component for G; environment for A | **yes**, temporal over traces | specified (architecture) |
| **Behavioural contracts**: Hüttel et al., ACM CSUR 49(1), art. 3, 2016 (**V per T3**; abstract) | Behavioural descriptions with compliance | both parties | yes (two-party interaction order) | specified |
| **Atomicity contracts**: D. G. Sousa, R. J. Dias, C. Ferreira, J. M. Lourenço, "Preventing Atomicity Violations with Contracts", arXiv:1505.02951, 2015 (**V per T1** for surnames, title and id; **initials P**; search summary of the abstract). "Verifying Concurrent Programs Using Contracts", ICST 2017, pp. 196–206 (**V per T1, T2** for title, venue, pages and DOI; **authors and order P**; summary) | Method sequences that must run atomically. T2 saw the phrase "contracts for concurrency" only in a search summary of the ICST 2017 abstract (P); it is also the exact title of the SCOOP paper above | library client | atomicity, not readiness | specified |
| **API directives**: Monperrus, Eichberg, Tekes, Mezini, EMSE 17(6):703–737, 2012 (**V per T2, T4**, bibliographic; definition wording **P**: snippet, and T2 and T4 differ) | Directives are natural-language statements that make developers aware of "constraints and guidelines related to the correct and optimal usage of an API" (T2's snippet; T4's snippet reads "related to the usage of an API"; open doi:10.1007/s10664-011-9186-4 before quoting) | API client | sometimes (sequence and state; synchronization in prose) | specified (docs) |
| **Inferred contracts**: Polikarpova, Ciupa, Meyer, ISSTA 2009 (**V per T2**). Wei, Furia, Kazmin, Meyer, ICSE 2011 (**V per T2**) | Contracts inferred by tools, always **qualified** as "inferred" | — | — | **inferred** |

All status tags in this table are carried over and were not re-verified in the citation or adversarial pass (header). **Before quoting** DbC, SCOOP or JML wording in the paper, open eiffel.org "ET: Design by Contract", eiffel.org "Design by Contract in SCOOP" and a JML reference-manual page, and cite their exact URLs.

**What cFS developers will read** [F: pinned checkout, recounted in revision 2]:
- **"Contract" is absent from cFS flight code.** In the C sources of `cfe`, `osal`, `psp`, `apps` and `libs`, the word occurs only:
  - in licence text, on 2 lines in `psp/fsw/generic-vxworks-rtp/parg/`;
  - in two unit-test comments. One is `cfe/modules/core_api/ut-stubs/src/cfe_config_handlers.c:63`, a unit-test stub: "The public API has a contract _not_ to return a NULL pointer", used in the supplier sense. The other is `cfe/modules/tbl/ut-coverage/tbl_ut_helpers.c:358–359`.
- **The documentation heading is mixed.** cFE's API headers record assumptions under `\par Assumptions, External Events, and Notes:`, in 138 blocks in `cfe/modules/core_api/fsw/inc`. This is not a heading for client obligations:
  - 57 of the blocks read "None" or "None.";
  - others describe supplier behaviour. For example, five `cfe_es.h` blocks say "In the case of a failure (#CFE_ES_ERR_RESOURCEID_NOT_VALID), an empty string is returned".

  Whether a block states an obligation on the caller is therefore the author's judgement, and it must be recorded (§5.4).
- **cFE also has functional requirements.** They are in `cfe/docs/cFE_FunctionalRequirements.csv` (570 lines), and each bundled app has its own `*_FunctionalRequirements.csv`. Several specify the supplier's handling of not-ready outcomes:
  - cEVS3103.1: an unregistered sender gets a counter increment and an event (`:269`);
  - cSB4305.5: a send with no subscriber gets a counter increment and an event, and the rationale says it "is not considered an error" (`:352`);
  - cSB4305.3 and cSB4305.4: on pipe overflow or when the MsgId-to-pipe limit is reached, the send to that pipe is aborted, an event is issued, and sending continues (`:350–351`).

  A NASA reader will count these among the "references" (§2.3, §7 Q5). Spec revision 1.1 adds the provenance kind `requirement` and records each record's side: for the uses of an application that calls EVS or SB, cEVS3103.1 and cSB4305.3–.5 are `supplier` records, i.e. framework-level information (spec §1.6).

### 2.2 Which parts of κ are contractual

Condensed from T2 §5 [I], revised in revision 2:

| κ part | Closest established notion | Contractual? |
|---|---|---|
| `Req` from `api_doc`, where the text states a caller obligation (author's judgement, recorded) | DbC precondition, lifted to typestate (Fugue-style typestate precondition; DeLine & Fähndrich, ECOOP 2004 [V per T2, T3]) | **DbC-like**, but cFE also documents the not-ready return codes (§2.3 R2); in revision 1.1 such a text is a `caller` record and the use's `appreq` (spec §1.6) |
| `Req` from `code_admission` | the client's own assumption about its environment: an A/G assumption, or an inferred, necessary precondition (Cousot et al., VMCAI 2013 [V per T2, T3]) | only in the A/G sense, or as an "inferred contract" |
| `Req` or `Allowed` from `code_pattern` | a rule applied to the program under analysis (§7.1 sources 4–6) | **no** (circular; §4.1); side `rule` in revision 1.1, so its acceptances are reported as rule-backed |
| `Oracle` | the supplier's defensive check, i.e. the executable form of the precondition | yes, as the supplier-side check of it |
| supplier-side statements: documented results (`\retval`, `@retval`), cFS requirements on supplier handling (cEVS3103.1, cSB4305.3–.5), a supplier's documented drop of a caller's request, a documented propagation | exceptional behaviour cases of the API contract (JML `exceptional_behavior`) | **yes, as the API contract**. Since revision 1.1 they are the framework-level judgement (`fw_doc`, `fw_conf`), not Allowed entries of the caller's κ, and they never make an occurrence TOLERATED (spec §1.6 "Three judgements", §1.10) |
| caller-side Allowed entries: `retry_until_ready(codes, bound)` with the bound read from client code; `defer_by_design` from an app comment; the using module's own documented drop, `fallback` and `exit` with a caller-side text | client tolerance policy; a hand-written bounded SCOOP-style wait | not in the DbC sense; it is the client's handling of an A/G assumption. These form the caller-level Allowed of revision 1.1 (spec §1.6) |
| `unspecified_by_reference` | absence of any deciding reference | **no**: there is nothing to violate |
| `scope` | A/G assumption, or rely condition | yes, in the A/G sense |
| `harm` | consequence class, reported in every verdict record (§1.10) | not a clause; part of κ's judgement |
| `prov`, `authored`, `guard` | traceability metadata | no (metadata, not clauses) |
| HB order between the provider and the use | *not in κ*; derived by HB and MustBefore; provider axis | — |

### 2.3 The three cases under a contract reading

All cFS quotes below were re-read in the pinned checkout.

| Case | What the references say | Is "contract violation" accurate? |
|---|---|---|
| **R2**, cFE #2663 (fixed-order; 24 uses per boot; observed 3/3 with gdb and with an instrumented build) | **The caller assumption.** Under `\par Assumptions, External Events, and Notes:`, `cfe_evs.h:169` says "The Application ID must correspond to a registered application or core service." The next lines name `CFE_ES_WriteToSysLog` for messages "outside the context of a registered application". **The error returns.** The same header documents `\retval #CFE_EVS_APP_NOT_REGISTERED` and `\retval #CFE_EVS_APP_ILLEGAL_APP_ID` (`cfe_evs.h:198–199`). **The requirement.** cEVS3103.1 (counter and event) governs the NOT_REGISTERED branch (`cfe_evs.c:190–194`). R2's zero AppId takes the ILLEGAL_APP_ID branch instead (`cfe_evs.c:185–189`), which only the `\retval` documents. **The use.** SB code passes `CFE_SB_Global.AppId` before SB's task publishes it. Every SB caller discards the code (harm `event_lost_silently`, consequence `status_ignored`) | **Partly.** R2 is a **documented caller assumption, violated in every boot, whose documented error return every caller discards**. The DbC reading rests on one sentence under a mixed heading, while the supplier also defines the not-ready result as a return code. So a reviewer can ask the C1 question here too: "EVS returned a documented code; which contract was violated?" It is not textbook DbC, where a precondition violation is not given a defined outcome [I. The non-redundancy principle was seen by T2 only in lecture slides (T2 §3.2, P). Meyer 1992's text was not seen, so this reading is UNVERIFIED]. Spec revision 1.1 answers the reviewer's question with three judgements: caller level, `appreq` = `cfe_evs.h:169–172` and `Allowed = none`; framework level, `fw_doc` = {`documented-result`}; observed, "status ignored and effect lost silently" (spec App. A.1 step 8) |
| **C1**, CF #184 (bounded retry) | `osapi-countsem.h:160`: "@retval #OS_ERR_NAME_NOT_FOUND if the name was not found in the table". This is a documented result (`@retval`); the header calls it neither normal nor an error, and no caller obligation says the name exists. CF's own comment (`cf_cfdp.c:1272–1278`): "There is a start up race condition because CFE starts all apps at the same time … if OSAL returns OS_ERR_NAME_NOT_FOUND, assume this is what is going on, delay a bit and try again." The retry is bounded by `DEFAULT_CF_STARTUP_SEM_MAX_RETRIES 25` × `DEFAULT_CF_STARTUP_SEM_TASK_DELAY 100` (`cf_internal_cfg.h:173, 185`) | **Not in the DbC sense**: no caller obligation is stated, and the not-found result is documented as an `@retval`. No OSAL contract is violated, even when the retries run out. **In the A/G sense, yes.** CF's admitted assumption is the A of CF's own A/G contract, an environment assumption that CF does not guarantee (T2 §6). Kairos's `timeout-dependent` verdict asks whether some schedule breaks it. That fits "contract" only in the A/G sense, which brings refinement and composition expectations that Kairos does not meet |
| **SB subscribe-before-first-send** (silent loss) | `cfe_sb.h:401`: the message is "transmitted to all subscribers". With none, that holds vacuously and the call returns `CFE_SUCCESS`. Requirement **cSB4305.5** (`cFE_FunctionalRequirements.csv:352`): "if no Application's have Subscribed to the Message, the cFE shall increment the No Subscriber's Counter and send an event that displays the Message ID". Its rationale: "It is not considered an error if no Applications have subscribed because of the dynamic environment supported by the cFE. In addition, during initialization of a multi-processor system, this can easily occur." The 17 `SEND_NO_SUBS` uses chained to R2 are events of this kind, sent with SB's unpublished AppId [F: spec §1.6; I for the link to the requirement] | **No.** The API and cSB4305.5 specify only the supplier's no-subscriber handling (a counter and an event). No reference requires delivery, or a subscribe-before-send order. The spec labels these LOSS-UNDER-ORDER with `Allowed = unspecified_by_reference`, and "contract violation" would assert a broken promise where cFE's own requirement says the condition is not an error at the framework level. cSB4305.5 is supplier-side: in spec revision 1.1 it gives the framework-level judgement `documented-not-error` and does not make the caller level `documented_drop`; the caller level stays `unspecified_by_reference` (LOSS-UNDER-ORDER) unless a consumer or mission reference decides (spec §1.6, §3.9; PI item QT-2, which re-asks STATUS decision T2). In R2's window the event it requires is rejected by EVS [M], and the window's counter increments are cleared later by SB's own initialization [I] (spec App. A.1 step 13a) |

### 2.4 Verdict: the precise sense in which κ is and is not a contract

- **κ is DbC-like** when a reference states `Req` as an obligation on the caller. These are the `contract-backed` entries, and R2 is the exemplar. cFE also documents the not-ready results as return codes, so even here Kairos judges how the caller handles a documented error return. The caller is not breaching a precondition with undefined behaviour. In spec revision 1.1 the return code is the framework-level judgement and the caller's obligation the caller-level one (spec §1.6 "Three judgements").
- **κ is A/G-like** when the requirement is an application's admitted assumption about its environment, handled by the application itself (C1). In A/G terms the environment must meet the assumption, and Kairos asks whether some schedule breaks it. This fits "contract" only in the A/G sense, which brings refinement and composition that Kairos does not perform.
- **κ is not a contract in any sense** in two cases:
  - its parts are generated by a rule from the code under analysis (`code_pattern`; §7.1 sources 4–6). Conformance then holds by construction;
  - no reference decides (`unspecified_by_reference`: K-ROUTE outside TO_LAB, K-CAP).

  A "reference-derived specification" would also overclaim here, which is why §5.1 uses the qualifier "provenance-tracked".
- **The cross-thread order is never in κ.** SCOOP waits, rely-guarantee stability and A/G trace assumptions can all express it, but Kairos borrows none of their semantics. What it borrows can be stated precisely: **a state requirement on a resource whose state other actors typically establish or keep true; which actor does so, and in what order, is derived, not specified** (adapted from T2 §7).
- **How many instances are contract-backed is not known yet** [I]. §3.15 shows several kinds whose `Allowed` comes from client code (RC1, the CF retry), from an app comment (RC5, TO_LAB) or is `unspecified_by_reference` (RC5, RC14). The share has to be measured by the census and reported. That is a reason not to name the whole set after its best-documented part.
- **Usage in neighbouring fields.**
  - **T4's census, and what it counted** (restated in revision 3, EXT-20261012 §6.3). T4 coded 52 rows: 37 distinct research papers (AI is rows 10 and 12) and 14 platform, language or guideline documentation rows (rows 29 and 36–48). For each row it recorded one *requirement term*, the name the work gives the required order or readiness requirement (T4 §2); 7 rows record none (rows 3, 4, 5, 6, 21, 35, 43). Apart from AI no paper was read in full: a paper's term comes from a search summary of its abstract or from AI's description of the work (T4 §1), and paper bodies were not searched for "contract". Under that coding "contract" is never the recorded name, "specification" about once (Lifestate's "lifestate specification") and "properties" about once (GPredict); the rest say typestate, protocol, repair policy, execution filter or order relationships (T4 §4.1). GPredict's prose, however, calls the Java Iterator usage rule "a more generic contract" (its Introduction; known from the external review's reading of the printed first page, EXT-20261012 §6.3, and from a web-search snippet; the PDF was not opened here [U]; P-PAPERS). So the census cannot support "related work does not use 'contract'" or "'contract' is non-standard in race work", and term frequency alone is no reason to exclude "contract". The census does not favour any noun, and the recommendation does not rest on it (§5.2).
  - In the contract literature, "contracts for concurrency" means either SCOOP wait conditions (Nienaltowski et al. 2009, whose title is exactly "Contracts for Concurrency") or atomicity contracts on call sequences (Sousa et al. 2015; ICST 2017). Neither means readiness (T2 §3.6, §3.11).

---

## 3. Alternatives, with pros and cons

Synthesized from T2 §9.1, T3 §3 and T4 §6.
- **Collision counts.** The cFS counts are whole-word, case-insensitive greps over the pinned flight-software C sources (`cfe`, `osal`, `psp`, `apps`, `libs`), with unit-test directories excluded. Revision 2 recounted them: "contract" 0 outside licence text; "specification" 227 lines, 221 of them "Specification for …" headers; "property" 14; "protocol" 56 (T3 counted 59 with a different file filter).
- **What they are.** They indicate collisions; they are not a lexical study.

| Candidate (KO) | Established precedent (status) | Pros | Cons and collisions | Verdict |
|---|---|---|---|---|
| **readiness contract** (준비 계약), unchanged | DbC (Meyer 1992, V per T1–T3); A/G contracts (Benveniste et al. 2018, V per T2); "API contract" in the documentation sense | short; no churn; signals "external reference"; **0 flight-software collisions in cFS** (only licence text and 2 unit-test comments); DbC-like for R2 and A/G-like for C1 | needs two different senses to cover R2 and C1; DbC blame; A/G expectations of refinement and composition; asserts an agreement for unreferenced and `code_pattern` entries; keyword noise; internal double meaning with M_api | **alternative 1**: a real trade-off (§5.2) |
| **readiness specification** (준비 상태 명세) | Lifestate "specification language" (ECOOP 2019, V, re-confirmed by the synthesis search); parametric properties and monitoring (MOP/JavaMOP: Chen & Roşu, OOPSLA 2007; TACAS 2009; V per T3); JML "behavioral interface specification" (V per T2); Dwyer et al. "property specifications" (ICSE 1999, V per T3 bibliographic, wording P) | names the **written artefact** (κ has provenance, author, time and guard); covers documented, admitted, rule-generated and unreferenced entries without claiming a promise; no blame or A/G baggage | neutral about provenance, with a mined mirror image ("Mining Specifications", Ammons, Bodík, Larus, POPL 2002, V per T1–T3), so it needs the provenance qualifier and the source tier. Collisions: the document name SPEC_FINAL; cFE's and the apps' "Specification for …" headers (221 of the 227 lines); **the CCSDS CFDP specification inside CF** (16 CF lines, e.g. `cf_cfdp_pdu.h:26`); **the `Spec` format-string parameter** of `CFE_EVS_SendEvent` and `CFE_EVS_SendEventWithAppID` (`cfe_evs.h:155, 208, 261`), R2's own API; Electronic Data Sheets; in Korean, 명세 vs "spec §x" (§5.1). Longer | **primary** (for κ, the artefact) |
| **readiness property** (준비 상태 속성) | temporal safety and liveness (Lamport, TSE 1977, V per T3, quote P; Alpern & Schneider, IPL 1985, V per T3); SLAM "temporal safety properties" (Ball & Rajamani, POPL 2002, V per T3, wording P); GPredict "concurrency properties" (ICSE 2015, V per T4); MOP's parametric properties | fits the semantics: σ_U verdicts are safety, stalls are liveness, and an instance (κ, t, key) denotes a parametric property; fewer cFS collisions (14 lines) | names a set of traces, not an authored artefact with provenance; mined mirror image too (Daikon reports "properties"); collides with OSAL "property object" structs in the C1 API family (`osapi-countsem.h:166`) | **alternative 2**; use it for what an instance denotes |
| **required typestate / readiness typestate** (요구 typestate) | Strom & Yemini, TSE 12(1):157–171, 1986 (V, re-confirmed by the synthesis search); Fugue (ECOOP 2004, V per T2, T3); concurrent typestate: Joshi & Sen, ASE 2008 (V per T3; the synthesis re-search did not return it), Beckman, Bierhoff, Aldrich, OOPSLA 2008 (V per T3); framework-lifecycle typestate: Lifestate (V) | the most precise term for the **lifecycle** conjuncts of `Req`: Kairos's lifecycle automata are typestate automata; 0 collisions in cFS | covers neither value conjuncts, `Allowed`, harm nor provenance; K-CAP (`depth(p) < cap_B(p)`, `BuffCount < MsgLim`), K-ATOM (check-then-act without interleaving) and K-HANDOFF (queue protocols) are not typestate; reviewers will judge Kairos as a typestate checker (aliasing, FSMs) | **use for lifecycle conjuncts of `Req`**, not as the umbrella |
| **readiness precondition** (준비 사전조건) | Hoare 1969 (V per T2, T3); DbC; necessary-precondition inference (Cousot et al., VMCAI 2013, V per T2, T3) | exact for a documented caller obligation (R2) | wrong for C1 (OSAL has no such precondition) and for the SB loss; drops `Allowed`; cFE never says "precondition" | **use only for `Req` of `contract-backed` entries** |
| readiness invariant / likely invariant (불변식) | Daikon (Ernst et al., TSE 2001, V per T2, T3; SCP 2007, V per T1, T3); AVIO, DefUse, PSet, AI (and CCI, as AI groups it) | — | **first**, κ is a specification object (required state, accepted not-ready outcomes with horizons, `appreq`, harm, provenance and scope), while an invariant is a predicate that should hold at given points, whether written, proved or inferred (Daikon's definition, T3 §2.1, snippet, P; AI p. 331, text seen); even `Req` need not hold at every use (`TOLERATED`). **Second** [I], a reader-side risk: in the concurrency-bug line the named interleaving invariants are learned from runs judged correct (§4), so the word may be read as learned. Also collides with DbC class invariants and with the spec's own specified usage ("the OSAL lock invariant", spec §7.1 source 8) | **never** for κ; use "invariant" only in its specified sense (proof-side facts such as MustBefore, documented supplier facts such as the OSAL lock-release note, state-machine invariants as in spec §12 D6) and write "learned" or "likely" invariants for comparators |
| readiness protocol | Vault (PLDI 2001, V per T3); object protocols (Beckman, Kim, Aldrich, ECOOP 2011, V per T2); session types | familiar in PL | implies call sequences on one object; collides with CFDP, SBN and SpaceWire "protocol" (56–59 FSW lines). CF has 18 "protocol" lines and also 16 "specification" lines, so the CFDP collision does not separate the two candidates | reject (the single-object call-sequence connotation is the reason) |
| readiness requirement | FRET/Ogma "requirements" (P per T2) | neutral; the flight-software word | collides with cFE Functional Requirements (`cES1335` "Wait for Startup Sync"; and the `requirement` provenance kind proposed in §7 Q5), with κ's own `Req` field, and with systemd's "requirement dependencies", which explicitly carry **no order** | reject |
| readiness assumption | cFE `\par Assumptions`; CF's "assume this is what is going on"; A/G "assumption" | exact for the **source text** of admitted entries | collides with spec §6.1 "Assumptions A" (the soundiness manifest); flips the obligation (an assumption is what the analysis takes for granted) | use only to name the source text |
| readiness rule | SDV "API usage rules" (EuroSys 2006, V per T2) | C precedent | collides with Kairos HB rules R1–R19 | reject |
| readiness obligation | DbC flavour | — | collides with "proof obligation" and spec §4's "candidate obligations" | reject |
| readiness dependency | PDG (Ferrante et al., TOPLAS 1987, V per T3); systemd | familiar to systems readers | conflates "needs R" with "needs R first"; collides with Kairos violation chains and build dependencies | reject |
| precedence property | Dwyer et al., ICSE 1999 (V per T3 bibliographic; precedence wording P, snippet) | an exact explanation of the derived order obligation | loses ABSENT, STALE, capacity and `Allowed` | explanatory mapping only |
| order intention / ordering constraint | Lu et al., ASPLOS 2008 ("programmers' order intentions", V-abs per T4); AI ("programmers' intentions about the correct order", p. 331, text seen); PSet "interleaving constrained" | the community's informal word for ground truth | an intention is not a checkable object; PSet's "constraints" are learned | prose only: Kairos's provenance records **where the intention is written down** (T4 §3.1) |

**The qualifier "readiness"** is kept by all three recommending reports.
- It is native cFE vocabulary: core apps "reach their respective 'ready' states", and there is `CFE_ES_SystemState_CORE_READY` (T3 §0 item 9, [F:code]).
- It is standard systems vocabulary [V-doc; T4]: Kubernetes "Readiness probes determine when a container is ready to accept traffic"; systemd `READY=1` "readiness notification".
- It collides with **I/O readiness** (`epoll(7)`, `select(2)`, V-doc; T4). Write "resource readiness" or "lifecycle readiness" on first use.
- T4's one extended search found no bug-detection paper that uses "readiness" as its term. This is a negative result, not proof of absence.

**A noun alone cannot do the work** (T3 §0 item 2). Nearly every candidate exists in a specified form and a mined form:
- specifications vs "Mining Specifications";
- properties vs Daikon's reported "properties";
- typestate vs typestate inference (Beckman & Nori, PLDI 2011, V per T3);
- preconditions vs necessary-precondition inference;
- protocols vs function-precedence-protocol inference (Ramanathan et al., ICSE 2007, V per T3);
- rules vs Perracotta's mined "temporal API rules" (J. Yang et al., ICSE 2006; V per T3, initials P).

Whatever noun is chosen, the provenance qualifier and the source tier carry the distinction.

---

## 4. The key distinction: SPECIFIED (provenance-tracked) vs LEARNED (from runs)

### 4.1 Definitions

- **LEARNED.** The expectation is computed from a set of training runs.
  - **AI.** AI defines a belonging set over "verified interleavings": `BSet(S_y)` is "the union of all the remote predecessors of its dynamic instructions that have been seen in verified interleavings". Its invariant is `RPre(I_x S_y) ∈ BSet(S_y)` (AI §2.1, p. 332; text seen). The accepted set is a **union over training runs**.
  - AI's training oracle is output-based (p. 334, text seen): "In order to infer AI automatically without any programmers' annotation, we rely on correct runs observed during the in-house testing phase. As pointed out by prior work [35], programmers can assert whether a test run is correct or not by verifying the outputs."
  - AI's definitional sentence uses "invariant" in the *specified* sense and marks the learned sense separately (pp. 331–332, text seen): "Program invariants are predicates that should always be true at certain points of the execution; they reflect programmers' intentions. Many recent works pay close attention to learn 'likely invariants' from testing runs … These invariants are supposed to be held in all the correct runs."
  - **Daikon** "reports properties that were true over the observed executions" (Daikon manual; snippet, P; T3). Daikon does not judge runs correct; it infers likely invariants from whatever executions it observes.
  - **Statistical isolation** (CCI, Jin et al. OOPSLA 2010). It samples interleaving predicates in deployed runs and ranks them by their correlation with failure, over failing and successful runs [P: mechanism from the title, "cooperative concurrency bug isolation", and the CBI tradition; the paper was not read]. It is learned, but it is not trained only on correct runs.
- **SPECIFIED (provenance-tracked).**
  - The expectation is fixed **from a text outside the executions** or **by a fixed rule over the program text**, before evaluation. The texts are an API header, a cFS requirement (kind `requirement`, added in spec revision 1.1, pending PI item QT-2), a code comment or an issue. Since revision 1.1 each record also states its side (`supplier`, `caller` or `rule`), and only `caller` and `rule` records can decide a status (spec §1.6).
  - Each part carries provenance: `path:line`, sha256, author, authoring time, and which cases the author had inspected.
  - It is frozen by hash at F1/F2 (spec §7.2). Runs are only checked against it. Run-derived sources (§7.1 sources 2, 10, 11) produce instances or candidates that can prompt authoring, never a trained entry. Later changes are labelled `oracle refinement` and reported apart.
- **The middle tier: CODE-DERIVED.**
  - Some Kairos entries are specified *from the program text* by a fixed rule:
    - `code_pattern`: retry-loop-derived bounds, lifecycle symmetry, context-lookup symmetry;
    - `code_admission`: CF's race comment.
  - These are close to the MUST beliefs of Engler et al. ("Bugs as Deviant Behavior: A General Approach to Inferring Errors in Systems Code", SOSP 2001, V per T3). A reviewer will read rule-generated entries as static inference or specification mining, not as derivation from a reference.
  - They are **not learned from runs**, so they do not share the blindness of §4.2.
  - They can be **circular** relative to the code under analysis: an `Allowed` bound read from the very loop that is judged makes that loop conform by construction. They must be labelled as such (T2 §8 risk 3).

### 4.2 Fixed-order violations: which learners cannot flag R2

**Facts** [F: spec §1.11, App. A.1; T1 §6.1; task context].
- In every boot, the ES and EVS tasks use SB's AppId before SB publishes it: 24 uses per boot (20 ES plus 4 EVS), observed 3/3 with gdb and with an instrumented build.
- The uses are HB-ordered **before** the publication by core serialization.
- The value read is the EarlyInit zero (`memset`, `cfe_sb_init.c:50`). The publication is `CFE_ES_GetAppID(&CFE_SB_Global.AppId)` at `cfe_sb_task.c:138`.
- The harm is `event_lost_silently`: EVS returns `CFE_EVS_APP_ILLEGAL_APP_ID`, every SB caller discards it, and the boot reaches OPERATIONAL.

**Structural observation for union-closed learners** [I]. This follows from the definitions; it is not a predicted measurement.
- Let a behaviour `o` occur in every execution of a basis B. Take a learner whose accepted set contains **every** behaviour of the training runs its oracle accepts, i.e. the set is closed under union over training runs. AI is such a learner: `BSet` is the union of remote predecessors seen in verified interleavings (AI §2.1, p. 332; §3.2, p. 334; text seen). So are DefUse, PSet and AVIO, and Daikon's all-samples value invariants. Then either:
  - (a) the oracle accepts those runs, so `o` is accepted and never flagged; or
  - (b) the oracle rejects every run containing `o`, and the training set is empty.
- For R2, case (a) holds: the loss is silent, so every boot passes an output oracle.
- Case (b) would not help either: under the default B **no correct boot exists**. The only runs without `o` come from a different basis B′ (another generated core order in `cfe_static_module_list.c`), which is a different program.
- Hence **no learner that accepts every behaviour seen in runs judged correct** can flag R2 when trained on runs of B. This covers AVIO, DefUse, PSet and AI, and Daikon for value invariants: Daikon would report the zero AppId at the use as an invariant. Spec §1.11 already makes this argument for DefUse. T1 makes it for AI at the instruction level, where the EarlyInit `memset` site (or `nil`, if the intrinsic is not traced) enters `BSet` of the capture load.
- **Not every learner is of this kind** [I].
  - **Deviance ranking.** Frequency- or deviance-based learners do not accept everything they saw. An Engler-style MAY-belief ranking, or a per-API outlier ranking of return codes over runs, could single out R2's 24 `ILLEGAL_APP_ID` returns against the `SUCCESS` returns at later `SendEventWithAppID` uses. The spec itself already ranks a neighbouring site this way: §7.1 source 9 flags `cfe_sb_task.c:138`, which ignores `GetAppID`'s status.
  - **What a ranking can and cannot give.** Such a ranking could surface R2's sites, but only as a ranking. It is not a verdict, and it cannot give the fixed-order certificate.
  - **Statistical isolation.** CCI needs failing runs to contrast [P]. With no failing boot it reports nothing, for that reason and not because it accepted the order.

**AI's own premise does not hold for R2.** AI §1.2, p. 331 (text seen): "there exists a turning point t during the manifestation of a concurrency bug: before t, the manifestation is non-deterministic; after t, the manifestation becomes deterministic." AI's abstract places concurrency bugs' difficulty in "their non-deterministic nature" (p. 330, text seen). R2's outcome is fixed by the basis before the first task runs, so there is no turning point inside the run (T1 §6.2).

**Stalling cannot repair R2** (T1 §6.1) [I].
- Suppose a guard were hand-given the correct predecessor. Stalling ES main at its use waits for SB's store. But SB's task is spawned only after ES main's later raise and the startup thread's `MainTaskSyncDelay` (`cfe_es_start.c:835`), so the wait is circular.
- A stall cap below the 30 000 ms core-startup limit expires and the event is still lost. A cap above it makes the startup thread panic.
- The MustBefore certificate is the proof that no schedule, and hence no stall, avoids the violation.
- This is consistent with AI's own admission (p. 334, text seen): "not all Anticipating Invariant Violations can be tolerated by just perturbing the thread schedule".

**With a provenance-tracked specification** [F: spec App. A.1, §7.2].
- `K-ID.send-with-appid` is written once from `cfe_evs.h:168–172`, per API and never per case, and instantiated at every `CFE_EVS_SendEventWithAppID` site. It has a value conjunct, `valid(arg.AppID)`, evaluated at the capture point, and a registry conjunct (spec §1.4, App. A.1 step 2).
- It is not trained, so it flags R2 in every boot.
- The MustBefore certificate lifts the observation to `fixed-order` over Exec(B, σ_U, M).
- Honesty point: the certificate is reported as "generic contract K-ID + framework rule R19 (CORE-SERIAL)". R19 was authored with the case known, and the development set is not blind.

**Race detectors** report nothing either, because the pair is ordered (spec §1.11).

### 4.3 Retry-tolerated readiness: C1

**Specified.**
- `Allowed = retry_until_ready(OS_ERR_NAME_NOT_FOUND, effective bound)`. The bound is the build's constant-evaluated `CF_STARTUP_SEM_MAX_RETRIES` × `CF_STARTUP_SEM_TASK_DELAY`; the defaults are 25 and 100.
- A single early failed lookup is therefore `TOLERATED` (an allowed not-ready outcome), not a violation.
- The bad case is exhaustion, classified `timeout-dependent` (spec App. A.4).

**Learned.**
- **What a per-access invariant cannot express** [I]. A per-access learned invariant judges each access, not the outcome and its continuation within a bound.
  - AI would accept the not-ready interleaving if a CF-first boot appeared in training, because `BSet` is a union over training runs.
  - What it cannot express is the **horizon**. Every retry iteration has the same remote predecessor. So, at the memory level, a single early failure followed by success looks the same as all 25 retries running out.
  - The learner either accepts every retry, which also hides exhaustion, or flags the first not-ready lookup.
  - Kairos's `retry_until_ready(codes, bound)` separates a tolerated first failure from `timeout-dependent` exhaustion.
- **What T1 §6.1 expects concretely at AI's memory level** [I]:
  - **(a) Silence.**
    - One generic static load in `OS_ObjectNameMatch` serves every slot, object type and caller, so its `BSet` saturates.
    - libc `strcmp` is outside the LLVM passes.
    - This is AI's own stated context limitation (§4.2.5), aggravated by framework code.
  - **(b) A false positive.**
    - Suppose training saw only provider-first boots. A CF-first boot then stalls CF inside the name search, while CF holds the count-semaphore type lock that the provider's create needs to finalize (`osapi-idmap.c:1092`).
    - The result is a circular wait broken only by AI's stall cap. CF then gets `NAME_NOT_FOUND`, and its own retry succeeds.

**Terminology consequence.** C1 shows that *specified* is not the same as *DbC contract*.
- The `Allowed` bound is specified (read from code, never learned from runs). It is CF's **admitted** assumption about its environment, not a promise by OSAL.
- In A/G terms the assumption is the environment's to meet (§2.3).
- Phrase it as "a schedule-dependent readiness condition whose not-ready outcome is handled by a bounded retry; it becomes a violation only if the retry bound can be exhausted" (T4 §5.2).
- Report it under the `admitted` tier (§5.4).

### 4.4 Silent SB losses (subscribe-before-first-send)

- **Learned.** Every-boot losses are absorbed as normal, for the same reason as R2. An example is the 17 `SEND_NO_SUBS` uses chained to R2: ES's EVS packets with MsgId 0x808, sent before any subscription [F: spec §1.6]. They also pass any output-based training oracle by construction, because they are silent (T1 §6.1).
- **Specified.**
  - K-ROUTE is `defer_by_design` where a reference documents the deferral (TO_LAB, `to_lab_app.c:64–66`).
  - Elsewhere it is `unspecified_by_reference`, with outcome label LOSS-UNDER-ORDER: "not-ready-unjudged" rather than "violation" (spec §1.6, §3.15 RC5).
  - The references that do exist specify only the supplier's side. The API says the message is "transmitted to all subscribers" (`cfe_sb.h:401`). Requirement cSB4305.5 mandates a counter and an event, and its rationale says the condition "is not considered an error" (§2.3).
  - No reference requires delivery, or a subscribe-before-send order. Kairos does not call these losses violations. But it **reports** them, and says that no reference decides the question.
  - Spec revision 1.1 answers this as a proposal (PI item QT-2): cSB4305.5 is supplier-side, so it gives the framework-level judgement `documented-not-error` beside the caller-level LOSS-UNDER-ORDER; it is not a `documented_drop` (spec §1.6, §3.9).
- **This matches the systems norm** [V-doc; text fetched by T4; quotes re-checked on disk].
  - ZeroMQ's "slow joiner": "the subscriber will always miss the first messages that the publisher sends".
  - ROS 2 Quality of Service, Durability (`ros2_jazzy_About-Quality-of-Service-Settings.rst:49–50`), two separate bullets:
    - *Transient local*: "the publisher becomes responsible for persisting samples for 'late-joining' subscriptions";
    - *Volatile*: "no attempt is made to persist samples".
  - So under volatile durability a late-joining subscription receives no earlier samples [I].

  A reviewer will accept such a loss as a defect only where a reference requires delivery.
- **This is where "contract" is most clearly wrong.** "Contract violation" asserts a broken promise exactly where cFE's own requirement says the condition is not an error at the framework level, while no consumer or mission reference decides the caller level.

### 4.5 Summary

| Case | Learned (AI, DefUse, …) | Provenance-tracked specification (source tier) | "Contract" accurate? |
|---|---|---|---|
| R2 (fixed-order) | union-closed learners cannot flag it: the order is in every training run (structural). A deviance ranking might rank its sites, without a verdict | violation in every boot; certificate lifts it to `fixed-order` (`contract-backed`, `api_doc`) | **partly**: a DbC-like caller assumption, but the not-ready results are also documented `\retval` codes, discarded by every caller |
| C1 (bounded retry) | silent (saturation), or a false positive with a stall under a lock; a per-access invariant cannot express the retry horizon | single early failure TOLERATED; exhaustion `timeout-dependent` (`admitted`: `code_admission`, caller side; the `@retval` is supplier-side, framework level) | **not a DbC precondition violation**; an A/G-style environment assumption admitted by CF. It would fit "contract" only in the A/G sense, which brings refinement and composition expectations that Kairos does not meet |
| SB loss (silent) | learned as normal; passes the output oracle | LOSS-UNDER-ORDER, unjudged (`unreferenced`; `defer_by_design` for TO_LAB). cSB4305.5 gives the framework-level judgement `documented-not-error` beside it; it does not make the caller level `documented_drop` (spec §1.6, §3.9; PI item QT-2) | **no**: no reference requires delivery, and cSB4305.5 says the condition is not an error |

**What the term must therefore do** [I]:
- (a) name a specification object written from a recorded source or by a fixed rule, never trained on runs: the required state with the accepted not-ready outcomes, the harm, the provenance and the scope. "Invariant" names a predicate (written, proved or inferred), so it fits `Req` at most; its learned reading next to AVIO, DefUse, PSet and AI is a secondary, reader-side risk [I];
- (b) not assert a promise where none exists, which rules out an unqualified "contract" for unreferenced and rule-generated entries (and, for C1, requires the A/G sense);
- (c) cover the required state, the allowed outcomes, the harm and the provenance.

"Readiness specification", with the first-use qualifier "provenance-tracked" and the source tier, meets all three. "Reference-derived" meets (b) only for the tiers that have an external reference, so it is reserved for them (§5.4).

---

## 5. Recommendation

### 5.1 Primary term and definition

**English: readiness specification.** On first use in a paper, write *provenance-tracked readiness specification*. The symbol κ stays. Never abbreviate it to "spec" (SPEC_FINAL).

> **Definition.** A *readiness specification* κ for a use pattern U of a framework API states, for the resource that each matching use selects by key:
> - (i) the **required state**: a conjunction of lifecycle (typestate) or value predicates on the key-selected resource, each evaluated at its own linearization point or capture point;
> - (ii) the finite set of **allowed not-ready outcomes** (those that the caller's, consumer's or mission's references accept; a supplier's documented result alone is not one), each with the continuation horizon its reference gives, or the explicit mark that no reference decides (`unspecified_by_reference`);
> - (iii) the **provenance** of each part (API documentation, a cFS requirement, a code admission, a code pattern, an issue, or the researcher), with location, text hash and side (supplier, caller or rule);
> - (iv) the **harm class** recorded when a not-ready outcome is not allowed (e.g. `event_lost_silently`, `app_error_exit`).
>
> Each conjunct is paired with the framework's own validity predicate, which serves as an independent observation of the required state. The state is typically established, or kept true, by other actors. Which actor does so, and in what order, is not part of κ; Kairos derives it (provider axis, HB/MustBefore). A readiness specification applies before the shutdown request. It is authored, or generated by a fixed rule from the program text, before evaluation, and frozen by hash. It is never trained on executions. Run-derived candidates can only prompt authoring, and any later change is labelled oracle refinement and reported separately.
>
> A **readiness violation** is an in-scope occurrence at which the required state does not hold and, where the allowed outcomes are specified, the outcome is not among them. Where no reference decides (`unspecified_by_reference`), the occurrence is reported as not-ready-unjudged, or as LOSS-UNDER-ORDER for route and capacity entries, and is not called a violation.

Notes on the definition:
- "A cFS requirement" in (iii) is the `requirement` provenance kind (requirement ID plus text hash; §7 Q5), which spec revision 1.1 adds pending PI item QT-2 (STATUS decision T2, re-asked). If the PI declines it, (iii) lists the spec's other five kinds. The side of each record follows spec §1.6.
- "Typestate" covers only the lifecycle conjuncts (§5.3).
- The violation clause matches spec §1.6 "Instances": `VIOLATING` iff not ready and rejected by the caller-level Allowed (its horizon monitor gives REJECT, or `Allowed = none`); `NOT_READY_UNJUDGED` iff not ready and the caller-level `Allowed = unspecified_by_reference`. It also matches the outcome labels of spec §1.10. "Allowed" in (ii) and here means the caller-level Allowed (spec §1.6).

**Korean: 준비 상태 명세.** On first use, write 근거 기록 준비 상태 명세. **Always write 준비 상태 명세 in full; never bare 명세.**
- **Why.** The Korean Architecture documents cite SPEC_FINAL with the word "spec" about 1,700 times. Of these, 1,204 are "spec §x"; for example, `01_overview.md` has "spec §1.11" ×13 and "spec §6.4" ×11. Korean readers map spec to 명세, so a bare 명세 for κ would collide with the document name, as in "spec §1.6의 명세".
- **Alternative.** If the full form is too long in running text, choose a Korean head noun that is not used for the document, e.g. 준비 조건 (a PI choice, §7 Q9).

> **정의.** 준비 상태 명세 κ는 프레임워크 API의 사용 패턴 U에 대해, 각 사용이 key로 고르는 자원에 관해 다음을 정한다.
> - (i) **요구 상태**: key로 고른 자원에 대한 lifecycle(typestate) 또는 값 술어의 논리곱이다. 각 conjunct는 자신의 LP 또는 capture point에서 평가한다.
> - (ii) **허용된 미준비 결과**(호출자·소비자·임무 쪽 근거가 받아들이는 것이다. 공급자의 문서화된 결과만으로는 해당하지 않는다): 유한한 목록이며, 각 항목은 reference가 준 continuation horizon을 가진다. 어떤 reference도 정하지 않으면 그 사실(`unspecified_by_reference`)을 명시한다.
> - (iii) 각 부분의 **근거**: API 문서, cFS 요구사항, 코드 주석의 인정, 코드 패턴, issue, 연구자 중 하나이며, 위치, 텍스트 hash, 근거의 쪽(공급자·호출자·규칙)을 기록한다.
> - (iv) 허용되지 않은 결과에 기록하는 **harm 분류**(예: `event_lost_silently`, `app_error_exit`).
>
> 각 conjunct에는 프레임워크 유효성 술어(Oracle)가 짝지어지며, 이는 요구 상태를 독립적으로 관찰하는 수단이다. 요구 상태는 대개 다른 actor가 성립시키거나 유지한다. 어느 actor가 어떤 순서로 그렇게 하는지는 κ에 속하지 않으며 Kairos가 도출한다(provider 축, HB/MustBefore). 적용 범위는 종료 요청 전이다. 준비 상태 명세는 평가 전에 작성하거나 프로그램 텍스트에서 고정된 규칙으로 생성하며, hash로 동결한다. 실행으로 학습하지 않는다. 실행에서 나온 후보는 작성의 계기가 될 뿐이며, 이후의 변경은 oracle refinement로 표시하고 따로 보고한다.
>
> **준비 상태 위반**은 범위 안의 발생에서 요구 상태가 성립하지 않고, 허용된 결과가 명세된 경우 그 결과가 허용 목록에 없는 경우다. 어떤 reference도 정하지 않은 경우(`unspecified_by_reference`)에는 not-ready-unjudged로(route·capacity 항목은 LOSS-UNDER-ORDER로) 보고하며 위반이라 부르지 않는다.

The Korean renderings (준비 상태 명세, 근거 기록, 준비 사전조건, 허용된 미준비 결과, 근거 등급) are suggestions in common usage. They were **not** checked against a Korean terminology source such as the KIISE glossary: **UNVERIFIED**. T2 also could not confirm 계약에 의한 설계 as the standard Korean name for DbC: **UNVERIFIED**.

**Abstract-level sentence** [S]:

> Kairos checks each use of a framework resource against a *provenance-tracked readiness specification* (the state the use requires and the not-ready outcomes that the caller's, consumer's or mission's references accept), not against an order learned from runs.

### 5.2 The choice, stated as a trade-off

**Primary: "readiness specification" (준비 상태 명세).** The reasons, in order:
1. It asserts no agreement. So it fits all of these without a second meaning: documented caller assumptions (R2), admitted environment assumptions (C1), rule-generated entries and unreferenced entries. "Contract" needs the DbC sense for R2, the A/G sense for C1, and fits neither for the last two.
2. It brings neither DbC's blame semantics nor A/G's refinement and composition, which Kairos would otherwise have to disclaim.
3. It names an authored artefact with provenance, which is what κ is (see the tie-breaker below).

This ranking does **not** rest on T4's census, which does not separate the candidates (§2.4). Nor does it rest on C1 alone, since C1 fits "contract" in the A/G sense. The name by itself guarantees neither novelty nor accuracy (EXT-20261012 §6.1).

**Alternative 1: keep "readiness contract" (준비 계약).** This is a genuine option.
- *For:*
  - shorter, and no churn;
  - no collision in cFS flight code, whereas "specification" collides with SPEC_FINAL, the "Specification for …" headers, the CFDP specification in CF and EVS's `Spec` parameter;
  - fits R2 (DbC-like) and C1 (A/G-like);
  - T1's AI-centred reading favours it, since "contract" sits on the specified side.
- *Against:*
  - it needs two senses;
  - it brings DbC blame and A/G composition expectations;
  - it asserts an agreement for unreferenced and rule-generated entries;
  - T2, T3 and T4 rank it below "specification".
- *Conditions if kept:*
  - (a) the paper carries the definition paragraph of §5.7(e2), which adopts the DbC reading for `contract-backed` entries and the A/G reading for `admitted` entries, and disclaims refinement, composition and SCOOP waits;
  - (b) every count and verdict is reported per source tier (§5.4);
  - (c) "contract violation" is used only for `contract-backed` entries, and never for `unreferenced` or `code-derived` ones;
  - (d) "API contract" for M_api is kept apart by always writing it in full.

**Alternative 2: "readiness property" (준비 상태 속성).**
- *For:* it is best for the semantics section (safety for σ_U verdicts, liveness for `stall`). Instances are parametric properties in the MOP sense. It has fewer cFS collisions than "specification" (14 lines).
- *Against:* it names a set of traces, not an authored text.
- **The tie-breaker between "specification" and "property"** [I]. κ carries provenance, an author, an authoring time and a version guard, and those are attributes of a written text, not of a trace set. So "specification" names κ itself, and "property" names what an instance of κ denotes. Recommended usage: readiness specification for κ, and readiness property for the set of good executions of an instance, in the formal section.
- The Daikon argument does not separate the two nouns: "property" has Daikon's reported "properties", and "specification" has "Mining Specifications".

**Both nouns need the provenance qualifier** on first use and the source tier in every count.

The three recommending reports converge on "readiness specification" (T2, T3, T4). **T1 recommends keeping "readiness contract"**, on the specified/learned axis alone, and lists "readiness specification" as its option B. This synthesis weighs the overclaiming axis as well (§4.5). It finds that "contract" survives on that axis only under two senses, and leaves the decision to the PI.

### 5.3 Supporting vocabulary

| Concept | English (paper and spec) | Korean (Architecture docs) | Anchor (status; source report) |
|---|---|---|---|
| κ | readiness specification (first use: provenance-tracked readiness specification) | 준비 상태 명세 (first use: 근거 기록 준비 상태 명세); never bare 명세 | Lifestate (V, re-confirmed by the synthesis search); MOP/JavaMOP parametric properties and monitoring (V per T3); Ammons et al. as the mined counterpart (V per T1–T3) |
| instance `(κ, t, key)` | readiness instance; it denotes a parametric readiness property | 준비 상태 명세 instance (short form: 준비 instance [S]) | parametric trace slicing (Chen & Roşu, TACAS 2009, V per T3) |
| `Req` (field name kept) | required state: lifecycle (typestate) or value conjuncts. "Typestate" covers only the lifecycle conjuncts. **Non-typestate members of the catalogue:** K-CAP (`depth(p) < cap_B(p)`; `BuffCount < MsgLim`), K-ATOM (check-then-act without interleaving, i.e. serializability) and K-HANDOFF (queue protocols). Write **readiness precondition** only for `contract-backed` entries | 요구 상태(lifecycle typestate 또는 값 술어) / 준비 사전조건 | Strom & Yemini 1986 (V, re-confirmed by the synthesis search); DeLine & Fähndrich 2004 (V per T2, T3) |
| `Allowed` (field name kept) | allowed not-ready outcomes at the caller level (client handling and acceptance: retry, defer, fallback, documented exit, the using module's own documented drop); supplier-side **documented results** (`\retval`, cFS requirements, a supplier's drop) are the framework-level judgement (spec §1.6 "Three judgements") | 허용된 미준비 결과(호출자 수준) / 문서화된 결과(프레임워크 수준) | JML exceptional behaviour (P-doc; T2); Linux `-EPROBE_DEFER` (V-doc; T4) |
| `harm` | harm class (consequence when not allowed) | harm 분류 | spec §1.6, §1.10 |
| `Oracle` | framework validity predicate (the second readiness oracle) | 프레임워크 유효성 술어 | spec §7.2 |
| bad occurrence | **readiness violation** (informal: use-before-ready), a subclass of **order violation** where a provider exists; unreferenced occurrences are **not-ready-unjudged** or LOSS-UNDER-ORDER | 준비 상태 위반 / 판정 불가 미준비 | AI p. 335 W-R order violation (text seen); Lu et al. ASPLOS 2008 (V per T2–T4; definition wording UNVERIFIED); InitRacer OOPSLA 2017 (V per T3, T4) |
| order that establishes `Req` | ordering (HB, MustBefore), stated as **orthogonal** to the requirement; provider axis | 순서 관계 | systemd: ordering dependencies are "independent of and orthogonal to the requirement dependencies" (V-doc, text; T4; `systemd.unit` l.855) |
| `TOLERATED` / refinement `tolerated` | spec: keep the names. Paper: rename to **ALLOWED_NOT_READY** / `allowed` [S]. In prose write "an allowed not-ready outcome", never "Kairos tolerates" | 허용된 미준비 발생 | avoids "tolerating concurrency bugs" (AI; Yu & Narayanasamy 2010, P) (§6.5) |
| source tier (new, derived) | contract-backed / requirement-derived / admitted / issue-derived / code-derived / researcher / unreferenced (§5.4) | 근거 등급: API 계약 근거 / 요구사항 근거 / 코드 인정 / issue 근거 / 코드 유도 / 연구자 / 근거 없음 | T2 §9.2 |
| documented supplier semantics | **API contract** (kept) | API 계약 (kept) | Meyer 1992 (V per T1–T3, bibliographic) |
| learned comparators | learned (likely) invariants; statistical isolation (CCI) | 학습 불변식 (kept) | AI (V); DefUse (V per T1, T3, T4); Daikon (V per T1–T3); CCI (V per T1, T3) |
| "invariant" inside Kairos | in its specified sense only: proof-side facts (MustBefore), documented supplier facts such as the OSAL lock-release note behind K-LOCKBAL ("the OSAL lock invariant", spec §7.1 source 8) and state-machine invariants (spec §12 D6); comparators are always "learned (likely) invariants" | — | T3 §0 item 3; AI p. 331 |
| message provenance `Prov(p)` | always "message provenance", never bare "provenance" | 메시지 출처 | spec §3.x, §4.3, glossary |

### 5.4 Source tier and reporting rule

- **Source tier** [S] (KO 근거 등급). This is a field derived from `prov` **and** `Allowed`, so no extra authoring is needed. It is not called a "provenance tier", because the spec already uses "provenance" for κ's `prov` and for message provenance `Prov(p)` (22 lines say "message provenance"; 60 lines contain "provenance" in either sense).
  - `contract-backed`: a reference states the condition as an obligation on the caller (judged by the author and recorded), whether or not the supplier also documents an error return; and every caller-side `Allowed` entry has a documentation or requirement source (`api_doc` or `requirement`), none from code; supplier-side documentation of the not-ready result (an error return) does not change the tier (spec §1.6). Example: R2's K-ID.
  - `requirement-derived` (the `requirement` kind, added in spec revision 1.1 pending PI item QT-2): some caller-side part from a cFS requirement, and nothing weaker.
  - `admitted`: some part from `code_admission`.
  - `issue-derived`: some part from `issue`.
  - `code-derived`: some part from `code_pattern`.
  - `researcher`: some part from `researcher`.
  - `unreferenced`: `Allowed = unspecified_by_reference`.
- **"Reference-derived"** is reserved for the first four tiers (contract-backed, requirement-derived, admitted, issue-derived). A `researcher` entry is not a reference. `code_pattern` entries are generated by a rule from the program under analysis.
- **Mixed provenance.** A mixed entry takes the weakest tier of its parts, in the order listed. Example: `K-REG.osal-countsem-lookup` combines OSAL's `@retval` (`api_doc`) with CF's comment (`code_admission`), so it is `admitted`; in spec revision 1.1 the `@retval` is a `supplier` record and decides no status, and the tier rests on CF's caller-side comment. The order itself is a PI decision (§7 Q4).
- **Reporting.**
  - Census counts, verdicts and recall are reported per tier.
  - Every bad occurrence with specified `Allowed` is a "readiness violation" with its tier.
  - "Contract violation" is not used in the paper unless the PI keeps "contract", and then only for `contract-backed` entries.
  - `unreferenced` instances keep the spec's existing labels, `not-ready-unjudged` or LOSS-UNDER-ORDER, and are never called violations.

### 5.5 Mapping table: old term → new term

**A. SPEC_FINAL.md (English).** 79 lines contain "contract". Sections are those in which each phrase occurs. This count and the counts in B and C were taken in revision 2, at commit b4ac55f, before spec revision 1.1; recount before applying any renaming.

| Old term | New term | Where (spec) | Note |
|---|---|---|---|
| readiness contract(s); "Readiness contracts" (heading) | readiness specification(s); "Readiness specifications" | §0.1, §1.6 heading | first use in the paper: "provenance-tracked readiness specification" |
| contract (bare, meaning κ): "A contract is κ = …" | readiness specification (or "specification" when unambiguous) | §0.2, §1.6, §2.3, §3.x, §4.x, §6.3, §8.x | do **not** abbreviate to "spec" (SPEC_FINAL) |
| readiness-contract instance; contract instance(s); "per contract instance" | readiness instance(s) | §0.1, §1.6 "Instances", §2.3 (`KZ_VERDICT_FIELDS` field `instance`, comment "(contract, context-qualified template, key)"), §8.1 RQ1, §12 D3 | — |
| contract library | readiness specification catalogue | §3.15 heading | — |
| 15 readiness-contract kinds; contract kinds | 15 **readiness categories** RC1–RC15 | §3.15, §7.7, ADR-024 | keep the IDs. Avoid "kind", which is taken by resource kinds |
| K-* contract templates; "generic contract K-ID"; "contract K-SELF"; "Generic K-SELF contract" | readiness templates K-*; generic specification K-ID; specification K-SELF | §2.3 examples, §3.3, §3.15, §6.4, §7.2, App. A.1, ADR-031, glossary | **IDs K-* unchanged** |
| DSL keyword `contract K-ID.send-with-appid { … }` | `readiness K-ID.send-with-appid { … }` (optional) | §2.3 | the DSL is not implemented yet, so the change is cheap |
| contract checking; dynamic contract checking | readiness checking; dynamic readiness checking | §0.2, §9.3 S2, §12 | — |
| contract-use sites; contract-relevant; contract-site outcomes | readiness use sites; readiness-relevant; use-site outcomes | §3.2, §3.4, §4.7, §4.9, §5.2, ADR-015 | — |
| contract oracle | readiness oracle (κ's Req, Oracle and Allowed evaluated on the trace) | §4.13, §6.3 G6 | §7.2 already speaks of "two readiness oracles" |
| Contract governance and anti-circularity | Specification governance and anti-circularity | §7.2 heading | — |
| Retry/timeout-loop-derived contracts | loop-derived readiness specifications (tier `code-derived`) | §7.1 source 4 | — |
| reference-sourced contract (§1.11 R2 row); reference-based contracts (§8.3 DefUse row) | reference-derived readiness specification (R2's K-ID, tier `contract-backed`); provenance-tracked readiness specifications (general) | §1.11 R2 row, §8.3 DefUse row | "reference-derived" only for the reference tiers (§5.4) |
| "Typestate contracts \| Strom & Yemini 1986 [V-me]" | "Typestate \| Strom & Yemini, IEEE TSE 12(1):157–171, 1986 [V]" | §1.11 established theory | "typestate contract" is not the paper's phrase (T2, T3) |
| HB-ordered contract violation | HB-ordered readiness violation | §1.11 "Defensible contribution" | — |
| the contract violated (mutants) | the violated readiness specification (instance) | §8.5 | — |
| route and capacity contracts; other contracts; cross-run contracts; capacity contracts | route and capacity specifications; other specifications; cross-run specifications; capacity specifications | §1.6, §1.7, §1.10, §3.9, §3.15 | consider the names **capacity property** (K-CAP) and **atomicity specification** (K-ATOM) inside the catalogue (T3 §5.3) |
| Contract K-LOCKBAL | **lock-balance postcondition** K-LOCKBAL (supplier postcondition; implementation-defect family) | §3.2, §4.13 | a PI decision whether to move it out of the readiness catalogue (§7 Q7) |
| Contracts off (ablation) | readiness specifications off | §8.4 item 16 | — |
| DCatch-style framework HB without contracts | … without readiness specifications | §8.3 | — |
| `Allowed`: "documented not-ready behaviours" | allowed not-ready outcomes (with reference horizon, or `unspecified_by_reference`) | §1.6, glossary | field name `Allowed` kept |
| `Req` (gloss) | required state (lifecycle typestate or value conjuncts); readiness precondition for `contract-backed` entries | §1.6, glossary | field name kept |
| `prov` kinds | `requirement` (requirement ID + text hash) and each record's side, both added in spec revision 1.1 (pending PI item QT-2) | §1.6, §7.2 | §7 Q5 |
| glossary: "Census: instantiation of every contract"; "Scope: contracts apply before …"; "LOSS-UNDER-ORDER: … route/capacity contracts" | "… every readiness specification"; "readiness specifications apply before …"; "… route/capacity specifications" | §13 | — |
| **unchanged:** "API contract", "OSAL API contract", "API-contract model / claims" (M_api) | keep | §0.2, §0.6, §1.4, §6.3 G2, ADR-008, glossary | correct DbC usage (supplier semantics) |
| **unchanged:** κ; field names; enums `READY`/`TOLERATED`/`VIOLATING`/`NOT_READY_UNJUDGED`; refinement `tolerated`; LOSS-UNDER-ORDER; shapes; "message provenance" | keep in the spec | — | add the derived `tier` (source tier, §5.4); the paper may rename `TOLERATED` (§6.5) |

**B. Architecture documents (Korean).** Lines containing 계약, by file: 05:31, 09:30, 01:26, 04:24, 03:21, 10:19, 07:17, 08:16, 02:13, 06:11, README:2 (210 lines; revision 2 count, before spec revision 1.1). Every target uses 준비 상태 명세 in full (§5.1).

| Old (KO) | New (KO) | Where | Note |
|---|---|---|---|
| 준비 계약 (readiness contract) | 준비 상태 명세 (first use: 근거 기록 준비 상태 명세, provenance-tracked readiness specification) | `01_overview.md` §1.2 heading; `04_formal_model.md` §5 heading and §0.3 table | first use with the English term in parentheses |
| 계약 (κ, bare) | 준비 상태 명세 | all ten files | never bare 명세 |
| 계약의 요소; R2의 계약 | 준비 상태 명세의 요소; R2의 준비 상태 명세 | `04_formal_model.md` §5.1, §5.7 | — |
| actor·자원·계약 | actor·자원·준비 상태 명세 | `04_formal_model.md` title; `README.md:18` | — |
| 계약 instance | 준비 상태 명세 instance (short form: 준비 instance) | 03, 07 | — |
| 계약 template K-* | 준비 상태 명세 template K-* | all ten files and `README.md` | IDs unchanged |
| generic 계약 K-ID | generic 준비 상태 명세 K-ID | 01, 02, 03, 04, 05, 07, 08 | — |
| 계약 라이브러리와 15개 계약 종류 | 준비 상태 명세 목록과 15개 준비 범주 | `05_cfs_mapping.md` §3 heading and title ("계약 coverage" → "준비 상태 명세 coverage") | — |
| 동적 계약 검사; HB와 동적 계약 검사 | 동적 준비 상태 검사 | `08_roadmap.md` §4.4 heading; 03, 07 | — |
| 계약 사용 (위치, site) | 준비 상태 명세 사용 site | 04, 05, 06, 10 | — |
| 순서가 정해진 계약 위반 | 순서가 정해진 준비 상태 위반 | 01, 09 | — |
| 허용되는 / 허용된 not-ready 행동·발생 | 허용된 미준비 결과·발생 (or keep the docs' convention: 허용된 not-ready 결과) | `04_formal_model.md` §0.3; `01_overview.md:475`; `03_workflow.md:649`; `10_decisions_adr.md:471` | pick one Korean form for all files |
| 용량 계약; 영속 복원 계약; 실행 사이 계약 | 용량 준비 상태 명세; 영속 복원 준비 상태 명세; 실행 사이 준비 상태 명세 (or 용량 속성 if K-CAP becomes "capacity property", §7 Q7) | 용량: 03, 04, 05, 06, 10; 영속 복원: 09; 실행 사이: 04 | — |
| 계약 K-LOCKBAL | 잠금 균형 사후조건 K-LOCKBAL | 01, 06, 09 | see §7 Q7 |
| reference 계약 (vs 학습 불변식) | 근거 기록 준비 상태 명세 (vs 학습 불변식) | `09_prior_work.md` §4.4, §7.2 | 학습 불변식 unchanged |
| 계약의 뿌리: typestate | 준비 상태 명세의 뿌리: typestate | `09_prior_work.md` §4.12 heading | — |
| **unchanged:** API 계약 (수준, model) | keep | 01–05, 07–10 | M_api |

**C. Diagrams (English labels).** These files used "contract" in revision 2 (before spec revision 1.1):
- `D0_big_picture.mmd` ("HB rules, contracts" → "readiness specifications");
- `D2_components_static_detail.mmd` ("rules, contracts (human-authored)" → "rules, readiness specifications (provenance-tracked)");
- `D3_analysis_overview.mmd` ("contract instance i = (κ, …)" → "readiness instance i = (κ, …)");
- `D4_research_overview.mmd` ("S2 trace HB + dynamic contracts" → "… dynamic readiness checking");
- `D9_teardown_scoping.mmd` ("contracts apply" → "readiness specifications apply").

The SVGs must be regenerated.

**D. Background notes R01–R07.** These are dated inputs, cited by section, so leave them unchanged [S]. Add one line to `background/README.md`: "Terminology: 'readiness contract' in R03–R06 is called 'readiness specification' from 08 on; see `08_terminology.md`". R03's original definition is order-based (§1.1); say so in that line.

### 5.6 What does not change

- κ and all field names.
- The K-* and RC IDs.
- The spec's enum and refinement names. Only the paper renames `TOLERATED` / `tolerated` (§6.5).
- "API contract" for M_api.
- "learned invariant" for the comparators.
- The spec's labelling of the SB loss (LOSS-UNDER-ORDER, `unspecified_by_reference`, at the caller level), with the framework-level judgement from cSB4305.5 reported beside it (spec revision 1.1 §3.9; PI item QT-2). Under the new term this labelling no longer contradicts the umbrella word.

### 5.7 How to phrase it in the paper

All drafts are [S]. Citations use the statuses in §8. Items marked "check before quoting" need the primary text opened first.

**(a) Problem statement:**

> Flight-software frameworks such as NASA's core Flight System start many tasks that share framework resources: named OS objects, application identities, message routes and tables. A use of such a resource requires the resource to be in a state that, typically, another task establishes. A semaphore must exist before it is looked up by name; an application must be registered before events are sent under its identity. The framework's own documentation states many of these requirements. cFE's event API, for example, says that "the Application ID must correspond to a registered application or core service". It also documents which not-ready outcomes occur, such as error returns, documented drops and deferrals, and applications add their own bounded retries.
>
> We call a use that reaches the resource before it is ready, with an outcome that no reference of the caller, consumer or mission accepts, a *readiness violation*. Where no reference decides, we report the not-ready use as unjudged. Readiness violations are order violations, in which, as AI puts it, "the programmer intends that a write operation should always be executed before another read operation, but this intention is not guaranteed" [Zhang et al. 2014, p. 335; cf. Lu et al. 2008]. They generalize the initialization races of InitRacer [Adamsen et al. 2017] from web pages to the tasks of a flight-software framework. Two properties put them outside existing detectors:
> - Some are ordered the wrong way in *every* execution. In cFE #2663, 24 uses per boot precede the provider's publication. Race detectors report nothing, because the pair is ordered. Detectors that accept every interleaving seen in runs judged correct accept this one as well. Unlike the non-deterministic order violations studied by Lu et al. and AI, no run of the build shows the intended order, so the violation cannot be established by contrasting runs. Kairos establishes it with a must-before certificate.
> - Others are acceptable by design. CF retries a failed semaphore lookup 25 times, so a not-ready state alone is not a bug.
>
> Kairos therefore checks executions against *provenance-tracked readiness specifications*. Each one states the state a use requires and the not-ready outcomes that the caller's, consumer's or mission's references accept; what the framework documents and returns is reported beside it. Kairos then classifies each use as fixed-order or schedule-dependent, with certificates.

Lu et al. 2008's own definition wording is UNVERIFIED; until it is read, the quoted description is AI's (p. 335, text seen) and Lu et al. is cited only for the category.

**(b) Terminology box (definition paragraph).** Use §5.1's definition, followed by the paragraph below. Its bracket after the invariant definition cites AI p. 331 for the definition only (AI also names the learned "likely invariants" there). The clause "it may be written, proved or inferred" rests on T3 §2.1 (classical invariants are specified or proved; Daikon's definition, snippet, P) and on EXT-20261012 §6.2, so it carries no citation until a primary source is read (check before quoting):

> We say *specification*, not *contract*, because only some requirements are documented obligations on the caller in the sense of Design by Contract [Meyer 1992]. Others are assumptions admitted in application code, rules applied to the program text, or have no reference at all. We report these groups separately. We do not call κ an *invariant*. An invariant is a predicate that should always hold at certain points of an execution [Zhang et al. 2014, p. 331]; it may be written, proved or inferred, and learning is not part of the definition. κ is not a predicate but a specification object: it pairs a required state (with the framework's validity predicate) with the not-ready outcomes that its references accept, each with a reference horizon, and with the harm, the provenance of each part and its scope. Even the required state need not hold at every use: a use whose not-ready outcome is accepted is legitimate; and where no reference decides, κ still states the required state but records that no reference says whether a not-ready outcome is acceptable. A secondary, reader-side reason: the word has a learned sense nearby, likely invariants inferred from observed executions [Ernst et al. 2001] and, in concurrency-bug work, interleaving invariants learned from runs judged correct [Lu et al. 2006; Shi et al. 2010; Zhang et al. 2014], so "invariant" next to them may be read as learned.

**(c) Related work: learned order invariants and statistical isolation** (adapted from T1 §6.5 and T3 §5.4):

> AVIO, DefUse, PSet and AI learn, from runs judged correct, which accesses may precede or interleave a given access, and they flag deviations [Lu et al. 2006; Shi et al. 2010; Yu & Narayanasamy 2009; Zhang et al. 2014]. CCI instead ranks sampled interleaving predicates statistically over failing and successful runs [Jin et al. 2010]. AI records, for each static instruction, the set of remote instructions seen to access the same address immediately before it, including an explicit "no predecessor" value. It stalls a thread just before an access whose predecessor was never seen. Kairos differs in where the expectation comes from. Its readiness specifications are written from recorded sources, or generated by fixed rules from the program text, and they are never trained on executions. A wrong order that occurs in every run, as in cFE #2663, therefore remains a violation. Any accepted set built as a union over runs of the same build contains it, and a statistical isolator has no failing run to contrast it with. Because a must-before certificate shows that the provider's publication depends on the user's own progress, no stall can repair it.

Check before quoting: CCI's mechanism (P; the paper was not read). Do not write that AI empirically subsumes AVIO, DUI, CCI and PSet: AI's Table 2 entries for them are "based on our understanding of their algorithms" (text seen, T1 §4).

**(d) Related work: typestate and framework protocols** (T3 §5.4):

> The lifecycle core of a readiness specification is a typestate requirement [Strom & Yemini 1986; DeLine & Fähndrich 2004] on a framework resource whose state, typically, another task establishes. That is the multithreaded setting of predictive typestate checking [Joshi & Sen 2008] and the framework-lifecycle setting of Lifestate [Meier et al. 2019], which checks traces against lifecycle-plus-typestate protocol specifications. A readiness specification also lists the not-ready outcomes that the caller's, consumer's or mission's references accept, each with a reference horizon, and records where each requirement is written down; the framework's documented results are reported separately. [U, P-PAPERS: whether either work records accepted not-ready outcomes with reference horizons is to be checked against the full texts; write "unlike both" only after that check.]

Check Joshi & Sen and Lifestate before quoting, and before claiming any difference, including this one (T3 §7 item 4; P-PAPERS; spec §1.11, App. B).

**(e) Related work: contracts.** Two variants, depending on the PI's decision (§7 Q1).

*(e1) If "specification" is adopted*, as a related-work paragraph:

> Documented caller obligations in Kairos resemble API contracts in the sense of Design by Contract [Meyer 1992], lifted to lifecycle state. cFE, however, also documents the not-ready results as error returns, so Kairos judges how the caller handles the outcome, not only whether the obligation held. Admitted assumptions, such as CF's bounded retry around a lookup of a peer's semaphore, resemble the environment assumptions of assume-guarantee contracts [Benveniste et al. 2018; Cofer et al. 2012], but Kairos performs no refinement or composition. SCOOP [Nienaltowski et al. 2009] turns a precondition on another processor's object into a wait condition, which Kairos does not. Atomicity contracts [Sousa et al. 2015; ICST 2017] list call sequences that must run atomically, not readiness. The cross-thread order that makes a requirement hold is not part of the specification: Kairos derives it with happens-before and must-before analysis.

*(e2) If "contract" is kept*, as the definition paragraph:

> A readiness contract is read in two established senses. Where a reference states the condition as the caller's obligation (for cFE's event API, "the Application ID must correspond to a registered application or core service"), it is a DbC-style obligation lifted to lifecycle state [Meyer 1992], with the supplier's documented error returns as the framework's exceptional outcomes, reported apart from the caller-level judgement. Where an application admits that it assumes its environment establishes the state, and bounds its own handling (CF's retry around a peer's semaphore), it is the environment assumption of an assume-guarantee contract [Benveniste et al. 2018; Cofer et al. 2012], and Kairos asks whether some schedule breaks it. Kairos borrows neither DbC's semantics for undefined behaviour nor A/G refinement and composition, and SCOOP-style waiting [Nienaltowski et al. 2009] is not part of it. Entries that no reference decides are reported as unjudged, never as contract violations.

Check before quoting: Meyer 1992 (text not seen), Benveniste et al. 2018 definitions (P), and the FAC 2009 wording on wait conditions (P). Do not put "Contracts for Concurrency" in quotation marks in either variant: it is the title of the SCOOP paper, and the ICST 2017 phrase was seen only in a summary.

**(f) Related work: systems readiness and allowed outcomes** (T4 §6.4):

> Platforms declare readiness and handle not-ready states as a mechanism: systemd's readiness notification and its separation of ordering from requirement dependencies; Kubernetes readiness probes; the Linux kernel's deferred probing ("resources that are not yet available"); ZeroMQ's "slow joiner"; and ROS 2's volatile durability, under which "no attempt is made to persist samples", so a late-joining subscription receives no earlier samples. cFE itself states, at the framework level, that a send with no subscriber "is not considered an error". Readiness specifications import these ideas into checking: an allowed not-ready outcome is not a bug, a loss is judged at the caller level only where a consumer's or mission's reference requires delivery, and the framework's own statement is reported beside that judgement.

The clause "so a late-joining subscription receives no earlier samples" is [I], not a quotation; "late-joining" appears in the *Transient local* bullet (`ros2_jazzy_About-Quality-of-Service-Settings.rst:49`).

---

## 6. What the AI paper changes in Kairos

### 6.1 Related-work positioning

Suggested edits from T1 §8; not applied, because the spec is read-only here.
- **§1.11, R2 row, "What prior work does".** Add: "AI (FSE 2014 [V]) learns per-instruction remote-predecessor sets from verified-correct runs; R2's order is in every run and hence in every learned set; stalling cannot repair it, because MustBefore makes the wait circular."
- **§1.11 "Established theory used".**
  - Add a row "Learned interleaving invariants; anticipation and stalling": AVIO, DefUse, PSet (bibliographic V per T1, T3; mechanism P) and AI [V].
  - List CCI separately, as "statistical interleaving-predicate isolation" (V per T1, T3; mechanism P).
  - Replace "Typestate contracts" with "Typestate" (§5.5).
  - Add Joshi & Sen ASE 2008 [V per T3] and Lifestate ECOOP 2019 [V] as close prior art. Neither was cited in §1.11 at revision 2 (T3 §5.4); spec revision 1.1 cites both as prior art only (§1.11, App. B).
- **§4.13.** Add one sentence citing AI as the tolerance-side precedent for thread stalling, with the two differences: lock-free gate placement, and reference-derived watchdogs with `timeout-exercised` (§6.4).
- **Framing** [I]. Kairos is complementary to tolerance work, not a competitor. Kairos verdicts say *where* AI-style tolerance would be:
  - useless: `fixed-order`, where the MustBefore certificate gives a circular wait;
  - unsafe: the LP is under a framework lock;
  - timing-sensitive: `timeout-dependent` instances (T1 §6.3).

### 6.2 An AI-style learned-invariant comparator

T1 §6.4 recommends this [S].
- **Placement.** Add it to §8.3 as a second variant in the "learned invariant" slot, next to the DefUse-style row, on the same training pool. In spec revision 1.1 terms it is an own-information re-implementation (kind K2), neither the original tool nor an ablation (spec §8.3).
- **Lifting to Kairos traces** (a reimplementation, labelled as such):
  - address → resource key `sel(e)`;
  - static instruction → context-qualified template;
  - thread → executing actor;
  - access → every Φ event on the key (variant **AI-all**, faithful to AI's read/write-agnostic predecessor), or lifecycle effects only (variant **AI-lc**, closer to DefUse's DSet and to PSet);
  - `nil` → no other-actor event on the key before the use.
- **Training.** `BSet(t)` is the union over N natural runs of one pool key that pass a **frozen output oracle**. Proposed oracle: reached OPERATIONAL, no ERROR-type event, normal exit. Record it as an assumption before F1.
- **Alarm.** A use with `RPre ∉ BSet(t)`.
- **Reporting.** Report alarms against N as a training-size curve, mirroring AI §4.5.
- **Golden tests.** Use AI's worked figures, which satisfies §8.3's rule that golden tests come from the original paper:
  - Fig. 4: `BSet(S2) = {S1, S3}`;
  - Fig. 7: `nil ∉ BSet`;
  - Fig. 10: HTTrack, `BSet(S4) = {S2}`;
  - also Figs. 3 and 6 (T1 §2).
- **What it adds beyond DefUse.**
  - It is the strongest member of the learned family on order violations: W-W order violations and use-before-any-access through `nil`.
  - Lifting gives it the calling context it lacks at memory level. If it still cannot flag R2, the cause is the *learning* premise, not the granularity. That separates the two inside this re-implementation; it is a K2 comparison, not an ablation of Kairos, and says nothing about what the original AI tool would report (spec §8.3, §8.4).
- **A deviance-ranking comparator** [S, optional]. §4.2 shows that a frequency-based learner is not covered by the structural argument. If the PI wants that flank covered, add a per-API return-code outlier ranking over the same pool as a third variant. Report it as a ranking (top-k), not as verdicts.
- **What may be written in advance.** Only the structural property of §4.2: R2 is in every learned set, under AI-all and under AI-lc. All other outcomes depend on the training orders, so no expected values are written down (§8.3 rule).
- **The original tool** is `NOT_RUN`. Blockers:
  - an LLVM 3.3-era pass pipeline;
  - source availability at `http://james0zan.github.io/AI.html` is **UNVERIFIED** (the egress proxy refused the host in revision 2);
  - per-access instrumentation of all of OSAL and cFE.

### 6.3 Turning point vs LP and capture point

From T1 §6.2 [I].

- **Correspondence.** For a **schedule-dependent** readiness violation, AI's turning point corresponds to the LP of the failing conjunct at the use. Operationally that is the execution of the access whose predecessor is unexpected. For a value conjunct, it is the **capture point**. This mirrors AI's observation that the decisive access can come well before the visible failure. In R2, the stamp is captured at argument evaluation and the failure appears only inside EVS.
- **Difference 1: evaluation vs intervention.**
  - AI uses one point for both.
  - Kairos's LPs often sit inside framework critical sections: the OSAL name search under the type lock, SB `route_lookup` under the SB lock, ES record lookup under the ES lock.
  - So Kairos keeps the **evaluation point** (LP or capture point) apart from the **intervention point**: the gate, placed before the lock acquisition, with an empty lockset (§4.13).
  - Cite AI for the observation that intervention must precede the decisive access, then explain why Kairos moves it earlier.
- **Difference 2: fixed-order violations have no turning point inside a run.** Their outcome is fixed by the basis B. The class `fixed-order` and its certificate say exactly that.
- **Difference 3: shapes.**
  - `ABSENT` ≈ `RPre = nil`.
  - `STALE` ≈ AI's atomicity case (an unexpected remote write intervenes).
  - `WRONG-WAY` and `UNORDERED` have no AI counterpart, since AI does not reason about happens-before.
- **Terminology.** Do not adopt "turning point" as a Kairos term. Its definition in AI is informal (prose and Figs. 1–2).

### 6.4 AI stalling vs `kz hold` / `kz gate`

From T1 §6.3.

| Aspect | AI stall | Kairos `kz hold` (F0) / `kz gate` (F1) |
|---|---|---|
| Purpose | **tolerance**: prevent an unseen order in production | **confirmation**: realize a predicted order on the real binary, as evidence |
| Placement | immediately before the guarded access, possibly inside a critical section [I] | empty program lockset; not in timebase callbacks, binsem internals or RESERVED windows |
| Release | polling re-check of the predecessor | event-based: the awaited event, or the other party blocks or exits |
| Cap | fixed threshold; the value is not stated in the paper | watchdog = the smallest *reference* timeout the hold can make expire (e.g. 30 s core startup; CF's 25 × 100 ms); result `inconclusive(await-timeout)` |
| Timeout semantics | "a correctly synchronized program will not depend on time delays" (AI §4.5, p. 339, text seen) | timeouts are part of the semantics: σ_U vs σ_U∞, `timeout-dependent`, `timeout-masked-liveness`; a hold during which a program timeout fires is labelled `timeout-exercised` |

**AI's harmlessness argument does not hold for cFS** [I]. cFS correctness depends on time delays by design:
- soft startup barriers that time out and only log;
- a core sync that panics after 30 s;
- CF's retry bound.

An AI-style stall can therefore turn a good run into a bad one. Kairos's `timeout-exercised` label and its σ_U/σ_U∞ refinements formalize the effect that AI's argument assumes away. This gives Kairos a citable motivation for these design choices.

### 6.5 Vocabulary consequences of the AI paper

1. **Never call κ an invariant.** The reasons, in order:
   - **κ is a specification object, not a predicate.** It bundles the required state with the accepted not-ready outcomes, `appreq`, harm, provenance and scope, and even `Req` need not hold at every use (`TOLERATED`). AI's own definition of the bare word is the specified one: "Program invariants are predicates that should always be true at certain points of the execution; they reflect programmers' intentions" (p. 331, text seen). So the word is wrong for κ before any learned sense enters; an invariant can be written or proved as well as learned.
   - **Secondary, reader-side reason [I]: the named interleaving invariants are learned from runs judged correct.** AI marks the learned sense as "likely invariants" learned "from testing runs", which "are supposed to be held in all the correct runs" (pp. 331–332, text seen). AVIO, DefUse, PSet and AI itself are of this kind. "Invariant" next to them would read as learned.
2. **"Tolerating" is taken.**
   - **Who uses it.** AI ("A Lightweight System for Tolerating Concurrency Bugs") and Yu & Narayanasamy, "Tolerating Concurrency Bugs Using Transactions as Lifeguards" (MICRO 2010, AI ref [36]; P, seen only in AI's bibliography), use "tolerating concurrency bugs" for a run-time system that avoids the bug. AI classifies Atom-Aid and ConAir as "bug-tolerating tools" (Fig. 1, p. 330, text seen). Their own titles say "Detecting and surviving atomicity violations" and "Featherweight concurrency bug recovery via single-threaded idempotent execution" (AI refs [17], [37]; titles P, seen only in AI's bibliography, except ConAir, which is V per T1).
   - **What Kairos means.** Kairos's `TOLERATED` / `tolerated` means that the program's own documented handling makes a not-ready outcome acceptable.
   - **The risk.** The paper's tables would show `TOLERATED` and `tolerated` right next to a citation of AI's "tolerating".
   - **Recommendation** [S]. The paper renames them to **ALLOWED_NOT_READY** / `allowed`, and the spec keeps its names. In prose, write "an allowed not-ready outcome". Never write "Kairos tolerates".
3. **"Intention."** AI's prose ("programmers' intentions about the correct order", p. 331, text seen) gives a useful bridge sentence: Kairos does not infer the intention from runs; it records where the intention is written down.

---

## 7. Open questions for the PI

1. **Rename or keep (the main decision; §5.2).**
   - Adopt "readiness specification" (§5.1), or keep "readiness contract" with the two-sense definition of §5.7(e2) and the conditions of §5.2?
   - The trade-off: "specification" avoids DbC blame and A/G composition baggage and asserts no agreement for unreferenced entries. "Contract" is shorter, has no cFS collision, and fits R2 (DbC-like) and C1 (A/G-like). Neither fits rule-generated entries without the source tier.
   - If renaming, apply it to the paper only, or also to SPEC_FINAL, the Architecture documents, the diagrams and the Φ DSL keyword?
2. **When to apply.** Now, or at the next spec revision? (Revision 2 asked this while AR3, the Architecture documentation, was in progress.) The edit is mechanical. In revision 2 it touched 79 spec lines, about 210 Korean lines and 5 diagrams (§5.5); these counts are from commit b4ac55f, before spec revision 1.1, and must be recounted before any renaming.
3. **First-use qualifier.** "Provenance-tracked" (근거 기록), "authored", or another word? "Authored" overclaims for `code_pattern` entries generated by rule. "Reference-derived" overclaims for `researcher`, `code_pattern` and `unreferenced` entries, so it is reserved for the reference tiers.
4. **Source tier.**
   - Adopt the derived `tier` field and the per-tier reporting rule (§5.4)?
   - Confirm the order used for mixed provenance: contract-backed > requirement-derived > admitted > issue-derived > code-derived > researcher > unreferenced. Or choose another.
   - Should headline numbers in the paper be given per tier?
5. **cFS requirements as references (a spec decision).**
   - **The new kind.** Add a `requirement` provenance kind (requirement ID plus text hash), or state in §1.6 why requirement documents are excluded. A NASA reader will expect "reference" to include `cFE_FunctionalRequirements.csv` and each app's `*_FunctionalRequirements.csv` first. *Spec revision 1.1 (proposal, PI item QT-2):* the kind is added.
   - **The SB case.** Does cSB4305.5 ("not considered an error", with initialization named in the rationale) make the SB no-subscriber case `documented_drop` rather than `unspecified_by_reference`? It is supplier-side documentation. Whether a given subscriber needed that message is still undecided by any reference. *Spec revision 1.1 (proposal, QT-2, re-asking STATUS decision T2):* no; cSB4305.5 is supplier-side and gives the framework-level judgement `documented-not-error`, and the caller level stays `unspecified_by_reference` (LOSS-UNDER-ORDER) unless a consumer or mission reference decides (spec §1.6, §3.9).
   - **The capacity entries.** cSB4305.3 and cSB4305.4 specify the supplier's handling of pipe overflow and the MsgId limit. Their rationale reads "The ground needs to know if a message cannot be routed to its destination", which sounds like an anomaly, not a normal outcome. Should they change K-CAP's `Allowed`? *Spec revision 1.1:* no; they are supplier-side, `fw_doc` = {`documented-result`} (spec §3.9).
   - **A side effect.** If adopted, the `requirement` kind makes the "readiness requirement" collision real (§3), which further supports rejecting that candidate.
6. **Split `Allowed`.** Split it into *documented outcomes* (supplier side) and *client handling* (retry, defer, fallback). In the Φ schema (a spec change), or only in the paper's prose? *Spec revision 1.1 (proposal, PI item QT-3):* in the Φ schema, as a side on each provenance record plus the field `appreq`, not as two Allowed fields (spec §1.6, ADR-043).
7. **Kinds that are not typestate.**
   - Keep K-CAP, K-ATOM and K-HANDOFF in the readiness catalogue under their own names ("capacity property", "atomicity specification")?
   - Move K-LOCKBAL, a supplier postcondition, to an implementation-defect family (T2 §9.2 item 5)? This is now PI item QT-9, which also covers K-SPAWN.osal and offers a researcher-authored `appreq` as the other option (spec §1.6 `harm`, §10.1).
8. **Names.**
   - Re-gloss RC1–RC15 as "readiness categories".
   - In the paper, rename `TOLERATED` / `tolerated` to ALLOWED_NOT_READY / `allowed` (§6.5)? The spec would keep its names.
9. **Korean terms.**
   - 준비 상태 명세, always written in full? Or a head noun not used for the document, such as 준비 조건?
   - 근거 기록 or 근거 추적 for "provenance-tracked"?
   - 허용된 미준비 결과 or 허용된 not-ready 결과 (the documents keep many English technical words)?
   - Check against KIISE usage or the lab's convention; all Korean renderings are UNVERIFIED.
10. **AI-style comparator.** Approve adding it to §8.3 before the F1 freeze (§6.2), including the frozen output oracle and, optionally, the deviance-ranking variant. Run the original tool, or record it as `NOT_RUN` (the default)?
11. **Venue framing.**
    - For ASPLOS/FSE: lead with "order violation" and "initialization race", and present "readiness violation" as the subclass (T4 §6.2). Expect the question whether a deterministic initialization-order bug is a concurrency bug in Lu's sense, and answer it with §5.7(a)'s fixed-order sentence.
    - For PLDI: lead with typestate.
    - Which venue should the terminology optimize for?
12. **Full texts to read before quoting, and a bibliographic follow-up with a fresh search budget** (T1 §10, T2 §2, T3 §7, T4 §9; header list):
    - Meyer 1992 (only a summary was seen; needed before claiming DbC blame semantics for an API that returns error codes);
    - Lu et al. 2008's definition of order violation (wording UNVERIFIED; "intended order" was not found);
    - Dwyer et al.'s precedence pattern and its scope semantics (P);
    - Nienaltowski et al. 2009, on wait conditions (P) and on author order (Springer page for doi:10.1007/s00165-007-0063-2);
    - Joshi & Sen 2008 and Lifestate 2019, before claiming differences; GPredict ICSE 2015, its Introduction ("a more generic contract", §2.4) and its §II.B property formalism (spec App. B; P-PAPERS);
    - InitRacer's definitions of its error types (P);
    - the AI TSE 2016 extension, for changes to the stall threshold, training or limitations (not read);
    - CCI (mechanism P);
    - arXiv:1505.02951 and the ICST 2017 record (authors, initials, order; the "contracts for concurrency" phrase);
    - the Perracotta UVA or ACM page (initials, pages);
    - the Monperrus et al. EMSE abstract;
    - eiffel.org "ET: Design by Contract", eiffel.org "Design by Contract in SCOOP", and a JML reference-manual page.

---

## 8. References and status

This table lists every reference the note uses. "V per Tn (not re-verified in the citation or adversarial pass)" means: V as reported by sibling report Tn. No web check was possible in either review pass, in revision 2 (header) or in revision 3. "Corroborated by AI ref [n]" means that AI's printed bibliography (p. 340, text seen) gives the same authors, title, venue and year.

| Reference | Status | What was seen | Report |
|---|---|---|---|
| M. Zhang, Y. Wu, S. Lu, S. Qi, J. Ren, W. Zheng. AI: A Lightweight System for Tolerating Concurrency Bugs. FSE 2014, pp. 330–340, doi:10.1145/2635868.2635885 | **V** (primary PDF front matter: "FSE '14, November 16–21/22, 2014, Hong Kong", ACM 978-1-4503-3056-5/14/11, DOI, printed pages; also re-confirmed by the synthesis search, [MADSys page](https://madsys.cs.tsinghua.edu.cn/publication/ai-a-lightweight-system-for-tolerating-concurrency-bugs/)) | text (uploaded PDF; every quote verified against the extraction) | T1–T4 |
| Same authors. A Lightweight System for Detecting and Tolerating Concurrency Bugs. IEEE TSE 42(10):899–917, 2016, doi:10.1109/TSE.2016.2531666 | **V** bibliographic (re-confirmed by the synthesis search, [MADSys page](https://madsys.cs.tsinghua.edu.cn/publication/a-lightweight-system-for-detecting-and-tolerating-concurrency-bugs/); not re-verified in the citation or adversarial pass) | content not read | T1, T2 |
| B. Meyer. Applying "Design by Contract". IEEE Computer 25(10):40–51, 1992 | **V per T1, T2, T3** bibliographic (not re-verified in the citation or adversarial pass) | summary only; text not seen | T1, T2, T3 |
| Eiffel documentation: DbC obligations and blame ("ET: Design by Contract"); "Design by Contract in SCOOP" | **P-doc** (snippet; page not opened; eiffel.org or Wikipedia, T2 l.116, l.160; exact URLs not recorded) | snippet | T2 |
| G. T. Leavens, A. L. Baker, C. Ruby. Preliminary Design of JML: A Behavioral Interface Specification Language for Java. ACM SIGSOFT SEN 31(3):1–38, 2006 | **V per T2** (not re-verified in the citation or adversarial pass); `normal_behavior`/`exceptional_behavior` wording **P-doc** (manual or tutorial snippets) | summary | T2 |
| P. Nienaltowski et al. Contracts for Concurrency. Formal Aspects of Computing 21(4):305–318, 2009, doi:10.1007/s00165-007-0063-2 | title, venue, pages, DOI **V per T2** (not re-verified in the citation or adversarial pass); **author order P**: T2 gives Nienaltowski, Ostroff, Meyer (per Meyer's publication list), the adversarial reviewer recalls Nienaltowski, Meyer, Ostroff (DBLP/Springer); wait-condition wording for this paper **P** | abstract snippet | T2 |
| C. B. Jones. Tentative Steps Toward a Development Method for Interfering Programs. ACM TOPLAS 5(4):596–619, 1983 | **V per T2** (not re-verified in the citation or adversarial pass); definitions **P** | secondary snippets | T2 |
| A. Benveniste et al. Contracts for System Design. FnT EDA 12(2–3):124–400, 2018 | **V per T2** (not re-verified in the citation or adversarial pass); definitions **P** | abstract, table of contents | T2 |
| D. Cofer, A. Gacek, S. Miller, M. Whalen, B. LaValley, L. Sha. Compositional Verification of Architectural Models. NFM 2012, LNCS 7226:126–140 | **V per T2** (not re-verified in the citation or adversarial pass) | snippet | T2 |
| H. Hüttel et al. Foundations of Session Types and Behavioural Contracts. ACM CSUR 49(1), art. 3, 2016 | **V per T3** (not re-verified in the citation or adversarial pass) | abstract | T3 |
| D. G. Sousa, R. J. Dias, C. Ferreira, J. M. Lourenço. Preventing Atomicity Violations with Contracts. arXiv:1505.02951, 2015 | surnames, title, id **V per T1** (not re-verified in the citation or adversarial pass); **initials P** (reviewer's recollection; T1 and T2 give surnames only) | search summary of the abstract | T1 |
| R. J. Dias, C. Ferreira, J. Fiedor, J. M. Lourenço, A. Smrčka, D. G. Sousa, T. Vojnar. Verifying Concurrent Programs Using Contracts. ICST 2017, pp. 196–206, doi:10.1109/ICST.2017.25 | title, venue, pages, DOI **V per T1, T2** (not re-verified in the citation or adversarial pass); **author order and initials P** (records differ; T1, T2); the phrase "contracts for concurrency" **P** (summary of the abstract) | summary | T1, T2 |
| M. Monperrus, M. Eichberg, E. Tekes, M. Mezini. What Should Developers Be Aware Of? An Empirical Study on the Directives of API Documentation. EMSE 17(6):703–737, 2012, doi:10.1007/s10664-011-9186-4 | **V per T2, T4** bibliographic (not re-verified in the citation or adversarial pass); definition wording **P** (snippets; T2 and T4 differ) | abstract snippet | T2, T4 |
| N. Polikarpova, I. Ciupa, B. Meyer. A Comparative Study of Programmer-Written and Automatically Inferred Contracts. ISSTA 2009, pp. 93–104 | **V per T2** (not re-verified in the citation or adversarial pass; the synthesis re-check was refused by the budget) | abstract summary | T2 |
| Y. Wei, C. A. Furia, N. Kazmin, B. Meyer. Inferring Better Contracts. ICSE 2011, pp. 191–200 | **V per T2** (not re-verified in the citation or adversarial pass) | summary | T2 |
| P. Cousot, R. Cousot, M. Fähndrich, F. Logozzo. Automatic Inference of Necessary Preconditions. VMCAI 2013, LNCS 7737:128–148 | **V per T2, T3** (not re-verified in the citation or adversarial pass) | summary, snippet | T2, T3 |
| C. A. R. Hoare. An Axiomatic Basis for Computer Programming. CACM 12(10):576–580, 583, 1969 | **V per T2, T3** (not re-verified in the citation or adversarial pass; page range varies) | snippet | T2, T3 |
| R. E. Strom, S. Yemini. Typestate: A Programming Language Concept for Enhancing Software Reliability. IEEE TSE 12(1):157–171, 1986, doi:10.1109/TSE.1986.6312929 | **V** (re-confirmed by the synthesis search: DBLP index; some IBM pages carry wrong metadata; not re-verified in the citation or adversarial pass) | abstract summary | T2, T3, T4 |
| R. DeLine, M. Fähndrich. Typestates for Objects. ECOOP 2004, LNCS 3086:465–490 | **V per T2, T3** (not re-verified in the citation or adversarial pass) | summary | T2, T3 |
| R. DeLine, M. Fähndrich. Enforcing High-Level Protocols in Low-Level Software (Vault). PLDI 2001, pp. 59–69 | **V per T3** (not re-verified in the citation or adversarial pass) | abstract | T3 |
| P. Joshi, K. Sen. Predictive Typestate Checking of Multithreaded Java Programs. ASE 2008, pp. 288–296 | **V per T3** (IEEE Xplore 4639332, ACM DL; the synthesis re-search did **not** return the record; not re-verified in the citation or adversarial pass) | abstract | T3 |
| N. E. Beckman, K. Bierhoff, J. Aldrich. Verifying Correct Usage of Atomic Blocks and Typestate. OOPSLA 2008, pp. 227–244 | **V per T3** (not re-verified in the citation or adversarial pass) | abstract | T3 |
| N. E. Beckman, D. Kim, J. Aldrich. An Empirical Study of Object Protocols in the Wild. ECOOP 2011, pp. 2–26 | **V per T2** #30 (not re-verified in the citation or adversarial pass) | summary | T2 |
| N. E. Beckman, A. V. Nori. Probabilistic, Modular and Scalable Inference of Typestate Specifications. PLDI 2011, pp. 211–221 | **V per T3** (not re-verified in the citation or adversarial pass) | abstract | T3 |
| S. Meier, S. Mover, B.-Y. E. Chang. Lifestate: Event-Driven Protocols and Callback Control Flow. ECOOP 2019, LIPIcs 134, 1:1–1:29, doi:10.4230/LIPIcs.ECOOP.2019.1 | **V** (re-confirmed by the synthesis search: [CU PLV page](https://plv.colorado.edu/papers/verivita-ecoop19.html), [Dagstuhl](https://drops.dagstuhl.de/entities/document/10.4230/LIPIcs.ECOOP.2019.1); not re-verified in the citation or adversarial pass) | summary | T2, T3, T4 |
| G. Ammons, R. Bodík, J. R. Larus. Mining Specifications. POPL 2002, pp. 4–16 | **V per T1, T2, T3** (not re-verified in the citation or adversarial pass) | summary | T1, T2, T3 |
| J. Yang, D. Evans, D. Bhardwaj, T. Bhat, M. Das. Perracotta: Mining Temporal API Rules from Imperfect Traces. ICSE 2006 | surnames, title, venue, year **V per T3** #26 (UVA page; not re-verified in the citation or adversarial pass); **initials P** (T3 gives surnames only; "J." is the reviewer's recollection; the earlier "S." had no source); **pages (282–291) P** | abstract | T3 |
| M. K. Ramanathan, A. Grama, S. Jagannathan. Path-Sensitive Inference of Function Precedence Protocols. ICSE 2007, pp. 240–250 | **V per T3** (not re-verified in the citation or adversarial pass) | abstract | T3 |
| M. D. Ernst, J. Cockrell, W. G. Griswold, D. Notkin. Dynamically Discovering Likely Program Invariants to Support Program Evolution. IEEE TSE 27(2):99–123, 2001 | **V per T2, T3** (not re-verified in the citation or adversarial pass) | summary | T2, T3 |
| M. D. Ernst, J. H. Perkins, P. J. Guo, S. McCamant, C. Pacheco, M. S. Tschantz, C. Xiao. The Daikon System for Dynamic Detection of Likely Invariants. Sci. Comput. Program. 69(1–3):35–45, 2007; Daikon manual | **V per T1, T3** (not re-verified in the citation or adversarial pass); corroborated by AI ref [5]; manual wording **P** (snippet) | snippet | T1, T3 |
| D. Engler, D. Y. Chen, S. Hallem, A. Chou, B. Chelf. Bugs as Deviant Behavior: A General Approach to Inferring Errors in Systems Code. SOSP 2001 | **V per T3** #31 (not re-verified in the citation or adversarial pass) | summary | T3, T4 |
| S. Lu, S. Park, E. Seo, Y. Zhou. Learning from Mistakes: A Comprehensive Study on Real World Concurrency Bug Characteristics. ASPLOS 2008 | **V per T2–T4** bibliographic (not re-verified in the citation or adversarial pass); corroborated by AI ref [12]; "order intentions" **V-abs** per T4; "desired order" **P**; formal definition wording **UNVERIFIED** | abstract | T2, T3, T4 |
| S. Lu, J. Tucek, F. Qin, Y. Zhou. AVIO: Detecting Atomicity Violations via Access Interleaving Invariants. ASPLOS 2006, pp. 37–48 | **V per T1, T3, T4** (not re-verified in the citation or adversarial pass); corroborated by AI ref [13] | abstract; definition as quoted in AI (text) | T1, T3, T4 |
| Y. Shi, S. Park, Z. Yin, S. Lu, Y. Zhou, W. Chen, W. Zheng. Do I Use the Wrong Definition?: DeFuse: Definition-Use Invariants for Detecting Concurrency and Sequential Bugs. OOPSLA 2010, pp. 160–174. ("DefUse" is the common name of the tool) | **V per T1, T3, T4** (not re-verified in the citation or adversarial pass); corroborated by AI ref [28] | abstract; description in AI (text) | T1, T3, T4 |
| J. Yu, S. Narayanasamy. A Case for an Interleaving Constrained Shared-Memory Multi-Processor (PSet). ISCA 2009, pp. 325–336, doi:10.1145/1555754.1555796 | **V per T1, T3** bibliographic (via DBLP; not re-verified in the citation or adversarial pass); corroborated by AI ref [35]; mechanism **P** | mechanism known only from AI's description | T1, T3, T4 |
| J. Yu, S. Narayanasamy. Tolerating Concurrency Bugs Using Transactions as Lifeguards. MICRO 43, 2010 | **P** (seen only in AI's bibliography, ref [36]; pages not seen) | title only | — |
| G. Jin, A. Thakur, B. Liblit, S. Lu. Instrumentation and Sampling Strategies for Cooperative Concurrency Bug Isolation (CCI). OOPSLA 2010, pp. 241–255, doi:10.1145/1869459.1869481 | **V per T1, T3** (not re-verified in the citation or adversarial pass; T4 lists P from AI only); corroborated by AI ref [7]; mechanism (statistical isolation over failing and successful runs) **P** | abstract | T1, T3, T4 |
| B. Lucia, J. Devietti, K. Strauss, L. Ceze. Atom-Aid: Detecting and Surviving Atomicity Violations. ISCA 2008 | **P** (seen only in AI's bibliography, ref [17]; pages not seen) | title only | — |
| W. Zhang, M. de Kruijf, A. Li, S. Lu, K. Sankaralingam. ConAir: Featherweight Concurrency Bug Recovery via Single-Threaded Idempotent Execution. ASPLOS 2013, pp. 113–126, doi:10.1145/2451116.2451129 | **V per T1** (UW BibTeX via search; not re-verified in the citation or adversarial pass); title corroborated by AI ref [37] | title, BibTeX | T1 |
| C. Q. Adamsen, A. Møller, F. Tip. Practical Initialization Race Detection for JavaScript Web Applications (InitRacer). OOPSLA 2017, PACMPL 1, art. 66 | **V per T3, T4** (not re-verified in the citation or adversarial pass); definitions of the error types **P** | abstract | T3, T4 |
| J. Huang, Q. Luo, G. Roşu. GPredict: Generic Predictive Concurrency Analysis. ICSE 2015, pp. 847–857 | **V per T4** (not re-verified in the citation or adversarial pass); the Introduction's "a more generic contract" for the Java Iterator usage rule, and the §II.B property formalism: **U** (the external review's reading and a search snippet; PDF not opened; P-PAPERS) | summary; snippet | T4 |
| M. B. Dwyer, G. S. Avrunin, J. C. Corbett. Patterns in Property Specifications for Finite-State Verification. ICSE 1999, pp. 411–420 | **V per T3** bibliographic (not re-verified in the citation or adversarial pass); pattern wording **P** | snippet | T3 |
| L. Lamport. Proving the Correctness of Multiprocess Programs. IEEE TSE SE-3(2):125–143, 1977 | **V per T3** (not re-verified in the citation or adversarial pass); quote **P** | abstract | T3 |
| B. Alpern, F. B. Schneider. Defining Liveness. IPL 21(4):181–185, 1985 | **V per T3** (not re-verified in the citation or adversarial pass) | abstract | T3 |
| T. Ball, S. K. Rajamani. The SLAM Project: Debugging System Software via Static Analysis. POPL 2002, pp. 1–3 | **V per T3** #33 (not re-verified in the citation or adversarial pass); "temporal safety properties" wording **P** | abstract | T3 |
| F. Chen, G. Roşu. MOP: An Efficient and Generic Runtime Verification Framework. OOPSLA 2007, pp. 569–588 (implemented as JavaMOP) | **V per T3** #39 (not re-verified in the citation or adversarial pass; DOI not confirmed) | abstract | T3 |
| F. Chen, G. Roşu. Parametric Trace Slicing and Monitoring. TACAS 2009 | **V per T3** (not re-verified in the citation or adversarial pass) | abstract | T3 |
| T. Ball et al. Thorough Static Analysis of Device Drivers (SDV). EuroSys 2006, pp. 73–85 | **V per T2** (not re-verified in the citation or adversarial pass) | abstract snippet | T2 |
| J. Ferrante, K. J. Ottenstein, J. D. Warren. The Program Dependence Graph and Its Use in Optimization. TOPLAS 9(3):319–349, 1987 | **V per T3** (not re-verified in the citation or adversarial pass) | abstract | T3 |
| NASA FRET / Ogma / Copilot cFS monitor generation (NTRS, Hackage) | **P per T2** #42 | snippets | T2 |
| systemd `systemd.unit(5)` (l.855), `sd_notify(3)` (l.582), `systemd.service(5)` (l.1943) (`main`) | **V-doc** (T4 fetched from GitHub; quotes re-checked on disk in the citation pass) | text | T3, T4 |
| Kubernetes docs: Pod Lifecycle (`kubernetes/website`, `main`), l.827 | **V-doc** | text | T4 |
| Linux `Documentation/driver-api/driver-model/driver.rst` (l.172), `device_link.rst` (`master`) | **V-doc** | text | T4 |
| ZeroMQ Guide, ch. 1 (l.215) and ch. 5 (`booksbyus/zguide`, `master`) | **V-doc** | text | T4 |
| ROS 2 docs: About Quality of Service Settings (`jazzy`), l.49–50 | **V-doc** (both bullets re-checked in revision 2) | text | T4 |
| Linux man-pages `epoll(7)`, `select(2)` | **V-doc** | text | T4 |
| nasa/cFE #2663; nasa/CF #184 (issue bodies) | **V per T3** (text via the GitHub connector in T3's session); **not re-checkable** in the review passes or revision 2 (GitHub access to nasa repositories not enabled); URLs in `cases/R2.md:3`, `cases/C1.yaml:8` | text (T3) | T3 |
| cFS v7.0.1 sources: `cfe_evs.h:155, 169–172, 198–199, 208, 261`; `cfe_evs.c:185–194`; `cfe_error.h:252–267`; `osapi-countsem.h:160, 166`; `cf_cfdp.c:1272–1278`; `cf_internal_cfg.h:173, 185`; `cfe_sb.h:401`; `cfe_sb_init.c:50`; `cfe_sb_task.c:138`; `cfe_es_start.c:835`; `to_lab_app.c:64–66`; `cf_cfdp_pdu.h:26`; `core_api/ut-stubs/src/cfe_config_handlers.c:63`; `tbl/ut-coverage/tbl_ut_helpers.c:358–359`; the 138 `\par Assumptions` blocks in `cfe/modules/core_api/fsw/inc` | local, read | text (re-checked in revision 2) | T2, T3 |
| cFE functional requirements, `cfe/docs/cFE_FunctionalRequirements.csv` (570 lines): cES1335 (`:147`), cEVS3103.1 (`:269`), cSB4305.3–.5 (`:350–352`) | local, read | text (revision 2) | — |
| AI tool source, `http://james0zan.github.io/AI.html` | **UNVERIFIED** (availability; the egress proxy refused the host in revision 2) | stated in the paper only | T1 |
| Korean renderings (준비 상태 명세, 근거 기록, 준비 사전조건, 허용된 미준비 결과, 근거 등급); 계약에 의한 설계 for DbC | **UNVERIFIED** | no Korean terminology source checked | T2, T3 |
| External review EXT-20261012 of commit b4ac55f, `history/review_EXT_20261012.md` (§6.1–§6.4; its sources P1–P3) | internal review document; it reports reading GPredict's Introduction and §II.B, the abstract and introduction of Joshi & Sen, and Lifestate's official abstract | the review's text; the papers themselves were not seen in revision 3 | — |

---

## Review notes

Issues 1–32 from the citation and adversarial passes on revision 1. All were addressed; issue 1, 2, 3, 13, 14 and 32 only as far as the missing web access allowed. Notes 33–37 record revision 3 (EXT-20261012 §6.1–§6.4, as spec revision 1.1 answers them; the review's §6.2 and §6.3 are P1 items; note 37 is a consistency pass on revision 3).

| # | Sev. | Handling in revision 2 |
|---|---|---|
| 1 | major | **Fixed, as far as possible.** The header now records all three passes and states that neither review pass, nor this revision, had a search budget. Every T1–T4-based V in §8 reads "V per Tn (not re-verified in the citation or adversarial pass)". AI FSE 2014 stays V (primary PDF). The eight entries confirmed by AI's reference list carry "corroborated by AI ref [n]". The follow-up list is in the header and §7 Q12. **Not done:** fresh web verification. WebSearch was refused at the first query, and arxiv.org, Springer, ACM, eiffel.org, UVA, OpenAlex and Semantic Scholar were refused by the egress proxy (403). The GitHub connector does not cover nasa/*, and Scholar Gateway denied access |
| 2 | major | **Fixed with P.** Perracotta now reads "J. Yang, D. Evans, D. Bhardwaj, T. Bhat, M. Das". The initials and pages (282–291) are P, because the UVA and ACM pages could not be opened |
| 3 | major | **Fixed with P.** Now "D. G. Sousa, R. J. Dias, C. Ferreira, J. M. Lourenço", with the initials P, in §2.1 and §8 |
| 4 | major | **Fixed.** §2.1 renames the row "Atomicity contracts" and records where the phrase was seen (an ICST 2017 summary, P) and that it is also the SCOOP paper's title. §5.7(e1) now cites "[Sousa et al. 2015; ICST 2017]" without quotation marks, and (e) warns against quoting the phrase. §2.4 attributes the two meanings separately |
| 5 | major | **Fixed.** The DbC, SCOOP and JML wording is re-tagged **P-doc** (snippet; page not opened; eiffel.org or Wikipedia) in §2.1, §5.3 and §8, and P-doc is added to the tag key. "Proceed only after C holds" lost its quotation marks and is marked [I] as T2's paraphrase. §2.1 says which pages to open, and that their exact URLs must be cited |
| 6 | minor | **Fixed.** CCI is separated from the correct-run learners in §1.2 item 1, §4.1, §4.2, §5.7(c) and §6.1, with the mechanism P. §4.2 gives CCI's own reason for silence on R2: no failing boot to contrast |
| 7 | minor | **Fixed.** §5.7(b) now cites "[Ernst et al. 2001]" for likely invariants inferred from observed executions and "[Lu et al. 2006; Zhang et al. 2014]" for concurrency-bug invariants. §4.1 notes that Daikon does not judge runs correct |
| 8 | minor | **Fixed.** The Monperrus wording is P, with both snippet versions shown and quoted in lower case inside a sentence, and the DOI is given for checking. The bibliographic V is kept, per T2 and T4 |
| 9 | minor | **Fixed.** SLAM, object protocols, FRET/Ogma, MOP/JavaMOP, Atom-Aid and ConAir were added to §8, along with Yu & Narayanasamy MICRO 2010 and the cFE requirements file. Atom-Aid is P (AI's bibliography only). ConAir is V per T1, which had a BibTeX record, plus AI ref [37]; this differs from issue 28's "P" for that one title |
| 10 | minor | **Fixed.** §3 and §5.3 now say "parametric properties and monitoring (MOP/JavaMOP: Chen & Roşu, OOPSLA 2007; TACAS 2009)", without quotation marks |
| 11 | minor | **Fixed.** §4.4 quotes the two ROS 2 bullets separately, with the file line numbers, and marks the late-joiner consequence [I]. §5.7(f) was changed the same way |
| 12 | minor | **Fixed.** §2.1 names the file as a unit-test stub (`core_api/ut-stubs/src/cfe_config_handlers.c:63`) |
| 13 | minor | **Fixed with P.** (1) The ICST 2017 entry now lists the seven authors, with order and initials P. (2) The full Engler title is given. (3) The title reads "DeFuse", with "DefUse" noted as the common name |
| 14 | minor | **Fixed.** Nienaltowski et al. now have author order P, with both orders and their sources stated, and the Springer DOI is listed as the check. §2.1 writes "Nienaltowski et al." |
| 15 | major | **Fixed.** §5.1 (i) and the KO version now read "lifecycle (typestate) or value predicates … at its own linearization point or capture point". The provider moved to a separate sentence that excludes actor and order from κ. §5.3 `Req` says that "typestate" covers only the lifecycle conjuncts and names K-CAP, K-ATOM and K-HANDOFF as non-typestate members of the catalogue. §2.4, §5.7(a) and §5.7(d) no longer say "established by another actor, the provider" |
| 16 | major | **Fixed.** §5.1 adds (iv) harm and the Oracle clause, and the KO version matches. §5.3 gains `harm` and `Oracle` rows, and §2.2 treats harm as part of κ's judgement |
| 17 | major | **Fixed.** The §5.1 violation clause and its KO version separate violations from not-ready-unjudged and LOSS-UNDER-ORDER occurrences, citing `SPEC_FINAL.md:289–290, 430`. §5.7(a) now says "an outcome its references do not allow" and adds the unjudged sentence |
| 18 | major | **Fixed.** The first-use qualifier is now "provenance-tracked" (근거 기록), which holds for every entry. "Reference-derived" is reserved for the reference tiers (§5.4). The definition carries "authored, or generated by a fixed rule … never trained on executions … oracle refinement". §4.5's column is now "Provenance-tracked specification (source tier)". "Authored" was not chosen because it overclaims for `code_pattern` entries generated by rule; the choice is left open in §7 Q3 |
| 19 | major | **Fixed.** "Textbook DbC" was dropped. R2 is now described as "a documented caller assumption violated in every boot, whose documented error return is discarded by every caller" (`cfe_evs.h:198–199`), and C1 as "no caller obligation is stated; the not-found result is documented as normal". `contract-backed` is redefined as the issue proposed (§5.4). §2.1 corrects the heading and gives the 57/138 count and a supplier-behaviour example. §1.2 item 3 applies the C1 question to R2. One addition: cEVS3103.1 covers the NOT_REGISTERED branch, while R2's zero AppId takes the ILLEGAL_APP_ID branch (`cfe_evs.c:185–189`), which only the `\retval` documents. The Meyer caveat is in §2.3 and §7 Q12 |
| 20 | major | **Fixed.** C1's §4.5 cell uses the proposed wording, and §2.3 and §2.4 add the A/G reading. §5.2 is rewritten as a trade-off for the PI, with "contract" listed as a genuine alternative. §5.7(e2) adopts the A/G reading for admitted entries if "contract" is kept. Not adopted: "neither noun fits unreferenced entries". With the "provenance-tracked" qualifier, "specification" asserts no agreement for those entries, while "contract" still does. That reasoning is stated in §5.2 as [I] |
| 21 | major | **Fixed.** §4.2 restricts the structural claim to union-closed learners and adds the deviance-ranking caveat, including spec §7.1 source 9. §0, §5.7(a) and §5.7(c) follow, and §6.2 offers an optional deviance-ranking comparator |
| 22 | major | **Fixed as a proposal.** The `requirement` kind was added to the definition (marked as a proposal), §5.4 (tier `requirement-derived`), §5.5 and §7 Q5. The SB case was restated with cSB4305.5, including its rationale, which says the condition "is not considered an error", a point the issue did not quote. The `documented_drop` question went to §7 Q5, together with cSB4305.3/.4 for K-CAP. Correction to the issue: cSB4305.5 is at `cFE_FunctionalRequirements.csv:352`; `:269` is cEVS3103.1 |
| 23 | minor | **Fixed.** Recounted: "contract" appears only in licence text plus 2 unit-test comments. "Specification" has 227 lines (221 "Specification for"), with 16 lines in CF; "protocol" has 56 (18 in CF); "property" has 14. §3 adds the pro for "contract" and the new cons for "specification" (the CFDP specification, `Spec` at `cfe_evs.h:155, 208, 261`). "C1 lives in CF" is no longer a reason against "protocol" |
| 24 | minor | **Fixed.** §5.1 requires 준비 상태 명세 in full and offers 준비 조건 as a PI choice (§7 Q9). Every Korean target in §5.5 B now uses 준비 상태 명세. The counts were verified: "spec" appears 1,714 times, 1,204 of them as "spec §x", and 01_overview.md has "spec §1.11" ×13 and "spec §6.4" ×11 |
| 25 | minor | **Fixed in revision 2; superseded by note 34**, which withdraws "0/52" and restates the census's scope (§2.4). The census is removed from the §0 headline. In revision 2, §2.4 and §3 reported "contract 0/52, specification ≈1/52, property ≈1/52" and used the census only for the weaker claim |
| 26 | minor | **Fixed.** §5.2 names the tie-breaker: κ is an authored text with provenance, author, time and guard, so "specification" names κ and "property" names what an instance denotes. It also states that the Daikon argument is symmetric and that both nouns need the qualifier |
| 27 | minor | **Fixed.** §6.5 item 1, §3 and §5.7(b) now give "κ is not always true at the use" first, with AI p. 331 quoted accurately, including "likely invariants" |
| 28 | minor | **Fixed.** §6.5 item 2 was reworded as proposed, with the titles tagged P (ConAir excepted, see #9). It recommends ALLOWED_NOT_READY / `allowed` for the paper while the spec keeps its names; §5.3, §5.5 and §7 Q8 follow |
| 29 | minor | **Fixed.** §0 and §4.3 use the horizon formulation. T1's concrete AI-level expectations are kept beneath it as [I] |
| 30 | minor | **Fixed.** §5.7(a) anchors the notion in AI p. 335 (text seen) and cites Lu et al. 2008 only for the category. It adds the fixed-order extension sentence, and §7 Q11 notes the reviewer question about deterministic bugs |
| 31 | minor | **Fixed.** The derived field is now "source tier" (근거 등급), derived from `prov` and `Allowed`. §5.3 reserves "message provenance" for Prov(p). The spec has 22 "message provenance" lines, and 60 lines contain "provenance" in either sense |
| 32 | minor | **Fixed, as far as possible.** The tags were carried over with "(not re-verified in the citation or adversarial pass)". AI FSE 2014 is V from the document front matter, with the quotes verified as text. The AI reference-list support is labelled as secondary corroboration. The re-run list is in the header and §7 Q12. It could not be run in this revision (see #1) |
| 33 | P1 (EXT §6.2) | **Revision 3. Invariant rationale.** The reason not to call κ an invariant is now that κ is a specification object (required state, accepted not-ready outcomes with horizons, `appreq`, harm, provenance, scope), while an invariant is a predicate that should hold at certain points (AI p. 331, which also names the learned "likely invariants" separately) and can be written, proved or inferred, so learning is not part of its definition (T3 §2.1: classical invariants are specified or proved, Daikon's definition snippet P; EXT-20261012 §6.2). Confusion with learned-invariant tools is a secondary, reader-side point [I]. Changed: §0 row "What must the term convey?", §3 invariant row, §4.5 (a), §5.3 row "invariant inside Kairos", §5.7(b), §6.5 item 1. T2, T3 and T4 carry errata |
| 34 | P1 (EXT §6.3) | **Revision 3. Census scope.** T4 recorded one requirement term per row, from abstract-level summaries (AI apart), for 52 rows (37 distinct research papers, AI in two rows, and 14 documentation rows); 7 rows record none; paper bodies were not searched. GPredict's prose calls the Java Iterator usage rule "a more generic contract" [U: the external review's reading and a search snippet; P-PAPERS]. "0/52" and "non-standard in race work" are withdrawn, and term frequency alone is no reason to exclude "contract". Changed: header input list, follow-up list, §2.4, §3 "readiness contract" row, §7 Q12, §8 GPredict row and a new §8 row for the review. This supersedes the scope statement of note 25. T4 carries an erratum |
| 35 | EXT §6.4 | **Revision 3. Three judgements.** Supplier-side results (documented returns, supplier requirements, a supplier's drop, documented propagation) are separated from caller-side acceptance, as in spec revision 1.1 §1.6 "Three judgements", §1.10, §3.9 and ADR-043: `appreq`, the side of each provenance record and the caller-level Allowed. The SB no-subscriber loss is a framework-level `documented-not-error` beside a caller-level LOSS-UNDER-ORDER, not a `documented_drop` (PI item QT-2, which re-asks STATUS decision T2; QT-3 and QT-4 are open). Changed: §0, §1.1, §1.2, §2.1–§2.4, §4.1, §4.4, §4.5, §5.1, §5.3–§5.7, §6.1, §7 Q5–Q6, and the paste-ready sentence of §5.7(d) at L559, which the spec cites. T1–T4 carry errata. "Contract" is kept while T1 is pending |
| 36 | EXT §6.1, §5.1 | **Revision 3. Name and comparison kind.** §0 and §5.2 now say that the name by itself guarantees neither novelty nor accuracy. §1.2 item 2 no longer calls specified vs learned the central argument (spec §1.11). §6.2's AI-style comparator is an own-information re-implementation (K2), not an ablation, and says nothing about what the original tool would report (spec §8.3, §8.4) |
| 37 | minor (consistency) | **Revision 3, consistency pass.** (1) The header no longer cites working paths outside the repository: the four reports are cited by their copies in `background/terminology/`, and T4's downloaded documentation and the AI text extraction are described as unpublished working copies. (2) Note 25 is marked as superseded by note 34. (3) §5.5 and §7 Q2 mark their edit counts as revision-2 counts (commit b4ac55f), taken before spec revision 1.1. (4) §7 Q7 points to PI item QT-9. (5) §5.7(b) cites AI p. 331 for the definition of an invariant only; "written, proved or inferred" rests on T3 §2.1 and EXT-20261012 §6.2, and note 33 says the same. (6) §5.7(d) no longer states the difference from Joshi & Sen and Lifestate as settled: it is [U, P-PAPERS], as in spec §1.11 and App. B, and the check note now covers this difference too. The line positions that the spec cites (L559–561, L754, L758) are unchanged |

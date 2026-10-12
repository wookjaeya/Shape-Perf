# T3: Alternatives to "contract" for what Kairos checks

**Scope.** The PI asked whether "contract" (계약) is the right word for Kairos's object κ (`spec/SPEC_FINAL.md` §1.6; also §1.10, §1.11, §3.15, §7.2). Two sibling notes already cover parts of the question:
- T1 (`T1_ai_paper.md`) analyses the AI paper (FSE 2014).
- T2 (`T2_contract_lineage.md`) traces where "contract" comes from.

This note, T3, surveys the **alternative terms**. For each term it gives the established meaning with verified citations, then judges how well the term fits κ and what it collides with in cFS. The spec and the background files were only read, never edited, and nothing was committed.

**Method and limits.**
- Every citation below was checked with WebSearch during this session.
- The egress policy blocked university hosts: a 403 for `www.cs.columbia.edu` and `pages.cs.wisc.edu`, and DNS failures for `www.cs.purdue.edu`, `csaws.cs.technion.ac.il`, `people.cs.ksu.edu` and `www.freedesktop.org`. Scholar Gateway returned "access denied". So I could not open the full texts.
- The only primary texts I actually read:
  - the AI paper, all 11 pages as images plus `pdftotext`;
  - the pinned cFS tree `/home/user/work/procrace/cfs_ref/cFS`;
  - the bodies of issues cFE #2663 and CF #184, read through GitHub MCP `search_issues`.

**Tags.**
- Bibliographic status:
  - **V**: authors, title, venue and year confirmed by a search result or a catalogue or doc page.
  - **P**: partly confirmed (the gap is named).
  - **UNVERIFIED**: not confirmed.
- What I saw of the content:
  - **[seen]**: I read the text myself.
  - **[abs]**: an abstract or summary returned by the search.
  - **[snip]**: a short phrase in a search snippet. The wording may differ from the original.
  - **[gk]**: general knowledge, not checked in this session.
- Other tags:
  - **[F:code path:line]**: a cFS fact I re-read for this note.
  - **[F:issue]**: GitHub issue text.
  - **[F:spec §]**: a statement taken from the Kairos spec.
  - **[I]**: my inference.
  - **[S]**: a suggestion.

---

## 0. Bottom line

1. **What has to be named.** κ has three parts:
   - (a) a **required lifecycle state of a keyed framework resource at a use**, where the state is established by *another* actor;
   - (b) a finite list of **tolerated not-ready outcomes**, each with a horizon;
   - (c) **provenance from references**, never from runs ([F:spec §1.6, §7.2]).

   No established term covers all three. Each candidate fits one part.
2. **No word by itself says "specified, not learned".**
   - Nearly every candidate term exists in both a specified form and a mined form:
     - specifications vs. "mining specifications" (Ammons et al.);
     - typestate vs. "inference of typestate specifications" (Beckman & Nori);
     - preconditions vs. "inference of necessary preconditions" (Cousot et al.);
     - protocols vs. "inference of function precedence protocols" (Ramanathan et al.);
     - rules vs. "mining temporal API rules" (Perracotta).
   - The two exceptions point in opposite directions:
     - "(likely) invariant" in the concurrency-bug line is tied to **learning from runs**;
     - "contract" (DbC) is tied to a **human-written agreement**, although "inferred contracts" also exist (T2 §3.14).
   - **Whatever word is chosen, Kairos must carry the qualifier "reference-derived" and the provenance tier.** The PI's worry cannot be solved by the noun alone.
3. **Reject "invariant" and "likely invariant" for κ.**
   - In the line Daikon → AVIO → DefUse / PSet / CCI → AI → Bugaboo / Recon, the object is learned from runs judged correct, or from labelled runs.
   - It is also meant to hold "always" at a point. κ is not always true at the use: TOLERATED occurrences are legitimate.
   - R2's wrong order occurs in every boot, so every one of these learners would absorb it as normal [I]. T1 and spec §1.11 make the same argument for AI and DefUse.
   - Reserve "invariant" for the proof side: MustBefore certificates are inductive facts over skeletons [I].
4. **The most precise established term for part (a) is typestate.**
   - Strom & Yemini 1986 introduced it. It was extended to objects and method specifications by DeLine & Fähndrich 2004 and Bierhoff & Aldrich 2007, and to aliasing by Fink et al. 2006.
   - There is **concurrent** typestate work:
     - Joshi & Sen, ASE 2008, *predictive typestate checking of multithreaded programs*;
     - Yang et al., MSR-TR-2008-108, typestate plus lockset;
     - Beckman, Bierhoff & Aldrich, OOPSLA 2008;
     - Crafa & Padovani, OOPSLA 2015.
   - There is **framework-lifecycle** typestate checked on traces: Lifestate, ECOOP 2019.
   - "Typestate" has **zero** occurrences in cFS code.
   - Note: Joshi & Sen and Lifestate are close prior art that SPEC_FINAL §1.11 does not cite [S].
5. **The best umbrella term for parts (a)+(b)+(c) is "specification", qualified as "reference-derived readiness specification".**
   - Precedents: Lifestate's "specification language", JavaMOP's "parametric specifications", and Dwyer et al.'s "property specifications".
   - This agrees with T2 §9.2.
   - "Readiness property" (the RV / model-checking sense) is the runner-up. It also captures stalls as liveness. But it collides with OSAL's "property object" structs (in the same API family as C1), and Daikon itself calls its learned facts "properties".
6. **The order part is Dwyer et al.'s "precedence" pattern.**
   - The provider transition must precede the use, within a scope; Kairos's scope is "before the shutdown request".
   - Use this as an **explanatory mapping, not as the name**. It loses the STALE and capacity predicates and the tolerated outcomes.
7. **"Precondition" is accurate only for API-documented instances.**
   - R2 fits: cFE writes *"The Application ID must correspond to a registered application or core service."* [F:code `cfe_evs.h:168–172`].
   - C1 does not fit: OSAL documents `OS_ERR_NAME_NOT_FOUND` as a normal result [F:code `osapi-countsem.h:160`]. The need is CF's own expectation.
   - The SB loss does not fit: Transmit is defined as delivery "to all subscribers", which is vacuously true with none [F:code `cfe_sb.h:400–403`].
   - Use "readiness precondition" for the `Req` of `api_doc`-tier instances only.
8. **Reject these as the umbrella:**
   - **protocol**: in cFS this means CFDP, SBN and network protocols (59 lines in FSW; C1 lives in the CFDP app).
   - **requirement**: cFE ships formal Functional Requirements with IDs, e.g. `cES1335` "Wait for Startup Sync". It also clashes with κ's own `Req` field.
   - **dependency**: it means data/control dependence (PDG). Kairos itself uses dependence for violation chains. Practitioners already separate "requirement dependencies" from "ordering dependencies" (systemd).
   - **assumption**: it collides with SPEC_FINAL §6.1 "Assumptions A". cFE's heading is "\par Assumptions, External Events, and Notes:", with 1048 occurrences.
   - **rule**: it collides with Kairos HB rules R1–R19.
9. **Keep the qualifier "readiness".**
   - It is native cFE vocabulary: core apps must "reach their respective 'ready' states" [F:code `default_cfe_core_private_internal_cfg.h:50–56`], and there is `CFE_ES_SystemState_CORE_READY` ("core is ready") [F:code `default_cfe_es_extern_typedefs.h:197–199`].
   - Its only collision is one OSAL comment about I/O readiness [F:code `os-impl-files.c:68`].
10. **Recommended scheme** [S] (details in §5): see the table below. Only `api_doc`-tier violations should be called "API contract violations".

| Kairos element | Proposed term |
|---|---|
| κ | reference-derived **readiness specification** |
| `Req` | **required typestate** (in general); **readiness precondition** (only when an API document states it) |
| `Allowed` | **tolerated not-ready outcomes** |
| violation | **readiness violation**, related to Lu et al.'s **order violation** class |
| "contract" | kept only for documented API semantics ("API contract", M_api) |

---

## 1. The object, a test sentence, and how cFS developers phrase it

### 1.1 What κ contains ([F:spec §1.6])

κ = ⟨id, U, sel, Req, Oracle, Allowed, harm, prov, authored, guard, scope⟩. The parts that matter for naming:

- **`sel` + `Req`**: a key-selected resource and a conjunction of lifecycle/value predicates at the use's linearization point. Some conjuncts are typestate (LIVE, REG, ROUTED, generation). Some are not: `depth(p) < cap` (K-CAP), and check-then-act atomicity (K-ATOM).
- **`Allowed`**: `none`, `documented_drop`, `returned_and_tested`, `retry_until_ready(codes, bound)`, `fallback`, `defer_by_design`, `exit`, or `unspecified_by_reference`.
- **`prov`**: one of `api_doc`, `code_admission`, `code_pattern`, `issue`, `researcher`. Contracts are frozen before evaluation and never trained on runs ([F:spec §7.2]).

### 1.2 Three facts that any term must survive

| Fact | Source | Consequence for the term |
|---|---|---|
| **R2 (cFE #2663) is fixed-order.** In every boot, ES and EVS use SB's AppId before SB publishes it: 24 uses per boot, seen in 3/3 boots with gdb and with an instrumented build. | task context; [F:spec §1.11] | A term that implies "learned from normal runs" names the opposite of what Kairos does. |
| **C1 (CF #184) tolerates.** A failed first lookup by name is retried, 25 × delay. | [F:code `cf_cfdp.c:1271–1291`; `cf_internal_cfg.h:173`] | A term that implies "always true at the use" is wrong: a not-ready state can be legitimate. |
| **SB subscribe-before-first-send losses are silent**, and no reference says that the subscriber must exist first. | [F:spec §3.15 RC5]; [F:code `cfe_sb.h:400–403`] | A term that implies a documented obligation overclaims for this kind. |

### 1.3 Test sentence S*

> *Resource R (key k) must be in lifecycle state S at the linearization point of operation op, which executes in a task other than R's provider; otherwise the outcome must be one of the listed tolerated not-ready outcomes.*

Each candidate is scored on three facets of S*:
- **(state)**: can it name a state predicate at op?
- **(other task)**: does its semantics allow the state to be changed by a concurrent actor?
- **(tolerance)**: can it list acceptable outcomes when the state does not hold?

### 1.4 How cFS developers phrase these [seen]

- **cFE #2663**, title: *"Unregistered AppId error with CFE_EVS_SendEventWithAppID() during cFE task startup"*. Body: *"…returns a `CFE_EVS_APP_ILLEGAL_APP_ID` presumably because CFE_SB has not been registered with event services. This might have gone unnoticed because there is no event outputted nor is the return code set to any variable."* [F:issue]
- **CF #184**, title: *"Possible race condition with creation of throttle sem (needs to exist before CF initializes)"*. Body:
  - *"the CF app expects that semaphore to be created _before_ it starts"*
  - *"there is no guarantee that the semaphore has been created before CF attempts to use it"*
  - *"Expected behavior: Should be guaranteed via sync mechanisms, or shouldn't be a hard error (e.g. maybe retry to bind later?)"* [F:issue]
- **CF code comment:** *"There is a start up race condition because CFE starts all apps at the same time, and if this sem is instantiated by another app, it may not be created yet. Therefore if OSAL returns OS_ERR_NAME_NOT_FOUND, assume this is what is going on, delay a bit and try again."* [F:code `cf_cfdp.c:1272–1278`]
- **The only technical use of "contract" in the tree** is a unit-test helper: *"The real thing should be implemented per the contract … that does not meet the required contract."* [F:code `cfe/modules/tbl/ut-coverage/tbl_ut_helpers.c:357–359`]. Every other "contract" line is licence text.

**Reading** [I]:
- Practitioners use these words: *registered*, *needs to exist before*, *expects … before*, *race condition*, *ready state*, *assume*. They never use invariant, typestate or protocol in this sense.
- The CF #184 reporter's "expected behavior" names **both halves of κ**. "Guaranteed via sync mechanisms" is the required state, achieved through order. "Shouldn't be a hard error … retry" is a tolerated not-ready outcome.

---

## 2. Survey by family

### 2.1 Program invariants and likely invariants

**Works.**
- Ernst, Cockrell, Griswold, Notkin, "Dynamically Discovering Likely Program Invariants to Support Program Evolution", IEEE TSE 27(2):99–123, 2001 **[V]**. One MIT reference list gives pp. 1–25; the journal listing gives 99–123. An earlier version appeared at ICSE '99.
- Ernst, Perkins, Guo, McCamant, Pacheco, Tschantz, Xiao, "The Daikon system for dynamic detection of likely invariants", Sci. Comput. Program. 69(1–3):35–45, 2007 **[V]**.

**Meaning.**
- Daikon's definition [snip, SCP 2007 abstract and Daikon manual]: *"An invariant is a property that holds at a certain point or points in a program; these are often used in assert statements, documentation, and formal specifications."*
- Its dynamic detection *"runs a program, observes the values that the program computes, and then reports properties that were true over the observed executions."* [snip, manual]
- Classical invariants (loop or class invariants in Hoare/DbC reasoning) are specified or proved. I did not verify the Hoare 1969 wording for loop invariants [gk].

**Specified or learned.** Classical invariants are specified or proved. *Likely* invariants are learned, and the accuracy "depends on the test suite" (T2 §3.14, [abs]).

**Against S*.**
- (state): yes, as a predicate at a point.
- (other task): yes, if the state is in shared memory.
- (tolerance): **no**, since an invariant holds always.

**R2** [I]. Over cFS boots, Daikon at the EVS use site would report the pre-publication value of SB's AppId as an invariant. The defect would become the norm.

**Collisions.**
- 0 occurrences in cFS FSW.
- Inside Kairos, the word fits the certificate side (MustBefore facts that hold over every path of the skeleton). Keep it there.

### 2.2 Concurrency invariants learned from runs (the AI line)

The AI paper states the community's usage [seen, AI p.331–332]:

> *"Program invariants are predicates that should always be true at certain points of the execution; they reflect programmers' intentions. Many recent works pay close attention to learn 'likely invariants' from testing runs and use them to detect or tolerate software bugs [5, 28, 7]. These invariants are supposed to be held in all the correct runs."*

On training [seen, AI p.334]:

> *"In order to infer AI automatically without any programmers' annotation, we rely on correct runs observed during the in-house testing phase."*

> *"programmers can assert whether a test run is correct or not by verifying the outputs."*

On learning in general [seen, p.339]:

> *"Similar to all the other invariant-based techniques, AI needs sufficient tested execution traces to achieve a good coverage."*

| Work (status) | What the object is called; what it is | Learned from | Expresses S*? | R2 / C1 / SB [I] |
|---|---|---|---|---|
| **AVIO**: Lu, Tucek, Qin, Zhou, ASPLOS 2006, pp. 37–48 **[V]** | **access interleaving invariant**. *"two static instructions from one thread that should not be interleaved by an unserializable memory operation from a different thread"* [seen, AI p.336]. The abstract says it "indicates programmers' assumptions about the atomicity of certain code regions" [abs] | multiple executions [abs] | atomicity only. AI's authors say *"AVIO cannot handle order violations at all"* [seen, AI p.336] | R2 is outside its class |
| **DefUse**: Shi, Park, Yin, Lu, Zhou, Chen, Zheng, OOPSLA 2010, pp. 160–174 **[V]**. The Tsinghua page spells it "DeFuse"; AI's ref [28] spells it "Defuse" | **definition-use invariants**: DUI-LR (*"a local read should always read a value defined by a local or remote writer"*), DUI-Follower, DUI-DSet [seen, AI p.336–337]. The abstract frames bugs as "violations to programmers' data flow intentions" [abs] | runs [abs] | order of def→use: yes. Lifecycle state: only as memory values. Tolerance: no | R2: the DSet of the EVS read contains the init write in every run, so it is learned as normal (spec §1.11 makes the same argument) |
| **PSet**: Yu, Narayanasamy, ISCA 2009, pp. 325–336, DOI 10.1145/1555754.1555796 **[V bibliographic]** | **interleaving constraints**. PSet ensures *"a memory operation M can only 'immediately depend on' an instruction P that belongs to a specific set named PSet, which is established during the testing phase"* [seen, AI p.337; the PSet paper itself was not read] | testing phase | immediate cross-thread predecessor | R2: every tested interleaving has the pre-publication predecessor, so it is allowed |
| **CCI**: Jin, Thakur, Liblit, Lu, OOPSLA 2010, pp. 241–255, DOI 10.1145/1869459.1869481 **[V]** | **predicates** used as statistical **failure predictors** [abs]. AI describes CCI as tracking *"whether the last access was from the same thread"* … [seen, AI p.337] | passing **and** failing production runs (statistical) [abs] | interleaving facts | R2: the predicate is true in every run and no run is labelled failing, so there is no signal |
| **AI**: Zhang, Wu, Lu, Qi, Ren, Zheng, FSE 2014, pp. 330–340, DOI 10.1145/2635868.2635885 **[V; read in full]** | **Anticipating Invariant**: RPre(IₓS_y) ∈ BSet(S_y). It *"reflects programmers' intentions about the correct order of concurrent memory accesses"* [seen, p.331] | correct test runs [seen] | immediate remote predecessor (incl. `nil`) | R2 absorbed (T1 §6.1). C1: no notion of a tolerated outcome. SB: learned as normal |
| **Bugaboo**: Lucia, Ceze, MICRO 2009 **[V]** | context-aware **communication graphs** from many executions, with "invariant-based" anomaly detection [abs] | many executions | communication edges | R2: an edge seen in every run is normal |
| **Recon**: Lucia, Wood, Ceze, PLDI 2011 **[V]**. Title wording differs: "Using" (PDF name; [04]) vs. "with" (UW research page) | **reconstructions** of buggy-schedule fragments, ranked by machine learning over many runs [abs] | many runs, including buggy ones | communication around a root cause | needs runs that are recognized as buggy |
| *Code-mined, not run-learned:* **MUVI**: Lu, Park et al., SOSP 2007, pp. 103–116 **[V venue and pages; full author list P]** | **multi-variable access correlations** inferred from code [abs] | source code | variable correlations | majority logic, as in Engler's beliefs (§2.4) |

**Lesson for naming** [I].
- In this community, "invariant", "predicate" and "constraint" all mean *what held in the training runs*. The ground truth behind them is called the programmer's **intention**:
  - AI p.331 [seen];
  - Lu et al. 2008's "order intentions" (ASPLOS 2008 **[V]**; wording **P**, see §2.9).
- Shan Lu co-authored AVIO, DefUse, CCI, AI and the 2008 study. Reviewers from this line will read "readiness invariant" as learned.
- "Contract", "specification" and "typestate" do not carry that reading.

### 2.3 Typestate

**Origin.** Strom, Yemini, "Typestate: A Programming Language Concept for Enhancing Software Reliability", IEEE TSE 12(1):157–171, 1986, DOI 10.1109/TSE.1986.6312929 **[V]**.
- Abstract [abs]: typestate refines type. Where a type fixes every operation an object may support, typestate fixes the subset permitted in a given context. Compile-time tracking catches errors such as reading a variable before initialization and using a pointer after deallocation.
- Some IBM pages list this paper under other author names. The search treated these as metadata errors: DBLP and the DOI give Strom and Yemini.

**Protocols on objects and resources.**
- DeLine, Fähndrich, "Enforcing High-Level Protocols in Low-Level Software" (Vault), PLDI 2001, pp. 59–69 **[V]**.
  - Abstract [abs]: resource-management **protocols** that "can require operations in a specific order and can require certain steps before a data object is accessed", checked statically.
  - Evaluated on the Windows 2000 kernel–driver interface.
- DeLine, Fähndrich, "Typestates for Objects" (Fugue), ECOOP 2004, LNCS 3086:465–490 **[V]**.
  - A modular, sound typestate checker for .NET.
  - The companion MSR-TR-2004-07 says it lets developers "restrict the order of method calls to transitions of a finite state machine" [snip].
- Fink, Yahav, Dor, Ramalingam, Geay, "Effective Typestate Verification in the Presence of Aliasing", ISSTA 2006, pp. 133–144 **[V]**.
  - A TOSEM 2008 journal version exists: **P**. Yahav's project page lists TOSEM 2008, but I could not confirm the volume, issue or article number.
  - Abstract [abs]: sound typestate verification for Java that combines typestate and aliasing, checking "correct API usage". It verified 93% of the potential points of failure.
- Bierhoff, Aldrich, "Modular Typestate Checking of Aliased Objects", OOPSLA 2007, pp. 301–320 **[V]**, confirmed in reference lists, e.g. arXiv 1205.5344, and on Bierhoff's CMU page.
  - Content [abs, secondary]: access permissions plus method specifications that depend on run-time state.

**Concurrent typestate** (cross-task state change, the "(other task)" facet):
- Joshi, Sen, "Predictive Typestate Checking of Multithreaded Java Programs", ASE 2008, pp. 288–296 **[V]** (DBLP index; ACM DL and IEEE Xplore listings).
  - Abstract [abs]: a *"typestate error … happens when a program does not follow the correct usage protocol of an object"*.
  - From one successful execution, it finds typestate errors that occurred *and* "many other typestate errors that could have occurred in a different execution".
  - [I] This is the closest prior art to Kairos's prediction layer applied to readiness. **It is not in SPEC_FINAL §1.11** [S].
- Yang, Gringauze, Wu, Rohde, "Detecting Data Race and Atomicity Violation via Typestate-Guided Static Analysis", MSR-TR-2008-108 **[V for the report]**.
  - The first author's given name is **P**: the MSR page says "Jason", the PDF says "Yue".
  - Content [abs]: typestate checking combined with lockset analysis. It moves the typestate to a transient state to simulate thread interference, for C/C++, applied to Windows.
- Beckman, Bierhoff, Aldrich, "Verifying Correct Usage of Atomic Blocks and Typestate", OOPSLA 2008, pp. 227–244 **[V]**.
  - Content [abs]: a static analysis with access permissions that enforces typestate properties in concurrent programs.
- Crafa, Padovani, "The Chemical Approach to Typestate-Oriented Programming", OOPSLA 2015 **[V]**; TOPLAS 39 (2017), DOI 10.1145/3064849 **[V]**.
  - Content [abs]: typestate where "several processes can access and modify objects at once".

**Framework lifecycle typestate on traces.** Meier, Mover, Chang, "Lifestate: Event-Driven Protocols and Callback Control Flow", ECOOP 2019, LIPIcs 134 **[V]**.
- Content [abs]: a **specification language** that "combines lifecycle constraints with typestate rules", stating "what callbacks can expect from the framework and what the app must respect when calling into it".
- Its dynamic algorithm takes a trace and "either finds a protocol violation or shows that none is realizable".
- [I] It is the closest prior work in *shape*: framework lifecycle, typestate, and checking on traces with realizability.

**Typestate can also be learned.**
- Beckman, Nori, "Probabilistic, Modular and Scalable Inference of Typestate Specifications", PLDI 2011, pp. 211–221 **[V]**.
- Whaley, Martin, Lam, ISSTA 2002 **[V]**: finite-state interface models extracted statically and dynamically [abs].
- So "typestate" is **neutral** about provenance.

**Against S*.**
- (state): **yes**, by definition. Kairos's lifecycle automata A_k ([F:spec §1.3]) *are* typestate automata.
- (other task): **yes**, in concurrent typestate.
- (tolerance): **no**. Typestate specifies permitted operations, not acceptable failures. Methods with state-dependent postconditions can model error returns (Fugue and Plural method specifications [abs]), but there is no notion of a continuation horizon.

**Expected by reviewers** [I]:
- a finite-state machine per object;
- the aliasing problem (in Kairos, key resolution via `sel` and `MUST_KEY` plays this role);
- sound static checking or predictive dynamic checking;
- the question "who changes the state?". The answer is the provider actor, which is exactly the concurrent setting.

**Collisions.**
- None in cFS: the 0 hits exclude OSAL UT stubs, whose `UT_ObjTypeState_t` is unrelated.
- **Caveat** [I]: capacity (`depth < cap`) and TOCTOU atomicity are not typestate, so §1.11's "Capacity as typestate" stretches the word. Name those kinds apart (see §5.3).

### 2.4 API usage protocols, rules and specification mining

| Work (status) | Name of the object | Specified or learned | Content |
|---|---|---|---|
| Ammons, Bodík, Larus, "Mining Specifications", POPL 2002, pp. 4–16, DOI 10.1145/503272.503275 **[V]** | **specification** (protocol) | learned from traces | the learned specifications "captured a correct protocol and also exposed serious bugs" [abs] |
| Whaley, Martin, Lam, "Automatic Extraction of Object-Oriented Component Interfaces", ISSTA 2002, DOI 10.1145/566210.566212 **[V]** | component **interface** (FSM submodels) | static and dynamic | illegal call sequences [abs] |
| Yang, Evans, Bhardwaj, Bhat, Das, "Perracotta: Mining Temporal API Rules from Imperfect Traces", ICSE 2006 **[V]** | **temporal API rules** | learned from imperfect traces | inferred "many of the properties checked by the Static Driver Verifier" [abs] |
| Wasylkowski, Zeller, Lindig, "Detecting Object Usage Anomalies" (JADET), ESEC/FSE 2007, pp. 35–44, DOI 10.1145/1287624.1287632 **[V]** | legal **method-call sequences**, patterns | learned from code examples | e.g. `hasNext()` before `next()` [abs] |
| Weimer, Necula, "Mining Temporal Specifications for Error Detection", TACAS 2005, LNCS 3440, pp. 461–476 **[V]** | **temporal safety rules** (specifications) | learned, using error-handling paths [abs] | — |
| Ramanathan, Grama, Jagannathan, "Path-Sensitive Inference of Function Precedence Protocols" (CHRONICLER), ICSE 2007, pp. 240–250 **[V]** | **function precedence protocols** | inferred statically, by sequence mining [abs] | deviations flagged as bugs |
| Engler, Chen, Hallem, Chou, Chelf, "Bugs as Deviant Behavior: A General Approach to Inferring Errors in Systems Code", SOSP 2001 **[V]** | **beliefs** (MUST and MAY) and rule templates such as "`<a>` must be paired with `<b>`" [abs] | inferred from code: MUST beliefs directly, MAY beliefs statistically | — |

**Lesson** [I].
- The mining community calls the learned object specification, rule, protocol, pattern or belief. The *specified* counterpart in SDV is "API usage rules" (SDV, EuroSys 2006: V-abs in [05]; also T2 §3.13).
- Kairos's `code_admission` and `code_pattern` tiers are **Engler-style MUST beliefs read from one site**. Example: CF's retry loop and its comment show that CF believes the lookup may fail early. They are neither learned from runs nor statistical. Calling them *beliefs inferred from code* is more accurate than calling them contracts (T2 §0 item 3 agrees).

### 2.5 Temporal safety and liveness properties; specification patterns

**Foundations.**
- Lamport, "Proving the Correctness of Multiprocess Programs", IEEE TSE SE-3(2):125–143, 1977 **[V]**.
  - Safety: *"something will not happen"*. Liveness: *"something must happen"*. **P** for the wording, which came from a summary of the paper.
- Alpern, Schneider, "Defining Liveness", IPL 21(4):181–185, 1985 **[V]**: formal definitions; every property is the intersection of a safety property and a liveness property [abs].
- Pnueli, "The Temporal Logic of Programs", FOCS 1977, pp. 46–57 **[V]**.

**Specification patterns.** Dwyer, Avrunin, Corbett, "Patterns in Property Specifications for Finite-State Verification", ICSE 1999, pp. 411–420 **[V bibliographic]**.
- **Precedence** intent: *"the occurrence of the first (enabler) is a necessary pre-condition for an occurrence of the second (action)"* [snip, quoted in a later UMass technical report].
- Scopes: global, before R, after Q, between Q and R, after Q until R [snip, Dwyer's scope page].
- **P** for both wordings: I could not open the pattern pages.

**Usage rules in C.**
- Ball, Rajamani, "The SLAM Project: Debugging System Software via Static Analysis", POPL 2002, pp. 1–3 **[V]**.
  - Checks whether a C program obeys **"API usage rules"** that specify what it means to be a good client of an API [abs].
  - The goal is to check **"temporal safety properties"** of *sequential* C programs, i.e. properties whose violation is witnessed by a finite execution trace [snip; **P** for wording].
- Ball, Rajamani, "Automatically Validating Temporal Safety Properties of Interfaces", SPIN 2001, DOI 10.1007/3-540-45139-0_7 **[V]**; checks properties such as correct locking in drivers [abs].
- Das, Lerner, Seigle, "ESP: Path-Sensitive Program Verification in Polynomial Time", PLDI 2002 **[V]**. Pages are **P**: 57–68 vs. 58–70.

**Against S*** [I].
- An instance with `Allowed = none` is a **parametric precedence safety property**: every `use(k)` is preceded by the provider transition on `k`, with no disabling transition in between, within the scope. A bad occurrence is a finite prefix, so this is **safety**.
- With `Allowed`, the property is still safety when every horizon is finite. For example, under `retry_until_ready(codes, b)`, a violation is witnessed once the b retries are exhausted.
- Under σ_U∞ (patient retries, [F:spec §0.2 item 5]), "eventually ready" is **liveness**. This matches Kairos's `stall` outcome.
- So this vocabulary explains the semantics well: safety for the σ_U verdicts, liveness for the stall outcomes.

**Scope caveat** [I; UNVERIFIED against the catalogue]. Kairos's scope is "no shutdown-request LP precedes the use". This must hold also in runs where shutdown never comes. The search snippet says that in Dwyer's patterns, "if the scope delimiters are not present in an execution then the specification will be true". So the correct Dwyer scope may be "after Q until R" with Q = program start, rather than "before R". Check this against the pattern catalogue before writing it in a paper.

**Expected by reviewers** [I]: LTL or automata, a model checker or monitor, and pattern names such as precedence, response and absence.

**Collisions.**
- "property" collides with OSAL's info structs ("Fill a property object buffer …", [F:code `osapi-countsem.h:166`, `osapi-mutex.h:134`]). This is the C1 API family.
- "safety" is overloaded in a flight-software paper (mission safety vs. temporal safety) [I].

### 2.6 Runtime verification: parametric properties and tracematches

- Chen, Roşu, "MOP: An Efficient and Generic Runtime Verification Framework", OOPSLA 2007, pp. 569–588 **[V]**. The venue and pages come from a reference list; the DOI was not confirmed.
  - Content [abs]: a **parametric**, formalism-independent extension of MOP, so that properties can refer to "two or more related objects". Implemented as JavaMOP.
- Chen, Roşu, "Parametric Trace Slicing and Monitoring", TACAS 2009 **[V]**.
  - Content [abs]: each event goes to the trace slices of the parameter bindings it belongs to, and any non-parametric analysis then runs per slice.
  - Extended as Roşu, Chen, "Semantics and Algorithms for Parametric Monitoring", LMCS 2012 **[V]**.
- Meredith, Jin, Griffith, Chen, Roşu, "An Overview of the MOP Runtime Verification Framework", STTT 14(3):249–289, 2012 **[V]**. Content [abs]: "Monitors are automatically synthesized from specified properties."
- Allan, Avgustinov, Christensen, Hendren et al., "Adding Trace Matching with Free Variables to AspectJ", OOPSLA 2005, DOI 10.1145/1094811.1094839 **[V]**. Content [abs]: **tracematches**, regular patterns over events with free variables bound to values.
- Leucker, Schallhart, "A Brief Account of Runtime Verification", JLAP 78(5):293–303, 2009 **[V bibliographic; content not retrieved]**.

**Against S*** [I].
- Kairos's `sel` is a **parameter binding**, and a κ instance `(κ, t, key)` is a **parametric slice** or monitor instance.
- (state): yes. (other task): yes, since monitors observe the global trace.
- (tolerance): only if written into the property; RV has no first-class notion of it.
- RV checks the *observed* trace. Kairos also predicts, certifies and holds, so the RV vocabulary fits only the trace layer.

**Expected by reviewers**: a specification formalism, monitor synthesis, overhead numbers.

**Naming value.**
- "Parametric property instance" is a good precedent for Kairos's "instance".
- "Specification" in "parametric specification" is a precedent for the umbrella term.

### 2.7 Preconditions and precondition inference

**Foundations.**
- Hoare, "An Axiomatic Basis for Computer Programming", CACM 12(10):576–580, 583, 1969 **[V]**. Page ranges differ between sources.
  - P{Q}R: *"If the assertion P is true before initiation of a program Q, the assertion R will be true on its completion"* [snip].
- Dijkstra, "Guarded Commands, Nondeterminacy and Formal Derivation of Programs", CACM 18(8):453–457, 1975 **[V]**: weakest-precondition semantics [abs, secondary].
- Design by Contract: Meyer, IEEE Computer 25(10):40–51, 1992 **[V]**; see T2 §3.2.

**Inference.**
- Cousot, Cousot, Fähndrich, Logozzo, "Automatic Inference of Necessary Preconditions", VMCAI 2013, LNCS 7737, pp. 128–148 **[V]**.
  - The question it asks: *"under which precondition, if violated, will the program always be incorrect?"* [snip].
- Ramanathan, Grama, Jagannathan, "Static Specification Inference Using Predicate Mining", PLDI 2007 **[V]**.
  - The abstract's example preconditions: *"whenever p is called, variable x must be non-null, and every call to p must be preceded by a call to q"* [snip].
  - [I] So "precondition" already covers **precedence** facts in the literature, but over one thread's call history.

**Against S*** [I].
- (state): yes.
- (other task): **weak**. A precondition over state that another task owns is unstable unless something guarantees the state stays put (rely-guarantee; T2 §3.7). Kairos proves stability separately, through MustBefore and the absence of disabling transitions.
- (tolerance): **no**. In DbC a precondition violation is the client's fault, and the supplier owes nothing. `Allowed` lies outside.

**Per case.**
- R2 is a documented precondition (§0 item 7).
- C1 is not an OSAL precondition. `OS_ERR_NAME_NOT_FOUND` is in OSAL's documented domain.
- SB Transmit has no such precondition.

**Collisions.**
- cFE API docs never say "precondition": 0 FSW hits, outside test comments. They use "\par Assumptions, External Events, and Notes:" (1048 occurrences).
- Those blocks mix true preconditions (EVS AppId), usage notes (*"NOTE: This API is not needed for Applications that call the CFE_ES_RunLoop call."* [F:code `cfe_es.h:463–464`]) and "None".
- SPEC_FINAL §6.1 uses "Assumptions A" for the soundiness manifest.

### 2.8 Behavioural and session types (protocols between roles)

- Honda, Vasconcelos, Kubo, "Language Primitives and Type Discipline for Structured Communication-Based Programming", ESOP 1998, LNCS 1381:122–138 **[V]**.
- Honda, Yoshida, Carbone, "Multiparty Asynchronous Session Types", POPL 2008, pp. 273–284 **[V]**. Content [abs]: **global types** describe multiparty conversations and "provide a means to check protocol compliance"; the results cover communication safety, progress and session fidelity.
- Gay, Gesbert, Ravara, Vasconcelos, "Modular Session Types for Objects", LMCS 11(4:12), 2015 **[V]**. Session types attached to classes yield a form of **typestate** for non-uniform objects [abs].
- Hüttel et al., "Foundations of Session Types and Behavioural Contracts", ACM CSUR 49(1), article 3, 2016, DOI 10.1145/2873052 **[V]**. Content [abs]: behavioural types cover "interfaces, communication protocols, and **contracts**".

**Against S*** [I].
- These describe message exchanges in typed sessions.
- SB's subscribe-before-first-send could be read as an ordering obligation of a global protocol among subscriber, sender and SB.
- But cFS routes by MsgId tables and named OSAL objects, not typed session endpoints, and registries (OSAL names, ES records) are not sessions.
- Not usable as Kairos's name.

**Relevance to the PI's question.** In concurrency theory "contract" also means a **behavioural** description with **compliance** semantics (Hüttel et al.), unlike DbC. A reviewer from that community may expect compliance or subcontract relations if Kairos keeps "contract" [I].

### 2.9 Ordering requirement / constraint / intention / assumption; dependency

**Bug-study vocabulary.**
- Lu, Park, Seo, Zhou, "Learning from Mistakes — A Comprehensive Study on Real World Concurrency Bug Characteristics", ASPLOS 2008 **[V]**.
- Abstract [abs]: about one third of the non-deadlock bugs examined come from violating programmers' intended order. The exact phrase "order intentions" is **P**, from T2's snippet.
- Its formal definition of "order violation" could **not** be retrieved: the full text is blocked. I give no quote **[UNVERIFIED wording]**.
- AI p.331 [seen]: *"programmers' intentions about the correct order of concurrent memory accesses"*.
- PSet: "interleaving **constrained**" (title).

**Practitioners.**
- CF #184 [F:issue]: "needs to exist before", "expects … created before".
- systemd's unit documentation [**P**: wording seen only in search snippets of man-page mirrors and tutorials, not on freedesktop.org]: requirement dependencies "do not influence the order in which services are started or stopped. This has to be configured independently with the After= or Before= options."

**Dependence in program analysis.** Ferrante, Ottenstein, Warren, "The Program Dependence Graph and Its Use in Optimization", TOPLAS 9(3):319–349, 1987 **[V]**: data and control dependence [abs].

**Against S*** [I].
- "Ordering requirement" names only the derived obligation that the provider transition must precede the use.
- It cannot express:
  - the ABSENT shape (no provider; [F:spec §1.10]);
  - STALE (generation mismatch);
  - capacity;
  - tolerated outcomes.
- "Dependency" conflates *needs R* with *needs R first*, which systemd keeps separate. C1 involves both.

**Collisions.**
- "requirement":
  - `cfe/docs/cFE_FunctionalRequirements.csv` (570 lines, IDs such as `cES1335` "ES: Wait for Startup Sync");
  - per-app `*_FunctionalRequirements.csv` for SC, MM, HS, CS, DS and FM;
  - SB's *"Some functions have EXTERNAL SYNC REQUIREMENTS"* [F:code `cfe_sb_priv.c:56`], meaning locking obligations;
  - Kairos's own `Req` field.
- "ordering": memory ordering, and Kairos §1.9 "Ordering relations".
- "dependency": build dependencies (cmake), and Kairos's violation chains ("control or data dependence", §1.6).
- "assumption": §6.1 "Assumptions A". But it is the right word for the **source** text: cFE's "\par Assumptions" heading and CF's "assume this is what is going on".

---

## 3. Comparison matrix

Counts are whole-word matches in the pinned tree's flight-software C sources (`cfe`, `osal`, `psp`, `apps`), with unit tests excluded. "Spec" means SPEC_FINAL.

| Term | Established meaning (anchor) | Specified or learned | S*: state / other task / tolerance | What reviewers will expect [I] | Collision in cFS or Kairos | Verdict |
|---|---|---|---|---|---|---|
| **contract** (baseline; T2) | DbC client/supplier agreement with blame (Meyer 1992); also behavioural contracts (Hüttel 2016), A/G (T2) | specified (also "inferred contracts") | yes (as precondition) / weak (sequential DbC; SCOOP waits) / partial (exceptional postconditions) | pre/post, blame, maybe compliance or composition | FSW: licence text, plus 1 UT comment ("per the contract", `tbl_ut_helpers.c:357`) | keep only for documented API semantics ("API contract"; `api_doc` tier) |
| **invariant** | holds at a point in every execution (Daikon's definition; Hoare-style) | specified or proved | yes / yes / **no** | an inductive proof, or Daikon | 0 | reject for κ; reserve for certificates |
| **likely / interleaving / def-use / anticipating invariant** | what held in training runs (Daikon; AVIO; DefUse; PSet; CCI; AI) | **learned** | memory-access order / yes / **no** | training runs, coverage, false positives | 0 | reject; this is the contrast class |
| **typestate** (requirement, property, precondition) | state-dependent permitted operations (Strom & Yemini 1986; Fugue; Fink 2006; Bierhoff 2007) | either (Beckman & Nori 2011 infer it) | **yes / yes** (Joshi & Sen 2008; Beckman 2008; Yang 2008) / no | an FSM per object, aliasing, sound or predictive checking | 0 | **adopt for `Req`** |
| **protocol** (API, object, event-driven) | permitted call or message sequences (Vault; Lifestate; Chronicler; session types) | either | order yes, state implicit / yes in Lifestate and MPST / no | call-sequence FSMs, mining, session typing | **59 FSW lines** (CFDP, SBN, SpaceWire); C1 is in CF (CFDP) | reject as name |
| **specification** (readiness, lifestate, parametric) | formal description of required behaviour (Lifestate; JavaMOP; Dwyer) | **neutral**: needs the "reference-derived" qualifier vs. "mined specification" | container: yes / yes / yes (as fields) | a language with semantics and a checker | "Specification for the CFE … command and telemetry" headers (229 FSW lines); EDS; the doc name SPEC_FINAL | **adopt as umbrella** |
| **property** (temporal safety, parametric) | set of traces; safety/liveness (Lamport; Alpern & Schneider; SLAM; MOP) | neutral (Daikon also says "properties") | yes / yes / only if encoded | LTL or automata, monitors | OSAL "property object" (14 FSW lines, incl. `osapi-countsem.h:166`) | runner-up umbrella; good for the semantics section |
| **precondition** | must hold at the call; the client's duty (Hoare 1969; Dijkstra 1975; DbC) | specified; inferred (Cousot 2013; Ramanathan 2007) | yes / **weak** (stability) / no | Hoare logic, blame, static proof | 0 in FSW docs; cFE says "\par Assumptions" | use only for `api_doc`-tier `Req` |
| **precedence property** | S must precede P within a scope (Dwyer 1999) | specified pattern; mined (Perracotta, Chronicler) | partial (no STALE or capacity) / yes / no | LTL pattern | 0 | explanatory mapping only |
| **ordering requirement / constraint / intention** | the intended order (Lu 2008; AI); interleaving constraints (PSet) | "intention" is ground truth; "constraints" are learned (PSet) | order only / yes / no | a two-access order violation | "requirement": cFE Functional Requirements; `Req`; "ordering": memory order, §1.9 | prose only, for the derived obligation |
| **assumption** | A/G environment side; cFE doc heading | specified | yes / yes / no | A/G or soundness assumptions | §6.1 "Assumptions A"; 956 FSW lines of "assumption(s)" | use only to name the source text |
| **dependency** | data/control dependence (Ferrante 1987); startup dependencies (systemd) | — | vague / vague / no | a PDG, build deps | build deps; Kairos violation chains | reject |
| **rule** (API usage rule) | SDV/SLIC rules; mined "temporal API rules" | either | yes / SDV: sequential / no | SLIC, SDV | Kairos HB rules R1–R19 | reject (internal collision) |
| **readiness** (the qualifier) | cFE's own "ready" states | — | — | — | 1 OSAL comment on I/O readiness | **keep** |

---

## 4. The three cases under the top candidates

| Case | API contract (DbC) | Precondition | Required typestate within a readiness specification | Precedence property |
|---|---|---|---|---|
| **R2** cFE #2663 | **violated**: `cfe_evs.h` documents the precondition; client SB code calls with an unregistered AppId | **violated** (documented) | `evs.reg(SB)` is not REG at the use; fixed-order; tier `api_doc` | the use precedes the publication in every boot |
| **C1** CF #184 | **not violated**: OSAL documents `OS_ERR_NAME_NOT_FOUND`; the need is CF's own ("expects … before", [F:issue]) | ✗ not an OSAL precondition | `osal.countsem(name)` LIVE at lookup; the first failure is TOLERATED by `retry_until_ready(…, 25)`; tier `code_admission` (the CF comment) | true only modulo retries |
| **SB loss** (RC5) | **no contract**: Transmit's doc is vacuously true with no subscriber | ✗ | `sb.route(m, consumer)` ROUTED at route lookup; `Allowed = unspecified_by_reference` → LOSS-UNDER-ORDER; tier `unreferenced` | a candidate property with no reference |

**Reading** [I].
- "Contract" and "precondition" fit R2 only.
- A **required typestate** inside a **readiness specification with a tier** covers all three cases without overclaiming. This is the decisive argument for the scheme in §5.

---

## 5. Recommendation [S]

### 5.1 Naming scheme (a PI decision; compatible with T2 §9.2)

| Element now | Proposed English | Suggested Korean [I] | Anchor (status) |
|---|---|---|---|
| readiness contract κ | **readiness specification** (in papers, first use: "reference-derived readiness specification") | 준비 명세 (참조 근거 준비 명세) | Lifestate (V); JavaMOP (V); Dwyer (V) |
| contract instance `(κ, t, key)` | **readiness instance**, or "specification instance" | 준비 명세 인스턴스 | parametric slices (Chen & Roşu 2009, V) |
| `Req` | **required typestate**. Write **readiness precondition** only for `api_doc`-tier instances | 요구 타입상태 / 준비 사전조건 | Strom & Yemini (V); DeLine & Fähndrich (V) |
| `Allowed` | **tolerated not-ready outcomes** | 허용된 미준비 결과 | no standard term; CF #184's "shouldn't be a hard error" [F:issue] |
| bad occurrence | **readiness violation**. When a provider exists, this is an **order violation** in Lu et al.'s sense | 준비 위반 / 순서 위반 | Lu et al. 2008 (V) |
| contract library (§3.15) | **readiness specification catalogue** (in Φ) | 준비 명세 목록 | — |
| "API contract" (M_api, §1.4) | **keep**: documented supplier semantics | API 계약 | Meyer 1992 (V) |
| provenance tiers | keep T2's tiers. Report `code_admission` and `code_pattern` as **beliefs inferred from code** (Engler-style MUST beliefs), not as contracts | 코드 근거 믿음 | Engler 2001 (V) |

The Korean renderings are common usage that I did not check against a Korean terminology source: UNVERIFIED.

### 5.2 If the PI prefers fewer changes

- **(a) Keep "readiness contract".**
  - Add T2 §9.3's definition paragraph.
  - Report per tier.
  - Never say "contract violation" for `unreferenced` instances: say "readiness violation (no reference)".
  - Add one sentence that rules out the DbC blame reading for `code_*` tiers.
- **(b) Use "readiness property"** (RV framing).
  - Good for the semantics section: safety for σ_U, liveness for stalls.
  - Weaker signal of "specified", because Daikon also reports "properties".
  - Small collision with OSAL "property object".
- **(c) Use "readiness typestate" as the umbrella.**
  - Most precise for `Req`.
  - But reviewers will judge Kairos as a typestate checker (aliasing, FSMs), and it does not cover `Allowed`, `harm` or provenance.

### 5.3 Never use, and why

| Term | Reason |
|---|---|
| **invariant**, **likely invariant** | the learned reading (§2.1–2.2); R2 refutes it |
| **protocol** | CFDP/SBN collision; implies call sequences |
| **requirement** | cFE Functional Requirements; the `Req` field |
| **dependency** | PDG meaning; mixes requirement with order |
| **rule** | HB rules R1–R19 |

Also, name the non-typestate kinds by what they are, inside the same catalogue:
- **capacity property** (RC14, K-CAP);
- **atomicity specification** (RC12, K-ATOM);
- **lock-balance postcondition** (K-LOCKBAL; T2 §9.2 item 5).

### 5.4 Related-work edits for SPEC_FINAL §1.11 (not applied; the spec is read-only here)

- **Add these rows:**
  - Joshi & Sen ASE 2008 (V) as *predictive typestate checking of multithreaded programs*: the closest prior art for predicting readiness violations from one trace.
  - Lifestate ECOOP 2019 (V): framework lifecycle × typestate, checked on traces with realizability.
  - Yang et al. MSR-TR-2008-108 (V; author name P): typestate plus lockset.
  - Daikon (TSE 2001, SCP 2007; V) as the origin of "likely invariant".
  - AI FSE 2014 (V) next to DefUse.
  - Dwyer et al. ICSE 1999 (V) for the precedence pattern.
- **Change:** "Typestate contracts | Strom & Yemini 1986" → "Typestate | Strom & Yemini, TSE 12(1), 1986 [V]" (T2 also found this).
- **Draft sentence:**

  > *Unlike likely invariants learned from runs judged correct (Daikon; AVIO; DefUse; PSet; AI), a readiness specification is derived from references (API documentation, code admissions, issues) and is never trained on executions. A wrong order that occurs in every run, as in cFE #2663, therefore remains a violation. Its core is a typestate requirement (Strom and Yemini) on a framework resource that another task establishes. This is the multithreaded setting of predictive typestate checking (Joshi and Sen) and the framework-lifecycle setting of Lifestate (Meier et al.). Unlike both, a readiness specification also lists the not-ready outcomes that the framework or the client documents as acceptable, each with a reference horizon.*

### 5.5 Proposed STATUS entry (for the parent to merge; Korean, matching `STATUS_20261012.md`)

> - **T3 '계약' 대안 용어 조사** 완료 (`scratchpad/arch/terms/T3_alternatives.md`).
>   - **결론 1. invariant 계열은 쓰면 안 된다.** Daikon, AVIO, DefUse, PSet, CCI, AI, Bugaboo, Recon의 invariant는 모두 "정상 실행에서 학습한 것"이다. 그래서 R2처럼 매 부팅 같은 잘못된 순서가 나면 그것을 정상으로 배운다.
>   - **결론 2. 단어 하나로 '참조에서 명시했고 학습하지 않았다'는 뜻을 담을 수 없다.** specification, typestate, precondition, protocol, rule은 모두 명시형과 마이닝형이 함께 있다. 그래서 어떤 이름을 쓰든 "참조 근거(reference-derived)"라는 수식어와 provenance tier를 붙여야 한다.
>   - **결론 3. 가장 정확한 기성 용어는 Req에 대한 typestate다.** 동시성 typestate(Joshi & Sen ASE 2008)와 프레임워크 생명주기 typestate(Lifestate ECOOP 2019)가 있다. 두 논문 모두 §1.11에 없으므로 추가해야 한다.
>   - **결론 4. 전체 이름으로는 '준비 명세(readiness specification)'가 가장 무난하다.** '계약'은 R2처럼 API 문서가 근거인 경우에만 맞다. C1은 CF 자신의 기대이고, SB 손실은 근거 문서가 없다.
>   - **피해야 할 용어.** protocol은 CFDP/SBN과 충돌한다. requirement는 cFE 기능 요구사항(cES…)과 `Req`와 충돌한다. dependency는 PDG 의미와 겹친다. rule은 HB 규칙과 충돌한다.
>   - **PI 결정 필요.** (가) '준비 명세 + 요구 타입상태 + 허용된 미준비 결과'로 이름을 바꾸거나, (나) '준비 계약'을 유지하되 정의 문단을 두고 tier별로 보고한다.

---

## 6. References and status

| # | Reference | Status | What I saw | Source of confirmation |
|---|---|---|---|---|
| 1 | Zhang, Wu, Lu, Qi, Ren, Zheng. AI: A Lightweight System for Tolerating Concurrency Bugs. FSE 2014, pp. 330–340. DOI 10.1145/2635868.2635885 | V | [seen] full PDF | uploaded PDF; T1 |
| 2 | Ernst, Cockrell, Griswold, Notkin. Dynamically Discovering Likely Program Invariants to Support Program Evolution. IEEE TSE 27(2):99–123, 2001 | V (pages vary in one list) | [abs] | [UW abstract page](https://homes.cs.washington.edu/~mernst/pubs/invariants-tse2001-abstract.html) |
| 3 | Ernst, Perkins, Guo, McCamant, Pacheco, Tschantz, Xiao. The Daikon System for Dynamic Detection of Likely Invariants. SCP 69(1–3):35–45, 2007 | V | [snip] definition | [UW abstract page](https://homes.cs.washington.edu/~mernst/pubs/daikon-tool-scp2007-abstract.html); [Daikon manual](https://plse.cs.washington.edu/daikon/download/doc/daikon.html) |
| 4 | Lu, Tucek, Qin, Zhou. AVIO: Detecting Atomicity Violations via Access Interleaving Invariants. ASPLOS 2006, pp. 37–48 | V | [abs]; definition [seen] as quoted in AI | search record; [patent US8533681](https://patents.google.com/patent/US8533681) |
| 5 | Shi, Park, Yin, Lu, Zhou, Chen, Zheng. Do I Use the Wrong Definition? DefUse: Definition-Use Invariants for Detecting Concurrency and Sequential Bugs. OOPSLA 2010, pp. 160–174 | V (spelling of DefUse varies) | [abs]; [seen] via AI | [Tsinghua page](https://pacman.cs.tsinghua.edu.cn/~cwg/publication/oopsla10_shi/) |
| 6 | Yu, Narayanasamy. A Case for an Interleaving Constrained Shared-Memory Multi-Processor. ISCA 2009, pp. 325–336. DOI 10.1145/1555754.1555796 | V bibliographic | mechanism [seen] via AI only | [DBLP](https://dblp1.uni-trier.de/rec/conf/isca/YuN09.html) |
| 7 | Jin, Thakur, Liblit, Lu. Instrumentation and Sampling Strategies for Cooperative Concurrency Bug Isolation. OOPSLA 2010, pp. 241–255. DOI 10.1145/1869459.1869481 | V | [abs] | [Liblit page](https://pages.cs.wisc.edu/~liblit/oopsla-2010); [UC Davis bib](https://thakur.cs.ucdavis.edu/bibliography/jin_etal_OOPSLA10.html) |
| 8 | Lucia, Ceze. Finding Concurrency Bugs with Context-Aware Communication Graphs. MICRO 2009 | V | [abs] | [MSR listing](https://www.microsoft.com/en-us/research/?p=165830) |
| 9 | Lucia, Wood, Ceze. Isolating and Understanding Concurrency Errors Using (or "with") Reconstructed Execution Fragments. PLDI 2011 | V (title word varies) | [abs] | [UW SAMPA page](https://sampa.cs.washington.edu/new/research/concurrencybugs.html) |
| 10 | Lu, Park, Seo, Zhou. Learning from Mistakes — A Comprehensive Study on Real World Concurrency Bug Characteristics. ASPLOS 2008 | V; definition wording UNVERIFIED | [abs] | [MSR listing](https://www.microsoft.com/en-us/research/?p=1123215) |
| 11 | Lu, Park, et al. MUVI: Automatically Inferring Multi-Variable Access Correlations and Detecting Related Semantic and Concurrency Bugs. SOSP 2007, pp. 103–116 | V (full author list P) | [abs] | [DOI](https://dx.doi.org/10.1145/1294261.1294272) |
| 12 | Strom, Yemini. Typestate: A Programming Language Concept for Enhancing Software Reliability. IEEE TSE 12(1):157–171, 1986 | V | [abs] | [DOI](https://doi.org/10.1109/TSE.1986.6312929) |
| 13 | DeLine, Fähndrich. Enforcing High-Level Protocols in Low-Level Software. PLDI 2001, pp. 59–69 | V | [abs] | [MSR](https://www.microsoft.com/en-us/research/?p=150228) |
| 14 | DeLine, Fähndrich. Typestates for Objects. ECOOP 2004, LNCS 3086:465–490 | V | [abs] | [MSR](https://www.microsoft.com/en-us/research/publication/typestates-for-objects/) |
| 15 | Fink, Yahav, Dor, Ramalingam, Geay. Effective Typestate Verification in the Presence of Aliasing. ISSTA 2006, pp. 133–144; TOSEM 2008 version | ISSTA V; TOSEM P (vol./issue/article not confirmed) | [abs] | [Technion abstract](https://csaws.cs.technion.ac.il/~yahave/papers/issta06.abs.htm) (search result; host blocked for fetch) |
| 16 | Bierhoff, Aldrich. Modular Typestate Checking of Aliased Objects. OOPSLA 2007, pp. 301–320 | V | [abs, secondary] | reference list in [arXiv 1205.5344](https://arxiv.org/pdf/1205.5344); [CMU page](https://www.cs.cmu.edu/~kbierhof/papers/techreports.html) |
| 17 | Joshi, Sen. Predictive Typestate Checking of Multithreaded Java Programs. ASE 2008, pp. 288–296 | V | [abs] | [IEEE Xplore](https://ieeexplore.ieee.org/document/4639332/); [ACM DL](https://dl.acm.org/citation.cfm?id=1642966) |
| 18 | Beckman, Bierhoff, Aldrich. Verifying Correct Usage of Atomic Blocks and Typestate. OOPSLA 2008, pp. 227–244 | V | [abs] | [Wisconsin TM bibliography](https://research.cs.wisc.edu/trans-memory/biblio/bibhtml/beckman_verifying_oopsla_2008.html) |
| 19 | Yang (Jason/Yue), Gringauze, Wu, Rohde. Detecting Data Race and Atomicity Violation via Typestate-Guided Static Analysis. MSR-TR-2008-108 | V (first author's given name P) | [abs] | [MSR](https://www.microsoft.com/en-us/research/?p=153267) |
| 20 | Crafa, Padovani. The Chemical Approach to Typestate-Oriented Programming. OOPSLA 2015; TOPLAS 39, 2017, DOI 10.1145/3064849 | V | [abs] | [HAL](https://hal.archives-ouvertes.fr/hal-01155682); [SPLASH 2015](https://2015.splashcon.org/details/oopsla2015/13/The-Chemical-Approach-to-Typestate-Oriented-Programming) |
| 21 | Gay, Gesbert, Ravara, Vasconcelos. Modular Session Types for Objects. LMCS 11(4:12), 2015 | V | [abs] | [LMCS](https://lmcs.episciences.org/1613) |
| 22 | Beckman, Nori. Probabilistic, Modular and Scalable Inference of Typestate Specifications. PLDI 2011, pp. 211–221 | V | [abs] | [MSR](https://www.microsoft.com/en-us/research/?p=160758) |
| 23 | Meier, Mover, Chang. Lifestate: Event-Driven Protocols and Callback Control Flow. ECOOP 2019, LIPIcs 134 | V | [abs] | [Dagstuhl](https://drops.dagstuhl.de/entities/document/10.4230/LIPIcs.ECOOP.2019.1) |
| 24 | Ammons, Bodík, Larus. Mining Specifications. POPL 2002, pp. 4–16 | V | [abs] | [EPFL Infoscience](https://infoscience.epfl.ch/entities/publication/0b08add4-69fd-4047-b341-21a3f909b253) |
| 25 | Whaley, Martin, Lam. Automatic Extraction of Object-Oriented Component Interfaces. ISSTA 2002 | V | [abs] | [MIT DSpace](https://dspace.mit.edu/handle/1721.1/149970) |
| 26 | Yang, Evans, Bhardwaj, Bhat, Das. Perracotta: Mining Temporal API Rules from Imperfect Traces. ICSE 2006 | V | [abs] | [UVA page](https://www.cs.virginia.edu/~evans/pubs/perracotta.html) |
| 27 | Wasylkowski, Zeller, Lindig. Detecting Object Usage Anomalies. ESEC/FSE 2007, pp. 35–44 | V | [abs] | [Saarland page](https://www.st.cs.uni-saarland.de/publications/details/wasylkowski-esec-2007/) |
| 28 | Weimer, Necula. Mining Temporal Specifications for Error Detection. TACAS 2005, LNCS 3440, pp. 461–476 | V | [abs] | [Springer](https://www.springerprofessional.de/doi/10.1007/978-3-540-31980-1_30) |
| 29 | Ramanathan, Grama, Jagannathan. Path-Sensitive Inference of Function Precedence Protocols. ICSE 2007, pp. 240–250 | V | [abs] | [CERIAS record](https://www.cerias.purdue.edu/apps/reports_and_papers/view/4087) |
| 30 | Ramanathan, Grama, Jagannathan. Static Specification Inference Using Predicate Mining. PLDI 2007 | V | [abs]; [snip] example | [Springer/ACM listing](https://www.springerprofessional.de/doi/10.1145/1250734.1250749) |
| 31 | Engler, Chen, Hallem, Chou, Chelf. Bugs as Deviant Behavior: A General Approach to Inferring Errors in Systems Code. SOSP 2001 | V | [abs] | [UW CSE 551 notes](https://courses.cs.washington.edu/courses/cse551/17wi/notes/mc.html) |
| 32 | Dwyer, Avrunin, Corbett. Patterns in Property Specifications for Finite-State Verification. ICSE 1999, pp. 411–420 | V bibliographic; pattern wording P | [snip] | [BibSonomy](https://bibsonomy.org/bibtex/10c6c1d1ea9dd6ac18039f4f8ef9065dd); [KSU scope page](https://people.cs.ksu.edu/~dwyer/SPAT/scopes.html) (search result only) |
| 33 | Ball, Rajamani. The SLAM Project: Debugging System Software via Static Analysis. POPL 2002, pp. 1–3 | V; "temporal safety properties" wording P | [abs] | [CPS-VO](https://archive.cps-vo.org/node/1616) |
| 34 | Ball, Rajamani. Automatically Validating Temporal Safety Properties of Interfaces. SPIN 2001 | V | [abs] | [DOI via Unpaywall](https://unpaywall.org/10.1007%2F3-540-45139-0_7) |
| 35 | Das, Lerner, Seigle. ESP: Path-Sensitive Program Verification in Polynomial Time. PLDI 2002 | V (pages P) | [abs] | [course copy](https://web.eecs.umich.edu/~weimerw/2011-6610/reading/dls-pldi02.pdf) |
| 36 | Lamport. Proving the Correctness of Multiprocess Programs. IEEE TSE SE-3(2):125–143, 1977 | V; quote P | [abs] | [MSR](https://www.microsoft.com/en-us/research/publication/proving-correctness-multiprocess-programs/) |
| 37 | Alpern, Schneider. Defining Liveness. IPL 21(4):181–185, 1985 | V | [abs] | [Cornell eCommons](https://ecommons.cornell.edu/entities/publication/2ed32f4f-cc5c-413b-ba16-5498641f1939) |
| 38 | Pnueli. The Temporal Logic of Programs. FOCS 1977, pp. 46–57 | V | [abs] | [IRISA bib](https://people.irisa.fr/Nicolas.Markey/biblio/focs1977-Pnu) |
| 39 | Chen, Roşu. MOP: An Efficient and Generic Runtime Verification Framework. OOPSLA 2007, pp. 569–588 | V (DOI not confirmed) | [abs] | [UIUC IDEALS](https://www.ideals.illinois.edu/items/11333) and a reference list |
| 40 | Chen, Roşu. Parametric Trace Slicing and Monitoring. TACAS 2009; ext. Roşu, Chen, LMCS 2012 | V | [abs] | [Springer](https://www.springerprofessional.de/doi/10.1007/978-3-642-00768-2_23); [LMCS](https://lmcs.episciences.org/710) |
| 41 | Meredith, Jin, Griffith, Chen, Roşu. An Overview of the MOP Runtime Verification Framework. STTT 14(3):249–289, 2012 | V | [abs] | [FSL page](https://fsl.cs.illinois.edu/publications/meredith-jin-griffith-chen-rosu-2011-jsttt.html) |
| 42 | Allan, Avgustinov, Christensen, Hendren, et al. Adding Trace Matching with Free Variables to AspectJ. OOPSLA 2005 | V | [abs] | [DOI](https://www.doi.org/10.1145/1094811.1094839) |
| 43 | Leucker, Schallhart. A Brief Account of Runtime Verification. JLAP 78(5):293–303, 2009 | V bibliographic | not retrieved | [Lübeck ISP](https://isp.uni-luebeck.de/node/453) |
| 44 | Hoare. An Axiomatic Basis for Computer Programming. CACM 12(10):576–580, 583, 1969 | V (page range varies) | [snip] | [Oxford listing](https://www.cs.ox.ac.uk/publications/publication8205-abstract.html) |
| 45 | Dijkstra. Guarded Commands, Nondeterminacy and Formal Derivation of Programs. CACM 18(8):453–457, 1975 | V | [abs, secondary] | [TU/e portal](https://research.tue.nl/en/publications/guarded-commands-nondeterminancy-and-formal-derivation-of-program/) |
| 46 | Meyer. Applying "Design by Contract". IEEE Computer 25(10):40–51, 1992 | V | [abs] (see T2) | [KTH copy](https://www.kth.se/social/files/59526bfb56be5b4f17000807/meyer-92-contracts.pdf) (search result) |
| 47 | Cousot, Cousot, Fähndrich, Logozzo. Automatic Inference of Necessary Preconditions. VMCAI 2013, LNCS 7737, pp. 128–148 | V | [abs]; [snip] | [NYU page](https://cs.nyu.edu/~pmc309/COUSOTpapers/VMCAI-13.shtml) |
| 48 | Honda, Vasconcelos, Kubo. Language Primitives and Type Discipline for Structured Communication-Based Programming. ESOP 1998, LNCS 1381:122–138 | V | [abs] | [Springer](https://link.springer.com/doi/10.1007/BFb0053567) |
| 49 | Honda, Yoshida, Carbone. Multiparty Asynchronous Session Types. POPL 2008, pp. 273–284 | V | [abs] | [Oxford MRG](https://mrg.cs.ox.ac.uk/publications/multiparty-asynchronous-session-types/) |
| 50 | Hüttel et al. Foundations of Session Types and Behavioural Contracts. ACM CSUR 49(1), art. 3, 2016 | V | [abs] | [ITU Pure](https://pure.itu.dk/en/publications/foundations-of-session-types-and-behavioural-contracts/) |
| 51 | Adamsen, Møller, Tip. Practical Initialization Race Detection for JavaScript Web Applications. PACMPL 1 (OOPSLA), art. 66, 2017 | V | [abs] | [Aarhus page](https://cs.au.dk/~amoeller/papers/initracer/) |
| 52 | Ferrante, Ottenstein, Warren. The Program Dependence Graph and Its Use in Optimization. TOPLAS 9(3):319–349, 1987 | V | [abs] | [MTU Digital Commons](https://digitalcommons.mtu.edu/michigantech-p/12533) |
| 53 | systemd.unit(5): After=/Before= vs Requires=/Wants= | P-doc (mirrors and tutorials only) | [snip] | [Debian mirror](https://manpages.debian.org/wheezy/systemd/systemd.unit.5.en.html); [Fedora Magazine](https://fedoramagazine.org/systemd-unit-dependencies-and-order/) |
| 54 | nasa/cFE #2663; nasa/CF #184 (issue bodies) | V | [seen] | GitHub MCP `search_issues` |

T2 verified, and this note relies on its verification without repeating it: SDV EuroSys 2006 (also V-abs in [05]), Polikarpova et al. ISSTA 2009, Wei et al. ICSE 2011, and the A/G and rely-guarantee references.

---

## 7. Open items

1. **Lu et al. 2008:** the exact wording of the order-violation definition. The full text is blocked; open the ACM DL PDF before quoting.
2. **Dwyer patterns:**
   - the exact precedence intent;
   - the "before R" vs. "after Q until R" semantics for runs without a shutdown (§2.5 caveat).
3. **Fink et al. TOSEM 2008:** volume, issue and article number. **MOP OOPSLA 2007:** the DOI.
4. **Joshi & Sen ASE 2008 and Lifestate:** read the full texts before claiming a difference from Kairos's prediction layer. Both are close prior art (§2.3).
5. **Korean terms:** check the Korean renderings against a terminology source (for example, the KIISE glossary) before using them in a Korean paper or report.
6. The FSW collision counts are whole-word greps over `cfe`, `osal`, `psp` and `apps` C sources, with unit tests excluded. They are indicators, not a lexical study.

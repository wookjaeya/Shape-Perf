# T2: Where "contract" comes from, and whether it fits Kairos's "readiness contract"

Scope: the PI asked whether "contract" (계약) is a valid term for Kairos's κ (SPEC_FINAL §1.6). This note traces how "contract" is used in software engineering and verification, compares each use with κ, and recommends wording. It reflects the spec as read on 2026-10-12 (§1.6, §1.10, §1.11, §2.3 examples, §3.15, §7.1–7.2). The spec was not edited. **Errata of 2026-10-15 are in §11** (marked "erratum En").

---

## 0. Bottom line

1. **Only part of κ is a contract in the established sense.**
   - "Contract" (Meyer's Design by Contract and everything after it) means a **specified** agreement. It has two parties: a client obligated to establish the precondition, and a supplier obligated to establish the postcondition. It assigns **blame**: a precondition violation is a client bug, a postcondition violation is a supplier bug.
   - κ is something else: an **analysis record**. It bundles a typestate precondition with these extra parts:
     - the supplier's documented error results (a separate, framework-level judgement since spec revision 1.1: erratum E1);
     - the *client's* continuation policy, which is often taken from the client's own code;
     - a harm label;
     - provenance;
     - a scope condition.
2. **Parts of κ that are contracts in the strict sense.**
   - `Req` when it comes from a cFE `\par Assumptions` block. Example: `cfe_evs.h` says *"The Application ID must correspond to a registered application or core service."* That is a natural-language precondition, and the obligation falls on the client.
   - Allowed entries that restate documented supplier behaviour: `\retval` codes, `documented_drop`, the documented `fallback` to `CFE_ES_WriteToSysLog`. These read like JML `exceptional_behavior` cases or Code Contracts `EnsuresOnThrow` clauses (erratum E1).
   - `scope`, the "no shutdown before the use" condition. This works like an assume-guarantee **assumption**, or a **rely** condition, that keeps readiness stable.
3. **Parts of κ that are not contracts.**
   - Allowed entries taken from the code under analysis (`retry_until_ready` with a bound read from CF's loop, `defer_by_design` from an app comment). These are client tolerance policies or inferred specifications. Calling them "contractual" invites a circularity objection.
   - `harm`. No contract tradition has it; it is a consequence classification (Portend-style).
   - The **cross-thread order** itself. Every documented cFS precondition we use is a *state* predicate at the call. The rule "the provider's transition must happen-before the use" is not written in any contract. Kairos derives it with HB and must-before analysis. SCOOP (wait conditions), rely-guarantee, and A/G contracts can express such things, but Kairos uses none of their semantics.
   - Instances whose Allowed is `unspecified_by_reference`, such as K-ROUTE subscribe-before-first-send. **No reference states the requirement**, so calling them contracts is a misnomer.
4. **Applied to the cases:**
   - **R2 (cFE #2663)** is a textbook contract violation (superseded: erratum E2). A documented precondition is violated by the client (SB code calling EVS with SB's not-yet-published AppId) in every boot.
   - **C1 (CF #184)** is *not* a violation of OSAL's contract. `OS_CountSemGetIdByName` documents `OS_ERR_NAME_NOT_FOUND` as a normal result (an `@retval`, neither normal nor an error: erratum E3). The readiness need is CF's own, code-admitted assumption about a peer app. It is closer to an unmet A/G *environment assumption* than to a DbC precondition.
   - **SB subscribe-before-first-send loss:** SB's documented contract is fulfilled ("transmitted to all subscribers", `CFE_SUCCESS`). No contract is violated (outside R2's window only: erratum E4).
5. **The AI paper (FSE 2014) is a useful contrast, not a competitor for the word.**
   - AI uses "invariant" in the "likely invariant" sense: something **learned from correct runs**.
   - R2's wrong order occurs in *every* run. So any run-learned invariant would absorb it as normal ([I], same argument the spec already makes for DefUse).
   - This is the strongest reason to keep a word that signals **external, documented reference**. "Contract" does that, but it overclaims for the code-derived and unreferenced parts.
6. **Recommendation (§9).** Rename the umbrella and keep "contract" only where documentation backs it.
   - κ becomes a **readiness specification**.
   - Its `Req` becomes a **readiness precondition** (a framework typestate precondition).
   - Each κ carries a provenance **tier**: `contract-backed` (API doc), `admitted` (code comment), `code-derived` (code pattern), or `unreferenced`.
   - Write "API contract" only for the documentation source.
   - If the PI prefers to keep "readiness contract", the minimum is an explicit definition paragraph (§9.3) plus per-tier reporting.

---

## 1. What κ is (from the spec)

`κ = ⟨id, U, sel, Req, Oracle, Allowed, harm, prov, authored, guard, scope⟩` (SPEC_FINAL §1.6).

| Component | Content | Who is it "about"? |
|---|---|---|
| `U`, `sel` | use pattern, resource key | the call site (client) |
| `Req` | conjunction of lifecycle/value predicates on the resource at the use's LP (typestate) | the resource state at the client's call; established by **another actor** (provider) |
| `Oracle` | the framework's own validity predicate, e.g. `EVS_GetAppDataByID` + `EVS_AppDataIsMatch` | the **supplier's** defensive check |
| `Allowed` | finite vocabulary: `none`, `documented_drop`, `returned_and_tested`, `retry_until_ready(codes,bound)`, `fallback`, `defer_by_design`, `exit`, `unspecified_by_reference` | mix: some entries are the supplier's documented results, some are the client's continuation (resolved in spec revision 1.1: erratum E1) |
| `harm` | `event_lost_silently`, `app_zombie`, … | the analyst's consequence class |
| `prov` | kind ∈ {`api_doc`, `code_admission`, `code_pattern`, `issue`, `researcher`}, `path:line`, hash | metadata |
| `scope` | "no SHUTDOWN_REQ LP precedes the use" | an assumption on the environment |

The three worked examples in §2.3 already cover every provenance tier:
- `K-ID.send-with-appid`: `api_doc` only (`cfe_evs.h:168–172`).
- `K-SELF.es`: `code_pattern` only (`cfe_es_api.c:312,463,…`).
- `K-REG.osal-countsem-lookup`: `api_doc` (OSAL `@retval`) plus `code_admission` (`cf_cfdp.c:1272–1278`).

§7.1 adds code-derived sources: retry-loop-derived contracts (source 4), lifecycle symmetry (5), and context-lookup symmetry (6). So the umbrella term covers documented, admitted, code-derived and unreferenced requirements.

**What the cited cFS text actually says** (local checkout, cFE v7.0.1 per `cfe_version.h`):

- `cfe/modules/core_api/fsw/inc/cfe_evs.h`, the `CFE_EVS_SendEventWithAppID` block:
  > "\par Assumptions, External Events, and Notes: The Application ID must correspond to a registered application or core service. For messages outside the context of a registered application (for example early in app initialization or if registration fails) #CFE_ES_WriteToSysLog can be used for reporting."

  Its retvals include `CFE_EVS_APP_NOT_REGISTERED` and `CFE_EVS_APP_ILLEGAL_APP_ID`. The supplier also checks defensively (`cfe_evs.c:185–188`: `EVS_GetAppDataByID(AppID) == NULL → CFE_EVS_APP_ILLEGAL_APP_ID`).
- `osal/src/os/inc/osapi-countsem.h:160`, for `OS_CountSemGetIdByName`:
  > "@retval #OS_ERR_NAME_NOT_FOUND if the name was not found in the table"

  There is **no** assumption that the name exists.
- `apps/cf/fsw/src/cf_cfdp.c:1272–1278`:
  > "There is a start up race condition because CFE starts all apps at the same time, and if this sem is instantiated by another app, it may not be created yet. Therefore if OSAL returns OS_ERR_NAME_NOT_FOUND, assume this is what is going on, delay a bit and try again."
- `CFE_SB_TransmitMsg` (`cfe_sb.h`):
  > "copies the specified message into a software bus buffer which is then transmitted to all subscribers"

  The retvals are `CFE_SUCCESS`, `BAD_ARGUMENT`, `MSG_TOO_BIG` and `BUF_ALOC_ERR`. There is no retval and no assumption about having zero subscribers.
- cFE's own code uses "contract" in the DbC (postcondition) sense:
  - `cfe/modules/core_api/ut-stubs/src/cfe_config_handlers.c:63`: "The public API has a contract _not_ to return a NULL pointer".
  - `cfe/modules/tbl/ut-coverage/tbl_ut_helpers.c:358–359`: "The real thing should be implemented per the contract …".

  So cFS developers would read "contract" as "what the API promises", i.e. a supplier obligation.
- 138 `\par Assumptions` blocks exist in `cfe/modules/core_api/fsw/inc/*.h` (re-counted; matches the spec).

---

## 2. Verification method and limits

- **Status tags:**
  - **V**: authors, title, venue and year confirmed by a search result or an official doc page.
  - **P**: partially confirmed (e.g. title only, or venue/pages inconsistent).
  - **UNVERIFIED**: not confirmed.
- **Access was limited, which affects how quotes can be used.**
  - WebFetch failed with DNS errors.
  - `curl` through the egress proxy got `403` for kth.se, yorku.ca, ilyasergey.net, uzh.ch, eiffel.org, arxiv.org, dl.acm.org, link.springer.com and others.
  - The Scholar Gateway returned `ACCESS_DENIED`.
  - So, apart from the AI paper (read in full from the uploaded PDF) and the local cFS sources, **no primary full text was read**.
  - Definitions below are marked by what I actually saw: **text** (read the primary text), **snippet** (an excerpt of the primary or an official doc shown in a search result), or **summary** (a search-tool summary or a secondary source).
  - Do not quote a "snippet" or "summary" item verbatim in the paper without opening the primary.

---

## 3. Lineage, entry by entry

For each entry: what a contract is; who is obligated; whether it can express temporal/ordering requirements across threads; whether it is specified or inferred.

### 3.1 Root: Hoare triples
- **Hoare, "An Axiomatic Basis for Computer Programming", CACM 12(10):576–580, 583, 1969.** [V]
- Pre/postcondition reasoning, the logical substrate of every later "contract". There are no parties and no blame. Not temporal. Specified.

### 3.2 Design by Contract (Meyer; Eiffel)
- **Meyer, "Applying 'Design by Contract'", IEEE Computer 25(10):40–51, Oct 1992.** [V]
  - The search results say DbC was "originally crafted for the Eiffel language". I saw only a summary, not the article text.
- **Meyer, *Object-Oriented Software Construction*, Prentice Hall, 1988; 2nd ed. 1997.** [V] Eiffel is the notation; DbC is a central topic.
- **What a contract is.** Preconditions, postconditions and class invariants attached to routines and classes. Seen as snippets of eiffel.org docs and Wikipedia:
  - "The client's obligation, which protects the supplier, is called a precondition. The client's benefit, which describes what the supplier must do (assuming the precondition was satisfied), is called a postcondition."
  - Class invariants "must be ensured by every creation procedure and maintained by every exported routine".
  - Obligations and benefits are cross-mapped. Eiffel's tutorial example is a homeowner and a telephone company.
- **Who is obligated.** The client must establish the precondition, and the supplier the postcondition and invariant. Blame (snippet, Eiffel tutorial): "a precondition violation signals a bug in the client … a postcondition (or invariant) violation signals a bug in the supplier".
- **Non-redundancy principle.** The supplier should *not* test its own precondition; this is set against defensive programming. I saw this only in lecture slides credited to Meyer, with wording "Under no circumstances shall the body of a routine ever test for the routine's precondition". Status [P]; quote the book or article, not the slides.
  - **Relevance:** cFE APIs are *defensive*. EVS documents the registration assumption **and** checks it and returns a code. In strict DbC that is a hybrid: either the condition is a precondition (the client's obligation, and the check is a redundant safety net), or it is part of a total specification (the error return is normal behaviour). The `\par Assumptions` heading supports the first reading for EVS. For OSAL lookups only the second reading exists.
- **Temporal / cross-thread?** No. Assertions are state predicates evaluated at call and return of a sequential routine.
- **Specified or inferred?** Specified by the programmer. Assertions are executable and checked at run time.

### 3.3 JML
- **Leavens, Baker, Ruby, "Preliminary Design of JML: A Behavioral Interface Specification Language for Java", ACM SIGSOFT SEN 31(3):1–38, 2006** (earlier as Iowa State TR 98-06). [V]
- **What it is.** A behavioral interface specification language. It takes Eiffel-style assertions and adds model-based features (quantifiers, model fields, frame conditions).
- **The point most relevant to κ:** JML separates heavyweight `normal_behavior` and `exceptional_behavior` specification cases. Seen in JML tutorial/manual snippets: "if P holds, then the exception is thrown" is written with `exceptional_behavior`. A spec can therefore make an error outcome *part of the contract* under a stated precondition.
  - This is the closest established analogue to Kairos's "documented not-ready outcomes". A documented `\retval` under a documented condition is an exceptional behaviour case of the *supplier's* contract.
- **Who is obligated.** As in DbC.
- **Temporal / cross-thread?** Not in core JML. Concurrency extensions exist; I did not verify them.
- **Specified or inferred?** Specified.

### 3.4 Code Contracts (.NET)
- **Fähndrich, Barnett, Logozzo, "Embedded Contract Languages", ACM SAC 2010 (OOPS track), pp. 2103–2110.** [V] Argues for contracts embedded as library calls in the host language.
- **Official docs** (Microsoft Learn, snippet) [V-doc]:
  - `Contract.Requires<TException>` makes a precondition failure throw a named exception.
  - `EnsuresOnThrow` is an **exceptional postcondition**.
  - **Legacy `if-then-throw` checks** followed by `Contract.EndContractBlock()` are "considered preconditions" by the tools.
  - **Relevance:** this is a precedent for reading a *defensive check in the supplier* as the *executable form of a precondition*. That matches what κ's `Oracle` field is (EVS's own `IsMatch` check). The analogy holds only loosely: C return codes are not exceptions.
- **Who is obligated.** As in DbC.
- **Temporal / cross-thread?** No.
- **Specified or inferred?** Specified. The static checker can also *infer* or suggest preconditions:
  - **Cousot, Cousot, Fähndrich, Logozzo, "Automatic Inference of Necessary Preconditions", VMCAI 2013, LNCS 7737:128–148.** [V] It infers *necessary* preconditions: "which precondition, if violated, guarantees the program is incorrect" (summary).

### 3.5 What C/C++ readers will bring: ACSL, C++26
- **ACSL (Frama-C) "function contracts".** `requires` / `ensures` / `assigns` with named `behaviors` guarded by `assumes`, plus `complete`/`disjoint` behaviors. [P] Seen in course slides and Frama-C API docs; I did not see the official ACSL manual.
  - The `behaviors` mechanism is again the "documented outcome per case" pattern.
  - Not temporal; no threads.
- **C++26 contracts (P2900: `pre`, `post`, `contract_assert`)** were adopted at the WG21 Hagenberg meeting, February 2025. [V; Herb Sutter trip report, snippet] A 2026 paper (P4043R0) questions whether they are ready to ship [snippet].
  - They are assertion-like pre/postconditions on functions; not temporal.
- **Effect on our audience.** cFS is C. Reviewers from embedded and flight software will hear "contract" as *per-function `requires`/`ensures` annotations*. κ is written per API and resource kind, with selectors and a lifecycle model; it is closer to SDV's "API usage rules" (§3.13).

### 3.6 Contracts in concurrency: SCOOP
- **Nienaltowski, Ostroff, Meyer, "Contracts for Concurrency", Formal Aspects of Computing 21(4):305–318, Aug 2009, DOI 10.1007/s00165-007-0063-2.** [V; author order per Meyer's publication list]
  - Abstract (snippet): SCOOP "is based on the principles of Design by Contract". The original semantics of contracts "is not suitable for concurrent programming because it restricts parallelism", so the paper proposes "a new contract semantics that applies equally well in concurrent and sequential contexts".
- **Meyer, "Systematic Concurrent Object-Oriented Programming", CACM 36(9):56–80, 1993.** [V] The original SCOOP proposal.
- **Wait conditions.** I saw these only in eiffel.org "Design by Contract in SCOOP" and Wikipedia snippets, not in the FAC paper text:
  - In SCOOP "preconditions can additionally take the role of a wait condition".
  - A precondition involving a separate object "is re-interpreted as a wait condition": the routine body is not entered until it holds, because a check made before the call "may have [been] invalidated" by another processor.
  - Attribute this to SCOOP generally [V-doc]. Attribute it to the FAC 2009 paper specifically only at [P] until the text is read.
- **Who is obligated.**
  - In sequential Eiffel, the client.
  - In SCOOP, for separate preconditions, the obligation turns into *waiting by the runtime*. The client's duty becomes "eventually some other processor makes it true".
- **Temporal / cross-thread?** **Yes, in one specific form.** "Proceed only after condition C over another processor's object holds". This is exactly a *readiness* condition.
  - cFS has no such semantics.
  - CF's 25 × delay retry loop is a **hand-written, bounded SCOOP-style wait condition** [I].
  - cFE's `WaitForStartupSync` / `WaitForSystemState` with a timeout is a soft, framework-level variant [I].
- **Specified or inferred?** Specified.
- **Fit.** This is the best literature anchor for "a precondition whose establishment belongs to another thread". It supports keeping "precondition" in the vocabulary. It does not license calling the client's retry policy a contract.

### 3.7 Rely-guarantee (the concurrent analogue of pre/post)
- **Jones, "Tentative Steps Toward a Development Method for Interfering Programs", ACM TOPLAS 5(4):596–619, 1983, DOI 10.1145/69575.69577.** [V]
  - Seen in summary/snippets of secondary sources, not the paper text: a **rely** condition is a two-state relation that every environment step is assumed to satisfy. A **guarantee** is a two-state relation that every step of the component must satisfy. One thread's guarantee must imply the others' relies.
  - A rely "record[s] assumptions … about any interfering state transitions from the environment, much as preconditions do for starting states" (snippet attributed to Jones's notes) [P].
- **Who is obligated.** Each component: it relies on others and guarantees to others.
- **Temporal / cross-thread?** Interference is two-state relations, i.e. *safety over steps*. Monotonic lifecycle facts ("once REG, stays REG until delete") are natural rely conditions. Specific event orders need auxiliary state.
- **Specified or inferred?** Specified (proof method).
- **Mapping to κ** [I]:
  - `scope` ("no shutdown request before the use") works as a **rely** that keeps `Req` stable.
  - The provider's lifecycle effects are its **guarantee**.
  - `Req` is a precondition that becomes stable once established.
  - Kairos uses no rely-guarantee proof rules; HB and must-before certificates replace them.

### 3.8 Assume-guarantee contracts for system design (incl. aerospace)
- **Benveniste, Caillaud, Nickovic, Passerone, Raclet, Reinkemeier, Sangiovanni-Vincentelli, Damm, Henzinger, Larsen, "Contracts for System Design", Foundations and Trends in Electronic Design Automation 12(2–3):124–400, 2018, DOI 10.1561/1000000053.** [V]
  - Contains a meta-theory of contracts (components, contracts, refinement, conjunction, parallel composition, quotient) and assume/guarantee contracts. These are synchronous, asynchronous dataflow, and cyber-physical. I saw the TOC and abstract only; exact definitions [P].
- **Sangiovanni-Vincentelli, Damm, Passerone, "Taming Dr. Frankenstein: Contract-Based Design for Cyber-Physical Systems", European Journal of Control 18(3):217–238, 2012.** [V]
- **Cofer, Gacek, Miller, Whalen, LaValley, Sha, "Compositional Verification of Architectural Models", NFM 2012, LNCS 7226:126–140, DOI 10.1007/978-3-642-28891-3_13.** [V] This is AGREE for AADL.
  - Snippet: "guarantees correspond to the component requirements"; "Assumptions correspond to the environmental constraints that were used in verifying the component requirements".
- **Backes, Cofer, Miller, Whalen, "Requirements Analysis of a Quad-Redundant Flight Control System", NFM 2015** (arXiv 1502.03343). [V]
  - Assume-guarantee contracts on the software components of NASA's Transport Class Model QFCS, verified compositionally with AGREE.
  - One repository record lists Gacek as an additional author; the arXiv and Springer listings do not.
- **Cimatti, Tonetta, "Contracts-Refinement Proof System for Component-Based Embedded Systems"** (OCRA; temporal-logic contracts, refinement checked by SMT/NuSMV). [P] Title and content seen; venue (SCP 2015) not confirmed.
- **What a contract is.** A pair (A, G) over component *behaviours (traces)*: what the component assumes of its environment and what it guarantees under that assumption.
- **Who is obligated.** The component, for G. The environment, i.e. the other components, for A. Composition checks that one component's guarantees discharge another's assumptions.
- **Temporal / cross-thread?** **Yes.** Contracts are trace properties: Lustre-style synchronous in AGREE, LTL in OCRA.
- **Specified or inferred?** Specified, at the architecture level, by designers.
- **Fit.**
  - A/G is the tradition in which "contract" *does* carry temporal, cross-component meaning. Aerospace reviewers know it through AGREE.
  - C1 fits A/G better than DbC [I]: CF *assumes* its environment creates the semaphore within 25 × delay, and no component *guarantees* it. That is an A/G compatibility gap.
  - But Kairos provides **no A/G semantics**: no refinement, no composition, no compatibility check. A reviewer from this community will expect them if the word is "contract".

### 3.9 Interface automata (temporal interfaces)
- **de Alfaro, Henzinger, "Interface Automata", ESEC/FSE 2001, pp. 109–120.** [V]
  - Summary: automata model "input assumptions about the order in which the methods of a component are called, and output guarantees about the order in which the component calls external methods".
  - They are not called contracts there, but they are the temporal ancestor of A/G interface theories.
- **Temporal / cross-thread?** Yes, between a component and its environment (two-party call order).
- **Specified or inferred?** Specified.

### 3.10 Behavioural contracts for services
- **Castagna, Gesbert, Padovani, "A Theory of Contracts for Web Services", ACM TOPLAS 31(5), 2009, DOI 10.1145/1538917.1538920** (preliminary version POPL 2008). [V]
  - Abstract (summary): contracts are behavioural descriptions of services. *Compliance* means a client and a service can always complete their interaction. *Subcontracting* means a service can replace another. Filters coerce behaviours. It builds on the must-testing preorder.
- **Who is obligated.** Both parties to the interaction protocol.
- **Temporal / cross-thread?** Yes, the order of interactions between two parties. There are no shared-state readiness predicates.
- **Specified or inferred?** Specified.
- **Fit.** Weak. Our order is between a provider actor and a consumer actor that never interact through the API in question (SB task vs SB code running in ES/EVS tasks).

### 3.11 "Contracts for concurrency" as usage protocols
- **Fiedor/Dias, Ferreira, Lourenço, Smrčka, Sousa, Vojnar, "Verifying Concurrent Programs Using Contracts", ICST 2017, pp. 196–206, DOI 10.1109/ICST.2017.25.** [V; author order differs across records]
  - Abstract (summary): "contracts for concurrency" capture "the expected atomicity of sequences of method or service calls". Checked statically and dynamically.
- **Fit.** This shows "contract" is already used for *concurrency usage protocols* on library APIs, with an atomicity flavour, not readiness. A precedent for the extended use, but a different property.

### 3.12 API documentation as the source: "API contracts", directives, usage constraints
- **Monperrus, Eichberg, Tekes, Mezini, "What Should Developers Be Aware Of? An Empirical Study on the Directives of API Documentation", Empirical Software Engineering 17(6):703–737, 2012, DOI 10.1007/s10664-011-9186-4.** [V]
  - Directive (abstract, snippet): "natural-language statements that make developers aware of constraints and guidelines related to the correct and optimal usage of an API".
  - A taxonomy of 23 directive kinds from >6,000 Java API documentation elements [V for counts per abstract].
  - The search results also surfaced a kind called **"Non Call-based State"** ("application-state requirements that cannot be expressed as a method call sequence") and a synchronization-related category [P; snippets of the author PDF and a summary; I did not see the full list].
  - **Relevance:** cFE `\par Assumptions` blocks are *directives* in this sense. Monperrus's vocabulary ("directive", "usage constraint") is the empirical-SE name for documented API obligations. It avoids the formal baggage of "contract".
- **Zhou, Gu, Chen, Huang, Panichella, Gall, "Analyzing APIs Documentation and Code to Detect Directive Defects", ICSE 2017, pp. 27–37, DOI 10.1109/ICSE.2017.11.** [V]
  - Checks directives on parameter constraints and exceptions against code with a constraint solver. Here directives are "API usage constraints".
- **Saied, Sahraoui, Dufour, "An Observational Study on API Usage Constraints and Their Documentation", SANER 2015, pp. 33–42.** [P; confirmed via a citing reference list and a summary]
  - Several kinds of usage constraint are common, and three are generally undocumented.
- **Dietrich, Pearce, Jezek, Brada, "Contracts in the Wild: A Study of Java Programs", ECOOP 2017, LIPIcs 74, 9:1–9:29.** [V] An empirical study of lightweight contract checking (preconditions etc.) in 200 Maven projects.
- **Estler, Furia, Nordio, Piccioni, Meyer, "Contracts in Practice", FM 2014.** [P] The arXiv entry 1211.4775 with this title was confirmed; authors and venue were not confirmed by a search result.
- **Who is obligated.** The client (the directive is addressed to the API user).
- **Temporal / cross-thread?**
  - Directives can be about call order (sequence) or state.
  - Cross-thread requirements appear in natural language at most ("external synchronization").
  - cFE's EVS assumption is a pure *state* directive. Its temporal side ("early in app initialization") appears only as a hint toward the fallback.
- **Specified or inferred?** Specified, in natural language.

### 3.13 Neighbouring concepts that do *not* use the word "contract"
These are often more exact names for parts of κ.

- **Typestate.** Strom, Yemini, "Typestate: A Programming Language Concept for Enhancing Software Reliability", IEEE TSE 12(1):157–171, 1986, DOI 10.1109/TSE.1986.6312929. [V]
  - Abstract (summary): typestate refines type to the operations allowed *at a program point*. It targets errors such as use before initialization.
  - This is exactly κ's `Req` (a lifecycle state required at the use).
  - **Spec note:** SPEC_FINAL §1.11 lists "Typestate contracts | Strom & Yemini 1986". "Typestate contract" is *our* phrase. I verified only the title and abstract, which speak of typestate, so the row should read "Typestate" [I].
- **Object typestate with method pre/post specifications.** DeLine, Fähndrich, "Typestates for Objects", ECOOP 2004, LNCS 3086:465–490 (Fugue). [V] Typestate pre- and postconditions on methods, i.e. typestate as part of method specifications. "Typestate precondition" is an established phrase-level fit for `Req` [I].
- **Object protocols.** Beckman, Kim, Aldrich, "An Empirical Study of Object Protocols in the Wild", ECOOP 2011, pp. 2–26. [V] Restrictions on method call order on one object; 7.2% of types define protocols (summary). Single-object, not cross-thread.
- **Framework lifecycle protocols.** Meier, Mover, Chang, "Lifestate: Event-Driven Protocols and Callback Control Flow", ECOOP 2019, LIPIcs 134, 1:1–1:29. [V]
  - A specification language for the protocol an app must follow *and* the callback control flow the framework imposes ("lifecycle constraints with typestate rules"), checked dynamically on traces (summary).
  - **This is the closest prior work in *shape* to κ:** framework lifecycle × typestate × trace checking. They call it a *specification*, not a contract.
- **Thread-context preconditions.** Hutchins, Ballman, Sutherland, "C/C++ Thread Safety Analysis", SCAM 2014. [V]
  - Clang docs (snippet) [V-doc]: `REQUIRES(mu)` declares that the caller must hold the capability. This is a concurrency-state precondition on a function, which Clang calls an *annotation* or *attribute*, not a contract.
- **API usage rules.** Ball, Bounimova, Cook, Levin, Lichtenberg, McGarvey, Ondrusek, Rajamani, Ustuner, "Thorough Static Analysis of Device Drivers", EuroSys 2006, pp. 73–85. [V]
  - SDV ships "over sixty API usage rules" with an OS environment model (abstract snippet). Rules are written in SLIC [P; Wikipedia only].
  - This is the closest precedent for *per-API rules plus a framework environment model in C*.
- **API property inference.** Robillard, Bodden, Kawrykow, Mezini, Ratchford, "Automated API Property Inference Techniques", IEEE TSE (online 2012, issue 2013). [V; volume and pages not confirmed]
  - Surveys over 60 techniques that infer "API properties": usage patterns, sequential patterns, behavioural specifications.

### 3.14 Inferred contracts and invariants (the learned side)
- **Ernst, Cockrell, Griswold, Notkin, "Dynamically Discovering Likely Program Invariants to Support Program Evolution", IEEE TSE 27(2):99–123, 2001** (Daikon; earlier ICSE 1999). [V] Likely invariants inferred from execution traces; accuracy depends on the test suite.
- **Ammons, Bodík, Larus, "Mining Specifications", POPL 2002, pp. 4–16.** [V] Protocol specifications learned from traces.
- **Polikarpova, Ciupa, Meyer, "A Comparative Study of Programmer-Written and Automatically Inferred Contracts", ISSTA 2009, pp. 93–104.** [V]
  - Daikon finds about 5× more relevant clauses than programmers write, but only about 60% of the programmer-written ones. The conclusion: inference strengthens written contracts but cannot replace them.
- **Wei, Furia, Kazmin, Meyer, "Inferring Better Contracts", ICSE 2011, pp. 191–200.** [V]
- **Takeaway.** "Contract" *is* used for inferred artefacts, but always qualified as an "inferred contract". The DbC community itself separates programmer-written from inferred contracts. Kairos's `code_pattern` tier is the inferred kind and should be labelled that way.

### 3.15 The PI's paper: AI (Anticipating Invariant)
- **Zhang, Wu, Lu, Qi, Ren, Zheng, "AI: A Lightweight System for Tolerating Concurrency Bugs", FSE 2014, pp. 330–340, DOI 10.1145/2635868.2635885.** [V; read in full from the uploaded PDF]
- **Journal extension:** Zhang et al., "A Lightweight System for Detecting and Tolerating Concurrency Bugs", IEEE TSE 42(10):899–917, 2016, DOI 10.1109/TSE.2016.2531666. [V]
- **What the paper says** (text, seen):
  - p. 331–332: "Program invariants are predicates that should always be true at certain points of the execution; they reflect programmers' intentions. Many recent works pay close attention to learn 'likely invariants' from testing runs … These invariants are supposed to be held in all the correct runs."
  - p. 332, definition: for dynamic instruction `IxSy`, the *remote predecessor* `RPre(Ix)` is the most recent preceding access to the same address from another thread, or `nil`. The *belonging set* is `BSet(Sy) = ∪ RPre` over verified interleavings. **AI:** `RPre(IxSy) ∈ BSet(Sy)`.
  - p. 334: "In order to infer AI automatically without any programmers' annotation, we rely on correct runs observed during the in-house testing phase."
  - On a violation, the thread is **stalled** up to a threshold, then resumed and logged.
  - p. 339 (§4.5): "Similar to all the other invariant-based techniques, AI needs sufficient tested execution traces … false negatives … if some shared-memory accesses are not identified during training".
- **Mapped to the four questions:**
  - *What it is:* a per-instruction set of permitted cross-thread predecessors.
  - *Who is obligated:* nobody; there are no parties.
  - *Temporal / cross-thread:* **yes**. It is purely about cross-thread order of memory accesses.
  - *Specified or inferred:* **inferred from runs judged correct.**
- **Why it matters for the term** [I]:
  1. AI chose "invariant" because the property is *empirical*: whatever held in all correct runs (a motive the paper does not state: erratum E5).
  2. R2's wrong order appears in every boot (24 uses per boot, 3/3 with gdb). Its harm, an event lost silently, does not change outputs that a tester would judge. So R2 runs would be "correct runs", and the EarlyInit predecessor would enter `BSet`. AI would never fire. The spec makes the same argument for DefUse (§1.11).
  3. Any term for κ must therefore signal **"external reference, not learned from runs"**. "Contract" signals that well. "Invariant" would signal the opposite, and it also collides with DbC class invariants (overstated: erratum E5).
  4. AI's stall mechanism is a run-time wait-until-ordered. It is conceptually akin to SCOOP's wait conditions and to CF's retry loop. All three "tolerate" a not-ready state by waiting. Kairos's `retry_until_ready` / `TOLERATED` is the classification side of the same idea.
- **Related "intention" wording.** Lu, Park, Seo, Zhou, "Learning from Mistakes: A Comprehensive Study on Real World Concurrency Bug Characteristics", ASPLOS 2008. [V for bibliographic data]
  - Abstract (snippet): about one third of non-deadlock bugs are "caused by violation to programmers' order intentions". [P for wording]
  - The concurrency-bug community therefore speaks of *intentions* and *invariants*, not contracts (not a census of the word: erratum E6).

### 3.16 cFS and flight-software use
- **No published work applying DbC, ACSL or A/G "contracts" to cFS was found** (one extended search; a negative result, not proof of absence).
- The nearest is NASA's FRET → Ogma → Copilot pipeline. It generates cFS monitor applications from temporal-logic **requirements** [P; NTRS and Hackage snippets]. That community says "requirements" and "monitors", not "contracts".
- Aerospace use of "contract" is predominantly A/G (AGREE/QFCS) and SPARK-style subprogram contracts. For SPARK: `Pre`/`Post` plus `Global`/`Depends` flow contracts [P; AdaCore tutorial snippets; no avionics deployment confirmed in this search].

---

## 4. Summary matrix

| Tradition | What a contract is | Who is obligated | Cross-thread temporal order expressible? | Specified or inferred | Status |
|---|---|---|---|---|---|
| Hoare 1969 | pre/post triple | — | no | specified | V |
| DbC / Eiffel (Meyer 1992; OOSC) | pre, post, class invariant; blame | client: pre; supplier: post/inv | no | specified (executable) | V |
| JML (Leavens et al. 2006) | behavioral interface spec; `normal_`/`exceptional_behavior` cases | as DbC | no (core) | specified | V |
| Code Contracts (SAC 2010) | embedded pre/post; `EnsuresOnThrow`; legacy if-throw = precondition | as DbC | no | specified; checker can infer (VMCAI 2013) | V |
| ACSL; C++26 P2900 | function contracts / `pre`,`post` | as DbC | no | specified | P; V |
| SCOOP (CACM 1993; FAC 2009) | DbC with separate preconditions as **wait conditions** | client, then runtime waits | **yes, "wait until C holds"** | specified | V (wait semantics V-doc, FAC-specific P) |
| Rely-guarantee (Jones 1983) | pre, rely, guarantee, post | each component, mutually | partly (two-state interference; stability) | specified | V (definitions P) |
| A/G contracts (Benveniste 2018; AGREE 2012; QFCS 2015; OCRA) | (assumption, guarantee) over traces; refinement/composition | component for G; environment for A | **yes, temporal over traces** | specified (architecture) | V (OCRA P) |
| Interface automata (2001) | input assumptions / output guarantees on call order | component and environment | yes, two-party | specified | V |
| Behavioural contracts (Castagna 2009) | service behaviour; compliance, subcontracting | both parties | yes, two-party interaction order | specified | V |
| Contracts for concurrency (ICST 2017) | call sequences that must run atomically | client of a library | atomicity of sequences | specified | V |
| API directives / usage constraints (EMSE 2012; ICSE 2017; SANER 2015) | natural-language usage obligations | API client | sometimes (sequence, state; sync in prose) | specified (docs) | V; V; P |
| Typestate (TSE 1986; ECOOP 2004); object protocols (ECOOP 2011) | allowed operations per state; call-order protocols | client | single-object order, not cross-thread | specified | V |
| Lifestate (ECOOP 2019) | framework lifecycle + callback protocol spec, trace-checked | app vs framework | yes, framework/app event order | specified | V |
| Clang TSA `REQUIRES` (SCAM 2014) | lock-held precondition annotation | caller | concurrency *state*, not order | specified | V |
| SDV API usage rules (EuroSys 2006) | per-API rules + environment model | driver (client) | rule automata over call sequences | specified | V |
| Daikon; spec mining; inferred contracts | likely invariants / inferred specs or contracts | — | some (protocols) | **inferred** | V |
| **AI (FSE 2014)** | **invariant**: permitted remote predecessors | — | **yes** (cross-thread access order) | **inferred from correct runs** | V (text read) |
| **Kairos κ** | typestate precondition + supplier outcomes + client tolerance + harm + provenance + scope | mixed (see §5) | **not in κ; derived by HB/must-before** | **mixed: documented, admitted, code-derived, unreferenced** | — |

---

## 5. Component-by-component fit of κ

| κ component | Closest established notion | Is it "contract"? |
|---|---|---|
| `Req` from `api_doc` (`\par Assumptions`) | DbC precondition, as a typestate precondition (Fugue) or a "Non Call-based State" directive | **Yes**: client obligation, documented |
| `Req` from `code_pattern` / `code_admission` | inferred or necessary precondition (Cousot et al.); client's own assumption | only as an "inferred contract"; better "inferred precondition" |
| `Oracle` | the supplier's defensive check, i.e. the executable form of the precondition (cf. Code Contracts legacy if-throw) | yes, as the *supplier-side check of* the precondition |
| `Allowed = none` | DbC: precondition must hold, no tolerated failure | yes |
| `documented_drop`, `returned_and_tested(codes)` (the code part), documented `fallback`, `exit` | supplier's exceptional behaviour (JML `exceptional_behavior`, ACSL behaviors, `EnsuresOnThrow`) or documented directive | **yes**, when sourced from docs |
| `returned_and_tested` (the "caller tests and continues" part) | client usage directive (cf. `cfe_sb.h` ReceiveBuffer: "recommended that the return code be tested") | directive-level; contract-like only if documented |
| `retry_until_ready(codes, bound)` with a bound from client code | client tolerance policy; hand-written bounded SCOOP wait | **no**; it is code-derived, and checking code against a policy taken from the same code is circular unless labelled |
| `defer_by_design(src)` from an app comment | app design admission | no (admission) |
| `unspecified_by_reference` | absence of any contract | **no**; the instance has no contract |
| `harm` | consequence classification (Portend-style) | no |
| `scope` (no shutdown before use) | A/G assumption / rely condition | yes, in the A/G sense |
| `prov`, `authored`, `guard` | traceability metadata | n/a |
| HB order between provider and use | *not in κ*; rely-guarantee, SCOOP or A/G could express it | κ does not contain it |

**Two internal collisions in the spec:**

1. **"API contract" in §1.4.** M_api is "a hypothetical OSAL backend that conforms atomically to the OSAL API contract". This is the *supplier semantics* sense, which is correct DbC usage. κ is a *usage requirement* (client-side precondition plus extras). The spec therefore uses "contract" in two senses.
2. **K-LOCKBAL** ("the API returns with the type lock released", derived from the header invariant `osapi-idmap.c:384–385`). This is a **supplier postcondition**. Its violation is an OSAL implementation bug, not a readiness violation. It is the most DbC-pure "contract" in the library, yet it is not about readiness.

---

## 6. The three cases under each reading

| Case | DbC reading | A/G or rely reading | κ today | Is "contract violation" accurate? |
|---|---|---|---|---|
| **R2** (cFE #2663; 24 uses per boot, observed 3/3) | Client (SB code running in ES/EVS task contexts) passes `CFE_SB_Global.AppId` before SB's task publishes it. This violates the documented precondition "must correspond to a registered application". The supplier's defensive check returns `CFE_EVS_APP_ILLEGAL_APP_ID`. The doc names the fallback (`CFE_ES_WriteToSysLog`) that the client did not use. | The provider (SB task) guarantees publication only *after* the uses. HB/must-before shows every execution is bad (`fixed-order`). | `K-ID.send-with-appid`, `api_doc`, `allowed none` | **Yes.** A textbook precondition violation, client's fault, in every execution. "Contract" is fully justified here, and the AI contrast (§3.15) shows why a reference outside the runs is needed. |
| **C1** (CF #184; 25 × delay retry around `OS_CountSemGetIdByName`) | OSAL's contract is total: `OS_ERR_NAME_NOT_FOUND` is a documented normal result (an `@retval`: erratum E3), with no precondition that the name exists. **No OSAL contract is violated**, even when the retries run out. | CF *assumes* (its own comment says "assume this is what is going on") that some peer app creates the semaphore within its retry budget. No component *guarantees* that. This is an A/G compatibility gap, tolerated by a hand-written bounded wait. | `K-REG.osal-countsem-lookup`, `api_doc` (retvals) + `code_admission`, `retry_until_ready` | **No.** The requirement is a client-admitted environment assumption with a code-derived tolerance. "Readiness assumption" or "inferred requirement" is accurate; "contract" overstates who promised what. |
| **SB subscribe-before-first-send** (silent loss) | `CFE_SB_TransmitMsg` fulfils its documented contract: it delivers to "all subscribers" (here none) and returns `CFE_SUCCESS`. No documented assumption requires a subscriber. | An end-to-end delivery expectation with no stated owner. | K-ROUTE; `unspecified_by_reference` → LOSS-UNDER-ORDER | **No.** There is no contract to violate. The spec already labels it correctly (LOSS-UNDER-ORDER); the umbrella word "contract" contradicts that label. |

---

## 7. Verdict: in what precise sense is a "readiness contract" a contract?

- **It is a contract** when its precondition and its tolerated outcomes are **documented by the framework**:
  - a `\par Assumptions` precondition (the client's obligation);
  - plus `\retval` / documented drops and fallbacks (the supplier's exceptional behaviour);
  - plus the shutdown scope (an A/G-style assumption).
  - In this sense κ is a **typestate-lifted API contract**, i.e. DbC with lifecycle state in the precondition. Its violation assigns blame to the client, exactly as DbC does. R2 is the exemplar.
- **It is not a contract:**
  - when the requirement is the client's own **assumption** (C1);
  - when its tolerance is **read from the code under analysis** (`retry_until_ready` bounds, K-SELF from `code_pattern`, the §7.1 sources 4–6);
  - when **no reference exists** (K-ROUTE, K-CAP with `unspecified_by_reference`).
  - In those cases κ is an *inferred* or *hypothesised readiness specification*. The literature names this "inferred contract", "inferred precondition" or "specification".
- **The cross-thread temporal part is never in the contract.** It lives in the analysis: HB, must-before certificates, prediction. Of the traditions that *can* express it:
  - SCOOP would turn the precondition into a wait;
  - rely-guarantee would require stability under interference;
  - A/G would state it as a trace assumption.

  Kairos borrows none of their semantics. It can say so precisely: **"a state precondition whose establishment is another actor's obligation"**. That is the SCOOP and rely-guarantee situation, without their proof systems.

---

## 8. Risks of reviewer confusion (ranked)

1. **Blame semantics (DbC readers).** "Contract violation" means "client bug" to DbC readers. For C1 and K-ROUTE that assigns blame where no documented promise exists. A reviewer can ask: "OSAL returned a documented result; which contract was violated?"
2. **"The API already checks its contract."** EVS checks registration defensively and returns a code, so a reviewer may ask what Kairos adds. The contribution has to be stated as **order classification** (fixed-order vs schedule-dependent, with certificates) plus **outcome and harm judgement**, not "contract checking". (This is a framing risk that the word "contract" invites.)
3. **Circularity.** Allowed entries derived from the very code being checked (retry bounds, K-SELF) make conformance hold by construction. "Contract" hides this; a tiered name exposes it honestly. §7.2's provenance and refinement accounting already help, but the word works against them.
4. **Formal-methods expectations (A/G and rely-guarantee readers, incl. NASA formal methods).** "Contract" in that community implies refinement, composition and compatibility checking (AGREE, OCRA, Benveniste et al.). Kairos has none of these.
5. **Runtime-assertion expectations (Eiffel/JML/Code Contracts/C++26/ACSL readers).** They expect per-function, executable, in-program annotations. κ is per API and resource kind, evaluated externally over traces at linearization points. It looks more like SDV "API usage rules" or Lifestate "specifications".
6. **Internal double meaning.** §1.4 "API contract" (supplier semantics, M_api) and κ "readiness contract" (usage requirement) appear in one paper. K-LOCKBAL is a supplier postcondition filed under the readiness library.
7. **Learned vs specified (the AI/DefUse audience).** "Contract" correctly signals "specified, not learned". But if the census shows most instances are code-derived (§7.1 sources 4–6), a reviewer will call them inferred specifications anyway. The tier counts must be reported.
8. **Keyword noise.** In this survey, a query mixing "contract", "locking" and "state" (with the Monperrus arXiv id) returned mostly smart-contract/blockchain results and none on API directives, and "behavioral contract" returned psychology. This matters for discoverability and reviewer bidding, and argues for always qualifying the word ("API contract", "readiness precondition").

---

## 9. Terminology options and recommendation

### 9.1 Candidates and collisions checked against the spec

| Candidate for κ | Pros | Cons / collisions |
|---|---|---|
| keep **readiness contract** | short; signals external reference; R2 fits perfectly | risks 1–7 above; contradicts `unspecified_by_reference` instances |
| **readiness specification** | standard term in spec mining, Lifestate and JML ("behavioral interface specification"); covers documented and inferred; no blame or A/G baggage | longer; the abbreviation "spec" collides with the SPEC_FINAL document name (write "readiness spec" in full, or call it RS) |
| readiness requirement | neutral; FRET/AGREE use "requirements" | collides with the `Req` field name |
| readiness assumption | matches cFE's `\par Assumptions` and CF's "assume" | **collides with §6.1 "Assumptions A" (the soundiness manifest)**; flips the obligation (an assumption is what the analysis takes for granted) |
| readiness rule | SDV precedent | collides with HB rules R1–R19 |
| readiness obligation | DbC flavour | collides with "proof obligation" and "candidate obligations" (§4) |
| readiness protocol | typestate/protocol precedent | suggests call-sequence protocols on one object (Beckman et al.) |
| readiness invariant | matches the AI paper | wrong: AI/Daikon "invariant" means learned (overstated; the reason is that κ is a specification object: erratum E5); DbC "invariant" means a class invariant |

### 9.2 Recommendation
1. **Umbrella:** call κ a **readiness specification**, with "contract" kept only in the tier name (point 3).
2. **Field names:** call `Req` a **readiness precondition**, defined as a framework typestate precondition, with Strom & Yemini 1986 and DeLine & Fähndrich 2004 as anchors. Split `Allowed` into:
   - **documented outcomes**: supplier-side, from `\retval`, documented drops and fallbacks;
   - **client tolerance**: from code, e.g. `retry_until_ready` with its effective bound.
3. **Tier field** (derived from `prov`, reported per verdict and per census count):
   - `contract-backed`: Req and Allowed from `api_doc`;
   - `admitted`: `code_admission`;
   - `code-derived`: `code_pattern`;
   - `issue-derived`;
   - `researcher`;
   - `unreferenced`: Allowed = `unspecified_by_reference`.

   Only `contract-backed` violations should be called **API contract violations** in the paper.
4. **Reserve "API contract"** for documented supplier semantics:
   - the M_api model in §1.4 ("conforms to the OSAL API contract");
   - documented preconditions and retvals.
5. **Re-file K-LOCKBAL** as a *supplier postcondition* check (implementation-defect family), apart from readiness.
6. **Fix the §1.11 row** "Typestate contracts | Strom & Yemini 1986" to "Typestate | Strom & Yemini, TSE 12(1) 1986 [V]". Add AI (FSE 2014 [V]; TSE 2016 [V]) next to DefUse as a run-learned order invariant that would absorb R2 [I].
7. **Korean renderings** (suggestions; the standard Korean term for DbC, 계약에 의한 설계, was not confirmed by a Korean source in this search, so UNVERIFIED):
   - κ: 준비 상태 명세 (readiness specification)
   - Req: 준비 사전조건
   - contract-backed: API 계약 근거
   - client tolerance: 클라이언트 허용 정책

### 9.3 If the PI keeps "readiness contract": a definition paragraph to paste into the paper

> We use *contract* in the sense of Design by Contract (Meyer 1992), lifted to framework lifecycle state (typestate; Strom and Yemini 1986). A readiness contract states a precondition on a framework resource's lifecycle state at a use (e.g., cFE's documented assumption that an Application ID "must correspond to a registered application"), together with the not-ready outcomes the framework documents (return codes, documented drops, fallbacks). Its violation is the client's fault, as in DbC. Unlike assume-guarantee contracts (Benveniste et al. 2018; Cofer et al. 2012), our contracts carry no refinement or composition semantics. Unlike SCOOP (Nienaltowski et al. 2009), they do not turn preconditions into waits. The cross-thread order that makes a precondition hold is not part of the contract: Kairos derives it with happens-before and must-before analysis. Each contract records its provenance. We report separately the contracts backed by API documentation, those admitted in code comments, those derived from code patterns such as retry loops (inferred, in the sense of Wei et al. 2011), and uses for which no reference exists (which we do not call contract violations).

### 9.4 Proposed STATUS entry (for the parent to merge)
- **T2 "contract" terminology:** done (report: this document, `Architecture/background/terminology/T2_contract_lineage.md`).
- **Finding:** "contract" is accurate only for documentation-backed readiness preconditions (R2-type). C1 is a client-admitted environment assumption with a code-derived tolerance, and the SB no-subscriber loss violates no contract.
- **Recommendation:** κ becomes "readiness specification"; Req becomes "readiness precondition". Add the provenance tier field (contract-backed / admitted / code-derived / issue-derived / researcher / unreferenced) and report it. Use "API contract" only for documented supplier semantics. Move K-LOCKBAL to the supplier-postcondition family. Fix the §1.11 typestate row. Add AI (FSE 2014; TSE 2016) to the learned-invariant baselines.
- **PI decision needed:** rename (§9.2), or keep the term with the §9.3 definition.

---

## 10. References (verification status)

| # | Reference | Status | What I saw |
|---|---|---|---|
| 1 | Hoare, An Axiomatic Basis for Computer Programming, CACM 12(10):576–580, 583, 1969 | V | snippet |
| 2 | Meyer, Applying "Design by Contract", IEEE Computer 25(10):40–51, Oct 1992 | V | summary (article not opened; download blocked) |
| 3 | Meyer, Object-Oriented Software Construction, Prentice Hall 1988; 2nd ed. 1997 | V | summary |
| 4 | Eiffel docs: DbC obligations/benefits; blame rule; "Design by Contract in SCOOP" (eiffel.org) | V-doc | snippet |
| 5 | Leavens, Baker, Ruby, Preliminary Design of JML, ACM SIGSOFT SEN 31(3):1–38, 2006 | V | summary |
| 6 | JML `normal_behavior`/`exceptional_behavior` (Leavens's JML manual, OpenJML tutorial, ESC/Java2 tutorial) | V-doc (wording P) | snippet |
| 7 | Fähndrich, Barnett, Logozzo, Embedded Contract Languages, ACM SAC 2010, pp. 2103–2110 | V | summary |
| 8 | Microsoft Learn, Code Contracts (`Requires<TException>`, `EnsuresOnThrow`, legacy if-then-throw + `EndContractBlock`) | V-doc | snippet |
| 9 | Cousot, Cousot, Fähndrich, Logozzo, Automatic Inference of Necessary Preconditions, VMCAI 2013, LNCS 7737:128–148 | V | summary |
| 10 | ACSL function contracts / behaviors (course slides, Frama-C API docs) | P | snippet |
| 11 | WG21 P2900 Contracts for C++, adopted Feb 2025 (Hagenberg); P4043R0 (2026) | V (adoption); P (P4043 content) | snippet |
| 12 | Nienaltowski, Ostroff, Meyer, Contracts for Concurrency, Formal Aspects of Computing 21(4):305–318, 2009 | V (wait-condition wording P for this paper) | abstract snippet |
| 13 | Meyer, Systematic Concurrent Object-Oriented Programming, CACM 36(9):56–80, 1993 | V | summary |
| 14 | Jones, Tentative Steps Toward a Development Method for Interfering Programs, ACM TOPLAS 5(4):596–619, 1983 | V (definitions P) | secondary snippets |
| 15 | Benveniste et al., Contracts for System Design, FnT EDA 12(2–3):124–400, 2018 | V (definitions P) | abstract, TOC |
| 16 | Sangiovanni-Vincentelli, Damm, Passerone, Taming Dr. Frankenstein, Eur. J. Control 18(3):217–238, 2012 | V | summary |
| 17 | Cofer, Gacek, Miller, Whalen, LaValley, Sha, Compositional Verification of Architectural Models, NFM 2012, LNCS 7226:126–140 | V | snippet |
| 18 | Backes, Cofer, Miller, Whalen, Requirements Analysis of a Quad-Redundant Flight Control System, NFM 2015 (arXiv 1502.03343) | V | summary |
| 19 | Cimatti, Tonetta, Contracts-Refinement Proof System for Component-Based Embedded Systems (OCRA) | P (venue not confirmed) | preprint summary |
| 20 | de Alfaro, Henzinger, Interface Automata, ESEC/FSE 2001, pp. 109–120 | V | summary |
| 21 | Castagna, Gesbert, Padovani, A Theory of Contracts for Web Services, ACM TOPLAS 31(5), 2009 | V | summary |
| 22 | Fiedor/Dias, Ferreira, Lourenço, Smrčka, Sousa, Vojnar, Verifying Concurrent Programs Using Contracts, ICST 2017, pp. 196–206 | V | summary |
| 23 | Monperrus, Eichberg, Tekes, Mezini, What Should Developers Be Aware Of?, EMSE 17(6):703–737, 2012 | V (taxonomy details P) | abstract snippet |
| 24 | Zhou, Gu, Chen, Huang, Panichella, Gall, Analyzing APIs Documentation and Code to Detect Directive Defects, ICSE 2017, pp. 27–37 | V | summary |
| 25 | Saied, Sahraoui, Dufour, An Observational Study on API Usage Constraints and Their Documentation, SANER 2015, pp. 33–42 | P | citing reference list + summary |
| 26 | Dietrich, Pearce, Jezek, Brada, Contracts in the Wild: A Study of Java Programs, ECOOP 2017, LIPIcs 74, 9:1–9:29 | V | summary |
| 27 | Estler, Furia, Nordio, Piccioni, Meyer, Contracts in Practice, FM 2014 (arXiv 1211.4775) | P | arXiv title only |
| 28 | Strom, Yemini, Typestate, IEEE TSE 12(1):157–171, 1986 | V | abstract summary |
| 29 | DeLine, Fähndrich, Typestates for Objects, ECOOP 2004, LNCS 3086:465–490 | V | summary |
| 30 | Beckman, Kim, Aldrich, An Empirical Study of Object Protocols in the Wild, ECOOP 2011, pp. 2–26 | V | summary |
| 31 | Meier, Mover, Chang, Lifestate: Event-Driven Protocols and Callback Control Flow, ECOOP 2019, LIPIcs 134, 1:1–1:29 | V | summary |
| 32 | Hutchins, Ballman, Sutherland, C/C++ Thread Safety Analysis, SCAM 2014; Clang ThreadSafetyAnalysis docs (`REQUIRES`) | V; V-doc | summary; snippet |
| 33 | Ball et al., Thorough Static Analysis of Device Drivers, EuroSys 2006, pp. 73–85 (SDV; SLIC rules P) | V | abstract snippet |
| 34 | Robillard, Bodden, Kawrykow, Mezini, Ratchford, Automated API Property Inference Techniques, IEEE TSE 2013 (online 2012) | V (vol/pages not confirmed) | summary |
| 35 | Ernst, Cockrell, Griswold, Notkin, Dynamically Discovering Likely Program Invariants, IEEE TSE 27(2):99–123, 2001 | V | summary |
| 36 | Ammons, Bodík, Larus, Mining Specifications, POPL 2002, pp. 4–16 | V | summary |
| 37 | Polikarpova, Ciupa, Meyer, A Comparative Study of Programmer-Written and Automatically Inferred Contracts, ISSTA 2009, pp. 93–104 | V | abstract summary |
| 38 | Wei, Furia, Kazmin, Meyer, Inferring Better Contracts, ICSE 2011, pp. 191–200 | V | summary |
| 39 | Zhang, Wu, Lu, Qi, Ren, Zheng, AI: A Lightweight System for Tolerating Concurrency Bugs, FSE 2014, pp. 330–340 | V | **full text read** |
| 40 | Zhang et al., A Lightweight System for Detecting and Tolerating Concurrency Bugs, IEEE TSE 42(10):899–917, 2016 | V | summary |
| 41 | Lu, Park, Seo, Zhou, Learning from Mistakes, ASPLOS 2008 | V (quote P) | abstract snippet |
| 42 | NASA FRET / Ogma / Copilot cFS monitor generation (NTRS, Hackage) | P | snippets |
| 43 | SPARK 2014 `Pre`/`Post`/`Global`/`Depends` contracts (AdaCore docs, tutorial) | P | snippets |
| 44 | Korean rendering 계약에 의한 설계 for DbC | UNVERIFIED | no Korean source found |
| L1 | cFE v7.0.1: `cfe_evs.h` (SendEventWithAppID assumption and retvals), `cfe_evs.c:185–188`, `cfe_sb.h` (TransmitMsg, ReceiveBuffer notes), `cfe_es.h` (WaitForSystemState assumption), `cfe_config_handlers.c:63`, `tbl_ut_helpers.c:358–359` | local, read | text |
| L2 | OSAL `osapi-countsem.h:160`; CF `cf_cfdp.c:1272–1290` | local, read | text |

---

## 11. Errata (2026-10-15)

These errata apply the external review EXT-20261012 §6.2–§6.4 (`../../history/review_EXT_20261012.md`) as `../../spec/SPEC_FINAL.md` revision 1.1 answers it (spec §0.7 item 4, §1.6 "Three judgements", §10.1 QT-2–QT-5, ADR-043). The text above is kept as written on 2026-10-12; the markers "erratum En" point here. The synthesis `../08_terminology.md` (revision 3) carries the same corrections. The word "contract" stays in every document while the renaming (T1) is pending.

- **E1. Supplier results and caller acceptance** [D in the spec]. §0 items 1–2 and §1's `Allowed` row count the supplier's documented error results, `\retval` codes and supplier drops as parts of κ or as Allowed entries. Spec revision 1.1 separates them (§1.6 "Three judgements", §1.10, ADR-043). What the supplier documents and returns is the framework-level judgement (`fw_doc` per instance, `fw_conf` per cell); it never makes an occurrence TOLERATED. An Allowed entry decides statuses only at the caller level, when a caller-side text supports it or, under the default of PI item QT-4, a fixed rule (reported as rule-backed). `documented_drop` now names a drop of the using code's own request documented on the caller side; a supplier's documented drop of a caller's request is framework-level information (spec §1.6, Allowed table). For R2 the caller-level Allowed is `none`, and the header's mention of `CFE_ES_WriteToSysLog` names a different API, not an accepted outcome of the use (spec App. A.1 step 8). The mix noted in §1 is thus resolved by the side of each provenance record (`supplier`, `caller`, `rule`) and the field `appreq` (spec §1.6).
- **E2. R2 is not a textbook contract violation** [F]. §0 item 4 was superseded by `../08_terminology.md` §2.3 ("Partly"): EVS also documents the not-ready results as return codes (`cfe_evs.h:198–199`). In spec revision 1.1, R2 violates a caller-side obligation (`appreq` = `cfe_evs.h:169–172`), while EVS's documented error return is the framework-level result `documented-result` (spec App. A.1 step 8).
- **E3. `OS_ERR_NAME_NOT_FOUND`** [F]. §0 item 4 and §6's C1 row: OSAL documents it as an `@retval` "if the name was not found in the table" (`osapi-countsem.h:160`). The header calls it neither normal nor an error.
- **E4. The SB loss** [M], [I]. §0 item 4's "SB's documented contract is fulfilled" holds outside R2's window. Inside it, the `SEND_NO_SUBS` events that cSB4305.5 requires are rejected by EVS [M], and the window's counter increments are cleared later by SB's own initialization [I] (spec App. A.1 step 13a). cSB4305.5 is framework-level (`documented-not-error`); the caller level is LOSS-UNDER-ORDER unless a consumer or mission reference decides (spec §3.9; PI item QT-2, which re-asks STATUS decision T2).
- **E5. "Invariant"** [F: AI p. 331; I]. §3.15 item 1 ("AI chose 'invariant' because the property is empirical") attributes a motive the paper does not state: AI defines program invariants in the specified sense and calls the learned objects "likely invariants" (p. 331; T1 §7 item 1), and Daikon's own definition is the specified one too (T3 §2.1, snippet). §3.15 item 3 and §9.1's "readiness invariant" row overstate in the same way. An invariant can be written, proved or inferred. The reason not to call κ an invariant is that κ is a specification object, not a single predicate (spec §1.6); the learned reading next to AI and DefUse is a secondary, reader-side risk [I].
- **E6. "Not contracts"** [U]. §3.15's "speaks of intentions and invariants, not contracts" describes the Lu-line works cited there; it is not a census of the word. GPredict's Introduction calls the Java Iterator usage rule "a more generic contract" (known from the external review's reading and a search snippet; PDF not opened; P-PAPERS); see T4 erratum E2.

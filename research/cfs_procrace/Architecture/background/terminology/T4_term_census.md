# T4: Term census. How related work names the required order (readiness) and its violation

**Scope.** The PI asked whether "contract" (계약) is the right word for Kairos's κ ("readiness contract", `spec/SPEC_FINAL.md` §1.6, §1.10, §3.15, §7.2). This note does not argue the lineage of "contract"; T2 (`T2_contract_lineage.md`) does that. This note asks a narrower, empirical question. **In the papers and systems a reviewer will compare us with, what words name (a) the required order or readiness condition and (b) its violation? Is that requirement learned, specified or built in?** It ends with the vocabulary a PLDI/ICSE/FSE/ASPLOS reviewer would expect for our problem.

**Date and inputs.** 2026-10-12. This note is a dated input; the **errata of 2026-10-15 are in §10** and are marked "erratum En" where they apply. Read:
- the AI paper PDF, all 11 pages (as images, plus `pdftotext`);
- SPEC_FINAL §1.6, §1.10, §1.11, §3.15, §7.2;
- background `04_lit_dynamic.md` [R04] and `05_lit_systematic_static.md` [R05];
- T1 and T2.

Nothing in the repository was edited.

**Tags.**
- **V**: authors, title, venue and year confirmed by a search result or a document page.
- **P**: only partly confirmed. Either the bibliographic data is confirmed but the wording comes from a summary or a secondary source, or the venue is not confirmed.
- **UNVERIFIED**: not confirmed.
- **V-doc**: a primary documentation source that I fetched and read (the path is given).
- For every quoted term I say whether I saw the **text** (PDF or document) or only a **summary** (search-engine summary of the abstract or paper, or a secondary source).
- **[I]** marks my inference and **[S]** a suggestion.

---

## 0. Bottom line

1. **No race, order-violation or initialization-race work in this census calls the required order a "contract".** (Recorded at abstract level only: errata E1–E3, §10.) The census has 52 entries. The required order is named in five ways (§4.1):
   - **as an intention**: "programmers' order intentions", "desired order", "assumed order", "the programmer intends …";
   - **as a learned invariant**: AVIO, DefUse, PSet, CCI, AI;
   - **as a built-in model** of what the platform guarantees: happens-before in WebRacer, EventRacer, DroidRacer, CAFA, SIERRA, DCatch, NodeRacer;
   - **as a specification**: typestate, Lifestate "protocols", LOOM "execution filters", EventRaceCommander "repair policies";
   - **in systems platforms, as a declared dependency or readiness condition**: systemd "ordering dependencies" and readiness notification, Kubernetes "readiness probe", Compose `condition: service_healthy`, OSGi DS "satisfied", Linux `-EPROBE_DEFER` and device links.
2. **The violation is named by order shape or by symptom.**
   - By shape: "order violation", "atomicity violation", "(event) race", "untimely message".
   - By symptom: "access-before-definition", "late-event-handler-registration", "use-free race", "use-before-initialization", "uninitialized read", "time-of-fault bug", "resume and restart errors".
   - Only work that checks an explicit object names the violation after that object: **"protocol violation"** (Lifestate, specified), "property violation" (GPredict, specified) and "invariant violation" (AI, learned).
3. **"Readiness" is a systems and operations word, not a bug-detection word.**
   - It is standard in Kubernetes ("Readiness probes determine when a container is ready to accept traffic"), systemd (`READY=1`, "readiness notification") and the ZeroMQ guide ("connected and ready").
   - One extended search found **no** PLDI/ICSE/FSE/ASPLOS bug-detection paper that uses "readiness" as its term. This is a negative result, not proof of absence.
   - It also collides with **I/O readiness** (`select`/`epoll`: descriptors "ready" for I/O).
   - Kairos can adopt it, but must define it [I].
4. **Learned vs specified vs built-in (§4.3).**
   - The multithreaded order-violation line is **learned** (AVIO, DefUse, PSet, CCI, AI) or **built-in** (CTrigger, ConMem, Falcon/Unicorn patterns).
   - Event and distributed race detectors are **built-in** (HB rules plus fixed error types).
   - Among bug-finding works, only six are **specified**: GPredict (user-written properties), LOOM (execution filters), CFix (order relationships taken from detector reports), EventRaceCommander (repair policies), Mishra et al. (typestate) and Lifestate. None of them targets start-up readiness. The systems platforms are also specified.
   - Kairos's κ is on the specified side, **with provenance per entry**. No census entry records provenance, so this is unusual in race work and should be stated as such.
5. **Our cases in census vocabulary (§5):**
   - **R2** is an order violation in Lu et al.'s sense: the desired order is flipped. But the wrong order is **enforced** in every boot. That makes it closer to a deterministic **initialization-order** defect than to the non-deterministic bugs that Lu 2008 and TaxDC study. The C++ "static initialization order" problem and UBITect's "use-before-initialization" are the nearest named classes. Learned invariants absorb it (T1).
   - **C1** is an initialization race tolerated by **retry**. The Linux kernel's "deferred probe" (`-EPROBE_DEFER`, "resources that are not yet available") is the closest systems name for that tolerance. "Retry bugs" (SOSP 2024) is the name for when a retry is wrong.
   - **SB subscribe-before-first-send loss** has three established names: InitRacer's "late-event-handler-registration", ZeroMQ's **"slow joiner"** ("the subscriber will always miss the first messages") and ROS 2's **"late-joining" subscriptions** under "volatile" durability. In ZeroMQ and ROS 2 the loss is **by design** unless durability is requested. This supports Kairos's LOSS-UNDER-ORDER / `unspecified_by_reference` labelling.
6. **Reviewer expectation and recommendation (§6).**
   - **Bug class.** Anchor it in **"order violation"** (Lu et al. 2008) and **"initialization race"** (InitRacer). Name our violation a **"readiness violation"** (short form: use-before-ready), defined as a use whose **readiness precondition** (a framework typestate precondition) is false, with a not-ready outcome that is not allowed.
   - **Object.** Call κ a **readiness specification**, as T2 recommends.
   - **Avoid unqualified words.** "Invariant" means learned in this literature (overstated: erratum E4, §10). "Contract" means DbC in this literature (T2). "Tolerated" means run-time bug avoidance in the AI / Atom-Aid / ConAir line.
   - **Cite systemd for the separation.** systemd states explicitly that ordering dependencies are "independent of and orthogonal to the requirement dependencies". That is a citable precedent for separating *what state must hold* (Req) from *what order establishes it* (HB/must-before).
7. **Premise corrections found while verifying:**
   - Unicorn is **ICST 2012**, not ICSE.
   - NodeRacer is **ICST 2020**, not ICSE.
   - The PLDI 2012 web race paper is Petrov et al.; the tool name "WebRacer" is still UNVERIFIED. EventRacer is OOPSLA 2013.
   - LOOM's authors are **Wu, Cui, Yang** (OSDI 2010).
   - DefUse's published title spells the tool **"DeFuse"**.
   - The phrase **"intended order" was not found** for Lu 2008. The verified wording is "programmers' order intentions" (abstract) and "desired order" (definition, via OSTEP, P).

---

## 1. Method and limits

- **Network.**
  - WebSearch worked, but it returns engine **summaries** of abstracts and pages, not full text.
  - WebFetch failed on every host with DNS errors (e.g. `kubernetes.io`, `www.cs.columbia.edu`).
  - Through the session proxy, `curl` reached **only `raw.githubusercontent.com`**. Other hosts got 403 (policy).
  - The Scholar Gateway connector returned `ACCESS_DENIED`.
  - Consequence: apart from the AI paper, **no paper's full text was read**. A definition from a paper is marked **P (summary)** unless the AI paper quotes or describes it (text seen).
- **Documentation fetched from GitHub** (raw files, fetched 2026-10-12, stored under `scratchpad/arch/terms/dl_t4/docs/`):

  | Source | Path | Note |
  |---|---|---|
  | systemd | `man/systemd.unit.xml`, `man/systemd.service.xml`, `man/sd_notify.xml` | branch `main` |
  | Kubernetes | `content/en/docs/concepts/workloads/pods/pod-lifecycle.md`, `.../init-containers.md` | `kubernetes/website`, `main` |
  | Compose spec | `05-services.md` | `compose-spec/compose-spec`, `main` |
  | OSGi DS (ch. 112) | `osgi.specs/docbook/112/service.component.xml` | `osgi/osgi`, `main`; spec release number not determined |
  | Linux | `Documentation/driver-api/driver-model/driver.rst`, `.../device_link.rst` | `torvalds/linux`, `master` |
  | C++ Core Guidelines | `CppCoreGuidelines.md` | `isocpp/CppCoreGuidelines`, `master` |
  | Go memory model | `doc/go_mem.html` ("Version of June 6, 2022") | `golang/go`, `master` |
  | AndroidX | `lifecycle/lifecycle-common/.../Lifecycle.kt` | `androidx/androidx`, `androidx-main` |
  | ZeroMQ guide | `chapter1.txt`, `chapter5.txt` | `booksbyus/zguide`, `master` |
  | ROS 2 docs | `source/Concepts/Intermediate/About-Quality-of-Service-Settings.rst` | `ros2/ros2_documentation`, `jazzy` (the `rolling` path returned 404) |
  | man-pages | `man7/epoll.7`, `man2/select.2` | `mkerrisk/man-pages`, `master` |
  | Tool READMEs | `eth-sri/EventRacer`, `cs-au-dk/initracer`, `andreendo/noderacer` | |

- **Not covered:**
  - aerospace partitioned-OS vocabulary (ARINC 653 partition modes);
  - Spring bean lifecycle phases;
  - the isocpp.org FAQ text itself (only cppreference via search).

---

## 2. Census table

Legend for **Kind** (how the requirement enters the tool):
- **L**: learned from runs (or ranked statistically from pass/fail runs).
- **S**: specified by a person (spec, annotation, filter, policy, configuration, documentation).
- **B**: built-in rule fixed by the tool's designers (an HB model, bug patterns, fixed error types, effect classes).
- **E**: empirical study; the authors judge intention from bug reports and fixes.

"Text" means I saw the wording in a PDF or document. "Summary" means I saw it only in a search-engine summary or a secondary source.

### 2.1 Multithreaded order and atomicity violations (detection, localization)

| # | Work (status) | Requirement term | Violation term | Kind | Where the wording comes from |
|---|---|---|---|---|---|
| 1 | Lu, Park, Seo, Zhou, ASPLOS 2008 (V) | "programmers' order intentions" (abstract); "desired order" (definition) | **"order violation"**, "atomicity violation" | E | Abstract wording in two summaries (V-abs). The definition, "the desired order between two (groups of) memory accesses is flipped", is quoted from Lu et al. in OSTEP ch. 32: **P (summary)**. "Intended order": **not found** |
| 2 | AVIO, Lu, Tucek, Qin, Zhou, ASPLOS 2006 (V: AI ref [13], text; R04 V-abs) | "access interleaving invariants" | unserializable interleaving / atomicity violation | L | AI text p.336: the AVIO invariant "is consisting of two static instructions from one thread that should not be interleaved by an unserializable memory operation from a different thread" |
| 3 | CTrigger, Park, Lu, Zhou, ASPLOS 2009 (V) | none (atomicity assumed) | "unserializable interleavings" (correlated with atomicity violations) | B | Summary of abstract |
| 4 | ConMem, Zhang, Sun, Lu, ASPLOS 2010 (V) | none (effect-oriented) | "concurrency-memory bugs": NULL-pointer dereference, dangling pointer, buffer overflow, **uninitialized read** | B | Summary of abstract. The TOSEM extension has 5 authors and 10 bugs; do not mix it up with the ASPLOS paper |
| 5 | Falcon, Park, Vuduc, Harrold, ICSE 2010, pp. 245–254 (V) | none (patterns) | "faulty data-access patterns", ranked by suspiciousness | L (pass/fail statistics) + B (patterns) | Summary. Exact pattern list: P |
| 6 | Unicorn, Park, Vuduc, Harrold, **ICST** 2012, pp. 51–60 (V) | none | "problematic memory access patterns" covering order violations and single- and multi-variable atomicity violations | L + B | Summary of abstract (academia listing) |
| 7 | DefUse, Shi, Park, Yin, Lu, Zhou, Chen, Zheng, OOPSLA 2010, pp. 160–174 (V; title spells "DeFuse") | **"definition-use invariants"**: LR, Follower, DSet | invariant violation; "Do I use the wrong definition?" | L | AI text p.337: "DUI-LR describes the property that a local read should always read a value defined by a local or remote writer; 2) DUI-Follower checks whether two consecutive reads from one thread must read the same value; and 3) DUI-DSet defines a 'definition set' for every read instruction" |
| 8 | PSet, Yu, Narayanasamy, ISCA 2009 (P: AI ref [35], text; R04 P) | "PSet", "interleaving constraints" | dependence on a predecessor outside PSet | L | AI text p.337: "PSet will ensure that a memory operation M can only 'immediately depend on' an instruction P that belongs to a specific set named PSet, which is established during the testing phase" |
| 9 | CCI, Jin, Thakur, Liblit, Lu, OOPSLA 2010 (P: AI ref [7], text) | predicates such as "whether the last access was from the same thread" | predicate correlated with failure | L | AI text p.337 (as described by AI, not by CCI) |
| 10 | **AI**, Zhang, Wu, Lu, Qi, Ren, Zheng, FSE 2014, pp. 330–340 (V; read in full) | **"Anticipating Invariant"**: `RPre(I_xS_y) ∈ BSet(S_y)`. In prose: "programmers' intentions about the correct order"; "the assumed order"; "The order assumed by developers is that S4 should always be executed after S2" | **"Anticipating Invariant Violation"**; "order violation", "atomicity violation" | L | Text: p.331 ("it reflects programmers' intentions about the correct order of concurrent memory accesses"), p.334, p.335 ("the programmer intends that a write operation should always be executed before another read operation, but this intention is not guaranteed"). Fig. 9 caption (seen in the page image): "global_opt should be initialized before being read" |
| 11 | Xiong, Park, Zhang, Zhou, Ma, OSDI 2010 (V) | the program's own **"ad hoc synchronization"**: loops whose exit conditions depend on "sync variables" | ad hoc synchronizations that introduce bugs or performance problems | B (SyncFinder patterns) | V-abs for the study numbers. The loop/exit-condition wording comes from a course summary: P |

### 2.2 Enforcing, repairing, tolerating an order

| # | Work (status) | Requirement term | Violation / remedy term | Kind | Where |
|---|---|---|---|---|---|
| 12 | AI, FSE 2014 (V) | Anticipating Invariant | "tolerating concurrency bugs" by **thread stalling** before the "turning point" | L | Text. **This is a collision for Kairos's `TOLERATED`** (§4.6) |
| 13 | LOOM, **Wu, Cui, Yang**, OSDI 2010 (V) | developer-written **"execution filters"**; "synchronization intents" | races "bypassed" in live applications | S | V-abs for "execution filters". "Synchronization intents" comes from an NSF grant description (summary): P. Filter syntax: UNVERIFIED |
| 14 | CFix, Jin, Zhang, Deng, Liblit, Lu, OSDI 2012, pp. 221–236 (V) | **"mutual-exclusion and order relationships"** to enforce | the "failing interleaving" reported by a detector | S (from detector output) | Summary of abstract |
| 15 | EventRaceCommander, Adamsen, Møller, Karim, Sridharan, Tip, Sen, ICSE 2017, pp. 289–299 (V) | **"repair policies"**, application-independent or specific | "event race errors", repaired by "postponing or discarding selected events" | S | Summary of abstract |
| 16 | WASABI, Stoica, Sethi, Su, Zhou, Lu, Mace, Musuvathi, Nath, SOSP 2024 (V) | the retry mechanism ("re-execution of a task on failure") | **"retry bugs"** / "retry problems" | B + LLM | Summary of abstract (MSR page) |

### 2.3 Event-driven programs (web, Node.js)

| # | Work (status) | Requirement term | Violation term | Kind | Where |
|---|---|---|---|---|---|
| 17 | Petrov, Vechev, Sridharan, Dolby, "Race Detection for Web Applications", PLDI 2012, pp. 251–262 (V; tool name "WebRacer" UNVERIFIED) | happens-before for browser events | (web) races | B | Summary (OpenAlex/bibsonomy). No full text |
| 18 | EventRacer, Raychev, Vechev, Sridharan, OOPSLA 2013, pp. 151–166 (V) | HB, plus **"race coverage"** to expose ad hoc synchronization | event races; **"uncovered races"** | B | Abstract quoted in a summary: "race coverage, a systematic method for exposing ad hoc synchronization and other (potentially harmful) races". "Uncovered races" is V-doc: the EventRacer README says "Read our paper … to understand the meaning of uncovered races and the race filters" |
| 19 | **InitRacer**, Adamsen, Møller, Tip, OOPSLA 2017, PACMPL 1, Art. 66, DOI 10.1145/3133890 (V) | implicit: initialization (page loading) completes before a dependent event or access | **"initialization races"** of three types: **"form-input-overwritten"**, **"late-event-handler-registration"**, **"access-before-definition"** | B (fixed error types; adverse + approximate execution) | Abstract: V-abs. Meaning of late registration ("a user event fires before its handler has been installed"; apple.com example): **P (summary)**. The repo's test directory is named `test/access-before-definition/…` (V-doc) |
| 20 | Node.fz, Davis, Thekumparampil, Lee, EuroSys 2017 (V) | the order between callbacks, which Node.js does not guarantee | **"order violations"**, "atomicity violations" | E + fuzzing | Summary. Blog definition: "operation A should always be invoked before operation B, but this order is not enforced" (P, secondary) |
| 21 | Wang, Dou, Gao, Gao, Qin, Yin, Wei, "A Comprehensive Study on Real World Concurrency Bugs in Node.js", ASE 2017 (P: reference list only) | — | — | E | Content not seen |
| 22 | NodeRacer, Endo, Møller, **ICST** 2020 (V) | HB relations guiding event postponement | event races | B | Summary of abstract. The README says "accepted in … ICST 2020" (V-doc) |

### 2.4 Android: event races, lifecycle, callback protocols

| # | Work (status) | Requirement term | Violation term | Kind | Where |
|---|---|---|---|---|---|
| 23 | DroidRacer, Maiya, Kanade, Majumdar, PLDI 2014, pp. 316–325 (V) | "happens-before relation for Android applications", which generalizes multithreaded and event-driven HB | data races | B | Summary of abstract. The "enable" (lifecycle) operation: UNVERIFIED (R04 secondary) |
| 24 | CAFA, Hsiao, Yu, Narayanasamy, Kong, Pereira, Pokam, Chen, Flinn, PLDI 2014, pp. 326–336 (V) | "causality model" including event-queue order; races between **"high-level operations"** | **"use-free race"** (a paper keyword) | B | Abstract: V-abs. "Use-free race" from the keyword list as reported by a summary: **P** (R04 had UNVERIFIED) |
| 25 | SIERRA, Hu, Neamtiu, ASPLOS 2018 (V) | statically inferred **happens-before between "concurrency actions"**, from lifecycle and GUI models | **"event-based races"** | B | Summary of abstract |
| 26 | KREfinder, Shan, Azim, Neamtiu, OOPSLA 2016, pp. 864–880 (V) | correct handling of instance data across the lifecycle | **"resume and restart errors"** (data loss or crash) | B | Summary of abstract |
| 27 | Mishra, Kanade, Srikant, MEMOCODE 2016, pp. 163–172 (V) | **typestate** "state machines" for resources; "all possible orderings of lifecycle callbacks" | resource API **"protocol violations"** | S (typestate specs) + B (lifecycle model) | Abstract: V-abs. "Protocol violations" comes from a thesis summary: P |
| 28 | **Lifestate**, Meier, Mover, Chang, ECOOP 2019, LIPIcs 134, 1:1–1:29 (V) | **"lifestate" specification** of "application-programming protocols" and "callback control flow"; combines lifecycle constraints and typestate rules | **"protocol violation"** (calling an API "in a state of the framework where such a call is invalid") | **S** | Summary of abstract |
| 29 | AndroidX `Lifecycle` (V-doc, `androidx-main`) | **"lifecycle state"**: `DESTROYED < INITIALIZED < CREATED < STARTED < RESUMED`; `isAtLeast(state)` | none (the framework enforces it) | built into the framework API | Text: "States are ordered as [DESTROYED] < [INITIALIZED] < [CREATED] < [STARTED] < [RESUMED]" |

### 2.5 Distributed systems

| # | Work (status) | Requirement term | Violation term | Kind | Where |
|---|---|---|---|---|---|
| 30 | TaxDC, Leesatapornwongsa, Lukman, Lu, Gunawi, ASPLOS 2016 (V) | **"triggering timing condition"**; a timing condition is "the smallest set of concurrent events E, so that a specific ordering of E can guarantee the bug manifestation" | **"untimely"** message delivery, fault or reboot, which "commits order violation or atomicity violation" | E | Abstract: V-abs. Definitions and the >60% figure: P (summaries). Fix strategies (blog review): add synchronization, or "ignoring/neutralizing the untimely message" (P) |
| 31 | DCatch, Liu, Li, Lukman, Li, Lu, Gunawi, Tian, ASPLOS 2017 (P) | **HB rules** for RPC, messages, events, threads | **"DCbugs"**, "triggered by untimely interaction among nodes" | B | Patent text (summary). The paper itself was not reached; R04 already had P |
| 32 | FCatch, Liu, Wang, Li, Lu, Ye, Tian, ASPLOS 2018 (V) | "fault-aware logical time" | **"time-of-fault (TOF) bugs"**: "crash-regular" and "crash-recovery" | B (predicted "by observing correct execution") | Abstract: V-abs. The two sub-classes come from slides: P |

### 2.6 Sequential initialization and typestate

| # | Work (status) | Requirement term | Violation term | Kind | Where |
|---|---|---|---|---|---|
| 33 | Strom, Yemini, "Typestate", IEEE TSE 12(1):157–171, 1986 (V) | **typestate**: the operations allowed in a given context | e.g. reading a variable before initialization | S (type system) | Summary (DBLP / Frama-C / Wikipedia) |
| 34 | UBITect, Zhai et al., ESEC/FSE 2020, DOI 10.1145/3368089.3409686 (V) | initialization before use (type qualifiers) | **"use-before-initialization (UBI) bugs"** | B | Summary of abstract |
| 35 | ExceptioNULL, Farzan, Madhusudan, Razavi, Sorrentino, "Predicting Null-Pointer Dereferences in Concurrent Programs" (P: venue FSE 2012 not confirmed by this search; R04 V-abs) | — | null-pointer dereferences in predicted interleavings | B | Summary of abstract |

### 2.7 Systems and platform vocabulary for "readiness" and startup order

| # | Source (status) | Requirement term | Violation / failure term | Kind | Text seen |
|---|---|---|---|---|---|
| 36 | **systemd** `systemd.unit(5)` (V-doc) | **"ordering dependencies"** (`Before=`, `After=`) vs **"requirement dependencies"** (`Requires=`, `Wants=`, `Requisite=`, `BindsTo=`) | none named | S | "Note that those settings are independent of and orthogonal to the requirement dependencies …" and "Note that requirement dependencies do not influence the order in which services are started or stopped" |
| 37 | systemd `sd_notify(3)`, `systemd.service(5)` (V-doc) | **"readiness notification"** (`READY=1`, `Type=notify`): "service startup is finished" | `Type=simple`: follow-up units start before the binary has even run; "systemctl start … will report success even if the service's binary cannot be invoked" | S | "systemd will consider the unit to be in the 'starting' state until a readiness notification has arrived" |
| 38 | **Kubernetes** Pod lifecycle, init containers (V-doc) | **"readiness probe"**, "startup probe", "liveness probe"; Pod condition **`Ready`**; **"readiness gates"**; init containers "run to completion" | not-ready: "the EndpointSlice controller removes the Pod's IP address" | S (probes) + platform | "Readiness probes determine when a container is ready to accept traffic." "`Ready`: the Pod is able to serve requests." Init containers: "cannot define readiness distinct from completion" |
| 39 | Compose spec `depends_on` (V-doc) | `condition`: **`service_started`**, **`service_healthy`**, **`service_completed_successfully`**; "the condition under which dependency is considered satisfied" | — | S | "Compose guarantees dependency services marked with `service_healthy` are 'healthy' before starting a dependent service." |
| 40 | OSGi Declarative Services (ch. 112) (V-doc) | component configuration **"satisfied"** when its references are satisfied (minimum cardinality) | **"unsatisfied"**: it "must be deactivated" | S (component descriptions) + runtime (SCR) | "A component configuration becomes *satisfied* when each specified reference is satisfied … An activated component configuration that becomes *unsatisfied* must be deactivated." |
| 41 | Linux driver model, `driver.rst` and `device_link.rst` (V-doc) | **"supplier"/"consumer"** device links: "The consumer devices are not probed before the supplier is bound to a driver" | **`-EPROBE_DEFER`**: the driver "depends on resources that are not yet available (e.g., supplied by a driver that hasn't initialized yet)"; "deferred probe list" | S (links) + built-in retry | Text as quoted |
| 42 | C++ Core Guidelines I.2, I.22 (V-doc) | **"initialization order"** of globals | "undefined initialization order"; "the order-of-initialization problem"; "one will access an uninitialized `const`"; remedy "initialization on first use" | language rule | "The initialization of global objects is not totally ordered." "Order of initialization problems become particularly difficult to handle in concurrent code." |
| 43 | **"Static initialization order fiasco"** (C++ FAQ; cppreference "siof") (P) | — | the fiasco; remedy "Construct On First Use Idiom" | language rule | Summary only. The isocpp.org FAQ text was not seen |
| 44 | Go memory model, "Version of June 6, 2022" (V-doc) | **"synchronized before"**: init → `main.main`; `once.Do(f)` | — | language rule | "The completion of all `init` functions is synchronized before the start of the function `main.main`." |
| 45 | Java: *Java Concurrency in Practice* (Goetz et al., 2006) and SEI CERT TSM01-J (P) | **"safe publication"**; do not let `this` escape during construction | "improperly published", "partially-constructed objects" | guideline | CERT wording via summary. JCIP from its table of contents (summary) |
| 46 | **ZeroMQ guide**, ch. 1 and 5 (V-doc) | publish only "until the subscribers really are connected and ready" | **"slow joiner" symptom/syndrome**: "the subscriber will always miss the first messages that the publisher sends" | by design (loss accepted unless synchronized) | Text as quoted. Alternative quoted from the guide: "assume that the published data stream is infinite and has no start and no end" |
| 47 | **ROS 2** QoS settings (V-doc, `jazzy`) | durability **"transient local"** vs **"volatile"** | **"late-joining" subscriptions** get nothing under volatile ("no attempt is made to persist samples") | S (QoS) | "the publisher becomes responsible for persisting samples for 'late-joining' subscriptions" |
| 48 | `epoll(7)`, `select(2)` (V-doc) | file descriptors **"ready" for I/O**; the "ready list" | — | kernel API | **A collision for "readiness"** |

### 2.8 API documentation and API misuse (the source side of κ)

| # | Work (status) | Requirement term | Violation term | Kind | Where |
|---|---|---|---|---|---|
| 49 | Monperrus, Eichberg, Tekes, Mezini, EMSE 17(6):703–737, 2012 (V) | **"API directives"**: "constraints and guidelines related to the usage of an API"; 23 kinds | (directive not followed) | S (natural language) | Summary of abstract. T2 §3.12 has more detail |
| 50 | Amann, Nguyen, Nadi, Nguyen, Mezini, "A Systematic Evaluation of Static API-Misuse Detectors", IEEE TSE, DOI 10.1109/TSE.2018.2827384 (V) | **"usage constraints"**, "such as restrictions on call order or call conditions" | **"API misuse"** (MUC classification) | L/B (detectors) vs S (benchmark) | Summary. MUC category names: P |
| 51 | Engler, Chen, Hallem, Chou, Chelf, SOSP 2001 (V) | **"beliefs"**: MUST beliefs (e.g. a dereference implies non-null) and MAY beliefs (statistical) | "deviant behavior" (contradictions) | L (inferred from code) | Summary |

### 2.9 Predictive analysis with user-written properties

| # | Work (status) | Requirement term | Violation term | Kind | Where |
|---|---|---|---|---|---|
| 52 | GPredict, Huang, Luo, Roşu, ICSE 2015, pp. 847–857 (V) | user-written **"concurrency properties"** ("high-level properties"); spec language adapted from MOP (P) | **"property violations"** predicted under the causal model | **S** | Summary of abstract (FSL page). R04 L553: "user-written generic properties, Java, no notion of a legal error outcome". See errata E2 and E7 (§10) |

---

## 3. Notes per family (what each line of work teaches about naming)

### 3.1 Lu 2008 and the multithreaded line: the requirement is an *intention*, then an *invariant*

- **Lu 2008.** Lu 2008 names the requirement only as an intention ("programmers' order intentions"). The violation is "order violation": the desired order is flipped, i.e. it is not enforced.
  - This is a **classification** of bugs, not a checkable object.
  - Every later detector in this line had to manufacture the requirement. Most learned it from runs: AVIO, DefUse, PSet, CCI, AI. A few hard-wired generic patterns: CTrigger, ConMem, Falcon/Unicorn.
- **AI follows the same pattern.**
  - Its prose speaks of "programmers' intentions about the correct order", "the order assumed by developers" and "the programmer intends … but this intention is not guaranteed" (text, pp. 331, 334, 335).
  - Its checkable object is a *learned* invariant.
  - Its violation is an "Anticipating Invariant Violation".
- **Takeaway for terms [I].** In this line, the word for the checked requirement is **"invariant"**, and it means **learned from correct runs** (overstated as a definition: erratum E4, §10).
  - Kairos's κ is not learned (T1, T2).
  - So "invariant" is the one word Kairos must not use for κ.
  - "Intention" is the informal word reviewers use for what an order violation violates. Kairos's provenance field is, in effect, a record of **where the intention is written down**.

### 3.2 Event-driven and distributed detectors: the requirement is *built in* as HB rules

- **The detectors.** WebRacer, EventRacer, DroidRacer, CAFA, SIERRA, NodeRacer and DCatch all encode **what the platform guarantees** (happens-before). They report pairs the model leaves unordered ("races", "event races", "event-based races", "DCbugs").
- **No user-facing requirement.** The implicit requirement is generic: conflicting operations must be ordered. Harm is then judged by built-in effect classes (CAFA's "use-free"; InitRacer's three types) or by a triage step (DCatch).
- **InitRacer is the closest neighbour in vocabulary.**
  - It names a **phase**: initialization.
  - Its violation types are named after **what was done too early**: access-before-definition, late-event-handler-registration, form-input-overwritten.
  - Kairos's RC1/RC2/RC4 lookups and RC6 identity uses are "access-before-definition" over framework registries. RC5 (SB routes) is "late registration".
- **Distributed work names the violation by timing.** TaxDC and DCatch say "untimely" message, fault or reboot. TaxDC's notion of a "triggering timing condition" is a *post hoc* description of the bad order, not a requirement.

### 3.3 Specification-based work: "specification", "protocol", "policy", "filter"

- **Specified requirements are called specifications.** When the requirement is written by a person, the census calls it:
  - a **specification** (Lifestate: "lifestate protocol specification");
  - a **typestate** / **state machine** (Strom & Yemini; Mishra et al.);
  - a **policy** (EventRaceCommander "repair policies");
  - a **filter** expressing **intents** (LOOM);
  - a **property** (GPredict: user-written "concurrency properties", with violations predicted by SMT).
- **The violation is a "protocol violation"** (Lifestate; Mishra et al.).
- **None uses "contract"** as the recorded requirement term (errata E1–E2, §10).
- **Lifestate is the closest in shape to κ** [I]: framework lifecycle × typestate × trace checking. T2 §3.13 already flags it as the closest prior work in shape.

### 3.4 Systems platforms: "readiness", "dependency", "satisfied", "defer"

- **The objects are first-class and declared.** systemd unit dependencies and readiness notification; Kubernetes probes, readiness gates, init containers; Compose `depends_on.condition`; OSGi DS references and "satisfied"; Linux device links and deferred probe.
- **Two lessons for Kairos [I]:**
  1. **"Ready" is a provider-side declaration** ("ready to accept traffic", "service startup is finished", `READY=1`).
     - Kairos's `Req` is the consumer-side requirement that the provider's resource has reached that state at the use.
     - That matches the systems meaning of "readiness", so the word transfers well to ASPLOS/OSDI readers.
  2. **Order and requirement are separate concepts.** systemd says ordering dependencies are "independent of and orthogonal to the requirement dependencies".
     - Kairos makes the same separation: a state precondition (`Req`, typestate) is distinct from the order that establishes it (HB, must-before, certificates).
     - T2 makes the same point from the contract side: the cross-thread order is not in any documented precondition.
     - systemd gives a concrete, citable precedent.
- **Not-ready handling is a platform mechanism.**
  - Kubernetes removes a not-ready Pod from endpoints.
  - Linux puts a not-ready driver on the deferred probe list and retries.
  - OSGi deactivates an unsatisfied component.
  - ZeroMQ and ROS 2 drop messages to a late joiner unless durability is configured.
  - These are platform-level analogues of Kairos's `Allowed` vocabulary (§4.6).

### 3.5 Initialization order in languages

- **The names.**
  - C++: "initialization order" / "order-of-initialization problem" (Core Guidelines), "static initialization order fiasco" (FAQ).
  - Go: "synchronized before" (init, once).
  - Java: "safe publication" (JCIP/CERT).
- **The C++ case is instructive for R2 [I].** The order is unspecified by the language but **fixed by the build (link order)**. So a bad order reproduces in every run of a given build, yet is not guaranteed by any specification.
  - R2 has the same structure. Its order is fixed by cFE's core start-up serialization (and by σ), not by any documented guarantee.
  - "Initialization-order bug" is therefore a familiar name for a **deterministic** wrong-order defect.

---

## 4. Cross-cutting patterns

### 4.1 Five ways to name the requirement

| Strategy | Words used | Works |
|---|---|---|
| Intention (informal) | "order intentions", "desired order", "assumed order", "programmer intends", "intents" | Lu 2008; AI; LOOM (P) |
| Learned invariant | "access interleaving invariant", "definition-use invariant", "PSet", "Anticipating Invariant", "belief" (MAY) | AVIO, DefUse, PSet, CCI, AI; Engler |
| Built-in model | "happens-before relation", "causality model", "race coverage", "fault-aware logical time" | WebRacer, EventRacer, DroidRacer, CAFA, SIERRA, DCatch, FCatch, NodeRacer |
| Specification | "typestate", "protocol", "lifestate specification", "repair policy", "execution filter", "order relationships", "concurrency properties" | Strom & Yemini; Mishra et al.; Lifestate; EventRaceCommander; LOOM; CFix; GPredict |
| Platform declaration | "ordering dependency", "requirement dependency", "readiness notification", "readiness probe", "readiness gate", "condition: service_healthy", "satisfied", "supplier/consumer", "lifecycle state ≥ X" | systemd, Kubernetes, Compose, OSGi DS, Linux, AndroidX |

**"Contract" appears in none of the 52 entries** as the name of the order or readiness requirement (recorded at abstract level: errata E1–E2, §10). T2 found "contract" for concurrency only as SCOOP preconditions-as-waits and as atomicity usage protocols (ICST 2017), not for readiness.

### 4.2 Three ways to name the violation

| Strategy | Words | Works |
|---|---|---|
| By order shape | "order violation", "atomicity violation", "race", "event race", "unserializable interleaving", "untimely message" | Lu 2008, Node.fz, TaxDC, CTrigger, all HB detectors |
| By symptom / what happened too early | "access-before-definition", "late-event-handler-registration", "form-input-overwritten", "use-free race", "use-before-initialization", "uninitialized read", "time-of-fault", "resume and restart errors", "slow joiner", "late-joining" | InitRacer, CAFA, UBITect, ConMem, FCatch, KREfinder, ZeroMQ, ROS 2 |
| By the violated object | "protocol violation", "property violation", "invariant violation", "API misuse" | Lifestate, Mishra et al.; GPredict; AI; Amann et al. |

### 4.3 Learned vs specified vs built-in

Each row is counted once, under its main kind.

| Kind | Count (of 52) | Rows |
|---|---|---|
| L (learned or statistical) | 9 | AVIO (2), Falcon (5), Unicorn (6), DefUse (7), PSet (8), CCI (9), AI (10, 12), Engler (51) |
| B (built-in rules or patterns) | 16 | CTrigger (3), ConMem (4), Xiong/SyncFinder (11), WASABI (16), Petrov (17), EventRacer (18), InitRacer (19), NodeRacer (22), DroidRacer (23), CAFA (24), SIERRA (25), KREfinder (26), DCatch (31), FCatch (32), UBITect (34), ExceptioNULL (35) |
| S (specified per program or system by a person) | 15 | LOOM (13), CFix (14), EventRaceCommander (15), Mishra et al. (27), Lifestate (28), typestate (33), systemd (36, 37), Kubernetes (38), Compose (39), OSGi (40), Linux (41), ROS 2 QoS (47), Monperrus directives (49), GPredict (52) |
| Framework- or language-defined rule | 5 | AndroidX (29), C++ (42, 43), Go (44), Java (45) |
| E (empirical classification) | 4 | Lu 2008 (1), Node.fz (20), Wang ASE 2017 (21), TaxDC (30) |
| By design or no requirement | 2 | ZeroMQ (46), epoll/select (48) |
| Mixed | 1 | Amann et al. (50): detectors are L/B, the benchmark is S |

Notes:
- Several works mix kinds: DCatch is B plus triage; Mishra et al. is S plus B; CFix takes its order relationships from detector reports.
- **Among the bug-finding papers, six are S**: LOOM, CFix, EventRaceCommander, Mishra et al., Lifestate, GPredict. **None of them targets start-up readiness**, and **none records where each requirement came from**.
- Kairos's κ is S **with provenance per entry**. Systems configurations come closest to recording provenance, because the configuration itself is the source [I].

### 4.4 Where "readiness" is used, and its collisions

- **Used:**
  - Kubernetes (readiness probe, readiness gates, `Ready`, `ContainersReady`, `PodReadyToStartContainers`);
  - systemd ("readiness notification", `READY=1`);
  - ZeroMQ ("connected and ready");
  - Compose (via "healthy");
  - OSGi says "satisfied", not "ready".
- **Not found** as a term in the bug-detection papers above. One extended search for readiness / not-ready initialization-ordering bug studies at ICSE/FSE/ASPLOS returned only blogs, issue trackers, patents and a microservices issue study that is not about ordering [negative result].
- **Collision 1: I/O readiness.** `select(2)`: descriptors "become 'ready'". `epoll(7)`: "the set of file descriptors that are 'ready' for I/O". OS reviewers may read "readiness" as I/O readiness [I]. Remedy: always say "resource readiness" or "lifecycle readiness" on first use.
- **Collision 2: systemd's "requirement".** In systemd a requirement dependency means "pull in", explicitly *without* order. If Kairos called κ a "readiness requirement", systemd-literate readers could misread it [I]. "Readiness precondition" avoids this.

### 4.5 What the absence of "contract" implies

- **It is not wrong, but it is non-standard here** (withdrawn in this form: erratum E3, §10). In this literature the requirement is an intention, a learned invariant, an HB model or a specification. A reviewer from this community will read "contract" in its DbC sense (pre/post with blame; T2 §3.2).
  - That sense fits R2: a documented precondition violated by the caller.
  - It does not fit C1, where OSAL documents not-found as a normal result (as an `@retval`, neither normal nor an error: erratum E6).
  - It does not fit the SB loss, where SB's documented behaviour is met (outside R2's window only: erratum E6). T2 §0 item 4 makes the same point.
- **What "contract" does get right [I].** It signals that the requirement comes from **outside the runs**, which the learned-invariant line lacks. T1's analysis of AI makes this the decisive property for R2.
- **"Specification" carries the same signal** with less baggage. Lifestate and typestate use it, and it is the word specification-mining papers set against "learned".

### 4.6 Tolerance vocabulary (analogues of `Allowed`), and one collision

| Kairos `Allowed` entry | Nearest named mechanism in the census | Source |
|---|---|---|
| `retry_until_ready(codes, bound)` | Linux `-EPROBE_DEFER` / deferred probe list ("try to call it again later"); WASABI "retry" (and "retry bugs" when it is wrong) | V-doc; V |
| `defer_by_design(src)` | Kubernetes readiness gates and init containers; Compose `condition: service_healthy`; systemd `Type=notify` (start follow-up units only after `READY=1`) | V-doc |
| `documented_drop(src)` and LOSS-UNDER-ORDER | ZeroMQ "slow joiner" (loss accepted unless synchronized: "assume that the published data stream is infinite"); ROS 2 "volatile" durability for "late-joining" subscriptions | V-doc |
| `returned_and_tested(codes)` | API directives / documented error results (Monperrus); OSGi "unsatisfied → deactivated" (a documented result is framework-level, not an acceptance: erratum E5) | V; V-doc |
| repair or avoidance at run time (not part of κ) | AI "tolerating" by stalling; EventRaceCommander "postponing or discarding selected events"; TaxDC fixes "ignoring/neutralizing the untimely message" | V (text); V; P |

**Collision.** In the AI / Atom-Aid / ConAir line, **"tolerating concurrency bugs"** means a *run-time system that avoids the bug* (AI text p.330: "tolerating concurrency bugs during production runs").
- Kairos's `TOLERATED` means something else: **the program's own documented handling makes the not-ready outcome acceptable**.
- A Shan Lu-lineage reviewer could misread "Kairos tolerates X" or "X is tolerated" [I].
- [S] In prose, write "the not-ready outcome is **handled as documented**" or "**an allowed not-ready outcome**". Keep `TOLERATED` only as the enum value, defined once.

### 4.7 Crosswalk: Kairos terms → nearest census term, and collision risk

| Kairos term (spec) | Nearest census term(s) | Risk | Note [I] |
|---|---|---|---|
| readiness contract κ | lifestate specification; typestate; API directive; readiness probe/condition | **medium** | No row records "contract" as the requirement term, at abstract level (errata E1–E3); DbC connotation (T2) |
| `Req` (required lifecycle state at the use) | typestate; `Lifecycle.State.isAtLeast`; OSGi "satisfied"; Kubernetes `Ready` | low | "Readiness precondition" is accurate (T2 §9.2) |
| `Allowed` | documented error results; deferred probe; durability QoS; repair policy | low | Split into supplier-documented vs client tolerance (T2); spec revision 1.1 makes the supplier side a separate judgement (erratum E5) |
| `TOLERATED` | AI's "tolerating" (different meaning) | **medium** | §4.6 |
| `VIOLATING` / "violation" | order violation; protocol violation; invariant violation | low | Say "readiness violation" |
| `NOT_READY_UNJUDGED`, LOSS-UNDER-ORDER | "slow joiner"; "late-joining"; late-event-handler-registration | low | Cite ZeroMQ/ROS 2 for "loss by design unless durability is requested" |
| provider | supplier (Linux device links); dependency (systemd, Compose); target service (OSGi) | low | — |
| publication / publish | JCIP "safe publication"; pub/sub "publish" (SB itself) | low–medium | In cFS, "publish" also means SB message publication. Say "identity publication" or "registration" for AppId |
| shape UNORDERED | race (HB-unordered pair) | low | — |
| shape WRONG-WAY | (no standard name) nearest: order violation with the wrong order enforced; initialization-order bug; use-before-initialization | **needs a definition** | This is Kairos's distinctive case (R2) |
| shape ABSENT | missing provider; access-before-definition when the definition never comes | low | — |
| shape STALE | use-after-free; CAFA "use-free" (P); stale reference | low | — |
| `fixed-order` | (none) Lu 2008 and TaxDC study *non-deterministic* bugs | **medium** | Reviewers may say "not a concurrency bug". The σ- and M-qualification is the answer (§5.1) |
| `schedule-dependent` | non-deterministic; "untimely" (TaxDC); timing-dependent | low | — |
| `invariant` (spec §1.11, §6) | learned invariant (AI, DefUse, AVIO) | low (internal use) | Never apply it to κ |

---

## 5. Our cases in census vocabulary

### 5.1 R2 (cFE #2663): fixed-order, wrong-way

- **Facts** (task context; spec §1.11):
  - ES and EVS use SB's AppId before SB publishes it: 24 uses per boot, 3 of 3 boots (gdb and an instrumented build).
  - The uses are HB-ordered **before** the publication by core serialization.
- **Census naming:**
  - In Lu's terms it is an **order violation**: the desired order (publish before use) is flipped.
  - But Lu's definition adds "not enforced during execution" (via OSTEP, P). In R2 the *wrong* order is enforced.
  - It is **not a race** in any HB detector's sense (WebRacer, EventRacer, DroidRacer, CAFA, SIERRA, DCatch): the pair is ordered.
  - It is **not** an "untimely" event in TaxDC's sense, since nothing varies.
  - Closest named classes: **initialization-order bug** (C++), **use-before-initialization** (UBITect, but sequential and intra-thread), and InitRacer's **access-before-definition**, with the race replaced by a fixed order.
- **Learned-invariant detectors cannot name it.** AVIO, DefUse, PSet, CCI and AI all learn the wrong order as normal (T1 §0).
- **Suggested phrasing [S]:** "a **wrong-way readiness violation**: the use is happens-before-ordered before the provider's transition, so the violation recurs in every execution under the given scheduling semantics (fixed-order)".
  - It is still a concurrency-relevant defect: the order is produced by inter-task serialization (σ), not by sequential program order within one task.
  - Under other σ (e.g. real-time priorities) the class is re-derived. This is the answer to "not a concurrency bug" [I].

### 5.2 C1 (CF #184): lookup-before-create, tolerated by retry

- **Census naming:**
  - **access-before-definition** over a named-object registry (InitRacer's term, lifted);
  - **initialization race** when the order varies;
  - the tolerance is a **retry** (WASABI's object of study) and in kernel terms a **deferred probe** ("resources that are not yet available").
  - A first-lookup failure inside the retry bound is an **allowed not-ready outcome**, not a violation. That is why Kairos needs `Allowed`. Neither the HB race detectors nor GPredict's user-written properties have it (R04 L553: GPredict has "no notion of a legal error outcome"; an abstract-level inference: erratum E7).
- **Suggested phrasing [S]:** "a schedule-dependent readiness condition whose not-ready outcome is handled by a bounded retry; it becomes a violation only if the retry bound can be exhausted".

### 5.3 SB subscribe-before-first-send: silent loss

- **Census naming:**
  - InitRacer's **late-event-handler-registration** (an event fired before its handler exists);
  - ZeroMQ's **"slow joiner"** (text: "the subscriber will always miss the first messages that the publisher sends");
  - ROS 2's **"late-joining" subscriptions** under "volatile" durability ("no attempt is made to persist samples").
- **Implication [I].** In mainstream pub/sub, this loss is **by design** unless durability or synchronization is configured.
  - So a reviewer will accept it as a defect only where a reference requires delivery.
  - Kairos already encodes exactly this: LOSS-UNDER-ORDER with `unspecified_by_reference`.
  - Calling these "contract violations" would contradict the systems norm. Calling them "late-joiner losses (unjudged)" matches it.

---

## 6. What a PLDI/ICSE/FSE/ASPLOS reviewer would expect

These are inferences from the census, not survey data.

### 6.1 Expected vocabulary by community [I]

- **ASPLOS/OSDI (systems; the Lu, Gunawi, Narayanasamy line).**
  - "Order violation" and "atomicity violation" (Lu 2008) are the lingua franca. So are "untimely", "timing condition" and "DCbug" (TaxDC, DCatch) and "time-of-fault" (FCatch).
  - "Readiness" will be read in the Kubernetes/systemd sense, which is good, or as I/O readiness, which is a risk.
  - These reviewers will ask how the work relates to learned-invariant tolerance (AI, PSet, ConAir) and to the HB detectors.
- **PLDI (languages and analysis).**
  - "Happens-before", "predictive", "sound", "typestate", "protocol", "specification".
  - "Contract" in PL usually means behavioural contracts with blame. T2 covers DbC and A/G contracts; higher-order contracts were not checked here (UNVERIFIED).
- **ICSE/FSE (software engineering).**
  - "Event race", "initialization race", "API misuse", "usage constraint", "API directive", "specification mining" vs "likely invariants".
  - FSE readers know AI (FSE 2014) and DefUse.

### 6.2 Recommended vocabulary for the paper

This is consistent with T2 §9.2, which this census supports.

| Concept | Recommended term | Korean gloss [S] | Anchor citations |
|---|---|---|---|
| bug class (title/abstract) | **readiness and order violations** (keep the spec's title wording); define a readiness violation as a subclass of order violation | 준비 상태·순서 위반 | Lu 2008 (V); InitRacer (V) |
| κ | **readiness specification** (per use pattern) | 준비 상태 명세 | Strom & Yemini (V); Lifestate (V); Monperrus (V) |
| `Req` | **readiness precondition** (a framework typestate precondition on a resource at a use) | 준비 사전조건 | typestate; AndroidX `isAtLeast` (V-doc); OSGi "satisfied" (V-doc) |
| order that establishes `Req` | **ordering** (happens-before / must-before), stated as orthogonal to the precondition | 순서 관계 | systemd "orthogonal" (V-doc); HB literature (R04) |
| violation | **readiness violation**; informal short form **use-before-ready** | 준비 위반 (준비 전 사용) | — |
| shapes | wrong-way / unordered / absent / stale (keep, define once) | — | race (HB detectors); use-free (CAFA, P) |
| acceptable not-ready results | **allowed not-ready outcomes**, split into *documented outcomes* (supplier) and *client handling* (retry, defer); see erratum E5 | 허용된 미준비 결과 | `-EPROBE_DEFER` (V-doc); WASABI (V); ZeroMQ/ROS 2 (V-doc) |
| SB loss | **late-joiner loss** (unjudged unless a reference requires delivery) | 늦은 구독 손실 | InitRacer; ZeroMQ; ROS 2 |
| documentation source | **API contract** / **API directive**, used *only* for documentation-backed entries | API 계약 / API 지시문 | T2 §9.2; Monperrus |
| learned comparators | **likely / learned invariants** (AI, DefUse, AVIO, PSet) | 학습된 불변식 | AI (V); DefUse (V) |

**Words to avoid or qualify:**
- "invariant" for κ (κ is a specification object, not a predicate; "it means learned" is overstated: erratum E4);
- unqualified "contract" for κ (DbC; T2);
- unqualified "tolerated" in prose (AI's sense);
- "readiness" without "resource" or "lifecycle" on first use (I/O sense);
- "requirement" for `Req` (systemd's "requirement" means no order).

### 6.3 Paste-ready definition paragraph [S]

> We call a *readiness violation* a use of a framework resource (a registry name, an application identity, a message route, a table) while the resource has not reached the lifecycle state that the use requires, with an outcome the framework does not document as acceptable. Readiness violations are order violations in the sense of Lu et al. [ASPLOS 2008]: the desired order between the provider's state transition and the use is flipped. They generalize the initialization races of InitRacer [OOPSLA 2017] (access-before-definition, late event-handler registration) from web pages to tasks of a flight-software framework. We state the requirement as a *readiness specification*: a typestate precondition [Strom and Yemini 1986] on the resource at the use, the not-ready outcomes that the framework's documentation allows, and the provenance of each part. As in systemd, where ordering dependencies are "independent of and orthogonal to" requirement dependencies, the specification fixes what must hold at the use, not the order that makes it hold; Kairos derives that order from happens-before and must-before analysis. Unlike learned order invariants (AVIO, DefUse, PSet, AI), readiness specifications are not inferred from runs. This matters because some violations, such as cFE #2663, occur in every run and would be learned as normal. *[Erratum E5, §10: read "an outcome the framework does not document as acceptable" and "the not-ready outcomes that the framework's documentation allows" as caller-side acceptance; the framework's own documented result is reported separately.]*

### 6.4 Related-work sentence that uses census vocabulary [S]

> Order-violation detectors either learn the intended order from correct runs (access-interleaving invariants, definition-use invariants, predecessor sets, Anticipating Invariants) or treat unordered conflicting operations under a framework happens-before model as candidates (EventRacer, DroidRacer, CAFA, SIERRA, DCatch), or check user-written properties on predicted traces (GPredict). The first absorbs violations that recur in every run. The second cannot report a use that is ordered, but ordered the wrong way. None of them has a notion of a documented not-ready outcome (errata E5 and E7, §10), such as a bounded retry (cf. the Linux kernel's deferred probing) or a by-design loss to a late-joining subscriber (cf. ZeroMQ's "slow joiner", ROS 2 volatile durability).

---

## 7. Proposed STATUS entry (for the parent to merge)

- **T4 term census:** done. Report: `scratchpad/arch/terms/T4_term_census.md`.
  - 52 entries across multithreaded, event-driven, Android, distributed, sequential-init, systems, API-doc and predictive-property work.
  - Each entry has a status tag and records whether the wording was seen in text or only in a summary.
- **Finding:**
  - No race, order-violation or initialization-race work names its requirement a "contract" (at abstract level, in this census: errata E1–E3, §10).
  - The requirement is an intention (Lu 2008), a learned invariant (AVIO, DefUse, PSet, AI), a built-in HB model (event and distributed detectors), or a specification (typestate, Lifestate, repair policies).
  - "Readiness" is standard in systems and operations (Kubernetes, systemd, ZeroMQ) but not in bug-detection papers. It collides with I/O readiness.
- **Recommendation** (consistent with T2):
  - κ becomes "readiness specification"; Req becomes "readiness precondition".
  - The violation is a "readiness violation", positioned as a subclass of order violation that generalizes initialization races.
  - Keep "contract" only as "API contract" for documentation-backed entries.
  - Avoid "invariant" for κ. Qualify "tolerated", which collides with AI's bug toleration.
  - Cite systemd's ordering/requirement orthogonality for the Req vs HB separation.
  - Cite ZeroMQ "slow joiner" and ROS 2 "late-joining" for the SB loss being by design unless a reference requires delivery.
- **Corrections to inputs:**
  - Unicorn ICST 2012 (not ICSE); NodeRacer ICST 2020 (not ICSE).
  - LOOM authors Wu, Cui, Yang.
  - DefUse's title spells "DeFuse".
  - CAFA "use-free race": UNVERIFIED → P (keyword).
  - "Intended order" not found for Lu 2008.
- **PI decision needed:** whether to adopt the §6.2 vocabulary. This is the same decision as T2 §9.4.

---

## 8. References and status

Bibliographic status reflects this session's searches. Where R04/R05 or T1/T2 already verified an entry, that is noted.

| Key | Reference | Status | How verified |
|---|---|---|---|
| Lu08 | S. Lu, S. Park, E. Seo, Y. Zhou. Learning from Mistakes — A Comprehensive Study on Real World Concurrency Bug Characteristics. ASPLOS 2008 | V (biblio); wording: "order intentions" V-abs, "desired order" P | Search (MSR page, KIT bibtex, UChicago award news); OSTEP ch. 32 via summary |
| AVIO06 | S. Lu, J. Tucek, F. Qin, Y. Zhou. AVIO: Detecting Atomicity Violations via Access Interleaving Invariants. ASPLOS 2006 | V | AI ref list [13] (text); R04 V-abs |
| CTrigger09 | S. Park, S. Lu, Y. Zhou. CTrigger: Exposing Atomicity Violation Bugs from Their Hiding Places. ASPLOS 2009, pp. 25–36 | V | Search (abstract, references) |
| ConMem10 | W. Zhang, C. Sun, S. Lu. ConMem: Detecting Severe Concurrency Bugs through an Effect-Oriented Approach. ASPLOS 2010 | V | Search (author PDF listing, UW seminar) |
| Falcon10 | S. Park, R. Vuduc, M. J. Harrold. Falcon: Fault Localization in Concurrent Programs. ICSE 2010, pp. 245–254, DOI 10.1145/1806799.1806838 | V | Search (arXiv reference list, academia) |
| Unicorn12 | S. Park, R. Vuduc, M. J. Harrold. A Unified Approach for Localizing Non-deadlock Concurrency Bugs. ICST 2012, pp. 51–60, DOI 10.1109/ICST.2012.85 | V | Search (arXiv reference list, academia abstract) |
| DefUse10 | Y. Shi, S. Park, Z. Yin, S. Lu, Y. Zhou, W. Chen, W. Zheng. Do I Use the Wrong Definition? DeFuse: Definition-Use Invariants for Detecting Concurrency and Sequential Bugs. OOPSLA 2010, pp. 160–174 | V | Search (Tsinghua page); AI ref [28] and AI text p.337 |
| PSet09 | J. Yu, S. Narayanasamy. A Case for an Interleaving Constrained Shared-Memory Multi-Processor. ISCA 2009 | P | AI ref [35] and text; R04 P |
| CCI10 | G. Jin, A. Thakur, B. Liblit, S. Lu. Instrumentation and Sampling Strategies for Cooperative Concurrency Bug Isolation. OOPSLA 2010 | P | AI ref [7] (text) only |
| AI14 | M. Zhang, Y. Wu, S. Lu, S. Qi, J. Ren, W. Zheng. AI: A Lightweight System for Tolerating Concurrency Bugs. FSE 2014, pp. 330–340, DOI 10.1145/2635868.2635885 | V | PDF read in full; T1 |
| Xiong10 | W. Xiong, S. Park, J. Zhang, Y. Zhou, Z. Ma. Ad Hoc Synchronization Considered Harmful. OSDI 2010 | V | Search (USENIX listing) |
| LOOM10 | J. Wu, H. Cui, J. Yang. Bypassing Races in Live Applications with Execution Filters. OSDI 2010 | V | Search (USENIX bibtex, Columbia page) |
| CFix12 | G. Jin, W. Zhang, D. Deng, B. Liblit, S. Lu. Automated Concurrency-Bug Fixing. OSDI 2012, pp. 221–236 | V | Search (USENIX, UW page) |
| ERC17 | C. Q. Adamsen, A. Møller, R. Karim, M. Sridharan, F. Tip, K. Sen. Repairing Event Race Errors by Controlling Nondeterminism. ICSE 2017, pp. 289–299 | V | Search (author page, NSF PAR) |
| WASABI24 | B. A. Stoica, U. Sethi, Y. Su, C. Zhou, S. Lu, J. Mace, M. Musuvathi, S. Nath. If At First You Don't Succeed, Try, Try, Again…? Insights and LLM-informed Tooling for Detecting Retry Bugs in Software Systems. SOSP 2024 | V | Search (MSR publication page and blog) |
| Petrov12 | B. Petrov, M. Vechev, M. Sridharan, J. Dolby. Race Detection for Web Applications. PLDI 2012, pp. 251–262, DOI 10.1145/2254064.2254095 | V; tool name "WebRacer" UNVERIFIED | Search (OpenAlex, bibsonomy) |
| EventRacer13 | V. Raychev, M. Vechev, M. Sridharan. Effective Race Detection for Event-Driven Programs. OOPSLA 2013, pp. 151–166 | V; "uncovered races" V-doc | Search; EventRacer README |
| InitRacer17 | C. Q. Adamsen, A. Møller, F. Tip. Practical Initialization Race Detection for JavaScript Web Applications. OOPSLA 2017, PACMPL 1, Art. 66, DOI 10.1145/3133890 | V; type definitions P | Search (author page, SPLASH page); repo README |
| NodeFz17 | J. Davis, A. Thekumparampil, D. Lee. Node.fz: Fuzzing the Server-Side Event-Driven Architecture. EuroSys 2017 | V; definitions P | Search (author PDF, acolyer blog, VT news) |
| WangNode17 | J. Wang, W. Dou, Y. Gao, C. Gao, F. Qin, K. Yin, J. Wei. A Comprehensive Study on Real World Concurrency Bugs in Node.js. ASE 2017 | P | Reference list in an arXiv paper only |
| NodeRacer20 | A. T. Endo, A. Møller. NodeRacer: Event Race Detection for Node.js Applications. ICST 2020 | V | Search (author page, researchr); README |
| DroidRacer14 | P. Maiya, A. Kanade, R. Majumdar. Race Detection for Android Applications. PLDI 2014, pp. 316–325 | V | Search (IISc eprints) |
| CAFA14 | C.-H. Hsiao, J. Yu, S. Narayanasamy, Z. Kong, C. L. Pereira, G. A. Pokam, P. M. Chen, J. Flinn. Race Detection for Event-Driven Mobile Applications. PLDI 2014, pp. 326–336 | V; "use-free race" P | Search (UMich PDF listing, ResearchGate) |
| SIERRA18 | Y. Hu, I. Neamtiu. Static Detection of Event-based Races in Android Apps. ASPLOS 2018, DOI 10.1145/3173162.3173173 | V | Search; R05 V-abs |
| KRE16 | Z. Shan, T. Azim, I. Neamtiu. Finding Resume and Restart Errors in Android Applications. OOPSLA 2016, pp. 864–880, DOI 10.1145/2983990.2984011 | V | Search (NJIT, SPLASH) |
| Mishra16 | A. Mishra, A. Kanade, Y. N. Srikant. Asynchrony-Aware Static Analysis of Android Applications. MEMOCODE 2016, pp. 163–172 | V | Search (IISc eprints) |
| Lifestate19 | S. Meier, S. Mover, B.-Y. E. Chang. Lifestate: Event-Driven Protocols and Callback Control Flow. ECOOP 2019, LIPIcs 134, 1:1–1:29, DOI 10.4230/LIPIcs.ECOOP.2019.1 | V | Search (Dagstuhl, CU page); T2 |
| TaxDC16 | T. Leesatapornwongsa, J. F. Lukman, S. Lu, H. S. Gunawi. TaxDC: A Taxonomy of Non-Deterministic Concurrency Bugs in Datacenter Distributed Systems. ASPLOS 2016 | V; definitions P | Search (ACM DL entry, ResearchGate, blog) |
| DCatch17 | H. Liu, G. Li, J. F. Lukman, J. Li, S. Lu, H. S. Gunawi, C. Tian. DCatch: Automatically Detecting Distributed Concurrency Bugs in Cloud Systems. ASPLOS 2017 | P | R04 (DBLP key via citing paper); patent text this session |
| FCatch18 | H. Liu, X. Wang, G. Li, S. Lu, F. Ye, C. Tian. FCatch: Automatically Detecting Time-of-Fault Bugs in Cloud Systems. ASPLOS 2018 | V; sub-classes P | Search (bibliography, slides, patent) |
| Typestate86 | R. E. Strom, S. Yemini. Typestate: A Programming Language Concept for Enhancing Software Reliability. IEEE TSE 12(1):157–171, 1986, DOI 10.1109/TSE.1986.6312929 | V | Search (DBLP/Frama-C); T2 |
| UBITect20 | Y. Zhai et al. UBITect: A Precise and Scalable Method to Detect Use-before-Initialization Bugs in Linux Kernel. ESEC/FSE 2020, DOI 10.1145/3368089.3409686 | V (full author list not recorded) | Search (UCR PDF, FSE page) |
| ExceptioNULL12 | A. Farzan, P. Madhusudan, N. Razavi, F. Sorrentino. Predicting Null-Pointer Dereferences in Concurrent Programs | P (venue FSE 2012 not confirmed here; R04 V-abs) | Search (Toronto PDF) |
| Monperrus12 | M. Monperrus, M. Eichberg, E. Tekes, M. Mezini. What Should Developers Be Aware Of? An Empirical Study on the Directives of API Documentation. EMSE 17(6):703–737, 2012, DOI 10.1007/s10664-011-9186-4 | V | Search (arXiv 1205.6363, Lancaster) |
| Amann | S. Amann, H. A. Nguyen, S. Nadi, T. N. Nguyen, M. Mezini. A Systematic Evaluation of Static API-Misuse Detectors. IEEE TSE, DOI 10.1109/TSE.2018.2827384 | V (issue year not recorded) | Search (arXiv 1712.00242) |
| GPredict15 | J. Huang, Q. Luo, G. Roşu. GPredict: Generic Predictive Concurrency Analysis. ICSE 2015, pp. 847–857 | V | Search (FSL publication page, Computing Reviews); R04 V-abs |
| Engler01 | D. Engler, D. Y. Chen, S. Hallem, A. Chou, B. Chelf. Bugs as Deviant Behavior: A General Approach to Inferring Errors in Systems Code. SOSP 2001 | V | Search (sosp.org PDF) |
| systemd | `systemd.unit(5)`, `systemd.service(5)`, `sd_notify(3)`, systemd `main` | V-doc | Fetched |
| k8s | Kubernetes docs: Pod Lifecycle; Init Containers (`kubernetes/website` `main`) | V-doc | Fetched |
| compose | Compose Specification, services, `depends_on` (`compose-spec` `main`) | V-doc | Fetched |
| osgi-ds | OSGi Declarative Services Specification (ch. 112 docbook, `osgi/osgi` `main`) | V-doc (release number not determined) | Fetched |
| linux | Linux `driver-api/driver-model/driver.rst`; `driver-api/device_link.rst` (`master`) | V-doc | Fetched |
| cppcg | C++ Core Guidelines I.2, I.22 (`master`) | V-doc | Fetched |
| siof | "Static initialization order fiasco" (isocpp FAQ; cppreference "siof") | P | Search summary only |
| gomem | The Go Memory Model, Version of June 6, 2022 | V-doc | Fetched |
| jcip-cert | B. Goetz et al., *Java Concurrency in Practice*, 2006; SEI CERT TSM01-J | P | Search (TOC; CERT page summary) |
| zguide | ZeroMQ Guide, ch. 1 ("slow joiner"), ch. 5 (`booksbyus/zguide` `master`) | V-doc | Fetched |
| ros2qos | ROS 2 docs, About Quality of Service Settings (`jazzy`) | V-doc | Fetched |
| epoll | Linux man-pages `epoll(7)`, `select(2)` (`mkerrisk/man-pages` `master`) | V-doc | Fetched |
| androidx | AndroidX `Lifecycle.kt` (`androidx-main`) | V-doc | Fetched |

---

## 9. Open items

1. **Full-text confirmation** was not possible in this environment. To be checked:
   - Lu 2008's exact definition sentence (and whether "intended order" occurs anywhere);
   - InitRacer's definitions of the three types;
   - CAFA's "use-free race" definition;
   - TaxDC's order-violation sub-classes;
   - DroidRacer's "enable" operation;
   - WebRacer as a tool name.
2. **Higher-order / behavioural "contracts" in PL** (for PLDI reviewers) were not searched here. T2 covers DbC, JML, A/G, SCOOP and session-type-like behavioural contracts.
3. **Aerospace partitioned-OS vocabulary** (ARINC 653 initialization vs normal mode) was not checked. It may be the most natural vocabulary for flight-software reviewers.
4. **The negative result on "readiness"** in bug-detection venues rests on one extended web search. A DBLP title search for "readiness" would strengthen or refute it.

---

## 10. Errata (2026-10-15)

These errata apply the external review EXT-20261012 §6.2–§6.4 (`../../history/review_EXT_20261012.md`) as `../../spec/SPEC_FINAL.md` revision 1.1 answers it (spec §0.7 item 4, §1.6 "Three judgements", §10.1 QT-2–QT-5, ADR-043). The text above is kept as written on 2026-10-12; the markers "erratum En" point here. The synthesis `../08_terminology.md` (revision 3) carries the same corrections. The word "contract" stays in every document while the renaming (T1) is pending.

- **E1. What the census counted** [F: this note]. The 52 rows are 37 distinct research papers (AI is rows 10 and 12) and 14 platform, language or guideline documentation rows (rows 29 and 36–48). The "Requirement term" column records one name per row: the name the work gives the required order or readiness requirement. For the papers other than AI, that name comes from a search summary of the abstract or from AI's description of the work (§1); no paper body was searched for "contract" or any other word. Seven rows record no requirement term (rows 3, 4, 5, 6, 21, 35, 43). So §0 item 1, §3.3 ("None uses 'contract'"), §4.1's last paragraph, §4.7's first row and §7's "Finding" hold only in this scope: no row records "contract" as the requirement's name at abstract level. They do not show that the works do not use the word.
- **E2. GPredict** [U]. GPredict (row 52) calls the Java Iterator usage rule "a more generic contract" in its Introduction. This is known from the external review's reading of the printed first page (EXT-20261012 §6.3) and from a web-search snippet; the PDF was not opened, so the wording is unverified (PI decision P-PAPERS). Row 52 records only "concurrency properties". The phrase shows that a predictive-analysis paper in this census uses "contract" in its prose for an API usage rule. In which sense it does so is not classified until the paper is read.
- **E3. Consequence for "contract"** [I]. §4.5's "non-standard here" is withdrawn in that form. Term frequency alone is no reason to exclude "contract" (EXT-20261012 §6.3). The recommendation of §6.2 does not rest on the census (`../08_terminology.md` §2.4, §5.2), and the decision is the PI's (T1).
- **E4. "Invariant"** [F: AI p. 331; I]. §0 item 6 ("'Invariant' means learned in this literature"), §3.1's takeaway ("it means learned from correct runs") and §6.2 ("it means learned") overstate. An invariant is a predicate that should hold at given points; it can be written, proved or inferred, and learning is not part of its definition. AI itself separates "program invariants", predicates "that should always be true at certain points of the execution", from the "likely invariants" learned from testing runs (p. 331; T1 §7 item 1). In the learned-invariant line (AVIO, DefUse, PSet, AI) the checked object is a learned invariant. The reason not to call κ an invariant is that κ is a specification object, not a single predicate: it bundles the required state (`Req` with its `Oracle`), the not-ready outcomes that its references accept (Allowed, each with a reference horizon), what the use must achieve (`appreq`), the harm, the provenance of each part and the scope (spec §1.6). Confusion with learned-invariant tools is a secondary, reader-side point [I].
- **E5. Supplier results and caller acceptance** [D in the spec]. §4.6, §4.7, §6.2, the paste-ready definition of §6.3 ("an outcome the framework does not document as acceptable"; "the not-ready outcomes that the framework's documentation allows") and §6.4 ("a notion of a documented not-ready outcome") merge two judgements that spec revision 1.1 keeps apart (§1.6 "Three judgements", §1.10). What the supplier documents and returns is the framework-level judgement (`fw_doc` per instance, `fw_conf` per cell); it never makes an occurrence TOLERATED. Which not-ready outcomes are accepted is the caller-level judgement, made from references of the caller, the consumer or the mission (`appreq` and the caller-level Allowed). The observed consequence is recorded apart. Read §6.3 as "… with an outcome that no caller-, consumer- or mission-side reference accepts; the framework's own documented result is reported separately".
- **E6. Facts** [F]. §4.5: OSAL documents `OS_ERR_NAME_NOT_FOUND` as an `@retval` "if the name was not found in the table" (`osapi-countsem.h:160`), without calling it normal or an error. §4.5's "SB's documented behaviour is met" holds outside R2's window only: inside it the `SEND_NO_SUBS` events that cSB4305.5 requires are rejected by EVS (spec App. A.1 step 13a). §0 item 5 and §5.3: LOSS-UNDER-ORDER with `unspecified_by_reference` is the caller-level label; cSB4305.5's "not considered an error" is reported beside it as the framework-level result `documented-not-error`, not as an allowed drop (spec §3.9; PI item QT-2, which re-asks STATUS decision T2).
- **E7. GPredict and "legal error outcomes"** [I], [U]. §5.2 and §6.4 rest on R04 L553 ("no notion of a legal error outcome"), which is an abstract-level inference of R04 §5. GPredict's property formalism (adapted from MOP, P) is unverified (P-PAPERS), and spec §8.3 infers that finite-horizon Allowed entries can be encoded in a GPredict-style property. Nothing here says what GPredict can or cannot express (spec §6.4 item 37).

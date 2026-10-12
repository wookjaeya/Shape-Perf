# T1: The AI paper (Zhang et al., FSE 2014) and what it implies for "readiness contract"

**Scope.** I read the paper in full and mapped it onto Kairos (`spec/SPEC_FINAL.md` §1.4, §1.6, §1.8, §1.10, §1.11, §3.15, §4.13, §7.2, §8.3, App. A.1, A.4). This is part of the PI's question of whether "contract" (계약) is the right word for Kairos's object.

**Tags.**
- `[F:AI §x p.N]` is a fact taken from text I read in the paper. Page numbers are the proceedings pages, 330–340.
- `[F:code file:line]` is a cFS v7.0.1 line that I re-read in `/home/user/work/procrace/cfs_ref/cFS` for this note.
- `[F:spec §]` is a statement taken from the Kairos spec.
- `[I]` is my inference. `[S]` is a suggestion.

Citation status:
- **V**: authors, title, venue and year were confirmed by a search result, or by the PDF itself.
- **P**: the bibliographic data is confirmed, but some content is known only from a summary or a secondary source.
- **UNVERIFIED**: not confirmed.

**How the paper was read.** I viewed all 11 pages as page images and extracted the text with `pdftotext`. I re-rendered Table 2 at 300 dpi to transcribe its cells exactly. Every quotation below was checked against the extracted text.

---

## 0. Answers in brief

| Question | Answer |
|---|---|
| (a) AI "invariant" vs Kairos "contract" | **They are different kinds of object.** AI's invariant is a **likely invariant learned from verified-correct test runs**: for each static instruction, the set of remote instructions seen to access the same address immediately before it. A Kairos contract is **specified from a reference, with provenance**. It states a required lifecycle state at the use and a finite set of documented not-ready outcomes that are acceptable. Training never changes it. **R2**: the wrong order occurs in every boot and passes any output check, so AI would learn it as normal and could never flag it. AI's premise that "the manifestation is non-deterministic before the turning point" does not hold for a fixed-order violation. **C1**: AI has no notion of a tolerated not-ready outcome. It either stays silent or reports a false positive on a first lookup that the retry tolerates. A stall inside OSAL's name search would hold the type lock that the provider needs. **Silent SB losses** are learned as normal when they happen in every boot. They pass AI's output-based correctness check by construction. |
| (b) Turning point vs LP / capture point | For a **schedule-dependent** violation, AI's turning point (in practice, the execution of the access whose remote predecessor is unexpected) corresponds to the **LP of the failing conjunct**. For a value conjunct this is the **capture point**. Kairos separates two points that AI merges: the **evaluation point** (LP or capture point) and the **intervention point** (the gate, which must have an empty lockset). R2 has no turning point inside a run: its outcome is fixed by the basis B (the core order). |
| (c) AI stalling vs `kz hold` / `kz gate` | The mechanism is similar: delay one thread until another thread's event, with a cap. The purpose and the rules differ. AI stalls to **prevent** an unseen order in production. Kairos holds to **realize** a predicted order, as evidence. Kairos adds what AI lacks: gate placement with an empty lockset, event-based release, a watchdog derived from the program's own reference timeouts, `timeout-exercised` labelling and a perturbation ratio. AI argues that stalls are harmless because "a correctly synchronized program will not depend on time delays" [F:AI §4.5 p.339]. That does not hold for cFS's timed soft barriers and retry bounds. |
| (d) AI-style comparator? | **Yes, add it** [S]. It is cheap to add and it sharpens the "specified vs learned" contrast. Add it as a second variant in the same learned-invariant slot as the DefUse-style comparator (§8.3), on the same training pool. It differs from DefUse in three ways: the predecessor relation ignores whether an access is a read or a write; it has an explicit `nil` (an order violation with no predecessor); and it gives an anticipation point. The paper's Figures 3, 4, 6, 7 and 10 give exact RPre/BSet values, which can serve as golden tests. The original LLVM tool should be recorded as `NOT_RUN` with its blockers. |
| (e) Citation | Cite it as Zhang, Wu, Lu, Qi, Ren, Zheng, "AI: A Lightweight System for Tolerating Concurrency Bugs", FSE 2014, pp. 330–340, DOI 10.1145/2635868.2635885 [V]. An extended version exists: IEEE TSE 42(10):899–917, 2016 [V bibliographic; content not read]. A draft paragraph is in §6.5. |
| What this means for the term "contract" | Relative to this whole line of work (Daikon, AVIO, DefUse, CCI, PSet, AI), "contract" sits on the right side of the specified/learned divide. **"Invariant" would be wrong for Kairos's object on both counts.** It is not learned, and it is not "always true at a point", because a not-ready state at the use is legitimate when the outcome is in Allowed (TOLERATED). The caveats and possible alternatives are in §7. |

---

## 1. Source

- **Paper.** Mingxing Zhang, Yongwei Wu, Shan Lu, Shanxiang Qi, Jinglei Ren, Weimin Zheng. "AI: A Lightweight System for Tolerating Concurrency Bugs." FSE '14, November 16–21, 2014, Hong Kong, China. pp. 330–340. DOI 10.1145/2635868.2635885.
  - **Status: V.** The DOI, pages and venue come from the PDF's first-page footer. The search confirmed the authors, the 22nd ACM SIGSOFT FSE and pp. 330–340 (Tsinghua MADSys publication page).
- **Extended version.** Same authors, "A Lightweight System for Detecting and Tolerating Concurrency Bugs," IEEE TSE 42(10):899–917, 2016, DOI 10.1109/TSE.2016.2531666.
  - **Status: V** for the bibliographic record, confirmed by search. **Content not read**: fetching the PDF failed (DNS). The search summary of its abstract repeats the FSE claims (35 bugs, under 1% overhead, two new bugs). Nothing below relies on the TSE version.
- **Tool.** The paper states (footnote 2, p.331) that the source is at `http://james0zan.github.io/AI.html`. **Availability was not checked**: GitHub API access to that account is not enabled in this session.

---

## 2. Terminology and definitions (text seen)

| Term | Definition in the paper (verbatim unless marked) | Where |
|---|---|---|
| **Program invariant** | "Program invariants are predicates that should always be true at certain points of the execution; they reflect programmers' intentions." | §2, p.331 |
| **Likely invariants** | "Many recent works pay close attention to learn "likely invariants" from testing runs and use them to detect or tolerate software bugs [5, 28, 7]. These invariants are supposed to be held in all the correct runs. If one of them is violated at run time, a bug probably has manifested." [5] = Daikon, [28] = DefUse, [7] = CCI. | §2, pp.331–332 |
| **Interleaving invariants** | Used as a family name, not defined: "What distinguishes AI from previously proposed interleaving invariants [13, 28, 7, 35] …" [13] = AVIO, [28] = DefUse, [7] = CCI, [35] = PSet. The paper also says: "most of the invariant-based techniques share the same formation of learning some invariants from testing runs and checking/guarding them later." | §1.3 p.331; §2.3 p.333 |
| **Static / dynamic instruction** | "S_y … a static instruction in the source code (a line of code that can be differentiated by its program counter), and I_x S_y … the dynamic instruction I_x observed at run time is derived from static instruction S_y." A dynamic instruction is "an execution instance of a static instruction". | §2.1 p.332 |
| **Remote predecessor RPre(I_x)** | "RPre(I_x) is a static instruction, which has at least one dynamic instruction derived from it that 1) accesses the same memory address as I_x; 2) comes from another thread (besides I_x's thread); and 3) accesses the same address immediately before I_x. We consider I2 from Thread 2 to be immediately before I1 from Thread 1 if and only if, except instructions from Thread 1, there is no instruction that accesses the same address of I1 between the execution of I2 and I1. And RPre(I_x) = nil if there is no such dynamic instruction." Further properties: it is defined for every dynamic instruction; `nil` is a special value; "Whether the instruction is a read or a write operation does not matter in calculating remote predecessors." | §2.1 p.332 |
| **`nil` state** | "nil is specially defined to describe the state that the instruction can be executed before any instructions from other threads that access the same address." Elsewhere: AI "explicitly defines nil to represent the initial state", which is "critical in anticipating W-R Order Violations". | §2.1 p.332; §2.3 p.333; §4.2.4 p.338 |
| **Belonging set BSet(S_y)** | "After investigating many concurrency bugs, we observe that: In all the correct runs, the remote predecessor of the same static instruction's dynamic instructions has fixed candidates. And once a dynamic instruction's remote predecessor does not belong to this set, it implies the occurrence of a concurrency bug. Hence we calculate a Belonging Set … for every static instruction, which is the union of all the remote predecessors of its dynamic instructions that have been seen in verified interleavings." | §2.1 p.332 |
| **Anticipating Invariant (AI)** | `RPre(I_x S_y) ∈ BSet(S_y)`, where I_x S_y is derived from S_y. Informal version: "for an instruction I that accesses variable V, AI captures which instructions are allowed to access V right before I from a different thread." | §2.1 p.332; §1.3 p.331 |
| **Turning point** | "there exists a turning point t during the manifestation of a concurrency bug: before t, the manifestation is non-deterministic; after t, the manifestation becomes deterministic. Thus if a concurrency bug can be anticipated before its turning point, its manifestation can be prevented by temporarily stalling a thread". Operationally: "postponing the execution of I2 can often prevent the bug (i.e., the execution of I2 is the turning point)". On the trade-off: "Anticipating too early will inevitably encounter many false positives … Anticipating too late will miss the chance of lightweight bug toleration". The definition is **informal**: prose and Figures 1–2 only. | §1.2 p.331; §2.1 p.332 |
| **AI violation** | "An Anticipating Invariant Violation is reported if it does not [hold], i.e. RPre(I_x) ∉ BSet(S_y) although I_x is derived from S_y." | §3.2 p.334 |
| **Design rationale** | "AI does not wait for the last instruction in a buggy region to detect the invariant violation. It constrains the instant state of each instruction not a continued state of an instruction region". AI "tries to make minimal assumptions about where a bug may hide". | §2.3 p.333 |

**Two worked properties from the figures** (useful as golden tests):
- **Fig. 4.** `BSet(S2) = RPre(I2) ∪ RPre(I4) = {S1, S3}`.
- **Fig. 7.** For the R-W and W-W order violations, the incorrect run has `RPre_in(I1) = nil`, which is not in `BSet(S2a) = {S1a}` nor in `BSet(S2b) = {S1b}`. The bug is caught by `nil`.
- **Fig. 10** (HTTrack W-R). `BSet(S4) = {S2}`. In the incorrect run, the first S4 sees `nil` and is stalled until the second sees `S2 ∈ BSet(S4)`.

---

## 3. Mechanism (text seen)

**Three LLVM passes** [F:AI §3.1 pp.333–334]:
1. **AIPrepare** assigns "an universally unique access ID to each load/store instruction in the LLVM IR (by adding a metadata node)". It stores the marked bitcode and, if a custom strategy is used, a white-list file.
2. **AITrace** adds, before each memory access, a logging call that writes the triplet (access ID, thread ID, address). These traces come from the in-house testing phase.
3. **AITolerate** encodes the synthesized BSets through an initialization function in `llvm.global_ctors` and adds a guard before every shared-memory access.

**Training** [F:AI §3.2 p.334]:
- The quoted procedure: "In order to infer AI automatically without any programmers' annotation, we rely on correct runs observed during the in-house testing phase. As pointed out by prior work [35], programmers can assert whether a test run is correct or not by verifying the outputs."
- Programmers should cover inputs and repeat runs. CHESS or CTrigger "can also be used".
- The BSet update is `BSet(y) = BSet(y) ∪ RPre(Triplet_x)` over the traces, taken in chronological order.
- An address counts as shared if some S accessing it has `BSet(S) − {nil} ≠ ∅` (footnote 3).

**Guard (Algorithm 1)** [F:AI §3.2 pp.334–335]:
- `Recorder[M]` keeps "the last two instructions that access memory M and are from different threads". The RPre is the last access if it came from a different thread, otherwise the second-last.
- Loop while `iter < threshold`: compute RPre. If it is not in the BSet, report a violation and stall "for a while"; otherwise break.
- At `iter = threshold`, report an "unresolved violation". Then update the Recorder.
- "we implement AI with per-variable synchronization". The paper does not say whether the check and the guarded access are atomic with respect to other threads.

**Stalling and its cap** [F:AI §3.2 p.334]:
- "It is possible that the only correct interleaving for an input is untested. If such "fake" violation (i.e. false positive) is not properly treated, it may cause an indefinite stall. Thus, in order to ensure forward progress, we set the maximum stall time to a threshold. Once the threshold is reached, the system will log the violation and resume the stalled thread's execution. The log is sent back to developers … If it is not [a bug], we can update the relevant AI to green-light this new interleaving in future runs."
- **The numeric threshold value is not stated.** The 30% elsewhere in the paper is the bias threshold, not the stall threshold.
- "Our algorithm ensures that stalling does not occur during the tested interleavings".

**Bias and custom instrumentation** [F:AI §3.3 p.335]:
- The premise: "in a well-tested program, bugs usually occur in cold (less-executed) regions. Thus if an access is deemed to be very "hot", we chose to not instrument it."
- Static instructions are grouped by disjoint-set merging, so that groups never share an address. Each group gets an "ins-proportion" (IP). Groups with IP above 30% are not instrumented.
- Annotation macros are `AI_INS_THIS_FUNC`, `AI_INS_THIS_BB` and `AI_INS_THIS_ADDR(void* addr)`. Instrumenting only bug-related instructions after a report is likened to Aviso and LOOM.

---

## 4. Evaluation (text and tables seen)

**Setup** [F:AI §4.1 p.335]:
- 35 real-world bugs from 11 multi-threaded applications:
  - servers: Apache httpd, Cherokee, MySQL;
  - desktop and client: Mozilla, Axel, Pigz, HTTrack, Transmission, PBZip2, ZSNES;
  - SPLASH-2.
- 13 of the 35 are bug kernels, mainly from Mozilla (footnote 5).
- Platform: 12-core Xeon at 2.67 GHz, 24 GB, Ubuntu 12.04.3, LLVM 3.3. MySQL was built with llvm-g++ from LLVM 2.9.

**Table 1 (patterns; † = new bug)** [F:AI p.336]

| Category | Pattern | n | Bugs |
|---|---|---|---|
| Atomicity | R-R \| W | 5 | MySQL#644, #3596, #12228; Mozilla#341323, #224911 |
| | W-R \| W | 2 | pigz#9ef658babd†; MySQL#19938 |
| | W-W \| R | 5 | MySQL#791, #12848; Mozilla#52111, #73761, #622691 |
| | R-W \| R-W | 7 | MySQL#56324, #59464; Apache#48735, #21287; Mozilla#342577, #270689, #225525 |
| | R-R-W \| R-R-W | 3 | Apache#25520, #46215; Cherokee#326 |
| Order | W-R | 10 | Axel#313564†; Httrack#20247; Transmission#1660, #1827; ZSNES#10918; Mozilla#61369; MySQL bug from Bugaboo [15]; FFT, LU, Barnes bugs from LOOM [34] |
| | R-W | 1 | Pbzip2 bug from Yu's homepage [1] |
| | W-W | 2 | MySQL#48930; Mozilla bug from Lu 2008 [12] |

**Table 2 (Detect / Tolerate without rollback)** [F:AI p.336]
- Key: ✓ means all manifestations were detected or tolerated; ✓~ means only on particular interleavings; blank means not violated.
- The caption says: "All results of AI in this table are obtained through experiments. And the results for other detectors are obtained based on our understanding of their algorithms."
- **The AVIO, DUI, CCI and PSet columns are therefore analytical, not runs.**

| Pattern | AI D/T | AVIO D/T | DUI D/T | CCI D/T | PSet D/T |
|---|---|---|---|---|---|
| R-R \| W | ✓ ✓ | ✓ – | ✓ – | ✓ – | ✓ ✓ |
| W-R \| W | ✓ ✓ | ✓ – | ✓ – | ✓ – | ✓ ✓ |
| W-W \| R | ✓ ✓ | ✓~ – | ✓ ✓ | ✓ – | ✓ ✓ |
| R-W \| R-W | ✓ ✓ | ✓~ – | – – | ✓~ – | ✓ – |
| R-R-W \| R-R-W | ✓ ✓ | ✓~ – | ✓~ – | ✓ – | ✓ – |
| W-R (order) | ✓ ✓ | – – | ✓~ ✓~ | ✓~ ✓~ | ✓~ ✓~ |
| R-W (order) | ✓ ✓ | – – | ✓ – | ✓ – | ✓ ✓ |
| W-W (order) | ✓ ✓ | – – | – – | ✓ ✓ | ✓ ✓ |

- **Method** (§4.2 p.336): each buggy program was run 1,000 times on the bug-triggering input, with random sleeps added.

**Overhead (Table 3)** [F:AI p.338]

| App | Default | Bias |
|---|---|---|
| PBZip2 | 0.38% | – |
| Pigz | 0.20% | – |
| Apache | 0.34% | – |
| MySQL | 0.57% | – |
| FFT | 1345% | 115% |
| LU | 1613% | 127% |

- The authors' comparison figures (p.339): PSet's software implementation costs more than 100× for I/O-intensive applications; AVIO's software implementation costs 15×–40×; DUI costs 5×–20×.

**Two new bugs** (§4.3 p.338):
- **Axel:** an order violation on `last_transfer` that causes an unnecessary thread cancel, or an infinite loop if the order is always flipped. Confirmed and fixed.
- **Pigz:** an atomicity violation that reads `pool->made` after releasing `pool->have` in `free_pool()`. Confirmed and fixed in 2.2.5.

**Training cost** (§4.5 p.339): "about 200 for PBZip2, Pigz, FFT, LU; about 1,000 for MySQL; and about 5,000 for Apache". Apache training is done "within half a day".

---

## 5. Limitations

**Stated by the authors** (text seen):
1. **Training sufficiency** (§4.5).
   - False negatives occur "if some shared-memory accesses are not identified during training". False positives (unnecessary stalls) occur "if some important correct interleavings are not covered".
   - The logging (AITrace) and guarding (AITolerate) builds perturb timing differently.
   - A BSet-relaxation tool re-runs the AI-guarded program, checks the output, and adds the reported predecessors.
2. **False positives cost a stall.**
   - Quote: "although the possibility of false positives cannot be eradicated, it will only incur a stalling timeout in our work … These stallings will never affect the program's correctness. Since the threads are randomly scheduled, a correctly synchronized program will not depend on time delays." (§4.5)
   - Also: "not all Anticipating Invariant Violations can be tolerated by just perturbing the thread schedule" (§3.2).
3. **Multiple variables.** The invariant is per address. A possible extension using ColorSafe's colouring (replace the address with a colour) is proposed but not evaluated (§4.2.5).
4. **Context.** It "cannot detect a synthetic bug described in Bugaboo … without recording any context information, AI cannot tell different dynamic instructions apart if they are derived from the same static instruction". The proposed fix is adding context to RPre (§4.2.5).
5. **Coverage.** The bugs cover 7 of the 8 patterns of Park et al.; W1-W2-W1 is argued to be akin to W-W | R (§4.2.5).
6. **Bias instrumentation** may miss bugs in hot code, which is accepted by design (§3.3).

**Not discussed in the paper** (my reading, [I]):
- address reuse after `free`;
- stalling while holding a lock;
- uninstrumented library code (libc);
- whether `memset`/`memcpy` intrinsics count as accesses (AIPrepare marks "load/store" instructions);
- programs whose correctness depends on timeouts;
- deterministic (every-run) defects.

---

## 6. Mapping to Kairos

### 6.1 (a) Learned invariant vs specified contract

| Dimension | AI (FSE 2014) | Kairos readiness contract (§1.6) |
|---|---|---|
| Source of truth | Behaviour seen in "verified" correct runs, with correctness judged by output [F:AI §3.2] | Reference text: API docs (`\par Assumptions`, `\retval`), code admissions, issues, each with `prov` (path:line, sha256) and an `authored` record [F:spec §1.6, §7.2] |
| Object constrained | Which remote static instruction last touched the address before this instruction | The **lifecycle state** of a framework resource at each conjunct's LP (`Req`), plus the **outcome and continuation** when it is not ready (`Allowed` with a reference horizon) |
| Unit | Memory address × static instruction × thread | Resource key `sel(e)` (whose Oracle reads several support locations) × context-qualified template (static actor, call string, site) × actor and the four identities [F:spec §1.2, §1.5] |
| Failure semantics | Two-valued: an RPre is either in the BSet or not | Four-valued per occurrence: READY / TOLERATED / VIOLATING / NOT_READY_UNJUDGED; outcome labels violation / LOSS-UNDER-ORDER / liveness [F:spec §1.6, §1.10] |
| Change over time | BSet grows with training; developers "green-light" new interleavings [F:AI §3.2] | Frozen by hash at F1/F2; refinements are labelled `oracle refinement` and reported apart [F:spec §7.2] |
| Claim scope | The tested interleavings | Exec(B, σ, M) via certificates or bounded exploration; σ_E only from natural witnesses [F:spec §1.10] |
| Intent | "reflects programmers' intentions about the correct order" [F:AI §1.3]: intent is **inferred from tested behaviour** | Intent is **read from the framework's documents**; behaviour is checked against it |

The last row is the decisive one. A learned invariant assumes that tested behaviour equals intended behaviour. That assumption fails exactly when a defect is **systematic**.

**R2 (cFE #2663, fixed-order, all 24 uses in every boot).** All [I], built on the facts listed first.

- **Facts.**
  - The value conjunct's capture point is the load of `CFE_SB_Global.AppId` at argument evaluation in SB code, for example the pending-event send "after final unlock" at `cfe_sb_api.c:220–228` [F:code].
  - `CFE_SB_Global` is zeroed by `memset` in SB's EarlyInit (`cfe_sb_init.c:50`) on the startup thread [F:code].
  - SB's own store comes later, `CFE_ES_GetAppID(&CFE_SB_Global.AppId)` at `cfe_sb_task.c:138` [F:code].
  - EVS returns `CFE_EVS_APP_ILLEGAL_APP_ID` (`cfe_evs.c:185–188`) [F:code], and every SB caller discards it [F:spec A.1 step 8].
  - Per boot: 20 ES uses plus 4 EVS uses (24), observed in 3 of 3 boots under gdb and an instrumented build.
- **What AI would learn.**
  - For ES main's first capture load, the remote predecessor is the EarlyInit `memset` site, or `nil` if the `memset` intrinsic is not traced (the paper only says "load/store instruction"). For EVS's loads, it is the same SB static sites as executed earlier by ES.
  - **Every** training boot reaches OPERATIONAL with no visible error: the event is lost silently. So every boot passes AI's output-based correctness check, and `{nil or S_memset, S_cap, …} ⊆ BSet(S_cap)`.
  - The R2 order is therefore *the* learned normal. AI raises no alarm in any run.
  - A stronger training oracle would not help either: **no correct boot exists** under the default basis B. The only "correct" runs come from a different B′ (another core order), which is a different program.
- **AI's own premise does not hold.** "Before t, the manifestation is non-deterministic" is false for R2. The outcome is fixed before the first task runs, by `cfe_static_module_list.c` (core order) [F:spec §3.6]. In Kairos's terms, R2 is `fixed-order(σ_U, M)` by a MustBefore certificate, with no turning point inside the run.
- **Stalling cannot repair a fixed-order violation.** Suppose a guard were hand-given `BSet(S_cap) = {S_138}`. Stalling ES main at its capture load would then wait for SB's store. But `m_ES ∈ MB(x)` [F:spec A.1 step 6]: SB's task is spawned only after ES main's raise and the startup thread's `MainTaskSyncDelay` at `cfe_es_start.c:835`, and that raise follows the stalled use in ES's program order. The result is a circular wait.
  - If AI's stall cap is below `CFE_PLATFORM_CORE_MAX_STARTUP_MSEC` (30000 ms) [F:spec §3.3], the stall expires and the event is still lost, only later.
  - If the cap is above 30000 ms, the core sync times out and the startup thread panics [F:spec §3.6 item 5].
  - The MB certificate is exactly the proof that no schedule, and so no stall, avoids the violation.
  - This is consistent with the paper's own admission that "not all Anticipating Invariant Violations can be tolerated by just perturbing the thread schedule" [F:AI §3.2].

**C1 (CF #184; retry `CF_STARTUP_SEM_MAX_RETRIES` = 25 × `CF_STARTUP_SEM_TASK_DELAY` = 100 ms around `OS_CountSemGetIdByName`, `cf_cfdp.c:1270–1300`, `cf_internal_cfg.h:173, 185`)** [F:code]. All [I].

- **At memory level AI is likely silent whatever the order.**
  - The lookup's accesses run in generic OSAL code: `OS_ObjectNameMatch` loads `obj->name_entry` and calls libc `strcmp` (`osapi-idmap.c:275–277`) under the global type lock (`OS_ObjectIdGetBySearch` → `OS_ObjectIdTransactionInit(OS_LOCK_MODE_GLOBAL…)`, `:920–931, :975–991`) [F:code].
  - One static load instruction serves every slot, every object type and every caller. Its BSet becomes the union over all of them, so it saturates.
  - `strcmp` in libc is outside the LLVM passes and is not instrumented.
  - This is the paper's own context limitation (§4.2.5), made much worse by framework code: a few generic static instructions serve every resource and every client.
- **If it did fire, it would be a false positive relative to the contract.**
  - Suppose training saw only provider-first boots. A CF-first boot would stall CF *inside* the name search, holding the countsem type lock. The provider's create needs that lock to finalize (`osapi-idmap.c:1092`) [F:spec §1.4].
  - The result is a circular wait that only AI's cap breaks. CF then gets `NAME_NOT_FOUND` and its own retry succeeds.
  - Kairos classifies the single early lookup as `tolerated` (`retry_until_ready`) and the exhaustion case as `timeout-dependent` [F:spec A.4].
  - AI has neither notion. It has no vocabulary for "a not-ready outcome followed by the documented continuation is fine".

**Silent SB losses ("subscribe-before-first-send").** All [I].

- The transmit path's route lookup `CFE_SBR_GetRouteId` runs under the SB lock (`cfe_sb_priv.c:1007–1010`). A missing route increments `NoSubscribersCounter` (`:1085`). Subscribe adds the route under the same lock (`cfe_sb_api.c:967, 995`) [F:code].
- **Every-boot losses** (e.g., ES event packets, MsgId 0x808, sent before any subscription [F:spec A.1 step 13]) are learned as normal, for the same reason as R2.
- **Rare-order losses** would trip the guard only if the lookup's static instruction had not already saturated across MsgIds. They would also stall inside the SB lock that the subscriber needs.
- AI cannot tell `defer_by_design` (TO_LAB) from LOSS-UNDER-ORDER.
- Most basically, AI's training oracle is "verify the outputs". A *silent* loss passes that oracle by definition, so the losses enter the BSet.

**Does AI's model even apply to cFS?** [I] Technically yes: on pc-linux, cFS is one process with about 27 threads [F:spec §1.7], and the resources live in shared user memory. The mismatch is semantic, not technical: granularity, outcome semantics and the training oracle.

### 6.2 (b) Turning point vs linearization point and capture point

| | AI turning point | Kairos LP / capture point | Kairos gate point |
|---|---|---|---|
| Definition | Informal: the point after which manifestation is deterministic; operationally, the execution of the access whose RPre is unexpected [F:AI §1.2, §2.1] | Per conjunct, from code: where a lifecycle effect takes place or where the use reads the state. For value conjuncts, the load from which the argument derives (T-DERIVE) [F:spec §1.4] | The point where a thread is held: before its first event of the pair, with an empty program lockset, outside timebase callbacks, binsem internals and RESERVED windows [F:spec §4.13] |
| Role | Both evaluation and intervention: the guard runs immediately before the access | Evaluation only: readiness is judged there | Intervention only |

- **Correspondence** [I]. For a **schedule-dependent** readiness violation, AI's turning point matches the LP of the failing conjunct at the use.
  - Example (App. A.2): the child's `GetAppID` critical section reads the unpublished App record. Before it the outcome is open; after it the failure is fixed.
  - For a **value conjunct** the capture point plays this role. This mirrors AI's straw-man 2 (§1.2): the decisive access can come well before the visible failure. In R2, the stamp is captured at argument evaluation, and the failure only appears inside EVS.
- **Difference 1: evaluation vs intervention** [I]. AI places the stall at the evaluation point. Kairos's LPs are often inside framework critical sections: the OSAL name search under the type lock, SB `route_lookup` under the SB lock, ES `GetAppRecordByContext` under the ES lock [F:spec §1.4]. The latest point where delaying the thread still helps is the lock acquisition *before* the LP, and that is where Kairos's gate rule puts holds. Kairos keeps the two points apart; AI does not, and so would stall inside locks (§6.1).
- **Difference 2: fixed-order violations have no turning point** [I]. Kairos's LP still exists, and the class `fixed-order` records that no schedule avoids the violation. For R2, the "turning point" would sit at build or configuration time (the generated core order), which is Kairos's basis B.
- **Difference 3: shapes** [I]. Some of Kairos's shapes have direct AI analogues:
  - `ABSENT` (no enabling transition before the use) corresponds to `RPre = nil`;
  - `STALE` (the conjunct held earlier, then was disabled) corresponds to AI's atomicity case, where an unexpected remote write intervenes;
  - `WRONG-WAY` and `UNORDERED` have no AI counterpart, because AI does not reason about happens-before.
- [S] Do not adopt "turning point" as a Kairos term. Where gate placement is motivated, cite AI as the source of the observation that intervention must come before the decisive access, then explain why Kairos moves the intervention earlier, to a lock-free point.

### 6.3 (c) AI stalling vs `kz hold` / `kz gate`

| Aspect | AI stall | Kairos F0 `kz hold` / F1 `kz gate` [F:spec §4.13] |
|---|---|---|
| Purpose | **Tolerance**: prevent an unseen order in production | **Confirmation**: realize a predicted order (bad, or good) on the real binary, as evidence |
| Trigger | Always-on guard at every instrumented shared access; fires on `RPre ∉ BSet` | Offline plan per predicted witness; one gate per run |
| Placement | Immediately before the access, even inside a critical section [I] | Empty program lockset; not in timebase callbacks, binsem internals or RESERVED windows; a free debug register |
| Release | Polling re-check of RPre; resume when it is in the BSet | Event-based: the awaited event (with its outcome) appears in the live stream, or the other party blocks or exits |
| Cap | Fixed threshold (value not stated); log "unresolved violation" | Watchdog = the smallest *reference* timeout the hold can make expire (e.g., `CFE_PSP_RESTART_DELAY`, core startup 30 s, retry bound × delay); result `inconclusive(await-timeout)` |
| Timeout semantics | "a correctly synchronized program will not depend on time delays" [F:AI §4.5] | Timeouts are part of the semantics: σ_U vs σ_U∞, `timeout-dependent`, `timeout-masked-liveness`; a hold during which a program timeout fires is labelled `timeout-exercised` [F:spec §1.8, §1.10] |
| Cost | Inline guard per access (Table 3) | ptrace plus a debug-register breakpoint (#DB ≈ 34 µs measured), no text change; per-run perturbation ratio |
| Evidence | A violation log | `realized` + naturalness (`held-F0`, `gated-F1`, `timeout-exercised`), `derived` flag |

- **Why AI's harmlessness argument fails for cFS** [I]. cFS correctness depends on time delays by design:
  - soft startup barriers that time out and only log [F:spec §3.6];
  - the core sync that panics after 30 s;
  - CF's 25 × 100 ms retry bound.
  An AI-style stall can therefore turn a good run into a bad one. Kairos's `timeout-exercised` label and its σ_U/σ_U∞ refinements formalize the effect that AI's argument assumes away.
- [S] **Possible discussion point, not part of the spec.** Kairos verdicts can say *where* AI-style tolerance would be useless or unsafe:
  - useless for `fixed-order`, because the MB certificate gives a circular wait;
  - unsafe where the LP is under a lock;
  - timing-sensitive where the instance is `timeout-dependent`.
  This positions Kairos as complementary to tolerance work rather than a competitor.

### 6.4 (d) Should an AI-style learned-invariant comparator be a baseline?

**Recommendation [S]: yes.** Add it as a second variant in the "learned invariant" slot of §8.3, next to the DefUse-style row, sharing the same training pool.

**How it differs from the DefUse-style learner** (based on AI's own description of DUI, §4.2.2 p.337, and the DefUse abstract [V]):

| | DefUse-style (DUI-LR / Follower / DSet) | AI-style (BSet) |
|---|---|---|
| Relation learned | Def-use: which **writes** a read may read from (DSet); local vs remote definer (LR); consecutive same-thread reads seeing the same value (Follower) | Last **remote accessor** of the address, read or write, plus `nil` |
| Order violations | AI's analysis: DUI cannot handle W-W order or R-W \| R-W atomicity, and DSet predicts only a subset of W-R order violations | All 8 patterns (Table 2; the AI column is experimental) |
| Initial state | Depends on how the initial definition is modelled (not checked here) | Explicit `nil`, the natural encoding of Kairos's `ABSENT` shape |
| Check point | At the read; LR and Follower are checked at the end of a region | Before the access (anticipation) |

**Lifting it to Kairos traces** (a reimplementation, labelled as such per §8.3's rules) [S]:
- address → resource key `sel(e)`;
- static instruction → context-qualified template;
- thread → actor (executing task);
- access → every Φ event on the key (variant **AI-all**, faithful to AI's read/write-agnostic RPre), or only lifecycle effects (variant **AI-lc**, closer to DSet);
- `nil` → no other-actor event on the key before the use.

Then:
- **Training.** `BSet(t)` = the union over N natural runs of one pool key that pass a **frozen output oracle**. That oracle is the analogue of AI's "verifying the outputs", for example: reached OPERATIONAL, no ERROR-type event, normal exit. Record the oracle as an assumption before F1.
- **Alarm.** A use with `RPre ∉ BSet(t)`.
- **Golden tests.** AI Figs. 3, 4, 6, 7 and 10 give exact RPre/BSet values, which satisfies §8.3's rule that golden tests come from the original paper's worked examples.
- **Report alarms against N** (a training-size curve). This mirrors §4.5 of the AI paper and shows the learned approach's dependence on training directly.

**Why it adds something beyond DefUse** [I]:
1. AI claims to cover order violations, including W-W and use-before-any-access through `nil`, and to subsume AVIO, DUI, CCI and PSet in Table 2. It is therefore the *strongest* member of the learned-interleaving-invariant family to contrast with reference-specified contracts. (Its claim over the other four is analytical, not run.)
2. Lifting to API level gives AI the context it lacks (§4.2.5), because templates are context-qualified. If it still cannot flag R2, the reason is the *learning* premise, not the granularity. That is a clean ablation of "specified vs learned".
3. It is cheap: a per-key two-entry recorder and set unions.

**Structural property, not a prediction** [I]. R2's order occurs in every natural run, so any learner trained on natural runs contains it in its BSet, under both AI-all and AI-lc. This follows from the definition, so it is not a prediction of a measured outcome. Note that §8.3 says "Predictions of comparator outcomes are not written down in advance". For the other cases (C1 in B′, A.2/A.3 windows, SB losses) the outcome depends on the training orders, so **no expected values should be written into the evaluation record**.

**The original tool.** Record it as `NOT_RUN`. Blockers:
- an LLVM 3.3-era pass pipeline;
- source availability unverified (`james0zan.github.io/AI.html`, footnote 2);
- per-access instrumentation of all of cFS, including OSAL and cFE.
Optionally treat it as an external memory-level reference point after a PI decision, like helgrind and DRD. It is not a data-race detector, so the "thread data-race tools are not baselines" rule does not exclude it. It is still memory-level.

**PSet.** AI describes PSet as "similar to our belonging set in the format" but constrained to data dependencies, where at least one access is a write [F:AI §4.2.4]. Rather than a separate comparator, treat PSet as the AI-lc variant's hardware ancestor.

### 6.5 (e) How to cite it in related work

**Bibliographic entry (V):** M. Zhang, Y. Wu, S. Lu, S. Qi, J. Ren, W. Zheng. "AI: A Lightweight System for Tolerating Concurrency Bugs." In *Proc. 22nd ACM SIGSOFT International Symposium on Foundations of Software Engineering (FSE '14)*, Hong Kong, pp. 330–340, 2014. doi:10.1145/2635868.2635885. The extended version (V, bibliographic only): IEEE TSE 42(10):899–917, 2016, doi:10.1109/TSE.2016.2531666.

**Draft paragraph [S]:**

> *Learned interleaving invariants.* AVIO, DefUse, CCI, PSet and AI learn, from runs judged correct, which accesses may precede or interleave a given access, and they flag deviations. AI [Zhang et al. FSE'14] records for each static instruction the set of remote instructions seen to access the same address immediately before it, including an explicit "no predecessor" value (nil). It stalls a thread just before an access whose predecessor was never seen, the bug's "turning point", and tolerates atomicity and order violations without rollback. Kairos differs in where the expectation comes from. Its readiness contracts are taken from framework documentation, with provenance, and they state which not-ready outcomes are acceptable. This matters for fixed-order violations such as cFE #2663, where the wrong order occurs in every boot. Any invariant learned from such runs contains the defect, and since a MustBefore certificate shows the provider's publication depends on the use's actor progressing, no stall can repair it. Kairos's holds resemble AI's stalls, but they confirm predicted witnesses rather than tolerate failures. They are placed at lock-free points before the decisive access, and their timeouts are derived from the program's own reference timeouts.

Put the one-line positioning in the §1.11 comparison table, R2 row, next to DefUse (see §8 below).

---

## 7. What the AI paper implies for the word "contract"

1. **The literature AI belongs to uses "invariant" with two meanings** [F:AI §2]: a predicate "that should always be true at certain points of the execution", and a *likely* invariant learned from runs (Daikon [5], DefUse [28], CCI [7]; AVIO's "access interleaving invariants" and AI's own). **Kairos's object is neither.**
   - It is not learned, by policy (provenance, freezes, anti-circularity, §7.2).
   - It is not "always true at the use": a not-ready state at the LP is legitimate when the outcome is in `Allowed` (TOLERATED, §1.6).
   - Calling it an invariant would misstate both properties and invite the comparison that R2 refutes.
2. **"Contract" names the specified side of the specified/learned divide.**
   - Design by Contract: B. Meyer, "Applying 'Design by Contract'," IEEE Computer 25(10):40–51, 1992 [V bibliographic]. *I could not retrieve the text (fetch failed). The search snippet of its abstract reads "This article shows how to reduce bugs by building software components on the basis of carefully designed contracts." My paraphrase of DbC below is from general knowledge and must be checked before quoting.* Under DbC, a contract binds a client and a supplier through preconditions, postconditions and invariants.
   - Contracts for concurrency exist as a term:
     - Beugnard et al., "Making Components Contract Aware," IEEE Computer 32(7):38–45, 1999, doi:10.1109/2.774917 [V bibliographic]. It has a **synchronization** contract level. *The level names come from secondary sources, which disagree on the wording: P.*
     - Sousa, Dias, Ferreira, Lourenço, "Preventing Atomicity Violations with Contracts," arXiv:1505.02951, 2015 [V]: library developers write contract terms listing methods that must run in one atomic scope, and client programs are checked against them.
     - "Verifying Concurrent Programs Using Contracts," ICST 2017, pp. 196–206, doi:10.1109/ICST.2017.25 [V for title, venue, pages and DOI; **author order P**: the FIT record and the slides list the seven authors, Dias, Ferreira, Fiedor, Lourenço, Smrčka, Sousa and Vojnar, in different orders].
   - The learned counterpart of a specification is "specification mining" (Ammons, Bodík, Larus, POPL 2002, pp. 4–16 [V]).
3. **Caveats for the PI.**
   - (i) In classical DbC a violated precondition is a client bug, and the supplier owes nothing. Kairos's `Allowed` instead specifies acceptable behaviour when the requirement is *not* met, such as documented error returns, retries, deferrals and drops. So a Kairos contract is a **precondition plus documented not-ready behaviour plus the client's required handling**. That is closer to an "API contract" in the documentation sense, which is how the spec already uses the word for OSAL (M_api, §1.4), than to a pure DbC precondition.
   - (ii) "Contracts for concurrency" (Sousa et al.; ICST 2017) means atomicity specifications. Cite it so that reviewers do not assume Kairos checks atomicity contracts.
   - (iii) `harm`, `prov`, `authored`, `guard` and `scope` are metadata, not contract clauses. Present them as such.
4. **Options** [S], for the PI to decide:
   - **(A) Keep "readiness contract"** (recommended in light of this paper). Define it in one sentence: "a reference-specified requirement on the lifecycle state of a framework resource at a use, together with the documented not-ready behaviours that are acceptable". Contrast it explicitly with likely invariants and mined specifications.
   - **(B) "Readiness specification"** (준비성 명세). This pairs naturally with "specification mining" as the learned counterpart, but loses the client/supplier connotation that `Allowed` relies on.
   - In either case, **never call it an invariant**, and do not use "precondition" alone, because that drops `Allowed`.

---

## 8. Suggested edits (not applied; the spec and background are read-only for this task)

1. **§1.11, R2 row, "What prior work does".** Add: "AI (FSE 2014 [V]) learns per-instruction remote-predecessor sets from verified-correct runs. R2's order occurs in every run, so it is in every learned set. Stalling cannot repair it: MustBefore makes the wait circular."
2. **§1.11, "Established theory used".** Add a row "Learned interleaving invariants; anticipation and stalling": AVIO [V-abs], DefUse [V-abs], CCI OOPSLA 2010 [V], PSet ISCA 2009 [V bibliographic; mechanism P], AI FSE 2014 [V].
3. **§4.13.** Add one sentence citing AI as the tolerance-side precedent for thread stalling, and stating the two differences: lock-free gate placement and reference-derived watchdogs with `timeout-exercised`.
4. **§8.3.** Expand the DefUse row to "Learned interleaving invariants: DefUse-style DSet and AI-style BSet (AI-all, AI-lc)", with the shared training pool, the frozen output oracle, golden tests from AI Figs. 3/4/6/7/10, and a training-size curve. Add a `NOT_RUN` row for the original AI tool with its blockers (§6.4).
5. **Appendix B.** Add AI 2014 (V) and its TSE 2016 extension (V bibliographic), CCI 2010 (V), Daikon 2007 (V), ColorSafe 2010 (V), Aviso 2013 (V), LOOM 2010 (V), ConAir 2013 (V), Kivati 2010 (V bibliographic; mechanism P), Meyer 1992 (V bibliographic), Beugnard 1999 (V bibliographic, levels P), Sousa et al. 2015 (V), ICST 2017 contracts (V, author order P) and Ammons et al. 2002 (V).
6. **`background/04_lit_dynamic.md` PSet entry.** The bibliography is now V: dblp gives ISCA 2009, pp. 325–336, doi:10.1145/1555754.1555796. The mechanism stays P: it is known from course slides (R04) and from AI's §4.2.4 description.
7. **Glossary.** Add "Contract (readiness)" with the definition from §7 item 4(A), and a line "Not an invariant: see T1".

---

## 9. References and status

| Ref | Status | Basis |
|---|---|---|
| Zhang, Wu, Lu, Qi, Ren, Zheng. AI: A Lightweight System for Tolerating Concurrency Bugs. FSE 2014, pp. 330–340, doi:10.1145/2635868.2635885 | **V** | PDF read in full; MADSys publication page via search |
| Same authors. A Lightweight System for Detecting and Tolerating Concurrency Bugs. IEEE TSE 42(10):899–917, 2016, doi:10.1109/TSE.2016.2531666 | **V** (bibliographic); content not read | Search (MADSys page, preprint record) |
| Yu, Narayanasamy. A Case for an Interleaving Constrained Shared-Memory Multi-Processor. ISCA 2009, pp. 325–336, doi:10.1145/1555754.1555796 | **V** bibliographic; **P** mechanism | dblp via search; mechanism from AI §4.2.4 only |
| Shi, Park, Yin, Lu, Zhou, Chen, Zheng. Do I Use the Wrong Definition? DeFuse/DefUse: Definition-Use Invariants for Detecting Concurrency and Sequential Bugs. OOPSLA 2010, pp. 160–174 | **V** | Author page, rankdblp via search. Title spelling varies between "DeFuse" and "DefUse" across sources |
| Lu, Tucek, Qin, Zhou. AVIO: Detecting Atomicity Violations via Access Interleaving Invariants. ASPLOS 2006, pp. 37–48 | **V** | Crossref, PDF host via search |
| Jin, Thakur, Liblit, Lu. Instrumentation and Sampling Strategies for Cooperative Concurrency Bug Isolation (CCI). OOPSLA 2010, pp. 241–255, doi:10.1145/1869459.1869481 | **V** | Author pages via search |
| Ernst, Perkins, Guo, McCamant, Pacheco, Tschantz, Xiao. The Daikon system for dynamic detection of likely invariants. Sci. Comput. Program. 69(1–3):35–45, 2007 | **V** | Author pages via search |
| Lucia, Ceze. Finding Concurrency Bugs with Context-Aware Communication Graphs (Bugaboo). MICRO 2009 | **V** | UW PDF, MSR listing via search |
| Lucia, Ceze, Strauss. ColorSafe. ISCA 2010, pp. 222–233 | **V** | MSR listing, UW PDF via search |
| Lucia, Ceze. Cooperative Empirical Failure Avoidance for Multithreaded Programs (Aviso). ASPLOS 2013 | **V** | UW PDF, OpenAlex via search |
| Wu, Cui, Yang. Bypassing Races in Live Applications with Execution Filters (LOOM). OSDI 2010 | **V** | USENIX page via search |
| Zhang, de Kruijf, Li, Lu, Sankaralingam. ConAir. ASPLOS 2013, pp. 113–126, doi:10.1145/2451116.2451129 | **V** | UW wiki BibTeX via search |
| Chew, Lie. Kivati: Fast Detection and Prevention of Atomicity Violations. EuroSys 2010, pp. 307–320 | **V** bibliographic; **P** mechanism | The debug-register monitoring and "undo their effects" description comes from the MSc thesis abstract via search; the EuroSys text was not fetched. Cite only if the mechanism is checked, since it is relevant to `kz hold`'s debug registers |
| Lu, Park, Seo, Zhou. Learning from Mistakes. ASPLOS 2008 | **V** | Search; already V-abs in the spec |
| Meyer. Applying "Design by Contract". IEEE Computer 25(10):40–51, 1992 | **V** bibliographic; text not seen | Search; the fetch of the text failed |
| Beugnard, Jézéquel, Plouzeau, Watkins. Making Components Contract Aware. IEEE Computer 32(7):38–45, 1999, doi:10.1109/2.774917 | **V** bibliographic; **P** level names | Search; secondary sources differ on the level names |
| Sousa, Dias, Ferreira, Lourenço. Preventing Atomicity Violations with Contracts. arXiv:1505.02951, 2015 | **V** | arXiv listing via search (the fetch failed; abstract from the search summary) |
| Verifying Concurrent Programs Using Contracts. ICST 2017, pp. 196–206, doi:10.1109/ICST.2017.25 | **V** title/venue/pages/DOI; **P** author order | FIT BUT record and ICST slides via search |
| Ammons, Bodík, Larus. Mining Specifications. POPL 2002, pp. 4–16, doi:10.1145/503272.503275 | **V** | EPFL record, dblp via search |
| AI tool source, `http://james0zan.github.io/AI.html` | **UNVERIFIED** (availability) | Stated in the paper; not checked |

---

## 10. Open items

- Check Meyer 1992's exact wording before quoting any DbC definition (the text was unavailable here).
- Check Kivati's mechanism in the EuroSys text before citing it as debug-register prior art for `kz hold`.
- Read the TSE 2016 extension for changes to the stall threshold, the training procedure or the limitations (not done).
- Check whether DefUse models the initial (pre-any-write) definition, to make the DefUse-style row's handling of `ABSENT` precise (not checked).
- Freeze the comparator's output oracle (the definition of a "verified-correct boot") before F1.

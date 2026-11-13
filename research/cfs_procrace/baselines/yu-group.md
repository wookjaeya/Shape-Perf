# Yu-group process-level tools: SimRacer, SIMEXPLORER, RRF, SimEvo, ReDPro

Status as of 2026-10-08 (revised 02:15Z after a review: evidence grades for platform, observation-model and excerpt rows were corrected; see §4, §5, §7.2 and §8). This file covers availability, documented requirements and observation models, which were checked against [CONDITIONS_POLICY.md](../CONDITIONS_POLICY.md). **None of the five tools was run.** None of them may be reported as "fails to detect" any of C1, C2, C3, C5 or R2.

## 1. Bottom line

| Tool (paper) | Public implementation or artifact found? | Platform (evidence grade) | Verdict |
|---|---|---|---|
| SimRacer (ISSTA 2013, 10.1145/2483760.2483771) | None found on GitHub. Author page, Zenodo and figshare are blocked here, so I could not check them. | Simics Virtual Platform, **by inference only**. DESCRY p.2 co-cites SimRacer [38] next to Simics [8] when describing DESCRY's *own* replay platform. A search excerpt (source: one of three listed pages, §3.2) says "commercial virtual platform". SimRacer's own text is unread. | material_unavailable |
| SIMEXPLORER (STVR 2017, 10.1002/stvr.1634) | None found. Same blocked hosts. | Simics. Excerpt only (source: one of two listed pages, §3.2), not opened | material_unavailable |
| RRF (ISSRE 2016, 10.1109/ISSRE.2016.35) | None found. Same blocked hosts. | Unnamed static program analysis tools, dynamic kernel event reporting tools and yield points. Search-tool paraphrase only (no verbatim quote) | material_unavailable |
| SimEvo (ICSME 2017, 10.1109/ICSME.2017.29) | None found. Same blocked hosts. | Simics. Excerpt only (NSF PAR copy, not opened) | material_unavailable |
| ReDPro (ACMSE 2022, 10.1145/3476883.3520207) | None found. Same blocked hosts. | **Not filled:** roadmap [P8]/§4.6 (full text unread). An ACM DL excerpt mentions Intel Pin; it is kept only as a lead (§3.2, R11). | material_unavailable |

The verdict has two causes:

- **No implementation:** None of the five has an implementation, VM image or script that I could reach.
- **No primary papers:** None of the five papers could be opened from this container. The egress policy denies `CONNECT` to every publisher, author-page, repository and archive host I tried (log: `yu-group_source_fetch_20261008.log`).

The only primary text by this group that I could open is:

- **DESCRY (FSE 2017):** the author-hosted PDF, obtained from the GitHub repository behind `chaowang-vt.github.io`.
- **SCMiner preprint:** obtained from `github.com/tarannumzaman/Scminer`.

DESCRY describes SimRacer in §7.2 and cites RRF [29] only alongside SimRacer ("SimRacer [29, 38]", p.8). SCMiner mentions SimRacer only (p.1 via [18], p.11). Neither paper describes RRF, SIMEXPLORER, SimEvo or ReDPro, and neither is documentation for any of the five tools.

## 2. Decisions needed from you

These conclusions depend on choices I should not make alone.

**D1. Network access to primary sources (blocking).**
- **What is blocked:** the environment's network policy denies these hosts:
  - homepages.uc.edu
  - dl.acm.org
  - ieeexplore.ieee.org
  - onlinelibrary.wiley.com
  - par.nsf.gov
  - digitalcommons.unl.edu
  - uknowledge.uky.edu
  - scholars.uky.edu
  - archive.cps-vo.org
  - zenodo.org, figshare.com
  - doi.org, api.crossref.org
  - web.archive.org
  - www.intel.com, software.intel.com
  - acmse.net, ischool.syracuse.edu (roadmap [P8] sources for ReDPro)
  - archive.softwareheritage.org (§3.3)
  - www.windriver.com, blogs.windriver.com (historical Simics vendor)
- **How to unblock:** add the hosts under Allowed domains in the environment's Network access settings (cloud environment menu in the session title bar, then Edit), **with the "Allow package managers" box left ticked**. Steps: https://code.claude.com/docs/en/cloud-environments#network-access. Other options:
  - Place the PDFs in the repository.
  - Reconnect or authorise the Scholar Gateway connector. It currently returns `ACCESS_DENIED: Access denied` (logged in `yu-group_source_fetch_20261008.log`, fourth round).
- **What it unblocks:**
  - Reading each tool's own implementation section, including the Simics version, simulated machine, guest kernel and Pin version.
  - Checking the author page, Zenodo and figshare for artifacts.
- **Without it:** every requirement below that is marked "excerpt" stays unverified.

**D2. Asking the authors for code.**
- **Situation:** no public code was found.
- **Option:** the remaining route to the *original* tools is a request to the authors: Tingting Yu, now at the University of Cincinnati, and Tarannum S. Zaman (ReDPro, SCMiner).
- **Who decides:** whether to contact them, and as whom, is your call. I have not contacted anyone.

**D3. How to record these tools in roadmap §4.7.**
- **Proposal:** fill the SimRacer and SIMEXPLORER rows with `자료 부족 (material unavailable)` for all five cases, plus a pointer to this file. Do the same for RRF and SimEvo in their own role tables (roadmap §4.7 output paragraph: RRF in the reproduction table, SimEvo as regression evaluation).
- **ReDPro is left out of this proposal.** Roadmap §4.6 says ReDPro must not enter function tables or performance comparisons before its full text, method and support scope are confirmed. [P8] says its method and cFS applicability must not be filled before then. Putting it in a role table would conflict with §4.6. If you want it listed anyway, that is your decision.
- **Not allowed:** recording "미탐지 (not detected)" or counting a false negative.
- **Needs approval:** your OK on this labelling.

**D4. Whether to pursue a labelled "paper-method reimplementation" (§3.5, §4.2).**
- **What it would mean:** observing and controlling the same events without the original tools. For example, Simics could be replaced by native ptrace/gdb or QEMU hooks, or Pin by another instrumenter.
- **Policy status:** this would be a `deviation` under the conditions policy. It would have to be reported as a reimplementation, never as an original-tool result.
- **Recommendation:** decide only after D1, because the method sections, not the abstracts, define what such a reimplementation would have to reproduce.

## 3. Sources opened and not opened

### 3.1 Opened (primary text actually read)

| Source | How obtained | Integrity | What it says about the five tools |
|---|---|---|---|
| **S1.** T. Yu, T. S. Zaman, C. Wang, *DESCRY*, ESEC/FSE 2017 (`pubDOC/YuZW17.pdf`, 11 pp.) | `git clone --filter=blob:none` of `github.com/chaowang-vt/chaowang-vt.github.io` @ `f3544731050f7f435640f53c1c938c0dea5135dd`. This is the same file as roadmap [P5]. | sha256 `f9d7b57b…dfbf60` | See the quotes listed after this table. |
| **S2.** T. S. Zaman, X. Han, T. Yu, *SCMiner* preprint (IEEE PDFeXpress 2019-09-13, 12 pp.) | `git clone --depth 1 github.com/tarannumzaman/Scminer` @ `255933c62394e0d4fc0a9063975d36aca1cb15a9` | sha256 `854f86c1…172099` | p.11: "SimRacer [18] and RacePro [11] aim to detect process-level concurrency faults by testing for different interleavings of system calls." |

Quotes from S1 (DESCRY):

- **p.2, §1 (about DESCRY's own implementation):** "DESCRY has been implemented as a software tool using the LLVM compiler front-end [1], the KLEE symbolic virtual machine [5], and the Simics Virtual Platform [8, 38] for deterministic replay." [8] is Engblom et al., *Full-System Simulation* (Simics); [38] is SimRacer (`YuZW17_DESCRY_raw.txt:160-162, 1790, 1884`). This is a co-citation, not a statement about SimRacer's platform. The p.7 implementation sentence ("our interleaving schedule generator was implemented using the Simics Virtual Platform [8]") cites only [8].
- **p.3, §2.1 (DESCRY's own event model; it defers to [21, 38] for details):**
  - "Each event in the schedule is either a shared resource access … or a synchronization operation. Details of shared resource and event modeling can be found in prior work [21, 38]."
  - "Common process-level synchronization primitives include fork, wait, exit, pipe, and signal."
- **p.8, §7.2:**
  - RacePro and SimRacer [29, 38] "require the user to provide concrete data inputs … their search for erroneous interleaving schedules is not guided by logs".
  - "SimRacer does not have the capability of generating new data inputs, so we had to feed random inputs to SimRacer."
  - "DESCRY_AS relies on active testing techniques [31, 38]".
- **References:** [29] is RRF. DESCRY cites it only together with SimRacer ("SimRacer [29, 38]", p.8, `YuZW17_DESCRY_raw.txt:1188`) and does not describe RRF.

Quoted lines with line numbers are in `logs/yu-group_opened_sources_20261008.log`. Text extracts are in `/home/user/work/procrace/baselines/yu-group/text/`.

These are statements by the SimRacer authors, written in a later paper about a different tool (DESCRY). Only the §7.2 comparison facts are statements about SimRacer itself. None of this is documentation for any of the five tools.

### 3.2 Seen only as search-engine excerpts (pages NOT opened)

Each page in the following table is blocked here: WebFetch returns `EGRESS_BLOCKED` and curl returns `CONNECT tunnel failed, response 403`. The quoted snippets came back from the session's search tool. Their wording is unverified (log: `logs/yu-group_websearch_excerpts_20261008.log`).

| Page | Excerpt used |
|---|---|
| SimRacer, Q1 snippets. Source page not established: the search tool listed three pages for Q1, namely scholars.uky.edu (STVR record), archive.cps-vo.org/node/36387 and springerprofessional.de/doi/10.1145/2483760.2483771. One snippet is written in the third person ("They evaluated …"). The snippets are therefore not attributed to the ISSTA abstract. | "SIMRACER first computes potential races based on runtime traces obtained by running existing tests on target processes, and then it controls process scheduling relative to the potential races so that real races can be created." "We implemented SIMRACER on a commercial virtual platform that is widely used to support hardware/software co-design." "They evaluated its effectiveness on sixteen real-world applications …" |
| SIMEXPLORER. The first two snippets came from Q1 (one of the same three listed pages; the log labels them STVR abstract snippets). The "By using Simics" snippet came from Q5, whose source is one of onlinelibrary.wiley.com/doi/am-pdf/10.1002/stvr.1634 (author manuscript) and digitalcommons.unl.edu/computerscidiss/77 (dissertation). | "SIMEXPLORER first uses dynamic analysis techniques to observe system execution, identify program locations of interest, and report faults related to oracles. Next, it uses virtualization to achieve the fine-grained controllability …" "By using Simics, our framework also operates at the binary level …" 24 Unix programs. |
| T. Yu, PhD dissertation, UNL 2014, *SimExplorer: A Testing Framework to Detect Elusive Software Faults* (digitalcommons.unl.edu/computerscidiss/77) | **Search-tool paraphrase:** process-level races in both user and kernel modes, involving files, shared memory, hardware components, signals and processing cores. |
| RRF (snippet source archive.cps-vo.org/node/30361; ieeexplore 7774517 listed in Q5) | **Search-tool paraphrase (no verbatim quote returned):** combines existing static program analysis tools, dynamic kernel event reporting tools and yield points to give the observability and controllability needed to reproduce user-reported process-level races. No tracer or kernel is named. |
| NSF PAR copy of SimEvo (par.nsf.gov/servlets/purl/10089644) | "SimEvo employs the Simics Virtualization Platform [14] to observe system execution and to deterministically control occurrences of system-level events." "…causing the kernel scheduler to explore the affected interleavings." Impact analysis plus test reuse and generation. No system-call model is stated in the snippets. |
| ACM DL page of ReDPro | **Lead only.** Per roadmap [P8]/§4.6 it is not used to fill ReDPro's method or cFS applicability. "We used a binary instrumentation tool named PIN for run-time monitoring and controlling the order of execution for potential race pairs." Failures are "sensitive to the execution order of system calls". Nine real-world bugs. The snippets do not define "potential race pairs". |
| Generic Intel Simics documentation (Q5). This is not text from SimRacer, SIMEXPLORER or SimEvo. | **Context only:** "In order to run Intel Architecture targets quickly on IA-based hosts, the Simics simulator makes use of Intel VT-x. This requires that you install a Linux kernel module known as VMXMON on the host running the simulator." It presents VMXMON as a speed feature for IA targets. Whether any of the tools' simulated targets was IA is unknown. |

### 3.3 Not opened (blocked or not found)

- **SimRacer, SIMEXPLORER, RRF, SimEvo, ReDPro:** no full text was opened, whether publisher version, author manuscript or dissertation.
- **Tingting Yu's homepage:** `homepages.uc.edu/~tyu/`, including `links/research.html` (roadmap [P2]), is blocked. I could not check its tools or software listing.
- **Zenodo, figshare, Software Heritage, Wayback Machine:** all blocked, so they were not searched.
- **Roadmap §13.2 items not re-opened:** the roadmap records checks made on 2026-10-07 of the Wiley Summary [P3], the UKy abstracts [P4]/[P7] and the ACMSE schedule [P8]. I could not re-open any of them today.
- **`github.com/tarannumzaman/ReproSys`:** this is the SysPro code link from [P6-a]. `git ls-remote` asks for credentials, so it is not publicly readable now. This belongs to the SysPro task and is noted here only for completeness.

## 4. Observation model per tool (what each tool watches and controls)

| Tool | Execution unit | Events observed | Control | Oracle / input | Evidence |
|---|---|---|---|---|---|
| SimRacer | OS process (inferred) | **Inferred; SimRacer's own model is unread.** DESCRY §2.1 describes *DESCRY's* model: system calls as read/write effects on shared resources, plus fork/wait/exit/pipe/signal as synchronization. It refers to [21, 38] for details (`YuZW17_DESCRY_raw.txt:421-432`). SCMiner p.11 (secondary) says SimRacer tests "different interleavings of system calls". | Controls process scheduling relative to potential races (Q1 excerpt); virtual platform, Simics by inference (R2) | Concrete data inputs; no input generation (DESCRY §7.2) | **Opened secondary:** DESCRY §7.2 only (concrete inputs, no input generation). **Inferred:** event model (DESCRY §2.1, SCMiner p.11). **Excerpt:** control |
| SIMEXPLORER | OS process; works at binary level (excerpt) | Per the excerpt, SIMEXPLORER's own dynamic analysis observes system execution and identifies "program locations of interest". Whether a user can supply cFS sites is unknown; doing so would be an extension (roadmap §3.5). | Virtualization for fine-grained interleaving control; Simics (Q5 excerpt) | Reports faults related to oracles (oracle source unknown) | Excerpt only |
| RRF | OS process (paraphrase) | Kernel event reports (tool unnamed) plus static program analysis | Yield points | A race already reported by users | Search-tool paraphrase only. DESCRY cites [29] only alongside SimRacer and does not describe RRF. |
| SimEvo | OS process (excerpt: "process interleavings") | System-level concurrent events affected by a code change (impact analysis). No system-call model is stated in the excerpt. | Simics; kernel scheduler driven to explore affected interleavings | Old and new versions plus the existing test suite | Excerpt only |
| ReDPro | Not filled | Not filled | Not filled | Not filled | **Not filled:** roadmap [P8]/§4.6 (full text unread). The Pin excerpt is kept only as a lead (§3.2, R11). |

No tool's own observation model was read, so the scope is graded per tool:
- **SimRacer:** OS processes and system-call interleavings. This is *inferred* from secondary sources (DESCRY §2.1 co-citation, SCMiner p.11).
- **SIMEXPLORER:** process level, binary level (excerpt). **Counter-indication** to a syscall-only reading: the dissertation, as paraphrased by the search tool (§3.2; not a verbatim quote), describes testing races in both user and kernel modes, involving files, shared memory, hardware components, signals and processing cores. Accesses to shared memory after mapping are not system calls. Its scope cannot be confirmed without the paper.
- **RRF:** kernel event reports plus static analysis (paraphrase). No event model is stated.
- **SimEvo:** "system-level events" observed through Simics, with the kernel scheduler driven (excerpt). No system-call model is stated.
- **ReDPro:** not filled (roadmap [P8]/§4.6).

## 5. Documented requirements

Status values follow the conditions policy. "Excerpt" means the requirement comes from a search-engine snippet of an unopened page, so it is a lead, not a verified reference. The excerpt log's own rule is that excerpts are never the sole basis for a met/not-met row (`yu-group_websearch_excerpts_20261008.log:6`). Rows R2, R5, R9 and R11 therefore separate two things: the requirement, which is unverified, and the absence of the software here, which is verified. The verdict does not count these rows as documented blockers.

| # | Tool | Requirement | Reference | Grade | Met here? |
|---|---|---|---|---|---|
| R1 | all five | Tool implementation (source, binary, VM image or scripts) | Nothing found: GitHub repo/code/user search (`logs/yu-group_github_search_20261008.log`); author page, Zenodo, figshare blocked | n/a | **No.** Not available. This is the blocker. |
| R2 | SimRacer | Simics Virtual Platform (inferred) | DESCRY p.2 §1 co-cites SimRacer [38] next to Simics [8] when describing DESCRY's own replay platform (`YuZW17_DESCRY_raw.txt:160-162`). Q1 excerpt: "commercial virtual platform" (source is one of three listed pages). | Inference from a co-citation in an opened secondary paper (DESCRY p.2) + unverified excerpt | **Requirement: unverified (inference).** **Simics absent here: verified** (no `simics` in PATH, no `/opt/*simics*`; vendor hosts www.intel.com and software.intel.com are blocked). Not counted as a documented blocker. |
| R3 | SimRacer | Simics version, simulated machine, guest OS/kernel, Simics scripts | Not stated in any source I could open | unspecified_by_reference (as far as reachable) | **Unknown.** I must not pick a version without the paper. |
| R4 | SimRacer | Existing tests with concrete data inputs for the target processes | DESCRY p.8 §7.2; Q1 excerpt "running existing tests" (source is one of three listed pages) | Opened (secondary) + excerpt | Not evaluated, because the tool is unavailable. A cFS startup run would be the natural "test"; that mapping is my analysis. |
| R5 | SIMEXPLORER | Simics; binary-level operation | Q5 excerpt "By using Simics …" (source is one of: Wiley am-pdf of 10.1002/stvr.1634, or the UNL dissertation) | Excerpt | **Requirement: unverified (excerpt).** **Simics absent here: verified** (as R2). Not counted as a documented blocker. |
| R6 | SIMEXPLORER | Oracles and "program locations of interest". Per the excerpt, the locations are identified by SIMEXPLORER's dynamic analysis, and it reports faults related to oracles (oracle source unknown). Supplying cFS sites by hand would be an extension (roadmap §3.5). | Q1 excerpt (log label: STVR abstract; source is one of three listed pages) | Excerpt | Unknown |
| R7 | RRF | Static analysis tools, dynamic kernel event reporting tools, yield-point mechanism (none named) | RRF abstract (search-tool paraphrase, Q2) | Excerpt (paraphrase) | **Unknown.** I cannot choose a tracer (SystemTap, LTTng, ftrace…) without the paper. For context only: this kernel has `CONFIG_FTRACE=y`, `CONFIG_UPROBE_EVENTS=y`, `# CONFIG_KPROBES is not set` and `# CONFIG_MODULES is not set`. |
| R8 | RRF | A user-reported race (bug report) as input | RRF abstract (search-tool paraphrase, Q2) | Excerpt (paraphrase) | Bug reports exist for C1, C2, C3, C5 and R2 (roadmap §13.1). Not evaluated. |
| R9 | SimEvo | Simics Virtualization Platform | NSF PAR copy "SimEvo employs the Simics Virtualization Platform [14]" | Excerpt | **Requirement: unverified (excerpt).** **Simics absent here: verified** (as R2). Not counted as a documented blocker. |
| R10 | SimEvo | Two program versions plus existing tests | SimEvo abstract | Excerpt | Fix commits exist (e.g., CF 833fdbb); not evaluated |
| R11 | ReDPro | Intel Pin (version unknown). Lead only; ReDPro's method is otherwise not filled (roadmap [P8]/§4.6). | ACM DL abstract "binary instrumentation tool named PIN" | Excerpt | **Requirement: unverified (excerpt).** **Pin absent here: verified** (not on PATH, no `/opt/*pin*`; the Intel download hosts are blocked). Not counted as a documented blocker. ptrace works in this container (strace, gdb OK). I did not verify that ptrace is Pin's injection requirement. |
| R12 | none (context only) | VT-x plus the VMXMON host kernel module, for *fast* simulation of Intel Architecture targets | Generic Intel Simics documentation snippet (Q5, `yu-group_websearch_excerpts_20261008.log:40`). It is not text from SimRacer, SIMEXPLORER or SimEvo. | Context only | **Not a requirement stated by any of the five tools.** A generic Intel excerpt describes VMXMON as needed only for fast IA simulation. Whether any tool's simulated target was IA is unknown. Not evaluated. The container observations are listed below as plain facts. |

### Container facts (re-verified 2026-10-08, `logs/yu-group_container_facts_20261008.log`)

**System**
- Ubuntu 24.04.4 LTS userspace.
- Kernel `6.18.44-fc-v80` (x86_64, `hypervisor` CPU flag).
- 4 vCPU.
- Free disk on `/` (shared with other tasks): 3.9 GB at 01:07Z, **2.4 GB at 2026-10-08T02:11Z** (`df -h /home/user/work`, appended to the same log). This matters for any later Simics or Pin install.

**Capabilities and kernel configuration**
- `CapEff 000001fffeffffff`: CAP_SYS_MODULE, CAP_SYS_PTRACE, CAP_SYS_BOOT and CAP_SYS_ADMIN are all present.
- The capability is present, but `# CONFIG_MODULES is not set`, `/proc/modules` is absent, and `finit_module` returns ENOSYS. **No kernel module can be loaded.** This is a plain observation. No documentation of the five tools states a kernel-module requirement (see R12).
- `# CONFIG_KEXEC is not set` and `kexec_load` returns ENOSYS. **No other kernel can be booted.**
- No `/dev/kvm` and no `vmx`/`svm` flag. **No hardware virtualization.**

**Present and absent tools**
- ptrace works: `strace -f /bin/true` and `gdb -batch -ex run /bin/true` both OK.
- Simics, QEMU and Pin are not installed.

## 6. What I ran

No yu-group tool was installed or executed. The commands below were for source discovery and reading only. Nothing from a cloned repository was executed.

1. Re-checked container facts: `uname -a`, `/etc/os-release`, `nproc`, `df -h /home/user/work`, `free -m`, `grep CapEff /proc/self/status`, `zgrep CONFIG_… /proc/config.gz`, `ls /dev/kvm`, cpuinfo flags, `finit_module` (syscall 313) and `kexec_load` (syscall 246) probes via `python3 ctypes`, `strace -f /bin/true`, `gdb -q -batch -ex run /bin/true`, `which simics qemu-system-x86_64 pin`. Log: `logs/yu-group_container_facts_20261008.log`.
2. Ran reachability probes with `curl -sS -o /dev/null -L --max-time 25 -w '%{http_code}' <url>` for every candidate source host. Only GitHub hosts answered: `api.github.com` (200), `gist.github.com` (200), `raw.githubusercontent.com` (404 for every path tried) and the `github.com` web UI (400/403). git over HTTPS to github.com worked; every non-GitHub source host probed was refused (the Ubuntu apt mirror used in step 7 is reachable). Log: `logs/yu-group_source_fetch_20261008.log`. The fourth round in that log adds three items:
   - **Proxy status:** each refusal reads, in full, "gateway answered 403 to CONNECT (policy denial or upstream failure)". This text alone does not separate a policy denial from an upstream failure.
   - **WebFetch:** `https://onlinelibrary.wiley.com/doi/abs/10.1002/stvr.1634` returned `EGRESS_BLOCKED`, "Access to onlinelibrary.wiley.com is blocked by the network egress proxy." This is the definitive evidence that the cause is policy.
   - **Scholar Gateway:** I also tried the `semanticSearch` connector as another route to full text. It returned `ACCESS_DENIED: Access denied`.
3. Searched GitHub with repository, code and user search via the GitHub MCP search tools. Log: `logs/yu-group_github_search_20261008.log`.
4. Ran `git ls-remote` on `tarannumzaman/Scminer`, `tarannumzaman/ReproSys`, `vampirecat35/SIMExplorer`, `columbia/racepro`, `chaowang-vt/chaowang-vt.github.io` and `Tarannum-Zaman/Scminer`.
5. Ran `git clone --depth 1` of `tarannumzaman/Scminer` and `vampirecat35/SIMExplorer`.
   - `vampirecat35/SIMExplorer` turned out to be a GSM SIM-card explorer and was deleted.
   - The `Scminer` zip archives were *listed* with `python3 -I` `zipfile` (Java/R data-mining code and audit-log traces for SCMiner). They were not extracted or run.
6. Ran `git clone --depth 1 --filter=blob:none --no-checkout` of `chaowang-vt/chaowang-vt.github.io`, then `git checkout HEAD -- pubDOC/YuZW17.pdf`. Log: `logs/yu-group_github_candidates_20261008.log`.
7. Installed the reading aid with `apt-get install -y -q poppler-utils` (24.02.0-1ubuntu9.9). This is a PDF reader, not a tool condition. Log: `logs/yu-group_apt_poppler.log`.
   - **Side effect on shared state:** the install upgraded the shared library `libpoppler134` from 24.02.0-1ubuntu9.8 to 24.02.0-1ubuntu9.9 ("Unpacking libpoppler134:amd64 (24.02.0-1ubuntu9.9) over (24.02.0-1ubuntu9.8)"). This is the same kind of shared-container change that racepro.md §7 records for glibc, so later provenance checks should account for it.
8. Extracted text with `pdftotext -layout` and plain `pdftotext`, read metadata with `pdfinfo`, hashed with `sha256sum`, and mapped quotes to pages with per-page `pdftotext -f N -l N`. Log: `logs/yu-group_opened_sources_20261008.log`.
9. Ran WebSearch (8 queries). The excerpts are logged with their unverified status in `logs/yu-group_websearch_excerpts_20261008.log`.

Work directory: `/home/user/work/procrace/baselines/yu-group/`, 22 MB in total. It holds `sources/` (two git clones) and `text/` (pdftotext output).

## 7. Which cFS resources from the five cases could these models observe, in principle

This section is analysis, and every cell is NOT_RUN. cFS facts are cited at the bundle revision `nasa/cFS@088b2fa8` in `/home/user/work/procrace/cfs_ref/cFS`, with osal `d2d877a6`, cfe `c5fb2b4d` and CF `15a871e6`. Those are current revisions, not the historical revisions of each case, which task A pins.

### 7.1 Facts that decide the mapping

**The whole cFS instance is one OS process.**
- The native Linux build starts from a single `main` (`osal/src/bsp/generic-linux/src/bsp_start.c:196`).
- Apps are loaded into that process with `dlopen` (`osal/src/os/portable/os-impl-posix-dl-loader.c:100`).
- Every cFS task is a `pthread_create` thread (`osal/src/os/posix/src/os-impl-tasks.c:568`, reached from `CFE_ES_StartAppTask`, `cfe/modules/es/fsw/src/cfe_es_apps.c:649/664`). On this glibc, `pthread_create` issues `clone3`, not `clone` (strace: `logs/syspro_syscall_surface_probe_20261008.log:12`, `logs/descry_syscall_visibility_check_20261008.log:19`).
- A model whose execution unit is the OS process therefore sees one process for all five cases.
- `OS_TaskDelay` sleeps with `clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, …)`, not `nanosleep` (`osal/src/os/posix/src/os-impl-tasks.c:761`).

**The shared state of the cases is user-space memory, not kernel objects.**
- **C1:** `OS_CountSemGetIdByName` is a lookup in the OSAL name table (`osal/src/os/shared/src/osapi-countsem.c:212`, which calls `OS_ObjectIdFindByName`, `osapi-idmap.c:975`). The semaphore itself is an unnamed, process-private `sem_init(&impl->id, 0, …)` (`osal/src/os/posix/src/os-impl-countsem.c:85`). No system call carries the semaphore name. CF's retry loop is at `apps/cf/fsw/src/cf_cfdp.c:1282`.
- **C5:** `CFE_ES_GetAppID` (`cfe/modules/es/fsw/src/cfe_es_api.c:738`) reads the ES app/task table in memory.
- **R2 and C3:** the SB AppId is a field that SB's own task writes during init: `CFE_ES_GetAppID(&CFE_SB_Global.AppId)` (`cfe/modules/sb/fsw/src/cfe_sb_task.c:138`).
- **Startup sync:** `CFE_ES_WaitForSystemState` (`cfe_es_api.c:514`) polls memory state with `OS_TaskDelay` (`cfe_es_api.c:603`).

### 7.2 Per case

| Case | Kernel-visible events (what a syscall/kernel-event model can see) | State that decides the order violation | SimRacer / SimEvo (syscall model *inferred* from secondary sources; own models unread; SimEvo's excerpt states no syscall model) | SIMEXPLORER (locations of interest identified by its own dynamic analysis, per excerpt; binary level) | RRF (kernel events + yield points; paraphrase only) | ReDPro |
|---|---|---|---|---|---|---|
| C1 CF#184 | `clone3` for BP/CF tasks; `clock_nanosleep` in the retry delay (`OS_TaskDelay`); `futex` only under contention | OSAL name-table entry for the throttle semaphore. The original BP code is not public, so the creator side cannot be reproduced as-is. | Not a shared-resource event under the syscall model inferred from secondary sources; own model unread; NOT_RUN. Name registration and lookup make no system call, and both sides are threads of one process. | Unknown; NOT_RUN. Whether its dynamic analysis would identify the `OS_CountSemCreate`/`OS_CountSemGetIdByName` sites is unknown. Whether a user can supply them is also unknown, and doing so would be an extension (roadmap §3.5). Whether its controller can order threads of one process is unknown. | Yield points could in principle be placed at the two sites given the bug report. Kernel events would not show the name table. NOT_RUN. | Not filled: roadmap [P8]/§4.6 (full text unread) |
| C2 cFE#198 | Unknown: the provider app (EVA CWS) and its late-init are not public | Provider late-init completion | No target code to run | No target code to run | No target code to run | Not filled |
| C3 cFE#73 | `clone3` for TIME/SB/EVS tasks | SB AppId and EVS AppId/filter data (memory); original platform was MicroBlaze | Not a shared-resource event under the syscall model inferred from secondary sources; own model unread; NOT_RUN | Unknown, as for C1; NOT_RUN. The original MicroBlaze setup is a separate issue. | Possibly, via yield points; NOT_RUN | Not filled |
| C5 cFE#72 | `clone3` of the new main task (inside `OS_TaskCreate`) | ES app/task registration entry and TaskID–AppID link (memory, under ES lock) | Under the syscall model inferred from secondary sources, only the task-creation half (`clone3`) would be visible; the registration it races with makes no system call. Own model unread; NOT_RUN. | Unknown, as for C1 (relevant sites: `OS_TaskCreate` return, registration write, `CFE_ES_GetAppID`); NOT_RUN | Possibly, via a yield point between creation and registration; NOT_RUN | Not filled |
| R2 cFE#2663 | `clone3` for ES/EVS/SB tasks | `CFE_SB_Global.AppId` and the AppId that EVS validates (memory) | Not a shared-resource event under the syscall model inferred from secondary sources; own model unread; NOT_RUN | Unknown, as for C1; NOT_RUN | Possibly, if R2 is a scheduling race and not a fixed init-order dependency (roadmap §7.3); NOT_RUN | Not filled |

RRF's candidate cases are C1, C3, C5 and R2, as in roadmap §4.3. C2 is excluded because its provider code is not public.

**Summary.**
- **cFS side (fact, from source and strace):** in all five cases the kernel-visible part is at most thread creation (`clone3`), sleeps (`clock_nanosleep`) and `futex` under contention. The state that defines each order violation lives in cFE/OSAL memory inside one process.
- **Tool side (graded, see §4):** only SimRacer's system-call view has any support, and that support is *inferred* from secondary sources (DESCRY §2.1 co-citation, SCMiner p.11). RRF (kernel events, paraphrase) and SimEvo ("system-level events", excerpt) state no event model in what I could reach. ReDPro is not filled (roadmap [P8]/§4.6).
- **Counter-indication:** the SimExplorer dissertation, as paraphrased by the search tool (not a verbatim quote), describes testing in both user and kernel modes, involving files, shared memory, hardware components, signals and processing cores. Shared-memory accesses after mapping are not system calls, so a syscall-only reading of this tool family may be too narrow.
- **SIMEXPLORER:** per the excerpt, its own dynamic analysis identifies "program locations of interest" at binary level. Whether that reaches cFE/OSAL memory state is unknown. Its actual interface could not be read.
- **Status of these statements:** they are hypotheses for the §4.7 "observation" and "resource representation" stages. They are neither results nor predicted non-detections, because no tool was run and no tool's own model was read.

## 8. Verdict and evidence

**Verdict: `material_unavailable` for all five tools.**

1. **No implementation reachable.**
   - GitHub search found nothing for SimRacer, SIMEXPLORER, RRF, SimEvo or ReDPro (`logs/yu-group_github_search_20261008.log`).
   - The only public repository from this group is `tarannumzaman/Scminer` (SCMiner, a different tool).
   - The author page, Zenodo and figshare could not be checked (policy 403). The result is "not found in reachable sources", not "does not exist".
2. **Tool documentation not readable.**
   - No paper of the five could be opened.
   - No platform statement is opened. For SimRacer, DESCRY co-cites SimRacer [38] next to Simics [8] when describing DESCRY's own replay platform (p.2). That is not a statement about SimRacer's platform.
   - Everything else rests on unverified search excerpts.
3. **Even with code, the excerpt-indicated platform is not present here.** This is context; it is not counted as a documented blocker.
   - Simics is not installed and its vendor hosts are blocked. It is excerpt-indicated for SIMEXPLORER and SimEvo and inferred for SimRacer; the version is unknown.
   - Intel Pin is not installed and cannot be downloaded here. It is an excerpt lead for ReDPro; the version is unknown.
   - These requirements (R2, R5, R9, R11) are unverified, so they are not documented blockers. The verdict rests on items 1 and 2: R1 plus the unreadable papers.
4. **Consequence:** per the conditions policy and roadmap §3.5/§4.7, these tools must appear as "자료 부족 / NOT_RUN", never as "미탐지".

## 9. Prior partial attempt (2026-10-07) — did its choices follow documentation?

| Item | What it did | Assessment |
|---|---|---|
| `logs/yu-group_container_facts.log` (07:27) | Observed the kernel (`fc-v77`), capabilities, kernel config, `/dev/kvm`, kexec, ptrace and presence of Simics | Observations only; it chose no tool settings. Its facts match today's re-check, except the kernel build tag (`fc-v77`, now `fc-v80`: a different VM boot). Superseded by `yu-group_container_facts_20261008.log`. |
| `logs/yu-group_source_fetch.log` (07:27) | Fetched `homepages.uc.edu/~tyu/` and `links/research.html` (roadmap [P2]) plus `links/software.html` and `links/tools.html`; all returned 403 | Source-discovery probes. They choose no environment, variable or reproduction condition, so they fall outside the conditions policy, which governs "연구 환경, 변수, 실험 재현 조건" (`CONDITIONS_POLICY.md:3`). `software.html` and `tools.html` were guessed paths, and nothing was retrieved. The same standard applies to today's probes of unreferenced hosts and paths: sites.google.com, www.cs.uky.edu, cse.unl.edu, tarannumzaman.github.io and the Zenodo/figshare search URLs (`yu-group_source_fetch_20261008.log`). |
| `/home/user/work/procrace/baselines/yu-group/sources/` | Empty directory | Nothing to reuse. It now holds today's clones. |
| `container_kernel_caps.log`, `racepro_*`, `apt_gcc_multilib.log`, `baselines/racepro`, `baselines/syspro` | RacePro and SysPro attempts | Not yu-group material and not reused here. For the per-item judgment, see racepro.md §7:<br>- Judged **not documentation-conformant:** `racepro_libscribe_build.log`, `racepro_libscribe_build_m32.log` (undocumented `-m32`, a `deviation`), `apt_gcc_multilib.log` (not in any RacePro document; upgraded glibc 2.39-0ubuntu8.7 → 8.9) and `racepro_scribe_runtime.log`.<br>- `container_kernel_caps.log`: factual.<br>- `racepro_emulation_feasibility.log`: exploratory ("Ubuntu 10.10" is not in any reference).<br>- `baselines/syspro`: an empty directory (racepro.md §7, syspro.md §7).<br>The overlapping container facts (`CONFIG_MODULES` unset, `finit_module` ENOSYS, CAP_SYS_MODULE present in CapEff) were re-verified and agree. |

## 10. Files

- This report: `/home/user/Shape-Perf/research/cfs_procrace/baselines/yu-group.md`
- Logs, all in `/home/user/Shape-Perf/research/cfs_procrace/baselines/logs/`:
  - `yu-group_container_facts_20261008.log`
  - `yu-group_source_fetch_20261008.log`
  - `yu-group_github_search_20261008.log`
  - `yu-group_github_candidates_20261008.log`
  - `yu-group_opened_sources_20261008.log`
  - `yu-group_websearch_excerpts_20261008.log`
  - `yu-group_apt_poppler.log`
- Work directory: `/home/user/work/procrace/baselines/yu-group/`
  - `sources/tarannumzaman_Scminer`
  - `sources/chaowang-vt.github.io`
  - `text/*.txt`

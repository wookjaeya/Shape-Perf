# Process-race baselines: availability, requirements and observation models (RQ1, task B)

- Date: 2026-10-08 (written 02:30–02:45 UTC).
- Scope: the first research question. Do existing process-level race tools detect C1 (CF#184), C2 (cFE#198), C3 (cFE#73), C5 (cFE#72) and R2 (cFE#2663), and if not, what is missing (roadmap §3, §4, §4.7, §13.2).
- Policy: [`../CONDITIONS_POLICY.md`](../CONDITIONS_POLICY.md).
- Sources: this file is a synthesis of five reports. It adds no new fact about any tool.
  - [`racepro.md`](racepro.md)
  - [`yu-group.md`](yu-group.md), which covers SimRacer, SIMEXPLORER, RRF, SimEvo and ReDPro
  - [`descry.md`](descry.md)
  - [`syspro.md`](syspro.md)
  - [`osal-visibility.md`](osal-visibility.md)
- New in this pass: one read-only container re-check, [`logs/availability_container_recheck_20261008.log`](logs/availability_container_recheck_20261008.log). It agrees with the reports.

## Status at a glance

- **Eight tools were examined and none was run on anything.** Every cell for C1, C2, C3, C5 and R2 is `NOT_RUN`. No tool may be reported as "fails to detect" or "fails to reproduce" any case, and no cell may be counted as a false negative (policy; roadmap §3.5, §4.7).
- **Only RacePro has public code.** It cannot run here. Its Scribe recorder is a built-in patch to an i386 Linux 2.6.35 kernel. This machine cannot boot another kernel: it has no kexec, no module loading and no KVM.
- **The other seven tools have no reachable implementation.** These are SimRacer, SIMEXPLORER, RRF, SimEvo, ReDPro, DESCRY and SysPro. For six of them the paper could not be opened either. DESCRY is the exception: its paper was opened and read in full.
- **What can go into the paper now:**
  1. The availability record (this file).
  2. An observation-model analysis, labelled "analysis, not run", with each tool's evidence grade.
  3. The OSAL visibility probe, labelled as a probe and not as a tool result.
- **The probe's result:** on OSAL's own test, creating a named count semaphore and looking it up by name issued zero syscalls. The name never appeared in any syscall that OSAL made (§4).
- **Thirteen decisions are open for you** (§0). The defaults keep everything `NOT_RUN` with the blockers on record.

---

## 0. Decisions needed from you (consolidated)

Each row merges the matching decisions from the per-tool reports. The default applies if you do not decide.

| # | Decision | Options | Default | Source decisions |
|---|---|---|---|---|
| A1 | **Access to the primary papers.** None of the following could be opened here: RacePro, SimRacer, SIMEXPLORER, RRF, SimEvo, ReDPro, SysPro (arXiv, SSRN, JSS), the DESCRY project page and the Zaman dissertation. The cause is this session's egress policy. | (a) Put the PDFs in `research/cfs_procrace/paper/`, which is empty now. (b) Add the hosts to the egress allowlist: `www.cs.columbia.edu` / `www.sigops.org` (RacePro); `arxiv.org` (SysPro); `cs.uky.edu` and `uknowledge.uky.edu` (DESCRY page, dissertation); `dl.acm.org`, `ieeexplore.ieee.org`, `onlinelibrary.wiley.com`, `par.nsf.gov`, `homepages.uc.edu` and the other hosts in yu-group.md D1 (Yu-group papers and author page). (c) Do without. | (c). RacePro's model stays code-derived. SysPro's model stays secondary-source. The Yu-group models stay excerpt-only or inferred. | racepro D1, yu-group D1, descry D1(b), syspro D2 |
| A2 | **Asking the authors for code.** The authors are T. Yu, T. S. Zaman and C. Wang. This covers SimRacer, SIMEXPLORER, RRF, SimEvo, ReDPro, DESCRY with its Simics setup, and the SysPro `ReproSys` repository. | Contact them (and decide as whom), or not. Nobody has been contacted. | Not contacted. The tools stay `material_unavailable`. | yu-group D2, descry D1(c), syspro D1(a) |
| A3 | **Whether to run RacePro at all.** | (a) Accept `NOT_RUN` (environment blocker). (b) Provide a separate bare-metal or KVM-capable machine. The userland distribution is not named by any reference, so it would be an `unspecified_by_reference` assumption that you approve. (c) Emulate i386 with QEMU TCG here. That is a `deviation` (emulated CPU), and it needs more disk than the 500 MB budget. Free disk is 2.2 GB at 02:30Z. | (a) | racepro D2 |
| A4 | **How cFS would run under RacePro.** This applies only if A3 is (b) or (c). The reference `core-cpu1` is an x86-64 binary that needs kernel ≥ 3.2.0. cFE needs CMake ≥ 3.10. cFS would have to be rebuilt in an old 32-bit userland, and no reference gives the conditions for that. | Approve a separate condition record, or not. | Not attempted | racepro D3 |
| A5 | **A leftover file with credential-named environment variables**, `/home/user/work/procrace/baselines/racepro/build-libscribe32/log`. It holds the environment of the earlier `record` run. The values were not inspected. | (a) Delete it. (b) Keep it in place. | Kept in place. Not copied. | racepro D4 |
| A6 | **The issue comment threads** of CF#184 and cFE#198, #73, #72 and #2663. The policy counts comments as part of the original report (`CONDITIONS_POLICY.md:7`). `issue_read` was denied. | Supply the threads, or give this session `issue_read` on `nasa/CF` and `nasa/cFE`. | Inputs are recorded as "issue body only; N comments not read" (C1 5, C2 11 per descry.md, C3 14, C5 9, R2 2). | descry D3; syspro §1, S8 |
| A7 | **The labels for roadmap §4.7.** | Approve the cell texts in §5.3. In particular: material-unavailable tools get `자료 부족 / NOT_RUN`, never `미탐지`. ReDPro is **left out** of every role table until its full text is read (roadmap §4.6). | Proposed labels are used, pending your OK. | yu-group D3, descry §8, syspro §8, racepro §8 |
| A8 | **Whether the OSAL probe counts as "event observation" evidence** for the syscall-level models in the §4.7 diagnosis table. | Accept it with the label "not observable at syscall level", which never means a tool missed a case. Or require each tool's own run first. | Not decided | osal-visibility D2 |
| A9 | **Whether to build labelled method reimplementations** (roadmap §3.5). | Per tool, see §5.2. Only DESCRY's method is documented in a source that was opened. | No reimplementation | yu-group D4, descry D2, syspro D3 |
| A10 | **Whether to run the reference cFS once under `strace -f`.** This would confirm, at cFS level, the PSP `prctl(PR_SET_NAME)`, the zero syscalls of CF's lookup and the ES record writes, and the SB pipe `mq_open` calls. | (i) Run it after ENV E7 (reset type) is decided. (ii) Do not run it; keep the statements source-level. | (ii) until E7 | osal-visibility D1 |
| A11 | **Hypothesis H-OSAL-1** (§4.4). A by-name lookup that hits an object still being created may leave the type lock held, so the creator blocks for good. This bears on C1. | (i) gdb breakpoints on OSAL's own `count-sem-test`. (ii) A minimal test against the documented API. (iii) Defer. | Not executed. It must not be cited as a result. | osal-visibility D3 |
| A12 | **The OSAL revision for the probe.** It was run on `d2d877a` (cFS `main`). The C1 case's OSAL is `42af0f73`. The cFS `dev` checkout uses `dad0ee9`. | Keep `main`, or re-run on `dev` or on `42af0f73`. Tied to ENV A2. | `main` | osal-visibility D4 |
| A13 | **The mqueue sysctls from OSAL's CI** (`msg_max`/`queues_max` 512/512, against 10/256 here). | Decide together with ENV E5. | Not applied | osal-visibility D5 |

**Note for the ENV owner (an action, not a decision).** The earlier `apt-get install gcc-multilib` upgraded glibc from `2.39-0ubuntu8.7` to `8.9` on the shared container. That happened on 2026-10-07 at 07:21:50, before the reference `core-cpu1` was built. The `env/` condition records do not state the glibc version (racepro.md §7). Every OSAL and probe run in these reports used `8.9`.

---

## 1. Summary table

Evidence grades:
- **paper**: the tool's own paper was opened.
- **code**: derived from the tool's public source.
- **secondary**: the roadmap author's earlier reading, or a search-engine summary of an unopened page.
- **excerpt**: a search snippet of an unopened page.
- **inferred**: drawn from a co-citation in another paper.

The column "observable in principle" is analysis. Nothing in it was run.

| Tool (role, roadmap §3.2) | Public implementation | Documented requirements → met here? | Observation model (grade) | Five-case resources observable in principle (analysis, not run) | Verdict | Evidence |
|---|---|---|---|---|---|---|
| **RacePro**, SOSP 2011 (main detection baseline) | **Yes.** `columbia/racepro@681f94e`, `linux-2.6-racepro@b29e9af`, `libscribe-racepro@011b644`, `py-scribe-racepro@56d6187`. The racepro repo has no README or INSTALL; the procedure comes from the kernel README (KR) and the bundle's `deploy.rb` (DR). Choosing the columbia forks is code-derived: KR names the `nviennot/*` repos, which lack `SCRIBE_REAPED`. | **i386 Linux 2.6.35 with Scribe built in** (KC L3, L6-8, L1609; `scribe/Kconfig` L5-6): **No**. **Install and boot it** (DR L24-25): **No**, because there is no kexec (unset, ENOSYS), no module loading (unset, ENOSYS), no `/dev/kvm`, no vmx/svm, and `/boot` is empty. **Python 2, Cython ≥ 0.13, `python-dev`, `cython`** (KR L39-44; DR): **No**, Ubuntu 24.04 has no candidates. **networkx 1.x APIs, pygraphviz** (`setup.py` L13): **No**. The paper is unopened. | **code.** Scribe records each task's syscalls. It records resource-lock events on INODE, FILE, FILES_STRUCT, PID, FUTEX, IPC (SysV), MMAP, PPID and SUNADDR. It records memory page ownership between threads and processes. RacePro forms candidates in four classes: RESOURCE (in-syscall accesses, FUTEX excluded), EXIT-WAIT, SIGNAL and TOCTOU. Its happens-before sources are program order, fork/clone, wait, pipe/socket and signal. Memory events never become candidates. Validation replays to a cutoff, goes live, and runs the user's `.test` oracle. | For all five cases, **no candidate whose racing pair is the case dependency**. `clone` gives only a happens-before edge. Whether `mq_open` causes FILES_STRUCT events is unchecked. Incidental FILE, INODE, FILES_STRUCT and MMAP candidates are possible. Caveat: after go-live the failure could occur, and the oracle could print `BUG REPRODUCED` under an unrelated candidate. | `not_runnable_here` | racepro.md §2, §3, §5, §6, §8; `logs/racepro_container_facts_20261008.log`, `racepro_code_inspection_20261008.log`, `racepro_review_recheck_20261008.log` |
| **SimRacer**, ISSTA 2013 (detection) | **None found** (GitHub search). The author page, Zenodo and figshare are blocked. | Implementation (R1): **No**. This is the blocker. Simics is inferred from a DESCRY co-citation plus an excerpt; it is absent here but is not counted as a documented blocker. Version and target are unspecified. | **inferred.** OS processes and syscall interleavings (DESCRY §2.1; SCMiner p.11). It controls process scheduling (excerpt). DESCRY §7.2 says it needs concrete inputs. | Under the inferred syscall model, the dependency is not a shared-resource event (C1, C3, R2). For C5 only the `clone3` half is visible. C2 has no public target code. | `material_unavailable` | yu-group.md §1, §4, §5, §7, §8; `logs/yu-group_github_search_20261008.log`, `yu-group_source_fetch_20261008.log` |
| **SIMEXPLORER**, STVR 2017 (detection) | **None found.** | Implementation: **No**. Simics at binary level (excerpt): absent, not verified as a requirement. Oracles and "program locations of interest" (excerpt): unknown. | **excerpt.** Dynamic analysis finds the locations of interest, and virtualization controls the interleaving. A paraphrase of the dissertation mentions user and kernel modes, including shared memory, which argues against a syscall-only reading. | Unknown for C1, C3, C5 and R2. It is not known whether its analysis would find the cFS sites. Supplying the sites by hand would be an extension. C2 has no target code. | `material_unavailable` | yu-group.md (same sections) |
| **RRF**, ISSRE 2016 (reproduction) | **None found.** | Implementation: **No**. Unnamed static analysis, kernel-event reporting and yield points (paraphrase): unknown. Input is a user-reported race; bug reports exist. | **excerpt (paraphrase only).** Kernel event reports plus static analysis plus yield points. | Possibly, through yield points placed at sites taken from the report (C1, C3, C5, R2). Kernel events would not show the OSAL name table. C2 has no source. | `material_unavailable` | yu-group.md |
| **SimEvo**, ICSME 2017 (regression) | **None found.** | Implementation: **No**. Simics (excerpt): absent, not verified as a requirement. Two versions plus tests: fix commits exist, not evaluated. | **excerpt.** "System-level events" through Simics, with impact analysis. No syscall model is stated. | As for SimRacer (yu-group.md §7.2 treats them together). | `material_unavailable` | yu-group.md |
| **ReDPro**, ACMSE 2022 | **None found.** | **Not filled** (roadmap §4.6, [P8]). Intel Pin is an excerpt lead only; it is absent here. | **Not filled.** | **Not filled.** | `material_unavailable`. Not entered in the §4.7 tables. | yu-group.md §1, §4, §7.2 |
| **DESCRY**, ESEC/FSE 2017 (reproduction, cases with a log) | **None.** The paper gives no artifact location. No GitHub repo was found, and none is in the authors' accounts. The SE-artifact census lists no URL. The project page `cs.uky.edu/~tyu/research/descry` is blocked by this session's allowlist, so its contents are unknown. | Implementation (Q1): **No**. "KLEE built from LLVM 3.4" (p8): **No**, only LLVM 18 is present and the KLEE tag is unspecified. KLEE's own Ubuntu 14.04 image route is untested (Docker daemon not running). Simics (p7): **No**; version and target unspecified. auditd (Q6): possible in principle. Failed-run default log (Q9): absent from all issue bodies; comments unread. | **paper.** Scope is inter-process, not intra-process (thread-level), bugs (p1). Units are processes, single-threaded "for ease of presentation" (p3), plus signals. Events are syscalls on system-wide resources. Must-happen-before relations are program order, fork, wait, pipe and signal. It flips "suspicious event pairs" (p7). The oracle is a printed failure message. | **None of the five.** The publish and use sites make no syscall (C1, C3, C5, R2). Threads are not PuDs. R2 and C3 print no failure message. C2's app source is not public. | `material_unavailable` | descry.md §1, §3, §5, §6, §8; `logs/descry_paper_requirements_20261008.log`, `descry_report_verification_20261008.log` |
| **SysPro**, arXiv 2601.09616v1 / JSS 2026 (reproduction from bug reports) | **Not readable.** `tarannumzaman/ReproSys` asks for credentials, raw files return 404, and `add_repo` reports not found or no access. The JSS data-availability statement is unread. | No requirement is `from-reference`, because the paper and the artifact are unread. Intel PIN (version unknown): **No**, not installed and the hosts are blocked. srcML, NLTK, Gensim, SPMF: **No**, and no versions are documented. The system-call description source is unknown. | **secondary.** The report gives relevant syscalls, then call sites (srcML). It inserts a sleep around those syscalls under PIN. It observes at most syscalls and failure output. No memory or happens-before model is indicated. | **The dependency itself is not observed** in C1, C3, C5 or R2: the publication is a memory store. A delay is possible only at a hand-picked nearby syscall: provider init for C1; the SB thread's `prctl`, `sched_setaffinity`, `clock_nanosleep` and `rt_sigaction` for C3 and R2. For C5, at the reference OSAL, a delay after `clone3` blocks or aborts the child. C2 is not applicable. Stage-1 output is unknown. | `material_unavailable` | syspro.md §1, §3, §5, §6, §8; `logs/syspro_repo_probe_20261008.log`, `syspro_paper_fetch_20261008.log`, `syspro_review_recheck_20261008.log` |

### 1.1 Container facts the verdicts rest on

These were re-checked read-only at 2026-10-08T02:30Z in `logs/availability_container_recheck_20261008.log`. They agree with every per-tool facts log.

**System**
- Ubuntu 24.04.4 LTS.
- Kernel `6.18.44-fc-v80` on x86_64. The hostname is `vm`. The "fc" suffix suggests Firecracker; that is an inference.
- 4 vCPU.
- **2.2 GB free disk**, not about 4 GB. The disk is shared with other tasks, and the per-tool logs show it shrinking from 3.9 GB at 01:07Z (yu-group) to 2.4 GB at 02:10–02:24Z (racepro, yu-group, descry, syspro).

**Kernel and capabilities**
- CapEff is `000001fffeffffff`, which includes CAP_SYS_MODULE, CAP_SYS_BOOT and CAP_SYS_PTRACE.
- Modules cannot be loaded: `# CONFIG_MODULES is not set`, there is no `/proc/modules`, and `finit_module` returns ENOSYS.
- kexec is unavailable: `# CONFIG_KEXEC is not set`, `# CONFIG_KEXEC_FILE is not set`, and `kexec_load` returns ENOSYS.
- There is no `/dev/kvm`, and the CPU shows 0 vmx/svm flags.
- **Booting another kernel is impossible.**

**Tools**
- ptrace works: `strace -f` and `gdb` both run.

**Workspaces**
- All are under the 500 MB budget: racepro 201 MB, osal-visibility 69 MB, yu-group 22 MB, descry 2.6 MB, syspro 36 KB.

---

## 2. Per-tool notes

### 2.1 RacePro: `not_runnable_here` ([racepro.md](racepro.md))

- **Code.** The code is public and pinned (§1 table). This is the only tool with an implementation.
- **The blocker is the environment, not the material.** Scribe exists only as a `bool` (built-in) patch to an i386 2.6.35 kernel:
  - The syscall hooks are only in `entry_32.S` and `syscall_table_32.S`.
  - Install is `make install` plus `mkinitramfs` into `/boot`.
  - This guest cannot replace its kernel (§1.1).
  - The user tools need Python 2 and Cython. Ubuntu 24.04 has neither, and the references do not specify versions.
- **Paper.** It is unopened, so the observation model is **code-derived**. Roadmap §3.3 says Scribe records processes, threads and shared memory, while the §5.2 detection model is syscall effects on kernel objects. The code agrees, but this was not checked against the paper (decision A1).
- **Analysis, not run.** None of the five dependencies matches a RacePro race class. Each dependency lives in user-space memory, guarded by futex-backed locks, and RacePro excludes FUTEX resources and memory events from its candidates.
- **The paper must not say "RacePro would not report the failure".** Validation goes live after the cutoff, and a cFS oracle could fire under an unrelated candidate.
- **Earlier attempt (2026-10-07).** It is judged in §6.

### 2.2 SimRacer, SIMEXPLORER, RRF, SimEvo, ReDPro: `material_unavailable` ([yu-group.md](yu-group.md))

- **No implementation, VM image or script was found** for any of the five. The author page, Zenodo, figshare and the publishers are blocked, so this means "not found in reachable sources", not "does not exist".
- **No paper of the five could be opened.** The only primary texts from this group that were read are DESCRY and the SCMiner preprint.
  - Platform statements are excerpts, or for SimRacer an inference from a DESCRY co-citation.
  - The Simics and Pin requirements are therefore **unverified**. Their absence here is verified but is not counted as a documented blocker.
- **ReDPro** is not filled anywhere, per roadmap §4.6 and [P8].
- **Analysis.**
  - Only SimRacer's syscall view has any support, and that support is inferred.
  - RRF's yield points could in principle sit at the sites named in the reports for C1, C3, C5 and R2.
  - A paraphrase of the SimExplorer dissertation mentions shared memory, so a syscall-only reading of this family may be too narrow.

### 2.3 DESCRY: `material_unavailable` ([descry.md](descry.md))

- **The paper was opened and read in full** (author PDF, sha256 `f9d7b57b…`). This is the only one of the seven unavailable tools with a fully read primary source.
- **No code exists in any reachable location.** The only lead, the cs.uky.edu project page, is denied by this session's allowlist (`x-deny-reason: host_not_allowed`), not by the remote server.
- **The toolchain is absent here and partly unspecified.** KLEE on LLVM 3.4 has no tag given. For Simics, no version or target is given.
- **Input.** DESCRY's mandatory input is the default log of the failed run. It is in none of the five issue bodies, and the comments are unread (decision A6).
- **Analysis.** DESCRY's documented scope excludes intra-process (thread-level) bugs. Its events are syscalls, and its relations are fork, wait, pipe and signal. The cFS dependencies are thread-shared memory inside one process. R2 and C3 have no printed failure point.

### 2.4 SysPro: `material_unavailable` ([syspro.md](syspro.md))

- **Not readable.** The only known artifact location, `ReproSys`, is not publicly readable. The paper (arXiv, SSRN, JSS) cannot be opened here.
- **No requirement is from-reference.** PIN and its version, srcML, NLTK, Gensim and SPMF are all from secondary sources only.
- **Not installed on purpose.** Choosing versions would be a non-reference choice.
- **Analysis.**
  - The publication in C1, C3, C5 and R2 is a memory store with no syscall of its own.
  - A SysPro-style sleep could act only at a hand-picked nearby syscall, and whether SysPro's front end would pick one is unknown.
  - In C5, at the reference OSAL, a delay after `clone3` holds the child back. Past about 300 ms it aborts the child with `OS_ERR_OBJECT_IN_USE`.
  - The token probe on the issue bodies is a stand-in, not SysPro's stage 1.

---

## 3. Five-case observation-model matrix (analysis, NOT run)

The dependency column uses the reference cFS: bundle `088b2fa8`, osal `d2d877a6`, cfe `c5fb2b4d`, cf `15a871e6`. These are not the historical case revisions, which task A pins. Native cFS is **one Linux process**, and every OSAL task is a `pthread_create` thread (`os-impl-tasks.c:568`), which appears as `clone3` on this glibc.

| Case | Dependency (where it lives) | RacePro (code) | SimRacer/SimEvo (inferred/excerpt) | SIMEXPLORER (excerpt) | RRF (paraphrase) | DESCRY (paper) | SysPro (secondary) | OSAL probe (measured or source) |
|---|---|---|---|---|---|---|---|---|
| C1 CF#184 | OSAL name-table entry for the count semaphore: creator `OS_CountSemCreate` versus CF's `OS_CountSemGetIdByName` (`cf_cfdp.c:1282`). Memory under a PI mutex. | No case-dependency candidate | Not a shared-resource event | Unknown | Possibly via yield points | No | Not observed. Delay only at a hand-picked provider-init syscall. | **Not visible: 0 syscalls (measured on OSAL's own test)** |
| C2 cFE#198 | Late-init readiness inside private EVA CWS apps | No case-dependency candidate (state not modelled) | No target code | No target code | No target code | Not applicable (no source) | Not applicable (no source) | Not visible (source reasoning) |
| C3 cFE#73 | `CFE_SB_Global.AppId`, EVS AppId and filter data, used on TIME's `CreatePipe` path | No case-dependency candidate | Not a shared-resource event | Unknown | Possibly | No (no syscall; no printed failure point) | Not observed. Delay at SB-thread startup syscalls (hand-picked). | **Partly.** Pipe `mq_open("<pid>.<name>")` visible; AppId and filter state not visible. |
| C5 cFE#72 | ES `AppTable`/`TaskTable` write after `OS_TaskCreate`, versus the child's `CFE_ES_GetAppID` | No case-dependency candidate (`clone` is only a happens-before edge) | Only the `clone3` half | Unknown | Possibly via a yield point | No (only `clone3` visible) | Not observed. `clone3` anchor blocks or aborts the child at the reference OSAL. | **Records not visible.** `clone3` and the PSP `prctl(PR_SET_NAME)` (source-level) are visible. |
| R2 cFE#2663 | `CFE_SB_Global.AppId` set in SB AppInit, versus ES/EVS calls into SB | No case-dependency candidate | Not a shared-resource event | Unknown | Possibly, if it is a scheduling race (roadmap §7.3) | No (prints nothing: `cfe_evs.c:185-188`) | Not observed; hand-picked anchor, if any | **Partly**, as for C3 |

Every cell is a hypothesis for the §4.7 "event observation" and "resource representation" stages. None is a detection or reproduction result.

---

## 4. What the OSAL visibility probe showed, precisely ([osal-visibility.md](osal-visibility.md))

**What it is.** It is a probe of what a **syscall-level observer** sees in OSAL and cFE. Such an observer is strace, ptrace or seccomp, RacePro's candidate model, or DESCRY's relations. It is **not a run of any baseline tool.** It also did not measure what a memory page-ownership recorder such as Scribe would log.

### 4.1 Setup

| Item | Value | Status |
|---|---|---|
| OSAL revision | `d2d877a` (tag `v7.0.1`), the gitlink of cFS `main` `088b2fa` | From the bundle gitlink |
| Build | Exactly README L24-30: `cmake -DENABLE_UNIT_TESTS=true -DOSAL_SYSTEM_BSPTYPE=generic-linux -DOSAL_CONFIG_DEBUG_PERMISSIVE_MODE=TRUE ..`, then `make`, then `make test`. Run as uid 1000. | Build: `from-reference`. Run user: `unspecified_by_reference` in README, chosen per Config Guide L170 (permissive mode for a normal user). |
| `make test` | 84/85 passed. `network-api-test` failed only on IPv6 (`EAFNOSUPPORT`), which is a container limit and was not worked around. | Result |
| Trace target | OSAL's own `count-sem-test`. No new test code was written. | — |
| Trace command | run1 used `strace -f -e trace=all` (base command from the task text, not a policy reference). 11 more runs added `-tt -T -s 65535 -k`. | `unspecified_by_reference` agent choices. `-k` perturbs scheduling. |
| Not met | OSAL CI's mqueue sysctls (512/512 against 10/256 here) and the CI image | Decision A13. No effect on count semaphores. |

### 4.2 Results, measured on OSAL's own tests

1. **Create.** Three `OS_CountSemCreate` calls (count-sem-test.c:105, :106 duplicate, :109) issued **zero syscalls**. The create window holds only the harness's `write` markers. This held in all 12 traces.
2. **Lookup.** Four `OS_CountSemGetIdByName(…, "Test_Sem")` lookups (:35 twice, :49, :63) issued **zero syscalls**.
   - In the stack-annotated runs, each of the two functions has exactly one syscall per run under it, a `FUTEX_WAKE`. They come from BUGCHECK console prints on the NULL-argument test paths, not from successful calls.
   - `OS_ObjectIdAllocateNew`, `OS_ObjectIdFindByName`, `OS_ObjectIdFinalizeNew`, `OS_Lock_Global_Impl` and `OS_Unlock_Global_Impl` showed 0 each.
3. **Names.** In each of the 11 runs with `-s 65535`, exactly 13 syscalls carry an object name. All 13 are `write(1, …)` from the UtAssert harness's report (`UtAssert_DoReport` → `OS_BSP_ConsoleOutput_Impl`). **None comes from OSAL itself.**
4. **The semaphore's kernel identity.** It is `sem_init(&impl->id, 0, …)`: unnamed and process-private. It shows up only as a futex word address (`OS_impl_count_sem_table+32`), only when a Take blocks or a Give wakes, and is tied to "Test_Sem" only through a symbol table.
5. **Contrast: `osal-core-test`** (624/624 passed).
   - **Queues are named kernel objects.** There were 69 `mq_open("<pid>.<name>")` calls, each followed by `mq_unlink`, with 64 distinct names. cFE SB pipes are OSAL queues.
   - **By-name lookups are still invisible.** `OS_QueueGetIdByName` and `OS_TaskGetIdByName` issued 0 syscalls. BinSem and MutSem creates issued 0.
   - **A registry wait** (a new task found its record still RESERVED) appeared **only** as futex wait/wake on the task-table lock and condvar addresses. Neither a name nor record content appeared.
6. **glibc check** (the only agent-written execution; it tests glibc, not OSAL or cFS). On this host `pthread_setname_np` becomes `prctl(PR_SET_NAME, "<name>")`.

### 4.3 Source-level statements (not executed in cFS; decision A10)

- **ES registration** (`CFE_ES_AppCreate`, `CFE_ES_StartAppTask`) writes only to `CFE_ES_Global` tables, under `SharedDataMutex`, a PI pthread mutex. `CFE_ES_GetAppID` is a memory lookup through `pthread_getspecific`.
- **The pc-linux PSP** calls `pthread_setname_np` on `OS_EVENT_TASK_STARTUP`, so task names do reach the kernel. They come through the PSP, not OSAL, and after OSAL registration. That call is not ordered with the ES record write.
- **SB and EVS AppId** are set by memory stores (`cfe_sb_task.c` L138, `cfe_evs_task.c` L312).

### 4.4 Side finding H-OSAL-1 (source reading only; **unverified**; decision A11)

- **Claim.** A by-name lookup that matches a RESERVED record gets `OS_ERR_INCORRECT_OBJ_STATE` and returns without releasing the type lock. The creator would then block forever in `OS_ObjectIdFinalizeNew`.
- **Code checked.** The same path exists in OSAL `42af0f73`, the C1 case's revision.
- **Status.** Not executed. Must not be cited as a result.

### 4.5 What the probe does not show

- **Not cFS, not a tool.** It was not run inside cFS, and it is not any tool's output.
- **Uncontended locks only.** Under contention, `FUTEX_LOCK_PI` and condvar waits on anonymous addresses would appear.
- **No page-ownership recording.** Scribe's page-ownership recording was not measured.
- **One OSAL revision only.** The historical case revisions were not executed.
- **Not a missed-detection result.** It supports "not observable at syscall level" for the C1 publish/lookup pair (measured on OSAL), and source-level statements for C3, C5 and R2. It never supports "tool X misses case Y".

**Consistency with the smaller probes.** The DESCRY and SysPro reports ran agent-written C programs. They are not tools and not cFS. Both showed no syscall for `sem_init`, `sem_post` without a waiter, or an uncontended (default or PI) mutex plus a table access. Both showed `pthread_create` → `clone3` and `mq_open` → `mq_open`. The OSAL probe supersedes them with OSAL's own tests.

---

## 5. Comparisons the paper can make

### 5.1 Labels (roadmap §3.5; policy)

- **`ORIGINAL-RUN`** means the authors' implementation was executed on the cFS cases with every documented requirement met.
- **`DOCUMENTED-SETUP-RUN`** means a run that follows a documented setup procedure exactly, with no cFS-specific additions. It is either the tool's own install plus a supported example (the smoke test in roadmap §3.5), or the target's documented build plus its own tests.
- **`DEVIATION-RUN`** means a run in which a documented requirement was substituted. The policy allows it only as a recorded `deviation`.
- **`EXTENSION`** means the original tool with cFS support added (roadmap §3.5).
- **`REIMPLEMENTATION`** means the paper's method rebuilt without the original tool. It is never reported as the original tool's result.
- **`ANALYSIS`** means an observation-model analysis. It is labelled "analysis, not run" and carries the tool's evidence grade.

### 5.2 Possible or impossible here, per tool

| Tool | `ORIGINAL-RUN` | `DOCUMENTED-SETUP-RUN` | `DEVIATION-RUN` | `EXTENSION` | `REIMPLEMENTATION` | `ANALYSIS` |
|---|---|---|---|---|---|---|
| RacePro | **Impossible here.** Needs a bootable i386 2.6.35-scribe kernel, but there is no kexec, no modules and no KVM (R1–R4). Elsewhere it needs a separate machine (A3(b)) with an unreferenced userland and an i386 cFS rebuild (A4). | **Impossible here**, for the same blocker. The user-space part alone would not be a documented setup either: its documented target is i386, the x86_64 build fails on i386-only `pt_regs` and the `-m32` build is a `deviation`, and Python 2 and Cython are absent. | QEMU TCG (A3(c)) only: emulated CPU, over the disk budget, userland unreferenced. racepro.md recommends A3(a) instead. | **Impossible here.** It needs the original code running. | Not assessed in racepro.md. The candidate rules are readable in the public code (§5.2), but checking them against the paper is open (A1). Needs your decision (A9). | **Done** (code-derived) |
| SimRacer, SIMEXPLORER, RRF, SimEvo | **Impossible**: no implementation (R1) | **Impossible**: no implementation | Not possible: no code | **Impossible**: no code | **Impossible now.** The method sections are unread; decide after A1 (yu-group D4). | **Done** (inferred, excerpt or paraphrase) |
| ReDPro | **Impossible**: no implementation | **Impossible** | Not possible | **Impossible** | **Impossible now**: method not filled (roadmap §4.6) | **Not filled** (roadmap §4.6) |
| DESCRY | **Impossible**: no implementation (Q1). Also KLEE/LLVM 3.4 and Simics are absent, and the failed-run log is not in the issue bodies. | **Impossible**: no implementation. Stock KLEE through its Ubuntu 14.04 image would not be DESCRY. | Not possible: no code | **Impossible**: no code | **Possible only in part, and only with your approval (A9).** The log→source mapping and the syscall-pair flipping are documented in the opened paper. The KLEE and Simics parts have no documented versions, so each choice would be a `deviation` or `unspecified_by_reference` item. The input also needs a failed-run log (A6). | **Done** (paper) |
| SysPro | **Impossible**: artifact not readable, paper unread | **Impossible** | Not possible: no code | **Impossible**: no code | **Impossible now.** The method cannot be copied while the paper is unread (A1); otherwise it would rest on guesses (syspro D3). | **Done** (secondary) |
| OSAL / cFS target side | — | **Done.** OSAL built per README L24-30, with its own `count-sem-test` and `osal-core-test` under strace. A cFS-level strace run is pending (A10). | — | — | — | — |

### 5.3 Proposed §4.7 entries (pending A7)

The detection table, roadmap §4.7:

| Detection baseline | C1 | C2 | C3 | C5 | R2 |
|---|---|---|---|---|---|
| RacePro original tool | NOT_RUN: environment blocker (R1–R4: i386 2.6.35-scribe kernel cannot be booted here) | same | same | same | same |
| SimRacer original tool | NOT_RUN: 자료 부족 (material unavailable) | same | same | same | same |
| SIMEXPLORER original tool | NOT_RUN: 자료 부족 (material unavailable) | same | same | same | same |

The reproduction table, roadmap §4.7 last paragraph:

| Reproduction baseline | C1 | C2 | C3 | C5 | R2 |
|---|---|---|---|---|---|
| RRF original tool | NOT_RUN: 자료 부족 | NOT_RUN: 자료 부족; app source not public | NOT_RUN: 자료 부족 | NOT_RUN: 자료 부족 | NOT_RUN: 자료 부족 |
| DESCRY original tool | NOT_RUN: material_unavailable (Q1); log absent from issue body, comments unread | + app source not public | + no printed failure point | (as C1) | + failure prints nothing (`cfe_evs.c:185-188`) |
| SysPro original tool | NOT_RUN: 자료 부족 (artifact and paper unavailable) | same | same | same | same |

- **SimEvo** stays in the regression role, with `자료 부족 / NOT_RUN`.
- **ReDPro** is not entered.
- **A separate column** labelled "observation-model analysis (not run)" carries §3 of this file, with each tool's evidence grade.

### 5.4 Wording the paper may and may not use

**May use:**
- "None of the eight process-level tools could be run on the cFS cases in our environment." Then give the per-tool blocker: an environment blocker for RacePro, and unavailable material for the other seven.
- "By analysis of each tool's documented or code-derived observation model, none of the five case dependencies is an event in that model." Label this as analysis and grade it per tool.
- "On OSAL's own tests, a syscall-level observer saw zero syscalls for named count-semaphore creation and by-name lookup. Object names appeared in no syscall made by OSAL." Label this as a probe.

**May not use:**
- "RacePro, DESCRY or SysPro fails to detect (or reproduce) C1, C2, C3, C5 or R2."
- Any false-negative count, detection rate or performance comparison.
- "Existing process-level techniques cannot handle order violations." Roadmap §3.3 rules this out.
- "RacePro does not observe shared memory." Scribe records page ownership; RacePro just does not form candidates from it.

---

## 6. Earlier partial attempt (2026-10-07): did its choices follow the documentation?

| Item | What it did | Followed the documentation? | Reuse |
|---|---|---|---|
| Clones in `/home/user/work/procrace/baselines/racepro/` | columbia repos at HEAD; sparse, blob-less kernel clone | **Partly.** The revisions match upstream HEAD. KR names the `nviennot/*` repos, and the fork choice is justified only now, as code-derived. The sparse checkout missed `README.md` (KR), which has since been fetched. | Yes, as source |
| `racepro_libscribe_build.log` | x86_64 `cmake` + `make` of libscribe; failed at `src/debug.c:400-403` (i386 `pt_regs` fields) | **No.** The documented target is i386 (KC L6-8), and the documented work directory is `build/` (DR L29, KR L60). | Only as evidence for R2 |
| `racepro_libscribe_build_m32.log` | Added `-m32`. Also hand-built a `record` binary outside CMake without logging the command. That run left `build-libscribe32/log` with credential-named variables. | **No** (`deviation`: undocumented flag, unlogged extra build) | No (see A5) |
| `racepro_scribe_runtime.log` | `./record -- /bin/true` with no Scribe kernel, giving `can't record: Bad file descriptor`. `grep -c scribe /proc/kallsyms` = 26 | **No.** The prerequisites were absent, so the result is meaningless. The kallsyms count includes `subscribe`/`describe`; the correct count is 0. | No |
| `apt_gcc_multilib.log` | `apt-get install gcc-multilib`, which upgraded glibc `2.39-0ubuntu8.7` → `8.9` on shared state | **No.** It is in no RacePro document. | Provenance only; see the ENV note in §0 |
| `container_kernel_caps.log` | Kernel config, CapEff, `finit_module` probe | **Factual** (taken on `fc-v77`, an inference). It agrees with today. | Yes, as facts |
| `racepro_emulation_feasibility.log` | `/dev/kvm` check, `apt-get -s install qemu-system-x86`, reachability of Ubuntu 10.10 | **Exploratory.** "Ubuntu 10.10" is in no reference (`unspecified_by_reference`). QEMU TCG would be a `deviation`. | Record only |
| `yu-group_container_facts.log`, `yu-group_source_fetch.log` (07:27) | Container observations; author-page fetches (403) | Observations and source probes; no tool condition chosen | Superseded by the 2026-10-08 logs |
| `/home/user/work/procrace/baselines/syspro/` | Empty directory | n/a | Now holds the 2026-10-08 inputs and probes |

**Changes to shared state made by today's passes.**
- `apt-get install poppler-utils` upgraded `libpoppler134` from 9.8 to 9.9 (yu-group.md §6). It is used only as a PDF reader.
- In the OSAL work tree, `chown -R ubuntu` was run (osal-visibility.md §5).
- No tool was installed system-wide.

**Agent choices still marked `unspecified_by_reference`.**
- The OSAL probe's strace flags beyond the base command.
- The SysPro token probe's syscall list and tokenizer.
- The DESCRY and SysPro primitive-probe programs.
- None of these is a tool condition or a research result.

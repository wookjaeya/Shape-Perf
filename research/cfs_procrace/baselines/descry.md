# DESCRY (ESEC/FSE 2017): is it public, what does it need, and can it run here?

- Date: 2026-10-08 (UTC 01:26–01:34; corrected after verification at 02:15–02:20)
- Scope: the RQ1 baseline "DESCRY original tool" (roadmap §3.2, §3.3, §4.4, §4.6, §4.7, §13.2 [P5]). Its role in the roadmap is a reproduction baseline for cases that have a log, not a detector.
- Policy: `CONDITIONS_POLICY.md`. Every requirement below has a reference and a status. Nothing was substituted to make anything run.
- **Verdict: `material_unavailable`.**
  - No DESCRY implementation could be obtained. The paper gives no code, artifact or download location.
  - No public repository was found. The one candidate page (`cs.uky.edu/~tyu/research/descry`) is blocked by this session's egress allowlist, so its contents are unknown.
  - Two further problems remain even with the code:
    - **Environment:** the documented toolchain (KLEE on LLVM 3.4, the Simics Virtual Platform) is absent here, and its versions are not fully specified.
    - **Inputs:** no failed-run log in any of the five issue **bodies**. DESCRY's mandatory input is a default log of the failed run. The comments (C1 5, C2 11, C3 14, C5 9, R2 2) were **not read** because `issue_read` was denied. The input is therefore **unverified for comments**, not shown to be absent.
- **No DESCRY component was run.** All five cells stay `NOT_RUN`. DESCRY has **not** been shown to "fail to reproduce" or "fail to detect" any case.

Notation: `pN rN.txt:L` means page N of the paper, line L of `/home/user/work/procrace/baselines/descry/text/rN.txt`. That text was extracted by `pdftotext -f N -l N` from the author PDF (sha256 `f9d7b57b…dfbf60`, see §1). Every log named below is in `/home/user/Shape-Perf/research/cfs_procrace/baselines/logs/`.

---

## 0. Decisions needed from you

| # | Decision | Options | Default if you do not decide |
|---|---|---|---|
| D1 | **How to get the implementation.** The paper has no artifact link. A GitHub search found none, and a third-party artifact dataset also lists none for this paper (§1). The only lead is the project page that SCMiner cites as `http://cs.uky.edu/~tyu/research/descry`. This session's proxy denies it with `403`, `x-deny-reason: host_not_allowed`, "Host not in allowlist: cs.uky.edu. Add this host to your network egress settings to allow access." The remote server never refused it. | (a) Accept `material_unavailable`. (b) Add `cs.uky.edu` (project page) and `uknowledge.uky.edu` (the Zaman dissertation `cs_etds/119`, which may describe the implementation) to this session's network egress allowlist. Both hosts return the same allowlist denial (`descry_report_verification_20261008.log` §3). Or place those pages or files in `paper/` yourself. (c) Ask the authors (T. Yu, T. S. Zaman, C. Wang) for the code and the Simics setup. I have not contacted anyone. | (a). The DESCRY row stays `NOT_RUN — material_unavailable`. |
| D2 | **Whether to build a "DESCRY-method reimplementation".** Under roadmap §3.5 this is a separate category from the original tool. | (a) No reimplementation. (b) Reimplement only the log→source mapping and the syscall-pair flipping, labelled as a reimplementation. Its KLEE and Simics parts have no documented versions here, so each choice would be a `deviation` or `unspecified_by_reference` item for your approval. | (a). |
| D3 | **How to record DESCRY against the five cases, given that no issue body contains a failure log and no comment was read** (§6). `CONDITIONS_POLICY.md:7` counts the comments as part of the original report: C1 has 5, C2 11, C3 14, C5 9 and R2 2. | First, **please supply the comment threads of CF#184 and cFE#198, #73, #72, #2663, or give this session `issue_read` access to `nasa/CF` and `nasa/cFE`**, so that the input status can be settled. Until then: (a) for every case, record "no failed-run log in the issue body; N comments not read (자료 미확인)", separately from the tool blocker. (b) Also design derived experiments that generate a cFS console log (for example C1 with the pre-fix CF). Their results would be labelled as derived, and they still need D1. | (a), with the input status marked "unverified for comments". |

**Recommendation:** D1 = (b) or (c), D2 = (a), D3 = (a).

Even if the code arrives, §6 shows that DESCRY's documented model would very likely not see these dependencies:
- Its scope is inter-process (system-level) bugs, as distinct from intra-process (thread-level) bugs, which "often corrupts only volatile memory within a process" (p1 r1.txt:81-91). Its events are system calls between processes, which it assumes to be single-threaded "for ease of presentation" (p3 r3.txt:145-148).
- The five dependencies are memory state shared between threads of one process.

That is an analysis, not an experimental result, and it must be reported as such.

---

## 1. Sources opened and not opened

| Source | Status | Evidence |
|---|---|---|
| DESCRY paper, author copy `https://chaowang-vt.github.io/pubDOC/YuZW17.pdf` (roadmap [P5]) | **OPENED, all 11 pages read.** The HTTPS URL itself is blocked. The identical file was taken from the site's git repo (`github.com/chaowang-vt/chaowang-vt.github.io` @ `f3544731`, path `pubDOC/YuZW17.pdf`). It was copied to `/home/user/work/procrace/baselines/descry/paper/`. sha256 `f9d7b57bbb0c1269f9143b4ec69286e90348a58fadd071f76f11022e06dfbf60`. pdfinfo: 11 pages, CreationDate 2017-07-17. | `descry_paper_requirements_20261008.log` |
| ACM DL / DOI 10.1145/3106237.3106266, NSF PAR copy `par.nsf.gov/servlets/purl/10075449` | NOT OPENED (403 / `EGRESS_BLOCKED`). The author copy above was used instead. | `descry_source_probe_20261008.log` |
| DESCRY project page `http://cs.uky.edu/~tyu/research/descry` (cited as "[45] 2016" by the SCMiner preprint, `yu-group/text/Scminer_preprint.txt:686`, as where the benchmarks "have been used in other research") | **NOT OPENED.** The proxy returned 403 for http and https, with and without `www`, and WebFetch returned `EGRESS_BLOCKED`. The Wayback Machine is also blocked. The 403 is this session's egress allowlist, not the remote server: `curl -i http://cs.uky.edu/~tyu/research/descry` (2026-10-08T02:17Z) gives `HTTP/1.1 403 Forbidden`, `x-deny-reason: host_not_allowed`, body "Host not in allowlist: cs.uky.edu. Add this host to your network egress settings to allow access." (`uknowledge.uky.edu` gives the same.) **Its contents (code or not) are unknown.** | `descry_source_probe_20261008.log`; `descry_report_verification_20261008.log` §3 |
| GitHub: repository and code search for DESCRY | DONE. No implementation found. The hits were unrelated `descry` projects (a Go rules engine, a Go uptime monitor, a Rust LLM tool), bibliography mirrors and word lists. `git ls-remote` on six plausible author repo names: none is public. The author accounts were also listed in full: `user:chaowang-vt` has 7 public repos (GridLayout, SendMessage, safetyguard, StopWatch, BeerAdviser, chaowang-vt.github.io, TestingIMath) and none is DESCRY; `user:tarannumzaman` has 1 (`Scminer`). | `descry_github_search_20261008.log`; `descry_report_verification_20261008.log` §2 |
| `github.com/morgen52/SE-artifact` @ `9eb9386a`, `data/papers.csv` row 1504 | OPENED. This is the dataset of the JSS study "Research Artifacts in Software Engineering Publications". Its annotators read each paper and recorded artifact URLs (README §1.2). The DESCRY row reads `FSE-2017-63,DESCRY: …,,,,`, i.e. **no artifact URL**. This is a third-party annotation and agrees with my own read. | `descry_github_search_20261008.log` |
| `github.com/wcventure/ConcurrencyPaper` @ `43c76f5f`, `README.md:168` | OPENED. It links only the paper, with no `[Source]` link, while its neighbours have one (CONCURRIT, L169). | same log |
| SCMiner preprint (`tarannumzaman/Scminer` @ `255933c6`), from the earlier yu-group pass | Re-read the lines about DESCRY (refs [17] and [45], related-work L638-648). It mentions no DESCRY code. | `yu-group_opened_sources_20261008.log` |
| SysPro arXiv 2601.09616 (same first authors). A search snippet says they "could not run a direct head-to-head comparison with Descry" because Descry needs console logs. | **NOT OPENED** (`EGRESS_BLOCKED`). Recorded as a **lead only**. | `descry_websearch_leads_20261008.log` |
| Zaman PhD dissertation `uknowledge.uky.edu/cs_etds/119` | NOT OPENED (403). It may describe the implementation. | `descry_websearch_leads_20261008.log`, earlier yu-group probe |
| Simics documentation (Intel public release page, Wind River academic page) | NOT OPENED (`EGRESS_BLOCKED` / 403). Search snippets are recorded as **leads only**. | `descry_websearch_leads_20261008.log` |
| KLEE (`github.com/klee/klee`) | Tags listed, and commit dates of v1.2.0, v1.3.0 and v1.4.0 fetched. At v1.3.0 (`3cc70a08`, 2016-11-30) and v1.4.0 (`9fb2f566`, 2017-07-21), `Dockerfile` and `.travis.yml` were read from blob-less clones, plus `README.md` at v1.4.0 (Q3, Q4). Nothing was built. | `descry_source_probe_20261008.log`; `descry_report_verification_20261008.log` §4 |
| cFS case reports C1, C2, C3, C5, R2 | Issue **bodies** read through GitHub issue search. **Comments not read**: `issue_read` was denied for `nasa/cfe`. Comment counts from the search: C1 CF#184 5, C2 cFE#198 11, C3 cFE#73 14, C5 cFE#72 9, R2 cFE#2663 2. | `descry_cfs_case_inputs_20261008.log`; `descry_report_verification_20261008.log` §1 |

---

## 2. What DESCRY is, according to the paper

- **Problem.** The paper defines the problem as follows: "Given the source code of a set of processes under debugging (PuDs) and default logs generated by these PuDs in a failed execution, compute the data inputs for these PuDs and their interleaving schedule such that the failure can be deterministically reproduced" (p3 r3.txt:91-94).
- **Pipeline (Fig. 1, Fig. 5).**
  1. **PuD identification** from the running process set `P_all` (p4 r4.txt:98-99).
  2. **Log analysis**: messages are mapped to logging points, and goal lists are built from them (§4).
  3. **Guided symbolic execution** produces the inputs (§5).
  4. **Schedule generation** permutes syscall event pairs (§6).
  5. **Replay** checks whether the failure is reproduced.
- **Implementation.** "our static program analysis for mapping log messages to program statements was implemented in LLVM [1], our log-guided symbolic execution was implemented using KLEE [5], and our interleaving schedule generator was implemented using the Simics Virtual Platform [8]" (p7 r7.txt:145-146). The introduction adds "for deterministic replay" (p2 r2.txt:49-50).
- **Is the implementation public?** **No public location is given or found** (§1). The paper contains only three URLs: the DOI, the auditd man page, and an example `http://cgi-bin/hello.cgi` (`descry_paper_requirements_20261008.log`, "whole document URL list").

---

## 3. Documented requirements

Status uses the policy terms. "Met here?" is judged against this container (`descry_container_facts_20261008.log`).

| # | Requirement | Reference | Status | Met here? |
|---|---|---|---|---|
| Q1 | The DESCRY implementation itself: the LLVM log analyser, the KLEE-based log-guided searcher, and the Simics-based schedule generator and replayer | p2 r2.txt:49-50; p7 r7.txt:145-146 | from-reference | **No.** It was not obtained (§1). This is the primary blocker. |
| Q2 | KLEE: "the most recent version of KLEE built from LLVM 3.4" | p8 r8.txt:114 | Tool from-reference. The **exact tag or commit is `unspecified_by_reference`**: the experiment date is not given. For context, v1.3.0 = 2016-11-30 and v1.4.0 = 2017-07-21. | **No.** KLEE is not installed and has no apt package. DESCRY's own KLEE changes (goal-distance searcher, pruning, seeding, §5) are part of Q1. |
| Q3 | LLVM 3.4, the front-end for both the log analysis and KLEE | p7 r7.txt:145; p8 r8.txt:114. For "KLEE built from LLVM 3.4", KLEE's own build recipe at both candidate tags: v1.3.0 `Dockerfile:1` `FROM ubuntu:14.04`, `:8` `LLVM_VERSION=3.4`, `:20-23` apt `clang-/llvm-/llvm-dev/llvm-runtime-${LLVM_VERSION}`. v1.4.0 has the same at `Dockerfile:1`, `:8` and `:24-27`. v1.4.0 `.travis.yml:2` also has `dist: trusty`. | LLVM from-reference (major.minor only). KLEE documents the environment as Ubuntu 14.04 with apt `llvm-3.4`. The KLEE tag, and with it the rest of the recipe, is `unspecified_by_reference` (Q2). | **No.** Only LLVM/clang 18 is present. `llvm-3.4` is not in the apt sources, and `clang-3.4` has no candidate. KLEE's documented route (an Ubuntu 14.04 image) was **not attempted** for two reasons. First, DESCRY's modified KLEE (Q1) is missing, so a stock KLEE would not be DESCRY. Second, DESCRY does not name the KLEE tag. Feasibility of that route here is **untested**: the Docker CLI and `dockerd` binaries are installed, but the daemon is not running ("Cannot connect to the Docker daemon at unix:///var/run/docker.sock"), and 2.4 GB is free (`df`, 2026-10-08T02:16Z). |
| Q4 | An SMT solver for the path conditions | p6 r6.txt:33 ("an SMT solver"). KLEE's Docker recipe: v1.3.0 `Dockerfile:9-10` `SOLVERS=STP:Z3`, `STP_VERSION=master`; v1.4.0 `Dockerfile:9-10` `SOLVERS=STP:Z3`, `STP_VERSION=2.1.2`. | `unspecified_by_reference`: DESCRY names no solver or version. KLEE's Docker recipe uses STP plus Z3, with a tag-dependent STP version. KLEE's CI at both tags also builds LLVM 3.4 with STP alone, Z3 alone and metaSMT (v1.4.0 `.travis.yml:47-93`; v1.3.0 `.travis.yml:34-64`). So KLEE's documentation does not fix DESCRY's solver either. | Not reached. The candidates are limited to the solvers KLEE supports at that tag. |
| Q5 | The Simics Virtual Platform, for the schedule generator and deterministic replay | p2 r2.txt:50 ("[8, 38]"); p7 r7.txt:146; ref [8] p11 r11.txt:23-24 (a 2010 book chapter); ref [38] p11 r11.txt:117-119 (SimRacer, ISSTA 2013, by the same first author and Simics-based, a plausible source of the Simics setup) | Product from-reference. **Edition, version, licence, simulated machine, guest OS/kernel and scripts are all `unspecified_by_reference`.** No SimRacer implementation or Simics script was found either (`yu-group_github_search_20261008.log:34`), so [38] offers no route to the version or target. | **No.** Simics is not installed, and its download and documentation hosts are blocked. Whether Simics would run on this Firecracker guest (no `/dev/kvm`, no vmx flag, no module loading) is **unverified**. *Lead only, not a blocker: the paper names no Simics release. An unopened search snippet says today's Intel public release needs a EULA and about 10 GB installed. Free disk varies: 3.1 GB at 01:27Z and 2.4 GB at 02:16Z on 2026-10-08.* |
| Q6 | The running process set captured by system tools "such as the Linux Auditd Daemon", with `/var/log` used to identify the active processes | p5 r5.txt:3-4, 11-17 | from-reference (no version or rules given) | **Possible in principle.** `CONFIG_AUDIT=y`, `CONFIG_AUDITSYSCALL=y`, `CAP_AUDIT_CONTROL` present, and a `NETLINK_AUDIT` socket opens. `auditd` is not installed (apt candidate 1:3.1.2). It was not installed, because nothing would consume it without Q1. |
| Q7 | Target: "multi-process applications written in C/C++", with their source code. Scope: inter-process (system-level) bugs, not intra-process (thread-level) bugs. | Capability: p2 r2.txt:51-52. Source: p1 r1.txt:96-97. Scope: p1 r1.txt:81-91. The evaluation used Linux applications only (p1 r1.txt:32; p2 r2.txt:52-53). That describes the evaluation and does not state a limit. | from-reference | cFS is C with source, and the reference native build exists (`env/`). **But it runs as one process with many threads**, and its dependencies are thread-level memory state (§6). |
| Q8 | Process model: "assuming each process has one thread" | p3 r3.txt:145-148, which states it "for ease of presentation". Program order is defined over "the same process/thread" (p7 r7.txt:29-30). | **Modelling assumption, stated for presentation, not a stated tool limit.** How threads are handled is `unspecified_by_reference`. The stronger scope evidence is Q7 (r1.txt:81-91). | Not a met/unmet condition. For cFS: every app and task is a pthread inside `core-cpu1` (`osal/src/os/posix/src/os-impl-tasks.c:568`), so the cFS dependencies are the intra-process (thread-level) kind that r1.txt:81-91 puts outside DESCRY's focus. |
| Q9 | Default log messages of the **failed** execution, produced at default verbosity | p1 r1.txt:75-76, 96-97; p10 r10.txt:45-46 | from-reference | **Unverified for comments.** None of the five issue bodies has a failed-run log. The comments were not read because `issue_read` was denied: C1 5, C2 11, C3 14, C5 9, R2 2. `CONDITIONS_POLICY.md:7` counts comments as part of the original report (§6, D3, `descry_cfs_case_inputs_20261008.log`). |
| Q10 | A failure point, i.e. "a program statement that prints a failure message" | p3 r3.txt:149-150 | from-reference | Varies by case: C1 has one in the pre-fix source, R2 has none by construction, C3 is a segfault (§6). |
| Q11 | Manual input: the type of input a process accepts (PuD scenario 1) | p5 r5.txt:4-5; p4 r4.txt:177-185 | from-reference | Not reached |
| Q12 | Tool parameters: loop-iteration step N = 10. The loop bound `L_max` is not given. | p6 r6.txt:22 | N from-reference; `L_max` `unspecified_by_reference` | Not reached |
| Q13 | Evaluation conditions: 2-hour time limit, 5 runs each | p8 r8.txt:115-117 | from-reference (evaluation, not installation) | Not reached |
| Q14 | Paper's experimental host: Core i5-2400, 8 GB RAM, Ubuntu 14.10 | p8 r8.txt:113 | from-reference. It describes the experiment, and the paper does not state it as a requirement. | Differs: Ubuntu 24.04.4, 4 vCPU Xeon, 16 GB, kernel 6.18.44-fc-v80. Using this host would be a `deviation`. |

Facts not stated anywhere in the paper:
- the Simics version and target;
- the KLEE commit;
- the LLVM patch level;
- the solver;
- the auditd configuration;
- any code, VM image or artifact location.

---

## 4. What I ran

None of these steps runs DESCRY. There is no DESCRY code here to run.

| Step | Command (abridged; full commands are in the logs) | Log |
|---|---|---|
| Verify and extract the paper | `cp …/yu-group/sources/chaowang-vt.github.io/pubDOC/YuZW17.pdf descry/paper/`; `sha256sum`; `pdfinfo`; `for p in 1..11: pdftotext [-layout] -f p -l p YuZW17.pdf {r,p}p.txt` | `descry_paper_requirements_20261008.log` |
| Container facts | `uname -a`; `/etc/os-release`; `nproc`; `df`; `free`; `ls /proc/modules`; CapEff decode (including the AUDIT bits); `zgrep CONFIG_{MODULES,KEXEC,KVM,AUDIT,AUDITSYSCALL,…} /proc/config.gz`; `ls /dev/kvm`; vmx/svm count; ctypes `finit_module`/`kexec_load` → ENOSYS; `socket(AF_NETLINK, SOCK_RAW, 9)`; `strace -f /bin/true`; `gdb -batch -ex run /bin/true`; look for simics, klee and llvm; `apt-cache policy llvm-3.4 clang-3.4 klee auditd qemu-system-x86` | `descry_container_facts_20261008.log` |
| Reachability of artifact hosts | `curl -sS -o /dev/null -w '%{http_code}'` on cs.uky.edu (4 URL forms), homepages.uc.edu, chaowang-vt.github.io, dl.acm.org, doi.org, web.archive.org, zenodo, klee-se.org, github.com; `git ls-remote` on 6 guessed author repos and on `klee/klee` tags; tag dates via bare `git fetch --depth 1 --filter=tree:0` | `descry_source_probe_20261008.log` |
| GitHub search | MCP `search_repositories`, 2 queries; `search_code`, 2 queries; blob-less clones of `morgen52/SE-artifact` (checkout of `README.md` and `data/papers.csv`) and `wcventure/ConcurrencyPaper` (`README.md`) | `descry_github_search_20261008.log` |
| Web search | WebSearch, 3 queries; WebFetch on cs.uky.edu, arxiv.org, par.nsf.gov, intel.com: all `EGRESS_BLOCKED` | `descry_websearch_leads_20261008.log` |
| Case inputs | GitHub issue search for the bodies of CF#184 and cFE#198, #73, #72, #2663; `git show 2a024d8e:fsw/src/cf_cfdp.c` (pre-fix CF); reads of the reference cFS (`088b2fa8`, cfe `c5fb2b4d`, osal `d2d877a6`, cf `15a871e6`, psp `c4b3b0b6`) and of `env/logs/run01_operational.log` | `descry_cfs_case_inputs_20261008.log` |
| glibc syscall-visibility check (**not a DESCRY run and not a cFS run**) | A 30-line C program (scratchpad `descry_sysvis/prims.c`) calls `sem_init`, `sem_post`, an uncontended `pthread_mutex_lock` plus a table write, `pthread_create`, a child that reads the table under the mutex, and `mq_open`, each between marker `write`s. **Its mutex is a default `PTHREAD_MUTEX_INITIALIZER` mutex (`prims.c:12`).** OSAL's idmap global lock is different: it is `PTHREAD_PRIO_INHERIT` (`os-impl-idmap.c:210`), and `OS_Unlock_Global_Impl` calls `pthread_cond_broadcast` (`:116`). Built with `gcc -O0 -pthread … -lrt` (gcc 13.3.0, glibc 2.39) and run under `strace -f` with loader noise filtered. | `descry_syscall_visibility_check_20261008.log` |
| Re-check with OSAL's mutex type (**not a DESCRY run and not a cFS run**) | `descry_verify/pi.c` runs the same M1–M4 sequence with a `PTHREAD_PRIO_INHERIT` mutex and a `pthread_cond_broadcast` under the lock, in both the writer and the child reader. Built with `gcc -O0 -pthread`. Both it and the original `prims` binary were run under **unfiltered** `strace -f`. | `descry_syscall_visibility_pi_recheck_20261008.log` |
| Direct OSAL measurement (separate report, written after this one) | strace of the real OSAL `count-sem-test` at osal `d2d877a6` | `osal-visibility_strace_analysis_20261008.log` (see `osal-visibility.md`) |
| Report verification (2026-10-08 02:15–02:17Z; nothing installed or run for DESCRY) | MCP `search_issues` for comment counts of the five reports; MCP `search_repositories` `user:chaowang-vt`, `user:tarannumzaman`; `curl -i` on cs.uky.edu and uknowledge.uky.edu; KLEE `Dockerfile` and `.travis.yml` at v1.3.0 and v1.4.0 via `git show` in blob-less clones (scratchpad `descry_verify/klee13`, `klee14`); `docker info`; `df -h`; PSP and OSAL signal lines at the reference commits | `descry_report_verification_20261008.log` |

Result of the visibility checks:
- **Markers M1–M3 have no syscall between them.** These are `sem_init`, `sem_post` with no waiter, and the mutex-protected table write. The child's mutex-protected read also makes no syscall. The same holds with a PI mutex plus `pthread_cond_broadcast`: in the unfiltered re-run, the only syscalls between M1 and M4 are the marker `write`s.
- These results hold for an **uncontended** lock. Under contention, `futex` calls on the lock word would appear. `futex` is not a DESCRY relation (§5), and OSAL's idmap lock is a process-private object, not a system-wide resource.
- The real OSAL agrees: "OS_CountSemCreate x3 issued no syscall" (`osal-visibility_strace_analysis_20261008.log:25`); "no syscall attributable to OS_CountSemGetIdByName itself" (`:196`); "the successful create/lookup calls issued 0 syscalls in every trace" (`:212-213`). No PI futex there comes from `OS_Lock_Global_Impl` (`:221-222`).
- `pthread_create` appears as `clone3(… CLONE_THREAD …)`, which creates a thread and not a process.
- `mq_open` and `mq_unlink` appear as syscalls.

Work directory: `/home/user/work/procrace/baselines/descry/`, 2.6 MB in total (`paper/`, `text/`, `sources/`).

---

## 5. Observation model (from the paper)

| Aspect | DESCRY as documented |
|---|---|
| Execution unit | OS **processes** and **software signals**. Scope: "DESCRY focuses on inter-process bugs … They differ from intra-process (thread-level) bugs … an intra-process (thread-level) concurrency bug often corrupts only volatile memory within a process" (p1 r1.txt:81-87; to :91). Model: "Each process may create multiple threads, but for ease of presentation, we focus only on the process-level concurrency in this work while assuming each process has one thread" (p3 r3.txt:144-148). That is a presentation assumption. Program order is defined over "the same process/thread" (p7 r7.txt:29-30). How threads are handled is `unspecified_by_reference`. |
| Which units are analysed (PuDs) | A process P is a PuD only if one of these holds (p4 r4.txt:177-185): (1) P accepts the same input type as the failing process; (2) it is another instance of the same program; (3) it was spawned by the same application; (4) P is a software signal within the failing process. Active processes come from auditd and `/var/log` (p5 r5.txt:3-4, 11-17). |
| Field input | Source code of the PuDs and the **default logs of the failed run**, nothing else (p1 r1.txt:75-76, 96-97). |
| Log → source | Format strings of printing APIs (`printf`, `sprintf`, …) are matched against log substrings, with inter-procedural back-tracing out of library code (p5 r5.txt §4.1). Messages built from dynamically assembled strings are not identified, and the search then becomes unguided (p5 r5.txt:51-52). Goal lists are built from a log hierarchy graph (§4.2). |
| Events | **System calls.** "the systems calls are modeled as concurrency events" (p3 r3.txt:185). Each schedule event is a shared-resource R/W or a synchronization operation. "Details of shared resource and event modeling can be found in prior work [21, 38]" (p3 r3.txt:179-180), i.e. RacePro and SimRacer. Resources are system-wide (file, device, `/proc`): "Such resources are often accessed through system calls" (p3 r3.txt:168-170). |
| Must-happen-before relations | Program order, fork→return, wait→exit, pipe write→read, and signal enabling (p7 r7.txt:29-39). **None for futex/mutex, POSIX semaphores, POSIX mqueue, shared memory, or user-level registries.** |
| Exploration | Two PuDs at a time, the failing process paired with one other (p7 r7.txt:127-128). **Definition:** "DESCRY starts from events in P F and selects the event e f closest to the failure point. Next, it selects an event ei that is close to but does not have order relations with e f. The event pair (ei, e f) is called a suspicious event pair" (p7 r7.txt:90-94). DESCRY flips each suspicious pair while keeping the order relations of the partial-order graph (r7.txt:94-97). **Prioritisation, not part of the definition:** "DESCRY prioritizes event pairs where at least one of the two events is a write system call and is closer to the failure point" (p7 r7.txt:136-137, continued at r7.txt:89). |
| Replay and oracle | Simics replays the PuDs while "controlling the system call events" (p7 r7.txt:112-115). Success means reaching the **failure point**, a statement that prints the failure message (p3 r3.txt:149-150; p8 r8.txt:134-136). |
| Purpose | **Reproduces** a known field failure. It does not search for unknown races (the paper's §8 contrasts it with fault detection, p10 r10.txt:55-65). |

---

## 6. The five cFS cases: could DESCRY observe them in principle? (analysis, NOT a run)

Two common facts:
- Native cFS runs as **one Linux process** (`core-cpu1`, `env/logs/run01_operational.log.meta`). Every cFE core app, app and child task is a `pthread_create` thread (`os-impl-tasks.c:568`). A bug between those threads is an intra-process (thread-level) bug, which the paper sets apart from DESCRY's inter-process focus (p1 r1.txt:81-91; §5, row 1).
  - Under DESCRY's PuD rules (p4 r4.txt:177-185) the whole flight software is **one** process, so there is **no second process PuD** to pair with (p7 r7.txt:127). Signal PuDs (scenario 4, "P is a software signal within the process P F") exist in principle. The PSP installs `SIGINT`, `SIGTERM` and `SIGFPE` handlers (`psp/fsw/pc-linux/src/cfe_psp_exception.c:179` `sigaction`; `:232-233`; `:266`), and the OSAL timebase waits on RT signals (`osal/src/os/posix/src/os-impl-timebase.c:150` `sigwait`). **None of the five dependencies involves a signal handler.** The PSP keeps the default `SIGSEGV` handler on purpose (`cfe_psp_exception.c:262-266`), so C3's crash prints no failure message in a native run either.
  - The paper models `clone` as a resource event: "The clone system call creates a new process inode under the /proc directory (write)" (p3 r3.txt:171-173). The fork-return relation (p7 r7.txt:31-32) is defined for child **processes**. How `clone3(CLONE_THREAD)` is treated is `unspecified_by_reference`. For C5 this does not matter: the order that matters is between memory accesses.
- cFS **does** produce default console logs (ES syslog lines and `EVS Port1` event lines, `run01_operational.log` L30-44). DESCRY's input type therefore exists for cFS in general. What matters is whether the **failed run's** log exists for each case.

The dependency column below uses the reference cFS. Historical revisions for C3, C5 and R2 still need checking under roadmap task A.

| Case | Failed-run log in the original report? (issue body read; comments not read, `issue_read` denied) | Failure point (a printed message)? | Dependency and its syscall visibility | Could DESCRY's documented model observe it? |
|---|---|---|---|---|
| C1 CF#184 | **No failed-run log in the issue body; 5 comments not read.** The body says only "Its a race condition, so not readily reproducible" and links code (`CF_184_body.md` L13, L25). | **Yes, in the pre-fix source**: `cf_cfdp.c@2a024d8e:1018-1020` `CFE_EVS_SendEvent(... "CF: failed to get sem id for name %s, error=0x%08x" ...)`. | A provider app creates a named count semaphore and CF looks it up by name. The body names the provider as "CI/TO or some other dedicated I/O app" (`CF_184_body.md` L8). The roadmap names BP, from comment [C1-BP] (`roadmap_input_20261007.md:86`, `:622`), but that comment was not re-read here. Creation is `sem_init(&impl->id, 0, …)` (`os-impl-countsem.c:85`), and registration is in the user-space OSAL table. The lookup (`OS_CountSemGetIdByName`, `cf_cfdp.c:1282`) searches that table under OSAL's idmap lock, a PI pthread mutex. Visibility checks: `sem_init`, `sem_post` and an uncontended default or PI mutex plus table access make **no syscall** (M1–M3, §4). The real OSAL issues 0 syscalls for successful `OS_CountSemCreate` and `OS_CountSemGetIdByName` (`osal-visibility_strace_analysis_20261008.log:25, 196, 212-213`). | **No.** When the lock is uncontended, neither the publish nor the lookup is a syscall event. Under contention, the only addition is `futex` on OSAL's private idmap lock, which is neither a DESCRY relation nor a system-wide resource. The provider app and CF are threads of one process, not two PuDs. A derived run with the pre-fix CF could produce the failure log and so satisfy Q9. Even then, the order that matters is not in DESCRY's event set. |
| C2 cFE#198 | **No failed-run log in the issue body; 11 comments not read.** The body names no app, function or log ("observed in the EVA CWS project …"). | Unknown | Late-init readiness inside private EVA CWS apps, whose source is not public | **Not applicable.** The source of the apps involved is not public, and no log is in the body (comments unread). |
| C3 cFE#73 | **No failed-run log in the issue body; 14 comments not read.** The body is a step-by-step debugging narrative from "the Microblaze processor used by the EVA team at GRC". Whether the comments hold a log is unknown. | **No.** The failure "ultimately segfaults and crashes CFE core". No program statement prints a failure message. | SB and EVS AppId globals and the EVS `AppData` filter table are memory. TIME's `CreatePipe` reaches `mq_open`, which is a syscall (visibility check M6), but mqueue is not among DESCRY's relations, and that call is not the dependent state. The report names the Microblaze processor but **not the OS**. The paper evaluated only Linux applications (p1 r1.txt:32), gives the Linux Auditd Daemon as its example process-capture tool (p5 r5.txt:3-4), and models system calls (p3 r3.txt:168-185). Whether C3 falls outside DESCRY's scope on platform grounds cannot be decided from these sources. | **No.** The dependent reads and writes are not syscalls, and there is no printed failure point. |
| C5 cFE#72 | **No failed-run log in the issue body; 9 comments not read.** The body is a code-review style description ("the child thread … read the shared table data before it is fully populated"). | Only if the app prints on a `CFE_ES_GetAppID` failure. That is app-specific, and the body names no app. | The ES `AppTable`/`TaskTable` write happens after task creation, under `CFE_ES_LockSharedData`. Task creation shows up as `clone3(CLONE_THREAD)` (M4). The registry write and the `GetAppID` read are memory (M3; child read). | **No.** Only the `clone3` is visible, and the order that matters is between memory accesses of two threads. |
| R2 cFE#2663 | **No failed-run log in the issue body; 2 comments not read.** The body says "there is no event outputted nor is the return code set to any variable", and the failure was observed with a gdb breakpoint. | **No.** The reference `cfe_evs.c:185-188` sets `Status = CFE_EVS_APP_ILLEGAL_APP_ID` and prints nothing. | The SB global AppId is memory, set in SB AppInit. | **No.** The failure prints nothing, so there is no failure point, and there is no syscall event on the dependency. |

**Analytical conclusion** (label it as analysis, not as a reproduction result):

- **Inputs:** DESCRY's mandatory input, the default log of the failed run, is in none of the five issue bodies. The comments (5, 11, 14, 9 and 2) were not read, so the input status is **unverified for comments**, not "absent" (D3).
- **Model:** the dependencies are thread-shared memory state in one process. DESCRY's documented model has three matching gaps:
  - its scope is inter-process bugs, not intra-process (thread-level) ones (p1 r1.txt:81-91), and its unit is a process (assumed single-threaded for presentation, p3 r3.txt:145-148) or a signal;
  - its events are syscalls;
  - its must-happen-before relations are fork, wait, pipe and signal.

These gaps map onto the roadmap §4.7 diagnosis rows as follows:

| §4.7 row | DESCRY gap for cFS |
|---|---|
| 사건 관찰 (event observation) | No syscall at the publish or use site |
| 자원 표현·연결 (resource identity) | OSAL names and cFE IDs are user-space tables |
| 실행 주체·동기화 모델 (unit and sync) | A thread is not a PuD: the PuD scenarios name only processes and signals (p4 r4.txt:177-185), and the scope excludes intra-process (thread-level) bugs (p1 r1.txt:81-91). Mutex, semaphore, mqueue and startup sync are not among the relations. |
| 결과 판정 (oracle) | The failure must be a printed message. R2 and C3 do not print one. |
| 적용 환경·입력 확보 (environment and inputs) | No implementation, Simics or KLEE-3.4 here. No failure log in any issue body; comments unread (D3). |

Treating OSAL/cFE registries as resources, threads as PuDs, and return codes as failure points would make DESCRY a cFS **extension** under roadmap §3.5. It would not be the original tool.

---

## 7. Earlier partial attempt: did it follow the documentation?

No earlier attempt touched DESCRY's implementation or toolchain. Each item is assessed only for what it means for DESCRY.

| Item | What it did | Followed the documentation? | Reused here? |
|---|---|---|---|
| `yu-group` fetch of the DESCRY PDF (`git clone --filter=blob:none --no-checkout chaowang-vt.github.io` + `git checkout HEAD -- pubDOC/YuZW17.pdf`), `yu-group_github_candidates_20261008.log` | Got the author-hosted PDF that roadmap [P5] names, through git, because the HTTPS URL is blocked | Yes. It is the same file as the [P5] URL (same repo path), and the hash was re-verified today. | **Yes**, as the source PDF |
| `yu-group_apt_poppler.log` | `apt-get install poppler-utils` (it also upgraded `libpoppler134` 24.02.0-1ubuntu9.8 → 9.9) | Not a DESCRY requirement. It is a reading tool and has no bearing on any tool condition. It did change shared container state slightly. | Used `pdftotext` and `pdfinfo` only |
| `yu-group_opened_sources_20261008.log`, the DESCRY quotes | Grepped quotes from the paper | The quotes match today's extraction. **One label is wrong:** the "system-level concurrency fault … system calls" passage is in §2.2 Problem Statement (p3 r3.txt:168-185), not §2.1. | Superseded by `descry_paper_requirements_20261008.log` |
| `yu-group_container_facts{,_20261008}.log`, `container_kernel_caps.log` | Kernel, capability and Simics/PIN presence checks | Factual. They agree with today's re-check (`fc-v77` on 10-07, `fc-v80` today). | As cross-checks; re-verified in `descry_container_facts_20261008.log` |
| `racepro_libscribe_build*.log`, `racepro_scribe_runtime.log`, `apt_gcc_multilib.log`, `racepro_emulation_feasibility.log` | RacePro/Scribe build and run attempts, the `gcc-multilib` install (which upgraded glibc 2.39-0ubuntu8.7 → 8.9), and a QEMU/Ubuntu 10.10 feasibility probe | Not DESCRY-related. They were already judged **not** documentation-conformant in `racepro.md` §7 (i386 target ignored, undocumented `-m32`, runtime without the Scribe kernel, Ubuntu 10.10 not named by any reference). The glibc upgrade predates today's syscall-visibility check, and that check states glibc 2.39 with no package revision assumed. | No |
| `/home/user/work/procrace/baselines/syspro/` | Empty directory | — | — |

---

## 8. Verdict

**`material_unavailable`.** Evidence:

1. The paper names the components (LLVM, KLEE, Simics; p7 r7.txt:145-146) but gives **no code, artifact or download location** (`descry_paper_requirements_20261008.log`, URL list).
2. No public DESCRY repository exists among the GitHub search results. Six plausible author repo names are not public. The authors' accounts hold no DESCRY repo: `user:chaowang-vt` has 7 unrelated public repos, and `user:tarannumzaman` has only `Scminer`. A third-party artifact census annotates DESCRY with **no artifact URL** (`SE-artifact/data/papers.csv:1504`).
3. The only remaining lead, `cs.uky.edu/~tyu/research/descry`, cannot be opened here. This session's proxy denies the host (`403`, `x-deny-reason: host_not_allowed`, "Host not in allowlist: cs.uky.edu"); the remote server never refused it. This report therefore says **"not obtained"**, not "does not exist". The remedy is to add `cs.uky.edu` and `uknowledge.uky.edu` to the egress allowlist (D1(b)).
4. Even with the code, the documented toolchain is not present: KLEE built on LLVM 3.4 and the Simics Virtual Platform. For KLEE, there is no package, and DESCRY does not specify the tag. KLEE's own recipe is an Ubuntu 14.04 image, which was untested here because the Docker daemon is not running. Simics is not installed, its version and target are unspecified, and its hosts are blocked. Using substitutes would be undocumented, so none was tried.
5. Independently of the tool, the **documented input** (a default log of the failed run) is in none of the five issue bodies. The comments (5, 11, 14, 9 and 2) were not read (`issue_read` denied), so this is **unverified for comments** (`descry_cfs_case_inputs_20261008.log`, D3).

**Roadmap entries** (reproduction-baseline table, §4.7 last paragraph):

| Baseline | C1 | C2 | C3 | C5 | R2 |
|---|---|---|---|---|---|
| DESCRY original tool | NOT_RUN — material_unavailable (Q1); log absent from issue body, comments unread | NOT_RUN — material_unavailable; app source not public; log absent from issue body, comments unread | NOT_RUN — material_unavailable; log absent from issue body, comments unread; no printed failure point | NOT_RUN — material_unavailable; log absent from issue body, comments unread | NOT_RUN — material_unavailable; log absent from issue body, comments unread; failure prints nothing (`cfe_evs.c:185-188`) |

Keep the §6 analysis in a separate column labelled "observation-model analysis (not run)": **no case dependency is a syscall-level event in DESCRY's documented model.** Do not count any cell as "not reproduced" or as a false negative.

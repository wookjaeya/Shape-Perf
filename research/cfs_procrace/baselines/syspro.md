# SysPro (arXiv 2601.09616v1 / JSS 2026): can it run here, and what would it observe?

- Date: 2026-10-08 (UTC 01:29–01:39). Revised 02:24–02:30 UTC after review (§9).
- Scope: the RQ1 baseline "SysPro original tool" (roadmap §3.2 L141, §3.3 L159, §4.4 L227, §4.6 L246, §4.7 L282, §13.2 [P6-a] L659–663, [P6-j] L664–668).
- Policy: `CONDITIONS_POLICY.md`. Every requirement below has a reference and a status. Nothing was installed, substituted or tuned to make the tool run.
- **Verdict: `material_unavailable`.** The code/data repository that the paper names (`github.com/tarannumzaman/ReproSys`) cannot be read publicly. The paper itself (arXiv v1, SSRN preprint, JSS article) could not be opened through any channel this session has. No SysPro component exists here, so nothing was run. Every SysPro cell for C1, C2, C3, C5 and R2 stays `NOT_RUN`, recorded as "자료 부족 / material unavailable". SysPro has **not** been shown to "fail to reproduce" or "fail to detect" any case.
- SysPro is a **reproduction** tool. Its input is a bug report, not an unknown program to search. Under roadmap §3.1 and §4.7 L282 it belongs in the separate RRF/DESCRY/SysPro reproduction table, not in the detection table.

---

## 0. Decisions needed from you

| # | Decision | Options | What happens by default (if you do not decide) |
|---|---|---|---|
| D1 | **How to get the SysPro artifact.** The only code/data location known, `github.com/tarannumzaman/ReproSys` (from arXiv v1, as recorded in roadmap L662), is not publicly readable today (§1, §2). The JSS version's data-availability statement has not been read (roadmap L668), so another location cannot be ruled out. | (a) You or a co-author email the authors (first author T. S. Zaman; senior co-author T. Yu) and ask for the artifact or for the repository to be made public. (b) If you already have access to the repository, attach it to this session with your own GitHub account. (c) Accept `material_unavailable`, keep SysPro as a literature-only row, and use the §6 analysis, labelled "analysis, not run". | (c). The SysPro cells stay `NOT_RUN — material unavailable`. No re-implementation is started. |
| D2 | **How to get the paper.** The paper is the only documentation of PIN version, tool chain and run procedure, and it cannot be opened here (§1). | (a) Put the arXiv v1 PDF (and the JSS version if you have access) in `research/cfs_procrace/paper/`. (b) Allow egress to `arxiv.org`. (c) Do without it. | §5 stays **secondary-source derived** (the roadmap author's earlier reading plus web-search excerpts). No paper section number, PIN version or quote is claimed as `from-reference`. |
| D3 | **Whether to re-implement the SysPro method for cFS** (replacing the Linux-syscall front end with cFS/OSAL API descriptions; roadmap §4.4 L227 plans this as this study's extension. That line is project plan wording, not a reading of the paper). | (a) Yes, as a separately labelled "paper-method re-implementation (cFS extension)" under roadmap §3.5. This needs D2 first, so the method is copied from the paper and not guessed. (b) No. | (b). The paper (D2) and the artifact (D1) are unavailable (`material_unavailable`), so the method cannot be copied now, and a re-implementation would rest on guesses. Per roadmap L227, a cFS front end would in any case be this study's extension, not the original tool. |
| D4 | Conditional on D1 (a) or (b): **access to the dependencies**, once the artifact names their versions. `www.intel.com` and `software.intel.com` (PIN), `www.srcml.org` and the SPMF site are all blocked by the egress proxy. Only `pypi.org` is reachable. | Allow those hosts, or provide the files. | Not needed until D1 is resolved. |

**Recommendation:** D1 = (a) together with (c) as the interim record, and D2 = (a). What SysPro's first stage would extract from these five reports is **unknown** until the paper or the artifact is read. The §6 token probe checks only for literal Linux-syscall-name tokens in the issue bodies and says nothing about SysPro's IR/TF-IDF step. The §6 source analysis (analysis, not an experimental result) shows only this much. In C1, C3, C5 and R2, the publication itself is a memory store with no syscall of its own. A SysPro-style delay would therefore have to sit at a nearby syscall chosen from the source. Such syscalls exist, for example the publisher-thread `prctl`/`sched_setaffinity`/`clock_nanosleep` in C3 and R2. Whether SysPro's front end would select one is unknown.

---

## 1. Sources opened and not opened

| Source | Status | Evidence |
|---|---|---|
| `github.com/tarannumzaman/ReproSys` (code/data link from [P6-a], roadmap L662) | **NOT OPENED: not publicly readable.** `git ls-remote` asks for credentials (exit 128), which is how GitHub answers anonymous HTTPS for private or missing repositories. The same command works on the same owner's public `Scminer`. `raw.githubusercontent.com/.../ReproSys/{main,master,HEAD}/README.md` returns 404, while a file in the same owner's public repo returns 200. GitHub search `user:tarannumzaman` lists only `Scminer`. Code search for `"ReproSys" "tarannumzaman"` finds 0 hits. `add_repo` (read) says "not found on github.com, or this session's GitHub credential doesn't have access". | `logs/syspro_repo_probe_20261008.log` |
| Name variants `Tarannum-Zaman/ReproSys`, `tarannumzaman/reprosys`, `{tarannumzaman,Tarannum-Zaman}/SysPro` | Same credential prompt. Not public. | same log |
| Archives: Software Heritage, `web.archive.org`, `archive.org` | **NOT OPENED** (proxy CONNECT 403) | same log, plus proxy `recentRelayFailures` |
| arXiv v1, `arxiv.org/{html,abs,pdf}/2601.09616v1`, `export.arxiv.org`, `web3.arxiv.org` | **NOT OPENED.** curl gets CONNECT 403. WebFetch returns `EGRESS_BLOCKED` (arxiv.org) or `ENOTFOUND` (web3). | `logs/syspro_paper_fetch_20261008.log` |
| JSS: `doi.org/10.1016/j.jss.2026.112785`, `api.crossref.org`, `sciencedirect.com/.../pii/S0164121226000191` | **NOT OPENED** (403 / `EGRESS_BLOCKED`). The data-availability statement of this version is therefore unread. | same log |
| SSRN preprint `papers.ssrn.com ... abstract_id=5035178` (authors as in arXiv v1) | **NOT OPENED** (403 / `EGRESS_BLOCKED`) | same log |
| ASE 2026 Journal-First page (`conf.researchr.org`), `emergentmind.com`, `uknowledge.uky.edu/cs_etds/119`, `par.nsf.gov` | **NOT OPENED** (403 / `EGRESS_BLOCKED`) | same log |
| Other paper/artifact indexes, tried in review (02:24Z): `zenodo.org/api/records`, `figshare.com`, `api.figshare.com`, `www.semanticscholar.org`, `api.semanticscholar.org`, `api.openalex.org`, `export.arxiv.org/api/query`, `scholar.archive.org` | **NOT OPENED** (all CONNECT 403) | `logs/syspro_review_recheck_20261008.log` §3; addendum in `logs/syspro_paper_fetch_20261008.log` |
| Scholar Gateway full-text search | **NOT OPENED** (`ACCESS_DENIED`) | tool response |
| Web-search result summaries (4 queries) | Read. These are **model-written summaries of unopened pages**. They are used only as leads and are marked "secondary" wherever they appear. | `logs/syspro_websearch_excerpts_20261008.log` |
| Roadmap's earlier reading of arXiv v1 (L141, L659–663) | Read. This is a project-internal secondary record. It states that §2.2, §3.1–3.2, §5 and §11 were compared directly, covering report-based location recovery, manual conditions for input generation, PIN instrumentation with sleep injection, and the data link. I cannot re-verify it this session. | roadmap |
| Five case reports (SysPro's input is the bug report; `CONDITIONS_POLICY.md` L7 defines a case's original report as the issue body **and its comments**) | **Issue bodies OPENED; comments NOT retrieved.** C1 body from `src/C1/retrieved_20261008/CF_184_body.md`. C3 (#73), C5 (#72) and R2 (#2663) are verbatim bodies from GitHub MCP `search_issues`. C2 (#198) is sentence-level quotes from `cases/C2.md` §1.1. Saved in `/home/user/work/procrace/baselines/syspro/inputs/`. The older `issues/*.json` files are 403 error bodies, not issue data. Comments exist but were not read: C1 has 5 (`cases/C1.md` L20), C3 has 14 (the migrated Trac #42 discussion, `cases/C3.md` L158), C5 has 9 (`cases/C5.md` L59), R2 has 2 (`cases/R2.md` L140). For C2 the count is not established: the logged-out page shows "No activity" (`cases/C2.md` L51), and `cases/C3.md` L159 shows that this display is unreliable. Every statement below about "the reports" covers **issue bodies only**. | `inputs/C1_CF184.md` L2 |
| Reference cFS source (bundle `088b2fa8`, osal `d2d877a6`, cfe `c5fb2b4d`, cf `15a871e6`, psp `c4b3b0b6`) | OPENED read-only for the §6 mapping | `logs/syspro_cfs_mapping_20261008.log`, `logs/syspro_review_recheck_20261008.log` §7 |

---

## 2. Artifact inventory (code, data, scripts)

**Nothing to inventory.** No file of the SysPro artifact was obtained, so its code, data, scripts, README and license are all unknown. `/home/user/work/procrace/baselines/syspro/` holds only my inputs and probes:

| Path | What |
|---|---|
| `inputs/{C1_CF184,C2_cFE198,C3_cFE73,C5_cFE72,R2_cFE2663}.md` | The five issue bodies (C2: sentence-level quotes), each with a provenance header. No comments. |
| `syscall_name_probe.py` | A stand-in probe that checks for literal Linux syscall-name tokens in the issue bodies (§4, §6). **Not SysPro, and not SysPro's first stage.** |
| `primitives.c` | A container probe that checks which OSAL-POSIX primitives used at the case anchors reach the kernel (§4, §6). **Not SysPro, not cFS.** |

Workspace size: 36 KB (02:24Z).

---

## 3. Documented requirements

Because the artifact and the paper could not be opened, **no requirement has `from-reference` status**. The "Reference" column says where each item comes from and how much that source is worth. "Secondary" means one of two things: the roadmap author's earlier reading of arXiv v1, or a web-search summary of an unopened page.

| ID | Requirement | Reference | Status of value | Met here? |
|---|---|---|---|---|
| S1 | SysPro code/data at `github.com/tarannumzaman/ReproSys/tree/main` | Roadmap L662 (records the link as given in arXiv v1). The JSS version's data-availability statement is unread (L668). | secondary (link only) | **No.** Not publicly readable (§1). |
| S2 | **Intel PIN** dynamic binary instrumentation, used to insert a sleep around selected system calls | Roadmap L661 ("PIN 계측·sleep 주입", from arXiv v1). Web-search summary, query 3: "instruments these system calls using PIN … by inserting a sleep function between each relevant system call pair". | secondary. **PIN version: unknown** (no excerpt names one). | **No.** PIN is not installed. The Intel download and documentation hosts are blocked (CONNECT 403). Without a documented version, no version may be chosen (policy). Which kernels a given PIN release supports cannot be checked either (Intel host blocked), so whether PIN supports Linux 6.18 is **unknown**. |
| S3 | **System-call description source** (content unknown) | Roadmap L141, the [출처 사실] row of §3.2 (L132: "이 표의 기능 설명은 [출처 사실]이다"), lists "시스템 호출 설명" (system-call descriptions) as an input. L159 ("Linux 호출 사전") sits in the §3.3 table, which L150 labels [분석]; L159 and L227 are project analysis and plan wording, not a reading of the paper. Web-search query 2: "No 'system call dictionary' found by that name"; a third-party summary says TF-IDF retrieval over man-page NAME sections (unverified). | secondary. Content and version unknown. | **Unknown.** The container has no `man2` pages (`/usr/share/man/man2` absent). `linux-libc-dev 6.8.0-106.106` headers exist, but using them would be my choice, not SysPro's. |
| S4 | **srcML** (source to XML, used to locate syscall call sites) | Web-search queries 3–4 (secondary) | version unknown | **No.** Not installed, and `www.srcml.org` is blocked |
| S5 | **NLTK**, **Gensim** (Python NLP/IR) | Web-search queries 3–4 (secondary) | versions unknown | **No.** Not installed (`ModuleNotFoundError`). `pypi.org` is reachable, but no version is documented, so nothing was installed. |
| S6 | **SPMF** (Java data-mining library) | Web-search queries 3–4 (secondary) | version unknown | **No.** OpenJDK 21.0.10 is present, the SPMF host is blocked, and no version is documented. |
| S7 | LLM or API use | No excerpt mentions an LLM. The described front end is keyword/IR/regex based. | unknown (absence is not verified) | n/a |
| S8 | Inputs: bug report text, target **source code**, **system-call descriptions**, and test inputs produced by IR, regex and the category-partition method. Manual test generation remains when the report is insufficient. | Roadmap L141 (arXiv v1 §2.2, §3.1–3.2, §5). Web-search query 1. | secondary | Report text: **issue body only** (C2: sentence-level quotes). Comments not retrieved (C1 5, C3 14, C5 9, R2 2; C2 count unknown; §1). Source: yes for C1, C3, C5 and R2 (public cFS/CF, but historical revisions still need task A). **No for C2**: the EVA CWS app is not public (`cases/C2.md` §1.1). System-call descriptions: see S3. |
| S9 | Failure oracle: the symptom described in the report | Roadmap §4.4 L228 (our plan); SysPro's own oracle form is unknown | unspecified_by_reference | n/a |
| S10 | Run budget (one unverified summary says 100 runs or 120 min) | Web-search query 2 (third-party, unverified) | unspecified_by_reference | n/a |
| S11 | Target OS / kernel / distribution of the original evaluation | Not found in any excerpt | unspecified_by_reference | n/a |
| S12 | Platform facts on record. **These are not PIN requirements**: no PIN version or documentation is available, so PIN's environment compatibility is **unknown** (S2). | — | — | ptrace works (`strace`, `gdb` OK). No Yama LSM. 4 vCPU Xeon (AVX-512). 16 GB RAM. Free disk 2.7 GB at 01:33Z and 2.4 GB at 02:24Z (not ~4 GB). `CONFIG_MODULES`, `CONFIG_KEXEC` and `CONFIG_KEXEC_FILE` unset; `kexec_load` ENOSYS. No `/dev/kvm`. **Assumption, not referenced** (general knowledge, not a SysPro or Intel document): PIN attaches through ptrace and needs no kernel module or reboot. If that holds, the kernel constraints that block RacePro would not apply to PIN. |

Bibliographic discrepancies, recorded but not resolved: the arXiv v1 and SSRN author lists (Zaman, Yan, Wang, Islam, Shi, Yu) differ from the JSS list (Zaman, Islam, Shi, Shi, Xian, Yu). This agrees with roadmap L144. Search summaries also report different benchmark counts (19 reports from 17 apps vs. 24 reports). No SysPro number is carried into this study (as roadmap L663 already says).

---

## 4. What I ran

All logs are under `/home/user/Shape-Perf/research/cfs_procrace/baselines/logs/`.

| Step | Commands | Log |
|---|---|---|
| Repository reachability | `GIT_TERMINAL_PROMPT=0 git ls-remote https://github.com/<owner>/<repo>` for 5 name variants, plus `tarannumzaman/Scminer` as a positive control. `curl` of the github.com pages, `codeload`, `api.github.com`, `raw.githubusercontent.com` (README on main/master/HEAD), and a raw positive control (`Scminer/master/.Rhistory` → 200). `gh api repos/...`, `users/...`, `search/...` (session-scoped 403s). GitHub MCP `get_file_contents`, `search_repositories` (`ReproSys`, `user:tarannumzaman`, `user:Tarannum-Zaman`, `SysPro concurrency bug reports`), `search_code`. `add_repo` (read). Software Heritage and Wayback. | `syspro_repo_probe_20261008.log` |
| Paper reachability (one attempt per host) | `curl -sS -L -o /dev/null -w '%{http_code}' --max-time 30 <url>` for the arXiv, DOI, Crossref, ScienceDirect, SSRN, researchr, emergentmind, uknowledge and NSF PAR URLs. WebFetch for arXiv, web3.arxiv, ScienceDirect, SSRN, uknowledge and emergentmind. Scholar Gateway. Proxy `__agentproxy/status`. One ScienceDirect URL in the first round was a **guessed placeholder PII**. It is marked in the log as carrying no information; the real PII came later from search. | `syspro_paper_fetch_20261008.log` |
| Web search (4 queries) | `WebSearch` standard ×1 and extended ×3. The summaries were saved verbatim in condensed form and marked secondary. | `syspro_websearch_excerpts_20261008.log` |
| Container facts | `uname -a`, `/etc/os-release`, `nproc`, cpuinfo, `free -m`, `df -h`, `/proc/modules`, CapEff decode, `/proc/config.gz` `CONFIG_MODULES`, Yama, `strace -e trace=execve /bin/true`, `gdb -batch -ex run /bin/true`, `kexec_load` probe (`syscall(246,0,0,0,0)` → ENOSYS), `/dev/kvm`, `which` for the tool chain, `import nltk/gensim`, PIN presence, `java -version`, man2 presence, package versions. Reachability only (no download, no version choice) for Intel PIN, srcML, SPMF and PyPI. | `syspro_container_facts_20261008.log` |
| Issue-body syscall-name token probe (**stand-in, not SysPro, not SysPro's stage 1**) | `python3 -I /home/user/work/procrace/baselines/syspro/syscall_name_probe.py /usr/include/x86_64-linux-gnu/asm/unistd_64.h inputs/*.md`. It intersects issue-body tokens with the 373 x86_64 syscall names from the container's `linux-libc-dev 6.8.0-106.106`. Two choices are mine and `unspecified_by_reference`: (1) that name list; (2) the tokenizer, which splits on non-alphanumerics and on `_` only. A camelCase split would also yield `pipe` for C3 and R2, from `CFE_SB_CreatePipe` (`syspro_review_recheck_20261008.log` §4). | `syspro_report_syscall_probe_20261008.log` |
| Syscall-surface probe (**container probe, not SysPro, not cFS**) | `gcc -O0 -pthread primitives.c` (gcc 13.3, glibc 2.39-0ubuntu8.9), then `strace -f -e trace=getpid,futex,clone,clone3,mq_open,mq_unlink,… ./primitives`, with `getpid()` as markers between `sem_init`, an uncontended `pthread_mutex_lock` plus name-table search, a memory publication, `pthread_create` and `mq_open`. That trace was **filtered**, so by itself it cannot show "no syscall at all" between markers. The review re-ran it with an **unfiltered** `strace -f`: markers 1–4 are adjacent, and `getrandom`, `brk` and `rt_sigaction` appear only in the `pthread_create` window. | `syspro_syscall_surface_probe_20261008.log`; unfiltered: `syspro_review_recheck_20261008.log` §5 |
| cFS anchors (read-only) | greps over `/home/user/work/procrace/cfs_ref/cFS` at the revisions above | `syspro_cfs_mapping_20261008.log` |
| Review re-check (02:24Z) | `df -h /`; `/proc/config.gz` MODULES/KEXEC/KEXEC_FILE; curl reachability of 10 more hosts; tokenizer sensitivity of the token probe; unfiltered strace of `primitives.c`; strace of a small probe that calls `pthread_setaffinity_np`, `pthread_setname_np`, `clock_nanosleep` and `sigaction` as the PSP/OSAL do (→ `sched_setaffinity`, `prctl(PR_SET_NAME)`, `clock_nanosleep`, `rt_sigaction` on this glibc); source greps for the C3/R2/C5 anchors in §6 | `syspro_review_recheck_20261008.log` |

**Not done, on purpose:**
- I did not install PIN, srcML, NLTK, Gensim or SPMF. No version is documented that I could open, and installing a version of my own choosing would be a non-reference choice.
- I did not re-implement SysPro (that is D3).
- I did not run cFS under `strace` or PIN. That would not be a SysPro result. It could also collide on POSIX mqueue names with the ENV task's cFS runs on the same host. The small probes answer the syscall-surface question without starting cFS. The cFS-level syscalls in §6 are therefore source-level, with the glibc mapping from the probes.

---

## 5. Observation model (secondary sources only; see D2)

Assembled from the roadmap's earlier reading of arXiv v1 (L141, L661) and the web-search summaries. **Not verified against the paper.**

1. **Front end (report → syscalls → code sites).** Extract the relevant system calls from the natural-language report (search summary, query 1: "extract relevant system call names from the report"). The roadmap's source-fact row lists system-call descriptions as an input (L141), and one unverified third-party summary says TF-IDF retrieval over man-page NAME sections. Such retrieval matches descriptive words, not only names. No source I could open says that syscall names must appear literally in the report. Then locate the call sites of those syscalls in the target's source code (srcML XML, structured search).
2. **Input generation.** Use IR, regular-expression matching and category-partition to build program inputs from the report. If the report is insufficient, manual test generation remains (roadmap L141).
3. **Reproduction (observation and control).** Run the program under **PIN**. At the selected syscalls (secondary sources: "between each relevant system call pair", "hooks on both sides of each relevant call"), insert a **sleep**. Try candidates in ranked order and move to the next one if the failure does not recur.
4. **What it observes.** At most, per secondary sources, syscall entry and exit events of the instrumented threads, plus the program's failure output. Nothing in any source indicates that it observes **memory accesses**, user-space object tables or happens-before relations. Per unverified secondary summaries, it iterates over ranked candidate syscall pairs and forces one order per run with sleeps, under a run budget (S10: 100 runs or 120 min, unverified). Whether that counts as a search over schedules is unverified. Roadmap §4.4 L229 ("지연 주입은 발생 기회를 늘리는 방법이다 … 부르지 않는다") is this project's terminology rule, not a statement about SysPro.
5. **Execution unit in the original benchmark.** Multi-process Linux utilities (search summaries mention coreutils, findutils, bash, gzip, bzip2, mv, rm, mkdir; unverified). Whether the tool handles several threads in one process is not documented in anything I could open. PIN itself instruments all threads of a process (general PIN capability, not a SysPro claim).

---

## 6. The five cFS cases: what SysPro's model could see or do in principle (analysis, NOT a run)

Common facts:
- Native cFS is **one Linux process**, and every OSAL task is a `pthread_create` thread (`osal/src/os/posix/src/os-impl-tasks.c:568`).
- The container probe shows that **`sem_init`, an uncontended mutex-guarded name lookup, and a plain memory publication reach no syscall at all**, while `pthread_create` → `clone3` and `mq_open` → `mq_open` do (`syspro_syscall_surface_probe_20261008.log`; confirmed with an unfiltered trace in `syspro_review_recheck_20261008.log` §5). A syscall-level observer therefore sees none of the dependencies of C1, C3, C5 and R2 directly. For C2, where the dependency lives is unknown, because the target source is not public (`cases/C2.md` §1.1). SysPro could only *perturb* timing at a syscall that happens to sit next to the dependency, and then rely on the report's failure symptom as the oracle.
- **Literal syscall-name tokens in the issue bodies (stand-in probe, not SysPro's stage 1):** no issue body uses a Linux syscall name as a syscall. The hits are English words or parts of cFE API names, listed per case below. No body has `name(` call syntax for any syscall, and every function a body names is a cFE/OSAL/CF API. The hits depend on the tokenizer (`_` split only; a camelCase split adds `pipe` for C3 and R2). **SysPro's stage-1 output on these reports is unknown** until the paper or the artifact is read (§5 item 1). Comments were not retrieved (§1, S8).
- New cFE core-app threads make syscalls of their own before the app's main function runs. These come from source reading at the reference revision; the glibc mapping is from the probe in `syspro_review_recheck_20261008.log` §6. In order: (1) `OS_TaskEntryPoint` → `OS_TaskPrepare` → `OS_NotifyEvent(OS_EVENT_TASK_STARTUP)` (`osapi-task.c:128`, `:103`) runs the PSP handler in the new task (`cfe_psp_start.c:170–201`, registered at L361). That handler calls `pthread_setaffinity_np` for `CFE_*` task names (L184–188; → `sched_setaffinity`) and `pthread_setname_np` (L201; → `prctl(PR_SET_NAME)`, as also in `osal-visibility.md` §1). (2) `CFE_ES_TaskEntryPoint` → `CFE_ES_GetTaskFunction` calls `OS_TaskDelay` at least once (`cfe_es_apps.c:575`, called at L625; → `clock_nanosleep`, `os-impl-tasks.c:761`). (3) `CFE_PSP_SetDefaultExceptionEnvironment` (`cfe_es_apps.c:633`; → `sigaction`, `cfe_psp_exception.c:179`; → `rt_sigaction`). (4) Only then does `(*RealEntryFunc)()` run (L639). Core apps start through `CFE_ES_StartAppTask` (`cfe_es_start.c:810`), and the SB task is named `CFE_SB` (`cfe_sb_objtab.c:33`). This list is not exhaustive.

Revisions: anchors are cited at the current reference cFS. C1, C3 and C5 are already fixed there, so a real run would need the historical revisions that roadmap task A is pinning.

| Case | Literal syscall-name tokens in the issue body (probe; `_` split) | Dependency (where it lives) | Nearest syscall anchor | Observe the dependency? | Could a SysPro-style delay realize the bad order? |
|---|---|---|---|---|---|
| C1 (CF#184) | `bind` ("bind to that semaphore"), `exit`, `sync` ("sync mechanisms"), `time` ("same time"). None is a syscall use. | Provider app's `OS_CountSemCreate` → `sem_init` (`os-impl-countsem.c:85`; **no syscall**) and name registration in the OSAL table, versus CF's `OS_CountSemGetIdByName` (`cf_cfdp.c:1282`) → `OS_ObjectIdFindByName` (`osapi-idmap.c:975`) under a pthread mutex (`os-impl-idmap.c:92`; futex only under contention). | Any syscall in the provider's init *before* sem creation. The report leaves the provider unidentified ("CI/TO or some other dedicated I/O app"). `cases/C1.md` L240 identifies a public provider: nasa/bp, `fsw/custom/bp_semcfg.c` L43–49 @`b051eb0` (`BP0_tsem`, `BP1_tsem`). | **No** | Only with a hand-picked anchor in a hand-picked provider. The report already gives the human recipe ("Add an artificial delay during startup for the app that creates the sem"). Whether SysPro's front end would derive such an anchor from the text is unknown. |
| C2 (cFE#198) | `sync` ("start up sync") | Late-init readiness inside a non-public EVA CWS app. Where the dependency lives is unknown. | Unknown (no source) | Unknown | **Not applicable.** A required input, the target source (S8), does not exist publicly. |
| C3 (cFE#73) | `time` (the TIME core app). A camelCase split adds `pipe` (`CFE_SB_CreatePipe`). | `CFE_SB_Global.AppId` set by `CFE_ES_GetAppID` (`cfe_sb_task.c:138`, memory), and EVS's AppId initial value, versus TIME's `CFE_SB_CreatePipe` (`cfe_time_task.c:186`) | *Using side:* TIME's `CreatePipe` → `mq_open` (`os-impl-queues.c:116`) is in the using thread, so a delay there pushes the wrong way. *Publisher side:* SB's `AppInit` itself has no syscall before L138, but the SB thread makes syscalls earlier, at the reference revision: `prctl(PR_SET_NAME)` and `sched_setaffinity` (PSP task-startup handler), `clock_nanosleep` (`GetTaskFunction` L575) and `rt_sigaction` (L633), all before `CFE_SB_TaskMain` → `AppInit` (`cfe_sb_task.c:70–77`). A delay at any of them pushes toward the bad order. | **No** | Only with a hand-picked anchor that delays SB before its AppId store (for example one of the publisher-side syscalls). The original crash also depends on Microblaze priorities and on the 6.4.x EVS initial value (historical revision). On native Linux as a normal user, priorities are "best effort" (cFS README L102). |
| C5 (cFE#72) | `read` ("read the shared table data") | ES `AppTable`/`TaskTable` entries written by the creator after `OS_TaskCreate` (`cfe_es_apps.c:664`, relock at L672; memory), versus the child's early `CFE_ES_GetAppID` | `clone3` from `OS_TaskCreate` → `pthread_create` in the creator thread (`os-impl-tasks.c:568`) | **No** (the table write is memory) | **At the reference OSAL, `clone3` is not a usable anchor.** The new task record stays RESERVED from `OS_ObjectIdAllocateNew` (`osapi-task.c:187`; `osapi-idmap.c:438`, `:513`) through `OS_TaskCreate_Impl` (L204) until `OS_ObjectIdFinalizeNew` (L207). The child's `OS_TaskPrepare` takes the record in GLOBAL lock mode (`osapi-task.c:84`) and waits while it is RESERVED (`osapi-idmap.c:460–485`). A delay right after `clone3` therefore holds the child back. If the delay exceeds about 300 ms (4 timed waits of 10, 40, 90 and 160 ms, `os-impl-idmap.c:156`; less if an unrelated broadcast ends a wait early), the fifth attempt returns `OS_ERR_OBJECT_IN_USE` (`osapi-idmap.c:476–478`) and the child never runs its entry (`osapi-task.c:128`). The useful window opens when `FinalizeNew` releases the record and closes at the relock at `cfe_es_apps.c:672`. In it, the creator's only possible syscall is a futex wake from the unlock's `pthread_cond_broadcast`/`pthread_mutex_unlock` (`os-impl-idmap.c:116`, `:123`), and only when the child is waiting. At the reference revision the child also waits in `CFE_ES_GetTaskFunction` (L559, L625). Whether `clone3` is a usable anchor at the historical OSAL/cFE revision that cFE#72 needs is **unexamined** (task A). The issue body names `OS_TaskCreate`, `CFE_ES_UnlockSharedData` and `CFE_ES_GetAppID`, never `clone`/`fork`/`pthread_create`. |
| R2 (cFE#2663) | none. A camelCase split gives `pipe` (`CFE_SB_CreatePipe`). | SB's `CFE_EVS_Register` (`cfe_sb_task.c:155`, memory state in EVS), versus ES/EVS calling `CFE_SB_CreatePipe` (`cfe_es_task.c:367`, `cfe_evs_task.c:289`) | *Using side:* `mq_open` inside those `CreatePipe` calls. *Publisher side:* the SB-thread syscalls before `AppInit` listed for C3 (`prctl`, `sched_setaffinity`, `clock_nanosleep` from L575, `rt_sigaction`); none between L138 and L155. | **No** | Needs a hand-picked anchor, if any. Probably no ordering perturbation is needed. The report reproduces it with a breakpoint on a default native run, so the order looks deterministic. If task A confirms that, roadmap §4.7 L266 excludes R2 from the race-positive denominator. |

**Analytical conclusion (label it "analysis", not "result"):** per secondary sources, SysPro's model is report → relevant syscalls → call sites → sleep around those syscalls. For the five cases:
1. Every issue body states the dependency in cFE/OSAL API terms. What SysPro's first stage would extract from them is unknown (D2). The token probe covers literal names only.
2. In C1, C3, C5 and R2, the publication itself is a memory store with no syscall (C2: unknown). A syscall-level delay can act only at a nearby syscall: one in the provider's init (C1), the publisher thread's startup syscalls (C3, R2), or the creator's `clone3` (C5). Each is a hand-picked anchor, and whether SysPro's front end would select it is unknown.
3. In C5, `clone3` is never named in the issue body. At the reference OSAL, a delay after it blocks or aborts the child. The historical revision is unexamined.

Whether the original front end could serve cFS cannot be judged without the paper. Roadmap §4.4 L227 plans to replace the Linux-syscall front end with cFS/OSAL descriptions as this study's extension. That would be a re-implementation (D3), not the original tool.

---

## 7. Earlier partial attempt (2026-10-07): did it follow the documentation?

| Item | What it is | Followed docs? | Reuse for SysPro? |
|---|---|---|---|
| `/home/user/work/procrace/baselines/syspro/` (created 2026-10-07 07:19) | **Empty directory.** No SysPro step was taken. | n/a | Nothing to reuse. Today's inputs and probes now live there. |
| `racepro_libscribe_build.log`, `racepro_libscribe_build_m32.log` | libscribe builds on x86_64, the second with an undocumented `-m32` | **No** (RacePro task, see `racepro.md` §7) | Not relevant to SysPro |
| `racepro_scribe_runtime.log` | `record /bin/true` with no Scribe kernel. Its `grep -c scribe /proc/kallsyms` = 26 is a substring artifact. | **No** | Not relevant |
| `apt_gcc_multilib.log` | `apt-get install gcc-multilib`, which **upgraded glibc 2.39-0ubuntu8.7 → 8.9** | **No** (not in any document) | Indirectly relevant: my `primitives.c` probe ran on glibc **2.39-0ubuntu8.9**. Its conclusion (`sem_init` is user-space only; `pthread_create` → `clone3`) does not depend on that patch level, but the provenance is recorded. |
| `container_kernel_caps.log` | `CONFIG_MODULES` unset, `finit_module` ENOSYS, CAP_SYS_MODULE in CapEff, ptrace status | Factual | Yes, as facts. It agrees with today's re-check on kernel `fc-v80`. The log itself has no kernel tag; it was taken on `fc-v77`, inferred from `racepro_scribe_runtime.log` (`# uname -r: 6.18.44-fc-v77`), written 17 s earlier (mtimes 07:22:03 and 07:22:20). For SysPro, the relevant facts are that ptrace works and that modules and kexec are unavailable. Whether PIN needs anything more is unknown (S12). |
| `racepro_emulation_feasibility.log` | QEMU dry-run; "Ubuntu 10.10" inferred, not referenced | Exploratory; `unspecified_by_reference` | Not relevant |
| `yu-group_source_fetch_20261008.log` and `yu-group_github_candidates_20261008.log` (ReproSys `ls-remote`) | Same credential prompt as today | Factual | Agrees with §1 |

---

## 8. Verdict

**`material_unavailable`.** Evidence:

1. The only artifact location known, `github.com/tarannumzaman/ReproSys` (arXiv v1, roadmap L662; the JSS version's data-availability statement is unread, L668), is not publicly readable. Anonymous `git ls-remote` asks for credentials while the same owner's public repo answers. Raw-file fetch gives 404 against a 200 positive control. The owner's public repo listing contains only `Scminer`. `add_repo` reports not found or no access (`syspro_repo_probe_20261008.log`). Other artifact indexes (Zenodo, figshare, Semantic Scholar, OpenAlex, scholar.archive.org) are blocked (`syspro_review_recheck_20261008.log` §3).
2. The paper (arXiv v1, SSRN, JSS) cannot be opened from this session (`syspro_paper_fetch_20261008.log`). Its documented requirements, including the **PIN version**, the system-call description source and the tool-chain versions, are therefore unknown. They cannot be set without making non-reference choices.
3. Even with D1 and D2 resolved, PIN, srcML and SPMF would need egress that is currently blocked (D4).
4. The environment compatibility of PIN is **unknown**: no PIN version or documentation is available (S2; Intel hosts CONNECT 403). Facts on record: ptrace works; `CONFIG_MODULES`, `CONFIG_KEXEC` and `CONFIG_KEXEC_FILE` are unset; there is no `/dev/kvm`. That PIN needs no kernel module or reboot, which would set it apart from RacePro, is an assumption (S12), not a referenced fact.

Entries for the RacePro/SimRacer-style §4.7 table do not apply. In the separate RRF/DESCRY/SysPro reproduction table (roadmap L282), SysPro gets C1, C2, C3, C5, R2 = `NOT_RUN — 자료 부족 (artifact and paper unavailable; repo probe and paper-fetch logs)`. Keep the §6 analysis in its own column, labelled "observation-model analysis (not run; secondary-source model)".

---

## 9. Review revisions (2026-10-08, 02:24–02:30 UTC)

Evidence for every item is in `logs/syspro_review_recheck_20261008.log` unless another source is named.

1. The stage-1 claims based on the token probe were withdrawn ("no anchor in four of the five cases", "the original syscall-name front end would find no syscall names"). SysPro's stage-1 output is now recorded as unknown. The probe is now described as checking literal tokens in the issue bodies, with its tokenizer recorded as a second `unspecified_by_reference` choice (§4). "System-call descriptions" (roadmap L141) was added to S8. The D3 default now rests on `material_unavailable`.
2. Inputs are recorded as issue bodies only, with the comment counts that were not retrieved (§1, S8).
3. C3/R2: the publisher-thread syscalls before `cfe_sb_task.c:138`/`:155` were added. Conclusion point 2 now says that the publication itself is a memory store, and "the only case" was dropped from C5.
4. C5: at the reference OSAL, a delay after `clone3` blocks or aborts the child. This was removed as the basis of the Recommendation.
5. §5 item 4: the "does not search schedules" claim is now attributed correctly. Roadmap L229 is the project's terminology rule.
6. S12 and §8 item 4: "the environment is not what blocks PIN" was replaced by "unknown", with the RacePro contrast labelled as an assumption.
7. S3: the reference is now L141 (source fact); L159 and L227 are marked as project analysis and plan wording.
8. D1, S1 and §8 item 1: "only documented location" became "only location known". The JSS data-availability statement is marked unread, and the extra blocked hosts were added (§1; addendum in `syspro_paper_fetch_20261008.log`).
9. §6: the claim that the dependency lives in memory is restricted to C1, C3, C5 and R2; for C2 it is unknown. In C1, the provider is identified from `cases/C1.md` L240.
10. The filtered syscall-surface trace was confirmed with an unfiltered one (§4).
11. The disk figure now carries its timestamp, and the `fc-v77` provenance of `container_kernel_caps.log` is marked as inferred (S12, §7).

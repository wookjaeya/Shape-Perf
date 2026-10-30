# OSAL syscall visibility probe (osal-visibility)

Date: 2026-10-08 (UTC). Policy: `../CONDITIONS_POLICY.md`. Logs: `logs/osal-visibility_*.log`. Work directory: `/home/user/work/procrace/baselines/osal-visibility/` (69 MB).

Revised 2026-10-08 after review. Changes: the name-bearing count is now scoped to the `-s 65535` runs; R5 (mqueue sysctls, CI link) and R11 (trace command) are re-sourced; the observer scope now excludes memory-page recorders; D1, D4 and H-OSAL-1 citations are corrected; container facts now include kexec and VM; the syspro provenance is fixed; counts are made precise. Evidence for the revision: `logs/osal-visibility_review_recheck_20261008.log`.

**What kind of result this is.** This is a probe, not a baseline-tool result. It checks what a syscall-level observer can see when OSAL and cFE create, register or look up named objects. Here "syscall-level observer" means an observer whose events are syscalls or syscall effects on kernel objects: strace, ptrace or seccomp; RacePro's race-candidate model (`racepro.md` §5.2); DESCRY's relations. A recorder that also logs memory page ownership (Scribe, `racepro.md` §5.1) may record anonymous, page-granular accesses to the in-memory tables described below. That was not measured here. No process-race tool was run here. Nothing below says that any tool "fails to detect" anything.

## 0. Decisions needed from the PI

| # | Question | Why it needs you | Options |
|---|---|---|---|
| D1 | Should the reference cFS build (`core-cpu1`, ENV) be run once under `strace -f`? It would confirm three things at the cFS level: (a) the PSP's per-task `prctl(PR_SET_NAME, <task name>)` (§6.4); (b) that CF's `OS_CountSemGetIdByName` and the ES record writes issue no syscalls; (c) that each SB pipe shows up as an `mq_open("<pid>.<pipe name>")`. | Each extra cFS run changes the PSP reset state. Since this report was first written (01:59), ENV has run run03-run07 (02:09-02:11). The reset-type sequence is now run01 PO → run02 PR → run03 PR, which hit "Maximum Processor Reset count reached (2)" and exited with POWERON → run04 PO → SIGINT → run05 PR → ES restart → run06 PO → ES restart → run07 PO → ES restart (`env/ENV.md` L189; `env/logs/run03_audit_operational.log` L229-230). ENV says ordering experiments need a fixed reset type (E7) and stop method (E11) (`env/ENV.md` L213). Both are still open (L18). Today (b) and (c) rest on OSAL tests plus source reading, and (a) rests on source reading plus a glibc probe. | (i) Run it after E7 is decided, using ENV's documented run procedure. (ii) Do not run it; keep the source-level statements and mark them as such. |
| D2 | Can this probe count as the "event observation" evidence for syscall-based baselines (RacePro's kernel-object model, DESCRY and SysPro's syscall orders) in the §4.7 diagnosis table? | This is evidence about the observation model, not a tool run. It may be cited only as "not observable at syscall level", never as a tool missing a case. For RacePro it covers the race-candidate model (`racepro.md` §5.2), not Scribe's recording. Scribe also logs memory page ownership between threads (`racepro.md` §5.1), and this probe did not measure that. | Accept with that label, or require each tool's own run first. |
| D3 | Should hypothesis **H-OSAL-1** (§6.5) be pursued? Source reading suggests that a by-name lookup that hits an object still being created gets `OS_ERR_INCORRECT_OBJ_STATE`, and that the count-semaphore table lock is then never released, so the creator blocks for good. This bears directly on C1. | It was not executed. Checking it needs a controlled interleaving. No OSAL test exercises it, so the policy's "own test only if none exists" clause applies. | (i) Check it with gdb breakpoints on OSAL's own `count-sem-test` (no new code). (ii) Write a minimal test against the documented API. (iii) Defer, and record it only in the C1 notes. |
| D4 | Revision. These results are for OSAL `d2d877a` (cFS `main` / `v7.0.1`). The local C1 cFS checkout (`src/C1/cFS`, `01e8416`) has the OSAL gitlink `dad0ee9` (`dev`). The C1 case's own OSAL is a third revision: `42af0f73` (v6.0.0-rc4, pinned by cFS caelum-rc4; `cases/C1.yaml` L35-37, L91-96). The H-OSAL-1 hypothesis in C1.yaml L96 is stated at that revision. | This is tied to the open ENV A2 (main vs dev). On the inspected paths, `dev` changes only a refactor in `OS_ObjectIdFindNextFree` and adds a core-count read at init. Neither `dev` nor `42af0f73` was executed. Read-only `git show` at `42af0f73` confirms the same no-unlock path for H-OSAL-1 (§6.5). | Keep main, re-run the probe on dev, or re-run it on `42af0f73`. |
| D5 | CI mqueue sysctls (R5): `fs.mqueue.msg_max=512`, `queues_max=512` in OSAL's CI, against 10 and 256 here. | Root here appears able to raise them (R5), but that changes shared container state and the README build does not require it. This is the same open question as ENV E5 (`env/ENV.md` L18: README `msg_max` 10 vs the bundle CI's 64). | Decide together with E5. Until then: not applied. |

## 1. Summary and verdict

- **Verdict: `probe_result`.** OSAL was built exactly as its README says (L24-30). Its own functional test `src/tests/count-sem-test` passed 81/81 under `strace -f -e trace=all`, the base command given by the orchestrating task (R11; not a policy reference). It passed again in 11 stack-annotated strace runs.
- **Count semaphore create and lookup issued no syscalls at all**, not even the expected futex or clock calls:
  - `OS_CountSemCreate(&id, "Test_Sem", 0, 0)` (count-sem-test.c:105), the duplicate create at :106, and the create at :109.
  - `OS_CountSemGetIdByName(&id, "Test_Sem")`: four lookups in three task functions (:35, :49, :63). `Task_0` is started twice (count-sem-test.c L160-162 and L178-180), so :35 runs twice (`run1_exact.strace` L328 and L399).
  - The result held in all 12 traces. run1 supports it through line-number markers, which survive strace's default string truncation (§6.2).
- **The name never appears in a syscall made by OSAL.** In each of the 11 runs with `-s 65535`, exactly 13 syscalls carry an object name (`Test_Sem` or `Task_N`). All 13 are `write(1, …)` from the test harness's own assertion report (`UtAssert_DoReport` → `OS_BSP_ConsoleOutput_Impl`). In run1 (base command, default `-s 32`) the write buffers are truncated at 32 bytes, so 0 matches appear. run1 supports only the window analysis by line-number markers, not any claim about names in long arguments.
- **Where the name and the registry live.** The name→object binding is only in process memory: `OS_common_table[].name_entry` and `OS_count_sem_table[].obj_name`. The semaphore itself is an unnamed, process-private `sem_t` (`sem_init(…, 0, …)`). At syscall level it appears only as a futex word address (an OSAL table slot), and only when a Take blocks or a Give wakes a waiter.
- **Contrast in OSAL's own `osal-core-test`:**
  - OSAL **queues** are named kernel objects. Each `OS_QueueCreate` issues `mq_open("<pid>.<name>")` and then `mq_unlink`. cFE SB pipes are OSAL queues (`cfe_sb_api.c` L163).
  - `OS_QueueGetIdByName` and `OS_TaskGetIdByName` still issued no syscalls.
  - A wait on OSAL registry state can surface, but only as futex wait/wake on the task-table condvar address. This happened in one run: a new task's record was still RESERVED.
- **cFE ES app/task registration** writes only to `CFE_ES_Global` tables, under an OSAL mutex (a PI pthread mutex). The source shows no syscall carries the record or its state.
- **Task names do reach the kernel in a real cFS run, through the PSP rather than OSAL.** The pc-linux PSP's `OS_EVENT_TASK_STARTUP` handler calls `pthread_setname_np(pthread_self(), name)`. On this host's glibc 2.39 that issues `prctl(PR_SET_NAME, "<name>")` (probe log). This happens after OSAL has registered the task. It is not ordered with respect to the ES task record write in the creating task.

## 2. Commits

Log: `logs/osal-visibility_commit_resolution_20261008.log`, `logs/osal-visibility_osal_clone_20261008.log`.

| Item | Value | How resolved |
|---|---|---|
| nasa/cFS `main` | `088b2fa828db9ff7e00733f1908e0eeb59f66ce3` (= tag `v7.0.1`, 2026-05-13) | `git ls-remote` at 2026-10-08T01:38Z. Remote `HEAD`/`dev` is `01e8416c…`, which is a different commit (ENV A2). |
| osal gitlink in cFS `main` | `d2d877a69cff47452bcca274b309147d48e6c16f` | Fresh blob-less clone of cFS `main`: `git ls-tree HEAD osal`. It is the same in the ENV clone. `.gitmodules` L4-6: `url = https://github.com/nasa/osal.git`. |
| nasa/osal clone used | `d2d877a…` = tag `v7.0.1` = osal `main`, 2026-05-12, "Merge pull request #1538 from nasa/dev". Clean tree, no nested submodules. | `git clone https://github.com/nasa/osal.git`, then `git checkout --detach d2d877a…`. |
| cFE / PSP read for the ES and PSP parts | cFE `c5fb2b4d540bd55eb6c3707da7dd13eee679d4dd`, PSP `c4b3b0b65b119e106481ad8e20976ae4d7f554e3` | Bundle gitlinks at `088b2fa`. Read in the ENV clone; nothing was modified. |

The `gh api` calls for the gitlink were refused with "GitHub access … not enabled" (HTTP 403). They were replaced by the clone check above.

## 3. Sources opened, and not opened

**Opened (OSAL `d2d877a`).** Verbatim excerpts are in `logs/osal-visibility_source_citations_20261008.log`.

- `README.md` (whole file).
- `docs/OSAL-Configuration-Guide.md`: L61-100, L130-176, L196-272, L643-750.
- `default_config.cmake` L160-190.
- `.github/workflows/standalone-build.yml` (whole file).
- `src/os/shared/src/osapi-countsem.c`, `osapi-idmap.c` and `osapi-task.c` (relevant functions).
- `src/os/shared/inc/os-shared-idmap.h` and `os-shared-countsem.h`.
- `src/os/posix/src/os-impl-countsem.c`, `os-impl-idmap.c`, `os-impl-tasks.c` and `os-impl-queues.c` (create/delete).
- `src/os/posix/inc/os-impl-countsem.h`, `os-impl-console.h`.
- `src/os/inc/osapi-idmap.h` (ID encoding).
- `src/tests/count-sem-test/count-sem-test.c`, `src/tests/osal-core-test/osal-core-test.c` (relevant parts), `src/tests/queue-test/queue-test.c` (grep only).
- `CMakeLists.txt` L405-425.

**Opened (cFE `c5fb2b4`, PSP `c4b3b0b`):**

- `modules/es/fsw/src/cfe_es_apps.c` L555-900.
- `cfe_es_api.c`: L738-766, L2075-2117.
- `cfe_es_resource.c`: L80-97, L277-300.
- `cfe_es_resource.h`: L204-207, L453-456.
- `modules/sb/fsw/src/cfe_sb_task.c` L138, `cfe_sb_api.c` L163.
- `modules/evs/fsw/src/cfe_evs_task.c` L312.
- PSP `fsw/pc-linux/src/cfe_psp_start.c`: L83, L147-202, L361.
- `sample_defs/default_osconfig.cmake` L28-35.

**Also opened:**

- Project files:
  - Policy, roadmap §2-§4 and §13.
  - `env/ENV.md`, plus the run01 log, line 42 only. In the revision: `env/ENV.md` L18, L32, L189, L213 and `env/logs/run03_audit_operational.log` L26, L229-230.
  - `cases/C1.yaml` (grep; in the revision, L35-37 and L91-96).
  - `baselines/racepro.md` §7. In the revision: §5.1-§5.2, `syspro.md` §7 and `descry.md` (grep for relations).
  - In the revision: `tool/DESIGN.md` (grep for `strace`), and OSAL `42af0f73:src/os/shared/src/osapi-idmap.c` via read-only `git show` in `src/C1/osal`.
- Earlier-attempt logs: listed in §10.
- SysPro task files: `logs/syspro_syscall_surface_probe_20261008.log` and `work/.../syspro/primitives.c`, to check for overlap.

**Not opened:**

- The OSAL API guide PDF (`cFS/gh-pages/osal-apiguide.pdf`, README L10/L35). The build steps came from README and the Configuration Guide.
- The CI container image `ghcr.io/core-flight-system/cfsbuildenv-linux:latest`: it was not pulled.
- `.travis.yml`, which README L24 links as `blob/master/.travis.yml`. The link is dead. nasa/osal has no `master` branch (`git ls-remote`: `main`, `dev` and others, no `refs/heads/master`), and the raw URL `raw.githubusercontent.com/nasa/osal/master/.travis.yml` returns 404. (The github.com page itself returned 403 from the agent proxy, so it is not used as evidence.) `.travis.yml` was deleted from OSAL in `72da4f28` (2021-02-03, "Fix #771, Add workflow timeout and format check"). See R5 for how the GitHub workflow is used instead.
- glibc sources. The `pthread_setname_np` → `prctl` behaviour was checked empirically instead (§5).
- `src/unit-tests/oscore-test/ut_oscore_countsem_test.c`: grep only. It was not used as the strace target.
- `src/examples/tasking-example`.

## 4. Documented requirements

| # | Requirement | Reference (quote) | Met here? |
|---|---|---|---|
| R1 | Use the OSAL commit pinned by the cFS main bundle | Task text; cFS `.gitmodules` L4-6; gitlink | **Yes**, `d2d877a` (§2) |
| R2 | CMake ≥ 3.5 | Config Guide L205-206: "OSAL requires at least version 3.5 of the cmake tool." | **Yes**, cmake 3.28.3 |
| R3 | Standalone build with tests, run from the base osal directory | README L24-30: `mkdir build_osal_test` / `cd build_osal_test` / `cmake -DENABLE_UNIT_TESTS=true -DOSAL_SYSTEM_BSPTYPE=generic-linux -DOSAL_CONFIG_DEBUG_PERMISSIVE_MODE=TRUE ..` / `make` / `make test`. README L16 "(from the base osal directory)" heads the first example, the library build at L16-22, not this one. For the test build at L24-30 the base directory is implied by `cmake … ..` run inside `build_osal_test`. | **Yes, verbatim** (`from-reference`). Serial `make`, because README uses no `-j`. |
| R4 | Alternative documented standalone build | Config Guide L245-246: "preferably outside the OSAL source tree". L253-262 add `-DCMAKE_BUILD_TYPE=debug`. | **Not used.** The README form was followed, as the task names README first. The only effective difference is `-g`: the README build has empty `C_FLAGS` (`flags.make`), so both are `-O0`. The two documents do not say which one governs (`unspecified_by_reference`). |
| R5 | CI environment that README L24 points to ("see also CI") | README L24 links `blob/master/.travis.yml`. nasa/osal has no `master` branch and the raw URL returns 404. `.travis.yml` was deleted in `72da4f28` (2021-02-03, "Fix #771, Add workflow timeout and format check") (§3). The GitHub workflow `.github/workflows/standalone-build.yml` is used as the successor CI **by inference from that commit**, not because README points to it. Its L27-71: `ubuntu-22.04`, container `cfsbuildenv-linux:latest`, L30 `--sysctl fs.mqueue.queues_max=512 --sysctl fs.mqueue.msg_max=512`, `-DCMAKE_BUILD_TYPE=Debug -DOSAL_OMIT_DEPRECATED=FALSE -DOSAL_VALIDATE_API=FALSE -DOSAL_INSTALL_LIBRARIES=FALSE`, `make -j2`, `ctest -j4` | **Not met and not attempted.** Here `msg_max` = 10 and `queues_max` = 256, against 512 and 512 in the CI. Raising the sysctls as root appears possible: `/proc` is mounted `rw`, `msg_max` is a root-owned `0644` file, and `access(W_OK)` returns True for root and False for `ubuntu`. OSAL's own error text tells users to "check the msg_max parameter located in /proc/sys/fs/mqueue/msg_max … and raise it if you need to or run as root" (`os-impl-queues.c` L122-126). Raising it would change shared container state, and the README build does not require it. Whether to apply the CI value is a PI decision (D5), the same open question as ENV E5 (`env/ENV.md` L18: `msg_max` 10 vs the bundle CI's 64). (`CAP_SYS_RESOURCE`, which is absent here, governs only a per-call `mq_open` bypass of `msg_max`; `env/ENV.md` L32.) No CI image was used. Effect: the generic-linux BSP reads `msg_max` only when `geteuid() != 0` (`bsp_start.c` L66-78), so the runs as uid 1000 truncate queue depth to 10 under permissive mode (`default_config.cmake` L173-175). There is no effect on count semaphores, and `queue-test` passed. |
| R6 | Linux needs NPTL and POSIX mqueue | Config Guide L653-655: "must have support for native POSIX threads (NPTL) and POSIX message queues (mqueue)." | **Yes**: glibc 2.39 NPTL; `CONFIG_POSIX_MQUEUE=y` (`container_kernel_caps.log`); `mq_open` works (core trace) |
| R7 | Tested platforms | Config Guide L645-647: "Ubuntu LTS versions (up through 20.04 …)" | **Outside the tested range** (Ubuntu 24.04.4, kernel 6.18). This is informational, not a stated requirement. |
| R8 | Permissive mode is for a normal user | Config Guide L170: "For debugging as a normal/non-root user …". `default_config.cmake` L169-177. | **Yes.** Build and runs were done as `ubuntu` (uid 1000), with `RLIMIT_RTPRIO` = 0. As documented, priorities are not enforced; the test printed "Task Priorities not in effect, skipping sem priority test" (count-sem-test.c:207). The run user is `unspecified_by_reference` in README. The normal user was chosen because of L170 and to match the ENV cFS runs (bundle README L102). |
| R9 | Run the tests | README L30 `make test`; Config Guide L735-746 | **Done.** 84/85 passed (173 s). `network-api-test` failed only at `OS_SocketOpen(…INET6…)` (network-api-test.c:148-149): the container has no IPv6 (`EAFNOSUPPORT`, no `/proc/net/if_inet6`). This is a container limitation, not worked around, and unrelated to semaphores. |
| R10 | Run a single test in place in the build tree | Config Guide L722-733; ctest's `add_test(count-sem-test "count-sem-test")` working directory | **Yes.** Run from `build_osal_test/tests`. |
| R11 | Trace command (**probe-design condition, not a policy reference**) | The base command `strace -f -e trace=all` comes from the orchestrating task text. No project reference holds it: the roadmap has no `strace`, and no project `.md` or `.yaml` other than this report has `trace=all`. The project design note `tool/DESIGN.md` L97 names a baseline "what `strace -f` shows" but gives no flags, and it is not one of the references the policy accepts (`CONDITIONS_POLICY.md` L5-10). | run1 used the base command (plus `-o file`). The annotated runs add `-tt -T -s 65535 -k`. These are `unspecified_by_reference` agent choices. They are not output-format only: `-k` unwinds the user stack at every stop and perturbs scheduling (§9). The header comment in `scripts/strace_countsem.sh` ("extra output-format options only") was written before this review. It is left unchanged as run provenance, and this row supersedes it. |
| R12 | ptrace allowed in the container | (container fact) | **Yes**: strace 6.8 works, there is no Yama, and `CAP_SYS_PTRACE` is present |

**Container facts** (`logs/osal-visibility_container_facts_20261008.log`):

- Ubuntu 24.04.4, kernel `6.18.44-fc-v80`, 4 vCPU, 2.6 GB free disk.
- Root with CapEff `0x1fffeffffff`:
  - `CAP_SYS_MODULE` is in the set, but `/proc/modules` is absent. `container_kernel_caps.log` shows `# CONFIG_MODULES is not set` and `finit_module` → ENOSYS.
  - `CAP_SYS_RESOURCE` is absent.
- No `/proc/sys/kernel/yama`. `fs.mqueue.msg_max` = 10, `fs.mqueue.queues_max` = 256.
- Booting another kernel is not available. Neither kexec nor a hardware-assisted nested VM is available (read-only checks, `logs/osal-visibility_review_recheck_20261008.log` F9):
  - `/proc/config.gz`: `# CONFIG_KEXEC is not set`, `# CONFIG_KEXEC_FILE is not set`. `kexec_load` returns ENOSYS (`logs/descry_container_facts_20261008.log` L48).
  - No `vmx`/`svm` CPU flags; the `hypervisor` flag is set. `/dev/kvm` is absent (also `racepro_emulation_feasibility.log`), and no `kvm` misc device is registered in `/sys/class/misc`, although the kernel is built with `CONFIG_KVM=y` and `CONFIG_KVM_PVM=y`.
  - The kernel is a guest kernel `6.18.44-fc-v80` with hostname `vm`. The `fc` suffix suggests Firecracker (inference). The process is in the init IPC namespace (`ipc:[4026531839]`).
- glibc is `2.39-0ubuntu8.9`. That version comes from the earlier non-reference `apt-get install gcc-multilib` (8.7 → 8.9, see §10). OSAL documents no glibc version (`unspecified_by_reference`).

## 5. What was run

Scripts are in `/home/user/work/procrace/baselines/osal-visibility/scripts/`. Their contents are reproduced in the configure log or described in their headers. All runs used `runuser -u ubuntu`. The OSAL tree was handed to `ubuntu` first (`chown -R`), as in ENV D-1.

| Step | Exact command | Result | Log |
|---|---|---|---|
| clone | `git clone https://github.com/nasa/osal.git osal && git -C osal checkout --detach d2d877a69cff47452bcca274b309147d48e6c16f` | `v7.0.1`, clean | `osal-visibility_osal_clone_20261008.log` |
| configure | `cd osal && mkdir build_osal_test && cd build_osal_test && cmake -DENABLE_UNIT_TESTS=true -DOSAL_SYSTEM_BSPTYPE=generic-linux -DOSAL_CONFIG_DEBUG_PERMISSIVE_MODE=TRUE ..` | exit 0. BSP generic-linux, OS posix, `_XOPEN_SOURCE=600;_POSIX_OS_` | `osal-visibility_build_configure_20261008.log` |
| build | `make` | exit 0, 26.8 s, 0 warnings, 0 errors | `osal-visibility_build_make_20261008.log` |
| test (1st) | `make test` | **Failed before running any test.** The agent had listed the tests with `ctest -N` as root, which created a root-owned `Testing/`. That directory was removed and the step re-run. | `osal-visibility_build_maketest_FAILED_root_owned_Testing_dir_20261008.log` |
| test | `make test` | 84/85 passed. Only `network-api-test` failed (IPv6). `count-sem-test` passed in 1.23 s. | `osal-visibility_build_maketest_20261008.log`, `osal-visibility_ctest_LastTest_20261008.log` |
| probe, exact | (cwd `build_osal_test/tests`) `strace -f -e trace=all -o run1_exact.strace ./count-sem-test` | exit 0, 81/81 PASS. strace's default `-s 32` truncates the UtAssert write buffers, so this trace is used only for the line-number marker windows (§6.2). | `osal-visibility_strace_run1_exact_20261008.log` (full trace and stdout) |
| probe, annotated ×11 | `strace -f -e trace=all -tt -T -s 65535 -k -o <label>.strace ./count-sem-test` (run2, rep03…rep12) | all exit 0, 81/81 PASS | `osal-visibility_strace_analysis_20261008.log`; traces in `work/…/traces/` |
| contrast | `strace -f -e trace=all -o core_exact.strace ./osal-core-test`, plus the same annotated form | exit 0, 624/624 PASS (both) | `osal-visibility_strace_osal-core-test_20261008.log` |
| glibc check | `strace -f -e trace=prctl -s 64 python3 -I -c '<ctypes call of pthread_setname_np(pthread_self(), b"CF_PROBE_NAME")>'` | `prctl(PR_SET_NAME, "CF_PROBE_NAME") = 0` | `osal-visibility_glibc_setname_probe_20261008.log` |

**About the extra runs.** The 10 repetitions are a variability check chosen by the agent, not a research condition. A single name-bearing syscall in any of them would have contradicted the conclusion.

**About the glibc check.** It is the only agent-written execution. It tests glibc, not OSAL or cFS. It is used only to state which syscall the PSP's documented call becomes on this host.

**Analysis helpers.** `analyze_strace.py`, `analyze_generic.py`, `show_futex_pi.py` and `summarize.py` were written by the agent. They are read-only parsers of the strace output.

## 6. Results

### 6.1 Where names are stored and matched (OSAL `d2d877a`)

**Name storage:**

- `OS_CountSemCreate` (`osapi-countsem.c` L83-110) calls `OS_ObjectIdAllocateNew(…, sem_name, &token)` at L94, then `OS_OBJECT_INIT(token, countsem, obj_name, sem_name)` at L100.
- `OS_OBJECT_INIT` (`os-shared-idmap.h` L106-111) does `strncpy(ref->namefield, …)` and `OS_ObjectIdGlobalFromToken(&tok)->name_entry = ref->namefield`.
- Storage locations:
  - `OS_count_sem_internal_record_t.obj_name[OS_MAX_API_NAME]` (`os-shared-countsem.h` L35), in the array `OS_count_sem_table` (`osapi-countsem.c` L59).
  - The pointer `OS_common_record.name_entry` (`os-shared-idmap.h` L39), in `static OS_common_record_t OS_common_table[]` (`osapi-idmap.c` L76).
  - `OS_global_count_sem_table` is a slice of that table (L95).

**Name matching:**

- `OS_CountSemGetIdByName` (L204-215) calls `OS_ObjectIdFindByName` (`osapi-idmap.c` L975-997).
- That goes through `OS_ObjectIdGetByName` (L961-964) and `OS_ObjectIdGetBySearch(…, OS_ObjectNameMatch, …)` (L920-947), then `OS_ObjectIdFindNextMatch` (L544-576).
- The match itself is `OS_ObjectNameMatch` (L275-278): `strcmp((const char *)ref, obj->name_entry) == 0`, a linear scan over the type's slice.
- Duplicate-name detection at create uses the same matcher (L1183).

**Registry state:**

- `OS_common_record.active_id` is the only state. It moves through:
  1. 0 (free).
  2. A new ID (`OS_ObjectIdFindNextFree` L645).
  3. `OS_OBJECT_ID_RESERVED` while being created (`OS_ObjectIdConvertToken` L437-438). The global lock is released at L515.
  4. The final ID (`OS_ObjectIdTransactionFinish` L1092, under the lock re-taken at L1074).

**Locking and kernel-object mapping (posix layer):**

- Locking: `OS_Lock_Global` → `OS_Lock_Global_Impl` (`os-impl-idmap.c` L83-98) → `pthread_mutex_lock` on a per-type mutex (`OS_count_sem_table_lock`, L41). The mutex is `PTHREAD_PRIO_INHERIT` (L210).
- Unlock: `pthread_cond_broadcast` + `pthread_mutex_unlock` (L116, L123).
- Contended state wait: `pthread_cond_timedwait` (L137-172). Its `clock_gettime` (L151) goes through the vDSO and makes no syscall.
- Kernel object for a count semaphore: `sem_init(&impl->id, 0, sem_initial_value)` (`os-impl-countsem.c` L85), where `sem_t id` is in `OS_impl_count_sem_table` (`os-impl-countsem.h` L34). `pshared` = 0 means a private, unnamed semaphore, so the name is never passed to libc or the kernel.
- Task creation:
  - `OS_TaskCreate` (`osapi-task.c` L187-207) stores the name the same way.
  - `OS_TaskCreate_Impl` passes only the OSAL ID as the pthread argument (`os-impl-tasks.c` L606, L617-622; `pthread_create` L568).
  - The new task registers with `pthread_setspecific` (L838) and finds its own ID with `pthread_getspecific` (L858-866).
  - No `pthread_setname_np`, `prctl` or `setname` call exists anywhere in `src/os/posix`, `src/os/portable` or `src/bsp` (grep: 0 matches).
- Queues (the contrast case): `OS_QueueCreate_Impl` builds `"/%d.%s"` from `getpid()` and the queue name (`os-impl-queues.c` L111), calls `mq_open` (L116), then `mq_unlink` right away (L142).

### 6.2 Syscalls around create and lookup (count-sem-test.c, OSAL's own test)

Test structure:

- The main task creates "Test_Sem" (L105), checks that a duplicate is rejected (L106), and creates "Test_Sem_Nonzero" (L109).
- It then starts OSAL tasks `Task_0`, `Task_1` and `Task_2` (L160-186). `Task_0` is started twice: once alone (L160-162, then deleted) and once with the other two (L178-180). Each task looks the semaphore up by name (L35, L49, L63) and then blocks on Take or TimedWait. That makes four lookups in three task functions (`run1_exact.strace` L328 and L399 for :35, L430 for :49, L459 for :63).

UtAssert prints one report line right after each call returns, and these `write`s serve as markers. The marker prefix (`01.0nn count-sem-test.c:NNN`) fits inside strace's default 32-byte string limit, so the windows can be read from run1 even though its write buffers are truncated.

- **Create window** (`run1_exact.strace` lines 200-216, reproduced in analysis log §A; same in all annotated runs): between the report for :102 and the report for :117 there are only `write(1, "[ PASS]"…)`, `write(1, " ")`, `write(1, "01.01x count-sem-test.c:10x - …")` and `write(1, "\n")`. Three `OS_CountSemCreate` calls issued **zero** syscalls.
- **Lookup windows** (`run1_exact.strace` lines 299-465, analysis log §B). For each child task the sequence is:

  | Who | Syscalls | Source |
  |---|---|---|
  | parent | `mmap`/`mprotect` (stack), `rt_sigprocmask`, `clone3(…)` | `OS_TaskCreate` |
  | child | `rseq`, `set_robust_list`, `rt_sigprocmask` | glibc thread start |
  | child | `futex(0x…e08, FUTEX_LOCK_PI_PRIVATE)` / `FUTEX_UNLOCK_PI_PRIVATE` | UtAssert's report lock, see below |
  | child | `write(1, "… count-sem-test.c:35 - OS_CountSemGetIdByName(&sem_id, \"Test_Sem\") (0) == OS_SUCCESS …")` | UtAssert report |
  | child | `futex(0x…0c0, FUTEX_WAIT_BITSET_PRIVATE…)` | blocking `OS_CountSemTake` |

  `OS_CountSemGetIdByName` itself issued **zero** syscalls.

**Stack attribution (11 annotated traces; table in the analysis log §C):**

| Function (any depth in the user stack) | Syscalls per run |
|---|---|
| `OS_CountSemCreate` | 1. It is the `FUTEX_WAKE` of the OSAL console task, caused by the BUGCHECK print for the NULL-argument checks (:93-94; `osapi-countsem.c` L90-91). The successful creates issued 0. |
| `OS_CountSemGetIdByName` | 1, the same kind of BUGCHECK print (:96-97; L209-210). The successful lookups issued 0. |
| `OS_ObjectIdAllocateNew`, `OS_ObjectIdFindByName`, `OS_ObjectIdFinalizeNew`, `OS_ObjectIdGetById`, `OS_ObjectIdConvertToken`, `OS_TaskPrepare`, `OS_TaskRegister_Impl`, `OS_NotifyEvent`, `OS_CountSemCreate_Impl` | 0 |
| `OS_Lock_Global_Impl`, `OS_Unlock_Global_Impl`, `OS_WaitForStateChange_Impl` | 0 |
| `OS_TaskCreate` | 18: `clone3` ×4, `rt_sigprocmask` ×8, `mmap` ×3, `mprotect` ×3. `clone3` carries flags, stack and TLS only, no name. |
| `OS_CountSemTake` / `OS_CountSemTimedWait` / `OS_CountSemGive` | futex WAIT/WAKE on `0x…0c0`, only when blocking or waking |
| `OS_TaskDelay` | `clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, …)` |

**Name-bearing syscalls** (`grep -cE 'Test_Sem|Task_[0-9]'`): in each of the 11 runs with `-s 65535`, exactly 13 syscalls carry an object name. All 13 are `write(1, …)` from `OS_BSP_ConsoleOutput_Impl` ← `UT_BSP_DoText` ← `UtAssert_DoReport`. The harness prints the source expression, and that is the only place the name appears. In run1 (base command, default `-s 32`) the write buffers are truncated (for example `write(1, "01.013 count-sem-test.c:105 - OS"..., 99)`, L204), so 0 matches appear. run1 therefore supports only the window analysis above, not any claim about names in long arguments.

**Futex address → object** (static symbols from `nm -S -n`; load base from the stack-attributed BSP lock; analysis log §F):

| Address | Object | What it is used for |
|---|---|---|
| `0x…e08` | `OS_BSP_GenericLinuxGlobal+8` (`AccessMutex`) | UtAssert and console output |
| `0x…0c0` | `OS_impl_count_sem_table+32` (slot 1, the `sem_t` of "Test_Sem") | Take/Give blocking and waking |
| `0x…f20` | `OS_impl_console_table` | Console `data_sem` |

So the semaphore's only syscall-level identity is an anonymous address. Nothing ties it to "Test_Sem".

### 6.3 Contrast: osal-core-test (OSAL's own test; log `osal-visibility_strace_osal-core-test_20261008.log`)

- **Queues are visible by name:**
  - There are 69 `mq_open("<pid>.q <n>", O_RDWR|O_CREAT, 0666, {…mq_maxmsg=10…})` calls, each followed by `mq_unlink`, all with stack `OS_QueueCreate_Impl` ← `OS_QueueCreate`. In `core_exact.strace` the pid is 25212 and there are 64 distinct names (`q 0` ×4, `q 2` and `q 3` ×2 each, the rest once).
  - `mq_maxmsg` = 10 is the permissive truncation (R5).
- **Lookups by name issued no syscalls:** `OS_QueueGetIdByName` (L411-414) and `OS_TaskGetIdByName` (L304-315) both had 0.
- **Bin and mutex semaphores:** `OS_BinSemCreate` and `OS_MutSemCreate` had 0 syscalls each.
- **A registry wait can be seen, but only as an address.** In this run a new task's `OS_TaskPrepare` → `OS_ObjectIdGetById` → `OS_ObjectIdConvertToken` found its record still RESERVED. The sequence was:

  | Thread | Syscall | Stack |
  |---|---|---|
  | new task | `futex(0x555e2b9754b0, FUTEX_WAIT_BITSET_PRIVATE…, timeout)` | `OS_WaitForStateChange_Impl` |
  | creator | `futex(0x555e2b9754b0, FUTEX_WAKE_PRIVATE, INT_MAX)` | `OS_Unlock_Global_Impl` ← `OS_ObjectIdFinalizeNew` ← `OS_TaskCreate` |
  | new task | `futex(0x555e2b975460, FUTEX_UNLOCK_PI_PRIVATE)` | (release) |

  The two addresses are `OS_global_task_table_lock` (+0x50 is inside its condvar; +0 is its mutex). The wait is visible only as futex operations on lock and condvar addresses. Neither the name nor the record content appears.

### 6.4 cFE ES registration, and the PSP's thread naming (source reading)

**ES records live only in memory:**

- `CFE_ES_AppCreate` (`cfe_es_apps.c` L733-900):
  1. Takes the ES lock (L757).
  2. Looks for a duplicate name with `strcmp` in `CFE_ES_LocateAppRecordByName` (`cfe_es_resource.c` L97).
  3. Copies the app name into `AppRecPtr->AppName` (L802) and marks the entry RESERVED (L821).
  4. Releases the lock (L827).
  5. Loads the module (L841). This goes to `OS_ModuleLoad`/`dlopen`, which shows up as `openat`/`mmap` of the module *file path*.
  6. Starts the main task (L849-854).
  7. Under the lock again, finalizes the ID (L869).
- `CFE_ES_StartAppTask` (L649-725):
  1. Calls `OS_TaskCreate(…, TaskName, CFE_ES_TaskEntryPoint, …)` at L664-670.
  2. Under the ES lock (L672), fills the task record: `AppId`, `EntryFunc`, `strncpy(TaskName)` (L700), and `CFE_ES_TaskRecordSetUsed` (L703).
- The `…SetUsed` functions are plain field stores (`cfe_es_resource.h` L204-207, L453-456).
- The ES lock is `OS_MutSemTake(CFE_ES_Global.SharedDataMutex)` (`cfe_es_api.c` L2079): a PI pthread mutex, which enters the kernel only under contention.

**Lookups are memory-only:**

- `CFE_ES_GetAppID` (`cfe_es_api.c` L738-766) goes through `CFE_ES_GetTaskRecordByContext` → `OS_TaskGetId()` (`cfe_es_resource.c` L285), which is `pthread_getspecific`.
- The new task's `CFE_ES_GetTaskFunction` (`cfe_es_apps.c` L573-596) polls with `OS_TaskDelay(CFE_PLATFORM_ES_STARTUP_SYNC_POLL_MSEC)` (L575) until its ES record has an `AppId` and an entry function. A syscall observer sees only the `clock_nanosleep` calls of that poll.

**PSP thread naming (pc-linux PSP):**

- `CFE_PSP_OS_EventHandler` is registered at L361.
- On `OS_EVENT_TASK_STARTUP` (L170-201) it calls `OS_GetResourceName` (L174), sets CPU affinity for names starting with `CFE_` (L184-189, which issues `sched_setaffinity`), and calls `pthread_setname_np(pthread_self(), taskname)` (L201). Names are truncated to 15 characters (L83).
- On `OS_EVENT_RESOURCE_CREATED` it does nothing (L164-166). So creating a count semaphore triggers no PSP syscall.

**What the glibc check confirms.** On this host, `pthread_setname_np` on the calling thread becomes `prctl(PR_SET_NAME, "<name>")`. For an ES main task, the OSAL task name is the app name (`cfe_es_apps.c` L665, L850).

**Timing.** OSAL raises `OS_EVENT_TASK_STARTUP` in `OS_TaskPrepare` (`osapi-task.c` L103). That is after the new task has seen its finalized OSAL record (L84) and set its thread key (L97). It runs in the new task, independent of the creator's ES record write (L672-722).

**Not executed.** None of §6.4 was run inside cFS (decision D1).

### 6.5 Side finding from source reading: H-OSAL-1 (not executed; decision D3)

**The window.** A count semaphore is RESERVED and already named during this stretch:

1. `OS_ObjectIdAllocateNew` → `OS_ObjectIdConvertToken` (EXCLUSIVE) sets `active_id = RESERVED` and releases the type lock (`osapi-idmap.c` L437-438; unlock at L513-516). The comments at `osapi-countsem.c` L93 and `osapi-task.c` L186 ("the common ObjectIdAllocate routine will lock the object type and leave it locked") are stale. They are not a counter-reference: the code unlocks.
2. The creator sets `name_entry` without holding the lock (`osapi-countsem.c` L100; `os-shared-idmap.h` L110), then calls `sem_init`.
3. `OS_ObjectIdFinalizeNew` (L859) → `OS_ObjectIdTransactionFinish` → `OS_Lock_Global` (L1074) takes the lock again.

Between steps 2 and 3 the record is RESERVED and carries its name.

**What a concurrent lookup does in that window:**

- Under the lock, `OS_ObjectIdFindByName` → `OS_ObjectIdFindNextMatch` matches it: `OS_ObjectIdDefined(RESERVED)` is true (L567) and the names are equal. It sets `token->obj_id = RESERVED` (L570).
- `OS_ObjectIdConvertToken` then returns `OS_ERR_INCORRECT_OBJ_STATE` at L401-404, because RESERVED's type field (`0xFFFF`, `os-shared-idmap.h` L257-260, L285-289) is not valid. It returns before the unlock logic at L494-527.
- Neither `OS_ObjectIdGetBySearch` (L932-946: no cancel when ConvertToken fails) nor `OS_ObjectIdFindByName` (L988-996: release only on success) unlocks.
- This contradicts the function's own comment (L384-385: "Upon failure, the global table lock is always released").

**Consequence, read from the source:**

- The looking-up task (C1: CF) gets `OS_ERR_INCORRECT_OBJ_STATE`. CF's retry fix (`833fdbb`) retries only on `OS_ERR_NAME_NOT_FOUND` (`cases/C1.yaml` L160).
- The count-semaphore table mutex stays locked. The creator (C1: BP) would then block forever in `OS_ObjectIdFinalizeNew` (L859) → `OS_ObjectIdTransactionFinish` → `OS_Lock_Global` (L1074).
- At syscall level this would show only as a `FUTEX_LOCK_PI` that never returns, on an anonymous address.

**At the C1 case's own OSAL revision.** The line numbers above are for `d2d877a`. The C1 case's own OSAL is `42af0f73` (v6.0.0-rc4; `cases/C1.yaml` L35-37, L91-96). The probe was not run there. Read-only `git show` at `42af0f73` confirms the same no-unlock path:

- `osapi-idmap.c@42af0f7` L414-417: `if (!OS_ObjectIdIsValid(expected_id)) { return OS_ERR_INCORRECT_OBJ_STATE; }`.
- `OS_ObjectIdFindNextMatch` matches any defined `active_id` (L583).
- `OS_ObjectIdGetBySearch` (L943-967) has no cancel when ConvertToken fails, and `OS_ObjectIdFindByName` (L1000-1025) releases only on success. Both are identical in structure to `d2d877a`.

**Caveats.** The window holds no syscalls at all (§6.2), so it is very short. This extends the `C1.yaml` L96 hypothesis (which predicted only the error code). It is unverified and must not be cited as a result until it is checked.

## 7. Observation model

**Scope.** The lists below apply to observers whose events are syscalls or syscall effects on kernel objects: strace, ptrace or seccomp; RacePro's race-candidate model (`racepro.md` §5.2); DESCRY's relations. A recorder that also logs memory page ownership (Scribe, `racepro.md` §5.1: page-ownership events between threads, plus `FUTEX` resource events) may record anonymous, page-granular accesses to `OS_common_table` and the ES tables. That was not measured here. Such events would carry a page, not a name or a record field.

**A syscall-level observer in this sense, watching a native cFS process, sees:**

- Thread creation: `clone3`, with no name.
- Kernel thread names: `prctl(PR_SET_NAME)` from the pc-linux PSP. This is source-level; D1 would confirm it in cFS.
- Named OSAL queues and SB pipes at creation: `mq_open("<pid>.<name>")` then `mq_unlink`. This was verified in OSAL.
- Module loads: `openat`/`mmap` of a file path.
- Blocking and waking on futex words. These are anonymous addresses, which map to OSAL table slots or locks only through symbol tables.
- Sleeps: `clock_nanosleep` from `OS_TaskDelay`, including ES and CF poll and retry loops.
- Logged text: console `write` (for example `EVS Port1 …`, `env/logs/run01_operational.log` L42) and TO_LAB UDP output.

**It does not see:**

- Name registration or name lookup for OSAL count, binary and mutex semaphores, or for tasks.
- `OS_common_table` state: free, RESERVED or final.
- cFE ES app and task records, `TaskId`–`AppId` links, or `CFE_ES_GetAppID` results.
- SB and EVS AppId fields, or EVS registration and filter state.
- Any app's init or late-init completion.

**When the registry does show up, it shows indirectly:**

- As futex wait/wake on a lock or condvar address when a lookup contends with a creator (§6.3).
- As text when an application chooses to log a failure. For example, CF's "failed to get sem id for name %s" event (`src/CF@35f408d` `cf_cfdp.c` L1323-1325) goes through EVS to the console.

These are effects after the fact. They are not the registry operation itself.

## 8. cFS resources from the five cases: observable in principle?

| Case | Resource or state that must be ordered | Syscall-level visibility | Why |
|---|---|---|---|
| C1 (CF #184) | A named OSAL count semaphore: provider's `OS_CountSemCreate`, then CF's `OS_CountSemGetIdByName` | **No** (verified on OSAL's own test) | `sem_init(pshared = 0)` is unnamed. Registration and lookup are a memory scan under a PI mutex: 0 syscalls (§6.2). Visible only indirectly: CF's retry `OS_TaskDelay` (`clock_nanosleep`), the failure event text, and futex waits if contended. |
| C2 (cFE #198) | A provider app's late-init completion | **No** (source reasoning) | It is application state. No OSAL or kernel object represents it. Startup-sync waits show only as sleeps. |
| C3 (cFE #73) | SB AppId and EVS AppId/registration/filters read by TIME's `CreatePipe` → event path | **Partly.** The pipe creation is visible (`mq_open` with the pipe name, `cfe_sb_api.c` L163 + `os-impl-queues.c` L111-116). The AppId and filter state is not. | `CFE_SB_Global.AppId` is set by a memory store (`cfe_sb_task.c` L138) and so is `CFE_EVS_Global.EVS_AppID` (`cfe_evs_task.c` L312). Checked at the bundle cFE only; the historical C3 revision was not read. |
| C5 (cFE #72) | ES app/task records and the `TaskId`→`AppId` link, against the new main task's `GetAppID` | **No** for the records. The task's existence (`clone3`) and kernel name (`prctl`, from the PSP, source-level) are visible. | ES records are field stores under `SharedDataMutex` (§6.4). `prctl` marks OSAL-level startup, not the ES record (§6.4 timing). Checked at the bundle cFE; the historical C5 code was not read. |
| R2 (cFE #2663) | SB AppId set by SB `AppInit`, against ES/EVS SB API calls that send events | **Partly**, as for C3. SB pipe creation is visible; the SB AppId and the EVS `ILLEGAL_APP_ID` status are not. | A return code and a memory field. Any SysLog or console print of the error is text after the fact. |

## 9. Limits

- Every result was obtained on the standalone OSAL build (`OSAL_CONFIG_DEBUG_PRINTF` FALSE, generic-linux BSP, UtAssert harness), not inside cFS. The cFS bundle enables `OSAL_CONFIG_DEBUG_PRINTF` TRUE (`sample_defs/default_osconfig.cmake` L35). That adds `write`s of OS_DEBUG text on OSAL error paths only. None lies on the successful create and lookup paths read here.
- Futex syscalls depend on contention. The "0 syscalls" result holds for the uncontended lookups observed in 12 runs. Under contention, `FUTEX_LOCK_PI`/`FUTEX_UNLOCK_PI` on the type-lock address and condvar waits can appear (§6.3). They carry no name.
- `-k` slows execution and changes interleavings. Run1 (no `-k`) showed the same create and lookup windows.

## 10. Earlier partial attempt (2026-10-07): did it follow the documentation?

These items are RacePro-related, not OSAL. The assessment agrees with `racepro.md` §7. Each log was re-read here.

| Item | Followed documentation? | Use here |
|---|---|---|
| `racepro_libscribe_build.log` (x86_64 build of libscribe) | **No.** The documented target is i386, and the documented work directory is `build/`. The failure only confirms the i386 ABI. | None |
| `racepro_libscribe_build_m32.log` (added `-m32`, hand-built `record`) | **No** (an undocumented flag and an undocumented extra build) | None |
| `racepro_scribe_runtime.log` (`record -- /bin/true` without the Scribe kernel) | **No.** Its prerequisites were absent. Its `grep -c scribe /proc/kallsyms: 26` is misleading because it counts "subscribe" and "describe". | None |
| `apt_gcc_multilib.log` (`apt-get install gcc-multilib`) | **No.** No RacePro document asks for it. It upgraded glibc from `2.39-0ubuntu8.7` to `8.9`. | Provenance only: **every OSAL run here used glibc 8.9.** |
| `container_kernel_caps.log` | Factual record | Used for `CONFIG_MODULES` unset, `CONFIG_POSIX_MQUEUE=y`, and `finit_module` ENOSYS |
| `racepro_emulation_feasibility.log` | Exploratory. The "Ubuntu 10.10" choice is not from a reference. | `/dev/kvm` is absent |
| `/home/user/work/procrace/baselines/racepro/` (sources and the two build directories above) | Sources: fetched only. Builds: as above. | Not used |
| `/home/user/work/procrace/baselines/syspro/` | The earlier attempt created it empty (birth 2026-10-07 07:19:19, the same instant as `racepro/`; `syspro.md` §7, `racepro.md` §7). It left nothing to assess there. `inputs/`, `syscall_name_probe.py` and `primitives.c` were added by the SysPro task on 2026-10-08 at 01:36-01:37. `primitives.c` is a self-labelled container probe of raw POSIX primitives ("NOT SysPro and NOT cFS"), not a documented procedure. | Consistent with this probe: `sem_init` and an uncontended mutex issue no syscalls, `pthread_create` issues `clone3`, and `mq_open` carries the name. This probe supersedes it with OSAL's own tests. |

## 11. Files

**Report:** this file.

**Logs** (`logs/`):

- `osal-visibility_container_facts_20261008.log`
- `osal-visibility_commit_resolution_20261008.log`
- `osal-visibility_osal_clone_20261008.log`
- `osal-visibility_build_configure_20261008.log`
- `osal-visibility_build_make_20261008.log`
- `osal-visibility_build_maketest_FAILED_root_owned_Testing_dir_20261008.log`
- `osal-visibility_build_maketest_20261008.log`
- `osal-visibility_ctest_LastTest_20261008.log`
- `osal-visibility_strace_run1_exact_20261008.log`
- `osal-visibility_strace_analysis_20261008.log`
- `osal-visibility_strace_osal-core-test_20261008.log`
- `osal-visibility_glibc_setname_probe_20261008.log`
- `osal-visibility_source_citations_20261008.log`
- `osal-visibility_review_recheck_20261008.log` (read-only recheck behind the 2026-10-08 revision)

**Work directory** (`/home/user/work/procrace/baselines/osal-visibility/`):

- `osal/`: clone and `build_osal_test/`
- `traces/`: run1, run2, rep03-rep12, core_exact and core_annotated traces with stdout
- `scripts/`
- `cfs_main_tree/`: blob-less clone used for the gitlink check

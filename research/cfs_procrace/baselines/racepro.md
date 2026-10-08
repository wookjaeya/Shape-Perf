# RacePro (SOSP 2011): can it run here, and what does it observe?

- Date: 2026-10-08 (UTC 01:10–01:21). Revised 2026-10-08 (UTC 02:10–02:20) after a review. The review fixes were re-checked read-only; outputs are in `logs/racepro_review_recheck_20261008.log`.
- Scope: RQ2-a baseline "RacePro original tool" (roadmap §3.2, §4.1, §4.7, §13.2 [P1], [P1-code])
- Policy: `CONDITIONS_POLICY.md`. Every requirement below has a reference and a status. Nothing was substituted to make the tool run.
- **Verdict: `not_runnable_here`.** No RacePro component was run against any program. All five §4.7 cells stay `NOT_RUN`, with the cause recorded as an environment blocker. RacePro has **not** been shown to "fail to detect" any case.

---

## 0. Decisions needed from you

| # | Decision | Options | What happens by default (if you do not decide) |
|---|---|---|---|
| D1 | **How to get the paper.** The policy treats the RacePro paper as a reference, but no channel available here can reach it (§1). | (a) Put the PDF in `paper/` yourself. (b) Allow egress to `www.cs.columbia.edu` or `www.sigops.org`. (c) Accept a model taken from the code only. | §5 stays **code-derived**. No paper section numbers or quotes are claimed. The roadmap's claims about P1 §5.2 stay unverified in this pass. |
| D2 | **Whether to run RacePro at all.** It needs a bootable i386 Linux 2.6.35-scribe kernel. This machine cannot boot another kernel (§3, R1–R3). | (a) Accept `NOT_RUN` (environment blocker) and use the code-level observation analysis (§6) as a separately labelled analysis. (b) Provide a separate bare-metal or KVM-capable machine. The references name no distribution, so the userland choice would be an `unspecified_by_reference` assumption that you approve. (c) Emulate i386 with QEMU TCG inside this container. That is a `deviation` (emulated CPU). It also needs disk beyond the 500 MB budget. Free disk on `/` was 3.3 GB at 01:16 UTC, 2.6 GB at the review re-check, and 2.4 GB at 02:10 UTC on 2026-10-08. The figure shrinks as other work writes to the shared disk. | (a). The §4.7 row for RacePro stays `NOT_RUN`, with blocker references R1–R3. |
| D3 | **How cFS would run under RacePro**, only if D2 = (b) or (c). The reference cFS binary `/home/user/work/procrace/cfs_ref/cFS/build-native_std/exe/cpu1/core-cpu1` is `ELF 64-bit LSB pie executable, x86-64, … for GNU/Linux 3.2.0` (`file`). It cannot execute on an i386 2.6.35 kernel at all, because it is the wrong architecture. Its glibc also requires kernel ≥ 3.2.0. cFE also requires CMake ≥ 3.10 (`cfe/CMakeLists.txt:57`). | cFS would have to be built inside the old 32-bit userland. The build conditions there are not given by any reference, so that needs its own condition record and your approval. | Not attempted. |
| D4 | **A leftover file from the earlier attempt, `/home/user/work/procrace/baselines/racepro/build-libscribe32/log`** (8089 bytes, mode 0600, found in the review pass). The hand-built `record` wrote it. It holds that session's full environment, including credential-named variables such as `GH_TOKEN`, `GITHUB_TOKEN` and `AWS_SECRET_ACCESS_KEY`. The values were not inspected (§7). | (a) Delete it. (b) Keep it in place. | Kept in place, not copied or read further. It is outside the git repository. |

**Recommendation:** D2 = (a), plus D1 = (a) or (b) so §5 can be checked against the paper. By analysis only, §6 finds no RacePro race candidate whose racing pair is one of the five case dependencies. Those dependencies live in user-space memory, and RacePro does not build race candidates from memory. This is **not** a prediction that RacePro would never surface the failures. RacePro also validates incidental candidates by replaying to a cutoff and then going live. After go-live the failure could occur, and the user's `.test` oracle would flag it without identifying the dependency (§6). That is an analysis, not an experimental result, and it must be reported as such.

---

## 1. Sources opened and not opened

| Source | Status | Evidence |
|---|---|---|
| Paper PDF, `https://www.cs.columbia.edu/~orenl/papers/sosp2011-racepro.pdf` (roadmap [P1]) | **NOT OPENED.** Container proxy CONNECT returned 403. WebFetch returned `EGRESS_BLOCKED`. | curl 403: `logs/racepro_source_fetch_20261008.log` L3-4. WebFetch: tool response, not logged. |
| Mirrors found by web search: `sigops.org/.../25-laadan-online.pdf`, `cs.columbia.edu/~junfeng/papers/racepro-sosp11.pdf`, `webstaging.cs.columbia.edu/...`, ACM DOI landing page, project page `systems.cs.columbia.edu/projects/racepro` | **NOT OPENED.** Each host was tried once. Every attempt got a policy 403 or `EGRESS_BLOCKED`. None was retried (`/root/.ccr/README.md` L18-19: do not retry policy denials). **DOI correction:** the ACM URL actually tried was `dl.acm.org/doi/10.1145/2043556.2043590`, which does not match the roadmap DOI `10.1145/2043556.2043589` (roadmap L637). The 403 was at host level (`dl.acm.org` CONNECT), so the outcome would be the same; it was not retried. | curl attempts (sigops, systems.cs.columbia.edu, dl.acm.org): same log L5-6, L40-46, plus proxy status `recentRelayFailures` (L37-39). The appended annotation records the DOI mismatch. WebFetch attempts (junfeng, webstaging): tool response, not logged. Their exact URLs, error strings and times cannot be recovered. |
| Scholar Gateway full-text search | **NOT OPENED** (`ACCESS_DENIED`) | tool response, not logged |
| Scribe slides `viennot.biz/scribe-slides/` and the Scribe SIGMETRICS 2010 paper (linked from the README) | NOT OPENED (403) / not attempted | source-fetch log |
| `github.com/columbia/racepro` @ `681f94e3` (2011-08-23), all files | OPENED (git). Matches upstream HEAD. **No README or INSTALL exists.** Documentation-like files: `setup.py`, `racepro.conf.example`, `TODO`, `Makefile`, `scripts/*`. | `git ls-remote`, `git ls-files` |
| `github.com/columbia/racepro-bundle` @ `72e9c55d` (2011-05-06) | OPENED. `config/deploy.rb` holds the **only build-and-install recipe**. `config/kernel_config` is the kernel configuration. `.gitmodules` pins the `nviennot/*` submodules. It also contains deploy SSH key material under `users/`, which was not used or copied. | — |
| `github.com/columbia/linux-2.6-racepro` @ `b29e9afb` (2011-08-26) | OPENED as a sparse, blob-less, depth-1 clone. Opened `README.md` (fetched as a single blob; the earlier sparse checkout had missed it), `Documentation/scribe.md`, `scribe/*`, `include/linux/scribe*`, and the file list of `git diff base..HEAD` (130 files). | `scribe_changed_files.txt` |
| `github.com/columbia/libscribe-racepro` @ `011b6442`, `github.com/columbia/py-scribe-racepro` @ `56d6187e` | OPENED. Both match upstream HEAD. | — |
| `github.com/nviennot/{linux-2.6-scribe,libscribe,py-scribe}` (the repos KR names) | HEADs listed with `ls-remote`. In the review pass, `libscribe` @ `b6dbd60` (2012-10-04) and `py-scribe` @ `d03f3f1` (2012-09-29) were shallow-cloned into the session scratchpad, not the workspace, only to check for `SCRIBE_REAPED`: 0 matches in each (§2). `linux-2.6-scribe` @ `d0203238` was not cloned. | source-fetch log L16-18; review re-check log [F3] |
| GitHub REST API for any of these repos | Not enabled for this session (403). Not needed, because git transport works. | — |

---

## 2. Components and pinned revisions (code-derived)

| Component | Repository @ commit | Role |
|---|---|---|
| Scribe kernel | `columbia/linux-2.6-racepro` @ `b29e9afb4dae248e83aeb58482fb60013017576f` | Record/replay inside Linux 2.6.35. Provides `/dev/scribe`, resource ordering, page-ownership memory tracking, and go-live. |
| libscribe | `columbia/libscribe-racepro` @ `011b64426dbd7d3f9185a8d2b61b0c476ec5164c` | C library that talks to `/dev/scribe`. Also builds `scribe_init`. |
| py-scribe | `columbia/py-scribe-racepro` @ `56d6187eafd7ec2d650099c7d7afd6130d4c5bff` | Cython bindings plus the `record`, `replay` and `profiler` scripts. |
| RacePro user tools | `columbia/racepro` @ `681f94e39bf5049e24fd2641099001f5246a7b6d` | `racetest`, `racepro`, `raceshow`, `isolate`. Python 2 analysis: execution graph, race candidates, log mutation, replay plus a test script. |

**Why the columbia forks: code-derived, because the documents conflict. This choice is not `from-reference`.**

- **What the documents say.** The only install document, KR, says to clone `nviennot/linux-2.6-scribe` (L50), `nviennot/libscribe` (L58) and `nviennot/py-scribe` (L66), all at HEAD. The bundle's `.gitmodules` points to the same `nviennot` repos, pinned at kernel `e5c7bc2`, libscribe `1542c634` and py-scribe `7631c545`. No RacePro document names the columbia forks. The `racepro` repo has no README or INSTALL in `git ls-files`, and its only reference to columbia is the `author_email` in `setup.py` L9.
- **What the code requires.** RacePro master uses `scribe.SCRIBE_REAPED` (`racepro/execgraph.py:124-125`). That constant exists in `libscribe-racepro/include/linux/scribe_api.h:102`, `py-scribe-racepro/src/scribe/constants.pxi:43` and `linux-2.6-racepro/include/linux/scribe_api.h:116`. All three fork HEADs carry the same commit subject, "Exit: Added SCRIBE_REAPED…" (2011-08-23).
- **Where the constant is missing:**
  - the bundle pins libscribe `1542c634` and py-scribe `7631c545` (both 2011-05-04, each an ancestor of the matching columbia fork HEAD): 0 files each;
  - the `nviennot` HEADs that KR names: `libscribe` `b6dbd60` (2012-10-04) and `py-scribe` `d03f3f1` (2012-09-29), 0 matches each (review re-check log [F3]).
- **Conclusion.** Following KR literally, or using the bundle pins, gives a set that RacePro master cannot use. The columbia forks are the only compatible set found. The fork choice is a code-derived resolution of a conflict between documents. The build *procedure* still comes from KR and DR (R9, R10).
- **Undocumented pairing, not verified.** KC (`racepro-bundle/config/kernel_config`, L4 `Fri Feb 18 06:04:13 2011`) ships in the bundle alongside the `nviennot` kernel pin `e5c7bc2`. Applying it to columbia `b29e9af` is a combination that no document states. It was not checked, for example with `make oldconfig`.

---

## 3. Documented requirements

Abbreviations: KR = `linux-2.6-racepro/README.md` @ b29e9af, saved locally as `/home/user/work/procrace/baselines/racepro/linux-2.6-racepro_README.md`. DR = `racepro-bundle/config/deploy.rb` @ 72e9c55. KC = `racepro-bundle/config/kernel_config` @ 72e9c55.

| ID | Requirement | Reference (quote) | Status of value | Met here? |
|---|---|---|---|---|
| R1 | Build the Scribe kernel **Linux 2.6.35** with the Scribe patch | Makefile L1-3 `VERSION = 2`, `PATCHLEVEL = 6`, `SUBLEVEL = 35`. KC L3 `# Linux kernel version: 2.6.35`. KR L48-54 `make menuconfig / make / make install`. DR L23 `'ln -fs ../config/kernel_config .config', 'make -j4'` | from-reference | **No.** Not attempted. A 2.6.35 tree plus build would probably exceed the 500 MB budget. That is an **unmeasured estimate**. R3 is the decisive blocker anyway: the result could not be booted. |
| R2 | **32-bit x86 (i386) kernel** | **Target (from-reference):** KC L6-8 `# CONFIG_64BIT is not set` / `CONFIG_X86_32=y`, and KC L298 `CONFIG_M686=y`. **Code side (code-derived):** the Scribe patch (`git diff base..HEAD`) touches 18 `arch/x86` files and no `*_64` file:<br>• 7 `*_32` files: `checksum_32.h`, `pgtable_32.h`, `uaccess_32.h`, `unistd_32.h`, `entry_32.S`, `syscall_table_32.S`, `lib/usercopy_32.c`;<br>• 11 shared x86 files: `Kconfig`, `mmu_context.h`, `uaccess.h`, `asm/scribe.h`, `dumpstack.c`, `process.c`, `signal.c`, `traps.c`, `tsc.c`, `mm/mmap.c`, `mm/pgtable.c`.<br>The syscall-entry hooks exist only in `entry_32.S` (`call scribe_enter_syscall` / `scribe_exit_syscall`, L462-467 and L557-562). The Scribe syscalls exist only in `syscall_table_32.S` (L341-343). At HEAD, `entry_64.S` and `ia32/ia32entry.S` contain no `scribe` string. `scribe/Kconfig` L5-12 does **not** restrict the architecture: there is no `X86_32` dependency. RacePro uses i386 syscall numbers (`unistd.py:31-32,150` `NR_exit = 1`, `NR_fork = 2`, `NR_clone = 120`). **Diff base:** the tag `base` is not upstream v2.6.35 (`9fe6206f`). It is Viennot's commit `6d9878b` "eclone: Turned clone_flags into a 64bits variable in do_fork()" (2010-09-24). The earlier eclone patches (`v2.6.35..base`, 61 files across many arches, including `entry_64.S`, `unistd_64.h`, `process_64.c` and `ia32entry.S`) are outside the listed diff. | from-reference (config); code-derived (arch diff) | **No.** The running kernel is `6.18.44-fc-v80 x86_64`. IA32 emulation runs i386 *user* binaries, but Scribe needs an i386 *kernel*. |
| R3 | **Install and boot** that kernel on the host | DR L24-25 `'make install', 'make modules_install', 'mkinitramfs -o /boot/initrd.img-2.6.35-scribe+ 2.6.35-scribe+'`. KR L54 `make install` | from-reference | **No.** The machine is a Firecracker guest. `uname`: `Linux vm 6.18.44-fc-v80`. `/proc/cmdline` contains `nomodule`, `rdinit=/process_api` and `-- --firecracker-init`. `/proc/config.gz` has `# CONFIG_KEXEC is not set` and `# CONFIG_KEXEC_FILE is not set`, and both `kexec_load` (nr 246) and `kexec_file_load` (nr 320) return ENOSYS. `/boot` is empty, and there is no bootloader control. `/dev/kvm` is absent although `CONFIG_KVM=y`. `/proc/cpuinfo` has 0 `vmx`/`svm` flags, so KVM could not be used even if `/dev/kvm` existed. Even with CAP_SYS_BOOT and CAP_SYS_MODULE present, nothing can replace the running kernel. (Review re-check log [F1].) |
| R4 | Scribe is **built in, not a module** | `scribe/Kconfig` L5-6 `menuconfig SCRIBE` / `bool "Scribe record/replay support"`. KC L1609 `CONFIG_SCRIBE=y` | from-reference | **No.** It cannot be loaded into the running kernel. That kernel has `# CONFIG_MODULES is not set`, its command line has `nomodule`, and `finit_module` (nr 313) returns ENOSYS. |
| R5 | Kernel features that Scribe selects: `PID_NS`, `IPC_NS`, `MM_OWNER`, `DEVPTS_MULTIPLE_INSTANCES`, `EXPERIMENTAL` | `scribe/Kconfig` L7-11 | from-reference | n/a (these come from R1) |
| R6 | Recording starts a new PID-namespace init and attaches on `execve` | `Documentation/scribe.md` L33-39 "create a new task with the `CLONE_NEWPID` flag, and that will be the init process… the session will officially starts when the init task called `execev`" | from-reference | Not reachable (needs R1–R3) |
| R7 | Prerequisites "GCC and its friends", CMake, Python, **Cython ≥ 0.13** | KR L39-44 | from-reference. Exact GCC, CMake and Python versions are **unspecified_by_reference**. | Partly. GCC 13.3 and CMake are present, but they are not of the era (see R9). Python 2 and Cython are missing (R10). |
| R8 | Build dependencies installed with apt: `make`, `gcc`, `cmake`, `python-dev`, `cython`, `git-core` | DR L22, L28, L34, L52 `apt-get -q -y install`, L65 | from-reference (package names). The distribution and release are **unspecified_by_reference**. The `apt-get` and `useradd -G admin,sudo` lines (DR L59) only imply a Debian/Ubuntu host of that era. | `python-dev` and `cython` for Python 2 do not exist on Ubuntu 24.04. `apt-cache policy python2 python-dev cython` gives `Candidate: (none)` for each, and `python2.7` has no entry. No `python2`, `python2.7`, `cython`, `cython3` or `qemu-system-i386` binary is installed. Only `python3.10`–`3.13` and `/root/.local/bin/uv` exist (review re-check log [F13]). |
| R9 | libscribe build and install. The two documents differ slightly. | KR L56-62: `cd build` / `cmake ..` / `make install`, with no separate `make`. DR L29-31: `:work_dir => 'build'`, `:build_cmd => ['cmake ..', 'make']`, `:install_cmd => 'make install'`. | from-reference (both quoted separately) | Not run in this pass. The earlier attempt shows that the x86_64 build fails on i386-only `struct pt_regs` fields (`xcs`, `eip`, `eax`, … through `xds`, `xes`, `xfs`, `xgs`, `xss`, in `src/debug.c:400-403`), which is consistent with R2. |
| R10 | py-scribe build and install with Cython. The two documents differ. | KR L64-68: `cd py-scribe` / `./setup install`. DR L35-36: `python setup.py build` / `python setup.py install`. `py-scribe-racepro` @ 56d6187 has **no `setup` script**, only `setup.py` (and `src/`), so the KR command does not exist at this revision. That is a small inconsistency between the documents. `setup.py` L6 `from Cython.Distutils import build_ext`. | from-reference (both quoted separately) | **No.** No Python 2 is available. A Python 2 version is not specified (unspecified_by_reference). |
| R11 | RacePro Python package requires `networkx`, `argparse`, `scribe`, `pygraphviz`. The code is Python 2. | `racepro/setup.py` L13. Python 2 idioms at `racecore.py:66,81` (`ifilter`, `iteritems`) and networkx 1.x APIs (`predecessors_iter`, `edges_iter`) at `execgraph.py:31,80`. | from-reference (packages). Versions are unspecified_by_reference. | **No.** Python 2 is missing, and so is the `scribe` module (R10). |
| R12 | Runtime: `sudo`, `mount`/`umount`, `chroot`, output directory `/persist` (default) | `racepro/execute.py:89-90,103` (`sudo(['mount', ...])`). `scripts/racetest` L155-156 `args.outdir = '/persist'` | from-reference | Possible here (root is available), but moot without R1–R4 |
| R13 | A per-program **test script** `<test>.test` serves as the bug oracle | `racetest.py` L151-152 (`'%s.test' % t_name`). `scribewrap.py` L234-239 (`BUG REPRODUCED` / `BUG not triggered`). Examples in `tests/*.test` | from-reference | n/a. This is an input the user writes. A cFS oracle would be a research-side addition. |
| R14 | Hardware | KC L268 `CONFIG_SMP=y`, L348 `CONFIG_NR_CPUS=8`. CPU vendor and memory size are not documented. | unspecified_by_reference | 4 vCPU Xeon x86_64. Moot. |
| R15 | Artifact-evaluation section in the paper | The paper was not opened (D1). SOSP 2011 had no artifact-evaluation track; this is not verified. | — | — |

---

## 4. What I ran

All logs are under `/home/user/Shape-Perf/research/cfs_procrace/baselines/logs/`.

| Step | Commands | Log |
|---|---|---|
| Source reachability (one attempt per host) | `curl -sS -o /dev/null -w '%{http_code}' --max-time 30 <url>` for the paper, ACM, slides and GitHub. `git ls-remote https://github.com/<repo>.git HEAD` for 8 repos. Proxy `__agentproxy/status`. | `racepro_source_fetch_20261008.log` |
| Container facts | `uname -a`; `/etc/os-release`; `nproc`; `free`; `df`; `ls /proc/modules`; `zgrep CONFIG_… /proc/config.gz`; CapEff decode; `finit_module` probe (ctypes `syscall(313, fd(/dev/null), "", 0)`); `kexec_load` probe (`syscall(246, 0, 0, NULL, 0)`); `ls /dev/kvm /dev/scribe`; kallsyms grep that excludes `subscribe`/`describe`; `gcc -m32` hello as an IA32-emulation check (scratchpad only, not a RacePro build); `strace -f true`; `which python2 cython qemu-*` | `racepro_container_facts_20261008.log` |
| Kernel docs and diff | `git fetch --depth=1 --filter=blob:none origin refs/tags/base:refs/tags/base`; `git diff --name-only base HEAD` (130 files, saved as `/home/user/work/procrace/baselines/racepro/scribe_changed_files.txt`); `git show HEAD:README.md` | `racepro_code_inspection_20261008.log` |
| Code inspection | greps over racepro, libscribe, py-scribe and kernel sources (pins, `SCRIBE_REAPED` coupling, resource types, race classes, HB edges, memory-event use, replay outcomes) | `racepro_code_inspection_20261008.log` |
| cFS mapping (read-only) | greps over `/home/user/work/procrace/cfs_ref/cFS` (bundle `088b2fa8`, osal `d2d877a6`, cfe `c5fb2b4d`, cf `15a871e6`) | `racepro_cfs_mapping_20261008.log` |
| Review re-check (02:10 UTC, read-only) | `/proc/cmdline`; KEXEC config; `kexec_file_load` probe; `vmx`/`svm` count; `/boot`; `df`; arch-file split and `base` tag; `entry_32.S`/`entry_64.S`/`ia32entry.S`/`syscall_table_32.S` at HEAD; `nviennot` HEAD clones (scratchpad); build-command sources; fget/mqueue path; RDTSC citations; `file core-cpu1`; oracle and cutoff lines; C2 ES wait loop; DOI mismatch; prior-attempt artifacts; apt history; Python 2 availability | `racepro_review_recheck_20261008.log`. Annotations were appended to `racepro_container_facts_20261008.log` and `racepro_source_fetch_20261008.log`. |

I did **not** build libscribe or py-scribe, install packages, compile a kernel, or run any RacePro or Scribe binary. The documented runtime prerequisites R1–R4 cannot be met. Any build of the user-space parts would require unreferenced choices: Python 2 version, `-m32`, networkx 1.x version. Even then it would yield at most an import-level unit test, which the policy says is not a research result. The workspace is 201 MB (`/home/user/work/procrace/baselines/racepro`).

One operational note: a first `git fetch` of the `base` tag without `--depth` began downloading the trees of the entire pre-2.6.35 history (about 150 MB). I stopped it and deleted the temporary pack before re-fetching with `--depth=1`.

Review-pass side effects: four HEAD blobs (`entry_32.S`, `syscall_table_32.S`, `entry_64.S`, `ia32/ia32entry.S`) were lazily fetched into the blob-less kernel clone, which stays at 197 MB in `.git`. Two shallow clones of `nviennot/libscribe` and `nviennot/py-scribe` (696 KB) are in the session scratchpad, not the workspace. No packages were installed, and nothing was built or installed system-wide.

---

## 5. Observation and race model (code-derived; paper §4–§5 NOT verified, see D1)

### 5.1 What Scribe records (kernel `b29e9af`)

- **Unit of recording.** A Scribe context covers a task tree whose root is the init of a new PID namespace. Tasks attach in `copy_process()` and `do_exec()` (`Documentation/scribe.md` L33-39, L80-85). Each task, whether a process or a thread, has its own event queue keyed by pid (L41-51, L112-131).
- **Nondeterminism recorded.** README L9-16: "rendezvous and sync points, to efficiently record nondeterministic interactions such as related system calls, signals, and shared memory accesses". Log levels list syscall return values, signal cookies, resource locks with object and serial, memory ownership, user-access data, and registers (`scribe.md` L145-164). That range does not mention RDTSC. RDTSC is cited separately:
  - `scribe.md` L245: "a rdtsc event: used when the userspace process does a RDTSC instruction";
  - `arch/x86/include/asm/scribe.h` L27 `scribe_handle_rdtsc`;
  - `arch/x86/kernel/tsc.c` and `traps.c` are in the diff;
  - the dedicated section, `scribe.md` L94, reads only "**RDTSC**. TODO".
- **Shared kernel resources.** Scribe serializes them and records the access order as "resource lock" events with a per-resource serial (`scribe.md` L89-92). Resource types are listed in `include/linux/scribe_resource.h` L221-229: `INODE`, `FILE`, `FILES_STRUCT`, `PID`, `FUTEX`, `IPC` (SysV), `MMAP`, `PPID`, `SUNADDR` (Unix socket address).
  - The diff touches `ipc/msg.c`, `ipc/sem.c`, `ipc/shm.c` and `kernel/futex.c`.
  - It does **not** touch `ipc/mqueue.c`, so POSIX message queues are not instrumented as a resource type.
  - The generic `fget` hook locks a file only when the syscall has first called `scribe_track_next_file` (`scribe/resource/lock_wrappers.c` L220-233). `scribe_pre_fget`/`scribe_post_fget` (L274-312) lock only if `scribe->lock_next_file` is set. Within `scribe/` and `include/`, nothing else sets it. Because `ipc/mqueue.c` is not modified, `mq_timedsend` and `mq_timedreceive` do not request file tracking.
  - Fd allocation in `mq_open` goes through the patched `fs/file.c` and `fs/open.c` (both in the diff), so a `FILES_STRUCT` event is possible there. **Whether `mq_open`'s fd allocation produces `FILES_STRUCT` events was not checked.**
- **Shared memory.** Scribe uses a page-ownership protocol (`scribe/memory.c` L27-39, L58-77, L118). A task takes ownership of a page, and each ownership carries a serial. The `mm_struct` reference count "is used to determine if a task is operating in single or multithreaded" mode. This makes shared-memory interleavings replayable between processes and between threads.
- **Threads vs processes.** Both are Scribe tasks with separate queues. README L7: "Scribe is a low-overhead multi-threaded application record-replay mechanism."

### 5.2 What RacePro turns into race candidates (`racepro` `681f94e`)

- **Graph nodes.** Each task's syscalls form a node chain in program order (`execgraph.py` L87-113). A "proc" is a Scribe pid (`session.py` L105-145). `clone` counts as `fork` (`unistd.py` L373 `SYS_fork = set([NR_fork, NR_clone, NR_vfork])`), so threads become graph nodes exactly like processes.
- **Happens-before edges.** There are only five sources, combined with vector clocks (L188-192):
  - program order;
  - fork/clone to the child's first node (`execgraph.py` L129-132);
  - the child's last node to the reaping `wait` (L134-137);
  - pipe or socket write to the matching read (L139-180);
  - signal send to receive (L182-186).
  - **No HB edge comes from futex, SysV semaphore, POSIX mqueue, or shared memory.**
- **Race classes** (`racecore.py` `find_show_races` L692-727, plus TOCTOU through `-T`):
  1. **RESOURCE** (L96-303). For each Scribe resource that has at least one write (L276-280):
     - excludes `SCRIBE_RES_TYPE_FUTEX` (L194, L282);
     - takes only accesses made inside a syscall (`in_syscall=True`, L62-78, L218);
     - pairs accesses from two different tasks that are concurrent by vector clock, with at least one write (L222-251);
     - filters parent-directory false positives (L196-213, L254-259) and ignored paths (L267-294).
  2. **EXIT-WAIT** (L396-567): concurrent exits of children that share a reaper.
  3. **SIGNAL** (L308-391): flips whether a signal interrupted a syscall.
  4. **TOCTOU** (L572-672): pattern-based, on `FILE`, `FILES_STRUCT` and `INODE` resources.
- **Shared memory is not a candidate source.** No RacePro module references Scribe memory events (`grep EventMem|mem_owned|mem_public` → 0 matches). Memory interleavings are recorded for replay, but they never become race candidates or HB edges.
- **Validation (replay plus go-live).** For each candidate, `prepare()` computes a crosscut and writes a mutated log (L146-182, `helpers.save_modify_log`). The log contains:
  - bookmarks before and after the reordered syscall;
  - injected psflags `SCRIBE_PS_ENABLE_RESOURCE | RET_CHECK` (plus `DATA` when the syscall is not a `wait`);
  - a cutoff after which tasks run live. `scribe.md` L133-136: "a process can golive when its entire event queue has been consumed."

  The replay runs with periodic deadlock checks. It reports `replay deadlock` or `replay diverge` on failure (`scribewrap.py` L185-219). On success, the user-supplied `<test>.test` script decides `BUG REPRODUCED` or `BUG not triggered` (L234-239; `racetest.py` L20-54). RacePro has no built-in failure oracle.

Roadmap §3.3 claims that P1 "records process, thread and shared memory; the §5.2 detection model is syscall effects on kernel objects". The code above is consistent with that claim, but I could not check it against the paper text in this pass.

---

## 6. The five cFS cases: could RacePro observe them in principle? (analysis, NOT a run)

This uses the current reference cFS (`088b2fa8`; osal `d2d877a6`; cfe `c5fb2b4d`; cf `15a871e6`). The historical revisions for C3, C5 and R2 still need to be checked under roadmap task A. Older OSAL POSIX implementations could differ.

Common fact: native cFS runs as **one Linux process**, and every OSAL task is a `pthread_create` thread (`osal/src/os/posix/src/os-impl-tasks.c:568`). Under Scribe each thread would be a separate task, so thread granularity is not itself the obstacle. The obstacle is that each case's dependency lives in process memory, guarded by futex-backed locks.

In the mapping column, "no case-dependency candidate" is short for: no RESOURCE, EXIT-WAIT, SIGNAL or TOCTOU candidate whose racing pair is the case dependency (analysis). It does **not** mean that RacePro would report nothing, or that the failure could not occur during a RacePro run (see the caveat below the table).

| Case | Dependency (reference cFS) | Kernel-visible effect | RacePro mapping |
|---|---|---|---|
| C1 (CF#184) | BP creates a named count semaphore. CF looks it up by name (`apps/cf/fsw/src/cf_cfdp.c:1282` `OS_CountSemGetIdByName`). | Creation calls `sem_init(&impl->id, 0, …)`, an unnamed, process-private semaphore (`os-impl-countsem.c:85`), so no named kernel object is created. The name is registered in `OS_count_sem_table` (`osapi-countsem.c:59`). The lookup searches that user-space table (`OS_ObjectIdFindByName`, `osapi-idmap.c:975`) under `OS_Lock_Global` (a pthread mutex, `posix/src/os-impl-idmap.c:92`). | No Scribe resource event links BP's create to CF's lookup. Futex syscalls happen only on contention, and the FUTEX type is excluded anyway. **No case-dependency candidate (analysis).** |
| C2 (cFE#198) | Late-init readiness of a provider app, plus the cFE ES startup synchronization that the fix relies on | **Private half:** EVA CWS app state. The apps are private (`cases/C2.md` L21), and no syscall-level effect is identified. **Public half (cFE ES startup sync):** `CFE_ES_WaitForSystemState` (`cfe_es_api.c:514`) loops `while (CFE_ES_Global.SystemState < MinSystemState)` (L585) with `OS_TaskDelay` (L603). The readiness state is an in-memory field. The waits are sleeps, which create no Scribe resource or HB edge. | **No case-dependency candidate (analysis).** The state is not visible to the race model. The C2 mapping stays role-derived, because the original apps are private. |
| C3 (cFE#73) | SB AppId and EVS AppData/filters used by the TIME path | `CFE_SB_Global.AppId` (`cfe_sb_priv.h:237-260`, field at L241) and `CFE_EVS_Global.AppData[]` (`cfe_evs_task.h:116`) are globals. TIME's `CreatePipe` calls `mq_open` (`os-impl-queues.c:116`). `ipc/mqueue.c` is not modified, so `mq_timedsend`/`mq_timedreceive` do not request file tracking. Whether `mq_open`'s fd allocation produces `FILES_STRUCT` events was not checked (§5.1). In any case the queue is not the dependent state. | **No case-dependency candidate (analysis).** The dependency is the in-memory AppId/AppData, not the queue. |
| C5 (cFE#72) | ES `AppTable`/`TaskTable` publication versus the new task's `GetAppID` | `CFE_ES_Global.TaskTable/AppTable` (`cfe_es_global.h:157,165`) under `CFE_ES_LockSharedData` → `OS_MutSemTake` (`cfe_es_api.c:2075-2079`). `clone` is visible, but only as an HB edge from creator to child. | **No case-dependency candidate (analysis).** The registry write after `clone` is a memory write. |
| R2 (cFE#2663) | SB global AppId set by SB AppInit versus ES/EVS calls into SB | `CFE_SB_Global.AppId` (memory) | **No case-dependency candidate (analysis).** |

**Analytical conclusion (to be labelled "analysis", not "detection result"):** none of the five dependencies matches any of RacePro's race classes as implemented. RacePro would at most build candidates on incidental kernel resources, such as stdout writes (`FILE`/`INODE`), `FILES_STRUCT` during module loading, or `MMAP`, and those are not the case dependencies. `racecore.py` L194 and L282 exclude only `FUTEX`, so `MMAP`, `FILE`, `FILES_STRUCT` and `INODE` remain candidate sources.

**Caveat: go-live after an incidental candidate.** RacePro validates *every* candidate, including incidental ones, in the same way:

1. It replays the mutated log up to a cutoff (`racecore.py` L173-178, `cutoff = dict(bookmark2)`).
2. The tasks then go live (`scribe.md` L133-136: "a process can golive when its entire event queue has been consumed"). After go-live the cFS tasks run nondeterministically, so a case failure could occur.
3. The user's `.test` oracle then decides `BUG REPRODUCED` or `BUG not triggered` for whichever candidate was being validated (`scribewrap.py` L234-239).

So a cFS failure-detecting oracle could print `BUG REPRODUCED` under an unrelated candidate. RacePro would not identify the case dependency as the race. The analysis supports only "no candidate whose racing pair is the case dependency". It does not support "RacePro would not report the failure". All §4.7 cells stay `NOT_RUN`.

Two further points:

- A future "RacePro + cFS semantics" extension, for example treating OSAL/cFE registry publications as resources, would be an **extension**, not the original tool (roadmap §3.5).
- Running cFS under Scribe would also need a cFS build that works on 2.6.35/i386 (see D3).
  - The reference binary `build-native_std/exe/cpu1/core-cpu1` is `ELF 64-bit LSB pie executable, x86-64, … for GNU/Linux 3.2.0`. It cannot execute on an i386 2.6.35 kernel at all, because it is the wrong architecture. Its glibc also requires kernel ≥ 3.2.0.
  - cFE also needs CMake ≥ 3.10.
  - Secondary note: an `-m32` hello built in the scratchpad is also tagged "for GNU/Linux 3.2.0" (`racepro_code_inspection_20261008.log`, last section). That build depends on the earlier, non-reference `gcc-multilib` install (§7).

---

## 7. Earlier partial attempt (2026-10-07): did it follow the documentation?

| Item | What it did | Followed docs? | Reuse? |
|---|---|---|---|
| Clones in `/home/user/work/procrace/baselines/racepro/{racepro,racepro-bundle,libscribe-racepro,py-scribe-racepro,linux-2.6-racepro}` | Cloned the columbia repos at HEAD. The kernel clone is sparse, blob-less and depth 1. | **Partly.**<br>• All revisions match upstream HEAD on 2026-10-08.<br>• KR names `nviennot/*` HEAD, not the columbia forks. The fork choice was not justified at the time. §2 now justifies it as code-derived: the documents conflict, and only the columbia set has `SCRIBE_REAPED`. It is not from-reference.<br>• The sparse checkout **missed `README.md`**, the main install document. It is now fetched. | **Yes**, as source of truth |
| `racepro_libscribe_build.log` (dir `build-libscribe/`) | `cmake` + `make` of libscribe on x86_64 with GCC 13.3 in an out-of-tree directory. Failed on i386 `pt_regs` fields. The first error is `debug.c:400:24` (`xcs`, then `eip`, `eax`, …) and the last is `debug.c:403:68` (`xss`), so the failure spans `src/debug.c:400-403`. | **No.** The documented target is i386 (KC L6-8), and the documented work dir is `build/` (DR L29, KR L60). The failure only confirms the i386-only ABI. It is not a tool result. | Only as evidence for R2 |
| `apt_gcc_multilib.log` | `apt-get install gcc-multilib`: 24 new packages, and **glibc upgraded 2.39-0ubuntu8.7 → 8.9** (`libc6`, `libc-bin`, `libc6-dev`, `libc6-dbg`, `libc-dev-bin`, `locales`). | **No.** Not in any RacePro document. The shared container state changed, which affects the provenance of later builds. Confirmed in `/var/log/apt/history.log` L539-543: `Commandline: apt-get install -y --no-install-recommends gcc-multilib`, End-Date 2026-10-07 07:21:50. `dpkg -l libc6` now shows `2.39-0ubuntu8.9`. The reference cFS `core-cpu1` was built 2026-10-08 01:23:15, after the upgrade. **Flag for the cFS ENV owner:** `env/ENV.md`, `env/CONDITIONS.md` and `env/logs/build_00_env.log` record no glibc version (0 matches for `2.39`, `libc6` or `multilib`). The cFS build condition record should state libc6 `2.39-0ubuntu8.9`, and that the cause of the upgrade was this non-reference `gcc-multilib` install. This report does not edit those files. | Record only |
| `racepro_libscribe_build_m32.log` (dir `build-libscribe32/`) | Added `-m32` to target i386 from x86_64 (`CMakeCache.txt` L42 `CMAKE_C_FLAGS:STRING=-m32`). Build succeeded. A `record` binary also exists, `build-libscribe32/record` (ELF 32-bit, Intel 80386, 2026-10-07 07:22:03). `CMakeLists.txt` builds only `libscribe` (L6) and `scribe_init` (L8), so `record` was built outside CMake, presumably from `examples/record.c`. **The compile command was never logged**: `racepro_libscribe_build_m32.log` does not mention `record`. This is a provenance gap. The run also left `build-libscribe32/log` (8089 bytes, mode 0600). It was written by `examples/record.c` L19 `open("log", …)` and holds the recorded process's argv and its full environment, 146 entries. Those entries include credential-named variables such as `GH_TOKEN`, `GITHUB_TOKEN`, `AWS_SECRET_ACCESS_KEY` and `CLOUDSDK_AUTH_ACCESS_TOKEN`; values not inspected. Do not copy or publish that file. Deleting it is your decision. | **No** (`deviation`: an undocumented flag, and an undocumented and unlogged extra build) | No |
| `racepro_scribe_runtime.log` | Ran `./record -- /bin/true` with no Scribe kernel and got `can't record: Bad file descriptor`. It also logged `grep -c scribe /proc/kallsyms: 26`, a substring count that includes `subscribe` and `describe`. | **No.** The documented runtime prerequisites were absent. The result is meaningless, and the kallsyms count is misleading (the correct count is 0, see `container_kernel_caps.log` and today's facts log). | No |
| `container_kernel_caps.log` | Kernel config, CapEff, `finit_module` probe | Factual. Consistent with today's re-check: `CONFIG_MODULES` unset, ENOSYS. The log itself has no `uname`. The kernel `fc-v77` is **inferred** from `racepro_scribe_runtime.log` (`# uname -r: 6.18.44-fc-v77`), which was written in the same minute (07:22). Today's kernel is `fc-v80`. | Yes, as facts |
| `racepro_emulation_feasibility.log` | `/dev/kvm` check, `apt-get -s install qemu-system-x86`, reachability of `old-releases.ubuntu.com/releases/10.10/` | Exploratory. "Ubuntu 10.10" is **not named in any reference**; it was inferred from the 2.6.35 kernel series (`unspecified_by_reference`). QEMU TCG would be a `deviation` (D2-c). | Record only |
| `/home/user/work/procrace/baselines/syspro/` | Empty directory | Out of scope for this report (separate SysPro task) | — |
| System-wide state | No `make install` was run. There is no `libscribe*` in `/usr/lib` or `/usr/local/lib`, no `scribe_init` in `/usr/bin` or `/usr/local/bin`, and no `scribe.h` in `/usr/include` or `/usr/local/include` (review re-check log [F11]). | — | Only the apt `gcc-multilib` step changed shared system state. |

---

## 8. Verdict

**`not_runnable_here`.** Evidence:

1. Scribe exists only as a built-in (`bool`) patch to an **i386 Linux 2.6.35** kernel (KC L3, L6-8, L1609; `scribe/Kconfig` L5-6). On the code side, the syscall-entry hooks exist only in `entry_32.S` and `syscall_table_32.S`. No `*_64` file is touched (see R2). Its documented installation is `make install` plus `mkinitramfs` into `/boot` (DR L24-25).
2. This machine is a Firecracker guest running `6.18.44-fc-v80 x86_64`. Its `/proc/cmdline` contains `nomodule` and `--firecracker-init`.
   - It cannot load modules: `# CONFIG_MODULES is not set`, and `finit_module` returns ENOSYS.
   - It cannot `kexec`: `# CONFIG_KEXEC is not set` and `# CONFIG_KEXEC_FILE is not set`, and `kexec_load` and `kexec_file_load` both return ENOSYS.
   - `/boot` is empty. There is no `/dev/kvm`, and the CPU exposes no `vmx`/`svm` flag.

   So there is no way to boot the documented kernel. `/dev/scribe` is absent.
3. The user tools need Python 2, Cython ≥ 0.13, networkx 1.x-style APIs, pygraphviz and the `scribe` extension. Ubuntu 24.04 has no Python 2, and the versions are unspecified by reference.
4. The paper could not be opened through any available channel, so the observation model above is code-derived (D1).

§4.7 table entries for the RacePro row (C1, C2, C3, C5, R2): `NOT_RUN — environment blocker (R1–R4: i386 2.6.35-scribe kernel cannot be booted here)`. Keep the §6 analysis in a separate, explicitly labelled "observation-model analysis (not run)" column.

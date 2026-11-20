# 07: Feasibility on this machine of the candidate detector mechanisms

Report AR1-07, written 2026-10-12. Scope: which build-, analysis-, instrumentation- and schedule-control mechanisms work on this VM against the pinned cFS v7.0.1 build, what each costs, and how far each moves away from the reference build (gcc, README mode).

The report labels its claims as follows:

- **Fact (probe)**: measured by me in this session, under `scratchpad/arch/probe_env/`.
- **Fact (code/doc)**: read from a file. Citations are `path:line` at the pinned revision.
- **Fact (record)**: taken from the existing research records.
- **Inference**: my reasoning. It is never presented as a measurement.

Rules followed:

- Nothing in `/home/user/work/procrace/cfs_ref`, `/home/user/work/llvm-project` or the research records was modified.
- Nothing was installed. No sysctl or cgroup was changed or created. cFS itself was **not** run.
- All probes, builds and outputs are under `/tmp/claude-0/-home-user-Shape-Perf/e983f27d-f015-5a5d-b912-399245630e78/scratchpad/arch/probe_env/` (96 MB at the end).
- The tracefs/debugfs mount test ran inside a private mount namespace (`unshare -m`). The mount was not visible outside it and disappeared with it.

Path abbreviations:

- `CFS` = `/home/user/work/procrace/cfs_ref/cFS`
- `BLD` = `CFS/build-native_std`
- `LLVM` = `/home/user/work/llvm-project`
- `REC` = `/home/user/Shape-Perf/research/cfs_procrace`
- `PROBE` = the probe directory above

---

## 0. Summary

1. **Static analysis is feasible now, at negligible cost, with no change to the cFS build.**
   - The reference build already exports `compile_commands.json`, because `CFS/target-rules.mk:33` adds `-DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE`.
   - The existing LLVM 23.0.0git build (Release, assertions, RTTI) is consumable out-of-tree through `find_package(MLIR)` and `find_package(Clang)`.
   - A Clang LibTooling probe parsed **all 448 flight-software compile commands** in 10 s on one thread. Clang needs two extra flags, because the build's gcc-only warning options become errors under `-Werror`.
   - Bitcode for all 448 TUs: 4.4 s. Link of the whole `core-cpu1` IR: 0.35 s. MLIR import: 0.78 s.
   - A minimal out-of-tree MLIR dialect plus an `mlir-opt`-style driver with the SMT dialect builds in about 7 s. `--export-smtlib` works.
   - **Z3 is not installed.**
2. **Runtime observation with no change to the binary is feasible from the root session.** The observed `core-cpu1` keeps running as the README-mode `ubuntu` user.
   - Root has `CAP_PERFMON`, `CAP_BPF`, `CAP_SYS_PTRACE` and `CAP_SYS_ADMIN`, but not `CAP_SYS_RESOURCE`.
   - uprobes through the perf PMU work, on a uid-1000 process and before the file is mapped. eBPF programs load and attach to uprobes. HW watchpoints work through perf and gdb. ptrace works. sched/syscall tracepoints exist.
   - **Cost trap in this KVM/Firecracker VM:** every trap that needs a single-step or debug exception costs about **32–34 µs**:
     - a uprobe on `endbr64` or on a non-emulated instruction;
     - a HW watchpoint hit.
     A uprobe on the emulated `push %rbp` at function entry +4 costs **0.57 µs**. A gdb breakpoint costs about **203 µs** per hit.
3. **No hardware PMU counters are exposed** (`PERF_TYPE_HARDWARE` returns ENOENT, and there is no `cpu` PMU in sysfs). So record/replay that depends on HW counters, such as rr, cannot work here. The reason rr needs them is UNVERIFIED (see §10). kprobes are not configured.
4. **Interposition with gcc kept as the compiler.**
   - `ld --wrap` works for calls from cFE core into the API and from apps into the API. The app `.so` files must be relinked with the same `--wrap`, and the wrappers must be exported by `core-cpu1`.
   - `--wrap` misses calls inside the same TU. That is **88 of 3,762** public cFE/OSAL API call sites, or 2.3% (`CFE_ES_WriteToSysLog` 21, and OSAL file/socket helpers).
   - `LD_PRELOAD` **cannot** interpose symbols defined in the PIE executable, even for callers in dlopen'ed apps (probe). It is also refused by policy (`REC/env/run_cfs.sh:154-158`).
   - `-finstrument-functions` adds +15.7% `.text` to `cfe_sb_api.o`. `-fpatchable-function-entry` and `-fcallgraph-info` work.
   - gcc plugin headers are absent (an apt candidate exists).
   - gcc TSan works.
5. **Schedule control.**
   - The README-mode user cannot use RT policies (`RLIMIT_RTPRIO` 0; `chrt -f` gives EPERM). That user can use `taskset` and raise niceness.
   - Root *can* impose `SCHED_FIFO` and affinity on the uid-1000 process from outside (probe). Native PCT is therefore technically possible, but only as a **deviation** from E1 (`Cpus_allowed 0-3`) and E4 (`SCHED_OTHER`).
   - Cheaper and condition-preserving alternatives: cooperative gates at the OSAL/cFE boundary (thread handoff 1.1 µs on the same CPU, 10.5 µs across CPUs), or a ptrace scheduler (per-thread stop works, but the round trip is expensive).
6. **Resources.**
   - Disk: **18 GB free**, not 1.1 GB. The host cleanup on 2026-10-12 freed space (`REC/env/HOST_CLEANUP_20261012.md`).
   - RAM 15 GB, 4 vCPU (Xeon @2.1 GHz, KVM, Firecracker microVM).
   - **The kernel changed** since the reference build: `6.18.44-fc-v80` then (CONDITIONS B3), `-fc-v114` now.

---

## 1. Machine facts and drift from the recorded conditions

| Item | Value now | Source | Note |
|---|---|---|---|
| OS | Ubuntu 24.04.4 LTS | `/etc/os-release` | same as B3 |
| Kernel | `6.18.44-fc-v114 #1 SMP PREEMPT_DYNAMIC`, built with gcc 15.3.0; `Dynamic Preempt: none`; `CONFIG_HZ=250`; `CONFIG_PREEMPT_NONE=y` | `uname -a`, `dmesg`, `/proc/config.gz` | **Drift:** CONDITIONS B3 (`REC/env/CONDITIONS.md:64`) records `-fc-v80`. Inference: timing measurements made under v80 (startup 1.6 s, overflow counts) should be re-checked before anyone compares them with new runs. |
| VM type | Firecracker microVM. Boot args include `nomodule ipv6.disable=1 rdinit=/process_api ... --firecracker-init` | `/proc/cmdline` | `nomodule`: no loadable kernel modules (rules out SystemTap/LKM-based tracers). `ipv6.disable=1` is the root cause of the IPv6 failure recorded in C12/B3. |
| CPU | 4 vCPU, Intel Xeon @2.10 GHz (family 6 model 207), KVM, 1 thread/core | `lscpu` | |
| RAM | 15.7 GB total, about 15 GB available, no swap | `/proc/meminfo` | |
| Disk | `/dev/vda` 252 G, **18 G available** | `df -h` | The task text says "~1.1 GB": the figure from before the cleanup (`REC/env/HOST_CLEANUP_20261012.md` L3). |
| Existing big trees | LLVM build 7.8 GB; `cfs_ref` 782 MB (`build-native_std` 454 MB) | `du -sh` | |
| Session user | root (uid 0). `CapEff 000001fffeffffff`: all capabilities except `CAP_SYS_RESOURCE`; includes `CAP_SYS_ADMIN`, `CAP_SYS_PTRACE`, `CAP_SYS_NICE`, `CAP_PERFMON`, `CAP_BPF`. `NoNewPrivs 0`, `Seccomp 0`. `RLIMIT_RTPRIO 0`. | `/proc/self/status`, `capsh --decode` | Matches D-4 (`CONDITIONS.md:123`). |
| README user | `ubuntu` uid 1000, also in group `sudo` | `id ubuntu` | The run uses `setpriv` with no capabilities (E1, `CONDITIONS.md:102`). |
| Kernel knobs | `perf_event_paranoid=2`, `unprivileged_bpf_disabled=2`, `kptr_restrict=0`, no Yama (`ptrace_scope` absent), `sched_rt_runtime_us=950000`, `sched_autogroup_enabled=1`, `mqueue msg_max=10`, `vm.mmap_rnd_bits=28`; `/proc/schedstat` still absent | `/proc/sys/...` | msg_max and schedstat unchanged from E5 and D-3. |

---

## 2. Toolchains

### 2.1 Existing LLVM/MLIR/Clang build at `LLVM/build`

**Fact (code/doc):** from `LLVM/build/CMakeCache.txt` and `llvm-config`.

| Property | Value |
|---|---|
| Revision | `1053047a4be7d1fece3adaf5e7597f838058c947` (2026-02-25, `main`), version `23.0.0git` (unreleased trunk). Source tree clean (`git status` empty). |
| Build type | `Release`; `LLVM_ENABLE_ASSERTIONS=ON` ("Optimized build with assertions"); `LLVM_ABI_BREAKING_CHECKS=WITH_ASSERTS` |
| RTTI / EH | `LLVM_ENABLE_RTTI=ON`, `LLVM_ENABLE_EH=OFF` |
| Projects | `LLVM_ENABLE_PROJECTS=mlir;clang`; `LLVM_ENABLE_RUNTIMES=openmp` (**no compiler-rt**) |
| Targets | `LLVM_TARGETS_TO_BUILD=host` → X86 only (**no BPF backend**) |
| Linking | static libraries (`BUILD_SHARED_LIBS=OFF`, no libLLVM dylib); also `libclang-cpp.so` and `libclang.so` (C API) |
| Plugins | `LLVM_ENABLE_PLUGINS=ON`, `CLANG_PLUGIN_SUPPORT=ON`; example plugins built (`lib/Bye.so`, analyzer sample plugins) |
| MLIR | `MLIR_ENABLE_BINDINGS_PYTHON=0`, `MLIR_ENABLE_EXECUTION_ENGINE=ON`; dialects include `llvm`, `smt`, `func`, `cf`, `scf`, `irdl`, `pdl` (`mlir-opt --help`); `libMLIRSMT.a`, `libMLIRExportSMTLIB.a`, `libMLIRTargetLLVMIRImport.a` present |
| Clang libraries | `clangTooling`, `clangASTMatchers`, `clangAnalysis*` (incl. FlowSensitive), `clangStaticAnalyzer*`, `clangCrossTU`, `clangIndex`, `clangRewrite` …. Built tools include `scan-build`, `clang-extdef-mapping` (cross-TU analysis), `clang-check` and `clang-refactor`. clang-tools-extra was not built, so there is no `clang-query` or `clang-tidy` in this build; system `clang-tidy-18` exists. |
| Not available | `LLVM_ENABLE_Z3_SOLVER=OFF`; no ClangIR (`CLANG_ENABLE_CIR` unset) |
| Host compiler used to build it | `/usr/bin/clang++` (system clang 18.1.3), generator Ninja |
| CMake packages | `lib/cmake/{llvm,mlir,clang}/` with `LLVMConfig.cmake`, `MLIRConfig.cmake`, `ClangConfig.cmake` |
| Headers | from the **source tree plus build tree** (`MLIR_INCLUDE_DIRS=LLVM/mlir/include;LLVM/build/tools/mlir/include`). Not an install. |

Inference: because headers come from the source tree, the tool depends on `LLVM/` staying at `1053047a` unmodified and present. The tool's provenance record must pin this commit. Trunk APIs change between versions, so the tool should not claim it builds against a released LLVM unless that is tested.

### 2.2 Out-of-tree consumption probes

**Fact (probe).** Out-of-tree projects were configured with `-DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++` (the same compiler that built LLVM) and `-DLLVM_USE_LINKER=lld`. `/usr/bin/ld.lld` (18) and `ld.gold` are present.

| Probe | Result | Time / size |
|---|---|---|
| `PROBE/cmake_find`: empty project, `find_package(MLIR)`, `find_package(Clang)`, `include(TableGen/AddLLVM/AddMLIR/AddClang)` | OK. Imported targets exist: `clangTooling`, `clangASTMatchers`, `clang-cpp`, `MLIRIR`, `MLIRParser`, `MLIRSMT`, `MLIRExportSMTLIB`, `MLIRTargetLLVMIRImport`, `MLIROptLib`, `LLVMPasses`. Reported: `LLVM_ENABLE_RTTI=ON`, `LLVM_ENABLE_EH=OFF`, `LLVM_ENABLE_ASSERTIONS=ON`, `CLANG_LINK_CLANG_DYLIB=OFF`. | configure 2.4 s |
| `PROBE/mlir_min`: ODS dialect `probe` (2 ops with assembly format) built with `add_mlir_dialect`/`add_mlir_dialect_library`, plus a tool that parses and walks a `.mlir` file | OK. The first attempt failed on my missing `#include "mlir/IR/Builders.h"` (my error, fixed). The LLVM CMake rule "one target per directory" applies (`LLVMProcessSources.cmake:113`). | full clean build 6.7 s wall (-j4); `probe-min` 3.4 MB |
| `probe-opt`: `MlirOptMain` + own dialect + `func` + `smt` + `ExportSMTLIB` | OK. Parses an `smt.solver` region; `mlir-translate --export-smtlib` emits `(declare-const …) (assert …) (check-sat)`. | link 6.5 s; 6.9 MB |
| `PROBE/clang_min`: LibTooling + ASTMatchers tool, linked statically | OK | build 29 s; **56.7 MB** |
| Same tool linked against `libclang-cpp.so` | OK at run time. No duplicate-option abort was observed. | build 16.5 s; 1.6 MB (needs `LLVM/build/lib` at run time) |
| LLVM pass plugin: `opt -load-pass-plugin=LLVM/build/lib/Bye.so -passes=goodbye` on IR of `cfe_sb_api.c` | OK (prints the functions) | — |
| `mlir-translate --import-llvm` of the IR of `cfe_sb_api.c` (clang -O0, `-disable-O0-optnone`) | OK: 343 KB of LLVM-dialect MLIR, `llvm.call @CFE_ES_GetAppID` … | 2.4 s cold |
| libclang Python bindings (`LLVM/clang/bindings/python` + `LLVM/build/lib/libclang.so`, `python3 -I`) | OK: parses `cfe_sb_api.c` through the compile DB; 247 call expressions | — |

Inference: an out-of-tree project (dialect + passes + LibTooling extractor) iterates in seconds to tens of seconds per rebuild on this machine. Linking LibTooling statically costs about 57 MB per binary; one driver binary avoids repeating that. Static linking is the safer default: the dylib variant worked, but mixing static LLVM components with `libclang-cpp.so` (which embeds LLVM, because `LLVM_LINK_LLVM_DYLIB=OFF`) is a known source of duplicate-registration trouble (inference, not observed here).

### 2.3 System clang/llvm 18 (runtime only)

**Fact (probe/package db):**

- Installed: `clang-18` 1:18.1.3, `llvm-18`, `llvm-18-runtime`, `clang-tools-18`, `clang-tidy-18`, `clang-format-18`, `libclang-cpp18`, `libclang1-18`, `libclang-common-18-dev`, `lld-18`, `lldb-18`.
- **Not installed:** `llvm-18-dev`, `libclang-18-dev`, `libmlir-18-dev`, `mlir-18-tools` (all apt candidates at 1:18.1.3).
- System `llc-18` has the **BPF backend** (`bpf`, `bpfel`, `bpfeb`). It can compile eBPF C programs, but `libbpf` headers (`/usr/include/bpf`) are absent.
- No compiler-rt for clang anywhere: `/usr/lib/llvm-18/lib/clang/18/lib/linux` and `LLVM/build/lib/clang/23/lib` do not exist. Clang's TSan, XRay and SanitizerCoverage runtimes are therefore unavailable without building compiler-rt.

### 2.4 Solvers and verifiers

**Fact (probe):** Not installed: `z3` (binary and `python3 -c "import z3"`), cvc5, yices, boolector, bitwuzla, minisat, cbmc, esbmc, frama-c, cppcheck, infer.

apt candidates:

| Package | Candidate |
|---|---|
| `z3`, `libz3-dev`, `python3-z3` | 4.8.12 |
| `cvc5` | 1.1.2 |
| `cbmc` | 5.95.1 |
| `frama-c-base` | 20220511 |
| `cppcheck` | 2.13.0 |

Also present: Python 3.11.15 and pip 24.0.

Inference: without an install, a solver-backed exploration can only *emit* SMT-LIB (MLIR `smt` → `--export-smtlib`) or use a hand-written bounded explorer. Installing `z3`/`libz3-dev` is a PI decision. The 4.8.12 candidate is old compared with the Z3 that current MLIR SMT lowering targets (UNVERIFIED compatibility).

### 2.5 Reference compiler

**Fact (record + probe):**

- `/usr/bin/cc` → `x86_64-linux-gnu-gcc-13`, **gcc 13.3.0 (Ubuntu 13.3.0-6ubuntu2~24.04.1)**. That is the compiler of the reference build (B2, `CONDITIONS.md` B-table; `REC/env/logs/build_04_prep.log` L6 "The C compiler identification is GNU 13.3.0").
- The gcc plugin directory `/usr/lib/gcc/x86_64-linux-gnu/13/plugin` has only `libcc1plugin`/`libcp1plugin`, **no `include/`**. `gcc-13-plugin-dev` is not installed (candidate 13.3.0-6ubuntu2~24.04.1).
- gcc sanitizer runtimes are installed: `libtsan.so.2`, `libasan.so.8`, `libubsan`, `libhwasan`. A gcc `-fsanitize=thread` probe reported the expected data race (`PROBE/tsan`).
- valgrind 3.22.0 with helgrind and drd. gdb 15.1. strace present. ltrace, perf, bpftool, bpftrace and rr are absent (apt candidates: `bpftrace` 0.20.2, `rr` 5.7.0, `linux-tools-generic` 6.8.0-142 — built for a 6.8 kernel, not 6.18).

---

## 3. cFS build integration

### 3.1 The compile database already exists

**Fact (code/doc):**

- `CFS/target-rules.mk:33` reads `$(ALL_TARGETS): PREP_OPTS += -DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE`.
- The effective CMake invocation recorded in C3 (`REC/env/CONDITIONS.md:74`) contains it.
- Files present (owner `ubuntu`, 2026-10-08 01:15):
  - `BLD/compile_commands.json`: mission/host tools, 20 KB.
  - `BLD/native/default_cpu1/compile_commands.json`: 2.6 MB, **1417 entries**, compiler `/usr/bin/cc` for all of them.
  - `BLD/native/default_cpu2/compile_commands.json`: same size.

Nothing has to change in the build.

**Fact (probe), composition of the cpu1 database:**

- 276 coverage entries (`-pg` with `--coverage` or `-fprofile-arcs -ftest-coverage`).
- 933 entries with `-D_UNIT_TEST_`.
- 159 entries with `-fPIC` (apps and libs).
- **448 entries with neither `-D_UNIT_TEST_` nor `-pg`** (428 distinct files). I call these "FSW" below.
- The FSW set still contains non-flight targets: `cfe_testcase` (33), `*_headercheck` (69), OSAL `MODULE0..20` test modules, `ut_assert_pic`, and table objects (`tblobj_*`).
- **Target membership must therefore be taken from the CMake target of each entry** (`CMakeFiles/<target>.dir` in `output`), or better from `link.txt`, not from a path heuristic.

### 3.2 Flags

**Fact (probe; matches C5, `CONDITIONS.md:76`):**

- Every entry has `-g -Wstrict-prototypes -Wwrite-strings -Wpointer-arith -Wno-format-overflow -Wno-format-truncation -Wno-stringop-overflow -Wno-stringop-truncation -std=c99 -pedantic -Wall -Werror -DSIMULATION=native -D_LINUX_OS_`.
- `-D_POSIX_OS_ -D_XOPEN_SOURCE=600` on 1357 entries. `-D_CFE_CORE_` on 269, `-D_CFE_PSP_` on 190.
- No `-O` (gcc default `-O0`).
- Sources: `sample_defs/arch_build_custom.cmake:28-33` (`-std=c99 -pedantic -Wall -Werror`) and `sample_defs/global_build_options.cmake:40-48` (`add_compile_options_if_available` of the `-W` set).

### 3.3 Clang parses the cFS sources with the recorded flags

**Fact (probe)**, on `cfe/modules/sb/fsw/src/cfe_sb_api.c` with its exact recorded command and `-fsyntax-only` (outputs in `PROBE/syntax/`):

| Compiler | Result |
|---|---|
| gcc (`/usr/bin/cc`) | rc 0 |
| clang-18 and clang-23 | **rc 1**: `unknown warning option '-Wno-stringop-overflow'` and `'-Wno-stringop-truncation'` turned into errors by `-Werror` |
| clang-23 + `-Wno-unknown-warning-option` | rc 0 |

**Fact (probe): all 448 FSW commands through LibTooling** (`PROBE/clang_min/build/bin/probe-calls -p PROBE/cdb_fsw`, with `--extra-arg=-Wno-unknown-warning-option --extra-arg=-resource-dir=LLVM/build/lib/clang/23`):

- Every TU produced an AST. 10.0 s wall, single thread.
- With `-Werror` kept, clang reports 3 diagnostics that gcc does not:
  - `apps/cf/fsw/src/cf_codec.c:169` and `:239`, `-Wunused-function` (`CF_Codec_Store_uint64`, `CF_Codec_Load_uint64`);
  - `apps/cs/fsw/src/cs_table_processing.c:932`, `-Wparentheses-equality`.
- With `--extra-arg=-Wno-error` the tool returns rc 0.
- The `-resource-dir` argument is needed because an out-of-tree tool does not find the clang builtin headers by itself.

Inference: the analysis front end should apply exactly two documented adaptations to the recorded commands: `-Wno-unknown-warning-option` and `-Wno-error`, plus `-resource-dir`. These change diagnostics only, not semantics. The bundle itself anticipates clang: it ships `CFS/sample_defs/toolchain-i686-linux-clang.cmake:12-13` (`/usr/bin/clang`), and `add_compile_options_if_available` filters options the compiler lacks. Bundle CI runs cppcheck and CodeQL on the sources: `CFS/.github/workflows/app-static-analysis-reusable.yml:30-31` and `codeql-reusable.yml:147`.

### 3.4 IR and MLIR for whole link units

**Fact (probe):**

- **Bitcode:** clang-23 with `-O0 -Xclang -disable-O0-optnone -emit-llvm`, `-P4`, from the recorded commands: **448/448 TUs** compiled. One TU first failed only because my `xargs` layer stripped the quotes around `-DCFE_CPU_NAME_VALUE="cpu1"`; run directly, it succeeded. Time **4.4 s** wall, 8.9 MB of `.bc`.
- **Linking TUs naively fails:**
  - linking all TUs: `CFE_TBL_FileDef` multiply defined (one per table object);
  - linking only the core members: `OS_Application_Startup` multiply defined. The PSP defines it, and so does `osal/src/bsp/shared/src/bsp_default_app_startup.c`, a default that `libosal_bsp.a` supplies only on demand.
- **Archive semantics come from the link command.** `BLD/native/default_cpu1/cpu1/CMakeFiles/core-cpu1.dir/link.txt` links:
  - `config es evs fs sb tbl time osal psp-pc-linux msg sbr resourceid` with `-Wl,--whole-archive`;
  - then `libosal_bsp.a` and the PSP module archives lazily.
- **Emulating those semantics works:** whole-archive members with `llvm-link`, then `llvm-link --only-needed` for the 4 lazy BSP members. The result is a closed `core-cpu1.bc` (2.3 MB, 0.35 s): 1222 defined functions, **0 undefined `CFE_`/`OS_` symbols**. The gcc binary exports 1212 `T` symbols.
- **MLIR import:** `mlir-translate --import-llvm core-cpu1.bc` gives 12.5 MB of MLIR in **0.78 s**.

Inference: whole-program static facts per link unit (core plus each app `.so`) are cheap here. The extractor must take module membership and archive semantics from `link.txt` (or the CMake File API), not from directory names. Analysing apps needs the core as the symbol provider, because app `.so` files resolve `CFE_`/`OS_` symbols from the executable (§4.1).

### 3.5 Reproducibility of the gcc build (baseline for measuring fidelity)

**Fact (probe):** I recompiled `cfe_sb_api.c` with the recorded gcc command into scratch. Its `.text` is **byte-identical** to `BLD/native/default_cpu1/sb/CMakeFiles/sb.dir/fsw/src/cfe_sb_api.c.o`.

Inference: each instrumented variant can therefore be diffed against the reference at object level. This turns "fidelity to the reference build" into a measurable quantity (changed functions and bytes per variant).

---

## 4. Instrumentation mechanisms that keep gcc

### 4.1 How the reference binary is linked and loaded

**Fact (code/doc + probe):**

- `BLD/exe/cpu1/core-cpu1`:
  - `ELF 64-bit LSB pie executable … with debug_info, not stripped`;
  - `NEEDED libc.so.6` only;
  - `FLAGS BIND_NOW`, `FLAGS_1 NOW PIE`.
- The link (`link.txt` above) uses `-Wl,--export-dynamic -rdynamic`, whole-archive cFE core, OSAL and PSP, and `-lpthread -ldl -lrt`. So **cFE, OSAL and PSP are statically inside the executable, and all their symbols are exported** (1212 `T` dynamic symbols, e.g. `CFE_SB_TransmitMsg`, `CFE_EVS_SendEvent`, `OS_QueuePut`, `OS_MutSemTake`, `CFE_ES_WaitForStartupSync`).
- Apps and libs are `.so` files in `BLD/exe/cpu1/cf/` (sch_lab, ci_lab, to_lab, sample_app, sample_lib, lc, cf, ds, fm, hk, hs, mm, sc, md, cs, sbn, sbn_udp, sbn_f_remap, cfe_assert, cfe_testcase). Each has `NEEDED libc.so.6` only and **14–180 undefined `CFE_*`/`OS_*` symbols** (e.g. cf.so 46, to_lab.so 35). These resolve against the executable at load time.
- OSAL loads them with `dlopen(path, RTLD_NOW | RTLD_GLOBAL)` unless the module is flagged local (`CFS/osal/src/os/portable/os-impl-posix-dl-loader.c:77`, `:86`, `:96`, `:100`).

### 4.2 GNU ld `--wrap` (API-boundary interposition)

**Fact (probe, `PROBE/wrap`):** a PIE "core" built with gcc 13 and the cFS flag set, linked with `-Wl,--export-dynamic -rdynamic -Wl,--wrap=api_send`, plus `dlopen` of an app `.so`.

| Caller | Wrapped? |
|---|---|
| Call inside the TU that defines `api_send` | **No.** The object has an `R_X86_64_PLT32 api_send` relocation, but the symbol is defined in that object, and `--wrap` applies only to undefined references. |
| Call from another TU of the executable | Yes |
| App `.so` linked **without** `--wrap` | **No** (binds to the exported real `api_send`) |
| App `.so` linked **with** `-Wl,--wrap=api_send` | Yes (binds to `__wrap_api_send` exported by the executable) |

**Fact (probe), size of the gap in cFS.** The LibTooling probe over the 394 FSW files, excluding `cfe_testcase`, counts call sites whose callee is first declared in `cfe/modules/core_api/fsw/inc/` or `osal/src/os/inc/`, excluding `static` (inline) callees:

- **3,762 public-API call sites.**
- **88 (2.3%) intra-TU**, so not wrappable. Top: `CFE_ES_WriteToSysLog` 21, `OS_close` 3, `OS_OpenCreate` 2, `OS_ObjectIdToArrayIndex` 2, `CFE_TBL_ReleaseAddress` 2, `CFE_SB_GlobalTlmTopicIdToMsgId` 2, `CFE_SB_GetPipeName` 2, `CFE_ES_PoolCreateEx` 2, then single OSAL file/socket/timed-I/O helpers.
- 110 indirect calls (function pointers).
- 4,368 call sites go to `static inline` helpers in public headers. These are not linker symbols and cannot be wrapped, but every one I found is a pure value conversion:
  - `CFS/osal/src/os/inc/osapi-idmap.h:81,103,130,151`;
  - `osapi-clock.h:225-624` (22 time helpers);
  - `osapi-error.h:166`;
  - `cfe/modules/core_api/fsw/inc/cfe_resourceid.h:129,148,160,178`;
  - `cfe_sb.h:796,827,856`;
  - `cfe_tbl.h:57,65,73`.

Inference:

- `--wrap` is a sound boundary mechanism for **cross-module** effects. That covers all app→cFE/OSAL calls and cFE-module→OSAL calls.
- `--wrap` cannot see intra-TU calls, and it cannot see what happens inside cFE beyond the API (e.g. ES internal table writes). Those need callee-side hooks: probes at the definition, or source instrumentation.
- Practical cost: add `-Wl,--wrap=<sym>` (one per symbol) to **both** the `core-cpu1` link and every app/lib link, and add one object that defines the `__wrap_*` symbols to `core-cpu1`. This is a link-flag change in a **variant configuration**, i.e. a `deviation` under `CONDITIONS_POLICY.md`. The sources and the compiler stay as they are. The only reference-documented hook for adding options is the `ENABLE_ASAN` pattern (`CFS/sample_defs/global_build_options.cmake:32-36`). A variant would most cleanly be a copy of `sample_defs` (a different `MISSIONCONFIG`) with link options added. `build_cfs.sh` refuses `CFLAGS`/`LDFLAGS`/`CMAKE_*` environment variables (`REC/env/build_cfs.sh:141-146`), so that route is closed by policy.

### 4.3 `LD_PRELOAD` and `LD_AUDIT`

**Fact (probe):** an `LD_PRELOAD` library defining `api_send`, with the unwrapped PIE core plus a dlopen'ed app. **None** of the three call paths reached the preload: intra-TU, cross-TU and app→core. The executable's own definition wins.

**Fact (code/doc):** `REC/env/run_cfs.sh:154-158` refuses to start when any `LD_*`, `MALLOC_*` or `GLIBC_TUNABLES` variable is set. `build_cfs.sh:146` refuses them as well.

Inference:

- `LD_PRELOAD` could intercept only OSAL→libc calls (`pthread_*`, `mq_*`, `sem_*`), which are resolved through PLT against libc. It can never intercept the cFE/OSAL API, because that API lives in the executable.
- `LD_AUDIT` (`la_symbind64`) could in principle redirect app→core bindings, since those are dynamic. It is also an `LD_*` variable and refused.
- Both are therefore unsuitable as the primary mechanism.

### 4.4 gcc code-generation hooks

**Fact (probe, `PROBE/gccinst`)**, recorded command for `cfe_sb_api.c` plus one option each, all under `-Werror`:

| Option | Compiles? | `.text` | Notes |
|---|---|---|---|
| reference | yes | 13,100 B | identical to the build-tree object |
| `-finstrument-functions` | yes | 15,151 B (**+15.7%**) | 36 `__cyg_profile_func_enter` call sites; adds calls on every function, including inlined header helpers |
| `-fpatchable-function-entry=16,0` | yes | 13,676 B (+4.4%), +288 B data (`__patchable_function_entries`) | inert NOPs; enables runtime patching |
| `-fcallgraph-info=su,da` | yes | unchanged | writes a VCG call graph `.ci` with stack usage and dynamic allocation; a gcc-side static fact source |

Inference:

- `-finstrument-functions` changes every function and gives only function-level events with no arguments. That is too coarse for resource-level events and perturbs timing everywhere.
- `-fpatchable-function-entry` is the least invasive compile-time option for later runtime patching, but it needs a patcher.
- `-fcallgraph-info` is useful for cross-checking clang-derived call graphs against the reference compiler.

### 4.5 gcc plugins

**Fact (probe):** not available. Plugin headers are absent and `gcc-13-plugin-dev` is not installed (§2.5).

Inference: a GIMPLE-level plugin would be the highest-fidelity compile-time instrumenter that keeps gcc. It needs a package install (PI decision) and plugin code written against GCC 13 internals, which are not a stable API.

### 4.6 Source-to-source rewriting (Clang tool, then gcc)

**Fact (probe):**

- Clang-23 LibTooling parses every FSW TU (§3.3). `clangRewrite` and `clangToolingCore` are available.
- **467 of the 3,762 API call sites (12.4%) lie inside macro expansions**, where a call-site rewriter cannot place text without rewriting the macro.
- The gcc flag set is strict. My own scratch harness failed under `-pedantic` ("ISO C forbids conversion of object pointer to function pointer type") until I rewrote a `dlsym` cast.

Inference:

- Feasible, and it keeps gcc as the compiler.
- Generated code must be clean ISO C99 under `-pedantic -Wall -Werror`.
- Macro-expansion sites force rewriting at the definition side (callee entry), not the call site.
- Fidelity: the source differs at every instrumented site. The diff is reviewable and can be measured per object (§3.5).

### 4.7 Out-of-process observation: ptrace, watchpoints, uprobes, eBPF, tracepoints

**Fact (probe, `PROBE/kprobe`).** The observer was root and the targets were gcc-built PIEs. The uid-1000 case used `/bin/sleep`, because uid 1000 cannot reach the 0700 scratch directory.

| Mechanism | Available | Measured |
|---|---|---|
| ptrace (`PTRACE_SEIZE` by root of a uid-1000 process; gdb 15.1) | **yes** (no Yama) | gdb breakpoint at `func+4` with `commands; silent; continue`: **≈203 µs per hit** (2,000 hits). R2 under gdb: OPERATIONAL at 2244–2693 ms vs 1613–1636 ms without gdb (`REC/results/R2_gdb_v701.md:32`). |
| HW watchpoints | **yes**: gdb "Hardware watchpoint"; perf `PERF_TYPE_BREAKPOINT` (PMU `breakpoint`, type 5) | perf write-watchpoint on a 4-byte global: **≈34 µs per hit** (1.7 ns without). x86 has 4 debug-address registers (fact of the architecture), so at most 4 watched locations per thread at a time. |
| uprobes through the perf PMU (`/sys/bus/event_source/devices/uprobe`, type 8), **without tracefs** | **yes**. Works on a uid-1000 process; registration before the file is mapped (child blocked before `exec`) still counted all hits. | At `api_send+0` (`endbr64`): **31.7 µs/hit**. At `+4` (`push %rbp`): **0.57 µs/hit**. At `+5` (`mov %rsp,%rbp`): **31.7 µs/hit**. Baseline call: 2.6 ns. |
| eBPF | **yes**. `bpf(BPF_PROG_LOAD)` of socket-filter and kprobe-type programs works; `PERF_EVENT_IOC_SET_BPF` attaches to a uprobe. BTF at `/sys/kernel/btf/vmlinux`. `CONFIG_BPF_SYSCALL`, `CONFIG_BPF_JIT`, `CONFIG_BPF_EVENTS`, `CONFIG_UPROBE_EVENTS`, `CONFIG_DEBUG_INFO_BTF` are set. | No userland tooling: no libbpf headers, bpftool or bpftrace. System clang-18 can compile BPF; the LLVM-23 build cannot (host target only). |
| tracefs / debugfs | not mounted (`/sys/kernel/{tracing,debug}` empty); **mountable** by root inside a private mount namespace | events include `sched_switch`, `sched_wakeup`, `sched_waking`, `sched_process_fork/exec/exit`, `syscalls/sys_enter_mq_timedsend`, `sys_enter_futex*`. Tracers: `blk function nop`. |
| kprobes | **no** (`# CONFIG_KPROBES is not set`) | — |
| Hardware PMU counters | **no**: `PERF_TYPE_HARDWARE` instructions returns ENOENT; sysfs PMUs are only `breakpoint msr power software tracepoint uprobe` | rr-style replay is unavailable (rr's dependency on HW counters is UNVERIFIED here, §10) |
| Kernel modules | **no** (`nomodule`) | SystemTap and custom LKMs ruled out |
| valgrind 3.22 (helgrind, drd) | yes | a trivial threaded program ran under helgrind in 1.5 s |

Inference:

- The cost cliff comes from the single-step and debug exception path (#DB), which is expensive in this KVM/Firecracker guest. int3 plus instruction emulation is cheap.
- gcc on Ubuntu emits `endbr64` at function entry (seen in the probe disassembly), and at `-O0` every function starts `endbr64; push %rbp; mov %rsp,%rbp`. So **probes must be placed at entry+4**. That gives function-entry uprobes at about 0.6 µs.
- Return probes (uretprobes) and probes in the middle of a function must be checked instruction by instruction. Watchpoints (34 µs) suit only targeted confirmation of a few variables, not broad tracing.
- These mechanisms need **no change to the binary, the build, the environment or the run user**. The observer runs as root alongside a README-mode `ubuntu` process. That is the highest-fidelity option available.
- The remaining perturbation is timing (traps), as with gdb in R2. It must be measured per configuration, like H-8 (`CONDITIONS.md:136`).
- Inference: the uprobe PMU needs `CAP_PERFMON` or `CAP_SYS_ADMIN`, so the observer must stay in the root session. `run_cfs.sh` would need a hook for a root-side observer next to the existing `RUN_CFS_WRAP`/`RUN_CFS_HOOK` (H-7, H-8).

### 4.8 Sanitizers

**Fact (probe/code):**

- gcc `-fsanitize=thread` works on this kernel (`mmap_rnd_bits` 28).
- The reference sample config already has an opt-in sanitizer knob: `ENABLE_ASAN` → `-fsanitize=address` (`CFS/sample_defs/global_build_options.cmake:32-36`). TSan has no such knob.

Inference: TSan finds memory-level data races between pthreads. It does not model OSAL- or cFE-level order (publish-before-use across tasks with no shared-variable race). It is a baseline or complement, not the detector. It changes code generation heavily (deviation).

---

## 5. Schedule control

### 5.1 Facts

**Fact (probe), README-mode user (`setpriv --reuid=1000 --regid=1000 --init-groups`):**

| Action | Result |
|---|---|
| `chrt -f 1 true` | `Operation not permitted` |
| `nice -n -1` | `Permission denied` |
| `nice -n 5` | OK |
| `taskset -c 0` | OK (`Cpus_allowed_list: 0`) |
| `ulimit -r` | 0 |

**Fact (probe), root acting on a uid-1000 process:**

- `taskset -p -c 0 <pid>` worked, and `chrt -f -p 1 <pid>` worked: the policy became `SCHED_FIFO`, priority 1.
- RT throttling: `cpu.rt_runtime_us 950000` in the root cpu cgroup (`CONFIG_RT_GROUP_SCHED=y`).

**Fact (probe), cgroups:**

- v1 hierarchies `cpu`, `cpuacct`, `cpuset`, `memory`, `devices`, `freezer`, `blkio`, `pids` and `name=systemd` are mounted rw; cgroup2 is at `/sys/fs/cgroup/unified`.
- The session runs in `cpu:/` and `cpuset:/` (cpus 0-3); memory/cpuacct are in `process_api/...`.
- Freezer and per-thread placement were **not** tested (doing so would create cgroups, i.e. change host state).

**Fact (code/doc), what OSAL does:**

- At startup OSAL tries `pthread_setschedparam(pthread_self(), SCHED_FIFO/RR, …)` and sets `EnableTaskPriorities = true` only if that succeeds (`CFS/osal/src/os/posix/src/os-impl-tasks.c:409-421`).
- Only then does task creation use `PTHREAD_EXPLICIT_SCHED` with a policy and priority (`:523-561`). Otherwise the attributes stay at their defaults, i.e. inherited (`pthread_create` at `:566`).
- In README mode all 27 threads run `SCHED_OTHER`, nice 0 (M1-5, `CONDITIONS.md:150`; E4 `:105`).

**Fact (probe), handoff latency of a cooperative gate.** A semaphore ping-pong between two pthreads measured one-way handoff at **10.5 µs** across CPUs (4 CPUs allowed) and **1.1 µs** when pinned to CPU 0.

**Fact (kernel config):** `CONFIG_TIME_NS=y` (clock offsets per namespace, no freezing). `CONFIG_SCHED_CLASS_EXT` is not set, so there is no sched_ext.

### 5.2 Feasibility of the schedule-control families (inference unless stated)

| Family | Works here? | Condition cost |
|---|---|---|
| **Native PCT**: strict priorities on one CPU, random priorities plus d−1 change points (PCT as described in report 04/05) | Only with a **root controller** that pins all cFS threads to one CPU and assigns `SCHED_FIFO` priorities from outside. New OSAL threads inherit the creator's policy in README mode (`os-impl-tasks.c:523-561`), so each new thread must be re-prioritised as it appears, e.g. via `PTRACE_O_TRACECLONE` or the `sched_process_fork` tracepoint. Changing priorities at change points is a root `sched_setscheduler` call. | Deviates from E1 (`Cpus_allowed_list 0-3`, `CONDITIONS.md:102`) and E4 (`SCHED_OTHER`). It is "flight-like" in spirit (strict priorities), but it is not README mode. Blocking calls with timeouts (OSAL timed waits, `OS_TaskDelay` polling) still let real time decide some orders. PCT's guarantee assumes the controller sees every scheduling point. |
| **Cooperative gates at instrumentation points** (CHESS-style interception at the OSAL/cFE boundary) | Yes, through `--wrap` (§4.2) or source instrumentation (§4.6). The gate blocks the calling task until the controller releases it. | No privilege needed. Keeps the run user and conditions. Adds 1–10 µs per gated event plus the controller's decision time. Risks: deadlock if a gate blocks while a program lock is held (design v0.1 §10 already notes this); real-time timeouts expiring while a task is held. Inference: timeouts must either be modelled as nondeterministic choices the controller makes explicitly, or the gate must account for them. Time namespaces cannot freeze time. |
| **ptrace-based scheduler** (stop and resume individual threads at breakpoints) | Yes (per-thread ptrace stops; root and uid-1000 both OK) | No change to the binary. Each decision costs a ptrace round trip: gdb measured about 203 µs per breakpoint; a purpose-built tracer should be cheaper, not measured. Stepping over breakpoints uses the expensive #DB path (§4.7). |
| **Freezer cgroup / SIGSTOP** | SIGSTOP stops the whole process, not one thread. A per-thread v1 freezer is plausible but untested. | Coarse; untested |
| **rr chaos mode / record-replay** | **No**: no HW PMU (§4.7) | — |
| **sched_ext** | No (`CONFIG_SCHED_CLASS_EXT` not set) | — |

---

## 6. Resource budget

| Resource | Fact | Inference for the tool |
|---|---|---|
| Disk | 18 GB free on `/` | Comfortable. One full instrumented cFS variant (source copy plus `build-native_std`) cost about 0.8 GB (`cfs_ref` 782 MB; the retired CORD copy was 517 MB, `HOST_CLEANUP_20261012.md`). |
| RAM | 15.7 GB, no swap | LibTooling over all TUs, `llvm-link`, MLIR import: all small. Building compiler-rt (for clang TSan/XRay) is untested. |
| CPU | 4 vCPU | Shared host: "other workloads … share the host, so timings are not controlled" (B3, `CONDITIONS.md:64`). |
| cFS full build | prep 126.6 s + install 794.7 s, serial, as README L104-105 prescribe (`REC/env/logs/build_00_steps.log` L8, L10; C2) | Each build variant costs about 15 min serial. A link-flag-only variant still goes through the full CMake flow unless the targets are relinked by hand. |
| Out-of-tree MLIR project | configure about 4 s; dialect + tool 6.7 s; `mlir-opt`-style driver 6.5 s; binaries 3–7 MB | Edit-build-test loops in seconds |
| LibTooling extractor | 29 s build (56.7 MB static) / 16.5 s (1.6 MB dylib); 10 s for all 448 FSW TUs | Full extraction runs in seconds |
| IR pipeline | 4.4 s bitcode for 448 TUs (-P4); 0.35 s core link; 0.78 s MLIR import | Negligible |
| Scratch used by this report | 96 MB | under the 200 MB cap |

---

## 7. Feasibility matrix

Fidelity scale, relative to the reference (gcc 13.3.0, recorded flags, README mode):

- **F0**: unmodified binary and conditions; only timing is perturbed.
- **F1**: same sources and compiler; link flags or a code-generation option changed.
- **F2**: same compiler; sources rewritten at the instrumented sites.
- **F3**: a different compiler (clang) for the executed binary.
- **A**: analysis only, nothing executed.
- "+dev" marks a change of run condition that must be recorded as a `deviation`.

| Mechanism | Available now? | Cost (setup / per event) | Fidelity | Main risks |
|---|---|---|---|---|
| Compile DB from the reference build | **Yes**, already exported (`target-rules.mk:33`) | none | A | Target membership must come from `link.txt`, not paths |
| Clang-23 LibTooling and AST matchers on cFS | **Yes** (448/448 TUs) | build 17–29 s; 10 s per full pass | A | Needs `-Wno-unknown-warning-option`, `-Wno-error`, `-resource-dir`. Clang-vs-gcc semantic differences are possible in implementation-defined areas (inference). LLVM trunk API drift. |
| Clang IR (LLVM bitcode) per link unit | **Yes** (core links closed) | seconds | A | Archive semantics (whole-archive vs lazy) must be replicated. IR is clang's, not the gcc binary's. |
| MLIR: import of LLVM IR plus a custom dialect plus passes | **Yes** | seconds per build | A | Build-tree (not installed) MLIR from source; Python bindings off |
| MLIR SMT dialect → SMT-LIB text | **Yes** | — | A | No solver to discharge it |
| Z3 / cvc5 / CBMC | **No** (apt candidates exist) | install = PI decision | A | Z3 4.8.12 is old |
| LLVM pass plugins (`opt -load-pass-plugin`) | **Yes** | — | A (or F3 if used for execution) | Executing clang-built code is F3 |
| clang TSan / XRay / SanCov | **No** (no compiler-rt) | building compiler-rt: time and space unmeasured | F3 | Executing a clang build departs from the reference compiler |
| gcc `-fcallgraph-info`, `-fdump-*` | **Yes** | negligible | A (reference compiler) | Coarse graphs: indirect calls marked, not resolved |
| ld `--wrap` at the core and app links | **Yes** (semantics probed) | variant build about 15 min; a few ns plus handler per call | F1 (+dev: link options) | Misses 88/3,762 intra-TU API sites (2.3%) and everything below the API. Every app must be relinked. The variant config must be recorded. |
| `LD_PRELOAD` / `LD_AUDIT` | Technically partly; **refused by policy** | — | F0 (+dev: environment) | Cannot interpose executable-defined API symbols (probe); `run_cfs.sh:154-158` refuses |
| `-finstrument-functions` | **Yes** | variant build; one call per function entry and exit | F1 | +15.7% `.text` in SB. Function-level only, no arguments. Perturbs every function. |
| `-fpatchable-function-entry` | **Yes** | variant build plus a runtime patcher to write | F1 (NOPs only) | Needs a self-written patcher; small code-layout change |
| gcc plugin (GIMPLE) | **No** (headers missing; apt candidate) | install plus plugin code | F1/F2-like | GCC-internal API, version-specific |
| Source-to-source (Clang rewrite → gcc) | **Yes** | tool plus variant build | F2 | 12.4% of API sites are in macro expansions. Strict `-pedantic -Werror`. Every site is a source diff. |
| ptrace / gdb | **Yes** | about 203 µs per gdb breakpoint hit | F0 (timing only) | Heavy perturbation (R2: +0.6–1.1 s to OPERATIONAL) |
| HW watchpoints (perf or gdb) | **Yes** | about 34 µs per hit; 4 slots | F0 | Only for a few targeted variables |
| uprobes through perf PMU (root) | **Yes** (also on uid 1000; pre-registration works) | 0.57 µs/hit at entry+4; 31.7 µs at `endbr64` or non-emulated instructions | F0 | Probe placement must avoid the #DB path. Root-side observer needs a harness hook. Probes on apps' `.so` work by file and inode (pre-registration probed on the main binary only). |
| eBPF on uprobes and tracepoints | **Yes** (raw `bpf()`); no libbpf/bpftool/bpftrace | write own loader, or install bpftrace/libbpf (PI decision) | F0 | Tooling effort; BPF code needs system clang-18 (LLVM-23 build lacks the BPF target) |
| Kernel tracepoints (`sched_switch`/`sched_wakeup`, `syscalls`) | **Yes** (tracefs mountable in a private namespace; perf tracepoint PMU) | low (not measured) | F0 | Shows OS-level scheduling only, not cFE semantics |
| kprobes | **No** | — | — | — |
| rr / HW-counter record-replay | **No** (no PMU) | — | — | — |
| valgrind helgrind/drd | **Yes** | large slowdown (not measured on cFS) | F0-binary, but valgrind serialises threads | Serialised scheduling is not README-mode scheduling. Memory-level only. |
| gcc TSan | **Yes** | variant build; large slowdown (not measured on cFS) | F1 (heavy code-generation change) | Memory-level races only; not the process-level order target |
| Native PCT (root `SCHED_FIFO` plus single-CPU affinity) | **Technically yes** (root can impose it on uid-1000 threads) | controller plus per-thread re-prioritisation | F0-binary **+dev** (E1 affinity, E4 policy) | Not README mode. Real-time timeouts. RT throttling 950 ms/s. |
| Cooperative gates at the OSAL/cFE boundary | **Yes** (through `--wrap` or rewriting) | 1.1 µs (same CPU) to 10.5 µs (cross CPU) per handoff plus controller | F1/F2 | Deadlock if gated while holding a lock; timeouts; misses events below the boundary |
| ptrace-based thread scheduler | **Yes** | about 0.2 ms per decision with gdb; custom tracer unmeasured | F0 | Slow; #DB stepping cost |
| cgroup v1 freezer per thread | Mounted; **untested** | — | F0 | Untested; would change host cgroup state |
| Raising `msg_max` / root mode | Possible for root (`CONDITIONS.md` M1-9, D-4) | — | +dev (VM-wide sysctl) | Already a pending PI decision (D2); root mode needs `CAP_SYS_RESOURCE`, which the container removes |

---

## 8. What this implies for the architecture candidates (inference)

1. **The static front end can be built on the existing tools with no build change.**
   - Use the cpu1 compile DB, Clang-23 LibTooling for source-level facts (call sites, arguments, configuration constants, macro provenance), and LLVM IR per link unit (assembled with `link.txt` archive semantics) for memory and call facts.
   - Lift them into a small out-of-tree MLIR dialect. All of this is measured to run in seconds.
   - Use gcc's `-fcallgraph-info` to cross-check the clang-derived call graph against the reference compiler.
2. **Observation runs should use F0 mechanisms first:** root-side uprobes, eBPF and tracepoints on the unmodified README-mode binary.
   - Probes go at entry+4 of the `CFE_*`/`OS_*` boundary functions, and at the definition side for the 88 intra-TU cases.
   - Only this choice keeps the reference binary *and* the run conditions unchanged.
   - The perturbation is about 0.6 µs per probed call. R2 alone had 24 relevant events per boot (`REC/results/R2_gdb_v701.md`), so a per-boot overhead well under 1 ms is plausible for targeted probes. That is two to three orders of magnitude below gdb.
   - Needs a harness hook and a recorded per-configuration timing check.
3. **Controlled (verification) runs need an F1 variant with cooperative gates at the OSAL/cFE boundary**, via `--wrap` on core and app links, recorded as a link-option deviation. A ptrace scheduler is the F0 alternative but is about 100× slower per decision.
   - Native PCT is possible only as a stated deviation (root-imposed `SCHED_FIFO` on one CPU). If PCT's probabilistic guarantee is wanted, the cleaner route is PCT-style priorities enforced *by the gates*, not by the kernel. Report 05's recommendation of scheduling points at the OSAL boundary is consistent with this.
4. **Do not plan on:** rr or HW-counter replay, kprobes, kernel modules, `LD_PRELOAD`/`LD_AUDIT`, clang-runtime instrumentation (TSan/XRay) without first building compiler-rt, or a solver backend before a PI install decision.
5. **Provenance obligations the tool must record for every run:**
   - LLVM commit `1053047a`, gcc 13.3.0, kernel version (now `-fc-v114`);
   - mechanism and fidelity class (F0–F3), probe placements (offsets);
   - variant link options;
   - the two clang adaptations (`-Wno-unknown-warning-option`, `-Wno-error`).

---

## 9. Open questions (for the PI or later work)

1. **Root-side observer.** Is a root process that attaches uprobes/eBPF to the README-mode `ubuntu` process acceptable as the "observation run" condition? It needs a new `run_cfs.sh` hook next to `RUN_CFS_WRAP`.
2. **Installs.** Should `z3`/`libz3-dev` (4.8.12), `gcc-13-plugin-dev`, `bpftrace`/`libbpf-dev` or `cppcheck` be installed? Each is an unreferenced package (`CONDITIONS.md` B5 precedent).
3. **Variant builds.** Is a link-option variant (`--wrap`), as a separate `MISSIONCONFIG` copy of `sample_defs`, acceptable as a recorded deviation for controlled runs?
4. **Native PCT.** Is root-imposed `SCHED_FIFO` with single-CPU affinity acceptable as a separate "strict-priority" experimental arm, alongside README mode?
5. **Kernel drift.** The kernel moved from `-fc-v80` to `-fc-v114`. Should the ENV baseline timings (OPERATIONAL 1.6 s, overflow counts) be re-measured before new comparisons?
6. **Untested items:** per-thread cgroup-v1 freezer; uprobe pre-registration on a not-yet-dlopen'ed app `.so` (tested only on the main binary); cost of a purpose-built ptrace scheduler; valgrind and TSan slowdown on cFS; time to build compiler-rt from `LLVM/`.

---

## 10. Literature

This report relies on measurements, not on literature. Where techniques are named:

- **PCT:** Burckhardt, Kothari, Musuvathi, Nagarakatte, "A Randomized Scheduler with Probabilistic Guarantees of Finding Bugs", ASPLOS 2010.
- **CHESS:** Musuvathi, Qadeer, Ball, Basler, Nainar, Neamtiu, "Finding and Reproducing Heisenbugs in Concurrent Programs", OSDI 2008.
- **ThreadSanitizer:** Serebryany, Iskhodzhanov, "ThreadSanitizer – Data Race Detection in Practice", WBIA 2009.

Sibling report `04_lit_dynamic.md` verified these three as V-abs. **I did not re-verify them here.** This turn's WebSearch budget was exhausted and Scholar Gateway returned `ACCESS_DENIED`.

- **rr:** that rr requires hardware performance counters, and the rr paper (O'Callahan et al., USENIX ATC 2017), are **UNVERIFIED**. The measured fact that matters here is independent of that: no HW PMU events are available in this VM.

---

## Appendix A. Probe inventory (all under `PROBE/`)

| Directory | Content |
|---|---|
| `cmake_find/` | Empty `find_package(MLIR/Clang)` project and its configured build |
| `mlir_min/` | ODS dialect `probe`, `probe-min` (parser/walker), `probe-opt` (`MlirOptMain` + `func` + `smt` + `ExportSMTLIB`), `t.mlir`, `smt.mlir` |
| `clang_min/` | `probe-calls` LibTooling tool: API call-site counts, intra-TU, macro-expansion, indirect; static (`build/`) and `libclang-cpp` (`build_dylib/`) builds |
| `cdb_fsw/` | Filtered FSW compile DB (448 entries), file lists, tool outputs (`run_werror.err`, `intra_tu.out`, `final_notest.out`) |
| `syntax/` | gcc / clang-18 / clang-23 `-fsyntax-only` outputs for `cfe_sb_api.c` |
| `ir/` | `cfe_sb_api.ll`, `cfe_sb_api.mlir`, `core-cpu1.mlir` |
| `bc/` | 448 `.bc`, `core_list.txt`, `core-cpu1.bc` (`--only-needed` BSP resolution) |
| `gccinst/` | `ref.o`, `finst.o`, `patch.o`, `cg.o`, `cg.ci` |
| `wrap/` | `--wrap` and `LD_PRELOAD` semantics probe |
| `kprobe/` | perf uprobe/watchpoint/bpf probes (`pe.c`, `pe2.c`, `attach.c`, `wp.c`), uprobe/watchpoint microbenchmarks, gdb breakpoint script, semaphore ping-pong |
| `tsan/` | gcc TSan smoke test |
| `mnt_tfs/`, `mnt_dfs/` | Empty mount points used only inside a private mount namespace |

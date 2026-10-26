# 컴파일러 연결부 후보 3: LLVM 새 pass manager plugin의 제한된 적합성 확인 (단계 C, 2026-10-26)

- 범위: IR(사용자 업로드 `Kairos_OS_Independent_Instrumentation_Review_20261026.md`) §5·§5.3, §13.1(L-A/L-I), §13.2, §15 단계 C, §15.1의 일곱 질문을 **고정 LLVM 빌드(`/home/user/work/llvm-project/build`, 커밋 `1053047a`)의 clang에 적재하는 pass plugin 한 후보**에 대해 증거로 답한다. 결정 기록 `Architecture/spec/PI_DECISIONS_RESOLVED_20261026.md`(PD26) §2.3, §4(구속), SPEC rev 1.4 ADR-023(Clang 빌드는 자기 비계측 기준선 L-A를 둔 별도 구성으로만)을 따른다. 런타임·ABI는 B1(`kairos/runtime/`, 커밋 `a335519`, `docs/event_semantics.md`)을 **고치지 않고** 썼고, 논리 site는 GCC 후보와 **같은 manifest 파일**(`instrument/gcc/sites/{c1_post_1df262a,r2_v701}.kzs`, sha256 `8dbdd2e8…`, `a4a92b90…`)이다.
- 표기: **[F]** 코드·문서에서 확인, **[M]** 이번에 빌드·실행해 확인, **[I]** 추론, **[D]** 설계 결정, **[U]** 미확인.
- 지위: 시제품과 **pilot/dev 부팅**(C1 tx·R2 각 L-I 1회, 컴파일러 차이 확인용 L-A 각 1회)이다. 본 실험 결과가 아니며 반복 횟수·빈도·비용을 주장하지 않는다. Clang 빌드는 기준 빌드의 **파생 구성**이고 차이는 §3.1에 모두 적었다(CONDITIONS_POLICY). 연구 조건을 새로 고르지 않았다.
- LLVM Instrumentor: 로컬 LLVM에 없다(PD26 §1). **사용하지 않았고**, IR이 인용한 상류 커밋 `8da87cd5…`의 소스 두 파일만 읽었다(§9).
- 산출물(저장소): `kairos/instrument/llvm/` — plugin `kzllvm.cpp`, `CMakeLists.txt`, 단위 시험 `tests/`, 빌드·실행·대조 스크립트 `scripts/`. 빌드 트리·실행 기록(저장소 밖): `/home/user/work/instr/llvm/{plugin-build,crt-profile,c1,r2,upstream_instrumentor}`.

## 0. 요약

1. **시제품**: `kzllvm.so`는 `-fpass-plugin=`으로 clang에 적재되는 plugin이다. manifest(`-mllvm -kzllvm-manifest=`)의 site가 있는 번역 단위에만 B1 hook 호출을 넣는다(호출 전·후, 함수 진입·반환, 포인터 매개변수를 통한 store). 삽입 위치는 PipelineStart(최적화 전, `-O0` 포함)이고, 확인은 OptimizerLast에서 한다. 단위마다 `kzs-sites 1` binding 파일에 `bound / inlined / duplicated / removed / unsupported`를 적는다. 값 교체 기능은 없다. 단위 시험 7개가 통과했다(GCC 후보의 시험 대상을 그대로 사용, `-O0/-O2/-O3/-O0 -g`, `line=` 처리, 무관 단위 불변, 런타임 거부) [M].
2. **빌드**: C1(Release `-O3`, PIE + `.so`)과 R2(debug `-O0 -g`, 단위 시험 포함)의 target 코드를 clang으로 빌드했다(L-A 비계측, L-I 계측). 기준 플래그를 그대로 두되 **번역 네 가지**가 필요했다(§3.1): T1·T2 진단 플래그, T3 빌드 체계의 컴파일러 id 분기, T4 단위 시험의 coverage runtime. 번역을 뺀 compile·link 명령은 기준 빌드와 같다(C1 285/285, 47/47; R2 1417/1417, 323개 중 1개는 비교 정규화의 충돌) [M].
3. **T3이 없으면 C1이 깨진다**: C1의 cFE(`6cccb3f`)는 core 라이브러리의 `--whole-archive`를 GNU 컴파일러 id에서만 넣는다. Clang id로 빌드하면 core가 내보내는 기호가 47개 적고, 부팅은 OPERATIONAL까지 가지만 CF·BP·bplib·TO_LAB·SCH_LAB이 적재되지 않는다(`undefined symbol`) [M].
4. **대조(IR §13.2)**: C1 tx의 정체성 표(OSAL task id 17개의 소유 앱·생성 task)와 CF·BP 세마포어 조회·생성 순서(이름, 반환 코드, id)가 F0 smoke3 기록과 diff 0이다. R2에서 SB 호출 3곳으로 AppID 0이 넘어간 전송은 **24건(CFE_ES 20 + CFE_EVS 4)**으로 F0와 같고, task 26개의 정체성도 diff 0이다. SB가 저장한 자기 id `0x110003`도 직접 얻었다 [M].
5. **이 후보에서만 보인 것**: `-O3`의 C1에서 CF·BP site 7개가 루프 펼침으로 **두 코드 사본**(`duplicated`)이 되었다. 실행에서는 binding id가 같고 pc가 다른 사본으로 구별되었다. 디버그 정보가 없는 C1 빌드에서는 `line=`를 컴파일러가 확인할 수 없다(호출이 하나뿐일 때만 연결하고 `line=unverified`로 표시). 반면 R2(`-g`)에서는 `line=`이 디버그 위치와 맞았다 [M].
6. **판단(§6)**: 일곱 질문 기준으로 이 후보는 C1·R2의 목표 지점을 모두 연결했고 값이 F0와 같았다. 부담은 compiler 자체가 아니라 **cFS 빌드 체계의 Clang 대응(T1–T4)과 L-A 기준선의 의무**, IR 수준의 ABI 낮춤, LLVM 판 고정이다(§7). 세 후보 중 하나를 고르는 일은 PI가 한다(PD26 §2.3) [D].

## 1. 환경 (PD26 §1)

| 항목 | 값 | 근거 | 상태 |
|---|---|---|---|
| clang | `/home/user/work/llvm-project/build/bin/clang` = `clang version 23.0.0git (… 1053047a…)`, `+assertions`, `LLVM_TARGETS_TO_BUILD=host`, `LLVM_ENABLE_PLUGINS=ON`(실행 파일이 LLVM 기호를 내보냄: `nm -D` 79,417개), `CLANG_DEFAULT_LINKER` 비어 있음(= `/usr/bin/ld`), crt·libgcc는 gcc 13 설치본 | `build/CMakeCache.txt`, `clang -###` | 기존 환경 [M] |
| plugin API | `LLVM_PLUGIN_API_VERSION 2`, 헤더 `llvm/Plugins/PassPlugin.h`. pass plugin은 `-mllvm` 해석 **전에** 적재되므로 plugin의 `cl::opt`를 `-mllvm`으로 줄 수 있다 | `clang/lib/Frontend/CompilerInstance.cpp` L1074–L1091, `ExecuteCompilerInvocation.cpp` L235–L249 | [F] |
| Instrumentor | 로컬 트리에 `Instrumentor.h` 없음 | PD26 §1 | 사용 안 함 |
| **환경 변경** | compiler-rt **profile runtime만** 같은 LLVM 소스(`1053047a`)에서 트리 밖에 빌드: `/home/user/work/instr/llvm/crt-profile/resource/lib/linux/libclang_rt.profile-x86_64.a`(sha256 `03c24ccc…`). 고정 LLVM 빌드에는 compiler-rt가 없다(`LLVM_ENABLE_RUNTIMES=openmp`). `llvm-project` 트리는 바꾸지 않았다 | R2 단위 시험의 coverage 링크에 필요(§3.1 T4) | deviation(환경 변경, PD26 §1의 P-LIBBPF 규칙) [M] |

## 2. 시제품 `kzllvm` (IR §5.3)

### 2.1 무엇을 넣는가 [D][M]

| manifest 형태 | 넣는 곳 | 값(`value … from=`) |
|---|---|---|
| `phase=before callee=C func=F` | F 안의 C 직접 호출 바로 앞 | `arg<k>` = 그 `call`의 IR 인자 operand(이미 계산된 SSA 값) |
| `phase=after callee=C func=F` | 같은 호출 바로 뒤 | `ret` = **그 `call` 명령 자체의 결과 값**(결과를 버리는 호출에도 값이 있으므로 코드를 바꾸지 않음), `out<k>` = 호출 직후 같은 스레드가 `arg<k>`에서 읽은 값 |
| `phase=before`, callee 없음 | F의 진입(entry block의 alloca 뒤) | 함수 `Argument` |
| `phase=after`, callee 없음 | F의 각 `ret` 앞 | 반환 operand, `link` = 진입 기록 |
| `phase=effect`, callee 없음 | F 안에서 포인터 매개변수를 통한 store 바로 뒤(`line=`이 있으면 그 줄만) | `operand0` = 저장 값, `operand1` = 주소 |
| `control <site> ctx=` | 관측 hook 바로 뒤에 `kz_rt_ctl(site, bind, aseq, ctx)` | 관측 hook과 따로 부른다(PD26 §4) |

- **삽입 시점의 IR**: PipelineStart는 clang이 낸 IR(지역 변수가 alloca·load·store인 형태)에서 돈다. 그래서 "포인터 매개변수를 통한 store"는 매개변수 자체이거나, 매개변수만 저장되고 load만 되는 entry alloca(clang의 매개변수 저장 칸)에서 읽은 포인터로 정의했다. 필드 store `p->f = v`는 GCC 후보와 같이 이 형태에서 빠진다 [D][F: `kzllvm.cpp` `param_slots`].
- **값 변환**: 정수는 manifest `type`의 폭으로 자른 뒤 0/부호 확장하고, 포인터는 `ptrtoint`로 넘긴다. 정수·포인터가 아닌 IR 값은 거부한다(`unsupported`). store를 atomic으로 바꾸거나 값을 교체하지 않는다 [F: `to_u64`, `build_obs`].
- **ABI 낮춤(이 후보 고유)**: IR의 인자는 clang이 대상 ABI에 맞게 바꾼 뒤의 것이다(큰 구조체는 `byval` 포인터, 반환 구조체는 `sret` 첫 인자, 작은 구조체는 정수로 합침). 그래서 IR 인자 k가 C 인자 k와 다를 수 있다. 시제품은 대상 인자나 그 앞에 `sret`/`byval`이 있으면 거부한다. 구조체가 정수 하나로 합쳐진 경우는 IR만으로 가려낼 수 없어 막지 못한다. 이번 목표 site의 인자·반환은 모두 정수·포인터다(manifest `ref=`의 선언) [F][I].
- **포인터가 가리키는 형의 근거**: opaque pointer에는 가리키는 형이 없다. 시제품이 대신 적는 근거는 다음과 같다(binding 파일 `# bind … notes=`).
  - 문자열 슬롯: operand가 `[N x i8]` 필드를 가리키는 GEP, 그런 배열의 decay, 또는 그런 배열이 그 상수 offset에 있는 global·alloca인지 본다. clang은 `&g[0].name`을 `@g`로 접는다. 결과는 C1 CF·BP 모두 `array char[20]`이다.
  - `out<k>`: GEP·global·alloca의 형이 manifest 폭과 같으면 `pointee type confirmed by IR`(ABI 정렬로 읽음)로 적고, 아니면 정렬 1로 읽고 미확인으로 적는다. C1·R2의 out 슬롯은 모두 확인됐다 [M].
- **hook 선언 속성**: `kz_rt_obs`는 `nounwind nocallback`이다(GCC `leaf`에 해당: 이 모듈로 다시 들어오지 않음). 메모리 효과는 정하지 않아 앞뒤 load·store·call과의 순서가 유지된다. `kz_rt_obs_str`·`kz_rt_ctl`은 `nounwind`만 붙인다(문자열을 읽고, 대기할 수 있음) [D].
- **binding id**: `(module << 16) | (site << 4) | n`(GCC 후보와 같은 규칙). module은 `-kzllvm-module=<id>:<경로 접두사>`로 정한다.
- **모듈 등록**: site가 있는 단위마다 `llvm.global_ctors`에 `kz_rt_module(module, manifest sha256)` 생성자를 넣는다 [M: dump MODULE].
- **런타임 비계측**: 런타임은 plugin 없이 gcc로 따로 빌드한다(B1 문서 방식). `kz_rt_*`를 정의한 단위는 거부하고 바꾸지 않는다(시험 `kzllvm_refuse`). 쓴 런타임 3개 파일(`libkzrt.a`, `libkzrt-posix.a`, `kz_rt_posix_auto.c.o`)은 **GCC 후보가 쓴 것과 바이트가 같다**(sha256 `3e588e18…`, `620bde6d…`, `87befe00…`) [M].

### 2.2 pass 위치·최적화·위치 대응 (IR §5.1·§9.3의 기록 항목)

- 삽입은 `registerPipelineStartEPCallback`에서 한다. 이 지점은 `-O0`(`buildO0DefaultPipeline` L2350)과 `-O1+`(`buildPerModuleDefaultPipeline` L1743)에서 모두 호출된다. pass는 `isRequired`라서 `-O0`의 `optnone` 함수에서도 돈다 [F: `PassBuilderPipelines.cpp`].
- 확인은 `registerOptimizerLastEPCallback`에서 한다. 함수마다 hook 호출을 binding id별로 세어 `removed`(0), `bound`(원 함수에 1), `inlined`(다른 함수에 1), `duplicated`(2 이상)로 판정한다. 그 뒤의 code generation 복제는 링크된 파일에서 GCC 후보의 `bincheck.sh`(`%esi`의 binding id)로 다시 센다. C1·R2 모두 IR 확인과 개수가 같았다 [M].
- **`line=` 처리**:
  - 디버그 위치가 있으면 그 줄과 비교한다. R2(`-g`)의 호출 6곳(275, 1183, 786, 138, store 754·759)이 manifest 줄과 맞았다.
  - 디버그 위치가 없는 단위(C1 Release)에서는 확인할 수 없다. 그 함수에 그 callee 호출이 **하나뿐일 때만** 연결하고 `line=unverified(no debug location; the only call of …)`로 적는다. 둘 이상이면 `unsupported`다.
  - 이 경우 틀린 줄을 잡지 못한다(시험 `kzllvm_lines`가 이 한계를 기록한다). C1의 해당 호출 4개가 함수 안에서 하나뿐임은 소스에서 확인했다(`cf_cfdp.c:1003`, `:1011`, `bp_flow.c:126`, `:132`) [M][F].
- binding 파일 머리에는 LLVM 판, plugin API, 단위, 출력 객체(`/proc/self/cmdline`의 `-o`; 같은 소스를 두 번 컴파일해도 파일이 갈림), manifest sha256, `-O` 단계, LTO 단계, PIC/PIE, coverage 여부, 디버그 위치 유무, triple을 적는다. LTO는 C1·R2 기준 플래그에 없어 평가하지 않았다(prelink 단계이면 `check=deferred-to-lto-link`로 적는 경로만 있음) [U].
- site가 없는 단위는 건드리지 않는다. 객체가 `-O2`와 `-O0 -g` 모두에서 바이트 동일했다. clang은 기본으로 명령행을 DWARF에 적지 않으므로 GCC 후보와 달리 `-g`에서도 같다(시험 `kzllvm_untouched`) [M].

### 2.3 시험 [M]

`cmake -S kairos/instrument/llvm -B /home/user/work/instr/llvm/plugin-build && cmake --build … && ctest`를 실행해 7개가 통과했다.
- `kzllvm_{O0,O2,O3,O0-g}`: GCC 후보의 두 스레드 시험 대상(`instrument/gcc/tests/kzgcc_target.c`, manifest 그대로)을 빌드·실행한다. GCC 시험과 같은 값 검사(이름 복사, `-17` 부호 확장, out id `0x30001`, 두 store 값, 결과를 버린 호출의 반환, 생성·진입 task id, link)를 하고, 추가로 문자열·out 형 근거를 본다.
  - `-O2`·`-O3`에서 static helper의 probe는 `provider_init`으로 inline된 뒤 2회 루프가 펼쳐져 `duplicated`(2 사본)가 된다. GCC는 같은 단위에서 `inlined`(1 사본)였다.
- `kzllvm_lines`: `line=`을 디버그 위치가 있을 때와 없을 때 처리하는지 본다.
- `kzllvm_untouched`, `kzllvm_refuse`.

시험 값은 단위 시험 값이며 연구 조건이 아니다.

## 3. 파생 빌드 (C1, R2)

### 3.1 차이 대장

| ID | 차이 | C1 (`scripts/build_c1.sh`) | R2 (`scripts/build_r2.sh`) | 영향·근거 |
|---|---|---|---|---|
| D1, D2, D4 | 작업 경로, `.git` 없는 사본(모듈 버전 표를 바이트 그대로 복원), 빌드 날짜 문자열 | GCC 후보의 `prepare` 단계를 그대로 호출(`/home/user/work/instr/llvm/c1`) | 같음(`…/r2/cFS`) | `instr_candidate_gcc.md` §3.1과 같음. 복원한 표 sha256 C1 `431189d0…`, R2 `4615470b…` [M] |
| L1 | target 코드 컴파일러 | arch 빌드 `native/default`를 빈 cache에서 다시 configure한다. cFE가 넘기는 인자(`mission_build.cmake` L438–L447)에 `CMAKE_C_COMPILER=<LLVM>/bin/clang`을 더한다. mission(host 도구)은 gcc다 | `native/default_cpu1`만 같은 방식으로(인자 L644–L655, 값은 prep의 cache에서 읽음). cpu2와 mission은 gcc다 | IR §13.1 L-A의 정의 |
| T1 | `-Wno-unknown-warning-option` | 기준 플래그의 GCC 전용 경고(`-Wno-error=restrict`, `-Wno-stringop-truncation` 등)를 clang이 모른다고 경고하고, `-Werror`가 그 경고를 오류로 만든다 | 같음 | 진단만 바뀐다 |
| T2 | `-Wno-error=<w>`: gcc 13.3은 침묵하지만 clang은 기준 `-Werror` 아래에서 오류로 내는 경고 | `make -k`로 찾은 20개: strict-prototypes(OSAL `os-impl-tasks.c:695`), cast-align 15(bplib, CF; gcc는 엄격 정렬 target에서만 경고), unused-function 3(CF `cf_codec.c`), sometimes-uninitialized 1(bplib `v7_dataservice_api.c:653`) | 단위 시험 코드의 header-guard 9, non-literal-null-conversion 13, strict-prototypes 4, uninitialized-const-pointer 34, default-const-init-var-unsafe 3, implicit-enum-enum-cast 1. 비행 코드의 unused-function 4(CF), parentheses-equality 2(CS `cs_table_processing.c:932`) | 경고로는 남는다(log에 C1 22, R2 95). code generation 플래그는 바뀌지 않는다 |
| T3 | `CMAKE_PROJECT_INCLUDE=scripts/cmake_gnu_id_gates.cmake`: arch 빌드의 각 `project()` 뒤에 `CMAKE_C_COMPILER_ID`를 GNU로 둔다(실제 id는 `KZ_T3_DETECTED_C_ID`). CMake가 Clang용으로 읽은 규칙·플래그는 그대로다 | **필수**. cFE `cmake/target/CMakeLists.txt` L159–L164는 `--whole-archive`를 GNU에서만 넣는다. Clang id로 빌드(T3 없이)하면 core-tx가 내보내는 동적 기호가 1,101 → 1,054개가 되고, 부팅 log에 `Error loading shared library: ./cf/cf.so: undefined symbol: CFE_TBL_Manage`(bp.so, bplib.so, to_lab.so, sch_lab.so도 같음)가 나온다 [M] | v7.0.1 cFE는 같은 곳에서 Clang도 받는다(L187–L188). 남은 GNU 분기는 OSAL `bsp/generic-linux/CMakeLists.txt` L42(단위 시험 coverage 플래그와 `gcov` 링크)다 | 빌드 체계의 판단이 기준 빌드와 같아진다. 결과로 compile·link 명령이 기준과 같다(아래). 허용 여부는 새 PI 결정이다(§10) |
| T4 | coverage 단위 시험의 runtime | 해당 없음(C1에 단위 시험 없음) | (a) `CMAKE_EXE_LINKER_FLAGS += -resource-dir=<§1의 profile runtime>`: `--coverage`로 링크하는 56개 단위용. (b) `CMAKE_C_STANDARD_LIBRARIES = libclang_rt.profile-x86_64.a`: GNU 분기의 `gcov` 라이브러리에 해당하며 `-fprofile-arcs` 단위 220개용. 링크 줄 끝에 붙는다. 처음 시도에서는 126개 링크가 `llvm_gcda_end_file`·`llvm_gcov_init` 미정의로 실패했다 | `core-cpu1`에는 profile 기호가 0개다(`nm`). coverage 객체가 없는 실행 파일은 이 archive에서 아무것도 끌어오지 않는다 [M] |
| L2 | (L-I) plugin과 런타임 | `CMAKE_C_FLAGS += -fpass-plugin=… -mllvm -kzllvm-manifest/out/module…`, `CMAKE_EXE_LINKER_FLAGS` = 런타임(whole archive 2개 + 시작 객체) | 같음 + `-kzllvm-skip-coverage`(GCC의 `skip-coverage`와 같은 뜻) | 링크 단계에서는 `-mllvm`이 쓰이지 않는다는 경고가 나온다(C1 45, R2 960, `-Wunused-command-line-argument`) |
| — | 빌드 명령·환경 | `c1_build/build_one.sh`와 같음(`CFS_APP_PATH`, DV-P1 `CFLAGS=-Wno-error=restrict`, DV-P2), root | `make native_std.prep`/`.install`를 `runuser -u ubuntu`로 실행하고 `env/build_cfs.sh`의 거부 변수 목록을 확인 | from-reference(환경 이름만 기록) |

**기준 빌드와의 명령 대조** [M]: `scripts/cmp_compile_commands.sh`와 `scripts/cmp_link_commands.sh`로 했다. 컴파일러 경로와 T1·T2·T4·L2의 플래그를 빼고, 경로를 사본으로 바꾸고, 표 객체 디렉터리 이름의 경로 해시를 지운 뒤 비교했다.
- C1 L-A·L-I: compile 285/285 같음, link 47/47 같음.
- R2 L-A·L-I: compile 1417/1417 같음. link 323개 중 1개가 다르다. 정규화 뒤 이름이 같아지는 두 표 target(`to_lab_sub_alt`/`_bad`)이 서로 짝지어진 비교 쪽의 결함이며, 빌드 차이가 아니다.

### 3.2 L-A 부팅 (컴파일러 차이만, IR §13.1) [M]

| 빌드 | 결과 |
|---|---|
| C1 L-A, T3 없음(Clang id) | tx·rx 5초 안에 OPERATIONAL, 정지 `ci-es-restart-poweron`, 종료 0/0. 그러나 CF·BP·bplib·TO_LAB·SCH_LAB **적재 실패**(위 log). 판정에 쓸 수 없는 구성 |
| C1 L-A, T3 | tx·rx OPERATIONAL, 적재 오류 0, BP가 `BP0_tsem`·`BP1_tsem`을 만들고 초기화를 마침, 종료 0/0 |
| R2 L-A | `run_cfs.sh` 정지 사유 `operational_seen`, OPERATIONAL 1,647 ms, 종료 0, 적재 오류 0. 기호 조회 실패 37줄(`SBN_F_Remap`)은 GCC 파생 실행과 같은 수 |

### 3.3 계측이 바꾼 것 (L-A 대 L-I) [M]

- C1: arch 빌드 객체 261개 중 바뀐 것은 계측 단위 6개뿐이다(`cfe_es_api.c`, `cfe_es_apps.c`, `osapi-task.c`, `os-impl-tasks.c`, `cf_cfdp.c`, `bp_flow.c`). 나머지는 바이트가 같다.
- R2: cpu1 객체 1,335개 중 276개가 바뀌었다(원시 바이트 기준이고, `--strip-debug` 뒤에도 같은 수).
  - 계측 단위 8개: `cfe_es_api.c`, `cfe_es_apps.c`, `cfe_evs.c`, `cfe_sb_api.c`, `cfe_sb_priv.c`, `cfe_sb_task.c`, `osapi-task.c`, `os-impl-tasks.c`.
  - D4의 `cfe_build_env_table.c.o`.
  - coverage 객체 267개: clang에서도 같은 파일을 두 번 컴파일하면 다르다(gcov stamp, 확인함). plugin은 이 단위들을 건너뛰었다(binding 파일 7개가 "not instrumented").
- 크기(text / bss, 런타임 포함):

  | 파일 | L-A | L-I | 비고 |
  |---|---|---|---|
  | C1 `core-tx` | 381,184 / 1,245,016 | 406,045 / 5,537,672 | gcc 기준 417,562 / 1,245,440 |
  | C1 `cf.so` | 82,127 | 81,862 | **줄었다** |
  | C1 `bp.so` | 35,381 | 36,312 | |
  | R2 `core-cpu1` | 483,974 | 510,331 | gcc 기준 519,663 |

- 명령 수(gcc 기준 / L-A / L-I):

  | 함수 | gcc 기준 | L-A | L-I |
  |---|---|---|---|
  | `CF_CFDP_InitEngine` | 229 | 458 | 328 |
  | `BP_FlowInit` | 92 | 91 | 211 |
  | `OS_TaskCreate` | 123 | 97 | 123 |
  | `CFE_EVS_SendEventWithAppID` | 94 | 91 | 127 |
  | `CFE_ES_GetAppID` | 34 | 35 | 53 |

  `CF_CFDP_InitEngine`이 계측 뒤 줄었으므로 hook이 clang의 펼침·inline 판단을 바꾼다. L-I와 L-A의 차이는 "hook 호출만 더한 것"이 아니다(IR §13.1의 G-I0/G-I 주의와 같은 종류) [I].
- 실행 비용(hook 비용, 기록 비용)은 재지 않았다(L-I0/L-I 미실시) [U].

## 4. site manifest와 binding 결과 (IR §9.3, §15 단계 C 산출물)

binding 파일은 단위마다 plugin이 쓴 것을 GCC 후보의 `merge_binds.sh`로 합쳤다(저장소 밖 `c1/binds_li.kzs`, `r2/binds_li.kzs`).

| 사례 | 논리 site → binding | 상태 | 비고 |
|---|---|---|---|
| C1 core | 정체성 9개(es.start_task.entry/ret, es.child_task.entry/ret, osal.task_create.entry/ret, osal.task_entry, osal.task_create.id, es.exit_app.entry) | 모두 `bound` | 디버그 정보가 없어 반환 site의 줄은 `:0` |
| C1 cf.so | cf_lookup_call.before(+control), cf_lookup_call, cf_retry_delay (3) | **`duplicated`**(`CF_CFDP_InitEngine` 안의 2 사본) | 채널 루프가 펼쳐졌다. `line=unverified`(유일 호출). 이름 슬롯 `char[20]`(array decay), out0 형 확인 |
| C1 bp.so | bp_existence_probe_call(.before), bp_create_call(.before, +control) (4) | **`duplicated`**(`BP_FlowInit`로 inline된 뒤 2 사본) | 원 함수 `initialize_throttling`(static, 한 번 호출)이 사라졌다. GCC는 `inlined` 1 사본 |
| R2 | SB 호출 3곳, sb_publication, `CFE_ES_GetAppID.store`(L754·L759 두 binding), evs.send.entry/ret, 정체성 8 (16) | 모두 `bound`(`-O0`) | `line=`이 디버그 위치와 맞음 |

- 독립 확인: 링크된 `core-tx`·`cf.so`·`bp.so`·`core-cpu1`에서 hook 호출을 objdump로 센 결과가 binding 파일과 같다(C1 core binding마다 1개, cf·bp binding마다 2개, R2 binding마다 1개). `.so`는 실행 파일이 내보낸 기호를 부른다(`cf.so`: `U kz_rt_obs`, `core-tx`: `T kz_rt_obs`) [M].
- 지원 못한 지점: 이 manifest의 목표 지점에는 없다. 형식상 `unsupported`가 되는 경우는 다음과 같다 [F: `kzllvm.cpp`].
  - noreturn·invoke 호출 뒤
  - 슬롯 4개 초과
  - 정수·포인터가 아닌 IR 값
  - 대상 인자나 그 앞의 `sret`/`byval`
  - 간접 호출(callee를 알 수 없음)
  - 디버그 위치 없이 `line=` site에 맞는 호출이 둘 이상
  - 한 단위에서 한 site의 binding이 16개 이상
- F0와 위치의 뜻이 다른 곳은 GCC 후보와 같다(`instr_candidate_gcc.md` §4). F0의 세마포어 조회·생성 입구는 피호출 쪽(모든 호출자)이고, 이 manifest는 CF·BP 호출 쪽이다.

## 5. pilot 부팅과 F0 대조 (IR §13.2 순서)

### 5.1 C1 tx (post 833fdbb, 1df262a): `scripts/run_c1.sh`, 대조 `scripts/compare.sh c1`

- 부팅 절차: `observe_c1_smoke.sh --instances`의 절차를 관측기 없이 따랐다(GCC 후보 `run_c1.sh`와 같은 단계). tx·rx 모두 5초 안에 OPERATIONAL, 정지 `ci-es-restart-poweron`, 종료 0/0.
- 환경: tx += `KZ_RT_DUMP`, `KZ_RT_RUN_ID=1`, `KZ_RT_INSTANCE=1`, rx += `KZ_RT_MODE=off`, `KZ_RT_INSTANCE=2`.
- 대조 기록: `c1_runs/smoke3/post_1df262a/run.kzt`(kz-dump 텍스트; GCC 후보가 쓴 것과 바이트가 같음).
- 산출물: `/home/user/work/instr/llvm/c1/{run1,cmp1}`, dump sha256 `c2e25583…` [M].

1. **위치 대응**: §4와 같다. 정체성 9 site는 F0와 같은 label·같은 함수 진입/반환이다. CF·BP의 상태·id는 F0의 호출 쪽 점과 같은 곳에서 읽는다. 이름·out id는 F0가 피호출 쪽에서 읽던 값을 호출 쪽에서 읽는다.
2. **도달**: 횟수가 F0와 같다(GCC 후보와도 같다).

   | site | 횟수 |
   |---|---|
   | start_task | 15 |
   | child_task | 5 |
   | task_create | 15 |
   | task_entry | 15 |
   | task_create.id | 17 |
   | exit_app | 0 |
   | CF 조회 | 3 |
   | 재시도 지연 | 1 |
   | BP 확인 | 2 |
   | BP 생성 | 2 |

   **사본별 도달(pc)**: CF 조회는 첫 사본에서 2회(채널 0: −17 뒤 성공), 둘째 사본에서 1회(채널 1)다. 재시도 지연은 첫 사본에서만, BP site는 사본마다 1회다. 같은 횟수를 합격 조건으로 쓰지는 않는다.
3. **포획 적합성**: 정규화한 줄의 diff는 0이다(`cmp1/sem.diff`).
   - CF: `BP0_tsem` −17 → `BP0_tsem` 0, id `0x30002` → `BP1_tsem` 0, id `0x30003`.
   - BP: `BP0_tsem` −17 → 생성 0, `0x30002` → `BP1_tsem` −17 → 생성 0, `0x30003`, 생성 초기값 10.
   - 실패한 조회 뒤의 out 값은 호출자 변수의 이전 내용이다. 상태가 0일 때만 id로 읽는다(GCC 후보 §5.1과 같은 규칙) [M].
4. **식별 적합성**: OSAL task id 17개 각각의 소유 앱 id와 생성 task가 F0와 같다(diff 0).
   - 예: CF = task `0x1000c`, owner `0x110009`. BP = `0x1000d`, `0x11000a`. BP의 child `0x1000e`·`0x1000f`.
   - timebase 계열 `0x80001`·`0xc0001`은 OSAL 초기화 중 생성되어 owner가 없다(`-`).
5. **기록 완전성**: 손실 0, actor 16개 모두 aseq 빈틈 0, partial 0, `kz-rt-dump --manifest` 검사가 통과했다. 사건은 117개다. dump는 정상 종료 경로의 `atexit`에서 써졌다.
6. **판정 근거**: 값(직접 포획)과 같은 actor 안의 순서(aseq, 같은 호출의 link)가 F0와 같은 근거를 준다. actor 사이의 순서는 런타임이 만들지 않는다. `kz replay` 판정은 dump → `.kzt` 변환이 없어 하지 않았다 [U].

### 5.2 R2 (cFE v7.0.1 ENV, native_std): `scripts/run_r2.sh`, 대조 `scripts/compare.sh r2`

- 부팅: `env/run_cfs.sh operational ci-es-restart-poweron`을 `CFS_DIR=<사본>`으로 관측기 없이 실행했다(persist 스냅숏 전후). OPERATIONAL 1,654 ms, 정지 사유 `operational_seen`, 종료 0, 적재 오류 0.
- 대조 기록: `r2_runs/v153_pilot3`.
- 산출물: `/home/user/work/instr/llvm/r2/{run1,cmp1}`, dump sha256 `bec07d1a…` [M].

1. **위치 대응**: §4. SB 호출 3곳은 F0 `at-call` 점과 같은 줄의 호출이다(디버그 위치로 확인). 슬롯은 같은 문장에서 읽은 `CFE_SB_Global.AppId` 인자 operand다.
2. **도달**:

   | site | 이 실행 | F0 |
   |---|---|---|
   | start_task | 24 | 24 |
   | child_task | 4 | 4 |
   | task_create | 24 | 24 |
   | task_entry | 24 | 24 |
   | task_create.id | 26 | 26 |
   | @275 | 26 | 26 |
   | @1183 | 111 | 111 |
   | @786 | **107** | 65 |
   | sb_publication | 1 | 1 |
   | evs.send.entry | **311** | 269 |

   차이는 모두 앱 task의 AppID `0x110003`(유효) 전송이다(TO_LAB 61 대 33, DS, HK, HS, MM, SC, MD, CS, SBN). GCC 실행(74/278)과 CORD 실행에서도 달라지는 부분이다. IR §13.2에 따라 차이로만 기록한다.
3. **포획 적합성**: SB 호출 3곳에서 AppID 0이 넘어간 전송이 **24건 = CFE_ES 20 + CFE_EVS 4**다. 호출 지점별 분포도 F0와 같다. `evs.send.entry`에서 AppID 0 줄의 차이는 없다.
4. **식별 적합성**: task id 26개의 소유 앱·생성 task가 F0와 같다(diff 0).
   - SB 공개: `sb_publication` 상태 0이고 호출 직후 out 값은 `0x110003`이다. `CFE_ES_GetAppID` 안의 store(L754)가 저장한 값도 `0x110003`이다.
   - 이 값은 SB task `0x10004`의 생성 쪽 소유 앱 id와 같다. 생성 쪽 근거와 공개 값이 독립적으로 맞는다 [M].
5. **기록 완전성**: 손실 0, actor 25개 빈틈 0, 검사 통과, 사건 2,562개다.
   - `CFE_ES_GetAppID.store`는 모든 호출자의 store라 1,541건이다(SBN 536, TO_LAB 346).
   - **가장 큰 actor 버퍼가 64,152 / 65,536 B(98%)까지 찼다.** 넘치지는 않았지만 다음 실행에서는 `overflow` 손실이 날 수 있다 [M][I].
6. **판정 근거**: AppID 0 24건은 직접 포획 값이고 F0와 같다. 앱 전송 수의 차이는 실행 경로의 차이다.

## 6. IR §15.1의 일곱 질문

| # | 질문 | 답과 근거 |
|---|---|---|
| 1 | 현재 공개 cFS 구성과 필요한 target을 빌드하는가 | **예(호스트 x86-64 Linux)**. 단 cFS 빌드 체계의 Clang 대응이 필요하다(T1–T4, §3.1). T3이 없으면 C1 cFE의 GNU 전용 `--whole-archive` 분기 때문에 앱 5개가 적재되지 않는다. T4는 고정 LLVM 빌드에 없는 compiler-rt profile runtime을 요구한다. 번역 뒤 compile·link 명령은 기준과 같고, L-A·L-I가 모두 부팅한다 [M]. 다른 target: clang 자체는 다중 target이지만 이 고정 빌드는 `LLVM_TARGETS_TO_BUILD=host`뿐이다. RTEMS·VxWorks용 target header·library·ABI는 따로 필요하다(IR §13.5) [F][U] |
| 2 | 실제 값·반환·공개 지점을 부작용 없이 전달하는가 | **예, 범위 안에서**. hook 인자는 원 명령의 IR operand이고, 반환은 `call`의 결과 값 그대로다. 재평가·store 변경·값 교체가 없다. 코드에 생기는 변화는 hook 호출, manifest가 선언한 `out` load, 그리고 그에 따른 최적화 판단 변화다(`CF_CFDP_InitEngine` 458 → 328 명령). 값은 F0와 같다(§5). 부작용 없음을 비용 면에서 재지는 않았다 [U] |
| 3 | source 위치와 실행 지점의 대응을 설명하는가 | **예, 디버그 위치가 있을 때**. R2에서는 `line=`이 디버그 위치로 확인됐고, inline·사본이 binding 파일·링크 뒤 검사·실행 pc로 세 번 맞았다. 디버그 정보가 없는 C1 Release에서는 `line=`을 컴파일러가 확인하지 못하므로 유일 호출일 때만 연결한다(표시함). `-gline-tables-only`를 더하면 확인할 수 있지만 기준 플래그를 바꾸므로 쓰지 않았다 [M][D] |
| 4 | 초기화 전 관측을 지원하는가 | **예(런타임이 제공)**. hook은 OSAL·cFE에 의존하지 않는다. OSAL 초기화 중 생성된 task(`0x80001`, `0xc0001`)도 기록됐다. 모듈 생성자(`llvm.global_ctors`)의 등록은 부트스트랩 경로로 들어갔다(`module 1 actor=?`). 생성자가 없는 RTOS에서는 하네스가 init/attach를 불러야 한다(B1 열린 항목) [M][U] |
| 5 | 필요한 미계측 경로를 확인할 수 있는가 | **예, 번역 단위 수준까지**. 연결 안 된 site는 `unsupported`(이유)로, 사라진 hook은 `removed`로 남고, 건너뛴 coverage 단위는 목록으로 남는다. 호출 쪽 site가 다른 호출자를 보지 않는다는 점은 manifest가 명시한다. 이 clang으로 빌드하지 않은 코드(gcc로 빌드한 런타임과 `/usr/local` tinycbor, cpu2·host 도구)와 간접 호출은 보이지 않는다 [F][M] |
| 6 | 같은 runtime에 연결되는가 | **예**. B1 런타임을 수정 없이 링크했고, 런타임 파일은 GCC 후보와 바이트가 같다. dump는 `kz-rt-dump --manifest` 검사를 통과했다. clang으로 빌드한 대상과 gcc로 빌드한 런타임이 같은 C ABI로 연결된다 [M] |
| 7 | 연구를 지연시키는 추가 frontend·ABI·모델 작업 | frontend 작업은 없다. 대신 다음이 생긴다. (a) **cFS 빌드 체계 번역**(T1–T4)을 구성마다 다시 찾아야 한다(C1·R2에서 경고 종류·GNU 분기가 다름). (b) **L-A 기준선이 의무**다(SPEC rev 1.4 ADR-023, IR §16 "Clang/GCC 차이와 계측 차이를 한 변수로 묶었는가"). F0·GCC 기록과의 비교에는 컴파일러 차이가 늘 함께 들어간다(명령 수 2배 차이 등, §3.3). (c) IR이 ABI를 낮춘 형태라서 인자 번호·구조체 값의 대응을 확인하는 일이 남는다. (d) LLVM 판 고정이 필요하다(C++ API·헤더 위치가 판마다 바뀜: 이 판에서 `PassPlugin.h`가 `llvm/Plugins/`로 옮겨짐). 시제품 크기: plugin 881줄(C++), 스크립트 9개, 시험 7개 [M] |

IR §15.1의 규칙("LLVM의 관측 범위와 개발 자산이 실질적 이점이고 대상 빌드가 확보되면 LLVM 경로를 주 구현으로 선택할 수 있다")에 비추면 다음과 같다. 대상 빌드는 번역 T1–T4를 거쳐 확보되었고, 목표 지점의 관측 범위는 GCC 후보와 같은 결과를 냈다. 이 후보만의 이점은 결과를 버린 호출에도 반환 값이 있다는 점, `-g`에서 무관 단위의 바이트가 같다는 점, 루프 사본이 pc로 드러난다는 점이다. 단점은 §7이다. 주 구현의 선택은 세 후보의 대조가 끝난 뒤 PI가 한다(PD26 §2.3) [D].

## 7. 노력과 위험

| 위험 | 내용 | 대응·상태 |
|---|---|---|
| 빌드 체계의 Clang 대응 | cFE·OSAL·bplib에 GNU 전용 분기가 있다(C1 whole-archive, OSAL coverage, bplib 경고 옵션·version script). 경고 종류도 구성마다 다르다 | T3·T2로 기준 빌드의 판단과 같게 하고, 명령 대조로 확인. T3 허용 여부는 PI 결정(§10) [M] |
| 컴파일러 차이 | L-A와 gcc 기준의 code generation이 크게 다르다(명령 수, 펼침). 순서 관련 결과를 F0와 비교할 때는 컴파일러 효과를 따로 봐야 한다 | L-A 부팅 확인만 했다. L-A 대 G-A 효과는 재지 않음 [U] |
| 최적화 교란 | hook이 펼침·inline 판단을 바꾼다(`CF_CFDP_InitEngine` 458 → 328) | 명령 수로만 확인 [M] |
| 디버그 정보 없는 위치 확인 | `-O3` Release에서 `line=` 미확인 | 유일 호출만 연결하고 표시. 근본 대응은 기준 플래그 변경이라 하지 않음 [D] |
| ABI 낮춤 | `sret`/`byval`은 거부. 구조체를 정수 하나로 합친 것은 IR에서 가려내지 못함 | 목표 site는 스칼라·포인터만. 일반화하려면 clang 측 정보(디버그 형, frontend)가 필요 [I] |
| LLVM 판 고정 | plugin은 그 빌드의 C++ API에 묶인다 | 판마다 단위 시험 7개로 확인 [D] |
| 교차 target | 고정 빌드는 host target뿐이다 | 두 번째 OS 단계(IR §15 H)에서 별도 LLVM 빌드가 필요 [U] |
| 정적 메모리·버퍼 | 런타임 bss 약 4.3 MB(B1 상수). R2의 피호출 쪽 store로 actor 버퍼가 98%까지 참 | 넘치면 `overflow`로 셈. 필요한 호출자만 호출 쪽 site로 [I] |
| 진단 소음 | 링크 단계의 `-mllvm` 미사용 경고(C1 45, R2 960) | 무해. 링크 플래그와 컴파일 플래그를 나누면 없어짐 [I] |
| 라이선스 | LLVM은 plugin에 라이선스 선언을 요구하지 않는다(GCC와 다름) | 저장소 LICENSE는 여전히 미정(GCC 후보 §9의 열린 항목) [F] |

## 8. B1 런타임에 필요한 변경 (이번에는 고치지 않음)

GCC 후보 §8의 1–4가 그대로 해당한다. 이 후보에서 더 보인 것은 아래 두 가지다.

1. `kz-rt-dump`에 EVENT의 `pc` 출력이 필요하다. `duplicated` binding(같은 bind id, 여러 코드 사본)을 가르는 근거는 pc뿐이다. 이번에는 GCC 후보의 `tools/kzgcc_events.c`(B1 decoder 사용)를 그대로 빌드해 썼다(`kz-events`).
2. RUN 레코드의 `backend`·`compiler`·`target`·`manifest_sha256`이 비어 있다(GCC 후보 §8 항목 2와 같음). Clang 빌드에서는 컴파일러 판이 기록의 해석에 더 중요하다(L-A 의무).

## 9. LLVM Instrumentor (상류 소스 읽기만, 사용 안 함)

- 읽은 것: IR이 고정한 커밋 `8da87cd554447470fa1af45a84e7642ab717011a`의 두 파일이다. `raw.githubusercontent.com`에서 받아 `/home/user/work/instr/llvm/upstream_instrumentor/`에 두었다.
  - `llvm/include/llvm/Transforms/IPO/Instrumentor.h`(sha256 `2a543149…`)
  - `llvm/lib/Transforms/IPO/Instrumentor.cpp`(sha256 `efb47916…`)
- 기본 계측 목록 `InstrumentationConfig::populate`(cpp L650–L662): BasePointer, Module, GlobalVar, Function, Alloca, Unreachable, Load, Store, Cast, Numeric, Compare. **call 명령의 계측 항목(호출 지점의 인자·결과)은 없다** [F].
- `FunctionIO::ConfigKind`(h L691–L700): `PassAddress, PassName, PassNumArguments, PassArguments, ReplaceArguments, PassIsMain, PassId`. 반환값을 넘기는 항목이 없다(IR §5.3의 지적과 같음) [F].
- **값 교체 항목**이 여러 곳에 있다: `ReplaceArguments`(L696), `ReplaceAddress`/`ReplaceSize`(L749–L751, L896), `ReplacePointer`(L966, L1062), `ReplaceValue`(L1066), `ReplaceResult`(L1164, L1235, L1282). 관측 경로에서는 모두 꺼야 한다(PD26 §4) [F].
- 걸러내기는 `function_regex`·`target_regex`(h L380–L388)와 실행 시 filter식이다. 호출 지점 단위로 고르는 수단은 없다 [F].
- 설정은 JSON 파일이다(`-instrumentor-read-config-files`, cpp L79–L83). 채택하면 PD26 §5대로 빌드 설정 경계에만 둔다 [F].
- 런타임 호출 이름은 `__instrumentor_` 접두사 규칙이다(h L376). B1 hook ABI에 연결하려면 shim이 필요하다 [I].
- 결론 [I]: C1·R2의 목표 중 Instrumentor로 바로 되는 것은 함수 진입 인자와 store 값·주소다(정체성 site, `CFE_ES_GetAppID.store`). 호출 쪽 site(CF·BP 조회·생성의 반환·out, R2 SB 호출 3곳)는 이번 plugin처럼 따로 짠 pass가 필요하다. 로컬 판에 없으므로 쓰려면 LLVM 판을 바꿔야 하고, 이는 위 §7의 판 고정 위험을 다시 연다.

## 10. 열린 항목과 PI 결정

- **PI 결정 필요(새)**: Clang 파생 구성에서 **번역 T3(빌드 체계의 컴파일러 id 분기를 기준 빌드의 GNU 분기로 맞춤)**과 T1·T2(진단 플래그)를 허용하는가.
  - 레퍼런스가 정하지 않는 이유: SPEC rev 1.4 ADR-023은 Clang 빌드를 "자기 비계측 기준선을 둔 별도 구성"으로 허용하지만, 기준 빌드 체계가 Clang을 지원하지 않는 부분을 어떻게 다룰지는 적지 않는다. CONDITIONS_POLICY는 차이를 기록하라고 할 뿐 허용 범위를 주지 않는다. C1의 cFE `6cccb3f`는 해당 분기를 GNU에만 둔다(v7.0.1은 Clang도 받음).
  - 고르지 않으면: C1 Clang 빌드는 앱 5개가 적재되지 않아 판정에 쓸 수 없다.
  - 이번에는 T3을 차이로 기록하고 썼다.
- T4(compiler-rt profile runtime을 같은 LLVM 소스에서 빌드)는 PD26 §1의 환경 변경 규칙(필요 기능 부재 → 설치·기록)으로 처리했다. 규칙의 적용이며 새 결정은 아니다.
- 미실시:
  - L-I0/L-I(같은 바이너리의 mode off/record) 비교와 hook 비용
  - L-A 대 G-A의 컴파일러 효과
  - 제어 hook의 실제 보류(`control` 두 곳은 RECORD 모드라 평가되지 않음)
  - dump → `.kzt` 변환과 `kz replay` 판정
  - OSAL 내부 점의 binding
  - LTO
  - 교차 target
- 세 후보 비교: GCC plugin(`instr_candidate_gcc.md`), Clang 소스 변환 → GCC(다른 작업), 이 문서를 IR §15.1의 세 줄로 비교해 고른다(PD26 §2.3).

## 11. 재현

```sh
# plugin과 단위 시험 (고정 LLVM 빌드 필요)
cmake -S kairos/instrument/llvm -B /home/user/work/instr/llvm/plugin-build && cmake --build /home/user/work/instr/llvm/plugin-build && ctest --test-dir /home/user/work/instr/llvm/plugin-build
P=/home/user/work/instr/llvm/plugin-build; L=kairos/instrument/llvm/scripts; G=kairos/instrument/gcc/scripts
# R2용 profile runtime (환경 변경, §1): compiler-rt를 트리 밖에서 profile만 빌드
cmake -G Ninja -S /home/user/work/llvm-project/compiler-rt -B /home/user/work/instr/llvm/crt-profile/build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=$CLANG -DCMAKE_CXX_COMPILER=$CLANG++ -DCMAKE_C_COMPILER_TARGET=x86_64-unknown-linux-gnu \
  -DLLVM_CMAKE_DIR=/home/user/work/llvm-project/build/lib/cmake/llvm -DCOMPILER_RT_DEFAULT_TARGET_ONLY=ON -DCOMPILER_RT_BUILD_PROFILE=ON \
  -DCOMPILER_RT_BUILD_{BUILTINS,SANITIZERS,XRAY,LIBFUZZER,MEMPROF,ORC,CTX_PROFILE,GWP_ASAN}=OFF -DCOMPILER_RT_INCLUDE_TESTS=OFF
ninja -C /home/user/work/instr/llvm/crt-profile/build profile   # → resource/lib/linux/libclang_rt.profile-x86_64.a 로 복사
# C1: 사본, L-A, (옮긴 뒤) L-I, binding 합치기, 부팅, 대조
$L/build_c1.sh prepare /home/user/work/instr/llvm/c1
$L/build_c1.sh build /home/user/work/instr/llvm/c1 la        # 끝나면 ws/build, ws/exe를 build_la, exe_la로 옮김
$L/build_c1.sh build /home/user/work/instr/llvm/c1 li $P/kzllvm.so $P/kzrt
$G/merge_binds.sh /home/user/work/instr/llvm/c1/binds_li /home/user/work/instr/llvm/c1/binds_li.kzs
$L/run_c1.sh /home/user/work/instr/llvm/c1/ws/exe /home/user/work/instr/llvm/c1/run1 $P/kzrt/kz-rt-dump $P/kz-events /home/user/work/instr/llvm/c1/binds_li.kzs
$L/compare.sh c1 <kz-dump of c1_runs/smoke3/post_1df262a/run.kzt> /home/user/work/instr/llvm/c1/run1/tx.events.tsv /home/user/work/instr/llvm/c1/cmp1
# 기준 빌드와의 명령 대조: $L/cmp_compile_commands.sh, $L/cmp_link_commands.sh (인자는 각 스크립트 머리말)
# R2: 같은 순서 (build_r2.sh, run_r2.sh는 root로, CFS_NORMAL_USER=ubuntu)
```

산출물 해시(저장소 밖):
- C1: L-I `core-tx` `cc519c7a…`, `cf.so` `1155e9b5…`, `bp.so` `8efe48e6…`, L-A `core-tx` `9658e508…`, dump `c2e25583…`
- R2: L-I `core-cpu1` `9bbf18cd…`, L-A `core-cpu1` `ae2656c1…`, dump `bec07d1a…`
- plugin `e758c7e5…`

큰 빌드 트리는 기록을 남긴 뒤 지웠다(binding 파일, build record, 명령·객체 해시, 실행 메타·dump·대조 결과는 남김).

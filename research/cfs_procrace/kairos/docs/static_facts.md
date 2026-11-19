# 정적 추출층: `kz-static basis` / `extract` (K4a)

이 문서는 S3a-R2 의 추출층(SPEC §1.7, §4.1, §4.2, §4.3 의 일부)을 설명한다. 코드는 `lib/llvm/`, `tools/kz-static/`, 레코드 배치는 `include/kz/schema/kz_facts.h`, 시험은 `tests/static/` 에 있다.

표기: **[M]** 이번 구현에서 실행해 확인, **[D]** 설계 결정, **[I]** 계획·미확인. 이 문서의 수치는 프로그램(core-cpu1)을 분석한 값이고, cFS 를 실행한 결과가 아니다. K4a 는 cFS 를 실행하지 않았다.

## 1. 하는 일과 하지 않는 일

| 단계 | SPEC | 이 구성요소 | 상태 |
|---|---|---|---|
| 분석 basis B 기록 | §1.7 | `kz-static basis`: 소스 트리 commit, compile DB, link 명령과 그 member, 컴파일러·플래그, 생성 파일 해시, 핵심 표의 초기값, 상수, POWERON 의 시작 스크립트 선택 규칙 | 구현 [M] |
| 번역 단위 읽기 | §4.1 1–2 | Clang LibTooling 으로 기록된 명령을 다시 읽음(분석 전용 인자 3개 추가, deviation 으로 기록) | 구현 [M] |
| 호출 지점 사실, `derives` | §4.1 3 | 인자 분류(상수·기호·주소·함수·top), 결과 사용·검사, 루프 문맥, 역할 있는 인자의 copy chain | 구현 [M] |
| 상수·표 | §4.1 4 | 표의 static initializer image(정수·문자열·주소·함수), 상수의 단위 내 평가 | 구현 [M]. `.tbl` 바이트 대조는 **미구현** |
| 문서 주석 후보 | §4.1 5 | — | **미구현** |
| link unit / app-unit probe | §4.1 6 | basis 의 link set(archive 의미, 지연 member). recipe `module` 이 있으면 시작 스크립트가 적재하는 `.so` 단위를 실행 파일과 한 닫힘으로 묶고, 단위마다 미정의 기호와 적재기 결합을 기록(§3.1) | 구현 [M](소스 수준: 이름 결합 + 적재기 결합 대조). IR 수준 link, `-fcallgraph-info` 대조는 **미구현** |
| 루프 사실 | §4.1 8, §4.3 | 종류, 횟수, 탈출식, mod set, 예산, 잠금 구간, 조건 탈출, 술어, 인증 뷰 규칙, table walk 펼치기 | 구현 [M] (§5) |
| writer 집합 | §4.2 1–3 | field 기반 writer(대입·증감·통째 대입·memset 계열·out 포인터), A-TYPE 목록, INIT image | 구현 [M] |
| transition filtering, Φ 대응, key 상태 | §4.2 6–8 | — | **하지 않음**: census(K4b) 몫. writer 의 `tstatus` 는 `n/a` |
| skeleton(두 뷰), once, MustBefore | §4.3–4.4 | 필요한 사실만 만든다 | **K4b** |
| anchor audit | §3.17 | — | **미구현** (`ANCHORS` 섹션은 비어 있음) |

## 2. 사용법

LLVM 이 필요한 도구라서 `-DKZ_BUILD_LLVM_TOOLS=ON` 으로 configure 한 빌드에만 있다(기본은 OFF). 고정된 LLVM/Clang 빌드(`/home/user/work/llvm-project/build`, LLVM 23.0.0git)를 쓰고 패키지는 설치하지 않는다.

```sh
SRC=/home/user/Shape-Perf/research/cfs_procrace/kairos
B=/home/user/work/kairos-build/k4a-llvm
cmake -S $SRC -B $B -G Ninja -DKZ_BUILD_LLVM_TOOLS=ON -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
cmake --build $B && ctest --test-dir $B -L static

BUILD=/home/user/work/procrace/cfs_ref/cFS/build-native_std
$B/tools/kz-static/kz-static basis --profile $SRC/profiles/cfs-v7.0.1 \
    --compile-db $BUILD/native/default_cpu1/compile_commands.json --build-dir $BUILD \
    --bundle-root /home/user/work/procrace/cfs_ref/cFS --recipe $SRC/profiles/cfs-v7.0.1/basis/core-cpu1.kzb -o basis.kz
$B/tools/kz-static/kz-static extract --profile $SRC/profiles/cfs-v7.0.1 --basis basis.kz \
    --delay-op OS_TaskDelay --lock-op CFE_ES_LockSharedData --unlock-op CFE_ES_UnlockSharedData \
    --spawn-op CFE_ES_CreateChildTask -o facts.kz
$B/tools/kz-static/kz-static show facts.kz --section loops --section order
$B/tools/kz-static/kz-static reach facts.kz --root CFE_ES_TaskMain --loc CFE_SB_Global_t.AppId --callee CFE_EVS_SendEventWithAppID
```

- `show` 의 section: `basis files functions calls derives loops iters order init consts writers atype locs targets spawnparams image terminals outcalls fwrites`, 컨테이너에 있으면 `p0 derived spawnwrap`(§5a)와 `reads returns retconsts loopexits`(§5b). `--loc Type.field`, `--callee NAME` 으로 거른다(`--loc` 은 `reads` 에도 걸린다).
- 프로필 없는 추출(§5a): `basis --no-profile …`, `extract --no-profile --basis basis.kz -o facts.kz`. `--profile` 과 어휘 옵션(`--target`, `--delay-op`, `--lock-op`, `--unlock-op`, `--spawn-op`, `--terminal-op`, `--poll-fn`)은 함께 쓸 수 없다(종료 코드 2).
- 확장 사실 F1–F3(§5b): 프로필 없는 추출은 늘 쓴다. 프로필 추출은 `extract --extended-facts` 를 줄 때만 쓴다. 주지 않으면 facts 는 예전 그대로다.
- 종료 코드: 0 정상, 1 분석 실패(번역 단위가 parse 되지 않음, 컨테이너 손상, 읽기 옵션이 basis 와 다름), 2 사용법·입출력 오류.
- `extract --basis` 는 제 읽기 옵션(`--resource-dir`, `--extra-arg`)과 프런트엔드를 basis 의 것과 대조하고 다르면 거부한다. `--accept-option-difference` 는 추출하고 그 차이를 편차로 적는다(§5q).
- 고정 도구(fixture)처럼 compile DB 없이 읽을 때는 `--source FILE ... -- <컴파일 플래그>` 를 쓴다.

**무엇을 찾는가는 데이터다** [D]. 추출 대상 필드(`loc`, `bind`, store pattern), 역할이 있는 인자(`op` 의 매개변수 역할), 종료 연산(`terminal`), spawn 연산(`hb` 의 spawn 템플릿), poll 함수는 프로필에서 온다. 프로필 문법(Φ)에 아직 **지연(delay) 연산과 잠금 연산을 선언하는 수단이 없어서**, 이 넷(`OS_TaskDelay`, `CFE_ES_LockSharedData`, `CFE_ES_UnlockSharedData`, 그리고 프로필의 spawn 템플릿이 아닌 `CFE_ES_CreateChildTask`)은 명령줄로 준다. 명령줄 항목과 C 라이브러리 데이터 함수 표(12개)는 사람이 넣은 입력이라 facts 컨테이너의 BASIS 항목에 정책 `declared-assumption`, 출처 계층 `researcher` 로 기록되고 요약에 "hand-entered inputs" 로 센다. 엔진 코드(`lib/`, `include/`)에는 cFS 이름이 없다(lint `lint_case-ids` 와 별도 grep 으로 확인 [M]).

`--profile` 은 되풀이할 수 있다(2026-10-24): 공통 디렉터리와 변형 디렉터리로 된 프로필(예: `profiles/c1-bp-b051eb0/` + `cf-…` + `bundle-…` + `rspec-…`)을 `kz replay --profile` 과 같은 순서로 읽는다. facts 의 프로필 해시가 재생 프로필의 해시와 같아야 `kz replay --facts` 가 받으므로, C1 의 닫힘은 rspec 변형마다 추출한다(`docs/c1_smoke_observe.md` §9.2).

## 3. basis 컨테이너(SPEC §1.7)

`kz-static basis` 는 artifact 종류 BASIS 의 KZ 컨테이너를 쓴다. 이 컨테이너의 artifact 해시가 B 의 해시이고, `extract --basis` 로 만든 facts 컨테이너는 헤더의 basis 해시와 부모 목록에 그것을 싣는다.

| 항목 | 내용 | core-cpu1 [M] |
|---|---|---|
| Build: commit | 프로필 `source` 선언의 commit 과 저장소 HEAD 를 비교. 다르면 deviation | cFS `088b2fa8`, cFE `c5fb2b4d`, OSAL `d2d877a6`, PSP `c4b3b0b6`, 모두 선언과 같음 |
| Build: 프로그램 | link 명령(`link.txt`)을 archive 의미로 풀어 실제로 들어가는 object 를 정하고, compile DB 와 대응 | compile DB 1417 항목, link 입력 28(archive 21), member 173 중 168 이 프로그램에 들어감(지연 member 5 는 들어가지 않음), 번역 단위 168, 서로 다른 플래그 집합 20 |
| Build: 컴파일러 | 기록된 컴파일러와 바이너리의 `.comment` | `/usr/bin/cc`, `GCC: (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`; 바이너리 sha256 과 build-id 기록 |
| Build: 생성 파일 | recipe 가 이름 붙인 생성 파일의 해시 | `cfe_static_module_list.c`, `cfe_module_version_table.c`, `cfe_static_symbol_list.c`, `cfe_psp_module_list.c`, `cfe_build_env_table.c` |
| Build: 표 | 생성 파일 안 표의 초기값(Clang 이 평가) | `CFE_CORE_MODULE_LIST` 13, `CFE_CORE_MODULE_OBJTAB` 10(CONFIG, ES, EVS, FS, SB, TBL, TIME, MSG, SBR, RESOURCEID), `CFE_MODULE_VERSION_TABLE` 35 |
| Config: 상수 | 프로필의 `const ... = eval` 과 recipe 의 파일 상수를 **사용하는 번역 단위 안에서** 평가 | `CFE_PLATFORM_CORE_MAX_STARTUP_MSEC` 30000, `CFE_PLATFORM_ES_MAX_APPLICATIONS` 32, 시작 스크립트 이름 둘 |
| Config: 시작 스크립트 | recipe 의 규칙(PROCESSOR: volatile 다음 nonvolatile, POWERON: nonvolatile)을 reset 종류 POWERON 으로 평가하고, `/cf` 를 빌드의 `exe/cpu1/cf` 로 대응해 실제 파일을 찾음 | `exe/cpu1/cf/cfe_es_startup.scr`, 라이브러리 2 + 앱 15. 규칙의 근거 줄(cfe_es_apps.c:111–200, cfe_psp_start.c:374–383, :449)은 본문 해시와 함께 PROVENANCE |
| 설정 사실(확인 대상) | 모듈 버전 표가 없을 때 관찰 사용 수가 24→11 로 바뀐다는 기록 | `config.module_version_table.effect_on_observed_uses`: **declared-assumption, "claim, unchecked"**. 관찰이 아니라 확인할 설정 사실로만 둔다 |
| Env: 읽는 front end | Clang LibTooling 23.0.0git. 기록된 명령에 `-Wno-unknown-warning-option -Wno-error -w -resource-dir=…` 를 덧붙임 | **deviation** (값, 이유, 영향 기록). gcc 와 Clang 의 내장 함수·미리 정의된 매크로 차이는 배제하지 않았다 |

손으로 넣은 basis 항목은 recipe(`profiles/cfs-v7.0.1/basis/core-cpu1.kzb`)에 있고 출처 계층 `researcher` 로 기록된다(reset 종류 1, 설정 사실 1).

**조건 기록(CONDITIONS_POLICY)** [M]:

| 항목 | 레퍼런스 | 사용 값 | 이유 | 영향 |
|---|---|---|---|---|
| compile DB 경로 | SPEC §4.1: cpu1 compile DB(1417 항목). 작업 지시문은 `build-native_std/compile_commands.json` 을 적었다 | `build-native_std/native/default_cpu1/compile_commands.json`(1417 항목, basis 항목 `compile_db.path`·`compile_db.entries`) | 지시문의 파일은 mission 빌드의 host 도구 14 항목뿐이고 cpu1 번역 단위가 없다. 사용한 DB 는 SPEC 이 말하는 것과 같다(레퍼런스와의 차이는 없음, 지시문의 경로만 다름) | 없음 |
| front end | 기록된 명령(gcc 13.3.0) | Clang 23.0.0git LibTooling | SPEC §4.2 가 필드 정체성이 명시된 AST 를 요구 | basis 의 deviation 기록 그대로 |
| 실행 횟수 N | — | 해당 없음 | K4a 는 cFS 를 실행하지 않는다 | — |

### 3.1 적재 모듈 닫힘(recipe `module`, SPEC §4.1 6, D5)

실행 파일 하나의 link 단위로는 `dlopen` 으로 적재되는 앱이 닫힘 밖이다. 앱이 넘긴 주소를 받는 OSAL 함수의 본문이 앱 단위에 없으면 그 필드는 address escape 로 DISABLED 가 된다(C1 W3, `plan/C1_build_manifest.md` §8). recipe 에 다음을 더하면 basis 가 실행 파일과 **선택된 시작 스크립트가 적재하는 모듈**을 한 프로그램으로 묶는다.

```
binary "../exe/tx/core-tx";                                   # 실행 파일이 내보내는 기호를 여기서 읽는다(필수)
module "cf" link "native/default/apps/cf/CMakeFiles/cf.dir/link.txt";   # 이름 = 스크립트 줄의 파일 이름 필드
startup_script {
  ...
  entry library "CFE_LIB" global;   # 종류별 기호 범위(적재기 규칙, cite 로 근거를 단다)
  entry app "CFE_APP" local;
  module_file field 1 path const "CFE_PLATFORM_ES_NONVOL_DISK_MOUNT_STRING" extension const "CFE_DEFAULT_MODULE_EXTENSION" in "cfe/cmake/target/src/target_config.c";
}
```

basis 가 하는 일 [D]:
1. 스크립트 줄마다 파일 이름 필드(`module_file field`)에 경로가 없으면 기본 경로를, 확장자가 없으면 기본 확장자를 붙인다(두 값은 사용 단위에서 상수 평가). `mount` 로 빌드의 파일에 대응한다.
2. 같은 이름의 `module` 이 있으면 그 link 명령을 archive 의미로 풀고, **link 가 쓴 파일과 스크립트가 적재하는 파일의 바이트가 같을 때만** 그 member 를 프로그램에 넣는다(다르면 넣지 않고 경고). 번역 단위의 `member` 에 `module cf (app, symbols local), …` 가 남는다. `module` 이 없는 줄(예: compile DB 에 없는 `tinycbor`)은 닫힘 밖이며, 적재 파일의 `.dynsym` 이 내보내는 기호만 결합 판단에 쓴다. 스크립트가 적재하지 않는 `module` 은 넣지 않는다.
3. **적재기 결합**: 모듈의 미정의 기호(member 의 미정의 − 정의) 중 적재 파일의 `.dynsym` 에 미정의로 남은 것을 실행 파일의 `.dynsym` 정의 → 그보다 먼저 적재된 global 라이브러리 순으로 찾는다(`module[x].bound[...]`, `.unbound`, `.resolved_by_its_link`). 시스템 라이브러리(DT_NEEDED)는 읽지 않으므로 `unbound` 는 "적재된 단위 중 내보내는 것이 없음"이다(`closure.binding_model`).
4. **이름 결합 대조**: extract 는 합친 프로그램에서 외부 함수·객체를 이름으로 결합한다. 이것이 적재기 결합과 같으려면 (a) 닫힘 안의 두 단위가 같은 이름을 정의하지 않고, (b) 단위 사이 참조마다 적재기가 그 이름을 정의한 단위로 결합해야 한다. 어긋나면 deviation 으로 기록한다(`closure.duplicate_definitions`, `closure.name_binding`).

**재생에서 쓰는 곳 (2026-10-24, R2 잔여)**: `kz replay --facts <facts.kz> --static-basis <basis.kz>` 가 FACT_LOCS·WRITERS·FACTS·FACT_ATYPE·CALL_TARGETS 를 읽어, 프로필이 묶은 필드가 모두 `writer-set-complete` 이면 별칭 포괄을, 그에 더해 모든 writer 가 기록의 관측 지점에 있으면 writer 포괄을 세운다(`include/kz/trace/static_closure.hpp`, `docs/observer.md` §3.4). 이 문서 §8 의 가정 G1–G4 가 그대로 따라간다.

extract 쪽은 바뀌지 않는다: basis 의 번역 단위 표를 읽으므로 앱의 호출이 OSAL 본문의 out 포인터 요약으로 이어진다. 남은 차이: SPEC 의 IR 수준 link(bitcode)와 gcc `-fcallgraph-info` 대조는 하지 않았고, 표 객체(`.tbl` 대상)는 모듈 link 에 들어 있지 않아 따로 제외하지 않는다. C1 결과는 `plan/C1_build_manifest.md` §8.5 에 있다.

## 4. facts 컨테이너

artifact 종류 FACTS. 모든 사실은 (file, line, col) 과 `fh`(같은 컨테이너 FILE_HASHES 의 색인: 그 파일의 SHA-256 과 크기)를 갖는다. 표는 위치·키로 정렬하고, 같은 입력은 같은 바이트를 낸다(두 번 실행해 바이트 비교, golden 시험 [M]). 시각·호스트 정보는 없다.

| 섹션 | 레코드 | 뜻 |
|---|---|---|
| FUNCS (20) | `kz_fact_func` | 함수(정의·선언), 내부 연결 함수는 `이름@파일:줄` |
| FACTS (10), FACT_ARGS (11) | `kz_fact_call`, `kz_fact_arg` | 호출 지점, 인자 |
| DERIVES (12) | `kz_fact_derives` | 역할 있는 인자마다 copy chain(T-DERIVE), 실패 이유 |
| FACT_EXPRS (13) | `kz_fact_expr` | 식 트리(탈출식, mod set, guard, chain, 술어). 공유되는 부분 트리는 한 번 |
| FACT_LOOPS (14) | `kz_fact_loop` | 루프 사실과 인증 뷰 규칙(§5) |
| LOOP_ITERS (37) | `kz_fact_iter` | table walk 의 펼친 반복(§5.3) |
| FACT_INIT (15) | `kz_fact_init` | INIT: 프로그램 진입 상태(정적 초기값, 0 초기화) |
| FACT_CONSTS (16) | `kz_fact_const` | 상수 경계 |
| FACT_ATYPE (17) | `kz_fact_atype` | A-TYPE 목록(untyped write, address escape, cast, union, unknown-effect) |
| WRITERS (18) | `kz_fact_writer` | 대상 필드의 writer. `tstatus` = `n/a`(아래) |
| FACT_LOCS (33) | `kz_fact_loc` | 대상마다 O7 행: writer 수, A-TYPE 수(종류별, unknown-effect 포함), 상태 `writer-set-complete` / `DISABLED` |
| CALL_TARGETS (31) | `kz_fact_target` | 간접 호출과 spawn 의 대상(함수 포인터 흐름 그래프) |
| SPAWN_PARAMS (38) | `kz_fact_spawn_param` | spawn 하는 함수의 task-main 매개변수(§5.4) |
| STATIC_IMAGE (32) | `kz_fact_image` | walk 가 읽는 표의 초기값 |
| TERMINALS (34) | `kz_fact_terminal` | 종료 연산 호출과 그것을 둘러싼 조건 |
| OUTCALLS (35) | `kz_fact_outcall` | 대상 필드의 주소를 넘기는 호출과 callee 요약 |
| FUNC_WRITES (39) | `kz_fact_fwrite` | 함수 본문이 직접 쓰는 필드(첫 지점). 루프 body 의 전이적 mod set 을 컨테이너에서 다시 계산할 수 있게 둔다 |
| BASIS, FILE_HASHES, PROVENANCE | — | 추출 매개변수, 수, 손으로 넣은 입력(계층 `researcher`) |
| P0_TABLE (120) | `kz_fact_p0` | 프로필 없는 추출만: P0 표의 항목(종류, 매개변수 위치, 판 표기, 원문 대조)과 이 프로그램의 호출 수(§5a) |
| OP_DERIV (121) | `kz_fact_opderiv` | 프로필 없는 추출만: F8 이 유도한 지연·종료 연산. 함수와 그 안의 호출마다 must/noreturn/may, guard, 호출 사슬(§5a) |
| SPAWN_WRAP (122) | `kz_fact_spawnw` | 프로필 없는 추출만: F7 이 유도한 spawn 연산과 시작 스크립트 진입의 해결(§5a) |
| READS (123) | `kz_fact_read` | 확장 사실(§5b): 필드·전역 읽기. 용도, 접근 경로의 뿌리와 첨자, guard |
| RETURNS (124) | `kz_fact_return` | 확장 사실(§5b): return 값. 돌려주는 지역 변수는 그 return 에 닿는 정의마다 한 행, 값과 조건 |
| RET_CONSTS (125) | `kz_fact_retconst` | 확장 사실(§5b): 함수마다 돌려주는 상수와 그 조건 |
| LOOP_EXITS (126) | `kz_fact_loopexit` | 확장 사실(§5b): 모든 루프의 탈출식·mod set 과 탈출마다의 조건 |
| ARGFLOW … GLOBAL_IMAGES (127–130) | | 확장 사실(§5c) |
| LOCK_OPS (131) | `kz_fact_lockop` | 확장 사실(§5d): P0 mutex 기본 연산에서 래퍼로 유도한 잠금 연산과 그 정체성 |
| LOCK_IDS (132) | `kz_fact_lockid` | 확장 사실(§5d): LOCKSET 이 잡는 잠금 정체성과 정적 인스턴스 여부 |
| LOCKSET (133) | `kz_fact_lockset` | 확장 사실(§5d): 모든 호출·읽기·쓰기에서 잡혀 있는 잠금(must) |
| FIELD_STATUS (134) | `kz_fact_fstatus` | 확장 사실(§5d): 대상에 무관한 두 규칙의 writer 집합 완전성 |
| AWAIT_L1 (135) | `kz_fact_await` | 확장 사실(§5d): 대상에 무관한 L1 (i)과 LOCKSET 의 L1 (ii) |

schema 버전: FACT_LOOPS 와 FACT_EXPRS 는 3, FACT_ATYPE 은 3(`unknown-effect` 종류), FACT_LOCS 는 2(`n_unknown_effect`), 새 섹션 37–39 는 1. writer 에 `partial-write`, 루프에 `static-image-assumed` 표시가 있다(revision 4). 섹션 120–122 는 1이고, TERMINALS 에 `conditional` 표시가 있다(revision 5, 스키마 개정 7). 섹션 123–126 은 1이다(revision 6, 스키마 개정 8). 섹션 127–130 은 1이다(revision 7, 스키마 개정 9). 섹션 131–135 는 1이다(revision 8, 스키마 개정 10). 바뀐 점은 `kz_facts.h` 머리 주석에 있다.

### 4.1 writer, A-TYPE, INIT (SPEC §4.2)

- 추상 위치는 (struct 형, field) 이고 base 객체와 무관하다 [D]. writer: `MemberExpr` 대입·복합 대입·증감, struct 통째 대입(그 형이 담은 모든 대상), 데이터 함수(memset 계열: 목적지 형이 담은 모든 대상, 상수 0 채움은 값 0 으로 기록), out 포인터(매개변수 저장 요약을 호출 지점에서 실체화; 저장마다 guard 와 값).
- **guard** 는 그 문장이 실행될 때 성립해야 하는 조건의 논리곱이다. `if (c) <탈출>;`(else 없음, `break`/`continue`/`return`/`goto` 로 끝남) 뒤의 문장에는 `¬c` 가 붙는다 [D]. 라벨과 `case` 는 합류점이라 거기서 떨어뜨린다. 각 조건은 **자기 위치에서 쓰인 그대로**이며, 한 상태 위의 술어로 읽으면 안 된다(사이에 지역 변수가 바뀔 수 있다).
- **A-TYPE**: cast(대상 형을 담은 형의 포인터와 다른 형 사이의 변환; 데이터 함수의 목적지·원본 인자는 제외. **const view 도 제외하지 않는다**: C 는 뒤에서 const 를 떼는 cast(`(char *)v`)를 허용하고 그 cast 는 대상 형을 언급하지 않는다), 대상 필드 주소의 escape(호출 인자 외의 사용, 저장만 하지 않는 callee, 필드의 **일부**(`&x.f.g`) 주소를 데이터 함수의 목적지·원본 밖으로 넘김), union 멤버 쓰기, untyped write(`void*`/`char*` 를 통한 저장, 데이터 함수), **unknown-effect**(아래). untyped write 는 대상 형에 cast·escape·union·unknown-effect 노출이 있을 때만 그 대상에 센다: 노출이 없으면 untyped 포인터가 그 형의 객체를 가리킬 방법이 없다 [D]. 하나라도 걸리면 그 대상은 `DISABLED`.
- **unknown-effect** (SPEC §4.2 2 단계 마지막 항목, 3 단계) [D]: 대상의 소유 형에 **닿는** 포인터(그 형을 가리키거나, 값으로 담은 record 를 가리키거나, record 의 포인터 필드를 따라 전이적으로 닿는 포인터; 포인터 단계 수 무관)가 (1) 닫힌 프로그램에 본문이 없는 함수, (2) 대상을 모두 알지 못하는 간접 호출(미해결이거나 해결된 대상 중 본문이 없는 것), (3) 가변 인자 위치(`va_arg` 읽기는 따라가지 않음)로 넘어가면 그 형의 모든 대상에 hit 를 기록한다. inline assembly 는 피연산자가 대상 형에 닿거나 대상 필드이면 그 대상에, `"memory"` clobber 가 있으면 **모든** 대상에 hit 다. 데이터 함수(memset 계열)는 요약이 쓰는 곳(목적지)을 정하므로 제외한다. 본문이 있는 callee 는 그 본문의 field 사실이 덮으므로(writer 수집은 닫힌 프로그램의 모든 본문을 읽는다) hit 가 아니다. 그래서 `unresolved-indirect` 수는 정보용이고 그 자체로 상태를 정하지 않는다: 미해결 호출은 대상에 닿는 포인터나 대상 필드의 주소를 받을 때만 hit 가 된다.
- **데이터 함수의 범위** [D]: "목적지 형의 모든 필드를 쓴다"는 크기 인자가 그 형의 크기 이하일 때만 쓰기 전체다. 크기 인자(목적지·원본이 아닌 첫 `size_t` 매개변수)를 상수로 평가해 목적지 영역 크기와 비교하고, 넘거나 상수가 아니거나 크기 매개변수가 없으면(`strcpy`, `sprintf`) 둘러싼 객체(`.` 멤버와 배열 첨자를 거슬러 올라간 가장 바깥 lvalue, `->`·`*p` 이면 그 포인터가 가리키는 record)가 담은 모든 대상의 writer(`covers-more-than-the-field`)로 기록한다. 상수 크기가 둘러싼 객체의 끝을 넘으면 형이 이름 붙이지 않은 메모리를 쓰므로 모든 대상에 unknown-effect hit 다. 목적지가 `T *` 포인터이면 크기가 `T` 의 배수일 때 `T` 배열로 본다(같은 위치들).
- **record·배열 형 대상** [D]: 대상 필드의 일부에 대한 저장(`x.f.g = v`, `x.f[i] = v`, 증감)은 그 필드의 writer 이고 `partial-write` 표시, 값은 top 이다. out 포인터 매개변수 `p` 에 대해 `(*p).g`, `p->g`, `p[i].g` 저장은 pointee 에 대한 저장으로 요약에 들어가고, 그 멤버의 주소를 잡으면 escape 다. 데이터 함수의 목적지가 대상 필드의 일부이면 그 대상의 `partial-write` writer 다.
- writer 의 `tstatus` 는 **`n/a`** 다 [D]. SPEC §4.2 의 COMPUTED 는 6–7 단계(conjunct 에 대한 transition filtering 과 Φ 대응)를 거친 항목이고, 추출기는 2–3 단계만 했다. 이전 판(revision 2)은 모든 writer 를 COMPUTED 로 썼는데 과장이라 고쳤다.
- INIT: 전역 객체의 정적 초기값 또는 0 초기화. EarlyInit 저장은 INIT 이 아니라 보통 writer(`early-init` 표시)다(SPEC §1.7).

### 4.2 derives

역할(stamp / key / id)이 있는 인자마다 한 행: 공유 위치(struct field, 전역, 전역 배열의 원소 — 이때 위치는 배열)를 읽은 값이 산술 없이 지역 변수 복사만 거쳐 인자에 오는지. 지역 변수는 정의가 하나이고 주소가 잡히지 않아야 한다. 복사로 보는 변환은 괄호, lvalue→rvalue, no-op, 그리고 정보를 잃지 않는 정수 변환(폭이 같거나 넓어짐)뿐이다. 좁히는 정수 변환(`(uint16)x`), bool 변환과 그 밖의 cast 는 `not-a-copy` 다 [D]. 실패하면 이유(정의 여러 개, 복사 아님, 매개변수에서 옴, 호출 결과 등)를 쓴다.

### 4.3 식 트리의 지역 변수

식 트리는 지역 변수를 이름 문자열로 적는다. 한 함수 안에서 같은 이름의 지역 변수가 둘 이상이면(가림, 형제 블록의 재사용) 선언 위치를 붙인 `이름@줄:열` 로 구별하고, 한 번만 쓰인 이름은 그대로 둔다. 술어 추출(counter, cursor, 예산)은 트리의 이름을 이 id 표로 선언에 되돌린다. id 는 선언에만 의존하고 포인터 값에 의존하지 않는다(이전 판은 포인터 순서의 map 에서 첫 이름 일치를 골라, 가림이 있으면 결과가 할당 순서에 따라 달라질 수 있었다).

## 5. 루프와 인증 뷰 규칙(SPEC §4.3)

### 5.1 루프 종류

| 종류 | 판정 |
|---|---|
| table-walk | `V = Table[i]; if (V == NULL) break;` 꼴. 경로(전역 객체와 필드)를 static image 위에서 따라가 NULL 원소까지 센다. image 가 실행 시 값인지 확인한다(§5.3) |
| constant-trip | `for (i = c0; i <op> bound; i += step)`, bound 는 상수, 본문이 i 를 바꾸지 않음 |
| count-bounded | constant-trip 이면서 **자기 본문에** 지연 연산 호출이 있음(delay-paced) |
| poll | 횟수를 모르고 자기 본문에 지연 연산 호출이 있음 |
| other | 나머지 |

안쪽 루프에만 지연 호출이 있는 루프는 poll/count-bounded 가 아니다(이전 판은 그렇게 분류했다).

### 5.2 규칙(`cert`)과 L1 의 전제

- **unroll**: 횟수가 정확한 루프(table-walk, constant-trip, count-bounded). count-bounded 에는 "delay-paced" 를 적는다.
- **await**: poll 루프 중 (1) 조건 탈출에서 인식된 술어 형태를 얻었고, (2) L1 (ii) 시험 읽기가 한 잠금의 한 임계 구역 안이거나 한 번의 원자적 읽기이며, (3) L1 (i) 본문(호출하는 함수 포함)이 관련 위치를 쓰지 않는 경우. **lemma L1 자체는 여기서 결정하지 않는다**(증명 상태는 certificate 몫). 기록은 "L1 의 전제가 추출된 사실 위에서 성립"까지다.
- **star-closed**: 그 밖. poll 이면 "unmodelled-sync" 와 이유를 쓴다.

세부:
- **조건 탈출과 예산 탈출**: 탈출을 결정한 조건(탈출 지점의 가장 안쪽 guard 또는 부정된 루프 조건)을 선언항으로 나누고(`not (and a b)` 는 `¬a ∨ ¬b`), 예산 변수(감소되고 탈출식이 검사하는 지역 변수)를 언급하는 선언항은 예산 탈출, 나머지는 조건 탈출이다. 조건 탈출에서 예산만 검사하는 논리곱 항은 뺀다. 상수 거짓 탈출(`while (1)`)은 버린다.
- **술어 형태**: (a) 지역 변수를 읽지 않는 직접 시험, (b) counter-zero: 본문 첫 수준의 무조건 `cnt = 0`, 바로 안쪽의 정확한 횟수 N 의 constant-trip 루프(탈출 없음) 안에서만 `++cnt`, 그 밖의 정의 없음, 그리고 `cnt` 를 검사하는 탈출이 모두 안쪽 루프 **뒤**에 있고 루프 조건은 `cnt` 를 읽지 않음 → `cnt == 0` 을 `forall i N (not φ) cursors` 로 바꾼다. φ 가 읽는 지역 변수는 안쪽 루프의 유도 변수나 **cursor**(안쪽 루프 전에 한 번 `p = base`, 안쪽 루프 본문 첫 수준에서 매 반복 `++p`)여야 한다. cursor 의 `base` 가 크기를 아는 배열이면 길이를 함께 쓴다. (c) (a)와 (b)의 조합.
- **L1 (ii)**: 본문의 공유 읽기(필드·전역의 lvalue→rvalue 변환; `x.f` 의 base 나 배열 decay 는 읽기가 아님)가 모두 잠금 구간 안이고 잠금 호출이 하나이면 one-lock-region. 공유 읽기가 하나이고 잠금 밖이며 `_Atomic` 이거나 C 의 `sig_atomic_t` 로 쓰인 `volatile` 이면 atomic-read. 잠금 연산은 명령줄 입력이다. 술어(또는 조건)에 호출 결과(`call-result`)가 있으면 callee 가 읽는 것은 위 수에 보이지 않으므로 [D]: atomic-read 는 성립하지 않고, one-lock-region 은 그 호출이 모두 그 루프 안에서 잠금 구간 안에서만 이루어질 때만 성립한다(간접 호출 결과는 성립하지 않음). callee 본문 안의 읽기를 세지는 않는다: 잠금 구간 안의 호출이면 그 읽기도 구간 안이다.
- **L1 (i)**: 관련 위치 = 술어가 직접 읽는 필드·전역(크기를 아는 배열 cursor 의 base 는 제외) ∪ 모든 추출 대상. 본문 쓰기(FUNC_WRITES 행 중 그 루프 안의 것)와 본문 호출에서 도달하는 함수(직접 호출, 해결된 간접 호출)의 쓰기를 모은다. `T.*`(record 통째 쓰기)는 `T` 의 모든 필드에 걸린다. 확실한 쓰기가 있으면 "L1 (i) fails" 다. 쓰기를 찾지 못했어도 다음이면 `mod-incomplete`(결정하지 않음): 해결되지 않은 간접 호출에 닿음, 관련 위치가 **추출 대상이 아님**(A-TYPE 목록이 없어 포인터·cast·untyped view 를 통한 쓰기를 배제할 수 없다), 관련 대상이 `DISABLED`. 그래서 await 는 술어가 읽는 위치가 모두 writer 집합이 complete 인 추출 대상일 때만 나온다 [D]. 술어가 호출하는 함수(예: `IsUsed`)가 읽는 필드는 그 필드가 추출 대상일 때만 관련 위치에 든다 [D].

### 5.3 table walk 펼치기

**image 가 실행 시 값인가** [D]. 펼치기는 경로의 필드, 표, 원소 객체를 static-initializer image 에서 읽는다. const 객체나 const 필드는 바뀔 수 없다(바꾸면 C 에서 정의되지 않은 동작). const 가 아닌 고리(link)마다 FUNC_WRITES 에서 그 필드(`T.f` 또는 `T.*`)나 그 객체(이름으로 대입, 주소 잡기)에 대한 **형 있는 쓰기**를 찾는다. 하나라도 있으면 image 가 낡았을 수 있으므로 walk 를 해결하지 않는다(trip 미상, star-closed, 이유에 쓰기 위치). 없으면 그 고리를 **선언된 가정**으로 펼친다: 루프에 `static-image-assumed` 표시, 규칙 설명에 고리 목록, facts 의 BASIS 에 `assume.static_image[<루프>]` 항목(정책 `declared-assumption`, 출처 계층 `researcher`), 요약에 "declared assumptions" 로 센다. 이 확인은 포인터·cast·본문 없는 코드를 통한 쓰기는 배제하지 않는다(그래서 가정이다).


resolved table walk 의 반복마다 원소 객체의 static image 로 루프 본문 호출의 guard 를 **부분 평가**한다(원소 필드 값 대입, 상수 비교, 알려진 non-NULL 포인터 대 0, 함수끼리 이름 비교, 논리 연결자). 결과: `taken`, `skipped`, 또는 원소에 의존하지 않는 나머지 조건만 남은 `residual`. 원소 필드를 읽는 인자는 그 반복의 값(함수, 정수, 문자열, 주소)으로, 원소 필드를 통한 간접 호출은 그 반복의 callee 로 기록한다. `order` view 는 이것으로 table walk 가 정하는 순서를 보여 준다.

### 5.4 spawn 사실

spawn 연산 호출의 entry 인자는 함수 포인터 흐름 그래프로 해결한다. **task-main 매개변수**: spawn 하는 함수 F 의 매개변수 k 가 (DIRECT) 그 spawn 의 entry 인자로 흐르거나, (VIA_ENTRY) 해결된 entry 에서 도달하는 함수의 간접 호출 callee 로 흐르면 기록한다. 이것은 field 기반 **may-flow** 사실이다: 어느 task 가 어느 값을 부르는지(인스턴스 대응)는 정하지 않는다. 그 대응은 SPEC §4.2 8 의 A-OWNER 와 같은 종류의 가정이고 K4b 가 신뢰 원장에 올려야 한다.

## 5a. 프로필 없는 추출 (`--no-profile`, 2026-10-30, 단계 2a S1) [D][M]

근거: `plan/extraction_design_20261030.md` §3.0, §6 항목 1–2. 결과는 `plan/step2a_s1_20261030.md`.

**입력** [D]. 프로필을 읽지 않고 명령줄 어휘도 받지 않는다. 고정된 내장 프로필 `phi 1; profile "kz-static-builtin-minimal";`(로더가 받는 최소 두 문장, 어휘 0)을 쓴다. 나머지는 빌드(basis), P0 표, recipe 의 시작 스크립트 블록(L0, basis 의 `startup.entry[k]`), 아래 규칙에서만 온다. facts 의 BASIS 에 `extract.mode`, `extract.builtin_profile`, `extract.p0_table`, `extract.derivation_rules` 가 남고 머리의 프로필 해시는 내장 프로필의 해시다. basis 에는 `profile.mode` 가 남는다. `kz replay` 의 프로필 해시 결합은 그대로다.

**P0 표** (`lib/llvm/p0.cpp`, 개정 1; 개정 2는 §5f). API 묶음마다 한 번: 스레드 생성 `pthread_create`(인자 0 에 새 스레드 id, 인자 2 시작 함수, 인자 3 그 인자), mutex `pthread_mutex_lock/trylock/timedlock/unlock`, 세마포어 `sem_wait/trywait/timedwait/post`, 조건 변수 `pthread_cond_wait/timedwait/signal/broadcast`, 잠자기 `clock_nanosleep`·`nanosleep`·`sleep`, 종료 `pthread_exit`(스레드)·`exit`·`_exit`·`_Exit`·`abort`(프로세스), `pthread_self`, `pthread_getspecific/setspecific`, `dlopen`·`dlsym`, 데이터 함수 12개. 항목마다 판 표기(POSIX.1-2017 / glibc 2.39; `bzero` 는 POSIX.1-2017 에 없음)와 원문 대조 줄이 있다. 원문은 upstream glibc 2.39 `nptl/pthread_create.c` 만 대조했다(L624, L755 의 `*newthread = pd` 가 L804·L836 의 `create_thread` 앞). 나머지는 `unverified` 다. try·timed 항목은 `conditional` 이다. 프로그램이 같은 이름의 함수를 정의하면 그 본문을 읽고 항목은 쓰지 않는다(`shadowed`). 데이터 함수 요약은 이 표에서 예전 순서대로 읽는다.

**두 번 읽기** [D]. 첫 읽기는 함수 본문마다 Clang CFG(모든 식을 원소로 둬서 블록 안 호출이 평가 순서)와 직접 호출의 함수 포인터 인자를 더 기록한다. finalize 한 첫 읽기 위에서 유도한다. 둘째 읽기는 유도한 연산을 어휘로 쓴다(함수 key 기준, 내부 연결 함수가 섞이지 않게). 둘째 읽기의 facts 와 세 섹션이 출력이다.

**F8 DELAY-W·TERM-W** [D]. 호출 그래프(직접 호출 + 해결된 간접 대상)의 강연결요소를 callee 먼저(역위상) 풀고, 요소 안에서는 고정점까지 되풀이한다. 블록을 따라가다 돌아오지 않는 호출(callee 가 돌아오지 않음, 선언이 noreturn)에서 그 경로는 멈춘다.
- returns: 진입에서 출구에 닿는가(처음 참, 내려가기만 함).
- 종료 must: 출구에 닿지 않고, 닿는 모든 경로가 종료 호출(P0 종료 또는 must 종료 callee; 간접 호출은 해결된 대상이 모두 그럴 때)에서 끝나며, 멈추지 않는 블록끼리 순환이 없다(끝없는 경로도 종료여야 must).
- 종료 noreturn: must 가 아니고 돌아오지 않으며 어떤 경로가 종료에 닿는다. 종료 may: 돌아오는 경로도 있다.
- 지연 must: 돌아오는 경로가 있고 돌아오는 모든 경로가 지연 호출을 지난다. 지연 may: 어떤 경로가 지연에 닿는다.
- 행: (함수, 종류, 그 함수 안에서 거기로 가는 호출)마다 하나. guard 는 둘째 읽기의 그 호출 guard, `chain` 은 너비 우선으로 정한 가장 짧은 사슬(같으면 key 순).
- **어휘 규칙** [D]: 지연은 must 만(무엇이 기다린다고 말할 수 있는 것만 루프 종류를 정한다). 종료는 must·noreturn·may 모두이고, noreturn·may 의 호출 행에는 `conditional` 을 단다. `process-terminal` 은 사슬이 모두 프로세스 종료로 끝날 때만. mutex 는 P0 의 무조건 획득(`pthread_mutex_lock`)과 해제를 잠금 어휘로 쓴다. 잠금 래퍼 유도(F5)는 S1 에 없다.

**F7 SPAWN-W** [D]. P0 스레드 생성에서 출발해 고정점까지: 함수 F 의 매개변수 k 가 (a) F 안 spawn 호출의 entry 인자로 흐르면 `entry-argument`, (b) 그 spawn 의 entry 에서 닿는 코드의 간접 호출로 흐르면(필드에 저장, 필드 기반) `called-by-entry`. (b)는 F 가 고정 entry spawn 함수를 부르는 경우도 본다: F 안의 spawn 호출이 매개변수에서 오지 않은, 흐름 그래프가 푸는 entry 를 쓰면 F 는 `fixed-entry` 다. fixed-entry 는 한 단계만 쓴다(그 호출자는 그것으로 spawn 연산이 되지 않는다). fixed-entry 함수는 어휘가 아니다. **시작 스크립트 진입**: spawn 호출의 entry 인자가 함수 포인터로 바꾼 데이터(정수·데이터 포인터)이고 흐름 그래프로 풀리지 않으며, 호출한 함수가 **직접 호출만으로** P0 `dlsym` 에 닿으면, basis 의 `startup.entry[k]` 중 종류 `app` 의 둘째 필드(진입 기호)가 그 spawn 의 entry 다(닫힘 밖이면 `not-in-closure`). 이 entry 는 둘째 읽기의 CALL_TARGETS 에도 들어간다.

**숫자 상한 없음** [D]. 유도에는 깊이·크기 상한이 없다(고정점, 순환은 방문 집합으로 끊는다). 규칙 선택(지연 어휘 must 만, 종료 어휘 전부, fixed-entry 한 단계)의 영향은 요약 줄에 센다: may 지연을 부르는 루프 수, 고정 entry 함수의 호출자 중 남긴 것.

**시험** [M]: `static_golden_wrappers`(fixture `wrappers.c`, `data/wrappers.golden`), `test_static_wrappers`(합성 모델 251 확인), `test_static_cli` 의 프로필 없는 닫힘(모듈 하나, 시작 스크립트, `dlsym`).

## 5b. 확장 사실 F1–F3 (`READS`, `RETURNS`, `RET_CONSTS`, `LOOP_EXITS`, 2026-10-30, 단계 2a S2) [D][M]

근거: `plan/extraction_design_20261030.md` §3.0 F1–F3, §6 항목 1. 결과는 `plan/step2a_s2_20261030.md`.

**언제 쓰나** [D]. 프로필 없는 추출(§5a)의 둘째 읽기는 늘 쓴다. 프로필 추출은 `--extended-facts` 를 줄 때만 쓰고, 그 밖에는 이 사실을 읽지도 않는다. 그래서 옵션 없는 프로필 추출의 facts 는 예전과 바이트가 같다(STRINGS 의 build identity 문자열 말고). 옵션을 주면 기존 섹션의 **레코드**는 같지만 바이트는 다를 수 있다: STRINGS 가 정렬된 정규 표이라 새 문자열이 들어오면 다른 섹션의 `kz_str` 오프셋이 옮겨진다. 새 섹션의 식 트리는 FACT_EXPRS 에서 기존 섹션들이 쓰는 행 **뒤에** 번호가 붙어 앞 행의 색인은 그대로다. 새 사실은 FILE_HASHES 에 파일을 더하지 않는다(다른 사실이 이름 붙이지 않은 파일이면 `fh` 는 없음). BASIS 에 `extract.extended_facts` 와 `count.reads`, `count.returns`, `count.return_constants`, `count.loop_exit_rows` 가 남는다.

**F1 READS** [D].
- 읽기 = 함수 본문에서 멤버 접근, 전역 변수, 전역 배열 원소의 lvalue→rvalue 변환. 배열이 포인터로 바뀌는 것(decay)도 읽기로 센다(`array-contents`: 그 포인터로 내용을 읽는다). 단 첨자의 base 인 배열 decay 는 더 긴 경로의 한 단계라 읽기가 아니다. 지역 변수와 매개변수 자체, 포인터가 가리키는 값(`*p`)은 넣지 않는다.
- 위치는 writer 와 같은 추상 위치다: 필드는 (소유 형, 필드), 전역은 (`<global>`, 객체 이름). 접근 경로를 뿌리까지 따라가 뿌리 종류(전역, 매개변수, 지역, 호출 결과, 그 밖)와 이름을 적는다. `->`, `*`, 포인터 첨자를 지나면 `through-pointer` 다. 경로의 첨자는 뿌리부터 순서대로 `index_root` 에 둔다. 첨자가 정의 하나인 지역 변수이면 그 정의의 값으로 바꾸고 `index-unfolded` 를 단다(RETURNS 와 같은 규칙).
- 용도(`use`, 집합): 값이 문장까지 올라가며 지나는 문맥을 모두 단다. 분기 조건(if·루프·switch·`?:` 조건, `&&`·`||` 피연산자; `ctx_root` 는 가장 안쪽 조건), return 값, 포인터 매개변수를 통한 저장의 우변(`out-store`; `ctx_root` 는 목적지), 그 밖의 저장·초기화의 우변(`store`), 호출 인자(가장 안쪽 호출의 site 와 인자 번호), 첨자, 역참조되는 포인터, 간접 호출의 callee. 하나도 없으면 `other`. 예: `if (strcmp(t[i].name, k) == 0)` 의 `name` 은 `call-argument` 이면서 `branch` 다.
- guard 는 writer 의 `guard_from` 과 같은 식이다. 가장 안쪽 루프의 색인을 함께 적는다.

**F2 RETURNS, RET_CONSTS** [D].
- 값 있는 return 문마다 행을 낸다. 값은 식 트리(상수, 필드 읽기, 호출 결과, 지역 변수 …)이고, 정의가 하나인 지역 변수(derives 의 정의 표: 정의 하나, 복사, 주소를 잡지 않음, 자기를 읽지 않음, 그 정의가 소스에서 return 보다 앞)는 그 값으로 펼친다(`unfolded`). 펼친 값 안의 지역 변수도 그 정의 앞을 기준으로 다시 펼친다. 순환은 펼치는 중인 이름의 집합으로 끊는다. 숫자 상한은 없다.
- 돌려주는 값이 지역 변수 v 이고 v 의 주소를 잡지 않았으면, 함수 본문의 Clang CFG(모든 식을 원소로) 위에서 그 return 에 **닿는 v 의 정의**(reaching definitions)를 구해 정의마다 한 행(`alt`)을 낸다(`from-definition`, 정의 위치, 그 정의의 guard). 정의는 v 에 대한 대입·복합 대입·증감, 초기값 있는 선언이다. 값은 정의의 우변(복합 대입·증감은 `top`)이다. 그 정의 뒤에 이 return 에 닿는 다른 정의가 올 수 있으면 `redefined-later` 다. 진입에서 정의 없이 닿는 경로가 있으면 `undefined-on-a-path` 다.
- `cond_root` = return 의 guard ∧ 정의의 guard ∧ ¬(뒤에 올 수 있는 정의들의 guard)이고, 정의 하나인 지역 변수를 펼친다. "이 조건이면 함수가 이 값을 돌려준다"로 읽는다. guard 는 writer 의 구조적 조건이라 루프 안(`in-loop`: return, 정의, 또는 뒤의 정의가 루프 안)에서는 반복마다의 조건이다. `case` 라벨은 합류점이라 switch 의 case 조건은 guard 에 없다(§4.1 과 같다).
- RET_CONSTS: 함수와 상수마다 그 값을 돌려주는 RETURNS 행 조건의 논리합(조건 없는 행이 하나라도 있으면 "always"). 모든 RETURNS 행이 상수인 함수에는 `only-constants` 를 단다. 상수 값은 FACT_EXPRS 의 식과 같은 방법으로 평가한다(바깥 cast 를 벗긴 피연산자). 그래서 부호 있는 형으로 바꾼 큰 상수(`((T)0xc2000007)` 꼴)는 부호 없는 수로 적히고, 호출하는 쪽 조건의 같은 상수와 같게 적힌다.

**F3 LOOP_EXITS** [D]. 모든 루프에 탈출식(탈출들의 논리합)과 본문의 mod set(쓰는 필드·전역·지역, 부르는 함수)을 둔다. FACT_LOOPS 는 지연 호출이 있는 루프와 table walk 에만 이 트리를 둔다(`ast_fn.cpp` 의 게이트, 그대로 둔다). 같은 트리이면 LOOP 행에 `as-in-loops` 를 단다. 탈출마다 EXIT 행이 있다: 종류(루프 조건, break, return, goto), 위치, 그 탈출이 실행될 조건(루프부터 그 지점까지의 guard), 그 탈출을 정하는 조건(루프 안의 가장 안쪽 guard 또는 부정된 루프 조건; 없으면 `unconditional`). 상수 거짓 탈출은 `never-taken`, 하나도 취할 수 없으면 LOOP 행이 `no-exit` 다. 루프 종류와 delay-in-body 는 FACT_LOOPS 에 남는다(프로필 없는 추출에서는 F8 이 정한 지연 연산으로).

**도구 매개변수** [D]. 새 규칙에는 숫자 상한이 없다. 식 트리 생성기(`to_expr`)의 기존 깊이 상한 24(레퍼런스가 정하지 않은 값, `unspecified_by_reference`)가 새 사실의 식에도 걸린다. 상한은 트리를 `top("too deep")` 으로 자를 뿐 행을 없애지 않는다. 그래서 행 수는 이 값과 무관하다. 기준 빌드 다섯에서 잘린 노드는 0이었다(`plan/step2a_s2_20261030.md` §3.5). 텍스트 view 는 트리를 40 단계까지만 보여 준다(표시만, 사실은 그대로).

**시험** [M]: `static_golden_flowfacts`(고정 도구 프로필 + `--extended-facts`), `static_golden_flowfacts_np`(`--no-profile`; 둘의 확장 섹션은 같다), fixture `flowfacts.c`, `test_static_cli` 의 확장 사실 확인(옵션 없으면 네 섹션이 없음, 있으면 기존 섹션 view 동일·FACT_EXPRS 앞 행 동일).

## 5c. 확장 사실 F4·F6 (`ARGFLOW`, `PARAM_FLOW`, `PTR_FIELDS`, `GLOBAL_IMAGES`, 2026-10-30, 단계 2a S3) [D][M]

근거: `plan/extraction_design_20261030.md` §3.0 F4·F6, §5 R1, §6 항목 1. 결과는 `plan/step2a_s3_20261030.md`.

**언제 쓰나** [D]. F1–F3(§5b)과 같은 조건이다: 프로필 없는 추출은 늘, 프로필 추출은 `--extended-facts` 를 줄 때만 쓴다. 옵션 없는 프로필 추출은 이 사실의 코드 경로를 타지 않는다. 식 트리는 FACT_EXPRS 에서 F1–F3 의 행 **뒤에** 번호가 붙는다. FILE_HASHES 에 파일을 더하지 않는다. BASIS 에 `extract.extended_facts_f4_f6`, 도구 매개변수 세 개(`param.argflow_depth_bound`, `param.argflow_size_bound`, `param.image_budget`, 모두 `unspecified_by_reference`)와 `count.argflow`, `count.param_flow`, `count.param_flow_nodes`, `count.param_flow_cut`, `count.ptr_fields`, `count.global_images` 가 남는다.

**F4 ARGFLOW** [D]. 모든 호출의 모든 인자(역할 없는 인자 포함; DERIVES 는 그대로 역할 인자만)마다 한 행이다.
- 복사 규칙은 derives 와 같다(정의 하나·복사·주소를 잡지 않는 지역 변수를 따라감). 다만 포인터 형 사이의 변환은 주소를 지키므로 복사로 보고 `pointer-conversion` 을 단다. 정수↔포인터 변환, 좁히는 정수 변환은 복사가 아니다. 숫자 상한은 없다(같은 지역 변수를 두 번 따라가지 않는다).
- `src` 는 사슬이 끝나는 곳이다: 상수, 문자열, 함수, 필드·전역·전역 배열 원소·역참조 읽기, 주소(`&x`), 매개변수(`param`), 호출 결과(`root` 에 callee), 지역 변수(더 따라가지 못한 이유를 `reason` 에, derives 의 이유와 같다), 그 밖의 식.
- 위치 읽기와 주소에는 접근 경로의 **base 저장 종류**(`base`)를 단다: 전역 객체, 전역 배열 원소, 지역, 매개변수의 pointee(`param`), 지역 포인터의 pointee, 포인터 필드의 pointee(`root` 는 그 필드, `object` 는 그 경로의 뿌리 전역), 전역 포인터의 pointee, 호출 결과의 pointee. 경로에서 뿌리에 가장 가까운 포인터 역참조가 base 를 정한다(그 뒤에 역참조가 더 있으면 `multi-level`). 정의 하나인 지역 포인터는 그 정의를 따라가 가리키는 곳을 base 로 삼는다(`base-via-local`; 예: `rec = &T[i]; rec->f` 는 전역 배열 원소). `object` 는 base 객체의 전역 키(STATIC_IMAGE·GLOBAL_IMAGES 의 객체 이름)다. 경로의 첨자는 뿌리부터 `index_root` 에, 원천 읽기의 READS 행은 `read` 에 둔다.

**F4 PARAM_FLOW** [D]. 함수 매개변수(`fn`, `param`)마다 그 값(위의 복사 규칙으로 따라간 복사)이 가는 곳이다.
- 깊이 0: 호출 인자(ARGFLOW 의 매개변수 원천 행; 간접 호출은 해결된 대상마다 한 행에 `indirect`·`may`, 대상을 다 모르면 `unresolved`, 본문 없는 callee 는 `callee-without-body`, 가변 위치는 `variadic`; 그 호출의 결과를 검사하면 `result-tested`), 비교 피연산자와 참·거짓으로 시험되는 조건(`truth-test`, switch 제어식은 `switch`), return 값, 필드·전역·포인터 매개변수를 통한 저장(지역 변수로의 대입은 복사라서 사슬로 따라간다), 첨자, 그리고 그 매개변수를 통한 저장(`out-store`, 기존 out-pointer 요약의 직접 저장).
- 깊이 d > 0: 본문 있는 함수로의 호출 인자 행을 간선으로 삼아, 호출 그래프의 강연결요소를 callee 먼저(역위상 순서) 처리하며 고정점까지 합성한다. 행은 d 번의 호출을 지나 닿는 깊이 0 사용(그 종류·위치·식·guard, `sink_fn`)이고, 같은 사용에는 가장 짧은 깊이(같으면 must 먼저)만 남긴다. 첫 호출(`site`, `next_fn`, `next_param`)과 경로(`chain`)를 적고, 간접 호출을 지나면 `may` 다. 깊이 d > 0 의 호출 인자 행은 경로의 열린 끝(본문 없음·미해결·가변 위치)이다.
- 도구 매개변수 두 개: 합성 행이 지나는 호출 수의 상한(`--argflow-depth N`)과 매개변수당 합성 행 수의 상한(`--argflow-size N`). 레퍼런스가 정하지 않는 값이라 기본은 **상한 없음**이다. 상한에 걸려 빠진 사용은 그 매개변수의 OPEN 행(`depth-bound`/`size-bound`, `n` 은 빠진 사용 수)으로 남기고 미해결로 센다.

**F4 PTR_FIELDS (포인터 필드 흐름)** [D].
- ADDRESS_WRITE: 포인터 필드에 전역 객체의 주소를 쓰는 저장(`p->f = &G`, `G.f = &G2[c]`, decay 되는 전역 배열 `p->f = arr` 은 원소 0). 쓰는 멤버 경로의 뿌리가 전역이면 그 객체(`root`), 아니면 필드 기반(`field-based`). STATIC_INIT: 전역의 정적 초기값이 그 필드에 주소를 넣는 경우(이미지의 주소 잎).
- 쓰기마다 그 함수의 Clang CFG 지배자 관계로 순서를 정한다: 쓰기가 지배하는 호출과 READS 행, 그리고 쓰기를 지배하는(루프 밖) 같은 필드·같은 형 레코드의 앞선 쓰기(`earlier-writes-overwritten`). 그 밖의 형 있는 쓰기(필드 저장, 레코드 통째 쓰기, 필드 주소의 escape; 함수·위치·종류마다 하나)가 남으면 `n` 과 `other-writes` 다.
- READ: 접근 경로가 포인터 필드를 지나는 READS 행(뿌리에 가장 가까운 단계). 같은 형·필드의 주소 쓰기 가운데 같은 뿌리이거나 필드 기반인 것이 후보다. RESOLVED = 후보 하나, 같은 뿌리, 쓰기가 읽기보다 프로그램 순서로 앞(`write-before-read`: 같은 함수에서는 쓰기가 읽기를 지배; 다른 함수이면 읽는 함수로 가는 **모든** 호출 경로가 쓰는 함수의, 쓰기가 지배하는 호출을 지난다; 거꾸로 따라가다 호출자 없는 함수, spawn 진입, 미해결 간접 호출이 있을 때 주소를 잡힌 함수에 닿으면 앞이 아니다; 정적 초기값은 늘 앞), 남은 다른 쓰기 0. 후보가 있지만 하나라도 어긋나면 MAY, 후보가 없으면 NEEDS field points-to 다. 첨자가 둘러싼 counted loop(정확한 횟수 또는 상한)의 유도 변수이면 원소 값 `elem` + k·`idx_step`(k < `idx_n`)을 적는다(`index-from-counted-loop`; 상한이면 `loop-may-exit-early`).
- 형 있는 쓰기만 본다. 형 없는 포인터로의 저장과 본문 없는 코드의 쓰기는 배제하지 않는다(table walk 의 이미지 확인과 같은 한계).

**F6 GLOBAL_IMAGES** [D]. ARGFLOW 의 `object` 와 PTR_FIELDS 의 대상·뿌리가 이름 붙인 전역마다, STATIC_IMAGE 와 같은 이미지 생성기(`flatten_initializer`, 같은 잎·종류·경로)로 만든 정적 초기값이다. table walk 와 무관하다. 스칼라 전역은 잎 하나, 초기값 없이 정의된 객체는 `zero-initialized` 한 행이다. 닫힌 프로그램 밖에 정의된 객체는 행이 없고 수만 센다. 잎이 매크로로 쓰였으면 거기 쓴 가장 바깥 매크로(`macro`)와 값의 첫 토큰이 쓰인 곳(spell 위치, 정의)을 적는다(시스템 헤더의 매크로는 적지 않는다). 잎이 레코드 필드이면 (`owner`, `field`)를 적고, 그 필드를 쓰거나 주소를 잡는 함수가 있으면 `field-written`(필드 기반), 그 형 레코드를 통째로 쓰면 `record-written`, 객체 자체를 쓰거나 주소를 잡으면 `object-written` 이다. 이 표시가 없으면 형 있는 쓰기로는 정적 값이 실행 중 값이다. 이미지 잎 수 상한(기존 4096, `--image-budget N`)은 레퍼런스가 정하지 않는 도구 매개변수다. 예산이 떨어진 곳의 표시 잎과 두 계수는 §5j 다(2026-11-10).

**시험** [M]: `static_golden_argflow`(고정 도구 프로필 + `--extended-facts`), `static_golden_argflow_np`(`--no-profile`; 둘의 새 섹션은 같다), fixture `argflow.c`, `test_static_cli` 의 F4·F6 확인(옵션 없으면 네 섹션이 없음, 있으면 기존 섹션 view·FACT_EXPRS 앞 행 동일, 상한과 이미지 예산의 기록·거부·효과).

## 5d. F5 잠금 집합과 대상에 무관한 규칙 (`LOCK_OPS`, `LOCK_IDS`, `LOCKSET`, `FIELD_STATUS`, `AWAIT_L1`, 2026-10-30, 단계 2a S4) [D][M]

근거: `plan/extraction_design_20261030.md` §3.0 F5, §3.3, §5 R3, §6 T3. 결과는 `plan/step2a_s4_20261030.md`.

**언제 쓰나** [D]. F1–F4·F6(§5b, §5c)과 같은 조건이다: 프로필 없는 추출은 늘, 프로필 추출은 `--extended-facts` 를 줄 때만 쓴다. 옵션 없는 프로필 추출은 이 코드 경로를 타지 않는다. 식 트리는 FACT_EXPRS 에서 개정 7의 행 **뒤에** 번호가 붙는다. FILE_HASHES 에 파일을 더하지 않는다. extract 요약 출력과 P0 표는 바꾸지 않았다(기존 golden 이 둘을 담는다). 이 단계가 바꾸는 기존 규칙은 없다: FACT_LOCS(§4.1의 옛 노출 규칙)와 FACT_LOOPS(§5.2의 옛 L1 (i))는 그대로이고, 새 규칙은 새 섹션에만 쓴다.

**F5 LOCK_OPS: 잠금 연산의 유도** [D].
- 출발점은 P0 의 mutex 기본 연산이다(`pthread_mutex_lock` 획득, `pthread_mutex_trylock`·`timedlock` 시도 획득, `pthread_mutex_unlock` 해제, `pthread_cond_wait`·`timedwait` 대기: 인자 1의 mutex 를 풀었다가 돌아올 때 다시 잡음). mutex 인자의 위치는 P0 의미 문장이 이미 적은 것이다.
- 함수마다 본문의 잠금 골격(Clang CFG 의 블록마다 호출·읽기·쓰기·지역 변수 정의·return 을 평가 순서로, 두 갈래 분기의 시험 `x ==/!= 상수`, `x`, `!x`; x 는 지역 변수나 그 블록의 호출 결과)을 걷는 전방 분석이다. 상태는 잡힌 잠금의 집합이고, **잡힌 잠금 집합(과 호출자의 잠금을 푼 집합)마다 상태를 따로 둔다**. 같은 집합끼리는 지역 변수 사실(호출 결과를 담은 변수의 값)을 교차로 합친다. 그래서 "0을 돌려줄 때만 잡고 돌아오는" 연산이 그렇게 보인다.
- 호출 그래프의 강연결요소를 callee 먼저 처리한다. 자기 요소 안의 호출과 미해결 간접 호출은 모든 잠금을 풀 수 있는 호출로 읽는다(`unknown-release`). 본문 없는 함수 가운데 P0 가 아닌 것은 잠금에 효과가 없다고 본다.
- 함수 요약: 진입 때 아무 잠금도 없이 분석해, 돌아오는 경로에서 잡힌 잠금 X 를 다음으로 나눈다. 모든 return 에서 잡힘 = EVERY_RETURN. 잡힌 return 이 모두 상수 c 를 돌려주고 나머지 return 은 c 와 다르다고 알려진 값을 돌려줌 = RESULT_CONDITIONAL(`success` c). try 기본 연산에서만 온 것 = TRY. 어느 쪽도 아니면 CONDITIONAL(연산 아님: 호출자는 잡힌 것으로 세지 않는다). 해제도 같은 방식으로 나눈다(호출자가 잡은 잠금을 모든 경로/결과 조건부로 풂). 어떤 경로에서만 푸는 것은 연산이 아니고 호출자의 그 잠금을 지우기만 한다.
- **선언된 가정** (`assume.lock_acquisition_succeeds`): try 가 아닌 잠금 연산은 성공한다. P0 기본 연산은 0을 돌려준다(POSIX.1-2017 RETURN VALUE; 원문 대조 unverified). 유도된 연산은 잡았을 때 돌려주는 값을 돌려준다. 결과를 시험하는 분기 가운데 실패만 가는 쪽은 **실패 경로**다: 요약에서 빼고, 그 안의 LOCKSET 행은 `failure-path` 로 그 경로에서 잡힌 잠금(실패한 획득은 빠짐)과 함께 낸다. 예: 잠금 래퍼의 오류 분기.
- **해제의 범위**: 함수가 스스로 잡은 잠금을 같은 정체성으로 풀면 그 함수 안의 짝이다. 호출자의 잠금에는 그 정체성이 정적 인스턴스이거나 고정 주소일 때만 효과가 없다(같은 mutex 의 재귀 획득이거나 다른 mutex 이다). 그렇지 않으면 그 해제는 다른 mutex 일 수 있으므로, 필드 기반 위치를 공유할 수 있는(`may` 가 같거나 어느 쪽이든 모름) 모든 잡힌 잠금을 지운다. `may` 는 이 "지우지 않음"에만 쓰고, 두 연산이 같은 잠금임을 보이는 데는 쓰지 않는다.
- 조건 대기는 그 전에 잡혀 있던 잠금만 대기 호출부터 다시 잡힌 것으로 한다(그 사이에 풀렸다: 구간이 끊긴다). 잡혀 있다고 알려지지 않은 잠금은 대기 뒤에도 잡힌 것이 아니다.
- `vocabulary`: EVERY_RETURN·RESULT_CONDITIONAL 의 획득, 그리고 같은 모드의 해제 가운데 어휘의 획득이 잡는 필드 기반 위치의 mutex 를 푸는 것(기본 연산은 늘). 프로필 없는 추출의 둘째 읽기는 첫째 읽기(이제 확장 사실도 읽는다)에서 이것을 유도해 잠금·해제 연산 어휘로 쓴다. 그래서 그 추출의 FACTS `lock-op`/`unlock-op` 표시와 FACT_LOOPS 의 L1 (ii) 판정이 이 연산을 따른다. BASIS `extract.lock_vocabulary` 가 둘째 읽기의 유도가 같은 집합을 냈는지 적는다.

**F5 LOCK_IDS: 잠금 정체성** [D].
- 잠금 정체성은 그 연산이 보는 함수의 매개변수 위의 식이다: `$k`(매개변수 k), 전역 객체, `x.f`, `x[i]`, `p->f`, `&x`, 위치에서 읽은 값, `?(이유)`(따라가지 못한 부분; 나머지 구조는 남긴다). 호출 인자의 정체성은 derives 의 복사 규칙으로 지역 변수를 따라가 만든다. 호출 지점에서 callee 요약의 정체성에 인자를 대입한다(`p[i]` 와 `p = &x[0]` 이면 `x[i]`).
- **must 정체성만** [D]: 두 행이 같은 정체성이고 그 상태가 STATIC INSTANCE 일 때만 같은 mutex 를 잡았다고 본다. STATIC INSTANCE = 매개변수도 모르는 부분도 없고, 읽는 모든 값이 전역 객체의 필드이며, 그 필드의 writer 집합이 FIELD_STATUS 의 필드 단위 규칙으로 complete 이고 writer 인스턴스가 없거나 하나이고 그 하나가 한 번만 실행된다(`once`). 그 밖은 NOT_STATIC(이유를 `why` 에), PARAMETER_RELATIVE(호출자가 정한다), UNDECIDABLE 이다. 필드 기반 위치(`may`)가 같아도 같은 잠금이라는 근거가 되지 않는다.
- **선언된 가정** (`assume.static_instance_read_after_write`): 정적 인스턴스를 읽는 두 잠금 연산은 그 한 writer 인스턴스 뒤에(또는 둘 다 앞에) 읽는다.
- `type-level-exposed`: 읽는 값의 위치가 형 단위 규칙으로는 complete 가 아니다(형 단위 규칙을 고르는 사용자가 볼 수 있게).

**F5 LOCKSET** [D]. 본문 있는 모든 함수의 모든 호출(FACTS), 읽기(READS), 형 있는 쓰기(FUNC_WRITES 사건의 모든 지점)마다 한 행이다. 그 문장에서 잡혀 있는 잠금을 함수 진입 기준으로 적는다(실패 경로가 아닌 경로들의 교차, must). 호출자가 잡은 잠금은 그 함수를 부르는 호출 행에 있다. `acquired-at` 은 그 잠금을 잡은 호출(어떤 경로에서든)이다. 한 획득 호출 하나에서 열린 구간이면 같은 임계 구역이다. CFG 가 없는 본문은 `no-cfg`, 닿지 않는 문장은 `not-reached` 다.

**FIELD_STATUS: writer 집합 완전성의 대상 무관 규칙 (ED §5 R3 (a))** [D].
- 행: READS 행이 이름 붙인 레코드 형의 모든 필드(그 형의 모든 필드: 형 단위 규칙에 필요하다).
- **완전성 pass**: 내장 프로필과 데이터 함수 표만 쓰고, 표적이 정확히 그 필드들인 별도 추출을 한 번 더 돈다(`ExtractSpec::keep_raw_atypes`). 그 결과는 이 섹션에만 쓴다. 그래서 행은 모드와 실행의 대상에 무관하다.
- A-TYPE 적중을 원인으로 나눈다: 그 필드에 대한 것(필드 주소의 escape, 그 이름의 union 멤버로의 저장)과 형 전체에 대한 것(cast, 형을 담은 union, unknown-effect).
- `status_field`(필드 단위): 그 필드나 형 전체에 대한 적중이 없으면 complete 다. 형 없는 저장은 그런 노출을 거쳐야만 그 필드에 닿는다. 다른 멤버의 주소에서 넘어가는 포인터 연산은 C 에서 정의되지 않은 동작이다(C11 6.5.6 p8: 멤버는 원소 하나짜리 배열). 이것은 선언된 가정 `assume.no_cross_member_pointer_arithmetic` 이다.
- `status_type`(형 단위 보수): 위에 더해, 같은 형의 다른 필드에 대한 적중이 있고 형 없는 쓰기 지점이 프로그램에 있으면 DISABLED 다.
- 옛 규칙(FACT_LOCS: 노출된 형의 모든 **대상**에 untyped write 를 셈)은 어떤 다른 필드가 대상인지에 따라 바뀐다. 두 새 규칙은 그렇지 않다.
- `n_instances`: writer 인스턴스(직접·데이터 함수 쓰기 지점 하나, out-pointer 호출 하나). `once`: 인스턴스가 없거나(`no-writer-instance`), 하나이고 루프 밖에 있으며 그 함수가 한 번만 실행된다(`one-instance-once`). 한 번만 실행됨 = 주소를 잡히지 않고, spawn 진입이 아니고, 재귀가 아니고, 호출 지점이 없거나(뿌리) 루프 밖의 호출 지점 하나가 그런 함수 안에 있다.
- 한계 [M]: writer 수집은 기존 기계와 같다. fixture `lockset.c` 의 `fx_g.tasks[slot] = slot`(스칼라 배열 필드의 원소에 대한 저장)은 WRITERS 에도 FIELD_STATUS 에도 writer 로 잡히지 않는다(FUNC_WRITES 에도 없다). 기존 섹션을 바꾸지 않으려고 고치지 않았다. §8 에 남은 일로 적는다. 셋째 수정(§5h)부터 FIELD_STATUS 는 그런 필드(const 가 아닌 스칼라 배열)를 complete 로 부르지 않는다(두 규칙 DISABLED, flag `array-element-stores-not-collected`).

**AWAIT_L1: 대상에 무관한 L1 (i) (ED §5 R3 (b))** [D].
- poll 루프마다 한 행이다. 관련 위치 = 술어가 직접 읽는 필드·전역(크기를 아는 배열 cursor 의 base 는 제외, FACT_LOOPS 와 같다) ∪ 술어가 부르는 함수와 그 함수가 부르는 함수들이 읽는 필드·전역(READS; `via` 는 이것만이 주는 것). **추출 대상은 넣지 않는다.**
- L1 (i)(규칙마다): 관련 필드의 writer(완전성 pass 의 writer 행)가 루프 본문 안에 있거나, 본문이 부르는 함수(직접 호출과 해결된 간접 호출로 닿는 모든 함수)에 있으면 FAILS 다. 아니면, 관련 필드의 writer 집합이 그 규칙으로 complete 가 아니거나, 관련 위치가 전역 변수이거나(이 개정에 변수의 writer 집합 규칙이 없다), 본문이 미해결 간접 호출에 닿으면 UNDECIDED 다. 둘 다 아니면 HOLDS 다.
- L1 (ii): 술어의 본문 안 읽기와 술어가 부르는 함수의 본문 안 호출이 모두 하나의 획득 호출이 연 잠금 하나를 잡고 있으면(LOCKSET) ONE_LOCK_REGION 이다(`lock` 그 정체성, 정적 인스턴스이면 `lock-static`). 아니면 FACT_LOOPS 의 atomic-read 를 쓰고, 그것도 아니면 FAILS 다.
- 판정(규칙마다): 술어가 인식되고(FACT_LOOPS) L1 (ii)·L1 (i)이 성립하면 AWAIT, 아니면 STAR_CLOSED(이유). FACT_LOOPS 의 인증 뷰 규칙은 그대로다.

**시험** [M]: `static_golden_lockset`(고정 도구 프로필 + `--extended-facts` + `--delay-op fx_delay`), `static_golden_lockset_np`(`--no-profile`; 둘의 새 섹션은 같다), fixture `lockset.c`, `test_static_cli` 의 F5 확인(옵션 없으면 다섯 섹션이 없음, 있으면 기존 섹션 view 동일, 두 모드 같은 행; 결과 조건부·모든 return·try 연산, 한 번 쓰인 handle(정적 인스턴스)과 두 번 불리는 함수가 쓰는 handle, spawn 앞뒤의 잠금 집합, 실패 경로, 형제 필드 노출, callee 를 거친 술어 읽기의 await, 대상을 바꿔도 FIELD_STATUS·LOCK_IDS·AWAIT_L1 이 같고 옛 FACT_LOCS 는 바뀜).

## 5e. 규칙 대상(`--targets-rule`)과 사례 무관 질의 `kz-static relations` (2026-10-30, 단계 2a S5) [D][M]

근거: `plan/extraction_design_20261030.md` §3.1–§3.4, §6 단계 2a 항목 2·3. 결과는 `plan/step2a_results_20261030.md`.

**`extract --no-profile --targets-rule R-T1,R-T2`** [D]. 둘째 읽기의 추출 대상을 첫째 읽기의 확장 사실에서 규칙으로 고른다(`lib/llvm/target_rules.cpp`, "target rules revision 1"). 두 규칙 모두 **공유 base** 의 레코드 필드만 고른다.
- 공유 base: 전역 객체, 전역 배열 원소, 전역 포인터의 대상, 전역에서 시작하는 접근 경로의 포인터 필드 대상(ARGFLOW `object`), 그리고 모든 반환값이 전역 객체(의 일부)의 주소이거나 null 인 함수의 호출 결과(RETURNS; 그런 호출 결과를 돌려주는 함수도 고정점까지 포함).
- R-T1: 호출 인자가 읽거나 주소를 넘기는 필드(ARGFLOW 의 원천 `field`·`address`).
- R-T2: 함수가 공유 base 로 읽어 반환하거나 포인터 매개변수를 통해 저장하는 필드, 또는 분기 조건에서 읽고 그 함수가 상수를 돌려주는 조건(RETURNS)에도 나오는 필드.
- 매개변수·지역 변수·다른 호출 결과로만 닿는 필드는 고르지 않는다. 그 수를 extract 요약의 `rule targets` 줄과 BASIS `count.targets_rule_t1_unshared_only`·`_t2_unshared_only` 로 낸다.
- 규칙 대상은 사람이 넣은 입력이 아니다: BASIS `target.T.f = rule R-T1` 등은 정책 `unset`, 출처 행 없음으로 적는다(명령줄 `--target` 은 지금처럼 `declared-assumption`, 출처 `researcher`). BASIS `extract.targets_rule` 과 `count.targets_rule*` 를 더한다.
- 옵션은 `--no-profile` 과만 쓴다. 옵션이 없으면 프로필 없는 추출은 앞과 같이 대상이 없다. 새 섹션은 없다(S5 때 facts schema 개정 10; LOCK_PATHS 를 더한 개정 11은 §5f).

**`kz-static relations <facts> [-o OUT]`** [D]. facts 컨테이너 하나를 읽어 텍스트를 낸다(`lib/llvm/relations.cpp`, "relations rules revision 1"; 개정 2에서 바뀐 규칙은 §5f). 필드·callee·사례 이름을 받지 않는다. 확장 사실(READS … AWAIT_L1)이 없는 facts 는 이유와 함께 거부한다. 같은 컨테이너는 같은 바이트를 낸다. 행의 기록, 사용 지점 키(`--keys`)와 공유 적재기는 §5l. 잘린 정적 이미지와 행마다의 `incomplete:` 줄(개정 6)은 §5m. 그 행에서 관측 계획을 내는 `kz-static plan` 은 §5n. 이름 공간의 소속, 표 선택 상수, writer 의 전제, out 포인터, 생성자 callee 의 상수(개정 7)는 §5o. 그 행의 순서 간선과 정적 순서(`kz-static order`)는 §5p.
- **머리**: facts 해시, 모드, 내장 프로필·P0 표·유도 규칙·대상 규칙의 판, P0 비교 표의 판과 항목(판 표기, 원문 대조), 입력 계층, 행위자 root 와 트램펄린, 경로별 행 수와 등급별 수.
- **입력 계층**: 행마다 B(빌드), P0(P0 표: spawn·잠금·지연·종료의 유도와 비교 함수), L0(행이 시작 스크립트 진입 root 에 기댈 때), R(규칙)을 붙인다. 프로필 추출의 facts 이면 `case:profile(<이름>)`, 명령줄 어휘가 있으면 `hand-entered:command-line(<n> items; G0 or other names, not automatic)` 를 모든 행에 붙인다.
- **P0 비교 표**(`p0_compare_table`, "P0 comparison table revision 1"): 바이트 문자열 비교 묶음(`memcmp`, `strcmp`, `strcoll`, `strncmp`, `strcasecmp`, `strncasecmp`)과 스레드 id 비교 묶음(`pthread_equal`). 추출은 이 표를 쓰지 않으므로 P0 표와 facts 의 P0_TABLE 은 그대로다. 프로그램이 같은 이름의 본문을 정의하면(inline 이 아닌 본문) 그 호출은 비교로 읽지 않는다.
- **정적 행위자**: root = `main`(process), 해결된 spawn entry(CALL_TARGETS spawn-entry, table walk 반복의 task-main, 시작 스크립트 진입). root 의 코드 = 다른 root 에 들어가지 않고 호출 그래프(직접 호출과 해결된 간접 대상)로 닿는 함수. process 가 아닌 root 가 다른 root 를 부르면 트램펄린이다: 행위자가 아니고, 그 코드는 그것이 부르는 root 들(트램펄린을 건너)의 것이다. 필드 기반 may 사실이다.
- **경로 B**: 원천이 공유 base 필드인 인자(ARGFLOW `field`)를 (필드, 본문 있는 callee, 인자 위치)로 묶는다. 필드의 writer 가 없거나 읽는 쪽과 쓰는 쪽이 한 행위자뿐이면 뺀다(수를 머리에 낸다). 행: 사이트마다 결과 사용(버림·사용·검사), 필드의 INIT 값과 writer(대상이면 WRITERS 의 값·guard, 아니면 LOCKSET 쓰기 위치), 행위자, callee 검사(그 매개변수가 조건에 나오는 callee 의 상수 반환과, 조건 안에서 그 매개변수로 부르는 함수의 RET_CONSTS 한 단계).
- **경로 C**: 해결된 entry 가 있는 spawn 호출마다, 생성자 함수가 그 호출 **뒤(소스 위치)** 에 쓰는 필드(LOCKSET 쓰기 행)와, spawn 된 entry 의 행위자가 닿는 코드에서 같은 필드를 분기·반환·out 저장으로 읽는 곳(READS)의 짝이다. 생성자가 받는 정체성(spawn 연산의 매개변수 가운데 out-store 가 있는 것, P0 의 out 인자), 쓰기의 첨자(WRITERS key), 읽기의 첨자가 읽는 쪽의 자기 정체성인지(첨자의 호출 결과가 P0 thread-self·thread-specific-get 에 닿는 사슬)를 적는다. 읽기가 정적 인스턴스 잠금을 잡지 않으면 그 함수의 호출 지점마다 따로 행을 낸다(spawn 된 entry 가 닿지 않는 호출이면 그렇게 적는다).
- **경로 A**: 키 비교 = (a) P0 비교 표 함수의 호출에서 비교되는 두 인자 가운데 하나가 레코드 필드이고 다른 하나가 매개변수(또는 그 대상), (b) 분기 조건의 `==`·`!=` 가 필드와 매개변수(루프 안이면 지역 변수·호출 결과도)를 비교함. 검색 = 그 비교의 루프(A1) 또는 그 비교를 가진 함수를 부르는 간접 호출이 있는 다른 함수의 루프(A2). 검색이 레코드 형을 전역 객체에서 읽지 않으면(전역 표 없음) 뺀다. 이름 공간 = (레코드 형, 키 필드). 활성 필드 = 검색 루프의 탈출 조건이 읽는 같은 형의 다른 필드. 활성 시험 값 = 키 비교가 있는 검색 루프에서 활성 필드를 상수와 `==`(사용 중)·`!=`(빈 칸)로 비교한 값. 키 매개변수 함수 = PARAM_FLOW 로 그 비교에 닿는 (함수, 매개변수). 사용 = 키 매개변수 함수를 바깥에서 부르는 호출. 키 필드 쓰기에 직접 호출로 닿는 함수의 사용은 생성자, 나머지는 조회다. 구획 = 사슬을 따라 처음 상수 인자가 있는 호출의 상수들. 키 값 = 문자열·상수 인자, 또는 포인터 필드 흐름(PTR_FIELDS)과 정적 이미지(GLOBAL_IMAGES)로 닿는 값(매크로와 철자 위치 포함). 조회 행마다 같은 구획의 생성자를 짝으로 내고, 키 상수가 다르면, 한 행위자뿐이면, probe-create(같은 함수에서 조회 결과 아래 같은 키 인자로 생성)이면 짝에서 빼고 그 이유를 적는다. 처리: 조회를 감싼 루프(종류, 횟수, 탈출식, 지연 호출과 F8 사슬), 결과를 돌려주는 함수를 거슬러 오른 사슬, 그 길의 종료 연산(결과 아래이거나, 결과 또는 결과를 쓴 함수의 지역 변수를 시험하는 guard 아래에서 쓴 필드를 인자로 받는 것; 지역 변수 경유는 may).
- **수명**(경로 A 의 이름 공간 머리): 키·활성 필드의 writer 와 값·guard, 등급(빈 칸 값을 쓰면 release, process 행위자만 닿는 루프·early-init 쓰기이면 initialization, 사용 중 값이면 reservation or activation), P0 out 인자로 쓰는 호출(예: thread-create 의 id), 생성자 매개변수가 thread-creation 기본 연산의 entry 인자로 그대로 가면 "그 entry 가 돌아온 뒤 프로그램의 함수가 돌지 않아 그 길에는 표 쓰기가 없다", 다른 검색이 시험하는 활성 필드를 이 검색이 시험하지 않으면 "no activity check".
- **칸** (ED §3.4): `dependency`(정적 근거), `run-order`(이 판은 실행 기록을 읽지 않는다: 늘 `no record given`), `feasibility`(정적, `proven-impossible` / `possible` / `undecidable` 과 이유), `violation`(RV2 §5.4; 기록이 없으면 `unconfirmed`), `flip`(`static-possible` / `static-undetermined` / `none` 과 이유), `layers`. 한 칸이 다른 칸을 정하지 않는다.
- **실현 가능성** [D]: C 는 F5 must 정체성으로만 "불가"를 낸다: spawn 과 그 뒤 모든 쓰기가 같은 획득 호출이 연 정적 인스턴스 잠금 L 을 잡고 읽기(또는 그 호출 지점)도 L 을 잡으면 `proven-impossible`. L 이 spawn 에서 잡혀 있지 않고 쓰기만 잡으면 `possible`(잠금 창). 읽기가 잠금을 잡지 않으면 `possible`. 읽기가 잠금 실패 경로에 있으면 `undecidable`. 정적 인스턴스가 아닌 잠금으로는 "불가"를 내지 않는다. B 는 두 행위자가 한 table walk 에서 만들어지면 생성 순서와, 같은 반복에서 생성 뒤 부르는 함수가 닿는 await 루프(AWAIT_L1 필드 단위 규칙)를 적고 `undecidable`(fixed unconfirmed)로 낸다: 사용이 그 행위자의 대기 상태 앞인지 뒤인지는 정적으로 정하지 않는다. A 는 늘 `undecidable` 이다(생성 순서가 초기화 순서를 정하지 않는다).
- **뒤집힘 신호** [D]: B 는 callee 검사 조건을 인자 = INIT 값과 인자 = 각 상수 writer 값으로 평가해 결과가 다르면 `static-possible`, 평가할 수 없는 부분(호출 결과, 상수가 아닌 값)이 있으면 `static-undetermined`. C 는 읽는 함수의 상수 반환 가운데 그 필드에 기대는 조건을 필드 = INIT 값과 필드 = 생성자의 상수 쓰기 값으로 평가한다. A 는 같은 구획·같은 키의 생성자가 다른 행위자에 있으면 `static-possible`, 키가 풀리지 않으면 `static-undetermined`. 판정이 아니다.
- 한계 [I]: 실행 기록 입력이 없다. C 의 "뒤"는 소스 위치이고 CFG 순서가 아니다(개정 2에서 CFG 도달로 바꿈, §5f). 행위자는 필드 기반 may 근사다. 구획은 상수 값의 집합 비교다. 지역 변수 경유의 종료 도달은 may 다. A 의 `==` 비교 상대가 정의 하나인 지역 변수여도 facts 가 그 정의를 담지 않으면 풀지 않는다.

**시험** [M]: fixture `relations.c`, `static_golden_relations`(고정 도구 프로필 + `--extended-facts`: 모든 행에 사례 계층), `static_golden_relations_np`(`--no-profile --targets-rule R-T1,R-T2`; relations 를 두 번 돌려 같은 바이트), `test_static_cli` 의 확인(옵션 조합, 규칙 대상의 기록, relations 의 사용법·거부·`-o`·결정성·칸·계층).

## 5f. 순서·진입 경로 사실 `LOCK_PATHS`, P0 표 개정 2, relations 규칙 개정 2 (2026-10-30, 단계 2a S5 수정) [D][M]

근거: 단계 2a S5 결과에 대한 검증 지적. 결과는 `plan/step2a_results_20261030.md` §10. facts schema 개정 11(추출 레코드 개정 9, `docs/schema.md` §19).

**LOCK_PATHS** [D]. F1–F6 과 같은 조건으로 쓴다(프로필 없는 추출은 늘, 프로필 추출은 `--extended-facts` 일 때). 식 트리가 없다. LOCKSET 과 같은 잠금 골격·같은 호출 효과에서 구한다.
- `after-spawn`: spawn-op 호출 S 가 있는 함수에서 S 뒤 같은 블록의 사건과, S 블록의 후속에서 닿는 모든 블록의 사건이다. 되돌아가는 간선으로 S 블록에 다시 닿으면 그 블록의 사건(S 자신 포함)도 들어간다. 분기 시험으로 후속을 자르지 않는다: 과대 근사이고, "뒤"가 많을수록 C 의 "불가"는 줄어든다.
- `entry-release`: 함수 진입에서 그 사건까지의 어떤 경로에서, (a) 함수가 잡지 않은 잠금을 푸는 호출(모든 경로 또는 일부 경로의 해제), (b) 정적 인스턴스가 아닌 정체성의 잠금을 지울 수 있는 호출(callee 안의 짝), (c) 효과를 따라가지 못하는 호출(미해결 간접 호출, 자기 재귀 요소)이 있다. `site` 는 그런 호출 가운데 소스 위치가 가장 앞선 것이다. 분석 상태에 경로 합집합으로 합치는 표시 하나를 더해 구하며 LOCKSET 행은 바뀌지 않는다. 이 행이 있는 사건에서는 함수의 호출자가 잡은 잠금이 잡혀 있다고 말하지 않는다.

**P0 표 개정 2** (`p0.cpp`) [D]. 개정 1의 묶음에 같은 묶음의 구성원을 더했다(개정 3이 "통째로"를 고침: 초기화·소멸·속성·조회 구성원과 빠진 묶음은 §5g): 스레드 생성 `thrd_create`, mutex `pthread_mutex_clocklock`·`mtx_lock/trylock/timedlock/unlock`, spin lock `pthread_spin_lock/trylock/unlock`(배타 획득이라 mutex 종류), 세마포어 `sem_clockwait`, 조건 변수 `pthread_cond_clockwait`·`cnd_wait/timedwait/signal/broadcast`, 잠자기 `usleep`·`thrd_sleep`, 종료 `thrd_exit`·`quick_exit`·`__assert_fail`·`__assert_perror_fail`(assert 매크로가 부르는 함수), 스레드 id `thrd_current`, thread-specific data `tss_get/set`·`pthread_key_create/delete`·`tss_create/delete`, cleanup handler `__pthread_register_cancel(_defer)`·`__pthread_unregister_cancel(_restore)`(예외 없이 컴파일한 단위에서 `pthread_cleanup_push/pop` 매크로가 부르는 함수), 동적 적재 `dlmopen`·`dlvsym`. 72항목, 항목마다 판 표기(POSIX.1-2017 / glibc 2.39, ISO C11 <threads.h> / glibc 2.39, `*_clock*` 은 glibc 2.39·POSIX.1-2024). 원문 대조는 개정 1과 같다(`pthread_create` 만). 새 종류 넷: `thread-specific-key-create`(`entry_param` = 소멸자), `thread-specific-key-delete`, `cleanup-push`, `cleanup-pop`. 어휘가 아니고 유도에 쓰지 않는다(relations 의 수명 규칙만 읽는다).
- 이 개정이 담지 않는 묶음은 BASIS `extract.p0_table_scope` 에 이름으로 적는다: 읽기-쓰기 잠금 전체, barrier, one-time initialization, 다른 스레드의 취소, 시그널, `dlclose`·`dlerror`·`dladdr`, 컴파일러나 fortify wrapper 가 원문 호출 없이 부르는 함수. 읽기-쓰기 잠금을 통째로 뺀 까닭: 잠금 집합 분석은 공유 획득을 읽지 않는다. 쓰기 획득만 넣으면 읽기 획득 뒤의 해제가 호출자 잠금의 해제로 읽힌다. 통째로 빼면 그 획득이 잠금 연산이 아니어서 판정이 약해지기만 한다.
- P0 비교 표 개정 2: 바이트 문자열 비교에 `strcoll_l`·`strcasecmp_l`·`strncasecmp_l`·`bcmp`·`strverscmp`, 스레드 id 비교에 `thrd_equal`. 넓은 문자 비교와 `__memcmpeq` 는 이 개정 밖이다(머리의 `p0-compare scope` 줄).
- 대상 규칙(§5e)의 R-T2 조건 걷기에서 깊이 상한(200)을 없앴다. 풀은 같은 트리를 한 노드로 두고 노드를 자식 뒤에 만들므로 방문 집합으로 끝난다.

**relations 규칙 개정 2** [D].
- **C 의 "spawn 뒤"**: LOCK_PATHS `after-spawn` 이다. 생성자가 spawn 뒤에 부르는 함수(직접 호출과 해결된 간접 대상, 그 아래 전부)가 같은 필드를 쓰면 그 쓰기도 실현 가능성에 넣는다. 그 호출이 부른 함수 안의 쓰기는 호출 행의 잠금과 합친다(그 쓰기에 `entry-release` 가 없을 때). 그보다 아래의 쓰기는 제 잠금만 센다. 유도된 spawn 연산(본문 있음)이 인자를 통해 저장하는 것(PARAM_FLOW out-store, 주소 인자)은 그 호출 안에서 제 스레드 생성과의 순서를 facts 가 주지 않으므로 spawn 뒤의 쓰기로, 잠금은 모르는 것으로 센다. P0 스레드 생성의 out 인자는 새 스레드가 시작하기 전에 저장된다(P0 의미)고 보고 세지 않는다. 생성자 본문에서는 쓰지 않고 callee 에서만 쓰는 필드는 이 판에서 짝을 짓지 않고 `[C]` 머리에 수를 낸다.
- **호출 경유 읽기**: 읽기 행에 `entry-release` 가 있으면 호출 지점의 잠금을 읽기에 더하지 않고 이유를 적는다.
- **기댄 것**: `proven-impossible` 의 이유에 선언된 가정 셋과 필드 단위 완전성 규칙(PI 결정 대기, 정체성이 `type-level-exposed` 이면 형 단위로는 정적 인스턴스가 아님)을 적고 layers 칸에 `rests-on(...)` 을 붙인다. B 의 await 인정도 필드 단위 규칙과 `assume.no_cross_member_pointer_arithmetic` 을 붙인다.
- **basis 편차**: 추출이 basis 의 편차 항목을 BASIS `extract.basis_deviation[<항목>]` 으로 복사한다(확장 사실을 쓰는 추출). 행이 이름 붙인 위치·함수의 파일이 단위별 편차 항목의 단위(이름의 첫 `[...]`)이면 layers 칸에 `basis-deviation(...)` 을 붙인다. 이름 공간 블록에는 `namespace layers` 줄이 있다.
- **행위자**: spawn 의 entry 인자가 정적 이미지가 있는 전역 표의 필드에서 오면 그 멤버의 함수 잎(GLOBAL_IMAGES)이 entry 다(root 종류 `table-image-entry`; 생성 순서는 읽지 않는다). 그 멤버의 잎이 없으면(union 의 다른 멤버로 초기화) 풀지 않고 머리의 `spawn calls with no resolved entry` 줄에 이유를 적는다. 트램펄린은 process 가 아닌 root 가운데 다른 root 에 **간접 호출로만** 들어가고, 제 본문이 공유 root(전역, 전역 주소만 돌려주는 함수의 결과)로 필드·전역을 읽지 않으며 전역 객체를 쓰지 않는 것이다. 형 있는 레코드 필드 쓰기는 facts 에 base 가 없어 보지 않는다. 다른 root 에 들어가지만 행위자로 남는 root 는 머리에 이유별로 적는다.
- **종료 처리**: 종료 연산이 may 또는 noreturn 이면 그 종료 호출들의 guard 를 적고, 이 호출에서 인자가 갖는 상수 값(guard 아래 쓴 필드의 상수, 상수 인자)으로 평가한다. 그 함수의 탈출 없는 루프도 적는다(그 길로는 돌아오지 않고 P0 종료에도 닿지 않는다).
- **수명**: entry 경로 규칙은 프로그램에 null 이 아닌 소멸자를 단 thread-specific key 생성(P0 `thread-specific-key-create`)도 cleanup push 도 없을 때만 "그 entry 가 돌아온 뒤 프로그램의 함수가 돌지 않는다"고 쓴다. 있으면 그것들을 적고 주장하지 않는다. P0 out 인자 줄은 relations 가 ARGFLOW 와 P0_TABLE 을 맞춰 만든 것이고 추출의 writer 집합(WRITERS, FACT_LOCS, FIELD_STATUS)에는 없다고 적는다. writer 수에서 covers-more-than-the-field 행을 따로 센다.
- **검사 중복 제거**: callee 검사와 읽기 함수의 상수 반환은 (값, 조건 트리)로 묶는다. 40단계 한계는 출력에만 쓴다.

**시험** [M]: fixture `relpaths.c`(루프 안 spawn, spawn 없는 분기의 쓰기, spawn 뒤 helper 의 잠금 밖 쓰기, 호출자 잠금을 푼 뒤의 읽기, union 멤버로 초기화한 entry 표, 다른 root 를 직접 부르는 root, 간접 호출만 하는 트램펄린, 소멸자 있는 key, 인자 guard 가 있는 noreturn 종료 연산, 인자를 통해 저장하는 spawn wrapper)와 `deepcond.c`(조건의 깊이 211). `static_golden_lockpaths_np`, `static_golden_relpaths_np`, `test_static_cli`·`test_static_wrappers` 의 확인.

## 5g. relations 규칙 개정 3, P0 표 개정 3, 걷기 상한의 기록 (2026-10-30, 단계 2a 둘째 수정) [D][M]

근거: 수정 단계 결과에 대한 둘째 검증 지적. 결과는 `plan/step2a_results_20261030.md` §11. facts schema 는 개정 11 그대로다(섹션·레코드 배치 변화 없음). 추출의 변화는 P0 표의 문장(개정 3)과 BASIS 의 걷기 상한 기록 둘뿐이다.

**relations 규칙 개정 3: 경로 C 의 "불가"** [D]. `proven-impossible` 은 다음이 모두 성립할 때만 낸다.
- **읽기 문맥**: 읽기가 제 함수 안에서 정적 인스턴스 잠금을 잡으면 그 읽기 하나다. 아니면 (a) 읽는 함수가 spawn 된 스레드의 entry(또는 자식 행위자의 root)이면 호출자 잠금 없이 도는 문맥 하나, (b) 직접 호출마다, (c) 해결된 간접 호출마다(CALL_TARGETS) 문맥 하나씩이다. 자식 행위자가 닿지 않는 호출의 문맥은 행으로 내되 `not graded` 로 두고 호출 지점의 잠금으로 등급을 매기지 않는다(머리 `[C]` 와 summary 에 수).
- **spawn 뒤의 쓰기**: 생성자 본문(LOCK_PATHS `after-spawn`)과 그 뒤 호출이 돌 수 있는 함수(직접 호출, 해결된 간접 대상, 본문 없는 P0 밖 함수에 넘긴 함수 인자; 그 아래 전부)의 LOCKSET 쓰기 가운데 그 필드의 형 있는 쓰기, 그 필드의 레코드 형 전체 쓰기(`T.*`: memset·구조체 대입·포함한 레코드의 쓰기), 그 필드에 닿을 수 있는 다른 필드의 데이터 함수 쓰기(WRITERS `covers-more-than-the-field` 와 같은 위치)를 모두 센다. 짝은 이전처럼 생성자가 제 본문에서 형 있게 쓰는 필드만 짓는다(전체 쓰기만 있는 레코드 형은 `[C]` 머리에 수).
- **잠금의 연속**: 읽기와 spawn 뒤의 모든 쓰기가 한 정체성 L 을 잡고, L 이 spawn 에서 같은 획득으로 잡혀 있으며(획득 지점 집합이 같음), 그 획득 지점이 spawn 뒤 사건(LOCK_PATHS `after-spawn`)이 아니어야 한다. 획득 지점이 spawn 뒤에 다시 닿으면 그 사이에 풀렸다 다시 잡힐 수 있어 `possible` 이다(되돌아가는 간선). L 이 정적 인스턴스가 아니면 `undecidable`(must 정체성 미증명, ED §3.3)이다. 읽기와 쓰기가 다른 정체성을 잡고 쓰기 쪽이 spawn 부터 연속이면, 둘 다 정적 인스턴스일 때만 `possible`(서로 다른 인스턴스), 아니면 `undecidable` 이다.
- **필드의 완전성**: 그 필드가 FIELD_STATUS 필드 단위 규칙(PI 결정 대기)으로 writer-set-complete 여야 한다. 그 규칙에서는 형 없는 저장이 그 필드에 닿지 않는다(그래서 형 없는 쓰기는 따로 세지 않는다). 필드가 추출 대상이 아니어서 데이터 함수 writer 의 위치를 모르면 `undecidable` 이다.
- **풀리지 않은 호출**: spawn 뒤(생성자와 그 아래)에 대상이 풀리지 않은 간접 호출이 있으면 `undecidable` 이다(그 호출이 그 필드를 잠금 없이 쓸 수 있다). 본문 없는 P0 밖 함수는 그 필드의 레코드에 대한 포인터를 받으면 FIELD_STATUS 의 unknown effect 로 필드가 완전하지 않게 되므로 따로 세지 않고, 함수 인자를 받으면 그 함수를 호출 대상으로 센다.
- **범위**: 생성자의 한 호출(ED §3.3 항목 3), spawn 부터 반환까지의 CFG(루프 포함)다. 생성자를 부르는 함수(직접·간접)의 제 본문에 그 필드·레코드 쓰기나 풀리지 않은 간접 호출이 있으면 반환과의 순서를 facts 가 주지 않으므로 `undecidable` 이다. 그 함수들이 부르는 함수와 그 위의 호출자가 반환 뒤에 하는 일은 읽지 않는다. 이 범위를 `proven-impossible` 이유 문장에 적는다.
- **기댄 것**: 정적 인스턴스 판정을 쓰는 모든 등급(불가, 잠금 창, 다른 획득, 다시 잡힘, 두 정적 인스턴스)에 `rests-on(...)` 으로 선언된 가정 셋과 필드 단위 규칙을 붙인다. `possible` 의 등급 자체는 형 단위 규칙으로도 같다(잠금이 정적 인스턴스가 아니어도 창은 창이다). 잠금을 이름 붙인 행은 P0 잠금 기본 연산에 기댄다(아래 `P0-unverified`).

**경로 B** [D]. 생성 순서는 펼친 table walk(LOOP_ITERS)에서만 읽는다. 등급 문장은 `fixed unconfirmed (creation order only)` 와 `fixed unconfirmed (creation order + await; the await rests on the L1 premise only)` 로 나뉜다. await 문장은 "await 루프가 생성 뒤에 온다(L1 전제 성립, L1 자체는 판정하지 않음; 그 술어가 생성된 행위자에 관한 것인지, 시간 초과 출구가 생성자를 끝내는지는 읽지 않음)" 이다. 정적 이미지에서 푼 root 끼리는 순서를 유도하지 않는다고 적고 이미지 경로(`[k]...`)를 보인다.

**행위자** [D]. 트램펄린이 대신하는 root 에 유도 spawn 연산의 호출 지점에서 푼 entry 를 더한다: SPAWN_WRAP `called-by-entry` 행(연산 F 가 안쪽 spawn 에 트램펄린 T 를 넘김)마다 F 를 부르는 호출의 spawn entry(table walk, 정적 이미지, 시작 스크립트, CALL_TARGETS)가 T 의 root 다. entry 를 풀지 못한 그런 호출은 머리에 수와 위치를 적는다(과소 근사). 정적 이미지 entry 는 그 멤버의 FIELD_STATUS 가 두 규칙 모두 complete 이고 writer 인스턴스가 없으며 쓰인 잎(object-, field-, record-written)이 없을 때만 쓴다. 아니면 풀지 않고 이유를 적는다.

**경로 A** [D]. 이미지 키의 잎이 실행 중에도 쓰이면(field- 또는 record-written) 키는 `unresolved (image, field written at run time)` 이고 "키 상수 다름" 제외와 같은 키 뒤집힘에 쓰지 않는다. probe-create(ED §3.5)는 같은 함수, 같은 키 인자, 생성자 키 읽기가 조회 결과 아래에 더해 극성을 본다: 검색 함수의 상수 반환(RET_CONSTS) 가운데 키 비교가 맞지 않을 때(P0 비교 호출이 0 이 아님, `==` 거짓, 콜백 검색이면 그 콜백의 반환값을 같은 치환으로 평가) 성립하는 값을, RETURNS 의 호출 결과 반환을 따라 조회 함수까지 올린 값 F 에서 생성자 읽기의 guard 가 거짓이 아니고, F 밖의 값(guard 가 비교하는 상수, 검색의 다른 상수 반환) 하나에서 거짓이어야 한다. probe-create 는 그 조회 행의 handling 칸에 `handling: probe-create: ...` 로 적는다. 짝이 없을 때 dependency 칸은 "no creator left after exclusions (probe-create n, key constants differ m, same actor only k)" 또는 "no creator of this partition" 이다.

**수명** [D]. entry 경로 문장은 읽지 않는 것을 이름으로 적는다: 프로세스 종료 처리기(`atexit`, `at_quick_exit`, `on_exit`), 소멸자 함수, 예외를 켠 단위(그때 `pthread_cleanup_push` 는 함수를 부르지 않는다).

**layers 칸의 `P0-unverified(...)`** [D]. 행의 유도가 쓴 P0 항목 가운데 원문 대조가 없는 것을 이름으로 적는다: spawn 연산 사슬의 기본 연산, 잠금을 이름 붙인 행의 잠금 기본 연산(프로그램이 부르는 것), 자기 정체성 사슬의 기본 연산, 시작 스크립트 entry 의 기호 조회, F8 지연·종료 사슬의 기본 연산, 이름 공간 검색의 P0 비교 함수, 수명 문장의 key 생성. 머리에 `pthread_create` 의 원문 대조가 upstream glibc 2.39 사본에 대한 것이고 빌드가 링크하는 배포판 패키지의 패치는 대조하지 않았다고 적는다.

**P0 표 개정 3** (`p0.cpp`) [D]. 항목 수(72)와 종류는 그대로다. 바뀐 것: `pthread_create` 원문 줄에 배포판 패치 미대조, `thrd_create` 의미에 "새 스레드가 시작하기 전에 저장"(relations 가 P0 스레드 생성의 out 인자를 spawn 뒤 쓰기로 세지 않는 근거; 원문 미대조), 그리고 scope 문장. scope 는 "묶음을 통째로"라고 하지 않고, 묶음의 어느 구성원이 항목인지(잡기·풀기·기다리기·알리기·잠자기·끝내기·id·key·handler·적재·조회·메모리 저장)와 빠진 묶음·구성원을 이름으로 적는다: 스레드 join·detach, POSIX timer(SIGEV_THREAD 는 새 스레드를 시작하지만 행위자가 아님), I/O 다중화, mutex·spin lock·세마포어·조건 변수·스레드 속성의 초기화·소멸·속성·조회 구성원(`sem_getvalue` 포함), 스케줄링, 이 판에 없는 데이터 함수(`stpcpy`, `stpncpy`, `strlcpy`·`strlcat`, `mempcpy`, `memccpy`, `explicit_bzero`, 넓은 문자 구성원), 인자를 통해 저장하는 형식 입력(`sscanf` 등), 프로세스 종료 처리기 등록.

**걷기 상한** [D]. 고정 상한이 남은 곳과 처리(셋째 수정에서 식 트리 깊이 24 를 더 기록함: §5h):
- FACT_INIT 의 한 객체 초기화자 걷기 4096 노드(`ast_tu.cpp` `kInitBudget`, 기존): 상한에서 멈추면 그 객체에 "상수가 아님" 행을 더한다(전에는 잎을 하나도 못 읽고 멈추면 0 으로 적었다). relations 는 그런 행이 있는 필드의 초기값을 쓰지 않는다.
- 함수 값을 담은 정적 초기화자의 중첩 6 단계(`ast_fn.cpp` `kFpInitDepth`, 기존): 그대로 두고 잘린 수를 센다.
- 두 상한은 BASIS `param.walk_bounds`(unspecified_by_reference)와 `count.walk_bounds_hit` 에 확장 사실과 함께 기록한다. 프로필 추출(옵션 없음)의 BASIS 는 바뀌지 않는다.
- PARAM_FLOW 사슬 문장의 반복 상한(100000)과 텍스트 보기의 SEQ 걷기 상한(10000)은 방문 집합·후위 번호 조건으로 바꿨다(출력 변화 없음). 텍스트 보기가 PTR_FIELDS 원소의 정적 값을 64 원소까지만 적는 것은 보기의 출력 한계로 두고, 넘으면 그렇다고 적는다.

**시험** [M]: fixture `relsound.c`(entry 읽기, 간접 호출자, 잠금 밖 레코드 전체 쓰기, 루프 안 잠금·해제, 정적 인스턴스가 아닌 잠금, 풀리지 않은 호출, 호출자의 반환 뒤 쓰기, 대조군)와 `static_golden_relsound_np`, `test_static_cli` 의 확인(이미지 필드가 실행 중에도 쓰이는 `relations.c` 변형의 키, entry 표 원소를 지우는 `relpaths.c` 변형의 정적 이미지 entry 포함). `test_static_wrappers` 가 P0 표 개정 3의 scope·문장을 본다. lint `case-ids` 는 `tools/` 도 읽는다.

## 5h. relations 규칙 개정 4, 잠금 집합의 대기·겹침 규칙, FIELD_STATUS 의 스칼라 배열, 식 깊이 상한의 기록 (2026-10-30, 단계 2a 셋째 수정) [D][M]

근거: 둘째 수정 결과에 대한 셋째 검증 지적. 결과는 `plan/step2a_results_20261030.md` §12. facts schema 는 개정 11 그대로다(섹션·레코드 배치 변화 없음). FIELD_STATUS flag 집합에 항목 하나를 더했다(`docs/schema.md` §19 셋째 수정). 추출의 변화는 LOCK_PATHS(대기), LOCKSET(겹치는 해제), FIELD_STATUS(스칼라 배열)와 그것을 읽는 LOCK_IDS·AWAIT_L1, BASIS 의 걷기 상한 기록이다. 옵션 없는 프로필 추출은 이 코드 경로를 타지 않는다(바이트 동일, STRINGS 말고).

**잠금 집합(F5)의 대기** [D]. 함수가 잡지 않은 잠금으로 기다리는 호출(P0 조건 대기, 또는 요약에 그런 대기가 있는 callee)은 기다리는 동안 호출자의 잠금을 푼다. 그래서 LOCK_PATHS `entry-release` 의 (a)에 넣는다: 그 뒤의 사건에서는 호출자가 잡은 잠금이 잡혀 있다고 말하지 않는다. 생성자가 spawn 뒤 부르는 함수 안에서 그런 대기 뒤에 쓰면, 그 쓰기에 호출 지점의 잠금을 합치지 않는다. 함수 제 본문에서 잡은 잠금으로 기다리면 이전과 같이 대기 호출부터 다시 잡힌 것으로 한다(획득이 끊긴다).

**잠금 집합(F5)의 겹침** [D]. 해제(또는 짝의 소거)가 지우는 잡힌 잠금은 같은 mutex 일 수 있는 것 전부다. 이전 규칙(텍스트 같음, 주소가 아닌 정체성, 필드 기반 위치 `may` 같음)에 더해 **접근 경로가 겹칠 수 있는 것**도 지운다.
- 같은 전역 객체에서 시작하는 두 경로: 한 경로가 다른 경로의 접두이면(객체 자신과 그 첫 멤버처럼 바깥 객체와 그 부분; 첨자는 비교하지 않는다) 겹친다. 처음 갈라지는 곳이 한 union 의 두 멤버이면 겹친다. 한 구조체의 서로 다른 멤버이면 겹치지 않는다.
- 포인터로 시작하는 경로(`p->f…`): 그 첫 멤버를 선언한 레코드 형 T 의 객체를 상대 경로가 이름 붙이는 곳(뿌리 전역 객체의 형, 경로의 단계가 지정하는 객체의 형, 단계의 멤버를 선언한 형)마다 거기서부터 위와 같이 비교한다.
- 서로 다른 전역 객체는 겹치지 않는다(C11 6.5.6 p8, 6.5.9 p6: 한 객체의 주소에서 다른 객체로 넘어가는 포인터 연산은 정의되지 않음).
- 정체성 노드는 이를 위해 레코드 형(전역 객체와 멤버가 지정하는 객체의 형, 멤버를 선언한 형)과 union 멤버 표시를 지닌다. 정체성의 텍스트·`may`·LOCK_IDS 행은 바뀌지 않는다.

**FIELD_STATUS 의 스칼라 배열** [D]. 필드의 형이 const 가 아닌 스칼라의 배열이면(`int flags[4]`, `uint32 ids[N]`) 두 규칙 모두 DISABLED 이고 flag `array-element-stores-not-collected` 를 단다. 원소 하나에 대한 저장(`x.f[i] = v`)은 WRITERS·FUNC_WRITES·FIELD_STATUS·LOCKSET 어디에도 writer 행을 만들지 않기 때문이다(§5d 한계; 기존 섹션을 바꾸지 않으려고 수집은 고치지 않았다). 레코드의 배열은 해당하지 않는다: 원소 멤버 쓰기는 그 멤버의 쓰기, 원소 통째 대입은 레코드 쓰기(`T.*`)와 aggregate-assign writer 로 잡힌다 [M: 시험 fixture]. 원소가 const 인 배열(`const char Name[N]`)도 해당하지 않는다: 원소에 저장하려면 const 를 벗기는 cast 나 주소 escape 를 거쳐야 하고, 그것은 그 필드나 형의 A-TYPE 적중으로 FIELD_STATUS 에 잡힌다고 본다 [I]. 이 판정을 LOCK_IDS 의 정적 인스턴스 판정, AWAIT_L1, relations 의 정적 이미지 entry·이미지 키·경로 C 의 완전성 조건이 따른다.

**걷기 상한** [D]. 식 트리 깊이 24(`ast_fn.cpp` `kExprDepth`, 기존: 더 깊은 부분식은 `top "too deep"` 노드)를 BASIS `param.walk_bounds` 에 더하고 잘린 부분식 수를 `count.walk_bounds_hit` 에 센다(확장 사실을 쓰는 추출). 이 상한과 FACT_INIT 4096 노드 상한은 옵션 없는 프로필 추출에서도 작동한다. 4096 에서 멈춘 객체에 "상수 아님" 행을 더하는 것(§5g)도 프로필 추출에 해당하며, 그 BASIS 는 상한도 걸린 수도 기록하지 않는다. 식별 시험의 여섯 경우(V0·V2 × C1 pre tx·R2 core·C5 6.4.1)는 바이트 동일이므로 그 경우들에서는 걸리지 않았다 [M]. 그 밖의 고정 상한은 §5g 의 것 그대로다.

**relations 규칙 개정 4: 경로 C** [D].
- **spawn 의 스레드**: 해결된 entry 의 행위자(자식)와, 그 코드가 제 spawn 호출로 시작하는 스레드(후손, 추이)다. 그 코드는 그 root 들(과 트램펄린 entry)에서 호출 그래프(직접 호출, 해결된 간접 대상, 본문 없는 P0 밖 함수에 넘긴 함수)로 닿는 함수이고 **다른 root 에서 멈추지 않는다**(자식이 직접 부르는 root 의 코드는 자식에서 돈다). 유도 spawn 연산이 제 매개변수를 넘기는 안쪽 spawn 호출(SPAWN_WRAP `entry-argument`, `called-by-entry`)은 그 연산의 호출에서 읽는다(그 호출이 spawn 호출이다). 읽기가 그 코드에 있거나 그 함수의 행위자 집합에 그 스레드가 있으면 행을 낸다. 행의 spawn 줄에 후손을 따로 적는다.
- **완전하지 않은 스레드 집합**: 그 안의 트램펄린이 entry 를 풀지 못한 유도 spawn 호출의 스레드도 대신하거나(`calls of such an operation with no resolved entry`), 그 코드가 entry 를 풀지 못한 spawn 호출을 하면, 그 스레드들이 닿지 않는 문맥은 `not graded` 가 아니라 `undecidable`(이유를 적음)이다.
- **읽기 문맥 더함**: (d) 읽는 함수를 본문 없는 P0 밖 함수에 넘기는 호출마다 문맥 하나(그 호출 동안 되부를 수 있음: 그 호출 지점의 잠금으로 등급; `call_runs` 와 같은 규칙). (e) 읽는 함수의 주소가 잡혀 있고 그 스레드들의 코드에 대상이 풀리지 않은 간접 호출이 있으면 문맥 하나(잠금 모름: `undecidable`). 그런 문맥이 있으면 그 읽기의 다른 문맥이 `proven-impossible` 이어도 행이 따로 있다.
- **생성자의 호출자**: 생성자를 본문 없는 함수에 넘기는 호출도 호출자로 센다(그 호출을 하는 함수의 제 본문을 본다). 생성자의 주소가 잡혀 있고 facts 에 대상이 풀리지 않은 간접 호출이 있으면 호출자를 다 알지 못하므로 `undecidable` 이다(그 호출을 적음).
- **두 번 돌아오는 함수**: C 라이브러리의 `setjmp`, `_setjmp`, `__setjmp`, `sigsetjmp`, `__sigsetjmp`(glibc 의 `pthread_cleanup_push` 매크로가 예외 없는 단위에서 부름), `savectx`, `getcontext`, `vfork` 를 이름으로 읽는다(머리에 목록과 호출 수). 생성자, spawn 뒤 호출이 돌 수 있는 함수, 읽는 함수, 문맥 호출의 함수 가운데 하나라도 그것을 부르면 `proven-impossible` 대신 `undecidable` 이다: 비국소 점프(`longjmp`, `siglongjmp`, `setcontext`, 취소의 풀기)가 그 호출로 돌아와 제어 흐름 그래프에 없는 순서와 잠금으로 코드를 돌린다. P0 표 밖이다(이 이름들은 relations 의 목록이고 추출의 P0_TABLE 은 바꾸지 않았다).
- **필드의 완전성**: FIELD_STATUS 의 스칼라 배열 표시가 있으면 이유에 적는다.
- **"불가" 문장**: (1) 이 호출의 쓰기에 관한 것이고, 그 쓰기의 키(WRITERS key 를 적음)가 이 호출이 시작하는 자식의 레코드인지(다른 호출이 등록하는 레코드가 아닌지)는 정하지 않는다(ED §3.3 항목 3 의 "쓰기 첨자가 생성자 정체성에서 옴"은 검사하지 않음). (2) spawn 연산이 본문 있는 유도 연산이면, 그 연산이 스레드를 시작한 뒤 실패를 알리고 생성자가 쓰기를 건너뛰는 길은 덮지 않는다(그 길에서 자식은 이전 값을 읽을 수 있다; 이 판은 읽지 않음). (3) 아래 전제를 이유와 `rests-on(...)` 에 적는다.
- **경로 C 의 전제**(premises of path C; 선언된 가정 셋, 필드 단위 규칙과 함께 "불가"가 기댄다): 두 잠금 정체성은 접근 경로가 보이는 대로만 같은 mutex 일 수 있다(위의 겹침; 상대 경로가 이름 붙이지 않은 중첩 멤버를 가리키는 포인터나 관련 없는 레코드 형 사이의 cast 로 같은 mutex 를 부르는 것은 보지 않음). 본문 없는 함수는 잠금에 효과가 없고 그 호출에서 받은 함수만 되부른다(앞서 저장해 둔 함수를 나중 호출에서 부르는 것은 보지 않음). 두 번 돌아오는 함수는 위의 이름 목록으로만 안다. (§5i 에서 (나)를 고치고 (라)–(바)를 더함: 받은 함수의 잠금 효과는 그 호출에서 읽는다.)
- **뒤집힘**: 실현 가능성이 센 spawn 뒤 쓰기(생성자·callee 의 형 있는 쓰기, spawn 연산의 인자 저장은 그 호출의 out-pointer writer 값)의 상수 값으로 평가한다. 레코드 전체·덮는 쓰기와 상수가 아닌 값은 평가하지 않음으로 적는다. 이전에는 생성자 제 본문의 상수 쓰기(spawn 앞의 것 포함)만 썼다.
- **`P0-unverified`**: C 의 모든 등급(잠금 집합에서 읽은 것: 잠금 없음, 잠금 창, 실패 경로 포함)과 잠금을 이름 붙인 모든 C 행에 프로그램이 부르는 잠금 기본 연산을 붙인다(§5g 의 문장대로).
- `not graded` 는 ED §3.4 의 등급 셋 밖의 넷째 값이다. 다음 ED 개정에서 다룰 편차로 결과 문서 §8 에 적는다.

**경로 A** [D]. 이미지 키는 정적 이미지 entry(§5g)와 같은 규칙일 때만 값이다: 잎에 object-/field-/record-written 표시가 없고, 그 멤버의 FIELD_STATUS 가 두 규칙 모두 complete 이며, 실행 중 writer 가 없다. 아니면 키는 `unresolved (image, ...)`(이유와 함께)이고 "키 상수 다름" 제외와 같은 키 뒤집힘에 쓰지 않는다.

**시험** [M]: fixture `relsound.c` 에 열한 경우(`wait_*` callee 의 조건 대기, `waitp_*` 두 단계 아래 매개변수 mutex 의 대기, `first_*` 첫 멤버와 객체, `union_*` union 의 두 멤버, `back_*` 본문 없는 함수의 되부름과 풀리지 않은 간접 호출, `addr_*` 주소가 잡힌 생성자, `jump_*` setjmp, `elem_*` 스칼라 배열, `root_*` 다른 root 를 직접 부르는 자식, `desc_*` 후손, `opn_*` entry 를 풀지 못한 spawn)와 `static_golden_relsound_np`. `test_static_cli` 가 그 등급과 이유, 이미지 키의 형 없는 저장 변형(`relations.c` 변형), 걷기 상한 24 를 본다. fixture `lockset.c` 의 `fx_shared.tasks`(스칼라 배열)가 FIELD_STATUS 에서 DISABLED 가 되었다(golden `lockset`, `lockset_np`).

## 5i. relations 규칙 개정 5, 잠금 집합 규칙 개정 2, 불완전한 간접 호출 대상, 내부 연결 객체, 걷기 상한 둘의 기록 (2026-10-30, 단계 2a 셋째 수정 둘째 차례) [D][M]

근거: 셋째 수정 결과에 대한 넷째 검증 지적 12건(막는 것 4). 결과는 `plan/step2a_results_20261030.md` §13. facts schema 는 개정 11 그대로다(섹션·레코드 배치 변화 없음). 열거형 항목 둘을 더했다(`docs/schema.md` §19 셋째 수정 둘째 차례): CALL_TARGETS 이유 `incomplete-source`, DERIVES 이유 `walk-bound`. 옵션 없는 프로필 추출은 이 절의 새 경로를 타지 않는다(바이트 동일, STRINGS 말고 [M]).

**불완전한 간접 호출 대상(`incomplete-source`)** [D]. 확장 사실을 쓰는 추출에서, 해결된 간접 호출의 함수 값이 흐름 그래프가 따라가지 않는 저장을 거칠 수 있으면 그 호출의 후보 행 뒤에 fn 없는 CALL_TARGETS 행(이유 `incomplete-source`, `via` 에 열린 노드와 까닭)을 붙인다. 호출의 노드에서 흐름 간선을 거슬러 닿는 노드 가운데 하나라도 다음이면 열린 것이다(`lockset.cpp` `mark_incomplete_targets`).
- 함수 포인터가 아닌 형의 변수·필드(정수, 함수 포인터로 바꾼 데이터 포인터): 그 저장은 간선을 만들지 않는다.
- 본문 없는 함수의 결과, 주소가 잡힌 함수의 매개변수(간접 호출의 인자는 따라가지 않음), 본문 없는 함수의 함수 포인터 포인터 매개변수(그 함수가 그것을 통해 저장하는 것은 읽지 않음).
- 주소가 포인터 변환만 거쳐 직접 호출의 인자로 나가는 노드(ARGFLOW address, pointer-conversion 또는 array-contents; `memcpy(&hook, …)`).
- 주소를 잡거나 배열이 첫 원소 포인터로 바뀌는 곳이, 직접 호출의 함수 포인터 포인터 매개변수에 변환 없이 넘기는 인자나 위의 변환 인자가 아닌 노드: cast lvalue(`*(void **)&x = dlsym(…)`), 그 주소를 지니는 포인터 변수·정적 초기화자, 간접 호출·가변 인자, 정수 변환(`Model::flow_addr_open`).
- 값이 복사되는 함수 포인터 포인터 변수·매개변수(`q = out` 뒤 `*q = f`): 그 노드는 가리키는 것을 대신하지만 복사본을 통한 저장은 따라가지 않는다. 검사·비교·`*p`·`p[i]`·호출의 변환 없는 함수 포인터 포인터 인자는 복사가 아니다.
- FIELD_STATUS 행이 없거나, 필드 단위 규칙으로 complete 가 아니거나, 대입이 아닌 writer(memset, memcpy, out-pointer, 구조체 대입, unknown effect)가 있는 필드.
그 호출은 잠금 집합(LOCK_OPS·LOCKSET 의 unknown 효과)과 relations(spawn 뒤 호출의 `unknown`, 생성자의 호출자 검사, 스레드 코드의 미해결 호출)가 대상이 풀리지 않은 호출로 읽는다. BASIS `count.indirect_calls_incomplete` 에 수를 적는다. 남는 것은 인라인 어셈블리의 저장과 다른 객체에서 포인터 연산으로 닿는 저장(정의되지 않은 동작)이고, 경로 C 의 전제 (마)로 적는다. 프로필 추출은 이 노드를 읽지 않는다(CALL_TARGETS 그대로).

**잠금 집합(F5) 규칙 개정 2** [D].
- **본문 없는 함수의 되부름**: 본문이 없고 P0 항목도 P0 비교 함수도 아닌 함수의 직접 호출에서, 그 호출이 인자로 넘기는 본문 있는 함수(ARGFLOW 원천 function; relations `call_runs` 와 같은 집합)는 그 호출 동안 돌 수 있다. 그 함수들의 요약을 그 호출의 may 효과로 적용한다: 해제·may 해제·소거는 may 해제, 대기는 대기, unknown 은 그대로이고 획득은 버린다. 그래서 그 호출에서 잡힌 잠금이 빠지고 LOCK_PATHS `entry-release` 가 생긴다. 그 함수들은 호출자보다 먼저 분석한다(호출 그래프에 그 간선을 더한 SCC 순서; 같은 SCC 이면 unknown).
- **내부 연결 객체**: 객체 키 `name@file:line` 은 정의 위치 하나에 하나다. 헤더에 정의된 `static` 객체는 그 헤더를 포함하는 단위마다 다른 객체다(C11 6.2.2p3). 그런 객체가 둘 이상의 단위에서 정의되거나, 단위의 주 파일이 아닌 파일에 정의된 것이 잠금 정체성에 쓰이면 LOCK_IDS 는 `not-static`(이유를 적음)이다. 함수 키도 같은 방식이어서(헤더의 `static inline` 함수) 단위마다의 본문이 한 함수로 합쳐진다. 그 본문은 같은 원문이고, 그 안의 내부 연결 잠금은 위 규칙으로 `not-static` 이 된다. 잠금이 아닌 필드에 대해서는 단위마다 다른 객체를 하나로 합쳐 짝이 느는 쪽이다 [I].

**relations 규칙 개정 5: 경로 C** [D].
- **생성자의 재진입**: spawn 뒤 호출이 돌 수 있는 함수에 생성자 자신이 있으면(상호 재귀 포함) 그 중첩 호출에서 spawn 의 획득 지점이 다시 돌아 새 획득이므로 `possible` 이다(그 호출을 적음).
- **아래 프레임의 획득**: 생성자 아래 프레임의 쓰기가 제 프레임의 획득으로 잠금을 잡고 그 획득 지점이 spawn 의 획득 지점과 같으면(같은 함수에 다시 들어옴) 새 획득이므로 `possible` 이다. callee 프레임의 제 획득은 spawn 의 획득으로 받지 않는다.
- **스레드 코드의 미해결 간접 호출**: spawn 의 스레드(자식과 후손)의 코드가 대상이 (전부) 풀리지 않은 간접 호출(`incomplete-source` 포함)을 하면, 그 스레드들이 정적 호출 그래프로 닿지 않는 문맥이라도 그 호출의 함수가 주소가 잡힌 함수에서 돌 수 있으면(`call_runs` 의 추이) `undecidable` 이다(그 호출들을 적음). 그런 읽기 함수는 닿지 않아도 읽기로 받는다(호출이 없으면 (e) 문맥 하나). 나머지 닿지 않는 문맥은 `not graded` 이고, 그 문장에 "그 코드는 대상이 풀리지 않은 간접 호출을 하지 않음" 또는 "그 호출은 주소가 잡힌 함수만 부르고 이 함수는 거기서 돌지 않음"을 적는다.
- **정적 이미지 entry 의 전제**: spawn 의 entry 를 정적 이미지의 함수 잎에서 풀 때(§5g), 그 이미지가 읽는 멤버를 덮는 상수 아닌 잎이나 같은 레코드의 다른 멤버의 함수 잎(union 이면 그 멤버를 읽어도 그 값)을 가지면, spawn 이 그 멤버가 함수 잎인 원소만 읽는다는 것은 spawn 의 guard 에 기댄다(이 개정은 guard 를 읽지 않음). 머리의 정적 이미지 줄에 그것을 적고 그 spawn 의 스레드 집합을 완전하지 않은 것으로 본다(닿지 않는 문맥은 `undecidable`).
- **경로 C 의 전제**(§5h 의 (가)–(다)에 더함; 이유 문장과 `rests-on(premises of path C …)`): (나)를 고쳐 "본문 없는 함수는 제 잠금 효과가 없고 그 호출에서 이름으로 받은 함수만 되부르며, 그 함수의 잠금 효과는 그 호출에서 읽는다"로 한다(함수 포인터 변수·필드에 담아 넘긴 함수, 저장해 두었다가 나중에 또는 비동기로 부르는 함수는 보지 않음). (라) mutex 는 잡은 스레드만 풀고, 그 객체는 P0 기본 연산으로만 바뀐다(다른 스레드의 해제와 잡힌 mutex 위의 일반 저장은 정의되지 않은 동작이고 보지 않음). (마) 간접 호출의 대상은 `incomplete-source` 표시가 없으면 해결된 후보 전부다(인라인 어셈블리와 다른 객체에서의 포인터 연산 저장은 보지 않음). (바) 대상이 풀리지 않은 간접 호출은 주소가 잡힌 함수만 부른다(실행 중 기호 조회가 돌려주는 함수는 시작 스크립트 entry 로 푼 것 말고 세지 않음).

**경로 A** [D]. probe-create(§5g)는 한 행위자가 조회와 생성을 모두 돌릴 때만 짝에서 뺀다(ED §3.5: W(u)는 같은 행위자의 쓰기를 프로그램 순서가 정할 때만 뺀다). 둘 이상의 행위자가 그 함수를 돌리면 짝으로 두고(한 행위자의 조회와 다른 행위자의 생성 사이) handling 칸에 행위자 수와 함께 적는다.

**걷기 상한** [D]. 남은 고정 상한을 다시 찾아 둘을 더 기록했다: DERIVES 복사 사슬 8(`ast_fn.cpp` `kDeriveDepth`; 잘린 사슬은 이유 `walk-bound`, 전에는 `not-a-copy` 로 틀렸다)과 LOOP_ITERS 에 저장하는 식 텍스트의 깊이 64(`model.cpp` `kEmitDepth`; 더 깊은 부분은 `...`). 둘 다 BASIS `param.walk_bounds` 에 값을, `count.walk_bounds_hit` 에 걸린 수를 확장 사실과 함께 적는다. DERIVES 는 역할 있는 인자에만 나오므로 프로필 없는 추출에는 행이 없다. 두 상한도 4096·24 처럼 프로필 추출에서 작동하고 그 BASIS 는 기록하지 않는다(식별 시험 여섯 경우는 바이트 동일이므로 걸리지 않았다 [M]). 그래서 남은 고정 상한은 FACT_INIT 4096 노드, 함수 값 초기화자 중첩 6, 식 트리 깊이 24, DERIVES 사슬 8, LOOP_ITERS 텍스트 64, 그리고 S3 의 `--image-budget` 기본 4096 이다. 출력에만 쓰는 한계(텍스트 보기의 식 40 단계, PTR_FIELDS 원소 64)는 facts 를 바꾸지 않는다.

**시험** [M]: fixture `relsound.c` 에 열 경우(`recur_*` 상호 재귀로 다시 들어가는 생성자, `memhook_*` memcpy 로 바꾼 전역 함수 포인터, `dlhook_*` cast lvalue 로 바꾼 멤버, `cbq_*` qsort 가 되부르는 함수의 해제, `unres_*` 자식의 미해결 호출이 닿는 주소 잡힌 함수, `unres2_*` 그 호출로만 닿는 읽기 함수, `img_*` union 멤버로 읽는 정적 이미지 entry, `varhook_*` cast lvalue 로 바꾼 전역 변수, `putcopy_*` 복사한 함수 포인터 포인터를 통한 저장, `arrhook_*` 첫 원소 포인터로 바뀐 함수 포인터 배열)와 `static_golden_relsound_np`. `test_static_cli` 가 그 등급과 `incomplete-source` 행, 두 단위가 포함하는 헤더의 `static` mutex(LOCK_IDS `not-static`, `undecidable`), 두 행위자가 도는 probe-create(짝으로 남음), 새 상한 둘의 기록을 본다.

## 5j. 정적 이미지 예산의 잘림 표시와 두 계수 (2026-11-10, 단계 2a 8b) [D][M]

근거: `plan/RV3_REVIEW_ANALYSIS_20261105.md` §2.5 ①②와 `plan/RV4_REVIEW_ANALYSIS_20261109.md` §2 #6(계수를 둘로 나눔). 결과는 `plan/step2a_results_20261030.md` §15. 예산 값은 바꾸지 않았다(기본 4096, `--image-budget N`; 레퍼런스가 정하지 않는 도구 매개변수). facts schema 는 개정 11 그대로다(섹션·레코드 배치·열거형 변화 없음; `docs/schema.md` §19).

**예산** [D]. 정적 이미지(`flatten_initializer`)는 STATIC_IMAGE, GLOBAL_IMAGES, basis 의 table 항목이 쓴다. 객체마다 예산을 따로 받는다. 잎 하나가 예산 하나를 쓴다. 0 채움 행(`[k..n-1]`)은 예산을 쓰지 않는다.

**잘림 표시 잎** [D]. 예산이 떨어져 읽지 않은 부분에 잎이 있었으면 그 자리에 표시 잎 하나를 둔다. 종류는 `non-constant`, 글은 `<image truncated>`, 위치는 객체의 정의다. 경로는 셋이다.
- `경로[...]`: 배열에서 읽지 않은 첫 원소부터 끝까지(0 채움 행 포함). 전에도 있던 표시다.
- `경로.멤버...`: 레코드·union 에서 읽지 않은 첫 멤버부터 끝까지. 새로 더했다. 전에는 이 부분이 표시 없이 빠졌다(RV3 §5.5).
- `경로...`: 그 부분 전체. 예산 없이 걷기에 들어온 경우다(옛 판의 이른 반환 자리). 명령줄 예산은 1 이상이고(`--image-budget 0` 은 거부), 걷기는 배열 원소와 멤버마다 내려가기 전에 예산을 본다. 그래서 지금 경로로는 이 표시가 나오지 않는다 [I: 코드].
- 이름 없는 멤버(C11 의 익명 struct·union)의 이름은 빈 글이다. 그 잎도 `경로..b` 처럼 빈 이름을 쓴다. 그래서 그 멤버부터 읽지 않으면 표시는 `경로.` 에 `...` 를 붙인 `경로....` 다. 셋째 꼴과 글이 같고 뜻도 같다(그 자리부터 읽지 않음). 그 멤버 안에서 잘리면 `경로..c...` 다(8b 수정 단계, 2026-11-10) [M: `test_static_cli`].
- 표시는 잘린 단계마다 하나다. 배열 안 레코드가 잘리고 배열에도 원소가 남으면 둘이다(예: `pts[1].y...` 와 `pts[...]`).
- 읽지 않은 부분에 잎이 없으면 표시를 두지 않는다(예: 초기값 없는 배열 멤버, 정의 없는 레코드). 배열도 같은 규칙이다. 전에는 배열에 원소가 남기만 하면 표시를 두었다. 다른 것은 남은 원소에 잎이 없는 드문 경우뿐이다.
- GLOBAL_IMAGES 의 `image-truncated` 플래그는 기존 경로로 붙는다(`argflow.cpp`: 객체의 잎 가운데 표시 잎이 있으면 그 객체의 모든 행). 이제 레코드·union 의 잘림에도 붙는다.

**정확히 맞음** [D]. 마지막 잎이 예산의 마지막을 쓰고 남은 부분에 잎이 없으면 표시가 없다. 빠진 것이 없다.

**배열의 다른 초기값 꼴** [D] (8b 수정 단계, 2026-11-10; 예산과 무관한 따로 항목). 배열의 초기값이 목록도 문자열도 0 채움(초기값 없음, 암묵 초기값)도 아니면 상수 아닌 잎 하나를 둔다. 레코드의 같은 경우와 같다. 예는 앞 지정자를 덮어쓰는 지정자다(`.s = "abc", .s[1] = 'X'`; clang 의 DesignatedInitUpdateExpr). 전에는 이것을 0 채움으로 읽었다. 그래서 그 배열은 행도 표시도 없이 빠졌다(2a 도구와 8b 도구 모두, 예산과 무관) [M: 검증 단계 fixture]. 이 잎은 예산 하나를 쓴다. `image_has_leaf` 도 같게 본다. 원소별 값은 읽지 않는다.

**두 계수** [D]. `read_global` 이 객체마다 센다. 합은 BASIS `count.walk_bounds_hit` 끝의 두 항목이다(확장 사실을 쓰는 추출; 다른 상한의 수 뒤에 붙는다).
- `image_budget_exhausted`: 걷기가 끝났을 때 예산이 0 인 객체의 수다. 정확히 맞은 객체도 든다. 그래서 이 수만으로는 잎이 빠졌는지 모른다. 보수적으로 "다 읽었는지 모름"으로 읽어도 된다.
- `image_leaves_omitted`: 표시 잎의 수다. 0 이 아니면 실제로 빠진 부분이 있다. 빠진 잎의 수는 아니다(빠진 부분은 걷지 않는다).
- 표시 잎이 있는 객체는 늘 `image_budget_exhausted` 에도 든다(예산은 다시 늘지 않는다).
- 객체별 값은 모델의 `image_budget_objects`(객체 키 → 표시 잎 수)다. `extract --verbose` 는 그 객체마다 stderr 에 한 줄을 쓴다(`kz-static: static image of <키>: budget N used up, marker leaves k`). facts 에는 객체별 계수가 없다. 직렬화되는 객체(GLOBAL_IMAGES)는 표시 잎 행과 플래그로 드러난다.
- 프로필 추출(옵션 없음)의 BASIS 는 다른 걷기 상한처럼 이 수를 적지 않는다(§5g). 표시 잎은 그 모드의 이미지에도 생긴다.
- 프로필 없는 추출은 세 번 읽는다(첫째 읽기, 완전성 pass, 둘째 읽기). 계수와 `--verbose` 줄은 facts 를 만드는 둘째 읽기의 것이다.

**소비자** [D]. 표시 잎과 `image-truncated` 플래그를 읽는 방식은 소비자마다 다르다.
- table walk(`finalize.cpp`): 경로를 정확히 맞추는 조회(`find_entry`)에서 표시 잎은 어느 멤버와도 맞지 않는다. 앞부분 조회(`has_prefix_entries`)는 표시 잎을 빼고 본다. walk 가 풀리지 않고 그 객체의 이미지에 표시 잎이 있으면 이유 문장이 잘림을 적는다. 필드가 잘린 바로 그 자리이면 "was not read", 아니면 "has no initializer or is in the part that was not read", 표가 끝나지 않으면 "the elements after the cut were not read" 다. 표시 잎이 없으면 이유는 전과 같다. walk 가 풀리는지는 바뀌지 않는다(8b 수정 단계, 2026-11-10) [M: `test_static_flow`, `test_static_cli`]. 8b 판에서는 앞부분 조회가 `경로.멤버...` 를 그 멤버의 중첩 항목으로 보았다. 그래서 이유가 "no null element ends the table" 로 틀렸다.
- 표의 원소가 가리키는 객체의 이미지가 잘려도 walk 는 풀릴 수 있다. 그때 펼친 차례는 잘린 필드를 "not in the image" 의 상수 아닌 값으로 읽고 residual 조건을 남긴다 [M: 검증 단계 fixture, `--image-budget 3`]. 잘림 플래그로 UNKNOWN 을 정하는 일반 규칙은 ③ 이다.
- (2026-11-16 §5q 에서 펼치기와 PTR_FIELDS 를 고침.) 펼치기(`unroll.cpp`), PTR_FIELDS 정적 초기값(`argflow.cpp`), basis 의 table 항목은 고치지 않았다. 펼치기의 필드 조회도 경로를 정확히 맞춘다. 그래서 잘린 멤버는 전처럼 "이미지에 없음"이다. argflow 의 정적 초기값 쓰기는 남은 잎에서만 나온다. basis 의 table 항목은 원소의 표시 잎을 그 원소의 글에 넣는다.
- relations(`relations.cpp`)는 표시 잎도 `image-truncated` 플래그도 읽지 않는다. spawn entry 를 표의 정적 이미지로 풀 때 남은 함수 잎만 본다. 상수 아닌 잎이 읽은 멤버를 덮는지 보는 검사(relations 규칙 개정 5)는 경로 모양을 비교한다. 배열 표시 `P[...]` 는 덮는 것으로 본다. 그래서 그 spawn 의 스레드는 완전하지 않은 것으로 남는다. 레코드·union 표시 `P.m...` 는 덮지 않는다. 모양 `[].fn...` 이 멤버의 모양 `[].fn` 과 같지 않고 그 앞부분도 아니기 때문이다. 그래서 잘린 원소의 entry 는 표시 없이 빠진다 [M: 검증 단계 fixture, 원소 셋의 표에서 `--image-budget 5` 면 entry 둘 가운데 하나, path C 후보 1 → 0]. PTR_FIELDS 를 지나는 이미지 값도 필드가 빈 행을 건너뛴다. 그래서 표시 잎은 값에 들지 않는다 [I: 코드]. 2a 도구도 같은 relations 출력을 낸다(표시 잎이 없었다). 회귀는 아니다. 다섯 빌드는 표시 잎이 0 이다(`plan/step2a_results_20261030.md` §15.8).
- 그러므로 relations 의 결과는 이미지가 잘리면 보수 쪽이 아닐 수 있다. 잘린 이미지를 UNKNOWN 으로 두는 소비자 규칙(RV3A §2.5 ③)은 하지 않았다. ③ 의 소비자에 relations(spawn 이미지 entry, PTR_FIELDS 이미지 값)를 더했다(RV3A §2.5, 8b 수정 단계).
- relations 규칙 개정 6(2026-11-16)부터 relations 는 표시 잎과 플래그를 읽는다. 잘린 이미지의 키는 풀리지 않음이고, 잘린 이미지에서 찾은 spawn entry 는 남기되 그 spawn 의 스레드는 완전하지 않음이다(§5m).

**시험** [M]: fixture `imgbudget.c`(사례 이름 없음)와 `test_static_cli` 의 이미지 예산 확인. `--image-budget 4` 로 다섯 경우를 따로(`-DIMG_CASE=1..5`), 그리고 함께 읽는다. 1 멤버가 예산보다 많은 구조체(표시 `.e...`, 소진 1, 표시 1). 2 활성 멤버가 잘리는 union(표시 `.s.e...`). 3 배열 안 구조체(표시 `[1].y...` 와 `[...]`, 소진 1, 표시 2). 4 정확히 맞음(뒤에 잎 없는 배열 멤버; 소진 1, 표시 0, 표시 잎 없음). 5 예산 안(둘 다 0). 기존 `argflow.c` 의 `--image-budget 1` 확인(`fx_slots[...]`)은 그대로다. 8b 수정 단계(2026-11-10): golden `static_golden_imgbudget_np`(`--no-profile --image-budget 4`, 다섯 경우 함께; 섹션 functions·argflow·paramflow·ptrfields·gimages)가 잎과 플래그를 행 단위로 고정한다. 두 계수는 BASIS 에 있다. golden 은 BASIS 를 뺀다(`tests/static/static_golden.cpp` 의 `kFactSections` 앞 주석: 도구 해시가 든다). 그래서 계수는 `test_static_cli` 가 본다. `test_static_cli` 는 C99 fixture 에 넣지 못하는 세 경우도 본다(소스를 시험 안에서 씀): 이름 없는 멤버의 표시(`an_rec....`, `an_rec..c...`), 덮어쓰는 지정자로 초기화한 배열의 상수 아닌 잎(`up_rec.s`), 잘린 이미지 위 table walk 의 이유 문장.

## 5k. 없는 명령줄 대상, 대상 기록 하나, 읽기 옵션의 기록, 죽은 입력 (2026-11-10, 단계 2a 8c) [D][M]

근거: `plan/RV3_REVIEW_ANALYSIS_20261105.md` §6.3 O2·O12. 결과는 `plan/step2a_results_20261030.md` §16. facts schema 는 개정 11 그대로다(섹션·레코드 배치·열거형 변화 없음).

**없는 명령줄 대상(`unknown-target`)** [D]. 명령줄 `--target T.f` 의 선언을 읽은 단위에서 찾는다. 찾는 규칙은 writer 를 맞추는 규칙과 같다(`TuReader::targets_of_field`: 필드 이름이 같고, 그 필드가 든 레코드나 그것을 감싼 이름 없는 레코드의 태그·typedef 이름에 `T` 가 있음). 레코드 정의만 센다. 앞선 선언(불완전 형)은 세지 않는다.
- 찾는 이름은 파일 범위의 태그와 typedef 이름뿐이다(`TuReader::collect_typedefs`). 레코드 안에 둔 태그(`struct Outer { struct Inner { int ix; } in; }` 의 `Inner`)와 블록 안의 태그는 보지 않는다. 그래서 그런 `T` 는 선언이 있어도 `unknown-target` 이다. 상태(unchecked)는 안전한 쪽이다. 그 직접 쓰기는 writer 로 잡지 못한다 [M: 8c 수정 단계, C99 소스 둘]. 중첩 태그를 이름 표에 넣으면 writer 를 맞추는 규칙이 바뀐다. 그래서 따로 선언하고 빌드한다. 남는 결함이다. (2026-11-16 §5q 에서 고침: 레코드 안과 블록 안의 태그도 이름 표에 든다.)
- 같은 규칙으로, 이름 없는 레코드 형의 멤버 안 필드도 감싼 레코드의 필드로 본다. `struct Unn { struct { int q; } m; }` 에서 `--target Unn.q` 는 선언이 있다고 보고, `g_unn.m.q = 5` 를 `Unn.q` 의 writer 로 센다 [M]. `Unn` 에는 멤버 `q` 가 없다(경로는 `m.q`). writer 를 맞추는 규칙의 성질이다. 바꾸지 않았다. (2026-11-16 §5q 에서 바꿈: 그 형은 `Unn::m` 이고 `Unn.q` 는 unknown-target 이다.)
- 어느 단위에도 없으면 FACT_LOCS 상태는 `unchecked` 다. `writer-set-complete` 가 아니다. 수(writers 등)는 0 이다.
- BASIS 에 `unknown_target.T.f` 한 항목을 둔다(정책 unset). 값은 까닭이다: "no record type T is found among the file-scope tags and typedef names of the units read" 또는 "the record type T has no field f in the units read". 8c 도구의 앞 글은 "no record type T is defined in the units read" 였다. 중첩 태그에는 틀린 글이라 8c 수정 단계에서 고쳤다.
- extract 요약 줄은 `-> unknown-target (까닭)` 이다.
- `--target` 이 프로필 대상과 같으면(예: fixture 프로필의 `loc Cell.id` 와 `--target Cell.id`) 대상 기록은 프로필의 출처 하나다(`target.T.f = profile …`). 8c 도구는 그 대상을 검사하지 않았다. 8c 수정 단계부터 명령줄 대상과 같은 검사를 한다(`TargetField::command_line`). 명령줄에서 다시 준 사실은 BASIS 에 따로 적지 않는다. 실행 기록(드라이버의 `args.txt`)에 남는다. 요약의 "hand-entered inputs" 수에도 들지 않는다. 2a 의 identity C1 pre tx V2 는 `--target` 일곱 가운데 셋이 프로필 대상이다(`OS_common_record_t.name_entry`, `OS_common_record_t.active_id`, `OS_count_sem_internal_record_t.obj_name`). 셋 다 선언이 있다 [M: FACT_LOCS writers 18, 6, 3].
- 새 열거형 값은 두지 않았다. `layout.golden` 이 열거형을 적는다. 그래서 새 값은 기존 golden 과 schema 개정을 바꾼다.
- 소비자: `unchecked` 는 complete 가 아니다. 루프 규칙(`unroll.cpp`)과 옛 재생의 정적 닫힘(`lib/trace/static_closure.cpp`)은 complete 만 완전한 것으로 본다.
- 프로필에만 있는 대상(`loc`, `bind`, store pattern)과 규칙 대상(`--targets-rule`)에는 적용하지 않는다. 두 곳 모두 같은 결함이 남는다.
  - 규칙 대상 [M]: 8c 판의 문장 "규칙 대상은 프로그램의 필드에서 고르므로 늘 선언이 있다" 는 틀렸다. 규칙(R-T1, R-T2)은 형 이름을 `field_type_name` 에서 받는다(`lib/llvm/ast_fn.cpp:587-590`). `targets_of_field` 가 아무것도 맞추지 못하면 그 이름은 `record_name(fl->getParent())` 다. 이름 없는 레코드와 중첩 태그 레코드에서는 `<anonymous>` 다. 이름 표의 어느 항목도 `<anonymous>` 가 아니다. 그래서 직접 쓰기는 그런 대상의 writer 로 맞지 않는다. 그래도 상태는 `writer-set-complete` 다(8c 수정 단계 C99 소스: `g_anon.u1 = 3` 과 `g_outer.in.ix = 7` 이 있어도 `<anonymous>.u1`, `<anonymous>.ix` 가 `writer-set-complete writers=0`).
  - 2a 다섯 빌드의 NP 에 그런 대상이 3 / 3 / 2 / 0 / 0 개 있다(c1_pre_tx, c1_post_tx: `<anonymous>.Data`, `.State`, `.sys_timebase_id`; r2_core: `.Data`, `.sys_timebase_id`). c1_pre_tx 의 `<anonymous>.State` 는 `writer-set-complete writers=0`, escape 0 이다. 그 필드는 이름 없는 구조체 `UnSock_Global` 의 멤버다(`psp/fsw/modules/unsock_intf/unsock_intf.c:81-85`). `:231` 의 memset 이 덮고, `:338` 에서 원소 주소를 받아 `:371`·`:409` 에서 쓴다. 이 대상들은 루프 행과 relations.txt 에 없다(grep 0). 그래서 T1–T7 은 바뀌지 않는다 [M].
  - 고치는 길은 둘이다. 규칙 대상에도 이 검사를 하거나, 규칙이 `targets_of_field` 가 주는 이름(감싼 이름 있는 레코드)을 고른다. 어느 쪽이든 C1·R2 NP 의 FACT_LOCS 가 바뀐다. 그래서 따로 선언하고 빌드한다. 이 판에서는 고치지 않았다.
  - 2026-11-16 §5q 에서 고쳤다: 이름 없는 레코드는 쓰임에서 온 제 이름을 갖고, 그 이름이 이름 표에 들어 규칙 대상의 writer 가 맞는다.
  - 프로필 대상 [M]: fixture 프로필의 `loc Cell.id` 는 `Cell` 이 없는 fixture 의 golden 아홉에 나온다(`Cell` 은 `writers.c`·`derives.c` 에만 있다). `[locs]` 행과 요약 줄이 `writer-set-complete writers=0`: atype, loops, outptr, relations, spawn. 요약 줄만(그 golden 은 `[locs]` 를 싣지 않음): argflow, flowfacts, lockset. clobber 는 DISABLED. 8c 판은 "다섯, 그 가운데 넷" 이라 적었다. spawn 과 요약 줄의 셋을 빠뜨렸다. 고치면 옛 경로 golden 아홉 이상이 바뀐다. 그래서 이 판에서는 고치지 않았다.

**명령줄 대상은 한 번 기록한다** [D]. 전에는 `target.T.f = command line` 과 `target.T.f = extraction target` 두 항목이었다. 이제 앞의 것 하나다(정책 declared-assumption, 출처 계층 researcher). 요약의 "hand-entered inputs" 수는 그대로다(명령줄 대상을 따로 센다). relations 의 "hand-entered vocabulary" 수와 행마다의 layers 칸(`hand-entered:command-line(N items; …)`)은 대상을 한 번 센다. 전에는 두 번 셌다. 2a 출력에서는 보이지 않는다. relations 는 명령줄 대상이 있는 facts(identity)에서 돌지 않는다. 이 변화는 8c 의 빌드 전 선언에 없었다(`plan/step2a_results_20261030.md` §16.10).

**읽기 옵션의 기록** [D]. facts BASIS 에 두 가지를 둔다(정책 unset).
- `extract.resource_dir`: 도구가 단위마다 더하는 `-resource-dir=` 의 값이다(`lib/llvm/ast_run.cpp` adaptations). `--resource-dir` 가 없으면 도구에 내장된 기본값이다. 값은 그 경로와 "(the tool's built-in default; no --resource-dir)" 또는 "(--resource-dir)" 다. 둘 다 없으면 "none (…)" 이다. `--resource-dir` 의 상대 경로는 준 그대로 적는다(풀지 않는다). `--resource-dir ""` 이면 resource 디렉터리 없이 읽고 값은 "none (…)" 이다. 그때 시스템 헤더를 쓰는 단위는 읽기가 실패해 facts 가 나오지 않는다 [M: 8c 수정 단계, `writers.c` 는 읽힘, `<stddef.h>` 를 쓰는 소스는 실패].
- `--extra-arg -resource-dir=X`(또는 `-resource-dir` 와 `X` 두 인자)는 도구의 것보다 뒤에 붙는다. 프런트엔드는 마지막 것을 쓴다 [M: 8c 수정 단계, 가짜 resource 디렉터리의 `stddef.h` 매크로가 읽힘]. 8c 도구는 이때도 내장 기본값을 적었다. 8c 수정 단계부터 값은 `X (--extra-arg -resource-dir, after and over the tool's: <도구의 값>)` 이다. basis 의 단위 분석 인자(`front_end.unit_diagnostic`, 더 뒤에 붙음)는 보지 않는다.
- `extract.extra_arg[i]`: `--extra-arg` 마다 하나, 준 차례대로. 없으면 `extract.extra_arg = none` 하나다.
- (2026-11-16 §5q 부터 extract 가 대조한다: 다르면 거부, `--accept-option-difference` 이면 편차로 적음.) basis 는 basis 를 만들 때의 값을 `front_end.arguments_added` 에 적는다. extract 는 제 읽기 옵션을 그 값과 대조하지 않는다(`lib/llvm/basis.cpp` 의 단위 읽기는 `front_end.unit_diagnostic` 인자만 가져온다). 그래서 두 값은 두 명령에 같은 옵션을 줄 때만 같다. 2a 의 실행은 그렇다(`tests_8c` 의 facts.txt 11개에서 같음) [M]. 이제 extract 의 값도 facts 에서 되짚을 수 있다(CONFIG_MANAGEMENT §1).

**죽은 입력** [D].
- `OpInfo.out`(프로필 op 매개변수의 `out` 역할)은 지웠다. 채우기만 하고 읽는 곳이 없었다.
- `poll_fns`(프로필 observe 규칙의 poll 함수, `--poll-fn`)는 옛 경로 전용이고 추출이 소비하지 않는다. 남겨 둔다. `--poll-fn` 은 BASIS 에 사람이 넣은 항목(`op.poll function.<이름>`)으로 기록될 뿐이다. 프로필 observe 규칙의 poll 함수는 프로필 해시에만 든다.

**시험** [M]: `test_static_cli` 의 8c 블록. fixture `writers.c` 에 없는 대상 셋(`NoSuch_t.nofield`, `Slot.nofield`, `Cell.nofield`)이 `unknown-target` 이고, 프로필 대상 `Cell.id` 는 전과 같다. BASIS 의 `unknown_target.*`, 대상 기록 하나, `extract.resource_dir`, `extract.extra_arg`(없음과 둘), relations 의 수 1 을 본다. 8c 블록의 `--target Cell.id` 는 프로필 대상을 다시 준 경우다. 8c 수정 단계 블록: 프로필에 없는 선언된 명령줄 대상(`Cell.aux`, writer-set-complete), `Cell` 이 없는 소스에서 `--target Cell.id`(unknown-target, 기록은 프로필 출처), 중첩 태그 `Inner.ix`(unknown-target, 남는 결함), `--target` 없는 `Cell.id`(프로필 규칙 그대로, 남는 결함), `extract.resource_dir` 의 네 꼴(내장 기본값, `--resource-dir`, `--extra-arg -resource-dir=`, `--resource-dir ""` 의 none).

## 5l. relations 행의 기록, 사용 지점 키, 공유 적재기 (2026-11-16) [D][M]

결과는 `plan/observation_path_results_20261116.md`. 규칙은 relations rules revision 5 그대로다. 개정 문자열, 행의 차례와 번호, 텍스트의 바이트를 바꾸지 않는다. facts schema 도 개정 11 그대로다.

**행은 기록이다** [D]. `kz-static relations` 는 행을 먼저 기록(`RelRow`, `lib/llvm/relations.hpp`)으로 만들고 텍스트를 그 기록에서 그린다(`render_relations`). 그래서 행을 읽는 도구는 텍스트가 말하는 것을 그대로 읽는다.
- 칸: 경로(`A`/`B`/`C`), 번호, 첫 줄, 첫 줄과 칸 사이의 줄(줄마다 이름·글·들여쓰기), 칸 여섯(`dependency`, `run-order`, `feasibility`, `violation`, `flip`, `layers`; 이름표 없는 값), 요약 줄이 센 등급(`feasibility_grade`, `flip_grade`).
- 사실 참조(섹션의 행 번호; 없으면 `KZ_NONE32`): A 는 lookup 호출(FACTS), 그 함수, 키 매개변수 함수(FUNCS)와 매개변수. B 는 callee 와 인자 위치, 사이트의 호출들. C 는 spawn 호출, 생성자, spawn 호출의 callee(본문 없는 P0 기본 연산이면 없음), 읽기(READS)와 읽는 함수, 맥락의 종류와 호출.
- 행이 아닌 줄(경로의 머리 줄, 경로 A 의 이름 공간 블록)은 묶음(`RelGroup`)의 글로 둔다. 머리(`RelReport::header`)는 앞과 같은 글이다.

**사용 지점 키** [D]. 행의 사용이 프로그램의 어디에 있는지를 이름으로 적은 글(key text)과 그 sha256 의 앞 16 hex 자리(key)다(`relations.cpp` 의 `key_a`, `key_b`, `key_c`).
- 넣는 것: 경로, 이름 공간 또는 필드, 호출하는 함수, callee 와 키 매개변수 위치, 그 callee 의 n 번째 호출.
- 넣지 않는 것: 줄·열 번호, creator, writer, 구획(partition), 계수, 판정. 이것들은 행의 내용이다.
- 함수는 이름이다. 내부 연결(static) 함수이면 정의된 파일을 `@` 뒤에 붙인다(줄은 붙이지 않는다). 본문 없는 함수와 간접 호출은 호출이 적은 이름이다.
- n 번째: 호출하는 함수 안에서 같은 callee 를 부르는 호출들을 소스 차례(파일, 줄, 열; 한 자리의 두 호출은 사이트 번호)로 세운 차례(1부터). 그래서 한 줄의 두 호출은 키가 다르다.
- A: `A|<이름 공간>|<lookup 을 부르는 함수>|<키 매개변수 함수>#<매개변수>|<n>`.
- B: 행은 (필드, callee, 인자 위치) 하나다. 사이트는 행의 내용이다. 키는 `B|<필드>|-|<callee>#<인자>|-` 이다. 사이트마다의 키 `B-site|<필드>|<부르는 함수>|<callee>#<인자>|<n>` 를 목록에 따로 둔다.
- C: `C|<필드>|<생성자>|<spawn 호출의 callee>|<n>|read <읽는 함수> <그 함수 안 같은 필드 읽기의 n>|<맥락>`. 맥락은 `itself`, `entry`, `unresolved`, 또는 읽는 함수의 호출(`call`, `indirect-call`, `callback`)과 그 호출의 부르는 함수, callee, n 이다.
- 키는 텍스트의 바이트가 아니다. 키를 만드는 동안 파일을 적지 않는다(행의 `layers` 칸이 기대는 파일 기록을 바꾸지 않는다).
- 한계 [I]: 같은 함수 안에서 같은 callee 의 호출이 앞에 더해지거나 빠지면 뒤의 n 이 바뀐다. 함수·필드 이름이 바뀌면 키가 바뀐다. 열을 적지 않는 사실(열 0)에서는 한 줄의 두 호출을 사이트 번호로 가른다. 사이트 번호는 추출의 차례다.

**`kz-static relations <facts> [-o OUT] [--keys KEYS]`** [D]. `--keys` 는 사용 지점 키의 목록을 KEYS 에 쓴다. 텍스트(표준 출력 또는 `-o`)의 바이트는 `--keys` 가 있든 없든 같다. 목록: `kz-relation-keys 1`, facts 해시, 규칙 개정, 키의 정의 한 줄, 수 한 줄(`rows`, `distinct keys`, `rows that share a key`, `keys shared by different key texts`), 키를 나누는 행마다 `collision <key> (one key text | different key texts): <행들>`, 그다음 행마다 `<행> <key> <key text>`, B 행의 사이트마다 `<행>.site <key> <key text>`.

**공유 적재기** [D]. facts 의 섹션 읽기, 색인, 정적 행위자(root, 그 코드, 트램펄린)를 `RelFacts`(`lib/llvm/rel_facts.hpp`, `rel_facts.cpp`)로 옮겼다. relations.cpp 에 있던 코드 그대로다(`load`, `index`, `actors` 와 그것들이 부르는 함수). relations 의 행은 그것 위에 선다. 같은 facts 를 같은 방식으로 읽는 다른 도구도 그것을 쓴다. 위치와 함수 이름을 적을 때 그 파일을 행의 기록에 적는 일은 `file_notes_` 가 가리키는 집합으로 한다(relations 만 설정한다).

**시험** [M]: `test_static_cli` 의 사용 지점 키 블록. 표를 찾는 lookup API(상수 구획 인자가 있음)를 한 줄에서 두 번 부르고 다른 스레드가 만든다. 줄이 밀리거나, creator 가 늘거나, 구획 상수가 바뀌어도 A 행 키는 같다. 그 사이 텍스트는 바뀐다. 한 줄의 두 호출은 키가 다르다(n 번째). `--keys` 가 텍스트를 바꾸지 않는다. fixture `relations.c` 의 다섯 행은 키가 서로 다르고, B 행 사이트의 키가 목록에 있다. golden 은 바뀌지 않는다.

## 5m. relations 규칙 개정 6: 잘린 정적 이미지, 행마다의 미완 사유 (2026-11-16) [D][M]

결과는 `plan/observation_path_results_20261116.md` §3. 개정 문자열은 "relations rules revision 6 (2026-11-16)" 이다. facts schema 는 개정 11 그대로다(`include/` 를 고치지 않음). 행의 차례·번호·사용 지점 키(§5l)는 바뀌지 않는다.

**잘린 이미지(표시 잎) 읽기** [D]. 객체의 GLOBAL_IMAGES 행에 표시 잎(`non-constant`, 글 `<image truncated>`)이 있거나 `image-truncated` 플래그가 있으면 그 객체의 이미지는 잘렸다(`RelFacts::image_cut`; 글은 표시 잎의 경로들). 객체 단위로 본다. 읽는 원소나 멤버가 표시 잎이 덮는 부분인지는 가리지 않는다(보수 쪽).
- 경로 A 의 키(`key_value`): PTR_FIELDS 로 닿은 객체의 이미지가 잘렸으면 키는 `unresolved` 다. 찾은 값이 있어도 그렇다(읽은 부분의 값이 이미지의 모든 값이라는 것을 모른다). 키 줄은 `; unresolved (image, not taken as the run-time value: the static image was cut at the image budget (marker leaves <경로들>): the part not read is not known)` 로 끝난다. 그래서 "key constants differ" 제외와 static-possible flip 이 그 키에 서지 않는다(둘 다 두 키가 known 일 때만 선다). 같은 글은 `image_value_why` 의 첫 이유다(`cut` 인자).
- spawn 의 정적 이미지 entry(`image_entries`): 잘린 이미지에서 찾은 함수 잎은 entry 로 남긴다(읽은 잎의 값은 틀리지 않다). 그러나 읽지 않은 부분의 entry 는 찾지 못한다. 그래서 그 spawn 은 `image_cut_` 에 들고, 머리의 "spawn calls resolved through the static image" 줄이 그 까닭을 적고, 그 spawn 의 스레드 집합(ChildSet)은 완전하지 않음이다. 그 스레드가 닿지 않는 맥락의 C 행은 "not graded" 가 아니라 "undecidable" 이다. 찾은 잎이 없으면 전처럼 풀리지 않은 spawn 이고, 이유가 잘림을 적는다.
- 표시 잎은 값이 아니다. relations 규칙 개정 5 의 "읽은 멤버를 덮는 상수 아닌 잎" 수에서 뺀다(전에는 배열 표시 `P[...]` 를 덮는 잎으로 셌다, §5j). 잘림은 위의 줄이 따로 적는다.

**행마다의 `incomplete:` 줄** [D]. A·B·C 의 모든 행은 `layers:` 다음에 한 줄 `incomplete: <사유>; <사유>...` 로 끝난다. 사유는 판정 규칙(`plan/ovm_rev1_20261116/JUDGEMENT_RULES_REV1.md` 3.5)의 어휘이고 `사유(하위 사유)` 꼴이다. 쓸 수 있는 사유는 고정 표 `relations_incomplete_reasons()`(`lib/llvm/relations.cpp`) 하나이고, 줄의 차례는 그 표의 차례다. 표의 셋째 칸(`when`)이 규칙이다. 요약:
- 모든 행: `no_run(no run record read)`(이 개정은 실행 기록을 읽지 않는다).
- facts 전체의 사유(모든 행): facts 에 entry 가 풀리지 않은 spawn 호출이 있거나 트램펄린이 대신하는 파생 spawn 연산의 호출 가운데 entry 가 풀리지 않은 것이 있으면 `actor_not_extracted(spawn call with no resolved entry)`. 잘린 이미지에서 entry 를 읽은 spawn 이 있으면 `actor_not_extracted(spawn entries from a cut static image)`. BASIS `count.walk_bounds_hit` 의 다섯 걷기 상한(FACT_INIT walk, 함수 값 초기값 깊이, 식 깊이, DERIVES 복사 고리, LOOP_ITERS 글) 가운데 0 이 아닌 것마다 `truncated(<상한> bound)`. 이미지 예산의 두 계수는 여기서 쓰지 않는다(잘린 이미지는 객체마다 읽는다).
- 행이 이름 대는 위치의 단위에 basis 편차가 있으면(`layers` 의 `basis-deviation(...)`) `reading_options_differ(basis deviation of a unit of the row)`.
- 행의 등급이 per-field 완전성 규칙에 서면(`rests-on`) `writer_set_incomplete(per-field completeness rule is a premise)`. proven-impossible 이 선언된 가정(`assume.*`)이나 path C 전제에 서면 `fixed_proof_open(proof rests on an assumption)`.
- A: 키가 풀리지 않으면 `key_unresolved(<까닭>)`. 까닭은 여덟: ARGFLOW 행 없음, points-to 없는 포인터 필드, 잘린 이미지, 실행 중 쓰이는 이미지, 실행 값이 아닌 이미지(FIELD_STATUS 없음·완전하지 않음·writer 있음), 필드의 이미지 잎 없음, 이미지에 닿지 않는 필드 읽기, 리터럴·상수·필드가 아닌 인자. 짝지은 creator 의 키가 풀리지 않으면 `key_unresolved(creator key)`. lookup 이나 짝지은 creator 의 키 이미지가 잘렸으면 `truncated(static image)`. lookup 에 닿는 root 가 없으면 `actor_not_extracted(no root reaches the use)`. 키 필드와 활성 필드마다 FIELD_STATUS 가 없거나 완전하지 않으면 `writer_set_incomplete(...)`. 이름 공간의 검색에 상수 반환(RET_CONSTS)이 없으면 `absent_result_unevaluated(no failure return of the search)`. 짝이 있으면 `fixed_proof_open(no static order between the actors)`.
- B: 읽는 쪽이나 쓰는 쪽의 행위자가 없으면 `actor_not_extracted(no root reaches the use)`. 필드의 FIELD_STATUS. flip 이 static-undetermined 면 `absent_result_unevaluated(callee check not evaluated)`. feasibility 는 늘 undecidable 이고 그 까닭에 따라 `fixed_proof_open` 의 셋(생성 차례만 있음, 한 정적 이미지에서 온 행위자, 정적 차례 없음).
- C: `link_unlinked(identity of the reader and the created thread not decided)`(dependency 칸이 그렇게 말한다). 읽기의 첨자가 읽는 쪽 자신의 식별이면 `link_unlinked(self-reported identity)`. 스레드 집합이 완전하지 않으면 `actor_not_extracted(child threads not all resolved)`, 그 집합에 잘린 이미지의 entry 가 들면 `truncated(static image)`. 필드의 FIELD_STATUS. flip 이 static-undetermined 면 `absent_result_unevaluated(reader's returns not evaluated)`. feasibility 가 undecidable 이면 그 갈래에 따라 `fixed_proof_open` 의 셋(잠금 집합·경로가 정하지 않음, 대상이 풀리지 않은 간접 호출, 스레드 집합이 완전하지 않음). possible, proven-impossible(가정 없음), not graded 는 이 사유를 내지 않는다.
- 머리의 요약 줄 다음에 한 줄: `summary incomplete (...)`: 사유마다 그 사유를 가진 행의 수, 그다음 하위 사유마다의 수, 끝에 facts 전체의 사유.
- 하지 않는 것: 실패 처리의 효과(`handling_effect_unknown`)와 호출자 의존(`caller_dependence_unknown`)은 행 글(handling 줄)에서 판정하지 않는다. 그것은 계획 생성기의 몫이다. 어휘의 다른 사유(`target_not_reached`, `record_insufficient`, `plan_*`, `budget_not_run`, `no_material`, `intervention_out_of_scope`, `build_mismatch` 등)는 실행·계획·개입의 사유라서 relations 가 내지 않는다.

**어휘 자료와 시험** [M]. `tests/static/data/reason_vocabulary.txt` 는 JR 3.5 의 사유 표를 한 줄에 하나씩 옮긴 것이다(2026-11-16 에 그 표에 더한 셋 포함). `test_static_reasons` 는 도구의 표의 모든 사유가 그 어휘에 있는지, 하위 사유와 규칙이 비지 않았는지, 같은 사유가 두 번 없는지, relations golden 의 모든 행이 `incomplete:` 줄 하나를 가지고 그 줄의 항목이 모두 도구의 표에 있는지 본다. 도구는 표의 번호로만 사유를 쓴다. 그래서 표가 도구가 쓸 수 있는 사유의 전부다. fixture `relincomplete.c`(사례 이름 없음): 이름 표를 찾는 lookup 셋(풀린 포인터 필드로 닿는 네 이름의 정적 표, 본문 없는 함수에서 온 포인터, 다른 행위자가 만드는 리터럴)과, union 멤버로 읽는 네 원소의 entry 표에서 시작하는 spawn. golden `static_golden_relincomplete_np`(기본 예산)와 `static_golden_relincomplete_cut_np`(`--image-budget 3`: 두 표가 잘림). `test_static_cli` 가 두 예산의 행과 줄을 본다.

## 5n. 관측 계획 생성 `kz-static plan` (plan rules revision 1, 2026-11-16) [D]

결과는 `plan/observation_path_results_20261116.md` §4. 규칙·입력·출력 형식은 `docs/plan_manifest.md` 다. relations 의 행을 같은 프로세스 안에서 `relations_rows`(§5l 의 공유 적재기 `RelFacts`)로 계산하고, 방법의 관측 집합(모든 행)을 방법의 우선순위 차례로 기록된 예산 아래 계획한다. 우선순위는 방법 문서 첫 단계 규칙의 이식이다(행 글의 표시, 순서 등급, 도달 표시, 처분). 관측 규칙은 use-call, key-text, creator-call, handling-chain, writer, spawn-call, actor, stop-event 다. 출력은 `kzs-sites 3` manifest 와 sidecar, module 표, 점검할 실행 파일 목록이다. 생성기는 명령줄에 이름이 있는 파일(facts, 그 basis, 프레임워크 계층 자료 `semantics/framework_api_roles.txt`)만 연다. facts schema(`include/`), relations 의 규칙과 글은 바꾸지 않는다.

## 5o. relations 규칙 개정 7: 이름 공간의 소속, 표 선택 상수, writer 의 전제, out 포인터, 생성자 callee 의 상수, 생성자 없음의 까닭 (2026-11-16) [D]

결과는 `plan/full_stage_results_20261116.md` §2. 개정 문자열은 "relations rules revision 7 (2026-11-16)" 이다. facts schema 는 개정 11 그대로다(`include/` 를 고치지 않음). 사용 지점 키(§5l)는 생성자·partition·계수를 담지 않으므로 남는 행의 키는 바뀌지 않는다. 행이 빠지면 뒤 행의 번호(A1, A2, …)는 당겨진다.

**이름 공간의 소속: 실제로 넘긴 callback** [D]. 경로 A 의 A2 검색(검색 루프가 간접 호출로 비교 함수를 부름)은 그 간접 호출의 callee 를 나르는 검색 함수의 매개변수를 적는다: CALL_TARGETS 의 그 호출·그 비교 함수 행의 `via` 가 `P:<검색 함수>#<j>`(흐름 그래프의 매개변수 노드, `ast_fn.cpp`)이면 j. 다른 노드(필드, 전역, 이름 없는 지역)이면 매개변수가 나르지 않는다고 적는다. 사용(이름 공간의 키 매개변수 함수를 바깥에서 부르는 호출)마다, 검색에서 키를 나르는 호출 사슬(PARAM_FLOW 깊이 0 의 call-argument 행, 키 매개변수 함수 사이)을 거슬러 올라가며 그 매개변수가 받는 인자를 본다(ARGFLOW): 함수(`function`)면 그 함수, 매개변수면 한 단계 위로, 그 밖이면 풀리지 않음. 사용 자신의 인자에서 멈춘다(그 위는 따라가지 않음).
- 소속(member): 어느 사슬이 그 이름 공간의 비교 함수를 넘기거나, 비교가 자기 검색 루프(A1) 안에 있는 함수에 직접 호출로 닿는다. 행에 `membership:` 줄이 그 까닭을 적는다.
- 다른 callback 만(other): 풀린 사슬이 모두 다른 함수를 넘긴다. 그 사용은 이 이름 공간의 행도 생성자도 아니다. 이름 공간 블록의 `uses:` 줄 끝과 `left out:` 줄(사용마다 하나, 넘긴 함수와 그 자리), 머리의 `summary A membership and creators` 줄이 센다.
- 풀리지 않음(unresolved): 행으로 남고 `membership: unresolved: <까닭>` 과 `link_unlinked(namespace membership unresolved)`. 짝지은 생성자의 소속이 풀리지 않아도 같은 사유를 단다.

**partition: 표 선택 상수만** [D]. 사슬을 따라 내려가며 상수를 넘기는 첫 호출의 상수 가운데, 그 callee 의 매개변수가 검색의 표 선택에 닿는 것만 partition 에 든다: 그 매개변수의 쓰임(PARAM_FLOW, 깊이 무관)이 검색 함수 안의 첨자이거나 검색 루프 출구의 비교이거나, 그것을 필드에 저장하고 검색 함수가 그 필드를 첨자로, 루프 출구에서, 또는 루프 출구가 읽는 지역으로 읽는다. 표 선택이 아닌 상수(잠금 방식, 플래그)는 `partition:` 줄의 `not a table selector:` 에 적고, 그 호출에 표 선택 상수가 없으면 더 아래 호출로 간다. 선택 상수마다 `(selects the table: <근거>)` 를 붙인다.

**writer 의 전제** [D]. 행이 쓰는 writer 집합마다 한 줄 `writers: <Type.field>: ...`: A 는 키 필드, 활성 필드, 그리고 lookup 이나 행이 이름 댄 생성자의 키를 읽은 정적 이미지의 필드; B 와 C 는 그 필드. 줄은 writer 수와 종류, 출처(WRITERS / LOCKSET / FUNC_WRITES), covers-more·early-init·init-tree·partial 의 수, FIELD_STATUS 의 두 규칙 상태와 그 수(writer, 인스턴스, 필드의 hit, 같은 타입 다른 필드의 hit, 프로그램의 untyped 쓰기 자리)와 플래그, 덮지 않는 것(대상이 풀리지 않은 호출의 수, closure 밖 단위의 쓰기: 읽지 않음)을 적는다. writer 가 없으면 "no writer found under these premises" 이고, writer 0 은 완전성 판정이 아니므로 행이 `writer_set_incomplete(zero writers under the rules)` 를 단다. `image_value_why` 의 결정(정적 이미지를 실행 값으로 받는지)은 바뀌지 않는다. 그 전제가 이 줄로 드러난다. const 한정은 facts 에 없어(필드 타입 표는 뒤의 schema 개정) 쓰지 않는다.

**out 포인터와 별칭** [D]. lookup 과 행이 이름 댄 생성자(짝, probe-create)마다, 키가 아닌 매개변수 가운데 callee 가 그것을 통해 저장하는 것(PARAM_FLOW out-store, 가장 얕은 행)을 `id holder:` 줄로 적는다: 넘긴 인자의 글·출처·필드·base·객체(ARGFLOW)와 저장 자리. 객체의 id 를 주는 callee 라면 id 는 그 가운데 있다(어느 것이 id 인지는 가리지 않음). 키가 풀린 포인터 필드를 거쳐 읽히면 `aliases:` 줄이 그 필드와 대상을 적는다(PTR_FIELDS).

**생성자 callee 의 상수** [D]. 행이 이름 댄 생성자의 callee 마다 `callee-check: <callee> returns <v> [<글>] [through <사슬>] when <조건>` 줄: callee 의 RET_CONSTS, 그리고 callee 가 호출 결과를 돌려주는 함수(RETURNS call-result)의 RET_CONSTS 를 따라 내려간 것(조건은 가장 안쪽 함수의 것). 매개변수·지역·필드·식을 돌려주는 반환과 본문 없는 함수의 결과는 따라가지 않고 `callee-check (returns not followed):` 줄에 적는다. 이 집합은 돌려줄 수 있는 상수이고, 무엇을 돌려주는지의 증명이 아니다. 생성자의 out 은 그 가운데 실패 등급이 아닌 상수 하나 아래에서만 읽는다(설명 기록의 규칙).

**생성자가 없는 까닭** [D]. 짝지은 생성자가 없는 lookup 행의 dependency 칸은 까닭을 붙인다(`; ` 로 나눔): 다른 partition 의 생성자만(`creators of other partitions only`), 키를 쓰지만 키 매개변수 함수가 그 writer 에 닿지 않음(`key written without a key parameter`, writer 의 종류와 함수), 키나 활성 필드의 writer 집합이 완전하지 않음(`writer set not complete`), 대상이 풀리지 않은 간접 호출이 키를 쓰는 함수를 돌릴 수 있음(`creators may be in code not read`; 행에 `writer_set_incomplete(key writes reachable through unresolved indirect calls)`), 키의 writer 가 없음(`only initial values`), lookup 의 소속이 풀리지 않음(`namespace membership unresolved`). 머리의 `summary A membership and creators` 줄이 까닭마다 행을 센다.

**시험** [M]. fixture `relcallback.c`(사례 이름 없음): callback 매개변수로 비교를 받는 검색 루프 하나, 두 필드의 비교 함수 둘(strcmp, strncmp), 이름·경로 lookup API, 이름을 쓰는 create API(그 상수를 helper 의 결과로 돌려줌), 본문 없는 함수에서 비교를 받는 검색, 토큰에 저장되어 첨자로 읽히는 종류 상수와 그 옆의 잠금 방식. golden `static_golden_relcallback_np`, `test_static_cli` 의 한 블록. `test_static_cli` 의 사용 지점 키 블록은 종류 인자가 표를 고르게 고쳤다(전에는 쓰지 않는 인자였고, 이 개정에서는 표 선택이 아니어서 partition 에 들지 않는다).

## 5p. 순서 간선 `kz-static order` (order rules revision 1, 2026-11-16) [D]

(2026-11-16 검토 뒤: order rules revision 2 의 두 고침, 호출자에 기대는 spawn 호출과 child without success 의 위치 근거는 §5r.)

결과는 `plan/full_stage_results_20261116.md` §3. 새 명령 `kz-static order <facts> [-o OUT] [--edges TSV]`(`lib/llvm/order.cpp`, "order rules revision 1 (2026-11-16)")이고 기존 출력은 바꾸지 않는다: facts schema(개정 11), 추출, relations 의 규칙과 글(개정 7), 계획 생성기는 그대로다. relations 의 행을 같은 프로세스 안에서 `relations_rows` 로 계산하고(§5l 의 `RelFacts` 를 다시 적재), 행마다 순서를 낸다. 필드·callee·사례 이름을 받지 않는다. 같은 컨테이너는 같은 바이트를 낸다.

**간선과 종류** [D]. 간선마다 id(규칙·양 끝·양 끝 행위자 글의 sha256 앞 16자리), 규칙, 종류, from/to(정적 위치와 행위자), 조건, 전제, 범위, 인스턴스 키를 적는다. 종류: `hard`(두 인스턴스가 모두 일어나는 모든 실행에서 이 차례다, 전제 아래), `conditional`(조건이 설 때만), `observed-only`(이 개정의 어떤 규칙도 차례를 정하지 않는다; 실행 기록이 차례를 보일 수 있다).
- **HB-spawn**: spawn 호출(해결된 entry 가 있는 호출: CALL_TARGETS spawn-entry, table walk 반복, 시작 스크립트 entry, 정적 이미지 entry)에서 그것이 시작하는 각 스레드(트램펄린은 그것이 대신하는 root)의 시작으로. hard, 전제 P0 스레드 생성 의미(POSIX.1-2017 `pthread_create`, XBD 4.12 메모리 동기화)와 원문 대조가 안 된 P0 항목(`P0-unverified`). 덧붙임: 생성 기본 연산까지의 사슬, 상태 조건(spawn 연산이 돌려줄 수 있는 상수 가운데 실패 등급이 아닌 것; RET_CONSTS 와 호출 결과 반환을 따라감; P0 기본 연산은 0), **child without success**: spawn 연산(과 그 아래 연산)의 반환 가운데 안쪽 spawn 호출보다 소스 위치가 뒤이거나 같은 루프에 있고, 값이 실패 등급 상수이거나 실패 등급을 돌려줄 수 있는 함수의 결과이거나 따라가지 못한 값이며, 그 조건이 안쪽 호출의 성공 값(하나일 때) 아래 거짓으로 정해지지 않는 것이 있으면 `possible`(그 반환들), 따라가지 못한 것만 있으면 `unknown`, 없으면 `not found`. 안쪽 spawn 호출은 spawn-op 표시가 있는 호출과 그 함수의 SPAWN_WRAP 행(어휘가 아닌 고정 entry 연산 포함)의 spawn 호출이다. 안쪽 연산 자신의 표시가 `not found` 가 아니면(실패를 돌려줄 때 스레드가 있을 수 있음) 안쪽 실패 아래의 반환을 빼지 않는다: 그 실패를 제 상수로 바꿔 돌려주는 반환도 child without success 다(괄호에 안쪽 호출을 적음). 스레드 생성 기본 연산의 결과를 돌려주는 연산의 상태 조건은 그 기본 연산의 성공 값 0 을 상수로 든다. 위치를 "뒤"로 읽으므로 다른 갈래의 반환도 센다(과대 근사: `possible` 은 일어난다는 말이 아니다). P0 기본 연산은 실패하면 스레드를 만들지 않는다(P0)
- **HB-seq**: 한 스레드의 프로그램 순서를 이 facts 가 주는 꼴로만: (a) 사건이 같은 함수의 spawn 호출 S(spawn-op 표시) 앞: LOCK_PATHS after-spawn 에 그 사건의 S 행이 없다(되돌아가는 간선을 포함해 S 뒤에서 닿지 않음; 함수에 CFG 가 없거나 닿지 않는 사건은 쓰지 않음), 범위 그 함수의 한 호출; (b) 사건은 그 함수를 부르는 호출 안에 있다(같은 스레드의 코드에 있는 호출자); (c) 스레드의 시작은 그 코드의 모든 사건 앞이다; (d) table walk 의 spawn 호출은 반복 차례로. (a)–(c) 는 닫힘이 따라간 걸음이고, 행의 경로가 쓴 것만 `[edges]` 에 적는다(머리가 따라간 걸음 수를 센다)
- **L0-script**: 시작 스크립트 진입(SPAWN_WRAP script-entry 의 `via` = `startup.entry[k]`)의 spawn 호출을 줄 차례로. hard, 전제 L0(로더가 한 스레드에서 줄 차례로 읽고 한 줄의 spawn 호출이 다음 줄 전에 돌아온다; relations 행의 L0 계층과 같고 로더의 코드는 이 개정에서 읽지 않는다)
- **observed-only** 의 시작 차례: 한 생성자가 줄 차례(또는 반복 차례)로 만든 두 스레드의 시작. 어느 스레드가 제 코드를 먼저 도는지는 정하지 않는다
- **HB-lock**: relations 경로 C 행이 다른 차례를 `proven-impossible` 로 매긴 행에서, spawn 뒤 쓰기에서 자식의 읽기로. hard, 전제는 그 잠금(spawn 과 읽기에서 함께 잡힌 정적 인스턴스)과 행의 `rests-on(...)`, 조건은 그 쓰기가 실행됨(그 호출의 spawn 뒤, WRITERS 의 guard 가 있으면 그것)과 spawn 연산의 상태 조건과 child without success
- **HB-await**: 대기(poll 또는 count-bounded 루프)의 인식된 술어(FACT_LOOPS `pred_root`)가 읽는 필드를 다른 스레드에서 쓰는 쓰기(LOCKSET 쓰기 행)에서, 대기한 스레드가 대기 뒤에 닿는 사용(아래 "의존하는 사용"의 읽기와 호출)으로. hard 는 다음이 모두 설 때만: 그 사용의 예산 탈출 결과가 `skip` 또는 `no-budget`, AWAIT_L1 의 필드 단위 판정이 await, 기다리는 필드의 writer 집합이 필드 단위 규칙으로 완전, 다른 스레드의 쓰기가 하나, 조건 탈출이 초기값(INIT)에서 거짓. 그 밖은 conditional(조건: 대기가 조건 탈출로 나왔고 그 탈출이 이 쓰기를 읽었다). 조건 탈출이 초기값에서 참이면 간선을 내지 않는다
- **닫힘**: hard_before 와 fixed_early_proved 는 hard 간선만 따라간다. conditional 간선이 끼면 그 경로는 conditional 이고 hard 닫힘에 들지 않는다. observed-only 간선은 따라가지 않는다

**대기** [D]. poll 과 count-bounded 루프마다 한 기록: 예산(FACT_LOOPS budget 의 지역; count-bounded 에서 budget 이 없으면 탈출이 trip 상수와 비교하는 지역), 지연 호출과 그 인자, 탈출(LOOP_EXITS, 결정 조건을 선언항으로 나눔: `or` 와 부정된 `and`; 예산 지역과 상수만 읽는 항은 예산 부분, 나머지는 조건 부분), 술어(인식된 꼴 또는 FACT_LOOPS 의 까닭)와 AWAIT_L1, 기다리는 필드와 다른 스레드의 그 writer, 필드 단위 완전성, 초기값에서의 조건 탈출. **의존하는 사용**: 그 함수에서 루프 뒤(같은 파일의 소스 위치가 루프 머리 뒤이고 루프 안이 아님)의 READS 와 호출, 그리고 그 함수의 호출(직접·간접). 결과: `inside`(대기 안: 설계된 대기, 간선 없음), `skip`(예산 탈출이 모두 return), `continue`(예산 탈출이 루프 뒤로 이어짐; 읽기의 guard 가 있으면 함께 적음), `no-budget`(예산 탈출 없음), `unknown(...)`(goto 인 예산 탈출; 호출자가 결과를 시험하거나 쓴다: 호출의 조건은 이 facts 에 없다). 예산 탈출 뒤 종료 호출이 사용 앞에 오는지는 읽지 않는다(그런 사용은 `continue` 로 남는다)

**행** [D]. relations 행마다(경로·번호·사용 지점 키): 쓰기 쪽 사건(A: `creator:` 줄의 생성 호출, B: `writer` 줄의 쓰기, C: `write after the spawn` 줄의 쓰기)과 사용 쪽 사건(A: lookup 호출, B: 사이트 호출, C: 읽기; C 의 스레드는 spawn 의 자식 가운데 그 읽기를 코드에 가진 것)의 짝을 스레드 짝마다 닫힘으로 본다(같은 스레드의 짝은 다른 스레드 짝이 있으면 세지 않는다; 경로 A 는 다른 스레드의 생성자만 짝짓는다): `hard_before`, `conditional_before`(쓰기가 앞), `fixed_early_proved`, `fixed_early_candidate`(사용이 앞), `unordered`(두 스레드가 풀렸고 어느 쪽 경로도 없음), `unknown`(한 스레드의 두 사건: 이 facts 는 spawn 호출 말고는 한 스레드 안의 차례를 주지 않는다; root 가 닿지 않는 함수; 행이 짝지을 사건을 이름 대지 않음). 행의 값은 짝이 하나의 값이면 그것, 아니면 `unordered` 가 있으면 unordered, `unknown` 이 있으면 unknown, 방향이 다르면 unknown, 그 밖은 conditional 쪽이다. 행은 값마다 짝 줄을 6 개까지 적고(출력 한도: 모든 짝을 계산하고 `pairs:` 줄이 모두 센다), 짝의 경로(간선 id), 조건, 스레드를 시작한 간선(`threads started by:`, C 는 그 행의 spawn 호출만), relations 행의 다른 차례 실현 가능성(`opposite:`)을 적는다.

**하지 않는 것** [D]. 한 스레드 안의 일반 프로그램 순서(CFG 순서는 facts 에 spawn 호출 뒤 도달뿐), 호출의 guard(facts schema 개정 11 의 호출 행에 없음; 예산 탈출 뒤 호출자 쪽 결과), 지역 플래그 술어(대기 술어의 다른 꼴), 잠금 정체성 개정 2 세대의 handle 단위, 생성 토큰, 실행 기록의 인스턴스화(`order.sync_results`, `order.observed`). 이것들은 그 단계의 일이다.

**시험** [M]. fixture `order.c`(사례 이름 없음): 이름을 만든 뒤 그 이름을 찾는 스레드를 시작(hard_before), 찾은 뒤 만드는 스레드를 시작(fixed_early_proved), 만드는 스레드를 시작한 뒤 찾음(unordered); spawn 과 쓰기에 걸쳐 잡은 잠금(HB-lock)과 잠금 창(unordered); 스레드를 만든 뒤 실패를 돌려주는 spawn 연산(child without success: possible)과 기본 연산의 결과를 돌려주는 연산(not found), 앞의 것의 실패를 제 상수로 바꾸는 연산(possible, 안쪽 호출을 거쳐); 읽는 스레드를 시작하기 전에 쓴 필드(경로 B, hard_before)와 다른 스레드의 writer(unordered); 지연을 두고 기다린 플래그를 검사 함수에 넘김(예산 탈출이 return: hard, 이어짐: conditional); 루프로 시작한 entry 표(반복 차례와 observed-only 시작 차례). golden `static_golden_order_np`(order 두 번 같은 바이트), `test_static_cli` 의 한 블록.

## 5q. 추출의 기본 경로 고침: 읽기 옵션의 대조, 잘린 이미지의 펼치기·포인터 필드, 이름 없는 레코드 형의 이름 (relations rules revision 8, 2026-11-16) [D]

결과는 `plan/full_stage_results_20261116.md` §5. facts schema 는 개정 12 그대로다(섹션·레코드 배치·열거형 변화 없음). 바뀐 것은 extract 의 BASIS 항목 하나, 두 소비자의 잘린 이미지 규칙, 레코드 형의 이름(기존 문자열 칸의 값), relations 의 사유 하나와 계층 표시 하나다.

**읽기 옵션의 대조** [D]. §5k 는 extract 의 읽기 옵션을 적기만 했고 basis 의 `front_end.arguments_added` 와 대조하지 않았다. 이제 대조한다.
- 두 글은 한 함수가 만든다(`kzs::reading_arguments_text`, `lib/llvm/ast_run.cpp`: 단위마다 주는 인자 `-Wno-unknown-warning-option -Wno-error -w`, `-resource-dir=`, `--extra-arg` 들, 프런트엔드가 받는 차례). basis(`lib/llvm/basis.cpp` `Builder::front_end`)와 extract 가 같은 함수를 쓰므로 두 글이 따로 만들어져 어긋날 수 없다. 앞판의 basis 글과 이 함수의 글은 같다(같은 차례, 같은 구분자).
- 프런트엔드도 대조한다: basis 의 `front_end`(`Clang LibTooling, <full version>`, `kzs::reading_front_end_text`)와 extract 를 돌리는 도구의 것.
- 같으면 facts BASIS `extract.reading_options_vs_basis = same as basis front_end.arguments_added and front_end`(정책 unset).
- 다르면(basis 에 그 항목이 없을 때도) extract 는 거부한다: 종료 코드 1, facts 파일을 쓰지 않는다. 그런 facts 는 basis 가 기술하는 단위를 기술하지 않기 때문이다. `--accept-option-difference` 를 주면 추출하고 `extract.reading_options_vs_basis = differs (--accept-option-difference): arguments: basis front_end.arguments_added [..], this extraction [..]; front end: …` 를 정책 deviation 으로 적는다(다른 쪽만).
- basis 가 없으면(`--source`) `not compared: no basis (--source)`.
- relations(규칙 개정 8): 그 항목이 `differs` 로 시작하면 모든 행이 `reading_options_differ(extraction options differ from the basis)` 를 갖고(전체 facts 의 사유), layers 칸에 `reading-options-differ(extract.reading_options_vs_basis)` 가 붙는다. 머리에 그 항목을 한 줄로 적는다(있을 때). 그 항목이 없는 facts(앞판 추출)는 다르다고 보지 않는다. 기존 사유 `reading_options_differ(basis deviation of a unit of the row)`(§5m)는 그대로다: 단위 하나의 basis 편차와 추출 전체의 옵션 차이는 다른 것이다.
- 이 대조는 프런트엔드가 받는 인자 글과 프런트엔드 판의 글을 비교한다. 같은 글이 같은 읽기를 준다는 것은 같은 도구 판 안에서의 전제다. basis 의 단위 분석 인자(`front_end.unit_diagnostic`)는 basis 에서 그대로 가져오므로 대조할 것이 없다.

**잘린 이미지의 소비자 둘** [D]. §5j 의 "고치지 않았다" 둘을 고쳤다.
- 펼치기(`lib/llvm/unroll.cpp` `unroll_table_walks`): 원소 객체의 이미지에 표시 잎이 있고 찾는 필드가 이미지에 없으면, 값은 전처럼 모름(상수 아님)이고 글이 까닭을 적는다: 인자·callee 는 `field F not in the image, which is truncated at <표시 경로> (the field may be in the part not read)`, guard 가 그런 필드를 읽어 residual 이 된 호출 행은 그 글을 행의 text 칸에 둔다(`kz-static show` 는 `call site … residual if … (the guard reads a field not in the static image of O, which is truncated at …)`). 표시 잎이 없으면 글은 전과 같다(`field F not in the image`). 원소의 멤버 차례를 이미지가 주지 않으므로 그 필드가 잘린 부분에 있는지는 정하지 않는다(있을 수 있다고만 적는다).
- PTR_FIELDS(`lib/llvm/argflow.cpp`): 정적 초기값 쓰기는 이미지의 남은 잎에서만 나온다. 읽기의 뿌리 객체 이미지에 표시 잎이 있으면, 읽지 않은 부분에 그 필드의 다른 초기값이 있을 수 있으므로 후보 하나로 `resolved` 가 되지 않고 `may` 다(기존 열거형 값).
- table walk(`finalize.cpp`)와 relations 는 §5j, §5m 그대로다.

**이름 없는 레코드 형의 이름** [D]. 앞판은 typedef 이름도 태그도 없는 레코드를 `<anonymous>` 하나로 불렀고, 이름 표(`TuReader::names`)는 파일 범위의 태그와 typedef 만 담았다. 그래서 (1) 서로 다른 형이 한 이름으로 합쳐졌고(C1 의 `<anonymous>.Data` 는 두 레코드의 멤버 형), (2) 규칙 대상(R-T1, R-T2)이 `<anonymous>.f` 를 고르면 둘째 읽기에서 어느 레코드도 그 이름이 아니어서 쓰기가 writer 로 맞지 않았고 상태가 `writer-set-complete writers=0` 이었으며(§5k), (3) 레코드 안의 태그와 블록 안의 태그는 찾지 못했다(`Inner.ix` 가 unknown-target). 이제 `TuReader::name_unnamed_records`(`lib/llvm/ast_tu.cpp`)가 단위의 모든 레코드 정의(레코드 안, 함수 몸 안 포함)를 걷고, 파일 범위 이름이 없는 레코드에 이름을 준다. 차례:
1. 레코드 안에 선언한 태그: 그 태그(C 에서 struct 안의 태그는 파일 범위다).
2. 함수 몸 안의 태그나 typedef 이름: 그 이름 + `@<파일>:<줄>`(선언 위치).
3. 태그 없는 레코드가 이름 있는 멤버의 형(배열 멤버 포함): `<바깥 레코드>::<멤버>`(선언 차례로 첫 멤버). 예: `CFE_SB_MemParams_t::Partition`.
4. 태그 없는 레코드가 객체의 형(파일·블록 범위, 배열 포함): `<객체 키>::type`(내부 연결 객체는 키에 정의 위치가 있다, `TuReader::object_key`). 예: `PSP_SoftTimebase_Global@psp/fsw/modules/soft_timebase/cfe_psp_soft_timebase_c:44::type`.
5. 그 밖: `<anonymous@<파일>:<줄>:<열>>`(정의 위치).
- 위치는 점 없이 쓴다: 파일의 `.` 를 `_` 로 적는다(`TuReader::name_pos`). `Type.field` 글을 첫 점에서 나누는 곳(`spec.cpp` `parse_field_ref`, `--target`)이 있어서 형 이름에 점이 없어야 한다. 객체 키 자체(GLOBAL_IMAGES 의 객체 칸 등)는 바꾸지 않았다.
- 같은 정의는 그것을 읽는 모든 단위에서 같은 이름이다(머리말의 형은 위치·바깥 이름이 같다). 2026-11-16 검토 뒤 덧붙임: 이것은 한 추출 안의 말이다. 묶음 루트 밖의 파일은 위치를 절대 경로로 쓰므로, 같은 소스라도 빌드 디렉터리가 다르면 이름이 다르다(§5r 알려진 한계).
- C11 이름 없는 멤버(익명 struct·union)는 규칙을 바꾸지 않았다: 그 필드는 감싼 레코드의 필드다(`record_name` 이 감싼 레코드의 이름을 주고, `targets_of_field` 가 그리로 올라간다).
- 바뀐 규칙 하나: 이름 있는 멤버의 형인 태그 없는 레코드는 이제 제 형이다. 앞판은 그 필드를 감싼 레코드의 필드로도 맞췄다(§5k 의 `struct Unn { struct { int q; } m; }` 에서 `--target Unn.q` 가 `g_unn.m.q = 5` 를 writer 로 셈). 이제 `Unn.q` 는 `unknown-target (the record type Unn has no field q …)` 이고 그 형은 `Unn::m` 이다(`--target Unn::m.q`).
- unknown-target 의 글: "no record type T is found among the record names of the units read"(앞판 "… the file-scope tags and typedef names …"). 이름 표가 넓어졌기 때문이다.
- 영향: 새 이름을 쓰는 모든 문자열 칸(WRITERS·READS·ARGFLOW·PTR_FIELDS·GLOBAL_IMAGES 의 형·필드, FUNC_WRITES 의 `record T.*`, FACT_LOCS·FIELD_STATUS, BASIS `target.T.f`, LOCKSET·LOCK_PATHS 의 형, 식 글의 `field:T.f`). 규칙 대상이 새 이름을 고르면 둘째 읽기에서 그 형의 쓰기가 writer 로 맞고(직접 쓰기, 데이터 함수(memset 등), out 포인터, 주소 탈출) 상태가 바뀔 수 있다. relations·order 의 행은 그 필드를 쓰면 바뀌고, 사용 지점 키의 글에 형 이름이 들면 키가 바뀐다(옛 키 → 새 키 표는 결과 기록 §5).

**relations rules revision 8** [D]: 위의 사유 하나(`reading_options_differ(extraction options differ from the basis)`)와 계층 표시와 머리 줄. 행의 다른 규칙은 개정 7 그대로다.

**시험** [M]. fixture `anonrec.c`(사례 이름 없음, C99): 매크로가 만든 이름 없는 union 형을 두 레코드의 멤버에 씀, 내부 연결의 이름 없는 struct 객체를 memset 하고 필드 주소를 생성자에 넘김, 레코드 안의 태그, 함수 몸 안의 태그와 이름 없는 union, 배열 멤버의 이름 없는 struct. golden `static_golden_anonrec_np`(`--no-profile --targets-rule R-T1,R-T2`, functions·writers·locs·fstatus·fwrites·gimages 와 relations). `test_static_cli`: 읽기 옵션의 같음·거부·`--accept-option-difference`(BASIS 항목, relations 의 모든 행의 사유와 계층 표시)·basis 없음; C11 이름 없는 멤버(`Anon.a`)와 이름 있는 멤버의 형(`Unn::m.q`, `Unn.q` 의 unknown-target); 중첩 태그 `Inner.ix`(이제 writer 1, writer-set-complete); 잘린 원소 이미지의 펼치기(guard residual 의 글, 인자의 글)와 잘린 뿌리 이미지의 PTR_FIELDS(`resolved` → `may`), 기본 예산과 `--image-budget 2` 의 대조. golden `imgbudget_np` 의 네 줄은 `field=<anonymous>.a` → `field=ImgUnion::s.a` 다.

## 5r. 검토 뒤의 규칙 고침: order rules revision 2, relations rules revision 9 (2026-11-16) [D]

결과는 `plan/full_stage_results_20261116.md` §6. facts schema(개정 12)와 추출은 바꾸지 않는다.

**order rules revision 2** (`lib/llvm/order.cpp`, "order rules revision 2 (2026-11-16)") [D]:
- **호출자에 기대는 spawn 호출.** spawn 연산의 사슬에 든 호출(spawn 호출, spawn-op 표시가 있는 호출, SPAWN_WRAP entry-argument·called-by-entry 행의 호출)이 부르는 함수 안의 spawn 호출은 호출자에 기댄다(caller-dependent): 그 호출에서 풀린 스레드는 모든 호출자의 합이지만, 한 호출 인스턴스는 그 함수에 닿은 바깥 spawn 호출의 스레드만 만든다. 앞판은 그런 호출(예: 생성 기본 연산을 부르는 감싸개 안의 호출)에 닿은 모든 스레드에서 그 호출이 풀린 모든 스레드로 갔다(`order.cpp` 의 닫힘 `'s'` 마디). 이제 스레드 t 가 닿은 그런 호출에서는 t 의 코드에 있는 그 함수의 호출자들이 만들 수 있는 스레드로만 간다(`children_on`: 바깥으로 따라감; 자기 스레드가 풀린 호출이면 그 안에서). 풀린 entry 를 넘기지 않는 호출자는 아무것도 더하지 않는다: 스레드를 덜 잡는 쪽이라 hard 닫힘은 간선을 잃기만 하고 얻지 않는다. HB-lock 간선도 쓰기의 스레드가 그 spawn 호출로 만들 수 있는 자식에게만 낸다. 그런 HB-spawn 간선에는 `caller-dependent:` 줄이 그 간선을 따라가는 스레드를 적는다. 근거: 한 호출 인스턴스의 자식은 그 인스턴스의 entry 인자로 정해지고(POSIX.1-2017 `pthread_create` 의 `start_routine`), 감싸개는 그 인자를 호출자에게서 받는다(SPAWN_WRAP entry-argument, called-by-entry).
- **child without success 의 위치 근거.** 증거 반환이 소스 위치만으로 서면(그 값이 RETURNS `redefined-later` 정의에서 오거나, 조건이 지역을 읽거나 — 그 지역이 안쪽 호출 자신의 결과일 수 있는데 이 facts 는 지역을 호출에 잇지 않는다 —, 그것이 옮기는 안쪽 호출의 표지가 그렇거나) `possible` 이 아니라 `unknown (position-based)` 다. 머리의 `summary spawn calls` 줄은 unknown 가운데 그 수를 괄호에 센다. 예: C1 의 `OS_Posix_InternalTaskCreate_Impl` 의 `return OS_ERROR`(`os-impl-tasks.c:547`)는 `pthread_create` 가 실패한 갈래이고(그 조건의 `return_code` 는 정의가 여럿인 지역), `OS_TaskCreate` 의 `:202` 반환은 `OS_ObjectIdAllocateNew` 의 정의가 `:199` 에서 다시 정의된다(`redefined-later`).

**relations rules revision 9** [D]: 생성자 callee 의 상수(§5o (5))에서, RETURNS 가 `redefined-later` 로 표시한 정의를 거쳐 오는 상수(그 상수의 RET_CONSTS 행, 또는 사슬의 call-result 반환)는 줄 끝에 `; not shown to reach the return: <그 정의>` 를 단다. 그런 정의의 값은 그 상수가 반환되는 경로에서 반환 전에 덮일 수 있다(도달 정의는 조건의 값을 보지 않는다; 예: `OS_CountSemCreate` 의 `return_code = OS_ObjectIdAllocateNew(...)` 는 성공이면 `:103`, `:106` 에서 다시 정의되고 `:109` 에 실패 값만 닿는다, `osapi-countsem.c:94-109`). 생성자 callee 마다 새 줄 `callee-check (set): <callee>: complete under these facts (…)` 또는 `… not established (N returns not followed, M constants not shown to reach the return): …; no success constant is taken from them`. 판정 규칙 8.2 기초 4 의 2026-11-16 덧붙임이 이 줄을 읽는다.

**알려진 한계** [D] (고치지 않음, 결과 §6.9):
- (2026-11-16 덧붙임: 아래 두 한계는 `extract --object-access` 아래에서 고친다, §5s. 옵션 없는 기본 경로는 그대로다.)
- §5q 의 위치가 붙은 형 이름은 한 추출 안에서만 같다: 위치는 `TuReader::name_pos` → `pos_of` 이고 묶음 루트 밖의 파일은 절대 경로로 쓴다(`lib/llvm/ast_tu.cpp`). 같은 CF 소스가 c1_pre_tx 와 c1_post_tx 에서 `…/c1_build/pre/…` 와 `…/c1_build/post/…` 로 다른 이름을 얻는다. 빌드 사이의 대응(이름 결합의 실행 사이 대응, 빌드 사이 키 비교)에 이 이름을 그대로 쓰지 않는다. 오늘 relations 의 키는 이 이름을 담지 않는다(`records/extract-fixes-20261116/key_map_<b>.tsv`: 바뀐 키 0).
- FACT_LOCS 의 writer-set-complete 는 배열 필드가 포인터로 바뀌어(배열에서 포인터로의 변환) 호출 인자로 넘어가는 것을 주소 탈출로 세지 않는다. C1 의 `CFE_SB_MemParams_t::Partition.Data`, `CFE_TBL_BufParams_t::Partition.Data` 는 `CFE_ES_PoolCreateEx`·`CFE_ES_PoolCreate` 에 `void *` 로 넘어가 풀 코드가 정수 주소로 그 안에 쓴다(`cfe_sb_init.c:104`, `cfe_tbl_internal.c:105`, `cfe_es_mempool.c:313`, `:84-85`). 그래서 그 두 행의 `writer-set-complete writers=1` 은 소스가 뒷받침하지 않는다. 추출 규칙의 고침(그런 넘김을 탈출이나 알 수 없는 효과로 셈)은 다섯 빌드의 다시 뽑기를 바꾸는 뒤 추출 단계의 일이다.

## 5s. 형과 객체 접근: `extract --object-access` (추출 옵션, 2026-11-16) [D]

결과는 `plan/full_stage_results_20261116.md` §7. 옵션을 주지 않은 추출은 앞판과 같은 바이트다(같은 코드 경로; 새 계수·수집·섹션은 모두 옵션 아래). 옵션은 확장 사실(`--extended-facts` 또는 `--no-profile`)과 함께만 받는다(그 밖이면 종료 코드 2). 읽기 규칙(아래 1–3)은 모든 추출 차례(profile-free 의 첫 차례, 완전성 차례, 둘째 차례)에 같고(`ReadOptions::object_access`), 섹션 쓰기는 마지막 facts 에만 한다(`ExtractSpec::object_access`). schema 는 개정 12 그대로다: 섹션 셋은 개정 12 가 정의한 레코드에 맞고(`include/kz/schema/kz_facts.h` 의 OBJECT_ACCESS·FIELD_TYPES·ARG_TYPES), `include/` 는 바꾸지 않았다.

1. **배열이 포인터로 바뀌는 대상 필드(writer 집합의 셈).** 배열 형의 대상 필드를 값으로 쓰면 첫 원소의 포인터로 바뀐다(C11 6.3.2.1 p3). 앞판은 그것을 읽기로만 보아(`classify_lvalue_use` 가 암묵 변환에서 멈춤) 그 포인터가 호출 인자나 정수로 나가도 writer 집합에 아무것도 세지 않았다(§5r 의 알려진 한계). 옵션 아래(`lib/llvm/ast_fn.cpp` `note_array_decay`, `decayed_target_arg`):
   - 호출 인자(괄호, 포인터 변환, 포인터 덧셈·뺄셈, 원소의 주소 `&x.f[i]` 를 지나도): `&x.f` 와 같은 out 포인터 호출(OUTCALLS 행). callee 요약이 판정한다: 저장만 하면 그 저장이 writer 행(out-pointer), 탈출하거나 본문이 없거나 간접 호출이면 그 필드의 주소 탈출 A-TYPE 행 "the array decays to a pointer to its first element, passed to …". 데이터 함수의 목적지·원천 인자는 그 요약이 이미 다룬다(건너뜀).
   - 정수로의 변환, 포인터의 저장(대입의 오른쪽, 초기화, 초기화 목록의 원소), 반환, 이 규칙이 읽지 않는 쓰임: 주소 탈출 A-TYPE 행 "the array decays to a pointer to its first element: …"(`&x.f` 의 "other use" 와 같은 꼴로 그 형의 대상에 걸림).
   - 원소에 대한 저장(`x.f[i] = v`, `*(x.f + k) = v`, 증감): 그 필드의 writer 행(assign·inc/dec, `partial-write`). 앞판은 이 저장을 writer 로 세지 않았다(FIELD_STATUS 의 `array-element-stores-not-collected` 표지는 그대로 남는다).
   - 포인터의 시험(비교, `!`, 조건, `&&`·`||`)과 원소의 읽기는 아무것도 세지 않는다.
2. **묶음 루트 밖 형 이름의 안정.** §5q 의 위치가 붙은 형 이름(`이름@파일:줄`, `<객체 키>::type`, `<anonymous@파일:줄:열>`)에서, 루트 밖 파일은 그 파일 위의 가장 가까운 `.git` 항목(디렉터리, 또는 submodule·worktree 의 `.git` 파일; `lib/llvm/gitrev.hpp`)이 있는 디렉터리, 곧 그 파일의 소스 트리를 기준으로 `<트리 이름>/<트리 안 경로>` 로 쓴다(`TuReader::name_file`). 같은 트리의 두 checkout(예: c1_pre_tx 의 `…/pre/src/cf` 와 c1_post_tx 의 `…/post/src/cf`)에서 같은 정의는 같은 이름을 얻는다. 어느 소스 트리에도 들지 않는 루트 밖 파일(시스템 머리말, 빌드 디렉터리의 생성 파일)은 앞판처럼 기록된 절대 경로다. BASIS `count.type_names_outside_root` 가 둘을 센다. 함수와 객체의 키(`이름@파일:줄`)는 바꾸지 않는다(형 이름이 아님).
3. **수집.** 읽기(READS)·writer(WRITERS)마다 접근 경로의 바탕(ARGFLOW 의 바탕 규칙 `analyze_path`): 포인터 없이 닿는 전역·전역 배열 원소·지역은 resolved, 전역에서 포인터를 지나는 경로는 partial(이름 댄 바탕은 경로가 시작하는 곳, 접근하는 객체는 pointee), 매개변수의 pointee·호출 결과·그 밖은 unresolved. 경로 글(`.`/`->`, 첨자는 상수 또는 변수 이름, 그 밖 `*`)과 첨자 식(SEQ). 읽기가 이름 대는 레코드 형의 필드마다 형(원소 종류와 크기, 배열 크기의 평가값과 쓴 그대로의 글, 비트 필드 폭, const·volatile·typedef). 호출 인자마다 형의 종류와 폭; 문자 배열(원소가 char·signed char·unsigned char)이 포인터로 바뀐 인자는 그 크기와 선언, 그리고 그 배열 위치(같은 형·필드, 같은 경로 글)를 읽는 READS 행이 호출을 지배하는지(같은 함수의 잠금 집합 골격 위: 같은 블록의 앞이거나 그 블록이 호출의 블록을 지배; 배열 자체의 넘김인 `array-contents` 읽기는 셈하지 않음).

**섹션**(옵션 아래에서만): OBJECT_ACCESS(READS 행마다 한 행, 이어 WRITERS 행마다 한 행; out-pointer·데이터 함수 writer 는 `site` 에 그 호출), FIELD_TYPES, ARG_TYPES(FACTS 차례). 식 뿌리는 앞 섹션의 것 다음. 선언이 있는 머리말 파일이 FILE_HASHES 에 더해질 수 있다. BASIS: `extract.object_access`, `count.object_access`, `count.field_types`, `count.arg_types`, `count.array_decay`, `count.type_names_outside_root`. `kz-static show` 의 절 `objaccess`, `ftypes`, `argtypes`.

**계획 생성기(plan rules revision 2, `docs/plan_manifest.md`)**: 키 인자가 문자 배열이고 그 크기가 있으며 호출을 지배하는 읽기가 있으면(ARG_TYPES), 사용 호출의 at-call 에서 키를 `string=validated bound=min(크기, KZ_RT_STR_MAX)` 로 적는다. 런타임의 복사는 프로그램이 스스로 읽는 그 배열 안에 머문다. ARG_TYPES 가 없는 facts(옵션 없이 뽑은 것)에서는 앞판처럼 포인터다.

**한계** [D]:
- 형 단위의 규칙은 그대로다: 주소 탈출 A-TYPE 행은 `&x.f` 의 규칙처럼 그 형의 모든 대상에 걸리고(FACT_LOCS), 필드 단위 규칙(FIELD_STATUS per-field)만 원인 필드로 가른다. 객체 단위의 writer 집합(같은 형의 두 객체를 가름)은 OBJECT_ACCESS 를 읽는 쪽(relations 의 writer 나눔, 판정 규칙 8.12 (ㅈ))의 일이고 이 옵션은 그 입력만 준다.
- 포인터 매개변수의 const 는 탈출을 면하지 않는다(C 는 const 를 벗기고 쓸 수 있다; §5h 의 형 변환 규칙과 같음). 그래서 본문 없는 함수(문자열 비교 등)에 넘어가는 문자 배열 대상은 모두 DISABLED 가 된다. P0 의미 표(뒤 단계)가 그런 함수의 효과를 주면 좁힐 수 있다.
- 지배는 잠금 집합 골격(Clang CFG)의 블록 지배다. 첨자가 변수인 두 경로(`chan[i]`)는 같은 변수 이름이면 같은 위치로 본다(그 사이에 변수가 바뀌지 않는다는 전제).
- 소스 트리의 기준은 `.git` 항목이다. 소스 트리가 아닌 checkout(내려받은 압축 파일 등)은 절대 경로로 남는다.

## 5t. 검증 상태가 붙은 의미: `extract --semantics` (추출 옵션, 2026-11-16) [D]

2026-11-18: 이 절의 판에서 SEMANTICS 의 `failure` 칸은 “성공 상수가 아닌 body 상수”였고(RESULT_EFFECT 의 outcome 과 다른 정의), status space 의 severity 상수는 모두(성공 severity 도) `failure` 칸에 `severity mask <이름>` 으로 적혔다. 둘 다 §5w 가 고쳤다(MC 도구는 아직 앞판).

결과는 `plan/full_stage_results_20261116.md` §8. 범위 규칙은 `docs/semantics_coverage.md`(개정 1, 어떤 자료 파일보다 먼저 커밋)이고, 자료 파일 `semantics/framework_semantics_<판>.txt`, `semantics/p0_declarations_<판>.txt`, `semantics/framework_namespaces_<판>.txt` 는 그 규칙의 기계적 훑기(연구 `env/tools/semantics_scan.py generate`)가 만든 것이며 `semantics_scan.py check` 가 다시 만들어 대조한다. 옵션을 주지 않은 추출은 앞판과 같은 바이트다(같은 코드 경로; 섹션·BASIS 항목·출력 줄은 모두 옵션 아래). 옵션은 확장 사실과 함께만 받고(그 밖이면 종료 코드 2), 자료 파일이 형식에 맞지 않거나(첫 줄, 알 수 없는 줄, 끝 줄 없음, 고정되지 않은 파일) 읽히지 않으면 아무것도 읽기 전에 종료 코드 2.

1. **결합(join).** 추출은 facts 를 한 번 쓰고 그 컨테이너를 다시 읽어(relations 와 같은 적재기 `RelFacts`, 그리고 FILE_HASHES) 의미 행을 계산한 뒤, 섹션 SEMANTICS 와 BASIS 항목을 더해 한 번 더 쓴다. 다른 섹션의 레코드는 같다(SEMANTICS 의 글은 다른 모든 글 다음에 STRINGS 에 들어간다).
2. **적용.** 프레임워크 자료의 항목은 빌드가 부르는 함수(직접 호출의 callee)에만, 그리고 빌드가 그 항목의 정의 파일을 고정된 sha256 으로 갖고 그 줄에서 그 함수를 정의하거나, 선언 머리말을 고정된 sha256 으로 가질 때만 적용한다. P0 선언 자료의 항목은 빌드의 FUNCS 가 그 함수의 선언으로 적은 파일·줄이 자료의 것이고 그 파일의 sha256 이 같을 때만 적용한다. 판이 다른 자료는 아무 행도 내지 않고 BASIS `count.semantics_entries[k]` 에 "called but neither pin held" 로 센다. 두 판의 고정 파일이 바이트로 같으면 두 판의 항목이 모두 적용된다(그 행들은 판의 글로 갈린다).
3. **층과 칸.**
   - P0: 빌드가 부르는 P0 표 항목마다 역할(종류·가족·의미 문장), out 인자, 조건부(try/timed); 검증 상태는 P0 표의 출처가 있으면 `standard-collated`(지금 `pthread_create` 하나: 로컬 upstream 사본), 없으면 `unverified`. P0 비교 표의 함수도 같다(모두 `unverified`). P0 선언 자료가 적용되면 그 선언의 속성: `pure`·`const` → out-behaviour("인자를 통해 저장하지 않음"), `noreturn` → role, `nonnull(i,…)` → precondition(0 부터 센 인자 번호); `doc-collated`, 출처는 머리말 file:line. `nothrow`, `warn-unused-result` 는 행을 내지 않는다.
   - 프레임워크: 문서의 반환(`documented-return`, 값, `doc-collated`, 머리말 file:line), 인자의 역할과 정의역(`arg-role`, `arg-domain`: 상태·event-type·run-status 와 그 상수 무리), 본문이 이 facts 에서 돌려주는 상수(`success`(성공 상수와 같은 값)·`failure`, `body-derived`; RET_CONSTS 와 RETURNS 의 call-result 반환을 따라간 사슬, relations 의 생성자 callee 상수와 같은 걸음), 문서와 본문의 일치(`agreement`), facts 가 도출한 역할(`role`: OP_DERIV 의 delay·terminal, SPAWN_WRAP 의 spawn).
   - 상태 공간: 항목이 하나라도 적용된 판의 성공 상수와 severity mask(`doc-collated`).
4. **일치(agreement)의 규칙.** 본문이 없으면 `not compared: no body`; 문서의 반환이 없으면 `not compared: no documented return`; 따라가지 못한 반환(매개변수·지역·필드·식·본문 없는 함수의 결과)이 있거나 나중에 다시 정의되는 정의를 거친 상수(relations rules revision 9 의 "not shown to reach the return")가 있으면 `not compared: the body's set is not established (…)` — 이 셋은 `unverified`. 집합이 서면 `derived = documented (n values)`, 또는 `the documentation lists a value the body cannot return: …` 와 `the body returns a value the documentation omits: …` — `body-derived`. 값은 쓴 그대로의 상수 값으로 비교한다(형 변환을 적용하지 않음; RET_CONSTS 가 주는 값과 같은 방식).
5. **BASIS**(옵션 아래): `extract.semantics`(결합 규칙 개정과 파일 수), `extract.semantics[i]`(파일 이름, sha256, 층, 판), `count.semantics`(행 수와 층마다 검증된 행 / 행), `count.semantics_entries[i]`(자료마다 항목 수, 적용 수와 그 고정, 부르지만 고정이 맞지 않는 수, P0 선언 속성 행 수). 추출 출력에 `semantics` 한 줄.
6. **relations**(SEMANTICS 가 있을 때만; 없으면 앞판과 같은 바이트): 머리에 `semantics of these facts …` 한 줄(BASIS `count.semantics`, 프레임워크 의미 항목이 있는 함수를 이름 대는 행 수와 그 가운데 검증되지 않은 항목이 있는 행 수), 행과 이름 공간 블록의 layers 칸에 `sem-unverified(api:column, …)`(행이 이름 대는 함수의 프레임워크 항목 가운데 `unverified` 인 것; P0 은 `P0-unverified(...)` 그대로). 규칙 개정 줄(relations rules revision 9)은 바뀌지 않는다.
7. **보기.** `kz-static show` 의 절 `semantics`.

**한계** [D]:
- 이름 공간 항목(`framework_namespaces_*`)은 판정 규칙 8.2 (가)의 입력이고 kz-static 은 읽지 않는다(이름 결합을 구현하는 설명 스크립트가 읽는다). `--semantics` 에 주면 형식 오류다.
- P0 의 행위 문장은 로컬 C 라이브러리 소스 사본이 있는 것만 대조된다(지금 `pthread_create` 의 upstream 2.39 사본; 배포판 패치는 대조하지 않음). 선언 속성은 빌드가 읽는 머리말의 것이고, 그 머리말 패키지와 빌드가 링크하는 C 라이브러리의 판이 같다는 것은 기록(`records/semantics-20261116/`)의 측정이다. 그 밖의 판·POSIX 글과의 대조는 새 외부 자원이 필요하다.
- 같은 이름의 함수가 판마다 다른 뜻을 가질 수 있으므로 항목은 이름으로 옮기지 않는다(고정 파일의 sha256 이 같을 때만 적용).
- 이 층이 방법이 받아들이는 framework-adapter 층인지는 PI 의 물음이다(가정하지 않음).

## 5u. 결과 흐름과 효과: `extract --result-flow`, relations rules revision 10, order rules revision 3 (2026-11-16) [D]

2026-11-18: (1) 개정 9 의 `callee-check (returns not followed)`·`callee-check (set)` 줄은 이 절의 줄이 생긴 뒤에도 그대로 둔다: 그 둘은 RETURNS 와 RET_CONSTS 만 읽은 집합의 완전성을 말하고(RESULT_EFFECT 를 읽지 않음), 판정 규칙 8.12 (ㄱ)과 설명 스크립트가 읽는 줄이다. 그래서 `callee-check (result flow)` 가 따라간 반환(매개변수를 그대로 돌려주는 마무리 함수)이 개정 9 의 줄에는 여전히 “not followed” 로 남는다. 두 줄을 하나의 완전성 규칙으로 맞추는 것은 판정 규칙의 덧붙임과 함께 할 뒤 단계다. (2) 이 절의 outcome 등급(success·failure·other)은 정의 없이 쓰였다: 정의와 그 고침은 §5w(outcome class rule revision 1). (3) guard 행의 주소 잡힌 지역, switch, 같은 callee 의 두 호출은 §5w(result flow rules revision 2). MC 도구는 아직 앞판이다.

옵션 `kz-static extract --no-profile --result-flow` (프로필 없는 방식에서만; 프로필 방식은 종료 코드 2). 옵션이 없으면 추출의 출력은 앞판 그대로이고, relations 와 order 는 RESULT_FLOW·RESULT_EFFECT 가 없는 facts 에서 revision 9·2 의 바이트를 낸다. 규칙의 판: `result flow rules revision 1 (2026-11-16)` (BASIS `extract.result_flow`). 섹션은 schema revision 12 의 142·143 이고 레코드·열거형은 바뀌지 않는다(`include/` 그대로).

**모든 회차가 적는 것.** 함수마다 매개변수를 바꾸지 않고 돌려주는 return(그 매개변수에 대입·증감·주소 취함이 없음), 호출 인자마다 그 상수 또는 그 값이 결과인 호출(직접, 또는 하나뿐인 도달 정의가 그 호출이나 상수인 지역). 첫 회차의 RETURNS 에서 함수마다 돌려줄 수 있는 값을 만든다(`lib/llvm/result_flow.cpp`): 상수 return, 호출 결과 return(그 호출의 값, 매개변수는 그 호출의 인자로 풀어), 바꾸지 않고 돌려주는 매개변수(기호로 두고 각 호출에서 인자로 품), 그 밖은 따라가지 않은 것. 몸이 없는 함수는 따라가지 않음이고, P0 의 스레드 생성은 0(성공)과 그 밖(오류 번호)이다(POSIX.1-2017 `pthread_create` RETURN VALUE; P0 표의 스레드 생성 계열). 나중에 다시 정의되는 정의를 거친 값은 `present-only` 다(RETURNS `redefined-later`).

**RESULT_FLOW (둘째 회차).** 결과가 있는 호출마다: 결과가 지역의 정의이면(대입·선언 초기값) 그 지역의 읽기마다 CFG 의 도달 정의(`reaching_defs`)로 이 정의가 닿는지 보고, 닿는 읽기마다 한 행(분기 조건·return·호출 인자·저장·그 밖; `unique` 이 정의만, `shared` 다른 정의도, `undef-path` 정의 없는 경로도). 결과를 바로 쓰면 `direct` 한 행, 버리면 `discarded`/`cast-void`, 필드·전역·역참조에 저장하면 `field-store`/`out-store`. 분기 행의 조건, 인자·return 행의 guard 에서 지역은 그 조건 자리에 닿는 정의로 이름 붙는다: `x@def:L:C`(하나), `x@defs:L:C|...[|undef]`(여럿), `x@undef`. 그래서 한 지역의 두 정의(다른 호출의 결과)는 서로 다른 항이다. **guard 행**(def none, use none): 호출 결과(직접, 또는 어떤 정의가 호출인 지역)를 읽는 조건 아래에서 도는 호출마다, 그 호출을 감싼 조건의 곱(같은 이름 규칙). 조건 안에서 표현 깊이 24 를 넘어 잘린 부분은 BASIS `count.result_flow_depth_cut` 에 따로 센다(`count.walk_bounds_hit` 는 앞판 그대로).

**RESULT_EFFECT (둘째 회차).** 결과가 분기에서 시험되거나 return 되는 호출 S 마다, S 의 callee 가 돌려줄 수 있는 값마다(따라가지 않은 return 이 있거나 P0 스레드 생성이면, 열거한 값 밖의 값 하나 더: `reach none`, 언제나 open) CFG 를 S 다음부터 걷는다. 이 값이 정하는 분기(조건이 S 의 살아 있는 정의나 S 자체를 읽어 값이 정해지는 것; switch 도)는 한 갈래만, 정하지 못하는 분기는 둘 다 간다(그 뒤의 효과는 `open`: "on a path through a branch this value does not decide"). 지역의 다른 정의가 나오면 S 의 값은 죽고, 다만 `v = g(..., v, ...)` 에서 g 가 그 매개변수를 바꾸지 않고 돌려주기만 하면 살아 있다. 모든 값이 닿는 블록은 공통이고, 이 값에만 닿는 블록의 호출·저장이 이 값의 효과다. 효과: 그 블록의 호출(직렬화가 분류: SEMANTICS 의 인자 역할 `event-type` 이면 event-send, `run-status` 이면 exit-app, 호출 표지 terminal-op 이면 terminal, 아니면 행 없음), 필드·전역에의 저장과 지역에의 상수 저장(store), 닿는 모든 return(S 의 값이면 return-result, 상수나 이 경로에서 대입된 상수면 return-const, 그 밖은 effect none open), S 가 다시 불리면 retry(그 경로의 지연 호출을 적음), 이 값의 효과가 없으면 fallthrough. 저장한 위치를 값으로 받는 뒤의 exit·terminal 호출과, S 의 값(살아 있는 지역)을 인자로 받는 exit·terminal 호출도 효과다; 그 사이에 그 위치의 주소를 받는 호출이 있으면, 그 callee 가 그 매개변수로 저장하거나 그것이 새거나 몸이 없을 때 open 이다(그 까닭을 적음). 위로의 합성(호출자에게 돌아간 값의 효과)은 facts 에 쓰지 않고 relations 가 읽을 때 한다(행 수가 호출자 수에 곱해지지 않게).

**relations rules revision 10** (그 두 섹션이 있을 때만; `rules` 줄은 revision 9 그대로이고 머리에 `rules with result flow:` 줄과 표지 수 줄이 더해짐):
- 경로 A 행: `absence:` 줄(부재 값 `nomatch_values` 마다 그 lookup 호출에서의 효과)과 `handling_label:` 줄. 부재 값이 정해지지 않으면 그 호출의 failure 값 모두로 계산하고 그렇다고 적는다.
- 경로 B 행: 그 행의 호출 지점마다 callee-check 의 failure 값으로 표지를 구하고 행의 표지는 R > H > T? > J > T 차례의 첫 것(지점 수를 함께 적음).
- 생성자 callee: `callee-check (result flow)` 줄(그 함수의 return 에 닿는다고 RESULT_EFFECT 가 보인 값)과 `callee-check (result flow, success)` 줄. 완전성은 여전히 `callee-check (set)` 줄이 말한다.
- 표지(판정 규칙 3.4 의 처리 사실이며 판정이 아님): R = 정해진 경로의 terminal·exit-app, 또는 failure 값이 행위자의 root 까지 return 됨; H = probe-create 또는 retry(재시도 루프 자신의 한도 분기를 지나는 경로도); T? = open 효과, 열거되지 않은 값, 결과가 시험되지도 return 되지도 않는 호출, 함수 포인터로만 불리는 함수, 또는 event-send 뿐(자료는 event type 영역의 상수를 줄 뿐 그 등급을 주지 않음); J = 버려지는 호출에 닿음(부재 값이 정해지지 않으면 J?); T = 실패 등급이 아닌 return·저장·합류뿐. failure 등급 return 은 호출자의 호출로 고정점까지(방문 집합) 따라간다.

**order rules revision 3** (그 두 섹션이 있을 때만; 머리에 `rules with result flow:` 줄):
- child without success: spawn 연산 안의 생성 호출 x 에 성공 값의 RESULT_EFFECT 행이 있으면 위치 대신 그 행: 성공 값이 닿는 failure 등급 return 은 증인(그 값이 정하지 않는 분기를 지나도 CFG 의 경로이므로 증인), x 자신이 스레드를 만든 뒤 실패를 알릴 수 있으면(그 표지가 not found 가 아니면) x 의 실패 값이 return 되는 것도 증인. 성공 값의 행이 없으면 revision 2 의 위치 규칙.
- 대기 함수 g 를 부르는 호출 x(결과를 시험·사용): 예산 출구가 return 이고 그 자리 RETURNS 가 상수이면 그 상수가 예산 값이다. x 의 결과를 이름 붙은 정의(`local@def:L:C`)나 호출 결과로 읽는 guard 행의 호출이 호출자 쪽 사용이고, 그 guard 를 예산 값마다 평가해 모두 거짓이면 skip, 모두 참이면 continue, 아니면 정해지지 않음. skip·continue 인 호출자 쪽 사용에 HB-await 간선(대기 행과 같은 hard 조건). 예산 출구가 루프를 벗어나 이어지면(그 뒤 return 의 값이 지역 플래그에 달림) 정해지지 않음이라 적는다.

**알려진 한계.** event type·run status 의 등급을 쓰지 않는다(자료에 없음). 루프 출구의 지역 플래그 값(예산 출구 뒤 return 되는 지역)은 따라가지 않는다. 몸이 없는 함수의 매개변수 저장은 알 수 없음으로 둔다. 효과의 "닿음"은 CFG 경로이며 무한히 도는 루프는 따로 가르지 않는다.

## 5v. 루프 플래그 술어와 핸들 단위 잠금 정체성: `extract --loop-defs`, `extract --lock-handles`, relations rules revision 11, order rules revision 4 (2026-11-16) [D]

2026-11-18: 이 절의 규칙 가운데 flag 정의의 switch, 정의 행의 깃발, 핸들의 대체 규칙, awaited 필드와 writer 는 §5w 가 고쳤다(MC 도구는 아직 앞판).

옵션 `kz-static extract --no-profile --loop-defs`, `--lock-handles` (프로필 없는 방식에서만; 프로필 방식은 종료 코드 2). 옵션이 없으면 추출의 출력은 앞판 그대로이고, relations 와 order 는 LOOP_DEFS·LOCK_HANDLES 가 없는 facts 에서 revision 10·3 의 바이트를 낸다. 규칙의 판: `loop flag predicate rules revision 1 (2026-11-16)` (BASIS `extract.loop_defs`), `lock handle rules revision 1 (2026-11-16)` (BASIS `extract.lock_handles`). 섹션은 schema revision 12 의 140·144 이고 레코드·열거형은 바뀌지 않는다(`include/` 그대로).

**LOOP_DEFS (둘째 회차, `ast_fn.cpp` `emit_loop_defs`).** poll·count-bounded 루프 가운데 FACT_LOOPS 에 술어가 없고 조건 출구가 지역을 읽는 것마다. 예산 지역(FACT_LOOPS 의 예산, count-bounded 루프의 귀납 변수)은 조건이 아니다: 예산만 시험하는 출구와 conjunct 를 뺀 조건 출구의 합이 출발점이다. 그 합이 읽는 지역마다:
- **flag**: 루프 앞에 상수 정의 하나(`v_out`), 루프 자신의 몸에 guard G 아래 다른 상수 정의 하나(`v_in`), 루프 안의 다른 정의 없음, 주소 취함 없음, 조건 안의 모든 사용이 `v_out`·`v_in` 과의 비교, 그 지역을 시험하는 출구가 모두 루프 조건이거나 몸 바로 아래 조건 하나로 정해지는 출구. 그러면 `f == v_in` 은 G(그 정의가 돌 때 참이던 guard), `f == v_out` 은 그 부정이다. 출구가 매 반복 시험되고 정의가 다시 바뀌지 않으므로, 루프가 조건 출구로 끝날 때 G 가 어느 반복에서 참이었다.
- **value**: 루프 자신의 몸에 정의 하나(호출 결과, 또는 필드·전역의 읽기와 그 첨자·역참조), 주소 취함 없음, 모든 사용이 같은 반복의 그 정의 뒤에 있고 그 정의가 도는 조건들이 사용이 도는 조건들의 앞부분(같은 `if` 문, 포인터 같음)이며 루프 조건에서 읽히지 않음. 그러면 그 지역은 대입된 값이다.
- G 나 대입된 값이 다른 지역을 읽으면 같은 규칙으로 푼다(세 단계까지). 선언만 있고 초기값이 없는 선언은 정의가 아니다.
- 거절: 몸 안의 정의가 둘 이상(`refused-reset-in-body`), 주소 취함(`refused-address-taken`), 그 밖(`refused-unresolved`: 안쪽 루프의 정의, 상수가 아닌 flag, 꼴이 맞지 않는 사용, 루프 조건에서 읽히는 value). 조건 출구가 예산뿐이면 `not-examined`.
- 행: 그렇게 본 지역마다 그 지역의 정의마다 한 행(`where` before-loop / in-body / in-condition / none, 값과 guard 의 FACT_EXPRS 뿌리, 그 지역의 상태, 형태 이름), 그리고 루프 자신의 행(`local` 빈칸; `guard_root` = 모든 지역을 바꾼 술어, `pred` = 쓴 형태 "flag", "value", "flag+value"). 깃발 `under-lock`(어휘의 잠금이 그 정의에서 잡혀 있음), `call-result`(value 가 호출 결과). FACT_LOOPS·AWAIT_L1 의 행은 바뀌지 않는다.

**LOCK_HANDLES (`serialize.cpp`).** LOCK_OPS 의 acquire·release 가운데 잠금 정체성이 호출 결과 뒤로 사라진 것(정체성 글에 `?(`)마다 한 행. 사슬의 호출들(첫 호출은 LOCK_OPS 의 호출, 다음은 그 callee 안에서 다음 이름을 부르는 하나뿐인 호출) 가운데 SEMANTICS 의 인자 역할 `lock-object`(검증 상태가 unverified 가 아님)를 가진 callee 의 핸들 인자가 있는 호출, 없으면 핸들 값(필드·전역·매개변수·지역·원소·호출 결과의 값; 주소·문자열·함수·상수가 아님)을 하나만 넘기는 가장 깊은 호출이 핸들의 호출이다. 핸들이 부르는 함수의 매개변수이면 사슬을 거슬러 그 호출의 인자로 간다. 사슬에 핸들 호출이 없고 연산 함수 자신의 매개변수에 `lock-object` 항목이 있으면 그 매개변수가 핸들이다(`handle_root` 는 매개변수 읽기).
- 상태: 핸들이 전역 객체의 필드이고 FIELD_STATUS 가 그 필드의 writer 집합을 per-field 완전, 한 번 도는 writer 하나(`one-instance-once`)라 하고 `lock-object` 항목이 있으면 `static-instance`(깃발 `one-writer`, `writer-runs-once`); 항목이 없으면 `semantics-unverified`; 완전하지 않으면 `writer-set-incomplete`; 그 밖(writer 인스턴스가 하나가 아님, 매개변수, 전역 객체의 필드가 아님, 핸들 없음)은 `not-static`. `writer` 는 그 필드의 하나뿐인 out-pointer writer, `lock_id` 는 같은 정체성의 LOCK_IDS 행, `premise` 는 판단의 글과 전제(“핸들이 가리키는 객체가 두 연산 사이에 지워지고 다시 만들어지지 않는다”; 실행이 확인할 수 있음).
- 이 판의 의미 자료(`semantics/` 의 생성된 파일)는 인자 역할 `lock-object` 를 만들지 않는다(semantics coverage rule revision 1 은 status·event-type·run-status 만). 그래서 실제 빌드에서 `static-instance` 는 나올 수 없고, fixture 는 손으로 쓴 시험 자료 `tests/static/data/looplock_fixture/` 로 그 갈래를 시험한다.

**relations rules revision 11** (LOCK_HANDLES 가 있을 때만; `rules` 줄은 revision 9 그대로, 머리에 `rules with lock handles:` 줄): 경로 C 에서 static instance 가 아닌 잠금이 spawn 과 쓰기들에 같은 획득으로 잡혀 있고 읽기가 잡고 있을 때, spawn 의 획득들과 읽기의 획득들이 모두 같은 핸들을 넘기고(획득 지점의 callee 의 LOCK_HANDLES acquire 행; 매개변수 핸들은 그 호출의 ARGFLOW 로 읽어 같은 기준으로 판단) 그 핸들이 `static-instance` 이면 static instance 처럼 읽는다(나머지 proven-impossible 조건은 그대로; 글에 `one mutex by its handle` 와 핸들의 전제). 같은 핸들이지만 다른 상태이면 undecidable 의 글에 그 핸들과 상태를 적는다.

**order rules revision 4** (LOOP_DEFS·LOCK_HANDLES 가 있을 때만; 머리에 `rules with loop definitions and lock handles:` 줄):
- 대기의 술어: FACT_LOOPS 에 술어가 없고 LOOP_DEFS 의 루프 행이 recognised 이면 그 술어(`predicate: through local definitions (LOOP_DEFS, form …)`); 기다리는 필드, 다른 스레드의 writer, 초기값에서의 조건 출구는 그 술어로. HB-await 의 hard 조건은 그대로(AWAIT_L1 의 per-field 인증이 await 이어야 함)이므로, LOOP_DEFS 의 술어에는 그 인증이 없어 간선은 conditional 이다.
- 호출자 쪽 예산 값: 예산 출구가 루프를 벗어나 이어지고 루프의 술어가 flag 이면, 그 함수가 루프 뒤(루프 밖, 위치가 뒤)에서 돌려주는 값은 RETURNS 의 상수 대안들이고 flag 의 몸 안 정의로 정의된 대안은 뺀다(예산 출구에서는 조건 부분이 거짓이므로 flag 가 그 값을 갖지 않음). 상수가 아닌 return 이 있으면 정해지지 않음.
- HB-lock 간선에 `lock identity source:` 줄(`must-static-instance (LOCK_IDS)` 또는 relations 행이 핸들을 적은 `handle (LOCK_HANDLES; …)`).

**호출자에 기대는 spawn 규칙의 측정 (`kz-static order --caller-dependent-report FILE`).** order 글과 간선 목록은 바뀌지 않는다. 따로 쓰는 보고: 풀린 스레드가 있는 spawn 호출 수, 그 가운데 호출자에 기대는 것, 그 함수의 호출자와 그 가운데 풀린 entry 를 넘기지 않는 것(그 호출자를 코드에 가진 어느 스레드에서도 그 호출자의 `children_on` 이 비어 있음); 그리고 그런 호출자가 그 호출의 자기 스레드 모두를 만들 수 있다고 둔 둘째 닫힘(위 근사)에서 값이 달라지는 짝과 행. 둘째 닫힘은 실행이 순서를 주지 않는 짝도 순서 짓을 수 있으므로, 달라지는 짝의 수는 이 규칙이 unordered 로 남긴 짝의 위 한계다.

**알려진 한계.** flag 규칙은 루프 앞의 정의 하나와 몸 안의 정의 하나만 본다(분기마다 다른 상수를 주는 꼴은 거절). value 규칙은 같은 `if` 문 안의 앞선 정의만 본다(goto 를 따로 보지 않음). LOOP_DEFS 의 술어에는 L1 인증이 없다. 핸들 정체성은 핸들 값이 같으면 같은 객체라는 의미 항목에 기대고, 그 항목이 자료에 없으면 쓰이지 않는다. 핸들의 writer 판단은 LOCK_IDS 의 값 읽기 규칙(per-field 완전, 한 번 도는 writer 하나)과 같다.

## 5w. 2026-11-18 의 고침: outcome class rule, 열쇠를 지우는 호출, 대기의 writer 와 awaited 필드, 결정되지 않는 guard, 핸들 (relations rules revision 12·13, order rules revision 5·6·7) [D]

검토가 확인한 지적을 코드로 고친 것이다(연구 결과 문서 `plan/full_stage_results_20261116.md` §12). 이 절의 규칙은 소스에 있고, **MC 의 kz-static 은 아직 다시 빌드하지 않았다**(그 문서 §12.6): 아래는 규칙의 글이고 다섯 빌드의 측정이 아니다. 스키마(`include/`)는 바뀌지 않는다.

**outcome class rule revision 1 (2026-11-18)** — `lib/llvm/outcome.hpp` 하나가 SEMANTICS 의 success·failure 칸(`semantics.cpp`), RESULT_EFFECT 의 outcome(`serialize.cpp`), relations 의 failure·success 등급(`relations.cpp`)을 정한다. 함수가 돌려주는 값 v 의 등급은 그 함수의 status space 에서:
- `success`: v 가 그 space 의 성공 상수(의미 자료의 `status-space success` 줄).
- `failure`: 성공 상수가 아니고, v 가 음수이거나, 그 space 에 severity 상수가 있고 그 상수들의 비트 합(mask)의 모든 비트가 32 비트 값 v 에 서 있음.
- `other`: 나머지.
- 함수의 space: 그 함수에 적용되는 의미 항목의 component 의 space. 항목의 return kind 가 `status` 가 아니면 모든 값이 `other`(SEMANTICS 에는 success·failure 행을 쓰지 않고 `none` 칸의 `returned constant (the return kind is …: no outcome class)` 행). 항목이 없는 함수는 facts 의 component 들이 함께 갖는 space(성공 상수가 모두 같을 때 그 값, mask 는 합). SEMANTICS 가 없는 facts 는 규칙의 대체값(성공 상수 0, mask 0xC0000000).
- 1차 근거: 값은 자료 행에서 온다(`kairos/semantics/framework_semantics_cfe-6.4.1.txt:122-126` 의 `status-space success CFE_SUCCESS 0 …cfe_error.h:158`, `severity CFE_SEVERITY_BITMASK 3221225472 …:132`, `CFE_SEVERITY_ERROR 3221225472 …:136`, `CFE_SEVERITY_INFO 1073741824 …:135`; OSAL 은 `status-space success OS_SUCCESS 0` 뿐). “mask 의 모든 비트가 서면 failure” 와 “음수면 failure” 는 method rule 이다: 앞의 failure 범위 규칙(`plan_gen.cpp` `fail_class`, `order.cpp` `fail_class`; 근거는 `plan/observation_path_results_20261116.md` 의 실패 등급 줄)과 같은 값 집합이고, mask 와 같은 값의 severity 상수가 error severity 라는 것은 자료 행의 이름(`CFE_SEVERITY_ERROR`)이 말한다. 엔진은 그 이름을 읽지 않는다. 대체값의 성공 상수 0 은 생성된 자료 일곱의 `status-space success` 줄이 모두 0 이라는 것에 기댄 method rule 이다 [I: 그 밖의 framework 에서는 확인하지 않음].
- BASIS: `extract.outcome_classes`(규칙 이름, facts 의 space 들, 항목 없는 함수의 space), `extract.semantics_outcome_rule`.
- 앞판과 달라지는 것: 앞판 RESULT_EFFECT 는 엔진의 범위(`0x40000000..0x7FFFFFFF` other, 그 밖의 비실패 success; `ast_fn.cpp` 의 옛 `rf_class`)로, SEMANTICS 는 “성공 상수가 아닌 body 상수는 모두 failure” 로 나눴다. 이제 성공 상수가 아닌 양수는 `other` 이고(앞판은 success), 정보 코드(severity 가 mask 전부가 아닌 값)는 SEMANTICS 에서도 `other` 다(앞판은 failure 칸).
- status space 의 severity 상수는 `status-space none … [severity constant <이름>]` 행이다(앞판은 failure 칸의 `severity mask <이름>`; 읽는 쪽은 옛 꼴도 받는다). 항목마다 `framework none <api> … [return kind <kind> (<by>)]` 행 하나가 더 생긴다.
- 문서가 반환을 적었는데 이 facts 의 본문이 값을 돌려주지 않는 함수(모든 RETURNS 행이 값 없음: void 로 선언된 함수)의 `documented-return` 행은 `unverified` 이고 항목 글에 `(doc-declaration mismatch: the body in these facts returns no value)` 가 붙는다(앞판은 `doc-collated`).

**효과의 이름(RESULT_EFFECT)** — 인자 역할 `run-status` 만으로는 `exit-app` 이라 부르지 않는다: 그 역할의 인자를 받는 호출이 facts 의 terminal operation 표지(`KZ_CALL_FLAG_TERMINAL_OP`)도 가질 때만 `exit-app` 이다. 역할은 매개변수의 값 영역만 말한다(`framework_semantics_cfe-6.4.1.txt:139` 는 run-loop 점검 함수의 포인터 매개변수에 그 역할을 준다). 그런 호출이 terminal 도 아니면 효과가 아니다.

**result flow rules revision 2** — guard 의 조건에서 주소가 잡힌 지역은 `x@addr-taken` 으로 적는다(도달하는 정의가 하나라는 주장을 하지 않음: 별칭을 통한 저장이나 주소를 받는 호출이 사이에 올 수 있다; 행에 `addr-taken-local`). switch 의 case 아래의 호출은 guard 에 `(top a case of the switch at L:C on <switch 의 값>)` 항을 갖고, switch 로 가는 사용 행의 조건은 `(top the value a switch selects its case by …)` 다(앞판은 case 조건이 없는 guard 와 `(ne 값 0)`). 읽는 쪽은 이 두 항을 모르는 값으로 두고 나머지 항으로만 guard 를 정한다(셋값 논리: 거짓인 항이 하나 있으면 거짓). 조건들이 같은 callee 의 서로 다른 호출 둘을 담는 guard 는 `not-computed` 다(call-result 마디가 callee 이름만 가져 두 결과가 한 값으로 읽힘): order 는 그런 guard 를 `unknown(…two calls of one callee)` 로 둔다. **남은 한계**: call-result 마디에 호출 site 를 싣지 않았다.

**loop flag predicate rules revision 2** — 몸 안 정의가 그 루프 안의 switch 의 case 아래에 있으면 flag·value 규칙을 쓰지 않고 `refused-unresolved` 다(guard 가 case 조건을 갖지 않아 정의의 guard 를 모름; 앞판은 술어 `1` 의 recognised 를 냈다). 정의 행의 깃발은 그 정의의 것이다: `call-result` 는 그 행의 value 가 호출 결과일 때, `under-lock` 은 루프 안의 정의에서 걷는 쪽의 잠금 깊이가 0 이 아닐 때만(루프 밖 정의의 잠금 상태는 적지 않는다; 앞판은 지역의 깃발을 모든 행에 되풀이했다). 루프 자신의 행은 인식된 정의들의 깃발을 모은다(앞판과 같음).

**lock handle rules revision 2** — 핸들은 연산의 사슬의 호출이 모두 찾아졌을 때만 이름 댄다(사슬의 함수가 다음 함수를 한 곳에서만 부름; 끝은 그 행의 잠금 family 의 잠금 연산). 아니면 핸들이 없고 전제 글은 `the calls of the chain are not all located (…): no handle is named` 다. 앞판은 따라간 앞부분의 가장 깊은 호출의 값 인자를 썼다: 그래서 다른 family 의 행에 한 family 의 핸들이, 잠금 연산이 아닌 함수의 인자가 핸들로 적혔다. **남은 한계**: 사슬이 모두 찾아진 행에서도 “값 하나를 넘기는 가장 깊은 호출” 은 그 값이 그 family 의 잠금을 가리킨다는 증명이 아니다(검증된 `lock-object` 항목이 있을 때만 `static-instance`).

**배열의 decay(`--object-access`)** — 대상 배열 필드의 포인터가 조건 연산자나 쉼표 연산자를 거쳐 호출 인자가 되면 주소 탈출이다(out-pointer 호출이 그 꼴을 읽지 않음: `passed to a call in an expression the out-pointer rule does not read`). 배열의 배열인 필드의 한 줄(`x.f[i]` 가 decay)은 `&x.f[i]` 처럼 out-pointer 호출이다. 본문이 있는 가변 인자 함수의 이름 없는 인자로 넘어간 주소의 사유는 `an argument beyond the named parameters of a callee defined in the closed program: …` 다(앞판은 “the callee has no body in the closed program”; 이 사유 글은 옵션 없는 경로의 out-pointer 호출에도 쓰인다).

**relations rules revision 12** — 경로 A 의 열쇠 필드에 대한 쓰기 가운데 WRITERS 가 저장 값을 상수로 주는 것은 열쇠를 지우거나 초기화하는 저장이다. 열쇠 매개변수 함수의 사용이 닿는 열쇠 쓰기가 모두 그런 저장이면(값이 상수가 아닌 WRITERS 행도, WRITERS 행이 없는 FUNC_WRITES 행도 닿지 않음) 그 사용은 **key-clearing call** 이다: 생성자 site 도 조회 행도 아니고, 같은 partition 의 조회 행에 `key clearing: <위치> in <함수> -> <callee> …; no creator: every write of the key it reaches stores a constant (…)` 줄로 적힌다. namespace 머리의 `key-parameter functions:` 줄에 `key-clearing (…)` 목록, `uses:` 줄과 `summary A membership and creators` 줄에 그 수(0 이 아닐 때만). 함수 이름을 보지 않는다. 1차 근거: cFE 6.4.1 의 `cfe_sb_api.c:431` `PipeId = CFE_SB_INVALID_PIPE`(지우기)와 `:235` `PipeId = PipeTblIdx`(만들기); 앞판은 `cfe_sb_priv.c:123` 의 지우는 호출을 13 행의 creator 로 적었다. **한계**: 값을 주는 WRITERS 행이 없는 쓰기는 살아 있는 쓰기로 둔다(덜 지우는 쪽); 상수를 열쇠로 저장하는 생성(드묾)은 지우는 저장으로 읽힌다 [미확인: 다섯 빌드에서 그런 생성이 있는지].
**relations rules revision 13**(RESULT_FLOW·RESULT_EFFECT 가 있을 때): failure·success 등급을 outcome class rule 로 읽는다.

**order rules revision 5**(모든 facts) — 대기의 awaited 필드는 술어가 스스로 읽는 필드다: 술어가 결과를 시험하는 호출의 인자 아래와 `addr` 아래의 필드는 awaited 가 아니다(out 인자나 열쇠이지 기다리는 값이 아님; `awaited:` 줄에 `not awaited, under the arguments …` 로 적음; 그런 필드뿐이면 `awaited: not named: the predicate reads no field of its own (…)`). awaited 필드의 writer 는 기다리는 스레드마다 센다: 쓰기의 스레드는 자기 아닌 모든 기다리는 스레드의 writer 다(앞판은 writer 의 스레드가 기다리는 스레드들 가운데 하나이기만 하면 모두에게서 뺐다: 시작할 때 스스로 기다리는 응용 스레드가 자식의 레코드를 쓰는 경우 그 writer 가 사라졌다). HB-await 간선은 (writer 스레드, 기다리는 스레드) 짝마다이고 hard 조건의 “writer 하나” 도 그 스레드의 writer 수로 본다. writer 스레드가 기다리는 스레드이기도 하면 `awaited:` 줄이 `writers N (a writer thread is itself a waiting thread: for each waiting thread the writers in another thread: …)` 꼴이다. 행위자는 entry 함수로 구분하므로 같은 entry 의 다른 스레드 인스턴스는 가르지 못한다(기다리는 행위자가 하나뿐일 때 그 행위자의 쓰기는 뺌).
**order rules revision 6**(RESULT_FLOW 가 있을 때): `not-computed` guard 는 아무것도 정하지 않는다. **order rules revision 7**(LOOP_DEFS 가 있을 때): 위 revision 5 의 두 규칙이 LOOP_DEFS 술어에도 적용된다.

## 5x. 다음 개정의 출력 형식: relations rules revision 14, order rules revision 8 의 쌍 파일 (2026-11-18; 글만, 그것을 내는 코드보다 먼저 커밋함) [D]

이 절은 형식이다. 아래 줄을 내는 코드는 아직 없고, MC 의 kz-static 은 이 절을 구현하지 않는다. 뒤의 kz-static 빌드가 이 형식대로 내고, 그 선언이 앞판 출력과의 차이를 줄 종류마다 적는다. 이 줄을 읽는 것은 판정 규칙 8.13(`plan/ovm_rev1_20261116/JUDGEMENT_RULES_REV1.md`)과 설명 스크립트 개정 4 다. 형식을 바꿔야 하면 이 절을 먼저 고쳐 커밋한다. 스키마(`include/`)는 바뀌지 않는다. 함수·필드·사례 이름은 어느 규칙에도 없다. `<…>` 는 자리, `|` 는 택일이다.

**머리의 도구 줄(relations 와 order 의 글 출력, 쌍 파일).** `rules` 줄 바로 뒤에 한 줄:

```
tool kz-static tool_sha256=<64 hex> tool_source=<64 hex>
```

계획 sidecar 의 `generator` 줄과 같은 두 값이다. 읽는 쪽은 relations 글, 쌍 파일, sidecar 의 `tool_sha256` 이 서로 같은지 본다. 이 줄 때문에 같은 사실의 글이 도구마다 한 줄 다르다(선언할 차이).

**사용 지점 키의 글(§5l 에 더함).** 묶음 루트 밖에 정의된 내부 연결 함수·객체의 위치는 키 글에서 절대 경로 대신 소스 트리 토큰으로 쓴다: `<tree>/<트리 안 경로>`, 형 이름이 이미 쓰는 토큰이다(§5s). 토큰은 추출 때 파일 시스템에서 정해지므로 사실이 그것을 싣는다: BASIS 항목 `source_tree[i] = <기록된 디렉터리> <토큰>`(옵션 `--object-access` 아래에서만). 그러므로 **빌드 사이의 행 정체성은 옵션을 켠 사실의 키에서만 쓴다.** 옵션 없는 사실의 relations 는 앞판의 키 글을 그대로 내고, 그 키는 빌드 사이의 행 대응에 쓰지 않는다. 사실 안의 함수 이름은 바뀌지 않는다. 앞 도구의 사실에서 잰 것 [예비: 연구 `records/closure-texts-20261118/out/p6_keys.txt`]: 키 글에 절대 위치가 든 키는 c1_pre_tx 3, c1_post_tx 3, 나머지 세 빌드 0.

**relations rules revision 14** (RESULT_FLOW·RESULT_EFFECT·OBJECT_ACCESS 가 있는 사실에서; 없는 사실은 앞판의 바이트에 도구 줄만 더함). 모든 줄은 더하는 줄이고, 있는 줄은 키와 머리 말고 바뀌지 않는다.

1. 경로 A 행, 생성자 callee 마다 한 줄:

   ```
   callee-check (success path): <callee>: held: success value <v> [outcome class rule]; carried by <n> alternative(s): <위치> alt <i>/<m> = <기원> under <조건>; ...; excluded: <위치> alt <i>/<m> <constant <c> | its own branch <조건>>; ...; name store <위치> [<Type.field>] implied by the carrying condition; out-id store <위치> in <함수> through parameter <k> (PARAM_FLOW out-store) under <조건>, parameter non-null on the path; stored value: <정의> at <위치>, the only definition that reaches the store under <조건>
   callee-check (success path): <callee>: missing(<까닭>): <보인 데까지의 같은 항들>
   ```

   까닭의 글: `the returns are not all followed`, `a second origin can carry the value`, `no name store on a carrying path`, `no out-id store on a carrying path`, `the stored value is not tied to the success path`, `an implication between conditions is not decided`. 조건 사이의 관계는 조건 나무의 함의로 본다(같은 글인지가 아니다; guard 의 `top` 항과 `@addr-taken` 항은 모르는 값, §5w). out-id 저장은 callee 사슬의 PARAM_FLOW `out-store` 행에서 읽는다. 개정 9 의 `callee-check (set)`·`(returns not followed)` 줄과 개정 10 의 `(result flow)` 줄은 그대로 둔다. 머리에 `summary success path: creator callees <n>: held <h>; missing <m> (<까닭> <수>; ...)`.

2. 경로 A 행, `absence:` 줄 뒤에 한 줄:

   ```
   absence origins: <값> [<상수 이름>]: <n> origin(s): <함수> at <위치> when <조건> [search no-match]; <함수> at <위치> when <조건> [other]; ...
   ```

   조회 사슬의 함수들에서 그 값을 내는 RET_CONSTS 행 모두다. `[search no-match]` 는 `absence:` 줄의 사슬이 끝나는 검색의 것.

3. 경로 A·B 행, 조회의 `id holder:` 줄 뒤에 한 줄:

   ```
   holder: <field <Type.field> | local <이름> in <함수>>; writers <n>: own out store <위치>; clearing <위치> in <함수> value=<상수> [<fill | assign>]; other definition <위치> in <함수> value=<값 종류>; ...; FIELD_STATUS per-field=<상태> type-level=<상태>[: <막은 까닭과 위치>]; readers <m>: <함수> at <위치>: <tests the cleared value before the dependent use at <위치> | uses the value without such a test | not decided: <까닭>>; ...
   ```

   writer 는 WRITERS 의 행 모두다(지우는 저장을 빼지 않는다). 값 종류는 WRITERS 의 `value=` 다(`const`, `symbolic`, `top`). "지운 값"은 `clearing` 줄들의 상수다. reader 의 가름은 그 reader 함수 안에서 그 필드의 값(또는 그것을 인자로 받은 호출의 결과)을 시험하는 분기가 의존하는 쓰임을 지배하는지를 RESULT_FLOW·READS 의 guard 로 본 것이다. 지역 holder 는 FIELD_STATUS 자리에 `local: definitions <n> (RESULT_FLOW)` 를 적는다.

4. 경로 A·B 행, 부재 값의 효과를 걷는 호출 사슬(문맥)마다 한 줄:

   ```
   alternatives [context <k>: <root 함수>@<호출 위치> <- ... <- <사용의 함수>@<사용 위치>]: none found (exploration complete) | found: <retry | creation in the same name space and partition by the same function | another path>: <위치와 줄> | not established (<까닭>); rests on: <P0-unverified(...) sem-unverified(...) | none>; premises: <글; ... | none>; decided calls: <호출 위치> result <r> by the stored-field rule (store <위치> := <c>) | none
   ```

   문맥의 차례는 사실의 호출 site 차례다. `rests on` 은 그 문맥의 결론이 기대는 미대조 항목(끝의 종료 호출의 P0 항목과 프레임워크 항목을 포함), `premises` 는 사실로 보이지 못해 적어 둔 것(필드 단위 규칙만 선 완전성, 다른 행위자의 writer 없음 등)이다. 필드 저장값으로 호출 결과를 정하는 규칙은 판정 규칙 8.13 의 17번 그대로이고, "저장과 호출 사이"는 함수의 제어 흐름 사실의 경로 조건으로 본다. 다른 상수로 바꿔 돌려주는 틀(`return-const`)은 따라간다. 머리에 한 줄:

   ```
   rules with decided call results: relations rules revision 14 (...): open effects by reason: <까닭> <수>; ...; call sites decided by the stored-field rule <n> (rows <m>)
   ```

5. 이름 공간 블록, 종류(partition 값)마다:

   ```
   stored-name buffer [partition <v>]: <Type.field> extent <n>; writers <w>: create copy <위치> in <함수>; clear <위치> in <함수> [fill <c>]; other <위치> in <함수> value=<값 종류>; ...; FIELD_STATUS per-field=<상태> type-level=<상태>; causes: <cause>: discharged (<사실과 수>) | not discharged; ...; address uses <u>: <callee> parameter <k> at <위치> [body: followed, <그 안의 쓰임> | library: <항목의 상태> | no body]; ...; complete | not complete (<까닭>)
   lifetime end [partition <v>]: clear-slot <위치> in <함수>: free-record condition shown (<조건>; <줄들>) | not shown (<까닭>)
   ```

   버퍼 필드는 이름 포인터 필드에 저장되는 주소의 배열 필드다(WRITERS 의 그 저장의 값). `causes` 는 그 필드의 FIELD_STATUS 가 `DISABLED` 인 원인 깃발 하나하나다(`array-element-stores-not-collected`, 주소 탈출, 형 없는 쓰기 등, §5h·§5s). `complete` 는 FIELD_STATUS 가 완전하고 writer 가 생성의 복사와 지우기뿐이며 주소의 쓰임이 모두 적혔을 때만 쓴다. 원인을 모두 풀었으나 FIELD_STATUS 가 `DISABLED` 면 `not complete (FIELD_STATUS disabled; every cause discharged)` 다. 이름 포인터 필드의 writer 줄(개정 7)은 그 writer 의 함수가 속한 종류로 나눈 `writers [partition <v>]:` 줄을 더한다.

**order rules revision 8: 완전한 쌍 파일.** `kz-static order <facts> [-o OUT] [--edges TSV] --pairs FILE`. 글 출력과 `edges.tsv` 는 도구 줄 말고 바뀌지 않는다(글의 `listing:` 줄의 인쇄 한도도 그대로). 쌍 파일은 탭으로 나눈 글이고 줄 수 한도가 없다:

```
kz-order-pairs 1
facts <64 hex>
tool kz-static tool_sha256=<64 hex> tool_source=<64 hex>
relations <relations 의 rules 줄들을 "; " 로 이은 것>
order <order 의 rules 줄들을 "; " 로 이은 것>
domain every creator and writer the relations rows name (creator lines; not-a-pair lines: key constants differ, probe-create; writers of path B and C); a pair in one thread has the value same_thread
columns row_key<TAB>path<TAB>use_pos<TAB>use_function<TAB>use_root<TAB>other_kind<TAB>other_pos<TAB>other_function<TAB>other_root<TAB>value<TAB>edges<TAB>premises<TAB>row_line
<쌍마다 한 줄>
rows <행 수> pairs <쌍 수> hard_before <n> conditional_before <n> fixed_early_proved <n> fixed_early_candidate <n> unordered <n> unknown <n> same_thread <n> no_pair <n>
end
```

- `row_key`: 사용 지점 키(§5l). `use_root`·`other_root`: 행위자의 뿌리 함수와 그 종류(`script-entry`, `process`, …)를 글 출력과 같은 글로. `other_kind`: `creator`, `creator-not-a-pair(key constants differ)`, `creator-not-a-pair(probe-create)`, `writer`. `row_line`: 그 생성자·writer 를 이름 댄 relations 글의 줄 종류.
- `value`: `hard_before`, `conditional_before`, `fixed_early_proved`, `fixed_early_candidate`, `unordered`, `unknown(<까닭>)`, `same_thread`. 글 출력의 값과 같은 뜻이고 `same_thread` 만 새 값이다: 두 위치의 뿌리가 같은 스레드다(이 개정은 한 스레드 안의 차례를 값으로 말하지 않는다). 뿌리가 여럿인 사용·생성자는 (사용 뿌리, 다른 쪽 뿌리) 짝마다 한 줄이다.
- `edges`: 그 값을 준 경로의 간선 id 들(`edges.tsv` 의 id, 쉼표; 없으면 `-`). `premises`: 그 간선들의 전제(`; `; 없으면 `-`).
- 생성자도 writer 도 이름 붙지 않은 행은 `other_kind` 부터 `-` 이고 `value` 가 `no_pair(<까닭>)` 인 한 줄이다. 그래서 모든 행이 파일에 있다.
- 읽는 쪽의 점검: `facts` 가 계획의 facts 와, `tool_sha256` 이 relations 머리와 같다; 글 출력에 적힌 `pair:` 줄마다 쌍 파일에 같은 값의 줄이 있다; 꼬리 줄의 수가 줄 수와 같다. 영역 안의 쌍에 줄이 없으면 파일이 완전하지 않은 것이다. 앞 도구의 글 출력에서 잰 것 [예비: `records/closure-texts-20261118/out/p5_pairs.txt`]: 쌍 24,796 / 24,796 / 8,739 / 4,406 / 4,395(다섯 빌드, `pairs:` 줄의 합; 다른 스레드의 생성자·writer 만), 적힌 `pair:` 줄 1,988 / 1,988 / 1,607 / 631 / 620.

**5x 덧붙임 (2026-11-19; 그것을 내는 코드보다 먼저 커밋함).** 코드를 쓰며 위 형식에서 정해지지 않았던 것을 정한다.
- order 글의 도구 줄은 `relations` 줄 바로 뒤에 둔다(order 글에는 `rules` 줄이 없고, 첫 줄이 order rules 개정을, `relations` 줄이 relations rules 개정을 이름 댄다). relations 글은 위대로 `rules` 줄 바로 뒤다. 도구 줄은 명령(`kz-static relations`, `kz-static order`)이 쓴다; 같은 사실을 안에서 읽는 다른 명령(계획 생성기, order 가 안에서 만드는 relations 행)은 그 줄을 내지 않는다.
- 쌍 파일의 `other_kind` 에 `creator-not-a-pair(same actor only)` 를 더한다: relations 의 `not a pair: same actor only (program order): …` 줄의 생성자다. 영역("같은 스레드의 생성자")에 이미 들어 있던 것이고 이름만 없었다. `not a pair: not a probe-create although …` 줄은 쌍 파일의 줄을 만들지 않는다(그 생성자는 같은 행의 다른 줄, `creator` 나 다른 `not a pair`, 에도 적힌다).
- `use_root` 와 `other_root` 가 같은 스레드면 값은 늘 `same_thread` 다. 글 출력은 그대로라서, 경로 B·C 의 글에 `unknown: one thread: …` 로 적힌 쌍은 쌍 파일에서 `same_thread` 다(읽는 쪽의 "같은 값" 점검은 이 짝을 같은 것으로 본다). 경로 A 의 글은 같은 스레드의 생성자를 쌍으로 세지 않으므로 그 줄은 쌍 파일에만 있다.
- 뿌리에 닿지 않는 쪽이 있는 쌍은 `use_root` 또는 `other_root` 가 `-` 이고 값이 `unknown(no root reaches <함수>)` 다(글 출력의 `unknown: no root reaches …` 와 같은 쌍).
- 꼬리 줄의 `pairs` 는 `no_pair` 가 아닌 줄의 수이고, 값마다의 수의 합이다. `unknown` 은 까닭과 상관없이 센다.
- 쌍 파일의 쌍은 글 출력을 다 쓴 뒤에 계산한다. 닫힘을 따라가며 HB-seq 단계 간선이 새로 생길 수 있는데(쌍 파일에만 있는 쌍, 인쇄 한도 밖의 쌍), 그것이 글의 `HB-seq steps the closures followed` 수나 `edges.tsv` 를 바꾸지 않게 하려는 것이다. 그래서 `edges` 칸의 id 가운데 `edges.tsv` 에 없는 것이 있다: 그런 간선은 쌍 줄 뒤, 꼬리 줄 앞에 `edge<TAB><edges.tsv 의 열한 칸>` 줄로 한 번씩 적고, 꼬리 줄 끝에 ` edges_not_in_tsv <n>` 을 더한다. 읽는 쪽은 `edges` 칸의 모든 id 가 `edges.tsv` 나 이 `edge` 줄에 있는지 본다.

## 6. core-cpu1 결과 [M]

`ctest -L static` 의 `static_golden_core_cpu1`(golden `tests/static/data/core_cpu1.golden`)이 basis → extract → 아래 수치를 매번 다시 만든다. 추출 시간은 이 VM 에서 약 4.5 초(한 번 측정, 비용 주장 아님).

| 항목 | 값 |
|---|---|
| 번역 단위 / 함수 / 호출 지점 | 168 / 1591(본문 1409, 시스템 헤더 54 건너뜀) / 5948 |
| 간접 호출 | 54, 해결 47, 미해결 7 |
| spawn 호출 | 4, entry 모두 해결 (`OS_TaskCreate` in `CFE_ES_StartAppTask` → `CFE_ES_TaskEntryPoint`; `CFE_ES_CreateChildTask` ×3: background task, TIME 의 1Hz task 둘) |
| 루프 | 257: unroll 58, await 1, star-closed 198; table walk 3 해결 |
| untyped write 지점 | 프로그램에 99, 대상에 걸리는 것 0 |

**`CFE_SB_Global_t.AppId` 의 writer**: 3 개.
1. `cfe_sb_init.c:50:5` `CFE_SB_EarlyInit` 의 `memset(&CFE_SB_Global, 0, …)`: 값 0, `early-init`, record 통째.
2. `cfe_sb_task.c:138:5` `CFE_SB_AppInit` 의 `CFE_ES_GetAppID(&CFE_SB_Global.AppId)` out 포인터 저장 둘: `cfe_es_api.c:754`(guard `AppIdPtr != NULL ∧ AppRecPtr != NULL`, 유효 ID), `:759`(guard `AppIdPtr != NULL ∧ AppRecPtr == NULL`, `CFE_ES_APPID_UNDEFINED`).

INIT: `cfe_sb_task.c:40:17` 의 0 초기화 전역. **A-TYPE 은 0 건**(untyped 0, escape 0, cast 0, union 0, unknown-effect 0)이라 writer 집합 상태는 `writer-set-complete` 이고 DISABLED 가 아니다. 다른 추출 대상 5개(App 레코드의 AppId·AppState·Type, PSP ShutdownReq, EVS AppID)도 모두 complete. 이 "complete" 는 §4.1 의 검사가 찾는 것이 없다는 뜻이고, §8 의 가정 위에서만 완전성이다.

2026-10-16 검토 반영 뒤 다시 확인한 것 [M]: 대상 형에 닿는 포인터가 본문 없는 함수·미해결 간접 호출·가변 인자 위치로 가는 곳 **0**(요약의 "unknown-effect passes"; `AppRecPtr` 같은 포인터 인자도 이 검사에 든다), 대상 형을 const view 로 바꾸는 cast **0**, memset 계열의 크기 검사로 늘어난 writer **0**, inline assembly **0**(번들의 cfe·osal·psp 에 `asm` 없음). table walk 의 image 확인: `GLOBAL_CONFIGDATA` 는 `typedef const struct {...} Target_ConfigData`(`cfe/cmake/target/inc/target_config.h:159`) 라서 const 이고, `CFE_CORE_MODULE_OBJTAB` 와 원소 객체(`CFE_ES_ModuleEntry` 등)도 const 정의라 **선언된 가정 0 개**로 펼친다. (검토 지적은 정의 줄 `target_config.c:245` 만 보고 non-const 로 읽었으나 const 는 typedef 에 있다. 확인 자체는 구현했고 fixture `init_live` 가 const 아닌 경로를 시험한다.)

**derives 와 gdb 기록의 대조(정적 호출 지점 수준)**: `CFE_EVS_SendEventWithAppID` 의 AppID 인자 59 행 중 39 행이 `CFE_SB_Global_t.AppId` 에서 copy 없이(hops 0) 온다. 파일별 `cfe_sb_api.c` 34, `cfe_sb_priv.c` 1, `cfe_sb_task.c` 4. `results/R2_gdb_v701.md` §6 이 소스를 다시 세어 적은 "SB AppId 를 넘기는 정적 SB 호출 지점 39(34/1/4)"와 **같다**. gdb 실행에서 공개 전에 실행된 위반 지점 셋(`cfe_sb_api.c` L277, L1185, `cfe_sb_priv.c` L786, 인자의 줄)은 모두 이 39 안에 있다. gdb 의 24 는 한 번 부팅의 **동적 발생 수**이고 3 개 정적 지점에서 나온다. 정적 지점 수와 비교할 수 없으며 이 추출은 어느 쪽 수도 주장하지 않는다. 나머지 20 행 중 19 는 TBL 의 identity 필드 `CFE_TBL_Global_t.TableTaskAppId` 에서 온다(E-TBL 쪽).

**도달성**(정적 호출 그래프, 해결된 간접 대상만 간선, 미해결 7 개는 간선 없음): spawn 간선 없이 `CFE_ES_TaskMain`·`CFE_EVS_TaskMain` 에서 각각 39 중 19, `CFE_SB_TaskMain` 22. 이는 over-approximation 이고 초기화 단계에 한정한 수가 아니다.

**핵심 직렬화의 루프**:
- `CFE_ES_CreateObjects` 의 두 table walk(`cfe_es_start.c:752`, `:776`): 각 10 회 unroll. EarlyInit 순서 CONFIG, ES, EVS, FS, SB, TBL, TIME(MSG, SBR, RESOURCEID 는 EarlyInit 이 NULL 이라 건너뜀). task 순서: 반복 1, 2, 4, 5, 6 에서 `CFE_ES_StartAppTask` 의 task-main 인자가 `CFE_ES_TaskMain`, `CFE_EVS_TaskMain`, `CFE_SB_TaskMain`, `CFE_TBL_TaskMain`, `CFE_TIME_TaskMain`(각각 나머지 조건 `AppRecPtr != NULL`), 나머지 5 반복은 `TaskMain == NULL` 의 `continue` 로 건너뜀. 이것이 생성된 core 목록이 정하는 core task 순서다.
- `CFE_ES_StartAppTask` 의 매개변수 2 는 task-main(VIA_ENTRY: `CFE_ES_TaskEntryPoint` 의 `cfe_es_apps.c:639` 간접 호출).
- `CFE_ES_MainTaskSyncDelay`(`cfe_es_start.c:871`): **await**. 술어 counter-zero: `forall i 32 (not (and (call-result CFE_ES_AppRecordIsUsed (seq local:AppRecPtr)) (lt field:CFE_ES_AppRecord_t.AppState param:AppStateId#0)))`, cursor `AppRecPtr` 가 `CFE_ES_Global_t.AppTable`(길이 32) 위를 지남. L1 (ii) one-lock-region, L1 (i) 관련 위치 6, 본문에서 도달하는 함수 93 에서 쓰기 없음.
- `CFE_ES_WaitForSystemState`(`cfe_es_api.c:585`): 직접 시험 `not (SystemState < MinSystemState)`, `volatile sig_atomic_t` 한 번 읽기(atomic-read, L1 (ii) 성립). 그러나 `CFE_ES_Global_t.SystemState` 는 프로필의 추출 대상이 아니라 A-TYPE 목록이 없으므로 L1 (i) 은 **결정하지 않음**(star-closed, unmodelled-sync). 이 필드를 프로필의 위치·bind 로 올리면 다시 판단할 수 있다. R2 경로(R19)는 `MainTaskSyncDelay` 를 쓴다.
- 그 밖의 poll 루프 4 개는 unmodelled-sync 이고 이유를 적는다(예: `CFE_ES_GetTaskFunction` 은 잠금 안에서 세운 지역 flag `ReturnCode` 를 시험하므로 SPEC §4.3 의 형태가 아니다).

## 7. 시험 [M]

| 시험 | 확인하는 것 |
|---|---|
| `static_fixture_compile_*` | 고정 도구 C 파일이 빌드 컴파일러로 `-std=c99 -pedantic -Wall -Wextra -Werror` 통과(`clobber.c` 는 일부러 객체 끝을 넘어 쓰므로 컴파일 검사에서 뺀다) |
| `static_golden_{writers,outptr,derives,loops,spawn,atype,clobber,wrappers,flowfacts,flowfacts_np,argflow,argflow_np}` | 고정 도구의 facts text view golden, 두 번 실행 바이트 동일, 컨테이너 검증 |
| `static_golden_core_cpu1` | 고정 번들의 core-cpu1: basis 항목, 요약, SB identity 필드의 INIT·writer·A-TYPE·out 포인터, 함수별 쓰기, poll·walk 루프의 규칙, table walk 순서, spawn 매개변수, image, 간접 대상, derives 39 의 목록·파일별 수·gdb 기록 지점 대조, 도달성. 번들이 없으면 건너뜀(77) |
| `test_static_unroll` | 부분 평가, 반복 펼치기(taken/skipped/residual, 인자·callee 값, spawn 표시), task-main 매개변수(DIRECT/VIA_ENTRY), 루프 규칙(각 실패 이유와 A-TYPE 상태에 따른 untyped write 처리), table walk image 확인(const 고리, 형 있는 쓰기, 객체 주소, 원소 필드), finalize 의 unknown-effect 결정(본문 없음, 가변 위치, 미해결 간접 호출, 해결된 간접 호출, 데이터 함수) |
| `test_static_{expr,flow,recipe,gitrev,link,cli}` | 식 pool, 흐름 그래프·table walk, recipe 문법(`module`, 범위 단어, `module_file` 과 그 오류), commit 읽기, link set, CLI(basis→extract 합성 프로그램; 실행 파일 + global 라이브러리 + local 앱 + 닫힘 밖 라이브러리의 닫힘: 앱이 out 포인터로 넘긴 필드가 실행 파일 본문으로 writer-set-complete, 늦게 적재된 라이브러리의 결합 차이 deviation, 적재 파일과 link 출력이 다르면 묶지 않음) |

고정 도구가 덮는 경우: 필드 writer(대입, 증감, 통째, memset, guard), A-TYPE 으로 DISABLED 되는 대상(cast, escape, union, untyped), out 포인터(중첩, 두 번 저장, escape), derives chain, 루프 탈출식(poll, 예산, counted, 중첩, table walk), counter-zero 술어와 cursor(`sync_delay`), 루프 조건이 counter 를 읽어 거부되는 경우, 본문이 시험 필드를 써서 L1 (i) 실패, 시험 필드가 추출 대상이 아니라 L1 (i) 미결정(`poll_untargeted`), 조건·예산이 섞인 탈출, `sig_atomic_t` 원자 읽기, spawn 지점(직접, 매개변수, 표, 미해결)과 trampoline 을 통한 task-main.

golden 을 의도적으로 바꿀 때: `KZ_GOLDEN_UPDATE=1 ctest --test-dir <빌드> -R static_golden` 후 `tests/static/data/` 의 diff 를 읽는다.

## 7a. 파생(계측) 빌드의 basis B′ (2026-10-26, 선택 문서 §4의 4, SPEC §1.7 "A configuration variant is a new basis B′") [D][M]

**무엇을 바꿨나** [D]. 파생 빌드의 compile DB에는 빌드 컴파일러의 plugin 옵션(`-fplugin=<file>`, `-fplugin-arg-<name>-<key>[=<v>]`)이 있다. Clang은 GCC plugin을 적재하지 못해 그 명령을 읽지 못한다(`clang -fplugin=…/kzgcc.so`: "unable to load plugin … undefined symbol: _Z24gt_ggc_mx_lang_tree_nodePv") [M]. 그래서:

- **기록**: basis의 TU 표는 명령을 실행된 그대로 둔다(옵션 포함; TU 명령 해시도). plugin 옵션이 있는 빌드에만 다음 항목이 붙는다(없는 빌드의 basis는 예전과 같음): `instrumentation.units_with_compiler_plugin`, `instrumentation.compiler_plugin_options[k]`(서로 다른 옵션 묶음마다, 단위 수), `instrumentation.plugin_file`(경로·sha256, FILE_HASHES에도), `front_end.arguments_removed`. 모두 정책 `deviation`이고 deviation 레코드 하나("build-compiler plugin in the recorded commands", 피할 수 있음 = no)를 가리킨다. 그 효과 문장은 "front end는 옵션을 뺀 명령으로 읽는다. 그것이 빌드 컴파일러가 구문 분석한 프로그램이라는 것은 전처리·구문 분석을 바꾸지 않는 plugin에서만 성립한다(kzgcc는 pass 셋과 단위 시작·끝·함수 구문 분석 끝 callback만 등록하고, 마지막 것은 런타임 함수 정의를 보고 계측을 거부할 뿐이다; pragma·attribute·매크로를 더하는 callback은 없다, `instrument/gcc/kzgcc.cpp` `plugin_init`·`finish_parse_function`). 다른 plugin이면 확인되지 않았다. 빌드된 객체에는 plugin이 넣은 것이 있고 facts는 그것을 기술하지 않는다"이다.
- **읽기**: front end(`extract_units`, 상수 평가)에는 옵션을 뺀 명령을 준다(`front_end_command`, `lib/llvm/ast_run.cpp`).
- **link 구성원**: 어느 compile 명령도 만들지 않는 link 구성원(예: 빌드 밖에서 link한 계측 런타임)을 `link.members_without_command`로 적는다(프로그램 안에 있으나 facts를 읽지 않음). 예전에는 경고로만 나왔다.
- 시험: `test_static_cli`의 합성 프로그램에 plugin 옵션을 붙인 compile DB로 basis·extract를 하고, 항목·deviation, 상수 평가, facts 섹션이 옵션 없는 명령의 facts와 같음(머리의 basis 해시·부모와 basis를 이름 붙이는 note만 다름), compile 명령 없는 link 구성원 기록을 본다 [M].

**C1에서 B와 B′ 비교** (파일럿·개발, 주 결과 아님; 산출물 `/home/user/work/instr/gcc/d1_basis/`, 저장소 밖, `sha256.txt`) [M].

- 절차: 단계 C와 같은 스크립트(`instrument/gcc/scripts/build_c1.sh prepare|build`)로 C1 post 1df262a의 사본을 새 작업 공간에 만들고, 기준 빌드(`ref`)와 파생 빌드(`instr`, plugin sha256 `4bfcb12b…`, manifest `8dbdd2e8…`, 단계 C와 같은 파일)를 **같은 경로**에서 차례로 빌드했다(경로 문자열 차이 D1을 없애려고; 단계 C 방식). 각 변형이 그 경로에 있을 때 `/home/user/work/procrace/integ_20261025/closures.sh`의 c1 post 1df262a tx와 같은 프로필·recipe·extract 인자로 `kz-static basis`·`extract`를 돌렸다(`closure_c1.sh wref|derived`). 원래 기준 트리(`c1_build/post/1df262a`)도 같은 인자로 읽어 비교에 썼다(쓰지 않음).
- 객체: 261개 중 253개 바이트 동일. 다른 8개 = 계측 단위 6개(binding 파일이 있는 `cfe_es_api.c`, `cfe_es_apps.c`, `osapi-task.c`, `os-impl-tasks.c`, `cf_cfdp.c`, `bp_flow.c`) + `cfe_build_env_table.c.o` 2개(tx, rx; 빌드 시각 문자열, 단계 C의 D4 — 기준 빌드 둘 사이에서도 다름). 단계 C의 같은 비교(`c1/objects_{ref,instr}.sha256`)는 6개였다(두 빌드가 같은 시각 문자열).
- facts(B 사본 대 B′): **섹션 내용이 같다.** 다른 것은 머리의 basis 해시·부모, basis를 이름 붙이는 note(`extract.units_from`), FILE_HASHES의 `cfe_build_env_table.c` 해시 하나(D4)뿐이다(`facts_wref_vs_derived.diff` 14줄). writer·A-TYPE 줄(위치 7개)은 원래 기준 트리의 facts와도 같다. 원래 트리와의 바이트 대조는 절대 경로(빌드 디렉터리의 생성 파일, 정렬 순서)가 달라 의미가 없다.
- basis(B 사본 대 B′): TU 177개의 flags(plugin 옵션), link 입력 +3(`libkzrt-posix.a`, `libkzrt.a`의 whole-archive 구성원과 시작 객체 `kz_rt_posix_auto.c.o`; `link.members_without_command = 3`), 실행 파일 export 1101 → 1119, cf.so·bp.so의 미정의 기호 +4(hook 함수, core-tx에 묶임), 단위 사이 참조 196 → 204, binary·모듈 해시, compile DB·link.txt·생성 표 해시, 위 instrumentation 항목. 런타임은 스레드를 만들지 않는다(`runtime/src`에 `pthread_create`·`OS_TaskCreate` 없음) [F].

**"계측 단위 밖의 객체가 바이트 동일할 때만 기준 facts를 재사용한다"는 규칙의 판단** [I, 위 [M]에 근거]:

1. 재사용의 근거가 되지 못한다. facts는 front end의 입력(소스 해시, plugin 옵션을 뺀 단위별 명령, front end가 읽은 생성 파일, 상수, 시작 스크립트, 모듈 닫힘·단위 집합)의 함수이고 객체는 그 뒤의 산출물이다. (a) 계측 단위 6개는 객체가 다르지만 facts는 같다(plugin은 구문 분석 뒤 GIMPLE에서만 코드를 바꾼다). (b) 기준 빌드 둘 사이에서도 `cfe_build_env_table.c.o`가 다르다(빌드 시각). (c) R2(`-g`)에서는 객체 1,335개가 **모두** 원시 바이트로 다르다(DWARF producer 문자열에 plugin 옵션), `--strip-debug` 뒤에도 276개가 다르다(계측 단위 8 + D4 1 + gcov stamp가 빌드마다 다른 단위 시험 coverage 객체 267, `docs/instr_candidate_gcc.md` §3). 바이트 동일 규칙은 R2에서 재사용을 모두 막는다.
2. 객체 동일성이 알려 주는 것은 계측하지 않은 단위의 **코드 생성**이 같다는 것뿐이고, 이는 F0식 바이너리 위치(파일 오프셋)에 해당한다. 소스 수준 facts의 조건이 아니다.
3. facts가 다루지 않는 차이는 객체 규칙과 무관하게 남는다: 런타임 구성원과 hook 호출(+8 참조). facts에 없으므로 B′의 facts가 B와 같아도 "실행 파일 전체"를 기술하지 않는다.
4. 그래서 **결정 [D]**: 기준 facts를 객체 비교로 재사용하지 않는다. 파생 빌드마다 그 compile DB로 B′와 facts를 직접 뽑는다(C1 tx: basis+extract 약 6 s). 기준 facts와의 대조는 위처럼 기록한다(같은 경로 사본 기준). 실행 기록의 부모 basis는 **그 실행을 만든 빌드**의 B′여야 한다: 단계 C의 파일럿 dump는 그 빌드 트리가 지워져 B′를 줄 수 없다(같은 스크립트로 다시 빌드해도 `cfe_build_env_table.c` 때문에 실행 파일 해시가 다르다: 단계 C `21f82eca…`, 이번 `4a17b154…`). D7의 실행은 빌드 직후 B′를 뽑고 같은 실행 파일로 돈다.
5. 열린 것 [U]: R2 파생 빌드의 B′ 추출은 이번에 하지 않았다(빌드 트리가 없음; 같은 코드 경로이며 `-fplugin-arg-kzgcc-skip-coverage`도 plugin 옵션으로 기록·제거된다).

## 8. 한계와 남은 일

**writer 집합 완전성(`writer-set-complete`)이 기대는 가정** [D]. 상태 `writer-set-complete` 는 §4.1 의 검사(형 있는 writer 수집, A-TYPE 의 cast·escape·union·untyped·unknown-effect)가 아무것도 찾지 못했다는 뜻이고, 다음 가정 위에서만 writer 집합이 완전하다. 증명된 성질이 아니다.
- G1. 본문 없는 코드(시스템 라이브러리, 시스템 헤더의 함수)는 프로그램의 전역을 **이름으로** 쓰지 않는다: 받은 포인터를 통해서만 쓴다(unknown-effect 는 이 경로만 본다). 실행 중 적재되는 앱(.so)이 cFE 전역을 이름으로 쓰는 경우는 닫힌 프로그램(core-cpu1) 밖이다. recipe `module` 로 묶은 닫힘(§3.1)에서는 시작 스크립트가 적재하고 link 명령이 있는 모듈이 닫힘 안이고, 밖에 남는 것은 link 명령이 없는 모듈과 시스템 라이브러리다.
- G2. 상수가 아닌 크기의 데이터 함수 쓰기는 둘러싼 객체 안에 머문다(벗어나면 C 에서 정의되지 않은 동작). `T *` 목적지에 상수가 아닌 크기는 `T` 배열 안에 머문다.
- G3. 데이터 함수 요약(C 라이브러리 12 개: 목적지 인자만 쓴다, `%n` 없음)은 사람이 넣은 표다(`spec.cpp`, BASIS 에 `researcher` 로 기록). Φ 데이터로 옮기는 일은 K1 에 남은 일이다(아래).
- G4. 포인터 도달은 형으로 따진다(필드 기반): `void*`·`char*` 필드는 아무 형에도 닿지 않는다(그 형으로의 변환이 cast hit 이기 때문).

그 밖:
- **증명하지 않은 것**: await 규칙은 L1 의 **전제**를 추출된 사실 위에서 확인한 것이고 L1 자체는 증명하지 않았다. 인증서에 쓰려면 lemma 의 증명 상태(`unproved` 등)를 붙여야 한다(K4b, PS3).
- L1 (i) 의 관련 위치 집합은 술어가 직접 읽는 필드와 추출 대상이다. 술어가 부르는 함수가 읽는 비대상 필드는 빠진다(보수적이지 않을 수 있는 지점: 그런 필드가 생기면 추출 대상으로 올려야 한다). `MainTaskSyncDelay` 의 `CFE_ES_AppRecordIsUsed` 는 `AppId` 를 읽고 이 필드는 추출 대상이다. L1 (ii) 쪽은 호출 결과가 있으면 atomic-read 를 거부하고 one-lock-region 은 호출이 잠금 구간 안일 때만 인정한다(§5.2). 호출이 잠금 구간 안이라는 것은 같은 함수 본문 안의 lock/unlock 호출 순서로만 판단한다(callee 안에서 잠금을 푸는 경우는 보지 않는다). 대상에 무관한 규칙(술어가 직접·부르는 함수를 거쳐 읽는 필드만, 각 writer 집합의 완전성 요구)은 새 섹션 AWAIT_L1 에 둔다(§5d); FACT_LOOPS 는 그대로다.
- 호출 그래프의 미해결 간접 호출 7 개는 간선을 더하지 않는다. 그런 호출에 닿는 루프는 `mod-incomplete` 로 결정하지 않는다. writer 집합에 대해서는 정보용이다(§4.1).
- task-main 매개변수와 간접 대상은 field 기반 may-flow 다(인스턴스 대응 없음).
- table walk 의 image 확인은 형 있는 쓰기만 본다. const 아닌 고리를 포인터·cast·본문 없는 코드로 바꾸는 쓰기는 배제하지 않으며, 그래서 그 경우는 선언된 가정으로 센다(core-cpu1 은 0 개).
- 하지 않은 SPEC 단계: §4.1 5(문서 주석), §4.1 6 의 IR 수준 link 와 `-fcallgraph-info` 대조(app-unit probe 는 소스 수준으로 구현, §3.1), `.tbl` 바이트 대조, §4.2 6–8(transition filtering, Φ 대응, key·allocation), §3.17 anchor audit, message provenance(§4.3).
- 프로필 문법에 지연·잠금 연산과 **데이터 함수 요약**을 선언하는 수단이 없다(K1 에 요청할 항목). 그때까지 지연·잠금은 명령줄 입력, 데이터 함수 표는 엔진의 내장 표이며 둘 다 사람이 넣은 입력으로 센다.
- core golden 은 지금 프로필의 대상·역할·종료·spawn 선언에 의존한다. 프로필이 바뀌면 다시 만들고 diff 를 검토해야 한다.
- writer 수집은 스칼라 배열 필드의 원소에 대한 저장(`x.arr[i] = v`)을 writer 로 잡지 않는다(fixture `lockset.c` 의 `fx_g.tasks[slot] = slot`; WRITERS·FUNC_WRITES·FIELD_STATUS 모두) [M]. 기존 섹션을 바꾸므로 단계 2a S4 에서는 고치지 않았다. 옛 FACT_LOCS 의 `writer-set-complete` 는 이 저장을 세지 않은 값이다. FIELD_STATUS 는 셋째 수정(§5h)부터 그런 필드를 두 규칙 모두 DISABLED 로 낸다.

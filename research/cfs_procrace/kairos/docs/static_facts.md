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
- 종료 코드: 0 정상, 1 분석 실패(번역 단위가 parse 되지 않음, 컨테이너 손상), 2 사용법·입출력 오류.
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

**F6 GLOBAL_IMAGES** [D]. ARGFLOW 의 `object` 와 PTR_FIELDS 의 대상·뿌리가 이름 붙인 전역마다, STATIC_IMAGE 와 같은 이미지 생성기(`flatten_initializer`, 같은 잎·종류·경로)로 만든 정적 초기값이다. table walk 와 무관하다. 스칼라 전역은 잎 하나, 초기값 없이 정의된 객체는 `zero-initialized` 한 행이다. 닫힌 프로그램 밖에 정의된 객체는 행이 없고 수만 센다. 잎이 매크로로 쓰였으면 거기 쓴 가장 바깥 매크로(`macro`)와 값의 첫 토큰이 쓰인 곳(spell 위치, 정의)을 적는다(시스템 헤더의 매크로는 적지 않는다). 잎이 레코드 필드이면 (`owner`, `field`)를 적고, 그 필드를 쓰거나 주소를 잡는 함수가 있으면 `field-written`(필드 기반), 그 형 레코드를 통째로 쓰면 `record-written`, 객체 자체를 쓰거나 주소를 잡으면 `object-written` 이다. 이 표시가 없으면 형 있는 쓰기로는 정적 값이 실행 중 값이다. 이미지 잎 수 상한(기존 4096, `--image-budget N`)은 레퍼런스가 정하지 않는 도구 매개변수다.

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
- 한계 [M]: writer 수집은 기존 기계와 같다. fixture `lockset.c` 의 `fx_g.tasks[slot] = slot`(스칼라 배열 필드의 원소에 대한 저장)은 WRITERS 에도 FIELD_STATUS 에도 writer 로 잡히지 않는다(FUNC_WRITES 에도 없다). 기존 섹션을 바꾸지 않으려고 고치지 않았다. §8 에 남은 일로 적는다.

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

**`kz-static relations <facts> [-o OUT]`** [D]. facts 컨테이너 하나를 읽어 텍스트를 낸다(`lib/llvm/relations.cpp`, "relations rules revision 1"; 개정 2에서 바뀐 규칙은 §5f). 필드·callee·사례 이름을 받지 않는다. 확장 사실(READS … AWAIT_L1)이 없는 facts 는 이유와 함께 거부한다. 같은 컨테이너는 같은 바이트를 낸다.
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

**P0 표 개정 2** (`p0.cpp`) [D]. 개정 1의 묶음을 통째로 채웠다: 스레드 생성 `thrd_create`, mutex `pthread_mutex_clocklock`·`mtx_lock/trylock/timedlock/unlock`, spin lock `pthread_spin_lock/trylock/unlock`(배타 획득이라 mutex 종류), 세마포어 `sem_clockwait`, 조건 변수 `pthread_cond_clockwait`·`cnd_wait/timedwait/signal/broadcast`, 잠자기 `usleep`·`thrd_sleep`, 종료 `thrd_exit`·`quick_exit`·`__assert_fail`·`__assert_perror_fail`(assert 매크로가 부르는 함수), 스레드 id `thrd_current`, thread-specific data `tss_get/set`·`pthread_key_create/delete`·`tss_create/delete`, cleanup handler `__pthread_register_cancel(_defer)`·`__pthread_unregister_cancel(_restore)`(예외 없이 컴파일한 단위에서 `pthread_cleanup_push/pop` 매크로가 부르는 함수), 동적 적재 `dlmopen`·`dlvsym`. 72항목, 항목마다 판 표기(POSIX.1-2017 / glibc 2.39, ISO C11 <threads.h> / glibc 2.39, `*_clock*` 은 glibc 2.39·POSIX.1-2024). 원문 대조는 개정 1과 같다(`pthread_create` 만). 새 종류 넷: `thread-specific-key-create`(`entry_param` = 소멸자), `thread-specific-key-delete`, `cleanup-push`, `cleanup-pop`. 어휘가 아니고 유도에 쓰지 않는다(relations 의 수명 규칙만 읽는다).
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
- writer 수집은 스칼라 배열 필드의 원소에 대한 저장(`x.arr[i] = v`)을 writer 로 잡지 않는다(fixture `lockset.c` 의 `fx_g.tasks[slot] = slot`; WRITERS·FUNC_WRITES·FIELD_STATUS 모두) [M]. 기존 섹션을 바꾸므로 단계 2a S4 에서는 고치지 않았다. 그런 필드의 `writer-set-complete` 와 FIELD_STATUS 의 complete 는 이 저장을 세지 않은 값이다.

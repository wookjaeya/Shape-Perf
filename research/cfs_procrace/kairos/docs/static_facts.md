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

- `show` 의 section: `basis files functions calls derives loops iters order init consts writers atype locs targets spawnparams image terminals outcalls fwrites`. `--loc Type.field`, `--callee NAME` 으로 거른다.
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

schema 버전: FACT_LOOPS 와 FACT_EXPRS 는 3, FACT_ATYPE 은 3(`unknown-effect` 종류), FACT_LOCS 는 2(`n_unknown_effect`), 새 섹션 37–39 는 1. writer 에 `partial-write`, 루프에 `static-image-assumed` 표시가 있다(revision 4). 바뀐 점은 `kz_facts.h` 머리 주석에 있다.

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
| `static_golden_{writers,outptr,derives,loops,spawn,atype,clobber}` | 고정 도구의 facts text view golden, 두 번 실행 바이트 동일, 컨테이너 검증 |
| `static_golden_core_cpu1` | 고정 번들의 core-cpu1: basis 항목, 요약, SB identity 필드의 INIT·writer·A-TYPE·out 포인터, 함수별 쓰기, poll·walk 루프의 규칙, table walk 순서, spawn 매개변수, image, 간접 대상, derives 39 의 목록·파일별 수·gdb 기록 지점 대조, 도달성. 번들이 없으면 건너뜀(77) |
| `test_static_unroll` | 부분 평가, 반복 펼치기(taken/skipped/residual, 인자·callee 값, spawn 표시), task-main 매개변수(DIRECT/VIA_ENTRY), 루프 규칙(각 실패 이유와 A-TYPE 상태에 따른 untyped write 처리), table walk image 확인(const 고리, 형 있는 쓰기, 객체 주소, 원소 필드), finalize 의 unknown-effect 결정(본문 없음, 가변 위치, 미해결 간접 호출, 해결된 간접 호출, 데이터 함수) |
| `test_static_{expr,flow,recipe,gitrev,link,cli}` | 식 pool, 흐름 그래프·table walk, recipe 문법(`module`, 범위 단어, `module_file` 과 그 오류), commit 읽기, link set, CLI(basis→extract 합성 프로그램; 실행 파일 + global 라이브러리 + local 앱 + 닫힘 밖 라이브러리의 닫힘: 앱이 out 포인터로 넘긴 필드가 실행 파일 본문으로 writer-set-complete, 늦게 적재된 라이브러리의 결합 차이 deviation, 적재 파일과 link 출력이 다르면 묶지 않음) |

고정 도구가 덮는 경우: 필드 writer(대입, 증감, 통째, memset, guard), A-TYPE 으로 DISABLED 되는 대상(cast, escape, union, untyped), out 포인터(중첩, 두 번 저장, escape), derives chain, 루프 탈출식(poll, 예산, counted, 중첩, table walk), counter-zero 술어와 cursor(`sync_delay`), 루프 조건이 counter 를 읽어 거부되는 경우, 본문이 시험 필드를 써서 L1 (i) 실패, 시험 필드가 추출 대상이 아니라 L1 (i) 미결정(`poll_untargeted`), 조건·예산이 섞인 탈출, `sig_atomic_t` 원자 읽기, spawn 지점(직접, 매개변수, 표, 미해결)과 trampoline 을 통한 task-main.

golden 을 의도적으로 바꿀 때: `KZ_GOLDEN_UPDATE=1 ctest --test-dir <빌드> -R static_golden` 후 `tests/static/data/` 의 diff 를 읽는다.

## 8. 한계와 남은 일

**writer 집합 완전성(`writer-set-complete`)이 기대는 가정** [D]. 상태 `writer-set-complete` 는 §4.1 의 검사(형 있는 writer 수집, A-TYPE 의 cast·escape·union·untyped·unknown-effect)가 아무것도 찾지 못했다는 뜻이고, 다음 가정 위에서만 writer 집합이 완전하다. 증명된 성질이 아니다.
- G1. 본문 없는 코드(시스템 라이브러리, 시스템 헤더의 함수)는 프로그램의 전역을 **이름으로** 쓰지 않는다: 받은 포인터를 통해서만 쓴다(unknown-effect 는 이 경로만 본다). 실행 중 적재되는 앱(.so)이 cFE 전역을 이름으로 쓰는 경우는 닫힌 프로그램(core-cpu1) 밖이다. recipe `module` 로 묶은 닫힘(§3.1)에서는 시작 스크립트가 적재하고 link 명령이 있는 모듈이 닫힘 안이고, 밖에 남는 것은 link 명령이 없는 모듈과 시스템 라이브러리다.
- G2. 상수가 아닌 크기의 데이터 함수 쓰기는 둘러싼 객체 안에 머문다(벗어나면 C 에서 정의되지 않은 동작). `T *` 목적지에 상수가 아닌 크기는 `T` 배열 안에 머문다.
- G3. 데이터 함수 요약(C 라이브러리 12 개: 목적지 인자만 쓴다, `%n` 없음)은 사람이 넣은 표다(`spec.cpp`, BASIS 에 `researcher` 로 기록). Φ 데이터로 옮기는 일은 K1 에 남은 일이다(아래).
- G4. 포인터 도달은 형으로 따진다(필드 기반): `void*`·`char*` 필드는 아무 형에도 닿지 않는다(그 형으로의 변환이 cast hit 이기 때문).

그 밖:
- **증명하지 않은 것**: await 규칙은 L1 의 **전제**를 추출된 사실 위에서 확인한 것이고 L1 자체는 증명하지 않았다. 인증서에 쓰려면 lemma 의 증명 상태(`unproved` 등)를 붙여야 한다(K4b, PS3).
- L1 (i) 의 관련 위치 집합은 술어가 직접 읽는 필드와 추출 대상이다. 술어가 부르는 함수가 읽는 비대상 필드는 빠진다(보수적이지 않을 수 있는 지점: 그런 필드가 생기면 추출 대상으로 올려야 한다). `MainTaskSyncDelay` 의 `CFE_ES_AppRecordIsUsed` 는 `AppId` 를 읽고 이 필드는 추출 대상이다. L1 (ii) 쪽은 호출 결과가 있으면 atomic-read 를 거부하고 one-lock-region 은 호출이 잠금 구간 안일 때만 인정한다(§5.2). 호출이 잠금 구간 안이라는 것은 같은 함수 본문 안의 lock/unlock 호출 순서로만 판단한다(callee 안에서 잠금을 푸는 경우는 보지 않는다).
- 호출 그래프의 미해결 간접 호출 7 개는 간선을 더하지 않는다. 그런 호출에 닿는 루프는 `mod-incomplete` 로 결정하지 않는다. writer 집합에 대해서는 정보용이다(§4.1).
- task-main 매개변수와 간접 대상은 field 기반 may-flow 다(인스턴스 대응 없음).
- table walk 의 image 확인은 형 있는 쓰기만 본다. const 아닌 고리를 포인터·cast·본문 없는 코드로 바꾸는 쓰기는 배제하지 않으며, 그래서 그 경우는 선언된 가정으로 센다(core-cpu1 은 0 개).
- 하지 않은 SPEC 단계: §4.1 5(문서 주석), §4.1 6 의 IR 수준 link 와 `-fcallgraph-info` 대조(app-unit probe 는 소스 수준으로 구현, §3.1), `.tbl` 바이트 대조, §4.2 6–8(transition filtering, Φ 대응, key·allocation), §3.17 anchor audit, message provenance(§4.3).
- 프로필 문법에 지연·잠금 연산과 **데이터 함수 요약**을 선언하는 수단이 없다(K1 에 요청할 항목). 그때까지 지연·잠금은 명령줄 입력, 데이터 함수 표는 엔진의 내장 표이며 둘 다 사람이 넣은 입력으로 센다.
- core golden 은 지금 프로필의 대상·역할·종료·spawn 선언에 의존한다. 프로필이 바뀌면 다시 만들고 diff 를 검토해야 한다.

# Kairos (접두어 `kz`)

cFS 애플리케이션·task 수준의 준비 상태(readiness)와 순서 위반을 판정하는 도구. 설계는 `../Architecture/spec/SPEC_FINAL.md`, 구현 계획은 `../plan/ROADMAP_20261015.md`, PI 결정은 `../Architecture/spec/PI_DECISIONS_RESOLVED_20261015.md` 에 있다.

이 트리의 현재 단계는 **M2(R2 사례의 최소 전체 경로)의 토대(K0)**이다: 레코드 형식, KZ 컨테이너 입출력, 해시, 정규 dump, 판정·신뢰 원장 레코드, 시험, lint. 근거 검사기와 판정 엔진은 **아직 없다**(아래 상태 표). 관측기(perf-sample 백엔드, K2)는 있다(`docs/observer.md`). 정적 추출층(`kz-static basis`/`extract`, K4a)은 있다(`docs/static_facts.md`; writer 집합의 `writer-set-complete` 는 그 문서 §8 의 가정 위에서만 완전성이다). 이 트리의 시험 값은 단위 시험 값이며 연구 조건이나 실험 결과가 아니다.

표기: **[M]** 이번 구현에서 실행해 확인, **[D]** 설계 결정, **[I]** 계획·미확인.

## 1. 디렉터리 구조

```
kairos/
  CMakeLists.txt             최상위. 각 구성요소 폴더의 CMakeLists.txt 가 있으면 추가한다
  cmake/KzSettings.cmake     경고, sanitizer, kz_add_unit_test(), kz_use_llvm()
  cmake/KzSourceIdentity.cmake  소스 identity(tool hash 의 재료): kz_source_identity(), kz_declare_sources(), kz_tool_identity()
  include/kz/schema/         레코드 배치의 유일한 정의 (X-macro): kz_schema.h + kz_raw.h, kz_facts.h, kz_cert.h
  include/kz/core/           libkz-core 의 공개 헤더
  lib/core/                  libkz-core: KZ 컨테이너(정규 문자열 풀 포함), SHA-256, CRC-32, 정규 dump, tool hash, 판정·원장 레코드 보조
  tools/kz-dump/             컨테이너를 정규 text 로 찍는 독립 도구 (`kz dump` 의 단독 형태)
  tests/unit/                libkz-core 단위 시험 (test_core_*.cpp 는 자동 등록) + 스키마 시험
  tests/golden/              정규 dump 의 golden 시험 (data/*.golden)
  tests/lint/                저장소 lint (kz_lint.cpp, data/)
  docs/schema.md             형식과 SPEC §2.3 대비 변경점
  docs/phi_reference.md      (K1) Φ 프로필 언어: 어휘·문법·의미, 평가기, 재생 엔진, 검사기 규칙, 정규형·해시, 출처 규칙, SPEC 과의 불일치
  profiles/cfs-v7.0.1/*.kzp  (K1) 판정용 Φ 프로필(V1 부분집합). 프레임워크 지식은 여기에만 있다
  tools/kz-phi/              (K1) 프로필 check·dump·hash·stats·verify-prov·replay
  tests/phi/                 (K1) 실제 프로필의 golden(정규 text, 해시, 합성 trace 재생 15개), CLI 시험, 인용 대조
  tests/property/            (K1) 평가기 성질 시험(추상 결과가 구체 결과를 포함함)
  lib/trace/                 (K2) perf 기록 해석, 병합·손실, 정체성 결합, 호출 짝짓기 — 순수 함수
  probe/                     (K2) anchor/ ELF·DWARF 줄 표·앵커, plan/ 관측 프로필·basis, loader/ perf_event_open·실행 컨테이너, scripts/ 연습 실행 드라이버
  tools/kz/                  (K2) CLI `kz`: anchor, observe, hits, replay(실행 컨테이너의 실제 기록 재생, docs/observer.md §3.4)
  tests/cfs-free/            (K2) 프레임워크 없이 도는 관측기 시험 + 목적 제작 C 프로그램 + golden
  profiles/cfs-v7.0.1/observe/  (K2) 관측용 프로브 계획
  docs/observer.md           관측기 사용법과 설계 세부
  docs/observer_smoke_run.md 실제 cFS 에 한 번 붙인 기록(조건, deviation, 개수; S1-R2 기능 확인 1/3, 미완)
  lib/llvm/                  (K4a) Clang 으로 읽는 정적 추출층: basis, 사실(writer, A-TYPE 과 unknown-effect, INIT, derives, 루프와 인증 뷰 규칙, table walk 펼치기와 image 확인, spawn)
  tools/kz-static/           (K4a) CLI `kz-static`: basis, extract, show, reach (-DKZ_BUILD_LLVM_TOOLS=ON 일 때만)
  tests/static/              (K4a) 고정 C 프로그램의 golden(7 개, `clobber.c` 는 읽기만), core-cpu1 golden, 단위 시험
  profiles/cfs-v7.0.1/basis/ (K4a) basis recipe(core-cpu1.kzb): link 명령, 생성 파일, 표, 시작 스크립트 규칙
  docs/static_facts.md       (K4a) 정적 추출층의 사용법, 사실의 뜻, core-cpu1 결과, 한계
  lib/predict/               (단계 D) kz predict: 모델(kzp-predict 1)·앵커·명시적 탐색·후보 재판정·보류 계획 (docs/predict.md)
  tests/predict/             (단계 D) 작은 프로필·모델의 예측 시험
```

Architecture/08_roadmap.md §2 의 나머지 폴더(`lib/static`, `lib/proofcheck` 등)는 해당 구성요소가 만들 때 같이 생긴다. **빈 틀은 만들지 않았다.** 구성요소는 자기 폴더에 `CMakeLists.txt` 를 두면 최상위가 자동으로 추가한다. 단위 시험은 `kz_add_unit_test()` 로 등록한다(이름은 `test_<구성요소>_*.cpp`; `test_core_*` 는 libkz-core 몫).

## 2. 빌드와 시험

빌드 트리는 저장소 **밖**, `/home/user/work/kairos-build/` 아래에 둔다(저장소에 올리지 않는다). 컴파일러는 시스템 gcc 13과 clang 18, 빌드 도구는 CMake와 Ninja.

```sh
SRC=/home/user/Shape-Perf/research/cfs_procrace/kairos
B=/home/user/work/kairos-build

# gcc
cmake -S $SRC -B $B/gcc -G Ninja -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
cmake --build $B/gcc && ctest --test-dir $B/gcc --output-on-failure

# clang
cmake -S $SRC -B $B/clang -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake --build $B/clang && ctest --test-dir $B/clang --output-on-failure

# gcc, ASan + UBSan (오류가 있으면 시험이 중단된다: -fno-sanitize-recover=all)
cmake -S $SRC -B $B/gcc-asan -G Ninja -DCMAKE_BUILD_TYPE=Debug -DKZ_SANITIZE=address,undefined
# clang, UBSan (이 환경의 clang 18 에는 compiler-rt 가 없어 trap 방식만 가능)
cmake -S $SRC -B $B/clang-ubsan -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
      -DCMAKE_BUILD_TYPE=Debug -DKZ_SANITIZE=undefined -DKZ_SANITIZE_TRAP=ON
```

옵션:

| 옵션 | 기본 | 뜻 |
|---|---|---|
| `KZ_WERROR` | ON | Kairos 코드의 경고를 오류로 본다 (`-Wall -Wextra -Wpedantic -Wshadow -Wundef -Wformat=2 -Wcast-qual …`) |
| `KZ_BUILD_LLVM_TOOLS` | **OFF** | `lib/llvm`, `tools/kz-static` 를 빌드한다. 켜면 `KZ_LLVM_BUILD_DIR`(기본 `/home/user/work/llvm-project/build`)의 고정된 LLVM·Clang 을 찾는다. LLVM 은 `kz_use_llvm(target)` 으로만 target 에 닿는다 |
| `KZ_SANITIZE` | (없음) | 모든 target 에 `-fsanitize=<값>` |
| `KZ_SANITIZE_TRAP` | OFF | UB 검사를 trap 으로 (runtime 라이브러리가 없는 clang 용) |

libkz-core, `tools/kz`, `lib/proofcheck` 는 LLVM 없이 빌드된다. 이는 lint(`lint_no-llvm`)가 지킨다: 이 디렉터리들의 헤더 include(`llvm/`, `llvm-c/`, `clang/`, `clang-c/`, `mlir/`)와 CMake 파일의 LLVM·Clang·MLIR 이름(대소문자와 경로 포함, 따옴표 안도)을 막고, `kz_use_llvm` 은 `lib/llvm`, `tools/kz-static` 의 CMake 파일(과 그 정의인 `cmake/KzSettings.cmake`)에서만 허용한다.

ctest 의 시험 이름과 라벨 (K0 토대의 32개 [M]; 관측기 시험은 이 표 뒤의 별도 표):

| 라벨 | 시험 | 확인하는 것 |
|---|---|---|
| unit | `test_core_sha256`, `test_core_crc32` | 공개 시험 벡터, 블록 경계 길이, 스트리밍 |
| unit | `test_core_schema` | padding 없음, 자연 정렬, SPEC 의 크기, C99·C11·C++ 배치 일치, 레지스트리·열거형·형식 일관성, 0 이 보수적 기본값, `safe`/`live` 필드 없음 |
| unit | `test_core_vocab` | 판정 레코드의 어휘(harm, consequence, outcome label, shape, class, unknown reason, reach, progress, tier, proof status …)가 SPEC §1.6·§1.10·§6.2 의 목록과 같음(값, 표시 이름, 순서). SPEC 이 퇴역시킨 낱말(`event_lost…`, `packets_lost…`)이 어느 열거형에도 없음 |
| unit | `schema_probe_{0,1,2}_{cxx,c99}`, `schema_bpf_compile` | padding 이 있는 레코드를 컴파일러가 실제로 거부함, `clang -target bpf` 에서 헤더 사용 |
| unit | `test_core_container` | 왕복, 안정된 해시, 호출 순서와 **문자열 intern 순서에 무관한** 결정적 writer, 정규 문자열 풀(정렬·중복 없음·참조되지 않은 문자열 제거), 0 padding, 문자열 참조의 경계(풀 끝, NUL 종결), 부모 kind 검사, 인자 검사, 파일 입출력 |
| unit | `test_core_container_corrupt` | **모든 바이트 변경·모든 절단**이 거부됨, 구조 규칙별 시험(CRC 를 다시 계산해 한 규칙만 어긴 이미지), 끝 padding(항상 존재하는 컨테이너 두 개로), 정규가 아닌 문자열 풀 일곱 가지(손으로 조립한 유효 파일) |
| unit | `test_core_verify` | 예약 필드 0, 참조 닫힘(부모만), 문자열 범위, 모든 레코드 종류의 패턴 왕복 |
| unit | `test_core_dump` | 필드 종류별 정규 dump 형식, 결정성, ASCII |
| unit | `test_core_verdict` | 기본 판정 셀(진행성 `unknown(not_checked)`), 불변식, 증명 상태 규칙(잘못된 상태는 `unproved`, `proved` 의 source 는 PROVENANCE 기록), 원장 집계·라벨, 서술 규칙(경계, 보존된 진술·witness, stall witness, 원장 대조) |
| unit | `test_core_identity` | tool hash 의 모든 입력이 해시를 바꿈, 필드 구분, **libkz-core 에 컴파일된 소스 identity 가 지금 트리의 파일로 독립 계산한 값과 같음** |
| unit | `test_core_kzdump` | `tools/kz-dump` 출력·종료 코드, `--identity`(tool hash 와 소스 identity 를 트리에서 다시 계산한 값과 대조) |
| unit | `source_identity_cmake` | `cmake/KzSourceIdentity.cmake`: 내용·이름·파일 집합에 의존하고 트리의 위치, 숨김 파일, 백업 파일에는 의존하지 않음 |
| golden | `golden_{layout,records,container_facts,container_run,container_verdicts,render,tool_hash}` | 모든 레코드의 배치, 모든 레코드의 dump, 표본 컨테이너 세 개의 dump 와 artifact 해시, 서술 문장, tool hash 의 정의(만든 입력으로) |
| lint | `lint_selftest`, `lint_{case-ids,retired-term,banned-files,no-llvm,no-generated}` | 아래 규칙. 자체 시험은 90개 사례 |

관측기(K2)의 시험은 `tests/cfs-free/` 에 있다(자세한 목록은 `docs/observer.md` §12) [M]:

| 라벨 | 시험 | 확인하는 것 |
|---|---|---|
| unit | `test_trace_decode_{perf,real,sites,merge}`, `test_trace_identity`, `test_trace_symbols` | 합성 ring 바이트와 커널이 실제로 쓴 바이트의 해석, 슬롯 적용, 병합·손실, 정체성 결합, 기호 조회 |
| unit | `test_probe_{elf_dwarf,anchor_compilers,profile,basis,record}` | ELF/DWARF 읽기와 앵커 해석(gcc·clang, DWARF 4·5 교차 확인; `addr2line` 이나 다른 컴파일러가 없으면 77 = 건너뜀), 프로필 문법(디코더 슬롯·짝 열쇠 검사 포함), basis, 조건 기록과 `/proc` 읽기 수 |
| unit | `probe_anchor_xcheck_script` | `probe/scripts/observe_cfs_smoke.sh --xcheck`(실행 뒤 앵커를 `addr2line`·`objdump` 로 교차 확인하는 단계)를 목적 제작 프로그램에서 확인 |
| unit, golden | `test_probe_golden` | basis 와 실행 컨테이너의 정규 dump(`tests/cfs-free/golden/`) |
| unit, integration | `test_probe_observe` | 목적 제작 C 프로그램에 실제 uprobe 를 붙인 통합 시험. root 가 아니면 77 로 끝나 건너뜀으로 보고된다. 약 20 초 |

프로필 언어와 평가기·재생 엔진(K1)의 시험 [M] (설명은 `docs/phi_reference.md` §13):

| 라벨 | 시험 | 확인하는 것 |
|---|---|---|
| unit | `test_core_phi_parser` | 어휘·구문 오류 71개를 줄:열과 함께, 식의 우선순위·출력의 고정점, 받아들이는 형태 |
| unit | `test_core_phi_check` | 검사기 규칙마다 한 번의 편집으로 오류를 일으킴(126개). 손으로 넣은·code-derived 기록의 `side caller` 와 그것을 인용한 appreq 를 거절(QT-4, QT-9) |
| unit | `test_core_phi_canonical` | 정규 text 의 고정점, 프로필 해시의 불변성(주석·공백·순서·파일 분할·정수 철자)과 변화(의미 편집 14가지; `when`/`filter` 보호 조건) |
| unit | `test_core_phi_allowed`, `test_core_phi_provenance` | 호출자 수준 Allowed 의 모든 가지(규칙 하나는 수용하지 않음; 손으로 넣은 caller 기록은 규칙으로 읽음), `stats` 의 호출자 수준 인용, 인용 본문 해시와 번들 대조, source 개정 고정(git 메타데이터 파일) |
| unit | `test_core_eval` | 평가기의 구체 의미(모든 구성)와 추상 domain 의 삼치 논리·구간·상태 집합 |
| property | `test_property_eval_agreement` | 생성한 식·상태·추상화에서 추상 결과가 구체 결과를 포함함(고정 시드, 약 96,000 평가). 일부러 틀린 추상화가 위반을 내는지도 확인. 잘못 만들어진 소유자(다른 종류·키 폭)를 섞은 상태 포함 |
| unit | `test_core_replay` | 효과 종류별 재생, 단언한 보호 조건이 거짓인 지점의 보고(반박·불일치)와 `filter`, 규칙 템플릿 4개의 간선, 발생의 상태 기계, 범위·종료, 가드·버전, trace text, 무작위 trace 400개에서 죽지 않고 결정적임 |
| unit | `test_core_realrecord` | 실제 기록 재생 모드(`docs/phi_reference.md` §7.6): 초기 상태 기본값 없음, ADR-055 의 쓰기 조건, 행위자 사이 순서, D2 요약(`PENDING`·`OPEN` → `INCONCLUSIVE(horizon-open)`), 반환 코드에 의한 고정과 분석 입력 오류, 파생 기록(사건 손실·지점 제거·늦은 시작)이 더 결정적이지 않음, 실행 관계(같은 task·같은 앱·다른 앱·코어·생성 관계·`inferred`·미정), P1 대안(반환 코드 고정 끄기) |
| unit | `test_trace_replay_out` (`tests/cfs-free`) | `kz replay --out` 출력 컨테이너의 왕복: 부모·basis, 요약·모델 id·적합성·정체성 인덱스, ACTOR_OWNERS 복사, 같은 입력 같은 바이트, 정규 dump(`docs/schema.md` §11) |
| unit | `test_realrecord_derived` (`tests/cfs-free`) | S1-R2 개발 실행 컨테이너의 실제 기록 재생과 파생 기록 71개(`docs/observer.md` §3.4) |
| golden | `phi_golden_*` (17개) | 실제 프로필의 정규 text·해시·통계, 합성 trace 12개로 만든 재생 결과 15개(규칙마다; 보호 조건의 반박, CleanUpApp 의 RESERVED 창 포함) |
| unit | `phi_profile_check`, `phi_profile_provenance`, `test_phi_cli` | `kz-phi check`, 인용 98개를 고정된 번들에서 다시 읽어 대조하고 source 4개의 개정 고정을 확인(번들이 없으면 건너뜀; 커밋되지 않은 수정은 확인하지 않음), CLI 출력·종료 코드 |

golden 을 의도적으로 바꿀 때: `KZ_GOLDEN_UPDATE=1 ctest --test-dir <빌드> -R phi_golden` 후 `tests/phi/data/` 의 diff 를 읽는다.

`kz-phi` (프로필 도구, libkz-core 만 링크):

```sh
$B/gcc/tools/kz-phi/kz-phi check  profiles/cfs-v7.0.1            # 읽고 검사 (디렉터리는 *.kzp 를 이름순으로)
$B/gcc/tools/kz-phi/kz-phi dump   profiles/cfs-v7.0.1            # 정규 text
$B/gcc/tools/kz-phi/kz-phi hash   profiles/cfs-v7.0.1            # 프로필 해시
$B/gcc/tools/kz-phi/kz-phi stats  profiles/cfs-v7.0.1            # tier 별 개수, 손으로 넣은 항목, declared_trusted
$B/gcc/tools/kz-phi/kz-phi verify-prov --root /home/user/work/procrace/cfs_ref/cFS profiles/cfs-v7.0.1
$B/gcc/tools/kz-phi/kz-phi replay --trace t.trace [--predicate 규칙=식] [--version 성분=버전] profiles/cfs-v7.0.1
```

`kz-dump` (canonical 형식 확인용 독립 도구, libkz-core 만 링크):

```sh
$B/gcc/tools/kz-dump/kz-dump <file.kz>              # 검증 후 정규 dump
$B/gcc/tools/kz-dump/kz-dump --check <file.kz>      # 검증만 (종료 코드 0/1/2)
$B/gcc/tools/kz-dump/kz-dump --no-verify <file.kz>  # 구조는 맞지만 내용 검사에 실패한 파일을 들여다볼 때
$B/gcc/tools/kz-dump/kz-dump --hash <file.kz>       # artifact hash (파일 전체의 SHA-256)
$B/gcc/tools/kz-dump/kz-dump --layout               # 스키마 배치와 fingerprint
$B/gcc/tools/kz-dump/kz-dump --identity             # 이 빌드의 tool hash 와 소스 identity 둘(libkz-core, 도구)
```

생성하는 쪽은 산출물의 BASIS 항목에 `kz::build_identity()` (스키마 fingerprint, libkz-core 소스 identity, 컴파일러, 언어 수준)를 기록해 provenance 를 남긴다.

**헤더의 tool hash** (`kz/core/identity.hpp`, 정의는 `docs/schema.md` §8)는 도구 **소스**의 identity 다: 이름, 버전 라벨, ABI, 스키마 fingerprint, libkz-core 의 소스 identity, 도구 자신(과 링크하는 라이브러리)의 소스 identity 를 길이 표지와 함께 해시한다. 소스 identity 는 CMake 가 configure 때 계산한다(`<트리 안의 상대 경로> <파일 SHA-256>` 줄을 경로순으로 해시; 트리의 위치와 컴파일러에 무관). 소스 파일이 바뀌면 `CMAKE_CONFIGURE_DEPENDS` 로 configure 가 다시 돌아 해시가 바뀐다. 같은 소스는 gcc 와 clang 에서 같은 해시를 낸다 [M]. **한계**: 파일의 identity 이지 도구 체인의 identity 가 아니다. 컴파일러와 언어 수준은 `build_identity()` 가 따로 기록한다.

도구는 `kz_tool_identity(<target>)` (CMake, `target_link_libraries` 뒤에)를 부르고 코드에서 `KZ_THIS_TOOL_HASH("이름")` 으로 해시를 만든다. 2인자 `kz::tool_hash(이름, 버전)` 은 libkz-core 소스만 덮으므로 시험용이다.

golden 을 의도적으로 바꿀 때: `KZ_GOLDEN_UPDATE=1 ctest --test-dir <빌드> -L golden` 후 `tests/golden/data/` 의 diff 를 읽고 확인한다.

## 3. 규칙 [D]

- **언어**: 도구 핵심은 C와 C++ 만(C++17/C11). Python, JSON, 생성 코드, 코드를 내는 코드는 없다. 레코드 배치는 X-macro 로 쓰고 스키마 컴파일러를 쓰지 않는다. 작은 빌드·시험 접착만 CMake 와 shell.
- **엔진에 사례 식별자 없음**: `include/` 와 `lib/` 에는 사례 식별자(R2, C1, C2, C3, C5, 2663, `CFE_*`, cFS 모듈 이름 cfe·evs·osal·psp·cfs, `MainTaskSyncDelay`, `GetAppID`, `AppId` 등, 목록은 `tests/lint/data/case_identifiers.txt`)가 없다. 낱말 하나가 아니라 **이름의 구성 요소**로 찾는다: `_` 와 camelCase 경계에서 잘라 대소문자 없이 비교하므로 `check_R2`, `r2_x`, `R2Cert`, `EVS_APP_ID`, `"cfe_sb_task.c:138"` 도 걸리고, `R2D2`, `C10`, `CRC32` 는 걸리지 않는다(소문자 변수 `c1` 도 걸린다: 이름을 바꾼다). cFS 지식은 `profiles/cfs-v7.0.1/*.kzp` 나 basis 레코드에 출처(file:line, hash)와 함께 둔다. 사람이 입력한 규칙·사실은 출처 계층 `researcher` / 항목 종류 `DECLARED_TRUSTED` 로 표시하고 보고에서 센다. `profiles/` 와 `tests/` 는 예외.
- **용어(T1)**: 판정 대상은 *readiness specification*(준비 상태 명세), 그 Req 는 *readiness precondition*, 위반은 *readiness violation*. 새 식별자에는 `rspec`, `rprecondition` 꼴을 쓰고 **`contract` 를 쓰지 않는다**(`lint_retired-term`). "API contract" 는 문서화된 API 의미를 가리킬 때만 문장에서 쓴다. 출처 계층: contract-backed / admitted / code-derived / issue-derived / researcher / unreferenced (C 식별자 `KZ_TIER_*`, 표시 이름은 PI 가 정한 그대로).
- **연구 조건**: 연구 환경·변수·재현 조건은 임의로 정하지 않는다(`../CONDITIONS_POLICY.md`). 레퍼런스가 정하지 않은 값은 `deviation`(값, 이유, 영향) 또는 `unspecified_by_reference` 로 기록한다(`kz_basis_item.policy`, `kz_deviation`). 단위 시험의 값은 정책 `unit-test-value` 로 표시하고 실험 결과로 쓰지 않는다.
- **결정적 산출물**: 정렬된 섹션·부모, 0 padding, 시각·호스트 정보 없음. 같은 내용(각 섹션의 레코드와 그 순서, 부모, 헤더의 세 해시)은 같은 바이트와 같은 해시를 낸다. 문자열 풀(STRINGS)은 레코드가 참조하는 문자열만 바이트순으로 정렬해 쓴 **정규 풀**이라 writer 가 문자열을 어떤 순서로 intern 했는지는 결과에 영향이 없다(`intern()` 이 돌려주는 `kz_str` 은 임시 값이다). 섹션 안의 레코드 순서는 의미이므로 생산자가 정한다(포인터를 키로 하는 맵을 그대로 순회하지 말 것). 모든 컨테이너 헤더가 basis·profile·tool 해시와 부모 artifact 해시를 싣는다(provenance).
- **안전성만, 진행성은 따로(D20)**: 판정에 "안전" 또는 "활성"을 뜻하는 필드가 없다. 진행성의 기본값은 `unknown(not_checked)`. lemma 의 증명 상태는 `unproved` / `depends-on-stated-assumption` / `proved` 로 기록하고, 의무를 적어 두었다는 이유로 `proved` 라고 쓰지 않는다. 코드가 확인하는 것: `proved` 항목의 `source` 는 PROVENANCE 기록을 가리켜야 하고(`validate_trust_entry`), 그 기록이 이 컨테이너에 있으면 경로(`file`)와 본문 해시(`text_sha256`)가 있어야 한다(`validate_proof_texts`). 그 문서가 **실제로 증명을 담고 있는지**는 코드로 확인할 수 없고 저자의 분류다(PS1). 인증서·셀에는 가장 약한 상태가 붙고(`validate_cert`, `validate_cell_proof`), 글로 쓸 때는 원장 집계를 함께 주면 저장된 상태를 원장과 대조해 어긋나면 `INCONSISTENT`, 원장 없이 `proved` 이면 "not checked against a ledger" 로 쓴다. "accepted by kz proofcheck" 뒤에는 항상 그 상태가 따른다.
- **LLVM**: 분석 도구(`lib/llvm`, `tools/kz-static`)에서만. libkz-core, `tools/kz`, `lib/proofcheck` 에는 쓰지 않는다. C++ 예외(exception)를 쓰지 않는다(libkz-core 는 `-fno-exceptions` 로 컴파일). LLVM 에 닿는 코드는 `LLVM_ENABLE_EH=OFF` 관례를 따른다.
- **cFS 번들**: `/home/user/work/procrace/cfs_ref/cFS` 와 그 빌드는 읽기 전용이다. cFS 바이너리는 `env/run_cfs.sh` 를 통해서만, 기록기 단계(K2)와 통합 단계에서만 실행한다. 이 토대(K0)는 cFS 를 실행하지 않는다.
- 코드 주석은 영어, 사람이 읽는 markdown 은 한국어(식별자는 영어).

## 4. 상태 표

2026-10-15 기준. 구성요소 소유자가 자기 행을 갱신한다.

| 구성요소 | 위치 | 상태 |
|---|---|---|
| 스키마 `kz_schema.h` (컨테이너, 출처, deviation, witness, bound, verdict, verdict cell) | `include/kz/schema/` | 구현, 시험 통과 [M] |
| `kz_raw.h` (관측 레코드) | `include/kz/schema/` | 구현(K2 소유). 초기 정의에서 SITES·LOSS 가 v2 가 되고 SITE_SLOTS, SIDE_EVENTS, THREADS, MAPS, SYMBOLS, TARGETS 가 더해졌다(`docs/schema.md` §9) [M] |
| `kz_facts.h` (사실·skeleton 레코드) | `include/kz/schema/` | 추출 레코드는 구현(K4a 소유, revision 4: A-TYPE `unknown-effect`, FACT_LOCS v2, writer `partial-write`, 루프 `static-image-assumed`; revision 3: 루프의 인증 뷰 규칙, 섹션 37–39) [M]. skeleton 레코드(23–29)는 초기 정의이고 K4b 가 쓴다 |
| `kz_cert.h` (certificate, 신뢰 원장) | `include/kz/schema/` | **초기 정의**(K4b 소유). 생성기·검사기는 없음 |
| KZ 컨테이너 입출력, SHA-256, CRC-32, 정규 dump, 레지스트리 | `lib/core/` | 구현, 시험 통과 [M] |
| 판정 셀·원장·증명 상태 보조 (기본값, 불변식, 서술) | `lib/core/verdict.cpp` | 구현, 시험 통과 [M]. 판정 격자(lattice)는 **미구현** |
| `tools/kz-dump` | `tools/kz-dump/` | 구현, 시험 통과 [M] |
| lint 다섯 개 + 자체 시험(90 사례) | `tests/lint/` | 구현, 통과 [M]. "맨 상수" lint(`KZ_REF(값, "경로:줄")` 규율)는 **미구현**: 이 규율을 쓰는 구성요소가 아직 없어서 규칙을 정할 수 없다. **S0-R2 관문의 열린 항목**이다(그때까지 맨 상수 금지는 사람의 검토로만 지킨다) |
| `cmake/KzSourceIdentity.cmake`, `kz/core/identity.hpp` | `cmake/`, `lib/core/identity.cpp` | 구현, 통과 [M]. 소스 identity 는 파일 identity 이고 도구 체인은 덮지 않는다 |
| Φ 프로필 언어: 어휘, 파서(줄·열 오류), 검사기, 정규 text, 프로필 해시, 출처 본문 대조 | `lib/core/phi_*.cpp`, `include/kz/core/phi*.hpp`, `docs/phi_reference.md` | 구현, 시험 통과 [M] |
| 평가기(구체·추상 domain) | `include/kz/core/eval*.hpp`, `lib/core/eval_{concrete,abstract}.cpp` | 구현, 성질 시험 통과 [M]. operational domain(탐색기용)은 **미구현** |
| 재생 엔진: 효과, HB 간선 4개 템플릿, readiness 발생의 상태, 결과 코드 검사 | `lib/core/eval_trace.cpp` | 구현, **합성 trace** 로 시험 통과 [M]. 실제 trace 에는 아직 쓰지 않았다. Allowed 항목의 horizon monitor, vector clock, 탐색기는 **미구현** (항목이 판정하는 발생은 `PENDING` 으로 남는다) |
| `tools/kz-phi` | `tools/kz-phi/` | 구현, 시험 통과 [M] |
| `profiles/cfs-v7.0.1/*.kzp` | `profiles/` | 판정용 Φ 프로필의 **V1 부분집합**(K-ID.send-with-appid, cell 과 INIT 사실, GetAppID 의 out 포인터 효과, App 레코드의 AppState, HB R1·R2-P-TASK·R18·R19, T-TERM 선언)은 구현. 인용 98개를 고정된 번들에서 다시 읽어 본문 해시를 대조했고 source 4개의 개정 고정이 체크아웃된 커밋과 같음을 확인했다 [M]. 출처 기록 109개, 항목 91개 중 손으로 넣은 것(가장 강한 tier researcher) 3개, `researcher` 기록을 하나라도 인용하는 항목 12개, `declared_trusted` 3개, 호출자 수준 인용: appreq 1개(contract-backed, side caller), 손으로 넣은 인용 0개(`kz-phi stats`). 프로필 해시 `bc8a3401e6b53b23642e258363dd27eabd7187eea2790f463a45171ae5ec90a4`. 나머지 프레임워크(C1·C2·C3·C5 의 지식)는 **미작성**. 관측용 프로브 계획 `profiles/cfs-v7.0.1/observe/sites.kzp` 는 있다(K2; 형식 `kzp-observe 1`, 인용 해시 검증됨 [M]) |
| basis, extract (LLVM) | `lib/llvm/`, `tools/kz-static/` | 구현(K4a), 시험 통과 [M] (`docs/static_facts.md`): core-cpu1 의 basis 와 facts. 2026-10-16 검토 반영: unknown-effect(본문 없는 callee·미해결 간접 호출·가변 위치·inline asm), 데이터 함수의 크기 검사, record 형 대상의 부분 쓰기, const view cast, 지역 변수 id, table walk image 확인(선언된 가정), L1 (ii) 의 호출 결과 제한, derives 의 cast. writer 집합 완전성은 문서 §8 의 가정 G1–G4 위에서만 성립. C 라이브러리 데이터 함수 표는 아직 엔진 안(Φ 문법 대기). app-unit link probe, anchor audit, transition filtering·key 상태, 문서 주석 후보는 **미구현**. skel 은 K4b |
| MustBefore, certificate, census, plan | `lib/static/` | **미구현** |
| 근거 검사기 `kz proofcheck` | `lib/proofcheck/` | **미구현** |
| 관측기 perf-sample 백엔드(F0), 앵커 해석, loader | `probe/` | 구현(K2), 시험 통과 [M]. 실제 cFS 에 한 번 붙여 보았다(`docs/observer_smoke_run.md`). **S1-R2 기능 확인은 미완(N=1/3)**: 남은 두 실행은 cFS 를 돌릴 수 있는 단계가 고친 스크립트(`observe_cfs_smoke.sh <tag> <kz> <ordinal>`)로 한다. s1r2_p1 을 쓴 도구의 원본은 되찾을 수 없다(같은 문서 §8). BPF 백엔드, 프로브 비용 측정, 앱 `.so` inode 등록, 생성 함수 출력 인자와 알린 id 의 대조는 **미구현**(V1b/S1) |
| trace 해석: 병합, 손실, 정체성 결합, 호출 짝짓기 | `lib/trace/` | 구현(K2), 시험 통과 [M]. 정체성 결합의 생성자 연결은 FORK 시각 포함 하나에 기댄다(A-CLK-PO; `match-by-time`), 알린 id 는 검증되지 않음(`id-unverified`). HB 간선과 사건 판정은 **미구현** |
| CLI `kz anchor`, `kz observe`, `kz hits` | `tools/kz/` | 구현(K2), 시험 통과 [M]. `kz-static` 은 K4a 행 참조. 사용법: `docs/observer.md` |
| predict (최소, 2026-10-25) | `lib/predict/`, `include/kz/predict/`, `tools/kz/cmd_predict.cpp`, `profiles/c1-bp-b051eb0/predict/*.kzm` | 구현(검증한 C1 범위만), 시험 통과 [M]: 행위자 프로그램 모델(`kzp-predict 1`), 기록 앵커, 명시적 상태 탐색(축소 없음, SMT 없음), 관측 종속 간선의 변수 처리와 두 ablation, 후보의 재생 재판정·기각, kzp-hold 1 계획 합성과 supervisor 의미의 점검(`docs/predict.md`). A-COMM 검사·외삽·explorer(σ_R1)·다른 사례는 **미구현** |
| conform, explore, confirm, baselines, gate | `lib/*`, `gate/` | **미구현** (R2 경로 밖) |
| 스키마의 EVENTS, EDGES, INSTANCES, TEMPLATES 등 | — | id 만 예약, 레코드 **미정의** (`docs/schema.md` §3) |

## 5. 알려진 제한 [M]

- 이 환경의 clang 18 에는 sanitizer runtime(compiler-rt)이 없다. 그래서 clang 으로는 UBSan 을 trap 방식으로만 돌렸고, ASan 과 완전한 UBSan 진단은 gcc 로 돌렸다. 설치는 하지 않았다.
- 시험은 모두 합성 표본이거나 목적 제작 프로그램이다. 시험은 cFS 를 실행하지 않는다. 관측기를 실제 cFS 에 한 번 붙인 것은 시험이 아니라 연습 실행이고 그 기록은 `docs/observer_smoke_run.md` 이다(한 번의 실행이라 빈도나 비용을 주장하지 않는다). 이 단계의 통과가 관측기·판정의 타당성을 뜻하지 않는다.
- tool hash 는 도구 **소스**의 identity 다. 같은 소스를 다른 컴파일러나 다른 최적화 수준으로 빌드한 바이너리가 다르게 동작할 수 있다는 점은 덮지 않는다(그 정보는 `build_identity()` 의 BASIS 항목에 있다). 소스 identity 에는 `docs/`, `tests/`, 프로필 데이터가 들어 있지 않다.
- 판정 셀 서술(`render_cell`)은 SPEC §1.10 의 서술 규칙을 **부분적으로** 구현한다. 하는 것: 보편 class 에 경계를 항상 쓰고(없으면 `[b=?]`), 보존된 진술과 witness 를 쓰고, stall witness 를 구간·종류·한정어·harm·실현 방식과 함께 쓰고, 증명 상태를 원장과 대조한다. 하지 못하는 것: stall witness 의 **wait-for root 집합**과 witness 의 ordinal 은 witness 기록에 있고 셀에는 없으므로 기록의 참조만 쓴다(해석은 호출자 몫). PS3 의 "증명되지 않은 lemma 의 이름"은 lemma 표의 `ident`(`#<n>`)로 쓴다. ident 를 이름(L-MB …)으로 바꾸는 표는 `kz proofcheck` 쪽(K4b)에 있어야 한다.
- 판정 격자, profile 평가기, 사례별 규칙은 이 토대에 없다(상태 표 참조). `kz_raw.h`, `kz_facts.h`, `kz_cert.h` 는 초기 정의이고 해당 구성요소가 바꿀 수 있다. 바꾸면 스키마 fingerprint 와 모든 도구의 tool hash 가 바뀐다(의도한 동작). 컨테이너 golden 은 고정 tool hash 를 쓰므로, 표본 컨테이너가 쓰는 레코드가 바뀌지 않는 한 영향을 받지 않는다.

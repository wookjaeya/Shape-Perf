# 컴파일러 연결부 선택: 세 후보의 단계 C 증거 비교와 단계 D–F 계획 (2026-10-26)

- 범위: IR(사용자 업로드 `Kairos_OS_Independent_Instrumentation_Review_20261026.md`) §15.1(일곱 질문과 선택 규칙 세 줄), §15 단계 C의 진행 근거("주 계측 구현 하나를 선택할 수 있음"), 단계 D–F, §13.1–§13.3. 결정 기록 `Architecture/spec/PI_DECISIONS_RESOLVED_20261026.md`(PD26) §2 항목 3(같은 C1·R2 관측점에서 비교해 하나를 고른다, 세 완성품을 만들지 않는다), §4(구속). SPEC rev 1.4 §5.12 "Choosing the compiler connector", §8.6 "Groups for the execution families", ADR-023, ADR-059.
- 입력: 후보 문서 `kairos/docs/instr_candidate_{gcc,clang,llvm}.md`(커밋 `4a3d424`, `e7f297a`, `06e3329`)와 시제품 `kairos/instrument/{gcc,clang,llvm}/`, 런타임 B1(`a335519`, `docs/event_semantics.md`), 중립 층 B2(`b2061a5`, `docs/schema.md` §12).
- 표기: **[F]** 코드·문서에서 확인, **[M]** 빌드·실행으로 확인(따로 적지 않으면 그 후보 문서의 [M]), **[I]** 추론, **[D]** 설계 결정, **[U]** 미확인.
- 지위: **선택 기록 [D]과 계획 [I]**이다. 이 문서를 쓰며 새 cFS 실행은 하지 않았다. 인용한 실행은 후보마다 pilot/dev 부팅 C1·R2 각 1회(LLVM은 L-A 부팅 각 1회 추가)이고 본 실험 결과가 아니다. IR §15.1대로 가중 점수를 쓰지 않는다. 연구 조건을 새로 정하지 않았다(CONDITIONS_POLICY).
- 이 선택은 PD26 §2 항목 3이 채택한 규칙(IR §15.1의 세 줄)을 증거에 적용한 것이다. 새 PI 결정이 아니며, PI는 언제든 뒤집을 수 있다.
- 이 문서에서 직접 확인한 것 [F][M]: F0 계획과 후보 manifest의 site label 대조(§4), `kzgcc.cpp`의 store 형태(L519–L527)와 `line=` 확인(L439, L530), 기준 소스 줄(R2 기준 트리 `cfs_ref/cFS` v7.0.1, cfe `c5fb2b4d`), 저장소 LICENSE 없음, 시험(기본 빌드 139개 통과·tracefs 1개 건너뜀, LLVM 빌드 161개 통과·같은 1개 건너뜀, HEAD `b2061a5`).

## 0. 결론

1. **주 구현: GCC GIMPLE plugin `kzgcc`(GCC 13.3.0)** [D]. 규칙 1("GCC plugin이 요구를 충족하면 주 구현으로 진행")의 조건이 단계 C 범위에서 충족됐다(§3.1).
2. 규칙 2(Clang 소스 변환 → gcc)로 바꿀 근거는 없다. 단계 D–F에서 GCC 쪽 내부 API·배포 문제는 작게 나타났고, Clang 경로에서 관찰된 비용(매크로 안 반환 경로 미계측, 빌드 파일 편집, R2 단위 시험 끄기, 구성마다 재변환)이 그보다 작거나 명확하지 않다(§3.2).
3. 규칙 3(LLVM 전용 pass)의 두 조건이 모두 서지 않는다. 같은 manifest에서 관측 범위가 GCC와 같았고, 대상 빌드는 번역 T1–T4와 아직 정하지 않은 T3에 기대며, 모든 비교에 L-A 기준선이 붙는다(§3.3).
4. F0 GCC 기준 실행은 비교 경로(G-A, G-F0, G-F0-H)로 남는다.
5. Clang·LLVM 시제품과 문서는 비교 증거로 저장소에 그대로 두고 더 키우지 않는다. 다시 볼 조건은 둘이다(§6). (a) 두 번째 OS의 교차 GCC가 plugin을 적재하지 못함. (b) 라이선스 결정이 GPL 호환 선언과 맞지 않음.
6. 단계 D에 아직 없는 것의 핵심(§4): 계측 기록의 정체성 결합, F0 계획 수준의 site 범위(F0 C1 계획 34개 중 13개, R2 35개 중 14개만 GCC manifest에 같은 label이 있음), plugin의 필드·전역 store 형태, 파생 빌드의 basis B′, B1 런타임 수정 넷.
7. 새 PI 결정은 없다. 이 선택에 걸린 열린 결정은 둘이다(§7). `kzgcc.cpp` 배포 라이선스(GCC 후보가 제기)와 파생 실행의 R2 시작 상태(Clang 후보가 제기, GCC 경로에도 해당)다.

## 1. 비교한 증거

| | GCC (`4a3d424`) | Clang 변환 → gcc (`e7f297a`) | LLVM pass (`06e3329`) |
|---|---|---|---|
| 연결부 | `kzgcc.cpp` 850줄. `ssa` 직후 삽입, `optimized` 직후 확인 | `kz_instr_clang.cpp` 1,179줄. compile DB로 파싱해 사본 트리에 소스 편집, 빌드는 gcc 13.3 | `kzllvm.cpp` 881줄. 고정 clang(`1053047a`)의 PipelineStart에 삽입, OptimizerLast에서 확인 |
| site manifest | `instrument/gcc/sites/`: C1 16 site, R2 15 site | `instrument/clang/sites/`: C1 28 site(OSAL 피호출 쪽·내부 점 포함), R2 16 site | **GCC 후보의 manifest 파일 그대로**(sha256 `8dbdd2e8…`, `a4a92b90…`) |
| 빌드 | C1 Release `-O3`(PIE + `.so`), R2 debug `-O0 -g`(단위 시험 포함) | C1 `-O3`, R2 `-O0`(단위 시험 끔) | clang으로 C1·R2 target 코드를 L-A(비계측)·L-I(계측) 두 벌씩 |
| 실행(pilot) | C1 tx 1, R2 1 | C1 tx·rx 1, R2 1 | L-I C1 tx 1, R2 1 + L-A 각 1 |
| F0 대조 기록 | C1 `smoke3/post_1df262a`, R2 `v153_pilot3` | 같음 | 같음 |
| 런타임 | B1 수정 없음 | B1 수정 없음 | B1 수정 없음. 런타임 파일 3개가 GCC 후보의 것과 바이트 동일 |

- 비교 가능성 [I]: GCC와 LLVM은 같은 manifest라 지점 단위로 바로 비교된다. Clang은 C1 manifest가 더 넓다. 그래서 OSAL 피호출 쪽·내부 점(조회·생성 입구와 반환, `OS_ObjectIdFinalizeNew` 래퍼, `osapi-idmap.c:978` store 탭)은 Clang에서만 [M]이고, GCC에서는 같은 형태로 묶을 수 있다는 추론 [I]이다(`instr_candidate_gcc.md` §4 끝).

## 2. IR §15.1의 일곱 질문: 후보별 증거

| # | 질문 | GCC plugin | Clang 변환 → gcc | LLVM pass |
|---|---|---|---|---|
| 1 | 현재 공개 cFS 구성과 필요한 target을 빌드하는가 | **예(host x86-64 Linux, gcc 13.3.0).** 기준 명령 그대로다. 바뀐 객체는 계측 단위뿐이다(C1 261개 중 6개, 나머지 바이트 동일. R2는 strip 뒤 계측 8개 + 빌드 날짜 1개 + 빌드마다 다른 coverage 267개). 빌드 파일은 고치지 않았다(configure 뒤 arch cache의 플래그만, D3). R2 단위 시험도 기준대로 빌드했다(coverage 단위는 `skip-coverage`). 환경 변경은 같은 판 `gcc-13-plugin-dev` 설치 하나이고, 컴파일러 바이너리는 바뀌지 않았다(gcc §1, §3) [M]. 교차 GCC의 plugin 지원은 [U] | **예(host, 빌드는 gcc 13.3).** 변환하지 않은 객체는 C1 238개 동일, 19개는 경로·버전·시각 문자열만 다르다. R2는 794개가 `.text` 동일이다. 차이는 `-I`(런타임 include), **사본의 `cfe/cmake/target/CMakeLists.txt`에 런타임 링크 한 줄 추가**, `.git` 없는 사본의 빈 버전 문자열, **R2 `ENABLE_UNIT_TESTS=FALSE`**(기준 TRUE)다(clang §4) [M]. 교차 대상에서 Clang이 대상 compile 명령을 파싱하는지는 [U]. 변환된 소스를 plugin 없는 교차 gcc가 빌드할 수 있다는 점은 [I] | **예, 번역 T1–T4를 거쳐(host).** T3(컴파일러 id 분기를 GNU로)이 없으면 C1의 CF·BP·bplib·TO_LAB·SCH_LAB이 `undefined symbol`로 적재되지 않는다. T4로 compiler-rt profile runtime을 따로 빌드했다(환경 변경). 번역을 빼면 compile·link 명령이 기준과 같다(C1 285/285·47/47, R2 1417/1417·323 중 1은 비교 정규화의 충돌)(llvm §3) [M]. 고정 LLVM 빌드는 `LLVM_TARGETS_TO_BUILD=host`뿐이다 [F] |
| 2 | 실제 값·반환·공개 지점을 부작용 없이 전달하는가 | **예, 범위 안에서.** hook 인자는 원 GIMPLE 문장의 operand다. 결과를 버리던 호출에는 SSA 이름을 붙인다. out은 호출 직후 같은 스레드가 읽는다(event_semantics §8.1의 허용 규칙). 문자열은 `string=validated` 슬롯에서만 읽는다. 값 교체·atomic 변환은 없다. 코드에 생긴 변화는 명령 수 증가다(`BP_FlowInit` 93 → 158)(gcc §2.1, §3.3, §6) [F][M] | **예, 조건부.** 인자 식은 한 번만 평가하고 평가 순서는 고정하지 않는다(부작용 있는 인자 시험). 원 코드에 없는 읽기가 셋 있다: 반환 직후 out, 검증된 문자열의 상한 복사, **반환 지점의 매개변수 다시 읽기**(본문에서 대입되거나 주소가 잡히면 거부). `-O0`에서는 래퍼·탭이 별도 함수로 남는다(clang §1, §2.2, §7) [M] | **예, 범위 안에서.** IR operand를 넘기고, 반환은 `call`의 결과 값을 그대로 쓴다(결과를 버린 호출도 코드 변경 없음). hook이 펼침·inline 판단을 바꿨다(`CF_CFDP_InitEngine` L-A 458 → L-I 328 명령; gcc 기준 229)(llvm §3.3, §6) [M] |
| 3 | source 위치와 실행 지점의 대응을 설명할 수 있는가 | **예.** binding 파일(원 줄·함수 → 최종 함수·개수·상태), 링크 뒤 objdump 검사, 실행 pc가 서로 맞았다. BP 4개는 `inlined`로 보고됐다. `line=`은 GIMPLE 문장 위치로 확인하며(`kzgcc.cpp` L439, L530), 디버그 정보가 없는 C1 Release에서도 `line=` site 7개(CF 3, BP 4)가 줄을 확인한 뒤 `bound`·`inlined`로 연결됐다. 한계: 합쳐진 return은 `:0`이고, RTL 단계의 복제는 링크 뒤 검사로만 본다(gcc §2.2, §4) [F][M] | **예.** binding마다 `파일:줄:열`과 형태가 있다. 기계어 조사로 binding 44개(C1 28, R2 16)가 모두 호출로 남았음을 확인했다. `#line`으로 `__LINE__`이 같고, `__func__`도 같다. `__FILE__`은 사본 경로가 된다(clang §2.2, §3) [M] | **예, 디버그 위치가 있을 때.** R2(`-g`)에서는 `line=`이 디버그 위치와 맞았다. C1 Release에서는 확인할 수 없어 함수 안 유일 호출일 때만 연결하고 `line=unverified`로 표시했다. `-O3`에서 CF·BP site 7개가 루프 펼침으로 두 사본(`duplicated`)이 됐고, pc로 구별했다(llvm §2.2, §4) [M] |
| 4 | 초기화 전 관측을 지원하는가 | **예(런타임이 제공).** OSAL 초기화 중의 task 생성(`0xc0001`, `0x80001`)이 기록됐고, 시작 객체보다 먼저 도는 모듈 생성자는 부트스트랩 경로로 들어갔다(gcc §6) [M]. 생성자 없는 RTOS에서는 하네스가 init/attach를 불러야 한다 [U] | **경로는 있다. 이번 실행에서 attach 전 사건은 없었다**(공유 버퍼 0 B). 모듈 등록 생성자는 actor 없이 기록됐다(clang §1) [M] | **예(런타임이 제공).** GCC와 같은 모양이다(llvm §6) [M] |
| 5 | 필요한 미계측 경로를 확인할 수 있는가 | **예, 번역 단위 수준까지.** `unsupported`(이유), `removed`, 계측하지 않은 coverage 단위 목록이 남는다. GIMPLE에는 매크로 경계가 없어 `OS_CHECK_*`의 오류 반환도 합쳐진 반환을 지난다 [I: gcc §4의 `:0`]. 실행으로 그 경로를 지난 기록은 없다. 보이지 않는 것은 plugin 없이 미리 빌드된 라이브러리와 간접 호출이다(gcc §6) [F][M] | **예, 정적 목록으로.** 매크로 안 반환 경로가 **미계측**으로 보고된다(C1 10, R2 5. 모두 OSAL `OS_CHECK_*`의 오류 반환). 매크로 본문·인자 안 호출, 비활성 구역, compile DB 밖 TU도 보고된다. 구성마다 다시 변환해야 한다(clang §2.3) [M] | **예, 번역 단위 수준까지.** GCC와 같은 보고에 ABI 낮춤의 한계가 더해진다. `sret`/`byval`은 거부하고, 정수 하나로 합쳐진 구조체는 IR만으로 가려내지 못한다(llvm §2.1, §6) [F] |
| 6 | 같은 runtime에 연결되는가 | **예.** B1을 수정 없이 링크했고 `kz-rt-dump --manifest` 검사를 통과했다. `.so`는 실행 파일이 내보낸 기호로 hook을 부른다 [M] | **예.** 같은 검사가 C1 tx·rx와 R2에서 실패 0이다 [M] | **예.** 런타임 파일이 GCC 후보와 바이트 동일하고, 검사를 통과했다 [M] |
| 7 | 연구를 지연시키는 추가 frontend·ABI·모델 작업이 얼마나 생기는가 | **작다.** frontend 작업과 ABI 모델이 없고 새 형식도 없다(`kzs-sites 1`). 대신 GCC 판마다 plugin을 다시 빌드하고 시험 6개를 돌려야 한다. 교차 툴체인마다 plugin 헤더가 필요하다. GPL 호환 선언(`plugin_is_GPL_compatible`, `kzgcc.cpp` L34–L35, L75)의 배포 라이선스는 PI가 정해야 한다(gcc §7, §9) [F][M] | **frontend·ABI·모델 작업은 없다.** 대신 빌드 체계 편집(런타임 링크 줄), 단위 시험 빌드 처리, 가변 인자 호출의 인자 탭, 구성마다 재변환이 필요하다. 매크로 텍스트는 편집하지 못한다(clang §1, §7) [M][I] | frontend 작업은 없다. 대신 구성마다 T1–T4를 다시 찾아야 하고(C1·R2의 경고 종류·GNU 분기가 다름), **L-A 기준선이 의무**다(ADR-023). IR 수준의 ABI 대응을 확인해야 하고, LLVM 판에 묶인다. Instrumentor는 로컬에 없다. 상류 `8da87cd`에는 호출 지점·반환값 항목이 없고 값 교체 항목이 있다(llvm §6, §9) [F][M] |

### 2.1 후보 사이에 차이가 없던 것 [M]

- 값과 정체성: C1에서 CF·BP의 세마포어 조회·생성 순서(이름, `-17`/0, `0x30002`/`0x30003`)와 OSAL task id 17개의 소유 앱·생성 task가 세 후보 모두 F0 smoke3와 같았다. R2에서는 세 후보 모두 SB 호출 지점에서 넘긴 AppID를 직접 얻었고, F0가 상태 코드만 읽던 SB의 저장 값 `0x110003`도 직접 얻었다.
- AppID 0 전송 수는 GCC·LLVM이 24(ES 20 + EVS 4, 호출 지점별 분포까지 F0와 같음), Clang이 11(ES 7 + EVS 4)이다. 차이는 모두 `cfe_sb_priv.c:786`에서 났다. 실행이 하나씩이라 원인(스케줄, 관측 섭동)을 가를 수 없다. IR §13.2대로 24는 합격 조건이 아니며, 연결부의 포획 능력 차이로 읽지 않는다 [I].
- 앱 전송 수는 실행마다 달랐다. `evs.send.entry`는 F0 269, GCC 278, Clang 274, LLVM 311이고, @786은 65, 74, 70, 107이다. 선택 근거로 쓰지 않는다.
- 기록: 세 후보 모두 손실 0, aseq 빈틈 0, 엄격 검사 통과였다. dump는 정상 종료 경로(ES POWERON → `exit`)의 `atexit`로 써졌다.
- 세 후보 모두 하지 않은 것: hook·기록 비용(G-I0/G-I), 제어(`control` 줄은 RECORD 모드라 평가되지 않음), `kz replay` 판정 대조, 교차 target, 시그널 종료 경로의 dump. B2가 GCC C1 dump를 중립 층으로 바꿔 재생했으나, CF 조회 3건이 근거 부족으로 `INCONCLUSIVE`였다(schema.md §12.6. 판정 대조가 아님).
- 세 후보가 같이 요구한 런타임 변경: RUN 레코드의 `backend`·`compiler`·`target`·`manifest_sha256` 채우기, `kz-rt-dump`의 pc·native key 출력, `kz_rt_module` 중복 등록 제거, binding id 규칙의 문서화.

정리 [I]: 값을 얻는 능력에서는 세 후보가 갈리지 않았다. 차이는 빌드 조건과 기준과의 거리(문항 1), 코드에 생기는 부수 변화(2), 위치 확인의 조건(3), 미계측 경로의 모양(5), 남는 작업의 종류(7)에서 났다.

## 3. 선택 규칙의 적용 (IR §15.1, SPEC rev 1.4 §5.12)

### 3.1 규칙 1: GCC plugin이 요구를 충족하는가 → 충족 (단계 C 범위)

요구는 IR §5.1의 GCC 시제품 항목과 PD26 §4(구속)에서 읽었다.

| 요구 | 근거 | 상태 |
|---|---|---|
| C1 생성·조회 API의 입력·반환·out-parameter | CF·BP 호출 쪽 site, 이름 `char[20]` validated, `-17` 부호 확장, out id(gcc §4, §5.1) | 충족 [M] |
| R2 전송 인자에 실제로 쓴 AppID와 호출 역할 | SB 호출 3곳의 인자 operand(gcc §5.2) | 충족 [M] |
| task 생성 요청·진입, 선택한 상태 공개 지점 | 정체성 site 9/8, `sb_publication`과 `CFE_ES_GetAppID` store(L754) | 충족 [M] |
| 런타임에 자기 계측을 넣지 않음 | `kz_rt_*`를 정의한 단위는 거부, 시험 `kzgcc_refuses_runtime` | 충족 [M] |
| GCC·plugin 빌드·pass 위치·최적화·LTO 기록, 판 사이 호환을 전제하지 않음 | binding 파일 머리, `plugin_default_version_check` | 충족 [F][M] |
| PD26 §4 값(원 operand, 재읽기·`strlen`·atomic 없음) | gcc §2.1 | 충족 [F] |
| PD26 §4 초기화 전·손실 구분·순서(런타임 순서는 HB 아님) | 런타임 공통(B1), 위 §2 문항 4 | 충족 [M] |
| PD26 §4 같은 빌드의 runtime mode(G-I0/G-I/G-I-H) | 런타임 `KZ_RT_MODE`. 비교 실행은 아직 없음 | 미실시(세 후보 공통) [U] |
| PD26 §4 보류 문맥 | `control <site> ctx=task` 두 곳이 별도 호출로 들어감. 평가는 안 됨 | 미실시(세 후보 공통) [U] |

미실시 항목은 세 후보에 공통이라 후보를 가르지 않는다. 따라서 규칙 1에 따라 GCC plugin을 주 구현으로 진행한다 [D].

### 3.2 규칙 2: Clang 변환이 GCC 내부 API·배포 문제보다 작고 명확한가 → 아니다 (단계 D–F 기준)

GCC 쪽에서 관찰된 문제와 단계 D–F에 주는 영향:

1. **판 고정**: plugin은 빌드한 GCC 판만 받는다. 그런데 기준 실험의 실행 컴파일러가 이미 gcc 13.3.0으로 고정되어 있다(SPEC §2.6 "gcc is the executed compiler"). 그래서 C1·R2의 단계 D–F에서 새 변수가 생기지 않는다 [F][I].
2. **plugin 헤더 패키지**: 같은 판으로 설치하고 기록했다. 컴파일러 파일은 바뀌지 않았다(gcc §1) [M].
3. **GPL 호환 선언**: 외부 배포 전에 PI 결정이 필요하다(§7). 저장소 안의 연구 실행은 막지 않는다 [I].
4. **교차 툴체인의 plugin 지원**: 미확인이다. 두 번째 OS 단계(IR §15 H)의 문제이고, D–F의 문제는 아니다 [U].
5. **`-g` 빌드의 DWARF producer 문자열**: 차이로 기록한다. 코드·데이터는 같다 [M].

Clang 경로에서 관찰된 비용:

1. OSAL 오류 반환이 매크로 안에 있어 **반환 경로가 미계측**이다(C1 10, R2 5). C1·R2 판정이 쓰는 OSAL 조회·생성 API(`OS_CountSemGetIdByName` 2, `OS_CountSemCreate` 3, `OS_TaskCreate` 5)가 해당한다. 그 경로로 끝난 호출에는 after 레코드가 없다(clang §2.3) [M].
2. 빌드 체계 파일을 편집했다(런타임 링크 줄). GCC는 cache 플래그만 썼다 [M].
3. R2 단위 시험을 껐다. 이것이 새 deviation이 됐고, 그 결정 요청도 생겼다. GCC는 기준대로 빌드했다 [M].
4. 모듈 버전 문자열이 비었다. GCC는 cFE hook으로 기준 표를 바이트 그대로 설치해 이 부작용을 없앴다 [M].
5. 원 코드에 없는 읽기가 하나 더 있다(반환 지점의 매개변수). `-O0`에서는 래퍼·탭 함수가 남아 코드 배치가 바뀐다. 구성마다 재변환해야 하고, 도구도 더 크다(1,179줄 대 850줄) [M].

판단 [D]: 단계 D–F에서 GCC 쪽 문제는 1·2가 이미 해소됐고, 3은 배포 결정이며, 4는 뒤 단계의 미확인이다. Clang 쪽 비용 1–5는 D–F에서 바로 생긴다. 그러니 소스 변환이 "작고 명확"하다고 볼 근거가 없다. 다만 4가 실제로 막히면(교차 GCC가 plugin을 적재하지 못하면) Clang 변환의 장점이 생길 수 있다 [I]. 그래서 이를 다시 볼 조건으로 둔다(§6).

### 3.3 규칙 3: LLVM의 관측 범위·자산이 실질적 이점이고 대상 빌드가 확보됐는가 → 아니다

- 관측 범위: 같은 manifest에서 연결 결과와 값이 GCC와 같았다(llvm §4, §5) [M].
- 이 후보에만 있던 것(llvm §6): 결과를 버린 호출에도 코드 변경 없이 반환값이 있다는 점, `-g`에서 무관 단위가 바이트 동일하다는 점, 루프 사본이 pc로 드러난다는 점이다. 이것들은 기록의 부수 성질이고, 분석기가 판정에 쓰는 값·정체성·순서를 늘리지 않는다 [I].
- 대상 빌드: T1–T4 번역이 있어야 서고, 그중 T3은 허용 여부가 정해지지 않았다(llvm §10). 고정 빌드는 host target뿐이다 [F][M].
- 더해지는 비용: F0·G-A와 비교할 때마다 컴파일러 차이(L-A)가 따로 붙는다(ADR-023, IR §16). C1 Release에서 `line=`을 확인하지 못한다. ABI 낮춤 대응이 필요하다. Instrumentor를 쓰려면 LLVM 판을 바꿔야 한다(llvm §7, §9) [M][F].

### 3.4 선택과 범위 [D]

- 주 구현은 `kairos/instrument/gcc/kzgcc.cpp`(GCC 13.3.0, `gcc-13-plugin-dev 13.3.0-6ubuntu2~24.04.1`)다. 다른 GCC 판에서는 plugin을 다시 빌드하고 시험 6개로 확인하기 전까지 쓰지 않는다(gcc §7).
- F0 GCC 기준 실행(재계측하지 않은 기준 빌드의 관측·보류)은 모든 단계의 비교 경로다. "기준 바이너리를 재계측 없이 관측한다"는 주장은 F0에 한정한다(PD26 §2 항목 6, SPEC §6.5).
- 이 선택의 결과는 경로(GCC plugin 파생 구성)·컴파일러·런타임·계획을 붙여 보고하고, F0 결과와 합치지 않는다(ADR-023, SPEC §6.5).

## 4. 선택한 경로가 단계 D에 아직 갖추지 못한 것

단계 D(IR §15)의 산출물은 "C1 및 R2 실제 실행의 계측 기록"이고, 진행 근거는 "identity와 판정에 필요한 사건·값을 확보"다. 단계 C는 값의 포획을 보였지만 판정 입력을 갖추지는 않았다. 증거는 B2 재생이 근거 부족으로 `INCONCLUSIVE`였다는 것이다(schema.md §12.6).

| # | 없는 것 | 근거 | 해야 할 일 | 성격 |
|---|---|---|---|---|
| 1 | **계측 기록의 정체성 결합**(생성 실행자, 생성 호출의 반환과 id, 소유 앱) | `kzs-sites 1`에 site의 정체성 읽기(F0 계획의 `decoder=`)를 적는 문법이 없다. 그래서 변환이 `OS_TaskCreate`의 out id와 소유 앱을 비워 두고, 재생이 `ownership-undetermined`가 된다(schema.md §12.6–§12.7). 값 자체는 이미 기록된다. GCC 대조 스크립트가 task id ↔ 소유 앱 ↔ 생성 task를 F0와 diff 0으로 맞췄다(gcc §5.1) [F][M] | manifest에 정체성 역할을 적는 문법을 정하고(`event_semantics.md` §3, `schema.md` §7 정책), `kzrt_convert`가 F0 변환과 같은 규칙(schema.md §12.2)으로 actor를 결합한다. 이름·역할에서 추측하지 않는다 | 도구 설계 [D] |
| 2 | **F0 계획 수준의 site 범위** | label 대조 [F, 이 문서]: GCC C1 manifest는 F0 C1 계획(`profiles/c1-bp-b051eb0/observe/sites-cf833fdbb.kzp`) 34개 중 13개만 같다. 빠진 21개는 피호출 쪽 세마포어 조회·생성 입구와 반환(SCH_LAB 등 모든 호출자), `exists`·`found`·`publish`·`created`, `cf.init*`·`bp.flow_init*`, 시작 상태(`clear_table`, `clear_names`), 삭제(입구·반환·`retire`), 수명(`request_shutdown`, `teardown`)이다. GCC R2 manifest는 F0 R2 계획(`profiles/cfs-v7.0.1/observe/sites.kzp`) 35개 중 14개만 같다. label이 없는 21개 중 `sb.appid.store`는 F0에서 `sb_publication`과 같은 지점(`cfe_sb_task.c:138`의 `CFE_ES_GetAppID` 호출 뒤, sites.kzp L78·L166)의 두 번째 label이라 GCC `sb_publication`이 그 호출을 묶는다. 실제로 빠진 것은 등록·writer·시작 상태·시스템 상태·정리 점 20개다. GCC의 `CFE_ES_GetAppID.store`(피호출 쪽 store)는 F0 계획에 없는 지점이다 | F0 계획의 의미와 인용을 옮겨 site를 더한다. 세 후보가 같이 쓸 논리 site 파일을 `profiles/` 아래에 둔다(clang §9의 열린 항목). Clang C1 site 30–38의 의미 정리(clang §3.1)를 참고하되 binding은 GCC가 새로 만든다. 두 계열의 동등성은 `equiv … unconfirmed`로 시작한다(event_semantics §3) | 도구·프로필 [D] |
| 3 | **plugin의 store·점 형태** | `phase=effect`는 포인터 매개변수를 통한 store(`*param = v`)만 잡는다(`kzgcc.cpp` L519–L527: lhs가 `MEM_REF`이고 base가 `PARM_DECL`의 기본 정의일 때만) [F]. F0 R2의 `CFE_ES_WaitForSystemState.raise_state`(sites.kzp L116, `cfe_es_api.c:573` `AppRecPtr->AppState = RequiredAppState;`, cfe `c5fb2b4d`)는 지역 포인터를 통한 필드 store라 지금 형태로는 묶을 수 없다. F0의 `line-start` 점 중 `cfe_evs.c:106`은 `EVS_AppDataSetUsed` 호출, `cfe_es_start.c:823`은 `memset` 호출이다. 둘 다 호출 형태로 옮길 수 있지만, GCC가 내장 함수로 다루는 `memset`이 삽입 pass 시점에 호출로 남는지는 [U] | 줄로 지정한 필드·전역 store 형태를 plugin에 더한다(operand0 = 저장 값, operand1 = 주소; 재읽기 없음, PD26 §4). F0의 점마다 그것이 어느 원 문장(호출·store)인지 정해 manifest에 적고, 시험 대상에 같은 모양을 더한다(시험 값은 단위 시험 값) | 도구 [D], 확인 [U] |
| 4 | **파생 빌드의 basis B′와 정적 닫힘** | "A configuration variant is a new basis B′"(SPEC §1.7). B2 재생에는 계측 빌드의 basis가 없었다(schema.md §12.6). `kz-static`은 compile DB의 `-fplugin*` 인자를 따로 다루지 않는다(`tools`·`lib` grep 0건) [F]. 파생 compile DB를 그대로 읽을 수 있는지는 [U] | 파생 빌드의 compile DB로 B′를 기록하고(plugin 인자는 계측 차이로 기록), facts를 뽑아 기준 B의 facts와 비교한다. 소스가 같으므로 basis 해시 밖의 내용이 같아야 한다 [I] | 도구·확인 |
| 5 | **B1 런타임 수정**(세 후보 공통, §2.1) | RUN 레코드의 컴파일러 정보가 비어 있다. dump 도구가 pc·native key를 내지 않는다(지금은 `tools/kzgcc_events.c`로 대신함). `kz_rt_module`은 단위마다 등록해 최대 32에서 `module-table` 손실이 난다(C1 6회, R2 8회, 2번 항목으로 단위가 늘어남). binding id 규칙 `(module << 16) | (site << 4) | n`이 문서에 없다(gcc §8) [F][M] | 고치고, 형식이 바뀌면 ABI·dump 판을 올린다(event_semantics §10의 규칙) | 도구 [D] |
| 6 | **버퍼 용량과 피호출 쪽 store** | R2 `CFE_ES_GetAppID.store`는 모든 호출자를 기록한다(GCC 1,519건). LLVM 실행에서는 가장 큰 actor 버퍼가 64 KiB 중 98%까지 찼다(llvm §5.2) [M]. 2번 항목으로 site가 늘면 넘칠 수 있다 [I] | 프로필이 요구하는 writer·호출자를 호출 쪽 site로 좁힌다. 넘치면 `overflow` 손실로 센다. 버퍼 크기는 도구 용량이고 RUN 레코드에 남으며 연구 조건이 아니다(event_semantics 머리말) | 설계 [D] |
| 7 | **C1 빌드 범위** | GCC manifest는 `post_1df262a`뿐이다. F0 smoke3는 네 빌드(수정 전·후 × 번들 `1df262a`·`1b0b338`, `plan/C1_build_manifest.md`)를 덮고, 계획은 CF 판마다 나뉜다(`sites-cf281a941.kzp`, `sites-cf833fdbb.kzp`) [F] | CF 판마다 manifest를 두고, 네 빌드마다 binding과 링크 뒤 검사를 한다 | 도구 |
| 8 | `kz hits` 등 F0 섹션만 읽는 명령 | schema.md §12.7 [F] | 필요할 때 중립 층을 읽게 한다 | 도구(작음) |

단계 D의 결손이 아닌 것: LTO의 링크 단계 확인(C1·R2 기준 플래그에 LTO가 없음, gcc §2.2) [F], 교차 툴체인(단계 H), 라이선스(배포 결정, §7).

진행(2026-10-26, D1·D5): 1번은 manifest 판 2(`kzs-sites 2`의 `identity`, `kairos/docs/event_semantics.md` §3)와 `kz convert --kzrt`의 결합(`kairos/docs/schema.md` §12.3.1)으로 갖췄다. 단계 C의 GCC 파일럿 dump에서 C1 15개·R2 24개 task의 소유 앱이 `confirmed`이고 F0 결합과 같다 [M]. 4번은 `kz-static`이 파생 compile DB를 받게 했고(plugin 옵션은 기록하고 front end에서 뺌), C1에서 B 사본과 B′의 facts가 생성 표 해시 하나 외에 같음을 보였다. 기준 facts를 객체 바이트 비교로 재사용하지 않고 파생 빌드마다 B′를 뽑기로 했다(`kairos/docs/static_facts.md` §7a) [M][D]. 남은 것: 단계 C 파일럿 dump에는 그 빌드의 B′가 없다(빌드 트리 삭제) — D7 실행부터 붙인다.

## 5. 단계 D–F 계획

시간표는 정하지 않는다(IR §15 머리말). 단계의 통과는 논문 기여의 확정을 뜻하지 않는다. 모든 실행은 pilot/dev 실행이다. 반복 수는 목적별로 본 실험 전에 정하며(SPEC §8.7 D14, IR §13.5), 이 문서에서 정하지 않는다. 기존 실험의 반복 수(예: confirm2의 인스턴스 4개)를 승계하지 않는다.

### 5.1 이 경로에 해당하는 대조군 (IR §13.1, SPEC §8.6)

| 군 | 이 경로에서의 구성 | 쓰는 단계 |
|---|---|---|
| G-A | 기준 GCC 빌드(`c1_build/*`, `cfs_ref/cFS`의 `build-native_std`), 추가 관측 없음 | E |
| G-F0 | 같은 빌드에 `kz observe` | D(기존 기록 smoke3·pilot3를 대조 기록으로), E |
| G-F0-H | 같은 빌드에 `kz hold` | F(confirm2를 첫 대조 자료로, IR §13.3) |
| G-I0 | `kzgcc` 파생 빌드, `KZ_RT_MODE=off` | E |
| G-I | **같은 바이너리**, `record` | D, E |
| G-I-H | 같은 바이너리, `control` + 계획 | F |
| L-A, L-I0/L-I, L-I-H | **해당 없음**(Clang으로 빌드한 실행이 없다). LLVM 경로를 다시 열면 L-A가 의무다(ADR-023) | — |

- G-I0, G-I, G-I-H는 같은 바이너리의 runtime mode로 나눈다(SPEC §8.6). 주의할 점이 셋 있다.
  - `off`에서도 POSIX 시작 객체의 생성자보다 먼저 도는 모듈 등록은 부트스트랩 버퍼에 남는다(event_semantics §8.2 [U]). 이것은 차이로 기록한다.
  - 런타임 정적 버퍼(bss 약 4.3 MB)는 모든 G-I*에 있다.
  - R2 `-g` 빌드에서는 모든 객체의 DWARF producer 문자열이 다르다(gcc §2.2).

### 5.2 단계 D: 관측 전용 런타임으로 실제 C1·R2 실행

| 순서 | 작업 | 산출물 |
|---|---|---|
| D1 | §4의 1(정체성 결합 문법·변환) | manifest 문법 개정, `kzrt_convert` 결합, 단위 시험 |
| D2 | §4의 2·7(site 범위, CF 판별 manifest, `profiles/`의 공용 논리 site 파일) | site 파일과 F0 계획 label 대응표 |
| D3 | §4의 3(필드·전역 store 형태, F0 점의 원 문장 대응) | plugin 개정과 시험 |
| D4 | §4의 5·6(런타임 수정, 버퍼 범위) | B1 개정(판을 올리면 기록) |
| D5 | §4의 4(B′와 facts, 기준 facts와 비교) | basis·facts 컨테이너와 비교 기록 |
| D6 | 파생 빌드: C1 네 빌드, R2. 단계 C의 D1–D4 차이 대장과 같은 방식으로 기록하고, 링크 뒤 hook 검사를 한다 | 빌드 기록, binding 파일 |
| D7 | G-I 실행: 단계 C와 같은 절차(`observe_c1_smoke.sh --instances`의 단계, `env/run_cfs.sh operational ci-es-restart-poweron`)로 F0 대조 기록이 있는 인스턴스부터 돈다(C1 smoke3의 tx, confirm1의 자연 rx 기록, R2 pilot3). 실행마다 S0를 기록한다(`env/tools/persist_snapshot.sh`) | dump, 실행 기록 |
| D8 | `kz convert --kzrt … --basis B′` 뒤 같은 프로필로 `kz replay`를 돌린다. IR §13.2의 여섯 단위로 F0 기록과 대조한다 | 대조표 |

진행 근거: F0가 판정한 키에서 계측 기록의 재생이 **근거 부족**(`ownership-undetermined`, 정적 닫힘 없음, 효과점 없음)으로 `INCONCLUSIVE`가 되지 않아야 한다. 판정 값 자체는 다른 실행이라 다를 수 있다. 다르면 단계 E에서 원인을 가른다(IR §13.2). 이와 함께 IR §16 / SPEC §5.12 "Checks that invalidate a run" 중 이 단계에 걸리는 것을 확인한다. 미관측 actor를 AppID 0으로 채우지 않았는지, 다시 읽은 값을 쓰지 않았는지, 함수 반환을 공개로 보지 않았는지(공개 점을 따로 둠), 런타임 순서를 HB로 쓰지 않았는지, 다른 빌드의 원시 id를 합치지 않았는지다.

### 5.3 단계 E: F0와 관측 대조

| 순서 | 작업 | 산출물 |
|---|---|---|
| E1 | 한 캠페인에서 G-A, G-F0, G-I0, G-I를 같은 커널(실행마다 기록, SPEC §1.7 Env)·같은 정지 방법·**같은 시작 상태 절차**(§7의 결정 2)로 돈다 | 군별 실행 기록 |
| E2 | site별 사건 대응: 위치 → 도달 → 포획 → 식별 → 완전성 → 판정 근거(IR §13.2). `equiv`는 근거가 있을 때만 확정한다 | 사건 대응표 |
| E3 | F0에서 보인 순서가 G-I에서 사라지거나 수가 다르면 IR §12.2의 여섯 원인을 가른다(누락 / 관측점 의미 / 코드 생성 / 런타임이 만든 대기 / 실행 시간 / 경로 미실행). 단계 C의 R2 @786 차이(Clang 11 대 F0 24)가 이 분석이 필요한 예다 | 차이의 원인표 |
| E4 | 외부 동작 비교: OPERATIONAL 시각, 적재 오류, 종료 문구, 콘솔 사건. 런타임 자원 사용도 비교한다. POSIX adapter는 OSAL 객체를 쓰지 않고(event_semantics §6) 버퍼·bss는 RUN에 기록된다 | 외부 동작 비교 |
| E5 | 비용: G-A 대 G-I0(코드 배치, 남은 분기), G-I0 대 G-I(기록). hook별 비용 계수는 B1 열린 항목이다(event_semantics §10) | 비용 기록 |

진행 근거(IR §15 E): backend 차이를 판정 의미의 차이와 분리할 수 있어야 한다.

### 5.4 단계 F: 협력적 gate

| 순서 | 작업 | 산출물 |
|---|---|---|
| F1 | `.kzh` + manifest → `kz_rt_point[]` 호스트 변환기를 만든다(`name` → validated 슬롯, `caller-in` → 호출 쪽 binding 집합, 변환할 수 없는 조건은 계획 거부). 계획을 대상에 넣는 길도 만든다(직렬화된 계획을 `control` 모드의 시작 객체가 읽음). 지금은 C 배열 API뿐이다(event_semantics §10) | 변환기, 계획 형식, 시험 |
| F2 | confirm2 계획의 게이트·해제점을 binding에 대응시킨다. 게이트는 `control` 줄이 필요하다. 지금 있는 것은 CF 조회 전(`cf_cfdp.c:1003`)과 BP 생성 전(`bp_flow.c:132`) 두 곳이고, 둘 다 `ctx=task`다. 해제점은 관측 site로 충분하다(`es.exit_app.entry`, `bp_create_call`, `cf_lookup_call`, `cf_retry_delay`, `es.child_task.entry`). F0의 4주소 한계는 이 경로에 없다(런타임 점 32) | 계획별 대응표 |
| F3 | 런타임 확인: 하네스 강제 종료·시그널 종료 경로의 dump(미확인, clang §5)는 P13의 "관측된 앞부분" 기록에 필요하다. 런타임 `timeout_ns`는 레퍼런스의 timeout에서 정한다(SPEC §4.13 Watchdog; 이 문서는 값을 정하지 않음). 하네스 안전장치는 confirm 실행에 기록된 것을 그대로 쓰고, 바꾸면 deviation이다 | 확인 기록 |
| F4 | 재생이 NCTL을 읽어 P11의 "제어 개입" 근거 종류를 만든다(B2 열린 항목). F0 `hold.kzr`도 NCTL로 바꿔 G-F0-H와 같은 층에서 비교한다 | 재생 개정, 변환 |
| F5 | 대표 계획부터 실행한다(IR §13.3, SPEC §8.6). confirm2 기록이 있는 계획 중 의미마다 하나를 고른다. (1) 생성 보류 + 종료 해제: post W1. (2) 해제된 task의 후속 해제(H6): pre W2 또는 post W10. (3) n번째 hit(H2): post W5(occurrence 2·3) 또는 W6(occurrence 27). (4) release 선도달(H5 `unless-released` 충족): W5–W12(결과표 §5.6.1). 그 뒤 confirm2의 계획 9개로 넓힌다 | 계획별 gate/release 실제 기록, 미완료 이유 |
| F6 | IR §13.3의 여섯 단위로 G-F0-H·G-I와 대조한다. 계획의 논리 순서 / 실제 보류·재개(NCTL) / 스케줄러가 낸 순서(충족된 게이트) / 최종 결과·판정 / 소진·종료·미해제(P13) / 제어 자원과 관측 비용이다. 결과의 자연성 값은 협력적 gate 실행의 값으로 따로 둔다(SPEC §1.10 rev 1.4) | 제어 대조표 |

진행 근거(IR §15 F): confirm2가 쓴 제어 의미(H1–H6)를 새 구현이 실현한다. 실현 여부와 판정은 경로와 함께 보고한다.

## 6. 다른 두 후보의 처리

- 저장소의 `instrument/clang/`, `instrument/llvm/`과 후보 문서는 단계 C의 비교 증거로 그대로 둔다. 더 키우지 않는다. 기존 시험(LLVM 빌드의 `instr_clang_transform`, LLVM 후보의 자체 빌드 시험)은 그대로 돈다.
- 다시 볼 조건 [D]:
  1. 두 번째 OS 단계(IR §15 H)에서 그 target의 교차 GCC가 plugin을 적재하지 못하면(`--enable-plugin` 없이 빌드됐거나 같은 판의 헤더가 없으면), 그 target에 한해 Clang 변환 → 교차 gcc를 다시 평가한다. 이때 매크로 안 반환 경로 미계측(§3.2의 1)이 그 target의 OSAL에서 어떤지 다시 센다.
  2. `kzgcc.cpp`의 라이선스 결정이 GPL 호환 선언과 맞지 않으면 GCC plugin은 배포할 수 없다. 그 경우 다른 두 경로를 다시 평가한다. LLVM은 plugin에 그런 선언을 요구하지 않는다(llvm §7) [F].
  3. LLVM 경로는 L-A 기준선과 T3 결정을 갖춘 별도 구성으로만 다시 연다(ADR-023).
- 넘겨받을 것: Clang C1 site 30–38의 의미 정리(OSAL 내부 점, §4의 2), Clang이 제기한 R2 시작 상태 문제(§7), LLVM이 보인 "디버그 위치 없는 `line=`" 한계. 이 한계는 GCC에는 없지만(§2 문항 3) 다른 target에서 다시 확인한다.

## 7. 열린 항목과 PI 결정

- **새 PI 결정: 없음.** 이 선택은 PD26 §2 항목 3이 채택한 규칙의 적용이다(§0).
- 이 선택에 걸린 기존 결정 요청:
  1. **`kzgcc.cpp`의 배포 라이선스**(gcc §9가 제기). GCC는 `plugin_is_GPL_compatible`을 선언한 plugin만 적재하고, 시제품에 그 선언이 있다(`kzgcc.cpp` L34–L35, L75) [F]. 저장소에 LICENSE가 없고 [F], 어느 레퍼런스도 이 연구 도구의 라이선스를 정하지 않는다. 저장소나 시제품을 외부에 공개하기 전(공개 범위도 PI 결정 대기, `plan/artifact_manifest_20261026.md`)에 정해야 한다. 단계 D–F의 내부 실행은 막지 않는다 [I]. 이 문서는 법적 판단을 하지 않는다.
  2. **파생 실행의 R2 시작 상태**(clang §9 항목 2가 제기). 선택지는 새 설치 상태(파일 없음)로 시작하는 것과, F0 기준 실행 직전 상태(0으로 찬 `EEPROM.DAT`, 빈 키 파일)를 재현하는 것이다. GCC R2 pilot도 새 설치로 시작했으므로(`instrument/gcc/scripts/run_r2.sh` 머리말 "the exe directory of the copy starts from its fresh install") [F] 이 경로에도 해당한다. SPEC §1.7은 Init을 B의 일부로 보고 실행마다 S0를 기록·정규화하라고 하지만, 새 exe 디렉터리가 어느 Init에서 시작할지는 정하지 않는다. 단계 D의 pilot은 S0를 기록하는 것으로 충분하다. 단계 E의 군 비교(G-A·G-F0·G-I0·G-I)를 근거로 세기 전에 정해야 한다.
- 이 선택으로 지금 정할 필요가 없어진 요청 [D]:
  - Clang 후보의 R2 단위 시험 범위. GCC 경로는 기준대로 단위 시험을 빌드하고 coverage 단위를 계측에서 뺀다.
  - LLVM 후보의 T3·T1·T2 허용. 이 경로에는 Clang으로 빌드한 실행이 없다. LLVM 경로를 다시 열 때 정한다.
- 열린 항목(결정 아님): §4의 1–8, 같은 바이너리를 처음부터 끄는 모드 원천과 hook별 비용 계수(event_semantics §8.2, §10), 생성자 없는 RTOS의 init/attach, 교차 툴체인의 plugin 지원(단계 H), lto1 단계 확인(기준 플래그에 LTO가 없어 D–F에는 불필요), MLIR은 D–F의 선행 조건이 아니다(IR §15.2, PD26 §2 항목 4).

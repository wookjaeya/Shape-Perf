# 단계 2a 완료 시험 T1–T7: 규칙 대상(R-T1·R-T2)과 사례 무관 질의 `kz-static relations` (2026-10-30)

- 성격: **구현과 측정 보고**다. 단계 2a의 다섯째 묶음(S5)이다. `plan/extraction_design_20261030.md`(이하 ED) §6 단계 2a 항목 2(규칙 대상 R-T1·R-T2를 공유 base로 거르고 §2.3을 다시 잼)와 항목 3(사례 무관 질의 `kz-static relations`)을 kz-static에 넣고, ED §6의 완료 시험 T1–T7을 사례 입력 없이 돌렸다. cFS를 기동하지 않았고 campaign을 돌리지 않았다. 정적 분석(kz-static)과 텍스트 처리만 했다.
- 근거: ED §3.0(입력 계층, P0 조건), §3.1–§3.4(경로 A/B/C, 반사실 뒤집힘 신호, 관계 행의 칸), §5(R1–R9), §6(항목 2·3, T1–T7, 반증 조건); `plan/input_audit_20261030.md`; `plan/REDESIGN_REVIEW_ANALYSIS_20261030.md`(이하 RA); S1–S4 보고(`plan/step2a_s1_20261030.md` … `step2a_s4_20261030.md`); `CONDITIONS_POLICY.md`.
- 코드: `kairos/lib/llvm/target_rules.cpp`·`.hpp`(새 파일: 규칙 대상), `relations.cpp`·`.hpp`(새 파일: 질의), `p0.cpp`·`p0.hpp`(비교 함수 표, 따로 된 판), `wrappers.cpp`·`wrappers.hpp`·`spec.hpp`·`serialize.cpp`(규칙 대상의 연결과 기록), `tools/kz-static/cmd_extract.cpp`·`cmd_show.cpp`·`commands.hpp`·`main.cpp`. 문서는 `kairos/docs/static_facts.md` §5e.
- 측정 드라이버: `env/redesign/step2a_tests.sh`(identity, npsame, run, twice, count, sha)와 `env/redesign/step2a_tests_count.py`. 출력은 저장소 밖 `/home/user/work/redesign/step2a/tests/`(이하 `T/`), 해시는 `T/sha256.txt`. 시험의 근거 줄은 `T/<build>/NP/relations.txt:L`(아래 "rel L")과 `T/<build>/NP/facts.txt:L`로 적는다. 증거 묶음은 `T/count/summary.txt`다.
- 도구: 병합 트리 빌드 `merge-check/kz-llvm/tools/kz-static/kz-static`(commit `4928979`의 트리; 측정은 이 commit 위에서 미커밋 변경 없이 돌렸다).
- 표기: [M] 이번에 도구로 확인, [F] 소스·문서에서 읽음, [I] 추론, [U] 미확인, [D] 이 단계의 결정.
- 사례 이름의 쓰임: 엔진 코드와 내장 표에는 사례 이름이 없다(`lint_case-ids` 통과 [M]). 사례·프레임워크 이름은 이 문서와 계수 스크립트의 **조회**(어느 행이 시험이 말하는 행인지 고르기)에만 쓴다. 프로필 없는 실행(NP)에는 프로필, 명령줄 어휘, `--target` 목록, G0 이름, 사례 rspec·`.kzm`·manifest를 주지 않았다. `--targets-rule`은 목록이 아니라 도구 규칙이다. `identity` 단계만 단계 2의 V2(사례 프로필)를 프로필 추출에 그대로 주어 그 모드가 바뀌지 않았음을 본다.

## 0. 요약

1. **구현** [M]
   - `extract --no-profile --targets-rule R-T1,R-T2`: 둘째 읽기의 추출 대상을 첫째 읽기의 확장 사실에서 규칙으로 고른다. 두 규칙 모두 공유 base(전역 객체, 전역 배열 원소, 전역 포인터·전역에서 시작한 포인터 필드의 대상, 전역 주소만 돌려주는 함수의 호출 결과)의 레코드 필드만 고른다(§1.1).
   - `kz-static relations <facts> [-o OUT]`: 경로 A·B·C의 관계와 반사실 뒤집힘 신호를 행으로 낸다. 행마다 의존 근거, 실행 순서, 실현 가능성(등급과 이유), 위반 근거, 뒤집힘 신호, 입력 계층이 따로 된 칸이다. 필드·callee·사례 이름을 받지 않는다(§1.2).
   - facts schema 개정은 **10 그대로**다. 새 섹션이 없다. 규칙 대상은 BASIS 항목(`extract.targets_rule`, `count.targets_rule*`, `target.T.f = rule …`)으로만 기록된다. relations 출력은 컨테이너가 아니라 판이 붙은 텍스트(`kz-relations 1`, "relations rules revision 1")다.
2. **기존 동작은 바뀌지 않았다** [M] (§2).
   - 시험 231/231 통과(`test_probe_anchor_reached`는 원래대로 건너뜀). S4 끝의 228에 `static_fixture_compile_relations`, `static_golden_relations`, `static_golden_relations_np` 셋을 더했다. `test_static_cli` 436개 확인(S4 378). lint 6/6.
   - 기존 golden은 하나도 바뀌지 않았다. 새 golden은 `relations.golden`, `relations_np.golden`이다.
   - 프로필 추출: V0·V2 × C1 pre tx·R2 core·C5 6.4.1 여섯 경우 모두 단계 2 출력과 섹션 CRC가 STRINGS 말고 같다. 다른 레코드는 S1–S4와 같은 셋(build identity, parent basis 해시, `units_from`)이다.
   - 프로필 없는 추출(옵션 없음): 다섯 빌드 모두 S4 facts와 레코드가 같다(§2, npsame).
   - 두 번 돌린 facts와 relations가 다섯 빌드 모두 바이트 단위로 같다(T7).
3. **완료 시험** [M] (§4). 사례 입력 없이(두 문장 내장 프로필, P0, L0, 빌드만; G0 없음) 돌렸다.

   | 시험 | 판정 | 한 줄 |
   |---|---|---|
   | T1 R2 | **PASS** | B 행 하나에 39곳, writer 3, callee 검사(`EVS_GetAppDataByID == NULL → CFE_EVS_APP_ILLEGAL_APP_ID`), 39/39 버림, 행위자 다름(F7 task-main root), 실현 가능성 "fixed unconfirmed"(생성 순서 + await)가 나온다. 뒤집힘 신호는 static-undetermined |
   | T2 C1 | **PARTIAL** | 같은 구획(상수 3), 게시 쓰기 L639/L640/L100/L1079, BP `:126 → :132` probe-create, BP 키 `"BP0_tsem"`(포인터 필드 + 이미지, 매크로 철자 `bp_semcfg.c:44`), SCH_LAB은 BP와 키 상수 불일치로 짝 아님, 수정 후 재시도(25, `-17`, F8 지연)가 나온다. 수정 전 실패 → 종료 도달은 `CF_Init`의 결과를 받은 지역 변수의 정의가 facts에 없어 **may**로만 이어진다 |
   | T3 C5 | **PASS** | 6.4.1: spawn 잠금 ∅, 등록 쓰기 {ES 잠금 #9}(L637), `CFE_ES_GetAppID` 읽기 {#9} → "possible(잠금 창)". 6.4.2: 읽기 지점마다 L800과 잠금 뒤 `GetAppIDInternal` 호출 다섯(L445, L551, L667, L1020, L1284) "proven-impossible", 오류 분기 둘(L1858, L1892) "undecidable(failure path)". "불가"는 모두 정적 인스턴스 #9로만 나온다 |
   | T4 C5 수명 | **PASS** | `OS_task_record_t.id`의 writer에 P0 `pthread_create`(out 인자 0), entry가 그대로 기본 연산으로 가서 entry 반환 경로에 표 쓰기 없음, 해제(L572/652/667/679, L761, L793)와 초기화(L279) 분류, `OS_TaskRegister` 검색에 활성 검사 없음. #72의 C 행과 다른 블록이다 |
   | T5 입력 계층 | **PARTIAL** | 다섯 빌드의 모든 행(1,177)이 B/P0/(L0)/R만이다. 사례 계층 0, G0 0. 다만 P0 항목의 원문 대조 줄은 38개 가운데 1개(`pthread_create`)뿐이고 37개와 비교 표 7개는 unverified다 |
   | T6 정상 대조 | **PASS** | C5 OSAL의 `*GetIdByName`·`*Create` 6쌍과 C1·R2의 `OS_ObjectIdFindNextMatch` 콜백 3종이 A로 인식된다. 모든 행의 위반 근거 칸은 unconfirmed다 |
   | T7 결정성 | **PASS** | facts·relations 두 번 바이트 동일(다섯 빌드) |

4. **반증 조건의 결과** [M][I] (§5). T1–T3 어느 것에도 **사례 입력을 더하지 않았고**, 세 사례의 관계가 행으로 나왔다. 그래서 ED §6의 반증 조건("사례 입력을 더하지 않으면 관계가 나오지 않음")은 **충족되지 않았다**(가설이 반증되지 않았다). 그러나 다음 셋 때문에 "사례별 Φ 없이 근거 있는 관계를 낸다"를 **성립했다고 적지 않는다**.
   - T2의 종료 도달 한 고리가 may다. 남은 수작업 입력(L0 시작 스크립트 블록, C5 `front_end_diagnostic` 2줄)과 미대조 P0 원문 37+7개도 그대로다.
   - 사례 관계는 후보 가운데 소수다. 빌드마다 행이 177–286개이고, 사례 관계 행은 1–8개(0.4–4.5%)다(§6).
   - "고정된 잘못된 순서"(R2)와 "반대 순서 가능"(C1)은 정적으로 증명되지 않는다. 실현 가능성은 등급으로만 낸다. 위반 근거는 실행 기록 없이는 늘 unconfirmed다.
5. **ED §2.3 재측정** [M] (§3). 공유 base 필터를 건 R-T1 ∪ R-T2의 대상 수는 C1 340, R2 240, C5 259/261이다(V3 R-T1: 841 / 504 / 322). 사례 필드 가운데 R2의 5개(`EVS_AppData_t.AppID`, `CFE_ES_AppRecord_t.AppId`·`AppState`·`Type`, `CFE_PSP_IdleTaskState_t.ShutdownReq`)와 C5 `CFE_ES_TaskRecord_t.TaskId`는 매개변수·지역 포인터로만 닿아 이 필터에서 빠진다.

## 1. 구현 [F][M]

### 1.1 규칙 대상 (`--targets-rule`, `lib/llvm/target_rules.cpp`, "target rules revision 1")

| 항목 | 내용 |
|---|---|
| 언제 | `extract --no-profile`과만 쓴다(프로필 모드에서 주면 exit 2). 값은 `R-T1`, `R-T2`, 또는 둘. 없으면 프로필 없는 추출은 앞과 같이 대상이 없다 |
| 무엇을 읽나 | 첫째 읽기의 확장 사실(ARGFLOW, READS, RETURNS). 이것들은 대상에 무관하다(S2–S4) |
| 공유 base [D] | ARGFLOW base가 전역, 전역 배열 원소, 전역 포인터의 대상, `object`(전역)가 있는 포인터 필드의 대상, 또는 **모든 반환값이 전역 객체(의 일부)의 주소이거나 null인 함수**의 호출 결과. 마지막 것은 RETURNS로 정한다: 주소 식의 가장 안쪽 위치가 전역이거나, 쓴 그대로의 주소가 `&`와 읽기가 이름 붙인 전역 객체 이름으로 시작한다. 그런 함수의 호출 결과를 돌려주는 함수도 고정점까지 넣는다(유한) |
| R-T1 | 호출 인자의 원천이 필드 읽기 또는 필드 주소이고 base가 공유인 레코드 필드 |
| R-T2 | 공유 base로(READS 뿌리가 전역, 또는 위 함수의 호출 결과) 읽어 반환하거나 포인터 매개변수를 통해 저장하는 필드, 또는 분기에서 읽고 그 함수가 상수를 돌려주는 조건(RETURNS)에도 나오는 필드 |
| 기록 | BASIS `target.T.f = rule R-T1` 등(정책 `unset`, 출처 행 없음: 사람이 넣은 입력이 아니다), `extract.targets_rule`, `count.targets_rule`·`_t1`·`_t2`·`_both`·`_t1_unshared_only`·`_t2_unshared_only`. extract 요약에 `rule targets` 줄 |
| 영향 | 둘째 읽기에 대상이 생기므로 WRITERS, FACT_ATYPE, FACT_LOCS, FACT_INIT, OUTCALLS와 옛 L1 (i)(FACT_LOOPS)가 대상에 따라 나온다. 대상 무관 섹션(FIELD_STATUS, AWAIT_L1, LOCK_*)은 S4 규칙 그대로다 |

### 1.2 `kz-static relations` (`lib/llvm/relations.cpp`, "relations rules revision 1")

규칙 전문은 `kairos/docs/static_facts.md` §5e다. 요점만 적는다.

- **입력**: facts 컨테이너 하나. 확장 사실이 없으면 이유와 함께 거부한다(exit 1). 받는 옵션은 `-o`뿐이다. 필드·callee·사례 이름을 받지 않는다.
- **머리**: facts 해시, 모드, 내장 프로필·P0 표·유도 규칙·대상 규칙의 판, **P0 비교 표**(따로 된 판 "P0 comparison table revision 1": 바이트 문자열 비교 `memcmp`·`strcmp`·`strcoll`·`strncmp`·`strcasecmp`·`strncasecmp`, 스레드 id 비교 `pthread_equal`; 모두 POSIX.1-2017 / glibc 2.39, 원문 대조 unverified), 행위자 root와 트램펄린, 경로·등급별 수.
- **정적 행위자** [D]: root = `main`, 해결된 spawn entry(CALL_TARGETS, table walk 반복의 task-main, 시작 스크립트 진입). root의 코드는 다른 root에 들어가지 않고 호출 그래프로 닿는 함수다. process가 아닌 root가 다른 root를 부르면 트램펄린이고(R2·C1의 `CFE_ES_TaskEntryPoint`, `OS_PthreadTaskEntry`), 그 코드는 그것이 부르는 root들의 것이다. 필드 기반 may 근사다.
- **경로 B**: 공유 base 필드 인자를 (필드, 본문 있는 callee, 인자 위치)로 묶는다. writer가 없거나 한 행위자뿐이면 뺀다. 칸: 사이트와 결과 처리, INIT·writer(대상이면 WRITERS의 값·guard), 행위자, callee 검사(그 매개변수가 조건에 나오는 상수 반환, 그리고 조건 안에서 그 매개변수로 부르는 함수의 RET_CONSTS 한 단계).
- **경로 C**: 해결된 entry가 있는 spawn마다, 생성자 함수가 spawn 뒤(소스 위치)에 쓰는 필드와 spawn된 entry의 행위자 코드가 그 필드를 분기·반환·out 저장으로 읽는 곳의 짝이다. 생성자 정체성(spawn 연산의 out-store 매개변수), 쓰기 첨자(WRITERS key), 읽기 첨자가 자기 정체성인지(P0 thread-self·thread-specific-get에 닿는 사슬)를 적는다. 읽기가 정적 인스턴스 잠금을 잡지 않으면 그 함수의 호출 지점마다 행을 따로 낸다.
- **경로 A**: 키 비교(P0 비교 표 함수 호출, 또는 분기의 `==`/`!=`)가 레코드 필드와 매개변수 사이에 있고, 그것이 검색 루프 안(A1)이거나 검색 루프의 간접 호출이 부르는 함수 안(A2)이며, 검색이 그 형을 전역 표에서 읽을 때 이름 공간(레코드 형, 키 필드)을 만든다. PARAM_FLOW로 키 매개변수 함수를 모으고, 바깥에서의 호출을 사용으로, 키 필드 쓰기에 닿는 것을 생성자로 나눈다. 구획은 사슬의 첫 상수 인자, 키 값은 문자열·상수 또는 PTR_FIELDS + GLOBAL_IMAGES다. 짝에서 빼는 이유(키 상수 다름, 한 행위자, probe-create)를 적는다. 처리: 감싼 루프(종류·횟수·탈출식·지연과 F8 사슬), 결과를 돌려주는 함수 사슬, 그 길의 종료 연산.
- **칸(ED §3.4)**: `dependency`, `run-order`(이 판은 기록을 읽지 않는다: 늘 `no record given`), `feasibility`(`proven-impossible`/`possible`/`undecidable` + 이유), `violation`(RV2 §5.4; 기록 없으면 `unconfirmed`), `flip`(`static-possible`/`static-undetermined`/`none` + 이유), `layers`(B, P0, L0, R; 프로필 facts면 `case:profile(…)`, 명령줄 어휘가 있으면 `hand-entered:command-line(…)`).
- **실현 가능성** [D]: C는 must 정체성(LOCK_IDS static-instance)으로만 "불가"를 낸다. B는 한 table walk의 생성 순서와 생성 뒤 같은 반복의 await(AWAIT_L1 필드 단위)를 적고 `undecidable`(fixed unconfirmed)로 낸다. A는 늘 `undecidable`이다.
- **결정성**: 정렬된 맵만 쓰고, 식 트리는 쓰는 쪽의 후위 번호(자식 < 부모)를 따라 걷는다(깊이 상한 없음). 텍스트의 식 출력은 `show`와 같은 40단계 출력 한계를 쓴다. 이 한계로 잘린 식은 다섯 빌드의 relations에 0개다 [M].

## 2. 기존 동작 불변 확인 [M]

| 확인 | 결과 | 근거 |
|---|---|---|
| 시험 | 231/231 통과(1개 건너뜀은 원래대로). 새 시험 3개. `test_static_cli` 436개 확인: `--targets-rule`의 옵션 조합·값·기록(규칙 대상은 `unset`, 사람 입력 아님), R-T1만일 때 분기에서만 읽히는 필드가 빠짐, 옵션 없으면 대상 0; relations의 사용법·거부(확장 사실 없는 facts)·`-o`·결정성·칸·계층, 프로필 facts의 사례 계층, 명령줄 어휘의 hand-entered 표시. lint 6/6 | ctest 로그 |
| 기존 golden | 바뀐 파일 0. 새 golden 2(`relations.golden`: 고정 도구 프로필 + `--extended-facts`, 모든 행에 `case:profile(static-fixtures / test data)`; `relations_np.golden`: `--no-profile --targets-rule R-T1,R-T2`, relations 두 번 같은 바이트) | `git status` |
| 프로필 추출(identity) | V0·V2 × C1 pre tx·R2 core·C5 6.4.1: basis 6섹션·facts 22섹션 가운데 CRC가 다른 것은 STRINGS뿐. 다른 레코드는 basis 1(`front_end.build_identity`), facts 3(`extract.build_identity`, parent basis 해시, `extract.units_from`). show 텍스트 차이 2줄 | `T/identity/identity.txt` |
| 프로필 없는 추출(npsame) | S4 basis에서 `--targets-rule` 없이 다시 뽑은 facts가 S4 facts와 STRINGS 말고 37개 섹션 모두 레코드가 같다(§2a) | `T/npsame/npsame.txt` |
| 결정성(T7) | 다섯 빌드 facts·relations 두 번 같음 | `T/twice.txt` |

### 2a. npsame (S4 프로필 없는 facts 대 S5 도구, 옵션 없음)

다섯 빌드 모두 STRINGS 말고 37개 섹션(BASIS, FACTS … AWAIT_L1)의 레코드가 S4 facts와 같다. 다른 섹션 0 [M]. 즉 `--targets-rule`을 주지 않으면 S5 도구의 프로필 없는 facts는 S4와 같다. P0 비교 표와 relations는 facts를 바꾸지 않는다.

## 3. ED §2.3 재측정: 공유 base의 R-T1 ∪ R-T2 [M]

`T/count/targets_rule.tsv`, `summary.txt` 끝 절. "FACT_LOCS"는 옛 규칙(대상 집합에 따라 바뀜, SF §4.1), "FIELD_STATUS"는 S4의 대상 무관 규칙(필드 단위 / 형 단위)이다. 괄호 안은 ED §2.3의 V3(G0 + R-T1, 공유 필터 없음).

| 빌드 | 대상(R-T1 / R-T2 / 둘 다) | FACT_LOCS complete / DISABLED | writer 0 / 1 / 2–5 / >5 | DISABLED 원인(겹침) untyped / escape / cast / union / unknown | FIELD_STATUS complete: 필드 단위 / 형 단위 |
|---|---|---|---|---|---|
| c1_pre_tx | 340 (308 / 53 / 21) (V3 841) | 142 / 198 (208 / 633) | 44 / 57 / 193 / 46 (174 / 107 / 422 / 138) | 198 / 124 / 71 / 10 / 16 (633 / 244 / 435 / 154 / 151) | 194 / 118 |
| c1_post_tx | 340 (308 / 53 / 21) | 142 / 198 | 44 / 57 / 193 / 46 | 198 / 124 / 71 / 10 / 16 | 194 / 118 |
| r2_core | 240 (220 / 34 / 14) (V3 504) | 110 / 130 (200 / 304) | 20 / 47 / 143 / 30 (101 / 52 / 257 / 94) | 130 / 90 / 50 / 2 / 0 (304 / 137 / 196 / 33 / 12) | 148 / 96 |
| c5_641 | 259 (189 / 99 / 29) (V3 322) | 74 / 185 (61 / 261) | 29 / 45 / 130 / 55 (91 / 68 / 115 / 48) | 185 / 117 / 68 / 0 / 0 (261 / 111 / 171 / 0 / 4) | 146 / 67 |
| c5_642 | 261 (188 / 102 / 29) | 74 / 187 | 29 / 44 / 133 / 55 | 187 / 119 / 68 / 0 / 0 | 148 / 67 |

공유 base가 아니어서 빠진 필드(그 규칙이 고를 수 있었으나 매개변수·지역·다른 호출 결과로만 닿음): R-T1 537 / 537 / 293 / 132 / 132, R-T2 355 / 355 / 246 / 85 / 85.

**사례 필드** (평가용 조회; `facts.txt` 줄은 `summary.txt` 끝 절)

| 빌드 | 필드 | 규칙 대상 | FACT_LOCS(옛 규칙) | FIELD_STATUS 필드 단위 / 형 단위 |
|---|---|---|---|---|
| C1 | `OS_common_record_t.name_entry` | R-T1 | complete, writer 18 | complete / complete |
| C1 | `OS_common_record_t.active_id` | R-T1 | complete, 6 | complete / complete |
| C1 | `BP_Throttle_t.Sem` | 둘 다 | complete, 2 | complete / complete |
| C1 | `BP_Throttle_t.Name` | R-T1 | complete, 0 | complete / complete |
| C1 | `CF_Channel_t.sem_id` | R-T1 | DISABLED, 2 | DISABLED / DISABLED |
| C1 | `CF_ChannelConfig_t.sem_name` | 둘 다 | DISABLED, 0 | DISABLED / DISABLED |
| R2 | `CFE_SB_Global_t.AppId` | R-T1 | **DISABLED**, 3 | complete / DISABLED |
| R2 | `EVS_AppData_t.AppID`, `CFE_ES_AppRecord_t.AppId`·`AppState`·`Type`, `CFE_PSP_IdleTaskState_t.ShutdownReq` | **아님** | — | 모두 complete / (AppRecord 셋 DISABLED, 둘 complete) |
| C5 | `CFE_ES_TaskRecord_t.RecordUsed` | R-T2 | complete, 7 | complete / complete |
| C5 | `CFE_ES_TaskRecord_t.AppId` | R-T2 | complete, 3 | complete / complete |
| C5 | `CFE_ES_TaskRecord_t.TaskId` | **아님** | — | complete / complete |
| C5 | `OS_task_record_t.id` | 둘 다 | DISABLED, 6 | DISABLED / DISABLED |
| C5 | `OS_task_record_t.free` | R-T2 | DISABLED, 13 | complete / DISABLED |

읽기 [I]:
- **공유 필터는 대상을 1/2–1/2.5로 줄인다.** C1의 `timespec.tv_nsec` 같은 지역 base 대상은 사라졌다(ED §2.3 첫 예).
- **ED §2.3의 "R-T1이 놓치는 사례 필드"는 R-T2로 일부만 들어왔다.** C5 `RecordUsed`는 R-T2로 들어왔다(전역 뿌리에서 분기로 읽히고 `CFE_ES_ERR_APPID` 반환 조건에 나온다). R2의 `AppState`·`Type`·`ShutdownReq`와 C5 `TaskId`는 들어오지 않았다. 이 필드들은 `AppRecPtr->AppState`처럼 지역 포인터(정의가 조회 함수 호출)나 매개변수 포인터로만 읽힌다. ARGFLOW는 지역 포인터의 정의를 따라가지만 READS는 따라가지 않는다. 그래서 공유 base로 판정되지 않는다. ED §6 항목 2가 요구한 필터를 그대로 걸었을 때의 결과다.
- **옛 FACT_LOCS 규칙의 비단조는 그대로다.** R2 `CFE_SB_Global_t.AppId`는 규칙 대상에서도 DISABLED다(V3와 같음). 대상 무관 FIELD_STATUS는 필드 단위 complete, 형 단위 DISABLED로 S4와 같다. T1의 writer 3개는 WRITERS에서 온다(완전성 상태와 무관).
- **R2 await와 옛 규칙.** R-T1 ∪ R-T2 대상의 옛 L1 (i)(FACT_LOOPS)은 `CFE_ES_MainTaskSyncDelay` 루프에서 여전히 fails다("the body writes `CFE_ES_PerfMetaData_t.FilterMask` … (and 35 more)", r2 `facts.txt:21004`; V3는 133곳). 대상 무관 AWAIT_L1 필드 단위 규칙은 await다(`facts.txt:90765–90766`). relations는 AWAIT_L1을 쓴다.

## 4. 완료 시험 T1–T7 (사례 입력 없음) [M]

근거 줄은 `T/<build>/NP/relations.txt`의 줄 번호(rel L)다. 모든 행은 `T/count/summary.txt`에서도 같은 줄 번호로 나온다.

### T1 R2 — **PASS**

**통과 조건(ED §6)**: B 관계 `CFE_SB_Global_t.AppId → CFE_EVS_SendEventWithAppID#2`; 39 지점; 쓰기 3(EarlyInit 0, AppInit out-pointer 둘); callee 검사 `EVS_GetAppDataByID == NULL → CFE_EVS_APP_ILLEGAL_APP_ID`(F2); 결과 버림 39/39; 읽는 쪽과 쓰는 쪽 행위자 다름(G0 없이, F7의 task-main root로); 순서 조건: 코어 초기화 순서가 고정됨(생성 순서 + `CFE_ES_MainTaskSyncDelay` await)을 보이거나, 보이지 못하면 "고정 미확인"을 실현 가능성 칸에 적음.

**나온 것** (r2_core)
- 행 B65(rel 2133): `CFE_SB_Global_t.AppId -> CFE_EVS_SendEventWithAppID#2 sites=39 callee-check=yes`. 사이트 39곳(`cfe_sb_api.c` 34, `cfe_sb_priv.c` 1, `cfe_sb_task.c` 4), base 전역, hops 0.
- 필드(rel 2173): INIT 0(`CFE_SB_Global`), writer 3, WRITERS(규칙 대상 R-T1, `facts.txt:107`).
  - rel 2174: memset `cfe_sb_init.c:50:5` in `CFE_SB_EarlyInit`, 값 0.
  - rel 2175: out-pointer `cfe_sb_task.c:138:5` in `CFE_SB_AppInit` via `CFE_ES_GetAppID`, 값 symbolic `[CFE_ES_AppRecordGetID(AppRecPtr)]`, guard `AppRecPtr != 0`.
  - rel 2176: 같은 곳, 값 0 `[CFE_ES_APPID_UNDEFINED]`, guard `AppRecPtr == 0`.
- 행위자(rel 2177): 읽는 쪽 {`CFE_ES_BackgroundTask`, `CFE_ES_TaskMain`, `CFE_EVS_TaskMain`, `CFE_SB_TaskMain`, `CFE_TBL_TaskMain`, `CFE_TIME_Local1HzTask`, `CFE_TIME_TaskMain`, `CFE_TIME_Tone1HzTask`}, 쓰는 쪽 {`CFE_SB_TaskMain`, `main`} → inter-actor. task-main root는 table walk 반복(F7이 P0에서 유도한 `CFE_ES_StartAppTask`)에서 오고, `CFE_ES_TaskEntryPoint`·`OS_PthreadTaskEntry`는 트램펄린으로 빠진다(머리의 actors 줄).
- callee 검사(rel 2179): `CFE_EVS_SendEventWithAppID returns 3254779907 [CFE_EVS_APP_ILLEGAL_APP_ID] when (and (not (eq param:Spec#3 0)) (eq (call-result EVS_GetAppDataByID (seq param:AppID#2)) 0))`. 한 단계 아래(rel 2182): `EVS_GetAppDataByID returns 0 [NULL] when (not (and (eq (call-result CFE_ES_AppID_ToIndex …) 0) (lt local:AppIndex 32)))`.
- 처리(rel 2184): `discarded 39, used 0, tested 0`.
- 실현 가능성(rel 2187): `undecidable: fixed unconfirmed: creation order at the table walk cfe_es_start.c:810:26 in CFE_ES_CreateObjects: writer actor CFE_SB_TaskMain at iteration 4; reader actors created before it: CFE_ES_TaskMain (iteration 1), CFE_EVS_TaskMain (iteration 2); after it: CFE_TBL_TaskMain (5), CFE_TIME_TaskMain (6). The call cfe_es_start.c:835:26 (CFE_ES_MainTaskSyncDelay) after the creation reaches the await loop cfe_es_start.c:871:5 (AWAIT_L1 per-field rule; type-level star-closed) …; neither order is proven`.
- 뒤집힘 신호(rel 2189): `static-undetermined` — 쓰기 값 하나가 상수가 아니고, 검사 조건이 `EVS_GetAppDataByID` 등의 호출 결과를 읽는다. 위반 근거(rel 2188) unconfirmed, 계층(rel 2190) `B P0 R (writers: rule targets)`.

**판정**: 조건이 모두 나왔다. 순서 조건은 "고정 미확인"으로 칸이 채워졌다. 정적으로 고정을 증명하지는 못했다: 생성 순서와 await는 나오지만, 사용이 그 행위자의 대기 상태 앞인지는 정적으로 정하지 않는다. await 인정은 S4의 필드 단위 완전성 규칙과 그 선언된 가정(`assume.no_cross_member_pointer_arithmetic`)에 기댄다. 형 단위 규칙으로는 star-closed다. 값 0이 검사를 통과하지 못함(ED §3.2)은 `CFE_ResourceId_ToIndex`의 상수 접기가 없어 평가 불가로 남는다.

### T2 C1 — **PARTIAL**

**통과 조건**: CF `cf_cfdp.c:993`(수정 후 `:1003`)과 BP `bp_flow.c:132`가 같은 구획; 게시 쓰기 L639/L640, L100, L1079; BP `:126 → :132` probe-create; BP 키 `"BP0_tsem"`(포인터 필드 흐름 + 이미지, 출처 `bp_semcfg.c` L43–L44)이 풀리거나 "필드 points-to 필요"로 표시; 수정 전 실패 분기 → 종료 도달(F2/F4, 종료 연산은 F8로 P0에서 유도); 수정 후 재시도(25, `!= -17`, 지연. 지연 연산은 F8로 P0에서 유도); SCH_LAB 생성은 키 상수 불일치로 짝 아님.

**나온 것** (c1_pre_tx; c1_post_tx는 같은 줄 번호에 같은 내용이고 다른 곳만 적는다)
- 이름 공간 `OS_common_record_t.name_entry`(rel 320): 검색 A2(rel 321) `OS_ObjectIdFindNextMatch`의 루프 `osapi-idmap.c:550:5`가 간접 호출(`:561:54`)로 `OS_ObjectNameMatch`의 `strcmp`(`:271:40`)에 닿는다. 활성 필드(rel 322) `OS_common_record_t.active_id`. 표 `OS_common_table`.
- 게시 쓰기(rel 367, 생성자 `OS_CountSemCreate#1`이 직접 호출로 닿는 쓰기): `active_id` `osapi-idmap.c:639:9`(`OS_ObjectIdFindNextFree`), `:1079:9`·`:1088:9`(`OS_ObjectIdTransactionFinish`), `:432:21`·`:519:13`(`OS_ObjectIdConvertToken`); `name_entry` `osapi-countsem.c:100:9`(`OS_CountSemCreate`), `osapi-idmap.c:640:9`(`FindNextFree`, := 0).
- CF 조회 행 A21(rel 403): `cf_cfdp.c:993:19 in CF_CFDP_InitEngine -> OS_CountSemGetIdByName#1 … partition {3} key unresolved`. 구획(rel 404) `3 [LOCAL_OBJID_TYPE] as OS_ObjectIdFindByName arg 0`. 키(rel 405) `read through the pointer field CF_AppData_t.config_table: needs field points-to (PTR_FIELDS)`. 생성자(rel 407) BP `bp_flow.c:132:18` `OS_CountSemCreate`, 같은 구획, "key unresolved (lookup)". 생성자(rel 408) SCH_LAB `sch_lab_app.c:168:16` `"SCH_LAB"`, 같은 이유로 짝 후보.
- BP 조회 행 A20(rel 389): `bp_flow.c:126:13 … partition {3} key known`. 키(rel 391): `read through the pointer field BP_GlobalData_t.Throttles -> BP_Throttles (PTR_FIELDS resolved), elements from 0 step 1 count 2; image BP_Throttles[0].Name = "BP0_tsem" (macro CF_CHANNEL_0_SEM_NAME, spelled at …/bp_semcfg.c:44:31); image BP_Throttles[1].Name = "BP1_tsem" (…:48:31)`. 짝 아님(rel 393): probe-create(`:132`, 같은 함수, 조회 결과 아래, 같은 키 인자). 짝 아님(rel 394): SCH_LAB, `key constants differ`.
- 수정 전 처리(rel 409–412): 루프 `cf_cfdp.c:967:5` constant-trip 2; `CF_CFDP_InitEngine returns the result of OS_CountSemGetIdByName`(rel 410); `CF_Init returns the result of CF_CFDP_InitEngine`(rel 411); 종료(rel 412) `terminal CFE_ES_ExitApp at cf_app.c:366:5 in CF_AppMain: the field CF_AppData_t.run_status := CFE_ES_RunStatus_APP_ERROR at cf_app.c:333:9 under (ne local:status 0) (the guard tests a local; the result of CF_Init is used in CF_AppMain: may) is argument 0 of it … (F8 may CFE_ES_ExitApp -> CFE_ES_ResetCFE -> CFE_PSP_Restart -> abort)`.
- 수정 후 처리(c1_post_tx rel 409): `in the loop cf_cfdp.c:1001:13 kind count-bounded trip upper-bound 25 exit (or (not (lt local:j 25)) (and (lt local:j 25) (ne local:ret -17))) delay cf_cfdp.c:1011:17 OS_TaskDelay (F8 must OS_TaskDelay -> OS_TaskDelay_Impl -> clock_nanosleep)`.
- 실현 가능성(rel 415) undecidable(생성 순서가 초기화 순서를 정하지 않는다), 뒤집힘(rel 417) static-undetermined(CF 키 미해결; 검색 반환 −17 / 0), 계층(rel 418) `B P0 L0 R`(CF·BP는 시작 스크립트 진입 root).

**판정**: 조건 일곱 가운데 여섯이 그대로 나왔다. 미달은 "수정 전 실패 분기 → 종료 도달"의 한 고리다. 반환 사슬(InitEngine → `CF_Init`)과 종료 연산의 F8 사슬(→ `abort`)은 나오지만, `CF_AppMain`에서 `CF_Init`의 결과가 지역 변수 `status`에 들어가고 그 `status`가 `run_status = APP_ERROR`의 guard라는 연결은 facts에 없다. WRITERS guard가 지역 변수를 펼치지 않기 때문이다. 그래서 그 고리를 may로 낸다.
- 더해야 할 입력: 사례 입력이 아니다. 계층 B/R의 사실 확장이다. WRITERS·TERMINALS guard의 지역 변수를 RETURNS처럼 정의로 펼치는 것(정의가 하나인 지역, 또는 도달 정의)이다. 기존 섹션을 바꾸므로 새 섹션으로 내야 한다.
- 참고: BP 키의 출처는 매크로 정의 줄 `bp_semcfg.c:44`(ED는 `#ifndef`를 포함해 L43–L44)다. `#ifndef` 기본값이라는 점은 AST가 주지 않는다(S3 열린 문제 6).
- 덤으로 생긴 것 [M]: CF·BP 조회는 이름 공간 `OS_filesys_internal_record_t.virtual_mountpt`에도 행이 있다(rel 924, 828). `OS_ObjectIdFindNextMatch`의 간접 호출이 필드 기반 may로 그 콜백에도 닿기 때문이다. 이 이름 공간에서는 생성자 짝이 없다. 후보 부담에 들어간다(§6).

### T3 C5 — **PASS**

**통과 조건**: spawn `CFE_ES_AppCreate:614`의 잠금 집합 ∅; 등록 쓰기 L649–L652의 잠금 집합 {ES 잠금}(F5 must 정체성 `CFE_ES_Global_t.SharedDataMutex`); 자식 `CFE_ES_GetAppID`의 `RecordUsed` 검사(F1)와 `CFE_ES_ERR_APPID` 반환(F2); "반대 순서 가능(잠금 창)". 6.4.2에서 읽기 지점마다: L800과 잠금 뒤 `GetAppIDInternal` 호출(L445, L551, L667, L1020, L1284) "불가 증명", `LockSharedData`·`UnlockSharedData` 오류 분기(L1858, L1892) "가능" 또는 "결정 불가". must 정체성이 없는 잠금 쌍으로 "불가"가 나오면 실패.

**나온 것**
- 6.4.1 (c5_641)
  - spawn(rel 2954): `callee OS_TaskCreate, entries CI_Lab_AppMain SAMPLE_AppMain SAMPLE_LibInit SCH_Lab_AppMain TO_Lab_AppMain`(F7 + 시작 스크립트 진입, L0); 생성자 정체성 `arg 0 [&CFE_ES_Global.AppTable[i].TaskInfo.MainTaskId] (OS_TaskCreate stores through it)`; `locks held {}`.
  - 등록 쓰기(rel 2955): `cfe_es_apps.c:649:18 key=CFE_ES_Global.TaskTable[TaskId] value=1 [TRUE] locks {#9 &OS_mut_sem_table[CFE_ES_Global.SharedDataMutex].id (static-instance)} acquired-at [cfe_es_apps.c:637:15]`. 같은 spawn의 `AppId`(L651)·`TaskId`(L652) 쓰기는 각 필드의 행에 있다.
  - 행 C31(rel 2953) `CFE_ES_GetAppID` L861 읽기: 첨자 `(call-result OS_TaskGetId)`, `OS_TaskGetId -> pthread_getspecific (P0 thread-specific-get)`(rel 2957); 잠금 {#9} L851(rel 2956); **`possible: lock window`**(rel 2960); 뒤집힘(rel 2962) `static-possible: CFE_ES_GetAppID returns 3288334337 [CFE_ES_ERR_APPID] with the earlier value (INIT 0), not with the written value 1 [TRUE]`.
  - `GetAppIDInternal` L1886의 호출 지점별 행 C33–C39(rel 2974–3049): 잠금을 쥔 호출 다섯은 모두 possible(잠금 창), 오류 분기 둘(L1919, L1953)은 undecidable(failure path).
- 6.4.2 (c5_642)
  - spawn(rel 2908): entries 같음, `locks held {#9 …} acquired-at [cfe_es_apps.c:571:12]`; 등록 쓰기(rel 2909) L652 {#9}, 같은 획득 L571.
  - C30(rel 2907) `CFE_ES_GetAppID` L800: 잠금 {#9} L790(rel 2910) → **`proven-impossible`**(rel 2914).
  - `GetAppIDInternal` L1825를 부르는 호출마다: L445 `CFE_ES_ExitApp`(C32, rel 2935), L551 `CFE_ES_RunLoop`(C33, rel 2946), L667 `CFE_ES_WaitForStartupSync`(C34, rel 2957), L1020 `CFE_ES_CreateChildTask`(C35, rel 2968), L1284 `CFE_ES_ExitChildTask`(C36, rel 2979) → 모두 **`proven-impossible`**. L1858 `CFE_ES_LockSharedData`(C37, rel 2990), L1892 `CFE_ES_UnlockSharedData`(C38, rel 3001) → **`undecidable`**(failure path).
  - L667·L1020·L1284 행에는 "the spawned entries do not reach this call"이 붙는다. 시작 스크립트의 다섯 앱이 이 함수들을 부르지 않는다는 뜻이다(행위자 귀속은 may).
- "불가"는 다섯 빌드를 통틀어 C 행에서만 나오고(6.4.1 14, 6.4.2 28), 모두 static-instance 정체성 #9다. 정적 인스턴스가 아닌 잠금(#7·#8·#10–#14, C1·R2의 undecidable)으로는 "불가"가 하나도 없다 [M].

**판정**: 조건이 모두 나왔다. 정체성 #9가 정적 인스턴스라는 판정은 S4의 필드 단위 완전성 규칙과 두 선언된 가정(`assume.static_instance_read_after_write`, `assume.no_cross_member_pointer_arithmetic`), 그리고 `assume.lock_acquisition_succeeds`에 기댄다(S4 §1.3–§1.4). 형 단위 규칙을 고르면 #9는 정적 인스턴스가 아니고, 6.4.2의 "불가"는 "결정 불가"가 된다 [I].

### T4 C5 수명 — **PASS**

**통과 조건**: `OS_task_table.id`의 writer에 `pthread_create`(P0) 포함; 진입 함수가 그냥 반환하는 경로에 해제 없음; 해제 쓰기는 `OS_TaskDelete`·`OS_TaskExit`와 `OS_TaskCreate` 실패 경로(L572, L652, L667, L679), 초기화는 `OS_API_Init`(L279)로 따로 분류; `OS_TaskRegister` 검색에 활성 검사 없음. #72 관계와 다른 행.

**나온 것** (c5_641; c5_642는 같은 줄 번호에 같은 내용)
- 이름 공간 `OS_task_record_t.id`(rel 1267): 검색 `OS_TaskRegister`의 루프 `osapi.c:921:5`, 키 `id ==`(L923)가 지역 `pthread_id`와 비교(rel 1269); `OS_FindCreator` 루프(L3786)의 `pthread_equal`(rel 1268).
- 활성 검사(rel 1270): `active fields: none in this namespace's searches; no activity check: other searches of OS_task_record_t test OS_task_record_t.free`. 활성 값(rel 1271–1272): `free == 0`은 사용 중, `free == 1`은 빈 칸(`OS_TaskGetIdByName`·`OS_TaskCreate`의 키 비교 검색에서).
- P0 writer(rel 1294): `pthread_create (P0 thread-create) stores through arg 0 [&(OS_task_table[possible_taskid].id)] at osapi.c:645:19 in OS_TaskCreate`.
- 진입 경로(rel 1295): `OS_TaskCreate parameter 2 … passed unchanged to the thread-creation primitive …; no function of this program runs after that entry returns, so the table has no write on that path`.
- `free` 쓰기 분류(rel 1274–1285): L279 `OS_API_Init` class=initialization; L551·L697 class=reservation or activation; L572·L652·L667·L679 `OS_TaskCreate` class=release(guard는 각 실패 조건); L761 `OS_TaskDelete` class=release; L793 `OS_TaskExit` class=release. `id` := 0(L766, L798)은 빈 칸 값 시험이 없어 분류하지 않는다.
- 이 블록은 이름 공간 블록이고, #72의 C 행(§T3)과 따로 나온다.

**판정**: 조건이 모두 나왔다. 단 `OS_TaskRegister`의 비교 상대가 `pthread_self()`라는 것은 facts가 지역 변수 `pthread_id`의 정의를 담지 않아 행에 나오지 않는다(T2와 같은 공백). 활성 검사 없음은 비교 상대와 무관하게 나온다.

### T5 입력 계층 — **PARTIAL**

**통과 조건**: 위 행마다 입력 계층이 B/P0/L0/R만이다. 사례 계층 0, G0 0. P0·L0 항목마다 판과 원문 줄.

**나온 것** (`summary.txt` T5 절)
- 행 수와 계층: c1_pre 286, c1_post 286, r2 248, c5_641 180, c5_642 177, 합 1,177행. 계층은 `B P0 R`, `B P0 L0 R`(시작 스크립트 진입 root에 기댈 때)과 그 둘에 `(writers: rule targets)`가 붙은 것뿐이다. `case:`·`hand-entered:` 표시가 붙은 행 0.
- 머리(rel 4–10): `mode profile-free`, 내장 프로필 두 문장, P0 표 판, 유도 규칙 판, 대상 규칙 판, P0 비교 표 판과 항목. extract 요약: `hand-entered inputs 38 (built-in P0 table 38, command line 0)`(다섯 빌드).
- P0 표(facts `[p0]`): 38항목 모두 판 표기(POSIX.1-2017 / glibc 2.39; `bzero`는 POSIX 밖으로 표기). 원문 대조 줄은 `pthread_create` 1개(glibc 2.39 `nptl/pthread_create.c` L624/L755/L804/L836), 37개는 `source unverified`. P0 비교 표 7항목도 모두 unverified.
- L0: recipe의 `startup_script` 블록은 판마다 `cite` 줄을 단다(basis, S1 보고). 이번에 새로 대조하지 않았다.

**판정**: 사례 계층과 G0이 0이라는 핵심 조건은 통과다. "P0 항목마다 원문 줄" 조건은 44개(37 + 비교 7)가 unverified라서 미달이다.
- 더해야 할 입력: P0 계층이다. glibc 2.39 upstream 원문(`string/strcmp.c` 등, `nptl/pthread_mutex_lock.c` 등)의 로컬 사본과 각 항목의 줄이다. 사례 입력이 아니다. 로컬에 있는 glibc 사본은 `c5/ref/glibc`의 `pthread_create` 셋뿐이다(S1).

### T6 정상 대조 — **PASS**

**통과 조건**: C5 OSAL `*GetIdByName`·`*Create` 6쌍(task, queue, binsem, countsem, mutsem, timer)과 C1·R2의 `OS_ObjectIdFindNextMatch` 콜백 3종이 A로 인식된다. 정상 호출자는 신호만 내고 위반 확정은 내지 않는다.

**나온 것**
- C5(두 판 같음): 이름 공간마다 `key-parameter functions` 줄. `OS_task_record_t.name`(rel 1298) lookups `OS_TaskGetIdByName#1` / creators `CFE_ES_AppCreate#3 CFE_ES_CreateChildTask#1 OS_TaskCreate#1`; `OS_queue_record_t.name`(rel 1244) `OS_QueueGetIdByName#1` / `CFE_SB_CreatePipe#2 OS_QueueCreate#1`; `OS_bin_sem_record_t.name`(rel 1156), `OS_count_sem_record_t.name`(rel 1176), `OS_mut_sem_record_t.name`(rel 1219), `OS_timer_record_t.name`(rel 1329) 각 `GetIdByName#1` / `Create#1`.
- C1(두 판)·R2: A2 콜백 검색 3종. 키 `OS_common_record_t.name_entry`(`strcmp`, C1 rel 321 / R2 rel 87), `OS_filesys_internal_record_t.virtual_mountpt`(`strncmp`, rel 745 / 343), `OS_impl_task_internal_record_t.id`(`pthread_equal`, rel 1869 / 1033).
- 위반 근거 칸: 다섯 빌드 1,177행 모두 `unconfirmed`.

**판정**: 조건대로 나왔다. 다만 "정상 호출자는 위반 확정을 내지 않는다"는 이번에 기록을 주지 않았기 때문에 자명하게 성립한다. 기록이 있을 때의 동작은 시험하지 않았다(§7 항목 1).

### T7 결정성 — **PASS**

`T/twice.txt`: 같은 basis에서 extract(`--targets-rule R-T1,R-T2`)와 relations를 다시 돌렸다. facts가 다섯 빌드 모두 같다(`ca3f9cff…`, `ca15701d…`, `466e7683…`, `78ff966a…`, `dbbf3016…`). relations도 같다(`ba7b4c82…`, `71b02914…`, `1cc215a1…`, `e91e9b31…`, `0fb230f2…`). fixture golden은 relations를 두 번 돌려 같은 바이트를 요구한다.

## 5. 반증 조건 (ED §6, RV2 §11 단계 2, §12)

- **조건**: T1–T3 가운데 하나라도 사례 입력을 더하지 않으면 관계가 나오지 않으면, 가설("사례별 Φ 없이 근거 있는 관계를 낸다")은 아직 성립하지 않은 것으로 보고한다.
- **이번 결과** [M]: T1–T3은 사례 입력(프로필, 명령줄 어휘, `--target`, G0, rspec·`.kzm`·manifest) 없이 돌렸고, 세 사례의 관계 행이 나왔다(T1 B65, T2 A20·A21, T3 C30–C39). 반증 조건은 충족되지 않았다. 사례 입력을 더해야 했던 시험은 없다.
- **그래도 "성립"이라고 적지 않는 이유** [I]
  1. T2의 종료 도달 한 고리가 may다. 사례 입력으로 메울 것은 아니고, 지역 변수 정의를 펼치는 사실 확장(계층 B/R)이 필요하다.
  2. "근거 있는"의 무게가 고르지 않다. T1의 순서는 "고정 미확인"이고, C1의 반대 순서 가능성도 "결정 불가"다. 위반 근거는 실행 기록이 있어야 생긴다(RV2 §5.4). T3의 "불가 증명"은 선언된 가정 셋과 필드 단위 규칙에 기댄다.
  3. 남은 수작업 입력이 있다. 사례 무관이지만 사람이 쓴 것이다. L0(recipe의 시작 스크립트 블록), C5 `front_end_diagnostic` 2줄(PI D7), unverified P0 원문 44개, P0·비교 표 자체가 그렇다.
  4. 사례 관계는 행 177–286개 가운데 1–8개다(§6). 질의는 사례 관계를 골라내지 않는다. 고르는 것은 이 문서의 평가용 조회다.
  5. 다섯 사례는 개발 집합이다(ED §5 R7). 동결 뒤 고른 구성으로 따로 평가하지 않았다.

## 6. 후보 부담 [M]

`T/count/burden.tsv`. "사례 관계 행"은 평가용 조회로 고른 행이다. C1은 CF `:993`(수정 후 `:1003`)과 BP `:126` 조회 각각이 두 이름 공간에 나오므로 4행이다. C5는 `CFE_ES_AppCreate` spawn × `RecordUsed` × `GetAppID`/`GetAppIDInternal` 읽기 지점 8행이다.

| 빌드 | A(조회 사용) | B | C | 합 | 이름 공간 블록 | 사례 관계 행(비율) | flip static-possible | feasibility proven-impossible |
|---|---|---|---|---|---|---|---|---|
| c1_pre_tx | 127 | 135 | 24 | 286 | 13 | 4 (1.4%) | 1 | 0 |
| c1_post_tx | 127 | 135 | 24 | 286 | 13 | 4 (1.4%) | 1 | 0 |
| r2_core | 68 | 155 | 25 | 248 | 8 | 1 (0.4%) | 1 | 0 |
| c5_641 | 66 | 73 | 41 | 180 | 31 | 8 (4.4%) | 16 | 14 |
| c5_642 | 66 | 71 | 40 | 177 | 31 | 8 (4.5%) | 16 | 28 |

- 경로별 남는 행의 거름(머리 줄): B는 필드–인자 묶음 가운데 writer 없음과 한 행위자뿐을 뺀 것이다. B 가운데 callee 검사가 있는 행은 25 / 25 / 29 / 26 / 24다. A는 이름 공간 13 / 13 / 8 / 31 / 31개의 조회 사용이고, 같은 구획의 다른 행위자 생성자가 있는 조회는 26 / 26 / 20 / 2 / 2, 키까지 같은 것은 1 / 1 / 1 / 0 / 0이다.
- 뒤집힘 신호 static-possible은 1 / 1 / 1 / 16 / 16행이다. C1·R2의 1행은 같은 키가 정적으로 맞는 조회–생성 짝 하나다. 사례 관계(T1, T2)는 static-undetermined에 들어 있다. 신호만으로 고르면 사례 관계를 놓친다.
- 넘치는 원인 [I]: (a) 필드 기반 may 간접 호출(T2 덤 참고: 한 조회가 여러 콜백 이름 공간에 들어간다), (b) 행위자 귀속의 과대 근사(트램펄린이 부르는 모든 root가 그 코드를 공유), (c) B의 inter-actor 기준이 `main`(process)을 writer 행위자로 세어 초기화 쓰기와 task 읽기를 모두 짝으로 만든다.
- 시간(이 VM, 한 번, 비용 주장 아님): NP extract(`--targets-rule`) 42.0 / 40.1 / 26.0 / 9.2 / 9.7 s(S4 41.1 / 39.3 / 25.5 / 8.6 / 9.4 s), relations 1.3 / 1.8 / 0.9 / 0.6 / 0.8 s(`T/run.log`).

## 7. 조건 기록 (`CONDITIONS_POLICY.md`)

| 조건 | 값 | 상태 | 근거·이유 |
|---|---|---|---|
| 분석 대상 빌드 | S1–S4와 같은 기준 빌드 5 | from-reference | ED §1.1, §1.4 |
| 대상 규칙 | R-T1 ∪ R-T2, 공유 base만 | 도구 규칙(연구 조건 아님), "target rules revision 1" | ED §6 항목 2. 호출 결과 base를 전역 주소만 돌려주는 함수로 넓힌 것은 ED의 "F4 원천이 전역인 포인터"를 이 판에서 읽은 방식이다 [D] |
| relations 규칙 | §1.2 | 도구 규칙, "relations rules revision 1" | ED §3.1–§3.4, §6 항목 3 |
| P0 비교 표 | 7항목, 별도 판 | 도구 내부 지원(P0 계층). 판 표기 POSIX.1-2017 / glibc 2.39, 원문 대조 unverified | 추출이 쓰지 않으므로 P0 표 판 1과 facts의 P0_TABLE은 그대로다 |
| 수치 도구 매개변수 | 새것 없음 | — | 대상 규칙과 relations에 상한이 없다. 식 트리 걷기는 후위 번호를 따라 끝난다. 식 텍스트의 40단계 출력 한계(`show`와 같음)로 잘린 식은 다섯 빌드 0개 [M]. 기존 F4 상한(`--argflow-depth`·`--argflow-size`)은 기본값 none, `--image-budget` 4096(S3, unspecified_by_reference) 그대로 |
| 선언된 가정 | S4의 셋 | S4 보고 §4와 같음 | T3의 "불가"가 기댄다 |
| 완전성 규칙의 선택 | relations와 AWAIT 인정은 필드 단위 규칙을 쓴다 | 결정 대기(S4 열린 문제 1) | 형 단위를 고르면 T1의 await와 T3의 "불가"가 사라진다 [I] |
| 실행 기록 | 없음 | 이번 시험의 조건 | ED §6: 사례 입력 없이. run-order 칸은 늘 `no record given` |
| L0, C5 `front_end_diagnostic` | S1과 같음 | S1 보고 §4와 같음 | — |

## 8. 열린 문제

1. **실행 기록 입력.** run-order 칸을 채우는 `--record`는 이 판에 없다. 기록의 neutral 사이트(파일:줄)를 행의 쓰기·사용 위치에 맞추는 일이 남았다. 그 전까지 위반 근거 칸은 늘 unconfirmed다.
2. **지역 변수 정의의 사실.** WRITERS·TERMINALS·READS ctx의 guard에 남는 지역 변수(`status = CF_Init()`, `pthread_id = pthread_self()`)를 정의로 펼치는 사실이 없다. T2의 종료 고리와 T4의 비교 상대가 여기에 걸린다. 기존 섹션을 바꾸지 않으려면 새 섹션이 필요하다.
3. **공유 base 판정의 범위.** READS는 지역 포인터의 정의를 따라가지 않는다. 그래서 `AppRecPtr->AppState` 같은 사례 필드가 R-T2에서 빠진다. ARGFLOW의 `base-via-local`과 같은 규칙을 READS에도 두면 넓어진다(새 섹션 또는 relations 쪽 계산).
4. **필드 기반 may의 부담.** A의 간접 호출 콜백 혼입(한 조회가 세 콜백 이름 공간에), 트램펄린 귀속, process 행위자를 writer로 세는 B 기준이 행 수를 키운다(§6). 사슬의 함수 포인터 인자(예: `OS_ObjectNameMatch`를 고정해 넘기는 호출)로 콜백을 좁히는 규칙이 다음 후보다.
5. **상수 접기.** R2의 "초기 값 0은 검사를 통과하지 못함"(ED §3.2)은 본문 있는 함수의 상수 접기가 없어 static-undetermined로 남는다. 값은 C int32가 아니라 캐스트를 벗긴 값으로 출력된다(S2 열린 문제 4: `3254779907`).
6. **C의 "spawn 뒤"는 소스 위치다.** CFG 순서가 아니다. 이번 사례에서는 소스 위치와 CFG 순서가 같다는 것을 원문으로 확인했다 [F]. 일반에는 지배 관계가 필요하다.
7. **P0 원문 대조.** 37 + 7항목 unverified(T5). glibc 2.39 upstream 사본이 필요하다.
8. **완전성 규칙의 PI 결정**(S4 열린 문제 1)과 **facts의 프로필 해시 결합**(ED §5 R8)은 그대로 대기다. relations는 프로필 facts를 받되 모든 행에 사례 계층을 단다.
9. **구획 비교**는 상수 값 집합의 같음이다. 상수가 둘 이상인 사슬(예: `OS_remove`의 `{1,11}`)에서 다른 의미의 상수가 섞일 수 있다.

## 9. 재현

```sh
R=/home/user/Shape-Perf/research/cfs_procrace
$R/env/redesign/step2a_tests.sh all     # identity npsame run twice count sha -> /home/user/work/redesign/step2a/tests
```

- 각 빌드 `T/<build>/NP/`에 `basis.kz`·`basis.txt`·`facts.kz`·`facts.txt`·`extract.txt`·`relations.txt`·`args.txt`·`sha256.txt`가 있다. `NP2/`는 T7의 두 번째 relations다.
- `T/count/summary.txt`(T1–T7 증거, 줄 참조), `burden.tsv`, `targets_rule.tsv`, `summary.tsv`. `T/identity/identity.txt`, `T/npsame/npsame.txt`, `T/twice.txt`, `T/run.log`.

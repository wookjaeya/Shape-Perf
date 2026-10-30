# 단계 2a 완료 시험 T1–T7: 규칙 대상(R-T1·R-T2)과 사례 무관 질의 `kz-static relations` (2026-10-30)

- 성격: **구현과 측정 보고**다. 단계 2a의 다섯째 묶음(S5)과 그 **수정 단계**(검증 지적 13건, §10)다. `plan/extraction_design_20261030.md`(이하 ED) §6 단계 2a 항목 2(규칙 대상 R-T1·R-T2를 공유 base로 거르고 §2.3을 다시 잼)와 항목 3(사례 무관 질의 `kz-static relations`)을 kz-static에 넣고, ED §6의 완료 시험 T1–T7을 사례 입력 없이 돌렸다. 수정 단계는 지적을 고치고 같은 시험을 다시 돌렸다. 이 문서의 수와 판정은 수정 단계의 출력이다. cFS를 기동하지 않았고 campaign을 돌리지 않았다. 정적 분석(kz-static)과 텍스트 처리만 했다.
- 근거: ED §3.0(입력 계층, P0 조건), §3.1–§3.4(경로 A/B/C, 반사실 뒤집힘 신호, 관계 행의 칸), §5(R1–R9), §6(항목 2·3, T1–T7, 반증 조건); `plan/input_audit_20261030.md`; `plan/REDESIGN_REVIEW_ANALYSIS_20261030.md`(이하 RA); S1–S4 보고(`plan/step2a_s1_20261030.md` … `step2a_s4_20261030.md`); `CONDITIONS_POLICY.md`.
- 코드: `kairos/lib/llvm/target_rules.cpp`·`.hpp`(규칙 대상), `relations.cpp`·`.hpp`(질의), `p0.cpp`·`p0.hpp`(P0 표·비교 표), `lockset.cpp`·`.hpp`(F5와 LOCK_PATHS), `model.hpp`·`serialize.cpp`·`facts_text.cpp`, `wrappers.cpp`·`wrappers.hpp`·`spec.hpp`(규칙 대상의 연결과 기록), `include/kz/schema/kz_facts.h`·`kz_schema.h`, `tools/kz-static/cmd_extract.cpp`·`cmd_show.cpp`·`commands.hpp`·`main.cpp`. 문서는 `kairos/docs/static_facts.md` §5e·§5f, `kairos/docs/schema.md` §19.
- 측정 드라이버: `env/redesign/step2a_tests.sh`(identity, npsame, run, s5same, twice, count, sha)와 `env/redesign/step2a_tests_count.py`. 출력은 저장소 밖 `/home/user/work/redesign/step2a/tests/`(이하 `T/`), 해시는 `T/sha256.txt`(119 파일). S5 때의 출력은 `/home/user/work/redesign/step2a/tests_s5/`(이하 `T_S5/`)에 그대로 두었다. 시험의 근거 줄은 `T/<build>/NP/relations.txt:L`(아래 "rel L")과 `T/<build>/NP/facts.txt:L`로 적는다. 증거 묶음은 `T/count/summary.txt`다.
- 도구: 병합 트리 빌드 `merge-check/kz-llvm/tools/kz-static/kz-static`. 측정은 commit `b76a9a9`의 트리에서 `kairos` 아래 미커밋 변경 없이 돌렸다(`T/identity/identity.txt` 머리).
- 표기: [M] 이번에 도구로 확인, [F] 소스·문서에서 읽음, [I] 추론, [U] 미확인, [D] 이 단계의 결정.
- 사례 이름의 쓰임: 엔진 코드와 내장 표에는 사례 이름이 없다(`lint_case-ids` 통과 [M]). 사례·프레임워크 이름은 이 문서와 계수 스크립트의 **조회**(어느 행이 시험이 말하는 행인지 고르기)에만 쓴다. 프로필 없는 실행(NP)에는 프로필, 명령줄 어휘, `--target` 목록, G0 이름, 사례 rspec·`.kzm`·manifest를 주지 않았다. `--targets-rule`은 목록이 아니라 도구 규칙이다. `identity` 단계만 단계 2의 V2(사례 프로필)를 프로필 추출에 그대로 주어 그 모드가 바뀌지 않았음을 본다.

## 0. 요약

1. **구현** [M]
   - `extract --no-profile --targets-rule R-T1,R-T2`: 둘째 읽기의 추출 대상을 첫째 읽기의 확장 사실에서 규칙으로 고른다. 두 규칙 모두 공유 base(전역 객체, 전역 배열 원소, 전역 포인터·전역에서 시작한 포인터 필드의 대상, 전역 주소만 돌려주는 함수의 호출 결과)의 레코드 필드만 고른다(§1.1). 조건 트리 걷기에 깊이 상한이 없다(수정 단계).
   - `kz-static relations <facts> [-o OUT]`: 경로 A·B·C의 관계와 반사실 뒤집힘 신호를 행으로 낸다. 행마다 의존 근거, 실행 순서, 실현 가능성(등급과 이유), 위반 근거, 뒤집힘 신호, 입력 계층이 따로 된 칸이다. 필드·callee·사례 이름을 받지 않는다(§1.2). 수정 단계에서 규칙 개정 2가 되었다.
   - **facts schema 개정**: S5는 10 그대로였다. 수정 단계에서 **11**이 되었다(추출 레코드 개정 9). 새 섹션 LOCK_PATHS(136)와 P0 종류 넷을 더했다. 기존 섹션과 레코드 배치는 그대로다(`docs/schema.md` §19).
   - **P0 표 개정 2**(72항목, 개정 1은 38)와 **P0 비교 표 개정 2**(13항목, 개정 1은 7). 묶음을 통째로 채우고, 이 개정이 담지 않는 묶음을 BASIS `extract.p0_table_scope`와 relations 머리에 이름으로 적는다(§1.3).
2. **기존 동작은 바뀌지 않았다** [M] (§2).
   - 시험 235/235 통과(`test_probe_anchor_reached`는 원래대로 건너뜀). S5 끝의 231에 fixture `relpaths.c`·`deepcond.c`의 compile 시험 둘과 `static_golden_lockpaths_np`, `static_golden_relpaths_np`를 더했다. `test_static_cli` 475개 확인(S5 436). lint 6/6.
   - golden: **S5 단독으로는** 기존 golden이 바뀌지 않았다(새 golden `relations.golden`, `relations_np.golden`만 더함). **단계 2a 전체로는** S1–S4가 schema 개정 6→10에 따라 schema dump golden 둘(`tests/golden/data/layout.golden`, `records.golden`)을 기계적으로 다시 만들었다(S1–S4 보고). 수정 단계는 개정 11로 그 둘을 다시 만들었고, 2a의 프로필 없는 golden(`wrappers`, `flowfacts_np`, `argflow_np`, `lockset_np`)은 P0 표 개정 2의 줄(P0 수·판 두 줄, `wrappers`의 P0 행 34개)만 바뀌었으며, S5의 relations golden 둘은 규칙 개정 2로 바뀌었다. 프로필 추출의 golden은 하나도 바뀌지 않았다.
   - 프로필 추출: V0·V2 × C1 pre tx·R2 core·C5 6.4.1 여섯 경우 모두 단계 2 출력과 섹션 CRC가 STRINGS 말고 같다. 다른 레코드는 S1–S5와 같은 셋(build identity, parent basis 해시, `units_from`)이다.
   - 프로필 없는 추출: S4·S5 facts와 비교하면 BASIS(P0 항목·주석의 추가)와 P0_TABLE(행 34개 추가, 이전 38행은 그대로)이 다르고 LOCK_PATHS가 새로 있다. 나머지 35개 섹션의 레코드는 같다(§2a).
   - 두 번 돌린 facts와 relations가 다섯 빌드 모두 바이트 단위로 같다(T7).
3. **완료 시험** [M] (§4). 사례 입력 없이(두 문장 내장 프로필, P0, L0, 빌드만; G0 없음) 돌렸다.

   | 시험 | 판정 | 한 줄 |
   |---|---|---|
   | T1 R2 | **PASS** | B 행 하나에 39곳, writer 3, callee 검사(`EVS_GetAppDataByID == NULL → CFE_EVS_APP_ILLEGAL_APP_ID`), 39/39 버림, 행위자 다름(F7 task-main root), 실현 가능성 "fixed unconfirmed"(생성 순서 + await)가 나온다. 뒤집힘 신호는 static-undetermined. layers 칸이 필드 단위 규칙(PI 대기)에 기댐을 적는다 |
   | T2 C1 | **PARTIAL** | 같은 구획(상수 3), 게시 쓰기 L639/L640/L100/L1079, BP `:126 → :132` probe-create, BP 키 `"BP0_tsem"`(포인터 필드 + 이미지, 매크로 철자 `bp_semcfg.c:44`), SCH_LAB은 BP와 키 상수 불일치로 짝 아님, 수정 후 재시도(25, `-17`, F8 지연)가 나온다. 수정 전 실패 → 종료 도달은 둘 다 미달: (a) `CF_Init`의 결과를 받은 지역 변수의 정의가 facts에 없어 may, (b) `CFE_ES_ExitApp`의 F8 종료 호출 둘의 guard가 CF가 넘기는 값 3(`APP_ERROR`)으로 모두 거짓이고, 남는 길은 탈출 없는 지연 루프다 |
   | T3 C5 | **PASS** | 6.4.1: spawn 잠금 ∅, 등록 쓰기 {ES 잠금 #9}(L637), `CFE_ES_GetAppID` 읽기 {#9} → "possible(잠금 창)". 6.4.2: 읽기 지점마다 L800과 잠금 뒤 `GetAppIDInternal` 호출 다섯(L445, L551, L667, L1020, L1284) "proven-impossible", 오류 분기 둘(L1858, L1892) "undecidable(failure path)". "불가"는 모두 정적 인스턴스 #9로만 나오고, 행이 기댄 가정과 규칙을 적는다 |
   | T4 C5 수명 | **PASS** | `OS_task_record_t.id`의 writer에 P0 `pthread_create`(out 인자 0; relations가 ARGFLOW와 P0_TABLE을 맞춘 것이고 추출의 writer 집합에는 없음), entry가 그대로 기본 연산으로 가서 entry 반환 경로에 표 쓰기 없음(소멸자 있는 key·cleanup push가 없음을 확인한 뒤), 해제(L572/652/667/679, L761, L793)와 초기화(L279) 분류, `OS_TaskRegister` 검색에 활성 검사 없음 |
   | T5 입력 계층 | **PARTIAL** | 다섯 빌드의 모든 행(1,436)이 B/P0/(L0)/R만이다. 사례 계층 0, G0 0. 다만 P0 항목의 원문 대조 줄은 72개 가운데 1개(`pthread_create`)뿐이고 71개와 비교 표 13개는 unverified다. 사람이 넣은 basis 편차(C5 `front_end_diagnostic`, PI D7)에 기대는 행 159·156개와 선언된 가정·PI 대기 규칙에 기대는 행 55·14·30개를 layers 칸이 따로 적는다 |
   | T6 정상 대조 | **PASS** | C5 OSAL의 `*GetIdByName`·`*Create` 6쌍과 C1·R2의 `OS_ObjectIdFindNextMatch` 콜백 3종이 A로 인식된다. 모든 행의 위반 근거 칸은 unconfirmed다 |
   | T7 결정성 | **PASS** | facts·relations 두 번 바이트 동일(다섯 빌드) |

4. **반증 조건의 결과** [M][I] (§5). T1–T3 어느 것에도 **사례 입력을 더하지 않았고**, 세 사례의 관계가 행으로 나왔다. 그래서 ED §6의 반증 조건("사례 입력을 더하지 않으면 관계가 나오지 않음")은 **충족되지 않았다**(가설이 반증되지 않았다). 그러나 다음 때문에 "사례별 Φ 없이 근거 있는 관계를 낸다"를 **성립했다고 적지 않는다**.
   - T2의 종료 도달이 미달이다. 지역 변수 정의의 사실이 없고, 종료 연산의 F8 유도가 CF의 길에서는 P0 종료에 닿지 않는다(그 스레드는 탈출 없는 루프에서 멈추고, 다른 행위자가 끝낸다).
   - 남은 수작업 입력(L0 시작 스크립트 블록, C5 `front_end_diagnostic` 2줄)과 미대조 P0 원문 71+13개가 그대로다.
   - 사례 관계는 후보 가운데 소수다. 빌드마다 행이 175–392개이고, 사례 관계 행은 1–8개(0.3–4.6%)다(§6).
   - "고정된 잘못된 순서"(R2)와 "반대 순서 가능"(C1)은 정적으로 증명되지 않는다. 실현 가능성은 등급으로만 낸다. 위반 근거는 실행 기록 없이는 늘 unconfirmed다.
5. **ED §2.3 재측정** [M] (§3). 공유 base 필터를 건 R-T1 ∪ R-T2의 대상 수는 C1 340, R2 240, C5 259/261이다(V3 R-T1: 841 / 504 / 322). 수정 단계에서 바뀌지 않았다. 사례 필드 가운데 R2의 5개(`EVS_AppData_t.AppID`, `CFE_ES_AppRecord_t.AppId`·`AppState`·`Type`, `CFE_PSP_IdleTaskState_t.ShutdownReq`)와 C5 `CFE_ES_TaskRecord_t.TaskId`는 매개변수·지역 포인터로만 닿아 이 필터에서 빠진다.

## 1. 구현 [F][M]

### 1.1 규칙 대상 (`--targets-rule`, `lib/llvm/target_rules.cpp`, "target rules revision 1")

| 항목 | 내용 |
|---|---|
| 언제 | `extract --no-profile`과만 쓴다(프로필 모드에서 주면 exit 2). 값은 `R-T1`, `R-T2`, 또는 둘. 없으면 프로필 없는 추출은 앞과 같이 대상이 없다 |
| 무엇을 읽나 | 첫째 읽기의 확장 사실(ARGFLOW, READS, RETURNS). 이것들은 대상에 무관하다(S2–S4) |
| 공유 base [D] | ARGFLOW base가 전역, 전역 배열 원소, 전역 포인터의 대상, `object`(전역)가 있는 포인터 필드의 대상, 또는 **모든 반환값이 전역 객체(의 일부)의 주소이거나 null인 함수**의 호출 결과. 마지막 것은 RETURNS로 정한다: 주소 식의 가장 안쪽 위치가 전역이거나, 쓴 그대로의 주소가 `&`와 읽기가 이름 붙인 전역 객체 이름으로 시작한다. 그런 함수의 호출 결과를 돌려주는 함수도 고정점까지 넣는다(유한) |
| R-T1 | 호출 인자의 원천이 필드 읽기 또는 필드 주소이고 base가 공유인 레코드 필드 |
| R-T2 | 공유 base로(READS 뿌리가 전역, 또는 위 함수의 호출 결과) 읽어 반환하거나 포인터 매개변수를 통해 저장하는 필드, 또는 분기에서 읽고 그 함수가 상수를 돌려주는 조건(RETURNS)에도 나오는 필드. 조건 트리는 깊이 상한 없이 걷는다: 풀이 같은 트리를 한 노드로 두고 노드를 자식 뒤에 만들므로 방문 집합으로 끝난다(수정 단계; S5에는 깊이 200 상한이 남아 있었다, §10 지적 1) |
| 기록 | BASIS `target.T.f = rule R-T1` 등(정책 `unset`, 출처 행 없음: 사람이 넣은 입력이 아니다), `extract.targets_rule`, `count.targets_rule`·`_t1`·`_t2`·`_both`·`_t1_unshared_only`·`_t2_unshared_only`. extract 요약에 `rule targets` 줄 |
| 영향 | 둘째 읽기에 대상이 생기므로 WRITERS, FACT_ATYPE, FACT_LOCS, FACT_INIT, OUTCALLS와 옛 L1 (i)(FACT_LOOPS)가 대상에 따라 나온다. 대상 무관 섹션(FIELD_STATUS, AWAIT_L1, LOCK_*)은 S4 규칙 그대로다 |

### 1.2 `kz-static relations` (`lib/llvm/relations.cpp`, "relations rules revision 2")

규칙 전문은 `kairos/docs/static_facts.md` §5e(개정 1)와 §5f(개정 2에서 바뀐 것)다. 요점만 적는다.

- **입력**: facts 컨테이너 하나. 확장 사실이 없으면 이유와 함께 거부한다(exit 1). 개정 2는 LOCK_PATHS(schema 개정 11)도 요구한다. 받는 옵션은 `-o`뿐이다. 필드·callee·사례 이름을 받지 않는다.
- **머리**: facts 해시, 모드, 내장 프로필·P0 표(판과 `p0_table_scope`)·유도 규칙·대상 규칙의 판, **P0 비교 표**(판, 항목, 담지 않는 묶음), facts에 복사된 basis 편차, 행위자 root(종류별 수)와 트램펄린, 다른 root에 들어가지만 행위자로 남은 root, 정적 이미지로 푼 spawn, entry를 풀지 못한 spawn과 그 이유, 경로·등급별 수.
- **정적 행위자** [D]: root = `main`, 해결된 spawn entry(CALL_TARGETS, table walk 반복의 task-main, 시작 스크립트 진입), 그리고 spawn의 entry 인자가 정적 이미지가 있는 전역 표의 필드에서 오면 그 멤버의 함수 잎(`table-image-entry`, 수정 단계). root의 코드는 다른 root에 들어가지 않고 호출 그래프로 닿는 함수다. 트램펄린(행위자가 아니고 그 코드는 그것이 부르는 root들의 것)은 process가 아닌 root 가운데 **다른 root에 간접 호출로만 들어가고, 제 본문이 공유 root로 필드·전역을 읽지 않으며 전역을 쓰지 않는** 것이다(수정 단계; S5는 "다른 root를 부르면" 모두 트램펄린이었다, §10 지적 9). 다섯 빌드에서는 R2·C1의 `CFE_ES_TaskEntryPoint`, `OS_PthreadTaskEntry`가 트램펄린이고 S5와 같다 [M]. 필드 기반 may 근사다.
- **경로 B**: 공유 base 필드 인자를 (필드, 본문 있는 callee, 인자 위치)로 묶는다. writer가 없거나 한 행위자뿐이면 뺀다. 칸: 사이트와 결과 처리, INIT·writer(대상이면 WRITERS의 값·guard), 행위자, callee 검사(그 매개변수가 조건에 나오는 상수 반환, 그리고 조건 안에서 그 매개변수로 부르는 함수의 RET_CONSTS 한 단계).
- **경로 C**: 해결된 entry가 있는 spawn마다, 생성자 함수가 spawn **뒤**에 쓰는 필드와 spawn된 entry의 행위자 코드가 그 필드를 분기·반환·out 저장으로 읽는 곳의 짝이다. "뒤"는 수정 단계부터 LOCK_PATHS `after-spawn`(생성자의 제어 흐름 그래프에서 spawn 호출 뒤에 닿는 사건; 되돌아가는 간선 포함, 분기 가지치기 없음)이다. S5는 소스 위치였다(§10 지적 3). 생성자가 spawn 뒤에 부르는 함수의 같은 필드 쓰기도 실현 가능성에 넣는다(한 호출 아래는 호출 지점의 잠금과 합치고, 더 아래는 제 잠금만 센다). 본문 있는 유도 spawn 연산이 주소 인자를 통해 저장하는 필드(PARAM_FLOW out-store, ARGFLOW 원천이 그 필드의 주소)는 spawn 뒤의 쓰기로 세고 잠금은 모르는 것으로 둔다: CFG 순서에서 인자 사건은 호출보다 앞이고, 그 호출 안에서 스레드 생성과의 순서는 facts가 주지 않기 때문이다. P0 스레드 생성 연산의 out 인자는 새 스레드가 돌기 전에 저장된다고 보고(P0 의미) 세지 않는다. 생성자 본문에서는 쓰지 않고 callee에서만 쓰는 필드는 짝짓지 않고 `[C]` 머리에 수를 낸다. 생성자 정체성(spawn 연산의 out-store 매개변수), 쓰기 첨자(WRITERS key), 읽기 첨자가 자기 정체성인지(P0 thread-self·thread-specific-get에 닿는 사슬)를 적는다. 읽기가 정적 인스턴스 잠금을 잡지 않으면 그 함수의 호출 지점마다 행을 따로 내고, 읽기 행에 LOCK_PATHS `entry-release`가 있으면 호출 지점의 잠금을 읽기에 더하지 않는다(수정 단계, §10 지적 4).
- **경로 A**: 키 비교(P0 비교 표 함수 호출, 또는 분기의 `==`/`!=`)가 레코드 필드와 매개변수 사이에 있고, 그것이 검색 루프 안(A1)이거나 검색 루프의 간접 호출이 부르는 함수 안(A2)이며, 검색이 그 형을 전역 표에서 읽을 때 이름 공간(레코드 형, 키 필드)을 만든다. PARAM_FLOW로 키 매개변수 함수를 모으고, 바깥에서의 호출을 사용으로, 키 필드 쓰기에 닿는 것을 생성자로 나눈다. 구획은 사슬의 첫 상수 인자, 키 값은 문자열·상수 또는 PTR_FIELDS + GLOBAL_IMAGES다. 짝에서 빼는 이유(키 상수 다름, 한 행위자, probe-create)를 적는다. 처리: 감싼 루프(종류·횟수·탈출식·지연과 F8 사슬), 결과를 돌려주는 함수 사슬, 그 길의 종료 연산. 종료 연산이 may·noreturn이면 그 종료 호출의 guard와, 이 호출에서 인자가 갖는 상수 값으로 그 guard를 평가한 결과, 그 함수의 탈출 없는 루프를 적는다(수정 단계, §10 지적 2).
- **수명**(경로 A 이름 공간 블록): 키·활성 필드의 writer(covers-more-than-the-field를 따로 셈), P0 out 인자 쓰기(relations가 ARGFLOW와 P0_TABLE을 맞춘 것이고 추출의 writer 집합에는 없다고 적음), entry 경로 규칙(프로그램에 null이 아닌 소멸자를 단 thread-specific key 생성도 cleanup push도 없을 때만 "entry가 돌아온 뒤 프로그램의 함수가 돌지 않는다"고 씀), 블록의 `namespace layers` 줄.
- **칸(ED §3.4)**: `dependency`, `run-order`(이 판은 기록을 읽지 않는다: 늘 `no record given`), `feasibility`(`proven-impossible`/`possible`/`undecidable` + 이유; 수정 단계부터 "불가"는 기댄 가정과 규칙을 적는다), `violation`(RV2 §5.4; 기록 없으면 `unconfirmed`), `flip`(`static-possible`/`static-undetermined`/`none` + 이유), `layers`(B, P0, L0, R; 프로필 facts면 `case:profile(…)`, 명령줄 어휘가 있으면 `hand-entered:command-line(…)`; 수정 단계부터 `basis-deviation(…)`과 `rests-on(…)`).
- **실현 가능성** [D]: C는 must 정체성(LOCK_IDS static-instance)으로만 "불가"를 낸다. B는 한 table walk의 생성 순서와 생성 뒤 같은 반복의 await(AWAIT_L1 필드 단위)를 적고 `undecidable`(fixed unconfirmed)로 낸다. A는 늘 `undecidable`이다.
- **결정성**: 정렬된 맵만 쓰고, 식 트리는 쓰는 쪽의 후위 번호(자식 < 부모)를 따라 걷는다(깊이 상한 없음). 텍스트의 식 출력은 `show`와 같은 40단계 출력 한계를 쓴다. 이 한계는 출력에만 쓴다: callee 검사와 상수 반환의 중복 제거는 (값, 조건 트리)로 한다(수정 단계; S5는 잘린 텍스트를 키로 썼다, §10 지적 11). 이 한계로 잘린 식은 다섯 빌드의 relations에 0개다 [M].

### 1.3 LOCK_PATHS와 P0 표 개정 2 (수정 단계)

- **LOCK_PATHS**(facts schema 개정 11, 섹션 136) [D]: `after-spawn`(spawn 호출 뒤에 그 함수의 제어 흐름 그래프로 닿는 LOCKSET 사건)과 `entry-release`(함수 진입에서 그 사건까지의 어떤 경로에서 함수가 잡지 않은 잠금을 풀거나, 정적 인스턴스가 아닌 정체성의 잠금을 지울 수 있거나, 효과를 따라가지 못하는 호출이 있음; `site`는 소스 위치가 가장 앞선 그런 호출). LOCKSET과 같은 분석에서 구하고 LOCKSET 행은 바뀌지 않는다 [M: §2a]. 다섯 빌드의 행 수: 9,330 / 9,331 / 5,803 / 2,335 / 2,344(after-spawn 234 / 234 / 242 / 198 / 203, spawn 호출 15 / 15 / 14 / 7 / 7) [M].
- **P0 표 개정 2** [D]: 개정 1의 묶음에 C11 `<threads.h>`의 같은 묶음 함수(`thrd_create`, `mtx_*`, `cnd_*`, `thrd_sleep`, `thrd_exit`, `thrd_current`, `tss_*`), glibc 2.39의 `*_clock*` 함수(`pthread_mutex_clocklock`, `pthread_cond_clockwait`, `sem_clockwait`), spin lock, `usleep`, `quick_exit`, assert 매크로가 부르는 `__assert_fail`·`__assert_perror_fail`, thread-specific key 생성·삭제, cleanup handler push·pop(`pthread_cleanup_push/pop` 매크로가 예외 없이 컴파일한 단위에서 부르는 함수), `dlmopen`·`dlvsym`을 더했다. 72항목, 항목마다 판 표기, 원문 대조는 `pthread_create` 하나다. 담지 않는 묶음(읽기-쓰기 잠금 전체, barrier, one-time init, 다른 스레드의 취소, 시그널, `dlclose`·`dlerror`·`dladdr`, 컴파일러·fortify가 원문 호출 없이 부르는 함수)은 `extract.p0_table_scope`에 이름으로 적는다. 읽기-쓰기 잠금을 통째로 뺀 까닭은 잠금 집합 분석이 공유 획득을 읽지 않기 때문이다: 쓰기 획득만 넣으면 읽기 획득 뒤의 해제가 호출자 잠금의 해제로 읽힌다. 통째로 빼면 그 획득이 잠금 연산이 아니어서 판정이 약해지기만 한다(R2는 `pthread_rwlock_rdlock/wrlock`을 부른다 [M]).
- **P0 비교 표 개정 2** [D]: `strcoll_l`, `strcasecmp_l`, `strncasecmp_l`, `bcmp`, `strverscmp`, `thrd_equal`을 더했다(13항목, 모두 원문 대조 unverified). 넓은 문자 비교와 `__memcmpeq`는 이 개정 밖이다.
- 새 P0 항목 가운데 다섯 빌드가 부르는 것은 `pthread_key_create`(다섯 빌드)와 `__pthread_register_cancel`·`__pthread_unregister_cancel`(C1·R2)뿐이다 [M]. 어휘가 아니어서 유도에 쓰이지 않으므로 FACTS, OP_DERIV, SPAWN_WRAP, LOCK_* 등 유도 결과는 S5와 같다 [M: §2a].

## 2. 기존 동작 불변 확인 [M]

| 확인 | 결과 | 근거 |
|---|---|---|
| 시험 | 235/235 통과(1개 건너뜀은 원래대로). S5 뒤 새 시험 4개(`static_fixture_compile_relpaths`, `_deepcond`, `static_golden_lockpaths_np`, `static_golden_relpaths_np`). `test_static_cli` 475개 확인: S5의 확인에 더해 P0 표 개정 2의 기록, LOCK_PATHS의 되돌아가는 간선·spawn 없는 분기·entry-release, relations의 정적 이미지 entry·트램펄린 규칙·callee 쓰기·유도 spawn 연산의 인자 저장·호출 경유 읽기·`rests-on`·소멸자 있는 key·종료 연산 guard 평가, 깊이 211의 조건에서 R-T2 대상. `test_static_wrappers`에 P0 개정 2 확인. lint 6/6 | ctest 로그 |
| golden | 프로필 추출의 golden 변화 0. schema dump golden 둘은 개정 11로 기계적 재생성(개정 4·5·7–10과 같은 종류). 2a의 프로필 없는 golden은 P0 표 개정 2의 줄만 바뀜(§0 항목 2). S5 relations golden 둘은 규칙 개정 2로 바뀜. 새 golden 2(`lockpaths_np`, `relpaths_np`) | `git diff 552b1eb..HEAD -- kairos/tests` |
| 프로필 추출(identity) | V0·V2 × C1 pre tx·R2 core·C5 6.4.1: basis 6섹션·facts 22섹션 가운데 CRC가 다른 것은 STRINGS뿐. 다른 레코드는 basis 1(`front_end.build_identity`), facts 3(`extract.build_identity`, parent basis 해시, `extract.units_from`). show 텍스트 차이 2줄 | `T/identity/identity.txt` |
| 프로필 없는 추출, 옵션 없음(npsame, S4 대비) | 35개 섹션 레코드 같음. BASIS·P0_TABLE 다름, LOCK_PATHS는 새 섹션(§2a) | `T/npsame/npsame.txt` |
| 프로필 없는 추출, `--targets-rule`(s5same, S5 대비) | 같은 35개 섹션 레코드 같음(WRITERS, FACT_LOCS, FIELD_STATUS, LOCKSET 포함). BASIS·P0_TABLE 다름, LOCK_PATHS 새 섹션(§2a) | `T/s5same/s5same.txt` |
| 결정성(T7) | 다섯 빌드 facts·relations 두 번 같음 | `T/twice.txt` |

### 2a. npsame과 s5same

다섯 빌드 모두 STRINGS 말고 35개 섹션(ARGFLOW … WRITERS, LOCKSET 포함)의 레코드가 S4(옵션 없음)·S5(`--targets-rule`) facts와 같다 [M]. 다른 것은 셋이다.
- **BASIS**: 항목 이름으로 비교하면 S5 대비 더해진 것은 `p0.*` 34개(P0 표 개정 2), `extract.p0_table_scope`, `count.lock_paths`, `extract.basis_deviation[…]` 2개(C1·R2) 또는 7개(C5)다. 값이 바뀐 것은 `extract.p0_table`(판과 수), `assume.lock_acquisition_succeeds`(획득 기본 연산이 mutex에서 spin lock·`mtx_lock`까지로 넓어진 문장), 그리고 매번 바뀌는 `extract.build_identity`, `extract.units_from`이다. 빠진 항목은 없다.
- **P0_TABLE**: 72행. S5의 38행은 항목별로 그대로이고, 34행이 묶음 안에 끼어 들어갔다(그래서 위치 비교로는 다르다).
- **LOCK_PATHS**: 새 섹션.

## 3. ED §2.3 재측정: 공유 base의 R-T1 ∪ R-T2 [M]

`T/count/targets_rule.tsv`, `summary.txt`의 "ED §2.3 again" 절. 수정 단계에서 수가 하나도 바뀌지 않았다(facts.txt 줄 번호만 P0 행만큼 밀림). "FACT_LOCS"는 옛 규칙(대상 집합에 따라 바뀜, SF §4.1), "FIELD_STATUS"는 S4의 대상 무관 규칙(필드 단위 / 형 단위)이다. 괄호 안은 ED §2.3의 V3(G0 + R-T1, 공유 필터 없음).

| 빌드 | 대상(R-T1 / R-T2 / 둘 다) | FACT_LOCS complete / DISABLED | writer 0 / 1 / 2–5 / >5 | DISABLED 원인(겹침) untyped / escape / cast / union / unknown | FIELD_STATUS complete: 필드 단위 / 형 단위 |
|---|---|---|---|---|---|
| c1_pre_tx | 340 (308 / 53 / 21) (V3 841) | 142 / 198 (208 / 633) | 44 / 57 / 193 / 46 (174 / 107 / 422 / 138) | 198 / 124 / 71 / 10 / 16 (633 / 244 / 435 / 154 / 151) | 194 / 118 |
| c1_post_tx | 340 (308 / 53 / 21) | 142 / 198 | 44 / 57 / 193 / 46 | 198 / 124 / 71 / 10 / 16 | 194 / 118 |
| r2_core | 240 (220 / 34 / 14) (V3 504) | 110 / 130 (200 / 304) | 20 / 47 / 143 / 30 (101 / 52 / 257 / 94) | 130 / 90 / 50 / 2 / 0 (304 / 137 / 196 / 33 / 12) | 148 / 96 |
| c5_641 | 259 (189 / 99 / 29) (V3 322) | 74 / 185 (61 / 261) | 29 / 45 / 130 / 55 (91 / 68 / 115 / 48) | 185 / 117 / 68 / 0 / 0 (261 / 111 / 171 / 0 / 4) | 146 / 67 |
| c5_642 | 261 (188 / 102 / 29) | 74 / 187 | 29 / 44 / 133 / 55 | 187 / 119 / 68 / 0 / 0 | 148 / 67 |

공유 base가 아니어서 빠진 필드(그 규칙이 고를 수 있었으나 매개변수·지역·다른 호출 결과로만 닿음): R-T1 537 / 537 / 293 / 132 / 132, R-T2 355 / 355 / 246 / 85 / 85.

**사례 필드** (평가용 조회; `facts.txt` 줄은 `summary.txt` 같은 절)

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
- **R2 await와 옛 규칙.** R-T1 ∪ R-T2 대상의 옛 L1 (i)(FACT_LOOPS)은 `CFE_ES_MainTaskSyncDelay` 루프에서 여전히 fails다. 대상 무관 AWAIT_L1 필드 단위 규칙은 await다(`facts.txt:90905`). relations는 AWAIT_L1을 쓴다.
- **깊이 상한 제거의 효과** [M]: 다섯 빌드의 대상 수와 대상 목록은 S5와 같다(§2a: BASIS의 `target.*` 항목이 그대로). RETURNS의 조건·guard 트리는 최대 27 / 27 / 18 / 25 / 25 단계여서(검증자 측정) 200 상한이 결과를 바꾼 적이 없다. fixture `deepcond.c`(필드가 조건 뿌리에서 211단계 아래)에서 R-T2가 그 필드를 고른다 [M].

## 4. 완료 시험 T1–T7 (사례 입력 없음) [M]

근거 줄은 `T/<build>/NP/relations.txt`의 줄 번호(rel L)다. 모든 행은 `T/count/summary.txt`에서도 같은 줄 번호로 나온다.

### T1 R2 — **PASS**

**통과 조건(ED §6)**: B 관계 `CFE_SB_Global_t.AppId → CFE_EVS_SendEventWithAppID#2`; 39 지점; 쓰기 3(EarlyInit 0, AppInit out-pointer 둘); callee 검사 `EVS_GetAppDataByID == NULL → CFE_EVS_APP_ILLEGAL_APP_ID`(F2); 결과 버림 39/39; 읽는 쪽과 쓰는 쪽 행위자 다름(G0 없이, F7의 task-main root로); 순서 조건: 코어 초기화 순서가 고정됨(생성 순서 + `CFE_ES_MainTaskSyncDelay` await)을 보이거나, 보이지 못하면 "고정 미확인"을 실현 가능성 칸에 적음.

**나온 것** (r2_core; S5와 같은 행이고 줄 번호와 layers 칸만 다르다)
- 행 B65(rel 2146): `CFE_SB_Global_t.AppId -> CFE_EVS_SendEventWithAppID#2 sites=39 callee-check=yes`. 사이트 39곳(`cfe_sb_api.c` 34, `cfe_sb_priv.c` 1, `cfe_sb_task.c` 4), base 전역, hops 0.
- 필드(rel 2186): INIT 0(`CFE_SB_Global`), writer 3, WRITERS(규칙 대상 R-T1, `facts.txt:107`).
  - rel 2187: memset `cfe_sb_init.c:50:5` in `CFE_SB_EarlyInit`, 값 0.
  - rel 2188: out-pointer `cfe_sb_task.c:138:5` in `CFE_SB_AppInit` via `CFE_ES_GetAppID`, 값 symbolic `[CFE_ES_AppRecordGetID(AppRecPtr)]`, guard `AppRecPtr != 0`.
  - rel 2189: 같은 곳, 값 0 `[CFE_ES_APPID_UNDEFINED]`, guard `AppRecPtr == 0`.
- 행위자(rel 2190): 읽는 쪽 {`CFE_ES_BackgroundTask`, `CFE_ES_TaskMain`, `CFE_EVS_TaskMain`, `CFE_SB_TaskMain`, `CFE_TBL_TaskMain`, `CFE_TIME_Local1HzTask`, `CFE_TIME_TaskMain`, `CFE_TIME_Tone1HzTask`}, 쓰는 쪽 {`CFE_SB_TaskMain`, `main`} → inter-actor. task-main root는 table walk 반복(F7이 P0에서 유도한 `CFE_ES_StartAppTask`)에서 오고, `CFE_ES_TaskEntryPoint`·`OS_PthreadTaskEntry`는 트램펄린으로 빠진다(개정 2의 좁힌 규칙으로도 같다).
- callee 검사(rel 2192): `CFE_EVS_SendEventWithAppID returns 3254779907 [CFE_EVS_APP_ILLEGAL_APP_ID] when (and (not (eq param:Spec#3 0)) (eq (call-result EVS_GetAppDataByID (seq param:AppID#2)) 0))`. 한 단계 아래(rel 2195): `EVS_GetAppDataByID returns 0 [NULL] when (not (and (eq (call-result CFE_ES_AppID_ToIndex …) 0) (lt local:AppIndex 32)))`.
- 처리(rel 2197): `discarded 39, used 0, tested 0`.
- 실현 가능성(rel 2200): `undecidable: fixed unconfirmed: creation order at the table walk cfe_es_start.c:810:26 in CFE_ES_CreateObjects: writer actor CFE_SB_TaskMain at iteration 4; reader actors created before it: CFE_ES_TaskMain (iteration 1), CFE_EVS_TaskMain (iteration 2); after it: CFE_TBL_TaskMain (5), CFE_TIME_TaskMain (6). The call cfe_es_start.c:835:26 (CFE_ES_MainTaskSyncDelay) after the creation reaches the await loop cfe_es_start.c:871:5 (AWAIT_L1 per-field rule, which rests on assume.no_cross_member_pointer_arithmetic and is a PI decision pending; type-level star-closed) …; neither order is proven`.
- 뒤집힘 신호(rel 2202): `static-undetermined`. 위반 근거(rel 2201) unconfirmed, 계층(rel 2203) `B P0 R (writers: rule targets) rests-on(assume.no_cross_member_pointer_arithmetic; per-field completeness rule (PI decision pending))`.

**판정**: 조건이 모두 나왔다. 순서 조건은 "고정 미확인"으로 칸이 채워졌다. 정적으로 고정을 증명하지는 못했다: 생성 순서와 await는 나오지만, 사용이 그 행위자의 대기 상태 앞인지는 정적으로 정하지 않는다. await 인정은 S4의 필드 단위 완전성 규칙과 그 선언된 가정에 기대고, layers 칸이 이제 그것을 적는다. 형 단위 규칙으로는 star-closed다. 값 0이 검사를 통과하지 못함(ED §3.2)은 `CFE_ResourceId_ToIndex`의 상수 접기가 없어 평가 불가로 남는다.
- 참고 [M]: C1의 같은 관계(B73, rel 3152)는 이제 쓰는 쪽이 {`CFE_SB_TaskMain` (table-image-entry), `main`}이다. S5에서는 C1의 코어 task main이 행위자가 아니어서 {`main`}이었다(§10 지적 5).

### T2 C1 — **PARTIAL**

**통과 조건**: CF `cf_cfdp.c:993`(수정 후 `:1003`)과 BP `bp_flow.c:132`가 같은 구획; 게시 쓰기 L639/L640, L100, L1079; BP `:126 → :132` probe-create; BP 키 `"BP0_tsem"`(포인터 필드 흐름 + 이미지, 출처 `bp_semcfg.c` L43–L44)이 풀리거나 "필드 points-to 필요"로 표시; 수정 전 실패 분기 → 종료 도달(F2/F4, 종료 연산은 F8로 P0에서 유도); 수정 후 재시도(25, `!= -17`, 지연. 지연 연산은 F8로 P0에서 유도); SCH_LAB 생성은 키 상수 불일치로 짝 아님.

**나온 것** (c1_pre_tx; c1_post_tx는 같은 줄 번호에 같은 내용이고 다른 곳만 적는다)
- 이름 공간 `OS_common_record_t.name_entry`(rel 333): 검색 A2(rel 334) `OS_ObjectIdFindNextMatch`의 루프 `osapi-idmap.c:550:5`가 간접 호출(`:561:54`)로 `OS_ObjectNameMatch`의 `strcmp`(`:271:40`)에 닿는다. 활성 필드(rel 335) `OS_common_record_t.active_id`. 표 `OS_common_table`.
- 게시 쓰기(rel 380, 생성자 `OS_CountSemCreate#1`이 직접 호출로 닿는 쓰기): `active_id` `osapi-idmap.c:639:9`(`OS_ObjectIdFindNextFree`), `:1079:9`·`:1088:9`(`OS_ObjectIdTransactionFinish`), `:432:21`·`:519:13`(`OS_ObjectIdConvertToken`); `name_entry` `osapi-countsem.c:100:9`(`OS_CountSemCreate`), `osapi-idmap.c:640:9`(`FindNextFree`, := 0).
- CF 조회 행 A21(rel 417): `cf_cfdp.c:993:19 in CF_CFDP_InitEngine -> OS_CountSemGetIdByName#1 … partition {3} key unresolved`. 구획(rel 418) `3 [LOCAL_OBJID_TYPE] as OS_ObjectIdFindByName arg 0`. 키(rel 419) `read through the pointer field CF_AppData_t.config_table: needs field points-to (PTR_FIELDS)`. 생성자(rel 421) BP `bp_flow.c:132:18` `OS_CountSemCreate`, 같은 구획, "key unresolved (lookup)". 생성자(rel 422) SCH_LAB `sch_lab_app.c:168:16` `"SCH_LAB"`, 같은 이유로 짝 후보.
- BP 조회 행 A20(rel 403): `bp_flow.c:126:13 … partition {3} key known`. 키(rel 405): `read through the pointer field BP_GlobalData_t.Throttles -> BP_Throttles (PTR_FIELDS resolved), elements from 0 step 1 count 2; image BP_Throttles[0].Name = "BP0_tsem" (macro CF_CHANNEL_0_SEM_NAME, spelled at …/bp_semcfg.c:44:31); image BP_Throttles[1].Name = "BP1_tsem" (…:48:31)`. 짝 아님(rel 407): probe-create(`:132`, 같은 함수, 조회 결과 아래, 같은 키 인자). 짝 아님(rel 408): SCH_LAB, `key constants differ`.
- 수정 전 처리(rel 423–426): 루프 `cf_cfdp.c:967:5` constant-trip 2; `CF_CFDP_InitEngine returns the result of OS_CountSemGetIdByName`(rel 424); `CF_Init returns the result of CF_CFDP_InitEngine`(rel 425); 종료(rel 426) `terminal CFE_ES_ExitApp at cf_app.c:366:5 in CF_AppMain: the field CF_AppData_t.run_status := CFE_ES_RunStatus_APP_ERROR at cf_app.c:333:9 under (ne local:status 0) (the guard tests a local; the result of CF_Init is used in CF_AppMain: may) is argument 0 of it …` 그리고 개정 2가 더한 F8 사슬의 guard: `(F8 may CFE_ES_ExitApp -> CFE_ES_ResetCFE -> CFE_PSP_Restart -> abort; may: the terminal calls of CFE_ES_ExitApp are guarded: cfe_es_api.c:399:30 -> CFE_ES_ResetCFE under (and (and (ne local:AppRecPtr 0) (eq field:CFE_ES_AppRecord_t.Type 1)) (eq param:ExitStatus#0 8)); cfe_es_api.c:424:17 -> OS_TaskExit under (… (eq param:ExitStatus#0 9)); with argument 0 = 3 every guard is false: no derived terminal call is reached on this path; the loop cfe_es_api.c:453:13 in CFE_ES_ExitApp has no exit: the call does not return on a path through it (no P0 terminal on that path))`.
- 수정 후 처리(c1_post_tx rel 423): `in the loop cf_cfdp.c:1001:13 kind count-bounded trip upper-bound 25 exit (or (not (lt local:j 25)) (and (lt local:j 25) (ne local:ret -17))) delay cf_cfdp.c:1011:17 OS_TaskDelay (F8 must OS_TaskDelay -> OS_TaskDelay_Impl -> clock_nanosleep)`.
- 실현 가능성(rel 429) undecidable(생성 순서가 초기화 순서를 정하지 않는다), 뒤집힘(rel 431) static-undetermined(CF 키 미해결; 검색 반환 −17 / 0), 계층(rel 432) `B P0 L0 R`(CF·BP는 시작 스크립트 진입 root).

**판정**: 조건 일곱 가운데 여섯이 그대로 나왔다. 미달은 "수정 전 실패 분기 → 종료 도달"이다. 그 고리에 빠진 것은 둘이다.
- (a) `CF_AppMain`에서 `CF_Init`의 결과가 지역 변수 `status`에 들어가고 그 `status`가 `run_status = APP_ERROR`의 guard라는 연결이 facts에 없다. WRITERS guard가 지역 변수를 펼치지 않기 때문이다. 그래서 may다.
- (b) **종료 연산의 F8 유도가 CF의 길에 닿지 않는다** [M][F]. `CFE_ES_ExitApp`의 두 종료 호출(`ResetCFE` L399, `OS_TaskExit` L424)은 모두 `AppRecPtr->Type == CFE_ES_AppType_CORE` 아래(L381)이고, 각각 `ExitStatus`가 8·9일 때만이다. CF가 넘기는 값은 3(`APP_ERROR`)이라 두 guard가 모두 거짓이다. CF는 외부 앱이어서 `else` 가지의 `while (1) { OS_TaskDelay(500); }`(L453, facts의 loop 171: poll, 탈출 0)로 간다. 그 스레드를 끝내는 것은 다른 행위자(ES가 앱을 지우는 경로)다. S5 문서는 (a)만 미달로 적었다(§10 지적 2).
- 더해야 할 입력: 사례 입력이 아니다. (a)는 계층 B/R의 사실 확장(WRITERS·TERMINALS guard의 지역 변수를 정의로 펼침; 기존 섹션을 바꾸지 않으려면 새 섹션). (b)는 "탈출 없는 루프에서 멈춘 스레드를 다른 행위자가 끝낸다"는 행위자 사이 종료의 유도다. P0 계층에 다른 스레드의 취소 묶음(`pthread_cancel` 등; 개정 2 밖)과, 그 스레드를 지우는 쪽의 경로를 잇는 규칙(R)이 필요하다.
- 참고: BP 키의 출처는 매크로 정의 줄 `bp_semcfg.c:44`(ED는 `#ifndef`를 포함해 L43–L44)다. `#ifndef` 기본값이라는 점은 AST가 주지 않는다(S3 열린 문제 6).
- 덤으로 생긴 것 [M]: CF·BP 조회는 이름 공간 `OS_filesys_internal_record_t.virtual_mountpt`에도 행이 있다(A48 rel 939, A40 rel 843). `OS_ObjectIdFindNextMatch`의 간접 호출이 필드 기반 may로 그 콜백에도 닿기 때문이다. 이 이름 공간에서는 생성자 짝이 없다("no creator of this partition in another actor"). 사례 관계로 세지 않고 후보 부담의 넘치는 원인 (a)로 센다(§6).

### T3 C5 — **PASS**

**통과 조건**: spawn `CFE_ES_AppCreate:614`의 잠금 집합 ∅; 등록 쓰기 L649–L652의 잠금 집합 {ES 잠금}(F5 must 정체성 `CFE_ES_Global_t.SharedDataMutex`); 자식 `CFE_ES_GetAppID`의 `RecordUsed` 검사(F1)와 `CFE_ES_ERR_APPID` 반환(F2); "반대 순서 가능(잠금 창)". 6.4.2에서 읽기 지점마다: L800과 잠금 뒤 `GetAppIDInternal` 호출(L445, L551, L667, L1020, L1284) "불가 증명", `LockSharedData`·`UnlockSharedData` 오류 분기(L1858, L1892) "가능" 또는 "결정 불가". must 정체성이 없는 잠금 쌍으로 "불가"가 나오면 실패.

**나온 것**
- 6.4.1 (c5_641)
  - spawn(rel 2986): `callee OS_TaskCreate, entries CI_Lab_AppMain SAMPLE_AppMain SAMPLE_LibInit SCH_Lab_AppMain TO_Lab_AppMain`(F7 + 시작 스크립트 진입, L0); 생성자 정체성 `arg 0 [&CFE_ES_Global.AppTable[i].TaskInfo.MainTaskId] (OS_TaskCreate stores through it)`; `locks held {}`.
  - 등록 쓰기(rel 2987): `cfe_es_apps.c:649:18 key=CFE_ES_Global.TaskTable[TaskId] value=1 [TRUE] locks {#9 &OS_mut_sem_table[CFE_ES_Global.SharedDataMutex].id (static-instance)} acquired-at [cfe_es_apps.c:637:15]`.
  - 행 C31(rel 2985) `CFE_ES_GetAppID` L861 읽기: 첨자 `OS_TaskGetId -> pthread_getspecific (P0 thread-specific-get)`(rel 2989); 잠금 {#9} L851(rel 2988); **`possible: lock window`**(rel 2992); 뒤집힘(rel 2994) `static-possible: CFE_ES_GetAppID returns 3288334337 [CFE_ES_ERR_APPID] with the earlier value (INIT 0), not with the written value 1 [TRUE]`.
  - `GetAppIDInternal` L1886의 호출 지점별 행 C33–C39(rel 3006–3081): 잠금을 쥔 호출 다섯은 모두 possible(잠금 창), 오류 분기 둘(L1919, L1953)은 undecidable(failure path).
- 6.4.2 (c5_642)
  - spawn(rel 2940): entries 같음, `locks held {#9 …} acquired-at [cfe_es_apps.c:571:12]`; 등록 쓰기(rel 2941) L652 {#9}, 같은 획득 L571.
  - C30(rel 2939) `CFE_ES_GetAppID` L800: 잠금 {#9} L790(rel 2942) → **`proven-impossible`**(rel 2946). 이유 뒤에 `rests on assume.lock_acquisition_succeeds, assume.static_instance_read_after_write, assume.no_cross_member_pointer_arithmetic, the per-field completeness rule of FIELD_STATUS that makes #9 … a static instance (PI decision pending); under the type-level rule it is not one`이 붙고, 계층(rel 2949)은 `B P0 L0 R basis-deviation(front_end.lowered[osal/src/os/posix/osapi.c]…; front_end.unit_diagnostic[osal/src/os/posix/osapi.c]…) rests-on(…)`이다.
  - `GetAppIDInternal` L1825를 부르는 호출마다: L445 `CFE_ES_ExitApp`(C32, rel 2960), L551 `CFE_ES_RunLoop`(C33, rel 2971), L667 `CFE_ES_WaitForStartupSync`(C34, rel 2982), L1020 `CFE_ES_CreateChildTask`(C35, rel 2993), L1284 `CFE_ES_ExitChildTask`(C36, rel 3004) → 모두 **`proven-impossible`**. L1858 `CFE_ES_LockSharedData`(C37, rel 3015), L1892 `CFE_ES_UnlockSharedData`(C38, rel 3026) → **`undecidable`**(failure path).
  - 다섯 호출 경유 행은 개정 2의 규칙(읽기 행에 `entry-release`가 없을 때만 호출 지점의 잠금을 더함)을 지난다: `GetAppIDInternal`의 진입에서 읽기까지 잠금을 풀거나 따라가지 못하는 호출이 없다 [M].
- "불가"는 다섯 빌드를 통틀어 C 행에서만 나오고(6.4.1 14, 6.4.2 30), 모두 static-instance 정체성 #9다. 6.4.2의 30은 S5의 28에 `RecordUsed`(AppRecord) 행 C19(rel 2821, `CFE_ES_GetAppName`)·C20(rel 2831, `CFE_ES_GetTaskInfo`)이 더해진 것이다: 실패 분기의 쓰기 L690·L702가 spawn 뒤가 아니게 되어(§10 지적 3) 남는 쓰기 L633이 spawn과 같은 획득 L571 아래다. 정적 인스턴스가 아닌 잠금으로는 "불가"가 하나도 없다 [M].

**판정**: 조건이 모두 나왔다. 정체성 #9가 정적 인스턴스라는 판정은 S4의 필드 단위 완전성 규칙과 두 선언된 가정(`assume.static_instance_read_after_write`, `assume.no_cross_member_pointer_arithmetic`), 그리고 `assume.lock_acquisition_succeeds`에 기댄다(S4 §1.3–§1.4). 개정 2부터 행이 이것을 스스로 적는다. 형 단위 규칙을 고르면 #9는 정적 인스턴스가 아니고, 6.4.2의 "불가"는 "결정 불가"가 된다 [I].

### T4 C5 수명 — **PASS**

**통과 조건**: `OS_task_table.id`의 writer에 `pthread_create`(P0) 포함; 진입 함수가 그냥 반환하는 경로에 해제 없음; 해제 쓰기는 `OS_TaskDelete`·`OS_TaskExit`와 `OS_TaskCreate` 실패 경로(L572, L652, L667, L679), 초기화는 `OS_API_Init`(L279)로 따로 분류; `OS_TaskRegister` 검색에 활성 검사 없음. #72 관계와 다른 행.

**나온 것** (c5_641; c5_642는 같은 줄 번호에 같은 내용)
- 이름 공간 `OS_task_record_t.id`(rel 1300): 검색 `OS_TaskRegister`의 루프 `osapi.c:921:5`, 키 `id ==`(L923)가 지역 `pthread_id`와 비교(rel 1302); `OS_FindCreator` 루프(L3786)의 `pthread_equal`(rel 1301).
- 활성 검사(rel 1303): `active fields: none in this namespace's searches; no activity check: other searches of OS_task_record_t test OS_task_record_t.free`. 활성 값(rel 1304–1305): `free == 0`은 사용 중, `free == 1`은 빈 칸.
- writer(rel 1320): `writers 6 from WRITERS (4 of them write another field with a size that may reach this one: covers-more-than-the-field)`. 6개 가운데 4개는 `name`의 memcpy(크기가 상수가 아니어 표 안에 머문다고 본 쓰기)이고, 각 행에 `covers-more-than-the-field`가 붙는다. `id`에 직접 쓰는 것은 L766·L798의 := 0 둘이다.
- P0 writer(rel 1327): `pthread_create (P0 thread-create) stores through arg 0 [&(OS_task_table[possible_taskid].id)] at osapi.c:645:19 in OS_TaskCreate (joined here from ARGFLOW and P0_TABLE: the extraction's writer sets -- WRITERS, FACT_LOCS, FIELD_STATUS -- have no row for this store)`. 즉 이 조건은 **relations가 질의 시점에 ARGFLOW와 P0 표를 맞춰** 채운 것이다. 추출의 facts에서는 이 인자가 FACT_LOCS의 escape로 남는다(`escape=1, out-pointer 0`, 필드 DISABLED). ED §3.0 G10("escape → writer")은 추출 사실에서는 아직 실현되지 않았다(§8 항목 10).
- 진입 경로(rel 1328): `OS_TaskCreate parameter 2 … passed unchanged to the thread-creation primitive …; no function of this program runs after that entry returns (thread-specific keys created with a null destructor only: pthread_key_create at osapi.c:340:10 (arg 1 [NULL]); no cleanup handler push in the program), so the table has no write on that path`. 개정 2는 이 문장을 P0 key 생성의 소멸자 인자와 cleanup push 호출을 본 뒤에만 쓴다(§10 지적 6). 6.4.1 `osapi.c:340`의 `pthread_key_create(&thread_key, NULL)` [F][M].
- `free` 쓰기 분류(rel 1306–1319): L279 `OS_API_Init` class=initialization; L551·L697 class=reservation or activation; L572·L652·L667·L679 `OS_TaskCreate` class=release(guard는 각 실패 조건); L761 `OS_TaskDelete` class=release; L793 `OS_TaskExit` class=release.
- 블록 계층(rel 1331): `namespace layers: B P0 R basis-deviation(front_end.lowered[osal/src/os/posix/osapi.c]…; front_end.unit_diagnostic[osal/src/os/posix/osapi.c]…)`. 이 블록 전체가 PI D7의 단위 편차로 읽은 `osapi.c`에 기댄다.

**판정**: 조건이 모두 나왔다. 단 (1) "writer에 `pthread_create` 포함"은 relations의 질의 시점 결합으로 채워졌고 추출의 writer 집합에는 없다. (2) `OS_TaskRegister`의 비교 상대가 `pthread_self()`라는 것은 facts가 지역 변수 `pthread_id`의 정의를 담지 않아 행에 나오지 않는다(T2 (a)와 같은 공백). 활성 검사 없음은 비교 상대와 무관하게 나온다.

### T5 입력 계층 — **PARTIAL**

**통과 조건**: 위 행마다 입력 계층이 B/P0/L0/R만이다. 사례 계층 0, G0 0. P0·L0 항목마다 판과 원문 줄.

**나온 것** (`summary.txt` T5 절)
- 행 수와 계층: c1_pre 392, c1_post 392, r2 299, c5_641 178, c5_642 175, 합 1,436행. 계층은 `B P0 R`, `B P0 L0 R`(시작 스크립트 진입 root에 기댈 때)와 그 둘에 `(writers: rule targets)`가 붙은 것뿐이다. `case:`·`hand-entered:` 표시가 붙은 행 0.
- 개정 2의 표시: `basis-deviation(…)`이 붙은 행 0 / 0 / 0 / 159 / 156(이름 공간 블록 0 / 0 / 0 / 8 / 8). C5에서 `osapi.c`의 위치나 그 파일에 정의된 함수를 이름 붙이는 행이다. `rests-on(…)`이 붙은 행 0 / 0 / 55 / 14 / 30(R2: B 행의 await 인정, C5: "불가" 행).
- 머리: `mode profile-free`, 내장 프로필 두 문장, P0 표 판과 `p0_table_scope`, 유도 규칙 판, 대상 규칙 판, P0 비교 표 판·항목·scope, basis 편차(C1·R2 2개, C5 7개). extract 요약: `hand-entered inputs 72 (built-in P0 table 72, command line 0)`(다섯 빌드).
- P0 표(facts `[p0]`): 72항목 모두 판 표기. 원문 대조 줄은 `pthread_create` 1개(glibc 2.39 `nptl/pthread_create.c` L624/L755/L804/L836), 71개는 `source unverified`. P0 비교 표 13항목도 모두 unverified.
- L0: recipe의 `startup_script` 블록은 판마다 `cite` 줄을 단다(basis, S1 보고). 이번에 새로 대조하지 않았다.

**판정**: 사례 계층과 G0이 0이라는 핵심 조건은 통과다. 미달은 둘이다.
- "P0 항목마다 원문 줄" 조건은 84개(71 + 비교 13)가 unverified다. 더해야 할 입력은 P0 계층의 glibc 2.39 upstream 원문의 로컬 사본과 각 항목의 줄이다. 로컬에 있는 glibc 사본은 `c5/ref/glibc`의 `pthread_create` 셋뿐이다(S1).
- 사례 입력은 아니지만 사람이 넣은 것과 결정이 대기 중인 것에 기대는 행이 있다: C5 행의 거의 전부(159 / 178, 156 / 175)가 PI D7의 `front_end_diagnostic` 편차로 읽은 단위에 기대고, "불가" 44행과 R2의 await 55행이 선언된 가정과 필드 단위 규칙(PI 결정 대기)에 기댄다. S5의 layers 칸은 이것을 적지 못했다(§10 지적 8). 이제 칸이 적는다.

### T6 정상 대조 — **PASS**

**통과 조건**: C5 OSAL `*GetIdByName`·`*Create` 6쌍(task, queue, binsem, countsem, mutsem, timer)과 C1·R2의 `OS_ObjectIdFindNextMatch` 콜백 3종이 A로 인식된다. 정상 호출자는 신호만 내고 위반 확정은 내지 않는다.

**나온 것**
- C5(두 판 같음): 이름 공간마다 `key-parameter functions` 줄. `OS_task_record_t.name`(rel 1332) lookups `OS_TaskGetIdByName#1` / creators `CFE_ES_AppCreate#3 CFE_ES_CreateChildTask#1 OS_TaskCreate#1`; `OS_queue_record_t.name`(rel 1276) `OS_QueueGetIdByName#1` / `CFE_SB_CreatePipe#2 OS_QueueCreate#1`; `OS_bin_sem_record_t.name`(rel 1184), `OS_count_sem_record_t.name`(rel 1205), `OS_mut_sem_record_t.name`(rel 1250), `OS_timer_record_t.name`(rel 1364) 각 `GetIdByName#1` / `Create#1`.
- C1(두 판)·R2: A2 콜백 검색 3종. 키 `OS_common_record_t.name_entry`(`strcmp`, C1 rel 334 / R2 rel 95), `OS_filesys_internal_record_t.virtual_mountpt`(`strncmp`, rel 759 / 352), `OS_impl_task_internal_record_t.id`(`pthread_equal`, rel 1885 / 1044).
- 위반 근거 칸: 다섯 빌드 1,436행 모두 `unconfirmed`.

**판정**: 조건대로 나왔다. 다만 "정상 호출자는 위반 확정을 내지 않는다"는 이번에 기록을 주지 않았기 때문에 자명하게 성립한다. 기록이 있을 때의 동작은 시험하지 않았다(§8 항목 1).

### T7 결정성 — **PASS**

`T/twice.txt`: 같은 basis에서 extract(`--targets-rule R-T1,R-T2`)와 relations를 다시 돌렸다. facts가 다섯 빌드 모두 같다(`ef975218…`, `dd49a91c…`, `5737363e…`, `726fbc80…`, `7a7d4d0b…`). relations도 같다(`8176a26c…`, `8e6e27a0…`, `be29324c…`, `85dc59dc…`, `930d2863…`). fixture golden은 relations를 두 번 돌려 같은 바이트를 요구한다.

## 5. 반증 조건 (ED §6, RV2 §11 단계 2, §12)

- **조건**: T1–T3 가운데 하나라도 사례 입력을 더하지 않으면 관계가 나오지 않으면, 가설("사례별 Φ 없이 근거 있는 관계를 낸다")은 아직 성립하지 않은 것으로 보고한다.
- **이번 결과** [M]: T1–T3은 사례 입력(프로필, 명령줄 어휘, `--target`, G0, rspec·`.kzm`·manifest) 없이 돌렸고, 세 사례의 관계 행이 나왔다(T1 B65, T2 A20·A21, T3 C30–C38). 반증 조건은 충족되지 않았다. 사례 입력을 더해야 했던 시험은 없다.
- **그래도 "성립"이라고 적지 않는 이유** [I]
  1. T2의 종료 도달이 미달이다. 지역 변수 정의의 사실이 없고(may), 종료 연산의 F8 유도는 CF의 길에서 P0 종료에 닿지 않는다(guard가 모두 거짓, 남는 길은 탈출 없는 루프). 사례 입력으로 메울 것은 아니고, B/R의 사실 확장과 행위자 사이 종료를 잇는 P0·R 규칙이 필요하다.
  2. "근거 있는"의 무게가 고르지 않다. T1의 순서는 "고정 미확인"이고, C1의 반대 순서 가능성도 "결정 불가"다. 위반 근거는 실행 기록이 있어야 생긴다(RV2 §5.4). T3의 "불가 증명"은 선언된 가정 셋과 필드 단위 규칙(PI 결정 대기)에 기댄다.
  3. 남은 수작업 입력이 있다. 사례 무관이지만 사람이 쓴 것이다. L0(recipe의 시작 스크립트 블록), C5 `front_end_diagnostic` 2줄(PI D7; C5 행의 대부분이 기댐), unverified P0 원문 84개, P0·비교 표 자체가 그렇다.
  4. 사례 관계는 행 175–392개 가운데 1–8개다(§6). 질의는 사례 관계를 골라내지 않는다. 고르는 것은 이 문서의 평가용 조회다. 수정 단계에서 행이 늘었다(C의 "뒤"가 되돌아가는 간선을 포함하고, C1의 코어 task main이 행위자가 됨).
  5. 다섯 사례는 개발 집합이다(ED §5 R7). 동결 뒤 고른 구성으로 따로 평가하지 않았다.

## 6. 후보 부담 [M]

`T/count/burden.tsv`. "사례 관계 행"은 평가용 조회로 고른 행이다. C1은 짝이 있는 이름 공간 `OS_common_record_t.name_entry`의 CF `:993`(수정 후 `:1003`) 조회 A21과 BP `:126` 조회 A20, 2행이다. 같은 조회가 콜백 이름 공간 `OS_filesys_internal_record_t.virtual_mountpt`에 낸 A40·A48은 짝이 없고 필드 기반 may의 부산물이어서 세지 않는다(S5 문서는 이것까지 4행으로 셌다: §10 지적 12). R2는 T1의 B65 1행, C5는 `CFE_ES_AppCreate` spawn × `RecordUsed` × `GetAppID`/`GetAppIDInternal` 읽기 지점 8행이다.

| 빌드 | A(조회 사용) | B | C | 합 | 이름 공간 블록 | 사례 관계 행(비율) | flip static-possible | feasibility proven-impossible |
|---|---|---|---|---|---|---|---|---|
| c1_pre_tx | 127 | 170 | 95 | 392 | 13 | 2 (0.5%) | 2 | 0 |
| c1_post_tx | 127 | 170 | 95 | 392 | 13 | 2 (0.5%) | 2 | 0 |
| r2_core | 68 | 155 | 76 | 299 | 8 | 1 (0.3%) | 1 | 0 |
| c5_641 | 66 | 73 | 39 | 178 | 31 | 8 (4.5%) | 16 | 14 |
| c5_642 | 66 | 71 | 38 | 175 | 31 | 8 (4.6%) | 16 | 30 |

- S5 출력(`T_S5/`)에 같은 계수를 고쳐 적용하면 사례 관계 행은 2 / 2 / 1 / 8 / 8, 비율 0.7% / 0.7% / 0.4% / 4.4% / 4.5%(행 286 / 286 / 248 / 180 / 177)다 [M].
- **S5 대비 변화** (`T/count/against_s5.tsv`, 행 머리로 비교) [M]
  - C1 A: 127행 그대로, 뒤집힘 등급이 바뀐 행 10(코어 task main이 행위자가 되어 `CFE_TIME_TaskInit`의 조회 하나가 다른 행위자의 같은 키 생성자와 짝이 되고(static-possible), 9행은 key 미해결 생성자와 짝이 됨(static-undetermined)). 짝 있는 조회 26 → 36, 키까지 같은 것 1 → 2.
  - C1 B: 135 → 170(+35, 코어 task main이 행위자가 되어 inter-actor가 됨). 등급이 바뀐 행 0.
  - C 행: C1 24 → 95(빠짐 2, 새로 73), R2 25 → 76(빠짐 2, 새로 53), C5 41 → 39·40 → 38(빠짐 2씩). 새 행은 모두 `CFE_ES_CreateObjects`의 table walk spawn이다: 루프 본문의 쓰기가 되돌아가는 간선으로 다음 반복의 spawn 뒤가 된다(C1은 그 spawn의 entry가 이제 풀림). 빠진 행은 다섯 빌드 모두 P0 스레드 생성의 out 인자(C1·R2 `OS_impl_task_internal_record_t.id`: `OS_TaskCreate_Impl`이 `&impl->id`를 `OS_Posix_InternalTaskCreate_Impl`에 넘기고, 그 함수가 매개변수를 그대로 `pthread_create`의 인자 0에 넘긴다(`os-impl-tasks.c:586`, `:543` [F]); C5 `OS_task_record_t.id`: `pthread_create(&OS_task_table[…].id, …)`)를 소스 위치 때문에 "뒤"로 셌던 것이다. P0 의미로 그 저장은 새 스레드가 돌기 전이다. 본문 있는 유도 spawn 연산 `CFE_ES_StartAppTask`가 인자 0으로 저장하는 `CFE_ES_AppRecord_t.MainTaskId`는 spawn 뒤 쓰기(잠금 모름)로 남는다: C1 `CFE_ES_AppCreate:768` spawn의 행 C15–C19가 그것이고, R2의 table walk 행 넷이 같은 줄을 단다 [M]. 등급이 바뀐 C 행은 6.4.2의 C19·C20 둘(possible → proven-impossible)뿐이다.
  - "불가": 0 / 0 / 0 / 14 / 28 → 0 / 0 / 0 / 14 / 30, 모두 #9.
- 경로별 남는 행의 거름(머리 줄): B는 필드–인자 묶음 가운데 writer 없음과 한 행위자뿐을 뺀 것이다. B 가운데 callee 검사가 있는 행은 40 / 40 / 29 / 26 / 24다. A는 이름 공간 13 / 13 / 8 / 31 / 31개의 조회 사용이고, 같은 구획의 다른 행위자 생성자가 있는 조회는 36 / 36 / 20 / 2 / 2, 키까지 같은 것은 2 / 2 / 1 / 0 / 0이다. C에서 생성자 본문에는 없고 spawn 뒤 callee에서만 쓰는 필드(짝짓지 않음)는 1,964 / 1,964 / 765 / 506 / 506이다.
- 뒤집힘 신호 static-possible은 2 / 2 / 1 / 16 / 16행이다. C1의 2행과 R2의 1행은 같은 키가 정적으로 맞는 조회–생성 짝이다(C1은 `OS_TimeBaseGetIdByName`의 SCH_LAB·TIME 조회). 사례 관계(T1, T2)는 static-undetermined에 들어 있다. 신호만으로 고르면 사례 관계를 놓친다.
- 넘치는 원인 [I]: (a) 필드 기반 may 간접 호출(T2 덤 참고: 한 조회가 여러 콜백 이름 공간에 들어간다), (b) 행위자 귀속의 과대 근사(트램펄린이 부르는 모든 root가 그 코드를 공유), (c) B의 inter-actor 기준이 `main`(process)을 writer 행위자로 세어 초기화 쓰기와 task 읽기를 모두 짝으로 만든다, (d) C의 "spawn 뒤"가 분기를 자르지 않는 CFG 도달이라 table walk 루프 본문의 모든 쓰기가 들어간다(과대 근사; "불가"를 줄이는 쪽).
- 시간(이 VM, 한 번, 비용 주장 아님): NP extract(`--targets-rule`) 31.9 / 30.3 / 20.4 / 8.4 / 8.3 s(S5 42.0 / 40.1 / 26.0 / 9.2 / 9.7 s), relations 1.8 / 1.3 / 0.8 / 0.7 / 0.6 s(`T/run.log`).

## 7. 조건 기록 (`CONDITIONS_POLICY.md`)

| 조건 | 값 | 상태 | 근거·이유 |
|---|---|---|---|
| 분석 대상 빌드 | S1–S4와 같은 기준 빌드 5 | from-reference | ED §1.1, §1.4 |
| 대상 규칙 | R-T1 ∪ R-T2, 공유 base만 | 도구 규칙(연구 조건 아님), "target rules revision 1" | ED §6 항목 2. 호출 결과 base를 전역 주소만 돌려주는 함수로 넓힌 것은 ED의 "F4 원천이 전역인 포인터"를 이 판에서 읽은 방식이다 [D] |
| relations 규칙 | §1.2 | 도구 규칙, "relations rules revision 2" | ED §3.1–§3.4, §6 항목 3; 수정 단계(§10) |
| P0 표 | 72항목, 개정 2, 담지 않는 묶음은 `extract.p0_table_scope` | 도구 내부 지원(P0 계층). 판 표기 항목마다, 원문 대조 1개 | 읽기-쓰기 잠금 묶음을 통째로 뺀 것은 이 개정의 범위 결정이다 [D](§1.3) |
| P0 비교 표 | 13항목, 개정 2 | 도구 내부 지원(P0 계층). 원문 대조 unverified | 추출은 쓰지 않는다 |
| 수치 도구 매개변수 | 새것 없음 | — | 대상 규칙(수정 단계에서 R-T2 조건 걷기의 깊이 200 상한을 없앰; S5 문서의 "상한이 없다"는 그 상한이 남아 있어 틀렸다), relations(식 트리 걷기, 행위자, LOCK_PATHS 읽기), LOCK_PATHS 유도(CFG 도달, 경로 합집합)에 수치 상한이 없다. 식 텍스트의 40단계 출력 한계(`show`와 같음)는 출력에만 쓰고 비교·중복 제거의 키로 쓰지 않는다; 이 한계로 잘린 식은 다섯 빌드 0개 [M]. 기존 F4 상한(`--argflow-depth`·`--argflow-size`)은 기본값 none, `--image-budget` 4096(S3, unspecified_by_reference) 그대로 |
| 선언된 가정 | S4의 셋 | S4 보고 §4와 같음. `assume.lock_acquisition_succeeds`의 문장은 P0 획득 기본 연산이 spin lock·`mtx_lock`까지로 넓어진 만큼 고쳤다 | T3의 "불가"가 기대고, 행이 적는다 |
| 완전성 규칙의 선택 | relations와 AWAIT 인정은 필드 단위 규칙을 쓴다 | 결정 대기(S4 열린 문제 1) | 형 단위를 고르면 T1의 await와 T3의 "불가"가 사라진다 [I]. 행이 `rests-on(per-field completeness rule (PI decision pending))`으로 적는다 |
| 실행 기록 | 없음 | 이번 시험의 조건 | ED §6: 사례 입력 없이. run-order 칸은 늘 `no record given` |
| L0, C5 `front_end_diagnostic` | S1과 같음 | S1 보고 §4와 같음 | 추출이 basis의 편차 항목을 `extract.basis_deviation[…]`로 facts에 복사하고, 행이 `basis-deviation(…)`으로 적는다 |

## 8. 열린 문제

1. **실행 기록 입력.** run-order 칸을 채우는 `--record`는 이 판에 없다. 기록의 neutral 사이트(파일:줄)를 행의 쓰기·사용 위치에 맞추는 일이 남았다. 그 전까지 위반 근거 칸은 늘 unconfirmed다.
2. **지역 변수 정의의 사실과 행위자 사이 종료.** WRITERS·TERMINALS·READS ctx의 guard에 남는 지역 변수(`status = CF_Init()`, `pthread_id = pthread_self()`)를 정의로 펼치는 사실이 없다(T2 (a), T4 (2)). T2는 그 위에, 탈출 없는 루프에서 멈춘 스레드를 다른 행위자가 끝내는 것을 유도해야 한다(T2 (b): P0 다른 스레드의 취소 묶음과 그 경로 규칙). 기존 섹션을 바꾸지 않으려면 새 섹션이 필요하다.
3. **공유 base 판정의 범위.** READS는 지역 포인터의 정의를 따라가지 않는다. 그래서 `AppRecPtr->AppState` 같은 사례 필드가 R-T2에서 빠진다. ARGFLOW의 `base-via-local`과 같은 규칙을 READS에도 두면 넓어진다(새 섹션 또는 relations 쪽 계산).
4. **필드 기반 may의 부담.** A의 간접 호출 콜백 혼입, 트램펄린 귀속, process 행위자를 writer로 세는 B 기준, 분기를 자르지 않는 C의 "spawn 뒤"가 행 수를 키운다(§6). 사슬의 함수 포인터 인자(예: `OS_ObjectNameMatch`를 고정해 넘기는 호출)로 콜백을 좁히는 규칙과, 생성 루프의 반복 사이 순서(같은 반복의 쓰기만 그 반복의 spawn에 묶기)가 다음 후보다. spawn 뒤 callee에서만 쓰는 필드(1,964 / 765 / 506)는 짝짓지 않았다.
5. **상수 접기.** R2의 "초기 값 0은 검사를 통과하지 못함"(ED §3.2)은 본문 있는 함수의 상수 접기가 없어 static-undetermined로 남는다. 값은 C int32가 아니라 캐스트를 벗긴 값으로 출력된다(S2 열린 문제 4: `3254779907`).
6. **C의 "spawn 뒤"와 callee의 잠금.** 수정 단계에서 CFG 도달(LOCK_PATHS)이 되었다. S5 문서의 "이번 사례에서는 소스 위치와 CFG 순서가 같다는 것을 원문으로 확인했다 [F]"는 틀렸다: 6.4.2 C19·C20의 L690·L702는 spawn 가지를 배제하는 `else` 가지에 있었다(§10 지적 3). 남은 것: 분기 시험으로 도달을 자르지 않는다(과대 근사). 한 호출보다 아래의 callee 쓰기는 호출 지점의 잠금과 합치지 않는다(보수). 본문 있는 유도 spawn 연산이 인자를 통해 저장하는 것은 그 호출 안에서 스레드 생성과의 순서를 읽지 않고 spawn 뒤, 잠금 모름으로 센다(보수; "불가"를 막는 쪽).
7. **P0 원문 대조.** 71 + 13항목 unverified(T5). glibc 2.39 upstream 사본이 필요하다.
8. **완전성 규칙의 PI 결정**(S4 열린 문제 1)과 **facts의 프로필 해시 결합**(ED §5 R8)은 그대로 대기다. relations는 프로필 facts를 받되 모든 행에 사례 계층을 단다.
9. **구획 비교**는 상수 값 집합의 같음이다. 상수가 둘 이상인 사슬(예: `OS_remove`의 `{1,11}`)에서 다른 의미의 상수가 섞일 수 있다.
10. **escape → writer(ED §3.0 G10).** P0 out 인자(`pthread_create`의 id 저장)는 relations가 질의 시점에 맞춘다. 추출의 WRITERS·FACT_LOCS·FIELD_STATUS·LOCK_IDS에는 반영되지 않는다(FACT_LOCS는 escape로 셈). 반영하려면 기존 섹션을 바꾸지 않는 새 섹션이 필요하다.
11. **C5의 코어 task main.** C5 `CFE_ES_CreateObjects`의 spawn(6.4.1 `:875`, 6.4.2 `:855`)은 entry를 union 멤버 `VoidPtr`로 읽고 정적 이미지는 `FunctionPtr`을 초기화한다. 이 판은 union의 다른 멤버로 읽는 것을 풀지 않으므로(머리에 이유를 적음) C5의 코어 task main은 행위자가 아니다. 그 코드는 `CFE_ES_CreateObjects`의 간접 호출(`:961`, `FunctionPtr` 필드 기반)로 `main`에 닿는다 [M]. 그래서 C5의 B·C 행위자 집합에는 코어 task가 없다. union 멤버의 별칭을 사실로 내는 규칙(형 정보가 facts에 필요)이 다음 후보다.
12. **트램펄린 규칙의 쓰기.** 형 있는 레코드 필드 쓰기는 facts에 base가 없어 트램펄린 판정이 보지 않는다(전역 객체 쓰기와 공유 root 읽기만 본다).
13. **읽기-쓰기 잠금.** P0 개정 2가 통째로 뺐다. 공유 획득을 상태에 따로 두는 잠금 집합 분석이 있어야 넣을 수 있다. 지금은 그 획득이 잠금 연산이 아니어서 판정만 약해진다.

## 9. 재현

```sh
R=/home/user/Shape-Perf/research/cfs_procrace
S5=/home/user/work/redesign/step2a/tests_s5 $R/env/redesign/step2a_tests.sh all   # identity npsame run s5same twice count sha -> /home/user/work/redesign/step2a/tests
```

- 각 빌드 `T/<build>/NP/`에 `basis.kz`·`basis.txt`·`facts.kz`·`facts.txt`·`extract.txt`·`relations.txt`·`args.txt`·`sha256.txt`가 있다. `NP2/`는 T7의 두 번째 relations다.
- `T/count/summary.txt`(T1–T7 증거, 줄 참조, S5 대비), `burden.tsv`, `targets_rule.tsv`, `summary.tsv`, `against_s5.tsv`. `T/identity/identity.txt`, `T/npsame/npsame.txt`, `T/s5same/s5same.txt`, `T/twice.txt`, `T/run.log`.

## 10. 수정 단계: 검증 지적과 처리 (2026-10-30)

검증자 지적 13건(같은 내용 둘을 하나로 묶어 12건)을 파일과 출력에 대어 보았다. 모두 성립했다. 판정·시험을 바꾼 것은 지적 2·3·5·8이다. 버린 지적은 없다.

| # | 지적(요지) | 확인 | 처리 | 결과 |
|---|---|---|---|---|
| 1 (와 10) | `target_rules.cpp` `field_loads`의 깊이 200 상한이 남아 있었고, §7과 보고가 "새 수치 매개변수 없음"이라 적었다 | 성립 [F] | 상한 없이 방문 집합으로 걷는다(풀은 같은 트리를 한 노드로 두고 노드를 자식 뒤에 만듦). §1.1, §7 고침. fixture `deepcond.c`(깊이 211) | 다섯 빌드 대상 수·목록 변화 0 [M]; fixture에서 R-T2가 211단계 아래 필드를 고름 [M] |
| 2 | T2의 F8 사슬 `ExitApp → ResetCFE → … abort`가 CF가 갈 수 없는 CORE 가지를 지난다 | 성립 [F][M] | relations가 may·noreturn 종료 연산의 종료 호출 guard(OP_DERIV)를 적고 이 호출의 인자 상수로 평가, 탈출 없는 루프도 적음. TERMINALS는 프로필 추출에도 있는 기존 섹션이라 바꾸지 않고 relations에서 OP_DERIV를 읽었다. §4 T2·§8 항목 2 고침 | C1 rel 426: "with argument 0 = 3 every guard is false … the loop cfe_es_api.c:453:13 … has no exit" [M]. T2 PARTIAL 유지, 미달 항목 (b) 추가 |
| 3 | §8 항목 6의 [F] 주장("소스 위치와 CFG 순서가 같음")이 C5 C19·C20과 어긋난다 | 성립 [F] | 새 사실 LOCK_PATHS `after-spawn`(CFG 도달)으로 "뒤"를 정함. [F] 문장 삭제·정정 | 6.4.2 C19·C20 possible → proven-impossible; 소스 위치로 "뒤"였던 P0 스레드 생성 out 인자의 행 C1·R2·C5 2씩 빠짐; 루프 본문 쓰기로 C1 +73·R2 +53. 본문 있는 유도 spawn 연산의 인자 저장(PARAM_FLOW out-store)은 CFG에서 호출보다 앞이므로 spawn 뒤 쓰기(잠금 모름)로 따로 셈: C1 `MainTaskId` 행 C15–C19 유지 [M]; fixture `relpaths.c` `spawn_marked` [M] |
| 4 | 호출 경유 문맥에서 callee가 읽기 전에 잠금을 푸는지, 생성자 callee의 spawn 뒤 쓰기를 보지 않았다 | 성립 [F] | LOCK_PATHS `entry-release`; 그 행이 있으면 호출 지점 잠금을 읽기에 더하지 않음. 생성자가 spawn 뒤 부르는 함수의 같은 필드 쓰기를 실현 가능성에 넣음(한 호출 아래만 합성) | 기준 빌드에서 등급이 바뀐 행 0(C5 해당 0, C1·R2는 잠금 없는 행) [M]; fixture `relpaths.c`에서 두 경우가 "불가"를 막음 [M] |
| 5 | C1·C5의 코어 task main이 행위자가 아니다 | 성립 [M] | spawn의 entry 인자가 정적 이미지가 있는 전역 표의 필드에서 오면 그 멤버의 함수 잎을 root로(`table-image-entry`); 풀지 못한 spawn은 머리에 이유와 함께 | C1 root 5개 더함(`CFE_ES/EVS/SB/TBL/TIME_TaskMain`), B73 writer `{CFE_SB_TaskMain, main}` [M]. C5는 union 멤버 별칭으로 미해결, 머리에 보고(§8 항목 11) |
| 6 | T4의 `pthread_create` writer는 relations 결합일 뿐이고, entry 경로 규칙이 TSD 소멸자·cleanup handler를 보지 않는다; writer 6에 `name` memcpy가 섞여 있다 | 성립 [M][F] | P0 out 인자 줄에 "relations 결합, 추출 writer 집합에 없음" 명시; entry 경로 규칙을 P0 key 생성의 소멸자 인자와 cleanup push 호출로 조건화(P0 종류 넷 추가); covers-more-than-the-field를 따로 셈 | C5 rel 1327·1328·1320 [M]. G10 미실현은 §8 항목 10 |
| 7 | P0 묶음이 다 차지 않았다(비교 표의 `_l`·`bcmp`, 본 표의 `*_clock*`·C11·`usleep`·`quick_exit`·rwlock) | 성립 [F] | P0 표 개정 2(72), 비교 표 개정 2(13), 담지 않는 묶음을 이름으로 기록(rwlock은 공유 획득 때문에 통째로 제외) | 새 항목 가운데 기준 빌드가 부르는 것은 `pthread_key_create`, `__pthread_(un)register_cancel`뿐이고 유도 결과 변화 0 [M] |
| 8 | layers 칸이 basis 편차(PI D7)와 선언된 가정·필드 단위 규칙을 적지 못한다 | 성립 [M] | 추출이 basis 편차를 facts BASIS에 복사; 행이 이름 붙인 파일의 단위 편차를 `basis-deviation(…)`, 실현 가능성이 기댄 가정·규칙을 `rests-on(…)`으로 | C5 행 159·156이 basis-deviation, rests-on 55·14·30 [M]. T5 미달 항목에 추가 |
| 9 | 트램펄린 판정이 보고서 설명보다 넓다(다른 root를 부르기만 하면 강등) | 성립 [F] | 간접 호출로만 다른 root에 들어가고 제 본문이 공유 읽기·전역 쓰기를 하지 않을 때만 강등. 머리에 남는 root를 이유별로 | 다섯 빌드 트램펄린 S5와 같음 [M]; fixture에서 직접 부르는 root는 행위자로 남음 [M] |
| 11 | `ex()`의 40단계 잘린 텍스트가 검사 중복 제거의 키다 | 성립 [F] | (값, 조건 트리 뿌리)로 중복 제거(풀이 같은 트리를 한 노드로 둠). 40단계는 출력에만 | R2·C5의 B 행 텍스트가 S5와 같음(새 주석 말고) [M] |
| 12 | C1 사례 관계 행 4는 콜백 이름 공간의 부산물 둘을 센 것이다 | 성립 [M] | 계수 스크립트가 짝 있는 이름 공간의 행만 셈; 부산물은 넘치는 원인 (a)로 | C1 2행; S5 출력에 고쳐 적용하면 0.7% [M] (§6) |
| 13 | "기존 golden은 하나도 바뀌지 않았다"가 2a 전체로는 틀리다 | 성립 [F] | 문장을 S5로 한정하고 2a 전체(S1–S4의 schema dump golden)와 수정 단계의 golden 변화를 적음 | §0 항목 2, §2 |

커밋: `29343d5`(lead의 WIP 스냅숏), `e43f611`, `83b2f3b`, `a378bf8`, `b76a9a9`(유도 spawn 연산의 인자 저장)와 이 문서의 커밋(접두 `kz-static 2a/fix:`).

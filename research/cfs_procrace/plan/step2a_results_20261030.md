# 단계 2a 완료 시험 T1–T7: 규칙 대상(R-T1·R-T2)과 사례 무관 질의 `kz-static relations` (2026-10-30)

- 성격: **구현과 측정 보고**다. 단계 2a의 다섯째 묶음(S5)과 그 **수정 단계**(검증 지적 13건, §10), **둘째 수정 단계**(검증 지적 23건, §11)다. `plan/extraction_design_20261030.md`(이하 ED) §6 단계 2a 항목 2(규칙 대상 R-T1·R-T2를 공유 base로 거르고 §2.3을 다시 잼)와 항목 3(사례 무관 질의 `kz-static relations`)을 kz-static에 넣고, ED §6의 완료 시험 T1–T7을 사례 입력 없이 돌렸다. 두 수정 단계는 지적을 고치고 같은 시험을 다시 돌렸다. 이 문서의 수와 판정은 **둘째 수정 단계**의 출력이다. cFS를 기동하지 않았고 campaign을 돌리지 않았다. 정적 분석(kz-static)과 텍스트 처리만 했다.
- 근거: ED §3.0(입력 계층, P0 조건), §3.1–§3.4(경로 A/B/C, 반사실 뒤집힘 신호, 관계 행의 칸), §5(R1–R9), §6(항목 2·3, T1–T7, 반증 조건); `plan/input_audit_20261030.md`; `plan/REDESIGN_REVIEW_ANALYSIS_20261030.md`(이하 RA); S1–S4 보고(`plan/step2a_s1_20261030.md` … `step2a_s4_20261030.md`); `CONDITIONS_POLICY.md`.
- 코드: `kairos/lib/llvm/target_rules.cpp`·`.hpp`(규칙 대상), `relations.cpp`·`.hpp`(질의), `p0.cpp`·`p0.hpp`(P0 표·비교 표), `lockset.cpp`·`.hpp`(F5와 LOCK_PATHS), `model.hpp`·`serialize.cpp`·`facts_text.cpp`, `wrappers.cpp`·`wrappers.hpp`·`spec.hpp`(규칙 대상의 연결과 기록), `include/kz/schema/kz_facts.h`·`kz_schema.h`, `tools/kz-static/cmd_extract.cpp`·`cmd_show.cpp`·`commands.hpp`·`main.cpp`. 문서는 `kairos/docs/static_facts.md` §5e·§5f, `kairos/docs/schema.md` §19.
- 측정 드라이버: `env/redesign/step2a_tests.sh`(identity, npsame, run, s5same, fixsame, twice, count, sha)와 `env/redesign/step2a_tests_count.py`. 출력은 저장소 밖 `/home/user/work/redesign/step2a/tests/`(이하 `T/`), 해시는 `T/sha256.txt`(121 파일). S5 때의 출력은 `/home/user/work/redesign/step2a/tests_s5/`(이하 `T_S5/`), 수정 단계의 출력은 `/home/user/work/redesign/step2a/tests_fix1/`(이하 `T_F1/`)에 그대로 두었다. 시험의 근거 줄은 `T/<build>/NP/relations.txt:L`(아래 "rel L")과 `T/<build>/NP/facts.txt:L`로 적는다. 증거 묶음은 `T/count/summary.txt`다.
- 도구: 병합 트리 빌드 `merge-check/kz-llvm/tools/kz-static/kz-static`. 측정은 commit `b00c370`의 트리에서 `kairos` 아래 미커밋 변경 없이 돌렸다(`T/identity/identity.txt` 머리). 수정 단계는 `b76a9a9`였다.
- 표기: [M] 이번에 도구로 확인, [F] 소스·문서에서 읽음, [I] 추론, [U] 미확인, [D] 이 단계의 결정.
- 사례 이름의 쓰임: 엔진 코드와 내장 표에는 사례 이름이 없다(`lint_case-ids` 통과 [M]). 사례·프레임워크 이름은 이 문서와 계수 스크립트의 **조회**(어느 행이 시험이 말하는 행인지 고르기)에만 쓴다. 프로필 없는 실행(NP)에는 프로필, 명령줄 어휘, `--target` 목록, G0 이름, 사례 rspec·`.kzm`·manifest를 주지 않았다. `--targets-rule`은 목록이 아니라 도구 규칙이다. `identity` 단계만 단계 2의 V2(사례 프로필)를 프로필 추출에 그대로 주어 그 모드가 바뀌지 않았음을 본다.

## 0. 요약

1. **구현** [M]
   - `extract --no-profile --targets-rule R-T1,R-T2`: 둘째 읽기의 추출 대상을 첫째 읽기의 확장 사실에서 규칙으로 고른다. 두 규칙 모두 공유 base(전역 객체, 전역 배열 원소, 전역 포인터·전역에서 시작한 포인터 필드의 대상, 전역 주소만 돌려주는 함수의 호출 결과)의 레코드 필드만 고른다(§1.1). 조건 트리 걷기에 깊이 상한이 없다(수정 단계).
   - `kz-static relations <facts> [-o OUT]`: 경로 A·B·C의 관계와 반사실 뒤집힘 신호를 행으로 낸다. 행마다 의존 근거, 실행 순서, 실현 가능성(등급과 이유), 위반 근거, 뒤집힘 신호, 입력 계층이 따로 된 칸이다. 필드·callee·사례 이름을 받지 않는다(§1.2). 둘째 수정에서 **규칙 개정 3**이 되었다: 경로 C의 "불가"를 조건 여섯(읽기 문맥 전부, 레코드 전체·덮는 쓰기, 다시 잡힌 잠금, 정적 인스턴스 아닌 공통 잠금, 필드의 완전성과 풀리지 않은 호출, 생성자 호출자의 제 본문 쓰기)으로 좁혔고, 자식이 닿지 않는 호출 문맥은 `not graded`로 낸다(§11 #9–#12, #14). 트램펄린의 root, B의 등급 문장, probe-create의 극성, 이미지 키·이미지 entry의 쓰기 검사, layers의 `P0-unverified(...)`를 더했다.
   - **facts schema 개정**: S5는 10, 수정 단계 11(LOCK_PATHS), 둘째 수정은 그대로 11이다. 확장 사실을 쓰는 추출의 BASIS에 걷기 상한 기록 둘(`param.walk_bounds`, `count.walk_bounds_hit`)이 더해졌다(§11 #20).
   - **P0 표 개정 3**(72항목 그대로; 개정 2는 수정 단계)과 **P0 비교 표 개정 2**(13항목). 개정 3은 scope 문장을 고쳐 묶음의 어느 구성원이 항목인지와 빠진 묶음·구성원(다섯 빌드가 부르는 join·detach, timer, select·poll, 초기화·소멸·속성, 스케줄링, `sscanf` 등)을 적고, `thrd_create`에 "시작 전 저장"을, `pthread_create` 원문 줄에 배포판 패치 미대조를 적었다(§1.3, §11 #18, #19).
2. **기존 동작은 바뀌지 않았다** [M] (§2).
   - 시험 237/237 통과(`test_probe_anchor_reached`는 원래대로 건너뜀). 수정 단계의 235에 fixture `relsound.c`의 compile 시험과 `static_golden_relsound_np`를 더했다. `test_static_cli` 515개 확인(수정 단계 475; 측정 뒤의 시험 commit 둘이 이미지 키·이미지 entry의 "실행 중에도 쓰임" 경우 12개를 더함), `test_static_wrappers` 512개 확인. lint 6/6, `case-ids`는 이제 `tools/`도 읽는다(§11 #21).
   - golden: 프로필 추출의 golden 변화 0. 2a의 프로필 없는 추출 golden 다섯(`wrappers`, `flowfacts_np`, `argflow_np`, `lockset_np`, `lockpaths_np`)은 P0 표 개정 3의 줄만 바뀌었다(요약의 판 1줄; `wrappers`는 P0 행 두 줄 더). relations golden 셋(`relations`, `relations_np`, `relpaths_np`)은 규칙 개정 3으로 바뀌었다. 새 golden 1(`relsound_np`).
   - 프로필 추출: V0·V2 × C1 pre tx·R2 core·C5 6.4.1 여섯 경우 모두 단계 2 출력과 섹션 CRC가 STRINGS 말고 같다. 다른 레코드는 S1–S5·수정 단계와 같은 셋(build identity, parent basis 해시, `units_from`)이다.
   - 프로필 없는 추출(`fixsame`, 수정 단계 출력 대비): 다섯 빌드 모두 BASIS(걷기 상한 두 항목, P0 판·scope, `p0.pthread_create`·`p0.thrd_create`)와 P0_TABLE(행 두 개) 말고 36개 섹션의 레코드가 같다. 걷기 상한에 걸린 수는 다섯 빌드 모두 0이다.
   - 두 번 돌린 facts와 relations가 다섯 빌드 모두 바이트 단위로 같다(T7).
3. **완료 시험** [M] (§4). 사례 입력 없이(두 문장 내장 프로필, P0, L0, 빌드만; G0 없음) 돌렸다. 판정은 둘째 수정의 출력에 따른다. PI 대기 결정 A2(사전 등록)를 기준으로 읽은 표시는 §5에 따로 적는다.

   | 시험 | 판정 | 한 줄 |
   |---|---|---|
   | T1 R2 | **PASS** (개정 뒤 통과, "관계를 낸다"의 통과: §5) | B 행 하나에 39곳, writer 3, callee 검사(`EVS_GetAppDataByID == NULL → CFE_EVS_APP_ILLEGAL_APP_ID`), 39/39 버림, 행위자 다름(F7 task-main root), 실현 가능성 "fixed unconfirmed (creation order + await; the await rests on the L1 premise only)". 순서 조건은 대체 표시 "고정 미확인"으로 문자 그대로 채워지고 완전성 규칙을 어느 쪽으로 정해도 채워진다(형 단위면 "creation order only"). 뒤집힘 static-undetermined |
   | T2 C1 | **PARTIAL** | 같은 구획(상수 3), 게시 쓰기, BP `:126 → :132` probe-create(이제 극성 확인과 handling 칸), BP 키 `"BP0_tsem"`, SCH_LAB 키 상수 불일치, 수정 후 재시도(25, `-17`, F8 지연; 탈출식의 지역 변수와 조회 결과의 연결은 facts에 없음)가 나온다. 수정 전 실패 → 종료 도달은 미달: (a) `CF_AppMain`의 `status`는 정의가 둘이어서 흐름 민감한 도달 정의가 필요하고, (b) ED §3.0의 F8 예("외부 앱 분기에서 `OS_TaskExit`")가 원문과 어긋난다: `OS_TaskExit`는 CORE 가지에 있고 외부 앱 가지는 탈출 없는 지연 루프다 |
   | T3 C5 | **PARTIAL** (수정 단계 PASS에서 내림) | 6.4.1: spawn 잠금 ∅, 등록 쓰기 {#9}, `CFE_ES_GetAppID` → "possible: lock window". 6.4.2: L800·L445·L551 "proven-impossible"(범위: `CFE_ES_AppCreate`의 한 호출), L1858·L1892 "undecidable(failure path)". 미달: **L667·L1020·L1284는 `not graded`**다. 다섯 스크립트 앱(spawn의 자식)은 `CFE_ES_WaitForStartupSync`·`CFE_ES_CreateChildTask`·`CFE_ES_ExitChildTask`를 부르지 않는다 [M][F]. 수정 단계의 그 "불가"는 자식이 실행하지 않는 문맥을 호출 지점 잠금으로 등급 매긴 것이었다(§11 #9). L1284는 `CFE_ES_CreateChildTask` spawn(자식 `CFE_ES_PerfLogDump`)의 행으로만 "불가"다 |
   | T4 C5 수명 | **PASS(질의 수준)** | 조건은 모두 나온다. 단 "writer에 `pthread_create`"는 relations가 질의 때 ARGFLOW와 P0 표를 맞춘 것이고, ED §3.0 G10이 말한 추출의 writer(그래서 `id`가 DISABLED를 벗어남)는 실현되지 않았다(§8 항목 10, §11 #13). entry 경로 문장은 읽지 않는 것(종료 처리기, 소멸자 함수, 예외 단위)을 이름으로 적는다 |
   | T5 입력 계층 | **PARTIAL** | 다섯 빌드의 모든 행(1,438)이 B/P0/(L0)/R만이다. 사례 계층 0, G0 0. 그러나 원문 대조 없는 P0 항목에 기대는 행이 272 / 272 / 138 / 149 / 147이고(layers `P0-unverified(...)`, 새 표시), PI A4의 기본값(결정하지 않으면 의존이 0일 때만 통과)으로는 통과로 세지 않는다. basis 편차 159·156행, 선언된 가정·PI 대기 규칙 0 / 0 / 55 / 17 / 16행 |
   | T6 정상 대조 | **PASS** | C5 OSAL `*GetIdByName`·`*Create` 6쌍과 C1·R2의 `OS_ObjectIdFindNextMatch` 콜백 3종이 A로 인식된다. 모든 행의 위반 근거 칸은 unconfirmed다(기록을 주지 않았으므로 자명하게) |
   | T7 결정성 | **PASS** | facts·relations 두 번 바이트 동일(다섯 빌드) |

4. **반증 조건의 결과** [M][I] (§5). T1–T3 어느 것에도 **사례 입력을 더하지 않았고**, 세 사례의 관계가 행으로 나왔다. 그래서 ED §6의 반증 조건("사례 입력을 더하지 않으면 관계가 나오지 않음")은 **충족되지 않았다**(ED §6의 뜻에서 반증되지 않았다). 그러나 PI A2 사전 등록 ④·⑤로 읽으면 이것은 탐지 근거가 아니다: T1·T2의 통과는 "관계를 낸다"의 통과이고(미확인·미해결 표시), T1은 시험 재료를 보고 바꾼 규칙(F2 도달 정의, A5 표) 뒤의 통과이며, T1–T3의 행은 두 차례 수정(시험 행을 계기로 한 규칙 개정 2·3) 뒤의 것이다(§5). 그래서 "사례별 Φ 없이 근거 있는 관계를 낸다"를 **성립했다고 적지 않는다**.
   - T2의 종료 도달이 미달이다(도달 정의 사실 없음, ED의 F8 예가 원문과 어긋남).
   - T3의 6.4.2 읽기 지점 셋은 자식이 실행하지 않는 문맥이어서 등급이 없다. ED가 그 셋을 자식의 읽기 지점으로 적은 전제가 원문과 어긋난다.
   - 남은 수작업 입력(L0 시작 스크립트 블록, C5 `front_end_diagnostic` 2줄)과 미대조 P0 원문 71+13개가 그대로이고, 행의 대부분이 거기에 기댄다(`P0-unverified`).
   - 사례 관계는 후보 가운데 소수다(행 175–393개 가운데 1–8개, 0.3–4.6%, §6).
   - "고정된 잘못된 순서"(R2)와 "반대 순서 가능"(C1)은 정적으로 증명되지 않는다. 위반 근거는 실행 기록 없이는 늘 unconfirmed다.
5. **ED §2.3 재측정** [M] (§3). 공유 base 필터를 건 R-T1 ∪ R-T2의 대상 수는 C1 340, R2 240, C5 259/261이다(V3 R-T1: 841 / 504 / 322). 두 수정 단계에서 바뀌지 않았다. 사례 필드 가운데 R2의 5개(`EVS_AppData_t.AppID`, `CFE_ES_AppRecord_t.AppId`·`AppState`·`Type`, `CFE_PSP_IdleTaskState_t.ShutdownReq`)와 C5 `CFE_ES_TaskRecord_t.TaskId`는 매개변수·지역 포인터로만 닿아 이 필터에서 빠진다.

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

### 1.2 `kz-static relations` (`lib/llvm/relations.cpp`, "relations rules revision 3")

규칙 전문은 `kairos/docs/static_facts.md` §5e(개정 1), §5f(개정 2에서 바뀐 것), §5g(둘째 수정의 개정 3에서 바뀐 것)다. 요점만 적는다. 개정 3에서 바뀐 곳은 "(개정 3)"으로 표시한다.

- **입력**: facts 컨테이너 하나. 확장 사실이 없으면 이유와 함께 거부한다(exit 1). 개정 2는 LOCK_PATHS(schema 개정 11)도 요구한다. 받는 옵션은 `-o`뿐이다. 필드·callee·사례 이름을 받지 않는다.
- **머리**: facts 해시, 모드, 내장 프로필·P0 표(판과 `p0_table_scope`)·유도 규칙·대상 규칙의 판, **P0 비교 표**(판, 항목, 담지 않는 묶음), facts에 복사된 basis 편차, 행위자 root(종류별 수)와 트램펄린, 트램펄린이 유도 spawn 연산의 호출로 대신하는 root와 entry를 풀지 못한 그런 호출의 수(개정 3), 다른 root에 들어가지만 행위자로 남은 root, 정적 이미지로 푼 spawn(그 멤버에 쓰기가 없다는 확인과 함께, 개정 3), entry를 풀지 못한 spawn과 그 이유, 경로·등급별 수(`not graded` 포함, 개정 3), `P0-unverified` 표시의 뜻과 `pthread_create` 원문 대조의 범위(개정 3).
- **정적 행위자** [D]: root = `main`, 해결된 spawn entry(CALL_TARGETS, table walk 반복의 task-main, 시작 스크립트 진입), 그리고 spawn의 entry 인자가 정적 이미지가 있는 전역 표의 필드에서 오면 그 멤버의 함수 잎(`table-image-entry`, 수정 단계). root의 코드는 다른 root에 들어가지 않고 호출 그래프로 닿는 함수다. 트램펄린(행위자가 아니고 그 코드는 그것이 부르는 root들의 것)은 process가 아닌 root 가운데 **다른 root에 간접 호출로만 들어가고, 제 본문이 공유 root로 필드·전역을 읽지 않으며 전역을 쓰지 않는** 것이다(수정 단계; S5는 "다른 root를 부르면" 모두 트램펄린이었다, §10 지적 9). 다섯 빌드에서는 R2·C1의 `CFE_ES_TaskEntryPoint`, `OS_PthreadTaskEntry`가 트램펄린이고 S5와 같다 [M]. 트램펄린이 대신하는 root는 (가) 제 코드가 해결된 간접 호출(필드 기반 함수 포인터 흐름)로 들어가는 root와, (나) 개정 3부터 그 트램펄린을 안쪽 spawn에 넘기는 유도 spawn 연산(SPAWN_WRAP `called-by-entry`)의 호출 지점에서 푼 entry(table walk, 정적 이미지, 시작 스크립트, CALL_TARGETS)다. 수정 단계의 문서는 이것을 "필드 기반 may 근사"(과대 근사)라 적었으나, (가)만으로는 C1에서 정적 이미지 entry와 스크립트 entry가 빠지는 **과소 근사**였다(§11 #2). (나)를 더한 뒤에도 entry를 풀지 못한 그런 호출의 스레드는 빠진다(머리에 수, §8 항목 14).
- **경로 B**: 공유 base 필드 인자를 (필드, 본문 있는 callee, 인자 위치)로 묶는다. writer가 없거나 한 행위자뿐이면 뺀다. 칸: 사이트와 결과 처리, INIT·writer(대상이면 WRITERS의 값·guard), 행위자, callee 검사(그 매개변수가 조건에 나오는 상수 반환, 그리고 조건 안에서 그 매개변수로 부르는 함수의 RET_CONSTS 한 단계).
- **경로 C (개정 3에서 "불가"의 조건을 고침, §11 #9–#12, #14, #15)**: 읽기 문맥은 읽기가 제 함수에서 정적 인스턴스 잠금을 잡으면 그 읽기 하나, 아니면 (i) 읽는 함수가 spawn된 스레드의 entry이면 호출자 잠금 없는 문맥, (ii) 직접 호출마다, (iii) 해결된 간접 호출마다 하나다. 자식이 닿지 않는 호출의 문맥은 `not graded`(등급 없음)다. spawn 뒤 쓰기는 그 필드의 형 있는 쓰기에 더해 레코드 전체 쓰기(`T.*`)와 그 필드를 덮을 수 있는 데이터 함수 쓰기를 세고, 본문 없는 P0 밖 함수에 넘긴 함수도 호출 대상으로 센다. `proven-impossible`은 한 정체성 L이 spawn과 모든 쓰기에서 같은 획득으로 잡히고 그 획득 지점이 spawn 뒤에 다시 닿지 않으며(되돌아가는 간선 검사), 읽기가 L을 잡고, L이 정적 인스턴스이고, 필드가 필드 단위 규칙으로 complete이며, spawn 뒤에 대상이 풀리지 않은 간접 호출이 없고, 생성자를 부르는 함수의 제 본문에 그 필드 쓰기가 없을 때만이다. 범위는 생성자의 한 호출이다(ED §3.3 항목 3). L이 정적 인스턴스가 아니면 `undecidable`이다. 아래는 개정 2까지의 기술이다. 해결된 entry가 있는 spawn마다, 생성자 함수가 spawn **뒤**에 쓰는 필드와 spawn된 entry의 행위자 코드가 그 필드를 분기·반환·out 저장으로 읽는 곳의 짝이다. "뒤"는 수정 단계부터 LOCK_PATHS `after-spawn`(생성자의 제어 흐름 그래프에서 spawn 호출 뒤에 닿는 사건; 되돌아가는 간선 포함, 분기 가지치기 없음)이다. S5는 소스 위치였다(§10 지적 3). 생성자가 spawn 뒤에 부르는 함수의 같은 필드 쓰기도 실현 가능성에 넣는다(한 호출 아래는 호출 지점의 잠금과 합치고, 더 아래는 제 잠금만 센다). 본문 있는 유도 spawn 연산이 주소 인자를 통해 저장하는 필드(PARAM_FLOW out-store, ARGFLOW 원천이 그 필드의 주소)는 spawn 뒤의 쓰기로 세고 잠금은 모르는 것으로 둔다: CFG 순서에서 인자 사건은 호출보다 앞이고, 그 호출 안에서 스레드 생성과의 순서는 facts가 주지 않기 때문이다. P0 스레드 생성 연산의 out 인자는 새 스레드가 돌기 전에 저장된다고 보고(P0 의미) 세지 않는다. 생성자 본문에서는 쓰지 않고 callee에서만 쓰는 필드는 짝짓지 않고 `[C]` 머리에 수를 낸다. 생성자 정체성(spawn 연산의 out-store 매개변수), 쓰기 첨자(WRITERS key), 읽기 첨자가 자기 정체성인지(P0 thread-self·thread-specific-get에 닿는 사슬)를 적는다. 읽기가 정적 인스턴스 잠금을 잡지 않으면 그 함수의 호출 지점마다 행을 따로 내고, 읽기 행에 LOCK_PATHS `entry-release`가 있으면 호출 지점의 잠금을 읽기에 더하지 않는다(수정 단계, §10 지적 4). P0 스레드 생성의 out 인자를 spawn 뒤로 세지 않는 근거는 그 종류 항목의 의미 문장이다(개정 3에서 `thrd_create`에도 적음, §11 #19).
- **경로 A**: 키 비교(P0 비교 표 함수 호출, 또는 분기의 `==`/`!=`)가 레코드 필드와 매개변수 사이에 있고, 그것이 검색 루프 안(A1)이거나 검색 루프의 간접 호출이 부르는 함수 안(A2)이며, 검색이 그 형을 전역 표에서 읽을 때 이름 공간(레코드 형, 키 필드)을 만든다. PARAM_FLOW로 키 매개변수 함수를 모으고, 바깥에서의 호출을 사용으로, 키 필드 쓰기에 닿는 것을 생성자로 나눈다. 구획은 사슬의 첫 상수 인자, 키 값은 문자열·상수 또는 PTR_FIELDS + GLOBAL_IMAGES다. 짝에서 빼는 이유(키 상수 다름, 한 행위자, probe-create)를 적는다. 개정 3: probe-create는 극성(생성자가 조회의 "맞음 없음" 결과에서 돌고 다른 값에서는 돌지 않음)을 RET_CONSTS·RETURNS로 확인할 때만 붙이고, 그 조회 행의 handling 칸에 적는다. 짝이 없을 때 dependency 칸은 제외 이유별 수를 적는다. 이미지 잎이 실행 중에도 쓰이는 키는 `unresolved`이고 제외·뒤집힘에 쓰지 않는다(§11 #6, #7). 처리: 감싼 루프(종류·횟수·탈출식·지연과 F8 사슬), 결과를 돌려주는 함수 사슬, 그 길의 종료 연산. 종료 연산이 may·noreturn이면 그 종료 호출의 guard와, 이 호출에서 인자가 갖는 상수 값으로 그 guard를 평가한 결과, 그 함수의 탈출 없는 루프를 적는다(수정 단계, §10 지적 2).
- **수명**(경로 A 이름 공간 블록): 키·활성 필드의 writer(covers-more-than-the-field를 따로 셈), P0 out 인자 쓰기(relations가 ARGFLOW와 P0_TABLE을 맞춘 것이고 추출의 writer 집합에는 없다고 적음), entry 경로 규칙(프로그램에 null이 아닌 소멸자를 단 thread-specific key 생성도 cleanup push도 없을 때만 "entry가 돌아온 뒤 프로그램의 함수가 돌지 않는다"고 씀), 블록의 `namespace layers` 줄.
- **칸(ED §3.4)**: `dependency`, `run-order`(이 판은 기록을 읽지 않는다: 늘 `no record given`), `feasibility`(`proven-impossible`/`possible`/`undecidable` + 이유; 수정 단계부터 "불가"는 기댄 가정과 규칙을 적는다; 개정 3부터 자식이 닿지 않는 호출의 C 문맥은 `not graded`), `violation`(RV2 §5.4; 기록 없으면 `unconfirmed`), `flip`(`static-possible`/`static-undetermined`/`none` + 이유), `layers`(B, P0, L0, R; 프로필 facts면 `case:profile(…)`, 명령줄 어휘가 있으면 `hand-entered:command-line(…)`; 수정 단계부터 `basis-deviation(…)`과 `rests-on(…)`; 개정 3부터 원문 대조가 없는 P0 항목에 기대면 `P0-unverified(…)`, 정적 인스턴스 판정을 쓰는 C의 모든 등급에 `rests-on(…)`).
- **실현 가능성** [D]: C는 위 조건이 모두 성립할 때만 "불가"를 낸다. B는 펼친 table walk(LOOP_ITERS)의 생성 순서만 읽고 `undecidable`로 낸다. 등급 문장은 `fixed unconfirmed (creation order only)`와 `fixed unconfirmed (creation order + await; the await rests on the L1 premise only)`로 나뉘고, await 문장은 L1 전제가 성립한다는 것만 말한다(개정 3, §11 #3). 정적 이미지에서 푼 root끼리는 순서를 유도하지 않는다고 적는다(개정 3, §11 #1). A는 늘 `undecidable`이다.
- **결정성**: 정렬된 맵만 쓰고, 식 트리는 쓰는 쪽의 후위 번호(자식 < 부모)를 따라 걷는다(깊이 상한 없음). 텍스트의 식 출력은 `show`와 같은 40단계 출력 한계를 쓴다. 이 한계는 출력에만 쓴다: callee 검사와 상수 반환의 중복 제거는 (값, 조건 트리)로 한다(수정 단계; S5는 잘린 텍스트를 키로 썼다, §10 지적 11). 이 한계로 잘린 식은 다섯 빌드의 relations에 0개다 [M].

### 1.3 LOCK_PATHS와 P0 표 개정 2·3 (수정 단계, 둘째 수정)

- **LOCK_PATHS**(facts schema 개정 11, 섹션 136) [D]: `after-spawn`(spawn 호출 뒤에 그 함수의 제어 흐름 그래프로 닿는 LOCKSET 사건)과 `entry-release`(함수 진입에서 그 사건까지의 어떤 경로에서 함수가 잡지 않은 잠금을 풀거나, 정적 인스턴스가 아닌 정체성의 잠금을 지울 수 있거나, 효과를 따라가지 못하는 호출이 있음; `site`는 소스 위치가 가장 앞선 그런 호출). LOCKSET과 같은 분석에서 구하고 LOCKSET 행은 바뀌지 않는다 [M: §2a]. 다섯 빌드의 행 수: 9,330 / 9,331 / 5,803 / 2,335 / 2,344(after-spawn 234 / 234 / 242 / 198 / 203, spawn 호출 15 / 15 / 14 / 7 / 7) [M].
- **P0 표 개정 3** (둘째 수정) [D]: 항목 72와 종류는 그대로다. scope 문장에서 "묶음을 통째로(listed whole)"를 없애고, 묶음의 어느 구성원이 항목인지와 빠진 묶음·구성원(스레드 join·detach, POSIX timer, I/O 다중화, 초기화·소멸·속성·조회 구성원, 스케줄링, `stpcpy`·`strlcpy` 등 데이터 함수, `sscanf` 등 형식 입력, 프로세스 종료 처리기 등록)을 이름으로 적는다. 다섯 빌드가 부르는 것 가운데 이 판 밖인 것: `pthread_join`(C1 1, R2 3), `pthread_detach`·`timer_create`·`timer_settime`·`timer_delete`(다섯 빌드), `select`(C1, R2), `poll`(R2), mutex·조건 변수·세마포어 초기화·소멸과 속성 함수, `sem_getvalue`(C1, R2), `sched_get_priority_*`·`pthread_setschedparam`, `sscanf`(C5) [M]. `thrd_create` 의미에 "새 스레드가 시작하기 전에 저장"을 적었고(원문 미대조), `pthread_create` 원문 줄에 배포판 패키지의 패치를 대조하지 않았다고 적었다(빌드는 `Ubuntu GLIBC 2.39-0ubuntu8.9`를 링크한다 [M]: `ldd --version`, `dpkg -l libc6`). §11 #18, #19.
- **P0 표 개정 2** [D]: 개정 1의 묶음에 C11 `<threads.h>`의 같은 묶음 함수(`thrd_create`, `mtx_*`, `cnd_*`, `thrd_sleep`, `thrd_exit`, `thrd_current`, `tss_*`), glibc 2.39의 `*_clock*` 함수(`pthread_mutex_clocklock`, `pthread_cond_clockwait`, `sem_clockwait`), spin lock, `usleep`, `quick_exit`, assert 매크로가 부르는 `__assert_fail`·`__assert_perror_fail`, thread-specific key 생성·삭제, cleanup handler push·pop(`pthread_cleanup_push/pop` 매크로가 예외 없이 컴파일한 단위에서 부르는 함수), `dlmopen`·`dlvsym`을 더했다. 72항목, 항목마다 판 표기, 원문 대조는 `pthread_create` 하나다(개정 2의 "묶음을 통째로"는 초기화·소멸·속성 구성원과 몇 묶음을 빠뜨린 말이었다: 개정 3). 담지 않는 묶음(읽기-쓰기 잠금 전체, barrier, one-time init, 다른 스레드의 취소, 시그널, `dlclose`·`dlerror`·`dladdr`, 컴파일러·fortify가 원문 호출 없이 부르는 함수)은 `extract.p0_table_scope`에 이름으로 적는다. 읽기-쓰기 잠금을 통째로 뺀 까닭은 잠금 집합 분석이 공유 획득을 읽지 않기 때문이다: 쓰기 획득만 넣으면 읽기 획득 뒤의 해제가 호출자 잠금의 해제로 읽힌다. 통째로 빼면 그 획득이 잠금 연산이 아니어서 판정이 약해지기만 한다(R2는 `pthread_rwlock_rdlock/wrlock`을 부른다 [M]).
- **P0 비교 표 개정 2** [D]: `strcoll_l`, `strcasecmp_l`, `strncasecmp_l`, `bcmp`, `strverscmp`, `thrd_equal`을 더했다(13항목, 모두 원문 대조 unverified). 넓은 문자 비교와 `__memcmpeq`는 이 개정 밖이다.
- 새 P0 항목 가운데 다섯 빌드가 부르는 것은 `pthread_key_create`(다섯 빌드)와 `__pthread_register_cancel`·`__pthread_unregister_cancel`(C1·R2)뿐이다 [M]. 어휘가 아니어서 유도에 쓰이지 않으므로 FACTS, OP_DERIV, SPAWN_WRAP, LOCK_* 등 유도 결과는 S5와 같다 [M: §2a].

## 2. 기존 동작 불변 확인 [M]

| 확인 | 결과 | 근거 |
|---|---|---|
| 시험 | 237/237 통과(1개 건너뜀은 원래대로). 수정 단계 뒤 새 시험 2개(`static_fixture_compile_relsound`, `static_golden_relsound_np`). `test_static_cli` 515개 확인: 수정 단계의 확인에 더해 P0 표 개정 3의 scope와 걷기 상한 기록, relations 개정 3의 `P0-unverified`·probe-create handling 줄·이미지 멤버 확인·entry 문맥·`not graded`, fixture `relsound.c`의 여덟 경우; 측정 뒤의 commit `9bef6ae`·`30c8713`이 이미지 키(`relations.c` 변형)와 정적 이미지 entry(`relpaths.c` 변형)가 실행 중에도 쓰이는 경우 12개를 더해 515개. `test_static_wrappers` 512개 확인(개정 3의 scope 이름, 스레드 생성 항목의 "시작 전 저장", `pthread_create`의 배포판 패치 미대조). lint 6/6(`case-ids`가 `tools/`도 읽음; 자기 시험에 `tools/` 경우를 더함) | ctest 로그 |
| golden | 프로필 추출의 golden 변화 0. 2a의 프로필 없는 golden 다섯은 P0 표 개정 3의 줄만 바뀜(`wrappers`는 3줄, 나머지 1줄). relations golden 셋은 규칙 개정 3으로 바뀜. 새 golden 1(`relsound_np`). 수정 단계까지의 변화는 §10과 같음 | `git diff f10afbb..HEAD -- kairos/tests` |
| 프로필 추출(identity) | V0·V2 × C1 pre tx·R2 core·C5 6.4.1: basis 6섹션·facts 22섹션 가운데 CRC가 다른 것은 STRINGS뿐. 다른 레코드는 basis 1(`front_end.build_identity`), facts 3(`extract.build_identity`, parent basis 해시, `extract.units_from`). show 텍스트 차이 2줄 | `T/identity/identity.txt` |
| 프로필 없는 추출, 옵션 없음(npsame, S4 대비) | 35개 섹션 레코드 같음. BASIS·P0_TABLE 다름, LOCK_PATHS는 새 섹션(§2a) | `T/npsame/npsame.txt` |
| 프로필 없는 추출, `--targets-rule`(s5same, S5 대비) | 같은 35개 섹션 레코드 같음(WRITERS, FACT_LOCS, FIELD_STATUS, LOCKSET 포함). BASIS·P0_TABLE 다름, LOCK_PATHS 새 섹션(§2a) | `T/s5same/s5same.txt` |
| 프로필 없는 추출, `--targets-rule`(fixsame, 수정 단계 대비) | 36개 섹션 레코드 같음(LOCK_PATHS, FACT_INIT, CALL_TARGETS 포함). BASIS·P0_TABLE만 다름(§2a) | `T/fixsame/fixsame.txt` |
| 결정성(T7) | 다섯 빌드 facts·relations 두 번 같음 | `T/twice.txt` |

### 2a. npsame, s5same, fixsame

다섯 빌드 모두 STRINGS 말고 35개 섹션(ARGFLOW … WRITERS, LOCKSET 포함)의 레코드가 S4(옵션 없음)·S5(`--targets-rule`) facts와 같다 [M]. 다른 것은 셋이다.
- **BASIS**: 항목 이름으로 비교하면 S5 대비 더해진 것은 `p0.*` 34개(P0 표 개정 2), `extract.p0_table_scope`, `count.lock_paths`, `extract.basis_deviation[…]` 2개(C1·R2) 또는 7개(C5), 그리고 둘째 수정의 `param.walk_bounds`, `count.walk_bounds_hit`다. 값이 바뀐 것은 `extract.p0_table`(판과 수), `extract.p0_table_scope`(개정 3의 문장), `p0.pthread_create`·`p0.thrd_create`(개정 3), `assume.lock_acquisition_succeeds`(획득 기본 연산이 mutex에서 spin lock·`mtx_lock`까지로 넓어진 문장), 그리고 매번 바뀌는 `extract.build_identity`, `extract.units_from`이다. 빠진 항목은 없다.
- **P0_TABLE**: 72행. S5의 38행은 항목별로 그대로이고, 34행이 묶음 안에 끼어 들어갔다(그래서 위치 비교로는 다르다). 개정 3은 `pthread_create`의 원문 줄과 `thrd_create`의 의미 두 행만 바꿨다.
- **LOCK_PATHS**: 수정 단계의 새 섹션. 둘째 수정 대비 같다(fixsame).
- **걷기 상한**: `count.walk_bounds_hit`는 다섯 빌드 모두 "FACT_INIT walk 0 objects, function-value initializer nesting 0"이다. 그래서 FACT_INIT의 새 대체 규칙("상수 아님" 행)이 낸 행은 없고 FACT_INIT·CALL_TARGETS는 수정 단계와 같다 [M].

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

근거 줄은 `T/<build>/NP/relations.txt`의 줄 번호(rel L)다. 모든 행은 `T/count/summary.txt`에서도 같은 줄 번호로 나온다. 판정은 둘째 수정 단계의 출력이다. PI A2 사전 등록 ④·⑤로 읽은 표시는 §5에 따로 적는다.

### T1 R2 — **PASS** (개정 뒤 통과, "관계를 낸다"의 통과)

**통과 조건(ED §6)**: B 관계 `CFE_SB_Global_t.AppId → CFE_EVS_SendEventWithAppID#2`; 39 지점; 쓰기 3(EarlyInit 0, AppInit out-pointer 둘); callee 검사 `EVS_GetAppDataByID == NULL → CFE_EVS_APP_ILLEGAL_APP_ID`(F2); 결과 버림 39/39; 읽는 쪽과 쓰는 쪽 행위자 다름(G0 없이, F7의 task-main root로); 순서 조건: 코어 초기화 순서가 고정됨(생성 순서 + `CFE_ES_MainTaskSyncDelay` await)을 보이거나, 보이지 못하면 "고정 미확인"을 실현 가능성 칸에 적음.

**나온 것** (r2_core; 수정 단계와 같은 행이다. 다른 곳은 줄 번호가 아니라 세 칸이다: 실현 가능성 칸의 등급 문장과 await 문장(개정 3, §11 #3), layers 칸의 `P0-unverified(...)`(§11 #18). S5 대비로는 수정 단계에서 layers 칸과 실현 가능성 칸의 rests-on 문장이 이미 달라졌다: 수정 단계 문서의 "줄 번호와 layers 칸만 다르다"는 틀렸다, §11 #8)
- 행 B65(rel 2146): `CFE_SB_Global_t.AppId -> CFE_EVS_SendEventWithAppID#2 sites=39 callee-check=yes`. 사이트 39곳(`cfe_sb_api.c` 34, `cfe_sb_priv.c` 1, `cfe_sb_task.c` 4), base 전역, hops 0.
- 필드(rel 2186): INIT 0(`CFE_SB_Global`), writer 3, WRITERS(규칙 대상 R-T1, `facts.txt:107`).
  - rel 2187: memset `cfe_sb_init.c:50:5` in `CFE_SB_EarlyInit`, 값 0.
  - rel 2188: out-pointer `cfe_sb_task.c:138:5` in `CFE_SB_AppInit` via `CFE_ES_GetAppID`, 값 symbolic `[CFE_ES_AppRecordGetID(AppRecPtr)]`, guard `AppRecPtr != 0`.
  - rel 2189: 같은 곳, 값 0 `[CFE_ES_APPID_UNDEFINED]`, guard `AppRecPtr == 0`.
- 행위자(rel 2190): 읽는 쪽 {`CFE_ES_BackgroundTask`, `CFE_ES_TaskMain`, `CFE_EVS_TaskMain`, `CFE_SB_TaskMain`, `CFE_TBL_TaskMain`, `CFE_TIME_Local1HzTask`, `CFE_TIME_TaskMain`, `CFE_TIME_Tone1HzTask`}, 쓰는 쪽 {`CFE_SB_TaskMain`, `main`} → inter-actor. 개정 3의 트램펄린 규칙(유도 spawn 연산의 호출 지점 entry를 더함)으로도 R2의 트램펄린 root는 같다 [M].
- callee 검사(rel 2192): `CFE_EVS_SendEventWithAppID returns 3254779907 [CFE_EVS_APP_ILLEGAL_APP_ID] when (and (not (eq param:Spec#3 0)) (eq (call-result EVS_GetAppDataByID (seq param:AppID#2)) 0))`. 한 단계 아래(rel 2195): `EVS_GetAppDataByID returns 0 [NULL] when …`.
- 처리(rel 2197): `discarded 39, used 0, tested 0`.
- 실현 가능성(rel 2200): `undecidable: fixed unconfirmed (creation order + await; the await rests on the L1 premise only): creation order at the table walk cfe_es_start.c:810:26 … writer actor CFE_SB_TaskMain at iteration 4; reader actors created before it: CFE_ES_TaskMain (iteration 1), CFE_EVS_TaskMain (iteration 2); after it: CFE_TBL_TaskMain (iteration 5), CFE_TIME_TaskMain (iteration 6). The call cfe_es_start.c:835:26 (CFE_ES_MainTaskSyncDelay) after the creation reaches the await loop cfe_es_start.c:871:5 …: an await loop follows the creation (the L1 premise holds under the AWAIT_L1 per-field rule …; type-level star-closed); lemma L1 itself is not decided, and whether the loop's predicate concerns the created actor and whether its timeout exit ends the creator are not read. … neither order is proven`.
- 뒤집힘(rel 2202) `static-undetermined`, 위반 근거(rel 2201) unconfirmed, 계층(rel 2203) `B P0 R (writers: rule targets) rests-on(assume.no_cross_member_pointer_arithmetic; per-field completeness rule (PI decision pending)) P0-unverified(clock_nanosleep, pthread_cond_timedwait, pthread_cond_wait, pthread_mutex_lock, pthread_mutex_timedlock, pthread_mutex_unlock)`.

**판정**: 조건이 모두 나왔다. 순서 조건은 ED가 허용한 대체 표시 "고정 미확인"으로 **문자 그대로** 채워진다. 그 표시는 await와 무관하게 table walk의 생성 순서만으로도 붙는다: 형 단위 완전성 규칙을 고르면 await 문장이 빠지고 등급 문장이 `fixed unconfirmed (creation order only)`가 되지만 조건은 그대로 채워진다(§11 #3). await 문장은 L1의 전제(AWAIT_L1이 인정한 루프가 생성 뒤에 옴)까지만 말한다. 수정 단계의 "the creations are serialized by it"은 도구가 유도하지 않은 것(술어가 생성된 행위자에 관한 것, 시간 초과 출구가 끝남)을 주장했다. R2에서 시간 초과 출구는 `CFE_PSP_Panic` → `abort()`(pc-linux `cfe_psp_support.c:120`)로 끝나지만 [F] 도구가 유도하지 않는다. 값 0이 검사를 통과하지 못함(ED §3.2)은 상수 접기가 없어 평가 불가로 남는다.
- 참고 [M]: C1의 같은 관계(B73, rel 3153)는 쓰는 쪽 {`CFE_SB_TaskMain` (table-image-entry), `main`}, 읽는 쪽은 개정 3에서 15 root(rel 3208: 코어 task main 다섯, 스크립트 앱 다섯, spawn entry 다섯)다. 실현 가능성(rel 3218)은 "no static order derived: the spawn cfe_es_start.c:767:34 … starts writer actor(s) CFE_SB_TaskMain [[19]…] and reader actor(s) … [[21]…], [[17]…], …; for roots resolved from a static image this revision derives no creation order … and no await"다. **C1의 이 행은 T1의 생성 순서·await 문장을 얻지 않는다**: C1의 코어 task main은 정적 이미지에서 풀렸고, 이 판은 이미지 첨자를 반복 순서로 읽지 않는다. C1 원문은 R2와 같은 구조다(`cfe_es_start.c:716` 표 걷기, `:767` 생성, `:810–811` `CFE_ES_MainTaskSyncDelay`, `:892` await 루프) [F]. 수정 단계의 이 행 이유 "not created by one table walk"는 틀렸다(§11 #1, §8 항목 15). C1 B 행 가운데 이 문장을 받는 행은 49다(`summary.txt`).

### T2 C1 — **PARTIAL**

**통과 조건**: CF `cf_cfdp.c:993`(수정 후 `:1003`)과 BP `bp_flow.c:132`가 같은 구획; 게시 쓰기 L639/L640, L100, L1079; BP `:126 → :132` probe-create; BP 키 `"BP0_tsem"`(포인터 필드 흐름 + 이미지, 출처 `bp_semcfg.c` L43–L44)이 풀리거나 "필드 points-to 필요"로 표시; 수정 전 실패 분기 → 종료 도달(F2/F4, 종료 연산은 F8로 P0에서 유도); 수정 후 재시도(25, `!= -17`, 지연. 지연 연산은 F8로 P0에서 유도); SCH_LAB 생성은 키 상수 불일치로 짝 아님.

**나온 것** (c1_pre_tx; c1_post_tx는 같은 줄 번호에 같은 내용이고 다른 곳만 적는다)
- 이름 공간 `OS_common_record_t.name_entry`(rel 333): 검색 A2(rel 334) `OS_ObjectIdFindNextMatch`의 루프 `osapi-idmap.c:550:5`가 간접 호출(`:561:54`)로 `OS_ObjectNameMatch`의 `strcmp`(`:271:40`)에 닿는다. 활성 필드(rel 335) `OS_common_record_t.active_id`.
- 게시 쓰기(rel 380): `active_id` `osapi-idmap.c:639:9`, `:1079:9`·`:1088:9`, `:432:21`·`:519:13`; `name_entry` `osapi-countsem.c:100:9`, `osapi-idmap.c:640:9`.
- CF 조회 행 A21(rel 418): `cf_cfdp.c:993:19 … partition {3} key unresolved`, 키(rel 420) "needs field points-to (PTR_FIELDS)", 생성자(rel 422) BP `bp_flow.c:132:18`, (rel 423) SCH_LAB `sch_lab_app.c:168:16`, 둘 다 "key unresolved (lookup)"로 짝 후보. dependency(rel 428) "2 creator(s) of this partition in other actors". 계층(rel 433) `B P0 L0 R P0-unverified(abort, dlsym, pthread_exit, strcmp)`.
- BP 조회 행 A20(rel 403): 키(rel 405) `BP_Throttles[0].Name = "BP0_tsem"`(매크로 `CF_CHANNEL_0_SEM_NAME`, `bp_semcfg.c:44:31`). 짝 아님(rel 407): **probe-create, 극성 확인**: "the creator's key read runs when OS_CountSemGetIdByName returns -17 (no match: OS_CountSemGetIdByName -> OS_ObjectIdFindByName -> OS_ObjectIdGetByName -> OS_ObjectIdGetBySearch -> OS_ObjectIdFindNextMatch) and not when it returns 0". 짝 아님(rel 408): SCH_LAB, `key constants differ`. handling(rel 411): `probe-create: on the lookup's failure result the same function creates the same key: bp_flow.c:132:18 -> OS_CountSemCreate (…)`. dependency(rel 412): `no creator left after exclusions (probe-create 1, key constants differ 1, same actor only 0)`.
- 수정 전 처리(rel 424–427): 루프 `cf_cfdp.c:967:5`; `CF_CFDP_InitEngine`·`CF_Init`이 결과를 돌려줌; 종료(rel 427) `terminal CFE_ES_ExitApp at cf_app.c:366:5 … under (ne local:status 0) (the guard tests a local; … may)` 그리고 F8 사슬의 guard 평가 "with argument 0 = 3 every guard is false … the loop cfe_es_api.c:453:13 in CFE_ES_ExitApp has no exit".
- 수정 후 처리(c1_post_tx A21의 handling): `in the loop cf_cfdp.c:1001:13 kind count-bounded trip upper-bound 25 exit (or (not (lt local:j 25)) (and (lt local:j 25) (ne local:ret -17))) delay cf_cfdp.c:1011:17 OS_TaskDelay (F8 must OS_TaskDelay -> OS_TaskDelay_Impl -> clock_nanosleep)`.

**판정**: 조건 일곱 가운데 여섯이 나왔다. 그 가운데 둘은 문자 그대로만 채워진다.
- **probe-create**: 개정 3부터 극성(조회의 "맞음 없음" 값 −17에서 생성자가 돌고, 0에서는 돌지 않음)을 RET_CONSTS·RETURNS로 확인한 뒤에만 붙고, 조회 행의 handling 칸에 적힌다. 수정 단계에서는 "짝 아님" 이유로만 나왔고 극성을 보지 않았다(소스 모양으로만 맞았다, §11 #6).
- **수정 후 재시도**: 25, `-17`, F8 지연이 나온다. 그러나 탈출식의 `local:ret`이 조회 결과라는 연결은 facts에 없다. `ret`은 정의가 다섯이다(post `cf_cfdp.c:960`, `:971`, `:980`, `:1000`, `:1003` [F]). ED §3.5가 묻는 "탈출 상수가 그 호출의 실패 코드인가"는 사실로 연결되지 않고 문자 그대로만 채워진다(§11 #4).
- 미달은 "수정 전 실패 분기 → 종료 도달"이다.
  - (a) `CF_AppMain`의 `status`는 정의가 둘이다(`cf_app.c:330` `= CF_Init()`, `:342` `= CFE_SB_ReceiveBuffer(...)` [F]). WRITERS guard `(ne local:status 0)`(`cf_app.c:333`)을 `CF_Init`의 결과로 펼치려면 `cf_app.c:331`에서의 **흐름 민감한 도달 정의**가 필요하다. 기존의 펼침 규칙(RETURNS, derives, READS 첨자)은 모두 정의 하나인 지역 변수만 펼친다(`docs/static_facts.md` 해당 절). 수정 단계의 처방("지역 변수를 정의로 펼침")은 그 제한을 말하지 않아 그대로는 (a)를 may로 남긴다(§11 #4).
  - (b) 종료 연산의 F8 유도가 CF의 길에 닿지 않는다 [M][F]. `CFE_ES_ExitApp`의 종료 호출(`ResetCFE` L399, `OS_TaskExit` L424)은 모두 `AppRecPtr->Type == CFE_ES_AppType_CORE`(L381) 아래이고 각각 `ExitStatus` 8·9일 때만이다. 외부 앱 가지는 `while (1) { OS_TaskDelay(500); }`(L453)다. **이것은 도구의 부족만이 아니다**: ED §3.0의 F8 예 "`CFE_ES_ExitApp → … → OS_TaskExit → pthread_exit`(종료, 외부 앱 분기에서만 may)"가 원문과 어긋난다. `OS_TaskExit`는 CORE 가지에 있다. T2의 조건 "종료 연산은 F8로 P0에서 유도"는 그 예에 기대므로, 설계대로의 F8 유도로는 채워질 수 없다. 다음 ED 개정에서 그 예를 고친다(§8 항목 2, §11 #5).
- 더해야 할 입력: 사례 입력이 아니다. (a)는 계층 B/R의 사실 확장(흐름 민감한 도달 정의; 기존 섹션을 바꾸지 않으려면 새 섹션). (b)는 "탈출 없는 루프에서 멈춘 스레드를 다른 행위자가 끝낸다"는 행위자 사이 종료의 유도(P0의 다른 스레드 취소 묶음과 그 경로 규칙)와 ED 예의 정정이다.
- 덤으로 생긴 것 [M]: CF·BP 조회는 이름 공간 `OS_filesys_internal_record_t.virtual_mountpt`에도 행이 있다(A48 rel 940, A40 rel 844; dependency "no creator of this partition"). 사례 관계로 세지 않는다(§6).

### T3 C5 — **PARTIAL** (수정 단계 PASS에서 내림)

**통과 조건**: spawn `CFE_ES_AppCreate:614`의 잠금 집합 ∅; 등록 쓰기 L649–L652의 잠금 집합 {ES 잠금}(F5 must 정체성 `CFE_ES_Global_t.SharedDataMutex`); 자식 `CFE_ES_GetAppID`의 `RecordUsed` 검사(F1)와 `CFE_ES_ERR_APPID` 반환(F2); "반대 순서 가능(잠금 창)". 6.4.2에서 읽기 지점마다: L800과 잠금 뒤 `GetAppIDInternal` 호출(L445, L551, L667, L1020, L1284) "불가 증명", `LockSharedData`·`UnlockSharedData` 오류 분기(L1858, L1892) "가능" 또는 "결정 불가". must 정체성이 없는 잠금 쌍으로 "불가"가 나오면 실패.

**나온 것**
- 6.4.1 (c5_641)
  - spawn(rel 2986): entries 다섯 스크립트 앱, `locks held {}`; 등록 쓰기(rel 2987) L649 {#9} `acquired-at [cfe_es_apps.c:637:15]`.
  - 행 C31(rel 2985) `CFE_ES_GetAppID` L861 읽기: **`possible: lock window: #9 … (static-instance) is not held at the spawn; the writes after it hold it from a later acquisition`**(rel 2992); 뒤집힘 static-possible(`CFE_ES_ERR_APPID`가 이전 값 0과, 쓴 값 1이 아님). layers에 `rests-on(...)`이 붙는다(개정 3: 정적 인스턴스 판정을 이름 붙인 등급 모두, §11 #16).
  - `GetAppIDInternal` L1886의 호출 지점별: L445·L551 possible(잠금 창), **L700·L1081·L1345 `not graded`**(rel 3028, 3039, 3050: 자식이 그 호출을 실행하지 않음), L1919·L1953 undecidable(failure path).
- 6.4.2 (c5_642)
  - spawn(rel 2940): `locks held {#9 …} acquired-at [cfe_es_apps.c:571:12]`; 등록 쓰기(rel 2941) L652 {#9}, 같은 획득 L571.
  - C30(rel 2939) `CFE_ES_GetAppID` L800 → **`proven-impossible`**(rel 2946). 이유 문장: 잠금 #9가 spawn과 `CFE_ES_AppCreate`의 이 호출에서 spawn 뒤 모든 `RecordUsed` 쓰기에 걸쳐 같은 획득으로 잡히고 그 획득이 spawn 뒤에 다시 닿지 않으며 읽기가 #9를 잡는다. **범위**: `CFE_ES_AppCreate`의 한 호출(ED §3.3 항목 3), 세는 쓰기는 형 있는 쓰기·레코드 전체 쓰기·덮는 데이터 함수·spawn 뒤 호출하는 함수의 쓰기, 필드는 필드 단위 규칙으로 complete, 생성자의 호출자 셋(`CFE_ES_ParseFileEntry`, `CFE_ES_ProcessControlRequest`, `CFE_ES_StartAppCmd`)의 제 본문에 그 필드 쓰기 없음, 그 호출자의 callee와 그 위 호출자가 반환 뒤 하는 일은 읽지 않음. 기댄 것: 선언된 가정 셋과 필드 단위 규칙(#9의 정적 인스턴스와 필드의 완전성), 형 단위로는 #9가 정적 인스턴스가 아님. 계층(rel 2949) `… rests-on(…) P0-unverified(dlsym, pthread_cond_timedwait, pthread_cond_wait, pthread_getspecific, pthread_mutex_lock, pthread_mutex_unlock)`.
  - `GetAppIDInternal` L1825를 부르는 호출마다: L445 `CFE_ES_ExitApp`(C32, rel 2960), L551 `CFE_ES_RunLoop`(C33, rel 2971) → **`proven-impossible`**. **L667 `CFE_ES_WaitForStartupSync`(C34, rel 2982), L1020 `CFE_ES_CreateChildTask`(C35, rel 2993), L1284 `CFE_ES_ExitChildTask`(C36, rel 3004) → `not graded`**. L1858 `CFE_ES_LockSharedData`(C37, rel 3015), L1892 `CFE_ES_UnlockSharedData`(C38, rel 3026) → `undecidable`(failure path).
  - 다섯 스크립트 앱(`sample_lib`, `sample_app`, `ci_lab`, `to_lab`, `sch_lab`; basis `startup.entry[0..4]`)의 소스는 `CFE_ES_WaitForStartupSync`, `CFE_ES_CreateChildTask`, `CFE_ES_ExitChildTask`를 부르지 않는다. 그 셋을 부르는 것은 코어 task(ES·SB·EVS·TBL·TIME)와 `cfe_es_perf.c`다 [F: 6.4.2 트리 grep]. 도구의 정적 행위자 집합도 같다 [M].
  - L1284는 다른 spawn의 행으로만 "불가"다: `CFE_ES_CreateChildTask` spawn(L1047, 자식 `CFE_ES_PerfLogDump`·`CFE_TIME_Local1HzTask`·`CFE_TIME_Tone1HzTask`)의 C16(rel 2788) `proven-impossible`. L667·L1020은 어느 spawn의 자식 문맥도 아니다(코어 task를 만드는 `CFE_ES_CreateObjects`의 spawn은 union 멤버로 entry를 읽어 풀리지 않는다, §8 항목 11).
  - 6.4.2의 AppRecord 행 C19(rel 2821)·C20(rel 2831)은 수정 단계의 `proven-impossible`에서 **`undecidable`**이 되었다: `CFE_ES_AppRecord_t.RecordUsed`는 FIELD_STATUS 필드 단위 DISABLED다(cast 10, other fields 2) [M](§11 #14).
- "불가"는 다섯 빌드를 통틀어 C 행에서만 나오고(0 / 0 / 0 / 6 / 14), 모두 정적 인스턴스 #9다. 6.4.1의 6은 `CFE_ES_CreateChildTask` spawn의 행이다(그 함수는 6.4.1에서도 spawn에 걸쳐 #9를 잡는다). 수정 단계의 14 / 30 가운데 8 / 14는 자식이 닿지 않는 호출 문맥이었고(이제 `not graded`), 6.4.2의 2는 C19·C20이다 [M](`summary.txt` fix1 대비 절). 정적 인스턴스가 아닌 잠금으로는 "불가"가 하나도 없다 [M].

**판정**: PARTIAL. 6.4.1의 조건(spawn ∅, 등록 쓰기 {#9}, 읽기 검사와 반환, 잠금 창)과 6.4.2의 L800·L445·L551·L1858·L1892는 조건대로 나왔다. 미달은 6.4.2의 L667·L1020·L1284다. 그 셋은 `not graded`다. 이것은 개정 3의 건전성 수정의 결과다: 수정 단계는 자식(스크립트 앱)이 실행하지 않는 호출 문맥을 그 호출 지점의 잠금으로 "불가"라 했다(§11 #9). ED의 조건은 그 셋을 이 spawn 자식의 읽기 지점으로 적었으나, 이 빌드의 자식은 그 함수들을 부르지 않는다 [F]. 그래서 이 조건은 문자 그대로는 건전한 규칙으로 채워질 수 없고, ED 조건의 전제를 다음 개정에서 고쳐야 한다(§8 항목 16).
- 정적 인스턴스 #9의 판정과 필드의 완전성은 S4의 필드 단위 규칙과 선언된 가정 셋에 기댄다. 6.4.1의 "잠금 창" 행도 그 정체성을 이름 붙이므로 같은 것을 `rests-on`으로 적는다. 다만 `possible` 등급 자체는 형 단위 규칙으로도 그대로다(잠금이 정적 인스턴스가 아니어도 창은 창이다). 형 단위 규칙을 고르면 6.4.2의 "불가"는 개정 3의 분기 규칙에서 `undecidable`(must 정체성 미증명)이 된다. 그래서 그 규칙에서의 대비는 6.4.1 `possible` 대 6.4.2 `undecidable`이다 [I: 코드의 분기; fixture `relsound.c` `ptr_*`에서 같은 분기를 확인 M]. 수정 단계의 "형 단위로는 '결정 불가'가 된다"는 그 코드에서는 틀렸다(정적이 아닌 공통 잠금이 `possible`로 갔다, §11 #12, #15).

### T4 C5 수명 — **PASS(질의 수준)**

**통과 조건**: `OS_task_table.id`의 writer에 `pthread_create`(P0) 포함; 진입 함수가 그냥 반환하는 경로에 해제 없음; 해제 쓰기는 `OS_TaskDelete`·`OS_TaskExit`와 `OS_TaskCreate` 실패 경로(L572, L652, L667, L679), 초기화는 `OS_API_Init`(L279)로 따로 분류; `OS_TaskRegister` 검색에 활성 검사 없음. #72 관계와 다른 행.

**나온 것** (c5_641; c5_642는 같은 줄 번호에 같은 내용)
- 이름 공간 `OS_task_record_t.id`(rel 1300): 검색 `OS_TaskRegister`의 루프 `osapi.c:921:5`, 키 `id ==`(L923, 지역 `pthread_id`와 비교, rel 1302); `OS_FindCreator`(L3786)의 `pthread_equal`(rel 1301). 활성 검사(rel 1303) 없음, `free == 0` 사용 중·`free == 1` 빈 칸(rel 1304–1305).
- writer(rel 1320): `writers 6 from WRITERS (4 of them … covers-more-than-the-field)`.
- P0 writer(rel 1327): `pthread_create (P0 thread-create) stores through arg 0 [&(OS_task_table[possible_taskid].id)] at osapi.c:645:19 in OS_TaskCreate (joined here from ARGFLOW and P0_TABLE: the extraction's writer sets -- WRITERS, FACT_LOCS, FIELD_STATUS -- have no row for this store)`.
- 진입 경로(rel 1328): `… no function of this program runs after that entry returns (thread-specific keys created with a null destructor only: pthread_key_create at osapi.c:340:10 (arg 1 [NULL]); no cleanup handler push in the program), so the table has no write on that path (not read in this revision: handlers registered for process exit (atexit, at_quick_exit, on_exit), destructor functions, and units built with exceptions, …)`. 6.4.1 트리(단위 시험 제외)에 `atexit`·`at_quick_exit`·`on_exit`·`__attribute__((destructor))`가 없고 compile DB에 `-fexceptions`가 없다 [F: grep].
- `free` 쓰기 분류(rel 1306–1319): L279 initialization; L551·L697 reservation; L572·L652·L667·L679·L761·L793 release. 블록 계층(rel 1331) `B P0 R basis-deviation(…osapi.c…)`.

**판정**: 조건이 문자 그대로 모두 나왔으므로 PASS이되 **질의 수준**으로 적는다. (1) "writer에 `pthread_create` 포함"은 relations의 질의 시점 결합이다. ED §3.0 G10은 P0 요약을 추출에 넣어 `id`가 escape(DISABLED)를 벗어나게 하라고 했으나, facts의 FIELD_STATUS·FACT_LOCS에서 `id`는 여전히 DISABLED다. 빠진 것은 추출 쪽 G10 writer다(§8 항목 10, §11 #13). (2) 진입 경로 문장은 읽지 않는 조건을 이제 이름으로 적는다(§11 #23). (3) `OS_TaskRegister`의 비교 상대가 `pthread_self()`라는 것은 지역 변수 정의 사실이 없어 행에 나오지 않는다(T2 (a)와 같은 공백). 6.4.1 원문 대조: `pthread_create(&OS_task_table[possible_taskid].id, …)` L645, `pthread_key_create(&thread_key, NULL)` L340, `OS_TaskRegister` L921–923 [F](검증자 확인과 같음).

### T5 입력 계층 — **PARTIAL**

**통과 조건**: 위 행마다 입력 계층이 B/P0/L0/R만이다. 사례 계층 0, G0 0. P0·L0 항목마다 판과 원문 줄.

**나온 것** (`summary.txt` T5 절)
- 행 수와 계층: c1_pre 393, c1_post 393, r2 299, c5_641 178, c5_642 175, 합 1,438행. 계층은 `B P0 R`, `B P0 L0 R`와 그 둘에 `(writers: rule targets)`가 붙은 것뿐이다. `case:`·`hand-entered:` 표시가 붙은 행 0.
- **`P0-unverified(...)`**(개정 3, §11 #18): 272 / 272 / 138 / 149 / 147행. 이름 붙은 항목(행 수, C1 pre / R2 / C5 6.4.2): `dlsym` 188 / 0 / 85(스크립트 entry), `strncmp` 99 / 55 / 15, `strcmp` 25 / 10 / 16(검색 비교), `abort` 35 / 22 / 0, `pthread_exit` 6 / 0 / 14, `exit` 2 / 1 / 3(F8 종료 사슬), `pthread_mutex_lock`·`unlock`·`timedlock`, `pthread_cond_wait`·`timedwait` 14 / 71 / 16(잠금을 이름 붙인 행), `clock_nanosleep` 1 / 56 / 0(F8 지연), `pthread_getspecific` 0 / 0 / 32(자기 정체성), `pthread_equal` 2 / 2 / 0. 원문 대조된 `pthread_create`만 쓰는 행에는 표시가 없다.
- `basis-deviation(…)` 0 / 0 / 0 / 159 / 156(이름 공간 블록 0 / 0 / 0 / 8 / 8). `rests-on(…)` 0 / 0 / 55 / 17 / 16(R2: B 행의 await 인정; C5: 정적 인스턴스 #9를 이름 붙인 C 행 — 불가 6·14, 잠금 창 11, 완전성·호출자 검사로 내려간 C19·C20).
- 머리: P0 표 개정 3의 판과 scope, P0 비교 표, basis 편차, `P0-unverified`의 뜻과 `pthread_create` 원문 대조의 범위(upstream glibc 2.39 사본; 빌드가 링크하는 배포판 패키지 `2.39-0ubuntu8.9`의 패치는 대조 안 함 [U]).
- P0 표(facts `[p0]`): 72항목 모두 판 표기. 원문 대조 줄은 `pthread_create` 1개, 71개는 `source unverified`. P0 비교 표 13항목도 모두 unverified.

**판정**: 사례 계층과 G0이 0이라는 핵심 조건은 통과다. 미달은 둘이다.
- "P0 항목마다 원문 줄" 조건은 84개(71 + 비교 13)가 unverified이고, 행의 다수가 그 항목에 기댄다(`P0-unverified`). PI A4가 정해지지 않았으므로 그 기본값("미대조 P0 항목 n개 의존"을 모든 결과에 표시하고, T5는 그 의존이 0일 때만 통과)을 따른다: 표시는 이제 행마다 있고, 의존이 0이 아니어서 통과로 세지 않는다. 대조된 한 항목도 upstream 사본에 대한 것이고 배포판 패치는 [U]다.
- 사람이 넣은 것과 결정이 대기 중인 것에 기대는 행: C5 행의 거의 전부(159 / 178, 156 / 175)가 PI D7의 `front_end_diagnostic` 편차에, R2의 await 55행과 C5의 #9 행 17·16이 선언된 가정과 필드 단위 규칙(PI 결정 대기)에 기댄다.

### T6 정상 대조 — **PASS**

**통과 조건**: C5 OSAL `*GetIdByName`·`*Create` 6쌍(task, queue, binsem, countsem, mutsem, timer)과 C1·R2의 `OS_ObjectIdFindNextMatch` 콜백 3종이 A로 인식된다. 정상 호출자는 신호만 내고 위반 확정은 내지 않는다.

**나온 것**
- C5(두 판 같음): `OS_task_record_t.name`(rel 1332) lookups `OS_TaskGetIdByName#1` / creators `CFE_ES_AppCreate#3 CFE_ES_CreateChildTask#1 OS_TaskCreate#1`; `OS_queue_record_t.name`(rel 1276); `OS_bin_sem_record_t.name`(rel 1184), `OS_count_sem_record_t.name`(rel 1205), `OS_mut_sem_record_t.name`(rel 1250), `OS_timer_record_t.name`(rel 1364) 각 `GetIdByName#1` / `Create#1`.
- C1(두 판)·R2: A2 콜백 검색 3종. 키 `OS_common_record_t.name_entry`(`strcmp`, C1 rel 334 / R2 rel 95), `OS_filesys_internal_record_t.virtual_mountpt`(`strncmp`, rel 760 / 352), `OS_impl_task_internal_record_t.id`(`pthread_equal`, rel 1886 / 1044).
- 위반 근거 칸: 다섯 빌드 1,438행 모두 `unconfirmed`.

**판정**: 조건대로 나왔다. 다만 "정상 호출자는 위반 확정을 내지 않는다"는 기록을 주지 않았기 때문에 자명하게 성립한다(§8 항목 1).

### T7 결정성 — **PASS**

`T/twice.txt`: 같은 basis에서 extract(`--targets-rule R-T1,R-T2`)와 relations를 다시 돌렸다. facts가 다섯 빌드 모두 같다(`8e189863…`, `9f3a0e20…`, `e7c4fa16…`, `26cd3ee3…`, `ceec3748…`). relations도 같다(`a40cdd11…`, `13b45c6f…`, `2e0eedf7…`, `4c3ca8fe…`, `6e91fe36…`). fixture golden은 relations를 두 번 돌려 같은 바이트를 요구한다(`relsound_np` 포함).

## 5. 반증 조건 (ED §6, RV2 §11 단계 2, §12)

- **조건**: T1–T3 가운데 하나라도 사례 입력을 더하지 않으면 관계가 나오지 않으면, 가설("사례별 Φ 없이 근거 있는 관계를 낸다")은 아직 성립하지 않은 것으로 보고한다.
- **이번 결과** [M]: T1–T3은 사례 입력(프로필, 명령줄 어휘, `--target`, G0, rspec·`.kzm`·manifest) 없이 돌렸고, 세 사례의 관계 행이 나왔다(T1 B65, T2 A20·A21, T3 C30–C38). **ED §6의 반증 조건은 충족되지 않았다.** 사례 입력을 더해야 했던 시험은 없다. 이 결론은 아래 A2의 읽기와 따로 둔다: ED §6의 뜻에서 반증되지 않았다는 것과 탐지 근거라는 것은 다르다.
- **PI A2 사전 등록으로 읽기** [I] (`plan/PI_DECISIONS_PENDING_20261030.md` A2 ④⑤, A5 표; A2는 결정 대기이고, 결정되지 않은 동안 이 문서는 그 권고안을 기준으로 표시만 한다)
  - **④ "미확인·미해결 표시로 통과"는 "관계를 낸다"의 통과**다. T1의 순서 조건은 "고정 미확인"으로 채워졌다. T2의 사례 행 A21은 구획만 같은 짝(`key unresolved`)이고, CF 조회는 SCH_LAB과도 똑같이 짝을 짓는다(rel 423 `key unresolved (lookup)`); 실현 가능성 `undecidable`, 뒤집힘 `static-undetermined`다. 그래서 T1·T2는 순서 위반 탐지의 근거로 세지 않는다.
  - **⑤ 시험이 계기가 된 개정 뒤의 통과는 "개정 n 뒤 통과"**다.
    - T1: F2의 돌려주는 지역 변수 도달 정의 규칙은 T1 재료를 보고 넣었다(A5 표, S2:59). T1은 **그 규칙 개정 뒤 통과**다.
    - 두 수정 단계의 개정 자체가 T1–T3 행을 계기로 했다: 수정 단계(relations 개정 2) 지적 2(T2 종료 사슬), 3(T3 C19·C20), 5(C1 B73 행위자); 둘째 수정(relations 개정 3) 지적 1(C1 B73), 3(T1 등급 문장), 6(T2 probe-create), 9–15(T3 C 행). 그래서 T1(PASS)은 "relations 개정 2·3 뒤의 통과"로, T2·T3(PARTIAL)의 채워진 부분도 같은 개정 뒤의 것으로 적는다. 그 사례들에 대해서는 "사례 입력 없이"의 근거로 세지 않는다. 그 근거는 동결 평가나 새 호출자 평가에서만 나온다.
  - 그래서 "사례별 Φ 없이 근거 있는 관계를 낸다"는 이 다섯 개발 사례에서 **관계를 냈다**는 데까지만 말한다.
- **그래도 "성립"이라고 적지 않는 이유** [I]
  1. T2의 종료 도달이 미달이다. 흐름 민감한 도달 정의 사실이 없고(may), 종료 연산의 F8 유도는 CF의 길에서 P0 종료에 닿지 않는다(guard가 모두 거짓, 남는 길은 탈출 없는 루프). ED의 F8 예 자체가 원문과 어긋난다(§4 T2 (b)). 사례 입력으로 메울 것은 아니다.
  2. T3의 6.4.2 읽기 지점 셋(L667, L1020, L1284)은 자식이 실행하지 않는 문맥이어서 등급이 없다. ED가 그 셋을 이 spawn 자식의 읽기 지점으로 적은 전제가 원문과 어긋난다(§4 T3).
  3. "근거 있는"의 무게가 고르지 않다. T1의 순서는 "고정 미확인"이고, C1의 반대 순서 가능성도 "결정 불가"다. 위반 근거는 실행 기록이 있어야 생긴다(RV2 §5.4). T3의 "불가 증명"은 선언된 가정 셋과 필드 단위 규칙(PI 결정 대기)에 기대고, 범위는 생성자의 한 호출이다. 6.4.1의 "잠금 창"(T3의 반대 순서 가능 조건)도 #9를 정적 인스턴스로 이름 붙이므로 같은 규칙과 가정에 기댄다(`rests-on`; `possible` 등급 자체는 형 단위 규칙에서도 같다, §11 #16).
  4. 남은 수작업 입력이 있다. L0(recipe의 시작 스크립트 블록), C5 `front_end_diagnostic` 2줄(PI D7; C5 행의 대부분이 기댐), unverified P0 원문 84개(행 다수가 `P0-unverified`로 기댐), P0·비교 표 자체가 그렇다.
  5. 사례 관계는 행 175–393개 가운데 1–8개다(§6). 질의는 사례 관계를 골라내지 않는다. 고르는 것은 이 문서의 평가용 조회다.
  6. 다섯 사례는 개발 집합이다(ED §5 R7). 동결 뒤 고른 구성으로 따로 평가하지 않았다.

## 6. 후보 부담 [M]

`T/count/burden.tsv`. "사례 관계 행"은 평가용 조회로 고른 행이다. C1은 짝이 있는 이름 공간 `OS_common_record_t.name_entry`의 CF `:993`(수정 후 `:1003`) 조회 A21과 BP `:126` 조회 A20, 2행이다. 같은 조회가 콜백 이름 공간 `OS_filesys_internal_record_t.virtual_mountpt`에 낸 A40·A48은 짝이 없고 필드 기반 may의 부산물이어서 세지 않는다(S5 문서는 이것까지 4행으로 셌다: §10 지적 12). R2는 T1의 B65 1행, C5는 `CFE_ES_AppCreate` spawn × `RecordUsed` × `GetAppID`/`GetAppIDInternal` 읽기 지점 8행이다.

| 빌드 | A(조회 사용) | B | C | 합 | 이름 공간 블록 | 사례 관계 행(비율) | flip static-possible | feasibility proven-impossible | not graded |
|---|---|---|---|---|---|---|---|---|---|
| c1_pre_tx | 127 | 170 | 96 | 393 | 13 | 2 (0.5%) | 2 | 0 | 20 |
| c1_post_tx | 127 | 170 | 96 | 393 | 13 | 2 (0.5%) | 2 | 0 | 20 |
| r2_core | 68 | 155 | 76 | 299 | 8 | 1 (0.3%) | 1 | 0 | 5 |
| c5_641 | 66 | 73 | 39 | 178 | 31 | 8 (4.5%) | 16 | 6 | 14 |
| c5_642 | 66 | 71 | 38 | 175 | 31 | 8 (4.6%) | 16 | 14 | 14 |

- **수정 단계 대비 변화** (둘째 수정, `T/count/against_fix1.tsv`, 행 머리로 비교) [M]
  - A·B: 행 수와 등급이 다섯 빌드 모두 그대로다(A 127 / 68 / 66, B 170 / 155 / 73·71). C1 B 행의 행위자 칸은 트램펄린 root가 늘어 바뀌었고(읽는 쪽에 코어 task main·스크립트 앱), B73의 실현 가능성 문장이 정적 이미지 문장으로 바뀌었다(§4 T1 참고). A 행에는 probe-create 극성, handling 줄, dependency 문장, `P0-unverified`가 더해졌다.
  - C: C1 95 → 96(머리 문장이 바뀐 행 6과 새 7: `StartAppTask:588` spawn의 자식이 트램펄린을 통해 15 root가 되어 "자식이 닿지 않음" 표시가 바뀌고 문맥이 하나 늚), R2 76 → 76(R2 `linux_sysmon` spawn의 읽기가 entry 문맥이 됨), C5 그대로. 등급이 바뀐 C 행 20 / 20 / 5 / 14 / 16: 대부분 `possible`·`proven-impossible` → `not graded`(자식이 닿지 않는 호출 문맥), 6.4.2의 C19·C20은 `proven-impossible` → `undecidable`(필드가 필드 단위 DISABLED).
  - 등급 수: proven-impossible 0 / 0 / 0 / 14 / 30 → 0 / 0 / 0 / 6 / 14; possible 95 / 95 / 76 / 17 / 0 → 76 / 76 / 71 / 11 / 0; undecidable 297 / 297 / 223 / 147 / 145 → 297 / 297 / 223 / 147 / 147; not graded 0 → 20 / 20 / 5 / 14 / 14.
  - 다섯 빌드에서는 C의 다른 새 조건(레코드 전체 쓰기, 덮는 쓰기, 다시 잡힌 잠금, 정적 인스턴스 아닌 공통 잠금, 풀리지 않은 호출, 생성자 호출자의 제 본문 쓰기)이 등급을 바꾼 행이 없다: 바뀐 행은 모두 문맥(자식이 닿지 않음)과 필드의 완전성 때문이다 [M]. 그 조건들이 등급을 바꾸는 경우는 fixture `relsound.c`가 보인다(§11).

- S5 출력(`T_S5/`)에 같은 계수를 고쳐 적용하면 사례 관계 행은 2 / 2 / 1 / 8 / 8, 비율 0.7% / 0.7% / 0.4% / 4.4% / 4.5%(행 286 / 286 / 248 / 180 / 177)다 [M].
- **S5 대비 변화(수정 단계의 기록; 수는 수정 단계 출력)** (`T_F1/count/against_s5.tsv`, 행 머리로 비교) [M]
  - C1 A: 127행 그대로, 뒤집힘 등급이 바뀐 행 10(코어 task main이 행위자가 되어 `CFE_TIME_TaskInit`의 조회 하나가 다른 행위자의 같은 키 생성자와 짝이 되고(static-possible), 9행은 key 미해결 생성자와 짝이 됨(static-undetermined)). 짝 있는 조회 26 → 36, 키까지 같은 것 1 → 2.
  - C1 B: 135 → 170(+35, 코어 task main이 행위자가 되어 inter-actor가 됨). 등급이 바뀐 행 0.
  - C 행: C1 24 → 95(빠짐 2, 새로 73), R2 25 → 76(빠짐 2, 새로 53), C5 41 → 39·40 → 38(빠짐 2씩). 새 행은 모두 `CFE_ES_CreateObjects`의 table walk spawn이다: 루프 본문의 쓰기가 되돌아가는 간선으로 다음 반복의 spawn 뒤가 된다(C1은 그 spawn의 entry가 이제 풀림). 빠진 행은 다섯 빌드 모두 P0 스레드 생성의 out 인자(C1·R2 `OS_impl_task_internal_record_t.id`: `OS_TaskCreate_Impl`이 `&impl->id`를 `OS_Posix_InternalTaskCreate_Impl`에 넘기고, 그 함수가 매개변수를 그대로 `pthread_create`의 인자 0에 넘긴다(`os-impl-tasks.c:586`, `:543` [F]); C5 `OS_task_record_t.id`: `pthread_create(&OS_task_table[…].id, …)`)를 소스 위치 때문에 "뒤"로 셌던 것이다. P0 의미로 그 저장은 새 스레드가 돌기 전이다. 본문 있는 유도 spawn 연산 `CFE_ES_StartAppTask`가 인자 0으로 저장하는 `CFE_ES_AppRecord_t.MainTaskId`는 spawn 뒤 쓰기(잠금 모름)로 남는다: C1 `CFE_ES_AppCreate:768` spawn의 행 C15–C19가 그것이고, R2의 table walk 행 넷이 같은 줄을 단다 [M]. 등급이 바뀐 C 행은 6.4.2의 C19·C20 둘(possible → proven-impossible)뿐이었다(둘째 수정에서 `undecidable`).
  - "불가": 0 / 0 / 0 / 14 / 28 → 0 / 0 / 0 / 14 / 30, 모두 #9(둘째 수정에서 0 / 0 / 0 / 6 / 14).
- 경로별 남는 행의 거름(머리 줄): B는 필드–인자 묶음 가운데 writer 없음과 한 행위자뿐을 뺀 것이다. B 가운데 callee 검사가 있는 행은 40 / 40 / 29 / 26 / 24다. A는 이름 공간 13 / 13 / 8 / 31 / 31개의 조회 사용이고, 같은 구획의 다른 행위자 생성자가 있는 조회는 36 / 36 / 20 / 2 / 2, 키까지 같은 것은 2 / 2 / 1 / 0 / 0이다. C에서 생성자 본문에는 없고 spawn 뒤 callee에서만 쓰는 필드(짝짓지 않음)는 둘째 수정에서 1,588 / 1,588 / 592 / 404 / 404다(수정 단계 1,964 / 765 / 506; 이제 그 수는 짝짓는 필드와 같은 spawn의 집합 안에서 세고, callee 쪽 레코드 전체 쓰기는 따로 센다). 생성자가 spawn 뒤 레코드 전체만 쓰고 그 형의 필드는 형 있게 쓰지 않는 레코드 형(짝짓지 않음)은 380 / 380 / 177 / 102 / 102다.
- 뒤집힘 신호 static-possible은 2 / 2 / 1 / 16 / 16행이다. C1의 2행과 R2의 1행은 같은 키가 정적으로 맞는 조회–생성 짝이다(C1은 `OS_TimeBaseGetIdByName`의 SCH_LAB·TIME 조회). 사례 관계(T1, T2)는 static-undetermined에 들어 있다. 신호만으로 고르면 사례 관계를 놓친다.
- 넘치는 원인 [I]: (a) 필드 기반 may 간접 호출(T2 덤 참고: 한 조회가 여러 콜백 이름 공간에 들어간다), (b) 트램펄린의 코드를 그것이 대신하는 모든 root에 귀속하는 것(그 코드의 행위자 집합이 넓어진다. 반대로 root 집합 자체는 유도 spawn 연산 호출의 entry를 풀지 못하면 빠지는 과소 근사다: 수정 단계 문서는 이것을 과대 근사라고만 적었다, §8 항목 14, §11 #2), (c) B의 inter-actor 기준이 `main`(process)을 writer 행위자로 세어 초기화 쓰기와 task 읽기를 모두 짝으로 만든다, (d) C의 "spawn 뒤"가 분기를 자르지 않는 CFG 도달이라 table walk 루프 본문의 모든 쓰기가 들어간다(과대 근사; "불가"를 줄이는 쪽).
- 시간(이 VM, 한 번, 비용 주장 아님): NP extract(`--targets-rule`) 36.0 / 39.4 / 26.4 / 9.9 / 10.0 s(수정 단계 31.9 / 30.3 / 20.4 / 8.4 / 8.3 s), relations 1.5 / 1.4 / 0.8 / 0.6 / 0.7 s(`T/run.log`).

## 7. 조건 기록 (`CONDITIONS_POLICY.md`)

| 조건 | 값 | 상태 | 근거·이유 |
|---|---|---|---|
| 분석 대상 빌드 | S1–S4와 같은 기준 빌드 5 | from-reference | ED §1.1, §1.4 |
| 대상 규칙 | R-T1 ∪ R-T2, 공유 base만 | 도구 규칙(연구 조건 아님), "target rules revision 1" | ED §6 항목 2. 호출 결과 base를 전역 주소만 돌려주는 함수로 넓힌 것은 ED의 "F4 원천이 전역인 포인터"를 이 판에서 읽은 방식이다 [D] |
| relations 규칙 | §1.2 | 도구 규칙, "relations rules revision 3" | ED §3.1–§3.4, §6 항목 3; 수정 단계(§10), 둘째 수정(§11) |
| P0 표 | 72항목, 개정 3, 담지 않는 묶음·구성원은 `extract.p0_table_scope` | 도구 내부 지원(P0 계층). 판 표기 항목마다, 원문 대조 1개(upstream 사본; 배포판 패치는 [U]) | 읽기-쓰기 잠금 묶음을 통째로 뺀 것은 개정 2의 범위 결정이다 [D](§1.3). 개정 3은 문장만 고쳤다 |
| P0 비교 표 | 13항목, 개정 2 | 도구 내부 지원(P0 계층). 원문 대조 unverified | 추출은 쓰지 않는다 |
| 수치 도구 매개변수 | 둘째 수정에서 모두 적음(아래) | unspecified_by_reference | 수정 단계 문서의 "새것 없음"은 틀렸다(§11 #20): 2a에서 더한 상한 셋이 표에 없었다. 둘째 수정의 처리: (1) PARAM_FLOW 사슬 문장의 반복 상한 100000(`argflow.cpp`, `129ace3`)과 텍스트 보기의 SEQ 걷기 상한 10000 둘(`facts_text.cpp`, `6025388`)은 방문 집합·후위 번호 조건으로 바꿨다(출력 변화 없음, golden 그대로). (2) 텍스트 보기가 PTR_FIELDS 원소의 정적 값을 64 원소까지만 적는 것(`facts_text.cpp`)은 보기의 출력 한계이고 facts에는 모든 행이 있다; 넘으면 그 줄에 적는다. (3) 2a 전부터 있던 둘: FACT_INIT 초기화자 걷기 4096 노드(`ast_tu.cpp`; 멈추면 이제 "상수 아님" 행을 더함, 전에는 잎 없이 멈추면 0)와 함수 값 초기화자 중첩 6 단계(`ast_fn.cpp`). 두 값은 BASIS `param.walk_bounds`(unspecified_by_reference), 걸린 수는 `count.walk_bounds_hit`에 확장 사실과 함께 남는다(빌드별 수는 §11 #20). 그 밖: 대상 규칙(R-T2 조건 걷기), relations(식 트리 걷기, 행위자, LOCK_PATHS 읽기), LOCK_PATHS 유도에 수치 상한이 없다. 식 텍스트의 40단계 출력 한계(`show`와 같음)는 출력에만 쓴다; 이 한계로 잘린 식은 다섯 빌드 0개 [M]. 기존 F4 상한(`--argflow-depth`·`--argflow-size`)은 기본값 none, `--image-budget` 4096(S3) 그대로 |
| 선언된 가정 | S4의 셋 | S4 보고 §4와 같음. `assume.lock_acquisition_succeeds`의 문장은 P0 획득 기본 연산이 spin lock·`mtx_lock`까지로 넓어진 만큼 고쳤다 | T3의 "불가"가 기대고, 행이 적는다 |
| 완전성 규칙의 선택 | relations와 AWAIT 인정은 필드 단위 규칙을 쓴다 | 결정 대기(S4 열린 문제 1) | 형 단위를 고르면 T1 행의 await 문장이 빠지지만 T1의 순서 조건은 "고정 미확인"(생성 순서만)으로 그대로 채워진다: 그 조건은 대체 표시로 문자 그대로 충족된다(§11 #3). T3에서는 #9가 정적 인스턴스가 아니게 되어 6.4.2의 "불가"가 `undecidable`(must 정체성 미증명)이 되고 6.4.1의 "잠금 창"은 `possible` 그대로다 [I: 개정 3의 분기 규칙에서]. 행이 `rests-on(per-field completeness rule (PI decision pending))`으로 적는다 |
| 실행 기록 | 없음 | 이번 시험의 조건 | ED §6: 사례 입력 없이. run-order 칸은 늘 `no record given` |
| L0, C5 `front_end_diagnostic` | S1과 같음 | S1 보고 §4와 같음 | 추출이 basis의 편차 항목을 `extract.basis_deviation[…]`로 facts에 복사하고, 행이 `basis-deviation(…)`으로 적는다 |

## 8. 열린 문제

1. **실행 기록 입력.** run-order 칸을 채우는 `--record`는 이 판에 없다. 기록의 neutral 사이트(파일:줄)를 행의 쓰기·사용 위치에 맞추는 일이 남았다. 그 전까지 위반 근거 칸은 늘 unconfirmed다.
2. **지역 변수 정의의 사실과 행위자 사이 종료.** WRITERS·TERMINALS·READS ctx의 guard에 남는 지역 변수(`status = CF_Init()`, `pthread_id = pthread_self()`)를 정의로 펼치는 사실이 없다(T2 (a), T4 (2)). 필요한 것은 **흐름 민감한 도달 정의**다(그 guard 지점에 닿는 정의; 정의 하나 제한 없음): `CF_AppMain`의 `status`는 정의가 둘(`cf_app.c:330`, `:342`), 수정 후 재시도의 `ret`은 다섯이다(§11 #4). T2는 그 위에, 탈출 없는 루프에서 멈춘 스레드를 다른 행위자가 끝내는 것을 유도해야 한다(T2 (b): P0 다른 스레드의 취소 묶음과 그 경로 규칙). 그리고 **ED §3.0 F8 예의 정정**: "`CFE_ES_ExitApp → … → OS_TaskExit → pthread_exit`(종료, 외부 앱 분기에서만 may)"는 원문과 어긋난다. `OS_TaskExit`(L424)와 `ResetCFE`(L399)는 CORE 가지(L381)에 있고 외부 앱 가지는 탈출 없는 지연 루프(L453)다. T2의 "종료 연산은 F8로 P0에서 유도" 조건은 그 예에 기대므로 다음 ED 개정에서 고친다(§11 #5). 기존 섹션을 바꾸지 않으려면 새 섹션이 필요하다.
3. **공유 base 판정의 범위.** READS는 지역 포인터의 정의를 따라가지 않는다. 그래서 `AppRecPtr->AppState` 같은 사례 필드가 R-T2에서 빠진다. ARGFLOW의 `base-via-local`과 같은 규칙을 READS에도 두면 넓어진다(새 섹션 또는 relations 쪽 계산).
4. **필드 기반 may의 부담.** A의 간접 호출 콜백 혼입, 트램펄린 귀속, process 행위자를 writer로 세는 B 기준, 분기를 자르지 않는 C의 "spawn 뒤"가 행 수를 키운다(§6). 사슬의 함수 포인터 인자(예: `OS_ObjectNameMatch`를 고정해 넘기는 호출)로 콜백을 좁히는 규칙과, 생성 루프의 반복 사이 순서(같은 반복의 쓰기만 그 반복의 spawn에 묶기)가 다음 후보다. spawn 뒤 callee에서만 쓰는 필드(1,964 / 765 / 506)는 짝짓지 않았다.
5. **상수 접기.** R2의 "초기 값 0은 검사를 통과하지 못함"(ED §3.2)은 본문 있는 함수의 상수 접기가 없어 static-undetermined로 남는다. 값은 C int32가 아니라 캐스트를 벗긴 값으로 출력된다(S2 열린 문제 4: `3254779907`).
6. **C의 "spawn 뒤"와 callee의 잠금.** 수정 단계에서 CFG 도달(LOCK_PATHS)이 되었다. 둘째 수정(개정 3)에서 "불가"의 조건을 좁혔다(§1.2, §11 #9–#14). 남은 것: "불가"의 범위는 생성자의 한 호출이다. 생성자를 부르는 함수의 제 본문만 순서 없이 보고, 그 함수들의 callee와 그 위 호출자가 반환 뒤 하는 일(예: C5 ES task의 명령 루프가 나중에 앱을 지우며 `CleanupTaskResources`로 `RecordUsed`를 쓰는 일, 다음 `CFE_ES_AppCreate` 호출의 쓰기)은 읽지 않는다. 그것을 읽으려면 호출 뒤 사건의 LOCK_PATHS(새 종류)가 필요하고, 필드 기반 추상에서는 그 쓰기가 다른 레코드에 대한 것인지 정하지 못해 "불가"는 거의 남지 않는다 [I]. 형 없는 저장은 필드 단위 완전성 규칙(PI 대기)으로만 배제된다. 생성자의 루프가 다음 반복에서 다시 잡는 잠금은 획득 지점이 spawn 뒤 사건인지로만 본다(그 획득이 실제로 그 쓰기에 이르는 길에 있는지는 보지 않음: 보수). 아래는 수정 단계의 기술이다. S5 문서의 "이번 사례에서는 소스 위치와 CFG 순서가 같다는 것을 원문으로 확인했다 [F]"는 틀렸다: 6.4.2 C19·C20의 L690·L702는 spawn 가지를 배제하는 `else` 가지에 있었다(§10 지적 3). 남은 것: 분기 시험으로 도달을 자르지 않는다(과대 근사). 한 호출보다 아래의 callee 쓰기는 호출 지점의 잠금과 합치지 않는다(보수). 본문 있는 유도 spawn 연산이 인자를 통해 저장하는 것은 그 호출 안에서 스레드 생성과의 순서를 읽지 않고 spawn 뒤, 잠금 모름으로 센다(보수; "불가"를 막는 쪽).
7. **P0 원문 대조.** 71 + 13항목 unverified(T5). glibc 2.39 upstream 사본이 필요하다.
8. **완전성 규칙의 PI 결정**(S4 열린 문제 1)과 **facts의 프로필 해시 결합**(ED §5 R8)은 그대로 대기다. relations는 프로필 facts를 받되 모든 행에 사례 계층을 단다.
9. **구획 비교**는 상수 값 집합의 같음이다. 상수가 둘 이상인 사슬(예: `OS_remove`의 `{1,11}`)에서 다른 의미의 상수가 섞일 수 있다.
10. **escape → writer(ED §3.0 G10).** P0 out 인자(`pthread_create`의 id 저장)는 relations가 질의 시점에 맞춘다. 추출의 WRITERS·FACT_LOCS·FIELD_STATUS·LOCK_IDS에는 반영되지 않는다(FACT_LOCS는 escape로 셈; `OS_task_record_t.id`는 FIELD_STATUS에서 DISABLED). 그래서 T4는 "질의 수준" PASS다. 반영하려면 기존 섹션을 바꾸지 않는 새 섹션이 필요하다(§11 #13).
11. **C5의 코어 task main.** C5 `CFE_ES_CreateObjects`의 spawn(6.4.1 `:875`, 6.4.2 `:855`)은 entry를 union 멤버 `VoidPtr`로 읽고 정적 이미지는 `FunctionPtr`을 초기화한다. 이 판은 union의 다른 멤버로 읽는 것을 풀지 않으므로(머리에 이유를 적음) C5의 코어 task main은 행위자가 아니다. 그 코드는 `CFE_ES_CreateObjects`의 간접 호출(`:961`, `FunctionPtr` 필드 기반)로 `main`에 닿는다 [M]. 그래서 C5의 B·C 행위자 집합에는 코어 task가 없다. union 멤버의 별칭을 사실로 내는 규칙(형 정보가 facts에 필요)이 다음 후보다.
12. **트램펄린 규칙의 쓰기.** 형 있는 레코드 필드 쓰기는 facts에 base가 없어 트램펄린 판정이 보지 않는다(전역 객체 쓰기와 공유 root 읽기만 본다).
13. **읽기-쓰기 잠금.** P0 개정 2가 통째로 뺐다. 공유 획득을 상태에 따로 두는 잠금 집합 분석이 있어야 넣을 수 있다. 지금은 그 획득이 잠금 연산이 아니어서 판정만 약해진다.
14. **트램펄린 root의 과소 근사.** 개정 3은 유도 spawn 연산의 호출 지점에서 푼 entry를 트램펄린의 root에 더한다. 그 호출 지점의 entry를 풀지 못하면(예: R2 `CFE_ES_AppCreate:849`의 `LoadStatus.InitSymbolAddress` entry) 그 스레드는 트램펄린 코드의 행위자에서 빠진다. 머리에 그런 호출의 수와 위치를 적는다: R2 1(`cfe_es_apps.c:849:13`; entry는 적재 모듈의 기호 주소 `LoadStatus.InitSymbolAddress`이고 R2에는 시작 스크립트 entry root가 없다(script-entry 0)), C1·C5 0 [M]. R2에서 그렇게 적재된 앱의 스레드가 트램펄린 코드를 돌리는 것은 행위자 집합에 없다 [I].
15. **정적 이미지 root의 생성 순서.** C1의 코어 task main은 정적 이미지(`CFE_ES_ObjectTable[k]`)에서 풀린다. 이 판은 이미지 첨자를 반복 순서로 읽지 않고, 같은 반복의 await도 찾지 않는다. C1 원문은 R2와 같은 구조(`cfe_es_start.c:716`, `:767`, `:810–811`, `:892`)이므로, 표 걷기 루프의 첨자가 이미지 첨자와 같다는 사실(LOOP_ITERS와 같은 펼침)을 이미지에도 내면 T1의 C1 판 문장이 나올 수 있다 [I](§11 #1).
16. **ED T3 조건의 전제.** ED §6 T3은 6.4.2의 `GetAppIDInternal` 호출 L667·L1020·L1284를 `CFE_ES_AppCreate` spawn 자식의 읽기 지점으로 적었다. 이 빌드의 자식(스크립트 앱 다섯)은 그 함수를 부르지 않는다 [F]. 건전한 규칙은 그 문맥에 등급을 주지 않는다(`not graded`). 다음 ED 개정에서 조건을 "자식이 실행하는 읽기 지점마다"로 고치거나, 그 셋을 부르는 자식이 있는 구성으로 옮긴다.
17. **P0 원문과 배포판 패치.** `pthread_create`의 원문 대조는 upstream glibc 2.39 사본에 대한 것이다. C1·R2 빌드는 `Ubuntu GLIBC 2.39-0ubuntu8.9`를 링크한다 [M]. 배포판 패치는 대조하지 않았다 [U]. P0 표 개정 3의 scope가 이름 붙인 판 밖 묶음 가운데 `pthread_join`·`timer_create`(SIGEV_THREAD)·`select`·`sscanf`는 다섯 빌드가 부른다(§1.3): 그 호출의 효과(스레드를 시작하는 timer, 인자를 통해 저장하는 `sscanf`)는 이 판의 사실에 없다.
18. **걷기 상한.** FACT_INIT 4096 노드와 함수 값 초기화자 중첩 6은 남았다(값과 상태는 BASIS, 걸린 수는 다섯 빌드 0). 레퍼런스가 정하지 않는 값이고, 걸리면 FACT_INIT은 "상수 아님" 행을, CALL_TARGETS는 그 아래의 함수 값을 잃는다(§7).

## 9. 재현

```sh
R=/home/user/Shape-Perf/research/cfs_procrace
S5=/home/user/work/redesign/step2a/tests_s5 FIX1=/home/user/work/redesign/step2a/tests_fix1 \
  $R/env/redesign/step2a_tests.sh all   # identity npsame run s5same fixsame twice count sha -> /home/user/work/redesign/step2a/tests
```

- 각 빌드 `T/<build>/NP/`에 `basis.kz`·`basis.txt`·`facts.kz`·`facts.txt`·`extract.txt`·`relations.txt`·`args.txt`·`sha256.txt`가 있다. `NP2/`는 T7의 두 번째 relations다.
- `T/count/summary.txt`(T1–T7 증거, 줄 참조, S5·수정 단계 대비), `burden.tsv`, `targets_rule.tsv`, `summary.tsv`, `against_s5.tsv`, `against_fix1.tsv`. `T/identity/identity.txt`, `T/npsame/npsame.txt`, `T/s5same/s5same.txt`, `T/fixsame/fixsame.txt`, `T/twice.txt`, `T/run.log`, `T/sha256.txt`(121 파일).
- `fixsame`은 `FIX1`(수정 단계 출력 `T_F1/`)의 facts와 이번 facts를 섹션별 레코드로 비교한다. 계수 스크립트의 넷째 인자가 `FIX1`이다(`step2a_tests_count.py T S5 OUT FIX1`).
- 측정은 commit `b00c370`에서 돌렸다. 그 뒤의 commit `9bef6ae`는 시험(`test_static_cli.cpp`)만 더했고 도구는 같다.

## 10. 수정 단계: 검증 지적과 처리 (2026-10-30)

검증자 지적 13건(같은 내용 둘을 하나로 묶어 12건)을 파일과 출력에 대어 보았다. 모두 성립했다. 판정·시험을 바꾼 것은 지적 2·3·5·8이다. 버린 지적은 없다.

| # | 지적(요지) | 확인 | 처리 | 결과 |
|---|---|---|---|---|
| 1 (와 10) | `target_rules.cpp` `field_loads`의 깊이 200 상한이 남아 있었고, §7과 보고가 "새 수치 매개변수 없음"이라 적었다 | 성립 [F] | 상한 없이 방문 집합으로 걷는다(풀은 같은 트리를 한 노드로 두고 노드를 자식 뒤에 만듦). §1.1, §7 고침. fixture `deepcond.c`(깊이 211) | 다섯 빌드 대상 수·목록 변화 0 [M]; fixture에서 R-T2가 211단계 아래 필드를 고름 [M]. 다른 걷기 상한(기존 둘, 새 루프 guard 셋, 텍스트 view 64)은 둘째 수정에서 정리함(§11 #20) |
| 2 | T2의 F8 사슬 `ExitApp → ResetCFE → … abort`가 CF가 갈 수 없는 CORE 가지를 지난다 | 성립 [F][M] | relations가 may·noreturn 종료 연산의 종료 호출 guard(OP_DERIV)를 적고 이 호출의 인자 상수로 평가, 탈출 없는 루프도 적음. TERMINALS는 프로필 추출에도 있는 기존 섹션이라 바꾸지 않고 relations에서 OP_DERIV를 읽었다. §4 T2·§8 항목 2 고침 | C1 rel 426: "with argument 0 = 3 every guard is false … the loop cfe_es_api.c:453:13 … has no exit" [M]. T2 PARTIAL 유지, 미달 항목 (b) 추가 |
| 3 | §8 항목 6의 [F] 주장("소스 위치와 CFG 순서가 같음")이 C5 C19·C20과 어긋난다 | 성립 [F] | 새 사실 LOCK_PATHS `after-spawn`(CFG 도달)으로 "뒤"를 정함. [F] 문장 삭제·정정 | 6.4.2 C19·C20 possible → proven-impossible(둘째 수정에서 `undecidable`로 고침: `RecordUsed`는 필드 단위 DISABLED, §11 #14); 소스 위치로 "뒤"였던 P0 스레드 생성 out 인자의 행 C1·R2·C5 2씩 빠짐; 루프 본문 쓰기로 C1 +73·R2 +53. 본문 있는 유도 spawn 연산의 인자 저장(PARAM_FLOW out-store)은 CFG에서 호출보다 앞이므로 spawn 뒤 쓰기(잠금 모름)로 따로 셈: C1 `MainTaskId` 행 C15–C19 유지 [M]; fixture `relpaths.c` `spawn_marked` [M] |
| 4 | 호출 경유 문맥에서 callee가 읽기 전에 잠금을 푸는지, 생성자 callee의 spawn 뒤 쓰기를 보지 않았다 | 성립 [F] | LOCK_PATHS `entry-release`; 그 행이 있으면 호출 지점 잠금을 읽기에 더하지 않음. 생성자가 spawn 뒤 부르는 함수의 같은 필드 쓰기를 실현 가능성에 넣음(한 호출 아래만 합성) | 기준 빌드에서 등급이 바뀐 행 0(C5 해당 0) [M]; fixture `relpaths.c`에서 두 경우가 "불가"를 막음 [M]. 이유로 적은 "C1·R2는 잠금 없는 행"은 틀렸다: 그 행들은 호출 지점·쓰기 잠금을 단다. 등급이 그대로인 까닭은 spawn에서 그 잠금을 잡지 않거나(잠금 창) spawn 뒤 잠금 없는 쓰기가 있어 이미 `possible`이었기 때문이다(둘째 수정, §11 #15) |
| 5 | C1·C5의 코어 task main이 행위자가 아니다 | 성립 [M] | spawn의 entry 인자가 정적 이미지가 있는 전역 표의 필드에서 오면 그 멤버의 함수 잎을 root로(`table-image-entry`); 풀지 못한 spawn은 머리에 이유와 함께 | C1 root 5개 더함(`CFE_ES/EVS/SB/TBL/TIME_TaskMain`), B73 writer `{CFE_SB_TaskMain, main}` [M]. C5는 union 멤버 별칭으로 미해결, 머리에 보고(§8 항목 11). 이미지가 실행 중의 값이라는 확인이 없었고 B73의 실현 가능성 문장이 틀렸다: 둘째 수정에서 고침(§11 #1, #22) |
| 6 | T4의 `pthread_create` writer는 relations 결합일 뿐이고, entry 경로 규칙이 TSD 소멸자·cleanup handler를 보지 않는다; writer 6에 `name` memcpy가 섞여 있다 | 성립 [M][F] | P0 out 인자 줄에 "relations 결합, 추출 writer 집합에 없음" 명시; entry 경로 규칙을 P0 key 생성의 소멸자 인자와 cleanup push 호출로 조건화(P0 종류 넷 추가); covers-more-than-the-field를 따로 셈 | C5 rel 1327·1328·1320 [M]. G10 미실현은 §8 항목 10(둘째 수정에서 T4를 "질의 수준"으로 적음, §11 #13). 종료 처리기·소멸자 함수·예외 단위는 둘째 수정에서 읽지 않는 것으로 이름 붙임(§11 #23) |
| 7 | P0 묶음이 다 차지 않았다(비교 표의 `_l`·`bcmp`, 본 표의 `*_clock*`·C11·`usleep`·`quick_exit`·rwlock) | 성립 [F] | P0 표 개정 2(72), 비교 표 개정 2(13), 담지 않는 묶음을 이름으로 기록(rwlock은 공유 획득 때문에 통째로 제외) | 새 항목 가운데 기준 빌드가 부르는 것은 `pthread_key_create`, `__pthread_(un)register_cancel`뿐이고 유도 결과 변화 0 [M]. "묶음을 통째로"라는 scope 문장은 틀렸다(빠진 구성원·묶음이 있었음): 둘째 수정의 P0 표 개정 3에서 고침(§11 #19) |
| 8 | layers 칸이 basis 편차(PI D7)와 선언된 가정·필드 단위 규칙을 적지 못한다 | 성립 [M] | 추출이 basis 편차를 facts BASIS에 복사; 행이 이름 붙인 파일의 단위 편차를 `basis-deviation(…)`, 실현 가능성이 기댄 가정·규칙을 `rests-on(…)`으로 | C5 행 159·156이 basis-deviation, rests-on 55·14·30 [M]. T5 미달 항목에 추가. rests-on은 "불가" 분기에만 붙어 잠금 창 행에 빠져 있었다: 둘째 수정에서 정적 인스턴스 판정을 쓰는 모든 등급에 붙임(0 / 0 / 55 / 17 / 16, §11 #16) |
| 9 | 트램펄린 판정이 보고서 설명보다 넓다(다른 root를 부르기만 하면 강등) | 성립 [F] | 간접 호출로만 다른 root에 들어가고 제 본문이 공유 읽기·전역 쓰기를 하지 않을 때만 강등. 머리에 남는 root를 이유별로 | 다섯 빌드 트램펄린 S5와 같음 [M]; fixture에서 직접 부르는 root는 행위자로 남음 [M]. 트램펄린이 대신하는 root가 과소 근사였던 것은 둘째 수정에서 고침(§11 #2) |
| 11 | `ex()`의 40단계 잘린 텍스트가 검사 중복 제거의 키다 | 성립 [F] | (값, 조건 트리 뿌리)로 중복 제거(풀이 같은 트리를 한 노드로 둠). 40단계는 출력에만 | R2·C5의 B 행 텍스트가 S5와 같음(새 주석 말고) [M] |
| 12 | C1 사례 관계 행 4는 콜백 이름 공간의 부산물 둘을 센 것이다 | 성립 [M] | 계수 스크립트가 짝 있는 이름 공간의 행만 셈; 부산물은 넘치는 원인 (a)로 | C1 2행; S5 출력에 고쳐 적용하면 0.7% [M] (§6) |
| 13 | "기존 golden은 하나도 바뀌지 않았다"가 2a 전체로는 틀리다 | 성립 [F] | 문장을 S5로 한정하고 2a 전체(S1–S4의 schema dump golden)와 수정 단계의 golden 변화를 적음 | §0 항목 2, §2 |

커밋: `29343d5`(lead의 WIP 스냅숏), `e43f611`, `83b2f3b`, `a378bf8`, `b76a9a9`(유도 spawn 연산의 인자 저장)와 이 문서의 커밋(접두 `kz-static 2a/fix:`).

## 11. 둘째 수정 단계: 검증 지적과 처리 (2026-10-30)

수정 단계(§10)에 대한 검증 지적 23건(막는 것 6: #9–#12, #14, #15; 나머지 17은 막지 않는 것)을 파일·출력·반례에 대어 보았다. **23건 모두 성립했다.** 처리 가운데 지적이 낸 방안과 다르게 한 곳(일부만 받거나 대안을 고른 곳)은 표 아래에 근거와 함께 적는다. 반례는 검증자의 것(`cx1`–`cx8`, `rec_after`·`memset_after`·`caller_after`·`nonstatic`)을 다시 돌려 확인했고, 그 모양을 fixture `relsound.c`(사례 이름 없음)에 옮겼다 [M]. 검증자의 반례 출력은 모두 고친 도구에서 `proven-impossible`이 아니다 [M].

| # | 지적(요지) | 확인 | 처리 | 결과 |
|---|---|---|---|---|
| 1 | C1 B73의 실현 가능성 이유 "한 table walk로 만들지 않음"이 틀리다: 코어 task main은 `cfe_es_start.c:716`의 표 걷기 루프가 `:767`에서 만들고 `:810–811`에서 기다린다 | 성립 [F][M] | 이유를 정확한 문장으로 바꿈(지적의 둘째 방안): "no static order derived: the spawn … starts writer actor(s) … [k]…; for roots resolved from a static image this revision derives no creation order … and no await". 이미지 첨자에서 반복 순서를 유도하는 것(첫째 방안)은 하지 않음(아래 (가)) | C1 B 행 49가 이 문장 [M]. §4 T1 참고 문장, §8 항목 15 |
| 2 | 트램펄린이 대신하는 root를 제 간접 호출에서만 모아, `CFE_ES_StartAppTask`가 `CFE_ES_TaskEntryPoint`를 넘기는 스레드(코어·스크립트·자식)가 빠진다(과소 근사인데 §6은 과대 근사라 함) | 성립 [F][M] | 지적의 첫째 방안: SPAWN_WRAP `called-by-entry` 연산의 호출 지점에서 푼 entry를 그 트램펄린의 root에 더하고, entry를 풀지 못한 그런 호출의 수를 머리에 적음. §1.2, §6 (b), §8 항목 14 | C1 두 트램펄린이 대신하는 root 5 → 15 [M]; R2 그대로. entry를 풀지 못한 그런 호출: R2 1(`CFE_ES_AppCreate` `cfe_es_apps.c:849:13`), C1·C5 0 [M]. C1 C 행 95 → 96(§6) |
| 3 | B의 "fixed unconfirmed"는 await 없이 생성 순서만으로도 붙는데 문장이 같고, await 문장 "serialized by it"이 술어·시간 초과 출구를 보지 않은 단정이다 | 성립 [F][M] | 등급 문장 둘(`creation order only` / `creation order + await; the await rests on the L1 premise only`); await 문장을 "await 루프가 생성 뒤에 온다(L1 전제; L1 자체·술어의 대상·시간 초과 출구는 읽지 않음)"로. §4 T1·§7에 "대체 표시로 문자 그대로, 완전성 규칙 어느 쪽으로도" | R2 B65 포함 55행이 둘째 문장 [M]. T1 PASS(개정 뒤 통과) |
| 4 | T2 (a)의 처방 "지역 변수를 정의로 펼침"은 정의 하나 규칙의 확장으로는 안 된다(`status` 정의 둘, 재시도 `ret` 정의 다섯); 재시도 항목의 탈출식이 조회 결과와 이어지지 않는다 | 성립 [F] | 문서: 처방을 흐름 민감한 도달 정의(정의 하나 제한 없음)로 적음; 재시도 항목은 문자 그대로(25, `-17`, F8 지연) 채워지나 탈출식 연결이 facts에 없다고 적음. 도구 변경 없음(새 사실이 필요, §8 항목 2) | T2 PARTIAL 그대로. §4 T2, §8 항목 2 |
| 5 | ED §3.0 F8 예가 `OS_TaskExit`를 외부 앱 가지에 두나, 원문은 CORE 가지(L381; L399, L424)이고 외부 앱 가지는 탈출 없는 지연 루프(L453)다 | 성립 [F] | §4 T2 (b), §5, §8 항목 2에 ED 전제의 오류를 적고 다음 ED 개정에서 예를 고친다고 적음(지적의 처리안). ED 본문은 이 단계에서 고치지 않음 | T2 (b)는 도구 미달만이 아니라 조건 전제의 오류 [I] |
| 6 | probe-create가 소스 모양만 보고 극성(조회 실패 결과 아래의 생성)을 보지 않으며, A의 handling 칸에 나오지 않고, dependency 문장이 제외 이유를 말하지 않는다 | 성립 [F][M] | 극성 검사(검색 함수의 RET_CONSTS를 "맞음 없음" 치환으로 평가, 콜백 검색은 콜백 반환으로, RETURNS 호출 결과 사슬로 조회 함수까지), `handling: probe-create: …` 줄, dependency "no creator left after exclusions (probe-create n, key constants differ m, same actor only k)" | C1 BP `:126 → :132` 행 handling에 극성(−17에서 돌고 0에서 돌지 않음) [M]; 제외 문장 15 / 15 / 9 / 9 / 9행; golden `relations`·`relations_np` |
| 7 | 이미지 키의 필드가 실행 중에도 쓰여도 이미지 값을 키로 써서 제외·뒤집힘에 쓴다; 정적 이미지 entry도 같은 확인이 없다 | 성립 [F] | 잎에 field-/record-written이 있으면 키 `unresolved (image, field written at run time)`, 제외·뒤집힘에 쓰지 않음. entry 쪽은 #22 | 다섯 빌드 해당 키 0 [M]. `relations.c` 변형 시험(commit `9bef6ae`) [M] |
| 8 | §4 T1 "나온 것"의 "S5와 같은 행이고 줄 번호와 layers 칸만 다르다"가 틀리다(rests-on 문장도 다름) | 성립 [M] | §4 T1 문장 고침(이번 판 대비와 S5 대비를 나눠 적음) | 문서 |
| 9 | C의 읽기 문맥이 직접 호출자뿐이다: 읽는 함수가 entry이면 잠금 없는 실행이 빠지고(`cx1`), 간접 호출자가 빠지며(`cx4`), 자식이 닿지 않는 호출을 그 호출 지점 잠금으로 "불가"라 한다(6.4.2 L667·L1020·L1284 포함) | 성립 [F][M] | entry 문맥(호출자 잠금 없음), 해결된 간접 호출자마다 문맥, 자식이 닿지 않는 문맥은 행으로 두되 `not graded`(등급 없음). fixture `relsound.c` `entry_*`·`via_*` | "불가" 0/0/0/14/30 → 0/0/0/6/14; `not graded` 20 / 20 / 5 / 14 / 14 [M]. T3 PASS → PARTIAL(§4 T3, §8 항목 16) |
| 10 | spawn 뒤 쓰기 집합이 형 있는 `T.f`만 센다: 레코드 전체 쓰기(`cx2`), 덮는 데이터 함수 쓰기, 형 없는 쓰기, 대상이 풀리지 않은 호출(`cx8`)을 빠뜨린다 | 성립 [F][M] | 레코드 전체 쓰기(`T.*`)와 WRITERS `covers-more-than-the-field` 위치의 쓰기를 셈; 생성자 아래 callee 닫힘(해결된 간접 대상, 본문 없는 P0 밖 함수에 넘긴 함수 인자 포함); 대상이 풀리지 않은 간접 호출이 있으면 `undecidable`. 형 없는 쓰기는 #14 (a)의 완전성 요구로 막음(아래 (나)). fixture `clear_*`·`hook_*` | 다섯 빌드 등급 변화 0(검증자의 C5 확인과 같음) [M] |
| 11 | "같은 획득"을 획득 지점 집합으로만 비교해, 루프에서 풀었다 같은 곳에서 다시 잡는 경우(`cx3`)를 연속으로 본다 | 성립 [F][M] | 획득 지점이 spawn 뒤 사건(LOCK_PATHS `after-spawn`)으로 다시 닿으면 연속이 아님 → `possible`("may be released and taken again between them") | 다섯 빌드 변화 0(C5 spawn은 루프 밖) [M]; fixture `loop_*` [M] |
| 12 | 공통 정체성이 정적 인스턴스가 아니면 `possible`("no lock of static identity")로 가고, ED §3.3의 "결정 불가"가 나오지 않는다(`cx5`) | 성립 [F][M] | 읽기·spawn·쓰기가 한 획득의 비정적 정체성을 공유하면 `undecidable`(must 정체성 미증명). §4 T3 판정과 §7의 [I] 문장을 고친 코드에서 다시 씀: 형 단위 규칙의 대비는 6.4.1 `possible` 대 6.4.2 `undecidable` | 다섯 빌드에 그 분기 행 0 [M]; fixture `ptr_*` [M] |
| 13 | T4의 "writer에 `pthread_create`"는 relations의 질의 결합이고 ED §3.0 G10의 추출 writer는 실현되지 않았다 | 성립 [F] | 지적의 첫째 방안: T4를 **PASS(질의 수준)**으로 적고 G10 미실현을 빠진 것으로 명시(§0, §4 T4, §8 항목 10). 새 추출 섹션(둘째 방안)은 하지 않음(아래 (다)) | T4 PASS → PASS(질의 수준) |
| 14 | "불가"가 필드의 FIELD_STATUS를 보지 않고(6.4.2 C19·C20의 `RecordUsed`는 DISABLED), 레코드 쓰기·형 없는 쓰기·효과 모르는 호출과 생성자 호출자의 반환 뒤 쓰기를 보지 않는다(`rec_after`, `memset_after`, `caller_after`) | 성립 [F][M] | (a) 필드가 필드 단위 규칙으로 writer-set-complete가 아니면 `undecidable`, 그 규칙을 `rests-on`에; (b) #10의 쓰기 집합; (c) 범위를 "생성자의 한 호출(ED §3.3 항목 3)"로 이유 문장과 문서 §5g에 적고, 생성자를 부르는 함수의 제 본문에 그 필드·레코드 쓰기나 풀리지 않은 호출이 있으면 `undecidable`(아래 (라)). fixture `after_*` | 6.4.2 C19·C20 `proven-impossible` → `undecidable` [M]. T3 사례 행(`CFE_ES_TaskRecord_t.RecordUsed`·`AppId`, 필드 단위 complete)은 이 조건으로 바뀌지 않음. §4 T3, §6, §10 지적 3 고침 |
| 15 | 비대칭: 읽기만 비정적이면 `undecidable`, 공통 비정적이면 `possible`; §4 T3 [I] 문장과 §10 지적 4의 "C1·R2는 잠금 없는 행"이 틀리다 | 성립 [F][M] | #12와 한 규칙으로(공통 정체성 전체에 대칭). §4 T3 [I] 문장, §10 지적 4의 결과 칸 고침 | 검증자가 센 R2 18·C1 15 행은 spawn에서 그 잠금을 잡지 않거나(잠금 창) 잠금 없는 쓰기가 있어 정체성과 무관하게 `possible`로 남는다: 다섯 빌드의 `possible` 이유는 "읽기 잠금 없음", "잠금 창", "잠금 없는 spawn 뒤 쓰기"뿐이다 [M] |
| 16 | `rests-on`이 "불가" 분기에만 붙어, 정적 인스턴스 판정을 쓰는 잠금 창 행(6.4.1 C31 등)에 없다 | 성립 [F][M] | 정적 인스턴스 판정을 쓰는 모든 분기(불가, 잠금 창, 다른 획득, 다시 잡힘, 두 정적 인스턴스)에 선언된 가정 셋과 필드 단위 규칙을 붙임. §4 T3·§5에 6.4.1 잠금 창도 기댄다고 적음 | rests-on 0 / 0 / 55 / 14 / 30 → 0 / 0 / 55 / 17 / 16 [M](6.4.2의 감소는 #9·#14로 "불가"가 줄어서) |
| 17 | §0·§5가 PI A2 ④⑤로 읽은 표시 없이 "반증 조건 충족되지 않음"만 적는다 | 성립 [F] | §5에 A2 단락: T1은 "개정 뒤 통과", T1·T2의 통과는 "관계를 낸다"의 통과(④), 두 수정의 개정이 T1–T3 행을 계기로 함(⑤). "ED §6에서 반증되지 않음"과 "A2에서 탐지 근거 아님"을 나눔. §0 항목 4 | 문서 |
| 18 | 행이 원문 대조 없는 P0 항목에 기대는 것이 layers 칸에 없고(PI A4 기본값), `pthread_create` 원문 대조가 배포판 패치를 보지 않았다는 [U]가 없다 | 성립 [F][M] | layers `P0-unverified(<항목>)`(행의 유도가 쓴 항목), T5에 행 수; `pthread_create` 원문 줄과 relations 머리에 배포판 패치 미대조. 빌드는 `Ubuntu GLIBC 2.39-0ubuntu8.9`를 링크 [M] | 272 / 272 / 138 / 149 / 147행 [M]. T5 PARTIAL 그대로(A4 기본값으로는 통과로 세지 않음) |
| 19 | P0 scope가 "묶음을 통째로"라 하나 빠진 묶음·구성원(join·detach, timer, select·poll, 초기화·속성, `sscanf` 등)이 있고, `thrd_create`에 "시작 전 저장"이 없다 | 성립 [F][M] | P0 표 개정 3: scope 문장이 묶음의 어느 구성원이 항목인지와 빠진 것을 이름으로 적음; `thrd_create` 의미에 시작 전 저장; §1.3·문서 §5f의 "listed whole" 고침 | 다섯 빌드가 부르는 판 밖 함수는 §1.3. 유도 결과 변화 0(fixsame: BASIS·P0_TABLE만 다름) [M] |
| 20 | 고정 수치 상한(argflow 100000, facts_text 10000·64, 기존 ast_tu 4096·ast_fn 6)이 기록·계수되지 않는다 | 성립 [F] | 루프 guard 둘(100000, 10000)은 방문 집합·후위 번호 조건으로 바꿈(출력 변화 없음); 64 원소는 보기의 출력 한계로 두고 넘으면 그렇다고 적음; 4096·6은 이름 붙인 상수로, 걸리면 FACT_INIT "상수 아님" 행(이전엔 0으로 적을 수 있었음)·잘린 수를 세고, BASIS `param.walk_bounds`(unspecified_by_reference)·`count.walk_bounds_hit`. §7 목록 | 다섯 빌드 걸린 수 0 [M] |
| 21 | lint `case-ids`가 `tools/`를 읽지 않는다; `cmd_hold.cpp` 주석 | 성립 [F] | `tools/`를 읽게 함, selftest에 도구 파일 경우 추가, 주석 고침 | lint 6/6 [M] |
| 22 | 정적 이미지 entry 규칙이 표가 실행 중에 쓰이지 않는다는 확인 없이 이미지를 값으로 쓴다(C1 표는 const 아님) | 성립 [F] | 멤버의 FIELD_STATUS가 두 규칙 모두 complete이고 writer 인스턴스가 없으며 이미지 잎에 object-/field-/record-written이 없을 때만 쓰고, 아니면 풀지 않고 이유를 머리에 적음. 머리에 확인 문장 | C1 `CFE_ES_FuncPtrUnion_t.MainTaskPtr` 확인 통과, root 5 그대로 [M]. `relpaths.c` 변형 시험(commit `30c8713`) [M] |
| 23 | 수명의 entry 경로 문장이 프로세스 종료 처리기·소멸자 함수·예외 단위를 보지 않는다 | 성립 [F] | 문장과 문서 §5g에 읽지 않는 것으로 이름 붙임(`atexit`, `at_quick_exit`, `on_exit`, 소멸자 함수, 예외를 켠 단위) | 다섯 빌드 수명 문장 [M] |

**지적의 방안과 다르게 한 곳** (받지 않은 부분과 근거)

- (가) **#1의 반복 순서 유도.** 이미지 첨자(`[17]`·`[19]`…)를 생성 순서로 읽으려면 spawn을 감싼 루프의 첨자가 이미지 첨자를 차례로 지난다는 사실이 필요하다. 이 판의 순서는 LOOP_ITERS(펼친 table walk)에서만 오고, C1의 `CFE_ES_CreateObjects` 루프(`cfe_es_start.c:716`, loop 204)는 FACT_LOOPS에서 `count-bounded trip=exact(30)`이며 C1의 LOOP_ITERS는 0행이다(R2는 같은 함수의 루프 둘이 `table-walk … table=GLOBAL_CONFIGDATA.CoreObjectTable`, LOOP_ITERS 314행) [M]. 그래서 C1의 root는 `table-image-entry`다. 이미지 경로만으로 순서를 내면 루프 첨자와 이미지 첨자가 같다는 것을 증명 없이 받는 것이 된다. 지적이 허용한 둘째 방안(정확한 이유)으로 두고, 필요한 사실을 §8 항목 15에 적었다.
- (나) **#10의 형 없는 쓰기.** 형 없는 쓰기를 행마다 짝짓지 않았다. 대신 #14 (a)로 필드가 필드 단위 규칙에서 writer-set-complete여야 "불가"를 낸다. 그 규칙에서 complete는 형 없는 저장(cast·escape·other fields·unknown effect)이 그 필드에 닿지 않는다는 뜻이다(S4, `kairos/docs/static_facts.md` §5c) [F]. 그래서 형 없는 쓰기가 있을 수 있는 필드는 이미 `undecidable`이 되고, 그렇지 않은 필드에는 셀 형 없는 쓰기가 없다. 다만 이 배제는 PI 결정 대기 규칙에 기대며, 행의 `rests-on`에 그 규칙이 적힌다. 같은 이유로 본문 없는 P0 밖 함수의 호출은, 그 레코드에 대한 포인터를 받으면 unknown effect로 필드가 complete가 아니게 되므로 따로 세지 않았고, 함수 인자를 받으면 그 함수를 호출 대상으로 센다(지적은 그런 호출을 모두 "잠금 없는 가능한 쓰기"로 보라 했다). 인자 없이 전역을 직접 쓰는 본문 없는 함수는 닫힌 프로그램(빌드의 단위 전부)을 전제로 한 S4 완전성 규칙 밖에서만 생긴다.
- (다) **#13의 G10.** 추출에 P0 out 인자 저장을 writer로 넣으면 FIELD_STATUS(`OS_task_record_t.id` 등)의 결정이 바뀐다. 이 단계의 범위(새 섹션만 golden 변경 허용)에서 그것을 하려면 새 섹션과 그것을 읽는 완전성 규칙이 함께 필요하고, 그 규칙은 PI의 완전성 결정과 엮인다. 지적이 허용한 첫째 방안(질의 수준 PASS와 빠진 것 명시)으로 두었다.
- (라) **#14 (c)의 호출자 반환 뒤.** 생성자를 부르는 함수의 **제 본문** 쓰기와 풀리지 않은 호출은 순서 없이 보아 있으면 `undecidable`로 내린다(fixture `after_*`). 그 함수의 callee와 그 위 호출자가 반환 뒤 하는 일은 읽지 않고, 그 범위를 이유 문장(`Scope: one call of P (ED §3.3 item 3) …`)과 문서에 적는다. 근거: ED §3.3은 질문을 "P 안에서 s 뒤에 오는 쓰기 w"(항목 3)와 "s 직전부터 w까지 같은 잠금"으로 정한다 [F]. P 밖의 뒤 쓰기는 다른 w의 관계다. 그것까지 "불가"의 조건에 넣으면 필드 기반 추상에서는 그 쓰기가 같은 레코드인지 정하지 못해 "불가"가 거의 남지 않는다 [I](C5의 ES 명령 루프는 나중에 앱을 지우며 `RecordUsed`를 쓴다). 6.4.2 L800·L445·L551의 "불가"는 이 범위의 것이다(호출자 `CFE_ES_ParseFileEntry`, `CFE_ES_ProcessControlRequest`, `CFE_ES_StartAppCmd`의 제 본문은 그 필드를 쓰지 않음 [M]). §8 항목 6.
- (마) **#5의 ED 정정과 T3 조건의 전제.** ED 본문은 이 단계의 커밋 범위(도구, 시험, 드라이버, 결과 문서) 밖이어서 고치지 않았다. 고칠 곳을 §8 항목 2(F8 예)와 항목 16(T3 읽기 지점 셋)에 적었다.

**재측정** [M]: 빌드 `merge-check/kz-llvm`, 시험 237/237(건너뜀 1, 원래대로), `test_static_cli` 515, `test_static_wrappers` 512, lint 6/6. 프로필 추출 V0·V2 × C1 pre tx·R2 core·C5 6.4.1 여섯 경우 STRINGS 말고 동일. `fixsame` 다섯 빌드 BASIS·P0_TABLE 말고 동일. T1–T7 드라이버(`all`)와 T7 두 번 실행 바이트 동일: facts `8e189863…`, `9f3a0e20…`, `e7c4fa16…`, `26cd3ee3…`, `ceec3748…`; relations `a40cdd11…`, `13b45c6f…`, `2e0eedf7…`, `4c3ca8fe…`, `6e91fe36…`(c1_pre_tx, c1_post_tx, r2_core, c5_641, c5_642 순). `T/sha256.txt` 121 파일.

**판정 변화**: T3 PASS → PARTIAL(#9), T4 PASS → PASS(질의 수준)(#13). T1 PASS(개정 뒤 통과), T2·T5 PARTIAL, T6·T7 PASS는 그대로다.

커밋: `beab307`(lead의 WIP 스냅숏), `b00c370`(도구·시험·golden·문서·드라이버), `9bef6ae`·`30c8713`(시험 추가, 도구 같음)과 이 문서의 커밋(접두 `kz-static 2a/fix2:`).

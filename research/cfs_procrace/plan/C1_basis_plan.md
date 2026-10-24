# C1 기준 구성 계획 — D22(공개 BP 기반)와 P-C1HOLD

> **2026-10-22 (SPEC rev 1.3, 결정 D3·D5):** C1은 **기여 증거의 첫 사례**다. D22 공개 BP 기반 빌드를 단계 B(실제 기록 판정)와 병행한다. 단계 B의 C1 완료 조건에 **CF와 세마포어 provider(BP) 코드까지 정적 닫힘을 넓히는 것**(app-unit link probe 필수, SPEC §4.1 "Closure per case")과 task→생성 앱 매핑의 생성자 쪽 증거(SPEC §5.7 rev 1.3 note)가 들어간다. 수정 전 `281a941`의 단일 실패 조회는 NOT_READY_UNJUDGED로 정리했다(SPEC App. A.4 step 6, §9.6의 라벨 불일치 해소). C1을 재시도 소진으로만 환원하지 않으며, OSAL 내부 객체 상태·잠금 경로는 다섯 사례 성공 수에 더하지 않는다(P-C1HOLD).

- 작성일: 2026-10-15. 개정 2: 2026-10-16(§15). 개정 3: 2026-10-16(검토 11건, §16).
- 성격: **원출처 소스의 재확인 기록 + 실행 계획**. 이 문서를 쓰는 동안 cFS·cFE·OSAL·PSP·CF·BP를 빌드하거나 실행하지 않았고, 설치도 하지 않았다. 아래의 모든 "빌드 계획"과 "보류 설계"는 계획이며 수행된 결과가 아니다.
- 입력 결정: [PI_DECISIONS_RESOLVED_20261015.md](../Architecture/spec/PI_DECISIONS_RESOLVED_20261015.md) §3.2(D22), §3.3(P-C1HOLD), §2(T1·QT-3·D1·D14), §4(D15·D19). 이 문서는 그 결정을 **결정으로** 적용한다. 결정문의 표현을 사실로 인용하지 않는다.
- 개정: 2026-10-15 검토 11건을 반영했다(처리 결과는 §14). 검토 반영으로 바뀐 판단은 D22의 적용 순서(§8.0), bplib 고정 커밋과 단위 시험 플래그(§4.3, DV-13), readiness precondition의 정의(§1.2), 해제 규칙 R-c의 삭제(§9.2), 섭동 비율의 비교 간격(§9.5)이다.
- 개정 2(2026-10-16): 명세 인용 기준을 커밋된 rev 1.2(`6be8935`)로 옮기고, 그 판의 App. A.4 step 3(QT-4·Q15 엄격 대안: `caller` 기록이 있어야 TOLERATED)과 §4.13 "Attach strategy"를 §9에 반영했다. 소스를 다시 읽어 BP `AppInit`의 조기 반환 경로(생성 호출 전에 provider가 스스로 실패하는 경우, §5.4, §9.1 P-E)와 OSAL 내부의 이름 공개·id 확정 사건(§9.1 P-2a~P-2c)을 더했고, CI 후보 OSAL에서 확정이 반환보다 앞선다는 점을 확인했다(§1.2). 변경 목록은 §15.
- 개정 3(2026-10-16): 검토 11건을 반영했다(§16). 바뀐 판단은 *readiness violation*을 VIOLATING 시도에만 쓰는 것(§9.3), `833fdbb`의 `caller` 기록 다시 묶기를 C1 출력 전의 선행 조건으로 둔 것(§9.3), 해제 규칙 R-d를 CF 종료로 줄인 것(§9.2, Q-10), 앞 채널 실패로 조회가 없는 키의 결과 "도달하지 않음"(§9.2, §9.3), BP 자신의 조회 호출 지점을 별개 인스턴스로 둔 것(§9.3, Q-11), 로드맵 `[제안]`과 결정의 출처 구분(§1.3 등)이다.
- 개정 4(2026-10-24, 마스터플랜 반영 W0, [MASTERPLAN_ADOPTION_20261024.md](MASTERPLAN_ADOPTION_20261024.md)): CF 조회·BP 존재 확인·provider 초기화 실패·provider 없는 이름의 호출 목적 구분(§9.7)을 더하고, §9.3 하단·§9.6·§11.2의 `281a941` 라벨 표기를 rev 1.3(NOT_READY_UNJUDGED, 요약 `INCONCLUSIVE(caller_requirement_absent)`)에 맞췄다. #184 댓글은 2026-10-24에 읽었다(`cases/C1.md` §0a). 기대 결과표는 [expected_results_C1_R2.md](expected_results_C1_R2.md)에 동결한다.
- 입력 계획: [ROADMAP_20261015.md](ROADMAP_20261015.md) §3(정정 목록), §8(C1), §12(산출물). 로드맵 §1.1에 따라 작업 지시·완료 기준·순서는 모두 `[제안]`이다. 이 문서는 로드맵의 `[제안]`·`[분석]` 항목을 제안·분석으로만 다루며, 그것을 근거로 "이미 정해졌다"고 쓰지 않는다. 결정의 출처는 결정문과 명세의 [D]뿐이다.
- 규칙: [CONDITIONS_POLICY.md](../CONDITIONS_POLICY.md) — 값은 레퍼런스에서 가져오고, 레퍼런스가 정하지 않는 값은 `unspecified_by_reference`로 둔다. 이 문서에는 임의로 고른 N·임계값·대기 상한·일정 수치가 없다.
- 이 문서가 건드리지 않는 것: `kairos/`와 소스 코드, 동결된 이력 문서, `env/`의 기존 기록, `/home/user/work/procrace/cfs_ref`(읽기만 했다).

## 0. 요약

1. **공개 BP 구성의 값은 고정 커밋 `b051eb0`에서 다시 읽어 확인했다**(§3). semaphore 이름 `BP0_tsem`·`BP1_tsem`은 CF 표(`cf_def_config.c` L40·L54)와 BP 기본값(`bp_semcfg.c` L43–L49)에 모두 있고, 시작 스크립트는 CF(L6) 다음 BP(L7), 설정 우선순위는 CF 65·BP 55이다. 결정문 §3.2의 표 4행은 그대로 맞다. 다만 OSAL에서는 **숫자가 작을수록 우선순위가 높으므로** 설정상 BP(55)가 CF(65)보다 높다(§3.6). 실제 적용 여부는 ENV에서 측정으로 "적용되지 않음"이 기록돼 있고(E4), BP CI에서는 알 수 없다.
2. **BP CI가 가리키는 참조는 대부분 움직이는 참조이다**(§4). cFS `main`, bplib `main`, CF `main` 또는 삭제된 브랜치, 합쳐 넣는 PSP 브랜치, 러너 이미지, `actions/*@v3`. 이 중 일부는 오늘 값을 읽어 **BP 고정 커밋과 맞지 않음을 확인했다**: bplib `main`의 오늘 값(`c5fb835`)에는 BP가 쓰는 bplib 식별자 30개 중 26개가 없다. 따라서 "워크플로를 그대로 다시 돌린다"는 재현이 아니다. 또 bplib `main`은 창의 끝(2022-11-30 20:14:16Z)에 `e7524239`로 옮겨 갔고, 이 커밋의 메시지는 CFS 빌드에서 bplib의 `ut-functional`이 "링크되지 않는다"는 빌드 오류를 고친다고 적는다(§4.3). ENV의 `-DENABLE_UNIT_TESTS=TRUE`는 `d4c6db9`의 CMake 분기에서 바로 그 `ut-functional`을 추가하게 만든다 [F, 소스 읽기]. 그 빌드가 실패하리라는 것은 커밋 메시지에 근거한 추론이며 [I] 빌드로 확인하지 않았다.
3. **BP 워크플로 두 개가 D22의 수정 전후 쌍을 그대로 정의한다**(§4.1): 수동 실행 워크플로는 `nasa/cf` `main`(2022-12-01 19:35Z까지 `281a941`), PR 워크플로는 `jphickey/cf@fix-184-throttle-sem-race`(`833fdbb`로 추정)를 쓴다. 이것은 워크플로 파일의 사실([F])과 시점별 해석([I])이며, 어느 쪽이 실제로 실패했는지의 로그는 없다.
4. **CF `281a941`과 `833fdbb`는 BP 표 형식과 필드 순서까지 일치한다**(§5). 두 버전의 차이는 3개 파일(재시도 루프, 상수 2개, 단위 시험)이고 `cf_app.c`·`cf_tbldefs.h`·`cf_def_config.c`는 바이트 단위로 같다. 반면 원 보고자의 `2a024d8`은 표 구조(타이머 위치, 헤더 위치, 필드 순서)가 달라 BP 표와 섞을 수 없다. 재시도 상수는 `833fdbb`와 v7.0.1 모두 소스에서 25회·100 ms를 읽었고 `281a941`에는 재시도가 없다.
5. **D22의 적용 순서는 "공개 통합 구성의 버전 호환성 확인이 먼저"이다**(결정문 §3.2). 이 문서의 정적 대조(§5.3)는 공개 통합 스택(CI 후보)에서 **이름 수준**으로 맞는다는 것만 보였고 조립·기동은 하지 않았다. 따라서 ENV 번들(v7.0.1)로 옮기는 계획(§8.3의 B0–B8)은 **둘째 단계**이다: 첫째 단계 P-REC(공개 통합 구성의 조립·기동, §8.5)가 그대로 구성되지 않는다고 기록된 뒤에만, 또는 PI가 이 순서에 대한 예외를 승인한 뒤에만 시작한다(§12 Q-5). 그 전에는 ENV 이식을 D22의 기준 구성이라고 부르지 않는다. 이식을 하게 되면 CF는 `CFS_APP_PATH` 앞순위로 교체할 수 있지만, BP는 (a) 메시지 ID 기준 매크로 2개, (b) PSP `iodriver`의 이름·구조체 필드 변경(8개 이름은 대소문자 변경 수준, `Location_t` 필드는 다름), (c) ENV PSP에 없는 `unsock_intf` 모듈, (d) BP CI가 복사하는 cFE 샘플 설정 파일 이름 변경을 만나며, ENV의 단위 시험 플래그 `-DENABLE_UNIT_TESTS=TRUE`는 고정 bplib `d4c6db9`에서 bplib 자신의 커밋 메시지가 말하는 빌드 오류 경로를 여는 것으로 읽힌다(§4.3, DV-13, [I]). 모든 차이를 이탈 대장(§8.4)에 적었다. 결정문 §3.2의 적용 순서("그대로 구성할 수 없으면 다른 버전으로 옮긴 차이를 기록한다")와 명세 App. A.4 step 2("the move to other versions … is a derived basis, and every difference from the public values is recorded as a `deviation`")에 따라 [D], 이식이 시작되면 그 구성을 공개 코드 기반 파생 구성으로 표시한다. 로드맵 §8.1도 같은 표시를 `[제안]`한다.
6. **P-C1HOLD는 사건 기반으로 설계했고 대기 상한 값은 정하지 않았다**(§9). 보류 지점은 provider의 `OS_CountSemCreate(name)` 진입 직전, 해제는 CF의 재시도 종료(C-4) 또는 최종 초기화 결과(C-6) 사건이다. A1–A5는 관측 사건 열로 대응시켰다. A2는 자연 실행에서만 얻는 것으로 둔다(결정 범위 밖의 "제어된 A2"는 Q-9로 올렸다). 중단 조건·감시 한도는 파일럿 값으로 남겼다. 개정 2에서 시도 분류를 명세 rev 1.2에 맞췄다: TOLERATED는 CF의 시작 경쟁 인정 주석이라는 `caller` 기록이 있을 때만 나오며, 그 주석은 `833fdbb`의 `cf_cfdp.c` L993–L999에 있으나 명세는 v7.0.1의 줄만 인용한다(§9.3, §9.6). 개정 3에서 그 기록을 `833fdbb`에 다시 묶는 anchor audit 항목을 **C1의 첫 출력 전 선행 조건**으로 두었다(그래야 A2의 앞선 시도가 좋은 쪽이다). *readiness violation*은 VIOLATING 시도에만 쓴다(명세 §0, §0.11 item 2). 해제 사건은 결정된 두 사건과 그것을 함의하는 CF 종료(C-7)만 쓰고, "다른 쪽이 막힘"에서의 해제는 Q-10으로 올렸다. 또 provider가 생성 호출 전에 스스로 실패하는 경로(`bplib_init` 등, §5.4)를 P-E로 관측해 A3와 섞지 않는다.
7. 여전히 `unspecified_by_reference`인 조건은 §10에 모았다. 가장 큰 것은 provider 구현 방식(§8.2의 PI 결정 필요 항목), D22 순서의 첫 단계 처리(Q-5), bplib 고정 커밋과 단위 시험 플래그의 조합(Q-4), 원 보고자·BP CI 당시의 전체 버전 조합이다. C1 기대 결과표를 동결하기 전에 필요한 것은 G-REBIND(§9.3, C1B-U21)와 BP 호출 지점의 기대 상태(Q-11, C1B-U23)이다. 해제 규칙에서 "다른 쪽이 막힘"을 쓸지는 Q-10이다.

## 1. 범위·표기·용어

### 1.1 표기

| 표시 | 뜻 |
|---|---|
| [F] | 고정 커밋의 소스·문서에서 읽은 사실. 경로와 줄 번호를 붙인다. |
| [M] | 이번 점검에서 도구로 얻은 값(차이 비교, 식별자 대조 결과, 병합 시험의 출력 등). 방법을 같이 적는다. |
| [I] | 사실에서 연결한 추론. 원출처가 직접 말한 것이 아니다. |
| [D] | PI 또는 명세가 내린 결정. 사실이 아니다. |
| [U] | 확인하지 못한 것. |

줄 번호는 모두 해당 커밋의 파일 기준이다. 커밋 약어는 §2의 표를 따른다.

### 1.2 용어(T1 = (a))

- 판정 대상의 이름은 **readiness specification(준비 상태 명세)**, 선행 조건은 **readiness precondition**, 위반은 **readiness violation**이다. *readiness violation*은 **VIOLATING 시도**, 곧 caller 수준 Allowed가 거부한 범위 안의 시도에만 쓴다 [D: SPEC_FINAL @`6be8935` §0 "An in-scope occurrence rejected by the caller-level Allowed is a readiness violation", §0.11 item 2 "a `VIOLATING` occurrence is a readiness violation"]. NOT_READY_UNJUDGED 시도는 나쁜 쪽이지만(명세 §1.6의 bad 정의) Allowed가 거부한 것이 아니므로 readiness violation이라고 부르지 않는다. 그 결과 라벨은 `not-ready-unjudged`이다(명세 §1.10 "Outcome label").
- **원출처의 말**[F]: 이슈 #184 본문 "the CF app expects that semaphore to be created _before_ it starts"; CF 문서(`2a024d8`의 `docs/cf_guide.md` L48–L57) "If a semaphore is expected and not found, the application will terminate during initialization". 저장소 기록 [cases/C1.md](../cases/C1.md) F1·F4가 인용한다. 두 출처 모두 "provider의 생성 호출이 반환했는가"를 말하지 않는다.
- **C1의 readiness precondition**(연구자의 형식화, 원출처의 인용이 아니다) [I/D]: *CF의 `OS_CountSemGetIdByName(sem_name)`이 효력을 갖는 시점(조회의 선형화 시점, 객체 표 훑기)에, 이름 `sem_name`의 counting semaphore가 존재할 것 — 곧 그 id가 확정되어 이름으로 보일 것.* 이것은 명세 §2.3의 `K-REG.osal-countsem-lookup` 블록의 `requires state(osal.obj<countsem>(arg.name)) = LIVE`와 같은 대상이다 [D: SPEC_FINAL rev 1.2 @`6be8935` §2.3의 `readiness K-REG.osal-countsem-lookup` 블록. rev 1.1 @`2499ad4`에서는 같은 블록의 키워드가 `contract`였다]. 판정은 **객체 존재**에 대한 것이며 `OS_CountSemCreate`의 **반환 시점**에 대한 것이 아니다: ENV OSAL에서 id 확정(`OS_ObjectIdFinalizeNew` 안의 `OS_ObjectIdTransactionFinish`, `osapi-idmap.c` L859)은 `OS_CountSemCreate`가 반환하기 전에 일어나므로(`osapi-countsem.c`: `OS_ObjectIdFinalizeNew` 호출 L106, 반환 L109), 조회가 생성 호출의 반환보다 먼저 시작해도 확정 뒤에 표를 훑으면 성공한다 [F, ENV OSAL 읽기]. CI 후보 OSAL `b44f4ce`·`f34759d`도 같은 구조이다: `osapi-countsem.c`의 `OS_ObjectIdAllocateNew` L94, `OS_OBJECT_INIT` L100, `OS_CountSemCreate_Impl` L103, `OS_ObjectIdFinalizeNew` L106, `osapi-idmap.c`의 `OS_ObjectIdTransactionFinish(token, &final_id)` L849, 그리고 `OS_ObjectIdConvertToken` 함수 본문은 ENV와 글자 단위로 같다 [F/M, 2026-10-16 `grep`·`diff`]. 저장소 기록 `cases/C1.md` §7이 쓰는 "생성이 반환한 뒤"는 이 형식화보다 강한 서술이며, 반환은 존재의 충분조건이지 필요조건이 아니다 [I]. 확정 전 RESERVED 구간에서의 조회는 별개의 가설 사례이다(A5, `cases/C1.md` §7 Refinement).
- 이 정의 아래에서 **판정 열은 반환 코드에서 채우지 않는다**(QT-3). 어떤 조회 시도가 precondition을 만족했는지는 id 확정 사건과 그 조회의 선형화 시점의 순서로 정하고, 확정 사건을 직접 관측하지 못하면 "`OS_SUCCESS`와 같은 id가 반환됐다"를 *결과에서 함의되는 reads-from*으로만 쓴다(명세 App. A.4 step 2의 "outcome-implied reads-from") [D]. 시도 하나의 분류는 명세 App. A.4 step 3·6의 어휘를 따른다: READY, TOLERATED, VIOLATING, NOT_READY_UNJUDGED(§9.3). rev 1.2의 QT-4 엄격 대안에 따라 TOLERATED는 `caller` 기록(CF 자신의 시작 경쟁 인정 주석, `code_admission`)이 있는 판에서만 나온다(명세 §1.6 "Caller-level Allowed", App. A.4 step 3; 기록 자체는 §2.3의 블록) [D].
- "API contract"라는 말은 **문서화된 API 의미**에만 쓴다. 예: OSAL의 `OS_CountSemGetIdByName`은 이름이 없으면 `OS_ERR_NAME_NOT_FOUND`를 반환한다고 문서에 적혀 있다(ENV OSAL `osapi-countsem.h` L157–L160 [F]).
- QT-3에 따라 세 층을 섞지 않는다: (1) 프레임워크 문서·실제 API 동작(OSAL 반환값), (2) 호출자 요구(CF의 readiness precondition), (3) 관측 결과. 이 문서의 A1–A5 대응표(§9.3)는 세 층을 열로 분리한다.

### 1.3 이 문서가 결정하지 않는 것

아래 항목은 이 문서가 결정하지 않는다. 각각이 어디에 남아 있는지는 다음과 같다. PI 결정이 필요한 것만 §12에 있다.

| 항목 | 남은 곳 | PI 질문인가 |
|---|---|---|
| 원 보고자 환경의 재현 | 결정문 §3.2가 공개 구성의 값과 원 보고자 환경의 값을 구분하고, "원 보고자가 사용한 provider와 해당 공개 BP 구성의 동일성"을 확정되지 않은 조건으로 남겼다 [D]. 명세 App. A.4 step 2도 BP 값을 "not as values of the original reporter's environment"로 적는다 [D]. 로드맵 §8.1의 "같다는 주장은 하지 않는다"는 `[제안]`이며 같은 방향이다. 값은 C1B-U01·U03·U04에 `unspecified_by_reference`로 남는다 | 아니오(재현을 시도하자는 결정이나 제안이 없다) |
| provider 구현 방식(B-1~B-4) | §8.2, §12 Q-2 | 예 |
| D22 순서의 첫 단계(P-REC 또는 대체) | §8.0, §8.5, §12 Q-5 | 예 |
| bplib 고정 커밋과 단위 시험 플래그 | §4.3, DV-13, §12 Q-4 | 예 |
| 실행 횟수 N | D14: S1에서 목적별로 정하고 본실험 전에 동결한다. §9.4 PV-4, C1B-U07 | 아니오(결정문이 시점과 방법을 이미 정했다) |
| 보류 감시 한도·중단 조건 | P-C1HOLD: 파일럿 값. §9.4 PV-1~PV-3, C1B-U06 | 아니오(같음) |
| A4를 사례 결과로 셀지 | 명세 SPEC_FINAL @`6be8935` App. A.4 step 6 [D]: "**no referenced basis exists** … A4 is shown by litmus L-PATIENT variant 2 and LP-CASE-C1 (App. A.8) with the provider never running, or recorded as 'not realized: no referenced configuration'; only on a provider-less basis that the PI sets as a recorded deviation is it realized". 로드맵 §8.3의 A4 행("단위 예제로 검증하고 실제 사례 결과로 세지 않음")은 `[제안]`이다. 결정문 §3.3은 A4를 다루지 않는다. §9.3 A4 행, C1B-U12 | 아니오(실현하려면 PI가 provider 없는 구성을 이탈로 정해야 하며, 이 문서는 그것을 요청하지 않는다) |
| BP 자신의 조회 호출 지점(P-1)의 분류 | §9.3 "BP 호출 지점의 인스턴스", §9.7 (b), Q-11 | 2026-10-24: 처리 방향은 호출 문맥 규칙(SPEC rev 1.3 보충)으로 정함. 프로필 선언은 남음 |
| "다른 쪽이 막힘"에서의 해제 | §9.2 R-d, Q-10 | 예 |
| "제어된 A2"(R-c) | 결정 범위 밖의 제안이다. §9.2.1, §12 Q-9 | 예 |

## 2. 점검에 쓴 소스 사본

모든 사본은 저장소 **밖**의 `/home/user/work/procrace/refs/` 아래에 새로 만들었다(blobless clone + `git worktree`). `/home/user/work/procrace/cfs_ref`는 읽기만 했다. 저장소 독자는 이 사본에 접근할 수 없으므로, 점검을 다시 하는 데 필요한 것을 이 문서에 넣었다: 사본 목록과 SHA, 식별자 대조 스크립트 전문, 표 행마다의 정규식은 §13에 있다. [M] 항목 중 `git log`·`git diff`·`git ls-tree`로 얻은 값은 SHA만 있으면 공개 원격(nasa/*, jphickey/*, intel/tinycbor)에서 다시 만들 수 있다. 원격에서 지워진 참조(`jphickey/cf`의 수정 브랜치)는 다시 만들 수 없다(§4.2).

명세 인용 규칙: Kairos 명세는 **`Architecture/spec/SPEC_FINAL.md` @`6be8935`(rev 1.2, T1 이름 변경 반영)의 절 이름**으로 인용하고 줄 번호를 쓰지 않는다. 개정 1은 @`2499ad4`(rev 1.1)를 기준으로 했다. 작업 트리의 같은 파일은 다른 에이전트가 병렬로 고치고 있으며(2026-10-16 점검 시 커밋되지 않은 31줄 변경. 이 문서가 인용하는 §2.3·§4.13·App. A.4는 그 변경에 들지 않는다 [M: `git diff -U0`]) 줄 번호가 커밋마다 달라진다. 명세의 규칙은 연구 설계의 **결정 [D]**이며 사실 [F]이 아니다.

| 이름(약어) | 저장소 @ 커밋 | 로컬 경로 | 용도 |
|---|---|---|---|
| BP | nasa/bp @ `b051eb0d12bfe76c0230215c2ff851cfaa553ce4` | `refs/bp_b051eb0` | 공개 BP 구성의 고정점(D22). 저자 2022-11-15 11:17:40 -0500, 커밋 2022-11-18 10:30:29 -0500 [M: `git log`] |
| CF-rep | nasa/CF @ `2a024d8efd9f44f54b7a59f7face24f255536236` | `refs/CF_2a024d8` | 원 보고자가 링크한 리비전 |
| CF-pre | nasa/CF @ `281a94188cd7d885a5aed01ee041f3ece2b0486f` | `refs/CF_281a941` | D22 수정 전 후보 |
| CF-post | nasa/CF @ `833fdbb27a26f15f2429e79392e7eb73d61abdea` | `refs/CF_833fdbb` | D22 수정 후 후보. 부모는 CF-pre [M] |
| CF-701 | nasa/CF @ `15a871e66cad7ec75e1be78989a93075ff142055` (= tag `v7.0.1`) | `refs/CF_v7.0.1_15a871e` | ENV 번들이 고정한 CF |
| bplib-ci | nasa/bplib @ `d4c6db9f0d197f78967455fbc83bf1c5e5c31c9c` | `refs/bplib_d4c6db9` | BP CI 창의 거의 전부에서 `main`이었던 값(§4.3) |
| bplib-fix | nasa/bplib @ `e7524239686ff7c88e6790e10fe8bb40e3feb30a` (PR #215 병합, 2022-11-30 20:14:16Z; 부모 `d4c6db9`) | `refs/bplib`의 git 객체(작업 트리 없음) | `d4c6db9`의 다음 `main`. CFS 빌드에서 `ut-functional`을 빼는 한 커밋(§4.3) |
| bplib-now | nasa/bplib @ `c5fb8358dd99149aa7ba463ed416d76049521a3a` | `refs/bplib_main_c5fb835` | 오늘의 `main` |
| cFE/OSAL (CI 후보) | cFE `fef4a0da`·`6cccb3f5`, OSAL `b44f4ce`·`f34759d` | `refs/cFE_*`, `refs/osal_*` | BP CI 창의 cFS `main`이 가리킨 값(§4.3) |
| PSP (CI 후보) | jphickey/PSP `techdev-iodriver` @ `bd4a2ea` | `refs/PSP_bd4a2ea_iodriver` | 창 시점의 PSP 패치 브랜치 후보(§4.3) |
| ENV | nasa/cFS @ `088b2fa828db9ff7e00733f1908e0eeb59f66ce3` (= `v7.0.1`) | `cfs_ref/cFS`(읽기 전용) | PI가 고정한 번들. 서브모듈은 §8.1 |

식별자 교차 점검 스크립트(`symcheck.py`, 전문은 §13.2)는 앱 소스에서 `CFE_*`·`OS_*` 같은 식별자를 정규식으로 뽑아 ENV(또는 후보 스택)의 소스에 같은 이름이 있는지 확인한다. **이름이 있다는 것만 확인하며, 인수 형식과 의미는 확인하지 않는다.** 빌드를 대신하지 않는다.

## 3. 공개 BP 빌드 구성의 값 (검증 항목 1)

모두 nasa/bp `b051eb0`에서 직접 읽은 값이다. **원 보고자 환경의 값이 아니다**(D22).

### 3.1 시작 스크립트 `.github/buildconfig/cfe_es_startup.scr` [F]

| 줄 | 내용 |
|---:|---|
| 1 | `CFE_LIB, tinycbor, NULL, TC_LIB, 0, 0, 0x0, 0;` |
| 2 | `CFE_LIB, bplib, NULL, BP_LIB, 0, 0, 0x0, 0;` |
| 3 | `CFE_APP, ci_lab, CI_Lab_AppMain, CI_LAB_APP, 60, 16384, 0x0, 0;` |
| 4 | `CFE_APP, to_lab, TO_LAB_AppMain, TO_LAB_APP, 70, 16384, 0x0, 0;` |
| 5 | `CFE_APP, sch_lab, SCH_Lab_AppMain, SCH_LAB_APP, 80, 16384, 0x0, 0;` |
| **6** | `CFE_APP, cf, CF_AppMain, CF, 65, 16384, 0x0, 0;` |
| **7** | `CFE_APP, bp, BP_AppMain, BP, 55, 16384, 0x0, 0;` |

- 순서: CF(6)가 BP(7)보다 먼저다. ES는 스크립트를 줄 순서대로 읽고 줄마다 `CFE_ES_AppCreate`를 호출한다(ENV cFE `cfe_es_apps.c` L313·L427).
- 우선순위는 5번째 필드(`TokenList[4]`), 스택 크기는 6번째 필드(`TokenList[5]`)다(ENV `cfe_es_apps.c` L393–L405). 값: CF 65, BP 55, 스택은 모두 16384.
- 앱 이름 필드가 `CF`이므로 CF 표의 등록 이름 `CF.config_table`과 맞는다(§5.4). ci/to/sch의 앱 이름은 `*_LAB_APP`이고 진입점은 `CI_Lab_AppMain`·`TO_LAB_AppMain`·`SCH_Lab_AppMain`이다. ENV의 앱은 이름 `CI_LAB`·`TO_LAB`·`SCH_LAB`, 진입점 `CI_LAB_AppMain`·`TO_LAB_AppMain`·`SCH_LAB_AppMain`을 쓴다(ENV `sample_defs/generate_startup.cmake`) — 이식 시 이름 대응이 필요하다(§8.4 DV-04).

### 3.2 CF 설정 표 `.github/buildconfig/{tx,rx}/tables/cf_def_config.c` [F]

- semaphore 이름: **`BP0_tsem`(L40, 채널 0)**, **`BP1_tsem`(L54, 채널 1)**. L54의 주석은 "throttle sem for channel 2, empty string means no throttle"로, 채널 번호 표기가 어긋나 있다(원문 그대로).
- tx와 rx의 차이는 L27 한 줄이다: tx `1, /* temp local id */`, rx `2` [M: `diff`].
- 표 정의: `CFE_TBL_FILEDEF(CF_config_table, CF.config_table, CF config table, cf_def_config.tbl)`(L59). 포함 헤더는 `cf_tbldefs.h`(L22).
- 채널 0: 입력 MID `0x182a`(L36), 출력 MID `0x082a`(L37). 채널 1: `0x183a`(L50), `0x083a`(L51). 채널당 pipe depth 16(L38, L52).
- BP 흐름 표와의 연결: `bp_flowtable.c`의 Flow 0("CFDP")이 `PktTbl = {0x082a,…}`(L58)와 `RecvStreamId = 0x182a`(L59)를 쓴다. 즉 BP의 흐름 0은 CF 채널 0의 출력·입력 MID와 연결된다. 채널 1(`BP1_tsem`)에 대응하는 BP 흐름은 이 표에 없다 [F, 흐름 배열에 Flow 0뿐].

### 3.3 BP 쪽 이름·개수·초기 카운트 [F]

| 항목 | 값 | 위치 |
|---|---|---|
| provider의 기본 이름 | `CF_CHANNEL_0_SEM_NAME "BP0_tsem"`, `CF_CHANNEL_1_SEM_NAME "BP1_tsem"` | `fsw/custom/bp_semcfg.c` L43–L49 |
| 이름 배열 | `BP_Throttles[BP_NUM_SEM_THROTTLES]` = 2개 | 같은 파일 L68–L70 |
| 개수 | `BP_NUM_SEM_THROTTLES 2` | `fsw/platform_inc/bp_platform_cfg.h` L29(주석 L26–L28). `bp_cfg.h` L25가 이 헤더를 포함하고, 없으면 L191–L192의 기본값 0 |
| 초기 카운트(최대값) | `BP_CF_SEM_MAX_COUNT = BP_APP_READ_LIMIT = 10` | `bp_semcfg.c` L37, `bp_cfg.h` L86–L88 |
| MsgId 기본값 | `CF_SPACE_TO_GND_PDU_MID0/1 = CFE_SB_MSGID_RESERVED` | `bp_semcfg.c` L51–L57 |

- MsgId 기본값은 BP 저장소 안의 다른 곳에서 재정의되지 않는다([M] `git grep`: `bp_semcfg.c`에만 나옴). 이것이 초기화 이후 데이터 경로의 throttle 동작(`accumulate_throttling`/`disperse_throttling`, `bp_flow.c` L149–L191)에 주는 영향은 [U]이며, 초기화 순서(C1) 범위 밖이다.
- BP는 `OS_CountSemGetIdByName`으로 먼저 찾는다(`bp_flow.c` L126–L127). **찾으면 만들지 않고** 정보 이벤트 `BP_SEM_INFO_EID`(=15, `bp_events.h` L42) "Found semaphore: %s"를 낸다(L129–L130). 못 찾으면 `OS_CountSemCreate(&Sem, Name, Max, 0)`로 만든다(L131–L133). 성공하면 **같은 이벤트 ID 15**로 "Created semaphore: %s"(L135), 실패하면 오류 이벤트 `BP_SEM_ERR_EID`(=14, L41) "Failed to create semaphore: %s"(L140). 따라서 이벤트 15만으로는 이 provider가 객체를 만들었는지 알 수 없다(§9.1 P-4). 이미 있는 객체를 찾는 경로는 provider의 재시작이나 재적재 같은 경우에 의미가 있고 [I], 이 경로는 C1의 첫 기동(객체 없음)에서는 일어나지 않을 것으로 기대한다 [I].

### 3.4 빌드 목표·앱 목록 `.github/buildconfig/targets.cmake` [F]

- `MISSION_NAME "BpTestWorkflow"`, `SPACECRAFT_ID 0xb9`(L8–L9).
- CPU 두 개: `MISSION_CPUNAMES tx rx`(L12), `tx_PROCESSORID 1`·`rx_PROCESSORID 2`(L19, L23), 둘 다 `PLATFORM default`(L21, L25).
- 앱: `MISSION_GLOBAL_APPLIST cf bp bplib ci_lab to_lab sch_lab`(L16). PSP 모듈: `iodriver unsock_intf`(L17, L20, L24).
- 두 CPU로 파일 전송을 시험하는 것이 워크플로의 목적이다(커밋 메시지, `build-run-bp-common.yml` L168–L212). C1의 초기화 순서 관찰에는 한 인스턴스면 충분할 수 있으나 그 선택은 레퍼런스가 정하지 않는다(§10 C1B-U10).

### 3.5 기타 빌드 구성 파일 [F]

- `extra_install.cmake`: sch_lab 표와 BP 흐름 표가 CF·BP의 include 경로를 보게 하고(L3–L9), `/usr/local/lib/libtinycbor.so`를 `tinycbor.so`로 설치하며(L13–L18), `${MISSION_DEFS}/cfe_es_startup.scr`를 설치한다(L19–L22). `tx/install_custom.cmake`·`rx/install_custom.cmake`는 이 파일을 include만 한다.
- `bp_msgids_extra.h`: `CFE_MISSION_BP_CMD_MSG 18`, `SEND_HK 19`, `WAKEUP 20`, `HK_TLM 17`, `FLOW_HK_TLM 18`. BP의 MID는 `CFE_PLATFORM_CMD_MID_BASE + 18` 형태이며(`bp_msgids.h` L31·L36·L41·L46·L51), CI 시기 cFE 샘플의 기준값은 `0x1800`/`0x0800`이다(CI 후보 cFE `cpu1_msgids.h` L59, L71). 따라서 `BP_CMD_MID = 0x1812`이며 워크플로의 `cmdUtil -I0x1812`와 일치한다 [M: 합산].
- `tables/to_lab_sub.c`는 `TO_LAB_APP.TO_LAB_Subs`, `tables/sch_lab_table.c`는 CF·BP의 MID 헤더(`cf_msgids.h`, `bp_msgids.h`)를 쓴다.

### 3.6 우선순위 값의 방향과 적용 여부

- OSAL의 우선순위는 **0이 가장 높고 255가 가장 낮다**("VxWorks style with zero being the highest"). ENV OSAL `os-impl-tasks.c`의 `OS_PriorityRemap` L78–L109(주석 L97–L98), CI 후보 OSAL `b44f4ce` 같은 파일 같은 함수 L62부터(주석 L81–L82) [F]. 따라서 설정상 **BP(55)가 CF(65)보다 우선순위가 높다**.
- ENV에서는 이 우선순위가 **스케줄링에 적용되지 않는다**: 일반 사용자 실행에서 `pthread_setschedparam`이 `EPERM`으로 실패하고 모든 스레드가 `SCHED_OTHER`이다([ENV CONDITIONS.md](../env/CONDITIONS.md) E4, 관측 근거 `run07` 스레드 표) [F, 저장소 기록].
- BP CI는 같은 샘플 `native_osconfig.cmake`(`OSAL_CONFIG_DEBUG_PERMISSIVE_MODE TRUE`, L39)를 복사한다(`build-run-bp-common.yml` L121; CI 후보 cFE에서 L39 확인 [F]). 러너에서 어떤 사용자로 돌았는지·실시간 우선순위가 적용됐는지는 기록이 없다 → `unspecified_by_reference` (§10 C1B-U04).

### 3.7 결정문 §3.2 표의 재확인

| 결정문 항목 | 재확인 | 비고 |
|---|---|---|
| semaphore 이름 `BP0_tsem`·`BP1_tsem` | 일치 [F] | CF 표 L40·L54, BP 기본값 L43–L49 |
| 시작 스크립트 순서 CF 다음 BP | 일치 [F] | L6·L7 |
| 설정 우선순위 CF 65, BP 55 | 일치 [F] | L6·L7. 방향은 §3.6 |
| 수정 전후 `281a941` → `833fdbb` | 일치 [M] | 부모-자식 관계, 변경 3개 파일(§7.2) |

## 4. BP CI가 쓰는 버전과 움직이는 참조 (검증 항목 2)

CI 실행 로그는 얻지 못했다. 아래는 **워크플로 파일이 가리키는 참조**([F])와 **그 참조가 CI 창에서 가리켰을 값의 추정**([I])이다.

### 4.1 워크플로 구조 [F]

| 파일(@`b051eb0`) | 트리거 | 하는 일 |
|---|---|---|
| `build-run-bp-common.yml` | `workflow_call` | 빌드와 실행의 본체. 입력 기본값은 L6–L37 |
| `build-run-bp-pullreq.yml` | `pull_request` | common을 `upstream-cf-repo: jphickey/cf`, `upstream-cf-ref: fix-184-throttle-sem-race`로 호출(L12–L14). 주석: "For now this needs to run with a patch to CF, until the patch gets merged into main line."(L6–L7) |
| `build-run-bp-dispatch.yml` | `workflow_dispatch` | 모든 입력의 기본값이 `main`(CF는 `nasa/cf`, L22–L29) |
| `static-analysis.yml` | PR·수동 | `nasa/cFS/.github/workflows/static-analysis.yml@main` 호출. 빌드와 무관 |
| `actions/setup-cfs/action.yml` | composite | cFS 번들을 서브모듈 포함으로 체크아웃(L18–L24) 후 PSP에 `jphickey/PSP`의 `techdev-iodriver`를 가져와 병합(L26–L31) |

- **같은 BP 커밋 안에서 두 진입점이 CF를 다르게 가리킨다.** 수동 실행 기본값은 `nasa/cf` `main`, PR 워크플로는 수정 브랜치이다. 이것이 D22 쌍(`281a941` → `833fdbb`)의 공개 근거이다. 단 **PR 워크플로의 주석은 "패치가 필요하다"고만 말하고 어떤 실패였는지는 적지 않는다**. 패치 없이 실패한 실행의 로그는 얻지 못했다. 그러므로 "수정 전 CF에서 BP CI가 실패했다"는 말은 이 문서에서 사실로 쓰지 않는다 [U].
- PR 워크플로 하에서 BP 자신의 소스는 `actions/checkout@v3`의 기본값(PR 병합 참조)이다. `b051eb0`는 PR #26의 머리였고 병합 `e871ad9`의 두 번째 부모이며 첫 번째 부모는 `0f48606`이다 [M: `git log -1 e871ad9`]. 따라서 병합 참조의 트리는 `b051eb0`와 같다고 본다 [I, 기준 브랜치가 그 사이 움직이지 않았음이 병합 부모로 확인됨].

### 4.2 구성요소별 참조와 성격

| 구성요소 | CI가 지정한 참조(근거) | 성격 | 오늘 해석되는 값 | CI 창의 값(추정 [I]) |
|---|---|---|---|---|
| cFS 번들 | `nasa/cFS` `main`(common L6–L13) | **움직임** | `088b2fa`(= `v7.0.1`, 2026-05-13) [M] | `1df262a`(11-17 21:23:45Z부터) 또는 `1b0b338`(11-28 13:58:55Z부터). 주간 캐시로 `30102b5`도 가능(§4.3) |
| cFE · OSAL · PSP | 번들의 서브모듈 핀(`submodules: true`, setup-cfs L21) | 번들 커밋에 종속 | — | §4.3 표 |
| PSP 패치 | `jphickey/PSP` `techdev-iodriver`를 번들 `psp/`에 병합(setup-cfs L30–L31) | **움직임** | `7c29c50`(2024-02-01) [M: `git ls-remote`] | 창 시점 후보 `bd4a2ea`(§4.3) |
| CF | PR: `jphickey/cf@fix-184-throttle-sem-race` / 수동: `nasa/cf@main` | **움직임**(전자는 삭제됨) | 전자: 브랜치 없음 [M: `git ls-remote`에 `184`·`throttle` 일치 항목 없음]. 후자: `15a871e` [M] | 전자 `833fdbb`(추정), 후자 `281a941` |
| BP | PR 병합 참조 (`actions/checkout@v3`) | 커밋에 고정 | `main`은 `c9cc1dd`(2026-03-20)로 달라짐 [M] | `b051eb0` |
| bplib | `nasa/bplib` `main`(common L14–L21) | **움직임** | `c5fb835`(2026-03-20) [M] | 후보 `d4c6db9`. 창의 끝 5분 안에 `e7524239`·`314384e1`로 옮겨 감(§4.3). 어느 쪽을 실행이 썼는지는 증명되지 않음 |
| tinycbor | `intel/tinycbor` `v0.6.0`(L30–L37) | 태그 | 주석 태그 → 커밋 `d393c16f3eb30d0c47e6f9d92db62272f0ec4dc7` [M: `git ls-remote`] | 같음(태그가 옮겨지지 않았다면) |
| 러너 | `runs-on: ubuntu-22.04`(L51, L146) | **움직임**(이미지 갱신) | — | [U] |
| 액션 | `actions/checkout@v3`, `actions/cache@v3`, `actions/upload-artifact@v3`, `download-artifact@v3` | **움직임**(주 버전 태그) | — | [U] |
| 번들 캐시 | 키 = `setup-cfs/action.yml`의 md5 + 해당 ISO 연-주(L73–L85); 적중하면 설정 단계를 건너뜀(L94–L100) | 주 단위로 갱신 | — | 같은 ISO 주에 만든 번들을 재사용했을 수 있음 |
| 빌드 형식 | `cmake -DCMAKE_BUILD_TYPE=Release -DSIMULATION=native -DCMAKE_INSTALL_PREFIX=/exe -DMISSIONCONFIG=bpxfer -DMISSION_DEFS=$PWD/bpxfer_defs -B build -S cfs-bundle/cfe`(L127), `make … -j2 mission-install`(L131) | 고정 | — | — |
| 실행 | `./core-tx`·`./core-rx`를 백그라운드로(L168–L174), `sleep 5` 후 "CFE_ES_Main entering OPERATIONAL state"를 grep(L176–L177) | 고정 | — | — |

### 4.3 CI 창의 해석 [I]

- 창: BP 커밋 시각 `2022-11-18T15:30:29Z`부터 PR #26 병합 `2022-11-30T20:19:21Z`까지 [M: 시각을 UTC로 환산].
- **cFS `main`**(첫 번째 부모만 따라간 병합 목록, [M]):

| cFS 커밋 | 시각(UTC 환산) | cFE | OSAL | PSP |
|---|---|---|---|---|
| `30102b5` | 2022-11-14 14:07:04Z | `6cccb3f5` | `e98bb23a` | `a260e9cc` |
| `1df262a` | 2022-11-17 21:23:45Z | `6cccb3f5` | `f34759d6` | `a260e9cc` |
| `1b0b338` | 2022-11-28 13:58:55Z | `fef4a0da` | `b44f4ce7` | `a260e9cc` |

  서브모듈 SHA는 각 cFS 커밋의 트리에서 읽었다 [F, `git ls-tree`]. 창의 시작에서는 `1df262a`, 끝에서는 `1b0b338`이다. 같은 ISO 주(2022-W46)에 캐시된 번들이 있으면 `30102b5`가 재사용될 수 있다. 어느 것이었는지는 기록이 없다. `1b0b338`의 번들에는 CF·BP·bplib가 없다(앱 서브모듈은 `ci_lab`·`sample_app`·`sch_lab`·`to_lab`뿐, `.gitmodules` [F]).
- **bplib `main`** [M: `git log --first-parent`, 커밋 시각을 UTC로 환산]:

| bplib 커밋 | 시각(UTC) | 내용 |
|---|---|---|
| `d4c6db9` | 2022-11-16 19:07:35Z | PR #213 병합. 창의 시작(11-18 15:30:29Z)에서의 `main` |
| `e7524239` | 2022-11-30 20:14:16Z | PR #215 병합 "Fix #214, resolve build error with CFS and tests". 부모 `d4c6db9` |
| `314384e1` | 2022-11-30 20:14:58Z | PR #220 병합. 변경은 `v7/ut-coverage/test_bplib_v7.c`·`test_bplib_v7_codec.c` 두 파일(+273/−281) |
| (BP PR #26 병합 `e871ad9`) | 2022-11-30 20:19:21Z | 창의 끝 |
| `edd1306d` | 2022-12-01 00:47:12Z | 창 밖 |

- **`e7524239`의 변경** [M: `git diff d4c6db9 e7524239`]: `CMakeLists.txt` 한 파일(+6/−1). `if (BPLIB_ENABLE_UNIT_TESTS)` 안의 `add_subdirectory(ut-functional)`을 `if (NOT IS_CFS_ARCH_BUILD)`로 감쌌고, 주석은 "BPLib Sanity checks are only for standalone builds, sanity checks for CFE/CFS builds should be part of the BP app"이다. PR의 커밋 메시지(`88815b18`, 2022-11-22): "Do not build the sanity checks when part of a CFS build, as this does not link properly due to the different linking environment. The sanity checks only work in a standalone bplib build." [F]
- **`d4c6db9`의 분기** [F: `CMakeLists.txt` L36–L47, L210–L213]: 변수 `ENABLE_UNIT_TESTS`가 **정의돼 있으면** `BPLIB_DEFAULT_BUILD_UNIT_TESTS`가 그 값을 물려받고(L36–L37), 정의돼 있지 않고 CFS 아키텍처 빌드(`IS_CFS_ARCH_BUILD`, cFE의 최상위 `CMakeLists.txt`가 설정)이면 OFF이다(L38–L41). 이 기본값이 옵션 `BPLIB_ENABLE_UNIT_TESTS`의 초기값이 된다(L47). 그것이 켜지면 `ut-stubs`와 **`ut-functional`**이 추가된다(L210–L213). `d4c6db9`에는 `IS_CFS_ARCH_BUILD` 보호가 없다.
- **함의** [I]: ENV의 cmake 호출은 `-DENABLE_UNIT_TESTS=TRUE`를 포함한다([env/ENV.md](../env/ENV.md) L132, [env/CONDITIONS.md](../env/CONDITIONS.md) C3). `d4c6db9`에서 이것은 CFS 빌드에 `ut-functional`을 추가하며, bplib 자신의 커밋 메시지에 따르면 그것은 CFS 빌드에서 링크되지 않는다. 따라서 **`d4c6db9` + ENV 플래그(켬)의 조합은 빌드 실패가 기대된다**. 빌드로 확인하지 않았다 [U]. BP CI의 cmake 호출(`build-run-bp-common.yml` L127)은 `ENABLE_UNIT_TESTS`를 지정하지 않으므로(L36의 분기가 거짓, L40–L41에서 OFF) 이 경로를 열지 않는다 [F, 소스 읽기. `IS_CFS_ARCH_BUILD`가 bplib의 CMake 처리 전에 설정돼 있다는 점은 cFE 최상위 `CMakeLists.txt` 읽기에 근거한다 I].
- **창의 해석** [I, 증명되지 않음]: `d4c6db9`는 창의 거의 전부(약 12일 중 마지막 약 5분을 뺀 구간)에서 `main`이었다. 그러나 워크플로는 실행 시점의 `main`을 가져오므로, 마지막 PR 실행이 `d4c6db9`를 썼다는 것은 **실행 시각·재실행 기록 없이는 증명되지 않는 추론**이다. 11-30 20:14Z 이후에 실행(또는 재실행)됐다면 `e7524239` 또는 `314384e1`을 썼을 것이다. 그래서 이 문서는 `d4c6db9`를 후보 하나로만 쓰고, `e7524239`를 두 번째 후보로 둔다. 두 후보 사이에서 C1의 코드 경로(`bp`·`cf`·OSAL)는 바뀌지 않는다: `e7524239`의 변경은 단위 시험 대상 빌드 분기 하나이다. CFS 빌드에서 `ut-functional`을 켜지 않는 한(BP CI 방식) 두 후보의 bplib 산출물이 같은지는 비교하지 않았다 [U].
- **nasa/CF `main`**: 첫 번째 부모 목록에서 `281a941`(2022-10-18) 다음 `32a8b03`(2022-12-01 19:35:01Z)이며, `281a941..32a8b03`은 README 한 줄 차이뿐이다 [M]. 즉 수동 실행 기본값은 창 내내 수정 전 코드였다. 수정 병합 `2ffbecc`는 그 22초 뒤(19:35:23Z)이다 [M].
- **PSP 패치 브랜치**: 브랜치 이력에서 `bd4a2ea`(2022-10-18 09:44:31 -0400) 다음 커밋은 `7dc1668`(2023-01-13)이다 [M]. 강제 푸시가 없었다면 창에서 `bd4a2ea`이다 [I]. `bd4a2ea`의 부모는 브랜치 쪽 `5572f76`과 `nasa/PSP`의 `a260e9c`(번들이 가리키는 값)이며, `a260e9c`는 `bd4a2ea`의 조상이다 [M: `git merge-base --is-ancestor`]. 그러므로 번들 `psp/`에 병합하면 fast-forward로 `bd4a2ea`가 된다 [I]. `git merge-tree`로 `a260e9c`와 `bd4a2ea`의 병합을 시험하니 충돌 없이 트리가 만들어졌다 [M].
- 이 브랜치의 첫 커밋은 `715a938`(2022-07-27, "Adds proof of concept iodriver and unix socket modules")이다 [M]. `unsock_intf` 모듈은 이 브랜치에만 있고 `nasa/PSP`에는 없다(ENV PSP의 `fsw/modules` 목록에 없음 [F]). `iodriver` 모듈은 이후 `nasa/PSP`에 편입됐다(`9764108`, 2023-04-06 "Fix #385…" [M]).

### 4.4 "main"이 낡았다는 확인

BP `b051eb0`의 `fsw/`에서 bplib 관련 식별자(`bplib_*`, `BPLIB_*`, `bp_*_t`) 30개를 뽑아 대조했다 [M, `symcheck.py`, 행 SC-7·SC-7′, 정규식은 §13.3]:

| 대조 대상 | 결과 |
|---|---|
| bplib `d4c6db9`(CI 창의 `main`) | 30개 모두 존재 |
| bplib `c5fb835`(오늘의 `main`) | **26개 없음**(예: `bplib_bind_socket`, `bplib_create_cla_intf`, `bp_handle_t`) |

따라서 BP CI를 지금 기본값 그대로 돌리면 창의 구성과 다른 코드가 조립된다. 고정 SHA 없이 "BP CI 재현"이라고 부를 수 없다.

### 4.5 결론

- cFS·cFE·OSAL·PSP·bplib의 **전체 버전 조합은 확정할 수 없다**. §4.3의 후보(bplib은 `d4c6db9`와 `e7524239`의 둘)는 추정이며 `unspecified_by_reference`로 둔다(C1B-U01, U02, U19).
- 확정할 수 있는 것은 (a) 워크플로가 가리키는 참조 이름, (b) 후보 SHA의 서브모듈 핀, (c) 오늘 해석되는 값이다.
- 이 점검에서 BP·bplib·CF·PSP 브랜치 모두 도달 가능했다. 도달하지 못한 것은 nasa/CF의 이슈 #184 댓글이다(§11).

## 5. CF `281a941`·`833fdbb`와 BP의 호환성 (검증 항목 3)

### 5.1 소스에서 읽은 값 [F]

| 항목 | CF-rep `2a024d8` | CF-pre `281a941` | CF-post `833fdbb` | CF-701 `15a871e` |
|---|---|---|---|---|
| `sem_name` 필드 | `char sem_name[OS_MAX_API_NAME]` `fsw/platform_inc/cf_tbldefs.h` L66 | 같은 선언 `fsw/src/cf_tbldefs.h` L74 | 같음 | `config/default_cf_tbldefs.h` L76 |
| 표 정의 헤더 위치 | `fsw/platform_inc/` | `fsw/src/`(커밋 `c948be4` "Fix #221") | 같음 | `config/` |
| 핸들 형식 | `uint32 sem_id`(`cf_cfdp_types.h` L390) | `osal_id_t sem_id`(L443) | 같음 | `osal_id_t sem_id`(L395) |
| 조회 호출 | `OS_CountSemGetIdByName` 1회, 실패 시 `goto err_out`(`cf_cfdp.c` L1012–L1023) | 1회, 실패 시 `break`(L991–L1002) | **루프**(L1001–L1012) | 루프(L1280–L1291) |
| 재시도 횟수 상수 | 없음 | 없음 | `CF_STARTUP_SEM_MAX_RETRIES 25`(`fsw/platform_inc/cf_platform_cfg.h` L338) | `DEFAULT_CF_STARTUP_SEM_MAX_RETRIES 25`(`fsw/inc/cf_internal_cfg.h` L173), `CF_INTERNAL_CFGVAL(x)=DEFAULT_CF_##x`(`config/default_cf_internal_cfg_values.h` L37) |
| 재시도 지연 상수 | 없음 | 없음 | `CF_STARTUP_SEM_TASK_DELAY 100`(ms, L349) | `DEFAULT_CF_STARTUP_SEM_TASK_DELAY 100`(`cf_internal_cfg.h` L185) |
| 재시도 조건 | — | — | 반환값이 `OS_ERR_NAME_NOT_FOUND`일 때만 `OS_TaskDelay` 후 재시도, 그 외 반환값은 즉시 루프 탈출(L1006–L1011) | 같음(L1285–L1290) |
| 실패 이벤트 | `CF_EID_ERR_INIT_SEM`(30), 문구 `error=0x%08x` | `CF_EID_ERR_INIT_SEM`(30, `cf_events.h` L158), 문구 `error=%ld`(L997–L999) | 같음 | `CF_INIT_SEM_ERR_EID`(30, `cf_eventids.h` L158) |
| 초기화 성공 이벤트 | `CF_EID_INF_INIT`(20) | 20(`cf_events.h` L47) | 20 | `CF_INIT_INF_EID`(20, `cf_eventids.h` L47) |
| 실패 후 처리 | `goto err_out`, `engine.enabled` 건너뜀(L1050–L1053), `CF_Init`→`APP_ERROR` | `break` 후 `if (ret == CFE_SUCCESS) enabled = 1; return ret;`(L1029–L1034); `CF_Init`이 상태를 반환(`cf_app.c` L250–L253); `CF_AppMain`이 `APP_ERROR`로 설정(L330–L334), `CFE_ES_RunLoop` 루프를 돌지 않고(L338), `CFE_ES_ExitApp`(L366) | 같음 | `CF_AppInit`(L208), `CF_AppMain`(L284–L295) |
| 시작 동기화 호출 | 없음 | 없음 | 없음 | 없음 [M: `grep CFE_ES_WaitFor*` 결과 없음, 네 버전 모두] |
| `CF_CFDP_InitEngine`의 다른 호출 지점(엔진 활성화 명령) | `CF_CmdEnableEngine` → `CF_CFDP_InitEngine`(`cf_cmd.c` L1063) | L1119 | L1119 | L1237 |

- **`2a024d8` 대 `281a941`의 실패 처리 방식 차이**(`goto` → `break`+상태 반환)는 이슈 #225·#285의 `goto` 제거 작업의 결과로 보이나 그 인과는 확인하지 않았다 [U]. 의미는 같다: 조회 실패는 `ret`에 남아 초기화 실패로 반환된다(§7.1).
- `281a941` ↔ `833fdbb`: `cf_app.c`(1–400행), `cf_tbldefs.h`, `cf_def_config.c`, `cf_cfdp_sbintf.c`(60–130행)는 **차이가 없다** [M: `diff`]. 변경은 `cf_platform_cfg.h`(상수 2개, +22/−0), `cf_cfdp.c`(루프, +21/−2), `unit-test/cf_cfdp_tests.c`(+19/−1)이다 [M: `git diff --numstat`](§7.2).
- 사용 지점: `CF_CFDP_MsgOutGet`이 `OS_ObjectIdDefined(c->sem_id)`이면 `OS_CountSemTimedWait(c->sem_id, 0)`을 호출하고 실패하면 메시지 버퍼를 할당하지 않는다(`cf_cfdp_sbintf.c` L91–L104 @`281a941`, `833fdbb` 동일 [M]).

### 5.2 표 형식 호환

BP의 `tx` 표 값을 `281a941`의 구조체 필드에 한 칸씩 대응시켰다 [F: BP 표 L24–L58, CF-pre `fsw/src/cf_tbldefs.h` L55–L94].

| BP 표의 값(채널 0) | 줄 | `281a941` 필드 | 일치 |
|---|---:|---|---|
| `5` | L29 | `max_outgoing_messages_per_wakeup` | ✓ |
| `5` | L30 | `rx_max_messages_per_wakeup` | ✓ |
| `3` | L31 | `ack_timer_s` | ✓ (채널 안) |
| `3` | L32 | `nak_timer_s` | ✓ |
| `30` | L33 | `inactivity_timer_s` | ✓ |
| `4` | L34 | `ack_limit` | ✓ |
| `4` | L35 | `nak_limit` | ✓ |
| `0x182a` | L36 | `mid_input` | ✓ |
| `0x082a` | L37 | `mid_output` | ✓ |
| `16` | L38 | `pipe_depth_input` | ✓ |
| `{{5, 25, CF_CFDP_CLASS_2, 23, "/cf/poll_dir", "./poll_dir", 0}, {0}×4}` | L39 | `polldir[5]` (`CF_MAX_POLLING_DIR_PER_CHAN (5)`, `cf_platform_cfg.h` L217) | ✓ |
| `"BP0_tsem"` | L40 | `sem_name` | ✓ |
| `1` | L41 | `dequeue_enabled` | ✓ |
| `480`, `"/cf/tmp"` | L56–L57 | `outgoing_file_chunk_size`, `tmp_dir` (표 최상위) | ✓ |

- 최상위 필드(`ticks_per_second`, `rx_crc_calc_bytes_per_wakeup`, `local_eid`)도 순서가 일치한다(L25–L27). `CF_NUM_CHANNELS`는 2(`cf_platform_cfg.h` L104)로 BP 표의 채널 두 개와 맞다.
- `2a024d8`의 구조체에서는 타이머 3개와 한계 2개가 **채널 밖, 표 끝**에 있고(`cf_tbldefs.h` L84–L89), 채널 필드는 `max_out, rx_max, mid_input, mid_output, pipe_depth, polldir, sem_name, dequeue`이다(L56–L67). BP 표를 그대로 넣으면 세 번째 필드부터 값이 한 칸씩 어긋난다(`3`→`mid_input` 등) [M: 필드 대조]. 따라서 **원 보고의 `2a024d8`과 공개 BP 표를 섞지 않는다**(결정문 §3.2). 커밋 `76d5429`("Fix #120 #256, Move timers into channel config")가 이 변경이다 [M: `git log`].
- `CFE_TBL_FILEDEF(CF_config_table, CF.config_table, …)`의 표 이름은 CF가 등록하는 이름과 맞다: CF는 `CFE_TBL_Register(..., CF_CONFIG_TABLE_NAME="config_table", ...)`(`cf_app.c` L155; `cf_platform_cfg.h` L268)이고 앱 이름이 `CF`(§3.1)이므로 `CF.config_table`이다. 표 파일은 `/cf/cf_def_config.tbl`에서 읽는다(`cf_platform_cfg.h` L280) [F].
- `281a941`의 기본 표(`fsw/tables/cf_def_config.c`)와 BP `tx` 표를 토큰 단위로 비교하면 [M], 값이 다른 곳은 `local_eid`(25 → 1; rx는 2), 채널 0의 MID(`0x18c8, 0x08c2` → `0x182a, 0x082a`), 채널 1의 MID(`0x18c9, 0x08c3` → `0x183a, 0x083a`), `sem_name` 두 개(`""` → `"BP0_tsem"`, `"BP1_tsem"`)뿐이다. BP 표는 사용하지 않는 폴링 디렉터리 슬롯을 `{0}`로 명시했을 뿐 값은 같다. 기본 표의 `sem_name`은 `""`(L59, L78)이므로 기본 표를 쓰면 C1 경로는 실행되지 않는다(경로 비활성 음성 대조, 결정문 §3.2).
- **v7.0.1 구조체**(`CF-701`)는 채널 끝에 `move_dir[OS_MAX_PATH_LEN]`이 추가되고(`config/default_cf_tbldefs.h` L78), 표 끝에 `fail_dir`이 추가됐다(`config/default_cf_tblstruct.h` L54). BP 표(필드 수가 적음)를 C 초기화로 쓰면 추가 필드는 0으로 채워지는 형식이지만 컴파일 결과는 확인하지 않았다 [U]. 이 쌍(`833fdbb` vs v7.0.1)을 같은 표로 비교하지 않는다(§7.3).

### 5.3 BP 앱과의 호환(이름 수준 교차 점검) [M]

`symcheck.py`(전문 §13.2)로 앱 소스의 식별자가 대상 스택의 소스에 존재하는지 대조했다. 행마다 쓴 정규식과 디렉터리 인수는 §13.3에 행 ID(SC-1 ~ SC-7)로 적었다. 이 문서를 고치면서 같은 인수로 다시 돌려 아래 숫자를 확인했다(2026-10-15). 스크립트는 **앱 쪽 소스에서 `#define`된 이름을 "대상에 없는 것"에서 뺀다**.

| ID | 앱 소스 | 대조 스택 | 사용 식별자 수 | 대상에 없는 것 |
|---|---|---|---:|---|
| SC-1 | CF-pre `fsw/` (`CFE_*`·`OS_*`) | CI 후보 (cFE `fef4a0da`/`6cccb3f5`, OSAL `b44f4ce`/`f34759d`, PSP `bd4a2ea`) | 89 | 1개: `CFE_SB_MAX_PIPE_DEPTH` — **주석에서만 나옴**(`cf_platform_cfg.h` L90) |
| SC-2 | CF-post `fsw/` | 같음 | 91 | 같은 1개(주석) |
| SC-3 | CF-pre / CF-post | **ENV**(cFE `c5fb2b4d`, OSAL `d2d877a6`, PSP `c4b3b0b`) | 89 / 91 | 같은 1개(주석) |
| SC-4 | BP `fsw/` + `.github/buildconfig` (`CFE_*`·`OS_*`·`OSAL_*`) | CI 후보 | 144 | 1개 `OS_OBJECT_ID_USER` — **주석에서만 나옴**(`bp_flow.c` L42, `bp_io.c` L72). 실제 없음 0 |
| SC-5 | BP `fsw/` + `.github/buildconfig` | **ENV** | 144 | 11개: ② `OS_OBJECT_ID_USER`(주석), ③ **`CFE_PLATFORM_CMD_MID_BASE`·`CFE_PLATFORM_TLM_MID_BASE`**(CI 시기 cFE 샘플 `cpu1_msgids.h`가 정의, ENV에는 없음), ④ **`CFE_PSP_IODRIVER_*` 8개**. (`CFE_MISSION_BP_*` 5개는 BP 자신의 `bp_msgids_extra.h`가 `#define`하므로 제외했다) |
| SC-6 | bplib `d4c6db9` (`OS_*`·`CFE_*`·`OSAL_*`) | CI 후보 | 27 | 스크립트 출력은 0개. 이 중 `OSAL_LOCALTIME_SECS_AT_2000`은 대상 스택에 없고 bplib 자신이 정의(`os/src/osal.c` L51)하므로 스크립트가 뺀다 |
| SC-7 | BP → bplib `d4c6db9` (`bplib_*`·`BPLIB_*`·`bp_*_t`) | bplib `d4c6db9` | 30 | 0 |
| SC-7′ | 같은 식별자 | bplib `c5fb835`(오늘의 `main`) | 30 | **26개**(§4.4) |

- ④의 8개 이름(`CFE_PSP_IODRIVER_DIRECTION_INPUT_ONLY`, `…_OUTPUT_ONLY`, `…_PACKET_IO_READ`, `…_PACKET_IO_WRITE`, `…_SET_DIRECTION`, `…_SET_RUNNING`, `…_U32ARG`, `…_VPARG`)은 ENV PSP에 **대소문자만 다른 이름**으로 존재한다(`CFE_PSP_IODriver_Direction_INPUT_ONLY`, `CFE_PSP_IODriver_PACKET_IO_READ`, `CFE_PSP_IODriver_SET_RUNNING`, `CFE_PSP_IODriver_U32ARG` 등, ENV `psp/fsw/modules/iodriver/inc/iodriver_base.h` L61·L79·L96–L98·L113·L126, `iodriver_packet_io.h` L53–L54) [M: 대소문자 무시 대응 8/8].
- 그러나 `CFE_PSP_IODriver_Location_t`의 필드가 다르다: CI 후보 브랜치 `BoardInstance`·`ChannelNumber`(`bd4a2ea` `iodriver_base.h`) ↔ ENV `SubsystemId`·`SubchannelId`(`c4b3b0b` `iodriver_base.h` L41–L46). BP는 `PspLocation.BoardInstance`·`ChannelNumber`를 쓴다(`bp_cla_bundle_io.c` L132–L133, L184–L185) [F]. 이름 점검만으로는 이 차이가 드러나지 않는다.
- **ENV PSP에는 `unsock_intf` 모듈이 없다**(`psp/fsw/modules` 목록, 번들 전체 `grep` 결과 없음 [M]). BP의 CLA는 `BP_CLA_PSP_DRIVER_NAME`으로 이 드라이버를 찾는다(`bp_cla_bundle_io.c` L124·L176).
- 요약 [I]: 공개 통합 스택(CI 후보)에서는 이름 수준에서 맞는다. ENV 스택에서 CF는 이름 수준에서 맞고, BP는 위 ③·④와 `unsock_intf` 부재 때문에 **원문 그대로는 조립되지 않을 가능성이 높다**. 이 판단은 컴파일 결과가 아니다.
- **이 요약의 한계와 D22 순서와의 관계.** 이 점검은 이름의 존재만 본다. 따라서 "공개 통합 구성은 그대로 구성할 수 없다"는 근거는 이 문서에 **없다**. 반대로 이름 수준에서 맞는다는 결과는 D22가 먼저 요구하는 "공개 통합 구성의 버전 호환성 확인"(결정문 §3.2)을 **실행으로** 채우지 못한다. 그 확인을 P-REC(§8.5)로 할지, 이 정적 대조로 갈음할지는 PI가 정한다(§12 Q-5).

### 5.4 provider 쪽 호환에서 C1에 직접 관련된 부분

- BP의 `AppInit`에서 semaphore를 만드는 `BP_FlowInit`(`bp_app.c` L203)은 CLA 초기화(`BP_CLA_Init`, L208)보다 **먼저** 실행된다. 즉 PSP 드라이버 문제로 `BP_CLA_Init`이 실패해도 semaphore 생성 호출은 이미 끝난 뒤이다. 단 `AppInit`이 실패하면 `BP_AppMain`이 `APP_ERROR`로 앱을 종료하고(`bp_app.c` L808–L818, L839–L844), ES는 종료한 앱의 OSAL 객체 중 counting semaphore를 삭제한다(ENV `cfe_es_apps.c` L1664–L1691 `OS_OBJECT_TYPE_OS_COUNTSEM` → `OS_CountSemDelete`) [F, 코드 읽기; 동작은 미관측 [U]]. 따라서 BP 일부만 초기화되는 변형을 쓰면 semaphore가 오래 남지 않을 수 있다.
- **생성 호출 전에 provider가 스스로 실패하는 경로** [F, 2026-10-16 재독해]. `AppInit`은 `BP_FlowInit`(L203)보다 앞선 단계의 실패에서 바로 반환한다(EVS 등록·앱 ID·앱 이름·파이프·구독의 실패는 L154–L184의 `return status;`). 그중 C1 구성에서 특히 걸리는 것은 bplib이다: `BP_SetupLibrary`(L196)가 `bplib_init` 실패(L112–L116) 또는 `bplib_route_alloc_table(10, 16 << 20)` 실패(L119–L124)에서 `CFE_STATUS_EXTERNAL_RESOURCE_FAIL`을 반환하면 `AppInit`은 L199에서 반환하고, `BP_AppMain`은 `run_status`를 `APP_ERROR`로 둔 채(L808, L812–L815) 루프를 돌지 않고 종료 이벤트 "BP application terminating"(L839–L840)와 `CFE_ES_ExitApp`(L844)으로 간다. 이 경로에서는 생성 호출(P-2)이 **한 번도 일어나지 않는다**. 이 실패의 출력은 이벤트가 아니라 표준 오류의 `"%s(): bplib_init failed"`·`"%s(): bplib_route_alloc_table failed"`(L114, L122)이다. 따라서 CF의 이벤트 30만으로는 "provider가 늦었다"(A3)와 "provider가 생성 전에 실패했다"를 구분할 수 없다. §9.1은 후자를 P-E로 따로 관측한다.
- 반대로 `initialize_throttling`(`bp_flow.c` L121–L143)은 `void`이고 생성 실패를 오류 이벤트 14로만 알린 뒤 다음 이름으로 넘어간다. `BP_FlowInit`은 생성 결과와 무관하게 표 등록으로 진행한다(L524–L530) [F]. 즉 생성 실패는 BP의 초기화 실패로 이어지지 않는다.
- 이 사실은 §8.2의 provider 구현 선택지(B-1~B-4)의 비용 비교에 쓰인다. 특히 B-1·B-3에서 bplib·tinycbor 구성이 잘못되면 provider가 생성 전에 끝나므로, B7의 기본 기동에서 BP의 표준 오류와 이벤트 15의 "Created" 문구를 함께 확인해야 한다.

## 6. provider 생성 순서와 CF 조회 순서 (검증 항목 4)

### 6.1 같은 시각표 위에 놓은 두 앱의 경로

ES의 앱 생성은 줄 순서이며 앱 사이에 대기가 없다: `CFE_ES_StartApplications`(ENV `cfe_es_apps.c` L115)가 줄마다 `CFE_ES_ParseFileEntry`(L313)→`CFE_ES_AppCreate`(L427)를 호출하고, 이 파일의 `OS_TaskDelay` 호출은 L575 한 곳뿐이다 [M: `grep`]. 각 앱의 주 스레드는 `CFE_ES_TaskEntryPoint`(L613)에서 `CFE_ES_GetTaskFunction`(L559)을 부르고, 이 함수는 **작업 기록을 확인하기 전에 먼저** `OS_TaskDelay(CFE_PLATFORM_ES_STARTUP_SYNC_POLL_MSEC)`를 호출한다(L573–L575; CI 후보 cFE에서는 L510) [F]. 이 값은 ENV 기본 50 ms(`cfe_es_internal_cfg.h` L860)이고 CI 시기 샘플 설정에서도 50(`cpu1_platform_cfg.h` L1689)이다. 즉 CF와 BP 모두 스레드 생성 후 최소 이 값만큼 기다린 뒤에 앱 코드를 시작한다.

| 단계 | CF (`281a941`/`833fdbb`) | BP (`b051eb0`) |
|---:|---|---|
| 0 | ES가 스크립트 6번째 줄에서 스레드 생성 | ES가 7번째 줄에서 스레드 생성(CF 직후) |
| 1 | 공통 시작 지연(`GetTaskFunction`의 첫 `OS_TaskDelay`) | 같음 |
| 2 | `CF_AppMain`(`cf_app.c` L323) → `CF_Init`(L206) | `BP_AppMain`(`bp_app.c` L804) → `AppInit`(L132) |
| 3 | `CFE_EVS_Register`(L216) | 구조체 초기화(L146–L147), `CFE_EVS_Register`(L152), `CFE_ES_GetAppID`(L157), `CFE_ES_GetAppName`(L162) |
| 4 | 명령 파이프 생성(L223)과 구독 3개(L233–L242) | 파이프 생성(L167)과 구독 3개(L172–L184), `CFE_MSG_Init` 2회(L187–L190) |
| 5 | `CF_TableInit`(L247): `CFE_TBL_Register`(L155), `CFE_TBL_Load("/cf/cf_def_config.tbl")`(L164), `CFE_TBL_Manage`(L174) | `BP_StorageService_Init`(L193) |
| 6 | `CF_CFDP_InitEngine`(L252, `cf_cfdp.c` L952): 채널 `i`마다 `CFE_SB_CreatePipe`(L971), `CFE_SB_SubscribeLocal`(L980) | `BP_SetupLibrary`(L196): `bplib_init`(L112), `bplib_route_alloc_table(10, 16<<20)`(L119) |
| 7 | **채널 `i`의 `OS_CountSemGetIdByName(sem_name)`**(`281a941` L993; `833fdbb` L1003 루프) | `BP_FlowInit`(L203) → `initialize_throttling`(`bp_flow.c` L524→L121): 이름마다 `OS_CountSemGetIdByName`(L126) → 실패하면 **`OS_CountSemCreate`**(L132) → 이벤트 15(L135) |
| 8 | 성공이면 `CF Initialized` 이벤트 20(`cf_app.c` L258); 실패면 이벤트 30 후 `CF_Init` 오류 반환 → `CFE_ES_ExitApp(APP_ERROR)`(L330–L334, L366) | 이어서 흐름 표 등록(`CFE_TBL_Register`, `bp_flow.c` L530), `BP_CLA_Init`(`bp_app.c` L208), 그 뒤 첫 `CFE_ES_RunLoop`(L818) |

### 6.2 순서를 정하는 장치와 정하지 않는 장치

| 장치 | 읽은 내용 | C1의 순서를 보장하는가 |
|---|---|---|
| 스크립트 줄 순서 | 스레드 *생성* 순서만 정한다(§6.1) | 아니다 [I] |
| 우선순위 | BP(55) > CF(65) 방향이나(§3.6) ENV에서는 적용되지 않음 | 아니다(ENV). BP CI는 [U] |
| ES 전역 상태 대기 | `CFE_ES_Main`이 `LATE_INIT`, `RUNNING` 두 번 대기하고 각각 `CFE_PLATFORM_ES_STARTUP_SCRIPT_TIMEOUT_MSEC`(기본 1000, `cfe_es_internal_cfg.h` L879)까지, 시간 초과 시 로그만 남기고 `OPERATIONAL`로 진행(ENV `cfe_es_start.c` L210–L234) | 아니다. 이 대기는 호출한 쪽(ES 주 작업)이 기다릴 뿐 CF·BP를 막지 않는다 |
| 시작 동기화 호출 | CF 네 버전과 BP `b051eb0`의 `fsw/`에 `CFE_ES_WaitForStartupSync`·`CFE_ES_WaitForSystemState` 호출이 없다 [M: `grep`] | 아니다 |
| OSAL 객체 표 잠금 | 생성은 독점 변환, 조회는 전역 잠금으로 각 호출을 원자적으로 만들지만 호출 사이 순서는 만들지 않는다(C1.md §6, 이번에 ENV `osapi-idmap.c`에서 구조 재확인: `OS_ObjectIdConvertToken` L388–L530, `OS_ObjectIdFindNextMatch` L544–L576) | 아니다 [I] |
| 재시도(`833fdbb`) | 25회까지 `OS_ERR_NAME_NOT_FOUND`를 재시도 | 순서 보장이 아니라 허용 구간을 늘릴 뿐이다 |

### 6.3 해석 [I]

- 기록상 두 앱의 시작 경로는 모두 공통 시작 지연 이후 **서로 다른 길이의 선행 작업**을 거친다: CF는 표 로드(파일 읽기·`CFE_TBL_*`)와 파이프·구독을, BP는 `bplib_init`과 16 MB 캐시 할당을 포함한 초기화를 거친다. 어느 쪽이 먼저 호출에 도달하는지를 소스만으로 정할 수 없다. 이 길이를 이 문서에서 수치로 추정하지 않는다.
- CF의 조회는 채널 0, 이어서 채널 1 순서이고, BP의 생성도 이름 0, 이어서 이름 1 순서이다. 따라서 **키(이름)마다 순서쌍이 따로 있고**, 채널 0의 조회가 성공한 뒤에도 채널 1의 조회가 BP의 두 번째 생성보다 앞설 수 있다. BP는 첫 생성 직후 이벤트 전송(`CFE_EVS_SendEvent`, `bp_flow.c` L135)을 한 다음 두 번째를 만든다. 순서 판정은 키별로 기록한다(§9).
- 실패한 CF 초기화(A3)의 소스상 하한: `833fdbb`에서 첫 조회부터 오류 이벤트까지 `OS_TaskDelay(100)`이 **25번** 호출된다. 마지막 조회가 실패해도 지연이 한 번 더 호출되므로(L1006–L1011) 이름당 호출 간격의 합은 명목 2500 ms이고, 마지막(25번째) 조회는 명목 2400 ms 이후이다 [I, `OS_TaskDelay_Impl`이 `clock_nanosleep`으로 최소 요청 시간을 잔다(ENV `os-impl-tasks.c` L744–L762)]. 결정문 §3.3에 따라 **이 값을 벽시계 상한으로 쓰지 않는다**.

## 7. 버전 간 차이 (검증 항목 5)

### 7.1 원 보고의 `2a024d8` → `281a941` (C1에 관련된 변화)

전체 변화는 175개 커밋·104개 파일(순수 변경량 +9106/−14774행)이다 [M: `git rev-list --count`, `git diff --shortstat`]. C1의 경로와 BP 표 호환에 직접 걸리는 것만 골랐다.

| 변화 | 근거 [M: `git log`/`diff`] | C1에 미치는 영향 |
|---|---|---|
| 타이머·한계를 표 끝에서 **채널 안**으로 이동 | `76d5429` (2022-06-07) "Fix #120 #256, Move timers into channel config" | BP 표는 이 이후 형식(`281a941` 이상)에만 맞는다 |
| `cf_tbldefs.h`가 `fsw/platform_inc`에서 `fsw/src`로 | `c948be4` (2022-04-06) "Fix #221" | include 경로가 달라진다 |
| 표 이름(`CF_APP.config_table` → `CF.config_table`) | `399f6db` (2022-02-01) "Fix #186, CF table name and default polling disabled" [M: `git log -S`]; `2a024d8`의 `CFE_TBL_FILEDEF(..., CF_APP.config_table, ...)`(`cf_def_config.c` L61), `281a941`의 `CF.config_table`(L84) [F] | 표 이름의 앞부분은 앱 이름과 맞아야 한다. `2a024d8`의 표(`CF_APP`)는 BP 스크립트의 앱 이름 `CF`와 맞지 않는다(C1.md §10 #38의 추론과 같은 방향, 이번에는 커밋을 확인) |
| `sem_id`가 `uint32`에서 `osal_id_t`로 | `cf_cfdp_types.h` L390 → L443 [F] | 조회 호출의 인수 형식이 달라진다 |
| 조회 실패 처리: `goto err_out` → `break`+`ret` 반환 | `cf_cfdp.c` L1012–L1023, L1050–L1053 → L991–L1002, L1029–L1034 [F] | 의미는 같다(오류 반환, 엔진 비활성). 실패 경로의 줄 번호가 달라진다 |
| 기본 표의 폴링 디렉터리 비활성화 등 | `f27d7c4` (2022-08-24) "Fix #319" | C1과 무관 |
| 조회 자체(1회 호출)와 호출 전 `CreatePipe`·`SubscribeLocal` | 변화 없음 [F] | `2a024d8`과 `281a941` 모두 재시도 없음 |

### 7.2 `281a941` → `833fdbb`

`git diff --numstat 281a941 833fdbb`: 3개 파일, `cf_platform_cfg.h` +22/−0, `cf_cfdp.c` +21/−2, `unit-test/cf_cfdp_tests.c` +19/−1, 합계 62줄 추가·3줄 삭제 [M]. (`--stat`의 파일별 숫자는 추가+삭제의 합이어서 `cf_cfdp.c`가 23, `cf_cfdp_tests.c`가 20으로 나온다.) 변경의 전부:

1. `CF_STARTUP_SEM_MAX_RETRIES 25`, `CF_STARTUP_SEM_TASK_DELAY 100`(주석 포함).
2. `cf_cfdp.c`: 1회 호출을 `ret = OS_ERR_NAME_NOT_FOUND; for (j < MAX_RETRIES) { ret = OS_CountSemGetIdByName(...); if (ret != OS_ERR_NAME_NOT_FOUND) break; OS_TaskDelay(DELAY); }`로 교체. 아래의 `if (ret != OS_SUCCESS)` 오류 처리는 그대로.
3. 단위 시험 세 경우 추가(이름이 아닌 이유의 실패 → 즉시 실패, 계속 `OS_ERR_NAME_NOT_FOUND` → 오류, 한 번 `OS_ERR_NAME_NOT_FOUND` 뒤 성공 → 엔진 활성).

초기 PR 머리 `c2671e8`(공백만 다름)과 PR 타임라인은 [cases/C1.md](../cases/C1.md) F6에 있으며 이번에 다시 가져오지 않았다. 이 쌍은 **재시도 하나만 다른** 가장 작은 A/B 쌍이다. 그러므로 BP 앱·bplib·번들·미션 정의를 같게 두면 두 빌드의 차이는 위 3개 파일의 패치로 한정된다(§8.3 B2).

### 7.3 `833fdbb` → v7.0.1 (`15a871e`)

- 규모: 151개 커밋, 124개 파일, 소스 영역(`fsw`, `config`, 표)에서 66개 파일 [M].
- **재시도 블록은 문자 수준에서 같다**: 두 버전의 루프를 정규화해 비교한 결과 차이는 `CFE_EVS_SendEvent` 호출의 줄 바꿈과 이벤트 ID 이름(`CF_EID_ERR_INIT_SEM` ↔ `CF_INIT_SEM_ERR_EID`)뿐이다 [M]. 상수 값은 25·100 그대로이며 이제 `CF_INTERNAL_CFGVAL`로 접근한다(§5.1).
- 구조 변화: 표 구조체가 `config/default_cf_tbldefs.h`+`default_cf_tblstruct.h`로 분리되고 `move_dir`·`fail_dir`이 추가됐다(§5.2). MID가 `CFE_PLATFORM_CF_CMD_MIDVAL(...)`(토픽 ID 기반) 매크로로 바뀌고 `CF_CH0_RX_MID`·`CF_CH1_TX_MID` 등이 추가됐다(`config/default_cf_msgids.h` L36–L70). 기본 표는 이 매크로를 쓴다(`cf_def_config.c` L43–L44, L71–L72). `CF_CFDP_InitEngine`은 시작에서 `OS_mkdir(tmp_dir, 0)`을 수행한다(`cf_cfdp.c` L1239). 앱 초기화 함수 이름은 `CF_Init`→`CF_AppInit`이다.
- 기본 표의 `sem_name`은 `""`(`cf_def_config.c` L60, L78) — 이 구성에서 C1 경로는 실행되지 않는다(경로 비활성 음성 대조, 결정문 §3.2).
- ENV 번들의 `sample_defs/tables/to_lab_sub.c`는 `CF_CH0_TX_MID`·`CF_CH1_TX_MID`를 참조한다(L81–L82). 이 이름은 `CF-pre`/`CF-post`의 `cf_msgids.h`에 없다(그 파일은 `CF_CMD_MID`, `CF_SEND_HK_MID`, `CF_WAKE_UP_MID`, `CF_HK_TLM_MID`, `CF_EOT_TLM_MID`만 정의) [F]. 그러므로 ENV 번들의 TO_LAB 표를 그대로 쓰면 CF를 교체하는 순간 컴파일이 깨질 수 있다 [I] (§8.4 DV-12).

### 7.4 판정에 영향을 주는 차이의 한눈 표

| 쌍 | 표 형식 | 조회 횟수 | 재시도 상수 | 메시지 ID | 비교 시 주의 |
|---|---|---|---|---|---|
| `2a024d8` vs `281a941` | 다름(필드 순서·위치) | 1 / 1 | — | 숫자 고정 | BP 표와 섞지 않음 |
| `281a941` vs `833fdbb` | 같음 | **1 / 25** | — / 25, 100 ms | 같음 | 원인 변수 하나(재시도) |
| `833fdbb` vs `15a871e` | 다름(필드 추가) | 25 / 25 | 25, 100 ms 동일 | 토픽 ID 매크로 | 배포 버전 전체 차이. 한 변수의 효과로 해석하지 않음 |

## 8. D22의 적용 순서와 ENV 번들 위의 수정 전후 쌍 빌드 계획 (검증 항목 6) — 지금은 빌드하지 않는다

### 8.0 D22의 적용 순서에서 이 절의 위치

결정문 §3.2의 적용 순서 [D]: **(1)** 공개 통합 구성의 버전 호환성을 먼저 확인한다. **(2)** 그대로 구성할 수 없으면 다른 버전으로 옮긴 차이를 기록한다. 원 보고의 CF `2a024d8`과 공개 BP 표는 섞지 않는다. v7.0.1 기본 구성의 빈 이름은 경로 비활성 음성 대조로만 쓴다.

이 문서의 계획을 이 순서에 맞추면 다음과 같다.

| 단계 | 내용 | 이 문서의 위치 | 상태 |
|---|---|---|---|
| P0 | 공개 통합 구성의 버전 호환성 확인. 수행 방식(P-REC 조립·기동, 또는 PI가 승인한 대체)은 Q-5 | §8.5 | 정적 이름 대조(§5.3)만 했다. 조립은 하지 않았다. 대체의 승인은 없다 |
| B0–B8 | ENV 번들(v7.0.1) 위의 이식 빌드 | §8.3 | **P0 결과가 "그대로 구성되지 않음"으로 기록된 뒤에만** 시작한다. 또는 PI가 이 순서에 대한 예외를 승인한 뒤에(Q-5) |

두 갈래 중 어느 쪽이든 ENV 이식은 **D22의 기준 구성이 아니다**. P0이 성공하면 P0의 구성이 공개 통합 기준이고 ENV 이식은 그와 비교하는 파생이다. P0이 실패하면(실패 지점·요구·레퍼런스 기록) ENV 이식이 "다른 버전으로 옮긴" 구성이며, 결정문 §3.2의 적용 순서와 명세 App. A.4 step 2("is a derived basis") [D]에 따라 공개 코드 기반 파생 구성으로 표시한다(로드맵 §8.1의 `[제안]`과 같은 표시). PI가 예외를 승인하는 경우는 승인 사실과 그 이유를 이탈 대장에 한 줄로 더한다(DV-00).

§8.3의 단계표는 B0 앞에 P0 행을 둔다.

### 8.1 ENV의 고정 요소 [F]

| 요소 | 값 | 근거 |
|---|---|---|
| cFS 번들 | `088b2fa828db9ff7e00733f1908e0eeb59f66ce3` (`main` = `v7.0.1`) | [env/ENV.md](../env/ENV.md) L7; `cfs_ref/cFS`에서 `git log`로 재확인 [M] |
| cFE | `c5fb2b4d540bd55eb6c3707da7dd13eee679d4dd` | `git submodule status` [M] |
| OSAL | `d2d877a69cff47452bcca274b309147d48e6c16f` | 같은 명령 [M] |
| PSP | `c4b3b0b65b119e106481ad8e20976ae4d7f554e3` | 같은 명령 [M] |
| CF | `15a871e66cad7ec75e1be78989a93075ff142055` | 같은 명령 [M] |
| ci_lab · sch_lab · to_lab | `f5d36625`·`bc848cd`·`38f7312` | 같은 명령 [M] |
| 빌드 | `make native_std.prep` → `make native_std.install`, 일반 사용자, 번들의 `Makefile`·`sample_defs/` 그대로. `-DCMAKE_BUILD_TYPE=debug`, `-g`, 최적화 옵션 없음, `-std=c99 -pedantic -Wall -Werror` 등 | ENV.md L8, L132–L133 |
| 실행 | `build-native_std/exe/cpu1`에서 `./core-cpu1`, 정지는 CI의 ES POWERON 재시작(D17) | ENV.md L11, README |
| 큐 깊이 | `msg_max` 10(D19로 유지) | 결정문 §4.1 |
| 시작 스크립트 | `sample_defs/cpu1/install_custom.cmake`가 `generate_startup.cmake`를 호출해 생성. CF는 줄 8, 우선순위 70 | ENV.md L139–L158 |

서브모듈 전체 목록(25개)은 ENV의 CONDITIONS.md가 기록한다. 위 값은 `cfs_ref/cFS`에서 `git submodule status`로 다시 읽은 것이다(읽기 전용).

호스트(이번 점검에서 읽음) [M]: Ubuntu 24.04.4, gcc 13.3.0, cmake 3.28.3, CPU 4개, `pkg-config tinycbor` 없음, 사용자는 root(`id -u`=0). ENV 빌드는 일반 사용자(`ubuntu`)로 수행했고 root의 우선순위·큐 동작 차이는 [env/CONDITIONS.md](../env/CONDITIONS.md)의 D-4 행(L123)이 기록한다(ENV.md는 L16·L296에서 그 행을 가리킨다).

### 8.2 바뀌는 구성요소

| 구성요소 | ENV | C1 파생 | 바꾸는 방법 | 근거·비고 |
|---|---|---|---|---|
| cFS 번들, cFE, OSAL, PSP | 위 표 | **같음**(새 작업 트리에 같은 SHA) | 새로 클론해 서브모듈을 같은 SHA로 맞추고 `git submodule status`를 ENV와 비교 | `cfs_ref` 자체는 수정하지 않는다 |
| CF | `apps/cf` = `15a871e` | **수정 전 `281a941` / 수정 후 `833fdbb`** | 환경변수 `CFS_APP_PATH`가 가리키는 디렉터리의 `cf`가 번들의 `apps/cf`보다 먼저 선택된다(ENV cFE `mission_build.cmake` L380–L384, L397–L407: 검색 경로 앞에 `CFS_APP_PATH`를 붙이고 처음 찾은 디렉터리에서 중단) [F] | 두 빌드 트리는 CF 소스 디렉터리만 다르다. **ENV는 `CFS_APP_PATH`를 설정하지 않았고(CONDITIONS C4: 빈 값) `build_cfs.sh`는 이 환경변수가 있으면 거부한다(`REFUSED_VARS`, L142). 이 방식은 ENV 기준에서의 이탈이다 → DV-16** |
| BP | 없음 | `b051eb0` | 같은 `CFS_APP_PATH` 아래 `bp/` | BP CI도 같은 방식(common.yml L45–L46, L54–L71) |
| bplib | 없음 | `d4c6db9` 또는 `e7524239` (후보, §4.3; **Q-4**) | `CFS_APP_PATH` 아래 `bplib/` | 이름 수준 호환 확인 [M]: ENV cFE·OSAL 이름 점검에서 미해결 0건(bplib 자체 정의 1건 제외; SC-6). 단위 시험 플래그와의 조합은 DV-13 |
| tinycbor | 없음 | `v0.6.0`(커밋 `d393c16`) | 사용자 영역에 설치하고 `PKG_CONFIG_PATH`로 노출(`bplib/v7/CMakeLists.txt` L13이 `pkg_search_module(TINYCBOR tinycbor REQUIRED)`) | CI는 `/`에 복사(common.yml L111); ENV 호스트에는 없음 [M] |
| 미션 정의(`MISSION_DEFS`) | `sample_defs/` | **파생 정의 디렉터리**(ENV `sample_defs` 복사본 + BP `buildconfig` 파일 + 보조 정의) | `-DMISSION_DEFS=…` | BP CI의 복사 단계(L118–L124)는 ENV cFE에 해당 파일이 없어 그대로 쓸 수 없다(§8.4 DV-05) |
| 앱 집합·시작 스크립트 | 15개 앱, 생성 스크립트 | BP의 7줄(앱 이름 대응) | 파생 정의의 `cfe_es_startup.scr`와 `install_custom.cmake` | **PI 결정 필요**(§12 Q-1) |
| CF 표 | CF 기본 표(`""`) | BP `tx` 표(`BP0_tsem`/`BP1_tsem`) | 파생 정의의 `tx/tables/cf_def_config.c`(또는 `tables/`) — 표 소스 검색 순서는 `<defs>/<tgt>/tables/…`, `<defs>/<tgt>_…`, `<defs>/tables/…`, `<defs>/…` (ENV `global_functions.cmake` L42–L82) [F] | |
| provider 앱 | 없음 | **선택 필요**: B-1 ~ B-4 | 아래 표 | **PI 결정 필요**(§12 Q-2) |

provider 선택지(어느 것도 레퍼런스가 정하지 않는다):

| 선택 | 내용 | 얻는 것 | 잃는 것·비용 |
|---|---|---|---|
| **B-1** 공개 BP 전부 이식 | BP의 CLA·IO를 ENV PSP의 iodriver에 맞춰 이름·`Location_t` 필드를 고치고, PSP `unsock_intf`를 ENV PSP에 맞춰 이식 | 공개 BP 코드 전체(전송 포함) | 이식량이 크다. 이식 코드는 연구자 작성 코드. 이식 후 BP가 공개 코드와 동작이 같은지 별도 확인 필요 |
| **B-2** 초기화 경로만 남긴 BP 변형 | BP에서 `bplib`·CLA·IO 호출을 제외하고 `initialize_throttling`까지만 두는 최소 변형(`BP_FlowInit` 이후 흐름 등록·CLA 제외) | semaphore 생성 호출의 *위치와 인수*가 공개 BP와 같다 | 생성 호출 이전의 선행 작업(`bplib_init`, 16 MB 캐시 할당, §6.1 단계 6)이 줄어 **자연 실행의 경쟁 구간이 공개 BP와 달라진다**. BP 이후 동작은 공개 BP가 아님. 데이터 전송은 불가 |
| **B-3** 공개 통합 스택을 별도로 재구성(P-REC, §8.5) | CI 후보 스택(cFS `1df262a` 또는 `1b0b338`, PSP `bd4a2ea` 등)에서 공개 구성 그대로 | D22가 먼저 요구하는 "공개 통합의 호환성 확인"을 실행으로 얻음. P0(§8.0)에서 이 구성이 조립되면 B-3이 곧 공개 통합 기준이다 | ENV 번들이 아님(핵심 스택이 다르다). 관측기(Kairos)를 이 스택에 붙이는 비용은 이 문서가 평가하지 않았다 [U] |
| **B-4** 역할 기반 provider | `OS_CountSemCreate(name, n, 0)`을 초기화에서 호출하는 앱을 새로 작성 | 가장 가볍다 | 공개 BP가 아님. C1.md D2의 "role-derived"에 해당. 결정문 §3.2는 공개 BP 기반을 채택했으므로 *기본안이 아니다* |

C1의 A1–A3(§9)는 초기화 시점의 생성·조회 관계만 필요로 하므로 B-2도 그 범위에서는 공개 BP와 같은 생성 호출을 갖는다 [I]. 다만 B-2에서는 생성 호출까지 걸리는 선행 작업이 달라지므로, 보류 없이 얻는 자연 순서의 분포는 공개 BP의 것이 아니다. B-2가 "공개 BP 기반"인지는 PI가 정한다.

### 8.3 단계별 절차(계획) — 단계마다 남길 기록을 함께 적는다

| 단계 | 하는 일 | 완료 시 남길 기록 |
|---|---|---|
| **P0** | **D22 순서의 첫 단계**(§8.0, §8.5). Q-5에 따라 P-REC(공개 통합 구성의 조립·기동) 또는 PI가 승인한 대체를 수행한다. 아래 B0–B8은 P0의 결과가 "그대로 구성되지 않음"으로 기록되었거나 PI가 예외를 승인한 뒤에 시작한다 | P-REC의 조립·기동 기록 또는 실패 지점(요구와 레퍼런스). 대체이면 PI 승인의 출처 |
| B0 | 호스트·환경변수·도구 버전 기록. `build_cfs.sh`의 거부 목록(`REFUSED_VARS`, L142–L146)을 파생 빌드에 적용했을 때 **걸리는 변수를 명시한다**: 파생 빌드는 `CFS_APP_PATH`를 설정하므로(DV-16) 이 점검에 걸린다. `ENABLE_UNIT_TESTS`·`MISSIONCONFIG`·`SIMULATION`은 ENV와 같이 환경변수가 아닌 `-D` 옵션으로만 준다. 나머지 거부 변수는 설정하지 않는다. `cfs_ref` 트리 해시를 작업 전·후에 기록해 변경 없음을 확인 | 호스트 기록, 거부 목록 중 설정한 변수와 설정한 방식(환경변수 또는 동등한 캐시 항목), `cfs_ref` 전후 해시 |
| B1 | 새 작업 공간에 번들 `088b2fa`를 클론하고 서브모듈을 ENV와 같은 SHA로 맞춘다 | `git submodule status` 전체와 ENV 목록과의 `diff`가 비어 있음 |
| B2 | `CFS_APP_PATH` 디렉터리 두 개(`pre`, `post`)를 만든다. 각각 `cf`(`281a941`/`833fdbb`), `bp`(`b051eb0`+필요 시 이식 패치), `bplib`(커밋은 `unspecified_by_reference`: 후보 `d4c6db9`·`e7524239` 중 Q-4가 정한다. **Q-4 전에는 B2를 시작하지 않는다**). 두 디렉터리의 차이는 `cf`뿐이어야 한다 | `diff -r`로 `pre`와 `post`의 차이가 §7.2의 3개 파일 패치와 같음 |
| B3 | tinycbor `v0.6.0`을 사용자 영역에 빌드·설치(일반 사용자 권한, `/` 미사용) | 설치 파일 해시, `pkg-config` 출력 |
| B4 | 파생 `MISSION_DEFS` 작성: `sample_defs` 복사본을 바탕으로 §8.4의 DV 항목을 적용한 파일만 덮어쓴다. **원본 `sample_defs`는 수정하지 않는다** | 덮어쓴 파일 목록과 각 파일의 `diff` |
| B5 | `cmake` 구성을 두 번 수행(`pre`, `post`). ENV와 같은 플래그를 유지하되 차이는 DV로 기록한다. **단위 시험 플래그의 기본은 "끔"이다**: `-DENABLE_UNIT_TESTS=TRUE`를 주지 않는다. 근거는 BP CI의 cmake 호출이 이 변수를 지정하지 않는다는 사실(`build-run-bp-common.yml` L127)과, ENV 플래그와 고정 bplib `d4c6db9`의 조합이 빌드 실패로 기대된다는 추론(§4.3)이다. 이것은 ENV 기준에서의 **기록된 이탈**이다(DV-13). Q-4에서 PI가 "켬 + `e7524239`" 등을 고르면 그에 따른다 | 두 구성의 `CMakeCache.txt`를 **정규화해** 비교한다: 각 캐시에서 자기 빌드 디렉터리의 절대 경로를 `<BUILD>`로, 자기 `CFS_APP_PATH` 디렉터리의 절대 경로를 `<APPS>`로 바꾼 뒤 `diff`가 비어야 한다. 정규화 전에는 경로 항목이 달라지는 것이 정상이다: 빌드 디렉터리를 값으로 갖는 항목(ENV 캐시에서 주석 줄을 빼고 18개: 예 `CMAKE_CACHEFILE_DIR` L488, `MISSION_BINARY_DIR` L310, `CFE_BINARY_DIR` L27, `OSAL_BINARY_DIR` L331, `*_DOCS_BINARY_DIR`, `CMAKE_FIND_PACKAGE_REDIRECTS_DIR` L123), `CFS_APP_PATH` 자체, 앱 소스 경로를 값으로 갖는 항목(ENV 캐시의 예 `CF_DOCS_SOURCE_DIR` L60) [M: ENV `build-native_std/CMakeCache.txt`를 `grep`, 2026-10-16]. 파생 빌드에 어떤 항목이 생길지는 구성 전에는 모른다 [U]. 정규화 뒤 남는 차이는 모두 설명하고 기록한다. `ENABLE_UNIT_TESTS`의 값과 bplib 커밋을 기록 |
| B6 | 설치 후 산출물 해시: `core-*`, `cf.so`, `bp.so`, `bplib.so`, 표 파일(`cf_def_config.tbl` 등), 시작 스크립트 | 해시표. `pre`/`post`에서 `cf.so`와 `cf_def_config.tbl`(같아야 함) 외의 차이 설명 |
| B7 | 관측 없이 기본 기동: ES POWERON 재시작으로 종료하는 ENV의 절차로 "OPERATIONAL" 도달과 CF·BP의 초기화 이벤트(CF 20 또는 30, BP 14 또는 15)를 콘솔 기록에서 확인. **BP의 이벤트 15는 두 가지 문구를 가진다**("Found semaphore: %s", "Created semaphore: %s"; `bp_flow.c` L129–L130, L135). 생성의 증거는 이벤트 ID가 아니라 문구와 P-3(§9.1)으로 가른다. **BP의 표준 오류도 함께 남긴다**: `bplib_init failed`·`bplib_route_alloc_table failed`(P-E, §5.4)가 있으면 provider는 생성 호출 전에 끝났으며 그 기동은 C1 조건 확인이 아니라 구성 오류로 기록한다 | 콘솔 기록 원본. 이 단계는 *조건 확인*이며 빈도·확률 자료가 아니다 |
| B8 | 음성 대조: ENV(`cfs_ref`)의 기존 빌드는 이름이 `""`이므로 경로 비활성 대조로 *그대로 인용*(재빌드하지 않음) | 기존 기록 경로 |

중단·기록 규칙: 어느 단계든 요구를 충족하지 못하면 오류, 요구 조건, 그 레퍼런스를 먼저 기록하고 우회는 `deviation`으로만 둔다(CONDITIONS_POLICY).

### 8.4 이탈 대장(deviation)

"ENV 기준"은 PI가 고정한 번들과 빌드, "공개 기준"은 BP `b051eb0`의 구성이다. 파생 구성은 둘 다에서 벗어난다. 영향 열은 모두 [I]이다.

| ID | 이탈 | ENV 기준 | 공개 기준 | 이유 | 영향 |
|---|---|---|---|---|---|
| DV-00 | **(PI가 D22 순서의 예외를 승인한 경우에만 쓴다)** P0(공개 통합 구성의 호환성 확인)을 건너뛰고 ENV 이식을 시작 | — | 공개 통합 구성을 그대로 구성하는 시도가 먼저(결정문 §3.2) | PI 승인의 출처와 이유를 적는다(Q-5). 승인이 없으면 이 행은 비어 있고 B0–B8은 P0 실패 뒤에만 시작한다 | 이식 구성이 "그대로 구성할 수 없어서 옮긴" 구성이라는 근거가 없다. 공개 통합 기준과의 비교 자료가 없다 |
| DV-01 | CF를 `281a941`/`833fdbb`로 교체 | `15a871e` | 같음(CI 후보) | C1 쌍 정의(D22) | 쌍 외 변수 없음 |
| DV-02 | BP·bplib·tinycbor 추가 | 없음 | 있음 | provider가 필요 | 번들 구성이 늘어난다 |
| DV-03 | 앱 집합을 6개 모듈(`cf bp bplib ci_lab to_lab sch_lab`; `MISSION_GLOBAL_APPLIST`, `targets.cmake` L16. 이 중 `bplib`은 라이브러리이고 시작 스크립트에는 앱 5개와 라이브러리 2개(`tinycbor`, `bplib`)가 있다)로 축소할지 여부 | 앱 15개: `sch_lab ci_lab to_lab sample_app lc cf ds fm hk hs mm sc md cs sbn`, 라이브러리 2개: `cfe_assert sample_lib` (ENV.md L139–L158) | 6개 모듈 | 공개 기준을 따를 때 ENV 앱 중 **11개가 빠진다**(`sample_app lc ds fm hk hs mm sc md cs sbn`; `cf ci_lab to_lab sch_lab` 4개는 겹친다). ENV 라이브러리 `cfe_assert`, `sample_lib`도 빠진다 | 시작 경쟁의 상대가 줄어든다. 선택은 레퍼런스에 없다 → Q-1 |
| DV-04 | 시작 스크립트: BP 7줄을 ENV 앱 이름으로 대응(`ci_lab`→`CI_LAB`/`CI_LAB_AppMain` 등) | 생성 스크립트(우선순위 35–70) | 7줄(60/70/80/65/55) | 우선순위·순서는 C1 레퍼런스 | 우선순위는 ENV에서 어차피 적용되지 않음(E4) |
| DV-05 | 미션 정의 생성 방식 | `sample_defs` 그대로 | `cfe/cmake/sample_defs`의 `sample_perfids.h`·`cpu1_msgids.h`·`cpu1_platform_cfg.h`·`sample_mission_cfg.h` 복사(common.yml L118–L124) | ENV cFE에는 이 파일들이 없다(대신 `cfe_perfids.h`, `example_mission_cfg.h`, `example_platform_cfg.h` 등, 목록 [F]) | 파생 정의를 새로 구성 |
| DV-06 | CF 표를 BP `tx` 표로 교체(`CF.config_table`) | 기본 표 | `tx`/`rx` 표 | C1 경로를 활성화 | 이름 두 개와 MID·`local_eid` 변경 |
| DV-07 | `CFE_PLATFORM_CMD_MID_BASE`(0x1800)·`CFE_PLATFORM_TLM_MID_BASE`(0x0800) 보조 정의 | 없음(토픽 ID 방식) | CI 시기 cFE 샘플이 정의 | BP의 MID가 이 기준값에 의존(`bp_msgids.h` L25–L51) | MID 값은 CI와 같게 맞출 수 있으나 ENV의 MID 체계와 겹치는지 별도 확인 필요 |
| DV-08 | PSP iodriver API 이름·`Location_t` 필드, `unsock_intf` | ENV PSP | CI 후보 PSP 브랜치 | §5.3 | B-1이면 이식 코드, B-2면 제외 → Q-2 |
| DV-09 | 빌드 형식 | `debug`(-g, 최적화 없음) | `Release` | ENV 고정([env/ENV.md](../env/ENV.md) C5) | 타이밍 길이가 다르다. 경쟁 구간의 길이를 비교하지 않는다 |
| DV-10 | tinycbor 설치 위치 | 없음 | `/`에 복사 | 호스트를 수정하지 않기 위해 사용자 영역 | `extra_install.cmake` L13이 `/usr/local/lib/libtinycbor.so`를 가리키므로 경로 치환 필요 |
| DV-11 | 인스턴스 수 | 1(`cpu1`) | 2(`tx`,`rx`) | C1 초기화 관찰에는 1개로 시작 가능 [I] | 파일 전송 시험(워크플로 후반)은 하지 않는다. 선택은 레퍼런스에 없다 → Q-3 |
| DV-12 | TO_LAB·SCH_LAB 표 | ENV 표(`TO_LAB.Subscriptions`, `SCH_LAB.Schedule`, CF `CF_CH0_TX_MID` 참조) | BP 표(`TO_LAB_APP.TO_LAB_Subs`; sch 표가 `bp_msgids.h` 포함) | CF 교체로 ENV 표의 CF 참조가 깨지고(§7.3), BP의 스케줄 항목이 필요(`BP_WAKEUP_MID`) | 표 이름을 앱 이름에 맞게 대응 |
| DV-13 | 단위 시험 플래그. **파생 빌드의 기본은 "끔"(ENV 플래그를 주지 않음)** | `-DENABLE_UNIT_TESTS=TRUE`(ENV.md L132, CONDITIONS C3) | 지정 없음(`build-run-bp-common.yml` L127) → bplib 기본 OFF | `d4c6db9`의 CMake는 `ENABLE_UNIT_TESTS`가 정의되면 `BPLIB_ENABLE_UNIT_TESTS`를 켜고 `ut-functional`을 CFS 빌드에 추가한다(`CMakeLists.txt` L36–L37, L47, L210–L213). bplib 자신의 다음 커밋 `e7524239`(PR #215)는 "sanity checks … do not link properly" 때문에 CFS 빌드에서 이를 뺀다(§4.3). 따라서 **"켬 + `d4c6db9`"는 bplib 자신의 커밋 메시지에 따라 빌드 실패가 기대된다**(빌드로 확인하지 않음 [U]). 끔은 BP CI의 호출과 같다 | "끔"이면 bplib `ut-functional`은 만들어지지 않는다. 같은 플래그가 cFE·OSAL·CF·BP의 단위 시험 실행 파일(`if (ENABLE_UNIT_TESTS)`의 `add_subdirectory`, 예: cFE `modules/es/CMakeLists.txt` L42–L44)도 만들지 않으므로 ENV의 산출물 목록과 다르다. `core-cpu1`과 앱 `.so`의 실행 코드가 달라지는지는 비교하지 않았다 [U]. **대안**: "켬 + `e7524239`"(ENV 플래그 유지, bplib 커밋이 달라짐). 결정 → Q-4 |
| DV-14 | 작업 디렉터리·소스 경로 | `cfs_ref` | CI 작업공간 | `cfs_ref` 불변 유지 | 없음 |
| DV-15 | 실행 사용자·스케줄링 | 일반 사용자, 우선순위 미적용 | 알 수 없음(§3.6) | ENV 고정 | root 실행·실시간 적용은 별도 조건으로만 |
| DV-16 | **`CFS_APP_PATH`를 설정**(CF 교체와 BP·bplib 추가의 수단) | 설정하지 않음. CONDITIONS C4: `CFS_APP_PATH` 빈 값(`CMakeCache.txt` L42 `CFS_APP_PATH:STRING=`). `build_cfs.sh`는 이 환경변수가 설정돼 있으면 중단한다(`REFUSED_VARS` L142–L146, `check_env()` L147부터) | BP CI가 `CFS_APP_PATH: ${{ github.workspace }}/src`로 설정(`build-run-bp-common.yml` L45–L46) | 번들의 `apps/cf`보다 앞서 CF를 찾고 BP·bplib을 더하는 방법이 이 변수의 검색 순서이다(ENV cFE `mission_build.cmake` L380–L384, L397–L407). 대안(번들 서브모듈의 CF를 직접 바꾸기, 번들 트리에 앱 디렉터리 추가)은 번들 자체를 수정해 "서브모듈 SHA가 ENV와 같다"는 B1의 기록을 깨뜨린다 [I] | 이 변수는 CMake 캐시에 저장돼(`mission_build.cmake` L380–L382, `CACHE STRING`) 이후 재구성에도 남는다. 소스 선택 외에 컴파일 옵션을 바꾸지 않는다 [I, 소스 읽기]. ENV의 환경 점검(`check_env`)이 그대로는 이 빌드를 거부하므로 B0에서 어떤 거부 변수를 어떤 방식으로 설정하는지 기록한다 |

DV-03, DV-11, DV-13은 레퍼런스가 결정하지 않으므로 값을 고르지 않았다. DV-13의 "끔"은 BP CI의 호출이라는 레퍼런스에서 가져온 값이며, ENV 플래그와의 충돌은 DV-13에 기록했다. bplib 커밋은 고르지 않았다: `unspecified_by_reference`이며 Q-4가 정할 때까지 B2를 시작하지 않는다. 기본안이 필요하면 §12에 PI 결정으로 올린다.

### 8.5 P0 — P-REC: 공개 통합 구성을 그대로 재구성 (B0 앞의 첫 단계)

D22의 적용 순서("공개 통합 구성의 버전 호환성을 먼저 확인")의 첫 단계이다(§8.0). 이번 점검에서는 **정적 대조[M]만** 했고 조립은 하지 않았다. 정적 대조는 이름의 존재만 보므로(§5.3) 이 단계를 채우지 못한다.

- 구성: cFS `1df262a` 또는 `1b0b338`(+ 서브모듈 핀), PSP에 `bd4a2ea` 병합(fast-forward), bplib `d4c6db9`(또는 `e7524239`; 단위 시험 플래그를 주지 않으므로 두 후보의 구성 단계가 같다 [I, §4.3]), tinycbor `v0.6.0`, CF `281a941`/`833fdbb`, BP `b051eb0`, BP의 `buildconfig` 그대로. 빌드는 `common.yml` L115–L131의 명령 그대로(ENV와 별도 작업 공간). `ENABLE_UNIT_TESTS`는 주지 않는다(BP CI와 같음).
- **결과의 분기**(§8.0과 같다):
  - 조립·기동에 성공하면 "공개 통합이 그대로 구성되는 기준"이 된다. ENV 이식(B0–B8)은 이 기준과 비교하는 파생 구성이며, 이식을 할지는 그때 PI가 정한다.
  - 실패하면 **실패 지점을 요구(오류 메시지)와 그 레퍼런스와 함께 먼저 기록**한다(CONDITIONS_POLICY). 우회는 `deviation`으로만 둔다. 그 뒤에 ENV 이식(B0–B8)을 시작하며, 이식 구성은 "그대로 구성할 수 없어서 다른 버전으로 옮긴" 구성이 된다.
- **실패의 원인 구분.** 이 호스트의 도구(Ubuntu 24.04, gcc 13.3, cmake 3.28.3, §8.1)는 BP CI의 러너(`ubuntu-22.04`; 컴파일러 버전은 기록 없음 [U])와 다르다. 따라서 P-REC의 실패는 (a) 공개 스택 버전 사이의 비호환, (b) 이 호스트의 도구 차이(예: 2022년 코드가 gcc 13의 경고·오류에 걸림), (c) 네트워크·설치 의존성(tinycbor, 움직이는 참조) 중 어느 것인지를 구분해 기록한다. (b)나 (c)는 "공개 통합 구성은 그대로 구성할 수 없다"는 근거가 아니다. 그 구분이 불가능하면 `unknown`으로 두고 PI에게 올린다.
- **대체를 쓰는 경우.** PI가 정적 대조(§5.3)로 P-REC를 갈음하겠다고 하면(Q-5), 그것은 D22 순서의 "확인"을 실행하지 않은 채 진행하는 예외이다. 그 승인의 출처와 이유를 DV-00에 기록한다. 이 문서는 정적 대조가 실행을 갈음한다고 주장하지 않는다.
- 이 경로도 cFS 핵심 스택이 ENV와 다르므로 ENV 위의 결과로 계산하지 않는다. 관측기(Kairos)가 이 스택에서 동작하는지는 평가하지 않았다 [U].

### 8.6 이 계획의 위험(정직하게)

- 이름 수준 대조가 통과한 CF·bplib도 인수 형식·매크로 의미·`-Werror` 경고에서 막힐 수 있다 [U]. ENV 컴파일 옵션은 `-Wall -Werror -pedantic -std=c99`이고 gcc 13.3이다. 2022년 코드를 이 조합으로 컴파일한 결과는 없다.
- BP의 `bp_msgids.h`는 `CFE_PLATFORM_*_MID_BASE + n` 방식이며 ENV의 MID 체계(토픽 ID)와 값이 충돌하는지 확인하지 않았다 [U].
- B-1의 이식량은 직접 시도하기 전에는 모른다.
- **단위 시험 플래그와 bplib 커밋**: ENV 플래그(켬)와 `d4c6db9`의 조합은 bplib 자신의 커밋 메시지에 따라 CFS 빌드에서 `ut-functional`이 링크되지 않는 경로로 읽히며, 빌드 실패가 기대된다 [I, §4.3]. B5의 기본을 "끔"으로 둔 이유이다(DV-13). "끔"이 산출물의 실행 코드를 바꾸는지는 비교하지 않았다 [U]. 이 조합이 실제로 실패하는지는 시험 빌드를 하기 전에는 모른다.
- **`CFS_APP_PATH`**: ENV의 환경 점검이 이 변수를 거부한다(DV-16). B0에서 설정 방식을 기록하지 않으면 ENV 기준 빌드와 파생 빌드의 구분이 흐려진다.
- **P0을 건너뛰는 위험**: P-REC 없이 ENV 이식으로 가면 "공개 통합 구성은 그대로 구성할 수 있었는가"에 대한 답이 없다. 그 경우 이식 구성이 D22의 "다른 버전으로 옮긴" 구성이라는 근거가 없고(§8.0), 공개 통합 기준과의 비교 자료도 없다.

## 9. P-C1HOLD — 사건 기반 보류 설계 (검증 항목 7)

결정문 §3.3: provider의 **생성 직전 보류**를 채택, RESERVED 구간 실험은 후속 과제, 해제는 CF의 **재시도 종료 또는 최종 초기화 결과라는 사건**, 재시도 수는 선택한 버전의 소스에서 읽고 고정하지 않으며, 재시도 수×지연은 벽시계 상한으로 쓰지 않는다. 보류로 만든 실패는 자연 실행과 구분하고, 단순 timeout 관측만으로 영구 교착이라 부르지 않는다. 구현(관측기·보류 장치)은 다른 에이전트의 작업이며 이 절은 **무엇을 관측하고 언제 풀지**를 정한다.

### 9.1 사건 어휘 — 관측점과 소스 위치

관측점의 실현 가능성(심볼 노출·`uprobe` 가능 여부)은 빌드 후 확인할 [U] 항목이다. 아래는 소스상 위치이다. 모든 사건은 하나의 단조 시계의 시각, 스레드 ID, 앱 ID를 함께 기록한다(관측기 시계 차이는 파일럿에서 측정).

**provider(BP) 쪽**

| 사건 | 정의 | 소스 위치(`b051eb0`) | 기록할 값 |
|---|---|---|---|
| P-0 | `BP_FlowInit` 진입 | `bp_flow.c` L519–L524 | 시각 |
| P-1 | BP 자신의 조회 호출. **성공하면 BP는 생성하지 않는다**: "Found semaphore: %s" 이벤트(L129–L130)를 내고 다음 이름으로 넘어간다. P-2·P-3은 그 이름에서는 일어나지 않는다 | `OS_CountSemGetIdByName`, L126–L127 | 이름, 반환값, 반환 ID |
| **P-2** | **생성 호출 진입(보류 지점)**. P-1이 실패했을 때만 도달한다(`else if`, L131–L133) | `OS_CountSemCreate`, L132 (인수 `sem_name`, `sem_initial_value`=`BP_GlobalData.Throttles[index].Max`=10) | 이름, 초기값, 호출 스레드 |
| P-2a | **이름 공개**(OSAL 내부). `OS_ObjectIdAllocateNew`가 독점 모드로 빈 기록을 잡은 뒤, `OS_OBJECT_INIT`이 그 기록의 `name_entry`를 설정한다. 이때부터 기록은 RESERVED 상태로 이름이 보인다 | ENV OSAL `osapi-countsem.c` L94, L100; 매크로 `os-shared-idmap.h` L106–L111. CI 후보 OSAL도 같은 줄 [F] | 시각, 스레드 |
| P-2b | 구현 호출(OSAL 내부) | `OS_CountSemCreate_Impl`, `osapi-countsem.c` L103 | 반환 상태 |
| **P-2c** | **id 확정 = 객체 사용 가능**(OSAL 내부). `OS_ObjectIdFinalizeNew`(L106) 안의 `OS_ObjectIdTransactionFinish(token, &final_id)`가 `active_id`를 최종 id로 바꾼다. §1.2의 readiness precondition은 이 사건 기준이다 | ENV `osapi-idmap.c` L859(함수 L838), CI 후보 OSAL L849(함수 L828) [F] | 시각, 최종 id |
| P-3 | 생성 호출 반환 | 같음 | 반환 상태, `osal_id_t` |
| P-E | **생성 호출 전의 provider 실패**(§5.4). `AppInit`이 `BP_FlowInit` 전에 반환: 표준 오류 `bplib_init failed`·`bplib_route_alloc_table failed`(`bp_app.c` L114, L122) 또는 L154–L184의 반환, 이어서 종료 이벤트 `BP_EXIT_ERR_EID`=7 "BP application terminating"(L839–L840, `bp_events.h` L34)과 `CFE_ES_ExitApp`(L844) | `bp_app.c` | 실패 단계, 반환 상태. P-0이 관측되지 않는다 |
| P-4 | 알림 이벤트. **같은 `BP_SEM_INFO_EID`=15가 두 문구를 가진다**: 조회 성공 시 "Found semaphore: %s", 생성 성공 시 "Created semaphore: %s". 생성 실패 시 `BP_SEM_ERR_EID`=14 "Failed to create semaphore: %s" | 문구는 `bp_flow.c` L129–L130(Found), L135(Created), L140(Failed). 이벤트 ID는 `bp_events.h` L41–L42 | 이벤트 ID와 **문구**. 생성을 이벤트로 판정하지 않는다: 이 provider가 만든 객체인지는 P-3(생성 호출이 `OS_SUCCESS`를 반환, 같은 `osal_id_t`)으로 정하고, 이벤트는 문구로 가른 보조 기록으로만 쓴다 |
| P-5 | provider 초기화 종료 | `BP_FlowInit` 반환, 이어서 `BP_CLA_Init`(`bp_app.c` L208) | 반환 상태 |
| P-6 | 첫 `CFE_ES_RunLoop` | `bp_app.c` L818 | 시각 |

**consumer(CF) 쪽** — 줄 번호는 `281a941`/`833fdbb`, v7.0.1은 대응 위치(§5.1)

| 사건 | 정의 | 소스 위치 | 기록할 값 |
|---|---|---|---|
| C-0 | `CF_CFDP_InitEngine` 진입 | `cf_cfdp.c` L952 | 시각 |
| C-1 | 채널 `i`의 조회 호출 진입(시도 번호 `k`) | `OS_CountSemGetIdByName`: `281a941` L993, `833fdbb` L1003 | 채널, 이름, 시도 번호(같은 이름·같은 스레드의 연속 호출 수로 관측기가 센다) |
| C-2 | 조회 호출 반환 | 같음 | 반환 상태(`OS_SUCCESS`/`OS_ERR_NAME_NOT_FOUND`/기타), 반환 ID |
| C-3 | 지연 호출 | `OS_TaskDelay(CF_STARTUP_SEM_TASK_DELAY)`: `833fdbb` L1011 (`281a941`에는 없음) | 인수(ms) |
| C-4 | 재시도 종료 | 정의: 같은 채널에서 C-2가 `OS_ERR_NAME_NOT_FOUND`가 아닌 값을 낸 첫 시점, 또는 시도 번호가 소스에서 읽은 최대 횟수에 도달한 뒤의 마지막 C-2 | 종료 사유(`success`/`other_status`/`exhausted`) |
| C-5 | 실패 이벤트 | 이벤트 30, 문구 "failed to get sem id … error=%ld" (`281a941` L997–L999, `833fdbb` L1016–L1018) | 이벤트 텍스트의 `error` 값 |
| **C-6** | **최종 초기화 결과** | `CF_CFDP_InitEngine` 반환(`281a941` L1034): 상태 값. 성공이면 이어서 이벤트 20 "CF Initialized"(`cf_app.c` L258) | 반환 상태 |
| C-7 | 앱 종료 요청 | `CFE_ES_ExitApp(run_status)`(`cf_app.c` L366) — 실패 시 `APP_ERROR` | 상태 |

**ES·시스템 쪽(부수 기록)**: ES의 `LATE_INIT`·`RUNNING` 대기와 시간 초과 로그(ENV `cfe_es_start.c` L210–L228), `OPERATIONAL` 진입 줄, 앱 상태. TO_LAB은 `CFE_ES_WaitForStartupSync(TO_LAB_STARTUP_SYNC_TIMEOUT)`(ENV `to_lab_app.c` L67)로 구독을 미루므로, BP를 보류하면 ES가 `OPERATIONAL`에 도달하는 시각이 늦어져 TO_LAB의 구독 시점이 달라진다 [I]. 이는 C1 변수가 아니라 **보류의 부수 효과**로 기록한다.

로드맵 §8.2가 `[제안]`하는 세 시점과의 대응(명세 App. A.4 step 5의 "The normal path records the creation and lookup events directly"와 결정문 §3.3의 같은 문장 [D]이 이 기록의 결정된 근거이다): "생성 시작" = P-2, "이름 공개" = P-2a, "객체 사용 가능" = P-2c [I: 대응은 이 문서의 해석]. P-2a~P-2c는 OSAL 공유 계층 안의 사건이므로 관측 방식(심볼 진입, 호출 지점 주소, 또는 P-3과 C-2에서 결과로 함의되는 순서만 사용)은 빌드 후 정한다 [U]. 이 셋을 직접 관측하지 못하면 §1.2에 따라 "같은 id의 `OS_SUCCESS`"를 결과로 함의되는 reads-from으로만 쓴다.

생성 세대 식별: P-3의 `osal_id_t`와 C-2의 반환 ID를 비교해 같은 객체인지 기록한다. 이름이 같아도 삭제·재생성된 객체를 같은 세대로 가정하지 않는다(로드맵 §8.2의 `[제안]`; 명세 §1.6의 상태 정의는 "same generation"의 수명 사건만 센다 [D]).

### 9.2 보류·해제 규칙(키마다)

- **키**는 semaphore 이름이다(`BP0_tsem`, `BP1_tsem`). A1–A5는 키별로 판정한다. 채널 0의 조회가 성공해도 채널 1은 다른 키이다.
- **앞 채널의 실패는 뒤 채널의 조회를 없앤다** [F]. CF의 채널 루프(`cf_cfdp.c` L967)는 조회가 최종 실패하면 이벤트 30을 낸 뒤 `break`로 **루프 전체를 빠져나간다**(`281a941` L1000, `833fdbb` L1019; `833fdbb`의 L1008 `break`는 재시도 루프만 빠져나간다). 같은 채널의 파이프 생성·구독 실패도 `break`이다(L977, L988). 따라서 채널 0(`BP0_tsem`)이 A3로 끝나면 CF는 `BP1_tsem`에 대해 `OS_CountSemGetIdByName`을 **한 번도 부르지 않는다**(C-1 없음). provider 쪽도 같은 방향이다: BP는 한 스레드에서 이름 0, 이름 1 순서로 조회·생성한다(`bp_flow.c` L124–L142). 그래서 G(`BP0_tsem`)에서 보류하면 `BP1_tsem`의 생성도 함께 막힌다.
- 이런 키의 결과는 **"도달하지 않음(앞 채널 실패, C-1 없음)"**으로 기록하고 A1–A4 어느 것으로도 세지 않는다. 명세의 말로는 그 실행에 그 키 인스턴스의 발생이 없으므로 좋은 증거도 나쁜 증거도 아니다 [I: 명세 §1.6의 instance·witness 정의에서]. 보류 실행 하나는 **한 키만** 시험한다: G(`BP0_tsem`) 보류 실행에서 `BP1_tsem`은 시험되지 않는다. `BP1_tsem`의 A3는 G(`BP1_tsem`)에서 보류하는 **별도 실행**(이름 0은 보류 없이 만들어짐)으로만 얻는다. 그 실행에서도 CF의 채널 0이 자연 순서로 실패하면 `BP1_tsem`은 다시 "도달하지 않음"이며, 그 실행은 `BP1_tsem`의 근거가 아니다.
- **보류 지점 G(name)**: provider 스레드가 `OS_CountSemCreate`(P-2)를 `sem_name == name`으로 호출하기 **직전**. 이 지점은 provider가 OSAL 객체 표를 건드리기 전이므로 RESERVED 구간 *밖*이다(§9.3 A5와 구분). 구현에서 이 지점은 API 심볼 진입 + 인수 조건 또는 BP의 호출 지점 주소 중 하나로 잡을 수 있다. 심볼 진입 방식은 다른 호출자(예: SCH_LAB의 `OS_CountSemCreate`)의 호출도 걸리므로 인수 조건이 필요하다 [I]. P-1이 성공한 이름에서는 P-2가 일어나지 않으므로(§9.1) G(name)에 도달하지 않는다. 그 경우는 보류가 걸리지 않은 실행으로 기록한다.
- **연결 방식**: provider(BP)의 주 스레드는 관측이 시작된 뒤에 ES가 만들므로, 보류 장치는 그 스레드를 만드는 쪽(ES 시작 스레드)에 clone 시 자동 연결해야 하고, 새 스레드는 생성 때 한 번 멈춘다. 이 정지는 앱 시작 창 안의 측정되지 않은 섭동이며, 이를 쓴 실행은 그 사실을 기록한다(SPEC_FINAL @`6be8935` §4.13 "Attach strategy", App. A.4 step 5) [D]. 정지 비용은 S6이 측정한다(명세). 이 문서는 값을 정하지 않는다.
- **보류 가능성 조건**(SPEC_FINAL @`6be8935` §4.13 "Gate placement"의 항목 1·2·3·4: 보류 대상의 프로그램 잠금 집합이 비어 있을 것, 타임베이스 콜백·binsem 내부 구간·시그널 핸들러 밖일 것, RESERVED 구간 밖일 것(후속 OSAL 결함 후보 실험은 예외), 디버그 레지스터 할당이 가능할 것) [D: 명세의 규칙]. 이 지점이 그 조건을 만족하는지는 빌드 후 확인한다 [U].
- **보류의 범위**: P-C1HOLD는 보류를 **실패 경로의 확인**에 한정한다(결정문 §3.3: "정상 경로는 생성·조회 사건을 직접 기록한다. 실패 경로 확인 시 provider를 생성 직전에 보류하고, CF의 재시도 종료 또는 최종 초기화 결과라는 사건을 기준으로 해제한다") [D]. 정상 경로(A1, A2)는 생성·조회 사건을 직접 기록하며 보류하지 않는다.
- **해제 사건**(사건 기반, 값 없음). 결정된 해제 사건은 "CF의 재시도 종료 또는 최종 초기화 결과"이다.
  - **R-a: C-4(재시도 종료)가 관측될 때.** 이것이 보류를 푸는 주된 규칙이다. C-4는 C-5(실패 이벤트)와 C-6(최종 초기화 결과)보다 앞서므로, **R-a는 C-5·C-6 이전에 provider를 푼다**. 따라서 보류로 만든 실행에서 P-2는 C-4 이후이다. **P-2와 C-5·C-6 사이의 순서는 관측해 기록하며 가정하지 않는다**: `833fdbb`에서는 마지막 실패한 조회 뒤에도 `OS_TaskDelay`가 한 번 더 불리고(L1011) 이벤트 전송이 이어지며, 풀린 provider 스레드가 먼저 스케줄된다는 근거는 없다.
  - **R-b: C-6(최종 초기화 결과)이 관측될 때.** C-4를 사건 열에서 식별하지 못한 경우의 대비이다(둘 중 먼저 오는 사건에서 푼다).
  - **R-d: CF가 종료한 경우(C-7)에만.** `CF_AppMain`은 `CF_Init`이 반환한 **뒤에** `CFE_ES_RunLoop`을 거쳐 `CFE_ES_ExitApp`에 이른다(`cf_app.c` L330–L338, L366; `281a941`·`833fdbb` 같음 [M: `diff`]). 그러므로 시작 구간의 C-7은 CF의 최종 초기화 결과(`CF_Init`의 반환)가 이미 나왔음을 함의한다 [F]. `CF_Init`이 `CF_CFDP_InitEngine`(C-0) 전에 실패한 경우에도 그렇다(그때는 C-6이 없고 모든 키가 "도달하지 않음"이다). 그래서 R-d는 결정된 해제 사건(최종 초기화 결과)을 C-6으로 식별하지 못했을 때 알아보는 수단이며, 새 해제 사건이 아니다. 명세 §4.13 "Release"의 일반 규칙("when the other party is blocked or has exited") [D] 중 **"막힌 경우"는 쓰지 않는다**: 결정문 §3.3이 C1에 정한 해제 사건은 재시도 종료와 최종 초기화 결과 둘뿐이며, CF가 막혔다는 것은 그 어느 쪽도 아니다. 이것은 §9.2.1이 R-c를 쓰지 않는 이유와 같다. CF가 C-4 전에 막히면 감시 한도(PV-1)에 따라 `inconclusive(await-timeout)`이다. "막힘"에서의 해제를 C1에 쓸지는 PI 질문으로 올렸다(§12 Q-10).
- 기다리는 사건이 오지 않을 때의 처리(감시 한도)는 값을 정하지 않고 §9.4에 둔다. 한도에 도달하면 결과는 `inconclusive(await-timeout)`이며 "교착"으로 부르지 않는다.
- 구현 틀: 명세의 F0 `kz hold`(§4.13 "Mechanisms": `PTRACE_SEIZE`+실행 중단점 레지스터, 해당 스레드만 정지, 나중에 생성되는 스레드는 생성 시 자동 연결) [D: 명세의 설계]. 구현은 이 문서의 범위가 아니다.

#### 9.2.1 결정 범위 밖의 제안: "제어된 A2"(R-c) — 이 문서는 쓰지 않는다

A2(먼저 실패, 이후 재시도로 성공)를 보류로 만들자는 제안이 있을 수 있다: 해당 키의 첫 `OS_ERR_NAME_NOT_FOUND`(C-2, k=1)가 관측될 때 provider를 푸는 규칙(R-c)이다. 이 문서는 이것을 **쓰지 않으며**, A2는 **자연 실행에서만** 얻는 구간으로 둔다. 이유는 다음과 같다.

- P-C1HOLD가 정한 것은 실패 경로 확인을 위한 보류와 그 해제 사건(재시도 종료 또는 최종 초기화 결과)이다. A2는 실패 경로가 아니며, 첫 `NAME_NOT_FOUND`는 두 해제 사건의 어느 쪽도 아니다 [D: 결정문 §3.3].
- R-c는 A2를 보장하지 않는다. 풀린 provider의 생성이 CF의 다음 시도(지연 `CF_STARTUP_SEM_TASK_DELAY` 뒤) 전에 끝나야 성공하는데, 그 길이는 이 문서가 모르는 값이며 정하지도 않는다(§6.3). 생성이 늦으면 같은 보류 실행이 A3로 끝난다.
- 보류 실행은 어느 쪽이든 `held-F0`이며 자연 실행의 빈도 정보를 주지 않는다.

PI가 "제어된 A2"를 P-C1HOLD의 범위 확장으로 받아들이면 그때 해제 규칙·표시·감시 한도를 새로 정한다(§12 Q-9). 그 전에는 §9.4 PV-6이 비어 있다.

### 9.3 A1–A5와 관측 사건의 대응

로드맵 §8.3의 다섯 구간(그 표는 `[제안]`이다. 구간의 결정된 분류는 명세 App. A.4 step 6 "Outcome partition" [D]이다). 판정의 세 층(QT-3)을 열로 분리했다: **관측 사건 열**(층 3), **OSAL 반환**(층 1: 문서화된 API 의미), **readiness precondition과 시도 분류**(층 2: 호출자 요구, §1.2의 정의).

층 2의 열은 OSAL 반환값을 베껴 채우지 않는다. 정의(§1.2)는 *조회의 선형화 시점에 객체가 존재하는가*이며, 시도의 분류는 SPEC_FINAL @`6be8935` App. A.4 step 3·6의 어휘를 따른다 [D]: **READY**(요구 충족), **TOLERATED**(요구 미충족이나 `caller` 기록이 있는 허용 항목 `retry_until_ready`가 받아들이는 시도; READY와 TOLERATED는 모두 좋은 쪽), **VIOLATING**(요구 미충족이고 허용 항목의 감시가 REJECT하는 시도), **NOT_READY_UNJUDGED**(요구 미충족인데 `caller` 기록이 없어 허용 여부를 판정하지 않는 시도; 명세 §1.6의 bad 정의에서 "where Allowed is `unspecified_by_reference`" VIOLATING과 함께 나쁜 쪽). *readiness violation*은 **VIOLATING 시도에만** 쓴다(명세 §0, §0.11 item 2; §1.2). 나쁜 쪽은 두 가지로 적는다: **VIOLATING → readiness violation**, **NOT_READY_UNJUDGED → 나쁜 쪽, 판정되지 않음, readiness violation 아님**(결과 라벨 `not-ready-unjudged`). TOLERATED인 A2의 앞선 시도는 좋은 쪽이며 violation이 아니다.

**`caller` 기록의 판별 [F/D].** rev 1.2의 QT-4 엄격 대안에서 TOLERATED는 `caller` 기록이 있을 때만 나온다(명세 §1.6 "Caller-level Allowed"). 명세 §2.3의 블록은 그 기록으로 CF 자신의 주석 "There is a start up race condition because CFE starts all apps at the same time …"(`code_admission`, side `caller`)을 ENV CF `15a871e`의 `cf_cfdp.c` L1272–L1278에서 인용한다. 같은 주석은 `833fdbb`의 `cf_cfdp.c` L993–L999에 있고(첫 문장 L994 [F, `grep`]), `281a941`·`2a024d8`에는 없다(재시도 루프 자체가 없다). 따라서 `833fdbb`에서 A2의 앞선 시도가 TOLERATED가 되려면 이 줄을 `833fdbb` 기준으로 다시 묶는 기록(명세의 anchor audit; App. A.4 step 6 "the line and the name are rebound per basis by the anchor audit")이 필요하다 [D/U].

**선행 조건 G-REBIND(이 문서의 선택).** 다시 묶기 전에는 그 시도가 NOT_READY_UNJUDGED이고, 명세 §1.6에 따라 그런 시도를 가진 실행은 **나쁜 실행**이다. 그러면 자연 A2 실행은 순번 (CF, 1 … m−1)에서 나쁜 증거가 되어, 자연 A1 실행(같은 순번 (CF, 1)에서 좋은 증거)과 함께 명세 step 6의 같은 순번 조건을 이미 채운다. 이 상태로는 기대 결과표가 "A2 = 좋은 쪽"이라는 이 절의 서술과 맞지 않는다. 그래서 이 문서는 다음을 **선행 조건**으로 둔다: **anchor audit가 `833fdbb`의 `cf_cfdp.c` L993–L999를 K-REG.osal-countsem-lookup의 `code_admission caller` 기록으로 다시 묶은 기록이 있기 전에는 `833fdbb` 빌드의 C1 출력(S1·S2의 자연 상태 집계, 보류 실행의 판정)을 하나도 내지 않는다.** 이것은 결정문 §3.7(P-F1GATE)의 "기대 결과표는 해당 사례의 첫 출력 전에 동결한다"[D]를 지키기 위한 순서이다. 아래 표와 해석 규칙은 G-REBIND가 충족된 상태의 기대 결과이다. anchor audit가 다시 묶기를 **거부하면**(예: 그 줄이 같은 기록이 아니라고 판정하면), 첫 출력 전에 기대 결과표를 다음과 같이 다시 쓰고 동결한다: `833fdbb`의 A2 실행은 순번 1 … m−1에 NOT_READY_UNJUDGED(나쁜 쪽, readiness violation 아님) 시도를 가진 나쁜 실행으로 좋음·나쁨 짝짓기에 들어가며, A2를 "수정이 회복시킨 실행"이라고 부르지 않는다. 그 변경은 이 문서의 개정으로 기록한다. `281a941`에는 다시 묶을 주석이 없으므로 G-REBIND는 `833fdbb`에만 해당한다.

| 구간 | 로드맵 정의 | 관측 사건 열(키별, 층 3) | OSAL 반환(층 1) | 시도 분류와 readiness precondition(층 2) | 필요한 보류 | `281a941` | `833fdbb` |
|---|---|---|---|---|---|---|---|
| **A1** 첫 조회 성공 | 첫 조회에서 성공 | C-1(k=1) → C-2(k=1)=`OS_SUCCESS`, 반환 ID = P-3의 ID(같은 생성 세대) → (모든 키) 이벤트 20. P-3과 C-1의 선후는 기록하되 판정에 쓰지 않는다 | `OS_SUCCESS` | (CF, 1) **READY**. 조회의 선형화 시점에 객체가 확정돼 있었다. 확정 사건을 직접 관측하지 못하면 이 사실은 "같은 id의 `OS_SUCCESS`"에서 결과로 함의된다(§1.2). 조회가 생성 호출의 반환보다 먼저 시작했어도 확정 뒤에 표를 훑었다면 READY이다 | 없음(자연). 자연 순서가 이 구간이 되는지는 관측으로만 안다 | ✓ | ✓ |
| **A2** 먼저 실패, 재시도 성공 | 처음은 실패, 이후 재시도로 성공 | C-2(k=1)=`OS_ERR_NAME_NOT_FOUND` → C-3 → … → C-2(k=m, 2≤m≤R)=`OS_SUCCESS`, 반환 ID = P-3의 ID; 이벤트 20 | 처음 `OS_ERR_NAME_NOT_FOUND`, 이후 `OS_SUCCESS` | (CF, 1 … m−1) **미충족이나 TOLERATED**(G-REBIND 충족 상태. 충족 전에는 출력하지 않는다; 다시 묶기가 거부되면 NOT_READY_UNJUDGED = 나쁜 쪽, 위 G-REBIND 문단), (CF, m) **READY**. 모든 시도가 좋은 쪽이므로 **좋은 실행**이며 readiness violation이 없다 | **없음(자연 실행에서만)**. "제어된 A2"는 결정 범위 밖이다(§9.2.1, Q-9) | **해당 없음**(조회 1회) | ✓ (`R`=25, 소스에서 읽음) |
| **A3** provider가 늦어 재시도까지 종료 | provider가 늦게 생성되어 재시도까지 종료 | **A3는 C-2 열로 정의한다**: C-2가 `R`번 모두 `OS_ERR_NAME_NOT_FOUND`(`281a941`은 `R`=1). 이어서 C-5(이벤트 30, `error`=`OS_ERR_NAME_NOT_FOUND`), C-6 오류, C-7 `APP_ERROR`. P-2/P-3의 위치: **보류 실행에서는 R-a가 C-4에서 풀므로 P-2는 C-4 이후**이다. 자연 실행에서는 P-2와 마지막 C-2의 선후를 관측으로 기록하며 가정하지 않는다 | `OS_ERR_NAME_NOT_FOUND` | 나쁜 쪽(VIOLATING → readiness violation; NOT_READY_UNJUDGED → 나쁜 쪽, 판정되지 않음, readiness violation 아님). `833fdbb`: (CF, 1 … R) 모두 **VIOLATING**(소진으로 REJECT, G-REBIND 충족 상태) = **readiness violation**. `281a941`: (CF, 1) **NOT_READY_UNJUDGED**(수정 전 처리를 받아들이거나 거부하는 `caller` 기록이 없음), 요약 뷰 **`INCONCLUSIVE(caller_requirement_absent)`**. readiness violation이 **아니고** 위반 수에 세지 않는다. 오류 반환(이벤트 30)과 CF 종료(`APP_ERROR`)는 consequence로 따로 남긴다. 나쁜 쪽이므로 좋음·나쁨 짝짓기의 class는 같다 [D: SPEC rev 1.3 App. A.4 step 6, §1.10 "Summary view", ADR-054(D2). rev 1.2 @`6be8935`의 step 3/step 6 표기 불일치는 rev 1.3이 step 3 쪽으로 정리했다(§9.6)] | 보류 가능: G(name)에서 정지하고 R-a(C-4) 또는 R-b(C-6)로 해제 → *controlled*. 자연 발생은 관측 가능성 확인 | ✓ (`R`=1) | ✓ (`R`=25) |
| **A4** 해당 이름의 provider 없음 | 원출처 구성에 provider가 없으면 단위 예제로 검증하고 실제 사례 결과로 세지 않음 | C-열은 A3와 같고 P-2가 관측 창 전체에서 없음 | `OS_ERR_NAME_NOT_FOUND` | A3와 같다: 나쁜 쪽(VIOLATING → readiness violation; NOT_READY_UNJUDGED → 나쁜 쪽, 판정되지 않음, readiness violation 아님). 버전별 라벨은 A3 행과 같다 | 보류 아님. *공개 BP 기반에는 이 구성이 없다*(BP는 두 이름을 모두 만든다). 명세 App. A.4 step 6 [D]: "no referenced basis exists" | litmus L-PATIENT variant 2·LP-CASE-C1(명세 App. A.8)로 보이거나 "not realized: no referenced configuration"으로 기록(명세 step 6 [D]). PI가 provider 없는 구성을 기록된 이탈로 정하는 경우에만 실현 | 같음 |
| **A5** 객체 생성 중간 상태에서 조회 | OSAL 생성 중간 상태 | C-2=`OS_ERR_INCORRECT_OBJ_STATE`(가설) + 이후 provider·표 정지 관측(wchan/futex) | 문서화된 반환값에 없다(ENV `osapi-countsem.h` L157–L160 [F]) | 이 문서가 분류하지 않는다. 명세 step 6은 VIOLATING으로 둔다 [D]. 확정 전 RESERVED 구간은 별개의 가설 사례이다(§1.2) | provider를 OSAL 내부(생성 진입 이후, 확정 전)에서 정지 | **후속 과제**(P-C1HOLD): 원 C1 시작 조건·다섯 사례 성과 수에 넣지 않는다 | 같음 |
| **도달하지 않음**(A1–A5 아님) | — (이 문서가 더한 기록 범주) | 그 키에 C-1이 없다. 앞 채널의 조회가 최종 실패했거나(이벤트 30 뒤 `break`) 앞 채널의 파이프·구독이 실패했다(§9.2) | 호출 없음 | 발생이 없으므로 분류 없음. 그 키에 대해 좋은 증거도 나쁜 증거도 아니다 [I] | 해당 키의 시험은 그 키에서 보류하는 별도 실행(§9.2) | ✓ (채널 0이 A3이면 채널 1) | ✓ (같음) |

해석 규칙:

- **BP 호출 지점의 인스턴스(P-1)** [F/D/U]. BP도 생성 전에 같은 연산 `OS_CountSemGetIdByName`을 부른다(`bp_flow.c` L126). 명세 §2.3의 `readiness K-REG.osal-countsem-lookup`은 `use op OS_CountSemGetIdByName`으로 열쇠가 잡히므로 이 호출도 같은 readiness specification의 사례이다. 명세 §1.6의 인스턴스는 `i = (κ, t, key, scope)`이고 `t`는 문맥이 붙은 호출 템플릿이므로, BP의 호출 지점은 CF의 것과 **별개의 인스턴스**이다(키가 `BP0_tsem`으로 같아도). 첫 기동(객체 없음)에서 BP의 조회는 `OS_ERR_NAME_NOT_FOUND`를 반환할 것으로 기대한다(§3.3, §9.1 P-1) [I]. 그 시도의 층 2 분류는 **커밋된 명세가 정하지 않는다** [U]: (a) `caller` 기록의 쪽(side)은 "(record, κ) 쌍"에 대해 정해지므로(명세 §1.6 "Side of a provenance record") CF의 인정 주석이 이 κ의 Allowed 항목 전체를 감시기로 만든다고 읽으면, 재시도가 없는 BP 지점에서 `retry_until_ready`의 감시기가 거부해 VIOLATING이 될 수 있다 [I]; (b) 그 기록이 CF의 호출 템플릿에만 해당한다고 읽으면 BP 지점의 Allowed는 `unspecified_by_reference`이고 NOT_READY_UNJUDGED이다 [I]; (c) BP의 "조회 후 없으면 생성"을 준비 요구가 있는 사용이 아니라고 보는 제외 규칙은 명세에서 찾지 못했다 [U]. (a)·(b) 모두 **모든 정상 첫 기동에서 나쁜 실행**을 만든다. 이 문서의 처리: BP 지점은 **별개 인스턴스로 따로 기록하고**(층 1·층 3: 반환값, P-1→P-2 순서), CF의 A1–A4 집계와 같은 순번 짝짓기에 넣지 않는다(템플릿이 달라 명세상으로도 같은 인스턴스가 아니다). 층 2의 기대 상태는 C1 기대 결과표 동결 전에 anchor audit 또는 PI가 정한다(§10 C1B-U23, §12 Q-11). 정해지기 전에는 BP 지점의 분류를 출력하지 않는다. **2026-10-24 갱신:** 위 (a)·(b)는 κ의 `use op`가 모든 호출 지점을 잡는다는 읽기에서 나온다. SPEC rev 1.3 보충(2026-10-24, §1.6 "Call contexts")은 준비 요구를 출처로 정당화된 역할의 호출 문맥에만 붙이므로, BP 지점은 역할 선언(생성 전 존재 확인, 출처 `bp_flow.c` L121–L144)으로 κ의 사례에서 빠지고 층 2 분류가 없다(§9.7 (b)). 이것은 앱 이름 예외가 아니다.
- **provider 자체 실패와 A3의 구분**: C-열이 A3와 같아도 P-E(§9.1)가 관측되고 P-0·P-2가 없으면 그 실행은 A3도 A4도 아니며 `provider-init-failed`로 따로 기록한다(§5.4). 이 실행은 순서 근거로 쓰지 않는다 [I: 이 문서의 기록 규칙. 명세에 같은 이름의 라벨은 없다].
- 좋은 쪽·나쁜 쪽은 **실행이 아니라 시도(순번)마다** 정한다(명세 §1.6: 실행은 나쁜 시도를 하나라도 가지면 나쁜 실행이고, READY·TOLERATED 시도를 가지면 그 순번의 좋은 증거이다). G-REBIND가 충족된 상태에서: A1은 (CF, 1) READY로 좋은 증거, A2는 (CF, 1 … m−1) TOLERATED와 (CF, m) READY로 모든 순번에서 좋은 증거, A3·A4는 (CF, 1 … R) 나쁜 증거(`833fdbb`는 VIOLATING, `281a941`의 (CF, 1)은 NOT_READY_UNJUDGED = `INCONCLUSIVE(caller_requirement_absent)`, rev 1.3 App. A.4 step 6)이다. "도달하지 않음"은 어느 쪽도 아니다. G-REBIND가 거부된 경우의 다른 분류는 위 G-REBIND 문단을 따른다. 같은 바이너리·구성·의미에서 **같은 키·같은 발생 순번**의 좋은 증거와 나쁜 증거가 모두 있을 때만 실행 순서 의존을 주장한다(명세 App. A.4 step 6 "schedule dependence needs a good and a bad witness at the same ordinal in one build, configuration and semantics" [D]; 로드맵 §8.5의 같은 완료 기준은 `[제안]`). 한 빌드에서 A1과 A3를 모두 얻는 경우는 `833fdbb`·`281a941` 각각 *따로* 센다. 두 빌드의 결과를 섞어 같은 순번으로 세지 않는다.
- `281a941`에서 A1이 나오면 그 키는 *수정 전 코드가 성공한 실행*이다. `833fdbb`에서 A2가 나오면 *수정이 회복시킨 실행*이다. A3는 두 버전에서 모두 오류이다. 수정 후에 A3가 나온다는 것은 재시도가 모든 순서 오류를 제거하지는 않는다는 관측이다(로드맵 §8.5는 "제거한다고 주장하지 않는다"를 완료 기준으로 `[제안]`한다. 이 문서는 그 제안을 따른다). 보류로 만든 A3라면 그 가능성을 보일 뿐 자연 발생 빈도의 증거가 아니다.
- 보류로 만든 A3(controlled)는 자연 실행의 빈도 정보를 주지 않는다. 라벨: `held-F0`(보류 사용), 보류 중에 시간 기반 전이(마지막 재시도 실패 등)가 일어난 경우 `timeout-exercised`(SPEC_FINAL @`6be8935` §1.8의 "Confirmation runs in which a time-based transition …"와 §1.10의 naturalness 축, §4.13 "Labels") [D: 명세의 라벨 규칙].
- A5는 명세 App. A.4 step 5의 RESERVED 구간 실험과 같은 것이다(rev 1.1 @`2499ad4`에서는 "declared experiment", rev 1.2 @`6be8935`에서는 "follow-up OSAL-defect-candidate item"이다). 이 문서는 A5의 보류 지점·감시 한도를 정하지 않는다(P-C1HOLD가 후속으로 분리).
- C-5의 이벤트 30 존재만으로 A3를 확정하지 않는다: C-2의 반환 값 열과 시도 횟수로 *재시도 종료 사유*를 구분해 기록한다(`exhausted` 또는 `other_status`).

### 9.4 파일럿 값 — 지금은 정하지 않는다

| ID | 값 | 측정·확인할 것 | 동결 시점 |
|---|---|---|---|
| PV-1 | 보류 중 대기 감시 한도(키별, 해제 사건이 오지 않을 때) | CF가 이벤트 20 또는 30에 이르는 실제 경과 시간(보류 없음/있음), 관측 영향. *재시도 수×지연으로 대신하지 않는다*(결정문 §3.3) | 해당 본실험 전 |
| PV-2 | 보류 대기 중단 조건(예: CF가 앱 종료 사건 없이 정지) | 정상·비정상 사건 열의 파일럿 분포 | 같음 |
| PV-3 | 감시 한도 도달 후 처리(정상화 부팅 포함 여부) | 명세는 비정상 종료 뒤 ES POWERON 재시작으로 정규화 부팅을 한다(SPEC_FINAL @`6be8935` §4.13 "Normalization after abnormal exits") [D] | 같음 |
| PV-4 | 실행 횟수 N(목적별: 동작 확인, 관측 영향 비교, 발생 빈도) | S1 파일럿 | D14: S1에서 정하고 본실험 전 동결 |
| PV-5 | 관측기 시계 간 오차·사건 순서 판정 가능 범위 | 같은 단조 시계 내 정렬 가능성 | 파일럿 후 |
| PV-6 | **비어 있음.** "제어된 A2"(§9.2.1)는 결정 범위 밖이며 Q-9가 승인될 때에만 해제 시점이 필요해진다 | 승인 전에는 측정하지 않는다 | Q-9 이후 |
| PV-7 | 보류 부수 효과(ES 대기 시간 초과, TO_LAB 구독 지연)의 크기 | `OPERATIONAL` 도달 시각의 변화 | 파일럿 후 |
| PV-8 | **섭동 비율(명세 §4.13)의 분모로 쓸 C1의 "코드에서 얻는 분리 간격"이 무엇인지.** `unspecified_by_reference` | 명세의 예(App-record 창의 50 ms 폴링 지연, 코어 직렬화 간격, SCH_LAB 첫 틱)는 C1에 없다. C1에서 보호하는 쪽 순서가 있는지, 있다면 그 간격이 무엇인지를 파일럿에서 측정(예: 키별로 생성의 id 확정과 CF의 첫 조회 선형화 사이의 자연 간격을 사건 열에서 읽는다)한 뒤 PI·명세가 정한다. 측정 전에는 비율을 계산하지 않는다 | 파일럿 후 |

이 값들은 *측정 대상을 정했을 뿐* 값을 고르지 않았다. 도구 단위 시험의 값은 연구 조건으로 승계하지 않는다.

### 9.5 관측 영향·표시(D1·D11)

- 보류·정지·해제를 쓴 실행에는 표시를 붙인다(`held-F0`). 보류 없는 관측 실행과 같은 집계에 합치지 않는다.
- 기록 손실, 사건 대응 실패, 부팅·진행 시간, 외부에서 확인 가능한 기능 결과(CF의 초기화 이벤트·BP의 이벤트)를 보류 없는 기준 실행과 비교한다. 관측하지 않은 기준 실행과 비교할 수 없는 내부 사건에 대해서는 영향이 없다고 결론 내리지 않는다.
- 명세의 섭동 장부(보류 횟수·위치·지속 시간, SPEC_FINAL @`6be8935` §4.13 "Perturbation accounting per run")를 쓴다 [D]. 비율의 분모는 명세가 "the code-derived separation that protects the other order"로 정의하며, 그 예(App-record 창의 50 ms 폴링 지연, 코어 직렬화 간격, SCH_LAB 첫 틱)는 다른 창의 값이다. **C1의 분모는 `unspecified_by_reference`이다**: 이 문서는 C1에 쓸 분리 간격을 고르지 않았고, §6.1의 공통 시작 지연 50 ms(CF와 BP가 **같이** 기다리는 값[F])를 그 자리에 쓰지 않는다. 그 지연은 두 앱의 호출 사이 간격이 아니라 공통 오프셋이기 때문이다 [I]. 무엇을 분모로 삼을지는 파일럿에서 측정한 뒤 정한다(§9.4 PV-8, §10 C1B-U18). 비율에 임계값을 두지 않는다(명세 규칙).

### 9.6 명세와 어긋나는 점(고치지 않고 보고)

- **감시 한도 문구(해소됨).** rev 1.1 @`2499ad4`의 §4.13 "Watchdog"과 App. A.4 step 5는 CF에 대해 "effective retry bound × effective delay"를 들었고, 결정문 §3.3은 이 곱을 벽시계 상한으로 쓰지 말라고 결정했다. 커밋된 rev 1.2 @`6be8935`의 §4.13 "Watchdog"은 "The retry count times the delay is not by itself taken as a safe wall-clock waiting limit"로, App. A.4 step 5는 "Their product is not taken as a safe wall-clock waiting limit"로 바뀌어 결정문과 일치한다 [M: 2026-10-16 `grep`]. 더 이상 어긋남이 아니다. Q-6은 철회 상태로 둔다.
- **`281a941`의 단일 조회 실패 시도의 표기(해소됨, rev 1.3).** rev 1.2 @`6be8935`에서는 App. A.4 step 3이 이 시도를 NOT_READY_UNJUDGED로, step 6이 VIOLATING으로 적어 라벨이 갈렸다. rev 1.3(2026-10-22)의 step 6은 step 3을 따라 **NOT_READY_UNJUDGED**로 정리했고(class는 그대로 나쁜 쪽), 요약 뷰는 **`INCONCLUSIVE(caller_requirement_absent)`**이다(§1.10 "Summary view", ADR-054, D2). readiness violation이 아니며 위반으로 세지 않는다. §9.3은 2026-10-24에 이 표기로 맞췄다. 이 라벨을 바꿀 수 있는 것은 수정 전 처리를 받아들이거나 거부하는 `caller` 기록뿐이며, 그 검토는 §9.7의 "판단할 수 없는 것" 1과 Q-12에 있다.
- **`caller` 기록의 판(版) 묶기.** 명세 §2.3의 `K-REG.osal-countsem-lookup` 블록은 `code_admission caller "apps/cf/fsw/src/cf_cfdp.c:1272-1278"`(ENV CF `15a871e`)를 인용한다. D22의 수정 후 판 `833fdbb`에서 같은 주석은 L993–L999이다(§9.3). 이 줄을 `833fdbb`에 묶는 기록이 없으면 그 판의 A2 앞선 시도는 TOLERATED가 아니라 NOT_READY_UNJUDGED가 된다. App. A.4 step 6은 "the line and the name are rebound per basis by the anchor audit"라고 적으므로 anchor audit의 항목으로 넘긴다 [D/U]. 개정 3부터 이 다시 묶기는 `833fdbb` 빌드의 C1 첫 출력 전 선행 조건(G-REBIND, §9.3)이다.
- **수정 전 구조의 재독해.** 명세 App. A.4 step 3은 수정 전 구조를 `2a024d8`에서 읽고 `281a941`에서의 재독해를 anchor audit 몫으로 남겼다([U]). 이번 점검이 그 재독해를 제공한다: `281a941`의 실패 경로는 `goto err_out`이 아니라 `break`+`ret` 반환이며(§5.1) 오류 상태가 반환된다는 점은 같다 [I]. 명세 문장의 줄 번호(`:1018–1021, 1052–1053` @`2a024d8`)는 `281a941`에서 L995–L1001, L1029–L1034로 바뀐다. 다만 명세가 쓴 "`err_out: return ret;`는 성공 분기도 도달하는 지점"이라는 구조는 `281a941`에서는 `if (ret == CFE_SUCCESS) { enabled = 1; } return ret;`(L1029–L1034)로 바뀌어 있으므로, 정상 연속 경로의 서술은 anchor audit에서 다시 읽어야 한다 [U].

### 9.7 같은 API, 다른 호출 목적 — 호출 문맥의 구분 (2026-10-24; 마스터플랜 §5.1·§5.2, F02–F04)

사본은 §2의 `refs/CF_281a941`, `refs/CF_833fdbb`, `refs/bp_b051eb0`, `refs/bplib_d4c6db9`이고, 줄 번호는 각 커밋의 파일 기준이다 [F, 2026-10-24 다시 읽음].

**규칙** [D: SPEC rev 1.3 보충 2026-10-24, §1.6 "Call contexts"; [MASTERPLAN_ADOPTION_20261024.md](MASTERPLAN_ADOPTION_20261024.md) §2]. 준비 요구(readiness precondition과 caller 수준 Allowed)는 출처(문서, 호출자 코드, 이슈·수정 설명)가 그 호출 문맥의 역할을 **의존적 사용**으로 설명할 때만 그 문맥에 붙는다. 소스 위치는 그 역할을 바이너리의 호출 지점에 묶는 근거로만 쓴다. 앱 이름에 따른 예외와 사례 결과에 맞춘 제외 규칙은 쓰지 않는다.

| 문맥 | 소스 위치 [F] | 역할과 그 출처 | 판정 모형의 처리 |
|---|---|---|---|
| **(a) CF의 의존적 조회** | `281a941` `fsw/src/cf_cfdp.c` L991–L1001: 빈 이름 검사 L991, 단일 조회 L993, 실패 시 이벤트 30(L997–L999)과 `break`(L1000). `833fdbb` L991–L1021: 같은 검사 L991, 인정 주석 L993–L999, 루프 L1001, 조회 L1003, `OS_ERR_NAME_NOT_FOUND`가 아니면 종료(L1006–L1008), 아니면 `OS_TaskDelay(CF_STARTUP_SEM_TASK_DELAY)`(L1011), 최종 실패 이벤트 L1014–L1019. 상수: `CF_STARTUP_SEM_MAX_RETRIES 25`(`fsw/platform_inc/cf_platform_cfg.h` L338), `CF_STARTUP_SEM_TASK_DELAY 100`(L349; 단위는 L341 "Number of milliseconds"). **이 두 값은 `833fdbb`의 값이며 다른 판의 조건으로 옮기지 않는다.** 얻은 id의 사용: `fsw/src/cf_cfdp_sbintf.c` L91–L93 `OS_CountSemTimedWait(c->sem_id, 0)`(두 판 같음) | CF 문서 `docs/dox_src/cfs_cf.dox` L208–L214(두 판 같음): "CF assumes that a per-channel semaphore is provided by the Telemetry Output (TO) application or an equivalent application", "must match the name of the semaphore created by TO", "If a semaphore is expected and not found, the application will terminate during initialization". 이슈 #184 본문(`cases/C1.md` F1) "the CF app expects that semaphore to be created _before_ it starts". `833fdbb`의 주석 L993–L999 | 준비 요구가 붙는다(`K-REG.osal-countsem-lookup`, §1.2의 정의). 시도 분류는 §9.3: `281a941`의 실패 단일 시도는 NOT_READY_UNJUDGED(요약 `INCONCLUSIVE(caller_requirement_absent)`), `833fdbb`는 G-REBIND 뒤 `retry_until_ready`(`caller` 기록 L993–L999)로 TOLERATED 또는 VIOLATING |
| **(b) BP의 생성 전 존재 확인** | `b051eb0` `fsw/src/bp_flow.c` L121–L144 `initialize_throttling`: 이름마다 `OS_CountSemGetIdByName`(L126)이 성공이면 "Found semaphore" 이벤트(L129–L130), 아니면 같은 이름으로 `OS_CountSemCreate(…, Max, 0)`(L132–L133) 후 "Created semaphore"(L135–L136), 생성도 실패하면 "Failed to create semaphore"(L140–L141). `BP_FlowInit`이 L524에서 부르고 결과와 무관하게 다음 단계로 간다(L526–L530) | 호출자 코드: 조회의 "없음"이 같은 이름을 만드는 분기의 조건이다(L126–L133). BP가 이 객체를 CF를 위해 만든다는 글: `fsw/custom/bp_semcfg.c` L33–L36 "The CF task uses semaphores to throttle how many CFDP PDUs it posts", 이름 L43–L49, 표 L68–L70; nasa/bp `0b7422e` 메시지 "This should work in conjunction with the CF app". BP가 이 객체의 사전 존재를 요구한다는 출처는 없다 [M: `b051eb0`의 `README*`·`docs`에서 semaphore·throttle 검색 결과 없음] | **준비 요구를 붙이지 않는다.** 표현은 프로필의 역할 선언(호출 문맥 = 생성 전 존재 확인, 출처 위 두 곳)이며 "BP"라는 앱 이름의 예외가 아니다: 같은 모양의 호출이 다른 앱에 있어도 출처가 보이는 역할로 정하고, 의존적 사용이 출처로 보이면 요구가 붙는다. BP 지점은 κ의 사례가 아니므로 상태·요약·짝짓기가 없고, 반환값(정상 첫 기동에서 `OS_ERR_NAME_NOT_FOUND`)과 P-1→P-2 순서는 층 1·층 3 사실로만 남긴다(§9.1). §9.3의 (a)·(b) 읽기("모든 정상 첫 기동이 나쁜 실행")는 생기지 않는다 |
| **(c) provider 초기화가 생성 전에 실패** | `b051eb0` `fsw/src/bp_app.c`: `AppInit`의 조기 반환 L152–L184(EVS 등록, AppID, 앱 이름, 파이프, 구독), `BP_SetupLibrary` 호출 L196–L200 → `bplib_init` 실패 L112–L116, `bplib_route_alloc_table(10, 16 << 20)` 실패 L119–L124. `BP_AppMain`: `run_status` 초기값 `APP_ERROR` L808, `AppInit` 성공일 때만 `APP_RUN` L812–L814, 종료 이벤트 L839–L840, `CFE_ES_ExitApp` L844. bplib `d4c6db9`: `bplib_init`은 `bplib_os_init`·`bplib_crc_init` 뒤 언제나 `BP_SUCCESS`를 반환한다(`lib/src/v7_bplib.c` L61–L71). `bplib_route_alloc_table`은 `max_routes == 0`(`lib/src/v7_routing.c` L236–L239) 또는 할당·풀 생성 실패(L256–L266)에서만 NULL | 순서 요구의 출처가 아니다. 이 경로는 provider 자신의 초기화 실패이다 | P-E(§9.1)로 관측하고 `provider-init-failed`로 따로 표시한다(§9.3). CF 쪽 시도의 상태는 (a)의 규칙대로 정하지만 그 실행은 순서 근거·같은 순번 짝짓기에 쓰지 않는다. [I] `d4c6db9`에서는 `bplib_init` 실패 가지에 닿지 않으므로 소스가 보이는 생성 전 실패는 자원 할당과 cFE API 실패뿐이다. 일어나는지는 관측으로만 안다. bplib 커밋이 바뀌면(Q-4) 다시 읽는다 |
| **(d) provider 없는 설정 이름** | `281a941`·`833fdbb` `cf_cfdp.c` L991: 이름이 비면 조회가 없다. 공개 BP는 두 이름을 모두 만든다(`bp_semcfg.c` L43–L49, L68–L70; `bp_flow.c` L124) | 이슈 #178(`cases/C1.md` F2): 예제 표의 `cf_1_sem`·`cf_2_sem`은 아무 앱도 만들지 않으며 "If a new user is starting CF by itself with no other support apps running, this semaphore will not exist". 수정은 설정 값을 빈 문자열로 바꾼 것이다(`06c2473`) | 설정 문제로 다룬다. 출처가 그렇게 다뤘고, 순서 의존 결함으로 바꾸지 않는다. A4(§9.3)는 참조 구성이 없어 litmus 또는 "not realized: no referenced configuration"으로만 남는다(SPEC App. A.4 step 6). 빈 이름의 기본 표는 경로 비활성 음성 대조이다 |

**판단할 수 없는 것으로 남는 것.**

1. **`281a941`의 수정 전 처리에 대한 caller 기록.** CF 문서의 "If a semaphore is expected and not found, the application will terminate during initialization"(`cfs_cf.dox` L212–L214)는 종료를 서술한다. 이것을 그 처리를 *받아들이는* `caller` 기록(`exit(code, src)`)으로 읽을지는 판단이고, 출처가 정하지 않는다. 이슈 #184 본문은 같은 종료를 두고 "shouldn't be a hard error"라고 적는다(F1). 받아들임으로 읽고 이슈를 그 판에 묶지 않으면 TOLERATED가 될 수 있다. 둘을 모두 묶으면 충돌이며 QT-5에 따라 판정을 보류한다. 어느 쪽에도 묶지 않으면 지금처럼 NOT_READY_UNJUDGED이다. 이 문서는 라벨을 rev 1.3대로 두고 Q-12로 올린다. 2026-10-24에 읽은 댓글(`cases/C1.md` §0a)도 받아들임이나 거부를 말하지 않는다 [I].
2. **G-REBIND**(§9.3). `833fdbb`의 인정 주석을 `caller` 기록으로 다시 묶는 일은 anchor audit에 남아 있다. 그 전에는 `833fdbb`의 C1 출력을 내지 않는다.
3. **역할 선언의 실체화.** (b)의 선언을 Φ 프로필(`40_rspec.kzp`)에 쓰고, 그 출처 기록과 바이너리 호출 지점의 결합(anchor)을 만드는 일은 W1–W3 뒤이다 [U]. 그 전의 도구 출력에는 BP 지점이 κ의 사례로 잡힐 수 있으므로, 그런 출력은 이 표와 맞지 않는 것으로 보고 결과로 쓰지 않는다.
4. **생성자가 둘인 구성.** 다른 앱이 먼저 만들면 BP의 L126이 성공해 기존 객체를 쓴다. 동시에 확인·생성하면 한쪽의 `OS_CountSemCreate`가 `OS_ERR_NAME_TAKEN`을 받는다(OSAL 문서, `cases/C1.md` F7). 이것은 조회 시점의 준비 요구가 아니라 확인과 생성의 원자성 조건이다. 이 준비 상태 명세로는 표현하지 않고 `not supported`로 기록한다(SPEC §1.6 "Supported conditions"). 참조 구성의 provider는 BP 하나뿐이다.
5. **A5**(RESERVED 구간의 `OS_ERR_INCORRECT_OBJ_STATE`)는 후속 OSAL 결함 후보 항목이다(P-C1HOLD).

## 10. `unspecified_by_reference`로 남는 조건 (검증 항목 8)

값을 고르지 않았다. 필요하면 PI 결정으로 올린다.

| ID | 조건 | 왜 비어 있는가 | 이 문서의 처리 |
|---|---|---|---|
| C1B-U01 | 원 보고자의 cFE·OSAL·PSP·cFS 리비전 | 이슈 #184 본문은 Ubuntu 21.10만 적음(저장소 기록 C1.md F1) | 기록하지 않음. `2a024d8`+`caelum-rc4`는 README 기반 후보일 뿐 |
| C1B-U02 | BP CI가 실행된 당시의 cFS·bplib·PSP·CF 참조 값, 러너 이미지, 액션 버전 | 움직이는 참조이며 실행 로그가 없다 | §4.3의 후보만 [I]로 둠 |
| C1B-U03 | 원 보고자의 provider가 공개 BP와 같은지 | 이슈는 "CI/TO or some other dedicated I/O app"만 적음. 5개 댓글은 읽지 못함(§11) | 같다고 쓰지 않음 |
| C1B-U04 | 실시간 우선순위의 실제 적용, CPU 수, affinity (원 보고자, BP CI) | 레퍼런스에 없음. ENV 측정은 "적용 안 됨"(E4) | ENV 값은 ENV 측정으로만 인용 |
| C1B-U05 | provider 지연량("artificial delay") | 이슈는 지연 값을 주지 않음 | **사건 기반 보류만** 사용 |
| C1B-U06 | 보류 감시 한도·중단 조건 | 파일럿 값(결정문 §3.3) | §9.4 |
| C1B-U07 | 실행 횟수 N | D14 | §9.4 PV-4 |
| C1B-U08 | ENV 위에서의 앱 집합, 시작 스크립트 이름 대응, 우선순위 | 공개 기준과 ENV 기준이 서로 다름. 결합 구성은 어느 레퍼런스에도 없음 | DV-03, DV-04, Q-1 |
| C1B-U09 | provider 구현 방식(B-1~B-4) | 레퍼런스에 없음 | Q-2 |
| C1B-U10 | 인스턴스 수(1 또는 2) | C1 레퍼런스는 인스턴스 수를 정하지 않음 | DV-11, Q-3 |
| C1B-U11 | 빌드 형식(`Release` 대 `debug`)이 결과에 주는 영향 | ENV 고정과 공개 구성이 다름 | DV-09 |
| C1B-U12 | A4를 실현하는 기준 | 공개 BP는 두 이름 모두 생성. provider 없는 구성은 레퍼런스가 없다 | 명세 App. A.4 step 6 [D]를 따름: litmus L-PATIENT variant 2·LP-CASE-C1로 보이거나 "not realized: no referenced configuration"으로 기록. PI가 provider 없는 구성을 기록된 이탈로 정할 때만 실현(로드맵 §8.3의 A4 행은 같은 방향의 `[제안]`) |
| C1B-U13 | `techdev-iodriver`의 CI 당시 팁 | 강제 푸시 여부를 알 수 없음 | `bd4a2ea` 후보 [I] |
| C1B-U14 | ENV TO_LAB·CI_LAB·SCH_LAB와 BP CI 시기 앱(`9e2ad8c` 등)의 동작 차이 | 후자의 소스를 이번에 읽지 않았다 | [U] |
| C1B-U15 | BP `MsgId` 기본값(`RESERVED`)이 데이터 경로 throttle에 주는 영향 | C1 초기화 순서 범위 밖 | §3.3 [U] |
| C1B-U16 | 이슈 #184 댓글 5개의 내용 | nasa/CF 이슈는 이 세션에서 접근 불가(HTTP 403 재확인) | §11 |
| C1B-U17 | 원 보고자의 semaphore 사용 채널 수·이름, `2a024d8` 경로의 시작 스크립트 항목(앱 이름·진입점·우선순위·스택), 관측된 실패 출력, 발생 빈도 | 이슈 본문에 없음("not readily reproducible")(C1.md §10 #15, #25, #29, #30, #38) | 값을 정하지 않음. `2a024d8`+BP 표는 섞지 않음 |
| C1B-U18 | 섭동 비율(명세 §4.13)의 분모로 쓸 C1의 코드상 분리 간격 | 명세의 예는 다른 창의 값이고 C1의 보호 간격은 레퍼런스에 없다. 공통 시작 지연 50 ms는 두 앱의 호출 사이 간격이 아니다 | 값을 고르지 않음. §9.4 PV-8에서 파일럿 측정 후 PI·명세가 정함 |
| C1B-U19 | bplib 고정 커밋(`d4c6db9` 또는 `e7524239`)과 `ENABLE_UNIT_TESTS`의 조합 | BP CI 창의 마지막 5분 안에 bplib `main`이 옮겨 갔고, 어느 쪽을 실행이 썼는지 기록이 없다. ENV 플래그(켬)와 `d4c6db9`의 조합은 빌드 실패가 기대되나 확인하지 않았다 | §4.3, DV-13, Q-4. B5 기본은 "끔"(BP CI의 호출을 따름) |
| C1B-U20 | 공개 통합 구성이 조립·기동되는지(D22 순서의 첫 단계 P0의 결과) | 이름 수준 정적 대조만 했고 조립하지 않았다 | §8.0, §8.5, Q-5. 이 결과 없이 ENV 이식 구성을 D22 기준 구성이라고 부르지 않음 |
| C1B-U21 | `833fdbb`에서 CF 인정 주석(L993–L999)을 `caller` 기록으로 묶는지 | 명세 §2.3은 `15a871e`의 L1272–L1278만 인용한다. 묶기는 anchor audit의 일이다 | §9.3·§9.6. 묶기 전에는 `833fdbb`의 A2 앞선 시도가 NOT_READY_UNJUDGED(나쁜 쪽)이므로, 묶기를 `833fdbb` 빌드의 C1 첫 출력 전 선행 조건(G-REBIND)으로 둔다. 거부되면 첫 출력 전에 기대 결과표를 다시 쓴다(§9.3) |
| C1B-U23 | BP 자신의 조회 호출 지점(`bp_flow.c` L126, P-1)의 층 2 분류 | 커밋된 명세는 CF의 `caller` 기록이 이 κ의 다른 호출 템플릿에도 해당하는지, 또는 "조회 후 없으면 생성"을 제외하는지 정하지 않는다 | §9.3 "BP 호출 지점의 인스턴스". 별개 인스턴스로 따로 기록하고 CF의 A1–A4 집계에 넣지 않는다. 기대 상태는 anchor audit 또는 PI가 정한다(Q-11). **2026-10-24:** 호출 문맥 규칙으로 "준비 요구 없음"이 정해졌다(§9.7 (b)). 프로필 선언과 anchor는 남는다 |
| C1B-U22 | OSAL 내부 사건 P-2a~P-2c(이름 공개, 구현 호출, id 확정)를 관측할 수 있는지와 그 방식 | 빌드 산출물의 심볼·인라인 여부를 아직 보지 않았다 | §9.1. 관측하지 못하면 결과로 함의되는 reads-from만 쓴다(§1.2) |

## 11. 접근 상태와 기존 문서와의 불일치

### 11.1 접근

- 도달함: nasa/bp, nasa/CF(코드), nasa/cFS, nasa/cFE, nasa/osal, nasa/PSP, nasa/bplib, jphickey/PSP, intel/tinycbor(원격 참조 조회), jphickey/cf(원격 참조 조회). 모두 `git clone --filter=blob:none`·`git ls-remote`로 수행 [M].
- **도달하지 못함**: nasa/CF 이슈 #184의 댓글(5개). `gh api repos/nasa/CF/issues/184/comments`와 이슈 페이지 모두 HTTP 403("GitHub access to this repository is not enabled for this session")을 반환했다. 저장소 기록 C1.md §0의 같은 결과를 이번에 다시 확인했다. 자격 증명을 붙이는 `add_repo`는 시도하지 않았다(동의가 필요). 2026-10-16에 다시 시도한 결과도 같다: `https://github.com/nasa/CF/issues/184`와 `api.github.com/.../issues/184/comments` 모두 HTTP 403, GitHub 도구는 "repository nasa/cf is not configured for this session"을 반환했다. 같은 시각 `raw.githubusercontent.com`의 nasa/bp 고정 파일은 HTTP 200이었다 [M].
- 워크플로 실행 로그(GitHub Actions): 가져오지 않았다. 따라서 BP CI의 성공·실패와 실제 해석 값은 모른다.

### 11.2 읽은 기존 문서와 어긋나거나 보완하는 점(편집하지 않음)

| 문서 | 내용 | 이번 점검 |
|---|---|---|
| `cases/C1.md` §3·§11 A1 | BP CI 창의 cFS `main` 후보를 cFS 커밋 3개로 제시 | 재확인함. 추가: bplib `main` = `d4c6db9`(창 대부분. 창의 끝 5분 안에 `e7524239`·`314384e1`로 옮겨 가며 `e7524239`는 CFS 빌드의 `ut-functional` 문제를 고친다, §4.3), PSP 패치 브랜치 팁 후보 `bd4a2ea`와 fast-forward 관계, nasa/CF `main`이 수동 실행 기본값으로 창 내내 `281a941`(=`32a8b03`과 코드 동일) |
| `cases/C1.md` §6 | 우선순위 "CF 65, BP 55"를 나열 | 방향(BP가 더 높음)을 명시하지 않았다 → §3.6 |
| `cases/C1.md` §6 "Fix (833fdbb)" 행 | "about 2.4–2.5 s of delays" | 소스상 지연 호출 25번 = 명목 2500 ms, 마지막 조회는 명목 2400 ms 이후(§6.3). 벽시계 상한 아님 |
| 명세 App. A.4 단계 3 | 수정 전 구조를 `2a024d8`에서 읽음 | `281a941` 재독해 제공(§9.6) |
| 명세 §4.13 "Watchdog"·App. A.4 step 5 (rev 1.1 @`2499ad4`) | CF 감시 한도 = 재시도 수 × 지연 | rev 1.2 @`6be8935`에서 결정문 §3.3에 맞게 고쳐졌다(§9.6). 해소 |
| 명세 App. A.4 step 3 대 step 6 (rev 1.2 @`6be8935`) | `281a941`의 단일 조회 실패 시도: step 3은 NOT_READY_UNJUDGED, step 6은 VIOLATING | rev 1.3에서 해소: step 6도 NOT_READY_UNJUDGED, 요약 `INCONCLUSIVE(caller_requirement_absent)`(§9.6) |
| 명세 §2.3 `code_admission` 기록 (rev 1.2) | `15a871e`의 `cf_cfdp.c:1272-1278`만 인용 | `833fdbb`의 같은 주석은 L993–L999. anchor audit에서 다시 묶어야 한다(§9.3, §9.6) |
| `cases/C1.md` §7 "Statement" | "생성 호출이 반환한 뒤" | 이 문서는 요구를 **객체 존재**(id 확정)에 대해 형식화한다. ENV OSAL에서 확정은 생성 호출의 반환 앞에 일어나므로 반환은 충분조건이지 필요조건이 아니다(§1.2) |
| 로드맵 §8.3 | "현재 CF `15a871e`에서 재시도 상수 25, 지연 100 ms" | 맞다. 추가: `833fdbb`도 같은 값을 `cf_platform_cfg.h`에 직접 둠(§5.1), 접근 방식은 v7.0.1에서 `CF_INTERNAL_CFGVAL` |

## 12. 열린 항목과 PI 결정이 필요한 항목

| ID | 질문 | 선택지 | 이 문서의 입장 |
|---|---|---|---|
| Q-1 | ENV 위 파생 구성의 앱 집합과 시작 스크립트 | (a) BP의 6개 모듈(앱 5개 + `bplib` 라이브러리; ENV 앱 11개와 ENV 라이브러리 2개가 빠진다, DV-03)만 + BP 스크립트 / (b) ENV 15개 앱 + BP·bplib 추가 / (c) 기타 | 정하지 않음. 레퍼런스가 정하지 않는다(C1B-U08). (a)는 공개 기준에, (b)는 ENV 기준에 가깝다 |
| Q-2 | provider 구현 방식 | B-1(전부 이식) / B-2(초기화 경로만) / B-3(공개 스택 별도 재구성) / B-4(역할 기반) | 정하지 않음. 결정문 §3.2는 공개 BP 기반이므로 B-4는 기본안이 아니다. B-1·B-2의 비용은 §8.2 |
| Q-3 | 인스턴스 수 | 1(`cpu1`) / 2(`tx`,`rx`) | 정하지 않음. 초기화 관찰에는 1개로 시작 가능하다고 보지만 [I] 레퍼런스 선택이 아님 |
| Q-4 | bplib 고정 커밋과 단위 시험 플래그 `ENABLE_UNIT_TESTS`(§4.3, DV-13) | (a) **끔 + `d4c6db9`**(BP CI의 호출을 따름, ENV 플래그에서 이탈) / (b) **켬 + `e7524239`**(ENV 플래그 유지, bplib 커밋이 `d4c6db9`와 다름) / (c) 켬 + `d4c6db9` — bplib 자신의 커밋 메시지에 따라 빌드 실패가 기대되므로 선택지로 올리지 않고, 시험 빌드에서 확인될 때에만 기록 | 정하지 않음. B5의 단위 시험 플래그는 "끔"으로 적었다: BP CI의 호출이라는 레퍼런스에서 가져온 값이며 ENV 기준에서의 기록된 이탈이다(DV-13). **bplib 커밋에는 기본값을 두지 않는다**: `unspecified_by_reference`이며 Q-4 전에는 B2를 시작하지 않는다. PI가 (b)를 고르면 B5의 플래그도 따라 바꾼다 |
| Q-5 | **D22의 적용 순서의 첫 단계**(공개 통합 구성의 버전 호환성 확인)를 어떻게 할지(§8.0, §8.5) | (a) P-REC를 B0 앞에서 수행하고 결과(조립 성공/실패 지점)에 따라 분기 / (b) 이미 한 이름 수준 정적 대조(§5.3)로 갈음 — 이는 D22 순서의 "확인"을 실행 없이 진행하는 **예외**이므로 PI의 승인과 이유를 DV-00에 기록 / (c) ENV 이식을 PI가 직접 선택 — 같은 성격의 예외 | 정하지 않음. 이 문서는 (a)를 D22 순서의 문자 그대로의 적용으로 읽고 B0을 P0 뒤에 둔다. 정적 대조가 호환성 확인을 갈음한다고 주장하지 않는다. 승인이 없는 동안 ENV 이식을 "D22 기준 구성" 또는 "공개 코드 기반 기준"이라고 부르지 않는다 |
| Q-6 | (철회) 명세 §4.13의 감시 한도 문구 정정 | — | PI 질문이 아니다. 커밋된 명세 rev 1.2 @`6be8935`의 §4.13 "Watchdog"과 App. A.4 step 5에서 문구가 이미 결정문 §3.3에 맞게 고쳐져 해소됐다(§9.6) [M]. ID는 유지한다 |
| Q-7 | `bd4a2ea`가 창에서의 PSP 브랜치 팁이라는 [I]를 어떻게 다룰지 | 후보로만 사용 | 후보로만 사용 |
| Q-8 | #184 댓글 접근 | `add_repo`(동의 필요) / 사용자가 댓글을 붙여 줌 | 시도하지 않음 |
| Q-9 | "제어된 A2"(첫 `NAME_NOT_FOUND`에서 provider를 푸는 규칙 R-c)를 P-C1HOLD의 범위 확장으로 받아들일지(§9.2.1) | 받아들임(해제 규칙·표시·감시 한도를 새로 정함) / 받아들이지 않음(A2는 자연 실행에서만) | 정하지 않음. 결정문 §3.3의 해제 사건은 재시도 종료와 최종 초기화 결과뿐이므로 이 문서는 "받아들이지 않음"을 기본으로 쓴다. R-c는 A2를 보장하지도 않는다 |
| Q-10 | 명세 §4.13 "Release"의 일반 규칙 중 "다른 쪽이 막힌 경우"의 해제를 C1의 P-C1HOLD에 쓸지(§9.2 R-d) | 씀(무엇을 "막힘"으로 볼지, 그 판정 사건과 표시를 새로 정함) / 쓰지 않음(해제는 C-4·C-6과 C-6을 함의하는 C-7뿐, 막힘은 감시 한도로 `inconclusive(await-timeout)`) | 정하지 않음. Q-9와 같은 이유로 이 문서는 "쓰지 않음"을 기본으로 쓴다: CF가 막혔다는 것은 결정된 두 해제 사건의 어느 쪽도 아니다 |
| Q-11 | BP 자신의 조회 호출 지점(P-1, `bp_flow.c` L126)의 층 2 기대 상태(§9.3, C1B-U23) | (a) CF의 `caller` 기록이 κ 전체에 적용 → VIOLATING 가능 / (b) 호출 템플릿별 → NOT_READY_UNJUDGED / (c) "조회 후 없으면 생성"을 범위 밖으로 두는 규칙을 명세에 더함(이유 기록) | 정하지 않음. anchor audit가 먼저 판단하고, 명세가 정하지 못하면 PI 결정이다. 정해지기 전에는 BP 지점의 분류를 출력하지 않으며 CF의 집계와 섞지 않는다. C1 기대 결과표 동결 전에 필요하다. **2026-10-24:** SPEC rev 1.3 보충의 호출 문맥 규칙(마스터플랜 반영, MASTERPLAN_ADOPTION §2)으로 (c)의 일반형이 정해졌다. 패턴 제외가 아니라 출처로 정당화된 역할만 요구를 받는다. BP 지점은 요구 없음(§9.7 (b)). 남은 일은 프로필 선언과 anchor이다 |
| Q-12 | `281a941`의 수정 전 처리에 `caller` 기록이 있는지: CF 문서 `cfs_cf.dox` L212–L214의 종료 서술을 `exit` 받아들임으로 읽을지, 이슈 #184 본문("shouldn't be a hard error")을 그 판의 요구로 묶을지(§9.7 "판단할 수 없는 것" 1) | (a) 둘 다 묶지 않음 → NOT_READY_UNJUDGED 유지 / (b) 문서만 받아들임으로 묶음 → TOLERATED 가능 / (c) 둘 다 묶음 → 충돌, QT-5로 보류(NOT_READY_UNJUDGED, 요약 같음) | 정하지 않음. 출처는 종료를 서술할 뿐 받아들임 여부를 말하지 않는다. 결정 전까지 rev 1.3의 라벨(a)을 쓰고 바꾸지 않는다(마스터플랜 §5.2) |

다음 단계(순서만, 일정 없음): ① 위 Q 항목 결정(Q-5가 먼저 — 그 결과가 ②–③의 시작 조건이다) → ② **P0: D22 순서의 첫 단계**(Q-5에 따른 P-REC 수행 또는 PI가 승인한 대체; 결과와 실패 지점의 기록) → ③ P0이 "그대로 구성되지 않음"으로 기록되었거나 PI가 예외를 승인한 경우에만 B0–B2 준비(소스 배치, 작업 공간 분리) → ④ B3–B6 빌드·산출물 해시 → ⑤ B7 기본 기동 확인 → ⑥ 관측기 사건(§9.1)의 심볼·`uprobe` 가능성 확인 → ⑦ 파일럿으로 PV-1~PV-8 측정·동결 → ⑦′ C1 기대 결과표 동결(결정문 §3.7 [D]). 그 전에 G-REBIND(`833fdbb`의 `caller` 기록 다시 묶기, §9.3)와 Q-11(BP 호출 지점의 기대 상태; 2026-10-24 §9.7 (b))이 정해져 있어야 한다. 출처에서 도출한 기대값은 2026-10-24에 [expected_results_C1_R2.md](expected_results_C1_R2.md)에 먼저 동결했다 → ⑧ A1–A3 실행. ③의 B2는 Q-4(bplib 커밋)가 정해진 뒤에 시작한다. P0이 성공하면 ③–⑤는 그 구성(공개 통합 기준)에서 이어지고, ENV 이식은 비교용 파생 구성으로 PI가 필요를 정한다.

## 13. 이 문서의 검증을 다시 하는 방법

### 13.1 무엇을 어디서 다시 만들 수 있는가

[M] 항목의 입력은 두 종류이다.

- **공개 원격에서 SHA만으로 다시 만들 수 있는 것**: `git log`, `git diff`, `git diff --numstat`, `git ls-tree`, `git ls-remote`, 소스 줄 읽기. 저장소 독자도 아래 13.2의 명령으로 같은 값을 얻는다.
- **이 저장소에 들어 있지 않은 것**: (a) 사본 디렉터리 `/home/user/work/procrace/refs/`와 읽기 전용 `cfs_ref`는 저장소 밖에 있다. (b) `jphickey/cf`의 수정 브랜치 `fix-184-throttle-sem-race`는 원격에서 사라졌다(§4.2). (c) 이슈 #184 댓글과 Actions 실행 로그는 얻지 못했다(§11.1). 이 셋은 저장소만으로는 다시 만들 수 없다.

식별자 교차 점검(§5.3, §4.4)의 입력인 사본 목록, 스크립트 전문, 행마다의 정규식은 아래에 넣었다. 저장소 밖 파일에 기대지 않고 이 문서만으로 다시 돌릴 수 있다. 사본의 용량이 크므로 저장소에는 사본을 넣지 않는다.

### 13.2 사본 만들기와 스크립트

```text
# 기준 디렉터리: 아무 새 디렉터리 (이하 refs/). 스크립트는 사본 밖에 둔다.
# 블롭 없는 복제 + 고정 SHA의 작업 트리 (nasa/bp, nasa/CF, nasa/bplib, nasa/cFE, nasa/osal, nasa/PSP, nasa/cFS)
git clone --filter=blob:none --no-checkout https://github.com/nasa/bp refs/bp
git -C refs/bp worktree add --detach ../bp_b051eb0 b051eb0d12bfe76c0230215c2ff851cfaa553ce4
# (CF, bplib, cFE, osal, PSP도 같은 방식. jphickey/PSP는 remote로 추가해 techdev-iodriver를 fetch)
# ENV 사본: nasa/cFS @ 088b2fa828db9ff7e00733f1908e0eeb59f66ce3 와 서브모듈을 ENV.md 의 SHA로 (cfe/osal/psp)

# 버전 비교의 예 (SHA만 있으면 되는 값)
git -C refs/CF diff --numstat 281a941 833fdbb                       # 22/0, 21/2, 19/1
git -C refs/bplib log --first-parent --format='%h %cI %s' d4c6db9~1..origin/main
git -C refs/bplib diff d4c6db9 e7524239                             # CMakeLists.txt +6/-1
git -C refs/bp   log -1 --format='%H %cI %P' e871ad9               # BP PR #26 병합

# 식별자 교차 점검 (이름 수준만). -I 로 실행하고 사본 디렉터리 안에서 돌리지 않는다
python3 -I symcheck.py <label> '<정규식>' <앱 소스 디렉터리...> -- <대상 스택 디렉터리...>
```

사본 목록(작업 트리 이름 → 커밋). 형식은 원래 `MANIFEST_C1_20261015.txt`이다.

```text
# C1 basis refs manifest (2026-10-15). worktree path -> commit
bp_b051eb0	b051eb0d12bfe76c0230215c2ff851cfaa553ce4
CF_2a024d8	2a024d8efd9f44f54b7a59f7face24f255536236
CF_281a941	281a94188cd7d885a5aed01ee041f3ece2b0486f
CF_833fdbb	833fdbb27a26f15f2429e79392e7eb73d61abdea
CF_v7.0.1_15a871e	15a871e66cad7ec75e1be78989a93075ff142055
bplib_d4c6db9	d4c6db9f0d197f78967455fbc83bf1c5e5c31c9c
bplib_main_c5fb835	c5fb8358dd99149aa7ba463ed416d76049521a3a
cFE_fef4a0d	fef4a0dacabf7e59f054c8aa2c9160a2f3d8db83
cFE_6cccb3f	6cccb3f5d10664b5b10aeb9cfcda871ba2d99b32
osal_b44f4ce	b44f4ce74a309ff93c985539ee8816a2fe92ed23
osal_f34759d	f34759d6d6f7087e81afda94eccf7554e013cbe6
PSP_bd4a2ea_iodriver	bd4a2ea239cff013b24bf93a5e8c451dbcb7cf97
# blobless clones (no checkout): bp CF cFS cFE osal PSP bplib; PSP has remote jph=https://github.com/jphickey/PSP.git (fetched techdev-iodriver, jph-unsock-pspmod)
```

`symcheck.py` 전문(42줄):

```python
import re, sys, os
# usage: symcheck.py <label> <prefix-regex> <app_src_dir>... -- <env_dir>...
args = sys.argv[1:]
label = args[0]
pat = re.compile(args[1])
sep = args.index('--')
app_dirs = args[2:sep]
env_dirs = args[sep+1:]
def walk(dirs, exts):
    for d in dirs:
        for root, ds, fs in os.walk(d):
            ds[:] = [x for x in ds if x != '.git']
            for f in fs:
                if f.endswith(exts):
                    yield os.path.join(root, f)
used = {}
for p in walk(app_dirs, ('.c', '.h')):
    # skip unit tests
    if '/unit-test' in p or '/ut-' in p or '/ut_' in p or 'coverage' in p: continue
    try:
        txt = open(p, errors='replace').read()
    except Exception:
        continue
    for m in pat.finditer(txt):
        used.setdefault(m.group(0), set()).add(os.path.relpath(p))
envtxt = []
for p in walk(env_dirs, ('.c', '.h', '.cmake', '.in')):
    try:
        envtxt.append(open(p, errors='replace').read())
    except Exception:
        pass
envall = "\n".join(envtxt)
envids = set(re.findall(r'[A-Za-z_][A-Za-z0-9_]*', envall))
localdef=set()
for p2 in walk(app_dirs, ('.c','.h')):
    try: t=open(p2,errors='replace').read()
    except Exception: continue
    for m in re.finditer(r'#\s*define\s+([A-Za-z_][A-Za-z0-9_]*)', t): localdef.add(m.group(1))
missing = sorted(s for s in used if s not in envids and s not in localdef)
print(f"[{label}] distinct symbols used: {len(used)}; missing in ENV sources: {len(missing)}")
for s in missing:
    print("  MISSING", s, "<-", sorted(used[s])[:2])
```

### 13.3 행마다의 정규식과 인수

2026-10-15에 이 문서를 고치면서 아래 인수로 다시 돌려 §5.3·§4.4의 숫자(89, 91, 144, 11, 27, 30, 26)가 나오는 것을 확인했다. 정규식 문자열은 처음 점검 때 따로 기록하지 않았으므로, 같은 숫자를 내는 형태를 **다시 구성해** 적은 것이다 [M]. 경로의 접두는 위 사본 이름이며 `CI`는 `cFE_fef4a0d osal_b44f4ce PSP_bd4a2ea_iodriver cFE_6cccb3f osal_f34759d`, `ENV`는 `cfs_ref/cFS/cfe cfs_ref/cFS/osal cfs_ref/cFS/psp`를 뜻한다.

| 행 | label | 정규식(`python3 -I symcheck.py` 의 2번째 인수) | 앱 소스 디렉터리 | 대상 스택 | 확인한 출력 |
|---|---|---|---|---|---|
| SC-1 | CFpre | `\b(CFE\|OS)_[A-Za-z0-9_]+` | `CF_281a941/fsw` | CI | 89, 대상에 없음 1 (`CFE_SB_MAX_PIPE_DEPTH`) |
| SC-2 | CFpost | 같음 | `CF_833fdbb/fsw` | CI | 91, 1 (같은 이름) |
| SC-3 | CFpre-ENV / CFpost-ENV | 같음 | `CF_281a941/fsw` / `CF_833fdbb/fsw` | ENV | 89 / 91, 각 1 (같은 이름) |
| SC-4 | BP | `\b(CFE\|OS\|OSAL)_[A-Za-z0-9_]+` | `bp_b051eb0/fsw bp_b051eb0/.github/buildconfig` | CI | 144, 1 (`OS_OBJECT_ID_USER`) |
| SC-5 | BP-ENV | 같음 | 같음 | ENV | 144, 11 |
| SC-6 | bplib | `\b(CFE\|OS\|OSAL)_[A-Za-z0-9_]+` | `bplib_d4c6db9` | CI | 27, 0 |
| SC-7 | bpbplib | `\b(bplib_[a-z0-9_]+\|BPLIB_[A-Z0-9_]+\|bp_[a-z0-9_]+_t)\b` | `bp_b051eb0/fsw` | `bplib_d4c6db9` | 30, 0 |
| SC-7′ | bpbplibmain | 같음 | 같음 | `bplib_main_c5fb835` | 30, 26 |

(표의 정규식에서 `|`는 마크다운 표 때문에 `\|`로 적었다. 렌더링된 화면에서는 `|`로 보이고, 원문을 직접 복사할 때는 백슬래시를 빼고 쓴다.) SC-6의 정규식에서 `OSAL`을 뺀 `(CFE|OS)`로 돌리면 26이 나온다. 차이 하나는 `OSAL_LOCALTIME_SECS_AT_2000`이다(`OSAL_` 접두) [M: 두 정규식의 집합 차].

이 점검은 이름의 존재만 본다. 인수 형식, 매크로 의미, 컴파일 경고는 보지 않으며 빌드를 대신하지 않는다.

[M]은 위 사본과 명령으로 다시 만들 수 있다(13.1에 적은 세 가지 예외 제외). [I]와 [U]는 소스를 다시 읽어도 달라지지 않는다.

## 14. 검토 11건의 처리 (2026-10-15)

각 지적을 소스로 먼저 확인한 뒤 고쳤다. "확인"은 이번에 다시 읽거나 돌린 것이다.

| # | 심각도 | 지적 | 판정 | 확인 | 고친 곳 |
|---:|---|---|---|---|---|
| 1 | major | D22 순서(P-REC)가 선택이고 다음 단계에 없음 | 수용 | 결정문 §3.2의 적용 순서, 이 문서의 §5.3 요약·§8.5·Q-5·다음 단계에 P-REC가 선행 단계로 없었음 | §0 5항, §8.0, §8.3 P0 행, §8.5, DV-00, Q-5, 다음 단계, C1B-U20 |
| 2 | major | `d4c6db9` + ENV `-DENABLE_UNIT_TESTS=TRUE`, `e7524239` 누락 | 수용 | `git diff d4c6db9 e7524239`(CMakeLists +6/−1), PR 커밋 `88815b18`의 메시지, `d4c6db9` CMakeLists L36–L47·L210–L213, ENV.md L132, BP CI 호출 L127, 시각 UTC 환산(20:14:16Z, 20:14:58Z, 20:19:21Z) | §0 2항, §2, §4.2, §4.3, DV-13, B2·B5, §8.6, Q-4, C1B-U19 |
| 3 | major | C1 섭동 비율에 50 ms | 수용 | 명세 App. A.2 step 5의 "50 ms poll separation"은 App-record 창. §4.13 "Perturbation accounting"의 예는 다른 창의 값이고 C1은 없음. §6.1 표에서 CF·BP가 같은 지연을 같이 기다림 | §9.5, §9.4 PV-8, C1B-U18 |
| 4 | major | R-c·"제어된 A2"가 P-C1HOLD 밖, A3 행이 R-a와 충돌 | 수용 | 결정문 §3.3의 해제 사건, 명세 App. A.4 step 5. R-a는 C-4에서 풀므로 C-5·C-6 앞이다 | §9.2, §9.2.1, §9.3 A2·A3 행, PV-6, Q-9 |
| 5 | major | readiness precondition 열이 정의·TOLERATED와 모순 | 수용 | 명세 §2.3 `readiness K-REG.osal-countsem-lookup`의 `requires state(...) = LIVE`, 명세 App. A.4 step 6의 TOLERATED, ENV OSAL `osapi-idmap.c` L859와 `osapi-countsem.c` L106–L109(확정이 반환보다 앞섬) | §1.2, §9.3, §11.2 |
| 6 | minor | `+23/−2`, "ENV 앱 9개", D14의 절 | 수용 | `git diff --numstat 281a941 833fdbb` = 22/0, 21/2, 19/1. ENV.md L139–L158의 앱 15개, 겹침 4개, 제외 11개. D14는 결정문 §2 | 머리말, §5.1, §7.2, DV-03, Q-1 |
| 7 | minor | 명세 줄 번호, 이미 고쳐진 불일치, [F] 태그 | 수용 | 커밋 `2499ad4`의 명세에서 해당 줄 번호가 다른 문장을 가리킴. 작업 트리 판은 감시 한도 문구가 이미 바뀜 | §2의 인용 규칙, §9.2–§9.6, §11.2, Q-6 철회 |
| 8 | minor | `CFS_APP_PATH`가 이탈로 기록되지 않음 | 수용 | `build_cfs.sh` L142–L147, ENV CONDITIONS C4, `mission_build.cmake` L380–L384, BP CI common.yml L45–L46 | DV-16, B0, §8.2, §8.6 |
| 9 | minor | 이벤트 15가 "Found semaphore"에도 쓰임 | 수용 | `bp_flow.c` L126–L135, `bp_events.h` L41–L42 | §9.1 P-1·P-4, B7, §9.2 |
| 10 | minor | §1.3이 말한 PI 항목이 §12에 없음 | 수용 | §12는 Q-1~Q-8만 있었음 | §1.3 |
| 11 | minor | [M]이 저장소 밖 파일에 의존 | 부분 수용 | `symcheck.py`와 MANIFEST를 다시 돌려 숫자 재확인(§5.3) | §2, §5.3, §13 |

### Rejected findings

**11건 모두 수용했다. 통째로 기각한 지적은 없다.** 다음은 리뷰가 제안한 고침과 **다르게** 처리한 부분이다.

- **#1**: 리뷰의 두 선택지 중 첫째(P-REC를 B0 앞에 둠)를 문서의 순서로 썼다. 둘째(ENV 이식이 D22 순서에서의 예외)는 PI만 승인할 수 있으므로 Q-5의 (b)·(c)로 두었고 DV-00에 승인 출처를 적게 했다. 이 문서가 예외를 스스로 승인하지 않는다.
- **#2**: B5의 기본을 "끔"으로 썼다. 근거는 BP CI의 cmake 호출이다(레퍼런스). 리뷰가 말한 "bplib 고정 커밋의 명시적 결정"은 PI 결정이므로 Q-4에 (a) 끔 + `d4c6db9` / (b) 켬 + `e7524239`로 올렸다. "켬 + `d4c6db9`"는 빌드로 확인하지 않았기 때문에 "실패한다"고 쓰지 않고 "bplib 자신의 커밋 메시지에 따라 실패가 기대된다 [I]"로 썼다. 또 리뷰가 지적하지 않은 `314384e1`(20:14:58Z, 창 안)을 같은 표에 적었다. 변경은 단위 시험 파일뿐이다.
- **#4**: 리뷰의 두 선택지 중 "A2는 자연 실행에서만"을 택하고, R-c는 지우는 대신 결정 범위 밖의 제안으로 §9.2.1에 이유와 함께 남기고 Q-9로 올렸다.
- **#7**: "커밋 시점에 다시 확인하고 해소됐으면 삭제"는 이 문서의 작성자가 커밋 시점을 알 수 없다. 그래서 §9.6은 조건부 문장("커밋된 명세에 문구가 남아 있을 때에만 어긋남")으로 바꾸고 Q-6은 PI 질문이 아니므로 철회 표시했다(ID는 유지).
- **#9**: 리뷰는 "L128–L131"로 적었으나 파일에서 확인한 값은 조회 호출 L126–L127, 이벤트 L129–L130, `else if` L131이다. 파일의 값을 썼다.
- **#10**: 없는 Q를 더하지 않고 문장을 고쳤다. 원 보고자 환경의 재현은 로드맵 §8.1이 이미 "같다는 주장을 하지 않는다"로 정했고, A4는 로드맵 §8.3과 결정문 §3.3이 이미 정했다. *(개정 3의 정정: 이 출처 표시는 틀렸다. 로드맵 §8.1·§8.3은 `[제안]`이고 결정문 §3.3은 A4를 다루지 않는다. 결정된 근거는 결정문 §3.2와 명세 App. A.4 step 2·6이다. §1.3과 §16 #3을 보라.)*
- **#11**: 저장소에 새 파일을 만들지 않았다(이 작업은 이 문서 한 파일만 소유한다). 대신 스크립트 전문, 사본 목록, 행마다의 정규식을 §13에 넣었다. 정규식은 처음 점검 때 따로 적어 두지 않았으므로 같은 숫자를 내는 형태를 다시 구성했다고 §13.3에 밝혔다.

## 15. 개정 2의 변경 (2026-10-16)

이번 개정은 같은 작업("계속")에서 이 문서를 다시 점검한 결과이다. 사본(`/home/user/work/procrace/refs/`)의 작업 트리가 모두 MANIFEST의 SHA에 있고 변경이 없음을 확인했고 [M: `git rev-parse HEAD`, `git status --porcelain`], 식별자 대조 SC-1·SC-5·SC-7′을 다시 돌려 89/1, 144/11, 30/26을 다시 얻었다 [M]. 빌드·실행·설치는 하지 않았다.

| # | 바뀐 곳 | 내용 | 근거 |
|---:|---|---|---|
| 1 | §2 인용 규칙, §1.2, §9.2–§9.6, §11.2 | 명세 인용 기준을 rev 1.1 @`2499ad4`에서 커밋된 rev 1.2 @`6be8935`로 옮겼다 | `6be8935`의 SPEC_FINAL 머리말 "Revision 1.2"; 작업 트리의 미커밋 변경은 인용한 절을 건드리지 않음 [M] |
| 2 | §9.6, §11.2 | 감시 한도 문구의 어긋남을 "해소"로 바꿨다 | rev 1.2 §4.13 "Watchdog", App. A.4 step 5 [M: `grep`] |
| 3 | §1.2, §9.3, §9.6, C1B-U21 | 시도 분류에 NOT_READY_UNJUDGED를 더하고 TOLERATED의 조건(`caller` 기록)을 적었다. `833fdbb`의 인정 주석 위치(L993–L999)와 명세 인용(`15a871e` L1272–L1278)의 차이, step 3과 step 6의 `281a941` 표기 차이를 보고했다(고치지 않음) | rev 1.2 §2.3 "Caller-level Allowed", App. A.4 step 3·6; CF 소스 `grep` [F] |
| 4 | §5.4, §9.1 P-E, §9.3, B7 | BP `AppInit`이 생성 호출 전에 반환하는 경로(`bplib_init`·경로 표 할당 실패 등)와, 생성 실패가 BP 초기화 실패로 이어지지 않는다는 점을 소스에서 읽어 더했다. 이 경우를 A3·A4와 섞지 않는다 | `bp_app.c` L112–L124, L154–L211, L804–L844; `bp_flow.c` L121–L143, L519–L530; `bp_events.h` L34 [F] |
| 5 | §9.1 P-2a~P-2c, C1B-U22 | 로드맵 §8.2의 "이름 공개·객체 사용 가능 시점"에 대응하는 OSAL 내부 사건을 더했다 | ENV OSAL `osapi-countsem.c` L94–L106, `os-shared-idmap.h` L106–L111, `osapi-idmap.c` L838–L859 [F] |
| 6 | §1.2 | CI 후보 OSAL `b44f4ce`·`f34759d`에서도 id 확정이 생성 호출의 반환보다 앞선다는 것을 확인해 [U]를 지웠다 | 같은 함수의 줄 대조와 `OS_ObjectIdConvertToken` 본문 `diff` [F/M] |
| 7 | §9.2 | 나중에 생성되는 provider 스레드에 대한 clone 시 자동 연결과 그 정지의 기록을 더했다 | rev 1.2 §4.13 "Attach strategy", App. A.4 step 5 [D] |
| 8 | §11.1 | #184 댓글 접근을 다시 시도했다. 여전히 HTTP 403이다 | [M] |

바뀌지 않은 판단: D22의 적용 순서(P0 먼저, Q-5), B5의 단위 시험 플래그 기본 "끔"(Q-4), A2는 자연 실행에서만(Q-9), 모든 파일럿 값 미정(§9.4).

## 16. 개정 3: 검토 11건의 처리 (2026-10-16)

각 지적을 원출처로 먼저 확인한 뒤 고쳤다. 명세는 커밋된 `6be8935`의 `SPEC_FINAL.md`를 `git show`로 읽었다. 빌드·실행·설치는 하지 않았다.

| # | 심각도 | 지적 | 판정 | 확인 | 고친 곳 |
|---:|---|---|---|---|---|
| 1 | major | NOT_READY_UNJUDGED 시도에 "readiness violation"을 씀 | 수용 | 명세 §0("An in-scope occurrence rejected by the caller-level Allowed is a readiness violation"), §0.11 item 2("a `VIOLATING` occurrence is a readiness violation"), §1.6의 bad 정의, §1.10 "Outcome label"(`violation` / `not-ready-unjudged`) | §1.2, §9.3(정의 문단, A3·A4 행), §9.6 |
| 2 | major | 기본 분류대로라면 A2 실행이 나쁜 실행인데 좋은 쪽으로만 다룸 | 수용(선택 1) | 명세 §1.6 "A finite execution is bad … where Allowed is `unspecified_by_reference`, a `NOT_READY_UNJUDGED` one"; App. A.4 step 6의 다시 묶기 문장; 결정문 §3.7의 "기대 결과표는 첫 출력 전에 동결" | §9.3 G-REBIND 문단, A2 행, 해석 규칙, §9.6, C1B-U21, 다음 단계 ⑦′, §0 6·7항 |
| 3 | major | 로드맵 `[제안]`과 없는 결정(§3.3의 A4)을 정해진 결정처럼 인용 | 수용 | 로드맵 §1.1("작업 지시, 산출물 이름, 완료 기준, 순서 … 모두 [제안]"), §8.1의 `[제안]` 목록, 결정문 §3.3 전문(A4 언급 없음), 명세 App. A.4 step 2·6 | 머리말 "입력 계획", §0 5항, §1.3의 두 행, §8.0, §9.1, §9.3 머리와 해석 규칙, C1B-U12, §14 #10에 정정 표시 |
| 4 | minor | R-d("막힌 경우")가 결정된 해제 사건 밖 | 수용 | 명세 §4.13 "Release"; 결정문 §3.3의 두 해제 사건; CF의 `CFE_ES_ExitApp`은 `CF_Init` 반환 뒤에만 불림(`cf_app.c` L330–L338, L366) | §9.2 R-d, §12 Q-10, §1.3 |
| 5 | minor | R-a 뒤 P-2가 C-6보다 앞선다는 근거 없는 서술 | 수용 | `833fdbb` `cf_cfdp.c` L1011(마지막 실패 뒤의 `OS_TaskDelay`), L1016–L1018(이벤트) | §9.2 R-a |
| 6 | minor | 앞 채널 실패 뒤의 키에 결과가 없음 | 수용 | `281a941` L1000, `833fdbb` L1019의 `break`가 채널 루프(L967)를 빠져나감; L977·L988도 같음; BP의 이름 순서 생성 `bp_flow.c` L124–L142 [F] | §9.2, §9.3 표의 "도달하지 않음" 행과 해석 규칙 |
| 7 | minor | BP 자신의 조회(P-1)가 같은 readiness specification의 사례인데 분류가 없음 | 수용(일부 다름, 아래) | 명세 §2.3 `use op OS_CountSemGetIdByName`; §1.6의 인스턴스 `(κ, t, key, scope)`와 "Side of a provenance record"의 "(record, κ)" 규칙; `bp_flow.c` L126 | §9.3 "BP 호출 지점의 인스턴스", C1B-U23, Q-11, §1.3 |
| 8 | minor | Q-6 행이 낡음 | 수용 | `6be8935`의 §4.13 "Watchdog"("The retry count times the delay is not by itself taken as a safe wall-clock waiting limit")와 App. A.4 step 5("Their product is not taken …") [M] | §12 Q-6 |
| 9 | minor | 두 빌드 트리의 `CMakeCache.txt` 차이가 `CFS_APP_PATH` 하나일 수 없음 | 수용 | ENV `build-native_std/CMakeCache.txt`에 빌드 디렉터리 경로를 값으로 갖는 항목 18개(주석 제외), 앱 소스 경로 항목(`CF_DOCS_SOURCE_DIR` L60) [M: `grep`] | B5 |
| 10 | minor | B2가 bplib 기본 커밋을 레퍼런스 없이 고름 | 수용 | §4.3에서 `d4c6db9`는 추론된 후보 [I]; CONDITIONS_POLICY | B2, §8.4 뒤 문단, Q-4, 다음 단계 |
| 11 | minor | D-4의 파일이 틀림 | 수용 | D-4 행은 `env/CONDITIONS.md` L123. `env/ENV.md`는 L16·L296에서 가리킬 뿐 | §8.1 |

검토 밖에서 함께 고친 것: 이 문서가 "Caller-level Allowed"를 명세 §2.3으로 인용했으나 그 문단은 §1.6에 있다(`6be8935`에서 §1.6은 L363–L449, 문단은 L403). §1.2와 §9.3의 인용을 §1.6으로 고쳤다. §15 #3의 같은 인용은 개정 기록이므로 그대로 둔다.

### Rejected findings

**통째로 기각한 지적은 없다.** 다음은 검토가 제안한 고침과 **다르게** 처리한 부분이다.

- **#2**: 두 선택지 중 선택 1(다시 묶기를 C1 출력 전 선행 조건으로)을 택했다. 다만 이 문서는 anchor audit의 결과를 미리 정할 수 없으므로, 다시 묶기가 **거부되는** 경우에 선택 2의 분류로 기대 결과표를 첫 출력 전에 다시 쓴다는 규칙을 함께 적었다. 선행 조건은 `833fdbb`에만 걸린다(`281a941`에는 다시 묶을 주석이 없다).
- **#4**: 검토의 두 선택지를 함께 썼다: R-d를 CF 종료(C-7)로 줄였고, "막힌 경우"는 Q-10으로 올렸다. C-7을 남긴 근거는 C-7이 C-6을 함의한다는 소스 사실이다.
- **#7**: 검토는 엄격 대안 아래에서 BP 지점에 NOT_READY_UNJUDGED가 생긴다고 적었다. 커밋된 명세를 읽으면 그것은 두 읽기 중 하나이다. `caller` 기록의 쪽이 "(record, κ)" 쌍에 대해 정해지므로, CF의 기록이 κ의 Allowed 항목 전체를 감시기로 만든다고 읽으면 재시도가 없는 BP 지점은 VIOLATING이 될 수도 있다 [I]. 이 문서는 어느 쪽인지 정하지 않고 Q-11과 C1B-U23으로 올렸다. 두 읽기 모두 정상 첫 기동마다 나쁜 실행을 만든다는 점은 검토와 같다.
- **#9**: 정규화 비교(빌드 디렉터리와 `CFS_APP_PATH` 디렉터리를 자리표시로 바꿈)를 완료 조건으로 삼았다. 정규화 전에 달라지는 항목은 ENV 캐시에서 확인한 범주로만 적었다. 파생 빌드의 정확한 항목 목록은 구성 전에는 모르므로 [U]로 두었다.
- **#10**: bplib 기본값을 지우는 것에 더해, Q-4가 정해지기 전에는 B2를 시작하지 않는다고 적었다. 값을 고르지 않은 채 B2를 진행할 수 없기 때문이다.

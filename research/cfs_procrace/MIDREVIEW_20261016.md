# 중간 검토 자료 (2026-10-16)

연구: Process-Level Race Detection in core Flight System
기준 커밋: `f41b84c` (브랜치 `claude/busy-shannon-y903qc`)
입력: `Architecture/spec/PI_DECISIONS_RESOLVED_20261015.md`(결정 결과), `plan/ROADMAP_20261015.md`(로드맵)

두 작업 흐름을 각각 **현재 단계까지만 마치고 멈췄다.** 멈춘 뒤 다음 단계에서 바뀐 파일은 없다.

## 0. 한눈에

| 흐름 | 마친 단계 | 멈춘 지점(아직 안 함) |
|---|---|---|
| 문서·결정 반영 | SPEC rev 1.2 반영(결정, 용어 변경, CORD 인용 제거), 한국어 문서 01–04, 계획 문서(`plan/`) 작성과 검토·수정 | 한국어 문서 05–10, 배경 보고서, 그림, 환경 스크립트의 CORD 정리, 문서 간 일관성 점검 |
| R2 구현 | K0 기반, K1 Φ 규칙·평가기, K2 관측기(실제 cFS 1회 관측), K4a 소스 사실 추출. 각각 적대적 검토 후 수정 | K3 trace 판정, K4b 정적 분석·인증서, K4c 독립 checker, 통합 실행, 최종 검토 |

## 1. 문서·결정 반영

### 1.1 SPEC rev 1.2 (`Architecture/spec/SPEC_FINAL.md`)

- **결정 반영.** 10월 15일 결정을 모두 반영했다. 위치는 §0.10의 결정별 대응표와 ADR-045–052다. `PI_DECISIONS.md`에는 결정마다 값·날짜·출처를 적었다. 결정 문서가 다루지 않은 항목(D18의 C1 외 부분, P-TRACEFS, D8/P-RECALL 세부)은 열린 항목으로 남겼다.
- **비교 대상 변경.** 실험 비교는 기존 Process-Level 도구 8개(RacePro, SimRacer, SIMEXPLORER, RRF, DESCRY, SimEvo, ReDPro, SysPro)로 한정했다. 여섯 가지 분석 질문과 여섯 결과 상태(`NOT_RUN` … `REPORTED`)로 비교한다. thread data-race 도구와 GPredict·HB·lockset식 재구현은 관련 연구로만 둔다. MLIR 단계(S8)는 로드맵에서 뺐다.
- **형식 정정** (로드맵 §3의 최우선 항목).
  - 전이 규칙 2와 σ_E 문장을 L-PR이 실제로 증명하는 범위로 줄였다. 조기 반환(`NOT_EVALUATED`)도 포함한다.
  - 신뢰 원장에 L-CONT(c)를 넣었다. R2 인증서처럼 실행으로 도달을 주장하는 모든 칸에 해당한다.
  - 보조정리마다 **증명 상태**(proved / depends-on-stated-assumption / unproved)를 붙였다. 의무를 목록에 적었다는 이유만으로 "증명됨"을 출력하지 않는 규칙도 넣었다.
  - 현재 R2 인증서 칸은 모두 **"lemma status: unproved"**로 출력된다. 정리별 상태는 `plan/proof_status_R2.md`, 정정 내역은 `plan/formal_corrections.md`에 있다.
- **용어 변경 (T1 = a).**
  - 판정 대상은 readiness specification(준비 상태 명세), `Req`는 readiness precondition, 위반은 readiness violation이다.
  - 근거 등급은 contract-backed / requirement-derived / admitted / issue-derived / code-derived / researcher / unreferenced다.
  - `tolerated`는 `allowed-not-ready`로 바꿨다.
  - "API contract"는 문서화된 API 의미에만 남겼다. 이전 이름과 새 이름의 대응표는 부록 C에 있다.
- **CORD 인용 제거 (D12).**
  - `[R2c]` 항목과 그 인용을 모두 지웠다. 근거는 gdb 기록이 직접 뒷받침하는 사실만 남겼다: 실행당 24회(ES 20, EVS 4), 정적 지점 3, 문맥 6, 223/229/229.
  - 11회 구성 등 CORD에서만 나온 수치는 지웠다.
  - 버전 테이블에 따라 24회와 11회가 갈리는 의존성은 관측 사실로 쓰지 않는다. 새 기록기가 확인할 구성 사실([I])로만 둔다.
- **R2 표현.** "이벤트 손실"을 쓰지 않고 "이벤트 요청 거부"(`event_request_rejected`)로 바꿨다.
- **D11.** 커널이 다시 바뀌었다(`-fc-v114` → 현재 `-fc-v139`). 재측정 대상은 현재 커널로 고쳤다.

### 1.2 한국어 문서

01–04는 rev 1.2에 맞췄다. 04에는 증명 상태 규칙 PS1–PS4, 신뢰 원장의 L-CONT, 용어 변경, 엄격한 QT-4/Q15가 들어갔다. **05–10, 배경 보고서, 그림은 아직 rev 1.1 상태다.**

### 1.3 계획 문서 (`plan/`, 모두 검토 1회 후 수정)

| 문서 | 내용 |
|---|---|
| `case_basis_register.md` | 다섯 사례의 조건표. 필드 9개마다 출처 종류를 표시했고, 기대 결과표 골격과 "첫 출력 전 해시 저장" 규칙을 넣었다 |
| `C1_basis_plan.md` | 공개 BP 구성(`BP0_tsem`/`BP1_tsem`, CF→BP 순서, 우선순위 65/55)을 고정 커밋에서 확인했다. CF `281a941`→`833fdbb` 호환성과 버전별 재시도 상수, 빌드 계획, 사건 기준 보류 설계(A1–A5), 레퍼런스 없는 조건 목록을 담았다 |
| `case_setup_C2_C3_C5.md` | C2 역할 재구성의 2×2 설계와 C3·C5의 6.4.1/6.4.2 분석 기준(헤더 차이를 deviation으로 기록), Q16 대응표 골격, `58abd38d` 보조 쌍 |
| `process_race_method_matrix.md` + `methods/*.md` | 도구 8개 × 사례 5개 × 질문 6개. 전문을 읽은 도구는 6개다(RacePro, RRF, DESCRY, SimEvo, SysPro, SIMEXPLORER). SimRacer는 초록만, ReDPro는 초록과 2차 자료만 읽었다. 200칸 중 132칸은 설계 분석, 68칸은 판단 불가다. 어느 칸에도 "미탐지"라고 쓰지 않았다 |
| `formal_corrections.md`, `proof_status_R2.md` | 형식 정정 내역과 R2 결과에 쓰이는 정리의 전제·결론·증명 상태 |

## 2. R2 구현 (`kairos/`, C/C++)

빌드 트리는 저장소 밖 `/home/user/work/kairos-build/`에 있다. 모든 테스트는 gcc 13, clang 18, ASan/UBSan 빌드에서 통과했다.

| 구성요소 | 상태 | 핵심 |
|---|---|---|
| **K0 기반** | 완료, 검토 반영 | X-macro 스키마(레코드 41종), 컨테이너, SHA-256, 결정적 출력, golden 시험. lint 5종: 사례 ID, `contract` 식별자, Python/JSON, core 안의 LLVM, 생성 코드를 막는다 |
| **K1 Φ·평가기** | 완료, 검토 반영 | `.kzp` 문법과 파서, 평가기 하나에 구체·추상 domain 두 개(포함 성질 시험을 통과했고, 일부러 넣은 오류 10개를 모두 잡음), R2 규칙 부분집합. 출처 기록은 109개이고, 인용 98개는 고정 소스와 hash로 대조했다. **엄격한 QT-4**를 반영해, 근거 등급이 낮은 기록에서는 caller side를 거부한다 |
| **K2 관측기** | 완료, 검토 반영 | 수정하지 않은 바이너리를 perf_event_open uprobe로 관측한다. 붙이기 전에 바이너리 hash·build-id·probe 대상 바이트를 확인하고, 손실 집계와 GetAppID 없는 task 식별을 한다 |
| **K4a 소스 추출** | 완료, 검토 반영 | Clang으로 사실을 추출한다. 결과는 아래 2.2 |

### 2.1 실제 cFS 관측 1회 (`env/logs/s1r2_p1*`, 10/15, 당시 커널 `-fc-v114`)

- **조건.** `run_cfs.sh`로 실행했다. 실행 사용자는 `ubuntu`, 바이너리는 기준 build와 같은 sha256 `0edb1ae2…`이고, root 관측기가 붙었다.
- **결과.** AppId 0을 전달한 `CFE_EVS_SendEventWithAppID` 요청 **24회**가 관측됐고, 대응하는 반환값은 모두 `CFE_EVS_APP_ILLEGAL_APP_ID`였다. 이 요청들의 timestamp는 SB의 AppId 저장 함수를 호출한 뒤의 관측점보다 앞섰다(한 시계의 순서이며 happens-before가 아니다). 실제 저장과의 선후와 원인 설명에는 값의 전달 경로와 task 간 순서 근거를 함께 쓴다. *(2026-10-22 정정: 처음 문안 "SB AppId 저장 전에 24회 호출"은 관측점의 의미보다 강했다. 2026-10-19 검토 §3.2.)*
  - task별: ES 20, EVS 4.
  - 호출 함수별: CreatePipe 2 / SubscribeFull 4 / ReportSingleEvent 18.
  - 모두 AppId 0이었고, 반환값은 `0xc2000003`(ILLEGAL_APP_ID)였다.
  - 이 값들은 gdb 기록과 같다. **하지만 같다는 것이 증명은 아니다.**
- **알려진 차이와 한계.**
  - **원인을 조사하지 않은 불일치:** 전체 송신 수는 271회로, gdb의 251/257/257과 다르다.
  - **횟수:** 실행은 1회다(기준 gdb는 3회).
  - **환경 차이:** 관측 script가 변수 `KZ`를 cFS 환경에 남겼다. 지금은 script를 고쳤고, 문서에도 정정했다.
  - **도구 이력:** 이 실행에 쓴 kz 바이너리는 다시 만들 수 없다. 이후 실행부터는 커밋과 diff를 기록하게 했다.
  - **손실 집계:** `PERF_RECORD_LOST`는 probe 손실의 상한값일 뿐이다.
  - **관측 범위:** uprobe는 파일 단위로 걸리므로, 같은 파일을 실행하는 다른 프로세스도 기록된다.
- 빈도나 오버헤드는 주장하지 않는다.

### 2.2 정적 추출 결과 (core-cpu1, cFS 미실행)

- 규모는 번역 단위 168개, 함수 1,591개, loop 257개다.
- **`CFE_SB_Global.AppId`의 writer는 3개다:** `cfe_sb_init.c:50`의 memset, 그리고 `cfe_sb_task.c:138`의 GetAppID out-pointer 저장 2개.
  - 이 field를 우회해서 쓰는 형 불안전 write(A-TYPE)는 0건이다. 따라서 writer 집합은 **완전하다고 판정됐다.** 단, 이 판정은 가정 G1–G4에 기댄다.
- SB에서 EVS로 이 field를 넘기는 정적 호출 지점은 39곳이다. 이 숫자는 gdb 문서의 정적 집계와 같고, 위반이 관측된 3곳이 모두 그 안에 있다.
- core 표를 따라가면 시작 순서는 ES → EVS → SB → TBL → TIME이다.
- `CFE_ES_MainTaskSyncDelay`는 await로 인식됐다. 다만 L1은 증명되지 않았다.

## 3. 검토해 주셨으면 하는 점

1. **SPEC rev 1.2의 방향.** 비교 범위, 용어 변경, 증명 상태 표기, CORD 인용 제거가 의도와 맞는지.
2. **계획 문서 4개.** 특히 다음 둘을 봐 주세요.
   - `C1_basis_plan.md`의 빌드 계획: ENV 번들에 BP·CF를 넣는 방법과, 그때 생기는 deviation.
   - `case_setup_C2_C3_C5.md`의 C2 2×2 설계.
3. **관측 결과의 해석.** 24회 집합이 맞는다는 것을 어디까지 주장할지. 271 대 251/257 불일치를 다음 단계(통합 실행)에서 조사하는 순서.
4. **다음 단계 진행 여부.** 아래 항목이 아직 남아 있습니다. 진행해도 될지 정해 주세요.
   - 구현: K3 trace 판정 → K4b 인증서 → K4c checker → 통합 실행
   - 문서: 05–10, 그림, 환경 스크립트의 CORD 정리, 일관성 점검
   - 커널이 바뀌었으므로(`-fc-v139`), 통합 실행 전에 기준 실행을 다시 측정합니다(D11).

## 4. 환경과 처리하지 못한 일

- **CORD 산출물 85개는 아직 작업 트리에 있다.** `git rm`이 자동 권한 검사에 두 번 막혔다. 삭제를 허용하시거나 Bash 권한 규칙을 추가해 주셔야 지울 수 있다. 목록: `/tmp/claude-0/-home-user-Shape-Perf/e983f27d-f015-5a5d-b912-399245630e78/scratchpad/cord_delete_list.txt`
- **디스크.** 작업 중 99%(여유 428MB)까지 찼다. 검토용 빌드 사본 3.3GB를 지워 지금은 여유가 3.7GB다. 남은 빌드 트리(`kairos-build`, 약 8GB)에는 중간 사본이 많아, 다음 단계 전에 정리가 필요하다.
- **사용량 한도.** 작업 중 세 번 걸렸다. 그때마다 이어서 진행했고, 잃은 결과는 없다.

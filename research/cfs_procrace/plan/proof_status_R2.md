# R2 첫 결과의 증명 상태표 (2026-10-15)

> **2026-10-22 지위 변경 (SPEC rev 1.3, ADR-053, ADR-057):** 이 표가 다루는 R2 인증서는 **선택 경로**가 되었다. R2는 관측 판정 경로의 기반 검증(V1′)이고, 인증서는 논문이 "R2의 모든 도달한 발생이 위반"이라는 보편 진술을 할 때만 만든다. 표의 증명 상태 분류는 그 경로에 대해 그대로 유효하다. **기록 시점 정정:** 아래 R-4·R-5와 §8의 "Kairos 자체 기록기 미실행"은 2026-10-15 작성 시점의 상태다. 같은 날 관측기 연습 실행 s1r2_p1이 한 번 있었고(`kairos/docs/observer_smoke_run.md`), 사용별 집합 24회(ES 20, EVS 4)를 집계했다. 그러나 그 실행은 도구 원본을 되찾을 수 없는 개발 기록이며(N=1), S1-R2 게이트의 재관측이 아니다. 게이트는 고정 조건의 새 실행 묶음으로 한다.

- 대상: R2(cFE #2663)의 첫 Kairos 결과 — ES·EVS 반쪽의 FIXED_ORDER 인증서(셀 (σ_U, M_api), (σ_U, M_impl))와 관측된 사용별 집합(per-use set).
- 근거 문서: `Architecture/spec/SPEC_FINAL.md` (§4.5, §4.5.1, §6.1, §6.2의 증명 상태 정의와 표, App. A.1 step 1–15, §9.3 V1), `plan/ROADMAP_20261015.md` §3·§5.3, `Architecture/spec/PI_DECISIONS_RESOLVED_20261015.md` §2.1 (D20의 추가 완료 조건).
- 성격: **증명 상태의 분류표**이다. 이 단계에서 Kairos는 구현되지 않았고 인증서도 발급된 적이 없다. 표는 SPEC App. A.1이 기대하는 인증서 한 벌이 쓰는 항목을 나열하고 각 항목의 증명 상태를 붙인 것이다. 모든 '기대'는 [I]이며 실행하지 않았다.
- 표시: **[F]** 출처가 있는 사실, **[M]** 측정, **[I]** 추론, **[D]** 결정, **[U]** 미확인.
- 용어(T1 = (a)): 이 문서는 '준비 상태 명세'(SPEC의 contract), '준비 상태 전제'(SPEC의 `Req`), '준비 상태 위반'을 쓴다. SPEC 본문은 아직 이름을 바꾸지 않았으므로 SPEC의 식별자(`K-ID.send-with-appid` 등)와 인용은 그대로 둔다.
- 근거 규칙: CORD 결과는 쓰지 않는다. R2의 관측 근거는 gdb 원시 기록(`env/logs/r2_g1..3_*`, `results/R2_gdb_v701.md`)뿐이다.

## 0. 먼저 읽을 것

1. **증명 상태 값** (SPEC §6.2에 한 곳으로 정의. 여기는 요약):
   - `proved` — 명세에 완전한 증명이 있고 §1의 정의와 `proved`인 보조정리만 쓴다.
   - `depends-on-stated-assumption` (명시적 가정에 의존) — 논증이 §6.1이 id로 적은 가정 아래에서 완전하다.
   - `unproved` (미증명) — 그 밖의 모든 경우. 스케치에 주장만 있는 단계가 있거나, 의무·[I]로 표시되었거나, `unproved` 보조정리를 쓴다.
   - 여러 보조정리를 쓰면 가장 약한 것이 그 진술의 상태이다. **의무로 목록에 적혔다는 이유만으로 '증명됨'이라고 출력하지 않는다** (SPEC §6.2 PS2).
2. **이 표의 상태는 작성자의 분류이다.** 독립 검토는 아직 없다 [I]. 상태는 증명을 쓰는 변경에서만 올라가고 검토에서 틈이 나오면 내려간다 (PS1). V0-P가 검토를 기록하는 단계이다.
3. **'원장 위치'** 는 SPEC §4.5.1의 네 부분이다. (i) 체커가 다시 확인한 추론 단계, (ii) 체커가 읽었지만 신뢰한 추출 사실(`facts.kz`, `skel.kz`, `anchors.kz`, 닫힘 목록, 실행·탐침 기록), (iii) 신뢰 항목(Φ 항목, `DECLARED_TRUSTED`, 가정, 보조정리, 도구 기반), (iv) 반증 가능하지만 증명되지 않은 항목(의무 O1–O10과 그 상태). 결정문 §2.1의 "체커가 확인한 추론 단계와 체커가 신뢰한 추출 사실·공유 코드·가정을 구분한다"를 이 네 부분이 구현한다.
4. **'체커가 확인했다'의 뜻**: `kz proofcheck`는 (ii)·(iii)에 상대적으로 추론 단계를 다시 확인한다. 추출 사실, 골격, 닫힘 목록, Φ, 공유 `libkz-core`는 신뢰하며, 일부 단계는 공유 `libkz-core`(T5)의 평가기·규칙 엔진을 호출하므로 'via T5'로 표시된다. 독립 검증했다고 쓰지 않는다 (SPEC §6.4 항목 31).

## 1. 첫 R2 결과가 말하는 것

| # | 결과 문장 (SPEC App. A.1 step 9, 15의 기대) | 상태 |
|---|---|---|
| R-1 | ES 반쪽, 셀 (σ_U, M_impl): b = ∞에서 `¬OK_i`. 곧 CFE_ES_TaskInit 문맥의 `CFE_EVS_SendEventWithAppID` 사용(SB 이벤트)의 판정된 범위 안 발생 중 `READY`·`TOLERATED`가 있는 유한 실행이 없다. `Allowed = none`이므로 준비 안 됨 발생은 결정 시점에 `VIOLATING`이고 결정 경로(deciding)가 있다 → `fixed-order` (안전성 진술) | 인증서 없음 (구현 전). 아래 표가 쓸 항목 |
| R-2 | ES 반쪽, 셀 (σ_U, M_api): 위와 같은 진술 | 같음 |
| R-3 | EVS 반쪽, (σ_U, M_impl)·(σ_U, M_api): 같은 진술. CFE_EVS_TaskInit 문맥. A-FRESH, A-OWNER(`DECLARED_TRUSTED`)를 추가로 씀 | 같음 |
| R-4 | 도달(reach): (σ_U, M_impl)은 S1이 관측한 실행 하나가 있으면 `proved`. (σ_U, M_api)는 cone이 task 생성을 포함하므로 M_api 적합성 검사를 통과해야 `proved`이고 아니면 `assumed` | 실행 없음 (S1 구현 전) |
| R-5 | 사용별 집합 관측: ES 20 = `SEND_NO_SUBS` 17 + `SUBSCRIPTION_RCVD` 2 + `PIPE_ADDED` 1, EVS 4 = 1 + 2 + 1, 합 24회 (한 부팅). Kairos 자체 기록기가 같은 집합을 재관측한다 (S1-R2 게이트) | 현재는 gdb 3회 기록뿐 [M] |
| R-6 | 진행성: 모든 셀의 진행 결과는 `unknown(not_checked)` (인증서는 진행성을 주장하지 않는다) | — |

**이 결과가 말하지 않는 것** (SPEC A.1 step 14, §6.4): R2가 'race'라는 것, 반복 실행으로 고정 순서가 증명되었다는 것, 사용이 모든 실행에서 도달된다는 것, 진행성·교착 부재, 체커가 추출 사실이나 골격을 검증했다는 것, 다른 셀(σ_U∞, σ_R1, σ_E)에 대한 진술(V0-P가 전이 보조정리를 쓰기 전까지는 하지 않는다), Kairos가 R2를 탐지했다는 것(Kairos는 구현되지 않았다).

**표현.** EVS의 `CFE_EVS_SendEventWithAppID`는 `AppID`가 유효하지 않으면 패킷을 만들기 전에 `CFE_EVS_APP_ILLEGAL_APP_ID`를 반환한다 [F: cFE `c5fb2b4d` `cfe_evs.c` L185–L188, 패킷 생성 `EVS_GenerateEventTelemetry`는 L204의 뒤쪽 가지. 이번 단계에서 다시 읽음]. 따라서 24회는 **이벤트 요청 24회가 거부됨**이다. 이미 만들어진 패킷 24개의 손실이 아니다. 24회는 한 원인에서 나온 반복 호출이며 독립 결함 24개가 아니다 (`results/R2_gdb_v701.md` §6).

## 2. 보조정리 (SPEC §6.2의 표 중 R2 첫 결과가 쓰는 것)

열: **전제** → **결론** / **상태** / **의존** (가정·다른 보조정리) / **R2에서 쓰이는 곳** / **원장 위치** (모든 보조정리 항목은 (iii)의 `LEMMA` 종류이며 `proof` 필드를 갖는다).

### 2.1 인증서 자신의 셀 (σ_U, M_api), (σ_U, M_impl)

| 보조정리 | 전제 → 결론 | 상태 | 의존 | R2에서 쓰이는 곳 | 원장 | 상태를 올리려면 |
|---|---|---|---|---|---|---|
| **L-MB** (MustBefore 건전성) | 유한 실행 ρ ∈ Pref(B, σ_U, M)에서 노드 n의 발생 n_k와 a ∈ MB(n)이면 → ρ에서 n_k 앞에 a의 발생이 있다 | `unproved` | A-SKEL, A3, A-WS, A1, A-EXTRACT. **L1** | step 6의 사슬: m_ES ∈ MB(raise_ES) → T-OBS → m_ES ∈ MB(o_ES) → 시작 스레드를 따라 → spawn_SB → T-SPAWN → SB 진입 → x. EVS 반쪽은 m_EVS로 같은 사슬 | (iii) LEMMA. 쓰는 단계: (i) T-PO, T-SPAWN, T-OBS | T-OBS 사례가 쓰는 PWB 제외 사실이 같은 MB에서 도출된다. 이 공동 귀납("앞선 발생에서의 귀납 가정으로 정당화")이 주장만 되고 쓰이지 않았다. 그 귀납과 L1을 쓰면 `depends-on-stated-assumption`까지 올라갈 수 있다 |
| **L-ONCE** | once(a) ∧ a ∈ MB(b) → PWB(a, b): a의 모든 발생이 b의 모든 발생보다 앞 | `unproved` | L-MB. 한 번만 실행 분석(A3, A-SKEL) | ONCE-PWB 제외: 시작 스레드의 `cfe_es_start.c:82` memset, EVS 반쪽의 raise_ES (raise_ES ∈ MB(o_ES) ⊆ MB(f_EVS)) | (iii) LEMMA. (i) ONCE 단계 | L-MB가 올라가면 따라 올라간다 |
| **L-PWB** (frontier) | once(actor(a)), m ∈ actor(a), m ∉ Anc(a), m ∈ MB(b) → a와 b가 모두 나오는 모든 유한 실행에서 a의 모든 발생이 b의 모든 발생보다 앞 | `unproved` | L-MB, A-SKEL | FIXED_ORDER 항목 4의 frontier: 이후에 생성되는 행위자(EVS, SB, TBL, TIME, 스크립트 앱과 그 자식)의 writer를 PWB(o_ES, w)로 제외 | (iii) LEMMA. (i) FRONTIER 단계 | 위와 같음 |
| **L-SCOPE** | SHUTDOWN_REQ ∈ MB(d)이고 d의 발생이 사용 u_k의 결정 사건 앞 → u_k는 범위 밖 | `unproved` | L-MB | 쓰이면 `W_j`의 SCOPE 제외. R2에서는 쓰이지 않을 것으로 기대한다 [I] | (iii) LEMMA, 쓰이는 경우만 | 위와 같음 |
| **L-DERIVE** | `derives(arg, loc, c)` → API가 슬롯 LP에서 검사하는 값 = c 시점의 loc 값 | `depends-on-stated-assumption` | A-EXTRACT, A-TYPE, A2 | 값 conjunct V(AppID): SB 이벤트 사이트의 `stamp` 인자가 `CFE_SB_Global.AppId`에서 복사된 값 (예: `cfe_sb_api.c:226–228`의 로드 c) | (iii) LEMMA. (i) DERIVES 단계 | 복사 사슬의 gcc 바이너리 충실성은 가정이다. 이 보조정리 자체의 논증은 사슬이 추출된 대로 있으면 완전하다 |
| **L-FO** (FIXED_ORDER 건전성) | FIXED_ORDER 인증서(항목 1–6)가 있으면 → 범위 안 발생 u_k는 (ℓ_j가 결정 이전이면 P_j가 거짓이거나, 다른 conjunct가 먼저 거짓이거나, 호출이 ℓ_j를 지나지 않고 반환하여 `NOT_EVALUATED`). 따라서 `READY`가 아니고, 불량성 조건으로 어떤 확장도 `TOLERATED`로 만들지 않는다 → `¬OK_i`. 도달·모든 실행이 결정에 이른다는 것은 주장하지 않는다 | `unproved` | L-PWB, L-ONCE, L-SCOPE(조건부), L-DERIVE, L-STATUS (b); A-WS, A-EXTRACT, T4 항목 | 두 반쪽의 핵심 결론 R-1–R-3. 조기 반환(`cfe_es_task.c`의 `CFE_ES_TaskInit` 안 L340/371/381/393, step 7)은 인증서가 사용이 raise를 지배한다고 요구하지 않으므로 포함된다 | (iii) LEMMA | 의존하는 보조정리가 올라간 뒤 |
| **L1** (await 요약) | poll 루프의 전제(몸체 mod 집합에 카운터·예산 외 관련 위치 없음, 테스트 읽기가 한 임계 구역 또는 한 원자적 읽기)가 성립 → 조건에 의한 루프 종료는 test의 원자적 읽기 지점의 `await(Pred_code)` 성공 전이, 예산에 의한 종료는 timeout 전이 | `unproved` | A-EXTRACT (추출된 술어) | `CFE_ES_MainTaskSyncDelay`(`cfe_es_start.c:860–918`)의 :835 루프. 규칙 R19가 이 루프를 묶는다. o_ES, o_EVS의 T-OBS가 이 요약에 기댄다 | (iii) LEMMA(의무). 체커는 L1의 전제(mod 집합, 잠금 구역)를 (ii)의 추출 사실에서 검사한다 (own) | SPEC이 "F1 전에 완전하게 쓴다"고 한 의무. 요약이 교착 판정을 바꾸는 부분(σ_U∞)은 R2 첫 결과의 셀에는 쓰이지 않는다 |
| **L-STATUS** | 범위 안 발생의 상태·범위는 ρ의 효과 사건만으로 정해진다. `READY`, `TOLERATED`, `VIOLATING`, `NOT_READY_UNJUDGED`, `NOT_EVALUATED`는 모든 확장에서 최종이다. 불량 실행의 확장은 불량이다 | `unproved` | §1.6의 정의 | L-FO의 "어떤 확장도 `TOLERATED`로 만들지 않는다". 실행에서 읽은 발생의 상태 (도달) | (iii) LEMMA | 정의에 가깝다. 여섯 가지 지평 감시자가 하나씩 최종적이고 ρ의 사건만 읽는지 확인하는 단계를 쓰면 `proved`가 된다 |

### 2.2 도달 항목이 쓰는 보조정리

| 보조정리 | 전제 → 결론 | 상태 | 의존 | R2에서 쓰이는 곳 | 원장 | 상태를 올리려면 |
|---|---|---|---|---|---|---|
| **L-CONT (c)** | B 아래 실제 유한 실행 → 그 LP 단위 사영이 Pref(B, σ_U, M_impl)에 속한다. 실행의 끝 상태에 대해서는 아무 말도 하지 않는다 | `depends-on-stated-assumption` | A7-impl, A7-api, A-SKEL, A3, A4, A9 | (σ_U, M_impl) 셀의 도달 `proved`: S1이 관측한 실행 하나가 사용 발생을 포함한다 (FIXED_ORDER 항목 6, step 9). 이전 원장에서 빠져 있던 항목이다 (REVIEW_RESPONSE §5 항목 8) | (iii) LEMMA. 반증: (iv) O1, O2, O4, O5 | 단계 모의가 "실행 길이에 대한 귀납"이라고만 적혀 있다. 귀납을 쓰면 상태는 유지(가정에 의존)된다. 틈이 나오면 `unproved`로 내려간다 |
| **L-MAPI-CONE** | 닫힌 cone C와 실제 실행의 사영 ρ에서 C 행위자의 모든 OSAL 호출이 M_api가 허용하는 결과이고 M_impl 전용 동작(`OS_ERR_INCORRECT_OBJ_STATE`, 잃어버린 task, 남은 type lock)이 없다 → Pref(B, σ_U, M_api)에 C의 행위자에 대해 같은 사영을 갖는 ρ′가 있다 | `unproved` (의무 [I]) | cone 닫힘 규칙(§4.10 항목 1), A7-api, L-CONT (c) | (σ_U, M_api) 셀의 도달 `proved`의 조건. 이 실행도 같은 사영에서 출발하므로 L-CONT (c)가 함께 적힌다 | (iii) LEMMA. 반증: (iv) O1, O2, O4, O5 | 증명을 쓴다. 쓰기 전에는 이 셀의 도달은 `assumed`로 둔다 |
| **L-MLEVEL** | cone과 인증서에 M_impl 관련 연산이 없으면 두 모델 수준의 사영이 같다 | `unproved` (의무 [I]) | cone 닫힘 | **R2에서는 쓰지 않는다**: cone이 task 생성을 포함하므로 M_impl 관련이다 | — | — |

### 2.3 파생 셀에서만 쓰이는 보조정리 (V0-P가 전이 보조정리를 쓰기 전에는 R2의 첫 결과에 포함하지 않는다)

| 보조정리 | 상태 | 쓰이는 셀 | 비고 |
|---|---|---|---|
| L-CONT (a) (σ_U∞ = σ_U에서 시간 기반 전이를 뺀 것) | `proved` (§1 정의만 사용, 길이에 대한 귀납) | σ_U∞ | 도달은 셀마다 L-REACH-CONE이 따로 필요 |
| L-CONT (b)·(b′) (σ_R1) | `unproved` | σ_R1 | 한정 b의 전이는 (b′) L-CONE-R1 |
| L-MON | `unproved` | 모든 파생 셀 | 모니터 상태가 모든 지평 종류에 충분한지는 주장만 됨 |
| L-TRANS (1a), (2) | `unproved` | σ_U∞, σ_R1, σ_E | σ_E는 M_impl에서 L-CONT (c)와 함께. 판정된 발생에 대한 진술 (SPEC §1.8 규칙 2) |
| L-REACH-CONE | `unproved` (의무 [I]) | σ_U∞ 셀의 실행 기반 도달 | |

### 2.4 쓰이지 않는 것

L-DPOR, L-DEAD, L-REF, L-PRED, L-LOOP, L2, L3: 인증서 경로가 진행 결과를 `unknown(not_checked)`로 두므로 R2 첫 결과의 셀에는 쓰이지 않는다. 탐색기(`exh`)를 돌리는 단계가 오면 그 셀의 원장이 이것들을 추가한다 (SPEC §4.5.1 '인증서 없는 판정').

## 3. 가정 (SPEC §6.1) — 증명 대상이 아니다

가정은 증명 상태를 갖지 않는다. 가정이라는 것이 그 성격이다. 위반하면 어떻게 되는지는 SPEC §6.1이 적었다. 원장 위치는 모두 (iii)이며 반증 가능한 것은 (iv)에 의무가 붙는다. R2 첫 결과의 SPEC App. A.1 step 9a (iii)에 있는 목록이다.

| 가정 | 내용 | R2에서의 역할 | 반증 의무 (iv) |
|---|---|---|---|
| A1 | Φ의 규칙·효과·바인딩이 고정 리비전에 대해 옳다 | K-ID 준비 상태 명세, R19, `CFE_ES_GetAppID` 효과, `AppState`/`IsUsed` 바인딩 | O3, O4, O7 |
| A2 | Clang 사이트·AST 패턴이 gcc 바이너리의 명령에 대응한다 | 탐침 위치(도달), L-DERIVE | 앵커 감사, DWARF 컴파일 단위 바인딩 |
| A3 | 닫힘: 행위자는 선택된 시작 스크립트·생성 객체표·`CreateChildTask` 사이트 등에서만 나온다 | T-SPAWN 목록, once, FRONTIER 제외, L-CONT (c) | O5 |
| A4 | x86-64 TSO, `-O0`에서 사건 단위 순차 일관성 | L-CONT (c)의 사건 단위 모의 | 표시만 함 (D10). 관측으로 반증하지 못한다 |
| A6 | 계획된 사이트의 관측 완전성(손실 계수 포함) | 도달 `proved` | `tseq` 틈, `unknown(loss)` |
| A7-impl | Φ_L-1 의미가 M_impl 경로의 커널·libc·OSAL POSIX 동작을 LP 단위에서 과대 근사한다 | M_impl 인증서, L-CONT (c) | 실제 OSAL 위의 litmus, O4 |
| A7-api | M_impl이 다루지 않는 OSAL 기본 연산이 실제 백엔드에서 LP에서 선형화 가능하다 | L-CONT (c), (σ_U, M_api) 도달 | litmus, O4 |
| A9 | B.Input이 실행의 외부 사건 전부이다 (R2 기본 부팅은 빈 입력) | 메시지 출처, L-CONT (c) | O5 |
| A10 | M_impl은 OSAL idmap·task·binsem·queue 경로만 다룬다 | M_impl 인증서 | 목록 |
| A-SKEL | `Sk^cert`가 행위자 인스턴스의 사영 행동을 과대 근사한다 | L-MB, L-PWB, L-CONT (c) | O1, O2, (S5부터) O8 |
| A-WS | writer 집합 완전성(A-TYPE 아래) | `CFE_SB_Global.AppId`와 `AppState`/`IsUsed`의 writer 집합 | O7 |
| A-TYPE | 지지 위치에 대한 모든 접근이 선언된 타입을 거친다 | `&CFE_SB_Global.AppId`는 `cfe_sb_task.c:138`에서만 취해진다는 목록 | 타입 없는 쓰기·주소 유출 목록(각 적중이 위치를 비활성화) |
| A-EXTRACT | `facts.kz`의 사실이 컴파일된 소스에 충실하다 | 모든 (ii) 항목 | O4, O7, 앵커 감사, litmus, 프로그램 변이 |
| A-CORE | 공유 `libkz-core`(Φ 파서, 평가기, 규칙 엔진, 컨테이너 I/O)가 옳다 | 'via T5' 단계 | 평가기 일치 속성 시험, golden 시험 (의무가 아니라 원장 밖 시험) |
| A-FRESH | 서로 다른 예약 노드는 서로 다르고 이전에 쓰이지 않은 레코드를 할당한다 | EVS 반쪽의 `ρ_EVS ≠ ρ_ES` | O9 (`DECLARED_TRUSTED`) |
| A-OWNER | `StartAppTask(…, ParentAppId)`로 만든 task의 App 레코드는 `PendingAppId`가 흘러간 예약의 레코드이다 | EVS 반쪽의 `ExitApp` STOPPED 제외 | O9 (`DECLARED_TRUSTED`) |
| A-ZT | σ_R1에서 블록 사이 계산은 시간 0이다 | σ_R1 파생 셀만 | 표시만 함 |
| T-JACM | `dpor`의 정확성 정리 | **R2 첫 결과에서는 쓰지 않는다** | — |

## 4. 추출 사실 (원장 (ii)) — 체커가 읽었지만 신뢰한다

체커가 다시 도출하거나 확인하는 것은 '처리' 열에 적었다. 나머지는 신뢰한다. 결정문 §2.1이 구분하라고 한 것이다.

| 항목 | 내용 | SPEC 위치와 근거 | 체커의 처리 | 반증 의무 |
|---|---|---|---|---|
| F-1 | `CFE_EVS_SendEventWithAppID`를 `stamp = CFE_SB_Global.AppId`로 부르는 SB 사이트 39개와 그 문맥(`CFE_ES_TaskInit`/`CFE_EVS_TaskInit`에서 인라인된 문맥은 서로 다른 템플릿) | A.1 step 1, 9a (ii). 39개는 `results/R2_gdb_v701.md` §6 (ENV 소스 재집계) [F] | 신뢰 | O4 |
| F-2 | 복사 사슬 AST 사실: 호출 인자 → `(CFE_SB_Global_t, AppId)` (`cfe_sb_api.c:226–228` 등) | step 2 | 직렬화된 사실 안의 사슬을 확인 (own, DERIVES). 추출 자체는 신뢰 | O4 |
| F-3 | INIT 이미지: `CFE_SB_Global`은 0으로 초기화된 전역 변수 (`cfe_sb_task.c:40`), 따라서 `AppId = 0 = CFE_ES_APPID_UNDEFINED` | step 3 | 이 이미지 위에서 `INIT_FALSE(V)`를 평가 (via T5) | O4 |
| F-4 | `(CFE_SB_Global_t, AppId)`와 `AppState`/`IsUsed`의 S3a writer 구간과 전이 분류·키 상태. 쓰기 `x`: `cfe_sb_task.c:138`, W·COMPUTED·MUST_KEY. 중립: 실패 가지 저장 `cfe_es_api.c:758–760`, EarlyInit memset `cfe_sb_init.c:50`, scan WAITING `cfe_es_apps.c:1144` | step 3–5, 9a (ii) | `W_o`를 이 구간에서 다시 계산하고 각 제외를 다시 확인 (own). 전이 분류와 완전성은 신뢰 (A-WS) | O7 (모델 빌드) |
| F-5 | A-TYPE 목록: 타입 없는 쓰기 없음, `&CFE_SB_Global.AppId`는 `:138`에서만 | step 3 | 신뢰 | 목록 자체가 검사 |
| F-6 | App 레코드 태그 `Type = CORE` (`cfe_es_start.c:795`)와 `PendingAppId` 흐름 (`:814`) | step 5, 9a (ii) | 신뢰. A-OWNER가 기댄다 | O9 |
| F-7 | `:835` 종료 트리를 직렬화한 것(루프의 mod 집합, 잠금 구역 포함) | step 4, 10 | `Pred_code`를 이 트리에서 체커 자신의 기호 평가기로 다시 도출하고 L1의 전제를 검사하며 `Pred_code ⇒ C`를 자기 절차로 결정 (own). C의 구문 트리만 공유 Φ 파서에서 옴 | O4 |
| F-8 | 생성된 core 표의 순서 (`cfe_static_module_list.c`) | step 6, 9a (ii) | 신뢰 | O5 |
| F-9 | 호출 그래프에 근거한 행: `RunLoop` raise가 동시 행위자에 없음, `cFS-Master`와 콘솔에는 writer가 없음 | step 4 | 신뢰 | O7 |
| F-10 | `Sk^cert` 그래프 (펼친 core 루프, 별 닫힘 메시지 루프, 인라인된 TaskInit 문맥) | step 6–7 | 체커의 지배·도달·한 번만 루틴이 이 그래프 위에서 돌지만 그래프는 다시 만들지 않는다 | O1, O2 |
| F-11 | 닫힘의 spawn 사이트 목록: SB main과 FRONTIER 제외에 이름이 나온 모든 행위자 | step 6, 9a (ii) | T-SPAWN 단계가 인증서의 목록과 닫힘의 목록이 같음을 확인 (own). 닫힘은 신뢰 | O5 |
| F-12 | 도달을 위한 실행·접두 탐침 기록 | step 9 | `reach` 단계가 실행이 있고 사용 발생을 포함하며 셀의 조건을 만족함을 확인 (own). 기록은 신뢰 (A2, A6) | O1, O2, O4, O5 |

## 5. 신뢰 항목 중 보조정리·가정이 아닌 것 (원장 (iii))

| 항목 | 내용 | 비고 |
|---|---|---|
| Φ: `K-ID.send-with-appid` | 준비 상태 전제 `valid(arg.AppID) ∧ state(evs.reg(arg.AppID)) = REG`, 캡처 지점 LP와 슬롯 검사 LP, 결과 대응 `cfe_evs.c:185–189`, `Allowed none`, 위해 `event_request_rejected`, `appreq` = `cfe_evs.h:169–172`(호출자 쪽) | `Allowed none`은 `appreq`가 지정되어 있어서 성립한다 (SPEC §1.6). 입력이지 증명 대상이 아님 |
| Φ: T-DERIVE, `CFE_ES_GetAppID` 효과, `AppState`/`IsUsed` 바인딩 | | 위와 같음 |
| Φ: R19 | `:835` 루프의 바인딩, 층 S, `obs_fatal`, terminal escape. 술어 자체는 신뢰하지 않고 다시 도출한다 | 규칙 해석은 via T5 [I] |
| Φ: T-TERM 선언 | pc-linux에서 `CFE_PSP_Panic`과 `abort`가 종단이다 | 선언의 참은 확인하지 않는다 (T4) |
| Φ_L-1 | M_impl 인증서의 연산 의미 | A7-impl이 뒷받침해야 함 |
| `DECLARED_TRUSTED` (두 반쪽) | `CleanUpApp` 레코드 해제 제외의 관계 보호 조건 | 반증: O3/O4. 단 `CleanUpApp`이 창 안에서 도는 실행에서만. 기본 부팅에는 없을 것으로 기대 [I] |
| `DECLARED_TRUSTED` (EVS 반쪽) | A-FRESH, A-OWNER와 이에 기댄 `ExitApp` STOPPED 제외 | 반증: O9 |
| 도구 기반 | `libkz-core`(Φ 파서, 'via T5' 단계의 평가기, 컨테이너 I/O)와 체커의 도구 해시 | A-CORE |

## 6. 체커가 다시 확인하는 추론 단계 (원장 (i))

모두 기대이다 [I]. 체커는 구현되지 않았다. 각 단계는 (ii)·(iii)에 상대적으로 옳음을 확인할 뿐이다 ("re-validated"는 그 뜻).

| 단계 | 확인하는 것 | 구분 |
|---|---|---|
| view | 모든 노드가 `Sk^cert` 노드이고 `Sk^h`·`TRUNC` 노드가 없다 | own |
| ONCE | once(PSP main)(정의), once(ES main)·once(EVS main)·once(SB main)(각각 펼친 core 루프의 한 spawn 노드), once(raise_ES), once(`cfe_es_start.c:82`) | own |
| T-PO | m_ES ∈ MB(raise_ES), m_EVS ∈ MB(raise_EVS), 유지된 `ExitApp` 저장이 m 뒤에 오는 것, 시작 스레드의 o_ES(o_EVS)→spawn_SB 사슬(패닉 가지 절단), SB 진입→x, FRONTIER 제외마다 시작 스레드의 경로 | own |
| T-TERM | `CFE_PSP_Panic`, `abort`가 pc-linux에서 종단이라는 선언이 인용과 함께 있음 | own (선언의 참은 확인하지 않음) |
| T-SPAWN | SB main의 spawn 사이트 목록 = 닫힘의 목록(SB 반복의 `:810` 한 곳). FRONTIER 제외에 쓰인 행위자마다 같음 | own |
| T-OBS (o_ES, o_EVS) | 반증자 조건(`:803`, 추상 사후 상태에서 평가). `Pred_code` 다시 도출. R19의 바인딩·층·종단 탈출. L1 전제. `Pred_code ⇒ C`. `W_o` 재계산과 제외 | 반증자 조건과 R19: via T5. 나머지: own |
| FRONTIER | 모든 캡처 지점 c에 대해 `Sk^cert` 경로 m →* c가 없다 | own |
| DERIVES | 호출 인자에서 `(CFE_SB_Global_t, AppId)`까지 복사 사슬 | own |
| INIT_FALSE(V) | INIT 이미지 위에서 V가 거짓 | via T5 |
| 불량성 | `Allowed = none`이므로 준비 안 됨 발생은 결정 시점에 `VIOLATING` | own |
| 상태 | 전이·키 상태가 writer 구간과 일치, `DECLARED_TRUSTED` 목록이 완전 (다시 도출하지 않고 일관성만) | own |
| 도달 | S1 이후: 실행이 있고 사용 발생을 포함. (σ_U, M_api)는 닫힌 cone 모든 행위자의 M_api 적합성 검사 통과 | own |

## 7. 반증 가능하지만 증명되지 않은 항목 (원장 (iv))

모든 상태가 `not run`이다 (S1·S2 전). 통과해도 "실행 집합 R의 확인된 경로에서 반증되지 않음"으로 적고 증명으로 적지 않는다.

| 항목 | 반증 의무 | 노출 경로의 한계 [I] |
|---|---|---|
| A-FRESH, A-OWNER | O9 | |
| `CleanUpApp` 보호 조건 | O3, O4 | `CleanUpApp`이 창 안에서 도는 실행에서만 검사됨. 기본 부팅에는 그런 실행이 없을 것으로 기대 |
| A-SKEL | O1, O2 (모든 trace), O8 (S5부터) | |
| 닫힘 (A3, A9) | O5 | |
| once·MB 사실 | O3 | |
| Φ 효과, LP, `derives`, `Pred_code` | O4(a)/(b). **SB AppId 설정 전에 성공하는 SB 사용이 하나라도 있으면 인증서가 반증된다** | |
| writer 완전성 (A-WS) | O7 (모델 빌드) | |
| **L-CONT (c)**, L-MAPI-CONE의 실행 쪽 전제 | O1, O2 (행위자 사건열이 자동자에 받아들여지는가), O4 (결과 코드와 계산된 상태), O5 (닫힘 밖의 행위자·키) | 이번에 새로 추가된 항목 (C-2) |
| 노출 경로의 한계 | 기본 부팅은 성공 경로만 행사한다. 다음은 기본 부팅의 의무로 반증할 수 없다: `:823` → 패닉 가지, `cfe_es_task.c`의 조기 반환(L340/371/381/393), 시간 초과된 대기 뒤 `ExitApp` 경로 | |

## 8. 관측된 사용별 집합 (R-5)

| 항목 | 값 | 근거와 상태 |
|---|---|---|
| gdb 실행 | 3회 (`r2_g1`, `r2_g2`, `r2_g3`), 모두 같은 24회 | [M] `results/R2_gdb_v701.md` §2. 같은 hit 위치. 3회가 같다는 사실이 고정 순서를 증명하지는 않는다 (같은 문서 §3, §6). ASLR을 끈 gdb 아래의 관측이며 교란이 있다 |
| ES 20 | `ReportSingleEvent` 17 (이벤트 14 `SEND_NO_SUBS`), `SubscribeFull` 2 (이벤트 10), `CreatePipe` 1 (이벤트 5) | [M] §2 표 |
| EVS 4 | `SubscribeFull` 2, `CreatePipe` 1, `ReportSingleEvent` 1 | [M] §2 표 |
| 17개의 `SEND_NO_SUBS`의 출처 | ES의 이벤트 패킷(MsgId 0x808)이 구독자 없이 전송됨 | [I] 이벤트 ID와 콘솔의 'No subscribers for MsgId 0x808'에서 추론 (§2). MsgId 귀속은 EVS 문맥의 1개에 대해 열려 있음 [U] |
| 단위 | 원인 1개, 정적 호출 사이트 39개 중 실행된 5개와 위반 3개, 문맥 6개(task × SB 함수), 호출 문자열 12개, 부팅당 발생 24회, 실행 3회 | [M] `results/R2_gdb_v701.md` §6, §7 |
| Kairos 자체 기록기의 재관측 | **S1-R2 게이트는 미실행.** 게이트: 사용별 집합(task, SB 함수, EID)이 [R2g §2]와, B에서 계산한 기대와 일치해야 하며 불일치는 조사하고 규칙을 맞추어 고치지 않는다. 2026-10-24 pilot/dev 실행 1회(커널 v153, 사이트 26개): 24 = ES 20 + EVS 4, 함수별 2/4/18, EID별 2/4/18 로 같은 값. 요약 `OBSERVED_VIOLATION` 24, 실행 task 소유 앱 `confirmed`(`kairos/docs/r2_pilot_v153.md`) | [M] pilot 1회. 게이트의 고정 실행 묶음·반복 수는 PI 결정(D14) |
| 이 관측과 증명의 관계 | 관측은 인증서를 반증할 수 있으나 증명하지 못한다. 관측이 하는 일은 (σ_U, M_impl) 셀의 도달 `proved`뿐이며 그것도 L-CONT (c)를 거친다 | |
| 이번 표가 쓰지 않는 것 | 버전 테이블 유무에 따라 부팅당 횟수가 달라진다는 기록은 폐기된 도구에서만 측정한 것이어서 쓰지 않았다 | SPEC에는 그런 문장이 남아 있다. CORD 정리 단계가 처리한다 |

## 9. 결과 문장별 합산 상태 (가장 약한 고리)

| 결과 | 사용하는 보조정리 항목 | 합산 증명 상태 | 렌더링 예 (기대, [I]) |
|---|---|---|---|
| R-1 ES 반쪽 (σ_U, M_impl) 진술 | L-MB, L-ONCE, L-PWB, L-DERIVE, L-FO, L1, L-STATUS (+ 도달이 실행으로 증명되면 L-CONT (c)) | **`unproved`** | `fixed-order — ¬OK_i, deciding (certificate, b = ∞); reach proved; progress not checked; lemma status: unproved (L-MB, L-ONCE, L-PWB, L-FO, L1, L-STATUS); lemmas:0/2/6` |
| R-2 ES 반쪽 (σ_U, M_api) | 위 + L-MAPI-CONE (도달이 실행으로 증명되면) | **`unproved`** | `… lemma status: unproved (…, L-MAPI-CONE); lemmas:0/2/7`. 도달이 `assumed`이면 `lemmas:0/1/6` |
| R-3 EVS 반쪽 | R-1·R-2와 같은 보조정리. 추가로 가정 A-FRESH, A-OWNER (보조정리가 아님) | **`unproved`** | 같음 |
| R-4 도달 | L-CONT (c)와 L-STATUS (실행에서 읽은 발생의 상태), 가정 A2, A6 (M_impl). M_api는 L-MAPI-CONE도 거침 | **`unproved`** (L-STATUS, M_api는 L-MAPI-CONE도). L-STATUS가 `proved`가 되면 M_impl은 `depends-on-stated-assumption`(L-CONT (c)) | `reach proved (run …); lemma status: unproved (L-STATUS); L-CONT (c): depends-on-stated-assumption` |
| R-5 사용별 집합 | 보조정리 없음. 관측 | 해당 없음 (관측 증거) | `observed 24 per boot in 3 gdb runs [M]; Kairos re-observation: not run` |
| R-6 진행성 | 없음 | 해당 없음 | `progress: unknown(not_checked)` |

(`lemmas:<p>/<d>/<u>`는 원장 (iii)의 `LEMMA` 항목 중 `proved`, `depends-on-stated-assumption`, `unproved`의 개수이다. 이 개수는 SPEC 문안에서 센 기대값이다. 위 표 §2.1·§2.2에서 도달이 실행으로 증명되는 (σ_U, M_impl) 셀은 항목 8개 = 가정에 의존 2(L-DERIVE, L-CONT (c)) + 미증명 6이고, (σ_U, M_api) 셀은 9개 = 가정에 의존 2 + 미증명 7이다. 도달이 `assumed`이면 두 셀 모두 7개 = 가정에 의존 1 + 미증명 6이다.)

**따라서 R2 첫 결과는 '체커가 신뢰 기반에 상대적으로 받아들였고 보조정리의 상태가 미증명인 FIXED_ORDER 진술'이다.** '증명된 고정 순서', '인증된 R2'로 쓰지 않는다 (SPEC §6.4 항목 45).

## 10. 상태를 올리기 위한 최소 작업

수치(주 수, 인월)는 쓰지 않는다. 근거가 없는 값은 정하지 않는다.

| 순서 | 작업 | 효과 |
|---|---|---|
| 1 | L-STATUS: 여섯 가지 지평 감시자를 하나씩 확인하여 최종성과 사건 의존을 쓴다 | L-STATUS `unproved` → `proved` (§1 정의만 사용) |
| 2 | L1을 완전하게 쓴다 (전제, 요약된 poller가 요약 안 된 poller가 갇히는 상태에서 정확히 죽는다는 것 포함) | L1 `unproved` → `depends-on-stated-assumption` (A-EXTRACT) |
| 3 | L-MB의 공동 귀납(MB와 PWB 사실)을 쓴다 | L-MB가 `depends-on-stated-assumption`이 되고, L-ONCE·L-PWB·L-SCOPE·L-FO도 순서대로 따라 올라간다 |
| 4 | L-CONT (c)의 단계 모의를 쓴다 | 상태 유지(가정에 의존). 틈이 나오면 내려가지 않게 하는 작업 |
| 5 | L-MAPI-CONE의 증명을 쓴다 | (σ_U, M_api) 셀의 도달 사슬이 완성 |
| 6 | 독립 검토 기록 (V0-P) | 모든 상태가 '작성자의 분류'에서 '검토된 분류'가 된다 |

**도달 가능한 최선의 상태.** 위 작업을 모두 해도 R2 인증서는 `proved`가 될 수 없다. 인증서는 A-WS, A-EXTRACT, A-SKEL, A3 같은 가정에 의존하기 때문이다. 최선은 `depends-on-stated-assumption`이다. 그때에도 `proved`를 출력하는 것은 보조정리뿐이고 판정이 아니다 (PS4).

## 11. 한계와 미확인

- 이 표의 상태는 작성자의 분류이고 독립 검토가 없다 [I]. 특히 L-STATUS를 `unproved`로 둔 것은 보수적인 판단이다. 정의에 가까워 쉽게 올라갈 수 있다.
- 증명 상태 필드는 SPEC의 레코드 정의에만 있고 구현되지 않았다. 체커·기록기·탐색기는 없다.
- SPEC A.1의 기대(경로 줄 번호, 표의 항목)는 App. A.1에 쓰인 대로 옮겼다. 이번 단계에서 모든 줄 번호를 다시 읽은 것은 아니다. 이번에 다시 읽은 것은 `cfe_evs.c`의 거부 분기(L185–L188, L204)이다 [F]. 나머지는 SPEC의 인용을 그대로 쓴다 [U: 이번 단계 재확인 안 함].
- R2에 병합된 수정이 없어 대조 구성(P-R2CTRL = (c2)를 1차로 채택)은 아직 없다. 이 표는 수정 전 구성 하나의 인증서만 다룬다. 순서 근거가 바뀐 구성에서 판정이 철회되는지는 V2의 일이며 이 표의 범위가 아니다. 철회되었다는 사실만으로 전체 안전성을 인정하지 않는다 (결정문 §3.1).
- SPEC에 남은 CORD 인용과 '버전 테이블이 없을 때의 부팅당 횟수'는 이 표에서 쓰지 않았다 (§8).

## 12. 관측점 추적표 (X-03, C-24; W2 2026-10-24)

`kairos/profiles/cfs-v7.0.1/observe/sites.kzp` 의 사이트가 어느 소스 줄에서 무엇을 관측하고, Φ 의 어느 효과·조건을 뒷받침하는지 적는다. 줄 번호는 R2 바이너리(`build-native_std/exe/cpu1/core-cpu1`, SHA-256 `0edb1ae2…7799`)를 빌드한 고정 트리 `/home/user/work/procrace/cfs_ref/cFS`(cFS `088b2fa8`, cFE `c5fb2b4d`, OSAL `d2d877a6`)에서 다시 읽었다. 인용 33개의 해시가 `kz anchor --source-root` 에서 모두 일치했고, 같은 줄을 인용하는 `01_prov.kzp` 의 해시와도 같다. 그래서 프로필의 줄은 옮겨지지 않았다 [F]. **정정**: 작업 지시의 "`cfe_es_api.c:803`" 은 `cfe_es_start.c:803`(핵심 앱 예약, `es_start_reserve`)이다. SPEC §2.3 표와 App. A.1 이 인용하는 줄과 같다.

| 사이트 (번호) | 소스 줄 | 프로브 위치 [M: `kz anchor`, `--xcheck`] | 관측하는 효과·사건 | 뒷받침하는 것 |
|---|---|---|---|---|
| `evs.send.entry/ret` (0, 1) | `cfe_evs.c:169-214` | 함수 진입(+4), 복귀 | K-ID 사용의 호출과 반환(인자 `AppID` = `stamp`, 결과 코드) | `id_valid: valid(arg.AppID)` (capture: 인자), 결과 대응 O4 |
| `CFE_EVS_SendEventWithAppID.slot_registered` (12) | `cfe_evs.c:190` (`EVS_AppDataIsMatch` 호출) | 호출 직후, discriminator 행 시작 `0x29d02` | 등록 검사 LP 의 통과(반환값 `is_match` 는 기록만) | `slot_registered: state(evs.reg(arg.AppID)) = REG` 의 LP 통과 여부 |
| `evs.register.entry/ret` (8, 9) | `cfe_evs.c:42-111` | 함수 진입, 복귀 | `CFE_EVS_Register` 호출 틀 | 아래 두 효과의 호출 문맥 |
| `CFE_EVS_Register.clear_slot` (10) | `cfe_evs.c:56` (`memset`) | `memset@plt` 호출 직후 `0x298c4` | `clear_slot: evs.reg(id(owner(exec))) -> UNREG` | `slot_registered` 의 writer |
| `CFE_EVS_Register.register` (11) | `cfe_evs.c:106` (`EVS_AppDataSetUsed`) | **줄 시작** `0x29a38` (호출 직후 지점은 반환 경로들의 합류점이라 `kz anchor` 가 거부) | `register: evs.reg(id(owner(exec))) -> REG` | `slot_registered` 의 writer |
| `es.wait.entry/ret` (13, 14) | `cfe_es_api.c:514-576` | 함수 진입, 복귀 | `CFE_ES_WaitForSystemState` 호출 | 올림의 호출 문맥 |
| `CFE_ES_WaitForSystemState.raise_state` (15) | `cfe_es_api.c:573` (저장, 줄에 호출 없음) | **줄 시작** `0x1ae79` (저장 가지를 탈 때만) | `raise_state: es.appstate(owner(exec)) raise lp.required`. 올리는 값은 읽지 않는다(저장 명령의 레지스터, ABI 위치 아님) | 인증서 경로 T-OBS 의 raise_ES/raise_EVS 와 o_ES/o_EVS 술어(AppState ≥ RUNNING). K-ID 조건은 아님 |
| `CFE_ES_Main.reset_records`, `reset_states` (16, 17) | `cfe_es_start.c:82` (`memset(&CFE_ES_Global…)`) | `memset@plt` 호출 직후 `0x24bbf` (한 주소에 두 사이트) | `reset_records`(모든 `es.app` → FREE), `reset_states`(모든 `es.appstate` → UNDEFINED) | 인증서의 EarlyInit memset(ONCE-PWB); `es.app` writer |
| `CFE_ES_CreateObjects.reserve_core` (18) | `cfe_es_start.c:803` | `CFE_ES_AppRecordSetUsed` 호출 직후 `0x25928` | `reserve_core: es.app(lp.slot) -> RESERVED`. `lp.slot` 은 읽지 않는다 | 인증서의 반증자 f_ES/f_EVS; `es.app` writer |
| `CFE_ES_CreateObjects.publish_core` (19) | `cfe_es_start.c:818` | 호출 직후 `0x259b2` | `publish_core: es.app(lp.slot) -> LIVE id lp.pending_id` (값 읽지 않음) | `es.app` writer → `id(owner(exec))` |
| `core_clear` (20) | `cfe_es_start.c:823` (`memset`) | **줄 시작** `0x259c3` (호출 직후 지점은 성공 가지와의 합류점) | `clear_core`, `clear_core_state`, `clear_core_type` (`lp.slot` 읽지 않음) | 인증서의 OWN-NOPATH 대상; `es.app` writer |
| `sb.appid.store` (2) | `cfe_sb_task.c:138` | 호출 직후(142 줄) | SB 의 `CFE_ES_GetAppID` 반환(저장 시각의 상한). 라벨이 Φ 지점 이름이 아니어서 재생에는 대응되지 않는다 | L-DERIVE 의 셀 writer `x` (개수 비교용) |
| `es.start_task.entry/ret` (21, 22) | `cfe_es_apps.c:649-670`, 넘기는 쪽 `cfe_es_start.c:810-814`, `cfe_es_apps.c:849-854` | 함수 진입(`r8` = `ParentAppId`), 복귀 | 생성 요청과 그것이 지정한 앱 | `app_owner` (생성 요청), K-ID 의 `owner(exec)` 키 |
| `es.child_task.entry/ret` (23, 24) | `cfe_es_api.c:1202-1208`, `:1255-1290` | 함수 진입, 복귀 | 자식 task 요청(앱 = 요청한 task 의 앱). 안에 겹친 `ParentAppId` 는 App 표 조회값이라 쓰지 않음 | `app_owner` (`creator-owner`, EV-03) |
| `osal.task_create.id` (25) | `os-impl-tasks.c:457-462`, `:596-625`; 같은 값이 출력 인자에 쓰임 `osapi-idmap.c:838-873` | `OS_Posix_InternalTaskCreate_Impl` 진입(`r9` 하위 4바이트) | 생성 호출이 배정한 OSAL id | 자식의 `OS_TaskEntryPoint(task_id)`(사이트 7)와 대조 → `confirmed` |

**포괄의 빈 곳** [F/I]. 이 표로 상태를 읽는 조건이 바로 풀리지는 않는다.
- **`evs.reg` writer.** 정적 추출은 core-cpu1 안 `EVS_AppData_t.AppID` 의 writer 를 4개로 센다(`kairos/tests/static/data/core_cpu1.golden:162`: direct 2, data function 2) [F]. 소스로 보면 `cfe_evs.c:56` memset, `:106` SetUsed, `cfe_evs_task.c:63` EarlyInit memset, `cfe_evs_task.c:194` CleanUpApp 의 SetFree 이다 [I: grep 대응]. Φ 는 앞의 두 개만 효과로 모델링한다. 그래서 `writers=covered` 를 세울 근거가 없다.
- **`es.app` writer.** AppCreate(`cfe_es_apps.c:821/869/893`)와 CleanUpApp(`:1498/:1632`)에는 사이트가 없다.
- **효과 값.** `lp.slot`, `lp.pending_id`, `lp.required` 를 S1 백엔드가 읽지 못한다. 그래서 효과의 대상이 정해지지 않는다.
- **행위자 사이 순서.** 다른 thread 가 쓴 등록을 읽는 사용에는 행위자 사이 순서가 필요하다. 재생은 병합 순서를 실행 순서로 쓰지 않는다(W1). 그래서 등록 조건의 값은 보류된다(`effect_unsupported`, `resource_or_lifetime_unmatched`). SPEC App. A.1 의 "registry LP 와 register 효과를 계획에 넣으면 READY" 는 writer 포괄과 순서가 세워진 뒤의 기대로 읽는다.
- **실행 결과.** 이 표로 처음 돈 실행은 pilot/dev 기록이다(`kairos/docs/r2_pilot_v153.md`).

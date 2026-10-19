# 연구 방향 메타 분석과 race 탐지 알고리즘 심층 검토 (2026-10-19)

- 연구: Process-Level Race Detection in core Flight System (도구 이름 Kairos, 접두어 `kz`)
- 기준 커밋: `b8365fb` (브랜치 `claude/busy-shannon-y903qc`). 이 문서를 쓰는 동안 저장소의 코드·명세·계획·기록은 **하나도 바꾸지 않았다.** 워크플로 재개, 구현, CORD 삭제, 10-19 검토가 제안한 문서 정정도 하지 않았다(지시: "아직 후속 작업은 수행하지 말기").
- 입력: 10-19 검토 문서(`Kairos 중간검토 분석결과 및 보완방안`), `MIDREVIEW_20261016.md`, `Architecture/spec/SPEC_FINAL.md` rev 1.2(§1.6–§1.11, §4.2–§4.15, §6, App. A.1), `plan/proof_status_R2.md`, `plan/formal_corrections.md`, `kairos/docs/{phi_reference,static_facts,observer_smoke_run}.md`, `kairos/profiles/cfs-v7.0.1/{30_hb,40_rspec,10_kinds}.kzp`, `observe/sites.kzp`, `kairos/lib/core/eval_trace.cpp`, `kairos/lib/llvm/{unroll,finalize}.cpp`, `kairos/tools/kz/cmd_hits.cpp`.
- 표기: **[F]** 출처가 있는 사실(SPEC·코드·기록의 인용), **[M]** 기록된 측정, **[I]** 추론, **[D]** 결정, **[U]** 미확인, **[검토]** 이 문서의 판단. 숫자·조건은 모두 기존 기록에서 가져왔고 새로 정한 값은 없다(`CONDITIONS_POLICY.md`).
- 구성: §1 한눈에 보기 → §2 메타 분석(연구 방향) → §3 알고리즘 심층 검토 → §4 사례별 적용성 → §5 의결 필요 항목 → §6 이 검토의 범위.

---

## 1. 한눈에 보기

**연구 방향에 대한 판단 [검토].** 10-19 검토의 종합 판단("방향 유지, 다음 구현 단계로 진행")에 동의한다. 다만 "다음 구현 단계(K3)"에 들어가기 전에 **알고리즘 수준에서 정해야 할 설계 결정이 세 개**(§5의 Q2, Q3, Q4) 있고, 그 결정 없이 K3를 시작하면 10-19 검토가 경고한 부분 관측 위험(§4.1)이 그대로 구현된다.

**가장 중요한 다섯 가지.**

| # | 결론 | 근거 절 |
|---|---|---|
| 1 | **알고리즘 경로와 사례가 일대일로 묶여 있다.** 인증서 경로는 R2에서만, 예측·탐색·hold 경로는 C1/C2/C5에서만, σ_R1은 C3에서만 검증된다. 어떤 경로도 두 사례 이상에서 검증되지 않는다. 지금까지 구현한 것(K0–K4a)과 계획된 것(K3·K4b·K4c)은 인증서 경로 하나이며, 이것은 다섯 사례 중 R2 하나를 다룬다 | §2.3, §4 |
| 2 | **R2 인증서의 핵심 보조정리 L-MB는 "미증명"이지만 논증 구멍은 아니다.** SPEC이 "작성되지 않았다"고 한 공동 귀납은 실행 위치에 대한 강한 귀납으로 닫히며, R2 사슬이 쓰는 제외(FRONTIER·ONCE-PWB)는 T-OBS 없이 T-PO·T-SPAWN만으로 성립한다 [I]. 증명을 쓰면 `depends-on-stated-assumption`까지 올라간다 | §3.2 |
| 3 | **K3의 설계 위험은 구체적이다.** 현 재생 엔진의 규약("관측되지 않은 자원은 초기 상태", "지나지 않은 조건 지점은 `NOT_EVALUATED`")과 8개 관측점만 있는 탐침 계획을 그대로 합치면, 공개 뒤의 `CFE_EVS_SendEventWithAppID` 호출 247건(271−24)은 등록 조건의 LP(`EVS_AppDataIsMatch` 호출 지점)가 관측되지 않아 전부 `NOT_EVALUATED`가 되고 READY는 0건이다 [I]. 등록 조건의 LP만 탐침에 더하고 등록 사건(`register` 효과)은 더하지 않으면 같은 247건이 초기 상태 `UNREG` 때문에 전부 VIOLATING이 된다 [I]. 또한 R2의 T-OBS 간선(raise→o_ES)은 탐침 계획에 writer 지점이 없어 실제 기록에서는 만들 수 없다 | §3.4 |
| 4 | **정적 닫힘이 core-cpu1에 한정되어 C1·C2·C5를 다룰 수 없다.** 세 사례의 분석 대상은 앱(.so)이다. 이것은 알고리즘 결함이 아니라 구현 범위이지만, 로드맵이 이를 전제 조건으로 적지 않았다 | §3.5, §4 |
| 5 | **논문 범위가 설계 범위보다 작아야 한다.** 형식 핵심(상태 6종, 클래스 7종, σ 4종, M 2종, 보조정리·의무 약 27개 중 `proved` 3개, 미주장 항목 57개)은 학회 논문 한 편의 범위를 넘는다. 주장할 핵심 세 가지를 정하고 나머지는 부록·후속으로 돌려야 한다 | §2.4, §5 Q7 |

---

## 2. 메타 분석: 연구 방향

### 2.1 연구가 주장하려는 것의 분해

SPEC §1.11 "Defensible contribution"을 그대로 풀면 연구의 주장은 네 겹이다 [F].

1. **명세층**: 레퍼런스에서 가져온 readiness specification κ(K-ID 등)와 호출자 수준 Allowed(출처·horizon 포함), 세 판정(프레임워크/호출자/관측 결과)의 분리.
2. **정적 순서층**: σ·M을 한정한 must-order 인증서(MustBefore + PWB)로 "HB로 잘못된 순서로 정렬된(wrong-way) 위반"을 보편 클래스 `fixed-order`로 올림. 체커가 신뢰 기반에 상대적으로 재검증.
3. **동적층**: 잠금 재배열을 허용하는 예측(§4.9), 보유(hold) 확인(§4.13), 그리고 한정 탐색(§4.10)으로 `schedule-dependent`·refinement·progress.
4. **평가**: 탐지·분류·증거를 분리 보고, Process-Level 도구 8개와 여섯 질문으로 비교, 변이 기반 재현율은 별도.

SPEC §6.4 항목 38이 스스로 말하듯 **"탐지(알람 유무)만으로는 인증서와 예측이 더해 주는 것을 보일 수 없다"** [F]. 같은 준비 조건을 주면 단순 상태 검사가 R2의 모든 실현된 발생을 찾는다. 따라서 이 연구의 가치는 2와 3, 곧 **분류와 증거의 질**에 있다. 이 점은 연구 방향을 판단할 때 기준이 된다: 어떤 작업이 분류·증거를 강화하지 않으면 우선순위가 낮다.

### 2.2 증거 기반의 현황: 무엇이 [M]이고 무엇이 [I]인가

| 층 | 실측·실행된 것 [M] | 설계·기대만 있는 것 [I] |
|---|---|---|
| 명세 | K1: Φ 파서·평가기, 출처 기록 109개 중 98개 해시 대조, 합성 trace golden | horizon monitor(6종) 미구현 → TOLERATED 생성 불가 |
| 정적 순서 | K4a: core-cpu1 168 TU, 함수 1,591, 루프 257, `CFE_SB_Global_t.AppId` writer 3, derives 39, await 1(`MainTaskSyncDelay`) | MB 고정점, PWB, 인증서, 체커(K4b·K4c) 모두 미구현. 인증서 0건 |
| 관측 | K2: 실제 cFS 1회(`s1r2_p1`, 복원 불가 도구), 사용별 집합 24 = gdb와 일치 | 탐침 계획 자동 합성(§4.7) 없음. 관측점 8개는 손으로 쓴 것 |
| 동적 판정 | 없음 | K3 미구현. VC, LP 구간, 라우팅 모두 없음 |
| 예측·탐색·hold | 없음 | 설계만. 보조정리 L-PRED, L-DPOR, L-DEAD(a) 미증명 |
| 사례 | R2 gdb 3회(24/부팅) | C1(D22 기반 미구축), C2(역할 앱 미작성), C3·C5(6.4.x 파싱 미완) |
| 비교 | 문헌 분석 200칸 중 132칸 | 원도구 실행 0(모두 `NOT_RUN`) |

진단 [검토]: **설계는 두껍고 증거는 얇다.** 판정(verdict)이 한 건도 없고, 알고리즘의 어떤 부분도 실제 cFS 기록에 적용된 적이 없다. 지금까지의 "일치"(24 = 24)는 관측기의 집계와 gdb의 집계가 같다는 것이지 알고리즘의 결과가 아니다(`observer_smoke_run.md` §5가 스스로 그렇게 적는다 [F]).

### 2.3 알고리즘 경로 × 사례: R2 집중의 구조적 함의

사용자가 10-12에 제기한 우려("R2만 파고드는 것 같다")는 다섯 사례 매트릭스(`plan/case_basis_register.md` 등)로 일부 답했다. 그러나 **알고리즘 경로** 축으로 다시 보면 문제가 남는다.

| 경로 | 결과 종류 | 필요한 사례 | 비고 |
|---|---|---|---|
| 인증서(MB·PWB·FIXED_ORDER) + 관측 도달 | `fixed-order(σ_U, M)`, b = ∞ | **R2만** | C1·C5는 잠금으로 보호된 쌍이라 S-간선이 없어 인증서가 나오지 않는다(SPEC §1.11 둘째 행, §4.4 "locks give no order") [F] |
| trace 판정(K3) | 관측된 위반·READY·O4 | 모든 사례 | R2에서는 "지지 증거"일 뿐(§4.8 라우팅 표의 둘째 행은 인증서로 보낸다) |
| 예측(잠금 재배열 + A-COMM) | `predicted(-modelled)` witness | C1, C5, App-record 창 | R2 불필요 |
| hold 확인 | `realized/held-F0` | C1, C5 | R2 불필요(fixed-order는 수동 관측만, §4.13 표) |
| 탐색기 exh/dpor | refinement, progress, `allowed-not-ready` | C1(timeout-dependent), C2(defer horizon), C5 | R2 불필요 |
| σ_R1[P_sched] | 우선순위 의존 클래스 | **C3만** | L-CONT(b) 미증명, 실행 검증 불가(D4 model-only) |

결론 [검토]: 지금 계획된 K3→K4b→K4c→통합은 첫 줄(인증서 경로)만 완성한다. R2는 SPEC §6.4 항목 12가 명시하듯 **race가 아니라 fixed-order 의존 오류**이고, "스케줄에 따라 달라지는 순서위반을 탐지·분류한다"는 논문 제목의 중심 주장은 R2로 보일 수 없다. R2가 보이는 것은 보조 주장("HB 도구가 보고하지 못하는 wrong-way 위반을 명세+인증서로 분류한다")이다. 이 보조 주장만으로도 논문이 성립하는지는 PI가 정할 일이지만(§5 Q7), **지금의 로드맵은 그 결정 없이 R2 경로에 자원을 집중하고 있다.**

### 2.4 설계 복잡도와 검증 가능성

- 보조정리·의무 약 27개(SPEC §6.2 표의 행) 중 `proved` 3개(L-CONT(a), 그 따름정리, L-DEAD(b)), `depends-on-stated-assumption` 3개(L-DERIVE, L-CONT(c), L-DEAD(c)(d)), 나머지 `unproved` [F]. 보장 G1–G11 중 가정 의존은 G1·G3·G6이고 나머지는 `unproved`. R2 인증서 칸은 모두 "lemma status: unproved"로 출력된다.
- 가정(§6.1) 약 26개, 의무 O1–O10, 미주장 목록 57항.
- 상태 6종 × 클래스 7종 × σ 4종 × M 2종 × 축 4개(reach, progress, provider, scope) × 한정 b.

[검토] 보수성 자체는 장점이다(soundiness, 미주장 명시). 그러나 리뷰어는 "그래서 무엇을 주장하는가"를 묻는다. 지금 SPEC에는 **긍정 주장 문장**이 §1.11 "Defensible contribution" 한 단락뿐이고 그것도 [I]와 P-PAPERS 조건부다. 논문에 들어갈 긍정 주장 세 개를 먼저 적고, 그 세 개가 쓰는 보조정리·가정만 본문에 두며, 나머지(탐색기, σ_R1, loop, hold의 F1 arm)는 부록이나 후속으로 돌리는 것이 현실적이다(§5 Q7).

### 2.5 레퍼런스 정책의 작동과 부작용

`CONDITIONS_POLICY.md`는 잘 지켜지고 있다: 레퍼런스 없는 값은 `unspecified_by_reference`로 남고, deviation은 이유·영향과 함께 기록된다(`observer_smoke_run.md` §3) [F]. 부작용은 **결정 대기가 실험을 막는 것**이다. 다음 항목은 레퍼런스가 없어서 PI 결정이 필요하고, 그 전에는 S1 이후 실험이 시작될 수 없다 [F: PI_DECISIONS_RESOLVED §2, ROADMAP]:

- D14(목적별 N), D15(작업 단위 캡), D11(커널 `-fc-v139`에서 group A 재측정), 10-19 검토 §3.4의 "고정 빌드 새 실행 묶음".

이 넷은 알고리즘과 무관하지만 K3 결과를 실제 기록으로 내는 시점을 정하므로 §5에 함께 둔다.

### 2.6 10-19 검토 문서에 대한 메타 평가

| 10-19 검토의 항목 | 평가 [검토] |
|---|---|
| §1 방향 유지, 다음 구현 단계 진행 | 동의. 단 K3 설계 결정(§5 Q2–Q4)이 먼저다 |
| §3.2 "저장 전 24회" 정정 | 동의. `cmd_hits.cpp` 머리말도 "one-clock evidence, not happens-before"라고 적고 있다 [F] |
| §3.3 271 vs 251/257: 집단 확인, 원인 미확정 | 동의. 덧붙여: 차이가 있는 세 묶음(event 14, 22, 25)은 모두 **공개 뒤**의 호출이라 R2 인증서의 반증 의무 O4와 무관하다. event 25(`Q_FULL_ERR`) 39 대 24–26은 용량 손실(K-CAP) 계수가 관측 방법에 따라 달라진다는 신호이므로 D1(관측 효과)과 K-CAP 평가에 관계한다 [I] |
| §3.4 첫 실행은 개발 기록, 새 묶음으로 시작 | 동의 |
| §4.1 부분 관측과 초기 상태 구분 | 동의하고 더 구체화한다(§3.4): 위험은 "가능성"이 아니라 현 프로필·관측점 조합에서 수치로 나온다 |
| §4.2 K3 완료 조건 6개 | 동의. 항목 3("timestamp만으로 간선을 만들지 않음")은 현 재생 엔진이 이미 지킨다(간선은 규칙에서만 나온다) [F]. 항목 5(사건 제거·식별 모호화 시험)는 좋은 시험이다 |
| §5.2 인증서 발급 조건 세 상태 | 동의. SPEC §4.2 step 7의 `DISABLED`와 §1.10 `unknown(writer_incomplete)`가 이미 이 구조다. 구현이 그것을 실제로 따르는지가 문제다 |
| §5.3 체커의 의미 | 동의. 신뢰 원장 네 부분은 SPEC §4.5.1에 있고 레코드 `TRUST_LEDGERS`·`TRUST`(part (i)–(iv), `KZ_TRUST.proof`)도 K0 스키마 `include/kz/schema/kz_cert.h`에 있다 [F]. 산출물로 나온 적은 없다 |
| §6.3 C2 세 구성 | 동의 |
| §7 RacePro 특성 정정, 실행 가능성부터 | 동의 |
| §8 문서 불일치 표 | 동의. 이 문서는 적용하지 않았다(지시) |

보완 의견 두 가지:

1. 10-19 검토는 K4b·K4c에 대해 "보장 범위 제한"을 권한다. 더 앞선 질문은 **R2 인증서의 논증 자체에 구멍이 있는가**이다. 이 문서 §3.2가 그 답을 시도한다: 구멍은 없고, 미작성 귀납은 쓸 수 있다.
2. 10-19 검토 §9의 순서(기준 정리 → 고정 빌드 → K3 → K4b·K4c → R2 대조)는 맞지만, **K3를 "실제 기록 판정"으로 정의하면서 탐침 계획(§4.7)을 언급하지 않았다.** 지금 관측점 8개로는 K3가 R2의 T-OBS 간선을 만들 수 없다(§3.4). 탐침 계획 합성 또는 손 추가가 K3의 일부여야 한다.

---

## 3. race 탐지 알고리즘 심층 검토

### 3.0 전체 구조와 "탐지"가 일어나는 곳

파이프라인 [F: SPEC §2.4, §4]: 닫힘·추출(`basis`, `extract`, `link`) → writer 집합(§4.2) → 골격 두 뷰·L1 요약·메시지 출처(§4.3) → MustBefore 최대 고정점(§4.4) → 인증서·체커(§4.5) → census(§4.6) → 탐침 계획(§4.7) → 관측 → trace 판정(§4.8) → 예측(§4.9) → 탐색(§4.10) → 적합성(§4.11) → 결합(§4.12) → hold(§4.13) → 루프(§4.14).

"탐지"는 세 경로에서 나온다. (i) 인증서: 보편 진술 `¬OK_i`(fixed-order) 또는 `¬NR_i`(protected). (ii) 관측: trace에서 최종 상태를 가진 발생(witness). (iii) 모델: 예측·탐색의 witness. 알람 = bad witness 또는 결정 경로가 있는 fixed-order. **같은 사건을 두 경로가 다르게 말하면 O10·O6·suspension으로 처리**(§4.12 항목 5). 이 구조는 일관적이다.

### 3.1 명세층: κ, Allowed, 세 판정, 상태

**A-1. 상태 정의와 최종성** [F: §1.6]. `NOT_EVALUATED`의 분리(조기 반환), 결정 사건, 서수 (a, k), 범위(scope)의 정의는 2026-10-15 정정(C-1) 뒤 L-FO·L-PR이 증명하는 것과 일치한다. 재생 엔진은 이 정의를 그대로 구현한다(`phi_reference.md` §7.5) [F].

**A-2. 결정 경로의 범위가 좁다** [F: §1.10 "decision route"]. 인증서가 `fixed-order`를 결정하는 것은 REJECT가 결정 사건에 있을 때뿐이다: `Allowed = none`, `unspecified_by_reference`, 항목 코드 밖의 결과. `retry_until_ready`(C1의 CF), `returned_and_tested`, `defer_by_design`(C2, TO_LAB)의 REJECT는 결정 뒤에 있으므로 인증서만으로는 `unknown(no_decision_route)`이고 bad witness(실행 또는 exh)가 있어야 fixed-order가 된다. [검토] 설계 의도이고 옳다(결정 뒤 경로는 σ_U∞가 지우는 전이로 막힐 수 있다). 그러나 이 때문에 **"must-order 인증서로 fixed-order를 올린다"는 주장은 R2형(Allowed none)에 한정**된다. 논문에서 이 범위를 명시해야 한다.

**A-3. horizon monitor의 REJECT 중 하나는 모델 상대적이다** [F: §1.6 "μ also gives REJECT as soon as no `Sk^cert` path … reaches the ACCEPT event"]. 이 경로 검사 REJECT는 ρ의 사건이 아니라 `Sk^cert`(모델)를 읽는다. 따라서:
- L-STATUS의 (a) "상태는 ρ의 효과 사건만으로 정해진다"는 이 REJECT에 대해 문자 그대로는 거짓이다. 최종성(b)은 A-SKEL 아래에서 성립한다(과대 근사 골격에 경로가 없으면 실제 실행에도 없다).
- 결과: 경로 검사 REJECT로 얻은 `VIOLATING`은 A-SKEL에 상대적이고, 그 사실이 원장 (iii)에 적혀야 한다. L-STATUS의 증명 상태를 `proved`로 올릴 때(proof_status_R2 §10의 1번 작업) 이 항목은 `depends-on-stated-assumption(A-SKEL)`로 갈라야 한다 [검토].
- R2에는 영향 없음(`Allowed = none`).

**A-4. 값 conjunct의 포획(capture) 지점** [F: §1.6 Instances, §4.8 항목 4]. 재생은 "호출 직전 마지막 포획 사건"을 호출에 붙이고, 포획이 없으면 호출 자체에서 평가한다. 실제 관측점(`sites.kzp`)은 `evs.send.entry`의 `rdx:u32:app_id`만 잡으므로 R2에서는 호출 인자 자체가 값이라 문제가 없다. 그러나 정적 L-DERIVE는 "포획 지점 c의 loc 값 = API 검사 값"을 말하고, trace의 LP 구간은 `[이전 사건, 호출 진입]`이다. 포획 지점에 탐침이 없으면 이 구간이 넓어 HB 결정이 열린 채로 남는다(§4.8 항목 4가 "너무 넓으면 포획 사이트를 탐침"이라 적는다). K3·탐침 계획이 이것을 어떻게 결정하는지 명시되어야 한다(§5 Q3).

### 3.2 정적 순서층: MustBefore, PWB, 인증서 — R2의 핵심 사슬

**정의 재확인** [F: §4.4]. MB는 `Sk^cert` 위의 **최대** 고정점이다. T-OBS의 writer 집합 `W_o(MB)`는 현재 MB로 PWB 제외를 계산하므로 MB에 반단조이고, 그래서 연산자는 단조 감소한다. SPEC은 L-MB를 `unproved`로 두며 그 이유를 "T-OBS가 같은 고정점에서 나온 PWB 사실을 쓰므로 MB와 PWB의 공동 귀납이 주장만 되고 쓰이지 않았다"고 적는다 [F: §6.2 표].

**B-1. 공동 귀납은 닫힌다 [I, 검토자의 논증].** 주장 P(k): "유한 실행 ρ의 위치 ≤ k에 있는 모든 노드 발생 n_k와 모든 a ∈ MB(n)에 대해 a의 발생이 n_k보다 앞에 있다." 위치에 대한 강한 귀납으로, T-OBS 사례에서 쓰는 모든 MB 사실이 **o_k보다 앞선 위치**에서만 적용됨을 확인하면 된다.

- 반증자 f_j < o_k(지배, 최신 발생). C가 o_k에서 참(L1: 원자적 시험 + `Pred_code ⇒ C`). 그러므로 f_j와 o_k 사이에 C를 참으로 만든 쓰기 w_l이 있고 A-WS로 w ∈ W_C.
- w가 제외 규칙으로 `W_o`에서 빠졌다면 모순을 보인다. 각 규칙이 쓰는 귀납 가정은 w_l, f_j, m_i(모두 < o_k)에서만 쓰인다:
  - PWB-before-falsifier, ONCE-PWB(w, f): once(w) ∧ w ∈ MB(f). 가정 P(f_j): 어떤 w_i < f_j. once(w) ⇒ w_l = w_i < f_j. f_j < w_l과 모순.
  - PWB-before-falsifier, FRONTIER: once(actor(w)), m ∉ Anc(w), m ∈ MB(f). P(f_j): m_i < f_j. w의 유일한 실행에서 w_l > m_i이면 A-SKEL로 경로 m →* w가 있어 m ∉ Anc(w)와 모순. 그러므로 w_l < m_i < f_j. 모순.
  - PWB-after-observation, ONCE-PWB(o, w): once(o) ∧ o ∈ MB(w). P(w_l): 어떤 o_i < w_l < o_k. once(o) ⇒ o_i = o_k. 모순.
  - PWB-after-observation, FRONTIER: m ∈ actor(o), m ∉ Anc(o), m ∈ MB(w). P(w_l): m_i < w_l. o_k > m_i이면 경로 m →* o. 모순. 그러므로 o_k < m_i < w_l < o_k. 모순.
  - OWN-NOPATH: 귀납 가정 불필요(A-SKEL만).
  - KEY-SEP: NO_KEY는 C의 키를 바꾸지 못함(A-FRESH 등 키 상태의 전제).
- 따라서 w ∈ W_o이고 a ∈ MB(w) ∪ {w}에서 P(w_l)로 a의 발생 < w_l < o_k.
- T-PO, T-SPAWN은 선행 발생(p_j, c_j < n_k)에 P를 적용. 기저(∅)는 자명.

결론: 최대 고정점이 "자기 일관적"일 뿐 거짓을 포함할 위험은 **없다**. 모든 PWB 사용이 o_k 앞의 위치에서 귀납 가정을 쓰기 때문이다. 전제는 A-SKEL, A3(spawn 목록·once 사실), A-WS, A-EXTRACT(반증자 사실, 직렬화된 탈출식), A1(Φ 효과), L1. 이것을 쓰면 L-MB는 `depends-on-stated-assumption`이 되고, L-ONCE·L-PWB·L-SCOPE·L-FO·L-PR이 따라 올라간다(proof_status_R2 §10 3번 작업과 일치). **이 문서는 SPEC을 고치지 않았다.** 위 논증은 검토 결과이며 SPEC에 넣을지는 §5 Q1.

**B-2. R2 사슬의 겉보기 순환** [F: App. A.1 step 4–7; I]. A.1 표는 EVS·SB·TBL·TIME·앱의 writer를 "FRONTIER: PWB(o_ES, w)"로 제외하고, 그 결과 `m_ES ∈ MB(o_ES)`를 얻는다. 겉보기에는 "m_ES ∈ MB(o_ES) ⇐ EVS writer 제외 ⇐ PWB(o_ES, w_EVS) ⇐ m_ES ∈ MB(w_EVS) ⇐ m_ES ∈ MB(spawn_EVS) ⇐ m_ES ∈ MB(o_ES)"의 순환처럼 보인다. 그러나 FRONTIER의 m은 `m_ES`가 아니어도 된다: `m = spawn_EVS`(시작 스레드에서 o_ES 뒤, `spawn_EVS ∉ Anc(o_ES)`, `spawn_EVS ∈ MB(w_EVS)`는 T-SPAWN·T-PO만으로 성립)를 쓰면 T-OBS 없이 제외된다. ONCE-PWB로도 된다: once(o_ES)이고 `o_ES ∈ MB(w_EVS)`는 T-PO(시작 스레드)·T-SPAWN으로 성립. 따라서 **층화 계산**(먼저 PWB 없이 MB⁰, 그 위에서 PWB¹, 다시 MB¹…)으로도 R2 사슬이 나온다 [I]. 권고: K4b 구현은 층화 계산을 기본으로 두고, 최대 고정점과 결과가 같은지 R2와 litmus(L-H2, L-ONCE, L-OWN)에서 시험한다. 다르면 층화 결과(더 보수적)를 쓴다.

**B-3. R2 인증서가 실제로 기대는 것의 목록** [F: A.1 step 4–5, 9a; static_facts.md §6, §8]. 보조정리 상태 외에 다음이 결과를 정한다.

| 항목 | 상태 | 비고 |
|---|---|---|
| `Pred_code ⇒ C_ES` | 자명한 인스턴스화(∀r over 32 records ⇒ ρ_ES) | K4a가 술어를 `forall i 32 (not (and IsUsed (lt AppState AppStateId)))`로 추출 [M] |
| L1 (i)(ii) 전제 | K4a가 추출 사실 위에서 확인 [M] | **callee 안에서 잠금을 푸는 경우는 보지 않음**(§8). `MainTaskSyncDelay`에는 그런 callee가 없다고 기대 [I] |
| 반증자 `:803`이 C_ES를 거짓으로 만듦 | ES 반쪽은 A-FRESH 불필요(첫 예약), EVS 반쪽은 A-FRESH·A-OWNER 필요 | `DECLARED_TRUSTED`, O9로만 반증 |
| writer 집합 완전성(AppState, AppId/IsUsed, Type) | `writer-set-complete` [M], 단 G1–G4 가정 | 10-19 검토 §5.1과 같음 |
| 중립 분류(ExitApp STOPPED, scan WAITING, `:759` UNDEFINED, memset 0) | **§4.2 step 6 전이 필터 미구현**. A.1 표의 분류는 손으로 한 것 | K4b의 일 |
| `CleanUpApp` 해제 제외 | 관계 보호 조건, `DECLARED_TRUSTED` | 기본 부팅으로는 반증 불가(exercised-path limit) [F: 9a (iv)] |
| `:823` → panic만 뒤따름(OWN-NOPATH) | SPEC 인용 | 이번에 재확인하지 않음 [U] |
| R19 버전 가드 `cfe >= 6.4.2` | 재검증 안 됨 [F: phi_reference §11 항목 4] | v7.0.1에는 영향 없음 |
| ES_BG_TASK의 쓰기 | 중립 또는 신뢰 | ES_BG_TASK는 `CFE_ES_TaskInit`(`cfe_es_task.c:114`에서 호출) 안의 `CFE_ES_BackgroundInit()`(`:474`)이 만들고, raise는 그 뒤 `:134`에 있다 [F: 고정 소스 재확인]. 그래서 이 행들은 "o_ES 뒤에 생성"으로 제외되지 않고 **전이 필터와 신뢰 조건에만** 기댄다 |

[검토] R2 인증서의 가장 약한 고리는 보조정리 상태가 아니라 마지막 두 행이다: 전이 필터가 구현되지 않았고, `CleanUpApp` 보호 조건은 기본 부팅으로 반증할 수 없다. 논문은 이것을 "trusted:2, 반증 경로 없음"으로 정직하게 적어야 하고, SPEC 9a (iv)가 이미 그렇게 설계되어 있다.

**B-4. 잠금은 순서를 주지 않는다** [F: §4.4]. 옳다(atomicity ≠ order, [R04 L5]). 결과로 C1·C5처럼 잠금 안에서 publish/lookup이 일어나는 쌍에는 인증서가 없고(§1.11), 예측·탐색이 맡는다. 일관적이다.

**B-5. 체커의 독립성 범위** [F: §4.5]. "추론의 독립, 사실의 독립 아님"이 정확하며, via T5 단계(반증자 조건, INIT_FALSE, R19 해석)가 표시된다. 변이 시험 16종은 체커를 시험하고 분석을 시험하지 않는다고 명시한다. [검토] 추가할 변이 하나: **층화 계산과 최대 고정점의 차이를 이용한 변이**(B-2) — 최대 고정점에서만 성립하는 MB 사실을 인증서에 넣었을 때 체커가 이를 거부하는지. 체커가 최대 고정점으로 재계산하면 통과시킬 것이므로, 체커의 MB 재도출은 층화 방식이어야 한다.

### 3.3 요약·골격층: L1, 골격 두 뷰, 닫힘, 메시지 출처

**C-1. L1은 두 주장으로 갈라야 한다** [F: §6.2 L1]. (i) 안전성: 조건 탈출 ↔ `await` 성공 전이(유한 실행). (ii) 진행성: 요약된 poller가 죽는 상태 = 요약 안 된 poller가 갇히는 상태. R2는 (i)만 쓴다. 지금 L1 하나로 두면 (ii)의 어려움이 R2의 증명 상태를 끌어내린다. [검토] L1-safety / L1-progress로 분리 권고(§5 Q5).

**C-2. L1 (ii)의 사각** [F: static_facts.md §8]. "호출이 잠금 구간 안이라는 것은 같은 함수 본문 안의 lock/unlock 순서로만 판단한다(callee 안에서 잠금을 푸는 경우는 보지 않는다)". 이것은 건전하지 않은 방향의 빈틈이다. 권고: 잠금 구간 안의 모든 callee에 대해 "잠금 연산을 호출하지 않음"을 호출 그래프로 확인하고, 미해결 간접 호출이 있으면 L1 (ii)를 `undecided`로 둔다. `MainTaskSyncDelay`에서 결과가 바뀌지 않을 것으로 기대 [I].

**C-3. 골격 두 뷰** [F: §4.3]. 인증 뷰(별 닫힘, 지평 없음)와 탐색 뷰(h, TRUNC). 보편 진술은 인증 뷰에서만, 체커는 TRUNC 노드를 거부. 일관적. 인라이닝 폴백(per-(actor, function) 요약)의 정밀도 손실이 모두 건전한 방향이라는 설명도 맞다. 단 "템플릿이 문맥별이 아니게 되어 공유 SB 함수 안의 사용에 R2형 인증서를 보류할 수 있다"는 점은 R2에 직접 닿는다: 39개 SB 사이트가 `cfe_sb_api.c` 34개에 있고 ES·EVS 문맥에서 인라인되어야 한다. K4a의 "도달성"은 spawn 간선 없는 과대 근사라 아직 이 인라이닝을 하지 않았다 [F: static_facts §6]. K4b에서 폴백이 일어나면 R2 인증서가 나오지 않을 수 있다 [I]. 작업 단위 캡(D15)이 여기에도 걸린다.

**C-4. 닫힘이 core-cpu1에 한정** [F: static_facts §8 G1]. 앱(.so)이 cFE 전역을 이름으로 쓰는 경우는 닫힌 프로그램 밖이다. R2에는 영향이 없다(앱은 모두 SB 공개 뒤에 생성되고, 앱이 부르는 ES API의 writer는 core 안에 있다). 그러나 **C1(CF, BP), C2(역할 앱), C5의 분석 대상은 앱 코드 자체**이므로 닫힘을 앱까지 넓히지 않으면 K4a·K4b는 그 세 사례에 적용될 수 없다. SPEC §6.1 A3는 "코드가 compile DB 밖이면 모델 밖"이라고만 적는다. 로드맵에 "앱을 compile DB에 포함"이 전제로 적혀야 한다(§5 Q4).

**C-5. 미해결 간접 호출 7개** [F: static_facts §6]. 대상 형 포인터가 미해결 호출로 가는 곳은 0이므로 R2의 writer 집합에는 영향이 없다 [M]. spawn 호출 4개는 모두 해결 [M]. 루프 분석에서는 `mod-incomplete`. 괜찮다.

**C-6. 메시지 출처·전이 필터·키·앵커 감사: 모두 미구현** [F: static_facts §8 "하지 않은 SPEC 단계"]. 전이 필터는 B-3의 중립 행을 정하고, 키·할당 레코드는 EVS 반쪽의 KEY-SEP(ρ_EVS ≠ ρ_ES)를 정한다. 메시지 출처는 R2에는 불필요하지만(핸들러 밖의 초기화 코드) C2에는 핵심이다.

### 3.4 관측·재생층: K2 → K3

**D-1. timestamp와 HB** [F: `cmd_hits.cpp` 머리말, `observer_smoke_run.md` §5]. `kz hits --before`는 한 시계의 병합 순서이며 HB가 아니라고 적혀 있다. 재생 엔진은 간선을 규칙(program_order, spawn, phase, observe)에서만 만든다 [F: eval_trace.cpp `add_edge` 호출 네 곳]. VC·VC_R 계산(§4.8 항목 3)은 아직 없다. 10-19 검토 §4.2 항목 3은 설계상 이미 충족된다.

**D-2. 부분 관측의 구체적 결과 [I, 핵심].** 네 사실을 합친다.
- 재생 규약 두 개: "상태가 정해진 적 없는 자원은 종류의 초기 상태"(`evs.reg`의 초기 상태는 `UNREG`, `10_kinds.kzp:73`); "호출이 조건 지점을 지나지 않고 반환하면 `NOT_EVALUATED`" [F: phi_reference §7.5, 10-19 검토 §4.1].
- 관측점 8개: `evs.send.entry/ret`, `sb.appid.store`, `es.sync.entry/ret`, `osal.task_create.entry/ret`, `osal.task_entry` [F: sites.kzp]. `EVS_Register`(evs.reg의 `register` 효과), `raise`(`cfe_es_api.c:573`), `SetUsed`(`:803`), memset(`:82`, `:823`), 그리고 등록 조건의 LP 어느 것도 없다.
- op 선언: `id_valid`는 `lp capture`(호출 인자에서 평가), `slot_registered`는 `lp at fn("CFE_EVS_SendEventWithAppID").call("EVS_AppDataIsMatch")`, 곧 호출 안의 지점 LP [F: 20_ops.kzp:17–18].
- rspec `K-ID.send-with-appid`: `requires valid(arg.AppID) && state(evs.reg(arg.AppID)) = REG` [F: 40_rspec.kzp].

결과, 두 시나리오.
- **지금 계획 그대로**: 공개 뒤의 호출 247건(271 − 24 [M])은 `valid`가 참이고 등록 조건의 지점 사건이 없으므로 반환에서 **전부 `NOT_EVALUATED`**. READY는 0건. O4(a)는 "거짓으로 평가된 조건 없음 + 성공 결과"를 `ready`로 맞추므로 발견 사항도 없다. 24건은 포획이 거짓이라 호출에서 VIOLATING. 출력은 정합적이지만 **R2의 protected 쪽 증거(공개 뒤에는 READY)가 원리적으로 나올 수 없고**, 그 사실이 조용히 지나간다.
- **등록 조건의 LP만 탐침에 더하고 `register` 효과는 더하지 않으면**: 같은 247건이 초기 상태 `UNREG` 때문에 둘째 조건 거짓 → **전부 VIOLATING**, 같은 수의 O4(a) 불일치. 10-19 검토 §4.1이 경고한 "미관측을 미등록으로 읽는" 경우가 이것이다.

둘 다 10-19 검토 §4.1의 표가 요구하는 "상태 변경 미관측 → 판단 불가"와 어긋난다. 필요한 것은 **재생 엔진의 규약 변경**(계획이 writer를 덮지 않는 종류의 자원을 읽는 조건은 초기 상태로 평가하지 않고 `undecidable`로 둠)과 **탐침 계획의 완결 조건**(조건 LP와 그 종류의 writer LP가 함께 있어야 함)이다(§5 Q2, Q3).

**D-3. R2의 T-OBS 간선을 실제 기록에서 만들 수 없다 [I].** R19는 `es.sync.ret`가 `CFE_SUCCESS`이고 술어가 참일 때 `es.app`·`es.appstate`의 **다른 행위자의 마지막 쓰기**에서 간선을 만든다 [F: 30_hb.kzp, eval_trace.cpp `observe_return`]. 쓰기 사건이 하나도 관측되지 않으므로 (a) 술어는 초기 상태(모든 레코드 FREE)에서 자명하게 참이 되어 발견 사항도 없고, (b) 마지막 쓰기가 없어 간선이 0개다. 그러면 ES의 24개 사용 → SB 저장의 HB 사슬에서 `raise_ES → o_ES` 고리가 빠지고, trace만으로는 wrong-way가 "S-간선으로 정렬"되지 않는다. §4.8 라우팅 표의 둘째 행("every ordering edge used is S")에 들어가려면 raise 지점의 탐침이 필요하다. 즉 **K3가 R2에서 의미 있는 출력을 내려면 탐침 계획이 바뀌어야 한다**(§5 Q3). SPEC §4.7 항목 1–2는 합성된 계획이 "모든 S·O 규칙의 앵커(ES 잠금 래퍼, 대기, `ExitApp`, 앱·task 생성 등)"와 "열린 인스턴스의 모든 conjunct의 LP 구간, API 경계가 덮지 않는 readiness 필드의 쓰기 감시점"을 포함한다고 적는다 [F]. 따라서 §4.7대로 합성하면 raise와 등록 LP가 들어온다. 문제는 §4.7이 구현되지 않았고 `sites.kzp`가 그 부분집합을 손으로 쓴 것이라는 점이다.

**D-4. spawn 간선의 짝짓기** [F: phi_reference §7.4, observer_smoke_run §4.1]. 재생은 `ret`의 `out.P`와 진입의 `arg.P`를 값으로 짝짓는다. perf-sample은 out 매개변수를 읽지 못해 K2는 FORK 시각 포함으로 짝지었고 알린 id 24개는 `id-unverified`다. K3는 K2의 짝을 재생의 값 짝으로 옮기는 어댑터가 필요하고, 그 간선에는 `assumed`(A-CLK-PO, 시각 포함) 표시가 붙어야 한다. 자식이 생성 호출 반환 전에 돌 수 있다는 점은 재생이 이미 반영한다(호출 사건 → 진입) [F].

**D-5. phase 규칙 R18** [F: 30_hb.kzp note]. "둘째 루프의 첫 LockSharedData"를 기준점으로 잡은 연구자 근사이며, 그 지점에도 탐침이 없다. R2 인증서에는 쓰이지 않는다 [I].

**D-6. 손실 집계** [F: observer_smoke_run §4, 10-19 검토 §4.2]. `PERF_RECORD_LOST` 0은 "커널이 보고하지 않음"이다. SPEC §5.6의 per-thread `tseq` 틈 검사가 K3에서 구현되어야 `unknown(loss)`가 나온다.

### 3.5 예측층(§4.9): 설계 검토, 미구현

**E-1. A-COMM과 희소 trace.** Kairos trace는 동기화 골격과 LP 구간만 담는다. `predicted`(trace-sound)가 되려면 cone 행위자의 **관측되지 않은 코드**가 읽는, 다른 행위자가 쓸 수 있는 모든 위치가 관측·상수·결과 보존 중 하나여야 한다. 현실적으로 대부분 `predicted-modelled`가 될 것이다 [I]. G4의 적용 범위가 좁다는 것을 논문이 미리 말해야 한다.

**E-2. 잠금 재배열.** "acquire needs the lock free" 제약과 임계 구역 원자 블록은 RVPredict/GPredict 계열의 처리와 같고 선행 연구로 인정한다 [F: §1.11]. C1에서 역순(lookup이 create보다 먼저) witness는 `NAME_NOT_FOUND` → `retry_until_ready` → PENDING → 요약 외삽으로 bound까지 → `predicted-extrapolated`. 이 witness가 σ_U∞에서는 사라진다(마지막 시도의 실패가 시간 기반 전이) → C1은 `timeout-dependent` 후보. 설계가 사례와 맞물린다 [I].

**E-3. NP-난해성과 캡** [F]. 작업 단위 캡은 deviation이고 anytime 곡선으로 보고. 괜찮다.

### 3.6 탐색층(§4.10): 설계 검토, 미구현

**F-1. 닫힌 cone의 크기.** SPEC이 스스로 기대하듯 R2형 인스턴스의 cone은 시작 스레드·모든 core task·대부분의 앱을 포함한다 [F: §4.10 항목 1]. 측정 전이다.

**F-2. 별 닫힘과 `choice-dependent`.** 인증 뷰의 모든 메시지 루프 탈출이 지역 선택이므로, 탐색기의 stall witness는 거의 모두 `choice-dependent`가 되고 §1.10 계수 규칙에 따라 **세지 않는다** [I]. 진행성 결과는 대부분 "후보"로만 남는다. 권고: `CFE_ES_RunLoop`의 탈출은 B.Input의 명령으로만 가능하므로 메시지 출처 fixpoint로 접어(fold) 선택을 줄일 수 있는지 검토(설계 개선 제안, 결함 아님).

**F-3. dpor.** T-JACM 미검증, `unverified` 표시, exh 차등 시험은 증거일 뿐. 정직하다. 논문에서는 exh만 쓰는 것이 안전하다 [검토].

**F-4. 안전 측 검열(safety-side censoring), 모니터 의존성 확장.** 설계가 치밀하다. 구현 난도가 높고 R2에는 불필요.

### 3.7 결합·확인층(§4.12–§4.14)

**G-1. 격자와 종료.** 단조 상승, suspension은 Φ 버전을 끝냄. 일관적.

**G-2. hold.** 이벤트 기반 해제, 디버그 레지스터, 감시견 상수는 레퍼런스에서. C1의 hold 설계(P-C1HOLD)와 맞는다. R2에는 쓰이지 않는다.

**G-3. same-ordinal 규칙.** `schedule-dependent`는 같은 서수의 bad·good 쌍. R2의 version-event 템플릿은 부팅당 13 서수. 정준 이름 (τ, generation, n)으로 짝짓는다. 괜찮다.

### 3.8 선행 연구 대비 위치

- R2가 HB 도구(FastTrack, WCP, SHB, SyncP)의 보고 범위 밖(wrong-way, HB로 정렬됨)이라는 설명은 정확하다 [F: §1.11 첫 행].
- MB·PWB 인증서는 본질적으로 **정적 must-happen-before(혹은 must-precede) 분석**이다. SPEC은 SIERRA(정적 action HB)를 인용하지만, Java의 MHP(may-happen-in-parallel) 분석 계열(Naumovich–Avrunin–Clarke 1999 등)이나 must-HB 분석과의 관계는 적지 않았다 [U: 이 검토에서 문헌을 확인하지 않음]. 리뷰어가 물을 가능성이 높으므로 P-PAPERS와 같은 수준의 문헌 확인 항목으로 두는 것이 안전하다.
- "기여는 메커니즘이 아니라 cFS 의미 중심의 추출·분류·증거"라는 입장은 방어 가능하다. 다만 §2.1에서 말했듯 그 기여는 **분류·증거 결과가 실제로 있을 때만** 성립한다.

### 3.9 보조정리 상태표에 대한 독립 의견

| 보조정리 | SPEC 상태 | 검토 의견 |
|---|---|---|
| L-MB | unproved | B-1의 귀납으로 `depends-on-stated-assumption(A-SKEL, A3, A-WS, A-EXTRACT, A1; L1-safety)`까지 가능 |
| L-ONCE, L-PWB, L-SCOPE, L-FO, L-PR | unproved | L-MB를 따라 올라감 |
| L-STATUS | unproved | 정의적이나 A-3의 경로 검사 REJECT 때문에 "일부는 A-SKEL 의존"으로 갈라야 함 |
| L1 | unproved(의무) | 안전성/진행성 분리 권고(C-1). 안전성 부분은 L1 (ii) callee 사각(C-2)을 닫으면 `depends-on-stated-assumption(A-EXTRACT)` |
| L-CONT (c) | depends-on-stated-assumption | 동의 |
| L-MAPI-CONE, L-REACH-CONE | unproved(의무) | R2 (σ_U, M_api) 도달과 σ_U∞ 도달에 필요. 접두 탐침(§4.7 항목 6)이 없으면 어차피 `assumed` |
| L-DEAD, L-DPOR, L-REF, L-PRED, L-LOOP, L2, L3 | unproved | R2에 쓰이지 않음. 논문 본문에서 빼도 R2 결과는 영향 없음 |

---

## 4. 사례별 알고리즘 적용성

| 사례 | 필요한 알고리즘 경로 | 현 구현이 닿는 범위 | 알고리즘·구현상 걸림돌 |
|---|---|---|---|
| **R2** (cFE#2663) | 인증서(σ_U) + 관측 도달 + O4 | K0–K4a 완료, K3·K4b·K4c 예정 | D-2(초기 상태 규약), D-3(raise 탐침 없음), 전이 필터 미구현(B-3), `CleanUpApp` 신뢰 조건 반증 불가, L-MB 귀납 미작성(B-1) |
| **C1** (CF#184) | 닫힘에 앱 포함 → `retry_until_ready` monitor → 예측(잠금 재배열+외삽) → hold → 탐색(σ_U∞ refinement) | 없음(K4a 닫힘 밖, monitor 미구현) | C-4, A-2(결정 경로는 bad witness 필요), E-1, D22 기반 미구축 |
| **C2** (cFE#198) | 역할 앱 → `defer_by_design`형 horizon(아직 없는 monitored actor) → 메시지 출처 → 예측·탐색 | 없음 | C-4, C-6(메시지 출처), A-3(경로 검사 REJECT), 세 구성 정정(10-19 §6.3) |
| **C3** (cFE#73) | 6.4.1 코드 파싱 → σ_R1[P_sched] 탐색(모델 전용) | 없음 | L-CONT(b)·L-CONE-R1 미증명, D13 헤더 민감도, 실행 검증 불가(D4) |
| **C5** (cFE#72) | 6.4.x 코드 → ES 잠금 아래 publish/lookup 쌍 → 예측 + hold | 없음 | C-4, E-1, D13 |

[검토] 인증서 경로가 유효한 것은 R2뿐이고, 나머지 넷은 예측·탐색·hold 중 적어도 둘이 필요하며 그것들은 설계만 있다. "Kairos가 다섯 사례를 다룬다"는 지금 알고리즘 검증 상태에서 1/5이다. 10-19 검토 §9의 "병행 A–D"가 이것을 메우려 하지만, **각 병행 작업이 어느 알고리즘 경로를 처음 검증하는지**가 적혀 있지 않다. 그것을 적으면 우선순위가 보인다: C1(예측·hold·σ_U∞)이 가장 많은 경로를 처음 검증한다.

---

## 5. 의결이 필요한 항목

후속 작업은 하지 않았다. 아래는 결정만 요청한다. 권고는 [검토]다.

| # | 결정 | 선택지 | 권고 | 근거 |
|---|---|---|---|---|
| Q1 | L-MB 귀납 증명을 SPEC §6.2에 쓸지 | (a) 쓰고 상태를 올린다 (b) 미증명으로 둔다 | (a). 작성 비용이 작고 R2 인증서의 모든 보조정리가 따라 올라간다 | §3.2 B-1 |
| Q2 | 재생 엔진의 "미관측 자원 = 초기 상태" 규약 | (a) 관측 범위 밖 자원을 `unobserved`로 두고 그것을 읽는 조건은 `undecidable` (b) 탐침 계획이 writer를 모두 덮는 종류만 초기 상태 허용 (c) 현행 유지 | (a)+(b): 종류별로 "writer LP가 계획에 있음"을 K3 입력으로 요구하고, 없으면 undecidable | §3.4 D-2 |
| Q3 | 탐침 계획을 K3 전에 바꿀지 | (a) §4.7 합성 구현 (b) `sites.kzp`에 raise(`cfe_es_api.c:573`), `:803`, `:82`, `EVS_Register`를 손으로 추가 (c) K3를 인증서 보조로만 두고 trace HB는 포기 | (b) 먼저, (a)는 K4b 뒤. (c)는 K3의 의미를 없앤다 | §3.4 D-3 |
| Q4 | 정적 닫힘을 앱(.so)까지 넓힐지, 언제 | (a) C1 기반(D22) 구축과 함께 (b) R2 통합 뒤 (c) 하지 않음 | (a). 하지 않으면 C1·C2·C5에 K4a·K4b가 적용될 수 없다 | §3.3 C-4 |
| Q5 | L1 분리 | (a) L1-safety / L1-progress로 분리 (b) 유지 | (a) | §3.3 C-1 |
| Q6 | L-STATUS의 경로 검사 REJECT를 A-SKEL 의존으로 갈라 적을지 | (a) 갈라 적는다 (b) 유지 | (a) | §3.1 A-3 |
| Q7 | 논문의 긍정 주장 범위 | (a) 명세+인증서(R2) + trace 판정 + 예측(C1 또는 C5 한 사례) 세 주장, 탐색기·σ_R1·loop·F1은 부록·후속 (b) 전체 (c) 명세+인증서만 | (a). (c)는 race 논문이 아니고 (b)는 검증 불가 | §2.3, §2.4 |
| Q8 | K4b의 MB 계산 방식 | (a) 층화 계산 기본, 최대 고정점은 시험용 (b) 최대 고정점 | (a) | §3.2 B-2, B-5 |
| Q9 | 실험을 막는 결정 대기 항목 | D14(N), D15(캡), D11(재측정), 고정 빌드 묶음 | 네 항목을 S1 파일럿 하나로 묶어 결정 시점을 정한다 | §2.5 |
| Q10 | 문헌 확인 추가 | MHP/must-HB 정적 분석 계열을 P-PAPERS에 추가할지 | 추가 | §3.8 |
| Q11 | 10-19 검토 §8의 문서 정정 목록 | 적용 시점 | K3 착수 전(검토 §9 순서 1과 같음) | §2.6 |

---

## 6. 이 검토의 범위와 하지 않은 것

- 수행: 10-19 검토 문서 전문, SPEC rev 1.2의 §1.6–§1.11, §4.2–§4.15, §6.1–§6.4, App. A.1, 계획·기록 문서, 프로필 세 개, 재생 엔진·루프 분석·`kz hits`의 해당 코드 읽기. 보조정리 L-MB의 귀납(B-1)과 R2 사슬의 순환 여부(B-2)는 손으로 따졌다 [I].
- 하지 않음: 빌드·시험·cFS 실행·K4a 재실행, 코드·명세·계획 수정, 워크플로 재개, CORD 삭제, 10-19 검토의 정정 적용, 문헌 확인(§3.8의 MHP 계열은 [U]).
- 한계: B-1·B-2의 논증은 검토자 한 사람의 것이고 독립 검토가 없다(SPEC PS1의 "작성자의 분류"와 같은 지위). D-2의 247이라는 수는 `271 − 24`의 산술이며, 실제 K3 출력이 아니다. R2 인증서의 writer 표(B-3)는 SPEC App. A.1의 기대를 옮긴 것이고 도구가 만든 적이 없다.
- 변경 파일: 이 문서 하나.

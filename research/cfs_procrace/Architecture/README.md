# Kairos 아키텍처 문서

NASA cFS v7.0.1에서 앱·서비스 task 사이의 준비(readiness)·순서 위반을 판정하는 설계 Kairos(도구 접두어 `kz`)를 한국어로 풀어 쓴 문서 모음이다. 이 파일은 길잡이일 뿐이고, 설계 내용은 담지 않는다.

상태: 설계 문서 — 구현·측정 전. 규범 원문: [spec/SPEC_FINAL.md](spec/SPEC_FINAL.md)(영문) **rev 1.4**(2026-10-26). rev 1.4는 2026-10-26 PI 결정([spec/PI_DECISIONS_RESOLVED_20261026.md](spec/PI_DECISIONS_RESOLVED_20261026.md))을 반영한 판이다: 분석기 하나에 실행 계열 둘(재계측 없이 기준 빌드를 다루는 F0와 계측 계열), 공통 사건·제어 의미, 계열별 주장 범위(spec §0.13, ADR-059). 한국어 문서에는 02·06·07·10에 짧게 옮겼다. rev 1.3은 PI 결정 D1–D5(2026-10-22, [spec/PI_DECISIONS_RESOLVED_20261022.md](spec/PI_DECISIONS_RESOLVED_20261022.md))를 반영한 판이다: 필수 경로는 관측 판정(불완전 기록의 설명 집합)·한정 예측·실행 확인이고, 인증서·탐색기·진행성·루프는 선택 경로다(ADR-053). 요약 뷰(`PENDING`·`OPEN` → `INCONCLUSIVE(horizon-open)`, ADR-054), 미관측 자원의 초기 상태 규약 제거(ADR-055), task→생성 앱 매핑 필수(ADR-056), 사다리 재정의(R2 기반 검증 V1′, C1 우선, 사례별 닫힘, ADR-057), 미실행 원 도구의 표현(ADR-058). 요약 spec §0.12, 변경표 [../plan/REV13_CHANGE_TABLE.md](../plan/REV13_CHANGE_TABLE.md). rev 1.2(2026-10-15 결정, spec §0.10–§0.11)와 rev 1.1(외부 검토 EXT-20261012, spec §0.7–§0.8)의 기록도 spec에 남아 있다. 01–10은 rev 1.3을 풀어 쓴다(각 문서 머리의 "rev 1.3 반영" 메모). 그림(`diagrams/`)의 반영 상태는 [diagrams/README.md](diagrams/README.md)에 적는다.

## 읽는 순서

처음이면 01부터 차례로 읽는다. 문제와 구조(01–03)를 먼저 보고, 형식 모형과 cFS 대응(04–05), 관측·확인과 평가(06–07), 구현 단계와 선행 연구(08–09)를 본 뒤, 설계 결정·PI 결정·용어집(10)으로 끝난다.

"읽는 시간"은 각 문서가 첫 문단에 적은 목표다. 02, 04, 05는 목표를 적지 않았고, 필요한 절만 찾아 읽는 참고서로 쓴다.

| 순서 | 문서 | 한 줄 요약 | 읽는 시간 | 그림 |
|---|---|---|---|---|
| 1 | [01_overview.md](01_overview.md) | 문제, 네 가지 예(R2, C1, SB 구독 전 송신, OSAL RESERVED 구간), 기존 기법의 한계, 판정 체계, 기여 가설과 하지 않는 주장 | 15–25분 | D0 |
| 2 | [02_architecture.md](02_architecture.md) | 계층 L0–L5, 구성요소, 두 실행 파일, typed 산출물, 데이터 흐름 12단계, obligation O1–O10과 CC | 적지 않음 | D1, D2 |
| 3 | [03_workflow.md](03_workflow.md) | instance 하나의 분석 순서, feedback loop, 판정 상태 기계, 연구 단계 개요 | 15–25분 | D3, D4 개요, D6, D7 |
| 4 | [04_formal_model.md](04_formal_model.md) | 형식 모형: actor·자원·준비 상태 명세, 세 판단(framework 수준·caller 수준·관측된 결과), 의미론 σ와 모형 수준 M, 유한 실행 위의 safety와 따로 두는 progress, prefix containment, MustBefore·PWB·certificate의 형식 유도와 신뢰 장부(trust ledger), 보장 | 적지 않음 | D10 |
| 5 | [05_cfs_mapping.md](05_cfs_mapping.md) | cFS 계층과 Φ_cFS의 대응, 준비 상태 명세 template K-*와 RC1–RC15, 다섯 사례의 자리와 사례별 기대 판정, R2 certificate와 신뢰 장부, SB 손실의 세 판단, 종료 범위 | 적지 않음 | D5, D8, D9 |
| 6 | [06_observation_confirmation.md](06_observation_confirmation.md) | 수정하지 않은 바이너리의 관측(`kz observe`)과 확인 실행(`kz hold`, `kz gate`), 비용 모형 | 15–25분 | D7 |
| 7 | [07_evaluation.md](07_evaluation.md) | 평가 계획: 연구 질문, 사례별 대상과 대조, 관측 단위 U1–U6, 비교의 세 종류와 세 결과(탐지·분류·근거), 기준선, ablation, recall protocol, triage, perturbation 실험군, 통계 | 15–25분 | D4 상세 |
| 8 | [08_roadmap.md](08_roadmap.md) | 구현 단계 S0–S10, 출구 조건, 단계별 공수 추정([I], rung에는 없음), 검증 사다리 V0–V4(rev 1.3: 첫 rung은 V1′ — ENV basis에서 R2 관측 경로의 기반 검증, C1(V3)을 병행), 다섯 사례(R2, C1, C2, C3, C5)의 경로와 대조, 범위를 줄이는 순서, 위험 | 15–25분 | D4 개요 |
| 9 | [09_prior_work.md](09_prior_work.md) | 선행 연구와의 관계, 빌려 온 것과 새로움, [R04 §5]의 gap G1–G8 | 15–25분 | D0 |
| 10 | [10_decisions_adr.md](10_decisions_adr.md) | 설계 결정 기록(ADR) 59개(ADR-001–ADR-059; 039–044는 rev 1.1, 045–052는 rev 1.2, 053–058은 rev 1.3, 059는 rev 1.4), PI 결정 목록, 이름 충돌 주의, 한국어 용어집 | 15–25분 | D4 개요 |

## 자주 찾는 곳

| 찾는 것 | 위치 |
|---|---|
| 한국어 용어집 | [10_decisions_adr.md §3](10_decisions_adr.md#3-용어집) (원문 spec §13) |
| 이름 충돌 주의(그림 D4와 PI 결정 D4, 보장 G5와 gap G5, 단계 S0와 Init snapshot S0 등) | [10_decisions_adr.md §0.2](10_decisions_adr.md#02-이름-충돌-주의) |
| 설계 결정 기록(ADR) | [10_decisions_adr.md §1](10_decisions_adr.md#1-설계-결정-기록adr) (원문 spec §11) |
| rev 1.4에서 바뀐 것(2026-10-26 결정: 실행 계열 둘, 공통 의미) | 요약 spec §0.13, ADR-059, 결정 기록 [spec/PI_DECISIONS_RESOLVED_20261026.md](spec/PI_DECISIONS_RESOLVED_20261026.md) |
| rev 1.3에서 바뀐 것(2026-10-22 결정 D1–D5) | 요약 spec §0.12, ADR-053–058, 변경표 [../plan/REV13_CHANGE_TABLE.md](../plan/REV13_CHANGE_TABLE.md), 결정 기록 [spec/PI_DECISIONS_RESOLVED_20261022.md](spec/PI_DECISIONS_RESOLVED_20261022.md) |
| rev 1.2에서 바뀐 것(2026-10-15 결정) | 요약 spec §0.10–§0.11, ADR-045–052, 결정 기록 [spec/PI_DECISIONS_RESOLVED_20261015.md](spec/PI_DECISIONS_RESOLVED_20261015.md) |
| rev 1.1에서 바뀐 것(외부 검토 EXT-20261012) | 요약 spec §0.7, 검토 항목별 처리 spec §0.8, 검증 근거와 남은 문제까지 담은 대응 기록 [spec/REVIEW_RESPONSE_20261012.md](spec/REVIEW_RESPONSE_20261012.md), ADR-039–ADR-044([10_decisions_adr.md §1.1](10_decisions_adr.md#11-adr을-읽는-법)), 검토 원문 [history/review_EXT_20261012.md](history/review_EXT_20261012.md) |
| PI가 내려야 할 결정 목록 | [10_decisions_adr.md §2](10_decisions_adr.md#2-pi가-내려야-할-결정) (원문 [spec/PI_DECISIONS.md](spec/PI_DECISIONS.md)) |
| 근거 표기([F], [M], [I], [D], [U])와 인용 형식 | [01_overview.md §0](01_overview.md#0-읽는-법) |
| 그림 목록, 범례, 다시 렌더링하는 법 | [diagrams/README.md](diagrams/README.md) |

## 폴더 구성

| 경로 | 내용 |
|---|---|
| `01_*.md`–`10_*.md` | 한국어 설명 문서. spec에 없는 설계 요소·수치·인용은 더하지 않는다 |
| [spec/](spec/) | 규범 원문 [SPEC_FINAL.md](spec/SPEC_FINAL.md) rev 1.4(요약 §0.13, ADR-059; rev 1.3 §0.12, ADR-053–058; rev 1.2 §0.10–§0.11, rev 1.1 §0.7–§0.8), PI 결정 기록 [PI_DECISIONS_RESOLVED_20261015.md](spec/PI_DECISIONS_RESOLVED_20261015.md)·[PI_DECISIONS_RESOLVED_20261022.md](spec/PI_DECISIONS_RESOLVED_20261022.md)·[PI_DECISIONS_RESOLVED_20261026.md](spec/PI_DECISIONS_RESOLVED_20261026.md), 검토 [X1]–[X3]의 쟁점 57개의 처리 [ISSUE_RESOLUTION.md](spec/ISSUE_RESOLUTION.md)(끝 절 EXT-20261012에 외부 검토 항목별 요약 표가 있다), 외부 검토 EXT-20261012 대응 기록 [REVIEW_RESPONSE_20261012.md](spec/REVIEW_RESPONSE_20261012.md)(영문: 항목마다 검토의 지적, 우리 검증 판정과 근거, 바뀐 파일과 절, 미룬 것과 이유; 사례별 범위, PI가 다시 볼 수 있는 설계 선택, 과정, 최종 점검 뒤 남은 문제. 규범 처리 표는 spec §0.8이다), PI 결정 원문 [PI_DECISIONS.md](spec/PI_DECISIONS.md) |
| [diagrams/](diagrams/README.md) | 그림 D0–D10의 Mermaid 원본(`.mmd`)과 렌더링(`.svg`) |
| [background/](background/README.md) | 배경 조사 보고서 [R01]–[R07] (영문), 용어 검토 [08_terminology.md](background/08_terminology.md)(영문, '계약(contract)'이라는 이름의 타당성; 이름 변경은 PI 결정 T1로 미결이라 "계약"을 유지한다)와 그 하위 조사 [terminology/](background/terminology/) T1–T4. 외부 검토 EXT-20261012 §6.1–§6.3 용어 항목의 처리 기록이다(spec §0.8) |
| [history/](history/) | 설계 이력: 설계안 [A]/[B]/[C], 심사 [J1]–[J3], 검토 [X1]–[X3], SPEC v1(영문), 외부 검토 EXT-20261012 [review_EXT_20261012.md](history/review_EXT_20261012.md)(한국어, commit `b4ac55f` 대상; spec rev 1.1이 답했다) |

어떤 문서도 "경보가 없으면 안전하다"고 말하지 않는다. 판정 기록에는 "safe" 칸이 없고, 어떤 progress 값도 "live"나 "deadlock-free"를 뜻하지 않으며, 근거가 없는 칸은 `unknown(reason)`이 된다(spec §0.1, §1.10).

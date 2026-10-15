# Kairos 아키텍처 다이어그램

Kairos 설계(규범 원문: [`../spec/SPEC_FINAL.md`](../spec/SPEC_FINAL.md), rev 1.1)를 그림으로 옮긴 폴더다. 각 그림은 spec §12 "Diagrams to draw"의 D1–D10 목록을 따르고, 한 화면 개요 D0을 더했다. 그림 원본은 Mermaid 텍스트(`.mmd`)이고, 같은 이름의 `.svg`는 그 원본을 렌더링한 결과다. Mermaid는 텍스트로 쓴 도표를 그림으로 바꾸는 도구이며, GitHub는 Markdown 안의 ` ```mermaid ` 블록을 직접 그린다.

상태: 설계 문서다. 구현·실행·측정한 것은 없다. 그림 안의 비용 값 중 `[M]` 표시는 spec이 인용한 기존 측정값([R07 §4.7])이고, 나머지 수치와 줄 번호는 spec의 코드 인용이다. 그림은 spec에 없는 설계 요소를 더하지 않는다.

## 읽는 법 (범례)

| 모양·선 | 뜻 |
|---|---|
| 사각형 | 구성요소(`kz`, `kz-static`의 하위 명령) 또는 처리 단계 |
| 원통 | 산출물(KZ container 파일, 예: `basis.kz`, `run-*.kzt`) 또는 저장소 |
| 마름모 | 판단 지점 |
| 둥근 상자 | 최종 판정(class), reach·progress 값 또는 결과 label |
| 점선 테두리 | PI 결정이 있어야 쓰는 구성요소, 조건부 단계(S3b), 늦은 단계(S7 explorer). D4 개요에서는 기반이나 arm이 PI 결정 뒤에만 생기는 rung과 track(V2, V3, VP-D3, VP-D4, VP-D5)도 점선 테두리다 |
| 점선 화살표 | feedback 경로(F1–F4)나 PI 결정이 단계·rung·track을 여는 관계, 조건부 경로(S3b, D4의 6.4.x track → S10, D3 상세 1의 progress candidate → explorer). 예외: D9에서는 spec 표기 "⇢"(disabler 관계, SCOPE로 제외)를 점선으로 그렸다 |
| `--x` 화살표 (D8) | 경로가 없음 또는 writer 집합에서 제외됨. 화살표 위 글자가 그것을 증명하는 규칙이다 |
| 빨간 상자 | `unknown(reason)`, `contested`, obligation 실패, `DECLARED_TRUSTED`(검증하지 못하고 신뢰한) 항목 |

어떤 그림에도 "safe" 상태는 없고, "live"나 "deadlock-free"를 뜻하는 값도 없다. 경보가 없다는 것은 근거가 없다는 뜻이고, 그런 칸은 `unknown(reason)`이 된다(spec §0.1, §1.10). class는 safety 분류이고, 영구 대기와 교착은 class가 아니라 칸별 progress 결과다(spec §1.8, §1.10).

## 그림 목록

"포함 문서" 열은 그 그림의 Mermaid 원본을 실제로 실은 문서다. 괄호 안은 그림을 싣지 않고 `.svg`만 링크한 문서다. 경로는 이 폴더 기준 상대 경로다. 문서 전체의 읽는 순서는 [`../README.md`](../README.md)에 있다.

| ID | 파일 (원본 / 렌더링) | 보여 주는 것 | spec 절 | 포함 문서 |
|---|---|---|---|---|
| D0 | [`D0_big_picture.mmd`](D0_big_picture.mmd) / [`.svg`](D0_big_picture.svg) | 한 화면 개요: 입력(basis B, profile Φ) → 정적 closure와 skeleton → formal core → 세 해석기(abstract, concrete, operational) → 수정하지 않은 gcc 바이너리에서 관측·확인 → 판정, feedback F1–F4 | §0.1–0.2, §2.1, §2.4, §4.14 | [`../01_overview.md`](../01_overview.md), [`../09_prior_work.md`](../09_prior_work.md) |
| D1 | [`D1_system_context.mmd`](D1_system_context.mmd) / [`.svg`](D1_system_context.svg) | 시스템 문맥: PI, `kz`/`kz-static`, 고정된 cFS tree와 build tree, LLVM 23, clang-18, Linux kernel, 실행 harness, cFS 프로세스, 유지 상태, 산출물 저장소. PI 결정 전 도구(Z3, libbpf, Nidhugg, `--wrap`, MLIR, TSan)는 점선 | §12 D1; §2.2, §2.6, §5.1, §10.1 | [`../02_architecture.md`](../02_architecture.md) |
| D2 개요 | [`D2_components_overview.mmd`](D2_components_overview.mmd) / [`.svg`](D2_components_overview.svg) | 계층 L0–L5별 구성요소와 주요 산출물 흐름. `kz proofcheck`는 신뢰 기반(facts, skeleton, closure 목록, Φ, `libkz-core`, run)에 대해 추론 단계를 다시 검증하고 trust ledger를 census에 돌려준다 | §2.1–2.4, §4.5.1 | [`../02_architecture.md`](../02_architecture.md) |
| D2 상세 1 | [`D2_components_static_detail.mmd`](D2_components_static_detail.mmd) / [`.svg`](D2_components_static_detail.svg) | `kz-static` 흐름: basis → extract/link(/modref) → skel → static → census ↔ `kz proofcheck` → plan. `kz proofcheck`로 가는 신뢰 입력(facts, skel, anchors, profile은 "trusted" edge, run 쪽 산출물은 입력 상자)과, 되돌아가는 accept/reject(CC)와 trust ledger | §12 D2; §4.1–4.7, §4.5.1 | [`../02_architecture.md`](../02_architecture.md) |
| D2 상세 2 | [`D2_components_dynamic_detail.mmd`](D2_components_dynamic_detail.mmd) / [`.svg`](D2_components_dynamic_detail.svg) | 관측 이후: observe → trace/conform/predict/explore → witness → hold/gate → verdicts → report, `kz loop`, feedback F1–F4와 obligation O1–O9 표시 | §12 D2; §2.5, §4.8–4.14 | [`../02_architecture.md`](../02_architecture.md) |
| D3 개요 | [`D3_analysis_overview.mmd`](D3_analysis_overview.mmd) / [`.svg`](D3_analysis_overview.svg) | 계약 인스턴스 하나의 분석 순서 7단계: 정적 census → 관측·routing → 예측 → explorer → 확인 → 칸별 결합 → σ_E 전이 | §4.6, §4.8–4.13 | [`../03_workflow.md`](../03_workflow.md) |
| D3 상세 1 | [`D3_analysis_static.mmd`](D3_analysis_static.mmd) / [`.svg`](D3_analysis_static.svg) | census 판단: 도달 가능성, key, writer 집합 완전성, provider 축, FIXED_ORDER·PROTECTED certificate, `kz proofcheck`(신뢰 기반에 대한 재검증, trust ledger), REJECT 위치에 따른 fixed-order와 `unknown(no_decision_route)`, protected(도달한 in-scope occurrence가 모두 READY; reach가 assumed면 "도달하면"), certificate의 progress `unknown(not_checked)`, progress candidate → explorer | §12 D3; §4.2, §4.5, §4.5.1, §4.6 | [`../03_workflow.md`](../03_workflow.md) |
| D3 상세 2 | [`D3_analysis_dynamic.mmd`](D3_analysis_dynamic.mmd) / [`.svg`](D3_analysis_dynamic.svg) | open 후보와 progress candidate 이후: routing(conforming / wrong-way / O-ordered / unordered), 예측(predicted not-ready evidence 포함), explorer 네 질의(Q_reach, Q_bad, Q_ok, Q_stall), 확인, 칸·bound별 결합. 출력은 따로 셋이다: safety record에서 나오는 class, reach, progress(stall-witness, `no-stall-within(b)`, `unknown(…)`, timeout-masked-liveness, progress chain의 root). stall은 class로 가지 않는다. 전이 규칙 1(a)(받는 칸에 safety 진술), 규칙 2(σ_E, b = ∞), `unknown(model_level)`, contested bound(O6(b), O10), portability-hazard-evidence, obligation 실패 | §12 D3; §1.8, §1.10, §4.8–4.14 | [`../03_workflow.md`](../03_workflow.md) |
| D4 개요 | [`D4_research_overview.mmd`](D4_research_overview.mmd) / [`.svg`](D4_research_overview.svg) | 검증 사다리(V0, V0-P, V1과 sub-gate, V1b, V2, V3, V4a, V4b; 첫 목표는 thin MVC 전체가 아니라 rung V1), 사례 track(다섯 사례의 paper track, VP-D3, VP-D4, VP-D5), 단계 S0–S10(thin MVC = S0+S1+S2+S3a, 그 전체 gate가 V1b), S3b 조건부, freeze F1·F2와 exposure·freeze 기록, 평가 요약, rung·track·단계를 여는 PI 결정(P-LADDER, D20, D1/D17, D14, P-R2CTRL, D22/P-C1HOLD, D13, P-C2, P-RGIT, P-D18GATES/D15, D16, P-F1GATE, P-RECALL/P-TRIAGE/P-STATS/P-CLASSGT, D3/D4, D6). V2와 V3 사이에는 edge가 없다 | §12 D4; §9.3, §9.4, §10.1; ADR-044 | [`../03_workflow.md`](../03_workflow.md), [`../08_roadmap.md`](../08_roadmap.md), [`../10_decisions_adr.md`](../10_decisions_adr.md) |
| D4 상세 | [`D4_research_evaluation.mmd`](D4_research_evaluation.mmd) / [`.svg`](D4_research_evaluation.svg) | 평가 경로: freeze F1 → template-only 실행 → 표시된 refinement → freeze F2 → held-out (a)–(e)와 recall, triage, ablation, 기준선, perturbation 실험군, normalization boot, 통계, claims table, deviation 기록 | §12 D4; §7.2, §7.5, §8.3–8.7 | [`../07_evaluation.md`](../07_evaluation.md) ([`../08_roadmap.md`](../08_roadmap.md)) |
| D5 개요 | [`D5_cfs_mapping_overview.mmd`](D5_cfs_mapping_overview.mmd) / [`.svg`](D5_cfs_mapping_overview.svg) | cFS 계층(PSP, OSAL, ES, EVS, SB, TBL, TIME, FS/CDS, 앱)마다 Φ_cFS의 어떤 kind·규칙·계약이 되는지 | §12 D5; §1.3, §3.1–3.16 | [`../05_cfs_mapping.md`](../05_cfs_mapping.md) |
| D5 상세 | [`D5_cfs_mapping_detail.mmd`](D5_cfs_mapping_detail.mmd) / [`.svg`](D5_cfs_mapping_detail.svg) | 코드 위치 예와 Φ 항목: `cfe_es_start.c:835` → R19, `:803/:818/:823` → `es.app`과 할당 기록 ρ, `cfe_sb_task.c:138` → `cell(CFE_SB_Global.AppId)`와 capture point, `OS_QueuePut` → K-CAP, MsgLim → K-CAP.msglim, `osapi-idmap.c:384–385` 대 `:401–404` → K-LOCKBAL, `cfe_psp_support.c:87` → `es.shutdown` | §12 D5; §3.2–3.9 | [`../05_cfs_mapping.md`](../05_cfs_mapping.md) |
| D6 개요 | [`D6_verdict_overview.mmd`](D6_verdict_overview.mmd) / [`.svg`](D6_verdict_overview.svg) | (σ, M) 칸과 bound 하나의 class 영역 상태 기계: candidate, open, inactive, protected[cert]·[model b], tolerated[model b], fixed-order[cert]·[model b], no-final-status, ordinal-split, schedule-dependent와 refinement timeout-dependent, `unknown(no_decision_route)`, contested, unknown 이유, model-fault 기록, 불변식 | §12 D6; §1.10, §4.12, §4.14 | [`../03_workflow.md`](../03_workflow.md) ([`../01_overview.md`](../01_overview.md), [`../04_formal_model.md`](../04_formal_model.md)) |
| D6 상세 | [`D6_verdict_axes.mmd`](D6_verdict_axes.mmd) / [`.svg`](D6_verdict_axes.svg) | 판정 칸의 병렬 영역: evidence realization, naturalness, provider, reach(assumed → proved, model(b), unreached(b) → contested), scope, progress(`unknown(not_checked)` → stall-witness, `no-stall-within(b)`, `unknown(…)`, contested), trust ledger 네 부분 | §12 D6; §1.8, §1.10, §4.5.1, §4.12–4.14 | [`../03_workflow.md`](../03_workflow.md) ([`../01_overview.md`](../01_overview.md)) |
| D7 | [`D7_observation_pipeline.mmd`](D7_observation_pipeline.mmd) / [`.svg`](D7_observation_pipeline.svg) | 관측 파이프라인: `plan.kz` → loader → inode 기준 uprobe 등록과 write watchpoint → BPF 프로그램(attach cookie로 site descriptor 조회, `tseq`/`objseq`) → ring buffer → drainer → `run-*.kzt`; `kz hold`의 stop/continue; 비정상 종료 뒤 normalization boot | §12 D7; §5.1–5.7, §4.13 | [`../03_workflow.md`](../03_workflow.md), [`../06_observation_confirmation.md`](../06_observation_confirmation.md) ([`../01_overview.md`](../01_overview.md)) |
| D8 개요 | [`D8_r2_certificate_overview.mmd`](D8_r2_certificate_overview.mmd) / [`.svg`](D8_r2_certificate_overview.svg) | R2 FIXED_ORDER certificate의 ES·EVS 두 절반: capture point `c`, frontier 노드 `m`, raise, falsifier `f`와 할당 기록 ρ, 관측 `o`(`:835`), panic 분기(T-TERM), SB spawn → SB 진입 → writer `x`(`cfe_sb_task.c:138`), INIT, `derives` | §12 D8; App. A.1; §4.4, §4.5 | [`../05_cfs_mapping.md`](../05_cfs_mapping.md) ([`../01_overview.md`](../01_overview.md), [`../04_formal_model.md`](../04_formal_model.md)) |
| D8 상세 | [`D8_r2_certificate_writers.mmd`](D8_r2_certificate_writers.mmd) / [`.svg`](D8_r2_certificate_writers.svg) | R2 certificate의 writer 목록 상자: V의 writer, `W_o(o_ES)`, `W_o(o_EVS)`, 각 항목의 처리 규칙과 transition·key status, 신뢰 항목(A-FRESH, A-OWNER, CleanUpApp guard) | App. A.1 steps 3–5, 9 | [`../05_cfs_mapping.md`](../05_cfs_mapping.md) |
| D9 | [`D9_teardown_scoping.mmd`](D9_teardown_scoping.mmd) / [`.svg`](D9_teardown_scoping.svg) | 종료 범위: 요청자 actor(`CFE_PSP_Restart` → `ShutdownReq` 저장 → 10 s 지연 → `abort`)와 PSP main thread(`OS_Application_Run` → `OS_DeleteAllObjects`), SR ∈ MB, in-scope 사용과 K-TEARDOWN, 다음 실행의 B.Init과 normalization boot | §12 D9; §3.5, §1.6, §4.5, §4.13 | [`../05_cfs_mapping.md`](../05_cfs_mapping.md) |
| D10 | [`D10_skeleton_views.mmd`](D10_skeleton_views.mmd) / [`.svg`](D10_skeleton_views.svg) | 같은 actor의 두 skeleton: certificate view `Sk^cert`(loop를 star-closure로 닫음)와 exploration view `Sk^h`(h = 1, 2로 펼치고 나머지는 TRUNC), 각 view를 쓰는 소비자, `kz proofcheck`의 view 검사, litmus L-H2 | §12 D10; §4.3, §4.5, App. A.8 | [`../04_formal_model.md`](../04_formal_model.md) ([`../01_overview.md`](../01_overview.md)) |

### 나눈 그림

§12의 노드가 한 그림에 넣기에 많은 D2, D3, D4, D5, D6, D8은 개요(`*_overview.mmd`)와 상세로 나눴다. 개요는 흐름만 보이고, 상세는 §12가 요구한 노드와 edge를 모두 담는다. D2와 D3은 상세가 두 장(정적 쪽, 동적 쪽)이다. D4는 개요가 검증 사다리·사례 track·단계·PI 결정이고, 상세가 평가 경로다. D6은 개요가 class 영역의 상태 기계이고, 상세가 병렬 영역(reach, progress, trust ledger 포함)이다. D8은 개요가 certificate의 순서 증명이고, 상세가 §12가 요구한 "writer 목록 상자"다.

### 용어 몇 가지 (그림 이해에 필요한 만큼)

- **certificate**: 판정이 맞다는 것을 다른 프로그램(`kz proofcheck`)이 다시 확인할 수 있게 적은 증명 기록이다(spec §4.5).
- **MustBefore (MB)**: "모든 실행에서 a가 n보다 먼저 일어난다"는 정적 관계다(§4.4). **PWB**(precedes-whenever-both-occur)는 "둘 다 일어나는 실행에서는 항상 a가 먼저"라는 관계다.
- **linearization point (LP)**: 연산의 효과가 일어났다고 보는 한 지점이다. Kairos는 요구 조건의 각 conjunct마다 LP를 둔다(§1.4). **capture point**는 호출자가 값을 미리 읽어 둔 load 지점이다.
- **happens-before (HB)**: 한 실행 안에서 동기화로 정해지는 사건 사이의 순서다. vector clock은 그것을 계산하는 자료 구조다(§1.9).
- **uprobe / eBPF**: 실행 파일을 바꾸지 않고 사용자 프로그램의 명령 주소에 kernel probe를 걸고, 그 자리에서 작은 kernel 프로그램(eBPF)을 실행하는 Linux 기능이다(§5.1, §5.3).
- **σ_U, σ_U∞, σ_R1, σ_E**: 스케줄 의미론이다. 각각 시간을 무시한 임의 interleaving, timeout 없는 인내 의미, 한 CPU의 고정 우선순위, 실제 환경이다(§1.8). **M_api / M_impl**은 OSAL을 API 계약 수준과 POSIX 구현 수준 중 어디까지 모델링하는지다(§1.4).
- **safety record와 class**: class는 유한 실행 위의 safety 진술(`¬Bad_i`, `¬OK_i`, PROTECTED certificate의 `¬NR_i`)과 witness에서 정해지는 분류다. 사용 지점에 도달하는지는 class가 아니라 reach 축에 적는다. reach가 `assumed`면 class는 "도달하면 …"으로 읽는다(§1.10).
- **progress 결과**: 영구 대기와 교착은 칸별 progress 결과다(`stall-witness`, `no-stall-within(b)`, `unknown(reason)`). 기본값은 `unknown(not_checked)`다. 모델 위 값은 `exh`만 정하고, certificate는 progress를 정하지 않는다(§1.8, §4.12 항목 9).
- **prefix 포함**: σ 사이의 포함은 유한 실행(prefix)끼리만 주장하고, 최대 실행의 포함은 주장하지 않는다(§1.8). 그래서 σ 사이로 옮기는 것은 safety 진술(아래로)과 witness(위로)이고, reach와 progress는 σ마다 따로 계산한다.
- **trust ledger**: certificate와 판정 칸마다 붙는 네 부분 기록이다: 다시 검증한 추론 단계, 추출한 사실, 신뢰한 항목, 반박 가능하지만 증명되지 않은 항목. `kz proofcheck`의 독립성은 추론의 독립성이고, 신뢰 기반(추출한 사실, skeleton, Φ, `libkz-core`)에 대해서만 성립한다(§4.5, §4.5.1).
- **검증 사다리**: 첫 목표는 thin MVC 전체가 아니라 rung V1(ENV basis에서 R2의 K-ID 사슬 전체)이다. thin MVC의 전체 gate는 V1b다(§9.3, ADR-044).

## spec rev 1.1 반영 상태

rev 1.1에서 spec §12가 바뀐 그림은 D2, D3, D4, D6, D8, D9다. D1, D5, D7, D10의 §12 항목은 바뀌지 않았다.

| 그림 | 상태 |
|---|---|
| D2 개요, D2 상세 1 | rev 1.1 반영(`kz proofcheck`의 신뢰 입력과 trust ledger) |
| D3 상세 1, D3 상세 2 | rev 1.1 반영 |
| D4 개요 | rev 1.1 반영 |
| D6 개요, D6 상세 | rev 1.1 반영 |
| D3 개요 | 아직 rev 1.1 이전 그림이다. explorer를 "두 질의"로 적고, progress 출력이 없다 |
| D4 상세 | 아직 rev 1.1 이전 그림이다. §12 D4의 comparator(detection / classification / evidence), exposure·freeze 기록, masked rater가 없다 |
| D8 상세 | 아직 rev 1.1 이전 그림이다. §12 D8의 trust-ledger 상자(App. A.1 step 9a의 네 부분)가 없다 |
| D9 | 아직 rev 1.1 이전 그림이다. §12 D9는 "uses after SR"를 "occurrences decided after SR"로 바꿨다 |
| D2 상세 2 | 바꿀 것이 없다. §12 D2의 바뀐 부분(신뢰 입력, trust ledger)은 D2 상세 1에 그렸다 |
| D8 개요 | 바꿀 것이 없다. §12 D8의 바뀐 부분(trust-ledger 상자)은 D8 상세 몫이다 |

문서에 복사한 Mermaid 블록은 각 문서가 `.mmd`를 다시 복사할 때까지 이전 내용이다.

## 다시 렌더링하는 법

필요한 것: Node.js(`npx`), 네트워크(처음 한 번 패키지 내려받기), Chromium. 이 머신에서는 미리 설치된 Chromium을 [`.puppeteer.json`](.puppeteer.json)으로 지정한다.

```json
{"executablePath":"/opt/pw-browsers/chromium-1194/chrome-linux/chrome","args":["--no-sandbox"]}
```

그림 하나(예: D0):

```sh
npx -y @mermaid-js/mermaid-cli@11.17.0 \
  -p /home/user/Shape-Perf/research/cfs_procrace/Architecture/diagrams/.puppeteer.json \
  -i /home/user/Shape-Perf/research/cfs_procrace/Architecture/diagrams/D0_big_picture.mmd \
  -o /home/user/Shape-Perf/research/cfs_procrace/Architecture/diagrams/D0_big_picture.svg -b white
```

전부:

```sh
cd /home/user/Shape-Perf/research/cfs_procrace/Architecture/diagrams
for f in *.mmd; do
  npx -y @mermaid-js/mermaid-cli@11.17.0 -p .puppeteer.json -i "$f" -o "${f%.mmd}.svg" -b white
done
```

다른 머신에서는 `.puppeteer.json`의 `executablePath`를 그 머신의 Chromium 경로로 바꾼다. Mermaid CLI 버전은 11.17.0으로 고정한다.

## 작성 규칙 (그림을 고칠 때)

- 원본은 `.mmd`다. `.svg`만 고치지 않는다. `.mmd`를 고치면 위 명령으로 `.svg`를 다시 만든다.
- 문서에 넣을 때는 `.mmd` 내용을 ` ```mermaid ` 블록에 그대로 복사한다. 블록 바로 앞에 한 줄 caption을 둔다: `**그림 Dn(개요/상세). 제목** (렌더링: diagrams/….svg; spec §…)`.
- **label 언어.** 그림 안의 label은 모두 영어로 쓴다(코드 이름, spec 용어와 같은 말을 쓰기 위해서다). 한국어 설명은 문서의 caption과 본문이 맡는다. 같은 개념은 그림마다 같은 label을 쓴다(예: feedback F4의 도착점은 언제나 "Φ revision (human review)").
- GitHub에서 그려지도록 Mermaid 10에 있던 문법만 쓴다: `flowchart`(subgraph, `direction`, `-.->`, `--x`, `<-->`, `classDef`), `stateDiagram-v2`(복합 상태, `--` 병렬 영역, note). 공백·괄호·기호·한글이 든 label은 큰따옴표로 감싼다. HTML은 `<br/>`만 쓴다. 꺾쇠와 `&`는 Mermaid entity code `#60;`, `#62;`, `#38;`로 쓴다(예: `osal.obj#60;T#62;`).
- 배치는 Mermaid가 자동으로 정한다. D1, D3 상세 2, D4 개요, D6 개요·상세처럼 넓은 그림은 GitHub 미리보기보다 `.svg`를 따로 열어 확대해 보는 편이 읽기 쉽다.
- 그림 내용은 spec을 따른다. spec이 바뀌면 그림을 고치고 이 표의 spec 절을 확인한다.

# Kairos 아키텍처 다이어그램

Kairos 설계(규범 원문: [`../spec/SPEC_FINAL.md`](../spec/SPEC_FINAL.md), rev 1.3)를 그림으로 옮긴 폴더다. 각 그림은 spec §12 "Diagrams to draw"의 D1–D10 목록을 따르고, 한 화면 개요 D0을 더했다. 그림 원본은 Mermaid 텍스트(`.mmd`)이고, 같은 이름의 `.svg`는 그 원본을 렌더링한 결과다. Mermaid는 텍스트로 쓴 도표를 그림으로 바꾸는 도구이며, GitHub는 Markdown 안의 ` ```mermaid ` 블록을 직접 그린다.

상태: 설계 문서다. 구현·실행·측정한 것은 없다. 그림은 spec rev 1.3(2026-10-22; spec §0.12, 변경표 `../../plan/REV13_CHANGE_TABLE.md`)에 맞췄고, 2026-10-22에 다시 렌더링했다(아래 "spec rev 1.3 반영"). 그림 안의 비용 값 중 `[M]` 표시는 spec이 인용한 기존 측정값([R07 §4.7])이고, 나머지 수치와 줄 번호는 spec의 코드 인용이다. 그림은 spec에 없는 설계 요소를 더하지 않는다.

## 읽는 법 (범례)

| 모양·선 | 뜻 |
|---|---|
| 사각형 | 구성요소(`kz`, `kz-static`의 하위 명령) 또는 처리 단계 |
| 원통 | 산출물(KZ container 파일, 예: `basis.kz`, `run-*.kzt`) 또는 저장소 |
| 마름모 | 판단 지점 |
| 둥근 상자 | 최종 판정(class), reach·progress 값 또는 결과 label |
| 점선 테두리 | rev 1.3의 선택 경로(ADR-053: `skel`, `static`, `kz proofcheck`, `explore`, `loop`, `gate`; D3의 certificate 갈래와 explorer; D4의 V1 certificate 출구 항목과 S7; D8의 주석 상자), PI 결정이 있어야 쓰는 구성요소, 조건부 단계(S3b), 늦은 단계(S7 explorer). D4 개요에서는 기반이나 arm이 PI 결정 뒤에만 생기는 rung과 track(V2, V3, VP-D3, VP-D4, VP-D5)도 점선 테두리다. D9에서는 다음 실행(B.Init) 묶음이다 |
| 점선 화살표 | rev 1.3의 선택 경로로 들어가고 나오는 흐름(D0, D2, D3, D4), feedback 경로(F1–F4)와 D3의 loop 경로(suspension → 새 Φ hash의 새 loop), PI 결정이 단계·rung·track을 여는 관계, 조건부 경로(S3b, D4의 6.4.x track → S10, D3 개요·상세의 progress candidate → S7 explorer), D4 상세의 기록 경로(exposure·freeze 기록 → F1, F2; deviation 기록으로 가는 edge). D7의 화살표 없는 점선은 다른 backend(perf-sample, tracefs)다. 예외: D9에서는 spec 표기 "⇢"(disabler 관계, SCOPE로 제외)를 점선으로 그렸다 |
| `--x` 화살표 (D8) | 경로가 없음 또는 writer 집합에서 제외됨. 화살표 위 글자가 그것을 증명하는 규칙이다 |
| 빨간 상자 | `unknown(reason)`, 요약 뷰의 `INCONCLUSIVE(reason)`과 분석 입력 오류(D3), `contested`, obligation 실패, portability-hazard-evidence(D3), `DECLARED_TRUSTED`(검증하지 못하고 신뢰한) 항목(D8 상세). 그 밖에 경로가 끝나거나 따로 보고되는 곳도 빨간색이다: D8 개요의 panic 분기, D9의 요청자 `abort()`와 K-TEARDOWN 사용, D10의 `TRUNC`와 proofcheck reject |

실선 상자와 실선 화살표는 rev 1.3의 필수 경로(basis·extract·link → plan → observe → trace → predict → hold → report)다(spec §2.2 "Required and optional routes"). 어떤 그림에도 "safe" 상태는 없고, "live"나 "deadlock-free"를 뜻하는 값도 없다. 경보가 없다는 것은 근거가 없다는 뜻이고, 그런 칸은 `unknown(reason)`이 된다(spec §0.1, §1.10). class는 safety 분류이고, 영구 대기와 교착은 class가 아니라 칸별 progress 결과다(spec §1.8, §1.10).

## 그림 목록

"포함 문서" 열은 그 그림의 Mermaid 원본을 실제로 실은 문서다. 괄호 안은 그림을 싣지 않고 `.svg`만 링크한 문서다. 경로는 이 폴더 기준 상대 경로다. 문서 전체의 읽는 순서는 [`../README.md`](../README.md)에 있다.

| ID | 파일 (원본 / 렌더링) | 보여 주는 것 | spec 절 | 포함 문서 |
|---|---|---|---|---|
| D0 | [`D0_big_picture.mmd`](D0_big_picture.mmd) / [`.svg`](D0_big_picture.svg) | 한 화면 개요: 입력(basis B, profile Φ) → 정적 closure(basis, extract, link) → probe plan(손 보강 가능) → 수정하지 않은 gcc 바이너리 관측 → 관측 적합성 기록 → concrete 해석(정규화, task→앱 매핑 `app_owner`, 설명 집합 위의 관측 기반 판정, 명시적 상태 탐색이 먼저인 예측) → hold → 판정(칸마다 class 또는 `unknown(reason)`, reach, progress, 증명 상태) → 요약 뷰로 시작하는 report. `skel`, abstract 해석(census, MustBefore, certificate → `kz proofcheck`), operational 해석(explorer), loop, gate는 점선의 선택 경로. feedback F1–F4 | §0.1–0.2, §0.12, §2.1, §2.2, §2.4, §4.14 | [`../01_overview.md`](../01_overview.md), [`../09_prior_work.md`](../09_prior_work.md) |
| D1 | [`D1_system_context.mmd`](D1_system_context.mmd) / [`.svg`](D1_system_context.svg) | 시스템 문맥: PI, `kz`/`kz-static`, 고정된 cFS tree와 build tree, LLVM 23, clang-18, 실행마다 기록하는 Linux kernel(`-fc-v114` in [R07 §1], 2026-10-16에 `-fc-v139`; spec §1.7 Env), 실행 harness(`RUN_CFS_WRAP`은 gdb만), cFS 프로세스, 유지 상태, 산출물 저장소. 후순위·조건부 도구(Z3, libbpf, Nidhugg, `--wrap`)는 점선. MLIR, TSan 변형, valgrind 기준점은 rev 1.2에서 빠졌다 | §12 D1; §1.7, §2.2, §2.6, §5.1, §10.1 | [`../02_architecture.md`](../02_architecture.md) |
| D2 개요 | [`D2_components_overview.mmd`](D2_components_overview.mmd) / [`.svg`](D2_components_overview.svg) | 계층 L0–L5별 구성요소와 주요 산출물 흐름. 필수 경로는 실선(basis·extract·link → plan → observe → 관측 적합성 기록 → trace·predict → hold → report의 요약 뷰), 선택 경로(`skel`, `static`, `kz proofcheck`, `explore`, `gate`, `loop`)와 `modref`는 점선. `plan`은 `static`과 따로 있다 | §12 D2 (rev 1.3, C-48); §2.1–2.4, §4.5.1 | [`../02_architecture.md`](../02_architecture.md) |
| D2 상세 1 | [`D2_components_static_detail.mmd`](D2_components_static_detail.mmd) / [`.svg`](D2_components_static_detail.svg) | `kz-static` 흐름: basis → extract/link(/modref) → plan(실선: `facts.kz`, `anchors.kz`; 손 보강 계획 `sites.kzp`, §4.7). 선택 경로 skel → static → census ↔ `kz proofcheck`(증명 상태가 붙은 trust ledger, "trusted" 입력)와 census의 open 후보 → plan은 점선 | §12 D2 (rev 1.3); §4.1–4.7, §4.5.1 | [`../02_architecture.md`](../02_architecture.md) |
| D2 상세 2 | [`D2_components_dynamic_detail.mmd`](D2_components_dynamic_detail.mmd) / [`.svg`](D2_components_dynamic_detail.svg) | 관측 이후: observe → 관측 적합성 기록 → trace(`app_owner`, 설명 집합) → conform/predict(명시적 상태 탐색 먼저) → witness → hold → verdicts → report(요약 뷰 먼저). 선택 경로 explore, gate, loop와 conform의 O8은 점선. feedback F1–F4와 obligation 표시 | §12 D2 (rev 1.3); §2.5, §4.8–4.14 | [`../02_architecture.md`](../02_architecture.md) |
| D3 개요 | [`D3_analysis_overview.mmd`](D3_analysis_overview.mmd) / [`.svg`](D3_analysis_overview.svg) | 준비 상태 명세 instance 하나와 (σ, M) 칸의 분석 순서. 필수 경로: 관측 → 관측 기반 판정(불완전한 기록의 설명 집합 `Cobs`, `Qv`/`Qn`) → 요약 뷰 끝 노드 `OBSERVED_VIOLATION`, `NO_VIOLATION_IN_CHECKED_PREFIX`, `INCONCLUSIVE(reason)`(`PENDING`·`OPEN`은 `horizon-open`), `NOT_APPLICABLE`과 분석 입력 오류; 예측 → `MODEL_CANDIDATE` / `NONE_IN_SCOPE` / `INCONCLUSIVE` → 확인 → `CONFIRMED` / `NOT_CONFIRMED`. 선택 경로(점선): 정적 census의 certificate 갈래와 explorer(네 질의). 칸별 결합, 전이(safety 진술만), 출력 셋(class, reach, progress), obligation 실패, contested bound, portability-hazard-evidence | §12 D3 (rev 1.3, C-48); §1.8, §1.10, §2.2, §2.5, §4.6, §4.8–4.14 | [`../03_workflow.md`](../03_workflow.md) |
| D3 상세 1 | [`D3_analysis_static.mmd`](D3_analysis_static.mmd) / [`.svg`](D3_analysis_static.svg) | census 판단: 도달 가능성, key, writer 집합 완전성, provider 축, 그리고 "보편 순서 주장이 필요한가": 아니면 certificate를 기다리지 않고 관측 기반 판정으로(§2.4). 점선 상자의 선택 경로: FIXED_ORDER·PROTECTED certificate, `kz proofcheck`, 증명 상태가 붙은 trust ledger, REJECT 위치에 따른 fixed-order와 `unknown(no_decision_route)`, protected(결정된 in-scope 발생이 모두 READY 또는 NOT_EVALUATED), progress candidate → explorer | §12 D3 (rev 1.3); §2.4, §4.2, §4.5, §4.5.1, §4.6 | [`../03_workflow.md`](../03_workflow.md) |
| D3 상세 2 | [`D3_analysis_dynamic.mmd`](D3_analysis_dynamic.mmd) / [`.svg`](D3_analysis_dynamic.svg) | 필수 경로의 instance, open 후보, progress 후보 이후: 관측 기반 판정과 요약 뷰 세 묶음(관측, 예측, 확인)의 끝 노드, routing(wrong-way의 certificate 요청은 선택 경로, 없으면 "schedule dependence undetermined"), 예측, explorer 네 질의(선택 경로), 확인, 칸·bound별 결합, class·reach·progress, 전이 규칙 1(a)와 2, `unknown(model_level)`, contested bound, portability-hazard-evidence, obligation 실패 | §12 D3 (rev 1.3); §1.8, §1.10, §4.8–4.14 | [`../03_workflow.md`](../03_workflow.md) |
| D4 개요 | [`D4_research_overview.mmd`](D4_research_overview.mmd) / [`.svg`](D4_research_overview.svg) | 검증 사다리(rev 1.3: 첫 rung V1′ = R2 기반 검증, 기여 근거가 아니다; V3 = C1(D22 basis)이 V1′와 병행; V2는 V1′ 뒤; V1의 certificate 출구 항목은 선택), 단계 A–F 대응과 단계 B의 사례별 완료 조건(D5), 사례 track, 단계 S0–S7과 S10(S8 삭제, D6), freeze F1·F2, 기존 process-level race 연구와의 비교(V4a), 확장 평가(V4b), 결정(P-LADDER와 2026-10-22 결정 D1, D3, D5 등). V2와 V3 사이에는 edge가 없다 | §12 D4; §9.3 "Ladder of revision 1.3", §9.4, §10.1; ADR-044, ADR-057 | [`../03_workflow.md`](../03_workflow.md), [`../08_roadmap.md`](../08_roadmap.md), [`../10_decisions_adr.md`](../10_decisions_adr.md) |
| D4 상세 | [`D4_research_evaluation.mmd`](D4_research_evaluation.mmd) / [`.svg`](D4_research_evaluation.svg) | 평가 경로: S3a → freeze F1(평가 engine과 규칙, process-level 기법의 재구현, 평가 기준, 기대표 hash) → template-only 실행 → refinement → F2 → 확장 평가(held-out (a)–(e), recall, triage). 사례 역할(rev 1.3: R2 = 기반 검증, C1 = 첫 기여 근거 사례), 기존 process-level race 연구와의 비교(분석 질문 여섯, 결과 상태; 실행하지 못한 원 도구는 문헌·코드 결과 상태만, 2026-10-22 결정 D4), ablation 24와 rev 1.3의 단일 요소 비교, perturbation 실험군과 관측 영향 점검, 기술 통계(관측 위반·모형 후보·확인 실행을 따로 셈, OUT-01), claims table, deviation 기록. thread 수준 도구와 GPredict식 재구현 비교자는 없다 | §12 D4; §7.2, §7.5, §8.1–8.7 | [`../07_evaluation.md`](../07_evaluation.md) ([`../08_roadmap.md`](../08_roadmap.md)) |
| D5 개요 | [`D5_cfs_mapping_overview.mmd`](D5_cfs_mapping_overview.mmd) / [`.svg`](D5_cfs_mapping_overview.svg) | cFS 계층(PSP, OSAL, ES, EVS, SB, TBL, TIME, FS/CDS, 앱)마다 Φ_cFS의 어떤 kind·규칙·준비 상태 명세가 되는지 | §12 D5; §1.3, §3.1–3.16 | [`../05_cfs_mapping.md`](../05_cfs_mapping.md) |
| D5 상세 | [`D5_cfs_mapping_detail.mmd`](D5_cfs_mapping_detail.mmd) / [`.svg`](D5_cfs_mapping_detail.svg) | 코드 위치 예와 Φ 항목: `cfe_es_start.c:835` → R19, `:803/:818/:823` → `es.app`과 할당 기록 ρ, `cfe_sb_task.c:138` → `cell(CFE_SB_Global.AppId)`와 capture point, `OS_QueuePut` → K-CAP, MsgLim → K-CAP.msglim, `osapi-idmap.c:384–385` 대 `:401–404` → K-LOCKBAL, `cfe_psp_support.c:87` → `es.shutdown` | §12 D5; §3.2–3.9 | [`../05_cfs_mapping.md`](../05_cfs_mapping.md) |
| D6 개요 | [`D6_verdict_overview.mmd`](D6_verdict_overview.mmd) / [`.svg`](D6_verdict_overview.svg) | (σ, M) 칸과 bound 하나의 class 영역 상태 기계: candidate, open(class가 아니고 `unknown(open-route reason)`으로 보인다), inactive, protected[cert]·[model b], allowed-not-ready[model b], fixed-order[cert]·[model b], no-final-status, ordinal-split, schedule-dependent와 refinement timeout-dependent, `unknown(no_decision_route)`, contested, unknown 이유(one_sided, unordered, O-ordered-only, conditional-observation 포함), model-fault 기록, 불변식. rev 1.3 주석: certificate 전이는 선택 경로, 요약 뷰는 상태를 바꾸지 않는다 | §12 D6; §1.10, §4.12, §4.14 | [`../03_workflow.md`](../03_workflow.md) ([`../01_overview.md`](../01_overview.md), [`../04_formal_model.md`](../04_formal_model.md)) |
| D6 상세 | [`D6_verdict_axes.mmd`](D6_verdict_axes.mmd) / [`.svg`](D6_verdict_axes.svg) | 판정 칸의 병렬 영역: evidence realization, naturalness, provider, reach(assumed → proved, model(b), unreached(b) → contested), scope, progress(`unknown(not_checked)` → stall-witness, `no-stall-within(b)`, `unknown(…)`, contested), 모든 칸의 trust ledger 네 부분(certificate가 없으면 (i)이 비어 있다; rev 1.3에서 관측 기반 판정·예측·확인에도 붙는다)과 증명 상태 | §12 D6; §1.8, §1.10, §4.5.1, §4.12–4.14 | [`../03_workflow.md`](../03_workflow.md) ([`../01_overview.md`](../01_overview.md)) |
| D7 | [`D7_observation_pipeline.mmd`](D7_observation_pipeline.mmd) / [`.svg`](D7_observation_pipeline.svg) | 관측 파이프라인: `plan.kz` → loader → inode 기준 uprobe 등록과 write watchpoint → BPF 프로그램(attach cookie로 site descriptor 조회, `tseq`/`objseq`) → ring buffer → drainer → `run-*.kzt`; `kz hold`의 stop/continue; 비정상 종료 뒤 normalization boot. kernel 상자는 실행마다 기록(spec §1.7 Env) | §12 D7; §5.1–5.7, §4.13 | [`../03_workflow.md`](../03_workflow.md), [`../06_observation_confirmation.md`](../06_observation_confirmation.md) ([`../01_overview.md`](../01_overview.md)) |
| D8 개요 | [`D8_r2_certificate_overview.mmd`](D8_r2_certificate_overview.mmd) / [`.svg`](D8_r2_certificate_overview.svg) | R2 FIXED_ORDER certificate의 ES·EVS 두 절반: capture point `c`, frontier 노드 `m`, raise, falsifier `f`와 할당 기록 ρ, 관측 `o`(`:835`), panic 분기(T-TERM), SB spawn → SB 진입 → writer `x`(`cfe_sb_task.c:138`), INIT, `derives`. 결론은 신뢰 기반에 상대적이고 "lemma status: unproved". rev 1.3: R2는 기반 검증(V1′)이고 이 certificate는 보편 진술을 위한 선택 경로(점선 주석 상자) | §12 D8; App. A.1; §4.4, §4.5, §6.2 | [`../05_cfs_mapping.md`](../05_cfs_mapping.md) ([`../01_overview.md`](../01_overview.md), [`../04_formal_model.md`](../04_formal_model.md)) |
| D8 상세 | [`D8_r2_certificate_writers.mmd`](D8_r2_certificate_writers.mmd) / [`.svg`](D8_r2_certificate_writers.svg) | R2 certificate의 writer 목록 상자(V의 writer, `W_o(o_ES)`, `W_o(o_EVS)`, 각 항목의 처리 규칙과 transition·key status, `DECLARED_TRUSTED` 항목)와 trust-ledger 상자(App. A.1 step 9a의 네 부분. (iii)에 lemma별 증명 상태, reach의 L-CONT (c)와 L-MAPI-CONE, A7-api; (iv)에 L-CONT (c)·L-MAPI-CONE·L-REACH-CONE의 실행 쪽 반박(O1, O2, O4, O5); 모든 R2 certificate 칸은 "lemma status: unproved") | §12 D8; App. A.1 steps 3–5, 9, 9a; §4.2, §4.4, §4.5.1, §6.2 | [`../05_cfs_mapping.md`](../05_cfs_mapping.md) |
| D9 | [`D9_teardown_scoping.mmd`](D9_teardown_scoping.mmd) / [`.svg`](D9_teardown_scoping.svg) | 종료 범위: 요청자 actor(`CFE_PSP_Restart` → `ShutdownReq` 저장 → 10 s 지연 → `abort`)와 PSP main thread(`OS_Application_Run` → `OS_DeleteAllObjects`), SR ∈ MB, in-scope 사용과 K-TEARDOWN 사용(SHUTDOWN_REQ 뒤에 결정되는 occurrence), 다음 실행: POWERON 재시작은 shm segment를 지우지만 모든 유지 파일을 초기화하지 않으므로 초기 상태를 기록·검사하고 깨끗하다고 가정하지 않는다(D17 (a)); 다른 종료 → PROCESSOR reset → normalization boot | §12 D9; §3.5, §1.6, §4.5, §4.13, §6.4 항목 49, App. A.7 | [`../05_cfs_mapping.md`](../05_cfs_mapping.md) |
| D10 | [`D10_skeleton_views.mmd`](D10_skeleton_views.mmd) / [`.svg`](D10_skeleton_views.svg) | 같은 actor의 두 skeleton: certificate view `Sk^cert`(loop를 star-closure로 닫음)와 exploration view `Sk^h`(h = 1, 2로 펼치고 나머지는 TRUNC), 각 view를 쓰는 소비자, `kz proofcheck`의 view 검사, litmus L-H2 | §12 D10; §4.3, §4.5, App. A.8 | [`../04_formal_model.md`](../04_formal_model.md) ([`../01_overview.md`](../01_overview.md)) |

### 나눈 그림

§12의 노드가 한 그림에 넣기에 많은 D2, D3, D4, D5, D6, D8은 개요(`*_overview.mmd`)와 상세로 나눴다. 개요는 흐름만 보이고, 상세는 §12가 요구한 노드와 edge를 모두 담는다. D2와 D3은 상세가 두 장(정적 쪽, 동적 쪽)이다. D4는 개요가 검증 사다리·사례 track·단계·PI 결정이고, 상세가 평가 경로다. D6은 개요가 class 영역의 상태 기계이고, 상세가 병렬 영역(reach, progress, trust ledger 포함)이다. D8은 개요가 certificate의 순서 증명이고, 상세가 §12가 요구한 "writer 목록 상자"와 trust-ledger 상자다.

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
- **검증 사다리**: rev 1.3에서 첫 rung은 V1′(ENV basis에서 R2의 기반 검증: 관측, task→앱 매핑을 포함한 정규화, 설명 집합 위의 관측 기반 판정, trust ledger)이고, V3(C1, D22 basis)가 나란히 간다. V1의 certificate 출구 항목은 선택이다. thin MVC의 전체 gate는 V1b다(§9.3 "Ladder of revision 1.3", ADR-044, ADR-057).
- **필수 경로와 선택 경로** (rev 1.3, ADR-053): 필수 경로는 basis·extract·link → plan → observe → trace → predict → hold → report다. MustBefore·certificate·`kz proofcheck`, explorer, 진행 결과, loop, gate는 그대로 정의된 선택 경로이고, 그 주장이 필요할 때만 쓴다(§2.2).
- **관측 기반 판정과 설명 집합**: 실제 기록 위의 판정은 기록과 양립하는 실행의 모든 설명(`Cobs`)에서 성립할 때만 확정되고, 아니면 이유를 붙인 `INCONCLUSIVE`다(§4.8, ADR-055). 관측 적합성 기록은 조건 instance마다 시작 상태 근거, 부착, 손실, 관측되지 않은 효과를 적는다(§2.3).
- **요약 뷰**: 바뀌지 않은 상태 위에 계산하는 판정 요약이다. 관측(`OBSERVED_VIOLATION`, `NO_VIOLATION_IN_CHECKED_PREFIX`, `INCONCLUSIVE(reason)`, `NOT_APPLICABLE`), 예측(`MODEL_CANDIDATE`, `NONE_IN_SCOPE`, `INCONCLUSIVE(reason)`, `NOT_RUN`), 확인(`CONFIRMED`, `NOT_CONFIRMED(reason)`, `NOT_RUN`). `PENDING`과 `OPEN`은 `INCONCLUSIVE(horizon-open)`이다(§1.10, ADR-054).
- **`app_owner`**: actor instance를 만든 애플리케이션과 그 근거 수준(`confirmed` / `inferred` / `undetermined`)이다(§1.2, §5.7, ADR-056).

## spec rev 1.3 반영

2026-10-22에 그림을 spec rev 1.3(spec §0.12, §12 D2·D3의 revision 1.3 지시 C-48, §9.3 "Ladder of revision 1.3")과, 그림에 아직 옮기지 않았던 rev 1.2 결정(2026-10-15: T1 이름 "readiness specification"(준비 상태 명세), 기존 process-level race 도구와만 비교, MLIR 단계 삭제, D17 (a), D11의 kernel 기록, CORD 인용 제거, lemma와 trust ledger의 증명 상태와 L-CONT (c))에 맞췄다. 모두 결정 [D]를 그림으로 옮긴 것이고, 새로 만들거나 실행한 것은 없다. 바뀐 `.mmd`는 모두 다시 렌더링했다(Mermaid CLI 11.17.0).

| 그림 | rev 1.3 반영에서 바뀐 것 |
|---|---|
| D0 | 필수 경로 실선, 선택 경로(`skel`, abstract·operational 해석, loop, gate) 점선; `plan`을 `static`에서 뗌; 관측 적합성 기록; trace의 정규화·`app_owner`·설명 집합; 예측은 명시적 상태 탐색 먼저; report의 요약 뷰; "준비 상태 명세"; 판정에 증명 상태 |
| D1 | kernel을 실행마다 기록(`-fc-v114` in [R07 §1], 2026-10-16에 `-fc-v139`; spec §1.7 Env); MLIR·TSan·valgrind 제거와 후순위·조건부 도구 표시(rev 1.2, 02의 사본과 맞춤) |
| D2 개요, 상세 1, 상세 2 | 선택 경로 점선(spec §12 D2 rev 1.3); `plan`이 `static`·`skel`과 따로 실선으로 `facts.kz`·`anchors.kz`를 받고 손 보강 계획을 받음; observe와 trace 사이의 관측 적합성 기록; report의 요약 뷰; conform의 O8은 선택 경로 |
| D3 개요, 상세 1, 상세 2 | certificate 판단 앞에 관측 기반 판정(설명 집합) → 요약 뷰 경로와 끝 노드(`OBSERVED_VIOLATION`, `NO_VIOLATION_IN_CHECKED_PREFIX`, `INCONCLUSIVE(reason)`(`horizon-open` 포함), `NOT_APPLICABLE`), 예측 → `MODEL_CANDIDATE` / `NONE_IN_SCOPE` / `INCONCLUSIVE` → 확인 → `CONFIRMED` / `NOT_CONFIRMED`; certificate 갈래와 explorer는 선택 경로; "schedule dependence undetermined"; rev 1.2의 "readiness instance", `allowed-not-ready`, 증명 상태, NOT_EVALUATED 포함 protected |
| D4 개요 | 첫 rung V1′(R2 기반 검증), V3(C1, D22 basis)가 V1′와 병행, V2는 V1′ 뒤, V1의 certificate 출구 항목은 선택; 단계 A–F 대응과 단계 B 완료 조건(D5); rev 1.2의 S8(MLIR) 삭제, 비교는 기존 process-level race 연구와, 결정 값 |
| D4 상세 | rev 1.2: 같은 정보 재구현·helgrind/DRD 비교자 제거, process-level race 연구와의 비교, ablation 24(기술), 기술 통계, S8 삭제; rev 1.3: 사례 역할(R2 기반 검증, C1 첫 기여 근거), 실행하지 못한 원 도구의 결과 상태(2026-10-22 결정 D4), 단일 요소 비교, OUT-01 |
| D6 개요, 상세 | rev 1.2 이름(`allowed-not-ready`, 결정된 발생이 READY 또는 NOT_EVALUATED)과 unknown 이유 목록; `open`은 `unknown(open-route reason)`으로 보인다; trust ledger 영역은 모든 칸(certificate가 없으면 (i)이 빈다); 증명 상태; rev 1.3 주석(certificate 전이는 선택 경로, 요약 뷰는 상태를 바꾸지 않음, 장부의 일반화) |
| D7 | kernel 상자: 실행마다 기록(spec §1.7 Env) |
| D8 개요 | 결론에 "lemma status: unproved"; rev 1.3 주석 상자(선택 경로, R2는 기반 검증) |
| D8 상세 | lemma별 증명 상태, reach의 L-CONT (c)·L-MAPI-CONE, A7-api, (iv)의 실행 쪽 반박 항목, "lemma status: unproved" |
| D9 | 다음 실행 상자: "POWERON restart → clean Init"을 D17 (a)로 바꿈(segment 삭제, 초기 상태 기록·검사) |
| D5, D10 | 바꿀 것이 없다. spec §12 D5·D10은 rev 1.2·1.3에서 이름("readiness specifications")만 바뀌었고 그림에 "contract"·CORD·MLIR이 없다. D10의 certificate 소비자는 rev 1.3에서 선택 경로이지만 그림은 skeleton view의 쓰임을 보일 뿐이다 |

문서에 복사한 Mermaid 블록은 `.mmd`와 글자 하나까지 같아야 한다. 2026-10-22에 01–06의 복사본은 모두 새 `.mmd`로 바꿨다. 아래 복사본은 아직 이전 판이고, 그 문서가 `.mmd`를 다시 복사할 때까지 이전 그림이다. 다시 복사하면 이 목록에서 뺀다. 각 문서의 caption 옆 `.svg` 링크는 이미 새 그림을 보인다.

- [`../07_evaluation.md`](../07_evaluation.md)의 그림 D4 상세: rev 1.2 판(사례 역할, 원 도구 미실행, 단일 요소 비교, OUT-01 없음).
- [`../08_roadmap.md`](../08_roadmap.md)의 그림 D4 개요: rev 1.2 판(V1′, V3 병행, 단계 A–F 없음).
- [`../09_prior_work.md`](../09_prior_work.md)의 그림 D0: rev 1.2 판(필수·선택 경로 구분, 관측 적합성 기록, 요약 뷰 없음).
- [`../10_decisions_adr.md`](../10_decisions_adr.md)의 그림 D4 개요: 08과 같다.

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

다른 머신에서는 `.puppeteer.json`의 `executablePath`를 그 머신의 Chromium 경로로 바꾼다. Mermaid CLI 버전은 11.17.0으로 고정한다. 2026-10-22의 렌더링도 이 버전으로 했다(같은 버전으로 rev 1.1 원본을 다시 렌더링하면 기존 `.svg`와 byte 단위로 같았다).

## 작성 규칙 (그림을 고칠 때)

- 원본은 `.mmd`다. `.svg`만 고치지 않는다. `.mmd`를 고치면 위 명령으로 `.svg`를 다시 만든다.
- 문서에 넣을 때는 `.mmd` 내용을 ` ```mermaid ` 블록에 그대로 복사한다. 블록 바로 앞에 한 줄 caption을 둔다: `**그림 Dn(개요/상세). 제목** (렌더링: diagrams/….svg; spec §…)`.
- **label 언어.** 그림 안의 label은 모두 영어로 쓴다(코드 이름, spec 용어와 같은 말을 쓰기 위해서다). 한국어 설명은 문서의 caption과 본문이 맡는다. 같은 개념은 그림마다 같은 label을 쓴다(예: feedback F4의 도착점은 언제나 "Φ revision (human review)").
- GitHub에서 그려지도록 Mermaid 10에 있던 문법만 쓴다: `flowchart`(subgraph, `direction`, `-.->`, `--x`, `<-->`, `classDef`), `stateDiagram-v2`(복합 상태, `--` 병렬 영역, note). 공백·괄호·기호·한글이 든 label은 큰따옴표로 감싼다. HTML은 `<br/>`만 쓴다. 꺾쇠와 `&`는 Mermaid entity code `#60;`, `#62;`, `#38;`로 쓴다(예: `osal.obj#60;T#62;`).
- 배치는 Mermaid가 자동으로 정한다. D1, D3 상세 2, D4 개요, D6 개요·상세처럼 넓은 그림은 GitHub 미리보기보다 `.svg`를 따로 열어 확대해 보는 편이 읽기 쉽다.
- 그림 내용은 spec을 따른다. spec이 바뀌면 그림을 고치고 이 표의 spec 절을 확인한다.

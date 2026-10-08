import re,glob
N='/tmp/claude-0/-home-user-Shape-Perf/e983f27d-f015-5a5d-b912-399245630e78/scratchpad/cfs_note'
toc=[]
for f in sorted(glob.glob('work/[01]*.md')):
    if f.endswith('00_front.md'): continue
    for l in open(f):
        if l.startswith('## '): toc.append('- §'+l[3:].strip().replace('. ',' ',1))
        elif l.startswith('### '): toc.append('  - '+l[4:].strip())
front=f'''# 연구노트: MLIR 기반 cFS 앱 간 순서·시간 의존성 정적 분석

| 항목 | 내용 |
| --- | --- |
| 작성일 | 2026-10-01 |
| 상태 | 문제 정의, 프레임워크 의미 조사, 분석 설계, 실현 가능성 실측 단계. 분석기는 구현하지 않았다. 검출 결과, 검출률, 신규성은 주장하지 않는다. |
| 바탕 | 두 입력 노트. 원노트 `4ccc8aa8-MLIR_Flight_Software_Race_Research_Note_20260930_1.md`(2026-09-30)와 수정본 `fc87c4b6-MLIR_Flight_Software_Race_Research_Note_202609302-2.md`(2026-10-01). 여기에 1차 출처 재확인, 이 컨테이너의 실측, 핵심 주장 22개의 교차 검증(검증 C01–C22, §10.1)을 더했다. 본문의 "원노트 §N"과 "수정본 §N"은 두 입력 노트의 절 번호다. |
| 고정점 | cFS bundle `5a9b075c` [S87]. 이 bundle이 cFE `546a002515be5a1e3b66f9ae2c14f948d9cec76f` [S25], OSAL `dad0ee99` [S88], PSP `53df1d54`, HK `0dc16b7a` [S91], to_lab `d27c6014` [S94], sch_lab `607e2f90` [S95], sample_app `199476a3` [S90]을 고정한다 (`$N/probe/submodules.txt`). ES·TBL 실측은 같은 cFE에 OSAL `5befd8e9` [S89], PSP `36c24cb9`를 붙인 별도 build에서 얻었다 (`$N/sem-es-tbl/runs/probe_results_summary.txt` L2-5). LLVM·MLIR은 로컬 `1053047a`(읽기 전용)를 쓰고 upstream main `ccac700c` [S107]과 대조했다. |
| 실측 환경 | Linux native simulation, 4 vCPU, kernel 6.18, gcc 13.3.0. **[해석]** 모든 실측은 RTOS 비행 build의 동작 근거가 아니다. 횟수는 관측값이며 발생률이 아니다. |
| `$N` | 로컬 증거 경로 `{N}`. 본문의 `$N/…`은 이 디렉터리 아래의 실행 로그·probe 출력·검증 기록이다. |

## 요약

이 노트는 NASA cFS 앱 사이의 메시지 순서, 메시지로 갱신되는 상태, startup·재시작·Table Services 의존에서 생기는 결함을 MLIR 기반 정적 분석으로 찾을 수 있는지 검토한다. 두 입력 노트의 구상을 고정 버전의 소스, 이 컨테이너의 실측, 1차 출처, 핵심 주장 22개의 교차 검증으로 다시 점검했다. **[확인된 사실]** cFE `546a002`의 SB·ES·TBL은 발신자에게 보이지 않는 목적지별 전달 실패, timeout이 있는 soft startup 동기화와 자기 준비 표시, handle 단위 table 잠금, 버전에 따라 다른 전달 lock 위치와 재시작 동작을 가진다 (§3). **[실측]** 실제 cFS C 파일 154개가 조건부 compile flag로 MLIR LLVM dialect에 진단 없이 import되었고, 코드에서 정의한 MID는 call site 상수로 회수되었다. 그러나 구독 site 47곳 중 8곳은 table·명령·설정에서 MID를 받고, MID 이름은 IR에서 사라진다 (§7). **[설계 제안]** 이를 바탕으로 결함 범주 F1–F8, `cfs` dialect, 사건 모델과 HB 규칙, 상관을 보존하는 추상 도메인, 세 층의 평가, 16주 계획을 제안한다 (§1, §5–§9). 분석기는 구현하지 않았으므로 검출 결과, 검출률, 신규성, MLIR의 우월성은 주장하지 않는다.

## 결론 상자

> **확립된 것.**
> - **[확인된 사실]** cFE `546a002`에서 경로 없음, 목적지 0개, MsgLim 초과, pipe full은 모두 발신자에게 `CFE_SUCCESS`다. startup script 앱의 ES 동기화는 soft limit이고 OPERATIONAL이 모든 앱의 RUNNING을 뜻하지 않는다. TBL 잠금은 handle 단위이며 `NEVER_LOADED`의 구현은 header 설명과 다르다 (§3, 검증 C09·C11·C13·C14).
> - **[확인된 사실]** 전달 lock 위치(550e7f7d [S85]), TBL 재등록(c1ab1b7 [S86]), TO_LAB 지연 구독(d3d52da [S46])은 cFE·앱 버전에 따라 바뀐다 (§3.6).
> - **[실측]** C → LLVM IR → MLIR import 경로는 실제 cFS 코드에서 막히지 않는다. 비용은 import가 아니라 의미 복원(MID 이름, table·명령 유래 MID, out-parameter, field 이름)에 있다 (§7, 검증 C17 정정 문구, C18).
> - **[확인된 사실]** cFE #73의 crash 경로는 현재 코드에서 닫혀 있다. 같은 모양의 잔여 문제(#2663 [S41])는 검증 중 gdb로 관측했다 (§2.2, 검증 C12·C21 정정 문구).
>
> **제안한 것.**
> - **[설계 제안]** 결함을 필요 순서 `Req`, 보장 부재, Σ에서의 실행 가능성, 관측 가능한 결과의 네 조건으로 정의하고, 결과를 후보·모델상 가능·재현으로 나눈다 (§1, §8.1).
> - **[설계 제안]** `cfs` dialect(§5), cFE 의미에서 얻는 HB 규칙과 상관 보존 도메인(§6), 합성 앱·공개 앱 결함 주입·역사적 사례의 세 층 평가(§8), 관문 GO-1–GO-8을 둔 16주 계획(§9).
>
> **모르는 것.**
> - **[미확인]** 같은 SB·ES·TBL 모델을 넣은 CodeQL이나 AST 구현이 같은 후보를 내는지(Q1), 읽지 못한 선행 본문(R03, R16, R17, R04 등)이 같은 일을 했는지(Q2), 앱별 계약 없이 `Req`를 얼마나 얻는지(Q3) (§10.5).
> - **[미확인]** RTOS build에서의 순서 의미, Swift/BAT race의 원인과 cFE와의 관계(검증 C22, contested), F4의 신규성(§4.7.3).
>
> **계속 여부.** **[해석]** 조건부 GO다. frontend 경로가 막히지 않았고 cFE 의미가 `Req`와 `G`의 구체 출처가 되므로 다음 단계로 갈 근거는 있다. 다음 단계는 §7.13.4의 23개 site 추출 정답표를 자동으로 재현하는 것이고, race 판정은 그다음이다. W4의 GO-2(23/23, table 값 집합, 47곳 분류)가 실패하면 RQ1을 철회하고 앱 내부 분석으로 줄인다. W8의 GO-3에서 AST 경로가 추출을 맞히고(PIVOT-1) `cfs` dialect가 E-llvm보다 나은 점이 없으며(PIVOT-2) Clang CFG 위의 시제품이 §44 예를 맞히면, MLIR 기반을 철회하고 도메인 의미의 기여만 계속 검증한다 (§9.3.5, §9.9). Q1–Q3이 닫히기 전에는 신규성과 MLIR의 이점을 주장하지 않는다.

## 근거 표기

| 표기 | 뜻 | 결론에 쓸 수 있는가 |
| --- | --- | --- |
| **[확인된 사실]** | 고정 commit의 코드, 공식 문서, 논문 본문을 직접 읽었다. 확인 범위(전문·초록·검색 snippet)를 함께 적는다 | 쓴다. snippet 수준이면 그 범위를 밝힌다 |
| **[실측]** | 이 컨테이너에서 명령을 실행해 얻었다. 명령과 결과 파일(`$N/…`)을 적는다 | 쓴다. 측정 조건 Σ(권한, CPU 수, `msg_max`, OSAL permissive 설정)를 함께 쓴다 |
| **[원노트 구상]** | 원노트·수정본이 제안한 아이디어다. 아직 검증하지 않았다 | 연구 방향으로만 쓴다 |
| **[설계 제안]** | 이 노트가 제안한 구체 설계다. 구현하지 않았다 | 실험 계획으로만 쓴다 |
| **[해석]** | 사실에서 끌어낸 판단이다 | 판단임을 밝혀 쓴다 |
| **[미확인]** | 근거를 열지 못했거나 실행하지 않았다 | 결론의 근거로 쓰지 않는다 |

**교차 검증 표기.** "검증 Cnn"은 핵심 주장 22개의 교차 검증 claim이다. 판정(confirmed, refuted, contested)과 목록은 §10.1, 기각·이견·보정 문장은 §10.3에 있다. refuted 주장은 원래 문장으로 쓰지 않고 정정 문구만 쓴다. contested 주장은 이견을 함께 쓴다.

**ID 규칙.**

| ID | 뜻 | 정의한 곳 |
| --- | --- | --- |
| R01–R66, S01–S150 | 참고문헌. R01–R11과 S01–S25는 수정본의 ID를 유지했다 | §11 |
| F1–F8 | 결함 범주. F7은 메시지 간 도착 순서, F8은 재시작 창이다 | §1.2 |
| Σ, RQ1–RQ6, H1′–H3′ | 실행 가정, 연구 질문, 가설 | §1.3, §1.6 |
| SB-, OS-, ES-, TBL-, EVT- 번호 | 프레임워크 의미 항목 | §3.1–§3.5 |
| M1–M14, A1–A11 | 분석기가 모델링할 사실, 가정해도 되는 사실 | §3.8, §3.9 |
| K1–K14, N1–N6 | 구성 요소별 선행 범위, 신규성 가설 | §4.6, §4.7.2 |
| D1–D8, V1–V11, L1–L5, P0–P7 | dialect 설계 원칙, verifier 규칙, lint 규칙, lifting pass | §5.1, §5.6, §5.8 |
| AP1–AP6, S1–S9, HB-*, R-F*, FP1–FP11 | 분석 설계 원칙, 처리 단계, HB 규칙, 필요 순서 규칙, 오경고 통제 | §6 |
| MVP | 첫 prototype(단계 1). §5.8의 pass P0과 다르다 | §7.13, §9.3 |
| L0–L2, SYN-*, INJ-*, B0–B8, AB1–AB10, I1–I7 | 증거 수준, 합성·주입 사례, 기준선, 도메인 ablation, 구현 기반 ablation | §8 |
| W1–W16, GO-1–GO-8, G3-a–G3-d, RK-1–RK-15 | 주, 관문, GO-3 측정 항목, 위험 | §9 |
| CM, CS, CE, CT, CG, CX, CL, CP, CD, CR 번호 | 주장–근거 대응표의 주장 | §10.2 |
| O1–O29, RV1–RV10, Q1–Q15 | 원노트 보정, 수정본 보정, 미해결 질문 | §10.4, §10.5 |

## 목차

'''+'\n'.join(toc)+'\n'
open('work/00_front.md','w').write(front)
print(len(front.splitlines()))

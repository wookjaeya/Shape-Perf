# 협조 게이트 G-I-H 제어 실행과 F0 confirm2 대조 — C1 파생 빌드, 계획 9개 × 4 인스턴스 (단계 F, 2026-10-26)

- 지위: **제어 실행이며 pilot/dev**다. 본 실험 결과가 아니다. 계획·구성·인스턴스마다 부팅 1회다. 빈도·재현성은 주장하지 않는다(P8). 반복 횟수는 PI 결정 대기다(단계 E).
- 범위: IR §15 단계 F의 협조 게이트, §13.3의 제어 비교 단위, PD26(`Architecture/spec/PI_DECISIONS_RESOLVED_20261026.md`) §3의 P11–P13. IR은 2026-10-26 계측 아키텍처 검토(사용자 업로드)다.
- 표기: [F] 코드·문서에서 확인, [M] 실행·재생으로 확인, [I] 추론, [D] 설계 결정, [U] 미확인.
- 대조 기록(F0): confirm2 36회(`/home/user/work/procrace/c1_runs/confirm2/<W>_<cfg>_<cand>_<inst>`). 계획은 confirm2와 같은 파일이다. W1·W2·W10은 `predict1/<cfg>_<cand>/cur/<W>.kzh`, 나머지는 `predict2/…`. 결과는 `plan/results_pilot_20261025.md` §5.6이다.
- 연구 조건은 새로 정하지 않았다(CONDITIONS_POLICY). 계측 빌드는 파생 구성이다. 차이는 §0과 각 빌드 기록(`build_instr.record`, D1–D4)에 있다.

## 0. 조건 [M]

| 항목 | 값 | 상태 |
|---|---|---|
| 빌드 | 단계 F 파생 빌드 `/home/user/work/instr/gcc/f1/c1/{pre,post}_{1df262a,1b0b338}`(`derive_c1.sh instr`). plugin `kzgcc 0.2` sha256 `56a96b79…`(단계 D와 같음). manifest `sites-cf833fdbb.kzs` `fe117ba6…`, `sites-cf281a941.kzs` `1f37f87c…`. 런타임 `kzrt 0.3.0`(dump 1.3, 계획 형식 `kzrt-plan` 1.0). binding은 post 38개, pre 37개이고 hook 검사는 모두 ok다. `kz_rt_ctl` 호출 binding은 5개다(197312·197328 BP, 131616·131632·131664 CF). 기준 빌드와 다른 object는 11개(계측 번역 단위와 D4) | deviation(파생 구성, D1–D4) |
| manifest 변경 | `control 35 ctx=task`, `control 45 ctx=task`(조회·생성 호출 반환 직후, 해제 사이트). 새 site 37 `cf_exit_app_call.before`(`CF_AppMain`의 `CFE_ES_ExitApp` 호출 직전, `cf_app.c:366`), 값 `exit_status`, `control 37 ctx=terminating`. 그 결과 단계 D에서 사본 2개이던 binding 131616·197312가 `link=1`이 되었다(제어 hook이 있는 사본 하나) | 도구 변경. 위치는 F0 계획의 사이트 label을 따름 |
| 모드·계획 | `KZ_RT_MODE=control`(G-I-H), `KZ_RT_PLAN=<run>/plan.kzrtp`. 계획은 실행 전에 `kz rtplan`이 그 빌드의 manifest와 binding 목록으로 변환했다(§5). dump는 `plan status=loaded flags=manifest-checked` 36/36 | 도구 실행 매개변수 |
| adapter | POSIX adapter(private pthread mutex 65·cond 64, 정적). 시작 객체가 main 전에 붙인다. OSAL 객체는 쓰지 않는다. OSAL adapter는 OSAL을 초기화한 단위 시험에서만 돈다(§6) | IR §11.4(초기화 전·OSAL 밖은 private primitive) |
| 절차 | F0 confirm2의 한 인스턴스 단계(`observe_c1_hold.sh --one`): tar/untar, 인스턴스 하나만(ubuntu), 8 s, OPERATIONAL 확인, ES POWERON(UDP 1234/1235), 5 s, 생존 프로세스 SIGKILL. 실행 뒤 confirm2의 정규화 검사(POWERON 종료 줄, ubuntu의 SysV segment 없음). 스크립트 `kairos/instrument/gcc/scripts/run_gih_c1.sh`, `run_gih_confirm2.sh` | from-reference(confirm2와 같음) |
| 안전장치 | `--one` 전체를 `timeout 150`으로 감쌌다(F0 보류 실행의 pilot 안전장치, 연구자 선택). 끝나면 INCONCLUSIVE다. 36회 모두 발동하지 않았다 | deviation(F0와 같은 값) |
| 게이트 시간 제한 | 모든 점 `timeout_ns 0`(없음). F0 holder에도 없다 | unspecified_by_reference |
| 시작 상태 | 새 untar(EEPROM.DAT·키 파일 없음). 실행 전후 `persist_snapshot.sh`(읽기만). F0 confirm2와 같다 | from-reference |
| 환경 차이 | 인스턴스 환경 += `KZ_RT_MODE`·`KZ_RT_PLAN`·`KZ_RT_DUMP`·`KZ_RT_RUN_ID`·`KZ_RT_INSTANCE`(값은 기록 안 함, cFS는 읽지 않음). kz hold·kz observe 없음(ptrace·debug register·uprobe 없음) | deviation |
| 호스트 | 커널 `6.18.44-fc-v153`, nproc 4, `mqueue.msg_max=10`(confirm2와 같은 컨테이너) | 기록 |
| 순서 | confirm2의 실행 순서. 대표 계획(W1, W2, W10, W5, W6)을 먼저 돌리고, 확인 뒤 W7·W8·W11·W12를 돌렸다. 각 계획은 post/pre × `1df262a`·`1b0b338` × tx·rx다. 개발 중 시험 실행 1회(`f1/trial/W1_post_1df262a_tx`)는 집계하지 않았다(P12) | 기록 |
| 재생 | 두 계열을 같은 kz와 같은 프로필로 읽었다(`replay_gih.sh`). kz sha256 `6c4b95ca…`(커밋 `97a24d8` + 이 단계의 변경, 패치 `/home/user/work/instr/gcc/f1/kz_snapshot_vs_97a24d8.patch` `44dc4128…`). 프로필은 `97a24d8`의 스냅숏이다(작업 트리의 C1 프로필은 다른 작업이 바꾸는 중이었다). G-I-H는 `kz convert --kzrt`(manifest, binding 목록, B′ 부모)로 바꾼 뒤 F0 재생과 같은 상수·읽기(cur·p1off·rfoff·rfoff_p1off·idoff·cw_counted·cw_excluded)에 `--control-order off` 두 개(ctloff, ctloff_p1off)를 더했다. F0는 confirm2의 닫힘(`c1prof_i1025[_rx]`)을 썼다. 커밋된 kz(`662abdd` + 이 커밋, `3c539d59…`)로 같은 입력을 다시 읽어도 G-I-H cur·ctloff 72/72, F0 cur 36/36이 같다 | from-reference(F0 재생 인수) |
| B′ | 빌드·인스턴스마다 그 compile DB로 만든다(`basis_gi.sh`, `KZ_PROFILES`로 같은 프로필 스냅숏) | `static_facts.md` §7a |
| 산출물 | `/home/user/work/instr/gcc/f1/`(저장소 밖): `gih/<W>_<cfg>_<cand>_<inst>/{run.meta,conditions.txt,rtplan.txt,plan.kzrtp,dump/,<inst>.dump.txt,normalize.txt,replay/}`, `gih/summary.txt`, F0 재재생 `f0re/`, `c1/`, `bprime/`. 재생 파일마다 `replay/sha256.txt` | — |

실행 결과: 36회 모두 OPERATIONAL, 정지 `ci-es-restart-poweron`, 종료 코드 0, POWERON 종료 줄 있음, SysV segment 남지 않음(정규화 부팅 불필요). `kz-rt-dump` 검사는 36/36 통과(capacity, mode, manifest, plan), `pool_dropped=0`, 손실 0이다 [M].

## 1. 요약 [M]

| 계획 (구성) | 라벨 | 실제 제어 = F0 | 부류 = F0 | 판정 ctloff = F0 cur | P11 결정 읽기에서 바뀐 근거 (인스턴스당) |
|---|---|---|---|---|---|
| W1 (post) | 제어 (P-C1HOLD), `timeout-exercised` | 4/4 | 4/4 (BP0 −17×25, CF 종료) | 4/4 | 위반 25: 반환 코드 좁힘 → 상태 순서 + 제어 순서 |
| W2 (pre) | 제어 (연구자 개입) | 4/4 | 4/4 (BP0 0×1, BP1 −17×1, 종료) | 4/4 | 2건 모두 제어 순서(READY는 RF 대신, 미판정은 좁힘 대신) |
| W10 (post) | 제어 (연구자 개입), `timeout-exercised` | 4/4 | 4/4 (BP0 0×1, BP1 −17×25, 종료) | 4/4 | 26건 모두 제어 순서 |
| W5 (post) | 제어 (연구자 개입), release 선도달 2 | 4/4 | 4/4 (BP0 −17×2 0, BP1 0, 초기화) | 4/4 | 실패 2: 좁힘 → 제어 순서. 성공 2는 RF 그대로 |
| W6 (post) | 제어 (연구자 개입), `timeout-exercised`, 선도달 2 | 4/4 | 4/4 (BP0 −17 0, BP1 −17×25, 종료) | 4/4 | 위반 25: 제어 순서. BP0 첫 실패는 좁힘 그대로 |
| W7 (post) | 제어 (연구자 개입), 선도달 3 | 4/4 | 4/4 (BP0 −17 0, BP1 −17×2 0, 초기화) | 4/4 | BP1 실패 2: 제어 순서 |
| W8 (post) | 제어 (연구자 개입), 선도달 3 | 4/4 | 4/4 (BP0 −17 0, BP1 −17 0, 초기화) | 4/4 | BP1 실패 1: 제어 순서 |
| W11 (post) | 제어 (연구자 개입), 선도달 1 | 4/4 | 4/4 (BP0 0, BP1 −17×2 0, 초기화) | 4/4 | 실패 2 + BP0 성공 1: 제어 순서 |
| W12 (post) | 제어 (연구자 개입), 선도달 1 | 4/4 | 4/4 (BP0 0, BP1 −17 0, 초기화) | 4/4 | 실패 1 + BP0 성공 1: 제어 순서 |

- "실제 제어 = F0"는 계획 점마다 보류(게이트·hit)와 해제(해제·hit·`resumed`/`satisfied`)를 정규형으로 바꿔 F0 `hold.kzr`과 비교한 것이다(`control_cmp.sh`). "부류"는 키마다 CF 조회 결과열과 CF의 끝이다(`classify_gih.awk`, confirm2 `classify.awk`의 표기). 판정 비교는 발생마다 이름·순번·상태·요약·근거를 본다(actor 이름·사건 번호 제외).
- 계획마다 네 인스턴스(`1df262a`·`1b0b338` × tx·rx)의 판정과 실제 제어가 모두 같다.
- 라벨은 confirm2 실행 기록의 라벨을 그대로 옮겼다.

## 2. IR §13.3 여섯 단위

### 2.1 계획이 요구한 논리 순서 [F][M]

계획 파일은 confirm2와 같은 `.kzh`(kzp-hold 1)다. `kz rtplan`이 점마다 사이트·binding·조건을 바꾸었고, 바꾼 내용이 실행 디렉터리의 `rtplan.txt`에 있다.

| 계획 | 게이트(사이트, 조건, 순번) | 해제(사이트, 순번 → 푸는 게이트) | 요구 순서 |
|---|---|---|---|
| W1 | G1 BP 생성 직전(44) `BP0_tsem` 첫 hit | R1 CF `CFE_ES_ExitApp` 호출 직전(37) 첫 hit → G1 | CF의 BP0 조회 25회 전부 → CF 종료 → BP0 생성 |
| W2·W10 | G1 BP 생성(44) `BP1_tsem`; G2 CF 조회 직전(34) `BP0_tsem` | R1 CF 종료(37) → G1; R2 BP 생성 반환(45) 첫 hit → G2 | BP0 생성 → CF BP0 조회 → CF BP1 조회(pre 1회, post 25회) → CF 종료 → BP1 생성 |
| W5 | G1 BP0 생성; G2 CF BP1 조회 첫 hit; G3 CF BP0 조회 3번째(모두 `unless-released`) | R1 조회 반환(35) 2번째 → G1; R2 생성 반환 2번째 → G2; R3 생성 반환 첫 hit → G3 | CF BP0 실패 2 → BP0 생성 → CF BP0 성공 → BP1 생성 → CF BP1 성공 |
| W6 | G1 BP0 생성; G2 BP1 생성; G3 CF BP0 조회 2번째 | R1 조회 반환 첫 hit → G1; R2 조회 반환 27번째 → G2; R3 생성 반환 첫 hit → G3 | BP0 실패 1 → BP0 생성 → BP0 성공 → BP1 실패 25 → (소진) → BP1 생성 |
| W7 (W8) | G1 BP0 생성; G2 BP1 생성; G3 CF BP0 조회 2번째; G4 CF BP1 조회 3번째(W8 2번째) | R1 조회 반환 첫 hit → G1; R2 조회 반환 4번째(W8 3번째) → G2; R3·R4 생성 반환 1·2번째 → G3·G4 | BP0 실패 1 → 성공; BP1 실패 2(W8 1) → BP1 생성 → BP1 성공 |
| W11 (W12) | G1 BP1 생성; G2 CF BP0 조회 첫 hit; G3 CF BP1 조회 3번째(W12 2번째) | R1 조회 반환 3번째(W12 2번째) → G1; R2·R3 생성 반환 1·2번째 → G2·G3 | BP0 생성 → BP0 성공; BP1 실패 2(W12 1) → BP1 생성 → BP1 성공 |

변환 규칙(H7):
- `caller-in <모듈>`이 붙은 함수 진입 점(`osal.countsem_create.entry caller-in bp.so` 등)은 그 모듈에서 그 callee를 부르는 `place=at-call` site의 호출 쪽 binding으로 바꾼다. 그런 site가 둘 이상이거나 없으면 계획을 거부한다.
- `name sem_name "…"`은 그 역할의 `string=validated` 슬롯과의 비교가 된다(상한 `OS_MAX_API_NAME` 20 미만).
- 조건 없는 해제 사이트는 그 site의 모든 binding이다.
- 평가할 site에는 manifest의 `control` 줄이 있어야 한다. 점의 문맥 class도 거기서 온다.

### 2.2 백엔드가 실제로 잡고 푼 것 [M]

- CTL 레코드(36 dump 합): `gate-hold` 52, `resume` 52, `fire resumed` 52, `fire satisfied` 48. `gate-refused`·`gate-busy`·`gate-consumed`·`timeout`은 0이다. 해제 37(ctx=terminating)은 해제 점이라 보류하지 않으므로 문맥 거부가 생기지 않는다.
- 계획 점마다 hit 번호가 F0와 같다. 예: W6 R2 hit 27 `resumed`, W11 R1 hit 3 `resumed`, W5 R2 hit 2 `satisfied`. 36/36이다(§1).
- 보류 시간(게이트 `gate-hold` 시각 → `resume` 시각, ms; F0는 `held_ns` → `release_ns`). 범위는 네 인스턴스의 최솟값–최댓값이다.

| 계획 · 게이트 | G-I-H | F0 confirm2 |
|---|---|---|
| W1 G1_BP | 2,491.5–2,503.7 | 2,494.2–2,504.8 |
| W2 G2_CF / G1_BP | 10.3–12.3 / 0.2–0.3 | 13.4–18.3 / 0.1–0.4 |
| W10 G1_BP / G2_CF | 2,504.2–2,508.4 / 10.9–14.4 | 2,516.1–2,549.3 / 11.9–15.3 |
| W5 G1_BP | 87.0–92.0 | 86.3–90.0 |
| W6 G2_BP | 2,488.5–2,504.7 | 2,502.0–2,508.1 |
| W7 G2_BP | 184.9–189.6 | 186.4–190.1 |
| W8 G2_BP | 86.9–89.7 | 83.0–88.7 |
| W11 G1_BP / G2_CF | 100.2–100.6 / 11.3–13.0 | 101.3–101.6 / 12.0–14.8 |
| W12 G1_BP / G2_CF | 0.2 / 9.6–12.5 | 0.0–0.4 / 11.3–16.1 |

- 긴 보류의 길이는 프로그램이 정한다. CF의 재시도 간격과 횟수(25 × 약 100 ms)다 [I: 범위가 F0와 겹침].
- CF를 잡은 짧은 보류(G2_CF)는 G-I-H 범위가 F0 범위보다 아래에 있다(끝값 차이 1–6 ms). F0의 해제는 BP의 ptrace 정지와 supervisor 처리를 거친다 [I]. 계획·인스턴스마다 1회라 차이를 주장하지 않는다.
- 측정 지점도 다르다. G-I-H는 hook 안의 시계, F0는 supervisor의 정지 처리 시각이다.

### 2.3 스케줄러가 낸 순서 [M]

- `fire satisfied` 48(인스턴스당 12)은 release가 게이트보다 먼저 온 경우다(H5, `unless-released`). W5 2, W6 2, W7 3, W8 3, W11 1, W12 1이다. 이 게이트는 잡지 않았고, 그 순서는 제어가 아니라 스케줄러가 냈다. F0도 같은 점에서 같은 hit로 `satisfied`였다.
- 재생에서도 이 부분은 제어 간선이 없다. 해당 판정은 F0와 같은 근거다.
  - W5·W7·W8의 성공 조회 2건과 W6·W11·W12의 성공 조회 1건은 RF 포함 상태 재생이다.
  - W6·W7·W8의 BP0 첫 실패는 반환 코드 좁힘이다.
- 제어 간선은 `resumed` 해제마다 하나다. 인스턴스당 W1 1, W2 2, W10 2, W5 1, W6 1, W7 1, W8 1, W11 2, W12 2개이고, 합 52 = `resume` 52다. 버린 간선(역방향·사건 없음)은 0이다.

### 2.4 최종 결과와 판정 [M]

> 2026-10-28 주: 이 절의 근거 일치는 프로필 스냅숏 `97a24d8`로 읽은 결과다. 현재 프로필(`stored_out` 절, `30_hb.kzp` L48, `662abdd`)로 읽으면 판정은 그대로 같고, 성공 조회의 근거가 F0 `return-derived-order`, 계측 `store-derived-order`로 갈린다(`plan/results_w4w5_20261028.md` §1.3, §4.3).

- 부류(키별 조회 결과열, CF의 끝)는 36/36이 confirm2와 같다.
- 판정, 민감도 읽기(B, ctloff): 발생마다 상태·요약·근거가 F0 cur와 36/36 같다. ctloff_p1off는 F0 p1off와 36/36 같다. 즉 제어 순서를 끄면 두 백엔드의 판정이 구별되지 않는다.
- P11 결정 읽기(cur, 제어 순서 켬): 상태·요약은 ctloff와 36/36 같고 근거만 바뀐다. 바뀐 것은 §3에 있다.
- 나머지 읽기(rfoff·rfoff_p1off·idoff·cw_counted·cw_excluded)는 모두 종료 코드 0이다.

### 2.5 소진·종료·미해제 [M]

- 소진: W1·W10·W6의 12회에서 CF의 25회 재시도가 모두 실패했고, CF가 `CFE_ES_ExitApp`으로 끝났다(부류 `ends: CF=x`). confirm2와 같다. 보류가 만든 소진이므로 `timeout-exercised`로 따로 센다.
- W2(pre)는 재시도 없이 BP1 조회 1회 실패 뒤 종료했다.
- 나머지 20회는 `CF_Init`가 0을 반환했다.
- 미해제: 덤프 시 POINT `held=-` 200/200, ACTOR `held` 없음. 미해제 게이트는 없다. 안전장치도 발동하지 않았다. 그래서 P13을 적용할 실행이 없다.

### 2.6 추가 비용 [M][U]

- 제어 자원: private pthread mutex 65(제어 잠금 1 + actor별 대기 64), cond 64. 모두 정적이고, 대상 OSAL 표를 쓰지 않는다. 같은 바이너리이므로 G-I0·G-I도 같은 bss를 가진다.
- F0의 제어 비용: 실행마다 ptrace 정지 1,446–1,804회(`hold.kzr`). G-I-H는 0회다.
- 기록: dump 24,352–38,432 B. pool chunk 19 또는 22개 사용(4 MiB pool의 2%). 손실 0.
- hook 비용(G-I0/G-I/G-I-H의 시간 비교)은 재지 않았다 [U]. 위 보류 시간 차이를 hook 비용으로 읽지 않는다.

## 3. P11: 결정 읽기와 민감도 [D][M]

PD26 §3의 P11은 이렇다.
- 실제 보류·해제 사건을 **제어 실행에 한정한 순서 근거**로 쓴다.
- 원 프로그램의 동기화와 분리된 근거 종류로 표시한다.
- 보류만으로 관련 값의 부재가 입증되지는 않는다.

재생은 다음처럼 구현한다(`lib/trace/real_trace.cpp`, `lib/core/eval_trace.cpp`, `kz replay --control-order on|off`, 기본 on).

- 간선:
  - 중립 층 NCTL에서 `fire` 사유 `resumed`인 레코드마다 간선 하나를 만든다.
  - 시작은 해제 hit를 낸 actor의 사건 중, 그 hit의 관측 사건이나 그 앞에 있는 마지막 사건이다.
  - 끝은 잡혔던 actor가 같은 게이트에서 가장 최근 `gate-hold`의 관측 사건 뒤에 처음 낸 사건이다.
  - 병합 순서에서 시작이 끝보다 앞서지 않으면 간선을 만들지 않고 그 이유를 적는다.
  - 간선 규칙 이름은 `control:gate<g>/release<p>[/deferred]`다.
- 근거 종류 `control-order`:
  - 쓰기와 읽기가 프로그램 간선(N)만으로는 순서가 서지 않고,
  - N + 제어 간선(C)으로 서거나, N + RF로도 서지 않을 때 붙는다.
  - RF는 N + C로 서지 않을 때만 쓴다.
  - 그래서 제어가 정한 순서는 반환값 유도 순서나 반환 코드 좁힘보다 먼저 근거가 된다.
  - 상태 순서의 다른 조건(관측한 시작 상태, 정적 닫힘의 writer, 자원 세대)은 그대로 요구한다. 이것이 P11의 "함께 맞아야 한다"를 받는다.
- 출력:
  - basis 줄에 `control-order`가 붙는다.
  - BASIS 항목 `replay.control_order`가 있다.
  - 요약에 `realized control: records …, releases that resumed a held thread …, control edges …` 줄이 붙는다.
  - trace 출력에 `control FROM TO LABEL` 줄이 붙는다.

결과 (인스턴스당; 네 인스턴스 같음):

| 계획 | F0 cur (= G-I-H ctloff) | G-I-H cur (P11 결정) | P1 끔: F0 / G-I-H |
|---|---|---|---|
| W1 | 좁힘 / VIOLATION 25 | SO+CO / VIOLATION 25 | INCONCLUSIVE 25 / **VIOLATION 25** |
| W2 | 좁힘 / INCONCLUSIVE 1; SO+RD / NO_VIOLATION 1 | SO+CO / INCONCLUSIVE 1; SO+CO / NO_VIOLATION 1 | 같음(미판정 1은 `caller_requirement_absent`) |
| W10 | 좁힘 / VIOLATION 25; SO+RD / NO_VIOLATION 1 | SO+CO / VIOLATION 25; SO+CO / NO_VIOLATION 1 | INCONCLUSIVE 25 / VIOLATION 25 |
| W5 | 좁힘 2; SO+RD 2 | SO+CO 2; SO+RD 2 | INCONCLUSIVE 2 / 0 |
| W6 | 좁힘 1; 좁힘 / VIOLATION 25; SO+RD 1 | 좁힘 1; SO+CO / VIOLATION 25; SO+RD 1 | INCONCLUSIVE 26 / 1 |
| W7 | 좁힘 3; SO+RD 2 | 좁힘 1; SO+CO 2; SO+RD 2 | INCONCLUSIVE 3 / 1 |
| W8 | 좁힘 2; SO+RD 2 | 좁힘 1; SO+CO 1; SO+RD 2 | INCONCLUSIVE 2 / 1 |
| W11 | 좁힘 2; SO+RD 2 | SO+CO 3; SO+RD 1 | INCONCLUSIVE 2 / 0 |
| W12 | 좁힘 1; SO+RD 2 | SO+CO 2; SO+RD 1 | INCONCLUSIVE 1 / 0 |

(SO 상태 순서, RD 반환값 유도 순서, CO 제어 순서, 좁힘 = 반환 코드 좁힘(P1). 표시 없는 수는 `NO_VIOLATION_IN_CHECKED_PREFIX`.)

- 제어 순서가 근거인 발생의 수는 키마다 confirm2 `classify.awk`의 읽기 A(보류 순서 근거)가 센 수와 36/36 같다. F0에서 기록 밖 스크립트로만 읽던 것을 재생이 직접 판정 근거로 낸다.
- P1(반환 코드 좁힘)을 끄면 F0에서 `INCONCLUSIVE(horizon-open)`인 발생은 인스턴스당 86개다. 이 가운데 83개가 G-I-H 결정 읽기에서 상태 순서 + 제어 순서로 판정된다. W1·W10·W6의 위반 25씩도 여기 든다.
- 남은 3개(W6·W7·W8의 BP0 첫 실패)는 그 게이트가 `satisfied`라 제어 간선이 없는 순서다(§2.3).
- 한계: 이 근거는 제어 실행에서만 선다. 자연 실행의 가능성이나 빈도를 보이지 않는다. 간선의 끝은 잡혔던 actor가 게이트 뒤에 낸 첫 관측 사건(예: BP의 생성 호출 사건)이다. 게이트와 그 사건 사이에는 호출 명령 하나만 있다(§5 position) [F: `rtplan.txt`].
- 민감도(P11 읽기 B = `--control-order off`)는 위 "F0 cur" 열과 같다(§2.4).

## 4. 라벨, P12, P13 [D][M]

- 라벨: W1 4회는 `controlled (P-C1HOLD)`이고, 나머지 32회는 `controlled (researcher intervention, outside P-C1HOLD)`다. confirm2 라벨과 같다.
- `timeout-exercised`: W1·W10·W6의 12회다. 보류가 CF의 재시도 소진을 만든 실행이라 따로 센다(STATUS §1의 E-06 표기와 같음).
- P12: 이 36회는 예측(`predict1`·`predict2`) 뒤에 그 계획으로 돌렸다. 개발 중 시험 실행 1회는 세지 않았다. 다만 이 실행들은 **예측 확인(H3)의 횟수에 더하지 않았다.** 이유는 둘이다.
  - 예측은 F0 기준 빌드의 기록에서 나왔고, 이 실행은 파생 빌드·다른 제어 백엔드다.
  - 이 단계의 목적은 백엔드 대조다.
  - 셀지는 §8의 새 PI 결정이다.
- P13: 미해제 게이트가 남은 실행이 없다(§2.5). 런타임은 미해제를 POINT `held_actor`와 ACTOR `held`로 남기고, `kz-rt-dump`가 그것을 출력한다.

## 5. 형식 판 차이 (kzrt-plan 1.0 대 F0 kzp-hold 1) [F][D]

변환기는 해당하는 차이를 계획마다 `rtplan.txt`에 적는다. 이번 계획 36개에 적힌 것은 position, name, release, timeout 네 가지이고, 36/36에 모두 있다.

| 차이 | 런타임 | F0 | 이번 실행 |
|---|---|---|---|
| position | `caller-in` 함수 진입 점을 호출 쪽 binding(호출 직전)에서 평가 | 피호출 함수 첫 명령, 스택의 반환 주소로 모듈 판정 | 모든 게이트. 그 사이에는 호출 명령 하나뿐이다 |
| name | 런타임이 `string=validated` 슬롯에 복사한 문자열(상한 64 B)과 비교 | 멈춘 스레드 메모리에서 읽은 문자열 | 모든 이름 조건 |
| release | 해제 사이트의 관측 레코드 뒤에 `kz_rt_ctl`이 평가. 잡힌 스레드의 게이트 관측 레코드는 대기 전에 쓴다 | 명령 앞의 debug trap | 모든 해제 |
| timeout | 없음(`timeout_ns 0`) | 없음 | 모두 |
| context | ctx가 task가 아닌 게이트는 거부하고 `gate-refused`로 기록 | 문맥 검사 없음 | 해당 없음(게이트는 모두 ctx=task) |
| each gate busy | 게이트가 다른 스레드를 잡고 있을 때의 hit는 `gate-busy`로 기록하고 잡지 않음 | 잡힌 스레드 기록을 덮어씀 | 해당 없음(`each` 게이트 없음) |

H1–H6의 의미(어느 스레드든 자격 hit 계수, n번째, 같은 hit에서 게이트 먼저, 잡힌 hit의 해제는 재개 때, release 선도달과 `unless-released`, 재무장)는 F0와 같다. 근거는 `event_semantics.md` §4와 §6의 시험이다. 형식 판 차이는 위 표뿐이다.

## 6. 바꾼 도구와 시험 [D][M]

- 런타임 `kzrt 0.3.0`:
  - 직렬화한 계획 `kzrt-plan` 1.0: little-endian, 헤더 120 B, 점 172 B, crc32. manifest·원본·binding sha256을 담는다.
  - `kz_rt_plan_load`는 형식·crc·크기를 확인하고, build identity의 manifest sha256과 맞아야 싣는다.
  - dump 1.3의 PLAN 레코드는 상태, 플래그, 실은 바이트를 담는다.
  - 시작 객체가 `KZ_RT_PLAN`을 읽는다. control 모드가 아니면 `ignored`를 기록한다.
  - `kz-rt-dump`는 `plan` 줄을 내고, `--plan FILE`로 바이트를 대조한다.
  - 자세한 것은 `kairos/docs/event_semantics.md` §11에 있다.
- 변환기 `kz rtplan <plan.kzh> --manifest <logical.kzs> --binds <binds.kzs> --out <plan.kzrtp> [--record <file>]`(`probe/hold/rt_plan.{hpp,cpp}`, `tools/kz/cmd_rtplan.cpp`):
  - 변환할 수 없는 조건이 있으면 계획을 거부한다.
- 재생: `--control-order`, 제어 간선, 근거 종류 `control-order`(§3).
- C1 manifest 두 개: `control 35·45`, site 37.
- 시험:
  - kzp-hold 1 규칙마다 작은 대상 프로그램을 둔다. `tests/cfs-free/target/hold_seq_prog.c`(F0 `kz hold` 순서 시험 프로그램)를 kzgcc `-O2`와 `kzgcc_hold_seq.kzs`로 계측하고, `kz rtplan`으로 바꾼 계획을 `KZ_RT_PLAN`으로 준다. 시험은 `instrument/gcc/tests/run_hold_test.sh`다.
    - `kzgcc_hold_rearm`: H6, H4의 재개 순서.
    - `kzgcc_hold_nth`: H1·H2·H3.
    - `kzgcc_hold_defer`: H4·H5 충족.
    - `kzgcc_hold_caller`: H7 `caller-in`.
    - `kzgcc_hold_refuse`: H5 소비, ctx=terminating 게이트 거부.
  - 런타임 시험 `rt_plan_{ok,mismatch,crc,format,size,points,twice,nobuild}`, `rt_auto_plan_{control,record,none}`과 그 dump 검사.
  - 변환기 시험 `test_rt_plan_convert`(42 검사).
  - OSAL 시험 `rt_osal_gate`(`runtime/tests/test_rt_osal.c`): C1 기준 OSAL(`f34759d`)의 `libosal.a`·`libosal_bsp.a`로 링크한다. 순서는 다음과 같다.
    1. OS_API_Init 전에 부트스트랩 기록.
    2. 초기화 뒤 OSAL adapter attach. 객체는 OSAL 표에서 가져온다(mutex 2, binsem 1, RESOURCE `target-table`).
    3. OSAL task 하나를 게이트에서 잡고, 다른 task의 해제로 재개한다.
    - `KZ_RT_OSAL_INCLUDE_DIRS`·`KZ_RT_OSAL_LIBS`를 줄 때만 빌드한다.
- 결과:
  - 기본 빌드 179개 중 178 통과 + 1 건너뜀(tracefs).
  - LLVM 빌드 201개 중 200 + 1 건너뜀.
  - plugin 빌드(`KZGCC_KZ` 지정) 14/14.
  - OSAL을 준 런타임 빌드 60/60.
  - 같은 작업 트리의 다른 진행 중 변경의 시험도 이 수에 들어 있다.

## 7. 보여 주는 것과 보여 주지 않는 것

- **보여 줌**:
  - 컴파일러가 넣은 협조 게이트가 F0 보류기(ptrace·debug register)와 같은 kzp-hold 1 계획을 C1 9개 계획 × 4 인스턴스에서 같은 hit로 실현했다. ptrace 정지는 없었다.
  - 결과 부류와 판정(제어 순서를 끈 읽기)이 F0 confirm2와 같다.
  - P11 결정 읽기에서 제어 실행의 위반·미준비 판정 대부분이 반환 코드가 아니라 기록된 제어 순서 위에 선다.
- **보여 주지 않음**:
  - 자연 실행에서의 가능성이나 빈도.
  - 반복 횟수에 따른 안정성(각 1회).
  - hook 비용.
  - OSAL adapter로 cFS를 제어한 실행. C1 실행은 POSIX adapter이고, OSAL adapter는 단위 시험에서만 돌았다.
  - R2·다른 사례의 제어.
  - 새 결함 기전.
  - E-06의 소진은 보류가 만든 것이다(`timeout-exercised`). PR #342의 재시도 상한은 의도된 것이다.

## 8. 남은 것과 PI 결정

- 도구(결정 아님):
  - `each` 게이트 busy의 형식 판 정리(`event_semantics.md` §4).
  - hook별 비용 계수(G-I0/G-I/G-I-H 시간 비교).
  - cFS 안에서 OSAL 초기화 뒤 OSAL adapter로 바꾸는 연결부. 지금 C1에는 그런 hook이 없어 POSIX adapter를 쓴다. 바꾸면 OSAL 객체 소비를 계상한다.
  - 시그널·강제 종료 경로의 dump.
  - 작업 트리 C1 프로필이 바뀐 뒤의 B′·F0 닫힘 재추출과 재재생. 지금 결과는 `97a24d8` 프로필 스냅숏의 것이다.
- 기존 대기(결정하지 않음): 파생 실행의 R2 시작 상태, 군 비교의 반복 횟수(단계 E), kzgcc 라이선스, 산출물 공개 범위.
- **새 PI 결정 1건: G-I-H 실행을 예측 확인(H3)으로 셀 수 있는가.**
  - 레퍼런스가 정하지 않는 이유:
    - P12는 실행을 예측 전과 뒤로만 가른다.
    - CONDITIONS_POLICY는 파생 빌드 실행을 pilot/dev 또는 제어로 두라고만 한다.
    - 어느 쪽도 기준 빌드의 기록에서 나온 예측을 파생 빌드·다른 백엔드의 실행이 확인할 수 있는지는 정하지 않는다.
  - 권고: 세지 않고 백엔드 대조로만 둔다. confirm2가 이미 같은 계획 9개를 기준 빌드에서 확인했다.

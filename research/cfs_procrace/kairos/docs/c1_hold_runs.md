# C1 보류 실행 (kz hold, F0, SPEC §4.13) — 단계 B·C (2026-10-25)

- 지위: **controlled**. 단계 B는 `controlled (P-C1HOLD)`, 단계 C는 `controlled (researcher intervention, outside P-C1HOLD)`(`Architecture/spec/PI_DECISIONS_RESOLVED_20261025.md` §2 C) [D]. 자연 실행(natural pilot)·build-smoke 결과와 섞지 않는다. 빈도·재현성·포괄성은 주장하지 않는다(P8). tx·rx는 별개 인스턴스이며 별도 기록이다(P7); 같은 이름의 OSAL 객체를 두 인스턴스 사이에서 합치지 않고, 두 인스턴스를 독립 버그 둘로 세지 않는다.
- 도구: `kz hold`(`tools/kz/cmd_hold.cpp`, `probe/hold/`). PTRACE_SEIZE + 디버그 레지스터(DR0–DR3, `PTRACE_POKEUSER`) 실행 중단점으로 **텍스트를 수정하지 않고** 한 스레드만 보류하고, 관측된 사건에서 해제한다. 나중에 생성되는 스레드는 `PTRACE_O_TRACECLONE`로 자동 연결한다. int3 텍스트 패치 대체는 쓰지 않았다(쓸 필요가 없었다; 디버그 레지스터가 이 VM에서 동작한다, 아래). [M]
- 산출물(저장소 밖): `/home/user/work/procrace/c1_runs/hold1/<단계><cfg>_<cand>_<inst>/`. 각 디렉터리에 `hold.kzr`(보류 기록·섭동), `run.kzt`(관측 컨테이너)+`run.kzt.record.txt`, `hits.txt`, `replay.{txt,kz}`, `conditions.tsv`, `driver.txt`(명령·해시), `cf_console.txt`, `inst/<inst>/console_output.log`. 전체 14 MB, 디스크 여유 7.7 GB. 저장소(`plan/expected_results_C1_R2.md` 등)는 고치지 않았다.
- `kz` sha256 앞 16자리 `9cba898ad7b145bd`(빌드 `/home/user/work/kairos-build`). 저장소 커밋은 각 `driver.txt`에 적혔다.

## 1. 기전과 소도구 (F0 `kz hold`) [M]

- 구조: `kz hold <core basis> <observe profile> <hold plan> --basis cf.so --basis bp.so -o hold.kzr -- kz observe … -- <한 인스턴스 시작/정지>`. 즉 **kz hold(ptrace, root)가 kz observe(perf uprobe)를 자식으로 돌리고**, 포크·클론·exec 트리를 따라 cFS 프로세스(core-tx/core-rx)와 그 앱 스레드까지 내려간다. perf uprobe(관측)와 ptrace(보류)가 **같은 프로세스에서 동시에 동작**함을 확인했다 — SPEC §4.13이 미측정으로 둔 ptrace×uprobe 상호작용을, 이 동시 실행의 섭동으로 측정했다(아래 holder cost).
- 게이트/해제 주소: observe 프로필(`observe/sites-cf<REV>.kzp`)의 사이트 라벨을 hold plan(`hold/hold-cf<REV>.kzh`)이 지목하고, 주소는 해당 basis의 TARGETS(파일 오프셋)에서 온다. 실행 시 `/proc/<pid>/maps`로 적재 주소를 구한다(cf.so/bp.so는 시작 스크립트가 적재한 뒤 매핑되므로 그때 해제·무장). 조건: 이름 일치(인자 레지스터가 가리키는 문자열을 ptrace로 읽음)와 호출자 모듈 일치(스택 꼭대기 반환 주소가 그 모듈 범위인지).
- 소프로그램 검증(SPEC §4.13 "Reuse conditions": 먼저 작은 2-스레드 프로그램): `tests/cfs-free/test_hold_ptrace.cpp` + `target/hold_prog.c`. 나중에 생성된 provider 스레드를 자동 연결·무장하고, 이름 "OBJ0"의 생성만 보류(이름 "OBJ1"은 걸러 통과), consumer가 해제 사건에 닿을 때 해제, 디버그 레지스터 사용, 자식 정상 종료를 확인한다. ctest 통과. [M]
- 디버그 레지스터 가용성 [M]: 이 VM(커널 6.18.44-fc-v153, root)에서 `PTRACE_SEIZE`·`PTRACE_O_TRACECLONE`·DR0 실행 중단점·새 스레드에의 DR 무장이 모두 동작한다(소프로그램과 8개 cFS 실행 모두 `debugregs_ok=yes`). 실패한 것은 없다.

## 2. 게이트·해제 지점과 출처, 게이트에서의 잠금 [F]

줄 번호는 §2의 사본(`prov_roots/<cfg>_<cand>`) 기준. 주소는 core-tx 링크 시점 vaddr(= 파일 오프셋).

| 단계 | 게이트 | 해제 | 비고 |
|---|---|---|---|
| **B** (post `833fdbb`) | `osal.countsem_create.entry` core-tx `0x37c94`, 이름 `sem_name=="BP0_tsem"`(인자 rsi) | `es.exit_app.entry` core-tx `0x17584`, 호출자 cf.so | provider(BP)를 `OS_CountSemCreate("BP0_tsem")` 진입 직전에 보류. BP는 한 스레드에서 BP0→BP1 순서(`bp_flow.c` L124–142)이므로 두 이름이 모두 미생성 → CF 두 채널 조회가 모두 실패 = E-06 소진. 해제는 CF가 소진 후 `CFE_ES_ExitApp`을 부를 때(최종 초기화 결과, C-6/C-7; 계획 §9.2 R-d) [D] |
| **C** (pre `281a941`) | `osal.countsem_lookup.entry` core-tx `0x37fc4`, 호출자 cf.so | `osal.countsem_create.entry` core-tx `0x37c94`, 이름 `sem_name=="BP1_tsem"` | consumer(CF)를 조회 직전에 보류(호출자가 cf.so일 때만 — BP의 생성 전 존재 확인은 bp.so 호출이라 보류하지 않음). 해제는 BP가 `OS_CountSemCreate("BP1_tsem")`에 진입할 때 = BP0_tsem 생성 호출이 이미 반환했다는 관측 증거(같은 루프 순차, `bp_flow.c` L124–142). 과제 문구 "BP의 해당 이름 OS_CountSemCreate가 반환할 때"를 이름으로 거를 수 있는 지점으로 표현한 것 [I] |

**게이트에서의 잠금 [F].** 두 게이트 모두 OSAL API **함수 진입(endbr64 다음, 프롤로그 push 전)** 이다 — 함수 본문이 객체 표 잠금을 잡기 전이다. 따라서 보류되는 스레드는 그 시점에 OSAL 객체 표의 전역/독점 잠금을 **쥐고 있지 않다**. BP의 `initialize_throttling`(`bp_flow.c` L121–144)은 조회·생성을 감싸는 앱 잠금을 쥐지 않고(앞 조회는 반환되어 그 OSAL 내부 잠금을 이미 풀었다), CF의 `CF_CFDP_InitEngine`(`cf_cfdp.c` L991–1012@833, L991–1001@281a941)도 조회를 감싸는 앱 잠금을 쥐지 않는다. 즉 상대가 필요로 하는 잠금을 게이트에서 쥐지 않는다(SPEC §4.13 "Gate placement" 항목 1 "program-lock set empty", 항목 3 "RESERVED 창 밖"; 계획 §9.2 "G(name)") [D]. 디버그 레지스터는 스레드별이므로 **그 스레드만** 정지했다.

## 3. holder cost와 섭동 (ptrace×uprobe, SPEC §4.13) [M]

8개 실행 모두 `debugregs_ok=yes`, 자식 종료 코드 0. 전체 트리에 대한 ptrace 정지는 많지만(아래) 보류된 스레드는 하나뿐이다. 보류 길이(holder cost)는 사건 기반이며, 어느 것도 벽시계 상한에서 유도하지 않았다.

| 실행 | 보류 길이 (ns) | 전체 정지 | 연결 스레드 | clone 연결 | 게이트 trap | 해제 trap |
|---|---:|---:|---:|---:|---:|---:|
| B_post_1df262a_tx | 2,496,653,749 | 1717 | 494 | 22 | 2 | 1 |
| B_post_1df262a_rx | 2,497,164,983 | 1682 | 484 | 22 | 2 | 1 |
| B_post_1b0b338_tx | 2,495,209,733 | 1696 | 488 | 22 | 2 | 1 |
| B_post_1b0b338_rx | 2,498,555,866 | 1871 | 538 | 22 | 2 | 1 |
| C_pre_1df262a_tx | 21,097,356 | 1846 | 530 | 22 | 3 | 3 |
| C_pre_1df262a_rx | 11,916,926 | 1412 | 406 | 22 | 3 | 3 |
| C_pre_1b0b338_tx | 13,210,127 | 1412 | 406 | 22 | 3 | 3 |
| C_pre_1b0b338_rx | 11,455,634 | 1629 | 468 | 22 | 3 | 3 |

- 단계 B의 보류 길이 ≈ **2.50 s**는 CF가 BP0_tsem을 25회 조회·재시도하는 동안 BP를 잡아 둔 시간이다(관측에서 `cf_retry_delay` 25회로 확인). 이것은 **보류가 만든 길이**이며, 재시도 수×지연(25×100 ms)을 벽시계 상한으로 단정한 값이 아니다(SPEC §4.13, 결정문 §3.3) [D]. 
- 단계 C의 보류 길이 ≈ **11–21 ms**는 CF 조회를 BP가 BP0_tsem을 만들 때까지 잡아 둔 시간이다.
- `holder stop/resume timestamp`는 각 `hold.kzr`의 `held_ns`/`release_ns`에 있다. 전체 ptrace 정지(1400–1870회), 연결 스레드(400–540), clone 연결(22)은 perf 관측과 **동시**에 일어난 섭동이다. 이 수치가 ptrace×uprobe 상호작용의 측정값이다. 관측 없는 기준 실행과의 정밀 비교(관측 영향 비율의 분모)는 `C1B-U18`에 따라 아직 정하지 않았다(파일럿 측정 대상).
- 안전장치: 전체 실행을 `timeout 150 s`로 감쌌다("pilot safeguard, researcher choice"); 8개 실행 모두 이 안전장치가 발동하지 않았다(정상 종료). 발동했다면 그 실행은 INCONCLUSIVE였을 것이다(`CONDITIONS_POLICY.md`).

## 4. 단계 B 결과 (E-06, controlled per P-C1HOLD) — 인스턴스별 [M]

post `833fdbb` × 번들 {1df262a, 1b0b338} × {tx, rx} 4개. 네 실행의 관측·사건열이 일치한다.

- 관측(`hits.txt`, run.kzt): `cf_lookup_call` **25**, `cf_retry_delay` **25**, `osal.countsem_lookup.entry` 27(CF 25 + BP 존재 확인 2), `es.exit_app.entry` **1**, `bp_create_call` 2, `OS_CountSemCreate.publish` 3. 손실 0.
- 사건 타임라인(console + hold.kzr): BP를 `OS_CountSemCreate("BP0_tsem")` 직전에 보류 → CF가 채널 0 `BP0_tsem`을 25회 조회·재시도, 모두 `OS_ERR_NAME_NOT_FOUND`(-17) → CF 이벤트 30 "CF: failed to get sem id for name BP0_tsem, error=-17" → `CFE_ES_ExitApp(Application CF)` 호출(= 최종 초기화 실패, 소진) → **그 사건에서 BP 해제** → BP가 BP0_tsem·BP1_tsem을 뒤늦게 생성("Created semaphore"). 채널 1(`BP1_tsem`)은 채널 0 소진 뒤 `break`로 조회되지 않음(E-09 일치).
- 판정: **E-06 소진 경로를 실제 프로그램에서 관측했다.** 기대 결과표 E-06의 "provider가 늦어 25번 모두 실패"가 실현됐다. 단, `VIOLATING`은 채택 모델(G-REBIND로 묶은 caller 기록, 상한 25)의 판정이고 PR #342가 의도적으로 상한을 둔 사실과 함께 해석한다(E-06 해석 주의, 라벨 불변). 강제 보류로 소진을 만든 실행이므로 자연 빈도·영구 교착의 증거가 아니다. `kz replay`(현행 kz)로 재생했다; 판정 규칙은 Integrate 에이전트가 새 규칙으로 재재생한다.
- 하네스가 끝내 반복이 열린 채 종료된 것이 아니라 **CF 자신이 재시도 루프를 끝냈다**(소진 후 ExitApp). 안전장치 미발동. 따라서 소진으로 센다.

## 5. 단계 C 결과 (controlled: researcher intervention, outside P-C1HOLD) — 인스턴스별 [M]

pre `281a941` × 번들 {1df262a, 1b0b338} × {tx, rx} 4개. 네 실행 일치.

- 사건 타임라인: CF를 (cf.so 호출자) 조회 직전에 보류 → BP가 `OS_CountSemGetIdByName`(존재 확인, 실패) 후 `OS_CountSemCreate("BP0_tsem")` 생성, 이어서 `OS_CountSemCreate("BP1_tsem")` 진입 → **그 사건에서 CF 해제** → CF 조회가 이제 **성공**, CF가 채널 0·1 모두 조회 성공(`cf_lookup_call` 2, 둘 다 성공) → **"CF Initialized"**, `es.exit_app.entry` **0**(CF가 종료하지 않음). 손실 0.
- 판정: **같은 바이너리·설정(pre-fix `281a941`)에서, 관련 작업 순서만 제어하면 CF의 조회가 READY에 이른다.** 즉 수정 전 실패는 CF 조회 로직의 결함이 아니라 **실행 순서 의존**이다. 값·앱 로직·자원 이름은 바꾸지 않았다(디버그 레지스터 보류/해제만). 라벨 `controlled (researcher intervention)`.
- 반대 순서(자연 실행)는 이미 build-smoke(smoke1–3, pre 4/4)에 기록돼 있다: 자연 pre 기동에서는 CF 조회가 BP 생성보다 앞서 `-17`로 실패(NOT_READY_UNJUDGED → `INCONCLUSIVE(caller_requirement_absent)`, `c1_smoke_observe.md` §9.3). 따라서 **같은 구성에서 좋은 순서(제어, READY)와 나쁜 순서(자연, 실패)를 모두** 확보했다. 단, 이 둘은 `controlled`와 `natural`로 라벨이 다르며 같은 집계에 합치지 않는다. 순서 의존의 "같은 순번 좋은/나쁜 증거"(SPEC App. A.4 step 6)에 대한 최종 주장은 Integrate 단계에서 자연/제어 라벨을 지킨 채 정리한다.

## 6. 명령 (대표) [M]

```sh
# tracefs(운영자가 마운트; kz hold는 ptrace로 읽으므로 보류에는 불필요, 내부 kz observe가 쓴다)
mount -t tracefs nodev /home/user/work/procrace/c1_runs/tracefs_hold1
# 단계 B (post) / 단계 C (pre), 인스턴스마다 1회:
kairos/probe/scripts/observe_c1_hold.sh <pre|post> <1df262a|1b0b338> <tx|rx> \
    /home/user/work/kairos-build/tools/kz/kz \
    --out-dir /home/user/work/procrace/c1_runs/hold1 \
    --tracefs /home/user/work/procrace/c1_runs/tracefs_hold1
umount /home/user/work/procrace/c1_runs/tracefs_hold1
```

- observe 프로필: `profiles/c1-bp-b051eb0/observe/sites-cf833fdbb.kzp`(B) / `sites-cf281a941.kzp`(C, sha256 `d375d853…`). hold plan: `profiles/c1-bp-b051eb0/hold/hold-cf833fdbb.kzh`(sha256 `f1215c23…`) / `hold-cf281a941.kzh`. 각 실행의 basis·plan·소스뷰 SHA와 cFS 빌드 해시는 그 `driver.txt`에.
- run.kzt sha256 앞 16: B_post_1df262a_tx `6c6c2fdb89dc0076`, C_pre_1df262a_tx `949c04e8949cc895`.
- 재생은 현행 프로필로 base 재생(정적 닫힘 `--facts`는 C1 프로필을 다른 에이전트가 동시에 개정 중이라 해시가 어긋나 생략; Integrate 에이전트가 새 규칙·새 닫힘으로 재재생).

## 7. 남은 것 / 새 PI 결정 [U]

- 단계 D(예측)는 아직 하지 않는다(과제 지시).
- 관측 영향 비율의 분모(`C1B-U18`, SPEC §4.13 "the code-derived separation")는 레퍼런스가 정하지 않아 미정 — 비율을 계산하지 않았다. 보류 없는 기준 실행과의 내부 사건 비교는 파일럿 측정 대상(PV-7, PV-8).
- 단계 C 해제의 "BP1_tsem 생성 진입 = BP0_tsem 생성 반환" 대체 표현은 순차 루프 출처(`bp_flow.c` L124–142)에 근거한 연구자 해석 [I]; 레퍼런스가 "생성 반환" 사건을 이름으로 직접 거를 지점을 정하지는 않는다(새 PI 항목이 아니라 표현 선택, 출처로 정당화됨).

## 8. 보류 계획 형식·의미 개정 (kzp-hold 1, confirm2, 2026-10-25) [M]

§1–§7 의 실행(hold1)과 confirm1 은 개정 전 의미로 돌았다. 아래는 그 뒤의 `kz hold`·`kz predict` 에만 적용된다. 디버그 레지스터 실행 중단점 설계는 그대로이고 텍스트는 수정하지 않는다(int3 대체 없음).

```
gate    <label> site=<site> [when <cond>] [occurrence first|each|<n>] [unless-released]
release <label> site=<site> releases <gate> [when <cond>] [occurrence first|each|<n>]
```

- **재무장(L384–L385 결함 수정).** 이전에는 해제한 스레드의 DR7 을 0 으로 지워, 그 스레드가 다음 ptrace 정지까지 어떤 게이트·해제에도 작용하지 못했다(pre W2·post W10 전체 계획이 실현 불가였던 이유). 이제 디버그 레지스터를 무장한 채 resume flag(RF)로 게이트 명령만 한 번 넘긴다. 풀린 스레드의 뒤 hit 는 다른 스레드의 hit 와 똑같이 작용하고, 이미 작용한 게이트(first·n)는 그 스레드를 다시 잡지 않는다.
- **`occurrence <n>`**(1–1000000, `first` = 1). 점마다 자격 hit(주소 도달 + 조건 충족, 어느 스레드든, 실행 시작부터)를 세어 n 번째 하나에만 작용한다. 재시도 m ≥ 2 의 특정 시도(예: CF 의 3번째 `BP0_tsem` 조회 진입, 2번째 조회 반환)를 고르는 데 쓴다.
- **한 주소의 여러 점.** 슬롯은 서로 다른 주소마다 하나(넷 이하)이고, 그 주소의 점을 모두 평가한다: 게이트 먼저(계획 순서; 처음 작용하는 게이트가 잡고, 같은 hit 에 작용하는 다른 게이트는 보류 없이 소비), 다음 해제. 그 hit 에서 스레드가 잡히면 그 hit 의 해제는 스레드가 재개될 때 작용한다(`deferred`).
- **`unless-released`**(게이트만, `each` 와 함께 쓰지 않음). 게이트가 스레드를 잡고 있지 않을 때 그 해제가 작용하면 게이트는 충족(satisfied)되어 이후 잡지 않는다: 게이트가 강제하려는 순서(게이트 단계가 해제 사건 뒤)가 이미 성립했기 때문이다. 없으면 이전처럼 해제는 소비되고 게이트는 나중 hit 를 끝까지 잡는다(confirm1 의 pre `W2-1`, post `W5-2`·`W6-1` 정체가 이것이다). `kz predict` 는 이제 모든 게이트에 이것을 쓴다.
- **기록(`hold.kzr`).** 제어점 줄에 `occurrence=`·`unless-released`·`qualifying_hits=`, 보류 줄에 `hit=`, 새 `fire` 줄(해제마다 쏜 스레드·hit·재개한 스레드, 또는 `resumed=none satisfied=yes|consumed=yes`, `deferred=yes`).
- **시험** [M]. `tests/cfs-free/test_hold_sequence.cpp` + `target/hold_seq_prog.c`(공급자·소비자 두 스레드, 두 키, 재시도·지연 인자; 단위 시험 값): (1) `hold_rearm.kzh` — 소비자를 첫 조회에서 잡았다가 공급자의 첫 생성 사건에서 풀고, 공급자를 둘째 생성에서 잡아 **풀린 소비자**의 종료 사건으로 푼다. 개정 전 의미(재개 때 DR7 지움)로 잠시 되돌려 돌리면 실행이 끝나지 않았다(25 s 에서 중단; 되돌린 코드는 커밋하지 않음). (2) `hold_nth.kzh` — 해제 `occurrence 3`(소비자의 3번째 실패 사건)이 공급자를 풀고(소비자의 첫 세 조회는 반드시 실패), 같은 주소의 이름 조건 없는 게이트 `occurrence 2` 가 둘째 생성만 잡는다. (3) `hold_defer.kzh` — 보류된 hit 의 해제가 재개 때 작용(`deferred=yes`)하고, 그때 잡은 것이 없는 `unless-released` 게이트가 충족된다(`satisfied=yes`, 공급자는 끝까지 잡히지 않음). 문법: `tests/cfs-free/test_hold_plan.cpp`. ctest: 기본 빌드 115 중 114 통과 + tracefs 시험 1 건너뜀, LLVM 빌드 136 중 135 + 1 건너뜀.

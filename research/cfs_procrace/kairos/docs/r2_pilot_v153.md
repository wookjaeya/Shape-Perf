# R2 파일럿 실행 기록 (커널 v153, W2, 2026-10-24)

- **지위: pilot/dev 실행 한 번. 본 실험 결과가 아니다.** 고정 본 실행 묶음의 반복 수는 레퍼런스가 정하지 않는다(`unspecified_by_reference`). 반복 수는 PI 결정(D14)으로 남긴다. 이 기록은 과거 개발 기록(s1r2_p1, gdb r2_g1–g3)과 합치지 않는다. 비용·빈도·재현성은 주장하지 않는다.
- 목적: W2(마스터플랜 W2, §3.2, F01, F05) — X-03 관측점과 생성 측 `app_owner` 를 실제 실행 하나에 걸어 본다. 경로는 정규화(ACTOR_OWNERS 포함) → 기록 판정 → 요약 뷰이다.
- 표기: [M] 이번 실행에서 확인, [F] 소스·문서의 사실, [I] 추론, [D] 결정.

## 1. 명령

저장소 commit `e37e76c`, `kairos/` 미커밋 차이 없음(`v153_pilot1_source.diff` 빈 파일). kz 는 `/home/user/work/kairos-build`(RelWithDebInfo, gcc 13.3.0) 빌드, SHA-256 `707884f8…`, 컨테이너의 도구 해시 `0dc20210…6fee`. root 에서 실행했다:

```sh
CFS_NORMAL_USER=ubuntu research/cfs_procrace/kairos/probe/scripts/observe_cfs_smoke.sh v153_pilot1 \
    /home/user/work/kairos-build/tools/kz/kz 0 --out-dir /home/user/work/procrace/r2_runs/v153_pilot1 --purpose pilot
kz replay v153_pilot1.kzt --profile research/cfs_procrace/kairos/profiles/cfs-v7.0.1 \
    --const CFE_PLATFORM_ES_MAX_APPLICATIONS=32 > v153_pilot1_replay.txt
```

스크립트 안의 단계(앵커, 조건 파일, `kz observe -- env/run_cfs.sh operational ci-es-restart-poweron`, 영속 상태 전후, `kz hits`, 앵커 교차 확인)는 `observer_smoke_run.md` §1·§9 와 같다. 상수 32 는 고정 소스 추출 golden 의 값이다(`kairos/tests/static/data/core_cpu1.golden:44`) [F].

## 2. 문서화된 절차와 다른 점

| 항목 | 문서(`observer_smoke_run.md` §9) | 이번 | 이유 |
|---|---|---|---|
| 출력 위치 | `env/logs/<tag>_*` | `/home/user/work/procrace/r2_runs/v153_pilot1/` | 작업 규칙: 저장소 밖, 과거 기록과 분리. 스크립트에 `--out-dir` 인자를 더했다. 환경 변수로 주지 않은 것은 스크립트 환경이 cFS 로 넘어가기 때문이다 |
| 실행 목적·횟수 | S1-R2 기능 확인 3회 중 하나(`run.count` deviation) | `--purpose pilot`: `run.purpose` = pilot/dev, `run.count` = `unspecified_by_reference` | 반복 수는 PI 결정 사항이다. 조건 파일의 첫 주석 줄에 "functional check" 문구가 남은 것은 스크립트 실수이다. 항목 `run.purpose` 를 기준으로 읽는다(이후 스크립트에서 고침) |
| 계획 | 사이트 8개 | 26개(0–7 같음, 8–20 X-03, 21–25 `app_owner`; `plan/proof_status_R2.md` §12) | W2 의 과제 자체 |
| 커널 | v114(s1r2_p1), v80(기준 gdb) | `6.18.44-fc-v153` | VM 커널이 바뀌었다. D11 에 따라 다시 쟀고 이전 커널의 값을 쓰지 않는다 |

나머지는 같다: 바이너리(`core-cpu1` SHA-256 `0edb1ae2…7799`, 수정 없음, 26 사이트 바이트 대조 통과), cFS 사용자 `ubuntu`(`setpriv`), 관측기 root(D1), 모드 `operational`, 중지 `ci-es-restart-poweron`, `/proc` 읽기 `full`(62회, 2.56 ms), 층 `none`(확립 안 됨), ring CPU 마다 2 MiB.

## 3. 조건과 시작 상태 [M]

- 시작 2026-10-24T16:50:54Z. 호스트는 4 vCPU, `clocksource=tsc`, `perf_event_paranoid=2` 이다. 실행 직전 영속 상태는 다음과 같다(`v153_pilot1_persist_before.txt`).
  - `EEPROM.DAT` 는 전부 0 이다.
  - 열쇠 파일 셋은 크기 0, SysV shm 은 없다.
  - `/dev/shm/osal:RAM` 은 빈 디렉터리로 있었다. 15:53 에 만들어졌으며 이 절차 밖의 이전 cFS 실행이 남긴 것이다. 정리하지 않았다.
- 콘솔: `POWER ON reset` 으로 시작했다. OPERATIONAL 줄을 3308 ms 에 읽었고 1 ms 뒤 ES 재시작 명령(종료 0)을 보냈다. 프로세스 종료 0, `POWERON`.
- 3308 ms 는 이 실행 하나의 값이다. 계획·커널이 다르므로 이전 값(1663 ms, gdb 2244–2693 ms)과 비교해 관측 비용을 말하지 않는다.
- 환경: cFS 가 받은 변수는 153개이다. 기준 실행과 이름 집합이 달라 `cfs.environment` deviation 으로 적혔다. 기록에는 이름만 있고 값은 없다(`<value not recorded>`).

## 4. 관측과 정규화 [M]

| 항목 | 값 |
|---|---|
| 손실 | `PERF_RECORD_LOST` 0, throttle 0, 스트림 오류 0("커널이 손실을 보고하지 않았다"; 관측 완전성의 증명이 아님) |
| 표본·사건 | 1084(사이트 없는 표본 0, 슬롯 실패 0), 호출 짝 362, 짝 없는 진입·복귀 0 |
| 사이트별 | `evs.send` 275/275, `sb.appid.store` 1, `es.sync` 7/7, `osal.task_create` 24/24, `osal.task_entry` 24, `evs.register` 19/19, `clear_slot` 19, `register` 19, `slot_registered` 251, `es.wait` 9/9, `raise_state` 8, `reset_records`·`reset_states` 1·1, `reserve_core` 5, `publish_core` 5, `core_clear` 0, `es.start_task` 24/24, `es.child_task` 4/4, `osal.task_create.id` 26 |
| thread | 27 |
| `app_owner` | **confirmed 24, inferred 0, undetermined 3**. 24개 모두 자식이 알린 OSAL id(0x10001–0x10018)와 생성 호출이 배정한 id 가 같다(`ids_matched` 24, `matched_by_time` 0, `ids_unverified` 0, 모순 0) |
| 앱 키 | 핵심 앱 5개(`CFE_ES` 0x110001 … `CFE_TIME` 0x110005)와 외부 앱 15개는 요청 인자로 정해졌다. 자식 task 4개(`ES_BG_TASK`, `TIME_TONE_TASK`, `TIME_ONEHZ_TASK`, `FM_CHILD_TASK`)는 생성자 task 의 소유 앱으로 정해졌다(`creator-owner`). 겹친 `ParentAppId` 는 쓰지 않았다(`analysed-table-excluded`) |
| undetermined 3 | 프로세스 주 thread(`core-cpu1`)와 이름 없는 thread 둘이다. 모두 프로브된 생성 호출 없이 생겼다(`raw-thread`). `spawn_ids_outside_create` 2 는 이 둘을 만든 OSAL 내부 thread 생성이다 [I: 생성 호출 밖의 `OS_Posix_InternalTaskCreate_Impl`] |

네 정체성 범주는 따로 남는다. 예를 들어 `TO_LAB` 이 SB 함수를 부른 사용은 다음과 같다. 실행 task `TO_LAB`, 생성 실행자 `core-cpu1`, 소유 앱은 confirmed 레코드 8(0x110008), 전달된 AppID 는 0x110003(SB)이다.

## 5. 기록 판정과 요약 뷰 [M]

`kz replay` 의 발생은 275개이다. 상태 VIOLATING 24, READY 251 이다. 요약은 `OBSERVED_VIOLATION` **24**, `NO_VIOLATION_IN_CHECKED_PREFIX` **251**, `INCONCLUSIVE` 0, 입력 오류 0, `partial` 아님이다.

- **251개는 모두 `outcome-narrowed` 이다.** 등록 조건의 LP 통과가 관측되었고(`slot_registered` 251), 그 값은 반환 코드(`CFE_SUCCESS` → ready, O4)로만 고정되었다. 상태 재생은 `evs.reg` 를 열린 채 둔다. 이 251개의 판정은 W1 의 PI 결정 사항("반환 코드만으로 열린 조건을 고정해도 되는가")에 그대로 기댄다. 그 결정이 "아니오"이면 251개는 `INCONCLUSIVE` 이다. 이유는 아래 `evs.reg` 의 두 사유이다.
- 종류별 적합성(`real record knowledge`):

| 종류·위치 | 세운 것 | 보류 사유 |
|---|---|---|
| `evs.reg` | start-state-known, **probes-attached, probes-active**(Φ 의 writer 지점 둘 다 관측), loss-free | `effect_unsupported`(writer·별칭 포괄 미확립: 정적 writer 4개 중 Φ 효과 2개, `plan/proof_status_R2.md` §12), `resource_or_lifetime_unmatched`(수명 미확립) |
| `es.app`, `es.appstate`, `es.shutdown` | start-state-known, loss-free | 같은 두 사유. writer 지점 일부 미관측(AppCreate·CleanUpApp), 효과 값(`lp.slot`, `lp.pending_id`, `lp.required`) 미판독 |
| `CFE_SB_Global.AppId` | start-state-known, lifetime-known, loss-free | `effect_unsupported` |

- 판정 찾기 결과 25건:
  - `R2-P-TASK` 24건: `OS_TaskCreate` 의 복귀에 `out.task_id` 가 없어 생성 간선을 만들지 못했다. 배정 id 는 사이트 25 가 읽지만, 어댑터는 아직 그 값을 복귀 사건으로 옮기지 않는다. (W2 잔여에서 해소: §5.2)
  - `R19` 1건: 버전이 주어지지 않아 적용하지 않았다.
- 실행 관계: 요약의 `relation` 은 여전히 `ownership-undetermined` 이다. 실행 관계 계산은 W1 에서 남긴 일이다. (W2 잔여에서 계산: §5.2)

### 5.1 R2 발생별 요약 뷰

공통 열(24개 모두 같음):
- 상태 `VIOLATING`, 호출에서 결정, 범위 default.
- `id_valid` 는 호출의 인자(capture)에서 거짓이다.
- `slot_registered` 는 `not_passed` 이다.
- 결과 `0xc2000003`(`CFE_EVS_APP_ILLEGAL_APP_ID`)이고 결과 대응이 일치한다.
- 요약 `OBSERVED_VIOLATION` {real-record}. 적합성은 값 조건이 인자만 읽으므로 전부 세워졌다.
- 정체성:
  - 생성 실행자는 `core-cpu1` 이다.
  - 소유 앱은 confirmed 이다. `CFE_ES` 는 레코드 1(0x110001), `CFE_EVS` 는 레코드 2(0x110002)이다.
  - 전달된 AppID 는 0 이다.

| 실행 task # 순번 | 호출 / 반환 사건 |
|---|---|
| CFE_ES #1 | #12 / #13 |
| CFE_ES #2 | #14 / #15 |
| CFE_ES #3 | #16 / #17 |
| CFE_ES #4 | #18 / #19 |
| CFE_ES #5 | #20 / #21 |
| CFE_ES #6 | #22 / #23 |
| CFE_ES #7 | #24 / #25 |
| CFE_ES #8 | #26 / #27 |
| CFE_ES #9 | #28 / #29 |
| CFE_ES #10 | #30 / #31 |
| CFE_ES #11 | #32 / #33 |
| CFE_ES #12 | #34 / #35 |
| CFE_ES #13 | #36 / #37 |
| CFE_ES #14 | #38 / #39 |
| CFE_ES #15 | #40 / #41 |
| CFE_ES #16 | #42 / #43 |
| CFE_ES #17 | #44 / #45 |
| CFE_ES #18 | #46 / #47 |
| CFE_ES #19 | #48 / #49 |
| CFE_ES #20 | #50 / #51 |
| CFE_EVS #1 | #68 / #69 |
| CFE_EVS #2 | #70 / #71 |
| CFE_EVS #3 | #72 / #73 |
| CFE_EVS #4 | #74 / #75 |

`kz hits` 의 묶음은 기준과 같다. ES 20 + EVS 4, `app_id` 0, 상태 `0xc2000003` 24, 호출 함수 `CFE_SB_CreatePipe` 2 / `CFE_SB_SubscribeFull` 4 / `CFE_SB_MessageTxn_ReportSingleEvent` 18, event id 5 / 10 / 14 = 2 / 4 / 18 이다. "24개가 `sb.appid.store` 보다 앞섬"은 한 시계의 병합 순서이고 happens-before 가 아니다(SPEC §5.8). 전체 진입은 275개이다. s1r2_p1 271, gdb 251/257/257 과 다르다. 원인은 조사하지 않았고 맞추려고 아무것도 바꾸지 않았다.

### 5.2 W2 잔여 재생: 실행 관계, 생성 id 결합, 출력 컨테이너, P1 두 읽기 (2026-10-24) [M]

같은 실행 컨테이너를 다시 재생한 것이다. 새 실행은 없다. kz 는 commit `ada3422` 를 같은 빌드 디렉터리에서 빌드한 것이다: SHA-256 `f5367833…4f5c`, 사본 `/home/user/work/kairos-build/runs/w2r_ada3422/kz`, 출력 컨테이너의 도구 해시 `bbb328c9…a024`. 이 결과도 pilot/dev 재생이다.

```sh
kz replay v153_pilot1.kzt --profile research/cfs_procrace/kairos/profiles/cfs-v7.0.1 \
    --const CFE_PLATFORM_ES_MAX_APPLICATIONS=32 --basis v153_pilot1_basis.kz \
    --core-request-caller CFE_ES_CreateObjects --outcome-narrowing on --compare-narrowing \
    --out v153_pilot1_replay_w2r.kz > v153_pilot1_replay_w2r.txt
# 같은 명령에 --outcome-narrowing off, 출력 이름 v153_pilot1_replay_w2r_p1off.{kz,txt}
```

- `--core-request-caller CFE_ES_CreateObjects` 의 출처는 두 가지이다 [F].
  - SPEC §3.3 "Creation events used for `app_owner`": 코어 task 의 요청은 `CFE_ES_CreateObjects` 의 코어 객체 루프에서 나온다.
  - 계획의 인용 `es_core_starttask`(`cfe_es_start.c:810-814`). 외부 앱의 요청 함수 `CFE_ES_AppCreate`(`es_appcreate_starttask`, `cfe_es_apps.c:849-854`)는 코어가 아닌 쪽으로 읽힌다.
- **생성 id 결합.** `OS_TaskCreate` 반환 24개 모두에 `out.task_id` 를 붙였다. 신원 결합의 `ID_MATCHED` 24개이다. 배정 id(사이트 25)와 자식이 알린 id 가 같은 경우만 묶었다. 그 결과 `R2-P-TASK` 간선 24개가 생겼고, 판정 찾기의 `R2-P-TASK` 24건이 사라졌다. 남은 찾기는 `R19` 1건이다(버전을 주지 않음, 전과 같음).
- **코어 여부.** 코어 서비스로 읽힌 thread 는 8개이다(`CFE_ES`, `ES_BG_TASK`, `CFE_EVS`, `CFE_SB`, `CFE_TBL`, `CFE_TIME`, `TIME_TONE_TASK`, `TIME_ONEHZ_TASK`). 이 앱들의 요청 함수가 `CFE_ES_CreateObjects` 이고, 자식 task 는 생성자의 앱을 따른다. 코어 아님은 16개이다(`CFE_ES_AppCreate` 로 요청된 15개 앱과 `FM_CHILD_TASK`). 소유 앱이 미정인 3개 thread 는 코어 여부도 세우지 않았다.
- **실행 관계.** 상대는 그 발생의 키 `evs.reg(AppID)` 를 등록한 task 이다. 사용한 소유 앱은 모두 `confirmed` 이다(`relation-inferred` 0, `core-status-unknown` 0).

| 관계 | 발생 | 내용 |
|---|---|---|
| `ownership-undetermined` | 24 | `CFE_ES` 20, `CFE_EVS` 4. 전달된 AppID 0 의 등록·게시가 기록에 없다(`evs.reg[0]`). 값 0 은 어떤 앱도 가리키지 않는다. 그 값을 읽어 온 SB 의 AppId 셀은 포획 지점이 아니어서, 기록이 상대 앱(SB)을 지목하지 못한다 [I] |
| `intra-application` | 5 | `CFE_SB` 가 SB 자신의 AppID 로 보낸 것(같은 task) |
| `core-application` | 246 | 전달 AppID 가 SB(0x110003)인 218, TBL(0x110004)인 28. 상대는 그 앱의 EVS 등록 task(`CFE_SB` / `CFE_TBL`)이다. 코어–코어 12(`CFE_TIME`→SB 8, `CFE_TBL`→SB 4), 앱–코어 234(`TO_LAB` 111, `DS` 21, `HS` 13, `HK` 12, `CF` 10, `LC` 10, 그 밖 57) |
| `inter-application`, `creation-framework-new-application` | 0 | — |

- **P1 두 읽기.** 발생 275개이다. 관계는 두 읽기에서 같다.

| 요약 | 반환 코드 고정 켬(현행) | 끔(P1 대안) |
|---|---|---|
| `OBSERVED_VIOLATION` | 24 (관계 미정 24) | 24 (관계 미정 24) |
| `NO_VIOLATION_IN_CHECKED_PREFIX` | 251 (core-application 246, intra 5) | 0 |
| `INCONCLUSIVE` | 0 | 251 (`resource_or_lifetime_unmatched`, `effect_unsupported`; 상태 `UNDETERMINED` {VIOLATING, READY}) |
| 입력 오류 | 0 | 0 |

  요약이 달라진 발생은 251개이다. 24개 위반은 인자만으로 결정되므로 두 읽기에서 같다.
- **과거 개발 기록 s1r2_p1**(개발 기록, 이 실행과 합치지 않음). 출력은 저장소 밖 `/home/user/work/procrace/r2_runs/s1r2_p1_rereplay/` 이다.
  - 발생 271개이다. 두 읽기 모두 `OBSERVED_VIOLATION` 24 / `INCONCLUSIVE` 247 이고, 달라진 발생은 0개이다. 247개는 등록 지점과 writer 가 계획에 없어 이미 열려 있다. 고정은 적용되지만(`outcome-narrowed` 247) 결정적이 되지 않는다.
  - 관계는 271개 모두 `ownership-undetermined` 이다(ACTOR_OWNERS 없음).
  - `R2-P-TASK` 24건은 그대로이다. 그 계획에는 배정 id 사이트가 없다.
- **출력 컨테이너**(`docs/schema.md` §11). 종류 `verdicts`, 부모는 실행 컨테이너 `dddd7746…3835`, 섹션 7개이다: STRINGS, BASIS 4, ACTOR_OWNERS 27, SUMMARIES 275, MODEL_IDS 1, OBS_ADEQUACY 275, IDENTITIES 526(사용 275 + 등록 task 251). `kz-dump` 로 읽었다.
- **손실의 task 귀속.** 이 실행은 손실 보고가 0 이다. 일반적으로도 하지 못한다: LOSS 의 `cpu` 는 기록을 버린 ring 이지만, RAW_EVENTS 에 CPU 가 없고 백엔드가 문맥 전환을 기록하지 않는다. 귀속에는 두 가지가 필요하다 [I]: 사건마다 CPU 를 남기는 것(디코더는 표본의 CPU 를 읽지만 버린다), 그리고 그 CPU 에서 손실 구간 동안 돈 task 의 기록(sched 전환)이다. 그때까지 손실은 기록 전체에 걸친다(P2 의 현행 유지와 같음).

## 6. 산출물과 SHA-256

`/home/user/work/procrace/r2_runs/v153_pilot1/` (저장소 밖). kz 사본은 `/home/user/work/kairos-build/runs/v153_pilot1/kz` 이다.

| 파일 | SHA-256 |
|---|---|
| `v153_pilot1.kzt` | `dddd77465652c41f3286caca90301a4a89dcb7d743b21402cb5eb2e1330d3835` |
| `v153_pilot1.kzt.record.txt` | `89ea6d681955c4841b77c27ce77a35d36be68c11b31c2c38a27833d680be5fb7` |
| `v153_pilot1_anchor_xcheck.txt` | `c7fe724214fefd4a605d5103571130da153016f9b6ec66c4179ea91ff184ff23` |
| `v153_pilot1_basis.kz` | `7c942049bac8b9e6dfa4552753776a91825e5fea4c536bcd6fb52d729c4bbccf` |
| `v153_pilot1_conditions.tsv` | `c0d77f028e3083ab5a109167c4dfaea610474f5336a4880e7c18c6508dc8de0c` |
| `v153_pilot1_driver.txt` | `86b064f2fc380626bbfae151bf9d45a10cd7e16839d2e8234697844fb97fda8b` |
| `v153_pilot1_hits.txt` | `e3e235de2e94e492839542640c3ff6819be99ac0d5ab01c8fccd04980084df6e` |
| `v153_pilot1_perf_operational_esrestart.log` | `0054fce138038c08423508ff5e0c1a7077066dba035a909bdcd829bcc21060b6` |
| `v153_pilot1_perf_operational_esrestart.log.meta` | `d4dcbaa468dcaaa54da4b6546c61b544196263a905c9e8529179483bda6b53fe` |
| `v153_pilot1_persist_after.txt` | `060d788401f4b25775ee895d521eabe833624cdc90eba831ebb403bded97f86b` |
| `v153_pilot1_persist_before.txt` | `f9a23eace68699a0140893ed8b37c273ef56ac52f196ea22bf5dec286c87444a` |
| `v153_pilot1_replay.err` | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` |
| `v153_pilot1_replay.txt` | `5e5798d23a4045cddf9b92475ccbddfa99954eba640a26b0f73a1762ad29cef2` |
| `v153_pilot1_source.diff` | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` |
| `v153_pilot1_replay_w2r.txt` (§5.2) | `84d8185a0903033f6fdf8e5982986a05cd10679a0a98d44545edad6844c31d17` |
| `v153_pilot1_replay_w2r.kz` | `88b2c863cb28e7bcf0bab26b4d325dc2765e2be0386101a6936a1febd93964c0` |
| `v153_pilot1_replay_w2r.kz.dump.txt` | `886d43829acea28e8dbd9644c8448531cd4c70277224d4b35681cbc9e0fbe889` |
| `v153_pilot1_replay_w2r_p1off.txt` | `8767af2df29d4bd784d41d199edd0cdb954490d5faa93d9a36c0f9eaf3634170` |
| `v153_pilot1_replay_w2r_p1off.kz` | `b025532d8fcdf3bd91bb0e3ff85cf9aabdd0ae85ddf3b78b963d5474d2a28e16` |
| `v153_pilot1_replay_w2r_p1off.kz.dump.txt` | `e6b0201993cce501314791a0938b19dfd99007a3716770863ddc0639615eda42` |
| `v153_pilot1_replay_w2r{,_p1off}.err` | `e3b0c442…b855` (빈 파일) |
| `../s1r2_p1_rereplay/s1r2_p1_replay_w2r.txt` | `a539172e7909291e828d6d422599970caeff32cb23ceb179e573574a7a6726fd` |
| `../s1r2_p1_rereplay/s1r2_p1_replay_w2r.kz` | `d117113679fdbe460210ad6db68428d51d65f7833526b9bbfd6ae2595a456fbb` |

컨테이너 헤더: basis `7c942049…bccf`, 관측 프로필(`sites.kzp`) `fbb31fda…1776`, 섹션 13개(ACTOR_OWNERS 27행 포함).

## 7. 남은 것

- 이 실행은 pilot/dev 하나이다. 고정 본 실행 묶음과 그 반복 수는 PI 결정(D14)이다.
- **251개의 판정이 반환 코드 고정(O4, `outcome-narrowed`)에 기댄다.** 레퍼런스는 코드만으로 고정해도 되는지 정하지 않는다(MP §3.1 은 반환값 증명을 허용하는 쪽, G3 은 계산된 상태와 코드 일치를 둘 다 요구). PI 결정이 필요하다.
- writer·별칭 포괄과 수명은 run 컨테이너로 세우지 않는다.
  - `evs.reg` 의 EarlyInit memset 과 CleanUpApp SetFree 는 Φ 효과가 아니다.
  - AppCreate·CleanUpApp 의 `es.app` writer 에는 사이트가 없다.
  - 효과 값(`lp.*`)은 S1 백엔드가 읽지 못한다.
  - 행위자 사이 순서는 확립되지 않는다.
- W2 잔여(`ada3422`)에서 한 일(§5.2): `R2-P-TASK` 간선에 필요한 `out.task_id` 를 배정 id(사이트 25)에서 복귀 사건으로 옮기는 일, 실행 관계 계산, 요약을 컨테이너에 쓰는 writer.
- 남은 일:
  - 위반 24개의 관계 상대(SB)를 기록으로 지목하는 일. 전달 값을 읽은 위치의 포획(`cell`)이 필요하다. (R2 잔여에서 함: §8.3)
  - 손실의 task 귀속. 사건별 CPU 와 문맥 전환 기록이 필요하다.
  - OBS_ADEQUACY 의 `probes`·`unobserved` 채우기. (R2 잔여에서 함: §8.4)
- L-OBS 는 여전히 `unproved` 이다.
- 위험 목록(마스터플랜 §11) 갱신:
  - **F01**: 실제 실행에서 task 24개의 소유 앱을 생성 측 근거(요청 인자, 생성자 소유, 배정 id = 알린 id)로 `confirmed` 했다 [M]. 실행 task, 생성 실행자, 소유 앱, 전달 AppID 를 따로 출력한다. 실행 관계는 W2 잔여에서 계산했다(§5.2): core-application 246, intra-application 5, 미정 24(위반 24개; 상대를 기록이 지목하지 못함).
  - **F05**: 등록 효과·등록 LP·App 레코드 writer·생성 근거를 실제 바이너리에서 손실 보고 없이 관측했다 [M]. 그러나 writer 포괄·수명·행위자 사이 순서가 없어서 상태로 판정하는 조건은 관측만으로 서지 않는다. 그 대신 반환 코드 고정(PI 결정 대기)에 기댄다.

## 8. 두 번째 pilot 관측 v153_pilot2 (R2 잔여, 2026-10-24) [M]

- **지위: pilot/dev 실행 한 번.** 본 실험 결과가 아니고 v153_pilot1 과 합치지 않는다. 반복 수는 PI 결정 P8(D14)이다. 비용·빈도는 주장하지 않는다.
- 목적: R2 잔여 세 가지를 실제 실행 하나에 건다. (1) OBS_ADEQUACY 의 `probes`·`unobserved` 와 정적 닫힘(kz-static facts)에서 writer·별칭 포괄 읽기, (2) 위반 24개가 넘긴 AppID 를 읽어 온 위치와 그 writer 의 포획, (3) 두 P1 읽기 재생.

### 8.1 명령과 §1 과 다른 점

저장소 commit `88bd322`, `kairos/` 미커밋 차이 없음(`v153_pilot2_source.diff` 빈 파일). kz SHA-256 `eb714007…89af`(사본 `/home/user/work/kairos-build/runs/v153_pilot2/kz`), 실행·재생 컨테이너의 도구 해시 `8b774f13…a5da`.

```sh
CFS_NORMAL_USER=ubuntu research/cfs_procrace/kairos/probe/scripts/observe_cfs_smoke.sh v153_pilot2 \
    /home/user/work/kairos-build/tools/kz/kz 1 --out-dir /home/user/work/procrace/r2_runs/v153_pilot2 --purpose pilot
# 정적 닫힘(분석 입력, cFS 실행 없음): docs/static_facts.md §2 의 명령 그대로, k1r3-llvm 의 kz-static (파일 SHA-256 43a3f268…475c)
kz-static basis ... --recipe profiles/cfs-v7.0.1/basis/core-cpu1.kzb -o v153_pilot2_static_basis.kz
kz-static extract ... -o v153_pilot2_facts.kz
kz replay v153_pilot2.kzt --profile research/cfs_procrace/kairos/profiles/cfs-v7.0.1 --const CFE_PLATFORM_ES_MAX_APPLICATIONS=32 \
    --basis v153_pilot2_basis.kz --core-request-caller CFE_ES_CreateObjects \
    --facts v153_pilot2_facts.kz --static-basis v153_pilot2_static_basis.kz \
    --outcome-narrowing on --compare-narrowing --out v153_pilot2_replay.kz > v153_pilot2_replay.txt
# 같은 명령에 --outcome-narrowing off, 출력 v153_pilot2_replay_p1off.{kz,txt}
```

| 항목 | v153_pilot1 | 이번 | 이유 |
|---|---|---|---|
| 계획 | 사이트 26개(`fbb31fda…1776`) | 30개(`2e4132e3…737c`): 26 `sb_publication`(`cfe_sb_task.c:138` 호출 직후, 사이트 2 와 같은 명령; 프로필의 게시 사이트), 27–29 포획 지점 `CFE_EVS_SendEventWithAppID.id_valid@…`(`cfe_sb_api.c:275`, `:1183`, `cfe_sb_priv.c:786` 의 `call` 명령, `rdx` = AppID, 역할 `value(CFE_SB_Global.AppId)`) | R2 잔여 (2). 인용 36개 해시가 R2 빌드 트리(`/home/user/work/procrace/cfs_ref/cFS`)에서 모두 일치(`kz anchor --source-root`), 실행 뒤 `--xcheck` 에서 27–29 의 프로브 명령이 `call CFE_EVS_SendEventWithAppID` [M] |
| 실행 순번 | 0 | 1 | 두 번째 pilot 관측이라는 식별자일 뿐 조건이 아니다 |
| 재생 입력 | 상수, basis, 코어 요청 함수 | 같음 + 정적 닫힘(`--facts`, `--static-basis`) | R2 잔여 (1). static basis 의 바이너리 해시 = 실행 basis 의 것(`0edb1ae2…7799`), facts 의 프로필 해시 = 재생 프로필 해시(`bc8a3401…90a4`). facts 는 `static_golden_core_cpu1` 의 작업 파일과 바이트가 같다 |

나머지(바이너리, 사용자 `ubuntu`, 관측기 root, `operational`, `ci-es-restart-poweron`, `/proc` 읽기 `full`, 층 `none`, ring, 커널 `6.18.44-fc-v153`, 환경 이름 153개의 `cfs.environment` deviation)는 §1–§3 과 같다. 시작 2026-10-24T19:45:34Z, 실행 직전 상태는 §3 과 같은 형태(`EEPROM.DAT` 전부 0, 열쇠 파일 크기 0, SysV shm 없음, `/dev/shm/osal:RAM` 은 15:53 의 빈 디렉터리, 정리하지 않음). OPERATIONAL 줄 1626 ms, 1 ms 뒤 재시작 명령(종료 0), 프로세스 종료 0, `POWERON`. 1626 ms 는 이 실행 하나의 값이고 계획이 달라 3308 ms 와 비교하지 않는다.

### 8.2 관측

- 사건 1269, 사이트 30, thread 27, **손실 0**(`PERF_RECORD_LOST` 0, throttle 0). `app_owner` confirmed 24 / undetermined 3, `ids_matched` 24.
- `evs.send` 269/269, `slot_registered` 245, `sb_publication` 1, 포획 26 / 111 / 65(합 202). 사이트 2 보다 앞선 전송은 24 = ES 20 + EVS 4, `app_id` 0, 결과 `0xc2000003`, 호출 함수 `CFE_SB_CreatePipe` 2 / `CFE_SB_SubscribeFull` 4 / `CFE_SB_MessageTxn_ReportSingleEvent` 18, event id 5 / 10 / 14 = 2 / 4 / 18(§5.1 과 같은 묶음). 전체 전송은 269 로 pilot1 의 275 와 다르다. 원인은 조사하지 않았고 맞추려고 바꾼 것은 없다.
- 포획 202개는 모두 바로 다음 같은 thread 의 전송 호출과 AppID·event id 가 같다(불일치 0). 포획이 없는 호출 67개는 다른 호출 지점(TBL 의 `TableTaskAppId` 를 넘기는 28 등)이고 호출에서 읽는다.

### 8.3 재생: 개수와 관계

| 요약 | 반환 코드 고정 켬(현행) | 끔(P1 대안) |
|---|---|---|
| `OBSERVED_VIOLATION` | 24 (core-application 24) | 24 (같음) |
| `NO_VIOLATION_IN_CHECKED_PREFIX` | 245 (outcome-narrowed 245; core-application 240, intra 5) | 0 |
| `INCONCLUSIVE` | 0 | 245 (`resource_or_lifetime_unmatched`, `effect_unsupported`) |
| 입력 오류 / `partial` | 0 / 아님 | 0 / 아님 |

요약이 달라진 발생 245개. 관계는 두 읽기에서 같다: core-application 264, intra-application 5, **ownership-undetermined 0**.

- **위반 24개의 상대 = `CFE_SB`.** 조건 `id_valid` 는 포획(`capture`)에서 결정되었고 포획이 위치 `CFE_SB_Global.AppId` 를 준다. 그 위치를 기록 안에서 쓴 행위자는 게시 사이트 `sb_publication`(`cfe_sb_task.c:138`)의 `CFE_SB` 하나이다(소유 앱 confirmed, 코어). 사용 쪽 `CFE_ES`·`CFE_EVS` 도 confirmed·코어이고 소유 앱이 달라 core-application(코어–코어)이다. 키 `evs.reg[0]` 의 등록은 여전히 없고 사유에 함께 적힌다: "no registration or publication of evs.reg[0] in the record; the passed value was read from CFE_SB_Global.AppId, written by CFE_SB". 관계는 행위자 사이의 것이고 "사용이 저장보다 앞섬"은 한 시계의 병합 순서이다(SPEC §5.8) [M].
- 나머지 245 의 전달 AppID 는 SB 217(그중 `CFE_SB` 자신 5 = intra) / TBL 28 이다. 포획이 있는 SB 사용은 키 등록자(`CFE_SB`)와 위치 writer(`CFE_SB`)가 같아 관계가 pilot1 의 계산과 같은 쪽이다.

### 8.4 적합성 필드

- `established`: 위반 24 는 7 항목 모두(값 조건이 인자만 읽음). 245 는 {`aliases-covered`, `start-state-known`, `probes-attached`, `probes-active`, `loss-free`} — pilot1 대비 `aliases-covered` 가 더해졌다(정적 닫힘: 묶인 필드 6개 모두 `writer-set-complete`, A-TYPE 0; G1–G4 가정 위). `writers-covered` 와 `lifetime-known` 은 세워지지 않아 보류 사유는 그대로이다.
- 정적 writer 중 관측 지점에 있는 것(종류·위치별): `evs.reg` 4 중 2(관측 안 됨: EVS EarlyInit memset `cfe_evs_task.c:63` — 간접 호출 대상, `EVS_AppDataSetFree` `cfe_evs_utils.h:150` — 호출 `cfe_evs_task.c:194`(CleanUpApp) 미관측), `CFE_SB_Global.AppId` 3 중 2(두 out-pointer writer 는 `sb_publication` 으로 덮임; SB EarlyInit memset `cfe_sb_init.c:50` 미관측), `es.app` 10 중 3, `es.appstate` 7 중 3, `es.shutdown` 2 중 0. 프로필 writer 효과 중 지점 미관측: `CFE_ES_GetAppID.store_ok`/`store_fail`, `CFE_SB_EarlyInit.clear_global`, `CFE_ES_CreateObjects.type_core` 등(`real record knowledge`).
- `probes`(REF_LISTS, 목록 2개): 포획이 있는 발생은 사이트 0, 1, 10, 11, 12, 26, 27, 28, 29, 없는 발생은 0, 1, 10, 11, 12. `unobserved` 는 위 목록 중 그 발생의 조건이 읽는 종류·위치의 것(위반 24 는 `evs.reg` 와 `CFE_SB_Global.AppId` 의 것).
- 출력 컨테이너 섹션 8개(STRINGS, BASIS 6, REF_LISTS 16, ACTOR_OWNERS 27, SUMMARIES 269, MODEL_IDS 1, OBS_ADEQUACY 269, IDENTITIES 538). BASIS 에 `replay.facts.sha256`, `replay.static_basis.sha256`.

### 8.5 손실

손실 보고 0, throttle 0. 손실 구간이 없어 모든 판정 구간의 `loss_reports` 0, `end_reason` decision. 손실의 task 귀속은 §5.2 와 같이 여전히 하지 않는다(사건별 CPU·문맥 전환 기록 없음).

### 8.6 기대 결과표(`plan/expected_results_C1_R2.md`, 동결)와의 대조

| 행 | 기대 | 이번(현행 / P1 대안) | 판단 |
|---|---|---|---|
| E-14 (U-R2-1, U-R2-2) | VIOLATING → `OBSERVED_VIOLATION`, 결과 `CFE_EVS_APP_ILLEGAL_APP_ID`, 실행 task 의 앱을 정하지 못하면 앱 간 결과로 세지 않음 | 24 `OBSERVED_VIOLATION`(ES 20, EVS 4), 결과 대응 일치, 소유 앱 confirmed, 관계 core-application / 같음 | 일치. 순서 의존은 주장하지 않는다(한 시계) |
| E-15 (U-R2-3) | VIOLATING 이 되지 않음. 등록 효과가 계획에 있으면 READY → `NO_VIOLATION_IN_CHECKED_PREFIX` | 245 `NO_VIOLATION_IN_CHECKED_PREFIX`(전부 반환 코드 고정) / 245 `INCONCLUSIVE(resource_or_lifetime_unmatched, effect_unsupported)`, VIOLATING 0 | VIOLATING 아님은 일치. 상태로 READY 가 서지는 않는다(writer 포괄·수명 미확립). 현행의 위반 없음은 P1 에 기댄다 |

### 8.7 산출물과 SHA-256

`/home/user/work/procrace/r2_runs/v153_pilot2/` (저장소 밖, 4.2 MB).

| 파일 | SHA-256 |
|---|---|
| `v153_pilot2.kzt` | `16175a4f7f46b374b9914317aa4847c22f8d20819163934327e2b1a9caead1ec` |
| `v153_pilot2.kzt.record.txt` | `0d11ff4fea5d772b6055f9b13592dd93de209e15fc6cb7a59893f06ebf4f5a23` |
| `v153_pilot2_basis.kz` | `5fee359417cb6b12babfbee8f4bd7fd4934a8d996c59cf0f42b8491655445ad4` |
| `v153_pilot2_anchor_xcheck.txt` | `55d9b4257370c34da834553e097914cd388045b289dc7b92ee19513cf3d9e2ab` |
| `v153_pilot2_conditions.tsv` | `94b27a67a679b2e0805dda74bfd6f42554449f6ecb6a52df689e1374f5a5ef4c` |
| `v153_pilot2_driver.txt` | `29cb71437d1b518b18b6eec316db05d5f3c7cc02ec826ab7b55e03b7d8a2f4fa` |
| `v153_pilot2_hits.txt` | `25565fbee8f00ad35a7d9bb89685880c67699c79c8d901aaa60d5b8c8af844b3` |
| `v153_pilot2_perf_operational_esrestart.log` / `.meta` | `0c07be8f…f3d8` / `76b31b49…99e2e3` |
| `v153_pilot2_persist_before.txt` / `_after.txt` | `a33957b6…aa29` / `2f3e2ddf…2eb3` |
| `v153_pilot2_static_basis.kz` | `7c28e22ce72865e13a9ed9beda6dddfc7f464fa3c32755b8db5c602aca4fc425` |
| `v153_pilot2_facts.kz` | `9c4dec50da5245fbc91d92becf7c988aa6106f1e8d2a2f325c546f922a56ae99` |
| `v153_pilot2_replay.txt` / `.kz` | `9287cde5…d1fd` / `cc5248c4480bcdca681a7357374041f6a6fa6197159045c80e5783769a460a10` |
| `v153_pilot2_replay_p1off.txt` / `.kz` | `0f7a5eb8…15ce` / `c0ec480f2d7075eef5a27589f63a019035cea0ac7a7b3e42fc7101cfea6b0ec2` |
| `*_replay*.kz.dump.txt` | `6f647312…794f` / `68e987a8…4005` |
| `v153_pilot2_source.diff`, `*_replay*.err` | `e3b0c442…b855` (빈 파일) |

### 8.8 남은 것

- 반복 수(P8)와 본 실행 묶음은 PI 결정이다. 이 실행은 pilot 하나이다.
- 245 의 위반 없음은 여전히 반환 코드 고정(P1)에 기댄다. 정적 닫힘으로 별칭 포괄은 섰지만 writer 포괄은 서지 않는다: 관측되지 않은 writer 가 있고(위 목록), 그중 EVS·SB 의 EarlyInit 지우기와 CleanUpApp 의 SetFree 는 Φ 효과가 아니다. 수명(`lifetime-known`)은 실행 컨테이너가 세우지 않는다.
- 위치의 writer 는 관측된 것만 상대가 된다. SB EarlyInit 지우기(`cfe_sb_init.c:50`, 시작 thread `core-cpu1` 이 실행; 소유 앱 미정)를 관측하면 위반 24 의 관계가 `ownership-undetermined` 로 바뀔 수 있다 [I]. **PI 확인(결정하지 않음)**: 위치에 무효 id 를 쓰는 지우기(EarlyInit memset, `CFE_ES_GetAppID` 실패 가지 저장)를 관계의 상대("actors involved")에 넣는가. SPEC §1.1 은 "Every result names the relation between the actors involved" 라고만 하고 여러 writer 가 있는 위치에서 누가 관여자인지 정하지 않는다. `plan/proof_status_R2.md` F-4 는 S3a writer 분류에서 이 둘을 중립으로 적지만 그것은 인증서의 writer 구간 분류이지 관계 규칙이 아니다. 현행: 종류의 등록과 같은 규칙(관측된 쓰기는 값과 무관하게 모두 상대)이고 대안은 고르지 않았다.
- 손실의 task 귀속, rx·C1 쪽은 이 절의 범위 밖이다.

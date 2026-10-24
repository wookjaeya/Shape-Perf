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
  - `R2-P-TASK` 24건: `OS_TaskCreate` 의 복귀에 `out.task_id` 가 없어 생성 간선을 만들지 못했다. 배정 id 는 사이트 25 가 읽지만, 어댑터는 아직 그 값을 복귀 사건으로 옮기지 않는다.
  - `R19` 1건: 버전이 주어지지 않아 적용하지 않았다.
- 실행 관계: 요약의 `relation` 은 여전히 `ownership-undetermined` 이다. 실행 관계 계산은 W1 에서 남긴 일이다.

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

컨테이너 헤더: basis `7c942049…bccf`, 관측 프로필(`sites.kzp`) `fbb31fda…1776`, 섹션 13개(ACTOR_OWNERS 27행 포함).

## 7. 남은 것

- 이 실행은 pilot/dev 하나이다. 고정 본 실행 묶음과 그 반복 수는 PI 결정(D14)이다.
- **251개의 판정이 반환 코드 고정(O4, `outcome-narrowed`)에 기댄다.** 레퍼런스는 코드만으로 고정해도 되는지 정하지 않는다(MP §3.1 은 반환값 증명을 허용하는 쪽, G3 은 계산된 상태와 코드 일치를 둘 다 요구). PI 결정이 필요하다.
- writer·별칭 포괄과 수명은 run 컨테이너로 세우지 않는다.
  - `evs.reg` 의 EarlyInit memset 과 CleanUpApp SetFree 는 Φ 효과가 아니다.
  - AppCreate·CleanUpApp 의 `es.app` writer 에는 사이트가 없다.
  - 효과 값(`lp.*`)은 S1 백엔드가 읽지 못한다.
  - 행위자 사이 순서는 확립되지 않는다.
- `R2-P-TASK` 간선에 필요한 `out.task_id` 를 배정 id(사이트 25)에서 복귀 사건으로 옮기는 일, 실행 관계 계산, 요약을 컨테이너에 쓰는 writer 가 남았다(W1 의 남은 일과 같음).
- L-OBS 는 여전히 `unproved` 이다.
- 위험 목록(마스터플랜 §11) 갱신:
  - **F01**: 실제 실행에서 task 24개의 소유 앱을 생성 측 근거(요청 인자, 생성자 소유, 배정 id = 알린 id)로 `confirmed` 했다 [M]. 실행 task, 생성 실행자, 소유 앱, 전달 AppID 를 따로 출력한다. 앱 사이 관계(실행 관계)의 계산은 아직 없다.
  - **F05**: 등록 효과·등록 LP·App 레코드 writer·생성 근거를 실제 바이너리에서 손실 보고 없이 관측했다 [M]. 그러나 writer 포괄·수명·행위자 사이 순서가 없어서 상태로 판정하는 조건은 관측만으로 서지 않는다. 그 대신 반환 코드 고정(PI 결정 대기)에 기댄다.

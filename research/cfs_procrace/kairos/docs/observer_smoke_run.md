# 관측기 연습 실행 기록 (sub-gate S1-R2, 실행 한 번)

- 날짜: 2026-10-15 (UTC 11:17). 실행한 것: `kz observe` 로 수정하지 않은 cFS(`core-cpu1`)를 **한 번** 관측했다. 이 문서는 정확한 조건, 기준과 다른 점(deviation), 그리고 센 값을 적는다.
- **이 실행이 보여 주는 것**: 관측기가 실제 cFS 에 붙고, 손실 없이 사건을 받고, thread 정체성을 GetAppID 없이 결합한다는 것. 
- **이 실행으로 주장하지 않는 것**: 빈도, 분포, 재현성, 관측 비용(오버헤드), gdb 실행과의 동등성, R2 의 판정. 실행이 한 번이다(기준 gdb 실행은 세 번). 분석은 K3 의 몫이고 여기서는 개수만 센다.
- 표기: **[M]** 이번에 실행해 확인한 사실, **[D]** 설계 결정, **[I]** 계획·미확인.
- 붙이기에 실패하지 않았으므로 블로커는 없다. 실패했다면 오류를 기록하고 멈추기로 했었고(gdb 나 수정한 바이너리로 대체하지 않는다), 그 경로는 쓰이지 않았다.

산출물은 모두 `env/logs/` 에 접두어 `s1r2_p1_` 로 있다(새 파일뿐이고 기존 파일은 바꾸지 않았다). 파일과 SHA-256 은 §7.

## 1. 정확한 명령

`probe/scripts/observe_cfs_smoke.sh s1r2_p1` 이 아래를 순서대로 한다. root 에서, 환경 변수 `KZ`(= `kz` 실행 파일 경로)와 `CFS_NORMAL_USER=ubuntu` 를 주고 시작했다.

```sh
KZ=/home/user/work/kairos-build/k2-gcc/tools/kz/kz CFS_NORMAL_USER=ubuntu \
  /home/user/Shape-Perf/research/cfs_procrace/kairos/probe/scripts/observe_cfs_smoke.sh s1r2_p1
```

스크립트 안에서(`$ENV`=`.../cfs_procrace/env`, `$LOGS`=`$ENV/logs`, `$EXE`=`/home/user/work/procrace/cfs_ref/cFS/build-native_std/exe/cpu1`):

1. `$ENV/tools/persist_snapshot.sh "before s1r2_p1" $EXE > $LOGS/s1r2_p1_persist_before.txt`
2. `$KZ anchor $EXE/core-cpu1 kairos/profiles/cfs-v7.0.1/observe/sites.kzp -o $LOGS/s1r2_p1_basis.kz --source-root /home/user/work/procrace/cfs_ref/cFS`
3. 조건 파일 `$LOGS/s1r2_p1_conditions.tsv` 쓰기(§3).
4. 관측:
   ```sh
   env CFS_NORMAL_USER=ubuntu $KZ observe $LOGS/s1r2_p1_basis.kz kairos/profiles/cfs-v7.0.1/observe/sites.kzp \
       -o $LOGS/s1r2_p1.kzt --record $LOGS/s1r2_p1.kzt.record.txt --conditions $LOGS/s1r2_p1_conditions.tsv \
       --stratum cold --run-ordinal 0 \
       -- $ENV/run_cfs.sh operational ci-es-restart-poweron $LOGS/s1r2_p1_perf_operational_esrestart.log
   ```
5. `$ENV/tools/persist_snapshot.sh "after s1r2_p1" $EXE > $LOGS/s1r2_p1_persist_after.txt`
6. `$KZ hits $LOGS/s1r2_p1.kzt` 와 `$KZ hits $LOGS/s1r2_p1.kzt --site evs.send.entry --before sb.appid.store --group-slot app_id --group-return-slot status` → `$LOGS/s1r2_p1_hits.txt`. 실행이 끝난 뒤 같은 컨테이너를 읽어 `--group-slot event_id`, (전체 구간) `--group-slot event_type`, `--basis $LOGS/s1r2_p1_basis.kz --group-caller` 를 더 돌렸다(§5). 이 세 출력은 파일로 저장하지 않았고 컨테이너에서 다시 만들 수 있다. 이 옵션들(`--group-caller`, thread 별 교차 개수)은 실행 **뒤에** `kz hits` 에 더한 것이라 실행에 쓴 `kz` 빌드와 다른 빌드로 읽었다. 컨테이너는 읽기만 했고 `kz hits` 는 아무것도 바꾸지 않는다.

`env/run_cfs.sh` 는 고치지 않았다. 인자 `operational ci-es-restart-poweron` 은 기준 gdb 실행(`env/run_r2_gdb.sh`)과 같고, 다른 점은 `RUN_CFS_WRAP`(gdb)가 없다는 것(§3의 deviation 1). cFS 프로세스를 띄우는 줄은 스크립트가 `.meta` 에 적은 그대로이다 [M]: `launcher=setsid setpriv --reuid=ubuntu --regid=1000 --init-groups --`, `command=./core-cpu1`, `cwd=/home/user/work/procrace/cfs_ref/cFS/build-native_std/exe/cpu1`.

## 2. 조건

| 항목 | 값 | 출처 / 상태 |
|---|---|---|
| 시작 | 2026-10-15T11:17:24Z (관측기 `observation started`), 끝 11:17:31Z | 기록 |
| 호스트 | `Linux 6.18.44-fc-v114 #1 SMP PREEMPT_DYNAMIC`, 4 vCPU, `clocksource=tsc`, `perf_event_paranoid=2`, 관측기는 root(능력으로 허용) | 기록 `host.*` [M] |
| 기준 실행의 커널 | `Linux 6.18.44-fc-v80` (r2_g1…g3 의 `.meta`) | deviation 3 |
| 관측기 | `kz observe 0.1.0-S0`, `k2-gcc` 빌드(RelWithDebInfo, gcc 13.3.0), SHA-256 `e1ed2cbd…5518` | 드라이버 기록 |
| 백엔드 | perf-sample(uprobe PMU type 8), 충실도 F0, 시계 `CLOCK_MONOTONIC` | 기록 |
| 관측기의 사용자·스케줄링 | root, `SCHED_OTHER` rt 우선순위 0 nice 0, CPU 0–3 | 기록 `observer.*` |
| cFS 사용자 | `ubuntu`(uid 1000, gid 1000, 보조 그룹 `adm dialout cdrom floppy sudo audio dip video plugdev`). `run_cfs.sh` 가 `setpriv` 로 내려간다. 능력 없음(`CapEff 0`), `NoNewPrivs 0`, seccomp 없음 | 스크립트 `.meta`, 기록 `traced.pid14587.*` [M]. 레퍼런스: README(일반 사용자로 실행), D1 |
| 바이너리 | `build-native_std/exe/cpu1/core-cpu1`, **수정하지 않음**. SHA-256 `0edb1ae2…7799`(기준 실행의 `.meta` 와 같은 값), build-id `be6d04f1…2941`, 1531256 바이트, inode 2155585 | 기록 `binary.*` [M]. 관측기가 basis 와 대조해 일치(크기, SHA-256, build-id, 프로브 주소 여덟 곳의 바이트) |
| 모드와 중지 | `operational`, `ci-es-restart-poweron`(기준 gdb 실행과 같음). OPERATIONAL 줄을 1663 ms 에 읽고 1664 ms 에 ES 재시작 명령(`cmd_send`, 종료 0)을 보냈고, 프로세스는 2203 ms 에 종료(`exit_status=0`, `console.exit_reset_status=POWERON`) | 스크립트 `.meta` [M] |
| 시작 상태 | 이 실행 직전의 영속 상태: `EEPROM.DAT` 524288 바이트 전부 0(SHA-256 `07854d2f…1541`), 열쇠 파일 세 개 크기 0, SysV 공유 메모리 없음, `/dev/shm/osal:*` 없음, `cf/tmp` 비어 있음. 콘솔: `Starting the cFE with a POWER ON reset` | `s1r2_p1_persist_before.txt`, 콘솔 로그 [M] |
| 끝 상태 | `EEPROM.DAT` 내용 불변(같은 SHA-256, mtime 만 갱신), `/dev/shm/osal:RAM`(빈 디렉터리, 소유자 ubuntu) 생성, 프로세스 남음 없음 | `s1r2_p1_persist_after.txt` [M]. 정리하지 않았다(기존 실행과 같음) |
| 작업 디렉터리 | cFS: `.../exe/cpu1`(스크립트). 관측기: `/home/user/work/kairos-build/k2-gcc` | 기록 |
| 환경 | cFS 프로세스가 이 하네스의 환경을 그대로 물려받았다(변수 **이름**만 기록, 환경 블록 SHA-256 `cda09e02…3d33`, 8038 바이트). 기준 실행의 환경 이름과 비교하면(`.meta` 의 목록) 기준에만 있는 이름 둘(`CLAUDE_CODE_REMOTE_TOOLS_FORWARD`, `R2_TRACE_OUT`), 이 실행에만 있는 이름 하나(`KZ`: 드라이버에 준 `kz` 경로)가 다르다. `TERM=linux`, `SHELL=/bin/bash`, `TMPDIR` 없음(cFS 가 읽는 세 변수) | 스크립트 `.meta`, 기록 [M] |
| 도구 매개변수 | ring CPU 마다 512 쪽(2 MiB), 비움 poll 최대 10 ms, 비움 thread 의 CPU 지정 없음 | 기록 `tool.*`: `unspecified_by_reference`(레퍼런스가 정하지 않았고 값은 도구의 임시 값) |

## 3. 기준과 다른 점 (deviation)

`--conditions` 로 컨테이너 DEVIATIONS 에도 들어 있다(`s1r2_p1_conditions.tsv`). 값, 이유, 영향, 피할 수 있었는지.

1. **관측 방법**
   - 기준: gdb `-batch` 중단점(`RUN_CFS_WRAP`), 중단점마다 프로세스가 멈춘다(`env/run_r2_gdb.sh`).
   - 이번: `kz observe`, perf-sample uprobe, 프로세스를 멈추지 않는다.
   - 이유: 이 실행의 목적이 새 관측기를 cFS 에서 시험하는 것이다.
   - 영향: 두 방법이 실행을 흔드는 방식이 다르다. 두 관측의 동등성을 주장하지 않는다. 피할 수 없음(목적 자체).
2. **실행 횟수 N**
   - 기준: 3회(r2_g1, r2_g2, r2_g3).
   - 이번: 1회. 
   - 이유: 과제가 연습 실행 한 번이다. 목적별 N 은 S1 의 파일럿에서 정한다(PI 결정 D14).
   - 영향: 한 번의 값으로 빈도, 산포, 기준 세 번과의 비교를 주장할 수 없다. 피할 수 없음(과제 정의).
3. **커널**
   - 기준: `6.18.44-fc-v80`. 이번: `6.18.44-fc-v114`.
   - 이유: VM 커널이 바뀌었다. group A 는 아직 새 커널에서 재측정하지 않았다(D11).
   - 영향: 시간과 스케줄링이 기준과 다를 수 있다. 크기는 모른다. 피할 수 없음.
4. **층(stratum) `cold`**
   - 기준: 층의 정의는 "VM 재부팅 뒤 첫 실행"(`env/BASELINE_MEASURE.md`).
   - 이번: **선언**이다. 이 VM 은 2026-10-15 08:07:13 UTC 에 부팅했고, `env/logs/` 의 마지막 cFS 실행은 10월 8일이며 `/dev/shm`·EEPROM.DAT 시각도 그 뒤 cFS 실행이 없었음을 보인다. 그러나 파일 캐시 상태는 재지도 통제하지도 않았다. 그리고 `core-cpu1` 은 실행 전에 `sha256sum`, `objdump`, `kz anchor`(여러 번)로 읽혔으므로 실행 파일의 캐시는 데워져 있다. 앱 `.so` 를 읽는 명령은 이 절차에 없지만 부팅 뒤 다른 읽기가 있었는지는 모른다.
   - 영향: `cold` 는 "첫 실행" 기준의 이름일 뿐 측정이 아니다. 시작 순서와 시간이 부팅 직후 첫 실행과 다를 수 있다. 피할 수 있었음(캐시를 비울 수 있으나 호스트 상태를 바꾸므로 하지 않았다).
5. **하네스 환경이 cFS 로 상속됨**: 위 표의 환경 이름 차이. 변수 `KZ` 가 cFS 환경에 있다(드라이버에 경로를 주려고 환경 변수로 넘겼다). cFS 는 `TERM`, `TMPDIR`, `SHELL` 만 읽는다고 `env/CONDITIONS.md` E12 에 있다. 영향 없음으로 예상하나 검증하지 않았다. 피할 수 있었음(다음 실행은 `KZ` 를 환경이 아닌 인자로 줄 수 있다).

기록된 위반이 아닌 사실 하나: 이 실행의 cFS 는 root 의 관측기 아래에서 ubuntu 로 돌았다. 이는 PI 결정 D1(관측기는 root, cFS 는 일반 사용자)과 같다.

## 4. 관측 결과 (기록 `s1r2_p1.kzt.record.txt` 의 값) [M]

| 항목 | 값 |
|---|---|
| 종료 | `kz observe` 종료 코드 0, 명령(`run_cfs.sh`) 종료 0 |
| 붙은 사이트 | 8개, 사이트 이벤트 8 × 4 CPU = 32개(+ CPU 마다 옆 가지 이벤트 하나). 부착 실패 없음. 관측기가 명령보다 23 ms 먼저 붙음 |
| 받은 표본 | 629개(= 해석한 raw 사건 629, 사이트 없는 표본 0, 슬롯 적용 실패 0, 읽을 수 없는 슬롯 0) |
| 손실 | `PERF_RECORD_LOST` 보고 0, 버려진 기록 0, throttle 구간 0, 해석 오류 0. 이 말의 뜻은 "커널이 손실을 보고하지 않았다"이다(`observer.md` §7) |
| ring 사용 | 한 번에 쌓인 가장 큰 양: CPU 0 에서 10448 바이트(2 MiB 중). 비움 380회. 비움 thread CPU 시간 23.5 ms, 관측기 프로세스 CPU 시간 47.8 ms(명령 종료 시점) |
| 사이트별 개수 | `evs.send.entry` 271, `evs.send.ret` 271, `sb.appid.store` 1, `es.sync.entry` 7, `es.sync.ret` 7, `osal.task_create.entry` 24, `osal.task_create.ret` 24, `osal.task_entry` 24 |
| 호출 짝 | 302(= 271 + 7 + 24), 진입만 있고 복귀 없음 0, 복귀만 있고 진입 없음 0 (사건 짝짓기 실패 0) |
| 적재 편향 | `0x5606d5d1b000`, 사이트 표본의 명령 포인터와 불일치 0 |
| 추적된 프로세스 | pid 14587 하나. 다른 프로세스의 옆 가지 기록 3436개와 다른 파일의 mapping 2685개는 버림 |
| 시간 | 명령 실행 3.89 s(`target.wall_ns`, 스크립트의 격리 검사와 로그 처리 포함), 그 가운데 cFS 가 OPERATIONAL 줄을 낸 것은 1663 ms, 프로세스 종료 2203 ms. 프로브를 닫는 데 3.15 s(`observer.stop_ns`, 명령 종료 뒤) |

참고만: 기준 gdb 실행의 OPERATIONAL 줄 읽기는 2693, 2244, 2257 ms 이고 SPEC 이 인용하는 gdb 없는 기준은 1613–1636 ms 이다(커널 `-fc-v80`). 이번 값은 1663 ms 이다. 실행이 한 번이고 커널이 다르므로 **관측 비용을 이 숫자들로 주장하지 않는다**.

### 4.1 thread 정체성 (GetAppID 없음)

- thread 27개(SPEC 이 인용하는 M1 의 27개와 같은 수). 이름은 커널 이름. 25개가 이름을 가졌고 2개(tid 14591, 14592)는 이름이 없다: 둘 다 주 thread 가 만들었고 사건이 0개이며, 이름을 붙이지 않은 채 끝났다. 정체성을 추측해 채우지 않았다.
- 24개가 task 생성 호출과 **정확히 하나씩** 결합되었다(생성 관계 FORK + 생성자 쪽 구간 포함 + 알린 id + 커널 이름). 모호한 결합 0, FORK 가 없는 생성 호출 0, 복귀가 없는 생성 호출 0, 생성 호출이 없는 복귀 0, 결합되지 않은 id 알림 0, 사건을 가졌으나 thread 기록이 없는 사건 0.
- 알린 id 는 `0x10001`부터 `0x10018` 까지 24개이고, 새 thread 의 첫 함수는 모두 같은 진입 함수(`CFE_ES_TaskEntryPoint`)로 기호 해석되었다. 생성자는 대부분 주 thread, 일부는 task 가 만들었다(예: `ES_BG_TASK` ← `CFE_ES`, `TIME_TONE_TASK`·`TIME_ONEHZ_TASK` ← `CFE_TIME`, `FM_CHILD_TASK` ← `FM`).
- 이름이 첫 사건보다 늦게 정해진 경우가 24건이다(thread 가 이름을 붙이기 전에 id 를 알리므로 예상된 것).
- 모든 27개 thread 의 `/proc` 기록을 읽었다(FORK 기록 뒤 2.4–11 ms 에). 정책은 전부 `SCHED_OTHER`, rt 우선순위 0, nice 0. 8개는 허용 CPU 가 `0`(CPU 0 에 고정), 19개는 `0-3`. (고정은 PSP 의 일이다. 이 실행은 이를 바꾸지 않았다.)

## 5. 개수 (분석이 아니다)

`kz hits` 의 출력이다. 같은 컨테이너에서 다시 만들 수 있다.

**`CFE_EVS_SendEventWithAppID` 진입 가운데 `cfe_sb_task.c:138` 의 AppId 저장 사건(`sb.appid.store`)보다 병합 순서에서 앞선 것: 24개.**

| 구분 | 개수 |
|---|---|
| thread `CFE_ES` | 20 |
| thread `CFE_EVS` | 4 |
| 인자 `app_id`(슬롯 2) | 전부 `0x0` (24개 모두) |
| 짝이 된 복귀의 상태 | 전부 `0xc2000003`(24개 모두), 짝 없음 0. 이 값은 `cfe_error.h:267` 의 `CFE_EVS_APP_ILLEGAL_APP_ID` |
| 인자 `event_id`(슬롯 0) | `0x5` 2개, `0xa` 4개, `0xe` 18개 |
| thread × `event_id` | `CFE_ES`: `0x5` 1, `0xa` 2, `0xe` 17. `CFE_EVS`: `0x5` 1, `0xa` 2, `0xe` 1 |
| 호출한 함수(진입 사건의 반환 주소 − 적재 편향 `0x5606d5d1b000` → basis 의 기호) | `CFE_SB_CreatePipe` 2, `CFE_SB_SubscribeFull` 4, `CFE_SB_MessageTxn_ReportSingleEvent` 18 |

기준 [R2g §2]의 값은 24개 = ES 20 + EVS 4, AppId 0, 결과 `ILLEGAL_APP_ID` 24개, 호출 함수별 CreatePipe 2 / SubscribeFull 4 / ReportSingleEvent 18, 그리고 ES 20 = 17 `SEND_NO_SUBS` + 2 `SUBSCRIPTION_RCVD` + 1 `PIPE_ADDED`, EVS 4 = 1 + 2 + 1 이다. 기준 요약(`r2_g1_summary.txt`)에서 `CFE_SB_CreatePipe` 는 event id 5, `CFE_SB_SubscribeFull` 은 10, `CFE_SB_MessageTxn_ReportSingleEvent` 는 14 로 나온다. 위 표의 값은 이 모든 수와 같다: 호출한 함수별 2 / 4 / 18, event id 별 2 / 4 / 18, ES 는 17 + 2 + 1 이고 EVS 는 1 + 2 + 1 이다. 반환 주소로 호출 함수를 되찾는 방법은 이 실행에서 처음 실제 바이너리에 쓰였고(합성 프로그램에서는 시험했다), 기준의 gdb 호출 스택(프레임)과 같은 답을 냈다. 이는 K3 의 분석이 아니라 개수의 일치이며, 일치가 이 관측기가 맞다는 증명은 아니다.

**전체 구간**: `CFE_EVS_SendEventWithAppID` 진입 271개(복귀 271개). 기준 gdb 실행의 전체 호출자 합계는 251, 257, 257 이다. **다르다**(이번 271). 원인을 조사하지 않았고 맞추려고 아무것도 바꾸지 않았다. 후보는 있으나 측정하지 않았다: gdb 와 관측기가 앱의 시작 순서를 다르게 흔든다, 커널이 다르다, 실행이 한 번이다. 기준의 "SB 호출자 223/229/229"는 호출자(SB 함수)를 가려 센 값이라 이번 271 과 같은 종류의 수가 아니다. 호출자별로 나누려면 진입 사건의 `a[3]`(반환 주소)을 basis 의 기호로 해석하면 된다(`observer.md` §8). 이 단계에서는 하지 않았다.

참고로 이번 실행의 thread 별 진입 수(전체 구간)는 `TO_LAB` 119, `DS` 21, `CFE_ES` 20, `HK` 11, `HS` 10, `CF` 9, `SC` 9, `CS` 8, `CFE_TIME` 8, `LC` 8, `MD` 8, `SBN` 7, `CFE_SB` 5, `FM` 5, `SAMPLE_APP` 5, `CFE_EVS` 4, `CFE_TBL` 4, `CI_LAB` 4, `MM` 3, `SCH_LAB` 3 이다. `event_type` 별은 1 → 204, 2 → 28, 3 → 39.

## 6. 한계와 알려진 문제

- 한 번의 실행이다. 이 숫자들이 다음 실행에서 같은 값이라고 기대할 근거가 없다.
- `kz hits` 의 "앞섬"은 한 시계(`CLOCK_MONOTONIC`)의 병합 순서이다. 서로 다른 thread 사이의 happens-before 가 아니다.
- 관측기는 프레임워크의 이름·줄을 프로필에서만 안다. 프로필 사이트 여덟 개 가운데 인용 해시가 검증된 것은 8개(`prov` 8개), SPEC 줄을 인용한 `researcher` 항목 1개는 해시하지 않았다.
- 프로브 비용, 앱 `.so` 의 inode 등록, group A 재측정, 목적별 N 은 이번에 하지 않았다(V1b/S1).
- 프로브를 닫는 3.15 s 는 cFS 가 끝난 뒤에 쓰인다. cFS 에는 영향이 없지만, 같은 VM 을 쓰는 다음 작업이 이 시간 동안 커널의 프로브 정리와 겹칠 수 있다.
- 환경 변수 이름이 기록에 들어 있다(값은 없다). 비밀 값의 이름(예: 토큰 변수명)이 하네스에 따라 기록에 남는다. 기록 파일을 밖으로 낼 때 이 점을 알아야 한다.
- `/dev/shm/osal:RAM` 이 이 실행의 결과로 남았다. 기존 실행들도 같고 정리하지 않는다는 원칙을 따랐다. 다음 실행의 시작 상태가 이 실행과 다르다는 뜻이므로, 다음 실행의 영속 상태 스냅샷에 반영된다.

## 7. 산출물과 SHA-256

`/home/user/Shape-Perf/research/cfs_procrace/env/logs/` 아래:

| 파일 | SHA-256 |
|---|---|
| `s1r2_p1.kzt` (실행 컨테이너) | `6df021fb65289a278182c9c4e2d49f24215c1babfa9f09ebb943feb58e77b414` |
| `s1r2_p1.kzt.record.txt` | `1f863c778b8e551ae0f45a2a9faac062c2b3b8f1943cb5882d90a72db9ffca76` |
| `s1r2_p1_basis.kz` (앵커 표) | `151dc985d9b12941ab9a35ffd62a9ba8cdda89ff0d98228e363f597587ed0653` |
| `s1r2_p1_conditions.tsv` | `4e56694d0cc4a798c863ac09cff5880c1d84fc1bdb70efe8a0545b9dabcb6068` |
| `s1r2_p1_driver.txt` | `4467b595ddb32333cbca6139d53e54f54db9ebb4462668c72a51666eb98b2d3f` |
| `s1r2_p1_hits.txt` | `1deba44d9390911891a06a001cec8f06ec556509da86b279fe8cdcc1a9bbae2c` |
| `s1r2_p1_perf_operational_esrestart.log` (콘솔) | `70d20079aa0d2df6d246f7867a657d3a1677ee8bc8f8e2e214c9650c2dc3332f` |
| `s1r2_p1_perf_operational_esrestart.log.meta` | `ff2ff88f7386b786b85f458e1a02803c2e5604fc32f300990b9267c8c8a4a637` |
| `s1r2_p1_persist_before.txt` | `cd0fa10509b935d94589ed9f2c76470975d8d9e7cfda72b35883a2ab963131ae` |
| `s1r2_p1_persist_after.txt` | `b1c50a6b30665d74cf0852bc27e0dc7bee2dc93812198134247f3e00483865bb` |

입력: 프로필 `kairos/profiles/cfs-v7.0.1/observe/sites.kzp`(SHA-256 `504c66f9…05a6`), cFS 번들 `088b2fa8…` 의 `build-native_std/exe/cpu1/core-cpu1`(위 표).

컨테이너 읽기: `kz-dump s1r2_p1.kzt` (정규 dump), `kz hits s1r2_p1.kzt …`. 컨테이너는 `verify_content` 를 통과한다(`kz hits` 가 읽을 때마다 확인한다).

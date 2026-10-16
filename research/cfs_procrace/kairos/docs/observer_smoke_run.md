# 관측기 연습 실행 기록 (sub-gate S1-R2, 실행 한 번)

- 날짜: 2026-10-15 (UTC 11:17). 실행한 것: `kz observe` 로 수정하지 않은 cFS(`core-cpu1`)를 **한 번** 관측했다. 이 문서는 정확한 조건, 기준과 다른 점(deviation), 그리고 센 값을 적는다.
- **S1-R2 기능 확인의 상태: 미완(N=1/3)**. 규칙은 기능 확인에 기준 gdb 실행과 같은 횟수(3회)를 쓰고 그 이유로 deviation 을 적으라고 한다. 이 실행(s1r2_p1, 순번 0)은 그 세 번 가운데 첫 번째일 뿐이다. 나머지 두 번(s1r2_p2, s1r2_p3; 순번 1, 2)은 cFS 를 돌릴 수 있는 단계(Integration)가 고친 스크립트로, 한 번에 하나씩 돌려야 한다(§9). 그 전까지 아래의 "개수가 기준과 같다"는 **한 실행의 값**이다.
- **이 실행이 보여 주는 것**: 관측기가 실제 cFS 에 붙고, 커널이 손실을 보고하지 않은 채 사건을 받고, thread 정체성을 GetAppID 없이(시간 포함에 기대어; §4.1) 결합한다는 것.
- **이 실행으로 주장하지 않는 것**: 빈도, 분포, 재현성, 관측 비용(오버헤드), gdb 실행과의 동등성, R2 의 판정. 실행이 한 번이다(기준 gdb 실행은 세 번). 분석은 K3 의 몫이고 여기서는 개수만 센다.
- **2026-10-16 검토 반영**: 실행의 산출물(`env/logs/s1r2_p1*`)은 **고치지 않았다**. 다시 쓰면 컨테이너의 해시와 도구 해시가 이 실행의 것이 아니게 된다. 검토에서 드러난 기록의 빈틈과 잘못된 표현은 이 문서에서 바로잡는다: 환경 deviation 이 기록에 없음(§3 항목 5), RUNMETA 의 층 값(§3 항목 4), 정체성 결합의 표현(§4.1), `/proc` 읽기와 PERTURBATION(§4.2), "저장보다 앞섬"의 표현(§5), 전체 구간 개수 차이의 조사(§5.1), 도구의 출처(§8).
- 표기: **[M]** 이번에 실행해 확인한 사실, **[D]** 설계 결정, **[I]** 계획·미확인.
- 붙이기에 실패하지 않았으므로 블로커는 없다. 실패했다면 오류를 기록하고 멈추기로 했었고(gdb 나 수정한 바이너리로 대체하지 않는다), 그 경로는 쓰이지 않았다.

산출물은 모두 `env/logs/` 에 접두어 `s1r2_p1_` 로 있다(새 파일뿐이고 기존 파일은 바꾸지 않았다). 파일과 SHA-256 은 §7.

## 1. 정확한 명령

당시의 `probe/scripts/observe_cfs_smoke.sh s1r2_p1` 이 아래를 순서대로 했다. root 에서, 환경 변수 `KZ`(= `kz` 실행 파일 경로)와 `CFS_NORMAL_USER=ubuntu` 를 주고 시작했다. **이 때문에 `KZ` 가 cFS 의 환경에 들어갔다**(§3 항목 5). 지금의 스크립트는 `kz` 경로를 인자로 받고(`observe_cfs_smoke.sh <tag> <kz> <run-ordinal>`) `KZ` 를 지운다. 아래는 s1r2_p1 에 쓴 그대로의 명령이다.

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

항목 1–4 는 `--conditions` 로 컨테이너 DEVIATIONS 에도 들어 있다(`s1r2_p1_conditions.tsv`; `kz-dump` 로 보면 DEVIATIONS 는 `observer.method`, `run.count`, `host.kernel_vs_reference`, `run.stratum` 넷이다). **항목 5 는 컨테이너에도 조건 파일에도 없고 이 문서에만 있다** — 처음 보고에서 "컨테이너와 이 문서에 있다"고 한 것은 틀렸다. 실행 기록은 고치지 않으므로 s1r2_p1 의 환경 deviation 은 이 문서가 유일한 기록이다. 지금의 스크립트는 조건 파일에 `cfs.environment` 항목과 (다르면) deviation 을 실행 전에 쓰고, 실행 뒤 cFS 가 실제로 받은 이름(`.meta`)과 비교해 driver 에 적는다. 값, 이유, 영향, 피할 수 있었는지.

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
   - **RUNMETA 와의 어긋남**: 스크립트가 `--stratum cold` 를 주었으므로 컨테이너의 기계가 읽는 값 `RUNMETA.stratum` 은 `cold` 이다. 같은 컨테이너의 deviation 은 층이 확립되지 않았다고 말한다. 뒤의 분석은 s1r2_p1 의 층을 **확립되지 않음**으로 다뤄야 한다(RUNMETA 값 `cold` 를 쓰지 않는다). 지금의 스크립트는 `--stratum none` 을 주고, 조건 파일의 `run.stratum` 을 deviation(기준: 부팅 뒤 첫 실행, 쓴 값: 확립되지 않음, 이유: 앵커와 검증을 위해 실행 전에 바이너리를 읽어야 함)으로 적는다. `kz observe --help` 도 확립하지 못한 층은 `none` 으로 주라고 말한다.
5. **하네스 환경이 cFS 로 상속됨** (컨테이너·조건 파일에 **없음**)
   - 기준: r2_g1…g3 의 `.meta` 에 있는 변수 이름 152개(세 실행 모두 같은 집합).
   - 이번: 151개. 기준에만 있는 이름 둘(`CLAUDE_CODE_REMOTE_TOOLS_FORWARD`, `R2_TRACE_OUT`), 이 실행에만 있는 이름 하나(`KZ`).
   - 이유: `run_cfs.sh` 는 호출자의 환경을 그대로 넘긴다(README 방식). 하네스 세션의 변수 집합이 기준 실행 뒤 바뀌었고, `KZ` 는 이 스크립트가 경로를 환경 변수로 받았기 때문에 들어갔다.
   - 영향: cFS 는 `TERM`, `TMPDIR`, `SHELL` 만 읽는다고 `env/CONDITIONS.md` E12 에 있고, 이 세 값은 기준과 같다(`TERM=linux`, `SHELL=/bin/bash`, `TMPDIR` 없음). 다른 이름의 영향은 없을 것으로 예상하나 검증하지 않았다.
   - 피할 수 있었는지: `KZ` 는 피할 수 있었다(지금의 스크립트는 경로를 인자로 받고 `env -u KZ` 로 시작한다). 하네스 세션의 이름 차이는 이 절차로는 피할 수 없다.

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
- 24개가 task 생성 호출과 **정확히 하나씩** 결합되었다. 모호한 결합 0, FORK 가 없는 생성 호출 0, 복귀가 없는 생성 호출 0, 생성 호출이 없는 복귀 0, 결합되지 않은 id 알림 0, 사건을 가졌으나 thread 기록이 없는 사건 0.
- **결합의 근거는 하나뿐이다** (처음 보고의 "세 가지 독립 증거"라는 표현을 바로잡는다): 어느 생성 호출이 어느 thread 를 만들었는지는 "커널의 FORK 기록(생성자 = 그 호출의 thread)이 그 호출의 [진입, 복귀] 시각 구간 안에 있다"는 **시각 포함 하나**로 정했다. 비교한 시각은 같은 thread(생성자)의 것이므로 시계 가정 A-CLK-PO 아래에서 성립한다. 새 thread 가 알린 id 와 커널 이름은 tid 로 붙여 **기록했을 뿐 생성자 쪽과 대조하지 않았다**. SPEC 5.7 항목 2 가 드는 생성자 쪽 증거(`OS_TaskCreate` 의 출력 인자, 배정된 id)는 perf-sample 이 읽지 못한다. 그래서 알린 id 24개는 **검증되지 않음**이다. 지금의 관측기는 이 둘을 `match-by-time`, `id-unverified` 표시와 `identity.matched_by_time`, `identity.ids_unverified` 개수로 남긴다. s1r2_p1 의 컨테이너에는 이 표시가 없다(그 전의 관측기로 썼다).
- 알린 id 는 `0x10001`부터 `0x10018` 까지 24개이고, 새 thread 의 첫 함수는 모두 같은 진입 함수(`CFE_ES_TaskEntryPoint`)로 기호 해석되었다. 생성자는 대부분 주 thread, 일부는 task 가 만들었다(예: `ES_BG_TASK` ← `CFE_ES`, `TIME_TONE_TASK`·`TIME_ONEHZ_TASK` ← `CFE_TIME`, `FM_CHILD_TASK` ← `FM`).
- 이름이 첫 사건보다 늦게 정해진 경우가 24건이다(thread 가 이름을 붙이기 전에 id 를 알리므로 예상된 것).
- 모든 27개 thread 의 `/proc` 기록을 읽었다(FORK 기록 뒤 2.4–11 ms 에). 정책은 전부 `SCHED_OTHER`, rt 우선순위 0, nice 0. 8개는 허용 CPU 가 `0`(CPU 0 에 고정), 19개는 `0-3`. (고정은 PSP 의 일이다. 이 실행은 이를 바꾸지 않았다.)

### 4.2 관측기의 `/proc` 읽기와 PERTURBATION (검토 반영)

- 위의 읽기는 cFS 가 시작하며 thread 를 만드는 동안 비움 thread 가 한 것이다. 첫 표본에서 프로세스의 `status`, `cmdline`, `exe`·`cwd` 링크, `environ`, `cgroup`, `limits` 와 thread 목록을 읽었고, thread 마다 `stat`, `status` 를 읽었다. `environ`, `cmdline` 은 대상의 메모리를 복사하므로 대상의 주소 공간 잠금을 잡는다(cFS 가 thread 스택을 만드는 중에). 이것은 관측기의 행동이며 D1 이 관측 효과로 따로 세라고 한 것이다.
- s1r2_p1 의 컨테이너 PERTURBATION 에는 이 행이 **없다**(프로브 적중, 관측기 CPU, 수명만 있다). 읽은 항목 수는 코드로부터 **계산한 값**으로 1 + 1 + 2 + 1 + 1 + 1 + 1(프로세스) + 27 × 2(thread) = 62 이고 [I], 그 읽기에 쓴 시간은 재지 않았다. thread 읽기는 각 FORK 기록 뒤 2.4–11 ms 에 있었다(기록의 `read_after_record_ns`) [M]. 영향은 재지 않았다.
- 지금의 관측기는 읽은 수와 그 시간을 PERTURBATION `kind=proc-capture`(`count`, `ns`)에, 모드(`full`/`status`)와 명령 시작 기준의 시간 창을 기록(`observer.proc_capture.*`)에 남긴다. 다음 실행도 s1r2_p1 과 같은 `full` 모드로 돈다(같은 절차).
- 같은 PERTURBATION 의 프로브 행: s1r2_p1 에서는 복귀 사이트 1, 4, 6(`evs.send.ret` 271, `es.sync.ret` 7, `osal.task_create.ret` 24)도 `cost_class=emulated` 로 적혔다. 그러나 복귀 사이트는 진입 사이트와 **같은 주소의 중단점을 공유**하므로 진입 트랩은 진입 행(사이트 0, 3, 5)에서 이미 세어졌고, 복귀 행의 몫은 uretprobe 트램펄린이다. 이 세 행으로 트랩 비용을 다시 세면 두 번 세는 것이 된다. 지금의 관측기는 복귀 행을 `cost_class=ret-trampoline` 으로 쓰고, 사이트마다 트랩이 어느 행에서 세어지는지를 `perturbation.site<N>.trap` 으로 적는다(`observer.md` §10).

## 5. 개수 (분석이 아니다)

`kz hits` 의 출력이다. 같은 컨테이너에서 다시 만들 수 있다.

**`CFE_EVS_SendEventWithAppID` 진입 가운데 시각이 `sb.appid.store` 의 첫 사건보다 앞선 것: 24개.** 표현을 정확히 하면(검토 반영): `sb.appid.store` 는 `cfe_sb_task.c:138` 의 `call CFE_ES_GetAppID` **다음 명령**(addr2line 기준 142 줄)에 놓인 프로브이다. 저장 자체(`*AppIdPtr = ...`)는 `CFE_ES_GetAppID` 안에서 일어나므로, 이 프로브의 시각은 저장 시각의 **상한**이다. 그리고 여기서 비교한 것은 서로 다른 thread 의 사건을 한 시계(`CLOCK_MONOTONIC`)로 늘어놓은 순서이다. 그래서 이 24개는 "**저장하는 호출에서 돌아온 시각보다 timestamp 가 앞선 사건**"이고, 한 시계의 증거일 뿐 happens-before 가 아니다(SPEC 5.8). 이 실행에서는 24개 모두의 `app_id` 인자가 0 이라는 것이 이 읽기와 맞지만, 순서 자체가 그것을 보이는 것은 아니다. (처음 보고의 "저장보다 앞선 24개"는 이 점에서 과장이었다.)

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

**전체 구간**: `CFE_EVS_SendEventWithAppID` 진입 271개(복귀 271개). 기준 gdb 실행의 전체 호출자 합계는 251, 257, 257 이다. **다르다**(이번 271). 맞추려고 아무것도 바꾸지 않았다. 조사는 §5.1.

### 5.1 전체 구간 개수 차이의 조사 (검토 반영, 2026-10-16)

조건은 하나도 바꾸지 않았고 cFS 를 다시 돌리지 않았다. 같은 컨테이너를 읽고(`kz hits s1r2_p1.kzt --site evs.send.entry --basis s1r2_p1_basis.kz --group-caller --group-slot event_id`; 호출 함수 × event_id, thread × 호출 함수 표는 검토 반영 뒤의 `kz hits` 가 낸다) 기준의 `r2_g*_trace.jsonl` 에서 `SEND_WITH_APPID` 사건을 `jq` 로 (`stack[1].func`, `event_id`, `thread_name`) 별로 셌다.

| 호출 함수 | event_id | 이번(s1r2_p1) | r2_g1 | r2_g2 | r2_g3 |
|---|---|---|---|---|---|
| `CFE_SB_CreatePipe` | 5 | 26 | 26 | 26 | 26 |
| `CFE_SB_SubscribeFull` | 10 | 111 | 111 | 111 | 111 |
| `CFE_SB_SubscribeFull` | 23 | 2 | 2 | 2 | 2 |
| `CFE_TBL_LoadFinish` | 35 | 28 | 28 | 28 | 28 |
| `CFE_SB_MessageTxn_ReportSingleEvent` | 14 | 28 | 38 | 38 | 37 |
| `CFE_SB_MessageTxn_ReportSingleEvent` | 25 | 39 | 24 | 26 | 26 |
| `CFE_SB_SendSubscriptionReport` | 22 | 37 | 22 | 26 | 27 |
| 합계 | | **271** | 251 | 257 | 257 |

- 차이(+20, +14, +14)는 **세 (호출 함수, event_id) 묶음에만** 있다 [M]: event 22(`CFE_SB_SendSubscriptionReport`, 모두 `TO_LAB`) +15/+11/+10, event 25(`CFE_SB_MessageTxn_ReportSingleEvent`) +15/+13/+13, event 14(같은 함수) −10/−10/−9. 나머지 네 묶음(165개)은 네 실행에서 같다.
- event 22: 이번 37개, 기준 22/26/27 개이고 모두 `TO_LAB` thread 이다. `TO_LAB` 의 구독(`CFE_SB_SubscribeFull`, event 10)은 이번과 r2_g1 모두 39번이다(r2_g1 의 스택으로는 2번이 `TO_LAB_init`, 37번이 `TO_LAB_UpdateSubscriptionsFromTable`). 기준 gdb 스택으로 보고는 `TO_LAB_UpdateSubscriptionsFromTable → CFE_SB_SubscribeEx → CFE_SB_SubscribeFull → CFE_SB_SendSubscriptionReport` 에서 나온다(r2_g1 의 22개 모두) [M]. 즉 구독 수는 같고, 구독 뒤에 보고가 나간 횟수가 다르다(이번에는 37번 모두, 기준에서는 일부).
- event 25: 이번에는 39개 모두 `TO_LAB` thread 에서 나왔다. 기준에서는 `TO_LAB` 16/21/22 와 `CS`, `DS`, `MD`, `SBN` 8/5/4 였다 [M]. 기준 gdb 스택으로는 `CFE_SB_TransmitMsg → CFE_SB_MessageTxn_ReportEvents → CFE_SB_MessageTxn_ReportSingleEvent` 경로이다(전송 쪽 thread 에서 나온다).
- event 14: 이번에는 `CFE_ES` 17, `CFE_EVS`·`CFE_SB`·`CFE_TBL`·`CFE_TIME`·`FM`·`SAMPLE_APP`·`TO_LAB` 각 1, `DS`·`LC` 각 2. 기준(r2_g1)에는 이 밖에 `CF`, `CI_LAB`, `MD`, `MM` 각 1, `FM` 이 1 더, `HS` 3, `LC` 가 2 더 있었다 [M]. `CFE_ES` 17 과 `CFE_EVS` 1(R2 의 사건)은 네 실행에서 같다.
- 해석(측정하지 않은 가설 [I]): 세 event 는 모두 "어떤 메시지가 보내지는 순간 다른 앱의 상태가 어떠한가"에 달린 결과이다 — 구독 보고가 켜져 있는가(22), 받는 pipe 가 찼는가(25), 받는 구독자가 아직 없는가(14). gdb 는 중단점마다 프로세스 전체를 멈추므로(기준 실행에서 OPERATIONAL 까지 2.2–2.7 s, 이번 1.66 s) 앱 사이의 시작 순서가 다르다. event 의 이름(22, 25 가 어느 `_EID` 인지)과 보고가 켜지는 조건은 cFE 원문으로 확인하지 않았다(이 단계는 `cfs_ref` 를 읽지 않는다).
- 결론: 전체 구간의 차이는 시작 시각에 따라 달라지는 세 묶음의 개수 차이로 설명되고, R2 와 관련된 개수(§5 의 24개, `CFE_ES` 17·`CFE_EVS` 1 의 event 14 포함)는 네 실행에서 같다. 이것은 실행 한 번과 기준 세 번의 비교이고, 위 가설은 검증하지 않았다. 다음 두 실행(§9)의 같은 표가 이 묶음들의 실행 간 변동을 보여 줄 것이다.

참고로 이번 실행의 thread 별 진입 수(전체 구간)는 `TO_LAB` 119, `DS` 21, `CFE_ES` 20, `HK` 11, `HS` 10, `CF` 9, `SC` 9, `CS` 8, `CFE_TIME` 8, `LC` 8, `MD` 8, `SBN` 7, `CFE_SB` 5, `FM` 5, `SAMPLE_APP` 5, `CFE_EVS` 4, `CFE_TBL` 4, `CI_LAB` 4, `MM` 3, `SCH_LAB` 3 이다. `event_type` 별은 1 → 204, 2 → 28, 3 → 39.

## 6. 한계와 알려진 문제

- 한 번의 실행이다. 이 숫자들이 다음 실행에서 같은 값이라고 기대할 근거가 없다.
- `kz hits` 의 "앞섬"은 한 시계(`CLOCK_MONOTONIC`)의 병합 순서이다. 서로 다른 thread 사이의 happens-before 가 아니다. 점 사이트(`sb.appid.store`)의 시각은 저장 시각의 상한이다(§5).
- 실제 바이너리의 앵커를 `objdump`·`addr2line` 으로 본 결과는 저장되지 않았다(`observer.md` §5). 다음 실행부터 `<tag>_anchor_xcheck.txt` 로 남는다.
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

컨테이너 읽기: `kz-dump s1r2_p1.kzt` (정규 dump), `kz hits s1r2_p1.kzt …`. 컨테이너는 `verify_content` 를 통과한다(`kz hits` 가 읽을 때마다 확인한다). 검토 반영 뒤의 도구도 이 컨테이너를 읽는다(바뀐 스키마는 열거 값과 이름뿐이고 레이아웃 버전은 그대로이다).

## 8. 도구의 출처 (검토 반영)

- 컨테이너(와 같은 실행의 basis)의 도구 해시는 `c9af043dbd587cf0caca3dabeec671535c0f0ee7b81d1477d0823a40edd24856` 이다. 도구 해시는 kz 를 이루는 원본 파일 전체의 해시(source identity)에서 나온다(`include/kz/core/identity.hpp`).
- 실행에 쓴 `kz` 는 `/home/user/work/kairos-build/k2-gcc/tools/kz/kz`, SHA-256 `e1ed2cbd…5518` 이다(driver). 그 파일은 실행 16분 뒤(11:33:57) 다시 빌드되어 덮였다(지금 SHA-256 `84b85f8b…`). 실행 뒤 `kz hits` 에 옵션을 더했고(§1), 다른 에이전트도 같은 원본 트리(`include/`, `lib/core` 등)를 고치고 있었다.
- 확인한 것 [M] (2026-10-16 에 각 빌드의 `kz anchor` 가 쓴 basis 의 헤더로 확인; 그 뒤 이 빌드들은 검토 반영으로 다시 빌드되었고 `k2-gcc` 는 디스크 공간 때문에 지웠다): 실행 뒤의 네 K2 빌드(`k2-gcc`, `k2-clang`, `k2-gcc-asan`, `k2-clang-ubsan`)의 도구 해시는 모두 `bfb4cbe7…` 이고, commit `6be8935` 의 `kairos/` 를 따로 빌드한 kz 의 도구 해시는 `faaee20a…` 이다. 어느 것도 `c9af043d…` 가 아니다. SHA-256 `e1ed2cbd…` 인 파일은 빌드 트리에 남아 있지 않다.
- 결론: **c9af043d 를 낸 원본 상태는 되찾을 수 없다.** 어느 commit 에도 대응하지 않고, 그 상태의 diff 도 남지 않았다. 이 실행의 개수(§4, §5)는 그 도구가 쓴 컨테이너에서 지금의 도구로 다시 읽은 값이고, 컨테이너 자체는 해시로 고정되어 있다. 그러나 컨테이너를 **쓴** 코드(해석기, 정체성 결합)는 원본으로 검토할 수 없다.
- 다음 실행부터: 스크립트가 실행 전에 저장소 commit 과 `kairos/` 의 diff(`<tag>_source.diff`, 추적되지 않은 파일 목록과 함께)를 남기고, kz 사본을 저장소 밖(`<빌드 루트>/runs/<tag>/kz`)에 두며, kz 의 원본이 kz 보다 새로우면(낡은 빌드) 시작하지 않는다.

## 9. 남은 두 실행 (S1-R2 기능 확인, N=3)

규칙: 기능 확인은 기준 gdb 실행과 같은 3회, 한 번에 하나, `env/run_cfs.sh` 를 통해, 같은 절차로. 실행 횟수는 deviation 으로 적는다(기준: D14 가 목적별로 S1 에서 정해 고정한다; 쓴 값: 3; 이유: 기준 gdb 실행 수). 이 단계(검토 반영)는 cFS 를 돌리지 않았다. cFS 를 돌릴 수 있는 단계가 아래를 한다:

```sh
# 같은 원본 트리에서 kz 를 새로 빌드한 뒤, root 로
CFS_NORMAL_USER=ubuntu kairos/probe/scripts/observe_cfs_smoke.sh s1r2_p2 <kz> 1
CFS_NORMAL_USER=ubuntu kairos/probe/scripts/observe_cfs_smoke.sh s1r2_p3 <kz> 2
```

그 다음 세 실행 각각의 `kz hits` 개수(§5 의 표, §5.1 의 표)를 나란히 적는다. 절차가 s1r2_p1 과 다른 점은 이 문서의 검토 반영 사항(KZ 인자, `--stratum none`, 환경 deviation, 실행 뒤 앵커 교차 확인, 도구 출처 기록)이고, 관측 자체(사이트, 프로필, `/proc` 읽기 모드 `full`, ring 크기)는 같다. 관측기 코드는 이 사이에 바뀌었으므로(PERTURBATION 의 복귀 행·`proc-capture` 행, THREADS 의 표시) 세 컨테이너의 도구 해시는 같지 않다.

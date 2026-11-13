# R2 공통 시작 조건 — Power-On 명시 (2026-10-28)

- 결정: `Architecture/spec/PI_DECISIONS_RESOLVED_20261028.md` Q3. R2는 Power-On을 명시한 공통 시작 상태를 쓴다. EEPROM 초기 이미지와 공유 메모리 초기 조건을 통일하고, 실제 시작 리셋 유형을 확인한다. F0와 계측 빌드에 같은 절차를 쓴다.
- 이 문서는 절차와 근거만 적는다. 실행 결과는 그 실행의 결과 문서에 적는다.
- 소스 기준: ENV 번들 `cfs_ref/cFS`(PSP·OSAL은 번들 고정 판). 인용한 파일의 sha256 앞 16자리: `cfe_psp_start.c` `2780f38cf141e862`, `cfe_psp_memory.c` `20e7ad4609722d99`, `cfe_psp_support.c` `c102c3d3849cf612`, `cfe_psp_eeprom_mmap_file.c` `2b56c62ffab76c74`, `os-impl-filesys.c` `816e5169e10d01e5` [M, 2026-10-28].

## 1. 리셋 유형을 정하는 코드 [F]

| 단계 | 소스 | 내용 |
|---|---|---|
| 명령행 | `psp/fsw/pc-linux/src/cfe_psp_start.c` L136, L270–282 | `-R`/`--reset`는 `PO` 또는 `PR`만 받는다. 받으면 `CFE_PSP: Reset Type: <값>`을 찍는다(L281) |
| 기본값 | 같은 파일 L528–530 (usage) | "PO for Power On reset ( default )" |
| 결정 | 같은 파일 L408–434 | 옵션이 없으면 boot record가 유효(`VALID`/`INVALID`)할 때만 `NextResetType`을 쓴다. `PR`이면 PROCESSOR, 그 밖은 모두 POWERON(L429–433). 찍는 줄: `CFE_PSP: Starting the cFE with a POWER ON reset.` |
| 부속 유형 | 같은 파일 L569–576 | `-S`가 없으면 SubType 1. ES는 `POWER ON RESET due to Power Cycle (Power Cycle)`로 적는다 |
| 예약 메모리 | `cfe_psp_memory.c` L646–656 | POWERON이면 CDS·reset·user reserved 영역과 boot·exception 기록을 0으로 지운다. 새로 만든 segment도 이미 0이다(L641–645 주석) |
| 종료 | `cfe_psp_support.c` L62–69 | POWERON 재시작(ES Restart, 기존 정지 방법 `ci-es-restart-poweron`)은 예약 메모리 SHM을 지운다 |
| EEPROM | `psp/fsw/modules/eeprom_mmap_file/cfe_psp_eeprom_mmap_file.c` L41, L55, L62, L69 | `EEPROM.DAT`를 `O_CREAT`로 열고 512 KiB로 `ftruncate`한 뒤 `MAP_SHARED`로 매핑한다. 파일이 없으면 0으로 찬 이미지가 생긴다 |
| 키 파일 | `cfe_psp_memory.c` L611–615 | `.cdskeyfile`·`.resetkeyfile`·`.reservedkeyfile`을 `O_CREAT`로 만든다(빈 파일). SysV 키는 `ftok(<파일>,'R')`(L149, L328, L454) |
| RAM 디스크 | `osal/src/os/posix/src/os-impl-filesys.c` L223–235, L254–256 | 형식화는 아무것도 하지 않는다(no-op). 마운트 지점 `/dev/shm/osal:RAM`이 없으면 만든다. 그래서 남은 파일은 POWERON 뒤에도 남는다 |

옵션 없이 시작하면 리셋 유형이 남아 있는 SHM의 boot record에 달린다(L412–418). `--reset PO`를 주면 그렇지 않다. 이것이 "Power-On 명시"의 실행 수준 의미다.

## 2. 과거 R2 실행의 시작 상태와 리셋 유형 [M, 기록]

| 항목 | F0 v153_pilot3 (2026-10-24) | 계측 G-I d4 run1 (2026-10-26) |
|---|---|---|
| 명령 | `./core-cpu1` (옵션 없음, `.meta` L7, L70) | `./core-cpu1` (옵션 없음, run_cfs.sh v5) |
| 콘솔의 리셋 줄 | `Starting the cFE with a POWER ON reset.` (L25), `POWER ON RESET due to Power Cycle (Power Cycle).` (L29) | 같음 (`console.log` L25, L29) |
| 리셋 유형이 정해진 경로 | 옵션 없음 + SHM 없음 → boot record 무효 → L429–433의 기본 POWERON | 같음 |
| `EEPROM.DAT` 실행 전 | 있음. 524,288 B, 0이 아닌 바이트 0, sha256 `07854d2f…1541` | 없음 |
| `EEPROM.DAT` 실행 후 | 같은 해시 | 524,288 B, sha256 `07854d2f…1541` (PSP가 만든 이미지) |
| 키 파일 실행 전 | 셋 다 있음, 크기 0 (2026-10-08 생성) | 셋 다 없음 |
| SysV shm 실행 전 | 없음 | 없음 |
| `/dev/shm/osal:RAM` 실행 전 | 빈 디렉터리 | 빈 디렉터리 |

출처: `v153_pilot3_persist_before.txt`·`_after.txt`, `instr/gcc/d4/r2/run1/persist_before.txt`·`persist_after.txt`, 각 콘솔 기록.

결론 [M]: 두 실행 모두 POWER ON reset(부속 유형 Power Cycle)이었다. EEPROM 이미지의 내용은 같았다(같은 sha256). 다른 것은 상태 파일이 미리 있었는지와, 그에 따른 키 파일의 inode·ftok 키 값뿐이다. 그래서 `STATUS_20261027.md`의 "F0 pilot3는 보존 파일 있음, 계측 실행은 새 설치"는 파일 존재의 차이로 읽어야 한다. EEPROM 내용의 차이로 읽으면 안 된다. 호출 수 차이(269 대 272)가 이 차이에서 왔는지는 이 기록만으로 알 수 없다.

## 3. 공통 절차 (F0와 계측 빌드 같음)

| 순서 | 절차 | 근거 | 지위 |
|---|---|---|---|
| 1 | 다른 cFS·OSAL 활동이 없는지 확인한다(run_cfs.sh 격리 검사) | `env/CONDITIONS.md` E13 | 기존 절차 |
| 2 | `env/tools/start_state_poweron.sh <exe_dir> <record_dir> <run_user>`: `EEPROM.DAT`와 키 파일 셋을 실행 기록 안의 `prior_state/`로 옮기고(해시 기록), 실행 사용자의 SysV segment가 있거나 `/dev/shm/osal:RAM`이 비어 있지 않으면 시작하지 않는다 | §1의 EEPROM·키 파일·RAM 디스크 행 | 결과 상태는 from-reference(PSP·OSAL이 만드는 상태). 옮기는 방식은 harness 선택(H-11) |
| 3 | `RUN_CFS_ARGS="--reset PO"`로 run_cfs.sh v6를 부른다. 모드 `operational`, 정지 `ci-es-restart-poweron`은 기존 R2 절차와 같다 | §1 명령행·기본값 행. 정지는 `env/run_cfs.sh` 머리말 | `--reset PO`는 README L112(옵션 없음)에서의 **이탈**이며 PI 결정이다 |
| 4 | 콘솔에서 `CFE_PSP: Reset Type: PO`, `Starting the cFE with a POWER ON reset.`, ES의 `POWER ON RESET due to Power Cycle (Power Cycle).`를 확인한다. 하나라도 없으면 그 실행은 이 조건의 실행으로 세지 않고 기록만 한다 | §1 | 확인 항목 |
| 5 | 실행 뒤 `persist_snapshot.sh`로 `EEPROM.DAT`가 524,288 B, sha256 `07854d2f…1541`인지 확인한다 | §2 | 확인 항목 |

통일되는 것: EEPROM 이미지(0으로 찬 524,288 B, 같은 해시), 키 파일(PSP가 새로 만듦), SysV 예약 메모리(새로 만들어지고 POWERON으로 지워짐), RAM 디스크(비어 있음), 리셋 유형과 그것이 정해지는 경로(명시 옵션).

## 4. 과거 기록과 새 조건의 차이 (기록 의무, Q3)

| 항목 | 과거 F0 pilot3 | 과거 G-I | 새 조건 |
|---|---|---|---|
| 리셋 유형 | POWERON | POWERON | POWERON |
| 정해진 경로 | 기본값(boot record 없음) | 기본값 | 명시 옵션 `--reset PO` |
| EEPROM 파일 | 미리 있음(내용 0) | 없음 → 생성 | 없음 → 생성 |
| 키 파일 | 미리 있음 | 없음 → 생성 | 없음 → 생성 |
| 명령행 | 옵션 없음 | 옵션 없음 | `--reset PO` (README L112 이탈) |

과거 pilot은 새 조건의 결과와 구분해 보존한다. 새 결과와 섞어 세지 않는다(Q3, AD §6.2).

## 5. 범위

- C1은 바꾸지 않는다. C1의 시작 상태는 기준 CI 절차(새 untar, 상태 파일 없음)가 정하고 F0·계측이 이미 같다(`plan/results_instr_pilot_20261026.md` §0 시작 상태 행).
- Processor Reset, CDS 등 상태 보존 시험은 후속 시나리오다(Q3).
- 같은 R2 실행 하나는 그 실행 경로의 가능성 증거이고 빈도의 증거가 아니다(P8).

# C5 큐 요구 목록 — cFE 6.4.1/6.4.2 cpu1 시작 경로 (2026-10-29)

- 성격: **소스 읽기와 계산**이다 [F][I]. 이 컨테이너의 한도는 읽기만 했다 [M]. cFS를 빌드·부팅하지 않았고, 큐를 만들지 않았고, sysctl·rlimit를 쓰지 않았다.
- 결정: `Architecture/spec/PI_DECISIONS_RESOLVED_20261029.md`(이하 PD29) D1·A3·A4·N-C5-1. D1은 "namespace 안의 값은 실제 빌드 구성에서 유도한다. 대상은 6.4.1·6.4.2에서 실제로 만들어지는 모든 큐(TO_LAB 포함)이고, 두 판에 공통으로 적용한다. 메시지 크기·큐 수·`RLIMIT_MSGQUEUE`도 확인한다"고 적는다. A3는 "요구량이 한도를 넘으면 환경 차단으로 보고한다. 깊이를 임의로 줄여 맞추지 않는다"고 적는다.
- 입력: `plan/c5_execution_basis_20261028.md`(이하 CB) §1.1–§1.3, §3.2, §3.5; `plan/case_setup_C2_C3_C5.md`(이하 CS) 부록 A.1; `cases/C5.md` §10; `REVIEW_ANALYSIS_20261029.md` §2(CR §9.3 큐 값 항목); `CONDITIONS_POLICY.md`; `env/CONDITIONS.md` D-4.
- 표기: [F] 해시를 적은 tarball·고정 태그·고정 판 문서에서 읽음, [M] 이번 작업에서 도구로 얻음, [I] 추론, [D] 결정, [U] 미확인. 줄 번호는 해당 파일의 줄이다. `6.4.1:`·`6.4.2:`는 각 판 tarball의 `cFE-6.4.x-OSS-release/` 아래 경로, `OSAL:`은 `osal-4.1.1-release/` 아래 경로다.
- 작업 공간(저장소 밖, 2026-10-29에 새로 만듦): `/home/user/work/c5/{dl,src,ref}`. 내려받은 파일은 `dl/`, 압축 해제본은 판마다 다른 빈 디렉터리(`src/641`, `src/642`, `src/osal`), 참고 원문은 `ref/{kernel,man,glibc}`에 있다. 해시는 각 디렉터리의 `sha256.txt`.

## 0. 요약

1. **출처** [M]. 세 tarball을 CS 부록 A.1의 URL에서 다시 받았다(2026-10-29T02:44Z). 바이트 수와 sha256이 기록과 같다(§1).
2. **큐 목록** [F]. cpu1 시작 경로가 만드는 큐는 판마다 **10개**이고, 두 판에서 같다. 모두 SB 파이프이고, 모두 `CFE_SB_CreatePipe` → `OS_QueueCreate` 한 경로로 만들어진다(§2).
3. **최대 깊이는 64**다 [F]. TO_LAB의 텔레메트리 파이프(`to_lab_app.c` L160, L187)다. CB §3.5가 후보로 적은 32는 TO_LAB을 읽기 전의 값이었다. 깊이의 합은 244다.
4. **메시지 크기는 4 B**다 [F][I]. 모든 큐가 `sizeof(CFE_SB_BufferD_t *)`를 넘기고(`cfe_sb_api.c` L214), 빌드는 `-m32`다.
5. **큐 수** [F]. 10개는 `queues_max` 256, OSAL `OS_MAX_QUEUES` 64, `CFE_SB_MAX_PIPES` 64 모두보다 작다.
6. **바이트** [F][M][I]. 커널의 계산식(`ipc/mqueue.c` L365–L371, getrlimit(2) `RLIMIT_MSGQUEUE`)과 이 커널의 구조체 크기(BTF: `struct msg_msg` 48 B, `struct posix_msg_tree_node` 48 B)로 셈하면 큐 하나가 깊이 × 100 B이다. 합은 **24,400 B**이고 한도 819,200 B의 3.0%다. **환경 차단은 없다**(A3).
7. **깊이 조정** [F]. OSAL·cFE 어느 쪽도 깊이를 바꾸지 않는다. SB는 1..256 밖이면 거부만 한다. OSAL 4.1.1의 큐 생성 경로에는 root 판별이 없다. 한도 검사의 차이는 커널이 `CAP_SYS_RESOURCE`로 가른다. 이 컨테이너의 root에게는 그 권한이 없으므로, 큐 한도 검사는 root와 일반 사용자에게 같다(§3, §4).
8. **유도한 실행 조건** [D: D1 적용, 구성에서 유도한 값]. C5 실행의 IPC namespace 안에서 `msg_max`를 64로 둔다. `msgsize_max`(8192)와 `queues_max`(256)는 새 namespace의 기본값으로 충분해 바꾸지 않는다. 바깥 `msg_max` 10(D19)과 `RLIMIT_MSGQUEUE`는 그대로 둔다(§7).

## 1. 출처와 해시 [M, 2026-10-29T02:44Z]

기록: `/home/user/work/c5/dl/{fetch_log.txt,sha256.txt,source_hashes.txt}`. 리다이렉트 URL의 질의 문자열은 기록하지 않았다(`<query not recorded>`).

| 파일 | URL (CS 부록 A.1) | HTTP | 바이트 | sha256 | 기록과 |
|---|---|---|---:|---|---|
| `cFE-6.4.1-OSS-release.tar.gz` | `https://sourceforge.net/projects/coreflightexec/files/cFE-6.4.1/cFE-6.4.1-OSS-release.tar.gz/download` | 200 | 39,586,940 | `ec26e33b4ab40b9616ccdbc984c5a00a790a7d05b260f2628c3335a29c23e98f` | 같음 |
| `cFE-6.4.2-OSS-release.tar.gz` | `https://sourceforge.net/projects/coreflightexec/files/cFE-6.4.2-OSS-release.tar.gz/download` | 200 | 36,411,531 | `c64ed2aac910e79a641f49279dfdff3b7b49374e54ed56b595d47e16a3f53a4f` | 같음 |
| `osal-4.1.1-release.tar.gz` | `https://sourceforge.net/projects/osal/files/OSAL-4.1.1/osal-4.1.1-release.tar.gz/download` | 200 | 1,973,805 | `8f0f2b23d8226256fdb8543bf9fcb560b38434411dc12cb4f7f35b9c19e61d46` | 같음 |

압축을 푼 주요 파일의 sha256도 CS 부록 A.4·CB §1.2의 기록과 같다. 6.4.1 `cfe_es_apps.c` `0a54974ba77af3db…`, `cfe_es_api.c` `0cb4c1c740f897a3…`, 6.4.2 `cfe_es_apps.c` `2a1e2f4ce32d05a9…`, `cfe_es_api.c` `8a67d0502452c0c6…`, OSAL posix `osapi.c` `32649c6f1cf3b586…`.

## 2. cpu1 시작 경로가 만드는 큐 [F]

### 2.1 무엇이 시작되는가

- 빌드되는 앱: `build/cpu1/Makefile` L42 `THE_APPS := sample_lib sample_app`, L43 `THE_APPS += ci_lab to_lab sch_lab`(두 판 같음).
- 시작 스크립트: `build/cpu1/exe/cfe_es_startup.scr` L1–L5의 `CFE_APP` 다섯 줄(`SAMPLE_LIB`, `SAMPLE_APP`, `CI_LAB_APP`, `TO_LAB_APP`, `SCH_LAB_APP`)이다. 빌드가 이 파일을 `exe/cf/apps`로 복사하고(`build/cfs.mak` L263, `CFS_PROM_APPS` `build/cpu1/Makefile` L89–L91), ES가 `CFE_ES_NONVOL_STARTUP_FILE` `"/cf/apps/cfe_es_startup.scr"`(`cfe/fsw/platform_inc/cpu1/cfe_platform_cfg.h` L856)를 연다. 두 판의 스크립트는 바이트로 같다 [M: `cmp`].
- 6.4.2 readme L183–L190도 같은 다섯 개("SCH_LAB, CI_LAB, TO_LAB, SAMPLE_LIB, SAMPLE_APP")가 "integrated and running"이라고 적는다.
- 코어 앱: ES, EVS, SB, TIME, TBL. 각 task의 초기화가 명령 파이프 하나씩을 만든다.
- 앱 소스는 두 판에서 같다(`diff -rq apps` 차이 없음) [M].
- 사용하는 설정 헤더: `CFE_CORE_PLATFORM_INC_SRC:=cpu1`(`build/cpu1/Makefile` L63). `make config`가 `cfe/fsw/platform_inc/cpu1/*.h`와 `$(OSAL)/build/inc/osconfig.h`를 `build/cpu1/inc`로 복사한다(`build/cfs.mak` L43, L45).

### 2.2 큐 표

모든 큐는 `CFE_SB_CreatePipe`(`cfe/fsw/cfe-core/src/sb/cfe_sb_api.c` L171)가 `OS_QueueCreate(&SysQueueId, PipeName, Depth, sizeof(CFE_SB_BufferD_t *), 0)`(L214)로 만든다. 두 판에서 이 줄 번호는 같다. 앱 행의 줄 번호는 두 판에서 같다. 코어 행은 "6.4.1 / 6.4.2"로 적는다.

| # | task | 생성 코드 (file:line) | 파이프 이름 (출처) | 요청 깊이 (출처) | `OS_QueueCreate`에 넘기는 크기 |
|---:|---|---|---|---:|---|
| 1 | CFE_ES | `es/cfe_es_task.c` L357 / L365 (`CFE_ES_TaskInit`) | `"ES_CMD_PIPE"` (L312 / L320) | 12 (`CFE_ES_TaskData.PipeDepth = 12`, L313 / L321) | `sizeof(CFE_SB_BufferD_t *)` |
| 2 | CFE_EVS | `evs/cfe_evs_task.c` L401–L402 / L410–L411 (`CFE_EVS_TaskInit`) | `CFE_EVS_PIPE_NAME` `"EVS_CMD_PIPE"` (`cfe_evs_task.h` L104) | 32 (`CFE_EVS_PIPE_DEPTH`, `cfe_evs_task.h` L100) | 같음 |
| 3 | CFE_SB | `sb/cfe_sb_task.c` L293–L295 / L301–L303 (`CFE_SB_AppInit`) | `CFE_SB_CMD_PIPE_NAME` `"SB_CMD_PIPE"` (`cfe_sb_priv.h` L137) | 32 (`CFE_SB_CMD_PIPE_DEPTH`, `cfe_sb_priv.h` L136) | 같음 |
| 4 | CFE_TIME | `time/cfe_time_task.c` L308–L310 / L316–L318 (`CFE_TIME_TaskInit`) | `CFE_TIME_TASK_PIPE_NAME` `"TIME_CMD_PIPE"` (`cfe_time_utils.h` L108) | 12 (`CFE_TIME_TASK_PIPE_DEPTH`, `cfe_time_utils.h` L109; 대입 `cfe_time_utils.c` L265) | 같음 |
| 5 | CFE_TBL | `tbl/cfe_tbl_task.c` L301–L303 / L309–L311 (`CFE_TBL_TaskInit`) | `CFE_TBL_TASK_PIPE_NAME` `"TBL_CMD_PIPE"` (`cfe_tbl_task.h` L78) | 12 (`CFE_TBL_TASK_PIPE_DEPTH`, `cfe_tbl_task.h` L79; 대입 L363 / L371) | 같음 |
| 6 | SAMPLE_APP | `apps/sample_app/fsw/src/sample_app.c` L95 (`SAMPLE_AppInit`) | `"SAMPLE_CMD_PIPE"` (L95) | 32 (`SAMPLE_PIPE_DEPTH`, `sample_app.h` L28) | 같음 |
| 7 | CI_LAB_APP | `apps/ci_lab/fsw/src/ci_lab_app.c` L152 (`CI_TaskInit`) | `"CI_LAB_CMD_PIPE"` (L152) | 32 (`CI_PIPE_DEPTH`, `ci_lab_app.h` L47) | 같음 |
| 8 | TO_LAB_APP | `apps/to_lab/fsw/src/to_lab_app.c` L177 (`TO_init`) | `"TO_LAB_CMD_PIPE"` (L159) | 8 (`PipeDepth = 8`, L158) | 같음 |
| 9 | TO_LAB_APP | `apps/to_lab/fsw/src/to_lab_app.c` L187 (`TO_init`) | `"TO_LAB_TLM_PIPE"` (L161) | **64** (`ToTlmPipeDepth = 64`, L160) | 같음 |
| 10 | SCH_LAB_APP | `apps/sch_lab/fsw/src/sch_lab_app.c` L82 (`SCH_Lab_AppMain`) | `"SCH_LAB_CMD_PIPE"` (L82) | 8 (L82의 상수) | 같음 |

- **SAMPLE_LIB**는 큐를 만들지 않는다(`apps/sample_lib/fsw/src/*.c`에 `CFE_SB_CreatePipe`·`OS_QueueCreate` 0건) [M: grep].
- **TO_LAB의 두 파이프**는 시작 직후 `TO_Lab_AppMain`(L99)이 부르는 `TO_init`(L105, 정의 L147)에서 만들어진다. 명령으로 만드는 것이 아니다.
- **검색 범위** [M]: 두 판의 `cfe/fsw`, `apps`, `psp` 아래 `.c` 파일 전체에서 `CFE_SB_CreatePipe`·`OS_QueueCreate`를 찾았다. 단위 시험 디렉터리와 빌드되지 않는 `to_lab_app.c.back`은 뺐다. 위 10개 말고는 SB 자신의 정의(L171)와 내부 호출(L214)뿐이다. OSAL·PSP는 큐를 만들지 않는다.
- **메시지 크기** [F][I]: `CFE_SB_BufferD_t`(`cfe_sb_priv.h` L160–L176)를 가리키는 **포인터**의 크기다. pc-linux 빌드는 `ARCH_OPTS = -m32`(`psp/fsw/pc-linux/make/compiler-opts.mak` L33)라 4 B다. 이 값은 CS §3.3 ③의 파싱 측정(`-m32`에서 `void*` = 4)과 맞는다.
- **이름 길이**: 가장 긴 이름이 `"SCH_LAB_CMD_PIPE"` 16자다. `OS_MAX_API_NAME` 20(`OSAL: build/inc/osconfig.h` L76)보다 짧다. 큐 이름 `"/<pid>.<name>"`의 버퍼는 40 B다(`osapi.c` L1544의 `OS_MAX_API_NAME * 2`) [F].

### 2.3 시작 경로 밖 (참고)

- 재시작·재적재 경로(`CFE_ES_AppCreate` 호출 `cfe_es_apps.c` L1098, L1134; 6.4.2 L1094, L1130)와 Start App 명령(`cfe_es_task.c` L880 / L888)은 같은 앱 코드로 같은 큐를 다시 만든다. cpu1 시작 경로가 아니므로 이 목록에 넣지 않았다. 앱을 지운 뒤에 다시 만드는 길이므로 동시에 쌓이는 큐 수는 늘지 않을 것으로 본다 [I, 정리 경로를 읽지 않음].

## 3. OSAL 4.1.1 posix `OS_QueueCreate` [F]

- 어느 구현이 빌드되는가: `osapi.c`에는 소켓 구현(L1110 `#ifdef OSAL_SOCKET_QUEUE`)과 POSIX 큐 구현(L1521 `#else` 아래)이 있다. `OSAL: build/inc/osconfig.h` L135의 `/* #define OSAL_SOCKET_QUEUE */`가 주석이므로 POSIX 큐 구현이 빌드된다. readme도 "the OSAL uses POSIX message queues"라고 적는다(6.4.1 L165–L167, 6.4.2 L167–L169).
- `OS_QueueCreate`(L1536–L1664):
  - 이름 검사: NULL(L1549), 길이 `>= OS_MAX_API_NAME`(L1557).
  - 표 칸: `OS_queue_table` 잠금 아래 빈 칸 검색. 한도는 `OS_MAX_QUEUES`(L1566–L1576)이고, 그 값은 `osconfig.h` L56의 **64**다.
  - **속성**: `queueAttr.mq_maxmsg = queue_depth`(L1598), `queueAttr.mq_msgsize = data_size`(L1599). `mq_flags`·`mq_curmsgs`는 채우지 않는다. mq_open(3)는 "Only the mq_maxmsg and mq_msgsize fields are employed when calling mq_open(); the values in the remaining fields are ignored"라고 적는다.
  - 이름: `"/" + getpid() + "." + queue_name`(L1601–L1620).
  - 호출: `mq_open(name, O_CREAT | O_RDWR, 0666, &queueAttr)`(L1625).
  - 실패: 표 칸을 돌려놓고(L1629–L1631), `errno == EINVAL`이면 msg_max 안내문을 찍는다(L1636–L1643). 반환은 `OS_ERROR`(L1644). 다른 깊이로 다시 시도하지 않는다.
- **깊이 조정 없음**: OSAL은 받은 깊이를 그대로 쓴다. cFE SB는 `Depth > CFE_SB_MAX_PIPE_DEPTH`(256, `cfe_platform_cfg.h` L206) 또는 0이면 `CFE_SB_BAD_ARGUMENT`로 거부한다(`cfe_sb_api.c` L192–L199). 줄이지는 않는다. 파이프 표가 차면(`CFE_SB_MAX_PIPES` 64, `cfe_platform_cfg.h` L139) `CFE_SB_MAX_PIPES_MET`이다(L204–L211). 큐 생성이 실패하면 `CFE_SB_PIPE_CR_ERR`다(L215–L221). 위 10개 앱 코드에도 실패 뒤 깊이를 바꿔 다시 부르는 곳은 없다. TO_LAB은 이벤트만 보낸다(L184, L190).
- **root와 일반 사용자**: OSAL 4.1.1에서 `geteuid()`는 세 곳뿐이다. `OS_API_Init`의 스케줄 설정(L435), `OS_TaskCreate`의 스케줄·스택·우선순위 속성(L587), `OS_TaskSetPriority`(L872). 큐 생성 경로에는 사용자 판별이 없다. 한도 검사의 차이는 커널에서 생기고, 기준은 uid가 아니라 `CAP_SYS_RESOURCE`다(§4.1). readme의 해법 2 "run the cFE core as root"(6.4.1 L171, 6.4.2 L174)는 root에게 그 권한이 있는 환경을 전제한다 [I].

## 4. 커널 규칙 [F][M]

원문: Linux stable 태그 `v6.18.44`의 `ipc/mqueue.c`, `ipc/mq_sysctl.c`, `include/linux/ipc_namespace.h`, `include/linux/msg.h`, `include/uapi/linux/mqueue.h`(git.kernel.org stable 저장소 `plain/…?h=v6.18.44`, 2026-10-29에 받음). man7.org의 mq_overview(7), getrlimit(2), mq_open(3)(Linux man-pages 6.19, 2026-02-08). 해시: `/home/user/work/c5/ref/{kernel,man}/sha256.txt`.

실행 커널은 `6.18.44-fc-v185`다 [M: `uname -r`]. 배포 쪽 패치가 아래 줄을 바꾸었는지는 볼 수 없다 [U]. 구조체 크기는 실행 커널의 BTF에서 읽었다 [M].

### 4.1 검사

| 무엇 | 위치 | 규칙 |
|---|---|---|
| 깊이·크기 | `mqueue.c` L348–L360 (`mqueue_get_inode`) | `mq_maxmsg <= 0` 또는 `mq_msgsize <= 0`이면 `EINVAL`. `CAP_SYS_RESOURCE`가 있으면 `HARD_MSGMAX`(65536)·`HARD_MSGSIZEMAX`(16 MiB) 이하. 없으면 그 IPC namespace의 `mq_msg_max`·`mq_msgsize_max` 이하. mq_overview(7) `/proc/sys/fs/mqueue/msg_max`: "The msg_max limit is ignored for privileged processes (CAP_SYS_RESOURCE), but the HARD_MSGMAX ceiling is nevertheless imposed." |
| 큐 수 | `mqueue.c` L582–L586 (`mqueue_create_attr`) | `mq_queues_count >= mq_queues_max`이고 `CAP_SYS_RESOURCE`가 없으면 `ENOSPC`. 세는 범위는 IPC namespace다 |
| 바이트 | `mqueue.c` L365–L387 | `mq_treesize = mq_maxmsg * sizeof(struct msg_msg) + min(mq_maxmsg, MQ_PRIO_MAX) * sizeof(struct posix_msg_tree_node)`, `mq_bytes = mq_maxmsg * mq_msgsize + mq_treesize`. 이 값을 만든 사용자의 ucounts(`current_ucounts()`, L372)에 더하고(L377), `rlimit(RLIMIT_MSGQUEUE)`(soft 값)를 넘으면 되돌리고 `EMFILE`(L378–L384). getrlimit(2) `RLIMIT_MSGQUEUE`가 같은 식을 "Since Linux 3.5"로 적는다 |
| 32비트 호출 | `mqueue.c` L1518–L1529 (`COMPAT_SYSCALL_DEFINE4(mq_open…)`) | 32비트 속성을 옮긴 뒤 같은 `do_mq_open`으로 간다 |
| 새 namespace의 값 | `mqueue.c` L1645–L1654 (`mq_init_ns`); `ipc_namespace.h` L118–L126 | `queues_max` 256, `msg_max` 10, `msgsize_max` 8192, `msg_default` 10, `msgsize_default` 8192 |
| sysctl 범위 | `mq_sysctl.c` L17–L21, L32–L38, L41–L47; L128–L131(namespace별 값으로 바꿔 끼움) | `msg_max` 1..65536, `msgsize_max` 128..16 MiB. 값은 namespace마다 따로다 |
| `MQ_PRIO_MAX` | `include/uapi/linux/mqueue.h` L24 | 32768 |

- 바이트 한도는 **IPC namespace가 아니라 사용자**(user namespace 안의 uid, ucounts) 단위로 센다(L372, L377) [F]. 그래서 IPC namespace를 따로 두어도, 같은 사용자의 다른 프로세스가 같은 시각에 가진 큐가 같은 819,200 B에서 빠진다 [I].

### 4.2 구조체 크기 [M: `/sys/kernel/btf/vmlinux`, 2026-10-29]

| 구조체 | 정의 | BTF 크기 |
|---|---|---:|
| `struct msg_msg` | `include/linux/msg.h` L9–L15 (`m_list`, `m_type`, `m_ts`, `next`, `security`) | 48 B (멤버 오프셋 0, 16, 24, 32, 40) |
| `struct posix_msg_tree_node` | `ipc/mqueue.c` L61–L65 (`rb_node`, `msg_list`, `priority`) | 48 B (오프셋 0, 24, 40) |

읽은 방법: BTF 형식을 읽는 짧은 스크립트(저장소 밖, 결과 `/home/user/work/c5/ref/kernel/btf_struct_sizes.txt`). 도구 코드이고 연구 조건이 아니다.

## 5. 계산 [I: §2·§4의 값으로 셈]

큐 하나의 바이트 = d × 48 + min(d, 32768) × 48 + d × 4 = **100 × d** (d ≤ 64이므로 min은 d).

| # | 큐 | 깊이 d | 바이트 |
|---:|---|---:|---:|
| 1 | ES_CMD_PIPE | 12 | 1,200 |
| 2 | EVS_CMD_PIPE | 32 | 3,200 |
| 3 | SB_CMD_PIPE | 32 | 3,200 |
| 4 | TIME_CMD_PIPE | 12 | 1,200 |
| 5 | TBL_CMD_PIPE | 12 | 1,200 |
| 6 | SAMPLE_CMD_PIPE | 32 | 3,200 |
| 7 | CI_LAB_CMD_PIPE | 32 | 3,200 |
| 8 | TO_LAB_CMD_PIPE | 8 | 800 |
| 9 | TO_LAB_TLM_PIPE | 64 | 6,400 |
| 10 | SCH_LAB_CMD_PIPE | 8 | 800 |
| | **합 (판마다 같음)** | **244** | **24,400** |

| 요구 | 값 | 한도 | 결과 |
|---|---:|---:|---|
| 큐 하나의 최대 깊이 | 64 | namespace `msg_max` (새 namespace 기본 10) | **올려야 함**: 64 |
| 메시지 크기 | 4 B | `msgsize_max` 8192 | 충분 |
| 큐 수 (실행 하나) | 10 | `queues_max` 256; OSAL `OS_MAX_QUEUES` 64; `CFE_SB_MAX_PIPES` 64 | 충분 |
| 사용자당 바이트 | 24,400 B | `RLIMIT_MSGQUEUE` 819,200 B | 충분 (3.0%, 남는 몫 794,800 B) |

- 바깥 namespace의 `msg_max` 10에서는 깊이 12·32·64인 큐 8개(1–7, 9)가 모두 `EINVAL`이 된다 [I]. 깊이 8인 두 개(8, 10)만 만들어질 수 있다. 2026-10-28 build-smoke가 코어 EVS·TIME의 실패로 끝난 것은 이와 맞는다(CB §3.2) [기록: 그 실행의 원 기록은 2026-10-29 컨테이너 교체로 없어졌다].

## 6. 이 컨테이너의 한도 [M, 2026-10-29]

| 항목 | 값 | 출처 |
|---|---|---|
| 커널 | `6.18.44-fc-v185`, x86_64, `CONFIG_POSIX_MQUEUE=y`, `CONFIG_IPC_NS=y`, `CONFIG_COMPAT=y`, `CONFIG_IA32_EMULATION=y` | `uname -a`, `/proc/config.gz` |
| `fs.mqueue` (현재 namespace) | `msg_max` 10, `msgsize_max` 8192, `queues_max` 256, `msg_default` 10, `msgsize_default` 8192 | `/proc/sys/fs/mqueue/*` |
| `RLIMIT_MSGQUEUE` | soft 819,200 B, hard 819,200 B | `/proc/self/limits` |
| 사용자와 권한 | uid 0. CapEff `000001fffeffffff`: 비트 24 `CAP_SYS_RESOURCE` **없음**, 비트 23 `CAP_SYS_NICE` 있음 | `id`, `/proc/self/status`; `env/CONDITIONS.md` D-4와 같은 값 |

- root에게 `CAP_SYS_RESOURCE`가 없으므로 hard 한도를 올릴 수 없다(PD29 A3). 이번 요구량은 한도 안이라 올릴 필요도 없다.

## 7. 유도한 실행 조건 — "구성에서 유도한 값" [D: D1 적용]

| 조건 | 값 | 유도 | 상태 |
|---|---|---|---|
| C5 실행의 IPC namespace 안 `fs.mqueue.msg_max` | **64** | §2.2의 모든 큐가 만들어지는 가장 작은 값 = 두 판 공통 최대 깊이(TO_LAB_TLM_PIPE 64, `to_lab_app.c` L160). 레퍼런스는 "increase the /proc/sys/fs/mqueue/msg_max parameter"만 말하고 값을 적지 않는다(6.4.1 L168–L171, 6.4.2 L171–L174). 64보다 큰 값은 근거 없이 고르는 값이 되므로 쓰지 않는다 | 구성에서 유도한 값. namespace 자체는 실행 구성 차이(`deviation`, D1) |
| namespace 안 `msgsize_max` | 바꾸지 않음 (기본 8192) | 메시지 크기 4 B ≤ 8192 (§2.2, §4.1) | 바꿀 근거 없음 |
| namespace 안 `queues_max` | 바꾸지 않음 (기본 256) | 큐 10개 ≤ 256 | 바꿀 근거 없음 |
| 바깥 `fs.mqueue.msg_max` | 10 유지 | D19, D1 | from-decision |
| `RLIMIT_MSGQUEUE` | 바꾸지 않음 (819,200 B) | 요구 24,400 B ≤ 한도. root가 올릴 수도 없음(§6) | 환경 값 그대로 |
| 깊이 | 소스 값 그대로 | A3: 깊이를 줄여 맞추지 않음. 이번에는 줄일 필요도 없음 | from-reference |

실행 때 남길 것 (D1: "상수·유도 과정·적용값·큐 생성 결과·바깥 값 유지를 기록하고, 실행 구성 차이로 표시한다"):

1. 상수와 유도: 이 문서의 §2.2와 §5(판, 파일, 줄).
2. 적용값: namespace 안에서 쓴 뒤 다시 읽은 `msg_max`·`msgsize_max`·`queues_max`, 그리고 바깥 namespace의 값. 바깥 값은 실행 앞뒤로 읽는다.
3. 큐 생성 결과: 그 namespace의 큐 목록(이름 `/<pid>.<pipe>`, 10개 기대)과 OSAL 안내문(`osapi.c` L1636–L1643)이 없음. 콘솔의 `CreatePipeErr`(`cfe_sb_api.c` L216–L219)도 없어야 한다.
4. 같은 사용자의 다른 큐: 바이트 한도는 사용자 단위라(§4.1), 같은 시각에 root로 도는 다른 cFS 실행이 있으면 그 사실을 적는다 [I].
5. 라벨: "원 소스 기반 현대 호스트 파생 구성, root·실시간 우선순위"(PD29 N-C5-1 적용). namespace 안의 `msg_max` 64를 실행 구성 차이로 적는다.

- 실행 사용자와의 관계 [I]: namespace 안에서 `msg_max`를 64로 두면 큐 때문에 root가 필요하지는 않다. root는 N-C5-1의 실시간 우선순위 때문에 쓴다(readme 6.4.1 L176–L177, 6.4.2 L179–L180). 큐 한도 검사는 이 컨테이너에서 root와 일반 사용자에게 같다(§3, §6).
- namespace에 쓰기가 되는지는 2026-10-28에 같은 값 10을 다시 써서 확인했다(CB §3.5 (c)) [기록: 원 기록은 없어졌다]. 이번 작업은 다시 시험하지 않았다. 부팅 준비 때 실제 값 64를 쓰고 다시 읽는 것으로 확인한다.

## 8. A3 판정

- 요구 바이트 24,400 B ≤ 819,200 B, 큐 수 10 ≤ 256, 메시지 크기 4 B ≤ 8192. **환경 차단 없음.**
- 바꿔야 하는 값은 namespace 안의 `msg_max` 하나뿐이고, 그 값은 64다.

## 9. 남은 것 [U]

1. 실제로 10개가 모두 만들어지는지는 부팅해야 보인다(N-C5-1 결정 뒤 단계 5).
2. 실행 커널이 v6.18.44 원 소스와 같은 검사를 하는지. 구조체 크기는 BTF로 확인했다. 검사 줄 자체는 배포 쪽 소스를 보지 못했다.
3. Processor Reset 경로는 큐를 `mq_unlink`하지 않는다(CB §9 항목 5). 실행마다 새 IPC namespace를 쓰면 namespace가 사라질 때 그 큐도 사라질 것으로 본다 [I: namespace 해제 경로를 읽지 않음]. 하네스는 실행 뒤 남은 큐를 기록한다.
4. 2.3의 재시작·재적재 경로의 큐 수(시작 경로 밖).

## 부록 A. 재현

```sh
W=/home/user/work/c5; mkdir -p $W/dl && cd $W/dl                 # 빈 디렉터리
curl -sS -L -o cFE-6.4.1-OSS-release.tar.gz "https://sourceforge.net/projects/coreflightexec/files/cFE-6.4.1/cFE-6.4.1-OSS-release.tar.gz/download"
curl -sS -L -o cFE-6.4.2-OSS-release.tar.gz "https://sourceforge.net/projects/coreflightexec/files/cFE-6.4.2-OSS-release.tar.gz/download"
curl -sS -L -o osal-4.1.1-release.tar.gz "https://sourceforge.net/projects/osal/files/OSAL-4.1.1/osal-4.1.1-release.tar.gz/download"
sha256sum *.tar.gz                                                 # §1 표와 대조
mkdir -p $W/src/641 $W/src/642 $W/src/osal
tar -xzf cFE-6.4.1-OSS-release.tar.gz -C $W/src/641; tar -xzf cFE-6.4.2-OSS-release.tar.gz -C $W/src/642
tar -xzf osal-4.1.1-release.tar.gz -C $W/src/osal
# 큐 생성 지점 (§2.2)
for V in 641 642; do C=$W/src/$V/cFE-6.4.${V:2}-OSS-release
  grep -rn "CFE_SB_CreatePipe\|OS_QueueCreate" $C/cfe/fsw $C/apps $C/psp --include=*.c | grep -v "unit-test\|ut_\|\.back"; done
grep -n "THE_APPS" $W/src/641/cFE-6.4.1-OSS-release/build/cpu1/Makefile
# OSAL (§3)
sed -n '1536,1664p' $W/src/osal/osal-4.1.1-release/src/os/posix/osapi.c
grep -n "OS_MAX_QUEUES\|OSAL_SOCKET_QUEUE" $W/src/osal/osal-4.1.1-release/build/inc/osconfig.h
# 커널 원문 (§4)
for f in ipc/mqueue.c ipc/mq_sysctl.c include/linux/ipc_namespace.h include/linux/msg.h include/uapi/linux/mqueue.h; do
  curl -sS -L -o $(echo $f | tr / _) "https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/plain/$f?h=v6.18.44"; done
# 한도 (§6)
cat /proc/sys/fs/mqueue/{msg_max,msgsize_max,queues_max}; grep -i msgqueue /proc/self/limits; grep CapEff /proc/self/status
```

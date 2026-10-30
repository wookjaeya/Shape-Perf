# C5 실행 기반 — cFE 6.4.1/6.4.2 빌드, build-smoke, 적용 설계 (2026-10-28)

- 성격: **빌드 기록, build-smoke 기록, 설계**다. 본 실험 결과가 아니다. 기동은 판마다 한 번만 했고(§3), Kairos 관측기는 붙이지 않았다. 실패 창이 있다거나 재현했다고 주장하지 않는다.
- 입력: `cases/C5.md`(§3, §7, §10, §11 A6–A8, §12 Q6), `cases/ledger.md` X7, `plan/case_setup_C2_C3_C5.md`(이하 CS; §3, §4.2, §4.3, §7 항목 4·8, 부록 A), `Architecture/spec/PI_DECISIONS_RESOLVED_20261015.md` §3.4(D13), `Architecture/spec/PI_DECISIONS_RESOLVED_20261028.md`(이하 PD28), `STATUS_20261027.md`, `kairos/docs/event_semantics.md`(이하 ES) §2–§3, §7–§8, §12, `kairos/docs/instr_candidate_gcc.md`, `plan/results_instr_pilot_20261026.md` §2.4, `plan/C1_build_manifest.md`(형식).
- 규칙: `CONDITIONS_POLICY.md`. 값마다 레퍼런스와 상태(`from-reference`·`deviation`·`unspecified_by_reference`)를 단다. 레퍼런스가 정하지 않는 값은 고르지 않았다. 형상 연결은 `CONFIG_MANAGEMENT.md` §1을 따른다.
- 표기: [F] 고정 출처(해시를 적은 tarball, 고정 태그)에서 읽음, [M] 이번 작업에서 도구로 얻음, [I] 추론, [D] 결정, [U] 미확인. 줄 번호는 해당 tarball 파일의 줄이다. `SF-6.4.1:` 표기는 `cases/C5.md` 머리말과 같다.
- 작업 공간(저장소 밖): `/home/user/work/c5/`. 빌드 트리는 기록을 남긴 뒤 지웠다. 남긴 것은 tarball, 실행 디렉터리 사본, 기록, 스크립트이며 해시는 `/home/user/work/c5/ARTIFACTS.sha256`에 있다(약 77 MB).
- **2026-10-29 갱신** (부팅·빌드 없음):
  - 원 기록 상실: 2026-10-29 컨테이너 교체로 위 작업 공간의 10-28 자료가 모두 없어졌다. 빌드 로그, 실행 디렉터리 사본, smoke 기록, binding 표, 32비트 런타임 `.a`, glibc 원 소스, `ARTIFACTS.sha256`이 여기에 든다(`REVIEW_ANALYSIS_20261029.md` §0). 그래서 §2–§3과 §8.2–§8.3의 결과는 **[기록]**이다. 다시 확인한 것으로 쓰지 않는다.
  - 다시 확인한 것: 세 tarball을 같은 URL에서 다시 받았고 sha256이 같다(`plan/c5_queue_inventory_20261029.md`(이하 QI) §1). §7.1의 OSAL·glibc 줄은 다시 받은 원문에서 같은 줄임을 확인했다 [F].
  - 새 작업 공간: `/home/user/work/c5/{dl,src,ref}`(QI 머리말).
  - PD29(`Architecture/spec/PI_DECISIONS_RESOLVED_20261029.md`) D1–D5·A3·A4·N-C5-1을 날짜 메모로 반영했다. 위치는 §0, §3.5, §5.2, §6, §7.1, §7.2, §8.3, §9, §10.2, §10.3, §11이다. 새 절은 §10.4(E-ID1 초안 명세)와 §10.5(E-ST 요구 목록)다. 둘 다 구현하지 않은 초안이다.
- **2026-10-29 C5 도구 갱신** (빌드 있음, 부팅 없음; 작업 공간 `/home/user/work/c5tools/`, 저장소 밖):
  - 근거 결정: PD29 D3·D5와 §3a D7.
  - 위치: §0 메모, §8.3 메모 2(E-RT), 새 §8.4(6.4.x 계측 파생 빌드), §10.4 메모(E-ID1 구현), §10.5 메모(E-ST 구현·실행, D7), §11 메모, 부록 A 끝(재현).

## 0. 요약

1. **판과 출처** [F][M]. 수정 전은 cFE **6.4.1**, 수정 후는 **6.4.2**, OSAL은 SourceForge `osal-4.1.1-release`, PSP는 tarball에 든 pc-linux 1.2.0.0이다(`cases/C5.md` §3, §10 행 1·2·4·19). 세 tarball을 기록된 URL에서 다시 받았고 sha256이 기록과 같다(§1.2). 예전 압축 해제본(`/home/user/work/procrace/src/C5*`)은 이미 지워져 없었다.
2. **빌드** [M]. 두 판 모두 readme 빠른 시작대로 `make config`·`make`가 끝났다. 소스·설정·컴파일러 옵션은 바꾸지 않았다. 이탈은 둘이다: 빈 자리 디렉터리 `osal/` 처리(DV-1)와 빌드 host·도구(DV-H). readme를 글자 그대로 따르면 OSAL이 `osal/osal-4.1.1-release`에 놓여 빌드가 멈춘다(§2.2).
3. **build-smoke** [M]. 두 판 모두 1초 안에 스스로 끝났다. EVS의 SB 파이프(깊이 32)가 `msg_max` 10을 넘어 `mq_open`이 실패했다. 코어 앱 초기화 실패로 ES가 Processor Reset을 요청했고 PSP가 종료 코드 1로 나갔다. ES는 운용 상태에 이르지 못했다. 시작 스크립트의 앱은 하나도 만들어지지 않았다. 그래서 **사례의 `CFE_ES_AppCreate` 경로는 실행되지 않았다**. 시작 리셋은 두 판 모두 `POWER ON`이었다(§3).
4. **실행을 막는 조건** [M][I]. readme의 두 해법(L171–L177) 가운데 "root로 실행"은 이 컨테이너에서 성립하지 않는다. root에게 `CAP_SYS_RESOURCE`가 없기 때문이다. 남는 해법은 `msg_max` 상향인데, D19가 현 기준을 10으로 묶었다. 따라서 6.4.x를 운용 상태까지 돌리려면 PI 결정이 필요하다(CS §7 항목 4). 새 선택지로, 별도 IPC namespace 안에서는 이 값을 쓸 수 있음을 확인했다. 같은 값 10을 다시 써서 시험했고 전역 값은 바뀌지 않았다(§3.5).
5. **판정 단위** [D]. PD28 §2 항목 6에 따라 순서위반을 단위로 한다. 위반은 새 task가 자기 `TaskTable` 항목을 잠금 아래에서 읽은 것이 생성자의 등록보다 먼저인 경우다. `CFE_ES_ERR_APPID`, 잘못된 AppID의 `CFE_SUCCESS`, 그 뒤의 소비 결과는 따로 보고한다(§4).
6. **A8** [F][I]. 소스로 정해지는 것이 있다. OSAL 4.1.1은 `&OS_task_table[i].id`를 `pthread_create`의 출력 인자로 넘긴다(`osapi.c` L645). glibc 2.12·2.17·2.39 원 소스는 `*newthread`를 스레드 생성 전에 쓴다. 그러면 새 task의 `OS_TaskRegister` 검색(L921–L927)은 자기 id를 찾는다. 그래서 새 항목에 대해서는 창 안의 결과가 `CFE_ES_ERR_APPID`이다. 잘못된 AppID 결과에는 낡은 항목 같은 다른 조건이 필요하다. 이 컨테이너의 libc 바이너리, 창에 실제로 들어가는지, 낡은 항목 경로는 실행으로만 확인된다(§7).
7. **계측 경로** [M]. `kzgcc` 0.2는 `-m32`로 이 옛 소스를 계측한다. 초안 manifest(`kairos/profiles/c5-cfe-6.4.x/instr/`)의 site는 6.4.1이 32/32, 6.4.2가 30/30 모두 `bound`였다. 경고 수는 기준 컴파일과 같았다. 런타임 C 라이브러리도 `-m32`로 빌드된다. 32비트 회수 도구(`kz-rt-dump`)는 32비트 C++ 라이브러리가 없어 링크되지 않는다(§8).
8. **F0 경로** [F]. 지금 F0 관측기는 ELF64 x86-64 파일만 받는다(`kairos/probe/anchor/elf_image.cpp` L57–L58). 6.4.x 기준 빌드는 readme대로 32비트(`-m32`)다. 그래서 F0 비교 경로에는 엔진 작업이 필요하다(§10의 E-F0).
9. **새 판정 알고리즘은 필요 없을 것으로 본다** [I]. 판정 구조는 C1의 "생성 전 조회"(반환 코드 좁힘)와 같다. 키가 이름 대신 OSAL task id일 뿐이다. 필요한 엔진 작업은 관측 backend(ELF32), 정체성 결합(생성 뒤에 정해지는 소유 앱), 회수 도구다. 조건식 키 하나는 확인이 남았다(§10).

**2026-10-29 메모 (PD29)** [D][F]:
- 결정: §11의 PI 결정 1–5가 D1–D5로 닫혔다. 실행 사용자·스케줄은 N-C5-1 (가) root·실시간 우선순위로 정해졌다.
- 큐 조건(D1·A3): §3.5 (c) IPC namespace를 쓴다. 값은 구성에서 유도한다. 유도 결과는 `msg_max` **64**다(32가 아님). 최대 깊이는 TO_LAB 텔레메트리 파이프다. 큐는 판마다 10개, 메시지 크기는 4 B, 바이트 합은 24,400 B로 819,200 B 한도 안이라 환경 차단이 없다(QI §0).
- 6 (A8): 위 요약은 다시 받은 원문에서도 같다(§7.1 메모). `cases/C5.md` A8을 고쳤다.
- 7·8 (경로, D3): 첫 검증은 계측 경로다. F0의 32비트 관측(E-F0)은 보류한다. 계측 경로에도 정적 분석의 ELF32(E-ST)가 선행 조건이다(§10.2, §10.5).
- 9: 조건식 키는 이미 지원된다(§10.3 메모). 남은 일은 실제 site 값을 묶어 재생하는 것이다.
- **C5 도구 (2026-10-29)** [M]:
  - 7: 32비트 런타임과 같은 판·설정의 32비트 `kz-rt-dump`를 빌드했다. 저장소는 같은 빌드의 회수 도구만 받는다(§8.3 메모 2). 두 판의 계측 파생 빌드에서 초안 site가 6.4.1 33/33, 6.4.2 31/31 모두 `bound`이고 링크 뒤 hook 검사가 모두 ok다(§8.4). site가 하나씩 는 것은 E-ID1의 owner-publish site 33이다.
  - 9: E-ID1과 E-ST를 구현했다(§10.4 메모, §10.5 메모). 두 판의 기준·파생 빌드에서 정적 basis가 만들어진다. D7로 낮춘 두 단위에서 `pthread_create`·`signal` 호출이 사실로 나온다.

## 1. 판, 출처, 릴리스의 빌드 지시 (목표 1)

### 1.1 판

| 항목 | 값 | 레퍼런스 | 상태 |
|---|---|---|---|
| 수정 전 | cFE 6.4.1 (6.4.0과 `cfe_es_apps.c`가 같음) | `cases/C5.md` §3, §10 행 1; 6.4.2 VDD §1.1 "found in cFE build 6.4.1"(F13) | from-reference |
| 수정 후 | cFE 6.4.2 | `cases/C5.md` §10 행 2; VDD Table 1.2-2 item 2 (Trac #41) | from-reference |
| OSAL | SourceForge `osal-4.1.1-release.tar.gz` | 6.4.1·6.4.2 readme L76 "4.1.1 or higher", L141(6.4.2 L143)의 파일 이름; VDD Table 1.4-1 | from-reference |
| PSP | pc-linux 1.2.0.0 (tarball 안) | `cases/C5.md` §10 행 19 | from-reference |
| cpu1 구성의 OSAL·PSP 선택 | `OS = posix`, `PSP = pc-linux` | `SF-6.4.1:build/cpu1/cfe/cfe-config.mak` L27, L32 (`cases/C5.md` §10 행 17) | from-reference |
| 32비트 | `-m32`, `ld -melf_i386` | `compiler-opts.mak` L33, L89; `link-rules.mak` L21, L42 [F: 이번에 다시 읽음] | from-reference |

### 1.2 내려받기와 해시 [M, 2026-10-28T01:28Z]

URL은 CS 부록 A.1의 것이다. 모두 HTTP 200이었고 `master.dl.sourceforge.net`으로 리다이렉트되었다. 기록: `/home/user/work/c5/dl/{fetch_log.txt,sha256.txt}`.

| 파일 | 바이트 | sha256 | 기록(CS §3.1, `cases/C5.md` §15)과 |
|---|---:|---|---|
| `cFE-6.4.1-OSS-release.tar.gz` | 39,586,940 | `ec26e33b4ab40b9616ccdbc984c5a00a790a7d05b260f2628c3335a29c23e98f` | 같음 |
| `cFE-6.4.2-OSS-release.tar.gz` | 36,411,531 | `c64ed2aac910e79a641f49279dfdff3b7b49374e54ed56b595d47e16a3f53a4f` | 같음 |
| `osal-4.1.1-release.tar.gz` | 1,973,805 | `8f0f2b23d8226256fdb8543bf9fcb560b38434411dc12cb4f7f35b9c19e61d46` | 같음 |

압축을 푼 주요 파일의 sha256도 기록과 같다: 6.4.1 `cfe_es_apps.c` `0a54974ba77af3db…`, `cfe_es_api.c` `0cb4c1c740f897a3…`, 6.4.2 `cfe_es_apps.c` `2a1e2f4ce32d05a9…`, `cfe_es_api.c` `8a67d0502452c0c6…`, OSAL `osapi.c` `32649c6f1cf3b586…`(CS 부록 A.4) [M: `logs/source_hashes.txt`]. 두 판에서 `setvars.sh`, `build/cfs.mak`, `build/cpu1/Makefile`, pc-linux의 `compiler-opts.mak`·`link-rules.mak`·`cfe_psp_start.c`·`cfe_psp_memory.c`·`cfe_psp_support.c`, 시작 스크립트가 바이트로 같다 [M].

### 1.3 릴리스가 주는 빌드·실행 지시 [F]

| 단계 | 6.4.1 `cfe-OSS-readme.txt` | 6.4.2 `cfe-OSS-readme.txt` |
|---|---|---|
| OSAL 풀기·옮기기 | L141 `tar -zxf ../Projects/osal-4.1.1-release.tar.gz`, L142 `mv osal-4.1.1-release osal` | L143, L144 (같은 명령) |
| 환경 | L150 `. ./setvars.sh` (`setvars.sh` L98 `OSAL_DIR=$CFS_MISSION/osal`) | L152 |
| 빌드 | L151–L156 `cd build`, `cd cpu1`("cpu1 is configured for the pc-linux target"), `make config`, `make` | L153–L158 |
| 실행 | L162–L163 `cd exe`, `./core-linux.bin` | L164–L165 |
| 큐 깊이 | L168–L174: `msg_max`를 올리거나 root로 실행 | L171–L177 |
| RT 스케줄링 | L176–L177 "Root/sudo is also required to enable the linux real time scheduler" | L179–L180 |
| 정지 | L182 "hitting control-c in the terminal" | L193 |
| 다음 시작 | — | L195–L196 "After quitting the cFE with a Control-C signal, it MUST be started next time with a Poweron Reset ( --reset PO )" |
| 빌드 host | L82–L91 (CentOS 6.5-x86, CentOS-7.0-x64, Ubuntu 12.04 LTS x86, Ubuntu 14.04 LTS x64) | 같은 문구, L129에 "Ubuntu 13.10 x64 and CentOS 6.5 i386" 추가 |

- 6.4.2 readme L136은 `tar -zxf cFE-6.4.1-OSS-release.tar.gz`라고 적는다. 6.4.1 문구를 그대로 둔 것으로 보인다 [I]. 이 작업은 각 판의 tarball을 풀었다.
- 두 tarball 모두 `osal/`에 `readme.txt` 하나만 있다(sha256 `6c1e9d10…`, 내용 "Obtain the OS Abstraction layer (osal) at …") [M].
- 6.4.2 tarball에는 `cfe/tools`가 없다(tar 목록에 `elf2cfetbl` 0건) [M]. 6.4.1에는 있다.
- cpu1 구성은 표를 빌드하지 않는다: `THE_TBLS :=` 빈 값(`build/cpu1/Makefile` L49) [F]. 앱은 `THE_APPS := sample_lib sample_app`, `+= ci_lab to_lab sch_lab`(L42–L43).

## 2. 빌드 (목표 2)

### 2.1 절차

스크립트 `/home/user/work/c5/scripts/build_64x.sh VER`. 판마다 빈 디렉터리 `b64<VER>`에서 다음을 한다.

```sh
tar -xzf dl/cFE-6.4.$V-OSS-release.tar.gz && cd cFE-6.4.$V-OSS-release
rm -r osal                                    # DV-1 (§2.3)
tar -zxf <dl>/osal-4.1.1-release.tar.gz       # readme L141 / L143 (경로만 절대 경로)
mv osal-4.1.1-release osal                    # readme L142 / L144
. ./setvars.sh                                # L150 / L152
cd build/cpu1 && make config && make          # L151-L156 / L153-L158  (병렬 없음; readme가 -j를 쓰지 않음)
```

실행 사용자는 root다. `make`에 변수나 플래그를 주지 않았다. 기록: `/home/user/work/c5/logs/b64{1,2}.{config,make}.log`, `.rc`.

### 2.2 첫 실패 지점 — readme 글자 그대로 (먼저 기록) [M]

6.4.1에서 readme 명령을 그대로 썼다. `mv osal-4.1.1-release osal`은 이미 있는 디렉터리 `osal/` **안으로** 옮겼다. 결과는 `osal/osal-4.1.1-release`다(`cases/C5.md` §10 행 6의 추론이 맞았다).

- `make config`: `cfs.mak:45`의 `cp -f $(OSAL)/build/inc/osconfig.h ./inc`가 "cannot stat …/osal/build/inc/osconfig.h"로 실패했다. 이 줄은 `-` 접두라 무시되고 계속된다. 이어 `elf2cfetbl` 빌드가 `cfe_tbl.h:61: fatal error: common_types.h`로 멈췄다(`cfs.mak:402`, rc 2).
- `make`: `Makefile:38: …/osal/src/os/posix/osal.mak: No such file or directory`, rc 2.
- 기록: `/home/user/work/c5/logs/literal641_{config,make}.log`.

### 2.3 이탈 대장

| ID | 이탈 | 레퍼런스 | 사용 값 | 이유 | 영향 |
|---|---|---|---|---|---|
| DV-1 | `mv` 전에 빈 자리 `osal/`(`readme.txt` 하나)를 지움 | readme L142(6.4.2 L144) `mv osal-4.1.1-release osal`; `setvars.sh` L98 `OSAL_DIR=$CFS_MISSION/osal` | 위 절차 | §2.2: 글자 그대로면 `OSAL_DIR`이 OSAL 뿌리가 아니다. 지우면 `mv`가 `setvars.sh`가 이름한 자리를 만든다. 다른 길(`OSAL_DIR`을 `osal/osal-4.1.1-release`로 바꿈)은 `setvars.sh`의 값을 바꾸므로 고르지 않았다. 레퍼런스는 이 충돌의 처리를 적지 않는다(`unspecified_by_reference`, `cases/C5.md` §10 행 6) | 지운 파일은 안내문뿐이다(내용 §1.3). 소스·설정·빌드 명령은 같다 |
| DV-H | 빌드 host와 도구 | readme L82–L91의 host 목록(D3a, `cases/C5.md` D3) | Ubuntu 24.04.4 LTS x86_64, 커널 6.18.44-fc-v180, gcc 13.3.0 (Ubuntu 13.3.0-6ubuntu2~24.04.1), GNU ld 2.42, make 4.3, glibc 2.39-0ubuntu8.9, `libc6-dev-i386` 2.39, `gcc-multilib` 4:13.2.0-7ubuntu1, `lib32gcc-13-dev` 13.3.0 | 이 컨테이너. 목록 host의 재구성은 시험하지 않았다(CS §3.3.1). 32비트 헤더는 CS §3.3의 분석 전용 파싱과 같은 것(`libc6-dev-i386`, `gcc-multilib`)이다 | 아래 "컴파일러 기본값". D13(`PI_DECISIONS_RESOLVED_20261015.md` §3.4)이 말한 "실제 실행 검증"의 별도 산출물이며 원 플랫폼 재현이 아니다. 이 이탈의 승인은 `cases/C5.md` D3a의 PI 몫이다. 이번 빌드는 build-smoke 라벨로만 쓴다 |

- **바꾸지 않은 것** [M]: 소스, 시작 스크립트, `-m32`, 경고 옵션(`-Wall -Wstrict-prototypes`, `compiler-opts.mak` L13), `-g -O0`, 정의(`-D_EL … -D_EMBED_`), 링크 명령. `-std=`, `-Wno-error`, `-fcommon`은 주지 않았다. 레퍼런스 make 트리에 `-Werror`가 없어 경고가 빌드를 막지 않았다(CS §3.3 grep).
- **컴파일러 기본값** [M]: 이 gcc는 옵션을 주지 않아도 다음을 켠다. `-fPIE`·PIE 링크(`--enable-default-pie`), `-fstack-protector-strong`(`__SSP_STRONG__ 3`), `-fstack-clash-protection`, `-fcf-protection=full`(`--enable-cet`), `-fno-common`. `-O0`이라 `_FORTIFY_SOURCE`는 정의되지 않는다. 목록 host의 gcc(CentOS 6.5 GA는 4.4.7, CS §3.3)는 이 값들이 다를 것이다 [I]. 이번 빌드는 이 값을 고르지도 바꾸지도 않았다. 소스 의미나 타이밍에 주는 영향은 확인하지 않았다 [U].
- **없는 도구** [M]: 레퍼런스 빌드에 필요한 도구는 모두 있었다. 6.4.2의 `make config`는 마지막 단계에서 rc 2로 끝났다. `cfs_make_tool_build_dirs`(`cfs.mak:419`)가 tarball에 없는 `cfe/tools/elf2cfetbl/for_build/Makefile`을 복사하려 했기 때문이다. `config::`의 순서는 `cfs_envs cfs_prebuild cfs_tool`(`build/cpu1/Makefile` L108)이므로 이 실패는 복사 단계 뒤다. 생성된 `build/cpu1/inc`·`build/mission_inc`의 파일 목록은 6.4.1과 같았다 [M]. 표를 빌드하지 않으므로(`THE_TBLS` 빈 값) `elf2cfetbl`이 없어도 결과는 같다. 이탈로 세지 않는다.
- **무시되는 오류** [M]: 두 판 모두 `make`의 `cfs.mak:252`(`chmod 644 *.tbl`)와 `:257`(`cp *.tbl`)이 "No such file"로 실패하고 무시된다. 레퍼런스 줄의 `-` 접두가 무시를 정한다. `.tbl`이 없는 것은 위 `THE_TBLS`와 맞다.

### 2.4 경고와 산출물 [M]

- 경고는 두 판 모두 32건이다. 문구 집합도 같다. 예: `osapi.c:645` "passing argument 3 of 'pthread_create' from incompatible pointer type"(clang 18에서는 기본 오류, CS §3.3 ⑤), `-Wsizeof-pointer-memaccess` 3, `-Waddress` 3, 앱 `.so` 둘(`sample_app.so`, `ci_lab.so`)의 "creating DT_TEXTREL in a shared object".
- 실행 파일과 앱(sha256, 이번 빌드):

| 판 | `core-linux.bin` | `sample_app.so` | `sample_lib.so` | `ci_lab.so` | `to_lab.so` | `sch_lab.so` |
|---|---|---|---|---|---|---|
| 6.4.1 | `09697cece137c800…` (646,244 B) | `2f563e3988b0fbe0…` | `936d49a4aac78d1b…` | `a20cbd2d3e3147f3…` | `906116ccdb72842a…` | `fa2db1c7a7df40ab…` |
| 6.4.2 | `4a57e8017b85fe81…` (646,332 B) | `cb29af7ad8eb1273…` | `d01843c5bb46c754…` | `964678abc98aad1f…` | `b1d5d3b11a6725b6…` | `da4648dddca5748e…` |

- 앱 소스는 두 판에서 같은데 `.so` 해시가 다르다. 빌드 경로(`b641`/`b642`)가 `-g` 디버그 정보에 들어가는 것이 한 원인일 것이다 [I, 확인 안 함].
- 실행 디렉터리 사본: `/home/user/work/c5/exe64{1,2}/`(각 900 KB). 빌드 트리는 지웠다. 다시 만들려면 §2.1을 다시 돌린다.

## 3. build-smoke (라벨: build-smoke, 결과 아님)

### 3.1 절차

스크립트 `/home/user/work/c5/scripts/smoke_64x.sh VER OUT`. 기록: `/home/user/work/c5/smoke/64{1,2}/`.

- 먼저 다른 cFS 프로세스를 찾았다(`pgrep -x 'core-cpu1|core-linux.bin|core-tx|core-rx'`). 없었다 [M].
- 시작: `cd exe`, `./core-linux.bin`, 인자 없음(readme L162–L163 / L164–L165). root로 실행했다(readme 해법 2, L171–L174; 6.4.2 L174–L177). 터미널은 util-linux `script`의 pty로 주었다(readme의 "terminal"). `msg_max` 등 sysctl, CPU·affinity·우선순위는 아무것도 바꾸지 않았다.
- 정지: SIGINT(readme의 control-c). 이번에는 보낼 대상이 이미 끝나 있었다(§3.2).
- 도구 값(30 s 대기 한도, 5 s 정착, 15 s 정지 한도)은 smoke 도구 값이고 연구 조건이 아니다.
- 실행 환경 [M]: 커널 `6.18.44-fc-v180`, uid 0, 4 CPU, `fs.mqueue.msg_max` 10, `msgsize_max` 8192, `queues_max` 256, `RLIMIT_MSGQUEUE` 819200 B, `CapEff 000001fffeffffff`(비트 24 `CAP_SYS_RESOURCE` 없음).

### 3.2 결과 [M]

| 항목 | 6.4.1 (`smoke/641/console.log`) | 6.4.2 (`smoke/642/console.log`) |
|---|---|---|
| 리셋 유형 | "CFE_PSP: Default Reset Type = PO", "Starting the cFE with a POWER ON reset.", "POWER ON RESET due to Power Cycle (Power Cycle)." | 같음 |
| 공유 메모리 | "Clearing out CFE CDS / Reset / User Reserved Shared memory segment." | 같음 |
| 코어 앱 | EVS·SB·ES·TIME·TBL 생성(App ID 0–4) | EVS 생성(App ID 0) 뒤 멈춤 |
| 큐 | OSAL 안내문 "Your queue depth may be too large …" 2회. "CreatePipeErr:OS_QueueCreate returned -1,app CFE_EVS", 같은 줄이 CFE_TIME | 1회, CFE_EVS |
| 코어 앱 실패 | "EVS:Application Init Failed,RC=0xCA000005" → "CFE_ES_ExitApp: CORE Application CFE_EVS Had an Init Error." → "PROCESSOR RESET called from CFE_ES_ResetCFE (Commanded)."; TIME도 같은 순서 | EVS만 |
| 시작 스크립트 | "Opened ES App Startup file: /cf/apps/cfe_es_startup.scr" 뒤 앱 적재 줄 없음 | 파일을 열기 전에 끝남 |
| ES 상태 | 운용 상태 표시("CFE Core Startup Complete") 없음 | "entering OPERATIONAL state" 없음. 마지막 상태 표시는 "CORE_STARTUP" |
| 끝 | "CFE_PSP: Exiting cFE with PROCESSOR Reset status. … Shared Memory segments have been PRESERVED." 종료 코드 1, 시작 뒤 1 s 안 | 같음 |

- 읽기 [F]: EVS 파이프 깊이는 32다(`cfe_evs_task.h` L100; SB 명령 파이프 32 `cfe_sb_priv.h` L136, TBL·TIME 12). OSAL 4.1.1 `OS_QueueCreate`는 `mq_maxmsg = queue_depth`로 `mq_open`한다(`osapi.c` L1598, L1625). 실패하면 L1636–L1642의 안내문을 찍고 `OS_ERROR`다. 깊이를 줄이는 대체 경로는 없다. 코어 앱이 `CFE_ES_CORE_APP_INIT_ERROR`로 끝나면 ES가 `CFE_ES_ResetCFE(CFE_ES_PROCESSOR_RESET)`를 부른다(`SF-6.4.1:…/cfe_es_api.c` L451–L470, 메시지 L234). PSP의 `CFE_PSP_Restart`는 공유 메모리를 남기고 `exit(CFE_ES_PROCESSOR_RESET)`한다(`psp/fsw/pc-linux/src/cfe_psp_support.c` L68–L85, 두 판 같음).
- 따라서 이 실행은 **#72의 경로(`CFE_ES_AppCreate`)에 이르지 못했다.** 시작 스크립트의 다섯 `CFE_APP` 항목(`cfe_es_startup.scr` L1–L5)은 하나도 `CFE_ES_AppCreate`에 가지 않았다. 스레드 표는 프로세스가 이미 없어 얻지 못했다.
- 이 smoke는 #72에 대해 아무것도 보이지 않는다. 판정 어휘로 옮기지 않는다.

### 3.3 시작 상태 [M] (`state_{before,running,after}.txt`)

| 시점 | 실행 디렉터리 | SysV shm | `/dev/shm` |
|---|---|---|---|
| 앞 | 빌드 산출물만(`core-linux.bin`, 앱 `.so` 5, `cfe_es_startup.scr`, `cf/apps/`의 복사본, 빈 `ram/`). 키 파일 없음 | 없음 | `rclone-vfscache` 디렉터리 하나(이 작업과 무관한 기존 항목) |
| 뒤 | 빈 키 파일 3개 추가: `.cdskeyfile`, `.resetkeyfile`, `.reservedkeyfile`(`cfe_psp_memory.c` L58–L60, 생성 L656–L660) | 3개 남음: 131,072 B(CDS), 174,080 B(Reset Area), 1,048,576 B(User Reserved). 키는 `ftok(<키 파일>, 'R')`(L122, L335, L465)라 실행 디렉터리마다 다르다(6.4.1 `0x5200e831`–`33`, 6.4.2 `0x52004817`–`19`) | 같음 |

- 정리 [M]: 내 실행이 만든 segment 6개(생성 pid 8537, 9318)를 `ipcrm`으로 지웠고, 키 파일 6개도 지웠다(`logs/shm_residue_before_cleanup.txt`, `logs/shm_cleanup.txt`). POSIX 큐는 만들어진 것이 없었다. 이 세션의 IPC namespace를 별도 mount namespace에서 mqueue로 마운트해 본 목록이 비어 있었다. 깊이 10 이하 큐가 없어 모든 `mq_open`이 실패했기 때문이다 [I].
- EEPROM: pc-linux PSP의 EEPROM 파일 경로는 `CFE_PSP_EEPROM_SUPPORT`가 정의될 때만 컴파일된다(`cfe_psp_start.c` L315). 이 빌드의 정의 목록에는 없다 [M: 컴파일 줄]. 그래서 6.4.x에는 EEPROM 초기 이미지가 없다. R2의 공통 시작 조건(PD28 Q3)에서 6.4.x에 해당하는 것은 SysV 세 segment와 키 파일이다 [I].

### 3.4 다음 기동 전에 지킬 것 [F]

- Processor Reset으로 끝난 뒤에는 PSP가 "Restart the cFE with the PR parameter"라고 찍는다. 그러나 공통 시작 상태는 Power-On이다. 그래서 하네스가 남은 segment와 키 파일을 기록하고 지운 뒤 `--reset PO`로 시작해야 한다. SIGINT로 끝난 뒤에는 readme(6.4.2 L195–L196)와 PSP(`cfe_psp_start.c` L489–L490)가 `--reset PO`를 요구한다. 첫 기동은 인자 없이도 PO다(L555–L559).

### 3.5 실행 조건 — PI 결정 필요 (CS §7 항목 4의 갱신)

readme의 두 해법(6.4.2 L171–L177)을 이 컨테이너에 대어 본 결과는 다음과 같다.

| 선택지 | 레퍼런스 | 이 컨테이너 [M] | 비용·영향 |
|---|---|---|---|
| (a) 전역 `msg_max` 상향 | readme 해법 1 (값은 적지 않음) | 쓰기 가능 | 같은 커널을 쓰는 ENV/R2 실행과 다른 작업이 함께 바뀐다. D19가 현 기준을 10으로 고정했다. 값은 레퍼런스에 없다 |
| (b) root 실행 | readme 해법 2 | **성립 안 함**: `CAP_SYS_RESOURCE`가 bounding set에 없어 root도 `msg_max`를 넘지 못함(§3.2) | 권한을 주려면 컨테이너 구성 변경이 필요하다 [I] |
| (c) 6.4.x 실행만 별도 IPC namespace(`unshare --ipc`)에 두고 그 안에서 `msg_max`를 올림 | readme 해법 1의 범위를 한 실행으로 좁힘. namespace 자체는 레퍼런스에 없음 | `unshare --ipc` 안에서 `/proc/sys/fs/mqueue/msg_max`에 쓰기 성공. 시험은 같은 값 10을 다시 쓴 것이고, 바깥 값은 10 그대로였다 | 전역 기준을 바꾸지 않는다. SysV shm도 namespace마다 따로라 실행마다 깨끗한 공유 메모리 초기 조건이 된다. 그러나 시작 환경 차이(`deviation`)이고, 값 선택이 남는다 |
| (d) 실패를 기록 | D19 "손실을 기록" | 이번 smoke가 그 기록 | 코어 EVS가 죽어 사례 경로에 이르지 못한다 |

- 값을 고르는 근거 후보 [I]: 레퍼런스는 "increase"만 말한다. 구성 안에서 정해지는 값으로는 이 빌드가 만드는 파이프 깊이의 최댓값이 있다(EVS·SB·SAMPLE·CI 32, `cfe_evs_task.h` L100, `cfe_sb_priv.h` L136, `sample_app.h` L28, `ci_lab_app.h` L47; TO_LAB은 이번에 읽지 않음). 이것도 고른 값이므로 "구성에서 유도한 값"으로 따로 표시해야 한다. 이 문서는 고르지 않았다.
- 큐 바이트 한도(`RLIMIT_MSGQUEUE` 819,200 B)와 `msgsize_max`가 깊이를 올린 뒤에도 충분한지는 계산하지 않았다 [U].

**2026-10-29 메모 (PD29 D1·A3·A4·N-C5-1)** [D][F][M]:
- **D1**: (c)를 쓴다. 전역 `msg_max` 10(D19)은 그대로 둔다.
- 값은 QI에서 유도했다. 판마다 큐 10개, 깊이 12·32·32·12·12(코어 ES·EVS·SB·TIME·TBL), 32·32·8·64·8(SAMPLE·CI·TO 명령·TO 텔레메트리·SCH)이고, 두 판에서 같다(QI §2.2).
  - namespace 안 `msg_max`는 **64**다(QI §7). 위 불릿의 32는 TO_LAB(`to_lab_app.c` L160 `ToTlmPipeDepth = 64`)을 읽기 전의 후보였고, 이 값으로 대체된다.
  - `msgsize_max`·`queues_max`는 새 namespace 기본값(8192, 256)으로 충분해 바꾸지 않는다.
- **A3**: 위 [U]는 닫혔다. 메시지 크기는 4 B(`sizeof(CFE_SB_BufferD_t *)`, `-m32`)다. 커널 식(`ipc/mqueue.c` L365–L371)과 BTF 구조체 크기(48 B, 48 B)로 셈하면 큐 하나가 깊이 × 100 B이고, 합은 24,400 B다. 한도는 819,200 B(soft = hard)다. **환경 차단이 없다.** 깊이는 바꾸지 않는다(QI §5, §8).
- **(b)의 "root로 실행"**: 이 컨테이너에서 성립하지 않는 이유가 소스로 확인된다. 커널은 uid가 아니라 `CAP_SYS_RESOURCE`로 `msg_max`를 넘게 한다(`ipc/mqueue.c` L352–L360). OSAL 4.1.1의 큐 생성 경로에는 root 판별이 없다(`osapi.c` L1536–L1664; `geteuid`는 스케줄용 L435, L587, L872뿐). 그래서 이 컨테이너에서는 root와 일반 사용자의 큐 한도 검사가 같다(QI §3, §6).
- **A4 → N-C5-1** (실행 사용자·스케줄): namespace에서 `msg_max`를 올리면 큐 때문에 root가 필요하지 않다. root(실시간 우선순위)와 일반 사용자(best effort) 사이의 선택은 레퍼런스가 정하지 않는다. 그래서 PI 결정 항목이 되었고, (가) root·실시간 우선순위로 정해졌다(PD29 §3). 근거가 되는 readme 원문은 다음과 같다 [F].
  - 6.4.1 L168–L174 (6.4.2 L171–L177, 같은 글): "The cFE by default needs a larger "msg_max" parameter in linux to run. There are two solutions to this problem: 1. increase the /proc/sys/fs/mqueue/msg_max parameter 2. run the cFE core as root"
  - 6.4.1 L176–L177 (6.4.2 L179–L180, 같은 글): "Root/sudo is also required to enable the linux real time scheduler for more accurate priorty control."
  - readme는 `msg_max`를 올린 뒤 일반 사용자로 돌리는 경우의 스케줄을 적지 않는다.
  - OSAL은 root일 때만 스케줄을 바꾼다 [F]: 주 스레드를 SCHED_FIFO 50으로(`osapi.c` L435–L446), 각 task를 `PTHREAD_EXPLICIT_SCHED`·SCHED_FIFO·재배치한 우선순위로(L587–L640) 만든다. PSP는 사용자를 따지지 않고 시작 스레드를 바꾼다. `CFE_ES_Main`을 부르기 전에 SCHED_FIFO 최대로 올리고(`cfe_psp_start.c` L356–L357), 돌아온 뒤 최소로 내린다(L378–L379). 시작 스크립트 앱의 생성자가 이 스레드다(`cases/C5.md` F8).
- N-C5-1이 실행마다 기록하라고 한 것 가운데 지금 읽은 값 [M, 2026-10-29, 읽기만 함]:
  - CapEff `000001fffeffffff`: `CAP_SYS_NICE`(비트 23) 있음.
  - `kernel.sched_rt_runtime_us` 950000, `sched_rt_period_us` 1000000.
  - `CONFIG_RT_GROUP_SCHED=y`. cpu cgroup(v1)은 `/`.
  - affinity `f`(CPU 4개).
  - 실시간 정책이 task마다 실제로 적용되는지는 실행으로만 보인다 [U].

## 4. 판정 단위 (목표 3a) [D][I]

PD28 §2 항목 6: 순서위반을 판정 단위로 한다. 최종 실패·피해는 따로 판정·보고한다.

- **위반(판정 단위)**: 생성자가 `CFE_ES_AppCreate`로 만든 새 task T가 자기 `TaskTable[TaskId]` 항목을 잠금 아래에서 읽었다(`CFE_ES_GetAppID` 6.4.1 L851–L867, 또는 잠금을 잡은 호출자의 `CFE_ES_GetAppIDInternal` L1873–L1893). 그 읽기가 같은 항목의 등록보다 먼저다. 등록은 `RecordUsed = TRUE` L649, `AppId = i` L651, `TaskId` L652이고, 잠금 독자에게는 L673의 해제에서 보인다. 요구 순서의 출처는 `cases/C5.md` §7이다.
- **따로 보고하는 결과**:
  - O1 `CFE_ES_ERR_APPID`: 등록 전 항목(`RecordUsed`가 TRUE가 아님)을 읽은 결과(L848 초기값, L861 검사).
  - O2 잘못된 AppID의 `CFE_SUCCESS`: 낡은 항목을 읽었거나(`RecordUsed`가 이미 TRUE, L643–L646) `OS_TaskGetId`가 다른 task의 색인을 준 경우(§7). 반환 AppId ≠ 생성자의 `i`.
  - O3 소비(파생): EVS 등록 건너뜀(`cfe_evs_utils.c` L127 → `cfe_evs.c` L105–L107), SB 파이프의 AppId `0xFFFFFFFF`(`cfe_sb_api.c` L173, L224, L237), `RunLoop`의 `FALSE`(`cfe_es_api.c` L551, L653–L661). 원 보고가 적지 않은 crash는 기대 결과에 넣지 않는다(CS §4.3).
- **위반이 아닌 것**: 새 task가 먼저 실행된 것만으로는 위반이 아니다(`cases/C5.md` §7). OSAL 등록(`OS_TaskRegister`)이 ES 등록보다 먼저인 것도 위반이 아니다. 판정 대상은 ES 표의 읽기다.
- **판정 입력 넷** (PD28 Q4): 위반 대상 = T의 `GetAppID` 호출 하나. 관련 task = 생성자(시작 스크립트면 `CFE_ES_Main` 스레드, 명령이면 ES task; `cases/C5.md` F8)와 T. 공유자원 = `TaskTable[TaskId]`이며 키는 OSAL task id. 근거 = §5.2.
- **6.4.2 대조**: 생성자가 L571–L669에서 잠금을 계속 쥔다. 그래서 T의 잠금 읽기는 L669 뒤에만 된다(`cases/C5.md` §8) [I, 코드 읽기]. 같은 사건을 기록하고, 위반이 없다는 것을 같은 판정기로 보인다. 6.4.2 diff에는 Trac #40·#42도 섞여 있다. 판 전체 비교의 결과에는 "여러 수정이 섞였다"를 표시한다(CS §4.3).

## 5. 관측 사건 (목표 3b)

### 5.1 사건과 위치 [F: 이번에 두 tarball에서 다시 확인한 줄]

사건 이름은 CS §4.3의 것이다. "site"는 초안 manifest(§8)의 site 번호다.

| 사건 | 6.4.1 | 6.4.2 | 기록할 값 | site |
|---|---|---|---|---|
| E5-CREATE (요청) | `cfe_es_apps.c` `CFE_ES_AppCreate` L426; 슬롯 예약 L449–L460; 채움 잠금 L571 | 같은 줄 | 앱 이름 포인터, 우선순위 | 1, 2 |
| E5-UNLOCK | L610 `CFE_ES_UnlockSharedData` | **없음**(diff로 제거, `cases/C5.md` F9) | `__LINE__` 인자 | 3 |
| E5-CREATE (호출) | L614 `OS_TaskCreate`(실패 경로 재획득 L628, 해제 L630); OSAL `osapi.c` L480 진입 | L619(잠금 안), 실패 해제 L634; OSAL 같음 | 진입 함수 주소, 이름, 우선순위 | 4, 13 |
| E5-START 전제 | OSAL `pthread_create` L645–L648(이 순간부터 새 스레드가 돌 수 있음) | 같음 | `&OS_task_table[i].id`, 진입 함수, 검증된 이름 | 14, 15 |
| E5-START | 새 스레드의 첫 사건. 진입은 앱 함수 그대로다(예 `SAMPLE_AppMain`, `sample_app.c` L40; OSAL 감싸개 없음) | 같음 | actor(native TID) | 32 |
| E5-REG | 새 task `CFE_ES_RegisterApp` `cfe_es_api.c` L767(잠금 L771) → `OS_TaskRegister` L776; OSAL L906–L948(`pthread_self` L916, 검색 L921–L927, `pthread_setspecific` L938) | L706(잠금 L710) → L715; OSAL 같음 | `pthread_self` 값, 찾은 색인, 반환값 | 18–22 |
| E5-USE | `CFE_ES_GetAppID` L846: 잠금 L851, `OS_TaskGetId` L856, 검사 L861, 성공 저장 L863, 해제 L867. 잠금 없는 변형 L1873–L1893(L1881, L1888) | L785: L790, L795, L800, L802, L806. 변형 L1812–L1832(L1820, L1827) | TaskId, 성공 저장 값, 반환값 | 23–29 |
| E5-RELOCK | L637 | **없음** | — | 6 |
| E5-PUB | L649 `RecordUsed`, L651 `AppId = i`, L652 `TaskId`, L657 `*ApplicationIdPtr = i`, 해제 L673 | L652, L654, L655, L660, 해제 L669 | 저장 값(TaskId, i) | 7–10 |
| 잠금 자체 | `CFE_ES_LockSharedData` L1910(`OS_MutSemTake` L1916), `CFE_ES_UnlockSharedData` L1944(`OS_MutSemGive` L1949) | L1849(L1855), L1883(L1888) | `LineNumber` 인자가 호출 줄을 가름 | 11, 12 |
| E5-CONSUME | `cfe_sb_api.c` L224, `cfe_evs_utils.c` L122–L140 | 같은 줄 | 반환값, 남은 AppId | 30, 31 |

### 5.2 판정 근거 [I]

- O1의 근거는 반환 코드 좁힘(P1 조건부)이다. 구조는 C1의 실패 조회와 같다. `CFE_ES_GetAppID`는 L861 검사가 거짓이면 초기값 `CFE_ES_ERR_APPID`를 돌려준다. 검사는 잠금 아래다. 그래서 반환 코드는 그 시점의 항목 상태를 함의한다(`holds_at return`의 뜻). Φ의 `narrow` 블록은 이 가지(L848, L851–L867)를 인용해 쓴다.
- 실패 가지에는 따로 지나는 문장이 없다(`if`에 `else`가 없음, L861–L865). 그래서 계측 경로도 실패에는 반환 코드 아닌 양의 근거를 주지 못한다. 성공에는 성공 가지 저장(L863, site 25)이 직접 포획 근거다. C1 수정 후 성공 조회 8건과 같은 종류다(`STATUS_20261027.md` §4).
- 생성자의 등록(site 7–10)과 T의 사용(site 24–26)을 잇는 키는 OSAL task id다. 사용 쪽은 `OS_TaskGetId`의 반환(site 24), 등록 쪽은 L652 저장 값(site 9)이다.
- 제어 근거(P11, 선택): `cases/C5.md` A6의 보류점은 6.4.1에서 `OS_TaskCreate` 반환 뒤, L637 재획득 전이다(site 5, `control … ctx=task`). 생성자는 이때 ES 잠금을 쥐지 않는다. 6.4.2에서 대응하는 점은 잠금 안이라 `ctx=lock-held`로 선언했다. 런타임은 계획 점에 `ALLOW_LOCK_HELD`가 없으면 잡지 않는다(ES §7). 6.4.2 대조에서 보류를 어디에 둘지는 A6대로 PI 몫이다.
- **2026-10-29 메모 (PD29 D4)** [D][F][I]:
  - 결정: 자연 실행을 먼저 관측한다. 보류가 필요하면 사건 기반 보류를 쓰되 대상의 잠금 제약을 깨지 않는다.
  - 6.4.2의 잠금 문맥 [F: 6.4.2 원문]:
    - 생성자는 L571에서 ES 잠금을 잡고 `OS_TaskCreate`(L619)를 지나 등록(L652–L660)을 마칠 때까지 쥔다. 해제는 L669다(실패 경로는 L634).
    - 새 task의 첫 ES 호출 `CFE_ES_RegisterApp`(`cfe_es_api.c` L706)은 `OS_TaskRegister`(L715)보다 먼저 같은 잠금을 잡는다(L710). `CFE_ES_GetAppID`(L785)도 L790에서 잡는다.
  - 그래서 L571–L669 안에서 "자식의 `GetAppID`가 관측될 때까지" 생성자를 잡아 두는 계획은 풀릴 수 없다 [I]. 자식은 L710에서 멈추고 `GetAppID`에 이르지 못한다. 보류는 시간 한도나 하네스가 끝낼 때까지 이어지는 정체가 된다. 이것은 race 재현이 아니다. 이런 계획은 쓰지 않는다(D4).
  - 6.4.2에서 보류가 필요하면 잠금 밖의 점(예: L669 해제 뒤, `ctx=task`)만 후보다 [I]. 그 점에서는 등록이 이미 끝났으므로, 같은 제어 모양에서 위반이 없음을 보이는 대조가 된다. 쓸지와 시간 한도는 질문과 근거를 정한 뒤에 정한다(D4).
  - 제어로 얻은 순서는 자연 빈도에 합치지 않는다(D4).

## 6. 등록 전 기록의 요구 (목표 3c)

1. **새 task의 첫 사건부터 남긴다.** 런타임은 스레드의 첫 hook에서 actor를 준다(attach 전이면 부트스트랩 임시 id, ES §8.3). 이 actor는 아직 어느 앱에도 묶이지 않는다. "앱 없음"으로 읽지 않는다(ES §2.2–§2.3).
2. **나중에 잇는다.** 정체성 결합은 actor 단위다(`kairos/include/kz/trace/identity_rule.hpp`). actor가 알린 id X를 X를 배정한 spawn 호출 하나에 묶으면, 그 actor의 앞선 사건도 함께 귀속된다. 6.4.x의 연결 사슬은 다음과 같다.
   - 새 task가 알린 id: `OS_TaskRegister`가 찾아 `pthread_setspecific`에 넘기는 색인(L938, site 20, `thread-entry`). 이것은 task의 첫 사건이 아니다. 6.4.x OSAL에는 id를 넘겨받는 진입 감싸개가 없기 때문이다(`osapi.c` L645–L648, 넷째 인자 `(void *)0`).
   - 생성자가 배정한 id: `*task_id = possible_taskid`(L690, site 16, `spawn-id`). 이 저장은 열린 `OS_TaskCreate` 호출(site 13 `spawn-create`, site 17 `spawn-return`) 안에 있다.
   - 생성자의 앱 슬롯 `i`: L651 저장 값(site 8). 같은 `CFE_ES_AppCreate` 호출 안에서 L614 호출 **뒤**에야 operand로 나온다(§10의 E-ID1). *2026-10-29: §10.4 초안은 소유 앱의 값을 L651(판정하는 공개 자체)이 아니라 L657 `*ApplicationIdPtr = i`에서 읽게 한다. L651은 대조용으로만 쓴다.*
   - 보조: 생성자가 `pthread_create` 직후 읽은 `OS_task_table[i].id`(site 15 `out0`)와 새 task의 `pthread_self()`(site 19). A8 점검에 쓴다(§7.3).
3. **logger가 창을 지우지 않는다.** logger는 ES 잠금을 잡지 않고, `CFE_ES_GetAppID`·`OS_TaskGetId`를 부르지 않고, 등록을 기다리지 않아야 한다. 그렇지 않으면 logger가 T와 생성자 사이에 순서를 끼워 창을 없앨 수 있다. `kzrt`는 이 조건을 설계로 지킨다 [F]: hook은 차단·잠금·계측 코드 호출이 없고(ES §8.1), 런타임은 정체성을 대상의 표에서 조회하지 않으며(ES §8.3), 버퍼가 차도 기다리지 않는다(ES §8.4). 남는 영향은 hook 비용 자체다. 같은 바이너리의 G-I0와 G-I 비교로 따로 재야 한다(미실시). F0의 uprobe도 같은 원칙이지만 probe 비용이 타이밍을 바꿀 수 있다(PI-D1 대상).
4. **등록이 실패하면.** `OS_TaskRegister`가 `OS_ERR_INVALID_ID`를 돌려주면(L932) 알린 id가 없다. 그러면 결합 규칙대로 그 actor는 묶이지 않고 소유자는 `undetermined`로 남는다. 기록에는 그 사용과 TaskId(site 24, 이때 0), 등록 반환값(site 21)이 남는다. 그래서 A8의 다른 경로는 그 자체로 보인다.
5. **시작 스크립트 경로의 생성자는 하나다.** `CFE_ES_Main` 스레드가 스크립트를 차례로 처리한다(`cases/C5.md` F8). 결합 규칙은 이 사실을 쓰지 않는다. 시간·이름·기록 순서로 묶지 않는다.

## 7. A8 기술 확인 (목표 3d)

### 7.1 소스 읽기로 정해지는 것 [F]

- OSAL 4.1.1 posix `OS_TaskCreate`(`osapi.c`):
  - 표 잠금 아래에서 빈 칸을 고르고 `free = FALSE`로 만든 뒤 잠금을 푼다(L519–L553).
  - `pthread_create(&(OS_task_table[possible_taskid].id), &custom_attr, function_pointer, (void *)0)`(L645–L648). 출력 인자가 표의 칸 자체다.
  - `*task_id = possible_taskid`(L690)와 이름·생성자 등 채우기(L695–L704)는 그 뒤다.
- `OS_TaskRegister`(L906–L948)는 `pthread_self()`(L916)를 표의 `id`와 차례로 비교한다(L921–L927). 잠금을 잡지 않고 `free`도 보지 않는다. 처음 맞는 칸에서 멈춘다. 찾지 못하면 `OS_ERR_INVALID_ID`(L932)이고 `pthread_setspecific`(L938)을 부르지 않는다. `OS_TaskGetId`(L959–L972)는 그 thread-specific 값의 하위 16비트이고, 설정된 적이 없으면 0이다.
- glibc `nptl/pthread_create.c`(sourceware 원 저장소, 태그 기준, 이번에 받음; `/home/user/work/c5/src_ref/`, sha256 `sha256.txt`):

| 태그 | `*newthread = (pthread_t) pd;` | 스레드 생성 |
|---|---|---|
| glibc-2.12 (CentOS 6.5 GA의 판 번호, CS §3.3) | L553 | L556 `return create_thread (…)` |
| glibc-2.17 | L561 | L566 |
| glibc-2.39 (이 컨테이너의 판 번호) | L755 | L804, L836 `create_thread (…)` |

  세 판 모두 출력 인자를 먼저 쓰고 스레드를 만든다. 새 스레드는 그 저장 뒤에 존재한다.
- 따라서 [I]: 이 glibc 소스에서는 새 task가 검색할 때 자기 `pthread_t`가 이미 칸에 있다. 검색은 성공하고 `OS_TaskGetId`는 자기 색인을 준다. 창 안의 새 항목 읽기 결과는 O1(`CFE_ES_ERR_APPID`)이다. `cases/C5.md` A8의 "등록 실패 → TaskId 0 → 다른 task의 AppId" 경로는 이 순서로는 생기지 않는다.
- O2가 생기려면 다른 조건이 필요하다 [I, 미확인]:
  1. 낡은 `TaskTable[TaskId].RecordUsed`(재시작·재적재 경로; `cases/C5.md` A8 둘째 추론, L643–L646의 처리).
  2. 앞쪽 칸에 같은 `pthread_t` 값이 남은 경우. 삭제된 task의 칸에 그 값이 남아 있고(`free`를 보지 않는 검색), glibc가 스레드 기술자를 재사용하면 새 task가 그 앞 칸의 색인을 알린다. 시작 스크립트 경로에는 삭제가 없다.
  3. 스레드를 시작한 뒤 출력 인자를 쓰는 C 라이브러리. 이 컨테이너의 glibc 소스는 그렇지 않다.
- POSIX가 출력 인자를 쓰는 시점을 정하는지는 이번에 원문을 받지 않아 확인하지 않았다 [U].
- **2026-10-29 메모** [F]: 위 줄을 다시 받은 원문에서 확인했다. 10-28의 `src_ref/`는 없어졌다.
  - OSAL 4.1.1 `osapi.c`(sha256 `32649c6f…`)의 L519–L553, L645–L648, L690, L906–L948(L916, L921–L927, L932, L938), L959–L972가 위 설명과 같다.
  - glibc `nptl/pthread_create.c`를 sourceware의 같은 URL에서 다시 받았다(`/home/user/work/c5/ref/glibc/`):
    - glibc-2.12 sha256 `e2b78866…`: L553 저장, L556 생성.
    - glibc-2.17 sha256 `7fe22abf…`: L561 저장, L566 생성.
    - glibc-2.39 sha256 `36fd1e6d…`: L755 저장, L804·L836 생성.
  - 결론은 바뀌지 않는다. 새 항목에 대한 창 안의 기본 결과는 O1(`CFE_ES_ERR_APPID`)이다.

### 7.2 실행만 보일 수 있는 것

1. 이 컨테이너의 32비트 `libc.so.6`(Ubuntu `2.39-0ubuntu8.9`)이 원 소스의 순서대로인지. 배포판 패치는 읽지 않았다. 실행은 매번 "검색 성공"이라는 결과를 보일 뿐 순서 자체를 보이지 않는다. 바이너리 디스어셈블로 저장과 `clone`의 순서를 정적으로 볼 수 있다(미실시) [U].
2. T의 `GetAppID`가 실제로 L610–L637 사이에 오는지. 스케줄링이 정한다: root면 생성자는 SCHED_FIFO 최대(`cases/C5.md` §6), CPU 4개, 우선순위는 시작 스크립트 값. 값은 `unspecified_by_reference`다(`cases/C5.md` §10 행 9–11).
3. O2 경로(위 1·2)가 닿는지.

*2026-10-29 메모*: 항목 2의 스케줄 조건은 N-C5-1로 정해졌다. root로 돌리고, OSAL이 실시간 우선순위를 적용한다. 실행마다 기록할 것은 PD29 §3의 N-C5-1 적용 목록이다. 실시간 정책이 실제로 적용되지 않은 실행은 이 구성으로 세지 않는다. 항목 1–3은 여전히 실행으로만 보인다 [U].

### 7.3 두 경로의 관측 설계

| 확인 | 계측 경로 (`kzgcc`, 초안 manifest) | F0 경로 (uprobe; 지금은 ELF32를 못 읽음, §10 E-F0) |
|---|---|---|
| 검색 성공 | site 21 반환 = `OS_SUCCESS` | `OS_TaskRegister` 반환 probe |
| 알린 id = 배정 id | site 20 값 = 같은 결합의 site 16 값 | L938 호출 인자(i386은 스택 인자) = L690 저장 값 |
| 칸의 값 = 자기 스레드 | site 19 = site 15 `out0` | **검색 순간의 칸**: L928 지점에서 `i`와 `OS_task_table[i].id`를 커널이 읽음 |
| 사용한 TaskId | site 24 = site 20 값. 0이면 A8의 다른 경로 | `OS_TaskGetId` 반환 probe |
| 낡은 항목 | 생성자 L643–L646 가지: site 7(`RecordUsed` 저장)이 없고 L645 syslog가 있음 | 같은 가지의 probe |

- 계측 경로는 공유 메모리를 다시 읽지 않는다(PD26 §4). 그래서 "검색할 때 칸에 무엇이 있었나"는 보지 못한다. 생성자가 쓴 값(site 15)과 새 task의 검색 결과(site 20·21)만 본다.
- F0는 probe 순간에 커널이 대상 메모리를 읽을 수 있다. 검색 순간의 칸 값은 F0만 직접 보인다 [I].

## 8. kzgcc와 site manifest (목표 3e)

### 8.1 초안 manifest [D]

`kairos/profiles/c5-cfe-6.4.x/instr/sites-641.draft.kzs`(32 site), `sites-642.draft.kzs`(30 site; site 3·6은 6.4.2에 문장이 없어 일부러 뺌). 둘 다 **초안**이고 어떤 실행에도 쓰지 않았다. `kzs-sites 2`이며 정체성 읽기는 site 13(`spawn-create`), 16(`spawn-id`), 17(`spawn-return`), 20(`thread-entry`)에 둔다. module은 `core-linux.bin` 1, `sample_app.so` 2다. F0 짝이 없어 `equiv` 줄은 없다.

### 8.2 단위 컴파일 확인 [M] (`/home/user/work/c5/scripts/kzgcc_unitcheck.sh`, 결과 `kzgcc_check/64{1,2}/binds/`)

- 방법: 빌드 기록의 컴파일 줄 그대로에 plugin 인자만 더해 6개 번역 단위(`cfe_es_apps.c`, `cfe_es_api.c`, `osapi.c`, `cfe_sb_api.c`, `cfe_evs_utils.c`, `sample_app.c`)를 다시 컴파일했다. 링크와 부팅은 하지 않았다. plugin은 `/home/user/work/instr/gcc/plugin-build/kzgcc.so`(sha256 `56a96b79…`, `kzgcc.cpp` 최종 커밋 `ca9917e`)이다.
- 결과: 6.4.1 32/32, 6.4.2 30/30 site가 `bound`이고 모든 값 슬롯이 켜졌다(`vals` 마스크가 선언과 같음). `inlined`·`duplicated`·`removed`·`unsupported`는 0이다. `-O0`, `pic=2 pie=2`(이 gcc의 기본 PIE)로 적혔다. 경고 수는 기준 컴파일과 같다(`cfe_es_api.c` 3, `osapi.c` 1).
- 메모: site 14의 이름 문자열 슬롯은 `string=type-unverified`로 적혔다. 매개변수가 배열이 아닌 `const char *`라 plugin이 길이를 형으로 확인하지 못한다. plugin은 그래도 상한 복사를 한다(`kzgcc.cpp` L372–L380). 근거는 manifest `ref`의 코드 검사다(L497 NULL, L504 `strlen < OS_MAX_API_NAME`, `osconfig.h` L76 값 20).
- 6.4.x의 계측 빌드 방법(설계, 미실시) [I]: 컴파일 규칙은 `$(COMPILER) $(LOCAL_COPTS) $(INCLUDE_PATH) $(COPTS) …`이고(CS §3.2, `make-rules.mak`; 앱은 `app-rules.mak` L21), `LOCAL_COPTS`는 모든 단위에서 비어 있다. 그래서 `make LOCAL_COPTS="<plugin 인자>"`가 기준 `COPTS`를 건드리지 않고 plugin을 넣는 가장 작은 차이다. 런타임은 코어 링크(`link-rules.mak` L36)에 넣어야 하는데, 그 방법(`LIBS` 재정의 등)은 차이 대장에 적을 빌드 차이다. 앱 `.so`의 hook 기호는 코어가 내보낸다(`-Wl,-export-dynamic`, L21) [I].

### 8.3 런타임의 32비트 빌드 [M]

- `cmake -S kairos/runtime -B /home/user/work/c5/rt32 -DCMAKE_C_FLAGS=-m32`로 `kzrt`, `kzrt-posix`, `kzrt-posix-auto`를 빌드했다. 경고 0, `__atomic_*` 외부 참조 0. 시험은 돌리지 않았다. `.a` 둘만 `rt32_keep/`에 남겼다.
- `g++ -m32` 링크는 실패한다("skipping incompatible …/libstdc++.so"). 32비트 `libstdc++`가 없다. 그래서 **32비트 `kz-rt-dump`를 만들 수 없다.** 저장소 머리에 포인터 크기와 배치 crc가 들어가고(`runtime/src/kz_rt.c` L1483, L1493) 회수는 이것이 같은 도구만 받는다(ES §12.1). 32비트 대상의 회수에는 (가) `g++-multilib` 같은 환경 변경, 또는 (나) 다른 배치의 저장소를 읽는 회수 도구가 필요하다. 이 작업은 설치하지 않았다. 정상 dump 스트림을 64비트 도구가 읽는지는 확인하지 않았다 [U].
- **2026-10-29 메모 (PD29 D5, E-RT)** [D][M]:
  - 결정: (가)로 정해졌다. 32비트 C++ 개발·링크 라이브러리 설치를 허용하고, 대상 런타임과 맞는 32비트 `kz-rt-dump`를 빌드한다.
  - 이 작업은 설치·빌드하지 않았다. 설치는 ENV 작업의 몫이다. 지금 컨테이너에는 `/usr/lib/gcc/x86_64-linux-gnu/13/32/libstdc++.so`가 있다 [M: 파일이 있는지만 봄]. 패키지·판·경로 기록은 ENV 쪽 환경 변경 기록을 따른다.
  - 대응시킬 것: 런타임과 회수 도구의 판, 포인터 크기(4), 바이트 순서(LE), 저장소 배치 crc(`kz_rt.c` L1493), 빌드 설정.
  - 회수 확인: 정상 종료, 비정상 종료(§9 (가)·(나)), 정체 후 종료(§9 (다)) 각각에서 한다. 이 시험은 race 사례로 세지 않는다(D5).
  - 위 §8.3의 10-28 결과(32비트 `.a` 빌드, 링크 실패)는 [기록]이다.
- **2026-10-29 메모 2 (E-RT 실시)** [M] (도구 시험이다. race 사례로 세지 않는다, D5):
  - 빌드: `instrument/gcc/scripts/build_runtime.sh -m32 <dir> <manifest> <kzgcc.so>`. 같은 `kairos/runtime` 소스를 `CMAKE_C_FLAGS`·`CMAKE_CXX_FLAGS` = `-m32`로만 다르게 빌드한다. 컴파일러 gcc 13.3.0, 빌드 형식은 빈 값, `KZ_RT_DEFINES`도 빈 값(런타임 기본 용량)이다. `runtime.record`에 컴파일러, 대상(`x86_64-linux-gnu -m32`), manifest sha256, 라이브러리·객체 sha256, ELF 형식을 적는다.
  - 32비트 `kz-rt-dump`는 ELF 32-bit LSB i386이고 32비트 `libstdc++.so.6`을 쓴다(ENV `env/REBUILD_20261029.md` §1.1의 `lib32stdc++6` 14.2.0).
  - 런타임 시험: 32비트 빌드에서 66개가 모두 통과했다. 세 32비트 빌드(`pre/rt32`, `build/641/rt32`, `build/642/rt32`)에서 같다. 회수 시험 `rt_end_{normal,kill,segv,abort,term,term-kill,notstore}`가 여기에 든다. 각각 정상 종료, SIGKILL, SIGSEGV, abort, 정체 뒤 SIGTERM, SIGTERM을 무시한 정체 뒤 SIGKILL이다(`runtime/tests/CMakeLists.txt` L183–L196).
  - 대응 (`instrument/gcc/scripts/rt_abi_check.sh`, 기록 `/home/user/work/c5tools/pre/abi/rt_abi_check.record`):

| 항목 | 32비트 빌드 | 64비트 빌드 |
|---|---|---|
| 런타임 판 | `kzrt 0.4.0` | 같음 |
| 포인터 크기 (저장소 머리) | 4 | 8 |
| 바이트 순서 | little-endian. 저장소와 dump는 빌드와 무관하게 LE로 쓴다(`runtime/src/kz_rt.c` L59–L81) | 같음 |
| 배치 crc (저장소 머리) | `b7bb3149` | `12584273` |
| pool·boot offset | 104,776 / 39,240 | 104,928 / 39,392 |
| 저장소 크기·용량 | 4,300,800 B, actor 64, pool 4 MiB, chunk 4 KiB | 같음 |
| dump 형식 | `kzrt-dump 1.4` | 같음 |

  - 회수: SIGKILL로 끝난 시험 대상의 32비트 저장소는 32비트 `kz-rt-dump`만 회수했다. 64비트 도구는 포인터 크기·배치 crc·offset이 다르다고 적고 거부했다(rc 2, 쓴 것 없음). 64비트 저장소도 반대로 같다.
  - 64비트 쪽 읽기: 32비트 dump 넷(회수한 kill, 정상, segv, term-kill)을 64비트 `kz-rt-dump`가 검사 통과(0 failures)로 읽었다. 64비트 `test_kzrt_recovered`가 32비트 rt_end dump에서 65 check를 통과했다. 작업 트리 빌드의 64비트 `kz convert --kzrt`가 회수 dump와 정상 dump를 변환했다. 둘 다 사건 900, actor 3이고, 손실은 회수 쪽 1(abnormal-end), 정상 쪽 0이다.
  - 정상 dump를 32/64 빌드에서 비교하면, thread id·시각·반환 주소·actor 번호를 가린 뒤 레코드가 같다(사건 900/900).

### 8.4 계측 파생 빌드 (2026-10-29) [M]

- 스크립트: `kairos/instrument/gcc/scripts/derive_c5.sh`. C1의 `derive_c1.sh`와 같은 틀이다. 작업 공간은 `/home/user/work/c5tools/build/64{1,2}/`, 실행 사용자는 root(§2.1과 같음)다. **부팅하지 않았다.**
- 단계:
  1. `prepare`: §2.1의 tarball 풀기와 DV-1을 `pristine/`에 한다. 트리 해시를 `prepare.record`에 적는다(6.4.1 파일 1,744개).
  2. `build ref`: 사본을 `b/`에 두고 §2.1 명령 그대로 빌드한다(`. ./setvars.sh`, `make config`, `make`). 객체 해시를 적은 뒤 트리를 지운다. 파생 빌드와 같은 경로에서 바이트를 비교하기 위해서다.
  3. `build keep`: 같은 기준 빌드를 `keep/`에 남긴다. §10.5 E-ST의 입력이다.
  4. 런타임: `build_runtime.sh -m32 rt32 sites-64<V>.draft.kzs kzgcc.so`. 설정은 §8.3 메모 2와 같고 manifest만 그 판의 것이다. 런타임 시험 66/66.
  5. `build instr`: `b/`에서 같은 명령에 make 변수 둘만 준다(아래 D3·D3′).
  6. `check`: 객체 비교, binding 병합, vals 마스크, 링크 뒤 hook 검사(`bincheck.sh`, `hookcheck.sh`).
- plugin: `/home/user/work/c5tools/plugin-build/kzgcc.so`, `kzgcc 0.3`, sha256 `27a49a6a…`(E-ID1의 store 연결, §10.4 메모). manifest는 `kzs-sites 3`로 올린 초안이다(§10.4 메모).

이탈 대장 (기준 빌드의 DV-1·DV-H(§2.3)에 더해):

| ID | 이탈 | 레퍼런스 | 사용 값 | 이유 | 영향 |
|---|---|---|---|---|---|
| D1 | 작업 경로 | §2.1 (`b641`/`b642`) | `/home/user/work/c5tools/build/64<V>/{b,keep}/cFE-6.4.<V>-OSS-release` | 새 작업 공간 | 경로 문자열과 디버그 정보. 기준(ref)과 파생(instr)은 같은 경로 `b/`라 서로 비교된다 |
| D3 | `make LOCAL_COPTS="<plugin 인자>"` | 컴파일 규칙의 첫 변수(`cfe/fsw/cfe-core/src/make/make-rules.mak` L17, `app-rules.mak` L21). 단위 Makefile이 빈 값으로 둔다(§8.2) | `-fplugin=<kzgcc.so> -fplugin-arg-kzgcc-manifest=<초안> -fplugin-arg-kzgcc-out=<dir> -fplugin-arg-kzgcc-module=2:<tree>/apps/sample_app/ -fplugin-arg-kzgcc-default-module=1` | 계측 | 이 자리는 모든 단위에서 비어 있었다. 빌드된 Makefile 중 값을 넣을 수 있는 것은 `build/cpu1/cfe/es/Makefile` L26–L30뿐이다(`EXCLUDE_CFE_TBL`이 `TRUE`일 때만 `-DEXCLUDE_CFE_TBL`, 아니면 빈 값). `cfe/{psp,osal,sb,evs,time,tbl}/Makefile`과 앱의 `build/cpu1/<앱>/Makefile` L36은 빈 값을 두고, `cfe/fs/Makefile`은 정하지 않는다. 값을 넣는 `cfe/unit-test/*/Makefile`은 빌드하지 않는다(로그에 `-fprofile-arcs` 0). `EXCLUDE_CFE_TBL`을 정하는 파일은 빌드에 없다. 그래서 기준 로그에서 `-DEXCLUDE_CFE_TBL`이 든 컴파일 줄이 0이라는 것이 확인이 된다(두 판 같음, 2026-10-29 검증 지적 반영). `make config`는 그대로다 |
| D3′ | `make LIBS="<런타임> -lm -lpthread -ldl -lrt"` | `psp/fsw/pc-linux/make/link-rules.mak` L26 `LIBS =  -lm -lpthread -ldl -lrt`, L36 코어 링크 | 앞에 `-Wl,--whole-archive libkzrt-posix.a libkzrt.a -Wl,--no-whole-archive kz_rt_posix_auto.c.o kz_rt_build_info.c.o`(모두 `-m32`), 뒤는 L26 값 그대로 | 런타임은 코어에 링크한다. 앱 `.so`는 코어가 내보낸 hook을 쓴다(`-Wl,-export-dynamic`, L21) | LIBS를 읽는 규칙은 코어 링크 하나다. 앱 링크(L42)는 그대로다 |
| D5 | 디버그 정보의 컴파일 옵션 | `build/cpu1/cfe/debug-opts.mak` L4 `-g -O0` | — | gcc가 `DW_AT_producer`에 옵션을 적는데 plugin 인자도 들어간다 | 객체 66개가 모두 다르다. 그래서 `objcopy --strip-debug` 뒤로도 비교한다 |

결과:

| 항목 | 6.4.1 | 6.4.2 |
|---|---|---|
| `make config` / `make` rc (ref·keep·instr) | 0 / 0 | 2 / 0 (§2.3 "없는 도구"와 같은 이유) |
| 경고 (`make` 부분) | 32 | 32 (§2.4 기록과 같음) |
| 컴파일 줄 / `-m32` / 파생 빌드의 `-fplugin` | 58 / 58 / 58 | 같음 |
| binding (bound) | 33 (33) | 31 (31) |
| vals 마스크가 선언 슬롯과 다른 binding | 0 | 0 |
| 링크 뒤 hook 호출 | 34: `core-linux.bin` obs 31·obs_str 1·ctl 1, `sample_app.so` obs 1 | 32: 29·1·1, 1 |
| hookcheck | ok=33 | ok=31 |
| 다른 객체 (디버그 정보 포함 / strip-debug 뒤) | 66 / 10 | 66 / 10 |
| `core-linux.bin` ref → instr | `116d2295…` 647,132 B → `ad217232…` 692,896 B | `93e465c6…` 647,220 B → `d17661a8…` 692,980 B |
| `sample_app.so` ref → instr | `590d85d7…` → `4d62fc3c…` | `d690cb8e…` → `ff7b4d4a…` |

- strip-debug 뒤 다른 10개는 계측 단위 6개(§8.2의 여섯)와 그 단위를 담은 subsystem의 `ld -r` 객체 4개(`es.o`, `evs.o`, `osal.o`, `sb.o`)다. 다른 것은 없다.
- 모든 binding이 `-O0`, `pic=2 pie=2`로 적혔다. 런타임 기호 28개를 코어가 동적으로 내보내고, `sample_app.so`는 `kz_rt_module`·`kz_rt_obs`를 찾는다.
- E-ID1 site: owner-publish(6.4.1 L657, 6.4.2 L660)와 owner-check(L651, L654)가 `place=store`에 `link=entry`로 묶였다.
- `bincheck.sh`(i386): 32비트 파일에서 binding id는 cdecl 호출의 둘째 인자, 곧 끝에서 둘째 push다. x86-64 파일의 출력은 이전 스크립트와 바이트로 같다(C1 post/1df262a의 tx 셋과 R2 `core-cpu1`, 87줄).
- 런타임 기록 `rt32/runtime.record`: backend `kzgcc 0.3 (gcc-plugin)`, manifest sha256 6.4.1 `6bd88dcb…`, 6.4.2 `1d17d92d…`. 32비트 `kz-rt-dump` sha256 `74b25512…`(두 판 같음. build identity 객체만 다르다).
- 기록: `build/64<V>/{prepare,build_ref,build_keep,build_instr,check}.record`, 로그 `build_<var>.log`, binding `binds_instr/`·`binds_instr.kzs`.
- 확인하지 않은 것 [U]: 부팅하지 않았으므로 hook이 실행 중에 기록하는지, 저장소가 이 대상에서 회수되는지는 모른다.

## 9. 실패·정체 실행의 기록 회수 요구 (목표 3f)

다른 작업이 런타임에 비정상 종료 회수를 넣고 있다(ES §12, 런타임 0.4.0). C5에 필요한 것은 다음과 같다.

1. **6.4.x가 끝나는 길 셋** [F]: (가) 코어 앱 실패 → `CFE_PSP_Restart` → `exit(1|2)`(이번 smoke, §3.2), (나) SIGINT 처리기 → 자원 삭제 → `exit(0)`(`cfe_psp_start.c` L415–L421, 처리기 설치 L270), (다) 정체(보류가 풀리지 않음, T가 `GetAppID`에 오지 않음) → 하네스가 죽임.
2. (나)는 시그널 처리기 안에서 `exit`를 부른다. atexit dump 경로는 pthread mutex를 잡으므로 시그널 문맥에서 안전하지 않다(ES §12.1) [I]. 그래서 C5 실행은 길에 상관없이 **`KZ_RT_STORE` 저장소 파일을 늘 켜고** 회수 dump를 기준으로 삼는 것이 맞다. 저장소는 `/dev/shm`의 tmpfs에 둔다(ES §12.4). 이 설정도 군 사이에 같게 두거나 차이로 적는다.
3. 회수 도구: 32비트 저장소를 읽는 `kz-rt-dump`(§8.3)가 먼저 있어야 한다.
4. 정체: 계획의 보류 시간 한도와 하네스의 SIGTERM → SIGKILL. 회수 dump는 `abnormal-end`를 단다(ES §12.2). 6.4.2의 잠금 안 보류는 정체를 만들 수 있으므로 계획에 넣지 않는다(§5.2). *2026-10-29: D4로 확정. 이유는 §5.2 메모에 적었다. 자식이 `CFE_ES_RegisterApp`의 잠금(6.4.2 `cfe_es_api.c` L710)에서 멈추기 때문이다.*
5. **시작 상태 회복**: (가)는 SysV segment 셋과 키 파일을 남긴다(§3.3). (나)는 지운다(`CFE_PSP_DeleteProcessorReservedMemory`). 하네스는 실행마다 앞뒤 상태를 기록하고, 자기 실행이 남긴 것만 지운 뒤 `--reset PO`로 다음을 시작한다(§3.4). POSIX 큐는 이름이 `/<pid>.<name>`이다(`osapi.c` L1607–L1620). 삭제(`mq_unlink`, L1712)는 (나)의 `CFE_PSP_DeleteOSResources`에서만 불린다. (가)·(다)의 남은 큐는 하네스가 별도 mount namespace의 mqueue 마운트로 찾아 자기 pid의 것만 지운다(이번 §3.3의 방법) [I].
6. 변환기: RUNX 1.4·`abnormal-end`를 중립 층에 옮기는 일(ES §12.6)이 끝나야 회수 dump의 앞부분을 판정에 쓸 수 있다.

## 10. 프로필·site 변경과 엔진 변경 (목표 3g)

10-28 점검표: 새 사례가 새 핵심 판정 알고리즘을 요구하지 않아야 한다. 요구한다면 정확히 무엇인지 적는다.

### 10.1 프로필·site만으로 되는 것 [D][I]

| 항목 | 내용 |
|---|---|
| site manifest | §8의 초안 둘(판마다). 정체성 읽기 포함 |
| Φ 종류 | `es.task(t: Int)`: `TaskTable[t]`, 상태 FREE < LIVE, `table` = `OS_MAX_TASKS`(64, `osconfig.h` L55) |
| Φ 연산 | `CFE_ES_GetAppID`·`CFE_ES_GetAppIDInternal`: 조건 `registered: state(es.task(<T의 TaskId>)) = LIVE`(LP는 L856/L861 쪽), `CFE_SUCCESS: ready`, `CFE_ES_ERR_APPID: not_ready(registered)` + `narrow`(L848, L851–L867 인용, `holds_at return`). `CFE_ES_AppCreate`: `lifecycle es.task(lp.task) -> LIVE`(L652 저장, values task). 정리 경로(`CFE_ES_CleanUpApp` 등)의 FREE는 재시작 시나리오 때 |
| 역할 | 앱 초기화의 `GetAppID` 호출을 `dependent_use`로 선언. 출처: 이슈 본문 "which many tasks call very early in their init procedures", VDD Table 1.2-2 item 2(`cases/C5.md` F1, F13). victim 후보 `sample_app`은 A4대로 보고자의 앱이 아니라고 표시 |
| 기대 결과표 | 실행 전에 동결(C1과 같은 절차). 6.4.1/6.4.2 각각 O1·O2·O3를 "관측 시 기록" 항목으로 두고, 위반의 존재를 예측값으로 넣지 않음 |

### 10.2 엔진 변경 (판정 알고리즘 밖)

| ID | 무엇 | 왜 | 범위 |
|---|---|---|---|
| E-F0 | F0 관측기의 ELF32 i386 지원: ELF 읽기(`probe/anchor/elf_image.cpp` L57–L58), probe 지점 명령 분류, 인자 읽기(i386은 스택), `kz hold`의 32비트 대상 레지스터 대응(`probe/hold/supervisor.hpp` L123의 x86-64 표), 관측 계획의 레지스터 이름(`probe/plan/profile.cpp` L85) | 6.4.x 기준 빌드가 readme대로 32비트(§1.1) | 관측 backend. 없으면 C5는 계측 경로 하나로만 보고 F0 비교가 빠진다 |
| E-ST | `kz-static`·B′의 ELF32 지원(`lib/llvm/elf_info.cpp` L37) | 정적 닫힘·B′ | 분석 입력 |
| E-ID1 | 정체성 결합: spawn 뒤에 operand로 나오는 소유 앱 값(L651 `AppId = i`)을 같은 생성 요청 인스턴스에 붙이는 읽기(예: 요청 안의 `owner-publish`) | 지금 규칙은 spawn이 시작될 때 열린 요청의 owner 인자만 쓴다(`identity_rule.hpp` `owner_from_request`). 6.4.x `CFE_ES_AppCreate`에는 그런 인자가 없다 | 결합 규칙. 없으면 생성자와 task id는 `confirmed`로 묶이고 소유 앱은 `undetermined`로 남는다(앱 이름 보고가 빠짐) |
| E-RT | 32비트 저장소 회수(§8.3) | 실패·정체 실행의 기록 | 도구 또는 환경 |
| E-CV | 변환기의 RUNX 1.4·`abnormal-end`(ES §12.6) | 회수 dump | 다른 작업이 진행 중 |

**2026-10-29 메모 (PD29 D3·D5)** [D][F]:
- E-F0: 보류한다(D3). C5의 첫 검증은 계측 경로이고, 두 경로가 같다고 C5에서 주장하지 않는다.
- E-ST: **계측 경로에도 선행 조건**이다. kz-static의 basis(B)와 B′는 링크 입력·실행 파일·module의 ELF를 읽는다. 그 읽기는 모두 `lib/llvm/elf_info.cpp`의 `load()`(L32–L39)를 거치고, L37이 ELF64 little-endian이 아니면 거부한다. 6.4.x는 `-m32`라 ELF32 i386이다. 그래서 F0를 빼도 정적 닫힘이 막힌다(`REVIEW_ANALYSIS_20261029.md` §1 표).
  - 위 표의 L37이 그 검사 문장이다. RA와 이번 작업 지시는 L36으로 적었는데, L36은 ELF 매직 검사다 [F].
  - 요구 목록은 §10.5에 있다.
- E-ID1: 초안 명세는 §10.4에 있다.
- E-RT: D5로 방법이 정해졌다(§8.3 메모).

### 10.3 확인이 남은 것 (판정 구조)

- 조건식의 키가 호출 **안에서** 얻은 값(T의 TaskId, `OS_TaskGetId` 반환)이다. Φ 문법의 `conjunct`는 `lp`를 받지만 그 식이 `lp.NAME`을 읽을 수 있는지는 확인하지 않았다(`kairos/docs/phi_reference.md` §3 L85, §5.5) [U]. 안 되면 대안이 둘이다. (가) 검사기·평가기가 조건식에서 `values`를 읽게 하는 작은 확장. (나) R2처럼 `exec`의 파생 종류로 두는 방법(`es.self(a)`가 `owner(a)`로 정해지는 형태, `profiles/cfs-v7.0.1/10_kinds.kzp`). (나)는 6.4.x에서 `owner_kind`가 앱 슬롯이면 E-ID1이 필요하다. `AppTable[i].RecordUsed`는 spawn 전에 이미 TRUE(L456, L576)라 앱 상태로 판정하면 틀린다. 어느 쪽도 **새 판정 알고리즘이 아니다**: 판정은 C1과 같은 "공개 전 사용 + 반환 코드 좁힘"이다 [I].
- 다음 단계에서 초안 프로필을 쓰고 `kz phi check`로 위 문제를 먼저 확인한다.
- **2026-10-29 메모** [F]: 첫 불릿의 [U]는 닫혔다. 조건식에서 `lp.NAME`을 이미 읽을 수 있다. 그래서 (가)·(나)의 확장은 필요 없다. 남은 일은 실제 site 값(T의 TaskId, site 24 `OS_TaskGetId` 반환)을 그 연산 점의 `values`에 묶고, 기록을 재생하는 것이다.
  - 검사기: `lib/core/phi_check.cpp` L333(`lp.` 이름 인식), L868–L877(`op_env`가 점의 `values`를 `lp.NAME`으로 환경에 넣음), L1003(conjunct 검사가 그 환경을 씀).
  - 평가기: `lib/core/eval_trace.cpp` L1251, L1596.
  - 문서: `kairos/docs/phi_reference.md` L167(이름 `lp.NAME`), L223(`values`).
- **2026-10-29 메모 2 (Φ 프로필 작성)** [M][F][D-제안]:
  - 위치: `kairos/profiles/c5-cfe-6.4.x/`의 공통 파일 8개(`00_profile`, `01_prov`, `02_researcher`, `05_consts`, `10_kinds`, `20_ops`, `30_hb`, `40_rspec`)와 판 디렉터리 `cfe-6.4.1/`·`cfe-6.4.2/`(각 `00_source`, `01_prov`). C1 프로필처럼 공통 디렉터리에 판 디렉터리 하나를 더해 인스턴스를 만든다. `instr/`·`basis/`는 바꾸지 않았다.
  - 내용: 종류 `es.task(t)`(FREE < LIVE), 연산 `CFE_ES_GetAppID`·`CFE_ES_GetAppIDInternal`(조건 `registered`, `CFE_SUCCESS`·`CFE_ES_ERR_APPID` 매핑에 `narrow`), `CFE_ES_AppCreate`(L652/L655의 `publish`), 다른 writer(`CFE_ES_Main` 초기화, `CreateObjects`, `CreateChildTask`, 해제 셋), `OS_TaskCreate`. hb는 R1과 phase 규칙 R2-P-TASK. 역할 `sb-create-pipe-dependent-lookup`. rspec `K-SELF.es-task`, `K-SELF.es-task-internal`. Φ 사이트 이름은 manifest의 label과 같게 했다(`es.get_app_id.task`, `es.get_app_id_int.task`, `es.app_create.pub_taskid`, `sb.create_pipe.get_app_id`).
  - 검사 [M] (merged 빌드 `/home/user/work/merge-check/kz-llvm`의 `kz-phi`, sha256 `c8ed043c823ffdf4e7b5b142b724da1ea85dcec8860e6d25d22d591802a0da4c`; `kz`에는 `phi` 명령이 없다):
    - `kz-phi check`: 두 인스턴스 모두 오류 0, 경고 2(두 rspec에 oracle 함수 없음: 검사가 조회 안에 들어 있다). 출처 기록 49, 출처가 있는 주장 30, 손으로 넣은 것 1(hb R1), declared-trusted 0.
    - `kz-phi verify-prov --root <pristine 트리>`(`/home/user/work/c5tools/build/64<V>/pristine/cFE-6.4.<V>-OSS-release`): 인용 42개 해시 일치, 문제 0. 식별자 20개 확인. source pin 2개는 `unverifiable`(git 메타데이터 없음).
    - profile hash: 6.4.1 `38a6db1bcdb849c9677628fa61af8d6047a979e0363c85dc827c285c04126c2e`, 6.4.2 `d92989a03cb21effe4f5224adf27ea906b0f70a72894bb8dbbc9ea8ccbb95303`.
  - **위 메모의 정정** [F][M]: `lp.NAME`을 읽을 수 있는 것은 연산의 조건식(conjunct)뿐이다. rspec의 `key`·`requires`는 호출의 매개변수만으로 풀린다(`lib/core/phi_check.cpp` L1288 `op_env(op, false, nullptr)`). 시험 프로필에서 `kz-phi check`가 "unknown parameter 'lp.task_id'"를 냈다.
    - 그래서 프로필은 두 조회에 C에 없는 매개변수 `task_id`를 두었다(researcher 기록 `r_task_id_param`). 점 사이트(site 24·27)의 슬롯 role `task_id`가 그 점 사건의 `arg.task_id`로 실린다(`lib/trace/real_trace.cpp` L117). 조건은 그 값으로 점에서 평가된다.
    - 남는 영향: 발생의 키는 호출 사건에서 정해진다(`lib/core/eval_trace.cpp` L1764–L1772). 키가 비어 per-key 서수와 실행 관계가 서지 않는다(관계 `ownership-undetermined`). 상태와 반환 코드 좁힘에는 영향이 없다.
    - 엔진 항목 제안 **E-KEY**(판정 알고리즘 밖): 호출 안의 점이 키 인자를 주면 그 점에서 발생의 키를 정한다. `lib/` 변경이라 이 작업에서 하지 않았다. 리드 확인이 필요하다.
  - **tarball의 source revision** [D-제안]: `source`는 7–40자리 16진 rev를 요구하고, tarball 트리에는 commit이 없다. tarball sha256의 앞 40자리를 rev로 적었다(cfe 6.4.1 `ec26e33b…a7d05`, 6.4.2 `c64ed2aa…7b49374e`, osal `8f0f2b23…3843441`; 전체 해시는 각 선언의 주석). commit이 아니고, verify-prov는 `unverifiable`로 적는다. 인용은 각자의 해시로 검사된다. 이 표현을 쓸지 리드 확인이 필요하다.
  - **선언하지 않은 것** [F][I]: `context`(소유 앱 종류)는 없다. 6.4.x의 AppId는 AppTable 색인(L651 `AppId = i`)이고 undefined·reserved 값이 없어 `ids`를 레퍼런스로 채울 수 없다(`unspecified_by_reference`). E-ID1의 owner는 기록의 identity 줄에만 남는다. 잠금 HB 규칙은 없다(템플릿에 잠금이 없음). 그래서 6.4.2의 구조적 순서도 반환 코드 좁힘으로만 선다. reads_from(`stored_out`) 규칙도 두지 않았다. 키가 점에서만 정해져 규칙의 자원이 호출 인자로 고정되지 않기 때문이다(E-KEY 뒤에 다시 본다). `scope`(6.4.x의 종료 경로)도 이 부분집합에 없다.
  - **appreq** [D-제안]: 두 rspec 모두 `appreq unspecified_by_reference`, Allowed `unspecified_by_reference`다. 이슈 본문·VDD는 프레임워크 결함의 서술이고, `cfe_es.h`는 공급자 쪽 결과 코드이며, 앱은 반환값을 주석 없이 버린다. 연구자가 만든 appreq는 넣지 않았다(QT-9). C5의 appreq를 둘지는 PI 결정 항목이다(C1의 D8과 같은 종류).
  - 합성 trace 도구 시험 [M] (사례 수에 넣지 않음, C5 기록 아님): `kz-phi replay`에 손으로 쓴 real-record trace를 주어 연결만 확인했다. 실패 코드는 NOT_READY_UNJUDGED, 성공 코드는 READY였고 둘 다 `outcome-narrowed`였다. 키는 `[]`, 관계는 `ownership-undetermined`였다. `plan/expected_results_C5.md`의 라벨은 SPEC 규칙과 코드 읽기에서 적었다. 이 시험의 출력에서 가져오지 않았다.
  - 동결 기대표: `plan/expected_results_C5.md`(별도 커밋, C5 재생 전).

### 10.4 E-ID1: 생성 뒤에 정해지는 소유 앱의 결합 — 초안 명세 (2026-10-29; 같은 날 구현, 절 끝 메모) [D-초안][F][I]

**지금 규칙과 6.4.x의 차이** [F]
- 지금 규칙(`include/kz/trace/identity_rule.hpp` 머리말, `lib/trace/identity_rule.cpp` L11–L26 `owner_from_request`)에서 소유 앱은 spawn 호출이 시작될 때 생성자 actor에 열려 있던 가장 바깥 생성 요청이 정한다.
  - `owner-request`는 요청을 열 때 owner 인자를 읽는다(`owner=` 필수).
  - `child-request`는 요청한 task 자신의 앱이다.
- 6.4.x `CFE_ES_AppCreate`(L426)는 둘 다 아니다. 새 앱을 만드는 요청이고, 그 앱의 슬롯 `i`는 호출 안에서 고르는 지역 변수다(L449–L460).
  - `i`가 정수 operand로 처음 나오는 곳은 `OS_TaskCreate`(L614; 6.4.2 L619) **뒤**의 두 저장이다. L651 `TaskTable[TaskId].AppId = i`와 L657 `*ApplicationIdPtr = i`다(6.4.2 L654, L660).
  - 그 전의 `i`는 배열 첨자(주소 계산)로만 쓰인다. L456, L575–L607, 그리고 L614의 첫 인자 `&CFE_ES_Global.AppTable[i].TaskInfo.MainTaskId`다.
- 그래서 지금 규칙으로도 생성자와 task의 결합은 id로 `confirmed`가 된다(site 20 = site 16). 그러나 소유 앱은 `undetermined`로 남는다(§10.2 E-ID1 행).

**확장 초안**
1. **새 요청 종류 `app-request`** (phase before, 슬롯 없음): 새 앱을 만드는 요청이다. 그 앱의 값은 요청이 끝나기 전에 owner-publish로 나온다. 6.4.x에서는 `CFE_ES_AppCreate` 진입에 둔다(초안 manifest site 1).
2. **새 읽기 `owner-publish`** (phase point 또는 effect, `owner=` 필수, 4바이트 이하 정수 슬롯): 같은 actor에 열린 app-request 호출 인스턴스 안에서, 요청이 요청자에게 돌려주는 앱의 값이다.
   - 짝짓기는 `link`(그 요청을 연 레코드의 aseq)로만 한다. link가 없거나, 다른 actor의 것이거나, 닫힌 요청을 가리키면 쓰지 않고 센다.
   - 6.4.x에서 묶을 곳은 L657 `*ApplicationIdPtr = i`(6.4.2 L660)의 `place=store`, `operand0`이다.
   - **L651(site 8)은 소유 앱의 출처로 쓰지 않는다.** L651은 판정하는 공개(E5-PUB) 자체다. 공개 순서를 분석하는 표를 정체성 근거로 쓰지 않는다는 기존 원칙(`kz_schema.h` L800–L811의 `analysed-table-excluded` 주석, EV-03)과 같은 이유다. L657은 같은 지역 변수 `i`를 요청자의 out 매개변수에 쓰는 문장이고, TaskTable이 아니다 [F]. L651의 값은 대조용으로만 기록한다. 값이 다르면 그 owner를 쓰지 않는다.
   - 한 요청 인스턴스에 owner-publish가 둘 이상이고 값이 다르면 아무것도 쓰지 않는다(`ids_conflicting`처럼 센다).
   - binding은 owner-publish 레코드에 그 함수 인스턴스를 연 레코드(site 1)의 aseq를 link로 줘야 한다. kzgcc가 `place=store` 점에 그 link를 주는지는 확인하지 않았다 [U]. 주지 못하면 구현 전에 연결부를 고쳐야 한다.
3. **`request-return`**: app-request를 닫는다. 선택 슬롯 `status=`와 `ok=`를 받는다(spawn-return과 같은 뜻). 6.4.x에서는 `CFE_ES_AppCreate` 반환(site 2)의 `ret`이고, ok는 `CFE_SUCCESS`(0)이다. status가 ok가 아니면 그 요청의 owner-publish를 쓰지 않는다. 6.4.1에는 L657 뒤의 실패 반환이 없지만(L657–L683, 반환은 L683 `CFE_SUCCESS`) [F], 규칙은 일반형으로 둔다.
4. **결합 규칙** (`owner_from_request`의 셋째 가지):
   - tie는 지금 규칙 그대로다. task가 알린 id(thread-entry)가 spawn 호출이 배정한 id(spawn-id/spawn-return)와 같고, 각각 하나여야 한다.
   - spawn 호출이 시작될 때 열린 가장 바깥 요청이 app-request이면, owner는 그 요청 인스턴스의 owner-publish 값이다. 다음 조건을 모두 채울 때만 owner의 level이 tie의 level(id로 묶였으면 `confirmed`)이 된다: owner 값이 관측됨, request-return의 status가 ok, L651 대조 값과 같음(관측된 경우). 아니면 owner는 `undetermined`다. 이때도 creator·creator_event·spawn_return은 기록한다.
   - evidence: `creation-request`와 `announced-id`에 새 플래그 `owner-at-publish`(spawn 뒤 같은 요청 안에서 정해진 owner)를 더한다.
   - app-request 안에서 owner-request·child-request가 열리면, 지금처럼 가장 바깥 요청을 따른다.
5. **언제 무엇이 확정되는가** (기록 항목):
   - creator, creator_event, spawn_return: tie를 이루는 두 레코드(생성자의 spawn-id/spawn-return, task의 thread-entry)가 있을 때 확정된다.
   - owner: 생성자 actor의 owner-publish 레코드와, 같은 요청의 request-return 레코드가 있을 때 확정된다. actor 기록에 `owner_event`(owner-publish 레코드)를 더한다. 지금의 `owner_request`(요청을 연 레코드)와 `owner_creator`는 그대로 둔다.
   - 귀속은 actor 단위다. owner가 정해지면 그 actor의 앞선 사건(thread-entry 전, owner-publish 전)도 같은 owner로 귀속된다(§6 항목 2와 같은 방식).
   - 정체성 확정은 순서 근거가 아니다(ES §2.10). "task의 사용이 owner-publish보다 먼저"라는 사실은 이 결합에서 나오지 않는다. 그 순서는 판정이 판정 근거로 따로 정한다: 같은 actor의 aseq, 잠금 쌍, 값 연결, 반환 코드 좁힘, 제어 기록.
6. **하지 않는 것**:
   - 이름(AppName, task 이름, 시작 스크립트 줄)을 대조하지 않는다.
   - 시각, 시간 포함, 기록 순서를 쓰지 않는다.
   - 시작 스크립트의 차례로 "다음 슬롯"을 추정하지 않는다.
   - task가 읽은 TaskTable·AppTable 값으로 owner를 정하지 않는다.
   - 시작 스크립트 경로의 생성자가 하나라는 사실(`cases/C5.md` F8)도 쓰지 않는다(§6 항목 5).
7. **두 계열**: 규칙은 `identity_rule`에 두어 F0(`identity_join.cpp`)와 계측 계열이 함께 쓴다. F0 쪽 binding(계획 decoder)은 E-F0와 함께 미룬다(D3).
8. **형식 판** [D-초안]: `app-request`와 `owner-publish`는 `kzs-sites 2`에 없는 reader다. 판 2 파일에 이 둘이 있으면 파서가 거부해야 하므로(§3의 판 1/판 2 규칙과 같은 방식) 새 판(`kzs-sites 3`)으로 둔다. 스키마는 `KZ_DECODER`와 `KZ_OWNER_EVIDENCE`에 항목을 더하고, neutral actor에 `owner_event`를 더한다(스키마 개정).
9. **도구 시험 (구현 때)**: 합성 입력으로 하며 사례 수에 넣지 않는다(PD28 §3).
   - (a) owner-publish 있음, status ok → `confirmed`.
   - (b) status 실패 → owner `undetermined`, tie는 유지.
   - (c) link 없음 또는 다른 actor → 쓰지 않고 셈.
   - (d) 값 두 개가 충돌 → 쓰지 않음.
   - (e) owner-publish 없는 app-request → `undetermined`.
   - (f) task 사건이 owner-publish보다 먼저 기록돼도 귀속은 같고, 순서 간선은 생기지 않음.
   - (g) 기존 owner-request·child-request 시험이 그대로 통과.
10. **초안 manifest와의 대응**: `kairos/profiles/c5-cfe-6.4.x/instr/sites-64{1,2}.draft.kzs`에 주석으로만 적었다. 지금 파서는 판 2만 읽으므로 실제 문장으로 넣지 않았다.
    - site 1에 `identity 1 decoder=app-request`.
    - 새 store site: 6.4.1은 L657, 6.4.2는 L660(`owner=0`).
    - site 2에 `identity 2 decoder=request-return status=0 ok=0`.

**2026-10-29 메모 (구현)** [M][D]:
- 구현 위치: `kairos/runtime/tools/manifest/kz_rt_manifest.{hpp,cpp}`(`kzs-sites 3`), `include/kz/trace/identity_rule.hpp`·`lib/trace/identity_rule.cpp`(결합 규칙), `include/kz/schema/kz_neutral.h`·`kz_schema.h`(스키마 개정 6), `instrument/gcc/kzgcc.cpp`(`kzgcc 0.3`). 커밋 `b73618d`.
- 초안대로인 것: 항목 1–6.
  - 새 reader `app-request`와 `owner-publish`. `request-return`의 `status=`·`ok=`는 판 3에서만 받는다.
  - 짝짓기는 link로만 한다. link 없음, 다른 actor, 닫힌 요청, 값 충돌, 실패 status는 쓰지 않고 센다.
  - 이름·시각·순서는 쓰지 않는다. 정체성 결합은 사건을 더하거나 옮기지 않는다.
- 초안과 다른 것:
  - L651 대조를 새 reader `owner-check`로 둔다. 초안 항목 2는 "대조용으로만 기록"이라 적었고 reader를 정하지 않았다. `owner-check`는 owner를 정하지 않는다. 값이 다르면 owner를 철회만 한다.
  - `owner_event`를 neutral actor 레코드에 필드로 더하지 않았다. 새 섹션 NOWNER(`kz_nowner`: actor, request, publish, request_return, check, owner_id, flags, level)로 둔다. 기존 레코드 크기와 섹션 판을 바꾸지 않기 위해서다. 그래서 이전 container를 그대로 읽고, app-request가 없는 실행은 이전과 같은 container를 쓴다.
  - 항목 2의 [U]는 풀렸다. `kzgcc 0.3`은 owner-publish·owner-check를 `place=store`에서만 묶고, 그 레코드의 link로 함수 진입 레코드를 준다. call·line-start 위치면 `unsupported`로 적는다. 이 reader가 없는 manifest에서는 객체와 bind 줄이 0.2와 같다. 판 문자열은 다르다(아래 C1/R2 동일성의 plugin 항목).
- 형식(항목 8): `kzs-sites 3`. 판 2 파일에 새 reader가 있으면 파서가 거부한다. 스키마 개정 6은 `KZ_DECODER` 값 셋, `KZ_OWNER_EVIDENCE`의 `owner-at-publish`, `KZ_NOWNER_FLAG`, 섹션 NOWNER(type 111)를 더한다.
- 시험(항목 9; 도구 시험이며 사례 수에 넣지 않음): `test_kzrt_app_request` 102 check((a)–(g), 그리고 판 2 identity 파일에서 NOWNER가 없음), `kzgcc_owner_{O0,O2}` 통과, `rt_manifest` 통과.
- C1/R2 동일성 [M]:
  - golden: `layout.golden`에서는 개정 번호(5→6), 새 레코드 `kz_nowner`, 새 enum·flag 값, 새 섹션 행, fingerprint만 바뀌었다. 모두 더한 것이다. `records.golden`은 seed로 채운 패턴 레코드다(`tests/unit/support/fixtures.cpp` `make_record`). enum 값 수·flag 수·섹션 수가 늘면 같은 seed에서 뽑는 값이 밀린다. 33행이 바뀌었고 `kz_nowner` 2행이 더해졌다. 바뀐 행은 모두 ref, `decoder` enum, `KZ_OWNER_EVIDENCE` 필드가 있는 레코드다. 의미가 바뀐 것이 아니다.
  - 분석기 출력: `env/c5/regress_eid1.sh`가 a3c1443 빌드(`/home/user/work/kairos-build`)와 이 빌드로 같은 입력을 돌렸다. 입력은 R2 F0 기록 `env/logs/s1r2_p1.kzt`의 replay(표준 출력과 container), convert, 런타임 시험 dump 셋의 convert다. 다른 줄은 도구 정체성·해시 줄뿐이고, NOWNER 섹션은 생기지 않는다(기록 `/home/user/work/c5tools/eid1/regress2/`). 2026-10-29 검증 지적 뒤 다시 돌린 결과는 아래 "검증 지적 반영" 메모에 있다.
  - plugin: `env/c5/plugin_same.sh`로 C1 pre/post 1df262a와 R2 파생 빌드의 계측 단위(11·11·12)를 kzgcc 0.2와 0.3으로 다시 컴파일했다. 객체와 bind 줄이 모두 같다. binding 파일의 머리 줄(`# kzgcc 0.3 …`)은 다르다(`plugin_same.sh`는 `bind ` 줄만 비교한다).
- 초안 manifest(항목 10): 주석을 실제 문장으로 바꿨다. site 1 `app-request`, site 2 `request-return status=0 ok=0`(`CFE_SUCCESS` `(0L)`, `cfe_error.h` L158), 새 site 33(6.4.1 L657, 6.4.2 L660) `owner-publish owner=0`, site 8 `owner-check owner=0`. 두 판 모두 묶였다(§8.4). 실제로 `confirmed`가 되는지는 실행 기록으로만 확인된다 [U].

**2026-10-29 메모 (검증 지적 반영)** [M]:
- 회귀 입력에 실제 실행을 더했다. 이전 입력(R2 F0 기록, 런타임 시험 dump)에는 실제 cFS의 identity read가 없었다. `env/c5/regress_eid1.sh` 항목 4가 W6 G-I dump 9개를 더한다. R2 `cpu1`, C1 natural tx·rx(pre/post × 1b0b338/1df262a)다.
  - 각 dump는 그 W6 replay가 쓴 manifest 둘과 B′ basis로 변환한다. 경로는 그 replay의 convert 로그와 `sha256.txt`에서 읽고, 해시로 같은 파일인지 확인한다. 이어 W6 replay의 인자로 읽기 cur를 돌린다(주 checkout `env/r2/replay_w6.sh`, `env/c1/replay_w6.sh`).
  - 옛 도구가 만든 container 9개는 모두 W6의 것과 sha256이 같다. 곧 W6 기록이 재현된다.
- 비교 방식을 좁혔다. 이전 스크립트는 정체성 줄을 고정되지 않은 패턴(`source=`, `sha256=`, `parent`)으로 걸렀다. 그래서 진짜 변화가 그 패턴에 걸리면 보이지 않을 수 있었다. 지금은 해시를 담는 줄 형식에만 고정한 패턴으로 그 값만 가린다(`MASK`). 나머지 줄은 모두 같아야 하고, 가린 뒤 다른 줄 수를 적는다. replay가 각 도구가 변환한 실행을 읽을 때는 그 실행의 해시를 같은 이름으로 바꾼 뒤 비교한다. container는 바이트로도 비교해 다른 위치를 적는다.
- 결과(`/home/user/work/c5tools/eid1/regress3/regress_eid1.record`): 비교 47개 모두 가린 뒤 0줄 다르다. kz container(replay·convert·kzrt convert)는 바이트 121–156(36 B)만 다르다. 변환된 실행을 읽은 W6 replay container는 296 B가 다르다. header 해시들, REF_LISTS 섹션의 crc, 그리고 REF_LISTS 레코드 7개의 부모 해시(32 B씩)다.
- 바이트 동일성의 정확한 범위 [M]: 새 container는 a3c1443의 것과 바이트로 같지 않다. 남는 차이는 모두 도구 정체성에서 온다. `KZ_SCHEMA_REVISION` 6이 layout fingerprint를 바꾸고, 그 fingerprint와 소스 정체성이 tool hash와 build identity 문자열에 들어간다(`cmake/KzSourceIdentity.cmake`).
  - kz container: header의 tool hash(32 B)와 header crc(4 B), 곧 바이트 121–156이다.
  - 변환된 실행을 읽은 replay container: 그에 더해 부모 해시, REF_LISTS의 부모 해시와 그 섹션 crc다.
  - C1 basis(§10.5 메모): 바이트 121–156, STRINGS 섹션 crc(201–204), `front_end.build_identity` 문자열의 `schema=`·`source=` 64자씩이다. 159–160 B다.
  - C1 facts: 그에 더해 basis 해시와 parent 레코드, `extract.units_from`의 basis 16자다. 238–240 B다.
  - 그러므로 records 묶음을 tool hash나 schema fingerprint로 대조하는 검증은 새 tool hash를 예상해야 한다. 리드에게 알린다.
- plugin 판 문자열 [M]: kzgcc 0.3은 0.2와 객체·bind 줄이 같다. 그러나 binding 파일 머리 줄에 `0.3`을 적는다. `build_runtime.sh`는 plugin 판을 런타임 build identity에 옮긴다(C5 `build_instr.record`: `backend="kzgcc 0.3 (gcc-plugin)"`). 그래서 C1/R2 파생 빌드를 0.3으로 다시 빌드해 돌리면 RUN/NRUN의 backend가 바뀌고, convert·replay 텍스트의 `run: backend kzrt kzgcc 0.2 (gcc-plugin)` 줄도 바뀐다. 기록된 C1/R2 빌드·실행·출력은 0.2의 것이고 이번에 바뀐 것은 없다. C1/R2를 다시 빌드할 때 0.2 plugin을 쓸지는 리드가 정한다 [U].

### 10.5 E-ST: kz-static의 ELF32 요구 목록 (2026-10-29; 같은 날 구현·실행, 절 끝 메모) [F][I]

**ELF를 읽는 곳** [F]
- 링크 명령의 목적 파일과 아카이브 멤버: 아카이브 선택(`lib/llvm/linkset.cpp` L185, L201, L215).
- 실행 파일과 적재 module의 동적 기호: `lib/llvm/basis.cpp` L688, L707, L746, L801, L835.
- 빌드 식별(`.comment`, build-id): `basis.cpp` L376.
- 셋 다 `elf_info.cpp`의 `load()`를 거친다. ELF를 직접 읽는 다른 곳은 F0 관측기(`probe/anchor/elf_image.cpp`, E-F0, 보류)와 시험용 합성기(`tests/cfs-free/support/elf_synth.hpp`)뿐이다 [M: grep `Elf64|ELFCLASS64|EM_X86_64`].

**요구**
1. **ELF 읽기**: ELFCLASS32·ELFDATA2LSB를 받는다.
   - `Elf32_Ehdr/Shdr/Sym/Nhdr`와 `ELF32_ST_*`로 같은 세 읽기(`read_object_symbols`, `read_dynamic_symbols`, `read_elf_notes`)를 한다.
   - 경계 검사는 지금과 같게 한다.
   - `e_machine`을 읽어 기록한다(6.4.x는 EM_386).
   - ELF64 경로의 결과는 바뀌지 않아야 한다. 기존 시험으로 확인한다.
2. **한 basis 안의 일치**: 목적 파일, 아카이브 멤버, 실행 파일, 적재 module이 모두 같은 class·byte order·machine이어야 한다. 섞이면 오류다. 조용히 넘기지 않는다.
3. **기록**: basis의 Env 항목에 대상의 class·machine·포인터 크기를 남긴다. B′와 32비트 회수 도구의 대응(D5의 포인터 크기·바이트 순서)을 확인하는 데 쓴다.
4. **파일 종류**:
   - `read_dynamic_symbols`는 ET_DYN과 ET_EXEC를 다 받는다(L91). ELF32에서도 같아야 한다.
   - 이 gcc로 만든 6.4.x `core-linux.bin`은 기본 PIE(§2.3 "컴파일러 기본값")라 ET_DYN일 것이다 [기록·I].
   - 앱 `.so`는 ET_DYN이고 DT_TEXTREL이 있다(§2.4) [기록]. TEXTREL은 기호 읽기에 영향이 없을 것이다 [I].
5. **컴파일 DB** (입력 준비, 엔진 변경 아님):
   - kz-static은 `compile_commands.json`을 받는다(`basis.hpp` L29, `ast_basis.cpp` L29). 6.4.x는 classic make라 이 파일이 없다.
   - 빌드 기록의 컴파일 줄에서 만들어야 한다. 만드는 방법과 입력 로그의 해시를 basis 입력으로 기록한다.
   - 컴파일 줄의 `-m32`·`-D` 정의를 그대로 둔다. `-m32`가 없으면 58/58 단위가 실패한다(CS §3.3 ③).
6. **앞단 진단 — PI 결정 필요**:
   - CS §3.3 ⑤에 따르면 clang 18 기본 설정은 두 단위를 `-Wincompatible-function-pointer-types` 기본 오류로 막는다. `osapi.c:647`(`pthread_create`에 함수 포인터)과 `cfe_psp_start.c:270`(시그널 처리기)이다.
   - kz-static의 앞단은 LLVM `1053047a`의 clang이다(PD29 A6). 같은 기본 오류일 것으로 본다 [I, 미시험].
   - `osapi.c:647`은 사례의 새 task 시작 호출(E5-START 전제, site 14)이다. 오류가 난 단위를 버리면 이 호출을 잃는다.
   - 이 진단을 경고로 낮추는 플래그는 레퍼런스에 없다. gcc 13은 플래그 없이 통과하므로, 지금은 `deviation`에 필요한 "그렇게 할 수밖에 없다는 근거"가 없다(CS §3.3 해당 행: "기준 조건으로 쓰려면 PI의 기록된 결정이 필요하다").
   - 분석 전용 앞단에서만 이 진단을 낮추는 것을 기록된 deviation으로 둘지, 다른 방법을 쓸지 정해야 한다. 조용히 적용하지 않는다.
7. **32비트 헤더**: 앞단은 `-m32`로 컨테이너의 multiarch 헤더를 읽는다(CS §3.3). 지금 컨테이너에는 `/usr/include/x86_64-linux-gnu/gnu/stubs-32.h`와 `/usr/lib32/libc.so.6`이 있다 [M: 파일이 있는지만 봄]. 패키지 판은 ENV 기록을 따른다.
8. **상수 평가**: basis Config는 플랫폼 상수를 쓰는 단위에서 평가한다. `-m32` 단위로 평가하면 크기는 i386 값(포인터 4 B)으로 나올 것이다 [I]. 시험으로 확인한다.
9. **recipe** (프로필 작업, 엔진 변경 아님):
   - 6.4.x 링크 명령: 코어 `$(COMPILER) $(DEBUG_FLAGS) -o … $(LDFLAGS) $(LIBS)`, `LDFLAGS = -m32 -Wl,-export-dynamic`(`psp/fsw/pc-linux/make/link-rules.mak` L21, L36). 앱은 `-m32 -shared`(L42).
   - 시작 스크립트 규칙: PO일 때 `CFE_ES_NONVOL_STARTUP_FILE`(`cfe_platform_cfg.h` L856).
   - 가상 경로 `/cf` → `build/cpu1/exe/cf`(`build/cpu1/Makefile` L90).
   - module: 스크립트 둘째 필드의 `/cf/apps/<x>.so` 전체 경로.
   - recipe의 mount·module 규칙(`basis.cpp` L584, L653)으로 적을 수 있을 것으로 본다 [I, 미시험].
10. **도구 시험**: 지금 합성기 `tests/cfs-free/support/elf_synth.hpp`는 ELF64만 만든다. ELF32 합성을 더한다. 정상 파일, 잘린 헤더, 범위 밖 섹션을 시험하고, class가 섞이면 거부되는지 본다. 실제 6.4.x 산출물로 B를 만드는 것은 계측 파생 빌드 뒤에 한다.
11. **범위 밖**: F0의 ELF32(E-F0)는 D3로 보류한다.

**2026-10-29 메모 (구현·실행, D7)** [M][D]:
- 결정: PD29 §3a D7. 분석 전용으로 `osapi.c:647`과 `cfe_psp_start.c:270`이 내는 진단 이름만 경고로 낮춘다. 근거는 실제 빌드의 gcc 13.3.0이 두 파일을 오류 없이 컴파일한다는 사실이다.
- 구현 (엔진, 일반형):
  1. 항목 1–4 (ELF32): `lib/llvm/elf_info.{hpp,cpp}`, `linkset`, `basis.cpp`. ELFCLASS32·ELFDATA2LSB를 같은 세 읽기로 읽고, 한 basis 안의 ELF 정체성이 섞이면 오류로 한다. ELF64가 아닐 때만 `target.elf` 항목을 쓴다. 커밋 `b73618d`.
  2. D7: recipe 문장 `front_end_diagnostic "<단위>" "<진단 이름>" at "<파일>" <줄> … reference "<근거>";`(`at`은 검증 지적 뒤 더함, 아래 "검증 지적 반영").
     - 그 단위에만 `-Wno-error=<이름>`을 더한다. basis에는 deviation(`front-end diagnostic <이름> of <단위>`, 근거 포함)과 항목 `front_end.unit_diagnostic[<단위>][<이름>]`을 남긴다. extract는 basis에서 이 인자를 다시 읽는다.
     - 이름이 아닌 값(`-Wno-error` 등), 근거 없음, `at` 없음, 프로그램에 없는 단위는 오류다.
     - 위치를 recipe로 둔 이유: kz-static의 앞단 설정은 지금 모든 단위에 붙는 `--extra-arg`뿐이다(basis의 `front_end.arguments_added` deviation). 이것으로는 진단 이름을 두 단위에 묶을 수 없다. 그러면 다른 단위가 같은 진단으로 실패해도 조용히 지나가고, D7의 "목록을 늘리지 않는다"를 지킬 수 없다. 단위별 값은 빌드마다 다른 데이터라서 link·module처럼 recipe에 둔다. 인자는 같은 조정 경로로, 모든 단위의 인자 뒤에 붙는다.
     - [D-제안] 이것은 지시("kz-static이 앞단 설정을 받는 방식을 따른다")의 문자 그대로가 아니다. 단위별 recipe 문장이라는 새 경로를 둔 설계 선택이다. 리드(필요하면 PI)의 확인이 필요하다(§11 메모).
     - 기존 `-Wno-error -w`(모든 단위)는 그대로다. 이 둘은 이 작업 전부터 C1을 포함한 모든 단위에 붙는다(SPEC 4.1 단계 2, `lib/llvm/ast_run.cpp`). D7 문구는 "일괄 `-Wno-error`·`-w`는 쓰지 않는다"이다. D7 때문에 더한 것은 없다. 그러나 이 둘은 clang의 기본 오류를 낮추지 않고, 낮춘 뒤의 경고는 `-w`가 extract에서 감춘다. 그래서 basis가 낮춘 자리를 따로 확인하고 적는다(아래). 이 사실은 리드·PI에게 따로 알린다(§11 메모).
  3. classic make 빌드를 읽으려고 더한 것(이 절에 없던 요구):
     - recipe `relocatable "<obj>" link "<file>";`. 코어 링크가 받는 subsystem 객체(`es.o` 등)는 `ld -melf_i386 -o es.o -r …`로 만든다(`make-rules.mak`의 `$(LINKER) -o $(TARGET) -r $(OBJS)`). 그 링크가 받은 객체가 프로그램의 단위가 된다. `-r`이 아니거나, 그 객체를 쓰지 않거나, 아무 링크도 받지 않는 문장은 오류다.
     - `module_file field 1 full_path;`. 6.4.x 시작 스크립트의 둘째 필드는 파일의 전체 가상 경로다(`CFE_ES_ParseFileEntry` L337–L400, `OS_ModuleLoad` L556). 경로·확장자 상수가 없다.
     - 상수 평가가 단위의 디렉터리에서 읽는다(`-working-directory`; extract의 ClangTool과 같다). 6.4.x 명령에는 상대 `-I../../inc`가 있다. clang 드라이버가 이때 프로세스 디렉터리를 바꾸므로 평가 뒤 되돌린다. 매크로 정의 파일도 그 디렉터리 기준으로 적는다.
     - 이 문장이 없는 recipe의 basis는 이전과 같다(아래 C1 동일성). 커밋 `3ab770b`.
  - 시험: `test_static_elf32` 179 check(ELF32, closure, D7 문장, relocatable, 상대 `-I`, full_path), `test_static_link` 68 check. 검증 지적 뒤 `test_static_elf32`는 233 check다(아래).
- 입력 준비 (항목 5·9; 엔진 변경 아님): `env/c5/est_inputs.sh`.
  - 빌드 로그의 `## make` 부분에서 `compile_commands.json`(58 항목)을 만든다. 명령은 로그의 줄 그대로이고, 디렉터리는 make의 "Entering directory" 줄이다.
  - 링크마다 명령 파일 하나(`<디렉터리>/<출력>.link.txt`)를 명령이 돈 디렉터리에 쓴다(`keep/` 트리 안). relocatable 8, 실행 파일 1, 앱 5다. kz-static은 명령 파일의 디렉터리를 작업 디렉터리로 본다.
  - 로그와 생성 파일의 sha256은 `build/64<V>/est/est_inputs.record`에 있다.
- recipe와 profile:
  - `kairos/profiles/c5-cfe-6.4.x/basis/core-linux.kzb`(두 판 공용; 인용한 줄이 두 판에서 같다). 링크 `cfe/link/core-linux.bin.link.txt`, relocatable 8, 앱 module 5(전체 경로; `osloader.c` L260이 `RTLD_GLOBAL`로 열어 scope는 둘 다 global), 시작 규칙 POWERON(`cfe_psp_start.c` L244–L251, readme의 인자 없는 실행), `/cf` → `exe/cf`(`cfe_psp_voltab.c` L42), `front_end_diagnostic` 둘.
  - profile은 머리만 있는 `basis/est-64<V>/00_profile.kzp`다. §10.3의 Φ 프로필이 아니다. source를 선언하지 않았다. source에는 commit(`rev`)이 필요한데 tarball 트리에는 없다. 읽은 파일은 모두 FILE_HASHES에 해시로 남는다.
- 실행: `env/c5/est_run.sh`(기록 `build/64<V>/est/est.record`, 파생 빌드는 `est_instr/`).

| 항목 | 6.4.1 기준 (keep) | 6.4.2 기준 (keep) | 파생 (instr, B′) |
|---|---|---|---|
| basis | rc 0, 단위 58(코어 53 + 앱 5), 경고 1 | 같음 | 두 판 rc 0, 단위 58, 컴파일 명령 없는 링크 멤버 5(런타임), 경고 6 |
| `target.elf` | ELF32 little-endian EM_386, 포인터 4 B | 같음 | 같음 |
| 상수 | `CFE_ES_NONVOL_STARTUP_FILE` = `"/cf/apps/cfe_es_startup.scr"`, `CFE_ES_VOLATILE_STARTUP_FILE` = `"/ram/apps/cfe_es_startup.scr"` | 같음 | 같음 |
| 시작 스크립트 | `exe/cf/apps/cfe_es_startup.scr`, `CFE_APP` 5줄(QI §2와 같음), 앱 module 5개 모두 closure에 링크됨 | 같음 | 같음 |
| extract | rc 0, 함수 771(본문 652), 호출 지점 3412 | rc 0, 772(653), 3411 | 기준과 같은 수 |
| `pthread_create` 호출 사실 | `osapi.c:645:19 OS_TaskCreate -> pthread_create`; arg 0 `&(OS_task_table[possible_taskid].id)`(필드 `OS_task_record_t.id`), arg 2 `function_pointer`(L647, 진단 위치) | 같음 | 같음 |
| `signal` 호출 사실 | `cfe_psp_start.c:270:4 main -> signal`; arg 1 함수 `CFE_PSP_SigintHandler` | 같음 | 같음 |
| D7 deviation | 2건, 근거는 gcc 빌드 로그의 경고 줄 | 같음 | 같음 |

- 진단 확인 [M]: 58 단위 모두를 kz-static과 같은 LLVM(`1053047a`)의 clang으로, 같은 조정 인자(`-Wno-unknown-warning-option -Wno-error -w`)와 `-fsyntax-only`로 읽었다(처음 실행의 스크립트와 출력은 남지 않았다. 검증 지적 뒤 `env/c5/est_probe.sh`로 다시 하고 기록했다, 아래). 기본 오류는 두 판 모두 정확히 둘이다. `osapi.c:647:34`와 `cfe_psp_start.c:270:19`이고, 둘 다 `[-Wincompatible-function-pointer-types]`다. 다른 단위는 실패하지 않았으므로 목록을 늘릴 일이 없었다. 같은 줄을 gcc 13.3.0은 경고 `-Wincompatible-pointer-types`로만 낸다(빌드 로그).
- 대조: 문장 둘을 빼면 extract가 `osapi.c`에서 멈춘다. `osapi.c` 문장만 두면 `cfe_psp_start.c`에서 멈춘다. 두 판 같다.
- 적어 둘 것: closure에서 이름 둘이 여러 단위에 정의된다(`closure.duplicate_definitions` deviation): `__x86.get_pc_thunk.ax`와 `.bx`. i386 PIC 코드에 컴파일러가 만드는 hidden 함수다. 소스 정의가 없으므로 빠지는 사실은 없다고 본다 [I]. 엔진은 이것을 그대로 deviation으로 적는다.
- C1 동일성 [M]:
  - `env/c5/regress_static.sh`로 C1 closure pre/post 1df262a의 basis와 facts를 a3c1443 kz-static(`/home/user/work/kairos-build/k1r3-llvm`)과 이 빌드로 만들었다. kz-dump 텍스트에서 다른 줄은 도구 정체성·해시 줄뿐이다(기록 `/home/user/work/c5tools/eid1/regress_static/`).
  - `static_golden_core_cpu1`의 출력(566줄)도 두 도구가 같다. golden과는 기준 cFS 빌드 날짜에서 오는 3줄만 다르다(ENV §5.2와 같음).
- 남은 것:
  - 정적 사실로 판정하는 일은 Φ 프로필(§10.3) 뒤다.
  - 큐 메시지 크기(`sizeof(CFE_SB_BufferD_t *)`, QI 항목 4)를 이 앞단으로 평가하려면 profile 상수가 source를 요구한다. tarball 트리에는 commit이 없어 하지 않았다 [U]. 포인터 4 B는 `target.elf`로만 확인했다.

**2026-10-29 메모 (검증 지적 반영)** [M][D]:
- 앞단: kz-static의 앞단은 clang 23.0.0git(LLVM `1053047a`)이다. D7이 이름으로 든 것은 clang 18이다. 시스템 clang 18.1.3(`env/REBUILD_20261029.md` §1.3에 적힌 판)도 같은 두 오류를 낸다(아래 probe).
- probe 기록: `env/c5/est_probe.sh <compile_commands.json> <트리> <clang> <출력>`. 단위마다 kz-static이 앞단에 주는 명령(컴파일러·`-o`·`-c`·plugin 인자를 뺀 기록 명령, 모든 단위의 조정 인자)에 `-fsyntax-only -ferror-limit=0`을 붙여 명령의 디렉터리에서 읽는다. recipe는 쓰지 않는다. `est_run.sh` 단계 1a가 두 컴파일러로 돌리고 요약과 오류 줄을 `est.record`에 적는다. 앞단의 드라이버는 kz-static 빌드의 `CMakeCache.txt` `LLVM_DIR`에서, clang 18은 `clang-18`에서 찾는다. 출력은 `build/64<V>/est{,_instr}/probe_{llvm,clang18}.txt`다.
  - 결과: 두 판 × 기준·파생 × 두 컴파일러 모두 "units 58, units rejected 2, errors 2", 둘 다 `[-Wincompatible-function-pointer-types]`다. `osapi.c:647:34`와 `cfe_psp_start.c:270:19`이고, 각 단위에서 한 번씩이다.
- 낮춘 자리의 확인(엔진, 일반형): 문장마다 `at "<파일>" <줄>`로 진단이 나는 줄을 든다. basis는 그 단위를 문장 없이 한 번, 문장의 인자를 붙여 한 번 더 읽는다. 둘 다 `-w` 없이, 오류 수 제한 없이 읽는다(`read_unit_diagnostics`, `lib/llvm/ast_run.cpp`). 첫 번째에서 오류였고 두 번째에서 경고가 된 것(같은 진단, 같은 자리)이 낮춘 것이다. 다음은 모두 오류다.
  - 낮춘 것이 문장의 이름이 아닌 진단이다. 상위 group 이름(`incompatible-pointer-types`)은 그 하위인 `incompatible-function-pointer-types`도 낮추므로 거부된다.
  - 낮춘 자리가 `at`이 들지 않은 줄에 있다(같은 단위에 같은 진단이 하나 더 생긴 경우).
  - `at`이 든 줄에서 그 진단이 오류로 나지 않는다.
  - 인자를 붙인 뒤에도 오류가 남는다. 앞단이 모르는 이름이 그렇다. `-Wno-unknown-warning-option` 때문에 인자가 조용히 무시되기 때문이다.
  - basis는 낮춘 자리마다 `front_end.lowered[<단위>][<이름>]` 항목을 deviation과 함께 남긴다. 값은 파일:줄:열과 앞단의 메시지다.
- C5 recipe: 두 문장에 `at "osal/src/os/posix/osapi.c" 647`, `at "psp/fsw/pc-linux/src/cfe_psp_start.c" 270`을 더했다. sha256 `7bd4d55d…`다.
- 다시 돌린 E-ST(kz-static `3ec20abf…`, 기록 `build/64<V>/est{,_instr}/est.record`; 이전 기록은 `est.record.74e271d`): 네 실행 모두 `front_end.lowered`가 단위마다 "1 occurrence(s), each on a cited line"이다(`osapi.c:647:34`, `cfe_psp_start.c:270:19`). 나머지 기록 줄(basis, extract 수, `pthread_create`·`signal` 호출 사실, deviation 2건, 대조 둘)은 이전 기록과 같다.
- 시험: `test_static_elf32` 233 check. 더한 것:
  - 두 단위가 같은 진단을 내고 문장이 한 단위에만 있으면 basis는 되지만 extract가 다른 단위(`src/core_api.c`)에서 멈춘다. 두 단위 모두 문장이 있으면 둘 다 읽힌다.
  - group 이름, 모르는 이름, `at`이 없는 줄의 둘째 발생, 발생이 없는 `at` 줄, `at` 없음은 오류다.
  - 낮춘 자리 항목의 값(`src/app.c:12:41 …`)을 확인한다.
- C1 동일성 다시 확인(`env/c5/regress_static.sh`, 기록 `/home/user/work/c5tools/eid1/regress_static2/`): 정체성 줄 패턴을 그 줄 형식에 고정했다(§10.4 "검증 지적 반영"과 같은 방식). pre/post 1df262a의 basis·facts·두 로그 모두 가린 뒤 0줄 다르다. 바이트로는 basis 160·159 B, facts 240·238 B가 다르다. 위치는 §10.4 "검증 지적 반영"의 바이트 동일성 항목에 적었다.

## 11. 남은 것과 PI 결정

**2026-10-29 메모 (PD29)** [D]: 아래 "PI 결정 필요" 다섯 항목은 모두 닫혔다.
1. 큐 조건: D1. (c) namespace를 쓰고, 값은 구성에서 유도한 `msg_max` 64다(§3.5 메모, QI §7).
2. DV-H: D2로 허용한다. 수정 전·후에 같은 컴파일러·라이브러리와 대응하는 옵션을 쓴다. 재구축 뒤 실제 판을 다시 기록하고, 10-28 판(§2.3 DV-H 행)을 자동으로 이어받지 않는다. 결과는 "원 소스 기반의 현대 호스트 파생 구성"으로 보고한다.
3. F0: D3. 계측 경로가 먼저다. E-F0는 보류하고, E-ST·E-ID1·E-RT는 진행한다. 두 경로가 같다고 C5에서 주장하지 않는다.
4. 보류: D4(§5.2 메모).
5. 32비트 회수: D5 (가)(§8.3 메모).
- 추가: 실행 사용자·스케줄은 N-C5-1 (가) root·실시간 우선순위로 정해졌다(§3.5 메모).
- 새로 PI 결정이 필요한 것: §10.5 항목 6. 분석 전용 앞단이 clang 기본 오류 두 건을 어떻게 다룰지다.
- 결정 없이 할 수 있는 다음 일(아래 목록)에서 "6.4.x 계측 파생 빌드"는 그대로다. 부팅은 단계 5에서 한다(PD29 §4). E-ID1(§10.4)과 E-ST(§10.5)는 초안을 적었고, 구현은 하지 않았다.

**PI 결정 필요**

1. 6.4.x 실행의 큐 조건(§3.5: (a)–(d)). 정하기 전에는 C5 경로에 이르는 실행이 없다.
2. 빌드 host 이탈 DV-H의 승인(`cases/C5.md` D3a). 이번 빌드는 build-smoke 라벨로만 썼다.
3. F0 비교 경로를 C5에 둘지(E-F0 비용), 아니면 C5를 계측 경로 하나로 보고하고 그 제한을 적을지. PD28 §4는 F0를 비교 경로로 유지한다.
4. 보류(A6)를 쓸지, 6.4.2 대조의 보류점(§5.2).
5. 32비트 회수의 방법(§8.3 (가)/(나)). (가)는 환경 변경이다.

**결정 없이 할 수 있는 다음 일**

- 초안 Φ 프로필과 `kz phi check`(§10.3).
- glibc 2.39 i386 `pthread_create`의 디스어셈블 확인(§7.2 항목 1).
- 6.4.x 계측 파생 빌드(§8.2의 `LOCAL_COPTS` 방법)와 객체 비교. 부팅은 항목 1 결정 뒤.

**2026-10-29 메모 (C5 도구)** [M]:
- 위 목록의 "6.4.x 계측 파생 빌드와 객체 비교"를 했다(§8.4). E-RT·E-ID1·E-ST도 구현하고 시험했다(§8.3 메모 2, §10.4 메모, §10.5 메모).
- 남은 것:
  - 부팅(PD29 §4 단계 5)과 실행 기록. hook이 실제로 기록하는지, E-ID1 owner가 `confirmed`가 되는지, 이 대상의 32비트 저장소가 회수되는지는 실행으로만 확인된다 [U].
  - 초안 Φ 프로필과 `kz phi check`(§10.3).
  - plugin 시험 `kzgcc_target_{O2,O3,O2-lto}`는 두 스레드의 실행 순서에 따라 실패한다(ENV §4.1과 같은 원인; 이번 실행에서는 O3 하나). 시험 파일은 이 작업의 소유가 아니어서 고치지 않았다.
  - `static_golden_core_cpu1`은 ENV §5.2의 3줄 때문에 계속 실패한다. golden은 이 작업의 소유가 아니다.
- 새 PI 결정 항목: 없음.
- 2026-10-29 검증 지적 뒤 리드(필요하면 PI)에게 올리는 것 [D-제안]:
  1. D7을 둔 자리. 지시는 "kz-static이 앞단 설정을 받는 방식을 따른다"였다. 지금 방식인 `--extra-arg`는 모든 단위에 붙어 "목록을 늘리지 않는다"를 지킬 수 없다. 그래서 단위별 recipe 문장 `front_end_diagnostic`을 새로 두었다(§10.5 메모). 이 설계를 받아들일지 확인이 필요하다.
  2. kz-static은 이 작업 전부터 모든 단위(C1 포함)에 `-Wno-error -w`를 붙인다. D7 문구는 이 둘을 쓰지 않는다고 적는다. D7 때문에 더한 것은 없지만, 이 둘은 계속 모든 단위에 걸린다. 그래서 낮춘 경고는 extract에서 `-w`로 감춰진다. 낮춘 자리는 basis가 `-w` 없이 따로 확인해 적는다(§10.5 "검증 지적 반영").
  3. C1/R2 출력의 바이트 동일성 범위와 kzgcc 판 문자열(§10.4 "검증 지적 반영"). records 묶음 검증은 새 tool hash를 예상해야 한다. C1/R2 파생 빌드를 다시 만들 때 쓸 plugin 판도 정해야 한다.

**기존 기록과 다른 점**

- `cases/C5.md` §15가 적은 로컬 사본 `/home/user/work/procrace/src/C5*`는 지금 없다. 이번 사본은 `/home/user/work/c5/dl/`이다.
- `cases/C5.md` A8의 "OS_TaskGetId가 0을 돌려주는 경로"는 glibc 원 소스의 순서로는 생기지 않는다(§7.1). 그 항목을 고칠지는 사례 파일 담당이 정한다.
- CS §3.5의 `msg_max` 영향 추정("root여도 EINVAL일 것")은 이번 smoke로 확인되었다(§3.2).

## 부록 A. 재현

```sh
# 내려받기·해시 (CS 부록 A.1과 같은 URL)
W=/home/user/work/c5; mkdir -p $W/dl && cd $W/dl
curl -sS -L -o cFE-6.4.1-OSS-release.tar.gz "https://sourceforge.net/projects/coreflightexec/files/cFE-6.4.1/cFE-6.4.1-OSS-release.tar.gz/download"
curl -sS -L -o cFE-6.4.2-OSS-release.tar.gz "https://sourceforge.net/projects/coreflightexec/files/cFE-6.4.2-OSS-release.tar.gz/download"
curl -sS -L -o osal-4.1.1-release.tar.gz "https://sourceforge.net/projects/osal/files/OSAL-4.1.1/osal-4.1.1-release.tar.gz/download"
sha256sum *.tar.gz                                   # §1.2 표와 대조
# 빌드 (§2.1), build-smoke (§3.1), 단위 계측 확인 (§8.2)
$W/scripts/build_64x.sh 1; $W/scripts/build_64x.sh 2
$W/scripts/smoke_64x.sh 1 $W/smoke/641; $W/scripts/smoke_64x.sh 2 $W/smoke/642
ipcs -mp                                              # 남은 segment(생성 pid)를 기록하고 자기 것만 ipcrm
$W/scripts/kzgcc_unitcheck.sh 1; $W/scripts/kzgcc_unitcheck.sh 2
# IPC namespace 쓰기 확인 (§3.5 (c); 같은 값을 다시 씀)
unshare --ipc sh -c 'cat /proc/sys/fs/mqueue/msg_max; echo 10 > /proc/sys/fs/mqueue/msg_max && echo ok'
# glibc 원 소스 (§7.1)
for t in 2.12 2.17 2.39; do curl -sS -L -o pthread_create_$t.c "https://sourceware.org/git/?p=glibc.git;a=blob_plain;f=nptl/pthread_create.c;hb=refs/tags/glibc-$t"; done
grep -n 'newthread = \|create_thread (pd' pthread_create_*.c
```

남긴 파일과 sha256: `/home/user/work/c5/ARTIFACTS.sha256`(tarball, 실행 디렉터리 사본, 기록, 스크립트, binding 표, 32비트 런타임 `.a`, glibc 원 소스).

*2026-10-29*: 위 스크립트(`build_64x.sh`, `smoke_64x.sh`, `kzgcc_unitcheck.sh`)와 `ARTIFACTS.sha256`은 컨테이너 교체로 없어졌다. 저장소에도 없다. 다시 만들 때는 §2.1·§3.1·§8.2의 절차를 따르고, 문서와 다른 점은 이탈로 적는다. 내려받기와 큐 목록의 재현은 QI 부록 A에 있다.

*2026-10-29 C5 도구* (작업 공간 `/home/user/work/c5tools/`; 무거운 명령은 `wait_campaign.sh`로 감쌌다):

```sh
R=<worktree>/research/cfs_procrace; K=$R/kairos; S=$K/instrument/gcc/scripts; T=/home/user/work/c5tools
cmake -S $K -B $T/kz-llvm -G Ninja -DKZ_BUILD_LLVM_TOOLS=ON -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
ninja -C $T/kz-llvm && ctest --test-dir $T/kz-llvm
cmake -S $K/instrument/gcc -B $T/plugin-build -DKZGCC_KZ=$T/kz-llvm/tools/kz/kz && cmake --build $T/plugin-build && ctest --test-dir $T/plugin-build
# E-RT (§8.3 메모 2)
$S/build_runtime.sh -m32 $T/pre/rt32 $K/profiles/c5-cfe-6.4.x/instr/sites-641.draft.kzs $T/plugin-build/kzgcc.so
ctest --test-dir $T/pre/rt32 && $S/rt_abi_check.sh $T/pre/rt32 $T/kz-llvm $T/pre/abi
# 계측 파생 빌드 (§8.4)와 E-ST (§10.5)
for V in 1 2; do W=$T/build/64$V
  $S/derive_c5.sh prepare $V $W; $S/derive_c5.sh build $V $W ref; $S/derive_c5.sh build $V $W keep
  $S/build_runtime.sh -m32 $W/rt32 $K/profiles/c5-cfe-6.4.x/instr/sites-64$V.draft.kzs $T/plugin-build/kzgcc.so
  $S/derive_c5.sh build $V $W instr $T/plugin-build/kzgcc.so $W/rt32 && $S/derive_c5.sh check $V $W
  $R/env/c5/est_run.sh $V $W $T/kz-llvm/tools/kz-static/kz-static
  $R/env/c5/est_run.sh $V $W $T/kz-llvm/tools/kz-static/kz-static instr
done
# C1/R2 동일성 (§10.4 메모, §10.5 메모)
$R/env/c5/regress_eid1.sh /home/user/work/kairos-build $T/kz-llvm $T/eid1/regress3          # 검증 지적 뒤 (이전: regress2)
$R/env/c5/regress_static.sh /home/user/work/kairos-build/k1r3-llvm $T/kz-llvm $T/eid1/regress_static2   # (이전: regress_static)
```

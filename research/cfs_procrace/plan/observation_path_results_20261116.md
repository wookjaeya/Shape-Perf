# 첫 자동 관측 경로: 결과 기록 (2026-11-16 시작)

이 문서는 첫 자동 관측 경로(계획 생성 → 파생 빌드 → 자연 부팅 → 설명 기록)를 만드는 작업의 단계별 결과다. 단계마다 절 하나를 둔다: 선언, 바뀐 것, 시험, 바이트 동일성, 디스크, 계획과 다르게 한 것. 기록은 `../records/obs1-20261116-*/`에 둔다. 근거 표기: [F] 파일에서 읽음, [M] 명령으로 잼, [I] 추론, [미확인] 서지 않음.

원칙은 PI 결정 (결정 검토서 2026-10-30; 2026-11-16 재확인. 원문은 이 작업 환경에 없어 대조하지 못함)을 따른다: 사례마다 사람이 짝·순서·오류 조건·관측점을 넣지 않는다; 유한한 예산은 값·이유·잘림·미해결·민감도를 적고 쓴다; 위반·최종 피해·반대 순서의 실현 가능성을 따로 둔다; 실제 cFS 소스와 구성이 평가의 중심이고 합성 시험은 부품 회귀에만 쓴다.

## 1. 바닥 다지기: 공간, 작업 스크립트, 도구 사본, 읽기 (빌드 없음)

기록: `../records/obs1-20261116-ground/` (`README.md`에 파일 목록). 이 단계는 Kairos 도구를 빌드하지 않았다. cFS를 부팅하지 않았다. push하지 않았다.

### 1.1 선언

kz-static 빌드가 없으므로 빌드 전 선언은 없다. 바뀌는 출력도 없다. 이 단계가 고친 것은 저장소 스크립트(`env/`, `kairos/instrument/gcc/scripts/`)와 기록뿐이다. `kairos/instrument/gcc`는 독립 CMake 프로젝트이고 최상위 빌드는 `lib/*`, `probe/*`, `tools/*`, `gate/`, `runtime/`, `instrument/clang`만 더한다(`kairos/CMakeLists.txt:46-61`, `kairos/instrument/gcc/CMakeLists.txt:1-12` [F]). 그래서 그 스크립트는 어떤 도구의 정체성에도 들지 않는다.

### 1.2 공간 먼저 [M]

| 시점 (UTC) | 여유 (df -B1 /) | 일 |
|---|---|---|
| 04:53:50 | 346,578,944 | 시작(하한 314,572,800 위 32.0 MB) |
| 04:55:19 → 04:55:42 | 346,058,752 → 452,665,344 | scratchpad 셋째 사본 101개 지우기 |
| 04:56:31 → 04:57:55 | 452,562,944 → 581,607,424 | 나머지 scratch 무손실 압축 |
| 05:09 | 539,791,360 | 도구 사본(40,216,352 B) 뒤 |
| 05:22:56 | 534,102,016 | 이 단계 끝 |

- 셋째 사본(`scratch_third_copies/`). 대상은 scratchpad 파일 가운데 (가) 2026-11-09 로컬 묶음 다섯의 한 멤버와 sha256이 같고 (나) 그 멤버의 원본(`/home/user/work` 아래)과 `cmp`로 같은 것이다. 먼저 묶음 다섯을 스트림으로 풀어 멤버마다 sha256을 다시 냈다. 다섯 모두 `MANIFEST.tsv`와 줄마다 같다(2,092 멤버; 빈 디렉터리에 아무것도 쓰이지 않음). 후보는 185개였다. 뺀 것: 4,096 B 미만 84개(빈 표지 파일, `0\n` 같은 내용이 우연히 다른 원본과 같은 것; 합 38 KB), 원본이 묶음 뒤 바뀐 1개(`rv4run/ck_sums.new`: CK `SHA256SUMS`가 11-10에 고쳐짐). 남은 101개(106,434,629 B)를 sha256 목록에 적고, 지우기 직전 원본과 다시 `cmp`해 같음을 본 뒤 지웠다. 뒤에 원본 91개를 `sha256sum -c`로, 묶음 조각을 `PARTS.sha256`로 다시 봤다(모두 OK).
- 압축(`scratch_pack/`). 남은 scratch 79묶음(2,226 파일, 134,711,698 B)을 항목마다 `tar | xz -6`로 묶고, 압축본을 스트림으로 풀어 파일마다 sha256이 목록과 같음을 본 뒤에만 원본을 지웠다(79 모두 같음; 11,407,816 B). 링크 1개(`lintroot/lib`)는 tar에 링크로 들어갔다. 뺀 것: 11-11 뒤에 바뀐 현행 작업 파일(약 2.8 MB), 11-10 압축본을 담은 `8cfix/`, 이 작업 디렉터리.
- 둘의 합 +235.5 MB(346,058,752 → 581,607,424). 압축본과 목록 원본은 scratchpad에 있고 scratch다. 근거가 아니다.

### 1.3 작업 스크립트 (바뀐 것)

| 파일 | 무엇 |
|---|---|
| `env/tools/space_guard.sh` | 명령 하나를 공간 감시 아래 돌린다. 시작 전 여유 ≥ 하한 + 필요량이 아니면 시작하지 않는다(rc 75). 2초마다 여유를 적고, 하한 아래로 가면 명령의 프로세스 그룹을 멈춘다(rc 76). 끝 줄에 가장 낮은 값, dip, 변화, 도구 종류(`--kind`) |
| `env/tools/build_record.sh` | 빌드 트리의 전·후 기록: 도구 일곱의 sha256·mtime·크기, `kz-static --identity`, `KZ_TOOL_SOURCE_SHA256` 정의, `CMakeCache.txt`와 모든 `flags.make`의 해시, HEAD, `kairos` 미커밋 줄 수, 여유. 형식은 `records/step2a-20261110-8cfix/pre_mc.txt`와 같다 |
| `records/TOOLS.tsv`, `env/tools/tools_check.sh` | 도구 등록부(도구 해시마다 한 줄: 있는 자리, 사본, 처음 기록, 상태 in-place / copy / in-place+copy / gone). 확인은 파일 해시, `xz -t`, `--deep`이면 사본을 스트림으로 풀어 해시 |
| `records/LEDGER.tsv`, `env/tools/ledger_check.sh` | 증거 장부: 누적 바이트 동일성 목록(1.5). 상태 네 칸(local, verified_copy, remote, published)을 따로 둔다 |
| `env/redesign/one_build.sh`, `env/redesign/compare_build.py` | 단일 빌드 실행기: `step2a_tests.sh`의 `run` 단계와 같은 입력·인자로 한 빌드만(basis·extract·relations). 출력은 모두 `NAME.partial`로 쓰고 모든 단계가 0이고 출력이 비지 않을 때만 이름을 바꾼다. 실패하면 `NAME.failed`가 되고 `status.txt`가 FAIL이며 rc 1: 실패한 extract나 relations가 완전해 보이는 `facts.kz`·`relations.txt`를 남기지 않는다. 단계마다 공간 감시. `facts.txt`는 `ONE_BUILD_SHOW=1`일 때만. 비교 스크립트는 `compare_8cfix.py`의 비교 함수를 그대로 쓰고, 허용 목록 밖의 차이가 있으면 rc 1이다 |
| `env/tools/chain_check.py` (`python3 -I`) | 사슬 점검: manifest(계획 sha256 = 빌드 기록 = run.meta = dump의 manifest_sha256), 실행 파일(빌드 기록 = 파생 트리의 파일 = run.meta), facts의 빌드(basis `binary.sha256` = run의 주 실행 파일), same_build(파생 트리 파일의 GNU build-id를 옆에 적음; 런타임 1.4 dump에는 build-id가 없어 늘 `unconfirmed(no build-id in dump)`) |
| `env/tools/hash_instance.sh`; `run_gi_c1.sh`, `run_gih_c1.sh`, `run_c1.sh`, `env/c5/run_c5.sh` | 부팅 전 인스턴스 디렉터리의 모든 파일(실행 파일, 모듈, 표, 시작 스크립트)을 해시해 `inst_files.sha256`에 쓰고 run.meta에 한 줄. C5는 `exe/`에서 제자리로 돌므로 그 디렉터리를 해시한다. `run_gi_c1.sh`는 7번째 인자로 site manifest를 받는다(없으면 예전 손 manifest) |
| `kairos/instrument/gcc/scripts/derive_args.sh`, `derive_c1.sh`, `derive_c5.sh`, `derive_plan.sh` | 파생 빌드가 manifest, module 표(모듈 id, 이름, 소스 접두어; `default` 줄), 점검할 실행 파일 목록을 인자(`--manifest --modules --exes`)로 받는다. 셋은 함께이거나 없거나다. 없으면 예전 고정값(손 manifest, `2:<W>/src/cf/`, `3:<W>/src/bp/`, 기본 모듈 1 또는 4, 고정 실행 파일 목록)이 예전 기록의 재실행에만 남는다. 생성된 계획의 입구 `derive_plan.sh`는 셋이 없으면 시작하지 않고, module 표의 첫 줄이 그것을 낸 계획 sidecar를 이름과 sha256으로 가리키지 않으면(파일이 없거나 해시가 다르면) 거부한다 |
| `env/redesign/step2a_tests.sh`, `step2a_tests_unit.sh` | 2a 드라이버는 kz-static 파일의 sha256이 `0024f192…`가 아니면 시작하지 않는다(rc 2; `STEP2A_KS_PIN`이 다른 값을 명시할 수 있다). 단위 점검 7을 더했다 |

이름 점검 스크립트(패턴 목록과 selftest)는 저장소 밖에 둔다. 커밋마다 staged diff와 새 출력 디렉터리 이름에 돌린다.

### 1.4 시험 [M]

합성(먼저 깨끗한 대조, 그다음 결함마다 기대 rc):

- `TMPDIR=<scratch> bash env/tools/tests/run_unit.sh` → 8 of 8: (1) 공간 감시: 대조 rc 0, 시작 거부 rc 75, 하한에서 멈춤 rc 76(가짜 writer 20단계 중 5단계 뒤); (2) 빌드 기록: 대조 rc 0, 도구 하나 없음 rc 1; (3) 장부: 대조 0, 파일 없음·바이트 바뀜·묶음 조각 깨짐 각 rc 1; (4) 도구 등록부: 대조 0, 도구 바이트 바뀜·사본 없음 각 1; (5) 단일 빌드 실행기(가짜 kz-static): 대조 PASS, extract 실패·relations 실패·sha256 목록 실패 각 rc 1이고 최종 이름의 `facts.kz`·`relations.txt` 없음, 입력 표가 `step2a_tests.sh`와 같음, 비지 않은 OUT rc 2; (6) 사슬: 일관 → confirmed(0), 실행 파일 한 바이트 → refused(1), 다른 manifest → refused(1), B′ 없음(트리·basis 없음) → unconfirmed(3); (7) 인스턴스 해시: 파일 2개와 링크 1개, 있는 목록 rc 64; (8) 파생 인자: 대조는 derive 스크립트를 부름, sidecar 참조 없음·sidecar 해시 다름·`--exes` 없음 각 rc 64, module 옵션이 표대로, 부분 계획에 `derive_c1.sh`·`derive_c5.sh` rc 64.
- `bash env/redesign/step2a_tests_unit.sh` → 7 of 7(점검 7: 고정되지 않은 kz-static → rc 2, 대조 rc 0).
- 이름 점검 selftest: 심은 13개를 찾고 깨끗한 8개를 통과.
- lint: MC에서 `ctest -R '^lint'` 6 of 6 통과(빌드 없이 지금 `kz_lint` `b0f0d6a5`로; 바뀐 `kairos/instrument/gcc/scripts/`는 `case-ids` 범위 밖이다). 빌드가 없으므로 전체 ctest는 돌리지 않았다.

실제(빌드 없음):

- MC 빌드 전 기록(`pre_mc.txt`): 도구 일곱의 sha256과 mtime·크기, `--identity`(tool_sha256 `3b7de5b7…`, tool_source `bbd48436…`, core_source `f6e3c7b4…`), 정의 넷이 `records/step2a-20261110-8cfix/post_mc.txt`와 같다. `CMakeCache.txt` `bc2f43b4…`는 PV 사본과 같다. HEAD `b4f1065`, `kairos` 미커밋 0줄.
- 단일 빌드 실행기, c5_641, 지금 도구 `0024f192`, 새 디렉터리 `/home/user/work/redesign/obs1/runner_check_c5_641`: rc 0, 약 10초. T8CF `c5_641/NP`와 파일마다 같음 8(`args.txt`, `basis.kz` `1481cc9d…`, `basis.txt`, `extract.txt`, `facts.kz` `e95bf36c…`, `relations.log`, `relations.txt` `1d4af508…`, `sha256.txt`), 한쪽에만 3(`facts.txt`는 쓰지 않음; `status.txt`, `space_guard.log`는 새 것), 허용 밖 0(`runner_check/compare.tsv`). 감시가 잰 dip: basis 65,536 B, extract 4,882,432 B, relations 598,016 B. 출력은 대조를 적은 뒤 지웠다(T8CF와 같다).
- 사슬 점검(`chain/`):
  - W6 C1 pre/1df262a tx: manifest `1f37f87c…`가 손 manifest, 빌드 기록, run.meta, dump(run과 module 1-3)에서 같다 → confirmed. 실행 파일 여섯(core-tx `5390d150…`, core-rx `eaba099b…`, cf.so `9b4cbff3…` 둘, bp.so `22867274…` 둘)이 빌드 기록, 파생 트리, run.meta에서 같다 → confirmed. 파생 빌드의 basis가 없으므로 facts_build unconfirmed, 사슬 unconfirmed(rc 3). 기준 basis(T8CF c1_pre_tx, `binary.sha256 = 0052e7ab…`)를 주면 run의 core-tx `5390d150…`와 달라 facts_build refused(rc 1): 2a facts는 기준 빌드의 것이고 부팅한 파생 빌드의 것이 아니다. 파생 core-tx의 build-id는 `ff7119f4…`(dump에는 없음).
  - C5 6.4.1: manifest `6bd88dcb…` 네 곳 같음, 실행 파일 여섯(core-linux.bin `ad217232…` 등) 같음, 기준 basis `867499f7…` 대 run `ad217232…` → facts_build refused. build-id `9c8cd965…`.
- 인스턴스 해시, 읽기 전용, `W/w6/run/c1nat/gi/pre_1df262a_tx/inst`: 파일 41, 다른 종류 0, 목록 sha256 `53d94a02…`. `tx/cf/cf_def_config.tbl`은 `455d265e…`다(`harness/`).
- 등록부: 26줄(있음 18, gone 8), `tools_check.sh --deep` 실패 0. 장부: 29줄, `ledger_check.sh` 실패 0.

### 1.5 바이트 동일성: 누적 목록 [M]

`records/LEDGER.tsv`가 목록이고 빌드마다 `ledger_check.sh` 전체를 다시 돌린다: T/, T_RV4, T_8B, T_8BF, T_8C, T8CF(각 `sha256.txt`: 125, 136, 137, 136, 136, 136 파일), PV 두 목록(5 + 4), 11-09 묶음 다섯의 조각, kairos-build 도구 다섯, 플러그인 둘(kzgcc 0.2 `56a96b79…`, 0.3 `27a49a6a…`), F1T(`W/instr/gcc/f1`, 16,444 파일), C5 641 `b/`(1,867 파일), W6·C5 기록 묶음(저장소의 조각)과 그 원본(W6 c1-predict 190, replays 1,066, runs2 1,449; C5 boots 40, replays 68), 이 단계의 도구 사본. 오늘 모두 같다. 한 가지: 묶음 `w6-20261029-c1-runs`의 원본 가운데 `run/campaign.log`는 그 묶음 뒤에 자랐다. 그 뒤의 바이트는 `w6-20261029-runs2`에 있으므로 c1-runs는 조각만 본다.

### 1.6 도구 사본 [M]

`/home/user/work/preserve/tools-20261116/`(기록 사본 `tool_copies/`): kz-static `0024f192`(xz 39,253,560 B; 2a 출력의 마지막 도구이고 다음 kz-static 빌드가 제자리에서 덮는다), kz_lint `b0f0d6a5`(944,160 B), MC `CMakeCache.txt`, 도구 일곱의 `flags.make`. 왕복: 열 개 모두 `xz -t` OK, `xz -dc | sha256sum` = 원본 목록, `xz -dc | cmp` = 원본(`ROUNDTRIP.txt`). kz `931dd39c`, kz-instr-clang, kz-phi, kz-rt-dump, 두 플러그인은 다음 단계들이 덮지 않는다. 덮는 단계 전에 복사한다.

### 1.7 읽기 과제 (`readings/`)

- 종료 상태 [M]. F0 기록에도 종료 상태 값이 있다: `W/w6/run/c1nat/f0tx/pre_1df262a/run.kzt`를 kz-dump(`05623054`)로 풀면 `CFE_ES_ExitApp` 입구 site 8의 slot 0이 `role="exit_status"`(SITE_SLOTS[21])이고, 그 사건 RAW_EVENTS[100]의 `a[0] = 0x3`이다. `pre_1b0b338`도 같다(같은 줄). 파생 빌드 G-I 기록은 `tx.dump.txt:218-219`(CF의 호출 전과 ES 입구, `v=[0x3]`)이다. 3은 `CFE_ES_RunStatus_APP_ERROR`다(bundle 1df262a `cfe/modules/core_api/fsw/inc/cfe_es_extern_typedefs.h:139` [F]). 그래서 `plan/ovm_evidence_20261111/p_offline.md:27`의 "Not in the record: … exit status value"는 F0 기록에 대해서는 맞지 않는다: 값은 원시 사건에 있고 텍스트 요약(`run.kzt.record.txt`)에 나오지 않았을 뿐이다. 그 줄은 이 단계에서 고치지 않았다(문서 주인의 결정).
- 부팅 전 두 표(`startup_tables.txt`) [F][M]. (가) 시작 스크립트의 앱마다 첫 run-loop 호출과 종료 호출이 facts에 있다: C1 ci_lab, to_lab, sch_lab, cf, bp 모두, C5 sample_app, ci_lab, to_lab, sch_lab 모두(sample_lib은 라이브러리 초기화뿐). W6 파생 빌드의 module 표로는 cf → 2, bp → 3이고 ci_lab, to_lab, sch_lab은 따로 된 공유 객체인데도 기본 모듈 1(core의 id)을 받는다. 거기 site를 두었을 때 등록이 맞게 되는지는 [미확인]이다(그런 site를 빌드한 적이 없다). 손 manifest에는 run-loop 호출 site가 없고, 종료는 ES 입구 site 9(모든 앱)와 CF의 호출 site 37뿐이다. (나) 시작 시간 한계: C1 pre·post는 `CFE_PLATFORM_ES_STARTUP_SCRIPT_TIMEOUT_MSEC 1000`(`bpxfer_defs/default_platform_cfg.h:1731`; 쓰임 `cfe_es_apps.c:505`, `cfe_es_start.c:208`, `:223`), `CFE_PLATFORM_ES_STARTUP_SYNC_POLL_MSEC 50`(`:1689`; `cfe_es_apps.c:510`, `:530`), `CFE_PLATFORM_CORE_MAX_STARTUP_MSEC 30000`(`:1713`; `cfe_es_start.c:811`). 예전 자연 기록 20개 콘솔(G-I 12, F0 8)에 "Startup Sync failed" 0, "Unable to obtain Entry Point" 0, OPERATIONAL 각 1: 모두 한계 안에서 RUNNING에 닿았다. 한계에 얼마나 가까웠는지는 기록에 없다 [미확인]. C5 6.4.1에는 시작 스크립트 시간 한계 상수가 없고, `CFE_ES_WaitForStartupSync`(`cfe_es_api.c:676`)를 부르는 단위가 `apps/`와 `cfe-core/src`에 없다. 예전 자연 부팅은 끝 줄 "ES Startup: CFE Core Startup Complete"를 1,095 ms에 읽고 멈췄다(`W/c5run/641/run.meta:32-34`; 한계 30,000 ms는 하네스의 도구 값).
- C 라이브러리(`c_library.txt`) [M]. C1 실행 파일은 64-bit, `NEEDED libc.so.6`, 해석기 `/lib64/ld-linux-x86-64.so.2` → `/usr/lib/x86_64-linux-gnu/libc.so.6`(`3a15d668…`, Ubuntu GLIBC 2.39-0ubuntu8.9, 패키지 libc6:amd64). C5는 32-bit, `NEEDED libc.so.6`, `/lib/ld-linux.so.2` → `/usr/lib32/libc.so.6`(`50deb775…`, 같은 2.39-0ubuntu8.9, libc6-i386). 기준 빌드와 파생 빌드가 같다. 로컬 C 라이브러리 사본 `W/c5/ref/glibc`는 upstream `pthread_create.c` 2.12·2.17·2.39다. 배포판 패치가 그 함수를 건드리는지는 [미확인]이다.

### 1.8 계획과 다르게 한 것

- 2a 드라이버의 고정: `kz-static --identity`가 내는 tool_sha256은 `3b7de5b7…`이고 `0024f192…`는 파일의 sha256이다(`post_mc.txt`). 그래서 파일 sha256을 고정했다.
- 종료 상태의 줄: G-I dump에서 값은 `tx.dump.txt:218-219`에 있다(226-227이 아니다).
- 셋째 사본은 101개 106.4 MB였다(이전 목록 158개 109.7 MB와 다름; 다시 잼). 4,096 B 미만과 원본이 바뀐 1개를 뺐다.
- 압축에서 11-11 뒤의 현행 작업 파일과 11-10 압축본 디렉터리를 뺐다(이득 123.3 MB).
- 도구 사본에 kz-static의 것만이 아니라 도구 일곱의 `flags.make`를 모두 넣었다(합 약 4 KB).
- 단일 빌드 실행기는 기본으로 `facts.txt`를 쓰지 않는다. 그래서 T8CF 대조에서 `facts.txt`는 한쪽에만 있다(허용 목록에 적음).
- `derive_plan.sh`는 module 표의 sidecar 참조만 검사한다. manifest 머리의 sidecar 해시 형식은 계획 생성기와 함께 정한다.
- 장부의 `w6-20261029-c1-runs`는 원본이 아니라 조각만 본다(1.5).

### 1.9 남은 것과 다음 빌드 전의 수

- 지금 여유 534,102,016 B(05:22:56Z), 하한 위 219.5 MB. 가장 최근의 kz-static 제자리 빌드는 74,153,984 B 내려갔다(`records/step2a-20261110-8cfix/commands.txt` 6절: 662,728,704 → 588,574,720) [F].
- 이 단계는 공간·도구 덮어쓰기·빌드 트리·정지 규칙에 대한 결정을 하지 않았다. 그 질문은 lead가 측정값과 함께 PI에게 올린다.

## 2. relations 행의 기록, 사용 지점 키, 공유 적재기 (kz-static 빌드 하나, 동작 변화 없음)

기록: `../records/obs1-20261116-relrows/`. 출력: 저장소 밖 `/home/user/work/redesign/obs1/relrows_check/`(새 디렉터리). T8CF(`/home/user/work/redesign/step2a/tests_8c_fix`)와 앞선 출력 디렉터리에는 쓰지 않는다. cFS를 부팅하지 않는다. push하지 않는다.

### 2.0 선언 (빌드 전에 적고 커밋함) [D]

빌드 전에 적었다. 아래 밖의 차이는 결함이다. 결함이면 다음으로 가기 전에 고친다.

- **코드**(설계, `kairos/`):
  - `lib/llvm/rel_facts.hpp`, `lib/llvm/rel_facts.cpp`(새 파일): facts 섹션 읽기(`load`), 색인(`index`과 그것이 부르는 `thread_end_code`, `image_entries`, `image_value_why`, `global_address_functions`, `callback_args`), 정적 행위자(`actors`, `own_shared_access`, `actor_set`, `entry_actors`), `call_runs`, `shared_base`, 작은 도움 함수(`str`, `fname`, `pos`, `call_pos`, `enum_text` 등)와 그 상태를 `relations.cpp`의 `Rel`에서 그대로 옮긴다(`RelFacts`). 규칙 글은 바꾸지 않는다. 바뀌는 것은 하나다: 위치·함수 이름을 적을 때 그 파일을 행의 기록에 적는 일을 `Rel`의 멤버 집합에 바로 하지 않고 `file_notes_`가 가리키는 집합(=`Rel`의 그 집합)에 한다. 같은 집합에 같은 차례로 들어간다.
  - `lib/llvm/relations.hpp`, `relations.cpp`: 행을 기록(`RelRow`: 경로, 번호, 첫 줄, 줄들, 칸 여섯, 등급, 사실 참조, 사용 지점 키)으로 만들고, 텍스트는 그 기록에서 그린다(`relations_rows`, `render_row`, `render_relations`). 행이 아닌 줄은 묶음(`RelGroup`)의 글이다. `relations_report`는 `relations_rows` + `render_relations`다. 사용 지점 키(`key_a`, `key_b`, `key_c`)와 그 목록(`relation_keys_text`). 키의 정의는 `docs/static_facts.md` §5l. relations rules revision 5 그대로(개정 문자열 그대로).
  - `tools/kz-static/cmd_show.cpp`, `main.cpp`: `kz-static relations <facts> [-o OUT] [--keys KEYS]`. `--keys`가 없으면 앞과 같다.
  - `lib/llvm/CMakeLists.txt`: `rel_facts.cpp`를 더하고 머리 주석 두 줄.
  - `tests/static/test_static_cli.cpp`: 사용 지점 키 블록(줄이 밀림, creator가 바뀜, 구획이 바뀜에도 A 행 키가 같음; 한 줄 두 호출의 키가 다름; `--keys`가 텍스트를 바꾸지 않음; fixture `relations.c`의 다섯 행 키). 다른 확인 글은 고치지 않는다.
  - `docs/static_facts.md`: §5l(새 절), §5e의 명령 줄에 §5l 가리킴 한 문장.
  - 하지 않는 것: 규칙, 행의 차례와 번호, 등급, 칸 글, facts schema(`include/` 그대로), golden, 2a 드라이버와 그 허용표.
- **도구 해시와 정체성**(`cmake/KzSourceIdentity.cmake:77-116`의 규칙: 도구의 정체성은 그 도구가 링크하는 대상들의 소스 디렉터리와 `include/`, `cmake/`, 최상위 `CMakeLists.txt`다):
  - kz-static: 파일 sha256, `--identity`의 `tool_sha256`, `tool_source`가 바뀐다(`lib/llvm`, `tools/kz-static`이 바뀜). `core_source` `f6e3c7b4…`는 그대로다 [I: `lib/core`, `include/`를 고치지 않음].
  - kz `931dd39c…`, kz-dump `05623054…`, kz-rt-dump `4390d4f1…`, kz-phi `7d34e6e2…`, kz-instr-clang `28db4017…`, kz_lint `b0f0d6a5…`: 다시 빌드·링크되지 않아야 한다 [I: 어느 것도 `kz-llvm`을 링크하지 않는다(`grep -rn kz-llvm --include=CMakeLists.txt`: `tools/kz-static`과 `tests/static`뿐); `include/`, `cmake/`, 최상위 `CMakeLists.txt`, `runtime/`, `tests/lint/`를 고치지 않는다]. 빌드 전 sha256은 `pre_mc.txt`에 적고 뒤와 비교한다. `flags.make`의 정체성 정의는 kz-static 것만 바뀌고 kz, kz-phi, kz-dump 것은 그대로다.
  - 다시 링크되는 시험 프로그램: `kz-llvm`을 링크하는 것(`test_static_expr`, `test_static_flow`, `test_static_recipe`, `test_static_gitrev`, `test_static_unroll`, `test_static_wrappers`, `test_static_link`, `test_static_elf32`)과 `test_static_cli`(소스가 바뀜). `static_golden`은 `kz-core`만 링크하므로 다시 링크되지 않아도 된다(다시 링크되어도 결함이 아니다: kz-static에 대한 `add_dependencies`).
  - `CMakeCache.txt` `bc2f43b4…`는 그대로다 [I]. flags.make 목록의 해시(`pre_mc.txt`의 sorted-list 줄)는 kz-static의 `flags.make` 하나 때문에 바뀐다.
- **golden**: 바뀌는 파일도 새 파일도 없다(`tests/static/data`, `tests/golden/data`, `tests/phi/data`). relations가 나오는 golden 넷(`relations`, `relations_np`, `relpaths_np`, `relsound_np`와 그 밖에 relations를 그리는 것)은 같은 바이트여야 한다. 다르면 결함이다.
- **relations 다섯 빌드 재실행**(새 도구, T8CF의 facts 그대로, facts는 다시 뽑지 않음): `kz-static relations <T8CF>/<b>/NP/facts.kz -o relations.txt > relations.log 2>&1`, 그리고 `--keys`를 더한 둘째 실행. 다섯 빌드 모두 둘 다 `relations.txt`가 T8CF와 바이트 동일해야 한다: c1_pre_tx `936cd6c8…`, c1_post_tx `00b57eb2…`, r2_core `fc8ea8da…`, c5_641 `1d4af508…`, c5_642 `34ac45f1…`. `relations.log`는 T8CF처럼 빈 파일이다. 둘째 줄 `facts <hash>`도 같다(facts가 같으므로).
  - 이 단계가 다시 돌리지 않는 것: basis, extract, NP2, identity, count, twice, 드라이버 비교. 새 도구로는 "확인하지 않음"이다("같음"이 아니다).
- **새 출력**: 빌드마다 사용 지점 키 목록(`keys.txt`, 옆 파일). 남겨 둔다(합 약 1 MB 이하 [I]). 재실행의 `relations.txt`·`relations.log`는 비교를 적은 뒤 지운다(T8CF와 같으므로 다시 만들 수 있다).
- **측정(판정이 아님)**: 빌드마다 행 수, 서로 다른 키 수, 키를 나누는 행(있으면 그 행들을 적는다), B 사이트 키의 겹침. c1_pre_tx의 A21과 c1_post_tx의 `cf_cfdp.c:1003:23` 행의 키가 같은지 [I: 구조가 같으므로 같다].
- **소비자**: 2a 드라이버(`env/redesign/step2a_tests.sh`)는 이 단계에서 돌리지 않는다. kz-static 파일 sha256이 `0024f192…`가 아니면 시작하지 않는다(rc 2). 그 허용표와 `step2a_tests_count.py`의 셈(행 정규식 `:51`, `summary `·`feasibility: ` 줄)은 kz-static `0024f192`까지의 도구를 말한다. 탐침 `s1_rev1.py`, `clauseB_rev1.py`(`--row`, `clauseB_rev1.py:676`)는 다시 돌리지 않는다. 그 행 id는 T8CF의 것이다. 새 relations에 다시 돌리면 행을 사용 지점 키로 맞춘다.
- **시험**: 전체 ctest 239개 그대로(새 ctest 없음), 실패 0, 건너뜀 1(`test_probe_anchor_reached`, tracefs 없음). `test_static_cli`의 확인 수는 는다(뒤에 적음). lint 6/6. 단위 점검 `env/tools/tests/run_unit.sh` 8/8, `env/redesign/step2a_tests_unit.sh` 7/7.
- **누적 바이트 동일성**: 빌드 뒤 `env/tools/ledger_check.sh` 전체(T/, T_RV4, T_8B, T_8BF, T_8C, T8CF, PV, 11-09 묶음, kairos-build 도구, 플러그인 둘, F1T, C5 641 `b/`, W6·C5 기록, 앞 단계의 도구 사본)와 `tools_check.sh --deep`. 모두 같아야 한다.
- **도구 보존**: kz-static `0024f192`는 이 빌드가 제자리에서 덮는다. 사본은 앞 단계에서 만들었다(`/home/user/work/preserve/tools-20261116/kz-static.0024f192.xz`, 왕복 확인됨). 새 도구는 그 도구의 남은 출력 가운데 relations 다섯만 다시 만든다. basis·facts는 다시 만들지 않으므로 `0024f192`를 "대체됨"으로 적지 않는다. 등록부의 상태는 `copy`가 된다.
- **공간**: 시작 전 여유 ≥ 하한 314,572,800 + 필요량. 필요량 = 가장 큰 kz-static 제자리 빌드 dip 92.6 MB + 새 출력 1 MB + 한 시간 흐름 13 MB = 106,600,000 B. 감시(`env/tools/space_guard.sh --kind kz-static`, 2초마다)가 하한을 넘으면 멈추고 보고한다. 재실행도 감시 아래 돌린다(`--kind relations`).

### 2.1 바뀐 것 [F]

- 커밋 `ab182a9`(선언·코드·시험·문서를 함께, 빌드 전). 규모(`git diff 1b8d82f ab182a9 -- kairos`): `lib/llvm/rel_facts.cpp` +594(새), `rel_facts.hpp` +235(새), `relations.cpp` +366/−799, `relations.hpp` +59, `lib/llvm/CMakeLists.txt` +5/−1, `tools/kz-static/cmd_show.cpp` +13/−4, `main.cpp` +2/−2, `tests/static/test_static_cli.cpp` +98, `docs/static_facts.md` +27/−1.
- `RelFacts`(`lib/llvm/rel_facts.hpp`): facts 섹션, 색인, 정적 행위자. `relations.cpp`에서 옮긴 코드이고 규칙 글은 그대로다. `prepare()` = `load()`, `index()`, `actors()`. 파일 기록은 `file_notes_` 포인터로 한다(relations가 제 행 기록 집합을 가리키게 한다).
- `RelRow`, `RelGroup`, `RelReport`(`lib/llvm/relations.hpp`), `relations_rows`, `render_row`, `render_relations`, `relation_keys_text`. 행의 줄은 `RowLines`(줄마다 이름을 단다)로 쓴다. 칸 여섯은 기록의 값이고, 그리는 쪽이 이름표를 붙인다. `relations_report`는 `relations_rows`를 그린 것이다.
- 사용 지점 키(`docs/static_facts.md` §5l). A: `A|이름 공간|부르는 함수|키 매개변수 함수#k|n`. B: `B|필드|-|callee#k|-`와 사이트마다 `B-site|필드|부르는 함수|callee#k|n`. C: `C|필드|생성자|spawn callee|n|read 읽는 함수 n|맥락`. 줄·열, creator, writer, 구획, 계수, 판정은 넣지 않는다.
- `kz-static relations <facts> [-o OUT] [--keys KEYS]`.

### 2.2 빌드 [M]

- 빌드 전(`pre_mc.txt`, 05:41:27Z): 도구 일곱이 앞 단계 `pre_mc.txt`와 같다. HEAD `ab182a9`, `kairos` 미커밋 0줄.
- MC 제자리 `make -j4 all`(05:41:33Z–05:44:01Z), 공간 감시 아래. rc 0, 첫 번에 됐다. CMake가 다시 구성했다. 링크는 `libkz-llvm.a`, kz-static, `kz-llvm`을 링크하는 시험 여덟, `test_static_cli`(11줄). kz_lint, kz-phi, `static_golden`은 "Built target"뿐이다. 빌드 시작 뒤 정체성 경로의 파일은 바뀌지 않았다(`find -newer` 0).

| 도구 | 전 | 뒤 |
|---|---|---|
| kz-static | `0024f192980dfbd93e4a662666239c3b4082fcb65f3bd5d25a19fbbfc06d2431` | `a763e2550ad6902f102d5a1c2d310c4a658a36c2087b1b6f6717d3d4b1193ed7` (176,674,872 → 178,301,376 B) |
| kz-static tool_sha256 | `3b7de5b7…` | `90046f5de050d8b49b1acb4dcb33ea4db6c3da07107cdb41e1fc8725f5d2710d` |
| kz-static tool_source | `bbd48436…` | `b9a4e4431a7af93675be307aa82372f863409da5c9d4ec29bec7133695cfa545` |
| core_source | `f6e3c7b4…` | 같음 |
| kz / kz-dump / kz-rt-dump / kz-phi / kz_lint / kz-instr-clang | `931dd39c` / `05623054` / `4390d4f1` / `7d34e6e2` / `b0f0d6a5` / `28db4017` | 모두 같음(다시 링크되지 않음) |
| kz, kz-phi, kz-dump의 정체성 정의 | `caf58235…`, `7fc3ed73…`, `cc51ff35…` | 같음 |
| `CMakeCache.txt` | `bc2f43b4…` | 같음 |
| flags.make 목록 해시 | `b0f15434…` | `b354fbbf…` (kz-static의 `flags.make`) |

- 선언대로다.

### 2.3 시험 [M]

- 전체 ctest(05:44:19Z–05:44:41Z): 239개, 실패 0, 건너뜀 1(`test_probe_anchor_reached`, tracefs 없음; `skip_reason.log`). 라벨 수는 §16.10.4와 같다(legacy-case 51, unit 84, runtime 66, static 45, golden 58, lint 6).
- `test_static_cli` 확인 798개(앞 752, +46: 사용 지점 키 블록). `lint_selftest` 102 경우, 실패 0. lint 6/6.
- golden: `tests/static/data`, `tests/golden/data`, `tests/phi/data`에 바뀐 파일도 새 파일도 없고, relations를 그리는 golden(`relations`, `relations_np`, `relpaths_np`, `relsound_np` 등)이 그대로 통과한다.
- 합성 시험(사용 지점 키 블록): 표를 찾는 lookup API를 한 줄에서 두 번 부르는 원본, 줄이 셋 밀린 것, creator 둘을 더한 것, 구획 상수 3을 4로 바꾼 것. 넷 모두 A 행 둘의 키 줄이 같다. 그 사이 텍스트는 위치, `creator:` 줄 수(1 → 2 이상), `partition {3}` → `{4}`로 바뀐다. 한 줄의 두 호출은 `…|t_find#0|1`과 `…|t_find#0|2`로 키가 다르다. `--keys`가 있어도 텍스트 바이트는 같다. fixture `relations.c`: 행 다섯, 서로 다른 키 다섯, B1 사이트 키 둘.
- 단위 점검: `env/tools/tests/run_unit.sh` 8/8, `env/redesign/step2a_tests_unit.sh` 7/7.

### 2.4 바이트 동일성 [M]

- relations 다섯 빌드(새 도구, T8CF의 NP facts 그대로; 05:44:58Z–05:45:04Z, 새 디렉터리 `/home/user/work/redesign/obs1/relrows_check`): `--keys` 없는 실행과 있는 실행 모두 `relations.txt`가 T8CF와 `cmp`로 같다. `relations.log`는 두 실행 모두 빈 파일로 T8CF와 같다. 20 of 20 same, DIFF 0(`byte_identity.txt`).

| 빌드 | relations.txt (두 실행) |
|---|---|
| c1_pre_tx | `936cd6c8…` |
| c1_post_tx | `00b57eb2…` |
| r2_core | `fc8ea8da…` |
| c5_641 | `1d4af508…` |
| c5_642 | `34ac45f1…` |

- 다시 돌리지 않은 것(새 도구로는 확인하지 않음): basis, extract, NP2, identity, count, twice, 드라이버 비교.
- 재실행의 relations 출력은 비교를 적은 뒤 지웠다(T8CF와 같다). 남긴 것은 키 목록 다섯(합 340,285 B; `keys.sha256`; 장부 행 `OBS_relrows_keys`).
- 누적 목록: `ledger_check.sh` 30행, 실패 0. `tools_check.sh --deep` 27행, 실패 0. 등록부에서 `0024f192`는 `copy`가 되었다(사본 `/home/user/work/preserve/tools-20261116/kz-static.0024f192.xz`). 새 도구는 그 도구의 relations 출력 다섯만 다시 만들었으므로 "대체됨"으로 적지 않는다. `a763e255`는 `in-place` 행으로 더했다.

### 2.5 사용 지점 키의 측정 [M]

| 빌드 | 행 | 서로 다른 키 | 키를 나누는 행 | 겹친 B 사이트 키 | 목록 바이트 |
|---|---|---|---|---|---|
| c1_pre_tx | 431 | 431 | 0 | 0 | 96,613 |
| c1_post_tx | 431 | 431 | 0 | 0 | 96,619 |
| r2_core | 338 | 338 | 0 | 0 | 77,674 |
| c5_641 | 178 | 178 | 0 | 0 | 35,051 |
| c5_642 | 175 | 175 | 0 | 0 | 34,328 |

- c1_pre_tx의 A21(`cf_cfdp.c:993:19` in `CF_CFDP_InitEngine` → `OS_CountSemGetIdByName#1`, 이름 공간 `OS_common_record_t.name_entry`)과 c1_post_tx의 A21(`cf_cfdp.c:1003:23`, 같은 이름 공간)은 키가 같다(`17975444d83f6589`, 키 글 `A|OS_common_record_t.name_entry|CF_CFDP_InitEngine|OS_CountSemGetIdByName#1|1`). c1_post_tx에는 같은 자리의 행이 하나 더 있다(A48, 이름 공간 `OS_filesys_internal_record_t.virtual_mountpt`, 키 `61d5d789b1133a4f`). 그래서 위치로는 행이 하나로 정해지지 않고, 키는 이름 공간을 담아 둘을 가른다.
- c1_pre_tx와 c1_post_tx가 함께 가진 키: 431개 가운데 425개. 나머지 여섯씩은 이 측정에서 더 보지 않았다 [미확인: 두 빌드 사이 소스 차이에서 오는지].

### 2.6 공간 [M]

| 시점 (UTC) | 여유 (df -B1 /) | 일 |
|---|---|---|
| 05:41:07 | 529,895,424 | 이 단계 시작(하한 위 215.3 MB) |
| 05:41:33 | 529,571,840 | 빌드 시작(감시 문턱 421,172,800 = 하한 + 106,600,000) |
| 05:42:01 | 436,551,680 | 빌드의 가장 낮은 점(dip 93,020,160) |
| 05:44:01 | 516,796,416 | 빌드 끝(변화 −12,775,424) |
| 05:45:04 | 504,549,376 | relations 재실행 끝(dip 11,857,920; 지우기 전) |
| 05:47:40 | 515,837,952 | 기록을 쓴 뒤 |

- 하한을 넘지 않았다. 이 빌드의 dip 93.0 MB는 앞의 kz-static 제자리 빌드 dip(74.2–92.6 MB)의 위 끝을 조금 넘는다. 순변화 −12.8 MB는 앞의 −0.8–−3.8 MB보다 크다: kz-static이 1.6 MB 커졌고 나머지는 빌드 트리의 새 목적 파일·시험 프로그램이다 [I: 파일별로 재지 않음].
- 지운 것: 재실행의 relations 출력(다시 만들 수 있음, T8CF와 같음)뿐이다. scratchpad의 다른 것, 도구, 저장소 자료, 묶음, PV, CK, T8CF, 빌드 트리는 건드리지 않았다.

### 2.7 계획과 다르게 한 것

- 키의 정의를 경로마다 적었다. 계획의 정의(경로, 이름 공간 또는 필드, 부르는 함수, callee, 키 매개변수 위치, n 번째 호출)는 경로 A의 lookup에 맞는다. 경로 B의 행은 (필드, callee, 인자) 묶음이고 사이트가 여럿이다: 행 키에는 부르는 함수와 n을 넣지 않고(`-`), 사이트마다의 키를 목록에 따로 둔다. 경로 C의 행은 spawn 호출 하나에 읽기와 맥락이 여럿이다: spawn 호출의 키에 읽기(읽는 함수와 그 함수 안 같은 필드 읽기의 n)와 맥락(종류, 맥락 호출의 부르는 함수·callee·n)을 더했다. 그래도 줄·열 번호는 넣지 않는다. 다섯 빌드에서 키 겹침은 0이다.
- 키는 sha256의 앞 16 hex 자리다. 목록에는 키 글을 함께 적어, 겹침을 키와 글 둘로 센다.
- 공유 적재기는 지금 relations만 쓴다. 순서 명령과 계획 생성기는 아직 없다(뒤 단계에서 같은 `RelFacts`를 쓴다).
- 키 목록은 저장소 밖 출력 디렉터리에 두고 저장소에는 sha256만 둔다(`keys.sha256`).
- 이 빌드의 dip(93.0 MB)과 순변화(−12.8 MB)는 계획의 추정(74–93 MB, 출력 약 1 MB 외 −0.8–−3.8 MB)의 위 끝이거나 그 밖이다. 다음 kz-static 빌드의 필요량은 이 값으로 다시 잡는다.

## 3. relations 규칙 개정 6: 잘린 정적 이미지, 행마다의 미완 사유 (kz-static 빌드 둘)

기록: `../records/obs1-20261116-relincomplete/`. 출력: 저장소 밖 `/home/user/work/redesign/obs1/relincomplete_check/`(다섯 빌드 재실행)와 `/home/user/work/redesign/obs1/relincomplete_sens/`(민감도 실행), 둘 다 새 디렉터리. T8CF(`/home/user/work/redesign/step2a/tests_8c_fix`)와 앞선 출력 디렉터리에는 쓰지 않는다. cFS를 부팅하지 않는다. push하지 않는다.

### 3.0 선언 (빌드 전에 적고 커밋함) [D]

빌드 전에 적었다. 아래 밖의 차이는 결함이다. 결함이면 다음으로 가기 전에 고친다.

- **판정 규칙의 어휘(문서 변경, 빌드 전)**: `plan/ovm_rev1_20261116/JUDGEMENT_RULES_REV1.md` 3.5의 사유 표에 세 줄을 더한다: `intervention_out_of_scope`(이 단계는 개입을 하지 않음), `reading_options_differ`(사실을 읽은 옵션이 빌드의 옵션과 다름), `build_mismatch`(기록과 사실이 다른 빌드에서 옴). 셋 각각의 뜻과 기존 사유에 들지 않는 까닭, 하위 사유 표기 `사유(하위 사유)`와 그 예(`record_insufficient(key text)`, `link_unlinked(no creation token)`, `link_unlinked(no creation-argument channel)`, `key_unresolved(runtime table buffer)`, `key_unresolved(pointer field without points-to)`), 관측 필요(need)의 상태 여덟과 사유의 대응 표 하나, "자기 식별이 다름"·"대기 시간 초과"는 사유가 아니라 값이라는 문장을 그 절에 덧붙인다. 모두 [보강 제안]이다. 그 밖의 줄은 바꾸지 않는다.
- **코드**(설계, `kairos/`):
  - `lib/llvm/rel_facts.hpp`, `rel_facts.cpp`: `image_cut(object)`(객체의 GLOBAL_IMAGES 에 표시 잎 `<image truncated>` 이나 `image-truncated` 플래그가 있으면 표시 경로들). `image_value_why` 에 넷째 인자 `cut`(비지 않으면 첫 이유: 이미지가 예산에서 잘림). `image_entries`: 잘린 이미지에서 찾은 함수 잎은 entry 로 남기고 그 spawn 을 `image_cut_` 에 적는다. 찾은 잎이 없으면 풀리지 않은 spawn 의 이유에 잘림을 적는다. 표시 잎은 개정 5 의 "읽은 멤버를 덮는 상수 아닌 잎" 수에서 뺀다.
  - `lib/llvm/relations.hpp`, `relations.cpp`: 개정 문자열 "relations rules revision 6 (2026-11-16)". 사유 표 `relations_incomplete_reasons()`(사유, 하위 사유, 규칙; 36 항목)와 `reason_text`. `RelRow::incomplete`. 행마다 `layers:` 다음 줄 `      incomplete: ...`(`render_row`). 머리의 "summary A" 줄 다음에 `summary incomplete (...)` 한 줄. 경로 A 의 키: 잘린 이미지의 키는 unresolved(찾은 값이 있어도). 경로 C 의 스레드 집합: 잘린 이미지의 entry 가 들면 완전하지 않음. 머리의 이미지 spawn 줄이 잘림을 적는다. 규칙은 `docs/static_facts.md` §5m.
  - `tests/static/fixtures/relincomplete.c`(새, 사례 이름 없음), `tests/static/test_static_reasons.cpp`(새), `tests/static/data/reason_vocabulary.txt`(새: JR 3.5 표의 사유 22, 위 셋 포함), `tests/static/static_golden.cpp`(사례 둘), `tests/static/CMakeLists.txt`(fixture·golden 둘·시험 하나, 머리 주석), `tests/static/test_static_cli.cpp`(개정 문자열 확인 5 → 6, 새 블록 하나).
  - `docs/static_facts.md`: §5m(새 절), §5j 소비자 문단에 한 문장, §5e 의 가리킴 한 문장.
  - `env/redesign/one_build.sh`: `ONE_BUILD_EXTRACT_EXTRA`(extract 단계에만 붙는 인자, `args.txt` 에 적힘; 기본 빈 값 = 드라이버 인자 그대로). 어떤 도구의 정체성에도 들지 않는다.
  - 하지 않는 것: facts schema(`include/` 그대로), 추출(`ast_*`, `argflow`, `finalize`, `serialize` 그대로), `cmake/`, 최상위 `CMakeLists.txt`, `runtime/`, `tests/lint/`, 2a 드라이버와 그 허용표.
- **도구 해시와 정체성**(`cmake/KzSourceIdentity.cmake` 의 규칙):
  - kz-static: 파일 sha256, `--identity` 의 `tool_sha256`, `tool_source` 가 바뀐다(`lib/llvm` 이 바뀜). `core_source` `f6e3c7b4…` 는 그대로다 [I].
  - kz `931dd39c…`, kz-dump `05623054…`, kz-rt-dump `4390d4f1…`, kz-phi `7d34e6e2…`, kz-instr-clang `28db4017…`, kz_lint `b0f0d6a5…`: 다시 링크되지 않아야 한다(빌드 전 sha256 은 `pre_mc.txt`). kz, kz-phi, kz-dump 의 정체성 정의는 그대로다.
  - 다시 링크되는 시험 프로그램: `kz-llvm` 을 링크하는 여덟, `test_static_cli`(소스), `static_golden`(소스), 새 `test_static_reasons`.
  - `CMakeCache.txt` `bc2f43b4…` 는 그대로다 [I]. flags.make 목록의 해시는 바뀐다(kz-static 의 `flags.make` 정의, 새 시험 `test_static_reasons` 의 `flags.make`).
- **도구 보존**: 이 빌드는 kz-static `a763e255` 를 제자리에서 덮는다. 그 도구의 남은 출력은 사용 지점 키 목록 다섯(`/home/user/work/redesign/obs1/relrows_check/<빌드>/keys.txt`)이다. 새 도구가 그것을 셋째 줄(규칙 개정) 밖에서 바이트 동일하게 다시 만들어야 "대체됨"이다. 그 비교는 덮은 뒤에야 할 수 있으므로, 빌드 전에 `a763e255` 를 압축 사본으로 둔다(`/home/user/work/preserve/tools-20261116-b/`, `xz -9`, 왕복 확인). 등록부에는 사본 행을 더한다.
- **golden**:
  - 바뀌는 넷: `relations`, `relations_np`, `relpaths_np`, `relsound_np`. 바뀌는 것은 셋뿐이다: `rules relations rules revision 5 (2026-10-30)` → `rules relations rules revision 6 (2026-11-16)`; "summary A" 줄 다음의 `summary incomplete (` 줄 하나; 행마다 `      layers: ` 줄 다음의 `      incomplete: ` 줄 하나. 그 셋을 되돌린 글은 옛 golden 과 바이트 동일해야 한다(`compare_declared.py relations`). 기본 예산이라 표시 잎이 없으므로 [I] 다른 줄은 바뀌지 않는다.
  - 새 둘: `relincomplete_np.golden`, `relincomplete_cut_np.golden`(`KZ_GOLDEN_UPDATE=1` 로 만들고 읽어 본다).
  - 그 밖의 golden(`tests/static/data` 의 나머지, `tests/golden/data`, `tests/phi/data`)은 바뀌지 않는다. 확인: relations 글(`kz-relations 1`)이 든 golden 은 위 넷뿐이다(`grep -l`, 빌드 전 [M]). 그 밖의 golden 은 relations 를 그리지 않고 추출 코드는 바뀌지 않는다.
- **relations 다섯 빌드 재실행**(새 도구, T8CF 의 NP facts 그대로, facts 는 다시 뽑지 않음): `kz-static relations <T8CF>/<b>/NP/facts.kz -o relations.txt --keys keys.txt > relations.log 2>&1`. 다섯 빌드 모두:
  - `relations.txt`: 위 golden 과 같은 세 변화뿐이다. 그 셋을 되돌리면 T8CF(`936cd6c8…`, `00b57eb2…`, `fc8ea8da…`, `1d4af508…`, `34ac45f1…`)와 바이트 동일해야 한다. 줄 수는 T8CF + 1 + 행 수다(c1_pre_tx 7162 + 1 + 431 = 7594, c1_post_tx 7594, r2_core 4966 + 1 + 338 = 5305, c5_641 3083 + 1 + 178 = 3262, c5_642 3037 + 1 + 175 = 3213). 둘째 줄 `facts <hash>` 는 같다.
  - T8CF 의 다섯 빌드에는 표시 잎도 `image-truncated` 플래그도 없다(`plan/step2a_results_20261030.md` §16 [F]). 그래서 [I] 잘린 이미지 규칙은 이 다섯 빌드의 어느 행도 바꾸지 않는다. 바꾸면 그 실행에서 멈추고 까닭을 적는다.
  - 키 다섯 빌드 모두 `key unresolved` 인 A 행(72/72/32/64/64 [M: T8CF 의 행 머리를 셈])은 하나도 빠짐없이 `key_unresolved(...)`(creator key 아닌 것)를 가져야 한다(`compare_declared.py` 가 센다).
  - `keys.txt`: 셋째 줄(규칙 개정)만 바뀌고 나머지 줄은 앞 단계의 키 목록과 같다(`compare_declared.py keys`). 그래서 모든 행의 사용 지점 키가 앞 단계와 같다.
  - `relations.log` 는 빈 파일이다.
  - 새 출력: 다섯 빌드의 `relations.txt` 와 `keys.txt` 는 남긴다(합 약 8.6 MB [I]; 계획 생성기의 다음 단계가 같은 수를 다시 만들 때까지). 
- **측정(판정이 아님)**: 빌드마다 사유별·하위 사유별 행 수(요약 줄), facts 전체의 사유. 
- **민감도 실행**: c1_pre_tx 를 `--image-budget 3` 으로 다시 추출한다(`env/redesign/one_build.sh`, `ONE_BUILD_EXTRACT_EXTRA="--image-budget 3"`, 새 디렉터리 `/home/user/work/redesign/obs1/relincomplete_sens/`). 예산의 값 3, 이유: fixture 의 값이다. fixture 의 네 잎짜리 두 표를 모두 자르는 가장 큰 예산이다. c1_pre_tx 에서는 잎이 셋보다 많은 모든 정적 이미지를 자르므로 잘림이 이미지를 읽는 행에 닿는지 볼 수 있다. 기록하는 것: BASIS 의 두 이미지 계수, 행 수와 사유별 수, 기본 예산(위 재실행)과 키가 같은 행·다른 행의 수와 그 키들. 결과는 측정이다. 기대 값을 두지 않는다. 출력은 다음 단계의 도구가 같은 수를 다시 만들 때까지 남긴다.
- **시험**: 전체 ctest 239 → 243(새: `static_fixture_compile_relincomplete`, `static_golden_relincomplete_np`, `static_golden_relincomplete_cut_np`, `test_static_reasons`). 라벨: static +4, golden +2, 그 밖은 그대로. 실패 0, 건너뜀 1(`test_probe_anchor_reached`, tracefs 없음). `test_static_cli` 의 확인 수는 는다(뒤에 적음). lint 6/6. 단위 점검 `env/tools/tests/run_unit.sh`, `env/redesign/step2a_tests_unit.sh` 는 앞과 같은 수로 통과해야 한다. 어휘 자료와 JR 3.5 표가 같은지: `vocab_check.py`(저장소 기록, `python3 -I`).
- **누적 바이트 동일성**: 빌드 뒤 `env/tools/ledger_check.sh` 전체와 `tools_check.sh --deep`. 모두 같아야 한다. 앞 단계의 키 목록(장부 행 `OBS_relrows_keys`)도 그 안에 있다.
- **공간**: 시작 전 여유 ≥ 하한 314,572,800 + 필요량. 필요량 = 앞 단계에서 잰 kz-static 제자리 빌드 dip 93,020,160 + 새 출력 10 MB + 한 시간 흐름 13 MB = 116,020,160 B(감시 문턱 430,592,960). 감시(`env/tools/space_guard.sh --kind kz-static`, 2초마다)가 하한을 넘으면 멈추고 보고한다. 재실행(`--kind relations`), 민감도 추출(`--kind extract`), 도구 사본(`--kind tool-copy`)도 감시 아래 돌린다.
- **비교 스크립트**(빌드 전에 이 선언과 함께 커밋): `records/obs1-20261116-relincomplete/compare_declared.py`(위 세 변화를 되돌려 바이트 비교, 행마다 한 줄, key unresolved 인 A 행의 사유; 키 목록은 셋째 줄만), `vocab_check.py`(어휘 자료 = JR 3.5 표). 둘 다 `python3 -I`.

### 3.1 바뀐 것 [F]

- 커밋 `750cc43`(선언·코드·시험·문서·비교 스크립트·도구 사본 기록을 함께, 빌드 전). 규모(`git diff --stat 357bdc0 750cc43 -- kairos`, 11 파일 +753/−20): `lib/llvm/relations.cpp` +263 줄 남짓, `relations.hpp` +18, `rel_facts.cpp` +27, `rel_facts.hpp` +18, `tests/static/` 의 여섯 파일(셋은 새: `fixtures/relincomplete.c` 150줄, `test_static_reasons.cpp` 127줄, `data/reason_vocabulary.txt` 26줄), `docs/static_facts.md` +25.
- 잘린 이미지: `RelFacts::image_cut`, `image_value_why(…, cut)`, `image_entries` 의 `image_cut_`, 개정 5 덮개 검사에서 표시 잎 빼기. 경로 A `key_value` 는 잘린 이미지에서 `unresolved`(KeyValue `why`, `cut`). ChildSet `cut`.
- 미완 사유: 표 `relations_incomplete_reasons()` 36 항목(사유 열: `no_run` 1, `actor_not_extracted` 4, `key_unresolved` 9, `truncated` 6, `writer_set_incomplete` 3, `absent_result_unevaluated` 3, `fixed_proof_open` 7, `link_unlinked` 2, `reading_options_differ` 1). `Rel::inc`, `global_reasons`, `inc_writers`, `finish`. 줄은 `render_row`, 요약은 `header`.
- 판정 규칙 3.5: 세 줄, 하위 사유 표기, need 상태 표(§3.0).
- `env/redesign/one_build.sh`: `ONE_BUILD_EXTRACT_EXTRA`.

### 3.2 빌드 [M]

- 빌드 전(`pre_mc.txt`, 06:12:46Z): 도구 일곱이 앞 단계 `post_mc.txt` 와 같다. HEAD `750cc43`, `kairos` 미커밋 0줄.
- 빌드 전에 kz-static `a763e255` 의 압축 사본을 만들었다(`/home/user/work/preserve/tools-20261116-b/kz-static.a763e255.xz`, 39,612,812 B; `xz -t`, 스트림 sha256, `cmp` 모두 같음; `tool_copy/ROUNDTRIP.txt`).
- MC 제자리 `make -j4 all`(06:12:56Z–06:15:16Z), 공간 감시 아래. rc 0, 첫 번에 됐다. CMake 가 다시 구성했다. 링크 13줄: `libkz-llvm.a`, kz-static, `kz-llvm` 을 링크하는 시험 여덟, `test_static_cli`, 새 `test_static_reasons`, `static_golden`. 빌드 시작 뒤 정체성 경로의 파일은 바뀌지 않았다(`find -newer` 0).

| 도구 | 전 | 뒤 |
|---|---|---|
| kz-static | `a763e2550ad6902f102d5a1c2d310c4a658a36c2087b1b6f6717d3d4b1193ed7` | `5d4a1d7f4023b20cbd6caf797edcdbd0e54d8efb329ace358c07002f27088749` (178,301,376 → 178,474,384 B) |
| kz-static tool_sha256 | `90046f5d…` | `c162e7f65c3235a3d0e92c867654fcca014a1097c075d46088d4c4cd7c6d3297` |
| kz-static tool_source | `b9a4e443…` | `7003f7ed3e7d0cba04ae150b805c2cf33b0cd83e57c5584c2a7ebefc5c0db342` |
| core_source | `f6e3c7b4…` | 같음 |
| kz / kz-dump / kz-rt-dump / kz-phi / kz_lint / kz-instr-clang | `931dd39c` / `05623054` / `4390d4f1` / `7d34e6e2` / `b0f0d6a5` / `28db4017` | 모두 같음(다시 링크되지 않음) |
| kz, kz-phi, kz-dump 의 정체성 정의 | `caf58235…`, `7fc3ed73…`, `cc51ff35…` | 같음 |
| `CMakeCache.txt` | `bc2f43b4…` | 같음 |
| flags.make 목록 해시 | `b354fbbf…` | `1d3b5bba…` (kz-static 의 `flags.make`, 새 `test_static_reasons`) |

- 선언대로다.

### 3.3 시험 [M]

- golden 갱신 전 전체 ctest(`ctest_first.log`): 243개 가운데 실패 7. 선언한 것뿐이다: 바뀌는 golden 넷, 아직 파일이 없는 새 golden 둘, 그 golden 을 읽는 `test_static_reasons`. `test_static_cli` 는 이때 이미 통과했다.
- `KZ_GOLDEN_UPDATE=1` 로 여섯 golden 을 쓰고 읽었다. 바뀐 넷은 `compare_declared.py relations` 로 옛 golden 과 비교했다: 넷 모두 SAME(행 3/5/7/38, 행마다 `incomplete:` 한 줄; 셋을 되돌린 글은 옛 golden 과 바이트 동일)(`golden_compare.txt`). 새 둘(`relincomplete_np`, `relincomplete_cut_np`)의 차이는 잘림이 닿는 자리뿐이다: GLOBAL_IMAGES 행의 `image-truncated` 와 표시 `name_defs[...]`, `task_defs[...]`; seek_known 의 키가 unresolved 가 되고 "key constants differ" 제외가 없어져 creator 와 짝이 됨; entry 가 `green_main red_main` 둘; blue_main 을 지나는 C 행이 undecidable.
- 그 밖의 golden(`tests/static/data` 의 나머지, `tests/golden/data`, `tests/phi/data`)은 바뀌지 않았다(`git status` 에 없음).
- 갱신 뒤 전체 ctest(06:16:14Z–06:16:34Z, `ctest_full.log`): 243개, 실패 0, 건너뜀 1(`test_probe_anchor_reached`, tracefs 없음; `skip_reason.log`). 라벨: golden 60, static 49, legacy-case 51, unit 84, runtime 66, lint 6(앞 단계에서 golden +2, static +4).
- `test_static_cli` 확인 837개(앞 798, +39: 미완 줄과 잘린 이미지 블록). `test_static_reasons` 575개. `lint_selftest` 102 경우 실패 0, lint 6/6. 빌드 전 kz_lint 규칙 다섯도 깨끗했다(`commands.txt` §0).
- 단위 점검: `env/tools/tests/run_unit.sh` 8/8, `env/redesign/step2a_tests_unit.sh` 7/7.
- 어휘 자료와 판정 규칙 3.5 표: `vocab_check.py` SAME(22 사유, 같은 차례).
- 합성 시험의 내용(`test_static_cli`, fixture `relincomplete.c`): 기본 예산에서 seek_known 의 키는 known(네 이름)이고 미완 줄에 `key_unresolved` 도 `truncated` 도 없다. seek_opaque 는 `key_unresolved(pointer field without points-to)` 와 `fixed_proof_open(no static order between the actors)`. red_main 의 "lime" 은 static-possible. C 행은 union 의 다른 멤버 때문에 `actor_not_extracted(child threads not all resolved)`. `--image-budget 3` 에서 seek_known 의 키는 unresolved(세 이름 뒤 `unresolved (image, not taken as the run-time value: the static image was cut at the image budget (marker leaves name_defs[...]): …)`), "key constants differ" 가 없고, 미완 줄에 `key_unresolved(static image truncated)`, `truncated(static image)`, `actor_not_extracted(spawn entries from a cut static image)`. 표시 잎은 "non-constant leaf covering" 수에 들지 않는다. blue_main 의 C 행은 `fixed_proof_open(child threads not all resolved)`.

### 3.4 바이트 동일성 [M]

- relations 다섯 빌드(새 도구, T8CF 의 NP facts 그대로; 06:17:10Z–06:17:12Z, 새 디렉터리 `/home/user/work/redesign/obs1/relincomplete_check`): 다섯 모두 선언대로다(`byte_identity.txt`).

| 빌드 | 행(미완 줄) | key unresolved 인 A 행 중 key_unresolved 를 가진 것 | 줄 수 | relations.txt | keys.txt |
|---|---|---|---|---|---|
| c1_pre_tx | 431 (431) | 72/72 | 7594 | `7c7396d2…` | 셋째 줄만 다름 |
| c1_post_tx | 431 (431) | 72/72 | 7594 | `0a3bf6d9…` | 셋째 줄만 다름 |
| r2_core | 338 (338) | 32/32 | 5305 | `3a6fcf81…` | 셋째 줄만 다름 |
| c5_641 | 178 (178) | 64/64 | 3262 | `b62fd1f2…` | 셋째 줄만 다름 |
| c5_642 | 175 (175) | 64/64 | 3213 | `996d32bd…` | 셋째 줄만 다름 |

- 세 변화를 되돌린 `relations.txt` 는 T8CF(`936cd6c8…`, `00b57eb2…`, `fc8ea8da…`, `1d4af508…`, `34ac45f1…`)와 바이트 동일하다. 둘째 줄 `facts` 는 같다. `relations.log` 는 빈 파일로 같다. 그래서 잘린 이미지 규칙은 다섯 빌드의 어느 행도 바꾸지 않았다(표시 잎이 없다).
- 사용 지점 키는 모든 행에서 앞 단계와 같다(키 목록의 셋째 줄 밖 모든 줄이 같음).
- 다시 돌리지 않은 것(새 도구로는 확인하지 않음): basis, extract(민감도 실행의 c1_pre_tx 하나 밖), NP2, identity, count, twice, 드라이버 비교.
- 남긴 것: 다섯 빌드의 `relations.txt`·`keys.txt`·`relations.log`(합 6,444,906 B; `outputs.sha256`; 장부 행 `OBS_relincomplete`).
- 도구 보존: 새 도구가 `a763e255` 의 남은 출력(키 목록 다섯)을 선언한 한 줄(규칙 개정) 밖에서 바이트 동일하게 다시 만들었으므로 등록부에 "대체됨"(사본 있음)으로 적었다. `5d4a1d7f` 는 `in-place` 행으로 더했다.
- 누적 목록: `ledger_check.sh` 33행, 실패 0(새 행 셋: 도구 사본, 다섯 빌드 출력, 민감도 출력). `tools_check.sh --deep` 28행, 실패 0. 바깥 이름 점검(`--tree`)은 두 출력 디렉터리와 사본 디렉터리에서 깨끗하다.

### 3.5 측정: 사유별 행 수 [M]

다섯 빌드의 요약 줄(`summary incomplete`)에서. 수는 그 사유를 가진 행이다(한 행이 여러 사유를 가진다). facts 전체의 사유는 다섯 빌드 모두 `actor_not_extracted(spawn call with no resolved entry)` 하나다(풀리지 않은 spawn 호출이 있다, 머리 줄).

| 사유 (하위 사유) | c1_pre_tx | c1_post_tx | r2_core | c5_641 | c5_642 |
|---|---|---|---|---|---|
| 행 | 431 | 431 | 338 | 178 | 175 |
| `no_run` | 431 | 431 | 338 | 178 | 175 |
| `actor_not_extracted` (spawn call with no resolved entry) | 431 | 431 | 338 | 178 | 175 |
| ″ (no root reaches the use) | 15 | 15 | 16 | 19 | 18 |
| ″ (child threads not all resolved) | 89 | 89 | 78 | 0 | 0 |
| `key_unresolved` 행 | 76 | 76 | 36 | 64 | 64 |
| ″ (pointer field without points-to) | 2 | 2 | 0 | 0 | 0 |
| ″ (field read without a static image) | 33 | 33 | 10 | 31 | 31 |
| ″ (argument is no literal, constant or field) | 37 | 37 | 22 | 33 | 33 |
| ″ (creator key) | 10 | 10 | 7 | 2 | 2 |
| `writer_set_incomplete` 행 | 318 | 318 | 272 | 146 | 143 |
| ″ (FIELD_STATUS not complete) | 318 | 318 | 245 | 132 | 129 |
| ″ (per-field completeness rule is a premise) | 0 | 0 | 55 | 17 | 16 |
| `absent_result_unevaluated` 행 | 68 | 68 | 35 | 35 | 33 |
| ″ (no failure return of the search) | 0 | 0 | 0 | 5 | 5 |
| ″ (callee check not evaluated) | 31 | 31 | 18 | 26 | 24 |
| ″ (reader's returns not evaluated) | 37 | 37 | 17 | 4 | 4 |
| `fixed_proof_open` 행 | 227 | 227 | 206 | 89 | 97 |
| ″ (no static order between the actors) | 157 | 157 | 120 | 75 | 73 |
| ″ (creation order without an order of the use) | 0 | 0 | 55 | 0 | 0 |
| ″ (no creation order from a static image) | 49 | 49 | 0 | 0 | 0 |
| ″ (lock sets do not decide the order) | 0 | 0 | 0 | 8 | 10 |
| ″ (indirect call with unresolved targets) | 20 | 20 | 29 | 0 | 0 |
| ″ (child threads not all resolved) | 1 | 1 | 2 | 0 | 0 |
| ″ (proof rests on an assumption) | 0 | 0 | 0 | 6 | 14 |
| `link_unlinked` (identity of the reader and the created thread not decided) | 134 | 134 | 115 | 39 | 38 |
| ″ (self-reported identity) | 0 | 0 | 0 | 32 | 32 |
| `reading_options_differ` (basis deviation of a unit of the row) | 0 | 0 | 0 | 159 | 156 |
| `truncated` | 0 | 0 | 0 | 0 | 0 |

- c1_pre_tx 의 A21(사용 지점 키 `17975444d83f6589`)의 줄: `no_run(no run record read); actor_not_extracted(spawn call with no resolved entry); key_unresolved(pointer field without points-to); fixed_proof_open(no static order between the actors)`. c1_post_tx 의 같은 키의 행도 같다.

### 3.6 민감도 실행: c1_pre_tx, `--image-budget 3` [M]

- 예산 기록: 값 3. 이유: fixture 의 값(fixture 의 네 잎짜리 두 표를 모두 자르는 가장 큰 예산)이고, c1_pre_tx 에서는 잎이 셋보다 많은 모든 정적 이미지를 자른다. 기본 값 4096 은 그대로다(이 실행만의 값). 잘림: BASIS `image_budget_exhausted` 34 객체, `image_leaves_omitted` 46 표시. 다른 걷기 상한은 0.
- 실행: `ONE_BUILD_EXTRACT_EXTRA="--image-budget 3" one_build.sh c1_pre_tx`(06:17:32Z–06:17:58Z, PASS; `args.txt` 의 extract 인자 `--targets-rule R-T1,R-T2 --image-budget 3`). 새 디렉터리 `/home/user/work/redesign/obs1/relincomplete_sens`. 같은 facts 에 `--keys` 를 더한 relations 도 돌렸다(글은 같은 바이트).
- 결과(`sens_detail.txt`):
  - 행 305(A 127, B 135, C 43). 기본 예산은 431(A 127, B 170, C 134). 305 행의 키는 모두 기본 예산의 키다. 기본 예산에만 있는 행 126(B 35, C 91).
  - 까닭: `cfe_es_start.c:767:34` 의 spawn(`CFE_ES_StartAppTask`)이 `CFE_ES_ObjectTable` 의 이미지로 풀리지 않는다. 이미지가 `MainTaskPtr` 잎 앞에서 잘렸다(표시 `CFE_ES_ObjectTable[0].ObjectSize...`, `CFE_ES_ObjectTable[...]`). 그래서 table-image-entry root 5 → 0 이고, 머리의 "spawn calls with no resolved entry" 가 2 → 3 이며 그 이유가 잘림을 적는다. 그 root 들이 닿던 B·C 행이 빠진다. 빠진 행은 남은 행의 미완 줄에 `actor_not_extracted(spawn call with no resolved entry)` 로 드러난다(모든 행). 행 자체는 남지 않는다 [I: 이 규칙에서 행이 없는 것은 그 행위자가 root 가 아니기 때문이다].
  - 잘림이 행에 닿은 것: `key_unresolved(static image truncated)` 3 행(A20, A40, A41: BP 의 `initialize_throttling` 이 `BP_Throttles` 이미지에서 읽는 키), `truncated(static image)` 4 행(그 셋과 A21). A21 은 BP creator 의 키(`BP_Throttles` 이미지)가 잘려 `key_unresolved(creator key)` 와 `truncated(static image)` 를 더 가진다. 기본 예산에서 creator 의 키는 known(`"BP0_tsem"`, `"BP1_tsem"`)이다.
  - 같은 키의 305 행 가운데 첫 줄이 같은 것 296, 미완 줄이 같은 것 244.
- 출력은 남겼다(12,955,197 B; `sens_outputs.sha256`; 장부 행 `OBS_relincomplete_sens`). 계획 생성기의 다음 단계가 같은 수를 다시 만든 뒤 지운다.

### 3.7 공간 [M]

| 시점 (UTC) | 여유 (df -B1 /) | 일 |
|---|---|---|
| 이 단계 처음 | 4,367,933,440 | 시작(앞 단계 끝의 515,837,952 에서 늘었다: 이 단계 밖의 지우기, 커밋 `47c0cfa` 의 기록) |
| 06:08–06:10 | 4,364,591,104 → 4,324,966,400 | 도구 사본(dip 39,632,896) |
| 06:12:56 | 4,323,872,768 | 빌드 시작(감시 문턱 430,592,960) |
| 06:15:14 | 4,149,112,832 | 빌드의 가장 낮은 점(dip 174,759,936) |
| 06:15:16 | 4,166,381,568 | 빌드 끝(변화 −157,491,200) |
| 06:17:12 | 4,158,799,872 | 다섯 빌드 재실행 끝(dip 6,488,064) |
| 06:17:58 | 4,146,688,000 | 민감도 실행 끝(extract dip 11,018,240) |
| 06:19:09 | 4,145,606,656 | 기록을 쓴 뒤 |

- 하한을 넘지 않았다. 빌드 dip 174.8 MB 는 선언한 필요량(116.0 MB)보다 크다. 새 시험 프로그램 `test_static_reasons`(154,547,088 B, `kz-llvm` 을 링크)가 더해졌기 때문이다. 순변화 −157.5 MB 의 대부분도 그것이다 [I: 파일별로 재지 않음]. 다음 kz-static 빌드의 필요량은 이 dip 으로 잡는다.
- 지운 것은 없다. scratchpad 의 작업 파일(fixture 시도, 합성 비교)만 scratchpad 에 남았다.

### 3.8 계획과 다르게 한 것

- 잘린 이미지는 객체 단위로 본다. 읽는 원소·멤버를 표시 잎이 덮는지는 가리지 않는다(보수 쪽). 그래서 읽는 원소가 잘리지 않은 부분에 있어도 키는 unresolved 다.
- 잘린 이미지에서 찾은 spawn entry 는 풀리지 않은 spawn 으로 돌리지 않고 entry 로 남긴다. 그 spawn 의 스레드를 완전하지 않음으로 두고 모든 행에 `actor_not_extracted(spawn entries from a cut static image)` 를 적는다. 찾은 잎이 하나도 없으면 전처럼 풀리지 않은 spawn 이고 이유가 잘림을 적는다(민감도 실행의 ES 표가 이 경우다). 잘린 이미지 규칙은 `image_value_why` 의 첫 이유로도 들어갔지만 spawn entry 에는 그 인자를 넘기지 않는다.
- 하위 사유는 relations 가 facts 에서 가를 수 있는 것만 썼다. `key_unresolved(runtime table buffer)` 는 쓰지 않는다: 지금 facts 에서 그 포인터 필드가 실행 중 표 버퍼로 쓰인다는 것을 일반 규칙으로 이끌어 낼 수 없다. A21 은 `key_unresolved(pointer field without points-to)` 다. 판정 규칙 3.5 에는 그 하위 사유를 표기의 예로 남겼다.
- relations 는 `handling_effect_unknown`, `caller_dependence_unknown` 을 내지 않는다(handling 줄의 판정은 계획 생성기 몫). 더한 세 사유 가운데 `reading_options_differ` 만 relations 가 낸다. 나머지 둘은 실행·개입의 사유다.
- 민감도 예산의 이유를 "fixture 의 두 표를 모두 자르는 가장 큰 예산"으로 적었다(계획의 글 "가장 작은 예산"은 이 fixture 에 맞지 않는다: 1 도 자른다).
- 이 빌드가 덮은 `a763e255` 는 빌드 전에 압축 사본을 두었다(그 도구의 출력을 다시 만들 수 있는지는 덮은 뒤에야 알 수 있어서).
- 빌드 dip 이 선언한 필요량을 넘었다(§3.7). 하한과는 멀었다.

## 4. 관측 계획 생성기, 프레임워크 계층 자료와 그 lint, 계획 (kz-static·kz_lint 빌드 셋)

기록: `../records/obs1-20261116-plangen/`. 출력: 저장소 밖 `/home/user/work/redesign/obs1/plangen/`(계획 다섯 빌드), `/home/user/work/redesign/obs1/plangen_relcheck/`(relations 재실행), `/home/user/work/redesign/obs1/plangen_bref/`(c1_pre_tx 다시 뽑기), 모두 새 디렉터리. T8CF(`/home/user/work/redesign/step2a/tests_8c_fix`)와 앞선 출력 디렉터리에는 쓰지 않는다. cFS를 부팅하지 않는다. 파생 빌드를 하지 않는다. push하지 않는다.

### 4.0 선언 (빌드 전에 적고 커밋함) [D]

빌드 전에 적었다. 아래 밖의 차이는 결함이다. 결함이면 다음으로 가기 전에 고친다.

- **코드**(설계, `kairos/`):
  - `lib/llvm/plan_gen.hpp`, `plan_gen.cpp`(새): `kz-static plan` 의 생성기. relations 의 행을 같은 프로세스 안에서 `relations_rows`(공유 적재기 `RelFacts`)로 계산하고, 방법의 관측 집합(모든 행)을 방법의 우선순위 차례로 예산 아래 계획한다. 우선순위는 방법 문서 첫 단계 규칙의 이식(행 글의 표시, 순서 등급, 도달 표시, 처분 D01–D13; `plan/ovm_rev1_20261116/S1_RETALLY.md` 2.1–2.4)이다. 층 1 = 처분 D07, 층 2 = 그 밖의 순서 등급 RELOC 행, 층 3 = 나머지. 관측 규칙(use-call, key-text, creator-call, handling-chain, writer, spawn-call, writer-after-spawn, actor-root, actor-create, actor-start, actor-owner, stop-event), 출력(`kzs-sites 3` manifest, sidecar, module 표, 점검할 실행 파일 목록), 사유 표 `plan_reasons()`(9 항목, 모두 판정 규칙 3.5 어휘의 사유와 하위 사유). 규칙과 형식은 `docs/plan_manifest.md`(새). 사례 이름, 필드·callee 이름을 받지 않는다. 생성기는 명령줄에 이름이 있는 파일(facts, 그 basis, 프레임워크 계층 자료)만 연다.
  - `tools/kz-static/cmd_plan.cpp`(새), `main.cpp`, `commands.hpp`, `CMakeLists.txt`: 명령 `kz-static plan <facts> [--basis B] --framework F --select observation-set --order method-priority --budget SPEC --budget-reason T [--budget-disclosure T] -o M --sidecar S --modules-out MT --exes-out EX`. 생성기는 manifest 파서(kzrt-manifest)를 링크하지 않는다(그 정체성이 `runtime/` 으로 넓어지지 않게).
  - `lib/llvm/CMakeLists.txt`: `plan_gen.cpp` 와 머리 주석 두 줄.
  - `semantics/framework_api_roles.txt`(새 디렉터리, 새 파일): 프레임워크 계층 자료. 다섯 빌드에 있는 cFE 판 넷(묶음 1df262a 의 cFE 6.7.99, cFE 7.0.1, 6.4.1, 6.4.2)마다 run-loop 와 app-exit 역할의 API(`CFE_ES_RunLoop`, `CFE_ES_ExitApp`) 하나씩, 정의 file:line 과 sha256, 공개 헤더의 선언 file:line 과 sha256. 출처: 헤더의 기계적 훑기(`env/tools/framework_scan.py scan`, 출력 `records/obs1-20261116-plangen/framework_scan.txt`)와 점검(`framework_scan.py check`: 여덟 항목 모두 훑기 출력에 있고 정의와 sha256 이 맞으며 판마다 역할마다 하나, `framework_check.txt` PASS, 빌드 전 [M]).
  - `tests/lint/kz_lint.cpp`, `tests/lint/data/case_identifiers.txt`, `tests/lint/data/framework_identifiers.txt`(새), `tests/lint/CMakeLists.txt`: 사례 식별자 목록을 둘로 나눈다. 사례(R2, C1, C2, C3, C5, 이슈 번호, 사례 커밋, BP0_tsem, BP1_tsem, sem_name, SBNSubPipe, MainTaskSyncDelay)와 프레임워크 이름(CFE_SB_Global, ILLEGAL_APP_ID, `CFE_*`, `OS_*`, cfe, evs, osal, psp, cfs, c_fs, GetAppID, AppId). 규칙 `case-ids` 는 두 목록을 지금 범위에 그대로 쓴다(결과가 같아야 한다). 새 규칙 `case-ids-data` 는 `semantics/` 의 모든 글 파일을 사례 목록만으로 본다. selftest 경우 7개 더(102 → 109), ctest `lint_case-ids-data` 하나 더. 빌드 전에 scratchpad 에서 따로 컴파일한 kz_lint 로 selftest 109 실패 0, `case-ids` 깨끗, `case-ids-data` 깨끗 [M].
  - `tests/static/fixtures/plangen.c`(새, 사례 이름 없음): 앱 둘이 spawn 연산을 거쳐 생성 인자와 함께 시작됨, 이름 등록 표(검색 루프의 strcmp)와 조회 API(이름의 non-null 검사 뒤 검색)·생성 API(non-null 검사와 길이 16 의 memchr), 앱 하나의 init 에서 조회하고 실패가 run status 의 저장을 거쳐 app-exit 호출에 닿음, 다른 앱의 생성, 한 줄의 두 조회 호출, 루프 안의 조회, spawn 뒤에 owner 매개변수 저장, `-DPLANGEN_SECOND_CALLER` 로 나중에 더한 셋째 호출자. `tests/static/data/plangen_framework.txt`(새): 그 fixture 의 run loop 와 app exit.
  - `tests/static/test_static_plan.cpp`(새, kz-llvm·kzrt-manifest·kz-testsupport 링크; 시험 프로그램은 어떤 도구의 정체성에도 들지 않는다): 두 번 생성의 바이트 동일, 한 바이트 고친 사본의 불일치, manifest 를 런타임의 호스트 파서로 읽음(`kzs-sites 3`, identity·control·bind 문 없음, 모든 site 에 규칙 이름이 든 label 과 ref), sidecar sha256 이 manifest 머리·module 표 첫 줄·실행 파일 목록에 있음, 관측 규칙의 site 들(위 fixture 의 각 경우), 거부(항목이 아닌 자료 줄, 예산 이유 없음, 모르는 예산, basis 가 아닌 `--basis`; 출력 파일 없음), 우선순위 이식(fixture 행과 여기서 쓴 행 여섯: T?/RELOC/D11, 0/NOPAIR/D05, J/INDEPENDENT/D08, FIXED/D02, F/REACH-STATED/D10, ENF/D01), 둘째 호출자(첫 계획의 site 키가 모두 둘째 계획에 있고 새 조회의 use-call site 가 있음), golden 글 `data/plangen_np.plan.golden`(새; sha256 값과 경로는 가림).
  - `tests/static/test_static_reasons.cpp`: 생성기의 사유 표도 어휘 자료와 비교한다.
  - `tests/static/CMakeLists.txt`: fixture 컴파일 시험 하나(`static_fixture_compile_plangen`), `test_static_plan`, 머리 주석.
  - `docs/plan_manifest.md`(새), `docs/static_facts.md` §5n(새 절)과 §5e 의 가리킴 한 문장.
  - 저장소 스크립트(어떤 도구의 정체성에도 들지 않음): `env/tools/framework_scan.py`(훑기·점검, `python3 -I`), `env/tools/plan_regen.sh`(재생성 점검: 기록된 인자로 다시 생성해 manifest·sidecar 는 바이트로, module 표·실행 파일 목록은 첫 줄의 sidecar 경로 밖에서 바이트로 비교).
  - 하지 않는 것: relations 의 규칙·글(개정 6 그대로), facts schema(`include/`), 추출, `cmake/`, 최상위 `CMakeLists.txt`, `runtime/`, `instrument/`, 2a 드라이버와 그 허용표.
- **도구 해시와 정체성**(`cmake/KzSourceIdentity.cmake` 의 규칙):
  - kz-static: 파일 sha256, `--identity` 의 `tool_sha256`, `tool_source` 가 바뀐다(`lib/llvm`, `tools/kz-static` 이 바뀜). `core_source` `f6e3c7b4…` 는 그대로다 [I].
  - kz_lint: 파일 sha256 이 바뀐다(소스가 바뀜; 정체성 정의는 없음).
  - kz `931dd39c…`, kz-dump `05623054…`, kz-rt-dump `4390d4f1…`, kz-phi `7d34e6e2…`, kz-instr-clang `28db4017…`: 다시 링크되지 않아야 한다(빌드 전 sha256 은 `pre_mc.txt`). kz, kz-phi, kz-dump 의 정체성 정의는 그대로다. kzrt-manifest 는 다시 컴파일되지 않는다(`runtime/` 그대로).
  - 다시 링크되는 시험 프로그램: `kz-llvm` 을 링크하는 여덟, `test_static_reasons`(소스), 새 `test_static_plan`, `static_golden`(kz-static 에 대한 의존). `test_static_cli` 는 다시 링크되지 않아도 된다(다시 링크되어도 결함이 아니다).
  - `CMakeCache.txt` `bc2f43b4…` 는 그대로다 [I]. flags.make 목록의 해시는 바뀐다(kz-static 의 정의, 새 시험의 `flags.make`).
- **도구 보존**: 이 빌드는 kz-static `5d4a1d7f` 와 kz_lint `b0f0d6a5` 를 제자리에서 덮는다. kz_lint `b0f0d6a5` 는 사본이 있다(`/home/user/work/preserve/tools-20261116/`). kz-static `5d4a1d7f` 의 사본은 빌드 전에 만들었다(`/home/user/work/preserve/tools-20261116-c/kz-static.5d4a1d7f.xz`, 39,663,956 B; `xz -t`, 스트림 sha256, `cmp` 모두 같음; `records/obs1-20261116-plangen/tool_copy/ROUNDTRIP.txt`). 그 도구의 남은 출력(앞 단계의 relations·키 목록 다섯 빌드, 민감도 실행)을 새 도구가 바이트 동일하게 다시 만들면 등록부에 "대체됨"으로 적는다. 빌드 뒤에 새 kz-static(생성기)을 압축 사본으로 둔다(`xz -9`, 왕복 확인, 같은 디렉터리). 그 사본으로 재생성 점검을 한다.
- **golden**:
  - 바뀌는 golden 은 없다(`tests/static/data`, `tests/golden/data`, `tests/phi/data` 의 모든 파일이 같은 바이트). relations 를 그리는 golden 넷과 relincomplete 둘도 같다(relations 를 고치지 않음).
  - 새 하나: `tests/static/data/plangen_np.plan.golden`(`KZ_GOLDEN_UPDATE=1 ctest -R '^test_static_plan$'` 으로 만들고 읽어 본다).
- **relations 다섯 빌드 재실행**(새 도구, T8CF 의 NP facts 그대로): `kz-static relations <T8CF>/<b>/NP/facts.kz -o relations.txt --keys keys.txt > relations.log 2>&1`, 새 디렉터리 `/home/user/work/redesign/obs1/plangen_relcheck`. 다섯 빌드 모두 `relations.txt`, `keys.txt`, `relations.log` 가 앞 단계의 출력(`/home/user/work/redesign/obs1/relincomplete_check/<b>/`)과 바이트 동일해야 한다. relations 글의 머리 줄에는 도구를 이름 대는 줄이 없다(첫 줄 `kz-relations 1`, 둘째 줄 `facts <hash>`, 셋째 줄 규칙 개정; 빌드 전에 앞 단계 출력의 머리를 읽음 [M]). 그래서 허용하는 줄도 없다.
  - 민감도 실행의 facts(`/home/user/work/redesign/obs1/relincomplete_sens/c1_pre_tx/NP/facts.kz`)에도 relations 를 다시 돌려 그 디렉터리의 `relations.txt` 와 바이트 동일해야 한다.
- **B 다시 뽑기**(c1_pre_tx, 새 도구, `env/redesign/one_build.sh c1_pre_tx /home/user/work/redesign/obs1/plangen_bref`): `compare_build.py` 로 T8CF `c1_pre_tx/NP` 와 파일마다 비교. 허용하는 것: `basis.kz`, `facts.kz`(컨테이너 머리의 도구 해시와, facts 머리가 담는 basis 의 artifact 해시·부모 표처럼 거기서 따라 바뀌는 머리 칸; 섹션의 레코드는 모두 같아야 한다), `basis.txt`(첫 줄 basis 해시), `extract.txt`(첫 줄 facts 해시), `relations.txt`(둘째 줄 facts 해시), `sha256.txt`(basis·facts·도구 줄), 한쪽에만 있는 `facts.txt`(쓰지 않음)·`status.txt`·`space_guard.log`. 레코드 하나라도 다르면 결함이다. 그 출력은 남긴다(파생 빌드 B′ 비교의 같은 도구 기준).
- **계획 다섯 빌드**(새 도구, T8CF 의 NP facts 와 basis, `semantics/framework_api_roles.txt`; 새 디렉터리 `/home/user/work/redesign/obs1/plangen/<b>/`): `--select observation-set --order method-priority --budget tier:D07`, 예산 기록:
  - 값 `tier:D07`(방법 우선순위의 층 1 전체, 곧 처분 D07의 행 모두). 이유: 기록 풀이 고정이고(`KZ_RT_POOL_BYTES = KZ_RT_MAX_ACTORS × 65536`, `kairos/runtime/include/kz_rt.h:61-64`) 생성된 계획의 사건 양은 아직 잰 적이 없다. 그래서 우선 층을 통째로 계획하고 다음 층은 넣지 않는다. 잘림: 층 2와 3의 행(sidecar 에 키마다 `budget_not_run(row outside the budget)`). 미해결: 잘린 행이 기록 풀에 들어가는지. 민감도: 재지 않음.
  - 밝힘: 이 값은 층 D07 이 다섯 빌드에서 어느 행을 담는지 알고 골랐다(`plan/ovm_rev1_20261116/S1_RETALLY.md` 3.1 과 그 탐침 표; c1_pre_tx 의 D07 12행에 사용 지점 키 `17975444d83f6589` 의 행이 있다). 계획의 행은 규칙의 출력이고, 어느 행이 계획에 드는지는 생성 뒤에 점검한다. 생성기는 행 id·키·사례 값을 받지 않는다(`--budget-reason`, `--budget-disclosure` 의 글은 sidecar 에 그대로 적힐 뿐 고르는 데 쓰이지 않는다).
  - 다섯 빌드 모두 rc 0 이어야 한다(규칙이 서지 않는 곳은 `gap`·`need` 줄로 남고 실패하지 않는다). 생성된 manifest 는 런타임의 호스트 파서로 읽혀야 한다(`kz-rt-dump` 가 아니라 `test_static_plan` 과 같은 파서를 링크한 작은 점검; 빌드 트리의 시험 프로그램을 쓰지 않으면 scratchpad 의 점검 프로그램, 이 기록에 명령을 적음).
  - 재생성 점검: c1_pre_tx 와 c5_641 의 계획을 새 kz-static 의 xz 사본으로 다시 만들어 `plan_regen.sh` 가 PASS 여야 한다.
- **측정(판정이 아님, 기대 값을 두지 않음)**: 빌드마다 층별 행 수, 계획된 행, 잘린 행, site 수(규칙별), need·gap 수(사유별). c1_pre_tx: 사용 지점 키 `17975444d83f6589` 의 행이 계획에 있는지, 그 층·차례·예산, 그 행의 site 들. c5_641: 행위자 규칙의 결과(생성 인자가 상수인 곳의 `link_unlinked(no creation-argument channel)` 여부). 우선순위 이식과 탐침 표(`plan/ovm_rev1_20261116/probe/s1_rev1_<b>.rows.tsv`)의 행별 일치 표(표시, 순서 등급, 도달 표시, 처분, 층 1 집합, RELOC 집합; `records/obs1-20261116-plangen/priority_agreement.py`). 손 manifest 와 site 별 비교(C1 pre `kairos/profiles/c1-bp-b051eb0/instr/sites-cf281a941.kzs` 37 site, C5 641 `kairos/profiles/c5-cfe-6.4.x/instr/sites-641.draft.kzs`): 빠진 site 와 더한 site 마다 까닭. 잘못이 아닌 차이는 측정으로 적는다. 생성 인자가 이어지는 조회의 키 글에 대해 lookup 쪽 non-null 검사가 facts 에서 규칙으로 쓰일 수 있는지도 여기서 잰다.
- **시험**: 전체 ctest 243 → 246(새: `static_fixture_compile_plangen`, `test_static_plan`, `lint_case-ids-data`). 라벨: static +2, lint +1, 그 밖은 그대로. golden 갱신 전 첫 실행에서는 `test_static_plan` 하나만 실패해야 한다(golden 파일이 아직 없음). 갱신 뒤 실패 0, 건너뜀 1(`test_probe_anchor_reached`, tracefs 없음). lint 7/7, selftest 109 경우 실패 0. 단위 점검 `env/tools/tests/run_unit.sh`, `env/redesign/step2a_tests_unit.sh` 는 앞과 같은 수로 통과. `vocab_check.py`(앞 단계의 기록 스크립트)로 어휘 자료 = 판정 규칙 3.5 표.
- **누적 바이트 동일성**: 빌드 뒤 `env/tools/ledger_check.sh` 전체와 `tools_check.sh --deep`. 모두 같아야 한다. 이 단계의 새 출력은 장부 행으로 더한다.
- **공간**: 시작 전 여유 ≥ 하한 314,572,800 + 필요량. 필요량 = 앞 단계에서 잰 kz-static 제자리 빌드 dip 174,759,936 + 새 시험 프로그램 하나(앞 단계의 새 시험 프로그램 154,547,088 B 와 같은 크기로 잡음) + 출력 25 MB + 한 시간 흐름 13 MB = 367,307,024 B(감시 문턱 681,879,824). 감시(`env/tools/space_guard.sh --kind kz-static`, 2초마다)가 하한을 넘으면 멈추고 보고한다. 재실행·다시 뽑기·계획·도구 사본도 감시 아래 돌린다. 빌드 전 여유 3,734,945,792 B(06:56:45Z) [M].
- **비교 스크립트**(빌드 전에 이 선언과 함께 커밋): `records/obs1-20261116-plangen/priority_agreement.py`(`python3 -I`), `env/tools/plan_regen.sh`, `env/tools/framework_scan.py`. relations 재실행의 비교는 `cmp`, B 다시 뽑기는 `env/redesign/compare_build.py` 와 허용 파일 `records/obs1-20261116-plangen/bref_allow.tsv`.
- **빌드 전에 한 것(빌드가 아님)**: 생성기의 동작을 잡으려고 scratchpad 에서 `plan_gen.cpp`, `cmd_plan.cpp` 와 작은 입구 하나만 컴파일해 MC 의 `libkz-llvm.a`(앞 단계 빌드의 것)에 링크한 점검 프로그램을 썼다. MC 의 어떤 파일도 바꾸지 않았다. 그 프로그램의 출력은 근거가 아니다(scratchpad). 그것으로 잰 것: fixture 의 계획, 다섯 빌드의 계획 시험 생성(모두 rc 0), 이식과 탐침 표의 일치(다섯 빌드 모두 모든 행 같음). 빌드 뒤의 도구로 다시 잰다.

#### 4.0a 첫 빌드 뒤의 고침 (둘째 빌드 전에 적고 커밋함) [D][M]

- 첫 빌드(07:00Z, kz-static `2e0f8e40…`, kz_lint `5f62ec20…`) 뒤 전체 ctest 에서 선언 밖의 실패가 하나 있었다: `lint_no-generated` 가 새 코드의 글 "generated by"(생성된 코드의 표지로 쓰이는 말) 여섯 곳을 위반으로 냈다(`lib/llvm/CMakeLists.txt:22`, `lib/llvm/plan_gen.cpp:1215` 의 manifest 머리 문자열, `plan_gen.hpp:1`, `tools/kz-static/CMakeLists.txt:9`, `cmd_plan.cpp:1`, `main.cpp:110`). 선언한 실패(`test_static_plan`, golden 없음)는 그대로였다. 빌드 전 점검에서 이 lint 를 돌리지 않았다.
- 고침: 그 글을 "derived by rule"로, manifest 머리 문자열을 "# kz plan written by kz-static plan (...)"로 바꾼다. 그 밖의 코드는 바꾸지 않는다. 고친 트리에서 빌드한 kz_lint(첫 빌드의 것)로 lint 여섯 규칙 모두 깨끗하다 [M].
- 둘째 빌드: MC 제자리 `make -j4 all`. 4.0 의 선언이 그대로 둘째 빌드의 선언이다. 다른 점: kz-static 의 sha256·`tool_sha256`·`tool_source` 는 첫 빌드의 값과도 다르다. kz_lint 는 다시 링크되지 않아야 한다(`tests/lint` 를 고치지 않음; `5f62ec20…` 그대로). 첫 빌드의 kz-static `2e0f8e40…` 은 남긴 출력이 없다(ctest 만 돌렸다). 그래서 사본을 두지 않고 덮는다(등록부에 "gone, 출력 없음"으로 적음). 공간 필요량은 4.0 과 같다.

### 4.1 바뀐 것 [F]

- 커밋 `9263532`(작업 환경이 자동으로 만든 진행 중 스냅숏: 코드·자료·시험·문서, 빌드와 선언 전), `1b38530`(선언, 비교 스크립트의 기록, 빌드 전 도구 사본), `5b13ff6`(첫 빌드 뒤의 글 고침과 선언 덧붙임, 둘째 빌드 전). 규모(`git diff --stat 6059788 5b13ff6 -- kairos`): `lib/llvm/plan_gen.cpp` 1309줄 남짓(새), `plan_gen.hpp` 95줄(새), `tools/kz-static/cmd_plan.cpp` 98줄(새), `tests/static/test_static_plan.cpp` 338줄(새), `tests/static/fixtures/plangen.c` 151줄(새), `semantics/framework_api_roles.txt` 25줄(새), `docs/plan_manifest.md` 108줄(새), `tests/lint/kz_lint.cpp` +83, 사례·프레임워크 목록 둘.
- 생성기의 출력 형식, 규칙, 예산 칸은 `kairos/docs/plan_manifest.md` §1–§6.

### 4.2 빌드 [M]

- 빌드 전(`pre_mc.txt`, 06:59:42Z): 도구 일곱이 앞 단계 `post_mc.txt` 와 같다. HEAD `1b38530`, `kairos` 미커밋 0줄.
- 첫 빌드(06:59:48Z–07:00:35Z, 공간 감시 아래, rc 0; `make_build1.log`, `df_build1.log`, `post_mc_build1.txt`): kz-static `2e0f8e40…`, kz_lint `5f62ec20…`. 전체 ctest 에서 선언 밖의 실패 `lint_no-generated` 하나(§4.0a). 글을 고치고 커밋(`5b13ff6`)한 뒤 둘째 빌드.
- 둘째 빌드(07:01:59Z–07:02:43Z, rc 0; `make_build2.log`, `df_build2.log`, `pre_mc_build2.txt`): 링크 12줄(`libkz-llvm.a`, kz-static, `kz-llvm` 을 링크하는 시험 여덟, `test_static_reasons`, `test_static_plan`). kz_lint 는 다시 링크되지 않았다. 빌드 시작 뒤 정체성 경로의 파일은 바뀌지 않았다(`find -newer` 0).

| 도구 | 전 | 뒤 |
|---|---|---|
| kz-static | `5d4a1d7f4023b20cbd6caf797edcdbd0e54d8efb329ace358c07002f27088749` | `de1315b359af279c46585d80e8a71db6143a19ded1ca164e4b88bc7106a3abdc` (178,474,384 → 184,095,248 B) |
| kz-static tool_sha256 | `c162e7f6…` | `c3677e76e8b01b55dd4c1e5b1778d6f61e3391942a83a4eba25c34ffb9304f7b` |
| kz-static tool_source | `7003f7ed…` | `5640d5d65ad3310115ca5616f96a9d794d07e5e3f1681bff010765b127f77cb1` |
| core_source | `f6e3c7b4…` | 같음 |
| kz_lint | `b0f0d6a5…` | `5f62ec20d2d26e74501cf7469649761df662d673659835ec9bb0902ed6acd018` (첫 빌드에서; 둘째 빌드는 다시 링크하지 않음) |
| kz / kz-dump / kz-rt-dump / kz-phi / kz-instr-clang | `931dd39c` / `05623054` / `4390d4f1` / `7d34e6e2` / `28db4017` | 모두 같음 |
| kz, kz-phi, kz-dump 의 정체성 정의 | `caf58235…`, `7fc3ed73…`, `cc51ff35…` | 같음 |
| `CMakeCache.txt` | `bc2f43b4…` | 같음 |
| flags.make 목록 해시 | `1d3b5bba…` | `c92ab3c1…` (kz-static 의 정의, 새 `test_static_plan`) |

- 선언과 다른 점: `static_golden` 은 다시 링크되지 않았다(선언은 kz-static 에 대한 의존으로 다시 링크된다고 적었다; 그 프로그램은 `kz-core` 만 링크하고 소스가 그대로라 다시 링크할 까닭이 없다). 결함이 아니다.

### 4.3 시험 [M]

- golden 갱신 전 전체 ctest(둘째 빌드, `ctest_first.log`): 246개 가운데 실패 1, 선언한 것뿐이다(`test_static_plan`: golden 파일이 아직 없음; 그 밖의 181 확인은 통과).
- `KZ_GOLDEN_UPDATE=1 ctest -R '^test_static_plan$'` 로 `tests/static/data/plangen_np.plan.golden`(129줄)을 쓰고 읽었다: manifest 18 site, sidecar(행 넷, 모두 층 1·D07; A1 은 표시 R, 나머지 J), module 표(모듈 하나), 실행 파일 목록(없음). sha256 값 8곳과 입력 경로는 가렸다. 그 밖의 golden 은 바뀌지 않았다(`git status` 에 없음).
- 갱신 뒤 전체 ctest(07:03:15Z–07:03:36Z, `ctest_full.log`): 246개, 실패 0, 건너뜀 1(`test_probe_anchor_reached`, tracefs 없음; `skip_reason.log`). 라벨: static 51, lint 7, golden 60, legacy-case 51, unit 84, runtime 66(앞 단계에서 static +2, lint +1).
- `test_static_plan` 182 확인, `test_static_reasons` 612 확인(앞 575, +37: 생성기의 사유 표 아홉), `test_static_cli` 837 확인(그대로), `lint_selftest` 109 경우 실패 0(앞 102, +7), lint 7/7.
- 단위 점검: `env/tools/tests/run_unit.sh` 8/8, `env/redesign/step2a_tests_unit.sh` 7/7. `vocab_check.py` SAME(22 사유).
- 합성 시험이 본 것(`test_static_plan`, fixture `plangen.c`): 두 생성의 바이트 동일, 한 바이트 고친 사본의 불일치; 런타임 호스트 파서가 manifest 를 받음(`kzs-sites 3`, identity·control·bind 없음, 모든 site 에 label·ref, 값 넷 이하); sidecar sha256 이 manifest 머리·module 표·실행 파일 목록 첫 줄에 있음; 조회의 at-call(키는 pointer-only)과 after-call(`ret`, `out0`); 조회 API 안의 키 글(`reg_search` 호출 앞, non-null 검사 아래, bound 64), 생성 API 안의 키 글(`memchr` 호출 앞, bound 16); 생성자 호출; init 의 반환, 앱의 결과(`one_init` 의 after-call), run status 저장(104행), app-exit 호출; 한 줄의 두 호출은 계획되지 않고 `record_insufficient(two calls of one callee on one line)`; 루프 안의 조회는 계획됨; 생성 primitive 의 at-call(start routine, 생성 인자)·after-call, start routine(= 앱의 entry)의 `param0`, spawn 뒤 owner 저장(84행); stop event 둘; 거부 넷과 출력 없음; 우선순위 이식의 여섯 경우; 둘째 호출자(첫 계획의 site 키 모두 + 새 조회의 at-call·after-call·entry).

### 4.4 바이트 동일성 [M]

- relations 다섯 빌드(새 도구, T8CF 의 NP facts; 07:04Z, `/home/user/work/redesign/obs1/plangen_relcheck`): `relations.txt`, `keys.txt`, `relations.log` 다섯 빌드 모두 앞 단계의 출력과 바이트 동일(`byte_identity.txt`; `7c7396d2…`, `0a3bf6d9…`, `3a6fcf81…`, `b62fd1f2…`, `996d32bd…`). 민감도 실행의 facts 에 돌린 relations 도 앞 단계의 것과 바이트 동일. 그래서 앞 단계의 kz-static `5d4a1d7f` 를 등록부에 "대체됨"(사본 있음)으로 적었다.
- B 다시 뽑기(c1_pre_tx, 새 도구, 07:04:26Z–07:04:54Z, PASS; `/home/user/work/redesign/obs1/plangen_bref`; `bref_run.txt`, `bref_compare.tsv`): 허용 밖 0. `basis.kz` 는 머리의 `artifact_sha256`, `tool_sha256` 만, `facts.kz` 는 머리의 `artifact_sha256`, `basis_sha256`, `parent[0]`, `tool_sha256` 과 BASIS 섹션의 레코드 하나(`extract.units_from`)와 STRINGS 의 crc 가 다르다. 그 레코드는 basis 의 artifact 해시 앞 16자리다(`basis 87a80eb0e3731b55` → `basis b06fd996d877d20a`). 선언은 "섹션의 레코드는 모두 같아야 한다"고 적었다. 이 레코드는 선언이 이름 대지 않은, 도구 정체성에서 따라 바뀌는 칸이다(§4.8). 그래서 facts 의 전체 글 보기(`kz-static show`, 모든 섹션, 148,887줄)를 T8CF 의 `facts.txt` 와 비교했다: 다른 줄은 그 하나뿐이다(`bref_facts_view.txt`). basis 는 머리 밖에서 같다. `relations.txt` 는 T8CF(개정 5)가 아니라 앞 단계의 c1_pre_tx relations(개정 6)와 둘째 줄(facts 해시) 밖에서 같다(선언의 "둘째 줄"은 이 기준을 뜻해야 했다, §4.8). `args.txt`, `relations.log` 는 같은 바이트.
- 다시 돌리지 않은 것(새 도구로는 확인하지 않음): 나머지 네 빌드의 basis·extract, NP2, identity, count, twice, 드라이버 비교.
- 계획의 재생성 점검(새 kz-static 의 xz 사본으로, `plan_regen.sh`): c1_pre_tx, c5_641 모두 PASS(`regen_c1_pre_tx.txt`, `regen_c5_641.txt`).
- 누적 목록: `ledger_check.sh` 37행, 실패 0(새 행 넷: 도구 사본 둘의 목록, 계획 다섯 빌드, relations 재실행, B 다시 뽑기). `tools_check.sh --deep` 31행, 실패 0(새 행 셋: `2e0f8e40` gone, `de1315b3` in-place+copy, kz_lint `5f62ec20`; `5d4a1d7f` 와 kz_lint `b0f0d6a5` 는 copy 로). 바깥 이름 점검(`--tree`)은 세 출력 디렉터리와 사본 디렉터리에서 깨끗하다.
- 도구 사본: kz-static `5d4a1d7f`(빌드 전, 39,663,956 B)와 생성기 `de1315b3`(빌드 뒤, 40,972,128 B), `/home/user/work/preserve/tools-20261116-c/`. 둘 다 `xz -t`, 스트림 sha256, `cmp` 같음(`tool_copy/ROUNDTRIP.txt`).

### 4.5 측정: 계획 [M]

계획 다섯 빌드(07:05:40Z, 모두 rc 0, 로그 빈 파일; `/home/user/work/redesign/obs1/plangen/<b>/`; `measure.txt`). 예산은 다섯 빌드 모두 `tier:D07`, 이유·밝힘은 §4.0 의 글 그대로(sidecar 의 `budget` 줄). 계획의 식별: c1_pre_tx manifest `5f015c5aa86bf642ac1a76cf49669c98520eac541868cf44e24966649d9d60ae`, sidecar `8f5f200cbe436f587e3ef1e3ee2ec0782675ba3c2860aa8d0ec4f7cdba29c721`; c5_641 manifest `17b6ab6a4089039b5ce601102a85adee6fed0a60f5218789155322d79907317e`, sidecar `2276c1015f7b0f3adb83a9019851bcbf960154bb2e2cddced917e6c97da7aa28`(사본: `records/obs1-20261116-plangen/plans/`). 나머지 셋의 해시는 `plans.sha256`.

| | c1_pre_tx | c1_post_tx | r2_core | c5_641 | c5_642 |
|---|---|---|---|---|---|
| 행(층 1/2/3) | 431 (12/306/113) | 431 (11/307/113) | 338 (3/196/139) | 178 (8/78/92) | 175 (7/66/102) |
| 계획된 행(경로) | 12 (A 5, B 7) | 11 (A 4, B 7) | 3 (B 3) | 8 (B 8) | 7 (B 7) |
| 잘린 행 (`budget_not_run`) | 419 | 420 | 335 | 170 | 168 |
| site / 값 | 301 / 671 | 287 / 641 | 42 / 62 | 147 / 275 | 139 / 263 |
| use-call / writer / creator-call | 194 / 35 / 40 | 192 / 35 / 36 | 10 / 17 / 0 | 110 / 23 / 0 | 104 / 21 / 0 |
| key-text (validated 값) / handling-chain | 4 (4) / 7 | 2 (2) / 0 | 0 / 0 | 0 / 0 | 0 / 0 |
| actor-root / -create / -start / -owner | 15 / 2 / 1 / 2 | 15 / 2 / 1 / 2 | 8 / 2 / 1 / 2 | 8 / 2 / 8 / 2 | 8 / 2 / 8 / 2 |
| stop-event | 2 | 2 | 2 | 2 | 2 |
| gap | `record_insufficient(key text)` 10 | 같음 10 | 0 | `link_unlinked(no creation-argument channel)` 1 | 같음 1 |
| need not-planned | wait 12, lock 12, event-send 5 | 11, 11, 4 | 3, 3, 0 | 8, 8, 0 | 7, 7, 0 |
| module 표 | 6 + 기본(core-tx) | 같음 | 기본만(core-cpu1) | 5 + 기본(core-linux.bin) | 같음 |

- 층별 행 수는 판정 규칙 개정 1 의 재집계(`plan/ovm_rev1_20261116/S1_RETALLY.md` 3.1: D07 12/11/3/8/7, 재배치 후보 318/318/199/86/73)와 같다. 이식과 탐침 표의 행별 일치(`priority_agreement.txt`): 다섯 빌드 모두 표시·순서 등급·도달 표시·처분이 모든 행에서 같고(431/431/338/178/175), 층 1 집합 = D07, RELOC 집합 = 지향 실행 후보.
- 생성된 manifest 다섯 개 모두 런타임의 호스트 파서로 읽힌다(`parse_check.txt`; scratchpad 의 점검 프로그램이 MC 의 `libkzrt-manifest.a` 를 링크; version 3, identity·control·bind 0, 가장 큰 site id 301, 가장 큰 slot 3).
- **c1_pre_tx, 사용 지점 키 `17975444d83f6589` 의 행(T8CF 의 A21)**: 계획에 있다. 층 1, 차례 1(우선순위 차례에서 첫째), 예산 `tier:D07` 안. 표시 R, 순서 RELOC, 도달 CALLEE-ONLY, 처분 D07. 그 행의 site 14개(sidecar 의 `map` 줄): 사용(CF_CFDP_InitEngine 의 at-call: 키는 pointer-only, `arg0`; after-call: `ret`, `out0`), 키 글 둘(조회 API 안 `osapi-countsem.c:212` 의 `OS_ObjectIdFindByName` 호출 앞, `arg1` validated bound 64 — facts 의 PARAM_FLOW 가드 `(ne param:sem_name#1 0)` 가 그 호출을 지배하므로 lookup 쪽 non-null 검사는 규칙에 쓰일 수 있었다; 생성 API 안 `osapi-countsem.c:91` 의 `memchr` 호출 앞, bound 20 = 그 `memchr` 의 길이 상수), 생성자 둘(BP `bp_flow.c:132`, SCH_LAB `sch_lab_app.c:168` 의 at-call·after-call), 처리 사슬(CF_Init 안 `CF_CFDP_InitEngine` 의 after-call, CF_AppMain 안 `CF_Init` 의 after-call, `cf_app.c:333`·`:357` 의 store, `CFE_ES_RunLoop` 의 after-call, `CFE_ES_ExitApp` 의 at-call). 그 밖에 이 행과 모든 행이 함께 쓰는 행위자 site(생성 primitive `os-impl-tasks.c:543` 의 at-call·after-call, start routine `OS_PthreadTaskEntry` 의 entry 와 `param0`, 앱 root 의 entry, `cfe_es_apps.c:619`·`:620` 의 owner store)와 stop event(`CFE_ES_RunLoop`, `CFE_ES_ExitApp` 의 entry).
- **c5_641**: 행위자 규칙은 생성 primitive `osapi.c:645` 의 생성 인자가 상수 0 이고 root 여덟이 그 호출을 거친다는 것을 facts 에서 얻어 `gap ... link_unlinked(no creation-argument channel)` 을 냈다. start routine 의 entry 는 매개변수 없이 계획되었다. relation 2 의 행(C22, C31)은 층 2(차례 79, 83)라 이 예산에서 잘렸다(C22 표시 E·처분 D12, C31 표시 F·처분 D10).

### 4.6 측정: 손 manifest 와의 site 별 비교 [M]

`hand_compare.py`(`python3 -I`): 함수, place(또는 entry·return 의 phase), callee, 단위 경로의 끝, (at-call·after-call·store 는) 줄이 같으면 대응. 손 manifest 의 `phase=after`(함수의 반환, kzs-sites 1 뜻)는 생성기에 같은 종류가 없어 호출자의 after-call 로 맞춰 보았다.

- C1 pre(`sites-cf281a941.kzs`, 37 site; `hand_compare_c1_pre.txt`): 대응 12, 빠짐 25, 생성기에만 있는 site 290.
  - 대응: ExitApp 의 entry(9, stop event), 조회의 반환(11, CF 의 after-call), 조회 API 안 `:212` 의 키 글(12), 생성의 반환(15, 생성자 둘의 after-call), CF_Init·InitEngine 의 반환(31, 33, 호출자의 after-call), CF 의 조회 at-call·after-call(34, 35), CF 의 ExitApp at-call(37), BP 의 생성 at-call·after-call(44, 45), 그리고 4(CreateChildTask 의 반환: B13 의 writer 규칙이 낸 `CFE_ES_BackgroundInit` 안의 after-call 과 맞음; 한 호출자에서만).
  - 빠짐과 까닭: 1–3, 5–8(생성 쪽 식별: StartAppTask·CreateChildTask·TaskCreate 의 entry·반환, OS_TaskEntryPoint, InternalTaskCreate_Impl 의 entry): 생성기는 손 manifest 의 identity 문 대신 생성 primitive 의 at-call·after-call, start routine 의 `param0`, spawn 뒤 owner store 로 묶는다(규칙이 facts 에서 내는 다른 자리). 10, 14(조회·생성의 entry 의 포인터 인자): 생성기는 호출자 쪽 at-call 로 인자를 적는다. 13(`osapi-idmap.c:978` 의 찾은 id store): 생성기는 조회의 after-call `out0` 으로 적는다. 16, 17(생성의 게시점과 NotifyEvent): 생성기의 키 글 자리는 `:91` 의 `memchr` 이고, 게시와 알림은 결과의 효과라 이 개정의 facts 에 없다. 18–22(OS_API_Init 의 표 지우기, 삭제와 그 반환·retire): 이름 공간의 수명 줄은 relations 의 묶음 글이고 행의 줄이 아니라 이 개정의 규칙이 읽지 않는다. 23, 24(PSP 의 종료 요청, teardown): 행의 처리 사슬에 없다; 멈춤 사건은 run loop 와 app exit 로 잡는다. 30, 32(CF_Init·InitEngine 의 entry): 생성기는 반환을 호출자의 after-call 로 적고 entry 는 규칙이 요구하지 않는다. 40, 41(BP_FlowInit 의 entry·반환): 행의 사슬에 없다. 42, 43(BP 의 존재 탐침 조회 `bp_flow.c:126`): 다른 행(T8CF 의 A20, 층 1 이 아님)이라 예산에서 잘렸다.
  - 생성기에만 있는 290: 규칙별 use-call 192, creator-call 37, writer 34, actor-root 15, handling-chain 3, key-text 3, actor-create 2, actor-owner 2, actor-start 1, stop-event 1(계획된 행 열둘의 사용·쓰기, A22·A23·A50·A51 과 B 행들의 site, 행위자와 stop event).
- C5 641(`sites-641.draft.kzs`, 33 site; `hand_compare_c5_641.txt`): 대응 4(생성 primitive `osapi.c:645` 의 at-call·after-call 14, 15; `CFE_ES_GetAppID` 의 반환 26 은 B 행의 writer 규칙이 낸 `CFE_SB_AppInit` 의 after-call 과; `SAMPLE_AppMain` 의 entry 32 는 actor-root), 빠짐 29, 생성기에만 있는 site 143. 빠짐의 까닭: 손 manifest 는 relation 2(C22, C31: AppCreate 의 게시 store, 잠금 풀기·다시 잡기, OS_TaskCreate 의 id store, TaskRegister, GetAppID 의 store)를 위한 것인데, 그 행은 층 2 라 이 예산에서 잘렸다. 잠금·대기 결과는 이 개정에서 계획하지 않는다(need not-planned). 생성기에만 있는 143 은 계획된 B 행 여덟의 site(use-call 110, writer 22), 행위자, stop event.

### 4.7 공간 [M]

| 시점 (UTC) | 여유 (df -B1 /) | 일 |
|---|---|---|
| 06:54:49 | 3,775,651,840 | 이 단계의 첫 잼(앞 단계 끝 4,145,606,656 에서 줄었다: scratchpad 의 점검 프로그램과 이 세션 밖의 흐름 [I: 파일별로 재지 않음]) |
| 06:54:49–06:56:45 | → 3,734,945,792 | 빌드 전 도구 사본(dip 40,706,048) |
| 06:59:48–07:00:35 | → 3,525,509,120 | 첫 빌드(dip 209,080,320, 변화 −209,080,320) |
| 07:01:59–07:02:43 | 3,523,895,296 → 3,523,829,760 | 둘째 빌드(가장 낮은 점 3,304,431,616, dip 219,463,680, 변화 −65,536) |
| 07:04:18 | 3,518,070,784 | relations 재실행(dip 7,364,608) |
| 07:05:42 | 3,503,730,688 | 계획 다섯(dip 1,093,632) |
| 07:08:02 | 3,461,013,504 | 생성기 사본(dip 40,980,480) |
| 07:08:15–07:08:19 | 3,276,591,104 (가장 낮은 점) | 재생성 점검 둘(사본을 scratchpad 에 풀었다; 점검 뒤 지움) |

- 하한을 넘지 않았다. 빌드 dip(209.1 MB, 219.5 MB)은 선언한 필요량 367,307,024 B 안이다. 새 시험 프로그램 `test_static_plan` 이 순변화의 대부분이다 [I: 파일별로 재지 않음].
- 지운 것: scratchpad 의 재생성 점검 디렉터리 둘(풀어 놓은 생성기 사본; 사본의 xz 는 남음)과 비교에 쓴 facts 글 보기. 그 밖에 아무것도 지우지 않았다. 앞 단계의 출력(relations 다섯 빌드, 민감도 실행)은 남겨 두었다(§4.8).

### 4.8 계획과 다르게 한 것

- 빌드가 두 번이었다. 첫 빌드 뒤 lint `no-generated` 가 새 글의 "generated by" 를 냈다(§4.0a). 글만 고치고 선언을 덧붙여 커밋한 뒤 다시 빌드했다. 첫 빌드의 kz-static `2e0f8e40` 은 출력이 없어 사본 없이 덮었다.
- 생성기 코드·자료·시험은 작업 환경의 자동 스냅숏 커밋(`9263532`)에 먼저 들어갔고, 선언은 그다음 커밋(`1b38530`)이다. 둘 다 빌드 전이다.
- 빌드 전에 scratchpad 의 점검 프로그램(생성기 코드를 MC 의 기존 `libkz-llvm.a` 에 링크)으로 생성기를 다듬었다(§4.0 끝 항목). MC 의 도구는 바꾸지 않았다.
- B 다시 뽑기의 비교: facts 의 BASIS 레코드 `extract.units_from`(basis artifact 해시 앞 16자리)는 선언이 이름 대지 않은, 도구 정체성에서 따라 바뀌는 칸이었다. 전체 글 보기 비교로 그 줄 하나만 다름을 확인했다. relations 의 기준은 T8CF 가 아니라 앞 단계의 출력이어야 했다(선언의 "둘째 줄"은 그 기준에서만 맞다). 둘 다 도구의 결함이 아니고 선언 글의 빈틈이다.
- relations 머리에는 도구를 이름 대는 줄이 없다(첫 줄 형식, 둘째 줄 facts 해시, 셋째 줄 규칙). 그래서 "도구를 이름 대는 머리 줄 밖에서 같음"이 아니라 바이트 동일로 비교했다.
- 예산 값의 이름: `tier:D07`(층 1). 그 밖의 값 `tier:directed`, `all`, `rows:<n>` 을 두었다. 예산의 이유와 밝힘은 명령줄 글로 받아 sidecar 에 그대로 적는다(생성기는 그 글을 읽어 고르지 않는다).
- 생성기의 입력: facts 만이 아니라 그 basis(module 표·실행 파일·단위 경로는 basis 의 TUS 와 항목에만 있다)와 프레임워크 계층 자료. facts 가 basis 를 이름 대면 `--basis` 없이는 거부하고, 다른 basis 는 거부한다.
- module 표의 접두어와 실행 파일 목록은 기준 빌드의 절대 경로다. 파생 빌드는 다른 디렉터리에서 하므로 파생 스크립트가 그 경로를 자기 복사본으로 옮겨야 한다(`derive_args.sh` 는 지금 `@ROOT/` 만 바꾼다). 그 매핑은 파생 빌드 단계의 일이다. 단위 경로(`tu=`)는 bundle root 기준이거나, bundle 밖 단위의 공통 디렉터리 기준이다(c1: `src/cf/fsw/src/cf_cfdp.c`). 실행 파일 목록은 basis 의 tx 쪽만이다(rx 는 basis 밖).
- 값의 형식: facts 에 형이 없어(형은 뒤 단계) 정수는 `u64`, 결과는 `s64`, 키는 `ptr`(문자열)로 적는다. 플러그인이 피연산자 자신의 형에서 넓힌다(`kzgcc.cpp` 의 `to_u64`).
- 한 줄에 같은 callee 를 두 번 부르는 자리는 계획하지 않고 gap 으로 적었다(바인딩이 줄 단위). 실제 다섯 빌드의 계획에서는 이 경우가 생기지 않았다.
- `need event-send` 는 경로 A 의 계획된 행마다 적었다(결과의 효과가 facts 에 없다). `cf_cfdp.c:997` 의 사건 전송(행의 처리 사슬에 없다)은 이 need 에 든다.
- 앞 단계의 출력(relations 다섯 빌드의 재실행, 민감도 실행)은 이 단계의 도구가 바이트 동일하게 다시 만든 뒤에도 지우지 않았다. scratchpad 밖의 출력은 지우지 않는다. 공간은 넉넉하다(§4.7).
- 정체성: `test_static_elf32` 도 `kz-llvm` 을 링크하는 여덟에 든다(선언의 "여덟"). 셈은 같다.

## 5. 생성된 계획으로 만든 파생 C1 빌드 B′ (Kairos 도구 빌드 없음)

기록: `../records/obs1-20261116-derived/`. 출력: 저장소 밖 새 디렉터리 셋, `/home/user/work/redesign/obs1/c1pre_rt/`(대상 런타임 트리), `/home/user/work/redesign/obs1/c1pre_derived/`(파생 작업 공간 W), `/home/user/work/redesign/obs1/c1pre_bprime/`(B′ 의 basis·facts). F1T(`/home/user/work/instr/gcc/f1`), W6 기록, T8CF, 앞 단계의 출력 디렉터리에는 쓰지 않는다. cFS 를 부팅하지 않는다. push 하지 않는다.

### 5.0 선언 (파생 빌드 전에 적고 커밋함) [D]

파생 빌드 전에 적었다. 아래 밖의 차이는 결함이다. 결함이면 다음으로 가기 전에 멈추고 까닭을 적는다(사례에 맞춘 고침은 하지 않는다).

- **Kairos 도구**: 다시 빌드하지 않는다. MC 의 도구 일곱(kz-static `de1315b3`, kz `931dd39c`, kz-dump `05623054`, kz-rt-dump `4390d4f1`, kz-phi `7d34e6e2`, kz-instr-clang `28db4017`, kz_lint `5f62ec20`), 플러그인 둘(`56a96b79`, `27a49a6a`), kairos-build 도구의 sha256 은 이 단계 앞뒤에 같아야 한다(`build_record.sh pre|post`, `tools_check.sh --deep`). 정체성이 바뀌는 도구: 없음. 대상 런타임 트리(`build_runtime.sh`)는 `kairos/runtime` 소스를 새 디렉터리에서 대상 gcc 로 따로 빌드한 것이다. 그 안의 라이브러리·객체·kz-rt-dump 의 sha256 은 기록으로 둔다(도구 등록부의 도구를 덮지 않는다).
- **스크립트**(이 선언과 함께 커밋; `kairos/instrument/gcc` 는 따로 된 CMake 프로젝트이고 `env/` 는 `kairos` 밖이라 어떤 도구의 정체성에도 들지 않는다, §1.1):
  - `kairos/instrument/gcc/scripts/path_map.sh`(새): 기준 빌드의 절대 경로를 파생 작업 공간으로 옮기는 기계적 매핑. 맵 파일의 줄은 "기준 접두어 TAB 파생 접두어"(둘 다 절대 경로, `/` 로 끝남, 기준 접두어는 한 번). 가장 긴 기준 접두어가 이긴다. 상대 경로는 그대로 둔다. 어느 줄도 덮지 않는 절대 경로는 거부한다. 스크립트 안에 경로 값이 없다.
  - `derive_c1.sh prepare`: 자기가 만든 복사본마다 한 줄씩 `<W>/path_map.tsv` 를 쓴다(`c1_build/bundle_1df262a/` → `<W>/bundle_1df262a/`, `c1_build/pre/src/` → `<W>/src/`, `c1_build/pre/1df262a/` → `<W>/ws/`; 마지막 줄은 기준 빌드 디렉터리이고 `build/`, `exe/` 가 `ws/` 아래 같은 자리에 만들어진다). 값은 prepare 가 복사한 원본과 대상에서 나오고 사례 값이 아니다.
  - `derive_args.sh`: `--path-map FILE`. 주면 module 표의 절대 접두어와 실행 파일 목록의 절대 경로를 맵으로 옮기고, 덮이지 않으면 rc 64. `@ROOT/` 와 상대 경로는 그대로다. 계획 입력 셋 없이 맵만 주면 거부.
  - `derive_plan.sh`: `--path-map` 을 요구한다(계획의 표가 기준 빌드의 경로를 적으므로). 실행 파일 목록의 첫 줄과 manifest 머리가 module 표와 같은 sidecar(경로와 sha256)를 가리키지 않으면 거부.
  - `derive_c1.sh build|check`(계획 모드): 해시 줄은 맵으로 옮긴 파생 파일과 계획이 이름 댄 기준 파일을 나란히 적는다. check 는 계획의 실행 파일 목록으로 bincheck·hookcheck 를 하고, 설치 트리의 모든 파일을 기준 빌드의 같은 자리 파일과 나란히 적는다(`install_inventory.tsv`: 파생 sha256, 기준 sha256, 같음/다름, 계획의 목록에 있는지).
  - `env/redesign/one_build.sh BUILD OUT --path-map MAP`: 단일 빌드 실행기의 파생 작업 공간 방식. 입력 표의 compile db, build dir, bundle root 를 맵으로 옮기고, 기준 recipe 는 제자리에서 읽는다(recipe 의 경로는 build dir 기준이다, `extract_baseline/inputs/c1_pre_tx.kzb:2`). 인자는 B 와 같다(basis·extract `--no-profile`, `--targets-rule R-T1,R-T2`, target 목록 없음). 덮이지 않는 입력, 없는 파생 입력은 rc 2. `args.txt` 에 맵과 그 sha256 한 줄.
  - `env/redesign/compare_derived.py`(새, `python3 -I`): B 와 B′ 의 basis·facts(kz-dump)와 글 출력을 레코드마다 비교한다. B′ 의 모든 레코드에서 맵의 파생 접두어를 기준 접두어로 되돌린다. 맵은 FILE_HASHES 의 내용으로 확인한다(옮긴 경로마다 B 의 같은 경로와 sha256 비교; 다른 것은 차이로 나열). `fh=<n>` 은 그 파일 FILE_HASHES[n] 의 경로로 비교하고, FILE_HASHES 는 경로의 집합으로 비교한다(순서가 같은지는 요약에 적음). 그 밖의 절에서 순서만 다른 것도 차이다. 차이마다 선언 목록(`records/obs1-20261116-derived/declared_diff.tsv`, 조건 `both=`/`new=`/`old=`/`only=필드`)에 맞는지 본다. 맞지 않으면 rc 1.
  - `env/tools/plan_bindings.py`(새, `python3 -I`): 바인딩 보고. manifest 의 site 마다 바인딩 줄이 있어야 한다(bound/inlined/duplicated, 또는 removed/unsupported 와 플러그인의 이유). 어느 단위도 보고하지 않은 site(`not-seen`)는 실패. 단위마다 바인딩 머리의 manifest sha256 = 계획의 것. module: manifest 의 `module` 문 = module 표(이름과 id, 기본), 플러그인 옵션 = 표의 id 차례와 기본, 단위마다 module id = 플러그인 규칙(`kzgcc.cpp` `start_unit`: 첫 접두어, 없으면 기본)이 주는 id.
  - `env/tools/tests/run_unit.sh`: 점검 8 을 넓히고(맵 없음, 다른 sidecar 의 실행 파일 목록·manifest → rc 64) 9–12 를 더함(맵 함수; 파생 작업 공간 실행기; 비교기; 바인딩 보고). 선언 전에 12/12 [M], `step2a_tests_unit.sh` 7/7 [M].
- **입력**(읽기만): 계획 c1_pre_tx(`/home/user/work/redesign/obs1/plangen/c1_pre_tx/`): manifest `5f015c5aa86bf642…`, sidecar `8f5f200cbe436f58…`, module 표 `plan.mod` `28180ed5d8c39c63…`, 실행 파일 목록 `plan.exe` `09718829e381d2c8…`. 플러그인 `27a49a6a…`(kzgcc 0.3, `/home/user/work/c5tools/plugin-build/kzgcc.so`; 생성된 `kzs-sites 3` manifest 를 읽는다: 작은 단위 하나로 rc 0 을 봤다 [M], scratchpad). 기준 빌드 `/home/user/work/procrace/c1_build/pre/1df262a`, `bundle_1df262a`, `pre/src`. 같은 도구의 기준 B: `/home/user/work/redesign/obs1/plangen_bref/c1_pre_tx/NP`(§4.4).
- **단계**(모두 공간 감시 아래, `--need 100000000`; 계획의 추정 최고점 약 53 MB + 런타임 6.4 MB + B′ 13 MB + 한 시간 흐름 13 MB 를 올림):
  1. `build_runtime.sh /home/user/work/redesign/obs1/c1pre_rt/rt <plan.kzs> <kzgcc.so 27a49a6a>`.
  2. `derive_c1.sh prepare pre 1df262a /home/user/work/redesign/obs1/c1pre_derived`(맵을 씀).
  3. `derive_plan.sh c1 build pre 1df262a <W> ref --manifest <plan.kzs> --modules <plan.mod> --exes <plan.exe> --path-map <W>/path_map.tsv`(같은 자리의 기준 빌드; 해시 뒤 지움, `derive_c1.sh` 의 기존 동작).
  4. `derive_plan.sh c1 build pre 1df262a <W> instr <kzgcc.so> <rt> …같은 넷`.
  5. `derive_plan.sh c1 check pre 1df262a <W> …같은 넷`.
  6. `plan_bindings.py`(module 옵션은 `derive_module_flags` 를 맵과 함께 다시 계산한 글이고, 그것이 `build_instr.record` 의 D3 줄에 그대로 있어야 한다).
  7. rx: 설치 트리 `exe/rx` 의 ELF 파일에 bincheck 를 돌려 (함수, hook, id) 목록을 계획의 그룹(tx)과 비교한다(측정; 계획의 입력이 아니다).
  8. `one_build.sh c1_pre_tx /home/user/work/redesign/obs1/c1pre_bprime --path-map <W>/path_map.tsv`(MC kz-static `de1315b3`).
  9. `compare_derived.py`(B = `plangen_bref/c1_pre_tx/NP`, B′ = 8 의 출력).
  10. B′ 실행 파일의 GNU build-id(`readelf -n`)와 sha256 을 적는다.
- **rx 인스턴스를 어떻게 만드는가(결정과 까닭)**: 같은 파생 트리다. 까닭: 두 인스턴스 기준 절차를 돌리는 하네스는 tx 기록에서 `<W>/ws/exe` 전체(tx, rx, host)를 묶어 풀고(`run_gi_c1.sh:47`) 그 안의 `./core-rx` 를 실행한다(`:68`). 그래서 rx 는 그 트리에 있어야 한다. `derive_c1.sh build` 는 `mission-install` 하나로 두 target 을 같은 arch 빌드(`build/native/default`)의 객체로 만들고 설치한다. 앞선 C1 G-I 부팅의 파생 트리도 그렇게 만들어졌다(F1T `c1/pre_1df262a/check.record`: "hook calls: tx 42, rx 42; same (function, hook, id) lists: yes"). 계획의 실행 파일 목록은 basis 의 것(tx 쪽)뿐이므로 hookcheck 는 tx 로 하고, rx 는 설치 목록과 7 의 비교로 적는다.
- **B 와 B′ 사이에 허용하는 차이**(레코드·칸 수준; 조건 그대로는 `declared_diff.tsv`):
  - (가) 객체: 계측된 단위(바인딩 파일이 있는 단위)의 객체와 D4 의 객체. `objects_differ.txt` 의 다른 객체는 결함(`derive_c1.sh check` 의 "UNEXPECTED").
  - (나) 기록된 명령의 플러그인 옵션(`kairos/lib/llvm/basis.cpp:1065-1098`: 명령은 `-fplugin` 과 함께 기록되고 front end 는 그 옵션 없이 읽는다): BASIS 의 `instrumentation.*`, `front_end.arguments_removed`(새 항목), `units.distinct_flag_sets`(값), DEVIATIONS 의 새 레코드 하나("build-compiler plugin in the recorded commands"), TUS 의 `command_sha256`(과 플러그인 옵션이 든 `flags`), FILE_HASHES 의 `compile_commands.json`(sha256, size)과 새 플러그인 파일 항목.
  - (다) 실행 파일에 링크된 런타임과 바인딩 자리의 런타임 호출(D3): core-tx 의 `link.txt`(sha256, size), BASIS 의 `link.*`(값, 새 `not_in_program[...]`·`members_without_command`), `binary.sha256`, `binary.build_id`, `closure.executable_exports`, `module[*]` 의 `undefined`·`unbound`·`bound[...]`·`link.output_is_the_loaded_file`·`resolved_by_its_link`(값 또는 새 항목), `basis.txt` 의 같은 줄과 요약 줄(link inputs, link members, module symbols bound, references across units).
  - (라) D2: 기준 빌드의 module 판 표를 같은 바이트로 설치(`derive_c1.sh:18`, `:43-48`). FILE_HASHES 의 그 파일(내용은 기준의 것과 같을 것이다 [I]).
  - (마) D4: 생성된 `cfe_build_env_table.c` 의 빌드 날짜·사용자·호스트 글. FILE_HASHES 의 그 파일, 그리고 그 파일을 이름 대는 facts 레코드.
  - (바) 모든 절대 경로 글: 맵으로 되돌린 뒤 비교한다. 되돌린 뒤 남는 파생 경로는 (나)의 레코드(플러그인의 out 디렉터리와 manifest 경로)에만 있어야 한다.
  - (사) 정체성 칸: 컨테이너 머리의 `basis_sha256`, `artifact_sha256`, `container.size`, `parent[0]`; 절 줄의 crc32(레코드는 하나씩 비교함)와 레코드가 더해지는 절(STRINGS, BASIS, FILE_HASHES, DEVIATIONS)의 count; facts BASIS 의 `extract.units_from`; `basis.txt`·`extract.txt`·`relations.txt` 의 해시 줄; `sha256.txt` 의 basis·facts·compile db 줄; `args.txt` 의 파생 작업 공간 줄.
  - 그 밖은 모두 같아야 한다: facts 의 STRINGS·BASIS·FILE_HASHES 밖의 절 36개(PROVENANCE, FACTS, FUNCS, READS, RETURNS, … LOCK_PATHS)는 맵 뒤 레코드가 차례까지 같고((마)의 레코드 밖), basis 의 PROVENANCE 도 같고, `relations.txt` 는 둘째 줄 밖에서 같다.
  - B′ 의 성질로 적는 관측 효과(차이가 아니라 B′ 의 속성): site 가 있는 단위, 실행 파일마다 hook 호출 수(bincheck), 링크된 런타임, wrapper 없음(이 주기에는 생성 토큰이 없다).
- **시험**: 바인딩 보고(모든 site 가 bound 이거나 이유가 있는 unsupported/removed; not-seen 0), `hookcheck` rc 0(계획의 목록), module id = 계획의 표, 점검한 실행 파일 = 계획의 목록(맵으로 옮긴 7개). 결과는 측정으로 적는다. 바인딩 수나 site 의 상태를 미리 기대하지 않는다.
- **누적 바이트 동일성**: 뒤에 `ledger_check.sh` 전체(F1T, `641/b`, 플러그인 둘, kairos-build 도구, W6·C5 기록 묶음과 앞 단계의 출력 포함)와 `tools_check.sh --deep`. 이 단계의 새 출력은 장부 행으로 더한다. 바깥 이름 점검을 새 출력 디렉터리에 돌린다.
- **지우는 것**: `derive_c1.sh` 가 같은 자리 기준 빌드 트리를 해시 뒤 지운다(다시 만들 수 있고 그 객체 해시는 남는다). 그 밖에 아무것도 지우지 않는다. 계획은 B 다시 뽑기(`plangen_bref`)를 이 비교 뒤 지운다고 했으나, 이 단계가 만든 scratch 가 아니므로 남긴다(지울지는 lead 가 정한다).

### 5.1 바뀐 것 [F]

- 커밋 `a79aa6d`(선언과 스크립트, 파생 빌드 전). 규모(`git diff --stat 21525ee a79aa6d`, 스크립트 여덟): `path_map.sh` 51줄(새), `compare_derived.py` 278줄(새), `plan_bindings.py` 158줄(새), `run_unit.sh` +128, `derive_args.sh` 46줄 바뀜, `derive_c1.sh` 37, `one_build.sh` 26, `derive_plan.sh` 24; 합 708줄 더함, 40줄 뺌.
- 파생 빌드 뒤 비교기의 읽기 결함 둘을 고쳤다(이 절의 커밋; §5.3, §5.8): (1) facts 의 `tu=` 는 FILE_HASHES 의 번호다(`kairos/lib/llvm/serialize.cpp:175`, `:198`, `:314` `r.tu = fh_of(...)`). 처음 비교기는 `fh=` 만 경로로 바꿨다. (2) 따옴표 값 안의 `\"`(TUS 의 `flags`)를 `only=` 가 한 칸으로 읽지 못했다. 둘 다 판정 기준을 넓히지 않는다. 고친 뒤 `run_unit.sh` 12/12 [M].
- 선언 목록의 덧붙임 `records/obs1-20261116-derived/declared_diff_after.tsv`(원래 목록 + 9줄, 첫 비교를 본 뒤; §5.3). 원래 목록 `declared_diff.tsv` 는 바꾸지 않았다.

### 5.2 빌드 [M]

공간 감시 아래, 모두 rc 0(`commands.txt`, `df_*.log`; 작업 공간의 작은 기록 사본은 `w/`).

| 단계 (UTC) | 결과 |
|---|---|
| 런타임 트리 07:37:43–47 | `/home/user/work/redesign/obs1/c1pre_rt/rt`; backend "kzgcc 0.3 (gcc-plugin)", manifest `5f015c5a…`. `libkzrt.a` `718acdfd…`, `libkzrt-posix.a` `d0fbd6a4…`, `kz_rt_posix_auto.c.o` `4edb90c6…` 는 F1T `rt-pre` 와 같은 바이트, `kz_rt_build_info.c.o` `d4995fa5…`(manifest 가 다름), `kz-rt-dump` `eb12d09e…`(F1T `21550a70…`, MC `4390d4f1…` 와 다름; `kairos/runtime` 의 마지막 변경 `b73618d` 은 manifest 파서) |
| prepare 07:39:23–25 | 복사본과 기준의 차이: bundle 0, `pre/src` 0, `bpxfer_defs` 는 D2 의 두 파일만. 맵 `path_map.tsv` `0cbb50e7…`: `c1_build/bundle_1df262a/` → `<W>/bundle_1df262a/`, `c1_build/pre/src/` → `<W>/src/`, `c1_build/pre/1df262a/` → `<W>/ws/` |
| 같은 자리 기준 빌드 07:39:37–51 | 객체 261. 계획의 일곱 파일 가운데 bp.so, cf.so, ci_lab.so, sch_lab.so, to_lab.so 는 원래 기준과 같은 바이트, core-tx `50305b10…`(원래 `0052e7ab…`; D4 와 경로)와 bplib.so `f16c177c…`(원래 `1766ee36…`; 경로 글)는 다르다. 해시 뒤 트리를 지웠다(스크립트의 동작) |
| 계측 빌드 07:40:06–24 | 플러그인 옵션 = 표의 module 여섯 줄과 기본을 맵으로 옮긴 것(`module_flags.txt`: `derive_module_flags` 의 출력이 `build_instr.record` D3 줄에 그대로 있음). core-tx `c2aa181b…`, bp.so `6789ff75…`, bplib.so `f16c177c…`(같은 자리 기준과 같음), cf.so `f5af74f7…`, ci_lab.so `d0c9adbe…`, sch_lab.so `9a6ae3c9…`, to_lab.so `ef83f6c2…`. 컴파일 경고 1(기준 빌드와 같은 수) |
| check 07:40:31 | 객체 261 가운데 다른 것 35: 계측 단위 33(바인딩 파일 33개와 같음), D4 의 `cfe_build_env_table.c.o` 둘(tx, rx). UNEXPECTED 0, 바뀌지 않은 계측 단위 0 |
| B′ basis·facts 07:40:55–07:41:21 | `one_build.sh c1_pre_tx /home/user/work/redesign/obs1/c1pre_bprime --path-map <W>/path_map.tsv`, kz-static `de1315b3`. basis `a57c227b2f9fd8dd…`, facts `6451f233ce73fef9…`, relations `03f1da462f576db6…`. basis 의 `binary.sha256` = 계측 core-tx `c2aa181b…`, `binary.build_id` `ca1ed0b1…` |

- MC 는 바뀌지 않았다: `pre_mc.txt` 와 `post_mc.txt` 가 도구 일곱, 정체성, 정의, `CMakeCache.txt`, flags.make 목록에서 같다. 다른 줄은 HEAD(`21525ee` → `a79aa6d`)와 `kairos` 미커밋 줄 수(앞 기록은 선언 커밋 전에 적어 스크립트 232줄이 미커밋이었다)뿐이다.
- B′ 의 성질(관측 효과): site 가 있는 단위 33(module 별: core-tx 23, cf 4, bp 3, ci_lab 1, sch_lab 1, to_lab 1; bplib 에는 site 가 없다), 링크된 hook 호출 315(core-tx 284, cf.so 21, bp.so 5, sch_lab.so 3, ci_lab.so 1, to_lab.so 1; `w/bincheck_basis.txt`), 링크된 런타임(basis 의 `link.inputs` 27 → 31, 아카이브 20 → 22), 생성 토큰 wrapper 없음. 설치 파일 31 가운데 원래 기준과 같은 바이트 15(표·시작 스크립트·host 도구·tinycbor.so), 다른 16(`w/install_inventory.tsv`).
- GNU build-id 와 sha256(설치된 ELF 21개, `exe_buildids.tsv`): core-tx `ca1ed0b12f7c8941…`, core-rx `2d028ce2cb972313…`, cf.so `785444d2…`, bp.so `098b4e62…`(rx 와 tx 의 같은 이름 모듈은 같은 파일).

### 5.3 시험 [M]

- 바인딩 보고(`bindings_report.tsv`, `plan_bindings.py`): 계획의 site 301 모두 바인딩이 있다. bound 293, duplicated 6, inlined 2, unsupported 0, removed 0, not-seen 0. 단위 헤더 33개 모두 계획의 manifest sha256. module: manifest 의 module 문 = 표(이름, id, 기본 5 = core-tx), 옵션 = 표의 차례와 기본, 단위마다 id = 플러그인 규칙(`kzgcc.cpp` `start_unit`). duplicated 여섯은 `CFE_ES_InitializeFileSystems` 의 creator-call 둘(site 24, 26), `CFE_SB_BackgroundFileEventHandler` 의 use-call 둘(150, 153), `CFE_TBL_HousekeepingCmd` 의 creator-call 둘(262, 263), inlined 둘은 BP `initialize_throttling` 의 creator-call(279, 280: `bp_flow.c:132` 의 at-call·after-call).
- 관계 1 의 행(사용 지점 키 `17975444d83f6589`)을 `ref` 로 가진 site 12개: 모두 바인딩됨(creator-call 2·3 SCH_LAB, 279·280 BP(inlined), key-text 272·273, handling-chain 284–287, 288(handling-chain 과 use-call 이 합쳐진 site), use-call 289). 공유하는 행위자·멈춤 사건 site 22개도 모두 bound.
- `hookcheck`(계획의 목록 일곱 파일, 맵으로 옮김): rc 0, ok 301, missing·gap-called·undeclared 0. 점검한 실행 파일 = 계획의 목록(`install_inventory.tsv` 의 "in the plan's list" 7).
- rx(`rx_check.txt`): `exe/rx` 의 ELF 9개, hook 호출 315, (함수, hook, id) 목록이 계획의 그룹과 같다. rx 의 모듈 여덟은 tx 의 같은 이름 파일과 같은 바이트다(같은 arch 객체에서 링크; core-rx 만 target 마다 따로 링크).
- B 와 B′ 비교(`compare_derived.py`, B = `plangen_bref/c1_pre_tx/NP`):
  - 첫 실행(요약 `compare_run1_summary.txt`): 선언 밖 3,650. 거의 모두(FACTS 2,721, FUNCS 860, FACT_CONSTS 37)가 `tu=` 번호의 차이였다. facts 의 `tu=` 는 FILE_HASHES 의 번호이고(§5.1) facts 의 FILE_HASHES 는 경로 차례다. 파생 작업 공간에서는 `<W>/src` 가 `<W>/ws/build` 앞에 와(기준에서는 `pre/1df262a/build` 가 `pre/src` 앞) 생성된 단위 여덟만큼 번호가 밀렸다. TUS 여섯은 `flags` 안의 `\"` 를 읽지 못한 것이다. 비교기를 고쳤다(§5.1).
  - 고친 비교기, 원래 목록(요약 `compare_run2_summary.txt`, 선언 밖 레코드 모두): 선언 밖 26. 모두 선언한 갈래(플러그인 옵션, 링크된 런타임, D1 경로)의 결과인데 목록이 이름 대지 않은 레코드다: module 여섯의 `link.txt`(CMake 의 공유 모듈 링크 규칙이 `CMAKE_C_FLAGS` 를 싣는다; `apps/bp` 의 diff 에서 `-fplugin` 확인), FILE_HASHES 의 링크된 파일 일곱(core-tx 와 계측 모듈 다섯: `binary.sha256`, `link.output_is_the_loaded_file` 와 같은 파일; bplib.so 는 같은 자리 기준 빌드와 같은 바이트이므로 D1 경로 글), `closure.name_binding` 의 교차 참조 수 195 → 205(BASIS 와 `basis.txt`), facts BASIS 의 `extract.basis_deviation[...]` 넷(facts 가 basis 의 편차를 옮김)과 `relations.txt` 13행의 그 수(2 → 6), `basis.txt` 의 "objects without a compile command: 5" 와 런타임 객체 다섯의 경고 줄.
  - 덧붙인 목록(`declared_diff_after.tsv`, `compare_after.tsv`): 선언 밖 0, rc 0. TUS 177 개가 플러그인 옵션으로 달라졌고 이는 `instrumentation.units_with_compiler_plugin` "177 of 177 units, 1 distinct option set(s)" 와 같은 수다.
  - 맵의 내용 확인: B′ 의 FILE_HASHES 195 가운데 194 가 맵 뒤 B 의 경로와 같고 그중 178 은 sha256 도 같다. 다른 16 = `compile_commands.json`, `link.txt` 일곱, `cfe_build_env_table.c`(D4), 링크된 파일 일곱. 더해진 1 = 플러그인 파일. D2 의 `cfe_module_version_table.c` 는 내용까지 같다. 맵 뒤 남은 파생 경로 0(FILE_HASHES).
  - facts: 39 절 가운데 레코드가 차례까지 같은 절 36(레코드를 찍지 않는 STRINGS 포함). 다른 셋: BASIS(basis 항목과 편차의 사본, `extract.units_from`), FILE_HASHES(위의 파일들; 차례는 경로 차례라 다름), GLOBAL_IMAGES 의 레코드 하나(`cfe_build_env_table.c` 의 `CFE_BUILD_ENV_TABLE[0].Value`, D4). FACTS, FUNCS, READS, RETURNS, LOCKSET, LOCK_PATHS 를 비롯한 그 밖의 절은 모두 같다. `relations.txt` 는 둘째 줄(facts 해시)과 13행(편차 수) 밖에서 같다. `extract.txt` 는 첫 줄 밖에서 같다.
  - basis: PROVENANCE 같음. BASIS 의 다른 항목은 모두 선언 갈래. DEVIATIONS 에 플러그인 편차 하나가 더해졌다.
- 단위 점검: 선언 전 `run_unit.sh` 12/12, `step2a_tests_unit.sh` 7/7. 비교기를 고친 뒤 `run_unit.sh` 12/12.
- MC `ctest -R '^lint'` 7/7(빌드 없이 kz_lint `5f62ec20`). Kairos 도구를 빌드하지 않았으므로 전체 ctest 는 돌리지 않았다.

### 5.4 바이트 동일성 [M]

- 누적 목록: `ledger_check.sh` 40행, 실패 0(새 행 셋: `OBS_c1pre_derived` 3,957 파일, `OBS_c1pre_rt` 355 파일, `OBS_c1pre_bprime` 10 파일). F1T(16,444 파일), C5 `641/b`, 플러그인 둘, kairos-build 도구, W6·C5 기록 묶음, 앞 단계의 출력과 도구 사본 모두 같다.
- 도구 등록부: `tools_check.sh --deep` 31행, 실패 0. 새 도구 행 없음(런타임 트리의 kz-rt-dump `eb12d09e` 는 장부 행 `OBS_c1pre_rt` 의 파일로 지킨다).
- 바깥 이름 점검(`--tree`): 런타임 트리, B′ 출력, 기록 디렉터리는 깨끗하다. 파생 작업 공간에서는 upstream cFE 소스 사본(`cfe/modules/sb/fsw/src/cfe_sb_priv.c` 의 지역 변수 이름 하나와 그 쓰임 셋)과 문서 SVG 한 줄이 패턴에 걸린다. 둘 다 기준의 소스를 바이트 그대로 복사한 것이고(prepare 의 diff 0) 이 작업이 쓴 글이 아니다. 대상 소스를 바꾸는 것은 PI 의 선택이므로 손대지 않았다.

### 5.5 rx 인스턴스 [D][M]

- rx 는 같은 파생 트리다(까닭은 §5.0, 측정은 §5.3). 부팅 하네스의 tx 기록은 `<W>/ws/exe` 전체를 묶으므로 다음 부팅의 두 인스턴스 절차는 이 트리의 `exe/rx/core-rx`(build-id `2d028ce2…`)와 tx 와 같은 모듈 파일을 쓴다. rx 는 `KZ_RT_MODE=off` 로 돈다(`run_gi_c1.sh:68`).

### 5.6 공간 [M]

| 시점 (UTC) | 여유 (df -B1 /) | 일 |
|---|---|---|
| 07:37:09 | 3,789,340,672 | 선언 커밋 전 |
| 07:37:43–47 | → 3,782,864,896 | 런타임 트리(dip 6,467,584) |
| 07:39:23–25 | → 3,755,446,272 | prepare(dip 27,279,360) |
| 07:39:37–51 | 가장 낮은 점 3,741,085,696 | 같은 자리 기준 빌드(dip 14,348,288, 변화 −94,208: 해시 뒤 트리를 지움) |
| 07:40:06–24 | → 3,736,080,384 | 계측 빌드(dip 19,234,816) |
| 07:40:55–07:41:21 | → 3,722,616,832 | B′ basis·facts(dip 188,416 / 11,059,200 / 1,863,680) |
| 07:46:44 | 3,682,033,664 | 비교·기록 뒤(이 세션 밖의 흐름 포함 [I: 파일별로 재지 않음]) |

- 하한을 넘지 않았다. 감시 문턱(하한 + 필요량 100,000,000 = 414,572,800) 아래로 가지 않았다. 남긴 크기(블록): 파생 작업 공간 46,743,552 B(겉보기 33,032,031 B; F1T `c1/pre_1df262a` 46,329,856 B 와 비슷), 런타임 트리 6,463,488 B, B′ 13,135,872 B.
- 지운 것: `derive_c1.sh` 가 같은 자리 기준 빌드 트리를 해시 뒤 지웠다(다시 만들 수 있고 객체 해시 `objects_ref.sha256` 이 남는다). 기록 디렉터리에 잠깐 복사한 바인딩 파일 사본 하나(원본은 작업 공간에 있고 장부가 지킨다), 앞의 두 비교의 전체 출력(요약을 남김, §5.8). 그 밖에 아무것도 지우지 않았다.

### 5.7 다음 단계로 넘기는 것

- 부팅에 쓸 kz-rt-dump: 이 런타임 트리의 `eb12d09e` 와 MC 의 `4390d4f1` 은 다른 파일이다. 어느 것을 쓰는지는 부팅 선언에서 정하고 적는다.
- `run_gi_c1.sh` 의 run.meta `file` 줄은 고정 목록(core, cf.so, bp.so)이고 인스턴스의 모든 파일은 `inst_files.sha256` 에 있다. 이 빌드의 `build_instr.record` 해시 줄은 계획의 일곱 파일(tx)이다. 사슬 점검이 rx 파일을 빌드 기록에서 찾는다면 맞지 않는다 [미확인]: 부팅 전에 `chain_check.py` 의 입력을 확인한다.
- 바인딩 파일 `<W>/binds_instr.kzs` `6a22b9ec9fe5dfcf…`(802 줄)가 dump 를 읽을 때의 두 번째 manifest 다.
- 계획의 재생성 점검(생성기 사본)은 부팅 전에 다시 한다(§4.4 의 것은 07:08Z).
- C5 의 파생 빌드는 아직 맵을 쓰지 않는다: `derive_c5.sh prepare` 는 맵을 쓰지 않고 `derive_plan.sh c5` 는 맵을 요구한다. C5 의 계획 모드는 그 맵의 출처(기준 `keep/` 과 파생 `b/` 의 짝)를 정한 뒤에 쓸 수 있다.
- 교차 참조 수 195 → 205 의 열 개가 어느 이름인지 [미확인](basis 는 수만 적는다). 런타임 객체가 링크 멤버로 들어온 결과로 보인다 [I].

### 5.8 계획과 다르게 한 것

- 계획은 module 표와 실행 파일 목록을 인자로 넘기라고만 했다. 기준 빌드의 절대 경로를 옮기는 맵을 새 인자(`--path-map`)로 더했고, 맵은 `derive_c1.sh prepare` 가 자기 복사본에서 쓴다. `derive_plan.sh` 는 맵과, 실행 파일 목록·manifest 가 같은 sidecar 를 가리키는지도 본다.
- 계획은 "FILE_HASHES 내용으로 경로를 짝짓는다"고 했다. 비교기는 작업 공간의 맵을 거꾸로 쓰고 FILE_HASHES 의 내용으로 그 맵을 확인한다(194 경로, 178 같은 sha256, 다른 16 은 모두 선언 갈래). 내용만으로 접두어를 거꾸로 구하면 같은 파일 짝의 공통 꼬리를 떼는 방식이라 `<W>` 하나가 `c1_build` 와 `c1_build/pre` 둘로 갈려 하나로 정해지지 않는다.
- 비교기의 읽기 결함 둘(facts `tu=`, 따옴표 안의 `\"`)을 첫 비교 뒤 고쳤다(§5.1). 선언 목록이 이름 대지 않은 레코드 26개는 덧붙인 목록으로 이름 댔다(§5.3). 원래 목록과 두 번의 비교 결과를 요약으로 남겼다(`compare_run1_summary.txt`, `compare_run2_summary.txt`: 절·갈래별 수, 첫 실행은 절마다 세 줄, 둘째는 선언 밖 26줄 모두; 전체 출력 3,389,314 B 와 817,599 B 는 sha256 을 요약에 적고 지웠다. 같은 비교기와 목록으로 다시 만들 수 있다). 덧붙인 줄은 모두 선언의 갈래 안이고 새 갈래가 아니다. 선언이 레코드 이름을 빠뜨린 것은 선언 글의 빈틈이다(§4.8 의 B 다시 뽑기와 같은 종류).
- 계획은 B 다시 뽑기(`plangen_bref`)를 이 비교 뒤 지운다고 했다. 이 단계가 만든 scratch 가 아니므로 남겼다(다시 만들 수 있다: 생성기 사본과 `one_build.sh`; 지울지는 lead 가 정한다).
- 기록 사본 `w/` 에는 바인딩 파일을 넣지 않았다(약 100 KB; 원본은 장부가 지킨다).
- 플러그인이 생성된 manifest 를 읽는지는 빌드 전에 scratchpad 의 작은 단위 하나로 봤다(rc 0). 그 출력은 근거가 아니다.

## 6. 생성된 계획으로 만든 B′ 의 자연 부팅 하나, 변환, 사슬 (Kairos 도구 빌드 없음)

기록: `../records/obs1-20261116-boot/`. 출력: 저장소 밖 새 디렉터리 둘, `/home/user/work/redesign/obs1/c1pre_boot/`(부팅 기록; 그 아래 `tx/` 가 하네스의 출력 디렉터리), `/home/user/work/redesign/obs1/c1pre_convert/`(run.kzt 와 그 글 보기, MC kz-rt-dump 의 글). 파생 작업 공간(`c1pre_derived`), 런타임 트리(`c1pre_rt`), B′(`c1pre_bprime`), 계획(`plangen/c1_pre_tx`)은 읽기만 한다. F1T, W6 기록, T8CF, 앞 단계의 출력 디렉터리에는 쓰지 않는다. push 하지 않는다.

### 6.0 선언 (부팅 전에 적고 커밋함) [D]

부팅 전에 적었다. 아래 밖의 차이는 결함이다. 결함이면 다음으로 가기 전에 멈추고 까닭을 적는다. 부팅은 한 번이다. 부팅이 계획 밖의 이유로 실패하면 그 기록을 남기고 까닭을 적으며, 까닭을 적지 않고 다시 부팅하지 않는다. 다시 부팅하더라도 새 디렉터리이고 첫 기록을 대신하지 않는다.

- **Kairos 도구**: 다시 빌드하지 않는다. MC 의 도구 일곱(kz-static `de1315b3`, kz `931dd39c`, kz-dump `05623054`, kz-rt-dump `4390d4f1`, kz-phi `7d34e6e2`, kz-instr-clang `28db4017`, kz_lint `5f62ec20`), 플러그인 둘(`56a96b79`, `27a49a6a`), kairos-build 도구는 이 단계 앞뒤에 같아야 한다(`pre_mc.txt` 는 적었다: §5 의 `post_mc.txt` 와 HEAD 줄 밖에서 같다 [M]; 끝에 `post_mc.txt`, `tools_check.sh --deep`). 정체성이 바뀌는 도구: 없음.
- **쓰는 도구**:
  - 부팅 하네스 `kairos/instrument/gcc/scripts/run_gi_c1.sh`(바꾸지 않는다).
  - 하네스 안의 dump 판독기: 런타임 트리의 `kz-rt-dump` `eb12d09e…`(`/home/user/work/redesign/obs1/c1pre_rt/rt/kz-rt-dump`). 까닭: 앞선 C1 G-I 자연 부팅은 부팅한 빌드가 링크한 런타임 트리의 kz-rt-dump 를 넘겼다(`env/campaigns/w6_20261029.sh:51`, `f1/rt-<cfg>/kz-rt-dump`). 이 판독기는 기록을 쓴 `libkzrt.a`(`718acdfd…`)와 같은 트리에서 같은 소스로 만들어졌다(`records/obs1-20261116-derived/runtime.txt`). 부팅 뒤 같은 dump 와 같은 manifest 둘로 등록부의 MC `kz-rt-dump` `4390d4f1` 도 돌려 두 글을 바이트로 비교하고 결과를 적는다(`kairos/runtime` 소스의 마지막 변경 `b73618d` 는 두 빌드보다 앞이다). 어느 한쪽 글이 다른 쪽을 대신하지 않는다. 다르면 차이를 적고, 다음 단계는 그 차이를 안고 읽는다.
  - 변환: MC `kz` `931dd39c` 의 `kz convert --kzrt <dump> --manifest <plan.kzs> --manifest <W>/binds_instr.kzs --basis <B′>/basis.kz --out <새 디렉터리>/run.kzt`(앞선 G-I 재생의 꼴, `kairos/instrument/gcc/scripts/replay_gi.sh:39`; dump 에 RUNX process 가 있으면 `--process` 는 주지 않는다). run.kzt 의 글 보기는 MC `kz-dump` `05623054`.
- **스크립트**(이 선언과 함께 커밋; `env/` 는 `kairos` 밖이라 어떤 도구의 정체성에도 들지 않는다):
  - `env/tools/chain_check.py`: 선택 연결을 더했다. 인자가 주어질 때만 나오고, 앞의 연결과 판정 규칙(하나라도 refused 면 refused, 하나라도 unconfirmed 면 unconfirmed)은 그대로다. `sidecar`(계획 머리의 `# sidecar sha256=` = sidecar 파일), `plan_inputs`(sidecar 의 `input facts`·`input basis` sha256 = 계획을 만든 T8CF facts·basis 파일), `inst_list`(run.meta 의 `instance_files … list_sha256=` = 목록 파일), `instance`(목록의 모든 파일 = 인스턴스를 풀어낸 트리 `<W>/ws/exe` 의 같은 자리 파일), `exe` 의 두 번째 출처(목록 안의 같은 경로; run.meta 의 `file` 줄과 둘 다 있으면 둘이 같아야 함), `bprime`(B′ facts 의 부모 basis = B′ basis 파일 = run.kzt 의 부모), `dump`(run.meta 의 `dump … sha256=` = dump 파일 = run.kzt 의 `convert.dump.sha256`), `bindings`(run.meta 의 `bindings … sha256=` = 파일 = run.kzt 의 FILE_HASHES), 그리고 manifest 연결에 run.kzt 의 NRUN `manifest_sha256` 과 FILE_HASHES 의 계획 경로. 까닭: 이 빌드의 `build_instr.record` 는 계획의 일곱 파일(tx)을 적고 run.meta 의 `file` 줄은 고정된 셋(core, cf.so, bp.so)이라(`run_gi_c1.sh:52`) 앞의 checker 로는 bplib.so·ci_lab.so·sch_lab.so·to_lab.so 가 unconfirmed 로 남는다. 인스턴스의 모든 파일은 run.meta 가 sha256 으로 이름 대는 목록(`run_gi_c1.sh:57`, `env/tools/hash_instance.sh`)에 있으므로 그 목록을 둘째 출처로 쓴다. 사례 값은 없다.
  - `env/obs1/stop_eval.py`(새, `python3 -I`): 멈춤 사건을 부팅 뒤 기록에서 평가한다(아래 예산 1). 계획의 sidecar `stop-event` 줄(API), manifest 의 stop-event·actor-root site, 인스턴스의 시작 스크립트(`CFE_APP` 줄; `CFE_LIB` 는 라이브러리로 따로), dump 글을 읽는다. 앱마다: root entry 를 기록한 행위자, 그 뒤 그 행위자의 첫 stop-event site 사건, 그 시각(dump 의 시계, t_init 뒤 ms), 창(RUN 줄의 t_init–t_stop) 안인지. 상태는 reached / not-reached / unresolved(사유) 만 적고 정상·실패를 판정하지 않는다.
  - `env/tools/tests/run_unit.sh`: 점검 13(chain checker 의 새 연결: 일관된 stub 사슬 confirmed; 다른 sidecar, 바뀐 인스턴스 파일, run.meta 가 이름 대지 않은 목록, 다른 부모 basis 의 run container, 다른 dump → refused)과 14(멈춤 사건 평가: 도달 → rc 0; stop site 에 닿지 않는 행위자, 계획에 root site 가 없는 앱 → rc 3; RUN 줄 없음 → rc 2). 선언 전에 14/14 [M], `env/redesign/step2a_tests_unit.sh` 7/7 [M].
  - `records/obs1-20261116-boot/run_boot.sh`: 단계(boot, control, rtdump-mc, convert, kzt-text, chain, stop, conditions). boot 와 convert 는 공간 감시 아래.
- **재생성 점검**(부팅 전, 이 선언 전에 함) [M]: `env/tools/plan_regen.sh /home/user/work/preserve/tools-20261116-c/kz-static.de1315b3.xz /home/user/work/redesign/obs1/plangen/c1_pre_tx/args.txt <scratchpad>` (07:58Z, 공간 감시 `--need 250000000`): 사본을 풀면 `de1315b3…`, manifest `5f015c5a…`·sidecar `8f5f200c…` 바이트 동일, module 표·실행 파일 목록은 첫 줄의 sidecar 경로 밖에서 동일, PASS(`regen_c1_pre_tx.txt`, `df_regen.log`: dip 184,422,400 B). 풀어 둔 scratchpad 디렉터리는 점검 뒤 지웠다(사본의 xz 는 남음).
- **무엇을 돌리는가**: `run_gi_c1.sh /home/user/work/redesign/obs1/c1pre_derived pre 1df262a tx /home/user/work/redesign/obs1/c1pre_boot/tx <런타임 트리 kz-rt-dump> /home/user/work/redesign/obs1/plangen/c1_pre_tx/plan.kzs`. 두 인스턴스 기준 절차: 설치 트리 `<W>/ws/exe` 를 묶어 새 디렉터리에 풀고(`run_gi_c1.sh:45-49`), tx 와 rx 를 ubuntu 로 배경에서 시작(`:79-80`; cFS README L107), `sleep 5`(`:82`; CI L176-L177), OPERATIONAL 확인(`:83`), ES 재시작 POWERON 명령을 UDP 1234/1235 로(`:85-89`; CI L196-L197), `sleep 5`(`:90`; CI L199-L200), 살아 있는 프로세스에 SIGKILL(`:91`). CI 는 `src/bp/.github/workflows/build-run-bp-common.yml`(sha256 `b156bbf2…`, nasa/bp b051eb0 의 사본과 같음). tx 만 기록한다(`KZ_RT_MODE=record`), rx 는 `KZ_RT_MODE=off`. rx 혼자의 경로(`sleep 8`, `:13-15`, `:97`)는 쓰지 않는다: 그 8 s 는 기준 절차가 아니라 결함의 재시도 시간에서 골랐다(`kairos/probe/scripts/observe_c1_hold.sh:51-54` 의 주석 "8 s covers the post-fix retry window (nominal ~2.5 s) and settling" 과 `sleep 8`).
- **관측만**: 계획에 control·hold·gate site 가 없다(생성된 manifest 의 control·identity·bind 문 0, `records/obs1-20261116-plangen/parse_check.txt`). 런타임 계획 파일을 주지 않고 `KZ_RT_MODE=record` 다(제어 경로 꺼짐). 부팅 뒤 dump 글에 control 레코드(`ctl`, `plan`, `point` 줄; `kairos/runtime/tools/kz-rt-dump/kz_rt_dump.cpp` 가 이 셋을 찍는다)가 0 인지 본다(`control_check.txt`). 0 이 아니면 결함이다.
- **부팅 전에 적는 예산**(값, 까닭, 발동 여부, 잘림, 미해결, 민감도; 발동 여부와 결과는 부팅 뒤 §6 의 뒤 절에 적는다):
  1. **멈춤 사건(규칙)**: 인스턴스 시작 스크립트의 모든 앱이 첫 run-loop 호출이나 종료에 이른다. run-loop 와 종료 API 는 프레임워크 계층 자료(`kairos/semantics/framework_api_roles.txt` 15–16행, sha256 `e2a39203…`): `CFE_ES_RunLoop` `cfe/modules/es/fsw/src/cfe_es_api.c:470`(선언 `cfe_es.h:397`), `CFE_ES_ExitApp` `cfe_es_api.c:340`(선언 `cfe_es.h:355`), 판 "cFE 6.7.99 (baseline v7.0.0-rc4, build 205)". 계획의 site: 6(ExitApp entry), 7(RunLoop entry); 앱의 root site: 1(CI_Lab_AppMain), 4(SCH_Lab_AppMain), 5(TO_LAB_AppMain), 276(BP_AppMain), 281(CF_AppMain). 하네스는 사건에서 멈출 수 없다(고정 대기; 기록 풀은 프로세스 끝의 atexit 처리기가 쓴다, `kairos/runtime/src/kz_rt_posix_auto.c:13`, `:126`, `:164`). 그래서 사건은 부팅 뒤 기록에서 `stop_eval.py` 로 평가한다: 앱마다 창 안에서 도달했는지, 언제. 이것은 lead 의 판단이다(근거는 아래 2).
  2. **도구의 멈춤 예산**: 값 = 기준 절차의 대기, 5 s(CI L176-L177 "Wait for Startup") + 명령(CI L196-L197 "Terminate cFS": ES 재시작 POWERON) + 5 s(CI L199-L200 "Wait for Shutdown"), 그 뒤 하네스의 SIGKILL(`run_gi_c1.sh:91`, CI 에는 없는 하네스의 단계). 까닭(lead 판단과 근거): 이 부팅은 자연 부팅이고 그 상한은 기준 절차(CI 의 위 줄들, 그것을 옮긴 `kairos/probe/scripts/observe_c1_smoke.sh --instances` 의 단계, `run_gi_c1.sh:9-12`)의 대기다. 값은 정해진 예산 원칙을 따른다: PI 결정 (결정 검토서 2026-10-30; 2026-11-16 재확인. 원문은 이 작업 환경에 없어 대조하지 못함). 이 부팅에는 hold, gate, control site 가 없다. 발동 여부: 예산(명령과 두 대기)은 언제나 창을 끝낸다. 그것이 어느 앱의 멈춤 사건보다 먼저 왔는지를 부팅 뒤 적는다. 잘림: 창 뒤의 일(명령 뒤 프로세스가 끝날 때까지의 사건은 기록되지만, 프로세스 끝 뒤와 SIGKILL 뒤는 기록되지 않는다; SIGKILL 로 끝난 프로세스는 atexit 처리기를 돌지 않으므로 그 인스턴스의 dump 가 없다), CI 의 L179-L191(흐름 켜기, Class 1 전송, `sleep 30`)은 기준 절차에 없으므로 이 기록 밖이다; 이것들은 다음 단계의 `effect.unobserved` 로 간다. 미해결: 멈춤 사건에 이르지 않은 앱(있으면 이름). 민감도: 재지 않음(부팅 하나). 예산의 멈춤은 실패로도 정상으로도 바꾸지 않는다.
  3. **프로그램의 시간 제한(도구 예산과 따로)**: `CFE_PLATFORM_ES_STARTUP_SCRIPT_TIMEOUT_MSEC` 1000(`<W>/ws/bpxfer_defs/default_platform_cfg.h:1731`; 쓰임 `cfe/modules/es/fsw/src/cfe_es_apps.c:505` 앱 task 마다 entry point 대기, `cfe_es_start.c:208` LATE_INIT 동기화, `:223` RUNNING 동기화), `CFE_PLATFORM_ES_STARTUP_SYNC_POLL_MSEC` 50(`:1689`), `CFE_PLATFORM_CORE_MAX_STARTUP_MSEC` 30000(`:1713`; `cfe_es_start.c:811`). 파생 트리의 이 파일은 기준 빌드의 것과 바이트로 같다 [M: `cmp`]. 표는 `records/obs1-20261116-ground/readings/startup_tables.txt` 2절. 창이 그것들을 덮었는지는 부팅 뒤 console 의 "Startup Sync failed"(`cfe_es_start.c:210`, `:225`)·"Unable to obtain Entry Point"·OPERATIONAL 줄과 dump 의 시각으로 적는다.
  4. **실행 예산**: 부팅 하나. 까닭: 설명 시험에는 기록 하나면 되고, 반복은 뒤의 비용 연구다. 두 번째 부팅은 까닭과 함께 적고 첫 기록을 대신하지 않는다. 민감도: 재지 않음.
  5. **계획 예산**: `tier:D07`(방법 우선순위의 층 1 전체, c1_pre_tx 에서 12행)과 행위자·멈춤 사건 site. 까닭(sidecar 12행 그대로): 기록 풀이 고정이고(`KZ_RT_POOL_BYTES = KZ_RT_MAX_ACTORS × 65536`, `kairos/runtime/include/kz_rt.h:61-64`) 생성된 계획의 사건 양은 아직 재지 않았다. 잘림: 아래 층의 419행(sidecar 의 `need row … budget_not_run` 줄, 키로 나열). 미해결: 다음 층이 풀에 들어가는지. 민감도: 재지 않음. 밝힘: 예산 값은 층 1 이 이 빌드에서 어떤 행을 갖는지(A21 이 층 1 임) 알고 골랐다(sidecar 의 `disclosure`).
  6. **한계**: site 마다 문자열 한계(manifest 의 값과 출처: 조회 API `osapi-countsem.c:212` 앞 `arg1` bound 64, 생성 API `osapi-countsem.c:91` 앞 bound 20; 런타임의 복사 한계 `KZ_RT_STR_MAX` 64, `kz_rt.h:82`; 값마다 잘림 표시), 사건당 값 4 slot(`kzgcc.cpp` build_obs), `KZ_RT_MAX_ACTORS` 64(`kz_rt.h:61`), 풀 4,194,304 B(64 × 65536), chunk 4096 B(`kz_rt.h:67`). 발동하면 셈이 남는다(ACTOR 의 overflow·dropped_bytes, RUNX 의 pool_dropped, LOSS). 부팅 뒤 셈을 적는다.
  7. **B′ 의 관측 효과**(원래 바이너리의 성질이 아님, §5.2): site 가 있는 단위 33, 링크된 hook 호출 315(core-tx 284, cf.so 21, bp.so 5, sch_lab.so 3, ci_lab.so 1, to_lab.so 1), 링크된 런타임, 생성 토큰 wrapper 없음. rx 는 같은 트리의 계측된 바이너리를 `KZ_RT_MODE=off` 로 돈다(hook 은 곧 돌아온다).
  8. **같은 빌드 입력**: 계획 sidecar `8f5f200c…`, manifest `5f015c5a…`; 계획을 만든 facts `0d7954fa…`·basis `87a80eb0…`(T8CF c1_pre_tx), 그 relations `936cd6c8…`; 프레임워크 계층 자료 `e2a39203…`; 플러그인 `27a49a6a…`; 런타임 `libkzrt.a` `718acdfd…`, `libkzrt-posix.a` `d0fbd6a4…`, `kz_rt_build_info.c.o` `d4995fa5…`; 바인딩 파일 `6a22b9ec…`; B′ basis `a57c227b…`, facts `6451f233…`; 실행 파일 core-tx `c2aa181b…`(build-id `ca1ed0b1…`), core-rx `d7d7be17…`(build-id `2d028ce2…`), 모듈 여섯(§5.2). 사슬 점검이 이것들을 run.meta, dump, run.kzt 와 잇는다.
  9. **환경**: 앞선 C1 G-I 자연 부팅의 조건 그대로(기록 `records/w6-20261029-c1-runs/`, 저장소 밖 `/home/user/work/w6/run/c1nat/gi/pre_1df262a_tx/`). root 가 하네스를 돌리고 인스턴스는 `setpriv` 로 ubuntu(cFS README L107). 새 시스템 설정, 새 권한, 실시간 우선순위 설정, IPC 이름 공간은 없다. 하네스가 run.meta 에 적는 호스트 줄의 항목(kernel, nproc, run_user, mqueue.msg_max)은 앞선 부팅의 것과 이름마다 같음/다름만 비교해 적는다(`conditions_compare.txt`; 값은 저장소 문서에 적지 않는다). 환경 변수는 이름만(run.meta).
  10. **공간**: 하한 314,572,800 B, 필요량 50,000,000 B(부팅 디렉터리 약 3.9 MB [M: W6 G-I 의 tx 2,494,464 B, rx 1,359,872 B] + run.kzt 약 0.16 MB + 한 시간 흐름 13 MB 를 올림), 문턱 364,572,800 B. 부팅과 변환을 감시 아래 돌린다(2 s 마다).
- **바이트로 비교하는 것**: 인스턴스의 모든 파일 = 풀어낸 트리 `<W>/ws/exe` 의 같은 자리 파일(`instance`); 계획의 일곱 실행 파일 = `build_instr.record` = 트리 = run.meta·목록(`exe`); 계획 manifest = 빌드 기록 = run.meta = dump 의 RUN·module 줄 = run.kzt 의 NRUN·FILE_HASHES(`manifest`); sidecar(`sidecar`); 계획 입력 = T8CF facts·basis(`plan_inputs`); B′ facts 의 부모 = B′ basis = run.kzt 의 부모(`bprime`); B′ basis 의 `binary.sha256` = run 의 core-tx(`facts_build`); dump(`dump`); 바인딩 파일(`bindings`); MC 의 도구(`pre_mc`/`post_mc`); 누적 목록(`ledger_check.sh` 전체, 새 장부 행 `OBS_c1pre_boot`, `OBS_c1pre_convert`). `same_build` 는 따로 적는다: 1.4 dump 에는 build-id 가 없어 `unconfirmed(no build-id in dump)` 이고, 실행 파일의 연결은 run.meta 의 sha256 으로 확인한다.
- **하네스의 조건 글**: `conditions.txt` 는 앞선 부팅 `W6 pre_1df262a_tx` 의 것과 빌드 경로를 이름 대는 줄(`cfs.build`) 밖에서 같아야 한다(`conditions_compare.txt`). `run.group` 줄의 "no plan" 은 런타임의 제어 계획(kzrt-plan)이 없다는 뜻이다(하네스 글 그대로).
- **달라도 되는 것(미리 기대하지 않음)**: 부팅 자체의 내용(시각, pid, thread id, 사건과 값, 수, 끝 상태, console 글, dump 크기). run.meta 의 label·manifest·bindings 줄의 경로. 결과는 측정으로 적는다. 조회의 결과(-17, 0, -35 어느 것이든)나 멈춤 사건의 도달을 미리 기대하지 않는다.
- **PI 의 선택이 되는 것(하면 멈추고 보고)**: 대상 소스·표·시작 스크립트·시작 구성을 계측 밖에서 바꾸기, hold·gate·지향 순서, 앞선 C1 부팅에서 쓰지 않은 권한이나 시스템 설정, 있는 파생 트리나 기록 디렉터리를 다시 쓰거나 덮기, 공개, 바깥 출처. 여유가 하한 + 필요량 아래.
- **지우는 것**: 이 단계가 scratchpad 에 만든 것(재생성 점검의 풀린 사본)만. 부팅 디렉터리, dump, 변환 출력은 지우지 않는다.

### 6.1 바뀐 것 [F]

- 커밋 `c9a680f`(선언, 재생성 점검의 기록, 스크립트; 부팅 전). 그 앞의 자동 스냅숏 커밋 `4f3ee54` 에 `chain_check.py` 의 바뀐 글이 먼저 들어갔다(선언 전, 부팅 전; 바깥 이름 점검 `--rev 719cb43 4f3ee54` rc 0). 규모: `chain_check.py` +156/−1, `stop_eval.py` 133줄(새), `run_unit.sh` +50, `run_boot.sh` 76줄(새, 기록 디렉터리).
- 부팅 뒤 `run_boot.sh` 의 control 단계를 한 번 고쳤다: 처음 글은 dump 의 `check` 줄 가운데 ok/pass 로 끝나지 않는 줄을 셌는데(22), 그 줄들은 행위자별 셈과 용량 줄이라 판정이 아니다. kz-rt-dump 의 마지막 판정 줄("check ok (0 failures)")을 적게 바꾸고 다시 돌렸다(읽기만 하는 단계).
- 대상 소스, 표, 시작 스크립트, 시작 구성, 하네스는 바꾸지 않았다.

### 6.2 부팅 [M]

공간 감시 아래 한 번(`boot.txt`, `df_boot.log`, `run_meta_excerpt.txt`, `conditions.txt`; 부팅 디렉터리 `/home/user/work/redesign/obs1/c1pre_boot/tx/`).

| 항목 | 값 |
|---|---|
| 시작·끝 (UTC) | 08:03:26.653 – 08:03:37.089 (`instances.start_utc`, `instances.end_utc`) |
| OPERATIONAL (5 s 대기 뒤) | tx yes, rx yes |
| UDP 1234/1235 의 주인 | tx, rx (하네스의 확인 통과) |
| 멈춤 | `stop=ci-es-restart-poweron (CI L196-L197)`; SIGKILL 줄 없음(두 인스턴스 모두 5 s 안에 끝남) |
| 끝 상태 | tx_exit=0, rx_exit=0 |
| dump | `tx.kzrt` 91,416 B, sha256 `fd05e211fb546046…`; 하네스의 kz-rt-dump(`eb12d09e`) exit 0 |
| 인스턴스 파일 | 31(`inst_files.sha256` `4aac220a…`; 다른 종류 0). tx 의 `cf_def_config.tbl` `455d265e…`(W6 인스턴스의 값과 같음), `cfe_es_startup.scr` `8ddb9359…`(tx·rx 같음) |
| console | tx·rx 모두 "entering OPERATIONAL state" 1, "Startup Sync failed" 0, "Unable to obtain Entry Point" 0. CF 줄(`cf_console.txt`): "CF: failed to get sem id for name BP0_tsem, error=-17"(64행) 뒤 BP 의 "Created semaphore: BP0_tsem", "BP1_tsem"(68, 69행) |

- 조건 글: `conditions.txt` 는 W6 의 `pre_1df262a_tx` 의 것과 `cfs.build` 줄(빌드 경로) 하나만 다르다(`conditions_compare.txt`, 선언대로). run.meta 의 호스트 줄은 nproc, run_user, mqueue.msg_max 가 같고 kernel 이 다르다(값은 run.meta 에만 있다). 새 시스템 설정, 새 권한은 쓰지 않았다.
- 부팅은 한 번이었다. 다시 부팅하지 않았다.

### 6.3 dump, 변환 [M]

- **관측만**(`control_check.txt`): dump 글(1,081줄, sha256 `1290a5f6…`)의 레코드 종류는 run 3, caps 1, runx 4, module 6, resource 2, actor 16, buf 17, ev 1,008, end 1, format 1, check 22. control 레코드(`ctl`, `plan`, `point`) 0. `runx window_mode=record mode_source=early`, `runx end=in-process … store=memory`. kz-rt-dump 의 판정 "check ok (0 failures)"; manifest 점검 "check manifest 5f015c5a… = plan.kzs"; 등록되지 않은 module 하나 "module 2 (bplib) never registered"(bplib 에는 site 가 없다, §5.3).
- **RUN 레코드**: backend "kzgcc 0.3 (gcc-plugin)", runtime "kzrt 0.4.0", `manifest_sha256=5f015c5a…`; module 여섯(5 core-tx 등록 23, 4 ci_lab 1, 7 to_lab 1, 6 sch_lab 1, 3 cf 4, 1 bp 3), 모두 같은 manifest sha256.
- **한계와 손실**: 행위자 16(최대 64), 모든 행위자 aseq 빠짐 0, overflow 0, dropped_bytes 0; 풀 4,194,304 B 가운데 chunk 32(1,024 가운데), committed 88,820 B, 가장 큰 행위자 수요 40,724 B(행위자 1); pool_dropped 0, boot 버퍼 0. 문자열 복사 14(`str=`), 잘림 표시(`str-trunc`) 0. 사건에 나온 site 는 301 가운데 82.
- **두 판독기**(`rtdump_compare.txt`): 같은 dump 와 manifest 둘로 MC `kz-rt-dump` `4390d4f1` 의 글은 하네스의 글(런타임 트리 `eb12d09e`)과 바이트 동일하다.
- **변환**(`convert.log`; MC kz `931dd39c`): rc 0. 중립 층 site 301, slot 671, 사건 1,008, 값 1,797, 행위자 16, 손실 0, control 레코드 0; 플래그 {loss-evidence, complete, stopped}; 끝은 프로세스 자신의 dump. `run.kzt` 234,528 B, sha256 `faa410e108546e97…`(`/home/user/work/redesign/obs1/c1pre_convert/`), 부모 basis = B′ basis `a57c227b…`. `--process` 는 주지 않았다(RUNX process 가 있다: "initial thread: run actor 1 of process …").
- **관계 1 의 행의 site**(사용 지점 키 `17975444d83f6589`, §5.3 의 12개): 모두 사건이 있다. 2·3(SCH_LAB 생성자) 각 1, 279·280(BP 생성자, inlined) 각 2, 272·273(키 글) 각 3, 284–289(처리 사슬과 사용) 각 1. 이 값들이 무엇을 뜻하는지는 다음 절의 설명 기록이 적는다(여기서는 수만 적는다).

### 6.4 사슬 [M]

`chain_check.py`(`chain.txt`, rc 0): 16 연결 모두 confirmed, 사슬 confirmed.

| 연결 | 비교한 것 |
|---|---|
| manifest | 계획 = 빌드 기록 = run.meta = dump(RUN, module) = run.kzt NRUN = run.kzt FILE_HASHES: 모두 `5f015c5a…` |
| sidecar | 계획 머리 = `plan.side` `8f5f200c…` |
| plan_inputs | sidecar 의 입력 = T8CF facts `0d7954fa…`, basis `87a80eb0…` |
| inst_list, instance | run.meta 의 목록 sha256 = 파일; 31 항목 모두 `<W>/ws/exe` 의 같은 자리 파일과 같음 |
| exe ×7 | 계획의 일곱 파일: 빌드 기록 = 트리 = 목록(core-tx, cf.so, bp.so 는 run.meta `file` 줄과도) |
| facts_build | B′ basis 의 `binary.sha256` `c2aa181b…` = run 의 core-tx |
| bprime | B′ basis `a57c227b…` = B′ facts 의 부모 = run.kzt 의 부모 |
| dump | 파일 = run.meta = run.kzt `convert.dump.sha256`: `fd05e211…` |
| bindings | 파일 = run.meta = run.kzt FILE_HASHES: `6a22b9ec…` |
| same_build (따로) | `unconfirmed(no build-id in dump)`; 트리의 core-tx build-id `ca1ed0b1…` 를 옆에 적음 |

- B 와 B′ 의 facts 가 선언 목록 밖에서 레코드마다 같다는 것은 §5.3 의 비교(`compare_after.tsv`, 선언 밖 0)다. 이 사슬은 그 B′ 에서 부팅 기록까지를 잇는다.

### 6.5 예산의 결과 [M]

선언(§6.0)의 예산마다, 부팅 뒤에 적는다.

1. **멈춤 사건**(`stop_eval.tsv`, rc 0): 시작 스크립트의 앱 다섯이 모두 창 안에서 멈춤 사건에 이르렀다. 라이브러리 둘(tinycbor, bplib)은 규칙상 멈춤 사건이 없다. 시각은 t_init 뒤 ms(dump 의 시계).

   | 앱 | 행위자 | root entry | 멈춤 사건 | 시각 |
   |---|---|---|---|---|
   | CI_LAB_APP | 10 | 1556.320 | CFE_ES_RunLoop (site 7, aseq 9) | 1556.705 |
   | TO_LAB_APP | 11 | 1556.384 | CFE_ES_RunLoop (aseq 49) | 1556.664 |
   | SCH_LAB_APP | 14 | 1556.758 | CFE_ES_RunLoop (aseq 20) | 1556.862 |
   | CF | 12 | 1556.708 | CFE_ES_RunLoop (aseq 31) | 1556.953 |
   | BP | 13 | 1556.966 | CFE_ES_RunLoop (aseq 41) | 1568.106 |

   창(RUN 줄 t_init–t_stop)의 길이 5,679.559 ms. 평가는 각 앱의 첫 멈춤 사건만 본다(규칙). CF 의 행위자에는 그 뒤 `CFE_ES_ExitApp` 의 entry(site 6, aseq 34)도 있다. 그것이 무엇인지는 설명 기록의 일이다.
2. **도구의 멈춤 예산**: 값 5 s + 명령 + 5 s(CI L176-L177, L196-L197, L199-L200), lead 판단(근거는 §6.0). 발동: 첫 5 s 대기 뒤의 ES 재시작 명령이 두 인스턴스를 끝냈다(exit 0; SIGKILL 없음). 다섯 앱의 멈춤 사건은 모두 t_init 뒤 1,568.106 ms 안에 있다. 첫 대기 5 s 는 인스턴스 시작(t_init 보다 앞이거나 같은 때)부터 센다. 그러므로 예산이 어느 앱의 멈춤 사건보다 먼저 오지 않았다. 다만 t_init 와 하네스의 시작 시각 사이의 차이는 기록되지 않았다(dump 는 단조 시계, run.meta 는 UTC) [미확인]. 1.57 s 와 5 s 의 거리가 그 차이보다 클 것이라는 것은 추정이다 [I]. 잘림: 프로세스 끝 뒤의 일은 기록되지 않았다. CI L179-L191(흐름 켜기, Class 1 전송, `sleep 30`)은 돌리지 않았다. 미해결: 없음(다섯 앱 모두 도달). 민감도: 재지 않음(부팅 하나). 예산의 멈춤은 실패로도 정상으로도 적지 않는다.
3. **프로그램의 시간 제한**: console 에 "Startup Sync failed"(`cfe_es_start.c:210`, `:225`) 0, "Unable to obtain Entry Point" 0, OPERATIONAL 1(tx, rx). 동기화 지점의 시각은 기록되지 않아(그 자리에 site 가 없다) 1000 ms 한계까지 얼마나 남았는지는 [미확인]. `CFE_PLATFORM_CORE_MAX_STARTUP_MSEC` 30000 은 창보다 길다. 창은 OPERATIONAL 까지 덮었다(5 s 확인에서 yes).
4. **실행 예산**: 부팅 하나. 다시 부팅하지 않았다.
5. **계획 예산**: `tier:D07` 12행, 잘린 419행(sidecar). 기록 풀 쓰임 chunk 32/1,024, 88,820 B(풀의 약 2.1%). 다음 층이 풀에 들어가는지는 이 수로 추정할 수 있을 뿐 재지 않았다 [미확인]. 민감도: 재지 않음.
6. **한계**: 손실 0(§6.3). 문자열 14 모두 잘림 없음. 행위자 16/64.
7. **관측 효과**: §5.2 그대로(이 부팅이 바꾼 것 없음). rx 는 계측된 바이너리를 `KZ_RT_MODE=off` 로 돌았다.
8. **같은 빌드 입력**: §6.4 의 사슬로 이어짐.
9. **환경**: §6.2.
10. **공간**: §6.7.

### 6.6 바이트 동일성 [M]

- MC: `post_mc.txt` 는 `pre_mc.txt` 와 시각·여유 줄 밖에서 같다. 도구를 빌드하지 않았다.
- 누적 목록: `ledger_check.sh` 42행, 실패 0(새 행 둘: `OBS_c1pre_boot` 52 파일, `OBS_c1pre_convert` 5 파일). F1T, `641/b`, 플러그인 둘, kairos-build 도구, W6·C5 기록 묶음, 앞 단계의 출력(파생 작업 공간, 런타임 트리, B′, 계획)과 도구 사본 모두 같다. 부팅은 파생 트리를 읽기만 했다(`OBS_c1pre_derived` 같음).
- 도구 등록부: `tools_check.sh --deep` 31행, 실패 0. 새 도구 행 없음.
- 단위 점검: `run_unit.sh` 14/14, `step2a_tests_unit.sh` 7/7(선언 전).
- 바깥 이름 점검(`--tree`): 부팅 디렉터리, 변환 디렉터리, 기록 디렉터리, `env/obs1` 깨끗함. 커밋 전 `--cached` 와 저장소 안 패턴 점검.

### 6.7 공간 [M]

| 시점 (UTC) | 여유 (df -B1 /) | 일 |
|---|---|---|
| 07:58:30 | 3,751,940,096 | 재생성 점검 시작 |
| 07:58:34 | 3,567,517,696 (가장 낮은 점) | 재생성 점검(dip 184,422,400; scratchpad 의 풀린 사본 184,395,429 B 를 점검 뒤 지움) |
| 08:02:57 | 3,751,612,416 | 선언 커밋 전 |
| 08:03:26–08:03:38 | 3,751,362,560 → 3,748,626,432 | 부팅(dip 2,736,128) |
| 08:03:56–08:03:58 | 3,748,417,536 → 3,748,171,776 | 변환(dip 245,760) |
| 08:06:20 | 3,747,467,264 | 기록 뒤 |

- 하한을 넘지 않았다. 감시 문턱(부팅·변환 364,572,800, 재생성 점검 564,572,800) 아래로 가지 않았다. 남긴 크기(블록): 부팅 디렉터리 2,732,032 B(겉보기 3,613,913 B; 계획의 추정 약 3.9 MB 는 tx·rx 두 기록의 합이고 이 부팅은 tx 기록 하나다), 변환 디렉터리 864,256 B(run.kzt 234,528 B, 그 글 보기 447,903 B 포함).
- 지운 것: scratchpad 의 재생성 점검 디렉터리(풀린 생성기 사본). 그 밖에 아무것도 지우지 않았다.

### 6.8 계획과 다르게 한 것, 다음 단계로 넘기는 것

- 사슬 점검기에 연결을 더했다(§6.0). 계획은 "facts, 계획, B′, run.meta, RUN 레코드, dump, run.kzt 위의 사슬"을 말했고, 앞의 점검기는 sidecar·계획 입력·인스턴스 목록·B′ 부모·dump·바인딩·run.kzt 를 보지 않았다. 앞의 점검기만으로는 이 빌드의 일곱 실행 파일 가운데 넷이 unconfirmed 로 남는다(run.meta 의 `file` 줄이 고정된 셋이다, `run_gi_c1.sh:52`). 하네스는 바꾸지 않고 run.meta 가 sha256 으로 이름 대는 인스턴스 목록을 둘째 출처로 썼다.
- 멈춤 사건 평가를 이 단계의 스크립트(`env/obs1/stop_eval.py`)로 했다. 계획은 이 평가를 예산 기록의 일로 적었고 스크립트 이름은 정하지 않았다. 설명 기록 단계의 스크립트는 이 결과(`stop_eval.tsv`)나 같은 규칙을 다시 쓸 수 있다.
- kz-rt-dump: 하네스 안에서는 런타임 트리의 것(`eb12d09e`), 뒤에 MC 의 것(`4390d4f1`)도 돌렸다. 두 글은 바이트 동일하다(§6.3).
- 계획 3.1 이 적은 `run_gi_c1.sh` 의 줄 번호 가운데 rx 경로의 `:62`, `:91` 은 지금 파일에서 `:97`(sleep 8), `:91`(tx 경로의 SIGKILL)이다. 선언에는 지금 파일의 줄을 적었다.
- 부팅 디렉터리의 크기는 계획의 추정(두 기록의 합)보다 작다. 이 부팅은 tx 기록 하나다(rx 는 기록하지 않는 인스턴스로 같은 디렉터리에 있다).
- 다음 단계로: 설명 기록의 입력은 dump 글 `/home/user/work/redesign/obs1/c1pre_boot/tx/tx.dump.txt`(`1290a5f6…`), sidecar, manifest, 바인딩 파일 `<W>/binds_instr.kzs`, relations 의 행(T8CF c1_pre_tx, 사용 지점 키 `17975444d83f6589`), run.meta, 사슬 기록 `records/obs1-20261116-boot/chain.txt`, 멈춤 사건 평가 `stop_eval.tsv`. 창의 경계: 시작은 t_init(RUN 줄), 끝은 프로세스 자신의 dump(t_stop); 창 밖은 프로세스 끝 뒤와 CI L179-L191. t_init 와 하네스 시각의 차이는 기록되지 않았다 [미확인].

## 7. 설명 기록과 완료 시험 (Kairos 도구 빌드 없음)

기록: `../records/obs1-20261116-explain/`. 출력: 저장소 밖 새 디렉터리 하나, `/home/user/work/redesign/obs1/c1pre_explain/`(facts 글 보기와 설명 기록). 로컬 묶음: `/home/user/work/preserve/bundles-20261116/obs1-20261116-cycle/`(새). 계획, 파생 작업 공간, B′, 부팅·변환 디렉터리, relations 재실행, F1T, W6 기록, T8CF 는 읽기만 한다. 부팅하지 않는다. push 하지 않는다.

### 7.0 선언 (실제 기록에 돌리기 전에 적고 커밋함) [D]

- **Kairos 도구**: 다시 빌드하지 않는다. 정체성이 바뀌는 도구: 없음. MC 의 도구 일곱, 플러그인 둘, kairos-build 도구는 이 단계 앞뒤에 같아야 한다(`pre_mc.txt`, `post_mc.txt`; `tools_check.sh --deep`). 쓰는 도구: MC `kz-dump` `05623054…`(계획의 facts 머리와 BASIS 항목 넷을 글로 보기만 함, `facts_view.txt`).
- **스크립트**(이 선언과 함께 커밋; `env/` 와 `records/` 는 `kairos` 밖이라 어떤 도구의 정체성에도 들지 않는다):
  - `env/obs1/explain.py`(새, `python3 -I`): 기록 단위는 (빌드, 계획된 행, 소비 행위자) 하나(판정 규칙 개정 1 3.1). 필드 이름과 상태 값은 판정 규칙 3.1–3.2, 판정은 3.3, 사유는 3.5 의 어휘 자료 `kairos/tests/static/data/reason_vocabulary.txt`(그 표에서 뽑은 것, `test_static_reasons` 가 지킴). 명령줄이 이름 대는 파일만 열고 모두 sha256 으로 기록마다 적는다. 계산 규칙(기록에 이름으로 적힘):
    - 생성 인자 묶기: 생성자의 actor-create at-call 에서 role `creation_arg` 값 = 자식의 actor-start entry 의 `param0`. 그 값이 생성 호출 쪽이나 시작 쪽에서 두 번 이상 보이면 그 값의 묶기는 모두 undetermined.
    - root 묶기: 행위자 안의 actor-root entry. RUNX 가 이름 대는 첫 스레드는 프로세스다.
    - 행위자별 호출 쌓기: 행위자 안에서 aseq 차례로, at-call 이 호출을 열고 같은 (함수, callee, 줄)의 after-call 이 가장 안쪽의 열린 것을 닫는다. callee 쪽 사건(그 함수가 callee 인 site)은 그것을 감싸는 닫힌 호출 가운데 가장 안쪽의 것에 든다. 감싸는 것이 없으면 귀속하지 않는다.
    - out 은 반환이 성공 상수와 같을 때만 읽는다. 성공 상수 = 행이 그 사용의 반환 상수로 주는 것 가운데 실패 등급이 아닌 유일한 것.
    - 실패 등급(방법의 S1 규칙의 이식, `kairos/lib/llvm/plan_gen.cpp:58`; 방법 문서 `ORDER_VIOLATION_METHOD_20261111.md:381` 의 어휘 (a) cFE 심각도 비트, (b) OSAL 음수): v < 0 이거나 0xC0000000 ≤ v ≤ 0xFFFFFFFF. 실행의 반환에 이 등급을 붙이는 것은 `odf`(순서 의존 실패) 계산뿐이고, `use_cond` 에는 쓰지 않는다(3.6 항목 3).
    - 축은 3.2 대로, `tool_verdict` 는 3.3 대로 축의 상태만으로 계산한다. 계산이 읽은 필드를 `tool_verdict.inputs` 에 적고, 처리 표시·`priority`·`review`·`eval_result` 가 거기 없음을 3.6 항목 5·6 으로 점검한다.
    - `use_cond` 는 정적 행의 글만 쓴다(줄 번호가 출처). 실행 값은 키 글만 싣는다(판정 규칙 4.2). (c1) callee 검사: 행의 반환 상수에 실패 등급이 있으면 있음. (c2) 부재 값의 도달: `flip: static-possible … with the earlier value` 이면 있음. (c3) 호출자의 기댐: 실패 경로의 종료 사슬(`handling: terminal`)이 있고 그 연결에 "may" 가 없으면 있음. (c4) 쓰기 집합: 행에 writer 나 creator 가 있고 행의 미완 사유에 `writer_set_incomplete`, 생성자 키의 `key_unresolved` 가 없고 feasibility 가 정해졌으면(possible, proven-impossible) 있음. 넷이면 derived, 하나 이상이면 partial, 없으면 not_derived.
    - `order.status`: 실행 없음 → static_only; 사용 사건 없음이나 값 빠짐 → record_insufficient(사유); P 가 유도되지 않으면 events_only; 유도되면 읽은 값으로만 P 를 본다(경로 A: 같은 키의 생성 at-call 이 사용 at-call 뒤 → unmet_state_observed, 사용이 성공 상수 아래 읽은 id = 앞선 생성의 out id → met_state_only; 경로 B: 읽은 필드 값 = 행의 INIT 값 → unmet). `order.static` 은 unknown("order edges not computed in this cycle").
    - `link.status`: 소비자와 게시자의 생성 인자 묶기와 root 묶기가 정해지고, 경로 A 에서 사용의 키 글과 게시자 생성의 키 글이 잘림 없이 같으면 established(사용이 성공 상수 아래 읽은 id 가 있으면 그것이 생성의 out id 와 같아야 한다). 모든 게시자의 키 글이 다르면 refuted. 그 밖은 unlinked 와 `link.missing`. 경로 B 는 사용에서 읽은 주소가 계획에 없어 unlinked(`record_insufficient(the address read at the use is not in the plan)`).
    - `intervention`: plan_origin generated, hold none, status unconfirmed, 사유 `intervention_out_of_scope`. 이 단계에는 개입이 없어 differs·same 은 나올 수 없다.
    - `effect`: 창 = RUN 의 t_init–t_stop, 멈춤 사건 줄(sidecar), 그 앱의 `stop_eval.tsv` 줄; 사용 뒤 같은 행위자의 처리 사슬 site 와 멈춤 사건 site 의 사건; 복구는 같은 행위자의 뒤 사용(성공 상수 아래 out 을 읽으면 completed, 아니면 attempted_not_completed), 뒤 사용 없이 app-exit entry 가 있으면 not_attempted(범위: 계획된 site), 그 밖은 not_observed.
    - 전제는 기록마다 적는다: P-ARG(시작 루틴이 생성 인자를 그대로 받음, POSIX; 로컬 glibc 2.39 사본과만 대조; 링크된 C 라이브러리는 §1.7 `c_library.txt`), P-ACTOR(행위자 id 는 스레드마다), P-STACK, P-CLOCK(모든 행위자의 시각은 한 시계: RUN clock, POSIX 어댑터는 CLOCK_MONOTONIC `kairos/runtime/src/kz_rt_posix.c:43-44`), P-STR, P-CONST(행이 그 사용에 주는 반환 상수가 그 callee 의 상태 상수), L-ABS(아래).
    - 3.6 서식 검사 여덟과 어휘 검사를 쓰기 전에 한다. 어긋나는 기록은 쓰지 않고 `refused.txt` 에 적는다(rc 4). `--check RECORD.json` 은 한 기록만 검사한다.
  - `env/obs1/tests/test_explain.py`(새)와 `env/tools/tests/run_unit.sh` 점검 15: 합성 fixture 16 경우. 필요 상태 여덟(present, value-missing, text-partial, not-reached, unknown-loss, unbound, not-planned, build-mismatch), link 의 established·refuted·unlinked, use_cond 의 derived·partial·not_derived, order 의 unmet_state_observed·met_state_only·events_only·record_insufficient·static_only, effect 의 observed·none_in_window·outside_window_unknown, intervention 의 unconfirmed, 생성 인자 값이 두 번 보이는 경우(두 묶기 모두 undetermined), 결합 거부(rc 3), 3.6 항목 1–8 과 어휘를 하나씩 깬 기록(모두 `--check` rc 4). 선언 전 `run_unit.sh` 15/15, `step2a_tests_unit.sh` 7/7 [M].
  - `records/obs1-20261116-explain/run_explain.sh`(단계 facts-view, explain, copy, plan-checks, hand, bundle)와 `hand_compare_record.py`(손 manifest 기록과의 비교; 비교 자료일 뿐 생성 경로의 입력이 아니다).
- **lead 판단 L-ABS(경로 A, 부재 결과의 연결)**: 객체를 찾지 못한 사용과, 같은 실행 같은 프로세스에서 행의 이름 공간 칸에 같은 키로 한 생성을, 같은 이름 있는 자원으로 본다. 사용이 본 세대는 "없음"이다. 근거: 판정 규칙 3.2 축 1 의 자원 키(객체 종류·이름 공간·이름·생성 세대), 4.2 의 표("경로 A 는 키의 실행 값과 같은 이름 객체의 연결이" 남았다), 방법 문서 :421(키가 다른 SCH_LAB 은 짝에서 빠진다). 축 1 의 "인정하지 않는 것"의 "ID 문자열이 같다는 것"을 이름 조회의 키 글로 읽지 않았다. 이 읽기는 [검토 판단]이고 PI 가 다르게 정하면 바뀐다. 그래서 기록마다 전제로 적는다.
- **입력**(sha256 앞 16자리; 모두 읽기만): sidecar `8f5f200c…`, manifest `5f015c5a…`(`plangen/c1_pre_tx`), 바인딩 `<W>/binds_instr.kzs` `6a22b9ec…`, relations `7c7396d2…`·keys `d6d970d6…`(`plangen_relcheck/c1_pre_tx`: 생성기 `de1315b3` 가 T8CF facts 에서 다시 낸 것, 앞 단계 출력과 바이트 동일 §4.4), 어휘 `4b964d34…`, dump 글 `1290a5f6…`, run.meta `075f39b2…`, 사슬 기록 `3d26a32b…`, 멈춤 사건 평가 `6eb44262…`, 시작 스크립트 `8ddb9359…`, B′ basis `a57c227b…`, facts 글 보기(facts-view 단계가 만든다; 그 첫 줄이 T8CF facts `0d7954fa…` 를 sha256 으로 이름 댄다).
- **기록할 행**: c1_pre_tx 의 계획된 행 12(sidecar `state=planned`) 모두, 같은 스크립트, 더한 입력 없음. 소비 행위자 = 그 행의 use-call site 에 사건이 있는 실행 행위자들, 더하기 행의 정적 소비자 root 가운데 실행 행위자가 묶이지 않은 것(묶인 행위자가 있으나 사용 사건이 없으면 그 행위자로, 없으면 실행 행위자 없는 기록). 둘 다 없으면 실행 행위자 없는 기록 하나. 셈은 판정별, 미완 사유별로 적는다.
- **완료 시험(관계 1, 사용 지점 키 `17975444d83f6589`)**: E1–E5 를 항목마다 잰 대로(pass, fail, partial 과 까닭) 적는다. E1: sidecar 머리의 입력·도구·규칙·예산, 선언되지 않은 파일 거부의 시험(`test_static_plan` 의 거부 경우), 재생성 점검(§6.0), RUN 의 manifest = 계획(사슬), 플러그인 module 인자 = module 표(§5.3), 프레임워크 계층 자료의 출처와 헤더 훑기, lint(§4). E2: 계획 글의 기계 점검(`plan-checks`: site 마다 규칙 이름과 ref, 기대 값이 없음)과 파생 스크립트의 인자(§5). E3: §5.3 비교, 사슬, build-id, 인스턴스 해시. E4·E5: 설명 기록과 3.6 점검. E6 은 C5 관계라 이 주기에 없다고 적는다.
- **바이트로 비교하는 것**: 입력 파일 모두 앞뒤로 같다(장부 `ledger_check.sh` 전체; 새 장부 행 `OBS_c1pre_explain`). 같은 입력으로 explain 을 scratchpad 에 한 번 더 돌려 출력이 바이트 동일한지 본다(그 scratchpad 사본은 비교 뒤 지운다).
- **손 manifest 기록과의 비교**: W6 G-I 자연 부팅 `pre_1df262a_tx` 의 `tx.dump.txt`(sha256 `9de99fd2…`, 205–240행)와 이 기록. site 대응은 부팅 전에 잰 `records/obs1-20261116-plangen/hand_compare_c1_pre.txt`. 두 기록이 함께 가진 값, 한쪽만 가진 값, 그 까닭을 적는다.
- **문서 정정**: `plan/ovm_evidence_20261111/p_offline.md:27` 의 "Not in the record: event type, exit status value" 는 F0 기록에 대해 맞지 않다(§1.7: `W/w6/run/c1nat/f0tx/pre_1df262a/run.kzt`, `pre_1b0b338/run.kzt` 의 site 8 `CFE_ES_ExitApp` entry, slot 0 `role="exit_status"`, `a[0]=0x3`). 그 줄에 날짜 붙은 정정 글을 덧붙이고(원문은 지우지 않음) 같은 폴더의 `SHA256SUMS` 에서 그 파일의 줄만 새 sha256 으로 바꾼다. 다시 `sha256sum -c` 를 돌린다.
- **로컬 묶음**: `records/local-20261109/bundle_local.sh`(바꾸지 않음, `BUNDLE_BASE` 로 새 위치)로 이 주기의 출력 일곱(`plangen`, `c1pre_rt`, `c1pre_derived`, `c1pre_bprime`, `c1pre_boot`, `c1pre_convert`, `c1pre_explain`)을 묶고 스트림으로 검증한다. push 하지 않는다. 저장소에는 글 기록(INFO, PARTS, 검증 기록)만 둔다.
- **미리 기대하지 않음**: 어느 축의 상태도, 판정도, 머리말도 미리 정하지 않는다. 결과는 잰 대로 적는다.
- **PI 의 선택이 되는 것(하면 멈추고 보고)**: 대상 소스·표·시작 스크립트·시작 구성 바꾸기, hold·gate·지향 순서, 새 권한이나 시스템 설정, 있는 출력 디렉터리 다시 쓰기, 공개, 바깥 출처. 이 단계는 그 어느 것도 하지 않는다.
- **공간**: 하한 314,572,800 B. explain 은 공간 감시 아래(필요량 30,000,000 B), 묶음은 `bundle_local.sh` 의 자체 점검(여유 − 한계 ≥ 하한).
- **지우는 것**: 이 단계가 scratchpad 에 만든 것(fixture 의 작업 디렉터리, 두 번째 explain 출력)만.

### 7.1 바뀐 것 [F]

- 커밋 `ab230a1`(선언, `explain.py`, fixture, 점검 15, 실행 스크립트; 실제 기록에 돌리기 전).
- 첫 실행 뒤 `explain.py` 를 세 군데 고쳤다(§7.10). (1) 사슬 기록의 `same_build` 줄을 공백에서 잘라 읽었다(`unconfirmed(no` 까지만 실렸다). (2) 행이 부호 없이 적는 32-bit 상태 상수(예 `3254779907`)와 부호 확장된 기록 값(`-0x3dfffffd`)을 맞추지 못했다. 그래서 경로 B 의 상태 이름이 비었다. 지금은 기록 값이 32-bit 범위 안이면 2³² 로 나눈 나머지로 맞춘다(fixture 17 을 더함). (3) 사용 뒤의 run-loop entry 를 `effect.events` 에 넣었었다. 그것은 결과의 효과가 아니라 그 행위자가 멈춤 사건에 이른 창의 사실이라 `effect.window.consumer_run_loop_after_use` 로 옮겼다. `effect.events_scope` 글도 더했다(사용 뒤 같은 행위자의 사건이며 인과를 주장하지 않는다). 판정 값(violation, odf, 머리말)은 세 실행에서 모두 같다. (3) 때문에 9 기록의 `effect.status` 가 observed 에서 none_in_window 로 바뀌었다(B73 넷, B81 둘, B99 셋).
- `run_explain.sh`: explain·copy 에 출력 이름 인자를 더했다. copy 는 색인, 입력, 거부, 모든 기록의 sha256, 관계 1 의 기록만 저장소에 둔다. 45 기록 모두는 2.3 MB 라 저장소 밖(`c1pre_explain/explain_r3/records/`)과 로컬 묶음에 둔다.
- `plan/ovm_evidence_20261111/p_offline.md:27` 정정 글(§7.7)과 `SHA256SUMS` 의 그 파일 줄.
- `records/LEDGER.tsv` 새 행 둘: `OBS_c1pre_explain`(145 파일), `B_obs1-20261116-cycle`(로컬 묶음 조각 1).
- 대상 소스, 표, 시작 스크립트, 시작 구성, 하네스, Kairos 소스는 바꾸지 않았다.

### 7.2 실행 [M]

- facts 글 보기(`facts_view.txt`): T8CF facts `0d7954fa…`, `tool_sha256=3b7de5b7…`(파일 sha256 `0024f192…` 의 도구, §1.8), `extract.build_identity`(source `f6e3c7b4…`), `extract.p0_table` "P0 table revision 3 (2026-10-30), 72 entries", `p0.pthread_create` 항목.
- explain 세 번(공간 감시 아래, 모두 rc 0, 기록 45, 거부 0): `explain`(08:33Z, 선언한 스크립트 그대로; 기록의 compat 글이 공백에서 잘림), `explain_r2`(08:35Z), `explain_r3`(08:36Z, 최종). 같은 입력으로 scratchpad 에 한 번 더 돌린 출력은 `explain_r3` 과 48 파일 모두 바이트 동일하다(`determinism.txt`; 그 사본은 지웠다).
- 입력(`inputs.txt`; 모든 기록의 `inputs` 에 같은 목록): bindings `6a22b9ec…`, bprime_basis `a57c227b…`, chain `3d26a32b…`, dump_text `1290a5f6…`, facts_view, keys `d6d970d6…`, manifest `5f015c5a…`, relations `7c7396d2…`, run_meta `075f39b2…`, sidecar `8f5f200c…`, startup_script `8ddb9359…`, stop_eval `6eb44262…`, vocabulary `4b964d34…`. 결합: "chain confirmed; same_build unconfirmed(no build-id in dump)".
- 단위 점검: `run_unit.sh` 15/15(점검 15 는 fixture 17/17), `step2a_tests_unit.sh` 7/7.

### 7.3 관계 1 의 설명 기록 [M]

기록 하나: (c1_pre_tx, 사용 지점 키 `17975444d83f6589`, 실행 행위자 12) — `records/obs1-20261116-explain/records/c1_pre_tx.A21.17975444d83f6589.actor-12.json`(39,561 B, sha256 `d6aa1575…`). 판정 규칙 3.1–3.2 의 필드마다:

| 필드 | 값 | 출처 |
|---|---|---|
| `build_id` | B′ basis `a57c227b…`; run.meta 의 실행 파일 sha256(core-tx `c2aa181b…`, cf.so `f5af74f7…`, bp.so `6789ff75…`, rx 셋); 계획 facts `0d7954fa…`; compat "chain confirmed; same_build unconfirmed(no build-id in dump)" | B′ basis 파일, run.meta, sidecar, 사슬 기록 |
| `extractor_rev`, `semantics_rev` | facts `tool_sha256 3b7de5b7…`, build_identity; `p0-only`, P0 표 개정 3(72), 프레임워크 계층 자료 `e2a39203…`(run-loop, app-exit 역할만); missing `absent_result_unevaluated(no common-semantics table of the framework APIs in this cycle)` | facts 글 보기, sidecar |
| `plan_id` | sidecar `8f5f200c…`, manifest `5f015c5a…`, RUN 레코드의 manifest `5f015c5a…` | 파일, dump RUN 줄 |
| `row_id`, `path` | 사용 지점 키 `17975444d83f6589`, 키 글 `A\|OS_common_record_t.name_entry\|CF_CFDP_InitEngine\|OS_CountSemGetIdByName#1\|1`, 행 번호 A21(출력 셈), `relations.txt:440`; path A | keys, relations |
| `consumer_actor` | 실행 행위자 12(tid 28934). 생성 인자 묶기 determined: 행위자 1 aseq 452 의 `pthread_create` at-call(`os-impl-tasks.c:543`) `creation_arg 0x1000c` = 행위자 12 aseq 1 의 `OS_PthreadTaskEntry` `param0 0x1000c`(두 쪽 모두 한 번); 생성 결과 status 0, out `0x7f820ebe36c0`; 뒤의 owner store `cfe_es_apps.c:619` 값 `0x110009`, `:620`. root 묶기 determined: `CF_AppMain`(script-entry, site 281 aseq 2). 앱: CF(시작 스크립트에서 entry `CF_AppMain`) | dump, manifest, 시작 스크립트 |
| `publisher_actors` | 13(BP_AppMain, `0x1000d`, 앱 BP), 14(SCH_Lab_AppMain, `0x1000b`, 앱 SCH_LAB_APP): 이 행의 생성자 site 에 사건이 있는 다른 행위자 | 같음 |
| `actor_pair_kind` | 13: app-app, 14: app-app | 묶인 root 의 종류와 앱 |
| `handling_label`, `handling_unknown_reason` | R, 없음(R 은 미확정 표시가 아님) | sidecar |
| `priority`, `budget` | 층 1, 차례 1, D07, 순서 RELOC, 도달 CALLEE-ONLY; 예산 `tier:D07`, 잘림 419행, 한계 셋, 실행 예산 부팅 1 | sidecar |
| `link` | status **established**(전제 L-ABS). resource_kind 이름 있는 객체; resource_key 이름 공간 `OS_common_record_t.name_entry` 칸 3, 이름 "BP0_tsem", 세대 `[0x30002]`; instance_evidence: 행위자 13 의 생성 키 "BP0_tsem" out `0x30002`, 13 의 "BP1_tsem" 와 14 의 "SCH_LAB" 은 다른 키; actor_token = 위의 생성 인자 묶기; missing 없음 | dump(키 글은 사용을 감싸는 호출 안 `osapi-countsem.c:212` 의 key-text 사건, 호출의 키 포인터 `0x55728e21174c` 와 같음) |
| `use_cond` | status **partial**. callee_check 있음(`-17 [OS_ERR_NAME_NOT_FOUND], 0 [OS_SUCCESS]`, `relations.txt:454`); reach 없음(`static-undetermined`); caller_dependence 없음(종료 사슬의 가드가 "may", `:449`); writers 없음(생성자 둘 `:444-445`, 키 미해결); run_key_value "BP0_tsem"(기록의 키 글); predicate 없음. missing: `absent_result_unevaluated(static-undetermined)`, `key_unresolved(…:454)`, `caller_dependence_unknown(the terminal chain's guard is linked 'may')`, `key_unresolved(creator key unresolved: relations.txt:444)`, `fixed_proof_open(undecidable)` | relations 만 |
| `order.static` | unknown, "order edges not computed in this cycle"(행의 feasibility 글을 note 로) | — |
| `order.use_events` | 사용 하나: at-call 행위자 12 aseq 25(t_init 뒤 1556.926 ms), 키 포인터 `0x55728e21174c`, 키 글 "BP0_tsem"(잘림 없음, aseq 26); after-call aseq 27 status `-0x11` = `OS_ERR_NAME_NOT_FOUND`(행의 상수), out 읽지 않음(status 가 성공 상수 `OS_SUCCESS 0` 이 아님) | dump |
| `order.publish_events` | 행위자 13: "BP0_tsem" 생성 at-call 1566.869 ms, status 0, out `0x30002`; "BP1_tsem" `0x30003`; 행위자 14: "SCH_LAB" 1556.760 ms, `0x30001`. out 은 등급 없이 적음(생성자의 반환 상수는 행에 없다) | dump |
| `order.sync_results` | 생성 결과: 행위자 12·13·14 의 `pthread_create` status 0; 대기·잠금 not-planned(`plan_missing(need not planned)`) | dump, sidecar |
| `order.per_consumer` | 항목 1 incomplete; 2 holds(게시 w = 행위자 13 aseq 12 앞의 사용 1, 실패 등급); 3 no_data(`no_matching_use_after_w`); 4 unchecked. 칸의 사건은 모두 행위자 12 | dump |
| `order.status` | **events_only**, missing `absent_result_unevaluated(the predicate is not derived: use_cond partial)` | 규칙 |
| `intervention` | generated, hold none, status **unconfirmed**, `intervention_out_of_scope(no hold, gate or directed run in this stage)` | sidecar |
| `effect` | status **observed**. 창 0–5679.559 ms(RUN t_init–t_stop), 끝: 프로세스는 `stop=ci-es-restart-poweron`, `tx_exit=0`; 행위자는 app-exit entry(aseq 34). 사건: `CF_CFDP_InitEngine` 반환 -17(aseq 28), `CF_Init` 반환 -17(29), `cf_app.c:333` store 값 3(30), `CFE_ES_RunLoop` 반환 0(32), `CFE_ES_ExitApp` at-call 인자 3(33), entry param0 3(34). run-loop entry(aseq 31, 1556.953 ms)는 창의 사실. recovery not_attempted(범위: 계획된 site). unobserved: 프로세스 끝 뒤, 계획되지 않은 자리, not-planned 필요 | dump, stop_eval |
| `tool_verdict` | violation **undetermined**(kind order_basis_unknown; 사유 = 위 use_cond 다섯과 order 하나), odf **failure_sign**(사용의 반환 -17 이 실패 등급; 유효한 개입 쌍 없음), final_effect `app_exit_called(CFE_ES_ExitApp param0=0x3)`, `no_recovery_in_window`; 머리말 **실패 징후**. 계산이 읽은 필드: link.status, use_cond.status, order.status, order.static, intervention.status, order.use_events.status_value, effect.status·events·recovery | 축만 |
| 미완(19.2) | 필요 17: present 13, not-reached 1(`cf_app.c:357` store), not-planned 3(event-send, wait-result, lock-result); 정적 미완 넷(행의 incomplete 줄 그대로, `no_run(no run record read)` 은 정적 출력의 말); 예산 잘림 419행; 열린 전제 일곱 | sidecar, relations, 규칙 |

- 이 기록이 주는 다섯 항목: 행위자(12 ↔ 생성 호출 1/452, root CF_AppMain, 앱 CF; 게시 13 ↔ 1/459, BP), 자원(칸 3 의 "BP0_tsem", 생성 세대 `0x30002`), 사용(1556.926 ms 의 조회, 같은 키의 생성 at-call 은 9.943 ms 뒤), 상태(-17 `OS_ERR_NAME_NOT_FOUND`, -17 이 InitEngine·CF_Init 로 올라감, run status store 3, RunLoop 반환 0, ExitApp 인자 3), 미완(위 사유들).
- 3 이 무엇인지: run status store 의 값 3 은 행이 `cf_app.c:333` 의 store 를 `CFE_ES_RunStatus_APP_ERROR` 로 적는 자리의 값이다(행 `:449`); 그 상수의 값 3 은 bundle 1df262a `cfe_es_extern_typedefs.h:139` [F, §1.7]. 기록은 이름을 붙이지 않고 값만 싣는다(종료 상태의 등급 어휘는 이 주기의 자료에 없다).
- L-ABS 에 기대는 것은 `link.status` 하나다. 이 기록에서 L-ABS 를 빼면 link 는 unlinked 가 되지만 violation 은 이미 use_cond partial 로 undetermined 이고 odf 의 failure_sign 은 link 를 조건으로 쓰지 않는다(판정 규칙 3.3). 그래서 판정은 바뀌지 않는다 [I: 규칙에서 읽음].

### 7.4 완료 시험 [M]

| 항목 | 결과 | 근거 |
|---|---|---|
| E1 출처 | pass | sidecar 머리(`plan.side:1-17`): 입력 셋(facts, basis, 프레임워크 계층 자료)과 sha256, 생성기 도구 `c3677e76…`(identity)·source, 규칙 판, 선택 규칙, 우선순위 판, 예산(값·이유·밝힘·잘림·미해결·민감도), 한계 셋, 매개변수 둘. 선언되지 않은 파일을 여는 길의 거부: `test_static_plan` 의 거부 경우(다른 파일을 이름 대는 자료 줄 → rc 2, 출력 없음; §4.3 ctest 246/0). 여는 호출의 추적은 하지 않았다(시험은 그 길을 막는 것을 본다). 재생성 점검 PASS(§6.0). RUN 레코드·B′·run.kzt 의 manifest = 계획(사슬 manifest 연결, §6.4). 플러그인 module 인자 = module 표(§5.2 `module_flags.txt`, §5.3). 프레임워크 계층 자료 9 항목 모두 출처 줄과 헤더 훑기에서 ok(`records/obs1-20261116-plangen/framework_check.txt`). lint `case-ids`, `case-ids-data` 통과(§4.3 `ctest_full.log`) |
| E2 손 내용 없음 | pass | `plan_checks.txt`: manifest site 301 모두 규칙 이름과 ref, sidecar site 줄 301 모두 refs; 값 줄의 칸은 role, from, type, string, bound, ref 뿐(기대 값 칸 없음); 주석 밖에 기대의 낱말(expect, should, must return, normal) 0. 파생 스크립트는 manifest, module 표, 실행 파일 목록을 인자로 받고 사례 값을 쓰지 않는다(§5.1, 점검 8·9) |
| E3 같은 빌드 | pass | B 와 B′ facts 는 선언 목록 밖 0(§5.3 `compare_after.tsv`); 실행 파일 일곱 = run.meta·목록(사슬 exe); RUN 의 manifest = 계획; build-id 는 B′ 파일에서 읽어 옆에 적음(core-tx `ca1ed0b1…`, `exe_buildids.tsv`), 사슬은 계획대로 `same_build: unconfirmed(no build-id in dump)`; 인스턴스 파일 31 해시, `cf_def_config.tbl` `455d265e…`(잰 값) |
| E4 설명 | pass | (빌드, 행, 소비 행위자) 마다 기록 하나(관계 1 은 하나); 필드 이름과 상태 값은 판정 규칙 3.1–3.2; 모든 필드가 기록이나 정적 사실로 채워지거나 3.5 사유와 함께 빠짐으로 적힘(빈 칸은 `missing` 이나 사유가 있음: 3.6 항목 7 점검); 3.6 여덟과 어휘 검사 통과(45 기록 거부 0); 빠진 것을 정상 값으로 채우지 않음(out 은 성공 상수 아래만, 생성자 out 은 "not graded", `order.static` unknown, P 없음). 열린 전제: L-ABS(§7.0; `link.status` 만 기대고 판정은 바뀌지 않음, §7.3) |
| E5 판정 | pass | `tool_verdict` 는 3.3 의 표대로 축의 상태만으로 계산(violation undetermined, odf failure_sign, 머리말 실패 징후)하고 3.6 을 통과. 미리 정하거나 빼지 않았다(선언 §7.0) |
| E6 C5 | 이 주기에 없음 | 계획은 생성되었다(c5_641 manifest `17b6ab6a…`, §4.5). relation 2 의 행 C22, C31 은 층 2(차례 79, 83)라 `tier:D07` 예산에서 잘렸다. C5 는 빌드·부팅하지 않았다(생성 토큰이 필요한 전체 단계의 일). "통과"로 적지 않는다 |

- 조회의 결과(-17)는 시험의 조건이 아니다(계획 1.2). 기록은 -17 과 그 처리 사슬을 설명한다.

### 7.5 계획된 다른 행 [M]

같은 스크립트, 같은 입력(더한 입력 없음). `summary.txt`, `index.tsv`.

- 기록 45(행 12): A21 1, A22 1, A23 1, A50 1, A51 1(모두 소비자 CF 행위자 12), B13 1(실행 행위자 없음: 읽는 쪽 root 없음, 사용 사건 없음), B73 15, B81 15, B99 6, B113 1, B118 1, B119 1(B 행의 소비자는 use-call 에 사건이 있는 행위자와, 정적 읽는 쪽 root 에 묶인 사건 없는 행위자).
- 판정별: violation undetermined 45. odf undetermined 43, failure_sign 2(A21 CF `OS_ERR_NAME_NOT_FOUND`; B73 의 행위자 2(`CFE_EVS_TaskMain`) `CFE_SB_Global.AppId` 를 넘긴 `CFE_EVS_SendEventWithAppID` 가 `CFE_EVS_APP_ILLEGAL_APP_ID` 를 넷 받음). 머리말: 미확정 43, 실패 징후 2.
- 축별: link established 1(A21), unlinked 44(경로 B 40: 사용에서 읽은 주소가 계획에 없다; 경로 A 넷: 사용 사건 없음). use_cond partial 45. order events_only 22, record_insufficient 23. intervention unconfirmed 45. effect observed 3, none_in_window 19, outside_window_unknown 23.
- violation 의 미확정 사유(사유별 기록 수): `absent_result_unevaluated` 45(하위 `static-undetermined` 45, `the predicate is not derived` 22), `caller_dependence_unknown` 45, `fixed_proof_open` 45, `writer_set_incomplete` 42(`FIELD_STATUS not complete` 26, 완전성이 전제뿐 16), `record_insufficient` 40(사용의 주소가 계획에 없음), `target_not_reached` 23, `key_unresolved` 12(경로 A 다섯 행), `link_unlinked` 1(B13 의 실행 행위자 없음).
- 필요 상태(모든 기록의 필요 합): not-reached 2,049, present 268, not-planned 95; value-missing, text-partial, unknown-loss, unbound, build-mismatch 0.
- 사용 사건이 없는 행: A22, A23, A50, A51(CF 의 `CF_CFDP_ResetTransaction`; CF 는 초기화에서 끝났다), B13, B113, B118, B119.

### 7.6 손 manifest 기록(2026-10-29)과의 비교 [M]

`hand_compare.txt`(`hand_compare_record.py`; site 대응은 부팅 전의 `hand_compare_c1_pre.txt`). 손 기록 `W/w6/run/c1nat/gi/pre_1df262a_tx/tx.dump.txt`(`9de99fd2…`, 205–240행; CF 행위자 13, BP 14), 이 기록(CF 12, BP 13, SCH_LAB 14).

- 둘 다 가진 값(같은 값): CF 조회의 반환 `0xffffffffffffffef`(-17)와 out `0x0`(호출자 after-call; 손 35 / 생성 288); 조회 API 안 `:212` 의 키 글 "BP0_tsem"(손 12 / 생성 273); `CF_CFDP_InitEngine`·`CF_Init` 반환 -17(손 33·31 / 287·285); `CFE_ES_ExitApp` 호출 인자 3 과 ES entry 3(손 37·9 / 286·6); BP 생성의 at-call 인자(초기 값 `0xa`)와 반환 0, out `0x30002`·`0x30003`(손 44·45 / 280·279); BP 의 존재 탐침 안의 키 글 "BP0_tsem"·"BP1_tsem"(손 12 / 생성 273, BP 행위자). 행위자의 task entry 값 `0x1000c`·`0x1000d` 도 두 기록에 있으나 손은 `OS_TaskEntryPoint`, 생성은 `OS_PthreadTaskEntry` 의 param0 이라 site 대응 밖이다.
- 손 기록에만 있는 것(42 사건)과 까닭: CF 호출자 at-call 의 키 글(생성은 호출자에서 포인터만 적는다: 호출자 쪽에 그 매개변수의 non-null 검사가 없어 gap `record_insufficient(key text)` 규칙이 글을 읽지 않는다; 글은 API 안에서 읽는다); `CF_Init`·`CF_CFDP_InitEngine`·`BP_FlowInit` 의 entry(규칙이 요구하지 않음); 조회 API entry 의 인자; BP 의 존재 탐침 호출(`bp_flow.c:126`, at-call 과 반환 -17)(다른 행 A20, 층 3 이라 예산에서 잘림; 생성은 API 안의 키 글만 가진다); `OS_CountSemCreate` entry, 게시 store, created(`OS_NotifyEvent`) 와 그 id(이 개정의 facts 에 게시·알림 규칙이 없음, §4.6).
- 이 기록에만 있는 것(이 행위자들의 281 사건)과 까닭: 생성자(행위자 1)의 생성 인자 at-call·after-call 과 owner store(생성 인자 묶기의 규칙), root entry `CF_AppMain`·`BP_AppMain`, run status store 3(`cf_app.c:333`), `CFE_ES_RunLoop` entry 와 반환 0(처리 사슬·멈춤 사건), SCH_LAB 의 생성("SCH_LAB", `0x30001`; 행의 생성자 둘째), 생성 API 안의 키 글(`osapi-countsem.c:91` 의 `memchr` 앞, "BP0_tsem"·"BP1_tsem"), 그리고 계획된 다른 행(B73, B81, B99 등)의 site 에서 이 행위자들이 낸 사건(BP 의 run loop 33 번, use-call 들).
- 손 기록은 사람이 사례를 보고 놓은 33 site 로 얻었고(판정 규칙 5.1), 이 기록은 규칙이 facts 에서 낸 301 site 로 얻었다. 관계 1 의 다섯 항목에 필요한 값(키, 반환, 처리 값, 종료 인자, 게시)은 두 기록이 같은 값으로 가진다. 손 기록에 없던 것은 행위자를 생성 호출에 잇는 값(생성 인자와 param0)과 run status·RunLoop 값이고, 이 기록에 없는 것은 호출자 쪽 키 글과 게시 store·알림이다.

### 7.7 문서 정정 [F][M]

- `plan/ovm_evidence_20261111/p_offline.md:27` 의 "Not in the record: event type, exit status value" 뒤에 "Correction (2026-11-16): …" 를 덧붙였다(원문은 그대로). 근거(`f0_exit_status.txt`): MC `kz-dump` `05623054` 로 푼 `W/run/c1nat/f0tx/pre_1df262a/run.kzt`(`d1e20517…`)와 `pre_1b0b338/run.kzt`(`c41adc9e…`) 모두 SITES[8] `CFE_ES_ExitApp` entry, SITE_SLOTS[21] slot 0 `role="exit_status"`, RAW_EVENTS[100] `a=[0x3,…]`. `run.kzt.record.txt` 에는 그 값이 찍히지 않는다(두 파일 모두 `exit_status`, `RAW_EVENTS`, `a[0]` 0줄). 사건 유형은 여전히 기록에 없다.
- 같은 폴더의 `SHA256SUMS` 에서 `p_offline.md` 줄만 새 sha256 으로 바꿨다. `sha256sum -c SHA256SUMS` 29 OK.

### 7.8 바이트 동일성, 로컬 묶음 [M]

- MC: `post_mc.txt` 는 `pre_mc.txt` 와 HEAD(`fbab3de` → `ab230a1`)와 여유 줄 밖에서 같다. 도구를 빌드하지 않았다.
- 누적 목록: `ledger_check.sh` 44행, 실패 0(새 행 둘). 입력(계획, 파생 작업 공간, B′, 부팅·변환 디렉터리, relations 재실행)과 F1T, W6·C5 기록, 도구 사본 모두 같다. `tools_check.sh --deep` 31행, 실패 0.
- 로컬 묶음 `obs1-20261116-cycle`(`/home/user/work/preserve/bundles-20261116/`; `bundle/`): 파일 4,554(plangen, c1pre_rt, c1pre_derived, c1pre_bprime, c1pre_boot, c1pre_convert, c1pre_explain), tar.xz 9,341,660 B, sha256 `4d2a0325…`, 조각 1. 스트림 검증: PARTS OK, xz -t OK, 멤버 4,554 의 sha256·크기 = MANIFEST.tsv(차이 0), 멤버 종류 파일·디렉터리만, 원본 = MANIFEST.sha256. VERIFIED_LOCAL 08:38:14Z. push 하지 않았다. 원격에서 다시 받아 확인한 것이 아니므로 "보존됨"이 아니다.
- 바깥 이름 점검(`--tree`): 묶음 디렉터리, `c1pre_explain`, 기록 디렉터리 깨끗함. 커밋 전 `--cached` 와 저장소 안 패턴 점검.

### 7.9 공간 [M]

| 시점 (UTC) | 여유 (df -B1 /) | 일 |
|---|---|---|
| 선언 커밋 전 | 3,742,834,688 | 선언 전(`pre_mc.txt` 와 같은 때) |
| 08:33:22 | 3,739,922,432 | 첫 explain(dip 2,519,040) |
| 08:36:14 | 3,736,305,664 | 최종 explain(dip 2,469,888) |
| 08:37–08:38 | 3,735,977,984 → 3,721,773,056 | 로컬 묶음(조각 9.3 MB, 검증 중 임시 디렉터리) |
| 끝 | 3,724,787,712 | 기록 뒤 |

- 하한을 넘지 않았다. 남긴 크기: `c1pre_explain` 7,091,104 B(세 실행과 facts 글 보기), 묶음 디렉터리 10,684,821 B.
- 지운 것: scratchpad 의 fixture 작업 디렉터리와 두 번째 explain 출력(결정성 비교 뒤). 그 밖에 아무것도 지우지 않았다.

### 7.10 계획과 다르게 한 것, 다음으로 넘기는 것

- 선언 뒤 스크립트를 세 군데 고쳤다(§7.1). 첫 실행(`explain`)은 선언한 코드 그대로의 출력으로 남겨 두었고, 판정 값은 세 실행에서 같다. 고친 것은 사슬 줄 읽기, 32-bit 상태 상수 맞춤, 효과 사건의 범위다.
- 설명 기록의 relations 입력은 T8CF 의 `relations.txt` 가 아니라 생성기 `de1315b3` 가 T8CF facts 에서 다시 낸 것(`plangen_relcheck/c1_pre_tx`, 개정 6, 앞 단계 출력과 바이트 동일)이다. 계획을 만든 도구와 규칙 판의 행이고, 사용 지점 키 파일이 함께 있다.
- 저장소에는 관계 1 의 기록 하나와 모든 기록의 sha256 을 두었다. 45 기록 모두(2.3 MB)는 저장소 밖과 로컬 묶음에 있다.
- `intervention` 의 differs·same 은 이 단계에서 나올 수 없어 fixture 도 unconfirmed 만 본다.
- 경로 B 의 link 는 모두 unlinked 다: 계획이 사용의 읽은 주소를 적지 않는다(at-call 의 `field_value` 만). 같은 인스턴스를 보이려면 계획 규칙에 읽은 주소가 필요하다(다음 계획 개정의 일).
- L-ABS(경로 A 부재 결과의 연결)는 lead 의 판단이고 PI 가 달리 정하면 바뀐다(판정은 바뀌지 않음, §7.3).
- 다음으로: 실행 기록 45 가운데 실패 징후 둘(A21, B73 행위자 2)의 사건은 이 기록에 있다. B73 의 `CFE_EVS_APP_ILLEGAL_APP_ID` 가 무엇을 뜻하는지는 이 기록이 말하지 않는다 [미확인]. 사용 조건의 (c2)·(c3)·(c4)를 세우는 규칙(함수를 건너는 부재 평가, 순서 간선, 완전성 규칙)은 전체 단계의 일이다. C5(E6)는 생성 토큰과 공간 선택 뒤다. t_init 와 하네스 시각의 차이, 시작 동기화 지점의 여유는 [미확인] 그대로다.

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

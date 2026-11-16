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

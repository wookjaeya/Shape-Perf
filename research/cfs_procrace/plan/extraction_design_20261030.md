# 의존성·자원·관측점 자동 추출 — 단계 2 출발점 측정과 설계 (2026-10-30)

- 성격: **측정 보고와 설계 문서**다. 구현 완료나 탐지 결과가 아니다. 엔진 코드(`kairos/lib`, `include`, `tools`)는 바꾸지 않았다. cFS를 기동하지 않았다. 정적 분석(kz-static)과 텍스트 처리만 했다.
- 근거: `plan/REDESIGN_REVIEW_ANALYSIS_20261030.md`(이하 RA) §1 E1–E3, §3.1, §4; 사용자 업로드 검토서 RV2(`Kairos_Redesign_Review_and_Reinforced_Spec_20261030.md`) §3–§5, §8, §11–§12; `plan/REDESIGN_PROPOSAL_20261029.md`(이하 RP); `kairos/docs/static_facts.md`(이하 SF).
- 산출물: 드라이버 `env/redesign/extract_baseline.sh`와 계수기 `env/redesign/extract_baseline_count.py`. 출력은 저장소 밖 `/home/user/work/redesign/extract_baseline/`(이하 `EB/`), 해시는 `EB/sha256.txt`.
- 표기: [M] 이번에 도구로 확인, [F] 소스·문서에서 이번에 읽음, [I] 추론·설계, [D] 이번 문서의 결정, [U] 미확인.
- 사례 지식의 쓰임: 사례 이름(CF, BP, SB, #72 …)은 **출력을 들여다볼 때만** 썼다(§2.4, `EB/count/eval_*.txt`). 추출 입력·규칙·관측점 생성에는 쓰지 않았다(§1.3).

## 0. 요약

1. **사례 프로필 없이도 kz-static은 대부분의 기초 사실을 낸다** [M]. 호출 지점, 인자 분류(상수·문자열·필드 읽기·필드 주소·함수·top), 결과 사용·검사 플래그, 루프의 종류·횟수, table walk 펼치기와 그 순서, 간접 호출 대상, 함수마다 직접 쓰는 필드(FUNC_WRITES)가 그렇다. 세 빌드에서 이 사실들은 프로필 유무와 무관하게 행 번호만 빼면 같다(§2.2).
2. **사례 프로필이 더하는 것은 세 종류뿐이다** [M].
   - (a) 대상 필드의 writer 집합·A-TYPE·INIT·out-pointer 사실(대상 선택).
   - (b) 역할 있는 인자의 copy chain(derives, 역할 선택).
   - (c) 연산 플래그(spawn·delay·lock·terminal)와 그에서 나오는 루프 분류·spawn 사실·terminal 사실.
   - (c)는 사례와 무관한 9개 이름(G0, §1.2)으로 대체된다. 루프 분류·task 순서·spawn 매개변수는 같은 줄이 나오고, terminal·spawn 대상은 G0 쪽이 더 넓다.
   - (a)는 규칙으로 고른 대상 집합(R-T1)으로 대체된다. R-T1에 든 사례 필드는 writer 수가 모두 같다. 다만 분기 조건에서만 읽히는 사례 필드 6개(C1 1, R2 3, C5 2)는 R-T1에 들지 않는다(§2.3).
   - (b)는 엔진의 역할 게이트(`ast_fn.cpp` L1665–L1666) 때문에 대체되지 않는다.
3. **지금 사실만으로는 경로 A/B/C의 관계가 나오지 않는다** [M][I]. 빠진 것은 필드 읽기 사실, 반환값 사실, 모든 루프의 탈출식, 역할 없는 인자의 흐름, 호출·쓰기마다의 잠금 집합, 다단계 spawn 래퍼 유도, table walk 밖 전역의 초기 이미지다(§2.5). 모두 지금 Clang AST 순회를 넓히는 일이다. 새 front end는 필요 없다.
4. **감사 사실** [M][F]
   - `--profile`은 basis·extract 모두 필수다. 로더가 받는 최소 입력은 `phi 1; profile "<이름>";` 두 문장이고, 어휘는 0이다.
   - 추출 결과가 판정 프로필의 해시에 묶인다(`cmd_replay.cpp` L341). 그래서 사례 프로필 없이 뽑은 facts는 지금 재생 경로에 넣을 수 없다.
   - `writer-set-complete`는 대상 집합에 따라 바뀐다. R2의 `CFE_SB_Global_t.AppId`는 사례 대상(V2)에서는 complete, 규칙 대상(V3)에서는 DISABLED다. writer 3개는 같다(§2.3).
5. **규칙으로 만든 관측점(원형)** [M]
   - 수작업 manifest 사이트 가운데 C1 29/37(수정 후 30/38), R2 34/44, C5 26/33을 **함수 수준에서** 덮는다.
   - 다만 상당수는 다른 이유(그 함수 안의 잠금 호출 등)로 덮인다. 수작업이 고른 분기·값 지점과 같지 않다.
   - 빠진 곳은 깊은 이름 조회 체인(C1), EVS 등록·정리(R2), OSAL 등록 검색과 내부 GetAppID(C5)다. 셋 다 읽기 사실과 비교 검출이 없어서 생긴다(§4).
   - 규칙은 수작업보다 한두 자릿수 많은 지점을 낸다. 예: C1 tx의 OP-B는 1,131개다.
6. **다음 단계**: 단계 2a는 사실 확장 7종(F1–F7)과 사례 무관 질의기다. 완료 시험은 §6이고, 세 사례의 관계를 사례 입력 없이 재현하는지, 그리고 반증 조건을 본다.

## 1. 무엇을 어떻게 쟀나

### 1.1 빌드 [M]

모두 **기준 빌드**다. 파생(계측) 빌드는 쓰지 않았다.

| 이름 | 트리 | compile DB | recipe(닫힘 기술) | 번역 단위 / 함수(본문) / 호출 지점 |
|---|---|---|---|---|
| `c1_pre_tx`, `c1_pre_rx` | `procrace/c1_build/pre/1df262a/build`, bundle `bundle_1df262a`, CF·BP 소스 `pre/src` | `build/native/default/compile_commands.json` | `c1_build/kz/pre_1df262a/closure[_rx].kzb`(`env/c1/kz_run.sh recipes`가 빌드에서 씀) | 177 / 2180(1981) / 7908 |
| `c1_post_tx`, `c1_post_rx` | `post/1df262a` | 같은 꼴 | `closure[_rx].kzb` | 177 / 2180(1981) / 7909 |
| `r2_core` | `procrace/cfs_ref/cFS/build-native_std`, core-cpu1 | `native/default_cpu1/compile_commands.json` | `core-cpu1.kzb`에서 `generated`·`table`·`declared` 줄을 뺀 것(`EB/inputs/r2_core_min.kzb`) | 168 / 1591(1409) / 5948 |
| `r2_all` | 같은 빌드, 실행 파일 + 시작 스크립트가 적재하는 모듈 17 | 같음 | 드라이버가 규칙으로 씀(`EB/inputs/r2_all.kzb`, §1.4) | 271 / 2718(2523) / 11217 |
| `c5_641` | cFE 6.4.1 기준 빌드 `c5tools/build/641/keep/cFE-6.4.1-OSS-release` | E-ST가 빌드 로그에서 만든 `c5tools/build/641/est/compile_commands.json`(58) | `profiles/c5-cfe-6.4.x/basis/core-linux.kzb` 그대로 | 58 / 771(652) / 3412 |

도구는 병합 트리 빌드 `merge-check/kz-llvm/tools/kz-static/kz-static`(tool_sha256 `9c6902db…`, `EB/inputs/inputs.record`)이다. 같은 입력으로 두 번 돌려 facts 28개가 바이트 단위로 같았다 [M].

### 1.2 변형

| 변형 | 프로필 | 명령줄 어휘 | 뜻 |
|---|---|---|---|
| V0 | 두 문장짜리 빈 프로필(`EB/inputs/bare/00_profile.kzp`) | 없음 | 필드·연산·역할·spawn·잠금·지연·종료를 아무것도 이름 붙이지 않음 |
| V0p | V0 | `--spawn-op pthread_create` | 호스트 스레드 라이브러리의 생성 기본 연산 하나만 씨앗으로 줌 |
| V1 | V0 | G0(아래) | 사례와 무관한 cFE/OSAL 기본 연산 이름 9개 |
| V2 | 사례 프로필 디렉터리(사례 드라이버와 같음) | 사례 드라이버의 4개 + C1은 `--target` 7개 | `env/c1/kz_run_c1.sh`, `env/r2/replay_w6.sh` closures, `env/c5/replay_c5.sh` bprime의 인자 |
| V3 | V0 | G0 + 규칙 R-T1 대상 | R-T1: V0 facts에서 호출 인자에 닿는 모든 `Type.field`(`arg i:symbolic T.f`, `arg i:addr-of-field … of T.f`). 목록은 드라이버가 씀(`EB/<build>/targets_rule.txt`) |
| V0r | V0 | 없음 | r2만. 원래 recipe `core-cpu1.kzb`로 V0을 다시 뽑음 |

**G0**(9개): spawn `OS_TaskCreate`, `CFE_ES_CreateChildTask`; delay `OS_TaskDelay`; lock/unlock `CFE_ES_LockSharedData`/`CFE_ES_UnlockSharedData`; terminal `CFE_ES_ExitApp`, `CFE_PSP_Restart`, `CFE_PSP_Panic`, `abort`.

- 사례 프로필과 사례 드라이버가 함께 쓰던 이름 가운데 판(cFE/OSAL)의 기본 연산만 골랐다. 각 이름이 닫힘 안에 본문으로 있는지는 `EB/count/g0_check.tsv`에서 확인한다. `abort`는 C1·R2에서 선언만 있고, C5에는 없다 [M].
- 본문을 따라가면 G0은 호스트 기본 연산으로 내려간다(C1 V0 호출 사실) [M].
  - `CFE_ES_LockSharedData → OS_MutSemTake → OS_MutSemTake_Impl → pthread_mutex_lock`
  - `OS_TaskDelay → OS_TaskDelay_Impl → clock_nanosleep`
  - `CFE_ES_ExitApp → OS_TaskExit → OS_TaskExit_Impl → pthread_exit`
- 그래서 G0은 §3.0의 P0(호스트 기본 연산 표)와 래퍼 유도로 대체할 수 있다 [I].

### 1.3 입력 감사: 프로필 없이 도는가 [M][F]

| # | 사실 | 근거 | 뜻 |
|---|---|---|---|
| A1 | `kz-static basis`는 `--profile`, `--compile-db`, `--build-dir`, `--bundle-root`, `-o`를 요구한다. `extract`는 `--profile`, `-o`, `--basis`/`--source` 가운데 하나를 요구한다 | `tools/kz-static/cmd_basis.cpp` L22–L26, `cmd_extract.cpp` L100–L102 | 프로필 없는 실행은 CLI가 막는다 |
| A2 | 빈 파일은 "expected 'phi'"로, `phi 1;`만 있는 파일은 "the profile has no name"으로 거부된다. `phi 1; profile "x";`는 받는다. `revision`은 없어도 된다 | `EB/inputs/inputs.record`(loader probe: 시험 fixture `tests/static/fixtures/spawn.c`에 네 가지 프로필로 extract), `lib/core/phi_check.cpp` L1411 | **최소 입력은 두 문장이고 어휘가 0이다.** 이번 V0은 이것을 썼다. 줘야 했던 항목은 이름 하나다. 이유는 로더가 이름 없는 프로필을 받지 않기 때문이다 |
| A3 | 프로필 해시가 facts 머리에 들어가고, `kz replay --facts`는 해시가 다르면 거부한다 | `tools/kz/cmd_replay.cpp` L341, SF §2 | **추출 산출물이 판정 프로필에 묶여 있다.** 사례 무관 추출을 재생에 넣으려면 이 결합을 끊어야 한다 |
| A4 | 프로필에 `source`가 없으면 basis는 소스 트리 commit을 대조하지 않는다("source trees 0 checked"). 파일 해시만 남는다. V2는 C1 5/3, R2 4/4를 대조했다 | `EB/*/V0/basis.txt`, `EB/*/V2/basis.txt` | 출처 고정이 프로필에 들어 있다. 빌드 입력(트리의 git 정보)에서 직접 읽도록 옮길 수 있다 [I] |
| A5 | 내장 데이터 함수 표 12개(memset 계열)는 늘 들어가고 `researcher` 계층으로 기록된다 | `lib/llvm/spec.cpp` `spec_add_builtin_data_fns` | 사례와 무관한 C 라이브러리 의미다. P0과 같은 판 관리 대상이다 |
| A6 | recipe의 `startup_script` 블록(reset 종류, 파일 규칙, mount, 진입 단어의 기호 범위, `module_file`)은 cFE 적재기 의미를 사람이 적은 것이다. 근거 줄(`cite`)이 붙는다 | `core-cpu1.kzb`, `closure.kzb`, `core-linux.kzb` | 판마다 한 번 쓰는 적재기 의미(L0)다. 사례 무관이지만 수작업 입력이므로 공개한다 |
| A7 | R2 recipe의 `generated`·`table`·`declared` 줄(사람이 고른 basis 항목)은 facts를 바꾸지 않는다. V0과 V0r의 facts 텍스트 차이가 0줄이다 | `EB/compare/r2_V0_vs_V0r.diff` | basis 기록에만 영향을 준다 |
| A8 | C5 recipe의 `front_end_diagnostic` 2줄(PI D7)이 없으면 `osapi.c`·`cfe_psp_start.c`가 front end에서 parse되지 않는다 | `c5tools/build/641/est/est.record`(control none: extract rc=1) | 이미 deviation으로 기록된 분석 전용 조치다. 사례 지식이 아니다 |
| A9 | V0의 basis에도 손으로 넣은 항목이 1개 있다(`startup.reset_type = POWERON`, declared-assumption) | `EB/*/V0/basis.txt` | 실행 입력(reset 종류)이다. RV2 §4.1의 "실행 입력"에 든다 |

### 1.4 조건 기록 (`CONDITIONS_POLICY.md`)

| 조건 | 값 | 상태 | 근거·이유 |
|---|---|---|---|
| 분석 대상 빌드 | §1.1의 기준 빌드 | from-reference | 각 사례의 기존 기준 빌드(C1 `plan/C1_build_manifest.md`, R2 `env/REBUILD_20261029.md`, C5 `plan/c5_execution_basis_20261028.md` §2) |
| C1 후보 | 1df262a만 | from-reference(과제 지시) | 1b0b338은 이번에 재지 않았다 |
| R2 닫힘 | core-cpu1 단독(사례와 같음)과 모듈 포함(`r2_all`) | `r2_all`은 **unspecified_by_reference** | 모듈 줄은 시작 스크립트의 각 줄 → `native/default_cpu1/apps/<m>/CMakeFiles/<m>.dir/link.txt`라는 규칙으로 썼다. `startup_script`의 모듈 규칙은 이 트리에서 읽은 줄을 인용한다(`cfe_es_apps.c` L465–L495, `os-impl-posix-dl-loader.c` L72–L96, `cfe_fs_api.c` L53–L110, `target_config.c` L50–L52, L223–L225) |
| G0 | 9개 이름 | **가정(assumption)**, 레퍼런스가 정하지 않음 | 판의 기본 연산이다. 사례 드라이버들의 공통 부분을 그대로 썼다. §3.0에서 P0 + 래퍼 유도로 대체한다 |
| R-T1 | 호출 인자에 닿는 필드 전부 | 도구 규칙(연구 조건 아님) | 사람이 고르지 않은 대상 집합의 한 예다. 다른 규칙(R-T2, §3.1)과 비교할 기준이다 |
| front end | Clang 23.0.0git LibTooling | deviation(기존 기록 그대로) | SF §3 |

## 2. 측정 결과

### 2.1 V0이 내는 사실 [M]

`EB/count/{calls,args,loops,fwrites,closure}.tsv`

| 항목 | c1_pre_tx | r2_core | r2_all | c5_641 |
|---|---|---|---|---|
| 호출 지점 / 결과 사용 / 결과 검사 / 결과 버림 | 7908 / 3933 / 2337 / 3975 | 5948 / 3190 / 1826 / 2758 | 11217 / 5405 / 3432 / 5812 | 3412 / 1230 / 928 / 2182 |
| 루프 안 호출 | 1123 | 625 | 1536 | 407 |
| 인자: 상수 / 문자열 / 필드 읽기 / 전역 / 필드 주소 / 객체 주소 / 함수 / top | 4618 / 1897 / 1901 / 102 / 1064 / 1148 / 124 / 6860 | 3195 / 1427 / 1114 / 75 / 454 / 1158 / 68 / 5287 | 7429 / 2944 / 2853 / 185 / 1137 / 1744 / 125 / 9435 | 2104 / 1036 / 950 / 94 / 219 / 660 / 6 / 2400 |
| 간접 호출(해결/미해결) | 97(79/18) | 54(47/7) | 99(61/38) | 11(2/9) |
| 루프: table walk / constant-trip / other; unroll / star-closed | 0 / 95 / 316; 95 / 316 | 3 / 55 / 199; 58 / 199 | 3 / 197 / 411; 200 / 411 | 0 / 140 / 143; 140 / 143 |
| table walk 펼친 행 / static image 행 / order 줄 | 0 / 0 / 0 | 314 / 72 / 2 묶음 | 314 / 72 / 2 | 0 / 0 / 0 |
| FUNC_WRITES 행 / 서로 다른 `T.f` / 함수 | 4912 / 2028 / 1101 | 2969 / 1383 / 726 | 6640 / 2593 / 1472 | 2136 / 898 / 391 |
| writer·A-TYPE·INIT·derives·terminal·spawn | 모두 0 | 모두 0 | 모두 0 | 모두 0 |

- **결과 검사 플래그는 "검사했다"만 말한다.** 어떤 상수와 비교했는지, 어느 분기로 가는지는 없다(`ast_fn.cpp` `call_context` L1350–L1387) [F].
- **필드 쓰기는 함수마다 필드별 첫 지점만** 남는다(FUNC_WRITES). 값과 guard가 없고, 필드 읽기 사실은 없다 [F].
- table walk 펼치기는 프로필 없이 된다. r2의 `CFE_ES_CreateObjects`에서 EarlyInit 순서(CONFIG, ES, EVS, FS, SB, TBL, TIME)가 V0에서 이미 나온다. task 생성 순서는 spawn 연산이 있어야 나온다(V1: ES, EVS, SB, TBL, TIME가 반복 1, 2, 4, 5, 6) [M].

### 2.2 프로필이 더하는 것과 그것을 사례 없이 대체하는 것 [M]

`EB/count/variant_vs_V0.tsv`. 행 번호(`site N`, `loop N` …)를 빼고 줄 multiset을 비교했다. 정렬 키가 바뀌면 행 번호가 밀리기 때문이다. C1 V2는 site 번호가 밀린 구간이 있어, 플래그·역할을 빼고 원문 줄로 비교해도 14,286줄이 다르게 나온다.

| 절 | V2(사례) − V0 | V1(G0) − V0 | V3(G0+R-T1) − V0 | 차이를 만든 입력 |
|---|---|---|---|---|
| calls | C1 ±147, R2 ±225, C5 ±102 | C1 ±182, R2 ±175, C5 ±140 | V1과 같음 | 연산 플래그(spawn/delay/lock/terminal), V2는 인자 `role=` |
| loops | C1 +63/−18, R2 +47/−14, C5 +30/−10 | **같음** | 같음 | delay 연산 → count-bounded·poll 분류와 탈출식 |
| loops rule await | R2 1 | 0 | 0 | `CFE_ES_MainTaskSyncDelay`의 L1 (i)는 술어가 읽는 필드가 추출 대상이어야 성립한다(SF §5.2). R-T1은 그 필드(`AppState` 등)를 고르지 않는다 |
| order / iters | R2 +6 / ±5 | **같음** | 같음 | spawn 연산 → task-main 순서 |
| targets / spawnparams | spawn-entry +4…+6, 1 | **같음**(C5는 V1이 더 많음: `OS_TaskCreate` 포함) | 같음 | spawn 연산 |
| terminals | C1 14, R2 37, C5 0 | C1 51, R2 46, C5 35 | 같음 | 종료 연산(G0이 더 넓음) |
| writers / locs / atype / init / outcalls | C1 33/8/268/7/3, R2 26/6/0/6/1, C5 10/2/0/2/0 | 0 | C1 3194/841/92189/561/1054, R2 2064/504/31823/397/443, C5 1342/322/18213/228/219 | 대상 필드 |
| derives | C1 2, R2 59 | 0 | **0** | 역할 인자. 역할 없는 인자에는 계산하지 않는다(`ast_fn.cpp` L1665–L1666) |

spawn 사실의 수 [M]:

- V1의 spawn 호출(해결된 entry): C1 6(6), R2 core 4(4), R2 all 15(15), C5 6(4).
- V0p(`pthread_create` 씨앗)의 spawn 호출: C1 1, R2 core 2(`linux_sysmon` 포함), C5 1. 유도된 task-main 매개변수는 각 1이다.

V2의 facts는 사례 드라이버가 예전에 만든 닫힘과 같다. 머리 줄을 빼면 차이가 0줄이다: C1 pre `3b7c9d78…`, post `b8155005…`, R2 `1f97e8f3…`(`EB/compare/compare.txt`). 그래서 위 차이는 도구 판이 아니라 프로필의 효과다 [M].

### 2.3 규칙 대상(V3)의 writer 집합 [M]

| 빌드 | 대상 필드 | complete / DISABLED | writer 0 / 1 / 2–5 / >5 | DISABLED 원인(겹침): untyped / escape / cast / union / unknown-effect |
|---|---|---|---|---|
| c1_pre_tx | 841 | 208 / 633 | 174 / 107 / 422 / 138 | 633 / 244 / 435 / 154 / 151 |
| r2_core | 504 | 200 / 304 | 101 / 52 / 257 / 94 | 304 / 137 / 196 / 33 / 12 |
| r2_all | 1154 | 313 / 841 | 306 / 122 / 510 / 216 | 841 / 298 / 491 / 99 / 186 |
| c5_641 | 322 | 61 / 261 | 91 / 68 / 115 / 48 | 261 / 111 / 171 / 0 / 4 |

사례 프로필 필드와 비교(`EB/count/summary.txt` 끝):

- **writer 수는 모두 같다.**
  - C1: `OS_common_record_t.name_entry` 18, `active_id` 6, `BP_Throttle_t.Sem` 2, `.Name` 0, `CF_Channel_t.sem_id` 2(DISABLED), `CF_ChannelConfig_t.sem_name` 0(DISABLED).
  - R2: `CFE_SB_Global_t.AppId` 3(EarlyInit memset 0; `CFE_SB_AppInit`이 `CFE_ES_GetAppID` out-pointer로 쓰는 둘: `AppRecPtr != NULL`이면 유효 ID, 아니면 `CFE_ES_APPID_UNDEFINED`), `EVS_AppData_t.AppID` 4, `CFE_ES_AppRecord_t.AppId` 5.
- **상태는 하나가 다르다.**
  - `CFE_SB_Global_t.AppId`가 V2에서는 complete, V3에서는 DISABLED(untyped 99)다.
  - 원인: 같은 형의 다른 필드(`CmdPipe`, `InTransitList`, `ZeroCopyList`)도 대상이 되자 그 주소의 escape 6건이 형 노출로 잡혔다. 노출된 형의 모든 대상에 untyped write 99건이 걸렸다.
  - SF §4.1의 규칙("untyped write는 대상 형에 노출이 있을 때만 센다")은 노출을 **대상 필드에 대해서만** 본다. 그래서 형 단위 판정이 대상 집합에 따라 달라진다(비단조) [M].
  - 사례 재생에 쓴 "complete"는 형제 필드의 escape를 보지 않은 판정이었다. 형제 필드 주소에서 이 필드로 넘어가는 쓰기는 C에서 객체 경계를 넘는 포인터 연산이라 정의되지 않은 동작이다. 그래서 V3 쪽이 과하게 보수적일 가능성도 있다 [I]. 어느 쪽이든 **완전성 판정 규칙이 대상 선택에 독립이어야 한다**(§5 R3).
- **R-T1이 놓치는 사례 필드**가 있다. C5 `CFE_ES_TaskRecord_t.RecordUsed`·`.TaskId`, R2 `CFE_ES_AppRecord_t.AppState`·`.Type`, `CFE_PSP_IdleTaskState_t.ShutdownReq`는 호출 인자로 넘어가지 않고 **분기 조건에서 읽힌다.** 그래서 대상 규칙에는 "결과 분기·out 저장을 정하는 조건이 읽는 필드"(R-T2, §3.1)가 필요하다.

### 2.4 사례 지점에서 지금 사실이 보여 주는 것 (평가용 조회) [M]

`EB/count/eval_C1.txt`, `eval_R2.txt`, `eval_C5.txt`. 사례 이름은 조회에만 썼다.

**C1 (V0)**

- count-semaphore 호출 4곳이 모두 나온다.
  - BP: `initialize_throttling` `bp_flow.c:126` 조회, `:132` 생성. 둘 다 루프 안(trip exact 2)이고, 인자 1은 필드 읽기 `BP_Throttle_t.Name`이다.
  - CF: `CF_CFDP_InitEngine` `cf_cfdp.c:993`(수정 후 `:1003`) 조회. 인자 1은 `CF_ChannelConfig_t.sem_name`이다.
  - SCH_LAB: `sch_lab_app.c:168` 생성. 인자는 문자열 상수 `"SCH_LAB"`다.
  - 넷 모두 결과를 검사한다.
- OSAL 이름 표 기계도 나온다.
  - `OS_ObjectIdFindNextMatch`의 루프(`osapi-idmap.c:550`)와, 그 루프의 간접 호출 대상 `OS_ObjectNameMatch`·`OS_TaskIdMatchSystemData_Impl`·`OS_FileSys_FindVirtMountPoint`.
  - `OS_common_record_t.name_entry`를 쓰는 함수 17개. 그중 `OS_CountSemCreate`(`osapi-countsem.c:100`)이 이름을 넣고, `OS_ObjectIdFindNextFree`(L640)는 NULL로 비운다.
  - `active_id`를 쓰는 함수 `OS_ObjectIdFindNextFree`(L639), `OS_ObjectIdTransactionFinish`(L1079), `OS_ObjectIdConvertToken`(L432).
- **수정 후 처리 사실이 V1에서 프로필 없이 나온다.**
  - `cf_cfdp.c:1001` 루프: count-bounded, trip upper-bound(25), 탈출식 `(or (not (lt j 25)) (and (lt j 25) (ne ret -17)))`.
  - mod set에 `OS_CountSemGetIdByName`의 결과가 있고, 지연 호출이 있다.
  - -17은 `OS_ERR_NAME_NOT_FOUND`다(`osapi-error.h` L113 [F]). 25는 `CF_STARTUP_SEM_MAX_RETRIES`다(`cf_platform_cfg.h` L338 [F]).
- **수정 전의 종료 경로는 아직 사실로 이어지지 않는다.**
  - `CF_AppMain`의 `CFE_ES_ExitApp` 호출(`cf_app.c:366`, 인자 `CF_AppData_t.run_status`)은 V1 terminal로 나온다.
  - 그러나 조회 실패 → `break` → `return ret` → `CF_Init` → `run_status = APP_ERROR` → `CFE_ES_RunLoop` 거짓 → `CFE_ES_ExitApp`로 가는 반환값·필드 흐름은 사실에 없다.
- **키 값**
  - BP의 키는 `BP_Throttles`의 정적 초기값이다(`bp/fsw/custom/bp_semcfg.c` L68–L70 [F]). 그런데 이미지가 table walk 대상에만 기록되어 facts에 없다. V3에서 `BP_Throttle_t.Name`은 writer 0, complete다.
  - CF의 키는 실행 중 `CFE_TBL_Load`로 읽는 표(`cf_def_config.c`, 링크 닫힘 밖의 표 원본)에서 온다. V3에서 `CF_ChannelConfig_t.sem_name`은 DISABLED다(untyped 133, escape 4, cast 1).

**R2 (V0)**

- `CFE_SB_Global_t.AppId`를 복사 없이 `CFE_EVS_SendEventWithAppID`의 인자 2로 넘기는 지점이 39곳이다. 파일별로 `cfe_sb_api.c` 34, `cfe_sb_priv.c` 1, `cfe_sb_task.c` 4다. SF §6과 `results/R2_gdb_v701.md` §6의 39(34/1/4)와 같다. 사례 프로필의 derives와도 같다.
- **39곳 모두 결과를 버린다**(used 0).
- 같은 인자 자리의 나머지는 `CFE_TBL_Global_t.TableTaskAppId` 19, top 1이다.
- 필드 쓰기: `CFE_SB_EarlyInit`의 record 쓰기(`cfe_sb_init.c:50`)와 `CFE_SB_AppInit`의 주소 인자(`cfe_sb_task.c:138`). V3에서는 writer 3개와 값·guard까지 나온다(§2.3).
- callee 쪽 사실:
  - `CFE_EVS_SendEventWithAppID → EVS_GetAppDataByID`(결과 검사) `→ CFE_ES_AppID_ToIndex`(결과 검사).
  - `EVS_AppDataIsMatch → CFE_ResourceId_Equal(EVS_AppData_t.AppID, …)`.
  - 그러나 "NULL이면 `CFE_EVS_APP_ILLEGAL_APP_ID`"라는 반환 사실은 없다(`cfe_evs.c` L185–L189 [F]).
- V1은 코어 task 순서 ES, EVS, SB, TBL, TIME(table walk 반복 1, 2, 4, 5, 6)을 낸다.
- 정적 행위자 귀속(§4)으로 보면 다음과 같다.
  - 39곳이 든 SB API 함수: `CFE_ES_TaskMain`, `CFE_EVS_TaskMain`, `CFE_SB_TaskMain`, `CFE_TBL_TaskMain`, `CFE_TIME_TaskMain`가 닿는다.
  - `CFE_SB_AppInit`: task-main root 가운데 `CFE_SB_TaskMain`만 닿는다. 래퍼 root `CFE_ES_TaskEntryPoint`·`OS_PthreadTaskEntry`는 간접 호출로 모든 task-main에 닿으므로 구별에 쓰지 않는다(§4.1).

**C5 6.4.1 (V0/V1)**

- `CFE_ES_AppCreate`의 호출 순서가 줄 번호와 함께 나온다.
  - lock L571, unlock L610, `OS_TaskCreate` L614(spawn, entry `StartAddr`은 미해결), lock L637, unlock L673.
  - 같은 함수의 `CFE_ES_TaskRecord_t.RecordUsed`(L649)·`AppId`(L651)·`TaskId`(L652) 첫 쓰기는 모두 spawn 뒤, L637–L673 구간에 있다.
  - `OS_TaskCreate`의 인자 0은 `&AppTable[i].TaskInfo.MainTaskId`(필드 주소)다. 생성자가 받는 정체성이 여기다.
- 자식 쪽 사실은 다음과 같다.
  - `CFE_ES_RegisterApp`: lock L771, `OS_TaskRegister` L776.
  - `CFE_ES_GetAppID`: lock L851, `OS_TaskGetId` L856(결과 사용·검사), unlock L867.
  - `OS_TaskRegister`: `pthread_self` → `pthread_setspecific`.
  - `OS_TaskGetId`: `pthread_getspecific`.
  - **`TaskTable[TaskId].RecordUsed` 검사와 `AppId`의 out 저장은 사실에 없다**(읽기 사실 없음).
- OSAL 4.1.1의 task 표는 다음과 같다.
  - `OS_task_record_t.id`는 `OS_TaskCreate`가 `pthread_create`에 주소로 넘긴다(L645, 본문 없는 함수 → V3에서 escape → DISABLED).
  - `id`·`free`를 지우는 쓰기는 `OS_TaskDelete`(L761, L766)와 `OS_TaskExit`(L793, L798)뿐이다.
  - `OS_TaskRegister`의 검색 루프(L921)는 `free`를 보지 않고 `id == pthread_self()`만 비교한다(`osapi.c` L921–L927 [F]). 이 비교는 문자열이 아니라 `==`라 §4의 비교 검출 원형이 놓친다.
- spawn 해결
  - `CFE_ES_CreateChildTask`·`CFE_TIME_TaskInit`·`CFE_ES_PerfStopDataCmd`의 entry는 해결된다.
  - `CFE_ES_AppCreate`(`OS_SymbolLookup`의 out `StartAddr`)와 `CFE_ES_CreateObjects`(`CFE_ES_FuncPtrUnion_t.VoidPtr`)의 entry는 미해결이다.
  - V0p(`pthread_create` 씨앗)는 `OS_TaskCreate`의 매개변수 2를 entry 인자로 유도한다(한 단계).

### 2.5 없는 것 (지금 사실의 공백) [M][F]

| # | 공백 | 위치 | 필요한 곳 |
|---|---|---|---|
| G1 | 필드 **읽기** 사실이 없다. 분기 조건·반환값·out 저장에 쓰인 읽기가 그렇다 | FUNC_WRITES만 있음(`serialize.cpp` L512–L533) | A(검색·검사 필드), C(자식의 등록 확인), 반사실 신호 |
| G2 | **반환값** 사실이 없다(return 식과 guard) | — | 조회 결과 코드, callee 검사의 오류 반환, 처리 분류 |
| G3 | 탈출식이 **지연 호출이 있는 루프와 table walk에만** 기록된다 | `ast_fn.cpp` L699–L700 | 검색 루프의 일치 조건, 지연 없는 재시도 |
| G4 | copy chain(derives)이 **역할 인자에만** 있고, 매개변수 → callee 매개변수 흐름이 없다 | `ast_fn.cpp` L1665–L1666, `KZ_DERIVES_REASON_FROM_PARAM` | B(필드 → 인자, 지역 변수·매개변수 경유), A(키 인자 → 비교) |
| G5 | 결과 검사 플래그에 **비교 상수와 분기**가 없다 | `call_context` | 처리 분류, 반사실 신호 |
| G6 | 호출·쓰기마다 **잠금 집합**이 없다. 루프의 잠금 구간만 있다 | `lock_depth_`는 루프용 | C(spawn–등록 창), 실현 가능성, 보류 계획 거부 |
| G7 | spawn 래퍼 유도가 **한 단계**다. `pthread_create` 씨앗으로 C1·R2는 `OS_Posix_InternalTaskCreate_Impl`까지만, C5는 `OS_TaskCreate`까지만 간다 | SPAWN_PARAMS(SF §5.4) | 생성 연산의 자동 인식(G0 제거) |
| G8 | static image가 **table walk 대상에만** 있다 | SF §5.3 | 키 값(BP `BP_Throttles`), 코어 객체 표(C5 `CFE_ES_ObjectTable`) |
| G9 | 시작 스크립트 진입 기호 → `OS_SymbolLookup` → spawn entry 해결이 없다. 함수 포인터 union(`VoidPtr`)도 해결되지 않는다 | spawn-entry UNRESOLVED 2(C5) | C의 자식 코드 귀속 |
| G10 | 본문 없는 호스트 함수의 요약이 데이터 함수 12개뿐이다(`pthread_create`의 `*thread` 쓰기 없음) | `spec.cpp` | C5 `OS_task_record_t.id`의 writer(지금은 escape) |
| G11 | 완전성 판정이 대상 집합에 따라 바뀐다 | §2.3 | 규칙 대상으로 넘어갈 때 상태 해석 |

## 3. 설계

### 3.0 공통 원칙과 새 사실

**입력 출처 추적** [D]. 모든 관계·후보에 그 근거가 어느 입력 계층에서 왔는지 붙인다.

| 계층 | 내용 | 관리 |
|---|---|---|
| B | 빌드: 소스, compile DB, link, 실행 파일·모듈, 시작 스크립트, 실행 입력(reset 종류) | 사용자 제공(RV2 §4.1) |
| P0 | 호스트 스레드 라이브러리·C 라이브러리 기본 연산의 의미. `pthread_create`(인자 0에 새 thread id를 씀, 인자 2가 새 스레드의 시작 함수), `pthread_mutex_lock/unlock`, `sem_*`, `pthread_cond_*`, `clock_nanosleep`/`nanosleep`, `pthread_exit`/`exit`/`abort`, `pthread_self`, `pthread_[gs]etspecific`, `dlopen`/`dlsym`, 데이터 함수 12개 | 도구 내부 지원. **판 표기**(POSIX 문서, glibc 판)와 **원문 대조**를 붙인다. 예: glibc 2.39 `pthread_create.c`는 `*newthread = (pthread_t) pd;`(L754–L755)를 `create_thread`(L804, L836)보다 먼저 한다. C5 파생 구성의 glibc 2.39(`results_c5_boot_20261029.md` §5)와 같은 판의 upstream 원문이다. 배포판 패치는 [U] |
| L0 | cFE 적재기 의미: 시작 스크립트 필드의 뜻(종류·파일·진입 기호), 기호 범위 | recipe `startup_script` 그대로. 판마다 `cite`. 이미 있음 |
| R | 이 문서의 추출 규칙(A/B/C, R-T1/R-T2, OP-*) | 코드로 동결, 판 번호 |
| O | 실행 기록(관측 값·순서) | 기록 |

**판정 입력에 사례 계층은 없다.** 사례 프로필의 `rspec`·`appreq`·`allowed`·`harm`·역할·`.kzm`·사례 manifest는 평가용으로만 남긴다(RA §2).

**새 사실 F1–F7** [I]. 모두 kz-static의 Clang AST 순회(`ast_fn.cpp`)를 넓히는 것이고, 필드 기반 추상(형, 필드)을 그대로 쓴다.

| # | 사실 | 기법 | 공백 |
|---|---|---|---|
| F1 | READS: 필드·전역 읽기. 용도 표시(분기 조건 / 반환 식 / out 저장 원천 / 호출 인자 / 첨자), 첨자 식, guard | `MemberExpr`·`DeclRefExpr`의 lvalue→rvalue 변환(SF §5.2의 "공유 읽기" 판정 재사용). 용도는 부모 문맥(`IfStmt` 조건, `ReturnStmt`, `*p = …` 우변, `CallExpr` 인자)으로 정한다. guard는 writer의 `guard_from`과 같은 식 | G1 |
| F2 | RETURNS: return 식(상수·필드 읽기·호출 결과·지역 변수)과 guard. 함수마다 "반환 상수 c ⇐ guard" 목록 | `ReturnStmt` 방문. 지역 변수는 정의 하나인 경우 값으로 펼친다(derives의 `DefInfo` 재사용) | G2, G5 |
| F3 | 모든 루프의 탈출식·mod set(지연 유무와 무관) | `ast_fn.cpp` L699의 조건 제거. 행 크기만 는다 | G3 |
| F4 | ARGFLOW: 역할 없는 모든 인자의 copy chain + 함수 요약 "매개변수 k → (callee, 매개변수 j) / 비교 피연산자 / out 저장 / 반환" | derives를 모든 인자로 넓히고, 요약은 호출 그래프 SCC 역위상 순서로 상향 합성한다. 간접 호출은 해결된 대상의 요약을 합친다(may) | G4 |
| F5 | LOCKSET: 호출·쓰기·읽기마다 그 문장에서 잡혀 있는 잠금. 잠금 정체성 = 잠금 연산 인자의 필드(B 기법) 또는 인자 없는 래퍼(단일 잠금) | 함수 본문 `clang::CFG` 위의 전방 must 분석(교차 = 합류). 래퍼 요약 "진입 시 잡고 반환"·"풀고 반환"은 P0의 `pthread_mutex_lock/unlock`에서 유도한다 | G6 |
| F6 | IMAGES: F4가 닿는 전역의 정적 초기값(table walk와 무관). 링크 닫힘 밖 표 원본은 L-TBL(§3.6)이 있을 때만 | `VarDecl::getInit()` 평가(지금 `kz_fact_image` 생성기 재사용) | G8 |
| F7 | SPAWN-W: P0 `pthread_create`에서 출발해, entry 함수 포인터가 매개변수 → 필드 저장 → 새 스레드 쪽 간접 호출로 가는 래퍼를 고정점까지 유도. 시작 스크립트 진입 기호 → `dlsym`(P0) 경유 entry 해결 | SPAWN_PARAMS(DIRECT/VIA_ENTRY)를 반복 적용하고(래퍼의 래퍼), 필드 기반 함수 포인터 흐름 그래프를 그대로 쓴다. 스크립트 진입 기호는 basis `startup.entry[k]`의 둘째 필드 | G7, G9 |

P0 요약(G10)은 데이터 함수 표와 같은 형식으로 넣는다. `pthread_create` 인자 0에 out-pointer 쓰기, 값은 "새 thread id"(top)다. 이것으로 C5 `OS_task_record_t.id`가 escape(DISABLED)에서 writer 있는 대상이 된다 [I].

### 3.1 경로 A: 이름·ID·등록 표

**인식** [I]. 세 꼴을 찾는다. 셋 모두 "표 원소의 키 필드와 인자(또는 인자에서 온 값)를 비교하고, 결과로 색인·id·레코드를 돌려준다"이다.

- A1 검색 루프: 루프가 전역 배열(또는 그 원소 포인터 cursor)을 지나며, 탈출식(F3)이나 루프 안 분기가 `T.f`와 매개변수 유래 값(F4)을 비교한다(`==`, `strcmp`/`strncmp`/`memcmp` 계열 = 0).
- A2 콜백 검색: A1의 비교가 루프 안 간접 호출의 해결된 대상 안에 있다. 매개변수 유래 값이 그 간접 호출의 인자로 넘어간다(F4).
- A3 색인 표: `G[k]`의 필드를 첨자 `k`(매개변수 또는 호출 결과)로 읽고, 그 레코드의 필드로 유효성을 분기한 뒤(F1, F2) 다른 필드를 반환하거나 out에 저장한다. 루프가 없다.

**상태 역할** [I]. 성공 반환(F2의 "성공 상수 ⇐ guard")의 guard를 연언항으로 나눠 다음을 정한다.

- 키 필드: 비교의 한쪽.
- 활성(in-use) 필드: 키 비교와 같은 guard에 있는 다른 필드 조건. 예: `OS_ObjectIdDefined(record->active_id)`, `RecordUsed == TRUE`.
- 값 필드: 성공 경로에서 반환·out 저장되는 필드.
- **"키 필드 쓰기만으로 게시라고 단정하지 않는다"(RV2 §5.2 A)는 이 guard 분해로 구현한다.** 게시 = guard의 모든 연언항을 참으로 만드는 쓰기들의 묶음이다.
- **대상 규칙 R-T2**: 성공 반환 guard와 out 저장 원천이 읽는 필드(키·활성·값 필드)를 writer 집합 추출 대상에 넣는다. 대상 규칙은 R-T1 ∪ R-T2다. R-T1만으로는 분기에서만 읽히는 필드(C5 `RecordUsed`, R2 `AppState`)가 빠진다(§2.3).

**수명 연산 분류** [I]. 키·활성 필드의 writer(V3의 writer 집합 + F1/F2의 값)를 값으로 나눈다.

| 분류 | 판정 |
|---|---|
| 초기화 | INIT 이미지 또는 `early-init`/record memset |
| 예약 | 활성 필드에 "정의됨" 값을 쓰고 키 필드는 비우거나 쓰지 않는 함수. 그 함수가 색인을 out으로 낸다 |
| 활성화 | 활성 guard를 처음 참으로 만드는 쓰기(키와 활성이 다 차는 시점) |
| 해제 | 활성 guard를 거짓으로 만드는 상수(0, UNDEFINED, FALSE) 쓰기 |
| 재사용 | 해제된 칸을 다시 예약할 수 있는가(예약 함수가 활성 필드만 보고 빈 칸을 고르는가) |

**이름 공간 분리** [I]. 표 객체 × 구획 상수 × 레코드 형으로 나눈다. 구획 상수는 API 호출 지점의 상수 인자에서 F4로 전파한다. 예: C1 `OS_ObjectIdFindByName(OS_OBJECT_TYPE_OS_COUNTSEM, …)`의 형 인자 → `OS_GetBaseForObjectType` → `OS_common_table`의 구획.

**미해결로 보고할 것**

- 표 객체가 포인터 하나로만 닿아 어느 전역인지 모를 때.
- 비교 함수가 미해결 간접 호출일 때.
- 키 값이 실행 중에만 정해질 때(C1 CF 키, §3.6).
- 활성 필드가 guard에서 안 보일 때(OSAL 4.1.1 `OS_TaskRegister`는 `free`를 보지 않는다. 이것은 "활성 검사 없음"이라는 사실로 낸다).

**세 사례에 대어 보면 (사례 입력 없이)** [I]

- **C1 조회와 생성**
  - A2가 `OS_ObjectIdFindNextMatch`(루프 `osapi-idmap.c:550`) + 콜백 `OS_ObjectNameMatch`(`strcmp(ref, obj->name_entry)`)를 잡는다.
  - 성공 guard `OS_ObjectIdDefined(record->active_id) && MatchFunc(arg, token, record)`(L561 [F])에서 활성 = `active_id`, 키 = `name_entry`가 나온다.
  - 매개변수 흐름(F4)으로 `OS_CountSemGetIdByName`(인자 1) → `OS_ObjectIdFindByName` → `OS_ObjectIdGetBySearch` → `OS_ObjectIdFindNextMatch`(arg) → 콜백 `ref`가 이어진다. 그러면 "`OS_CountSemGetIdByName`의 인자 1이 COUNTSEM 구획의 `name_entry`와 비교된다"는 관계가 나온다.
  - 생성 쪽 수명 연산은 다음과 같다. 셋 다 V0에 위치가 있다.
    - 예약: `OS_ObjectIdFindNextFree` L639–L640(`active_id` ← 새 id, `name_entry` ← NULL)
    - 키 기록: `OS_CountSemCreate` L100
    - 활성 확정: `OS_ObjectIdTransactionFinish` L1079
  - CF 조회(`cf_cfdp.c:993`)와 BP 생성(`bp_flow.c:132`)은 같은 구획의 조회–생성 짝이 된다. 키 일치는 BP 쪽만 정적으로 풀린다(F6: `BP_Throttles` → `CF_CHANNEL_0_SEM_NAME`). CF 쪽은 "키 미해결, 구획 일치"로 남는다. 실행 기록이 키 문자열을 주면 짝이 확정된다.
- **BP 자신의 존재 확인(E3)**
  - 같은 함수, 같은 반복 안에서 조회(L126)의 실패 분기가 같은 키 인자의 같은 구획 생성(L132)으로 간다.
  - 이것은 A가 아니라 처리 분류(§3.5)의 "조회 실패 → 같은 키 생성"(probe-create) 꼴로 낸다.
  - 결과를 바꾸는 쓰기는 BP 자신의 생성뿐이고, 그것은 프로그램 순서로 조회 뒤에 고정된다. 그래서 행위자 사이 후보가 아니다(§3.4).
  - SCH_LAB의 생성은 키가 문자열 상수 `"SCH_LAB"`라 BP 키와 다르다고 정적으로 정해진다. 그래서 짝에서 빠진다.
- **R2**
  - EVS 쪽에 A3가 있다. `EVS_GetAppDataByID`는 `CFE_ES_AppID_ToIndex`로 색인을 내고 `&CFE_EVS_Global.AppData[idx]`를 돌려준다. `EVS_AppDataIsMatch`는 키 `EVS_AppData_t.AppID == AppID`를 본다. writer는 `CFE_EVS_Register`·`CFE_EVS_CleanUpApp`·EarlyInit이다(V3 writer 4).
  - 이것은 R2의 **둘째** 의존(SB가 EVS에 등록됐는가)이다. 주 의존은 B(§3.2)다.
- **C5**
  - ES `TaskTable`은 A3다. 첨자 = `OS_TaskGetId()`, 활성 = `RecordUsed`, 값 = `AppId`, 실패 반환 `CFE_ES_ERR_APPID`(`cfe_es_api.c` L846–L871 [F]).
  - OSAL `OS_task_table`은 A1(`OS_TaskRegister` 루프, 키 `id == pthread_self()`, 활성 검사 없음)이다.
  - OSAL 표의 해제는 `OS_TaskDelete`·`OS_TaskExit`에만 있다. **진입 함수가 그냥 반환하는 경로에는 해제가 없다**는 것이 수명 사실로 나온다. OSAL 4.1.1은 `function_pointer`를 `pthread_create`에 그대로 넘긴다(L645–L648 [F]).
  - 이 사실이 C5의 `pthread_t` 재사용 경로(E2)의 정적 근거가 된다. 이것은 #72가 아니고, 순서위반인지도 [U]다. 정체성·수명 자료로 따로 보고한다(RA E2).

### 3.2 경로 B: 공유 필드 → 호출 인자

**인식** [I]. 호출 인자 a에 대해 F4로 원천을 거슬러 올라간다.

- 원천이 공유 위치(전역, 전역 레코드의 필드, 전역 배열 원소)면 (필드 F, 호출 c, 인자 a)를 낸다.
- 매개변수에서 왔으면 상위 호출 지점들로 넘어간다. 깊이 제한이 있고, 한계에 걸리면 미해결로 남긴다.
- 지금 V0에도 복사 없는(hops 0) 꼴은 인자 종류 `symbolic T.f`로 이미 있다. C1 1,901 / R2 1,114 / C5 950 인자, 서로 다른 필드 611 / 413 / 258이다 [M].

**필드 쪽** [I].

- F의 초기 이미지(INIT), 쓰기 집합(V3 writer 집합: 직접·데이터 함수·out-pointer, 값·guard), 쓰기 함수의 정적 행위자(§4)를 붙인다.
- "잘못된 초기 값과 나중의 유효 값이 달라도 같은 필드의 의존성을 유지한다"(RV2 §5.2 B)는 F 단위로 묶으면 그대로 성립한다. 값으로 묶지 않는다.

**callee 쪽** [I].

- 인자 a가 callee 안에서 어디로 가는지 F4 요약으로 본다.
- 그 값을 분기 조건으로 쓰는 검사(F1/F2: 검사 결과가 오류 상수 반환 또는 다른 함수 호출로 이어짐)를 "callee 자신의 유효성 검사"로 붙인다.
- 검사를 정적으로 평가할 수 있으면(상수 값, 본문 있는 순수 함수) 값마다 통과 여부를 적고, 아니면 "평가 불가"로 둔다.

**호출자 처리**: 결과 사용 플래그와 F2/F4로 "버림 / 저장 / 상위 반환 / 검사"를 낸다(RV2 §5.6 넷째 줄).

**R2 처리 (사례 입력 없이)** [I][M 일부]

- 관계: `CFE_SB_Global_t.AppId → CFE_EVS_SendEventWithAppID` 인자 2, 39곳, hops 0 [M].
- 필드: INIT 0(`cfe_sb_task.c:40`), 쓰기 `CFE_SB_EarlyInit` memset 0, `CFE_SB_AppInit`이 `CFE_ES_GetAppID`로 쓰는 out-pointer 둘 [M, V3].
- 정적 행위자: 읽는 쪽 SB API 함수는 코어 task-main 다섯이 닿고, 쓰는 쪽 `CFE_SB_AppInit`은 task-main 가운데 SB만 닿는다 → 행위자 사이 관계다 [M].
- callee 검사: `EVS_GetAppDataByID`가 `CFE_ES_AppID_ToIndex(AppID) != SUCCESS`이면 NULL이고, 그러면 `Status = CFE_EVS_APP_ILLEGAL_APP_ID`다. F2가 이 반환을 사실로 낸다.
  - 값 0을 넣은 검사 평가는 `CFE_ResourceId_ToIndex`의 기준값 비교를 상수로 접어야 한다. 그것이 되면 "초기 값은 검사를 통과하지 못함"이다.
  - 쓰기 값 `CFE_ES_AppRecordGetID(AppRecPtr)`(guard `AppRecPtr != NULL`)는 "통과 가능"이다.
  - 상수 접기가 안 되면 "평가 불가"로 두고 실행 기록의 반환값을 쓴다.
- 처리: 39/39 버림 [M].
- 순서
  - 생성 순서는 table walk의 task-main 순서에서 온다(V1) [M].
  - 각 task의 초기화 완료를 기다리는 `CFE_ES_MainTaskSyncDelay`의 await 인정에는 술어 필드(`CFE_ES_AppRecord_t.AppState` 등)가 대상이어야 한다. R-T2로 들어온다.
  - 이 둘이 서면 "EVS·ES 초기화가 SB 초기화보다 앞에 고정됨"이 정적 근거로 나온다. 그러면 R2는 스케줄 race가 아니라 **고정된 잘못된 순서**의 후보로 분류된다(RA E1, RV2 §3.4).
  - await는 L1의 전제만 확인하고 L1 자체는 증명하지 않는다(SF §8). 그래서 보고 문구는 "정적 근거상 고정"이다. 실행 기록에서 매번 u가 w보다 앞이면 "관측된 고정 순서"가 붙는다.
- 앱 관계(RV2 §5.5): 실행 task(EVS 또는 ES), 코드가 속한 모듈(sb), 전달된 AppId의 원천 필드(SB 전역)를 따로 적는다. 모듈은 basis 단위표(`member module`/`member of libsb.a`)에서 온다 [M].
- C3(#73)의 AppId 흐름도 이 경로다(RV2 §5.2 B). 이번 빌드에는 없다.

### 3.3 경로 C: 생성·등록·소유

**인식** [I].

- spawn 연산은 F7로 자동 유도한다(G0 제거). spawn 호출 s(함수 P 안)마다 다음을 낸다.
  1. 생성자가 받는 정체성: spawn 연산의 out 인자(F4: `OS_TaskCreate` 인자 0 → `*task_id = possible_taskid`).
  2. 자식 entry와, 그 entry에서 닿는 코드(정적 행위자, §4).
  3. P 안에서 s **뒤에** 오는 쓰기(F1 없이도 FUNC_WRITES와 CFG 도달로 가능). 그 쓰기의 첨자가 1의 정체성에서 오는지(F4).
  4. 자식 코드가 같은 필드를 읽는 지점(F1). 그 읽기의 첨자가 자식이 스스로 알리는 정체성에서 오는지. 예: `OS_TaskGetId → pthread_getspecific`, P0.
  5. s와 3의 쓰기 사이의 잠금 집합(F5)과, 4의 읽기의 잠금 집합.
- 출력: "생성자의 생성 후 등록 쓰기 w(필드 f, 첨자 = 생성자 정체성)"와 "자식의 읽기 u(필드 f, 첨자 = 자식이 알린 정체성)"의 짝이다.
  - u의 결과 분기(F2)가 f에 달려 있으면 §3.4의 신호 후보다.
  - **두 정체성이 같은지는 정적으로 정하지 않는다.** 실행에서 생성 측 토큰과 자식 진입을 독립 경로로 잇는 단계 1의 몫이다(RA §2, RV2 §3.7). 정적 출력은 "두 정체성의 원천"만 적는다.

**실현 가능성(반대 순서)** [I]. F5로 정한다.

- s 직전부터 w까지 같은 잠금 L이 계속 잡혀 있고 u가 L 구간 안이면, u는 s 뒤 w 앞에 올 수 없다.
  - 근거: 자식은 s 뒤에 시작하고(P0 spawn 의미), L은 상호 배제다.
  - 이 경우 "반대 순서 불가(잠금 연속)"다.
- 잠금이 s 앞에서 풀리고 w 앞에서 다시 잡히면 "반대 순서 가능(잠금 창)"이다.
- 이것은 한 실행의 잠금 획득 순서를 모든 실행의 순서로 고정하지 않는다(RV2 §3.3). 판단은 코드 구조로만 한다.
- 같은 정보로 **보류 계획 거부**도 한다. "L을 잡은 채 자식 진행을 기다리는" 계획은 만들지 않는다(RV2 §3.8).

**C5 처리 (사례 입력 없이)** [I][M 일부]

- spawn: `CFE_ES_AppCreate` L614 [M].
  - entry는 F7이 시작 스크립트의 둘째 필드(`SAMPLE_AppMain` 등, basis `startup.entry[k]` [M])와 P0 `dlsym`으로 푼다.
  - 정적 행위자 귀속은 이미 스크립트 진입으로 된다(§4: SAMPLE_AppMain, CI_Lab_AppMain …).
- w: L649–L652(`TaskTable[TaskId]`의 `RecordUsed`·`AppId`·`TaskId`) [M]. 첨자 `TaskId = AppTable[i].TaskInfo.MainTaskId`는 `OS_TaskCreate` 인자 0의 out이다 [F].
- u: 자식이 `CFE_ES_RegisterApp`/`CFE_EVS_Register` … → `CFE_ES_GetAppID`에서 `TaskTable[OS_TaskGetId()].RecordUsed`를 검사한다(F1). 실패 반환 `CFE_ES_ERR_APPID`(F2).
- 잠금: s(L614)의 잠금 집합 = ∅(L610에서 풀림). w의 잠금 집합 = {ES 공유 잠금}(L637). u의 잠금 집합 = {ES 공유 잠금}(L851). 이 값은 V1의 잠금·해제 호출 위치에서 사람이 읽은 것이고, F5가 계산해야 할 값이다 [M: 호출 위치]. → **6.4.1은 반대 순서 가능(잠금 창 L610–L637)**.
- 6.4.2는 L571–L669 잠금이 `OS_TaskCreate`와 등록을 감싼다(`cases/C5.md` §0.1 [F]). → **반대 순서 불가**. 또 그 구간에서 자식 진행을 기다리는 보류 계획은 거부된다.
- `pthread_t` 재사용 경로는 C와 별개다. A의 수명 사실(§3.1 C5)과 단계 1의 정체성 자료로 보고하고, #72 수에 넣지 않는다.
- 코어 task(C5 `CFE_ES_CreateObjects`의 union entry)는 F6이 `CFE_ES_ObjectTable` 이미지를 읽어야 풀린다. 못 풀면 그 spawn은 "entry 미해결"로 남는다.

**C1 처리**: CF·BP는 둘 다 시작 스크립트가 만든 앱이다. 생성자(ES 시작 경로)의 생성 후 쓰기는 CF·BP 사이 관계에 들어가지 않는다. C1의 주 관계는 A다. C는 앱 귀속(행위자 roots)에만 쓴다.

### 3.4 반사실 결과 뒤집기 신호 (RA §3.1)

**정의** [I]. 결과가 분기되는 사용 u(호출 결과 검사, 또는 F1의 분기 조건 읽기)에 대해 다음을 정한다.

- S(u): u의 결과 분기를 정하는 상태 위치들. F2/F1/F4 요약을 합성한 "성공 ⇔ φ(상태, 인자)"에서 나온다. 예: C1 조회는 φ = ∃i. defined(active_id[i]) ∧ name_entry[i] = key.
- W(u): S(u)를 쓰는 쓰기 가운데 **u와 다른 정적 행위자에게 닿는 것**(§4). 같은 행위자의 쓰기는 프로그램 순서로 u 앞 또는 뒤에 고정되면 뺀다(BP probe).
- w ∈ W(u)마다 w의 값(writer 값·guard)으로 φ를 다시 평가한다. 결과가 바뀌면(실패 → 성공, 또는 읽는 값이 바뀜) (w, u)를 **"결과를 바꾸는 순서 의존" 후보**로 낸다.
- 평가 근거 등급은 다음 셋이다.
  - 정적-가능: 값이 상수이거나 guard가 참으로 접힌다.
  - 정적-미정: 값이 top이다.
  - 동적-관측: 기록에 u의 결과와 w의 발생 순서가 있다.

**판정이 아니다**. 이 신호는 "코드가 성공 경로를 기대한다"는 것만 말한다. 위반 확정은 RV2 §5.4의 별도 근거(관측된 실패 + 반대 순서 가능성, 또는 실현된 강제 순서)를 요구한다. 보고는 RV2 §5.5의 독립 항목으로 한다.

| 사례 | u | W(u) | 뒤집힘 | 처리(§3.5) | 순서 항목 |
|---|---|---|---|---|---|
| C1 수정 전 | CF 조회 `cf_cfdp.c:993` | BP 행위자의 `name_entry`(L100)·`active_id`(L1079) 쓰기, 키 = BP0 이름 | 실패(-17) → 성공 [정적: 키가 CF 쪽 미해결이면 "정적-미정", 기록의 키 문자열로 확정] | 종료 경로(반환값 → `run_status` → `CFE_ES_RunLoop` → `CFE_ES_ExitApp`, F2/F4 필요) | 정적 순서 근거는 생성 순서뿐이다(생성 순서는 초기화 완료 순서를 정하지 않는다, RP §4). 반대 순서 가능성은 실행 제약 분석(F5, 대기 의미)으로 따로 판단 [U] |
| C1 수정 후 | 같은 조회, 루프 `:1001` 안 | 같음 | 같음 | 재시도: 결과가 -17이 아닐 때까지, 최대 25회, 지연 있음 [M: V1 루프 사실]. 상한 이후는 같은 종료 경로 | 개별 실패를 버그 수로 세지 않는다(RV2 §9.2) |
| BP 존재 확인 | BP 조회 `bp_flow.c:126` | BP 자신의 생성은 같은 행위자이고 뒤에 고정 → 빠짐. 다른 생성자: SCH_LAB(키 상수 다름 → 빠짐) | 후보 없음. 키를 모르는 다른 생성자가 있으면 "정적-미정" 후보로 남는다 | probe-create | — |
| R2 | EVS 검사(SB API 39곳에서) | `CFE_SB_AppInit`의 out-pointer 쓰기(SB 행위자) | 초기 값 0: 검사 실패(상수 접기가 되면 정적-가능) → 쓰기 값: 통과 가능 | 결과 버림 39/39 | 생성 직렬화 + await → "정적 근거상 고정"(§3.2). 기록이 맞으면 "관측된 고정 순서 위반" |
| C5 #72 | 자식의 `RecordUsed` 검사(`CFE_ES_GetAppID`) | 생성자 L649 `RecordUsed = TRUE`(생성자 행위자) | 실패(`CFE_ES_ERR_APPID`) → 성공. 단 첨자 일치(정체성)가 전제 → 정체성은 동적으로만 | 호출자마다 다름(F2/F4) | 6.4.1 반대 순서 가능(잠금 창), 6.4.2 불가(§3.3) |

지금 `kz predict`의 "결과 뒤집기"와 같은 질문이다. 다른 점은 의존을 손으로 쓴 `.kzm`이 아니라 F1–F7 사실에서 만든다는 것이다. `kz predict` 엔진이 이 입력을 받을 수 있는지는 단계 0 감사 뒤에 정한다(RV2 §10).

### 3.5 후속 처리 분류 (RV2 §5.6)

| RV2 보강 | 쓰는 사실 | 새 분석 | 이번 측정에서 나온 것 |
|---|---|---|---|
| 같은 자원·호출 문맥·실패 분기·루프 발생을 확인한 재시도 | 루프 사실(F3 포함), mod set의 `call-result`, 탈출식의 비교 상수 | 루프 안 같은 호출 지점이 같은 인자 원천(F4)으로 되풀이되는지. 탈출식의 상수가 그 호출 결과의 실패 코드(F2 요약의 실패 상수)인지 | C1 수정 후 `:1001`: count-bounded, upper-bound 25, `ret != -17`에서 탈출, 지연 있음 [M, V1] |
| 조건·갱신·탈출·정수 연산으로 입증한 상한 | 지금 constant-trip/count-bounded 판정(SF §5.1) | 같음. 입증하지 못하면 "정적 상한 미확인" | 25 = `CF_STARTUP_SEM_MAX_RETRIES`(상수 평가) |
| 도달 가능 경로와 실제 종료의 구분 | terminal 사실(G0 또는 P0 유도) | 실패 분기에서 terminal까지의 반환값·필드 흐름(F2/F4). 실제 종료는 기록에서만 | C1 수정 전: `CFE_ES_ExitApp` 호출 지점만 [M]. 흐름은 F2/F4 뒤 |
| 저장·상위 반환·간접 검사·out 인자까지 추적한 "무시" | 결과 사용 플래그 | F2/F4로 저장된 변수의 이후 사용 | R2 39/39는 표현식 문장이라 저장도 없다 [M] |
| 단순 후속 연산과 기능적 대체의 구별 | — | 실패 분기에서 **같은 키 인자 원천으로 같은 이름 공간의 생성 연산**을 부르면 "probe-create". 그 밖의 다른 연산은 "후속 연산"으로만 적는다. 대체라고 이름 붙이지 않는다 | BP `:126` → `:132` [M: 위치·인자] |
| 같은 task·키 반복 성공이 별개 요청·새 세대가 아닌지 확인 | — | 기록에서 같은 호출 지점의 같은 동적 반복(루프 반복 번호)인지 확인. 관측 계획(§4)에 루프 반복 표시가 필요하다 | — |

### 3.6 일반화할 수 없거나 아직 못 하는 곳 (명시) [D]

1. **C1 CF 키(표 파일)**
   - 키가 `CFE_TBL_Load(…, CFE_TBL_SRC_FILE, CF_CONFIG_TABLE_FILENAME)`로 읽는 표 원본에서 온다(`cf_app.c` L155–L184 [F]).
   - 정적으로 풀려면 두 가지가 필요하다. (i) cFE 표 서비스 의미(L-TBL: 등록·적재·`GetAddress`의 포인터 연결). (ii) 빌드의 표 대응(표 원본 TU → 설치된 `.tbl` 파일 이름; R2 빌드의 `tblobj_*` link 명령이 그 예다 [M]).
   - (i)은 판마다 쓰는 도구 내부 지원이고 (ii)는 빌드 입력이다. 둘 다 없으면 **"키 미해결, 구획 일치"**로 남긴다. 사례마다 키를 적어 넣지 않는다.
2. **값 평가**: R2의 "0은 검사 실패"는 본문 있는 함수의 상수 접기가 필요하다. 실패하면 "평가 불가"로 두고 관측 반환값을 쓴다.
3. **정체성 일치(C5)**: 생성자 정체성과 자식이 알린 정체성이 같은지는 정적으로 정할 수 없다. 단계 1의 독립 관측 경로 몫이다.
4. **C2(#198)**: 앱 내부 late-init의 상태 의미는 이번 빌드에 없다. A/B/C가 원 앱 없이 그 의미를 찾는다고 주장하지 않는다(RV2 §7).
5. **포인터·별칭**: 필드 기반 may 사실이다. 같은 형의 다른 인스턴스를 가르지 못한다. 그래서 행위자 귀속과 짝은 "가능한 의존"(RV2 §5.2 끝)이고, 동적 근거와 같은 등급으로 세지 않는다.
6. **P0·L0는 사람이 쓴다**: 사례 무관이지만 수작업이다. 판 표기, 원문 대조, 공개로 관리한다. P0 항목이 바뀌면 판 번호를 올리고 전체를 다시 돌린다.

## 4. 관측점 자동 생성 (원형)과 수작업 manifest 비교

### 4.1 규칙 [I]

V1/V0p 사실과 basis만 쓴다(`EB/count/obs_points_rule.tsv`).

**정적 행위자(roots)**

- `main`, 해결된 spawn entry, table walk 반복의 task-main, 시작 스크립트 진입 기호.
- 호출 그래프는 본문 있는 직접 호출 + 해결된 간접 대상이다. spawn은 호출 간선이 아니다.
- 모듈은 basis 단위표의 소속(`module X`, `member of libX.a`/`X.o`, link 명령에 직접 = exe)이다.
- 원형은 래퍼 root(`OS_PthreadTaskEntry`, `CFE_ES_TaskEntryPoint`처럼 간접 호출로 다른 root에 닿는 진입)도 root로 센다. 구현에서는 이런 트램펄린을 행위자에서 빼고, 그것이 부르는 task-main에 귀속시킨다.

| 규칙 | 지점 |
|---|---|
| OP-E | 행위자 root의 진입·반환 |
| OP-S | spawn 호출 전후 |
| OP-L | 잠금·해제 호출 |
| OP-T | 종료 연산 호출 |
| OP-A | 검색 함수(루프 안 `strcmp` 계열에 필드 피연산자, 또는 그런 비교를 가진 콜백을 부르는 루프)와 그 키 필드의 쓰기 함수의 진입·반환. 같은 모듈에서 그들에 2단계 안에 닿는 함수를 다른 모듈이 부르는 지점. OP-A4는 3–4단계 |
| OP-B | `symbolic T.f` 인자 호출 가운데 그 필드를 쓰는 함수와 **다른 root**가 닿는 것(호출 값) + 그 필드의 쓰기 함수 |
| OP-C | spawn 호출 뒤 생성자 함수가 처음 쓰는 필드 |

### 4.2 수 [M]

| 빌드 | roots | OP-E | OP-S | OP-L | OP-T | OP-A(+A4) | OP-B | OP-C | 관련 함수 |
|---|---|---|---|---|---|---|---|---|---|
| c1_pre_tx | 15(process 1, spawn-entry 9, 스크립트 앱 5) | 15 | 7 | 96 | 51 | 132(+24) | 1131 | 9 | 506 |
| r2_core | 14(process 1, spawn-entry 8, task-main 5) | 14 | 6 | 99 | 46 | 98(+17) | 1007 | 9 | 457 |
| r2_all | 35 | 35 | 15 | 99 | 61 | 2051(+156) | 1638 | 27 | 1139 |
| c5_641 | 9(process 1, spawn-entry 3, 스크립트 앱 5) | 9 | 7 | 79 | 35 | 228 | 362 | 24 | 202 |

수작업 manifest(`kairos/profiles/*/instr/*.kzs`)의 사이트 수: C1 수정 전 37, 수정 후 38, R2 44, C5 33.

### 4.3 수작업 manifest와의 비교 [M]

`EB/count/obs_manifest_coverage.tsv`. 함수 수준 비교다. 사이트의 함수가 어떤 규칙의 대상이거나 그 함수를 부르는 지점이 규칙 지점이면 "생성됨"으로 셌다.

| manifest | 생성됨 / 전체 | OP-A4 없이 | 생성되지 않은 사이트 |
|---|---|---|---|
| C1 `sites-cf281a941.kzs` | 29 / 37 | 22 | `OS_ObjectIdFindByName`(found), `OS_CountSemAPI_Init`, `OS_CountSemDelete` ×3, `OS_Application_Run`, `CF_Init` ×2 |
| C1 `sites-cf833fdbb.kzs` | 30 / 38 | 22 | 같은 8 |
| R2 `sites.kzs` | 34 / 44 | 34 | `CFE_EVS_Register` ×4, `CFE_EVS_CleanUpApp` ×3, `CFE_ES_RestartCmd`, `CFE_SB_SetSubscriptionReporting`, `OS_Application_Run` |
| C5 `sites-641.draft.kzs` | 26 / 33 | 26 | `OS_TaskRegister` ×3, `CFE_ES_GetAppIDInternal` ×3, `EVS_GetAppID` |

해석 [I]:

- **함수 수준의 "생성됨"은 넉넉한 기준이다.**
  - C5 `CFE_ES_GetAppID`는 그 안의 잠금 호출 때문에 OP-L로만 덮인다. 수작업이 고른 `store_ok`(RecordUsed 분기와 AppId 저장)는 F1이 있어야 규칙으로 나온다.
  - C1의 CF 조회 호출과 `OS_CountSemGetIdByName`은 깊이 4(OP-A4)에서만 나온다. 매개변수 → 비교 흐름(F4)이 깊이 매개변수를 대신해야 한다.
  - R2의 `evs.send`는 OP-B(39곳)로, 올바른 이유로 덮인다.
- **빠진 것은 모두 §2.5 공백과 맞는다.**
  - OSAL 4.1.1 `OS_TaskRegister`: `==` 비교(G1/G3).
  - `CFE_ES_GetAppIDInternal`, `EVS_GetAppID`: 색인 표 읽기(G1).
  - `CFE_EVS_Register`·`CleanUpApp`: A3 쓰기 함수. 키 필드가 비교 검출에서 안 나와 OP-A에 들지 않음.
  - `OS_CountSemDelete`·`OS_CountSemAPI_Init`: 해제·초기화(A의 수명 분류).
  - `CF_Init`: 반환값 사슬(F2).
  - `OS_Application_Run`: 종료 때 전체 해제(teardown). A의 수명 분류 가운데 해제다.
- **규칙은 넘치게도 낸다.**
  - OP-B는 C1 1,131개다. 수작업 37개의 30배다.
  - r2_all의 OP-A는 `CFE_EVS_SendEvent` 호출 1,277개를 포함한다. 원형의 "2단계 안에 닿음"이 너무 넓다.
  - 실제 구현에서는 OP-A를 "키 인자가 비교에 닿는 API"(F4)로, OP-B를 "필드 → 인자 → callee 검사(F2)가 있는 것"으로 좁힌다.
  - 그래도 수가 크면 계측 비용과 손실 계정(SPEC §5.6)이 먼저다.
  - 수작업 manifest의 F0 hold 4주소 한계는 확인 단계에만 해당한다(RP §3.7).
- **수작업 manifest의 선택 근거**(어느 분기·값을 왜 봤는지)는 사례 지식이다. 규칙 지점이 그것을 다 덮더라도 "사례 선택이 필요 없었다"는 주장은 단계 3의 완료 기준(새 호출자를 수작업 site 없이 관측)으로 따로 확인한다(RV2 §11).

## 5. 열린 위험

| # | 위험 | 대응 |
|---|---|---|
| R1 | F4(매개변수 요약)의 폭발: C1 2180 함수, 간접 호출 미해결 18 | 필드 기반 요약, SCC 단위 고정점, 깊이·크기 상한. 상한에 걸리면 "미해결"로 남기고 센다 |
| R2 | 정적 행위자 귀속의 과대 근사: C1에서 본문 함수 1984 가운데 737이 둘 이상의 root에서 닿는다 | 짝을 "가능한 의존"으로만 내고, 실행 기록의 실제 task와 대조한다. 정적 귀속을 판정 근거로 쓰지 않는다 |
| R3 | 완전성 판정의 비단조(§2.3): 대상 집합이 바뀌면 complete ↔ DISABLED | 형 노출을 필드 단위로 보거나 "형 단위 보수" 규칙을 명시한다. 어느 쪽이든 대상 선택과 독립이어야 한다. 사례 재생이 V2의 complete에 기댄 곳(R2 `CFE_SB_Global_t.AppId`의 별칭 포괄, SF §3.1 재생 절)은 다시 확인할 항목이다 [U] |
| R4 | P0·L0가 "이름만 바꾼 수작업 표"가 될 위험(RV2 §4.2) | 항목마다 사례 무관성, 판 표기, 원문 대조 줄을 단다. 사례를 본 뒤의 편집은 판 번호를 올린다. 모든 후보에 입력 계층(§3.0)을 붙여 P0·L0가 없으면 후보가 안 나오는 경우를 숨기지 않는다 |
| R5 | 표 파일 키(C1 CF)처럼 실행 중에만 정해지는 키 | 정적 "미해결 + 구획 일치" → 관측 값으로 확정. 정적 단독 결과로 세지 않는다 |
| R6 | 기존 기록의 충분성(RV2 §3.9) | 새 관측점(OP-A 읽기·분기, F5 잠금)이 옛 기록에 없을 수 있다. 재생 전에 기록마다 충분성 점검을 하고, 부족하면 그 한계를 출력한다 |
| R7 | 다섯 사례가 개발 집합이라는 한계(RV2 §8.3) | 같은 판의 다른 호출자(예: C5의 OSAL `*GetIdByName`·`*Create` 6쌍, C1·R2의 `OS_ObjectIdFindNextMatch` 콜백 3종)를 정상 대조로 쓰고, 동결 뒤 고른 구성으로 따로 평가한다 |
| R8 | 추출 산출물이 판정 프로필 해시에 묶임(A3) | 단계 2a에서 facts의 프로필 결합을 "추출 규칙 판 + P0/L0 판"으로 바꿀지 PI 결정이 필요하다. 엔진 변경이다 |

## 6. 다음 구현 단계와 완료 시험

**단계 2a (구현)** [D-제안]. 엔진 변경이므로 이번에는 하지 않았다.

1. kz-static에 F1(READS), F2(RETURNS), F3(모든 루프 탈출식), F4(역할 없는 derives + 매개변수 요약), F5(LOCKSET), F7(SPAWN-W, 스크립트 진입 해결)과 P0 요약 표를 더한다. F6은 F4가 닿는 전역의 이미지로 시작한다.
   - 새 섹션은 `kz_facts.h` schema 판을 올린다. 기존 섹션의 바이트는 바뀌지 않아야 한다. 기존 golden은 그대로 통과해야 하고, 새 섹션 golden은 새로 만든다.
2. `kz-static extract --no-profile`(또는 빈 프로필의 내장화)과 `--targets-rule R-T1,R-T2`. R-T2 = 성공 반환 guard·out 저장 원천이 읽는 필드.
3. 사례 무관 질의기 `kz-static relations`. A/B/C 관계와 §3.4 신호를 표로 내고, 각 행에 입력 계층을 붙인다.

**완료 시험** (모두 사례 입력 없이: 두 문장 프로필, P0, L0, 빌드만):

| # | 시험 | 통과 조건 |
|---|---|---|
| T1 R2 | `relations`가 B 관계 `CFE_SB_Global_t.AppId → CFE_EVS_SendEventWithAppID#2`를 낸다 | 39 지점; 쓰기 3(EarlyInit 0, AppInit out-pointer 둘); callee 검사 `EVS_GetAppDataByID == NULL → CFE_EVS_APP_ILLEGAL_APP_ID`(F2); 결과 버림 39/39; 읽는 쪽과 쓰는 쪽 행위자 다름 |
| T2 C1 | A 관계 "COUNTSEM 구획 `name_entry`/`active_id` 조회–생성"을 낸다 | CF `cf_cfdp.c:993`(수정 후 :1003)과 BP `bp_flow.c:132`가 같은 구획; 게시 쓰기 L639/L640, L100, L1079; BP `:126 → :132` probe-create; 수정 전 실패 분기 → `CFE_ES_ExitApp` 도달(F2/F4); 수정 후 재시도(25, `!= -17`, 지연); SCH_LAB 생성은 키 상수 불일치로 짝 아님 |
| T3 C5 | C 관계를 낸다 | spawn `CFE_ES_AppCreate:614`의 잠금 집합 ∅; 등록 쓰기 L649–L652의 잠금 집합 {ES 잠금}; 자식 `CFE_ES_GetAppID`의 `RecordUsed` 검사(F1)와 `CFE_ES_ERR_APPID` 반환(F2); "반대 순서 가능(잠금 창)". 같은 시험을 6.4.2 기준 빌드에 돌려 "반대 순서 불가"가 나와야 한다 |
| T4 C5 수명 | A 수명 사실 | `OS_task_table.id`의 writer에 `pthread_create`(P0) 포함; 해제는 Delete/Exit만; `OS_TaskRegister` 검색에 활성 검사 없음. #72 관계와 다른 행으로 나온다 |
| T5 입력 계층 | 위 행마다 입력 계층이 B/P0/L0/R만이다 | 사례 계층 0. P0·L0 항목마다 판과 원문 줄 |
| T6 정상 대조 | 같은 판의 다른 호출자에 같은 규칙 | C5 OSAL `*GetIdByName`·`*Create` 6쌍(task, queue, binsem, countsem, mutsem, timer)과 C1·R2의 `OS_ObjectIdFindNextMatch` 콜백 3종이 A로 인식된다. 정상 호출자는 신호만 내고 위반 확정은 내지 않는다 |
| T7 결정성 | 두 번 실행 | facts·relations 바이트 동일 |

**반증 조건** (RV2 §11 단계 2, §12): T1–T3 가운데 하나라도 **사례 입력을 더하지 않으면 관계가 나오지 않으면**, 새 핵심 가설("사례별 Φ 없이 근거 있는 관계를 낸다")은 아직 성립하지 않은 것으로 보고한다. 그 경우 무엇을 더해야 했는지를 입력 계층과 함께 적는다. T5에서 사례 계층이 나오면 그 행은 자동 결과로 세지 않는다.

## 7. 재현

```sh
R=/home/user/Shape-Perf/research/cfs_procrace
$R/env/redesign/extract_baseline.sh all            # inputs, run(28 추출), compare, count -> /home/user/work/redesign/extract_baseline
```

- 각 실행 디렉터리 `EB/<build>/<variant>/`에는 `basis.kz`, `basis.txt`, `basis.dump`, `facts.kz`, `facts.txt`(`kz-static show`), `extract.txt`, `args.txt`, `sha256.txt`가 있다.
- 계수 결과는 `EB/count/`의 `summary.txt`와 TSV 15종, 평가용 조회 `eval_C1.txt`·`eval_R2.txt`·`eval_C5.txt`다.
- 시간(이 VM, 한 번, 비용 주장 아님): extract C1 약 4 s, R2 core 약 4 s, R2 all 약 8–12 s, C5 약 1.4 s. 계수 약 17 s.

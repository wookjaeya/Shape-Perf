# C1 빌드 manifest — W3: P0(P-REC, 공개 통합 구성)의 수정 전후 쌍 (2026-10-24)

- 성격: **빌드 기록과 build-smoke 기록**. W4 실험이 아니고 본실험 결과도 아니다. 기동은 빌드마다 한 번만 했고(§7), 관측기(Kairos)는 붙이지 않았다. 빈도·확률 자료로 쓰지 않는다.
- 입력: [C1_basis_plan.md](C1_basis_plan.md) §8(8.0 순서, 8.3 절차, 8.4 이탈 대장, 8.5 P-REC), [MASTERPLAN_ADOPTION_20261024.md](MASTERPLAN_ADOPTION_20261024.md) W3, 마스터플랜 §5.3·W3·§9.3, 결정 D3·D5(`Architecture/spec/PI_DECISIONS_RESOLVED_20261022.md`), D22(`PI_DECISIONS_RESOLVED_20261015.md` §3.2).
- 규칙: [CONDITIONS_POLICY.md](../CONDITIONS_POLICY.md). 값마다 레퍼런스와 상태(`from-reference`·`deviation`·`unspecified_by_reference`)를 붙인다. 레퍼런스가 정하지 않는 값은 고르지 않았다: 후보가 둘이면 둘 다 빌드했다(cFS 번들).
- 표기: [F] 고정 커밋의 소스·문서에서 읽음, [M] 이번 작업에서 도구로 얻음, [I] 추론, [D] 결정, [U] 미확인. 줄 번호는 해당 커밋의 파일 기준이다. "CI L*n*"은 nasa/bp `b051eb0`의 `.github/workflows/build-run-bp-common.yml`의 줄이다.
- 작업 공간(저장소 밖): `/home/user/work/procrace/c1_build/`. 소스 사본은 `/home/user/work/procrace/refs/`의 blobless clone에서 만든 `git worktree`이다. `cfs_ref`(ENV)는 이번 작업에서 읽지도 쓰지도 않았다.
- 기대 결과표([expected_results_C1_R2.md](expected_results_C1_R2.md))는 이 smoke보다 먼저 커밋됐다(`cf0e383`).

## 0. 요약

1. **D22 순서의 첫 단계 P0을 P-REC로 수행했다**(계획 §8.0·§8.5, Q-5의 (a) = 계획이 문자 그대로의 적용으로 읽은 쪽). 공개 통합 구성(BP CI가 가리킨 스택)을 BP의 `.github/buildconfig` 그대로 조립했다. cFS 번들은 CI 창의 후보 둘(`1df262a`, `1b0b338`)을 모두 빌드했다. 결과: 2 후보 × 수정 전후 = **4개 빌드 모두 조립·설치·기동(OPERATIONAL)** [M].
2. **원문 그대로의 첫 빌드는 실패했다.** 실패 지점은 OSAL `os-impl-filesys.c:171`의 `snprintf`에 대한 gcc 13.3의 `-Werror=restrict` 하나뿐이다(§4.1). 계획 §8.5의 분류로는 (b) 이 호스트의 도구 차이로 본다 [I]. 최소 이식은 `CFLAGS=-Wno-error=restrict` 하나이며 진단 수준만 바꾼다. 자원 이름·생성/조회 순서·재시도 설정·우선순위는 바뀌지 않았다(§6에서 산출물로 대조 [M]). 그래서 멈추지 않고 진행했다. 이 이탈은 명시한 빌드 이탈로 수용됐다(P5, 2026-10-25, `Architecture/spec/PI_DECISIONS_RESOLVED_20261025.md` §1) [D]. 표현: "실험 대상 소스와 설정을 수정하지 않고, 기록한 경고 강등을 적용해 공개 구성을 빌드했다". 그 경고가 오탐이라는 것은 보이지 않았고(§4.1 [U]), 과거 컴파일러·실행 환경과 같다고 주장하지 않는다.
3. **유효 설정**(산출물에서 읽음 [M]): 이름 `BP0_tsem`·`BP1_tsem`(CF 표 `.tbl` 바이트, BP `.so`의 기본값, BP 이벤트 문구 모두 일치). CF 재시도: 수정 전 없음, 수정 후 `CF_STARTUP_SEM_MAX_RETRIES`=25·`CF_STARTUP_SEM_TASK_DELAY`=100(빌드 명령으로 매크로 평가). 시작 순서는 스크립트 L1–L7 그대로(CF 다음 BP), 설정 우선순위 CF 65·BP 55.
4. **build-smoke 관측**(빌드마다 1회, tx·rx 두 인스턴스): 수정 전 4개 인스턴스 모두 CF 이벤트 30 "failed to get sem id for name BP0_tsem, error=-17"(`OS_ERR_NAME_NOT_FOUND`) 후 CF 종료, 그 뒤 BP가 두 semaphore를 "Created". 수정 후 4개 인스턴스 모두 BP "Created" 두 줄과 CF 이벤트 20 "CF Initialized". 스케줄러는 모든 스레드 `SCHED_OTHER`(nice 0): OSAL이 `SCHED_RR`을 고르지만 `setschedparam`이 `EPERM`. affinity는 기본값(0–3) 그대로이며 이 작업은 아무것도 설정하지 않았다. 수정 전 실패 시도의 판정 라벨은 rev 1.3 그대로 NOT_READY_UNJUDGED이며, 이 smoke는 판정 출력이 아니다.
5. **앱 포함 정적 범위(D5)**: 기존 `kz-static`(k1r3-llvm)으로 링크 단위마다 basis·extract를 돌렸다(core-tx, cf.so, bp.so, bplib.so). CF·BP의 semaphore 호출 지점 전부와 OSAL 쪽 이름·id writer가 각 단위의 사실에 들어왔다. 그러나 **코어와 앱 `.so`를 하나의 닫힌 프로그램으로 묶는 기능이 없어서**(recipe의 `link`가 하나뿐, app-unit probe 미구현) CF·BP 쪽 id 필드는 단위 안에서 DISABLED이다. 빠진 것을 §8.4에 적었다. **후속(§8.5)**: recipe `module`로 코어와 스크립트가 적재하는 `.so` 6개를 한 닫힘으로 묶었다. 닫힘 조건은 CF·BP 모두 충족, `BP_Throttle_t.Sem`은 writer-set-complete, CF 두 필드는 CF 자체의 cast로 DISABLED가 남는다.
6. ENV 이식(B0–B8)은 시작하지 않았다. P0이 조립·기동됐으므로 계획 §8.0에 따라 공개 통합 구성이 기준이고, ENV 이식은 필요하면 PI가 정하는 비교용 파생이다.

## 1. 구성요소 SHA

| 구성요소 | SHA | 출처(레퍼런스) | 상태 |
|---|---|---|---|
| BP | nasa/bp `b051eb0d12bfe76c0230215c2ff851cfaa553ce4` | D22; CI L54–L57(PR 병합 참조의 트리 = `b051eb0`, 계획 §4.1) | from-reference |
| CF 수정 전 | nasa/CF `281a94188cd7d885a5aed01ee041f3ece2b0486f` | D22; 수동 실행 워크플로의 `nasa/cf@main`(계획 §4.3) | from-reference |
| CF 수정 후 | nasa/CF `833fdbb27a26f15f2429e79392e7eb73d61abdea` | D22; PR 워크플로 `jphickey/cf@fix-184-throttle-sem-race`(`build-run-bp-pullreq.yml` L12–L14), PR #342의 머리(`cases/C1.md` §0a) | from-reference |
| bplib | nasa/bplib `d4c6db9f0d197f78967455fbc83bf1c5e5c31c9c` | CI L14–L21의 `main`; 계획 §8.5가 P-REC에 적은 값 | from-reference(후보). `e7524239`와의 관계는 아래 |
| tinycbor | intel/tinycbor `v0.6.0` → `d393c16f3eb30d0c47e6f9d92db62272f0ec4dc7` | CI L30–L37 | from-reference |
| cFS 번들 (후보 A) | nasa/cFS `1df262a64a09f128357482d8f2e06b0fdfe84d37` (2022-11-17T21:23:45Z) | CI L6–L13 `main`, 계획 §4.3 | **unspecified_by_reference** → 두 후보 모두 빌드 |
| cFS 번들 (후보 B) | nasa/cFS `1b0b338fd14030038fe1aa0e873c485bf5c851e2` (2022-11-28T13:58:55Z) | 같음 | 같음 |
| cFE | A: `6cccb3f5d10664b5b10aeb9cfcda871ba2d99b32` / B: `fef4a0dacabf7e59f054c8aa2c9160a2f3d8db83` | 각 번들의 gitlink [M: `git ls-tree`] | from-reference(후보에 종속) |
| OSAL | A: `f34759d6d6f7087e81afda94eccf7554e013cbe6` / B: `b44f4ce74a309ff93c985539ee8816a2fe92ed23` | 같음 | 같음 |
| PSP | `bd4a2ea239cff013b24bf93a5e8c451dbcb7cf97` (두 후보 공통) | 번들 gitlink `a260e9cc`에 `jphickey/PSP` `techdev-iodriver`를 병합(setup-cfs `action.yml` L26–L31). `a260e9cc`는 `bd4a2ea`의 조상 [M: `git merge-base --is-ancestor`] → fast-forward로 `bd4a2ea` [I]. 브랜치 팁이 창에서 `bd4a2ea`였다는 것은 계획 §4.3의 [I] | from-reference(후보, Q-7) |
| ci_lab · sch_lab · to_lab | `a5e8b4b0` · `91d3b7d5` · `9e2ad8cc` (두 후보 공통) | 번들 gitlink | from-reference |
| sample_app · sample_lib | `a410436f` · A `2da11733` / B `2dc2845a` | 번들 gitlink(빌드 목록에는 없음) | from-reference |
| tools: cFS-GroundSystem · elf2cfetbl · tblCRCTool | `73935aca` · `46b29f82` · `9abe3a7a` | 번들 gitlink | from-reference |

- **bplib 후보 둘(Q-4)의 관계** [F/M]: `git diff d4c6db9 e7524239`는 `CMakeLists.txt` 한 파일, `if (BPLIB_ENABLE_UNIT_TESTS)` 블록 안의 `add_subdirectory(ut-functional)`을 `if (NOT IS_CFS_ARCH_BUILD)`로 감싼 것뿐이다. 이번 빌드의 캐시는 `BPLIB_ENABLE_UNIT_TESTS:BOOL=OFF`(arch 빌드 `native/default/CMakeCache.txt`) [M]이므로 그 블록은 평가되지 않는다. 따라서 P-REC에서 두 후보의 컴파일 입력은 같다 [I, F+M에서]. ENV 이식(단위 시험 플래그 켬)에서는 Q-4가 그대로 남는다.
- **cFS 후보 둘의 차이**: cFE·OSAL·sample_lib gitlink만 다르다. 앱·PSP·도구는 같다 [M]. CI 창에서 어느 쪽이 쓰였는지는 실행 기록이 없다(C1B-U02). 주간 캐시로 `30102b5`(OSAL `e98bb23a`)가 재사용됐을 가능성(계획 §4.3)은 계획 §8.5가 P-REC 후보로 올리지 않았으므로 빌드하지 않았다.

## 2. 사용한 구성 파일 (모두 BP `b051eb0`의 `.github/buildconfig`, 바이트 그대로)

| 항목 | 값 | 근거 | 상태 |
|---|---|---|---|
| 미션 정의 생성 | CI L115–L124의 7개 명령을 경로만 바꿔 그대로 실행(§3 `prep_defs.sh`) | CI L117–L124 | from-reference |
| 시작 스크립트 | `cfe_es_startup.scr` L1 tinycbor(CFE_LIB) · L2 bplib(CFE_LIB) · L3 ci_lab 60 · L4 to_lab 70 · L5 sch_lab 80 · **L6 CF 65** · **L7 BP 55**. 설치된 파일과 원본이 바이트 단위로 같다 [M: `diff`] | 계획 §3.1 | from-reference |
| CPU·앱·PSP 모듈 | `MISSION_CPUNAMES tx rx`(L12), `MISSION_GLOBAL_APPLIST cf bp bplib ci_lab to_lab sch_lab`(L16), `iodriver unsock_intf`(L17), `tx_PROCESSORID 1`(L19), `rx_PROCESSORID 2`(L23) | `targets.cmake` | from-reference |
| CF 표 | `tx/tables/cf_def_config.c`·`rx/tables/cf_def_config.c`: `"BP0_tsem"`(L40), `"BP1_tsem"`(L54). 빌드는 `bpxfer_defs/{tx,rx}/tables/cf_def_config.c`를 컴파일했다 [M: compile DB] | 계획 §3.2 | from-reference |
| BP 흐름 표, sch/to 표 | `{tx,rx}/tables/bp_flowtable.c`, `tables/sch_lab_table.c`, `tables/to_lab_sub.c` | 계획 §3.5 | from-reference |
| tinycbor 설치 | `extra_install.cmake` L13–L18이 `/usr/local/lib/libtinycbor.so`를 `tinycbor.so`로 설치 | CI L110–L111(`sudo cp -rv -t / …`) | from-reference(호스트 변경, §4.4) |
| cmake | `-DCMAKE_BUILD_TYPE=Release -DSIMULATION=native -DCMAKE_INSTALL_PREFIX=/exe -DMISSIONCONFIG=bpxfer -DMISSION_DEFS=<ws>/bpxfer_defs -B build -S <bundle>/cfe` | CI L127 | from-reference |
| make | `make DESTDIR=<ws> -j2 mission-install` | CI L131 | from-reference |
| `CFS_APP_PATH` | `<ws>/src` (`bp`, `bplib`, `cf`) | CI L45–L46, L54–L71 | from-reference |
| `ENABLE_UNIT_TESTS` | 주지 않음(캐시 `ENABLE_UNIT_TESTS:BOOL=` 빈 값, `BPLIB_ENABLE_UNIT_TESTS=OFF`) [M] | CI L127에 없음 | from-reference |
| 컴파일 옵션(결과) | `-O3 -DNDEBUG`(Release), `-std=c99 -pedantic -Wall -Wstrict-prototypes -Wwrite-strings -Wpointer-arith -Werror`(+A만 `-Wcast-align`), `-Wno-format-truncation -Wno-stringop-truncation`, `-fPIC`(앱), 그리고 이탈 `-Wno-error=restrict` [M: compile DB의 플래그 집합] | cFE `sample_defs/arch_build_custom.cmake`(A L36, B L35의 `-Werror`), CI L121이 복사 | from-reference + DV-P1 |

## 3. 빌드 절차와 명령

호스트 [M]: Ubuntu 24.04.4, 커널 `6.18.44-fc-v153`, CPU 4, gcc 13.3.0(`/usr/bin/cc`), cmake 3.28.3, GNU ld 2.42, `fs.mqueue.msg_max` 10. 빌드 사용자 root(CI는 runner 계정 — L111이 `sudo`를 쓰므로 비root로 본다 [I]).

작업 공간 배치(CI 작업 공간 ↔ 이번 경로): `./cfs-bundle` ↔ `c1_build/bundle_<A|B>`(후보마다 하나, 수정 전후가 공유), `./src` ↔ `c1_build/<pre|post>/src`, 작업 공간 루트(`bpxfer_defs`, `build`, `exe`) ↔ `c1_build/<pre|post>/<1df262a|1b0b338>/`.

```sh
# tinycbor (bplib d4c6db9 .github/actions/setup-tinycbor/action.yml L18-L28, 그 다음 CI L111)
cd c1_build/tinycbor-source && make all && make DESTDIR=c1_build/tinycbor-staging install
cp -rv -t / c1_build/tinycbor-staging/*

# prep_defs.sh BUNDLE SRC  (CI L117-L124와 같은 7개 명령; ./cfs-bundle -> BUNDLE, ./src -> SRC)
mkdir ./bpxfer_defs
cp -v $BUNDLE/cfe/cmake/sample_defs/sample_perfids.h ./bpxfer_defs/cfe_perfids.h
cp -v $BUNDLE/cfe/cmake/sample_defs/cpu1_msgids.h ./bpxfer_defs/cfe_msgids.h
cp -v $BUNDLE/cfe/cmake/sample_defs/cpu1_platform_cfg.h ./bpxfer_defs/default_platform_cfg.h
cp -v -t ./bpxfer_defs $BUNDLE/cfe/cmake/sample_defs/*{osconfig,custom,options}.cmake
cp -v -t ./bpxfer_defs $SRC/bp/.github/buildconfig/*.{cmake,scr}
cp -rv -t ./bpxfer_defs $SRC/bp/.github/buildconfig/{tx,rx,tables}
cat $BUNDLE/cfe/cmake/sample_defs/sample_mission_cfg.h $SRC/bp/.github/buildconfig/bp_msgids_extra.h > ./bpxfer_defs/bpxfer_mission_cfg.h

# build_one.sh CFG CAND
export CFS_APP_PATH=c1_build/$CFG/src             # CI L45-L46
export CMAKE_EXPORT_COMPILE_COMMANDS=ON           # DV-P2
export CFLAGS="-Wno-error=restrict"               # DV-P1
cmake -DCMAKE_BUILD_TYPE=Release -DSIMULATION=native -DCMAKE_INSTALL_PREFIX=/exe -DMISSIONCONFIG=bpxfer \
      -DMISSION_DEFS=$PWD/bpxfer_defs -B build -S c1_build/bundle_$CAND/cfe            # CI L127
(cd build && make DESTDIR=c1_build/$CFG/$CAND -j2 mission-install)                    # CI L131
```

스크립트 원본은 `c1_build/{prep_defs.sh,build_one.sh,smoke_one.sh,kz_run.sh}`, 빌드 기록은 `c1_build/<cfg>/<cand>/build.log`, 원문 그대로의 첫 시도는 `c1_build/pre/1df262a/build_attempt1_as_is.log`·`enum_k.log`이다. 네 빌드 모두 `cmake rc=0`, `make rc=0` [M]. 산출물은 `exe/{tx,rx}/core-{tx,rx}`, `exe/{tx,rx}/cf/{cf,bp,bplib,tinycbor,ci_lab,to_lab,sch_lab,cfe_assert}.so`, 표 `cf_def_config.tbl`·`bp_flowtable.tbl`·`sch_lab_table.tbl`·`to_lab_sub.tbl`, `cfe_es_startup.scr`, `exe/host/{cmdUtil,elf2cfetbl,cfe_ts_crc}`.

## 4. 이식 차이와 이탈 대장

### 4.1 첫 실패 지점(원문 그대로의 빌드) — 먼저 기록

- 시도: `pre/1df262a`, CI 명령 그대로(CFLAGS 없음). `make rc=2` [M].
- 오류: `osal/src/os/posix/src/os-impl-filesys.c:171:9`의 `snprintf(local->system_mountpt, sizeof(local->system_mountpt), "%s/osal:%s", tmpdir, local->volume_name)`에서 "`'__builtin___snprintf_chk' argument 7 may overlap destination object 'OS_filesys_table' [-Werror=restrict]`". 두 인수는 같은 표 원소의 서로 다른 필드다 [F]. 진단이 오탐인지는 확인하지 않았다 [U].
- 요구: `-Werror`(cFE `sample_defs/arch_build_custom.cmake` A L36, B L35; CI L121이 복사) [F].
- 범위 확인: `make -k`로 계속 컴파일한 191개 object 중 `-Werror` 오류는 이 한 건뿐이었다 [M: `enum_k.log`]. 최종 네 빌드의 기록에서도 `-Wrestrict` 경고는 빌드마다 이 지점 한 번뿐이고 다른 경고는 없다 [M]. 그래서 강등된 진단은 정확히 이 하나이다.
- 원인 분류(계획 §8.5): **(b) 이 호스트의 도구 차이** [I]. 근거: 오류는 BP·CF·bplib 조합과 무관한 OSAL 내부 코드에서 나고, `-Wrestrict`의 fortify 경로 진단은 컴파일러·glibc 판에 따라 달라진다. CI 러너(`ubuntu-22.04`, CI L51)의 컴파일러 판은 기록이 없다 [U]. 계획 §8.5에 따라 (b)는 "공개 통합 구성은 그대로 구성할 수 없다"는 근거가 아니다.

### 4.2 이탈 대장(P-REC)

| ID | 이탈 | 레퍼런스 | 사용 값 | 이유 | 의미 영향 |
|---|---|---|---|---|---|
| DV-P1 | 진단 강등 `CFLAGS=-Wno-error=restrict`(arch·mission 빌드의 `CMAKE_C_FLAGS`에 들어감 [M: 캐시]) | CI L127(플래그 없음), cFE 샘플의 `-Werror` | 위 값 | §4.1. 경고 하나를 오류에서 경고로만 바꾼다. 다른 `-W` 옵션, 최적화, 정의는 그대로 | 경고 옵션이며 최적화·정의는 그대로다 [I]. 경고가 오탐인지는 보이지 않았고, 과거 컴파일러와 같다는 주장이나 의미 변화가 전혀 없다는 주장으로 넓히지 않는다(P5 결정 2026-10-25). 자원 이름·순서·재시도·우선순위 불변 [M: §6]. 대안(러너 시기의 gcc로 빌드)은 레퍼런스가 컴파일러 판을 정하지 않아 고르지 않았다 → PI-2 |
| DV-P2 | `CMAKE_EXPORT_COMPILE_COMMANDS=ON`(환경변수, CMake ≥ 3.17이 모든 하위 빌드의 캐시 기본값으로 읽음) | 없음 | ON | W3 산출물(compile DB)과 정적 분석 입력 | 컴파일 명령 불변(JSON 파일만 씀) |
| DV-P3 | 작업 공간 경로(§3) | CI 작업 공간 | `c1_build/…` | 저장소 밖, 수정 전후를 별도 트리로 | 경로가 `__FILE__` 문자열로 산출물에 들어감: `bplib.so`의 수정 전후 차이는 이 경로 문자열뿐 [M: `strings` 비교] |
| DV-P4 | 소스 획득: `actions/checkout@v3` 대신 고정 SHA의 `git worktree`. PSP는 병합 대신 병합 결과 SHA `bd4a2ea`를 직접 체크아웃 | setup-cfs `action.yml` L18–L31, CI L54–L71 | §1의 SHA | 움직이는 참조를 고정 SHA로 | 트리 동일 [I: fast-forward]. 번들 버전 문자열이 `git:draco-rc3-45-g1df262a-dirty`(psp gitlink 불일치)로 찍히며 CI의 병합 뒤에도 같았을 것이다 [I] |
| DV-P5 | 호스트·도구·빌드 사용자 | `runs-on: ubuntu-22.04`(L51, 움직이는 이미지) | Ubuntu 24.04.4, gcc 13.3, root 빌드 | 이 컨테이너 | DV-P1의 원인. 그 밖의 차이는 [U] |
| DV-P6 | 번들 캐시 없음 | CI L76–L100(주간 캐시) | 매번 새 트리 | 캐시 상태는 재현 불가 | `30102b5` 후보 미빌드(§1) |
| DV-P7 | build-smoke 절차(§7): 전송 단계(CI L157–L163, L179–L191, L210–L212) 생략. `pre/1df262a`는 정지가 ES 재시작이 아니라 SIGKILL | CI L165–L200 | §7 | smoke는 기동 확인만 한다. SIGKILL은 스크립트 결함(호스트에 `ss` 없음 → 포트 소유 확인 실패 → ES 명령 생략)이며, 남은 공유 메모리 6개(생성 pid가 그 실행의 것)는 지웠다 | 기동 구간(0–5 s)의 관측은 같은 절차. 정지 방식만 다름 |

### 4.3 이식하지 않은 것

소스 코드, 표, 시작 스크립트, 미션 정의 파일은 하나도 고치지 않았다 [M: `diff -r`로 `pre/src`와 `post/src`의 차이가 CF의 3개 파일(`cf_platform_cfg.h`, `cf_cfdp.c`, `unit-test/cf_cfdp_tests.c`, 계획 §7.2)뿐이고, `bpxfer_defs`는 원본 복사본].

### 4.4 호스트에 남긴 변경

- `/usr/local`에 tinycbor 파일·링크 9개와 디렉터리 하나(`bin/cbordump`, `lib/libtinycbor.{a,so,so.0.6,so.0.6.0}`, `lib/pkgconfig/tinycbor.pc`, `include/tinycbor/{cbor.h,cborjson.h,tinycbor-version.h}`). `libtinycbor.so.0.6.0` sha256 `d5a45125af3bf53d906dda59974b77f952f7d1e54051d0bbcf96b618cbe58954` = 설치된 `tinycbor.so`. CI L111과 같은 동작이다. 실행 시에는 시작 스크립트 L1이 `tinycbor.so`를 먼저 적재하므로 `/usr/local`이 필요하지 않다 [I].
- `refs/`에 blobless clone 9개 추가(`ci_lab`, `sch_lab`, `to_lab`, `sample_app`, `sample_lib`, `cFS-GroundSystem`, `elf2cfetbl`, `tblCRCTool`, `tinycbor`)와 worktree 등록.
- smoke가 만든 `/tmp/CFE:1.sock`·`/tmp/CFE:2.sock`(unsock_intf)은 지웠다. 정상 정지(ES POWERON) 실행은 공유 메모리를 남기지 않았다 [M: `ipcs -m` 빈 목록].

## 5. 산출물 해시

compile DB(sha256 전체) [M]:

| 빌드 | `build/native/default/compile_commands.json` (285 항목, 고유 파일 276) | `build/compile_commands.json` (mission, host 도구 4 항목) |
|---|---|---|
| pre/1df262a | `57d406fd5d05fd2b03570db58644ac93b8490f9eb1cfcc0ab789f69946890b78` | `1a32cb5322ba1851a224b847d2df54b1721b28c6f60adc023a3679e554fd4da1` |
| post/1df262a | `2e3879510f06b0ad091eaf10f7331714fdf14a621a9c629d4fc67b2ecfaf2bbf` | `3c0db5ab022fb6ccd3a9e2f663dca16898e5970b636d5aae55b39c15fd9c812b` |
| pre/1b0b338 | `7e48c05a46c0d278affaac0deb54c38f3d53864a4756c81e9c3b5a49927a76de` | `d8bec2b99989495f1ae83247fd611e1143dfe65fbffb723dfc041306aa9d8809` |
| post/1b0b338 | `b68fb7bec608e749e9513b4cd8c03490e819d915b482fcbb74e538c0e2833d6a` | `f7b6146172bbe5f59c8c7df1194330200b6367e6890a52d365ccd82e68a687c3` |

- 구성(arch DB, A 기준): OSAL 112, cFE 55, bplib 21, PSP 20, CF 13, BP 8, 미션 표 8, 생성 파일·헤더 검사 등 나머지 [M].
- 수정 전후 비교: 각 후보에서 두 DB의 경로(`c1_build/pre|post/`)를 정규화하면 항목 집합과 모든 명령이 같다(명령 차이 0) [M]. 해시가 다른 이유는 경로뿐이다.

앱·표(sha256 앞 16자리, 두 후보에서 같음) [M]: `cf.so` 수정 전 `1d27f27f2554dfed` / 수정 후 `9423ab04489bbdf4`, `bp.so` `334b073b63dee85e`(네 빌드 공통), `bplib.so` 수정 전 `1766ee36c5c16afd` / 수정 후 `3f168525b709b7f6`(경로 문자열 차이, DV-P3), `cf_def_config.tbl` tx `455d265e875c1cc3` / rx `b16ae225de620da3`(네 빌드 공통; tx·rx 차이는 `local_eid` 1·2, 계획 §3.2), `cfe_es_startup.scr` `8ddb9359fd2e897b`.

코어 [M]: `core-tx` A pre `f0b1c0ad7f8076b2` / A post `47436ed429965ff8` / B pre `249088e0fbcb0e8a` / B post `c5ebf628e7e257e6`. 수정 전후의 코어 차이는 모듈 버전 표의 CF 버전 문자열(`git:draco-rc3-13-g281a941` ↔ `-14-g833fdbb`)과 빌드 시각 문자열뿐이다 [M: `strings` 비교, 29바이트 차이].

## 6. 유효 설정 대조 (산출물 기준)

| 항목 | 값 | 방법 | 출처 값과 |
|---|---|---|---|
| CF 표의 semaphore 이름 | 채널 0 `BP0_tsem`, 채널 1 `BP1_tsem`(tx·rx 표 모두), 표 이름 `CF.config_table` | 설치된 `.tbl`에서 `strings` [M] | `cf_def_config.c` L40·L54와 일치 |
| BP의 이름·개수·최대값 | `CF_CHANNEL_0_SEM_NAME "BP0_tsem"`, `CF_CHANNEL_1_SEM_NAME "BP1_tsem"`, `BP_NUM_SEM_THROTTLES 2`, `BP_CF_SEM_MAX_COUNT = BP_APP_READ_LIMIT = 10` | `bp_semcfg.c`의 기록된 컴파일 명령에 `-E -dM` [M]; `bp.so`의 문자열 [M] | 계획 §3.3과 일치 |
| CF 재시도(수정 전) | `CF_STARTUP_SEM_MAX_RETRIES`·`CF_STARTUP_SEM_TASK_DELAY` **정의 없음**. 조회 1회(`cf_cfdp.c` L993) | `cf_cfdp.c`의 기록된 명령으로 매크로 평가 [M] | 계획 §5.1과 일치 |
| CF 재시도(수정 후) | `25`, `100`(ms). 조회 루프 L1003, 지연 L1011 | 같음 [M]; 정의 `fsw/platform_inc/cf_platform_cfg.h` L338·L349 [F]. 미션 정의에 이 헤더를 덮는 파일이 없다 | 일치 |
| 그 밖의 CF 상수 | `CF_NUM_CHANNELS 2`, `OS_MAX_API_NAME 20`, 표 파일 `/cf/cf_def_config.tbl` | 같음 [M] | — |
| cFE 시작 관련 | `CFE_PLATFORM_ES_STARTUP_SYNC_POLL_MSEC 50`, `CFE_PLATFORM_CORE_MAX_STARTUP_MSEC 30000`, 시작 파일 `/cf/cfe_es_startup.scr` (두 후보 같음) | `cfe_es_apps.c`의 명령으로 평가 [M] | 계획 §6.1의 50 ms와 일치 |
| OSAL | `OSAL_CONFIG_DEBUG_PERMISSIVE_MODE` 정의됨, `OS_UTILITYTASK_PRIORITY 10`, `OS_MAX_TASKS 64` | `os-impl-tasks.c`의 명령으로 평가 [M] | 계획 §3.6 |
| 시작 순서(적재) | `tinycbor.so` → `bplib.so` → `ci_lab.so`(CI_LAB_APP) → `to_lab.so` → `sch_lab.so` → `cf.so`(CF) → `bp.so`(BP) | 콘솔의 `CFE_ES_ParseFileEntry: Loading …` 줄, 8개 인스턴스 모두 [M] | 스크립트 L1–L7과 일치 |
| 설정 우선순위 | CF 65, BP 55(작을수록 높음, 계획 §3.6) | 스크립트 | 일치. 실제 적용은 §7 |

## 7. build-smoke 관측 (label: build-smoke)

절차(`smoke_one.sh`): CI L133–L135(exe를 tar), L154–L155(풀기), L165–L166(`tx/storage`, `rx/storage`), L168–L174(`./core-tx`, `./core-rx`를 배경 실행, 콘솔은 각 `console_output.log`), L176–L177(`sleep 5` 뒤 OPERATIONAL 확인), 스레드 표 수집, L193–L197(ES 재시작 명령 cmdcode 2, `0x0002` POWERON을 UDP 1234·1235로; 보내기 전에 두 포트의 소유 pid가 이번 실행의 것인지 확인), L199–L200(`sleep 5`). 실행 사용자 `ubuntu`(uid 1000, `setpriv`로 내림): 근거는 cFS `README.md` L107(두 후보 모두) "as a normal user (best effort message queue depth and task priorities)" [F]. root 실행은 이 컨테이너에서 큐 생성이 실패한다는 기록이 있다(`env/CONDITIONS.md` D-4). CPU·affinity·우선순위·sysctl은 아무것도 설정하지 않았다. 기록: `c1_build/<cfg>/<cand>/smoke/{smoke.meta,tx/console_output.log,rx/console_output.log}`.

| 빌드 | 인스턴스 | OPERATIONAL(5 s) | CF | BP | 정지 |
|---|---|---|---|---|---|
| pre/1df262a | tx, rx | 예, 예 | 이벤트 30 "CF: failed to get sem id for name BP0_tsem, error=-17" → `CFE_ES_ExitApp` → "Exit Application CF Completed" | 15 "Created semaphore: BP0_tsem", 15 "Created semaphore: BP1_tsem", 2 "Initialized" | SIGKILL(DV-P7) |
| pre/1b0b338 | tx, rx | 예, 예 | 같음(이벤트 30, error=-17, CF 종료) | 같음 | ES POWERON, 종료 코드 0 |
| post/1df262a | tx, rx | 예, 예 | 이벤트 20 "CF Initialized. Version 3.0.99.0" | 같음 | ES POWERON, 0 |
| post/1b0b338 | tx, rx | 예, 예 | 이벤트 20 | 같음 | ES POWERON, 0 |

- `error=-17`은 `OS_ERR_NAME_NOT_FOUND`이다(`osal/src/os/inc/osapi-error.h` L113, 두 후보) [F]. 수정 전 CF는 채널 0에서 `break`하므로 채널 1(`BP1_tsem`)은 조회하지 않는다(계획 §5.1) [F].
- 수정 후 CF가 첫 조회에서 실패하고 재시도했는지는 콘솔로 알 수 없다: 재시도마다 이벤트가 없고, EVS 줄에는 cFE 시각이 없으며 콘솔 줄 순서는 순서 근거가 아니다(`env/CONDITIONS.md` E8) [U].
- BP의 "Created" 문구가 8개 인스턴스 모두에서 두 이름에 대해 나왔다. 이 실행들에서 BP는 이름을 찾지 못하고 스스로 만들었다 [M, 문구 기준; 계획 §3.3]. `bplib_init failed`·`bplib_route_alloc_table failed`(P-E)는 없었다 [M]. BP CLA는 `/tmp/CFE:1.sock`·`/tmp/CFE:2.sock`을 열었다.
- **스케줄러(실제 적용값)** [M]: 모든 인스턴스에서 OSAL이 "Selected policy 2 for RT tasks, root task = 99"(`SCHED_RR`) 뒤 "Could not setschedparam in main thread: Operation not permitted (1)"을 냈다(`os-impl-tasks.c` L372·L389). `ps -L` 스레드 표(5 s 시점): 모든 스레드 `CLS TS`(`SCHED_OTHER`), rtprio 없음, nice 0. 수정 후는 CF·BP 스레드가 각각 `CF`·`BP`, BP 자식 `BP_CLA_OUT`·`BP_CLA_IN`; 수정 전에는 `CF` 스레드가 없다(종료). 프로세스 상태: `CapEff 0`, `RLIMIT_RTPRIO 0`, `RLIMIT_NICE 0`, `RLIMIT_MSGQUEUE 819200`, affinity mask `f`(`Cpus_allowed_list 0-3`, 기본값), `kernel.sched_rt_runtime_us 950000`, `kernel.sched_autogroup_enabled 1`. 따라서 설정 우선순위(CF 65, BP 55)는 커널 스케줄링에 적용되지 않았다. OSAL·ES가 내부에 기록한 우선순위 값은 이번에 읽지 않았다 [U].
- 이 표는 빌드마다 1회의 기동이다. 수정 전 실패가 "항상" 또는 "자주" 일어난다는 뜻이 아니다. 판정 어휘로 옮기지 않는다(수정 전 단일 실패 조회의 라벨은 rev 1.3 App. A.4 step 6의 NOT_READY_UNJUDGED, 요약 `INCONCLUSIVE(caller_requirement_absent)`). 반복 수는 PI 결정(마스터플랜 §12.3, C1B-U07).

## 8. 앱 포함 정적 범위 (D5)

### 8.1 `.so` ↔ 앱 대응 [M]

| 시작 스크립트 | `.so` | 앱/라이브러리 이름, 진입점 | 링크 입력(번역 단위) |
|---|---|---|---|
| L1 | `tinycbor.so` | CFE_LIB `TC_LIB` | 이 빌드가 만들지 않음(`/usr/local/lib/libtinycbor.so.0.6.0`의 사본). compile DB에 없음 |
| L2 | `bplib.so` | CFE_LIB `BP_LIB` | bplib 21개(`os/src/{cfe,heap,osal}.c`, `store/file_offload.c`, `common/src/{crc,v7_rbtree}.c`, `cache/src/*`(3), `lib/src/*`(4), `mpool/src/*`(6), `v7/src/{v7,v7_codec}.c`) |
| L6 | `cf.so` | APP `CF`, `CF_AppMain`, 65 | CF 13개: `fsw/src/cf_{app,cfdp,cfdp_r,cfdp_s,cfdp_sbintf,cfdp_dispatch,chunk,clist,codec,cmd,crc,timer,utils}.c` |
| L7 | `bp.so` | APP `BP`, `BP_AppMain`, 55 | BP 8개: `fsw/src/bp_{app,flow,cla_bundle_io,storage,global}.c`, `fsw/custom/bp_{semcfg,storecfg,tlmcfg}.c` |
| — | `core-tx`/`core-rx` | 코어(cFE·OSAL·PSP, `iodriver`·`unsock_intf` 포함) | 링크 입력 27(archive 20), 프로그램에 들어간 member 132 |

- 앱 `.so`의 동적 미정의 기호 중 코어가 정의한 **데이터** 기호는 셋 모두 0개이다 [M: `nm -D`]. 즉 CF·BP·bplib는 OSAL 객체 표를 이름으로 쓰지 않고 API로만 닿는다: `cf.so` → `OS_CountSemGetIdByName`, `OS_CountSemTimedWait`; `bp.so` → `OS_CountSemCreate`, `OS_CountSemGetIdByName`, `OS_CountSemGive`; `bplib.so` → countsem API 없음 [M]. 이것은 `static_facts.md`의 가정 G1(적재 앱이 코어 전역을 이름으로 쓰는 경우는 닫힘 밖)이 이 빌드에서는 비어 있다는 확인이다.

### 8.2 실행한 것

도구: `/home/user/work/kairos-build/k1r3-llvm/tools/kz-static/kz-static`(`tool_sha256=2fbab068…`, LLVM `/home/user/work/llvm-project/build`, 둘 다 수정하지 않음). 프로필: `kairos/profiles/cfs-v7.0.1`(유일한 프로필). 링크 단위마다 recipe(`link`, `binary` 두 줄)를 `c1_build/kz/<cfg>_<cand>/<unit>.kzb`로 만들고 `basis` → `extract`(`--delay-op OS_TaskDelay --lock-op CFE_ES_LockSharedData --unlock-op CFE_ES_UnlockSharedData --spawn-op CFE_ES_CreateChildTask`, 대상은 `--target`)를 돌렸다. 모두 종료 코드 0 [M]. 각 basis는 프로필이 고정한 ENV SHA와 소스 트리 4개가 다르다는 revision deviation 4건을 기록했다(의도된 결과).

| 단위(빌드) | 번역 단위 | 함수(본문) | 호출 지점 | 간접(해결/미해결) | 대상 위치와 상태 |
|---|---|---|---|---|---|
| core-tx (pre/A, pre/B 같은 수) | 132 | 1290 (1120) | 5007 | 41/10 | `OS_common_record_t.name_entry` writer 18, `.active_id` writer 6, `OS_count_sem_internal_record_t.obj_name` writer 3, 모두 A-TYPE 0 → writer-set-complete(이 단위 안에서) |
| cf.so (pre/A) | 13 | 339 (288) | 820 | 10/8 | `CF_Channel_t.sem_id` writer 1(데이터 함수) + escape 1·cast 1·untyped 10 → DISABLED; `CF_ChannelConfig_t.sem_name` writer 0, cast 1·unknown-effect 2·untyped 10 → DISABLED |
| cf.so (post/A) | 13 | 340 (288) | 821 | 10/8 | 같음 |
| bp.so (pre/A, post/A) | 8 | 190 (120) | 394 | 1/1 | `BP_Throttle_t.Name` 초기값만, A-TYPE 0 → writer-set-complete; `BP_Throttle_t.Sem` escape 2 → DISABLED |
| bplib.so (pre/A) | 21 | 589 (526) | 1523 | 21/5 | countsem 대상 없음(호출 없음) |

semaphore 호출 지점(사실 컨테이너의 `calls`) [M]:
- CF 수정 전 `cf_cfdp.c:993` / 수정 후 `:1003`(루프 안) `CF_CFDP_InitEngine → OS_CountSemGetIdByName(&CF_AppData.engine.channels[i].sem_id, <CF_ChannelConfig_t.sem_name>)`; `cf_cfdp_sbintf.c:93` `CF_CFDP_MsgOutGet → OS_CountSemTimedWait(<CF_Channel_t.sem_id>, 0)`.
- BP `bp_flow.c:126` `initialize_throttling → OS_CountSemGetIdByName(&BP_GlobalData.Throttles[index].Sem, <BP_Throttle_t.Name>)`, `:132` `→ OS_CountSemCreate(&…Sem, <Name>, <Max>, 0)`, `:184` `disperse_throttling → OS_CountSemGive(<Sem>)`.
- DISABLED의 원인 [M]: `CF_Channel_t.sem_id`·`BP_Throttle_t.Sem`은 주소가 `OS_CountSemGetIdByName`·`OS_CountSemCreate`로 넘어가는데 그 본문이 앱 단위에 없다(본문은 core 단위의 OSAL). `CF_ChannelConfig_t.sem_name`은 표 포인터가 `CFE_TBL_GetAddress`(`cf_app.c:96`, `:184`)로 넘어가고 표 내용은 코어의 TBL이 `.tbl`에서 채운다. CF의 untyped 10건은 `cf_codec.c`·`cf_cfdp.c` 등의 `void*`/`uint8*` 저장이다.

### 8.3 범위 판단

- 대상 자원의 변경 경로 중 **CF·BP 쪽 호출 지점과 OSAL 쪽 writer는 모두 사실로 들어왔다**. 이름의 출처도 닫혔다: BP 이름은 정적 초기값뿐이고(writer 0), CF 이름은 표 파일 바이트로 확인했다(§6, 도구 밖의 [M]).
- 그러나 **하나의 닫힌 프로그램(코어 + 적재 `.so`)으로서의 writer 집합은 계산되지 않았다**. 앱 단위에서 DISABLED인 두 id 필드는 코어 단위의 OSAL 본문과 묶여야 풀린다. 그래서 D5의 완료 조건("정적 닫힘 범위가 writer·별칭 포괄 요구를 충족")은 **아직 충족되지 않았다**.

### 8.4 빠진 것(정확히)

1. **다중 링크 단위**: recipe 문법은 `link`를 하나만 받는다(`lib/llvm/recipe.cpp` L86–L87, 두 번째 `link`는 앞의 값을 덮어씀). 코어와 `dlopen`으로 적재되는 `.so`를 한 프로그램으로 묶는 app-unit probe(SPEC §4.1 6)는 미구현이다(`kairos/docs/static_facts.md` §1 표 "link unit / app-unit probe" 행). 필요한 것: 여러 link 단위의 합집합과 단위 사이 기호 해석(앱의 미정의 함수 → 코어 정의), 그리고 적재 순서의 근거(시작 스크립트)를 basis에 싣는 것. → §8.5에서 구현(소스 수준).
2. **`.tbl` 바이트 대조**: 표 내용(`sem_name`)이 실행 시 코어 TBL이 파일에서 채우는 값이라 도구가 static image로 볼 수 없다. `.tbl` 대조는 미구현이다(같은 문서 §1 "상수·표" 행).
3. **C1 프로필**: 프로필은 `cfs-v7.0.1` 하나이고 ENV SHA를 고정한다. 2022 스택의 `source` 선언, countsem 종류·조회/생성 연산(`K-REG.osal-countsem-lookup`)·대상 위치가 프로필에 없어서 이번에는 대상을 명령줄 `--target`으로 줬다(사람이 넣은 입력 6건으로 기록됨). K4b(census, Φ 대응)는 이 범위에서 돌리지 않았다.
4. **바이너리 연결 관측점**(마스터플랜 W3 산출물 "바이너리에 연결된 프로필·관측점"): 하지 않았다(W2의 `sites.kzp` 보강과 W4의 몫).

### 8.5 결합 닫힘 (D5 후속, 2026-10-24) [M]

정적 분석만 했다(cFS 실행 없음, 실험 결과 아님). 도구: 이번 커밋의 `kz-static`(k1r3-llvm 재빌드, `tool_sha256=9647b97a…`)과 recipe `module`(`kairos/docs/static_facts.md` §3.1). 실행 `c1_build/kz_run.sh <cfg> <cand> closure`(W3 판은 `kz_run.sh.w3`로 보존), 산출물 `c1_build/kz/<cfg>_<cand>/closure.{kzb,basis.kz,facts.kz,basis.log,extract.log}`. recipe 값의 근거 줄은 A 번들(cFE `6cccb3f`, OSAL `f34759d`, PSP `bd4a2ea`) 기준이며, B 번들의 해당 파일 5개는 바이트가 같다 [M: `cmp`].

| recipe 항목 | 값 | 근거 | 상태 |
|---|---|---|---|
| 실행 파일 | `core-tx`의 link·binary(§8.2와 같음) | 빌드 트리 | from-reference |
| 스크립트 선택 | `reset POWERON` → nonvolatile `CFE_PLATFORM_ES_NONVOL_STARTUP_FILE`(`target_config.c`에서 평가 = `/cf/cfe_es_startup.scr`) | `cfe_es_apps.c` L86–L150, `cfe_psp_start.c` L67·L387–L415·L429, `target_config.c` L143–L145. reset 종류는 basis 입력이며, 네 빌드의 smoke 콘솔이 "Starting the cFE with a POWER ON reset"을 냈다(§7) | declared(근거 있음) |
| `/cf` 대응 | `../exe/tx/cf` | `cfe_psp_start.c` L355–L358(`./cf` → `/cf`), smoke는 `tx/`에서 실행(`smoke_one.sh`) | from-reference |
| 모듈 파일 | 스크립트 필드 1 + 기본 경로 `/cf` + 기본 확장자 `.so`(둘 다 `target_config.c`에서 평가) | `cfe_es_apps.c` L316–L317, `cfe_fs_api.c` L53–L110·L582–L588, `target_config.c` L171 | from-reference |
| 기호 범위 | `CFE_LIB` global, `CFE_APP` local, `RTLD_NOW` | `cfe_es_apps.c` L410–L430, `os-impl-posix-dl-loader.c` L72–L96 | from-reference |
| 모듈 link | bplib·ci_lab·to_lab·sch_lab·cf·bp·cfe_assert의 `link.txt` | 묶을지는 스크립트가 정한다. link 출력과 적재 파일의 sha256이 같음을 모듈마다 확인 [M] | from-reference |

결과(네 빌드 모두 basis·extract 종료 코드 0):

| 항목 | pre/A | post/A | pre/B | post/B |
|---|---|---|---|---|
| 번역 단위 | 177 = core-tx 132 + 모듈 45(bplib 21, cf 13, bp 8, ci_lab·to_lab·sch_lab 각 1) | 177 | 177 | 177 |
| 모듈 미정의 기호의 결합 | core-tx 178, 라이브러리 31(bp→bplib 17, bplib→tinycbor 14), 없음 46(시스템 라이브러리 몫) | core-tx 179(CF의 `OS_TaskDelay`) | 178 | 179 |
| 단위 사이 참조 / 적재기와 다른 결합 / 두 번 정의된 이름 | 195 / 0 / 0 | 196 / 0 / 0 | 195 / 0 / 0 | 196 / 0 / 0 |
| 함수(본문) / 호출 지점 | 2180 (1981) / 7908 | 2180 (1981) / 7909 | 2180 (1981) / 7908 | 2180 (1981) / 7909 |

닫힘 밖: `tinycbor.so`(L1, 이 빌드가 만들지 않아 compile DB 없음; 내보내는 기호 43개는 결합 판단에만 씀), `cfe_assert.so`(스크립트에 없음), 시스템 라이브러리.

대상 위치(네 빌드 같은 값; 줄은 수정 전/후 CF):

| 위치 | W3(단위별, §8.2) | 결합 닫힘 |
|---|---|---|
| `BP_Throttle_t.Sem` | DISABLED(escape 2) | **writer-set-complete**: out-pointer writer 2(`bp_flow.c:126` → `OS_ObjectIdFindByName` `osapi-idmap.c:978`; `:132` → `OS_ObjectIdFinalizeNew` `:860`), A-TYPE 0 |
| `BP_Throttle_t.Name` | writer-set-complete(초기값만) | 같음 |
| `CF_Channel_t.sem_id` | DISABLED(escape 1, cast 1, untyped 10) | **DISABLED**: writer 2(memset `cf_cfdp.c:965`, out-pointer `:993`/`:1003` → `osapi-idmap.c:978`), escape 0, cast 1, untyped 133. escape는 풀렸고, 남은 원인은 CF 자체의 cast `cf_cmd.c:734` `(CF_CListFn_t)CF_PurgeHistory`(채널 포인터를 `void *` 문맥으로 넘김)다. cast 노출이 있으면 프로그램의 untyped 쓰기 133곳이 모두 센다 |
| `CF_ChannelConfig_t.sem_name` | DISABLED(cast 1, unknown-effect 2, untyped 10) | **DISABLED**: writer 0, cast 1(`cf_app.c:114`, `void *tbl_ptr` → `CF_ConfigTable_t *`), unknown-effect 0, untyped 133. 표 내용은 코어 TBL이 파일에서 untyped 복사로 채우므로 `.tbl` 대조(§8.4 2) 없이는 닫히지 않는다 |
| OSAL `OS_common_record_t.name_entry`·`.active_id`, `OS_count_sem_internal_record_t.obj_name` | writer-set-complete(18·6·3) | 같음(앱 코드가 A-TYPE을 더하지 않음) |

countsem API 호출자(닫힘 안 전부): BP `bp_flow.c:126` GetIdByName, `:132` Create, `:184` Give; CF `cf_cfdp.c:993`/`:1003` GetIdByName, `cf_cfdp_sbintf.c:93` TimedWait; sch_lab `sch_lab_app.c:168` Create(`"SCH_LAB"`), `:146` Give, `:100` Take. bplib·ci_lab·to_lab에는 없다.

판단:
- **D5의 닫힘 조건**(코어와 적재 `.so`를 한 프로그램으로 묶는 app-unit probe, SPEC §4.1 "Closure per case")은 CF·BP 모두 **충족** [M]: 위 writer·별칭(id 쓰기 경로, 이름 복사 경로, API 호출자)이 한 facts 컨테이너에 있고, 이름 결합이 적재기 결합과 같다(차이 0, 중복 0).
- **writer 집합 상태**: BP 두 필드와 OSAL 쪽 세 위치는 writer-set-complete. CF 두 필드는 DISABLED로 남는다. 원인은 닫힘 경계가 아니라 CF 코드 안의 cast(A-TYPE)이고, SPEC상 그 위치에 기대는 conjunct는 `unknown(writer_incomplete)`가 된다(SPEC §4.2 A-TYPE, O7). S3b(points-to)는 DISABLED가 인증서를 막을 때만 만든다(SPEC §4.1 10).
- 남은 것: `.tbl` 대조(§8.4 2), C1 프로필(§8.4 3), IR 수준 link와 `-fcallgraph-info` 대조(SPEC §4.1 6의 나머지), 바이너리 관측점(§8.4 4).
- **PI 확인(결정하지 않음)**: C1 단계 B의 완료에 CF 쪽 두 필드의 writer-set-complete까지 필요한가. 레퍼런스가 정하지 않는 이유: SPEC §4.1 "Closure per case"는 "the closure covers the writers and aliases its conditions need"라고만 하고 A-TYPE 목록에 오른 별칭이 '포괄됨'에 드는지 말하지 않는다. 어느 conjunct가 이 필드를 support location으로 쓰는지는 아직 없는 C1 Φ 프로필이 정한다. 현행: DISABLED를 그대로 보고하고 S3b나 선언 규칙은 만들지 않았다.

## 9. 원 보고(#184)·BP CI와의 차이

| 항목 | 원 보고 #184 | #184 댓글 2·3·5 (2022-08/11) | BP CI(`b051eb0` 워크플로) | 이번 빌드 |
|---|---|---|---|---|
| CF | `2a024d8`(표 구조가 BP 표와 다름, 섞지 않음 — 계획 §5.2) | 미상 | 수동 `main`=`281a941`, PR `833fdbb`(추정) | `281a941` / `833fdbb` |
| provider | "CI/TO or some other dedicated I/O app"(F1) | BP(공개 커밋과 동일시되지 않음, `cases/C1.md` §0a [I]) | BP `b051eb0` | BP `b051eb0` |
| cFS·cFE·OSAL·PSP | 미상(C1B-U01) | 미상 | 움직이는 `main`(C1B-U02) | 후보 A·B 모두, PSP `bd4a2ea` |
| bplib | — | 미상 | 움직이는 `main` | `d4c6db9`(`e7524239`와 컴파일 입력 같음, §1) |
| 시작 순서 | 미상 | BP가 CF보다 **먼저** 시작(댓글 2) | 스크립트: CF(L6) 다음 BP(L7) | 스크립트 그대로 |
| OS·도구 | Ubuntu 21.10 | 미상 | `ubuntu-22.04` 러너, 컴파일러 미기록 | Ubuntu 24.04.4 컨테이너, 커널 6.18.44, gcc 13.3 + DV-P1 |
| CPU·스케줄링 | 미상(C1B-U04) | 미상 | 미기록 | CPU 4, 비root, 전 스레드 `SCHED_OTHER`(§7) |
| 인스턴스 | 미상 | 미상 | tx·rx 두 개 | tx·rx 두 개(CI 그대로) |
| 빌드 형식 | 미상 | 미상 | Release | Release |
| 관측된 실패 출력 | 없음(C1B-U17) | "CF bails out"(질적) | 로그 없음 | 수정 전 이벤트 30 `error=-17`(build-smoke 1회/빌드) |

- 이번 구성은 원 보고 환경의 재현이 아니다(결정문 §3.2, SPEC App. A.4 step 2 [D]). 댓글 2의 "BP를 먼저 시작" 구성과도 다르다(이번에는 스크립트대로 CF가 먼저).
- `msg_max`는 10이고 OSAL permissive 모드가 비root 실행에서 큐 깊이를 그 값으로 줄인다(`env/CONDITIONS.md` C6). 러너의 값은 기록이 없다 [U].

## 10. 남은 것과 PI 결정

- **PI-1 (cFS 번들 후보)**: `1df262a`와 `1b0b338` 중 W4의 기준(또는 둘 다). 레퍼런스가 정하지 않는 이유: CI는 실행 시점의 `nasa/cFS@main`을 쓰고(CI L6–L13) 주간 캐시를 재사용할 수 있으며(L76–L100), 실행 기록·로그가 없다(C1B-U02). 두 후보는 cFE·OSAL만 다르고 이번 smoke에서 같은 결과를 보였다.
- **PI-2 (DV-P1 수용)**: gcc 13.3에서 `-Wno-error=restrict` 하나로 진단을 강등한 빌드를 P0의 "그대로 구성됨"으로 볼지, 러너 시기의 컴파일러를 따로 구하라고 할지. 레퍼런스가 정하지 않는 이유: `runs-on: ubuntu-22.04`(L51)는 갱신되는 이미지이고 그때의 컴파일러 판은 어디에도 기록되지 않았다. 의미 영향(이름·순서·재시도·우선순위)은 없음을 §6에서 대조했다. **결정(2026-10-25, P5) [D]:** 명시한 빌드 이탈로 수용. 표현은 "실험 대상 소스와 설정을 수정하지 않고, 기록한 경고 강등을 적용해 공개 구성을 빌드했다"로 한다. 경고가 오탐이라는 입증이 아니며 과거 컴파일러와의 동등성을 주장하지 않는다. 원 CI 이미지 복원은 선행 조건이 아니다(RV §9.1).
- **PI-3 (Q-5의 후속)**: P0은 조립·기동됐다. 계획 §8.0에 따라 ENV 이식(B0–B8)은 비교용 파생이며 할지 말지는 PI가 정한다. 그 경우 Q-1·Q-2·Q-4가 다시 필요하다.
- **PI-4 (인스턴스, Q-3)**: P-REC는 CI대로 tx·rx를 함께 띄운다. W4에서 한 인스턴스만 관측할지 둘 다 관측할지는 레퍼런스가 정하지 않는다(C1B-U10).
- 열린 기술 항목: D5의 남은 부분(§8.4 2–4; 결합 닫힘 자체는 §8.5), 수정 후 첫 조회 실패·재시도 여부의 관측(§7, W4의 관측기 몫), OSAL·ES 내부 우선순위 기록값 [U], G-REBIND(`833fdbb`의 `caller` 기록 다시 묶기, 계획 §9.3)는 W4 첫 출력 전 선행 조건으로 그대로 남는다.
- 위험 목록(마스터플랜 §11) 갱신: **F01** — 실제 실행에서 CF 조회와 BP 생성이 서로 다른 앱·스레드(`CF`, `BP`)에서 같은 이름으로 일어남을 콘솔·스레드 표로 확인 [M]; task→생성 앱 매핑의 도구 경로는 아직 없음(W2). **F05** — 이번 smoke에는 관측기가 없다; 판정에 필요한 사건(생성·조회 순서, 재시도) 중 콘솔로 얻을 수 없는 것이 있음을 확인(§7). **D5** — 단위별 범위는 확보, 결합 닫힘은 미충족(§8.3).

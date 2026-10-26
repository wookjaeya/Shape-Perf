# 컴파일러 연결부 후보 1: GCC GIMPLE plugin의 제한된 적합성 확인 (단계 C, 2026-10-26)

- 범위: IR(사용자 업로드 `Kairos_OS_Independent_Instrumentation_Review_20261026.md`) §5·§5.1, §13.2, §15 단계 C, §15.1의 일곱 질문을 **GCC 13.3 GIMPLE plugin 한 후보**에 대해 증거로 답한다. 결정 기록 `Architecture/spec/PI_DECISIONS_RESOLVED_20261026.md`(PD26) §2.3(세 후보 비교 후 하나를 고름), §4(런타임·계측 필수 조건, 구속)를 따른다. 런타임과 ABI는 B1(`kairos/runtime/`, 커밋 `a335519`, `docs/event_semantics.md`)을 **고치지 않고** 썼다.
- 표기: **[F]** 코드·문서에서 확인, **[M]** 이번에 빌드·실행해 확인, **[I]** 추론, **[D]** 설계 결정, **[U]** 미확인.
- 지위: 시제품과 **pilot/dev 부팅 각 1회**(C1 tx, R2)다. 본 실험 결과가 아니며 반복 횟수·빈도·비용을 주장하지 않는다. 계측 빌드는 기준 빌드의 **파생 구성**이고 차이는 §3에 모두 적었다(CONDITIONS_POLICY). 연구 조건을 새로 고르지 않았다.
- 산출물(저장소): `kairos/instrument/gcc/` — plugin `kzgcc.cpp`, 논리 site manifest `sites/{c1_post_1df262a,r2_v701}.kzs`, 빌드·실행·대조 스크립트 `scripts/`, 단위 시험 `tests/`, dump 이벤트 출력 도구 `tools/kzgcc_events.c`. 빌드 트리·실행 기록(저장소 밖): `/home/user/work/instr/gcc/{plugin-build,runtime-build,c1,r2,env}`.

## 0. 요약

1. **설치**: 같은 판의 배포 패키지 `gcc-13-plugin-dev 13.3.0-6ubuntu2~24.04.1`만 설치했다(의존 `libgmp-dev`, `libmpfr-dev`, `libmpc-dev`, `libgmpxx4ldbl`). 컴파일러 파일은 바뀌지 않았다(`dpkg --verify`, §1) [M].
2. **시제품**: `kzgcc` plugin은 manifest의 논리 site를 읽어, 그 site를 가진 번역 단위에만 B1 hook 호출을 넣는다(호출 전·후, 함수 진입·반환, 포인터 인자를 통한 store). 삽입은 `ssa` 직후(최적화 전), 확인은 `optimized` 직후다. 단위마다 `kzs-sites 1` binding 파일을 쓰고 `bound / inlined / duplicated / removed / unsupported`를 판정한다. 런타임 TU는 거부한다. 단위 시험 6개 통과(-O0/-O2/-O3/-O2 LTO, 무관 단위 불변, 런타임 거부) [M].
3. **빌드**: C1(Release `-O3`, PIE + `.so`)·R2(debug `-O0 -g`, 단위 시험 포함)를 기준 명령 그대로 빌드했다. 바뀐 객체는 계측한 번역 단위뿐이다(C1: 261개 중 6개, 나머지는 바이트 동일. R2: §3.3) [M].
4. **대조(IR §13.2)**: C1 tx 부팅의 정체성 표(OSAL task id 17개, 소유 앱 id, 생성 task)와 CF·BP 세마포어 조회·생성 순서(이름, 반환 코드, id)가 F0 smoke3 기록과 **같다**. R2는 §5.2 [M].
5. **판단(§6)**: 일곱 질문에 대해 이 후보는 C1·R2의 목표 지점을 모두 연결했고, 값은 원 명령의 operand로 전달됐으며, 위치 대응은 plugin 보고·바이너리 검사·실행 pc로 세 번 확인된다. 남은 위험은 GCC 내부 API의 판 고정, 교차 컴파일러의 plugin 지원, 라이선스 선언, `-g` producer 문자열, LTO에서의 확인 지연이다(§7). 세 후보 비교와 선택은 다른 후보의 결과와 함께 한다(PD26 §2.3) [D].

## 1. 환경 변경 (PD26 §1 표 마지막 행)

| 항목 | 값 | 근거 | 상태 |
|---|---|---|---|
| 필요 | `gcc-plugin.h` 등 plugin 헤더(설치된 gcc 13.3.0에 없음) | PD26 §1 | 기존 환경에 없는 기능 |
| 설치 | `apt-get install --no-install-recommends gcc-13-plugin-dev` → `13.3.0-6ubuntu2~24.04.1`(noble-updates/security), 의존 4개(위) | 설치된 `gcc-13 13.3.0-6ubuntu2~24.04.1`과 같은 판 | deviation(환경 변경, P-LIBBPF와 같은 규칙) |
| 검증 | `dpkg --verify gcc-13 cpp-13 …`: 바이너리 불일치 0(문서 파일 누락만). `cc1` sha256 `5d167913…`, `/usr/bin/cc` → `x86_64-linux-gnu-gcc-13` | `/home/user/work/instr/gcc/env/{apt_install.log,toolchain.txt}` | [M] |

## 2. 시제품 `kzgcc` (IR §5.1)

### 2.1 무엇을 넣는가 [D][M]

| manifest 형태 | 넣는 곳 | 값(`value … from=`) |
|---|---|---|
| `phase=before callee=C func=F` | F 안의 C 직접 호출 바로 앞 | `arg<k>` = 그 호출의 GIMPLE 인자 operand(이미 평가된 SSA 값·불변식; 다시 평가하지 않음) |
| `phase=after callee=C func=F` | 같은 호출 바로 뒤 | `ret` = 호출 결과(결과를 버리는 호출이면 새 SSA 이름을 붙임), `out<k>` = 같은 스레드가 호출 직후 읽은 `*arg<k>`(event_semantics.md §8.1의 허용 규칙) |
| `phase=before`, callee 없음 | F의 진입 간선 | 매개변수의 기본 정의 |
| `phase=after`, callee 없음 | F의 각 `return` 앞 | `ret` = 반환 operand, `link` = 진입 기록 |
| `phase=effect`, callee 없음 | F 안에서 포인터 매개변수를 통한 store 바로 뒤(`line=`이 있으면 그 줄만) | `operand0` = 저장한 값, `operand1` = 주소 |
| `control <site> ctx=` | 관측 hook 바로 뒤 `kz_rt_ctl(site, bind, aseq, ctx)` | 관측 hook과 별도 호출(관측 callback에 대기를 섞지 않음, PD26 §4) |

- 값은 manifest의 `type`대로 0/부호 확장해 64비트로 넘긴다(포인터는 정수로; 8바이트 이하 집합체는 `VIEW_CONVERT_EXPR`). store를 atomic으로 바꾸거나 값을 교체하지 않는다 [F: `kzgcc.cpp` `to_u64`, `build_obs`].
- 문자열은 `string=validated` 슬롯에서만 `kz_rt_obs_str`로 상한 복사한다. plugin은 operand의 타입이 알려진 크기의 `char` 배열 주소인지 추가로 확인해 binding 파일에 적는다(C1: CF `array char[20] (address of array)`, BP `array char[20] (pointer type)`) [M].
- `kz_rt_obs`에는 `leaf`(대상 메모리를 읽지 않음)를 붙여 주변 코드의 최적화 교란을 줄였다. `kz_rt_obs_str`, `kz_rt_ctl`에는 붙이지 않는다(문자열을 읽음, 대기 가능) [D].
- binding id = `(module << 16) | (site << 4) | n`(event_semantics.md §3 권고). module은 plugin 인자 `module=<id>:<경로 접두사>`로 정한다. site의 `tu`가 단위 경로의 접미사이고 `func`가 함수 이름과 같을 때만 연결한다.
- 모듈 등록: site가 있는 단위마다 `*free_lang_data` 직후의 작은 IPA pass가 정적 생성자 `kz_rt_module(module, manifest sha256)`를 만든다 [M: dump MODULE 레코드].

### 2.2 pass 위치·최적화·LTO와 확인 (IR §5.1 "pass 위치·최적화·LTO 설정 기록")

- 삽입: `ssa` 첫 인스턴스 직후(`pass_build_ssa_passes` 안, `*rebuild_cgraph_edges` 전). 모든 최적화 전이므로 hook은 inlining·복제·unrolling을 원 코드와 함께 따라간다 [D].
- 확인: `optimized` 직후 함수마다 hook 호출을 binding id별로 센다. 0이면 `removed`, 1이고 원 함수면 `bound`, 다른 함수면 `inlined`, 2 이상이면 `duplicated`. RTL 단계의 복제는 이 확인이 보지 못하므로 링크된 파일을 `scripts/bincheck.sh`(objdump, `%esi`에 들어간 binding id)로 다시 센다 [M].
- binding 파일 머리에 GCC 판, 단위, dumpbase, manifest sha256, `-O`, LTO, PIC/PIE, `-fprofile-arcs` 여부를 적는다. LTO(`-flto`, fat 아님)에서는 확인을 링크 단계로 미루고 `check=deferred-to-lto-link`로 표시한다(시험 `kzgcc_target_O2-lto`: 계측은 LTO 뒤에도 남고 값이 맞다) [M]. C1·R2 기준 플래그에는 LTO가 없다 [F: compile DB].
- 판 고정: `plugin_default_version_check`로 빌드한 GCC와 정확히 같은 판만 받는다. GCC 판 사이의 내부 API 호환성을 전제하지 않는다 [F].
- 런타임 비계측: (1) 런타임은 plugin 없이 따로 빌드한다(B1 문서의 `cmake -S kairos/runtime -B <dir>`), (2) 단위가 `kz_rt_*` 함수를 정의하면 파싱 직후 그 단위를 거부하고 아무것도 넣지 않는다(시험 `kzgcc_refuses_runtime`: 객체 바이트 동일) [M].
- site가 없는 단위는 pass gate가 거짓이라 손대지 않는다: `-O2` 객체 바이트 동일, `-O0 -g`는 DWARF producer 문자열에 plugin 옵션이 들어가는 것만 다르다(GCC 기본 `-grecord-gcc-switches`) [M: `kzgcc_untouched_unit`].

### 2.3 시험 [M]

`cmake -S kairos/instrument/gcc -B /home/user/work/instr/gcc/plugin-build && cmake --build … && ctest`: 6개 통과. `kzgcc_target_{O0,O2,O3,O2-lto}`는 두 스레드 시험 대상(CF·BP·SB·OSAL 모양: 재시도 루프의 조회와 out id, 한 번만 불리는 static helper의 루프, 두 분기의 포인터 store, 결과를 버리는 호출, task id 전달)을 빌드·실행하고 `kz-rt-dump --manifest`(실패 0)와 값(이름 복사, `-17` 부호 확장, out id `0x30001`, store 값, 생성·진입 task id, link)을 확인한다. 시험 값은 단위 시험 값이며 연구 조건이 아니다.

## 3. 파생 빌드 (C1, R2)

### 3.1 차이 대장

| ID | 차이 | C1 (`scripts/build_c1.sh`) | R2 (`scripts/build_r2.sh`) | 영향 |
|---|---|---|---|---|
| D1 | 작업 경로 | `/home/user/work/instr/gcc/c1/{bundle_1df262a,src,ws}` | `/home/user/work/instr/gcc/r2/cFS` | `__FILE__`·디버그 정보의 경로 문자열(DV-P3와 같은 종류) |
| D2 | `.git` 없는 사본 | 기준 트리의 `.git` 파일이 `refs/`의 worktree를 가리켜 git이 그 index를 다시 쓸 수 있으므로 뺐다 | ENV 트리의 `.git`·`build-native_std` 제외 | 모듈 버전 표가 NULL이 되는 부작용을 cFE의 문서화된 hook(`<mission defs>/generate_module_version.cmake`, `cfe/cmake/mission_build.cmake` C1 L125–129, R2 L130–137)으로 기준 빌드의 생성 파일을 바이트 그대로 설치해 없앴다(2026-10-08 CORD 사본 결정과 같은 방법, `results/R2_cord_v701.md` §7). 생성된 `cfe_module_version_table.c`가 기준과 같은 sha256(C1 `431189d0…`, R2 `4615470b…`) [M] |
| D3 | arch 빌드 cache(계측 빌드만) | `build/native/default`: `CMAKE_C_FLAGS = -Wno-error=restrict` + plugin 옵션, `CMAKE_EXE_LINKER_FLAGS` = 런타임(whole archive `libkzrt-posix.a libkzrt.a` + 시작 객체 `kz_rt_posix_auto.c.o`) | `native/default_cpu1`만 같은 방식(+`skip-coverage`); `default_cpu2`와 mission(host 도구) 빌드는 그대로 | configure 직후 cache를 다시 설정; 소스·설정·표·시작 스크립트는 바뀌지 않음 |
| D4 | 빌드 날짜·사용자·호스트 문자열 | `cfe_build_env_table.c`(빌드 시 생성) | 같음 | 콘솔 출력 문자열 |
| — | 빌드 명령·환경 | `c1_build/build_one.sh` 그대로(`CFS_APP_PATH`, DV-P1, DV-P2), root | `make native_std.prep` / `.install`을 `runuser -u ubuntu`로, `env/build_cfs.sh`의 거부 변수 목록 확인 | from-reference(환경 이름은 하네스 세션의 것; 값은 기록하지 않음) |

### 3.2 기준 빌드 재현 확인 [M]

계측 전에 같은 경로에서 기준 명령 그대로 한 번 빌드했다(같은 경로라야 객체를 바이트로 비교할 수 있다).
- C1: `cf.so`·`bp.so`가 기준 빌드와 **바이트 동일**, `core-tx`는 `.text` 동일이고 문자열 두 개(빌드 시각, `cfe_sb_task.c`의 `__FILE__`)만 다르다.
- R2: `core-cpu1`의 크기가 같고, `.text`의 명령 열은 같으며 경로 문자열이 짧아져 바뀐 rip 상대 주소 20바이트만 다르다.

### 3.3 계측이 바꾼 것 [M]

- C1: 객체 261개 중 바뀐 것은 계측 단위 6개뿐(`cfe_es_api.c`, `cfe_es_apps.c`, `osapi-task.c`, `os-impl-tasks.c`, `cf_cfdp.c`, `bp_flow.c`). 나머지 255개는 바이트 동일.
- R2: `-g` 빌드이므로 cpu1 arch 빌드의 객체 1,335개가 모두 원시 바이트로는 다르다(DWARF producer 문자열에 plugin 옵션; §2.2). `objcopy --strip-debug` 뒤에 다른 것은 276개: 계측 단위 8개(`cfe_es_api.c`, `cfe_es_apps.c`, `cfe_evs.c`, `cfe_sb_api.c`, `cfe_sb_priv.c`, `cfe_sb_task.c`, `osapi-task.c`, `os-impl-tasks.c`), D4의 `cfe_build_env_table.c.o` 1개, 단위 시험 coverage 객체 267개. coverage 객체는 plugin과 무관하게 빌드마다 다르다(gcov stamp; 같은 파일을 `--coverage`로 두 번 컴파일해도 다름을 확인). plugin은 이들을 `skip-coverage`로 건너뛰었다(binding 파일 7개가 "not instrumented"로 남음). cpu2와 host 도구는 손대지 않았다.
- 크기(R2): `core-cpu1` text 519,663 → 545,636 B, bss 1,249,200 → 5,541,824 B; `CFE_EVS_SendEventWithAppID` 103 → 134 명령, `CFE_ES_GetAppID` 34 → 57.
- 크기(C1): `core-tx` text 417,562 → 442,590 B(런타임 포함), bss 1,245,440 → 5,538,064 B(런타임 정적 버퍼: actor 64 × 64 KiB + 부트스트랩 64 KiB, B1 상수), `cf.so` text +703 B, `bp.so` +807 B. 계측 함수의 명령 수: `CF_CFDP_InitEngine` 251 → 290, `BP_FlowInit` 93 → 158, `OS_TaskCreate` 122 → 156. 꼬리 점프 목록은 바뀌지 않았다.
- 실행 비용(hook 비용, 기록 비용)은 재지 않았다(G-I0/G-I 비교 미실시) [U].

## 4. site manifest와 binding 결과 (IR §9.3, §15 단계 C 산출물)

논리 site는 F0 계획(`profiles/c1-bp-b051eb0/observe/sites-cf833fdbb.kzp`, `profiles/cfs-v7.0.1/observe/sites.kzp`)의 의미와 인용을 옮긴 것이다(`sites/*.kzs`, 줄마다 `ref=`). binding 파일은 plugin이 단위마다 쓴 것을 `scripts/merge_binds.sh`로 합쳤다(저장소 밖 `c1/binds_instr.kzs`, `r2/binds_instr.kzs`).

| 사례 | 논리 site → binding | 상태 | 비고 |
|---|---|---|---|
| C1 core | es.start_task.entry/ret, es.child_task.entry/ret, osal.task_create.entry/ret, osal.task_entry, osal.task_create.id, es.exit_app.entry (9) | 모두 `bound` | 반환 site는 GCC가 합친 반환 하나(`osapi-task.c:0`: 위치 없는 합쳐진 return) |
| C1 cf.so | cf_lookup_call.before(+control), cf_lookup_call, cf_retry_delay (3) | `bound` | 이름 슬롯 타입 확인 `char[20]` |
| C1 bp.so | bp_existence_probe_call(.before), bp_create_call(.before, +control) (4) | **`inlined`** → `BP_FlowInit` | 원 함수 `initialize_throttling`(static, 한 번 호출)이 `-O3`에서 사라졌다. F0 계획이 `func=BP_FlowInit`으로 닻을 둔 것과 같은 결과 |
| R2 | SB 호출 3곳(id_valid@275/1183/786), sb_publication, CFE_ES_GetAppID.store(L754, L759 두 binding), evs.send.entry/ret, 정체성 8 (16) | 모두 `bound`(`-O0`) | `duplicated`·`removed`·`unsupported` 없음 |

- 독립 확인: 링크된 `core-tx`·`cf.so`·`bp.so`·`core-cpu1`에서 hook 호출을 objdump로 센 결과(`scripts/bincheck.sh`)가 binding 파일의 함수·개수와 모두 같다(binding마다 호출 1개). 실행에서도 binding마다 pc가 하나였다 [M].
- 지원 못한 지점: 이 manifest의 목표 지점에는 없다. 형식상 plugin이 `unsupported`로 남기는 경우는 블록을 끝내는 호출(noreturn 등) 뒤, 슬롯 4개 초과, 8바이트 초과 집합체 값, 이름이 아닌 간접 호출(callee를 알 수 없음), 한 단위에서 한 site의 binding 16개 이상이다 [F: `kzgcc.cpp`].
- F0와 위치의 뜻이 다른 곳: F0의 `osal.countsem_lookup.entry/ret`·`osal.countsem_create.entry`는 **피호출 쪽**(모든 호출자), GCC manifest는 CF·BP **호출 쪽**이다. 그래서 F0에 있는 SCH_LAB의 세마포어 생성은 GCC 기록에 없다(목표 site 밖). OSAL 내부 점(`OS_CountSemGetIdByName.exists`의 꼬리 점프, `.found`의 token 읽기)은 이 manifest에 넣지 않았다. GCC에서는 원 소스의 호출(`OS_ObjectIdFindByName` 호출)과 포인터 매개변수 store(`*object_id = token.obj_id`, `osapi-idmap.c:978`)로 묶을 수 있다 [I, 실행 안 함].

## 5. pilot 부팅과 F0 대조 (IR §13.2 순서)

### 5.1 C1 tx (post 833fdbb, 1df262a) — `scripts/run_c1.sh`, 대조 `scripts/compare_c1.sh`

부팅: `observe_c1_smoke.sh --instances`의 절차를 관측기 없이(tar/untar, `ubuntu`로 tx·rx, sleep 5, OPERATIONAL 확인, ES POWERON, sleep 5). tx·rx 모두 5초 안에 OPERATIONAL, 정지 `ci-es-restart-poweron`, 종료 코드 0/0. tx 환경 += `KZ_RT_DUMP`, `KZ_RT_RUN_ID=1`, `KZ_RT_INSTANCE=1`; rx += `KZ_RT_MODE=off`, `KZ_RT_INSTANCE=2`(rx는 F0처럼 관측하지 않음). 대조 기록: `c1_runs/smoke3/post_1df262a`(kz-dump 텍스트). 산출물: `/home/user/work/instr/gcc/c1/{run1,cmp1}` [M].

1. **위치 대응**: §4. 정체성 9 site는 F0와 같은 label·같은 함수 진입/반환. CF·BP의 상태·id는 F0의 호출 쪽 점(`cf_lookup_call`, `bp_*_call`)과 같은 곳이고, 이름·out id는 F0가 피호출 쪽에서 읽던 값을 호출 쪽에서 읽는다.
2. **도달**: 횟수가 F0와 같다 — start_task 15/15, child_task 5/5, task_create 15/15, task_entry 15/15, task_create.id 17/17, exit_app 0/0(post CF는 종료하지 않음), CF 조회 3/3, 재시도 지연 1/1, BP 확인 2/2, BP 생성 2/2. (F0의 피호출 쪽 조회 입구 5 = CF 3 + BP 2, 생성 입구 3 = BP 2 + SCH_LAB 1.) 같은 횟수를 합격 조건으로 쓰지는 않는다.
3. **포획 적합성**: CF `BP0_tsem` −17 → `BP0_tsem` 0, id `0x30002` → `BP1_tsem` 0, id `0x30003`; BP `BP0_tsem` −17 → 생성 0, `0x30002`; `BP1_tsem` −17 → 생성 0, `0x30003`; 생성 초기값 10. 정규화한 줄의 diff 0(`cmp1/sem.diff`). 실패한 조회 뒤의 out 값은 0이다: OSAL은 실패 시 out 인자를 쓰지 않으므로(`osapi-idmap.c:962-984`) 이것은 호출자 변수의 이전 내용이며, 상태가 0일 때만 id로 읽는다 [F][M].
4. **식별 적합성**: OSAL task id 17개 각각의 소유 앱 id(`0x110001`…`0x11000a`)와 생성 task가 F0와 같다(diff 0, `cmp1/identity.diff`). 예: CF = task `0x1000c`, owner `0x110009`; BP = `0x1000d`, `0x11000a`; BP_CLA_OUT/IN은 BP가 만든 child, owner `0x11000a`. F0처럼 커널 thread 이름을 쓰지 않고 manifest 값만으로 결합했다.
5. **기록 완전성**: 손실 0, actor 16개 모두 aseq 빈틈 0, partial 0, `kz-rt-dump --manifest` 검사 통과. dump는 cFS의 정상 종료 경로(ES POWERON → `exit`)에서 시작 객체의 `atexit`로 써졌다(B1 열린 항목 "cFS 종료 경로의 자동 dump" 확인) [M].
6. **판정 근거**: CF·BP의 차이 없음 → 값(직접 포획)과 순서(같은 actor의 aseq, 같은 호출의 link)가 F0와 같은 근거를 준다. 서로 다른 actor 사이 순서는 런타임이 만들지 않는다(event_semantics.md: 런타임 순서는 HB 아님). 이 기록을 `kz replay`로 판정하려면 dump → `.kzt` 변환이 필요하다(B1 열린 항목, 미실시) [U].

### 5.2 R2 (cFE v7.0.1 ENV, native_std) — `scripts/run_r2.sh`, 대조 `scripts/compare_r2.sh`

부팅: `env/run_cfs.sh operational ci-es-restart-poweron`을 `CFS_DIR=<사본>`으로(관측기 없이, persist 스냅숏 전후). OPERATIONAL 1,660 ms, 정지 사유 `operational_seen`, 종료 0. 환경 += `KZ_RT_DUMP`, `KZ_RT_RUN_ID`, `KZ_RT_INSTANCE`(값은 run_cfs.sh 기록 규칙상 기록 안 됨). 대조 기록: `r2_runs/v153_pilot3`. 산출물: `/home/user/work/instr/gcc/r2/{run1,cmp1}` [M].

1. **위치 대응**: §4. SB 호출 3곳은 F0의 `at-call` 점과 같은 줄의 호출이고, 슬롯은 같은 문장에서 읽은 `CFE_SB_Global.AppId` 인자 operand다.
2. **도달**: start_task 24/24, child_task 4/4, task_create 24/24, task_entry 24/24, task_create.id 26/26, @275 26/26, @1183 111/111, @786 **74/65**, sb_publication 1/1, evs.send.entry **278/269**. 차이는 앱 task들의 AppID ≠ 0 전송(TO_LAB, DS, FM, HK, HS, MM, SC, MD, CS, SBN의 @786·일반 전송)이며 실행마다 달라지는 부분이다(CORD 실행에서도 258/240/254, `results/R2_cord_v701.md` §7). IR §13.2에 따라 차이로만 기록한다.
3. **포획 적합성**: SB 호출 3곳에서 AppID 0이 넘어간 전송이 **24건 = CFE_ES 20 + CFE_EVS 4**, 호출 지점별로도 같다(ES: @786 17, @1183 2, @275 1; EVS: 각 1, 2, 1). F0와 같다.
4. **식별 적합성**: task id 26개의 소유 앱·생성 task가 F0와 같다(diff 0). SB 공개: `sb_publication` 상태 0(F0 0)이고, F0가 읽지 않던 **저장된 값**을 `CFE_ES_GetAppID` 안의 store(L754)에서 `0x110003`으로 얻었으며 호출 직후 out 값도 같다. 이 값은 SB task(`0x10004`)의 생성 쪽 소유 앱 id(`0x110003`)와 같다 — 생성 쪽 근거와 공개 값이 서로 독립적으로 맞는다 [M].
5. **기록 완전성**: 손실 0, actor 24개 빈틈 0, 검사 통과, 사건 2,441개. `CFE_ES_GetAppID.store`는 모든 호출자의 store라 1,519건(SBN 544, TO_LAB 316)이다. 이번에는 actor 버퍼(64 KiB)가 넘치지 않았으나, 피호출 쪽 store를 긴 실행에 두면 넘칠 수 있다(넘치면 `overflow`로 셈) [M][I].
6. **판정 근거**: AppID 0 24건은 직접 포획 값이고 F0와 같다. 앱 전송 수의 차이는 상태·순서 근거가 아니라 실행 경로의 차이다.

## 6. IR §15.1의 일곱 질문

| # | 질문 | 답과 근거 |
|---|---|---|
| 1 | 현재 공개 cFS 구성과 필요한 target을 빌드하는가 | **예(호스트 x86-64 Linux, gcc 13.3.0)**. C1 Release(`-O3`, PIE + `.so` 적재 모듈)와 R2 debug(`-O0 -g`, 단위 시험 포함)를 기준 명령으로 빌드했고 경고 0, 바뀐 객체는 계측 단위뿐이다(§3) [M]. 다른 target(RTEMS·VxWorks 교차 GCC)은 plugin 지원(`--enable-plugin`, 같은 판의 plugin 헤더)이 있어야 하며 확인하지 않았다 [U]. |
| 2 | 실제 값·반환·공개 지점을 부작용 없이 전달하는가 | **예, 범위 안에서**. hook 인자는 원 문장의 GIMPLE operand이고, 재평가·store 변경·값 교체가 없다. 결과를 버리던 호출에 결과 이름을 붙이는 것과 hook 호출 추가가 코드에 생기는 변화다(명령 수 증가, §3.3). C1·R2의 이름·반환·id·AppID·저장 값이 F0와 같다(§5). out 값은 호출 직후 같은 스레드의 읽기이며 실패 시에는 이전 내용이다(§5.1). 부작용 없음을 실행 비용 면에서 재지는 않았다 [U]. |
| 3 | source 위치와 실행 지점의 대응을 설명하는가 | **예**. binding 파일(원 줄·원 함수 → 최종 함수·개수), 링크된 파일의 hook 호출 검사, 실행 시 pc가 서로 맞았다. inlining(BP)이 그대로 보고됐다. 한계: 합쳐진 return에는 줄이 없고(`:0`), RTL 단계의 복제는 plugin이 아니라 링크 뒤 검사가 본다 [M]. |
| 4 | 초기화 전 관측을 지원하는가 | **예(런타임이 제공)**. hook은 OSAL·cFE에 의존하지 않으므로 OSAL 초기화 중의 task 생성(timebase 등 `0xc0001`, `0x80001`, OS task 전)도 기록됐고, 시작 객체보다 먼저 도는 모듈 생성자의 등록은 attach 전 부트스트랩 경로로 들어갔다(`module 1 actor=?`) [M]. 생성자가 없는 RTOS에서는 하네스가 init/attach를 불러야 한다(B1 열린 항목) [U]. |
| 5 | 필요한 미계측 경로를 확인할 수 있는가 | **예, 컴파일 단위 수준까지**. 연결되지 않은 site는 `unsupported`(이유)로, 최적화로 사라진 hook은 `removed`로, 계측하지 않은 coverage 단위는 목록으로 남는다(§2.2, §4). 호출 쪽 site는 다른 호출자를 보지 않음을 manifest가 명시한다(SCH_LAB 예). plugin 없이 미리 빌드된 라이브러리(예: `/usr/local`의 tinycbor)와 간접 호출은 보이지 않으므로 피호출 쪽 binding이 필요하다 [F][M]. |
| 6 | 같은 runtime에 연결되는가 | **예**. B1 런타임(hook ABI 1, POSIX adapter, 시작 객체)을 수정 없이 링크했고, dump는 `kz-rt-dump --manifest` 검사를 통과했다. `.so` 모듈은 실행 파일이 내보낸 기호로 hook을 부른다(`nm -D`: `cf.so`·`bp.so`는 `U kz_rt_obs`, `core-tx`는 `T`) [M]. |
| 7 | 연구를 지연시키는 추가 frontend·ABI·모델 작업 | **작다**: frontend 작업 없음(GCC가 그대로 빌드), ABI 모델 없음(값은 operand), 새 형식 없음(`kzs-sites 1`). 대신 GCC 판마다 plugin을 다시 빌드·확인해야 하고(내부 API), 교차 툴체인마다 plugin 헤더가 필요하다. 시제품 크기: plugin 850줄(C++), 스크립트 8개, 시험 6개 [M]. |

IR §15.1의 규칙("GCC plugin이 요구를 충족하면 주 구현으로 진행")에 비추면 이 후보는 C1·R2의 목표 지점에서 요구를 충족했다. 다만 주 구현의 선택은 세 후보의 같은 대조가 끝난 뒤 PI가 한다(PD26 §2.3) [D].

## 7. 노력과 위험

| 위험 | 내용 | 대응·상태 |
|---|---|---|
| GCC 판 고정 | 내부 API·pass 이름(`ssa`, `optimized`, `*free_lang_data`)이 판마다 바뀔 수 있다. 판 검사로 다른 GCC에서는 적재를 거부한다 | 판마다 단위 시험 6개로 확인 [D] |
| 교차 툴체인 | RTEMS(RSB)·VxWorks GCC의 plugin 지원과 헤더 배포는 미확인 | 두 번째 OS 단계(IR §15 H)에서 확인 [U] |
| 라이선스 | GCC는 `plugin_is_GPL_compatible`을 선언한 plugin만 적재한다. 저장소에는 LICENSE가 없다 | PI 결정 필요(§9) [U] |
| `-g` 빌드 | DWARF producer 문자열에 plugin 옵션이 들어가 모든 단위의 디버그 정보가 다르다. 코드·데이터는 같다 | 차이로 기록. `-gno-record-gcc-switches`는 기준 플래그를 바꾸므로 쓰지 않음 [D] |
| LTO | 비-fat LTO에서는 plugin의 확인이 컴파일 단계에서 끝나지 않는다(계측 자체는 남음) | `check=deferred-to-lto-link` 표시; lto1 단계 확인은 미구현 [U] |
| 최적화 교란 | hook이 매개변수·결과를 사용하므로 IPA 결정(매개변수 제거, 분할, store 합치기)과 레지스터 배치가 달라질 수 있다 | 객체·명령 수 차이로만 확인, 실행 비용 미측정 [U] |
| 정적 메모리 | 런타임 bss 약 4.3 MB(B1 상수) | RTOS에서는 `-D`로 줄여야 함(B1 문서 §8.7) [I] |
| 피호출 쪽 store | 공용 함수의 store(`CFE_ES_GetAppID`)는 모든 호출자를 기록한다(1,519건) | 버퍼 넘침은 `overflow`로 셈; 필요한 호출자만 호출 쪽 site로 [I] |
| cFE 빌드 주입 | arch 빌드 cache를 configure 뒤에 다시 설정하는 방식은 cFE CMake 구조에 기댄다 | 소스·설정은 바뀌지 않음; 다른 빌드 체계에서는 다시 정해야 함 [I] |

## 8. B1 런타임에 필요한 변경 (이번에는 고치지 않음)

1. `kz-rt-dump`가 EVENT의 `pc`와 ACTOR의 native key를 출력하지 않는다 → 이번에는 `tools/kzgcc_events.c`(B1 decoder 사용)로 대신했다. dump 도구에 넣는 것이 맞다.
2. 시작 객체 `kz_rt_posix_auto.c`가 RUN 레코드의 `backend`·`compiler`·`target`·`manifest_sha256`을 채우지 않는다(dump에 빈 값). 연결부가 링크하는 상수(예: 약한 기호)나 환경 이름으로 채울 길이 필요하다.
3. `kz_rt_module`은 호출마다 MODULE 항목을 하나 쓴다(중복 제거 없음, 최대 32). 단위마다 생성자를 두는 이 연결부에서는 C1 tx가 6번(core 4, cf.so 1, bp.so 1), R2가 8번 등록했다. 계측 단위가 많아지면 `module-table` 손실이 난다 → (module, sha) 중복 제거 권고.
4. binding id 권고 `(module << 16) | k`를 이 연결부는 `(module << 16) | (site << 4) | n`으로 썼다(site < 4096, 단위당 binding 15개). 세 후보가 같은 규칙을 쓰려면 event_semantics.md §3에 적을 필요가 있다.

## 9. 열린 항목과 PI 결정

- **PI 결정 필요(새)**: `kzgcc.cpp`의 배포 라이선스. GCC는 GPL 호환을 선언한 plugin만 적재하므로 시제품은 그 선언을 담고 있다. 저장소에는 LICENSE가 없고, 어느 레퍼런스도 이 연구 도구의 라이선스를 정하지 않는다(법적·배포 선택). 시제품을 외부에 배포하기 전에 정해야 한다.
- 미실시: G-I0/G-I(같은 바이너리의 mode off/record) 비교와 hook 비용, 제어 hook의 실제 보류(이번 manifest의 `control` 두 곳은 RECORD 모드라 평가되지 않음), dump → `.kzt` 변환과 `kz replay` 판정, OSAL 내부 점의 binding, 교차 툴체인, lto1 단계 확인.
- 세 후보 비교: Clang 소스 변환 → GCC, LLVM 전용 pass의 같은 대조 결과와 함께 IR §15.1의 세 줄로 고른다(PD26 §2.3).

## 10. 재현

```sh
# plugin과 단위 시험 (gcc-13-plugin-dev 필요)
cmake -S kairos/instrument/gcc -B /home/user/work/instr/gcc/plugin-build && cmake --build /home/user/work/instr/gcc/plugin-build && ctest --test-dir /home/user/work/instr/gcc/plugin-build
# 대상용 런타임 (B1 문서 그대로)
cmake -S kairos/runtime -B /home/user/work/instr/gcc/runtime-build && cmake --build /home/user/work/instr/gcc/runtime-build
G=kairos/instrument/gcc/scripts; P=/home/user/work/instr/gcc/plugin-build/kzgcc.so; RT=/home/user/work/instr/gcc/runtime-build
# C1: 사본, 같은 경로의 기준 빌드(비교용), 계측 빌드, binding 합치기, 부팅, 대조
$G/build_c1.sh prepare /home/user/work/instr/gcc/c1
$G/build_c1.sh build /home/user/work/instr/gcc/c1 ref      # 객체 해시를 남긴 뒤 ws/build, ws/exe를 지운다
$G/build_c1.sh build /home/user/work/instr/gcc/c1 instr $P $RT
$G/merge_binds.sh /home/user/work/instr/gcc/c1/binds_instr /home/user/work/instr/gcc/c1/binds_instr.kzs
$G/run_c1.sh /home/user/work/instr/gcc/c1 /home/user/work/instr/gcc/c1/run1 $RT/kz-rt-dump /home/user/work/instr/gcc/plugin-build/kzgcc-events
$G/compare_c1.sh <kz-dump of c1_runs/smoke3/post_1df262a/run.kzt> /home/user/work/instr/gcc/c1/run1/tx.events.tsv /home/user/work/instr/gcc/c1/cmp1
# R2: 같은 순서 (build_r2.sh, run_r2.sh는 CFS_NORMAL_USER=ubuntu, root로)
```

산출물 해시(저장소 밖): C1 계측 `core-tx` `21f82eca…`, `cf.so` `c99e8300…`, `bp.so` `6e83e6b7…`, dump `61586c46…`; R2 계측 `core-cpu1` `967a1f00…`, dump `0af58a06…`; plugin `4bfcb12b…`; manifest C1 `8dbdd2e8…`, R2 `a4a92b90…`. 큰 빌드 트리는 기록을 남긴 뒤 지웠다.

## 11. 단계 D2: site 형태, F0 동등 manifest, 파생 빌드 다섯 (2026-10-26)

- 범위: 선택 문서(`plan/instrumentation_selection_20261026.md`) §4의 2·3·7, §5.2의 D2·D3·D6. 부팅은 하지 않았다(D3 몫). 연구 조건을 새로 정하지 않았다. 파생 실행의 시작 상태는 PI 결정 대기 그대로다(선택 문서 §7의 2).
- **plugin 0.2** (`kzgcc.cpp`) [D][M]
  - 판 2 `place=at-call|after-call|line-start|store`와 `from=param<k>`를 더했다(형식: `event_semantics.md` §3).
  - `store`는 그 줄의 store 문장을 무엇을 저장하든 묶는다(지역 포인터를 통한 필드, 전역 객체의 필드, 포인터 대상). `operand1`은 주소 계산이고 다시 읽지 않는다. bit-field와 주소를 잡지 않은 지역 객체는 그 값이 `unsupported`다.
  - `after-call`은 `from=ret`일 때만 결과에 SSA 이름을 붙인다(0.1은 결과가 있는 모든 호출에 붙였다).
  - binding id 범위(site ≤ 4095, module ≤ 32767)를 적재 때 검사한다.
  - `memset`은 삽입 시점(`ssa` 직후)에 `-O0/-O2/-O3` 모두 호출로 남는다 [M: 시험]. 선택 문서 §4의 3에서 [U]였던 항목이다.
  - 시험 9개 통과: 기존 6개, `kzgcc_shapes_{O0,O2,O3}`(필드·전역 store의 값·주소, 한 memset 뒤의 두 site, 매개변수 문자열, after-call 결과, line-start 순서, place 없는 점의 빈틈). 단계 C manifest(판 1)도 0.2로 같은 binding을 낸다(기존 시험).
- **manifest** (`profiles/c1-bp-b051eb0/instr/sites-cf{833fdbb,281a941}.kzs`, `profiles/cfs-v7.0.1/instr/sites.kzs`, 판 2, 정체성 읽기 포함) [F][D]
  - F0 계획의 site 34·33·35개가 **모두** 같은 label의 짝을 가진다. 짝마다 `equiv … backends=f0,gcc-plugin status=unconfirmed`를 두고, F0 위치와의 관계를 `ref`에 적었다. `unsupported`는 0이다. GCC 전용 site는 C1의 `.before` 셋(호출 쪽 이름, F 단계 gate 둘)이다.
  - 위치는 프로필의 LP 문장을 따른다. F0와 위치가 다른 곳은 다음과 같다.
    - `clear_table`·`clear_names`: F0는 `OS_API_Init` 안 호출의 반환, GCC는 그 안의 memset 뒤.
    - `request_shutdown`: F0는 L94 `pthread_kill` 반환, GCC는 L93 store 뒤.
    - `teardown`: F0는 `OS_TaskDelay` 반환, GCC는 `OS_DeleteAllObjects` 호출 앞.
    - R2 `register`·`free_slot`·`raise_state`·`core_clear`: F0는 줄 시작이나 호출 앞, GCC는 호출·store 뒤.
    - `.exists`·`.publish`·`.created`: F0는 꼬리 점프로 닿는 진입, GCC는 호출 앞.
  - 슬롯이 다른 곳은 다음과 같다.
    - F0 `return_address`는 모두 없다. 진입·반환은 `link`로 묶는다.
    - 진입의 세마포어 이름은 검증 전 포인터라 포인터만 둔다. 텍스트는 검증 뒤의 `.exists`와 `.publish`(`param1`)에서 읽는다.
    - 새로 얻는 값: `raise_state`의 `required`, `publish_core`의 `pending_id`, `register`의 `app_id`, `sb_publication`의 `out_app_id`.
  - 범위가 넓은 곳: `.found`(`OS_ObjectIdFindByName`의 store)와 `.created`(`OS_ObjectIdFinalizeNew`의 `OS_NotifyEvent` 호출)는 모든 객체 종류에서 기록된다. 재생은 열린 조회·생성 호출 안의 것만 쓴다(`schema.md` §12.4의 out 값 규칙) [I, 실제 dump로는 미확인].
  - 단계 C manifest와 다른 점: 호출 쪽 site는 호출/반환이 아니라 점이다. 그래서 같은 함수의 진입 site와 함께 있어도 한 호출을 두 번 세지 않는다. `CFE_ES_GetAppID.store`는 뺐다(§4의 6). 단계 C 파일(`sites/*.kzs`)은 그 dump의 근거로 그대로 둔다.
- **파생 빌드** (`scripts/derive_c1.sh`, `derive_r2.sh`, `build_runtime.sh`, `hookcheck.sh`). 같은 경로의 기준 빌드와 객체를 바이트로 비교했다 [M]

| 구성 | 객체 | 달라진 객체 | binding | 링크 뒤 hook(obs+ctl) | hookcheck |
|---|---|---|---|---|---|
| C1 post 1df262a | 261 | 계측 단위 11 | 37 (bound 33, inlined 4) | tx 39 = rx 39 | ok 37 |
| C1 post 1b0b338 | 261 | 계측 단위 11 | 37 (33, 4) | 39 = 39 | ok 37 |
| C1 pre 1df262a | 261 | 계측 단위 11 + `cfe_build_env_table.c.o` 2 (D4) | 36 (32, 4) | 38 = 38 | ok 36 |
| C1 pre 1b0b338 | 261 | 계측 단위 11 | 36 (32, 4) | 38 = 38 | ok 36 |
| R2 | 1,335 (cpu1) | 원시 바이트는 전부(`-g` producer). strip-debug 뒤 계측 단위 11 + D4 1 + coverage 267(빌드마다 다름, 단계 C와 같은 수) | 35 (bound 35) | 35 | ok 35 |

  - 차이 대장은 단계 C의 D1–D4와 같다. D3에 build identity 객체 `kz_rt_build_info.c.o`를 더했다(backend `kzgcc 0.2 (gcc-plugin)`, compiler·target·manifest sha256, D1-runtime의 경로).
  - C1 core의 module은 번들마다 다르다(1df262a = 1, 1b0b338 = 4). 그래서 두 번들의 binding id가 섞이지 않는다.
  - 경고는 기준과 같다(C1의 DV-P1 `-Wrestrict` 하나, R2는 0).
  - 크기: C1 post 1df262a `core-tx` text 417,562 → 448,538 B, bss 1,245,440 → 5,547,824 B. R2 `core-cpu1` text 519,663 → 551,760 B, bss 1,249,200 → 5,551,584 B(런타임 pool 4 MiB).
  - 산출물(저장소 밖, `/home/user/work/instr/gcc/d2/`, `sha256.txt`): 계측 빌드 트리와 exe 다섯은 D3·B′(D5 규칙: 빌드마다 B′)를 위해 남겼다. 같은 경로의 기준 빌드 트리는 해시를 남긴 뒤 지웠다.
- 열린 것 [U]
  - 실제 dump에서 `kz-rt-dump --manifest`, `kz convert`, `kz replay`의 확인(D3/D7·D8).
  - `equiv` 확정(단계 E).
  - ~~함수 진입에 묶은 요청의 호출 함수(`request_func`)는 정해지지 않는다(F0 `return_address`의 짝이 없음).~~ → 2026-10-26: R2 manifest가 `CFE_ES_StartAppTask` 요청을 세 호출 쪽(`CFE_ES_CreateObjects`, `CFE_ES_AppCreate`, `CFE_ES_CreateChildTask`)의 call/return 짝 `es.start_task.entry@<caller>`/`.ret@<caller>`로 묶는다. 요청 함수는 site의 `func`다. 다시 빌드했고(`/home/user/work/instr/gcc/d4/r2`, binding 39/39 bound, hookcheck ok 39), 실행에서 core 여부가 F0와 같다(core 8, not core 16). 결과는 `plan/results_instr_pilot_20261026.md` §8에 있다 [M].
  - B′ 추출.
- 새 PI 결정: 없다. 위치·슬롯 선택은 프로필 LP와 원 코드 문장을 인용해 정한 도구 binding이다.

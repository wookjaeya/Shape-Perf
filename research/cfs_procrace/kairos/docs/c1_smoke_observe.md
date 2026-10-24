# C1 build-smoke 관측 (W3 빌드 4개, 2026-10-24)

- **지위: build-smoke/pilot.** 빌드마다 관측 1회. W4 실험이 아니고 본 실험 결과도 아니다. 빈도·재현성·관측 비용은 주장하지 않는다. 반복 수는 PI 결정(P8)이다.
- 입력: 프로필 `profiles/c1-bp-b051eb0/`(Φ), 관측 계획 `profiles/c1-bp-b051eb0/observe/sites-cf{281a941,833fdbb}.kzp`, W3 빌드(`plan/C1_build_manifest.md` §1–§7), 동결된 기대 결과표(`plan/expected_results_C1_R2.md`, 고치지 않았다).
- 표기: [M] 이번 실행에서 확인, [F] 고정 커밋의 소스, [I] 추론, [D] 결정.
- 산출물은 저장소 밖 `/home/user/work/procrace/c1_runs/smoke1/<cfg>_<cand>/`에 있다. 파일 목록과 SHA-256은 각 `driver.txt` 끝에 있다.

## 1. 프로필과 관측점

**Φ 프로필**(`kz-phi check` 8개 조합 오류 0, `verify-prov` 8개 조합 인용 59–65개 해시 일치, 고정 개정 4개 일치 [M]). 공통 디렉터리 하나에 변형 디렉터리를 하나씩 더한다:

| 변형 | 내용 | 비고 |
|---|---|---|
| `cf-281a941/`, `cf-833fdbb/` | CF 고정 개정과 CF 인용(같은 이름, 다른 줄) | 833fdbb만 재시도 상수 25·100(`cf_platform_cfg.h` L329–L349)과 `cf_retry_delay` 사이트 |
| `bundle-1df262a/`, `bundle-1b0b338/` | cFE·OSAL 고정 개정 | 인용한 cFE·OSAL 파일은 두 후보에서 바이트가 같다 [M: `cmp`]. P4 미결정이라 둘 다 둔다 |
| `rspec-281a941/` | `allowed unspecified_by_reference` → 실패 조회는 NOT_READY_UNJUDGED | rev 1.3 그대로. P3 결정 전 변경 없음 |
| `rspec-833fdbb/` | `retry_until_ready {OS_ERR_NAME_NOT_FOUND} bound CF_STARTUP_SEM_MAX_RETRIES`, caller 기록 `cf_cfdp.c` L993–L999(sha256 `1c487620…`) | G-REBIND 결론 1–3 그대로 |
| `rspec-*-altP3/` | **현행 아님.** `cfs_cf.dox` L212–L214를 종료를 받아들이는 caller 기록으로 묶은 P3 대안. 833fdbb에서는 G1과 QT-5 충돌(`conflicts_caller`) | 디렉터리를 바꿔 고를 때만 쓴다 |

- 자원 종류 `osal.countsem(name: Str)`: ABSENT → LIVE는 BP의 `OS_CountSemCreate`(`OS_ObjectIdFinalizeNew`, `osapi-countsem.c` L106). 이름은 빌드 값 `BP0_tsem`·`BP1_tsem`(CF 표 `.github/buildconfig/{tx,rx}/tables/cf_def_config.c` L40·L54, BP 기본값 `bp_semcfg.c` L43–L49). 닫힘 안의 앱 중 삭제하는 곳은 없고 `OS_DeleteAllObjects`(`osapi-common.c` L356–L380)는 종료 때만 돈다. RESERVED 창(A5)과 세대는 모형에 없다(`r_countsem_by_name`).
- **호출 문맥의 역할**(Φ에 `role` 선언을 새로 넣었다, `docs/phi_reference.md` §5.7a). CF 조회 = `dependent_use`(출처 `cfs_cf.dox` L208–L214, #184 본문, `cf_events.h` L148–L158). BP 조회 = `existence_probe`(출처 `bp_flow.c` L121–L144의 else-if 생성, `bp_semcfg.c` L33–L36). 앱 이름 예외가 아니다. 검사기는 문맥을 빼는 역할에 출처 기록을 요구한다.
- provider 초기화 실패(P-E)와 provider 없는 설정 이름(A4)은 판정 상태가 아니라 실행 분류로 따로 보고한다. 근거 기록은 `bp_app_setup_library`·`bp_app_init_before_flow`·`bp_app_main_exit`(P-E)와 표·기본값 기록(이름은 있고 provider가 만든다)이다.

**관측 계획**(파일 3개: `core-tx` 주 파일, `cf.so`, `bp.so`). 관측기에 여러 파일 계획을 넣었다(`docs/observer.md` §3.5). W3 빌드는 Release이고 DWARF가 없으므로, 선 사이트 대신 함수 안 호출 지점(`point func=F after-call=C`)을 썼다. 이 지점도 W2 규칙을 따른다(PLT 스텁 호출, 분기 합류 반환점 거부). 사이트 22개(281a941) / 23개(833fdbb)는 다음과 같다.

- `OS_CountSemGetIdByName`·`OS_CountSemCreate`: 진입(이름·id 포인터)과 복귀(상태). 이름 문자열과 id 값은 메모리에 있어 읽지 못한다.
- `CFE_ES_ExitApp` 진입(종료 상태).
- `CF_Init`·`CF_CFDP_InitEngine` 진입·복귀.
- 역할 사이트 `cf_lookup_call`: `CF_CFDP_InitEngine` 안 조회 직후, 상태.
- 833fdbb 전용 `cf_retry_delay`: 같은 함수 안 `OS_TaskDelay` 직후.
- `BP_FlowInit` 진입·복귀.
- 역할 사이트 `bp_existence_probe_call`과 `bp_create_call`: `initialize_throttling`은 static이고 기호가 없어 `BP_FlowInit` 코드 안에 있다 [F L121·L524, M `nm`].
- app_owner 생성 측 사이트 7개: v7.0.1 21–25를 다시 앵커했다. 이 OSAL에서는 `OS_Posix_InternalTaskCreate_Impl`의 `entry_arg`가 5번째 인자(`r8`)이다(`os-impl-tasks.c` L434–L435, L571–L590). v7.0.1에서는 6번째(`r9`)였다.

## 2. 명령 [M]

```sh
# 소스 보기(저장소 밖): 빌드한 트리에 대한 심볼릭 링크
R=/home/user/work/procrace/c1_runs/prov_roots; B=/home/user/work/procrace/c1_build
for cfg in pre post; do for cand in 1df262a 1b0b338; do d=$R/${cfg}_$cand; mkdir -p $d
  ln -sfn $B/$cfg/src/cf $d/cf; ln -sfn $B/$cfg/src/bp $d/bp; ln -sfn $B/bundle_$cand/cfe $d/cfe; ln -sfn $B/bundle_$cand/osal $d/osal; done; done
# 관측 (root, 빌드마다 1회)
kairos/probe/scripts/observe_c1_smoke.sh <pre|post> <1df262a|1b0b338> /home/user/work/kairos-build-c1/tools/kz/kz --out-dir /home/user/work/procrace/c1_runs/smoke1
```

스크립트 단계는 다음과 같다.

1. 압축·풀기·storage: `smoke_one.sh`, CI L133–L166.
2. `kz anchor --bin`: 파일 3개.
3. 조건 파일 작성.
4. `kz observe … --basis cf --basis bp -- observe_c1_smoke.sh --instances`: CI L168–L200을 `smoke_one.sh`대로 실행한다.
5. `kz hits`와 `kz replay` 두 번: 현행 프로필 + `--compare-narrowing`(P1 두 읽기), P3 대안.
6. 앵커 교차 확인(`objdump`).

`--const CFE_PLATFORM_ES_MAX_APPLICATIONS=32`: `bpxfer_defs/default_platform_cfg.h` L491을 네 빌드의 기록된 컴파일 명령으로 평가한 값이다 [M].

도구: 저장소 `03e7544`, 그리고 커밋 전 조건 파일 한 줄 수정(`source.diff`, sha256 `7befa7e9…`; 이 문서와 함께 커밋). `kz` sha256 `40dd02e0…`(빌드 `/home/user/work/kairos-build-c1`, RelWithDebInfo, gcc 13.3). 계획 sha256: 281a941 `6c5d5371…`, 833fdbb `5fda968a…`. 프로필 해시(공통+cf+bundle+rspec): 281a941/A `467f94f2…`, /B `ee4ea11e…`, 833fdbb/A `808df01e…`, /B `67a7ebb0…`.

| 빌드 | `core-tx` / `cf.so` / `bp.so` sha256(앞 16) | run.kzt artifact | 시작(UTC) |
|---|---|---|---|
| pre/1df262a | `f0b1c0ad7f8076b2` / `1d27f27f2554dfed` / `334b073b63dee85e` | `11706606400dc52b…` | 18:04:41 |
| post/1df262a | `47436ed429965ff8` / `9423ab04489bbdf4` / 같음 | `f83ff972bb067913…` | 18:05:28 |
| pre/1b0b338 | `249088e0fbcb0e8a` / `1d27f27f2554dfed` / 같음 | `7ad6bf4aa61583aa…` | 18:05:50 |
| post/1b0b338 | `c5ebf628e7e257e6` / `9423ab04489bbdf4` / 같음 | `7753cca1a92e6dd8…` | 18:06:12 |

관측한 파일(압축을 푼 사본)과 빌드 산출물의 해시는 같다. 이 값은 W3 manifest §5의 값과 같다 [M].

## 3. W3 build-smoke·관측기 절차와 다른 점

1. **관측기 추가.** W3 smoke에는 관측기가 없었다. 이번에는 uprobe 23개 안팎 × CPU 4개를 걸었고 프로세스는 멈추지 않는다. 섭동은 재지 않았다.
2. **순서.** 압축 풀기 → `kz anchor`(파일을 읽는다) → `kz observe`가 시작·정지 단계를 자식으로 실행한다. uprobe는 실행될 inode에 미리 등록해야 한다. 인스턴스는 `kz observe` → bash → `setpriv`의 손자 프로세스이다.
3. **tx만 관측.** P7이 결정되지 않았다. rx는 W3대로 함께 시작·정지하지만 별도 사본이라 프로브가 없다.
4. **선 표 없음.** 선 시작 사이트는 쓰지 못했다. 호출 지점은 함수 안 호출로 찾았다(`anchor_xcheck.txt`: 23개 모두 `objdump`의 호출 바로 뒤, "address not found" 0).
5. app_owner `spawn-id` 레지스터가 `r9`에서 `r8`로 바뀌었다(OSAL 판 차이, §1).
6. 층: 확립되지 않음(`none`). 출력 위치: 저장소 밖.
7. pre/1df262a의 첫 호출은 `kz observe`가 조건 파일 형식 오류(`avoidable` 값)로 거부했다(종료 코드 1). 프로브를 붙이기 전이고 cFS도 시작하지 않았다. 디렉터리는 `smoke1_refused_pre_1df262a`로 옮겨 두었다. 고친 뒤 실행한 것이 위의 1회이다.

## 4. 결과 [M]

네 실행 모두 다음과 같다.

- 손실: `PERF_RECORD_LOST` 0, throttle 0, 스트림 오류 0. 뜻은 "커널이 손실을 보고하지 않았다"이다.
- 호출 짝 실패 0. thread 18개.
- app_owner: **confirmed 15, inferred 0, undetermined 3**(주 thread와 프로브된 생성 호출 없이 생긴 thread 둘). `ids_matched` 15.
- tx·rx 둘 다 5 s 안에 OPERATIONAL. ES POWERON 정지, 종료 코드 0. 결과는 cFS 후보 A와 B가 같다.

| 빌드 | CF 조회(결과) | CF 지연 | CF 종료 | BP 조회 / 생성 | 기타 생성 |
|---|---|---|---|---|---|
| pre (A, B) | 1회: `-17`(`OS_ERR_NAME_NOT_FOUND`). `CF_CFDP_InitEngine`·`CF_Init` → `-17` | — | `CFE_ES_ExitApp(3)` = APP_ERROR | 2회 `-17` / 2회 `0` | SCH_LAB 1회 `0` |
| post (A, B) | 3회: `-17`, `0`, `0`. `CF_CFDP_InitEngine` → `0` | 1회 | 없음 | 2회 `-17` / 2회 `0` | SCH_LAB 1회 `0` |

**발생별 요약 뷰** (`kz replay`, 현행 프로필, P1 현행 = 반환 코드 고정 켬):

| 빌드 | 발생 | 상태 → 요약 | 문맥 |
|---|---|---|---|
| pre | (CF, 1) | NOT_READY_UNJUDGED → `INCONCLUSIVE(caller_requirement_absent)` {outcome-narrowed} | `cf-dependent-lookup` |
| post | (CF, 1) | PENDING → `INCONCLUSIVE(horizon-open)` {outcome-narrowed} | 같음 |
| post | (CF, 2), (CF, 3) | UNDETERMINED {READY, NOT_EVALUATED} → `INCONCLUSIVE(resource_or_lifetime_unmatched, effect_point_ambiguous, effect_unsupported, not_evaluated)` | 같음 |
| 전부 | BP 조회 2회 | 발생 아님(역할 사실 `bp-existence-probe … not an instance of K-REG.osal-countsem-lookup`, 결과 `-17`) | `bp-existence-probe` |

- 키(이름)가 기록되지 않아 발생의 `key []`이다. 그래서 실행 관계는 `ownership-undetermined`("the record does not fix the key")이다.
- 어느 채널(키)의 조회인지는 코드에서 추론한다 [I]. 채널 루프 L967, 표 L40·L54를 따르면 pre의 1회는 `BP0_tsem`이다. post는 `BP0_tsem` 2회(A2, m=2)와 `BP1_tsem` 1회(A1)이다. 기록이 정한 것은 아니다.
- 요약의 바탕:
  - post의 성공 조회 둘이 UNDETERMINED인 이유는 조건의 LP(조회 안의 `OS_ObjectIdFindByName`)를 관측하지 못해서다. 이 빌드에서 그 호출은 꼬리 점프(`jmp`)이다. 반환 코드 `ready`는 "도달한 조건 중 거짓 없음"만 말하므로 통과 여부가 열린다.
  - (CF, 1)이 PENDING인 이유는 `retry_until_ready`의 horizon monitor가 재생에 아직 없어서다(`phi_reference.md` §7.5).
- 관측 적합성: `osal.countsem`, `es.app` 모두 `open {resource_or_lifetime_unmatched, effect_unsupported}`이고, `start-state-known`, `loss-free`는 세워졌다.
- P1 대안(반환 코드 고정 끔): 발생 요약이 모두 바뀐다(pre 1, post 3). 모두 INCONCLUSIVE로 남고, 바뀌는 것은 사유이다.
- P3 대안(현행 아님):
  - pre (CF, 1)은 PENDING → `INCONCLUSIVE(horizon-open)`이다. 문서만 묶으면 종료 항목이 판정하지만, 그 monitor가 없다.
  - post (CF, 1)은 NOT_READY_UNJUDGED → `INCONCLUSIVE(caller_requirement_absent)`이다(충돌). 성공 둘은 현행과 같다.

## 5. 기대 결과표와 대조 (표는 고치지 않았다)

| 행 | 기대 | 이번 | 판단 |
|---|---|---|---|
| E-03 (pre A3) | (CF,1) NOT_READY_UNJUDGED → `INCONCLUSIVE(caller_requirement_absent)`, 위반 아님. 이벤트 30, 종료 | 같음. 종료 `CFE_ES_ExitApp(3)`, `CF_Init` `-17` | **일치** |
| E-09 (pre, 채널 1) | 도달하지 않음 → 발생 없음 | CF 조회 1회뿐 [I: 채널 0] | 일치 (키를 기록이 정하지 않는다는 단서) |
| E-11 (BP 정상 첫 기동) | 요구 없음 → 발생·요약 없음. `-17` 뒤 생성 | 역할 사실 2개, 발생 0. `-17` 뒤 생성 `0` 2회 | **일치** |
| E-05 (post A2, BP0_tsem) | (CF,1) TOLERATED, (CF,2) READY → `NO_VIOLATION_IN_CHECKED_PREFIX` | (CF,1) PENDING → `horizon-open`, (CF,2) UNDETERMINED → INCONCLUSIVE | **불일치**: horizon monitor 미구현, LP 통과 미관측(꼬리 점프), 이름 미판독 |
| E-04 (post A1, BP1_tsem) | (CF,1) READY → `NO_VIOLATION…`. 세대를 잇지 못하면 `INCONCLUSIVE(resource_or_lifetime_unmatched)` | UNDETERMINED → INCONCLUSIVE(… `resource_or_lifetime_unmatched` 포함) | 표의 "관측 부족" 열과 같은 갈래. READY는 얻지 못함 |
| E-06, E-10, E-07, E-12 | A3(post)·provider 초기화 실패·provider 없음·다른 생성자 | 나오지 않음. post 종료 없음. BP는 `BP_FlowInit`에 도달하고 생성 성공. P-E 없음 | 해당 없음 |

수정 전 실패가 공개 구성의 시작 순서(CF L6 다음 BP L7)만으로 매번 나는지는 이 4회로 말하지 않는다(W3 해석 주의와 같음). 실행 묶음의 병합 순서는 thread 사이 실행 순서가 아니다. post에서 BP 생성이 CF 둘째 조회보다 앞에 기록된 것은 한 시계의 증거일 뿐이다.

## 6. 관측 적합성과 D5 닫힘

결합 닫힘은 이미 있다(`66caa4e`, `plan/C1_build_manifest.md` §8.5): BP 두 필드와 OSAL 기록 필드는 writer-set-complete이다. 그러나 이번 기록은 writer·별칭 포괄과 수명을 세우지 않는다. 실행 컨테이너 어댑터가 닫힘 사실을 읽지 않기 때문이다(`docs/observer.md` §3.4). 그래서 `osal.countsem`은 열려 있다. 범위 밖이거나 관측되지 않은 것은 셋이다.

- CF 쪽 `CF_Channel_t.sem_id`·`CF_ChannelConfig_t.sem_name`: CF 자체 cast로 DISABLED이다.
- Φ 효과 지점 `OS_CountSemCreate.publish`(`FinalizeNew` 호출): 프로브가 없다.
- 이름 문자열(키)과 생성 id: 레지스터만 읽는 백엔드라 얻지 못한다.

## 7. 남은 것

- `retry_until_ready` horizon monitor(E-05의 TOLERATED에 필요).
- 꼬리 점프 지점 앵커(조회 LP 통과).
- 이름 판독: BP 이름은 파일 기반 데이터라 정적으로 풀 수 있다. CF 이름은 TBL 버퍼에 있어 메모리 읽기 백엔드(eBPF)가 필요하다.
- 생성 id 판독.
- rx 관측(P7).
- 281a941 P3 하위 대안(문서와 이슈를 함께 묶은 충돌)은 보고만 하고 부호화하지 않았다.
- PI 관련:
  - P1: post 성공 조회의 판정이 기댄다.
  - P3: 대안은 선택할 수 있다. 레퍼런스가 정하지 않는 이유는 글이 종료를 서술할 뿐 받아들인다고 하지 않고, 수정 전부터 있었으며 수정이 고치지 않았기 때문이다(`C1_basis_plan.md` §9.8 G5).
  - P4: 두 후보의 결과가 같다.
  - P7: tx만 관측했다.
  - P8: 반복 수는 PI가 정한다.

# 계측 계열 G-I pilot 실행과 F0 대조 — C1 네 빌드 × tx·rx, R2 (단계 D, 2026-10-26)

- 지위: **pilot/dev 실행**이다. 본 실험 결과가 아니다. 구성마다 인스턴스마다 부팅 1회이며 빈도·재현성은 주장하지 않는다(P8). 군 비교(단계 E: G-A·G-F0·G-I0·G-I)는 하지 않았다. 파생 실행의 시작 상태가 PI 결정 대기이기 때문이다(`plan/instrumentation_selection_20261026.md` §7 항목 2).
- 범위: 선택 문서 §5.2의 D7(G-I 실행)·D8(변환·재생·대조). IR(사용자 업로드 `Kairos_OS_Independent_Instrumentation_Review_20261026.md`) §13.2의 여섯 단위, §12.2의 여섯 원인, §13.4의 독립 근거. PD26(`Architecture/spec/PI_DECISIONS_RESOLVED_20261026.md`) §4.
- 표기: [F] 코드·문서에서 확인, [M] 실행·재생으로 확인, [I] 추론, [D] 설계 결정, [U] 미확인.
- 대조 기록(F0, 같은 구성): C1 tx = smoke3(`c1_runs/smoke3/<cfg>_<cand>`, `replay_i1025/`), C1 rx = confirm1 자연 rx(`c1_runs/confirm1/N_<cfg>_<cand>_rx`, `replay_c1025/`), R2 = `r2_runs/v153_pilot3`(`replay_i1025/`). 판정은 `plan/results_pilot_20261025.md` §1·§5.3의 값이다.
- 연구 조건은 새로 정하지 않았다(CONDITIONS_POLICY). 계측 빌드는 파생 구성이고 차이는 아래 §0과 각 빌드 기록(`build_instr.record`, D1–D4)에 있다.

## 0. 조건 [M]

| 항목 | 값 | 상태 |
|---|---|---|
| 빌드 | D2 파생 빌드(`/home/user/work/instr/gcc/d2/`, 커밋 `ca9917e`): C1 pre/post × `1df262a`/`1b0b338`, R2. plugin `kzgcc 0.2` sha256 `56a96b79…`, manifest `sites-cf833fdbb.kzs` `47bf57a3…`, `sites-cf281a941.kzs` `39301d34…`, R2 `sites.kzs` `d72eb214…`, 런타임 `kzrt 0.2.0` | deviation(파생 구성; D1 경로, D2 사본, D3 plugin 옵션·런타임 링크, D4 빌드 시각 문자열) |
| 모드 | 기록하는 인스턴스 `KZ_RT_MODE=record`(G-I). dump에 `window_mode=record mode_source=early`, gate 없음·계획 없음 | 도구 실행 매개변수 |
| C1 tx 절차 | `observe_c1_smoke.sh --instances`의 단계(CI L165–L200): tar/untar, tx·rx 동시 시작(ubuntu), 5 s, OPERATIONAL 확인, ES POWERON(UDP 1234/1235), 5 s. rx는 `KZ_RT_MODE=off`(F0 smoke3도 rx를 관측하지 않음). 스크립트 `kairos/instrument/gcc/scripts/run_gi_c1.sh … tx` | from-reference. rx가 파생 바이너리의 G-I0로 돈 것은 deviation |
| C1 rx 절차 | `observe_c1_hold.sh --one --no-hold`의 단계(F0 confirm1 자연 rx와 같음): rx만, 8 s, ES POWERON(UDP 1235), 5 s. `run_gi_c1.sh … rx` | from-reference(F0 rx 기록과 같은 절차) |
| R2 절차 | `env/run_cfs.sh operational ci-es-restart-poweron`, `CFS_DIR=<W>/cFS`, 전후 `persist_snapshot.sh`(observe_cfs_smoke.sh 1·5단계). `run_gi_r2.sh` | from-reference |
| 시작 상태(S0) | C1: 새 untar(EEPROM.DAT·키 파일 없음, ipcs 비어 있음; `persist_before_<inst>.txt`). 기준 exe 디렉터리에도 `cf`와 core 실행 파일뿐이라 F0 smoke3·confirm1도 같은 S0다 [F: 기준 디렉터리 목록]. R2: 파생 빌드의 새 설치(EEPROM.DAT·키 파일 없음). F0 pilot3는 0으로 찬 EEPROM.DAT(524,288 B)와 빈 키 파일 셋에서 시작했다(`v153_pilot3_persist_before.txt`) | C1: from-reference(절차가 정함). R2: unspecified_by_reference(PI 결정 대기, 기록만 함) |
| 환경 차이 | 인스턴스 환경 += `KZ_RT_MODE`·`KZ_RT_DUMP`·`KZ_RT_RUN_ID`·`KZ_RT_INSTANCE`(값은 기록하지 않음, cFS는 읽지 않음). 관측기 없음(uprobe·tracefs 없음). 시작 전 읽기에 sha256sum·persist_snapshot.sh가 더해짐(F0는 kz anchor) | deviation |
| 재생 | `kz` sha256 `4bacf805…`(이 커밋의 재생 규칙 포함, §5), 프로필·상수·읽기는 F0와 같다: C1 `--const CFE_PLATFORM_ES_MAX_APPLICATIONS=32 --const OS_MAX_API_NAME=20`, cur·p1off·rfoff·rfoff_p1off·idoff·cw_counted·cw_excluded; R2 `--const CFE_PLATFORM_ES_MAX_APPLICATIONS=32 --core-request-caller CFE_ES_CreateObjects`, cur·p1off·idoff·idoff_p1off·cw_counted·cw_excluded. 스크립트 `replay_gi.sh` | from-reference(F0 재생 인수) |
| B′ | 빌드·인스턴스마다 그 compile DB로 `kz-static basis`+`extract`(F0 닫힘과 같은 프로필·recipe·target·extract 인수; C1 rx는 `closure_rx.kzb`). 스크립트 `basis_gi.sh`. B′ basis가 변환 기록의 부모이자 재생의 `--basis`·`--static-basis`다(파생 바이너리 해시를 이름 붙임) | `static_facts.md` §7a의 결정 |
| 산출물 | `/home/user/work/instr/gcc/d3/`(저장소 밖, 50 MB, `sha256.txt`): `c1/<cfg>_<cand>_<inst>/{run.meta,conditions.txt,persist_*,dump/,<inst>.dump.txt,replay/}`, `r2/run1/`, `bprime/`, F0 재재생 `f0check/`. 규칙 수정 전 재생은 각 `replay_before_args/` | — |

실행 결과: 9회 모두 OPERATIONAL(C1 5 s 안, R2 1,650 ms; F0 pilot3 1,628 ms), 정지 `ci-es-restart-poweron`·`operational_seen`, 종료 코드 0, 생존 프로세스 없음. dump는 정상 종료 경로(atexit)에서 써졌다. 이벤트 콘솔: post 네 실행 "Created semaphore BP0/BP1" 뒤 "CF Initialized", pre 네 실행 "failed to get sem id for name BP0_tsem, error=-17" 뒤 CF 종료 — F0의 같은 구성과 같다 [M].

## 1. 판정 대조 요약 [M]

| 구성 · 인스턴스 | F0 판정(대조 기록) | G-I 판정 | 근거 종류 | 민감도 읽기 |
|---|---|---|---|---|
| C1 post × A·B, tx·rx (4) | (CF,BP0,1) TOLERATED → `NO_VIOLATION…` · ON; (CF,BP0,2) READY · SO+RD; (CF,BP1,1) READY · SO+RD (C1-N2–N4, §5.3) | **같음** | 같음 | 7 읽기의 발생 줄·요약 수 diff 0 |
| C1 pre × A·B, tx·rx (4) | (CF,BP0,1) NOT_READY_UNJUDGED → `INCONCLUSIVE(caller_requirement_absent)` · ON (C1-N1, §5.3) | **같음** | 같음 | 7 읽기 diff 0 |
| R2 (1) | ES 20 + EVS 4 = 24 `OBSERVED_VIOLATION` · DC; SB 5 `NO_VIOLATION…` · DC+SO(A-OWNER); 다른 task 240 `NO_VIOLATION…` · DC+ON (R2-1–R2-4) | 24 · 5 같음. 다른 task **260** · DC+ON | 같음 | P1 끔: 24 VIOLATION·5 NO_VIOLATION·**260** INCONCLUSIVE(F0 240). 관계 `core-application` → **`inter-application`**(§3 R-2) |

C1의 비교 방법: 발생마다 상태·결과 코드·좁힘·요약·근거·적합성·소유 앱·관계·감시 줄을 뽑아(사건 번호와 actor 이름 제외) F0 출력과 비교했다. 8개 인스턴스 × 7 읽기 모두 diff 0이고, `summary counts` 절도 같다. R2는 (키, 소유 앱, 요약, 근거)별 수를 비교했다.

## 2. IR §13.2 여섯 단위 [M]

### 2.1 위치 대응

- C1 manifest는 F0 계획 34(post)·33(pre) site의 같은 label 짝과 GCC 전용 `.before` 3개를 가진다(37·36 site). R2는 35 대 35다. 모든 짝이 `equiv … status=unconfirmed`이고 이번 실행으로 확정하지 않았다(확정은 단계 E).
- 재생에서 매핑되지 않은 site는 F0와 같은 정체성 전용 5개(`es.start_task.*`, `es.child_task.*`, `osal.task_create.id`)와 GCC 전용 `.before` 3개다. `.found`·`.created`는 "열린 호출의 out 값"으로 매핑된다.
- F0와 위치가 다른 점(D2 기록, `instr_candidate_gcc.md` §11)은 재생 결과를 바꾸지 않았다. 관찰된 영향은 R2 초기 스레드에서 `reset_records`와 `reset_states`의 기록 순서가 F0와 반대인 것 하나다(F0 줄 시작, GCC memset 뒤). 같은 actor 안의 순서이고, 둘 다 시작 상태 효과라 판정에 닿지 않았다.

### 2.2 도달

- C1: 공통 label 전부에서 사건 수가 F0와 같다. 예: task 생성 15(post·pre), 조회 입구 post 5·pre 3, 생성 입구 3, `es.exit_app.entry` pre 1·post 0. 다른 것은 셋이다. GCC 전용 `.before`(cf 3·1, bp 2·2)가 있고, `.created` 73·72(F0 3)와 `.found` 4·2(F0 2·0)는 모든 객체 종류의 store라서 수가 많다. 재생은 열린 countsem 호출 안의 것만 썼다: post 5, pre 3. 이로써 D2의 미확인 항목이 실제 dump에서 확인됐다.
- R2: 공통 label 전부 같다. 예외는 `evs.send` 289(F0 269), `slot_registered` 265(245), `@786` 85(65)다. task별로 보면 SBN(task `0x10017`)이 28(F0 7), TO_LAB(`0x1000b`)이 111(112)이고 나머지 task는 같다. `@275` 26, `@1183` 111, `raise_state` 8, `sb_publication` 1은 같다. IR §13.2대로 이 수를 합격 조건으로 쓰지 않는다.

### 2.3 포획 적합성

- task별 사건열을 F0와 비교했다. actor를 각자 알린 OSAL task id로 이름 붙였다. C1 8개 인스턴스는 사건 수(post 96, pre 88)와 값이 같다: 이름 `BP0_tsem`·`BP1_tsem`, 상태, sem id `0x30001`–`0x30003`, task id, 우선순위.
- C1의 차이 둘:
  - 반환 상태 표기: F0는 `rax`를 u32로 읽어 `0xffffffef`, GCC는 s32로 읽어 `-17`이다. 좁힘 줄은 같다.
  - `.publish` 점: GCC는 검증 뒤 이름을 가진다(F0는 생성 입구에서 이름을 가짐).
- R2의 차이:
  - 위반 24건의 반환 표기(u32 `0xc2000003` 대 s32 부호 확장)가 다르다. 판정 줄은 같다.
  - GCC가 새로 얻은 값: `raise_state`의 `required` 8건, `publish_core`의 `pending_id` 5건, `sb_publication`의 저장된 AppID(§4).
- 진입에서 검증 전 포인터를 읽지 않았다. 이름 텍스트는 `.exists`(OS_CHECK_POINTER 뒤 FindByName에 넘기는 곳)와 `.publish`에서만 읽었다(PD26 §4) [F: manifest].

### 2.4 식별 적합성

- 소유 앱: C1 15개, R2 24개가 id로 묶여 `confirmed`다. (task id, 소유 앱, 생성 task)가 F0와 같다(발생 줄의 owner 색인 diff 0).
- 차이 둘:
  1. **초기 스레드**: F0는 `core-tx`/`core-cpu1`을 프로필 `initial_thread not_app`(thread id = process id)로 표시한다. G-I는 `owner undetermined`다. 그래서 초기 스레드의 지우는 쓰기(`clear_table`·`clear_names`, EVS·SB EarlyInit)와 TEARDOWN 뒤 삭제는 관계 부분 줄에서 `core-application` 대신 `ownership-undetermined`로 나온다. 발생의 관계 요약과 판정은 바뀌지 않았다(따로 적힌 영향).
  2. **R2 core 여부**: F0는 생성 요청의 반환 주소와 basis 기호로 요청 함수(`CFE_ES_CreateObjects`/`CFE_ES_AppCreate`)를 정했다. GCC manifest는 요청을 `CFE_ES_StartAppTask` 진입에 묶어 호출 함수가 없다(D2 열린 항목 `request_func`). 그래서 core 여부가 25개 모두 "not established"이고, 관계가 `core-application`(F0 264) 대신 `inter-application`(284)이다. 소유 앱 id는 같다.

### 2.5 기록 완전성

- 9개 dump 모두 손실 0, actor별 aseq 빈틈 0, overflow·partial 0, pool 넘침 0이다.
  - C1: actor 16, chunk 19/1024, 최대 actor 수요 8,976 B.
  - R2: actor 25, chunk 41, 최대 actor 수요 36,432 B.
- `kz-rt-dump --manifest` 검사가 모두 통과했다(manifest sha256 일치, 모드 `early`).
- 변환 시작 근거는 `bootstrap-buffer`, 재생은 "record from the process start"다. 수명이 섰다: C1 `osal.countsem`, R2 `evs.reg`·`es.shutdown`.
- 정지는 ES POWERON → `exit`이고 잘림이 없다.

### 2.6 판정 근거

C1: 두 계열의 근거 종류가 발생마다 같다. 미준비·TOLERATED는 ON(P1)이고, READY는 SO+RD(RF 간선, 같은 id `0x30002`/`0x30003`)다. R2: 위반 24는 DC만, SB 5는 DC+SO(A-OWNER), 나머지는 DC+ON이다. 차이는 R2의 관계 종류(§2.4)와 다른 task 전송 수(§2.2)뿐이고 둘 다 근거 종류와 무관하다.

## 3. 차이와 원인 분류 (IR §12.2) [M][I]

| # | 차이 | 원인 분류 | 근거 |
|---|---|---|---|
| C-1 | (규칙 수정 전) C1 모든 발생이 `INCONCLUSIVE(effect_unsupported,horizon-open)`, 키 없음 | **관측점 의미**(2): 이름 텍스트를 함수 진입이 아니라 호출 안 LP에서 읽는다. 재생은 키를 호출 사건의 인수에서만 찾았다 | `replay_before_args/cur.txt` "the record carries no text of sem_name at the call". §5의 규칙 뒤 F0와 같음 [M] |
| C-2 | 반환 상태의 폭(u32 대 s32) | 관측점 의미(값의 형) — 판정 무관 | 좁힘·판정 줄 동일 [M] |
| C-3 | `.created`·`.found` 수가 많음 | 관측점 의미(피호출 쪽 store, 모든 객체 종류) — 열린 호출 규칙으로 걸러짐 | 사용 5·3, 판정 동일 [M] |
| C-4 | 초기 스레드 not_app 표시 없음 | **사건 누락**(1)에 가까움: 런타임이 process-initial 사실(pid)을 기록하지 않는다(D1 열린 항목) | 관계 부분 줄만 달라짐 [M] |
| R-1 | 다른 task 전송 260(F0 240): SBN +21, TO_LAB −1 | **실행 시간**(5) [I]. SBN·TO_LAB의 자기 루프 전송 수이고, 정지 시각(OPERATIONAL 1,650 대 1,628 ms)과 관측기 비용이 다르다. 사건 누락이나 경로 미실행의 표시는 없다(손실 0, 다른 task 수 동일). 앱 task의 전송 수는 이전에도 실행마다 달랐다(단계 C GCC 278 대 F0 269, CORD 258/240/254) | 원인 분리는 G-A/G-I0 대조(단계 E) 전에는 [I] |
| R-2 | 관계 `core-application` → `inter-application` | **관측점 의미**(2): 생성 요청을 함수 진입에 묶어 요청 함수가 없다(F0 `return_address` 슬롯의 짝 없음) | core 여부 25 미정 [M] |
| R-3 | 초기 스레드의 `reset_*` 순서 | 관측점 의미(위치: 줄 시작 대 memset 뒤) — 판정 무관 | [M] |

이 실행에서 "코드 생성"(3)이나 "logger가 만든 대기"(4)를 가리키는 증거는 없다. C1의 모든 task 사건열과 R2의 핵심 task 사건열이 F0와 같다. 런타임의 관측 hook은 lock을 잡지 않고 actor별 chunk에 기록하며, 대기하는 경로가 없다(`kairos/runtime/src/kz_rt.c` L5–L15, chunk 확보는 원자적 더하기 하나) [F]. 다만 G-A·G-I0 대조 없이 배제한 것은 아니다.

## 4. 독립 근거 (IR §13.4)와 반환 코드 좁힘 [M][I]

- **R2 SB가 저장한 AppID**:
  - 기록됨. `sb_publication`의 `out_app_id`(`cfe_sb_task.c:138` 호출 직후 `CFE_SB_Global.AppId`) = `0x110003`이고, SB task(`0x10004`)의 생성 쪽 소유 앱 id `0x110003`과 같다. F0에는 없는 값이다(`effect_unsupported`).
  - **재생은 이 값을 쓰지 않는다.** 프로필의 점 `sb_publication`은 값을 받지 않고, 위치 `CFE_SB_Global.AppId`의 효과 `CFE_ES_GetAppID.store_ok|store_fail`는 피호출 쪽 store다. 그런데 GCC manifest는 R2 `CFE_ES_GetAppID.store`를 뺐다(D2, 버퍼 범위). 그래서 provider 줄은 여전히 "the record does not establish the value stored"다.
  - 독립 근거는 기록에 있으나 판정에 들어가지 않았다. 판정에 넣으려면 호출 쪽 out 값을 그 효과로 읽는 프로필 대응이 필요하다(도구·프로필 작업).
- **C1 out id**:
  - `.found`는 FindByName의 store operand(`token.obj_id`)를 다시 읽지 않고 잡는다. `.created`는 `OS_NotifyEvent`의 id다. F0는 해제 시점에 token 메모리를 읽었다. 값은 같다(`0x30002`/`0x30003`).
  - RF 규칙(프로필 `reads_from`)은 여전히 "반환 코드 + 같은 id"에서 간선을 만든다. 그래서 READY는 RD에 기대고, RF·P1을 끄면 INCONCLUSIVE다(F0와 같음).
- **반환 코드 좁힘 없이 새로 서는 판정: 없다.**
  - C1 pre·post의 미준비·TOLERATED는 P1을 끄면 INCONCLUSIVE다. 생성 게시(BP)와 조회 LP(CF) 사이에 actor 간 순서 근거가 없기 때문이고, F0와 같다.
  - R2의 위반 24는 원래부터 DC만으로 선다(두 계열 모두 P1 무관). 260은 ON이다.

## 5. 이번에 바꾼 도구 [D][M]

- **재생 규칙(real_trace)**: 호출 안 선형화 점의 인수.
  - 연산의 conjunct·effect `lp at` 점 사건이 그 연산의 non-out 매개변수 값을 가질 때 적용한다. 같은 actor의 가장 안쪽 열린 호출이 그 연산이고 호출 사건에 그 인수가 없으면, 호출 사건이 그 값을 받는다. 호출 사건이 가진 값은 바꾸지 않는다.
  - 출력 머리 줄 `arguments from linearization points inside the call N`. post 8, pre 6이다.
  - 근거: 프로필의 점 이름 표가 이미 LP 점의 매개변수 슬롯을 `arg.NAME`으로 읽는다(`real_trace.cpp` point_names). 검증 뒤 읽은 값은 원 코드가 실제로 넘긴 그 매개변수다(`osapi-countsem.c:210–212`, manifest site 12 ref) [F].
  - 코드: `kairos/lib/trace/real_trace.cpp`, `include/kz/trace/real_trace.hpp`("arguments"). 문서: `kairos/docs/schema.md` §12.4.
- **F0 불변**: smoke3 4개, confirm1 자연 rx 4개, R2 pilot3의 재생 출력이 규칙 전후로 같다. 다른 것은 B2가 더한 `record layer` 머리 줄뿐이다(`f0check/`) [M].
- 시험: `test_neutral_kzrt`에 사례를 더했다(호출에 없는 인수를 받고, 호출이 가진 인수는 유지, 점이 없는 호출은 그대로; 59 checks). 기본 빌드 163개 중 162개 통과, LLVM 빌드 185개 중 184개 통과이고, 둘 다 tracefs 시험 1개는 건너뛰었다.
- 스크립트(`kairos/instrument/gcc/scripts/`): `run_gi_c1.sh`, `run_gi_r2.sh`, `basis_gi.sh`, `replay_gi.sh`. 단계 C의 `run_c1.sh`·`run_r2.sh`는 그 기록의 근거라 그대로 두었다.
- B′:
  - C1 8개와 R2 1개를 뽑았다. R2 B′는 처음이다(D1 열린 항목).
  - extract 요약(위치별 writer 수·판정, A-TYPE 줄, 손 입력 수)이 기준 닫힘(`c1prof_i1025[_rx]`, R2 `replay_i1025`)과 줄 단위로 같다.
  - 다른 것은 compile 명령이 없는 link 구성원 4개다(런타임 whole archive 2, 시작 객체, build identity 객체). 이 넷은 basis에 기록되고 facts에는 없다(`static_facts.md` §7a의 3).

## 6. 보여 주는 것과 보여 주지 않는 것

**보여 주는 것.**
1. 선택 문서 §5.2의 진행 근거가 이 범위에서 충족됐다. F0가 판정한 키에서 계측 기록의 재생이 근거 부족(`ownership-undetermined`, 정적 닫힘 없음, 효과점 없음)으로 INCONCLUSIVE가 되지 않는다. C1 8개 인스턴스의 판정·근거·민감도가 F0와 같다. R2의 위반 24(ES 20 + EVS 4)와 SB 5도 같다.
2. 이것을 OS 호출 경로·관측기 없이, 같은 소스의 파생 빌드에서 얻었다. 결과는 IR §12.1의 둘째 행 범위("계측한 빌드에서 확인", 경로 kzgcc 0.2 + kzrt 0.2.0 + 위 manifest, 계획 없음)다.
3. SPEC §5.12 / IR §16의 무효화 검사를 통과했다.
   - 미관측 actor를 AppID 0으로 채우지 않았다(초기 스레드 `undetermined`).
   - 다시 읽은 값이 없다(SSA·operand, 검증 뒤 텍스트).
   - 함수 반환을 공개로 보지 않았다(`.publish`는 `OS_ObjectIdFinalizeNew` 호출).
   - 병합 순서를 쓰지 않았다(재생 "the merged order is not used").
   - 다른 빌드의 원시 id를 섞지 않았다(빌드마다 manifest·binding·B′; 번들별 module 번호 1·4).

**보여 주지 않는 것.**
1. 군 비교와 계측 영향. G-A·G-I0 없이는 R2 전송 수 차이의 원인(실행 시간인지 관측기 비용인지)과 "원 실행에 영향이 없다"를 말할 수 없다(IR §12.1 다섯째 행).
2. `equiv` 확정. 판정이 같다고 site 의미가 같다는 뜻은 아니다. 두 계열이 같은 잘못된 대응을 공유할 수 있다(IR §12.1 분석).
3. 반환 코드와 독립인 판정(§4).
4. R2 시작 상태가 F0와 같다는 것. 다르고 기록만 했다.
5. 제어(G-I-H, 단계 F).

## 7. 남은 것과 PI 결정

- 도구(결정 아님):
  - 초기 스레드 표시: 런타임 RUN에 pid를 넣거나, adapter가 process-initial을 표시한다(C-4).
  - R2 생성 요청의 호출 쪽 binding: `CFE_ES_CreateObjects`·`CFE_ES_AppCreate` 안의 `CFE_ES_StartAppTask` 호출(R-2). manifest를 바꾸면 다시 빌드한다.
  - SB 저장 AppID를 위치 효과로 읽는 프로필 대응(§4).
  - C1 RF 간선을 저장된 id의 직접 포획에서 세울지 정하는 프로필 검토(§4).
  - `kz hits`의 중립 층 읽기(선택 문서 §4의 8).
- 단계 E 전에 정해야 하는 기존 대기: **파생 실행의 R2 시작 상태**(선택 문서 §7 항목 2). 이번 실행으로 범위가 좁혀졌다. C1은 기준 절차가 새 untar로 S0를 정하고, 기준 exe 디렉터리에도 상태 파일이 없어 F0와 같다. 그래서 C1에서는 이 결정이 필요 없다. R2만 다르다(F0 0으로 찬 EEPROM.DAT·빈 키 파일, G-I 없음).
- **새 PI 결정: 없음.**
  - §5의 재생 규칙은 도구 해석 규칙이다. 이미 LP 점의 매개변수 슬롯을 `arg.NAME`으로 읽는 대응을 호출 사건까지 잇는 것이고, F0 결과를 바꾸지 않는다. PI는 뒤집을 수 있다.
  - B′ 사용은 `static_facts.md` §7a의 결정을 적용한 것이다.
  - R2 시작 상태는 새 질문이 아니다. 레퍼런스(README L102–L114, run_cfs.sh, SPEC §1.7)는 S0를 기록하라고 할 뿐, 새 exe 디렉터리가 어느 Init에서 시작할지는 정하지 않는다.

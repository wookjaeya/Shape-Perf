## 9. 단계별 실행 계획

이 절은 §3–§8의 설계를 16주 일정으로 옮긴다. 순서는 원노트 §66의 우선순위를 따른다.

1. 고정과 인벤토리
2. Software Bus만 다루는 MVP(메시지 → handler → state → 사용)
3. startup 의미
4. 다중 stream 일관성
5. Table Services

단계마다 산출물, 종료 기준, go/no-go 관문을 둔다.

**[설계 제안]** 이 절의 일정, 산출물, 기준, 관문, 임계값은 모두 계획이다. 분석기 구현, 검출 결과, 검출률은 없다.

이 절의 **[실측]**은 두 종류다.

- 앞 절들이 인용한 probe 결과.
- 이 절을 쓰면서 이 컨테이너에서 측정한 세 가지: 디스크 여유, out-of-tree dialect build, dense 분석 골격 실행. 산출물은 `$N/09_roadmap/oot_smoke/`에 있다.

**표기.**

- 주는 W1–W16, 단계는 단계 0–5, 관문은 GO-1–GO-8, 위험은 RK-1–RK-15로 부른다.
- 다른 절의 ID는 그대로 쓴다.
  - §5.8의 lifting pass P0–P7.
  - §6의 처리 단계 S1–S9와 규칙 ID(HB-*, R-F*).
  - §8의 사례 SYN-*·INJ-*, 기준선 B0–B8, ablation AB1–AB10·I1–I7.
- 결함 범주 번호는 §1.2를 따른다. F7은 메시지 간 순서, F8은 재시작 창이다. 첫 prototype은 "단계 1(MVP)"로 부르며 §5.8의 pass P0(build·import)과 구별한다.
- GO-3의 측정 항목은 G3-a–G3-d로 부른다.
- 증거 수준 L0(후보), L1(모델상 가능), L2(재현)는 §8.1을 따른다. 판정 이름(GUARANTEED, GUARANTEED_UNDER(A′) 등)은 §6.7.1을 따른다.

### 9.0 계획의 전제

#### 9.0.1 고정 입력과 인력

| 항목 | 값 | 근거 |
| --- | --- | --- |
| 소스 | cFS bundle `5a9b075cd4c818ee8555f349a9f78ba5632d0384`와 그 submodule 전부 | **[실측]** `$N/probe/submodules.txt`: cFE `546a002515be5a1e3b66f9ae2c14f948d9cec76f` [S25], OSAL `dad0ee99`, PSP `53df1d54`, sample_app `199476a3`, hk `0dc16b7a`, to_lab `d27c6014`, sch_lab `607e2f90`, ci_lab `edb8ddf5` (`$N/probe/submodules.txt`) |
| OSAL·PSP 통일 | bundle 값 하나로 통일 | **[확인된 사실]** §3의 ES·TBL probe는 날짜로 고른 OSAL `5befd8e9`·PSP `36c24cb9`를 썼다 (§7.2). **[설계 제안]** 단계 2와 단계 4의 모델 적합성 시험을 bundle 값으로 다시 실행한다. |
| build 설정 | `native_std`, cpu1, `MISSIONCONFIG=sample`, `CFE_EDS_ENABLED=OFF` | **[확인된 사실]** `target-configs.mk` L52–L56 (§7.3). EDS build는 범위 밖이다 (§1.4). |
| 분석 도구 | clang·mlir-translate·mlir-opt·mlir-tblgen @ llvm-project `1053047a` (읽기 전용) | **[실측]** §7.1 |
| 인력 | 연구자 1명이 구현·평가를 맡는다. 독립 작성자 1명이 정답·계약을 동결한다(주당 약 1일). | **[설계 제안]** §8.11은 결함 주입·label을 분석기 작성자와 분리하라고 요구한다. |
| 실행 환경 | 4 vCPU, `CAP_SYS_RESOURCE`가 없는 root, `msg_max = 10` | **[실측]** §7.1, §8.7.1. host IPC의 root 실행은 abort한다. 성공한 구성은 uid 65534 실행과 private IPC namespace 실행이다. |

#### 9.0.2 이 절에서 새로 측정한 것

**디스크.** 계획의 크기를 정하는 제약이다.

| 측정 | 결과 |
| --- | --- |
| `df -h /` | **[실측]** Avail 2.9G, 사용률 93%. 공통 전제의 "약 5 GB"보다 작다. |
| `du -sh $N` | **[실측]** 2.3G |
| 큰 디렉터리 (`du -sh $N/*/ \| sort -h`) | **[실측]** `probe` 375M (그중 `probe/cFS/build-native_std` 254M), `verify_C21_overreach` 270M, `rw-mlir` 250M, `rw-flight` 230M, `verify_C21` 221M |
| 6.4.1·6.4.2 tarball | **[확인된 사실]** 검증 C21의 두 투표자가 같은 tarball을 각각 `verify_C21/`와 `verify_C21_overreach/`에 내려받았다. sha256은 `ec26e33b…`와 `c64ed2aa…`로 같다. |

**[해석]** 다른 버전 build tree를 동시에 여러 개 둘 수 없다. bundle build tree 하나가 약 250 MB다. 그래서 역사 사례(6.4.1, 6.5.0a, 6.6.0a, v6.7–6.8 계열)를 한 번에 하나씩만 build해야 한다 (RK-1).

**out-of-tree dialect build.** W3의 위험을 미리 확인했다. 한 op짜리 ODS dialect를 local build tree의 `MLIRConfig.cmake`로 build했다.

| 단계 | 명령 | 결과 |
| --- | --- | --- |
| configure | `cmake -S . -B build -DMLIR_DIR=/home/user/work/llvm-project/build/lib/cmake/mlir` | **[실측]** rc 0, real 2.4 s (`cmake.log`) |
| build | `make -C build -j4` (`mlir_tablegen`으로 `-gen-op-decls/-defs`, `-gen-dialect-decls/-defs`) | **[실측]** rc 0, real 8.2 s, 실행 파일 8.9 MB, build 디렉터리 11 MB (`make.log`) |
| 실행 | `cfs.sb.subscribe %c6274_i32, %c1_i32 : i32`를 parse·verify·print | **[실측]** rc 0 (`run.out`). `MemoryEffects<[MemWrite]>`를 붙인 op에 `getEffects`가 생성되었다 (`gen_CfsOps.h.inc` L191). |

**[실측]** build tree는 Release+assertions, RTTI ON, 정적 라이브러리(`BUILD_SHARED_LIBS=OFF`)다 (`CMakeCache.txt`).

**dense 분석 골격.** W6의 위험을 미리 확인했다. §6.5.2가 고른 설정을 그대로 썼다. 분석은 `DenseForwardDataFlowAnalysis` 하위 class 하나다. 설정은 `DataFlowConfig().setInterprocedural(false)`와 `loadBaselineAnalyses`, 그리고 `ExternalCallee` hook override다. 이 골격은 API 호환성 확인용이다. 제안 분석이 아니다.

| 입력 (§5의 sample_app IR) | 결과 |
| --- | --- |
| `$N/s05_dialect/linked_noinl.opt.mlir` (앱 함수 `noinline`) | **[실측]** rc 0, real 0.029 s. `ExternalCallee` hook 호출 68회, 그중 몸체가 있는 callee 15회, 선언만 있는 callee 53회 (`dense_linked_noinl.opt.mlir.out`) |
| `$N/s05_dialect/linked.opt.mlir` (inline 후) | **[실측]** rc 0, real 0.062 s. hook 호출 243회, 모두 선언 callee (`dense_linked.opt.mlir.out`) |
| build | **[실측]** real 12.4 s, 실행 파일 24 MB (`make2.log`). 측정 뒤 build 디렉터리를 지웠다. |

**[해석]** local `1053047a`에서도 non-interprocedural 설정의 모든 `llvm.call`은 `ExternalCallee` hook으로 온다. 몸체가 있는 callee도 마찬가지다. 따라서 §6.5.2의 "summary를 이 hook에서 적용한다"는 설계는 local API로 구현할 수 있다. PoTATo는 같은 local LLVM에 대해 build가 실패했다 (`$N/rw-mlir/potato_build.log`). 이 골격은 그때 문제가 된 `RegionSuccessor` 계열 API를 쓰지 않는다.

**비주소 resource.** **[실측]** local `SideEffectInterfaces.h`에는 `isAddressable`이 없다 (`grep -n isAddressable` 결과 없음). main 사본에는 L135, L166, L340에 있다 (`$N/main_SideEffectInterfaces.h`). **[해석]** §5.4가 쓴 "비주소 resource effect는 alias 질의에서 NoAlias" 이점은 local에서 쓸 수 없다. 그래서 MVP 분석기는 SB·TBL op의 effect를 upstream `getModRef`에 맡기지 않고 직접 해석한다 (RK-2).

#### 9.0.3 계획 원칙

1. **[설계 제안]** 추출이 판정보다 먼저다. §7.13.4의 23개 site 정답표를 자동으로 재현하기 전에는 race 판정을 시작하지 않는다. §7의 현재 판단과 같다.
2. **[설계 제안]** 단계마다 관문을 둔다. 관문은 다음 셋 중 하나로 닫는다.
   - GO: 계획대로 진행한다.
   - PIVOT: 범위를 줄여 진행한다.
   - STOP: 해당 주장을 버린다.
3. **[설계 제안]** 정답과 계약은 분석기 실행 전에 동결한다 (§8.6, §8.11).
4. **[설계 제안]** 모델 적합성 시험(§8.3.4)을 통과한 의미만 benchmark 판정에 쓴다.
5. **[설계 제안]** 결과마다 cFE 버전, Σ(§1.3), 증거 수준을 붙인다. 수준이 다른 결과는 합산하지 않는다.
6. **[설계 제안]** 모르는 것은 `unknown`으로 센다 (§6 AP4). unknown의 수도 산출물이다.

### 9.1 전체 일정

| 주 | 단계 | 이번 주의 질문 | 주 산출물 **[설계 제안]** | 관문 |
| --- | --- | --- | --- | --- |
| W1 | 0 | 같은 입력에서 같은 IR이 나오는가 | `pin.lock`, `env.json`, `build_ir.py`, 154개 파일의 `-O1` 경로 import 결과 | — |
| W2 | 0 | 무엇을 맞혀야 하는가 | `api_model.yaml`, `truth/extract_23.json`, `truth/normal.yaml`, 사례 기록 template | GO-1 |
| W3 | 1 | cFE 호출을 `cfs` op로 올리는가 | `cfsa` 저장소, MVP op ODS, P1·P2 pass, 음성 시험 | — |
| W4 | 1 | MID를 정적으로 회수하는가 | P3·P5·P7, `sites.jsonl`, `edges.jsonl`, `config.jsonl` | GO-2 |
| W5 | 1 | handler와 state field를 찾는가 | P4·P6, `fields.jsonl`, `dispatch.jsonl` | — |
| W6 | 1 | payload → field → 사용을 잇는가 | `CfsProvAnalysis`, 함수 summary, `prov.jsonl` | — |
| W7 | 1 | 합성 앱에서 후보를 내는가 | SENSOR·NAV·CONTROL, 최소 HB 규칙, `cands.jsonl` | — |
| W8 | 1 | MVP가 성립하는가, MLIR이 필요한가 | SB 모델 적합성 결과, E-ast·E-llvm 비교표 | GO-3, GO-4 |
| W9 | 2 | ES 동기화를 조건부 간선으로 다루는가 | ES op 의미, HB-SYS1·SYS2·LIB·OSAL, R-F1i·R-F5 | — |
| W10 | 2 | 실제 startup 사례를 재현하는가 | SYN-F1-01, SYN-F5-01, CF #184, cFE #198 파생, INJ-TO-1 | GO-5 |
| W11 | 3 | 갱신 경계와 공동 갱신을 표현하는가 | `upd[β]`, `cls`, timestamp 층, HB-FIFO, R-F3·R-F4·R-F7 | — |
| W12 | 3 | 허용된 조합과 결함을 가르는가 | SYN-F3/F4, SYN-ORD-01, INJ-HK-2, INJ-LC-1/2, F4 문헌 확인 | GO-6 |
| W13 | 4 | TBL·재시작 의미가 실행과 맞는가 | TBL op·typestate, HB-TACT·TREG·RST, 적합성 시험 | — |
| W14 | 4 | TBL·재시작 사례를 가르는가 | SYN-F6-*, SYN-F8-*, INJ-SA-*, INJ-HK-1, 역사 사례 | GO-7 |
| W15 | 5 | 기준선·ablation과 비교해 무엇이 남는가 | 전체 bundle 실행, B0·B1·B2·B4, AB1–AB10, I1–I5 | — |
| W16 | 5 | 무엇을 주장할 수 있는가 | 주장–근거표, 층별 결과표, 공개 artifact | GO-8 |

**[해석]** 일정의 임계 경로는 W3–W6이다. 이 네 주가 늦어지면 뒤 단계가 모두 밀린다. 그래서 축소안(§9.11)은 단계 1을 줄이지 않고 단계 3–5를 줄인다.

### 9.2 단계 0 (W1–W2): 고정, 재현, 정답 동결

#### 9.2.1 W1: 고정과 재현

| 작업 **[설계 제안]** | 산출물 | 근거와 주의 |
| --- | --- | --- |
| 모든 SHA를 한 파일에 적는다. bundle, submodule 전부, llvm-project, 그리고 W10·W14에 쓸 역사 버전의 위치까지 적는다. | `pin.lock` | §9.0.1 |
| clang으로 configure한 build tree를 만든다. `CMAKE_C_COMPILER`는 local clang, `ENABLE_UNIT_TESTS=FALSE`이다. 그 tree의 `compile_commands.json`을 쓴다. | `build-ana/…/compile_commands.json` | **[확인된 사실]** 검증 C17 정정 문구: 154/154 import는 이 tree에서 `-Werror`, `-Wno-stringop*`를 빼고 `-Wno-everything`, `-disable-O0-optnone`을 더한 결과다. gcc DB에 `-Wno-unknown-warning-option`만 더하면 `cf_codec.c`, `cs_table_processing.c`가 `-Werror`로 실패한다. |
| §6.0 S1의 flag로 IR을 만든다: `-O1 -Xclang -disable-llvm-passes -g -Wno-unknown-warning-option -Wno-error`. 이어서 `mlir-translate --import-llvm --mlir-print-debuginfo`와 `mlir-opt --inline --sroa --mem2reg --canonicalize --cse`를 적용한다. | `build_ir.py`, `ir/`, `mlir/` | **[미확인]** `-O1` 경로는 앱 5개 17 TU에서만 측정했다 (§7.5.1). 154개 전체는 W1에서 처음 잰다. |
| 실행 환경 기록을 만든다. 항목은 kernel, nproc, uid, CapEff, `msg_max`, IPC namespace, scheduling policy, CPU affinity, OSAL permissive 여부, OSAL·PSP·LLVM SHA다. | `env.json` | §8.7.1의 항목 |
| 디스크 정책을 정한다. live build tree는 하나만 둔다. IR·MLIR·log만 남긴다. 중복 tarball은 hash를 확인하고 하나로 정리한다. | `disk_policy.md`, 주간 `df` 기록 | §9.0.2 |

**[설계 제안] W1 종료 기준.** `make reproduce` 한 번으로 다음을 다시 얻는다.

1. `-O0` 경로에서 154/154 import, 진단 0. 조건은 검증 C17 정정 문구와 같다.
2. 연결한 mission module에서 정의된 `llvm.func` 2389, 외부 선언 184, indirect `llvm.call` 73 (**[실측]** §7.5.2).
3. `-O1` 경로에서 코드 정의 MID의 상수 회수 site/전체 site가 sample_app 3/3, hk 4/7, sch_lab 1/2, ci_lab 4/4, to_lab 3/7이다 (**[실측]** 검증 C18 정정; to_lab의 세 번째는 `TO_LAB_CmdSubscribe` inline 사본의 6272·6273).
4. `-O1` 경로 154개의 결과표. 성공 수, 실패 파일, 실패 사유를 적는다.

#### 9.2.2 W2: 인벤토리와 정답 동결

**[설계 제안] API 의미표.** §3의 사실 ID와 §5의 op를 API 하나당 한 항목으로 묶는다. 의미를 켜는 단계도 적는다. 아래는 두 항목의 예다.

```yaml
# [설계 제안] api_model.yaml의 항목 예. cFE 546a0025 기준
- api: CFE_SB_Subscribe
  header: "cfe_sb.h:302"                 # §5.5.3
  op: cfs.sb.subscribe                   # variant = plain, msglim = 기본값
  default_msglim: 4                      # [확인된 사실] cfe_sb_internal_cfg.h L107 (§3.1 SB-2)
  effect: [W(SBRoutes)]
  status_semantics: "성공 경로에만 sub(U,m,p) 사건"        # §6 AP3
  facts: [SB-2]
  semantics_phase: 1
- api: CFE_SB_TransmitMsg
  header: "cfe_sb.h:438"
  op: cfs.sb.transmit                    # variant = msg
  effect: [R(%msg), R(SBRoutes), W(SBRoutes), W(SBQueues)]
  model: M1                               # destination별 put 사건의 열 (§3.8)
  nonsuccess_means: "호출자 쪽 오류만"      # A10 (§3.9)
  facts: [SB-3, SB-4, SB-5, SB-6, SB-7, SB-8]
  semantics_phase: 1
```

**[설계 제안] 동결할 정답.**

| 파일 | 내용 | 출처 |
| --- | --- | --- |
| `truth/extract_23.json` | §7.13.4의 23개 site: 앱, 파일:줄, API, `mid` 분류(const 값 집합, param, table, state, cmd_payload) | **[확인된 사실]** 고정 commit 소스. **[실측]** probe·검증 재실행의 값 |
| `truth/normal.yaml` | 경고하면 안 되는 정상 대조 다섯 가지와 출처 | 아래 표 |
| `truth/cases/` | §8.6 schema의 template. W7·W9·W11·W13에 사례를 채운다. | §8.6 |

| 정상 대조 | 근거 |
| --- | --- |
| TO_LAB은 table 기반 telemetry 구독을 `CFE_ES_WaitForStartupSync` 뒤로 미룬다. 그래서 `0x808` 미구독 발행이 생긴다. | **[확인된 사실]** [to_lab_app.c L69–L73](https://github.com/nasa/to_lab/blob/d27c6014cb5d2979140500f635876ee87b81995b/fsw/src/to_lab_app.c#L69-L73), commit d3d52da [S46] (v7.0.1에만 있음, 검증 C16) |
| HK는 기본값 `HK_DISCARD_INCOMPLETE_COMBO 0`에서 불완전 combined packet을 보낸다. 대신 `DataPresent`와 `MissingDataCtr`로 표시한다. | **[확인된 사실]** [hk_utils.c L483–L504](https://github.com/nasa/HK/blob/0dc16b7a7bab71747b9c063cf405ad59a65a40a5/fsw/src/hk_utils.c#L483-L504), [hk_internal_cfg.h L57–L69](https://github.com/nasa/HK/blob/0dc16b7a7bab71747b9c063cf405ad59a65a40a5/fsw/inc/hk_internal_cfg.h#L57-L69) |
| core 초기화 중의 `0x808` 미구독 발행. 발신자는 `CFE_SB`, `CFE_TBL`, `CFE_TIME`이고, CORE_READY 이전이다. | **[실측]** 8회 실행 모두에서 관측되었다. **[확인된 사실]** ES는 CORE_READY 뒤에 script 앱을 시작한다 ([cfe_es_start.c L195–L202](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/es/fsw/src/cfe_es_start.c#L195-L202)) |
| SCH_LAB은 1 Hz 메시지를 한 번 받은 뒤에만 schedule 전송을 시작한다 (`SCH_OneHzPktsRcvd > 0`). | **[확인된 사실]** [sch_lab_app.c L116](https://github.com/nasa/sch_lab/blob/607e2f90c5f828f3f3ae655a4debacf8b2cc4bbb/fsw/src/sch_lab_app.c#L116) (§8.3.1) |
| Ogma `cfs-002-state-machines`의 passive 입력은 저장만 된다. active 입력이 올 때 평가된다. | **[확인된 사실]** 검증 C06(contested)의 code 사실. db.json [S93], [copilot_cfs.c L218–L239](https://github.com/nasa/ogma/blob/08b384b4335cce324fac74577adc7f08e61b7cce/ogma-core/templates/cfs/copilot/fsw/src/copilot_cfs.c#L218-L239) |

**[설계 제안]** W2에 문헌 요청도 시작한다 (§9.12). 결과가 W12와 W16의 관문에 필요하기 때문이다.

#### 9.2.3 관문 GO-1 (W2 말): 재현 가능한 입력

| 판정 | 조건 **[설계 제안]** | 다음 행동 |
| --- | --- | --- |
| GO | W1 종료 기준 1–4를 모두 만족한다. 정답 파일이 작성자 B의 서명과 함께 동결되었다. | W3 시작 |
| PIVOT | `-O1` 경로가 154개 중 일부 파일에서 실패한다. | 실패 파일을 목록에 남기고 MVP 앱 5개의 성공만 확인한 뒤 진행한다. 실패 파일이 MVP 앱에 있으면 그 파일만 `-O0` 경로와 alloca 1단계 추적으로 처리한다 (§7.12의 대비 경로). |
| STOP | 해당 없음 | **[실측]** import 경로는 이미 작동한다 (§7.5). 이 관문에서 연구 방향을 버릴 이유는 없다. |

### 9.3 단계 1 (W3–W8): MVP — Software Bus, dispatch, state 출처

#### 9.3.1 범위

**[원노트 구상]** 원노트 §30·§56의 MVP는 `CFE_SB_CreatePipe`, `CFE_SB_Subscribe`, `CFE_SB_ReceiveBuffer`, `CFE_SB_TransmitMsg` 네 함수에서 시작한다. 수정본 §11.2는 네 이름만 인식해서는 메시지 처리 관계를 복원할 수 없다고 보정했다.

**[설계 제안]** 네 함수를 중심에 두고, §7.13.2의 실측 근거에 따라 필요한 것만 더한다.

| 묶음 | API | MVP에서의 처리 | 더하는 이유 |
| --- | --- | --- | --- |
| 원노트의 네 함수 | `CFE_SB_CreatePipe`, `Subscribe`, `ReceiveBuffer`, `TransmitMsg` | 의미 적용 | 원노트 §56 |
| 구독 변형 | `SubscribeEx`, `SubscribeLocal`, `Unsubscribe` | 의미 적용 | **[확인된 사실]** TO_LAB table 구독은 `SubscribeEx`다 ([to_lab_app.c L504](https://github.com/nasa/to_lab/blob/d27c6014cb5d2979140500f635876ee87b81995b/fsw/src/to_lab_app.c#L500-L506)). |
| 발행 MID | `CFE_MSG_Init`, `CFE_SB_TransmitBuffer` | 의미 적용 | **[확인된 사실]** 발행 MID는 `TransmitMsg`의 인자가 아니다. `CFE_MSG_Init`이 buffer에 먼저 묶는다 (§4.2.2). |
| dispatch | `CFE_MSG_GetMsgId`, `CFE_MSG_GetFcnCode`, helper `ValueToMsgId`·`MsgId_Equal`·`MsgIdToValue` | 의미 적용 | **[실측]** dispatch는 `GetMsgId` out-parameter와 함수 static cache의 비교다 (§7.7.3). |
| ES 대기 | `CFE_ES_WaitForStartupSync`, `WaitForSystemState`, `RunLoop` | op로만 올린다. HB 간선은 만들지 않는다. | **[해석]** TO_LAB의 지연을 단계 2에서 판정하려면 site가 먼저 필요하다. 단계 1의 F1 출력에는 `es_semantics: off`를 붙인다. |
| 그 밖의 `CFE_*`·`OS_*` | — | `llvm.call`로 둔다. lint L5로 센다 (§5.6). | D7 |

**[설계 제안] op 집합.** §5.5에서 다음만 구현한다. ODS는 §5.5의 초안을 그대로 쓴다.

- 선언: `cfs.app`, `cfs.task`, `cfs.pipe_site`, `cfs.field`, `cfs.dispatch_case`, `cfs.config_send`, `cfs.config_subscribe`.
- handle·MID: `cfs.handle.wrap`, `cfs.handle.raw`, `cfs.mid.const`, `cfs.mid.from_value`, `cfs.mid.raw`, `cfs.mid.equal`, `cfs.mid.is_valid`.
- SB·MSG: `cfs.sb.create_pipe`, `cfs.sb.subscribe`(plain|ex|local), `cfs.sb.unsubscribe`, `cfs.sb.receive`, `cfs.sb.buf_ptr`, `cfs.sb.transmit`(msg|buffer), `cfs.msg.init`, `cfs.msg.get_msgid`, `cfs.msg.get_fcncode`.
- ES(구문만): `cfs.es.run_loop`, `cfs.es.wait_system_state`, `cfs.es.wait_startup_sync`.
- state: `cfs.state.read`, `cfs.state.update`.

**[설계 제안] 도메인 부분집합.** §6.3의 추상 상태 중 단계 1에서 켜는 성분은 다음과 같다.

```text
# [설계 제안] 단계 1의 CfsLattice (§6.3의 부분집합)
σ♯₁ = ( Prov₁, Mem, Inp, Val, Aux₁ )
  Prov₁ := ⊥ | Disj(S ⊆ Joint₁, |S| ≤ K) | NonRel(Field → 2^Origin, lostAt)
  Joint₁ := ( asg : Field ⇀ Origin, cls : Field ⇀ ClassId, val : CondField ⇀ {c, ⊤} )
            # upd[β]는 단계 3에서 켠다
  Origin := Msg(mid, payloadPath, rcvSite) | Init(site) | Const(c) | OutParam(api, site)
          | Param(i) | Unknown(reason)
            # Tbl(table, path)는 단계 4, Time(cfe)는 단계 3에서 켠다
  Aux₁ := ( bufOf : Value ⇀ (pipe, rcvSite, cls, mid?, live), status : Value ⇀ 2^Code )
            # subs·readyMark는 단계 2, tblPtr는 단계 4
```

**[설계 제안] 규칙 부분집합.**

- HB 규칙은 HB-PO, HB-MSG, HB-CORE, HB-LIB만 쓴다 (§6.2).
- `Req`는 R-F2(기본 생성)와 R-F1c(계약이 있을 때만)만 쓴다 (§6.6).
- 판정은 `G0`만 쓴다.

**[설계 제안] 앱 범위.**

- 포함: 공개 앱 sample_app, ci_lab, hk, sch_lab, to_lab과 합성 앱 SENSOR, NAV, CONTROL.
- 제외: SBN과 CF.
- **[실측]** 제외하는 이유는 mission의 indirect call 73곳 중 SBN 계열이 30곳, CF가 16곳이기 때문이다 (§6.5.5).
- cFE core module은 분석 대상이 아니라 §3의 모델로만 쓴다.

#### 9.3.2 주별 작업

| 주 | 작업 **[설계 제안]** | 산출물 | 종료 기준 **[설계 제안]** |
| --- | --- | --- | --- |
| W3 | `cfsa` 저장소를 만든다 (`find_package(MLIR)`, §9.0.2의 방식). MVP op의 ODS를 쓴다. P1 `cfs-close-world`와 P2 `cfs-recognize-api`를 구현한다. | `include/cfs/CfsOps.td`, `lib/Lifting/CloseWorld.cpp`, `lib/Lifting/RecognizeApi.cpp`, `tools/cfs-opt`, `test/neg/*.mlir` | (a) §5의 음성 시험 `neg1–neg3.mlir`, `neg_sig.mlir`이 같은 이유로 거부된다. (b) sample_app에서 P2가 바꾼 op 수가 `$N/s05_dialect/api_sites.py`가 위치 기준 중복 제거로 센 MVP API site 수와 같다. (c) 바꾼 op의 Location이 원래 `llvm.call`과 100% 같다 (D6). (d) op 종류와 순서가 손으로 쓴 `sample_app_lifted.mlir`과 같다 (SSA 이름 무시). |
| W4 | P3 `cfs-fold-mid`, P5 `cfs-bind-headers`, P7 `cfs-attach-config`를 구현한다. table C 초기화자는 Clang AST로 읽는다 (§6.0 S2). | `sites.jsonl`(§7.13.3의 `SiteRecord`), `config.jsonl`, `edges.jsonl` | GO-2 (§9.3.4) |
| W5 | P4 `cfs-resolve-fields`, P6 `cfs-recover-dispatch`를 구현한다. out-parameter 쓰기를 명시한다 (D2). | `fields.jsonl`, `dispatch.jsonl` | 아래 W5 기준 |
| W6 | `CfsProvAnalysis`를 구현한다. `DenseForwardDataFlowAnalysis<CfsLattice>`, `setInterprocedural(false)`, `ExternalCallee` hook의 summary 적용(§6.5.3)이다. | `lib/Analysis/Provenance.cpp`, `prov.jsonl` | 아래 W6 기준 |
| W7 | §8.3.1의 SENSOR·NAV·CONTROL을 bundle `apps/`에 넣는다. 사건 추출(§6.1), `BUILD_HB` 부분집합, R-F2·R-F1c, `G0` 판정을 구현한다. 작성자 B가 합성 앱 계약과 정답을 동결한다. | `bench/syn/`, `contracts/*.yaml`, `truth/cases/SYN-F2-*.yaml`, `cands.jsonl` | 아래 W7 기준 |
| W8 | SB 모델 적합성 시험(§8.3.4의 SB 다섯 줄)을 돌린다. E-ast·E-llvm 비교를 한다. | `conformance_sb.md`, `go3_compare.md` | GO-3, GO-4 |

**[설계 제안] W5 기준.**

- probe가 `-O0`에서 위치로 찾은 전역 field store 83곳이 모두 `cfs.state.update`가 되고, 모두 field 이름을 가진다.
  - **[실측]** 83곳의 앱별 분포는 sample_app 11, hk 15, ci_lab 16, to_lab 41, sch_lab 0이다 (§7.8).
  - **[실측]** probe가 offset 0 때문에 이름을 붙이지 못한 5곳(sample_app 4, ci_lab 1)도 포함한다 (§7.8).
- out-parameter 쓰기 세 종류가 `via = out_param`으로 잡힌다.
  - `CFE_SB_CreatePipe(&SAMPLE_APP_Data.CommandPipe, …)`
  - `CFE_TBL_Register(&…TblHandles[0], …)`
  - `OS_CountSemCreate(&SCH_LAB_Global.TimingSem, …)` (§7.8)
- `sch_lab_app.c:130`의 `LocalStateEntry->Counter` 쓰기는 둘 중 하나로 처리한다. SSA로 풀거나, `field = unknown`으로 세어 보고한다. 조용히 빠뜨리면 실패다.
- `cfs.dispatch_case`가 작성자 B의 손 정답표와 같다. 대상은 sample_app(`SAMPLE_APP_TaskPipe.CMD_MID`, `SEND_HK_MID`; [sample_app_dispatch.c L132–L167](https://github.com/nasa/sample_app/blob/199476a34827ae84d50d66f97619227854cd971a/fsw/src/sample_app_dispatch.c#L132-L167)), hk, ci_lab이다.

**[설계 제안] W6 기준.**

- (a) 원노트 §44 예.
  - 입력은 `$N/verify-C20/counter/s44.c`를 cFS 모양으로 넓힌 `s44_cfs.c`다. `CFE_SB_ReceiveBuffer` loop가 `ATTITUDE_MID`를 `HandleAttitude`로 dispatch한다.
  - 이 입력에서 `Guidance`의 `global.att` 읽기의 Origin은 `{Msg(ATTITUDE_MID, att, rcvSite)}`만이어야 한다.
  - **[실측]** 대조로, upstream `-test-last-modified`는 private visibility와 driver를 붙여도 같은 예에서 `<unknown>`을 돌려준다 (`$N/verify-C20/counter/s44_O0_priv_driver_lastmod.out`).
- (b) 수정본 §6.4의 함정 사례(SYN-F4-02의 정상 변형)에서 `(k, j)` Joint가 생기지 않는다.
- (c) K ∈ {2, 4, 8, 16}에서 MVP 앱 5개의 `NonRel` 사영 위치 수(`lostAt`)를 표로 낸다.
- (d) MVP 앱 범위의 분석 시간과 최대 메모리를 기록한다.

**[설계 제안] W7 기대 판정.** 정답은 작성자 B가 분석기 실행 전에 동결한다.

| 사례 | 변형 | 기대 |
| --- | --- | --- |
| SYN-F2-01 | faulty | R-F2 후보 L0. witness에 `CONTROL_WAKEUP` 처리가 첫 `NAV_STATE` 수신보다 먼저 오는 사건열이 있다. |
| | correct (`AttValid` guard) | 후보 없음. `Val`·`CondField`로 guard를 인식한다 (§6.3.2). |
| | benign (`allows_default`) | 후보 없음. 계약 출처를 기록한다. |
| SYN-F2-02 | faulty (원노트 §44 형태, guard 없음) | 후보 L0 |
| | correct (`IsAttReady()` 안의 guard) | 후보 없음. summary의 `pathCond`가 필요하다 (§6.5.3). |
| SYN-F1-01 | 네 변형 모두 | 단계 1에서는 `es_semantics: off`를 붙인 L0 목록만 낸다. 판정은 단계 2에서 한다. |

#### 9.3.3 MVP 파이프라인과 출력 기록

```text
# [설계 제안] MVP driver (W3–W8). 구현되지 않음.
# 입력 : System = (bundle 5a9b075c + pin.lock, build-ana의 compile DB,
#                  sample_defs/tables/*.c, 앱 table 소스, cfe_es_startup.scr,
#                  contracts/*.yaml, Σ, cfe = 546a0025)
# 출력 : sites.jsonl, config.jsonl, edges.jsonl, fields.jsonl, dispatch.jsonl,
#        prov.jsonl, cands.jsonl, run.json (env, 시간, unknown 수)
procedure MVP(System):
  for tu in compileDB where app(tu) ∈ MVP_APPS:
      ir[tu] := clang(tu.flags ⊕ S1_FLAGS)                        # §6.0 S1
  for app in MVP_APPS:
      m := llvm-link(ir[app]) |> import --mlir-print-debuginfo |> inline,sroa,mem2reg,canonicalize,cse
      m := m |> P1 close-world |> P2 recognize-api(MVP_API) |> P3 fold-mid
             |> P4 resolve-fields |> P5 bind-headers |> P6 recover-dispatch |> P7 attach-config
      solver := DataFlowSolver(DataFlowConfig().setInterprocedural(false))
      loadBaselineAnalyses(solver); solver.load<CfsProvAnalysis>(K, summaries(m), api_model)
      solver.initializeAndRun(m)
      facts[app] := extract_events(m, solver)                       # §6.1
  sites := collect(cfs.sb.*, cfs.msg.init);  CHECK_EXTRACTION(sites, truth/extract_23.json)
  edges := JOIN(Pub(m) ← cfs.sb.transmit.mid ∪ cfs.config_send,
                Sub(m) ← cfs.sb.subscribe ∪ cfs.config_subscribe, handler ← cfs.dispatch_case)
  G0 := BUILD_HB(facts, edges, rules = {HB-PO, HB-MSG, HB-CORE, HB-LIB}, ver = 546a0025, A = ∅)
  for r in R-F2(facts) ∪ R-F1c(contracts):
      emit Candidate(r, verdict = (G0 ⊢ r.e1 ≺ r.e2 ? GUARANTEED : UNPROVEN), level = L0)
```

```text
# [설계 제안] 출력 기록. SiteRecord는 §7.13.3, 진단 문면은 §6.9, 정답 schema는 §8.6을 따른다.
EdgeRecord { mid : u32,
             pub : [ (app, SiteRef, source : code | table(file:line) | cmd(handler)) ],
             sub : [ (app, pipe_site, msglim, SiteRef) ],
             handler : [ FuncRef ],
             pub_complete : bool }          # HB-MSG 조건 ②. 명령·unknown 발행자가 있으면 false
ProvRecord { app, read_loc : file:line:col, field : (global, [lo, hi)),
             joints : [ { asg : Origin, cls : ClassId } ] | nonrel { origins, lostAt } }
Candidate  { id, category : F1..F8, rule : R-F*, req : (e1, e2),
             verdict : GUARANTEED | GUARANTEED_UNDER(A′) | UNPROVEN | VIOLATED_ALWAYS,
             assumptions : [..], witness : [ (event, file:line:col) ],
             level : L0 | L1 | L2, unknowns : [..], es_semantics : on | off,
             cfe : sha, sigma : Σ-id, dedup_key }     # dedup_key는 §8.6과 같은 규칙
```

```text
# [설계 제안] GO-2 판정에 쓰는 비교 절차
procedure CHECK_EXTRACTION(sites, truth):          # truth = 23개 (§7.13.4)
  miss := []; wrong := []
  for t in truth:
    s := sites.find(file = t.file, line = t.line, api = t.api)   # inline 사본은 위치로 묶는다
    if s = ⊥:                                          miss += t
    elif t.kind = const and values(s) ≠ t.values:      wrong += t
    elif t.kind = param and inlined_values(s) ≠ {6272, 6273}: wrong += t   # to_lab_app.c:261
    elif t.kind ∉ {const, param} and kind(s) ≠ t.kind: wrong += t
  extra := sites(MVP 공개 앱 5개) \ truth                # 과잉 site는 사유와 함께 보고
  return (miss, wrong, extra)
```

#### 9.3.4 관문 GO-2 (W4 말): MID를 정적으로 회수하는가

**[설계 제안] 측정.**

1. `CHECK_EXTRACTION`의 결과.
   - **[확인된 사실]** 23개 중 15개는 코드에서 정한 MID다. 14개는 직접 상수이고, 1개는 param이다 (`to_lab_app.c:261`, inline 사본에서 6272·6273).
   - **[확인된 사실]** 나머지 8개는 실행 데이터에서 온다. table 5개(`hk_utils.c:305`, `:322`, `:436`, `sch_lab_app.c:246`, `to_lab_app.c:504`), state 1개(`to_lab_app.c:454`), cmd_payload 2개(`to_lab_cmds.c:218`, `:280`)다 (§7.13.4).
2. P7이 table site를 값 집합으로 바꾼 결과.
   - TO_LAB 구독 table은 37개 항목이다 (**[확인된 사실]** `sample_defs/tables/to_lab_sub.c`, 검증 C16).
   - HK copy table은 기본 `hk_cpy_tbl.c`의 항목 0–4다 (**[확인된 사실]** 검증 C16).
   - SCH_LAB은 bundle override인 `sample_defs/tables/sch_lab_table.c`를 쓴다 (**[확인된 사실]** 검증 C16). sch_lab 저장소의 기본 table은 비어 있다.
   - 기준은 별도 counting 스크립트가 C 초기화자에서 센 수와 같은지다.
3. bundle 전체의 구독 site 분류.
   - **[확인된 사실]** `apps/*/fsw/src`의 `CFE_SB_Subscribe*` 호출 47곳 중 8곳이 data·명령 구동이다 (검증 C16 정정; `cf_cfdp.c:1288`, `ds_table.c:938`, `ds_cmds.c:1327`, `hk_utils.c:322`, `lc_watch.c:225`, `sbn_subs.c:425`, `to_lab_app.c:504`, `to_lab_cmds.c:218`).
   - 기준은 47곳 모두에 `mid` 분류가 붙는 것이다.

| 판정 | 조건 **[설계 제안]** | 다음 행동 |
| --- | --- | --- |
| GO | `miss = wrong = ∅`. table site 다섯 곳이 모두 값 집합을 갖고, 항목 수가 counting 스크립트와 같다. 47곳 모두에 분류가 붙는다. | W5 진행 |
| PIVOT-A (코드 MID 실패) | 15개 코드 정의 site 중 하나라도 IR에서 상수로 회수되지 않는다. | MID 결정을 AST sidecar로 옮긴다. literal의 spelling 위치는 AST에 남는다 (**[실측]** `$N/probe/cir/ast_SAMPLE_APP_Init.txt` L124–L136). 흐름 분석은 IR에 둔다. 이 경로는 I6 ablation의 한쪽이 된다. |
| PIVOT-B (table MID 실패) | table 이미지를 값으로 풀지 못한다. | 해당 MID의 `pub_complete := false`로 둔다. 그러면 HB-MSG 조건 ②가 깨진다. F1·F7 판정을 발행자가 모두 코드 상수인 MID로 줄이고, 줄인 범위를 결과에 적는다. |
| STOP | 합성 앱의 구독 MID조차 IR과 AST 어느 쪽으로도 회수되지 않는다. | RQ1(앱 간 관계 복원)을 버린다. 연구를 앱 내부 분석(F2의 단일 앱 형태, F6 typestate)으로 줄인다. **[해석]** 실측상 가능성은 낮다. 코드 정의 MID는 `-O1` 경로에서 이미 상수다 (§7.7). |

#### 9.3.5 관문 GO-3 (W8): MLIR이 AST 경로보다 무엇을 더하는가

**[원노트 구상]** 원노트 §43은 "grep → graph"로 끝나면 MLIR이 필요 없다고 썼다. 수정본 §9.4는 같은 의미의 그래프·AST 분석과 비교하라고 요구한다.

**[설계 제안] 비교 대상.** §8.10.2의 E-dialect, E-llvm에 시간 제한을 둔 E-ast를 더한다.

| 엔진 | 구현 범위 | 시간 제한 |
| --- | --- | --- |
| E-dialect | 단계 1 구현 그대로 | — |
| E-llvm | lifting 없이 같은 `CfsLattice`. `llvm.call`마다 API 이름 표로 전이 함수를 고른다. | 2일 |
| E-ast (이 절에서 추가) | Clang LibTooling과 Clang CFG. 23개 site, dispatch case, field 쓰기 위치를 추출한다. 같은 API 의미표를 쓴다. 함수 사이 출처 분석은 하지 않는다. | 3일 |

**[설계 제안] 측정 항목.**

| ID | 측정 | 이미 알려진 출발점 |
| --- | --- | --- |
| G3-a | E-ast가 23/23, dispatch, field 쓰기를 맞히는가. 그때 든 시간과 LOC. | **[실측]** AST에는 MID literal의 spelling 위치와 `MemberExpr` field 이름이 남는다 (§7.12). |
| G3-b | §44 예와 SYN-F2-01/02에서 SB·MSG 호출이 만든 `Unknown` 출처 수. E-dialect와 E-llvm을 비교한다. | **[실측]** `llvm.call`은 callee가 외부든 정의되었든 모든 위치에 ModRef다. readnone `memory_effects`가 있어도 같다 (검증 C20 (b); I2). |
| G3-c | 진단 witness 사건 중 file:line:col을 가진 비율 | **[실측]** 154개 파일의 import에서 `llvm.call` 8830개와 `llvm.load` 26378개가 모두 위치를 가진다 (검증 C17 정정; §5.7.1). 앱 5개에서 위치가 없는 것은 함수 입구의 매개변수 spill store와 입구로 올라간 상수뿐이다 (§7.9). |
| G3-d | 단계 2의 ES op 묶음을 더할 때 바뀐 파일 수와 LOC. 이 측정은 W9 말에 기록한다. | — |

| 판정 | 조건 **[설계 제안]** | 다음 행동 |
| --- | --- | --- |
| GO (MLIR 유지) | E-dialect가 G3-b 또는 G3-c에서 E-llvm보다 낫다. W9의 G3-d에서 크게 불리하지 않다. | 단계 2 진행. 이점은 공학 지표로만 보고한다 (§8.10.2). |
| PIVOT-1 | E-ast가 G3-a를 3일 안에 맞힌다. | 추출은 MLIR의 근거가 아니라고 기록한다. MID 이름은 어차피 AST sidecar로 얻는다 (I6). RQ6의 주장은 출처·순서 분석(G3-b)으로만 좁힌다. |
| PIVOT-2 | E-dialect와 E-llvm이 G3-b, G3-c에서 같다. | 단계 2–4의 규칙을 E-llvm 방식(이름 표 전이)으로 구현해 시간을 아낀다. `cfs` dialect는 IRDL 구조 명세(§5)로만 남긴다. "dialect가 기여한다"는 주장은 버린다. |
| STOP (MLIR 기반 철회) | PIVOT-1과 PIVOT-2가 모두 성립한다. Clang CFG 위의 출처 시제품(추가 3일)이 §44 예를 맞힌다. | 원노트 §43의 조건이 성립했다고 결과로 보고한다. 이후 단계는 가장 싼 기반에서 구현한다. 도메인 의미의 기여(N1–N4, §4.7.2)는 기반과 무관하게 계속 검증한다. |

**[해석]** 이 관문은 MVP 시점의 잠정 판정이다. 최종 판정은 W15의 E-ql(B2와 같은 CodeQL 구현)까지 포함한 GO-8에서 한다.

#### 9.3.6 관문 GO-4 (W8): MVP가 성립하는가

**[원노트 구상]** 원노트 §57은 `APP_A → MID_X → Software Bus → APP_B Handler → state.x → Control()`가 소스에서 자동 복원되고, 삽입한 use-before-update에 경고하면 Phase 1이 성공이라고 썼다.

**[설계 제안] 기준.** 다섯 개를 모두 만족해야 한다.

1. 합성 앱에서 사슬 `NAV tx(NAV_STATE_MID) → CONTROL hdl → CONTROL_Data.Att 쓰기 → CONTROL_ControlStep 읽기`가 `EdgeRecord`와 `ProvRecord`로 자동 복원된다. 모든 고리에 위치가 있다.
2. W7 기대 판정표의 SYN-F2-01·SYN-F2-02 다섯 변형이 모두 기대와 같다.
3. §9.2.2의 정상 대조 다섯 가지에서 F2 후보가 0이다. F1은 `es_semantics: off` 목록에만 나온다.
4. 모든 후보에 §6.9 형식의 witness와 file:line:col이 있다.
5. GO-2를 통과했다.

**[설계 제안] SB 모델 적합성.** W8에 §8.3.4의 SB 다섯 줄(MsgLim 초과, pipe 가득 참, 구독 해제 뒤 송신, 수신 순서, 발행 중 선점)도 확인한다. 실측 근거는 §8.3.4의 표에 있다. 분석기 모델의 예측은 차례로 다음과 같아야 한다. 해당 pipe에서만 drop하고 반환은 `CFE_SUCCESS`다. depth 초과분은 drop하고 반환은 성공이다. 구독 해제 뒤 송신은 counter·event 없이 성공하고 sequence가 증가한다. HB-FIFO는 같은 pipe·같은 sender에만 적용한다. `tx.e → hdl` 간선은 없다. 예측이 하나라도 틀리면 그 의미를 쓰는 판정은 L0으로만 보고한다.

| 판정 | 조건 | 다음 행동 |
| --- | --- | --- |
| GO | 기준 1–5 만족 | 단계 2 진행 |
| PIVOT | correct 변형에서 오경고가 난다. 원인이 guard 인식(`CondField`, `Val`)이다. | W9의 처음 3일을 guard 인식에 쓴다. 단계 2에서 cFE #198 파생 예제를 뺀다. |
| STOP | W6 기준 (a)를 summary로도 맞히지 못한다. 즉 handler의 쓰기와 다른 함수의 읽기를 잇지 못한다. | RQ2를 지지하지 못한다고 기록한다. 연구를 topology(B1 수준)와 API 프로토콜 검사로 줄인다. |

### 9.4 단계 2 (W9–W10): startup 의미

#### 9.4.1 범위

| 항목 | 단계 2에서 켜는 것 **[설계 제안]** | 근거 |
| --- | --- | --- |
| op 의미 | `cfs.es.run_loop`, `wait_system_state`, `wait_startup_sync`, `create_child_task`, `exit_app` (§5.5.4) | §3 ES-1–ES-8 |
| 사건 | `ready(B)`: B의 main task가 처음 도달하는 `WaitForSystemState(S ≥ OPERATIONAL)`·`WaitForStartupSync`·`RunLoop` 진입. `waitok(A,S)`: status를 검사한 `WaitForSystemState`의 성공 분기에서만 생긴다. | **[확인된 사실]** 대기 호출은 호출자 자신의 AppState를 먼저 올린다. `WaitForStartupSync`는 상태를 버리는 void wrapper다 ([cfe_es_api.c L514–L619](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/es/fsw/src/cfe_es_api.c#L514-L619)). |
| HB | HB-SYS1(`G0`), HB-SYS2(`G_A`, 가정 `NoStartupTimeout(B)`), HB-OSAL(가정 `OsalNameLookup`, **[미확인]**). HB-CORE·HB-LIB는 단계 1에서 켰다. | **[확인된 사실]** script 앱 동기화는 기본 1000 ms의 soft limit다. 실패하면 syslog만 쓰고 진행한다 ([cfe_es_start.c L204–L229](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/es/fsw/src/cfe_es_start.c#L204-L229)). |
| Req | R-F1i, R-F5 (§6.6) | — |
| 도메인 | `Aux.subs`, `Aux.readyMark` | §6.3.4 |
| 판정 | `GUARANTEED_UNDER(A′)`를 쓴다. 가정 목록을 결과에 싣는다. | §6.7.1 |

#### 9.4.2 주별 작업

| 주 | 작업 **[설계 제안]** | 종료 기준 **[설계 제안]** |
| --- | --- | --- |
| W9 | ES op 의미와 위 HB·Req를 구현한다. bundle의 OSAL·PSP 고정값으로 §8.3.4의 "OPERATIONAL 의미" 시험을 다시 실행한다. W9 말에 GO-3의 G3-d를 기록한다. | (a) §6.2의 사슬 예에서 판정이 둘로 갈린다. `a`가 status를 검사한 `WaitForSystemState(OPERATIONAL)`를 쓰면 `GUARANTEED_UNDER(NoStartupTimeout(b))`다. `WaitForStartupSync`를 쓰면 사슬이 끊겨 후보가 된다. (b) 정상 대조 다섯 가지에서 F1·F5 후보가 0이다. (c) 적합성 시험이 다음 실측을 예측한다. **[실측]** 2.5 s 늦은 앱이 있을 때 ES는 'Startup Sync failed'를 두 번 쓰고 OPERATIONAL로 갔다. 다른 앱의 `WaitForSystemState(OPERATIONAL,5000)`은 약 1.95–2.01 s 뒤 성공했다. 네 설정 각 1회와 반복 20회가 모두 같았다 (`$N/sem-es-tbl/runs/probe_results_summary.txt` L66–L80, L240–L254, L415–L427, L590–L604; §1.2 F5). |
| W10 | 아래 사례를 분석하고, 가능한 것은 L2 재현을 시도한다. 작성자 B는 W9 초에 정답을 동결한다. | GO-5 |

| 사례 | 내용 | 기대 판정 **[설계 제안]** | L2 시도 방법 |
| --- | --- | --- | --- |
| SYN-F1-01 (네 변형) | §8.3.3 | faulty: 후보. correct: GUARANTEED. benign: 허용. conditional: `GUARANTEED_UNDER(NoStartupTimeout)` | `CONTROL` init의 `OS_TaskDelay` 주입, startup script 순서, 1 CPU·SCHED_RR (§8.7.2) |
| SYN-F5-01 (네 변형) | §8.3.3, CF #184 형태 | faulty: R-F5 후보. `CFE_LIB` init으로 옮긴 correct: GUARANTEED(HB-LIB). conditional 1·2: 가정을 적은 조건부 | provider init 지연 |
| CF #184 | **[확인된 사실]** 수정 833fdbb [S44]와 그 parent가 공개되어 있다. 수정은 `OS_CountSemGetIdByName`을 100 ms 간격으로 25번까지 재시도한다. | parent: 후보. 수정판: conditional. 재시도 한도와 owner 재시작을 처리하지 않기 때문이다 (§8.5). | semaphore를 만드는 합성 앱, startup 순서 제어 |
| cFE #198 파생 | **[확인된 사실]** 6.5.0a(792f5e35 [S101])에는 `APPS_INIT`·`LATE_INIT`이 없고 6.6.0a(`2661d19f`)에는 있다 (§2.4.2). EVA CWS 앱은 비공개다. | 6.5.0a 위의 2앱: 후보. 6.6.0a에서 `WaitForSystemState(APPS_INIT)`를 쓴 판: 가정 아래 보장 | 두 cFE 버전을 각각 build한다. **[미확인]** gcc 13.3에서 build되는지 확인하지 않았다. |
| INJ-TO-1 | **[확인된 사실]** d3d52da 이전처럼 table 구독을 `TO_LAB_init` 안으로 옮긴다 (§8.4.2). | F1 계열 MsgLim 손실 후보. 현재판은 benign | 수정하지 않은 bundle과 같은 실행 구성 |
| cFE #73 | **[확인된 사실]** 수정 전 6.4.1(SourceForge, 2014-12-12)과 수정 후 6.4.2(2015-07-13)가 공개되어 있다. 6.4.2는 core 앱별 `CFE_ES_ApplicationSyncDelay`와 panic을 넣었다. `EVS_SendEvent`의 범위 검사와, `EVS_AppID`를 TaskInit 마지막에 기록하는 변경도 넣었다. SB AppId 초기값의 불일치는 고쳐지지 않았다 (검증 C21 정정 문구; §1.2 F5). | source 수준의 수정 전후 대조만 한다. Microblaze/GRC 환경은 없다. | build 시간 제한 2일. 실패하면 정적 대조로 끝낸다. |

#### 9.4.3 관문 GO-5 (W10 말): 실제 사례를 재현하는가

**[설계 제안] 측정.** 위 사례마다 다음을 적는다.

- 분석기 판정과 수준.
- L2 시도 결과: 실행 구성(`env.json`), 반복 횟수, 관측 횟수.
- 분석기가 보고한 위치와 공개 기록 위치의 일치 여부.

| 판정 | 조건 **[설계 제안]** | 다음 행동 |
| --- | --- | --- |
| GO | 공개 기록에서 파생한 사례(CF #184 또는 #198 파생) 중 하나 이상이 L2에 도달한다. 분석기가 그 사례를 같은 위치의 L0·L1 후보로 냈다. 정상 대조는 조용하다. | 단계 3 진행 |
| PIVOT | 어떤 공개 기록 파생 사례도 시간 제한 안에 L2에 도달하지 못한다. 옛 버전 build 실패도 여기에 해당한다. | F5 결과를 L1까지만 보고한다. 역사 사례는 "source 수준 수정 전후 대조"로 적는다. 합성 사례의 L2는 "합성"으로 따로 센다. |
| STOP | ES 모델이 W9 적합성 시험(c)을 통과하지 못한다. | F1·F5에서 L0보다 강한 판정을 하지 않는다. startup 주장은 "후보 목록"으로만 낸다. |

**[해석]** "실제 사례를 하나도 재현하지 못하면" 연구가 끝나는 것은 아니다. 다만 그 범주의 결론이 "모델상 가능"으로 약해진다. F1과 F4에는 공개 실패 보고가 없었다 (§8.5, 검색 범위 안의 결과). 그래서 이 두 범주의 L2는 처음부터 합성·주입 사례에서만 나온다.

### 9.5 단계 3 (W11–W12): 다중 stream 일관성, freshness, 메시지 간 순서

#### 9.5.1 범위

| 항목 | 단계 3에서 켜는 것 **[설계 제안]** | 근거 |
| --- | --- | --- |
| 도메인 | Joint의 `upd[β]`(이벤트 경계 층). 계약이 있을 때만 timestamp 층. Origin `Time(cfe)`. | §6.3.3. **[확인된 사실]** 기본 origination action은 `IsOrigination=true` telemetry의 header 시각을 송신 시각으로 덮는다 ([cfe_msg_integrity.c L30–L53](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/msg/fsw/src/cfe_msg_integrity.c#L30-L53)). 그래서 header 시각 비교는 측정 시각 guard로 인정하지 않는다. |
| HB | HB-FIFO. 조건은 같은 sender, 같은 pipe, `Pub(m2)` 단일 site, OSAL POSIX 또는 RTEMS다. 가정 `NoDrop(p, m1)`을 쓴다. HB-ATX는 cFE 버전이 `550e7f7d` 이전일 때만 켠다(기본 꺼짐). | §6.2. **[확인된 사실]** mutex 밖 put은 `550e7f7d`(v7.0.0)부터다 (검증 C10). |
| 용량 매개변수 | `msglim(MID, pipe)`, `depth_eff(pipe)` | §3 M4. **[실측]** non-root 실행은 depth 20을 10으로 조용히 줄였다 (§3.2 OS-2). |
| Req | R-F3, R-F4, R-F7c, R-F7i | §6.6 |
| 계약 문법 | `requires_same_instance`, `requires_same_cycle … boundary`, `requires_updated_since`, `timestamp … kind=measurement`, `order(c, m1, m2)` | §6.6 |

**[원노트 구상]** 원노트의 `20 ms`, `100 ms`는 기준값으로 쓰지 않는다 (수정본 §6.5). 계약이 없으면 시간 기반 경고를 만들지 않는다.

#### 9.5.2 주별 작업

| 주 | 작업 **[설계 제안]** | 종료 기준 **[설계 제안]** |
| --- | --- | --- |
| W11 | 위 범위를 구현한다. 합성 사례 SYN-F3-01, SYN-F4-01, SYN-F4-02를 분석한다. SYN-ORD-01을 추가한다. SYN-ORD-01은 이 절에서 제안하는 F7 사례다. relay 앱이 원 메시지를 받은 handler 안에서 결과 메시지를 보내고, 관찰자 앱이 두 메시지를 다른 pipe에서 받는다. | (a) SYN-F4-02 함정에서 `(k, j)`가 생기지 않는다. (b) SYN-F4-01 faulty는 `requires_same_instance` 계약 위반 후보다. correct(한 packet 공동 갱신, 또는 `Att.Seq == Pos.Seq` 검사)는 후보가 없다. benign은 허용이다. (c) SYN-ORD-01에서 HB-FIFO가 적용되지 않음을 보이고, R-F7i 후보를 낸다. |
| W12 | 공개 앱 주입 INJ-HK-2, INJ-LC-1, INJ-LC-2를 분석한다. harness table 세 가지를 데이터로 둔다: SCH 항목 추가, LC WDT·ADT 정의, HK copy table 입력 선택. Ogma를 실행할 수 있으면 INJ-OG-1도 한다. F4 관련 문헌 확인 결과를 받는다 (§9.12). | GO-6 |

**[확인된 사실]** 기본 bundle의 SCH_LAB table에는 `LC_SAMPLE_AP_MID`도 `HK_SEND_COMBINED_PKT_MID`도 없다. LC watchpoint 항목도 모두 `LC_DATA_WATCH_NOT_USED`다 (§8.4.1). **[해석]** 그래서 W12의 harness table 없이는 HK·LC의 freshness 경로가 실행되지 않는다. harness table도 분석 입력이다. 정답 기록에 commit과 함께 저장한다.

**[실측] SYN-ORD-01의 L2 근거.** 같은 구조의 probe가 이미 있다. 관찰자 pipe가 먼저 구독하고 relay pipe가 나중에 구독했다. 1 CPU·SCHED_RR에서 relay가 송신자보다 priority가 높으면 역전은 500/500(두 실행)이었다. relay가 낮으면 0/500이었다. 4 CPU에서는 두 설정 모두 0/500이었다 (§3.1 SB-4, 검증 C10). **[해석]** 다른 구독 순서는 시험하지 않았다. 그래서 SYN-ORD-01은 구독 순서를 변형 요소로 기록한다 (§8.7.2).

#### 9.5.3 관문 GO-6 (W12 말): 허용된 조합과 결함을 가르는가

**[설계 제안] 측정.**

- (a) SYN-F3-01, SYN-F4-01, SYN-ORD-01, INJ-HK-2, INJ-LC-1, INJ-LC-2의 판정. 계약이 있을 때와 없을 때를 모두 잰다.
- (b) 공개 앱 5개의 소비 위치 중 `NonRel`로 사영된 비율. K=16 기준이다.
- (c) 정상 대조 두 가지(HK 기본 정책, Ogma passive 입력)의 판정.
- (d) §4.7.3의 F4 관련 미확인 항목의 상태. 대상은 Artho 등 HLDR, ATVA 2004, time-disparity 문헌이다.

| 판정 | 조건 **[설계 제안]** | 다음 행동 |
| --- | --- | --- |
| GO | 계약이 있을 때 faulty·correct·benign을 모두 기대대로 가른다. HK·Ogma 정상 대조가 "허용"이고 출처가 붙는다. | 단계 4 진행 |
| PIVOT-1 (§4.7.2 N4의 반박 조건) | 계약 없이는 faulty와 benign이 구별되지 않는다. | 기여를 "계약 검사"로 줄여 쓴다. guard 인식의 기여는 계약이 없는 정상 대조(TO_LAB, SCH_LAB guard)에서만 주장한다. |
| PIVOT-2 (N5의 반박 조건) | HLDR·time-disparity 문헌이 F4를 이미 정의하거나, 문헌을 아직 읽지 못했다. | F4를 새 범주로 주장하지 않는다. 기존 정의의 cFS 적용으로 쓴다. |
| STOP | K=16에서도 소비 위치의 절반 넘게 `NonRel`로 사영된다. 이 임계값은 설계 제안이다. | F4 판정을 `unknown`으로 보고하고 한계로 적는다. F3의 이벤트 경계 판정은 따로 유지할 수 있다. |

### 9.6 단계 4 (W13–W14): Table Services와 재시작 창

#### 9.6.1 범위

| 항목 | 단계 4에서 켜는 것 **[설계 제안]** | 근거 |
| --- | --- | --- |
| op | `cfs.tbl.register`, `share`, `unregister`, `load`, `get_address`, `release_address`, `manage`, `update`, `get_status`, `get_info` (§5.5.5). `cfs.es.restart_app`, `cfs.es.get_app_id` | §5 |
| 도메인 | `Aux.tblPtr ∈ {PROTECTED, UNPROTECTED, RELEASED, NULL}`, M10의 `desc(h)`·`table(t)`, Origin `Tbl(table, path)` | §3 M10, §6.3.4 |
| HB | HB-TACT(`Update`·`Manage` 경로), HB-TREG(cFE ≥ v7.0.0), HB-RST(가정 `CoopExit`) | §6.2 |
| Req | R-F6(typestate), R-F8 | §6.6 |
| 버전 매개변수 | `NEVER_LOADED`의 포인터·lock 동작, 재시작 때 table 이름 처리 | **[확인된 사실]** 546a002에서 `NEVER_LOADED`는 NULL을 돌려주고 lock을 걸지 않는다 ([cfe_tbl_registry.c L192–L196](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/tbl/fsw/src/cfe_tbl_registry.c#L192-L196)). v6.7.0, v6.8.0-rc1, draco-rc5는 lock을 걸고 0으로 채운 buffer 포인터를 돌려주었다 (§3.6, 검증 C13). v6.7.0부터 draco-rc5까지는 cleanup 때 table 이름을 지웠다. 현재 동작은 v7.0.0 commit c1ab1b7부터다 (§1.2 F8, 검증 C15). |

#### 9.6.2 주별 작업

| 주 | 작업 **[설계 제안]** | 종료 기준 **[설계 제안]** |
| --- | --- | --- |
| W13 | 위 범위를 구현한다. bundle의 OSAL·PSP 고정값으로 §8.3.4의 TBL·재시작 네 줄을 다시 실행한다. 이 절은 단일·이중 buffer 줄을 둘로 나눠 다섯 시험으로 센다. 작성자 B는 W13 초에 TBL 사례 정답을 동결한다. | 모델이 다섯 시험의 실측을 모두 예측한다 (아래 표). |
| W14 | 사례를 분석한다: SYN-F6-01·02·03, SYN-F8-01·02, INJ-SA-1·2, INJ-HK-1. 역사 사례도 다룬다: sample_app #28→#101, HS #148, MD #79, cFE #1750. | GO-7 |

**[설계 제안] TBL 적합성 시험.** 다섯 시험은 §8.3.4 표의 `NEVER_LOADED`, handle 단위 lock, 단일 buffer, 이중 buffer, owner 재시작이다. 모델이 낼 결론과 실측 근거(로그 줄, 상태 코드, 재시작 시간 1.57–1.63 s)는 그 표에 있다 (검증 C13–C15).

| 역사 사례 | 공개 자료 | 계획한 사용 **[설계 제안]** |
| --- | --- | --- |
| sample_app #28→#101 | **[확인된 사실]** 693d75f [S59]가 `INFO_UPDATED` 경로의 release 누락을 만들었다. 61f657d [S60]가 고쳤다. | F6 typestate 사례. 같은 시기 cFE(v6.7·6.8 계열)가 필요하다. **[미확인]** build 가능 여부. 시간 제한 2일. 단일 앱 API 오용이므로 앱 간 race로 세지 않는다. |
| HS #148 | **[확인된 사실]** 수정 b7530d9 [S51]. 재현법은 `hs_amt.tbl`을 지우고 enable 명령을 보내는 것이다. | `Tbl` 출처의 F2. 결정적 결함이므로 순서 결함 집계와 분리한다. |
| MD #79 | **[확인된 사실]** HEAD `65eb7b3`의 file load 경로가 `MD_CopyUpdatedTbl`을 부르지 않는다 ([md_app.c L423–L449](https://github.com/nasa/MD/blob/65eb7b3b0aa8acd05076128a623cd696582b6d7c/fsw/src/md_app.c#L423-L449)). | 현재 결함(open), 결정적. 분리 집계 |
| cFE #1750 | **[확인된 사실]** open: 잠긴 table에 load하면 `LoadInProgress`가 남는다 (§2.4.2). | **[미확인]** 546a002에서 현존 여부. 먼저 확인하고 사례로 쓴다. |

#### 9.6.3 관문 GO-7 (W14 말): TBL 결과를 보고할 수 있는가

| 판정 | 조건 **[설계 제안]** | 다음 행동 |
| --- | --- | --- |
| GO | TBL 적합성 다섯 시험을 모두 예측한다. W14 사례가 동결 정답과 같다. 순서 결함과 API 오용이 따로 집계된다. | 단계 5 진행 |
| PIVOT | owner `CFE_TBL_Load`가 registry lock을 일찍 놓는 구간(TBL-6, **[미확인]**)이 판정에 걸린다. | 그 구간에 닿는 후보는 `unknown`으로 둔다. F6는 typestate(API 오용)로만 보고하고 race로 세지 않는다. |
| STOP | 적합성 시험이 실패한다. | TBL 결과를 보고하지 않는다 (§8.3.4의 원칙). F8은 SB 쪽 재시작 창(SYN-F8-02)만 남긴다. |

### 9.7 단계 5 (W15–W16): 통합 평가와 주장 정리

#### 9.7.1 W15: 전체 실행, 기준선, ablation

| 작업 **[설계 제안]** | 내용 | 주의 |
| --- | --- | --- |
| 수정하지 않은 bundle 전체 실행 | SBN·CF를 포함한다. indirect call은 상수 table 해석 뒤 남은 것에 `unknownCallee`를 붙인다 (§6.5.5). 경고를 §8.4.3의 다섯 분류로 나눈다. | 정답 집합이 없으므로 recall을 계산하지 않는다. |
| B0, B1 | B0은 MLIR `CallGraph`, B1은 자체 추출의 topology 간선만 쓴다. | B1은 추가 비용이 거의 없다. |
| B2 (CodeQL + 같은 SB 모델) | `isAdditionalFlowStep`으로 같은 상수 MID의 transmit→receive 간선을 넣는다. F1·F2·F6 query를 쓴다. | **[확인된 사실]** CodeQL의 C/C++ global data flow는 함수 사이 전역을 실행 순서와 무관하게 잇는다 (§4.5.2). **[미확인]** CLI 설치 크기와 licence를 확인하지 않았다. 디스크 여유가 2.9 GB이므로 설치 전에 크기를 확인한다. |
| B4 (ROSInfer 방식 휴리스틱) | publish 조건에 쓰이고 publish 함수 밖에서 대입되는 변수를 상태 변수로 본다. | **[확인된 사실]** ROSInfer는 메시지 내용을 모델링하지 않는다 (검증 C02). |
| 선택 기준선 | B3(CSA custom checker), B6(Ogma), B7(TSan), B8(Goblint) | 시간이 남을 때만. B7·B8은 cFE #950, #46 같은 공유 메모리 대조 사례에만 쓴다. |
| ablation | 도메인 AB1–AB10을 flag로 끈다. 구현 기반 I1–I5를 바꾼다. I7(ClangIR)은 CIR을 켠 build가 있을 때만 한다. | **[실측]** local clang은 CIR을 쓸 수 없다 (검증 C19). |

**[설계 제안]** B2 구현에도 E-dialect의 단계별 구현과 같은 시간 예산을 정하고 기록한다 (§8.11 기준선 공정성).

#### 9.7.2 W16: 관문 GO-8과 산출물

**[설계 제안] GO-8 규칙.** 주장 하나(§4.7.2의 N1–N6, §1.6의 RQ1–RQ6)는 세 조건을 모두 만족할 때만 결과 절에 쓴다.

1. 증거 수준이 적혀 있다. L2가 없으면 "L1까지"라고 적는다.
2. 같은 SB 모델을 쓴 B2가 같은 후보 집합을 내지 않았다. 이것이 §4.7.2 N3의 반박 조건이다.
3. 그 주장에 걸린 §4.7.3의 미확인 문헌을 닫았다.

조건을 만족하지 못한 주장은 "검증할 가설"이라는 문구로 남긴다.

| 산출물 **[설계 제안]** | 내용 |
| --- | --- |
| 주장–근거표 | N1–N6, RQ1–RQ6마다 판정, 증거 수준, 사례 ID, 기준선 비교 |
| 층별 결과표 | 합성·주입·역사·수정하지 않은 bundle을 나눈다. 범주별 TP/FP/FN, unknown 수, Σ, cFE 버전을 싣는다. |
| 공개 artifact | `pin.lock`, `env.json`, 동결된 정답과 계약, 주입 patch, harness table, 분석기 소스, 기준선 query |
| 위협 정리 | §8.11에 실제로 일어난 일을 더한다. |

### 9.8 단계별 활성화 요약

| 단계 | API·op | 도메인 성분 | HB 규칙 | Req 규칙 | 사례 | 모델 적합성 |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (W3–W8) | SB·MSG 전부, ES 대기(구문만) | Prov₁(asg, cls, val), Mem, Inp, Val, bufOf, status | HB-PO, HB-MSG, HB-CORE, HB-LIB | R-F2, R-F1c | SYN-F2-01, SYN-F2-02, SYN-F1-01(L0만), §44 예, SYN-F4-02 함정 | SB 다섯 줄 |
| 2 (W9–W10) | ES 의미 | + subs, readyMark | + HB-SYS1, HB-SYS2, HB-OSAL | + R-F1i, R-F5 | SYN-F1-01, SYN-F5-01, CF #184, #198 파생, INJ-TO-1, #73(정적) | OPERATIONAL 의미 |
| 3 (W11–W12) | (변화 없음) | + upd[β], timestamp 층, Time | + HB-FIFO, (HB-ATX: 옛 버전만) | + R-F3, R-F4, R-F7c, R-F7i | SYN-F3-01, SYN-F4-01, SYN-ORD-01, INJ-HK-2, INJ-LC-1/2, (INJ-OG-1) | relay 역전, MsgLim·depth |
| 4 (W13–W14) | TBL 전부, restart | + tblPtr, Tbl | + HB-TACT, HB-TREG, HB-RST | + R-F6, R-F8 | SYN-F6-01–03, SYN-F8-01/02, INJ-SA-1/2, INJ-HK-1, 역사 4건 | TBL·재시작 다섯 시험 |
| 5 (W15–W16) | 전체 | 전체 | 전체 | 전체 | 수정하지 않은 bundle, 기준선, ablation | — |

### 9.9 관문 종합

| 관문 | 시점 | 질문 | GO | PIVOT | STOP |
| --- | --- | --- | --- | --- | --- |
| GO-1 | W2 | 입력이 재현되는가 | W1 기준 1–4 | `-O1` 실패 파일만 `-O0` 경로로 | — |
| GO-2 | W4 | MID를 정적으로 회수하는가 | 23/23, table 값 집합, 47곳 분류 | A: MID를 AST sidecar로. B: table MID는 `pub_complete=false`, F1·F7 범위 축소 | RQ1 철회, 앱 내부 분석으로 축소 |
| GO-3 | W8 (최종은 W16) | MLIR이 AST·E-llvm보다 무엇을 더하는가 | G3-b 또는 G3-c 우위 | 1: 추출은 MLIR 근거에서 뺀다. 2: E-llvm 방식으로 전환 | MLIR 기반 철회, 원노트 §43 조건 성립 보고 |
| GO-4 | W8 | MVP가 성립하는가 | 기준 1–5 | guard 인식 보강, #198 파생 제외 | RQ2 철회, topology·API 검사로 축소 |
| GO-5 | W10 | 실제 startup 사례를 재현하는가 | 공개 기록 파생 사례 하나 이상 L2 | F5는 L1까지, 역사 사례는 정적 대조 | ES 모델 부적합: F1·F5는 L0만 |
| GO-6 | W12 | 허용된 조합과 결함을 가르는가 | 계약으로 분리, 정상 대조 허용 | 1: 계약 검사로 축소(N4). 2: F4를 새 범주로 주장하지 않음(N5) | `NonRel` 과반: F4는 unknown |
| GO-7 | W14 | TBL 결과를 보고할 수 있는가 | TBL 적합성 다섯 시험, 동결 정답 일치 | TBL-6 구간은 unknown, F6은 typestate로만 | TBL 결과 미보고 |
| GO-8 | W16 | 무엇을 주장하는가 | 세 조건 만족 | 해당 주장은 "가설"로 | — |

### 9.10 위험과 완화

| ID | 위험 | 근거 | 영향 받는 주 | 완화 **[설계 제안]** | 발동 신호 |
| --- | --- | --- | --- | --- | --- |
| RK-1 | 디스크 부족 | **[실측]** 여유 2.9 GB, 작업 공간 2.3 GB, bundle build tree 하나에 254 MB (§9.0.2) | W1, W10, W14, W15 | live build tree는 하나만 둔다. IR·MLIR·log만 남긴다. 중복 tarball을 정리한다. 역사 버전은 한 번에 하나씩 build하고 지운다. CodeQL은 설치 전에 크기를 확인한다. | 주 시작 `df`에서 Avail < 1 GB |
| RK-2 | MLIR API 차이와 local 기능 부족 | **[실측]** local에는 `isAddressable`이 없다 (§9.0.2). PoTATo가 local LLVM에 대해 build되지 않았다 (`$N/rw-mlir/potato_build.log`). | W3, W6 | `1053047a`에 고정한다. SB·TBL effect는 분석기가 직접 해석한다. main 전용 기능에 기대지 않는다. ODS build와 dense 골격은 이미 local에서 작동했다 (§9.0.2). | 새 upstream 기능이 필요해지는 설계 변경 |
| RK-3 | MID가 table·명령·상태에서 온다 | **[확인된 사실]** 구독 site 47곳 중 8곳 (검증 C16) | W4 | P7과 `pub_complete` 표시. GO-2의 PIVOT-B. | GO-2 측정 2 실패 |
| RK-4 | field 인식 공백 | **[실측]** offset 0, out-parameter, 지역 포인터를 놓친다. sch_lab은 0곳이다 (§7.8). | W5 | byte 범위로 offset 0을 식별한다. API 모델로 out-parameter를 쓴다. 지역 포인터는 SSA와 summary로 추적한다. 남는 것은 unknown으로 센다. | W5 기준 실패 |
| RK-5 | 간접 호출 | **[실측]** 73곳 중 SBN 계열 30, CF 16, ES pool 9 (§6.5.5). MLIR `CallGraph`는 `<Unknown-Callee-Node>`만 남긴다 (검증 C20 (d)). | W15 | MVP에서 SBN·CF를 뺀다. 상수 table을 해석한다. 외부 points-to(SVF)는 **[미확인]**이므로 선택 사항으로 둔다. | 전체 실행의 `unknownCallee` 수 |
| RK-6 | 실행 환경 제약 | **[실측]** host IPC의 root는 abort한다. non-root는 depth를 10으로 줄이고 priority를 조용히 버린다 (§8.7.1). | W8, W10, W12, W13 | §8.7.1의 구성만 쓴다. 모든 결과에 `env.json`을 붙인다. 구성이 다른 결과는 합치지 않는다. | `env.json` 누락 |
| RK-7 | 재현의 비결정성 | **[실측]** relay 역전이 1 CPU에서 500/500, 4 CPU에서 0/500이었다 (검증 C10). | W10, W12 | §8.7.2의 순서 강제 방법을 쓴다. 반복 횟수를 미리 정한다. 관측 횟수를 그대로 보고하고 확률로 바꾸지 않는다. | 같은 구성에서 결과가 갈림 |
| RK-8 | 옛 버전 build 실패 | **[미확인]** 6.4.1, 6.5.0a, 6.6.0a, v6.7–6.8 계열을 gcc 13.3으로 build할 수 있는지 확인하지 않았다. | W10, W14 | 버전마다 2일 시간 제한을 둔다. 실패하면 source 수준 수정 전후 대조로 끝낸다. 파생 예제는 "파생"으로 표시한다 (수정본 §5.3). | 시간 제한 초과 |
| RK-9 | cFE 버전마다 의미가 다르다 | **[확인된 사실]** `550e7f7d`(lock 위치), `c1ab1b7`(TBL 재등록), `d3d52da`(TO_LAB 지연), `NEVER_LOADED` 동작 (검증 C10, 검증 C13, 검증 C15, 검증 C16) | W10, W13, W14 | 규칙마다 버전 조건을 둔다 (§6 AP6). 역사 사례는 그 버전의 의미로 분석한다. | 버전 조건이 없는 규칙 |
| RK-10 | 정답·계약의 순환 | §8.11 | W2, W7, W9, W11, W13 | 작성자 B가 분석기 실행 전에 동결한다. 앱 자체의 주석·설정을 먼저 출처로 쓴다. | 동결 뒤 정답 수정 |
| RK-11 | 약한 기준선 | §8.11. B2·B3를 연구자가 쓴다. | W15 | 같은 SB 모델을 쓴다. 같은 시간 예산을 둔다. query와 checker를 공개한다. | B2 구현 시간이 예산의 절반 미만 |
| RK-12 | 정밀도 손실 | **[설계 제안]** context-insensitive 합류는 상관을 깬다 (§6.5.2). Disj가 K를 넘으면 `NonRel`이 된다. | W6, W12 | `setInterprocedural(false)`와 summary. `lostAt`을 보고한다. K를 sweep한다. | GO-6 STOP 조건 |
| RK-13 | 선행연구 미확인 | **[확인된 사실]** R03 본문, SPLC 2009, WCRE 2010, HLDR, ROSInfer 학위논문 등이 snippet 수준이다 (§4.7.3). 이 컨테이너에서는 NTRS·IEEE·ACM·arXiv 등이 막혀 있다 (`$N/rw-flight/access_log.txt`, `$N/rw-races/sources_manifest.tsv`). | W12, W16 | §9.12의 일정으로 다른 네트워크·도서관·저자 요청을 쓴다. 닫기 전에는 신규성을 주장하지 않는다. | GO-6, GO-8의 문헌 조건 |
| RK-14 | 범위 확장 | 원노트 §59, §1.4 | 전체 | SBN 원격 구독, EDS, RTOS, CIR은 범위 밖이다. CIR은 I7으로만 두고, CIR을 켠 build가 있을 때만 한다. 그 build 비용은 **[미확인]**이다. | 범위 밖 작업에 하루 넘게 씀 |
| RK-15 | 1인 일정 지연 | 임계 경로 W3–W6 (§9.1) | 전체 | 주마다 종료 기준을 확인한다. 2주 이상 밀리면 §9.11 축소안으로 바꾼다. | 종료 기준 미달 2주 누적 |

### 9.11 12주 축소안

**[설계 제안]** 단계 1(MVP)은 줄이지 않는다. 임계 경로이고, 이후 모든 단계가 그 산출물을 쓰기 때문이다.

| 주 | 16주안 대비 변화 |
| --- | --- |
| W1–W2 | 같음 |
| W3–W7 | 단계 1. GO-3에서 E-ast를 빼고 E-dialect와 E-llvm만 비교한다. W7에 GO-3·GO-4를 함께 닫는다. |
| W8–W9 | 단계 2. 역사 사례는 CF #184만 다룬다. #198 파생과 #73 정적 대조는 뺀다. |
| W10 | 단계 3. SYN-F4-01, SYN-F4-02, INJ-HK-2만 다룬다. SYN-F3-01, SYN-ORD-01, INJ-LC-*, INJ-OG-1은 뺀다. |
| W11 | 단계 4. 적합성 시험, SYN-F6-01·02, INJ-SA-1·2만 다룬다. F8(재시작 창)은 후속 과제로 넘긴다. |
| W12 | 단계 5. B0·B1·B2와 ablation AB1–AB5, I1–I3만 한다. GO-8을 닫는다. |

**[해석]** 축소안에서는 F3, F7, F8의 결과가 없다. 그래서 논문의 범주 범위는 F1, F2, F4, F5, F6로 줄어든다. 줄인 범위를 결과 절에 명시해야 한다.

### 9.12 병행 작업: 문헌 닫기와 독립 동결

**[설계 제안] 문헌 닫기.** §4.7.3의 미확인 항목을 필요한 관문 순으로 둔다.

| 항목 | 이유 | 필요 시점 |
| --- | --- | --- |
| Artho·Havelund·Biere HLDR(STVR 2003), ATVA 2004, time-disparity 문헌, AUTOSAR TIMEX | F4의 신규성(N5) | GO-6 (W12) |
| typestate·API protocol 문헌 | F6의 위치 | GO-7 (W14) |
| R03(ISSRE 2016) 본문 | N1. SB 모델이 앱 순서나 state를 다루는지 | GO-8 (W16) |
| SPLC 2009 본문, FSW-08 SAVE 발표, WCRE 2010 본문 | RQ1·N2. MID·pipe 간선을 복원했는지 | GO-8 |
| ROSInfer TLA+ 생성기 코드와 2025 학위논문 | N3. 구독 전 발행·timing 확장이 이미 있는지 | GO-8 |
| NASA IV&V 2020 보고서 §2.1–§2.2 원문 | 순서·startup 결과를 보고했는지 | GO-8 |

**[확인된 사실]** 이 컨테이너에서는 NTRS, IEEE, ACM, Springer, arXiv 등 주요 host가 막혀 있었다 (`$N/rw-flight/access_log.txt`, `$N/rw-races/sources_manifest.tsv`). **[설계 제안]** 그래서 W2에 다른 네트워크, 기관 도서관, 저자 사본 요청을 시작한다.

**[설계 제안] 독립 동결 일정.** 작성자 B가 다음 시점에 정답과 계약을 동결한다.

| 시점 | 동결 대상 |
| --- | --- |
| W2 | `truth/extract_23.json`, `truth/normal.yaml` |
| W7 초 | 합성 앱 계약, SYN-F2-* 정답, dispatch 손 정답표(W5에 미리) |
| W9 초 | SYN-F1-01, SYN-F5-01, CF #184, #198 파생, INJ-TO-1 정답 |
| W11 초 | SYN-F3/F4, SYN-ORD-01, INJ-HK-2, INJ-LC-* 정답과 harness table |
| W13 초 | TBL·재시작 사례 정답 |

### 9.13 원노트·수정본 계획과의 대응

| 원노트·수정본 | 이 절의 위치 | 바꾼 점 |
| --- | --- | --- |
| 원노트 §58 P1 API lifting | W3 | 네 함수에서 §9.3.1의 MVP 집합으로 넓혔다 (근거 §7.13.2). |
| P2 Message graph | W4 | table·명령 MID를 P7로 넣고, 관문 GO-2를 두었다. |
| P3 State provenance | W5–W6 | field 인식 공백과 상관 보존 도메인을 종료 기준으로 만들었다. |
| P4 Basic HB | W7 | `G0` 규칙 네 개로 시작한다. |
| P5 Fault injection | W7(합성), W12·W14(공개 앱 주입) | 모든 사례에 정상·허용·조건부 변형을 둔다 (§8.3.2). |
| P6 Startup semantics | W9–W10 | 조건부 간선과 실제 사례 관문 GO-5를 두었다. #73은 정적 대조로만 다룬다. |
| P7 Temporal contract | W11–W12 | 이벤트 경계 층을 기본으로 한다. 시간 값은 계약이 있을 때만 쓴다. |
| P8 Table lifecycle | W13–W14 | 구현 기준의 TBL 의미와 버전 조건을 쓴다. |
| 원노트 §63 Step 1 관련연구 scan | §9.12 병행 | 관문별 필요 시점을 두었다. |
| Step 2 API inventory | W2 | `api_model.yaml` |
| Step 3 taxonomy | W2에 §1.2(F1–F8) 확인 | — |
| Step 4 최소 test app 3개 | W7 | §8.3.1의 SENSOR·NAV·CONTROL과 GAINTBL(W13) |
| Step 5 MLIR representation | W3 | §5의 ODS 초안을 구현한다. |
| Step 6 첫 분석 use-before-init | W6–W7 | R-F2 |
| 수정본 §11.1의 일곱 단계 | W1–W14 | 단계마다 산출물, 종료 기준, 관문을 붙였다. |
| 수정본 §13.4의 "이어서 할 작업" 1–7 | §9.12(1), W1(2), W3–W5(3), W7(4), W8(5), W10(6), GO-5–GO-8(7) | — |

### 9.14 이 절이 주장하지 않는 것

- **[설계 제안]** 일정, 임계값(K 값, "과반", 시간 제한 2일·3일), 인력 배분은 계획이다. 실제 소요를 측정한 것이 아니다.
- **[실측]** §9.0.2의 build와 실행은 API 호환성 확인이다. 제안 분석의 구현이나 결과가 아니다.
- **[미확인]** 옛 cFE 버전의 build 가능성, CodeQL CLI의 크기·licence, Ogma 실행 가능성, SVF·Goblint의 동작은 확인하지 않았다.
- **[해석]** 관문을 모두 GO로 통과해도 비행 결함 검출 성능을 뜻하지 않는다. 결과는 고정한 공개 bundle, native Linux 실행, 합성·주입 사례의 범위에 한정된다 (수정본 §11.4, §8.12).

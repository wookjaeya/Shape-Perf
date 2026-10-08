## 5. cfs dialect와 lifting 설계

이 절은 원노트 §11의 `cfs` dialect 구상을 타입, 속성, op, effect, verifier 규칙으로 구체화한다. 이어서 각 op를 C 코드에서 어떻게 인식하는지 적는다. frontend마다 잃는 정보와 되찾는 방법도 정리한다. 마지막으로 NASA sample_app 코드를 이 dialect로 올린 예제를 보인다.

**[설계 제안]** 이 절의 dialect, pass, 규칙은 모두 설계 제안이다. 구현된 것은 없다. 탐지율이나 분석 결과도 없다.

이 절의 실측은 세 종류다. 첫째, frontend 출력에 필요한 정보가 남아 있는지 측정했다. 둘째, IRDL로 dialect의 구조만 선언하고, 손으로 쓴 lifted IR을 그 선언으로 검사했다. 셋째, inlining 범위를 바꿨을 때 정보가 어떻게 달라지는지 측정했다.

**[원노트 구상]** 원노트 §11은 `cfs.sb.*`, `cfs.app*`, `cfs.state.*`, `cfs.tbl.*`, `cfs.lifecycle.*`, `cfs.sync.*`라는 op 이름을 제안했다. 수정본 §9.1은 이 이름들을 검증되지 않은 설계로 분류했다. 이 절은 그 이름에서 출발한다. 다만 op 하나를 cFE API 하나에 대응시키고, 측정으로 확인한 frontend 사실에 맞춘다. 대응 관계는 §5.5.8에 있다.

### 5.0 근거, 고정 버전, 산출물

| 항목 | 값 | 근거 |
| --- | --- | --- |
| cFE | `546a002515be5a1e3b66f9ae2c14f948d9cec76f` | **[확인된 사실]** commit record [S25]. cFS bundle `5a9b075c`가 이 commit을 고정한다 (`$N/probe/submodules.txt`, §3.0). |
| sample_app | `199476a34827ae84d50d66f97619227854cd971a` | **[확인된 사실]** bundle `5a9b075c`의 submodule 고정값이다. 수정본의 S21은 `dev`의 `b07a4317`을 기록했다. 이 절의 줄 번호는 모두 `199476a3` 기준이다. |
| to_lab, sch_lab | `d27c6014`, `607e2f90` | **[확인된 사실]** 같은 bundle의 submodule 고정값이다. |
| LLVM·MLIR | local `1053047a` (읽기 전용), upstream main `ccac700c`과 대조 | **[실측]** `/home/user/work/llvm-project/build/bin`의 clang, opt, llvm-link, mlir-translate, mlir-opt |
| 작업 디렉터리 | `$W` = `$N/s05_dialect` | `$N`은 문서 머리의 정의를 따른다. |

| `$W`의 산출물 | 내용 |
| --- | --- |
| `sample_app_linked_o1np.ll` | sample_app의 4개 TU를 `-O1 -Xclang -disable-llvm-passes -g`로 컴파일한 probe 출력(`$N/probe/ir_o1np/sample_app/`)을 `llvm-link`로 합친 것 |
| `linked.raw.mlir`, `linked.opt.mlir` | 위 파일의 import 결과와, `--inline --sroa --mem2reg --canonicalize --cse`를 돌린 결과 |
| `linked_noinl.ll`, `linked_noinl.opt.mlir` | 앱 함수 12개에 `noinline`을 붙인 뒤 같은 pipeline을 돌린 결과 (§5.8 P0) |
| `api_sites.py`, `resolve_fields.py`, `dedupe_sites.py`와 `.out` | API 호출 위치, 전역 field 접근, 위치 기준 중복 제거를 세는 regex probe. 분석기가 아니다. |
| `cfs_irdl.mlir` | IRDL로 쓴 dialect 구조 선언. effect는 표현하지 못한다. |
| `sample_app_lifted.mlir` | 손으로 쓴 lifting 목표 IR. pass의 출력이 아니다. |
| `neg1.mlir`, `neg2.mlir`, `neg3.mlir`, `neg_sig.mlir`, `neg.out` | 구조 검사의 음성 시험 |

### 5.1 설계 원칙

**D1. overlay.** **[설계 제안]** cfs op는 새 함수 형식을 만들지 않는다. import된 `llvm.func` 몸체 안에서 `llvm.call @CFE_*`를 같은 위치의 cfs op로 바꾼다. load, store, 분기, 산술은 LLVM dialect 그대로 둔다. **[확인된 사실]** 근거는 import 경로가 이미 작동한다는 것이다. cFE module 5개와 앱 디렉터리 17개, 모두 154개 파일이 진단 0개로 import되었다. 단 이 결과는 clang으로 설정한 build tree에서 `-Werror`를 뺀 조건의 것이다 (조건은 §7.5.2, 검증 C17 정정 문구). 공식 문서 [S16]는 LLVM IR에서의 번역을 experimental subset으로 기술한다. 이 범위의 cFS 코드에서는 그 제한이 import를 막지 않았다 (§7.5). **[해석]** C 의미를 새 dialect로 다시 정의하면 오류 가능성만 늘어난다.

**D2. out-parameter 명시화.** **[설계 제안]** API가 포인터 인자로 값을 돌려주면 그 값을 op의 SSA 결과로 꺼낸다. 바로 뒤에 원래 포인터로 raw 값을 쓰는 op를 둔다. 대상이 앱 전역 field면 `cfs.state.update`이고, 지역 변수면 `llvm.store`다. **[확인된 사실]** §7.8의 실측은 `CFE_SB_CreatePipe(&SAMPLE_APP_Data.CommandPipe, …)`와 `CFE_TBL_Register(&…TblHandles[0], …)` 같은 out-parameter 쓰기가 store로 보이지 않아 state 정의에서 빠진다고 보고했다.

**D3. handle은 함수 안에서만 타입을 갖는다.** **[설계 제안]** `!cfs.pipe`, `!cfs.tbl` 같은 handle 타입은 함수 몸체 안에서만 쓴다. 함수 경계와 메모리에서는 raw 정수로 남긴다. 둘 사이는 `cfs.handle.wrap`과 `cfs.handle.raw`로 바꾼다. **[실측]** `llvm.func @f(%p: !cfs.pipe …)`는 `custom op 'llvm.func' failed to construct function type: expected LLVM type for function arguments`로 거부된다 (`$W/neg.out`).

**D4. 거친 resource, operand가 정하는 정체성.** **[설계 제안]** SB route 표, SB queue, ES 상태, TBL registry, EVS 상태, perf log를 비주소(non-addressable) resource로 둔다. 어느 pipe·MID·table인지는 resource로 나누지 않는다. op의 operand와 속성이 그 정체성을 갖는다. **[확인된 사실]** main `ccac700c`의 header는 resource 위계가 "deliberately *not* intended for fine-grained regions with specific addresses/sizes, or for alias classes / offset-based disambiguation"이라고 적는다 ([SideEffectInterfaces.h L80-L87](https://github.com/llvm/llvm-project/blob/ccac700c68e92a7eabcb2f745789978213f16f2d/mlir/include/mlir/Interfaces/SideEffectInterfaces.h#L80-L87)). 비주소 resource effect에 Value를 붙이면 fatal error다 ([L339-L343](https://github.com/llvm/llvm-project/blob/ccac700c68e92a7eabcb2f745789978213f16f2d/mlir/include/mlir/Interfaces/SideEffectInterfaces.h#L339-L343)).

**D5. verifier와 lint를 나눈다.** **[설계 제안]** verifier는 IR 표현의 불변식만 검사한다. 상태 미검사나 구독 전 수신 같은 프로그램 성질은 별도 `cfs-lint` pass가 위치와 함께 보고한다. **[해석]** verifier 오류는 module 전체를 무효로 만든다. 실제 앱의 결함 후보 때문에 IR이 거부되면 분석 자체를 할 수 없다.

**D6. 위치를 보존한다.** **[설계 제안]** cfs op는 대체한 `llvm.call`의 Location을 그대로 갖는다. inlining이 만든 `callsite(… at …)`도 지우지 않는다. 코드 밖에서 온 선언(table, startup script)은 `FusedLoc`에 출처 file:line을 담는다. 실측 근거는 §5.7에 있다.

**D7. 모르는 것은 모른다고 적는다.** **[설계 제안]** MID, field, handler를 정하지 못하면 추측하지 않는다. `cfs.mid.from_value`와 `#cfs.mid_src`로 값의 출처 종류만 남긴다. 이후 분석은 그 값을 ⊤로 다룬다.

**D8. closed world.** **[설계 제안]** `cfs-close-world` pass가 entry가 아닌 함수에 `sym_visibility = "private"`을 붙인다. entry는 startup script의 entry 함수와, 주소가 API 인자로 넘어간 callback(예: `SAMPLE_APP_TblValidationFunc`)이다. **[확인된 사실]** importer는 visibility를 정하지 않는다. DeadCodeAnalysis는 public 함수의 predecessor를 미지로 본다 (검증 C20 (c), §4.5.3).

### 5.2 타입

| 타입 | C 원형 (cFE 546a0025) | raw 표현 | 의미와 생성 | 경계 규칙 |
| --- | --- | --- | --- | --- |
| `!cfs.pipe` | `CFE_SB_PipeId_t` = `CFE_RESOURCEID_BASE_TYPE` ([default_cfe_sb_extern_typedefs.h L112](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/sb/config/default_cfe_sb_extern_typedefs.h#L112)) | **[실측]** i32 (`SAMPLE_APP_Data_t`의 5번째 원소, byte offset 24) | **[설계 제안]** pipe 하나. `cfs.sb.create_pipe` 또는 `cfs.handle.wrap`이 만든다. | **[설계 제안]** 함수 경계·메모리에서는 i32 |
| `!cfs.msgid` | `CFE_SB_MsgId_t` (`Value` 하나를 가진 struct) | **[실측]** 호출 인자에서 i32로 coerce된다 (`$N/probe/ir/sample_app/sample_app.ll` L143-L149) | **[설계 제안]** MID 값. `cfs.mid.const`, `cfs.mid.from_value`, `cfs.msg.get_msgid`가 만든다. | **[설계 제안]** 메모리에서는 `struct<(i32)>` 그대로 둔다 |
| `!cfs.buf` | `CFE_SB_Buffer_t *` (ReceiveBuffer의 out-parameter) | ptr | **[설계 제안]** 수신 buffer의 수명 토큰. 포인터는 `cfs.sb.buf_ptr`로만 꺼낸다. | **[설계 제안]** 함수에는 꺼낸 ptr만 넘긴다. M6(§3.8)의 "다음 receive까지 유효" 검사를 위해 토큰을 따로 둔다. |
| `!cfs.tbl` | `CFE_TBL_Handle_t` | **[확인된 사실]** 기본 build는 `int16`이다. `CFE_OMIT_DEPRECATED_6_8`이면 `CFE_TBL_HandleId_t`다 ([cfe_tbl_api_typedefs.h L145-L170](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_tbl_api_typedefs.h#L145-L170)). **[실측]** 이 build에서 i16이다. | **[설계 제안]** table descriptor 하나. `cfs.tbl.register`, `cfs.tbl.share`, `cfs.handle.wrap`이 만든다. | **[설계 제안]** raw 폭은 module 속성 `cfs.abi`에 적고 verifier가 대조한다 |
| `!cfs.appid` | `CFE_ES_AppId_t` = `CFE_RESOURCEID_BASE_TYPE` ([default_cfe_es_extern_typedefs.h L314](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/es/config/default_cfe_es_extern_typedefs.h#L314)) | **[미확인]** sample_app은 쓰지 않아 IR 폭을 보지 않았다 | **[설계 제안]** 앱 인스턴스. `cfs.es.get_app_id`가 만든다. 재시작 뒤 같은 값이 유지되는지는 M9(§3.8)의 모델을 따른다. | **[설계 제안]** raw 정수 |

### 5.3 속성과 소스 위치

| 속성 | 매개변수 | 값의 출처 | 비고 |
| --- | --- | --- | --- |
| `#cfs.mid<value, kind, name>` | **[설계 제안]** `value: i32`, `kind: "cmd"\|"tlm"\|"unknown"`, `name: string` (macro 이름 또는 빈 문자열) | **[설계 제안]** value는 IR 상수에서, kind와 name은 AST·preprocessor sidecar에서 온다 (§5.7.3) | name이 비어도 유효하다. kind는 값의 bit에서 추론하지 않는다. 쓰인 macro(`…_CMD_PLATFORM_MIDVAL`, `…_TLM_PLATFORM_MIDVAL`)에서 정한다 ([default_sample_app_msgids.h L29-L31](https://github.com/nasa/sample_app/blob/199476a34827ae84d50d66f97619227854cd971a/config/default_sample_app_msgids.h#L29-L31)). |
| `#cfs.mid_src<kind, ref>` | **[설계 제안]** `kind: code_const\|param\|table_image\|cmd_payload\|runtime_fn\|unknown`, `ref: "file:line"` | **[설계 제안]** P3, P7 (§5.8) | D7. **[확인된 사실]** `table_image`, `cmd_payload`의 실제 예는 HK copy table, TO_LAB 구독 table과 명령, SCH_LAB schedule이다 (검증 C18). |
| `#cfs.tbl_opts<buffering, load, critical>` | **[설계 제안]** `"single"\|"double"`, `"load_dump"\|"dump_only"\|"user_addr"`, `bool` | **[확인된 사실]** `CFE_TBL_OPT_*` 상수 ([cfe_tbl_api_typedefs.h L50-L67](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_tbl_api_typedefs.h#L50-L67)) | sample_app은 `CFE_TBL_OPT_DEFAULT` = `SNGL_BUFFER \| LOAD_DUMP`를 쓴다. |
| `#cfs.timeout<kind, ms>` | **[설계 제안]** `pend_forever\|poll\|ms`, `i32` | **[확인된 사실]** `CFE_SB_PEND_FOREVER` = -1, `CFE_SB_POLL` = 0 ([cfe_sb_api_typedefs.h L45-L46](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_sb_api_typedefs.h#L45-L46)) | 상수일 때만 붙인다. operand가 원본이다. |
| `#cfs.sys_state<s>` | **[설계 제안]** `CFE_ES_SystemState_*` 이름 | **[확인된 사실]** UNDEFINED 0부터 SHUTDOWN 6까지 ([default_cfe_es_extern_typedefs.h L184-L214](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/es/config/default_cfe_es_extern_typedefs.h#L184-L214)) | `cfs.es.wait_system_state`의 상수 인자에만 붙인다. |

**소스 위치.** **[설계 제안]** 위치는 속성이 아니라 MLIR Location으로 둔다. 종류는 세 가지다.

- 직접 호출: DI에서 온 `FileLineColLoc`를 그대로 쓴다.
- inline된 호출: `CallSiteLoc(callee 위치 at 호출 위치)`를 그대로 쓴다.
- 코드 밖 선언: `FusedLoc<#cfs.origin<"table">>[loc("sch_lab_table.c":66:9)]`처럼 출처 종류를 metadata로 둔다.

### 5.4 resource와 effect 모델

**[설계 제안]** 아래 resource를 둔다. effect 표기는 이렇다. `W(R)`·`R(R)`은 resource R에 대한 MemWrite·MemRead이고 Value가 없다. `W(%x)`·`R(%x)`는 DefaultResource에 대한 effect이고 Value %x를 갖는다. `A(R)`·`F(R)`은 Allocate·Free다.

| resource | 주소성 | 대상 상태 | 쓰는 op | 읽는 op | §3 근거 |
| --- | --- | --- | --- | --- | --- |
| `CFS_Runtime` | 비주소 (root) | 아래 비주소 resource의 부모 | — | — | — |
| `CFS_SBRoutes` | 비주소 | MID → destination 목록, MsgLim, route sequence | `sb.subscribe`, `sb.unsubscribe`, `sb.delete_pipe`, `sb.transmit`(sequence) | `sb.transmit`, `evs.send_event` | SB-2, SB-6, SB-11 |
| `CFS_SBQueues` | 비주소 | pipe queue 내용, buffer pool, 수신 buffer 수명 | `sb.create_pipe`(A), `sb.delete_pipe`(F), `sb.transmit`(put), `sb.receive`(get) | — | SB-3, SB-7, SB-9, OS-1 |
| `CFS_ESState` | 비주소 | SystemState, AppState, 제어 요청, child task | `es.run_loop`, `es.wait_*`, `es.exit_app`, `es.create_child_task`, `es.restart_app` | `es.get_app_id`, `es.wait_*` | ES-1–ES-10 |
| `CFS_TBLRegistry` | 비주소 | registry, descriptor의 lock·buf, load 진행 상태 | `tbl.register`, `share`, `load`, `get_address`, `release_address`, `manage`, `update`, `unregister` | `tbl.get_status`, `tbl.get_info` | TBL-1–TBL-9 |
| `CFS_EVSState` | 비주소 | 등록, filter 상태, event 수 | `evs.register`, `evs.send_event` | — | EVT-1–EVT-3 |
| `CFS_PerfLog` | 비주소 | perf 기록 | `es.perf_log` | — | — |
| `CFS_TBLBuffers` | 주소 (DefaultResource의 자식) | table buffer의 내용 | `tbl.load`, `tbl.manage`, `tbl.update` | `get_address`가 준 포인터를 통한 load | TBL-4, TBL-5 |

**[확인된 사실] local과 main의 차이.**

- local `1053047a`의 `Resource`에는 위계와 주소성이 없다 (`/home/user/work/llvm-project/mlir/include/mlir/Interfaces/SideEffectInterfaces.h` L79-L85).
- local `LocalAliasAnalysis::getModRef`는 Value가 없는 effect를 모든 위치와 MayAlias로 본다 (`/home/user/work/llvm-project/mlir/lib/Analysis/AliasAnalysis/LocalAliasAnalysis.cpp` L528).
- main `ccac700c`에는 `getParent`, `isAddressable`, `isDisjointFrom`이 있다 ([L131-L151](https://github.com/llvm/llvm-project/blob/ccac700c68e92a7eabcb2f745789978213f16f2d/mlir/include/mlir/Interfaces/SideEffectInterfaces.h#L131-L151)).
- main의 `getModRef`는 Value 없는 비주소 effect를 NoAlias로 건너뛴다 ([LocalAliasAnalysis.cpp L528-L535](https://github.com/llvm/llvm-project/blob/ccac700c68e92a7eabcb2f745789978213f16f2d/mlir/lib/Analysis/AliasAnalysis/LocalAliasAnalysis.cpp#L528-L535)).
- main은 "주소 resource는 비주소 부모를 갖지 않는다"는 불변식을 둔다. 검사는 `EXPENSIVE_CHECKS` build에서만 돈다 ([L159-L171](https://github.com/llvm/llvm-project/blob/ccac700c68e92a7eabcb2f745789978213f16f2d/mlir/include/mlir/Interfaces/SideEffectInterfaces.h#L159-L171)). 그래서 `CFS_TBLBuffers`는 `CFS_Runtime`이 아니라 DefaultResource 아래에 둔다.
- local과 main 모두 `EffectInstance`가 `Attribute parameters`를 받는다 (local L173-L204, main [L248-L255](https://github.com/llvm/llvm-project/blob/ccac700c68e92a7eabcb2f745789978213f16f2d/mlir/include/mlir/Interfaces/SideEffectInterfaces.h#L248-L255)).

**[설계 제안] 결정.**

- main의 위계로 옮기면 cfs op의 비주소 effect는 C 메모리 질의에서 NoAlias가 된다. local에 머물면 같은 효과를 domain alias 구현으로 얻어야 한다. **[확인된 사실]** local `AliasAnalysis`에는 `addAnalysisImplementation`이 있다 (`mlir/include/mlir/Analysis/AliasAnalysis.h` L263). SYCL-MLIR은 `LocalAliasAnalysis`를 상속한 domain 규칙을 쓴다 (§4.5.1).
- 상수로 알려진 MID·pipe site는 effect의 `parameters`에 `{mid = #cfs.mid<…>, pipe = @site}`로 담을 수 있다. **[해석]** upstream 분석은 이 값을 읽지 않는다. 이 연구의 분석은 operand를 직접 읽으므로 `parameters`는 보조 수단이다.
- **domain alias 규칙 AL1.** `cfs.sb.buf_ptr`의 결과는 앱 전역(`llvm.mlir.addressof`)과 `llvm.alloca`에 대해 NoAlias다. **[해석]** 수신 buffer는 SB buffer pool에 있다 (SB-9, SB-10).
- **domain alias 규칙 AL2.** `cfs.tbl.get_address`의 결과는 앱 전역과 `llvm.alloca`에 대해 NoAlias다. 예외는 `#cfs.tbl_opts`의 load가 `"user_addr"`인 table이다. **[해석]** `CFE_TBL_OPT_USR_DEF_ADDR`(0x0006)는 사용자가 정한 주소를 쓰므로 앱 메모리를 가리킬 수 있다 ([cfe_tbl_api_typedefs.h L59-L60](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_tbl_api_typedefs.h#L59-L60)).

### 5.5 op 정의

**[설계 제안]** 아래는 ODS 형식을 따른 초안이다. 컴파일하지 않았다. 대표 op 다섯 개는 ODS로 쓰고, 나머지는 표로 정리한다. §5.5의 op, operand, 결과, 속성, effect, verifier 연결은 모두 설계 제안이다. 표 안의 **[확인된 사실]**은 대체하는 C API의 위치와 동작에만 붙는다.

```tablegen
// [설계 제안] 구현되지 않음. llvm-project 1053047a의 ODS 문법을 따른 초안.
def CFS_SBRoutes   : Resource<"::cfs::SBRoutesResource">;    // C++에서 isAddressable() = false (main)
def CFS_SBQueues   : Resource<"::cfs::SBQueuesResource">;
def CFS_TBLRegistry: Resource<"::cfs::TBLRegistryResource">;

def CFS_PipeType  : CFS_Type<"Pipe",  "pipe">;
def CFS_MsgIdType : CFS_Type<"MsgId", "msgid">;
def CFS_BufType   : CFS_Type<"Buf",   "buf">;
def CFS_TblType   : CFS_Type<"Tbl",   "tbl">;

def CFS_MidAttr : CFS_Attr<"Mid", "mid"> {
  let parameters = (ins "uint32_t":$value, "::cfs::MidKind":$kind, StringRefParameter<>:$name);
}

def CFS_SBSubscribeOp : CFS_Op<"sb.subscribe",
    [MemoryEffects<[MemWrite<CFS_SBRoutes>]>, AttrSizedOperandSegments]> {
  let arguments = (ins CFS_MsgIdType:$mid, CFS_PipeType:$pipe, Optional<I16>:$msglim,
                       CFS_SubscribeVariantAttr:$variant,          // plain | ex | local
                       OptionalAttr<CFS_QosAttr>:$qos);
  let results   = (outs I32:$status);
  let hasVerifier = 1;   // V3
}

def CFS_SBReceiveOp : CFS_Op<"sb.receive", [MemoryEffects<[MemWrite<CFS_SBQueues>]>]> {
  let arguments = (ins CFS_PipeType:$pipe, I32:$timeout, OptionalAttr<CFS_TimeoutAttr>:$timeout_kind);
  let results   = (outs I32:$status, CFS_BufType:$buf);
}

def CFS_SBTransmitOp : CFS_Op<"sb.transmit",
    [DeclareOpInterfaceMethods<MemoryEffectOpInterface>]> {   // R(%msg), R/W(SBRoutes), W(SBQueues)
  let arguments = (ins Arg<LLVM_AnyPointer, "message", [MemRead]>:$msg, I1:$is_origination,
                       CFS_TransmitVariantAttr:$variant,           // msg | buffer
                       OptionalAttr<CFS_MidAttr>:$mid, OptionalAttr<CFS_MidSrcAttr>:$mid_src);
  let results   = (outs I32:$status);
  let hasVerifier = 1;   // V7
}

def CFS_TBLGetAddressOp : CFS_Op<"tbl.get_address", [MemoryEffects<[MemWrite<CFS_TBLRegistry>]>]> {
  let arguments = (ins CFS_TblType:$handle);
  let results   = (outs I32:$status, LLVM_AnyPointer:$ptr);       // AL2
}

def CFS_StateUpdateOp : CFS_Op<"state.update"> {
  let arguments = (ins Arg<LLVM_AnyPointer, "field address", [MemWrite]>:$addr, AnyType:$value,
                       FlatSymbolRefAttr:$field,                   // -> cfs.field
                       CFS_UpdateViaAttr:$via);                    // store | out_param | memset
  let hasVerifier = 1;   // V5
}
```

#### 5.5.1 선언 op

| op | 위치 | 속성 | 의미 | verifier |
| --- | --- | --- | --- | --- |
| `cfs.app` | module | `sym_name`, `entry: FlatSymbolRef`, `startup: {line, priority, stack}` | **[설계 제안]** 앱 하나. ODS 판은 `SymbolTable`, `NoTerminator`, `SingleBlock` region에 아래 선언들을 담는다. | V9 |
| `cfs.task` | `cfs.app` 안 | `sym_name`, `entry`, `role: main\|child` | **[설계 제안]** 실행 단위. 정체성은 앱 단위다 (M8). | V10 |
| `cfs.pipe_site` | `cfs.app` 안 | `sym_name`, `pipe_name`, `depth_req: i16` | **[설계 제안]** `CreatePipe` 호출 위치 하나. 실행 중 pipe 하나와 대응한다. | — |
| `cfs.tbl_site` | `cfs.app` 안 | `sym_name`, `tbl_name`, `opts: #cfs.tbl_opts` | **[설계 제안]** `Register` 또는 `Share` 위치 하나 | — |
| `cfs.field` | `cfs.app` 안 | `sym_name`, `global`, `byte_offset`, `size`, `kind: counter\|control\|handle\|tlm_payload\|msg_state` | **[설계 제안]** 앱 전역의 field 하나. 원노트의 `cfs.state.define`에 해당한다. | V5 |
| `cfs.dispatch_case` | `cfs.app` 안 | `pipe`, `mid: #cfs.mid`, `cc: i32` (-1은 CC 무관), `handler` | **[설계 제안]** P6가 만든 (pipe, MID, CC) → handler 관계 | V6 |
| `cfs.config_send`, `cfs.config_subscribe` | module | `app`, `mid`, `msglim?`, `source` | **[설계 제안]** table image에서 온 발행·구독 | V11 |

**[실측]** IRDL은 `NoTerminator`를 표현하지 못한다. region에 선언 op를 담은 IRDL op는 "block with no terminator"로 거부되었다. 그래서 IRDL 검사에서는 선언을 module 수준에 평평하게 두었다 (`$W/cfs_irdl.mlir`).

#### 5.5.2 handle과 MID op

| op | 대체하는 C 요소 | operands → results | 속성 | effect | verifier |
| --- | --- | --- | --- | --- | --- |
| `cfs.handle.wrap` | 메모리에서 읽은 handle | `(i32\|i16) → !cfs.pipe\|!cfs.tbl\|!cfs.appid` | `site?` | Pure | V1, V2 |
| `cfs.handle.raw` | handle을 메모리에 쓰기 | `(!cfs.*) → i32\|i16` | — | Pure | V2 |
| `cfs.mid.const` | 상수 MID | `() → !cfs.msgid` | `value: #cfs.mid` | Pure, ConstantLike | V4 |
| `cfs.mid.from_value` | 상수가 아닌 `CFE_SB_ValueToMsgId` ([cfe_sb.h L936](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_sb.h#L936)) | `(i32) → !cfs.msgid` | `src: #cfs.mid_src` | Pure | — |
| `cfs.mid.raw` | `CFE_SB_MsgIdToValue` ([L907](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_sb.h#L907)) | `(!cfs.msgid) → i32` | — | Pure | — |
| `cfs.mid.equal` | `CFE_SB_MsgId_Equal` ([L876](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_sb.h#L876)) | `(!cfs.msgid, !cfs.msgid) → i1` | — | Pure | — |
| `cfs.mid.is_valid` | `CFE_SB_IsValidMsgId` ([L856](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_sb.h#L856)) | `(!cfs.msgid) → i1` | — | Pure | — |

**[확인된 사실]** `CFE_SB_IsValidMsgId`는 값이 0이 아니고 `CFE_PLATFORM_SB_HIGHEST_VALID_MSGID` 이하인지만 본다 ([cfe_sb_msg_id_util.c L200-L204](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/sb/fsw/src/cfe_sb_msg_id_util.c#L200-L204)). **[설계 제안]** 그래서 외부 함수지만 Pure로 모델링한다. P6가 static cache의 지연 초기화를 풀 때 이 의미를 쓴다.

#### 5.5.3 SB·MSG op

| op | C API (cfe_sb.h, cfe_msg.h) | operands → results | 속성 | effect | out-parameter 처리 |
| --- | --- | --- | --- | --- | --- |
| `cfs.sb.create_pipe` | `CFE_SB_CreatePipe` ([L89](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_sb.h#L89)) | `(depth: i16, name: ptr) → (status: i32, pipe)` | `site` | A(SBQueues), R(%name) | `PipeIdPtr` → `pipe` 결과 + `handle.raw` + `state.update{via=out_param}` |
| `cfs.sb.delete_pipe` | `CFE_SB_DeletePipe` ([L118](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_sb.h#L118)) | `(pipe) → status` | — | F(SBQueues), W(SBRoutes) | — |
| `cfs.sb.subscribe` | `Subscribe`·`SubscribeEx`·`SubscribeLocal` ([L302](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_sb.h#L302), [L267](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_sb.h#L267), [L337](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_sb.h#L337)) | `(mid, pipe, msglim?) → status` | `variant`, `qos?` | W(SBRoutes) | — |
| `cfs.sb.unsubscribe` | `Unsubscribe`·`UnsubscribeLocal` ([L362](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_sb.h#L362), [L388](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_sb.h#L388)) | `(mid, pipe) → status` | `variant` | W(SBRoutes) | — |
| `cfs.sb.receive` | `CFE_SB_ReceiveBuffer` ([L479](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_sb.h#L479)) | `(pipe, timeout: i32) → (status, buf)` | `timeout_kind?` | W(SBQueues) | `BufPtr` → `buf` 결과. 이후 `sb.buf_ptr`로 ptr을 꺼내 원래 지역에 store |
| `cfs.sb.buf_ptr` | 수신 포인터의 사용 | `(buf) → ptr` | — | Pure, AL1 | — |
| `cfs.sb.transmit` | `TransmitMsg`·`TransmitBuffer` ([L438](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_sb.h#L438), [L634](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_sb.h#L634)) | `(msg: ptr, is_origination: i1) → status` | `variant`, `mid?`, `mid_src?` | R(%msg), R·W(SBRoutes), W(SBQueues). buffer 변형은 F(SBQueues)를 더한다 | — |
| `cfs.msg.init` | `CFE_MSG_Init` ([cfe_msg.h L61](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_msg.h#L61)) | `(msg: ptr, mid, size: i64) → status` | — | W(%msg) | — |
| `cfs.msg.get_msgid` | `CFE_MSG_GetMsgId` ([L657](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_msg.h#L657)) | `(msg) → (status, mid)` | — | R(%msg) | `MsgId` → `mid` 결과 + 지역 store |
| `cfs.msg.get_fcncode` | `CFE_MSG_GetFcnCode` ([L582](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_msg.h#L582)) | `(msg) → (status, cc: i16)` | — | R(%msg) | 같음 |
| `cfs.msg.get_size` | `CFE_MSG_GetSize` ([L83](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_msg.h#L83)) | `(msg) → (status, size: i64)` | — | R(%msg) | 같음 |
| `cfs.msg.timestamp` | `CFE_SB_TimeStampMsg` ([cfe_sb.h L717](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_sb.h#L717)) | `(msg) → ()` | — | W(%msg) | — |

**[설계 제안]** 위 표의 op와 effect는 모두 설계 제안이다. `cfs.sb.transmit`은 원본 `%msg`에 쓰지 않는다. **[확인된 사실]** §3.8 M1의 모델에서 SB는 메시지를 복사한 뒤 복사본에 sequence와 시각을 쓴다.

#### 5.5.4 ES op

| op | C API (cfe_es.h) | operands → results | 속성 | effect | 비고 |
| --- | --- | --- | --- | --- | --- |
| `cfs.es.run_loop` | `CFE_ES_RunLoop` ([L388](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_es.h#L388)) | `(run_status: ptr) → i1` | — | R(%run_status), W(%run_status), W(ESState) | **[확인된 사실]** `*RunStatus`를 읽고([cfe_es_api.c L453](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/es/fsw/src/cfe_es_api.c#L453)), AppState를 RUNNING으로 올리며(L471), `*RunStatus`를 쓸 수 있다(L489). |
| `cfs.es.exit_app` | `CFE_ES_ExitApp` ([L346](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_es.h#L346)) | `(status: i32) → ()` | — | W(ESState) | — |
| `cfs.es.wait_system_state` | `CFE_ES_WaitForSystemState` ([L418](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_es.h#L418)) | `(min: i32, timeout_ms: i32) → status` | `state?` | R·W(ESState) | 성공 반환만 R-sync-1 간선을 만든다 (§3.3) |
| `cfs.es.wait_startup_sync` | `CFE_ES_WaitForStartupSync` ([L449](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_es.h#L449)) | `(timeout_ms: i32) → ()` | — | R·W(ESState) | **[확인된 사실]** void라 timeout 여부가 IR에 없다 (M7). |
| `cfs.es.create_child_task` | `CFE_ES_CreateChildTask` ([L822](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_es.h#L822)) | `(name, entry: ptr, stack, size, prio, flags) → (status, task: i32)` | `entry_sym?` | W(ESState), R(%name) | **[설계 제안]** entry가 상수면 P7이 `cfs.task{role = child}`를 만든다 |
| `cfs.es.restart_app` | `CFE_ES_RestartApp` ([L263](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_es.h#L263)) | `(app: !cfs.appid) → status` | — | W(ESState) | M9 |
| `cfs.es.get_app_id` | `CFE_ES_GetAppID` ([L525](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_es.h#L525)) | `() → (status, !cfs.appid)` | — | R(ESState) | out-parameter → 결과 |
| `cfs.es.perf_log` | `CFE_ES_PerfLogAdd` ([L1574](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_es.h#L1574)) | `() → ()` | `id`, `kind: entry\|exit` | W(PerfLog) | **[확인된 사실]** `PerfLogEntry`·`PerfLogExit`는 macro다 (L1520, L1539). IR에는 `PerfLogAdd(id, 0\|1)`만 남는다. |

#### 5.5.5 TBL op

| op | C API (cfe_tbl.h) | operands → results | 속성 | effect |
| --- | --- | --- | --- | --- |
| `cfs.tbl.register` | `CFE_TBL_Register` ([L234](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_tbl.h#L234)) | `(name, size: i64, opts: i16, validator: ptr) → (status, tbl)` | `site`, `opts` | W(TBLRegistry), R(%name) |
| `cfs.tbl.share` | `CFE_TBL_Share` ([L275](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_tbl.h#L275)) | `(name) → (status, tbl)` | `site?` | W(TBLRegistry) |
| `cfs.tbl.unregister` | `CFE_TBL_Unregister` ([L314](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_tbl.h#L314)) | `(tbl) → status` | — | W(TBLRegistry) |
| `cfs.tbl.load` | `CFE_TBL_Load` ([L372](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_tbl.h#L372)) | `(tbl, src_type: i32, src: ptr) → status` | `src_kind: file\|address` | W(TBLRegistry), W(TBLBuffers), address일 때 R(%src) |
| `cfs.tbl.get_address` | `CFE_TBL_GetAddress` ([L567](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_tbl.h#L567)) | `(tbl) → (status, ptr)` | — | W(TBLRegistry). 결과 ptr은 AL2 |
| `cfs.tbl.release_address` | `CFE_TBL_ReleaseAddress` ([L597](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_tbl.h#L597)) | `(tbl) → status` | — | W(TBLRegistry) |
| `cfs.tbl.manage` | `CFE_TBL_Manage` ([L459](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_tbl.h#L459)) | `(tbl) → status` | — | W(TBLRegistry), W(TBLBuffers). validator callback을 부를 수 있다 |
| `cfs.tbl.update` | `CFE_TBL_Update` ([L400](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_tbl.h#L400)) | `(tbl) → status` | — | W(TBLRegistry), W(TBLBuffers) |
| `cfs.tbl.get_status` | `CFE_TBL_GetStatus` ([L721](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_tbl.h#L721)) | `(tbl) → status` | — | R(TBLRegistry) |
| `cfs.tbl.get_info` | `CFE_TBL_GetInfo` ([L752](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_tbl.h#L752)) | `(info: ptr, name: ptr) → status` | — | R(TBLRegistry), W(%info) |

**[설계 제안]** 위 표의 op와 effect는 모두 설계 제안이다. TBL op의 추상 상태는 §3.8 M10의 `desc(h) = {lock, buf, updated}`를 그대로 쓴다. op는 상태를 바꾸는 사건만 표시한다. 상태 전이 함수는 §6의 분석에 둔다. validator callback이 어느 task 문맥에서 실행되는지는 **[미확인]**이다.

#### 5.5.6 EVS op

| op | C API (cfe_evs.h) | operands → results | effect |
| --- | --- | --- | --- |
| `cfs.evs.register` | `CFE_EVS_Register` ([L105](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_evs.h#L105)) | `(filters: ptr, n: i16, scheme: i16) → status` | **[설계 제안]** W(EVSState) |
| `cfs.evs.send_event` | `CFE_EVS_SendEvent` ([L155](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_evs.h#L155)) | `(eid: i16, type: i16, fmt: ptr, args: variadic) → status` | **[설계 제안]** W(EVSState), R(SBRoutes), W(SBQueues) |

**[확인된 사실]** EVS long event는 SB 메시지 `0x808`로 나간다. 모든 실행에서 OPERATIONAL 전에 이 MID의 "No subscribers" event가 기록되었다 (§7.4.3). **[설계 제안]** 그래서 `send_event`도 SB effect를 갖는다.

#### 5.5.7 state op

| op | 대체하는 C 요소 | operands → results | 속성 | effect | verifier |
| --- | --- | --- | --- | --- | --- |
| `cfs.state.read` | 앱 전역 field의 `llvm.load` | `(addr: ptr) → T` | `field` | R(%addr) | V5 |
| `cfs.state.update` | 앱 전역 field의 `llvm.store`, out-parameter 쓰기, `memset` | `(addr: ptr, value: T) → ()` | `field`, `via` | W(%addr) | V5 |

**[설계 제안]** state op는 DefaultResource에 Value를 붙인 effect를 쓴다. 그래서 upstream alias·modref 질의가 계속 작동한다. field 정체성은 `field` 속성에 있다. **[확인된 사실]** upstream `LocalAliasAnalysis`는 한 struct의 서로 다른 field를 MustAlias로 본다 (검증 C20 (a)). **[설계 제안]** 이 연구의 분석은 field 비교에 `field` 속성을 쓴다.

**[설계 제안]** `memset`처럼 구간 전체를 쓰는 연산은 `via = memset`인 `state.update`를 겹치는 field마다 하나씩 만든다. **[실측]** `SAMPLE_APP_Init`의 `memset`(sample_app.c L117)은 TBAA tag가 없어 §5.7.4의 field resolver가 잡지 못했다.

#### 5.5.8 원노트 op 이름과의 대응

| 원노트 §11 이름 **[원노트 구상]** | 이 절의 처리 **[설계 제안]** | 이유 |
| --- | --- | --- |
| `cfs.sb.create_pipe`, `subscribe`, `unsubscribe`, `receive` | 그대로 둔다. `subscribe`에 `variant`·`msglim`을 더한다 | MsgLim이 drop을 정한다 (M4) |
| `cfs.sb.publish` | `cfs.sb.transmit` | cFE API 이름(`TransmitMsg`, `TransmitBuffer`)에 맞춘다 |
| `cfs.sb.dispatch` | op가 아니라 선언 `cfs.dispatch_case` | **[확인된 사실]** dispatch는 API가 아니라 `if`·`switch` 코드다 (sample_app_dispatch.c L149-L164) |
| `cfs.app`, `cfs.app.init`, `cfs.app.runloop`, `cfs.app.ready` | `cfs.app`·`cfs.task` 선언, `cfs.es.run_loop` | **[확인된 사실]** init은 앱 함수이고 API가 아니다. 준비 완료 선언은 `RunLoop`·`WaitFor*` 호출이 한다 (M7). |
| `cfs.state.define`, `update`, `read` | `cfs.field`, `cfs.state.update`, `cfs.state.read` | — |
| `cfs.state.consume` | 두지 않는다 | **[해석]** 소비는 다른 task의 `state.read`이고 분석이 정한다 |
| `cfs.tbl.acquire`, `release` | `cfs.tbl.get_address`, `cfs.tbl.release_address` | API 이름에 맞춘다 |
| `cfs.sync.startup`, `cfs.sync.system_state` | `cfs.es.wait_startup_sync`, `cfs.es.wait_system_state` | — |
| `cfs.sync.handshake` | 두지 않는다 | **[해석]** 대응하는 cFE API가 없다. 앱 사이 handshake는 SB 메시지 쌍으로 나타난다 |
| `cfs.lifecycle.*` | `cfs.es.exit_app`, `cfs.es.restart_app` | 재시작 창의 사건열은 §3.8 M9 모델에 둔다 |

### 5.6 verifier 규칙과 lint 규칙

**[설계 제안]** verifier 규칙(V)은 IR 표현만 검사한다.

| ID | 규칙 |
| --- | --- |
| V1 | `cfs.handle.wrap`의 `site`는 같은 module의 `cfs.pipe_site`·`cfs.tbl_site`를 가리킨다. 결과 타입은 site의 종류와 같다. |
| V2 | `handle.wrap`의 입력과 `handle.raw`의 결과 폭은 module 속성 `cfs.abi`가 정한 폭과 같다. sample_app build에서 pipe는 i32, table은 i16이다. |
| V3 | `cfs.sb.subscribe`의 `variant`가 `plain`이면 `msglim`이 없다. `ex`·`local`이면 `msglim`이 있다. |
| V4 | `#cfs.mid`의 `kind`는 cmd, tlm, unknown 중 하나다. module에 MID 표(`cfs.mid_table`)가 있고 `name`이 비어 있지 않으면 값이 표와 같다. |
| V5 | `cfs.state.*`의 `field`는 `cfs.field`를 가리킨다. 값의 폭은 field 크기와 같다. 주소가 상수 GEP로 전역에 뿌리를 두면 byte offset이 field와 같다. |
| V6 | `cfs.dispatch_case`의 `pipe`는 `cfs.pipe_site`, `handler`는 ptr 하나를 받는 `llvm.func`다. (pipe, mid, cc) 조합은 한 번만 나온다. |
| V7 | `cfs.sb.transmit`에 `mid`가 있으면 `mid_src`도 있다. |
| V8 | status 결과는 i32다. **[확인된 사실]** `CFE_Status_t`는 `int32`다 ([cfe_error.h L43](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_error.h#L43)). |
| V9 | `cfs.app`의 `entry`는 인자가 없는 `llvm.func`다. |
| V10 | `cfs.task`의 `role = main`은 앱마다 하나다. |
| V11 | `cfs.config_*`의 `source`는 비어 있지 않다. |

**[설계 제안]** lint 규칙(L)은 프로그램 성질이다. `cfs-lint`가 위치와 함께 보고하고 IR은 유효하게 남긴다. 판정 방법은 §6에서 다룬다.

| ID | 후보 | 근거 |
| --- | --- | --- |
| L1 | `create_pipe`, `subscribe`, `tbl.register`, `get_address`의 status를 쓰지 않는다 | SB-8, TBL-9 |
| L2 | `!cfs.buf`에서 꺼낸 포인터를 같은 pipe의 다음 `sb.receive` 뒤에 쓴다 | M6 |
| L3 | `get_address`의 status가 음수인 경로에서 ptr을 역참조한다. `release_address` 뒤에 ptr을 쓴다 | TBL-1, M10, M11 |
| L4 | `mid_src`가 `code_const`가 아닌 `transmit`·`subscribe` | 통신 그래프가 불완전함을 알린다 (M14) |
| L5 | 모델이 없는 `CFE_*`·`OS_*` 호출이 남았다 | D7 |

### 5.7 frontend별 인식 방법

#### 5.7.1 세 frontend 비교

| 정보 | LLVM import, `-O0` | LLVM import, `-O1 -Xclang -disable-llvm-passes` + MLIR pipeline | ClangIR [S17] | Clang AST (LibTooling, S18) |
| --- | --- | --- | --- | --- |
| 가용성 | **[실측]** 작동. 154개 파일 범위는 §5.1 D1의 조건과 같다 (검증 C17 정정) | **[실측]** sample_app 4개 TU에서 작동 | **[실측]** 로컬에서 쓸 수 없다. `-emit-cir`는 rebuild를 요구하고, `-fclangir -S -emit-llvm`은 조용히 무시된다 (검증 C19) | **[실측]** `-ast-dump` 작동 (`$N/probe/cir/ast_SAMPLE_APP_Init.txt`) |
| API 호출 | **[실측]** callee 이름. 154개 파일의 per-file import에서 `llvm.call` 8830개와 `llvm.load` 26378개가 모두 file:line:col을 갖는다 (검증 C17 정정) | **[실측]** 같음. inlining이 호출을 복제한다 (§5.7.2) | **[미확인]** `cir.call` (문서 수준) | **[실측]** `CallExpr` |
| MID 값 | **[실측]** 상수가 아니다. `ValueToMsgId(const)` → alloca store → load (검증 C18) | **[실측]** call site의 직접 상수 (검증 C18) | **[미확인]** | **[실측]** literal 식. `6144`가 `global_core_api_base_msgid_values.h:30:34`, `131`이 `sample_app_topicids.h:31:52` (§7.7.3) |
| MID 이름 | **[확인된 사실]** 없다. `-fdebug-macro`의 DIMacro도 import가 경고 없이 버린다 (검증 C18 정정) | 같음 | **[미확인]** | **[실측]** literal의 spelling 위치만 있다. macro 이름은 preprocessor callback이 필요하다 (§5.7.3) |
| field 이름 | **[실측]** GEP 위치 index만 있다. 이름은 DI에 있고, 그 struct를 정의에 쓰는 TU에만 있다 | **[실측]** TBAA struct-path tag + 앱 연결 DI로 이름이 정해진다 (§5.7.4) | **[미확인]** 문서상 `cir.get_member`가 이름을 유지한다 (§4.5.1) | **[실측]** `MemberExpr .CommandPipe` (§7.7.3) |
| offset 0 field | **[확인된 사실]** GEP가 없어 모호하다 (§7.8) | **[실측]** sample_app 31/31 해결 | **[미확인]** | **[해석]** `MemberExpr`로 구별된다 |
| out-parameter 쓰기 | **[확인된 사실]** store로 보이지 않는다 (§7.8) | 같음 | **[해석]** 같다. API 모델이 필요하다 | **[해석]** `&x` 인자만 보인다 |
| 제어 구조 | CFG | CFG. **[실측]** CC `switch`가 `llvm.switch`로 남는다 | **[미확인]** 문서상 구조적 `cir.if`·`cir.switch` | Stmt 트리 |
| 간접 호출 | **[실측]** 연결 mission module에 73곳 (검증 C17 정정) | 같음 | **[미확인]** | 식 수준 |

**[해석]** 결론은 이렇다. 주 경로는 LLVM import + MLIR pipeline이다. AST는 MID 이름과 kind를 위한 sidecar다. ClangIR는 로컬에서 시험할 수 없으므로 설계를 그에 묶지 않는다.

#### 5.7.2 API 호출 인식

**[설계 제안]** `cfs-recognize-api`는 `llvm.call`의 callee 이름과 시그니처를 cFE `546a0025` header에서 만든 표와 대조한다. 이름이 같아도 인자 수나 타입이 다르면 바꾸지 않고 L5로 보고한다.

**[실측]** 인자 타입은 C 원형과 다르다. `CFE_SB_Subscribe(CFE_SB_MsgId_t, CFE_SB_PipeId_t)`는 IR에서 `(i32, i32) -> i32`다. struct MID가 i32로 coerce되었다 (`$N/probe/ir/sample_app/sample_app.ll` L149). 표는 이 ABI 형태로 만든다.

**[확인된 사실]** 세 가지는 API 이름으로 직접 보이지 않는다.

- `CFE_ES_PerfLogEntry(id)`와 `PerfLogExit(id)`는 `CFE_ES_PerfLogAdd(id, 0|1)` macro다 (cfe_es.h L1520, L1539).
- `CFE_MSG_PTR(x)`는 GEP로만 남는다.
- `CFE_SB_ValueToMsgId`, `CFE_SB_MsgId_Equal`, `CFE_SB_MsgIdToValue`는 `static inline`이다 ([cfe_sb.h L876-L936](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_sb.h#L876-L936)). `-O0`에서는 module마다 internal 함수 호출로 남는다 (§7.6). pipeline 뒤에는 inline되어 사라진다.

**[실측]** inline된 helper는 이름이 없다. 그래서 pipeline 뒤의 `MsgId_Equal`은 `llvm.icmp "eq"`로만 남는다. `linked.opt.mlir`의 `SAMPLE_APP_TaskPipe`에서 `%58 = llvm.icmp "eq" %56, %57`이 그 예다. `%56`은 `CFE_MSG_GetMsgId`의 out-parameter alloca를 읽은 값이다. `%57`은 `@SAMPLE_APP_TaskPipe.SEND_HK_MID`를 읽은 값이다. **[설계 제안]** 이 icmp는 두 operand의 출처가 모두 MID일 때만 `cfs.mid.equal`로 바꾼다. 출처는 `msg.get_msgid` 결과, `struct.CFE_SB_MsgId_t` 타입 전역, `mid.const`다.

**[실측] inlining 범위의 영향.** 같은 4개 TU에 대해 두 설정을 비교했다.

| 설정 | `llvm.call @CFE_*`·`@OS_*` 수 | 위치 기준 중복 제거 후 | 위치 없는 호출 | MID 상수 | handler 호출 |
| --- | --- | --- | --- | --- | --- |
| 기본 `--inline` (`linked.opt.mlir`, 3437줄, 0.164 s) | 156 | 45 | 0 | 직접 상수 | inline되어 사라진다 |
| 앱 함수 12개에 `noinline` (`linked_noinl.opt.mlir`, 1656줄, 0.038 s) | 45 | 45 | 0 | 직접 상수 (`SAMPLE_APP_Init` 안의 2179, 6275, 6274) | `TaskPipe`→`SendHkCmd`, `ProcessGroundCommand`→4개 명령 handler가 `llvm.call`로 남는다 |

명령은 `opt -S -passes=forceattrs -force-attribute=<fn>:noinline …`이다. 그 뒤 `mlir-translate --import-llvm --mlir-print-debuginfo`와 같은 `mlir-opt` pipeline을 돌렸다 (`$W/api_sites.out`, `$W/api_sites_noinl.out`). 두 번째 설정에서도 cFE helper 호출은 0개로 남았다. 기본 설정에서는 `CFE_EVS_SendEvent`가 41번 나오지만 소스 위치는 12곳이다.

**[해석]** MID 상수화에 필요한 것은 cFE helper의 inline뿐이다. 앱 함수까지 inline하면 호출이 복제되고 handler 경계가 사라진다. **[설계 제안]** 그래서 P0은 앱 함수를 `noinline`으로 둔다 (§5.8). 이 설정을 다른 앱과 mission 전체에 적용한 결과는 **[미확인]**이다.

#### 5.7.3 MID 인식

**[확인된 사실]** `-O0`에서는 코드에서 정한 MID가 `llvm.call @CFE_SB_ValueToMsgId(i32 6275)` → alloca store → load를 거쳐 `CFE_SB_Subscribe`에 간다 (`sample_app.ll` L143-L149, 검증 C18). 같은 IR에 MLIR pipeline만 돌리면 helper가 `optnone noinline`이라 호출로 남는다 (검증 C18 정정). `-O1 -Xclang -disable-llvm-passes` 뒤 pipeline을 돌리면 직접 상수가 된다.

**[확인된 사실]** 앱별 상수 회수 수와 나머지 site의 출처(table, 명령 payload)는 §7.7.3의 표에 있다 (검증 C18 정정 문구).

to_lab의 3개 중 1개는 helper `TO_LAB_CmdSubscribe`([to_lab_app.c L261](https://github.com/nasa/to_lab/blob/d27c6014cb5d2979140500f635876ee87b81995b/fsw/src/to_lab_app.c#L261))의 매개변수다. out-of-line 사본에서는 `%arg0`이다. inline된 사본에서 `0x1880`·`0x1881`(6272, 6273) 상수가 된다. **[설계 제안]** 이런 site는 `#cfs.mid_src<param>`으로 두고, 호출자마다 값을 정하는 context 복제는 P3의 선택 사항으로 둔다.

**[실측] 상수의 위치.** pipeline 뒤의 MID 상수에는 자기 위치가 없다. `sample_app.dbg.mlir` L207-L217의 6274, 6275, 2179는 모두 `loc(#loc172)` = `callsite(unknown at sample_app.c:60:14)`다. 상수가 함수 입구로 올라갔기 때문이다.

**[실측] 위치를 되찾는 단서.** inline된 `CFE_SB_ValueToMsgId`의 매개변수 `MsgIdValue`에 대한 `llvm.intr.dbg.value`는 호출 위치를 유지한다.

```mlir
// $N/probe/mlir_opt/sample_app/sample_app.dbg.mlir (발췌)
%19 = llvm.mlir.constant(6275 : i32) : i32 loc(#loc172)            // callsite(unknown at sample_app.c:60:14)
llvm.intr.dbg.value #di_local_variable5 = %19 : i32 loc(#loc228)    // name = "MsgIdValue"
#loc228 = loc(callsite(#loc133 at #loc189))                         // #loc133 -> cfe_sb.h:936:70
#loc189 = loc(callsite(#loc118 at #loc160))                         // #loc118 -> sample_app.c:158:35, #loc160 -> 60:14
%58 = llvm.call @CFE_SB_Subscribe(%19, %57) : (i32, i32) -> i32 loc(#loc191)   // callsite(sample_app.c:158:18 at 60:14)
```

같은 방식으로 2179는 `sample_app.c:135:22`, 6274는 `sample_app.c:173:35`에 묶인다. **[실측]** AST의 `ValueToMsgId` `DeclRefExpr`도 `col:35`에 있다. `CFE_SB_Subscribe`는 `col:18`이다 (`ast_SAMPLE_APP_Init.txt` L124, L127). 두 위치가 MLIR 위치와 같다.

**[설계 제안] 이름 join.** P3은 MID 상수 operand마다 `dbg.value(MsgIdValue)`의 위치를 찾는다. 그 위치로 AST·preprocessor sidecar를 조회한다. sidecar는 LibTooling 도구로 만든다. `PPCallbacks::MacroExpands`로 그 범위에서 펼쳐진 macro 이름(`SAMPLE_APP_SEND_HK_MID`)을 기록한다. kind는 `…_CMD_PLATFORM_MIDVAL` 또는 `…_TLM_PLATFORM_MIDVAL` 중 어느 macro를 거쳤는지로 정한다. **[미확인]** 이 sidecar는 만들지 않았다. 측정한 것은 위치가 일치한다는 사실뿐이다.

**[확인된 사실] EDS build.** EDS 설정에서는 MID mapping macro가 `CFE_SB_LocalCmdTopicIdToMsgId` 같은 함수 호출이다 (§7.7.3). EDS 구현은 processor ID에 따라 값을 정한다 (검증 C18 정정). **[미확인]** EDS build는 컴파일하지 않아 IR 형태를 보지 않았다. **[설계 제안]** 이 호출의 결과는 `#cfs.mid_src<runtime_fn>`으로 두거나, 설정별 표로 평가한다.

#### 5.7.4 state 접근 인식

**[확인된 사실]** `-O0`에서는 field가 GEP 위치 index로 남는다. 이름은 DI의 `DW_TAG_member`에 있다. offset 0 field는 GEP 없이 base 주소로 접근되어 모호하다. §7.8의 실측은 sample_app에서 store 11개 중 7개에 이름을 붙였고 4개가 offset 0 모호였다고 보고했다.

**[실측] DI의 위치.** `SAMPLE_APP_Data_t`의 member DI는 `sample_app.c`의 module에만 있다. cmds, dispatch, utils module에는 0개다. 그 TU들에서는 `SAMPLE_APP_Data`가 `extern`이기 때문이다. 그래서 앱 단위로 `llvm-link`한 뒤 import해야 한다.

**[실측] TBAA로 offset 0을 푼다.** `-O1 -Xclang -disable-llvm-passes`는 load·store에 TBAA struct-path tag를 붙인다. tag의 base type과 offset을 DI member의 `offsetInBits`와 맞추면 field 경로가 나온다. `linked.opt.mlir`에 대한 결과는 이렇다 (`$W/resolve_fields.out`, `$W/dedupe_sites.out`).

| 항목 | 값 |
| --- | --- |
| TBAA tag가 있고 전역에 뿌리를 둔 접근 | 93 |
| 그중 GEP 없이 base 주소로 접근한 것 | 31. 31개 모두 tag offset 0으로 `CommandCounter`가 되었다 |
| scalar tag (`short`) 접근 | 13개 모두 `TblHandles`. tag offset이 0이라 GEP index로 대신 정했다 |
| 위치 기준 중복 제거 후 (field, load·store) 사이트 | 25. 위치 없는 접근 0 |
| `noinline` 설정 (`linked_noinl.opt.mlir`) | 접근 25개, base 주소 접근 8개, 사이트 25개로 같다 |

**[실측] TBAA의 조건.** `sample_app_cmds.c`를 같은 설정으로 다시 컴파일했다. 기본 설정에서 `!tbaa`가 45개다. `-fno-strict-aliasing`을 더하면 0개다 (`$W/cmds_sa.ll`, `$W/cmds_nsa.ll`). bundle의 cpu1 compile DB에는 `-fno-strict-aliasing`이 한 번도 나오지 않는다. **[설계 제안]** 분석용 컴파일은 strict aliasing을 켠 상태로 고정한다. 이 flag를 쓰는 mission에서는 GEP index와 DI만으로 정하고, offset 0 접근은 `field = unknown`으로 둔다.

**[실측] 잡지 못한 것.** `memset`(sample_app.c L117)과 API out-parameter 쓰기 네 곳이다. out-parameter는 `CreatePipe`→`CommandPipe`, `TBL_Register`→`TblHandles[0]`, `RunLoop`→`RunStatus`, `ReceiveBuffer`→지역 `SBBufPtr`이다. **[확인된 사실]** 지역 포인터를 통한 쓰기(`sch_lab_app.c:130`의 `LocalStateEntry->Counter = 0`)도 TBAA만으로는 전역과 연결되지 않는다 (§7.8). **[설계 제안]** 이것은 points-to가 필요하다.

#### 5.7.5 dispatch 구조 인식

**[확인된 사실]** sample_app의 수신 경로는 pipe 하나, `MsgId_Equal` 분기 두 개, CC `switch` 하나다 ([sample_app_dispatch.c L146-L164](https://github.com/nasa/sample_app/blob/199476a34827ae84d50d66f97619227854cd971a/fsw/src/sample_app_dispatch.c#L146-L164), [L86-L122](https://github.com/nasa/sample_app/blob/199476a34827ae84d50d66f97619227854cd971a/fsw/src/sample_app_dispatch.c#L86-L122)). 비교 대상 MID는 함수 안의 `static` cache다. cache는 `CFE_SB_MSGID_RESERVED`로 시작하고 첫 호출에서 채워진다 (L134-L144, §7.7.3).

**[설계 제안] 규칙.** pipe p에서 받은 메시지가 분기 b로 가는 MID 집합은 `handlers(p, b) = Sub(p) ∩ CmpSet(b)`다. `Sub(p)`는 p에 대한 `sb.subscribe`의 MID 집합이다. `CmpSet(b)`는 분기 조건의 `mid.equal`에서 비교 상대가 가질 수 있는 값의 집합이다.

**[해석] sample_app 적용.**

- `Sub(CMD_PIPE)` = {0x1882, 0x1883}이다.
- L149 분기에서 `SEND_HK_MID` cache가 가질 수 있는 값은 {0x0000, 0x1883}이다. `mid.is_valid` 의미(§5.5.2)를 쓰면 {0x1883}으로 줄어든다. 어느 쪽이든 교집합은 {0x1883}이다.
- L154 분기는 같은 방식으로 {0x1882}다.
- 0x1882 분기 안의 `llvm.switch`는 `GetFcnCode` out-parameter를 읽은 값에 대해 0, 1, 2, 3을 나눈다 (`linked.opt.mlir` L777).
- 각 case의 handler 호출은 `SAMPLE_APP_VerifyCmdLength`가 참일 때만 실행된다. **[설계 제안]** 이 길이 검사는 `dispatch_case`의 guard로 기록하지 않고, CFG 조건으로 분석에 남긴다.

### 5.8 lifting pass 순서

| 단계 | pass **[설계 제안]** | 입력 → 출력 | 쓰는 정보 | 실패 시 |
| --- | --- | --- | --- | --- |
| P0 | build·import | compile DB [S19] → 앱별 `llvm.func` module | clang `-O1 -Xclang -disable-llvm-passes -g`, 앱 함수 `noinline`, `llvm-link`, `mlir-translate --import-llvm --mlir-print-debuginfo`, `mlir-opt --inline --sroa --mem2reg --canonicalize --cse` | 파일 단위로 제외하고 목록에 남긴다 |
| P1 | `cfs-close-world` | visibility 부여 | startup script entry, 주소가 API 인자로 넘어간 함수 | public으로 둔다 |
| P2 | `cfs-recognize-api` | `llvm.call @CFE_*` → cfs op | cFE header 시그니처 표, out-parameter 명시화(D2), handle wrap·raw | `llvm.call`로 두고 L5 |
| P3 | `cfs-fold-mid` | MID operand → `mid.const` 또는 `mid.from_value` | 상수, `dbg.value(MsgIdValue)` 위치, AST·preprocessor sidecar | `mid_src` 표시 |
| P4 | `cfs-resolve-fields` | 전역 load·store → `state.read`·`state.update`, `cfs.field` | TBAA struct-path tag, 앱 연결 DI, GEP index | `field = unknown` |
| P5 | `cfs-bind-headers` | `sb.transmit`에 `mid` 부여 | 같은 (전역, field 경로)에 대한 `msg.init`. 앱 안에서 flow-insensitive로 모은다 | 서로 다른 MID로 init되는 buffer는 `mid` 없음 |
| P6 | `cfs-recover-dispatch` | `cfs.dispatch_case` 생성 | §5.7.5 규칙, `mid.is_valid` 의미, `llvm.switch` | handler 없는 case로 남긴다 |
| P7 | `cfs-attach-config` | `cfs.app`·`cfs.task`, `cfs.config_*` | startup script, table 소스 (`sample_defs/tables/*.c`), `CreateChildTask` 상수 entry | config 없음으로 표시 |

**[실측] P0의 비용.** sample_app 4개 TU는 `llvm-link`가 rc 0으로 끝났다. 기본 설정의 import는 0.048 s(2075줄), pipeline은 0.164 s(3437줄)였다. `noinline` 설정은 각각 0.025 s, 0.038 s(1656줄)였다. 모두 `time`의 real 값이다.

**[설계 제안] 순서의 이유.** P3은 P2 뒤에 둔다. MID 위치는 op의 operand를 따라가야 찾기 때문이다. P5는 P4 뒤에 둔다. `msg.init`과 `transmit`의 buffer를 같은 field 경로로 맞춰야 하기 때문이다. P6은 P3 뒤에 둔다. `Sub(p)`가 상수 MID를 필요로 하기 때문이다.

### 5.9 worked example: sample_app

#### 5.9.1 원본 코드

**[확인된 사실]** 아래는 sample_app `199476a3`의 해당 줄이다.

| 위치 | 코드 |
| --- | --- |
| [sample_app.c L69](https://github.com/nasa/sample_app/blob/199476a34827ae84d50d66f97619227854cd971a/fsw/src/sample_app.c#L69) | `while (CFE_ES_RunLoop(&SAMPLE_APP_Data.RunStatus) == true)` |
| [L77](https://github.com/nasa/sample_app/blob/199476a34827ae84d50d66f97619227854cd971a/fsw/src/sample_app.c#L77) | `status = CFE_SB_ReceiveBuffer(&SBBufPtr, SAMPLE_APP_Data.CommandPipe, CFE_SB_PEND_FOREVER);` |
| [L134-L135](https://github.com/nasa/sample_app/blob/199476a34827ae84d50d66f97619227854cd971a/fsw/src/sample_app.c#L134-L135) | `CFE_MSG_Init(CFE_MSG_PTR(SAMPLE_APP_Data.HkTlm.TelemetryHeader), CFE_SB_ValueToMsgId(SAMPLE_APP_HK_TLM_MID), …)` |
| [L141](https://github.com/nasa/sample_app/blob/199476a34827ae84d50d66f97619227854cd971a/fsw/src/sample_app.c#L141) | `status = CFE_SB_CreatePipe(&SAMPLE_APP_Data.CommandPipe, …)` (depth 32, 이름 `"SAMPLE_APP_CMD_PIPE"`) |
| [L158](https://github.com/nasa/sample_app/blob/199476a34827ae84d50d66f97619227854cd971a/fsw/src/sample_app.c#L158) | `status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(SAMPLE_APP_SEND_HK_MID), SAMPLE_APP_Data.CommandPipe);` |
| [L173](https://github.com/nasa/sample_app/blob/199476a34827ae84d50d66f97619227854cd971a/fsw/src/sample_app.c#L173) | `status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(SAMPLE_APP_CMD_MID), SAMPLE_APP_Data.CommandPipe);` |
| [L188](https://github.com/nasa/sample_app/blob/199476a34827ae84d50d66f97619227854cd971a/fsw/src/sample_app.c#L188) | `status = CFE_TBL_Register(&SAMPLE_APP_Data.TblHandles[0], "ExampleTable", …, CFE_TBL_OPT_DEFAULT, SAMPLE_APP_TblValidationFunc)` |
| [sample_app_dispatch.c L146-L156](https://github.com/nasa/sample_app/blob/199476a34827ae84d50d66f97619227854cd971a/fsw/src/sample_app_dispatch.c#L146-L156) | `CFE_MSG_GetMsgId(&SBBufPtr->Msg, &MsgId);` 뒤 `MsgId_Equal(MsgId, SEND_HK_MID)`, `MsgId_Equal(MsgId, CMD_MID)` |
| [sample_app_cmds.c L54-L61](https://github.com/nasa/sample_app/blob/199476a34827ae84d50d66f97619227854cd971a/fsw/src/sample_app_cmds.c#L54-L61) | `HkTlm.Payload.CommandErrorCounter = CommandErrorCounter; HkTlm.Payload.CommandCounter = CommandCounter;` 뒤 `TimeStampMsg`, `TransmitMsg(…, true)` |
| [sample_app_cmds.c L81](https://github.com/nasa/sample_app/blob/199476a34827ae84d50d66f97619227854cd971a/fsw/src/sample_app_cmds.c#L81) | `SAMPLE_APP_Data.CommandCounter++;` (NoopCmd) |
| [sample_app_cmds.c L123-L135](https://github.com/nasa/sample_app/blob/199476a34827ae84d50d66f97619227854cd971a/fsw/src/sample_app_cmds.c#L123-L135) | `GetAddress(&TblAddr, TblHandles[0])`, `if (Status < CFE_SUCCESS)`, `TblPtr->Int1` 사용, `ReleaseAddress` |

**[확인된 사실]** MID 값은 설정에서 온다. `SAMPLE_APP_CMD_MID`는 CMD base `0x1800`과 topic `0x82`로 6274가 된다. `SEND_HK_MID`는 `0x1800 | 0x83` = 6275, `HK_TLM_MID`는 TLM base `0x0800 | 0x83` = 2179다 ([default_sample_app_msgids.h L29-L31](https://github.com/nasa/sample_app/blob/199476a34827ae84d50d66f97619227854cd971a/config/default_sample_app_msgids.h#L29-L31), [sample_app_topicids.h L29-L33](https://github.com/nasa/sample_app/blob/199476a34827ae84d50d66f97619227854cd971a/fsw/inc/sample_app_topicids.h#L29-L33)).

**[실측]** `SAMPLE_APP_Data_t`의 IR 타입은 `{i8, i8, HkTlm, i32, i32, [1 x i16]}`다 ([sample_app.h L48-L72](https://github.com/nasa/sample_app/blob/199476a34827ae84d50d66f97619227854cd971a/fsw/src/sample_app.h#L48-L72)). byte offset은 `CommandCounter` 0, `CommandErrorCounter` 1, `HkTlm` 2, `HkTlm.Payload.CommandCounter` 18, `HkTlm.Payload.CommandErrorCounter` 19, `RunStatus` 20, `CommandPipe` 24, `TblHandles` 28이다.

#### 5.9.2 import된 IR

**[실측]** `-O0`의 L158 구독은 이렇다 (`$N/probe/ir/sample_app/sample_app.ll` L143-L149).

```llvm
%call16 = call i32 @CFE_SB_ValueToMsgId(i32 noundef 6275)
store i32 %call16, ptr %coerce.dive17, align 4                    ; alloca %agg.tmp15
%6 = load i32, ptr getelementptr inbounds nuw (%struct.SAMPLE_APP_Data_t, ptr @SAMPLE_APP_Data, i32 0, i32 4)
%7 = load i32, ptr %coerce.dive18, align 4
%call19 = call i32 @CFE_SB_Subscribe(i32 %7, i32 noundef %6)
```

**[실측]** pipeline 뒤에는 MID가 직접 상수다. `CommandPipe` load에는 TBAA tag가 붙는다 (`sample_app.dbg.mlir` L267-L268, 위치 줄임).

```mlir
%57 = llvm.load %42 {alignment = 4 : i64, tbaa = [#tbaa_tag3]} : !llvm.ptr -> i32
%58 = llvm.call @CFE_SB_Subscribe(%19, %57) : (i32, i32 {llvm.noundef}) -> i32
```

#### 5.9.3 lifted IR

**[설계 제안]** 아래는 `$W/sample_app_lifted.mlir`의 발췌다. 손으로 썼고 pass의 출력이 아니다. IRDL은 op 이름에 `.`을 허용하지 않는다. 그래서 파일에서는 `cfs.sb_subscribe`처럼 `_`를 쓴다. ODS 판의 이름은 `cfs.sb.subscribe`다.

```mlir
// module 수준 선언 (P4, P6, P7의 결과)
"cfs.app"() {sym_name = "SAMPLE_APP", entry = @SAMPLE_APP_Main,
             startup = {line = 6 : i32, priority = 50 : i32, stack = 32768 : i32}} : () -> ()
"cfs.pipe_site"() {sym_name = "SAMPLE_APP.pipe.SAMPLE_APP_CMD_PIPE", app = @SAMPLE_APP,
                   pipe_name = "SAMPLE_APP_CMD_PIPE", depth_req = 32 : i16} : () -> ()
"cfs.field"() {sym_name = "SAMPLE_APP.CommandCounter", app = @SAMPLE_APP, global = @SAMPLE_APP_Data,
               byte_offset = 0 : i32, size = 1 : i32, kind = "counter"} : () -> ()
"cfs.dispatch_case"() {pipe = @SAMPLE_APP.pipe.SAMPLE_APP_CMD_PIPE,
    mid = #cfs.mid<6275 : i32, "cmd", "SAMPLE_APP_SEND_HK_MID">, cc = -1 : i32, handler = @SAMPLE_APP_SendHkCmd} : () -> ()
"cfs.dispatch_case"() {pipe = @SAMPLE_APP.pipe.SAMPLE_APP_CMD_PIPE,
    mid = #cfs.mid<6274 : i32, "cmd", "SAMPLE_APP_CMD_MID">, cc = 0 : i32, handler = @SAMPLE_APP_NoopCmd} : () -> ()
"cfs.config_send"() {app = "SCH_LAB", mid = #cfs.mid<6275 : i32, "cmd", "SAMPLE_APP_SEND_HK_MID">,
                     source = "sample_defs/tables/sch_lab_table.c:66"} : () -> ()
"cfs.config_subscribe"() {app = "TO_LAB", mid = #cfs.mid<2179 : i32, "tlm", "SAMPLE_APP_HK_TLM_MID">,
                          msglim = 1 : i16, source = "sample_defs/tables/to_lab_sub.c:78"} : () -> ()

// SAMPLE_APP_Init 안 (sample_app.c L141-L173)
%s2, %pipe = "cfs.sb_create_pipe"(%c32_i16, %pn) {site = @SAMPLE_APP.pipe.SAMPLE_APP_CMD_PIPE}
    : (i16, !llvm.ptr) -> (i32, !cfs.pipe)
%praw = "cfs.handle_raw"(%pipe) : (!cfs.pipe) -> i32
"cfs.state_update"(%cp, %praw) {field = @SAMPLE_APP.CommandPipe} : (!llvm.ptr, i32) -> ()   // out-parameter (D2)
%mhkreq = "cfs.mid_const"() {value = #cfs.mid<6275 : i32, "cmd", "SAMPLE_APP_SEND_HK_MID">} : () -> !cfs.msgid
%s3 = "cfs.sb_subscribe"(%mhkreq, %pipe) {variant = "plain"} : (!cfs.msgid, !cfs.pipe) -> i32

// SAMPLE_APP_Main 안 (L69-L86)
%go = "cfs.es_run_loop"(%rs) : (!llvm.ptr) -> i1
%raw = "cfs.state_read"(%cp) {field = @SAMPLE_APP.CommandPipe} : (!llvm.ptr) -> i32
%pipe = "cfs.handle_wrap"(%raw) {site = @SAMPLE_APP.pipe.SAMPLE_APP_CMD_PIPE} : (i32) -> !cfs.pipe
%rst, %buf = "cfs.sb_receive"(%pipe, %m1) : (!cfs.pipe, i32) -> (i32, !cfs.buf)
%bp = "cfs.sb_buf_ptr"(%buf) : (!cfs.buf) -> !llvm.ptr
llvm.call @SAMPLE_APP_TaskPipe(%bp) : (!llvm.ptr) -> ()

// SAMPLE_APP_SendHkCmd 안 (sample_app_cmds.c L54-L61)
%b = "cfs.state_read"(%g) {field = @SAMPLE_APP.CommandCounter} : (!llvm.ptr) -> i8              // 55:73
"cfs.state_update"(%pl, %b) {field = @SAMPLE_APP.HkTlm.Payload.CommandCounter} : (!llvm.ptr, i8) -> ()   // 55:55
"cfs.msg_timestamp"(%hk) : (!llvm.ptr) -> ()
%t = "cfs.sb_transmit"(%hk, %true) {variant = "msg",
       mid = #cfs.mid<2179 : i32, "tlm", "SAMPLE_APP_HK_TLM_MID">} : (!llvm.ptr, i1) -> i32
```

**[설계 제안]** `transmit`의 MID 2179는 P5가 붙인다. 출처는 `sample_app.c` L134의 `msg.init`이다. 두 op의 buffer가 같은 `SAMPLE_APP_Data.HkTlm`(offset 2)이므로 묶인다.

#### 5.9.4 앱 경계를 넘는 사슬

**[설계 제안]** 위 선언과 op를 이으면 아래 사슬이 나온다. 각 고리는 위치를 갖는다.

| 순서 | 사건 | 위치 | 근거 수준 |
| --- | --- | --- | --- |
| 1 | SCH_LAB이 table entry로 6275를 보낸다. 주석은 "every 5.4 seconds"다 | [sch_lab_table.c L66](https://github.com/nasa/cFS/blob/5a9b075cd4c818ee8555f349a9f78ba5632d0384/sample_defs/tables/sch_lab_table.c#L66) | **[확인된 사실]** table 소스. 실행 중 적재된 image와 같은지는 **[미확인]** |
| 2 | SAMPLE_APP이 `CMD_PIPE`에 6275를 구독한다. `Subscribe`라 MsgLim은 기본값이다 | sample_app.c L158 | **[확인된 사실]** 코드. 기본값 4는 §3.8 M4 |
| 3 | `dispatch_case(6275) → SAMPLE_APP_SendHkCmd` | sample_app_dispatch.c L149 | **[설계 제안]** P6 결과 |
| 4 | `state.read CommandCounter` → `state.update HkTlm.Payload.CommandCounter` | sample_app_cmds.c 55:73 → 55:55 | **[실측]** P4 방식으로 이름이 정해진 사이트 |
| 5 | `sb.transmit` MID 2179 | sample_app_cmds.c L61, MID는 sample_app.c L134 | **[설계 제안]** P5 결과 |
| 6 | TO_LAB이 2179를 MsgLim 1로 구독한다 | [to_lab_sub.c L78](https://github.com/nasa/cFS/blob/5a9b075cd4c818ee8555f349a9f78ba5632d0384/sample_defs/tables/to_lab_sub.c#L78) | **[확인된 사실]** table 소스 |

**[실측]** `CommandCounter`의 writer는 네 곳이다. `sample_app_cmds.c`의 81:35(Noop), 100:41(ResetCounters, 0으로), 122:35(Process), 157:35(DisplayParam)이다. reader는 81:35, 122:35, 157:35, 55:73이다 (`$W/dedupe_sites.out`). writer는 모두 `dispatch_case(6274, cc 0..3)`에서 도달한다.

**[해석]** 이 사슬로 §6이 물을 수 있는 질문은 예를 들어 이렇다.

- 6274 명령과 6275 요청은 같은 pipe에 들어간다. §3.9 A1 아래에서 HK telemetry의 `CommandCounter`는 6275보다 먼저 꺼낸 명령을 모두 반영한다. sample_app이 pipe를 하나만 만든다는 것은 `cfs.pipe_site`가 하나뿐인 데서 확인된다.
- TO_LAB의 MsgLim이 1이다. TO_LAB이 읽기 전에 2179가 두 번 오면 두 번째가 drop된다 (M4, SB-7).

**[확인된 사실]** sample_app에는 메시지 payload에서 state로 가는 흐름이 없다. `DisplayParamCmd`는 payload를 event로만 보낸다 ([sample_app_cmds.c L155-L166](https://github.com/nasa/sample_app/blob/199476a34827ae84d50d66f97619227854cd971a/fsw/src/sample_app_cmds.c#L155-L166)). state는 counter, 제어 값, handle뿐이다. **[해석]** 그래서 이 예제는 RQ1의 그래프 복원과 RQ2의 "state → telemetry payload" 방향만 보여 준다. payload → state 방향(원노트 §44)은 HK 같은 다른 앱으로 확인해야 한다.

#### 5.9.5 구조 검사 결과

**[실측]** `mlir-opt sample_app_lifted.mlir --irdl-file=cfs_irdl.mlir`는 rc 0으로 끝났다. 다시 출력한 IR은 170줄이다 (`$W/lifted.roundtrip.mlir`). 음성 시험 결과는 이렇다 (`$W/neg.out`).

| 시험 | 내용 | 결과 |
| --- | --- | --- |
| neg1 | raw i32를 `sb_receive`의 pipe로 넘김 | `error: expected base type 'cfs.pipe' but got 'builtin.integer'` |
| neg2 | `sb_subscribe`의 operand 순서를 바꿈 | `error: expected base type 'cfs.msgid' but got 'cfs.pipe'` |
| neg3 | `tbl_get_address`의 ptr 결과를 `!cfs.buf`로 씀 | 오류 없음 |
| neg_sig | `llvm.func`의 인자를 `!cfs.pipe`로 선언 | `custom op 'llvm.func' failed to construct function type: expected LLVM type for function arguments` |

**[실측] IRDL의 제약 (mlir-opt 1053047a).**

- op 이름에 `.`을 쓰면 "name of operation must contain only lowercase letters, digits and underscores"로 거부된다.
- `irdl.base "!llvm.ptr"`는 IRDL 파일을 읽을 때 "no registered type with name !llvm.ptr"로 실패한다. 그래서 ptr 제약을 `irdl.any`로 바꿨다. neg3이 통과한 이유가 이것이다.
- dialect 타입 둘에 대한 `irdl.any_of`는 `IRDLLoading.cpp:541`에서 mlir-opt를 abort시켰다 (`$W/cfs_irdl_anyof_crash.mlir`).
- 같은 constraint 변수를 두 속성에 쓰면 두 값이 같아야 한다. 속성마다 따로 `irdl.any`를 둬야 한다.
- IRDL op에는 `MemoryEffectOpInterface`가 없다 (§4.5.3). effect는 이 검사에서 전혀 확인되지 않았다.

**[해석]** 이 검사가 보이는 것은 타입 구분이 operand 혼동을 잡는다는 것뿐이다. effect, verifier V1–V11, lifting pass는 ODS·C++ 구현이 있어야 확인된다.

### 5.10 frontend별 정보 손실과 복원

| 잃는 정보 | 잃는 곳 | 복원 방법 **[설계 제안]** | 남는 위험 |
| --- | --- | --- | --- |
| MID macro 이름 | **[확인된 사실]** LLVM IR과 import 모두. DIMacro도 import가 버린다 (검증 C18 정정) | P3: `dbg.value(MsgIdValue)` 위치로 AST·preprocessor sidecar 조회 | **[미확인]** sidecar 미구현. 같은 줄·열에 macro 여러 개가 펼쳐질 때의 처리 |
| MID 상수의 자기 위치 | **[실측]** pipeline의 상수 hoisting | P3: `dbg.value` 위치, 또는 사용하는 call의 위치 | 같은 상수를 여러 site가 공유한다. site는 call 위치로 구별한다 |
| `-O0`의 MID 상수성 | **[실측]** alloca 경유 | `-O1 -Xclang -disable-llvm-passes` + helper inline. 또는 alloca 1단계 추적 (§7.7.1) | — |
| table·명령 유래 MID | **[확인된 사실]** 앱 코드에 값이 없다 (검증 C18) | P7: table 소스를 파싱해 `config_*` 생성. 명령 유래는 `cmd_payload`로 남김 | **[미확인]** 실행 중 적재된 table image와 소스의 일치 |
| field 이름 | **[실측]** DI가 정의 TU에만 있다 | 앱 단위 `llvm-link` 후 import | 여러 앱이 같은 이름의 다른 struct를 쓰면 앱별로 따로 연결해야 한다 **[해석]** |
| offset 0 field | **[확인된 사실]** GEP 없음 (§7.8) | P4: TBAA struct-path + DI. **[실측]** sample_app 31/31 | `-fno-strict-aliasing`이면 tag가 0개다 **[실측]** |
| scalar TBAA tag의 field | **[실측]** tag offset이 scalar 기준이다 | P4: GEP index와 DI member 순서 | 동적 index(`TblHandles[i]`)는 배열 원소를 구별하지 못한다 |
| out-parameter 쓰기 | **[확인된 사실]** 모든 frontend에서 store가 없다 | P2: D2 명시화 | API 표에 없는 함수의 out-parameter는 남는다 (L5) |
| static inline helper 경계 | **[실측]** pipeline에서 inline되어 이름이 사라진다 | P2: operand 출처로 `icmp eq` → `mid.equal` | 출처를 모르면 일반 icmp로 남는다 |
| handler 경계 | **[실측]** 기본 `--inline`이 handler를 흡수한다 (156 → 45 사이트) | P0: 앱 함수 `noinline` | **[미확인]** 다른 앱에서의 효과 |
| macro API (`PerfLogEntry`, `CFE_MSG_PTR`) | **[확인된 사실]** 전처리 | P2: 펼친 형태(`PerfLogAdd(id, 0)`, GEP)로 인식 | — |
| visibility | **[확인된 사실]** importer가 정하지 않는다 (검증 C20) | P1 | 함수 포인터로만 불리는 함수를 entry로 놓칠 수 있다 |
| 간접 호출 대상 | **[확인된 사실]** `<Unknown-Callee-Node>` (검증 C20 (d)) | points-to (§4.5.2의 SVF, PoTATo 등) | **[실측]** PoTATo는 local LLVM에 빌드되지 않았다 (§4.5.1) |
| task 문맥 | **[확인된 사실]** 코드에 task 경계가 없다 | P7: startup script, `CreateChildTask` 상수 entry | entry가 상수가 아니면 child task를 놓친다 |
| EDS MID | **[확인된 사실]** 실행 중 함수 호출 (§7.7.3) | `runtime_fn`으로 표시하거나 설정별 평가 | **[미확인]** IR 형태 미관찰 |
| ClangIR의 정보 | **[실측]** 로컬 사용 불가 (검증 C19) | 없음 | 장래 경로는 문서 수준에서만 비교했다 |

### 5.11 한계와 미확인

- **[설계 제안]** 이 절의 dialect와 pass는 구현되지 않았다. 손으로 쓴 lifted IR은 목표 형태를 보일 뿐이다. 자동 lifting이 같은 결과를 낸다는 증거는 없다.
- **[실측]** 실측 대상은 sample_app 하나다. TBAA·`noinline` 결과를 hk, to_lab, sch_lab, ci_lab과 mission 전체에 적용하는 것은 **[미확인]**이다.
- **[확인된 사실]** local `1053047a`에는 비주소 resource가 없다. 설계의 effect 이점을 쓰려면 main 위계로 옮기거나 domain alias 구현을 더해야 한다 (§5.4).
- **[해석]** sample_app은 counter state만 갖는다. RQ2의 payload → state 출처는 이 예제로 판단할 수 없다.
- **[미확인]** validator callback의 task 문맥, child task의 entry 해석, 실행 중 table image와 소스의 일치는 확인하지 않았다.
- **[해석]** 지역 포인터를 통한 state 쓰기(sch_lab)는 TBAA와 DI로 풀리지 않는다. points-to가 필요하다.

### 5.12 원노트·수정본 대비 정리

| 원노트·수정본의 서술 | 이 절의 처리 |
| --- | --- |
| **[원노트 구상]** §7의 `cfs.sb.create_pipe name="GUIDANCE_PIPE" depth=32`처럼 이름과 depth를 속성으로 둔다 | **[설계 제안]** depth와 이름은 operand로 둔다. 상수일 때만 `cfs.pipe_site`의 속성으로 복사한다. 실제 코드는 depth를 macro 상수로 넘긴다 ([sample_app_internal_cfg.h L42](https://github.com/nasa/sample_app/blob/199476a34827ae84d50d66f97619227854cd971a/fsw/inc/sample_app_internal_cfg.h#L42)). |
| **[원노트 구상]** §7의 `#cfs.mid<ATTITUDE>`처럼 이름이 MID의 정체다 | **[설계 제안]** 정체는 값이다. 이름은 선택적 부가 정보다. **[확인된 사실]** IR에는 값만 남는다 (검증 C18). |
| **[원노트 구상]** §10의 초기 frontend는 C → LLVM IR → MLIR LLVM dialect, 장기 frontend는 CIR | **[실측]** 초기 경로는 작동한다. sample_app에서 MID 상수, field 이름, handler 경계를 함께 얻은 조건은 `-O1 -Xclang -disable-llvm-passes`, helper inline, 앱 함수 `noinline`, 앱 단위 연결이었다 (§5.7.2–§5.7.4). CIR는 로컬에서 시험할 수 없다 (검증 C19). |
| 수정본 §9.1: `cfs.*` 이름은 검증되지 않은 설계 | 그대로 유지한다. 이 절도 구조 검사(IRDL)만 했다. |
| 수정본 §9.2: MLIR이 이 문제를 자동으로 풀지 않는다 | **[확인된 사실]** 유지한다. effect·alias·dispatch·MID 복원은 모두 이 연구가 만들어야 한다 (검증 C20, §5.4–§5.8). |
| 수정본 §11.2: MVP API 집합에 MID·dispatch·status가 필요하다 | **[설계 제안]** status는 모든 op의 결과로, MID는 타입과 속성으로, dispatch는 `cfs.dispatch_case`로 반영했다. **[확인된 사실]** HK와 TO_LAB은 table 유래 MID가 많다 (검증 C18). 그래서 MVP에 P7(table 소스)을 포함해야 한다. |

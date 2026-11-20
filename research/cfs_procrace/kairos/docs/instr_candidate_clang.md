# 컴파일러 연결부 후보: Clang AST 소스 변환 → gcc 빌드 (단계 C 제한된 적합성 확인, 2026-10-26)

- 범위: IR §5.2, §13.2, §15 단계 C, §15.1의 일곱 질문을 **이 후보 하나**에 대해 답한다. IR은 2026-10-26 계측 아키텍처 검토(사용자 업로드)다. 세 후보의 비교·선택은 하지 않는다(PD26 §2 항목 3: 세 후보를 같은 C1·R2 관측점에서 비교한 뒤 하나를 고른다). 근거: `Architecture/spec/PI_DECISIONS_RESOLVED_20261026.md`(PD26) §2·§4(구속), SPEC rev 1.4 §4.1 "Clang as a source transformer", §5.12, `docs/event_semantics.md`(런타임 ABI 1, manifest `kzs-sites 1`, dump 1.0).
- 표기: **[F]** 소스·문서에서 확인, **[M]** 이번에 빌드·실행해 확인, **[I]** 추론, **[D]** 설계 결정, **[U]** 미확인.
- 지위: **pilot/dev**. 파생(계측) 빌드 두 개와 기동 한 번씩이다. 본 실험 결과가 아니고 연구 조건을 정하지 않았다. 빌드·실행 조건은 레퍼런스 절차를 따르고, 다른 점은 모두 아래 §4·§5에 적었다(CONDITIONS_POLICY). 참조 트리(`c1_build`, `cfs_ref/cFS`, `refs`)는 읽기만 했다.
- 코드·자료: `kairos/instrument/clang/`(도구 `kz_instr_clang.cpp`, 사이트 `sites/*.kzs`, 스크립트 `scripts/`, 시험 `tests/`, 산출물 `out/{c1,r2}/`). 빌드 트리·실행 기록은 저장소 밖 `/home/user/work/instr/clang/`.

## 1. 요약 (IR §15.1의 일곱 질문)

| # | 질문 | 답 | 근거 |
|---|---|---|---|
| 1 | 공개 cFS 구성과 대상을 빌드하는가 | **예 (x86-64 Linux, gcc 13.3).** C1(Release `-O3`, BP CI 구성)과 R2(native_std, debug `-O0`)를 레퍼런스 명령·플래그로 빌드했다. 변환하지 않은 목적 파일은 C1에서 238개가 바이트 동일, 19개는 경로·버전·빌드 시각 문자열만 다르다. R2에서 794개가 `.text` 동일이고 경로 문자열만 다르다 | §4 [M] |
| 2 | 실제 값·반환·공개 지점을 부작용 없이 전달하는가 | **예, 조건부.** 인자 식은 한 번만 평가된다(래퍼 매개변수 또는 인자 탭). C가 정하지 않은 평가 순서는 그대로 둔다. 값은 원 코드가 쓴 operand다. 원 코드에 없는 읽기는 셋이다: out 매개변수를 반환 직후 같은 스레드가 읽는 것, 검증된 문자열을 상한 안에서 복사하는 것, 반환 지점에서 매개변수 값을 다시 읽는 것(본문에서 대입되면 거부). `volatile`·atomic은 바꾸지 않는다 | §2, 시험 `instr_clang_transform` [M] |
| 3 | source 위치와 실행 지점의 대응을 설명하는가 | **예.** binding마다 `파일:줄:열`·형태(호출 지점 래퍼/탭, 피호출 진입/반환, store 탭)가 manifest에 있고, 기계어 조사(`hook_census.sh`)로 28+16개 binding 모두 실제 호출로 남았음을 확인했다. C1 BP 4개는 `BP_FlowInit`에 인라인됐다. 복제·제거는 없었다 | §3 [M] |
| 4 | 초기화 전 관측을 지원하는가 | **경로는 있음, 이번 실행에서 사건은 없음.** 변환은 OSAL·PSP 초기화 코드에도 넣을 수 있다. 런타임의 부트스트랩 버퍼가 attach 전 사건을 받는다(B1 단위 시험). 이번 두 실행에서는 시작 객체가 `main` 전에 attach해서 공유 버퍼가 0바이트였다. 모듈 등록 생성자는 attach 전에 실행되어 actor 없이(t=0) 기록됐다 | §5 [M], [U] |
| 5 | 필요한 미계측 경로를 확인할 수 있는가 | **예, 정적으로 목록화.** 보고서가 매크로 안 반환 경로(C1 10, R2 5: `OS_CHECK_*`의 오류 반환), 매크로 본문·인자 안 호출, 비활성 전처리 구역에서 이름이 나오는 곳, compile DB에 없는 TU, 이 구성에서 정의되지 않은 함수를 적는다. 함수 포인터 호출이나 사이트로 고르지 않은 호출 지점은 피호출 측 binding으로만 덮인다 | §2.3, §3 [M] |
| 6 | 같은 runtime에 연결되는가 | **예.** `kz_rt.h` hook ABI 1을 그대로 썼고 런타임은 고치지 않았다. POSIX adapter와 시작 객체를 썼다. `kz-rt-dump --manifest`의 엄격 검사가 C1 tx·rx와 R2 dump에서 실패 0이다 | §5 [M] |
| 7 | frontend·ABI·모델 작업이 얼마나 더 생기는가 | **frontend·ABI·모델 작업은 없다.** 기존 LLVM 빌드의 Clang을 쓰고 `kz-static`과 같은 방식으로 compile DB를 읽는다. 대신 사이트마다 manifest를 쓰고(값·문자열 검증 근거·phase), 빌드 시스템 한 줄(런타임 링크)과 `CFLAGS`(include 경로)를 고쳐야 한다. 단위 시험 빌드는 따로 처리해야 하고, 가변 인자 호출은 인자 탭이 필요하다 | §8 [M][I] |

판단 근거(IR §15.1 규칙): 이 후보는 GCC 내부 API·plugin 배포 문제가 없고 산출물이 검토 가능한 C 소스 diff다. 반면 매크로 텍스트와 비활성 구성은 편집하지 못한다. 선택은 세 후보의 같은 표가 모인 뒤에 한다 [D: PD26 §2 항목 3].

## 2. 변환기 `kz-instr-clang` [D][M]

### 2.1 동작

1. 논리 사이트 manifest(`kzs-sites 1`, 런타임 파서 `kzrt-manifest`로 읽음)의 `site … tu= func= [callee=] [line=] phase=`와 `value … from= type= [string=validated bound= ref=]`가 규칙이다. 도구 안에는 cFS 지식이 없다.
2. 사이트가 가리키는 TU를 **참조 빌드가 기록한 명령**(`compile_commands.json`)으로 파싱한다. 분석 전용 조정은 `kz-static`과 같다(`-Wno-unknown-warning-option -w -resource-dir`). 여러 명령이 같은 TU를 컴파일하면(R2의 cpu1·cpu2) 명령마다 변환해 결과 텍스트가 같은지 확인한다. 다르면 거부한다.
3. 결과는 **별도 소스 트리**(참조 트리의 사본, `.git` 제외)의 같은 상대 경로에 쓴다. 참조 소스에는 쓰지 않는다. 산출물은 binding 목록(`kzs-sites 1`의 `bind` 줄), 보고서, 그리고 스크립트가 만드는 소스 diff다.

### 2.2 binding 형태 (IR §5.2의 구분)

| 형태 | 언제 | 무엇을 바꾸나 | 기록 시점과 값 |
|---|---|---|---|
| **호출 지점 래퍼** | `callee=`가 있고 피호출이 가변 인자가 아님 | 호출의 함수 이름 토큰 하나만 `kz_w<bind>`로 바꾼다. 래퍼(`static inline`)는 피호출의 매개변수 형으로 감싼 함수 앞에 정의한다 | before: 인자 평가 뒤·호출 직전의 sequence point(래퍼 진입). after: 반환 직후. ret = 반환값, out*k* = 같은 스레드가 곧바로 읽은 `*p`(p가 NULL이 아닐 때만, 아니면 valid 비트 없음) |
| **호출 지점 탭** | `callee=`가 있고 피호출이 가변 인자(R2의 `CFE_EVS_SendEventWithAppID`) | 잡을 고정 인자 E를 `kz_t<b>_<k>(&kz_l<b>, (E))`로, 호출 전체를 `kz_r<b>(&kz_l<b>, (call))`로 감싼다 | 인자마다 그 인자가 평가될 때 레코드 하나. 인자 사이 순서는 C처럼 정하지 않는다(실제로 gcc는 오른쪽부터 평가했다, 시험 기록). after의 link는 마지막에 평가된 인자의 레코드다. out*k*는 없다 |
| **피호출 진입** | `callee=` 없음, phase=before | 본문 `{` 바로 뒤에 레코드 하나(같은 줄) | 매개변수 값(매개변수 읽기에는 부작용이 없다) |
| **피호출 반환** | `callee=` 없음, phase=after | 파일 텍스트인 `return E;`를 `{ T kz_r = (E); 기록; return kz_r; }`로 바꾼다(세 군데 국소 편집, E의 텍스트는 그대로). void 함수는 닫는 `}` 앞에도 기록 | 반환값(변환은 `return`이 하는 것과 같다). 매크로 안의 `return`은 바꾸지 않고 **미계측 경로**로 보고한다. 반환 지점에서 읽는 매개변수가 본문에서 대입되거나 주소가 잡히면 거부한다 |
| **store 탭** | phase=effect, `from=operand0`, `line=` | 그 줄의 단순 대입 `L = R`의 R을 `kz_s<b>((R))`로 감싼다 | R이 계산된 뒤, 저장 전. 변환은 대입의 것과 같다 |

공통 규칙:
- **두 번 평가 금지, 순서 고정 금지.** 래퍼는 인자를 매개변수로 한 번 받는다. 탭은 그 자리에서 값을 그대로 돌려준다. 문(statement)으로 인자를 미리 끌어올리지 않는다. 시험의 `tcase.c`에서 부작용 있는 인자(`i++`, `j++`, `next_id()`, `k++`)가 한 번씩만 실행됨을 확인했다 [M].
- **매크로.** 이름 토큰이 매크로 본문이나 매크로 인자에서 온 호출은 편집하지 않고 `unsupported`로 쓴다(인자에 쓰인 호출은 매크로가 `#x`로 문자열화하거나 두 번 쓸 수 있다). 파일에 쓰인 매크로 사용 전체를 감싸는 것은 허용한다(예: `return API_ERR;`, `bool` 반환형) [M].
- **줄 번호와 `__func__`.** 함수 앞에 넣는 여러 줄 텍스트 뒤에는 `#line N`을 둔다. 나머지 편집은 같은 줄 안에서 한다. 그래서 `__LINE__`과 진단 줄이 원본과 같다(시험: `__LINE__` 출력 동일). 함수 이름을 바꾸지 않으므로 `__func__`도 같다. `__FILE__`은 사본의 경로가 되어 다르다(§4) [M].
- **include.** `#include "kz_rt.h"`는 첫 계측 함수 앞에 넣는다. TU 맨 위에 넣으면 `_GNU_SOURCE` 같은 기능 매크로보다 시스템 헤더가 먼저 들어간다. 그래서 피했다 [D].
- **모듈 등록.** 모듈마다 첫 편집 TU 끝에 생성자 하나를 둔다. 이 생성자가 `kz_rt_module(id, 0)`을 호출해 "적재·실행 안 됨"과 구별한다(GNU `__attribute__((constructor))`) [D].
- 생성 텍스트는 C99에 GNU 속성 하나만 쓴다. C1의 `-std=c99 -pedantic -Wall -Wstrict-prototypes -Wwrite-strings -Wpointer-arith -Wcast-align -Werror -O3`와 R2의 debug 플래그에서 경고 없이 컴파일됐다(빌드 로그의 경고 수는 참조와 같은 1, DV-P1) [M].

### 2.3 보고되는 한계 (조건부 컴파일·매크로)

- 파싱한 구성의 **활성 분기만** 보인다. 다른 구성(다른 `-D`)은 다시 변환해야 한다. 비활성 구역에 피호출·함수 이름이 나오면 보고서에 `inactive`로 적는다. R2에서는 비행 명령과 단위 시험 명령(`-D_UNIT_TEST_`)이 같은 텍스트를 냈다(cpu1·cpu2도 같다) [M]. C1·R2 대상 TU에는 해당 비활성 구역이 없었다 [M].
- **매크로 안 호출 비율.** SPEC의 12.4%(R07 §4.6)는 FSW 전체 API 호출 기준이다. 이번에 고른 호출 지점은 모두 파일 텍스트였다(보고서 census: 각 TU에서 해당 피호출의 호출 이름이 매크로에서 온 것 0). 매크로가 실제로 막은 것은 **반환 경로**다. OSAL의 `OS_CHECK_POINTER`·`OS_CHECK_APINAME`·`OS_CHECK_SIZE`(→ `BUGCHECK`/`ARGCHECK`)가 오류 반환을 매크로 안에 둔다. C1에서는 `OS_CountSemGetIdByName` 2개, `OS_CountSemCreate` 3개, `OS_TaskCreate` 5개가 미계측이고, R2에서는 `OS_TaskCreate` 5개다. 이 경로로 끝난 호출은 after 레코드가 없다(시험의 `api_lookup(0, …)`) [M].

### 2.4 시험

`ctest -L instr`(LLVM 빌드, `instr_clang_transform`)는 다음을 확인한다. `tcase.c`를 변환하고 C1과 같은 경고 플래그로 gcc 빌드한 뒤 런타임에 링크해 실행한다. 원본과 출력이 같은지(부작용·결과·`__LINE__`)를 본다. `kz-rt-dump --manifest` 검사가 통과하는지, 사이트별 레코드 수(매크로 본문 호출 0, 매크로 반환 경로 미기록, 탭은 인자마다 1)·문자열·값을 확인한다. 미계측 반환 2개와 비활성 구역 보고도 확인한다.

## 3. 대상 사이트와 binding

사이트 파일: `instrument/clang/sites/c1_post_1df262a.kzs`(28 사이트), `sites/r2_cfs_v701.kzs`(16 사이트). 의미와 인용은 F0 계획 `profiles/c1-bp-b051eb0/observe/sites-cf833fdbb.kzp`, `profiles/cfs-v7.0.1/observe/sites.kzp`를 따랐다. label은 지점이 같을 때만 F0와 같게 했다(함수 진입·반환, 호출 전·후). 세 후보가 같은 논리 사이트를 쓰도록 `profiles/`로 옮기는 것은 비교 단계의 일이다 [I]. 두 백엔드의 동등성은 `equiv … unconfirmed`이며, 근거로 확인하기 전까지는 확정하지 않는다(event_semantics §3).

### 3.1 C1 (post-fix CF 833fdbb + BP b051eb0, cFS 1df262a)

| 논리 지점 | F0 사이트(smoke3) | 이 연결부의 binding (`out/c1/binds.kzs`) | 기계어 위치 (`out/c1/hook_census.txt`) |
|---|---|---|---|
| CF 조회 호출 전: 이름·out 포인터 | (없음; 진입 사이트가 대신) | 10 래퍼 before `cf_cfdp.c:1003:23`, 이름은 표 배열 `char sem_name[OS_MAX_API_NAME]`이라 validated(상한 20) | cf.so `CF_CFDP_InitEngine` |
| CF가 받은 상태·id | `cf_lookup_call`(상태) | 11 래퍼 after: ret, out0(sem_id) | 같음 |
| 재시도 지연 | `cf_retry_delay` | 12 래퍼 after `cf_cfdp.c:1011` | 같음 |
| CF 초기화 결과 | `cf.init.ret`, `cf.init_engine.ret` | 14, 13 피호출 반환(경로 1/1) | `CF_Init`, `CF_CFDP_InitEngine` |
| CF 종료 경로 | `es.exit_app.entry`(피호출) | 15 래퍼 before `cf_app.c:366`(호출 지점), 40 피호출 진입 `CFE_ES_ExitApp` | cf.so `CF_AppMain`, core `CFE_ES_ExitApp` |
| BP 존재 확인 | `bp_existence_probe_call`(상태) | 20 before(이름 validated, `Name[BP_SEM_NAME_SIZE]`), 21 after(ret, out0) | bp.so **`BP_FlowInit`에 인라인**(소스 함수는 static `initialize_throttling`) → `status=inlined` |
| BP 생성 | `bp_create_call`(상태) | 22 before(이름, 초기값), 23 after(ret, out0) | 같음, inlined |
| BP 초기화 결과 | `bp.flow_init.ret` | 24 피호출 반환(경로 2/2; 같은 bind가 두 반환 지점에 있고 `pc`로 가른다) | `BP_FlowInit` 2곳 |
| 조회 API(모든 호출자) | `osal.countsem_lookup.entry/.ret` | 30 진입(포인터만: 이름은 아직 검증 전), 31 반환(ret, out0; **미계측 반환 2**) | core `OS_CountSemGetIdByName` |
| 조회의 전제 지점 | `OS_CountSemGetIdByName.exists`(꼬리 점프 진입) | 32 래퍼 before `osapi-countsem.c:212`(`OS_CHECK_POINTER(sem_name)` 뒤라 이름 validated) | 같음 |
| 찾은 id | `osal.countsem_lookup.found`(토큰 obj_id) | 33 래퍼 after(ret, out2) / 34 **store 탭** `osapi-idmap.c:978 *object_id = token.obj_id`(모든 객체 형의 이름 조회) | `OS_CountSemGetIdByName`, `OS_ObjectIdFindByName` |
| 생성 API | `osal.countsem_create.entry/.ret` | 35 진입, 36 반환(**미계측 반환 3**) | `OS_CountSemCreate` |
| 생성의 공개 지점 | `OS_CountSemCreate.publish`, `.created` | 37 래퍼 before `OS_ObjectIdFinalizeNew`(operation_status), 38 after(ret, out2 = 공개된 id) | 같음 |
| 생성 측 정체성 | `es.start_task.*`, `es.child_task.*`, `osal.task_create.*`, `osal.task_entry`, `osal.task_create.id` | 41–47: 요청(이름 포인터·진입 함수·ParentAppId), 반환(상태·배정 id), `OS_TaskCreate` 반환에서 **task 이름**(validated: 유일한 계측 반환 앞에 `OS_CHECK_APINAME`), task 진입(task_id). `osal.task_create.id`(새 스레드에 넘기는 인자)는 `OS_TaskCreate`의 out0로 대신했다 | core |

### 3.2 R2 (cFE v7.0.1 ENV 트리)

| 논리 지점 | F0 사이트(pilot3) | binding (`out/r2/binds.kzs`) | 기계어 |
|---|---|---|---|
| SB가 넘긴 AppID (3개 호출 지점) | `CFE_EVS_SendEventWithAppID.id_valid@sb_api_275/1183/sb_priv_786`(at-call 레지스터) | 50/52/54 **탭**(가변 인자): event_id(arg0), app_id(arg2) 인자마다 1 레코드. 51/53/55 after(ret) | debug `-O0`: 탭·래퍼 함수가 인라인되지 않아 `kz_t*`·`kz_r*` 안에 hook 호출 |
| SB의 자기 AppId 저장 | `sb_publication`(상태만) | 56 래퍼 after `cfe_sb_task.c:138`: ret, out0 = **저장된 AppId 값** | `kz_w65549` |
| 전송 함수(모든 호출자) | `evs.send.entry/.ret` | 57 진입(event_id, event_type, app_id), 58 반환(경로 2/2; 가변 인자 함수도 반환문 편집은 된다) | `CFE_EVS_SendEventWithAppID` |
| 생성 측 정체성 | 21–25 | 61–67(C1과 같은 형태, v7.0.1 줄) | core |

`cfe_sb_priv.c`의 함수는 반환형 `bool`(매크로)로 시작한다. 처음에는 "함수가 줄을 시작하지 않음"으로 거부됐고, 파일에 쓰인 매크로 사용 전체는 허용하도록 고쳤다 [M].

## 4. 파생 빌드: 참조와의 차이 (모두 기록)

| 항목 | C1 (`scripts/build_c1.sh`) | R2 (`scripts/build_r2.sh`) | 상태 |
|---|---|---|---|
| 절차 | `c1_build/build_one.sh`의 cmake·make 명령, `prep_defs.sh`(참조 스크립트 그대로) | `make native_std.prep`·`native_std.install`(README L104–105)를 `ubuntu`로 | from-reference |
| 소스 | 참조 트리의 사본(`.git` 제외, 복사 직후 `diff -r` 동일) + 변환 8 TU / 7 TU | 같음 | deviation (계측) |
| `CFLAGS` | `-Wno-error=restrict`(DV-P1) + `-I<kairos>/runtime/include` | `-I<kairos>/runtime/include`(참조는 CFLAGS 없음) | deviation |
| 런타임 링크 | 사본의 `cfe/cmake/target/CMakeLists.txt` 끝에 `target_link_libraries(core-${TGTNAME} -Wl,--whole-archive libkzrt-posix.a libkzrt.a -Wl,--no-whole-archive kz_rt_posix_auto.c.o)` | 같음 | deviation |
| 단위 시험 | 참조도 끔 | `ENABLE_UNIT_TESTS=FALSE`(참조는 TRUE). 커버리지 시험이 같은 소스를 컴파일하므로 런타임 링크가 더 필요하다. 변환 텍스트는 단위 시험 명령에서도 같았다 | deviation (R2) |
| 런타임 | `cmake -S kairos/runtime -DCMAKE_BUILD_TYPE=Release`(gcc 13.3), `/home/user/work/instr/clang/rt` | 같은 빌드 | 계측 부분 |
| 모듈 버전 문자열 | 사본에 `.git`이 없어 `git describe` 문자열이 빈다(참조: `git:draco-rc3-45-g1df262a-dirty` 등) | 같음(참조는 git clone) | deviation |
| 확인 결과 (`out/*/build_diff.txt`) | compile 명령 289개: 추가 `-I`를 빼면 동일. 목적 파일 265개: 동일 238, 경로·버전·시각 문자열만 다름 19, 변환한 8개만 다름. 링크 명령: `-I` 텍스트와 core의 런타임 줄만 다름. 빌드 경고 수 참조와 같음(1) | 비행 compile 명령: 파생 479개 중 477개가 참조 명령과 같고, 2개는 경로 해시가 든 표 타깃 이름만 다름. 참조에만 있는 940개는 단위 시험·stub·커버리지. 비행 목적 파일: 794개가 `.text` 동일·경로 문자열만 다름, 변환한 7 TU × 2 CPU = 14개만 다름 | [M] |

L-A(비계측 Clang 빌드)는 이 후보에 해당하지 않는다. 실행 코드는 참조와 같은 gcc가 만든다. 그래서 비계측 기준은 G-A(참조 빌드) 자체이며, 위 표가 변환한 TU 밖에서 파생 빌드가 G-A와 같음을 보인다 [M]. 이번 실행은 IR §13.1의 G-I(같은 빌드, 기록 활성, gate 비활성)에 해당한다. G-I0(`KZ_RT_MODE=off`)는 돌리지 않았다 [U].

## 5. pilot 기동 (각 1회)

| | C1 tx (`scripts/run_c1_pilot.sh`) | R2 (`scripts/run_r2_pilot.sh`) |
|---|---|---|
| 절차 | `observe_c1_smoke.sh --instances`의 단계(tar/untar를 `ubuntu`로, tx·rx 백그라운드, 5 s, OPERATIONAL 확인, UDP 1234/1235 ES 재시작 POWERON, 5 s), F0 관측기 없음 | `observe_cfs_smoke.sh` 단계 4의 명령 `env/run_cfs.sh operational ci-es-restart-poweron`(`CFS_DIR`=파생 트리), 영속 상태 스냅숏 전후, F0 관측기 없음 |
| 더한 환경 이름 | `KZ_RT_MODE=record KZ_RT_DUMP KZ_RT_RUN_ID KZ_RT_INSTANCE`(tx 1, rx 2). rx도 파생 빌드로 돌고 기록한다(F0에서 rx는 비관측) | 같은 넷(instance 1). `run_cfs.sh`가 이름만 기록 |
| 결과 | 2026-10-26T06:53:39Z, tx·rx OPERATIONAL(5 s), stop 성공, 종료 0·0, dump tx 14,428 B / rx 14,428 B | 2026-10-26T07:01:45Z, POWER ON reset, OPERATIONAL 줄 1,654 ms(pilot3는 1,628 ms), 종료 0, `POWERON`, dump 117,032 B |
| dump 검사 | `kz-rt-dump --manifest sites --manifest binds`: tx 139 사건·actor 16, rx 139 사건, actor별 aseq 빈틈 0, LOSS 없음, 공유(부트스트랩) 버퍼 0 B, 검사 실패 0 | 1,298 사건, actor 25, 빈틈 0, LOSS 없음, 공유 버퍼 0 B, 검사 실패 0 |
| 그 밖의 차이 | 실행 층 미확립(파일을 먼저 읽음) | 파생 exe 디렉터리가 새것이라 `EEPROM.DAT`·키 파일이 실행 전에 없었다(PSP가 만듦; pilot3는 0으로 찬 EEPROM과 빈 키 파일). 같은 시각에 다른 작업의 빌드가 호스트에서 돌았다(`run_cfs.out`의 프로세스 목록, loadavg 1.14) |

- 종료 경로의 자동 dump(B1 열린 항목): 두 구성 모두 ES 재시작 POWERON 뒤 정상 `exit`로 끝나서 `atexit` dump가 써졌다 [M]. 시그널로 끝나는 경로는 확인하지 않았다 [U].
- 실행 기록과 dump는 저장소 밖 `/home/user/work/instr/clang/runs/{c1_post_1df262a_p1,r2_v701_p1}/`에 있다.

## 6. F0 기록과의 대조 (IR §13.2 순서)

비교 대상: C1 `/home/user/work/procrace/c1_runs/smoke3/post_1df262a`(tx, 158 사건, 손실 0), R2 `/home/user/work/procrace/r2_runs/v153_pilot3`(1,271 사건, 손실 0). 같은 구성의 **다른 실행**이다. 횟수가 같기를 요구하지 않는다(IR §13.2).

### 6.1 C1

| 단위 | 결과 |
|---|---|
| 위치 대응 | F0 사이트 34개 중 계획한 의미 지점은 모두 binding이 있다(§3.1). F0의 시작 상태·삭제·종료 범위 사이트(19–25: `OS_ObjectIdInit.clear_table`, `OS_CountSemDelete.*`, `CFE_PSP_Restart.request_shutdown`, `OS_Application_Run.teardown`)는 이번 대상 목록에 없어 넣지 않았다(같은 형태로 넣을 수 있다 [I]). F0의 `found`(토큰 해제 진입)와 `created`(통지 진입)는 지점이 달라 다른 label(33/34, 38)로 두었다 |
| 도달 | 횟수가 F0와 같다: CF 조회 3(실패 1 → 지연 1 → 성공 2), BP 존재 확인 2, 생성 2, 조회 API 5, 생성 API 3(SCH_LAB 1 포함), start_task 15, child_task 5, task_create 15, task_entry 15, `CFE_ES_ExitApp` 0(CF가 정상 초기화). store 탭은 4(이름 조회 전체: 세마포어 2 + 다른 형 2) |
| 포획 적합성 | 값이 F0와 같다: CF 첫 조회 `-17`(`OS_ERR_NAME_NOT_FOUND`) 다음 `BP0_tsem` → `0x30002`, `BP1_tsem` → `0x30003`. BP 존재 확인 `-17` ×2, 생성 `0` → `0x30002`·`0x30003`, SCH_LAB `0x30001`. 이름은 F0가 커널 fetch로 읽었고, 여기서는 원 코드가 검증한 뒤 상한 복사로 읽었다(조회 진입에서는 포인터만). CF 호출 지점의 out id(11)와 내부 store 값(34)이 같다. F0에는 CF 호출 지점의 id가 없었다 |
| 식별 적합성 | actor(스레드 셀) ↔ task id(`osal.task_entry`) ↔ 이름(`osal.task_create.ret`의 이름)이 F0의 thread ↔ task id 대응과 같다: CF `0x1000c`, BP `0x1000d`, BP_CLA_OUT `0x1000e`, BP_CLA_IN `0x1000f` 등. 소유 앱(`es.start_task.entry` owner_id)도 F0와 같다(CF `0x110009`, BP `0x11000a`, 자식은 생성 task의 앱). 주 스레드는 생성 요청 없이 생겨 소유 앱이 미확정이다(F0와 같은 범주; AppID 0으로 채우지 않음) |
| 기록 완전성 | 손실 0, aseq 빈틈 0, 미계측 경로는 정적으로 목록화(§2.3)했고 이번 실행에서는 그 경로를 지나지 않았다(모든 진입에 반환 레코드가 짝지어짐 [M]) |
| 판정 근거 | 직접 값(상태·이름·id)은 F0와 같은 것을 준다. 순서는 actor 내부 aseq와 단일 시계 시각이다. 서로 다른 actor 사이의 시각 순서는 HB가 아니다(F0도 같은 단일 시계 증거). dump → `.kzt` 변환기가 없어 `kz replay` 판정은 다시 하지 않았다 [U] |

### 6.2 R2

| 단위 | 결과 |
|---|---|
| 위치 대응 | F0의 전송 진입·반환, SB 세 호출 지점, `sb_publication`, 생성 측 사이트는 binding이 있다. F0 사이트 8–20, 30–34(등록·writer·시작 상태)는 이번 대상 목록 밖이라 넣지 않았다 [I: 같은 형태로 가능] |
| 도달 | 전송 274/274(F0 269/269), SB 호출 지점 275: 26(F0 26), 1183: 111(F0 111), 786: 70(F0 65), `sb_publication` 1(1), start_task 24(24), child_task 4(4), task_create 24(24), task_entry 24(24) |
| 포획 적합성 | 넘긴 AppID 값: 275 → `0x0` 2·`0x110003` 24(F0 같음), 1183 → `0x0` 4·`0x110003` 107(F0 같음), 786 → `0x0` 5·`0x110003` 65(F0 `0x0` 18·`0x110003` 47). SB가 저장한 값 `0x110003`을 **저장 직후 직접** 얻었다(F0는 상태만). 탭은 원 호출이 넘긴 인자 그대로다 |
| 식별 적합성 | 24 task 모두 actor ↔ task id ↔ 이름이 이어진다. AppID 0을 넘긴 actor는 CFE_ES·CFE_EVS뿐이다(F0와 같은 두 task) |
| 기록 완전성 | 손실 0, 빈틈 0. SB 저장보다 앞선(단일 시계) 전송은 **11건(ES 7, EVS 4)**, F0 pilot3는 24건(ES 20, EVS 4)이다. 차이는 모두 `cfe_sb_priv.c:786`(ES 4 대 17)에서 났다. 275·1183은 같다(ES 1·2, EVS 1·2). 실행이 하나씩이고 관측 방식이 달라서 원인(스케줄 차이, 관측 섭동)은 가를 수 없다. 24는 합격 상수가 아니다(IR §13.2) |
| 판정 근거 | 위반의 직접 근거(넘긴 AppID 0과 호출 지점·task)는 두 계열이 같은 종류로 준다. 저장 값이 직접 값이 된 것은 이 연결부가 더한 것이다. 상태 근거(등록 slot)는 이번 사이트에 없다 |

## 7. 노력과 위험

- 노력 [M]: 도구 C++ 약 1,180줄(주석 포함), 스크립트 약 220줄, 사이트 파일 2개. 변환 시간은 C1 8 TU 0.3 s다. 빌드는 C1 약 17 s, R2(단위 시험 제외) 약 76 s 걸렸다.
- 위험·한계:
  1. 매크로 텍스트(본문·인자)와 비활성 구성은 편집하지 못한다. OSAL의 오류 반환이 대표적이다(§2.3). 매크로 정의를 고치는 방식은 모든 사용처를 바꾸므로 쓰지 않았다 [D].
  2. 구성마다 다시 변환해야 한다. 변환 결과는 그 구성의 전처리 결과에 의존한다(cpu1/cpu2·단위 시험 명령은 같았다).
  3. 원 코드에 없는 읽기(out 포인터, 문자열 복사, 반환 지점의 매개변수)가 들어간다. 다른 스레드가 그 사이에 쓰면 반환 직후 값과 피호출이 쓴 값이 다를 수 있다. 저장 operand가 필요한 곳은 store 탭으로 원 값을 잡는다(C1 34).
  4. `-O0`(R2)에서는 래퍼·탭이 별도 함수로 남아 호출이 늘고 코드 배치가 달라진다. `-O3`(C1)에서는 인라인된다.
  5. 탭 레코드 사이 순서는 정해지지 않는다(인자 평가 순서). before 레코드가 인자마다 나뉜다.
  6. 빌드 시스템 편집(런타임 링크), `CFLAGS`, 단위 시험 빌드, 모듈 버전 문자열(`.git` 없음), `__FILE__` 경로가 파생 구성의 차이로 남는다.
  7. 생성자 속성(GNU)과 `#line`에 기댄다. 다른 컴파일러는 확인하지 않았다 [U].
  8. Clang이 gcc용 명령을 파싱해야 한다. 이번 15 TU는 됐지만 다른 구성이 되는지는 미확인이다 [U].

## 8. 런타임에 필요한 변경 (고치지 않음, 목록만)

1. 시작 객체 `kz_rt_posix_auto.c`가 `kz_rt_config`의 `backend`, `compiler`, `target`, `manifest_sha256`을 채우지 않는다. dump의 RUN이 `backend="" … manifest_sha256=none`이다. 연결부가 정의하는 기호(weak)나 환경 이름으로 받을 길이 필요하다. 지금은 binding 목록과 빌드 기록으로 대신 식별한다.
2. `kz_rt_module(id, sha)`에 넘길 manifest SHA를 변환기가 만들지 않는다(도구에 해시가 없다). 1과 같은 경로로 빌드가 넘겨야 한다.
3. (참고) 탭 형태는 한 호출의 여러 인자를 레코드 하나로 묶지 못한다. 런타임에서 묶으려면 같은 actor의 "부분 레코드" 결합 규칙이 필요하다. 호스트 변환에서 site·bind·actor 순서로 묶을 수도 있다 [I].

## 9. 열린 항목과 결정

- **새 PI 결정: 없음.** 다음 둘은 파생 실행을 증거로 쓰게 될 때 정해야 한다. 레퍼런스가 계측 빌드를 정의하지 않으므로 레퍼런스가 정해 주지 않는다.
  1. R2 파생 빌드의 단위 시험 범위: 끄는 방식(이번 선택, 비행 목적 파일이 참조와 같음을 보였다)과 단위 시험 실행 파일에도 런타임을 링크하는 방식(빌드 시스템 편집이 늘어남) 가운데 무엇을 쓸지.
  2. 새 exe 디렉터리의 영속 상태 초기화: 파일 없이 시작(이번)과 참조 실행 직전 상태(0으로 찬 EEPROM, 빈 키 파일)를 재현하는 방식 가운데 무엇을 쓸지.
- dump → `.kzt` 변환과 그 위의 `kz replay` 재판정(B1 열린 항목). 이 문서의 대조는 값·횟수·정체성 수준이다.
- G-I0(같은 빌드, `KZ_RT_MODE=off`) 실행과 hook 비용 비교, 제어 gate(`kz_rt_ctl`) binding. 이번 연결부는 관측만 넣었다.
- F0 사이트 중 이번 목록 밖(C1 시작 상태·삭제·종료 범위, R2 등록·writer)의 binding.
- 논리 사이트 파일을 세 후보가 공유하도록 `profiles/` 아래로 옮기기.

## 10. 재현

```sh
# 도구와 시험 (LLVM 빌드)
cmake --build /home/user/work/kairos-build/k1r3-llvm --target kz-instr-clang && ctest --test-dir /home/user/work/kairos-build/k1r3-llvm -L instr
# 런타임 (대상용, 단독)
cmake -S kairos/runtime -B /home/user/work/instr/clang/rt -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=/usr/bin/cc && cmake --build /home/user/work/instr/clang/rt
# C1: 사본(tar --exclude=.git) → 변환 → 빌드 → 비교 → 기계어 조사 → 기동
kz-instr-clang --sites instrument/clang/sites/c1_post_1df262a.kzs --db /home/user/work/procrace/c1_build/post/1df262a/build/native/default/compile_commands.json \
  --map /home/user/work/procrace/c1_build/bundle_1df262a=/home/user/work/instr/clang/c1/bundle_1df262a --map /home/user/work/procrace/c1_build/post/src=/home/user/work/instr/clang/c1/post/src \
  --module 2=/default/apps/cf --module 3=/default/apps/bp --module '1=*' --binds-out …/c1/xform/binds.kzs --report-out …/c1/xform/report.txt
instrument/clang/scripts/build_c1.sh /home/user/work/instr/clang/c1 /home/user/work/instr/clang/rt
instrument/clang/scripts/compare_build.sh <ref build> <derived build> "-I<kairos>/runtime/include" <ref prefix>=<copy prefix>...
instrument/clang/scripts/hook_census.sh <core-tx> <cf.so> <bp.so>
instrument/clang/scripts/run_c1_pilot.sh /home/user/work/instr/clang/c1/post/1df262a <run dir> 1
# R2: --db cpu1와 cpu2, --exclude ut-coverage --exclude unit-test-coverage, --map <cfs_ref/cFS>=<copy>/cFS --module '1=*'
instrument/clang/scripts/build_r2.sh /home/user/work/instr/clang/r2 /home/user/work/instr/clang/rt
instrument/clang/scripts/run_r2_pilot.sh /home/user/work/instr/clang/r2/cFS <run dir> 2
kz-rt-dump --manifest instrument/clang/sites/<case>.kzs --manifest instrument/clang/out/<case>/binds.kzs <dump>
```

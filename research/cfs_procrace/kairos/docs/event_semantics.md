# 사건·제어 공통 의미와 계측 런타임 ABI (2026-10-26)

- 범위: 두 실행 계열(F0, 계측 계열)이 함께 쓰는 **사건 의미, 논리 site manifest, `kzp-hold 1` 제어 의미, 실제 제어 기록, 백엔드 capability, 보류 가능한 문맥**과, 계측 계열의 공통 C 런타임(`kairos/runtime/`, hook ABI 1, dump 형식 1.0). 근거: 검토 문서(사용자 업로드 `Kairos_OS_Independent_Instrumentation_Review_20261026.md`, 이하 IR) §8, §9, §11, §13.2–13.3, 결정 기록 `Architecture/spec/PI_DECISIONS_RESOLVED_20261026.md`(이하 PD26) §2.5, §4(구속), SPEC rev 1.4 §2.3 "neutral event", §4.13 "Common gate and release semantics"·"Cooperative gate", §5.12.
- 표기: **[F]** 코드·문서에서 확인한 사실, **[M]** 이번에 빌드·실행해 확인, **[D]** 설계 결정, **[I]** 계획·추정, **[U]** 미정. 코드 인용은 커밋 `88bdea9` 기준이다. §3의 binding id 규칙과 §8.2–§8.9는 런타임 0.2.0(dump 형식 1.1, 2026-10-26 단계 D 런타임 개정: 세 후보 문서 §8 "B1 런타임에 필요한 변경"과 `plan/instrumentation_selection_20261026.md` §4 항목 5·6) 기준이다.
- 지위: 설계와 단위 시험뿐이다. cFS를 계측해 돌리지 않았고 연구 조건을 정하지 않았다. 시험의 값(버퍼 크기, 시간 제한, 이름)은 단위 시험 값이며 실험 조건이 아니다. 런타임의 크기 상수는 도구 용량이고, 실행마다 dump의 RUN 레코드에 기록되며 넘친 것은 손실로 센다(§2.11).
- 이 문서는 schema 개정 번호·필드 위치를 `.kzt`에 배정하지 않는다(SPEC rev 1.4 "Open after revision 1.4"). 런타임 dump 형식은 `.kzt`가 아니라 호스트에서 `.kzt`로 바꿀 **백엔드 원시 근거**다.

## 1. 경계: 공통 의미 / 백엔드 binding / 원시 근거 [D]

| 층 | 무엇 | F0 | 계측 계열 |
|---|---|---|---|
| 공통 의미 | 이 문서 §2–§7. 판정·재생·예측이 읽는 것 | 기존 `.kzt` 해석(정체성 네 범주, 요약 뷰 등) | 같은 의미로 변환 |
| 백엔드 binding | 논리 site를 실제 위치에 묶는 것 | 계획 사이트 → ELF 파일 오프셋·레지스터·fetch(`SITES`, `SITE_SLOTS`, `SLOT_FETCHES`) | manifest의 `bind` 줄 → 컴파일러가 넣은 hook 호출 |
| 원시 근거 | 감사용으로 보존, 공통 해석은 의존하지 않음 | perf 레코드, `tid`/`tseq`, `hold.kzr` | `kzrt-dump` 스트림(EVENT/CTL 레코드, `pc`, 버퍼 바이트) |

관측 API와 제어 API는 분리한다. "그 site에서 관측 가능"은 "그 site에서 안전하게 보류 가능"을 뜻하지 않는다(IR §8.1).

## 2. 공통 사건 정보 (IR §9.2, SPEC rev 1.4 §2.3) [D]

각 항목: 의미(규범) → F0의 현재 근거 [F] → 계측 런타임의 근거(kz_rt 0.1) → 값이 없을 때 읽는 법.

| # | 정보 | 의미 | F0 (`kz_raw.h` 등) | 계측 런타임 | 없을 때 |
|---|---|---|---|---|---|
| 2.1 | 실행·cFS 인스턴스 | 같은 이름을 다른 실행·tx/rx 사이에 섞지 않음 | 실행 컨테이너 1개 = 인스턴스 1개, `RUNMETA`·실행 스크립트 태그 | RUN `run_id`, `instance_id`(하네스가 줌; 0 = 주지 않음) | 다른 실행과 합치지 않음 |
| 2.2 | 실행 actor + 세대 | native TID·OSAL id 재사용 구별 | `tid` + `THREADS.generation` | 런타임 actor id(실행 안에서 재사용 안 함), native key(Linux TID)+세대, 부트스트랩의 임시 id(§2.12) | 미확정 = `?`(actor 0) 또는 임시 `P<n>`. 결코 "앱 없음" 아님 |
| 2.3 | 생성자 / 소유 앱 / 서비스 대상 앱 | 실행자와 인자로 넘긴 AppID를 섞지 않음 | `IDENTITIES` 네 범주, `ACTOR_OWNERS`(level, evidence) | 런타임은 actor만 준다. 생성 요청·배정 id·알린 id·전달 AppID는 manifest `identity`(§3, 판 2)가 슬롯을 지정한 **관측 값**이고, 호스트가 F0와 같은 소유 앱 규칙으로 범주를 만든다(id로만 묶음, `schema.md` §12.3) | `undetermined`. 런타임은 `CFE_ES_GetAppID`·`OS_TaskGetId`를 hook 안에서 부르지 않는다(PD26 §4) |
| 2.4 | 논리 site / binding | 같은 의미 지점을 백엔드별 위치로 연결 | 계획 사이트 label = 논리 site, 파일 오프셋 = binding | `site`(논리) + `bind`(binding) 상수 둘 다 hook 인자. `pc`(hook의 반환 주소)는 인라인·복제를 가르는 원시 근거 | 동등성은 `equiv … confirmed`가 있을 때만(§3) |
| 2.5 | 연산·phase·호출 인스턴스 | 호출 전·효과·반환·종료 구별 | 진입/복귀 사이트, `a[3]` 반환 주소로 LIFO 짝짓기 | manifest `op=`, `phase=before|effect|after|exit|point`; 호출 인스턴스 = `link`(같은 actor의 before 레코드 aseq) | 짝 없는 after는 짝 없음으로 셈 |
| 2.6 | 자원 종류·키·생명주기 | 주소·이름·핸들 재사용과 namespace 구별 | Φ 프로필의 kind·bind·via, 관측 슬롯 역할 | manifest `value role=`(키·이름·id) + Φ 프로필. 런타임은 값만 기록 | 생명주기 미관측이면 `lifetime-known` 부재 |
| 2.7 | 인자·실제 사용 값·반환·유효성 | 미관측과 값 0 구별 | `TRUNC`, 슬롯 값; 문자열은 kernel fetch(`EVENT_STRINGS`) | 64비트 값 ≤8개 + `valid` 비트(비트가 꺼지면 미관측). 원 명령이 쓴 operand를 binding이 넘김(공유 메모리 재읽기 없음). 문자열은 `string=validated` 슬롯에서만 상한 복사, 아니면 포인터만 | 미관측 ≠ 0 |
| 2.8 | actor 내부 순서 | 관측 가능한 프로그램 순서 | `tseq`: **해석기**가 붙임 → 빈틈은 손실 증거 아님 | `aseq`: **target이 사건 시점에** 붙임(넘쳐 버린 레코드도 번호를 씀) → 빈틈 = 그 actor가 시도했으나 dump에 없는 레코드. `kz-rt-dump`가 빈틈 수와 센 overflow를 대조 [M] | — |
| 2.9 | 시간·시계 조건 | 성능·관측 기록용. actor 사이 HB를 만들지 않음 | `t_ns`(CLOCK_MONOTONIC), A-CLK-PO는 tracefs에만 | `t` + RUN `clock_id`(POSIX: CLOCK_MONOTONIC = F0와 같은 시계; OSAL 골격: `OS_GetLocalTime`, 설정 가능한 벽시계라 actor 안의 순서도 aseq로만) | 시계 없음(부트스트랩) = `t` 무효 플래그 |
| 2.10 | 순서 근거의 출처 | 아래 네 종류를 구별 | 요약 플래그 `basis-*`, P11 | 아래 표 | 근거 없으면 순서 없음 |
| 2.11 | 손실·미계측·관측 구간 | 기록 부재를 사건 부재로 읽지 않음 | `LOSS`(커널 보고만: `perf-record-lost` 상한, throttle), `OBS_ADEQUACY` | LOSS 레코드 13종(§2.11), 관측 구간 = RUN `t_init/t_attach/t_stop/t_dump` + 부트스트랩 구간, manifest의 `removed|unsupported` binding과 미등록 module | 손실 0 = "센 손실 없음", 증명 아님 |
| 2.12 | 초기화 전 사건 | 대상의 등록 절차 이전부터 관측(IR §11.1) | uprobe를 실행 전 inode로 등록 → 시작부터 | 정적 부트스트랩 버퍼 + 스레드별 임시 id(§8.3) | — |
| 2.13 | 버전 | 같은 기록을 같은 의미로 재해석 | `RUNMETA`, tool hash, profile sha256 | RUN: 런타임 판(`kzrt 0.2.0`), ABI 1, 형식 1.1, adapter 이름·caps, `backend`/`compiler`/`target` 태그, manifest sha256(실행 파일에 build identity 객체가 있으면 그 값, §8.5). RUNX: 모드 원천·창의 모드·용량(§8.2, §8.3). 프로필은 호스트 변환 때 묶음(F0의 `profile_sha256`과 같음) | 태그가 비면 "주지 않음" |

**순서 근거의 네 종류 (2.10)** [D]. 판정·재생이 쓰는 순서 간선은 출처를 하나씩 단다.

| 종류 | 무엇 | 기존 표시와의 대응 |
|---|---|---|
| 원 프로그램 동기화 | 같은 actor의 `aseq` 순서, Φ `hb` 규칙에 맞는 관측된 동기화 쌍(잠금/해제, give/take, put/get, 생성→시작) | `basis-state-order` |
| 실제 값 연결 | 읽은 값과 쓴 값이 같은 id·키로 이어짐(reads-from) | `basis-direct-capture`, `basis-return-derived-order`(반환값에서 유도) |
| 조건부 추론 | 신뢰 가정(A-OWNER, 역할 선언, `via` 키 분리)이나 반환 코드 좁힘(P1)에 기댐 | `assumes-*`, `outcome-narrowed` |
| 제어 개입 | 보류·해제의 실제 기록(§5)이 만든 순서. **그 제어 실행에서만** 근거(P11) | P11 |

근거가 **아닌** 것: 서로 다른 actor의 시각, 런타임 내부 순서(버퍼 순서, 공유 버퍼 예약 순서, actor id 배정 순서, 제어 잠금, `ctl_seq`). `ctl_seq`는 제어 결정의 순서이며 "제어 개입" 근거의 재료로만 쓴다(IR §11.4, PD26 §4).

## 3. 논리 site manifest `kzs-sites 1` / `kzs-sites 2` (IR §9.3, SPEC rev 1.4 §5.12) [D][M]

텍스트, 줄 단위(`#` 주석, 공백 구분, 따옴표 안은 공백 허용). JSON이 아니며 도구 핵심의 C/C++ 규칙 안에 있다. 파서: `runtime/tools/manifest/kz_rt_manifest.{hpp,cpp}`(호스트 C++; 세 후보가 그대로 쓸 수 있다). 논리 site(프로필에서 연구자가 씀)와 백엔드의 binding 목록(컴파일러 연결부가 씀)은 **같은 형식의 다른 파일**로 두고 한 Manifest로 합쳐 읽는다.

```
kzs-sites 1
source <name> rev=<revision> [ref=<text>]
module <id> name=<text> source=<source-name>
site <id> label=<label> op=<operation> phase=before|effect|after|exit|point tu=<path> func=<function>
     [callee=<function>] [line=<n>] [place=at-call|after-call|line-start|store] [ref=<file:line at the revision>]   # place= 판 2만
value <site-id> <slot 0..7> role=<role> from=arg<k>|ret|out<k>|operand<k>|param<k> type=u8..u64|s8..s64|ptr     # param<k> 판 2만
     [string=pointer-only|validated] [bound=<1..64>] [ref=<text>]
control <site-id> ctx=task|lock-held|isr|osal-internal|terminating|unsupported [ref=<text>] [note=<text>]
bind <id> site=<site-id> backend=<name> module=<module-id> at=<location text>
     status=bound|inlined|duplicated|removed|unsupported [vals=<mask>] [reason=<text>]
equiv <site-id> backends=<a>,<b> status=unconfirmed|confirmed [ref=<text>]
identity <site-id> decoder=<reader> [id=<slot>] [owner=<slot>] [status=<slot> ok=<integer>] [ref=<text>]   # 판 2만
```

**판 2 `kzs-sites 2` (2026-10-26, 정체성 읽기)** [D][M]. 판 2 = 판 1 + `identity` 문. 파서는 두 판을 다 읽고(파일마다 머리줄), 판 1 파일에 `identity`가 있으면 거부한다. 여러 파일을 한 Manifest로 읽으므로 정체성 읽기는 논리 site 파일과 다른 파일(판 2)에 둘 수 있다(예: `instrument/gcc/sites/{c1_post_1df262a,r2_v701}.identity.kzs`가 각 manifest의 site 1–8에 붙음). `identity`는 F0 계획의 `decoder=`(`kz_raw.h` KZ_DECODER)와 **같은 이름·같은 의미**이고, 값 슬롯은 이름·역할에서 찾지 않고 **번호로 명시**한다. 슬롯은 그 site에 먼저 선언된 `value`여야 하고 4바이트 이하 정수(u8–u32, s8–s32)다. site당 하나.

| reader | site phase | 슬롯 | 의미 (F0 decoder와 같음) |
|---|---|---|---|
| `spawn-create` | before | — | task를 만드는 호출의 시작(생성 호출 구간을 연다) |
| `spawn-return` | after | 선택: `id=` + `status=` + `ok=` | 그 호출의 반환. `id`는 호출 뒤 읽은 배정 id(`from=out<k>` 또는 `ret`). 실패한 호출은 out 매개변수를 그대로 두므로 `status` 슬롯 값이 `ok`(성공 값, 10진 또는 0x, 슬롯 폭으로 비교)일 때만 쓴다 |
| `spawn-id` | before / point / effect | `id=` 필수 | 같은 actor에 열린 spawn-create 호출 안에서, 호출이 새 task에 넘기는 id |
| `thread-entry` | before | `id=` 필수 | 자식 쪽: task가 시작에서 알린 id |
| `owner-request` | before | `owner=` 필수 | 인자로 지정한 앱을 위한 task 생성 요청: 그 앱의 식별 값(예: `ParentAppId`) |
| `child-request` | before | — | 요청한 task 자신의 앱을 위한 생성 요청 |
| `request-return` | after | — | 두 요청의 반환 |

**판 2의 점 위치 `place=`와 매개변수 값 `from=param<k>` (2026-10-26, D2)** [D][M]. F0 계획의 점(`at-call`·`after-call`·`line-start`, 꼬리 점프로 닿는 점, 줄의 store)을 계측 계열에서 **점으로** 적기 위한 것이다. 판 1의 `phase=before|after` + `callee`는 호출 쪽에서 본 그 함수의 호출·반환이라(변환이 `call-site` 호출/반환으로 읽음), 같은 함수의 진입 site와 함께 두면 재생이 한 호출을 두 번 본다. `place=`가 있는 site는 `phase=point|effect|exit`이고 언제나 점이다(`func` 안의 문장에 묶임).

| place | 필요 | 묶는 곳 | 값 |
|---|---|---|---|
| `at-call` | `callee=` | `callee` 호출 바로 앞(`line=`이면 그 줄의 호출만) | `arg<k>` = 그 호출의 인자 operand |
| `after-call` | `callee=` | 그 호출 바로 뒤 | `ret`(필요할 때만 결과에 SSA 이름을 붙임), `out<k>`, `arg<k>` |
| `line-start` | `line=`, callee 없음 | 기본 블록 안에서 그 줄 문장이 시작될 때마다 그 앞 | `param<k>` |
| `store` | `line=`, callee 없음 | 그 줄의 store 문장(필드·전역·포인터 대상 무엇이든) 바로 뒤 | `operand0` = 저장한 값, `operand1` = 저장한 주소(주소 계산만, 다시 읽지 않음) |

`from=param<k>`는 `func`가 받은 매개변수 k의 값(SSA 기본 정의)이다. 메모리에서 다시 읽지 않는다. 주소가 잡힌 매개변수처럼 레지스터 값이 아니면 그 binding은 `unsupported`(이유)다. `place=` 없는 `phase=point|exit`, callee가 있는 `phase=effect`는 묶을 곳이 없어 선언된 빈틈(이유 "needs place=")이 된다. 판 1 파일은 두 키를 쓸 수 없다(파서가 거부). 변환(`kz convert --kzrt`)은 바뀌지 않았다: 점은 그 label이 프로필의 점이면 그 점이다. 구현 `instrument/gcc/kzgcc.cpp` L444–L700, 시험 `kzgcc_shapes_{O0,O2,O3}`, 파서 시험 `rt_manifest`(배치 site 5개 수용, 7개 거부) [M]. 적용한 manifest: `profiles/c1-bp-b051eb0/instr/sites-cf{833fdbb,281a941}.kzs`, `profiles/cfs-v7.0.1/instr/sites.kzs`(`docs/instr_candidate_gcc.md` §11).

닫는 레코드(spawn-return, request-return)는 사건의 `link`(같은 호출 인스턴스를 연 레코드의 aseq)로 여는 레코드와 짝짓는다. 그래서 binding은 정체성 여는 site의 레코드를 link로 줘야 한다(kzgcc는 같은 호출·함수의 첫 before site를 link로 준다: 여는 site를 그 호출의 before 중 처음에 둔다). 결합 규칙은 §2.3과 `include/kz/trace/identity_rule.hpp`(F0와 같은 소유 앱 규칙 `owner_from_request`)이며 `schema.md` §12.3에 적었다.

- id는 십진 1..2147483647, 종류별 유일. `label`은 유일하며 kzp-observe 사이트 label·`.kzh`의 `site=`와 같은 이름을 쓴다. **site id와 bind id가 hook 호출의 컴파일 시점 상수**다. bind id 배정은 연결부가 하며 실행 전체에서 유일해야 한다. 규칙은 다음 항목이다 [D].
- **binding id 규칙** [D]: `bind = (module << 16) | (site << 4) | n`. module은 1..32767(결과가 2147483647 이하), site는 1..4095, n은 한 번역 단위 안에서 그 site의 binding 순번 0..14이고, n = 15는 그 단위에 binding이 없는 site의 선언된 빈틈 줄(`status=unsupported`)이다. 한 단위에서 한 site의 binding이 15개를 넘으면 넘는 것은 `unsupported`(이유 기록)로 둔다. 한 module 안에서 site는 한 번역 단위(`site tu=`)에만 묶이므로 id가 겹치지 않고, 겹치면 binding 합치기(`instrument/gcc/scripts/merge_binds.sh`)가 거부한다. GCC 연결부(`instrument/gcc/kzgcc.cpp` L294, L412–L428, L841)와 LLVM 후보(`instrument/llvm/kzllvm.cpp` L210, L467–L480, L837)가 이 규칙을 쓴다 [F]. kzgcc 0.2는 site ≤ 4095·module ≤ 32767을 적재 때 검사하고 넘으면 거부한다(L917–L932). LLVM 후보는 검사하지 않는다 [F: grep]. 지금 manifest의 범위는 module 1–4, site ≤ 46이다(`instrument/gcc/sites/*.kzs`, `profiles/*/instr/*.kzs`) [F]. Clang 변환 후보는 `(module << 16) | 순번`을 썼다(`instrument/clang/kz_instr_clang.cpp` L314) [F]. 단계 C 비교 증거로 그대로 둔다. 런타임과 호스트 도구는 bind id를 해석하지 않는다(상수로 기록만). binding의 module·site는 manifest의 `bind` 줄에서만 읽고 id의 비트에서 거꾸로 읽지 않는다.
- IR §9.3의 항목 대응: source revision·번역 단위·함수/호출 문맥 = `source`·`site tu/func/callee`; 전·후·효과 = `phase`; 인자·값·자원 키의 근거 = `value … ref=`; 백엔드가 고른 위치 = `bind at=`; 최적화로 사라짐·복제·인라인 = `bind status=removed|duplicated|inlined`(같은 bind id가 여러 위치에 있으면 `pc`가 가른다); 지원 못한 지점과 이유 = `status=unsupported reason=`.
- `string=validated`는 `type=ptr`, `bound`, `ref`(원 코드가 그 지점 전에 포인터를 검증한 곳, 또는 그 메모리가 크기를 아는 배열이라는 근거)를 요구한다. 없으면 포인터 값만 기록한다(검증 전 `strlen` 금지, PD26 §4). `kz-rt-dump --manifest`는 validated가 아닌 슬롯에서 복사된 문자열, binding이 선언하지 않은 `valid` 비트, `removed|unsupported` binding의 사건, 등록되지 않은 module을 보고한다 [M: `rt_dump_tool`, `rt_dump_tool_strict`].
- 동등성: 두 백엔드의 binding이 같은 효과를 관측함이 근거(`ref`)로 확인되기 전에는 `equiv … unconfirmed`(= "동일 의미 확인 전")다. 이름이 같다고 동등하지 않다.
- 예시 파일: `runtime/tests/data/sites_example.kzs`(단위 시험용 일반 이름). C1·R2의 실제 manifest는 단계 C(후보 비교)에서 실제 소스 위치·근거와 함께 만든다 [I]. 판 2의 받는 것·거부하는 것은 `runtime/tests/test_rt_manifest.cpp`(정체성 읽기 8개 수용, 18개 거부) [M].

## 4. `kzp-hold 1` 제어 의미 (현행 구현의 명세화; IR §9.4, SPEC rev 1.4 §4.13) [F][D]

F0 `kz hold`가 2026-10-25(confirm2)부터 구현한 의미를 그대로 규범으로 적는다. 모든 백엔드는 이것을 구현하거나, 바꾸면 형식 판을 올리고 변환을 적는다. 근거 줄은 `probe/hold/hold_plan.hpp` L12–L41, `probe/hold/hold_plan.cpp` L87–L92·L139, `probe/hold/supervisor.cpp`(아래).

| # | 규칙 | F0 구현 [F] | 런타임 구현 [M] |
|---|---|---|---|
| H1 | 자격 hit = 그 점의 위치 도달 + 조건 충족, **어느 스레드든**, 실행 시작부터 점마다 하나의 계수 | `count_hit` L300–L308 (조건은 hit 시점에 평가 L419–L439) | `count_hit`, `qualifies`(조건은 그 hit의 관측 레코드 값으로) |
| H2 | `occurrence first`(=1) / `<n>`(1..1000000): n번째 자격 hit 하나에만 작용. `each`: 매 자격 hit | 같은 곳; 파서 L87–L92 | 같음(`occurrence` 0 = each) |
| H3 | 한 위치의 여러 점: 게이트 먼저(계획 순서; 처음 작용한 게이트가 잡고, 같은 hit에 작용한 다른 게이트는 보류 없이 소비), 다음 해제 | 게이트 L442–L466, 해제 L468–L474 | 같음(`gate-consumed` 기록) |
| H4 | 잡힌 hit의 해제는 그 스레드가 **재개될 때** 계수·작용(deferred). 재개는 해제가 작용한 즉시, 다른 사건보다 먼저 처리 | `deferred` L472, `drain_resumes` L348–L362 | 재개시킨 스레드가 제어 잠금 안에서 곧바로 처리(`drain`); 기록은 그 스레드 버퍼에 `subject` = 잡혔던 actor, 사유 `deferred` |
| H5 | 해제는 게이트가 잡은 스레드가 없을 때도 작용해 소비된다. `unless-released`(게이트만, `each`와 함께 못 씀): 그 경우 게이트가 충족되어 이후 잡지 않음 | `fire` L313–L344; 파서 L139 | 같음(`fire` 사유 `satisfied` / `consumed`); `kz_rt_plan`이 `each`와의 조합을 거부 |
| H6 | 재개된 스레드는 계속 무장: 뒤 hit는 다른 스레드와 똑같이 작용, 이미 작용한 게이트(first·n)는 다시 잡지 않음 | RF로 게이트 명령만 넘김 L292–L298, L360 | 무장 개념 없음(hook은 늘 평가) — 같은 의미 |
| H7 | 조건은 관측 사이트와 슬롯 역할·호출자로만 적는다(레지스터·오프셋·슬롯 번호 없음) | `name <role> "<text>"`(멈춘 스레드 메모리에서 문자열 읽음), `caller-in <binary>`(스택 꼭대기 반환 주소의 모듈) | 호스트가 변환: `name` → 그 역할의 슬롯이 `string=validated`인 binding에서 복사된 문자열과 비교(`str_slot`, 최대 64 B); `caller-in` → 그 모듈에 있는 호출 측 binding 집합(`bind[]`). 값 비교 `val_slot` 추가 |

**kzp-hold 1이 정하지 않은 경우와 두 구현의 차이** [F][D]:
- `each` 게이트가 이미 한 스레드를 잡고 있을 때 다른 스레드의 hit가 작용하면: F0는 잡은 스레드 기록을 덮어써(`held_for_gate[cp.label] = w`, L457) 앞 스레드가 이후 해제로 풀리지 않는다. 런타임은 `gate-busy`를 기록하고 새 스레드를 잡지 않는다. 저장소의 `.kzh`에는 `each` 게이트가 없다 [F: grep]. 다음 형식 판에서 한쪽으로 정한다 [U].
- 보류할 수 없는 문맥(§7)에서 게이트가 작용하면 런타임은 `gate-refused`(사유)를 기록하고 그 작용을 소비한다(first·n은 다시 작용하지 않음). F0는 문맥을 검사하지 않는다(배치 규칙은 계획 작성자가 지킴, SPEC §4.13 Gate placement).
- 시간 제한: F0 holder에는 없다(외부 watchdog). 런타임 점은 `timeout_ns`(0 = 없음)를 가지며, 지나면 `timeout`을 기록하고 그 hit의 deferred 해제를 작용시킨 뒤 진행한다. 값은 레퍼런스의 timeout에서 정한다(SPEC §4.13 Watchdog); 이 문서는 값을 정하지 않는다.
- 계수 순서: 서로 다른 스레드의 hit를 세는 순서는 F0에서는 supervisor가 정지를 처리한 순서, 런타임에서는 제어 잠금을 얻은 순서다. 둘 다 제어기의 순서이며 프로그램 순서가 아니다.

## 5. 실제 제어 기록 (IR §9.5, SPEC rev 1.4 §4.13 "Control realization records") [D][M]

| IR §9.5 항목 | F0 `hold.kzr` [F] | 런타임 CTL 레코드(`what`, `reason`) |
|---|---|---|
| gate 도달 | 보류 줄(`gate`, `tid`, `name`, `hit`) — 잡은 경우만 | `gate-hold` / `gate-refused` / `gate-consumed` / `gate-busy`: actor, site, bind, `ev`(그 hit의 관측 레코드 → 자원 키 값), `hit` |
| 보류 확정 | 보류 줄의 `held_ns`(스레드는 ptrace 정지) | `gate-hold`는 대기 직전에 쓴다. 확정 = 그 actor의 다음 레코드가 `resume`/`timeout`이고 사이에 그 actor 레코드가 없음, 또는 dump 시 POINT `held_actor`·ACTOR `held` |
| release 도달 | `fire` 줄(해제·스레드·hit·재개한 스레드) | `fire` 사유 `resumed`, `other` = 재개된 actor, `subject` = hit의 actor |
| release 선도달 | `resumed=none satisfied=yes|consumed=yes` | `fire` 사유 `satisfied` / `consumed` |
| 재개 | 보류 줄 `release_ns`·`release` | `resume`(잡혔던 actor가 씀, `other` = 해제한 actor) |
| 보류 실패·미지원 | `debugregs_ok=false`(실행 전체) | `gate-refused` 사유 `context` / `no-wait` / `pre-attach` / `no-slot`; LOSS `ctl-skipped`, `ctl-nocond` |
| 종료까지 미해제 | 보류 줄 `released=false` | POINT `held_actor ≠ 0`(덤프 시 미해제), ACTOR 플래그 `held` → P13: 후보 전체 확인으로 세지 않고 앞부분과 미완료 원인만 기록 |

모든 CTL 레코드는 `ctl_seq`(제어 결정 순서)를 가진다. 논리 게이트의 존재, 실제 보류, 스케줄러가 스스로 낸 순서는 합치지 않는다: 게이트가 작용하지 않은 실행의 순서는 `natural`이고, 제어가 만든 순서만 "제어 개입" 근거다(§2.10).

## 6. 백엔드 capability descriptor (IR §9.4) [F][M]

| 항목 | F0 (`kz observe` perf-sample + `kz hold`) | 계측 런타임 kz_rt 0.1 (`kz_rt_caps_get`, RUN 레코드) |
|---|---|---|
| 제어 주소 수 | 서로 다른 주소 ≤ 4 (DR0–DR3, 감시점과 공유; `supervisor.cpp` L151–L160) | 주소 제한 없음; 계획 점 ≤ `KZ_RT_MAX_POINTS`(32) |
| 게이트 대상 | 주 바이너리를 매핑한 프로세스의 모든 스레드, clone 자동 연결 | attach 뒤 run actor 슬롯이 있는 스레드(≤ `KZ_RT_MAX_ACTORS` 64 = C1 빌드의 `OS_MAX_TASKS 64`, `plan/C1_build_manifest.md` L148) |
| actor당 게이트 | 한 번에 하나(정지) | 한 번에 하나(`gates_per_actor` 1); 게이트당 잡는 스레드 하나 |
| 보류 가능한 문맥 | 사용자 공간 어디든(ptrace 정지) — 안전성은 배치 규칙으로 | `holdable_ctx_mask`: task; lock-held·osal-internal은 점 플래그가 허용할 때만(osal-internal은 잠금이 private일 때만); ISR·terminating·unsupported 불가 |
| 대기 primitive | ptrace 정지 | adapter `hold_wait`: POSIX = pthread 조건 변수(실제 대기, 시간 제한 지원), OSAL 골격 = 이진 세마포어(컴파일만), 없음 = 기록만 하는 no-op(`no-wait`) |
| 값 | 사이트당 슬롯 4(`a[0..3]`, 레지스터·스택·fetch), 문자열은 kernel fetch | 사건당 64비트 값 ≤ 8, 검증된 문자열 ≤ 64 B, `pc` |
| 버퍼·손실 | perf ring, 커널 보고 손실만 | run actor가 함께 쓰는 정적 pool(기본 4 MiB = 64 × 64 KiB, 4 KiB chunk; 빌드 파라미터 `KZ_RT_POOL_BYTES`·`KZ_RT_CHUNK_BYTES`), actor당 할당량(실행 파라미터 `actor_buf_bytes`, 0 = 없음), 부트스트랩 64 KiB, 손실 13종. 넘침은 기다리지 않고 버리며 버린 바이트와 사유를 센다(§8.3, §8.9) |
| 초기화 전 | 실행 전 inode 등록 | 정적 부트스트랩 버퍼 + 임시 id |
| 계획 기능 | kzp-hold 1 전체 | kzp-hold 1 전체 + `timeout_ns`, 값 조건; `name`은 validated 슬롯에서만, `caller-in`은 호출 측 binding으로만 |

## 7. 보류 가능한 문맥 class (IR §11.5, SPEC rev 1.4 §4.13) [D][M]

manifest `control <site> ctx=`로 binding마다 정하고, 연결부가 그 값을 `kz_rt_ctl`의 `ctx` 상수로 넘긴다. 런타임은 hit마다 검사한다.

| class (`KZ_RT_CTX_*`) | 원칙 | 런타임 동작 | SPEC §4.13 Gate placement와의 관계 |
|---|---|---|---|
| `task` | release 경로가 진행 가능한 일반 task: 제어 후보 | 잡음 | 규칙 1(프로그램 잠금 집합이 빔)을 계획이 만족한 지점 |
| `lock-held` | release에 필요한 잠금 보유: 정체 위험 명시, 다른 지점 우선 | 점에 `ALLOW_LOCK_HELD`가 있을 때만 잡음; 아니면 `gate-refused(context)` | 규칙 1 위반 지점 |
| `isr` | interrupt/exception/signal: task 대기 적용 안 함 | 평가 자체를 안 함(잠금도 안 잡음), LOSS `ctl-skipped` | 규칙 2(signal handler) |
| `osal-internal` | scheduler·OSAL 등록 내부: primitive의 재진입·초기화 조건 먼저 | 점에 `ALLOW_OSAL_INTERNAL`이 있고 제어 잠금이 private일 때만; adapter 잠금이 대상 객체(OSAL 골격)이면 평가하지 않고 `ctl-skipped` | 규칙 2·3(binsem 내부, `OS_TaskPrepare` 전, RESERVED 구간) |
| `terminating` | 종료·삭제 중 task: 미완료 계획과 수명 종료 구별 | 잡지 않음(`gate-refused(context)`); 해제는 작용 | — |
| `unsupported` | 미지원 지점: 조용히 건너뛰지 않음 | 잡지 않음, 기록, 계획 실현 범위가 낮아짐 | 안전한 지점이 없으면 "현재 기구로 확인 불가"(SPEC §4.13) |

추가 조건: attach 전(`pre-attach`), run actor 슬롯 없음(`no-slot`), adapter에 대기 primitive 없음(`no-wait`)이면 잡지 않고 기록한다. compiler hook은 실행이 그 위치에 올 때만 작동한다: 계측되지 않은 코드나 임의의 interrupt 시점은 제어하지 못한다(IR §11.5).

## 8. 런타임 (`kairos/runtime/`) [D][M]

### 8.1 hook ABI 1 (`include/kz_rt.h`)

```c
/* 관측 (차단·잠금·계측 코드 호출 없음; kz_rt_obs_str만 상한 안에서 대상 메모리를 읽음) */
uint32_t kz_rt_obs(uint32_t site, uint32_t bind, uint32_t link, uint32_t valid,
                   uint64_t v0, uint64_t v1, uint64_t v2, uint64_t v3);
uint32_t kz_rt_obsv(uint32_t site, uint32_t bind, uint32_t link, uint32_t valid, uint32_t n, const uint64_t *v);
uint32_t kz_rt_obs_str(uint32_t site, uint32_t bind, uint32_t link, uint32_t valid,
                       uint64_t v0, uint64_t v1, uint64_t v2, uint64_t v3, uint32_t str_slot, uint32_t str_bound);
/* 제어 (허용 문맥에서만 차단) */
int kz_rt_ctl(uint32_t site, uint32_t bind, uint32_t ev, uint32_t ctx);
/* 하네스 */
int  kz_rt_init(const kz_rt_config *cfg);          int  kz_rt_attach(const kz_rt_adapter *adapter);
int  kz_rt_plan(const kz_rt_point *points, uint32_t n);
void kz_rt_stop(void);                             int  kz_rt_dump(kz_rt_write_fn write, void *ctx);
void kz_rt_caps_get(kz_rt_caps *out);              int  kz_rt_ctl_state(uint32_t point, kz_rt_point_state *out);
void kz_rt_module(uint32_t module, const uint8_t *manifest_sha256);
void kz_rt_note_loss(uint32_t kind, uint32_t key, uint32_t count);
void kz_rt_actor_exit(void);                       kz_rt_cell *kz_rt_tls_cell(void);
```

- 반환값 `aseq`는 그 레코드의 actor 내부 순번이다. 연결부는 before 레코드의 aseq를 after의 `link`로, 제어점에서는 바로 앞 관측의 aseq를 `kz_rt_ctl`의 `ev`로 넘긴다(조건은 그 hit의 값으로 평가).
- 값 규칙(연결부 책임): 원 명령이 쓴 operand(SSA 값·호출 인자)를 manifest `type`대로 0/부호 확장해 넘긴다. 공유 변수 재읽기, `volatile`·MMIO 추가 읽기, 늦은 out-parameter 읽기, store의 atomic 변경, Instrumentor류 값 교체를 하지 않는다(PD26 §4). out-parameter는 `from=out<k>`로 표시한 after 지점에서 같은 스레드가 곧바로 읽은 값만이다.
- 연결 규칙: 런타임 TU는 계측하지 않는다. 프로세스당 한 번(실행 파일에) 링크하고, 적재 모듈(`.so`)은 동적 기호로 부른다(`KZ_RT_API` = default visibility). 관측 hook 안에서 `kz_rt_ctl`을 부르지 않는다.

### 8.2 모드 (같은 빌드의 G-I0 / G-I / G-I-H, IR §13.1)

`KZ_RT_MODE_OFF`(hook 즉시 반환, 아무것도 세지 않음), `RECORD`(관측만, `kz_rt_ctl` 즉시 반환), `CONTROL`(계획 평가; 계획이 없으면 모든 게이트가 비활성). `kz_rt_stop` 뒤 RECORD·CONTROL 창은 `STOPPED`(세기만, `after-stop`)가 되고, OFF는 OFF로 남는다(RUN `mode=off`). 창의 모드는 RUNX `window_mode`에 남는다 [D][M].

모드 단어의 원천은 셋이다. 이른 것부터 적는다. RUNX `mode_source`가 그 실행의 원천을 적는다 [D].

| 원천 (`mode_source`) | 언제 | 규칙 |
|---|---|---|
| `kz_rt_early_mode(mode, note)` (`early`) | 첫 hook보다 먼저. POSIX 시작 객체는 `.preinit_array`에서 부른다(§8.5) | 모드를 프로세스 전체에 고정한다. 뒤의 `kz_rt_init`이 다른 모드를 주면 거부하고(`KZ_RT_EINVAL`, 아무것도 설정하지 않음) RUNX `init-refused`를 남긴다. 그보다 먼저 hook이 돌았으면(부트스트랩 바이트·손실, 미확정·재진입 hook, run actor) `hooks-before-early`를 남긴다. 대상 메모리를 읽지 않고 런타임 자신의 메모리만 쓴다 |
| `kz_rt_init` (`init`) | early가 없을 때 | 그 전의 hook은 정적 기본값 RECORD로 부트스트랩 버퍼에 남는다(0.1.0 동작) |
| 정적 기본값 RECORD (`static-default`) | 설정 호출 전 | — |

같은 바이너리 전환 [M]: 시작 객체를 링크한 실행 파일 하나를 `KZ_RT_MODE=off|record|control`만 바꿔 돌린다(시험 `rt_mode_*`). 우선순위가 가장 높은 생성자(`constructor(101)`: 시작 객체의 생성자보다 먼저 도는 계측 코드의 대역)의 hook도 `off`에서는 기록되지 않는다(`buf owner=shared bytes=0`, 사건 0). `record`·`control`에서는 그 hook이 부트스트랩 버퍼에 `P1`로 남고, main의 hook은 같은 셀로 이어진 run actor 1에 남는다. 이름이 없으면 record로 하고 `mode-defaulted`를 남긴다. 틀린 이름(시험은 `of`)이면 정적 기본값 record로 두고 `mode-invalid`를 남기며, 이것은 `kz-rt-dump` 검사 실패다. G-I0를 의도한 실행이 기록 실행으로 잘못 붙는 것을 막는다.

남는 차이와 읽는 법 [F][I]:
- OFF에서도 hook 호출 자체(인자 계산, 호출, 반환)와 모듈 등록 생성자(`kz_rt_module`, MODULE 레코드)는 남는다. 이것이 G-I0가 재는 "남은 hook·분기 비용"이다(SPEC §8.6). 런타임 정적 메모리(§8.3)도 모든 G-I*에 같다.
- OFF의 dump는 사건이 없었다는 기록이 아니라 **기록하지 않았다**는 기록이다(RUN `mode=off`, RUNX `window_mode=off`). 변환기(`lib/trace/kzrt_convert.cpp`)는 RUN `mode`가 off이면 시작 근거(부트스트랩부터 전부)를 주지 않는다 [F]. RUNX를 읽어 창의 모드·원천을 중립 층에 옮기는 것은 변환기의 열린 항목이다(§10).
- `.preinit_array`는 ELF 실행 파일에만 있고, glibc가 (argc, argv, envp)로 부른다. 그 진입점이 돌지 않는 C 라이브러리(예: musl)에서는 생성자가 `kz_rt_early_mode`를 부르고, 그보다 먼저 돈 hook은 `hooks-before-early`로 드러난다. 생성자가 없는 RTOS에서는 BSP의 첫 코드가 `kz_rt_early_mode`를 부를 수 있다 [U: RTOS 실행 안 함].
- G-I-H의 계획을 대상에 넣는 길(직렬화된 계획을 `control` 모드의 시작 객체가 읽음)은 단계 F1이다. 지금 `control` 모드의 시작 객체는 계획 없이 돈다(게이트 비활성).

### 8.3 부트스트랩과 actor

- attach 전: 모든 사건은 정적 부트스트랩 버퍼(다중 작성자, 원자적 예약 + size 단어를 마지막에 release로 써서 확정)에 들어간다. actor는 스레드별 임시 id `P<n>`(컴파일러 TLS 셀; 없으면 actor 0 = 미확정, 공유 순번)이고 시계가 없다. 가득 차면 기다리지 않고 `preinit` 손실로 센다 [M: `rt_bootstrap`].
- attach 뒤: 스레드의 다음 사건에서 run actor 슬롯과 자기 버퍼를 받는다. 같은 셀에 임시 id가 있었으면 ACTOR `boot_link = P<n>`, 근거 `same-cell`이며, aseq는 이어진다(그 actor의 범위는 `first_aseq`부터). 새 스레드는 링크 없이 1부터 [M].
- **기록 공간(0.2.0)** [D][M]: run actor는 정적 pool 하나(`KZ_RT_POOL_BYTES`, 기본 `KZ_RT_MAX_ACTORS × 65536` = 4 MiB. 0.1.0의 actor별 64 KiB 버퍼 64개와 같은 크기)를 `KZ_RT_CHUNK_BYTES`(기본 4 KiB) chunk로 나눠 쓴다. chunk는 처음 받은 actor만 쓰고(작성자 하나), 레코드는 chunk를 넘지 않는다(chunk 끝의 남는 공간 < 가장 큰 레코드 184 B). chunk를 얻는 일은 pool 계수기의 원자적 덧셈 하나다(잠금·대기 없음). 이 덧셈의 순서도 런타임 내부 순서이며 HB가 아니다(§2.10). actor는 chunk마다 확정 길이와 다음 chunk 연결을 release로 공개한다. dump는 연결을 먼저 acquire로 읽고 길이를 읽는다. 다음 chunk가 있는 chunk는 완결이므로 dump는 actor마다 빈틈 없는 앞부분을 읽는다(`rt_pool_dumps`: 작성자 4 × 3,000 레코드 동안 dump를 반복, ThreadSanitizer 통과). 실행 파라미터 `actor_buf_bytes`(시작 객체: `KZ_RT_ACTOR_QUOTA`)는 actor당 확정 바이트 할당량이다(0 = 없음: pool만 제한). pool이 비거나 할당량에 닿으면 그 레코드를 기다리지 않고 버리고 `overflow`로 센다. 사유(`overflow_quota` / `overflow_pool`)와 버린 바이트(`dropped_bytes`)는 ACTOR에, 합계는 RUNX(`pool_dropped`, `chunks_used`)에 남는다. 그래서 actor의 수요(`buf_bytes + dropped_bytes`)가 기록에서 바로 나온다(§8.9). 한 actor가 pool을 다 쓰면 다른 actor의 기록이 버려질 수 있다. 이것도 actor별 손실로 센다(`rt_pool_empty`).
- 런타임은 정체성을 대상의 표에서 조회하지 않는다. OSAL task id·앱 id는 manifest 역할이 붙은 관측 값으로만 들어오고, 호스트 결합에서 근거와 함께 연결된다. 슬롯이 모자라면 `actor-table`을 세고 임시 id로 공유 버퍼에 남긴다.
- 재진입: 스레드가 hook 안에 있을 때 다시 들어오면(signal handler, adapter가 계측 코드를 부른 경우) 기록하지 않고 `reentry`로 센다. per-actor 버퍼는 스레드 셀에 묶이므로 CPU 이동과 무관하고, dump는 확정된 앞부분만 읽으며 쓰는 중이던 레코드는 `partial`로 센다 [M: `rt_loss`]. RTOS의 ISR은 adapter가 ISR용 셀을 주지 않으면 `self_cell`이 NULL을 돌려 미확정 actor로 남아야 한다(OSAL 골격은 아직 ISR을 가르지 않는다) [U].

### 8.4 손실 종류 (LOSS `kind`)

`overflow`(actor 할당량 도달 또는 pool 소진; aseq 빈틈과 같은 수. 사유와 버린 바이트는 ACTOR, §8.3), `partial`, `preinit`(버린 바이트는 RUNX `boot_bytes_dropped`), `excluded`·`unloaded`(연결부·adapter가 `kz_rt_note_loss`로 알림; manifest의 `removed|unsupported` binding과 미등록 module은 호스트가 따로 보고), `reentry`, `actor-table`, `unresolved`, `ctl-skipped`, `ctl-nocond`, `module-table`, `after-stop`, `note-table`. 가득 찬 버퍼 때문에 기다리는 경로는 없다(기다리게 하면 그것도 제어 효과이므로).

### 8.5 adapter 인터페이스와 두 adapter

`kz_rt_adapter`(ABI 1): `self_cell`(필수), `native_key`, `now_ns`, `ctl_lock`/`ctl_unlock`, `hold_wait`/`hold_wake`, `resources`와 `caps` 비트. 제어 잠금은 `kz_rt_plan` 때 고정된다: 그때 잠금이 있는 adapter가 attach되어 있으면 그 잠금, 아니면 런타임 자체 spin 잠금(부트스트랩 중에 계획을 넣은 경우).

| adapter | 상태 | 정체성·시계 | 제어 | 대상 객체 소비 |
|---|---|---|---|---|
| POSIX `kz_rt_posix_adapter()` | 동작 [M] | TLS 셀, Linux TID, CLOCK_MONOTONIC | private pthread mutex, actor별 mutex+cond로 실제 대기(시간 제한) | 없음(RESOURCE: private mutex 65, cond 64). OSAL 초기화 전에도 attach 가능 |
| POSIX 시작 객체 `kzrt-posix-auto`(OBJECT 라이브러리) | 동작 [M] | `.preinit_array` 진입점이 `KZ_RT_MODE`로 `kz_rt_early_mode`를 부르고(§8.2), 생성자가 main 전에 attach + init, 종료 시 stop + dump. 설정은 환경 이름 `KZ_RT_MODE`(off/record/control), `KZ_RT_DUMP`, `KZ_RT_RUN_ID`, `KZ_RT_INSTANCE`, `KZ_RT_ACTOR_QUOTA` → 대상 환경에 더해지므로 기준 실행과의 차이로 기록. build identity는 환경으로 받지 않는다(아래) | 계획은 읽지 않음(하네스가 `kz_rt_plan`; 계획 주입은 단계 F1) | 위와 같음. 정상 `exit` 경로에서만 dump(시그널로 죽으면 없음) [U: cFS 종료 경로에서 확인 안 함] |
| OSAL `kz_rt_osal_adapter(wait_sems)` | 골격: C1 기준 OSAL `f34759d` 헤더(+ C1 빌드의 생성 `osconfig.h`)로 gcc·clang 컴파일만 [M], target 실행 안 함 | TLS 셀, `OS_GetLocalTime` | OSAL mutex 2(제어·대기표), 이진 세마포어 `wait_sems`개(= 계획의 게이트 수, 하네스가 줌) | RESOURCE origin `target-table`: mutex 2 + binsem `wait_sems` — C1 빌드의 `OS_MAX_MUTEXES 20`, `OS_MAX_BIN_SEMAPHORES 20`(`pre/1df262a/build/osal_public_api/inc/osconfig.h` L72, L65)에서 빠짐. `OS_API_Init` 성공 뒤에만 |

**build identity(RUN `backend`·`compiler`·`target`·`manifest_sha256`)** [D][M]. 단계 C의 세 후보 dump에서 이 넷이 비어 있었다(`instr_candidate_gcc.md` §8 항목 2, `instr_candidate_clang.md` §8 항목 1·2, `instr_candidate_llvm.md` §8 항목 2). 이것은 실행의 속성이 아니라 바이너리의 속성이므로 빌드 때 넣는다.
- 경로: 고정 소스 `runtime/src/kz_rt_build_info.c`를 파생 빌드가 대상 컴파일러로 `-D` 값과 함께 컴파일하고, 그 객체를 실행 파일에 링크한다. CMake로는 런타임 단독 빌드의 캐시 변수 `KZ_RT_BUILD_BACKEND`(필수)·`KZ_RT_BUILD_COMPILER`·`KZ_RT_BUILD_TARGET`·`KZ_RT_BUILD_MANIFEST`가 OBJECT 라이브러리 `kzrt-build-info`를 만든다(함수 `kzrt_build_info()`). 객체로 링크해야 한다. 런타임의 참조가 weak라 archive 안에 두면 끌려오지 않는다.
- manifest sha256은 그 manifest 파일 바이트의 SHA-256이다(CMake가 구성 때 계산). 연결부가 `kz_rt_module`에 넘기는 값(kzgcc: manifest 텍스트의 SHA-256)과 정의가 같다. 그러므로 `KZ_RT_BUILD_MANIFEST`에는 연결부의 `manifest=`와 같은 파일을 준다. `KZ_RT_BUILD_COMPILER`가 없으면 그 객체를 컴파일한 컴파일러의 `__VERSION__`을 적고(`__VERSION__ 13.3.0`), 값의 출처를 구조체 `origin`에 남긴다.
- 저장소 규칙(lint `no-generated`: 생성 코드와 코드를 생성하는 코드를 두지 않음) 때문에 소스를 쓰는 생성 도구 대신 고정 소스 + `-D`를 택했다. plugin이 정의를 내보내는 길은 택하지 않았다. plugin은 번역 단위마다 돌기 때문에 실행 파일 전체에 정의 하나를 보장하지 못하고(적재 모듈 `.so`에도 들어감), target 이름을 따로 받아야 한다 [I].
- 런타임 핵심은 `kz_rt_build_info`를 weak로 참조한다(라이브러리 기호가 아님, `rt_no_library_symbols` 통과). 있으면 dump의 RUN 태그를 그 값으로 쓴다. 하네스가 `kz_rt_config`로 다른 값을 주면 바이너리의 값을 유지하고 RUNX `tag-conflict`를 남긴다(검사 실패). sha256이 16진 64자리가 아니면 `build-info-bad`다(검사 실패, 길이는 컴파일 때 검사).
- `kz-rt-dump --manifest`는 RUN과 MODULE이 가리키는 sha256을 준 파일들의 sha256과 대조하고, 어느 것에도 없으면 실패한다(바인딩 검사가 빌드와 다른 manifest로 이루어졌다는 뜻). 같은 module이 두 manifest로 등록되었거나 RUN과 MODULE의 sha256이 다르면 역시 실패다 [M: `rt_auto_bi_dump`, `rt_auto_bad_dump`, `rt_mode_api_latch_dump`, `rt_dump_tool`, `rt_dump_tool_strict`]. 단계 C의 GCC dump 2개는 저장소의 manifest(`instrument/gcc/sites/c1_post_1df262a.kzs` `8dbdd2e8…`, `r2_v701.kzs` `a4a92b90…`)와 맞았다 [M].

**모듈 등록** [D][M]. `kz_rt_module`은 (module, sha256) 쌍마다 MODULE 항목 하나를 두고, 같은 쌍의 다음 호출은 `registrations`만 센다. 단계 C에서 단위마다 생성자를 둔 연결부는 C1 tx 6번(core 4, cf 1, bp 1), R2 8번을 따로 등록했다(최대 32에서 `module-table` 손실). 이제는 module 수만큼 항목이 생긴다(시험 `rt_mode_api_latch`: 3번 → `registrations=3`). 서로 다른 스레드의 첫 등록이 겹치면 같은 쌍이 두 항목이 될 수 있고 reader가 더한다. 같은 module의 다른 sha256은 따로 남는다(검사 실패).

### 8.6 dump 형식 `kzrt-dump` 1.1 (`include/kz_rt_format.h`)

모든 필드 little-endian, 바이트 단위 직렬화(대상의 byte order·정렬·struct 배치와 무관). 레코드는 **쓸 때** 직렬화되므로 버퍼 메모리를 그대로 떠도(디버거·JTAG) 같은 형식이다. 파일 시스템을 요구하지 않는다(`kz_rt_dump`는 바이트 콜백; POSIX는 `kz_rt_posix_dump_file/fd`).

```
"KZRTDUMP" u16 major=1 u16 minor=1 u32 dump_flags(1 = stopped)
레코드: u32 size(4의 배수, 전체) u16 type u16 sub, 본문      (모르는 type은 size로 건너뜀)
순서: RUN, RUNX, MODULE*, RESOURCE*, ACTOR*, LOSS*, POINT*, BUF*, END(u64 bytes, u32 crc32)
RUNX(1.1): window_mode mode_source flags pool_bytes chunk_bytes chunks_used pool_dropped actor_quota
           boot_bytes_used boot_bytes_dropped max_modules max_notes
MODULE: module actor u64 t sha256[32] + (1.1) registrations
ACTOR:  … buf_bytes(확정) + (1.1) dropped_bytes chunks overflow_quota overflow_pool
BUF: u32 owner(0 = 공유 버퍼, 그 밖 = run actor) u32 nbytes u32 cap u32 rsv + 버퍼 레코드(EVENT 0x0101, CTL 0x0102, AEXIT 0x0103, PAD 0x0100)
     (actor의 BUF는 그 actor chunk들의 확정 레코드를 순서대로 이은 것; cap = 할당량, 없으면 pool)
EVENT: actor aseq site bind link valid u64 t u64 pc u16 nvals u16 str_len u8 str_slot + u64 v[nvals] + str(4 정렬)
CTL:   actor aseq site bind ev ctl_seq u64 t point hit subject other gate u16 reason u16 ctx   (sub = what)
```

reader는 major가 같을 때만 읽고 minor는 무시한다(레코드 끝에 필드를 더하거나 새 type을 더하는 것이 minor). 바이트 배치를 바꾸면 major를 올린다 [D]. 1.1은 minor 개정이다(새 레코드 RUNX, MODULE·ACTOR 끝의 필드). 1.0 reader는 RUNX를 건너뛰고 늘어난 필드를 무시한다. 1.1 reader는 1.0 스트림도 읽는다(없는 필드는 기록 안 됨, MODULE `registrations` = 1). 단계 C의 1.0 dump 7개를 새 `kz-rt-dump`로 읽어 모두 통과했다 [M]. hook ABI는 1 그대로다(호출 추가만, 배치·기존 호출의 인자 변경 없음). 호스트 reader `tools/decode/kz_rt_decode.{h,c}`(C), 도구 `kz-rt-dump [--manifest <kzs>]... [--events-only] <dump>`: 레코드마다 한 줄, 그다음 검사(actor별 aseq 빈틈 대 센 overflow, 용량·수요(`check capacity`), 모드 원천(`check mode`), build identity와 manifest sha256, manifest 대조, 미등록 module). 사건 줄에는 `pc=`(hook의 반환 주소: 어느 코드 사본이 돌았나. `duplicated` binding을 가르는 근거)와 `key=`(그 actor의 native key, 예 `tid:12345`; 임시·미확정 actor는 `-`)가 붙는다. 단계 C에서 이것을 대신한 `instrument/gcc/tools/kzgcc_events.c`(TSV)는 후보의 대조 스크립트용으로 그대로 둔다. 종료 코드 0 = 통과, 1 = 검사 실패, 2 = 사용법·형식 오류 [M].

### 8.7 빌드와 링크

```
# Kairos 트리 안 (최상위 CMakeLists.txt가 runtime/을 더함; kz에는 링크하지 않음)
cmake --build /home/user/work/kairos-build && ctest --test-dir /home/user/work/kairos-build -L runtime
# 단독 (교차 컴파일러·대상 플래그를 줄 때)
cmake -S kairos/runtime -B <dir> [-DCMAKE_C_COMPILER=<cc>] [-DKZ_RT_OSAL_INCLUDE_DIRS="<osal>/src/os/inc;<build>/osal_public_api/inc"]
cmake --build <dir>      # libkzrt.a, libkzrt-posix.a, [libkzrt-osal.a], libkzrt-decode.a, libkzrt-manifest.a, kz-rt-dump
# 대상 링크: 실행 파일에  -I kairos/runtime/include  …  libkzrt-posix.a libkzrt.a -lpthread   (또는 libkzrt-osal.a libkzrt.a)
#            소스를 고치지 않는 POSIX 실행: kz_rt_posix_auto.c.o 도 링크 (CMake: target_link_libraries(<exe> PRIVATE kzrt-posix-auto))
# 하네스: [kz_rt_early_mode(m, note) →] kz_rt_init(&cfg) → kz_rt_attach(kz_rt_posix_adapter()) [→ kz_rt_plan(...)] … kz_rt_stop(); kz_rt_posix_dump_file(path)
# 파생 빌드의 build identity(§8.5): 런타임 단독 빌드에 값을 주고 객체를 링크
#   cmake -S kairos/runtime -B <dir> -DCMAKE_C_COMPILER=<대상 cc> -DKZ_RT_BUILD_BACKEND="<연결부와 판>" \
#         -DKZ_RT_BUILD_COMPILER="<cc --version 첫 줄>" -DKZ_RT_BUILD_TARGET="<cc -dumpmachine>" -DKZ_RT_BUILD_MANIFEST=<연결부가 읽은 manifest>
#   링크: <dir>/CMakeFiles/kzrt-build-info.dir/src/kz_rt_build_info.c.o  (kz_rt_posix_auto.c.o와 같은 방식)
# 용량(빌드 파라미터, §8.9): -DKZ_RT_DEFINES="KZ_RT_POOL_BYTES=<bytes>;KZ_RT_CHUNK_BYTES=<bytes>"  (RUNX에 남음)
```

- `kzrt`는 C99, `-ffreestanding -fno-stack-protector`(gcc는 `-fno-tree-loop-distribute-patterns`), PIC, TLS는 initial-exec. 라이브러리 기호를 참조하지 않음을 `rt_no_library_symbols`가 확인한다(`_GLOBAL_OFFSET_TABLE_`은 링커가 정의) [M]. 이 플래그들은 계측 부분(런타임 TU)의 것이며 대상 소스의 플래그가 아니다. 파생 빌드에서는 기준 빌드와 다른 점으로 모두 기록한다(CONDITIONS_POLICY).
- 크기 상수(`KZ_RT_MAX_ACTORS`, `KZ_RT_POOL_BYTES`, `KZ_RT_CHUNK_BYTES`, `KZ_RT_BOOT_BUF_BYTES` 등)는 `-D`(CMake `KZ_RT_DEFINES`)로 바꿀 수 있고 RUN·RUNX 레코드에 남는다. 도구 용량이며 연구 조건이 아니다. 파생 빌드에서는 기준 빌드와의 차이 대장에 런타임 판과 함께 적는다.

### 8.8 시험 [M]

`ctest -L runtime` 21개: `rt_bootstrap`(init·attach 전 기록, 스레드별 임시 id, 같은 셀 링크, attach 뒤 새 스레드, preinit 손실), `rt_order`(4 스레드 동시, actor별 버퍼와 연속 aseq, 전역 순번 없음), `rt_loss`(overflow와 빈틈, 기록 중 dump의 partial, reentry, excluded·unloaded 알림, after-stop, 잘린 스트림), `rt_serial`(전 필드 왕복, little-endian 바이트 확인, 문자열 상한·잘림, valid 비트 없으면 복사 안 함, 모르는 type 건너뜀, 다른 major 거부, CRC), `rt_gate_*` 7개(관측 hook은 게이트 site에서도 막히지 않음; 보류→해제→재개와 `ctl_seq` 순서; RECORD 모드; 문맥 거부·소비, lock-held 허용; ISR 미평가; 대기 없는 adapter의 no-op; 시간 제한과 그 hit의 deferred 해제; attach 전 계획(spin 잠금)), `rt_holdsem_*` 4개(`tests/cfs-free/target/hold_{rearm,nth,defer}.kzh`의 의미를 두 스레드로: 재무장, 값 조건 + n번째, deferred + unless-released 충족, busy), `rt_manifest`(예시 수용, 잘못된 입력 17종 거부), `rt_dump_tool`·`rt_dump_tool_strict`, `rt_auto`·`rt_auto_dump`(시작 객체: setup 호출 없는 프로그램의 종료 시 dump), `rt_no_library_symbols`. 별도로 scratch 빌드에서 gcc ThreadSanitizer, gcc AddressSanitizer+UBSan(각각 nm 검사를 뺀 20개 반복), clang 18(21개)이 모두 통과했다. Kairos 기본 빌드 136개 중 135 통과 + tracefs 시험 1 건너뜀, LLVM 빌드 157개 중 156 + 1 건너뜀 [M].

0.2.0 개정(2026-10-26)으로 `ctest -L runtime`은 44개다. 더한 것: `rt_pool_{big,quota,dumps}`(actor 하나가 88,000 B를 22 chunk에 손실 없이 기록; 할당량 1,000 B에서 11개 보존·19개(1,672 B) `overflow_quota`; 작성자 4 × 3,000 레코드 동안 반복한 dump가 모두 actor별 빈틈 없는 앞부분), `rt_pool_empty`(+`_dump`, 4 chunk pool로 다시 빌드한 핵심: 소진 때 actor 2가 14개(1,232 B)를 `overflow_pool`로 잃고 RUNX `pool_dropped=14`), `rt_mode_{off,record,control,unset,bogus}`(+`_dump`, 한 실행 파일, §8.2), `rt_mode_api_{latch,late}`(+`_dump`, early 고정·init 거부·OFF 유지, build identity 우선과 충돌 기록, 모듈 중복 제거, early 전 hook 기록), `rt_auto_bi`·`rt_auto_bad`(+`_dump`, build identity 객체와 그 기본값·잘못된 sha256). 기존 `rt_dump_tool`은 `pc=`·`key=`와 manifest sha256 일치를, `rt_dump_tool_strict`는 불일치 실패를 함께 확인한다. 별도 scratch 빌드에서 gcc ThreadSanitizer와 gcc AddressSanitizer+UBSan(각각 nm 검사를 뺀 43개), clang 18(44개)이 모두 통과했다. GCC plugin 시험 6개(`instrument/gcc`, 런타임을 단독 빌드로 링크)도 0.2.0으로 통과했다. Kairos 기본 빌드 163개 중 162 통과 + tracefs 시험 1 건너뜀, LLVM 빌드 185개 중 184 + 1 건너뜀 [M]. (이 수에는 같은 작업 트리의 다른 진행 중 변경의 시험도 들어 있다.)

### 8.9 용량 선택의 도출 (도구 용량, 연구 조건 아님) [M][D]

단계 C pilot 부팅의 dump 7개(각 후보 C1·R2 1회, 1.0 형식)에서 수요를 읽었다. 크기는 레코드 크기(사건 56 B + 값 8 B × n + 문자열)의 합이고, 4 KiB chunk 수는 같은 레코드를 actor별로 chunk에 넣는 모의 배치다(레코드는 chunk를 넘지 않음). 창은 각 실행의 `t_stop − t_init`이며 기준 절차(C1 smoke 단계, R2 `env/run_cfs.sh operational ci-es-restart-poweron`)가 정한 것이다.

| 기록 (dump sha256 앞 8자리) | 창 | 사건이 있는 actor | 확정 바이트 합 | 가장 큰 actor | 부트스트랩 | 4 KiB chunk |
|---|---|---|---|---|---|---|
| GCC C1 tx (`61586c46`) | 5,681 ms | 16 | 10,352 | 4,576 | 0 | 17 |
| GCC R2 cpu1 (`0af58a06`) | 2,181 ms | 25 | 214,808 | 53,856 | 0 | 69 |
| LLVM C1 tx (`c2e25583`) | 5,680 ms | 16 | 10,352 | 4,576 | 0 | 17 |
| LLVM R2 cpu1 (`bec07d1a`) | 2,184 ms | 25 | 225,456 | **64,152** | 0 | 72 |
| Clang C1 tx (`bf0b5ba8`) | 6,180 ms | 16 | 12,480 | 3,604 | 0 | 16 |
| Clang C1 rx (`dc36298c`) | 6,130 ms | 16 | 12,480 | 3,604 | 0 | 16 |
| Clang R2 (`9bfb6f0a`) | 2,191 ms | 25 | 114,404 | 38,896 | 0 | 42 |

도출:
1. 0.1.0의 한계는 총량이 아니라 actor 사이의 배분이었다. actor 하나의 상한 65,536 B에 LLVM R2의 actor 12가 64,152 B(97.9%)로 닿았지만, 한 프로세스 전체는 최대 225,456 B로 정적 버퍼 합 4,194,304 B의 5.4%였다. 가장 큰 actor가 프로세스 합의 25–44%를 차지했다. GCC R2의 그 actor는 612개 중 316개가 피호출 쪽 site `CFE_ES_GetAppID.store`였다(selection §4 항목 6).
2. 그래서 크기를 새로 고르지 않고 배분만 바꿨다 [D]. pool = 0.1.0 정적 버퍼의 합(64 × 65,536 = 4,194,304 B)이다. bss는 0.1.0과 같고(chunk 표 2 × 1,024 × 4 B = 8 KiB가 더해짐) 같은 바이너리이므로 모든 G-I*에 같다. 관측 수요에 4 KiB 배치를 모의하면 최대 72 chunk(294,912 B, pool의 7.0%, chunk 끝 공간 포함)다. 할당량을 주지 않으면 actor 하나의 상한은 pool 전체다.
3. chunk 4 KiB [D]: 가장 큰 레코드(184 B)를 담아야 하고, 그때 chunk당 버리는 공간은 4.5% 미만이다. chunk가 작을수록 actor의 최소 점유는 줄고 pool 계수기의 원자적 덧셈은 잦아진다. 관측 기록에서 최소 점유는 25 actor × 4 KiB = 100 KiB, 덧셈은 88 B 레코드 46개마다 한 번이다.
4. 연구 조건이 아니다 [D]. 창의 길이와 site 집합은 이 선택이 정하지 않는다. 단계 D에서 site가 F0 계획 수준(34·35개, selection §4 항목 2)으로 늘면 수요가 커질 수 있다 [I]. 그 수요는 dump에 남는다(`buf_bytes + dropped_bytes`, RUNX `pool_dropped`, `kz-rt-dump`의 `check capacity`). pool이 모자란 실행은 손실로 드러나고, 다음 빌드의 `KZ_RT_POOL_BYTES`는 그 기록의 수요에서 다시 도출한다. G-I0·G-I·G-I-H는 같은 바이너리라 함께 바뀐다. 추정으로 여유 배수를 정하지 않았다.
5. 부트스트랩 64 KiB는 바꾸지 않았다. 일곱 기록 모두 0 B였다. 시작 객체가 main 전에 attach하고, 모듈 등록은 사건이 아니다.

## 9. 사건·제어 비교의 단위 (IR §13.2–13.3) [D]

| 단위 | 무엇을 맞춰 보나 | 이 문서의 근거 |
|---|---|---|
| 위치 대응 | 같은 의미 지점이 넣어지고 연결되었나 | manifest `site`/`bind`, `equiv` 상태; F0 계획 사이트 label |
| 도달 | 그 실행에서 경로가 실행되었나 | 사건 존재 + 손실 없음(§8.4). 부재 ≠ 누락: C1에서 CF가 일찍 끝나면 뒤 조회 자체가 없다 |
| 포획 적합성 | 실제 인자·반환·쓰기 값과 phase | `valid` 비트, 값, `phase`; F0 슬롯·TRUNC |
| 식별 적합성 | task·소유 앱·대상 앱·자원 세대 | actor+세대, 정체성 네 범주(§2.3) |
| 기록 완전성 | 손실·미관측·잘림 | LOSS 종류, aseq 빈틈, 관측 구간 |
| 판정 근거 | 차이가 값·상태/순서·조건부 추론 중 어디서 | 근거 네 종류(§2.10) |

동적 횟수와 정적 관측점 수를 구별한다: R2 pilot3의 "ES 20 + EVS 4"는 그 기록을 다시 해석할 때의 기준이고 새 자연 실행의 합격 조건이 아니다(IR §13.2).

제어 비교(confirm2를 첫 대조 자료로, IR §13.3): 계획의 논리 순서 / 실제 보류·재개(CTL) / 스케줄러가 낸 순서(게이트 미작용) / 최종 결과·판정 / 소진·종료·미해제(POINT `held_actor`, P13) / 제어 자원과 관측 비용(RESOURCE; hook 비용은 G-I0/G-I/G-I-H 비교로)을 따로 적는다. 첫 시제품은 의미가 서로 다른 대표 계획부터: 생성 보류와 종료 해제, 해제된 스레드의 후속 해제(H6), n번째 hit(H2), release 선도달(H5) — 단위 시험이 이 넷을 이미 덮는다(`rt_holdsem_*`, `rt_gate_separation`) [M]. 반복 횟수는 기존 실험에서 승계하지 않는다.

## 10. 남은 것 [U]

- `.kzh` + manifest → `kz_rt_point[]` 호스트 변환기(`name` → validated 슬롯, `caller-in` → 호출 측 binding 집합; 변환할 수 없는 조건은 계획 거부)와, 계획을 대상에 넣는 길(직렬화된 계획 형식). 지금은 C 배열 API뿐이다.
- ~~dump → `.kzt` 변환(중립 사건 + 원시 근거 참조)~~ → 2026-10-26 스키마 개정 3(중립 사건 층, 섹션 100–107)과 `kz convert --kzrt`로 구현했다(`docs/schema.md` §12). 호스트 reader의 항목에 스트림 안 위치(`kz_rt_dec_item.offset`)를 더했다(형식 변경 없음). ~~남은 것: 계측 기록의 정체성 결합(생성자·소유 앱)~~ → 2026-10-26 `kzs-sites 2`의 `identity`(§3)와 `kz convert --kzrt`의 결합(`schema.md` §12.3)으로 구현했다 [M].
- ~~같은 바이너리로 init 이전부터 끄는 모드 원천(§8.2)~~ → 0.2.0의 `kz_rt_early_mode`와 시작 객체의 `.preinit_array`(§8.2). 남은 것: hook별 비용 계수, ISR 셀(§8.3), `each` 게이트 busy의 형식 판 정리(§4).
- 0.2.0에서 남은 것: 변환기가 RUNX(창의 모드·원천, pool 수요)와 MODULE `registrations`를 중립 층에 옮기는 일(지금은 RUN `mode`만 읽음, §8.2). 파생 빌드 스크립트가 `kzrt-build-info` 객체를 링크하는 일(단계 D6). 두 연결부의 binding id 범위 검사(§3). 시그널·강제 종료 경로의 dump(단계 F3).
- OSAL adapter의 target 실행과 OSAL 객체 소비의 실측, 두 번째 OS.
- 컴파일러 연결부 세 후보는 이 ABI에 맞춰 시제품을 만든다(단계 C). 런타임을 고칠 필요가 생기면 ABI 판을 올리고 이 문서에 적는다.

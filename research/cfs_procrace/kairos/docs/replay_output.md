# 재생 출력의 발생별 근거와 비교 (`kz replay --out`, `kz occ`; 스키마 개정 4, 2026-10-28)

- 지위: 도구 문서다. 연구 조건을 정하지 않는다(CONDITIONS_POLICY). 아래 시험 값은 도구 시험 값이며 사례 수에 넣지 않는다(PI 결정 2026-10-28, `Architecture/spec/PI_DECISIONS_RESOLVED_20261028.md`).
- 표기: [F] 코드·문서에서 확인, [M] 실행·재생으로 확인, [I] 추론, [D] 설계 결정, [U] 미확인.
- 관련: `schema.md` §11(재생 출력 컨테이너), §13(개정 4), `phi_reference.md` §7.6(근거 종류), `event_semantics.md` §4–§5(kzp-hold 1, 실제 제어 기록).

## 1. 왜 바꿨나 [F]

- 재생은 발생마다 근거 종류를 계산한다. 그런데 컨테이너의 발생별 요약 `summary.flags`는 16비트가 다 찼다(`schema.md` §10).
- 그래서 개정 3까지는 `store-derived-order`(2026-10-26)와 `control-order`(P11)가 텍스트와 메모리에만 있었다(`plan/results_instr_pilot_20261026.md` §8.2 항목 4).
- 판정이 기댄 간선(어떤 규칙의 어느 간선인지)도 컨테이너에 없었다.
- 기존 섹션은 바꾸지 않고 새 섹션 셋을 더했다. 개정 2·3과 같은 방식이다.

## 2. 새 섹션 (`include/kz/schema/kz_replay.h`) [D][M]

| 섹션 (id) | 레코드 | 내용 |
|---|---|---|
| OCC_BASIS (108) | `kz_occ_basis` 104 B, 발생마다 1개(SUMMARIES 순서) | 판정(`status`, `status_flags`)과 요약 뷰(`observation`, `obs_reasons`), 근거 종류 전체 `basis`(KZ_OCC_BASIS), 선언 가정 `assumptions`(KZ_OCC_ASSUME), 결과 검사 `outcome`(KZ_OUTCOME_CHECK), 실행 관계 `relation`, 발생 키(아래 §3), `via` 이름, 간선 구간 `first_edge`·`n_edges`, 감사용 위치 `call`·`decision` |
| OCC_EDGES (109) | `kz_occ_edge` 24 B | 판정이 기댄 actor 사이 간선: 규칙 이름 `rule`, 종류 `kind`(rule / reads-from / control), `by-store`, 재생 trace의 위치 `from`·`to` |
| REPLAY_READ (110) | `kz_replay_read` 24 B, 1개 | 읽기의 모든 스위치(기본값이어도 적음): P1 `outcome_narrowing`, `owner_identity_targets`, `reads_from_edges`, `reads_from_outcome`, P11 `control_order`, P10 `relation_rule`. trace의 제어 간선 수, trace의 병합 순서를 선형화하며 옮긴 사건 수(0 = 옮기지 않음) |

- 근거 종류(KZ_OCC_BASIS): `direct-capture`, `state-order`, `return-derived-order`(RF, 반환 코드로 보인 사용), `outcome-narrowed`(P1), `store-derived-order`, `control-order`. 계산은 재생이 하던 그대로다(`phi_reference.md` §7.6). 이 섹션은 옮겨 적기만 한다.
- 선언 가정(KZ_OCC_ASSUME): `via-key-separation`, `owner-identity-targets`(A-OWNER), `reads-from-edges`(RF 간선을 썼음, 두 종류 모두), `role`. 역할 이름은 `context`, via 이름은 `via`에 있다.
- 간선의 정의: 상태를 읽은 고정 조건마다, 읽은 대상에 다른 actor가 쓴 쓰기 중 그 읽기와 순서가 선 것을 찾는다. 이전 패스의 순서(결과가 실제로 기댄 순서)에서 쓰기와 읽기 사이 경로 하나의 간선을 적는다. 경로는 끝에서 거꾸로 찾는다. 현재 actor 줄에 들어오는 간선 중 현재 사건 이전이고 시작이 경로 시작 뒤인 것 가운데 가장 이른 것을 고른다(`eval_trace.cpp` `Replayer::support_of`). 그래서 "필요한 간선 전부"가 아니라 "그 순서를 세운 경로 하나"다 [D].
- 위치(`call`, `decision`, `from`, `to`)는 재생 trace의 위치다. 기록과 백엔드마다 다르므로 감사용이다. 비교는 키와 규칙 이름만 쓴다.
- 같은 입력은 같은 바이트를 낸다(시각·호스트 정보 없음) [M: `test_trace_occ_basis`].

## 3. 발생 키 (경로 무관) [D]

```
rspec | op | owner | resource | ordinal
```

| 성분 | 출처 | 왜 |
|---|---|---|
| rspec | 발생의 준비 명세 | 같은 호출에 명세가 둘일 수 있음 |
| op | 사용 지점의 호출 연산(`trace.events[call].site`) | 사용 지점 label은 백엔드마다 다름(F0 SITES label, 계측 manifest label). 연산 이름은 프로필의 것이라 같음 |
| owner | 실행 task의 소유 앱 레코드 키 `app[<key>]`(수준 inferred·confirmed). 개정 5부터 그 앱 안 task의 자리 `#<task>`가 붙는다(아래). 없으면 `app:undetermined` | actor 이름·tid는 경로마다 다름(F0 커널 이름, kzrt `a13`). 소유 앱 키는 같은 규칙(`identity_rule.hpp`)으로 정해짐 |
| resource | 발생 키 값(hex). 키가 정해지지 않았으면 `name:` + 호출의 텍스트 인자 | 이름 핸들은 텍스트에서 정해져 두 경로가 같음 |
| ordinal | 그 task가 그 키로 시작한 몇 번째 발생(`Occurrence::ordinal`) | 재시도 25회를 하나씩 구별 |

- 인스턴스(tx·rx)는 컨테이너 단위다. 비교하는 두 컨테이너가 같은 인스턴스인지는 호출자가 밝힌다.
- task의 자리(2026-10-28 검토 반영, 스키마 개정 5, `schema.md` §14.2) [D]: 한 앱의 두 task가 같은 자원·명세를 쓰면 위 다섯 성분으로는 키가 겹친다. 그래서 소유 앱 안에서 task가 몇 번째인지를 키에 더했다(OCC_BASIS `task`).
  - 두 경로가 다 가진 것만 쓴다: 소유 앱과 생성 요청 사건. task 이름 텍스트는 F0에만 있고(계측 manifest의 이름 인자는 포인터만), 진입 함수 주소는 빌드마다 다르다.
  - 생성 경로 = 요청한 actor가 같은 앱을 위해 한 요청 가운데 몇 번째인지(그 actor의 프로그램 순서). 요청한 actor가 같은 앱의 task면 그 actor의 경로 뒤에 붙인다. 앱의 task를 경로 순서로 놓아 1, 2, …를 준다. 요청이 기록되지 않았거나 다른 task와 경로가 같으면 0(정해지지 않음)이고 키에 붙지 않는다.
  - cFE에서는 앱의 주 task(ES가 요청, `[1]`)가 1, 그 주 task가 만든 child task(`[1,1]`, `[1,2]`)가 2, 3이다 [I].
  - 비교는 두 컨테이너가 모두 task를 기록했을 때만(REPLAY_READ format 2) 그것을 키에 쓴다. 아니면 머리에 `task in the key: not used`를 적는다.
- 겹치는 키(2026-10-28 검토 반영) [D]: 같은 키를 한쪽의 발생 둘 이상이 가지고 다른 쪽에도 그 키가 있으면, 어느 것이 어느 것인지 정해지지 않은 것이다. 컨테이너 순서로 짝짓지 않는다. 그 묶음을 `ambiguous keys`로 발생 번호·판정과 함께 적고, 차이로 센다(종료 코드 3). 개정 전에는 컨테이너 순서로 짝짓고 수만 적었으며 종료 코드에 넣지 않았다.
- 키 값과 이름(2026-10-28 검토 반영) [D]: 한쪽은 키 값(hex)을 정했고 다른 쪽은 정하지 못해 `name:`이면, 같은 발생이 두 키로 갈려 양쪽에 "한쪽에만"으로 나왔다. 이제 남은 발생을 텍스트 인자로 한 번 더 맞춘다(키 값과 순번을 빼고 rspec, 연산, 소유 앱·task, 텍스트 인자). 그 묶음에 양쪽 발생이 하나씩이고 키 값을 정한 쪽이 한쪽뿐일 때만 짝짓는다. 짝은 `matched by the text arguments`로 따로 적고 차이로 센다. 텍스트 인자가 다르면 같은 자원이라고 하지 않는다(한쪽에만으로 남음).
- 이전 비교와의 관계: `summarize_gih.sh`의 `norm`은 `<narrowing 이름>#<이름별 순번> 상태 요약 basis`로 발생을 맞췄다(actor 이름·사건 번호 제외). 이 키도 actor 이름과 사건 번호를 쓰지 않는다. 이름 대신 키 값과 소유 앱을, 이름별 순번 대신 task별 순번을 쓴다. 한 task가 한 키를 쓰는 C1에서는 두 방식의 짝이 같다 [I].

## 4. `kz occ show`, `kz occ diff` (`tools/kz/cmd_occ.cpp`, `lib/trace/occ_table.cpp`) [D][M]

```
kz occ show <verdicts.kz>
kz occ diff <a.kz> <b.kz>        # 차이가 있으면 종료 코드 3
```

- show: 발생마다 한 줄. 키, 실행 task 이름(표시용), 판정·요약 뷰, 근거·가정·역할·결과 검사, 텍스트 인자, 간선(`규칙[종류]#from->#to`).
- diff: 키로 맞춘 뒤, 남은 것을 소유 앱 없이 한 번 더 맞춘다(그 경우 "matched without the owner"로 따로 적음). 그다음 키 값을 한쪽만 정한 발생을 텍스트 인자로 맞춘다(§3). 겹치는 키는 어느 단계에서도 짝짓지 않는다. 그다음 다음을 따로 적는다.
  - 읽기 차이(REPLAY_READ 스위치가 다를 때).
  - 판정 차이: 상태, 상태 플래그, 요약 뷰, 보류 사유.
  - 근거 차이: 근거 종류, 가정, 결과 검사, via, 역할, 간선 규칙(종류 포함, 위치 제외).
  - 텍스트 인자로 맞춘 짝, 겹치는 키의 묶음.
  - 한쪽에만 있는 발생.
- 종료 코드 3: 판정·근거 차이, 소유 앱 없이 맞춘 짝, 텍스트 인자로 맞춘 짝, 겹치는 키, 한쪽에만 있는 발생 가운데 하나라도 있을 때.
- `summarize_gih.sh`의 "status/summary same"은 판정 차이 0에, "with basis same"은 근거 차이 0에 해당한다. 간선 규칙은 이번에 더한 비교 항목이다.

## 5. 이전 컨테이너 [D][M]

- 개정 4 이전 컨테이너에는 OCC_BASIS가 없다. 그대로 읽는다.
- show는 SUMMARIES에서 판정·요약 뷰와 `summary.flags`의 근거·가정만 낸다. 행마다 "store-derived-order, control-order, edges not recorded"를 붙인다. 빈 근거로 채우지 않는다.
- 키가 없으므로 diff는 위치로 짝짓고 `summary.flags` 범위만 비교한다고 머리에 적는다.
- 확인: 기존 golden 컨테이너 dump는 바뀌지 않았다(`layout.golden`, `records.golden`만 갱신) [M].

## 6. F0 보류 기록을 공통 제어 입력으로 (`kz convert <run.kzt> --hold <hold.kzr>`, `lib/trace/hold_nctl.cpp`) [D][M]

### 6.1 무엇을 하나

- F0 `kz hold`의 실행 기록 `hold.kzr`에 실제로 한 보류·재개·해제 선도달이 있다(`tools/kz/cmd_hold.cpp`). 이것을 그 실행 컨테이너의 중립 제어 레코드 NCTL로 옮긴다. G-I-H 기록이 재생에 들어가는 길과 같은 길이다(`schema.md` §12.7).
- 기록의 사실마다:
  - 보류 줄 → NCTL `gate-hold`(잡힌 스레드의 actor). 해제됐으면 `resume`도(상대 = 해제한 actor).
  - fire 줄 → NCTL `fire`. 사유 `resumed`/`satisfied`/`consumed`, `deferred`는 기록 그대로.
  - 점 번호: 게이트 먼저, 다음 해제, 각각 기록 순서. `kz rtplan`이 런타임 점에 주는 번호와 같다. 그래서 제어 간선 규칙 이름(`control:gate<g>/release<p>`)이 두 경로에서 같다.
  - `site`: 관측기가 그 점의 바이너리·파일 offset에 둔 진입·점 probe(SITES). 반환 probe는 진입과 offset이 같지만 반환을 관측하므로 쓰지 않는다.
  - 문맥 class: `undeclared`(F0는 문맥을 검사하지 않는다, `event_semantics.md` §4).
  - `ctl_seq`: supervisor 기록의 시각 순서(같은 시각이면 gate-hold, fire, resume). 제어기의 순서이며 프로그램 순서가 아니다.
  - `t`: supervisor의 CLOCK_MONOTONIC. `raw_off`: 그 줄의 hold.kzr 안 byte 위치(BASIS `convert.hold.sha256`).
- BASIS에 `convert.hold`, `convert.hold.sha256`, 사실마다 `convert.hold.note`를 남기고, FILE_HASHES에 hold.kzr를 넣는다. 원래 F0 섹션은 그대로이고 입력이 부모(role 1)다.

### 6.2 관측 사건 대응 (사건을 만들지 않음)

- F0 보류는 점 주소의 명령이 실행되기 전에 debug register trap으로 스레드를 세운다(`probe/hold/supervisor.cpp`). 같은 주소의 uprobe는 그 명령에서 기록되므로, 그 hit의 관측 사건은 trap 뒤 그 스레드의 첫 사건이다 [F].
- 보류: 잡힌 actor에서 시각이 `held_ns` 뒤인 첫 사건. 그 actor는 `held_ns`와 `release_ns` 사이에 사건이 없어야 한다(서 있었음). 그 사건이 게이트 site여야 한다.
- fire: hit을 낸 actor에서 시각이 `t_ns` 뒤인 첫 사건. 해제 site여야 한다.
- 시각은 actor의 사건 가운데 어느 것이 그 hit인지 고르는 데만 쓴다. 두 시각은 같은 CLOCK_MONOTONIC이다(관측기는 perf `use_clockid`, `probe/loader/perf_session.cpp`) [F]. 순서는 제어가 만든 것이고 시계의 것이 아니다.
- 다음이면 사건 없이 둔다(`event` none, 플래그 `no-observation-event`, 이유는 note): 주소에 probe 없음, trap 뒤 사건 없음, 보류 구간 안의 사건, 첫 사건이 다른 site. 스레드 id에 actor가 없거나 여럿이면 변환하지 않고 note를 남긴다.
- 사건을 찾은 레코드는 플래그 `observed-after-control`(kz_neutral.h)을 단다. 재생(`real_trace.cpp`)은 이때 제어 간선을 다음처럼 둔다.
  - 시작: 해제한 actor가 해제 hit의 관측 사건 **앞**에 낸 마지막 사건(해제는 그 관측보다 먼저 작용했다).
  - 끝: 잡혔던 actor의 게이트 관측 사건 **자체**(재개 뒤 첫 사건이다).
  - 계측 경로는 게이트 관측 레코드를 대기 전에 쓰고 해제를 관측 레코드 뒤에 평가하므로 그 플래그가 없다. 그래서 간선은 예전 그대로다(그 hit 사건부터, 게이트 뒤 첫 사건까지).
- 의미는 kzp-hold 1과 P11 그대로다. 제어 순서는 이 제어 실행에서만 유효한 별도 근거 종류다. 보류만으로 값의 부재가 입증되지 않는다. 시작 상태, 다른 writer, 자원 세대, 관측된 제어 위치가 함께 맞아야 한다(재생은 상태 순서의 다른 조건을 그대로 요구한다).

### 6.3 confirm2 36회에 적용 (제어 실행, pilot/dev) [M]

- 같은 제어 사실을 두 경로에 준 비교다. 경로의 우열 주장이 아니다. 실행은 새로 하지 않았다.
- 입력: `/home/user/work/procrace/c1_runs/confirm2/<W>_<cfg>_<cand>_<inst>/{run.kzt,hold.kzr}`(36회, 계획 9개 × 4 인스턴스). G-I-H는 단계 F의 변환 기록 `/home/user/work/instr/gcc/f1/gih/<run>/replay/run.kzt`와 B′.
- 읽기: 단계 F의 재생 인수(`replay_gih.sh f0`·`gih`, 프로필 스냅숏 `97a24d8`, 닫힘 `c1prof_i1025[_rx]`). 같은 kz로 두 경로를 읽었다. 스크립트 `instrument/gcc/scripts/f0ctl_confirm2.sh`, 집계 `f0ctl_summary.sh`. 산출물 `/home/user/work/instr/gcc/f0ctl/`(실행마다 `sha256.txt`).
- 대응: hold.kzr 사실 152개(보류 52, fire 100 = 재개 52 + 선도달 48)가 모두 관측 사건에 대응했다. 사건 없음 0, 변환하지 않음 0.
- 실제 제어 줄(`control_cmp.sh`)은 36/36 G-I-H와 같다(단계 F와 같은 결과).
- 제어 순서를 끈 F0 읽기(ctloff, ctloff_p1off)는 단계 F의 F0 재생(`f1/f0re/<run>/{cur,p1off}.kz`)과 36/36 같다(판정·`summary.flags` 근거). NCTL을 더해도 끄면 예전과 같다. 2026-10-28 검토 뒤에는 이것이 규칙으로 보장된다. 제어 순서를 끈 읽기는 제어 간선으로 사건을 옮기지 않는다(§7.3 읽기 선택). 그 전에는 이 기록들에서 옮긴 사건이 0이라서 같았을 뿐이다.
- `kz occ diff`(F0+보류 기록 대 G-I-H): cur·p1off·ctloff_p1off 세 읽기 모두 36/36에서 판정 차이 0, 근거 차이 0(근거 종류·가정·결과 검사·간선 규칙 포함), 한쪽에만 있는 발생 0.
- `results_instr_gate_20261026.md` §3의 86(인스턴스당, P1을 끄고 제어 순서도 쓰지 않는 F0 읽기에서 `INCONCLUSIVE(horizon-open)`인 호출)과 그 가운데 제어 순서를 켠 P1 끔 읽기가 판정한 수:

| 계획 | 보류 해소 대상(인스턴스당) | F0 + hold.kzr 판정 | G-I-H 판정 |
|---|---|---|---|
| W1 | 25 | 25 | 25 |
| W2 | 0 | 0 | 0 |
| W10 | 25 | 25 | 25 |
| W5 | 2 | 2 | 2 |
| W6 | 26 | 25 | 25 |
| W7 | 3 | 2 | 2 |
| W8 | 2 | 1 | 1 |
| W11 | 2 | 2 | 2 |
| W12 | 1 | 1 | 1 |
| 합 | 86 | 83 | 83 |

- 네 인스턴스가 모두 같다. 남은 3개(W6·W7·W8의 BP0 첫 실패)는 두 경로 모두 그 게이트가 `satisfied`라 제어 간선이 없다.
- 해석 [I]: 단계 F의 "83/86"은 제어 근거를 판정에 연결한 효과다. 같은 제어 사실을 F0에도 주면 F0도 같은 83을 같은 근거(상태 순서 + 제어 순서, 같은 간선 규칙)로 판정한다. 이 비교에서 두 경로의 판정 차이는 수집·제어 기구에서 생기지 않았다. 이것은 이 36회 제어 실행에 한정된다. 자연 실행이나 다른 계획으로 넓히지 않는다. G-I-H 36회는 경로 대조로만 보고한다(H3에 더하지 않음, PI 결정 2026-10-28).

### 6.4 검토 반영 (2026-10-28) [D][M]

- **기록 해석.** `hold.kzr` 앞부분의 한 키 줄(`primary_binary`, `child_pid`, `debugregs_ok`, `debugregs_error`, `wall_ns`, `child_exit`, `child_signal`; `tools/kz/cmd_hold.cpp`가 쓰는 것)은 값을 줄 끝까지 텍스트로 읽는다. 개정 전에는 공백이 든 값(`debugregs_error=<메시지>`, 공백 있는 경로)을 "기록의 줄이 아님"으로 거부했다. 그래서 `debugregs_ok=no` 기록은 변환되지 않았고 그 note에 닿지 않았다. 이제 변환되고 note에 오류 메시지가 붙는다. 모르는 키에 공백이 있으면 여전히 거부한다.
- **재사용된 thread id** (시험이 드러낸 결함): 같은 tid의 actor(incarnation)가 여럿이면, 개정 전에는 사실 시각 이전에 첫 사건이 있는 것 중 가장 늦은 것을 골랐다. 그래서 새 incarnation의 첫 사건이 그 게이트 관측이면(그 전에 사건이 없음) 끝난 앞 incarnation을 골랐다. 보류가 엉뚱한 actor에 붙고, 관측 사건이 없어 간선도 없었다. 이제는 다음 순서로 고른다.
  - 사건이 사실 시각을 걸치는 incarnation(그 전과 그 뒤에 사건이 있음). incarnation은 겹치지 않는다(커널은 끝난 스레드의 id만 다시 쓴다).
  - 그런 것이 없고 그 스레드가 사실 뒤에 계속 돌았으면(해제 hit, 재개된 스레드, 해제된 보류) 첫 사건이 그 시각 뒤 가장 이른 incarnation. 계속 돌았다면 다음 사건이 그 뒤에 있어야 하기 때문이다.
  - 끝까지 해제되지 않은 보류는 첫 사건이 그 시각 이전 가장 늦은 incarnation.
- **`each` 게이트의 두 보류** (시험이 드러낸 결함): 재개 레코드의 상대(해제한 actor)를 (게이트, 잡힌 tid)로 찾았다. 한 스레드가 두 번 잡히고 다른 스레드가 해제하면 두 재개가 모두 마지막 해제자를 가리켰다. 이제 (게이트, 잡힌 tid, 해제 시각)으로 찾는다. supervisor가 보류의 해제 시각에 fire 시각을 그대로 적기 때문이다. 재생의 제어 간선은 fire 레코드로 세우므로 간선은 전에도 맞았다.
- **끝에서 끝까지의 도구 시험** (`test_trace_hold_nctl`, 도구 시험 값) [M]:
  - satisfied·consumed 해제: NCTL fire 둘(관측 사건 있음), 제어 간선 0.
  - deferred 연쇄: C가 G2에 잡혀 있다. B가 G1에서 trap되는데, 그 주소가 G2의 해제 R2이기도 해서 B의 R2 hit은 미뤄진다. A의 R1이 B를 재개하고, B의 미룬 R2가 C를 재개한다. 간선은 둘이다. A의 hit 앞 마지막 사건 → B의 게이트 관측. B의 trap 앞 마지막 사건 → C의 게이트 관측(`control:gate1/release3/deferred`).
  - 재사용된 tid: 앞 incarnation이 끝난 뒤 같은 tid의 새 스레드가 잡히고, 그 첫 사건이 게이트 관측이다. 보류·해제·재개가 모두 새 incarnation에 붙고 간선이 하나 선다. 해제되지 않은 보류는 그때 있던 incarnation에 붙는다.
  - `each` 게이트에서 한 스레드가 두 번 잡히고 A와 M이 하나씩 해제한다. 보류마다 자기 관측 사건(첫·둘째 create 진입), 자기 재개(상대 A, M), 자기 간선이 있다.
- confirm2 36회: 고치기 전 kz(a13b37a 빌드)와 고친 kz로 `kz convert <run.kzt> --hold <hold.kzr>`를 돌렸다. 변환 컨테이너의 정규 dump(`kz-dump`)가 tool hash·artifact hash 줄을 빼고 36/36 같다(NCTL의 actor·상대·사건 포함). 이 기록들에는 tid 재사용도, 다른 스레드가 해제한 `each` 게이트의 두 보류도 없다. 그래서 §6.3의 대응 수(사실 152개, 사건 없음 0)는 그대로다 [M].

## 7. 병합 순서의 표현 독립성 (`eval::linearize_real_trace`, 2026-10-28) [D][M]

### 7.1 무엇이 병합 순서에 기대고 있었나 [F]

- 중립 층의 사건 순서는 병합 순서다. actor 사이의 순서가 아니다(`schema.md` §12, SPEC 5.8). kzrt 변환은 actor마다 aseq를 지키면서 (시계 있음, 시각, decode 순서, actor)로 합친다(`lib/trace/kzrt_convert.cpp` 병합 부분). 시계 없는 사건(부트스트랩)은 버퍼 순서로 먼저 온다.
- 그런데 이미 세운 순서 근거를 받아들일지 정할 때 병합 위치를 썼다.
  - 제어 간선: 시작이 끝보다 병합 순서상 앞이 아니면 만들지 않았다(`real_trace.cpp`, 개정 전).
  - reads_from 간선: 공개가 사용보다 뒤에 병합되면 만들지 않았다(`eval_trace.cpp` `reads_from_edges`).
  - spawn 간선: 새 task의 진입이 생성 호출보다 앞에 병합되면 짝짓지 않았다(`final_edges`, 조용히).
  - 순서표(`build_hb_order`)는 병합 순서로 한 번 훑는 vector clock이다. 뒤로 가는 간선이 하나라도 있으면 기록 전체의 actor 사이 순서를 읽지 않았다.

### 7.2 도구 시험: 표현만 바꿈 (`tests/cfs-free/test_trace_merge_order.cpp`) [M]

- 같은 actor 내부 순서와 같은 제어 사실을 두고 병합 표현만 바꿨다. 시각 동률을 반대로 깬 경우, 시계 없이 버퍼를 하나씩 이은 경우, decode 순서를 바꾼 경우다. 작은 합성 입력이며 사례가 아니다.
- 고치기 전(같은 시험을 linearize 없이 돌림):
  - 동률(조회가 생성보다 앞에 병합): RF 간선이 사라졌다. READY는 같지만 근거가 `state-order + return-derived-order`에서 `outcome-narrowed`로 바뀌었다.
  - 버퍼 순서(A, B, 그다음 생성자 M): RF와 spawn 간선 둘이 모두 사라졌다. P1을 끄면 READY가 INCONCLUSIVE가 되었다.
  - 제어(P1 끔, 생성이 조회보다 앞에 병합): 뒤로 가는 제어 간선 하나 때문에 actor 사이 순서 전체를 읽지 않았다. `NOT_READY`(상태 순서 + 제어 순서)가 INCONCLUSIVE가 되었다.
  - 어댑터(kzrt식 중립 층, 시계 없이 잡힌 actor의 버퍼가 먼저): 제어 간선을 "not before in the merged order"로 버렸다.
- 즉 유효한 간선과 판정이 표현에 따라 바뀌었다. 지금 보존 기록(CLOCK_MONOTONIC, 시각이 다름)에서 이것이 일어났다는 뜻은 아니다. 보존 기록의 출력에는 그런 거부가 없었다 [M: 저장된 재생 출력 grep].

### 7.3 고친 것: 알려진 순서의 선형 확장 [D]

- 재생 전에 trace의 사건 순서를 기록의 알려진 순서의 선형 확장으로 만든다. 시계는 동률 해소로만 쓴다.
  - 바탕: 프로그램 순서(actor마다 trace 순서)와 첫 패스의 규칙 간선. reads_from·spawn 짝은 바탕에 넣지 않는다.
  - 후보: 실행의 제어 간선, 첫 패스가 찾은 reads_from·spawn 짝 전부. 짝은 병합 순서상 앞으로 가든 뒤로 가든 같게 다룬다.
  - 동률 해소: 알려진 순서가 정하지 않는 사건 사이에서는 병합 순서가 앞선 것이 먼저다(Kahn, 최소 위치 우선). 그래서 이미 선형 확장인 병합 순서는 그대로다.
- 거부(2026-10-28 검토 반영) [D]:
  - 후보 하나가 바탕과 모순이면(끝이 바탕에서 이미 시작보다 앞섬) 거부한다.
  - 남은 후보를 모두 넣었을 때 순환 위에 있는 후보는 모두 거부한다(강연결 성분). 기록의 순서 사실끼리 모순이라는 뜻이다. 어느 것이 먼저 왔는지, 병합 순서상 어느 방향인지는 결정에 쓰지 않는다. 그래서 표현이 달라도 같은 간선이 거부된다.
  - 이유: 제어 사실과 값의 근거(RF)가 모순이면 어느 쪽도 다른 쪽을 이기지 않는다. 보류 하나만으로 값의 부재가 증명되지 않는다(P11, `event_semantics.md` kzp-hold 1). 반대로 값이 맞는다고 실제 제어 사실을 버리지도 않는다. 둘 다 받지 않으면 판정은 보류 쪽으로 간다.
  - 거부된 제어 간선은 trace에서 빼고 `order control edge … refused: …`, `control … no edge (…)` 줄로 적는다. 거부된 짝은 `Trace::refused_matches`에 남고, 재생은 그 짝에 간선을 만들지 않으며 finding으로 적는다.
  - 서로 모순인 제어 간선 둘: 개정 전에는 뒤의 것(ctl_seq)만 거부했다. 지금은 둘 다 거부한다.
- 읽기 선택(2026-10-28 검토 반영) [D]: 선형화는 그 trace를 읽을 재생의 읽기 선택으로 한다(`RealTraceOptions::reading`; `kz replay`, `kz predict`가 넘김).
  - 제어 순서를 끈 읽기(`--control-order off`)에서는 제어 간선을 후보로 받지 않는다. 그래서 P11 민감도 읽기는 NCTL이 없는 같은 기록의 읽기와 같다.
  - reads-from 간선을 끈 읽기에서는 RF 짝을 받지 않는다. 반환 코드 사용을 끈 읽기(`--reads-from-outcome off`)에서는 저장으로 보인 사용만 받는다. 첫 패스가 같은 선택으로 읽기 때문이다.
  - `--predicate`가 필요한 관측 규칙(R19 CORE-SERIAL 등)의 간선도 첫 패스에 들어간다.
  - 개정 전에는 어댑터가 기본 읽기로 선형화했다. 꺼진 근거가 사건 위치를 바꿀 수 있었다.
- 손실 구간(2026-10-28 검토 반영) [D]: 재생은 구간의 시작부터 기록된 쓰기가 아무것도 정하지 않는다고 읽는다(`Replayer::apply_losses`). 그래서 옮긴 뒤에도 그 절단을 지킨다.
  - 새 시작 = 옛 시작 이후 사건들의 새 위치 중 가장 앞.
  - 새 끝(닫힌 구간) = 옛 끝 이전 사건들의 새 위치 중 가장 뒤 + 1.
  - 그래서 구간은 전에 덮던 것을 모두 덮고, 손실 뒤에 있던 사건은 손실 앞으로 가지 않는다. 개정 전 규칙("덮던 사건의 최소~최대")은 손실 뒤의 읽기를 구간 앞에 둘 수 있었고, 그 읽기가 `loss` 사유를 잃었다.
- 한 번만 한다(첫 패스의 간선과 짝). 모델 trace와 순서가 선언된 기록은 그대로다.
- 위치: `eval::linearize_real_trace`(`include/kz/core/eval_trace.hpp`). 어댑터(`real_trace_from_neutral`)가 trace를 만든 뒤 부른다. `kz replay`, `--emit-trace`, `kz predict`가 같은 규칙으로 읽는다. 읽기 선택이 없으면 기본 읽기와 `RealTraceOptions::versions`를 쓴다.
- 출력: 사건을 옮기면 report에 `order merged order linearized: N of M events moved …` 줄이 붙고, REPLAY_READ `reordered`에 그 수가 남는다. 옮기지 않으면 출력은 예전과 같다.
- 시험(`test_trace_merge_order`, 도구 시험 값) [M]:
  - 7.2의 모든 표현이 시계 순서 표현과 같은 간선(규칙, actor, actor 안 위치)과 같은 판정·근거를 낸다.
  - 모순(검토 사례): 성공한 조회가 B의 생성을 읽고(RF), 제어 간선이 조회 반환 → B의 생성 호출이다. 생성이 먼저 병합되든 조회가 먼저 병합되든 두 간선이 모두 거부되고 같은 판정이 나온다. P1을 끄면 READY도 NOT_READY도 아니다. 개정 전에는 표현에 따라 READY(상태 순서 + RF)와 NOT_READY_UNJUDGED(상태 순서 + 제어 순서)로 갈렸다.
  - 손실: 뒤로 가는 제어 간선이 있는 기록과 뒤로 가는 RF 짝이 있는 기록 모두, 손실 뒤의 읽기가 `loss` 사유를 지킨다. 옛 시작 이후 사건은 모두 새 시작 이후에 있다.
  - 읽기: 제어 순서를 끈 읽기는 제어 간선이 있든 없든 같다(사건 0개 이동). RF를 끈 읽기는 뒤로 병합된 RF 짝 때문에 사건을 옮기지 않는다. 어댑터: 제어 순서를 끈 읽기의 trace는 NCTL을 뺀 층의 trace와 같다.
- 보존 기록 [M]: 개정 전 kz(9d7eccb 빌드)와 개정 후 kz로 같은 명령을 돌렸다. R2 v153_pilot3, C1 smoke3 post_1df262a, confirm2 F0(`kz convert --hold` 결과) 36회와 그 G-I-H 짝 36회다. 읽기는 cur, p1off, ctloff+p1off, rfoff, rfoff+p1off, `--emit-trace`다. 출력 텍스트는 모두 같다(`wrote` 줄 제외). `--out` 컨테이너도 `kz occ diff`로 판정 차이 0, 근거 차이 0이다. 이 기록들에서는 뒤로 가는 후보가 없어서 옮긴 사건이 0이고 거부도 없다. 그래서 §6.3의 수(F0 + hold.kzr 83/86, G-I-H 83/86)는 그대로다.

### 7.4 남은 병합 순서 사용 [U]

- 기본 범위 판정(SCOPE): 발생의 결정과 종료 요청의 병합 순서로 범위를 정한다(`ordered_in`). 선형화 뒤의 순서를 쓰지만 시각 동률에서 둘 사이 순서는 여전히 표현이다.
- 규칙의 짝 고르기(같은 값의 진입이 여럿일 때 첫 미사용 진입, 공개와 읽기 사이의 종류 전체 쓰기 검사)는 선형화 뒤의 위치를 쓴다.
- 관측 규칙(Observe)의 "조건이 참이 된 순간의 마지막 쓰기"는 병합 순서다.
- 위 셋은 이번 도구 시험의 대상이 아니다. 시각 동률이 많거나 시계가 없는 기록에 적용하기 전에 같은 방식으로 시험한다.

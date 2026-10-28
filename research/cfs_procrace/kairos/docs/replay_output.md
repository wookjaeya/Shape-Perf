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
| owner | 실행 task의 소유 앱 레코드 키 `app[<key>]`(수준 inferred·confirmed). 없으면 `app:undetermined` | actor 이름·tid는 경로마다 다름(F0 커널 이름, kzrt `a13`). 소유 앱 키는 같은 규칙(`identity_rule.hpp`)으로 정해짐 |
| resource | 발생 키 값(hex). 키가 정해지지 않았으면 `name:` + 호출의 텍스트 인자 | 이름 핸들은 텍스트에서 정해져 두 경로가 같음 |
| ordinal | 그 task가 그 키로 시작한 몇 번째 발생(`Occurrence::ordinal`) | 재시도 25회를 하나씩 구별 |

- 인스턴스(tx·rx)는 컨테이너 단위다. 비교하는 두 컨테이너가 같은 인스턴스인지는 호출자가 밝힌다.
- 한 앱의 두 task가 같은 자원·명세를 쓰면 키가 겹친다. 그때는 컨테이너 순서대로 짝짓고 "keys shared" 수를 출력한다.
- 이전 비교와의 관계: `summarize_gih.sh`의 `norm`은 `<narrowing 이름>#<이름별 순번> 상태 요약 basis`로 발생을 맞췄다(actor 이름·사건 번호 제외). 이 키도 actor 이름과 사건 번호를 쓰지 않는다. 이름 대신 키 값과 소유 앱을, 이름별 순번 대신 task별 순번을 쓴다. 한 task가 한 키를 쓰는 C1에서는 두 방식의 짝이 같다 [I].

## 4. `kz occ show`, `kz occ diff` (`tools/kz/cmd_occ.cpp`, `lib/trace/occ_table.cpp`) [D][M]

```
kz occ show <verdicts.kz>
kz occ diff <a.kz> <b.kz>        # 차이가 있으면 종료 코드 3
```

- show: 발생마다 한 줄. 키, 실행 task 이름(표시용), 판정·요약 뷰, 근거·가정·역할·결과 검사, 텍스트 인자, 간선(`규칙[종류]#from->#to`).
- diff: 키로 맞춘 뒤, 남은 것을 소유 앱 없이 한 번 더 맞춘다(그 경우 "matched without the owner"로 따로 적음). 그다음 다음을 따로 적는다.
  - 읽기 차이(REPLAY_READ 스위치가 다를 때).
  - 판정 차이: 상태, 상태 플래그, 요약 뷰, 보류 사유.
  - 근거 차이: 근거 종류, 가정, 결과 검사, via, 역할, 간선 규칙(종류 포함, 위치 제외).
  - 한쪽에만 있는 발생.
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
- 제어 순서를 끈 F0 읽기(ctloff, ctloff_p1off)는 단계 F의 F0 재생(`f1/f0re/<run>/{cur,p1off}.kz`)과 36/36 같다(판정·`summary.flags` 근거). NCTL을 더해도 끄면 예전과 같다.
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

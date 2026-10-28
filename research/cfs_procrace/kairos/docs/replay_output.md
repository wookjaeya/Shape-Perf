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

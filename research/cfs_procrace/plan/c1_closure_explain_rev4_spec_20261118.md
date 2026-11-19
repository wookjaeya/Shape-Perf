# 설명 스크립트 개정 4 의 명세 (2026-11-18; 코드보다 먼저 커밋함)

설명 스크립트(`env/obs1/explain.py`, 지금 개정 3)의 다음 개정이 무엇을 읽고 무엇을 내는지를 항목마다 적는다. 코드는 아직 없다. 이 글이 코드보다 먼저 커밋되고, 코드가 이 글과 달라야 하면 이 글을 먼저 고쳐 커밋한다. 따로 표시하지 않는 한 [보강 제안]이다.

(2026-11-19) 2026-11-18 의 판을 판정 규칙 8.13, 형식(`kairos/docs/static_facts.md` 5x, `kairos/docs/plan_manifest.md` 10절, `kairos/docs/event_semantics.md` 13절), 평가 절차와 맞춰 보고 마무리했다. 더한 것: 후보 목록과 인정 파일의 형식(4.1), 실패한 생성의 되돌림을 이름에 붙이는 규칙의 재진입 조건(3절 5), 필드 저장값 규칙으로 선 갈래의 표지(3절 8), 요약과 기대 대조 스크립트의 동결(3절 13, 8절). 앞판의 뜻은 바꾸지 않았다.

(2026-11-19 PI 결정 뒤) 11절은 판정 규칙 8.14 와 평가 절차 14절을 따라 판정 부분의 고침 넷(문맥 줄이 없는 행의 문맥, 항목이 없는 라이브러리 호출, 찾은 사용의 이름 상태, 요소가 기대는 목록 항목)과 서식 검사 16 의 넓힘, 읽는 부분의 새 입력 하나, 요약의 주·보조 표지와 표, 기대의 줄마다 가름, 반증·평가 스크립트, 부팅 전 다시 재기를 정한다. 판정 부분과 읽는 부분의 sha256 이 바뀌므로 동결을 다시 한다. 1–10.7 의 글은 지우지 않는다.

- 규칙: 판정 규칙(`plan/ovm_rev1_20261116/JUDGEMENT_RULES_REV1.md`) 3.2–3.6, 8.2–8.5, 8.12, 8.13. 아래에서 "8.13-n" 은 8.13 의 n 번이다.
- 형식: relations rules revision 14 의 줄과 쌍 파일(`kairos/docs/static_facts.md` 5x), 계획 sidecar 의 줄(`kairos/docs/plan_manifest.md` 10절), dump 글과 같은 빌드 줄(`kairos/docs/event_semantics.md` 13절).
- 절차: `plan/c1_closure_evaluation_protocol_20261118.md`.

## 1. 개정 3 에서 그대로 두는 계약

1. 명령줄에 이름 붙은 파일만 열고, 연 파일마다 sha256 을 기록의 `inputs` 에 넣는다 [직접 확인: `env/obs1/explain.py:17-19`, `read_input`].
2. 사례의 값을 읽지 않는다: 행, site, 값, 키, 상수는 계획(sidecar, manifest), relations 글, 결합 파일, dump 에서만 온다. 행 번호·키·앱 이름·기대 판정은 인자가 아니다.
3. 판정(`tool_verdict`)은 축의 상태만으로 계산한다(3.3). 처리 표시와 실행의 반환 등급은 `use_cond` 의 어느 필드에도 들지 않는다(3.6 의 3·5).
4. 기록 하나는 (빌드, 계획된 관계 행, 소비 행위자) 하나다. 모든 기록은 쓰기 전에 서식 검사를 지나고, 어긋나면 `refused.txt` 에만 적는다(rc 4).
5. 생성 인자 묶기, root 묶기, 호출 스택의 규칙은 개정 3 그대로다. 실행 반환의 실패 등급 규칙도 그대로이고 `odf` 에만 쓴다.

## 2. 두 부분

스크립트를 두 부분으로 나누고 sha256 을 따로 적는다(모든 기록의 `inputs` 와 부팅 선언에).

| 부분 | 하는 일 | 하지 않는 일 |
|---|---|---|
| **읽는 부분** (`env/obs1/explain_read.py`) | 입력 파일을 열고, 형식 점검(4절)을 하고, 줄을 정규화한 중간 글 `kz-explain-normal 1`(JSON 하나)로 옮긴다. 값마다 온 파일의 sha256 과 줄(또는 dump 사건의 행위자와 차례)을 붙인다 | 항목·요소·판정을 계산하지 않는다 |
| **판정 부분** (`env/obs1/explain_judge.py`) | 중간 글과 어휘 자료만 읽어 3절의 3–12 항목을 계산하고 서식 검사를 한다 | 다른 입력 파일을 열지 않는다(시험으로 점검) |

`explain.py` 는 둘을 차례로 부르는 얇은 구동부로 남는다. 나누는 까닭: 첫 실제 dump(적재된 모듈의 build-id 줄)는 부팅 때에야 생기므로, 줄을 읽는 쪽의 고침이 부팅 뒤에 필요할 수 있다. **읽는 부분만 고친 것(판정 부분의 sha256 이 같고 diff 를 남김)은 부팅의 지위를 바꾸지 않는다**고 부팅 선언에 미리 적는다. 판정 부분을 부팅 뒤에 고치면 그 부팅들은 "규칙을 정하기 전에 본 기록"이 된다. 동결은 런타임 단계의 끝 점검과, 그 단계의 런타임 시험 dump 에 대한 예행 뒤에 한다.

## 3. 항목

| 번호 | 항목 | 부분 | 입력 | 규칙 | 내는 것 |
|---|---|---|---|---|---|
| 1 | 인자와 결합 점검 | 읽는 | 개정 3 의 인자에 더함: `--pairs`(쌍 파일), `--namespaces`(여럿), `--candidates`(미대조 항목의 후보 목록), `--admitted`(인정 파일; `D2` 와 부분 인정에 필수), `--data-branch D1\|D2\|D2[종류,…]`, `--readings`(쉼표로 나눈 `Ra`–`Rf`; 없으면 `R0`) | 쌍 파일의 `facts` = sidecar 의 `input facts sha256`; 쌍 파일·relations 머리·sidecar `generator` 의 `tool_sha256` 이 서로 같음; `--namespaces` 파일마다 sha256 = sidecar 의 `input namespace-data` 줄; 인정 파일의 줄 ⊆ 후보 목록의 줄, 그 종류 집합 = `--data-branch` 의 것; relations 의 `facts` = sidecar 의 facts. **어긋난 점검은 그 입력만 물린다**(그것에 기대는 항목이 사유와 함께 `missing`); 기록을 물리지 않고, 조용히 넘기지도 않는다 | `inputs`, `input_checks[]`(점검마다 ok / refused 와 까닭) |
| 2 | 사건 | 읽는 | dump 글, manifest, 결합 파일, sidecar | `uses[]`: 행의 사용 site 의 모든 호출 인스턴스를 행위자 차례로(상태 값, out 값, 키 포인터, 안쪽 키 글과 잘림 표지·길이, 종류 값, 문맥 틀로 배정한 문맥 또는 `unknown`). `creates[]`: 행의 `creator:` 줄과 `not a pair: key constants differ` 줄의 생성 site 의 모든 인스턴스, 탐침 행의 대조 기록에서는 `not a pair: probe-create` 줄의 것도(8.12 (ㄴ)); 상태, out 값, 키 글, 종류 값, 행위자. `lifetime_ends[]`: 삭제·되돌림·표 초기화·빈 자리 지우기 사건과 각 사건이 이름 댄 것(id, 이름 글, 표 전체; 8.13-13). `buildids[]`: `buildid` 줄. `tok-*` 줄은 읽어 수만 센다 | 중간 글의 사건 목록 |
| 3 | 쌍의 항목 열둘 | 판정 | 2 의 사건; relations 행의 줄(`callee-check (success path)`, `absence origins:`, partition); 이름 공간 블록의 줄(`stored-name buffer:`, `lifetime end:`); sidecar 의 `namespace-entry`·`namespace-layer`·`lifetime` 줄; 연결 점검의 `same_build_object` 줄; 후보 목록과 인정 파일 | 기초 1–6, (가), (나) 의미, (나) 한도, (다), (라), (마)를 8.13-6…13 으로. 항목마다 `held`·`missing(<하위 사유>)`·`contradicted`, 근거, `depends_on`(기대는 후보 목록 줄), `premises`. 자료 갈래의 규칙(8.13-2)을 모든 항목에 건다 | `link.name_binding.pairs[]` 의 `items{}`, `missing_items[]`(이름), `missing_count`. 항목 하나만 보는 지름길 필드는 없다 |
| 4 | 쌍의 결과 | 판정 | 3 | `bound`: 열둘이 `held` 이고 두 키 글의 바이트가 같다. `refuted`: 8.3 의 조건 위에서 글이 다르다. `differs_from_use_key`: 글이 다르나 조건이 모두 서지 않는다(증거일 뿐). `unlinked`: 그 밖. 8.2 의 모순과 여럿 규칙 | `pairs[].result` |
| 5 | 수명과 이름 상태 | 판정 | `creates[]`, `lifetime_ends[]`, sidecar 의 `lifetime-complete` | 세대 = (종류 값, 이름 글, id), 성공한 생성에서 첫 수명 끝까지(8.13-13). 되돌림 사건은 id 를 이름 대지 않으므로 같은 행위자에서 그 앞에 있는 같은 생성 함수 실행의 `key-text` 사건의 이름 글에 붙인다. sidecar 의 `lifetime` 줄이 `reentry=possible` 이면 붙이지 않고, 그 사건은 구간 안에서 그 종류의 모든 이름에 대한 수명 끝으로 센다(`kairos/docs/plan_manifest.md` 10.4). 사용마다 이름 상태: `met`(사용의 at-call 앞에 끝난 생성의 세대가 사용 내내 열려 있음), `unmet`(그 종류·글의 세대가 사용 구간 어디에도 열려 있지 않고, 생성자 site 와 수명 끝 site 가 완전함), `undecided(<까닭>)`(겹침, 완전하지 않음, 손실, 종류 값 없음) | `uses[].name_state`, 완전성 깃발 |
| 6 | 인스턴스 연결 | 판정 | `uses[]`, `creates[]`, 5 | 8.13-14: 열린 세대에만. 같은 글의 연결 없는 생성은 `same_text_unbound` 증거 | `uses[].instance_link` |
| 7 | 기록의 연결 | 판정 | 4, 6 | 사용 사건마다의 결과를 검증된 키 글로 묶고 8.12 (ㄹ)로 집계한다. `link.basis` 는 `established` 일 때만 | `link.status`, `link.basis`, `link.missing[]` |
| 8 | 사용 조건 | 판정 | relations 행의 줄: `callee-check`, `flip:`, `absence:`, `holder:`, `alternatives [context …]:`, `creator:`, `feasibility:`; 사용의 문맥 | (c1) 개정 3 그대로. (c2) flip 줄(`Rc` 면 부재 줄). (c3): `branch_i` ∈ {`held`, `not_held(<근거>)`, `not_established(<까닭>)`}, 문맥마다 `branch_iii` ∈ 같은 셋(8.13-15, 16, 17). (c4) 개정 3 그대로(`Rd` 면 그 읽기). (iii)이 `alternatives` 줄의 `decided calls:` 에 기대어 섰으면(필드 저장값 규칙, 8.13-17) 그 갈래에 `decided_by = stored-field rule` 과 그 호출 자리를 적는다. 요약은 그런 갈래를 따로 세고, 그 규칙을 쓰기 전에 읽은 행(설계 행) 밖의 진술에 넣지 않는다. `derived` 는 `R0` 에서 (c1), (c2), (c4)와 (i)·(iii)(또는 (ii))이 모두 설 때. 출처는 정적 줄뿐이다. 읽기 갈래는 `--readings` 에 든 것만 건다 | `use_cond.*`, `use_cond.branch_i`, `use_cond.branch_iii[]`, `use_cond.caller_dependence` |
| 9 | 순서 | 판정 | 쌍 파일만 | 연결된 쌍마다: 행 키, 두 위치, 실행 행위자를 root 묶기로 옮긴 두 뿌리로 쌍 파일의 줄을 찾는다. 찾으면 값·간선·전제, 못 찾으면 `order_basis_unknown`. 행 값으로 대신하지 않는다. `order.status` 는 3.2 축 3 그대로(P 가 유도되지 않았으면 `events_only`). `hard_before` 인 쌍에 `unmet` 사용이 있으면 표시하고 따로 센다 | `order.*`, `order_ground` |
| 10 | 판정 | 판정 | 축의 상태 | 3.3 의 표 그대로 | `tool_verdict.*` |
| 11 | 중립 필드 | 판정 | 행위자의 뿌리와 소속(개정 3 의 근거), RUNX 의 프로세스 | `pair_kind` ∈ {`app-app`, `core-app`, `process-app`, `same-app-other`, `same_actor`, `core-core`, `creator-app`, `same-app-main-child`, `unknown`}; `order_ground`; `other_order_seen_in_boots`(요약이 부팅에 걸쳐 채움); 객체마다의 `same_build`; `uses`, `uses_reached`, `uses_decided`; `eval_branch`, 자료 갈래, 인정 파일과 후보 목록의 sha256 | 기록의 머리 필드 |
| 12 | 평가 갈래 | 판정 | 같은 중간 글 | 갈래마다 한 번 돌리고 출력 디렉터리를 갈래마다 따로 둔다. 읽는 부분의 출력은 갈래와 무관하다(한 번 만든 중간 글을 모든 갈래가 읽는다) | 갈래마다의 기록과 `index.tsv` |
| 13 | 요약 | 따로 둔 스크립트 | 모든 부팅·갈래의 `index.tsv` 와 기록 | 평가 절차 8·10절의 표: 판정 × `pair_kind` × 순서 값, 부팅별 분포, `other_order_seen_in_boots`, 미확정의 종류별 수. "탐지" 수와 주장의 말은 없다. 기대와의 대조는 요약도 판정 부분도 하지 않는다: 채운 기대 표를 읽는 비교 스크립트가 따로 하고, 그 스크립트는 기록을 바꾸지 않는다. 필드 저장값 규칙으로 선 갈래(8)는 제 칸에 센다. 요약과 비교 스크립트의 sha256 도 부팅 선언에 든다(8절) | 요약 표 |

## 4. 입력의 형식 점검 (읽는 부분)

| 입력 | 머리와 줄의 점검 | 어긋나면 |
|---|---|---|
| relations 글 | `kz-relations 1`, `facts`, `rules`, `tool` 줄(없으면 개정 13 까지의 글로 읽고 도구 대조는 "견줄 수 없음"); 행의 새 줄 종류 다섯과 블록의 둘을 문법대로 읽는다 | 문법에 맞지 않는 줄은 종류별로 세어 `unparsed` 에 나열하고 그 줄에 기대는 항목은 `missing(<줄 종류> not readable)` |
| 쌍 파일 | `kz-order-pairs 1`, `facts`, `tool`, `domain`, `columns`, 꼬리의 수 = 줄 수, 열 13 | 순서 입력 전체를 물린다: 모든 쌍이 `order_basis_unknown(<까닭>)` |
| sidecar | `kz-plan-sidecar 2`, `manifest-revision 4`, `namespace-entry`·`namespace-layer`·`context`·`lifetime`·`lifetime-complete` 줄의 칸 | 그 줄에 기대는 항목이 `missing` |
| 이름 공간 자료 | `semantics-data 2`, `rule` 줄, sha256 = sidecar | 그 파일을 물린다: (가) `missing` |
| dump 글 | `format kzrt-dump 1.<n>` 줄; `buildid` 줄은 1.5 부터; `ev` 줄의 칸 | `buildid` 줄이 없으면 기초 1 이 `missing(same build not confirmed)`; 값을 지어내지 않는다 |
| 연결 점검의 글 | `same_build_object`·`same_build`·`chain` 줄 | 기초 1 이 `missing` |
| 후보 목록, 인정 파일 | 4.1 의 머리와 줄의 여덟 칸, 꼬리의 수, 부분집합 관계 | 자료 갈래를 `D1` 로 낮추지 않는다: 그 실행을 rc 2 로 멈춘다(갈래가 정해지지 않은 평가는 하지 않는다) |

### 4.1 후보 목록과 인정 파일의 형식

판정 규칙 8.13 의 3·4번. 둘 다 탭으로 나눈 글이고, 주기를 준비하는 단계의 스크립트가 규칙으로 만든다(손으로 줄을 고르지 않는다). 판정 규칙이 적은 한 줄의 일곱 요소 가운데 "자료 파일과 sha256" 을 두 칸으로 나눠 여덟 칸이다.

```
kz-candidates 1
rule judgement rules 8.13 item 3 (2026-11-18)
plan <sidecar 의 sha256> <sidecar 의 경로>            (계획마다 한 줄)
columns layer<TAB>function<TAB>column<TAB>data_file<TAB>data_sha256<TAB>data_line<TAB>named_by<TAB>kind
<항목마다 한 줄>
lines <n> kinds <k>: <종류> <줄 수>; ...
end
```

- `layer`: `P0` 또는 `framework`. `function`, `column`: 그 자료 파일의 항목. `data_line`: 그 항목의 줄 번호.
- `named_by`: 그 항목을 이름 댄 줄의 종류: `P0-unverified`, `sem-unverified`(행의 계층 줄), `compare-call`, `copy-call`, `length-call`, `fill-call`(sidecar 의 `namespace-entry`·`namespace-layer` 줄). 한 항목을 여러 줄이 이름 대면 쉼표로 잇는다.
- `kind`: P0 항목은 P0 표의 family 이름, 프레임워크 항목은 `framework:<column>`. 같은 (layer, function, column) 은 한 줄이다.

```
kz-admitted 1
candidates <후보 목록의 sha256>
branch D2|D2[<종류>,...]
ground <종류>: <PI 가 댄 근거의 글 | none: evaluation branch written before the PI's answer>   (종류마다 한 줄)
columns (후보 목록과 같은 여덟 칸)
<인정하는 항목마다 한 줄: 후보 목록의 줄 그대로>
lines <n>
end
```

- `D2` 의 인정 파일은 후보 목록의 모든 줄이고, `D2[…]` 는 그 종류의 줄 모두다(종류 안에서 줄을 고르지 않는다).
- 읽는 부분의 점검: 인정 파일의 `candidates` = 넘겨받은 후보 목록의 sha256; 줄마다 후보 목록에 같은 줄이 있음; `branch` 의 종류 집합 = 줄들의 종류 집합 = `--data-branch` 의 것; 꼬리의 수. 하나라도 어긋나면 4절의 표대로 rc 2.

## 5. 서식 검사 (판정 부분; `--check` 도 같은 검사를 한다)

3.6 의 1–8, 8.5 의 9–11, 8.12 (ㄹ)의 12 는 적힌 그대로다. 13–17 은 8.13-21 그대로다: 13 `derived` 의 출처, 14 사용 사건마다의 필드와 `uses_decided ≤ uses_reached ≤ uses`, 15 근거가 파일(sha256·줄)이나 dump 사건을 가리키고 사람의 읽기를 가리키지 않음, 16 자료 상태 1 의 기록에 미대조 항목·전제에 기대어 `held` 인 것이 없음(`use_cond` 의 요소와 갈래 포함)과 `order_ground` 의 꼴, 17 `eval_branch` 와 두 sha256. 어휘 검사는 바탕 사유와 `사유(하위 사유)` 꼴을 본다(8.10; 8.13-22 의 하위 사유는 바탕 사유의 하위라 어휘 자료를 바꾸지 않는다).

## 6. fixture (합성 이름만 쓴다)

각 fixture 는 입력 글 한 벌과 기대 기록이다. 사례의 앱·함수·키 이름을 쓰지 않는다.

- 열두 항목이 하나씩만 빠진 것 열둘; 모두 `held` 인 것(`D2`)과 같은 기록의 `D1`; 종류 하나만 인정한 부분 인정.
- 모순(8.2); `bound` 인 생성 둘; 사이의 삭제, 되돌림, 표 초기화, 빈 자리 지우기(조건이 보인 것과 보이지 않은 것); 겹친 호출; 찾은 뒤 부재와 부재 뒤 찾음; 키 글 둘.
- refuted 쌍, refuted 기록과 그 세 가지 모자람.
- 다른 사실이나 다른 도구의 쌍 파일; 쌍 파일에 없는 쌍; `same_thread` 인 쌍; `hard_before` 인 쌍과 `unmet` 사용; `process-app`·`core-app`·`same-app-other`·`same_actor` 쌍.
- 문맥 둘의 사용 하나씩과 틀이 싸지 않는 사용; (c3)(i)의 `held`·`not_held`(지우는 정의, 다른 정의)·`not_established`; (c3)(iii)의 `held`·`not_held`·미대조 항목에 기댄 것·전제에 기댄 것; 읽기 `Ra`–`Rf` 하나씩이 움직이는 것.
- `buildid` 줄이 없는 dump(모든 쌍의 기초 1 이 빠짐), 한 객체만 `refused` 인 것, note 가 없는 객체; `tok-*` 줄이 든 합성 dump(수만 셈).
- 서식 검사 9–17 을 하나씩 깨는 기록(모두 물려야 한다); 판정 부분이 중간 글 말고 다른 파일을 열려 하면 실패하는 시험.
- (2026-11-19) 4.1 의 점검을 하나씩 깨는 인정 파일(후보 목록에 없는 줄, 종류 안의 일부 줄만, `candidates` 의 sha256 이 다름: 모두 rc 2); `reentry=possible` 인 되돌림(이름에 붙지 않고 그 종류의 모든 이름에 대해 셈); `decided calls:` 에 기댄 (iii)(표지가 붙고 요약에서 따로 셈).

## 7. 2026-11-16 의 입력에 대한 회귀

개정 3 을 그 입력에 먼저 다시 돌려 기준으로 삼는다(45 기록 [문서 보고: 판정 규칙 8.3 이 인용한 `records/obs1-20261116-reviewfix/index.tsv`]). 개정 4 를 같은 입력에 돌린다. 그 입력에는 쌍 파일, relations 의 새 줄, 새 sidecar 줄, `buildid` 줄이 없으므로 기대는: 새 항목이 모두 빠진 입력을 이름 붙여 `missing` 이고, 축의 상태는 기준과 같으며, 다른 것은 선언한 필드뿐이다. 이 출력에는 "규칙을 정하기 전에 본 기록"이라는 표지를 단다. 수는 그 단계가 남긴 출력에서 센다.

## 8. 끝 점검과 동결

- fixture 가 모두 통과한다(수는 남긴 출력에서). `--check` 가 서식 검사를 깨는 기록을 물린다.
- kz-static 단계의 실제 정적 출력(두 C1 빌드)과 빈 실행(`no_run`)으로 예행해, 새 줄 종류가 모두 읽히고 `unparsed` 가 0 이거나 나열된다.
- 런타임 단계의 시험 dump(시험 실행 파일과 등록된 모듈 하나의 `buildid` 줄)로 예행한다.
- 그 뒤 두 부분의 sha256 을 부팅 선언에 적는다. 요약 스크립트와 기대 대조 스크립트의 sha256 도 함께 적는다. 이 둘은 기록을 바꾸지 않으므로, 부팅 뒤에 고치면 고친 판으로 다시 내고 앞판 출력과의 diff 를 남긴다(평가 절차 12절).

## 9. 열린 것

- 읽는 부분과 판정 부분의 파일 이름, 중간 글의 칸 이름은 코드를 쓰는 단계가 이 글을 고쳐 먼저 커밋한 뒤 확정할 수 있다(뜻은 바꾸지 않는다).
- 적재된 모듈의 `buildid` 줄은 부팅 전까지 실제로 본 적이 없다 [미확인].
- 읽기 갈래 `Rd`((c4)를 partition 의 생성자 집합과 전제로 받는 것)가 정확히 어느 줄들을 받는지는 PI 의 답을 들은 뒤가 아니라 **부팅 전에** 평가 절차의 채운 판에 적는다(받는 줄: 행의 `creator:` 줄들과 그 `incomplete:` 사유, 쌍 파일의 그 쌍의 값).

## 10. 코드를 쓰며 정한 것과 실제 출력에 맞춘 고침 (2026-11-19; 코드보다 먼저 커밋함)

이 절은 개정 4 의 코드를 쓰기 전에, 위 글을 두 C1 빌드의 실제 출력(kz-static `6f40dd60…` 의 `cz_final_order/<빌드>/relations.txt`·`pairs.tsv`, `cz_plans_v4_ns2/<빌드>/plan.side`·`plan.kzs`)과 런타임 단계의 시험 dump·연결 점검 글(`kz-rt-dump` `d83fdde9…`, `env/tools/chain_check.py`)에 맞춰 보고 정한 것이다. 3–8절의 뜻은 바꾸지 않는다. **고침**이라고 표시한 것만 위 글을 바꾼다. 9절의 첫 항목이 열어 둔 파일 이름과 칸 이름을 여기서 정한다.

### 10.1 파일과 구동

- 읽는 부분 `env/obs1/explain_read.py`, 판정 부분 `env/obs1/explain_judge.py`, 구동부 `env/obs1/explain.py`(둘을 차례로 부르고 그 밖의 계산을 하지 않는다), 평가 갈래 파일을 규칙으로 만드는 `env/obs1/explain_branches.py`, 요약 `env/obs1/explain_summary.py`, 기대 대조 `env/obs1/explain_expect.py`. 개정 3 의 `explain.py`(sha256 `acfd4271…`, 커밋 `d75aa53`)는 바이트 그대로 `env/obs1/explain_rev3.py` 로 남겨 회귀(7절)와 단위 점검 15 가 읽는다.
- 읽는 부분과 판정 부분은 각각 표준 라이브러리만 가져오고 서로를 가져오지 않는다(한 파일의 sha256 이 그 부분의 모든 규칙을 덮게). 판정 부분은 중간 글(`normal.json`)과 어휘 파일만 연다. 시험은 `open` 을 바꿔 그 밖의 파일을 열면 실패하게 한다.
- 출력 디렉터리(새 디렉터리만): `normal.json`(읽는 부분의 출력, 갈래와 무관), `inputs.txt`, `input_checks.txt`, 그리고 갈래마다 `<갈래 id>/records/*.json`, `<갈래 id>/index.tsv`, `<갈래 id>/uses.tsv`(집계 전의 사용 사건 표), `<갈래 id>/refused.txt`. rc: 0 모두 씀, 2 갈래 입력(후보 목록·인정 파일·갈래 파일)의 점검이 어긋남(아무 기록도 쓰지 않음), 3 결합이 거부됨(기록은 `build_mismatch`), 4 서식 검사에 물린 기록이 있음, 64 인자 오류.

### 10.2 평가 갈래는 명시한 입력 파일이다 (고침: 3절 1 의 `--data-branch`·`--readings` 대신)

갈래를 명령줄의 손 선택으로 두지 않고, 규칙으로 만든 파일 하나를 입력으로 받는다(`--branches`). 파일에는 행 번호·키·기대 판정이 없다.

```
kz-eval-branches 1
rule judgement rules 8.13 item 5 (2026-11-18): data branches D1 and D2 (and D2[<kinds>] for each admission file given) x readings R0, each of Ra-Rf alone, all six
candidates <후보 목록의 sha256 | none>
branch <id> data=<D1|D2|D2[<종류>,...]> readings=<R0|Ra,...> admitted=<경로|none> admitted_sha256=<hex|none>
...
lines <n>
end
```

- `explain_branches.py --out FILE [--candidates FILE] [--admitted FILE]...` 가 만든다: 자료 갈래 `D1` 하나와, 인정 파일마다 그 파일의 `branch` 줄이 이름 댄 자료 갈래(`D2` 또는 `D2[…]`) 하나씩. 각 자료 갈래에 읽기 여덟(`R0`, `Ra`…`Rf` 하나씩, 여섯 모두). id 는 `<자료>-<읽기>` 를 파일 이름에 쓸 수 있는 글로 바꾼 것(`D1-R0`, `D2-Ra`, `D2_k1_k2-all` 등). 후보 목록이 없으면 `D2` 갈래를 만들지 않는다(인정 파일은 후보 목록 없이 점검할 수 없다; 4절 표의 rc 2 를 피하려고 갈래를 낮추지 않는다).
- 읽는 부분은 갈래 파일의 꼴, `candidates` 줄 = 넘겨받은 후보 목록의 sha256, 갈래마다 인정 파일의 sha256 = `admitted_sha256` 과 4.1 의 점검을 한다. 하나라도 어긋나면 rc 2 로 멈춘다.
- 읽기 갈래가 든 기록은 `eval_branch.tag = "4.2 의 읽기 갈래(PI 결정 전)"` 를 단다(서식 검사 17).

### 10.3 실제 출력에 맞춘 읽기 (고침이 아닌 정함)

1. **relations 의 개정 14 줄.** `rules` 줄은 개정 12 그대로이고, 개정 14 는 `rules with object access: relations rules revision 14 …` 줄이 이름 댄다(`kairos/docs/static_facts.md` 5x 아홉째 덧붙임). 읽는 부분은 이 줄이 있고 그 줄에 `with RESULT_FLOW and RESULT_EFFECT` 가 있을 때만 행의 새 줄 다섯(성공 경로, 부재 기원, holder, 대안, 그리고 이름 공간 블록의 `writers [<묶음>] of`·`stored-name address uses`·`stored-name buffer`·`lifetime end`)을 기대한다. 없으면 그 줄들에 기대는 항목은 `missing(<줄 종류> not in this relations text)` 의 근거를 단다(사유의 글은 8.4·8.13 의 하위 사유 그대로).
2. **행 몸의 줄과 이름 공간 블록.** 행 몸은 6 칸 들여 쓴 줄이다. 이름 공간 블록(`  namespace <키 필드> …` 머리 뒤의 6 칸 줄)은 행이 아니고, 행의 `namespace <키 필드> partition {<v>}` 가 그 블록을 이름 댄다. 블록 안의 8 칸 줄(성공 경로 줄의 되풀이, `lifecycle` 의 writer 줄)은 블록에 붙인다.
3. **쌍 파일.** 머리는 `kz-order-pairs 1`, `facts`, `tool`, `relations`, `order`, `domain`, `columns` 이고, 쌍 줄 뒤 꼬리 줄 앞에 `edge<TAB>…` 줄이 올 수 있으며, 꼬리 줄은 `rows … pairs … <값마다의 수> no_pair <n>[ edges_not_in_tsv <n>]` 이다. 점검: 열 13; `edge` 줄을 뺀 줄의 행 키 수 = `rows`; `no_pair` 가 아닌 줄 수 = `pairs` = 값마다의 수의 합; `edge` 줄 수 = `edges_not_in_tsv`. 중간 글에는 계획된 행의 줄만 싣는다(점검은 파일 전체에서 한다).
4. **쌍 줄 찾기.** (행 키, 다른 쪽 위치, 사용 뿌리, 다른 쪽 뿌리)로 찾는다. 다른 쪽 위치는 생성 사건의 site(`tu`, `line`, `col`)를 쌍 줄의 `other_pos` 의 꼬리와 줄·열로 맞춘다(쌍 파일은 묶음 루트 밖을 절대 경로로, manifest 는 묶음 안의 상대 경로로 쓴다). 뿌리는 root 묶기의 뿌리 함수와 종류를 `<함수> (<종류>)` 로 쓴 글이고, 프로세스의 첫 스레드는 종류 `process` 로 맞춘다(쌍 파일의 글은 `main (process)`). 맞는 줄이 없으면 `order_basis_unknown(no line of the pair in the pair file)`, 값이 다른 줄이 둘 이상이면 `order_basis_unknown(more than one line of the pair)`.
5. **사건이 온 객체(기초 1).** dump 의 `ev` 줄의 `bind=` 는 `(모듈 << 16) | (site << 4) | n` 이다(`kairos/docs/event_semantics.md` 3절, `kairos/instrument/gcc/kzgcc.cpp:308`). 그래서 사건의 객체는 `module:<bind >> 16>` 이고, 기초 1 은 연결 점검 글의 `same_build_object` 줄 가운데 `main` 과 그 쌍의 두 사건의 모듈 객체가 모두 `confirmed` 일 때 선다. 연결 점검 글이 옛 꼴(`same_build` 한 줄뿐)이거나 그 객체의 줄이 없으면 `record_insufficient(same build not confirmed)`, 하나라도 `refused` 면 `build_mismatch`.
6. **dump 1.5.** `buildid` 줄은 `module` 줄 뒤에 오고 경로가 잘린 줄은 끝에 ` path_cut=yes` 가 붙는다. `tok-create`·`tok-return`·`tok-start` 줄은 수만 센다. `format kzrt-dump 1.4` 이하는 `buildid` 줄이 없다고 적는다.
7. **문맥 틀.** sidecar 의 `context row=<키> id=<n> frame=<at-call site>,<after-call site>|- chain="…"` 이다(`kairos/docs/plan_manifest.md` 10.8 의 고침). `id` 는 relations 의 `alternatives [context <n>: …]` 의 n 이다.
8. **holder 가 지역 변수인 줄**(`holder: local …; local: definitions <n> (RESULT_FLOW)…; readers: not decided …`): (c3)(i)은 `not_established(the writer set of a local holder is not shown complete)` 다. 이 사실은 지역 변수의 정의를 다 싣지 않는다(5x 다섯째 덧붙임).
9. **대안 줄의 필드 저장값 규칙과 읽기 `Re`.** 실제 줄은 규칙이 정하지 못했을 때 `[the stored-field rule at <호출>: not decided: (5) or (6) rests on a premise]` 와 `premises: (6) …` 를 적고, 그 전제 아래에서 규칙이 줄 호출 결과와 그 결과로 닫히는 효과를 적지 않는다. 그래서 판정 부분은 `Re` 아래에서도 그 문맥의 (iii)을 `held` 로 만들 수 없고, `not_established(the alternatives line gives no call result under the premise)` 로 둔다(값을 지어내지 않는다). `Re` 가 (i)에서 움직이는 것은 holder 줄의 FIELD_STATUS 가 `per-field=writer-set-complete` 이고 `type-level=DISABLED` 인 때뿐이다.
10. **(다)의 두 집합.** (a) 이름 포인터 필드: 행의 `writers: <키 필드>: … premises: FIELD_STATUS per-field <상태>, type-level <상태>` 가 둘 다 `writer-set-complete` 이고, 이름 공간 블록의 `writers [<묶음>] of <키 필드>:` 줄 가운데 그 행의 partition 을 담는 묶음의 writer 가 모두 이름 공간 항목의 `create=` 함수 안의 저장이거나 상수(지우기)이며, partition 을 이름 대지 않는 묶음(`no key-parameter function reaches …`)에 상수가 아닌 writer 가 없을 때. 종류에 묶이지 않은 writer 는 그 종류의 writer 가 아니라고 보일 수 없다. (b) 버퍼: 항목의 `stored-name` 필드마다, 그 행의 partition 을 담는 묶음의 `stored-name buffer` 줄이 하나 이상 있고 모두 `complete` 일 때(읽기 `Rf` 에서는 `not complete (FIELD_STATUS disabled; every cause discharged)` 도 받는다). `stored-name address uses` 줄의 `library:` 항목은 후보 목록의 줄로 자료 갈래를 건다.
11. **후보 목록이 없을 때.** 미대조 여부를 읽을 수 없으므로 후보 목록에 기대는 항목·요소는 자료 갈래 `D1` 에서 성립이 아니다(근거 `no candidate list given: the state of the library entries is not read`). 2026-11-16 입력의 회귀와 이 단계의 예행이 이 경우다.

### 10.4 중간 글 `kz-explain-normal 1`

JSON 하나. 최상위 칸: `format`, `reader{script, sha256}`, `inputs{이름: {path, sha256}}`, `input_checks[]`(점검, `ok`|`refused`, 까닭), `unparsed{줄 종류: [위치]}`, `plan{sidecar, sites, bindings}`, `relations{head, rows, namespace_blocks}`, `keys`, `pairs{head, tail, lines}`, `namespaces[]`, `candidates`, `branches[]`(인정 파일의 줄 포함), `facts_view`, `run`(dump, run.meta, 연결 점검, 시작 스크립트, 멈춤 평가, 인스턴스 목록, 부팅 예산; 없으면 `null`), `label_record`. 줄을 옮긴 값마다 `[파일 이름, 줄 번호]` 를, dump 사건마다 행위자와 aseq 를 단다. 계획된 행(층과 대조)의 relations 행과 쌍 줄만 싣는다.

### 10.5 쌍 종류

개정 3 의 `creator-app`(프로세스의 첫 스레드와 앱)은 개정 4 에서 `process-app` 으로 낸다(8.13-18). 같은 run 행위자면 `same_actor`, 앱(또는 그 자식)끼리 같은 앱이면 주-자식 `same-app-main-child`, 아니면 `same-app-other`; 다른 앱이면 `app-app`; 코어와 앱 `core-app`, 코어끼리 `core-core`; 나머지 `unknown`. 회귀에서 이 이름 바뀜은 선언한 필드 차이다.

### 10.6 기대 대조 파일 `kz-expect 1`

```
kz-expect 1
plan <sidecar 의 sha256>                  (계획마다 한 줄)
expect<TAB><갈래 id><TAB><빌드><TAB><행 키><TAB><소비자 뿌리|*><TAB><필드><TAB><기대 값>
...
lines <n>
end
```

`필드` 는 기록의 칸 경로(`tool_verdict.violation.value`, `use_cond.status`, `use_cond.branch_i.status`, `link.status`)이거나 `pair[<생성 site 위치>].missing_items`(쉼표로 이은 이름, 차례 무관)·`pair[<위치>].result`·`use[<n>].name_state` 다. `explain_expect.py` 는 기록을 바꾸지 않고 줄마다 `match`·`mismatch`·`no_record` 를 쓴다. 기대 파일은 주기를 준비하는 단계가 평가 절차 5절에서 만든다. 이 단계는 fixture 로만 시험한다.

### 10.7 코드를 쓰며 정한 것 둘째 (2026-11-19; 코드 커밋보다 먼저 커밋함)

10.1–10.6 을 커밋한 뒤 코드를 짜고 scratch 에서 돌리며(합성 fixture, 2026-11-16 입력, 두 C1 빌드의 정적 출력) 드러난, 위 글이 정하지 않은 것이다. 판정 규칙의 글은 바꾸지 않는다.

1. **서식 검사 16 과 읽기 갈래.** 8.13-21 의 16(자료 상태 1 에 전제에 기댄 `held` 가 없다)을 글자대로 걸면, 전제를 받는 읽기(8.13-5 의 `Rd`: 생성자 집합과 그 전제, `Re`: 필드 단위 규칙만 선 완전성)가 든 `D1` 갈래의 기록은 모두 물린다(scratch 의 정적 예행: `D1-Rd` 46 기록 가운데 46 이 물림). 8.13-5 는 읽기 갈래를 "전제나 다른 증거를 받는 읽기"로 두고 모든 갈래를 평가하라고 한다. 그래서 검사 16 은 이렇게 읽는다: 읽기가 받는 전제로 선 항목·요소는 칸 `reading`(그 읽기 이름)과 `premises` 를 달고, 그 기록의 갈래가 그 읽기를 담을 때만 검사 16 을 지난다. 그 밖의 전제(읽기가 이름 대지 않은 것)는 어느 갈래에서도 성립이 아니다. `R0` 갈래의 결과는 바뀌지 않는다.
2. **자기 sha256.** 읽는 부분과 판정 부분은 자기 파일을 읽어 sha256 을 기록에 적는다(그 밖의 파일은 10.1 대로). fixture 의 열기 시험은 이 하나를 허용한다.
3. **프로세스의 첫 스레드가 만든 생성.** 그 행위자는 생성 호출이 없으므로 생성 인자 묶기가 정해지지 않는다. 기초 2 는 글대로 그 쌍에서 `missing` 이다(규칙을 바꾸지 않는다; 열린 점).
4. **사용의 수.** `uses` = 행의 서로 다른 사용 호출(at-call·after-call 짝을 하나로) × 문맥 수(적어도 1); `uses_reached` = 사건이 하나 이상인 (호출, 문맥); `uses_decided` = 기록의 `violation` 이 `observed`·`not_observed_in_runs`·`refuted_link` 이면 `uses_reached`, 아니면 0.
5. **순서 근거는 연결된 쌍에서만**(8.13-14): `bound` 쌍과 인스턴스로 연결된 사용의 생성에만 쌍 파일의 줄을 찾는다. 연결된 쌍이 없으면 기록의 `order_ground` 는 `order_basis_unknown(no linked pair)` 이고 `order.static` 은 `unknown` 이다.
6. **삭제의 성공 값.** 삭제 마무리 호출의 반환을 그 행의 성공 경로 줄의 성공 값(outcome class rule)과 비교한다. 그 줄이 없으면 삭제의 성공을 모르고, 그 사건은 이름 상태를 `undecided` 로, (마)를 "배제되지 않음"으로 만든다.
7. **경로 B·C 의 writer 호출.** 행의 writer site 의 호출 인스턴스는 개정 3 처럼 게시 사건(`other_kind writer`)으로 남긴다. 쌍(이름 결합)은 경로 A 의 생성 site 에서만 만든다.
8. **요약의 미확정 종류.** 바탕 사유를 평가 절차 8절의 다섯 종류로 나누는 표: `no_run`·`target_not_reached`·`record_insufficient` → 실행 증거 없음; `absent_result_unevaluated`·`key_unresolved`·`writer_set_incomplete`·`fixed_proof_open`·`caller_dependence_unknown`·`actor_not_extracted` → 정적 입력 없음; `plan_missing` → 자원; `link_unlinked`·`multi_state_unlinked`·`build_mismatch`·`intervention_out_of_scope` → 구조; 하위 사유 `absence after a live create of the same name` → 모순; 그 밖 → `other`. 요약은 필드 저장값 규칙으로 선 (iii)을 가진 기록을 따로 센다.

## 11. PI 결정 뒤 덧붙임: 판정 부분의 고침 넷, 읽는 부분의 입력 하나, 요약의 주·보조 표지, 기대의 가름, 반증·평가 스크립트, 부팅 전 다시 재기 (2026-11-19; 코드보다 먼저 커밋함)

근거: 2026-11-19 PI 결정(`Architecture/spec/PI_DECISIONS_RESOLVED_20261119.md`), 판정 규칙 8.14, 평가 절차 14절, lead 의 판단(`plan/full_stage_results_20261116.md` 26.0 (나)–(자)). 이 절은 코드보다 먼저 커밋한다(머리의 규칙: 코드가 이 글과 달라야 하면 이 글을 먼저 고쳐 커밋한다). 1–10.7 의 뜻은 아래에서 **고침**이라고 표시한 곳만 바꾼다. 새 파일과 칸의 이름은 코드를 쓰는 단계가 이 절을 고쳐 먼저 커밋한 뒤 확정할 수 있다(9절 첫 항목과 같다; 뜻은 바꾸지 않는다). 행 번호, 키, 앱·함수 이름, 기대 판정은 어느 스크립트의 인자도 상수도 아니다(1절 2; 평가 절차 0절). fixture 는 합성 이름만 쓰고 설계 행을 쓰지 않는다(6절). 따로 표시하지 않는 한 [보강 제안]이다.

**판정 부분을 바꾸는 항목은 1, 3, 4, 5, 6 이다.** 그래서 `explain_judge.py` 의 sha256(`531a6b3e…`)이 바뀌고 동결을 다시 한다(12절 6; 11.3). 2 는 읽는 부분을 바꾼다(`explain_read.py`, `ff0a0625…`). 7–9 와 11 은 요약, 12 는 기대 쪽, 13 은 새 스크립트, 14–15 는 실행 스크립트를 바꾼다. 이 넷은 판정 부분의 sha256 을 바꾸지 않는다. 다시 동결한 판이 생기기 전에는 부팅하지 않는다(평가 절차 14.0).

### 11.1 판정 부분과 읽는 부분

1. **문맥 줄이 없는 행의 문맥 (26.0 (나); `rules-01`; 판정 부분. 고침: 3절 2 의 문맥 배정, 10.3-7).**
   - 입력: 중간 글의 그 행의 계획 `context` 줄, 그 행의 relations `alternatives [context <id>…]:` 줄, 그 행의 사용 사건.
   - 규칙(판정 규칙 8.14 의 6 (가)): 행에 `context` 줄이 하나 이상 있으면 지금대로다(8.13-16: 그 행위자 안에서 사건을 정확히 한 문맥 틀이 싸면 그 문맥, 아니면 `unknown`). 행에 `context` 줄이 없으면, 그 행의 대안 줄들이 이름 붙인 서로 다른 문맥 id 의 집합을 본다. 그 집합이 정확히 하나이면 그 행의 모든 사용 사건의 문맥은 그 id 다. 사건의 틀과 at-call 의 유무에 달리지 않는다. id 는 줄이 적은 글 그대로이고, 줄이 `-` 를 적었으면 `-` 다. 집합이 비었거나 둘 이상이면 모든 사건의 문맥은 `unknown` 이고, 그 사건의 (c3)(iii)은 `not_established(context unknown)` 다. 줄에 없는 문맥 id 를 만들지 않는다. 지금 판의 "문맥 줄이 없으면 `-`" 길은 없앤다.
   - 출력: `uses[].context`, 갈래마다의 `uses.tsv` 의 `context`, `use_cond.branch_iii[].context`.
   - 서식 검사(5절의 14 에 더함): 문맥이 `unknown` 이 아닌 사용 사건마다 그 id 의 대안 줄이 그 행에 있다.
   - fixture(합성 행): 문맥 줄이 없고 대안 줄이 문맥 하나를 이름 대는 행에서, 그 줄이 (a) `found: …`(→ (iii) `not_held`), (b) `none found (exploration complete)`(→ `held` 로 가는 길), (c) `not established (…)`(→ `not_established`)인 셋. 문맥 줄이 없는데 대안 줄이 문맥 둘을 이름 대는 행(→ `unknown`). 대안 줄이 `[context -]` 인 행(→ `-`).

2. **이름 붙은 라이브러리 호출의 기록 수 (26.0 (다); 읽는 부분과 새 입력. 고침: 3절 1, 4절의 표, 4.1 에 더함).**
   - 판정 부분은 중간 글과 어휘 파일만 연다(10.1). 그래서 '항목 없음'을 알아보려면 중간 글에 그 사실이 있어야 한다. 새 입력 파일 하나를 둔다. 꼴은 다음과 같다.
     ```
     kz-named-calls 1
     rule judgement rules 8.14 item 5 (2026-11-19): every library call that a planned row's namespace-entry or namespace-layer line names, with its records in the facts
     plan <sidecar 의 sha256> <sidecar 의 경로>            (계획마다 한 줄)
     facts <facts 의 sha256> <facts 의 경로>              (계획마다 한 줄)
     columns function<TAB>named_by<TAB>records<TAB>unverified<TAB>layers
     <호출마다 한 줄>
     lines <n> no_entry <m>
     end
     ```
     `named_by` 는 그 호출을 이름 댄 칸(`compare-call`, `copy-call`, `length-call`, `fill-call`; 여럿이면 쉼표)이다. `records` 는 그 facts 의 P0 표와 SEMANTICS 기록 가운데 그 함수의 기록 수(모든 칸)다. `unverified` 는 그 가운데 `verify=unverified` 인 것의 수다. `layers` 는 기록이 있는 층(`P0`, `framework`; 없으면 `-`)이다. `no_entry` 는 `records = 0` 인 줄의 수다.
   - 만드는 것: 주기를 준비하는 단계의 스크립트가 후보 목록과 같은 입력(두 계획, relations, facts)과 같은 규칙(판정 규칙 8.13 의 3번의 '이름 댄 항목')으로 만든다. 후보 목록을 만든 스크립트에 선택을 더해도 되고 따로 둔 스크립트로 만들어도 된다. 어느 쪽이든 후보 목록과 인정 파일이 바이트까지 같음을 다시 만들어 보인다(26.0 (너)). 두 빌드의 수가 다르면 그 스크립트는 멈춘다(후보 목록의 규칙과 같다). 파일의 자리는 `records/closure-c1prep-20261119/candidates/named_calls.tsv` 로 한다.
   - 읽는 부분: 새 인자 `--named-calls FILE` 이다. `--candidates` 가 주어지면 이것도 있어야 한다. 점검은 머리, 열 다섯, 꼬리의 수, `plan` 줄들이 후보 목록의 `plan` 줄들과 같음, 그 실행의 sidecar sha256 이 그 가운데 있음, 그 계획의 `namespace-entry`·`namespace-layer` 줄의 네 칸이 이름 댄 함수가 모두 파일에 있음이다. 하나라도 어긋나면 후보 목록처럼 rc 2 로 멈추고 아무 기록도 쓰지 않는다(4절의 표: 갈래가 정해지지 않은 평가는 하지 않는다).
   - 출력: 중간 글의 새 칸 `named_calls{<함수>: {named_by, records, unverified, layers, src: [파일 이름, 줄 번호]}}`, 그리고 `inputs`·`input_checks` 의 줄.
   - fixture: 머리가 다른 것, 꼬리의 수가 틀린 것, `plan` 줄의 sha256 이 다른 것, 이름 붙은 호출이 빠진 것. 모두 rc 2 여야 한다.

3. **항목이 없는 호출의 판정 (26.0 (다); `rules-04`; 판정 부분. 고침: `entry_state`, 3절 3 의 (나) 의미와 (다), 3절 8 의 (c3)(iii)).**
   - 입력: 중간 글의 `candidates`, `named_calls`, 평가 갈래, relations 의 버퍼 주소 쓰임 줄.
   - 규칙(판정 규칙 8.14 의 5): 라이브러리 호출 하나의 상태를 다음 차례로 정한다.
     - (a) 후보 목록에 그 (층, 함수, 칸, 이름 댄 줄)의 줄이 있으면 지금대로다. 그 자료 갈래에서 인정되면 성립하고, 아니면 미대조다.
     - (b) 후보 목록에 없고 `named_calls` 에서 그 함수의 `records = 0` 이면 **항목 없음**이다. 모든 자료 갈래에서 성립하지 않는다.
     - (c) 후보 목록에 없고 `records > 0` 이면 지금대로 '미대조가 아님'이다. 검증 상태가 unverified 가 아닌 기록이 있다는 뜻이다(PI 결정 ⑤).
     - (d) 후보 목록에도 `named_calls` 에도 없으면 상태를 읽지 못한 것이다. 성립하지 않고, 하위 사유는 `the state of the entry of <호출> is not read` 다.
     - 후보 목록이 없는 실행(10.3-11)은 지금대로다.
   - 조건 (다)의 버퍼 주소 쓰임(판정 규칙 8.13-11 (b), 8.14 의 5): relations 의 `stored-name address uses` 줄에서 `library: <호출> parameter <k>: no entry says the parameter is not written` 인 쓰임은 그 매개변수의 항목이 없는 것이다. 그런 쓰임이 하나라도 있으면 (다)는 모든 갈래에서 `missing` 이고, 근거에 그 쓰임들을 적는다. 읽기 `Rf` 도 이것을 움직이지 않는다. 이 주기에는 쓰임 20 가운데 풀리지 않은 14 의 13 이 이 꼴이고, 나머지 하나는 가변 인자다 [M: `/home/user/work/redesign/obs1/cz_final_order/c1_pre_tx/relations.txt:608` 의 `discharged 6, not discharged 14` 와 `no entry says the parameter is not written` 13 번; PI 결정 §2 A5].
   - 사유의 꼴: 항목과 요소의 바탕 사유는 그대로 두고, 하위 사유를 `no entry: <호출>` 로 쓴다. 예를 들어 조건 (나)의 의미는 `link_unlinked(name binding: no entry: <호출>)` 다. 근거에는 `named_calls` 의 그 줄(파일 이름, 줄 번호)을 붙인다. 판정 부분은 규격(ISO C·POSIX)의 글을 읽지 않고 근거로 쓰지 않는다. 보고가 그 조항을 이름 붙인 전제로 따로 적을 수 있을 뿐이다. 바탕 사유가 어휘 자료에 있는 것이므로 어휘 파일은 바뀌지 않는다(8.10).
   - 출력: `link.name_binding.pairs[].items.na_semantics`·`da_no_name_mapping` 의 상태, 사유, 근거. (c3)(iii)의 `depends_on` 과 사유.
   - fixture(합성 호출 이름): 후보 목록에 없고 기록이 0 인 호출을 이름 댄 줄(`D1`, `D2`, 그 호출의 종류를 빼지 않는 부분 인정에서 모두 `missing`). 지금의 fixture(`tests/test_explain4.py`)는 그 호출을 P0 의 data function 후보로 두었으므로 이 꼴을 시험하지 못한다(`rules-04`). 기록이 있고 미대조가 아닌 호출(성립). 상태를 읽지 못한 호출(성립하지 않음). 버퍼 주소 쓰임 줄에 `no entry …` 가 있는 것(모든 갈래와 `Rf` 에서 (다)가 `missing`).

4. **찾은 사용의 이름 상태와 인스턴스 연결 (26.0 (라); `semantics-01`; 판정 부분. 고침: 3절 5·6).**
   - 입력: 사용 사건의 결과 종류, 생성 사건, 세대(지금대로 성공 경로 줄이 `held` 인 생성에서만 생긴다), 행의 `incomplete:` 줄.
   - 규칙(판정 규칙 8.14 의 6 (나)): 결과 종류가 found 인 사용에 그 종류·글의 열린 세대가 있으면 지금대로다(`met`, 또는 빈 자리 지우기 때문의 `undecided`). 열린 세대가 없으면 이름 상태는 `undecided` 다. 행의 생성자 site 가 완전하지 않으면(지금의 생성자 완전성이 거짓) 하위 사유는 `the row's creator sites are not complete` 이고, 아니면 `no create of the found id in the record` 다. 찾은 사용에는 `unmet` 을 쓰지 않는다. 그 앞의 갈래(키 글이 다 기록되지 않음, 종류 값이 없음, 수명 끝이 완전하지 않음, 겹침)는 지금대로 먼저 본다. 인스턴스 연결은 그 id 의 세대가 없으면 `unlinked` 이고, 사유는 지금대로 `link_unlinked(no create of the id in the record)` 다. 찾은 id 로 세대를 만들지 않는다.
   - 출력: `uses[].name_state`, `uses[].instance_link`, `uses.tsv` 의 두 칸.
   - 서식 검사(5절의 14 에 더함): 결과 종류가 found 인 사용 사건의 이름 상태는 `unmet` 이 아니다.
   - fixture: 성공 경로 줄이 `missing` 인 생성과 그 id 를 찾은 사용(→ `undecided`, `unlinked`). 생성자 site 가 완전하고 그 id 의 생성이 기록에 없는 찾은 사용(→ `undecided(no create of the found id in the record)`, `unmet` 이 아님). 성공 경로 줄이 `held` 이고 열린 세대 위에서 찾은 사용(→ `met`, `linked`; 지금대로).

5. **요소와 조건 (다)가 기대는 목록 항목 (26.0 (사); `rules-05`; 판정 부분. 고침: 3절 8 과 3절 3 의 (다)).**
   - 입력: relations 행의 `layers:` 줄(그 행의 `P0-unverified(…)`·`sem-unverified(<api>:<칸>, …)` 목록), 요소마다의 출처 줄(기록의 `sources` 가 가리키는 줄), 버퍼 주소 쓰임 줄, 후보 목록, 평가 갈래.
   - 규칙(판정 규칙 8.13 의 2번 끝 문장; 8.14 의 6 (다)): (c1), (c2), (c4), (c3)(i)마다, 그 요소의 출처 줄이 글로 이름 댄 함수 가운데 그 행의 `layers:` 목록에 든 (층, 함수, 칸)을 모은다. 그것을 후보 목록에서 찾아 `depends_on` 에 적는다. 없으면 빈 목록이고 그것이 '없음'이다. (c3)(iii)은 지금대로 대안 줄의 `rests on:` 에서 읽는다. 조건 (다)는 버퍼 주소 쓰임 가운데 기록이 미대조인 쓰임(`… unverified …`)의 호출을 후보 목록에서 찾아 `depends_on` 에 적는다. 항목이 없는 쓰임은 3 대로 다룬다. 그다음 갈래마다 `entry_state` 를 건다. `D1` 과 그 줄을 인정하지 않는 `D2[…]` 에서, 인정되지 않은 줄에 기댄 요소는 present 나 `held` 가 아니다. 하위 사유는 그 요소의 바탕 사유 아래 `rests on an unverified entry: <함수>:<칸>` 이다.
   - 한계 [I]: 출처 줄이 이름 댄 함수와 행의 목록 항목은 글의 함수 이름으로 맞춘다. 그래서 요소가 실제로는 기대지 않는 항목을 이름으로 붙일 수 있다. 그 잘못은 `D1` 을 더 엄격하게 하는 쪽이다. relations 의 줄이 항목을 줄마다 '기댐'으로 적게 되면 그 줄로 바꾼다(다음 kz-static 빌드; 26.0 (하)와 같은 자리).
   - 이 주기 [I]: 이 규칙으로 `use_cond.status` 와 `violation` 은 바뀌지 않으리라 본다(26.0 (사)). 요소 하나의 상태는 바뀔 수 있다. 예를 들어 읽기 `Rd` 에서 present 인 (c4)는, 그 출처 줄이 이름 댄 생성 함수의 `agreement` 항목이 그 행의 목록에 있으면 `D1-Rd` 에서 present 가 아니게 된다. 바뀌면 다시 쓰는 기대(12)에 규칙에서 적는다.
   - 출력: `use_cond.callee_check.depends_on`, `use_cond.reach.depends_on`, `use_cond.writers.depends_on`, `use_cond.branch_i.depends_on`, `link.name_binding.pairs[].items.da_no_name_mapping.depends_on`.
   - fixture(합성 행): 요소의 출처 줄이 행의 목록 항목을 이름 대는 행(`D1` 에서 present 가 아님, 그 종류를 인정한 `D2[…]` 에서 present). 이름 대지 않는 행(`depends_on` 이 빈 목록). 버퍼 쓰임의 기록이 미대조인 것과 항목이 없는 것.

6. **서식 검사 16 의 넓힘 (26.0 (사); 판정 부분. 고침: 5절의 16).** 지금의 16 에 넷을 더한다.
   - (a) 모든 기록에서 (c1), (c2), (c4), (c3)(i), 문맥마다의 (c3)(iii), 쌍마다의 조건 (다)에 `depends_on` 칸이 있다. 빈 목록도 칸이다.
   - (b) `D1` 기록에서 `depends_on` 이 비지 않은 (c1), (c2), (c4)는 present 가 아니고, 그런 (c3)(i)과 조건 (다)는 `held` 가 아니다.
   - (c) `D2[…]` 기록에서 present 나 `held` 인 것의 `depends_on` 줄은 모두 그 갈래의 인정 파일에 있다(검사 15 의 "미대조 항목에 기대어 선 항목은 인정 파일의 줄을 가리킨다").
   - (d) 하위 사유가 `no entry: …` 인 항목과 요소는 어느 갈래에서도 present 나 `held` 가 아니다.
   - 읽기가 받는 전제의 다룸(10.7-1)은 그대로다. 어긋난 기록은 `refused.txt` 에만 적고 rc 4 다.
   - fixture: (a)–(d)를 하나씩 깨는 기록. 모두 물려야 한다.

### 11.2 요약, 기대, 반증·평가, 실행

7. **주·보조 표지 (26.0 (자); 요약).** 요약은 갈래마다 표의 머리에 `main (declared before the boot: D1/R0)` 또는 `auxiliary (data <자료 갈래>, readings <읽기들>)` 를 적는다. `D2/R0` 도 `auxiliary` 다. 갈래는 기록의 `eval_branch` 에서 읽는다. 주 결과의 갈래 id 는 상수 `D1-R0` 하나다(평가 절차 14.1; 행이나 키가 아니다). 판정 부분이 기록에 쓴 `eval_branch.tag`(읽기 갈래에만 "4.2 의 읽기 갈래(PI 결정 전)")는 그대로 옮기고 고치지 않는다. 형식 점검: 표마다 머리 줄이 하나 있고, `main` 은 `D1-R0` 의 표에만 있다.

8. **사용 단위의 표 (요약).** 평가 절차 14.2 의 단위로 낸다.
   - 입력: 기록의 `uses`, `uses_reached`, `uses_decided`(10.7-4), 갈래마다의 `uses.tsv`, 그리고 요약의 새 인자로 받는 미리 정한 사용의 목록(분모; `uses/uses_<빌드>.tsv`).
   - 출력: 부팅마다 닿은 사용 / 정한 사용, 판정된 사용 / 닿은 사용(미확정은 같은 줄에 10.7-8 의 종류별로), 사용마다 부팅에 걸친 분포("N 부팅 가운데 observed k, not observed m, undetermined n"). 사용마다의 사건 수는 따로 둔 칸이다. 한 기록에 든 사용들이 판정 하나를 나눠 가짐을 표 아래에 적고, 그런 기록을 이름 댄다.
   - 형식 점검: 사용 목록의 줄마다 표에 줄이 하나 있다. 닿지 않은 사용도 줄이 있다.

9. **쌍의 표와 실패 징후 (요약).** 평가 절차 14.4 대로 낸다.
   - `pair_kind` 의 여덟 값(`app-app`, `core-app`, `process-app`, `same-app-other`, `same-app-main-child`, `same_actor`, `core-core`, `unknown`)을 모두 따로 둔다. 3절 11 의 `creator-app` 은 10.5 대로 내지 않는다.
   - `unknown` 은 행위자의 소속 상태(기록의 소비 행위자와 게시 행위자의 `app` 칸의 `status`·`class`)로 (a) 소유 미상, (b) 두 소유가 정해진 코어–프로세스 쌍, (c) 그 밖으로 나눈다.
   - '앱 사이' 줄은 두 소유가 정해진 `app-app` 만이다. 한 앱 안의 쌍과 `same_actor` 는 '대조' 줄에 둔다. 코어와 프로세스, 소유 미상의 쌍은 제 줄에 둔다.
   - `odf = failure_sign` 인 기록의 수를 그 기록의 `pair_kind` 집합마다, 그리고 소비 행위자의 소속(app, core, process, 미상)마다 나눈다.
   - 범례: 정적 `unordered` 는 "이 개정이 간선을 찾지 못함"이다.
   - 형식 점검: 여덟 값의 줄이 모두 있다(0 이어도 있다).

10. **(ii).** 3절 8 의 "(i)·(iii)(또는 (ii))"는 판정 규칙 8.14 의 2 로 읽는다. (ii)는 (i)·(iii) 짝을 대신하는 따로의 근거다(lead 의 선택; PI 의 확인을 기다림). 이 개정은 (ii)의 입력 줄, 칸, 계산을 더하지 않는다.

11. **요약의 때 (26.0 (자). 고침: 3절 13 과 8절 끝의 '부팅 뒤에 고치면').** 7–9 를 넣은 요약을 이번 동결에 함께 넣고, 그 sha256 을 새 동결 파일과 새 부팅 선언에 적는다. 지금의 `17c3a9eb…` 를 대신한다. 부팅 뒤에 요약을 고쳐 다시 내는 길(평가 절차 12절 6)은 이 주기에 쓰지 않는다. 요약은 판독지와 기대 파일을 읽지 않는다(평가 절차 9절). fixture: 7–9 의 형식 점검을 하나씩 깨는 합성 입력.

12. **기대의 다시 씀과 줄마다의 가름 (26.0 (나)·(다)·(라)·(바); 기대 쪽. 고침: 10.6 의 꼴).**
    - 기대 파일과 예측은 판정 규칙 8.13–8.14 에서 다시 쓴다. 예행의 출력에서 베끼지 않는다(26.0 (라)). 고친 판정 부분을 실제 두 계획 위의 합성 사용 사건(16)에 돌린 예행과 맞대어, 어긋나는 줄마다 판정 부분의 결함인지 기대의 잘못인지를 가려 결과 문서에 적는다.
    - `kz-expect 1` 의 `expect` 줄(10.6)에 여덟째 칸 `class` 를 더한다. 값은 `boot`(부팅으로 틀릴 수 있음), `static`, `format` 가운데 하나다(평가 절차 14.6). 기대를 쓰는 스크립트가 칸마다 규칙으로 정한다. 동결한 판정 부분이 정적 입력만으로 내는 칸(정적 요소 표에서 오는 칸)은 `static` 이고, 사건에서 오는 칸은 `boot` 다. 머리에 `rule` 줄을 하나 더해 그 규칙을 적는다.
    - `explain_expect.py` 는 줄마다 `match`·`mismatch`·`no_record` 를 `class` 와 함께 쓰고, 꼬리에 `class` 마다의 수를 낸다.
    - 형식 점검: 줄마다 칸이 여덟이고 `class` 가 셋 가운데 하나다. 꼬리의 수가 맞다.
    - fixture: `class` 가 없는 줄, 셋 밖의 값. 모두 물려야 한다.

13. **반증·평가 스크립트 (26.0 (마); `rules-02`; 새 스크립트).** 이름은 `env/obs1/explain_eval.py` 로 한다. 표준 라이브러리만 쓰고 판정 부분과 읽는 부분을 가져오지 않는다.
    - **입력**(명령줄에 이름 붙은 파일만 열고 연 파일마다 sha256 을 출력의 머리에 적는다): 한 부팅의 설명 출력 디렉터리(갈래마다 `records/*.json`, `index.tsv`, `uses.tsv`), 그 부팅의 문 결과(`gate.txt`), 쌍 파일, 계획 sidecar, relations 글, 미리 정한 사용의 목록, 판독지의 정적 부분과 부팅별 부분(없으면 `none`), 그리고 F1 에만 기대 파일이다. 이 가운데 relations 글과 문 결과 둘은 평가 절차 14.7 의 입력 목록에 이 절이 더하는 것이다. relations 글은 두 가지에만 읽는다. 하나는 F3 의 생성 성공 값이다(`callee-check (success path)` 줄이 적은 값; 줄이 `missing` 이어도 값을 적는다. 기록은 성공 경로 줄이 `held` 가 아닌 생성의 성공 값을 싣지 않는다). 다른 하나는 F5·F8 (a)의 영역과 `expect_discriminate` 가 요구하는 근거(대안 줄과 holder 줄)다. 요구를 판정 부분의 출력에서 읽으면 요구와 충족이 같은 것이 되기 때문이다. 문 결과는 F10 이 '문을 지난 부팅'을 가르는 데만 읽는다.
    - **하지 않는 것.** 기록을 쓰거나 바꾸지 않는다. 입력 파일을 바꾸지 않는다. 그 출력은 Kairos 의 어느 부분의 입력도 아니다. 기대 파일은 F1 의 계산에서만 연다.
    - **출력** `kz-eval 1`(부팅마다 한 파일): 머리(입력과 sha256), 그리고 줄마다 `check`, `class`(`boot`·`static`·`format`; 평가 절차 14.6), `branch`, `build`, `row_label`, `row_key`, `use`(또는 `*`), `result`, `ground`(기록 파일과 칸 경로, 쌍 파일의 줄, 판독지의 줄), `note`. `result` 는 점검이면 `pass`·`fail`·`not_evaluable`, `expect_discriminate` 면 `met`·`unmet`·`not_counted`, 수치면 값이나 `undefined` 다. 꼬리에는 점검 × `class` × `result` 의 수를 낸다. `expect_safe` 는 `boot` 인 점검의 지남 / `boot` 인 점검으로 내고, `static`·`format` 은 따로 낸다. `expect_discriminate` 는 충족 / 센 행, 정밀도와 계획 안의 재현율(14.3 의 꼴), 미확정의 분해도 꼬리에 낸다.
    - **점검마다의 셈**(평가 절차 6–7절과 14.6–14.7; 행을 이름으로 고르지 않는다):
      - F1: 기대 파일의 줄마다 기록의 칸을 대조한다(`explain_expect.py` 와 같은 규칙을 따로 구현). 결과를 줄의 `class` 와 함께 쓴다.
      - F2: `D1` 갈래마다, (a) `bound` 인 쌍이 없음, (b) `depends_on` 이나 `premises` 를 가진 `held` 항목이 없고 인정되지 않은 `depends_on` 으로 present·`held` 인 요소가 없음.
      - F3: 경로 A 기록의 부재 사용 사건마다, 같은 종류 값과 같은 완전한 키 글을 가진 생성 사건이 그 사용의 at-call 앞에 after-call 을 마쳤고, 그 상태 값이 relations 의 성공 값과 같으며, 그 사이에 그 이름·id·표 전체에 대한 수명 끝(기록의 `lifetime_ends`)이 없으면 `fail` 이다. 성공 값을 읽을 수 없는 생성의 쌍은 `not_evaluable` 이다.
      - F4: 결과가 found 인 사용 사건마다, 기록의 생성 사건 가운데 기록된 out 값(`creates[].out` 의 값이나 `raw`)이 찾은 id 와 같고 사용보다 앞선 것이 있으면 `pass` 다.
      - F5: 대조 행(아래)마다, relations 의 대안 줄이 `found: …` 인 문맥의 (c3)(iii)이 `held` 가 아니고, holder 줄이 다른 정의를 적으면 (c3)(i)이 `held` 가 아니다.
      - F6: 대안 줄의 `decided calls:` 가 정한 문맥이 없으면 `static` 의 '점검할 것 없음'이다. 있으면, 정해진 결과가 들어가지 않을 분기 안의 site 를 계획이 이름 대지 않는 동안 `not_evaluable` 이다(한계).
      - F7: `R0` 갈래마다, 미리 정한 사용의 목록의 모든 행에서 `use_cond.status` 가 `derived` 가 아니고 `violation` 이 `observed` 가 아니다. 목록은 설계 행을 담으므로 설계 행을 이름으로 고르지 않는다.
      - F8: (a) 계획된 행마다, relations 행이 이름 댄 모든 생성자와 writer 의 위치에 쌍 파일의 줄이 있다. 글 출력(`order.txt`)과 값을 대는 쪽은 이 스크립트의 입력이 아니어서 셈하지 않고, 그렇게 적는다. (b) 기록의 `order_ground` 가 쌍 파일의 줄을 가리키면 그 줄이 쌍 파일에 같은 값으로 있다. 그런 줄이 없으면 `not_evaluable` 이다.
      - F9: sidecar 에 `context` 줄이 둘 이상인 행의 사용 사건마다 문맥이 `unknown` 이 아니다.
      - F10: `gate.txt` 가 PASS 인 부팅에서, 기록의 객체마다의 같은 빌드 줄이 모두 `confirmed` 다.
      - s1: 대조 행의 `D1` 기록에 `bound` 쌍이 없다. s2: 대조 행에 `held` 인 (c3) 갈래((i)이나 어느 문맥의 (iii))가 없다. s3: `linked` 인 인스턴스 연결마다, 그 id 를 out 으로 적은 생성 사건이 사용 앞에 있고 그 사이에 수명 끝이 없다. s4: `refuted` 인 쌍마다 기초 1, 2, 4, 5 와 조건 (가)–(마)가 `held` 이고 두 키 글이 완전하며 다르다.
      - 대조 행: 미리 정한 사용의 목록에서 `plan_state` 가 대조를 담는 행(`control`, `tier+control`)이다. s1–s4 는 모든 대조 행에서 센다. `expect_discriminate` 는 `plan_state = control` 인 행에서만 센다. `tier+control` 인 행의 줄은 `not_counted` 로 쓴다(평가 절차 14.7).
      - `expect_discriminate`: 평가 절차 14.7 의 규칙 (1)–(3)으로 relations 와 사건에서 요구를 정하고, 기록에서 그 근거가 나왔는지 본다. 그 밖에 나온 근거(`refuted_link`, `not_observed_in_runs`)는 `note` 에 적는다.
      - 정밀도와 계획 안의 재현율: 평가 절차 14.3 대로 셈한다. 판독은 판독지 부팅별 부분의 칸에서 읽는다. 그 칸의 꼴(사용마다 `T`·`F`·`U` 와 양성·음성·판독 불가)은 판독자를 정할 때 파일 목록과 함께 커밋한다(평가 절차 14.5). 그 전, 곧 정적 부분이 모두 'not filled: no reader' 이면 두 수치는 `undefined(no reader)` 다. 도구 양성이 없으면 정밀도는 `undefined(no tool positive)` 다.
      - 미확정의 분해: 미리 정한 사용마다 그 기록이 `undetermined` 이면 바탕 사유를 10.7-8 의 종류로 나눠 센다. 명시적 음성과 섞지 않는다.
    - **형식 점검**(스크립트 자신이 끝에 하고, 어긋나면 rc 0 이 아님): 위의 점검마다 적용할 단위에 줄이 하나씩 있다(조용히 빠지는 단위가 없다). `class` 와 `result` 의 값이 정한 집합 안에 있다. 꼬리의 수가 줄 수와 같다.
    - **fixture**(합성 이름): F1–F10, s1–s4 마다 그것을 `fail` 로 만드는 입력 하나와 `pass` 하나. `expect_discriminate` 의 충족, 미달, `not_counted`. 정밀도의 `undefined`(N = 0), `undefined`(T + F = 0), `U` 가 있어 확정하지 않음, `T`·`F` 가 있는 값. 재현율의 0/k, `undefined`(k = 0), 미확정 옆 칸. 그리고 이 스크립트가 F1 밖에서 기대 파일을 열면 실패하는 시험(판정 부분의 열기 시험과 같은 꼴), 출력 밖의 파일을 쓰면 실패하는 시험.

14. **부팅 전 다시 재기 (26.0 (아); `builds-04`; 실행 스크립트).** `run_cycle.sh verify <빌드> <N>` 단계를 더한다.
    - 입력: 새 `boot/declaration.tsv`.
    - 출력: `boot/<빌드>/boot<N>/verify.txt`, 꼴 `kz-verify 1`. 머리에 선언 목록의 경로와 sha256, 잰 시각을 적는다. 줄마다 (목록의 줄 번호, 종류, 경로, 선언의 값, 잰 값, `ok`|`differs`|`missing`)를 적는다. 꼬리는 `rows <n> ok <a> differs <b> missing <c>` 다.
    - 규칙: sha256 줄은 파일을 다시 해시한다. build-id 줄은 선언을 쓸 때와 같은 방법으로 ELF 의 GNU build-id 를 다시 읽는다. rc 는 `differs` 와 `missing` 이 0 이면 0, 그 밖은 1, 인자 오류는 64 다.
    - `boot` 단계는 같은 N 의 `verify.txt` 가 있고, 그 꼬리가 깨끗하며, 그 머리의 선언 sha256 이 지금 목록의 sha256 과 같을 때만 돈다. 아니면 rc 1 로 멈추고 부팅하지 않는다.
    - 선언에 더하는 줄: 설명 단계가 읽는 어휘 파일 `kairos/tests/static/data/reason_vocabulary.txt`, 두 sidecar 의 `input namespace-data` 줄이 이름 댄 이름 공간 자료 다섯, 2 의 `named_calls.tsv` 와 그것을 만든 스크립트, 13 의 스크립트, 새 동결 파일.
    - 시험: 선언의 줄 하나를 바꾼 복사본에서 `verify` 가 rc 1 과 `differs` 를 낸다. 파일 하나를 뺀 것에서 `missing` 을 낸다. 깨끗하지 않은 `verify.txt` 앞에서 `boot` 단계가 부팅 명령을 부르지 않고 멈춘다(부팅 명령을 대신하는 모의로 시험한다).

15. **실행 스크립트의 단계와 명령 (26.0 (아)·(마); 실행 스크립트).**
    - 빌드마다 부팅 N 의 단계는 `verify` → `boot` → `control` → `convert` → `kzt-text` → `chain` → `stop` → `gate` → `explain` → `eval`(13) → `expect` 다. 끝에 `summary`(11; 미리 정한 사용의 목록을 인자로 받음)를 둔다.
    - `boot/commands.txt` 를 이 차례로 다시 쓴다. `verify` 가 빌드마다의 첫 명령이다.
    - `eval` 단계의 출력은 `boot/<빌드>/boot<N>/eval.txt` 다. 작은 글이므로 기록 디렉터리에 둔다.
    - 기대 파일이 어느 단계의 입력도 아님을 점검한 기록(`boot/no_expect_input_check.txt`)은 다시 만든다. 예외 둘(`expect` 단계, 그리고 `eval` 단계의 F1)을 이름 대어 적는다.
    - `bash -n` 구문 점검을 한다.

16. **fixture, 회귀, 예행, 동결 (26.0 (나)).**
    - fixture: 지금의 127 과 위 항목들의 새 fixture 를 모두 돌린다. 수는 남긴 출력에서 센다.
    - 회귀: 2026-11-16 의 입력에 고친 판을 돌리고, 동결한 판(`531a6b3e…`)의 출력과 견준다. 다른 칸은 위 항목이 바꾼 칸뿐이어야 하고, 그 차이를 칸마다 센다(7절의 꼴).
    - 예행: 두 C1 빌드의 실제 정적 출력과 계획 위에 합성 사용 사건을 넣어 돌린다. 합성 사건은 계획의 site 와 결합 파일에서 만들고, 시작 스크립트의 차례와 그 반대의 두 경우를 둔다. 사례의 값(행, 키, 기대)을 생성기의 인자로 쓰지 않는다.
    - 출력을 쓰는 실행은 모두 `space_guard.sh` 아래에서 돌리고 그 기록을 남긴다(평가 절차 14.9).
    - 그 뒤 다시 동결한다. 읽는 부분, 판정 부분, 구동부, 갈래 생성, 요약, 기대 대조, 반증·평가 스크립트, fixture 파일의 sha256 을 새 동결 파일에 적는다. 그 동결 파일과 12 의 기대 파일이 새 부팅 선언에 든다(평가 절차 14.0).

### 11.3 바뀌는 파일과 sha256

| 파일 | 바꾸는 항목 | 결과 |
|---|---|---|
| `env/obs1/explain_judge.py` (판정 부분) | 1, 3, 4, 5, 6 | sha256 이 바뀐다(`531a6b3e…` 를 대신함). 다시 동결한다 |
| `env/obs1/explain_read.py` (읽는 부분) | 2 | sha256 이 바뀐다(`ff0a0625…` 를 대신함) |
| `env/obs1/explain.py` (구동부) | 2 의 인자를 넘김 | 바뀌면 다시 적는다(`af9d4395…`) |
| `env/obs1/explain_summary.py` | 7–9, 11 | `17c3a9eb…` 를 대신한다 |
| `env/obs1/explain_expect.py`, `records/closure-c1prep-20261119/scripts/expect_write.py` | 12 | 다시 적는다 |
| `env/obs1/explain_eval.py` (새로) | 13 | 새로 적는다 |
| `records/closure-c1prep-20261119/candidates/named_calls.tsv` 와 그것을 만든 스크립트 (새로) | 2 | 새로 적는다. 후보 목록과 인정 파일은 바이트가 같다 |
| `records/closure-c1prep-20261119/boot/run_cycle.sh`, `commands.txt`, `declaration.tsv` | 14, 15 | 새 부팅 선언 |
| `env/obs1/tests/test_explain4.py` 와 새 시험 파일 | 1–16 | 다시 적는다 |

바뀌지 않는 것: `env/obs1/explain_branches.py`, 후보 목록, 인정 파일, 갈래 파일, 미리 정한 사용의 목록, 판독지, 계획, 파생 빌드, 판정 부분이 읽는 어휘 파일(26.0 (너)).

### 11.4 코드를 쓰며 정한 것 (2026-11-19; 코드 커밋보다 먼저 커밋함)

11.1–11.3 을 커밋한 뒤 코드를 짜고 합성 fixture 와 2026-11-16 입력, 두 C1 계획에서 scratch 로 돌리며 정한 것이다. 11.1–11.3 의 뜻은 바꾸지 않는다. 판정 규칙의 글도 바꾸지 않는다. 결과는 결과 문서 26.3 에 적는다(26.1·26.2 는 앞 단계가 썼다).

1. **이름 붙은 호출 파일(2).** 만드는 스크립트는 따로 둔다(`records/closure-c1prep-20261119/scripts/named_calls.py`). `candidates.py` 는 바꾸지 않는다. 입력은 `candidates.py` 와 같다(두 계획 디렉터리, 각 계획을 만든 facts). `records` 는 facts 의 `P0_TABLE` 기록 가운데 `name=` 이 그 함수인 것과 `SEMANTICS` 기록 가운데 `api=` 가 그 함수인 것(모든 칸)의 수다. `layers` 는 `P0_TABLE` 이면 `P0`, `SEMANTICS` 이면 그 `layer=` 다. `namespace-layer` 줄은 계획된 행의 `namespace-entry` 줄과 `data=` 가 같은 줄만 본다. 읽는 부분의 넷째 점검도 같은 줄들을 본다. 그 줄을 다시 만들어 후보 목록과 인정 파일 64 개가 바이트까지 같음을 보인다.
2. **판정 부분.**
   - 버퍼 주소 쓰임(3, 5): `library: <f> parameter <k>: no entry says the parameter is not written` 인 쓰임이 '항목 없음'이다. `library: <f> parameter <k>: <칸> unverified` 인 쓰임이 미대조이고, 후보 목록에서 (`P0`, f, 칸)의 줄을 찾는다. 그 줄이 없으면 그 함수의 모든 칸의 줄을 찾는다. 사유는 '항목 없음'이 먼저다(`link_unlinked(name binding: no entry: <호출>, …)`). 그다음 다른 까닭이고, 그것도 없을 때만 미대조의 미인정이다(`link_unlinked(name binding: rests on an unverified entry: <f>:<칸>, …)`).
   - 요소의 `depends_on`(5): 요소의 출처 줄은 그 요소의 `sources` 가 가리키는 relations 줄이다. 함수 이름은 단어 단위로 맞춘다. 후보 목록에 없는 목록 항목은 더하지 않는다. present 가 아니게 된 (c1)·(c2)의 사유는 `absent_result_unevaluated(rests on an unverified entry: <f>:<칸>)`, (c4)는 `writer_set_incomplete(…)` 다. `held` 가 아니게 된 (c3)(i)는 `not_established` 이고, 그 `why` 는 `rests on an unverified entry: <f>:<칸>; ` 로 시작한다.
   - 기록에 칸 넷을 더한다. `data_branch.admitted_kinds`(검사 16 (c)를 기록만으로 할 수 있게), `data_branch.named_calls_sha256`, `events.lifetime_ends[].t_ms`(반증 스크립트가 사건의 `t_ms` 와 같은 단위로 견주려고), 그리고 `branch_iii` 의 모든 항목(문맥을 모름, 대안 줄 없음 포함)의 `depends_on` 이다. 이름 붙은 호출의 근거는 `{"file": [파일 이름, 줄 번호], "text": …}` 다.
   - 검사 16 (c)는 기록의 `admitted_kinds` 와 견준다. 판정 부분 안에서 돌 때는 그 갈래 인정 파일의 줄과도 견준다. (d)는 사유, `why`, 글로 된 근거에 `no entry: ` 가 든 것을 본다.
3. **요약(7–9, 11).** 새 인자는 `--uses FILE` 이다(반드시 줌, 여럿 가능). 형식 점검만 하는 `--check-summary FILE` 도 둔다(어긋나면 rc 3). 표는 `table <갈래>` 줄로 열고 다음 줄이 `  head: …` 다. 사용 행은 `    use\t…`, 쌍 종류 줄은 `    pair_kind\t<값>\t<수>\t<무리>` 로 쓴다. 미리 정한 사용은 기록과 빌드, 행 키, 소비 뿌리('뿌리 (종류)'), 사용 site(at-call 이나 after-call), 문맥으로 맞춘다. 목록의 문맥 `-` 는 그 기록의 모든 문맥과 맞는다.
4. **기대 대조(12).** 꼬리에 `# by class:` 줄을 더한다.
5. **반증·평가 스크립트(13).**
   - 인자: `--build`, `--boot`(이름표)를 더한다. 문 결과가 없으면 `--gate none`, 판독지 부팅별 부분이 없으면 `--sheets-boot none` 이다.
   - 판독지 부팅별 부분의 꼴(이 스크립트가 읽는 꼴; 판독자를 정할 때 커밋하는 꼴이 다르면 그 전에 이 스크립트를 고쳐 다시 동결한다): `kz-sheet-boot 1`, 칸 `build`, `row_key`, `consumer_root`, `context_id`, `reading`(`T`·`F`·`U`), `label`(`positive`·`negative`·`unreadable`).
   - 줄마다의 가름(평가 절차 14.6 의 가름 규칙을 줄마다 건다): F1 은 기대 줄의 `class` 다. F2 (a), s1, s3, s4 는 그 행의 성공 경로 줄이 하나라도 `held` 이면 `boot`, 아니면 `static` 이다(세대, `bound`, `refuted` 가 생길 수 없다). F2 (b)와 F7 은 `format` 이다. F5, F6, F8 (a), s2 는 `static` 이다. F3, F4, F8 (b), F9, F10 은 `boot` 다. `expect_discriminate` 는 (3)의 요구가 있고 그 행의 성공 경로 줄이 `held` 일 때만 `boot` 이고, 아니면 `static` 이다.
   - 단위가 없는 기록(부재 사용이 없는 기록의 F3, 찾은 사용이 없는 기록의 F4, 사용 사건이 없는 기록의 F9)은 `pass` 로 적고 그 까닭을 `note` 에 적는다. 그 부팅에서 그 점검의 문장이 참이기 때문이다. 문을 지나지 않은 부팅(또는 문 결과가 없는 예행)의 F10 은 `not_evaluable` 이다.
   - F5 와 `expect_discriminate` (2)의 '다른 정의'는 holder 줄의 writer 가운데 own out store 가 아닌 것이다. 읽기 `Rb` 의 갈래에서는 clearing writer 를 뺀다(판정 규칙 8.13 의 5번이 적은 `Rb` 의 뜻).
   - F6 에는 실패하는 입력이 없다. 정한 문맥이 있으면 `not_evaluable`(11 의 13 의 한계), 없으면 '점검할 것 없음'의 `pass` 다. 그래서 F6 의 fixture 는 두 결과를 보이고 실패는 보이지 않는다.
   - 미리 정한 행의 기록이 그 갈래에 없으면 `records` 줄(`format`, `fail`)을 쓴다.
6. **실행 스크립트(14–15).**
   - 시험 이음새 둘: `RUN_CYCLE_OBS`(작업 디렉터리를 바꿈)와 `RUN_CYCLE_BOOT_CMD`(부팅 명령을 바꿈)다. 시험에만 쓰고, 쓰면 그 출력에 적힌다.
   - `verify` 의 경로 풀이: 절대 경로, `research/…`(저장소 뿌리에서), `records/…`(연구 뿌리에서), 그 밖은 작업 디렉터리다. `plan-id` 줄은 같은 빌드의 `plan.side` 줄이 가리키는 파일의 `plan-id` 줄과 견준다. 앞의 `verify.txt` 는 새 이름으로 남긴다.
   - `eval` 단계는 `boot<N>/sheet_boot.tsv` 가 있을 때만 판독지 부팅별 부분을 준다.
   - 시험은 `boot/test_run_cycle.sh` 다.
7. **공간 감시(26.0 (차)).** `below_floor_at_end` 의 rc 는 77 이다(명령의 rc 는 끝 줄의 `rc=` 에 남는다). 끝 값을 읽지 못하면 0 으로 센다. 등록부 `records/TOOLS.tsv` 에는 줄 둘을 둔다. 하나는 지금 판(`in-place`)이고, 다른 하나는 앞 판 `730bcc06…` 이다. 앞 판은 `gone` 이고, 그 글은 git 에 있다고 적는다.
8. **fixture.** 경로 B 행의 합성 세계에서 경로 A 행의 문맥 줄을 뺀다. 검사 14 에 더한 줄(문맥이 있으면 그 대안 줄이 있음)이 그 세계를 물리는데, 경로 B 행에는 대안 줄이 없기 때문이다.

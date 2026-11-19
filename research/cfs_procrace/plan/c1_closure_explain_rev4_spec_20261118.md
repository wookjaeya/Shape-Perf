# 설명 스크립트 개정 4 의 명세 (2026-11-18; 코드보다 먼저 커밋함)

설명 스크립트(`env/obs1/explain.py`, 지금 개정 3)의 다음 개정이 무엇을 읽고 무엇을 내는지를 항목마다 적는다. 코드는 아직 없다. 이 글이 코드보다 먼저 커밋되고, 코드가 이 글과 달라야 하면 이 글을 먼저 고쳐 커밋한다. 따로 표시하지 않는 한 [보강 제안]이다.

(2026-11-19) 2026-11-18 의 판을 판정 규칙 8.13, 형식(`kairos/docs/static_facts.md` 5x, `kairos/docs/plan_manifest.md` 10절, `kairos/docs/event_semantics.md` 13절), 평가 절차와 맞춰 보고 마무리했다. 더한 것: 후보 목록과 인정 파일의 형식(4.1), 실패한 생성의 되돌림을 이름에 붙이는 규칙의 재진입 조건(3절 5), 필드 저장값 규칙으로 선 갈래의 표지(3절 8), 요약과 기대 대조 스크립트의 동결(3절 13, 8절). 앞판의 뜻은 바꾸지 않았다.

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

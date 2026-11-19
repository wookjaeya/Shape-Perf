# 설명 스크립트 개정 4 의 명세 (2026-11-18; 코드보다 먼저 커밋함)

설명 스크립트(`env/obs1/explain.py`, 지금 개정 3)의 다음 개정이 무엇을 읽고 무엇을 내는지를 항목마다 적는다. 코드는 아직 없다. 이 글이 코드보다 먼저 커밋되고, 코드가 이 글과 달라야 하면 이 글을 먼저 고쳐 커밋한다. 따로 표시하지 않는 한 [보강 제안]이다.

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
| 5 | 수명과 이름 상태 | 판정 | `creates[]`, `lifetime_ends[]`, sidecar 의 `lifetime-complete` | 세대 = (종류 값, 이름 글, id), 성공한 생성에서 첫 수명 끝까지(8.13-13). 사용마다 이름 상태: `met`(사용의 at-call 앞에 끝난 생성의 세대가 사용 내내 열려 있음), `unmet`(그 종류·글의 세대가 사용 구간 어디에도 열려 있지 않고, 생성자 site 와 수명 끝 site 가 완전함), `undecided(<까닭>)`(겹침, 완전하지 않음, 손실, 종류 값 없음) | `uses[].name_state`, 완전성 깃발 |
| 6 | 인스턴스 연결 | 판정 | `uses[]`, `creates[]`, 5 | 8.13-14: 열린 세대에만. 같은 글의 연결 없는 생성은 `same_text_unbound` 증거 | `uses[].instance_link` |
| 7 | 기록의 연결 | 판정 | 4, 6 | 사용 사건마다의 결과를 검증된 키 글로 묶고 8.12 (ㄹ)로 집계한다. `link.basis` 는 `established` 일 때만 | `link.status`, `link.basis`, `link.missing[]` |
| 8 | 사용 조건 | 판정 | relations 행의 줄: `callee-check`, `flip:`, `absence:`, `holder:`, `alternatives [context …]:`, `creator:`, `feasibility:`; 사용의 문맥 | (c1) 개정 3 그대로. (c2) flip 줄(`Rc` 면 부재 줄). (c3): `branch_i` ∈ {`held`, `not_held(<근거>)`, `not_established(<까닭>)`}, 문맥마다 `branch_iii` ∈ 같은 셋(8.13-15, 16, 17). (c4) 개정 3 그대로(`Rd` 면 그 읽기). `derived` 는 `R0` 에서 (c1), (c2), (c4)와 (i)·(iii)(또는 (ii))이 모두 설 때. 출처는 정적 줄뿐이다. 읽기 갈래는 `--readings` 에 든 것만 건다 | `use_cond.*`, `use_cond.branch_i`, `use_cond.branch_iii[]`, `use_cond.caller_dependence` |
| 9 | 순서 | 판정 | 쌍 파일만 | 연결된 쌍마다: 행 키, 두 위치, 실행 행위자를 root 묶기로 옮긴 두 뿌리로 쌍 파일의 줄을 찾는다. 찾으면 값·간선·전제, 못 찾으면 `order_basis_unknown`. 행 값으로 대신하지 않는다. `order.status` 는 3.2 축 3 그대로(P 가 유도되지 않았으면 `events_only`). `hard_before` 인 쌍에 `unmet` 사용이 있으면 표시하고 따로 센다 | `order.*`, `order_ground` |
| 10 | 판정 | 판정 | 축의 상태 | 3.3 의 표 그대로 | `tool_verdict.*` |
| 11 | 중립 필드 | 판정 | 행위자의 뿌리와 소속(개정 3 의 근거), RUNX 의 프로세스 | `pair_kind` ∈ {`app-app`, `core-app`, `process-app`, `same-app-other`, `same_actor`, `core-core`, `creator-app`, `same-app-main-child`, `unknown`}; `order_ground`; `other_order_seen_in_boots`(요약이 부팅에 걸쳐 채움); 객체마다의 `same_build`; `uses`, `uses_reached`, `uses_decided`; `eval_branch`, 자료 갈래, 인정 파일과 후보 목록의 sha256 | 기록의 머리 필드 |
| 12 | 평가 갈래 | 판정 | 같은 중간 글 | 갈래마다 한 번 돌리고 출력 디렉터리를 갈래마다 따로 둔다. 읽는 부분의 출력은 갈래와 무관하다(한 번 만든 중간 글을 모든 갈래가 읽는다) | 갈래마다의 기록과 `index.tsv` |
| 13 | 요약 | 따로 둔 스크립트 | 모든 부팅·갈래의 `index.tsv` 와 기록 | 평가 절차 8·10절의 표: 판정 × `pair_kind` × 순서 값, 부팅별 분포, `other_order_seen_in_boots`, 미확정의 종류별 수. "탐지" 수와 주장의 말은 없다. 기대와의 대조는 요약도 판정 부분도 하지 않는다: 채운 기대 표를 읽는 비교 스크립트가 따로 하고, 그 스크립트는 기록을 바꾸지 않는다 | 요약 표 |

## 4. 입력의 형식 점검 (읽는 부분)

| 입력 | 머리와 줄의 점검 | 어긋나면 |
|---|---|---|
| relations 글 | `kz-relations 1`, `facts`, `rules`, `tool` 줄(없으면 개정 13 까지의 글로 읽고 도구 대조는 "견줄 수 없음"); 행의 새 줄 종류 다섯과 블록의 둘을 문법대로 읽는다 | 문법에 맞지 않는 줄은 종류별로 세어 `unparsed` 에 나열하고 그 줄에 기대는 항목은 `missing(<줄 종류> not readable)` |
| 쌍 파일 | `kz-order-pairs 1`, `facts`, `tool`, `domain`, `columns`, 꼬리의 수 = 줄 수, 열 13 | 순서 입력 전체를 물린다: 모든 쌍이 `order_basis_unknown(<까닭>)` |
| sidecar | `kz-plan-sidecar 2`, `manifest-revision 4`, `namespace-entry`·`namespace-layer`·`context`·`lifetime`·`lifetime-complete` 줄의 칸 | 그 줄에 기대는 항목이 `missing` |
| 이름 공간 자료 | `semantics-data 2`, `rule` 줄, sha256 = sidecar | 그 파일을 물린다: (가) `missing` |
| dump 글 | `format kzrt-dump 1.<n>` 줄; `buildid` 줄은 1.5 부터; `ev` 줄의 칸 | `buildid` 줄이 없으면 기초 1 이 `missing(same build not confirmed)`; 값을 지어내지 않는다 |
| 연결 점검의 글 | `same_build_object`·`same_build`·`chain` 줄 | 기초 1 이 `missing` |
| 후보 목록, 인정 파일 | 머리, 줄의 일곱 칸, 부분집합 관계 | 자료 갈래를 `D1` 로 낮추지 않는다: 그 실행을 rc 2 로 멈춘다(갈래가 정해지지 않은 평가는 하지 않는다) |

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

## 7. 2026-11-16 의 입력에 대한 회귀

개정 3 을 그 입력에 먼저 다시 돌려 기준으로 삼는다(45 기록 [문서 보고: 판정 규칙 8.3 이 인용한 `records/obs1-20261116-reviewfix/index.tsv`]). 개정 4 를 같은 입력에 돌린다. 그 입력에는 쌍 파일, relations 의 새 줄, 새 sidecar 줄, `buildid` 줄이 없으므로 기대는: 새 항목이 모두 빠진 입력을 이름 붙여 `missing` 이고, 축의 상태는 기준과 같으며, 다른 것은 선언한 필드뿐이다. 이 출력에는 "규칙을 정하기 전에 본 기록"이라는 표지를 단다. 수는 그 단계가 남긴 출력에서 센다.

## 8. 끝 점검과 동결

- fixture 가 모두 통과한다(수는 남긴 출력에서). `--check` 가 서식 검사를 깨는 기록을 물린다.
- kz-static 단계의 실제 정적 출력(두 C1 빌드)과 빈 실행(`no_run`)으로 예행해, 새 줄 종류가 모두 읽히고 `unparsed` 가 0 이거나 나열된다.
- 런타임 단계의 시험 dump(시험 실행 파일과 등록된 모듈 하나의 `buildid` 줄)로 예행한다.
- 그 뒤 두 부분의 sha256 을 부팅 선언에 적는다.

## 9. 열린 것

- 읽는 부분과 판정 부분의 파일 이름, 중간 글의 칸 이름은 코드를 쓰는 단계가 이 글을 고쳐 먼저 커밋한 뒤 확정할 수 있다(뜻은 바꾸지 않는다).
- 적재된 모듈의 `buildid` 줄은 부팅 전까지 실제로 본 적이 없다 [미확인].
- 읽기 갈래 `Rd`((c4)를 partition 의 생성자 집합과 전제로 받는 것)가 정확히 어느 줄들을 받는지는 PI 의 답을 들은 뒤가 아니라 **부팅 전에** 평가 절차의 채운 판에 적는다(받는 줄: 행의 `creator:` 줄들과 그 `incomplete:` 사유, 쌍 파일의 그 쌍의 값).

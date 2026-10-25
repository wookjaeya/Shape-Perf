# KZ 스키마 노트

- 범위: `include/kz/schema/` 의 레코드 정의와 `lib/core` 의 컨테이너 형식. 규범 원문은 `Architecture/spec/SPEC_FINAL.md` §2.3 이고, 이 문서는 그 구현(S0, K0)에서 정한 세부와 **원문과 달라진 곳**을 적는다.
- 표기: **[D]** 설계 결정, **[M]** 이번 구현에서 실행해 확인한 사실, **[I]** 계획이며 아직 확인하지 않음. 값은 단위 시험 값이며 연구 조건이나 실험 결과가 아니다.
- 용어(T1): 판정 대상은 *readiness specification*(준비 상태 명세), 그 Req는 *readiness precondition*, 위반은 *readiness violation*이다. 식별자에는 `rspec` / `rprecondition` 형태를 쓰고 *contract* 를 쓰지 않는다. 원문 SPEC의 `Φ` 는 식별자에서 `profile` 이다.

## 1. 컨테이너 형식 [D]

모든 산출물은 하나의 KZ 컨테이너다. 모든 값은 little-endian이고(다른 바이트 순서의 호스트는 `kz_schema.h` 가 컴파일을 거부한다), 모든 영역은 8바이트 경계에 놓이며, 빈틈(padding)은 0이다.

```
kz_header                 160 바이트
kz_parent[n_parents]      (kind, role, sha256) 순으로 정렬, 중복 없음
kz_section_entry[n]       section type이 엄격하게 증가하는 순서
섹션 본문                  섹션 표의 순서. 각 섹션은 레코드 한 종류의 배열(STRINGS는 바이트 배열)
```

- **artifact hash** = 파일 전체의 SHA-256. 자식은 부모의 artifact hash를 `kz_parent` 로 나열한다(Merkle 방식). 그래서 한 해시가 입력 사슬 전체를 묶는다.
- `header_crc` 는 헤더, 부모 표, 섹션 표를 `header_crc = 0` 으로 보고 계산한 CRC-32이다. 섹션 본문은 섹션 표의 `crc32` 가 따로 지킨다.
- **정규 배치만 읽는다.** 같은 내용은 같은 바이트, 같은 해시다. 여기서 "내용"은 각 섹션의 레코드(와 그 순서), 부모, 헤더의 세 해시다. 시각·호스트 정보는 컨테이너 계층이 쓰지 않는다. 쓰는 쪽은 섹션을 id 순으로, 부모를 정렬해서 쓰므로 호출 순서가 결과에 영향을 주지 않는다. 한 섹션 안에서 레코드의 순서는 생산자가 정한다: 순서가 비결정적인 컨테이너(포인터를 키로 하는 맵 등)를 그대로 순회해 레코드를 만들면 해시가 실행마다 달라진다.
- 문자열은 `STRINGS` 섹션에 NUL로 끝나는 항목으로 들어가고, 레코드는 `kz_str(off, len)` 으로 가리킨다. 빈 문자열은 `(0, 0)` 이다.
- **STRINGS 는 정규 풀이다**: 0바이트 하나, 이어서 레코드가 참조하는 서로 다른 문자열을 바이트순(부호 없는 바이트, 짧은 접두어가 먼저)으로 정렬해 각각 NUL 을 붙여 쓴 것. 참조되지 않은 문자열도, 중복도, 접미어 공유도 없다. 그래서 문자열을 어떤 순서로 intern 했는지는 파일에 나타나지 않는다. writer 는 `finish()` 에서 풀을 만들고 레코드의 `kz_str` 오프셋을 다시 쓴다(`intern()` 이 돌려주는 `kz_str` 은 임시 값이다). 엄격한 writer 는 풀에서 해석되지 않는 `kz_str` 을 `BadRef` 로 거부한다(`set_check_reserved(false)` 는 읽는 쪽 시험용이다). 읽는 쪽은 `verify_strings()` (`verify_content()` 가 부른다)로 풀이 정규인지와 모든 참조가 정규 오프셋을 가리키는지 확인하고 `NotCanonical` 로 거부한다. 이 파서가 모르는 섹션이 있는 컨테이너(`allow_unknown_sections`)는 보이지 않는 참조가 있을 수 있어 풀을 통째로 비교하지 않는다. [M]
- `kz_ref(artifact_sha256, section, index)` 는 형식이 있는 교차 참조다. **모두 0이면 null**, artifact hash가 0이고 section이 0이 아니면 *이 컨테이너 안의 레코드*, 아니면 부모 artifact의 레코드다. `Container::verify_refs()` 가 이를 검사한다(참조된 artifact는 부모여야 하고, 자기 참조는 범위 안이어야 한다).
- `rsv*` 필드는 정렬용 예약 필드이며 0이어야 한다. 쓰는 쪽(`ContainerWriter::add_section`)은 0이 아닌 예약 필드를 거부하고, 읽는 쪽은 `verify_reserved()` 로 확인한다.
- 읽는 쪽은 한 바이트라도 바뀐 파일, 잘린 파일, 뒤에 바이트가 붙은 파일을 모두 거부한다. 고정 표본 컨테이너의 모든 바이트에 대해 이를 시험한다(`test_core_container_corrupt`). 어떤 CRC 도 덮지 않는 끝 padding 은 "0 이어야 한다"는 규칙만이 막는다: 끝 padding 이 항상 있는 컨테이너 두 개(풀만 있는 것: 7바이트, 60바이트 레코드가 마지막인 것: 4바이트)의 모든 padding 바이트로 이를 시험한다. [M]

## 2. 레코드 정의 방식 [D]

레코드는 `KZ_RECORD(T, LIST)` 한 곳에서만 정의한다. 필드 목록 `LIST` 하나가 다음으로 펼쳐진다.

1. C 구조체 `T`.
2. 구조체와 같은 크기의 *바이트 배열 쌍둥이* `T_kz_bytes`. 쌍둥이는 padding이 있을 수 없으므로, 각 필드의 offset이 쌍둥이의 offset(앞선 필드 크기의 합)과 같아야 한다는 `static assert` 가 필드마다 붙는다.
3. `sizeof(T)` = 필드 크기의 합이라는 `static assert`(끝 padding도 막는다)와 필드마다의 자연 정렬 검사.
4. `lib/core/schema_tables.cpp` 에서 같은 목록이 필드 서술자 표(이름, offset, 크기, 개수, 종류)로 펼쳐진다. 정규 dump, 컨테이너 검사, 시험이 모두 이 표를 따른다. 레코드마다 따로 쓴 코드는 없다.

열거형과 집합(비트 플래그)도 목록 하나(`ITEM(P, id, value, "표시 이름")`)에서 상수와 이름 표가 나온다. 필드에 어떤 열거형을 쓰는지는 `KZ_*_FORMATS` 목록이 정하며 레이아웃에는 영향이 없다. 모든 열거형에서 **값 0은 가장 보수적인 값**이다(예: `cls = unknown`, `reach = assumed`, `progress = unknown / not_checked`, `harm = unspecified_by_reference`, `tier = unreferenced`). 0으로 채운 레코드는 아무것도 확립하지 않는다고 말한다. [D]

판정 레코드의 어휘(harm, consequence, outcome label, shape, class, unknown reason, reach, progress, tier, proof status …)는 SPEC §1.6·§1.10·§6.2 가 정한 낱말이다. `test_core_vocab` 이 SPEC 의 목록을 값·표시 이름·순서까지 한 번 더 적어 두고 비교하므로, golden 을 한 번에 갱신해도 SPEC 과 어긋난 낱말은 남지 않는다. R2 의 harm 은 `event_request_rejected` 이다: EVS 가 거부한 사건 요청은 "rejected" 이지 "lost" 가 아니다(SPEC §0.9 항목 4). 퇴역한 낱말(`event_lost…`, `packets_lost…`)이 어떤 열거형에도 없다는 것도 시험한다. SPEC 이 낱말을 바꾸면 헤더, 이 시험의 표, golden 을 한 커밋에서 같이 고친다. [M]

헤더는 C99(`gcc -std=c99 -pedantic -Wall -Werror`), C11, C++17로 컴파일되고, `clang -target bpf -ffreestanding` 로도 컴파일된다. C99에서는 `_Static_assert` 대신 음수 크기 배열을 쓴다. 이 네 경우와, padding이 있는 레코드가 실제로 거부되는지를 시험한다(`schema_probe_*`, `schema_bpf_compile`, `test_core_schema`). [M]

## 3. 섹션과 레코드

섹션 id는 한 곳에서 배정한다(병렬 작업의 충돌 방지). 정규 순서는 id 오름차순이다. 크기는 바이트.

| 범위 | 헤더(소유) | 섹션 : 레코드 (크기) |
|---|---|---|
| 1–9 | `kz_schema.h` (K0) | STRINGS : kz_byte (1), BASIS : kz_basis_item (28), FILE_HASHES : kz_file_hash (56), PROVENANCE : kz_prov (68), DEVIATIONS : kz_deviation (48), REF_LISTS : kz_ref (40) |
| 10–39 | `kz_facts.h` (K4a, 개정 4; 추출 레코드는 `docs/static_facts.md` §4, skeleton 레코드 23–29 는 초기 정의) | FACTS : kz_fact_call (76), FACT_ARGS : kz_fact_arg (56), DERIVES : kz_fact_derives (68), FACT_EXPRS : kz_fact_expr (32, v3), FACT_LOOPS : kz_fact_loop (120, v3), FACT_INIT : kz_fact_init (64), FACT_CONSTS : kz_fact_const (56), FACT_ATYPE : kz_fact_atype (56, v3), WRITERS : kz_fact_writer (112), ALLOCS : kz_fact_alloc (56), FUNCS : kz_fact_func (48), ANCHORS : kz_anchor (64), SKEL_CERT : kz_skel_node (28), SKEL_CERT_EDGES : kz_skel_edge (16), ONCE : kz_skel_once (48), ACTORS : kz_actor (24), CLOSURE_SPAWNS : kz_closure_spawn (16), CALL_TARGETS : kz_fact_target (24), STATIC_IMAGE : kz_fact_image (56), FACT_LOCS : kz_fact_loc (64, v2), TERMINALS : kz_fact_terminal (48), OUTCALLS : kz_fact_outcall (64), TUS : kz_tu (88), LOOP_ITERS : kz_fact_iter (64), SPAWN_PARAMS : kz_fact_spawn_param (32), FUNC_WRITES : kz_fact_fwrite (40) |
| 40–59 | `kz_raw.h` (K2, 개정 2; 2026-10-16 열거 값 추가, 레이아웃 그대로; 스키마 개정 2 에서 섹션 추가) | SITES : kz_site (96, v2), RAW_EVENTS : kz_raw_ev (64), STACKS : kz_stack_frame (24), LOSS : kz_loss (40, v2), RUNMETA : kz_runmeta (152), PERTURBATION : kz_perturbation (32), SITE_SLOTS : kz_site_slot (20), SIDE_EVENTS : kz_side_ev (40), THREADS : kz_thread (88), MAPS : kz_map_ev (88), SYMBOLS : kz_symbol (40), TARGETS : kz_target (80), ACTOR_OWNERS : kz_app_owner (32; 개정 2) |
| 60–79 | `kz_cert.h` (초기, K4b) | CERTIFICATES : kz_cert (136), CERT_STEPS : kz_cert_step (36), TRUST_LEDGERS : kz_trust_ledger (224), TRUST : kz_trust (60), OBLIGATIONS : kz_obligation (64) |
| 80–99 | `kz_schema.h` (K0; 스키마 개정 2 에서 섹션 추가) | WITNESSES : kz_witness (112), VERDICTS : kz_verdict (112), VERDICT_CELLS : kz_verdict_cell (472), BOUNDS : kz_bound (192), SUMMARIES : kz_summary (44; 개정 2), MODEL_IDS : kz_model_id (44; 개정 2), OBS_ADEQUACY : kz_obs_adequacy (88; 개정 2), IDENTITIES : kz_identity (68; 개정 2) |

컨테이너 자체의 구조 레코드: `kz_header` (160), `kz_parent` (40), `kz_section_entry` (48), `kz_str` (8).

**id만 예약하고 정의하지 않은 SPEC §2.3 섹션**(후속 단계가 필요할 때 소유 헤더에 레코드를 정의한다): TEMPLATES 22, SKEL_EXPL 25, PROVENANCE_MSG 27, RESOURCES 30, EVENTS 42, EDGES 47, INSTANCES 48, HOLDPLANS 49. 이 섹션을 쓰려 하면 writer가 거부한다.

`facts`, `skeleton`, `raw` 헤더의 레코드는 SPEC §4.1–§4.3, §5.5, §4.13 에서 *지금 도출할 수 있는 만큼*만 정의한 초기본이다. 소유 구성요소가 바꿀 수 있다. 바꿀 때는 해당 섹션의 `schema_version` 을 올린다.

## 4. SPEC §2.3 발췌와 다른 곳 [D]

SPEC rev 1.2 를 쓰는 쪽이 반영해야 할 목록이다.

| 항목 | SPEC §2.3 발췌 | 구현 | 이유 |
|---|---|---|---|
| 필드 목록 매크로 | `FIELDS(X)`, `X(type, name, arr)` | `FIELDS(X, T)`, `X(T, type, name, arr)` | offset assertion이 형 이름을 써야 한다 |
| offset 검사 | 필드마다 `_Static_assert` | 바이트 쌍둥이와의 비교 + 자연 정렬 + `sizeof` = 합 | 앞선 필드 크기의 합을 전처리기로 셀 수 없다. 쌍둥이는 같은 효과를 낸다 |
| 가변 길이 묶음 | `KZ_CERT` 뒤에 `nsteps` 개 step, `KZ_VERDICT` 뒤에 `ncells` 개 cell(주석) | `first_step`, `first_cell` 을 추가하고 CERT_STEPS 섹션 신설. 모든 섹션은 레코드 한 종류의 정규 배열 | 섹션을 균일한 배열로 두어야 CRC, 검사, dump가 레코드 종류마다 따로 필요하지 않다 |
| `KZ_VERDICT` | 104 B | 112 B (`first_cell`, `rsv2`) | 위와 같음 |
| `KZ_CERT` | 128 B, `contested_phi`, `proof_status` | 136 B (`first_step`, `rsv1` 추가), `contested_profile`. `proof_status` 는 SPEC 그대로 | 위와 같음. 식별자에 `Φ` 를 쓰지 않는다 |
| `KZ_CERT_STEP` | 32 B | 36 B (`flags`: `at-decision`, `after-decision`, `reject-event`, `rsv0`) | §4.5 badness 단계는 REJECT 사건마다 결정 시점 여부를 기록해야 한다 |
| 증명 상태 필드 | SPEC 현재 본문(D20 반영, 2026-10-15 읽음)이 `KZ_TRUST.proof`, `KZ_CERT.proof_status`, `KZ_VERDICT_CELL.proof_status`(+ `rsv2[3]`, 472 B)를 둔다 | 같은 필드. `KZ_TRUST` 60 B, `KZ_TRUST_LEDGER` 224 B 도 발췌 그대로 | 원장 레코드는 증명 상태 요약을 따로 갖지 않는다: `lemmas:<p>/<d>/<u>` 는 항목에서 센다 |
| 헤더 필드 이름 | B hash, Φ hash | `basis_sha256`, `profile_sha256`, `tool_sha256` | 용어(T1) |
| 추가 섹션 | — | FILE_HASHES, PROVENANCE, DEVIATIONS, REF_LISTS, CERT_STEPS, FACT_*, SKEL_CERT_EDGES, CLOSURE_SPAWNS 등 | 출처 계층(T1)과 조건표의 deviation(CONDITIONS_POLICY)을 기록하려면 레코드가 필요하다 |

## 5. 증명 상태와 진행성 [D]

- **안전성만** 다룬다(D20 = A). 판정 셀의 `progress` 는 기본이 `unknown(not_checked)` 이다. 0으로 채운 셀이 이미 그 값이다. `progress` 가 `unknown` 이면 stall 세부 필드는 모두 비어 있어야 하고, 아니면 `validate_cell` 이 거부한다.
- **안전·활성을 뜻하는 필드는 없다.** 스키마의 필드 이름에 `safe`, `live`, `deadlock_free` 가 없다는 것을 시험한다. 셀을 글로 쓸 때(`render_cell`)는 항상 class와 reach와 progress를 함께 쓴다.
- **lemma의 증명 상태**(SPEC §6.2)는 `unproved` / `depends-on-stated-assumption` / `proved` 이고 `n/a`(lemma 없음)가 따로 있다. 항목은 `kz_trust.proof`, 인증서는 `kz_cert.proof_status`, 판정 셀은 `kz_verdict_cell.proof_status` 에 적는다. 뒤의 둘은 원장의 LEMMA 항목 중 **가장 약한 상태**여야 하고(`validate_cert`, `validate_cell_proof`) 증명 상태가 없는(`n/a`) 인증서 기반 셀은 거부된다(PS3). 상태가 올바르지 않은 LEMMA 항목(범위 밖의 값, `n/a`)은 `unproved` 로 센다(`weakest_proof`, `count_ledger`). 의무를 목록에 올리기만 한 lemma는 `unproved` 이다(`make_trust_entry` 의 기본값, PS2). 실행, 시험 통과, 의무 통과(part iv)는 상태를 올리지 않는다(PS1).
- **`proved` 항목이 가리키는 것**: 상태는 문서(SPEC §6.2 표)의 속성이므로(PS1) `proved` LEMMA 항목의 `source` 는 그 증명이 들어 있는 문서의 기록이다. 코드가 확인하는 것은 두 가지다. (1) `validate_trust_entry`: `source` 가 null 이 아니고 **PROVENANCE 섹션**의 기록을 가리킨다(실행 기록이나 다른 레코드는 안 된다). (2) `validate_proof_texts`: 그 기록이 이 컨테이너에 있으면 존재하고, 인용 경로 또는 요구 id(`file`)가 있고, 본문 해시(`text_sha256`)가 0 이 아니다(부모 artifact 안의 기록은 부모를 가진 쪽이 확인한다). 코드가 확인하지 **못하는** 것: 그 문서가 정말 그 lemma 를 증명하는지. 그것은 저자의 분류이고 검토에서 내려간다(PS1, V0-P).
- 글로 쓸 때(`render_cell`, `render_acceptance`): 인증서 기반 진술 옆에 `lemma status: <상태>` 와 (있으면) `lemmas:<p>/<d>/<u>` 와 증명되지 않은 lemma 의 ident(`not proved: #3, #7`; ident → 이름 표는 checker 쪽)를 쓰고, "accepted by kz proofcheck" 는 항상 "relative to its trusted base" 와 그 상태가 뒤따른다(PS3). 원장 집계를 함께 주면 저장된 상태를 원장의 가장 약한 상태와 대조하고, 어긋나면 `lemma status: INCONSISTENT (the record says …, its ledger says …)` 로 쓰며 `proved` 라고 쓰지 않는다. 원장 없이 `proved` 이면 `proved (not checked against a ledger)` 로 쓴다. "verified", "certified" 라는 말은 쓰지 않으며, 상태가 `proved` 가 아니면 "lemma status: proved" 도 쓰지 않는다(PS2).
- **서술 규칙**(SPEC §1.10) 중 구현한 것 [M]: 보편 class(protected, fixed-order, no-final-status)는 항상 경계 `[b=…]` 와 함께(경계를 못 받으면 `[b=?]`); reach `model(b)`·`unreached(b)` 와 progress `no-stall-within(b)` 에 경계 값; unknown 셀이 가진 진술(`no_decision_route` 의 ¬OK)과 witness(`one_sided`), schedule-dependent·ordinal-split 의 witness; stall witness 의 구간(`pre-use`, `in-use`, `horizon`)·종류(`deadlock-cycle`, `leaked-lock`, `other`)·한정어·harm·실현 방식·`terminal-pending` 의 "relative to the closed cone". **구현하지 못한 것**: stall witness 의 wait-for root **집합**과 witness 의 ordinal 은 셀에 없고 witness 기록에 있으므로, 서술은 그 기록의 참조(`wait-for root: in witness self/WITNESSES#1`)만 쓴다. 기록을 열어 root 를 쓰는 일은 호출자(보고기)의 몫이다.
- `trusted:<n>` 은 part (iii) 중 `DECLARED_TRUSTED` 항목 수, `ledger:<i>/<ii>/<iii>/<iv>` 는 네 부분의 크기(원장 레코드에서), `lemmas:<p>/<d>/<u>` 는 `proved` / `depends-on-stated-assumption` / `unproved` 인 LEMMA 항목 수이다.

## 6. 정규 dump 형식 [D]

한 줄에 레코드 하나. 필드는 선언 순서. 0인 예약 필드는 출력하지 않는다. 정수는 십진수, 집합은 `{a,b}`, 열거형은 표시 이름(목록에 없으면 `#n`), `Hex` 필드는 `0x…`, 인덱스의 전부 1인 값은 `none`, 경계 `h` 의 전부 1인 값은 `inf`, 바이트 배열은 소문자 hex, 참조는 `-` / `self/섹션#인덱스` / `<sha256>/섹션#인덱스`, 문자열은 `"…"`(ASCII만, 나머지는 `\xHH`). 컨테이너 dump는 마지막에 `artifact_sha256=` 를 쓴다. `tools/kz-dump` 가 같은 출력을 낸다.

## 7. 레이아웃을 바꿀 때

1. 헤더의 필드 목록을 고친다. 컴파일러가 padding이나 어긋난 offset을 거부한다.
2. 그 섹션의 `schema_version` 을 올린다(`KZ_*_SECTIONS` 목록).
3. `KZ_GOLDEN_UPDATE=1 ctest -L golden` 으로 golden을 다시 만들고 **차이를 읽어 본다**. `layout.golden` 의 마지막 줄 `fingerprint=` 가 바뀐다. 컨테이너 golden(`container_*.golden`)은 고정된 시험용 tool hash 를 쓰므로 레코드 배치가 바뀌어도 그 섹션을 가진 컨테이너의 dump 만 바뀐다: 컨테이너 golden 이 바뀌었다면 컨테이너 형식이나 표본 내용이 바뀐 것이다. 실제 tool hash 가 스키마 fingerprint 에 의존한다는 것은 `test_core_identity` 와 `golden_tool_hash` 가 시험한다.
4. `test_core_schema` 의 `check_known_sizes` 에 적힌 크기를 의도한 값으로 고친다.

## 8. tool hash와 소스 identity [D]

헤더의 `tool_sha256`(SPEC §2.3의 tool hash)은 산출물을 쓴 도구(또는 `KZ_TRUST_LEDGER.checker` 가 가리키는, 인증서를 받아들인 `kz proofcheck`)의 빌드를 식별한다. 이전 판은 이름, 버전 라벨, ABI, 스키마 fingerprint 만 해시해서, 쓰기·판정·검사 논리가 달라져도 버전 라벨이 같으면 같은 해시를 냈다. 지금의 정의:

```
tool_sha256 = SHA-256 of the fields, each as "<decimal length>:<bytes>\n":
  "kz-tool/2"                      domain tag
  tool name                        e.g. "kz-dump"
  version                          KZ_TOOL_VERSION (CMake cache variable, a human label)
  ABI version                      KZ_ABI_VERSION
  schema fingerprint               hex SHA-256 of layout_text(): every record, enum, section and format
  core source identity             hex, the sources libkz-core was built from
  tool source identity             hex, the sources of the tool and of every library it links (may be empty)
```

**소스 identity** 는 `cmake/KzSourceIdentity.cmake` 가 configure 때 계산하는 SHA-256이다: 대상 경로 아래 일반 파일 전부(이름이 `.` 로 시작하는 파일·디렉터리와 `~` 로 끝나는 파일은 제외)에 대해 `<소스 루트에 대한 상대 경로> <파일 내용의 SHA-256>\n` 줄을 만들고 경로의 바이트순으로 정렬해 해시한다. 트리를 어디에 체크아웃했는지, 어떤 컴파일러로 빌드하는지에 의존하지 않는다. 계산에 쓴 파일은 모두 `CMAKE_CONFIGURE_DEPENDS` 에 등록되어, 파일이 바뀐 뒤 빌드하면 configure 가 다시 돌고 컴파일 정의가 갱신된다. 이는 컴파일 정의를 파일 내용에서 계산하는 것이고 코드를 생성하지 않는다.

- libkz-core 의 소스 identity: `include/kz/core`, `include/kz/schema`, `lib/core`, `cmake/KzSettings.cmake`, `cmake/KzSourceIdentity.cmake`, 최상위 `CMakeLists.txt` (`KZ_CORE_SOURCE_PATHS`). `identity.cpp` 한 파일에만 컴파일 정의 `KZ_CORE_SOURCE_SHA256` 로 들어간다.
- 도구의 소스 identity: `kz_tool_identity(<target>)` 이 target 과 링크 폐포의 모든 (imported 가 아닌) 라이브러리의 소스 디렉터리와 `kz_declare_sources()` 로 선언된 경로에 더해 `include/`, `cmake/`, 최상위 `CMakeLists.txt` 를 덮는다. 도구에서 `KZ_THIS_TOOL_HASH("이름")` 으로 쓴다. 이 매크로가 쓰는 `KZ_TOOL_SOURCE_SHA256` 을 정의하지 않은 target 은 컴파일되지 않는다.
- **한계**: 파일의 identity 이지 도구 체인의 identity 가 아니다. 같은 소스라도 컴파일러와 옵션이 다르면 다른 바이너리일 수 있다. 컴파일러와 언어 수준은 `kz::build_identity()` 가 BASIS 항목으로 따로 기록한다(두 컴파일러가 같은 tool hash 를 내는 것은 의도다). `docs/`, `tests/`, 프로필 데이터는 소스 identity 에 들어 있지 않다. 프로필은 헤더의 `profile_sha256` 이 따로 식별한다.
- 시험: 입력 하나하나가 해시를 바꾸는지(`test_core_identity`), CMake 가 컴파일해 넣은 libkz-core 의 identity 와 `kz-dump --identity` 의 tool identity 가 지금 트리의 파일로 독립 계산한 값과 같은지(C++ 로 다시 계산), identity 함수 자체의 성질(`source_identity_cmake`), 정의의 고정(`golden_tool_hash`).

## 9. 관측 레코드 (`kz_raw.h`, K2) [D][M]

K0 의 초기 정의에서 K2 가 perf-sample 백엔드를 구현하며 바꾼 것이다. 규범은 헤더의 주석과 필드 목록이고, 아래는 그 요약이다. 바뀐 섹션의 `schema_version` 을 올렸다(SITES 2, LOSS 2).

| 섹션 | 어느 컨테이너 | 내용 |
|---|---|---|
| SITES (v2) | 실행 | 계획의 사이트 하나당 한 레코드. 96 B. `rsv1` 이 `line` 이 되었고 `label`(계획이 붙인 이름)과 `source`(소스 위치)가 더해졌다. 프로브한 파일의 `inode`, `device`, `build_id` 를 가진다. |
| SITE_SLOTS | 실행 | 사이트의 네 값(`a[0..3]`)이 어디서 오는지: 레지스터, 스택 단어, 표본의 명령 포인터. 너비와 `role`(값의 뜻 이름). `generic` 사이트는 `role` 로 찾는다. 정체성 결합의 디코더와 진입·복귀 짝짓기는 **정해진 위치**를 읽고, 계획 검사(`check_site`, `check_plan`)가 그 위치에 요구한 역할이 없는 계획을 거부한다(아래 디코더 의미). TRUNC 사건의 슬롯 값은 열쇠로 쓰지 않는다. |
| RAW_EVENTS | 실행 | 64 B 사건. `tseq` 는 **해석기가 붙인** thread 별 순번이다(병합 순서: 시각, CPU, 도착). 커널이 번호를 붙이지 않으므로 번호의 빈틈은 손실의 증거가 아니다. `objseq` 는 항상 0 이고 `OBJSEQ_VALID` 는 꺼져 있다. |
| SIDE_EVENTS | 실행 | 추적된 프로세스의 FORK/EXIT/COMM(커널 옆 가지 기록). 생성 관계와 커널 thread 이름의 증거. |
| MAPS | 실행 | 대상 파일의 실행 mapping. `bias`(적재 편향)와 `bias_known`: 계획의 사이트 오프셋과 mapping 에서 계산한다. |
| THREADS | 실행 | thread 한 번의 생애마다 한 레코드(`generation` 으로 tid 재사용 구분). 생성자 thread, 알린 id, 이름과 그것이 정해진 시각, 생성자 쪽 사건·첫 사건의 `RAW_EVENTS` 인덱스, 진입 함수 포인터와 기호, 우선순위. `flags` 는 각 사실이 어떤 증거로 확립됐는지(`fork-seen`, `comm-seen`, `entry-seen`, `spawn-matched`, `spawn-ambiguous`, `name-inherited`, `name-after-first-event`, `tid-reused`, `exit-seen`, `entry-unmatched`, 그리고 2026-10-16 에 더한 `match-by-time` 0x400: 생성자 연결이 FORK 시각의 구간 포함 하나에 기댐(한 thread 의 시각, A-CLK-PO), `id-unverified` 0x800: 알린 id 를 생성자 쪽 값과 대조하지 않음 — perf-sample 은 생성 함수의 출력 인자를 읽지 못한다). 플래그 없음 = 확립되지 않음. 인덱스 없음 = `KZ_NONE32`. |
| LOSS (v2) | 실행 | 커널이 알려 준 손실만. `PERF_RECORD_LOST`: `count` 는 커널이 버렸다고 보고한 **기록**의 수이다 — 표본만이 아니라 같은 ring 을 쓰는 모든 기록(옆 가지 기록, 다른 프로세스의 것 포함)이라 프로브 표본 손실의 상한이다. `t_ns` 는 보고가 쓰인 시각이고 `tid`/`site` 는 보고를 쓴 기록의 것이지 버려진 기록의 것이 아니다. `PERF_RECORD_THROTTLE`: `t_ns` 는 구간 시작, `count` 는 길이(ns), 끝나지 않았으면 `tseq_first = 1`. |
| RUNMETA, PERTURBATION | 실행 | 기록의 기준 해시(basis, 계획), 층(호출자가 선언; 확립하지 못했으면 `none`), 백엔드·충실도, 커널, 관측기 CPU 시간, 관측기 수명. 섭동은 사이트별 프로브 횟수(진입·점 사이트는 주소의 명령 부류, 복귀 사이트는 `ret-trampoline`), 관측기 CPU 시간, 수명, 그리고 `proc-capture`(관측기가 대상이 도는 동안 읽은 `/proc` 항목 수와 그 시간). 한 파일 오프셋의 트랩은 그 오프셋의 첫 진입·점 사이트 행에서만 센다(헤더의 계수 규칙; 실행 기록의 `perturbation.site<N>.trap`). 손실이나 해석 오류가 하나라도 있으면 `LOSS_PRESENT`. |
| SYMBOLS | basis | 바이너리의 함수 기호(주소, 크기, 파일 오프셋, 이름, 결합). 반환 주소를 함수 이름으로 바꾸는 데 쓴다. |
| TARGETS | basis | 계획의 사이트마다 해석된 프로브 대상: 링크 시점 주소, 파일 오프셋, 함수 시작과 거리, 줄, 처음 8바이트, 비용 부류. 관측기는 붙이기 전에 이 바이트를 파일에서 다시 확인한다. |

| VIA (57) | basis·실행 | `enter=` 사이트(대상 종류 `entry-reached`)의 호출 경로: 사슬 `F>G[>H]`, 홉 수, 기대 반환 주소(링크 시점) 또는 `ret-dynamic`(모든 홉이 점프: 스택 포인터로 귀속). basis 에서는 `id` = 대상 인덱스, 실행에서는 사이트 id (2026-10-24) |
| SLOT_FETCHES (58) | 실행 | 원천이 `mem`/`str` 인 슬롯의 레지스터와 오프셋(커널의 probe-event fetch argument가 읽은 메모리) (2026-10-24) |
| EVENT_STRINGS (59) | 실행 | `str` 슬롯의 문자열, `RAW_EVENTS` 인덱스와 슬롯으로. 슬롯 값 `a[]` 는 바이트 길이 (2026-10-24) |

- **2026-10-25 추가(PI 결정 P1·P10, 단계 A)**: 열거 값만 — `KZ_SUMMARY_FLAG` 의 0x100–0x8000 여덟 개(판정 근거 셋, 신뢰 가정 셋, P10 관계 둘; SUMMARIES 행). `flags` 는 이미 16비트이므로 레이아웃·섹션 버전은 그대로이고 이전 컨테이너도 읽힌다. `layout.golden`(열거 값 여덟 줄, 지문), `records.golden`(무작위 레코드 두 개의 `flags` 표시 이름이 새 값에 따라 바뀜)을 갱신했다(diff 확인). 재생 출력 컨테이너는 새 섹션 없이 이 플래그를 SUMMARIES 에 싣는다 [M: `test_trace_replay_out`].
- **2026-10-24 추가(R2 잔여)**: 열거 값 `KZ_TARGET_KIND` 의 `at-call` 5 하나(호출 명령 자체의 점 사이트). 레이아웃은 그대로이다. `layout.golden`, `records.golden` 을 갱신했다(diff: 열거 값 한 줄, 지문, 무작위 레코드 두 개의 대상 종류 이름이 열거 수에 따라 바뀜). 재생 출력 컨테이너가 REF_LISTS 를 처음 쓴다(§11).
- **2026-10-24 추가(C1 판정 가능성)**: 섹션 세 개(관측 범위 40–59 의 남은 번호)와 열거 값(`KZ_SLOT_SRC` 의 `mem` 4·`str` 5, `KZ_TARGET_KIND` 의 `entry-reached` 4, 새 `KZ_FETCH_KIND`, `KZ_VIA_FLAG`)만 더했다. 기존 레이아웃과 섹션 버전은 그대로이므로 smoke1 의 실행·basis 컨테이너도 읽힌다. 섹션은 그 사이트가 있는 계획에서만 쓰이므로 다른 계획의 컨테이너와 계획 해시는 바뀌지 않는다(`test_probe_golden`). `layout.golden`, `records.golden` 은 갱신했다(diff 확인: 새 레코드 세 개와 열거 값, 무작위 레코드의 섹션 이름 참조가 등록 순서에 따라 바뀜).
- **디코더 의미**(SITES 의 `decoder`): `generic`, `spawn-create`(`a[0]` 이름 포인터 — 읽지 않음, `a[1]` 역할 `entry_fn`, `a[2]` 역할 `priority`(4바이트 이하), `a[3]` 역할 `return_address`), `spawn-return`(`a[0]` 상태, `a[3]` 역할 `return_address` = 명령 포인터), `thread-entry`(`a[0]` 알린 id, 4바이트 이하). 이 의미는 정체성 결합이 쓴다. `check_site` 가 이 위치·역할을 요구하고, 정체성 결합도 맞지 않는 사이트를 받으면 오류로 끝난다.
- **진입·복귀 짝**: 진입 사이트와 복귀 사이트가 기호가 같고 **둘 다 `a[3]` 에 역할 `return_address`(8바이트)를 선언하면** 짝짓기 후보이다(`check_plan` 이 요구한다). 진입에서는 스택 꼭대기의 단어, 복귀에서는 표본의 명령 포인터.
- **2026-10-16 개정(검토 반영)**: 열거 값만 더했다 — `KZ_INSN_CLASS` 에 `ret-trampoline`(3; PERTURBATION 의 복귀 행 전용, 사이트의 `insn_class` 에는 쓰지 않는다), `KZ_PERTURBATION_KIND` 에 `proc-capture`(5), `KZ_THREAD_FLAG` 에 `match-by-time`(0x400)과 `id-unverified`(0x800). 레코드 레이아웃과 섹션 버전은 그대로이므로 그 전의 실행 컨테이너(`env/logs/s1r2_p1.kzt`)도 읽힌다. 헤더 주석의 두 문장도 바로잡았다: 시각 순서는 서로 다른 thread 사이의 HB 간선이 될 수 없고(세는 도구는 한 시계의 증거로 쓸 수 있다), `a[]` 는 `generic` 에서는 역할로, 디코더와 짝짓기에서는 검사된 위치로 읽는다.
- 실행 컨테이너의 부모는 basis 컨테이너이고, 헤더의 `profile_sha256` 은 프로필 **텍스트**의 해시이다. 컨테이너의 내용에 벽시계 시각은 없다(`CLOCK_MONOTONIC` 의 값은 사건의 `t_ns` 로 들어 있다).
- 이 개정으로 스키마 fingerprint 가 바뀌었다. `layout.golden`, `records.golden` 은 갱신했고(K0 소유의 golden 이므로 diff 를 읽어 확인했다), 관측기의 basis 와 실행 컨테이너 golden 은 `tests/cfs-free/golden/` 에 따로 있다(`KZ_GOLDEN_UPDATE=1 ctest -R test_probe_golden`). 2026-10-16 개정도 같다. 그때 `kz_facts.h` 도 다른 작업에서 바뀌고 있었으므로, 두 golden 은 **마지막 스키마 변경 뒤에 다시** 만들어야 한다.

## 10. 스키마 개정 2 (SPEC rev 1.3 §2.3, 변경표 X-01, 2026-10-24) [D][M]

SPEC rev 1.3 의 "Formats added in revision 1.3"(ADR-054–ADR-056, C-17)을 구현한 것이다. 정한 것은 레이아웃과 어휘뿐이고 연구 조건은 없다.

**버전을 올린 방식** [D]. 기존 레코드의 배치는 하나도 바꾸지 않고 **새 섹션만** 더했다. 그래서 기존 섹션의 `schema_version` 과 `KZ_ABI_VERSION`(1)은 그대로이고, 레코드 집합의 개정 번호 `KZ_SCHEMA_REVISION` = 2 를 새로 두었다. 이 번호는 `layout_text()` 의 첫 줄(`kz-layout abi=1 revision=2`)에 들어가므로 스키마 fingerprint 와 모든 tool hash 가 바뀐다. 이렇게 한 이유: 레이아웃이 바뀐 섹션은 읽는 쪽이 정확한 버전만 받으므로(§1, `container_reader.cpp`) 기존 섹션을 고치면 이전 컨테이너를 읽는 변환기가 따로 있어야 하고, 그 변환이 빈 칸을 "정상 값"으로 채울 위험이 있다. 새 섹션만 더하면 이전 컨테이너(골든 표본 컨테이너, `env/logs/s1r2_p1.kzt`)는 쓴 그대로 읽힌다 [M: 기존 golden 컨테이너 dump 는 바뀌지 않았고 `layout.golden`, `records.golden` 만 바뀌었다].

**이전 컨테이너의 뜻** [D]. 새 섹션이 없는 컨테이너는 요약·적합성 기록·소유 앱이 **없다(absent)** 고 읽는다. 없음은 결코 정상 값으로 채우지 않는다: 요약이 없으면 관측 판정을 주장하지 않고, ACTOR_OWNERS 가 없으면 모든 thread 의 소유 앱은 `undetermined` 이지 "소유 앱 없음"이 아니며, 적합성 기록이 없으면 시작 상태·writer 포괄·손실 없음은 확립되지 않은 것이다. 실행 컨테이너의 손실 여부는 원래 뜻 그대로 RUNMETA 의 `loss-present` 와 LOSS 섹션으로 읽는다(§9).

| 섹션 (id) | 레코드 | 내용 |
|---|---|---|
| SUMMARIES (84) | `kz_summary` 44 B | 판정 셀 하나의 요약 뷰. `observation`(KZ_OBS: `none`, `OBSERVED_VIOLATION`, `NO_VIOLATION_IN_CHECKED_PREFIX`, `INCONCLUSIVE`, `NOT_APPLICABLE`), `obs_reasons`(KZ_WITHHOLD 집합), `prediction`(KZ_PRED: `NOT_RUN`, `MODEL_CANDIDATE`, `NONE_IN_SCOPE`, `INCONCLUSIVE`) 과 `pred_reasons`, `confirmation`(KZ_CONF: `NOT_RUN`, `CONFIRMED`, `NOT_CONFIRMED`) 과 `conf_reason`(문자열; SPEC 이 어휘를 정하지 않음), `relation`(KZ_EXEC_RELATION), 요약의 근거가 된 `status`(KZ_OCC) 와 `status_flags`(KZ_STATUS_FLAG: `open` = OPEN, `undetermined` = 실제 기록의 설명들이 갈림), `flags`(KZ_SUMMARY_FLAG: `analysis-input-error`, `partial`, `caller-record`, `accept`, `real-record`, `outcome-narrowed`, 그리고 2026-10-24 W2 잔여에서 더한 `relation-inferred`(0x40: 실행 관계가 `inferred` 인 `app_owner` 를 하나 이상 썼다)와 `core-status-unknown`(0x80: 소유 앱이 서로 달라 `inter-application` 이지만 기록이 두 앱의 코어 서비스 여부를 세우지 못해 `core-application` 을 배제하지 못함), 2026-10-25(PI 결정 P1·P10, 단계 A)에 더한 **판정 근거**(하나 이상: `basis-direct-capture` 0x100 = 고정된 조건이 기록된 인자 값만 읽음, `basis-state-order` 0x200 = 재생 상태(관측 효과·동기화·생성 순서·정체성)를 읽음, `basis-return-derived-order` 0x400 = 그 상태의 순서가 reads_from 간선(반환값·같은 id 에서 유도)에 기댐, 그리고 기존 `outcome-narrowed` 0x20 = 반환 코드가 조건을 직접 좁힘), **쓴 신뢰 가정**(`assumes-via-key-separation` 0x800 = `bind … via` 키 분리 선언으로 센 writer 포괄, `assumes-owner-identity-targets` 0x1000 = A-OWNER, `assumes-role` 0x2000 = 호출 문맥의 역할 선언; RF 간선은 0x400 이 드러냄), **P10 실행 관계**(`relation-unconfirmed-influence` 0x4000 = 객체에 연결되지 않은 쓰기를 따로 둠, `relation-not-app-actor` 0x8000 = 관계가 앱 아님이 확인된 행위자(초기 thread)를 씀)), 그리고 `cell`, `model`, `adequacy`, `identity_first`/`n_identities` 인덱스 |
| MODEL_IDS (85) | `kz_model_id` 44 B | SPEC §1.8 의 `model_id`, `model_level`(KZ_MLEVEL), `schedule_profile_id`(와 `sigma`), `basis_id`, `scope_id`, 그리고 관측 결과를 해석한 프로필 `profile_id` |
| OBS_ADEQUACY (86) | `kz_obs_adequacy` 88 B | 조건 인스턴스 하나의 관측 적합성: `established`/`absent`(KZ_ADEQ: `writers-covered`, `aliases-covered`, `start-state-known`, `lifetime-known`, `probes-attached`, `probes-active`, `loss-free`), 그것이 부르는 보류 사유 `reasons`, `start_evidence`(KZ_START_EVIDENCE: `none`, `init`, `state-capture`, `declared`), `end_reason`(KZ_INTERVAL_END), 판정 구간 `first_event`–`last_event`, `loss_reports`, 붙은 프로브 목록 `probes`(REF_LISTS), 관측되지 않은 효과·지점 이름 `unobserved` |
| IDENTITIES (87) | `kz_identity` 68 B | 한 사건에서 한 행위자의 정체성 **네 범주**(섞지 않음): ① 실행 task(`exec_tid`, `exec_generation`, `exec_name`) ② 생성 실행자(`creator_tid`, `creator_generation`, `creator_name`; `creator-known`) ③ 소유/생성 앱(`owner_key`, `owner_name`, `owner_level` KZ_OWNER_LEVEL `undetermined`/`inferred`/`confirmed`, `owner_evidence` KZ_OWNER_EVIDENCE: `creation-request`, `fork-in-creating-call`, `announced-id`, `es-records`, `match-by-time`, `id-unverified`, `raw-thread`, `analysed-table-excluded`) ④ 전달된 식별자(`passed_id`, `passed_role`; `passed-id-present`) |
| ACTOR_OWNERS (56) | `kz_app_owner` 32 B | 실행 컨테이너에서 thread 생애(THREADS 인덱스 `thread`)마다 `app_owner`: 생성 실행자 `creator_thread`, 생성 요청 사건 `creation_event`, 소유 앱의 키 `owner_key`(프로필의 owner 종류의 키; 이름만 알면 `KZ_NONE32`)와 이름 `app_name`, `level`, `evidence`. 이 섹션에 레코드가 없는 thread 의 소유 앱은 `undetermined` 이다 |

- **0 값**: 모든 새 열거형에서 0 은 가장 보수적이다(`KZ_OBS` `none`, `KZ_PRED`/`KZ_CONF` `NOT_RUN`, `KZ_EXEC_RELATION` `ownership-undetermined`, `KZ_OWNER_LEVEL` `undetermined`, `KZ_START_EVIDENCE` `none`, `KZ_INTERVAL_END` `unknown`). `test_core_schema` 가 확인한다 [M].
- **요약 규칙**: `kz::summarize_observation`(`verdict.hpp`)이 SPEC §1.10 표를 D2 대로 옮긴다. `VIOLATING` → 모든 설명에서 위반일 때만 `OBSERVED_VIOLATION`(아니면 `INCONCLUSIVE(explanation_ambiguous)`), `READY` 와 caller 기록·ACCEPT 가 있는 `TOLERATED` → `NO_VIOLATION_IN_CHECKED_PREFIX`, `NOT_READY_UNJUDGED` → `INCONCLUSIVE(caller_requirement_absent)`, `PENDING`·`OPEN` → `INCONCLUSIVE(horizon-open)`, `NOT_EVALUATED` → `INCONCLUSIVE(not_evaluated)`, 결정되지 않은 상태 → 기록이 준 사유(없으면 `explanation_ambiguous`). 어떤 상태도 `NOT_APPLICABLE` 로 가지 않는다. 설명 집합이 빈 경우(분석 입력 오류)는 `observation = none` 에 `analysis-input-error` 이며 위반도 위반 없음도 아니다. `validate_summary` 가 저장된 요약의 불변식을 검사한다 [M: `test_core_verdict`].
- **어휘 고정**: `test_core_vocab` 이 KZ_OBS, KZ_PRED, KZ_CONF, KZ_EXEC_RELATION, KZ_WITHHOLD(SPEC §4.8 순서), KZ_OWNER_LEVEL, KZ_STATUS_FLAG 와 여섯 상태 KZ_OCC 를 SPEC 목록과 대조한다 [M].
- 정체성 네 범주는 SPEC §1.2 의 사건별 필드와 이렇게 대응한다: 실행 task = `exec(e)`, 소유/생성 앱 = `owner(e)` 의 생성 측 근거판(`app_owner`, ADR-056), 전달된 식별자 = `stamp(e)`. 생성 실행자는 `creator_event` 의 실행 주체이다. `svc(e)`(활성 프레임 스택)는 이 레코드에 없다 [I: 필요해지면 따로 둔다].

## 11. 재생 출력 컨테이너 (`kz replay --out`, 2026-10-24, W2 잔여) [D][M]

`kz replay --out <file.kz>` 는 실제 기록 재생의 결과를 개정 2 섹션에 담은 컨테이너 하나를 쓴다(`include/kz/trace/replay_container.hpp`, `lib/trace/replay_container.cpp`). 새 레코드나 섹션은 없다. 이미 정의한 섹션을 처음으로 채운다. 종류는 `verdicts`, 부모는 재생한 실행 컨테이너(종류 `run`, role 0)이고, 헤더의 basis 해시는 실행 컨테이너의 basis 해시, profile 해시는 기록을 해석한 Φ 프로필의 해시(`phi::profile_hash`)이다. 시각·호스트 정보가 없어 같은 입력은 같은 바이트를 낸다 [M: `test_trace_replay_out`].

| 섹션 | 내용 |
|---|---|
| SUMMARIES | 발생 하나에 한 레코드(재생 결과의 순서). 요약 뷰와 실행 관계(`relation`, 플래그 `relation-inferred`·`core-status-unknown`; 2026-10-25 부터 P10 규칙의 관계와 `relation-unconfirmed-influence`·`relation-not-app-actor`), 판정 근거와 신뢰 가정의 플래그(위 SUMMARIES 행; `docs/phi_reference.md` §7.6). BASIS 항목 `replay.relation_rule`(`P10-occurrence`, 또는 민감도 읽기 `sensitivity-clearing-writes-counted|excluded`)이 예전의 `replay.clearing_writes` 를 대신한다. `cell` = none(재생은 판정 셀을 쓰지 않는다), `model` = 0, `adequacy` = 그 발생의 OBS_ADEQUACY 레코드, `identity_first`/`n_identities` = 그 발생의 정체성 |
| MODEL_IDS | 한 레코드. 관측 결과이므로 `sigma` = `sigma_E`, 모델·스케줄 프로필 없음(`model_id`·`schedule_profile_id` 빈 문자열, `model_level` unset), `basis_id` = 실행의 basis 해시(hex), `profile_id` = 해석 프로필 해시(hex). `scope_id` 는 비운다(재생이 세우지 않음) |
| OBS_ADEQUACY | 발생마다. `established` = 조건이 읽은 것에 대해 기록이 세운 적합성, `absent` = 0, `reasons` = 발생의 보류 사유, `start_evidence`, 판정 구간(재생 trace 의 사건 위치: 호출부터 결정 또는 반환까지), 그 구간과 겹치는 손실 구간 수, `end_reason`(결정 / 손실 / 프로세스 종료 / 기록 끝). `probes` = 그 발생의 조건이 기대는 부모 SITES 의 목록(REF_LISTS 안, 아래), 목록이 비거나 부모가 없으면 null. `unobserved` = 관측되지 않은 조건 지점·writer 효과(`OP.label`)와 정적 닫힘의 관측되지 않은 writer·별칭 위치(`<종류|위치>: …`), 닫힘을 주지 않았으면 `<종류|위치>: static closure not given`, `;` 로 구분(2026-10-24 R2 잔여; 그 전의 컨테이너는 null 과 빈 문자열 = "기록 안 함") |
| REF_LISTS | (2026-10-24 R2 잔여) `probes` 의 목록들. 목록 하나는 부모 실행 컨테이너의 SITES 를 가리키는 참조(`artifact_sha256` = 실행 컨테이너 해시, `section` = SITES, `index` = 사이트 id)가 이어지고 **null 참조로 끝난다**. 같은 목록은 한 번만 쓴다. 실행 컨테이너의 사이트는 기록 전체에 걸쳐 붙어 있고 활성이었다(붙이지 못하면 관측기가 실행을 실패시킨다) |
| IDENTITIES | 발생마다 사용의 실행 task 가 먼저(전달된 식별자 `passed_id`·`passed_role` 포함), 이어서 실행 관계가 쓴 등록·게시 행위자. 각 레코드는 네 범주를 따로 적는다: 실행 task(tid·세대, `exec-known`), 생성 실행자(tid 를 알면 `creator-known`; 이름만 알면 이름만), 소유 앱(레코드 키·수준·근거·이름), 전달 식별자 |
| ACTOR_OWNERS | 실행 컨테이너의 것을 그대로 복사한다(문자열만 다시 넣음). **`thread`, `creator_thread`, `creation_event` 는 부모 실행 컨테이너의 THREADS·RAW_EVENTS 인덱스이다.** 이전 컨테이너(ACTOR_OWNERS 없음)를 재생하면 이 섹션도 없다 |
| BASIS | 호출자가 준 재생 입력: `replay.mode`, `replay.outcome_narrowing`(P1 의 현행 `on` / 대안 `off`), `replay.const.<이름>`, `replay.version.<성분>`, `replay.predicate`, `replay.core_request_caller`, `replay.facts.sha256`·`replay.static_basis.sha256`(정적 닫힘을 줬을 때). policy 는 `unset` 이다. 값만 기록하며 그 출처는 호출자가 밝힌다(이 writer 는 확인하지 않는다) |

`kz-dump` 가 이 컨테이너를 다른 컨테이너와 같은 정규 dump 로 읽는다 [M: v153 pilot 재생 출력, `test_trace_replay_out` 의 dump 검사].

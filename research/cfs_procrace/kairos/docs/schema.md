# KZ 스키마 노트

- 범위: `include/kz/schema/` 의 레코드 정의와 `lib/core` 의 컨테이너 형식. 규범 원문은 `Architecture/spec/SPEC_FINAL.md` §2.3 이고, 이 문서는 그 구현(S0, K0)에서 정한 세부와 **원문과 달라진 곳**을 적는다.
- 표기: **[D]** 설계 결정, **[M]** 이번 구현에서 실행해 확인한 사실, **[I]** 계획이며 아직 확인하지 않음. 값은 단위 시험 값이며 연구 조건이나 실험 결과가 아니다.
- 용어(T1): 판정 대상은 *readiness specification*(준비 상태 명세), 그 Req는 *readiness precondition*, 위반은 *readiness violation*이다. 식별자에는 `rspec` / `rprecondition` 형태를 쓰고 *contract* 를 쓰지 않는다. 원문 SPEC의 `Φ` 는 식별자에서 `profile` 이다.

## 1. 컨테이너 형식 [D]

모든 산출물은 하나의 KZ 컨테이너다. 모든 값은 little-endian이고(다른 바이트 순서의 호스트는 `kz_schema.h` 가 컴파일을 거부한다), 모든 영역은 8바이트 경계에 놓이며, 빈틈(padding)은 0이다. 섹션 100–119(중립 사건 층)의 레코드는 struct 바이트를 복사하지 않고 필드 목록에서 한 필드씩 little-endian으로 쓰고 읽는다(§12.1).

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
| 100–119 | `kz_neutral.h` (스키마 개정 3, 2026-10-26; §12. **필드 단위 little-endian 직렬화**) | NRUN : kz_nrun (168), NSITES : kz_nsite (104), NSLOTS : kz_nslot (64), NEVENTS : kz_nevent (88), NVALUES : kz_nvalue (24), NACTORS : kz_nactor (184), NLOSS : kz_nloss (72), NCTL : kz_nctl (64) |
| 120–139 | `kz_facts.h` 둘째 추출 범위 (스키마 개정 7, 2026-10-30; 추출 레코드 개정 5, §15. 구조체 복사 직렬화) | P0_TABLE : kz_fact_p0 (64), OP_DERIV : kz_fact_opderiv (40), SPAWN_WRAP : kz_fact_spawnw (56) |

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

2026-10-28(개정 4)부터 발생별 근거 섹션 OCC_BASIS·OCC_EDGES와 읽기 레코드 REPLAY_READ 가 더해진다(§13, `docs/replay_output.md`). SUMMARIES 는 그대로다.

## 12. 스키마 개정 3: 중립 사건 층 (SPEC rev 1.4 §2.3, 2026-10-26) [D][M]

- 근거: SPEC rev 1.4 §2.3 "Formats in revision 1.4", 결정 기록 `Architecture/spec/PI_DECISIONS_RESOLVED_20261026.md`(PD26) §2 항목 5, IR §9.1–§9.2(IR은 2026-10-26 계측 아키텍처 검토(사용자 업로드)). 연구 조건은 정하지 않았다. 아래 수·이름은 단위 시험 값이거나 파일럿 기록의 값이다.

**무엇을 바꿨나** [D]. 실행 컨테이너에 **새 섹션 100–107**(중립 사건 층, `include/kz/schema/kz_neutral.h`)을 더했다. 개정 2와 같은 방식이다: 기존 레코드의 배치·섹션 버전·필드 이름은 하나도 바꾸지 않았고, `KZ_SCHEMA_REVISION` = 3 이 `layout_text()` 첫 줄(`kz-layout abi=1 revision=3`)에 들어가 schema fingerprint와 모든 tool hash가 바뀐다. 새 열거형의 값 0은 모두 가장 보수적이다(`unknown`/`none`/`unresolved`/`undetermined`). 필드의 의미는 `docs/event_semantics.md` §2(공통 사건 정보), §5(실제 제어 기록), §8.4(손실 종류)를 따른다.

**원시 근거와의 관계** [D]. F0의 기존 섹션(RAW_EVENTS, SITES, SITE_SLOTS, SLOT_FETCHES, THREADS, ACTOR_OWNERS, LOSS, RUNMETA, MAPS, SIDE_EVENTS …)은 **백엔드 확장 정보로 그대로** 둔다. 중립 레코드는 원시 근거를 `raw`(이 컨테이너 안의 레코드를 가리키는 `kz_ref`) 또는 `raw_off`(NRUN `raw_sha256`이 가리키는 원시 스트림 — kzrt-dump — 안의 바이트 위치)로 참조한다. 공통 해석(재생·예측)은 원시 근거를 읽지 않는다. 그래서 감사는 가능하고 해석은 백엔드에 묶이지 않는다.

| 섹션 (id) | 레코드 | 내용 |
|---|---|---|
| NRUN (100) | `kz_nrun` 168 B, 1개 | 실행 계열(`backend` F0/kzrt)과 그 안의 백엔드 이름, 시계(`clock`: CLOCK_MONOTONIC / adapter의 시계 / 없음), 시작 근거(`start_evidence`: `exec-mapping` = F0 기록이 대상 파일의 첫 실행 mapping부터, `bootstrap-buffer` = kzrt가 첫 hook부터 정적 버퍼에 손실 없이), `flags`(`loss-evidence` 백엔드가 손실을 보고함, `loss-present`, `owners-recorded`, `complete`, `control-recorded`, `stopped`; 개정 5에서 `recovered`, `dumped`), 개정 5의 끝(`end_kind`, `end_value`, §14), `run_id`·`instance_id`, 판 태그(runtime, adapter, compiler, target), manifest와 원시 스트림의 sha256, F0 RUNMETA 참조 |
| NSITES (101) | `kz_nsite` 104 B | 논리 site: 백엔드의 site id, 재생이 읽는 종류(`call`/`return`/`point`), manifest phase, `label`, `op`, `symbol`(호출이면 그 함수, 점이면 들어 있는 함수), `source`·`line`, 백엔드가 묶은 곳 `at`, binding이 하나면 그 id, `flags`(`attached` 기록 내내 관측, `partial-binding`, `call-site` 호출 측에서 본 호출/반환), F0 `decoder`, SITES 참조 |
| NSLOTS (102) | `kz_nslot` 64 B | site의 값 슬롯: 역할(`role`), 종류(`int`/`text`/`pointer`), 너비, 부호, 출처(`from`: kzrt `arg<k>`·`ret`·`out<k>`, F0 `reg`·`stack`·`mem`·`str`…), SITE_SLOTS 참조 |
| NEVENTS (103) | `kz_nevent` 88 B | 중립 사건: `actor`(NACTORS 색인), `aseq`(actor 내부 순서; `aseq-target` = 대상이 사건 시점에 붙여 빈틈이 손실 증거, 없으면 F0 `tseq`처럼 해석기가 붙임), `site`, `bind`, `link`(같은 호출 인스턴스를 연 사건), 값 범위(`first_value`, `nvals`), `flags`(`clock`, `no-values`=F0 TRUNC, `boot`, `pc`), `t`, `raw`/`raw_off`. 섹션의 순서는 병합 순서이며 actor 사이 순서가 **아니다** |
| NVALUES (104) | `kz_nvalue` 24 B | 관측된 값만(미관측 값은 레코드가 없다 — 0으로 채우지 않음): 사건, 슬롯, 종류, 값(문자열이면 길이), 문자열, `text-truncated` |
| NACTORS (105) | `kz_nactor` 184 B | actor = 실행 안에서 재사용하지 않는 실행 문맥 하나 + 생성 세대 + 백엔드 binding(`linux-tid` F0 정체성 결합의 thread incarnation, `kzrt-actor`, `kzrt-provisional`, `unresolved`)과 그 native key·종류·`backend_id`·process. 정체성 네 범주를 섞지 않는다: 실행자(이 레코드), 생성 실행자 `creator`, 소유 앱 `owner_*`(수준, 근거, 생성 요청이 넘긴 식별 값, 생성 요청 사건, 요청한 코드의 link-time 주소 또는 binding이 이름 붙인 함수), 알린 id. `join_flags` = 정체성 결합의 근거(KZ_THREAD_FLAG), `spawn_return` = 이 actor를 만든 호출의 반환(id로 묶였을 때만), `boot_link` = 이 actor에 합친 임시 actor, `flags`(`join-record`, `process-initial`, `owner-recorded`, `request-addr-known`, `exited`, `held-at-end`), THREADS·ACTOR_OWNERS 참조 |
| NLOSS (106) | `kz_nloss` 72 B | 손실: 종류(F0의 `records-lost`·`throttle`·`ring-reserve-fail`·`tseq-gap`, 기록 없는 `flagged-without-record`, 손실 근거 자체가 없는 `no-loss-evidence`, kzrt의 13종과 개정 5의 `abnormal-end`(§14), `truncated-stream`), 영향을 줄 수 있는 사건 구간 `begin`–`end`(`end` none = 기록 끝까지), 귀속 actor, 센 곳(`cpu-ring`/`actor-buffer`/`boot-buffer`/`key`), LOSS 참조 |
| NCTL (107) | `kz_nctl` 64 B | 실제 제어 기록(계획과 별개): 무엇(`gate-hold` … `timeout`), 사유, `deferred`, 문맥 class, 쓴 actor·hit의 actor·상대 actor, hit의 관측 사건, site, `ctl_seq`(제어기의 순서이며 프로그램 순서가 아님), 점·hit·게이트, `raw_off` |

### 12.1 바이트 순서 [D][M]

- `include/kz/core/record_codec.hpp`(`lib/core/record_codec.cpp`): 레코드의 **저장 형식은 필드 목록이 정한다** — 필드 목록 offset에, 정수 원소마다 little-endian, `kz_ref`는 해시 32 B + section + index, `kz_str`는 off + len. `encode_records`/`decode_records`는 원소를 자기 형으로 읽어 shift로 쓰므로 호스트의 byte order·struct 배치와 무관하다.
- 컨테이너 계층은 섹션 100–119(`section_is_serialized`)를 이 codec으로 쓰고 읽는다(`ContainerWriter::add_section_raw`, `Container::copy_section`). writer가 STRINGS 정규화 때 `kz_str`를 다시 쓰는 부분도 저장 바이트를 명시적 little-endian으로 읽고 쓴다. **기존 섹션은 예전처럼 struct 바이트 복사로 읽고 쓴다**(little-endian 호스트, padding 없는 레코드에서는 둘이 같은 바이트다). `kz_schema.h`의 big-endian 호스트 거부는 그대로다(SPEC rev 1.4 §2.3 "Byte order").
- 시험: 등록된 모든 레코드에서 codec 출력 = struct 바이트(이 호스트), decode∘encode = 항등; `kz_nevent` 하나의 저장 바이트를 손으로 쓴 little-endian 바이트와 대조 [M: `test_trace_neutral`]. 기존 golden 컨테이너의 바이트는 바뀌지 않았다 [M: golden 시험 통과, `layout.golden`·`records.golden`만 갱신].

### 12.2 F0 → 중립 (`kz::trace::neutral_from_f0`, `include/kz/trace/neutral.hpp`) [D][M]

개정 2까지 재생 어댑터(`real_trace.cpp`)가 F0 섹션에 직접 적용하던 규칙을 **그대로** 변환으로 옮겼다. 이제 커널 tid는 정체성이 아니라 binding의 native key다(IR §9.1).

- actor: THREADS의 thread incarnation 하나당 하나(색인 = THREADS 색인, `join-record`), 그다음 사건·생성 관계가 가리키지만 THREADS 레코드가 없는 tid마다 하나(`join-record` 없음). 사건의 actor는 그 시각에 살아 있던 incarnation(같은 tid의, FORK 시각이 그 사건보다 늦지 않은 가장 큰 세대 — 예전 규칙 그대로)이다. 생성 실행자 = FORK의 ptid가 그 FORK 시각에 가리킨 actor. `process-initial` = tid = pid 이고 세대 0.
- 소유 앱: ACTOR_OWNERS(thread당 하나; 둘이면 변환이 거부)를 actor에 옮기고, 생성 요청 사건(OWNER_REQUEST decoder, TRUNC 아님)의 return_address 슬롯 값을 MAPS의 적재 편향으로 link-time 주소로 바꿔 `request_addr`로 둔다(코어 여부는 재생이 basis 기호로 정한다 — 예전과 같다).
- `spawn_return`: spawn-create/spawn-return decoder 사건을 같은 tid에서 반환 주소로 짝지은 반환이, id 일치(ID_MATCHED + SPAWN_MATCHED)로 묶인 thread의 생성 사건과 짝이면 그 thread의 actor에 둔다(예전 `bound_at_return`).
- `link`(새 정보, 재생은 쓰지 않음): 같은 actor·같은 함수의 진입/복귀 사이트가 둘 다 return_address 슬롯을 가질 때, 복귀를 같은 반환 주소의 열린 진입에 묶는다.
- 값: 역할이 선언된 슬롯만; 문자열 슬롯은 EVENT_STRINGS에 문자열이 있을 때만(없으면 값 없음 = 미관측); TRUNC 사건은 값 없음(`no-values`). `aseq` = `tseq`(`aseq-target` 아님).
- 손실: throttle = 시작 시각 이후 첫 사건부터 끝 시각 뒤 첫 사건 전까지(끝나지 않았으면 기록 끝까지), 그 밖의 커널 보고 = 처음부터 보고 시각 뒤 첫 사건 전까지, RUNMETA `loss-present`인데 LOSS가 없으면 `flagged-without-record` 전체, RUNMETA가 없으면 `no-loss-evidence` 전체.

### 12.3 kzrt-dump → 중립 (`kz::trace::neutral_from_kzrt`, `include/kz/trace/kzrt_convert.hpp`) [D][M]

규칙은 헤더 주석에 적었다. 요점: manifest site → NSITES(`before`/`after`는 `callee`의 호출/반환, `call-site`; callee가 없으면 `func`의 진입/복귀; 나머지는 점), binding이 모두 bound·inlined·duplicated일 때만 `attached`; ACTOR 레코드 → `kzrt-actor`; 같은 셀 근거(`boot_link`, `same-cell`)의 임시 actor는 그 run actor에 합침(aseq가 이어진다); 다른 임시 actor는 `kzrt-provisional`; 셀 없는 사건(actor 0)은 사건마다 actor 하나(`unresolved`, 없는 프로그램 순서를 만들지 않음); 사건 순서 = 시계 없는 사건(부트스트랩)을 버퍼 순서로 먼저, 그다음 각 actor의 aseq 순서를 지키는 시각 병합; 값 = valid 비트가 선 슬롯, 문자열은 `string=validated`이고 런타임이 복사했을 때만; 손실 = actor 버퍼의 overflow·partial은 그 actor의 첫 aseq 빈틈(CTL·AEXIT 레코드의 aseq도 셈)부터 끝까지, preinit·부트스트랩 partial·excluded·unloaded·reentry·note-table·설명되지 않은 빈틈·잘린 스트림은 전체, ctl-skipped·ctl-nocond·actor-table·unresolved·module-table·after-stop은 보고만 하고 구간이 아님(사건이 기록되었거나 관측 창 밖), 회수 dump의 `abnormal-end`도 같음(§14.1); 시작 근거 `bootstrap-buffer` = 완전한 스트림 + init + record/control 모드 + preinit·부트스트랩 partial·reentry 손실 없음. 정체성: 런타임은 생성자·소유 앱·이름을 기록하지 않는다. manifest에 정체성 읽기(`kzs-sites 2` `identity`, `event_semantics.md` §3)가 없으면 **결정하지 않음**으로 둔다(예전과 같음). 있으면 아래 §12.3.1의 결합을 한다.

#### 12.3.1 계측 기록의 정체성 결합 (2026-10-26) [D][M]

- 코드: `include/kz/trace/identity_rule.hpp`, `lib/trace/identity_rule.cpp`. 두 실행 계열이 **한 규칙**을 쓴다: `owner_from_request`(생성 호출이 시작될 때 생성 actor에 열린 가장 바깥 생성 요청이 앱을 정한다. owner-request는 그 인자, child-request는 생성 task 자신의 app_owner이며 그 수준을 넘지 않는다; 묶음이 id에 기대면 `confirmed`, fork 시각에만 기대면 `inferred`)를 F0의 `identity_join.cpp` 5단계와 계측 계열의 `join_actor_identities`가 함께 부른다. F0의 결과는 바뀌지 않는다 [M: F0 시험·golden 그대로 통과].
- 계측 계열의 묶음 키는 **id 하나**다. 커널 생성 기록(FORK)이 없으므로 시각 포함은 쓰지 않는다. actor마다 aseq 순서(대상의 프로그램 순서)로 읽는다. spawn-create는 생성 호출을 열고 그때 열린 요청을 기억한다. spawn-id(호출 안, 같은 actor)는 배정 id다(한 호출에 둘이면 둘 다 쓰지 않음). spawn-return은 `link`가 가리키는 열린 호출을 닫는다. 그 `id`는 status가 `ok`일 때만 쓰고, spawn-id와 다르면 둘 다 쓰지 않는다(`conflicting`). 요청은 link로 열고 닫는다. owner 값이 관측되지 않은 owner-request는 앱을 정하지 않는다. thread-entry는 알린 id다(첫 번째만).
- 묶음: id X를 배정한 호출이 **정확히 하나**이고 X를 알린 actor가 **정확히 하나**이며 둘이 다른 actor일 때만, 그 actor를 그 호출의 task로 묶는다. 그러면 `creator`(호출한 actor), `creator_event`(spawn-create 사건), `spawn_return`, `announced_id`, `join_flags` = entry-seen·spawn-matched·id-matched, 소유 앱은 위 규칙으로 `confirmed`이고 근거 `creation-request`·`announced-id`(child면 `creator-owner`, 안에 owner-request가 있었으면 `analysed-table-excluded`)다. `fork-in-creating-call`·`match-by-time`은 없다. id가 둘 이상의 호출이나 actor에 있으면 `spawn-ambiguous`이고 묶지 않는다. 묶이지 않은 actor는 `entry-unmatched`(알렸을 때)·`id-unverified`이고 소유 앱은 `undetermined`다("앱 없음"이나 0이 아님, PD26 §4). 이름·시각·actor 사이 기록 순서로는 묶지 않는다.
- 요청의 호출 함수: 요청 site가 호출 쪽 binding(`callee`가 있음)이면 그 site의 `func`를 `request_func`로 둔다(재생의 core 판정이 쓴다). 함수 진입 binding이면 호출 함수를 모른다(정하지 않음).
- 기록: NRUN `owners-recorded`, kzrt-actor·kzrt-provisional actor마다 `owner-recorded`, NSITES `decoder` = 정체성 읽기의 KZ_DECODER, 결합 수(묶음·모호·충돌·성공 아님 등)는 변환 note(BASIS `convert.note`). unresolved actor(셀 없는 사건)는 참여하지 않는다.
- 시험(단위 시험 값): `test_kzrt_identity`(실제 스레드로 런타임이 쓴 dump: id로 묶인 task와 child task의 `confirmed` 소유 앱, 호출 함수, id 충돌·실패한 호출의 out 값·같은 id를 알린 두 task의 거부, 판 1 파일만이면 결합 없음, 재생이 confirmed 2를 읽음; 공용 규칙 단독 검사), `rt_manifest`(판 2 문법) [M].
- 파일럿 기록(단계 C의 GCC plugin 기동 dump, 주 결과 아님): C1 tx(`/home/user/work/instr/gcc/c1/run1/dump/tx.kzrt`, sha256 `61586c46…`)를 `c1_post_1df262a.kzs` + binding 목록 + `c1_post_1df262a.identity.kzs`로 변환하면 알린 id 15개가 모두 id로 묶이고 소유 앱 15개 `confirmed`, 결정하지 않음 1개(초기 스레드; 아무것도 알리지 않음), 호출 밖 spawn-id 2개(OSAL이 OS_TaskCreate 밖에서 만드는 스레드; F0도 같음). (task id, 소유 앱 id, 생성 task) 표가 단계 C 대조 스크립트의 표(`c1/cmp1/identity.gcc`)와 diff 0이고, F0 smoke3 post(`c1_runs/smoke3/post_1df262a/run.kzt`)를 변환한 F0 결합의 표와 수준까지 diff 0이다(15개 모두 `confirmed`). R2(`r2/run1/dump/cpu1.kzrt`, sha256 `0af58a06…`, `r2_v701.identity.kzs`): 24개 묶음, `confirmed` 24, F0 pilot3 결합과 diff 0. 산출물: `/home/user/work/instr/gcc/d1_identity/`(저장소 밖). 다른 실행끼리의 대조이며 같은 표가 나온 것은 시작 순서가 같았기 때문이다 [M].
- 같은 C1 기록을 C1 post 프로필로 재생하면(정적 닫힘 없이) CF 조회 발생 3개의 실행자 정체성이 `exec a13 creator a1 owner confirmed [9]`로 정해진다. 판정은 여전히 `INCONCLUSIVE`이고 관계 `ownership-undetermined`의 이유는 이제 "결정 전에 osal.countsem 의 공개·지우는 쓰기가 없음"(OSAL 안 효과점이 manifest에 없음, 선택 문서 §4의 2·3)이다. 정체성 부족은 원인에서 빠졌다 [M].

호스트 reader(`runtime/tools/decode`)의 항목에 스트림 안 바이트 위치 `offset`을 더했다(`raw_off`의 근거; dump 형식은 그대로) [M].

### 12.4 재생은 중립 층만 읽는다 (`real_trace.hpp`) [D][M]

- `real_trace_from_run` = `neutral_of_run`(컨테이너에 NRUN이 있으면 중립 섹션, 없으면 F0 섹션을 메모리에서 변환) + `real_trace_from_neutral`. 그래서 **개정 3 이전의 모든 실행 컨테이너는 쓴 그대로 읽힌다**: 같은 규칙이 메모리에서 돌 뿐이다.
- actor 이름(`actor_names`): 백엔드 이름, 없으면 `<접두>` + key(`tid`/`a`/`P`/`?`); join-record actor 사이에 겹치는 이름은 `.<접두><key>`, 세대가 0이 아니면 `.g<n>` — F0에서는 예전 이름과 같다.
- `ThreadIdentity`에 `actor_index`·`binding`을 더했다. `tid`는 binding의 native key이며 F0에서는 예전처럼 커널 tid다(기존 필드의 뜻을 바꾸지 않음).
- 새로 읽는 것: 점은 `attached`일 때만 관측된 점; `call-site` 호출/반환의 label이 프로필의 점이면 그 점도 된다(F0는 두 probe로 보는 것을 한 hook으로 본 경우; 반환 뒤·호출 앞에 둔다); 보고만 하는 손실 종류는 구간을 만들지 않는다.
- 호출 안 선형화 점의 인수(2026-10-26, D3 실행에서 추가) [D][M]: 연산의 conjunct·effect `lp at` 점 사건이 그 연산의 non-out 매개변수 값(역할 이름이 같은 슬롯)을 가지고, 같은 actor의 가장 안쪽 열린 호출이 그 연산이며, 호출 사건에 그 `arg.NAME`이 없으면 호출 사건이 그 값을 받는다. 호출 사건이 스스로 가진 값은 바꾸지 않는다. 출력 머리에 `arguments from linearization points inside the call N`이 붙는다(0이면 줄 없음). 이유: GCC manifest는 검증 전 포인터를 읽지 않으므로(PD26 §4) 세마포어 이름을 함수 진입이 아니라 검증 뒤의 `.exists`·`.publish`에서 읽는다(`instr_candidate_gcc.md` §11). 이 규칙이 없으면 C1의 조회·생성 호출에 키가 없어 모든 발생이 `INCONCLUSIVE(effect_unsupported,horizon-open)`였다. F0 기록 9개(smoke3 4, confirm1 자연 rx 4, R2 pilot3)의 재생 출력은 이 규칙 전후로 같다(머리의 `record layer` 줄 외 diff 0) [M]. 시험: `test_neutral_kzrt`(호출에 없는 인수를 받고, 호출이 가진 인수는 그대로).
- `kz replay` 출력에 `record layer:` 한 줄(중립 섹션을 읽었는지, F0 섹션을 메모리에서 변환했는지)을 더했고, kzrt 기록이면 시작 문장과 손실 줄(`loss record: …`)이 달라진다. 재생 출력 컨테이너의 `probes` 목록은 부모에 SITES가 있으면 예전처럼 SITES를, 없으면(계측 기록) NSITES를 가리킨다.

### 12.5 `kz convert` [D][M]

```
kz convert <run.kzt> --out <file.kzt>          # F0: 모든 섹션 그대로 + 중립 층, 입력을 부모(kind run, role 1)로
kz convert --kzrt <dump> --manifest <kzs> [--manifest <kzs>]... [--basis <basis.kz>] --out <file.kzt>
                                               # 계측 기록: 중립 층만 + BASIS(convert.*) + FILE_HASHES(dump, manifest)
```

이미 중립 층이 있는 컨테이너는 다시 변환하지 않는다. 같은 입력은 같은 바이트를 낸다 [M].

### 12.6 확인 [M]

- 단위 시험(값은 단위 시험 값): `test_trace_neutral`(codec; 합성 F0 기록에서 actor·세대·생성자·초기 thread·link·id로 묶인 반환·관측 값·손실 구간; 변환 컨테이너의 F0 섹션 보존·재읽기 동일·재변환 거부·결정성·정규 dump; 원본과 변환본의 재생 trace·보고·판정 동일), `test_trace_neutral_real`(저장소의 `env/logs/s1r2_p1.kzt`에서 같은 동일성), `test_neutral_kzrt`(이 시험 안에서 POSIX 런타임이 두 스레드로 쓴 dump: 임시 actor 합침, aseq 순서, link, call-site 매핑, removed binding, 선언되지 않은 site, 재생과 발생 2개, 잘린 스트림의 손실, 형식 오류 거부). 기본 빌드 139개 중 138 통과 + tracefs 1 건너뜀, LLVM 빌드 161개 중 160 통과 + 같은 1 건너뜀.
- 실제 기록(파일럿·개발 자료, 주 결과 아님): R2 `v153_pilot3`, C1 `smoke3/pre_1df262a`를 `kz convert`로 바꾸고, 2026-10-25 재생 스크립트(`/home/user/work/procrace/integ_20261025/replays.sh`)의 `cur` 읽기와 같은 인수로 (a) 변경 전 kz(HEAD `e7f297a`로 빌드)가 원본을, (b) 새 kz가 원본(메모리 변환)을, (c) 새 kz가 변환본(중립 섹션)을 재생했다. 출력은 머리 줄(경로, artifact hash, `record layer`, `wrote`)을 빼면 셋이 같고, `--emit-trace`의 trace는 (a)와 (c)가 바이트까지 같으며(R2 1353줄, C1 171줄), `--out` 컨테이너는 부모 run hash·tool hash·그 hash가 든 REF_LISTS의 CRC를 빼면 같다. 변환 층: R2 사건 1271·actor 28·값 3061, C1 사건 148·actor 19·값 378, 둘 다 손실 0, 시작 `exec-mapping`. 산출물·스크립트: `/home/user/work/procrace/neutral_b2/{run_equiv.sh,final/}`(저장소 밖, `final/sha256.txt`).
- 계측 기록(파일럿, 주 결과 아님): GCC plugin 후보의 C1 기동 dump(`/home/user/work/instr/gcc/c1/run1/dump/tx.kzrt`, sha256 `61586c46…`, `docs/instr_candidate_gcc.md`)와 그 manifest 둘을 변환해 C1 post 프로필(cf-833fdbb, bundle-1df262a, rspec-833fdbb)로 재생했다: site 16, 사건 117, actor 16, 손실 0, 시작 `bootstrap-buffer`; CF 조회 발생 3개(BP0 1·2회, BP1 1회)가 모두 `INCONCLUSIVE`, 관계 `ownership-undetermined`. 같은 키에서 F0 smoke3 post가 TOLERATED/READY를 낸 것과 다른 까닭은 근거의 부족이다: 정적 닫힘을 주지 않았고(계측 빌드의 basis가 없음), manifest에 OSAL 내부 효과점(생성의 publish 등)이 없으며, 계측 기록의 정체성 결합이 없어 `OS_TaskCreate`의 out id와 소유 앱이 비어 있다. 순서가 달라서가 아니다. 이것은 "변환된 계측 기록을 `kz replay`가 읽는다"의 확인이지 판정 대조가 아니다.

### 12.7 남은 것 [U]

- ~~계측 기록의 정체성 결합~~ → §12.3.1(`kzs-sites 2`). ~~계측 계열의 초기 스레드~~ → 2026-10-26: RUNX 1.2 `process`(`event_semantics.md` §8.6)의 key를 가진 run actor 하나에 `initial`·`process`를 준다. 1.1 dump에는 `kz convert --kzrt … --process PID`(하네스가 기록한 pid, Linux TID = pid인 run actor 하나, BASIS `convert.harness_process`)를 쓴다. 둘 다 아니면 노트 "initial thread: not recorded"다. 재생 문구는 그 actor의 TID가 pid이면 F0와 같은 "thread id = process id"다. 호출 쪽 out id(`spawn-return id=`)를 쓰는 manifest는 아직 없다(지금은 F0처럼 spawn-id).
- (2026-10-26) 열린 호출 안의 점이 준 out 값 중 그 슬롯이 store의 operand인 것(`from=operand0`)은 반환 사건에 text `store.out.X`(site label·사건 번호)로도 표시된다. 머리 줄은 `out values from points inside a call N (k of them the operand of a store)`이다. 프로필 RF 규칙의 `stored_out`만 이 표시를 읽는다(`phi_reference.md` 표 RF).
- ~~NCTL은 기록만 하고 재생이 쓰지 않는다~~ → 2026-10-26 단계 F(P11 결정, PD26 §3): 재생이 NCTL `fire`(사유 `resumed`)마다 제어 간선 하나를 세운다. 시작은 해제 hit의 actor가 그 hit의 관측 사건이나 그 앞에 낸 마지막 사건, 끝은 잡혔던 actor가 같은 게이트의 마지막 `gate-hold` 뒤에 낸 첫 사건이다. 역방향이거나 사건이 없으면 간선을 세우지 않고 그 이유를 적는다. 프로그램 간선만으로 서지 않는 순서가 이 간선으로 서면 근거 종류는 `control-order`다(RF는 그 간선으로도 서지 않을 때만 쓴다). `kz replay --control-order on|off`(기본 on, off = 민감도 읽기 B), BASIS `replay.control_order`, 출력 `realized control:` 줄과 trace의 `control FROM TO LABEL` 줄. 결과는 `plan/results_instr_gate_20261026.md` §3이다. ~~F0의 `hold.kzr`는 실행 컨테이너 밖에 있어 NCTL로 옮기지 않았다. 그래서 F0 기록에는 이 근거가 없다(P11 읽기 A는 confirm2 `classify.awk`가 따로 계산함).~~ → 2026-10-28: `kz convert <run.kzt> --hold <hold.kzr>`가 F0 보류 기록을 NCTL로 옮긴다(§13.1, `docs/replay_output.md` §6).
- `kz hits` 등 F0 섹션을 직접 읽는 다른 명령은 그대로다(계측 기록에는 F0 섹션이 없다).
- adapter 시계(`adapter-clock`)의 병합 순서는 actor 내부 aseq 순서만 지킨다. → 2026-10-28: 재생은 병합 순서를 알려진 순서의 선형 확장으로 만든 뒤 읽는다(§13.2). 간선은 그 순서와 모순일 때만 거부한다.

## 13. 스키마 개정 4: 재생 출력의 발생별 근거 (2026-10-28) [D][M]

- 근거: `plan/results_instr_pilot_20261026.md` §8.2 항목 4(`summary.flags`에 자리가 없어 `store-derived-order`가 텍스트·메모리에만 남음), `STATUS_20261027.md` §6 "근거 종류 보존", PI 결정 2026-10-28(`Architecture/spec/PI_DECISIONS_RESOLVED_20261028.md`). 연구 조건은 정하지 않았다.
- 방식: 개정 2·3과 같다. 기존 레코드의 배치·섹션 버전·필드 이름은 바꾸지 않았다. `KZ_SCHEMA_REVISION` = 4가 `layout_text()` 첫 줄에 들어가 schema fingerprint와 모든 tool hash가 바뀐다. 이전 컨테이너는 쓴 그대로 읽힌다 [M: golden 컨테이너 dump 불변, `layout.golden`·`records.golden`만 갱신].

| 섹션 (id) | 레코드 | 내용 |
|---|---|---|
| OCC_BASIS (108) | `kz_occ_basis` 104 B | 발생마다(SUMMARIES 순서): 판정과 요약 뷰, 근거 종류 전체(KZ_OCC_BASIS: `store-derived-order`, `control-order` 포함), 선언 가정(KZ_OCC_ASSUME), 결과 검사(KZ_OUTCOME_CHECK), 경로 무관 발생 키(rspec, 연산, 호출 문맥 역할, 소유 앱 키·수준·이름, 키 값, 텍스트 인자, 순번), via 이름, 간선 구간 |
| OCC_EDGES (109) | `kz_occ_edge` 24 B | 판정이 기댄 actor 사이 간선(경로 하나): 규칙, 종류(KZ_OCC_EDGE_KIND rule / reads-from / control), `by-store`, 위치 |
| REPLAY_READ (110) | `kz_replay_read` 24 B | 읽기 스위치 전부(KZ_READ_SWITCH, KZ_RELATION_RULE), 제어 간선 수, 선형화가 옮긴 사건 수 |

- 세 섹션은 100–119 범위라 명시적 little-endian codec으로 쓰고 읽는다(§12.1).
- NCTL 플래그 두 개를 더했다(KZ_NCTL_FLAG): `observed-after-control`(0x2, hit의 관측 사건이 제어가 작용한 뒤에 기록됨), `no-observation-event`(0x4, 기록에 그 hit의 관측 사건이 없음, `event` = none). 쓰는 곳은 F0 보류 기록의 변환이다(§13.1).
- 읽는 법, 발생 키, `kz occ show|diff`, 이전 컨테이너의 뜻은 `docs/replay_output.md`에 있다.
- 시험(도구 시험 값): `test_trace_occ_basis`(RF의 두 종류, 제어 순서와 그 간선, 읽기 스위치, 이전 컨테이너 읽기, 키의 경로 무관성, 판정 차이와 근거 차이의 분리) [M].

### 13.1 F0 보류 기록 → NCTL (`kz convert <run.kzt> --hold <hold.kzr>`, `include/kz/trace/hold_nctl.hpp`) [D][M]

- F0 변환(§12.2) 뒤에 hold.kzr의 보류·재개·해제를 NCTL로 더하고 NRUN `control-recorded`를 세운다. BASIS `convert.hold`·`convert.hold.sha256`·`convert.hold.note`, FILE_HASHES에 hold.kzr. 나머지 F0 섹션과 부모는 `kz convert`와 같다.
- 관측 사건은 trap 뒤 그 actor의 첫 사건이고 점의 site에 있어야 한다. 아니면 `event` = none, `no-observation-event`, 이유는 note. 찾으면 `observed-after-control`. 재생은 그 플래그가 있을 때 간선 시작을 해제 hit 관측의 앞 사건으로, 끝을 게이트 관측 사건 자체로 둔다.
- 규칙과 confirm2 36회 결과는 `docs/replay_output.md` §6. 시험(도구 시험 값): `test_trace_hold_nctl`(기록 해석, 사실별 대응, 사건 없음의 다섯 경우, 컨테이너 보존, 재생의 제어 간선 끝점) [M].

### 13.2 병합 순서의 선형화 (`eval::linearize_real_trace`) [D][M]

- 형식 변경은 없다(REPLAY_READ `reordered`만 기록). 어댑터가 trace를 만든 뒤, 사건 순서를 프로그램 순서·첫 패스의 규칙 간선(바탕)과 제어 간선·reads_from·spawn 짝(후보)의 선형 확장으로 만든다. 병합 순서는 동률 해소로만 쓴다. 이미 선형 확장이면 그대로다.
- 간선은 병합 위치로 거부하지 않는다. 바탕과 모순인 후보, 그리고 다른 후보와 함께 순환을 이루는 후보는 모두 거부한다(이유를 적음). 거부는 간선 집합으로만 정해지고, 병합 방향이나 후보의 순서에 기대지 않는다.
- 선형화는 재생의 읽기 선택으로 한다(`RealTraceOptions::reading`). 제어 순서·RF·반환 코드 사용을 끈 읽기는 그것들로 사건을 옮기지 않는다. 손실 구간은 재생이 읽는 절단(시작부터 기록된 쓰기가 아무것도 정하지 않음)을 지키게 옮긴다.
- 고치기 전후의 도구 시험과 남은 병합 순서 사용은 `docs/replay_output.md` §7. 시험: `test_trace_merge_order` [M].

## 14. 스키마 개정 5: 회수 기록의 끝, 발생 키의 task (2026-10-28) [D][M]

- 근거: `docs/event_semantics.md` §12.6(런타임 0.4.0의 회수 dump를 변환기가 옮기는 일), 2026-10-28 분석 패키지 검토의 발생 키 지적(`docs/replay_output.md` §3). 연구 조건은 정하지 않았다.
- 방식: 레코드 크기·offset·섹션 버전은 바꾸지 않았다. 예약 필드 셋에 이름을 주고 열거 값을 더했다. 예약 필드는 이전의 모든 컨테이너에서 0이고(쓰기·읽기가 0을 강제했음), 0은 "기록 안 됨"으로 읽힌다. 그래서 이전 컨테이너는 쓴 그대로 읽힌다. `KZ_SCHEMA_REVISION` = 5가 `layout_text()` 첫 줄에 들어가 schema fingerprint와 모든 tool hash가 바뀐다 [M: `layout.golden`·`records.golden`만 갱신].

| 레코드 | 필드·값 | 뜻 |
|---|---|---|
| `kz_nrun` | `end_kind`(옛 `rsv0`, u8, KZ_NEND) | 기록이 말하는 실행의 끝: `not-recorded`(F0, kzrt-dump 1.3 이하), `in-process`(프로세스가 스스로 쓴 dump), `signal`·`exit`(프로세스가 끝난 뒤 저장소에서 회수한 dump, 하네스가 준 시그널·종료 코드), `unstated`(회수했지만 하네스가 아무것도 주지 않음) |
| `kz_nrun` | `end_value`(옛 `rsv1`, u32) | `signal`·`exit`의 번호. 그 밖에는 0 |
| `kz_nrun` `flags` | `recovered`(0x40), `dumped`(0x80) | `recovered`: dump 머리의 회수 비트(또는 RUNX `end_kind` recovered). 이 기록에는 `complete`를 두지 않는다. `dumped`: 프로세스가 `kz_rt_stop` 뒤 자기 dump를 끝낸 뒤에 회수했다(그 dump와 같은 기록) |
| `kz_nloss` `kind` | `abnormal-end`(21) | 회수 dump이고 `dumped`가 아니다. 기록은 저장소가 가진 앞부분이다. 사건 안의 구간은 없다(`begin` = `end` = 사건 수). `loss-present`는 둔다 |
| `kz_occ_basis` | `task`(옛 `rsv0`, u16) | 실행 task가 소유 앱 안에서 몇 번째 task인지(생성 순서, 1부터). 0 = 정해지지 않음 |
| `kz_replay_read` `format` | 2 | OCC_BASIS `task`를 채웠다. 개정 4의 컨테이너는 1이다 |

### 14.1 회수 dump → 중립 (`kz convert --kzrt`, `lib/trace/kzrt_convert.cpp`) [D][M]

- 머리의 회수 비트와 RUNX `end_kind`가 다르면 note를 남기고 회수로 본다.
- `end_kind`·`end_value`는 RUNX `end_flags`(signal·exit)와 `end_value`에서 온다. 프로세스 안의 1.4 dump는 `in-process`다. 1.3 이하 dump는 `not-recorded`이고 중립 층이 예전과 같다.
- LOSS kind 14는 `abnormal-end`가 된다. 예전에는 모르는 kind라 `unknown`, 구간이 처음부터 전체였다. 그래서 회수 앞부분 전체가 손실 구간 안에 있었다.
- 끝날 때 hook 안에 있던 actor는 런타임이 그 actor의 LOSS `partial`(count 1)을 쓴다. 변환은 예전처럼 그 actor의 마지막 사건 뒤부터 끝까지의 구간으로 옮긴다(actor 버퍼의 partial과 같은 표현).
- `dumped`가 아닌 회수 dump에 `abnormal-end`가 없으면 하나를 더하고 note를 남긴다. 끝은 note(`end: …`, 저장소 `file`/`memory`/`refused`)로도 남는다.
- 시작 근거는 끝과 무관하다. 스트림이 온전하면(END, CRC) 회수 dump도 `bootstrap-buffer`를 받을 수 있다.
- 재생: `abnormal-end`는 구간을 만들지 않는다(보고만). 재생 출력에 `record end: …` 한 줄이 붙는다. 끝이 기록되지 않은 기록(F0, 1.3 이하)에는 줄이 없다. `kz convert` 출력에는 `end: …` 줄이 붙는다.
- 시험(도구 시험 값): `test_kzrt_recovered`. 런타임 시험 `rt_end_kill`·`rt_end_segv`·`rt_end_normal`이 남긴 dump를 FIXTURES로 읽는다. SIGKILL: 손실 구간 0, `abnormal-end` 하나(구간 없음), `recovered`·`loss-present`, `complete` 없음, `signal 9`. SIGSEGV: 위에 더해 main actor의 `partial`이 그 actor의 마지막 사건 뒤에서 시작하는 유일한 구간이고, main의 300 기록은 모두 그 앞에 있다. 정상 종료 뒤의 회수: `recovered`·`dumped`, `exit 0`, `abnormal-end` 없음, trace가 프로세스의 dump와 같다. 프로세스의 dump: `in-process`, `complete`. 시작 근거는 셋 모두 프로세스의 dump와 같다 [M].

### 14.2 발생 키의 task (`real_trace.hpp` `ThreadIdentity::task_ordinal`) [D][M]

- 두 경로가 다 가진 것만 쓴다. F0와 계측 기록 모두 actor의 소유 앱(`owner_id`, 같은 규칙 `identity_rule.hpp`)과 생성 요청 사건(`owner_request`: F0는 ACTOR_OWNERS의 creation_event, 계측은 결합이 찾은 요청)을 가진다. task 이름 텍스트는 F0에만 있다(계측 manifest의 이름 인자는 포인터만). 진입 함수 주소는 빌드마다 다르다. 그래서 둘 다 쓰지 않는다.
- 규칙: task의 생성 경로 = 그 요청을 한 actor가 같은 앱을 위해 한 요청 가운데 몇 번째인지(그 actor의 aseq 순서). 요청한 actor가 같은 앱의 task면 그 actor의 경로 뒤에 붙인다. 앱의 task를 경로 순서로 놓은 위치(1부터)가 `task`다. 요청이 없거나, 순환이거나, 다른 task와 경로가 같으면(요청 하나에 task 둘) 0이다.
- actor 이름·tid·시각·병합 순서는 쓰지 않는다. actor 하나의 프로그램 순서만 읽는다.
- 재생 출력 텍스트는 바뀌지 않는다(`--out` 컨테이너의 OCC_BASIS와 REPLAY_READ만).
- 시험: `test_kzrt_identity`(한 앱의 첫 task 1, 그 child 2, 소유 앱이 정해지지 않은 actor 0) [M].

### 14.3 보존 기록 (개정 전후) [M]

고치기 전 kz(a13b37a 빌드)와 고친 kz로 같은 명령을 돌렸다. 파일럿·개발 기록이며 주 결과가 아니다.
- R2 `v153_pilot3`(F0, 중립 층을 메모리에서 변환): `cur`·`p1off`·`idoff` 읽기의 출력 텍스트가 `wrote` 줄을 빼고 같다. `--emit-trace` 출력(1353줄)도 같다. `--out` 컨테이너는 `kz occ diff`로 269/269가 맞고 차이 0이다(한쪽이 format 1이라 task 없이 맞춤).
- confirm2 `W1_post_1df262a_tx`(F0 + `hold.kzr`): 고치기 전 kz가 변환한 컨테이너를 두 kz가 읽으면 `cur` 텍스트가 `wrote` 줄을 빼고 같다. 각 kz가 스스로 변환하면 `cur`·`p1off`·`ctloff_p1off` 텍스트가 둘째 줄(변환 컨테이너의 artifact hash; tool hash가 다름)과 `wrote` 줄을 빼고 같다. 그 G-I-H 짝(단계 F 변환 기록)의 `cur`·`p1off`도 `wrote` 줄을 빼고 같다.
- 고친 kz의 `kz occ diff`(F0 + hold.kzr 대 G-I-H, `cur`·`p1off`): 25/25가 맞고 판정·근거 차이 0, 종료 코드 0. 두 경로 모두 키의 소유 앱이 `app[9]#1`이다. 그래서 task 자리가 두 경로에서 같게 정해진다.
- 변환: confirm2 36회의 `kz convert --hold` 결과와 G-I-H 1.3 dump 하나의 `kz convert --kzrt` 결과가 정규 dump에서 tool hash·artifact hash 줄을 빼고 같다. 그래서 1.4 이전 dump와 F0 기록의 중립 층은 바뀌지 않았다.

## 15. 스키마 개정 7: 프로필 없는 추출의 세 섹션 (2026-10-30) [D][M]

- 근거: `plan/extraction_design_20261030.md` §3.0(P0 표, F7, F8), §6 단계 2a 항목 1–2. 연구 조건은 정하지 않았다. 아래 수는 단위 시험·고정 도구의 값이다.
- 방식: 개정 2–6과 같다. 기존 레코드의 배치·섹션 버전·필드 이름은 바꾸지 않았다. 추출 범위 10–39가 다 차서 둘째 추출 범위 120–139를 두었다(100–119는 필드 단위 직렬화 범위라 피했다). `KZ_SCHEMA_REVISION` = 7이 `layout_text()` 첫 줄에 들어가 schema fingerprint와 모든 tool hash가 바뀐다.
- 새 섹션: P0_TABLE(120, `kz_fact_p0` 64 B), OP_DERIV(121, `kz_fact_opderiv` 40 B), SPAWN_WRAP(122, `kz_fact_spawnw` 56 B), 모두 버전 1. 새 열거형 `KZ_P0_KIND`, `KZ_P0_FLAG`, `KZ_OPDERIV_KIND`, `KZ_OPDERIV_MODE`, `KZ_OPDERIV_FLAG`, `KZ_SPAWNW_KIND`, `KZ_SPAWNW_FLAG`. 값 0은 모두 `none`이다. TERMINALS의 flags에 `conditional`(0x8)을 더했다. 뜻은 `docs/static_facts.md` §5a.
- **프로필로 하는 추출은 세 섹션을 쓰지 않는다.** 그래서 그 facts의 기존 섹션은 바이트 그대로다 [M: C1 pre tx, R2 core, C5 6.4.1의 V0·V2를 새 도구로 다시 뽑아 kz-dump 비교. 섹션 CRC가 다른 것은 STRINGS뿐이고, 레코드 줄 차이는 build identity 항목과 basis 해시뿐이다. `env/redesign/step2a_s1.sh identity`].
- golden: `layout.golden`은 새 레코드·열거형·섹션 줄이 더해지고 revision·fingerprint 줄이 바뀌었다. `records.golden`은 새 레코드 줄이 더해지고, 패턴으로 채운 참조(`…/<섹션>#n`)가 가리키는 섹션 이름이 섹션 목록이 늘어난 만큼 밀렸다(개정 4·5 때와 같은 기계적 변화). TERMINALS의 패턴 레코드는 `conditional` 비트를 표시한다. 나머지 golden(`container_*`, `render`, `tool_hash`, `tests/static/data/*.golden`)은 그대로다 [M].

